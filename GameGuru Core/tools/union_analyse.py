#!/usr/bin/env python3
# Analyse the 19+1 demo VRAM union sweep.
#
# The question is not "did the total go down" - the demos differ in size, so the total moves with
# content. It is: (a) does a category GROW MONOTONICALLY with load count, which is the signature of
# accumulation, and (b) does demo 1 reloaded at position 20 cost what it cost at position 1, which
# holds content constant across the whole session and is the only clean per-session leak test.
import re, sys, os, glob

OUT = sys.argv[1] if len(sys.argv) > 1 else (
    os.path.dirname(os.path.abspath(__file__)) + "/vram_union")

def load(p):
    recs = []
    for ln in open(p, encoding="utf-8", errors="replace"):
        if ln.startswith(("CENSUS", "#")):
            continue
        m = re.match(r'(\w) (\d+) .* "(.*)"\s*$', ln.strip())
        if m:
            recs.append((int(m.group(2)), m.group(3)))
    return recs

def cat(n):
    nl = n.lower().replace("\\", "/")
    if "terraintextures/" in nl: return "TERRAIN material DDS"
    if "/files/" in nl:          return "other content textures"
    if not n:                    return "(unnamed)"
    return "engine: " + n.split("::")[0][:26]

tags = []
for f in sorted(glob.glob(OUT + "/d*.txt")):
    tags.append(os.path.basename(f)[:-4])
if not tags:
    print("no dumps in", OUT); raise SystemExit(1)
data = {t: load(OUT + "/" + t + ".txt") for t in tags}

tot = {t: sum(b for b, _ in data[t]) / 1048576 for t in tags}
rec = {t: len(data[t]) for t in tags}

print("=" * 100)
print("VRAM UNION SWEEP - %d dumps from %s" % (len(tags), OUT))
print("=" * 100)
print("%-14s %10s %9s" % ("dump", "census MB", "records"))
for t in tags:
    print("%-14s %10.1f %9d" % (t, tot[t], rec[t]))

# --- per-category trajectory, ordered by how much they grow over the run -------------
cats = sorted({cat(n) for t in tags for _, n in data[t]})
series = {}
for c in cats:
    series[c] = [sum(b for b, n in data[t] if cat(n) == c) / 1048576 for t in tags]

# monotonic growth score: how much of the run is spent non-decreasing, and total drift
loads = [t for t in tags if t != "d00_boot"]
def drift(c):
    v = [series[c][tags.index(t)] for t in loads]
    return v[-1] - v[0]
print()
print("CATEGORY TRAJECTORY (first load -> last load, and the max seen)")
print("%-30s %10s %10s %10s %10s" % ("category", "first", "last", "max", "drift"))
for c in sorted(cats, key=lambda x: -abs(drift(x))):
    v = [series[c][tags.index(t)] for t in loads]
    if max(v) < 1.0: continue
    print("%-30s %10.1f %10.1f %10.1f %+10.1f" % (c[:30], v[0], v[-1], max(v), v[-1] - v[0]))

# --- the decisive comparison -----------------------------------------------------------
if "d01" in data and "d20_repeat" in data:
    print()
    print("=" * 100)
    print("DECISIVE: demo 1 at load 1  vs  demo 1 at load 20 (identical level, 19 loads apart)")
    print("=" * 100)
    a, b = "d01", "d20_repeat"
    print("  census   %10.1f MB  ->  %10.1f MB   %+.1f MB" % (tot[a], tot[b], tot[b] - tot[a]))
    print("  records  %10d     ->  %10d      %+d" % (rec[a], rec[b], rec[b] - rec[a]))
    print()
    print("  %-30s %10s %10s %10s" % ("category", "load 1", "load 20", "retained"))
    rows = []
    for c in cats:
        x = series[c][tags.index(a)]; y = series[c][tags.index(b)]
        if abs(y - x) > 0.4: rows.append((y - x, c, x, y))
    for d, c, x, y in sorted(rows, key=lambda r: -r[0]):
        print("  %-30s %10.1f %10.1f %+10.1f" % (c[:30], x, y, d))
    print("  %-30s %10s %10s %+10.1f" % ("TOTAL RETAINED", "", "", sum(r[0] for r in rows)))
    print()
    print("  Pre-fix reference, same comparison on two levels: +186.1 MB retained")
    print("  (terrain 142.4 / gpup imageTex 20.4 / content 14.5 / gpup renderTex 7.5)")
