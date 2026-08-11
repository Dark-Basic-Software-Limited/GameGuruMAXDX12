#!/bin/bash
# Visual + cost A/B of the wi::terrain chunk ring radius across several `generation` values.
#
# WHY A SCREENSHOT SWEEP AND NOT JUST needGen: TERRAIN_RING's needGen only covers the PLAYABLE
# map (+/-editable_size). Players see far past it — Island Showdown's map is 250m half-extent
# but terrain is built to 1878m, so 12 of its 14 rings are scenery outside the editable area.
# Whether those rings are visible (horizon silhouette) or not (below the ocean / inside fog) is
# an eye question, so this captures the same two views at each generation for comparison.
#
# Usage: ringvisual.sh <demo> [gen ...]     default gens: 14 12 10 8
#
# Each arm is a COLD LAUNCH: `generation` is latched once by GGTerrainWicked_Init, so setup.ini
# terraingen=<N> is the only way to vary it (the same one-shot trap that made the 2026-08-09
# SET_TREES pool A/B measure nothing — see SWITCHESCAPE_PERF.md §2).
#
# ⚠ Writes setup.ini and RESTORES IT ON EXIT (trap). A stray key left behind contaminates every
#   later run — the standing setup.ini-cleanliness check exists because of exactly that.
set -u
D="/d/DEV/BUILD/GameGuru Wicked MAX Build Area/Max"
OUT="$(dirname "$0")"
DEMO="${1:?need a demo name}"; shift || true
if [ "$#" -gt 0 ]; then GENS=("$@"); else GENS=(14 12 10 8); fi
SHOTS="$OUT/ringshots"; mkdir -p "$SHOTS"
RES="$OUT/ring_visual.txt"; LOG="$OUT/ring_visual.log"
INI="$D/setup.ini"

LOCK="$OUT/.ringvisual.lock"
if [ -e "$LOCK" ] && kill -0 "$(cat "$LOCK" 2>/dev/null)" 2>/dev/null; then
  echo "REFUSING TO START: ringvisual.sh already running as PID $(cat "$LOCK")."; exit 3
fi
echo $$ > "$LOCK"

# Restore setup.ini no matter how we leave.
cp "$INI" "$OUT/.setup.ini.bak"
cleanup() { cp "$OUT/.setup.ini.bak" "$INI" 2>/dev/null; rm -f "$LOCK"; }
trap cleanup EXIT INT TERM

: > "$RES"; : > "$LOG"
say() { echo "$(date +%H:%M:%S) $*" >> "$LOG"; echo "$(date +%H:%M:%S) $*" >&2; }
send() { rm -f "$D/auto_result.txt"; echo "$1" > "$D/auto_command.txt"; local t=0
  while [ $t -lt ${2:-40} ]; do if [ -f "$D/auto_result.txt" ]; then sleep 0.3; cat "$D/auto_result.txt"; return 0; fi; sleep 1; t=$((t+1)); done; echo TIMEOUT; }
alive() { tasklist.exe //FI "IMAGENAME eq GameGuruMAX.exe" 2>/dev/null | grep -qi GameGuruMAX; }
wait_state() { local t=0; while [ $t -lt $2 ]; do alive || return 2
  [ "$(send "GET_STATE" 20|head -1)" == "STATE: $1" ] && return 0; sleep 5; t=$((t+7)); done; return 1; }
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
# Copy the file the harness just wrote to a name that says which arm produced it.
grab() {
  local tag="$1" r; r=$(send "SCREENSHOT" 60)
  local p="${r#OK: Screenshot saved to }"
  if [ -f "$p" ]; then cp "$p" "$SHOTS/${tag}.png"; say "    shot -> ${tag}.png"
  else say "    SHOT FAILED: $r"; fi
}

say "=== ring visual sweep: demo=$DEMO gens=${GENS[*]} ==="
for G in "${GENS[@]}"; do
  say "--- generation=$G ---"
  taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true; sleep 4
  # rewrite the key from the pristine backup so arms never stack
  grep -viE '^[[:space:]]*terraingen[[:space:]]*=' "$OUT/.setup.ini.bak" > "$INI"
  echo "terraingen=$G" >> "$INI"
  rm -f "$D/auto_command.txt" "$D/auto_result.txt"
  ( cd "$D" && ./GameGuruMAX.exe >/dev/null 2>&1 & )
  sleep 15
  if ! wait_state "hub" 90; then say "  FAIL_HUB"; echo "gen=$G FAIL_HUB" >> "$RES"; continue; fi
  sleep 4
  R=$(send "SELECT_DEMO $DEMO" 20)
  if [[ "$R" != OK:* ]]; then say "  FAIL_SELECT $R"; echo "gen=$G FAIL_SELECT" >> "$RES"; continue; fi
  sleep 3; send "CLICK edit_game" 20 >/dev/null; sleep 10
  send "CLICK_ONLY_LEVEL" 120 >/dev/null; sleep 50
  wait_state "editor" 240 || say "  WARN not editor"
  wait_chunks 240
  sleep 10   # let the VT finish resolving before the shot

  # View 1: the level's own default camera — what a user actually opens the level to.
  grab "${DEMO// /_}_gen${G}_default"
  CAM=$(send "GET_CAMERA" 20); say "    default cam: $CAM"

  # View 2: high vantage over the map centre looking out at the horizon. This is where a
  # shortened ring would show first — at ground level fog and the ocean hide the edge.
  send "SET_CAMERA 0 40000 0 12 0" 20 >/dev/null; sleep 12
  grab "${DEMO// /_}_gen${G}_horizon"

  d=$(send "GET_PERF_DATA" 60)
  ring=$(echo "$d" | grep -m1 '^TERRAIN_RING:')
  obj=$(echo "$d" | grep -m1 '^SCENE_OBJECTS:'   | awk '{print $2}')
  msh=$(echo "$d" | grep -m1 '^SCENE_MESHES:'    | awk '{print $2}')
  mat=$(echo "$d" | grep -m1 '^SCENE_MATERIALS:' | awk '{print $2}')
  tra=$(echo "$d" | grep -m1 '^SCENE_TRANSFORMS:'| awk '{print $2}')
  hie=$(echo "$d" | grep -m1 '^SCENE_HIERARCHY:' | awk '{print $2}')
  pol=$(echo "$d" | grep -m1 '^POLYS:'           | awk '{print $2}')
  vram=$(echo "$d" | grep -m1 '^VRAM_USED_MB:'   | awk '{print $2}')
  say "  $ring"
  say "  objects=$obj meshes=$msh materials=$mat transforms=$tra hierarchy=$hie polys=$pol vram=$vram"
  echo "gen=$G ${ring#TERRAIN_RING: } objects=$obj meshes=$msh materials=$mat transforms=$tra hierarchy=$hie polys=$pol vram=$vram" >> "$RES"
done
taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true
say "=== done -> $RES ; shots in $SHOTS ==="
