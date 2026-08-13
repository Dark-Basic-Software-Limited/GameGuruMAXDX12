# ★ MILESTONE (FINAL, POST-MAX): convert every DDS in the stock MAX product

**Status: NOT STARTED — deliberately deferred until MAX itself is finished.**
**Owner: Lee. Do not start this without him — it is a bulk content mutation.**

This is the last deliverable of the DX12 port, and it is a *content* pass, not a code pass. It is
recorded here because it is easy to forget: nothing in the codebase will fail a build or a test if
it never happens, and the symptoms it fixes look like unrelated one-off art bugs.

---

## 1. Convert ALL stock DDS

Every DDS shipped in the stock MAX product needs to go through the conversion pass. Scope is the
whole content tree, not a sampled subset:

```
D:\DEV\BUILD\GameGuru Wicked MAX Build Area\Max\Files\
```

Notably includes `charactercreatorplus\parts\` (85+ surface maps in `Adult Female` alone),
`entitybank`, `imagebank`, `terrainbank`, `gamecore`.

### Why it matters — evidence already collected

* **`tools/ddsscan.py`** already reports the two properties that decide whether a DDS behaves:
  **single-mip files** (can never stream at all) and **block-compressed files whose dimensions stop
  halving early**. Run it across the tree first to size the job — it produces the work list.
* Texture streaming is currently **DEFAULT OFF** (task #37) partly because of content-side
  surprises. A clean, uniformly-converted content tree is a precondition for revisiting that.
* The VRAM campaign's remaining work is **floor reduction** (`VRAM_FLOOR.md`), and mip-complete,
  properly block-compressed textures are exactly what makes the streaming floor achievable.

### Acceptance gate

Re-run `tools/ddsscan.py` after the pass: **zero single-mip block-compressed files**, and no file
whose mip chain stops early. Then a 19-demo sweep with the standard gates (all levels load, VRAM
under 4 GB, POLYS unchanged).

---

## 2. Fold in the female head surface repair (GGMAX 2.40)

Do **not** let this get lost as a separate errand — it needs the same texconv pass, so it should
ride along with the bulk conversion.

**File:** `Files\charactercreatorplus\parts\Adult Female\adult female head 15_surface.dds`
**Corrected pixels:** `GameGuru Core\tools\texturefix\adult female head 15_surface_FIXED.png`
**Recipe + full analysis:** `GameGuru Core\tools\texturefix\README.md`

```
texconv -f BC3_UNORM -m 12 -y "adult female head 15_surface_FIXED.png"
```

Measured defect (Wicked surfacemap = R occlusion, G roughness, B metalness, A reflectance):

| map | G p1 | G mean | % texels G<40 | R (occlusion) |
|---|---|---|---|---|
| female head 15 (bad) | **34** | **103.5** | **5.82 %** | **flat 255, std 0** |
| female head 04 (good) | 56 | 120.3 | 0.00 % | 0–255, std 21.7 |
| male head 16 (good) | 54 | 132.4 | 0.00 % | 0–255, std 57.3 |

The PNG fixes the roughness (G +21, sub-40 tail 5.82 % → 0.01 %). ⚠ It does **NOT** fix the
occlusion channel — that is unauthored (constant 255) and needs a **re-bake**, which is authoring,
not repair. Decide at conversion time whether to re-bake AO or ship neutral.

★ `adult female head 15b` has **no surface map of its own** — it reuses head 15's. One file, two
heads.

### Worth checking during the pass

The flat-255 occlusion channel is unlikely to be unique to one head. **Scan every `_surface.dds`
for degenerate channels** (std 0) while the tree is already being walked — that is nearly free at
that point and would name any sibling assets with the same defect. A one-line extension to
`ddsscan.py` covers it.

---

## Why this is deferred

Converting content while MAX itself is still changing means doing it twice, and a bulk mutation of
the stock product is not something to run mid-development. The correct sequence is: finish MAX →
freeze content → convert → gate → ship.
