#!/bin/bash
D="/d/DEV/BUILD/GameGuru Wicked MAX Build Area/Max"
send() { rm -f "$D/auto_result.txt"; echo "$1" > "$D/auto_command.txt"; local t=0
  while [ $t -lt ${2:-30} ]; do [ -f "$D/auto_result.txt" ] && { cat "$D/auto_result.txt"; return 0; }; sleep 1; t=$((t+1)); done; echo TIMEOUT; }
alive() { tasklist.exe //FI "IMAGENAME eq GameGuruMAX.exe" 2>/dev/null | grep -qi GameGuruMAX; }
wait_state() { local t=0; while [ $t -lt $2 ]; do alive || return 2
  [ "$(send GET_STATE 20 | head -1)" == "STATE: $1" ] && return 0; sleep 5; t=$((t+7)); done; return 1; }
taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; sleep 4; rm -f "$D/auto_command.txt"
cd "$D" && ./GameGuruMAX.exe > /dev/null 2>&1 & sleep 15
wait_state hub 60 || exit 1; sleep 4
send "SELECT_DEMO Canyon Offensive" 15 >/dev/null; sleep 3
send "CLICK edit_game" 15 >/dev/null; sleep 8
wait_state storyboard 40 || exit 1
send "CLICK_ONLY_LEVEL" 15 >/dev/null
wait_state editor 180 || exit 1
sleep 25
echo "== A baseline (ocean on)";      send "SCREENSHOT x" 25 >/dev/null
echo "== B bake on, MAGENTA";         send "SET_WATERBAKEDEBUG 1" 20; send "SET_BAKEWATER 1" 20 >/dev/null; sleep 8
send "DUMP_BAKE" 20 | grep -E "plane|drawn|ocean|colour" | sed 's/^ */   /'
send "SCREENSHOT x" 25 >/dev/null
echo "== C bake on, normal colour";   send "SET_WATERBAKEDEBUG 0" 20 >/dev/null; sleep 5
send "SCREENSHOT x" 25 >/dev/null
send "SET_BAKEWATER 0" 20 >/dev/null
taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true
echo "DONE $(date +%H:%M:%S)"
