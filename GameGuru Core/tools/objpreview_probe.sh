#!/bin/bash
# GGMAX 3.83: object-library LIVE PREVIEW probe.
#
# The thing under test cannot be reached by any existing harness verb: it is triggered by
# ImGui::IsItemHovered(), which reads io.MousePos, which comes from the OS cursor. So this
# probe drives the REAL cursor through MOUSE_AT / MOUSE_DOWN / MOUSE_UP (added in 3.83) and
# reads the chain state back through DUMP_OBJPREVIEW.
#
# ★ Several screenshots per hover, ~1s apart. A live preview is not "an image appeared" - it
#   has to CHANGE between shots, because the object is rotating. One shot cannot tell a live
#   render from a static thumbnail, and the motion IS the feature.
#
# ⚠ MOUSE_CLICK (down+up in one SendInput) does NOT work on ImGui widgets - one message pump
#   drains both and io.MouseDown[0] nets to false. Always AT / DOWN / sleep / UP.
#
# Usage: objpreview_probe.sh <label> [mode]     mode = SET_OBJPREVIEW stage, default 3
D="/d/DEV/BUILD/GameGuru Wicked MAX Build Area/Max"
OUT="${GGMAX_PREVIEW_OUT:-/c/Users/leeba/AppData/Local/Temp/claude/D--max-GameGuruMAXDX12/9a28c586-4c13-4447-916e-7fb51301bfa8/scratchpad/objpreview}"
LABEL="${1:-run}"
MODE="${2:-3}"
# Archway thumbnail in the default "Arctic Collection" library view at 1536x801
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
# a hang shows up as the harness going silent while the process stays alive at 0% CPU
# (documented signature: a modal owns the message pump). Say so instead of timing out blind.
health() { if alive; then
    if [ "$(send GET_STATE 8 | head -1)" == "TIMEOUT" ]; then echo "   !! HARNESS SILENT - process alive. Device hang / modal. Check dred_report.txt"; return 1; fi
  else echo "   !! PROCESS GONE"; return 1; fi; return 0; }

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

echo "== editor reached, staging mode $MODE"
send "SET_OBJPREVIEW $MODE" 20 | sed 's/^/   /'
send "DUMP_OBJPREVIEW" 20 | sed 's/^/   /'

echo "== open the Object Library (Add button, Level Objects panel)"
click 23 120 3
shot 01_library
health || exit 2

echo "== hover the Archway thumbnail at ($HX,$HY)"
send "MOUSE_AT $HX $HY" 20 | sed 's/^/   /'
for i in 1 2 3 4; do
  sleep 2
  health || { echo "   died after hover sample $i"; exit 2; }
  send "DUMP_OBJPREVIEW" 20 | sed 's/^/   /'
  shot "02_hover$i"
done

echo "== move away, the static thumbnail must come back"
send "MOUSE_AT 900 740" 20 >/dev/null; sleep 3
health || exit 2
send "DUMP_OBJPREVIEW" 20 | sed 's/^/   /'
shot 03_unhover
echo "== done"
