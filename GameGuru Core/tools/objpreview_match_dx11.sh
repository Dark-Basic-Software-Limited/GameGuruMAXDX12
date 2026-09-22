#!/bin/bash
# GGMAX 3.84b: match the live preview's light strength to the DX11 reference thumbnail.
#
# ★ THE STATISTIC MATTERS MORE THAN THE SWEEP. Comparing Lee's two screenshots on MEAN cell
#   luminance says they are identical (66.8 vs 67.6) - because the backdrop fills most of the cell
#   and swamps the subject. On p99 - the brightest 1%, which IS the lit side of the subject - they
#   are 145.6 vs 220.1, i.e. the DX12 highlight is 51% hotter. Same trap as the reflection fix:
#   measure the SIGNAL, not the frame.
#
# ★ SELF-CALIBRATING. Every run measures the STATIC thumbnail in the same cell, at the same
#   resolution, in the same session - and that static thumbnail IS the DX11-generated reference.
#   So the target travels with the run and no cross-resolution fudge is needed.
D="/d/DEV/BUILD/GameGuru Wicked MAX Build Area/Max"
OUT="${GGMAX_PREVIEW_OUT:-/c/Users/leeba/AppData/Local/Temp/claude/D--max-GameGuruMAXDX12/9a28c586-4c13-4447-916e-7fb51301bfa8/scratchpad/objpreview}"
SP="/c/Users/leeba/AppData/Local/Temp/claude/D--max-GameGuruMAXDX12/9a28c586-4c13-4447-916e-7fb51301bfa8/scratchpad"
LABEL="${1:-match}"
shift
KS="${*:-0.00010 0.00020 0.00030 0.00045 0.00060}"
# the Astra cell in a 1536x801 harness screenshot
CX=516; CY=236; CW=330; CH=178
mkdir -p "$OUT"

send() { rm -f "$D/auto_result.txt"; echo "$1" > "$D/auto_command.txt"; local t=0
  while [ $t -lt ${2:-30} ]; do [ -f "$D/auto_result.txt" ] && { sleep 0.2; cat "$D/auto_result.txt"; return 0; }; sleep 1; t=$((t+1)); done; echo TIMEOUT; }
alive() { tasklist.exe //FI "IMAGENAME eq GameGuruMAX.exe" 2>/dev/null | grep -qi GameGuruMAX; }
wait_state() { local t=0; while [ $t -lt $2 ]; do alive || return 2
  [ "$(send GET_STATE 20 | head -1)" == "STATE: $1" ] && return 0; sleep 5; t=$((t+7)); done; return 1; }
shot() { local l p; l=$(send "SCREENSHOT" 40)
  p=$(echo "$l" | sed "s/^OK: Screenshot saved to //" | tr '\\' '/')
  [ -f "$p" ] && cp "$p" "$OUT/${LABEL}_$1.png" && echo "$OUT/${LABEL}_$1.png"; }
stat() { python "$SP/cellstat2.py" "$1" $CX $CY $CW $CH "$2"; }
click() { send "MOUSE_AT $1 $2" 20 >/dev/null; sleep 1; send "MOUSE_DOWN" 20 >/dev/null; sleep 0.5
          send "MOUSE_UP" 20 >/dev/null; sleep "${3:-2}"; }
away() { send "MOUSE_AT 950 745" 20 >/dev/null; sleep 4; }

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
click 327 404 4
away
P=$(shot 00_static); [ -n "$P" ] && stat "$P" "DX11 static REFERENCE"

for k in $KS; do
  send "SET_OBJPREVIEW_LIGHT $k" 20 >/dev/null
  send "MOUSE_AT 680 327" 20 >/dev/null
  sleep 6
  P=$(shot "k$k"); [ -n "$P" ] && stat "$P" "live k=$k"
  away
done
echo "== done"
