
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_filter.h>

#include "yuv2rgb.h"

static int Activate(vlc_object_t *);
static void Deactivate(vlc_object_t *);

vlc_module_begin ()
    set_description(("YUV to RGB conversions using yuv2rgb from theorarm"))
#if HAVE_ANDROID
    set_capability("video filter2", 160)
#else
    set_capability("video filter2", 0)
#endif
    set_callbacks(Activate, Deactivate)
vlc_module_end ()

static picture_t *yuv420_rgb565_filter(filter_t *, picture_t *);
static picture_t *yuv422_rgb565_filter(filter_t *, picture_t *);
static picture_t *yuv444_rgb565_filter(filter_t *, picture_t *);

static int Activate(vlc_object_t *p_this) {
    filter_t *p_filter = (filter_t *)p_this;

    if (p_filter->fmt_out.video.i_chroma != VLC_CODEC_RGB16)
        return VLC_EGENERIC;
    if (p_filter->fmt_in.video.i_width != p_filter->fmt_out.video.i_width)
        return VLC_EGENERIC;
    if (p_filter->fmt_in.video.i_height != p_filter->fmt_out.video.i_height)
        return VLC_EGENERIC;
    switch (p_filter->fmt_in.video.i_chroma) {
        case VLC_CODEC_YV12:
        case VLC_CODEC_I420:
            p_filter->pf_video_filter = yuv420_rgb565_filter;
            break;
        //case VLC_CODEC_YUVA:
        //    p_filter->pf_video_filter = yuv444_rgb565_filter;
        //    break;
        default:
            return VLC_EGENERIC;
    }

    return VLC_SUCCESS;
}

static void Deactivate( vlc_object_t *p_this ) {
}

//static mtime_t total = 0;
//static int count = 0;

static picture_t *yuv420_rgb565_filter(filter_t *p_filter, picture_t *p_pic) {
    int width, height;
    picture_t *p_dst;

    //mtime_t bgn = mdate();
    if (!p_pic)
        return NULL;

    if (p_pic->U_PITCH != p_pic->V_PITCH) {
        picture_Release(p_pic);
        return NULL;
    }

    p_dst = filter_NewPicture(p_filter);
    if (!p_dst) {
        picture_Release(p_pic);
        return NULL;
    }

    width = p_filter->fmt_in.video.i_width;
    height = p_filter->fmt_in.video.i_height;

    yuv420_2_rgb565(
        p_dst->p[0].p_pixels,   // dst ptr
        p_pic->Y_PIXELS,        // y
        p_pic->U_PIXELS,        // u
        p_pic->V_PIXELS,        // v
        width,                  // width
        height,                 // height
        p_pic->Y_PITCH,         // y stride
        p_pic->U_PITCH,         // uv stride
        p_dst->p[0].i_pitch,    // dst stride
        yuv2rgb565_table,       // table
        0                       // dither
    );

    picture_CopyProperties(p_dst, p_pic);
    picture_Release(p_pic);
    //mtime_t end = mdate();
    //total += (end - bgn);
    //count += 1;
    //msg_Dbg(VLC_OBJECT(p_filter), "%s takes %lld us, average %lld", __func__, end - bgn, total / count);

    return p_dst;
}

static picture_t *yuv422_rgb565_filter(filter_t *p_filter, picture_t *p_pic) {
    //XXX
    return NULL;
}

static picture_t *yuv444_rgb565_filter(filter_t *p_filter, picture_t *p_pic) {
    //XXX
    return NULL;
}

