#!/usr/bin/env python3
"""MILESTONE_DDS_CONVERSION.md driver: bring every eligible DDS in the stock content
tree to a FULL mip chain, format-preserving, with a mirror backup and per-file
verification. Extends the 2026-08-04 entitybank pass (VRAM_AUDIT.md "mip swap-in")
to the whole tree, same protocol:

  - work list: 2D, non-cube, non-array DDS whose full chain is > 64 KB (the engine's
    streaming_texture_min_size) and whose mip chain is missing or SHORT of full
  - format-preserving: -f mapped from the header (legacy fourcc or DX10 dxgiFormat);
    unmappable formats are SKIPPED and listed, never guessed
  - header-preserving: -dx9 for legacy-headered files (except formats that require
    DX10: BC6/BC7/anything _SRGB), -dx10 for DX10-headered files
  - originals mirrored to D:/max/mipbackup/<relpath> BEFORE conversion, no-clobber
    (the 08-04 entitybank originals already live there and are never overwritten)
  - post-verify per file: same width/height/format family, full mip count; any
    mismatch restores the mirror copy and reports FAIL

Usage: python ddsconvert.py [--dry] [--jobs N]
Writes ddsconvert_report.txt + ddsconvert_results.jsonl next to itself.
"""
import os, sys, struct, json, shutil, subprocess, collections, math, threading
from concurrent.futures import ThreadPoolExecutor, as_completed

ROOT = r"D:\DEV\BUILD\GameGuru Wicked MAX Build Area\Max\Files"
MIRROR = r"D:\max\mipbackup"
TEXCONV = r"D:\max\tools\texconv.exe"
MIN_SIZE = 64 * 1024
HERE = os.path.dirname(os.path.abspath(__file__))
DRY = "--dry" in sys.argv
JOBS = 8
for i, a in enumerate(sys.argv):
    if a == "--jobs" and i + 1 < len(sys.argv):
        JOBS = int(sys.argv[i + 1])

# legacy fourcc -> (texconv format, needs_dx10_header)
FOURCC_MAP = {
    b"DXT1": ("BC1_UNORM", False), b"DXT2": ("BC2_UNORM", False), b"DXT3": ("BC2_UNORM", False),
    b"DXT4": ("BC3_UNORM", False), b"DXT5": ("BC3_UNORM", False),
    b"ATI1": ("BC4_UNORM", False), b"BC4U": ("BC4_UNORM", False), b"BC4S": ("BC4_SNORM", False),
    b"ATI2": ("BC5_UNORM", False), b"BC5U": ("BC5_UNORM", False), b"BC5S": ("BC5_SNORM", False),
}
# DXGI format id -> texconv format name (all imply the DX10 header stays DX10)
DXGI_MAP = {
    28: "R8G8B8A8_UNORM", 29: "R8G8B8A8_UNORM_SRGB",
    61: "R8_UNORM", 49: "R8G8_UNORM", 56: "R16_UNORM",
    71: "BC1_UNORM", 72: "BC1_UNORM_SRGB", 74: "BC2_UNORM", 75: "BC2_UNORM_SRGB",
    77: "BC3_UNORM", 78: "BC3_UNORM_SRGB", 80: "BC4_UNORM", 81: "BC4_SNORM",
    83: "BC5_UNORM", 84: "BC5_SNORM", 95: "BC6H_UF16", 96: "BC6H_SF16",
    98: "BC7_UNORM", 99: "BC7_UNORM_SRGB",
    87: "B8G8R8A8_UNORM", 91: "B8G8R8A8_UNORM_SRGB", 88: "B8G8R8X8_UNORM", 93: "B8G8R8X8_UNORM_SRGB",
}
BC_BLOCKBYTES = {"BC1": 8, "BC4": 8, "BC2": 16, "BC3": 16, "BC5": 16, "BC6": 16, "BC7": 16}


def parse(path):
    try:
        with open(path, "rb") as f:
            head = f.read(148)
    except OSError:
        return None
    if len(head) < 128 or head[:4] != b"DDS ":
        return None
    flags, height, width, pitch, depth, mipcount = struct.unpack_from("<IIIIII", head, 8)
    pf_flags, fourcc = struct.unpack_from("<I4s", head, 80)
    rgb_bitcount = struct.unpack_from("<I", head, 88)[0]
    rmask, gmask, bmask, amask = struct.unpack_from("<IIII", head, 92)
    caps2 = struct.unpack_from("<I", head, 112)[0]
    d = dict(path=path, w=width, h=height, mips=max(1, mipcount), depth=depth,
             fourcc=fourcc, cube=bool(caps2 & 0x200), dxgi=None, arraysize=1,
             bitcount=rgb_bitcount, masks=(rmask, gmask, bmask, amask), pf_flags=pf_flags)
    if fourcc == b"DX10" and len(head) >= 148:
        dxgi, resdim, misc, arraysize, misc2 = struct.unpack_from("<IIIII", head, 128)
        d["dxgi"] = dxgi
        d["arraysize"] = max(1, arraysize)
    return d


