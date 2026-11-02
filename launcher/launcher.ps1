# MallVR front end (MallVR_Launcher.bat). Shows the game select screen (MallLauncher.cs, compiled once into
# ..\logs), runs play.ps1 with the choice, waits for the game to close, then shows the screen again.
# The Play_*.bat / Link_*.bat files still work on their own; this is the same launch path with a menu on top.
#   launcher.ps1                 the menu
#   launcher.ps1 -Shot <png>     dev: draw the menu to a picture and exit
param([string]$Shot = '')
$ErrorActionPreference = 'Stop'
$Engine = $PSScriptRoot
$Root = Split-Path -Parent $Engine
. (Join-Path $Engine 'games.ps1')
. (Join-Path $Engine 'options.ps1')
$logs = Join-Path $Root 'logs'; $settings = Join-Path $Root 'settings'
New-Item -ItemType Directory -Force $logs, $settings | Out-Null

Add-Type -AssemblyName System.Windows.Forms, System.Drawing
# 32-bit Windows: the 32-bit engine in engine\x86, window mode only (DevilMaster 10-22; SteamVR needs 64-bit Windows)
$x86 = -not [Environment]::Is64BitOperatingSystem
if ($x86 -and -not (Test-Path -LiteralPath (Join-Path $Engine 'x86\dosbox-x.exe'))) {
    [void][System.Windows.Forms.MessageBox]::Show("MallVR needs 64-bit Windows 10 or 11.`n`nThis PC runs 32-bit Windows, which can't start MallVR's engine (with or without a headset).", 'MallVR', 'OK', 'Error')
    exit 1
}
$cs = @((Join-Path $Engine 'MallLauncher.cs'), (Join-Path $Engine 'MallHall.cs'), (Join-Path $Engine 'MallOnline.cs'), (Join-Path $Engine 'MallGetGames.cs'), (Join-Path $Engine 'MallVrPanel.cs'), (Join-Path $Engine 'MallNotes.cs'), (Join-Path $Engine 'openvr_api.cs'))
$tag = (($cs | ForEach-Object { (Get-FileHash -LiteralPath $_ -Algorithm SHA1).Hash.Substring(0, 4) }) -join '')
$dll = Join-Path $logs "MallLauncher_$tag.dll"
if (-not (Test-Path $dll)) {
    Get-ChildItem -LiteralPath $logs -Filter 'MallLauncher_*.dll' | Remove-Item -Force -ErrorAction SilentlyContinue
    Add-Type -Path $cs -ReferencedAssemblies System.Windows.Forms, System.Drawing, System.Net, System.IO.Compression -OutputAssembly $dll
}
Add-Type -Path $dll
# the Gainesphere intro (MallHall.cs, MallIntro) plays while the games are found and SteamVR starts; 'intro = 0' in
# settings\launcher.txt (or VPIX_NOSPLASH) turns it off
$sfIntro = Join-Path $settings 'launcher.txt'
if (-not $Shot -and -not $env:VPIX_NOSPLASH -and -not ((Test-Path -LiteralPath $sfIntro) -and (Select-String -LiteralPath $sfIntro -Pattern '^\s*intro\s*=\s*0' -Quiet))) {
    [MallIntro]::Start((Join-Path $Engine 'hall\intro'))
}
[HallData]::Preload((Join-Path $Engine 'hall'), $settings, [MallLauncher]::EraPods)   # the hall's pictures + text, while the intro plays

# which games are in the games folder: <CODE>\<CODE>.EXE, as play.ps1 looks for them. Looked for again each time the game select
# screen opens (GET GAMES may have just unpacked some).
function Find-Games {
$exes = @{}
Get-ChildItem -LiteralPath (Join-Path $Root 'games') -Recurse -Filter '*.EXE' -File -ErrorAction SilentlyContinue |
    ForEach-Object {
        $c = $_.BaseName.ToUpper()
        if ($_.Directory.Name -ine $c) { return }
        # SU3000 games keep their own EXE name (GUN\GUN.EXE = Total Recoil, ZERO\ZERO.EXE = Zero Hour): tell them apart by SHA1, as play.ps1 does
        $su3k = @($Games.Keys | Where-Object { ($Games[$_].Su3k -or $Games[$_].VersionOf) -and $Games[$_].Exe -ieq $c })   # (other builds too, games.ps1 VersionOf)
        if ($su3k.Count) {
            $h = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA1).Hash.ToLower()
            $hit = $su3k | Where-Object { $Games[$_].Sha1 -eq $h } | Select-Object -First 1
            if ($hit) { $exes[$hit] = $_.FullName; return }
        }
        $exes[$c] = $_.FullName
    }
