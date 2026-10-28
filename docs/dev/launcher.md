# Launcher, online and updates

Players never run `dosbox-x.exe` directly. Everything goes through PowerShell scripts in `launcher/` (shipped as
`engine\`), with C# for the windows.

## What starts what

```
MallVR_Launcher.bat ─► launcher.ps1 ─(compiles the C#)─► menu (MallLauncher.cs)
                                     └─ START ─► play.ps1 <CODE> [-Pod n] [-Peers host:port,...] [-Desktop]
                                                   └─ writes logs\<tag>.conf, sets VPIX_*, runs dosbox-x.exe, waits
Play_<Game>.bat ───────────────────────────────► play.ps1 <CODE>          (one game, no menu)
Check_My_Games.bat ─► check.ps1                  Update.bat ─► update.ps1
```

| File | Job |
|---|---|
| `launcher.ps1` | Compiles the C#, shows the menu, runs `play.ps1`, comes back to the menu when the game exits. Exit code 2 = could not start; any other non-zero exit counts as a crash. |
| `play.ps1` | Starts one game: finds the EXE, writes the DOSBox-X conf, sets the engine's environment, shows the update splash, runs `dosbox-x.exe -conf <conf> -nopromptfolder`. Also takes an EXE path instead of a code, for LaunchBox. |
| `games.ps1` | The game table. See [Adding a game](adding-a-game.md#2-the-gamesps1-entry). |
| `check.ps1` | Check My Games: READY / READY? / MISSING / NEEDS CD, from SHA-1s of the EXE and CD image. |
| `update.ps1`, `findupdate.ps1` | Self-update (below). |
| `MallLauncher.cs` | The menu (WinForms, drawn at 352x288 like a pod operator screen) and the play-notes consent screen. |
| `MallVrPanel.cs` | The same menu as a SteamVR overlay you click with the laser. In a game, holding **A+B** for 1 s brings it up (the engine asks over a localhost UDP port, `VPIX_MENU_PORT`). |
| `MallOnline.cs` | Online lobby and relay. |
| `MallNotes.cs` | Opt-in play notes and REPORT A PROBLEM. |
| `MallGetGames.cs` | GET GAMES downloader. |
| `MallSplash.cs` | The see-through splash shown while `play.ps1` checks for updates. |
| `mallvr_*.json` | SteamVR input actions and default bindings (Index, Vive, Touch). |

**C# without installing a compiler:** `launcher.ps1` builds the C# files with PowerShell's `Add-Type -OutputAssembly` into
`logs\MallLauncher_<hash>.dll`. The hash comes from the files' SHA-1s, so editing any `.cs` file rebuilds it on the next
start.

## How `play.ps1` sets up a game

- Finds `<EXE>\<EXE>.EXE` under `games\`. SU2000 and SU3000 copies with the same EXE name are told apart by SHA-1.
- Writes `logs\<tag>.conf`: `machine=svga_s3` and OpenGL output. SU2000 gets a 486 with 32 MB at `cycles=max`. SU3000 gets
  a Pentium with 64 MB and a Sound Blaster 16. The autoexec mounts the game as C:, then an optional per-pod overlay, then
  the CD image as F:, and runs the game with its operator command line from `games.ps1`.
- Sets the engine's environment:
    - always: `VPIX_PLAY=1`, `VPIX_LOG`, `VPIX_HOOKS=engine\hooks\<CODE>.hooks`, `VPIX_SETTINGS`, `VPIX_CDSIG`, `VPIX_FPS`
    - one `VPIX_*` for each behaviour flag in the game's `games.ps1` entry
    - `-Desktop` sets `VPIX_VR=0`
- **Linked pods:** each pod gets a `settings\pod<N>` overlay on C:. `play.ps1` patches the game's own master / slave
  setting file there (`DEFAULTS.DN2`, `LASTSETS.DBS`, `SETTINGS.PRL`, `CONFIG.BIN`). It also sets `VPIX_NET=<pod>`, and
  `VPIX_NET_PEERS` lists the other pods (port 27100+pod).

## Online play (`MallOnline.cs`)

There is **no game server**. The host player's PC relays the match. Our server only keeps the lobby list.

- **Lobby:** `https://downloads.gainesphere.com/mallvr/online/list.php`. `server =` in `settings\launcher.txt` overrides
  it. Every 3 s each launcher sends a heartbeat (`op=beat`: name, game, players, version, platform ...).
- **Host addresses are never listed.** A host's row carries a token. A joiner asks for that one host's address
  (`call=<token>`), and the host is told about the caller, so both sides can punch through NAT.
- **LAN:** hosts also broadcast on UDP 27209.
- **HOST:** binds UDP 27200 (`MALLVR_HOST_PORT`) and learns its public port from STUN (Google, Cloudflare). It tries UPnP;
  `MALLVR_NO_UPNP=1` skips it. START hands out pod numbers: the host is pod 1, the others 2, 3 ...
- **JOIN:** sends `J name game netproto version id exe-sha1 platform`. The host refuses a different protocol version
  (NETPROTO, currently 2) or a different game EXE.
- **QUICK MATCH:** joins the fullest open host for the chosen game, or else hosts. Two lone hosts merge: the one whose
  name sorts later joins the other. A full game counts down and starts.
- **Bridge to the engine:** the engine believes it is on a local pod network. It sends its frames to
  `127.0.0.1:27110+pod`. The launcher wraps each one as `D <pod> <frame>` and sends it to the host, which forwards it to
  the others. Frames for this PC go to the engine on `127.0.0.1:27100+pod`. `MALLVR_LAG_MS` adds fake lag for testing.
- The Quest app has a Java port of this protocol, packet for packet, so PC and Quest players can share a match.

## Self-update (`findupdate.ps1`, `update.ps1`)

- **Sources** come from `engine\version.txt` and are tried in this order:
    1. `url`: our https pointer file
    2. `drive`: a Google Drive pointer
    3. `repo`: this repo's latest GitHub release (the `*_Loader.zip` asset plus its `.sig`)

    If all three are `none`, that copy never updates itself (the with-games editions).
- **Every release is signed.** The signed text is `MALLVR-RELEASE 1|<version>|<SHA-256 of the zip>`, using RSA with
  SHA-256. `engine\release_key.txt` is the **public** key, in .NET RSA XML. The private key never leaves the release PC.
  An unsigned or badly signed zip is refused.
- **The update flow:**
    - `play.ps1` checks for updates with a 3 s timeout, and asks the player once per version.
    - `update.ps1` refuses to run while a game is running.
    - It downloads to `%TEMP%`, verifies the signature, and checks that the version inside the zip matches the signed one.
    - It replaces `engine\`, `docs\`, `source\` and the root files. It never touches `games\`, `settings\` or `logs\`.

## Play notes and REPORT A PROBLEM (`MallNotes.cs`)

- **Off until the player says yes** on the consent screen. The choice and a random stats ID (not the player's name or
  key) are kept in `%APPDATA%\MallVR\play_notes.txt`. Unsent records wait in `notes_queue.txt` (500 lines max).
- **Records:**
    - `launcher_start`: version, Windows version, GPU, headset / runtime, mode, games owned
    - `game_end`: game, solo / linked / online, seconds played, how it ended, fps and worst frame time
    - `crash`: exit code plus the last 200 lines of the render log
    - `net` and `online`
- The Windows user name is scrubbed from paths.
- Records go to `.../online/stats.php` as `MALLVR-NOTES 1` text, after each game and on exit.
- **REPORT A PROBLEM** zips the player's text, the last game's logs and the settings (with `key` lines removed). It posts
  the zip to `.../online/report.php`. If that fails, the zip goes on the Desktop.

## GET GAMES (`MallGetGames.cs`)

- **Downloads come only from `https://archive.org/download/<item>/<file>`.** They resume after a break (a `.part` file
  plus HTTP Range) and are checked against the SHA-1 in the list before unpacking. 7z unpacks with Windows `tar.exe`, zip
  with .NET.
- **The list** is `engine\getgames.txt`. A newer list from our server is used **only if its signature checks out**
  (`MALLVR-GAMES 1|<SHA-256 of the list>`, same public key).
- **Line format:** `id|title|what it gives|archive.org item|file|size|SHA-1|unpack`. The unpack field is one of `7z`,
  `7z-keep:<part>`, `zip`, `zip-cd` or `zip-cd-inner:<zip>`.
