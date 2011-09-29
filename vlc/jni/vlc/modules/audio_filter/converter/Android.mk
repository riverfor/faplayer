
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := a52tofloat32_plugin

LOCAL_CFLAGS += \
    -std=gnu99 \
    -DHAVE_CONFIG_H \
    -D__PLUGIN__ \
    -DMODULE_STRING=\"a52tofloat32\" \
    -DMODULE_NAME=a52tofloat32

LOCAL_C_INCLUDES += \
    $(VLCROOT) \
    $(VLCROOT)/include \
    $(VLCROOT)/src

LOCAL_SRC_FILES := \
    a52tofloat32.c

LOCAL_STATIC_LIBRARIES += liba52

LOCAL_SHARED_LIBRARIES += libvlccore

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)

#LOCAL_MODULE := converter_fixed_plugin

LOCAL_CFLAGS += \
    -std=gnu99 \
    -DHAVE_CONFIG_H \
    -D__PLUGIN__ \
    -DMODULE_STRING=\"converter_fixed\" \
    -DMODULE_NAME=converter_fixed

LOCAL_C_INCLUDES += \
    $(VLCROOT) \
    $(VLCROOT)/include \
    $(VLCROOT)/src

LOCAL_SRC_FILES := \
    fixed.c

LOCAL_SHARED_LIBRARIES += libvlccore

#include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := dtstofloat32_plugin

LOCAL_CFLAGS += \
    -std=gnu99 \
    -DHAVE_CONFIG_H \
    -D__PLUGIN__ \
    -DMODULE_STRING=\"dtstofloat32\" \
    -DMODULE_NAME=dtstofloat32

LOCAL_C_INCLUDES += \
    $(VLCROOT) \
    $(VLCROOT)/include \
    $(VLCROOT)/src

LOCAL_SRC_FILES := \
    dtstofloat32.c

LOCAL_STATIC_LIBRARIES += libdca

LOCAL_SHARED_LIBRARIES += libvlccore

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)

#LOCAL_MODULE := mpgatofixed32_plugin

LOCAL_CFLAGS += \
    -std=gnu99 \
    -DHAVE_CONFIG_H \
    -D__PLUGIN__ \
    -DMODULE_STRING=\"mpgatofixed32\" \
    -DMODULE_NAME=mpgatofixed32

LOCAL_C_INCLUDES += \
    $(VLCROOT) \
    $(VLCROOT)/include \
    $(VLCROOT)/src

LOCAL_SRC_FILES := \
    mpgatofixed32.c

LOCAL_STATIC_LIBRARIES += libmad

LOCAL_SHARED_LIBRARIES += libvlccore

#include $(BUILD_SHARED_LIBRARY)

