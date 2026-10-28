# MallVR launcher. Finds the game in ..\games, writes a DOSBox-X config, starts the
# HLE build and waits for it to close (so LaunchBox / Steam see the game as running).
#   play.ps1 <code>            e.g. dac, pac, zone
#   play.ps1 <path to EXE>     e.g. ...\games\...\DAC\DAC.EXE  (how LaunchBox passes a "ROM")
# Linked pods (multiplayer, games with Link in games.ps1):
#   play.ps1 dac -Pod 1                          pod 1 = master, pod 2.. = slaves (follow the master's start)
#   play.ps1 dac -Pod 2 -Peers 192.168.1.20      the other pods' addresses (default: pods on this PC)
#   -Desktop                                     no headset for this copy (a window, keyboard controls)
param([Parameter(Mandatory = $true)][string]$Game, [int]$Pod = 0, [string]$Peers = '', [switch]$Desktop)
$ErrorActionPreference = 'Stop'
$Engine = $PSScriptRoot
$Root = Split-Path -Parent $Engine
. (Join-Path $Engine 'games.ps1')

function Fail($msg) {
    Add-Type -AssemblyName System.Windows.Forms
    [void][System.Windows.Forms.MessageBox]::Show($msg, 'MallVR', 'OK', 'Error')
    exit 2      # the launcher's play notes: "couldn't start", not a crash
}
# the engine is a 64-bit program: on 32-bit Windows it can't start at all, headset or window (bug list 10-24: a Win10 32-bit PC)
if (-not [Environment]::Is64BitOperatingSystem) { Fail "MallVR needs 64-bit Windows 10 or 11.`n`nThis PC runs 32-bit Windows, which can't start MallVR's engine (with or without a headset)." }

# game code from a code or an EXE path
$code = [IO.Path]::GetFileNameWithoutExtension($Game).ToUpper()
# an EXE path whose file is an SU3000 game (GUN.EXE = Olin Shotgun or Total Recoil, ZERO.EXE = Zero Hour): pick it by SHA1
if (Test-Path -LiteralPath $Game -PathType Leaf) {
    $h = (Get-FileHash -LiteralPath $Game -Algorithm SHA1).Hash.ToLower()
    foreach ($k in $Games.Keys) { if ($Games[$k].Su3k -and $Games[$k].Sha1 -eq $h) { $code = $k } }
}
if (-not $Games.Contains($code)) { Fail "Unknown game '$Game'.`n`nKnown games: $($Games.Keys -join ', ')" }
$info = $Games[$code]
$exeName = $(if ($info.Exe) { $info.Exe } else { $code })
# native rebuild (Buggy Ball): no DOS EXE, the engine runs the game from its data folder (games\...\BALL\RUNFILES\CARS.DBS)
if ($info.Native) {
    $dataDir = Get-ChildItem -LiteralPath (Join-Path $Root 'games') -Recurse -Directory -Filter $info.DataDir -ErrorAction SilentlyContinue |
        Where-Object { Test-Path -LiteralPath (Join-Path $_.FullName $info.DataFile) } | Select-Object -First 1 -ExpandProperty FullName
    if (-not $dataDir) { Fail "$($info.Name) was not found.`n`nPut its $($info.DataDir) folder (the one holding $($info.DataFile)) inside the 'games' folder." }
    $Game = $code
}

# locate the game's EXE: the path given, else search the games folder (same name in both generations: match the SHA1)
if ($info.Native) { $exe = Join-Path $dataDir $info.DataFile }
elseif (Test-Path -LiteralPath $Game -PathType Leaf) { $exe = (Resolve-Path -LiteralPath $Game).Path }
else {
    $cands = Get-ChildItem -LiteralPath (Join-Path $Root 'games') -Recurse -Filter "$exeName.EXE" -File -ErrorAction SilentlyContinue |
        Where-Object { $_.Directory.Name -ieq $exeName }
    $exe = $null
    foreach ($c in $cands) {
        $isSu3k = $false
        foreach ($k in $Games.Keys) { if ($Games[$k].Su3k -and $Games[$k].Exe -ieq $exeName -and $Games[$k].Sha1 -eq (Get-FileHash -LiteralPath $c.FullName -Algorithm SHA1).Hash.ToLower()) { $isSu3k = $true } }
        if ([bool]$info.Su3k -eq $isSu3k) { $exe = $c.FullName; break }
    }
}
if (-not $exe) { Fail "$($info.Name) was not found.`n`nPut your Virtuality $(if ($info.Su3k) { 'SU3000' } else { 'SU2000' }) HDD dump inside the 'games' folder, then run Check_My_Games.bat." }
$gameDir = Split-Path -Parent $exe
$driveC = Split-Path -Parent $gameDir           # the dump root: holds DAC\, PAC\, DOS\ ...

