#!/bin/bash
# Capture the scene-population counts + the SU-Hierarchy load-balance readout on one demo.
# The counts use the SAME labels the DX11 editor's performance panel prints
# (GameGuruMAX/GameGuru/Source/M-GridEdit.cpp:11840-11841 "Scene Transforms" / "Scene Hierarchy"),
# so a DX11 screenshot of that panel can be compared line for line with no code change on the
# read-only DX11 side.
set -u
D="/d/DEV/BUILD/GameGuru Wicked MAX Build Area/Max"
OUT="$(dirname "$0")"; DEMO="${1:-Switch Escape}"; RES="$OUT/su_hier_${2:-se}.txt"
LOCK="$OUT/.sumeasure.lock"
if [ -e "$LOCK" ] && kill -0 "$(cat "$LOCK" 2>/dev/null)" 2>/dev/null; then echo "REFUSING: PID $(cat "$LOCK") running"; exit 3; fi
echo $$ > "$LOCK"; trap 'rm -f "$LOCK"' EXIT INT TERM
: > "$RES"
say(){ echo "$(date +%H:%M:%S) $*" | tee -a "$RES"; }
send(){ rm -f "$D/auto_result.txt"; echo "$1" > "$D/auto_command.txt"; local t=0
  while [ $t -lt ${2:-40} ]; do if [ -f "$D/auto_result.txt" ]; then sleep 0.3; cat "$D/auto_result.txt"; return 0; fi; sleep 1; t=$((t+1)); done; echo TIMEOUT; }
alive(){ tasklist.exe //FI "IMAGENAME eq GameGuruMAX.exe" 2>/dev/null | grep -qi GameGuruMAX; }
wait_state(){ local t=0; while [ $t -lt $2 ]; do alive || return 2
  [ "$(send "GET_STATE" 20|head -1)" == "STATE: $1" ] && return 0; sleep 5; t=$((t+7)); done; return 1; }

say "=== $DEMO ==="
taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true; sleep 4
rm -f "$D/auto_command.txt" "$D/auto_result.txt"
cd "$D" && ./GameGuruMAX.exe >/dev/null 2>&1 &
sleep 15
wait_state "hub" 90 || { say FAIL_HUB; exit 1; }
sleep 4
R=$(send "SELECT_DEMO $DEMO" 20); [[ "$R" == OK:* ]] || { say "FAIL_SELECT $R"; exit 1; }
sleep 3; send "CLICK edit_game" 20 >/dev/null; sleep 10
send "CLICK_ONLY_LEVEL" 120 >/dev/null; sleep 60
wait_state "editor" 240 || say "WARN not editor"
sleep 30
d=$(send "GET_PERF_DATA" 60)
echo "$d" | grep -E "^FPS:|^POLYS:|^SCENE_OBJECTS:|^SCENE_TRANSFORMS:|^SCENE_HIERARCHY:|^SCENE_ARMATURES:|^SCENE_MESHES:|^HIER:" | tee -a "$RES"
say "-- with SET_SCENESERIAL 1 (SU-* shares) --"
send "ENABLE_PROFILER" 30 >/dev/null; sleep 6
send "SET_SCENESERIAL 1" 30 >/dev/null; sleep 8
for i in 1 2 3; do
  sleep 4; dd=$(send "GET_PERF_DATA" 60)
  h=$(echo "$dd" | grep -m1 -F "SU-Hierarchy" | sed -n 's/.*: *\([0-9.]*\) ms.*/\1/p')
  t=$(echo "$dd" | grep -m1 -F "SU-Transform" | sed -n 's/.*: *\([0-9.]*\) ms.*/\1/p')
  hi=$(echo "$dd" | grep -m1 '^HIER:')
  say "  SU-Hierarchy=$h SU-Transform=$t | $hi"
done
send "SET_SCENESERIAL 0" 30 >/dev/null; send "DISABLE_PROFILER" 30 >/dev/null
taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true
say "=== DONE ==="
