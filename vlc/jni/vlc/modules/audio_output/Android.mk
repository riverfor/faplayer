
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := amem_plugin

LOCAL_CFLAGS += \
    -std=gnu99 \
    -DHAVE_CONFIG_H \
    -D__PLUGIN__ \
    -DMODULE_STRING=\"amem\" \
    -DMODULE_NAME=amem

LOCAL_C_INCLUDES += \
    $(VLCROOT) \
    $(VLCROOT)/include \
    $(VLCROOT)/src

LOCAL_SRC_FILES := \
    amem.c

LOCAL_SHARED_LIBRARIES += libvlccore

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := android_audiotrack_plugin

LOCAL_CFLAGS += \
    -std=gnu99 \
    -DHAVE_CONFIG_H \
    -D__PLUGIN__ \
    -DMODULE_STRING=\"android_audiotrack\" \
    -DMODULE_NAME=android_audiotrack

LOCAL_C_INCLUDES += \
    $(VLCROOT) \
    $(VLCROOT)/include \
    $(VLCROOT)/src

LOCAL_SRC_FILES := \
    androidaudiotrack.c

LOCAL_SHARED_LIBRARIES += libvlccore

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := android_opensles_plugin

LOCAL_CFLAGS += \
    -std=gnu99 \
    -DHAVE_CONFIG_H \
    -D__PLUGIN__ \
    -DMODULE_STRING=\"android_opensles\" \
    -DMODULE_NAME=android_opensles

LOCAL_C_INCLUDES += \
    $(VLCROOT) \
    $(VLCROOT)/include \
    $(VLCROOT)/src \
    $(DEPROOT)/include

LOCAL_SRC_FILES := \
    opensles_android.c

LOCAL_SHARED_LIBRARIES += libvlccore

include $(BUILD_SHARED_LIBRARY)

