#!/bin/bash
# Is the plane failing to draw, or is it drawing and being occluded by the pool floor?
# Fly high and look straight down with the plane forced opaque magenta. If the plane exists at
# y=478 it will paint the whole basin; if nothing appears the draw is being rejected.
D="/d/DEV/BUILD/GameGuru Wicked MAX Build Area/Max"
send() { rm -f "$D/auto_result.txt"; echo "$1" > "$D/auto_command.txt"; local t=0
  while [ $t -lt ${2:-30} ]; do [ -f "$D/auto_result.txt" ] && { cat "$D/auto_result.txt"; return 0; }; sleep 1; t=$((t+1)); done; echo TIMEOUT; }
alive() { tasklist.exe //FI "IMAGENAME eq GameGuruMAX.exe" 2>/dev/null | grep -qi GameGuruMAX; }
wait_state() { local t=0; while [ $t -lt $2 ]; do alive || return 2
  [ "$(send GET_STATE 20 | head -1)" == "STATE: $1" ] && return 0; sleep 5; t=$((t+7)); done; return 1; }
taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; sleep 4; rm -f "$D/auto_command.txt"
cd "$D" && ./GameGuruMAX.exe > /dev/null 2>&1 & sleep 15
wait_state hub 60 || exit 1; sleep 4
send "OPEN_PROJECT TESTPRO2" 25 >/dev/null; sleep 6
send "CLICK_ONLY_LEVEL" 20 >/dev/null
wait_state editor 200 || exit 1; sleep 25
send "SET_BAKEWATER 1" 20 >/dev/null
send "SET_WATERBAKEDEBUG 2" 20; sleep 6
echo "-- A0: gpup STILL ON (control)"
send "SET_CAMERA 3647 6000 3216 89 125" 20 >/dev/null; sleep 4
send "SCREENSHOT x" 25 >/dev/null
echo "-- A: gpup DISABLED (the predicted A/B)"
send "GPUP_SHOW 0" 20
sleep 4
send "SET_CAMERA 3647 6000 3216 89 125" 20
sleep 4; send "GET_CAMERA" 20
send "DUMP_BAKE" 20 | grep -E "plane|drawn|VERTEX|colour" | sed 's/^ */   /'
send "SCREENSHOT x" 25 >/dev/null
echo "-- B: just above the plane, looking down (pitch 45)"
send "SET_CAMERA 3647 900 3216 45 125" 20
sleep 4; send "GET_CAMERA" 20
send "SCREENSHOT x" 25 >/dev/null
echo "-- C: BELOW the plane looking UP (pitch -40)"
send "SET_CAMERA 3647 300 3216 -40 125" 20
sleep 4; send "GET_CAMERA" 20
send "SCREENSHOT x" 25 >/dev/null
taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true
echo "DONE $(date +%H:%M:%S)"
