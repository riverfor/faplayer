
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := liblibmp4_plugin

LOCAL_CFLAGS += \
    -std=gnu99 \
    -DHAVE_CONFIG_H \
    -D__PLUGIN__ \
    -DMODULE_STRING=\"libmp4\" \
    -DMODULE_NAME=libmp4

LOCAL_C_INCLUDES += \
    $(VLCROOT) \
    $(VLCROOT)/include \
    $(VLCROOT)/src

LOCAL_SRC_FILES := \
    drms.c \
    libmp4.c \
    mp4.c

LOCAL_SHARED_LIBRARIES += libvlccore

LOCAL_LDLIBS += -lz

include $(BUILD_SHARED_LIBRARY)

