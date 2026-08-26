#!/bin/bash
# GGMAX 3.25d: does Water Bake come back OFF as reliably as it goes ON? Toggles twice, because
# the defect Lee hit was one frame out of step and only showed on the SECOND half of a cycle.
D="/d/DEV/BUILD/GameGuru Wicked MAX Build Area/Max"
OUT="C:/Users/leeba/AppData/Local/Temp/claude/D--max-GameGuruMAXDX12--claude-worktrees-determined-chebyshev-bf0892/9a28c586-4c13-4447-916e-7fb51301bfa8/scratchpad/night/water"
mkdir -p "$OUT"; LOG="$OUT/log.txt"; : > "$LOG"
send() { rm -f "$D/auto_result.txt"; echo "$1" > "$D/auto_command.txt"; local t=0
  while [ $t -lt ${2:-30} ]; do [ -f "$D/auto_result.txt" ] && { cat "$D/auto_result.txt"; return 0; }; sleep 1; t=$((t+1)); done; echo TIMEOUT; }
alive() { tasklist.exe //FI "IMAGENAME eq GameGuruMAX.exe" 2>/dev/null | grep -qi GameGuruMAX; }
wait_state() { local t=0; while [ $t -lt $2 ]; do alive || return 2
  [ "$(send GET_STATE 20 | head -1)" == "STATE: $1" ] && return 0; sleep 5; t=$((t+7)); done; return 1; }
ocean() { send "DUMP_BAKE" 20 | grep -m1 "ocean" | sed 's/^ *//'; }

taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; sleep 4; rm -f "$D/auto_command.txt"
cd "$D" && ./GameGuruMAX.exe > /dev/null 2>&1 & sleep 15
wait_state hub 60 || { echo FAIL_HUB | tee -a "$LOG"; exit 1; }
sleep 4; send "SELECT_DEMO Canyon Offensive" 15 >/dev/null; sleep 3
send "CLICK edit_game" 15 >/dev/null; sleep 8
wait_state storyboard 40 || { echo FAIL_SB | tee -a "$LOG"; exit 1; }
send "CLICK_ONLY_LEVEL" 15 >/dev/null
wait_state editor 180 || { echo FAIL_ED | tee -a "$LOG"; exit 1; }
sleep 25

for cycle in 1 2; do
  echo "--- cycle $cycle : baseline  $(ocean)" | tee -a "$LOG"
  send "SCREENSHOT $OUT/c${cycle}_0_before.png" 25 >/dev/null
  send "SET_BAKEWATER 1" 20 >/dev/null; sleep 8
  echo "--- cycle $cycle : bake ON   $(ocean)" | tee -a "$LOG"
  send "DUMP_BAKE" 20 | grep -E "plane|drawn" | sed 's/^ */    /' | tee -a "$LOG"
  send "SCREENSHOT $OUT/c${cycle}_1_on.png" 25 >/dev/null
  send "SET_BAKEWATER 0" 20 >/dev/null; sleep 8
  echo "--- cycle $cycle : bake OFF  $(ocean)" | tee -a "$LOG"
  send "SCREENSHOT $OUT/c${cycle}_2_off.png" 25 >/dev/null
done
alive && echo "STILL ALIVE" | tee -a "$LOG" || echo "!!! DIED" | tee -a "$LOG"
taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true
echo "WATERTEST DONE $(date +%H:%M:%S)" | tee -a "$LOG"
