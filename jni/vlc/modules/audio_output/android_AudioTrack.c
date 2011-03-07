
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_aout.h>

#include <jni.h>

extern JavaVM *gJVM;

struct aout_sys_t {
    int type;
    int rate;
    int channel;
    int format;
    int size;
    int mode;
    jobject audio_track_object;
    jobject audio_track_reference;
    jmethodID audio_track_ctor;
    jmethodID audio_track_getMinBufferSize;
    jmethodID audio_track_play;
    jmethodID audio_track_stop;
    jmethodID audio_track_write16;
};

static int  Open(vlc_object_t *);
static void Close(vlc_object_t *);
static void Play(aout_instance_t *);

vlc_module_begin ()
    set_shortname("AndroidAudioTrack")
    set_description(N_("Android AudioTrack audio output"))
    set_capability("audio output", 10)
    set_category(CAT_AUDIO)
    set_subcategory(SUBCAT_AUDIO_AOUT)
    add_shortcut("android")
    set_callbacks(Open, Close)
vlc_module_end ()

static int Open(vlc_object_t *p_this) {
    struct aout_sys_t *p_sys;
    aout_instance_t *p_aout = (aout_instance_t*)(p_this);
    JNIEnv *env;
    jclass clz;
    jobject obj;

    int err = (*gJVM)->AttachCurrentThread(gJVM, &env, NULL);
    if (err)
        return VLC_EGENERIC;
    p_sys = (struct aout_sys_t*)malloc(sizeof(aout_sys_t));
    if (p_sys == NULL)
        return VLC_ENOMEM;
    // setup
    clz = (*env)->FindClass(env, "android/media/AudioTrack");
    if (!clz) {
        free(p_sys);
        (*gJVM)->DetachCurrentThread(gJVM);
        msg_Err(VLC_OBJECT(p_this), "Could not find class AudioTrack");
        return VLC_EGENERIC;
    }
    p_sys->audio_track_ctor = (*env)->GetMethodID(env, clz, "<init>", "(IIIIII)V");
    p_sys->audio_track_getMinBufferSize = (*env)->GetStaticMethodID(env, clz, "getMinBufferSize", "(III)I");
    p_sys->audio_track_play = (*env)->GetMethodID(env, clz, "play", "()V");
    p_sys->audio_track_stop = (*env)->GetMethodID(env, clz, "stop", "()V");
    p_sys->audio_track_write16 = (*env)->GetMethodID(env, clz, "write", "([SII)I");
    // streamType = STREAM_MUSIC
    p_sys->type = 0x00000003;
    // 4000 <= sampleRateInHz <= 48000
    if (p_aout->output.output.i_rate < 4000)
        p_aout->output.output.i_rate = 4000;
    if (p_aout->output.output.i_rate > 48000)
        p_aout->output.output.i_rate = 48000;
    p_sys->rate = p_aout->output.output.i_rate;
    // channelConfig
    p_sys->channel = aout_FormatNbChannels(&p_aout->output.output);
    if (p_sys->channel != 2) {
        p_sys->channel = 2;
        p_aout->output.output.i_physical_channels = AOUT_CHAN_LEFT | AOUT_CHAN_RIGHT;
    }
    // legacy CHANNEL_CONFIGURATION_STEREO = 0x00000003
    p_sys->channel = 0x00000003;
    // audioFormat
    if (p_aout->output.output.i_format != VLC_CODEC_S16L)
        p_aout->output.output.i_format = VLC_CODEC_S16L;
    // ENCODING_PCM_16BIT = 0x00000002
    p_sys->format = 0x00000002;
    // bufferSizeInBytes
    p_aout->output.i_nb_samples = (*env)->CallStaticIntMethod(env, clz, p_sys->audio_track_getMinBufferSize, p_sys->rate, p_sys->channel, p_sys->format);
    p_aout->output.i_nb_samples <<= 1;
    p_sys->size = p_aout->output.i_nb_samples;
    // mode = MODE_STREAM
    p_sys->mode = 0x00000001;

    // now create the AudioTrack object
    p_sys->audio_track_object = (*env)->NewObject(env, clz, p_sys->audio_track_ctor, p_sys->type, p_sys->rate, p_sys->channel, p_sys->format, p_sys->size, p_sys->mode);
    (*env)->DeleteLocalRef(env, clz);
    if (!p_sys->audio_track_object) {
        free(p_sys);
        (*gJVM)->DetachCurrentThread(gJVM);
        msg_Err(VLC_OBJECT(p_this), "Could not create AudioTrack");
        return VLC_EGENERIC;
    }
    p_sys->audio_track_object = (*env)->NewGlobalRef(env, p_sys->audio_track_object);
    // now let it play
    (*env)->CallVoidMethod(env, p_sys->audio_track_object, p_sys->audio_track_play);

    p_aout->output.p_sys = p_sys;
    p_aout->output.pf_play = Play;

    (*gJVM)->DetachCurrentThread(gJVM);

    return VLC_SUCCESS;
}

static void Close(vlc_object_t *p_this) {
    JNIEnv *env;
    aout_instance_t *p_aout = (aout_instance_t*)p_this;
    struct aout_sys_t *p_sys = p_aout->output.p_sys;

    int err = (*gJVM)->AttachCurrentThread(gJVM, &env, NULL);
    if (!err) {
        (*env)->CallVoidMethod(env, p_sys->audio_track_object, p_sys->audio_track_stop);
        (*env)->DeleteGlobalRef(env, p_sys->audio_track_reference);
        (*gJVM)->DetachCurrentThread(gJVM);
    }
    vlc_object_kill(p_aout);
    vlc_thread_join(p_aout);
    p_aout->b_die = false;
    free(p_sys);
}

static void Play(aout_instance_t *p_aout) {
    JNIEnv *env;
    struct aout_sys_t *p_sys = p_aout->output.p_sys;
    aout_buffer_t *p_buffer;

    int err = (*gJVM)->AttachCurrentThread(gJVM, &env, NULL);
    if (err) {
        msg_Err(VLC_OBJECT(p_aout), "Could not attach current thread, will not play");
        return;
    }
    while (vlc_object_alive(p_aout)) {
        p_buffer = aout_FifoPop(p_aout, &p_aout->output.fifo);
        if (p_buffer != NULL) {
            if (p_sys->format == 0x00000002) {
                jint offset = 0;
                jsize length = p_buffer->i_buffer >> 1;
                jshortArray array = (*env)->NewShortArray(env, length);
                if (array) {
                    (*env)->SetShortArrayRegion(env, array, 0, length, (jshort*)p_buffer->p_buffer);
                    while (offset < length) {
                        jint temp = (*env)->CallIntMethod(env, p_sys->audio_track_object, p_sys->audio_track_write16, array, offset, length - offset);
                        if (temp < 0)
                            break;
                        offset += temp;
                    }
                    // explicit free?
                    //(*env)->DeleteLocalRef(env, array);
                }
            }
            else {
                msg_Warn(VLC_OBJECT(p_aout), "%s: unknown format %d, discard buffer only", __func__, p_sys->format);
            }
            aout_BufferFree(p_buffer);
        }
        else
            break;
    }
    (*gJVM)->DetachCurrentThread(gJVM);
}

