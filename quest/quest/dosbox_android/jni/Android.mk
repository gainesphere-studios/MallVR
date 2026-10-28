# DOSBox-X (stock or ours) for Android arm64 + SDL2. DBX = DOSBox-X tree, SDL = SDL2 tree (passed by build_dosbox.sh).
LOCAL_PATH := $(call my-dir)
TOP := $(abspath $(LOCAL_PATH))

include $(SDL)/Android.mk

include $(CLEAR_VARS)
LOCAL_PATH := $(DBX)
LOCAL_MODULE := main
LOCAL_SRC_FILES := $(shell cat $(TOP)/sources.txt) $(TOP)/../android_stubs.cpp
LOCAL_C_INCLUDES := $(TOP)/.. $(DBX)/include $(DBX)/src $(DBX)/src/cpu $(DBX)/src/aviwriter \
    $(DBX)/src/hardware/snd_pc98/cbus $(DBX)/src/hardware/snd_pc98/common $(DBX)/src/hardware/snd_pc98/generic \
    $(DBX)/src/hardware/snd_pc98/sound $(DBX)/src/hardware/snd_pc98/x11 $(DBX)/src/hardware/snd_pc98/sound/getsnd \
    $(SDL)/include $(DBX)/vs/libpng
LOCAL_CFLAGS := -O2 -w -D_FILE_OFFSET_BITS=64 -DC_SDL2=1 -DRESDIR=\"/sdcard/MallVR\" -fno-strict-aliasing -DPNG_ARM_NEON_OPT=0
LOCAL_CPPFLAGS := -std=gnu++14 -fexceptions -frtti
LOCAL_SHARED_LIBRARIES := SDL2
LOCAL_LDFLAGS := -Wl,--error-limit=0
LOCAL_LDLIBS := -llog -landroid -lz -lGLESv2
include $(BUILD_SHARED_LIBRARY)
