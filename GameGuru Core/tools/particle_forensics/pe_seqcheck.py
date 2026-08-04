#!/usr/bin/env python3
"""
Validates the SEQUENTIAL .PE walk implemented in C++ (WickedCall_LoadLegacyWPE)
by mirroring it exactly here and checking that it lands on the emitter block at
the same offset the independent signature scan in pe_decode.py finds.

If these two agree for every shipped file, the component layout - including the
large version-gated MaterialComponent - is right.
"""
import struct, sys, glob, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from pe_decode import Reader, find_emitters, EMITTER_SIZE


class R(Reader):
    def u8(self):
        v = self.d[self.p]; self.p += 1; return v

    def f4v(self):
        v = struct.unpack_from('<ffff', self.d, self.p); self.p += 16; return v

    def f2v(self):
        v = struct.unpack_from('<ff', self.d, self.p); self.p += 8; return v


def read_material(r, ver):
    r.u64()                       # _flags
    r.u8(); r.u8(); r.u8()        # engineStencilRef, userStencilRef, userBlendMode
    r.f4v()                       # baseColor
    if ver >= 25: r.f4v()         # emissiveColor
    r.f4v()                       # texMulAdd
    r.f32(); r.f32(); r.f32()     # roughness, reflectance, metalness
    r.f32()                       # refraction
    r.f32(); r.f32(); r.f32()     # normalMapStrength, parallaxOcclusionMapping, alphaRef
    r.f2v()                       # texAnimDirection
    r.f32(); r.f32()              # texAnimFrameRate, texAnimElapsedTime
    r.string(); r.string(); r.string(); r.string()   # basecolor, surface, normal, displacement
    if ver >= 24: r.string()      # emissive
    if ver >= 28:
        r.string()                # occlusion
        for _ in range(6): r.u64()
        r.f32()                   # displacementMapping
    if ver >= 48: r.u8()          # shadingRate
    if ver >= 50: r.u64(); r.u64()
    if 52 <= ver < 54: r.u64()
    if ver >= 54: r.f4v()
    if ver >= 56: r.f4v()
    if ver >= 59: r.f32(); r.string(); r.u64()
    if ver >= 61:
        r.f4v(); r.f32()
        r.string(); r.string()
        r.u64(); r.u64()
        r.f32(); r.f32()
        r.string(); r.string(); r.string()
        r.u64(); r.u64(); r.u64()
    if ver >= 68: r.string(); r.u64()


def sequential_emitter_offset(data):
    r = R(data)
    ver = r.u64()
    if ver < 5000 or ver > 5077:
        return None, 'not legacy (v%d)' % ver
    r.u64()                                   # reserved
    if ver >= 63:
        rc = r.u64()
        for _ in range(rc):
            r.string(); r.u64()
            ln = r.u64(); r.p += ln
    n = r.u64()                               # names
    for _ in range(n): r.string()
    for _ in range(n): r.u64()
    n = r.u64()                               # layers
    for _ in range(n): r.u64()
    for _ in range(n): r.u64()
    n = r.u64()                               # transforms
    for _ in range(n):
        r.u64(); r.f3(); r.f4v(); r.f3()
    for _ in range(n): r.u64()
    n = r.u64()                               # prev_transforms
    for _ in range(n): r.u64()
    n = r.u64()                               # hierarchy
    for _ in range(n): r.u64(); r.u64()
    for _ in range(n): r.u64()
    n = r.u64()                               # materials
    for _ in range(n): read_material(r, ver)
    for _ in range(n): r.u64()
    for i in range(16):                       # 16 always-empty managers
        c = r.u64()
        if c != 0:
            return None, 'manager %d non-zero (%d)' % (i, c)
    return r.p, None                          # offset of the emitters count


def main():
    files = sys.argv[1:] or sorted(glob.glob('*.pe'))
    okc = badc = 0
    for f in files:
        data = open(f, 'rb').read()
        scan = find_emitters(data)
        scan.sort(key=lambda x: (-x[1], x[0]))
        scan_off = scan[0][0] if scan else None
        seq_off, err = sequential_emitter_offset(data)
        name = os.path.basename(f)
        if err:
            print('%-42s SEQ FAILED: %s' % (name, err)); badc += 1
        elif scan_off is None:
            print('%-42s scan found nothing (seq=%d)' % (name, seq_off)); badc += 1
        elif seq_off == scan_off:
            print('%-42s OK   emitters at %d' % (name, seq_off)); okc += 1
        else:
            print('%-42s MISMATCH seq=%d scan=%d' % (name, seq_off, scan_off)); badc += 1
    print('\n%d agree, %d disagree' % (okc, badc))


if __name__ == '__main__':
    main()
