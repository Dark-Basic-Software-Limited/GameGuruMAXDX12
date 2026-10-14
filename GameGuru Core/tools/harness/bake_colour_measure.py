# GGMAX 3.113: companion to bake_colour_probe.sh - baked vs live ground colour from screenshot pairs.
# usage: python bake_colour_measure.py <outdir> <tag> <mask...>
# Regions are in 1536x801 screenshot pixels inside the 3D view, clear of the player-start gizmo.
# sd = mean per-channel standard deviation inside the region = local contrast.
import sys, os
import numpy as np
from PIL import Image

REGIONS = {"near": (220, 600, 1280, 770), "mid": (700, 300, 1280, 590), "far": (220, 130, 1280, 300)}
LUM = np.array([0.299, 0.587, 0.114])

def stats(fn, box):
    a = np.asarray(Image.open(fn).convert("RGB")).astype(float)
    x0, y0, x1, y1 = box
    r = a[y0:y1, x0:x1].reshape(-1, 3)
    return r.mean(0), r.std(0).mean()

def line(label, ml, sl, mb, sb):
    ratio = mb / np.maximum(ml, 1e-3)
    return ("   %-4s live %5.1f %5.1f %5.1f lum %5.1f sd %4.1f | baked %5.1f %5.1f %5.1f lum %5.1f sd %4.1f | b/l %.2f %.2f %.2f lum %.2f"
            % (label, ml[0], ml[1], ml[2], ml @ LUM, sl, mb[0], mb[1], mb[2], mb @ LUM, sb, ratio[0], ratio[1], ratio[2], (mb @ LUM) / max(ml @ LUM, 1e-3)))

outdir, tag, masks = sys.argv[1], sys.argv[2], sys.argv[3:]
for ao in ("", "_aooff"):
    live = os.path.join(outdir, "%s_live%s.png" % (tag, ao))
    if not os.path.exists(live):
        print("missing", live); continue
    for m in masks:
        baked = os.path.join(outdir, "%s_baked_m%s%s.png" % (tag, m, ao))
        if not os.path.exists(baked):
            print("missing", baked); continue
        print("== mask %s %s" % (m, "AO OFF" if ao else "AO on"))
        for rn, box in REGIONS.items():
            ml, sl = stats(live, box)
            mb, sb = stats(baked, box)
            print(line(rn, ml, sl, mb, sb))
