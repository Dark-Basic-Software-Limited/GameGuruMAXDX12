#!/bin/bash
# FPS survey v3 (2026-08-01, build engine 53481336 / game 0627f986 — streaming ON).
# Same method as the 07-29/07-30/07-31 baselines so numbers stay comparable:
#   fresh MAX launch -> first level -> 30s soak at start camera -> 3 editor FPS samples
#   -> CLICK test_level -> WAIT FOR THE PREPARING LOOP TO CLEAR -> 3 in-game samples.
#
# The prep gate is the whole reason for v3. On 07-31 the sweep sampled Horseshoe Bend
# DURING "PREPARING TEST LEVEL - N/100" and recorded 3.7 FPS, which is a loading screen,
# not gameplay. Gate on the text clearing (WETEST GET_SCREEN_TEXT trap note).
D="/d/DEV/BUILD/GameGuru Wicked MAX Build Area/Max"
OUT="/c/Users/leeba/AppData/Local/Temp/claude/D--max-GameGuruMAXDX12--claude-worktrees-determined-chebyshev-bf0892/9a28c586-4c13-4447-916e-7fb51301bfa8/scratchpad/demo_fps"
mkdir -p "$OUT/run0801"
RESULTS="$OUT/results_0801.txt"
: > "$RESULTS"

send() {
  rm -f "$D/auto_result.txt"
  echo "$1" > "$D/auto_command.txt"
  local t=0
  local timeout=${2:-30}
  while [ $t -lt $timeout ]; do
    if [ -f "$D/auto_result.txt" ]; then cat "$D/auto_result.txt"; return 0; fi
    sleep 1; t=$((t+1))
  done
  echo "TIMEOUT"; return 1
}

