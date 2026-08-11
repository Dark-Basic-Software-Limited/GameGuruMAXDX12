#!/bin/bash
# ENTITY-RETENTION (leak) probe.
#
# QUESTION: does changing level inside one session leave the previous level's ECS entities
# behind? Wicked's Entity_Remove on an ObjectComponent does not necessarily destroy the
# MeshComponent it referenced, and the empty-level panels hinted at it: on delete-all DX11 freed
# 92 meshes where DX12 freed only 34.
#
# DESIGN: capture the SAME target level's scene counts two ways.
#   arm FRESH : launch -> target -> capture
#   arm AFTER : launch -> decoy -> back to hub -> target -> capture
# Identical counts => nothing retained across a level change. AFTER > FRESH => that difference
# IS the retained population, and its size.
#
# This beats a delete-all comparison: no harness command deletes all objects, and level-change
# is the operation that actually happens in use.
set -u
D="/d/DEV/BUILD/GameGuru Wicked MAX Build Area/Max"
OUT="$(dirname "$0")"; RES="$OUT/su_leak.txt"
TARGET="${1:-Switch Escape}"; DECOY="${2:-Island Showdown}"
LOCK="$OUT/.sumeasure.lock"
if [ -e "$LOCK" ] && kill -0 "$(cat "$LOCK" 2>/dev/null)" 2>/dev/null; then echo "REFUSING: PID $(cat "$LOCK")"; exit 3; fi
echo $$ > "$LOCK"; trap 'rm -f "$LOCK"' EXIT INT TERM
: > "$RES"
say(){ echo "$(date +%H:%M:%S) $*" | tee -a "$RES"; }
send(){ rm -f "$D/auto_result.txt"; echo "$1" > "$D/auto_command.txt"; local t=0
  while [ $t -lt ${2:-40} ]; do if [ -f "$D/auto_result.txt" ]; then sleep 0.3; cat "$D/auto_result.txt"; return 0; fi; sleep 1; t=$((t+1)); done; echo TIMEOUT; }
alive(){ tasklist.exe //FI "IMAGENAME eq GameGuruMAX.exe" 2>/dev/null | grep -qi GameGuruMAX; }
wait_state(){ local t=0; while [ $t -lt $2 ]; do alive || return 2
  [ "$(send "GET_STATE" 20|head -1)" == "STATE: $1" ] && return 0; sleep 5; t=$((t+7)); done; return 1; }
enter(){ # $1 = demo name
  R=$(send "SELECT_DEMO $1" 20); [[ "$R" == OK:* ]] || { say "  FAIL_SELECT $1 -> $R"; return 1; }
  sleep 3; send "CLICK edit_game" 20 >/dev/null; sleep 10
  send "CLICK_ONLY_LEVEL" 120 >/dev/null; sleep 60
  wait_state "editor" 300 || say "  WARN not editor after $1"
  sleep 25; return 0; }
capture(){ # $1 = label
  local d=$(send "GET_PERF_DATA" 60)
  local line=$(echo "$d" | grep -E "^SCENE_MESHES:|^SCENE_MATERIALS:|^SCENE_TRANSFORMS:|^SCENE_HIERARCHY:|^SCENE_OBJECTS:|^POLYS:" | tr '\n' ' ')
  local h=$(echo "$d" | grep -m1 '^HIER:')
  say "  [$1] $line"
  say "  [$1] $h"; }
launch(){ taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true; sleep 4
  rm -f "$D/auto_command.txt" "$D/auto_result.txt"
  cd "$D" && ./GameGuruMAX.exe >/dev/null 2>&1 &
  sleep 15; wait_state "hub" 90 || { say FAIL_HUB; exit 1; }; sleep 4; }

say "=== ARM FRESH: $TARGET loaded first in a clean session ==="
launch; enter "$TARGET" && capture "FRESH"

say "=== ARM AFTER: $DECOY first, back to hub, then $TARGET ==="
launch; enter "$DECOY" && capture "decoy=$DECOY"
say "  -> NAVIGATE hub"
send "NAVIGATE hub" 60 >/dev/null; sleep 12
wait_state "hub" 120 || say "  WARN did not return to hub"
sleep 4
enter "$TARGET" && capture "AFTER"

# GGMAX 2.23 gate: after a release, a TREE level must rebuild the pool and still draw its trees.
# This is the regression the release could plausibly cause, so it is tested in the same session,
# immediately after the release has happened.
say "=== ARM REGROW: back to $DECOY after the release — trees must come back ==="
say "  -> NAVIGATE hub"
send "NAVIGATE hub" 60 >/dev/null; sleep 12
wait_state "hub" 120 || say "  WARN did not return to hub"
sleep 4
enter "$DECOY" && capture "REGROW=$DECOY"
send "DUMP_TREEPOOL" 60 >/dev/null; sleep 2
say "  $(head -1 "$D/Files/treepool_dump.txt" 2>/dev/null)"

taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true
say "=== DONE ==="
say "  PASS if: AFTER == FRESH (leak gone) AND REGROW POLYS == the decoy's reference (trees back)"