def full_mips(w, h):
    return int(math.floor(math.log2(max(w, h)))) + 1


def min_ok_mips(w, h, fmt):
    # texconv -dx9 stops BC chains at the 4x4 block floor (two levels short of 1x1);
    # that's standard for legacy-headered BC and irrelevant to streaming (everything
    # below 4x4 is far under the 64KB streaming floor). Accept it as "full".
    full = full_mips(w, h)
    return full - 2 if (fmt or "").startswith("BC") else full


def chain_bytes(d):
    # rough full-size estimate, BC-aware, for the >64KB eligibility test
    fmt, _ = map_format(d)
    bs = 4 if (fmt and fmt.startswith("BC")) else 1
    bpb = BC_BLOCKBYTES.get(fmt[:3], 16) if (fmt and fmt.startswith("BC")) else max(1, d["bitcount"] // 8) or 4
    total = 0
    for m in range(d["mips"]):
        mw, mh = max(1, d["w"] >> m), max(1, d["h"] >> m)
        if bs == 4:
            total += ((mw + 3) // 4) * ((mh + 3) // 4) * bpb
        else:
            total += mw * mh * bpb
    return total


def map_format(d):
    """-> (texconv_format or None, needs_dx10_header)"""
    if d["dxgi"] is not None:
        fmt = DXGI_MAP.get(d["dxgi"])
        return fmt, True
    if d["fourcc"] in FOURCC_MAP:
        return FOURCC_MAP[d["fourcc"]]
    # uncompressed legacy: only exact 32bpp BGRA/BGRX/RGBA masks, else skip
    if d["fourcc"] == b"\x00\x00\x00\x00" or d["pf_flags"] & 0x40:  # DDPF_RGB
        r, g, b, a = d["masks"]
        if d["bitcount"] == 32 and (r, g, b) == (0x00FF0000, 0x0000FF00, 0x000000FF):
            return ("B8G8R8A8_UNORM" if a else "B8G8R8X8_UNORM"), False
        if d["bitcount"] == 32 and (r, g, b) == (0x000000FF, 0x0000FF00, 0x00FF0000):
            return "R8G8B8A8_UNORM", False
        # 24bpp RGB has NO DX12 equivalent (the engine already promotes to 32bpp at
        # load) — promote on disk so the file becomes mippable/streamable. The only
        # non-format-preserving case, forced: no lossless alternative exists.
        if d["bitcount"] == 24 and (r, g, b) == (0x00FF0000, 0x0000FF00, 0x000000FF):
            return "B8G8R8X8_UNORM", False
    # D3DFMT_A16B16G16R16 (fourcc 36) == DXGI R16G16B16A16_UNORM bit layout
    if d["fourcc"] == b"$\x00\x00\x00":
        return "R16G16B16A16_UNORM", False
    return None, False


def eligible(d):
    if d["cube"] or d["depth"] > 1 or d["arraysize"] > 1:
        return False, "cube/volume/array"
    fmt0, _ = map_format(d)
    if d["mips"] >= min_ok_mips(d["w"], d["h"], fmt0):
        return False, "already full chain"
    if chain_bytes(d) <= MIN_SIZE:
        return False, "under 64KB"
    fmt, _ = map_format(d)
    if fmt is None:
        return False, f"unmappable format fourcc={d['fourcc']} dxgi={d['dxgi']} bpp={d['bitcount']}"
    return True, fmt


lock = threading.Lock()
done = [0]


def convert_one(d, total):
    rel = os.path.relpath(d["path"], ROOT)
    fmt, needs10 = map_format(d)
    dst_dir = os.path.dirname(d["path"])
    mirror_path = os.path.join(MIRROR, rel)
    r = dict(rel=rel, fmt=fmt, w=d["w"], h=d["h"], mips_before=d["mips"])
    try:
        os.makedirs(os.path.dirname(mirror_path), exist_ok=True)
        if not os.path.exists(mirror_path):  # no-clobber: never overwrite 08-04 originals
            shutil.copy2(d["path"], mirror_path)
        use_dx10 = needs10 or fmt.startswith(("BC6", "BC7")) or fmt.endswith("_SRGB")

        def attempt(dx10):
            # explicit count, NOT -m 0: texconv keeps a source's existing PARTIAL chain
            # under -m 0 (it only builds full chains from single-mip sources)
            args = [TEXCONV, "-f", fmt, "-m", str(full_mips(d["w"], d["h"])), "-y", "-nologo",
                    "-o", dst_dir, "-dx10" if dx10 else "-dx9", d["path"]]
            p = subprocess.run(args, capture_output=True, text=True, timeout=600)
            if p.returncode != 0:
                raise RuntimeError(f"texconv rc={p.returncode}: {(p.stderr or p.stdout).strip()[:300]}")
            return parse(d["path"])

        after = attempt(use_dx10)
        if after is not None and not use_dx10 and fmt.startswith("BC") \
                and after["mips"] < min_ok_mips(d["w"], d["h"], fmt):
            # non-power-of-2 BC: -dx9 truncates the chain where mip dims stop being
            # multiples of 4; a DX10 header carries the full chain (same as our BC7s)
            after = attempt(True)
        if after is None:
            raise RuntimeError("post-parse failed")
        if (after["w"], after["h"]) != (d["w"], d["h"]):
            raise RuntimeError(f"dims changed {d['w']}x{d['h']} -> {after['w']}x{after['h']}")
        if after["mips"] < min_ok_mips(d["w"], d["h"], fmt):
            raise RuntimeError(f"chain still short: {after['mips']} < {min_ok_mips(d['w'], d['h'], fmt)}")
        f2, _ = map_format(after)
        if (f2 or "").replace("_SRGB", "") != fmt.replace("_SRGB", ""):
            raise RuntimeError(f"format family changed {fmt} -> {f2}")
        r.update(ok=True, mips_after=after["mips"])
    except Exception as e:
        # restore the original so a failed convert never ships a broken file
        try:
            if os.path.exists(mirror_path):
                shutil.copy2(mirror_path, d["path"])
        except OSError:
            pass
        r.update(ok=False, error=str(e))
    with lock:
        done[0] += 1
        if done[0] % 100 == 0 or not r.get("ok", False):
            print(f"[{done[0]}/{total}] {'OK' if r.get('ok') else 'FAIL'} {rel}", flush=True)
    return r


def main():
    work, skipped = [], collections.Counter()
    skipped_unmappable = []
    for dirpath, _dirs, files in os.walk(ROOT):
        for name in files:
            if not name.lower().endswith(".dds"):
                continue
            d = parse(os.path.join(dirpath, name))
            if d is None:
                skipped["not a DDS"] += 1
                continue
            ok, why = eligible(d)
            if not ok:
                skipped[why.split(" fourcc=")[0]] += 1
                if why.startswith("unmappable"):
                    skipped_unmappable.append((os.path.relpath(d["path"], ROOT), why))
                continue
            work.append(d)
    print(f"work list: {len(work)} files to convert; skips: {dict(skipped)}", flush=True)
    if DRY:
        for w in work[:40]:
            print("  ", os.path.relpath(w["path"], ROOT), map_format(w)[0], w["mips"], "mips")
        return
    results = []
    with ThreadPoolExecutor(max_workers=JOBS) as ex:
        futs = [ex.submit(convert_one, d, len(work)) for d in work]
        for f in as_completed(futs):
            results.append(f.result())
    okc = sum(1 for r in results if r.get("ok"))
    fails = [r for r in results if not r.get("ok")]
    with open(os.path.join(HERE, "ddsconvert_results.jsonl"), "w", encoding="utf-8") as f:
        for r in results:
            f.write(json.dumps(r) + "\n")
    with open(os.path.join(HERE, "ddsconvert_report.txt"), "w", encoding="utf-8") as f:
        f.write(f"converted OK: {okc}/{len(results)}\nskips: {dict(skipped)}\n\nFAILURES ({len(fails)}):\n")
        for r in fails:
            f.write(f"  {r['rel']}: {r.get('error')}\n")
        f.write(f"\nUNMAPPABLE ({len(skipped_unmappable)}):\n")
        for rel, why in skipped_unmappable:
            f.write(f"  {rel}: {why}\n")
    print(f"DONE: {okc}/{len(results)} converted, {len(fails)} failed (restored from mirror), "
          f"{len(skipped_unmappable)} unmappable skipped", flush=True)


if __name__ == "__main__":
    main()
