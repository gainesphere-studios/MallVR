#!/bin/bash
# MallVR for Meta Quest: the release APK (launcher + engine), its GPL source zip and the install guide.
#   bash quest/build_mallvr_apk.sh [--engine]     --engine: sync + ndk-build libmain.so first (= build_mallvr_android.sh)
# Out: H:\Android\release\<ver>\  MallVR-Quest-<ver>.apk, MallVR-Quest-<ver>-source.zip, QUEST_INSTALL.txt
# Work dir H:\Android\build\mallvr_apk (no apostrophe for the tools). Same no-Gradle steps as build_dosbox.sh: javac + d8 + aapt +
# zipalign + apksigner. Signed with H:\Android\keys\debug.keystore = THE MallVR Quest key: keep it (backed up), or players have to
# uninstall before an update installs.
set -e
VER=0.2; CODE=2; VNAME=$VER-quest   # 0.2 = online + crossplay with PC (10-26)
A=/h/Android; Q="$(cd "$(dirname "$0")" && pwd)"; P="$(dirname "$Q")"
LIBS=$A/build/mallvr/q/mallvr_android/libs/arm64-v8a
out=$A/build/mallvr_apk; rel=$A/release/$VER; M=$A/dl/maven
CC=$M/commons-compress-1.25.0.jar; XZ=$M/xz-1.9.jar
export JAVA_HOME='H:\Android\jdk17'; J=$A/jdk17/bin; BT=$A/sdk/build-tools/34.0.0; JAR='H:\Android\sdk\platforms\android-32\android.jar'
w() { cygpath -w "$1"; }

[ "$1" = "--engine" ] && bash "$Q/build_mallvr_android.sh"
[ -f $LIBS/libmain.so ] || { echo "no libmain.so: run with --engine"; exit 1; }
if grep -qaE "VPIX_TESTPOSE|VPIX_TESTAIM" $LIBS/libmain.so; then echo "libmain.so contains the test player: never ship it"; exit 1; fi
for f in "$CC" "$XZ"; do [ -f "$f" ] || { echo "missing $f (Maven Central, see QUEST_HANDOFF.md)"; exit 1; }; done

rm -rf "$out"; mkdir -p "$out"/{assets/engine/hooks,assets/engine/buggy,assets/licenses,classes,dex,apk/lib/arm64-v8a} "$rel"

# --- assets: games.json (games.ps1 + play.ps1), the hook tables of those games, Buggy Ball's badges, licence texts
powershell -NoProfile -ExecutionPolicy Bypass -File "$(w "$Q/make_games_json.ps1")" "$(w "$out/assets/games.json")"
for c in $(node -e "for (const g of require(process.argv[1]).games) if (!g.native) console.log(g.code)" "$(w "$out/assets/games.json")"); do
    [ -f "$P/hooks/$c.hooks" ] || { echo "missing hooks/$c.hooks"; exit 1; }
    for f in "$P/hooks/$c.hooks" "$P/hooks/$c.patch" "$P/hooks/${c}_duo.patch" "$P/hooks/$c.game"; do [ -f "$f" ] && cp "$f" "$out/assets/engine/hooks/" || true; done
done
cp "$P"/buggy_rebuild/assets/ball_logo_256.rgba "$P"/buggy_rebuild/assets/ball_logo2_256.rgba "$out/assets/engine/buggy/"
L=$out/assets/licenses
cp $A/src/dosbox-x/COPYING "$L/DOSBox-X_and_MallVR_GPL-2.0.txt"
cp $A/src/SDL2/LICENSE.txt "$L/SDL2_zlib.txt"
cp $A/src/gl4es/LICENSE "$L/gl4es_MIT.txt"
cp $A/openxr_loader/META-INF/LICENSE "$L/OpenXR_loader_Apache-2.0.txt"
(cd "$out" && "$J/jar.exe" xf "$(w "$CC")" META-INF/LICENSE.txt META-INF/NOTICE.txt && cat META-INF/NOTICE.txt META-INF/LICENSE.txt > "$L/Apache_Commons_Compress_Apache-2.0.txt" && rm -rf META-INF)
cp "$Q/mallvr_apk/licenses/"*.txt "$L/"

