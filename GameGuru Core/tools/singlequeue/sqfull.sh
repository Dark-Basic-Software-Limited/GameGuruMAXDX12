#!/bin/bash
# FULL SET_SINGLEQUEUE sweep — all 19 hub demos, editor, three arms each (0 / 1 / 0).
#
# PRE-REGISTERED FLIP CRITERIA (written before the data exists, so the decision cannot be
# rationalised after the fact). Flip the default ON only if ALL hold:
#   C1 FPS: >= 16/19 demos positive, AND no demo regresses by more than 2.0% (the measured
#           A-vs-A2 control drift is well under that everywhere so far).
#   C2 TAIL: no demo's per-second count of frames over 16.7 ms rises by more than 25% on the
#           ON arm. HITCH counters are cumulative since launch, so each arm is diffed against
#           the previous one and normalised by arm duration.
#   C3 GEOMETRY: POLYS identical between arms on every demo (proves no visual/content change).
# Fail any one -> default stays OFF and the sweep becomes the documented reason.
#
# Arms are 40 s so the tail has 3000-10000 frames to show itself (the 08-09 arms were 22 s,
# which is why Island Showdown's single 6.71 ms frame could not be classified).
D="/d/DEV/BUILD/GameGuru Wicked MAX Build Area/Max"
OUT="$(dirname "$0")"; LOG="$OUT/sqfull.log"; RES="$OUT/sqfull_results.txt"
ARM_S="${1:-40}"
START_AT="${2:-1}"     # 1-based demo index to resume from (a killed run costs one demo, not all)

# ⚠ SINGLE-INSTANCE LOCK. Learned the hard way 2026-08-09: three copies of this script ended up
# running at once (a nohup'd launch that outlived its parent, plus two relaunches after a pkill
# that silently does nothing under Git Bash on Windows). All three drove the SAME MAX through the
# SAME auto_command.txt and appended to the SAME log, which produced impossible data — 40 s arms
# completing in 16 s, a level's A1 arm read at 3.9 FPS mid-load, duplicate demo headers. None of
# it was detectable from the numbers alone. A leaked parallel runner is the single most dangerous
# failure mode in this harness because it corrupts quietly.
LOCK="$OUT/.sqfull.lock"
if [ -e "$LOCK" ] && kill -0 "$(cat "$LOCK" 2>/dev/null)" 2>/dev/null; then
  echo "REFUSING TO START: sqfull.sh already running as PID $(cat "$LOCK"). Kill it first."
  exit 3
fi
echo $$ > "$LOCK"
trap 'rm -f "$LOCK"' EXIT INT TERM

if [ "$START_AT" == "1" ]; then : > "$LOG"; : > "$RES"; fi
say() { echo "$(date +%H:%M:%S) $*" | tee -a "$LOG"; }
send() { rm -f "$D/auto_result.txt"; echo "$1" > "$D/auto_command.txt"; local t=0
  while [ $t -lt ${2:-40} ]; do if [ -f "$D/auto_result.txt" ]; then sleep 0.3; cat "$D/auto_result.txt"; return 0; fi; sleep 1; t=$((t+1)); done; echo TIMEOUT; }
