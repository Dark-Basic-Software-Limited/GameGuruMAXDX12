#!/bin/bash
# THE 19-DEMO VRAM UNION SWEEP.
#
# The A/B/A probe proves retention between TWO levels. It cannot prove the 19-demo case, because
# nineteen demos hold far more distinct content than two and a mechanism that saturates at two
# levels can still climb for nineteen. This walks all 19 in one session and dumps the full named
# census after each, so what accumulates is attributable BY NAME at every step rather than inferred.
#
# No Test Game here: editor load only, which is the cheapest thing that exercises level teardown.
# Pair it with the soak sweep when the question is gameplay rather than memory.
set -u
D="/d/DEV/BUILD/GameGuru Wicked MAX Build Area/Max"
OUT="${1:-/c/Users/leeba/AppData/Local/Temp/claude/D--max-GameGuruMAXDX12/9a28c586-4c13-4447-916e-7fb51301bfa8/scratchpad/vram_union}"
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
grab(){ send "DUMP_VRAM $1" 90 >/dev/null; sleep 3
  local src="$D/Files/vram_census_$1.txt"; [ -f "$src" ] || src="$D/vram_census_$1.txt"
  if [ -f "$src" ]; then cp -f "$src" "$OUT/$1.txt"
    say "   $1 $(head -1 "$OUT/$1.txt" | tr ' ' '\n' | grep -E '^(records|census_bytes|driver_usage)=' | tr '\n' ' ')"
  else say "   $1 CENSUS FILE NOT FOUND"; fi; }

DEMOS=(
"Aztec Game Kit Teaser" "Aztec Game Kit" "Bounty" "Horseshoe Bend" "Island Showdown"
"Operation Amazon" "River Raiders" "Snowy Mountain Stroll" "A Grand Canyon Adventure"
"Disruption" "Foggy Forest" "Indian Strike Force" "Switch Escape" "Canyon Offensive"
"Escape from the Zombie Cellar" "Jungle Fever" "RPG Template" "The Mystery of Z Island" "Trapped"
)

killmax; rm -f "$D/auto_command.txt" "$D/auto_result.txt" "$D/Files/vram_census_"*.txt
( cd "$D" && ./GameGuruMAX.exe >/dev/null 2>&1 & ); sleep 22
wait_state "hub" 180 || { say "FATAL no hub"; exit 1; }; sleep 5
grab d00_boot

I=0
for demo in "${DEMOS[@]}"; do
  I=$((I+1)); TAG=$(printf "d%02d" $I)
  say "=== [$I/19] $demo"
  if ! alive; then say "   !! process gone - sweep aborted at $I"; break; fi
  if load "$demo"; then sleep 60; grab "$TAG"; else say "   LOAD FAILED"; fi
done
# THE DECISIVE STEP: load demo 1 again, now 19 loads deep. Comparing d20 against d01 holds level
# content constant across the whole session, which the two-level A/B/A/B probe could not do - with
# only two alternating levels, an orphaned-entity leak and a stale-paint-map leak predict exactly
# the same "current union previous" set and are indistinguishable.
say "=== [20/20] ${DEMOS[0]} AGAIN (same level as d01, 19 loads later)"
if alive && load "${DEMOS[0]}"; then sleep 60; grab d20_repeat; else say "   REPEAT LOAD FAILED"; fi

send "QUIT" 10 >/dev/null 2>&1; t=0; while alive && [ $t -lt 25 ]; do sleep 1; t=$((t+1)); done; killmax
say "UNION SWEEP DONE"
