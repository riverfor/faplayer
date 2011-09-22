
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

ifeq ($(BUILD_WITH_ARM),1)
LOCAL_ARM_MODE := arm
endif

LOCAL_MODULE:= liba52

LOCAL_CFLAGS += -DHAVE_CONFIG_H

LOCAL_C_INCLUDES += $(LOCAL_PATH)/include

LOCAL_SRC_FILES := \
	liba52/bit_allocate.c \
	liba52/bitstream.c \
	liba52/downmix.c \
	liba52/imdct.c \
	liba52/parse.c

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/include

include $(BUILD_STATIC_LIBRARY)

