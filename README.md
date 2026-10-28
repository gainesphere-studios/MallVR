# MallVR

**Play the 1990s Virtuality arcade VR pod games on a modern PC: in a SteamVR headset with true stereo 3D and head tracking, or in a window with keyboard and mouse.**

MallVR is a modified [DOSBox-X](https://github.com/joncampbell123/dosbox-x). The original games run unchanged; the custom pod hardware they expect (PIX graphics cards, InsideTrak head and hand tracker, Format C controls and sound) is replaced by high-level emulation. The games' library calls are trapped and drawn with OpenGL, and sent to SteamVR.

- **Download, setup and the game list:** [gainesphere.com/pages/mallvr-download](https://gainesphere.com/pages/mallvr-download)
- **Community, help and multiplayer:** [MallVR Discord](https://discord.gg/7tDn2AAXEC)
- **Support the work:** [Ko-fi](https://ko-fi.com/mallvr)

## What works

- 14 games: Dactyl Nightmare, Dactyl Nightmare 2, Pac-Man VR, Zone Hunter, Ghost Train, Virtuality Boxing, Missile Command VR, Olin Shotgun, Shoot For Loot, Sea Diving (Pearl), Buggy Ball, plus the SU3000 games Total Recoil, Zero Hour and Zeneca Migraine
- SteamVR headsets (developed on an HTC Vive), or a desktop window; a 32-bit build for older Windows
- Meta Quest, standalone (no PC needed): see [`quest/`](quest/)
- Linked pods: the original pod network card is emulated, so linked games run between copies on one PC, on a home network, or online (QUICK MATCH)
- Controller rumble, crouch, arm reach and aim trims, self-update

**MallVR ships no game files.** Bring your own copy of the SU2000 software.

## For developers

**Developer docs: [gainesphere-studios.github.io/MallVR](https://gainesphere-studios.github.io/MallVR/)** (source in [`docs/dev`](docs/dev)): how the emulation works, what each file does, building, adding a game, the launcher and online play, and the Quest port.

## Meta Quest

[`quest/`](quest/) holds the source of **MallVR Quest**, the first DOSBox-X running on Meta Quest: the same engine built for Android with OpenXR, a headset launcher, play notes and REPORT A PROBLEM. Build steps are in `quest/README_SOURCE.txt`. The APK and install guide are on the [download page](https://gainesphere.com/pages/mallvr-download#quest).

## Play online

The pods were cabled together in the arcade; MallVR links them over the internet the same way: one arena, one shared scoreboard. Open `MallVR_Launcher.bat`, pick a linkable game, click **ONLINE**, then **QUICK MATCH**. It pairs you with another player and starts the match by itself. No server to set up, no IP addresses to type.

Step-by-step instructions: [gainesphere.com/pages/mallvr-download](https://gainesphere.com/pages/mallvr-download) (the "Play online" section). Need someone to play? Find a match on the [Discord](https://discord.gg/7tDn2AAXEC) and use its voice channels while you play.

## Join the MallRats

MallVR is built with its players. Here is how to get involved:

- **Play and talk:** the [Discord](https://discord.gg/7tDn2AAXEC) is where players link up for multiplayer, share gameplay clips, get setup help and dig through Virtuality history.
- **Report bugs:** open an [issue](../../issues) with your game, headset (or window mode) and what happened. Logs are in MallVR's `logs` folder.
- **Share what you know:** if you worked on, ran or owned Virtuality pods, we want your stories, photos, manuals and settings. Every detail helps the emulation get closer to the real thing.
- **Build with us:** code, fixes and new tools are welcome as pull requests. New games and experiences for this platform, even walkable 3D movies, are on the roadmap.
- **Back the work:** [Citizens](https://ko-fi.com/mallvr/tiers) ($7 a month) get early test builds of new games and features, a vote on what we build next, the Citizen channel on the Discord and priority support. Every finished game stays free for everyone. Tips on [Ko-fi](https://ko-fi.com/mallvr) fund what's next: a Quest test headset first, then two-player Pac-Man online and our own original games.

More from Gainesphere Studios, including 3D files and other projects, will be published here on GitHub.

## This repository

The GPL source of what ships in every MallVR download (the `source` folder):

| File | What it is |
|---|---|
| `vpix.cpp` | port and memory probe for the PIX and Format cards, entry point |
| `vpix_hle.cpp` | hook table loader, PIX / tracker / sound / CD call emulation, scene capture |
| `vpix_render.cpp`, `vpix_render.h` | OpenGL renderer, SteamVR output, controller and keyboard input |
| `vpix_net.cpp`, `vpix_net.h` | emulated pod network card (SMC/WD8003 class, NS 8390) over UDP |
| `vpix_splash.h` | loading splash |
| `openvr.h` | Valve OpenVR SDK header (BSD licence) |
| `dosbox-x.patch` | changes to upstream DOSBox-X files |
| `launcher/` | the PowerShell and C# launcher, online play, updater and game table |
| `docs/` | the player docs that ship in the download |

Build steps are in [BUILDING.txt](BUILDING.txt).

## Licence

GNU General Public License v2 (see [LICENSE](LICENSE)), the same as DOSBox-X. `openvr.h` is under Valve's BSD licence.

MallVR is independent software from Gainesphere Studios. It is not affiliated with or endorsed by the companies that made or own the original games.
