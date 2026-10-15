# GGMAX 3.115: companion to bake_sharpness_probe.sh - baked vs live ground SHARPNESS from screenshots.
# usage: python bake_sharpness_measure.py <outdir> <tag> <mask...>
# HF = mean absolute 3x3 Laplacian of luma inside a band (texture detail; blur lowers it).
# Bands are rows of the 3D view (1536x801 shot, view x 212..1285, y 105..777), split by height:
#   near = bottom 30% of the view, mid = the 30% above it, far = the strip just under the horizon.
# The player-start gizmo sits low in the middle of the default view, so x 480..700 is excluded.
import sys, os
import numpy as np
from PIL import Image

LUM = np.array([0.299, 0.587, 0.114])
VX0, VX1, VY0, VY1 = 212, 1285, 105, 777

def bands(cam):
    h = VY1 - VY0
    near = (VY0 + int(h * 0.70), VY1 - 4)
    mid = (VY0 + int(h * 0.40), VY0 + int(h * 0.70))
    far = (VY0 + int(h * 0.18), VY0 + int(h * 0.40)) if cam != 2 else (VY0 + int(h * 0.05), VY0 + int(h * 0.40))
    return {"near": near, "mid": mid, "far": far}

def measure(fn, y0, y1):
    a = np.asarray(Image.open(fn).convert("RGB")).astype(float)
    r = a[y0:y1, VX0:VX1]
    keep = np.ones(r.shape[1], bool)
    keep[480 - VX0:700 - VX0] = False
    r = r[:, keep]
    lum = r @ LUM
    lap = np.abs(4 * lum[1:-1, 1:-1] - lum[:-2, 1:-1] - lum[2:, 1:-1] - lum[1:-1, :-2] - lum[1:-1, 2:])
    return lap.mean(), r.reshape(-1, 3).mean(0)

outdir, tag, masks = sys.argv[1], sys.argv[2], sys.argv[3:]
for cam in range(3):
    live = os.path.join(outdir, "%s_c%d_live.png" % (tag, cam))
    if not os.path.exists(live):
        print("missing", live); continue
    print("== camera %d" % cam)
    for bn, (y0, y1) in bands(cam).items():
        hl, cl = measure(live, y0, y1)
        row = "   %-4s live HF %5.2f rgb %5.1f %5.1f %5.1f |" % (bn, hl, cl[0], cl[1], cl[2])
        for m in masks:
            baked = os.path.join(outdir, "%s_c%d_baked_m%s.png" % (tag, cam, m))
            if not os.path.exists(baked):
                row += " m%s missing |" % m; continue
            hb, cb = measure(baked, y0, y1)
            lr = (cb @ LUM) / max(cl @ LUM, 1e-3)
            row += " m%s HF %5.2f (%.2f) lum %.2f |" % (m, hb, hb / max(hl, 1e-3), lr)
        print(row)
