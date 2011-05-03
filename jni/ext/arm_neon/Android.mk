
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm
ifeq ($(BUILD_WITH_NEON),1)
LOCAL_ARM_NEON := true
endif

LOCAL_MODULE := arm_neon

LOCAL_SRC_FILES := \
    memcpy.S

include $(BUILD_STATIC_LIBRARY)

