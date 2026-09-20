#!/bin/bash
# ============================================================================================
# SINGLE-SESSION DEMO SOAK SWEEP (2026-09-19, Lee's request).
#
# Differs from tools/demo_fps_sweep.sh in exactly one load-bearing way: that script relaunches
# MAX for every demo, so it can never observe anything that ACCUMULATES across level loads.
# That blind spot is what let the 2026-09-18 blank-second-level bug live for four weeks under
# a green 19/19 gate. This one launches MAX ONCE and walks all 19 demos through
#     hub -> editor -> TEST GAME -> back to editor -> hub -> next demo
# so leaks, atlas/pool state, VRAM creep and teardown bugs are all in scope.
#
# FPS here is NOT comparable to any other sweep (machine state) - but WITHIN this run the
#   trend across demo position IS the measurement Lee asked for.
# Blank frames are detected from LUMINANCE VARIANCE, not shot byte size. Byte size conflates
# "blank" with "dark": Escape from the Zombie Cellar's editor frame is a fully rendered windowless
# interior that compresses to 977 KB and false-failed a 1 MB byte threshold. The shot size is still
# recorded here as a cheap smoke signal; soak_analyse.py does the real check on std.
# ============================================================================================
set -u
D="/d/DEV/BUILD/GameGuru Wicked MAX Build Area/Max"
OUT="${1:-/c/Users/leeba/AppData/Local/Temp/claude/D--max-GameGuruMAXDX12/9a28c586-4c13-4447-916e-7fb51301bfa8/scratchpad/soak0919}"
mkdir -p "$OUT/perf" "$OUT/shots"
RES="$OUT/results.txt"
LOG="$OUT/progress.log"

LOCK="$OUT/.lock"
if [ -e "$LOCK" ] && kill -0 "$(cat "$LOCK" 2>/dev/null)" 2>/dev/null; then
  echo "REFUSING: already running as PID $(cat "$LOCK")"; exit 3; fi
echo $$ > "$LOCK"; trap 'rm -f "$LOCK"' EXIT INT TERM

say(){ echo "$(date +%H:%M:%S) $*" | tee -a "$LOG"; }

send(){ rm -f "$D/auto_result.txt"; echo "$1" > "$D/auto_command.txt"; local t=0; local to=${2:-30}
  while [ $t -lt $to ]; do [ -f "$D/auto_result.txt" ] && { sleep 0.2; cat "$D/auto_result.txt"; return 0; }; sleep 1; t=$((t+1)); done
  echo TIMEOUT; return 1; }
