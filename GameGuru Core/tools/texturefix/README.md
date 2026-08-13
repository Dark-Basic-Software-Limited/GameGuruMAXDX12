# adult female head 15_surface — corrected roughness

## Why

Reported: the Aztec Witch's face is too shiny; assigning a **male** head surface DDS fixes it.
Confirmed — it is the texture, not the material or the shader. Wicked's surfacemap convention is
R=occlusion, G=roughness, B=metalness, A=reflectance.

| map | G (roughness) p1 | G mean | % texels G<40 | R (occlusion) |
|---|---|---|---|---|
| `adult female head 15_surface` (bad) | **34** | **103.5** | **5.82 %** | **flat 255, std 0** |
| `Adult Female head 04_surface` (good) | 56 | 120.3 | 0.00 % | 0–255, std 21.7 |
| `adult male head 16_surface` (good) | 54 | 132.4 | 0.00 % | 0–255, std 57.3 |

Two defects, both absent from every working map:

1. **Roughness is shifted glossy and has a mirror tail.** 5.82 % of texels sit below 40 (the good
   maps have *exactly zero* below 40) and the minimum is 0 — a perfect mirror.
2. **The occlusion channel is unauthored** — literally constant 255 with zero variance, so the head
   receives no ambient occlusion at all.

Note `adult female head 15b` has no surface map of its own; it is a colour variant that reuses this
file. Fixing this one file fixes both heads.

## The correction applied

`adult female head 15_surface_FIXED.png` = the original with **G + 21, clamped to 255**.

A pure offset, chosen so the 1st percentile lands on 55 — inside the 54–56 floor both good maps
share. It preserves every bit of the map's own detail and contrast, removes the sub-40 tail
(5.82 % → 0.01 %), and puts the mean at 124.5, between female04's 120.3 and male16's 132.4.

R, B and A are untouched. **The flat occlusion channel is NOT fixed** — real AO cannot be invented
from a constant, and guessing it would be authoring, not repair. That wants a re-bake.

## ⚠ Why this is a PNG and not the .dds

The original is **DXT5 with 12 mips**. There is no DXT5 compressor on this machine (no texconv,
nvcompress or nvtt), and Pillow writes only *uncompressed, mip-less* DDS — which would quadruple
the file, defeat texture streaming, and alias badly at distance. Shipping that would trade a
shininess bug for a worse one.

So this PNG carries the corrected pixels and needs one pass through the normal texture pipeline:

```
texconv -f BC3_UNORM -m 12 -y "adult female head 15_surface_FIXED.png"
```

then drop the result over
`Files/charactercreatorplus/parts/Adult Female/adult female head 15_surface.dds`
(back the original up first). The source .dds is untouched by this change.
