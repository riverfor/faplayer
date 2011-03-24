
LOCAL_PATH := $(call my-dir)

#libvout_android-4_plugin.so

include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm
ifeq ($(BUILD_WITH_NEON),1)
LOCAL_ARM_NEON := true
endif

LOCAL_MODULE := vout_android-4_plugin

LOCAL_CFLAGS += \
    -D__PLATFORM__=4 \
    -D__THROW= \
    -DHAVE_CONFIG_H \
    -DNDEBUG \
    -D__PLUGIN__ \
    -DMODULE_STRING=\"vout_android-4\"

LOCAL_CPPFLAGS += $(COMMON_TUNE_CFLAGS)
LOCAL_LDFLAGS += $(COMMON_TUNE_LDFLAGS)

LOCAL_C_INCLUDES += \
    $(EXTROOT)/pixman/pixman \
    $(DEPROOT)/android-4/include \
    $(VLCROOT)/compat \
    $(VLCROOT) \
    $(VLCROOT)/include \
    $(VLCROOT)/src

LOCAL_SRC_FILES := \
    android_Surface.cpp

LOCAL_LDFLAGS += -L$(DEPROOT)/android-4/lib
LOCAL_LDLIBS += -lui

LOCAL_SHARED_LIBRARIES += vlccore

include $(BUILD_SHARED_LIBRARY)

#libvout_android-5_plugin.so

include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm
ifeq ($(BUILD_WITH_NEON),1)
LOCAL_ARM_NEON := true
endif

LOCAL_MODULE := vout_android-5_plugin

LOCAL_CFLAGS += \
    -D__PLATFORM__=5 \
    -D__THROW= \
    -DHAVE_CONFIG_H \
    -DNDEBUG \
    -D__PLUGIN__ \
    -DMODULE_STRING=\"vout_android-5\"

LOCAL_CPPFLAGS += $(COMMON_TUNE_CFLAGS)
LOCAL_LDFLAGS += $(COMMON_TUNE_LDFLAGS)

LOCAL_C_INCLUDES += \
    $(EXTROOT)/pixman/pixman \
    $(DEPROOT)/android-5/include \
    $(VLCROOT)/compat \
    $(VLCROOT) \
    $(VLCROOT)/include \
    $(VLCROOT)/src

LOCAL_SRC_FILES := \
    android_Surface.cpp

LOCAL_LDFLAGS += -L$(DEPROOT)/android-5/lib
LOCAL_LDLIBS += -lui

LOCAL_SHARED_LIBRARIES += vlccore

include $(BUILD_SHARED_LIBRARY)

#libvout_android-8_plugin.so

include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm
ifeq ($(BUILD_WITH_NEON),1)
LOCAL_ARM_NEON := true
endif

LOCAL_MODULE := vout_android-8_plugin

LOCAL_CFLAGS += \
    -D__PLATFORM__=8 \
    -D__THROW= \
    -DHAVE_CONFIG_H \
    -DNDEBUG \
    -D__PLUGIN__ \
    -DMODULE_STRING=\"vout_android-8\"

LOCAL_CPPFLAGS += $(COMMON_TUNE_CFLAGS)
LOCAL_LDFLAGS += $(COMMON_TUNE_LDFLAGS)

LOCAL_C_INCLUDES += \
    $(EXTROOT)/pixman/pixman \
    $(DEPROOT)/android-8/include \
    $(VLCROOT)/compat \
    $(VLCROOT) \
    $(VLCROOT)/include \
    $(VLCROOT)/src

LOCAL_SRC_FILES := \
    android_Surface.cpp

LOCAL_LDFLAGS += -L$(DEPROOT)/android-8/lib
LOCAL_LDLIBS += -lsurfaceflinger_client

LOCAL_SHARED_LIBRARIES += vlccore

include $(BUILD_SHARED_LIBRARY)

#libvout_android-9_plugin.so

include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm
ifeq ($(BUILD_WITH_NEON),1)
LOCAL_ARM_NEON := true
endif

LOCAL_MODULE := vout_android-9_plugin

LOCAL_CFLAGS += \
    -D__PLATFORM__=9 \
    -D__THROW= \
    -DHAVE_CONFIG_H \
    -DNDEBUG \
    -D__PLUGIN__ \
    -DMODULE_STRING=\"vout_android-9\"

LOCAL_CPPFLAGS += $(COMMON_TUNE_CFLAGS)
LOCAL_LDFLAGS += $(COMMON_TUNE_LDFLAGS)

LOCAL_C_INCLUDES += \
    $(EXTROOT)/pixman/pixman \
    $(DEPROOT)/android-9/include \
    $(VLCROOT)/compat \
    $(VLCROOT) \
    $(VLCROOT)/include \
    $(VLCROOT)/src

LOCAL_SRC_FILES := \
    android_Surface.cpp

LOCAL_LDFLAGS += -L$(DEPROOT)/android-9/lib
LOCAL_LDLIBS += -lsurfaceflinger_client

LOCAL_SHARED_LIBRARIES += vlccore

include $(BUILD_SHARED_LIBRARY)

