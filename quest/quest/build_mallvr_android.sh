#!/bin/bash
# MallVR for Meta Quest: sync the Android tree from dosbox_src/, then ndk-build the engine (libmain.so + SDL2 + OpenXR loader).
# Builds in H:\Android\build\mallvr\q\mallvr_android (make can't cope with the apostrophe in "Claude's Folder").
# Log: H:\Android\build\mallvr\build.log
A=/h/Android; out=$A/build/mallvr; Q="$(cd "$(dirname "$0")" && pwd)"
bash "$Q/sync_android_tree.sh" || exit 1
mkdir -p "$out/q"; rm -rf "$out/q/mallvr_android/jni" "$out/q/mallvr_android/compat" "$out/q/dosbox_android"
cp -r "$Q/mallvr_android" "$Q/dosbox_android" "$out/q/"            # Android.mk reads ../../dosbox_android (config.h, sources.txt)
cp /h/Android/src/mallvr-dbx/vs/config_package.h "$out/q/dosbox_android/"
cd "$out/q/mallvr_android"
cmd //c "H:\\Android\\ndk\\ndk-build.cmd" -k -j16 NDK_PROJECT_PATH=. APP_BUILD_SCRIPT=jni/Android.mk NDK_APPLICATION_MK=jni/Application.mk \
    DBX=H:/Android/src/mallvr-dbx SDL=H:/Android/src/SDL2 GL4ES=H:/Android/src/gl4es XR_LOADER=H:/Android/openxr_loader > "$out/build.log" 2>&1
echo "exit $?"; grep -c "error:" "$out/build.log"; ls -la libs/arm64-v8a 2>/dev/null
