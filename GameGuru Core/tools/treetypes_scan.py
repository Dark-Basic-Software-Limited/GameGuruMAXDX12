# GGMAX 3.123: count a level's tree instances by palette type, straight from the .fpm (a ZipCrypto zip,
# password 'mypassword', holding '<bytes>.tre': uint32 version, then per tree x float, z float, packed
# uint32 - flags in bits 0-7 (bit 0 = visible), type in bits 11-16, scale in bits 17-23). Only types
# 0-37 exist, in DX11 and DX12 alike (compiled in, TreeMeshesLOD0.h g_GGTreesLOD0[38]); there are NO
# custom tree slots. A visible type >= 38 is invalid data no MAX editor writes - both builds hide it
# on load (DX11 silently; DX12 3.123 logs it, see GGTrees_GetLastLoadInvalidTypeTrees).
#
# usage: python -I treetypes_scan.py <level.fpm | extracted .tre> [...]
import os, struct, sys, zipfile, collections, hashlib

NUM_STOCK_TYPES = 38

def scan_tre(raw, label):
    n = (len(raw) // 4 - 1) // 3
    version = struct.unpack_from('<I', raw, 0)[0]
    vis_types = collections.Counter()
    all_types = collections.Counter()
    for i in range(n):
        x, z, d = struct.unpack_from('<ffI', raw, 4 + i * 12)
        t = (d >> 11) & 0x3F
        all_types[t] += 1
        if d & 0x1:
            vis_types[t] += 1
    custom_vis = sum(c for t, c in vis_types.items() if t >= NUM_STOCK_TYPES)
    print('%s: version %d, %d slots, %d visible, %d visible with type >= %d' % (
        label, version, n, sum(vis_types.values()), custom_vis, NUM_STOCK_TYPES))
    print('   visible by type: ' + ', '.join('%d:%d' % (t, c) for t, c in sorted(vis_types.items())))

for path in sys.argv[1:]:
    if path.lower().endswith('.fpm'):
        with zipfile.ZipFile(path) as z:
            z.setpassword(b'mypassword')
            for info in z.infolist():
                if info.filename.lower().endswith('.tre'):
                    raw = z.read(info)
                    scan_tre(raw, '%s :: %s (%d bytes, md5 %s)' % (os.path.basename(path), info.filename, len(raw), hashlib.md5(raw).hexdigest()[:8]))
    else:
        raw = open(path, 'rb').read()
        scan_tre(raw, path)
