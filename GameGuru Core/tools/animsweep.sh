#!/bin/bash
# GGMAX 3.25n: Reduction Scale fidelity/performance sweep. For each scale: how many armatures are
# held, what the Skinning and Morph GPU row costs, and a screenshot to judge fidelity.
D="/d/DEV/BUILD/GameGuru Wicked MAX Build Area/Max"
send() { rm -f "$D/auto_result.txt"; echo "$1" > "$D/auto_command.txt"; local t=0
  while [ $t -lt ${2:-40} ]; do [ -f "$D/auto_result.txt" ] && { cat "$D/auto_result.txt"; return 0; }; sleep 1; t=$((t+1)); done; echo TIMEOUT; }
alive() { tasklist.exe //FI "IMAGENAME eq GameGuruMAX.exe" 2>/dev/null | grep -qi GameGuruMAX; }
wait_state() { local t=0; while [ $t -lt $2 ]; do alive || return 2
  [ "$(send GET_STATE 20 | head -1)" == "STATE: $1" ] && return 0; sleep 5; t=$((t+7)); done; return 1; }
skinms() { send "DUMP_PROFILER" 30 | grep -i "Skinning and Morph" | head -1 | sed 's/^ *//'; }
taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; sleep 4; rm -f "$D/auto_command.txt"
cd "$D" && ./GameGuruMAX.exe > /dev/null 2>&1 & sleep 15
wait_state hub 60 || exit 1; sleep 4
send "OPEN_PROJECT TESTPRO2" 25 >/dev/null; sleep 6
send "CLICK_ONLY_LEVEL" 20 >/dev/null
wait_state editor 220 || exit 1; sleep 30
send "SET_ANIM30FPS 1" 20 >/dev/null 2>&1
for SC in 1 10 25 50 100; do
  send "SET_ANIMREDUCTION $SC" 20 >/dev/null; sleep 10
  echo "---- scale $SC"
  send "DUMP_ANIMREDUCTION" 25 | grep -E "engine scale|armatures|500 units|1000 units|2000 units" | sed 's/^ */   /'
  echo "   skin: $(skinms)"
  send "GET_PERF_DATA" 20 | grep -E "^FPS:" | tr '\n' ' '; echo
  send "SCREENSHOT x" 30 >/dev/null
done
send "SET_ANIMREDUCTION 1" 20 >/dev/null
alive && echo "STILL ALIVE" || echo "!!! DIED"
taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true
echo "DONE $(date +%H:%M:%S)"
