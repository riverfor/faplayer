# armeabi/armeabi-v7a or both
APP_ABI := armeabi-v7a
# default depends on AndroidManifest.xml
APP_OPTIM := release
# default is -O2
OPT_CFLAGS := -O3
OPT_CPPFLAGS := $(OPT_CLFAGS)
# override default
APP_CFLAGS := $(APP_CFLAGS) $(OPT_CFLAGS)
APP_CPPFLAGS := $(APP_CPPFLAGS) $(OPT_CPPFLAGS) 
