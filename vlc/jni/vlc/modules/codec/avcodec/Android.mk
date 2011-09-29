
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := avcodec_plugin

LOCAL_CFLAGS += \
    -std=gnu99 \
    -DHAVE_CONFIG_H \
    -D__PLUGIN__ \
    -DMODULE_STRING=\"avcodec\" \
    -DMODULE_NAME=avcodec

LOCAL_C_INCLUDES += \
    $(VLCROOT) \
    $(VLCROOT)/include \
    $(VLCROOT)/src

LOCAL_SRC_FILES := \
    audio.c \
    avcodec.c \
    chroma.c \
    copy.c \
    deinterlace.c \
    fourcc.c \
    subtitle.c \
    video.c

LOCAL_SHARED_LIBRARIES += libavcodec libavutil

LOCAL_SHARED_LIBRARIES += libvlccore

LOCAL_LDLIBS += -lz

include $(BUILD_SHARED_LIBRARY)

