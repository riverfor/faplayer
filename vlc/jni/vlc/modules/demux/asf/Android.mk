
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

ifeq ($(BUILD_WITH_ARM),1)
LOCAL_ARM_MODE := arm
endif

LOCAL_MODULE := liblibasf_plugin

LOCAL_CFLAGS += \
    -std=gnu99 \
    -DHAVE_CONFIG_H \
    -DMODULE_STRING=\"libasf\" \
    -DMODULE_NAME=libasf

LOCAL_C_INCLUDES += \
    $(VLCROOT) \
    $(VLCROOT)/include

LOCAL_SRC_FILES := \
    asf.c \
    libasf.c

include $(BUILD_STATIC_LIBRARY)

