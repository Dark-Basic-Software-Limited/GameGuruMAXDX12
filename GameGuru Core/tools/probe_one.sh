#!/bin/bash
# Single-demo EDITOR-ONLY FPS probe for the 0814 regression bisect.
# Same recipe as demo_fps_sweep.sh's editor phase so numbers are comparable:
#   fresh MAX launch -> hub -> SELECT_DEMO -> edit_game -> CLICK_ONLY_LEVEL
#   -> editor -> 30s soak at start camera -> 3 GET_PERF_DATA samples -> kill MAX.
# Usage: probe_one.sh <TAG> <DEMO NAME...>
D="/d/DEV/BUILD/GameGuru Wicked MAX Build Area/Max"
OUT="/c/Users/leeba/AppData/Local/Temp/claude/D--max-GameGuruMAXDX12--claude-worktrees-determined-chebyshev-bf0892/9a28c586-4c13-4447-916e-7fb51301bfa8/scratchpad/demo_fps"
TAG="$1"; shift
DEMO="$*"
mkdir -p "$OUT/probe$TAG"
RES="$OUT/probe_results_$TAG.txt"
: > "$RES"

# single-instance lock, same rule as the sweep
LOCK="$OUT/.probe_one.lock"
if [ -e "$LOCK" ] && kill -0 "$(cat "$LOCK" 2>/dev/null)" 2>/dev/null; then
  echo "REFUSING TO START: probe_one.sh already running as PID $(cat "$LOCK")."
  exit 3
fi
echo $$ > "$LOCK"
trap 'rm -f "$LOCK"' EXIT INT TERM

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

wait_state() {
  local t=0
  while [ $t -lt $2 ]; do
    if ! alive; then return 2; fi
    local s=$(send "GET_STATE" 20 | head -1)
    if [ "$s" == "STATE: $1" ]; then return 0; fi
    sleep 5; t=$((t+7))
  done
  return 1
}

echo "### probe $TAG : $DEMO — $(date +%H:%M:%S)"
taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true
sleep 4
rm -f "$D/auto_command.txt"   # stale-command rule
cd "$D" && ./GameGuruMAX.exe > /dev/null 2>&1 &
sleep 15

if ! wait_state "hub" 60; then echo "$DEMO|FAIL_HUB" >> "$RES"; echo "FAIL: never reached hub"; taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; exit 1; fi
sleep 4

R=$(send "SELECT_DEMO $DEMO" 15)
if [[ "$R" != OK:* ]]; then echo "$DEMO|FAIL_SELECT|$R" >> "$RES"; taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; exit 1; fi
sleep 3

R=$(send "CLICK edit_game" 15)
if [[ "$R" != OK:* ]]; then echo "$DEMO|FAIL_EDIT|$R" >> "$RES"; taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; exit 1; fi
sleep 8

if ! wait_state "storyboard" 40; then echo "$DEMO|FAIL_STORYBOARD" >> "$RES"; taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; exit 1; fi

R=$(send "CLICK_ONLY_LEVEL" 15)
if [[ "$R" != OK:* ]]; then echo "$DEMO|FAIL_LEVEL|$R" >> "$RES"; taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; exit 1; fi

if ! wait_state "editor" 150; then echo "$DEMO|FAIL_EDITOR" >> "$RES"; taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; exit 1; fi

sleep 30  # soak at start camera

send "GET_PERF_DATA" 20 > "$OUT/probe$TAG/ed1.txt"; sleep 4
send "GET_PERF_DATA" 20 > "$OUT/probe$TAG/ed2.txt"; sleep 4
send "GET_PERF_DATA" 20 > "$OUT/probe$TAG/ed3.txt"

F1=$(grep -m1 "^FPS:" "$OUT/probe$TAG/ed1.txt" | awk '{print $2}')
F2=$(grep -m1 "^FPS:" "$OUT/probe$TAG/ed2.txt" | awk '{print $2}')
F3=$(grep -m1 "^FPS:" "$OUT/probe$TAG/ed3.txt" | awk '{print $2}')
FT=$(grep -m1 "^FRAME_TIME_MS:" "$OUT/probe$TAG/ed3.txt" | awk '{print $2}')
ST=$(grep -m1 "^SUBMIT_STALL_WINDOW:" "$OUT/probe$TAG/ed3.txt" | tr -d '\r')

echo "$DEMO|$F1|$F2|$F3|ft=$FT|$ST" >> "$RES"
echo "RESULT $TAG: $DEMO FPS $F1 / $F2 / $F3 (frame ${FT}ms)"

taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true
echo "PROBE DONE $(date +%H:%M:%S)"
