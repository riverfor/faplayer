
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm
ifeq ($(BUILD_WITH_NEON),1)
LOCAL_ARM_NEON := true
endif

LOCAL_MODULE := rtp_plugin

LOCAL_CFLAGS += \
    -std=c99 \
    -DHAVE_CONFIG_H \
    -DMODULE_STRING=\"rtp\" \
    -DMODULE_NAME=rtp

LOCAL_CFLAGS += $(COMMON_TUNE_CFLAGS)
LOCAL_LDFLAGS += $(COMMON_TUNE_LDFLAGS)

LOCAL_C_INCLUDES += \
    $(VLCROOT)/compat \
    $(VLCROOT) \
    $(VLCROOT)/include \
    $(VLCROOT)/src

LOCAL_SRC_FILES := \
    input.c \
    rtp.c \
    session.c \
    xiph.c

include $(BUILD_STATIC_LIBRARY)
