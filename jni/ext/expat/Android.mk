
LOCAL_PATH:= $(call my-dir)

common_SRC_FILES := \
	lib/xmlparse.c \
	lib/xmlrole.c \
	lib/xmltok.c

common_CFLAGS := -Wall -Wmissing-prototypes -Wstrict-prototypes -fexceptions -DHAVE_EXPAT_CONFIG_H

common_C_INCLUDES += \
	$(LOCAL_PATH)/lib

include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm
ifeq ($(BUILD_WITH_NEON),1)
LOCAL_ARM_NEON := true
endif

LOCAL_SRC_FILES := $(common_SRC_FILES)
LOCAL_CFLAGS += $(common_CFLAGS)
LOCAL_C_INCLUDES += $(common_C_INCLUDES)

LOCAL_MODULE:= libexpat

include $(BUILD_STATIC_LIBRARY)

