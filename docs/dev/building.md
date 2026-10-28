# Building (PC)

MallVR's PC engine is DOSBox-X with MallVR's files added. You need Windows and the **Visual Studio 2022 Build Tools**
(MSVC, x64).

## Steps

1. Get DOSBox-X at the exact commit MallVR is built against (`BUILDING.txt` in the repo root names it):

    ```
    git clone https://github.com/joncampbell123/dosbox-x.git
    git -C dosbox-x checkout <commit from BUILDING.txt>
    ```

2. Apply MallVR's changes to the stock files:

    ```
    git -C dosbox-x apply dosbox-x.patch
    ```

3. Copy the engine files from this repo:
    - `vpix*.cpp`, `vpix*.h` and `openvr.h` → `dosbox-x\src\hardware\`
    - the `su2000\` folder → `dosbox-x\src\hardware\su2000\`

4. Get the [OpenVR SDK](https://github.com/ValveSoftware/openvr). Fix the `openvr_api.lib` path at the top of
   `vpix_render.cpp`, and put `openvr_api.dll` next to the built exe.

5. Build:

    ```
    msbuild vs\dosbox-x.vcxproj /p:Configuration=Release /p:Platform=x64
    ```

    For the 32-bit build (older Windows), use `/p:Platform=Win32`. The patch's `config.h` change switches DOSBox-X to its
    portable FPU code for that build.

## Running your build

The engine expects the launcher's environment. The easiest way to test is to drop your `dosbox-x.exe` into a MallVR
release's `engine\` folder and start a game the normal way (`MallVR_Launcher.bat` or `Play_<Game>.bat`).

To run it by hand, set at least `VPIX_LOG` (a log path; without it MallVR stays switched off) and
`VPIX_HOOKS=<engine>\hooks\<CODE>.hooks`, then start `dosbox-x.exe -conf <conf>` with a conf that mounts the game.
`play.ps1` shows exactly what it writes and sets. Read it before writing your own.

`VPIX_VR=0` runs in a desktop window without a headset.

## Logs

With `VPIX_LOG=logs\<game>`, look in:

| File | What's in it |
|---|---|
| `<game>.calls` | every hooked library call (capped in packaged mode) |
| `<game>.render` | renderer start-up, SteamVR, frame timing (`perf:` lines), errors |
| `<game>.net` | network card and link traffic |

These are the first things to attach to a bug report.
