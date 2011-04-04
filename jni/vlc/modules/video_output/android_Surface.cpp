
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_vout_display.h>
#include <vlc_picture_pool.h>
#include <vlc_threads.h>
#include <pixman.h>

#ifndef __PLATFORM__
#error "android api level is not defined!"
#endif

#if __PLATFORM__ < 8
#include <ui/Surface.h>
#else
#include <surfaceflinger/Surface.h>
#endif

using namespace android;

extern "C" void LockSurface();
extern "C" void UnlockSurface();
extern "C" void *GetSurface();

static int Open (vlc_object_t *);
static void Close(vlc_object_t *);

vlc_module_begin()
    set_shortname("AndroidSurface")
    set_category(CAT_VIDEO)
    set_subcategory(SUBCAT_VIDEO_VOUT)
    set_description(N_("Android Surface video output"))
    set_capability("vout display", __PLATFORM__)
    add_shortcut("android")
    set_callbacks(Open, Close)
vlc_module_end()

static picture_pool_t *Pool(vout_display_t *, unsigned);
static void Display(vout_display_t *, picture_t *, subpicture_t *);
static int Control(vout_display_t *, int, va_list);

static void picture_Strech2(vout_display_t *, picture_t *, picture_t *);
static void picture_CopyToSurface(vout_display_t *, picture_t *, picture_t *);

struct vout_display_sys_t {
    picture_pool_t *pool;
};

static int Open(vlc_object_t *p_this) {
    vout_display_t *vd = (vout_display_t *)p_this;
    vout_display_sys_t *sys;

    // get surface infomation
    LockSurface();
    Surface *surf = (Surface*) GetSurface();
    if (!surf) {
        UnlockSurface();
        msg_Err(vd, "android surface is not ready");
        return VLC_EGENERIC;
    }
    Surface::SurfaceInfo info;
    surf->lock(&info);
#if __PLATFORM__ == 4
    surf->unlock();
#else
    surf->unlockAndPost();
#endif
    UnlockSurface();
    // TODO: what about the other formats?
    const char *chroma_format;
    switch (info.format) {
    case PIXEL_FORMAT_RGB_565:
        chroma_format = "RV16";
        break;
    default:
        msg_Err(vd, "unknown chroma format %08x (android)", info.format);
        return VLC_EGENERIC;
    }    
    vlc_fourcc_t chroma = vlc_fourcc_GetCodecFromString(VIDEO_ES, chroma_format);
    if (!chroma) {
        msg_Err(vd, "unsupported chroma format %s", chroma_format);
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
        msg_Err(vd, "unknown chroma format %08x (vlc)", chroma);
        return VLC_EGENERIC;
    }
    sys = (struct vout_display_sys_t*) malloc(sizeof(struct vout_display_sys_t));
    if (!sys)
        return VLC_ENOMEM;
    memset(sys, 0, sizeof(*sys));
#if __PLATFORM__ > 4
    msg_Dbg(VLC_OBJECT(p_this), "SurfaceInfo w = %d, h = %d, s = %d", info.w, info.h, info.s);
#else
    msg_Dbg(VLC_OBJECT(p_this), "SurfaceInfo w = %d, h = %d", info.w, info.h);
#endif
    sys->pool = NULL;

    // 
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
    Surface *surf;
    Surface::SurfaceInfo info;
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
    surf = (Surface*)(GetSurface());
    if (surf) {
        surf->lock(&info);
        dw = info.w;
        dh = info.h;
#if __PLATFORM__ > 4
        ds = info.s;
#else
        ds = info.w;
#endif
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
        switch (info.format) {
        case PIXEL_FORMAT_RGB_565: {
            dst_image = pixman_image_create_bits(PIXMAN_r5g6b5, dw, dh, (uint32_t*)(info.bits), ds << 1);
            if (!dst_image) {
                goto bail;
            }
            break;
            }
        default:
            goto bail;
        }
        pixman_image_composite(PIXMAN_OP_SRC, src_image, NULL, dst_image, srx, sry, 0, 0, drx, dry, srw , srh);
bail:
        surf->unlockAndPost();
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

