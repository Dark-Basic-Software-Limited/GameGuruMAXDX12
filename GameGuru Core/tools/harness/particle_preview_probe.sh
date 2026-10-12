# GGMAX 3.110: Particle Library LIVE hover preview probe.
# MAX must already be in the editor with the Particle Library open (Object Tools > Particles >
# New Particle on a selected particle object).
#
# usage: particle_preview_probe.sh <tag> <x0> <y0> <x1> <y1> [nx0 ny0 nx1 ny1]
#   x0..y1   = the hovered tile's image rect in screenshot pixels (the cursor goes to its centre)
#   n*       = a NEIGHBOUR tile's rect, which must stay still (proves only the hovered cell animates)
#
# ★ A live preview has to CHANGE between shots. One shot cannot tell a live render from a static
#   thumbnail, so this takes five, ~0.4 s apart, and requires every consecutive pair to differ.
# ⚠ MOUSE_CLICK does not work on ImGui; this only moves the cursor (hover), it never clicks.
source "$(dirname "$0")/lib.sh"
TAG="${1:-pp}"; X0=$2; Y0=$3; X1=$4; Y1=$5; NX0=${6:-0}; NY0=${7:-0}; NX1=${8:-0}; NY1=${9:-0}
alive || { echo "MAX NOT RUNNING"; exit 1; }
CX=$(( (X0 + X1) / 2 )); CY=$(( (Y0 + Y1) / 2 ))
PY="$(cygpath -w "$(dirname "$0")/particle_preview_measure.py")"

send "MOUSE_AT 1240 760" 10 >/dev/null; sleep 2
echo "== baseline (not hovered)"
send "DUMP_OBJPREVIEW" 10 | grep -o "active=[0-9]*\|haveFrame=[0-9]*\|particle=[0-9-]*\|gpup=[0-9-]*\|wpeRoot=[0-9]*\|bbEmitter=[0-9-]*" | tr '\n' ' '; echo
shot "${TAG}_base" >/dev/null || exit 1

echo "== hover ($CX,$CY)"
send "MOUSE_AT $CX $CY" 10 >/dev/null
sleep 2.5
send "DUMP_OBJPREVIEW" 10 | grep -o "active=[0-9]*\|haveFrame=[0-9]*\|particle=[0-9-]*\|gpup=[0-9-]*\|wpeRoot=[0-9]*\|particleFrames=[0-9]*\|wpeSim=[0-9]*\|wpeSkip=[0-9]*\|bbEmitter=[0-9-]*" | tr '\n' ' '; echo
for i in 1 2 3 4 5; do shot "${TAG}_h$i" >/dev/null || exit 1; sleep 0.4; done
send "DUMP_OBJPREVIEW" 10 | grep -o "particleFrames=[0-9]*\|wpeSim=[0-9]*" | tr '\n' ' '; echo

echo "== un-hover"
send "MOUSE_AT 1240 760" 10 >/dev/null; sleep 2
send "DUMP_OBJPREVIEW" 10 | grep -o "active=[0-9]*\|particle=[0-9-]*\|bbEmitter=[0-9-]*\|previewWPERoot=[0-9]*" | tr '\n' ' '; echo
shot "${TAG}_unhover" >/dev/null || exit 1

python "$PY" "$(cygpath -w "$OUT")" "$TAG" $X0 $Y0 $X1 $Y1 $NX0 $NY0 $NX1 $NY1
