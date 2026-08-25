import io, os, re, glob, hashlib

OUT = (r"C:/Users/leeba/AppData/Local/Temp/claude/D--max-GameGuruMAXDX12--claude-worktrees-"
       r"determined-chebyshev-bf0892/9a28c586-4c13-4447-916e-7fb51301bfa8/scratchpad/settle")

ROW = re.compile(r"^([ \t]*)(.+?)(?: \((\d+)x\))?: (-?[\d.]+) ms")
HID = re.compile(r"\[(\d+) idle hidden\]")
SKIP = ("CPU Frame", "GPU Frame", "Main Thread Total",
        "GPU Busy (union of rows)", "GPU Idle + unranged")


def parse(p):
    t = io.open(p, encoding="utf-8", errors="replace").read()
    i = t.find("PROFILER_DATA:")
    if i < 0:
        return None
    rows, hid = [], None
    for l in t[i:].split("\n"):
        l = l.rstrip("\r")
        if not l.strip():
            continue
        m = HID.search(l)
        if m:
            hid = int(m.group(1))
        mm = ROW.match(l)
        if not mm:
            continue
        ind, name, _, v = mm.groups()
        name = name.strip()
        if name in SKIP:
            continue
        d = ind.count("  ") if ind.startswith(" ") else (1 if "\t" in ind else 0)
        rows.append((name, d))
    return {"n": len(rows), "hidden": hid, "set": set(r[0] for r in rows),
            "hash": hashlib.md5("\n".join("%d|%s" % (r[1], r[0]) for r in rows).encode()).hexdigest()[:8]}


def ph(L):
    fs = sorted(glob.glob(os.path.join(OUT, L + "[0-9]*.txt")),
                key=lambda p: int(re.search(r"(\d+)\.txt$", p).group(1)))
    return [(os.path.basename(f), parse(f)) for f in fs if parse(f)]


def curve(L, label, secs_per=2.0):
    ds = ph(L)
    if not ds:
        print("%s: none" % L)
        return
    print("\n%s  (%d dumps, %.0fs apart)" % (label, len(ds), secs_per))
    prev = None
    last_change = 0.0
    changes = []
    for k, (n, d) in enumerate(ds):
        t = k * secs_per
        moved = prev is not None and d["hash"] != prev["hash"]
        if moved:
            last_change = t
            changes.append((t, len(d["set"] - prev["set"]), len(prev["set"] - d["set"])))
        prev = d
    print("   rows: %s" % " ".join(str(d["n"]) for _, d in ds[:30]))
    if len(ds) > 30:
        print("         %s" % " ".join(str(d["n"]) for _, d in ds[30:]))
    print("   hidden: first=%s  last=%s   distinct layouts=%d"
          % (ds[0][1]["hidden"], ds[-1][1]["hidden"], len(set(d["hash"] for _, d in ds))))
    print("   layout changed %d times; LAST CHANGE at t=%.0fs; held still for the final %.0fs"
          % (len(changes), last_change, (len(ds) - 1) * secs_per - last_change))
    for t, a, r in changes[:14]:
        print("       t=%3.0fs   +%-2d -%-2d" % (t, a, r))
    if len(changes) > 14:
        print("       ... %d more" % (len(changes) - 14))


print("=" * 74)
curve("A", "BEFORE ticking (box off)")
curve("S", "AFTER TICKING at 0.05 ms - the settle")
curve("R", "AFTER FLYING - the re-settle")
pk = os.path.join(OUT, "peaks.txt")
if os.path.exists(pk):
    print("\n" + "=" * 74)
    print(io.open(pk, encoding="utf-8", errors="replace").read()[:430])
