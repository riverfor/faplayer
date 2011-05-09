
LOCAL_PATH := $(call my-dir)

ifeq ($(BUILD_WITH_NEON),1)

include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm
LOCAL_ARM_NEON := true

LOCAL_MODULE := audio_format_neon_plugin

LOCAL_CFLAGS += \
    -std=c99 \
    -DHAVE_CONFIG_H \
    -fasm \
    -DMODULE_STRING=\"audio_format_neon\" \
    -DMODULE_NAME=audio_format_neon

LOCAL_C_INCLUDES += \
    $(VLCROOT)/compat \
    $(VLCROOT) \
    $(VLCROOT)/include \
    $(VLCROOT)/src

LOCAL_SRC_FILES := \
    audio_format.c \
    s32_s16.S

include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm
LOCAL_ARM_NEON := true

LOCAL_MODULE := memcpy_neon_plugin

LOCAL_CFLAGS += \
    -std=c99 \
    -DHAVE_CONFIG_H \
    -DMODULE_STRING=\"memcpy_neon\" \
    -DMODULE_NAME=memcpy_neon

LOCAL_C_INCLUDES += \
    $(VLCROOT)/compat \
    $(VLCROOT) \
    $(VLCROOT)/include \
    $(VLCROOT)/src

LOCAL_SRC_FILES := \
    memcpy.c \
    memcpy_impl.S

include $(BUILD_STATIC_LIBRARY)

endif

include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm
ifeq ($(BUILD_WITH_NEON),1)
LOCAL_ARM_NEON := true
endif

LOCAL_MODULE := yuv2rgb_plugin

LOCAL_CFLAGS += \
    -std=c99 \
    -DHAVE_CONFIG_H \
    -fasm \
    -DMODULE_STRING=\"yuv2rgb\" \
    -DMODULE_NAME=yuv2rgb

LOCAL_C_INCLUDES += \
    $(EXTROOR)/pixman/pixman \
    $(VLCROOT)/compat \
    $(VLCROOT) \
    $(VLCROOT)/include \
    $(VLCROOT)/src

LOCAL_SRC_FILES := \
    yuv2rgb.c \
    yuv2rgb16tab.c \
    yuv420rgb565.S \
    yuv422rgb565.S \
    yuv444rgb565.S

ifeq ($(BUILD_WITH_NEON),1)
LOCAL_CFLAGS += -DHAVE_NEON=1
LOCAL_SRC_FILES += yuv2rgb.444565.S yuv2rgb.422565.S yuv2rgb.420565.c
endif

include $(BUILD_STATIC_LIBRARY)

