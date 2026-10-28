MallVR for Meta Quest 0.2 - complete source (GNU GPL v2 or later, like DOSBox-X)
=================================================================================
MallVR is a modified DOSBox-X that plays the 1990s Virtuality SU2000 / SU3000 arcade VR games. This zip is the source of
MallVR-Quest-0.2.apk: everything we wrote or changed, the build scripts, and the exact versions of the stock parts.

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
