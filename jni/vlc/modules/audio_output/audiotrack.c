/*****************************************************************************
 * audiotrack.c: Android native AudioTrack audio output module
 *****************************************************************************
 *
 * Authors: Ming Hu <tewilove@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_aout.h>

#include <dlfcn.h>

#define SIZE_OF_AUDIOTRACK 256

/* see AudioSystem.h */
enum stream_type {
    MUSIC            = 3
};

enum pcm_sub_format {
    PCM_SUB_16_BIT          = 0x1, // must be 1 for backward compatibility
    PCM_SUB_8_BIT           = 0x2  // must be 2 for backward compatibility
};

enum audio_format {
    PCM                 = 0x00000000, // must be 0 for backward compatibility
    PCM_16_BIT          = (PCM|PCM_SUB_16_BIT),
    PCM_8_BIT          = (PCM|PCM_SUB_8_BIT)
};

enum audio_channels {
    CHANNEL_OUT_FRONT_LEFT = 0x4,
    CHANNEL_OUT_FRONT_RIGHT = 0x8,
    CHANNEL_OUT_MONO = CHANNEL_OUT_FRONT_LEFT,
    CHANNEL_OUT_STEREO = (CHANNEL_OUT_FRONT_LEFT | CHANNEL_OUT_FRONT_RIGHT)
};

// _ZN7android11AudioSystem19getOutputFrameCountEPii
typedef int (*AudioSystem_getOutputFrameCount)(int *, int);
// _ZN7android11AudioSystem16getOutputLatencyEPji
typedef int (*AudioSystem_getOutputLatency)(unsigned int *, int);
// _ZN7android11AudioSystem21getOutputSamplingRateEPii
typedef int (*AudioSystem_getOutputSamplingRate)(int *, int);

// _ZN7android10AudioTrack16getMinFrameCountEPiij
typedef int (*AudioTrack_getMinFrameCount)(int *, int, unsigned int);

// _ZN7android10AudioTrackC1EijiiijPFviPvS1_ES1_ii
typedef void (*AudioTrack_ctor)(void *, int, unsigned int, int, int, int, unsigned int, void (*)(int, void *, void *), void *, int, int);
// _ZN7android10AudioTrackC1EijiiijPFviPvS1_ES1_i
typedef void (*AudioTrack_ctor_legacy)(void *, int, unsigned int, int, int, int, unsigned int, void (*)(int, void *, void *), void *, int);
// _ZN7android10AudioTrackD1Ev
typedef void (*AudioTrack_dtor)(void *);
// _ZNK7android10AudioTrack9initCheckEv
typedef int (*AudioTrack_initCheck)(void *);
// _ZN7android10AudioTrack5startEv
typedef int (*AudioTrack_start)(void *);
// _ZN7android10AudioTrack4stopEv
typedef int (*AudioTrack_stop)(void *);
// _ZN7android10AudioTrack5writeEPKvj
typedef int (*AudioTrack_write)(void *, void  const*, unsigned int);
// _ZN7android10AudioTrack5flushEv
typedef int (*AudioTrack_flush)(void *);

struct aout_sys_t {
    int type;
    uint32_t rate;
    int channel;
    int format;
    int size;
    void *libmedia;
    void *AudioTrack;
    AudioSystem_getOutputFrameCount as_getOutputFrameCount;
    AudioSystem_getOutputLatency as_getOutputLatency;
    AudioSystem_getOutputSamplingRate as_getOutputSamplingRate;
    AudioTrack_getMinFrameCount at_getMinFrameCount;
    AudioTrack_ctor at_ctor;
    AudioTrack_ctor_legacy at_ctor_legacy;
    AudioTrack_dtor at_dtor;
    AudioTrack_initCheck at_initCheck;
    AudioTrack_start at_start;
    AudioTrack_stop at_stop;
    AudioTrack_write at_write;
    AudioTrack_flush at_flush;
};

static void *InitLibrary(struct aout_sys_t *p_sys);

static int  Open(vlc_object_t *);
static void Close(vlc_object_t *);
static void Play(audio_output_t *, block_t *);

vlc_module_begin ()
    set_shortname("AudioTrack")
    set_description(N_("Android AudioTrack audio output"))
    set_capability("audio output", 185)
    set_category(CAT_AUDIO)
    set_subcategory(SUBCAT_AUDIO_AOUT)
    add_shortcut("android")
    set_callbacks(Open, Close)
vlc_module_end ()

