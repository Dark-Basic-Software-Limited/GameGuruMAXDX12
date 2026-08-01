#!/usr/bin/env python3
"""Scan DDS content for the two properties that decide whether texture streaming helps:
   - single-mip files (can never stream at all)
   - block-compressed files whose dimensions stop halving early (block alignment)
Reports how many mip levels the streaming reduction can actually shed for each file.
"""
import os, struct, sys, collections

ROOT = sys.argv[1] if len(sys.argv) > 1 else r"D:\DEV\BUILD\GameGuru Wicked MAX Build Area\Max\Files"
MIN_SIZE = 64 * 1024   # engine's streaming_texture_min_size

def block_info(fourcc, flags, rgb_bitcount):
    """Return (block_size, bytes_per_block_or_pixel, is_bc)."""
    if fourcc in (b"DXT1",):            return 4, 8, True
    if fourcc in (b"DXT2", b"DXT3", b"DXT4", b"DXT5"): return 4, 16, True
    if fourcc in (b"ATI1", b"BC4U", b"BC4S"): return 4, 8, True
    if fourcc in (b"ATI2", b"BC5U", b"BC5S"): return 4, 16, True
    if fourcc == b"DX10":               return 4, 16, True   # assume BC7/BC6 in practice
    return 1, max(1, rgb_bitcount // 8), False

def mip_chain_bytes(w, h, mips, bs, bpb):
    total = 0
    for m in range(mips):
        mw = max(1, w >> m); mh = max(1, h >> m)
        bx = (mw + bs - 1) // bs; by = (mh + bs - 1) // bs
        total += bx * by * bpb
    return total

def analyse(path):
    try:
        with open(path, "rb") as f:
            head = f.read(128)
    except OSError:
        return None
    if len(head) < 128 or head[:4] != b"DDS ":
        return None
    flags, height, width, pitch, depth, mipcount = struct.unpack_from("<IIIIII", head, 8)
    pf_flags, fourcc = struct.unpack_from("<I4s", head, 80)
    rgb_bitcount = struct.unpack_from("<I", head, 88)[0]
    caps2 = struct.unpack_from("<I", head, 112)[0]
    is_cube = bool(caps2 & 0x200)
    mips = max(1, mipcount)
    bs, bpb, is_bc = block_info(fourcc, pf_flags, rgb_bitcount)

    # How many halvings can the engine's reduction perform?
    # Stops when the remaining chain fits under 64KB, or (with the 1.73 fix) when the
    # next halving would break block alignment.
    shed = 0
    w, h, m = width, height, mips
    while m > 1 and depth <= 1 and not is_cube and mip_chain_bytes(w, h, m, bs, bpb) > MIN_SIZE:
        nw, nh = w >> 1, h >> 1
        if is_bc and (nw % bs != 0 or nh % bs != 0):
            break
        w, h, m = nw, nh, m - 1
        shed += 1

    # What it WOULD have shed without the alignment rule (pre-1.73 behaviour)
    shed_unaligned = 0
    w2, h2, m2 = width, height, mips
    while m2 > 1 and depth <= 1 and not is_cube and mip_chain_bytes(w2, h2, m2, bs, bpb) > MIN_SIZE:
        w2, h2, m2 = w2 >> 1, h2 >> 1, m2 - 1
        shed_unaligned += 1

    full = mip_chain_bytes(width, height, mips, bs, bpb)
    kept = mip_chain_bytes(w, h, m, bs, bpb)
    return dict(path=path, w=width, h=height, mips=mips, fourcc=fourcc.decode("ascii", "replace"),
                is_bc=is_bc, is_cube=is_cube, shed=shed, shed_unaligned=shed_unaligned,
                full=full, kept=kept, blocked=(shed < shed_unaligned))

singlemip = []
blocked = []
total_files = 0
total_full = 0
total_kept = 0
total_kept_ifunaligned = 0
by_ext = collections.Counter()

for dirpath, _dirs, files in os.walk(ROOT):
    for name in files:
        if not name.lower().endswith(".dds"):
            continue
        p = os.path.join(dirpath, name)
        r = analyse(p)
        if r is None:
            continue
        total_files += 1
        total_full += r["full"]
        total_kept += r["kept"]
        # what the pre-fix reduction would have kept
        w2, h2, m2 = r["w"], r["h"], r["mips"]
        bs, bpb, is_bc = block_info(r["fourcc"].encode(), 0, 32)
        for _ in range(r["shed_unaligned"]):
            w2, h2, m2 = w2 >> 1, h2 >> 1, m2 - 1
        total_kept_ifunaligned += mip_chain_bytes(w2, h2, m2, bs, bpb)
        if r["mips"] == 1 and r["full"] > MIN_SIZE:
            singlemip.append(r)
        if r["blocked"]:
            blocked.append(r)

MB = 1024.0 * 1024.0
print(f"scanned {total_files} DDS under {ROOT}")
print(f"full mip chains          : {total_full/MB:9.1f} MB")
print(f"streamed base (1.73 fix) : {total_kept/MB:9.1f} MB")
print(f"streamed base (pre-fix)  : {total_kept_ifunaligned/MB:9.1f} MB")
print(f"COST OF THE ALIGNMENT FIX: {(total_kept-total_kept_ifunaligned)/MB:9.1f} MB across ALL content")
print()
print(f"--- block-alignment blocked ({len(blocked)} files) ---")
agg = collections.Counter()
for r in blocked:
    agg[(r['w'], r['h'], r['fourcc'])] += 1
for (w, h, fc), n in agg.most_common(20):
    print(f"  {w}x{h} {fc}: {n} files")
print()
print(f"--- single-mip >64KB, can NEVER stream ({len(singlemip)} files) ---")
sm_bytes = sum(r['full'] for r in singlemip)
print(f"  total {sm_bytes/MB:.1f} MB locked at full size")
for r in sorted(singlemip, key=lambda x: -x['full'])[:15]:
    print(f"  {r['full']/MB:7.2f} MB  {r['w']}x{r['h']} {r['fourcc']}  {os.path.relpath(r['path'], ROOT)}")
