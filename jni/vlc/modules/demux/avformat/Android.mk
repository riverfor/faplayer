
LOCAL_PATH := $(call my-dir)

# libavformat_plugin.so

include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm

LOCAL_MODULE := avformat_plugin

LOCAL_CFLAGS += \
    -std=c99 \
    -D__THROW= \
    -DHAVE_CONFIG_H \
    -DNDEBUG \
    -D__PLUGIN__ \
    -DMODULE_STRING=\"avformat\"

LOCAL_CFLAGS += $(COMMON_OPT_CFLAGS)

LOCAL_C_INCLUDES += \
    $(VLCROOT)/compat \
    $(VLCROOT) \
    $(VLCROOT)/include \
    $(VLCROOT)/src \
    $(EXTROOT)/ffmpeg

LOCAL_SRC_FILES := \
    avformat.c \
    chroma.c \
    demux.c \
    fourcc.c

LOCAL_SHARED_LIBRARIES += vlccore ffmpeg

include $(BUILD_SHARED_LIBRARY)

