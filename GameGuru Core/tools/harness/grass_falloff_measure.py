# GGMAX 3.117: companion to grass_falloff_probe.sh.
# usage: python grass_falloff_measure.py <outdir> <tag> <lod_dist> [<camera_height_above_ground> <pitch_deg>]
# A pixel is GRASS when it differs from the Grass Off shot; only pixels that are GROUND in the Grass Off shot
# count (water and sky drop out). Each pixel's camera->ground distance comes from <tag>_grid.txt, the
# harness GET_GROUNDDIST_GRID ray-march against the terrain (interpolated in 1/distance between cell
# centres). Without a grid it falls back to a FLAT-ground row mapping from the camera height and pitch,
# which is only valid on flat ground - on a hilly level it read grass past the cull radius (3.117 gf3).
# 3D panel y 105..777, x 212..1285 of the screenshot; the projection (vertical FOV 60) spans the WHOLE window.
import sys, os, math
import numpy as np
from PIL import Image

outdir, tag, L = sys.argv[1], sys.argv[2], float(sys.argv[3])
R = L + 2500.0
VX0, VX1, VY0, VY1 = 212, 1285, 105, 777
W, Hpx = VX1 - VX0, VY1 - VY0

IMG_W, IMG_H = Image.open(os.path.join(outdir, "%s_off.png" % tag)).size   # the client window (1536 x 801)
def load(n):
    return np.asarray(Image.open(os.path.join(outdir, "%s_%s.png" % (tag, n))).convert("RGB")).astype(float)[VY0:VY1, VX0:VX1]

off = load("off")
shots = {m: load("m%d" % m) for m in (2, 1, 0)}
ground = (off[..., 0] > off[..., 2] + 6) & (off.sum(-1) > 90)          # brownish ground, not water/sky

def distance_map():
    gp = os.path.join(outdir, "%s_grid.txt" % tag)
    if os.path.exists(gp):
        lines = [l for l in open(gp).read().splitlines() if l.strip()]
        hdr = lines[0].split()
        cols, rows = int(hdr[2]), int(hdr[3])
        g = np.array([[float(v) for v in l.split()] for l in lines[1:1 + rows]])
        assert g.shape == (rows, cols), g.shape
        inv = np.where(g > 0, 1.0 / np.maximum(g, 1e-3), np.nan)
        # the scene is projected over the WHOLE client window (the 3D panel is a crop of it - checked
        # against PICK_AT terrain hits, 3.117), so a screenshot pixel's NDC comes from the full image size
        gx = np.clip((VX0 + np.arange(W) + 0.5) / IMG_W * cols - 0.5, 0, cols - 1)
        gy = np.clip((VY0 + np.arange(Hpx) + 0.5) / IMG_H * rows - 0.5, 0, rows - 1)
        x0 = np.minimum(np.floor(gx).astype(int), cols - 2); fx = gx - x0
        y0 = np.minimum(np.floor(gy).astype(int), rows - 2); fy = gy - y0
        a = inv[y0][:, x0]; b = inv[y0][:, x0 + 1]; c = inv[y0 + 1][:, x0]; d = inv[y0 + 1][:, x0 + 1]
        fx = fx[None, :]; fy = fy[:, None]
        iv = (a * (1 - fx) + b * fx) * (1 - fy) + (c * (1 - fx) + d * fx) * fy
        print("distance source: %s (%dx%d ray-march grid)" % (os.path.basename(gp), cols, rows))
        return 1.0 / iv
    H, pitch = float(sys.argv[4]), float(sys.argv[5])
    print("distance source: FLAT-ground row mapping (no grid) - camera height %.0f pitch %.1f" % (H, pitch))
    tan_half = math.tan(math.radians(30.0))
    dm = np.full((Hpx, W), np.nan)
    for y in range(Hpx):
        a = math.radians(pitch) + math.atan((VY0 + y + 0.5 - IMG_H / 2.0) / (IMG_H / 2.0) * tan_half)
        if a > math.radians(0.3): dm[y, :] = H / math.sin(a)
    return dm

def model_survival(d):
    # DX11: drop when d > L + u*(R-L), u ~ Irwin-Hall(3)/3  ->  survival = P(u > t)
    t = (d - L) / (R - L)
    if t <= 0: return 1.0
    if t >= 1: return 0.0
    x = 3.0 * t   # Irwin-Hall n=3 CDF at x
    if x < 1: cdf = x ** 3 / 6.0
    elif x < 2: cdf = (-2 * x ** 3 + 9 * x ** 2 - 9 * x + 3) / 6.0
    else: cdf = 1 - (3 - x) ** 3 / 6.0
    return 1.0 - cdf

dist = distance_map()
grass = {m: (np.abs(s - off).sum(-1) > 40) for m, s in shots.items()}
# bands relative to L and R: three inside the full-density zone, six across the dissolve, two past the cull
edges = [L * 0.55, L * 0.7, L * 0.85, L] + [L + (R - L) * k / 6.0 for k in range(1, 7)] + [R * 1.1, R * 1.25]
bands = [(edges[i], edges[i + 1]) for i in range(len(edges) - 1)]
print("L=%.0f R=%.0f" % (L, R))
print("  band (units)    pixels |  cov m2   cov m1   cov m0 | surv m1  surv m0 | DX11 model")
for lo, hi in bands:
    sel = ground & (dist >= lo) & (dist < hi)
    n = int(sel.sum())
    if n < 200: continue
    cov = {m: (g & sel).sum() / n for m, g in grass.items()}
    def surv(m):
        c, b = min(cov[m], 0.999), min(cov[2], 0.999)
        return (-math.log(1 - c)) / max(-math.log(1 - b), 1e-6)
    mid = 0.5 * (lo + hi)
    print("  %5d-%-5d  %7d | %6.3f   %6.3f   %6.3f |  %5.2f    %5.2f  |  %5.2f" % (lo, hi, n, cov[2], cov[1], cov[0], surv(1), surv(0), model_survival(mid)))
    # the dissolve only REMOVES strands, so mode 1 above the no-dissolve baseline means a shot caught a
    # stale state (3.117 gfpre: the baseline was shot before the draw-distance rebuild landed)
    if cov[1] > cov[2] * 1.15 + 0.01:
        print("    ^ INVALID: mode 1 above the mode 2 baseline - a shot caught a stale or rebuilding state")
