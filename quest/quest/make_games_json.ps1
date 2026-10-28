# MallVR Quest: games.ps1 + play.ps1's env table -> games.json for the Quest launcher (plan section 5: per-game settings in one place).
#   powershell -File quest\make_games_json.ps1 <out.json>
# Reads package\engine\games.ps1 and play.ps1 (never edits them). play.ps1's VPIX_* block (from "$env:VPIX_PLAY = '1'" up to
# "if ($Desktop)") is run once per game with placeholder folders, so every game gets exactly the env the PC launcher gives it
# (solo, headset). Placeholders the app fills in: @ROOT@ = /sdcard/MallVR, @ENGINE@ = @ROOT@/engine, @LOGS@ = @ROOT@/logs,
# @SETTINGS@ = @ROOT@/settings, @GAMEDIR@ = the game's EXE folder, @DATADIR@ = a native game's data folder. A value that starts
# with one of them is a host path (the app turns \ into /); every other value goes to the engine as is (DOS paths keep their \).
param([Parameter(Mandatory = $true)][string]$Out)
$ErrorActionPreference = 'Stop'
$eng = Join-Path (Split-Path -Parent $PSScriptRoot) 'package\engine'
. (Join-Path $eng 'games.ps1')
$play = [IO.File]::ReadAllText((Join-Path $eng 'play.ps1'))
$a = $play.IndexOf("`$env:VPIX_PLAY = '1'"); $b = $play.IndexOf("if (`$Desktop) { `$env:VPIX_VR = '0' }")
if ($a -lt 0 -or $b -le $a) { throw "play.ps1 changed: the VPIX_* env block anchors are gone (update make_games_json.ps1)" }
$block = [ScriptBlock]::Create($play.Substring($a, $b - $a))
# the linked / online part right after it (voice chat, VPIX_NET, Boxing's remote seat), up to the launch screen
$c = $play.IndexOf("`$env:VPIX_VOICE = "); $d = $play.IndexOf("# launch screen: the studio ident")
if ($c -le $b -or $d -le $c) { throw "play.ps1 changed: the linked-pod env block anchors are gone (update make_games_json.ps1)" }
$podBlock = [ScriptBlock]::Create($play.Substring($c, $d - $c))

$Root = '@ROOT@'; $Engine = '@ENGINE@'; $logs = '@LOGS@'; $settings = '@SETTINGS@'; $gameDir = '@GAMEDIR@'; $dataDir = '@DATADIR@'
$list = @()
foreach ($code in $Games.Keys) {
    $info = $Games[$code]
    Get-ChildItem Env: | Where-Object { $_.Name -like 'VPIX_*' } | ForEach-Object { Remove-Item "Env:$($_.Name)" }
    $tag = $code.ToLower(); $sig = '@CDSIG@'
    & $block
    $ev = [ordered]@{}
    Get-ChildItem Env: | Where-Object { $_.Name -like 'VPIX_*' } | Sort-Object Name | ForEach-Object { $ev[$_.Name] = $_.Value.Replace([string][char]8, '\b') }   # play.ps1 10-25 has backspace bytes for \b in the Buggy paths
    $ev.Remove('VPIX_FFMPEG')         # no ffmpeg on the headset (SU3000 movies: pl_mpeg later, plan section 8)
    $ev.Remove('VPIX_CD_DIR')         # the app finds the CdAudio folder itself
    $g = [ordered]@{ code = $code; name = $info.Name; exe = $(if ($info.Exe) { $info.Exe } else { $code }); args = "$($info.Args)" }
    foreach ($k in 'Sha1', 'Cd', 'CdBin', 'CdSha1', 'Native', 'DataDir', 'DataFile', 'CdAudio', 'Mpeg', 'Su3k', 'Link', 'MaxPlayers', 'Fps') {
        if ($null -ne $info[$k] -and "$($info[$k])" -ne '') { $g[$k.Substring(0, 1).ToLower() + $k.Substring(1)] = $info[$k] }
    }
    $g["env"] = $ev
    # online (crossplay): the same block once per pod, as the PC launcher runs play.ps1 -Pod <n> -Peers 127.0.0.1:<27110 + n>
    # (MallOnline's relay port), so a Quest pod gets exactly the env a PC pod gets: VPIX_NET, the 60 fps cap, Boxing's remote seat...
    if ($info.Link) {
        $pods = [ordered]@{}
        $mx = $(if ($info.MaxPlayers) { [int]$info.MaxPlayers } else { 2 })
        foreach ($Pod in 1..$mx) {
            Get-ChildItem Env: | Where-Object { $_.Name -like 'VPIX_*' } | ForEach-Object { Remove-Item "Env:$($_.Name)" }
            $Peers = "127.0.0.1:$(27110 + $Pod)"; $tag = "$($code.ToLower())_pod$Pod"
            & $block
            & $podBlock
            $pe = [ordered]@{}
            Get-ChildItem Env: | Where-Object { $_.Name -like 'VPIX_*' } | Sort-Object Name | ForEach-Object { $pe[$_.Name] = $_.Value.Replace([string][char]8, '\b') }
            $pe.Remove('VPIX_FFMPEG'); $pe.Remove('VPIX_CD_DIR')
            $pods["$Pod"] = $pe
        }
        $Pod = 0; $Peers = ''
        $g["pods"] = $pods
        if ($info.PodArgs) { $g["podArgs"] = @($info.PodArgs) }
    }
    $list += $g
}
Get-ChildItem Env: | Where-Object { $_.Name -like 'VPIX_*' } | ForEach-Object { Remove-Item "Env:$($_.Name)" }
$json = [ordered]@{ made = (Get-Date -Format 'yyyy-MM-dd HH:mm'); games = $list } | ConvertTo-Json -Depth 5
[IO.File]::WriteAllText($Out, $json, (New-Object Text.UTF8Encoding $false))
"games.json: $($list.Count) games -> $Out"
