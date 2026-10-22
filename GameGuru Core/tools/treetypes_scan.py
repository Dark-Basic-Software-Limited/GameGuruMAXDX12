# GGMAX 3.123: count a level's tree instances by palette type, straight from the .fpm (a ZipCrypto zip,
# password 'mypassword', holding '<bytes>.tre': uint32 version, then per tree x float, z float, packed
# uint32 - flags in bits 0-7 (bit 0 = visible), type in bits 11-16, scale in bits 17-23). Only types
# 0-37 exist, in DX11 and DX12 alike (compiled in, TreeMeshesLOD0.h g_GGTreesLOD0[38]); there are NO
# custom tree slots. A visible type >= 38 is invalid data no MAX editor writes - both builds hide it
# on load (DX11 silently; DX12 3.123 logs it, see GGTrees_GetLastLoadInvalidTypeTrees).
#
# For an .fpm it also names the build that LAST SAVED the level (DX11 or DX12) from its visuals.ini:
# DX12 writes per-level settings DX11 never writes (absent from DX11 M-Visuals.cpp), and both builds
# rewrite the whole file on every save (DX11 visuals_save deletes it first), so the key set belongs to
# the last saver. The newest DX12 setting present dates the saving build.
#
# usage: python -I treetypes_scan.py <level.fpm | extracted .tre> [...]
import os, re, struct, sys, zipfile, collections, hashlib

NUM_STOCK_TYPES = 38

# visuals.ini keys only DX12 writes, with the first DX12 commit that wrote each (oldest first).
DX12_MARKERS = [
    ('visuals.WaterCausticSize', '2026-07-19', 'Caustic Size slider'),
    ('visuals.UnderwaterFog', '2026-07-21', 'Underwater Color / Fog'),
    ('visuals.TreeShadowLODDist', '2026-07-28', 'tree shadow LOD distance'),
    ('visuals.LowVRAM', '2026-08-04', 'Low VRAM Mode'),
    ('visuals.TerrainBake', '2026-08-26', '3.25 Terrain Bake'),
    ('visuals.WaterBake', '2026-08-26', '3.25 Water Bake'),
    ('visuals.Wateralpha', '2026-08-26', '3.25f water alpha'),
    ('visuals.OcclusionCullDelay', '2026-08-28', '3.28 Occlusion Cull Delay'),
    ('visuals.SuperQuickLevel', '2026-08-28', '3.34 Super Quick rung'),
    ('visuals.ReflectionWidth', '2026-09-23', '3.89 Water Reflection Size'),
    ('visuals.ReflectionBlur', '2026-09-23', '3.90 Water Reflection Blur'),
]

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
    bad = {t: c for t, c in vis_types.items() if t >= NUM_STOCK_TYPES}
    custom_vis = sum(bad.values())
    print('%s: version %d, %d slots, %d visible, %d visible with type >= %d' % (
        label, version, n, sum(vis_types.values()), custom_vis, NUM_STOCK_TYPES))
    print('   visible by type: ' + ', '.join('%d:%d' % (t, c) for t, c in sorted(vis_types.items())))
    if custom_vis:
        # version 2 is relabelled 3 at load (M-MapFile_part0.cpp), so a genuine old version-2 file (5-bit
        # type, scale from bit 16) turns odd-scale types 6/7/8 into 38/39/40 - and nothing higher
        if max(bad) <= 40:
            print('   invalid types only 38-40: the signature of an OLD version-2 tree file (pre-2022 format)')
        else:
            print('   invalid types spread over %d-%d: damaged tree data, not an old format' % (min(bad), max(bad)))

def fingerprint(z, label):
    try:
        text = z.read('visuals.ini').decode('latin-1')
    except KeyError:
        print('%s: no visuals.ini - cannot tell which build saved it' % label)
        return
    keys = set(re.findall(r'^\s*([A-Za-z_][A-Za-z0-9_.]*)\s*=', text, re.M))
    present = [m for m in DX12_MARKERS if m[0] in keys]
    if not present:
        print('%s: last saved by DX11 (or a DX12 build older than %s) - none of the %d DX12-only settings in visuals.ini'
              % (label, DX12_MARKERS[0][1], len(DX12_MARKERS)))
        return
    newest = max(present, key=lambda m: m[1])
    missing = [m[0] for m in DX12_MARKERS if m[0] not in keys and m[1] <= newest[1]]
    later = [m for m in DX12_MARKERS if m[0] not in keys and m[1] > newest[1]]
    window = ('from %s, before %s (no %s)' % (newest[1], later[0][1], later[0][2])) if later else ('from %s or later' % newest[1])
    print('%s: last saved by DX12 - %d of %d DX12-only settings in visuals.ini; newest %s (%s) = a DX12 build %s'
          % (label, len(present), len(DX12_MARKERS), newest[0], newest[2], window))
    if missing:
        print('   ⚠ older DX12 settings missing (%s) - hand-edited or mixed file?' % ', '.join(missing))

for path in sys.argv[1:]:
    if path.lower().endswith('.fpm'):
        whole = open(path, 'rb').read()
        name = os.path.basename(path)
        print('%s: %d bytes, md5 %s' % (name, len(whole), hashlib.md5(whole).hexdigest()))
        with zipfile.ZipFile(path) as z:
            z.setpassword(b'mypassword')
            fingerprint(z, name)
            for info in z.infolist():
                if info.filename.lower().endswith('.tre'):
                    raw = z.read(info)
                    scan_tre(raw, '%s :: %s (%d bytes, md5 %s)' % (name, info.filename, len(raw), hashlib.md5(raw).hexdigest()[:8]))
    else:
        raw = open(path, 'rb').read()
        scan_tre(raw, path)
