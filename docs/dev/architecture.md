# How it works

A Virtuality SU2000 pod was a 486 PC running DOS with custom expansion cards:

- **PIX** graphics cards that drew the 3D scene for each eye
- an **InsideTrak** tracker for the head and the hand
- a **format / control card** for the buttons, joystick and coin input
- sound hardware
- an **Ethernet card** for linking pods

The SU3000 is the later Pentium version with a PCI graphics board.

The games talk to that hardware through Virtuality's own libraries (`PIX_*`, `TRK_*`, `CTRL_*`, `SND_*`, `CD_*`, `NET_*`),
linked into each game EXE. MallVR does **not** emulate the graphics card at the register level. It catches the game's calls
into those libraries and does the work itself on modern hardware. This is high-level emulation (HLE).

```
 game EXE (unchanged, running in DOSBox-X)
   │  calls PIX_LoadModel, PIX_RenderSwap, TRK_GetTrackerRecord, CTRL_GetButton, SND_..., CD_...
   ▼
 patched entry points ──► vpix_hle.cpp (DOSBox callback)
   │                          ├─ models / textures / bitmaps ─────────► vpix_render.cpp (load)
   │                          ├─ end of frame: read the PIX tables ───► VPIX_Frame ─► render thread
   │                          ├─ tracker / buttons ◄── VPIX_Input ◄─── headset + controllers
   │                          └─ sound, CD audio, FLIC/MPEG video
   ▼
 render thread (vpix_render.cpp): OpenGL, one image per eye ──► SteamVR (OpenVR) or a desktop window
                                                              (OpenXR on Quest)
```

## 1. Start-up

`dosbox-x.patch` adds two calls to DOSBox-X's `main()` in `src/gui/sdlmain.cpp`, right after `DISNEY_Init()`:
`VPIX_Init()` and `SU2000_Init()`.

`VPIX_Init` (`vpix.cpp`) does nothing unless the launcher set `VPIX_LOG`, so a stock config still behaves like plain
DOSBox-X. When it is set, `VPIX_Init`:

- starts the network card emulation first (`VPIX_NET_Init`), then the HLE (`VPIX_HLE_Init`, which also starts the render
  thread)
- claims the SU2000 I/O ports: 0x300 / 0x360 (PIX), 0x340, 0x320, 0x270 (tracker), 0x210 (format card)
- maps the PIX memory window at 0xD0000 and the format-card window at 0xE0000
- for SU3000 games (`VPIX_PIX3K`), maps the PCI board's memory at 0xC0000000 and 0xC4000000

With `VPIX_REF=1` it leaves the ports alone so the low-level `su2000/` emulation can run instead. That is a research mode
for comparing pictures, not how players run MallVR.

## 2. Finding and patching the game's library calls

The game has to be loaded and running before its code can be patched, so the hooks are "armed" late:

- by default on the game's first access to ports 0x300-0x36F (it is starting its graphics card)
- with `VPIX_ARM_POLL=1`, by a 50 ms timer instead

`VPIX_HLE_Arm` (`vpix_hle.cpp`) then:

1. Loads the game's hook table (`VPIX_HOOKS` = `hooks/<CODE>.hooks`), then the optional `<CODE>.game` file next to it.
2. **Finds where DOS loaded the code.** It scans guest RAM for the 16-byte prologue of the table's `anchor` function. The
   code offset is the address it was found at minus the static address in the table.
3. Works out the data offset from one or two `dataref` lines. These are code locations that hold the absolute address of a
   known table.
4. Applies the `.patch` pokes, each written only if the original bytes match.
5. **Patches each hooked function.** It checks the function's prologue, then overwrites its first 5 bytes with
   `FE 38 <callback number> 90`. `FE 38` is DOSBox's callback opcode, so every call into that function now jumps into
   MallVR's C++ code.

That is the whole trapping mechanism: binary patches at function entry points. There are no breakpoints, no interrupt
hooks and no address watching.

## 3. Handling a call

`vpix_hle_cb` looks up which hook was hit, and acts on the hook's mode:

| Mode | Meaning |
|---|---|
| `R` (replace) | Run MallVR's version (`hle_replace`), put the result in EAX, return to the game. Arguments arrive in registers (Watcom convention: EAX, EDX, EBX, ECX), or on the stack for functions marked `cdecl`. A replaced function MallVR has no special code for just returns the table's `retval`. |
| `L` (log) | Record the call (and any side effects in `hle_log`), replay the 5 overwritten bytes, and let the game's own code continue. |
| `O` | Reference mode: MallVR's side effects, then the native code. |
| `F` | Float stub: puts the host clock (or a sample length) where the game expects an x87 float. |

What happens per library:

- **PIX loads** (`PIX_LoadModel`, `PIX_LoadTexture`, `PIX_LoadBitmap`): MallVR reads the file through DOS and hands it to the
  renderer.
- **PIX frame** (`PIX_RenderSwap` / `PIX_Render`): this is the end of a game frame. MallVR applies the frame limit, then
  `submit_frame()` builds the scene. It does **not** decode graphics-card commands. Instead it reads the PIX library's own
  tables in guest memory (the `region` lines in the hook table: view, model, light, window and so on). From those it picks
  the active camera and builds a `VPIX_Frame`: camera, lights, fog, model instances and 2D blits. SU3000 games use
  `submit_frame_su3k`, which builds meshes and textures from the PowerPC board memory.
