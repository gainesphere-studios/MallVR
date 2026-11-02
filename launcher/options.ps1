# Per-game OPTIONS (the hall's OPTIONS tab, Operator_Settings_and_2P.md "Design: per-game OPTIONS sheet"). Only settings the game
# itself has: its own switches (Kind arg) or its own settings files (Kind file). The hall (launcher.ps1 -> MallLauncher.AddOption) draws
# these rows and saves the choices in settings\options_<CODE>.txt (key=value per line); play.ps1 applies them. No file = the game's
# own defaults. Online, the host's choices (MallOnline opt=) go to every pod through play.ps1 -Options.
#   Values  = 'id=LABEL' in the order a click steps through them. Default = the id the game uses by itself: an arg row adds no switch; a file row
#             is the value its shipped files hold (Write-MVOptionFiles).
#   arg     : Arg = switch for a value ('{0}' = the id) or a table id -> switch; Drop = table id -> a switch taken OFF the launch line.
#   file    : File = path in the game folder. Token = n-th whitespace-separated number (0-based; the Dactyl-engine settings files are
#             read as a stream of numbers), Dword = n-th int32 (binary), Key = 'KEYWORD value' line (GUN.DBS), Lines = the whole file
#             is the value's lines ('|' between them) (OP.DAT). Written into a writable layer over C:, never the player's own files.
#   version : another build of the game (Bobby 11-01: "selection option in our mallvr for game version per game"). Build = table id -> its code in games.ps1 (VersionOf); ids not in it = this code.
# Bobby 10-31: every row ships, players try them (no per-game test pass). Checked headless 10-31 in the real game: Zone language +
# timer, Boxing time + gore, Dactyl time + billboards, Ghost Train ride time + FreeTime. The rest come from the 10-31 code sweep.
$GameOptions = [ordered]@{
    'DAC' = @(
        @{ Key = 'ver'; Label = 'VERSION'; Kind = 'version'; Default = 'CUR'; Build = @{ 'V103' = 'DAC103' }; Values = @('CUR=V1.04', 'V103=V1.03') }
        @{ Key = 'time'; Label = 'GAME TIME'; Kind = 'file'; File = 'RUNFILES\DEFAULTS.DN2'; Token = 7; Default = '180'
           Values = @('60=1:00', '90=1:30', '120=2:00', '150=2:30', '180=3:00', '240=4:00', '300=5:00', '360=6:00') }
        @{ Key = 'death'; Label = 'DEATH'; Kind = 'file'; File = 'RUNFILES\DEFAULTS.DN2'; Token = 8; Default = '0'; Values = @('0=FALL', '1=EXPLODE') }
        @{ Key = 'shots'; Label = 'SHOTS BEFORE THE BIRD'; Kind = 'file'; File = 'RUNFILES\DEFAULTS.DN2'; Token = 10; Default = '10'; Values = @('5=5', '10=10', '15=15') }
        @{ Key = 'bill'; Label = 'BILLBOARDS'; Kind = 'file'; File = 'RUNFILES\DEFAULTS.DN2'; Token = 13; Default = '0'; Values = @('0=OFF', '1=ON') }
    )
    'DN2' = @(
        @{ Key = 'ver'; Label = 'VERSION'; Kind = 'version'; Default = 'CUR'; Build = @{ 'V204' = 'DN2F' }; Values = @('CUR=V2.05', 'V204=V2.04') }
        @{ Key = 'time'; Label = 'GAME TIME'; Kind = 'file'; File = 'RUNFILES\DEFAULTS.DN2'; Token = 4; Default = '180'
           Values = @('60=1:00', '90=1:30', '120=2:00', '150=2:30', '180=3:00', '240=4:00', '300=5:00', '360=6:00') }
        @{ Key = 'death'; Label = 'DEATH'; Kind = 'file'; File = 'RUNFILES\DEFAULTS.DN2'; Token = 5; Default = '1'; Values = @('0=FALL', '1=EXPLODE') }
        @{ Key = 'bill'; Label = 'BILLBOARDS'; Kind = 'file'; File = 'RUNFILES\DEFAULTS.DN2'; Token = 8; Default = '0'; Values = @('0=OFF', '1=ON') }
    )
    'PAC' = @(
        @{ Key = 'ver'; Label = 'VERSION'; Kind = 'version'; Default = 'REL'; Build = @{ 'P69' = 'PACMAN3K' }; Values = @('REL=RELEASE', 'P69=1996 PROTOTYPE') }
        @{ Key = 'time'; Label = 'TIME PER CREDIT'; Kind = 'arg'; Arg = '/time={0}'; Default = '45'; Values = @('30=0:30', '45=0:45', '60=1:00', '90=1:30', '120=2:00', '180=3:00') }
        @{ Key = 'attract'; Label = 'ATTRACT SOUND'; Kind = 'arg'; Arg = @{ '0' = '/mu' }; Default = '1'; Values = @('1=ON', '0=OFF') }
    )
    'MISS' = @(
        @{ Key = 'ver'; Label = 'VERSION'; Kind = 'version'; Default = 'CUR'; Build = @{ 'V12' = 'MISS12' }; Values = @('CUR=V1.10', 'V12=V1.12 RETAIL') }
        @{ Key = 'skill'; Label = 'DIFFICULTY'; Kind = 'file'; File = 'RUNFILES\LASTSETS.DBS'; Token = 2; Default = '1'; Values = @('0=EASY', '1=MEDIUM', '2=HARD') }
    )
    'GHOST' = @(
        @{ Key = 'ver'; Label = 'VERSION'; Kind = 'version'; Default = 'CUR'; Build = @{ 'V11' = 'GHOST11' }; Values = @('CUR=V1.09', 'V11=V1.11') }
        @{ Key = 'ride'; Label = 'RIDE TIME'; Kind = 'file'; File = 'RUNFILES\LASTSETS.DBS'; Token = 2; Default = '0'
           Values = @('0=1:00', '1=1:30', '2=2:00', '3=2:30', '4=3:00', '5=3:30', '6=4:00') }
        @{ Key = 'free'; Label = 'FREE TIME (CLOCK STOPS)'; Kind = 'arg'; Arg = @{ '1' = '/ft' }; Default = '0'; Values = @('0=OFF', '1=ON') }
    )
    'SFL' = @(
        @{ Key = 'time'; Label = 'TIME LIMIT'; Kind = 'file'; File = 'CONFIG.BIN'; Dword = 1; Default = '2'; Values = @('0=0:45', '1=1:00', '2=1:15', '3=1:30', '4=1:45') }
    )
    'BOX' = @(
        @{ Key = 'time'; Label = 'GAME TIME'; Kind = 'arg'; Arg = '/t{0}'; Default = '150'; Values = @('90=1:30', '120=2:00', '150=2:30', '180=3:00', '240=4:00', '300=5:00') }
        @{ Key = 'stamina'; Label = 'STAMINA'; Kind = 'arg'; Arg = '/S{0}'; Default = '300'; Values = @('150=150', '300=300', '600=600', '1000=1000') }
        @{ Key = 'gore'; Label = 'GORE'; Kind = 'arg'; Arg = '/G{0}'; Default = '1'; Values = @('1=BLOOD', '0=SWEAT') }
        @{ Key = 'skill'; Label = 'DIFFICULTY'; Kind = 'arg'; Arg = '/l{0}'; Default = '1'; Values = @('0=EASY', '1=MEDIUM', '2=HARD') }
    )
    'ZONE' = @(
        @{ Key = 'lang'; Label = 'LANGUAGE'; Kind = 'arg'; Arg = @{ 'JP' = '/J' }; Default = 'EN'; Values = @('EN=ENGLISH', 'JP=JAPANESE') }
        @{ Key = 'timer'; Label = 'GAME TIMER'; Kind = 'arg'; Drop = @{ '0' = '/T' }; Default = '1'; Values = @('1=ON', '0=OFF') }
        @{ Key = 'rand'; Label = 'RANDOM ENEMIES'; Kind = 'arg'; Arg = @{ '1' = '/D4' }; Drop = @{ '1' = '/D3' }; Shuffle = $true; Default = '0'; Values = @('0=OFF', '1=ON') }
    )
    'GUN3K' = @(
        @{ Key = 'pair'; Label = 'GAMES'; Kind = 'file'; File = 'OP.DAT'; Default = 'TQ'
           Lines = @{ 'TQ' = 'TRAP_MASTER|QUICKSHOT_CARNIVAL'; 'TS' = 'TRAP_MASTER|SKEET_MASTER'; 'TC' = 'TRAP_MASTER|CLAY_MASTER'
                      'QS' = 'QUICKSHOT_CARNIVAL|SKEET_MASTER'; 'QC' = 'QUICKSHOT_CARNIVAL|CLAY_MASTER'; 'SC' = 'SKEET_MASTER|CLAY_MASTER' }
           Values = @('TQ=TRAP + QUICKSHOT', 'TS=TRAP + SKEET', 'TC=TRAP + CLAY', 'QS=QUICKSHOT + SKEET', 'QC=QUICKSHOT + CLAY', 'SC=SKEET + CLAY') }
        @{ Key = 'weather'; Label = 'WEATHER'; Kind = 'file'; File = 'RUNFILES\GUN.DBS'; Key2 = 'WEATHER'; Default = 'RANDOM'
           Values = @('RANDOM=RANDOM', 'SPRING=SPRING', 'SUMMER=SUMMER', 'AUTUMN=AUTUMN', 'WINTER=WINTER') }
        @{ Key = 'pull'; Label = 'TRIGGER SENSITIVITY'; Kind = 'file'; File = 'RUNFILES\GUN.DBS'; Key2 = 'PULL'; Default = '6'
           Values = @('1=1', '2=2', '3=3', '4=4', '5=5', '6=6', '7=7', '8=8', '9=9', '10=10', '11=11') }
        @{ Key = 'recoil'; Label = 'RECOIL'; Kind = 'file'; File = 'RUNFILES\GUN.DBS'; Key2 = 'RECOIL'; Default = 'ON'; Values = @('ON=ON', 'OFF=OFF') }
    )
    'ZERO3K' = @(
        @{ Key = 'god'; Label = 'INVINCIBLE'; Kind = 'arg'; Arg = @{ '1' = '/INVINCIBLE' }; Default = '0'; Values = @('0=OFF', '1=ON') }
        @{ Key = 'recoil'; Label = 'RECOIL'; Kind = 'arg'; Arg = @{ '0' = '/RECOIL-' }; Default = '1'; Values = @('1=ON', '0=OFF') }
    )
    'ZENECA3K' = @(
        @{ Key = 'ver'; Label = 'VERSION'; Kind = 'arg'; Arg = @{ 'Z' = '/Z' }; Default = 'N'; Values = @('N=NORMAL', 'Z=ZOMIG ADVERT') }
    )
}
$GameOptions['PHANTOM'] = @($GameOptions['GHOST'] | Where-Object { $_.Kind -ne 'version' })    # Phantom Freight runs Ghost Train's EXE and files

