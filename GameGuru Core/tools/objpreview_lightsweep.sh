#!/bin/bash
# GGMAX 3.84: sweep the preview light intensity against ONE live hover, and read back where the
# light actually IS (LightComponent::position, the baked world position the renderer uses) rather
# than where we asked it to go.
#
# ★ The object is a pure black silhouette while its UNLIT backdrop renders correctly, and at
#   exposure 64 the backdrop blows out to white and the object is STILL black - so it receives
#   exactly zero light. Two candidates: the light is somewhere else, or 8 candela at ~50 world
#   units (~1.3 m at GG's ~39 units/metre) is simply nothing. One run answers both.
D="/d/DEV/BUILD/GameGuru Wicked MAX Build Area/Max"
OUT="${GGMAX_PREVIEW_OUT:-/c/Users/leeba/AppData/Local/Temp/claude/D--max-GameGuruMAXDX12/9a28c586-4c13-4447-916e-7fb51301bfa8/scratchpad/objpreview}"
LABEL="${1:-lit}"
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

click 23 120 3           # Add -> Object Library
click 327 404 4          # Characters, so the subject is the reported case (a character)
send "MOUSE_AT 680 327" 20 >/dev/null
sleep 6
echo "== baseline (whatever EnableThumbLight made)"
send "DUMP_OBJPREVIEW" 20 | sed 's/^/   /'
shot 00_baseline

for i in 0.02 0.06 0.15 0.35; do
  send "SET_OBJPREVIEW_LIGHT $i" 20 >/dev/null
  sleep 3
  echo "== k $i"
  send "DUMP_OBJPREVIEW" 20 | sed 's/^/   /'
  shot "i$i"
done
echo "== done"
