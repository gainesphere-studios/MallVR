#!/bin/bash
# MallVR Quest: make H:\Android\src\mallvr-dbx = stock DOSBox-X 2026.10.01 (our tree's base) + our changes, from the master
# sources in dosbox_src/ (never edits them):
#   - the 7 MallVR lines in dosbox.cpp / sdlmain.cpp (SU2000_AddConfigSection, VPIX_Init, SU2000_Init)
#   - vpix*.cpp / vpix*.h (WITHOUT vpix_testbot.h: it never ships), su2000/, openvr.h (types only on Android)
#   - vpix_render.cpp with the OpenXR hooks (apply_xr_hooks.js)
# Run before build_mallvr_android.sh; safe to re-run.
set -e
HERE="$(cd "$(dirname "$0")/.." && pwd)"; T=/h/Android/src/mallvr-dbx; S=/h/Android/src/dosbox-x
[ -d "$T" ] || { cp -r "$S" "$T"; rm -rf "$T/.git"; }
cp "$S/src/dosbox.cpp" "$T/src/dosbox.cpp"; cp "$S/src/gui/sdlmain.cpp" "$T/src/gui/sdlmain.cpp"
node - "$T" <<'EOF'
const fs = require('fs'), T = process.argv[2];
function patch(f, pairs) {
    let s = fs.readFileSync(f, 'utf8');
    const crlf = s.includes('\r\n'); s = s.replace(/\r\n/g, '\n');
    for (const [a, b] of pairs) { if (s.split(a).length !== 2) { console.error('anchor not unique in ' + f + ': ' + a); process.exit(1); } s = s.replace(a, () => b); }
    fs.writeFileSync(f, crlf ? s.replace(/\n/g, '\r\n') : s);
}
patch(T + '/src/dosbox.cpp', [
    ['void                DISNEY_Init(Section*);\n', 'void                DISNEY_Init(Section*);\nvoid                SU2000_AddConfigSection(Config *conf);   /* MallVR: LeonG-ZA SU2000 card (reference) */\n'],
    ['    secprop = control->AddSection_prop("innova",&Null_Init,true);//done', '    SU2000_AddConfigSection(control);\n\n    secprop = control->AddSection_prop("innova",&Null_Init,true);//done']]);
patch(T + '/src/gui/sdlmain.cpp', [
    ['void DISNEY_Init();\n', 'void DISNEY_Init();\nvoid VPIX_Init();\nvoid SU2000_Init();\n'],
    ['        DISNEY_Init();\n        GUS_Init();', '        DISNEY_Init();\n        VPIX_Init();\n        SU2000_Init();\n        GUS_Init();']]);
EOF
hw="$T/src/hardware"; mkdir -p "$hw/su2000"
cp "$HERE"/dosbox_src/vpix*.cpp "$HERE"/dosbox_src/vpix*.h "$hw/"; rm -f "$hw/vpix_testbot.h"
cp "$HERE"/dosbox_src/su2000/* "$hw/su2000/"
cp /h/Games/Virtuality/_work/src/src/hardware/openvr.h "$hw/"
# Windows "\" in host paths the engine builds itself (Zone Hunter's CD tracks, SU3000 movies, Buggy Ball's data files): "/" on
# Android. Copies only; a missing anchor (master edited) is a warning, the game still builds.
node - "$hw" <<'EOF'
const fs = require('fs'), d = process.argv[2];
function fix(f, pairs) {
    let s = fs.readFileSync(d + '/' + f, 'utf8');
    for (const [a, b] of pairs) { if (!s.includes(a)) { console.error('WARNING path fix: anchor gone in ' + f + ': ' + a); continue; } s = s.split(a).join(b); }
    fs.writeFileSync(d + '/' + f, s);
}
fix('vpix_hle.cpp', [['"%s\\\\TRACK%02u.WAV"', '"%s/TRACK%02u.WAV"'], ['"%s\\\\TRACK%02u.MP3"', '"%s/TRACK%02u.MP3"'],
                     ['std::string path = std::string(dir) + "\\\\" + dosname;', 'std::string path = std::string(dir) + "/" + dosname; for (char &c : path) if (c == \'\\\\\') c = \'/\';']]);
fix('vpix_buggy.h', [['std::string p = dir + "\\\\" + rel;', 'std::string p = dir + "/" + rel; for (char &c : p) if (c == \'\\\\\') c = \'/\';']]);
EOF
node "$(cygpath -w "$HERE/quest/apply_xr_hooks.js")" "$(cygpath -w "$HERE/dosbox_src/vpix_render.cpp")" "$(cygpath -w "$hw/vpix_render.cpp")"
echo "mallvr-dbx synced"