static void *InitLibrary(struct aout_sys_t *p_sys) {
    void *p_library;

    p_library = dlopen("libmedia.so", RTLD_GLOBAL);
    if (!p_library)
        return NULL;
    p_sys->as_getOutputFrameCount = (AudioSystem_getOutputFrameCount)(dlsym(p_library, "_ZN7android11AudioSystem19getOutputFrameCountEPii"));
    p_sys->as_getOutputLatency = (AudioSystem_getOutputLatency)(dlsym(p_library, "_ZN7android11AudioSystem16getOutputLatencyEPji"));
    p_sys->as_getOutputSamplingRate = (AudioSystem_getOutputSamplingRate)(dlsym(p_library, "_ZN7android11AudioSystem21getOutputSamplingRateEPii"));
    p_sys->at_getMinFrameCount = (AudioTrack_getMinFrameCount)(dlsym(p_library, "_ZN7android10AudioTrack16getMinFrameCountEPiij"));
    p_sys->at_ctor = (AudioTrack_ctor)(dlsym(p_library, "_ZN7android10AudioTrackC1EijiiijPFviPvS1_ES1_ii"));
    p_sys->at_ctor_legacy = (AudioTrack_ctor_legacy)(dlsym(p_library, "_ZN7android10AudioTrackC1EijiiijPFviPvS1_ES1_i"));
    p_sys->at_dtor = (AudioTrack_dtor)(dlsym(p_library, "_ZN7android10AudioTrackD1Ev"));
    p_sys->at_initCheck = (AudioTrack_initCheck)(dlsym(p_library, "_ZNK7android10AudioTrack9initCheckEv"));
    p_sys->at_start = (AudioTrack_start)(dlsym(p_library, "_ZN7android10AudioTrack5startEv"));
    p_sys->at_stop = (AudioTrack_stop)(dlsym(p_library, "_ZN7android10AudioTrack4stopEv"));
    p_sys->at_write = (AudioTrack_write)(dlsym(p_library, "_ZN7android10AudioTrack5writeEPKvj"));
    p_sys->at_flush = (AudioTrack_flush)(dlsym(p_library, "_ZN7android10AudioTrack5flushEv"));
    // need the first 3 or the last 1
    if (!((p_sys->as_getOutputFrameCount && p_sys->as_getOutputLatency && p_sys->as_getOutputSamplingRate) || p_sys->at_getMinFrameCount)) {
        dlclose(p_library);
        return NULL;
    }
    // need all in the list
    if (!((p_sys->at_ctor || p_sys->at_ctor_legacy) && p_sys->at_dtor && p_sys->at_initCheck && p_sys->at_start && p_sys->at_stop && p_sys->at_write && p_sys->at_flush)) {
        dlclose(p_library);
        return NULL;
    }
    return p_library;
}

