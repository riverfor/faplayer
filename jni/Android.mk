
LOCAL_PATH := $(call my-dir)

BUILD_WITH_NEON := 1

ifeq ($(APP_ABI),armeabi)
BUILD_WITH_NEON := 0
endif

VLCROOT := $(LOCAL_PATH)/vlc
EXTROOT := $(LOCAL_PATH)/ext
DEPROOT := $(LOCAL_PATH)/dep

COMMON_TUNE_CFLAGS := -mlong-calls -fstrict-aliasing -fprefetch-loop-arrays -ffast-math
COMMON_TUNE_CPPFLAGS := -mlong-calls -fstrict-aliasing -fprefetch-loop-arrays -ffast-math
COMMON_TUNE_LDFLAGS :=

ifeq ($(BUILD_WITH_NEON),1)
COMMON_TUNE_CFLAGS += -mfpu=neon -mtune=cortex-a8 -ftree-vectorize -mvectorize-with-neon-quad
COMMON_TUNE_CPPFLAGS += -mfpu=neon -mtune=cortex-a8 -ftree-vectorize -mvectorize-with-neon-quad
COMMON_TUNE_LDFLAGS += -Wl,--fix-cortex-a8
else
COMMON_TUNE_CFLAGS += -march=armv6j -mtune=arm1136j-s -msoft-float
COMMON_TUNE_CPPFLAGS += -march=armv6j -mtune=arm1136j-s -msoft-float
COMMON_TUNE_LDFLAGS += 
endif

include $(CLEAR_VARS)

include $(call all-makefiles-under,$(LOCAL_PATH))

