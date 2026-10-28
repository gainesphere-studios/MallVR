# MallVR OpenXR test build (dev, PC): same sources as build.ps1, built in its OWN tree so it never touches the normal build.
#   quest\build_xr.ps1   -> H:\Games\Virtuality\_work\src_xr\bin\x64\Release\dosbox-x.exe (+ openxr_loader.dll)
# vpix_render.cpp gets the OpenXR hooks from apply_xr_hooks.js (copy only; the master file is unchanged). Run a game with
# VPIX_XR=1 (Play_XR_<game>.bat / run_game.sh with VPIX_EXE_DIR=that bin); without VPIX_XR it is the normal OpenVR build.
$here = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$tree = "H:\Games\Virtuality\_work\src_xr"
$hw = "$tree\src\hardware"
$bin = "$tree\bin\x64\Release"
Copy-Item "$here\dosbox_src\vpix*.cpp","$here\dosbox_src\vpix*.h" $hw -Force
New-Item -ItemType Directory -Force "$hw\su2000","$hw\openxr",$bin | Out-Null
Copy-Item "$here\dosbox_src\su2000\*" "$hw\su2000" -Force
Copy-Item "H:\Games\Virtuality\_work\openxr\include\openxr\*.h" "$hw\openxr" -Force
& node "$here\quest\apply_xr_hooks.js" "$here\dosbox_src\vpix_render.cpp" "$hw\vpix_render.cpp"
if ($LASTEXITCODE -ne 0) { "hook anchors no longer match vpix_render.cpp: update quest\apply_xr_hooks.js"; exit 1 }
Get-ChildItem "$hw\vpix*.cpp","$hw\vpix*.h","$hw\su2000\*" | ForEach-Object { $_.LastWriteTime = Get-Date }
Copy-Item "H:\Games\Virtuality\_work\openvr\bin\win64\openvr_api.dll" $bin -Force
Copy-Item "H:\Games\Virtuality\_work\openxr\x64\bin\openxr_loader.dll" $bin -Force
Copy-Item "$here\dosbox_src\mallvr_*.json" $bin -Force
$msb = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe"
Push-Location "$tree\vs"
& $msb dosbox-x.vcxproj /m /p:Configuration=Release /p:Platform=x64 /p:PlatformToolset=v143 /p:WindowsTargetPlatformVersion=10.0 /v:minimal /nologo 2>&1 | Select-String -Pattern " error |vpix.*\(.*warning|dosbox-x.vcxproj ->"
Pop-Location
