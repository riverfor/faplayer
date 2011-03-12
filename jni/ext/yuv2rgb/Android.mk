
# awesome code from theorarm

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm

LOCAL_MODULE := yuv2rgb

LOCAL_SRC_FILES := \
    yuv2rgb16tab.c \
    yuv420rgb565.S \
    yuv422rgb565.S \
    yuv444rgb565.S

LOCAL_CFLAGS += $(COMMON_OPT_CFLAGS)
LOCAL_LDFLAGS += $(COMMON_OPT_LDFLAGS)

include $(BUILD_STATIC_LIBRARY)

