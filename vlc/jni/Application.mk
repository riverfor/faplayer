OPT_CFLAGS := -O3 -fstrict-aliasing -fprefetch-loop-arrays -ffast-math -mlong-calls -march=armv7-a -mtune=cortex-a9 -mfpu=vfpv3-d16
APP_OPTIM := release
APP_ABI := armeabi-v7a
APP_CFLAGS := $(APP_CFLAGS) $(OPT_CFLAGS)
APP_CPPFLAGS := $(APP_CPPFLAGS) $(OPT_CPPFLAGS) 
APP_STL := gnustl_static
BUILD_WITH_ARM := 1
BUILD_WITH_ARM_VFPV3D16 := 1