#!/bin/bash
# NAME THE VRAM RETAINED ACROSS A LEVEL LOAD.
#
# The census (engine 1.70) records every D3D12MA allocation with its debug name and erases the
# record in ~Resource_DX12, so a resource still present after its level was unloaded is a genuine
# retention, not driver caching. Load A, B, A, B in ONE session and dump at each step: diffing
# A1 against A2 - the SAME level, same camera, same geometry - names whatever survived.
#
# Also marks VRAM_STAGE at each step so the driver-usage curve is attributable alongside.
set -u
D="/d/DEV/BUILD/GameGuru Wicked MAX Build Area/Max"
OUT="/c/Users/leeba/AppData/Local/Temp/claude/D--max-GameGuruMAXDX12/9a28c586-4c13-4447-916e-7fb51301bfa8/scratchpad/vramleak"
mkdir -p "$OUT"; RES="$OUT/log.txt"; : > "$RES"
LOCK="$OUT/.lock"
if [ -e "$LOCK" ] && kill -0 "$(cat "$LOCK" 2>/dev/null)" 2>/dev/null; then echo "REFUSING"; exit 3; fi
echo $$ > "$LOCK"; trap 'rm -f "$LOCK"' EXIT INT TERM
say(){ echo "$(date +%H:%M:%S) $*" | tee -a "$RES"; }
send(){ rm -f "$D/auto_result.txt"; echo "$1" > "$D/auto_command.txt"; local t=0; local to=${2:-30}
  while [ $t -lt $to ]; do [ -f "$D/auto_result.txt" ] && { sleep 0.2; cat "$D/auto_result.txt"; return 0; }; sleep 1; t=$((t+1)); done
  echo TIMEOUT; return 1; }
alive(){ tasklist.exe //FI "IMAGENAME eq GameGuruMAX.exe" 2>/dev/null | grep -qi GameGuruMAX; }
killmax(){ local i=0; while [ $i -lt 10 ]; do taskkill.exe //IM GameGuruMAX.exe //F >/dev/null 2>&1; sleep 2; alive || return 0; i=$((i+1)); done; return 1; }
wait_state(){ local t=0; while [ $t -lt $2 ]; do alive || return 2
  [ "$(send "GET_STATE" 20 | head -1)" == "STATE: $1" ] && return 0; sleep 5; t=$((t+7)); done; return 1; }
load(){ send "NAVIGATE hub" 20 >/dev/null; sleep 6; send "NAVIGATE hub.demo_games" 20 >/dev/null; sleep 4
  send "SELECT_DEMO $1" 20 >/dev/null; sleep 3; send "CLICK edit_game" 20 >/dev/null; sleep 8
  wait_state "storyboard" 90 || return 1; send "CLICK_ONLY_LEVEL" 20 >/dev/null
  wait_state "editor" 420 || return 1; }

grab(){ # $1 = tag
  send "VRAM_STAGE $1" 25 >/dev/null
  send "DUMP_VRAM $1" 90 >/dev/null
  sleep 3
  local src="$D/Files/vram_census_$1.txt"
  [ -f "$src" ] || src="$D/vram_census_$1.txt"
  if [ -f "$src" ]; then cp -f "$src" "$OUT/$1.txt"
    say "   $1: $(head -1 "$OUT/$1.txt" | tr ' ' '\n' | grep -E '^(records|census_bytes|census_default_heap|driver_usage)=' | tr '\n' ' ')"
  else say "   $1: CENSUS FILE NOT FOUND"; fi
}

killmax; rm -f "$D/auto_command.txt" "$D/auto_result.txt" "$D/Files/vram_census_"*.txt
( cd "$D" && ./GameGuruMAX.exe >/dev/null 2>&1 & ); sleep 22
wait_state "hub" 180 || { say "FATAL no hub"; exit 1; }; sleep 5
grab boot

step(){ # $1 = demo, $2 = tag
  say "=== load $1 -> $2"
  load "$1" || { say "   LOAD FAILED"; return 1; }
  sleep 60                     # settle: streaming, VT, tree pool (60 s, per 3.64)
  grab "$2"
}

step "Operation Amazon" A1
step "RPG Template"     B1
step "Operation Amazon" A2
step "RPG Template"     B2
# a third A proves whether retention is per-load linear or one-shot
step "Operation Amazon" A3

send "QUIT" 10 >/dev/null 2>&1; t=0; while alive && [ $t -lt 25 ]; do sleep 1; t=$((t+1)); done; killmax
say "VRAM LEAK PROBE DONE"
