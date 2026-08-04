#!/usr/bin/env python3
"""
Decoder for GameGuru MAX .PE particle effect files.

A .PE file is a wiArchive scene archive written by the DX11-era GameGuru fork of
Wicked Engine (D:\\max\\WickedRepo), whose __archiveVersion is 5077. Every shipped
.PE declares version 5076.

Archive primitive widths (WickedRepo/WickedEngine/wiArchive.h):
    bool            -> uint32   (4 bytes)
    char            -> int8     (1 byte)
    int             -> int64    (8 bytes)
    unsigned int    -> uint64   (8 bytes)   <-- note: uint32_t fields occupy 8 bytes
    size_t          -> uint64   (8 bytes)
    float           -> 4 bytes
    XMFLOAT3        -> 12 bytes
    std::string     -> uint64 len (INCLUDING null terminator) + len bytes
    Entity          -> uint64   (wiECS::SerializeEntity)

Scene layout (GameGuruMAX/GameGuru Core/Guru-WickedMAX/wickedcalls.cpp:7543 WickedCall_LoadWiSceneDirect):
    uint64 version
    uint64 reserved                     (uint32_t promoted to 8 bytes)
    resource block (version >= 63)      uint64 count, then per resource:
                                        string name, uint64 flags, vector<uint8> filedata
    names, layers, transforms, prev_transforms, hierarchy, materials, meshes,
    impostors, objects, aabb_objects, rigidbodies, softbodies, armatures, lights,
    aabb_lights, cameras, probes, aabb_probes, forces, decals, aabb_decals,
    animations, EMITTERS, hairs, weathers, ...

Rather than implement every component serializer (MaterialComponent alone is large and
heavily version-gated), this tool locates the emitter block by signature scan and then
validates the parse. The emitter record layout for archive version 5076 is taken from
wiEmittedParticle::Serialize (WickedRepo/WickedEngine/wiEmittedParticle.cpp:896).
"""

import struct
import sys
import glob
import os

U64 = struct.Struct('<Q')
F32 = struct.Struct('<f')
U32 = struct.Struct('<I')


class Reader:
    def __init__(self, data, pos=0):
        self.d = data
        self.p = pos

    def u64(self):
        v = U64.unpack_from(self.d, self.p)[0]
        self.p += 8
        return v

    def f32(self):
        v = F32.unpack_from(self.d, self.p)[0]
        self.p += 4
        return v

    def b32(self):
        v = U32.unpack_from(self.d, self.p)[0]
        self.p += 4
        return v != 0

    def f3(self):
        v = struct.unpack_from('<fff', self.d, self.p)
        self.p += 12
        return v

    def string(self):
        n = self.u64()
        s = self.d[self.p:self.p + n]
        self.p += n
        return s.rstrip(b'\0').decode('utf-8', 'replace')


# Field order for archive version 5076, from wiEmittedParticle::Serialize.
# (name, kind) where kind is one of u64/f32/bool/f3
EMITTER_FIELDS = [
    ('_flags', 'u64'), ('shaderType', 'u64'), ('meshID', 'u64'), ('MAX_PARTICLES', 'u64'),
    ('FIXED_TIMESTEP', 'f32'), ('size', 'f32'), ('random_factor', 'f32'),
    ('normal_factor', 'f32'), ('count', 'f32'), ('life', 'f32'), ('random_life', 'f32'),
    ('scaleX', 'f32'), ('scaleY', 'f32'), ('rotation', 'f32'), ('motionBlurAmount', 'f32'),
    ('mass', 'f32'), ('SPH_h', 'f32'), ('SPH_K', 'f32'), ('SPH_p0', 'f32'), ('SPH_e', 'f32'),
    # >= 45
    ('framesX', 'u64'), ('framesY', 'u64'), ('frameCount', 'u64'), ('frameStart', 'u64'),
    ('frameRate', 'f32'),
    # >= 64
    ('velocity', 'f3'), ('gravity', 'f3'), ('drag', 'f32'), ('random_color', 'f32'),
    # >= 5072  (GameGuru fork extensions begin here)
    ('restitution', 'f32'), ('fadein_time', 'f32'), ('burst_amount', 'f32'), ('burst_delay', 'f32'),
    # >= 5073
    ('normal_factor_x', 'f32'), ('normal_factor_y', 'f32'), ('normal_factor_z', 'f32'),
    # >= 5074
    ('normal_random', 'f32'), ('rotation_random', 'f32'), ('size_random', 'f32'),
    ('spawn_random', 'f32'), ('scaling_random', 'f32'), ('spawn_pause', 'f32'),
    ('spawn_pause_random', 'f32'),
    ('endcolor_red', 'u64'), ('endcolor_green', 'u64'), ('endcolor_blue', 'u64'),
    ('burst_split', 'f32'),
    ('burst_factor_x', 'f32'), ('burst_factor_y', 'f32'), ('burst_factor_z', 'f32'),
    # >= 5075
    ('startpos', 'f3'), ('bFindFloor', 'bool'), ('burst_factor_speed', 'f32'),
    ('start_rotation', 'f32'), ('bFollowCamera', 'bool'),
    # >= 5076
    ('random_position', 'f32'), ('random_position_scale', 'f32'),
]

SIZES = {'u64': 8, 'f32': 4, 'bool': 4, 'f3': 12}
EMITTER_SIZE = sum(SIZES[k] for _, k in EMITTER_FIELDS)

