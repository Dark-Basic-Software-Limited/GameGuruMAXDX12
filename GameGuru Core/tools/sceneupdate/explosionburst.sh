#!/bin/bash
# Capture the barrel explosion at FRAME rate, not at harness round-trip rate.
#
# The single-SCREENSHOT approach missed it entirely: each round-trip costs ~1s, the plume lasts well
# under that, and the frames landed after the blast (barrels already gone, camera thrown at the
# ceiling). BURST_FRAMES writes N CONSECUTIVE rendered frames to Files/screenshots/frame_###.png,
# so at ~215 FPS a 120-frame burst covers ~0.55s of the plume with no gaps.
#
# Order matters: fire FIRST, then arm the burst on the very next command. The blast needs a moment
# to develop (bullet -> barrel destroy -> explosion spawn), so this lands the window inside it.
set -u
D="/d/DEV/BUILD/GameGuru Wicked MAX Build Area/Max"
# ⚠ Write to burstshots/latest, and wipe ONLY that. burstshots/ itself holds TRACKED reference
# captures (keep/ = the 2.43 before-fix frames, after_2.44/ = the fixed ones); an earlier version
# did `rm -rf burstshots` at startup and silently deleted every one of them.
OUT="$(dirname "$0")"; SH="$OUT/burstshots/latest"; rm -rf "$SH"; mkdir -p "$SH"
LOG="$SH/burst.log"
PROJ="${1:-testpro2}"; N="${2:-120}"

LOCK="$OUT/.explosionburst.lock"
if [ -e "$LOCK" ] && kill -0 "$(cat "$LOCK" 2>/dev/null)" 2>/dev/null; then
  echo "REFUSING TO START: already running as PID $(cat "$LOCK")."; exit 3
fi
echo $$ > "$LOCK"; trap 'rm -f "$LOCK"' EXIT INT TERM
: > "$LOG"
say() { echo "$(date +%H:%M:%S) $*" >> "$LOG"; echo "$(date +%H:%M:%S) $*" >&2; }
send() { rm -f "$D/auto_result.txt"; echo "$1" > "$D/auto_command.txt"; local t=0
  while [ $t -lt ${2:-40} ]; do if [ -f "$D/auto_result.txt" ]; then sleep 0.3; cat "$D/auto_result.txt"; return 0; fi; sleep 1; t=$((t+1)); done; echo TIMEOUT; }
alive() { tasklist.exe //FI "IMAGENAME eq GameGuruMAX.exe" 2>/dev/null | grep -qi GameGuruMAX; }
wait_state() { local t=0; while [ $t -lt $2 ]; do alive || return 2
  [ "$(send "GET_STATE" 20|head -1)" == "STATE: $1" ] && return 0; sleep 5; t=$((t+7)); done; return 1; }

say "=== $PROJ burst explosion capture (${N} frames) ==="
taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true; sleep 4
rm -f "$D/auto_command.txt" "$D/auto_result.txt"
rm -f "$D/Files/Files/screenshots/frame_"*.png
( cd "$D" && ./GameGuruMAX.exe >/dev/null 2>&1 & )
sleep 15
wait_state "hub" 90 || { say "FAIL_HUB"; exit 1; }
sleep 4
say "open: $(send "OPEN_PROJECT $PROJ" 60)"
sleep 6
send "CLICK_ONLY_LEVEL" 120 >/dev/null; sleep 45
wait_state "editor" 240 || say "WARN not editor"
sleep 15
say "test game: $(send "CLICK test_level" 120)"
sleep 40
wait_state "game" 240 || { say "FAIL never reached game"; exit 1; }
sleep 12

say "fire: $(send "FIRE_WEAPON 3" 30)"
say "burst: $(send "BURST_FRAMES $N" 60)"
sleep 20

# ⚠ BURST_FRAMES writes "Files/screenshots/frame_###.png" RELATIVE TO THE GAME'S CWD, and the
# game's CWD is Max/Files — so the frames land in Max/Files/Files/screenshots/, NOT
# Max/Files/screenshots/. The memory rule "runtime fopen files land in the game's Files/ CWD"
# exists precisely for this and I still looked in the wrong folder first (reported 0 frames).
FRAMEDIR="$D/Files/Files/screenshots"
CNT=$(ls "$FRAMEDIR/frame_"*.png 2>/dev/null | wc -l)
say "captured $CNT consecutive frames"
cp "$FRAMEDIR/frame_"*.png "$SH/" 2>/dev/null
say "DONE -> $SH"
taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true
