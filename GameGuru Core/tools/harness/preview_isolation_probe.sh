# GGMAX 3.121: does ANY level setting reach the Object / Particle Library live preview?
# MAX must already be in the editor with a library open; the cursor is put on the tile's centre.
#
# usage: preview_isolation_probe.sh <tag> <x0> <y0> <x1> <y1> [shots]
#   x0..y1 = the hovered tile's image rect in screenshot pixels
#
# Four states, each the median/mean of the tile over [shots] screenshots (default 4):
#   base       studio on, level as loaded
#   perturb    studio on, PERTURB_SCENE 1 (ambient red, fog magenta 0.002 from 0, wind 60, sun green x4)
#   leak       studio OFF, perturbed - the instrument's own check: this one MUST move, or the probe
#              cannot see a leak and its 'no change' means nothing
#   restored   studio on, PERTURB_SCENE 0
# PASS = perturb within 2/255 of base on every channel (median AND mean), leak moved > 10.
# ⚠ An object preview turns: freeze the turntable first (SET_OBJPREVIEW_LIGHT 0 4 45 1 0 0.35 1 1).
source "$(dirname "$0")/lib.sh"
TAG="${1:-iso}"; X0=$2; Y0=$3; X1=$4; Y1=$5; N=${6:-4}
alive || { echo "MAX NOT RUNNING"; exit 1; }
CX=$(( (X0 + X1) / 2 )); CY=$(( (Y0 + Y1) / 2 ))
PY="$(cygpath -w "$(dirname "$0")/preview_isolation_measure.py")"
LIGHT=$(send "SET_OBJPREVIEW_LIGHT" 10)   # ERROR reply carries the current k / abs / fov
K=$(echo "$LIGHT" | sed -n 's/.*k=\([0-9.]*\).*/\1/p'); A=$(echo "$LIGHT" | sed -n 's/.*abs=\([0-9.]*\).*/\1/p'); F=$(echo "$LIGHT" | sed -n 's/.*fov=\([0-9.]*\).*/\1/p')
studio() { send "SET_OBJPREVIEW_LIGHT ${K:-0} ${A:-4} ${F:-45} $1" 10 >/dev/null; }

send "MOUSE_AT $CX $CY" 10 >/dev/null; sleep 3
send "DUMP_OBJPREVIEW" 10 | grep -o "active=[0-9]*\|particle=[0-9]*\|studio=[0-9]*\|studioFrames=[0-9]*\|studioOwn=[0-9]*\|cubeAvg=[0-9.]*\|freeze=[0-9]*" | tr '\n' ' '; echo
grab() { local i; for i in $(seq 1 $N); do shot "${TAG}_$1_$i" >/dev/null || exit 1; sleep 0.3; done; }

studio 1; send "PERTURB_SCENE 0" 10 >/dev/null; sleep 2; grab base
send "PERTURB_SCENE 1" 10; echo; sleep 2; grab perturb
studio 0; sleep 2; grab leak
studio 1; send "PERTURB_SCENE 0" 10; echo; sleep 2; grab restored
shot "${TAG}_full" >/dev/null

python "$PY" "$(cygpath -w "$OUT")" "$TAG" $X0 $Y0 $X1 $Y1 $N
