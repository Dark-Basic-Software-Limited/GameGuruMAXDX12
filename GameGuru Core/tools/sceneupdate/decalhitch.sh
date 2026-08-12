#!/bin/bash
# GATE for the GGMAX 2.27 decal pool (prewarm + grow) — does on-demand growth cause a HITCH?
#
# THE QUESTION: 2.27 stops building all 100 decal-pool quads at startup and grows them on demand.
# Decals are allocated at RUNTIME from bullet impacts, so the risk is a stall mid-firefight.
#
# METHOD — the tail, not the mean:
#   arm CONTROL : setup.ini decalprewarm=99  == the pre-2.27 eager pool, exactly
#   arm PREWARM : setup.ini decalprewarm=24  == the shipping default
# Same binary, cold launch each. In each arm: settle -> HITCH_RESET -> DECAL_BURST xN -> read the
# HITCH: histogram. A growth stall lands in worst_ms / the over-buckets and is INVISIBLE in mean
# FPS, which is why engine 1.82's hitch instrument exists at all.
#
# ⚠ Judge worst_ms and over(16.7/25/33), never FPS.
# ⚠ DECALPOOL: built= must DIFFER between arms or the knob never reached the pool and both arms
#   measured the same thing (the SET_TREES pool trap, SWITCHESCAPE_PERF.md §2).
#
# Usage: decalhitch.sh [demo] [burst] [repeats]
set -u
D="/d/DEV/BUILD/GameGuru Wicked MAX Build Area/Max"
OUT="$(dirname "$0")"
DEMO="${1:-Switch Escape}"; BURST="${2:-120}"; REPS="${3:-4}"
RES="$OUT/decal_hitch.txt"; LOG="$OUT/decal_hitch.log"; INI="$D/setup.ini"

LOCK="$OUT/.decalhitch.lock"
if [ -e "$LOCK" ] && kill -0 "$(cat "$LOCK" 2>/dev/null)" 2>/dev/null; then
  echo "REFUSING TO START: decalhitch.sh already running as PID $(cat "$LOCK")."; exit 3
fi
echo $$ > "$LOCK"
cp "$INI" "$OUT/.setup.ini.decal.bak"
cleanup() { cp "$OUT/.setup.ini.decal.bak" "$INI" 2>/dev/null; rm -f "$LOCK" "$OUT/.setup.ini.decal.bak"; }
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
    if [ "$c" == "$prev" ]; then good=$((good+1)); [ $good -ge 2 ] && return 0; else good=0; fi
    prev=$c
  done
  return 1
}

run_arm() {
  local LABEL="$1" PW="$2"
  say "===== arm $LABEL (decalprewarm=$PW) ====="
  taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true; sleep 4
  grep -viE '^[[:space:]]*decalprewarm[[:space:]]*=' "$OUT/.setup.ini.decal.bak" > "$INI"
  echo "decalprewarm=$PW" >> "$INI"
  rm -f "$D/auto_command.txt" "$D/auto_result.txt"
  ( cd "$D" && ./GameGuruMAX.exe >/dev/null 2>&1 & )
  sleep 15
  wait_state "hub" 90 || { say "  FAIL_HUB"; echo "$LABEL FAIL_HUB" >> "$RES"; return 1; }
  sleep 4
  R=$(send "SELECT_DEMO $DEMO" 20); [[ "$R" == OK:* ]] || { say "  FAIL_SELECT"; echo "$LABEL FAIL_SELECT" >> "$RES"; return 1; }
  sleep 3; send "CLICK edit_game" 20 >/dev/null; sleep 10
  send "CLICK_ONLY_LEVEL" 120 >/dev/null; sleep 50
  wait_state "editor" 240 || say "  WARN not editor"
  wait_chunks 240
  sleep 10

  # KNOB-REACHES-THE-THING CHECK, before any timing is believed.
  local pool0; pool0=$(send "GET_PERF_DATA" 60 | grep -m1 '^DECALPOOL:')
  say "  pre-burst  $pool0"
  echo "$LABEL PRE $pool0" >> "$RES"

  send "HITCH_RESET" 30 >/dev/null
  sleep 3
  local i
  for i in $(seq 1 "$REPS"); do
    say "  burst $i/$REPS: $(send "DECAL_BURST $BURST" 60)"
    sleep 6
  done
  sleep 4
  local d; d=$(send "GET_PERF_DATA" 60)
  local hitch pool1 fps
  hitch=$(echo "$d" | grep -m1 '^HITCH:')
  pool1=$(echo "$d" | grep -m1 '^DECALPOOL:')
  fps=$(echo "$d"  | grep -m1 '^FPS:' | awk '{print $2}')
  say "  post-burst $pool1"
  say "  $hitch"
  echo "$LABEL POST $pool1 fps=$fps" >> "$RES"
  echo "$LABEL $hitch" >> "$RES"
}

say "=== decal pool hitch gate: demo=$DEMO burst=$BURST x$REPS ==="
run_arm CONTROL 99
run_arm PREWARM 24
taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true
say "=== done -> $RES ==="
echo; echo "===== RESULT ====="; cat "$RES"