# CD-ROM drive F: the game's disc image when it needs one, else an empty folder (MSCDEX still answers)
$cdLine = "mount f `"$(Join-Path $Engine 'emptycd')`" -t cdrom"
if ($info.Cd) {
    $cue = Get-ChildItem -LiteralPath (Join-Path $Root 'games') -Recurse -Filter $info.Cd -File -ErrorAction SilentlyContinue |
        Select-Object -First 1 -ExpandProperty FullName
    if (-not $cue) { Fail "$($info.Name) needs its CD image ($($info.Cd) + its .BIN) inside the 'games' folder.`n`nRun Check_My_Games.bat for details." }
    $cdLine = "imgmount f `"$cue`" -t cdrom"
}

# disc-check signature for games without an image: CODE and NAME lines from the game's .INF
$sig = ''
$inf = Join-Path $gameDir "$code.INF"
if (Test-Path -LiteralPath $inf) {
    $c = ''; $n = ''
    foreach ($line in Get-Content -LiteralPath $inf) {
        $t = $line.Trim() -split '\s+', 2
        if ($t[0] -ieq 'CODE') { $c = $t[1] } elseif ($t[0] -ieq 'NAME') { $n = $t[1] }
    }
    $sig = "$c|$n"
}

$logs = Join-Path $Root 'logs'
$settings = Join-Path $Root 'settings'
New-Item -ItemType Directory -Force $logs, $settings | Out-Null
$g = $code.ToLower()
$tag = $g

