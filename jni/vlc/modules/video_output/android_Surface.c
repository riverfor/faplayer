
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_vout_display.h>
#include <vlc_picture_pool.h>
#include <vlc_threads.h>

#include <dlfcn.h>

extern void LockSurface();
extern void UnlockSurface();
extern void *GetSurface();

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

// _ZN7android7Surface4lockEPNS0_11SurfaceInfoEb
typedef void (*Surface_lock)(void *, void *, int);
// _ZN7android7Surface13unlockAndPostEv
typedef void (*Surface_unlockAndPost)(void *);

struct vout_display_sys_t {
    vout_display_place_t place;
    picture_pool_t *p_pool;
    void *p_libsfc_or_ui;
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

static void *InitLibrary()
{
    void *p_library;

    p_library = dlopen("libsurfaceflinger_client.so", RTLD_NOW);
    if (p_library)
    {
        s_lock = (Surface_lock)(dlsym(p_library, "_ZN7android7Surface4lockEPNS0_11SurfaceInfoEb"));
        s_unlockAndPost = (Surface_unlockAndPost)(dlsym(p_library, "_ZN7android7Surface13unlockAndPostEv"));
        if (s_lock && s_unlockAndPost)
        {
            return p_library;
        }
        dlclose(p_library);
    }
    p_library = dlopen("libui.so", RTLD_NOW);
    if (p_library)
    {
        s_lock = (Surface_lock)(dlsym(p_library, "_ZN7android7Surface4lockEPNS0_11SurfaceInfoEb"));
        s_unlockAndPost = (Surface_unlockAndPost)(dlsym(p_library, "_ZN7android7Surface13unlockAndPostEv"));
        if (s_lock && s_unlockAndPost)
        {
            return p_library;
        }
        dlclose(p_library);
    }
    return NULL;
}

static int Open(vlc_object_t *p_this)
{
    vout_display_t *vd = (vout_display_t *) p_this;
    void *p_library;
    void *p_surface;
    SurfaceInfo info;
    int i_pixel_format;
    char *psz_chroma_format = "RV16";
    vout_display_sys_t *sys;

    p_library = InitLibrary();
    if (!p_library)
    {
        return VLC_EGENERIC;
    }

    LockSurface();
    p_surface = GetSurface();
    if (!p_surface)
    {
        UnlockSurface();
        dlclose(p_library);
        return VLC_EGENERIC;
    }
    s_lock(p_surface, &info, 1);
    s_unlockAndPost(p_surface);
    UnlockSurface();

    vlc_fourcc_t chroma = vlc_fourcc_GetCodecFromString(VIDEO_ES, psz_chroma_format);
    if (!chroma || chroma != VLC_CODEC_RGB16)
    {
        dlclose(p_library);
        return VLC_EGENERIC;
    }

    video_format_t fmt = vd->fmt;
    fmt.i_chroma = chroma;
    /* rgb32 looks strange and slow */
    fmt.i_rmask = 0x0000f800;
    fmt.i_gmask = 0x000007e0;
    fmt.i_bmask = 0x0000001f;

    sys = (struct vout_display_sys_t*) calloc(1, sizeof(struct vout_display_sys_t));
    if (!sys)
        return VLC_ENOMEM;
    sys->p_pool = NULL;
    sys->p_libsfc_or_ui = p_library;

    vout_display_SendEventFullscreen(vd, false);
    vout_display_SendEventDisplaySize(vd, fmt.i_width, fmt.i_height, false);

    vd->sys = sys;
    vd->fmt = fmt;
    vd->pool = Pool;
    vd->prepare = NULL;
    vd->display = Display;
    vd->control = Control;
    vd->manage  = NULL;

    return VLC_SUCCESS;
}

static void Close(vlc_object_t *p_this)
{
    vout_display_t *vd = (vout_display_t *) p_this;
    vout_display_sys_t *sys = vd->sys;

    if (sys->p_pool)
        picture_pool_Delete(sys->p_pool);

    free(sys);
}

static picture_pool_t *Pool(vout_display_t *vd, unsigned count)
{
    vout_display_sys_t *sys = vd->sys;

    if (!sys->p_pool)
        sys->p_pool = picture_pool_NewFromFormat(&vd->fmt, count);

    return sys->p_pool;
}

static void Display(vout_display_t *vd, picture_t *picture, subpicture_t *subpicture)
{
    vout_display_sys_t *sys = vd->sys;
    void *p_surface;
    SurfaceInfo info;
    uint32_t sw, sh, dw, dh;
    void *p_bits;

    VLC_UNUSED(subpicture);

    sw = picture->p[0].i_visible_pitch / picture->p[0].i_pixel_pitch;
    sh = picture->p[0].i_visible_lines;

    LockSurface();

    p_surface = GetSurface();
    if (!p_surface)
        goto bail;

    s_lock(p_surface, &info, 1);

    dw = info.w;
    dh = info.h;
    p_bits = (void *)(info.s >= info.w ? info.c : info.b);

    if (sw == dw && sh == dh)
    {
        if (info.s > info.w)
        {
            for (int i = 0; i < sh; i++) 
            {
                memcpy(p_bits + i * info.s * 2, ((char *) picture->p[0].p_pixels) + i * picture->p[0].i_pitch, sw * 2);
            }
        }
        else
            memcpy(p_bits, picture->p[0].p_pixels, sw * sh * 2);
    }

    s_unlockAndPost(p_surface);

bail:
    UnlockSurface();

    picture_Release(picture);
}

static int Control(vout_display_t *vd, int query, va_list args)
{
    vout_display_sys_t *sys = vd->sys;

    switch (query)
    {
    case VOUT_DISPLAY_HIDE_MOUSE:
        return VLC_SUCCESS;
    default:
        return VLC_EGENERIC;
    }

    return VLC_EGENERIC;
}

