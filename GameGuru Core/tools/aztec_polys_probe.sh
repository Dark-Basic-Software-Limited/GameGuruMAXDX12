#!/bin/bash
# GGMAX 3.73b: Aztec Game Kit Teaser moved +312 POLYS in both post-3.73 soaks (1413604 -> 1413916)
# while the fresh-launch sweep still read 1413604. Same demo, same position, same 90 s settle as
# the soak that validated the pre-alpha build - so it is build-attributable and has to be named.
#
# Reproduces the SOAK's measurement exactly (60 s settle, 3 samples 15 s apart, take the MAX)
# on a single demo, and adds the pool census so the triangles can be attributed.
#   usage: aztec_polys_probe.sh <label>
D="/d/DEV/BUILD/GameGuru Wicked MAX Build Area/Max"
LABEL="${1:-probe}"
OUT="${GGMAX_AZTEC_OUT:-/c/Users/leeba/AppData/Local/Temp/claude/D--max-GameGuruMAXDX12/9a28c586-4c13-4447-916e-7fb51301bfa8/scratchpad/aztec}"
mkdir -p "$OUT"

send() { rm -f "$D/auto_result.txt"; echo "$1" > "$D/auto_command.txt"; local t=0
  while [ $t -lt ${2:-30} ]; do [ -f "$D/auto_result.txt" ] && { sleep 0.2; cat "$D/auto_result.txt"; return 0; }; sleep 1; t=$((t+1)); done; echo TIMEOUT; }
alive() { tasklist.exe //FI "IMAGENAME eq GameGuruMAX.exe" 2>/dev/null | grep -qi GameGuruMAX; }
wait_state() { local t=0; while [ $t -lt $2 ]; do alive || return 2
  [ "$(send GET_STATE 20 | head -1)" == "STATE: $1" ] && return 0; sleep 5; t=$((t+7)); done; return 1; }

taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; sleep 4; rm -f "$D/auto_command.txt"
cd "$D" && ./GameGuruMAX.exe > /dev/null 2>&1 &
sleep 25
wait_state hub 90 || { echo FAIL_HUB; exit 1; }
sleep 4
# same route the soak takes: the hub defaults to My Games once a user project exists
send "NAVIGATE hub" 20 >/dev/null; sleep 6
send "NAVIGATE hub.demo_games" 20 >/dev/null; sleep 4
send "SELECT_DEMO Aztec Game Kit Teaser" 20
sleep 3
# ⚠ the soak does SELECT_DEMO -> CLICK edit_game -> storyboard. Dropping edit_game leaves
# the hub sitting on the tab and every later step fails as something else.
send "CLICK edit_game" 20
sleep 8
wait_state storyboard 90 || { echo FAIL_STORYBOARD; exit 1; }
send "CLICK_ONLY_LEVEL" 25
wait_state editor 300 || { echo FAIL_EDITOR; exit 1; }

echo "== $LABEL : soak-identical settle (60 s, then 3 samples 15 s apart, MAX wins)"
sleep 60
BEST=0
for s in 1 2 3; do
  P=$(send "GET_PERF_DATA" 30 | tr ',' '\n' | grep -m1 "POLYS:" | tr -dc '0-9')
  echo "   sample $s POLYS=$P"
  [ -n "$P" ] && [ "$P" -gt "$BEST" ] && BEST=$P
  sleep 15
done
echo "   POLYS(max) = $BEST"

send "DUMP_TREEPOOL" 25 >/dev/null; sleep 1
if [ -f "$D/Files/treepool_dump.txt" ]; then
  cp "$D/Files/treepool_dump.txt" "$OUT/treepool_$LABEL.txt"
  head -4 "$D/Files/treepool_dump.txt" | sed 's/^/   /'
fi
echo "$LABEL|POLYS=$BEST|$(head -1 "$OUT/treepool_$LABEL.txt" 2>/dev/null)" >> "$OUT/summary.txt"

taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true
echo "DONE $LABEL $(date +%H:%M:%S)"
