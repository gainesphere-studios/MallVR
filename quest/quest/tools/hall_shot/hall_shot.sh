#!/bin/bash
# Draw the Quest launcher's Cyberspace Hall on the PC (no headset, no emulator): HallPainter + HallData compiled against the small
# Java2D stand-in for android.graphics in this folder. Fonts and anti-aliasing are Java2D's, not the Quest's; the layout is the same.
#   bash quest/tools/hall_shot/hall_shot.sh <out dir>
# Writes hall.png (3x the 352x288 grid, like the PC's launcher.ps1 -Shot), wide.png (a 1280x720 panel, mouse over a game),
# wanted.png, machine.png (SU3000 sheet + CyberMind), pac.png (a game that needs its CD), nofiles.png (before All files access),
# store.png (the store build: no GET GAMES).
set -e
H="$(cd "$(dirname "$0")" && pwd)"; Q="$(dirname "$(dirname "$H")")"; P="$(dirname "$Q")"
out="${1:?out dir}"; mkdir -p "$out"; J=/h/Android/jdk17/bin; tmp=$(mktemp -d)
S=$Q/mallvr_apk/java/com/gainesphere/mallvr
# LauncherActivity is Android-only: hallName() comes from a copy of just that function
mkdir -p "$tmp/src"; cp "$S/HallPainter.java" "$S/HallData.java" "$H"/android/graphics/*.java "$H"/com/gainesphere/mallvr/*.java "$H/HallShot.java" "$tmp/src/"
{ echo "package com.gainesphere.mallvr; class LauncherActivity {"; sed -n '/static String hallName/,/^    }/p' "$S/LauncherActivity.java"; echo "}"; } > "$tmp/src/LauncherActivity.java"
# (javac args: no apostrophe paths)
"$J/javac.exe" -nowarn -encoding UTF-8 -d "$(cygpath -w "$tmp/cls")" $(cygpath -w "$tmp"/src/*.java)
hall="$(cygpath -w "$P/package/engine/hall")"
run() { "$J/java.exe" -cp "$(cygpath -w "$tmp/cls")" HallShot "$hall" "$(cygpath -w "$out/$1")" "${@:2}"; }
run hall.png 1056 864; run wide.png 1280 720 hover; run wanted.png 1056 864 wanted; run machine.png 1056 864 machine
run pac.png 1056 864 pac; run nofiles.png 1056 864 nofiles; run store.png 1056 864 store; run alarm.png 1056 864 alarm
run card1.png 1056 864 nofiles card1; run card2.png 1056 864 card2; run card2_store.png 1056 864 card2 store
rm -rf "$tmp"; ls -la "$out"
