# GGMAX 3.120: the baked water seen from BELOW vs the live ocean, same camera, same session. MAX in the editor.
# usage: water_bake_under_probe.sh [tag] ["x y z angx angy" (a pose under the water)] [gain x100]
# Shots: live (ocean on), old (Water Bake, 3.119 look), new (opaque ceiling + engine underwater fog), ghost (new with
# the engine's stock ceiling term, SET_UWCEILING 0). Prints the median colour of screen bands for each.
source "$(dirname "$0")/lib.sh"
TAG="${1:-uw}"; CAM="${2:-3500 -80 7500 -5 220}"; GAIN="${3:-100}"
alive || { echo "MAX NOT RUNNING"; exit 1; }
send "SET_CAMERA $CAM" 10 >/dev/null; sleep 2
send "SET_BAKETERRAIN 1" 20 >/dev/null
send "SET_BAKEWATER 0" 20 >/dev/null; sleep 10; shot "${TAG}_live" >/dev/null
send "SET_BAKEWATER 1" 20 >/dev/null; sleep 8
send "SET_WATERBAKEUNDER 0" 10 >/dev/null; sleep 3; shot "${TAG}_old" >/dev/null
send "SET_WATERBAKEUNDER 1 $GAIN" 10 >/dev/null; sleep 3; shot "${TAG}_new" >/dev/null
send "SET_UWCEILING 0" 10 >/dev/null; sleep 3; shot "${TAG}_ghost" >/dev/null; send "SET_UWCEILING 1" 10 >/dev/null
send DUMP_BAKE 20 | sed -n '/underwater 3.120/,+1p'
PY="$(cygpath -w "$(dirname "$0")/water_bake_under_measure.py")"
python "$PY" "$(cygpath -w "$OUT")" "$TAG"
