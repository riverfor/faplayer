
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm
ifeq ($(BUILD_WITH_NEON),1)
LOCAL_ARM_NEON := true
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

include $(BUILD_STATIC_LIBRARY)

