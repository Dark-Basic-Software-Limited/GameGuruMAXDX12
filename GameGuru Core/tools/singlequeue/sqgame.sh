#!/bin/bash
# Close the two gaps left by the 2.17 default flip (task #131):
#   PART A  TESTPRO1 in the EDITOR — the level that produced the original -4.7 FPS verdict
#           (2026-07-26, 1.48b). If it is still negative that is a real content-dependent
#           exception and the flip needs a caveat; if positive, the old verdict was simply stale.
#   PART B  TEST-GAME mode on a representative spread — everything measured so far is editor,
#           which has ImGui and a different pass/queue mix. Gameplay is where users judge FPS.
#           Includes Horseshoe Bend, the only demo that went negative in the editor sweep.
#
# Same discipline as the full sweep: PID lock, settle gate (not a fixed soak), three arms
# (0/1/0) so control drift is visible, POLYS checked for content identity.
# ⚠ Test-game gating uses FPS RECOVERY, not screen text — GET_SCREEN_TEXT cannot see the
# navmesh banner (documented trap), and a loading screen reads as ~3 FPS.
D="/d/DEV/BUILD/GameGuru Wicked MAX Build Area/Max"
OUT="$(dirname "$0")"; LOG="$OUT/sqgame.log"; RES="$OUT/sqgame_results.txt"
ARM_S="${1:-40}"
LOCK="$OUT/.sqgame.lock"
if [ -e "$LOCK" ] && kill -0 "$(cat "$LOCK" 2>/dev/null)" 2>/dev/null; then
  echo "REFUSING: already running as PID $(cat "$LOCK")"; exit 3; fi
echo $$ > "$LOCK"; trap 'rm -f "$LOCK"' EXIT INT TERM
: > "$LOG"; : > "$RES"

say(){ echo "$(date +%H:%M:%S) $*" | tee -a "$LOG"; }
send(){ rm -f "$D/auto_result.txt"; echo "$1" > "$D/auto_command.txt"; local t=0
  while [ $t -lt ${2:-40} ]; do if [ -f "$D/auto_result.txt" ]; then sleep 0.3; cat "$D/auto_result.txt"; return 0; fi; sleep 1; t=$((t+1)); done; echo TIMEOUT; }
