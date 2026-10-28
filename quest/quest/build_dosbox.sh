#!/bin/bash
# MallVR Quest proof 2: cross-build DOSBox-X for Android arm64 with SDL2 (ndk-build). Log: H:\Android\build\dosbox\build.log
#   bash quest/build_dosbox.sh [DOSBox-X tree, default H:/Android/src/dosbox-x (stock 2026.10.01)]
A=/h/Android; out=$A/build/dosbox; DBX=${1:-H:/Android/src/dosbox-x}
mkdir -p "$out"; cp -r "$(dirname "$0")/dosbox_android/." "$out/"
cp "$(cygpath "$DBX")/vs/config_package.h" "$out/"; cd "$out"
cmd //c "H:\Android\ndk\ndk-build.cmd" -k -j16 NDK_PROJECT_PATH=. APP_BUILD_SCRIPT=jni/Android.mk NDK_APPLICATION_MK=jni/Application.mk \
    DBX=$DBX SDL=H:/Android/src/SDL2 > build.log 2>&1
echo "exit $?"; grep -c "error:" build.log; ls -la libs/arm64-v8a 2>/dev/null

# --- APK: SDL2's Java activity + our DosActivity -> classes.dex, then the same aapt/zipalign/apksigner steps as build_apk.sh
[ -f libs/arm64-v8a/libmain.so ] || exit 1
export JAVA_HOME='H:\Android\jdk17'; J=$A/jdk17/bin; BT=$A/sdk/build-tools/34.0.0; JAR='H:\Android\sdk\platforms\android-32\android.jar'
rm -rf classes dex apk; mkdir -p classes dex apk/lib/arm64-v8a
"$J/javac.exe" -nowarn -encoding UTF-8 -source 8 -target 8 -Xlint:-options -cp "$JAR" -d classes \
    $(cygpath -w H:/Android/src/SDL2/android-project/app/src/main/java/org/libsdl/app/*.java) $(cygpath -w java/com/gainesphere/mallvr/*.java) 2>&1 | grep -v "^Note:"
cmd //c "H:\Android\sdk\build-tools\34.0.0\d8.bat" --release --min-api 29 --lib "$JAR" --output dex $(cd classes && find . -name "*.class" | sed 's#^\./#classes/#')
"$BT/aapt.exe" package -f -M AndroidManifest.xml -I "$JAR" -F unsigned.apk
cp dex/classes.dex apk/; cp libs/arm64-v8a/*.so apk/lib/arm64-v8a/
cd apk; "$BT/aapt.exe" add ../unsigned.apk classes.dex lib/arm64-v8a/*.so >/dev/null; cd ..
"$BT/zipalign.exe" -p -f 4 unsigned.apk aligned.apk
cmd //c "H:\Android\sdk\build-tools\34.0.0\apksigner.bat" sign --ks 'H:\Android\keys\debug.keystore' --ks-pass pass:android --out mallvr_dosbox_test.apk aligned.apk
ls -la mallvr_dosbox_test.apk; unzip -l mallvr_dosbox_test.apk | tail -8
