LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_DEFAULT_CPP_EXTENSION := cpp 

LOCAL_MODULE    := dhdalvikpatch
LOCAL_SRC_FILES += mapinfo.cpp dalvikpatch.cpp main.cpp 

LOCAL_LDLIBS += -llog

include $(BUILD_SHARED_LIBRARY)