foreach ($k in @($Games.Keys)) {   # native rebuilds: their data folder
    $gi = $Games[$k]
    if ($gi.Native) {
        $d = Get-ChildItem -LiteralPath (Join-Path $Root 'games') -Recurse -Directory -Filter $gi.DataDir -ErrorAction SilentlyContinue |
            Where-Object { Test-Path -LiteralPath (Join-Path $_.FullName $gi.DataFile) } | Select-Object -First 1
        if ($d) { $exes[$k] = $d.FullName }
    }
}
foreach ($k in @($Games.Keys)) {   # games built on another one's files (Phantom Freight = Ghost Train): found when the base game is
    $gi = $Games[$k]
    if ($gi.Base -and $exes[$gi.Base]) { $exes[$k] = $exes[$gi.Base] }
}
$exes
}
$exes = Find-Games

# last choices
$sf = Join-Path $settings 'launcher.txt'
$nameFile = Join-Path $env:APPDATA 'MallVR\screen_name.txt'
$last = @{}
if (Test-Path $sf) { foreach ($l in Get-Content -LiteralPath $sf) { $kv = $l -split '=', 2; if ($kv.Count -eq 2) { $last[$kv[0].Trim()] = $kv[1].Trim() } } }

# online play (MallOnline.cs): one object for the whole launcher session, so you stay in the lobby between games.
# The game list lives on the website; 'server = <url>' in settings/launcher.txt points somewhere else (dev: online/list_mock.py).
$Net = New-Object MallOnline
$Net.ListUrl = $(if ($last.server) { $last.server } else { 'https://downloads.gainesphere.com/mallvr/online/list.php' })
$ver = Get-Content -LiteralPath (Join-Path $Engine 'version.txt') -ErrorAction SilentlyContinue | ConvertFrom-StringData
if ($ver -and $ver.version) { $Net.Version = $ver.version }
if (-not ($last.key -match '^[0-9a-f]{32}$')) { $last.key = [MallOnline]::NewKey() }
$Net.Key = $last.key
$online = $false

# play notes (MallNotes.cs): asked once on the first screen, OFF until the player says yes. stats.php / report.php sit next to the
# game list, so a dev 'server = ' (online/list_mock.py) gets them too.
if ($ver -and $ver.version) { [MallNotes]::Version = $ver.version }
if ($last.server -and -not $env:MALLVR_NOTES_URL) {
    [MallNotes]::Url = $last.server -replace 'list\.php', 'stats.php'
    [MallNotes]::ReportUrl = $last.server -replace 'list\.php', 'report.php'
}
[MallNotes]::Load($last.stats)
$noted = $false
$lastTag = ''; $lastName = ''

# the same screen in the headset (MallVrPanel.cs): when SteamVR is already running, a panel in front of you, clicked with the laser
$Vr = New-Object MallVrPanel
$Vr.Net = $Net; $Vr.Root = $Root
foreach ($k in $Games.Keys) { $Vr.Titles[$k] = $Games[$k].Name -replace '^Virtuality ', '' }
$vrOn = $false
if ($exes['DAC']) { $Vr.VoiceDir = Join-Path (Split-Path -Parent $exes['DAC']) 'SOUNDS' }   # the Dactyl announcer's own lines, from your DAC files

$ps = Join-Path $Engine 'play.ps1'

