# armeabi/armeabi-v7a

_ABI ?= $(ABI)
ABI := $(_ABI)
_FPU ?= $(FPU)
FPU := $(_FPU)
_TUNE ?= $(TUNE)
TUNE := $(_TUNE)

ifeq ($(ABI),)
    ABI := armeabi-v7a
endif
ifeq ($(FPU),)
    FPU := neon
endif
ifeq ($(TUNE),)
    TUNE := cortex-a8
endif

APP_ABI := $(ABI)
APP_OPTIM := release

OPT_CFLAGS := -O3 -mlong-calls -fstrict-aliasing -fprefetch-loop-arrays -ffast-math

ifeq ($(APP_ABI),armeabi-v7a)
    ifeq ($(FPU),neon)
        OPT_CFLAGS += -mfpu=neon -mtune=$(TUNE) -ftree-vectorize -mvectorize-with-neon-quad
        BUILD_WITH_NEON := 1
    else
        OPT_CFLAGS += -mfpu=$(FPU) -mtune=$(TUNE) -ftree-vectorize
        BUILD_WITH_NEON := 0
    endif
else
    OPT_CFLAGS += -march=armv6j -mtune=arm1136j-s -msoft-float
    BUILD_WITH_NEON := 0
endif

OPT_CPPFLAGS := $(OPT_CLFAGS)

APP_CFLAGS := $(APP_CFLAGS) $(OPT_CFLAGS)
APP_CPPFLAGS := $(APP_CPPFLAGS) $(OPT_CPPFLAGS) 

APP_STL := gnustl_static

