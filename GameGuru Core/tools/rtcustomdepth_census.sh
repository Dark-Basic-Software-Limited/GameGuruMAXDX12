#!/bin/bash
# GGMAX 3.77: the fresh sweep's median VRAM rise was +15.5 MB against a predicted +7.91 MB for
# rtCustomDepth. Do not argue about it - ask the named census, which records every D3D12MA
# allocation with its debug name. Confirms (a) the texture really is allocated, i.e. the fix is
# live, and (b) exactly what it costs.
D="/d/DEV/BUILD/GameGuru Wicked MAX Build Area/Max"
OUT="${GGMAX_CENSUS_OUT:-/c/Users/leeba/AppData/Local/Temp/claude/D--max-GameGuruMAXDX12/9a28c586-4c13-4447-916e-7fb51301bfa8/scratchpad/census377}"
mkdir -p "$OUT"

send() { rm -f "$D/auto_result.txt"; echo "$1" > "$D/auto_command.txt"; local t=0
  while [ $t -lt ${2:-30} ]; do [ -f "$D/auto_result.txt" ] && { sleep 0.2; cat "$D/auto_result.txt"; return 0; }; sleep 1; t=$((t+1)); done; echo TIMEOUT; }
alive() { tasklist.exe //FI "IMAGENAME eq GameGuruMAX.exe" 2>/dev/null | grep -qi GameGuruMAX; }
wait_state() { local t=0; while [ $t -lt $2 ]; do alive || return 2
  [ "$(send GET_STATE 20 | head -1)" == "STATE: $1" ] && return 0; sleep 5; t=$((t+7)); done; return 1; }

taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; sleep 4; rm -f "$D/auto_command.txt"
cd "$D" && ./GameGuruMAX.exe > /dev/null 2>&1 &
sleep 25
wait_state hub 90 || { echo FAIL_HUB; exit 1; }
sleep 4
send "OPEN_PROJECT TESTPRO2" 30 >/dev/null; sleep 6
wait_state storyboard 90 || { echo FAIL_STORYBOARD; exit 1; }
send "CLICK_NODE testpro2level" 25 >/dev/null
wait_state editor 420 || { echo FAIL_EDITOR; exit 1; }
sleep 40

send "DUMP_VRAM rt377" 60
sleep 2
C="$D/Files/vram_census_rt377.txt"
if [ -f "$C" ]; then
  cp "$C" "$OUT/vram_census_rt377.txt"
  echo "== the render targets the prepass owns =="
  grep -iE "rtCustomDepth|rtPrimitiveID|depthBuffer" "$C" | sed 's/^/   /'
  echo "== header (census vs driver) =="
  head -12 "$C" | sed 's/^/   /'
else
  echo "no census file at $C"
fi
taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true
echo "DONE $(date +%H:%M:%S)"
