# GGMAX 3.115: baked vs live terrain SHARPNESS, measured. MAX in the editor on a level (TESTPRO2 >
# DX12TESTLEVEL); grass off for the run so only the ground is measured.
#
# usage: bake_sharpness_probe.sh [tag] [masks...]
#   masks: SET_BAKESHADING values for the BAKED shots. Bits 0x1F = the 3.113 shading terms,
#          0x20 aniso sampler, 0x40 near detail colour, 0x80 near detail normals. Default "31".
# Three cameras (the editor's default view, the 3.113 view, and one looking down 45 degrees). LIVE
# shots once per camera, then BAKED shots per mask per camera; bake_sharpness_measure.py prints the
# high-frequency energy (mean |3x3 Laplacian| of luma) and mean colour per screen band, baked vs live.
# ★ HF ratio b/l near 1 = as sharp as live; well below 1 = blurry. Colour ratio ~1 = no shift.
source "$(dirname "$0")/lib.sh"
TAG="${1:-bs}"; shift 1 2>/dev/null
MASKS="${*:-31}"
CAMS=("226 476 83 17 336" "226 476 83 30 336" "226 900 83 45 336")
alive || { echo "MAX NOT RUNNING"; exit 1; }
waitbake() { local i=0; while [ $i -lt 60 ]; do send "DUMP_BAKE" 20 | grep -q "wicked terrain *: $1" && return 0; sleep 1; i=$((i+1)); done; return 1; }
send "SET_GRASSOFF 1" 10 >/dev/null
send "SET_BAKETERRAIN 0" 10 >/dev/null; waitbake "still live" || { echo "FAIL live"; exit 1; }
for i in 0 1 2; do
  send "SET_CAMERA ${CAMS[$i]}" 10 >/dev/null; sleep 12   # the VT has to page the near tiles in
  shot "${TAG}_c${i}_live" >/dev/null
done
send "SET_BAKESHADING 255" 10 >/dev/null   # what a bake KEEPS (detail normals) is decided by the mask at bake time
send "SET_BAKETERRAIN 1" 10 >/dev/null; waitbake "TORN" || { echo "FAIL bake"; exit 1; }; sleep 3
for M in $MASKS; do
  send "SET_BAKESHADING $M" 10 >/dev/null
  for i in 0 1 2; do
    send "SET_CAMERA ${CAMS[$i]}" 10 >/dev/null; sleep 3
    shot "${TAG}_c${i}_baked_m$M" >/dev/null
  done
done
send "SET_BAKESHADING 255" 10 >/dev/null
send "SET_GRASSOFF 0" 10 >/dev/null
send "SET_CAMERA ${CAMS[0]}" 10 >/dev/null
PY="$(cygpath -w "$(dirname "$0")/bake_sharpness_measure.py")"
python "$PY" "$(cygpath -w "$OUT")" "$TAG" $MASKS
