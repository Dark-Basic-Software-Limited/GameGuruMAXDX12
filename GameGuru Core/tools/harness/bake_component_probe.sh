# GGMAX 3.113: baked vs live terrain, ONE LIGHTING TERM AT A TIME. MAX in the editor on a level.
# Uses the engine's SET_TANGENTVIS debug views, which the bake pixel shader answers with the same
# quantities and encoding (GGTerrainBakePS.hlsl): 9 albedo in, 13 occlusion, 17 sun diffuse,
# 18 direct specular, 19 ambient diffuse, 20 reflections, 23 albedo out, 3 normal.
#
# usage: bake_component_probe.sh [tag] ["x y z angx angy"] [bakeMask] [views...]
source "$(dirname "$0")/lib.sh"
TAG="${1:-bk}"; CAM="${2:-226 476 83 30 336}"; MASK="${3:-31}"; shift 3 2>/dev/null
VIEWS="${*:-9 23 13 17 19 20 3}"
alive || { echo "MAX NOT RUNNING"; exit 1; }
waitbake() { local i=0; while [ $i -lt 60 ]; do send "DUMP_BAKE" 20 | grep -q "wicked terrain *: $1" && return 0; sleep 1; i=$((i+1)); done; return 1; }
send "SET_GRASSOFF 1" 10 >/dev/null
send "SET_CAMERA $CAM" 10 >/dev/null
send "SET_BAKETERRAIN 0" 10 >/dev/null; waitbake "still live" || { echo "FAIL live"; exit 1; }; sleep 15
for V in $VIEWS; do send "SET_TANGENTVIS $V" 10 >/dev/null; sleep 3; shot "${TAG}_live_v$V" >/dev/null; done
send "SET_TANGENTVIS 0" 10 >/dev/null
send "SET_BAKETERRAIN 1" 10 >/dev/null; waitbake "TORN" || { echo "FAIL bake"; exit 1; }; sleep 3
send "SET_BAKESHADING $MASK" 10 >/dev/null
for V in $VIEWS; do send "SET_TANGENTVIS $V" 10 >/dev/null; sleep 3; shot "${TAG}_baked_v$V" >/dev/null; done
send "SET_TANGENTVIS 0" 10 >/dev/null; send "SET_BAKESHADING 31" 10 >/dev/null; send "SET_GRASSOFF 0" 10 >/dev/null
python - "$(cygpath -w "$OUT")" "$TAG" $VIEWS <<'EOF'
import sys, os
import numpy as np
from PIL import Image
out, tag, views = sys.argv[1], sys.argv[2], sys.argv[3:]
R = {"near": (220, 600, 1280, 770), "mid": (700, 300, 1280, 590), "far": (220, 130, 1280, 300)}
names = {"9": "albedo in", "23": "albedo out", "13": "occlusion", "17": "sun diffuse", "18": "direct spec", "19": "ambient diffuse", "20": "reflections", "3": "normal", "12": "F0"}
for v in views:
    print("== view %s (%s)" % (v, names.get(v, "?")))
    for rn, (x0, y0, x1, y1) in R.items():
        m = []
        for mode in ("live", "baked"):
            a = np.asarray(Image.open(os.path.join(out, "%s_%s_v%s.png" % (tag, mode, v))).convert("RGB")).astype(float)
            m.append(a[y0:y1, x0:x1].reshape(-1, 3).mean(0))
        r = m[1] / np.maximum(m[0], 1e-3)
        print("   %-4s live %5.1f %5.1f %5.1f | baked %5.1f %5.1f %5.1f | b/l %.2f %.2f %.2f" % (rn, *m[0], *m[1], *r))
EOF
