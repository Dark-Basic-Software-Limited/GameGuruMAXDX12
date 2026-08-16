#!/usr/bin/env python3
"""GGMAX 2.74 (task #155, Lee-directed 2026-08-16): terrain roughness dry-look floor.

DX12 restored stock PBR energy (DX11 carried two deliberate env-reflection cuts), which
exposed that several stock terrain surface maps are authored GLOSSY (beach sand mat2
roughness mean 41/255 = wet look). Lee's decision: keep DX12's true energy, lift the
CONTENT — raise every terrain Surface.dds whose roughness (G channel) mean is below
mat4's dry reference (175/255 = 0.686) up to exactly that mean. Mats already at/above
the floor are untouched. Originals are mirrored (no-clobber) to D:\\max\\mipbackup and
double as the "maximum energy" custom set for users who want the shiny look back.

Method: additive lift G' = clamp(G + (TARGET - mean), 0, 255) — preserves the authored
variation amplitude exactly (a multiplicative scale on a x4-5 lift would clamp-distort
the distribution). R (AO), B (metalness) and A (reflectance) are passed through; the
DXT1 re-encode re-quantizes them slightly (unavoidable; originals mirrored).

texconv landmines honored (see ddsconvert.py): explicit -m (never -m 0 on partial
chains — not applicable here, sources are full-chain), -dx9 legacy header preserved
(DXT1 sources), BC chains stop at the 4x4 block floor => accept full-2 mips (2048 ->
10..12 mips all OK).

Usage: python terrainroughness.py [--apply]   (default = audit/dry-run only)
"""
import struct, glob, os, sys, subprocess, tempfile, shutil, json

ROOT    = r"D:\DEV\BUILD\GameGuru Wicked MAX Build Area\Max\Files\terraintextures"
MIRROR  = r"D:\max\mipbackup\terraintextures_buildarea"
TEXCONV = r"D:\max\tools\texconv.exe"
TARGET  = 175.0          # mat4's measured roughness mean (0.686) = the dry-look floor
TOL     = 3.0            # post-verify tolerance on the new mean
SETS    = ["mat*", os.path.join("extras", "lowpoly", "mat*")]

def dds_header(p):
    with open(p, 'rb') as f: h = f.read(148)
    if h[:4] != b'DDS ': return None
    w = struct.unpack_from('<I', h, 16)[0]; hh = struct.unpack_from('<I', h, 12)[0]
    mips = max(1, struct.unpack_from('<I', h, 28)[0]); fourcc = h[84:88]
    return w, hh, mips, fourcc

def full_mips(w, h):
    n, m = 1, max(w, h)
    while m > 1: m >>= 1; n += 1
    return n

def channel_stats(png_path):
    from PIL import Image
    im = Image.open(png_path).convert("RGBA")
    w, h = im.size; n = w * h
    data = im.tobytes()
    sums = [0, 0, 0, 0]
    for i in range(4):
        sums[i] = sum(data[i::4])
    means = [s / n for s in sums]
    return im, means

def process(path, apply_it, results):
    rel = os.path.relpath(path, ROOT)
    hdr = dds_header(path)
    if hdr is None:
        results.append({"file": rel, "status": "NOT-DDS"}); return
    w, h, mips, fourcc = hdr
    with tempfile.TemporaryDirectory() as td:
        r = subprocess.run([TEXCONV, "-f", "R8G8B8A8_UNORM", "-m", "1", "-ft", "PNG",
                            "-y", "-o", td, path], capture_output=True, text=True)
        png = os.path.join(td, os.path.splitext(os.path.basename(path))[0] + ".png")
        if not os.path.exists(png):
            results.append({"file": rel, "status": "DECODE-FAIL"}); return
        im, means = channel_stats(png)
        gmean = means[1]
        if gmean >= TARGET - 1.0:   # epsilon: don't re-encode mats already at the floor (mat4 itself)
            results.append({"file": rel, "status": "SKIP-already-dry", "gmean": round(gmean, 1)}); return
        lift = TARGET - gmean
        entry = {"file": rel, "status": "AUDIT-would-lift", "gmean": round(gmean, 1),
                 "lift": round(lift, 1), "fmt": fourcc.decode(errors='replace'), "mips": mips}
        if not apply_it:
            results.append(entry); return

        # mirror original, no-clobber
        mpath = os.path.join(MIRROR, rel)
        os.makedirs(os.path.dirname(mpath), exist_ok=True)
        if not os.path.exists(mpath):
            shutil.copy2(path, mpath)

        # lift G channel
        rch, gch, bch, ach = im.split()
        gch = gch.point(lambda v: min(255, max(0, int(round(v + lift)))))
        from PIL import Image
        Image.merge("RGBA", (rch, gch, bch, ach)).save(png)

        # re-encode: DXT1 == BC1_UNORM, legacy -dx9 header, explicit full mip request
        want = full_mips(w, h)
        r2 = subprocess.run([TEXCONV, "-f", "BC1_UNORM", "-m", str(want), "-dx9",
                             "-y", "-o", td, "-sx", "_enc", png], capture_output=True, text=True)
        enc = os.path.join(td, os.path.splitext(os.path.basename(png))[0] + "_enc.dds")
        if not os.path.exists(enc):
            entry.update({"status": "ENCODE-FAIL", "err": (r2.stdout + r2.stderr)[-300:]})
            results.append(entry); return

        # verify before install: dims/format preserved, mips >= full-2, new G mean on target
        h2 = dds_header(enc)
        ok = h2 and h2[0] == w and h2[1] == h and h2[3] == b'DXT1' and h2[2] >= want - 2
        if ok:
            r3 = subprocess.run([TEXCONV, "-f", "R8G8B8A8_UNORM", "-m", "1", "-ft", "PNG",
                                 "-y", "-o", td, "-sx", "_chk", enc], capture_output=True, text=True)
            chk = os.path.join(td, os.path.splitext(os.path.basename(enc))[0] + "_chk.png")
            if os.path.exists(chk):
                _, m2 = channel_stats(chk)
                ok = abs(m2[1] - TARGET) <= TOL and abs(m2[0] - means[0]) <= 3.0
                entry["new_means"] = [round(x, 1) for x in m2]
            else:
                ok = False
        if not ok:
            entry.update({"status": "VERIFY-FAIL (original untouched)"})
            results.append(entry); return

        shutil.copy2(enc, path)
        entry.update({"status": "LIFTED", "new_mips": h2[2]})
        results.append(entry)

def main():
    apply_it = "--apply" in sys.argv
    results = []
    files = []
    for pat in SETS:
        files += sorted(glob.glob(os.path.join(ROOT, pat, "Surface.dds")))
    for p in files:
        process(p, apply_it, results)
        print(results[-1])
    rep = os.path.join(os.path.dirname(os.path.abspath(__file__)), "terrainroughness_report.txt")
    with open(rep, "w") as f:
        f.write(f"terrain roughness dry-look floor — target G mean {TARGET} (mat4), mode={'APPLY' if apply_it else 'AUDIT'}\n")
        for e in results:
            f.write(json.dumps(e) + "\n")
    lifted = sum(1 for e in results if e["status"] == "LIFTED")
    would = sum(1 for e in results if e["status"] == "AUDIT-would-lift")
    skip = sum(1 for e in results if e["status"].startswith("SKIP"))
    bad = [e for e in results if "FAIL" in e["status"]]
    print(f"\nTOTAL {len(results)}: lifted={lifted} would-lift={would} already-dry={skip} failures={len(bad)}")
    for e in bad: print("  FAIL:", e)

main()
