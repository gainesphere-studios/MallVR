# Phantom Freight (Halloween 2026): more targets and zombie crowds on Ghost Train's track, made from the PLAYER'S OWN Ghost Train
# files at launch (MallVR never ships the game's data). play.ps1 runs this for games.ps1 ModScript = 'phantom_mod.ps1'.
#   phantom_mod.ps1 -Src <Ghost Train RUNFILES> -Dst <Phantom Freight's layer>\GHOST\RUNFILES
# Each enemy in RUNFILES\SECTIONn.ASS is three objects with one number: the body placed in the world (CCP_SKEL::002, under CCPS,
# with a world POSITION + heading), a marker on a track piece that brings it on (CCPADD_SKEL_002) and one that takes it off
# (CCPREM_SKEL_002). A copy = all three with a new number (+ Offset), its body moved sideways along its own right axis, so it
# comes and goes with the original. Ghouls and rock men are the zombies: copied twice, one each side.
param([Parameter(Mandatory = $true)][string]$Src, [Parameter(Mandatory = $true)][string]$Dst, [string[]]$Only = $null)   # -Only SKEL,BAT: dev, copy just these types ('NONE' = no copies)
$ErrorActionPreference = 'Stop'

# type -> sideways offsets (game units, + = the body's right) of its copies. Not copied: barriers, rocks, switches, timers, quakes,
# the dragon / devil bosses and the scripted wolves (their own logic).
$Copies = [ordered]@{
    'SKEL' = @(1400); 'BAT' = @(1200); 'GOB' = @(1400); 'DEM' = @(1600); 'GARG' = @(1600); 'BT' = @(1800)
    'GHL'  = @(1400); 'RMAN' = @(1600)   # zombies
}
# tested 2026-10-27 (window, 70 s each): every type above loads and plays. Ghosts (GHOST) are left out: with copies the ride never
# started (no CCPREM markers, a fixed set of 4). Two copies of ghouls / rock men hung the loading (fixed slots), one is fine.
if ($Only) { foreach ($k in @($Copies.Keys)) { if ($Only -notcontains $k) { $Copies.Remove($k) } } }
$Offset = 500   # copy n of number k = k + Offset * n (originals stay below 100)

New-Item -ItemType Directory -Force $Dst | Out-Null
$types = $(if ($Copies.Count) { ($Copies.Keys -join '|') } else { 'NO_TYPE_AT_ALL' })
foreach ($f in Get-ChildItem -LiteralPath $Src -Filter 'SECTION*.ASS' -File) {
    $lines = [IO.File]::ReadAllLines($f.FullName)
    $out = New-Object System.Collections.Generic.List[string]
    $added = 0
    foreach ($l in $lines) {
        $out.Add($l)
        # the object this line is about: CCP_<T>::<n>, CCPADD_<T>_<n> or CCPREM_<T>_<n>
        $m = [regex]::Match($l, "\b(CCP_($types)::|CCP(ADD|REM)_($types)_)(\d+)(?=[A-Z]?\b)")   # ghouls' 2nd markers: CCPADD_GHL_003A
        if (-not $m.Success) { continue }
        $t = $(if ($m.Groups[2].Success -and $m.Groups[2].Value) { $m.Groups[2].Value } else { $m.Groups[4].Value })
        $n = [int]$m.Groups[5].Value; $w = $m.Groups[5].Value.Length
        $offs = $Copies[$t]
        for ($c = 0; $c -lt $offs.Count; $c++) {
            $nn = ($n + $Offset * ($c + 1)).ToString().PadLeft($w, '0')
            $copy = $l.Substring(0, $m.Index) + $m.Groups[1].Value + $nn + $l.Substring($m.Index + $m.Length)
            # the body's world position: moved along its right axis (heading az: forward = (-sin az, cos az), right = (cos az, sin az))
            $p = [regex]::Match($copy, 'XYZ\s+(\S+)\s+(\S+)\s+(\S+)\s+AZELTW\s+(\S+)')
            if ($m.Groups[1].Value.StartsWith('CCP_') -and $p.Success -and $copy -match '^\s*POSITION') {
                $inv = [Globalization.CultureInfo]::InvariantCulture
                $x = [double]::Parse($p.Groups[1].Value, $inv); $y = [double]::Parse($p.Groups[2].Value, $inv); $az = [double]::Parse($p.Groups[4].Value, $inv)
                $x += $offs[$c] * [Math]::Cos($az); $y += $offs[$c] * [Math]::Sin($az)
                $copy = $copy.Substring(0, $p.Groups[1].Index) + $x.ToString('0.###', $inv) + $copy.Substring($p.Groups[1].Index + $p.Groups[1].Length)
                $p2 = [regex]::Match($copy, 'XYZ\s+(\S+)\s+(\S+)')
                $copy = $copy.Substring(0, $p2.Groups[2].Index) + $y.ToString('0.###', $inv) + $copy.Substring($p2.Groups[2].Index + $p2.Groups[2].Length)
            }
            $out.Add($copy); $added++
        }
    }
    [IO.File]::WriteAllLines((Join-Path $Dst $f.Name), $out)
    Write-Output "$($f.Name): $added lines added"
}

