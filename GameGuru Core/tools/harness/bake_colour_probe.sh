# GGMAX 3.113: baked vs live terrain COLOUR, measured. MAX in the editor on a level (TESTPRO2 >
# DX12TESTLEVEL); grass is switched off for the run so only the ground is measured.
#
# usage: bake_colour_probe.sh [tag] ["x y z angx angy"] [masks...]
#   masks: SET_BAKESHADING values for the BAKED shots (1 normal, 2 ssao, 4 probe ambient, 8 full sun,
#          16 live material; 31 = all = shipped). Default "0 1 31".
# Takes LIVE shots once (AO on and AO off), then BAKED shots per mask (AO on and off), all from the
# same camera, and bake_colour_measure.py prints each baked mask against live.
# ★ Ratio ~1 on every channel = match. A uniform ratio = brightness (lighting); a per-channel
#   ratio = hue (albedo or light colour). AO off isolates the SSAO term.
source "$(dirname "$0")/lib.sh"
TAG="${1:-bc}"; CAM="${2:-226 476 83 30 336}"; shift 2 2>/dev/null
MASKS="${*:-0 1 31}"
alive || { echo "MAX NOT RUNNING"; exit 1; }
waitbake() { local i=0; while [ $i -lt 60 ]; do send "DUMP_BAKE" 20 | grep -q "wicked terrain *: $1" && return 0; sleep 1; i=$((i+1)); done; return 1; }
send "SET_GRASSOFF 1" 10 >/dev/null
send "SET_CAMERA $CAM" 10 >/dev/null
# live first
send "SET_BAKETERRAIN 0" 10 >/dev/null; waitbake "still live" || { echo "FAIL live"; exit 1; }; sleep 15
shot "${TAG}_live" >/dev/null
send "SET_AOOFF 1" 10 >/dev/null; sleep 4; shot "${TAG}_live_aooff" >/dev/null; send "SET_AOOFF 0" 10 >/dev/null
# baked, per mask
send "SET_BAKETERRAIN 1" 10 >/dev/null; waitbake "TORN" || { echo "FAIL bake"; exit 1; }; sleep 3
for M in $MASKS; do
  send "SET_BAKESHADING $M" 10 >/dev/null; sleep 3; shot "${TAG}_baked_m$M" >/dev/null
  send "SET_AOOFF 1" 10 >/dev/null; sleep 4; shot "${TAG}_baked_m${M}_aooff" >/dev/null; send "SET_AOOFF 0" 10 >/dev/null; sleep 2
done
send "SET_BAKESHADING 31" 10 >/dev/null
send "SET_GRASSOFF 0" 10 >/dev/null
PY="$(cygpath -w "$(dirname "$0")/bake_colour_measure.py")"
python "$PY" "$(cygpath -w "$OUT")" "$TAG" $MASKS
