
LOCAL_PATH := $(call my-dir)

# libavcodec_plugin.so

include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm
ifeq ($(BUILD_WITH_NEON),1)
LOCAL_ARM_NEON := true
endif

LOCAL_MODULE := avcodec_plugin

LOCAL_CFLAGS += \
    -std=c99 \
    -DHAVE_CONFIG_H \
    -D__PLUGIN__ \
    -DMODULE_STRING=\"avcodec\"

LOCAL_CFLAGS += $(COMMON_TUNE_CFLAGS)
LOCAL_LDFLAGS += $(COMMON_TUNE_LDFLAGS)

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

LOCAL_SHARED_LIBRARIES += vlccore

include $(BUILD_SHARED_LIBRARY)

