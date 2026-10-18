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
$logs = Join-Path $Root 'logs'; $settings = Join-Path $Root 'settings'
New-Item -ItemType Directory -Force $logs, $settings | Out-Null

Add-Type -AssemblyName System.Windows.Forms, System.Drawing
$cs = @((Join-Path $Engine 'MallLauncher.cs'), (Join-Path $Engine 'MallOnline.cs'))
$tag = (($cs | ForEach-Object { (Get-FileHash -LiteralPath $_ -Algorithm SHA1).Hash.Substring(0, 4) }) -join '')
$dll = Join-Path $logs "MallLauncher_$tag.dll"
if (-not (Test-Path $dll)) {
    Get-ChildItem -LiteralPath $logs -Filter 'MallLauncher_*.dll' | Remove-Item -Force -ErrorAction SilentlyContinue
    Add-Type -Path $cs -ReferencedAssemblies System.Windows.Forms, System.Drawing, System.Net -OutputAssembly $dll
}
Add-Type -Path $dll

# which games are in the games folder: <CODE>\<CODE>.EXE, as play.ps1 looks for them
$exes = @{}
Get-ChildItem -LiteralPath (Join-Path $Root 'games') -Recurse -Filter '*.EXE' -File -ErrorAction SilentlyContinue |
    ForEach-Object { $c = $_.BaseName.ToUpper(); if ($_.Directory.Name -ieq $c) { $exes[$c] = $_.FullName } }

# last choices
$sf = Join-Path $settings 'launcher.txt'
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

$ps = Join-Path $Engine 'play.ps1'
function Run-Play([string[]]$a, [switch]$NoWait) {
    $args2 = @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-WindowStyle', 'Hidden', '-File', "`"$ps`"") + $a
    if ($NoWait) { Start-Process powershell -ArgumentList $args2 | Out-Null } else { Start-Process powershell -ArgumentList $args2 -Wait }
}

while ($true) {
    $f = New-Object MallLauncher
    $i = 0
    foreach ($code in $Games.Keys) {
        $g = $Games[$code]
        $name = $g.Name -replace '^Virtuality ', ''
        $f.AddGame($code, $name, [bool]$g.Link, $exes.ContainsKey($code))
        if ($last.game -eq $code) { $f.Game = $i }
        $i++
    }
    if ($last.mode) { $f.Mode = [int]$last.mode }
    if ($last.players) { $f.Players = [int]$last.players }
    if ($last.pod) { $f.Pod = [int]$last.pod }
    if ($last.peer) { $f.Peer = $last.peer }
    if ($last.name) { $f.PlayerName = $last.name }
    $f.Net = $Net
    $f.OnlineScreen = $online

    if ($Shot) { $f.SavePng($Shot); exit 0 }
    [void]$f.ShowDialog()

    $code = $f.Codes[$f.Game]
    $last = @{ game = $code; mode = $f.Mode; players = $f.Players; pod = $f.Pod; peer = $f.Peer; name = $f.PlayerName; server = $last.server; key = $Net.Key }
    if (-not $last.server -or $last.server -eq 'https://downloads.gainesphere.com/mallvr/online/list.php') { $last.Remove('server') }
    $online = $f.OnlineScreen
    ($last.GetEnumerator() | ForEach-Object { "$($_.Key) = $($_.Value)" }) | Set-Content -LiteralPath $sf -Encoding UTF8
    $g = $code.ToLower()
    $desk = @(); if ($f.Mode -eq 1) { $desk = @('-Desktop') }
    switch ($f.Action) {
        'start' {
            if ($f.Players -eq 1) {        # two pods on this PC: pod 2 in a window first, then pod 1 (headset unless WINDOW)
                Run-Play @($g, '-Pod', '2', '-Desktop') -NoWait
                Run-Play (@($g, '-Pod', '1') + $desk)
            }
            elseif ($f.Players -eq 2) { Run-Play (@($g, '-Pod', "$($f.Pod)", '-Peers', $f.Peer) + $desk) }
            else { Run-Play (@($g) + $desk) }
        }
        'online' {      # START pressed (host) or the host started (joiner): our pod links through MallOnline
            $g = $Net.Game.ToLower()
            $pod = $f.OnlinePod
            $Net.BeginGame($pod)
            Run-Play (@($g, '-Pod', "$pod", '-Peers', "127.0.0.1:$([MallOnline]::LocalPort($pod))") + $desk)
            $Net.AfterGame()
        }
        'check' { Start-Process powershell -ArgumentList @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-Command', "& '$(Join-Path $Engine 'check.ps1')'; pause") -Wait }
        'update' { Start-Process powershell -ArgumentList @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-Command', "& '$(Join-Path $Engine 'update.ps1')'; pause") -Wait; $Net.Stop(); exit 0 }
        default { $Net.Stop(); exit 0 }
    }
}
