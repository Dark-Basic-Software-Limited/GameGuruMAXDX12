# Report the entity-file version inside a GameGuru MAX .fpm level.
#   python ele_version.py "path\to\level.fpm"
# The .fpm is a password-protected zip; map.ele's first 4 bytes are the format version.
import sys, zipfile, struct, os

if len(sys.argv) < 2:
    print("usage: python ele_version.py <level.fpm> [more.fpm ...]"); sys.exit(2)

for p in sys.argv[1:]:
    if not os.path.isfile(p):
        print("  MISSING  %s" % p); continue
    try:
        z = zipfile.ZipFile(p)
        z.setpassword(b"mypassword")
        eles = [n for n in z.namelist() if n.lower().endswith(".ele")]
        if not eles:
            print("  no .ele inside  %s" % p); continue
        for n in eles:
            d = z.read(n)
            v = struct.unpack_from("<i", d, 0)[0]
            note = ""
            if v > 342:   note = "  <-- NEWER than this build writes (342): trailing fields would be left unread"
            elif v < 340: note = "  <-- below 340: skips the block 3.38 touched entirely"
            print("  v%-5d %9d bytes  %-12s  %s%s" % (v, len(d), n, os.path.basename(p), note))
    except Exception as e:
        print("  ERROR %s: %s" % (os.path.basename(p), e))