# the rows a game shows and applies
function Get-MVOptionRows([string]$code) { @($GameOptions[$code] | Where-Object { $_ }) }

# the player's saved choices: settings\options_<CODE>.txt, or the online host's string ('k1:v1,k2:v2'; 'none' = all defaults)
function Read-MVOptions([string]$code, [string]$settings, [string]$hostString) {
    $o = @{}
    if ($hostString) {
        foreach ($p in $hostString -split ',') { $kv = $p -split ':', 2; if ($kv.Count -eq 2) { $o[$kv[0].Trim()] = $kv[1].Trim() } }
        return $o
    }
    $f = Join-Path $settings "options_$code.txt"
    if (Test-Path -LiteralPath $f) { foreach ($l in Get-Content -LiteralPath $f) { $kv = $l -split '=', 2; if ($kv.Count -eq 2) { $o[$kv[0].Trim()] = $kv[1].Trim() } } }
    $o
}

# a row's value: the saved one if it is one of its values, else its default
function Get-MVOptionValue($row, $saved) {
    $v = $saved[$row.Key]
    if ($v -and @($row.Values | ForEach-Object { ($_ -split '=', 2)[0] }) -contains $v) { $v } else { $row.Default }
}

# the build the VERSION row picks: its code in games.ps1, or '' for this one
function Get-MVVersion([string]$code, [string]$settings, [string]$hostString) {
    $row = Get-MVOptionRows $code | Where-Object { $_.Kind -eq 'version' } | Select-Object -First 1
    if (-not $row) { return '' }
    $b = $row.Build[(Get-MVOptionValue $row (Read-MVOptions $code $settings $hostString))]
    $(if ($b) { $b } else { '' })
}

