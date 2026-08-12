#!/bin/bash
# COST GATE for GGMAX 2.28 C: what does widening the shadow-caster cull cost?
#
# 2.28 raises the directional cascade's culling extrusion from a stock 1000 units (25 m — a
# metres-scale constant in an inch-scale world) to gg_shadow_caster_extrude, default 4000 u.
# That pulls MORE casters into the near cascades, which are the expensive ones.
#
# SET_SHADOWEXTRUDE is fully live, so both arms run in ONE launch — no cross-launch FPS drift,
# which on this rig is +-8 FPS and would swamp the effect.
#
# Metric: the "Shadowmap Rendering" CPU range, NOT FPS. The editor frame is GPU-fence-bound, so a
# real cost can read 0 FPS (SWITCHESCAPE_PERF.md METHOD).
#
# Usage: shadowextrude.sh [demo] [samples]
set -u
D="/d/DEV/BUILD/GameGuru Wicked MAX Build Area/Max"
OUT="$(dirname "$0")"
DEMO="${1:-Island Showdown}"; NSAMP="${2:-5}"
RES="$OUT/shadow_extrude.txt"; LOG="$OUT/shadow_extrude.log"

LOCK="$OUT/.shadowextrude.lock"
if [ -e "$LOCK" ] && kill -0 "$(cat "$LOCK" 2>/dev/null)" 2>/dev/null; then
  echo "REFUSING TO START: already running as PID $(cat "$LOCK")."; exit 3
fi
echo $$ > "$LOCK"; trap 'rm -f "$LOCK"' EXIT INT TERM
: > "$RES"; : > "$LOG"
say() { echo "$(date +%H:%M:%S) $*" >> "$LOG"; echo "$(date +%H:%M:%S) $*" >&2; }
send() { rm -f "$D/auto_result.txt"; echo "$1" > "$D/auto_command.txt"; local t=0
  while [ $t -lt ${2:-40} ]; do if [ -f "$D/auto_result.txt" ]; then sleep 0.3; cat "$D/auto_result.txt"; return 0; fi; sleep 1; t=$((t+1)); done; echo TIMEOUT; }
alive() { tasklist.exe //FI "IMAGENAME eq GameGuruMAX.exe" 2>/dev/null | grep -qi GameGuruMAX; }
wait_state() { local t=0; while [ $t -lt $2 ]; do alive || return 2
  [ "$(send "GET_STATE" 20|head -1)" == "STATE: $1" ] && return 0; sleep 5; t=$((t+7)); done; return 1; }
wait_chunks() { local maxs=${1:-240} t=0 prev=-1 good=0 c
  while [ $t -lt $maxs ]; do sleep 15; t=$((t+15)); alive || return 2
    c=$(send "GET_PERF_DATA" 45 | grep -m1 '^TERRAIN_RING:' | sed -n 's/.*chunks=\([0-9]*\).*/\1/p')
    [ -z "$c" ] && continue
    if [ "$c" == "$prev" ]; then good=$((good+1)); [ $good -ge 2 ] && return 0; else good=0; fi
    prev=$c; done; return 1; }
row() { echo "$1" | grep -m1 -F "$2" | sed -n 's/.*: *\([0-9.]*\) ms.*/\1/p'; }

# One arm: set the knob, settle, sample the shadow pass.
arm() {
  local LABEL="$1" VAL="$2"
  say "-- arm $LABEL: $(send "SET_SHADOWEXTRUDE $VAL" 30)"
  sleep 8
  local sums="" i d sm vo
  for i in $(seq 1 "$NSAMP"); do
    sleep 4; d=$(send "GET_PERF_DATA" 60)
    sm=$(row "$d" "Shadowmap Rendering"); cast=$(echo "$d" | grep -m1 "^SHADOW_CASTERS:")
    vo=$(echo "$d" | grep -m1 '^VISIBLE_OBJECTS:' | awk '{print $2}')
    say "   shadowmap=${sm:-NA} ms  $cast  visible_objects=$vo"
    echo "$LABEL sample=$i shadowmap_ms=${sm:-NA} ${cast#SHADOW_CASTERS: } visible_objects=$vo" >> "$RES"
    sums="$sums ${sm:-0}"
  done
  local mean; mean=$(echo "$sums" | awk '{s=0;n=0;for(i=1;i<=NF;i++){s+=$i;n++} if(n)printf "%.3f", s/n}')
  say "== $LABEL mean shadowmap = $mean ms"
  echo "RESULT $LABEL mean_shadowmap_ms=$mean" >> "$RES"
}

say "=== shadow extrude cost gate: demo=$DEMO ==="
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
wait_chunks 240
send "ENABLE_PROFILER" 30 >/dev/null; sleep 8

# STOCK first, then the new default, then back to STOCK — the repeat is the drift control.
arm STOCK_1000 0
arm NEW_4000   4000
arm STOCK_REPEAT 0

send "DISABLE_PROFILER" 30 >/dev/null
taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true
say "=== done ==="; echo; cat "$RES" | grep RESULT
