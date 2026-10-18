# Update: checks for a newer MallVR (Google Drive first, GitHub as fallback) and installs it in place.
# Replaces engine\, docs\, source\, the .bat launchers and README.txt. Never touches
# games\, settings\ or logs\ (your game files, gun trim and high scores stay put).
#   update.ps1          check and ask before installing
#   update.ps1 -Auto    install without asking (the launcher uses this after you said yes)
#   update.ps1 <zip>    install a Loader zip you downloaded yourself (drag it onto Update.bat)
param([string]$Zip, [switch]$Auto)
$ErrorActionPreference = 'Stop'
$Engine = $PSScriptRoot
$Root = Split-Path -Parent $Engine
. (Join-Path $Engine 'findupdate.ps1')
$cfg = Get-Content -LiteralPath (Join-Path $Engine 'version.txt') | ConvertFrom-StringData
function Done($code) { if ($Auto) { if ($code) { Read-Host '  Press Enter to close' } else { Start-Sleep 2 } }; exit $code }

Write-Host ''
Write-Host "  MALLVR by Gainesphere Studios - update check   (installed: $($cfg.version))" -ForegroundColor Cyan
if (-not $Zip) {
    Write-Host '  Checking for a newer version...'
    try { $latest = Get-MVLatest $cfg 15 }
    catch { Write-Host "  Could not check for updates: $($_.Exception.Message)" -ForegroundColor Yellow; Done 1 }
    if (-not $latest) { Write-Host '  No update source set in engine\version.txt.'; Done 0 }
    if ((Get-MVVersion $latest.Version) -le (Get-MVVersion $cfg.version)) {
        Write-Host "  You have the latest version ($($cfg.version))." -ForegroundColor Green; Done 0
    }
    Write-Host "  New version: $($latest.Version)  (from $($latest.Source))" -ForegroundColor Green
    if ($latest.Notes) { Write-Host ''; Write-Host ($latest.Notes -replace '(?m)^', '    '); Write-Host '' }
    if (-not $Auto -and (Read-Host '  Install it now? (y/n)') -notmatch '^[yY]') { exit 0 }
}
if (Get-Process dosbox-x -ErrorAction SilentlyContinue | Where-Object { $_.Path -like "$Engine*" }) {
    Write-Host '  Close the running game first, then run Update again.' -ForegroundColor Yellow; Done 1
}

$tmp = Join-Path $env:TEMP "MallVR_update_$([guid]::NewGuid().ToString('N'))"
New-Item -ItemType Directory $tmp | Out-Null
try {
    if ($Zip) { $zip = (Resolve-Path -LiteralPath $Zip).Path; $tag = [IO.Path]::GetFileNameWithoutExtension($zip) }
    else {
        $zip = Join-Path $tmp ($latest.Name -replace '[^\w.\-]', '_'); $tag = $latest.Version
        if ($zip -notlike '*.zip') { $zip += '.zip' }
        $mb = if ($latest.Size) { " ($([math]::Round($latest.Size / 1MB, 1)) MB)" } else { '' }
        Write-Host "  Downloading $($latest.Name)$mb..."
        Save-MVRelease $latest $zip
    }
    Expand-Archive -LiteralPath $zip -DestinationPath (Join-Path $tmp 'x')
    $new = Get-ChildItem -LiteralPath (Join-Path $tmp 'x') -Directory | Select-Object -First 1 -ExpandProperty FullName
    if (-not $new -or -not (Test-Path (Join-Path $new 'engine\dosbox-x.exe'))) { throw 'download does not look like a MallVR package' }
    # this script lives in engine\ - copy everything else first, engine last
    foreach ($d in 'docs', 'source') {
        if (Test-Path (Join-Path $Root $d)) { Remove-Item -Recurse -Force (Join-Path $Root $d) }
        Copy-Item -Recurse (Join-Path $new $d) $Root
    }
    Get-ChildItem -LiteralPath $new -File | Copy-Item -Destination $Root -Force
    Get-ChildItem -LiteralPath (Join-Path $Root 'engine\hooks') -File | Remove-Item -Force
    Copy-Item -Recurse -Force (Join-Path $new 'engine\*') (Join-Path $Root 'engine')
    Write-Host "  Updated to $tag. Your games and settings were not touched." -ForegroundColor Green
}
catch { Write-Host "  Update failed: $($_.Exception.Message)" -ForegroundColor Yellow; Write-Host '  Nothing was changed if it failed before copying. Try again, or drag a downloaded Loader zip onto Update.bat.'; Done 1 }
finally { Remove-Item -Recurse -Force $tmp -ErrorAction SilentlyContinue }
Done 0
