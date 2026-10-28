# Meta Quest port

`quest/` is **MallVR Quest**: the same engine built for Android with OpenXR, running standalone on Quest 2, Pro, 3 and 3S.
`quest/README_SOURCE.txt` lists the exact versions of every stock part.

## What's in `quest/`

| Folder | Contents |
|---|---|
| `dosbox_src/` | The engine files as used for the Quest build, plus `vpix_xr.h` (OpenXR) and `su2000/` |
| `android_tree/` | The same files exactly as compiled, after the sync step (below) |
| `quest/` | The Android project: `mallvr_apk/` (Java launcher and game activity), `mallvr_android/` (ndk-build makefiles, `compat/` layer), `dosbox_android/` (DOSBox-X source list and `config.h`), build scripts |
| `hooks/`, `package/engine/` | The same hook tables and `games.ps1` / `play.ps1` as the PC version. `make_games_json.ps1` turns them into the APK's `games.json`. |
| `licenses/` | Every licence in the APK |

## Toolchain

You need:

- Windows with Git Bash, Node.js and PowerShell. Gradle is not used.
- Android NDK r29, SDK platform 32, build-tools 34.0.0, JDK 17
- DOSBox-X tag `dosbox-x-v2026.10.01`, SDL2 `release-2.32.10`, gl4es (the commit in `README_SOURCE.txt`)
- Khronos OpenXR loader 1.1.65 for Android, Apache Commons Compress 1.25.0, XZ for Java 1.9

The ABI is arm64-v8a, `android-29` minimum.

!!! warning "Hard-coded paths"
    The scripts expect everything under `H:\Android\...` (and `openvr.h` from `H:\Games\Virtuality\_work\...`). Change
    those paths to your own before building. The native build runs from `H:\Android\build`, because `make` can't handle
    an apostrophe in a path.

## Build steps

```
bash quest/sync_android_tree.sh      # stock DOSBox-X + dosbox_src -> the Android tree
bash quest/build_mallvr_android.sh   # ndk-build -> libmain.so, libSDL2.so, libopenxr_loader.so
bash quest/build_mallvr_apk.sh       # assets + Java + APK (+ the GPL source zip)
```

- **`sync_android_tree.sh`** copies the stock DOSBox-X tree and adds MallVR's 7 lines to `dosbox.cpp` and `sdlmain.cpp`
  (each anchor must match exactly once, or it stops). It copies the vpix files and `su2000/`, turns Windows `\` path
  joins into `/`, and runs `apply_xr_hooks.js`. It is safe to run again.
- **`apply_xr_hooks.js`** writes an OpenXR-hooked copy of `vpix_render.cpp`. The master file is never edited, so the PC
  and Quest builds share one renderer source.
- **`build_mallvr_android.sh`** runs `ndk-build`. `vpix_render.cpp` is compiled on its own with the Windows compat layer
  (below). Everything else goes into `libmain.so`.
- **`build_mallvr_apk.sh`** does the rest:
    - Refuses to package a build that contains the developer test player.
    - Builds `games.json`, copies the hook tables, compiles the Java with `javac` + `d8`, then packages with `aapt`,
      `zipalign` and `apksigner`.
    - **Keep your signing key.** An APK signed with a different key won't install over the old one; players have to
      uninstall first.

## How the app is put together

| Part | Job |
|---|---|
| `LauncherActivity` | The 2D panel you see first. It lists the games (green = READY, amber = different version or needs the CD, grey = missing). Buttons: ONLINE, GET GAMES, REPORT, ABOUT, CHECK AGAIN, PLAY NOTES. |
| `MallVRActivity` | The game. An `SDLActivity` in its own process (`:game`) that runs DOSBox-X with `-conf <conf> -nopromptfolder` and the environment from the launcher. When it ends it returns to the launcher and kills its own process, because SDL can't start twice in one process. |
| `MallVR.java` | Shared helpers: folders, `games.json`, finding a game (EXE, SHA-1, CD), and `plan()`, which writes the conf and the environment (the Java version of `play.ps1`). |
| `GetGamesActivity`, `GameFiles` | GET GAMES: the same four archive.org items as the PC version. Downloads resume after a break, are SHA-1 checked, and 7z unpacks in Java. |
| `ReportActivity` | REPORT A PROBLEM: a zip of the report, logcat and logs, saved to `/sdcard/Download`. SEND uploads it. |
| `PlayNotes` | Opt-in play notes: the same protocol as the PC, plus OpenXR status and fps. |
| `OnlineActivity`, `OnlineService`, `MallOnline.java` | Online play. `MallOnline.java` is a packet-for-packet Java port of the PC's `MallOnline.cs`, so PC and Quest players share a lobby and a match. `OnlineService` keeps it alive (foreground service with Wi-Fi locks). |
| `AboutActivity` | The GPL source offer and licences. |

**On the headset,** files live in `/sdcard/MallVR/`, in the same layout as the PC version: `games/`, `engine/` (hook
tables, copied from the APK), `logs/`, `settings/`, `downloads/`.

**Talking to the launcher:**

- The game process writes `start` / `end` lines to `logs/quest_game_state.txt`. When the launcher comes back, it turns
  them into play-notes records. A `start` with no `end` while the game process has died counts as a crash.
- The game also opens a localhost UDP port (`VPIX_MENU_PORT`). Holding **A+B** for 1 s makes the engine send `MENU`. The
  activity answers `QUIT` and brings the launcher forward.

## OpenXR

`vpix_xr.h` is the OpenXR layer. `apply_xr_hooks.js` wires it into the renderer at start-up, per frame and at shutdown.
If OpenXR fails to start, the renderer falls back to OpenVR.

- The loader is `libopenxr_loader.so`, with the GLES and Android-instance extensions, bound to the EGL context.
- It uses the **stage** reference space, falling back to local.
- The eye size is the runtime's recommendation, times `VPIX_XR_SCALE` if set.
- There is one sRGB swapchain per eye (`VPIX_XR_LINEAR=1` switches to linear). Each frame the game's own eye image is
  copied in and sent as one projection layer.
- There are input bindings for Touch, Index, Vive and simple controllers. The hand pose is `grip`; `VPIX_XR_POSE=aim`
  switches to the aim pose.

## The compat layer (`quest/mallvr_android/compat/`)

`vpix_render.cpp` is Windows code. It is compiled for Android **with `_WIN32` defined**, so it runs unchanged, against
small stand-ins:

- **Windows calls:** the window and message functions do nothing. Timing uses `clock_gettime`. `LoadLibrary` maps to
  `dlopen`. Process and pipe creation fail, so there is no ffmpeg and no SU3000 movies on Quest.
- **OpenGL:** `vpix_android.cpp` makes an EGL GLES3 context and starts gl4es, which runs the desktop OpenGL calls on GLES.
- **Network:** the Winsock names map onto BSD sockets.
- **Microphone:** stubbed in the compat layer.

`win32_prelude.h` includes the C++ library headers before `_WIN32` is defined, so libc++ doesn't pick its Windows code.
