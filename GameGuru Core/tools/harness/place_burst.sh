# Place an object into the 3D view while capturing consecutive frames (report #4: one-frame
# polygon splay as a character is put down). MAX must already be in the editor.
# usage: place_burst.sh <pickX> <pickY> <placeX> <placeY> <frames> <tag>
#   pick = the object's thumbnail in the Level Items panel (attaches it to the cursor)
#   place = where to put it down in the 3D view
# Frames land in Max/Files/Files/screenshots (the game's CWD is Max/Files) and are MOVED to
# $OUT/<tag>/ - only the frame_###.png files this run wrote.
source "$(dirname "$0")/lib.sh"
PX=$1; PY=$2; QX=$3; QY=$4; N=${5:-60}; TAG=${6:-burst}
SS="$D/Files/Files/screenshots"
alive || { echo "MAX NOT RUNNING"; exit 1; }
rm -f "$SS"/frame_*.png 2>/dev/null
click "$PX" "$PY" 2
send "MOUSE_AT $QX $QY" 10 >/dev/null; sleep 2
send "BURST_FRAMES $N" 10
send "MOUSE_DOWN" 10 >/dev/null; sleep 0.4
send "MOUSE_UP" 10 >/dev/null
sleep 12
mkdir -p "$OUT/$TAG"
n=0; for f in "$SS"/frame_*.png; do [ -f "$f" ] && mv "$f" "$OUT/$TAG/" && n=$((n+1)); done
echo "FRAMES $n -> $OUT/$TAG"
