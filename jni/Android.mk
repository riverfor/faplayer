
LOCAL_PATH := $(call my-dir)

BUILD_WITH_NEON := 1

VLCROOT := $(LOCAL_PATH)/vlc
EXTROOT := $(LOCAL_PATH)/ext
DEPROOT := $(LOCAL_PATH)/dep

# build for arm1136j-s target is proper
# armv6j/soft float
# for example, HTC G1, and later phones
COMMON_OPT_CFLAGS := -march=armv6j -mtune=arm1136j-s -msoft-float
COMMON_OPT_CPPFLAGS := -march=armv6j -mtune=arm1136j-s -msoft-float
# neon related flags
NEON_OPT_CFLAGS := -march=armv7-a -mtune=cortex-a8 -mfloat-abi=softfp -mfpu=neon -mvectorize-with-neon-quad
NEON_OPT_CPPFLAGS := -march=armv7-a -mtune=cortex-a8 -mfloat-abi=softfp -mfpu=neon -mvectorize-with-neon-quad
NEON_OPT_LDFLAGS := -Wl,--fix-cortex-a8

include $(CLEAR_VARS)

include $(call all-makefiles-under,$(LOCAL_PATH))

