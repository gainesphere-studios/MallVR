# Source file guide

## Engine (repo root, built into `dosbox-x.exe`)

| File | What it does |
|---|---|
| `vpix.cpp` | Entry point (`VPIX_Init`). Claims the SU2000 I/O ports and memory windows, maps the SU3000 PCI board, and arms the HLE on the game's first graphics-card access. |
| `vpix_hle.cpp` | The heart of MallVR. Hook table loader (`load_hooks`), code finder and patcher (`VPIX_HLE_Arm`, `match_prologue`), callback dispatcher (`vpix_hle_cb`), and the replacements for the PIX / TRK / CTRL / SND / CD / MPEG / PCI calls (`hle_replace`, `hle_log`). Builds each frame from the game's tables (`submit_frame`, `submit_frame_su3k`), plays sound (`hle_sound`, `hle_awe`), CD audio (`hle_cd`), FLIC (`hle_flic`) and MPEG video (`mpeg_start`). Also fills empty linked seats with bots (`net_bots`). |
| `vpix_render.h` | The shared types: `VPIX_Frame` (camera, lights, fog, instances, blits), `VPIX_Instance`, `VPIX_Pose` (tracker space: X right, Y forward, Z up, mm), `VPIX_Blit`, `VPIX_Input` (headset and controller state), `VPIX_MeshPoly` (SU3000), and the renderer API. |
| `vpix_render.cpp` | Render thread (`thread_main`): window, OpenGL, SteamVR, per-eye drawing (`draw_scene`, `draw_sprites`, `draw_flic`, `draw_blits`), model / texture parsers (`parse_d`, `parse_bin`, `parse_tex`, `parse_vst`), SteamVR input actions and rumble (`acts_init`, `acts_read`, `haptics_play`), the desktop keyboard / mouse mode (`desktop_input`), per-game settings (`load_settings`, `save_settings`). |
| `vpix_net.cpp`, `vpix_net.h` | The pod network card: NS 8390 registers (`net_read`, `net_write`), shared RAM, UDP transport (`rx_loop`, `transmit`), lockstep wait, microphone capture for voice chat. `VPIX_Net_SetSender` / `VPIX_Net_Receive` are the seam for other transports. |
| `vpix_menu.h` | In-game menu: holding A + B for 1 s tells the launcher (over localhost UDP on `VPIX_MENU_PORT`) to show its panel over the game. |
| `vpix_seat.h` | Remote seat: the second seat of a two-player "Duo" game (Virtuality Boxing) played from another MallVR copy (`VPIX_SEAT_HOST` / `VPIX_SEAT_CLIENT`). |
| `vpix_voice.h` | Voice chat between linked players (`VPIX_VOICE=1`): 16 kHz mono, 20 ms mu-law packets, sent only while you talk. |
| `vpix_buggy.h`, `vpix_buggy_net.h` | Buggy Ball. Its EXE survives nowhere, so this is a **new game loop** that uses the original models, textures, sounds and the game's own rule files, drawn by the MallVR renderer (`VPIX_NATIVE=buggy`). Link play is up to 4 pods, with the master pod running the game. |
| `vpix_splash.h` | Placeholder for a start-up splash (currently off). |
| `su2000/` | Low-level SU2000 card emulation by LeonG-ZA ([github.com/LeonG-ZA/dosbox-x](https://github.com/LeonG-ZA/dosbox-x), branch `su2000`, GPL): PIX 1000 with an MC88110 interpreter, tracker, format card, network chip. Only used in reference mode (`VPIX_REF=1`) to compare pictures against the real card's output. |
| `openvr.h` | Valve OpenVR SDK header (BSD-3). |
| `dosbox-x.patch` | Changes to stock DOSBox-X: the two init calls in `sdlmain.cpp`, the new files in the Visual Studio project, and `config.h` for the 32-bit build. |

The small vpix headers are included **at the end of `vpix_hle.cpp`** so they can use its statics; they are not separate
compile units.

## Game data (`hooks/`)

One set per game, named by the game's code (`PAC`, `DAC`, `ZONE` ...): `<CODE>.hooks`, an optional `<CODE>.game` and an
optional `<CODE>.patch`. See [Adding a game](adding-a-game.md) for the format.

## Launcher (`launcher/`, shipped as `engine\`)

See [Launcher, online and updates](launcher.md). In short: `launcher.ps1` (menu loop), `play.ps1` (starts one game),
`games.ps1` (game table), `check.ps1`, `update.ps1` / `findupdate.ps1`, and the C# windows `MallLauncher.cs`,
`MallVrPanel.cs`, `MallOnline.cs`, `MallNotes.cs`, `MallGetGames.cs`, `MallSplash.cs`. `openvr_api.cs` is Valve's C#
binding. `mallvr_*.json` are the SteamVR input actions and default bindings.

## Quest (`quest/`)

See [Meta Quest port](quest.md).
