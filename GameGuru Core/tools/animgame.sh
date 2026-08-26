#!/bin/bash
# GGMAX 3.25n: Reduction Scale measured IN GAME, where Lee sees the flicker, with the profiler on
# so the "Skinning and Morph" GPU row is real. One screenshot per scale for the fidelity call.
D="/d/DEV/BUILD/GameGuru Wicked MAX Build Area/Max"
send() { rm -f "$D/auto_result.txt"; echo "$1" > "$D/auto_command.txt"; local t=0
  while [ $t -lt ${2:-40} ]; do [ -f "$D/auto_result.txt" ] && { cat "$D/auto_result.txt"; return 0; }; sleep 1; t=$((t+1)); done; echo TIMEOUT; }
alive() { tasklist.exe //FI "IMAGENAME eq GameGuruMAX.exe" 2>/dev/null | grep -qi GameGuruMAX; }
wait_state() { local t=0; while [ $t -lt $2 ]; do alive || return 2
  [ "$(send GET_STATE 20 | head -1)" == "STATE: $1" ] && return 0; sleep 5; t=$((t+7)); done; return 1; }
skin() { send "DUMP_PROFILER" 30 | grep -iE "Skinning and Morph" | head -1 | sed 's/^ *//'; }
taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; sleep 4; rm -f "$D/auto_command.txt"
cd "$D" && ./GameGuruMAX.exe > /dev/null 2>&1 & sleep 15
wait_state hub 60 || exit 1; sleep 4
send "OPEN_PROJECT TESTPRO2" 25 >/dev/null; sleep 6
send "CLICK_ONLY_LEVEL" 20 >/dev/null
wait_state editor 220 || exit 1; sleep 25
send "ENABLE_PROFILER 1" 20
echo "== entering game"
send "CLICK test_level" 20 >/dev/null
wait_state game 120 || { echo "FAIL_GAME"; taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; exit 1; }
sleep 35
for SC in 1 10 25 50 100; do
  send "SET_ANIMREDUCTION $SC" 20 >/dev/null; sleep 12
  echo "---- scale $SC"
  send "DUMP_ANIMREDUCTION" 25 | grep -E "tick box|engine scale|armatures" | sed 's/^ */   /'
  echo "   skin : $(skin)"
  send "GET_PERF_DATA" 20 | grep -E "^FPS:|^FRAME_TIME_MS:" | tr '\n' ' '; echo
  send "SCREENSHOT x" 30 >/dev/null
  alive || { echo "!!! DIED at $SC"; break; }
done
send "SET_ANIMREDUCTION 1" 20 >/dev/null
alive && echo "STILL ALIVE" || echo "!!! DIED"
taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true
echo "DONE $(date +%H:%M:%S)"
