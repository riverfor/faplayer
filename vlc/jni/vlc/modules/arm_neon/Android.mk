
LOCAL_PATH := $(call my-dir)

ifeq ($(BUILD_WITH_ARM_NEON),1)

include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm
LOCAL_ARM_NEON := true

#@ armeabi-v7a neon
LOCAL_MODULE := audio_format_neon_plugin

LOCAL_CFLAGS += \
    -std=gnu99 \
    -DHAVE_CONFIG_H \
    -D__PLUGIN__ \
    -fasm \
    -DMODULE_STRING=\"audio_format_neon\" \
    -DMODULE_NAME=audio_format_neon

LOCAL_C_INCLUDES += \
    $(VLCROOT) \
    $(VLCROOT)/include \
    $(VLCROOT)/src

LOCAL_SRC_FILES := \
    audio_format.c \
    s32_s16.S

LOCAL_SHARED_LIBRARIES += libvlccore

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm
LOCAL_ARM_NEON := true

#@ armeabi-v7a neon
LOCAL_MODULE := yuv2rgb_neon_plugin

LOCAL_CFLAGS += \
    -std=gnu99 \
    -DHAVE_CONFIG_H \
    -D__PLUGIN__ \
    -fasm \
    -DMODULE_STRING=\"yuv2rgb_neon\" \
    -DMODULE_NAME=yuv2rgb_neon

LOCAL_C_INCLUDES += \
    $(VLCROOT) \
    $(VLCROOT)/include \
    $(VLCROOT)/src

LOCAL_SRC_FILES := \
    yuv2rgb.c \
    yuv420rgb565.c \
    yuv422rgb565.S \
    yuv444rgb565.S

LOCAL_SHARED_LIBRARIES += libvlccore

include $(BUILD_SHARED_LIBRARY)

endif

