#!/usr/bin/env python3
"""MILESTONE_DDS_CONVERSION.md §2 rider: scan every *_surface.dds for degenerate
channels while the tree is being walked. Wicked surfacemap convention is
R=occlusion, G=roughness, B=metalness, A=reflectance — a channel with (near-)zero
variance is unauthored (the known case: 'adult female head 15_surface' shipped a
flat-255 occlusion). Pillow decodes DXT1/3/5; anything it can't read is decompressed
via texconv to a temp PNG first.

Reports per file: per-channel (mean, std), flagging any channel with std < 0.5.
Writes surfacescan_report.txt next to itself.
"""
import os, sys, subprocess, tempfile
from PIL import Image
import PIL

ROOT = r"D:\DEV\BUILD\GameGuru Wicked MAX Build Area\Max\Files"
TEXCONV = r"D:\max\tools\texconv.exe"
HERE = os.path.dirname(os.path.abspath(__file__))
CHAN = ["R=occlusion", "G=roughness", "B=metalness", "A=reflectance"]


def stats(img):
    img = img.convert("RGBA")
    out = []
    st = img.getextrema()
    from PIL import ImageStat
    s = ImageStat.Stat(img)
    for i in range(4):
        out.append((s.mean[i], s.stddev[i], st[i][0], st[i][1]))
    return out


def load(path, tmpdir):
    try:
        return Image.open(path)
    except Exception:
        pass
    # fallback: texconv to PNG (base mip)
    p = subprocess.run([TEXCONV, "-ft", "png", "-m", "1", "-y", "-nologo", "-o", tmpdir, path],
                       capture_output=True, text=True, timeout=300)
    if p.returncode != 0:
        return None
    png = os.path.join(tmpdir, os.path.splitext(os.path.basename(path))[0] + ".png")
    return Image.open(png) if os.path.exists(png) else None


def main():
    flagged, clean, unreadable = [], 0, []
    with tempfile.TemporaryDirectory() as tmpdir:
        for dirpath, _dirs, files in os.walk(ROOT):
            for name in files:
                if not name.lower().endswith("_surface.dds"):
                    continue
                path = os.path.join(dirpath, name)
                rel = os.path.relpath(path, ROOT)
                try:
                    img = load(path, tmpdir)
                except Exception:
                    img = None
                if img is None:
                    unreadable.append(rel)
                    continue
                try:
                    ch = stats(img)
                except Exception as e:
                    unreadable.append(f"{rel} ({e})")
                    continue
                bad = [(CHAN[i], ch[i]) for i in range(4) if ch[i][1] < 0.5]
                if bad:
                    flagged.append((rel, ch, bad))
                else:
                    clean += 1
    with open(os.path.join(HERE, "surfacescan_report.txt"), "w", encoding="utf-8") as f:
        f.write(f"scanned *_surface.dds under {ROOT}\n")
        f.write(f"clean: {clean}   flagged (a channel with std<0.5): {len(flagged)}   unreadable: {len(unreadable)}\n\n")
        f.write("FLAGGED (degenerate = unauthored channel; AO re-bakes are authoring, not repair):\n")
        for rel, ch, bad in sorted(flagged):
            badstr = ", ".join(f"{c} flat@{int(v[0])}" for c, v in bad)
            f.write(f"  {rel}\n    {badstr}\n")
            f.write("    full: " + "  ".join(
                f"{CHAN[i][0]}(mean {ch[i][0]:.1f} std {ch[i][1]:.1f})" for i in range(4)) + "\n")
        if unreadable:
            f.write("\nUNREADABLE:\n")
            for r in unreadable:
                f.write(f"  {r}\n")
    print(f"clean={clean} flagged={len(flagged)} unreadable={len(unreadable)}")


if __name__ == "__main__":
    main()