# play notes: what this PC is, once per launcher session (only when play notes are ON)
function Note-Start([int]$mode) {
    if ($script:noted -or -not [MallNotes]::On) { return }
    $script:noted = $true
    $ctl = ''; $rt = ''
    $hmd = $Vr.DeviceInfo([ref]$ctl, [ref]$rt)
    [MallNotes]::Add('launcher_start', 'ver', [MallNotes]::Version, 'plat', 'PC', 'windows', [MallNotes]::WindowsVersion(), 'gpu', [MallNotes]::GpuName(),
        'headset', $hmd, 'controller', $ctl, 'runtime', $rt, 'mode', $(if ($mode -eq 0) { 'headset' } else { 'window' }), 'have', (@($Games.Keys | Where-Object { $exes.ContainsKey($_) }) -join ' '))
}

# runs one game and waits for it (MallVrPanel.WaitGame: the BUTTON MASHER MENU in the headset, the online group's switch either way).
# Returns WaitGame's answer: '' closed by itself, 'back', 'switch', 'stayout'. Writes the game's play notes; a crash sets $script:crashed.
function Run-Play([string[]]$a, [switch]$NoWait, [string]$How = 'solo', [int]$Pod = 0) {
    $args2 = @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-WindowStyle', 'Hidden', '-File', "`"$ps`"") + $a
    if ($NoWait) { Start-Process powershell -ArgumentList $args2 | Out-Null; return '' }
    $code = $a[0].ToUpper()
    $name = $(if ($Games[$code]) { $Games[$code].Name } else { $code })
    # headset: holding A + B in the game brings up the BUTTON MASHER MENU (vpix_menu.h talks to the panel on this port).
    # Headset games always get the port, SteamVR or not (Bobby 10-26: the menu didn't open when SteamVR wasn't running yet): the panel
    # connects to SteamVR when the game says hello (MallVrPanel.MenuPortForGame / WaitGame).
    if ($a -notcontains '-Desktop') { $mp = $Vr.MenuPortForGame($Engine); if ($mp) { $env:VPIX_MENU_PORT = "$mp" } }
    $t0 = (Get-Date).AddSeconds(-1)
    $script:lastTag = $(if ($Pod -gt 0) { "$($code.ToLower())_pod$Pod" } else { $code.ToLower() }); $script:lastName = $name
    $Vr.LogTag = $script:lastTag
    $p = Start-Process powershell -ArgumentList $args2 -PassThru
    $null = $p.Handle       # keeps the process handle, or ExitCode comes back empty after it ends
    Remove-Item Env:\VPIX_MENU_PORT -ErrorAction SilentlyContinue
    $r = $Vr.WaitGame($p, $name)
    $p.WaitForExit()
    $exit = $p.ExitCode
    $secs = [int]((Get-Date) - $t0).TotalSeconds
    $crash = ($r -eq '' -and $exit -ne 0 -and $exit -ne 2 -and $null -ne $exit)
    if ([MallNotes]::On) {
        $render = Join-Path $logs "$($script:lastTag).render"
        $avg = ''; $worst = ''
        [MallNotes]::Perf($(if (Test-Path -LiteralPath $render) { $render } else { $null }), [ref]$avg, [ref]$worst)
        $ended = $(if ($r -eq 'back') { 'back_to_menu' } elseif ($r) { $r } elseif ($exit -eq 2) { 'couldnt_start' } elseif ($crash) { 'crashed' } else { 'closed' })
        [MallNotes]::Add('game_end', 'game', $code, 'how', $How, 'pod', "$Pod", 'secs', "$secs", 'ended', $ended, 'exit', "$exit",
            'fps', $avg, 'worst_ms', $worst, 'mode', $(if ($a -contains '-Desktop') { 'window' } else { 'headset' }), 'menu', "$($Vr.MenuOpens)", 'picks', $Vr.MenuPicks)
        if ($crash) {
            [MallNotes]::Add('crash', 'game', $code, 'exit', ('0x{0:X8}' -f $exit), 'log', [MallNotes]::Tail($render, 200))
        }
        if ($Pod -gt 0) {   # linked / online: the game's own network numbers (frames, voice, mic, strangers' packets dropped)
            [MallNotes]::Add('net', [string[]](@('game', $code, 'pod', "$Pod") + [MallNotes]::NetStats((Join-Path $logs "$($script:lastTag).net"))))
        }
        [MallNotes]::Flush($false)
    }
    if ($crash) { $script:crashed = $name }
    return $r
}
$crashed = 

