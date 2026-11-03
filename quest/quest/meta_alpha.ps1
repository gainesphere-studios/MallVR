# MallVR Quest -> Meta Horizon Store ALPHA channel (app 1456219684238039). Bobby runs this himself: it asks for the app secret
# (Developer Dashboard > MallVR > Development > API), which never goes into a file or the repo.
#   powershell -ExecutionPolicy Bypass -File quest\meta_alpha.ps1             validate only (Meta's server checks, nothing uploaded)
#   powershell -ExecutionPolicy Bypass -File quest\meta_alpha.ps1 -Upload     upload to ALPHA (after the validate passes)
param([switch]$Upload, [string]$Version = '0.5.0')
$ErrorActionPreference = 'Stop'
$util = 'H:\Android\dl\ovr-platform-util\ovr-platform-util.exe'
$apk = "H:\Android\release\$Version\MallVR-Quest-$Version-store.apk"
if (-not (Test-Path $apk)) { throw "no $apk (bash quest/build_mallvr_apk.sh --store)" }
$s = Read-Host 'MallVR app secret' -AsSecureString
$secret = [Runtime.InteropServices.Marshal]::PtrToStringBSTR([Runtime.InteropServices.Marshal]::SecureStringToBSTR($s))
$a = @('upload-quest-build', '--app-id', '1456219684238039', '--app-secret', $secret, '--age-group', 'TEENS_AND_ADULTS',
    '--apk', $apk, '--channel', 'ALPHA', '--disable-progress-bar',
    '--notes', "MallVR $Version`: the LOST GAMES: 12 Virtuality games nobody has found, rebuilt from footage as playable placeholders, with online play. Online with PC v0.15.0 or newer.")
if (-not $Upload) { $a += '--validate' }
& $util @a
"exit code $LASTEXITCODE"
