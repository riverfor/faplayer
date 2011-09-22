
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

ifeq ($(BUILD_WITH_ARM),1)
LOCAL_ARM_MODE := arm
endif

LOCAL_MODULE := iomx_gingerbread_plugin

LOCAL_CFLAGS += \
    -std=gnu99 \
    -DHAVE_CONFIG_H \
    -D__PLUGIN__ \
    -DMODULE_STRING=\"iomx_gingerbread\" \
    -DMODULE_NAME=iomx_gingerbread \
    -DUSE_IOMX

LOCAL_CPPFLAGS += \
    -fno-exceptions \
    -fno-rtti

LOCAL_C_INCLUDES += \
    $(VLCROOT) \
    $(VLCROOT)/include \
    $(VLCROOT)/src

LOCAL_SRC_FILES := \
    iomx.cpp \
    omxil.c \
    utils.c

LOCAL_SHARED_LIBRARIES += libvlccore

LOCAL_SHARED_LIBRARIES += libbinder_gingerbread libcutils_gingerbread  libmedia_gingerbread libstagefright_gingerbread libutils_gingerbread

LOCAL_LDLIBS += -llog

include $(BUILD_SHARED_LIBRARY)