# window / taskbar icon (a .bat can't carry one): stacked MALL / VR wordmark, yellow on navy
$icon = $null; try { $icoPath = Join-Path $Engine 'mallvr.ico'; if (Test-Path -LiteralPath $icoPath) { $icon = New-Object System.Drawing.Icon $icoPath } } catch { }

# a "MallVR" shortcut with the MallVR icon on the Desktop and in the Start menu (Bobby 10-31: players should see which file to
# start, with no .exe that antivirus could flag). Made on this PC, so Windows has nothing to warn about. Made once: deleted by the
# player = stays deleted (AppData\MallVR\shortcut_made.txt). A shortcut that is there is moved to this copy (a new version
# unzips to a new folder). It starts MallVR_Launcher.bat in a normal window (7 = minimized came down to the menu: Bobby 10-31).
if (-not $Shot -and (Test-Path -LiteralPath (Join-Path $Root 'MallVR_Launcher.bat'))) {   # not from the dev template (no .bat there)
    try {
        $made = Join-Path $env:APPDATA 'MallVR\shortcut_made.txt'
        $bat = Join-Path $Root 'MallVR_Launcher.bat'
        $sh = New-Object -ComObject WScript.Shell
        foreach ($lnk in @((Join-Path ([Environment]::GetFolderPath('Desktop')) 'MallVR.lnk'),
                           (Join-Path ([Environment]::GetFolderPath('Programs')) 'MallVR.lnk'))) {
            if (-not (Test-Path -LiteralPath $lnk) -and (Test-Path -LiteralPath $made)) { continue }
            $s = $sh.CreateShortcut($lnk)
            if ($s.TargetPath -eq $bat -and $s.IconLocation -like "$icoPath*" -and $s.WindowStyle -eq 1) { continue }
            $s.TargetPath = $bat; $s.WorkingDirectory = $Root; $s.IconLocation = "$icoPath,0"; $s.WindowStyle = 1
            $s.Description = 'MallVR: the Virtuality arcade in your headset'
            $s.Save()
        }
        if (-not (Test-Path -LiteralPath $made)) { New-Item -ItemType Directory -Force (Split-Path $made) | Out-Null; Set-Content -LiteralPath $made "made $(Get-Date -Format s) from $Root" }
    } catch { }
}