alive(){ tasklist.exe //FI "IMAGENAME eq GameGuruMAX.exe" 2>/dev/null | grep -qi GameGuruMAX; }
wait_state(){ local t=0; while [ $t -lt $2 ]; do alive || return 2
  [ "$(send "GET_STATE" 20|head -1)" == "STATE: $1" ] && return 0; sleep 5; t=$((t+7)); done; return 1; }
getfps(){ send "GET_PERF_DATA" 45 | grep -m1 '^FPS:' | awk '{print $2}'; }
getpolys(){ send "GET_PERF_DATA" 45 | grep -m1 '^POLYS:' | awk '{print $2}'; }

# settle: POLYS stable AND FPS within 3% twice running. minfps rejects loading screens.
wait_stable(){ local maxs=${1:-300} minfps=${2:-0} t=0 pf=-1 pp=-1 good=0
  while [ $t -lt $maxs ]; do
    sleep 15; t=$((t+15)); alive || return 2
    local d=$(send "GET_PERF_DATA" 45)
    local f=$(echo "$d"|grep -m1 '^FPS:'|awk '{print $2}')
    local p=$(echo "$d"|grep -m1 '^POLYS:'|awk '{print $2}')
    [ -z "$f" ] && continue
    if awk -v a="$f" -v m="$minfps" 'BEGIN{exit !(a<m)}'; then pf=$f; pp=$p; good=0; continue; fi
    if [ "$pf" != "-1" ] && [ "$p" == "$pp" ] && awk -v a="$pf" -v b="$f" 'BEGIN{exit !(a>0&&(b-a)/a<0.03&&(b-a)/a>-0.03)}'; then
      good=$((good+1)); [ $good -ge 2 ] && { say "    settled ${t}s (fps=$f polys=$p)"; return 0; }
    else good=0; fi
    pf=$f; pp=$p
  done; say "    WARN unsettled ${maxs}s (fps=$pf polys=$pp)"; return 1; }

arm(){ send "$1" 30 >/dev/null; sleep 5; send "SET_SUBMITSTATS 1" 30 >/dev/null; sleep "$ARM_S"
  local d=$(send "GET_PERF_DATA" 45)
  local f=$(echo "$d"|grep -m1 '^FPS:'|awk '{print $2}')
  local sw=$(echo "$d"|grep -m1 '^SUBMIT_STALL_WINDOW:')
  local sm=$(echo "$sw"|sed -n 's/.*mean=\([0-9.]*\).*/\1/p'); local sx=$(echo "$sw"|sed -n 's/.*max=\([0-9.]*\).*/\1/p')
  local ov=$(echo "$d"|grep -m1 '^HITCH:'|sed -n 's/.*over([^)]*)=\([0-9\/]*\).*/\1/p')
  local pol=$(echo "$d"|grep -m1 '^POLYS:'|awk '{print $2}')
  echo "${f:-0}|${sm:-0}|${sx:-0}|$(echo "$ov"|cut -d/ -f1)|${pol:-0}"; }

three(){ # $1 tag
  local A1=$(arm "SET_SINGLEQUEUE 0"); alive || return 1
  local B=$(arm  "SET_SINGLEQUEUE 1"); alive || return 1
  local A2=$(arm "SET_SINGLEQUEUE 0"); alive || return 1
  say "  A1=$A1"; say "  B =$B"; say "  A2=$A2"
  echo "$1|$A1|$B|$A2" >> "$RES"; }

boot(){ taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true; sleep 4
  rm -f "$D/auto_command.txt" "$D/auto_result.txt"
  cd "$D" && ./GameGuruMAX.exe >/dev/null 2>&1 & sleep 15
  wait_state "hub" 90; }

# ---------------- PART A: TESTPRO1 (editor) ----------------
say "######## PART A: TESTPRO1 editor — the level behind the original -4.7 verdict ########"
if boot; then
  sleep 4
  R=$(send "OPEN_PROJECT TESTPRO1" 180); say "  open: ${R:0:90}"
  if [[ "$R" == OK:* ]]; then
    sleep 15; say "  level: $(send "CLICK_ONLY_LEVEL" 120 | head -c 90)"; sleep 50
    wait_state "editor" 240 || say "  WARN not editor"
    wait_stable 300 0
    three "TESTPRO1-editor" || say "  DIED"
  else say "  SKIP TESTPRO1 (project not found)"; echo "TESTPRO1-editor|SKIP" >> "$RES"; fi
else say "  FAIL hub"; fi

# ---------------- PART B: test-game mode ----------------
GDEMOS=("Trapped" "Switch Escape" "Escape from the Zombie Cellar" "Island Showdown" "Horseshoe Bend" "Foggy Forest")
for DEMO in "${GDEMOS[@]}"; do
  say "######## PART B: $DEMO — TEST GAME ########"
  boot || { say "  FAIL hub"; echo "$DEMO-game|FAIL_HUB" >> "$RES"; continue; }
  sleep 4
  R=$(send "SELECT_DEMO $DEMO" 20)
  [[ "$R" == OK:* ]] || { say "  FAIL select"; echo "$DEMO-game|FAIL_SELECT" >> "$RES"; continue; }
  sleep 3; send "CLICK edit_game" 20 >/dev/null; sleep 10
  send "CLICK_ONLY_LEVEL" 120 >/dev/null; sleep 50
  wait_state "editor" 240 || say "  WARN not editor"
  wait_stable 240 0
  say "  -> entering test game"
  send "CLICK test_level" 30 >/dev/null
  sleep 60
  # gate on FPS RECOVERY: loading/prep screens sit near 3 FPS; real gameplay never <20.
  if ! wait_stable 420 20; then say "  WARN game never settled >20fps — recording anyway"; fi
  three "$DEMO-game" || say "  DIED"
done
taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true
say "=== SQGAME DONE -> $RES ==="
