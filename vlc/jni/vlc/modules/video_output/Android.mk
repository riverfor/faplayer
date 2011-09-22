
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

ifeq ($(BUILD_WITH_ARM),1)
LOCAL_ARM_MODE := arm
endif

LOCAL_MODULE := android_surface_plugin

LOCAL_CFLAGS += \
    -std=gnu99 \
    -DHAVE_CONFIG_H \
    -D__PLUGIN__ \
    -DMODULE_STRING=\"android_surface\" \
    -DMODULE_NAME=android_surface

LOCAL_C_INCLUDES += \
    $(VLCROOT) \
    $(VLCROOT)/include \
    $(VLCROOT)/src

LOCAL_SRC_FILES := \
    androidsurface.c

LOCAL_SHARED_LIBRARIES += libvlccore

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)

ifeq ($(BUILD_WITH_ARM),1)
LOCAL_ARM_MODE := arm
endif

LOCAL_MODULE := vmem_plugin

LOCAL_CFLAGS += \
    -std=gnu99 \
    -DHAVE_CONFIG_H \
    -D__PLUGIN__ \
    -DMODULE_STRING=\"vmem\" \
    -DMODULE_NAME=vmem

LOCAL_C_INCLUDES += \
    $(VLCROOT) \
    $(VLCROOT)/include \
    $(VLCROOT)/src

LOCAL_SRC_FILES := \
    vmem.c

LOCAL_SHARED_LIBRARIES += libvlccore

include $(BUILD_SHARED_LIBRARY)