alive(){ tasklist.exe //FI "IMAGENAME eq GameGuruMAX.exe" 2>/dev/null | grep -qi GameGuruMAX; }
killmax(){ local i=0; while [ $i -lt 10 ]; do taskkill.exe //IM GameGuruMAX.exe //F >/dev/null 2>&1; sleep 2; alive || return 0; i=$((i+1)); done; return 1; }
wait_state(){ local t=0; while [ $t -lt $2 ]; do alive || return 2
  [ "$(send "GET_STATE" 20 | head -1)" == "STATE: $1" ] && return 0; sleep 5; t=$((t+7)); done; return 1; }

# Both known loading overlays plus the shared "N/100 Complete" progress shape (v4a gate).
OVERLAY='PREPARING|RASTERIZING|BUILDING|GENERATING|[0-9]+.?[0-9]*00 Complete'
wait_prep_clear(){ local t=0; local maxs=${1:-240}
  while [ $t -lt $maxs ]; do alive || { echo "$t"; return 2; }
    send "GET_SCREEN_TEXT" 25 | grep -qiE "$OVERLAY" || { echo "$t"; return 0; }
    sleep 5; t=$((t+7)); done; echo "$t"; return 1; }

shot(){ send "SCREENSHOT" 60 >/dev/null; sleep 3
  local n=$(ls -t "$D/screenshots/"*.png "$D/Files/screenshots/"*.png 2>/dev/null | head -1)
  if [ -n "$n" ]; then cp -f "$n" "$OUT/shots/$1.png" 2>/dev/null; wc -c < "$OUT/shots/$1.png" 2>/dev/null | tr -d ' '; else echo 0; fi; }

boot(){ killmax; rm -f "$D/auto_command.txt" "$D/auto_result.txt"
  ( cd "$D" && ./GameGuruMAX.exe >/dev/null 2>&1 & ); sleep 22
  wait_state "hub" 180; }

DEMOS=(
"Aztec Game Kit Teaser" "Aztec Game Kit" "Bounty" "Horseshoe Bend" "Island Showdown"
"Operation Amazon" "River Raiders" "Snowy Mountain Stroll" "A Grand Canyon Adventure"
"Disruption" "Foggy Forest" "Indian Strike Force" "Switch Escape" "Canyon Offensive"
"Escape from the Zombie Cellar" "Jungle Fever" "RPG Template" "The Mystery of Z Island" "Trapped"
)

: > "$RES"; : > "$LOG"
UP=$(powershell.exe -NoProfile -Command "\$b=(Get-CimInstance Win32_OperatingSystem).LastBootUpTime; '{0:N1}' -f ((Get-Date)-\$b).TotalHours" 2>/dev/null | tr -d '\r')
say "single-session soak sweep | exe $(wc -c < "$D/GameGuruMAX.exe") bytes | ${UP}h since boot"
echo "# single-session soak $(date '+%F %T')  uptime=${UP}h  exe=$(wc -c < "$D/GameGuruMAX.exe")" >> "$RES"

RESTARTS=0
say "BOOT"; if ! boot; then say "FATAL: never reached hub on first launch"; exit 1; fi
sleep 5
SESSION_START=$(date +%s)

IDX=-1
for demo in "${DEMOS[@]}"; do
  IDX=$((IDX+1)); T0=$(date +%s)
  say "[$IDX] $demo"

  # If the process died in the previous demo, relaunch and SAY SO - a restart breaks the
  # cumulative premise for everything after it, so it must be visible in the results, not
  # silently healed.
  if ! alive; then
    RESTARTS=$((RESTARTS+1)); say "  !! process gone - relaunching (restart #$RESTARTS)"
    echo "--- SESSION RESTART #$RESTARTS before [$IDX] $demo ---" >> "$RES"
    boot || { say "  relaunch failed"; echo "$demo|FAIL_RELAUNCH" >> "$RES"; continue; }
    sleep 5
  fi

  # SELECT_DEMO matches an EMPTY list unless the Demo Games tab is the live one first.
  send "NAVIGATE hub" 20 >/dev/null; sleep 6
  send "NAVIGATE hub.demo_games" 20 >/dev/null; sleep 4
  R=$(send "SELECT_DEMO $demo" 20)
  [[ "$R" == OK:* ]] || { say "  FAIL select: $R"; echo "$demo|FAIL_SELECT|$R" >> "$RES"; continue; }
  sleep 3
  R=$(send "CLICK edit_game" 20)
  [[ "$R" == OK:* ]] || { say "  FAIL edit: $R"; echo "$demo|FAIL_EDIT|$R" >> "$RES"; continue; }
  sleep 8
  wait_state "storyboard" 90 || { say "  FAIL storyboard"; echo "$demo|FAIL_STORYBOARD" >> "$RES"; continue; }
  R=$(send "CLICK_ONLY_LEVEL" 20); LVL=$(echo "$R" | head -1)
  [[ "$R" == OK:* ]] || { say "  FAIL level: $R"; echo "$demo|FAIL_LEVEL|$R" >> "$RES"; continue; }
  if ! wait_state "editor" 420; then
    say "  FAIL editor (alive=$(alive && echo yes || echo NO))"
    echo "$demo|FAIL_EDITOR|alive=$(alive && echo yes || echo no)" >> "$RES"; continue; fi
  TLOAD=$(( $(date +%s) - T0 ))

  # SETTLE. 2026-09-19: the first run of this script used 25 s + three samples 4 s apart and
  # read POLYS LOW on 11 of 19 demos - every one of them vegetation-heavy, every sparse or indoor
  # level exact. Direct re-measurement with a longer ladder returned the C2 reference EXACTLY on
  # Operation Amazon (486602, was 315943) and RPG Template (540778, was 351661), on a cold load
  # and again on a warm second load. So it was the window, not the geometry - the tree pool had
  # not finished populating. Under-settling can only ever UNDER-count, so the max of the samples
  # is the settled figure. 60 s plus a wider spread; do not shorten it to save wall clock.
  sleep 60
  for s in 1 2 3; do send "GET_PERF_DATA" 30 > "$OUT/perf/ed${s}_$IDX.txt"; sleep 15; done
  EF=$(grep -hm1 "^FPS:" "$OUT/perf/ed3_$IDX.txt" | awk '{print $2}')
  EVR=$(grep -hm1 "^VRAM:" "$OUT/perf/ed3_$IDX.txt" | sed 's/.*driver_usage_mb=\([0-9.]*\).*/\1/')
  # POLYS = MAX of the three samples; under-settling can only ever UNDER-count triangles.
  EPOLY=$(for s in 1 2 3; do grep -hm1 "^POLYS:" "$OUT/perf/ed${s}_$IDX.txt" 2>/dev/null; done \
          | tr -d '\r' | awk '{if($2+0>m)m=$2+0}END{print m+0}')
  ESHOT=$(shot "ed_$IDX")

  # ---- test game ----
  GST=NOGAME; GF=""; GVR=""; GSHOT=0; PREP=""
  R=$(send "CLICK test_level" 20)
  if [[ "$R" == OK:* ]]; then
    sleep 12
    if wait_state "game" 150; then
      PREP=$(wait_prep_clear 300)
      if alive; then
        GST=GAME; sleep 15
        for s in 1 2 3; do send "GET_PERF_DATA" 30 > "$OUT/perf/gm${s}_$IDX.txt"; sleep 5; done
        GF=$(grep -hm1 "^FPS:" "$OUT/perf/gm3_$IDX.txt" | awk '{print $2}')
        # Sub-20 FPS means a loading screen GET_SCREEN_TEXT cannot see (the navmesh banner is
        # printscreenprompt, not ImGui). Poll for recovery, then re-sample.
        if [ -n "$GF" ] && [ "$(awk -v v="$GF" 'BEGIN{print (v<20)?1:0}')" == "1" ]; then
          say "  sub-20 ($GF) - polling for recovery"; rt=0
          while [ $rt -lt 300 ]; do alive || break
            cur=$(send "GET_PERF_DATA" 30 | grep -m1 "^FPS:" | awk '{print $2}')
            [ -n "$cur" ] && [ "$(awk -v v="$cur" 'BEGIN{print (v>=20)?1:0}')" == "1" ] && break
            sleep 5; rt=$((rt+7)); done
          PREP="$PREP+rec@${rt}"
          if alive; then sleep 20
            for s in 1 2 3; do send "GET_PERF_DATA" 30 > "$OUT/perf/gm${s}_$IDX.txt"; sleep 5; done
            GF=$(grep -hm1 "^FPS:" "$OUT/perf/gm3_$IDX.txt" | awk '{print $2}'); GST=GAME_RECHECK; fi
        fi
        GVR=$(grep -hm1 "^VRAM:" "$OUT/perf/gm3_$IDX.txt" | sed 's/.*driver_usage_mb=\([0-9.]*\).*/\1/')
        GSHOT=$(shot "gm_$IDX")
        send "PRESS_ESCAPE" 25 >/dev/null
        wait_state "editor" 150 || { say "  !! did not return to editor after ESC"; GST="$GST+NOEXIT"; }
      else GST=DIED_IN_PREP; fi
    else
      if alive; then GST=NO_GAME_STATE; else GST=DIED_ENTERING_GAME; fi
    fi
  else GST="FAIL_CLICK"; fi

  TOT=$(( $(date +%s) - T0 )); ELAPSED=$(( $(date +%s) - SESSION_START ))
  echo "$demo|$GST|load=${TLOAD}s|edFPS=$EF|edVRAM=$EVR|polys=$EPOLY|edshot=$ESHOT|gmFPS=$GF|gmVRAM=$GVR|gmshot=$GSHOT|prep=${PREP}|dur=${TOT}s|elapsed=${ELAPSED}s|restarts=$RESTARTS|$LVL" >> "$RES"
  say "  $GST load=${TLOAD}s ed=$EF/$EVR MB game=$GF/$GVR MB shots=$ESHOT/$GSHOT dur=${TOT}s"
done

say "sweep body done - one more hub return to prove the session is still healthy"
send "NAVIGATE hub" 20 >/dev/null; sleep 5
FINAL=$(send "GET_STATE" 25 | head -1)
echo "# final state after all 19: $FINAL  restarts=$RESTARTS  total=$(( $(date +%s) - SESSION_START ))s" >> "$RES"
say "final: $FINAL  restarts=$RESTARTS"
send "QUIT" 10 >/dev/null 2>&1; t=0; while alive && [ $t -lt 25 ]; do sleep 1; t=$((t+1)); done; killmax
say "SOAK SWEEP COMPLETE"
