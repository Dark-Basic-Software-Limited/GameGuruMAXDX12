#!/bin/bash
# Photograph the terrain ring EDGE on a level whose playable map is bigger than its ring.
#
# WHY: TERRAIN_RING reports needGen=19 for the two 5 km demos (A Grand Canyon Adventure,
# Operation Amazon) against a shipped generation of 12 — arithmetic says the outer ~891 m of their
# own EDITABLE map has no terrain chunk under it. That is an inference from ring math, not an
# observation. This puts the camera at successive distances along +X and photographs whether there
# is ground there, so the claim rests on a picture rather than a calculation.
#
# Usage: ringedge.sh <demo> [xUnits ...]
#   Defaults straddle the gen-12 ring edge (63360 u) and run out to the map edge.
#
# Reference for the default demos: chunkU 5280, gen 12 -> ring reach 63360 u (1609 m);
# editable half-size 98425 u (2500 m). So 70000+ should be map-legal but terrain-less.
set -u
D="/d/DEV/BUILD/GameGuru Wicked MAX Build Area/Max"
OUT="$(dirname "$0")"; SHOTS="$OUT/ringshots"; mkdir -p "$SHOTS"
DEMO="${1:?need a demo name}"; shift || true
if [ "$#" -gt 0 ]; then XS=("$@"); else XS=(30000 55000 70000 90000); fi
LOG="$OUT/ring_edge.log"; RES="$OUT/ring_edge.txt"

LOCK="$OUT/.ringedge.lock"
if [ -e "$LOCK" ] && kill -0 "$(cat "$LOCK" 2>/dev/null)" 2>/dev/null; then
  echo "REFUSING TO START: ringedge.sh already running as PID $(cat "$LOCK")."; exit 3
fi
echo $$ > "$LOCK"; trap 'rm -f "$LOCK"' EXIT INT TERM
: > "$LOG"; : > "$RES"
say() { echo "$(date +%H:%M:%S) $*" >> "$LOG"; echo "$(date +%H:%M:%S) $*" >&2; }
send() { rm -f "$D/auto_result.txt"; echo "$1" > "$D/auto_command.txt"; local t=0
  while [ $t -lt ${2:-40} ]; do if [ -f "$D/auto_result.txt" ]; then sleep 0.3; cat "$D/auto_result.txt"; return 0; fi; sleep 1; t=$((t+1)); done; echo TIMEOUT; }
alive() { tasklist.exe //FI "IMAGENAME eq GameGuruMAX.exe" 2>/dev/null | grep -qi GameGuruMAX; }
wait_state() { local t=0; while [ $t -lt $2 ]; do alive || return 2
  [ "$(send "GET_STATE" 20|head -1)" == "STATE: $1" ] && return 0; sleep 5; t=$((t+7)); done; return 1; }
grab() { local tag="$1" r; r=$(send "SCREENSHOT" 60); local p="${r#OK: Screenshot saved to }"
  if [ -f "$p" ]; then cp "$p" "$SHOTS/${tag}.png"; say "    shot -> ${tag}.png"; else say "    SHOT FAILED: $r"; fi; }

say "=== ring edge: $DEMO at x = ${XS[*]} ==="
taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true; sleep 4
rm -f "$D/auto_command.txt" "$D/auto_result.txt"
( cd "$D" && ./GameGuruMAX.exe >/dev/null 2>&1 & )
sleep 15
wait_state "hub" 90 || { say "FAIL_HUB"; exit 1; }
sleep 4
R=$(send "SELECT_DEMO $DEMO" 20); [[ "$R" == OK:* ]] || { say "FAIL_SELECT $R"; exit 1; }
sleep 3; send "CLICK edit_game" 20 >/dev/null; sleep 10
send "CLICK_ONLY_LEVEL" 120 >/dev/null; sleep 50
wait_state "editor" 240 || say "WARN not editor"
sleep 45
say "  $(send "GET_PERF_DATA" 60 | grep -m1 '^TERRAIN_RING:')"

for X in "${XS[@]}"; do
  # Elevated and pitched down so ground-vs-void is unambiguous regardless of yaw.
  send "SET_CAMERA $X 25000 0 35 90" 20 >/dev/null
  # Terrain streams in at the new centre chunk; give the generator time before judging "no ground".
  sleep 40
  d=$(send "GET_PERF_DATA" 60)
  ring=$(echo "$d" | grep -m1 '^TERRAIN_RING:')
  pol=$(echo "$d" | grep -m1 '^POLYS:' | awk '{print $2}')
  grab "${DEMO// /_}_edge_x${X}"
  say "  x=$X polys=$pol"
  echo "x=$X polys=$pol ${ring#TERRAIN_RING: }" >> "$RES"
done
taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true
say "=== done -> $RES ==="
