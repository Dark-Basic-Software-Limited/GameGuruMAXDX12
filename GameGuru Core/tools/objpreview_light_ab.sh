#!/bin/bash
# GGMAX 3.84: the preview object renders as a pure black SILHOUETTE - at exposure 64 the backdrop
# blows out to white and the object is still black, so it is receiving exactly ZERO light, not too
# little. But the very first archway hover of a fresh launch WAS lit. So the question is not
# "which objects" but "when does the light stop arriving".
#
# ★ This probe is about the SHAPE of the failure, not a still image. Same launch, same level,
#   same lights: prop -> character -> prop again. If the first prop is lit and the second is not,
#   the light is being lost progressively (state), not per object class.
D="/d/DEV/BUILD/GameGuru Wicked MAX Build Area/Max"
OUT="${GGMAX_PREVIEW_OUT:-/c/Users/leeba/AppData/Local/Temp/claude/D--max-GameGuruMAXDX12/9a28c586-4c13-4447-916e-7fb51301bfa8/scratchpad/objpreview}"
LABEL="${1:-ab}"
mkdir -p "$OUT"

send() { rm -f "$D/auto_result.txt"; echo "$1" > "$D/auto_command.txt"; local t=0
  while [ $t -lt ${2:-30} ]; do [ -f "$D/auto_result.txt" ] && { sleep 0.2; cat "$D/auto_result.txt"; return 0; }; sleep 1; t=$((t+1)); done; echo TIMEOUT; }
alive() { tasklist.exe //FI "IMAGENAME eq GameGuruMAX.exe" 2>/dev/null | grep -qi GameGuruMAX; }
wait_state() { local t=0; while [ $t -lt $2 ]; do alive || return 2
  [ "$(send GET_STATE 20 | head -1)" == "STATE: $1" ] && return 0; sleep 5; t=$((t+7)); done; return 1; }
shot() { local line; line=$(send "SCREENSHOT" 40); local p
  p=$(echo "$line" | sed 's/^OK: Screenshot saved to //' | sed 's|\\|/|g')
  [ -f "$p" ] && cp "$p" "$OUT/${LABEL}_$1.png" || echo "   SHOT FAILED: $line"; }
click() { send "MOUSE_AT $1 $2" 20 >/dev/null; sleep 1; send "MOUSE_DOWN" 20 >/dev/null; sleep 0.5
          send "MOUSE_UP" 20 >/dev/null; sleep "${3:-2}"; }
away() { send "MOUSE_AT 900 745" 20 >/dev/null; sleep 3; }
probe() { # <tag> <x> <y>
  send "MOUSE_AT $2 $3" 20 >/dev/null; sleep 5
  echo "   [$1] $(send DUMP_OBJPREVIEW 20)"
  shot "$1"; away; }

taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; sleep 4; rm -f "$D/auto_command.txt"
cd "$D" && ./GameGuruMAX.exe > /dev/null 2>&1 &
sleep 25
wait_state hub 90 || { echo FAIL_HUB; exit 1; }
sleep 4
send "OPEN_PROJECT TESTPRO2" 30 >/dev/null; sleep 6
wait_state storyboard 90 || { echo FAIL_STORYBOARD; exit 1; }
send "CLICK_NODE testpro2level" 25 >/dev/null
wait_state editor 420 || { echo FAIL_EDITOR; exit 1; }
sleep 30

click 23 120 3          # Add -> Object Library (opens on the Arctic Collection search)
probe 1_prop_first 623 290
probe 2_prop_again 623 290
probe 3_prop_neighbour 856 290
probe 4_prop_first_again 623 290
click 327 404 4         # Characters category
probe 5_character 680 327
probe 6_character_again 680 327
echo "== done"
