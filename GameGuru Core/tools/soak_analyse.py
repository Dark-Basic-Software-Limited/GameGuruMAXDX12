#!/usr/bin/env python3
# Analyse the single-session soak sweep against the 0826b fresh-launch reference.
#   - C1 LOAD / C4 GAME pass counts (the two Lee asked for)
#   - blank-frame check from shot byte size
#   - POLYS vs the reference (geometry must not change)
#   - the cumulative question: does VRAM climb with LOAD ORDER, or only with level content?
import re, sys, os

# 2026-09-23: OUT was hardcoded to tools/soak0919, a directory that no longer exists, and the
# reference file was a hard open() - so this refused to run on any later soak. Both are now
# optional: pass the run directory as argv[1], and a missing reference simply drops the POLYS
# comparison (sweepgate.sh scores POLYS for a soak run anyway, advisory, against the curated
# table it already owns - one reference, one place).
#   usage: soak_analyse.py [run_dir] [reference_file]
OUT = sys.argv[1] if len(sys.argv) > 1 else os.path.dirname(os.path.abspath(__file__)) + "/soak0919"
OUT = OUT.rstrip("/\\")
REFPATH = sys.argv[2] if len(sys.argv) > 2 else OUT + "/reference_0826b.txt"
REF = {}
if os.path.exists(REFPATH):
    for ln in open(REFPATH):
        m = re.match(r"(.+?)\s+edVRAM=([\d.]+)\s+gmVRAM=([\d.]+)\s+POLYS:\s*(\d+)", ln.strip())
        if m:
            REF[m.group(1).strip()] = (float(m.group(2)), float(m.group(3)), int(m.group(4)))
else:
    print("(no reference file at %s - POLYS and dVRAM columns are skipped)" % REFPATH)

rows, restarts = [], []
for ln in open(OUT + "/results.txt"):
    ln = ln.strip()
    if ln.startswith("#"):
        continue
    if ln.startswith("---"):
        restarts.append(ln)
        continue
    f = ln.split("|")
    if len(f) < 5:
        continue
    d = {"demo": f[0], "state": f[1]}
    for p in f[2:]:
        if "=" in p:
            k, v = p.split("=", 1)
            d[k] = v
    rows.append(d)

def num(x, default=0.0):
    try:
        return float(re.sub(r"[^\d.]", "", str(x)) or default)
    except Exception:
        return default

# Blankness is measured from the IMAGE, not the file size. Byte size conflates "blank" with
# "dark": Escape from the Zombie Cellar's editor frame is a fully rendered windowless interior
# that compresses to 977 KB and would fail a 1 MB byte threshold. Luminance std is the property
# that actually separates them - a blank frame has near-zero variance whatever its size.
BLANK_STD = 3.0
SHOTS = OUT + "/shots"
def frame_std(name):
    try:
        from PIL import Image
        import numpy as np
        a = np.asarray(Image.open(SHOTS + "/" + name + ".png").convert("L"), dtype=float)
        return float(a.std())
    except Exception:
        return -1.0

print("=" * 116)
print("SINGLE-SESSION SOAK SWEEP - 19 demos, ONE MAX launch, each taken into Test Game")
print("=" * 116)
hdr = "%-31s %-6s %5s %7s %7s %7s %7s %9s %9s %8s"
print(hdr % ("demo", "game", "load", "edFPS", "gmFPS", "edVRAM", "gmVRAM", "ed shot", "gm shot", "dVRAM"))
print("-" * 116)

load_ok = game_ok = blank = 0
vr, order, polydiff = [], [], []
for i, r in enumerate(rows):
    ev, gv = num(r.get("edVRAM")), num(r.get("gmVRAM"))
    es, gs = int(num(r.get("edshot"))), int(num(r.get("gmshot")))
    ref = REF.get(r["demo"])
    dv = ev - ref[0] if ref else 0.0
    reached = r["state"].startswith("GAME")
    if "FAIL_EDITOR" not in r["state"] and ev > 0:
        load_ok += 1
    if reached:
        game_ok += 1
        vr.append(ev); order.append(float(i))
    esd, gsd = frame_std("ed_%d" % i), frame_std("gm_%d" % i)
    if (0 <= esd < BLANK_STD) or (0 <= gsd < BLANK_STD):
        blank += 1
    if ref and r.get("polys"):
        p = int(num(r["polys"]))
        if p and abs(p - ref[2]) > 0:
            polydiff.append((r["demo"], p, ref[2]))
    flag = ""
    if 0 <= esd < BLANK_STD: flag += " !EDBLANK"
    if 0 <= gsd < BLANK_STD: flag += " !GMBLANK"
    if "NOEXIT" in r["state"]: flag += " !NOEXIT"
    print(hdr % (r["demo"][:31], "yes" if reached else "NO", r.get("load", "?"),
                 r.get("edFPS", "-"), r.get("gmFPS", "-"), "%.1f" % ev, "%.1f" % gv,
                 "{:,}".format(es), "{:,}".format(gs), "%+.0f" % dv) + flag)

print("-" * 116)
print("C1 LOAD  reached the editor : %d/%d" % (load_ok, len(rows)))
print("C4 GAME  reached Test Game  : %d/%d" % (game_ok, len(rows)))
print("blank frames (luminance std < %.1f, not byte size) : %d" % (BLANK_STD, blank))
print("session restarts            : %d %s" % (len(restarts), restarts if restarts else ""))

if polydiff:
    print("\nPOLYS differs from the 0826b reference (geometry should be identical):")
    for d, p, q in polydiff:
        print("   %-31s soak %-10d ref %-10d  %+d" % (d, p, q, p - q))
else:
    print("POLYS vs reference          : %s" % ("identical on every demo compared" if REF else "no reference loaded - see sweepgate.sh"))

# --- the cumulative question -------------------------------------------------
# Regress editor VRAM on LOAD ORDER. Level content is the dominant term and is not ordered,
# so a real per-load accumulation shows as a positive slope that survives that noise.
if len(vr) >= 6:
    n = len(vr)
    mx, my = sum(order) / n, sum(vr) / n
    sxy = sum((order[i] - mx) * (vr[i] - my) for i in range(n))
    sxx = sum((order[i] - mx) ** 2 for i in range(n))
    syy = sum((vr[i] - my) ** 2 for i in range(n))
    slope = sxy / sxx if sxx else 0.0
    r2 = (sxy * sxy) / (sxx * syy) if sxx and syy else 0.0
    print("\nCUMULATIVE VRAM across load order")
    print("   first load %.1f MB -> last load %.1f MB" % (vr[0], vr[-1]))
    print("   slope %+.1f MB per level loaded (r2 = %.2f over %d loads)" % (slope, r2, n))
    print("   peak editor VRAM %.1f MB on '%s'" % (max(vr), rows[vr.index(max(vr))]["demo"]))
    over = [(rows[i]["demo"], v) for i, v in enumerate(vr) if v > 4096]
    print("   loads above the 4096 MB min-spec floor: %d of %d" % (len(over), n))
