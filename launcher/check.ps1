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
    $exe = $all | Where-Object { $_.Name -ieq "$code.EXE" -and $_.Directory.Name -ieq $code } | Select-Object -First 1
    if (-not $exe) { Write-Host "  [ MISSING ] $label $code\$code.EXE not found" -ForegroundColor DarkGray; continue }
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
if ($ready -eq 0) {
    Write-Host ''
    Write-Host '  Copy your Virtuality SU2000 HDD dump into the games folder (the folder that'
    Write-Host '  holds DAC, DN2, PAC, ZONE ...). Pac-Man VR also needs PACMAN.CUE + PACMAN.BIN.'
    Write-Host '  See docs\SETUP_GUIDE.txt.'
}
Write-Host ''
