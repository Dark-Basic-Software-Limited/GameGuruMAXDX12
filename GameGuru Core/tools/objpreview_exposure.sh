#!/bin/bash
# GGMAX 3.83: sweep the preview exposure against ONE live hover.
#
# The preview inherits nothing from the level's grading any more, so the only question left is
# what fixed value to bake in. Sweeping it in one session beats one rebuild per guess, and it
# has to be judged on the SAME object, same lights, same frame-to-frame rotation - a value
# picked from separate runs would be comparing different rotations as well as different grades.
D="/d/DEV/BUILD/GameGuru Wicked MAX Build Area/Max"
OUT="${GGMAX_PREVIEW_OUT:-/c/Users/leeba/AppData/Local/Temp/claude/D--max-GameGuruMAXDX12/9a28c586-4c13-4447-916e-7fb51301bfa8/scratchpad/objpreview}"
LABEL="${1:-exp}"
HX="${HOVERX:-623}"
HY="${HOVERY:-290}"
mkdir -p "$OUT"

send() { rm -f "$D/auto_result.txt"; echo "$1" > "$D/auto_command.txt"; local t=0
  while [ $t -lt ${2:-30} ]; do [ -f "$D/auto_result.txt" ] && { sleep 0.2; cat "$D/auto_result.txt"; return 0; }; sleep 1; t=$((t+1)); done; echo TIMEOUT; }
alive() { tasklist.exe //FI "IMAGENAME eq GameGuruMAX.exe" 2>/dev/null | grep -qi GameGuruMAX; }
wait_state() { local t=0; while [ $t -lt $2 ]; do alive || return 2
  [ "$(send GET_STATE 20 | head -1)" == "STATE: $1" ] && return 0; sleep 5; t=$((t+7)); done; return 1; }
shot() { local line; line=$(send "SCREENSHOT" 40); local p
  p=$(echo "$line" | sed 's/^OK: Screenshot saved to //' | sed 's|\\|/|g')
  if [ -f "$p" ]; then cp "$p" "$OUT/${LABEL}_$1.png"; echo "   -> ${LABEL}_$1.png"
  else echo "   SHOT FAILED: $line"; fi }
click() { send "MOUSE_AT $1 $2" 20 >/dev/null; sleep 1; send "MOUSE_DOWN" 20 >/dev/null; sleep 0.5
          send "MOUSE_UP" 20 >/dev/null; sleep "${3:-2}"; }

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

click 23 120 3
send "MOUSE_AT $HX $HY" 20 >/dev/null
sleep 5
for e in 0.30 0.60 1.00 1.60 2.50 4.00; do
  send "SET_OBJPREVIEW 3 $e" 20 | sed 's/^/   /'
  sleep 2
  shot "e$e"
done
echo "== done"
