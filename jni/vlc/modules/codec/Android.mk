
LOCAL_PATH := $(call my-dir)

# liblibass_plugin.so

include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm
ifeq ($(BUILD_WITH_NEON),1)
LOCAL_ARM_NEON := true
endif

# "^lib" is auto removed
LOCAL_MODULE := liblibass_plugin

LOCAL_CFLAGS += \
    -std=c99 \
    -DHAVE_CONFIG_H \
    -D__PLUGIN__ \
    -DMODULE_STRING=\"libass\"

LOCAL_CFLAGS += $(COMMON_TUNE_CFLAGS)
LOCAL_LDFLAGS += $(COMMON_TUNE_LDFLAGS)

LOCAL_C_INCLUDES += \
    $(VLCROOT)/compat \
    $(VLCROOT) \
    $(VLCROOT)/include \
    $(VLCROOT)/src \
    $(EXTROOT)/iconv/include \
    $(EXTROOT)/freetype/include \
    $(EXTROOT)/libass/include

LOCAL_SRC_FILES := \
    libass.c

LOCAL_SHARED_LIBRARIES += vlccore

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)

include $(call all-makefiles-under,$(LOCAL_PATH))

