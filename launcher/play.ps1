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
    exit 1
}

# game code from a code or an EXE path
$code = [IO.Path]::GetFileNameWithoutExtension($Game).ToUpper()
if (-not $Games.Contains($code)) { Fail "Unknown game '$Game'.`n`nKnown games: $($Games.Keys -join ', ')" }
$info = $Games[$code]

# locate the game's EXE: the path given, else search the games folder
if (Test-Path -LiteralPath $Game -PathType Leaf) { $exe = (Resolve-Path -LiteralPath $Game).Path }
else {
    $exe = Get-ChildItem -LiteralPath (Join-Path $Root 'games') -Recurse -Filter "$code.EXE" -File -ErrorAction SilentlyContinue |
        Where-Object { $_.Directory.Name -ieq $code } | Select-Object -First 1 -ExpandProperty FullName
}
if (-not $exe) { Fail "$($info.Name) was not found.`n`nPut your Virtuality SU2000 HDD dump inside the 'games' folder, then run Check_My_Games.bat." }
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
    if ($Pod -gt 4) { Fail "Pod number must be 1 to 4." }
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
    $ovLine = "mount -t overlay c `"$ov`""
}
$conf = Join-Path $logs "$tag.conf"
@"
[sdl]
output=opengl
[dosbox]
machine=svga_s3
memsize=32
[cpu]
core=normal
cputype=486
cycles=max
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
path c:\dos;c:\utils
cd \$g
$g $($info.Args) > c:\gameout.txt
exit
"@ | Set-Content -LiteralPath $conf -Encoding Ascii

Get-ChildItem -LiteralPath $logs -Filter "$tag.*" -File | Where-Object { $_.Extension -ne '.conf' } | Remove-Item -Force
$env:VPIX_PLAY = '1'
$env:VPIX_LOG = Join-Path $logs $tag
$env:VPIX_HOOKS = Join-Path $Engine "hooks\$code.hooks"
$env:VPIX_SETTINGS = Join-Path $settings 'vpix_settings.txt'
$env:VPIX_CDSIG = $sig
$env:VPIX_FPS = "$($info.Fps)"
$env:VPIX_AIM_HEAD = $(if ($info.AimHead) { '1' } else { '0' })
$env:VPIX_NOWALK = $(if ($info.NoWalk) { '1' } else { '0' })   # rail / ring game: no stick back/strafe   # aim trim tilts the head (game aims where you look)
$env:VPIX_WALK_TURN = $(if ($info.WalkTurn) { '1' } else { '0' })   # Pac-Man: back/strafe turn the head the game sees
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
$env:VPIX_CLAMP_Y = $(if ($info.ClampY) { $info.ClampY } else { '' })   # ZONE: corridor tunnel ends trimmed behind the walls
$env:VPIX_MOVE_Y = $(if ($info.MoveY) { $info.MoveY } else { '' })   # ZONE: pill sign moved in front of the pill's face
$env:VPIX_FOLLOW_LATCH = $(if ($info.FollowLatch) { '1' } else { '0' })   # ZONE: attract keeps InHead on; free look there
$env:VPIX_FOLLOW_PITCH = $(if ($info.FollowPitch) { '1' } else { '0' })   # MISS: game camera = head + constant pitch; follow it
$env:VPIX_STICK_TWIST = $(if ($info.StickTwist) { "$($info.StickTwist)" } else { '0' })   # MISS: stick left/right twists the gun hand
$env:VPIX_WORLD_SCALE = $(if ($info.WorldScale) { "$($info.WorldScale)" } else { '' })   # MISS 0.6: eye spacing for the near gun barrels (empty = 1.85)
$env:VPIX_BLIT_NUDGE = $(if ($info.BlitNudge) { "$($info.BlitNudge)" } else { '' })   # MISS '1,2': 2D layer nudge in screen pixels (sight onto the shots)
$env:VPIX_ON_TOP = $(if ($info.OnTop) { "$($info.OnTop)" } else { '' })   # MISS 'skip.bin': models drawn over everything (no depth test)
$env:VPIX_PICTURE_BACK = $(if ($info.PictureBack -is [bool]) { if ($info.PictureBack) { '1' } else { '0' } } elseif ($info.PictureBack) { "$($info.PictureBack)" } else { '0' })   # ZONE 1: big opaque screen picture = bottom layer; SFL 2: translucent ones too
$env:VPIX_BLIT_PRIORITY = $(if ($info.BlitPriority) { '1' } else { '0' })   # DAC: bitmaps in the card's priority order; title/intro picture under the 3D logos + texts
$env:VPIX_BLIT_UNDER = $(if ($info.BlitUnder) { $info.BlitUnder } else { '' })   # PAC 'bells': alarm clock bells under the clock face
$env:VPIX_HIDE_ABOVE = $(if ($info.HideAbove) { "$($info.HideAbove)" } else { '' })   # SFL 5000: under the level (character select) the level is not drawn
$env:VPIX_DEPTH_FIX = $(if ($info.DepthFix) { '1' } else { '0' })   # reversed-Z float depth: no z-fighting of distant decals
$env:VPIX_PRIO_LAYERS = $(if ($info.PrioLayers) { '1' } else { '0' })   # Olin Shotgun: models in the card's priority layers (back panels / dome never hide targets)
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
$env:VPIX_BOX_KO_LIFT = $(if ($info.KoLift) { '1' } else { '0' })   # KO'd boxer lifted onto the mat (no sinking)
$env:VPIX_DECAL_STACK = $(if ($info.DecalStack) { '1' } else { '0' })   # coplanar floor decals in card order (Boxing ring, DN2 arena; no flicker)
foreach ($kv in @(@('HandFwdOff', 'VPIX_HAND_FWD_OFF'), @('HandReachFight', 'VPIX_HAND_REACH_FIGHT'), @('HandXSpread', 'VPIX_HAND_X_SPREAD'),
                  @('HandZRight', 'VPIX_HAND_Z_RIGHT'), @('KoExtra', 'VPIX_BOX_KO_EXTRA'), @('DecalBias', 'VPIX_DECAL_BIAS'))) {
    if ($info[$kv[0]]) { Set-Item "Env:$($kv[1])" "$($info[$kv[0]])" } else { Remove-Item "Env:$($kv[1])" -ErrorAction SilentlyContinue }
}
if ($Desktop) { $env:VPIX_VR = '0' }
if ($Pod -gt 0) {
    $env:VPIX_NET = "$Pod"
    # the pods ran ~30 fps; uncapped (thousands of fps) linked copies flood each other with packets
    if ($info.Fps -eq 0) { $env:VPIX_FPS = '60' }
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
