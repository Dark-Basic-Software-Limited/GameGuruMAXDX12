#!/bin/bash
# GGMAX 3.25g HARSH: the benign cycle test never caught the terrain mid-generation - every build
# succeeded first try, which is NOT Lee's condition. This toggles fast and MOVES THE CAMERA
# between toggles so chunks are always generating, which is exactly when merge_pending is set and
# the old code allocated 227 MB a frame. If the cascade survives anywhere, it is here.
D="/d/DEV/BUILD/GameGuru Wicked MAX Build Area/Max"
send() { rm -f "$D/auto_result.txt"; echo "$1" > "$D/auto_command.txt"; local t=0
  while [ $t -lt ${2:-30} ]; do [ -f "$D/auto_result.txt" ] && { cat "$D/auto_result.txt"; return 0; }; sleep 1; t=$((t+1)); done; echo TIMEOUT; }
alive() { tasklist.exe //FI "IMAGENAME eq GameGuruMAX.exe" 2>/dev/null | grep -qi GameGuruMAX; }
wait_state() { local t=0; while [ $t -lt $2 ]; do alive || return 2
  [ "$(send GET_STATE 20 | head -1)" == "STATE: $1" ] && return 0; sleep 5; t=$((t+7)); done; return 1; }
stat_line() { send "GET_PERF_DATA" 20 | grep -E "^FPS:|^VRAM_MB:" | tr '\n' ' '; echo; }
taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; sleep 4; rm -f "$D/auto_command.txt"
cd "$D" && ./GameGuruMAX.exe > /dev/null 2>&1 & sleep 15
wait_state hub 60 || exit 1; sleep 4
send "OPEN_PROJECT TESTPRO2" 25 >/dev/null; sleep 6
send "CLICK_ONLY_LEVEL" 20 >/dev/null
wait_state editor 200 || exit 1; sleep 30
echo "baseline            $(stat_line)"
X=3647; Z=3216
for c in 1 2 3 4 5 6 7 8; do
  X=$((X + 4000)); Z=$((Z + 2500))          # force fresh chunk generation every cycle
  send "SET_CAMERA $X 900 $Z 20 125" 20 >/dev/null
  send "SET_BAKETERRAIN 1" 20 >/dev/null; sleep 3     # tick while chunks are still generating
  send "SET_BAKETERRAIN 0" 20 >/dev/null; sleep 2
  send "SET_BAKETERRAIN 1" 20 >/dev/null; sleep 3
  echo "cycle $c (moving)    $(stat_line)"
  send "DUMP_BAKE" 20 | grep -E "waiting on|video memory|wicked terrain" | tr '\n' ' ' | sed 's/  */ /g'; echo
done
send "SET_BAKETERRAIN 0" 20 >/dev/null; sleep 20
echo "settled, bake OFF   $(stat_line)"
send "SET_BAKETERRAIN 1" 20 >/dev/null; sleep 20
echo "settled, bake ON    $(stat_line)"
send "DUMP_BAKE" 20 | grep -E "waiting on|chunks baked|video memory|wicked terrain" | sed 's/^ */     /'
alive && echo "STILL ALIVE" || echo "!!! DIED"
taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true
echo "DONE $(date +%H:%M:%S)"
