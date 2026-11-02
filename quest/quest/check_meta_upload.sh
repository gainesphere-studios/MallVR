#!/bin/bash
# Meta Horizon Store upload checks, run locally on a built APK (ovr-platform-util has no validate-only command: it checks at upload).
#   bash quest/check_meta_upload.sh [apk]      default: the store build /h/Android/release/<VER>/MallVR-Quest-<VER>-store.apk
# Rules from developers.meta.com/horizon/resources/publish-mobile-manifest (VRC.Quest.Packaging.1 / .4) + the release-key rule.
Q="$(cd "$(dirname "$0")" && pwd)"; BT=/h/Android/sdk/build-tools/34.0.0
VER=$(grep -o "^VER=[0-9.]*" "$Q/build_mallvr_apk.sh" | cut -d= -f2)
apk="${1:-/h/Android/release/$VER/MallVR-Quest-$VER-store.apk}"; [ -f "$apk" ] || { echo "no $apk"; exit 1; }
x=$("$BT/aapt.exe" dump xmltree "$apk" AndroidManifest.xml); b=$("$BT/aapt.exe" dump badging "$apk")
fail=0; ok() { echo "  ok    $1"; }; bad() { echo "  FAIL  $1"; fail=1; }
hexattr() { echo "$x" | grep -m1 "android:$1(" | grep -o "0x[0-9a-f]*$"; }

echo "$apk"
t=$(echo "$b" | grep -o "targetSdkVersion:'[0-9]*'" | grep -o "[0-9]*"); [ "$t" = 34 ] && ok "targetSdkVersion 34" || bad "targetSdkVersion $t (must be 34)"
c=$(hexattr compileSdkVersion); if [ -z "$c" ]; then ok "no compileSdkVersion"; elif [ $((c)) -ge "$t" ]; then ok "compileSdkVersion $((c)) >= target"; else bad "compileSdkVersion $((c)) < targetSdkVersion $t (build against platforms/android-34)"; fi
d=$(hexattr debuggable); [ -z "$d" ] || [ "$d" = 0x0 ] && ok "not debuggable" || bad "android:debuggable is on"
i=$(hexattr installLocation); [ "$i" = 0x0 ] && ok "installLocation auto" || bad "installLocation ${i:-missing} (must be auto)"
echo "$x" | grep -A1 '"com.oculus.supportedDevices"' | grep -q 'quest' && ok "com.oculus.supportedDevices" || bad "no com.oculus.supportedDevices meta-data"
echo "$x" | grep -A1 '"com.gainesphere.mallvr.store"' | grep -q '0xffffffff' && ok "store build (no GET GAMES)" || bad "not the store build: GET GAMES is on (build with --store)"
echo "$x" | grep -A2 '"android.hardware.vr.headtracking"' | grep -q "required.*0xffffffff" && ok "headtracking required (immersive)" || bad "android.hardware.vr.headtracking not required=true"
# the launch activity (MAIN + LAUNCHER) carries excludeFromRecents=true
la=$(echo "$b" | grep -o "launchable-activity: name='[^']*'" | cut -d"'" -f2)
echo "$x" | awk -v n="$la" '/E: activity/{blk=""} {blk=blk"\n"$0} /E: intent-filter/{if (index(blk,"\""n"\"")) {print blk; exit}}' | grep -q "excludeFromRecents.*0xffffffff" \
    && ok "launch activity $la has excludeFromRecents" || bad "launch activity $la lacks excludeFromRecents=true"
# "APK screen orientation is not landscape" (found by Meta's --validate 10-28): every activity screenOrientation="landscape" (= 0)
na=$(echo "$x" | grep -c "E: activity"); nl=$(echo "$x" | grep -c "android:screenOrientation(0x0101001e)=(type 0x10)0x0$")
[ "$nl" = "$na" ] && ok "all $na activities landscape" || bad "$nl of $na activities have screenOrientation=landscape"
s=$(cmd //c "H:\\Android\\sdk\\build-tools\\34.0.0\\apksigner.bat" verify --verbose --print-certs --min-sdk-version 24 "$(cygpath -w "$apk")" 2>/dev/null)
echo "$s" | grep -q "^Verified using v2 scheme.*true" && ok "v2 signature" || bad "no v2 signature"
echo "$s" | grep -q "DN: .*CN=Android Debug" && bad "signed with the Android debug key" || ok "release key: $(echo "$s" | grep -m1 'DN:' | sed 's/.*DN: //')"
echo "  info  $(echo "$b" | grep -o "versionCode='[0-9]*' versionName='[^']*'")"
echo "  info  review-required permissions: $(echo "$b" | grep -oE "android.permission.(MANAGE_EXTERNAL_STORAGE|RECORD_AUDIO)" | sort -u | tr '\n' ' ')"
[ $fail = 0 ] && echo "PASS" || { echo "FAILED"; exit 1; }
