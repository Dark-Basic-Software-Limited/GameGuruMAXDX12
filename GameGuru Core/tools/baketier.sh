#!/bin/bash
# GGMAX 3.25j: two-tier bake. Does the play area get promoted, does the budget hold, and what
# does the near tier actually cost?
D="/d/DEV/BUILD/GameGuru Wicked MAX Build Area/Max"
send() { rm -f "$D/auto_result.txt"; echo "$1" > "$D/auto_command.txt"; local t=0
  while [ $t -lt ${2:-40} ]; do [ -f "$D/auto_result.txt" ] && { cat "$D/auto_result.txt"; return 0; }; sleep 1; t=$((t+1)); done; echo TIMEOUT; }
alive() { tasklist.exe //FI "IMAGENAME eq GameGuruMAX.exe" 2>/dev/null | grep -qi GameGuruMAX; }
wait_state() { local t=0; while [ $t -lt $2 ]; do alive || return 2
  [ "$(send GET_STATE 20 | head -1)" == "STATE: $1" ] && return 0; sleep 5; t=$((t+7)); done; return 1; }
taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; sleep 4; rm -f "$D/auto_command.txt"
cd "$D" && ./GameGuruMAX.exe > /dev/null 2>&1 & sleep 15
wait_state hub 60 || exit 1; sleep 4
send "OPEN_PROJECT TESTPRO2" 25 >/dev/null; sleep 6
send "CLICK_ONLY_LEVEL" 20 >/dev/null
wait_state editor 220 || exit 1; sleep 35
echo "== REAL terrain"; send "GET_PERF_DATA" 20 | grep -E "^FPS:|^VRAM_MB:" | tr '\n' ' '; echo
send "SCREENSHOT x" 30 >/dev/null
echo "== TWO-TIER BAKE (far 256 / near 4096)"
send "SET_BAKETERRAIN 1" 20 >/dev/null; sleep 40
send "DUMP_BAKE" 30 | sed 's/^ */   /'
send "GET_PERF_DATA" 20 | grep -E "^FPS:|^VRAM_MB:" | tr '\n' ' '; echo
send "SCREENSHOT x" 30 >/dev/null
alive && echo "STILL ALIVE" || echo "!!! DIED"
taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true
echo "DONE $(date +%H:%M:%S)"
