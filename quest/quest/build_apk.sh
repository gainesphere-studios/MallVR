#!/bin/bash
# MallVR Quest: build a NativeActivity APK with no Gradle (ndk-build + aapt + zipalign + apksigner).
#   bash quest/build_apk.sh <project dir with AndroidManifest.xml + jni/> <apk name>
# Builds in H:\Android\build\<name> (make can't cope with the apostrophe in "Claude's Folder"). Debug key = H:\Android\keys\debug.keystore.
set -e
A=/h/Android; SDK=$A/sdk; BT=$SDK/build-tools/34.0.0; JAR=$SDK/platforms/android-32/android.jar
export JAVA_HOME='H:\Android\jdk17'; export PATH="$A/jdk17/bin:$PATH"
src="$1"; name="$2"; out=$A/build/$name
rm -rf "$out"; mkdir -p "$out"; cp -r "$src"/. "$out"/
cd "$out"
cmd //c "H:\\Android\\ndk\\ndk-build.cmd" -j8 NDK_PROJECT_PATH=. APP_BUILD_SCRIPT=jni/Android.mk NDK_APPLICATION_MK=jni/Application.mk XR_LOADER=H:/Android/openxr_loader $EXTRA_NDK
mkdir -p apk/lib/arm64-v8a; cp libs/arm64-v8a/*.so apk/lib/arm64-v8a/
"$BT/aapt.exe" package -f -M AndroidManifest.xml -I "$JAR" $( [ -d assets ] && echo "-A assets" ) -F unsigned.apk
cd apk; for f in lib/arm64-v8a/*.so; do "$BT/aapt.exe" add ../unsigned.apk "$f" >/dev/null; done; cd ..
"$BT/zipalign.exe" -p -f 4 unsigned.apk aligned.apk
mkdir -p $A/keys
[ -f $A/keys/debug.keystore ] || keytool -genkeypair -keystore 'H:\Android\keys\debug.keystore' -storepass android -keypass android \
    -alias androiddebugkey -keyalg RSA -keysize 2048 -validity 10000 -dname "CN=Android Debug,O=Android,C=US" >/dev/null
cmd //c "H:\\Android\\sdk\\build-tools\\34.0.0\\apksigner.bat" sign --ks 'H:\Android\keys\debug.keystore' --ks-pass pass:android --out "$name.apk" aligned.apk
cmd //c "H:\\Android\\sdk\\build-tools\\34.0.0\\apksigner.bat" verify --print-certs "$name.apk" | head -2
ls -la "$name.apk"; unzip -l "$name.apk"
