#!/usr/bin/env python3
# GGMAX 3.110: measure a particle_preview_probe.sh run.
# usage: particle_preview_measure.py <outdir> <tag> x0 y0 x1 y1 [nx0 ny0 nx1 ny1]
# PASS needs: every consecutive hovered shot differs inside the tile (it animates), the tile
# differs from the un-hovered baseline (the live render replaced the thumbnail), the neighbour
# tile does not change (only the hovered cell is live), and the un-hover shot is back to baseline.
import os, sys
import numpy as np
from PIL import Image

out, tag = sys.argv[1], sys.argv[2]
r = [int(v) for v in sys.argv[3:7]]
n = [int(v) for v in sys.argv[7:11]] if len(sys.argv) >= 11 else [0, 0, 0, 0]

def load(name):
    return np.asarray(Image.open(os.path.join(out, f"{tag}_{name}.png")).convert("RGB")).astype(np.float32)

def crop(a, b):
    return a[b[1]:b[3], b[0]:b[2]]

def mad(a, b):
    return float(np.mean(np.abs(a - b)))

base = load("base")
hov = [load(f"h{i}") for i in range(1, 6)]
unh = load("unhover")
t = [crop(h, r) for h in hov]
tb = crop(base, r)

pair = [mad(t[i], t[i + 1]) for i in range(4)]
vsbase = [mad(x, tb) for x in t]
lum = [float(np.mean(x)) for x in t]
lit = [float(np.mean(np.max(x, axis=2) > 40)) for x in t]   # fraction of tile pixels with something drawn
print("tile pair diffs   :", " ".join(f"{v:6.2f}" for v in pair))
print("tile vs baseline  :", " ".join(f"{v:6.2f}" for v in vsbase))
print("tile mean lum     :", " ".join(f"{v:6.1f}" for v in lum))
print("tile lit fraction :", " ".join(f"{v:6.3f}" for v in lit))
unbase = mad(crop(unh, r), tb)
print(f"unhover vs base   : {unbase:6.2f}")
ok = all(v > 0.5 for v in pair) and all(v > 2.0 for v in vsbase) and unbase < 1.0
if n[2] > n[0]:
    nb = crop(base, n)
    nd = [mad(crop(h, n), nb) for h in hov]
    print("neighbour vs base :", " ".join(f"{v:6.2f}" for v in nd))
    ok = ok and all(v < 0.5 for v in nd)
print("RESULT:", "PASS" if ok else "FAIL")
