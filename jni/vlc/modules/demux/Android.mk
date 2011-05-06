
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm
ifeq ($(BUILD_WITH_NEON),1)
LOCAL_ARM_NEON := true
endif

LOCAL_MODULE := live555_plugin

LOCAL_CFLAGS += \
    -std=c99 \
    -DHAVE_CONFIG_H

LOCAL_CPPFLAGS += \
    -DHAVE_CONFIG_H \
    -DMODULE_STRING=\"live555\" \
    -DMODULE_NAME=live555

LOCAL_C_INCLUDES += \
    $(VLCROOT)/compat \
    $(VLCROOT) \
    $(VLCROOT)/include \
    $(VLCROOT)/src \
    $(EXTROOT)/live/BasicUsageEnvironment/include \
    $(EXTROOT)/live/UsageEnvironment/include \
    $(EXTROOT)/live/groupsock/include \
    $(EXTROOT)/live/liveMedia/include

LOCAL_SRC_FILES := \
    ../access/mms/asf.c \
    ../access/mms/buffer.c \
    live555.cpp

include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)

include $(call all-makefiles-under,$(LOCAL_PATH))

