
LOCAL_PATH := $(call my-dir)

# libfreetype_plugin.so

include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm

LOCAL_MODULE := freetype_plugin

LOCAL_CFLAGS += \
    -std=c99 \
    -D__THROW= \
    -DHAVE_CONFIG_H \
    -DNDEBUG \
    -D__PLUGIN__ \
    -DMODULE_STRING=\"freetype\"

LOCAL_CFLAGS += $(COMMON_OPT_CFLAGS)
LOCAL_LDFLAGS += $(COMMON_OPT_LDFLAGS)

LOCAL_C_INCLUDES += \
    $(EXTROOT)/iconv/include \
    $(EXTROOT)/freetype/include \
    $(VLCROOT)/compat \
    $(VLCROOT) \
    $(VLCROOT)/include \
    $(VLCROOT)/src

LOCAL_SRC_FILES := \
    freetype.c

LOCAL_SHARED_LIBRARIES += vlccore

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)

include $(call all-makefiles-under,$(LOCAL_PATH))

