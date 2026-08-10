#!/bin/bash
# FPS survey v4 (2026-08-06, build engine a229fffe / game 50ca7c28 — the 2.07g light/shadow build).
# Same method as the 07-29/07-30/07-31/08-01 baselines so numbers stay comparable:
#   fresh MAX launch -> first level -> 30s soak at start camera -> 3 editor FPS samples
#   -> CLICK test_level -> WAIT FOR THE PREPARING LOOP TO CLEAR -> 3 in-game samples.
#
# The prep gate (v3) is load-bearing. On 07-31 the sweep sampled Horseshoe Bend
# DURING "PREPARING TEST LEVEL - N/100" and recorded 3.7 FPS, which is a loading screen,
# not gameplay. Gate on the text clearing (WETEST GET_SCREEN_TEXT trap note).
#
# v4 adds a per-demo EDITOR + GAME screenshot, copied out of Files/screenshots by demo name.
# Reason: 2.07g fixed an fp16 range^2 overflow that had deleted the point/spot attenuation
# falloff window, so every big-range light in every demo now falls off correctly (dimmer rim,
# longer true reach). Task #109 needs eyes-on evidence of that look change across the hub.
#
# Usage: demo_fps_sweep.sh [RUNTAG] [first-demo-index]
#   RUNTAG defaults to 0806; index lets a crashed run resume part-way through the list.
D="/d/DEV/BUILD/GameGuru Wicked MAX Build Area/Max"
SHOTS="$D/Files/screenshots"
TAG="${1:-0806}"
START_AT="${2:-0}"
OUT="/c/Users/leeba/AppData/Local/Temp/claude/D--max-GameGuruMAXDX12--claude-worktrees-determined-chebyshev-bf0892/9a28c586-4c13-4447-916e-7fb51301bfa8/scratchpad/demo_fps"
mkdir -p "$OUT/run$TAG" "$OUT/shots$TAG"
RESULTS="$OUT/results_$TAG.txt"

# ⚠ SINGLE-INSTANCE LOCK (added 2026-08-10; this script predates the rule).
# On 2026-08-09 three copies of a sweep ran concurrently against one MAX — a nohup'd launch
# that outlived its parent plus two relaunches after a `pkill` that silently does nothing under
# Git Bash on Windows. All three drove the SAME auto_command.txt and appended to the SAME log:
# 40 s arms "completing" 16 s apart, duplicate demo headers, one arm sampled mid-load at 3.9 FPS.
# None of it was detectable from the numbers alone. Kill strays with `ps -W` + `kill -9`.
LOCK="$OUT/.demo_fps_sweep.lock"
if [ -e "$LOCK" ] && kill -0 "$(cat "$LOCK" 2>/dev/null)" 2>/dev/null; then
  echo "REFUSING TO START: demo_fps_sweep.sh already running as PID $(cat "$LOCK"). Kill it first."
  exit 3
fi
echo $$ > "$LOCK"
trap 'rm -f "$LOCK"' EXIT INT TERM

[ "$START_AT" == "0" ] && : > "$RESULTS"

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

# Poll until EVERY test-game loading overlay is gone. Returns the seconds waited.
# v4a: gating on "PREPARING" alone was not enough. On 08-06 Horseshoe Bend cleared the
# PREPARING loop instantly (navmesh cache) and then sat in a SECOND overlay,
# "RASTERIZING NAVIGATION MESH - N\100 Complete", at 3 FPS — so the sampler recorded a
# loading screen exactly like the 07-31 fake collapse the gate was built to prevent.
# Gate on the shared "N\100 Complete" progress shape plus both known overlay names.
LOADING_OVERLAY='PREPARING|RASTERIZING|BUILDING|GENERATING|[0-9]+.?[0-9]*00 Complete'
wait_prep_clear() {
  local t=0
  local maxs=${1:-240}
  while [ $t -lt $maxs ]; do
    if ! alive; then echo "$t"; return 2; fi
    local txt=$(send "GET_SCREEN_TEXT" 25)
    if ! echo "$txt" | grep -qiE "$LOADING_OVERLAY"; then echo "$t"; return 0; fi
    sleep 5; t=$((t+7))
  done
  echo "$t"; return 1
}

