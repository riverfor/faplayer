
LOCAL_PATH := $(call my-dir)

# libavcodec_plugin.so

include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm

LOCAL_MODULE := avcodec_plugin

LOCAL_CFLAGS += \
    -std=c99 \
    -D__THROW= \
    -DHAVE_CONFIG_H \
    -DNDEBUG \
    -D__PLUGIN__ \
    -DMODULE_STRING=\"avcodec\"

LOCAL_CFLAGS += $(COMMON_OPT_CFLAGS)

LOCAL_C_INCLUDES += \
    $(VLCROOT)/compat \
    $(VLCROOT) \
    $(VLCROOT)/include \
    $(VLCROOT)/src \
    $(EXTROOT)/ffmpeg

LOCAL_SRC_FILES := \
    audio.c \
    avcodec.c \
    chroma.c \
    copy.c \
    deinterlace.c \
    fourcc.c \
    subtitle.c \
    video.c

LOCAL_SHARED_LIBRARIES += vlccore ffmpeg

include $(BUILD_SHARED_LIBRARY)

