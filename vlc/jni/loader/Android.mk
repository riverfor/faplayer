
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CFLAGS += -std=gnu99

LOCAL_MODULE := libvlc-loader

LOCAL_SRC_FILES := loader.c

LOCAL_LDLIBS += -llog

include $(BUILD_SHARED_LIBRARY)

