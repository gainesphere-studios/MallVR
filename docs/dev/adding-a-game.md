# Adding a game

MallVR runs a Virtuality game once it knows **where that game's library functions are** in its EXE. That knowledge lives
in a hook table. Adding a game means:

1. making `hooks/<CODE>.hooks` (plus an optional `.game` and `.patch`)
2. adding an entry to `launcher/games.ps1`
3. only if needed: a new behaviour flag or a link-setting patch in `launcher/play.ps1`

## 1. The hook table

Plain text, one keyword per line. `#` starts a comment. Addresses are hex **static** (link-time) addresses from the EXE.
MallVR works out where DOS actually loaded the code.

| Line | Meaning |
|---|---|
| `anchor <name> <addr> <16 bytes hex>` | **Required.** A function whose first 16 bytes are unique in the program. MallVR finds the loaded code by scanning RAM for these bytes. |
| `dataref <codeaddr> <target>` | Up to 2. A place in the code that holds the absolute address of a known table; gives the data offset. |
| `region <name> <addr> <hexsize>` | The PIX library's tables in the game's data: `view`, `model`, `light`, `window`, `windowSwitch`, `windowSwitchUpdate`, `texture`, `material` (SU3000: `iview`, `imodel`). Frames are built from these. |
| `global <name> <addr>` | A game variable MallVR reads or writes (`trk_config`, `pixConfig`, ...). |
| `globalref <name> <codeaddr>` | SU3000: an imm32 in the code that holds a global's address. |
| `watch`, `code`, `dataobj`, `modelstride`, `personstride`, `personpos` | Extra locations and record sizes used by some games. |
| `cdecl <addr>` | Marks a hook as stack-argument (caller pops). |
| `hook <Name> <R\|L\|F\|O> <addr> <16 bytes hex> <retpop> <retval>` | One library function to trap. Mode: see [How it works](architecture.md#3-handling-a-call). `retpop` = bytes of stack arguments to pop, `retval` = what to return if MallVR has no special code for it. |

A real example, from `PAC.hooks` (Pac-Man VR):

```
anchor PIX_InitChannel 00036CE0 6818000000e89e5affff535156575589
dataref 0002D548 0013DD3C
region view 00168D50 880
region model 0013DD3C 15800
global trk_config 000EC070
hook PIX_LoadModel R 0002D07C 685c090000e802f7ffff535152565755 0 0
hook PIX_RenderSwap R 0003EB4C 6808000000e832dcfeff52e8e88a0000 0 0
```

Lines like `# missing PIX_DrawStatic` mean that function isn't in this game.

**`<CODE>.game`** has the same syntax. It holds hand-written hooks on the game's *own* code, not the Virtuality libraries.
For example, `BOX.game` hooks Virtuality Boxing's lean-to-move routine:

```
hook BOX_Lean L 00032F03 6894000000e8572b0200535152565755 0 0
```

**`<CODE>.patch`** holds byte pokes: `poke <addr> <expected bytes> <new bytes>`. They are written only if the bytes
currently there match (64 bytes max). For example, `GHOST.patch` cuts Ghost Train's operator countdown from 180 s to
0.5 s:

```
poke 00043B2E c745d400003443 c745d40000003f
```

### Where the addresses come from

For each `PIX_*`, `TRK_*`, `CTRL_*`, `SND_*`, `CD_*` and `NET_*` function the game uses, you need its static address and
its first 16 bytes. Older Virtuality builds start functions with `68 <imm32>` (push) followed by a call to the stack check.
You also need the anchor, one or two `dataref` sites, and the addresses and sizes of the PIX tables.

The existing tables were generated from the games' own debug symbols (several Virtuality games shipped `.SYM` / `.MAP`
files) and then checked by hand. For a game without symbols, start from a game that has them: the library code is the same
across games, so the same functions can be found by their byte patterns. A disassembler (Ghidra, IDA) helps.

Start with `L` (log) hooks to watch what the game calls. Switch a function to `R` once MallVR handles it.

## 2. The `games.ps1` entry

```powershell
'GUN' = @{ Name = 'Olin Shotgun'; Args = '/vcc'; Fps = 30; GlyphColour = $true; NoWalk = $true;
           PrioLayers = $true; LightWorld = $true; Haptic = 'shot=g:70:1';
           Sha1 = '35ec8e0c619b2c6039773ca364ed4c350cb2c7dd' }
```

| Field | Meaning |
|---|---|
| `Name` | Name on the menu |
| `Args` | The operator command line, from the game's own `*RUN.BAT` |
| `Fps` | Frame limit (0 = none; linked games use 60) |
| `Sha1` | SHA-1 of the EXE the hook table was made for. A different EXE version shows as READY? |
| `Link`, `MaxPlayers`, `PodArgs` | Linked multiplayer |
| `Cd`, `CdBin`, `CdSha1` | CD image |
| `Su3k`, `Exe`, `ScreenW`, `ScreenH` | SU3000 hardware |
| `Native`, `DataDir`, `DataFile` | Native rebuilds such as Buggy Ball |
| everything else | Behaviour flags. Each maps one-to-one to a `VPIX_*` variable in `play.ps1` (`AimHead` → `VPIX_AIM_HEAD` ...). |

The game folder must be `games\...\<EXE>\<EXE>.EXE`. The launcher and Check My Games pick up a new entry automatically.

## 3. `play.ps1`, only if needed

- **A new behaviour flag:** add the field to the entry and one `$env:VPIX_...` line in `play.ps1`, then read it in the
  engine with `getenv`.
- **Linked play:** if the game keeps its network master / slave choice in a file (Dactyl Nightmare 2: `DEFAULTS.DN2`,
  Sea Diving: `SETTINGS.PRL`, ...), add a block like the existing ones that patches that file into each pod's
  `settings\pod<N>` overlay.

## Testing

Run it in a window first (`-Desktop` / `VPIX_VR=0`), then check `logs\<CODE>.calls` for calls that hit stub
`retval`s, and `logs\<CODE>.render` for errors. Then put on the headset.