alive() { tasklist.exe //FI "IMAGENAME eq GameGuruMAX.exe" 2>/dev/null | grep -qi GameGuruMAX; }

wait_state() { # $1 = wanted state, $2 = max seconds
  local t=0
  while [ $t -lt $2 ]; do
    if ! alive; then return 2; fi
    local s=$(send "GET_STATE" 20 | head -1)
    if [ "$s" == "STATE: $1" ]; then return 0; fi
    sleep 5; t=$((t+7))
  done
  return 1
}

# Poll until the "PREPARING TEST LEVEL - N/100" overlay is gone. Returns the seconds waited.
wait_prep_clear() {
  local t=0
  local maxs=${1:-240}
  while [ $t -lt $maxs ]; do
    if ! alive; then echo "$t"; return 2; fi
    local txt=$(send "GET_SCREEN_TEXT" 25)
    if ! echo "$txt" | grep -qi "PREPARING"; then echo "$t"; return 0; fi
    sleep 5; t=$((t+7))
  done
  echo "$t"; return 1
}

DEMOS=(
"Aztec Game Kit Teaser"
"Aztec Game Kit"
"Bounty"
"Horseshoe Bend"
"Island Showdown"
"Operation Amazon"
"River Raiders"
"Snowy Mountain Stroll"
"A Grand Canyon Adventure"
"Disruption"
"Foggy Forest"
"Indian Strike Force"
"Switch Escape"
"Canyon Offensive"
"Escape from the Zombie Cellar"
"Jungle Fever"
"RPG Template"
"The Mystery of Z Island"
"Trapped"
)

for demo in "${DEMOS[@]}"; do
  echo "### $demo — $(date +%H:%M:%S)"
  taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true
  sleep 4
  cd "$D" && ./GameGuruMAX.exe > /dev/null 2>&1 &
  sleep 15

  if ! wait_state "hub" 60; then echo "$demo|FAIL_HUB|" >> "$RESULTS"; echo "  FAIL: never reached hub"; continue; fi
  sleep 4

  R=$(send "SELECT_DEMO $demo" 15)
  if [[ "$R" != OK:* ]]; then echo "$demo|FAIL_SELECT|$R" >> "$RESULTS"; echo "  FAIL select"; continue; fi
  sleep 3

  R=$(send "CLICK edit_game" 15)
  if [[ "$R" != OK:* ]]; then echo "$demo|FAIL_EDIT|$R" >> "$RESULTS"; echo "  FAIL edit"; continue; fi
  sleep 8

  if ! wait_state "storyboard" 40; then echo "$demo|FAIL_STORYBOARD|" >> "$RESULTS"; echo "  FAIL: no storyboard"; continue; fi

  R=$(send "CLICK_ONLY_LEVEL" 15)
  if [[ "$R" != OK:* ]]; then echo "$demo|FAIL_LEVEL|$R" >> "$RESULTS"; echo "  FAIL level"; continue; fi
  LEVELINFO=$(echo "$R" | head -1)

  if ! wait_state "editor" 180; then echo "$demo|FAIL_EDITOR|" >> "$RESULTS"; echo "  FAIL: editor never reached"; continue; fi

  sleep 30  # soak at start camera

  send "GET_PERF_DATA" 25 > "$OUT/run0801/ed1_$demo.txt"; sleep 4
  send "GET_PERF_DATA" 25 > "$OUT/run0801/ed2_$demo.txt"; sleep 4
  send "GET_PERF_DATA" 25 > "$OUT/run0801/ed3_$demo.txt"

  F1=$(grep -m1 "^FPS:" "$OUT/run0801/ed1_$demo.txt" | awk '{print $2}')
  F2=$(grep -m1 "^FPS:" "$OUT/run0801/ed2_$demo.txt" | awk '{print $2}')
  F3=$(grep -m1 "^FPS:" "$OUT/run0801/ed3_$demo.txt" | awk '{print $2}')
  VRAM=$(grep -m1 "^VRAM:" "$OUT/run0801/ed3_$demo.txt" | sed 's/.*driver_usage_mb=\([0-9.]*\).*/\1/')
  POLYS=$(grep -m1 "^POLYS:" "$OUT/run0801/ed3_$demo.txt" | tr -d '\r')

  # ---- test-game phase, gated on the PREPARING overlay clearing ----
  GF1=""; GF2=""; GF3=""; GSTATE="NOGAME"; PREPS=""
  R=$(send "CLICK test_level" 15)
  if [[ "$R" == OK:* ]]; then
    sleep 12
    if wait_state "game" 90; then
      PREPS=$(wait_prep_clear 240)
      if alive; then
        GSTATE="GAME"
        sleep 15  # settle: spawn scripts, dynamic res, LOS caches, streaming equilibrium
        send "GET_PERF_DATA" 25 > "$OUT/run0801/gm1_$demo.txt"; sleep 5
        send "GET_PERF_DATA" 25 > "$OUT/run0801/gm2_$demo.txt"; sleep 5
        send "GET_PERF_DATA" 25 > "$OUT/run0801/gm3_$demo.txt"
        GF1=$(grep -m1 "^FPS:" "$OUT/run0801/gm1_$demo.txt" | awk '{print $2}')
        GF2=$(grep -m1 "^FPS:" "$OUT/run0801/gm2_$demo.txt" | awk '{print $2}')
        GF3=$(grep -m1 "^FPS:" "$OUT/run0801/gm3_$demo.txt" | awk '{print $2}')
      else
        GSTATE="DIED_IN_PREP"
      fi
    else
      GSTATE="NO_GAME_STATE"
    fi
  fi

  echo "$demo|OK|$F1|$F2|$F3|$GSTATE|$GF1|$GF2|$GF3|prep=${PREPS}s|$VRAM|$POLYS|$LEVELINFO" >> "$RESULTS"
  echo "  OK: editor $F1/$F2/$F3  game($GSTATE) $GF1/$GF2/$GF3  prep=${PREPS}s"
done

taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true
echo "SWEEP COMPLETE $(date +%H:%M:%S)"
cat "$RESULTS"
