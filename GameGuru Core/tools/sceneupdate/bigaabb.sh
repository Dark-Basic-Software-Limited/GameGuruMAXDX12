#!/bin/bash
# Run DUMP_BIGAABB on one or more demos: how much of a real level carries an inflated AABB?
#
# WHY: the corrupt-armature AABB is understood for ONE prop (a pistol on spotshadowtest). That
# says nothing about whether it is worth an engine-side fix. If it is a handful of pickups the
# answer is "leave it"; if every skinned character in every level carries a 100 km box then it
# is defeating culling shipping-wide. Reading code cannot answer that — only a census can.
#
# The gate is an INFLATION RATIO (actual radius vs the box the mesh alone would produce), so it
# stays valid whatever `masterpark` is set to. A coordinate threshold would report "0 affected"
# the moment the park point moved, and that zero would look like a fix.
#
# Usage: bigaabb.sh <demo> [demo ...]
set -u
D="/d/DEV/BUILD/GameGuru Wicked MAX Build Area/Max"
OUT="$(dirname "$0")"; CEN="$OUT/census"; mkdir -p "$CEN"
LOG="$CEN/bigaabb.log"

LOCK="$OUT/.bigaabb.lock"
if [ -e "$LOCK" ] && kill -0 "$(cat "$LOCK" 2>/dev/null)" 2>/dev/null; then
  echo "REFUSING TO START: bigaabb.sh already running as PID $(cat "$LOCK")."; exit 3
fi
echo $$ > "$LOCK"; trap 'rm -f "$LOCK"' EXIT INT TERM
: > "$LOG"
say() { echo "$(date +%H:%M:%S) $*" >> "$LOG"; echo "$(date +%H:%M:%S) $*" >&2; }
send() { rm -f "$D/auto_result.txt"; echo "$1" > "$D/auto_command.txt"; local t=0
  while [ $t -lt ${2:-40} ]; do if [ -f "$D/auto_result.txt" ]; then sleep 0.3; cat "$D/auto_result.txt"; return 0; fi; sleep 1; t=$((t+1)); done; echo TIMEOUT; }
alive() { tasklist.exe //FI "IMAGENAME eq GameGuruMAX.exe" 2>/dev/null | grep -qi GameGuruMAX; }
wait_state() { local t=0; while [ $t -lt $2 ]; do alive || return 2
  [ "$(send "GET_STATE" 20|head -1)" == "STATE: $1" ] && return 0; sleep 5; t=$((t+7)); done; return 1; }

for DEMO in "$@"; do
  say "--- $DEMO ---"
  taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true; sleep 4
  rm -f "$D/auto_command.txt" "$D/auto_result.txt" "$D/Files/dumpbigaabb.txt"
  ( cd "$D" && ./GameGuruMAX.exe >/dev/null 2>&1 & )
  sleep 15
  wait_state "hub" 90 || { say "  FAIL_HUB"; continue; }
  sleep 4
  R=$(send "SELECT_DEMO $DEMO" 20); [[ "$R" == OK:* ]] || { say "  FAIL_SELECT $R"; continue; }
  sleep 3; send "CLICK edit_game" 20 >/dev/null; sleep 10
  send "CLICK_ONLY_LEVEL" 120 >/dev/null; sleep 50
  wait_state "editor" 240 || say "  WARN not editor"
  sleep 20
  say "  $(send "DUMP_BIGAABB" 90)"
  if [ -f "$D/Files/dumpbigaabb.txt" ]; then
    cp "$D/Files/dumpbigaabb.txt" "$CEN/${DEMO// /_}_bigaabb.txt"; say "    -> ${DEMO// /_}_bigaabb.txt"
  else
    say "    NO OUTPUT FILE"
  fi
done
taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true
say "DONE"
