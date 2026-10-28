# Finds the newest MallVR release. Dot-sourced by play.ps1 and update.ps1.
#  1. Google Drive: MallVR_latest.txt in the release folder (its file ID is "drive" in version.txt).
#     It names the version, the Drive file ID of the Loader zip and the zip's SHA1.
#  2. GitHub Releases ("repo" in version.txt), as the fallback.
# MALLVR_DRIVE_URL (testing only) replaces the Drive download URL, {0} = file ID.

function Get-MVVersion([string]$s) { [version](($s -replace '^[vV]', '') -replace '[^0-9.].*$', '') }

function Get-MVHead([string]$Path) {
    $fs = [IO.File]::OpenRead($Path)
    try { $b = New-Object byte[] 512; $n = $fs.Read($b, 0, 512); [Text.Encoding]::ASCII.GetString($b, 0, $n) }
    finally { $fs.Close() }
}

# Download a shared Drive file to $Out. Files Drive can't virus-scan (over ~100 MB) come back as a
# warning page first; follow its download form once.
function Save-MVDriveFile([string]$Id, [string]$Out, [int]$TimeoutSec) {
    $ProgressPreference = 'SilentlyContinue'      # the PS 5.1 progress bar slows downloads 10x
    [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
    if ($env:MALLVR_DRIVE_URL) { $url = $env:MALLVR_DRIVE_URL -f $Id }
    else { $url = "https://drive.usercontent.google.com/download?id=$Id&export=download" }
    for ($i = 0; $i -lt 2; $i++) {
        Invoke-WebRequest -UseBasicParsing -TimeoutSec $TimeoutSec -Uri $url -OutFile $Out -UserAgent 'MallVR-updater'
        if ((Get-MVHead $Out) -notmatch '^\s*<(!DOCTYPE|html)') { return }
        $html = Get-Content -LiteralPath $Out -Raw
        $form = [regex]::Match($html, '<form[^>]*id="download-form"[^>]*action="([^"]+)"')
        if (-not $form.Success) { break }
        $q = [regex]::Matches($html, '<input type="hidden" name="([^"]+)" value="([^"]*)"') |
            ForEach-Object { "$($_.Groups[1].Value)=$([uri]::EscapeDataString([Net.WebUtility]::HtmlDecode($_.Groups[2].Value)))" }
        $url = "$([Net.WebUtility]::HtmlDecode($form.Groups[1].Value))?$($q -join '&')"
    }
    throw 'Google Drive sent a web page instead of the file (is it shared "Anyone with the link"?)'
}

# The newest release as {Version, Name, Notes, Sha1, Sig, Size, Source, DriveId, Url}; $null if no update
# source is set. Throws if every source that is set failed.
function Get-MVLatest($Cfg, [int]$TimeoutSec = 10) {
    $errors = @()
    # gainesphere.com first: "url" in version.txt is the pointer file (same key = value lines as the Drive one,
    # but with loader_url = the https address of the Loader zip). Any failure falls through to Drive, then GitHub.
    if ($Cfg.url -and $Cfg.url -ne 'none') {
        try {
            [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
            $txt = (Invoke-WebRequest -UseBasicParsing -TimeoutSec $TimeoutSec -Uri $Cfg.url -UserAgent 'MallVR-updater').Content
            if ($txt -is [byte[]]) { $txt = [Text.Encoding]::UTF8.GetString($txt) }
            $d = @{}
            foreach ($line in ($txt -split "`r?`n")) {
                if ($line -match '^\s*([A-Za-z0-9_]+)\s*=\s*(.*?)\s*$') { $d[$Matches[1]] = $Matches[2] -replace '\\n', "`n" }
            }
            if (-not $d.version -or -not $d.loader_url) { throw 'pointer has no version or loader_url' }
            if ($d.loader_url -notmatch '^https://') { throw 'loader_url must be https' }
            $name = if ($d.loader_name) { $d.loader_name } else { "MallVR_$($d.version)_Loader.zip" }
            return [pscustomobject]@{ Version = $d.version; Name = $name; Notes = $d.notes; Sha1 = $d.sha1; Sig = $d.sig
                Size = [long]$d.size; Source = 'gainesphere.com'; DriveId = $null; Url = $d.loader_url }
        }
        catch { $errors += "gainesphere.com: $($_.Exception.Message)" }
    }
    if ($Cfg.drive -and $Cfg.drive -ne 'none') {
        $tmp = [IO.Path]::GetTempFileName()
        try {
            Save-MVDriveFile $Cfg.drive $tmp $TimeoutSec
            $d = @{}                              # key = value lines; \n in a value is a line break
            foreach ($line in Get-Content -LiteralPath $tmp) {
                if ($line -match '^\s*([A-Za-z0-9_]+)\s*=\s*(.*?)\s*$') { $d[$Matches[1]] = $Matches[2] -replace '\\n', "`n" }
            }
            if (-not $d.version -or -not $d.loader_id) { throw 'MallVR_latest.txt has no version or loader_id' }
            $name = if ($d.loader_name) { $d.loader_name } else { "MallVR_$($d.version)_Loader.zip" }
            return [pscustomobject]@{ Version = $d.version; Name = $name; Notes = $d.notes; Sha1 = $d.sha1; Sig = $d.sig
                Size = [long]$d.size; Source = 'Google Drive'; DriveId = $d.loader_id; Url = $null }
        }
        catch { $errors += "Google Drive: $($_.Exception.Message)" }
        finally { Remove-Item -LiteralPath $tmp -ErrorAction SilentlyContinue }
    }
    if ($Cfg.repo -and $Cfg.repo -ne 'none') {
        try {
            [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
            $rel = Invoke-RestMethod -UseBasicParsing -TimeoutSec $TimeoutSec "https://api.github.com/repos/$($Cfg.repo)/releases/latest" -Headers @{ 'User-Agent' = 'MallVR-updater' }
            $asset = $rel.assets | Where-Object { $_.name -like '*_Loader.zip' } | Select-Object -First 1
            if (-not $asset) { throw "release $($rel.tag_name) has no Loader zip attached" }
            # its signature: the <zip>.sig asset sign_release.ps1 wrote (no .sig = unsigned = Save-MVRelease refuses it)
            $sigAsset = $rel.assets | Where-Object { $_.name -eq "$($asset.name).sig" } | Select-Object -First 1
            $sig = $null
            if ($sigAsset) {
                $st = (Invoke-WebRequest -UseBasicParsing -TimeoutSec $TimeoutSec -Uri $sigAsset.browser_download_url -UserAgent 'MallVR-updater').Content
                if ($st -is [byte[]]) { $st = [Text.Encoding]::ASCII.GetString($st) }
                $sd = Read-MVSig $st; if ($sd.version -eq $rel.tag_name) { $sig = $sd.sig }
            }
            return [pscustomobject]@{ Version = $rel.tag_name; Name = $asset.name; Notes = $rel.body; Sha1 = $null; Sig = $sig
                Size = [long]$asset.size; Source = 'GitHub'; DriveId = $null; Url = $asset.browser_download_url }
        }
        catch { $errors += "GitHub: $($_.Exception.Message)" }
    }
    if ($errors) { throw ($errors -join ' / ') }
    $null
}

# Download a release found by Get-MVLatest to $Out and check its SHA1 when one is published, then its signature.
function Save-MVRelease($Latest, [string]$Out) {
    $ProgressPreference = 'SilentlyContinue'
    if ($Latest.DriveId) { Save-MVDriveFile $Latest.DriveId $Out 60 }
    else { Invoke-WebRequest -UseBasicParsing -Uri $Latest.Url -OutFile $Out -UserAgent 'MallVR-updater' }
    if ((Get-MVHead $Out) -notmatch '^PK') { throw 'the download is not a zip file' }
    if ($Latest.Sha1 -and (Get-FileHash -LiteralPath $Out -Algorithm SHA1).Hash -ne $Latest.Sha1.Trim().ToUpper()) {
        throw 'the download is damaged (SHA1 does not match) - try again'
    }
    Test-MVSignature $Out $Latest.Version $Latest.Sig
}

# Signed releases (security review 2026-10-26): only a zip signed with Gainesphere's release key installs. The key's public half is
# release_key.txt next to this file; the private half never leaves Bobby's PC (package\sign_release.ps1). Signed text:
# "MALLVR-RELEASE 1|<version>|<SHA-256 of the zip>", so an old signed zip can't pose as a newer version. Throws if it doesn't check out.
function Test-MVSignature([string]$Zip, [string]$Version, [string]$Sig) {
    $keyFile = Join-Path $PSScriptRoot 'release_key.txt'
    if (-not (Test-Path -LiteralPath $keyFile)) { throw 'this copy has no release key (engine\release_key.txt), so it cannot check updates' }
    if (-not $Sig) { throw "the update $Version is not signed by Gainesphere - not installed" }
    $sha = (Get-FileHash -LiteralPath $Zip -Algorithm SHA256).Hash.ToUpper()
    $msg = [Text.Encoding]::ASCII.GetBytes("MALLVR-RELEASE 1|$Version|$sha")
    $rsa = New-Object System.Security.Cryptography.RSACryptoServiceProvider
    $rsa.PersistKeyInCsp = $false
    $rsa.FromXmlString((Get-Content -LiteralPath $keyFile -Raw))
    $ok = $false
    try { $ok = $rsa.VerifyData($msg, 'SHA256', [Convert]::FromBase64String($Sig.Trim())) } catch { }
    if (-not $ok) { throw "the update $Version does not carry Gainesphere's signature - not installed (someone may have changed it)" }
}

# key = value lines of a .sig file (sign_release.ps1 writes one next to each signed zip)
function Read-MVSig([string]$Text) {
    $d = @{}
    foreach ($line in ($Text -split "`r?`n")) { if ($line -match '^\s*([A-Za-z0-9_]+)\s*=\s*(.*?)\s*$') { $d[$Matches[1]] = $Matches[2] } }
    $d
}
