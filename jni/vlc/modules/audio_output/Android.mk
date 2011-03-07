
LOCAL_PATH := $(call my-dir)

# libaout_android_plugin.so

include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm

LOCAL_MODULE := aout_android_plugin

LOCAL_CFLAGS += \
    -std=c99 \
    -D__THROW= \
    -DHAVE_CONFIG_H \
    -DNDEBUG \
    -D__PLUGIN__ \
    -DMODULE_STRING=\"aout_android\"

LOCAL_CPPFLAGS += $(COMMON_OPT_CFLAGS)

LOCAL_C_INCLUDES += \
    $(VLCROOT)/compat \
    $(VLCROOT) \
    $(VLCROOT)/include \
    $(VLCROOT)/src

LOCAL_SRC_FILES := \
    android_AudioTrack.c

LOCAL_SHARED_LIBRARIES += vlccore

include $(BUILD_SHARED_LIBRARY)

