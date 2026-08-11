#!/bin/bash
# Survey the wi::terrain chunk RING BUDGET across demos: what each level builds vs what it needs.
#
# WHY: the ring is the DX12 entity floor. (2*gen+1)^2 chunk entities, each a mesh + material +
# transform + hierarchy node walked by Scene::Update every frame; DX11 has none of them. Before
# lowering `generation` the question is not what a ring costs but what it COVERS — the playable
# map is +/-editable_size, a UI slider (0.5-5.0 km) that every level sets differently.
#
# TERRAIN_RING (GET_PERF_DATA, GGMAX 2.25) prints both sides. The column that decides the floor
# is needGen = ceil(mapHalfU / chunkU): the smallest generation that still covers the playable
# map. Rings beyond it are filler OUTSIDE the editable area.
#
# Usage: terrainring.sh [outfile] [demo ...]
#   default demo list = a spread of terrain-heavy / island / interior levels
#
# One launch per demo (a level change needs a hub round-trip). ~2 min each.
#
# TRAPS GUARDED (each cost real time before — see sumeasure.sh header):
#  - PID lockfile: pkill silently does nothing under Git Bash; leaked runners corrupt quietly.
#  - Settle gate, not a fixed soak: the ring builds progressively, so an early read undercounts
#    `chunks`. gen/needGen/mapHalfU are stable from load, but `chunks` is not.
#  - say() writes to the LOG, never to captured stdout.
set -u
D="/d/DEV/BUILD/GameGuru Wicked MAX Build Area/Max"
OUT="$(dirname "$0")"
RES="${1:-$OUT/terrain_ring.txt}"; shift || true
if [ "$#" -gt 0 ]; then DEMOS=("$@"); else
  DEMOS=("Island Showdown" "Switch Escape" "Aztec Teaser" "Horseshoe Bend" "Zombie Cellar" "Aliens Are Coming")
fi
LOG="${RES%.txt}.log"

LOCK="$OUT/.terrainring.lock"
if [ -e "$LOCK" ] && kill -0 "$(cat "$LOCK" 2>/dev/null)" 2>/dev/null; then
  echo "REFUSING TO START: terrainring.sh already running as PID $(cat "$LOCK")."; exit 3
fi
echo $$ > "$LOCK"; trap 'rm -f "$LOCK"' EXIT INT TERM

: > "$RES"; : > "$LOG"
say() { echo "$(date +%H:%M:%S) $*" >> "$LOG"; echo "$(date +%H:%M:%S) $*" >&2; }
send() { rm -f "$D/auto_result.txt"; echo "$1" > "$D/auto_command.txt"; local t=0
  while [ $t -lt ${2:-40} ]; do if [ -f "$D/auto_result.txt" ]; then sleep 0.3; cat "$D/auto_result.txt"; return 0; fi; sleep 1; t=$((t+1)); done; echo TIMEOUT; }
alive() { tasklist.exe //FI "IMAGENAME eq GameGuruMAX.exe" 2>/dev/null | grep -qi GameGuruMAX; }
wait_state() { local t=0; while [ $t -lt $2 ]; do alive || return 2
  [ "$(send "GET_STATE" 20|head -1)" == "STATE: $1" ] && return 0; sleep 5; t=$((t+7)); done; return 1; }
# Settle on the CHUNK COUNT, not FPS: the ring builds progressively and that is the number we
# are here to read. Two consecutive identical reads = the generator has stopped adding.
wait_chunks() {
  local maxs=${1:-240} t=0 prev=-1 good=0 c
  while [ $t -lt $maxs ]; do
    sleep 15; t=$((t+15)); alive || return 2
    c=$(send "GET_PERF_DATA" 45 | grep -m1 '^TERRAIN_RING:' | sed -n 's/.*chunks=\([0-9]*\).*/\1/p')
    [ -z "$c" ] && { say "    (no TERRAIN_RING yet)"; continue; }
    if [ "$c" == "$prev" ]; then good=$((good+1)); [ $good -ge 2 ] && { say "    chunks settled at $c (${t}s)"; return 0; }
    else good=0; fi
    prev=$c
  done
  say "    WARN chunks never settled (last=$prev)"; return 1
}

say "=== terrain ring survey: ${#DEMOS[@]} demos ==="
for DEMO in "${DEMOS[@]}"; do
  say "--- $DEMO ---"
  taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true; sleep 4
  rm -f "$D/auto_command.txt" "$D/auto_result.txt"
  ( cd "$D" && ./GameGuruMAX.exe >/dev/null 2>&1 & )
  sleep 15
  if ! wait_state "hub" 90; then say "  FAIL_HUB"; echo "$DEMO FAIL_HUB" >> "$RES"; continue; fi
  sleep 4
  R=$(send "SELECT_DEMO $DEMO" 20)
  if [[ "$R" != OK:* ]]; then say "  FAIL_SELECT $R"; echo "$DEMO FAIL_SELECT" >> "$RES"; continue; fi
  sleep 3; send "CLICK edit_game" 20 >/dev/null; sleep 10
  send "CLICK_ONLY_LEVEL" 120 >/dev/null; sleep 50
  wait_state "editor" 240 || say "  WARN not editor"
  wait_chunks 240

  d=$(send "GET_PERF_DATA" 60)
  ring=$(echo "$d" | grep -m1 '^TERRAIN_RING:')
  obj=$(echo "$d" | grep -m1 '^SCENE_OBJECTS:'   | awk '{print $2}')
  msh=$(echo "$d" | grep -m1 '^SCENE_MESHES:'    | awk '{print $2}')
  mat=$(echo "$d" | grep -m1 '^SCENE_MATERIALS:' | awk '{print $2}')
  tra=$(echo "$d" | grep -m1 '^SCENE_TRANSFORMS:'| awk '{print $2}')
  hie=$(echo "$d" | grep -m1 '^SCENE_HIERARCHY:' | awk '{print $2}')
  pol=$(echo "$d" | grep -m1 '^POLYS:'           | awk '{print $2}')
  say "  $ring"
  say "  objects=$obj meshes=$msh materials=$mat transforms=$tra hierarchy=$hie polys=$pol"
  echo "DEMO=$DEMO ${ring#TERRAIN_RING: } objects=$obj meshes=$msh materials=$mat transforms=$tra hierarchy=$hie polys=$pol" >> "$RES"
done
taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true
say "=== done -> $RES ==="
