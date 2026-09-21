#!/bin/bash
# GGMAX 3.79b: is the retained terrain-texture set BOUNDED or does it GROW?
#
# The 3-level probe found 15 retained terraintextures/matN entries (~65 MB). Two explanations
# that call for opposite actions:
#   (a) gg_prevMaterialSetRetention (GGTerrainWicked.cpp:678) deliberately pins the PREVIOUS
#       swap's textures for one generation, for the 08-05 crash rule. Bounded. Not a bug.
#   (b) material slots overwritten in place orphan their entities, so every level leaves its set
#       behind. Unbounded. A real leak.
# Loading MORE intervening levels separates them: (a) stays flat, (b) scales.
#
# ⚠ Checked first: leakterraintex.txt is NOT present, so the 2.05 gg_pins diagnostic
# (wiResourceManager.cpp:1090, pins every terraintextures/* for the process) is not armed.
D="/d/DEV/BUILD/GameGuru Wicked MAX Build Area/Max"
OUT="${GGMAX_SCALE_OUT:-/c/Users/leeba/AppData/Local/Temp/claude/D--max-GameGuruMAXDX12/9a28c586-4c13-4447-916e-7fb51301bfa8/scratchpad/residue_scale}"
mkdir -p "$OUT"
TARGET="The Mystery of Z Island"

send() { rm -f "$D/auto_result.txt"; echo "$1" > "$D/auto_command.txt"; local t=0
  while [ $t -lt ${2:-30} ]; do [ -f "$D/auto_result.txt" ] && { sleep 0.2; cat "$D/auto_result.txt"; return 0; }; sleep 1; t=$((t+1)); done; echo TIMEOUT; }
alive() { tasklist.exe //FI "IMAGENAME eq GameGuruMAX.exe" 2>/dev/null | grep -qi GameGuruMAX; }
wait_state() { local t=0; while [ $t -lt $2 ]; do alive || return 2
  [ "$(send GET_STATE 20 | head -1)" == "STATE: $1" ] && return 0; sleep 5; t=$((t+7)); done; return 1; }
open_demo() {
  send "NAVIGATE hub" 20 >/dev/null; sleep 6
  send "NAVIGATE hub.demo_games" 20 >/dev/null; sleep 4
  send "SELECT_DEMO $1" 20 >/dev/null; sleep 3
  send "CLICK edit_game" 20 >/dev/null; sleep 8
  wait_state storyboard 90 || return 1
  send "CLICK_ONLY_LEVEL" 20 >/dev/null
  wait_state editor 420 || return 1
  sleep 35; return 0
}
census() {
  send "DUMP_VRAM $1" 60 >/dev/null; sleep 2
  local C="$D/Files/vram_census_$1.txt"
  [ -f "$C" ] || { echo "   $1  NO CENSUS"; return; }
  cp "$C" "$OUT/vram_census_$1.txt"
  local n mb du
  n=$(grep -c "terraintextures" "$C")
  mb=$(grep "terraintextures" "$C" | awk '{s+=$2} END {printf "%.1f", s/1048576}')
  du=$(head -1 "$C" | tr ' ' '\n' | grep "^driver_usage=" | cut -d= -f2)
  cb=$(head -1 "$C" | tr ' ' '\n' | grep "^census_bytes=" | cut -d= -f2)
  echo "   $1: terraintextures=$n entries, ${mb} MB | census_bytes=$((cb/1048576)) MB | driver_usage=$((du/1048576)) MB"
  echo "$1 terraintex=$n mb=$mb census=$((cb/1048576)) driver=$((du/1048576))" >> "$OUT/summary.txt"
  grep "terraintextures" "$C" | sed 's/.*terraintextures.//; s/\/[A-Za-z]*\.dds.*//' | sort | uniq -c | sort -rn | head -20 | sed 's/^/        /'
}

taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; sleep 4; rm -f "$D/auto_command.txt"
cd "$D" && ./GameGuruMAX.exe > /dev/null 2>&1 &
sleep 25
wait_state hub 90 || { echo FAIL_HUB; exit 1; }
sleep 4

echo "== baseline: $TARGET loaded FIRST"
open_demo "$TARGET" || { echo FAIL_BASE; exit 1; }
census scale_base

i=0
for d in "Aztec Game Kit Teaser" "Aztec Game Kit" "Operation Amazon" "Canyon Offensive" "RPG Template" "River Raiders"; do
  i=$((i+1))
  echo "== intervening level $i: $d"
  open_demo "$d" || { echo "   (failed $d)"; continue; }
  if [ $i -eq 3 ] || [ $i -eq 6 ]; then
    echo "   -- return to $TARGET after $i intervening levels"
    open_demo "$TARGET" || { echo FAIL_RET; continue; }
    census "scale_after$i"
  fi
done

echo "== VERDICT DATA =="; cat "$OUT/summary.txt"
alive && echo "STILL ALIVE" || echo "!!! DIED"
taskkill.exe //IM GameGuruMAX.exe //F 2>/dev/null; true
echo "DONE $(date +%H:%M:%S)"