- **Tracker** (`TRK_*`): returns 6-float records (x, y, z in mm; azimuth, elevation, roll) made from the live headset and
  controller poses.
- **Controls** (`CTRL_*`): writes buttons, joystick axes and the start / coin input into guest memory.
- **Sound** (`SND_*`, AWE): loads the WAVs from the game's files and mixes them into a DOSBox mixer channel.
- **CD audio** (`CD_*`): plays the soundtrack from `VPIX_CD_DIR`, or leaves the calls native so a mounted CD image plays
  through MSCDEX (`VPIX_CD_NATIVE=1`).
- **MPEG / PCI (SU3000):** MPEG playback is timed by file size, with optional ffmpeg video; the PCI calls report a fake
  PIX2000E board.
- **Network** (`NET_*`): not hooked when linking is on. The Ethernet card is emulated well enough for the game's own network
  code to run (section 5).

## 4. Drawing and input

The render thread (`thread_main` in `vpix_render.cpp`) owns a window with an OpenGL context. If a headset is present and
`VPIX_VR` isn't 0, it starts SteamVR (`vr::VR_Init`), loads the input actions (`mallvr_actions.json`) and makes one
framebuffer per eye. Each loop it:

1. gets the head and hand poses (`WaitGetPoses`) and reads the controller actions
2. for each eye, combines the game's camera with the live eye pose (`eye_camera`), then draws: `draw_scene`, `draw_sprites`,
   `draw_static`, `draw_flic`, `draw_blits`
3. submits both eyes to the SteamVR compositor and mirrors the left eye to the window

Without a headset it draws one view to the window and takes keyboard and mouse input (`desktop_input`).

Input flows back the other way. The render thread fills a `VPIX_Input` (head and hand poses, buttons, coins, start,
locomotion, aim trims). On the game's first tracker call each frame, the HLE thread reads it with `VPIX_Input_Get` and
turns it into tracker records and joystick data. Rumble goes out through a queue (`VPIX_Haptic`), which the render thread
plays on the controllers.

Model files are parsed by `parse_d`, `parse_bin`, `parse_tex` and `parse_vst`. Per-game aim trims and settings are saved to
the `VPIX_SETTINGS` file.

## 5. Linked pods and online play

`vpix_net.cpp` emulates an SMC / WD 8003-class Ethernet card: an NS 8390 chip at I/O 0x280 with 8 KB of shared RAM at
0xC8000. Pod N gets the MAC address `00:00:C0:56:50:N`. Each game's own `NET_` library drives it unchanged.

- **Transport:** whole Ethernet frames over UDP. Pod N binds port 27100+N. The peers come from `VPIX_NET_PEERS` and default
  to the other local pods. Frames are only accepted from known peers.
- **Lockstep:** when the game polls for a frame that hasn't arrived, the card waits up to `VPIX_NET_WAIT_MS` (default 60 ms)
  of real time for it.
- **Bots:** in 4-player Dactyl Nightmare links, `net_bots` fills empty seats with computer players.
- **Voice chat** travels as its own EtherType (0x88B6, "MVOI", mu-law) and never reaches the game's card.
- **Online:** the engine always thinks it is on a local pod network. The launcher's online code relays its frames between
  players' PCs. See [Launcher, online and updates](launcher.md#online-play-mallonlinecs).

## 6. Configuration

The engine is configured entirely by environment variables that the launcher sets per game. There are over 100. The
important ones:

| Variable | Meaning |
|---|---|
| `VPIX_LOG` | Master switch, and the base path for the logs (`.calls`, `.render`, `.net` ...) |
| `VPIX_PLAY` | Packaged mode: no debug dumps, capped call log |
| `VPIX_HOOKS` | Path to the game's hook table |
| `VPIX_VR` | `0` = desktop window instead of SteamVR |
| `VPIX_FPS` | Frame limit |
| `VPIX_SETTINGS` | Per-game settings / aim trim file |
| `VPIX_PIX3K`, `VPIX_MMC`, `VPIX_ARM_POLL` | SU3000 hardware |
| `VPIX_NET`, `VPIX_NET_PEERS`, `VPIX_NET_WAIT_MS` | Pod number, peers, lockstep wait |
| `VPIX_VOICE` | Voice chat |
| `VPIX_CD_DIR`, `VPIX_CD_NATIVE`, `VPIX_CDSIG` | CD audio |
| `VPIX_PATCH_EXTRA` | One extra `.patch` file |
| `VPIX_NATIVE=buggy` | Run the native Buggy Ball rebuild instead of a DOS game |
| `VPIX_REF` | Reference mode with the low-level `su2000/` emulation |

Everything else is a per-game behaviour flag (`VPIX_AIM_HEAD`, `VPIX_CROUCH`, `VPIX_DEPTH_FIX`, `VPIX_HAPTIC` ...),
documented where the code reads it. To list them all:

```
grep -o 'getenv("[A-Z0-9_]*")' vpix*.cpp vpix*.h | sort -u
```
