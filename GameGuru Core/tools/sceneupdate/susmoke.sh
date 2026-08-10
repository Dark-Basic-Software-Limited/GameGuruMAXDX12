#!/bin/bash
# Correctness smoke test for the wiECS LOOKUP_SPARSE switch. The ECS lookup is used by EVERY
# system, so the gate is not performance but GEOMETRY IDENTITY: POLYS must match the reference
# sweep exactly on each demo, and the level must reach the editor without dying.
set -u
D="/d/DEV/BUILD/GameGuru Wicked MAX Build Area/Max"
OUT="$(dirname "$0")"; RES="$OUT/su_smoke.txt"; LOG="$OUT/su_smoke.log"
: > "$RES"; : > "$LOG"
say(){ echo "$(date +%H:%M:%S) $*" | tee -a "$LOG"; }
send(){ rm -f "$D/auto_result.txt"; echo "$1" > "$D/auto_command.txt"; local t=0
  while [ $t -lt ${2:-40} ]; do if [ -f "$D/auto_result.txt" ]; then sleep 0.3; cat "$D/auto_result.txt"; return 0; fi; sleep 1; t=$((t+1)); done; echo TIMEOUT; }
alive(){ tasklist.exe //FI "IMAGENAME eq GameGuruMAX.exe" 2>/dev/null | grep -qi GameGuruMAX; }
wait_state(){ local t=0; while [ $t -lt $2 ]; do alive || return 2
  [ "$(send "GET_STATE" 20|head -1)" == "STATE: $1" ] && return 0; sleep 5; t=$((t+7)); done; return 1; }

# demo | expected POLYS (from tools/singlequeue_sweep_0809_full.txt, same content build)
CASES=("Island Showdown|4114598" "Trapped|" "Switch Escape|109358")
for C in "${CASES[@]}"; do
  DEMO="${C%%|*}"; EXP="${C##*|}"
  say "######## $DEMO (expect POLYS=${EXP:-unknown}) ########"
  taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true; sleep 4
  rm -f "$D/auto_command.txt" "$D/auto_result.txt"
  cd "$D" && ./GameGuruMAX.exe >/dev/null 2>&1 &
  sleep 15
  if ! wait_state "hub" 90; then say "  FAIL_HUB"; echo "$DEMO|FAIL_HUB" >> "$RES"; continue; fi
  sleep 4
  R=$(send "SELECT_DEMO $DEMO" 20)
  if [[ "$R" != OK:* ]]; then say "  FAIL_SELECT"; echo "$DEMO|FAIL_SELECT" >> "$RES"; continue; fi
  sleep 3; send "CLICK edit_game" 20 >/dev/null; sleep 10
  send "CLICK_ONLY_LEVEL" 120 >/dev/null; sleep 60
  if ! wait_state "editor" 300; then say "  WARN not editor"; fi
  sleep 25
  if ! alive; then say "  DIED"; echo "$DEMO|DIED" >> "$RES"; continue; fi
  d=$(send "GET_PERF_DATA" 60)
  f=$(echo "$d"|grep -m1 '^FPS:'|awk '{print $2}'); p=$(echo "$d"|grep -m1 '^POLYS:'|awk '{print $2}')
  o=$(echo "$d"|grep -m1 '^SCENE_OBJECTS:'|awk '{print $2}'); v=$(echo "$d"|grep -m1 '^VRAM'|head -1)
  VER="n/a"; if [ -n "$EXP" ]; then if [ "$p" == "$EXP" ]; then VER="POLYS_MATCH"; else VER="POLYS_MISMATCH(exp $EXP)"; fi; fi
  say "  fps=$f polys=$p objects=$o -> $VER"
  echo "$DEMO|fps=$f|polys=$p|objects=$o|$VER" >> "$RES"
  send "SCREENSHOT" 60 >/dev/null
done
taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true
say "=== SMOKE DONE ==="
