
LOCAL_PATH := $(call my-dir)

#libyuv2rgb_plugin.so

include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm

LOCAL_MODULE := yuv2rgb_plugin

LOCAL_CFLAGS += \
    -std=c99 \
    -D__THROW= \
    -DHAVE_CONFIG_H \
    -DNDEBUG \
    -D__PLUGIN__ \
    -DMODULE_STRING=\"yuv2rgb\"

LOCAL_CPPFLAGS += $(COMMON_OPT_CFLAGS)
LOCAL_LDFLAGS += $(COMMON_OPT_LDFLAGS)

LOCAL_C_INCLUDES += \
    $(VLCROOT)/compat \
    $(VLCROOT) \
    $(VLCROOT)/include \
    $(VLCROOT)/src \
    $(EXTROOT)/yuv2rgb

LOCAL_SRC_FILES := \
    yuv2rgb.c \
    yuv2rgb16tab.c \
    yuv420rgb565.S \
    yuv422rgb565.S \
    yuv444rgb565.S

LOCAL_SHARED_LIBRARIES += vlccore

include $(BUILD_SHARED_LIBRARY)

