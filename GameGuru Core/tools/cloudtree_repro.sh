#!/bin/bash
# GGMAX 3.75: clouds composited OVER far tree billboards (Lee, Aztec Game Kit Teaser).
# Capture the artefact under the harness and run the cheap discriminators:
#   clouds ON vs OFF  -> does the tree come back when the cloud pass is removed?
#   far trees ON/OFF  -> is it the BILLBOARD pass specifically?
# Shots are paired so a fix can be A/B'd against the same camera later.
D="/d/DEV/BUILD/GameGuru Wicked MAX Build Area/Max"
OUT="${GGMAX_CLOUD_OUT:-/c/Users/leeba/AppData/Local/Temp/claude/D--max-GameGuruMAXDX12/9a28c586-4c13-4447-916e-7fb51301bfa8/scratchpad/cloudtree}"
LABEL="${1:-before}"
mkdir -p "$OUT"

send() { rm -f "$D/auto_result.txt"; echo "$1" > "$D/auto_command.txt"; local t=0
  while [ $t -lt ${2:-30} ]; do [ -f "$D/auto_result.txt" ] && { sleep 0.2; cat "$D/auto_result.txt"; return 0; }; sleep 1; t=$((t+1)); done; echo TIMEOUT; }
alive() { tasklist.exe //FI "IMAGENAME eq GameGuruMAX.exe" 2>/dev/null | grep -qi GameGuruMAX; }
wait_state() { local t=0; while [ $t -lt $2 ]; do alive || return 2
  [ "$(send GET_STATE 20 | head -1)" == "STATE: $1" ] && return 0; sleep 5; t=$((t+7)); done; return 1; }
shot() { local line; line=$(send "SCREENSHOT x" 40); local p
  p=$(echo "$line" | sed 's/^OK: Screenshot saved to //' | tr '\\' '/')
  if [ -f "$p" ]; then cp "$p" "$OUT/${LABEL}_$1.png"; echo "   -> $OUT/${LABEL}_$1.png"
  else echo "   SHOT FAILED: $line"; fi }

taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; sleep 4; rm -f "$D/auto_command.txt"
cd "$D" && ./GameGuruMAX.exe > /dev/null 2>&1 &
sleep 25
wait_state hub 90 || { echo FAIL_HUB; exit 1; }
sleep 4
send "NAVIGATE hub" 20 >/dev/null; sleep 6
send "NAVIGATE hub.demo_games" 20 >/dev/null; sleep 4
send "SELECT_DEMO Aztec Game Kit Teaser" 20
sleep 3
send "CLICK edit_game" 20
sleep 8
wait_state storyboard 90 || { echo FAIL_STORYBOARD; exit 1; }
send "CLICK_ONLY_LEVEL" 25
wait_state editor 420 || { echo FAIL_EDITOR; exit 1; }
sleep 45

echo "== start camera"
CAM=$(send "GET_CAMERA" 20); echo "   $CAM"
# pos=(x,y,z) ang=(ax,ay)
X=$(echo "$CAM" | sed -n 's/.*pos=(\([-0-9.]*\),.*/\1/p')
Y=$(echo "$CAM" | sed -n 's/.*pos=([-0-9.]*,\([-0-9.]*\),.*/\1/p')
Z=$(echo "$CAM" | sed -n 's/.*pos=([-0-9.]*,[-0-9.]*,\([-0-9.]*\)).*/\1/p')
AX=$(echo "$CAM" | sed -n 's/.*ang=(\([-0-9.]*\),.*/\1/p')
AY=$(echo "$CAM" | sed -n 's/.*ang=([-0-9.]*,\([-0-9.]*\)).*/\1/p')
echo "   parsed X=$X Y=$Y Z=$Z AX=$AX AY=$AY"
shot 00_start

# The artefact needs the camera high enough to see billboarded ridge trees against cloud.
# Sweep a few heights at the start XZ, each looking slightly down, and pair clouds on/off.
i=0
for DY in 0 3000 8000; do
  i=$((i+1))
  NY=$(python -c "print(float('${Y:-0}') + $DY)")
  echo "== view $i  y=$NY"
  send "SET_CAMERA $X $NY $Z ${AX:-0} ${AY:-0}" 20 >/dev/null; sleep 6
  send "SET_VOLCLOUDS 1" 20 >/dev/null; sleep 4; shot "v${i}_clouds_on"
  send "SET_VOLCLOUDS 0" 20 >/dev/null; sleep 4; shot "v${i}_clouds_off"
  send "SET_VOLCLOUDS 1" 20 >/dev/null; sleep 3
done

echo "== is it the BILLBOARD pass? far trees off, clouds on"
send "SET_FARTREES 0" 20; sleep 5; shot "fartrees_off"
send "SET_FARTREES 1" 20 >/dev/null; sleep 5; shot "fartrees_on"

alive && echo "STILL ALIVE" || echo "!!! DIED"
taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true
echo "DONE $LABEL $(date +%H:%M:%S)"
