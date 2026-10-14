# GGMAX 3.112: does the grass survive TERRAIN BAKE, follow the camera while baked, and come back
# after it? MAX must already be in the editor on a level with painted grass (TESTPRO2 > DX12TESTLEVEL).
#
# usage: bake_grass_probe.sh [tag] [farX farZ]     farX/farZ = a second camera spot, still inside the
#                                                   editable area, far enough to need different chunks
#
# Reads after each step: HAIR_SYSTEMS (live grass systems), GRASS_CHUNKS, VRAM_MB, and DUMP_BAKE's
# grass line. ★ A count alone cannot tell "kept" from "regrown"; the bake step also prints how many
# systems were MOVED onto emitters (rehomed) - a regrow would show 0 there and a 30-frame gap.
source "$(dirname "$0")/lib.sh"
TAG="${1:-bg}"; FX="${2:-30000}"; FZ="${3:-30000}"
alive || { echo "MAX NOT RUNNING"; exit 1; }
grass() { send "GET_PERF_DATA" 20 | grep -E "^FPS:|^SYSTEM_MEM_MB|^VRAM_MB|^HAIR_SYSTEMS|^GRASS_CHUNKS" | tr '\n' ' '; echo; }
bakeline() { send "DUMP_BAKE" 20 | grep -E -A1 "wicked terrain|grass  " | grep -v "^--" ; }
waitbake() { # $1 = want "TORN" or "live"
  local i=0; while [ $i -lt 40 ]; do
    if send "DUMP_BAKE" 20 | grep -q "wicked terrain *: $1"; then return 0; fi; sleep 1; i=$((i+1)); done; return 1; }

CAM=$(send "GET_CAMERA" 10 | head -1); echo "camera: $CAM"
X0=$(echo "$CAM" | grep -o "pos=([^)]*" | sed 's/pos=(//' | cut -d, -f1)
Y0=$(echo "$CAM" | grep -o "pos=([^)]*" | sed 's/pos=(//' | cut -d, -f2)
Z0=$(echo "$CAM" | grep -o "pos=([^)]*" | sed 's/pos=(//' | cut -d, -f3)

echo "== 1. live terrain";            grass; shot "${TAG}_1_live" >/dev/null
send "SET_BAKETERRAIN 1" 20 >/dev/null
waitbake "TORN" || { echo "FAIL: bake never tore the terrain down"; exit 1; }
sleep 2
echo "== 2. baked (same spot)";      grass; bakeline; shot "${TAG}_2_baked" >/dev/null
send "SET_CAMERA $FX $Y0 $FZ" 10 >/dev/null; sleep 6
echo "== 3. baked, camera moved to $FX,$FZ"; grass; shot "${TAG}_3_baked_far" >/dev/null
send "SET_GRASSOFF 1" 10 >/dev/null; sleep 2
echo "== 4. baked + Grass Off (emitters DEMOTED to CPU copies: 0 emitters, all bare)"; grass; bakeline
# Grass back on: the bare copies are PROMOTED again (24 a frame, nearest first) - no re-bake, the
# baked ground must stay TORN DOWN throughout.
send "SET_GRASSOFF 0" 10 >/dev/null; sleep 6
send "DUMP_BAKE" 20 | grep -q "wicked terrain *: TORN" || echo "FAIL: the terrain came back (a re-bake ran)"
echo "== 5. baked + Grass back on (promoted)";  grass; bakeline; shot "${TAG}_5_grass_on" >/dev/null
send "SET_CAMERA $X0 $Y0 $Z0" 10 >/dev/null; sleep 6
echo "== 6. baked, camera home";     grass
send "SET_BAKETERRAIN 0" 20 >/dev/null
waitbake "still live" || { echo "FAIL: terrain never came back"; exit 1; }
sleep 12
echo "== 7. bake off";               grass; bakeline; shot "${TAG}_7_unbaked" >/dev/null
send "SET_BAKETERRAIN 1" 20 >/dev/null
waitbake "TORN" || { echo "FAIL: second bake never tore down"; exit 1; }
sleep 2
echo "== 8. baked again";            grass; bakeline
send "SET_BAKETERRAIN 0" 20 >/dev/null
waitbake "still live"; sleep 12
echo "== 9. bake off again";         grass
