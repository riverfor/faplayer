
LOCAL_PATH := $(call my-dir)

BUILD_WITH_NEON := 1

VLCROOT := $(LOCAL_PATH)/vlc
EXTROOT := $(LOCAL_PATH)/ext
DEPROOT := $(LOCAL_PATH)/dep

ifneq ($(BUILD_WITH_NEON),1)
COMMON_OPT_CFLAGS := -march=armv6j -mtune=arm1136j-s -msoft-float
COMMON_OPT_CPPFLAGS := -march=armv6j -mtune=arm1136j-s -msoft-float
COMMON_OPT_LDFLAGS := 
else
COMMON_OPT_CFLAGS := -march=armv7-a -mtune=cortex-a8 -mfloat-abi=softfp -mfpu=neon
COMMON_OPT_CPPFLAGS := -march=armv7-a -mtune=cortex-a8 -mfloat-abi=softfp -mfpu=neon
COMMON_OPT_LDFLAGS := -Wl,--fix-cortex-a8
endif

include $(CLEAR_VARS)

include $(call all-makefiles-under,$(LOCAL_PATH))