# linked pod: its own writable layer over C: (network settings, scores) so pods can share one game folder
$ovLine = ''
if ($Pod -gt 0) {
    if (-not $info.Link) { Fail "$($info.Name) has no linked (multiplayer) mode in MallVR yet." }
    $maxPods = $(if ($info.MaxPlayers) { [int]$info.MaxPlayers } else { 4 })
    if ($Pod -gt $maxPods) { Fail "$($info.Name) links $maxPods pods at most: pod number must be 1 to $maxPods." }
    $tag = "${g}_pod$Pod"
    $ov = Join-Path $settings "pod$Pod"
    New-Item -ItemType Directory -Force $ov | Out-Null
    if ($code -eq 'DAC') {
        # RUNFILES\DEFAULTS.DN2: line 5 = network on, line 6 = master (1) / slave (0)
        $dn2 = Join-Path $ov 'DAC\RUNFILES\DEFAULTS.DN2'
        $src = if (Test-Path -LiteralPath $dn2) { $dn2 } else { Join-Path $gameDir 'RUNFILES\DEFAULTS.DN2' }
        $lines = @(Get-Content -LiteralPath $src)
        if ($lines.Count -lt 6) { Fail "Dactyl Nightmare's RUNFILES\DEFAULTS.DN2 looks damaged." }
        $lines[4] = '1 '; $lines[5] = $(if ($Pod -eq 1) { '1 ' } else { '0 ' })
        New-Item -ItemType Directory -Force (Split-Path -Parent $dn2) | Out-Null
        [IO.File]::WriteAllText($dn2, ($lines -join "`r`n") + "`r`n")
    }
    if ($code -eq 'DN2') {
        # Dactyl Nightmare 2 keeps a shorter RUNFILES\DEFAULTS.DN2: line 3 = network on, line 4 = master (1) / slave (0)
        $dn2 = Join-Path $ov 'DN2\RUNFILES\DEFAULTS.DN2'
        $src = if (Test-Path -LiteralPath $dn2) { $dn2 } else { Join-Path $gameDir 'RUNFILES\DEFAULTS.DN2' }
        $lines = @(Get-Content -LiteralPath $src)
        if ($lines.Count -lt 4) { Fail "Dactyl Nightmare 2's RUNFILES\DEFAULTS.DN2 looks damaged." }
        $lines[2] = '1 '; $lines[3] = $(if ($Pod -eq 1) { '1 ' } else { '0 ' })
        New-Item -ItemType Directory -Force (Split-Path -Parent $dn2) | Out-Null
        [IO.File]::WriteAllText($dn2, ($lines -join "`r`n") + "`r`n")
    }
    if ($code -eq 'GHOST') {
        # RUNFILES\LASTSETS.DBS (operator screen settings): line 2 = network link on (else the operator settings switch /nm /ns off)
        $ls = Join-Path $ov 'GHOST\RUNFILES\LASTSETS.DBS'
        $src = if (Test-Path -LiteralPath $ls) { $ls } else { Join-Path $gameDir 'RUNFILES\LASTSETS.DBS' }
        $lines = @(Get-Content -LiteralPath $src)
        while ($lines.Count -lt 3) { $lines += '0' }
        $lines[1] = '1'
        New-Item -ItemType Directory -Force (Split-Path -Parent $ls) | Out-Null
        [IO.File]::WriteAllText($ls, ($lines -join "`r`n") + "`r`n")
    }
    if ($code -eq 'PEARL') {
        # RUNFILES\SETTINGS.PRL (Dactyl engine, read as numbers): line 2 = network on, line 3 = master (1) / slave (0)
        $sp = Join-Path $ov 'PEARL\RUNFILES\SETTINGS.PRL'
        $src = if (Test-Path -LiteralPath $sp) { $sp } else { Join-Path $gameDir 'RUNFILES\SETTINGS.PRL' }
        $lines = @(Get-Content -LiteralPath $src)
        if ($lines.Count -lt 3) { Fail "Sea Diving's RUNFILES\SETTINGS.PRL looks damaged." }
        $lines[1] = '1 '; $lines[2] = $(if ($Pod -eq 1) { '1 ' } else { '0 ' })
        New-Item -ItemType Directory -Force (Split-Path -Parent $sp) | Out-Null
        [IO.File]::WriteAllText($sp, ($lines -join "`r`n") + "`r`n")
    }
    if ($code -eq 'SFL') {
        # CONFIG.BIN = 5 x int32 {?, time, hiscore keep, network 0 none / 1 master / 2 slave, camera}
        $cb = Join-Path $ov 'SFL\CONFIG.BIN'
        $src = if (Test-Path -LiteralPath $cb) { $cb } else { Join-Path $gameDir 'CONFIG.BIN' }
        $b = [IO.File]::ReadAllBytes($src)
        if ($b.Length -lt 20) { Fail "Shoot For Loot's CONFIG.BIN looks damaged." }
        [BitConverter]::GetBytes([int]$(if ($Pod -eq 1) { 1 } else { 2 })).CopyTo($b, 12)
        New-Item -ItemType Directory -Force (Split-Path -Parent $cb) | Out-Null
        [IO.File]::WriteAllBytes($cb, $b)
    }
    $ovLine = "mount -t overlay c `"$ov`""
}
$conf = Join-Path $logs "$tag.conf"
$gameArgs = $(if ($Pod -gt 0 -and $info.PodArgs) { $info.PodArgs[$Pod - 1] } else { $info.Args })   # linked pods: their own launch line (Ghost Train car + master/slave)
$exeDir = $exeName.ToLower()
if ($info.Su3k) {
    # SU3000 / 2000E pod PC: Pentium, 64 MB, Sound Blaster 16 (the AWE32 module is answered by the engine)
    $machine = "memsize=64`r`n[cpu]`r`ncore=normal`r`ncputype=pentium`r`ncycles=max`r`n[sblaster]`r`nsbtype=sb16`r`nsbbase=220`r`nirq=5`r`ndma=1`r`nhdma=5"
    $envLines = "set BLASTER=A220 I5 D1 H5 P330 T6`r`nset SOUND=C:\SB16"
} else {
    $machine = "memsize=32`r`n[cpu]`r`ncore=normal`r`ncputype=486`r`ncycles=max"
    $envLines = ''
}
if ($info.Native) {
@"
[sdl]
output=opengl
[dosbox]
machine=svga_s3
memsize=16
[cpu]
cycles=3000
[autoexec]
"@ | Set-Content -LiteralPath $conf -Encoding Ascii
} else {
@"
[sdl]
output=opengl
[dosbox]
machine=svga_s3
$machine
[dos]
ems=false
umb=false
xms=true
[autoexec]
mount c "$driveC"
$ovLine
$cdLine
c:
set dos4g=quiet
set vpctemp=c:\vpctemp
$envLines
path c:\dos;c:\utils
cd \$exeDir
$exeDir $gameArgs > c:\gameout.txt
exit
"@ | Set-Content -LiteralPath $conf -Encoding Ascii
}

Get-ChildItem -LiteralPath $logs -Filter "$tag.*" -File | Where-Object { $_.Extension -ne '.conf' } | Remove-Item -Force
$env:VPIX_PLAY = '1'
$env:VPIX_LOG = Join-Path $logs $tag
$env:VPIX_HOOKS = $(if ($info.Native) { '' } else { Join-Path $Engine "hooks\$code.hooks" })
$env:VPIX_SETTINGS = Join-Path $settings 'vpix_settings.txt'
$env:VPIX_CDSIG = $sig
$env:VPIX_FPS = "$($info.Fps)"
$env:VPIX_AIM_HEAD = $(if ($info.AimHead) { '1' } else { '0' })
$env:VPIX_NOWALK = $(if ($info.NoWalk) { '1' } else { '0' })   # rail / ring game: no stick back/strafe   # aim trim tilts the head (game aims where you look)
$env:VPIX_WALK_TURN = $(if ($info.WalkTurn) { '1' } else { '0' })   # Pac-Man: back/strafe turn the head the game sees
$env:VPIX_TRIG_WALK = $(if ($info.TrigWalk) { '1' } else { '0' })   # Pac-Man: right trigger walks forward, left trigger back
$env:VPIX_HUD_SCALE = $(if ($info.HudScale) { "$($info.HudScale)" } else { '1' })   # Pac-Man: 2D layer pulled in from the lens edges (VR)
$env:VPIX_NOTURN = $(if ($info.NoTurn) { '1' } else { '0' })   # look-around only: no stick/grip turning
$env:VPIX_SCREEN_CLIP = $(if ($info.Clip) { "$($info.Clip)" } else { '0' })   # visible bottom of the 2D layer
$env:VPIX_SCREEN_H = $(if ($info.ScreenH) { "$($info.ScreenH)" } else { '0' })   # PEARL: 2D screen is 352x248 (0 = default 288)
# PEARL (older PIX lib) turns on by itself in the engine: 0x28 polygons = material colours (glass temple), bitmaps between models
# by priority + head-locked sell messages (VPIX_BLIT_MID / VPIX_PANEL_HEADLOCK = 0 turn those off; not set here)
$env:VPIX_FLIC_PACE = $(if ($info.FlicPace) { '1' } else { '0' })   # videos at the FLIC file's own speed
$env:VPIX_GLYPH_COLOUR = $(if ($info.GlyphColour) { '1' } else { '0' })   # letters in their colour + drop shadow
$env:VPIX_SCARED_FACE = $(if ($info.ScaredFace) { '1' } else { '0' })   # Pac-Man: scared faces on blue ghosts
$env:VPIX_FLIC_CENTRE = $(if ($info.FlicCentre) { '1' } else { '0' })   # Pac-Man: FLIC position is the centre
$env:VPIX_MATERIALS = $(if ($info.Materials) { '1' } else { '0' })   # Pac-Man: type 0x68 polygons textured through PIX_SetMaterial (blue ghosts)
$env:VPIX_HIDE_OWN_HEAD = $(if ($info.HideOwnHead) { '1' } else { '0' })   # DAC: own head model locked behind the eye (view went inside it)
$env:VPIX_CROUCH = $(if ($info.Crouch) { '1' } else { '0' })   # DAC/DN2: hold the crouch button = head fed 400 mm lower (the game's crouch state)
$env:VPIX_HOLD_VIEW = $(if ($info.HoldView) { '1' } else { '0' })   # DAC/DN2: round over (frozen game camera): look around + gun stay live
$env:VPIX_HAPTIC = $(if ($info.Haptic) { $info.Haptic } else { '' })   # rumble on the game's own shot / hit sounds (see games.ps1 Haptic)
$env:VPIX_ARM_TO_GUN = $(if ($info.ArmToGun) { '1' } else { '0' })   # ZONE: forearm re-aimed onto the gun (wrist gap)
$env:VPIX_TIMER_COPY = $(if ($info.TimerCopy) { '1' } else { '0' })   # ZONE: top timer keeps its units digit while the game shows it big
$env:VPIX_NEAR_MODELS = $(if ($info.NearModels) { $info.NearModels } else { '' })   # ZONE: doors drawn just in front of the wall
$env:VPIX_ONE_SIDED = $(if ($info.OneSided) { $info.OneSided } else { '' })   # ZONE: vr_logo white panels one-sided
$env:VPIX_POLY_SELECT = $(if ($info.PolySelect) { $info.PolySelect } else { '' })   # ZONE: scorepow bonus box shows only the picked strokes
$env:VPIX_MUZZLE = $(if ($info.Muzzle) { $info.Muzzle } else { '' })   # ZONE: first gun's muzzle flash, a burst per own shot sound
$env:VPIX_PICTURE_HEADLOCK = $(if ($info.PictureLock -is [bool]) { if ($info.PictureLock) { '1' } else { '0' } } elseif ($info.PictureLock) { "$($info.PictureLock)" } else { '0' })   # ZONE: opening title + logo locked to the view with the picture
$env:VPIX_PICTURE_WAIT_MS = $(if ($info.PictureWait) { "$($info.PictureWait)" } else { '0' })   # ZONE: picture held back until its texts are up
$env:VPIX_PANEL_WORLD = $(if ($info.PanelWorld) { '1' } else { '0' })   # ZONE: attract 2D layer fixed to the game camera
$env:VPIX_FREE_REF_LATCH = $(if ($info.FreeRefLatch) { '1' } else { '0' })   # ZONE: free-look reference yaw taken once (left/right look in intro/attract)
$env:VPIX_FREE_YAW_MAX_DEG = $(if ($info.FreeYawMax) { "$($info.FreeYawMax)" } else { '0' })   # ZONE: desktop free look limit (degrees)
$env:VPIX_FULLPIC_LOCK = $(if ($info.FullPicLock) { '1' } else { '0' })   # ZONE: full-screen picture screens have no free look
$env:VPIX_SUB_LARGEST_FIRST = $(if ($info.SubLargestFirst) { '1' } else { '0' })   # ZONE: fullest octant sub-model merged first (decal order)
$env:VPIX_FOLLOW_YAW = $(if ($info.FollowYaw) { '1' } else { '0' })   # ZONE: follow decision by left/right head turns
$env:VPIX_CLAMP_Y = $(if ($info.ClampY) { $info.ClampY } else { '' })   # ZONE: corridor tunnel ends trimmed behind the walls
$env:VPIX_MOVE_Y = $(if ($info.MoveY) { $info.MoveY } else { '' })   # ZONE: pill sign moved in front of the pill's face
$env:VPIX_FOLLOW_LATCH = $(if ($info.FollowLatch) { '1' } else { '0' })   # ZONE: attract keeps InHead on; free look there
$env:VPIX_FOLLOW_PITCH = $(if ($info.FollowPitch) { '1' } else { '0' })   # MISS: game camera = head + constant pitch; follow it
$env:VPIX_STICK_TWIST = $(if ($info.StickTwist) { "$($info.StickTwist)" } else { '0' })   # MISS: stick left/right twists the gun hand
$env:VPIX_WORLD_SCALE = $(if ($info.WorldScale) { "$($info.WorldScale)" } else { '' })   # MISS 0.6: eye spacing for the near gun barrels (empty = 1.85)
$env:VPIX_BLIT_NUDGE = $(if ($info.BlitNudge) { "$($info.BlitNudge)" } else { '' })   # MISS '1,2': 2D layer nudge in screen pixels (sight onto the shots)
$env:VPIX_ON_TOP = $(if ($info.OnTop) { "$($info.OnTop)" } else { '' })   # MISS 'skip.bin': models drawn over everything (no depth test)
# CD-audio music + voices (Zone Hunter): its CD_PlayTrack plays <folder>TRACKnn.MP3 (or .WAV), the disc's audio tracks ripped and named by
# track number. games.ps1 CdAudio = the folder name searched for under 'games' (Zone: ZONE_CD). No folder = silent, as before.
Remove-Item Env:VPIX_CD_DIR -ErrorAction SilentlyContinue
if ($info.CdAudio) {
    $cdDir = Get-ChildItem -LiteralPath (Join-Path $Root 'games') -Recurse -Directory -Filter $info.CdAudio -ErrorAction SilentlyContinue |
        Where-Object { Get-ChildItem -LiteralPath $_.FullName -Filter 'TRACK*.*' -File -ErrorAction SilentlyContinue | Select-Object -First 1 } |
        Select-Object -First 1 -ExpandProperty FullName
    if ($cdDir) { $env:VPIX_CD_DIR = $cdDir }
}
$env:VPIX_PICTURE_BACK = $(if ($info.PictureBack -is [bool]) { if ($info.PictureBack) { '1' } else { '0' } } elseif ($info.PictureBack) { "$($info.PictureBack)" } else { '0' })   # ZONE 1: big opaque screen picture = bottom layer; SFL 2: translucent ones too
$env:VPIX_BLIT_PRIORITY = $(if ($info.BlitPriority) { '1' } else { '0' })   # DAC: bitmaps in the card's priority order; title/intro picture under the 3D logos + texts
$env:VPIX_BLIT_UNDER = $(if ($info.BlitUnder) { $info.BlitUnder } else { '' })   # PAC 'bells': alarm clock bells under the clock face
$env:VPIX_HIDE_ABOVE = $(if ($info.HideAbove) { "$($info.HideAbove)" } else { '' })   # SFL 5000: under the level (character select) the level is not drawn
$env:VPIX_DEPTH_FIX = $(if ($info.DepthFix) { '1' } else { '0' })   # reversed-Z float depth: no z-fighting of distant decals
$env:VPIX_PRIO_LAYERS = $(if ($info.PrioLayers) { '1' } else { '0' })   # Olin Shotgun: models in the card's priority layers (back panels / dome never hide targets)
$env:VPIX_LIGHT_WORLD = $(if ($info.LightWorld) { '1' } else { '0' })   # Olin Shotgun: the game's lights are world directions (card match 10-25)
$env:VPIX_HIDE_DEATH_BOX = $(if ($info.DeathBox) { '1' } else { '0' })   # DAC: black death/respawn box only drawn from inside
if ($info.HandXY) { $env:VPIX_HAND_XY = "$($info.HandXY)" } else { Remove-Item Env:VPIX_HAND_XY -ErrorAction SilentlyContinue }   # tracker scale (Boxing: real mm)
$env:VPIX_JOYAXES = $(if ($info.JoyAxes) { '1' } else { '0' })   # thumbsticks = pod joystick axes
$env:VPIX_TOP_BUTTONS = $(if ($info.TopButtons) { '1' } else { '0' })   # Ghost Train: grip / stick click / left trigger = top button
$env:VPIX_DESK_CONVERGE = $(if ($info.DeskConverge) { [string]$info.DeskConverge } else { '8000' })   # window mode: gun meets the view ray at this many mm
$env:VPIX_HAND_ROLL = $(if ($info.HandRoll) { '1' } else { '0' })   # Shoot For Loot: gun roll not reversed
$env:VPIX_LEAN = $(if ($info.Lean) { "$($info.Lean)" } else { '0' })   # sticks = held head lean (Boxing)
$env:VPIX_HEAD_CENTRE = $(if ($info.HeadCentre) { '1' } else { '0' })   # where you stand = pod centre (Boxing)
if ($info.HandZ) { $env:VPIX_HAND_Z = "$($info.HandZ)" } else { Remove-Item Env:VPIX_HAND_Z -ErrorAction SilentlyContinue }   # Boxing: hand height scale,offset vs the head
$env:VPIX_TWO_HANDS = $(if ($info.TwoHands) { '1' } else { '0' })   # Boxing: either controller punches (left one = trigger = left glove)
if ($info.HeadXY) { $env:VPIX_HEAD_XY = "$($info.HeadXY)" } else { Remove-Item Env:VPIX_HEAD_XY -ErrorAction SilentlyContinue }
# Boxing (same flags as run_game.sh box). Valued ones are removed for other games: the engine tests some for presence only.
$env:VPIX_BOX_FORCEGLOVE = $(if ($info.ForceGlove) { '1' } else { '0' })   # own forearm + red glove in first person
$env:VPIX_BOX_GLOVE_TRACK = $(if ($info.GloveTrack) { '1' } else { '0' })   # own gloves drawn at the controllers (like the Dactyl gun)
$env:VPIX_MENU_FWD_FLIP = $(if ($info.MenuFwdFlip) { '1' } else { '0' })   # character select: forward jab = menu punch
$env:VPIX_BOX_RING_FEED = $(if ($info.RingFeed) { '1' } else { '0' })   # character select gets the ring's hand feed (tracking right from the start)
$env:VPIX_DESK_BOX = $(if ($info.DeskBox) { '1' } else { '0' })   # window mode: left / right mouse button = left / right punch
$env:VPIX_TRIM_LOCK = $(if ($info.TrimLock) { '1' } else { '0' })   # touchpad trims off, saved reach / arm height ignored (Boxing: thumbs brushed the pads mid-punch)
$env:VPIX_BOX_KO_LIFT = $(if ($info.KoLift) { '1' } else { '0' })   # KO'd boxer lifted onto the mat (no sinking)
$env:VPIX_DECAL_STACK = $(if ($info.DecalStack) { '1' } else { '0' })   # coplanar floor decals in card order (Boxing ring, DN2 arena; no flicker)
foreach ($kv in @(@('HandFwdOff', 'VPIX_HAND_FWD_OFF'), @('HandReachFight', 'VPIX_HAND_REACH_FIGHT'), @('PunchRel', 'VPIX_BOX_PUNCH_REL'), @('HandXSpread', 'VPIX_HAND_X_SPREAD'),
                  @('HandZRight', 'VPIX_HAND_Z_RIGHT'), @('KoExtra', 'VPIX_BOX_KO_EXTRA'), @('DecalBias', 'VPIX_DECAL_BIAS'))) {
    if ($info[$kv[0]]) { Set-Item "Env:$($kv[1])" "$($info[$kv[0]])" } else { Remove-Item "Env:$($kv[1])" -ErrorAction SilentlyContinue }
}
# SU3000 / 2000E: emulated MMC (MPEG) card + PIX2000E PCI board, hooks armed as soon as the code loads, its own 2D size
foreach ($kv in @(@('VPIX_MMC', '1'), @('VPIX_ARM_POLL', '1'), @('VPIX_PIX3K', '1'))) {
    if ($info.Su3k) { Set-Item "Env:$($kv[0])" $kv[1] } else { Remove-Item "Env:$($kv[0])" -ErrorAction SilentlyContinue }
}
if ($info.ScreenW) { $env:VPIX_SCREEN_W = "$($info.ScreenW)" } else { Remove-Item Env:VPIX_SCREEN_W -ErrorAction SilentlyContinue }
$env:VPIX_NO_AIM_ASSIST = $(if ($info.NoAimAssist) { '1' } else { '0' })   # Total Recoil: the game's head-gun blend narrowed
$env:VPIX_AIM_ASSIST_DEG = $(if ($info.AimAssistDeg) { "$($info.AimAssistDeg)" } else { '0' })   # ... to this window (degrees)
$env:VPIX_AIM_ASSIST_FAR_DEG = $(if ($info.AimAssistFarDeg) { "$($info.AimAssistFarDeg)" } else { '0' })   # ... fading to this with the gun away from the face
foreach ($kv in @(@('AimAssistNear', 'VPIX_AIM_ASSIST_NEAR'), @('AimAssistFar', 'VPIX_AIM_ASSIST_FAR'))) {   # snap: full lock with the stock at the face (mm)
    if ($info[$kv[0]]) { Set-Item "Env:$($kv[1])" "$($info[$kv[0]])" } else { Remove-Item "Env:$($kv[1])" -ErrorAction SilentlyContinue }
}
$env:VPIX_DESK_DOT = $(if ($info.DeskDot) { '1' } else { '0' })   # Total Recoil, window only: centre sight
$env:VPIX_THUMB_ON_B = $(if ($info.ThumbOnB) { '1' } else { '0' })   # Total Recoil: menu next on stick down, not stick up
$env:VPIX_GUN_NUDGE = $(if ($info.GunNudge) { '1' } else { '0' })   # Total Recoil: right touchpad nudges the gun
$env:VPIX_AIM_MODES = $(if ($info.AimModes) { '1' } else { '0' })   # Total Recoil: left trigger flips free / assisted aim
foreach ($kv in @(@('GunSnapNear', 'VPIX_GUN_SNAP_NEAR'), @('GunSnapFar', 'VPIX_GUN_SNAP_FAR'))) {   # cheek snap zone (mm)
    if ($info[$kv[0]]) { Set-Item "Env:$($kv[1])" "$($info[$kv[0]])" } else { Remove-Item "Env:$($kv[1])" -ErrorAction SilentlyContinue }
}
# Zero Hour (SU3000): start = front-panel bit 4, decal gap (mm), 2D layer in its draw priority, reload on the left trigger, view fog
$env:VPIX_START_BITS = $(if ($info.StartBits) { "$($info.StartBits)" } else { '0' })
if ($info.DecalGap) { $env:VPIX_DECAL_GAP = "$($info.DecalGap)" } else { Remove-Item Env:VPIX_DECAL_GAP -ErrorAction SilentlyContinue }
$env:VPIX_BLIT3K_PRIO = $(if ($info.Blit3kPrio) { '1' } else { '0' })
$env:VPIX_LTRIG_THUMB = $(if ($info.LtrigThumb) { '1' } else { '0' })
$env:VPIX_FOG3K = $(if ($info.Fog3k) { '1' } else { '0' })
$env:VPIX_MULTIVIEW3K = $(if ($info.Multiview3k) { '1' } else { '0' })   # ZENECA: scene split over views sharing one camera
$env:VPIX_DESK_ATTRACT_LOCK = $(if ($info.Su3k) { '1' } else { '0' })   # window only: no mouse look while the camera is scripted
if ($info.Mpeg) {   # its MPEG movies through ffmpeg (engine\ffmpeg.exe if shipped, else ffmpeg on PATH)
    $env:VPIX_MPEG_DIR = $gameDir
    $ff = Join-Path $Engine 'ffmpeg.exe'
    $env:VPIX_FFMPEG = $(if (Test-Path -LiteralPath $ff) { $ff } else { 'ffmpeg' })
} else { Remove-Item Env:VPIX_MPEG_DIR -ErrorAction SilentlyContinue }
if ($info.Native) {
    $env:VPIX_NATIVE = $info.Native
    $env:VPIX_BUGGY_DIR = $dataDir
    $env:VPIX_BUGGY_LOGO = Join-Path $Engine 'buggy\ball_logo_256.rgba'     # Gainesphere / MallVR badges on the ball
    # $env:VPIX_BUGGY_LOGO = 'none'                                          # plain ball (no badges): use this line instead of the one above
    $env:VPIX_BUGGY_MUSIC = Join-Path $Root 'music\buggy'                   # any WAVs dropped here play as the game music
} else { Remove-Item Env:VPIX_NATIVE -ErrorAction SilentlyContinue }
if ($Desktop) { $env:VPIX_VR = '0' }
foreach ($v in 'VPIX_SEAT_HOST', 'VPIX_SEAT_CLIENT', 'VPIX_SEAT_BIND', 'VPIX_PATCH_EXTRA') { Remove-Item "Env:$v" -ErrorAction SilentlyContinue }
# voice chat in every linked / online game (vpix_voice.h): the mic starts live, Index left stick click (window: M) = mic off / on.
# MALLVR_VOICE=0 turns it off for this PC.
$env:VPIX_VOICE = $(if ($Pod -gt 0 -and $env:MALLVR_VOICE -ne '0') { '1' } else { '0' })
# per-friend volume (Button Masher Menu > VOICE CHAT): the launcher writes it, the engine reads it once a second
$env:VPIX_VOICE_VOL_FILE = Join-Path $settings 'voice_pods.txt'
# every other pod on this PC (online through the launcher's relay, or two pods here): the game's ports listen on 127.0.0.1 only
$env:VPIX_NET_LOCAL = $(if (-not $Peers -or -not ($Peers -split '[,; ]+' | Where-Object { $_ -and $_ -notmatch '^(127\.|localhost)' })) { '1' } else { '0' })
if ($Pod -gt 0 -and $info.Seat2) {
    # Duo game, 2 players = the game's own two seats on pod 1's PC (Boxing 'Link Up'); pod 2 is the REMOTE SEAT: it sends its headset +
    # controllers to pod 1 and draws seat 2's view + plays its sounds from there (vpix_seat.h). UDP 27100 + pod, like the linked pods,
    # so the online relay (MallOnline: 127.0.0.1:27110 + pod) and -Peers work unchanged.
    $env:VPIX_PATCH_EXTRA = Join-Path $Engine "hooks\$($info.Seat2)"   # forces 2 trackers + 2 control boards + 2 video channels
    $env:VPIX_START_BITS = '2'                                          # start = LINK UP (both seats)
    if ($Pod -eq 1) { $env:VPIX_SEAT_HOST = '27101' }
    else {
        $hp = '127.0.0.1:27101'
        if ($Peers) { $h = ($Peers -split '[,; ]+' | Where-Object { $_ } | Select-Object -First 1); $hp = $(if ($h -match ':\d+$') { $h } else { "${h}:27101" }) }
        $env:VPIX_SEAT_CLIENT = $hp; $env:VPIX_SEAT_BIND = "$(27100 + $Pod)"
        $env:VPIX_NOSPLASH = '1'
    }
    if ($info.Fps -eq 0) { $env:VPIX_FPS = '60' }
} elseif ($Pod -gt 0) {
    $env:VPIX_NET = "$Pod"
    # the pods ran ~30 fps; uncapped (thousands of fps) linked copies flood each other with packets
    if ($info.Fps -eq 0 -and -not $info.Native) { $env:VPIX_FPS = '60' }   # (a native game paces itself)
    if ($Peers) {   # "host" or "host:port", comma separated; port defaults to 27100 + that pod's number
        $list = foreach ($h in $Peers -split '[,; ]+' | Where-Object { $_ }) {
            if ($h -match ':\d+$') { $h } else { 1..4 | Where-Object { $_ -ne $Pod } | ForEach-Object { "${h}:$(27100 + $_)" } }
        }
        $env:VPIX_NET_PEERS = ($list -join ',')
    }
    if ($Pod -gt 1) { $env:VPIX_NOSPLASH = '1' }   # one launch screen / update question per PC is enough
}

# launch screen: the studio ident floats over the desktop (see-through window) while we check
# for a newer version (Drive, then GitHub), ~2.5 s. MallSplash.cs is compiled once into logs\ and reused.
$splash = Join-Path $Engine 'splash.png'
$offer = $null
if ((Test-Path $splash) -and -not $env:VPIX_NOSPLASH) {
    try {
        Add-Type -AssemblyName System.Windows.Forms, System.Drawing
        $cs = Join-Path $Engine 'MallSplash.cs'
        $tag = (Get-FileHash -LiteralPath $cs -Algorithm SHA1).Hash.Substring(0, 8)
        $dll = Join-Path $logs "MallSplash_$tag.dll"
        if (-not (Test-Path $dll)) {
            Get-ChildItem -LiteralPath $logs -Filter 'MallSplash_*.dll' | Remove-Item -Force -ErrorAction SilentlyContinue
            Add-Type -Path $cs -ReferencedAssemblies System.Windows.Forms, System.Drawing -OutputAssembly $dll
        }
        Add-Type -Path $dll
        $form = New-Object MallSplash $splash
        $form.Show()
        $grey = [System.Drawing.Color]::FromArgb(244, 234, 210); $gold = [System.Drawing.Color]::FromArgb(232, 179, 58)
        $form.SetStatus("Starting $($info.Name)...", $grey); [System.Windows.Forms.Application]::DoEvents()
        $t0 = Get-Date
        $cfg = Get-Content -LiteralPath (Join-Path $Engine 'version.txt') -ErrorAction SilentlyContinue | ConvertFrom-StringData
        $hasSource = ($cfg.drive -and $cfg.drive -ne 'none') -or ($cfg.repo -and $cfg.repo -ne 'none')
        if ($hasSource -and $cfg.check_on_launch -ne 'no') {
            $form.SetStatus('Checking for updates...', $grey); [System.Windows.Forms.Application]::DoEvents()
            try {
                . (Join-Path $Engine 'findupdate.ps1')
                $latest = Get-MVLatest $cfg 3
                if ($latest -and (Get-MVVersion $latest.Version) -gt (Get-MVVersion $cfg.version)) {
                    $form.SetStatus("Update $($latest.Version) available", $gold); $offer = $latest
                }
                else { $form.SetStatus("Up to date ($($cfg.version))", $grey) }
            } catch { $form.SetStatus("Starting $($info.Name)...", $grey) }   # offline: just play
        }
        while (((Get-Date) - $t0).TotalSeconds -lt 2.5) { [System.Windows.Forms.Application]::DoEvents(); Start-Sleep -Milliseconds 50 }
        $form.Close(); $form.Dispose()
    } catch { }                                   # a launch screen problem never stops the game
}

# a newer version: ask once per version. Yes = install it, then start the game on the new engine.
if ($offer) {
    try {
        $skipFile = Join-Path $settings 'update_skipped.txt'
        $skipped = if (Test-Path $skipFile) { (Get-Content -LiteralPath $skipFile -Raw).Trim() } else { '' }
        if ($skipped -ne $offer.Version) {
            $notes = if ($offer.Notes) { "`n`n$($offer.Notes)" } else { '' }
            $msg = "MallVR $($offer.Version) is out (you have $($cfg.version)).$notes`n`n" +
                "Install it now? It takes about a minute. Your games, settings and high scores are kept.`n`n" +
                "No = play now. You won't be asked about $($offer.Version) again; Update.bat installs it any time."
            $owner = New-Object System.Windows.Forms.Form -Property @{ TopMost = $true }
            $ans = [System.Windows.Forms.MessageBox]::Show($owner, $msg, 'MallVR update', 'YesNo', 'Question')
            $owner.Dispose()
            if ($ans -eq 'Yes') {
                $u = Start-Process powershell -PassThru -Wait -ArgumentList '-NoProfile', '-ExecutionPolicy', 'Bypass',
                    '-File', "`"$(Join-Path $Engine 'update.ps1')`"", '-Auto'
                if ($u.ExitCode -eq 0) {
                    $env:VPIX_NOSPLASH = '1'
                    & powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $Engine 'play.ps1') $Game
                    exit $LASTEXITCODE
                }
            }
            else { Set-Content -LiteralPath $skipFile -Value $offer.Version }
        }
    } catch { }                                   # an update problem never stops the game
}

$p = Start-Process -FilePath (Join-Path $Engine 'dosbox-x.exe') -WorkingDirectory $Engine -PassThru `
    -ArgumentList '-conf', "`"$conf`"", '-nopromptfolder'
$p.WaitForExit()
exit $p.ExitCode    # the launcher tells a crash (not 0) from a normal close
