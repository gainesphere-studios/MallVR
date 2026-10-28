LOCAL_PATH := $(call my-dir)

# Khronos OpenXR loader (from the Maven AAR, unpacked to $(XR_LOADER))
include $(CLEAR_VARS)
LOCAL_MODULE := openxr_loader
LOCAL_SRC_FILES := $(XR_LOADER)/prefab/modules/openxr_loader/libs/android.arm64-v8a/libopenxr_loader.so
LOCAL_EXPORT_C_INCLUDES := $(XR_LOADER)/prefab/modules/headers/include
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := mallvr_hello
LOCAL_SRC_FILES := main.c
LOCAL_CFLAGS := -Wall -O2
LOCAL_LDLIBS := -llog -landroid -lEGL -lGLESv3
LOCAL_STATIC_LIBRARIES := android_native_app_glue
LOCAL_SHARED_LIBRARIES := openxr_loader
LOCAL_LDFLAGS := -u ANativeActivity_onCreate
include $(BUILD_SHARED_LIBRARY)

$(call import-module,android/native_app_glue)
