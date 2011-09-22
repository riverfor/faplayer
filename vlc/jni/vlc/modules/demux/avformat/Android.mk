
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

ifeq ($(BUILD_WITH_ARM),1)
LOCAL_ARM_MODE := arm
endif

LOCAL_MODULE := avformat_plugin

LOCAL_CFLAGS += \
    -std=gnu99 \
    -DHAVE_CONFIG_H \
    -D__PLUGIN__ \
    -DMODULE_STRING=\"avformat\" \
    -DMODULE_NAME=avformat

LOCAL_C_INCLUDES += \
    $(VLCROOT) \
    $(VLCROOT)/include \
    $(VLCROOT)/src

LOCAL_SRC_FILES := \
    avformat.c \
    demux.c \
    ../../codec/avcodec/chroma.c \
    ../../codec/avcodec/fourcc.c

LOCAL_STATIC_LIBRARIES += libavformat libavcodec libavutil

LOCAL_SHARED_LIBRARIES += libvlccore

LOCAL_LDLIBS += -lz

include $(BUILD_SHARED_LIBRARY)

