# MallVR developer docs

MallVR plays the 1990s **Virtuality** arcade VR pod games (SU2000 and SU3000) on a modern PC, in a SteamVR headset or a
window, and standalone on Meta Quest. It is a modified [DOSBox-X](https://github.com/joncampbell123/dosbox-x). The original
game EXEs run unchanged. The custom pod hardware they expect is replaced by **high-level emulation (HLE)**.

These pages are for people who want to build MallVR, fix it, port it or add a game. Players want the
[download page](https://gainesphere.com/pages/mallvr-download) instead.

| If you want to... | Read |
|---|---|
| understand how a 1990s pod game ends up in a modern headset | [How it works](architecture.md) |
| find which file does what | [Source file guide](files.md) |
| build `dosbox-x.exe` yourself | [Building (PC)](building.md) |
| get another Virtuality game running | [Adding a game](adding-a-game.md) |
| work on the menu, online play, updates or play notes | [Launcher, online and updates](launcher.md) |
| work on the Quest app | [Meta Quest port](quest.md) |
| send a fix | [Contributing](contributing.md) |

## Repository layout

```
vpix*.cpp / vpix*.h   the engine: Virtuality hardware emulation, renderer, network, voice (added to DOSBox-X)
su2000/               low-level SU2000 card emulation (LeonG-ZA), used only in reference mode (VPIX_REF)
dosbox-x.patch        the few changes to stock DOSBox-X files
openvr.h              Valve OpenVR SDK header (BSD)
hooks/                one hook table per game: where each library function sits in that game's EXE
launcher/             the PowerShell + C# front end that ships in engine\
quest/                MallVR Quest: the Android / OpenXR build of the same engine
docs/                 player docs (setup, controls, LaunchBox, Steam); docs/dev holds these pages
```

**MallVR ships no game files.** You need your own copy of the SU2000 / SU3000 software.

Licence: GNU GPL v2 or later, like DOSBox-X.
