#!/bin/bash
# GGMAX 3.79: the soaks show ~+610 MB of VRAM residue on a level loaded in a session where other
# levels were loaded first - enough to push the three biggest demos over the 4 GB min-spec limit.
# But `gvram` is DRIVER-REPORTED usage, which counts free space pooled inside D3D12MA blocks. That
# is not the same as live data, and only one of the two is a capacity risk.
#
# THE DISCRIMINATOR: take the NAMED census of the SAME demo twice -
#   (a) loaded FIRST in a fresh process
#   (b) loaded after several other levels
# and compare census_bytes (live allocations) against driver_usage (what the driver committed).
#   census_bytes flat + driver_usage up  -> pooling/overhead, the allocator will reuse it
#   census_bytes up too                  -> genuinely retained data, a real 4 GB risk
D="/d/DEV/BUILD/GameGuru Wicked MAX Build Area/Max"
OUT="${GGMAX_RESIDUE_OUT:-/c/Users/leeba/AppData/Local/Temp/claude/D--max-GameGuruMAXDX12/9a28c586-4c13-4447-916e-7fb51301bfa8/scratchpad/residue}"
mkdir -p "$OUT"
TARGET="The Mystery of Z Island"     # the soak peak, every run

send() { rm -f "$D/auto_result.txt"; echo "$1" > "$D/auto_command.txt"; local t=0
  while [ $t -lt ${2:-30} ]; do [ -f "$D/auto_result.txt" ] && { sleep 0.2; cat "$D/auto_result.txt"; return 0; }; sleep 1; t=$((t+1)); done; echo TIMEOUT; }
alive() { tasklist.exe //FI "IMAGENAME eq GameGuruMAX.exe" 2>/dev/null | grep -qi GameGuruMAX; }
wait_state() { local t=0; while [ $t -lt $2 ]; do alive || return 2
  [ "$(send GET_STATE 20 | head -1)" == "STATE: $1" ] && return 0; sleep 5; t=$((t+7)); done; return 1; }

open_demo() {   # the soak's exact route
  send "NAVIGATE hub" 20 >/dev/null; sleep 6
  send "NAVIGATE hub.demo_games" 20 >/dev/null; sleep 4
  send "SELECT_DEMO $1" 20 >/dev/null; sleep 3
  send "CLICK edit_game" 20 >/dev/null; sleep 8
  wait_state storyboard 90 || return 1
  send "CLICK_ONLY_LEVEL" 20 >/dev/null
  wait_state editor 420 || return 1
  sleep 40; return 0
}
census() {
  send "DUMP_VRAM $1" 60 >/dev/null; sleep 2
  local C="$D/Files/vram_census_$1.txt"
  [ -f "$C" ] && cp "$C" "$OUT/vram_census_$1.txt"
  if [ -f "$C" ]; then
    head -1 "$C" | tr ' ' '\n' | grep -E "^(census_bytes|census_default_heap|d3d12ma_allocated|d3d12ma_blocks|driver_usage|records)=" \
      | sed "s/^/   $1  /"
  else echo "   $1  NO CENSUS"; fi
}

taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; sleep 4; rm -f "$D/auto_command.txt"
cd "$D" && ./GameGuruMAX.exe > /dev/null 2>&1 &
sleep 25
wait_state hub 90 || { echo FAIL_HUB; exit 1; }
sleep 4

echo "== A: $TARGET loaded FIRST in a fresh process"
open_demo "$TARGET" || { echo FAIL_A; exit 1; }
send "GET_PERF_DATA" 25 | tr ',' '\n' | grep -iE "VRAM|GVRAM" | sed 's/^/   /'
census zisland_first

echo "== now load three other levels, then come back"
for d in "Aztec Game Kit Teaser" "Aztec Game Kit" "Operation Amazon"; do
  echo "   -- $d"
  open_demo "$d" || echo "   (failed $d)"
done

echo "== B: $TARGET loaded AGAIN, same process, after three other levels"
open_demo "$TARGET" || { echo FAIL_B; exit 1; }
send "GET_PERF_DATA" 25 | tr ',' '\n' | grep -iE "VRAM|GVRAM" | sed 's/^/   /'
census zisland_after3

alive && echo "STILL ALIVE" || echo "!!! DIED"
taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true
echo "DONE $(date +%H:%M:%S)"
