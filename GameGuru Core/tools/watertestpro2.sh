#!/bin/bash
# GGMAX 3.25e: reproduce Lee's spotshadowtest scene in TESTPRO2 and read the real numbers.
D="/d/DEV/BUILD/GameGuru Wicked MAX Build Area/Max"
send() { rm -f "$D/auto_result.txt"; echo "$1" > "$D/auto_command.txt"; local t=0
  while [ $t -lt ${2:-30} ]; do [ -f "$D/auto_result.txt" ] && { cat "$D/auto_result.txt"; return 0; }; sleep 1; t=$((t+1)); done; echo TIMEOUT; }
alive() { tasklist.exe //FI "IMAGENAME eq GameGuruMAX.exe" 2>/dev/null | grep -qi GameGuruMAX; }
wait_state() { local t=0; while [ $t -lt $2 ]; do alive || return 2
  [ "$(send GET_STATE 20 | head -1)" == "STATE: $1" ] && return 0; sleep 5; t=$((t+7)); done; return 1; }
taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; sleep 4; rm -f "$D/auto_command.txt"
cd "$D" && ./GameGuruMAX.exe > /dev/null 2>&1 & sleep 15
wait_state hub 60 || { echo FAIL_HUB; exit 1; }
sleep 4
echo "-- open TESTPRO2"; send "OPEN_PROJECT TESTPRO2" 25
sleep 6
wait_state storyboard 60 || echo "(not storyboard)"
send "CLICK_ONLY_LEVEL" 20
wait_state editor 200 || { echo FAIL_EDITOR; exit 1; }
sleep 25
echo "== 1 BASELINE (real water)"; send "DUMP_BAKE" 20 | grep -E "colour|Terrain Tools|plane|drawn|ocean" | sed 's/^ */   /'
send "SCREENSHOT x" 25 >/dev/null
echo "== 2 WATER BAKE ON (authored RGBA)"; send "SET_BAKEWATER 1" 20 >/dev/null; sleep 8
send "DUMP_BAKE" 20 | grep -E "colour|Terrain Tools|plane|drawn|ocean" | sed 's/^ */   /'
send "SCREENSHOT x" 25 >/dev/null
echo "== 3 MAGENTA (is it drawn at all?)"; send "SET_WATERBAKEDEBUG 1" 20; sleep 6
send "GET_CAMERA" 20
send "DUMP_BAKE" 20 | grep -E "colour|plane|drawn" | sed 's/^ */   /'
send "SCREENSHOT x" 25 >/dev/null
send "SET_WATERBAKEDEBUG 0" 20 >/dev/null; sleep 4
echo "== 4 BAKE OFF (water must return)"; send "SET_BAKEWATER 0" 20 >/dev/null; sleep 8
send "DUMP_BAKE" 20 | grep -E "ocean" | sed 's/^ */   /'
send "SCREENSHOT x" 25 >/dev/null
alive && echo "STILL ALIVE" || echo "!!! DIED"
taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true
echo "DONE $(date +%H:%M:%S)"
