#!/bin/bash
# GGMAX 3.25h: real vs baked from the SAME camera looking at the horizon. The claim under test is
# that the baked terrain reaches as far as the real one and the tree billboards are culled the
# same either way (GG_GetTerrainViewRadius now reports the baked radius).
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
wait_state editor 200 || exit 1; sleep 35
send "SET_CAMERA 3647 1400 3216 8 125" 20 >/dev/null; sleep 8
echo "== REAL terrain"; send "GET_PERF_DATA" 20 | grep -E "^FPS:|^VRAM_MB:" | tr '\n' ' '; echo
send "DUMP_FARTREES" 20 2>/dev/null | head -3
send "SCREENSHOT x" 25 >/dev/null
send "SET_BAKETERRAIN 1" 20 >/dev/null; sleep 25
echo "== BAKED terrain (same camera)"; send "GET_PERF_DATA" 20 | grep -E "^FPS:|^VRAM_MB:" | tr '\n' ' '; echo
send "DUMP_BAKE" 20 | grep -E "chunks baked|video memory|wicked terrain" | sed 's/^ */   /'
send "DUMP_FARTREES" 20 2>/dev/null | head -3
send "SCREENSHOT x" 25 >/dev/null
taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true
echo "DONE $(date +%H:%M:%S)"
