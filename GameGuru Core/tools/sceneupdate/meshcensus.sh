#!/bin/bash
# Run DUMP_MESHES on one or more demos and collect the censuses.
#
# WHY: SWITCHESCAPE_PERF.md §16 derived the "~75 unexplained meshes" in the DX12 entity floor by
# SUBTRACTION (target minus accounted creators). That can prove a remainder exists but never say
# what it is, and a subtraction-derived finding is the shape that produced the retracted §22.7.
# DUMP_MESHES enumerates every live MeshComponent and groups it two ways (geometry signature,
# name), so the remainder has to name itself.
#
# Usage: meshcensus.sh <demo> [demo ...]
set -u
D="/d/DEV/BUILD/GameGuru Wicked MAX Build Area/Max"
OUT="$(dirname "$0")"; CEN="$OUT/census"; mkdir -p "$CEN"
LOG="$CEN/meshcensus.log"

LOCK="$OUT/.meshcensus.lock"
if [ -e "$LOCK" ] && kill -0 "$(cat "$LOCK" 2>/dev/null)" 2>/dev/null; then
  echo "REFUSING TO START: meshcensus.sh already running as PID $(cat "$LOCK")."; exit 3
fi
echo $$ > "$LOCK"; trap 'rm -f "$LOCK"' EXIT INT TERM
: > "$LOG"
say() { echo "$(date +%H:%M:%S) $*" >> "$LOG"; echo "$(date +%H:%M:%S) $*" >&2; }
send() { rm -f "$D/auto_result.txt"; echo "$1" > "$D/auto_command.txt"; local t=0
  while [ $t -lt ${2:-40} ]; do if [ -f "$D/auto_result.txt" ]; then sleep 0.3; cat "$D/auto_result.txt"; return 0; fi; sleep 1; t=$((t+1)); done; echo TIMEOUT; }
alive() { tasklist.exe //FI "IMAGENAME eq GameGuruMAX.exe" 2>/dev/null | grep -qi GameGuruMAX; }
wait_state() { local t=0; while [ $t -lt $2 ]; do alive || return 2
  [ "$(send "GET_STATE" 20|head -1)" == "STATE: $1" ] && return 0; sleep 5; t=$((t+7)); done; return 1; }
# Settle on the chunk count: the terrain ring builds progressively and its chunks ARE most of the
# floor, so an early census undercounts the very thing being audited.
wait_chunks() {
  local maxs=${1:-240} t=0 prev=-1 good=0 c
  while [ $t -lt $maxs ]; do
    sleep 15; t=$((t+15)); alive || return 2
    c=$(send "GET_PERF_DATA" 45 | grep -m1 '^TERRAIN_RING:' | sed -n 's/.*chunks=\([0-9]*\).*/\1/p')
    [ -z "$c" ] && continue
    if [ "$c" == "$prev" ]; then good=$((good+1)); [ $good -ge 2 ] && { say "    chunks settled at $c (${t}s)"; return 0; }
    else good=0; fi
    prev=$c
  done
  say "    WARN chunks never settled (last=$prev)"; return 1
}

for DEMO in "$@"; do
  say "--- $DEMO ---"
  taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true; sleep 4
  rm -f "$D/auto_command.txt" "$D/auto_result.txt" "$D/Files/mesh_census.txt"
  ( cd "$D" && ./GameGuruMAX.exe >/dev/null 2>&1 & )
  sleep 15
  wait_state "hub" 90 || { say "  FAIL_HUB"; continue; }
  sleep 4
  R=$(send "SELECT_DEMO $DEMO" 20); [[ "$R" == OK:* ]] || { say "  FAIL_SELECT $R"; continue; }
  sleep 3; send "CLICK edit_game" 20 >/dev/null; sleep 10
  send "CLICK_ONLY_LEVEL" 120 >/dev/null; sleep 50
  wait_state "editor" 240 || say "  WARN not editor"
  wait_chunks 240
  say "  $(send "GET_PERF_DATA" 60 | grep -m1 '^TERRAIN_RING:')"
  say "  $(send "DUMP_MESHES ${MESHFILTER:-}" 90)"
  if [ -f "$D/Files/mesh_census.txt" ]; then
    cp "$D/Files/mesh_census.txt" "$CEN/${DEMO// /_}_census.txt"; say "    -> ${DEMO// /_}_census.txt"
  else
    say "    NO CENSUS FILE WRITTEN"
  fi
done
taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true
say "=== done -> $CEN ==="
