# Check My Games: looks through ..\games for each supported game and says whether it will run.
$Engine = $PSScriptRoot
$Root = Split-Path -Parent $Engine
. (Join-Path $Engine 'games.ps1')
$gamesDir = Join-Path $Root 'games'

function Sha1($path) { (Get-FileHash -LiteralPath $path -Algorithm SHA1).Hash.ToLower() }

Write-Host ''
Write-Host '  MALLVR by Gainesphere Studios - game check' -ForegroundColor Cyan
Write-Host "  Looking in: $gamesDir"
Write-Host ''
$all = Get-ChildItem -LiteralPath $gamesDir -Recurse -File -ErrorAction SilentlyContinue
$ready = 0
foreach ($code in $Games.Keys) {
    $info = $Games[$code]
    $label = $info.Name.PadRight(20)
    if ($info.Native) {
        $d = Get-ChildItem -LiteralPath $gamesDir -Recurse -Directory -Filter $info.DataDir -ErrorAction SilentlyContinue |
            Where-Object { Test-Path -LiteralPath (Join-Path $_.FullName $info.DataFile) } | Select-Object -First 1
        if ($d) { Write-Host "  [  READY  ] $label rebuilt by MallVR, data folder found" -ForegroundColor Green; $ready++ }
        else { Write-Host "  [ MISSING ] $label $($info.DataDir)\$($info.DataFile) not found" -ForegroundColor DarkGray }
        continue
    }
    $en = $(if ($info.Exe) { $info.Exe } else { $code })   # SU3000 games keep their own EXE name (GUN3K = GUN\GUN.EXE)
    $cands = @($all | Where-Object { $_.Name -ieq "$en.EXE" -and $_.Directory.Name -ieq $en })
    # GUN.EXE is Olin Shotgun or Total Recoil: an SU3000 entry takes only its own SHA1, an SU2000 one skips SU3000 copies
    $su3kSha = @($Games.Values | Where-Object { $_.Su3k -and $_.Exe -ieq $en } | ForEach-Object { $_.Sha1 })
    $exe = $cands | Where-Object { $h = Sha1 $_.FullName; if ($info.Su3k) { $h -eq $info.Sha1 } else { $su3kSha -notcontains $h } } | Select-Object -First 1
    if (-not $exe) { Write-Host "  [ MISSING ] $label $en\$en.EXE not found" -ForegroundColor DarkGray; continue }
    $ok = (Sha1 $exe.FullName) -eq $info.Sha1
    $note = if ($ok) { 'verified' } else { 'different version - may not work' }
    if ($info.Cd) {
        $cue = $all | Where-Object { $_.Name -ieq $info.Cd } | Select-Object -First 1
        $bin = $all | Where-Object { $_.Name -ieq $info.CdBin } | Select-Object -First 1
        if (-not $cue -or -not $bin) {
            Write-Host "  [ NEEDS CD] $label game found, but the CD image ($($info.Cd) + $($info.CdBin)) is missing" -ForegroundColor Yellow
            continue
        }
        if ((Sha1 $bin.FullName) -ne $info.CdSha1) { $ok = $false; $note += ', CD image is a different version' }
    }
    if ($ok) { Write-Host "  [  READY  ] $label $note" -ForegroundColor Green; $ready++ }
    else     { Write-Host "  [  READY? ] $label $note" -ForegroundColor Yellow; $ready++ }
}
Write-Host ''
Write-Host "  $ready of $($Games.Count) games ready."
if ($ready -lt $Games.Count) {
    Write-Host ''
    Write-Host '  Missing games? The easy way: open MallVR_Launcher.bat and press GET GAMES' -ForegroundColor Cyan
    Write-Host '  (bottom of the screen). It downloads the game files from archive.org and' -ForegroundColor Cyan
    Write-Host '  puts them in the right place for you.' -ForegroundColor Cyan
    Write-Host '  By hand instead: docs\SETUP_GUIDE.txt, step 2.'
}
Write-Host ''
