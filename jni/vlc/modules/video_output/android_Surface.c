
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_vout_display.h>
#include <vlc_picture_pool.h>
#include <vlc_threads.h>

#include <dlfcn.h>
#include <pixman.h>

void LockSurface();
void UnlockSurface();
void *GetSurface();

static void *InitLibrary();

static int Open (vlc_object_t *);
static void Close(vlc_object_t *);

vlc_module_begin()
    set_shortname("AndroidSurface")
    set_category(CAT_VIDEO)
    set_subcategory(SUBCAT_VIDEO_VOUT)
    set_description(N_("Android Surface video output"))
    set_capability("vout display", 25)
    add_shortcut("android")
    set_callbacks(Open, Close)
vlc_module_end()

static picture_pool_t *Pool(vout_display_t *, unsigned);
static void Display(vout_display_t *, picture_t *, subpicture_t *);
static int Control(vout_display_t *, int, va_list);

static void picture_Strech2(vout_display_t *, picture_t *, picture_t *);
static void picture_CopyToSurface(vout_display_t *, picture_t *, picture_t *);

// _ZN7android7Surface4lockEPNS0_11SurfaceInfoEb
typedef void (*Surface_lock)(void *, void *, int);
// _ZN7android7Surface13unlockAndPostEv
typedef void (*Surface_unlockAndPost)(void *);

struct vout_display_sys_t {
    picture_pool_t *pool;
    void *libsfc_or_ui;
    int i_width, i_height;
};

typedef struct _SurfaceInfo {
    uint32_t    w;
    uint32_t    h;
    uint32_t    s;
    uint32_t    a;
    uint32_t    b;
    uint32_t    c;
    uint32_t    reserved[2];
} SurfaceInfo;

static Surface_lock s_lock = NULL;
static Surface_unlockAndPost s_unlockAndPost = NULL;

static void *InitLibrary() {
    void *p_library;

    p_library = dlopen("libsurfaceflinger_client.so", RTLD_NOW);
    if (p_library) {
        s_lock = (Surface_lock)(dlsym(p_library, "_ZN7android7Surface4lockEPNS0_11SurfaceInfoEb"));
        s_unlockAndPost = (Surface_unlockAndPost)(dlsym(p_library, "_ZN7android7Surface13unlockAndPostEv"));
        if (s_lock && s_unlockAndPost) {
            return p_library;
        }
        dlclose(p_library);
    }
    p_library = dlopen("libui.so", RTLD_NOW);
    if (p_library) {
        s_lock = (Surface_lock)(dlsym(p_library, "_ZN7android7Surface4lockEPNS0_11SurfaceInfoEb"));
        s_unlockAndPost = (Surface_unlockAndPost)(dlsym(p_library, "_ZN7android7Surface13unlockAndPostEv"));
        if (s_lock && s_unlockAndPost) {
            return p_library;
        }
        dlclose(p_library);
    }
    return NULL;
}

static int Open(vlc_object_t *p_this) {
    vout_display_t *vd = (vout_display_t *)p_this;
    vout_display_sys_t *sys;
    void *surf;
    SurfaceInfo info;
    void *p_library;

    p_library = InitLibrary();
    if (!p_library) {
        msg_Err(VLC_OBJECT(p_this), "Could not initialize libui.so/libsurfaceflinger_client.so!");
        return VLC_EGENERIC;
    }
    LockSurface();
    surf = GetSurface();
    if (!surf) {
        UnlockSurface();
        dlclose(p_library);
        msg_Err(vd, "No surface is attached!");
        return VLC_EGENERIC;
    }
    s_lock(surf, &info, 1);
    s_unlockAndPost(surf);
    UnlockSurface();
    // TODO: what about the other formats?
    int pixel_format = info.s >= info.w ? info.b : info.a;
    const char *chroma_format;
    switch (pixel_format) {
    case 4:     // PIXEL_FORMAT_RGB_565
        chroma_format = "RV16";
        break;
    default:
        dlclose(p_library);
        msg_Err(vd, "Unknown chroma format %08x (android)", pixel_format);
        return VLC_EGENERIC;
    }    
    vlc_fourcc_t chroma = vlc_fourcc_GetCodecFromString(VIDEO_ES, chroma_format);
    if (!chroma) {
        dlclose(p_library);
        msg_Err(vd, "Unsupported chroma format %s", chroma_format);
        return VLC_EGENERIC;
    }
    video_format_t fmt = vd->fmt;
    fmt.i_chroma = chroma;
    switch (chroma) {
    case VLC_CODEC_RGB16:
        fmt.i_rmask = 0xf800;
        fmt.i_gmask = 0x07e0;
        fmt.i_bmask = 0x001f;
        break;
    default:
        dlclose(p_library);
        msg_Err(vd, "Unknown chroma format %08x (vlc)", chroma);
        return VLC_EGENERIC;
    }
    sys = (struct vout_display_sys_t*) calloc(1, sizeof(struct vout_display_sys_t));
    if (!sys)
        return VLC_ENOMEM;
    sys->pool = NULL;
    sys->libsfc_or_ui = p_library;

    vout_display_cfg_t cfg = *vd->cfg;
    cfg.display.width = info.w;
    cfg.display.height = info.h;
    cfg.is_fullscreen = false;

    vout_display_SendEventDisplaySize(vd, info.w, info.h, false);

    vd->sys = sys;
    vd->fmt = fmt;
    vd->pool = Pool;
    vd->prepare = NULL;
    vd->display = Display;
    vd->control = Control;
    vd->manage  = NULL;

    return VLC_SUCCESS;
}