# Monster of the night (Bobby 10-27): Ghost Train's ghost is a flat 8-frame picture (BITMAPS\16BIT\GHOST1-8.VST: 4 alive, 4 dissolving).
# Each ride a monster is picked at random from engine\phantom_monsters\<name>\ (our own creatures, make_floaty_head.py) and put in its place.
$sets = @(Get-ChildItem -LiteralPath (Join-Path $PSScriptRoot 'phantom_monsters') -Directory -ErrorAction SilentlyContinue |
    Where-Object { (Get-ChildItem -LiteralPath $_.FullName -Filter 'GHOST*.VST' -File).Count -eq 8 })
if ($sets.Count) {
    $pick = $sets | Get-Random
    $bm = Join-Path (Split-Path -Parent $Dst) 'BITMAPS\16BIT'
    New-Item -ItemType Directory -Force $bm | Out-Null
    Copy-Item -Path (Join-Path $pick.FullName 'GHOST*.VST') -Destination $bm -Force
    Write-Output "monster of the night: $($pick.Name)"
}

# Critter ball (Bobby 10-27): the track barriers (MANTYPE 3, models 32-39, the 8 parts that fly apart) become a ball of snarling critters.
# Our own models + barrier type come from engine\phantom_assets (make_critter_ball.py, same paths as under GHOST\); MODELS.DBS is the
# player's own list with those 8 lines pointed at them.
$assets = Join-Path $PSScriptRoot 'phantom_assets'
if (Test-Path -LiteralPath $assets) {
    $gh = Split-Path -Parent $Dst
    Copy-Item -Path (Join-Path $assets '*') -Destination $gh -Recurse -Force
    $md = [IO.File]::ReadAllLines((Join-Path $Src 'MODELS.DBS'))
    for ($i = 0; $i -lt $md.Count; $i++) {
        $mm = [regex]::Match($md[$i], '^(MODEL\s+)(3[2-9])(\s+)models\\barrier\\\S+', 'IgnoreCase')
        if ($mm.Success) { $md[$i] = $mm.Groups[1].Value + $mm.Groups[2].Value + $mm.Groups[3].Value + "models\pfball\ball_$([int]$mm.Groups[2].Value - 32).bin" }
    }
    [IO.File]::WriteAllLines((Join-Path $Dst 'MODELS.DBS'), $md)
    Write-Output 'barriers: critter balls'
}

# The full ride's order (Bobby 10-28/29; events\phantom_freight\hell_slice_dev.py): TRACK.DBS = the original sections 1, 3, 4, 6, then
# section 5 (hell, stage 4), section 2 (the museum, stage 5: flat, "no ups and downs"), section 7 (the end of the line: its end finishes
# the ride, so it stays last), then the tutorial.
$order = 1, 3, 4, 6, 5, 2, 7
[IO.File]::WriteAllText((Join-Path $Dst 'TRACK.DBS'), ((@($order | ForEach-Object { "runfiles\\section$_.ass" }) + 'runfiles\\tutorial.ass') -join "`r`n") + "`r`n")
Write-Output "TRACK.DBS: sections $($order -join ', ') + tutorial"
# the museum's section is just the ride (Bobby 10-28: "no witch grabbing us ... or monsters or shooting"): every enemy and barrier in it
# goes 200 m under the ground and the markers that bring them on come off the track (the names stay declared: the game still finds them)
$sec = Join-Path $Dst 'SECTION2.ASS'
if (-not (Test-Path -LiteralPath $sec)) { Copy-Item -LiteralPath (Join-Path $Src 'SECTION2.ASS') -Destination $sec }
$en = 'WOLF\d|BAR|BAT|BT|GHL|SKEL|GOB|DEM|GARG|RMAN|BUF'
$t = [IO.File]::ReadAllText($sec)
$t = [regex]::Replace($t, "(POSITION\s+CCP_(?:$en)::\d+\s+XYZ\s+)(\S+)\s+(\S+)\s+(\S+)", { param($m) $m.Groups[1].Value + $m.Groups[2].Value + '  ' + $m.Groups[3].Value + '  -200000' })
$t = [regex]::Replace($t, "(?m)^[ \t]*(?:ADD|POSITION|PRIORITY)[ \t]+CCPADD_(?:$en|REN)_\d+[A-Z]?\b[^\n]*\n", '')
[IO.File]::WriteAllText($sec, $t)
Write-Output 'SECTION2.ASS: the museum cleared of enemies'
