# MallVR for Meta Quest: our DOSBox-X tree (DBX = H:/Android/src/mallvr-dbx, made by sync_android_tree.sh) + SDL2 + gl4es + OpenXR.
# vpix_render.cpp is its own module, built with _WIN32 + compat/ (fake Windows -> EGL / gl4es), so its Windows code runs as is.
LOCAL_PATH := $(call my-dir)
TOP := $(abspath $(LOCAL_PATH))
COMPAT := $(TOP)/../compat
XRINC := $(XR_LOADER)/prefab/modules/headers/include

include $(SDL)/Android.mk
include $(GL4ES)/Android.mk

include $(CLEAR_VARS)
LOCAL_MODULE := openxr_loader
LOCAL_SRC_FILES := $(XR_LOADER)/prefab/modules/openxr_loader/libs/android.arm64-v8a/libopenxr_loader.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_PATH := $(DBX)
LOCAL_MODULE := vpixrender
LOCAL_SRC_FILES := src/hardware/vpix_render.cpp
LOCAL_C_INCLUDES := $(COMPAT) $(GL4ES)/include $(DBX)/src/hardware $(XRINC) $(DBX)/include
LOCAL_CFLAGS := -O2 -w -include $(COMPAT)/win32_prelude.h -fdeclspec -fms-extensions
LOCAL_CPPFLAGS := -std=gnu++14 -fexceptions -frtti
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_PATH := $(DBX)
LOCAL_MODULE := main
LOCAL_SRC_FILES := $(shell cat $(TOP)/../../dosbox_android/jni/sources.txt) \
    src/hardware/vpix.cpp src/hardware/vpix_hle.cpp src/hardware/vpix_net.cpp \
    $(patsubst $(DBX)/%,%,$(wildcard $(DBX)/src/hardware/su2000/*.cpp)) \
    $(COMPAT)/vpix_android.cpp $(TOP)/../../dosbox_android/android_stubs.cpp
LOCAL_C_INCLUDES := $(TOP)/../../dosbox_android $(DBX)/include $(DBX)/src $(DBX)/src/cpu $(DBX)/src/aviwriter \
    $(DBX)/src/hardware/snd_pc98/cbus $(DBX)/src/hardware/snd_pc98/common $(DBX)/src/hardware/snd_pc98/generic \
    $(DBX)/src/hardware/snd_pc98/sound $(DBX)/src/hardware/snd_pc98/x11 $(DBX)/src/hardware/snd_pc98/sound/getsnd \
    $(SDL)/include $(DBX)/vs/libpng $(DBX)/src/hardware $(DBX)/src/hardware/su2000 $(COMPAT)
LOCAL_CFLAGS := -O2 -w -D_FILE_OFFSET_BITS=64 -DC_SDL2=1 -DRESDIR=\"/sdcard/MallVR\" -fno-strict-aliasing -DPNG_ARM_NEON_OPT=0 -D_stricmp=strcasecmp -D_strnicmp=strncasecmp
LOCAL_CPPFLAGS := -std=gnu++14 -fexceptions -frtti
LOCAL_STATIC_LIBRARIES := vpixrender GL
LOCAL_SHARED_LIBRARIES := SDL2 openxr_loader
LOCAL_LDFLAGS := -Wl,--error-limit=0 -Wl,--no-dependent-libraries
LOCAL_LDLIBS := -llog -landroid -lz -lEGL -ldl
LOCAL_SHORT_COMMANDS := true
include $(BUILD_SHARED_LIBRARY)
