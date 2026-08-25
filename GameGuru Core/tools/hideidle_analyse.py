import io, os, re, glob, hashlib, sys

OUT = sys.argv[1] if len(sys.argv) > 1 else (
    r"C:/Users/leeba/AppData/Local/Temp/claude/D--max-GameGuruMAXDX12--claude-worktrees-"
    r"determined-chebyshev-bf0892/9a28c586-4c13-4447-916e-7fb51301bfa8/scratchpad/hideidle2")

ROW = re.compile(r"^([ \t]*)(.+?)(?: \((\d+)x\))?: (-?[\d.]+) ms")
HIDDEN = re.compile(r"\[(\d+) idle hidden\]")
SKIP = ("CPU Frame", "GPU Frame", "Main Thread Total",
        "GPU Busy (union of rows)", "GPU Idle + unranged")


def parse(path):
    txt = io.open(path, "r", encoding="utf-8", errors="replace").read()
    i = txt.find("PROFILER_DATA:")
    if i < 0:
        return None
    rows, hidden = [], None
    for line in txt[i:].split("\n"):
        line = line.rstrip("\r")
        if not line.strip():
            continue
        m = HIDDEN.search(line)
        if m:
            hidden = int(m.group(1))
        mm = ROW.match(line)
        if not mm:
            continue
        ind, name, hits, t = mm.groups()
        name = name.strip()
        if name in SKIP:
            continue
        depth = ind.count("  ") if ind.startswith(" ") else (1 if "\t" in ind else 0)
        rows.append((name, depth, float(t)))
    return {"rows": rows, "hidden": hidden,
            "order": [r[0] for r in rows],
            "set": set(r[0] for r in rows),
            "depth": dict((r[0], r[1]) for r in rows),
            "time": dict((r[0], r[2]) for r in rows),
            "hash": hashlib.md5("\n".join("%d|%s" % (r[1], r[0]) for r in rows).encode()).hexdigest()[:10]}


def phase(letter):
    fs = sorted(glob.glob(os.path.join(OUT, letter + "[0-9]*.txt")),
                key=lambda p: int(re.search(r"(\d+)\.txt$", p).group(1)))
    return [(os.path.basename(f), parse(f)) for f in fs if parse(f)]


def report(letter, label):
    ds = phase(letter)
    if not ds:
        print("\n%s: NO DUMPS" % letter)
        return []
    print("\n%s  %s   (%d dumps)" % (letter, label, len(ds)))
    print("   rows         : %s" % " ".join(str(len(d["rows"])) for _, d in ds))
    hs = [d["hash"] for _, d in ds]
    print("   layout hash  : %d distinct%s" % (len(set(hs)), "" if len(set(hs)) == 1 else "  " + " ".join(sorted(set(hs)))))
    print("   [idle hidden]: %s" % " ".join("-" if d["hidden"] is None else str(d["hidden"]) for _, d in ds))
    return ds


print("=" * 78)
print("SOURCE: %s" % OUT)
A = report("A", "hiding OFF, parked")
B = report("B", "hiding ON,  parked")
C = report("C", "hiding ON,  camera flown")
E = report("E", "hiding ON,  parked again (settle)")
D = report("D", "hiding OFF again")

# ---------------------------------------------------------------- re-parent check
print("\n" + "=" * 78)
print("TREE-POSITION CHECK  (3.20 latch: a row must never change depth between dumps)")
allds = A + B + C + E + D
seen = {}
moved = []
for n, d in allds:
    for name, depth in d["depth"].items():
        if name in seen and seen[name][1] != depth:
            moved.append((name, seen[name], (n, depth)))
        seen[name] = (n, depth)
print("   dumps inspected : %d" % len(allds))
print("   rows that moved : %d" % len(moved))
for name, a, b in moved[:10]:
    print("       %-40s depth %d (%s) -> %d (%s)" % (name, a[1], a[0], b[1], b[0]))

# ---------------------------------------------------------------- orphan + safety
if A and B:
    aset, bset = A[-1][1]["set"], B[-1][1]["set"]
    gone = aset - bset
    print("\n" + "=" * 78)
    print("ORPHAN CHECK  (rows that disappear must be EXACTLY the ones reported hidden)")
    print("   rows in A not in B : %d      panel reported: %s      rows in B not in A: %d"
          % (len(gone), B[-1][1]["hidden"], len(bset - aset)))
    print("   -> %s" % ("MATCH - nothing was orphaned" if B[-1][1]["hidden"] == len(gone)
                        else "MISMATCH - a visible row lost its parent"))

    worst = sorted(((max(d["time"].get(k, 0.0) for _, d in A), k) for k in gone), reverse=True)
    bad = [w for w in worst if w[0] > 0.0]
    print("\nSAFETY CHECK  (nothing hidden may ever have been seen above 0.00 in phase A)")
    print("   hidden rows that showed >0.00 in the %d A dumps: %d" % (len(A), len(bad)))
    for mx, k in bad[:8]:
        print("       %-50s %.2f ms" % (k, mx))

    zero_always = set(k for k in aset if max(d["time"].get(k, 0.0) for _, d in A) == 0.0)
    print("\nTHE CONSTRAINT  (rows printing 0.00 that were KEPT anyway)")
    print("   total rows                        : %d" % len(aset))
    print("   printed 0.00 in ALL %2d A dumps    : %d" % (len(A), len(zero_always)))
    print("   of those, HIDDEN (never ran)      : %d" % len(zero_always & gone))
    print("   of those, KEPT  (ran, but tiny)   : %d   <- Lee's constraint, working"
          % len(zero_always - gone))
    print("   hidden rows: %s" % sorted(gone))

# ---------------------------------------------------------------- movement / settle
if B and C:
    print("\n" + "=" * 78)
    print("UNDER MOVEMENT then SETTLE (a row may hide ONCE; after that it must hold)")
    prev = B[-1][1]["set"]
    for n, d in C + E:
        add, lost = d["set"] - prev, prev - d["set"]
        print("   %-8s rows=%-4d +%-2d -%-2d %s%s" % (n, len(d["rows"]), len(add), len(lost),
              ("gained: " + ", ".join(sorted(add)[:3])) if add else "",
              ("   lost: " + ", ".join(sorted(lost)[:3])) if lost else ""))
        prev = d["set"]
if E:
    hs = set(d["hash"] for _, d in E)
    print("   settle verdict: %d distinct layouts over the last %d parked dumps -> %s"
          % (len(hs), len(E), "SETTLED" if len(hs) == 1 else "STILL MOVING"))

# ---------------------------------------------------------------- restore
if A and D:
    print("\n" + "=" * 78)
    print("RESTORE CHECK (unticking brings the full list back)")
    aset, dset = A[-1][1]["set"], D[-1][1]["set"]
    print("   A rows=%d  D rows=%d   in A not D=%d   in D not A=%d"
          % (len(aset), len(dset), len(aset - dset), len(dset - aset)))
    if aset - dset:
        print("   MISSING after untick: %s" % sorted(aset - dset)[:8])
    print("   D layout hashes distinct: %d" % len(set(d["hash"] for _, d in D)))
