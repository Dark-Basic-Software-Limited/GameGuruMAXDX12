#!/bin/bash
# GGMAX 3.84c: "Bodger - Creeper Zombie" renders mostly BLACK in the live preview while its DX11
# static thumbnail shows a light striped vest and brown trousers. The cap and skin look right, so
# it is PER-MATERIAL, not per-object - which rules out the lighting (that would darken everything
# equally) and points at the textures.
#
# ★ The A/B that separates the two candidates, in one run:
#     streaming     - the preview loads the object fresh and Wicked streams its textures in; a
#                     material whose mips have not arrived yet can sample to nothing.
#                     SET_TEXSTREAM 0 stops NEWLY loaded textures being enrolled at all.
#     content/bind  - the material genuinely has no base-colour texture, or a black one.
#   DUMP_STREAM writes every material's per-slot CURRENT resolution + resource name, so it
#   distinguishes "not resident yet" from "not there at all" without guessing.
D="/d/DEV/BUILD/GameGuru Wicked MAX Build Area/Max"
OUT="${GGMAX_PREVIEW_OUT:-/c/Users/leeba/AppData/Local/Temp/claude/D--max-GameGuruMAXDX12/9a28c586-4c13-4447-916e-7fb51301bfa8/scratchpad/objpreview}"
LABEL="${1:-zom}"
HX="${HOVERX:-684}"
HY="${HOVERY:-352}"
mkdir -p "$OUT"

send() { rm -f "$D/auto_result.txt"; echo "$1" > "$D/auto_command.txt"; local t=0
  while [ $t -lt ${2:-30} ]; do [ -f "$D/auto_result.txt" ] && { sleep 0.2; cat "$D/auto_result.txt"; return 0; }; sleep 1; t=$((t+1)); done; echo TIMEOUT; }
alive() { tasklist.exe //FI "IMAGENAME eq GameGuruMAX.exe" 2>/dev/null | grep -qi GameGuruMAX; }
wait_state() { local t=0; while [ $t -lt $2 ]; do alive || return 2
  [ "$(send GET_STATE 20 | head -1)" == "STATE: $1" ] && return 0; sleep 5; t=$((t+7)); done; return 1; }
shot() { local l p; l=$(send "SCREENSHOT" 40)
  p=$(echo "$l" | sed "s/^OK: Screenshot saved to //" | tr '\\' '/')
  [ -f "$p" ] && cp "$p" "$OUT/${LABEL}_$1.png" && echo "   -> ${LABEL}_$1.png"; }
click() { send "MOUSE_AT $1 $2" 20 >/dev/null; sleep 1; send "MOUSE_DOWN" 20 >/dev/null; sleep 0.5
          send "MOUSE_UP" 20 >/dev/null; sleep "${3:-2}"; }
away() { send "MOUSE_AT 950 745" 20 >/dev/null; sleep 4; }

taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; sleep 4; rm -f "$D/auto_command.txt" "$D/stream_dump.txt"
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
shot 00_library
echo "== hover the zombie at ($HX,$HY) - streaming AS SHIPPED"
send "MOUSE_AT $HX $HY" 20 >/dev/null; sleep 10
send "DUMP_OBJPREVIEW" 20 | sed 's/^/   /'
shot 01_stream_on
send "DUMP_STREAM" 60 | sed 's/^/   /'
cp "$D/stream_dump.txt" "$OUT/${LABEL}_stream_on.txt" 2>/dev/null && echo "   -> ${LABEL}_stream_on.txt"

echo "== hold the hover another 20s - does it heal as mips arrive?"
sleep 20
shot 02_stream_on_later

echo "== now with texture-streaming ENROLMENT off, reloaded fresh"
away
send "SET_TEXSTREAM 0" 20 | sed 's/^/   /'
sleep 2
send "MOUSE_AT $HX $HY" 20 >/dev/null; sleep 12
shot 03_texstream_off
send "DUMP_STREAM" 60 >/dev/null
cp "$D/stream_dump.txt" "$OUT/${LABEL}_stream_off.txt" 2>/dev/null && echo "   -> ${LABEL}_stream_off.txt"
echo "== done"