[MallGetGames]::Games = Join-Path $Root 'games'
while ($true) {
    if ([MallGetGames]::AnyDone) { $exes = Find-Games; [MallGetGames]::AnyDone = $false }
    $f = New-Object MallLauncher
    if ($icon) { $f.Icon = $icon }
    $i = 0
    foreach ($code in $Games.Keys) {
        $g = $Games[$code]
        if ($g.VersionOf) { continue }   # another build of a game: its VERSION row (options.ps1), no block of its own
        $name = $g.Name -replace '^Virtuality ', ''
        $f.AddGame($code, $name, [bool]$g.Link, $exes.ContainsKey($code), $(if ($g.MaxPlayers) { [int]$g.MaxPlayers } else { 2 }))
        if ($g.Sha1) { $Net.Sha1For[$code] = $g.Sha1 }   # sent on JOIN: a host refuses different game files (check.ps1 only starts this EXE)
        if ($last.game -eq $code) { $f.Game = $i }
        # its OPTIONS tab (options.ps1: checked rows only)
        foreach ($row in Get-MVOptionRows $code) {
            $ids = [string[]]@($row.Values | ForEach-Object { ($_ -split '=', 2)[0] }); $labels = [string[]]@($row.Values | ForEach-Object { ($_ -split '=', 2)[1] })
            $f.AddOption($code, $row.Key, $row.Label, $ids, $labels, $row.Default)
        }
        $i++
    }
    if ($last.mode) { $f.Mode = [int]$last.mode }
    if ($env:MALLVR_MODE) { $f.Mode = [int]$env:MALLVR_MODE }        # dev: -Shot in HEADSET (0) or WINDOW (1) mode
    if ($last.style) { $f.Style = [int]$last.style }                 # 0 = CYBERSPACE HALL (default), 1 = OPERATOR
    if ($env:MALLVR_STYLE) { $f.Style = [int]$env:MALLVR_STYLE }      # dev: -Shot of either style
    if ($last.music -eq '0') { $f.Music = $false }                   # the hall's menu loop, on unless switched off
    if ($env:MALLVR_WANTED) { $f.Wanted = $true }                     # dev: -Shot of the WANTED board
    # dev: MALLVR_HALLSHOT=1 = -Shot of the hall on a PC with no games (normally MallVR opens GET GAMES then, below)
    if ($last.sbs) { $f.Sbs = [int]$last.sbs }
    if ($last.players) { $f.Players = [int]$last.players }
    if ($last.pod) { $f.Pod = [int]$last.pod }
    if ($last.peer) { $f.Peer = $last.peer }
    # screen name: the last one used, from any MallVR copy (a new version unzips to a new folder with fresh settings)
    $f.NameFile = $nameFile
    $saved = ''; try { if (Test-Path -LiteralPath $nameFile) { $saved = (Get-Content -LiteralPath $nameFile -Raw).Trim() } } catch { }
    if ($saved) { $f.PlayerName = $saved } elseif ($last.name) { $f.PlayerName = $last.name }
    $f.Net = $Net
    $f.Root = $Root; $f.SyncOptions()                              # every game's OPTIONS, for the START packet when this PC hosts
    $f.OnlineScreen = $online
    if ($online -and $Net.InGroup) { $f.Advanced = $true }      # back from a group game: the group's screen (START / next game / LEAVE)
    $f.Root = $Root; $f.ReportTag = $lastTag; $f.ReportGame = $lastName
    if ($crashed) { $f.CrashGame = $crashed; $crashed = '' }
    if ($exes.Count -eq 0 -and -not $online -and -not $gotOffered -and -not $env:MALLVR_HALLSHOT) { $gotOffered = $true; $f.OpenGetGames() }   # no games yet: GET GAMES right away

    if ($Shot) { $f.ShotTime = 2.0; $f.SavePng($Shot); exit 0 }
    if ($x86) { $f.Mode = 1 }   # 32-bit Windows: WINDOW only
    if (-not $vrOn -and $f.Mode -eq 0) { $vrOn = $Vr.Start($Engine) }   # headset mode only; tried again each time the screen opens
    if ($vrOn) { $f.InHeadset = $true; $Vr.Attach($f) }
    [MallIntro]::Wait()
    [void]$f.ShowDialog()
    $Vr.Detach()
    if ($vrOn -and ($f.Action -eq 'start' -or $f.Action -eq 'online')) { $Vr.Say('VS16A.WAV') }   # "Nice move. Let's get out of here."

    $code = $f.Codes[$f.Game]
    $last = @{ game = $code; mode = $f.Mode; players = $f.Players; pod = $f.Pod; peer = $f.Peer; name = $f.PlayerName; server = $last.server; key = $Net.Key; stats = [MallNotes]::Consent; sbs = $f.Sbs; sbs_monitor = $last.sbs_monitor; style = $f.Style; music = $(if ($f.Music) { 1 } else { 0 }); intro = $last.intro }
    if (-not $last.stats) { $last.Remove('stats') }
    if (-not $last.sbs_monitor) { $last.Remove('sbs_monitor') }
    if (-not $last.intro) { $last.Remove('intro') }
    if ($f.Action -eq 'start' -or $f.Action -eq 'online') { Note-Start $f.Mode }
    if (-not $last.server -or $last.server -eq 'https://downloads.gainesphere.com/mallvr/online/list.php') { $last.Remove('server') }
    $online = $f.OnlineScreen
    ($last.GetEnumerator() | ForEach-Object { "$($_.Key) = $($_.Value)" }) | Set-Content -LiteralPath $sf -Encoding UTF8
    $g = $code.ToLower()
    $desk = @(); if ($f.Mode -eq 1 -or $x86) { $desk = @('-Desktop') }
    # window 3D for a head-mounted display (VPIX_SBS, 1920x1080 borderless; sbs_monitor = N in settings\launcher.txt picks the
    # screen, 1 = the first Windows lists). Never on pod 2's window when two pods share this PC (that one stays flat).
    $sbs = $(if ($desk.Count -and $f.Sbs -eq 1) { 'full' } elseif ($desk.Count -and $f.Sbs -eq 2) { 'half' } else { '' })
    $env:VPIX_SBS = ''; $env:VPIX_SBS_MONITOR = $(if ($last.sbs_monitor) { "$($last.sbs_monitor)" } else { '' })
    switch ($f.Action) {
        'start' {
            if ($f.Players -eq 1) {        # two pods on this PC: pod 2 in a window first, then pod 1 (headset unless WINDOW)
                Run-Play @($g, '-Pod', '2', '-Desktop') -NoWait | Out-Null
                $env:VPIX_SBS = $sbs
                Run-Play (@($g, '-Pod', '1') + $desk) -How 'link_pc' -Pod 1 | Out-Null
            }
            elseif ($f.Players -eq 2) { $env:VPIX_SBS = $sbs; Run-Play (@($g, '-Pod', "$($f.Pod)", '-Peers', $f.Peer) + $desk) -How 'link_net' -Pod $f.Pod | Out-Null }
            else { $env:VPIX_SBS = $sbs; Run-Play (@($g) + $desk) | Out-Null }
        }
        'online' {      # START pressed (host) or the host started (joiner): our pod links through MallOnline
            $g = $Net.Game.ToLower()
            $pod = $f.OnlinePod
            $quick = $Net.Quick
            $env:VPIX_SBS = $sbs
            # the group can switch games mid-game (host: Button Masher Menu > SWITCH GAME): close, start the next one as the new pod
            while ($true) {
                $Net.BeginGame($pod)
                # the host's OPTIONS for this game (MallOnline: sent with START / the switch), the same on every pod
                $opt = $(if ($Net.Options) { $Net.Options } else { 'none' })
                $r = Run-Play (@($g, '-Pod', "$pod", '-Peers', "127.0.0.1:$([MallOnline]::LocalPort($pod))", '-Options', $opt) + $desk) -How 'online' -Pod $pod
                if ([MallNotes]::On) {
                    $host1 = $Net.IsHost
                    [MallNotes]::Add('online', 'game', $g.ToUpper(), 'role', $(if ($host1) { 'host' } else { 'member' }), 'quick', $(if ($quick) { '1' } else { '0' }),
                        'via', $(if ($host1) { if ($Net.RouterOpen) { 'upnp' } else { 'punch' } } else { $Net.JoinVia }), 'connect_secs', $(if ($host1) { '' } else { '{0:0.0}' -f $Net.JoinSecs }),
                        'lag_ms', $Net.LagNote(), 'arrived', $(if ($host1) { $Net.Arrivals() } else { '' }), 'ver', [MallNotes]::Version, 'match', $Net.MatchId,
                        'group', (($Net.Roster() | ForEach-Object -Begin { $k = 0; $pl = $Net.RosterPlat() } -Process { "$($_ -replace ' ', '_'):$($pl[$k])"; $k++ }) -join ' '))
                }
                if ($r -eq 'switch' -and $Net.SwitchGame -ne '') {
                    [MallNotes]::Add('group_switch', 'from', $g.ToUpper(), 'to', $Net.SwitchGame, 'role', $(if ($Net.IsHost) { 'host' } else { 'member' }), 'pod', "$($Net.SwitchPod)",
                        'answers', $(if ($Net.IsHost) { $Net.SwitchNote } else { '' }))
                    $g = $Net.SwitchGame.ToLower(); $pod = $Net.SwitchPod
                    $Net.SwitchTaken()
                    $Net.EndGame()
                    continue
                }
                break
            }
            [MallNotes]::Flush($false)
            $Net.AfterGame()
        }
        'rescan' { }      # back from GET GAMES with new files: the loop looks for games again
        'check' { Start-Process powershell -ArgumentList @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-Command', "& '$(Join-Path $Engine 'check.ps1')'; pause") -Wait }
        'update' { Start-Process powershell -ArgumentList @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-Command', "& '$(Join-Path $Engine 'update.ps1')'; pause") -Wait; [MallNotes]::Flush($true); $Net.Stop(); $Vr.Stop(); exit 0 }
        default { [MallNotes]::Flush($true); $Net.Stop(); $Vr.Stop(); exit 0 }
    }
}
