# armeabi/armeabi-v7a
APP_ABI := armeabi-v7a
# default depends on AndroidManifest.xml
APP_OPTIM := release
# default is -O2
OPT_CFLAGS := -O3 -mlong-calls -fstrict-aliasing -fprefetch-loop-arrays -ffast-math
ifeq ($(APP_ABI),armeabi-v7a)
# most common case
OPT_CFLAGS += -mfpu=neon -mtune=cortex-a8 -ftree-vectorize -mvectorize-with-neon-quad
BUILD_WITH_NEON := 1
# tegra 2 without neon
#OPT_CFLAGS += -mfpu=vfpv3 -mtune=cortex-a9 -ftree-vectorize
#BUILD_WITH_NEON := 0
else
OPT_CFLAGS += -march=armv6j -mtune=arm1136j-s -msoft-float
BUILD_WITH_NEON := 0
endif
OPT_CPPFLAGS := $(OPT_CLFAGS)
# override default
APP_CFLAGS := $(APP_CFLAGS) $(OPT_CFLAGS)
APP_CPPFLAGS := $(APP_CPPFLAGS) $(OPT_CPPFLAGS) 
# stl
APP_STL := stlport_static
