# GGMAX 3.123: copy a GG MAX .fpm and rewrite the first N VISIBLE trees in its <size>.tre to invalid tree
# types >= 38 - the data tester report #8 loaded ('TREE TYPES DROPPED: 15015 ...'). Writes a new
# ZipCrypto('mypassword') .fpm: the engine's minizip applies the password to every entry, so an
# unencrypted repack would not load. 'spread' = types 38..63 (corruption signature), 'v2sig' = 38..40
# only (an old version-2 .tre relabelled 3: the scale's low bit lands in the type's top bit).
# Write the result to a SCRATCH folder; to reproduce the tester, copy it over a writable-area mapbank
# copy of the level (Documents/GameGuruApps/GameGuruMAX/Files/mapbank/<level>.fpm) - and move it back out.
# usage: python -I tre_craft.py <src.fpm> <dst.fpm> <count> [spread|v2sig]
import sys, zipfile, zlib, struct, os, binascii, array, time

PWD = b'mypassword'
CRCTAB = [0] * 256
for i in range(256):
    c = i
    for _ in range(8):
        c = (c >> 1) ^ 0xEDB88320 if c & 1 else c >> 1
    CRCTAB[i] = c

class ZipCrypto:
    def __init__(self, pwd):
        self.k0, self.k1, self.k2 = 0x12345678, 0x23456789, 0x34567890
        for b in pwd: self.update(b)
    def update(self, b):
        self.k0 = (self.k0 >> 8) ^ CRCTAB[(self.k0 ^ b) & 0xFF]
        self.k1 = (self.k1 + (self.k0 & 0xFF)) & 0xFFFFFFFF
        self.k1 = (self.k1 * 134775813 + 1) & 0xFFFFFFFF
        self.k2 = (self.k2 >> 8) ^ CRCTAB[(self.k2 ^ (self.k1 >> 24)) & 0xFF]
    def encrypt(self, data):
        out = bytearray(len(data))
        for i, p in enumerate(data):
            t = (self.k2 | 2) & 0xFFFF
            out[i] = p ^ (((t * (t ^ 1)) >> 8) & 0xFF)
            self.update(p)
        return bytes(out)

def craft_tre(raw, count, mode):
    a = array.array('I'); a.frombytes(raw)
    n = (len(a) - 1) // 3
    done = 0
    for i in range(n):
        if done >= count: break
        d = a[1 + i * 3 + 2]
        if d & 1:
            t = (38 + done % 26) if mode == 'spread' else (38 + done % 3)
            a[1 + i * 3 + 2] = (d & ~(0x3F << 11)) | (t << 11)
            done += 1
    return a.tobytes(), done

def main():
    src, dst, count = sys.argv[1], sys.argv[2], int(sys.argv[3])
    mode = sys.argv[4] if len(sys.argv) > 4 else 'spread'
    zin = zipfile.ZipFile(src)
    entries = []
    for info in zin.infolist():
        try: data = zin.read(info)
        except RuntimeError: data = zin.read(info, pwd=PWD)
        if info.filename.lower().endswith('.tre'):
            data, done = craft_tre(data, count, mode)
            print('crafted %s: %d trees -> types >= 38 (%s)' % (info.filename, done, mode))
        entries.append((info.filename, data, info.date_time))
    out = open(dst, 'wb'); central = []
    for name, data, dt in entries:
        crc = binascii.crc32(data) & 0xFFFFFFFF
        co = zlib.compressobj(6, zlib.DEFLATED, -15); comp = co.compress(data) + co.flush()
        zc = ZipCrypto(PWD)
        hdr = bytearray(os.urandom(11)) + bytes([crc >> 24])
        enc = zc.encrypt(bytes(hdr)) + zc.encrypt(comp)
        dostime = (dt[3] << 11) | (dt[4] << 5) | (dt[5] // 2)
        dosdate = ((dt[0] - 1980) << 9) | (dt[1] << 5) | dt[2]
        fn = name.encode('cp437')
        off = out.tell()
        out.write(struct.pack('<IHHHHHIIIHH', 0x04034b50, 20, 1, 8, dostime, dosdate, crc, len(enc), len(data), len(fn), 0) + fn + enc)
        central.append(struct.pack('<IHHHHHHIIIHHHHHII', 0x02014b50, 20, 20, 1, 8, dostime, dosdate, crc, len(enc), len(data), len(fn), 0, 0, 0, 0, 0x20, off) + fn)
    cd_off = out.tell(); cd = b''.join(central); out.write(cd)
    out.write(struct.pack('<IHHHHIIH', 0x06054b50, 0, 0, len(central), len(central), len(cd), cd_off, 0))
    out.close()
    print('wrote', dst, os.path.getsize(dst), 'bytes')

main()