FLAG_NAMES = [
    (1 << 0, 'DEBUG'), (1 << 1, 'PAUSED'), (1 << 2, 'SORTING'),
    (1 << 3, 'DEPTHCOLLISION'), (1 << 4, 'SPH_FLUIDSIMULATION'),
    (1 << 5, 'HAS_VOLUME'), (1 << 6, 'FRAME_BLENDING'), (1 << 7, 'EMIT_PAUSE'),
]
SHADER_TYPES = ['SOFT', 'SOFT_DISTORTION', 'SIMPLE', 'SOFT_LIGHTING']


def read_emitter(r):
    e = {}
    for name, kind in EMITTER_FIELDS:
        e[name] = getattr(r, {'u64': 'u64', 'f32': 'f32', 'bool': 'b32', 'f3': 'f3'}[kind])()
    return e


def plausible(e):
    """Signature test: does this look like a real emitter record?"""
    if e['shaderType'] > 3:
        return False
    if not (1 <= e['MAX_PARTICLES'] <= 10_000_000):
        return False
    if e['_flags'] > 0xFF:
        return False
    if not (e['FIXED_TIMESTEP'] == -1.0 or 0.0 <= e['FIXED_TIMESTEP'] <= 1.0):
        return False
    if not (0.0 < e['size'] < 100000.0):
        return False
    if not (0.0 < e['life'] <= 100000.0):
        return False
    if not (0.0 <= e['count'] <= 1_000_000.0):
        return False
    for k in ('framesX', 'framesY', 'frameCount'):
        if not (1 <= e[k] <= 4096):
            return False
    if not (0 <= e['frameStart'] <= 4096):
        return False
    for k in ('endcolor_red', 'endcolor_green', 'endcolor_blue'):
        if e[k] > 255:
            return False
    if not (0.0 <= e['drag'] <= 100.0):
        return False
    return True


def find_emitters(data):
    """Scan for the emitters ComponentManager block and parse it."""
    n = len(data)
    results = []
    # The block is: uint64 count, count x emitter record, count x uint64 entity
    for off in range(0, n - 8):
        cnt = U64.unpack_from(data, off)[0]
        if not (1 <= cnt <= 16):
            continue
        need = 8 + cnt * EMITTER_SIZE + cnt * 8
        if off + need > n:
            continue
        r = Reader(data, off + 8)
        emitters = []
        ok = True
        try:
            for _ in range(cnt):
                e = read_emitter(r)
                if not plausible(e):
                    ok = False
                    break
                emitters.append(e)
        except struct.error:
            ok = False
        if not ok:
            continue
        try:
            ents = [r.u64() for _ in range(cnt)]
        except struct.error:
            continue
        if any(x == 0 or x > (1 << 40) for x in ents):
            continue
        results.append((off, cnt, emitters, ents))
    return results


def parse_header(data):
    r = Reader(data)
    version = r.u64()
    reserved = r.u64()
    info = {'version': version, 'reserved': reserved, 'resources': []}
    if version >= 63:
        rc = r.u64()
        info['resource_count'] = rc
        for _ in range(min(rc, 64)):
            name = r.string()
            flags = r.u64()
            dl = r.u64()
            r.p += dl
            info['resources'].append((name, flags, dl))
    # names manager
    ncount = r.u64()
    names = []
    for _ in range(min(ncount, 256)):
        names.append(r.string())
    info['names'] = names
    return info


def fmt(v):
    if isinstance(v, tuple):
        return '(' + ', '.join('%.4g' % x for x in v) + ')'
    if isinstance(v, float):
        return '%.6g' % v
    return str(v)


def main():
    files = sys.argv[1:]
    if not files:
        files = sorted(glob.glob('*.pe'))
    for f in files:
        with open(f, 'rb') as h:
            data = h.read()
        print('=' * 78)
        print(os.path.basename(f), '  (%d bytes)' % len(data))
        print('=' * 78)
        try:
            hdr = parse_header(data)
            print('  archiveVersion : %d' % hdr['version'])
            print('  embedded res   : %s' % hdr.get('resource_count', 'n/a'))
            for (nm, fl, dl) in hdr['resources']:
                print('       - %-40s flags=%d  %d bytes' % (nm, fl, dl))
            print('  names          : %s' % ', '.join(hdr['names'][:12]))
        except Exception as ex:
            print('  header parse failed: %s' % ex)
        found = find_emitters(data)
        if not found:
            print('  NO EMITTER BLOCK FOUND')
            continue
        # Prefer the candidate with the most emitters, then the earliest offset.
        found.sort(key=lambda x: (-x[1], x[0]))
        off, cnt, emitters, ents = found[0]
        print('  emitter block  : offset %d, %d emitter(s), entities %s' % (off, cnt, ents))
        if len(found) > 1:
            print('  (%d candidate offsets matched the signature; using the richest)' % len(found))
        for i, e in enumerate(emitters):
            flags = e['_flags']
            fl = [nm for bit, nm in FLAG_NAMES if flags & bit] or ['none']
            print('  --- emitter %d ---' % i)
            print('      flags=0x%02X (%s)  shader=%s  MAX_PARTICLES=%d' % (
                flags, '|'.join(fl),
                SHADER_TYPES[e['shaderType']] if e['shaderType'] < 4 else '?',
                e['MAX_PARTICLES']))
            order = [k for k, _ in EMITTER_FIELDS
                     if k not in ('_flags', 'shaderType', 'meshID', 'MAX_PARTICLES')]
            line = []
            for k in order:
                line.append('%s=%s' % (k, fmt(e[k])))
            # print 3 per line for readability
            for j in range(0, len(line), 3):
                print('      ' + '  '.join('%-30s' % x for x in line[j:j + 3]))
        print()


if __name__ == '__main__':
    main()
