@echo off
rem Run first by every MallVR .bat. MallVR's scripts need Windows PowerShell 5.1 (built into Windows 10 and 11).
rem Windows 7 and 8.1 come with an older PowerShell that stops with odd errors (DevilMaster 10-22: Split-Path in check.ps1).
powershell -NoProfile -Command "if ($PSVersionTable.PSVersion.Major -ge 5) { exit 0 } else { exit 1 }" >nul 2>&1
if not errorlevel 1 exit /b 0
echo.
echo  MALLVR NEEDS WINDOWS POWERSHELL 5.1
echo.
echo  This PC has an older PowerShell (Windows 7 and 8.1 come with one). It is a free
echo  update from Microsoft, Windows Management Framework 5.1:
echo.
echo   1. Windows 7 needs Service Pack 1 (Start, right-click Computer, Properties).
echo   2. Install .NET Framework 4.8 (offline installer):
echo      https://dotnet.microsoft.com/download/dotnet-framework/net48
echo   3. Install Windows Management Framework 5.1:
echo      https://www.microsoft.com/download/details.aspx?id=54616
echo      Windows 7 64-bit: Win7AndW2K8R2-KB3191566-x64.zip, unzip it and run
echo      Install-WMF5.1.ps1 (or the .msu file inside). Windows 8.1: Win8.1AndW2K12R2-KB3191564-x64.msu
echo   4. Restart the PC, then start MallVR again.
echo.
echo  No internet on that PC? Download both on another PC and bring them on a USB stick.
echo.
pause
exit /b 1
