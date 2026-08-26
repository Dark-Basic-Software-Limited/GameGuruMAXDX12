#!/bin/bash
# GGMAX 3.25: functional test for Terrain Bake / Water Bake / Reduction Scale.
# Loads one demo in the editor and drives the three new switches through the harness, capturing
# a screenshot and a DUMP_BAKE at every step so the visual and the counters can be read together.
# Usage: baketest.sh <TAG> <DEMO NAME...>
D="/d/DEV/BUILD/GameGuru Wicked MAX Build Area/Max"
OUT="/c/Users/leeba/AppData/Local/Temp/claude/D--max-GameGuruMAXDX12--claude-worktrees-determined-chebyshev-bf0892/9a28c586-4c13-4447-916e-7fb51301bfa8/scratchpad/night/bake"
TAG="$1"; shift
DEMO="$*"
mkdir -p "$OUT/$TAG"
LOG="$OUT/$TAG/log.txt"
: > "$LOG"

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
step() { echo "" | tee -a "$LOG"; echo "===== $* =====" | tee -a "$LOG"; }

taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true
sleep 4
rm -f "$D/auto_command.txt"
cd "$D" && ./GameGuruMAX.exe > /dev/null 2>&1 &
sleep 15

if ! wait_state "hub" 60; then echo "FAIL: hub" | tee -a "$LOG"; exit 1; fi
sleep 4
send "SELECT_DEMO $DEMO" 15 | tee -a "$LOG"
sleep 3
send "CLICK edit_game" 15 | tee -a "$LOG"
sleep 8
if ! wait_state "storyboard" 40; then echo "FAIL: storyboard" | tee -a "$LOG"; exit 1; fi
send "CLICK_ONLY_LEVEL" 15 | tee -a "$LOG"
if ! wait_state "editor" 180; then echo "FAIL: editor" | tee -a "$LOG"; exit 1; fi
sleep 25

step "BASELINE (no switches)"
send "GET_PERF_DATA" 20 | head -6 | tee -a "$LOG"
send "DUMP_BAKE" 20 | tee -a "$LOG"
send "SCREENSHOT $OUT/$TAG/1_baseline.png" 25 | tee -a "$LOG"

step "TERRAIN BAKE ON"
send "SET_BAKETERRAIN 1" 20 | tee -a "$LOG"
sleep 20
send "DUMP_BAKE" 20 | tee -a "$LOG"
send "GET_PERF_DATA" 20 | head -6 | tee -a "$LOG"
send "SCREENSHOT $OUT/$TAG/2_terrainbake_on.png" 25 | tee -a "$LOG"

step "TERRAIN BAKE OFF (terrain must come back)"
send "SET_BAKETERRAIN 0" 20 | tee -a "$LOG"
sleep 20
send "DUMP_BAKE" 20 | tee -a "$LOG"
send "SCREENSHOT $OUT/$TAG/3_terrainbake_off.png" 25 | tee -a "$LOG"

step "WATER BAKE ON"
send "SET_BAKEWATER 1" 20 | tee -a "$LOG"
sleep 12
send "DUMP_BAKE" 20 | tee -a "$LOG"
send "SCREENSHOT $OUT/$TAG/4_waterbake_on.png" 25 | tee -a "$LOG"

step "WATER BAKE OFF"
send "SET_BAKEWATER 0" 20 | tee -a "$LOG"
sleep 12
send "DUMP_BAKE" 20 | tee -a "$LOG"
send "SCREENSHOT $OUT/$TAG/5_waterbake_off.png" 25 | tee -a "$LOG"

step "REDUCTION SCALE"
send "SET_ANIMREDUCTION 50" 20 | tee -a "$LOG"
send "SET_ANIMREDUCTION 1" 20 | tee -a "$LOG"

step "BOTH BAKES ON TOGETHER"
send "SET_BAKETERRAIN 1" 20 | tee -a "$LOG"
send "SET_BAKEWATER 1" 20 | tee -a "$LOG"
sleep 20
send "DUMP_BAKE" 20 | tee -a "$LOG"
send "GET_PERF_DATA" 20 | head -6 | tee -a "$LOG"
send "SCREENSHOT $OUT/$TAG/6_both_on.png" 25 | tee -a "$LOG"

if alive; then echo "STILL ALIVE - no crash" | tee -a "$LOG"; else echo "!!! PROCESS DIED" | tee -a "$LOG"; fi
taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true
echo "BAKETEST DONE $(date +%H:%M:%S)" | tee -a "$LOG"