# SCREENSHOT ignores its label and writes a timestamped sc_*.png — grab the newest afterwards.
grab_shot() { # $1 = destination basename
  send "SCREENSHOT" 25 > /dev/null
  sleep 3
  local newest=$(ls -t "$SHOTS"/sc_*.png 2>/dev/null | head -1)
  if [ -n "$newest" ]; then cp "$newest" "$OUT/shots$TAG/$1.png" 2>/dev/null; fi
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

IDX=-1
for demo in "${DEMOS[@]}"; do
  IDX=$((IDX+1))
  [ $IDX -lt $START_AT ] && continue
  echo "### [$IDX] $demo — $(date +%H:%M:%S)"
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

  send "GET_PERF_DATA" 25 > "$OUT/run$TAG/ed1_$demo.txt"; sleep 4
  send "GET_PERF_DATA" 25 > "$OUT/run$TAG/ed2_$demo.txt"; sleep 4
  send "GET_PERF_DATA" 25 > "$OUT/run$TAG/ed3_$demo.txt"

  F1=$(grep -m1 "^FPS:" "$OUT/run$TAG/ed1_$demo.txt" | awk '{print $2}')
  F2=$(grep -m1 "^FPS:" "$OUT/run$TAG/ed2_$demo.txt" | awk '{print $2}')
  F3=$(grep -m1 "^FPS:" "$OUT/run$TAG/ed3_$demo.txt" | awk '{print $2}')
  VRAM=$(grep -m1 "^VRAM:" "$OUT/run$TAG/ed3_$demo.txt" | sed 's/.*driver_usage_mb=\([0-9.]*\).*/\1/')
  POLYS=$(grep -m1 "^POLYS:" "$OUT/run$TAG/ed3_$demo.txt" | tr -d '\r')

  grab_shot "ed_$demo"

  # ---- test-game phase, gated on the PREPARING overlay clearing ----
  GF1=""; GF2=""; GF3=""; GSTATE="NOGAME"; PREPS=""; GVRAM=""
  R=$(send "CLICK test_level" 15)
  if [[ "$R" == OK:* ]]; then
    sleep 12
    if wait_state "game" 90; then
      PREPS=$(wait_prep_clear 240)
      if alive; then
        GSTATE="GAME"
        sleep 15  # settle: spawn scripts, dynamic res, LOS caches, streaming equilibrium
        send "GET_PERF_DATA" 25 > "$OUT/run$TAG/gm1_$demo.txt"; sleep 5
        send "GET_PERF_DATA" 25 > "$OUT/run$TAG/gm2_$demo.txt"; sleep 5
        send "GET_PERF_DATA" 25 > "$OUT/run$TAG/gm3_$demo.txt"
        GF1=$(grep -m1 "^FPS:" "$OUT/run$TAG/gm1_$demo.txt" | awk '{print $2}')
        GF2=$(grep -m1 "^FPS:" "$OUT/run$TAG/gm2_$demo.txt" | awk '{print $2}')
        GF3=$(grep -m1 "^FPS:" "$OUT/run$TAG/gm3_$demo.txt" | awk '{print $2}')
        GVRAM=$(grep -m1 "^VRAM:" "$OUT/run$TAG/gm3_$demo.txt" | sed 's/.*driver_usage_mb=\([0-9.]*\).*/\1/')
        grab_shot "gm_$demo"
        # The text gate is NOT sufficient on its own. Measured 08-06: Horseshoe Bend clears
        # "PREPARING TEST LEVEL" instantly (navmesh cache) and then spends ~a minute in
        # "RASTERIZING NAVIGATION MESH - N\100 Complete" at 3 FPS — and GET_SCREEN_TEXT cannot
        # see that banner at all (it is printscreenprompt, not ImGui text), so wait_prep_clear
        # returns 0s and the sampler lands in a loading screen exactly like the 07-31 fake
        # collapse. FPS recovery is the signal that actually works: no hub demo runs below
        # 20 FPS in real gameplay, so poll until it climbs out, then re-sample.
        if [ -n "$GF3" ] && [ "$(awk -v v="$GF3" 'BEGIN{print (v<20)?1:0}')" == "1" ]; then
          echo "  ...sub-20 FPS ($GF3) — polling for FPS recovery (loading screen, not gameplay)"
          rt=0
          while [ $rt -lt 300 ]; do
            if ! alive; then break; fi
            cur=$(send "GET_PERF_DATA" 25 | grep -m1 "^FPS:" | awk '{print $2}')
            [ -n "$cur" ] && [ "$(awk -v v="$cur" 'BEGIN{print (v>=20)?1:0}')" == "1" ] && break
            sleep 5; rt=$((rt+7))
          done
          PREPS="$PREPS+recovered@${rt}s"
          if alive; then
            sleep 20
            send "GET_PERF_DATA" 25 > "$OUT/run$TAG/gm1_$demo.txt"; sleep 5
            send "GET_PERF_DATA" 25 > "$OUT/run$TAG/gm2_$demo.txt"; sleep 5
            send "GET_PERF_DATA" 25 > "$OUT/run$TAG/gm3_$demo.txt"
            GF1=$(grep -m1 "^FPS:" "$OUT/run$TAG/gm1_$demo.txt" | awk '{print $2}')
            GF2=$(grep -m1 "^FPS:" "$OUT/run$TAG/gm2_$demo.txt" | awk '{print $2}')
            GF3=$(grep -m1 "^FPS:" "$OUT/run$TAG/gm3_$demo.txt" | awk '{print $2}')
            GVRAM=$(grep -m1 "^VRAM:" "$OUT/run$TAG/gm3_$demo.txt" | sed 's/.*driver_usage_mb=\([0-9.]*\).*/\1/')
            GSTATE="GAME_RECHECK"
            grab_shot "gm_$demo"
          fi
        fi
      else
        GSTATE="DIED_IN_PREP"
      fi
    else
      GSTATE="NO_GAME_STATE"
    fi
  fi

  echo "$demo|OK|$F1|$F2|$F3|$GSTATE|$GF1|$GF2|$GF3|prep=${PREPS}s|$VRAM|$POLYS|gvram=$GVRAM|$LEVELINFO" >> "$RESULTS"
  echo "  OK: editor $F1/$F2/$F3  game($GSTATE) $GF1/$GF2/$GF3  prep=${PREPS}s  vram=$VRAM"
done

taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true
echo "SWEEP COMPLETE $(date +%H:%M:%S)"
cat "$RESULTS"
