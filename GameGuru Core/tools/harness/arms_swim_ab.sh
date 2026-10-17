# GGMAX 3.122: A/B the swimming arms across arm sets in ONE MAX session (each set = its own Test Game).
# For each set: ESC to the editor, SET_PLAYERARMS, Test Game, swimming.lua on element 3, player under
# deep water, a breaststroke played, 6 shots during the stroke. Crops the lower half into one grid.
#
# usage: arms_swim_ab.sh <tag> "<set1>" "<set2>" ...   (MAX must already be in the editor or a test game)
source "$(dirname "$0")/lib.sh"
TAG="$1"; shift
send "SET_ENTITYSCRIPT 3 global\\swimming.lua" 10 >/dev/null
k=0
for ARMS in "$@"; do
  if [ "$(send GET_STATE 10 | head -1)" == "STATE: game" ]; then send "PRESS_ESCAPE" 10 >/dev/null; wait_state editor 120; sleep 4; fi
  send "SET_ENTITYSCRIPT 3 global\\swimming.lua" 10 >/dev/null
  send "SET_PLAYERARMS $ARMS" 10 >/dev/null
  send "CLICK test_level" 10 >/dev/null
  wait_state game 180 || { echo "FAIL: no game state for $ARMS"; exit 1; }
  sleep 8
  send "RUN_LUA SetFreezePosition(3500,-80,7500); SetFreezeAngle(20,0,0); TransportToFreezePositionOnly(); return 1" 10 >/dev/null
  sleep 4
  send 'RUN_LUA SetGunAnimationSpeed(100); PlayGunAnimation(70,110); return 1' 10 >/dev/null
  for i in 1 2 3 4 5 6; do sleep 0.15; shot "${TAG}_${k}_$i" >/dev/null; done
  d=$(send DUMP_GUNARMS 10)
  echo "[$k] $ARMS :: $(echo "$d" | grep -o "customArms='[^']*'") :: $(echo "$d" | grep -o "gun[0-9]* 'interactive.swimming'[^|]*")"
  k=$((k+1))
done
python - "$TAG" "$k" <<'EOF'
import os, sys
from PIL import Image
b = os.path.expanduser('~/ggmax_harness_out'); tag = sys.argv[1]; n = int(sys.argv[2])
rows = []
for k in range(n):
    ims = [Image.open(os.path.join(b, '%s_%d_%d.png' % (tag, k, i))).convert('RGB').crop((0, 420, 1536, 864)).resize((512, 148)) for i in (3, 4, 5)]
    row = Image.new('RGB', (1536, 148))
    for i, im in enumerate(ims): row.paste(im, (i * 512, 0))
    rows.append(row)
out = Image.new('RGB', (1536, 152 * n), (255, 255, 255))
for k, r in enumerate(rows): out.paste(r, (0, k * 152))
out.save(os.path.join(b, '%s_grid.png' % tag)); print(os.path.join(b, '%s_grid.png' % tag))
EOF
