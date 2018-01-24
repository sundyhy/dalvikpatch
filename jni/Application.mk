APP_ABI := armeabi-v7a
STLPORT_FORCE_REBUILD := false
APP_STL := gnustl_static
APP_CPPFLAGS += -fexceptions 
APP_CPPFLAGS += -frtti -fPIC -fpic
APP_CPPFLAGS += -fpermissive
APP_CFLAGS += -Wno-error=format-security 
GLOBAL_CFLAGS += -fvisibility=default
APP_PLATFORM := android-9