static int Open(vlc_object_t *p_this) {
    struct aout_sys_t *p_sys;
    void *p_library;
    audio_output_t *p_aout = (audio_output_t*)(p_this);
    int status;
    int afSampleRate, afFrameCount, afLatency, minBufCount, minFrameCount;
    int type, channel, rate, format;

    p_sys = (struct aout_sys_t*) malloc(sizeof(aout_sys_t));
    if (p_sys == NULL)
        return VLC_ENOMEM;
    p_library = InitLibrary(p_sys);
    if (!p_library) {
        msg_Err(p_aout, "Could not initialize libmedia.so!");
        return VLC_EGENERIC;
    }
    p_sys->libmedia = p_library;
    // AudioSystem::MUSIC = 3
    type = MUSIC;
    p_sys->type = type;
    // 4000 <= frequency <= 48000
    if (p_aout->format.i_rate < 4000)
        p_aout->format.i_rate = 4000;
    if (p_aout->format.i_rate > 48000)
        p_aout->format.i_rate = 48000;
    rate = p_aout->format.i_rate;
    p_sys->rate = rate;
    // U8/S16 only
    if (p_aout->format.i_format != VLC_CODEC_U8 && p_aout->format.i_format != VLC_CODEC_S16L)
        p_aout->format.i_format = VLC_CODEC_S16L;
    // AudioSystem::PCM_16_BIT = 1
    // AudioSystem::PCM_8_BIT = 2
    format = (p_aout->format.i_format == VLC_CODEC_S16L) ? PCM_16_BIT : PCM_8_BIT;
    p_sys->format = format;
    // TODO: android supports more channels
    channel = aout_FormatNbChannels(&p_aout->format);
    if (channel > 2) {
        channel = 2;
        p_aout->format.i_physical_channels = AOUT_CHAN_LEFT | AOUT_CHAN_RIGHT;
    }
    // AudioSystem::CHANNEL_OUT_STEREO = 12
    // AudioSystem::CHANNEL_OUT_MONO = 4
    channel = (channel == 2) ? CHANNEL_OUT_STEREO : CHANNEL_OUT_MONO;
    p_sys->channel = channel;
    // use the minium value
    if (!p_sys->at_getMinFrameCount) {
        status = p_sys->as_getOutputSamplingRate(&afSampleRate, type);
        status ^= p_sys->as_getOutputFrameCount(&afFrameCount, type);
        status ^= p_sys->as_getOutputLatency((uint32_t*)(&afLatency), type);
        if (status != 0) {
            free(p_sys);
            dlclose(p_sys->libmedia);
            return VLC_EGENERIC;
        }
        minBufCount = afLatency / ((1000 * afFrameCount) / afSampleRate);
        if (minBufCount < 2)
            minBufCount = 2;
        minFrameCount = (afFrameCount * rate * minBufCount) / afSampleRate;
        p_aout->format.i_bytes_per_frame = minFrameCount;
    }
    else {
        status = p_sys->at_getMinFrameCount(&p_aout->format.i_bytes_per_frame, type, rate);
        if (status != 0) {
            dlclose(p_sys->libmedia);
            free(p_sys);
            return VLC_EGENERIC;
        }
    }
    p_aout->format.i_bytes_per_frame <<= 1;
    p_sys->size = p_aout->format.i_bytes_per_frame;
    // sizeof(AudioTrack) == 0x58 (not sure) on 2.2.1, this should be enough
    p_sys->AudioTrack = malloc(SIZE_OF_AUDIOTRACK);
    if (!p_sys->AudioTrack) {
        dlclose(p_sys->libmedia);
        free(p_sys);
        return VLC_ENOMEM;
    }
    *((uint32_t *) ((char *) p_sys->AudioTrack + SIZE_OF_AUDIOTRACK - 4)) = 0xbaadbaad;
    // higher than android 2.2
    if (p_sys->at_ctor)
        p_sys->at_ctor(p_sys->AudioTrack, p_sys->type, p_sys->rate, p_sys->format, p_sys->channel, p_sys->size, 0, NULL, NULL, 0, 0);
    // higher than android 1.6
    else if (p_sys->at_ctor_legacy)
        p_sys->at_ctor_legacy(p_sys->AudioTrack, p_sys->type, p_sys->rate, p_sys->format, p_sys->channel, p_sys->size, 0, NULL, NULL, 0);
    if (*((uint32_t *) ((char *) p_sys->AudioTrack + SIZE_OF_AUDIOTRACK - 4)) != 0xbaadbaad) {
        msg_Err(p_aout, "AudioTrack ctor touched somewhere not belongs to it, abort now!");
        abort();
    }
    status = p_sys->at_initCheck(p_sys->AudioTrack);
    // android 1.6 uses channel count instead of type
    if (status != 0) {
        p_sys->channel = (p_sys->channel == CHANNEL_OUT_STEREO) ? 2 : 1;
        p_sys->at_ctor_legacy(p_sys->AudioTrack, p_sys->type, p_sys->rate, p_sys->format, p_sys->channel, p_sys->size, 0, NULL, NULL, 0);
        status = p_sys->at_initCheck(p_sys->AudioTrack);
    }
    if (status != 0) {
        msg_Err(p_aout, "Cannot create AudioTrack!");
        free(p_sys->AudioTrack);
        free(p_sys);
        return VLC_EGENERIC;
    }

    p_aout->sys = p_sys;
    p_aout->pf_play = Play;

    p_sys->at_start(p_sys->AudioTrack);

    return VLC_SUCCESS;
}

static void Close(vlc_object_t *p_this) {
    audio_output_t *p_aout = (audio_output_t*)p_this;
    aout_sys_t *p_sys = p_aout->sys;

    p_sys->at_stop(p_sys->AudioTrack);
    p_sys->at_flush(p_sys->AudioTrack);
    p_sys->at_dtor(p_sys->AudioTrack);
    free(p_sys->AudioTrack);
    dlclose(p_sys->libmedia);
    free(p_sys);
}

static void Play(audio_output_t *p_aout, block_t *p_buffer) {
    aout_sys_t *p_sys = p_aout->sys;

    size_t length = 0;
    while (length < p_buffer->i_buffer) {
        length += p_sys->at_write(p_sys->AudioTrack, (char*)(p_buffer->p_buffer) + length, p_buffer->i_buffer - length);
    }

    aout_BufferFree(p_buffer);
}

