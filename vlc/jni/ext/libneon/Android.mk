
LOCAL_PATH := $(call my-dir)

ifeq ($(BUILD_WITH_ARM_NEON),1)

include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm

LOCAL_MODULE := libneon

LOCAL_SRC_FILES := \
    memcpy.S \
    memset.S

include $(BUILD_STATIC_LIBRARY)

endif

