
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := mpgv_plugin

LOCAL_CFLAGS += \
    -std=gnu99 \
    -DHAVE_CONFIG_H \
    -D__PLUGIN__ \
    -DMODULE_STRING=\"mpgv\" \
    -DMODULE_NAME=mpgv

LOCAL_C_INCLUDES += \
    $(VLCROOT) \
    $(VLCROOT)/include \
    $(VLCROOT)/src

LOCAL_SRC_FILES := \
    mpgv.c

LOCAL_SHARED_LIBRARIES += libvlccore

include $(BUILD_SHARED_LIBRARY)

