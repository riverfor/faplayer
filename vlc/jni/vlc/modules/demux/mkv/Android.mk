
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

ifeq ($(BUILD_WITH_ARM),1)
LOCAL_ARM_MODE := arm
endif

LOCAL_MODULE := libmkv_plugin

LOCAL_CFLAGS += \
    -std=gnu99 \
    -DHAVE_CONFIG_H \
    -D__PLUGIN__ \
    -DMODULE_STRING=\"mkv\" \
    -DMODULE_NAME=mkv

LOCAL_CPPFLAGS += \
    -DHAVE_CONFIG_H \
    -D__PLUGIN__ \
    -DMODULE_STRING=\"mkv\" \
    -DMODULE_NAME=mkv

LOCAL_C_INCLUDES += \
    $(VLCROOT) \
    $(VLCROOT)/include \
    $(VLCROOT)/src

LOCAL_SRC_FILES := \
	chapter_command.cpp \
	chapters.cpp \
	demux.cpp \
	Ebml_parser.cpp \
	matroska_segment.cpp \
	matroska_segment_parse.cpp \
	mkv.cpp \
	stream_io_callback.cpp \
	util.cpp \
	virtual_segment.cpp \
	../mp4/libmp4.c \
	../mp4/drms.c

LOCAL_SHARED_LIBRARIES += libvlccore

LOCAL_STATIC_LIBRARIES += libmatroska libebml

LOCAL_LDLIBS += -lz

include $(BUILD_SHARED_LIBRARY)