# the launch line with the arg rows: their switches added, Drop switches taken off
function Add-MVOptionArgs([string]$code, [string]$gameArgs, $saved) {
    $words = [System.Collections.Generic.List[string]]@($gameArgs -split ' ' | Where-Object { $_ })
    foreach ($row in Get-MVOptionRows $code) {
        if ($row.Kind -ne 'arg') { continue }
        $v = Get-MVOptionValue $row $saved
        if ($v -eq $row.Default) { continue }
        if ($row.Drop -and $row.Drop[$v]) { [void]$words.Remove($row.Drop[$v]) }       # case-sensitive: Zone's /T is not /t
        if ($row.Arg -is [hashtable]) { if ($row.Arg[$v]) { $words.Add($row.Arg[$v]) } }
        elseif ($row.Arg) { $words.Add(($row.Arg -f $v)) }
    }
    $words -join ' '
}

# the file rows, written into the layer $ov (<GAMEDIR>\<file>, e.g. settings\opt_dac\DAC\RUNFILES\DEFAULTS.DN2). Each file is taken from
# the layer when it is there already (pod layers keep the game's own writes), else from the player's game folder. With choices saved
# (or sent by the host) every row is written as shown, its default included: a player's own file may hold other values (a Ghost Train
# copy with ride time 4). No choices at all (DEFAULTS, or a linked pod that never opened the tab) = the player's own values put back.
# Returns the number of rows that are not at their default.
function Write-MVOptionFiles([string]$code, [string]$gameDir, [string]$ov, $saved) {
    $changed = 0
    $own = $saved.Count -eq 0
    $rows = @(Get-MVOptionRows $code | Where-Object { $_.Kind -eq 'file' })
    foreach ($file in @($rows | ForEach-Object { $_.File } | Select-Object -Unique)) {
        $orig = Join-Path $gameDir $file
        if (-not (Test-Path -LiteralPath $orig)) { continue }
        $dst = Join-Path (Join-Path $ov (Split-Path -Leaf $gameDir)) $file
        $src = $(if (Test-Path -LiteralPath $dst) { $dst } else { $orig })
        $mine = @($rows | Where-Object { $_.File -eq $file })
        if ($mine[0].Dword -ne $null) {
            $b = [IO.File]::ReadAllBytes($src); $ob = [IO.File]::ReadAllBytes($orig)
            foreach ($row in $mine) {
                $v = Get-MVOptionValue $row $saved; $at = 4 * [int]$row.Dword
                if ($b.Length -lt $at + 4) { continue }
                if ($own) { if ($ob.Length -ge $at + 4) { [Array]::Copy($ob, $at, $b, $at, 4) } }
                else { [BitConverter]::GetBytes([int]$v).CopyTo($b, $at); if ($v -ne $row.Default) { $changed++ } }
            }
            New-Item -ItemType Directory -Force (Split-Path -Parent $dst) | Out-Null
            [IO.File]::WriteAllBytes($dst, $b)
            continue
        }
        $text = [IO.File]::ReadAllText($src, [Text.Encoding]::GetEncoding(28591)); $otext = [IO.File]::ReadAllText($orig, [Text.Encoding]::GetEncoding(28591))
        foreach ($row in $mine) {
            $v = Get-MVOptionValue $row $saved
            if ($v -ne $row.Default) { $changed++ }
            if ($row.Lines) {
                $text = $(if ($own) { $otext } else { ($row.Lines[$v] -replace '\|', "`r`n") + "`r`n" })
            }
            elseif ($row.Key2) {
                $pat = '(?m)^(' + [regex]::Escape($row.Key2) + '[ \t]+)\S+'
                $val = $(if ($own) { $m = [regex]::Match($otext, $pat); if ($m.Success) { ($m.Value -split '\s+')[-1] } else { $v } } else { $v })
                $text = [regex]::Replace($text, $pat, { param($m) $m.Groups[1].Value + $val })
            }
            elseif ($row.Token -ne $null) {
                $val = $v
                if ($own) { $om = [regex]::Matches($otext, '\S+'); if ($om.Count -gt $row.Token) { $val = $om[$row.Token].Value } }
                $tm = [regex]::Matches($text, '\S+')
                if ($tm.Count -le $row.Token) { continue }
                $t = $tm[$row.Token]
                $text = $text.Substring(0, $t.Index) + $val + $text.Substring($t.Index + $t.Length)
            }
        }
        New-Item -ItemType Directory -Force (Split-Path -Parent $dst) | Out-Null
        [IO.File]::WriteAllText($dst, $text, [Text.Encoding]::GetEncoding(28591))
    }
    # Missile Command 1.12 (Bobby 11-01): always has the developers' cut final wave, "IMPOSSIBLE END BIT", commented out after the mother ship
    # in all three level files. It goes back in before the game's last report.
    if ((Split-Path -Leaf $ov) -match '^opt_miss12') {   # its folder is called MISS too; the layer is named after the build
        foreach ($i in 0, 1, 2) {
            $orig = Join-Path $gameDir "RUNFILES\LEVELS$i.DBS"
            if (-not (Test-Path -LiteralPath $orig)) { continue }
            $otext = [IO.File]::ReadAllText($orig, [Text.Encoding]::GetEncoding(28591))
            $h = $otext.IndexOf('IMPOSSIBLE END BIT'); $e = $otext.LastIndexOf(";`tEND_LEVEL")
            if ($h -lt 0 -or $e -le $h) { continue }
            $m = [regex]::Match($otext.Substring(0, $h), 'PRINT_REPORT\s+1\s*\r?\n\s*END_LEVEL')
            if (-not $m.Success) { continue }
            $s = $otext.IndexOf("`n", $otext.IndexOf('IMPOSSIBLE BIT!!!!', $h)) + 1
            $block = [regex]::Replace($otext.Substring($s, $e - $s), '(?m)^;', '')
            $last = [regex]::Matches($block, 'PRINT_REPORT\s+\d'); $last = $last[$last.Count - 1]
            $dst = Join-Path (Join-Path $ov (Split-Path -Leaf $gameDir)) "RUNFILES\LEVELS$i.DBS"
            New-Item -ItemType Directory -Force (Split-Path -Parent $dst) | Out-Null
            [IO.File]::WriteAllText($dst, $otext.Substring(0, $m.Index) + $block.Substring(0, $last.Index) + "PRINT_REPORT 1`r`n`r`n`tEND_LEVEL`r`n", [Text.Encoding]::GetEncoding(28591))
        }
    }
    $changed
}
# RANDOM ENEMIES (Bobby 11-01: after one play you know where everything is): a shuffled copy of the level data in the layer, as data set 4 (/D4), with
# its own hi scores. DATA\3 is never written; OFF = /D3 and no layer. Zone is on rails and ccp.dat only says which aim point starts which enemy, so no door,
# room, wall or route file changes. Side swap: the enemy takes its partner's mirrored path (player 2's). Trigger shift: it starts up to 2 aim points
# earlier or later. Pickup swap: pickups in one room trade places. Bosses, cars and flyers stay. Seed + changes: DATA\4\SHUFFLE.TXT.
function Read-MVAim([string]$file) {
    $at = @{}; $list = New-Object System.Collections.ArrayList
    foreach ($l in [IO.File]::ReadAllLines($file)) {
        $p = $l.Trim() -split '\s+'
        if ($p.Count -ge 5 -and $p[0] -match '^\d+$') {
            $pt = @{ Cmd = $p[1]; X = [int]$p[2]; Y = [int]$p[3]; Z = [int]$p[4] }
            $at[[int]$p[0]] = $pt; [void]$list.Add($pt)
        }
    }
    @{ At = $at; List = $list }
}
function Write-MVZoneShuffle([string]$gameDir, [string]$ov, [int]$seed, [double]$sideChance = 0.5, [double]$shiftChance = 0.5) {
    $src = Join-Path $gameDir 'DATA\3'
    $dst = Join-Path (Join-Path $ov (Split-Path -Leaf $gameDir)) 'DATA\4'
    New-Item -ItemType Directory -Force (Join-Path $dst 'AIM') | Out-Null
    foreach ($f in Get-ChildItem -LiteralPath $src -Recurse -File) {
        $to = Join-Path $dst $f.FullName.Substring($src.Length + 1)
        if ($f.Name -eq 'HISCORE.DAT' -and (Test-Path -LiteralPath $to)) { continue }   # this mode's own scores stay
        Copy-Item -LiteralPath $f.FullName -Destination $to -Force
    }
    $rng = New-Object Random $seed
    $log = New-Object System.Collections.ArrayList; [void]$log.Add("seed $seed")
    $latin = [Text.Encoding]::GetEncoding(28591)
    $path = @{ DROID = 'DROID'; CAR = 'CAR'; JETPACK = 'JET'; SPLITTER = 'SPLIT' }   # everyone else walks a PLAYER<id>.aim
    $fixed = 'ALIEN_BOSS', 'RAMBO', 'CAR', 'SPLITTER', 'JETPACK'
    $aimOf = { param($t, $id) $f = Join-Path $src ("AIM\{0}{1}.AIM" -f $(if ($path[$t]) { $path[$t] } else { 'PLAYER' }), $id); if (Test-Path -LiteralPath $f) { Read-MVAim $f } }
    $walk = @{ 0 = Read-MVAim (Join-Path $src 'AIM\PLAYER0.AIM'); 1 = Read-MVAim (Join-Path $src 'AIM\PLAYER1.AIM') }

    # ccp.dat: TYPE id player aimpoint az el tw a b c sx sy sz
    $lines = [IO.File]::ReadAllLines((Join-Path $src 'CCP.DAT'), $latin)
    $rows = @(); for ($i = 0; $i -lt $lines.Count; $i++) {
        $p = $lines[$i].Trim() -split '\s+'
        if ($p.Count -eq 13) { $rows += , @{ Line = $i; P = $p; Type = $p[0]; Id = [int]$p[1]; Pl = [int]$p[2]; Aim = [int]$p[3] } }
    }
    foreach ($a in @($rows | Where-Object { $_.Pl -eq 0 -and $fixed -notcontains $_.Type })) {
        $b = $rows | Where-Object { $_.Pl -eq 1 -and $_.Type -eq $a.Type -and $_.Aim -eq $a.Aim -and $_.Id -eq $a.Id + 1 } | Select-Object -First 1
        if (-not $b) { continue }
        $pa = & $aimOf $a.Type $a.Id; $pb = & $aimOf $b.Type $b.Id
        if (-not $pa -or -not $pb) { continue }
        # side swap: only true mirrors (same points, x flipped, same y)
        $mirror = $pa.List.Count -eq $pb.List.Count -and $pa.List.Count -gt 0
        if ($mirror) {
            $same = 0; for ($k = 0; $k -lt $pa.List.Count; $k++) { if ($pa.List[$k].X -eq -$pb.List[$k].X -and $pa.List[$k].Y -eq $pb.List[$k].Y) { $same++ } }
            $mirror = $same -ge 0.8 * $pa.List.Count -and $pa.List[0].X -eq -$pb.List[0].X
        }
        if ($mirror -and $rng.NextDouble() -lt $sideChance) {
            $t = $a.P[1]; $a.P[1] = $b.P[1]; $b.P[1] = $t      # id: the path, and the object slot, so still unique
            $t = $a.P[4]; $a.P[4] = $b.P[4]; $b.P[4] = $t      # az
            $t = $pa; $pa = $pb; $pb = $t
            [void]$log.Add("side $($a.Type) $($a.Id)/$($b.Id) at $($a.Aim)")
        }
        # trigger shift: both players' enemies must stay ahead, not too close (the game deletes one spawned on top of you), not much further
        if ($rng.NextDouble() -lt $shiftChance) {
            $good = @(); foreach ($d in -2, -1, 1, 2) {
                $ok = $true
                foreach ($side in @(@(0, $pa), @(1, $pb))) {
                    $s = $side[1].List[0]; $old = $walk[$side[0]].At[$a.Aim]; $new = $walk[$side[0]].At[$a.Aim + $d]
                    if (-not $old -or -not $new -or $old.Cmd -ne 'WALK' -or $new.Cmd -ne 'WALK' -or [Math]::Abs($new.Z - $old.Z) -ge 1000 -or $s.Y -le $new.Y) { $ok = $false; break }
                    $dn = [Math]::Sqrt([Math]::Pow($s.X - $new.X, 2) + [Math]::Pow($s.Y - $new.Y, 2) + [Math]::Pow($s.Z - $new.Z, 2))
                    $do = [Math]::Sqrt([Math]::Pow($s.X - $old.X, 2) + [Math]::Pow($s.Y - $old.Y, 2) + [Math]::Pow($s.Z - $old.Z, 2))
                    if ($dn -lt 8000 -or $dn -gt $do + 25000) { $ok = $false; break }
                }
                if ($ok) { $good += $d }
            }
            if ($good.Count) {
                $d = $good[$rng.Next($good.Count)]
                $a.P[3] = "$($a.Aim + $d)"; $b.P[3] = "$($b.Aim + $d)"
                [void]$log.Add("shift $($a.Type) $($a.Aim) by $d")
            }
        }
    }
    foreach ($r in $rows) { $lines[$r.Line] = $r.P -join "`t" }
    [IO.File]::WriteAllLines((Join-Path $dst 'CCP.DAT'), $lines, $latin)

    # powerup.dat: room player type x y z az. Same room, same player, about the same height (ledge pickups stay on ledges)
    $lines = [IO.File]::ReadAllLines((Join-Path $src 'POWERUP.DAT'), $latin)
    $ups = @(); for ($i = 0; $i -lt $lines.Count; $i++) { $p = $lines[$i].Trim() -split '\s+'; if ($p.Count -eq 7) { $ups += , @{ Line = $i; P = $p } } }
    foreach ($g in @($ups | Group-Object { "$($_.P[0]) $($_.P[1]) $([Math]::Round([int]$_.P[5] / 2000))" } | Where-Object { $_.Count -gt 1 })) {
        $m = @($g.Group); for ($i = $m.Count - 1; $i -gt 0; $i--) {
            $j = $rng.Next($i + 1); foreach ($k in 3, 4, 5, 6) { $t = $m[$i].P[$k]; $m[$i].P[$k] = $m[$j].P[$k]; $m[$j].P[$k] = $t }
        }
    }
    foreach ($u in $ups) { $lines[$u.Line] = $u.P -join "`t" }
    [IO.File]::WriteAllLines((Join-Path $dst 'POWERUP.DAT'), $lines, $latin)
    [IO.File]::WriteAllLines((Join-Path $dst 'SHUFFLE.TXT'), [string[]]$log, $latin)
}
# (the online string, 'k1:v1,k2:v2', is made by the hall: MallLauncher.SyncOptions)
