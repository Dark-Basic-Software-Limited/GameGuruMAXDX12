#!/bin/bash
# GGMAX 3.72: does a billboard tree now haze at the same rate as the terrain it stands on?
# Lee's repro: TESTPRO2 / testpro2desertlevel2, Fog Opacity 100, Horizon/Fog colour pure red.
# Shot A = the level's own saved fog range. Shot B = range squeezed to match Lee's second image.
D="/d/DEV/BUILD/GameGuru Wicked MAX Build Area/Max"
OUT="${GGMAX_FOG_OUT:-/c/Users/leeba/AppData/Local/Temp/claude/D--max-GameGuruMAXDX12/9a28c586-4c13-4447-916e-7fb51301bfa8/scratchpad/fogshots}"
mkdir -p "$OUT"

send() { rm -f "$D/auto_result.txt"; echo "$1" > "$D/auto_command.txt"; local t=0
  while [ $t -lt ${2:-30} ]; do [ -f "$D/auto_result.txt" ] && { cat "$D/auto_result.txt"; return 0; }; sleep 1; t=$((t+1)); done; echo TIMEOUT; }
alive() { tasklist.exe //FI "IMAGENAME eq GameGuruMAX.exe" 2>/dev/null | grep -qi GameGuruMAX; }
wait_state() { local t=0; while [ $t -lt $2 ]; do alive || return 2
  [ "$(send GET_STATE 20 | head -1)" == "STATE: $1" ] && return 0; sleep 5; t=$((t+7)); done; return 1; }
# the screenshot verb returns "OK: Screenshot saved to <path>" - copy it out under our own name
shot() { local nm="$1"; local line; line=$(send "SCREENSHOT x" 30)
  local p; p=$(echo "$line" | sed 's/^OK: Screenshot saved to //')
  p=$(echo "$p" | tr '\\' '/')
  if [ -f "$p" ]; then cp "$p" "$OUT/$nm.png"; echo "   shot -> $OUT/$nm.png"; else echo "   SHOT FAILED: $line"; fi }

taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; sleep 4; rm -f "$D/auto_command.txt"
cd "$D" && ./GameGuruMAX.exe > /dev/null 2>&1 &
sleep 25
wait_state hub 90 || { echo FAIL_HUB; exit 1; }
sleep 4

echo "-- open TESTPRO2"; send "OPEN_PROJECT TESTPRO2" 30
sleep 6
wait_state storyboard 60 || echo "(not storyboard)"
echo "-- storyboard nodes"; send "GET_SCREEN_TEXT" 25 | head -40
if [ -n "$1" ]; then send "CLICK_NODE $1" 25; else send "CLICK_ONLY_LEVEL" 25; fi
wait_state editor 300 || { echo FAIL_EDITOR; exit 1; }
sleep 30

echo "== A  level's own saved fog"
send "GET_CAMERA" 20
send "SET_FOGDENS -1" 20
send "GET_WEATHER" 25 | head -25
shot A_saved_fog

echo "== B  range squeezed to Lee's second image (Fog Range 0-37 -> far 18500, density 4/18500)"
send "SET_FOGDENS 0.000216" 20
sleep 3
shot B_range37

echo "== C  fog off, as the control"
send "SET_FOGDENS 0" 20
sleep 3
shot C_fogoff

echo "== D  back to the saved model"
send "SET_FOGDENS -1" 20
sleep 3
shot D_restored

alive && echo "STILL ALIVE" || echo "!!! DIED"
taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true
echo "DONE $(date +%H:%M:%S)"
