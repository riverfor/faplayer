
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

ifeq ($(BUILD_WITH_ARM),1)
LOCAL_ARM_MODE := arm
endif

LOCAL_MODULE := libiconv

LOCAL_CFLAGS += \
    -DHAVE_CONFIG_H \
    -DBUILDING_LIBICONV \
    -DIN_LIBRARY

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/include \
    $(LOCAL_PATH)/libcharset/include

LOCAL_SRC_FILES := \
    lib/iconv.c

LOCAL_STATIC_LIBRARIES += libcharset

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/include

include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)

include $(call all-makefiles-under,$(LOCAL_PATH))

