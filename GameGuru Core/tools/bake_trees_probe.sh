#!/bin/bash
# GGMAX 3.73: Terrain Bake makes the near 3D trees vanish (Lee, testpro2level).
# Read the pool census either side of the switch, and prove WHY it never comes back.
D="/d/DEV/BUILD/GameGuru Wicked MAX Build Area/Max"
OUT="${GGMAX_BAKE_OUT:-/c/Users/leeba/AppData/Local/Temp/claude/D--max-GameGuruMAXDX12/9a28c586-4c13-4447-916e-7fb51301bfa8/scratchpad/baketrees}"
mkdir -p "$OUT"

send() { rm -f "$D/auto_result.txt"; echo "$1" > "$D/auto_command.txt"; local t=0
  while [ $t -lt ${2:-30} ]; do [ -f "$D/auto_result.txt" ] && { cat "$D/auto_result.txt"; return 0; }; sleep 1; t=$((t+1)); done; echo TIMEOUT; }
alive() { tasklist.exe //FI "IMAGENAME eq GameGuruMAX.exe" 2>/dev/null | grep -qi GameGuruMAX; }
wait_state() { local t=0; while [ $t -lt $2 ]; do alive || return 2
  [ "$(send GET_STATE 20 | head -1)" == "STATE: $1" ] && return 0; sleep 5; t=$((t+7)); done; return 1; }
pool() { send "DUMP_TREEPOOL" 25 >/dev/null; sleep 1
  if [ -f "$D/Files/treepool_dump.txt" ]; then cp "$D/Files/treepool_dump.txt" "$OUT/$1.txt"
    head -12 "$D/Files/treepool_dump.txt" | sed 's/^/   /'
  else echo "   (no treepool_dump.txt)"; fi }
shot() { local line; line=$(send "SCREENSHOT x" 30); local p
  p=$(echo "$line" | sed 's/^OK: Screenshot saved to //' | tr '\\' '/')
  [ -f "$p" ] && cp "$p" "$OUT/$1.png" && echo "   shot -> $OUT/$1.png"; }

taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; sleep 4; rm -f "$D/auto_command.txt"
cd "$D" && ./GameGuruMAX.exe > /dev/null 2>&1 &
sleep 25
wait_state hub 90 || { echo FAIL_HUB; exit 1; }
sleep 4
send "OPEN_PROJECT TESTPRO2" 30 >/dev/null
sleep 6
wait_state storyboard 60 || echo "(not storyboard)"
send "CLICK_NODE testpro2level" 25
wait_state editor 300 || { echo FAIL_EDITOR; exit 1; }
sleep 35

echo "== 1 BAKE OFF (baseline)"
pool p1_bakeoff
send "GET_PERF_DATA" 25 | tr ',' '\n' | grep -iE "TOTAL_OBJ|OBJECTS|TERRAIN_DRAW|CULLED" | sed 's/^/   /'
shot s1_bakeoff

echo "== 2 BAKE ON"
send "SET_BAKETERRAIN 1" 25
sleep 20
send "DUMP_BAKE" 25 | grep -iE "ready|chunks|terrain|radius|torn" | sed 's/^/   /'
pool p2_bakeon
send "GET_PERF_DATA" 25 | tr ',' '\n' | grep -iE "TOTAL_OBJ|OBJECTS|TERRAIN_DRAW|CULLED" | sed 's/^/   /'
shot s2_bakeon

echo "== 3 still gone 15 s later? (is anything trying to regrow the pool?)"
sleep 15
pool p3_bakeon_later

echo "== 4 BAKE OFF again"
send "SET_BAKETERRAIN 0" 25
sleep 20
pool p4_bakeoff_again
send "GET_PERF_DATA" 25 | tr ',' '\n' | grep -iE "TOTAL_OBJ|OBJECTS|TERRAIN_DRAW|CULLED" | sed 's/^/   /'
shot s4_bakeoff_again

alive && echo "STILL ALIVE" || echo "!!! DIED"
taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true
echo "DONE $(date +%H:%M:%S)"