alive() { tasklist.exe //FI "IMAGENAME eq GameGuruMAX.exe" 2>/dev/null | grep -qi GameGuruMAX; }
wait_state() { local t=0; while [ $t -lt $2 ]; do alive || return 2
  [ "$(send "GET_STATE" 20|head -1)" == "STATE: $1" ] && return 0; sleep 5; t=$((t+7)); done; return 1; }

# Wait until the level is genuinely settled, instead of trusting a fixed soak.
# WHY: a fixed 90 s soak captured Aztec Game Kit's first arm at 3.9 FPS / POLYS 154768 while
# the level was still streaming in (its settled state is ~106 FPS / POLYS 3438876). Averaging
# that into the OFF baseline would have manufactured a fake +96% win for the knob.
# Gate on BOTH: POLYS stable (geometry finished loading) and FPS within 3% of the previous
# sample (lazy-PSO warm-up finished). Returns 1 on timeout so the caller can mark the demo.
wait_stable() {
  local maxs=${1:-300} t=0 pf=-1 pp=-1 good=0
  while [ $t -lt $maxs ]; do
    sleep 15; t=$((t+15))
    alive || return 2
    local d=$(send "GET_PERF_DATA" 45)
    local f=$(echo "$d" | grep -m1 '^FPS:' | awk '{print $2}')
    local p=$(echo "$d" | grep -m1 '^POLYS:' | awk '{print $2}')
    [ -z "$f" ] && continue
    if [ "$pf" != "-1" ]; then
      # both must hold: same geometry, and FPS moved less than 3%
      if [ "$p" == "$pp" ] && awk -v a="$pf" -v b="$f" 'BEGIN{exit !(a>0 && (b-a)/a<0.03 && (b-a)/a>-0.03)}'; then
        good=$((good+1))
        [ $good -ge 2 ] && { say "    settled after ${t}s (fps=$f polys=$p)"; return 0; }
      else good=0; fi
    fi
    pf=$f; pp=$p
  done
  say "    WARN never settled in ${maxs}s (last fps=$pf polys=$pp)"; return 1
}

# capture one arm -> "FPS|stallmean|stallmax|over16|over33|polys"
arm() {
  send "$1" 30 >/dev/null; sleep 5
  send "SET_SUBMITSTATS 1" 30 >/dev/null
  sleep "$ARM_S"
  local d=$(send "GET_PERF_DATA" 45)
  local fps=$(echo "$d" | grep -m1 '^FPS:' | awk '{print $2}')
  local sw=$(echo "$d" | grep -m1 '^SUBMIT_STALL_WINDOW:')
  local sm=$(echo "$sw" | sed -n 's/.*mean=\([0-9.]*\).*/\1/p')
  local sx=$(echo "$sw" | sed -n 's/.*max=\([0-9.]*\).*/\1/p')
  local h=$(echo "$d" | grep -m1 '^HITCH:')
  # over(16.7/25/33/50/100)=a/b/c/d/e  -> take a and c
  local ov=$(echo "$h" | sed -n 's/.*over([^)]*)=\([0-9\/]*\).*/\1/p')
  local o16=$(echo "$ov" | cut -d/ -f1); local o33=$(echo "$ov" | cut -d/ -f3)
  local pol=$(echo "$d" | grep -m1 '^POLYS:' | awk '{print $2}')
  echo "${fps:-0}|${sm:-0}|${sx:-0}|${o16:-0}|${o33:-0}|${pol:-0}"
}

DEMOS=(
"Aztec Game Kit Teaser" "Aztec Game Kit" "Bounty" "Horseshoe Bend" "Island Showdown"
"Operation Amazon" "River Raiders" "Snowy Mountain Stroll" "A Grand Canyon Adventure"
"Disruption" "Foggy Forest" "Indian Strike Force" "Switch Escape" "Canyon Offensive"
"Escape from the Zombie Cellar" "Jungle Fever" "RPG Template" "The Mystery of Z Island" "Trapped"
)

say "=== FULL SET_SINGLEQUEUE SWEEP — ${#DEMOS[@]} demos, ${ARM_S}s arms ==="
IDX=0
for DEMO in "${DEMOS[@]}"; do
  IDX=$((IDX+1))
  [ "$IDX" -lt "$START_AT" ] && continue
  say "######## [$IDX/${#DEMOS[@]}] $DEMO ########"
  taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true; sleep 4
  rm -f "$D/auto_command.txt" "$D/auto_result.txt"
  cd "$D" && ./GameGuruMAX.exe >/dev/null 2>&1 &
  sleep 15
  if ! wait_state "hub" 90; then say "  FAIL_HUB"; echo "$DEMO|FAIL_HUB" >> "$RES"; continue; fi
  sleep 4
  R=$(send "SELECT_DEMO $DEMO" 20)
  if [[ "$R" != OK:* ]]; then say "  FAIL_SELECT $R"; echo "$DEMO|FAIL_SELECT" >> "$RES"; continue; fi
  sleep 3; send "CLICK edit_game" 20 >/dev/null; sleep 10
  send "CLICK_ONLY_LEVEL" 120 >/dev/null; sleep 50
  if ! wait_state "editor" 240; then say "  WARN not editor"; fi
  wait_stable 300
  if ! alive; then say "  DIED_SOAK"; echo "[$IDX] $DEMO|DIED_SOAK" >> "$RES"; continue; fi

  A1=$(arm "SET_SINGLEQUEUE 0"); alive || { say "  DIED_A1"; echo "[$IDX] $DEMO|DIED" >> "$RES"; continue; }
  B=$(arm  "SET_SINGLEQUEUE 1"); alive || { say "  DIED_B";  echo "[$IDX] $DEMO|DIED" >> "$RES"; continue; }
  A2=$(arm "SET_SINGLEQUEUE 0"); alive || { say "  DIED_A2"; echo "[$IDX] $DEMO|DIED" >> "$RES"; continue; }
  say "  A1=$A1"; say "  B =$B"; say "  A2=$A2"
  # index-stamped so a mislabelled row is visible rather than silently attributed
  echo "[$IDX] $DEMO|$A1|$B|$A2" >> "$RES"
done
taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true
say "=== SWEEP DONE -> $RES ==="