static void Close(vlc_object_t *p_this) {
    vout_display_t *vd = (vout_display_t *)p_this;
    vout_display_sys_t *sys = vd->sys;

    if (sys->pool)
        picture_pool_Delete(sys->pool);
    free(sys);
}

static picture_pool_t *Pool(vout_display_t *vd, unsigned count) {
    vout_display_sys_t *sys = vd->sys;

    if (!sys->pool) {
        sys->pool = picture_pool_NewFromFormat(&vd->fmt, count);
    }
    return sys->pool;
}

static void Display(vout_display_t *vd, picture_t *picture, subpicture_t *subpicture) {
    VLC_UNUSED(subpicture);
    vout_display_sys_t *sys = vd->sys;
    void *surf;
    SurfaceInfo info;
    int srx, sry, srh, srw;
    int drx, dry, drh, drw;
    int sw, sh, ss, dw, dh, ds;
    int q, q1, q2;
    pixman_image_t *src_image = NULL, *dst_image = NULL;
    pixman_transform_t transform;

    if (picture->i_planes != 1) {
        goto out;
    }
    // XXX: DO NOT ASSUME RGB565
    sw = picture->p[0].i_visible_pitch / picture->p[0].i_pixel_pitch;
    sh = picture->p[0].i_visible_lines;
    ss = picture->p[0].i_pitch / picture->p[0].i_pixel_pitch;
    src_image = pixman_image_create_bits(PIXMAN_r5g6b5, sw, sh, (uint32_t*)(picture->p[0].p_pixels), ss << 1);
    if (!src_image) {
        goto out;
    }
    srx = 0;
    sry = 0;
    srw = sw;
    srh = sh;
    LockSurface();
    surf = GetSurface();
    if (surf) {
        s_lock(surf, &info, 1);
        dw = info.w;
        dh = info.h;
        ds = info.s >= info.w ? info.s : info.w;
        if (sw > dw || sh > dh) {
            q1 = (dw << 16) / sw;
            q2 = (dh << 16) / sh;
            q = (q1 < q2) ? q1 : q2;
            drw = (sw * q) >> 16;
            drh = (sh * q) >> 16;
            q1 = (sw << 16) / dw;
            q2 = (sh << 16) / dh;
            q = (q1 < q2) ? q2 : q1;
            pixman_transform_init_scale(&transform, q, q);
            pixman_image_set_transform(src_image, &transform);
        }
        else {
            drw = sw;
            drh = sh;
        }
        drx = (dw - drw) >> 1;
        dry = (dh - drh) >> 1;
        // msg_Dbg(VLC_OBJECT(vd), "%dx%d %d %d,%d %dx%d => %dx%d %d %d,%d %dx%d", sw, sh, ss, srx, sry, srw, srh, dw, dh, ds, drx, dry, drw, drh);
        int pixel_format = info.s >= info.w ? info.b : info.a;
        void *bits = (void *)(info.s >= info.w ? info.c : info.b);
        int pixel_size = 0;
        switch (pixel_format) {
        case 4: {       // PIXEL_FORMAT_RGB_565
            pixel_size = 2;
            dst_image = pixman_image_create_bits(PIXMAN_r5g6b5, dw, dh, (uint32_t*) bits, ds << 1);
            if (!dst_image) {
                goto bail;
            }
            break;
            }
        default:
            goto bail;
        }
        // fill it when needed
        if (sys->i_width != drw || sys->i_height != drh) {
            sys->i_width = drw;
            sys->i_height = drh;
            memset(bits, 0, ds * dh * pixel_size);
        }
        pixman_image_composite(PIXMAN_OP_SRC, src_image, NULL, dst_image, srx, sry, 0, 0, drx, dry, srw , srh);
bail:
        s_unlockAndPost(surf);
    }
    UnlockSurface();
    if (src_image) {
        pixman_image_unref(src_image);
    }
    if (dst_image) {
        pixman_image_unref(dst_image);
    }
out:
    picture_Release(picture);
}

static int Control(vout_display_t *vd, int query, va_list args) {
    return VLC_EGENERIC;
}