mkdir -p "$out/javasrc"; cp "$Q"/mallvr_apk/java/com/gainesphere/mallvr/*.java "$out/javasrc/"    # (javac args: no apostrophe paths)
# --- Java: SDL2's activity + ours, Commons Compress + XZ (to unpack the archive.org .7z / .zip files) -> classes.dex
"$J/javac.exe" -nowarn -encoding UTF-8 -source 8 -target 8 -Xlint:-options -cp "$JAR;$(w "$CC");$(w "$XZ")" -d "$(w "$out/classes")" \
    $(cygpath -w $A/src/SDL2/android-project/app/src/main/java/org/libsdl/app/*.java) $(cygpath -w "$out"/javasrc/*.java) 2>&1 | grep -v "^Note:" || true
(cd "$out" && "$J/jar.exe" cfM ours.jar -C classes .)
mkdir -p "$out/libjars"
for j in "$CC" "$XZ"; do      # without META-INF (module-info.class of the multi-release jars upsets d8)
    d="$out/libjars/$(basename "$j" .jar)"; mkdir -p "$d"; (cd "$d" && "$J/jar.exe" xf "$(w "$j")" && rm -rf META-INF)
    "$J/java.exe" "$(w "$Q/tools/StripMethodParams.java")" "$(w "$d")"     # d8 34 crashes on nameless MethodParameters
    (cd "$out" && "$J/jar.exe" cfM "libjars/$(basename "$j")" -C "libjars/$(basename "$j" .jar)" .)
done
(cd "$out" && cmd //c "H:\\Android\\sdk\\build-tools\\34.0.0\\d8.bat" --release --min-api 29 --lib "$JAR" --output dex ours.jar libjars/commons-compress-1.25.0.jar libjars/xz-1.9.jar) 2>&1 | grep -v "^Warning\|Missing class\|^  " | head -20 || true
[ -f "$out/dex/classes.dex" ] || { echo "d8 made no classes.dex"; exit 1; }

# --- APK
sed "s/@VERSION@/$VNAME/; s/@CODE@/$CODE/" "$Q/mallvr_apk/AndroidManifest.xml" > "$out/AndroidManifest.xml"
cp -r "$Q/mallvr_apk/res" "$out/res"
cd "$out"
"$BT/aapt.exe" package -f -M AndroidManifest.xml -S res -A assets -I "$JAR" -F unsigned.apk
cp dex/classes*.dex apk/; cp $LIBS/libmain.so $LIBS/libSDL2.so $LIBS/libopenxr_loader.so $LIBS/libc++_shared.so apk/lib/arm64-v8a/
(cd apk && "$BT/aapt.exe" add ../unsigned.apk classes*.dex lib/arm64-v8a/*.so >/dev/null)
"$BT/zipalign.exe" -p -f 4 unsigned.apk aligned.apk
cmd //c "H:\\Android\\sdk\\build-tools\\34.0.0\\apksigner.bat" sign --v2-signing-enabled true --v3-signing-enabled true --v4-signing-enabled false --ks 'H:\Android\keys\debug.keystore' --ks-pass pass:android --out "$(w "$rel/MallVR-Quest-$VER.apk")" aligned.apk
cmd //c "H:\\Android\\sdk\\build-tools\\34.0.0\\apksigner.bat" verify --verbose "$(w "$rel/MallVR-Quest-$VER.apk")" | head -6

# --- GPL source zip: our files as built (the Android tree's patched copies too) + the stock versions to put them on
S="$out/src/MallVR-Quest-$VER-source"; T=$A/src/mallvr-dbx
mkdir -p "$S/android_tree/src/hardware/su2000" "$S/android_tree/src/gui" "$S/dosbox_src" "$S/hooks" "$S/package/engine" "$S/buggy_rebuild/assets" "$S/licenses"
(cd "$Q/.." && git ls-files quest | while read -r f; do mkdir -p "$S/$(dirname "$f")"; cp "$f" "$S/$f"; done)
cp -r "$Q/mallvr_apk" "$S/quest/"                     # (also the files not committed yet when this runs)
cp "$Q"/*.sh "$Q"/*.js "$Q"/*.ps1 "$S/quest/"
for f in "$P"/dosbox_src/*; do [ -f "$f" ] && [ "$(basename "$f")" != vpix_testbot.h ] && cp "$f" "$S/dosbox_src/" || true; done
cp -r "$P/dosbox_src/su2000" "$S/dosbox_src/"
cp "$T"/src/hardware/vpix*.cpp "$T"/src/hardware/vpix*.h "$T"/src/hardware/openvr.h "$S/android_tree/src/hardware/"
cp "$T"/src/hardware/su2000/* "$S/android_tree/src/hardware/su2000/"
cp "$T/src/dosbox.cpp" "$S/android_tree/src/"; cp "$T/src/gui/sdlmain.cpp" "$S/android_tree/src/gui/"
node "$(w "$Q/strip_dev_only.js")" $(find "$S/dosbox_src" "$S/android_tree" -name "vpix*" -exec cygpath -w {} \;)
cp "$out"/assets/engine/hooks/* "$S/hooks/"
cp "$P/package/engine/games.ps1" "$P/package/engine/play.ps1" "$S/package/engine/"
cp "$P"/buggy_rebuild/assets/ball_logo*_256.rgba "$S/buggy_rebuild/assets/"
cp "$L"/*.txt "$S/licenses/"
cat > "$S/README_SOURCE.txt" <<EOF
MallVR for Meta Quest $VER - complete source (GNU GPL v2 or later, like DOSBox-X)
=================================================================================
MallVR is a modified DOSBox-X that plays the 1990s Virtuality SU2000 / SU3000 arcade VR games. This zip is the source of
MallVR-Quest-$VER.apk: everything we wrote or changed, the build scripts, and the exact versions of the stock parts.

What's here
  dosbox_src/      MallVR's DOSBox-X additions (vpix*.cpp/.h = the Virtuality hardware: PIX graphics card HLE, VR renderer,
                   OpenXR layer vpix_xr.h, network; su2000/ = LeonG-ZA's SU2000 card code, GPL-2.0). The developer-only test
                   player is left out, as in every MallVR release.
  android_tree/    the same files exactly as compiled for the Quest (after quest/sync_android_tree.sh: OpenXR hooks applied by
                   quest/apply_xr_hooks.js, Android path fixes) + the two stock DOSBox-X files with our 7 added lines
                   (src/dosbox.cpp, src/gui/sdlmain.cpp). openvr.h = Valve's OpenVR header (types only on Android, BSD-3).
  quest/           the Android project: mallvr_apk/ (launcher + game activity, Java), mallvr_android/ (ndk-build makefiles, compat/
                   = the small Windows-to-Android layer), dosbox_android/ (DOSBox-X source list + config.h), build scripts.
  hooks/           per-game hook tables (function addresses in each game's EXE), package/engine/games.ps1 + play.ps1 = per-game
                   settings (quest/make_games_json.ps1 turns them into the APK's games.json).
  licenses/        every licence in the APK.

Stock parts (download these versions)
  DOSBox-X     v2026.10.01 (github.com/joncampbell123/dosbox-x, tag dosbox-x-v2026.10.01)
  SDL2         release-2.32.10 (github.com/libsdl-org/SDL)
  gl4es        commit 2037865590d410bcd7b8d1f22b841472ed6d39ac (github.com/ptitSeb/gl4es)
  OpenXR loader  Khronos 1.1.65 for Android (Maven org.khronos.openxr:openxr_loader_for_android:1.1.65)
  Apache Commons Compress 1.25.0, XZ for Java 1.9 (Maven Central)
  Android NDK r29, SDK platform 32, build-tools 34.0.0, JDK 17

Build (Windows + Git Bash, no Gradle; paths in the scripts point at H:\Android)
  bash quest/sync_android_tree.sh        stock DOSBox-X + dosbox_src -> the Android tree
  bash quest/build_mallvr_android.sh     ndk-build -> libmain.so, libSDL2.so, libopenxr_loader.so
  bash quest/build_mallvr_apk.sh         assets + Java + APK (+ this zip)

gainesphere.com/pages/mallvr
EOF
rm -f "$rel/MallVR-Quest-$VER-source.zip" "$rel"/*.idsig
(cd "$out/src" && "$J/jar.exe" cfM "$(w "$rel/MallVR-Quest-$VER-source.zip")" "MallVR-Quest-$VER-source")
cp "$Q/QUEST_INSTALL.txt" "$rel/QUEST_INSTALL.txt"
ls -la "$rel"
