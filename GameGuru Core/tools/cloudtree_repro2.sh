#!/bin/bash
# GGMAX 3.77: Lee's dedicated repro level for clouds drawn over far tree billboards.
# TESTPRO2 / testpro2level, saved at a camera where billboards poke above the hill silhouette.
# Pairs every view with clouds ON / OFF at a FIXED camera so a fix can be A/B'd frame for frame.
#   usage: cloudtree_repro2.sh <label>
D="/d/DEV/BUILD/GameGuru Wicked MAX Build Area/Max"
OUT="${GGMAX_CLOUD_OUT:-/c/Users/leeba/AppData/Local/Temp/claude/D--max-GameGuruMAXDX12/9a28c586-4c13-4447-916e-7fb51301bfa8/scratchpad/cloudtree2}"
LABEL="${1:-before}"
mkdir -p "$OUT"

send() { rm -f "$D/auto_result.txt"; echo "$1" > "$D/auto_command.txt"; local t=0
  while [ $t -lt ${2:-30} ]; do [ -f "$D/auto_result.txt" ] && { sleep 0.2; cat "$D/auto_result.txt"; return 0; }; sleep 1; t=$((t+1)); done; echo TIMEOUT; }
alive() { tasklist.exe //FI "IMAGENAME eq GameGuruMAX.exe" 2>/dev/null | grep -qi GameGuruMAX; }
wait_state() { local t=0; while [ $t -lt $2 ]; do alive || return 2
  [ "$(send GET_STATE 20 | head -1)" == "STATE: $1" ] && return 0; sleep 5; t=$((t+7)); done; return 1; }
shot() { local line; line=$(send "SCREENSHOT x" 40); local p
  p=$(echo "$line" | sed 's/^OK: Screenshot saved to //' | tr '\\' '/')
  if [ -f "$p" ]; then cp "$p" "$OUT/${LABEL}_$1.png"; echo "   -> ${LABEL}_$1.png"
  else echo "   SHOT FAILED: $line"; fi }

taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; sleep 4; rm -f "$D/auto_command.txt"
cd "$D" && ./GameGuruMAX.exe > /dev/null 2>&1 &
sleep 25
wait_state hub 90 || { echo FAIL_HUB; exit 1; }
sleep 4
send "OPEN_PROJECT TESTPRO2" 30
sleep 6
wait_state storyboard 90 || { echo FAIL_STORYBOARD; exit 1; }
# CLICK_NODE loads the level directly - CLICK_ONLY_LEVEL would pick the first level node instead.
send "CLICK_NODE testpro2level" 25
wait_state editor 420 || { echo FAIL_EDITOR; exit 1; }
sleep 45

CAM=$(send "GET_CAMERA" 20); echo "   $CAM"
echo "$LABEL $CAM" >> "$OUT/cameras.txt"

echo "== saved camera (Lee framed the repro here)"
send "SET_VOLCLOUDS 1" 20 >/dev/null; sleep 6; shot "v0_clouds_on"
send "SET_VOLCLOUDS 0" 20 >/dev/null; sleep 5; shot "v0_clouds_off"
send "SET_VOLCLOUDS 1" 20 >/dev/null; sleep 5

echo "== same camera, billboards off (are the white blobs on billboard pixels?)"
send "SET_FARTREES 0" 20 | head -3; sleep 6; shot "v0_fartrees_off"
send "SET_FARTREES 1" 20 >/dev/null; sleep 6; shot "v0_fartrees_on"

echo "== a second frame after 4 s, same camera - anything that MOVES is the parity flicker"
sleep 4; shot "v0_clouds_on_t2"
sleep 4; shot "v0_clouds_on_t3"

alive && echo "STILL ALIVE" || echo "!!! DIED"
taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true
echo "DONE $LABEL $(date +%H:%M:%S)"
