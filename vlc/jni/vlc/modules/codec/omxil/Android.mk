
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := android_iomx_gingerbread_plugin

LOCAL_CFLAGS += \
    -std=gnu99 \
    -DHAVE_CONFIG_H \
    -D__PLUGIN__ \
    -DMODULE_STRING=\"android_iomx_gingerbread\" \
    -DMODULE_NAME=android_iomx_gingerbread \
    -DOMXIL_EXTRA_DEBUG \
    -DUSE_IOMX \
    -DTARGET_SDK_VERSION=9

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

include $(CLEAR_VARS)

LOCAL_MODULE := omx_plugin

LOCAL_CFLAGS += \
    -std=gnu99 \
    -DHAVE_CONFIG_H \
    -D__PLUGIN__ \
    -DMODULE_STRING=\"omx\" \
    -DMODULE_NAME=omx

LOCAL_CPPFLAGS += \
    -fno-exceptions \
    -fno-rtti

LOCAL_C_INCLUDES += \
    $(VLCROOT) \
    $(VLCROOT)/include \
    $(VLCROOT)/src

LOCAL_SRC_FILES := \
    omxil.c \
    utils.c

LOCAL_SHARED_LIBRARIES += libvlccore

include $(BUILD_SHARED_LIBRARY)

