---
name: project-vram-census
description: "VRAM forensics for GameGuruMAX DX12 (2026-08-01). Engine 1.70 built a per-resource VRAM census (harness DUMP_VRAM; census total == D3D12MA AllocationBytes byte-for-byte = proof of completeness). Island Showdown driver VRAM 8.85 -> 7.06 GB (-20%) with zero visual change by (a) lazily allocating GGTerrain's DEAD custom virtual-texture machinery (883.6 MB page cache + 427 MB source arrays that duplicated the terrain DDS) and (b) gating the grass vb_pos_raytracing copy (write-only without an RT feature). Repo GameGuru Core/VRAM_CENSUS.md holds the line items + ranked backlog; biggest remaining prize is the grass per-(chunk x type) 4-5x over-allocation. Knobs that do NOT save VRAM: grass density slider, grass draw distance, SET_GRASS blades/maxstrands (dead)."
metadata: 
  node_type: memory
  type: project
  originSessionId: 9a28c586-4c13-4447-916e-7fb51301bfa8
  modified: 2026-08-01T17:06:20.101Z
---

**Repo `GameGuru Core/VRAM_CENSUS.md` is the authority** — line items, formulas, ranked
backlog. **Repo `GameGuru Core/VRAM_HUB_SWEEP.md`** = all 19 demos ranked. This is the summary.

## Hub-wide sweep — LATEST 2026-08-02 (engine `a6cb310e` / game `8fd09cb7`)

**Repo `GameGuru Core/VRAM_HUB_SWEEP.md` is the authority** (full table + per-category line
items + census dumps). Streaming ON, merged grass default OFF.

**Range 10023 MB (Z Island) -> 4644 MB (Switch Escape), mean 6060.** Worst four: Z Island
10023, Aztec Game Kit 8069, Jungle Fever 7651, Canyon Offensive 7002. Within ~1% of the
08-01 run, so this is a re-confirmation on the current build, not a change.
- **~1.85 GB floor on EVERY level** (SVT pool 768 + mesh alloc 512 + arrays 220 + chunk maps
  ~106 + SVT bookkeeping ~134). Switch Escape: no grass, 109k polys, still 4.6 GB.
- **Grass is the whole spread: 17.3 GB hub-wide**, 0 on six demos, 4297 MB on Z Island.
- **Transparent shadow atlas 512 MB** on Amazon/Foggy/Disruption/Bounty (160 elsewhere) +
  ~260 MB depth. **Bounty = 772 MB of shadow atlas with NO grass** — the biggest non-grass item.
- **Aztec Game Kit content outlier**: 1114 MB textures + doubled 1024 MB mesh allocator.
- Polygons are no VRAM guide (Aztec Teaser 10.3M polys / 5.7 GB vs Jungle Fever 74k / 7.7 GB).
  Converse also true — FPS tracks polygons, not VRAM ([[project-demo-fps-baseline]]).

⚠ **`GRASS_CHUNKS` phantom fixed 2026-08-02** (`740e9a6c`): it counted map slots without
checking the entity still had a hair component, so grass-free demos reported a fake
`chunks=9 systems=19` while HAIR_SYSTEMS read 0. Now validated against the live scene —
verified Switch Escape 0/0 and Island Showdown 34 == HAIR_SYSTEMS 34.

## SVT atlas — **DEFAULT NOW 12288** (engine `90375285` / game `0daf5692`, 2026-08-02)

**SHIPPED AS DEFAULT at Lee's instruction; he is flying Canyon / Jungle Fever / Aztec at high
camera speed to confirm.** Peak tile demand measured on FOUR levels: Z Island 1971, Canyon 1966,
Jungle Fever 1864, **Aztec 2001**. 12288 = 2852 tiles → 30-35% headroom on all four. Savings:
Canyon 7002→6761, Jungle 7651→7347, Aztec 8069→7926, Z Island 10150→9894.
**Aztec's 2001 peak is above 8192's ENTIRE 1922 capacity** — 8192 was broadly wrong, not unlucky.
setup.ini `svtatlasheight` still overrides (2048..16384); 16384 = stock for A/B.


`tools/svt_soak.sh` (Z Island, 3 laps × 8 waypoints, 5 s dwells so the atlas must evict):

| height | tiles | min free | peak used | headroom | VRAM |
|---|---|---|---|---|---|
| 16384 stock | 3844 | 1873 | 1971 | 49% | 10150 MB |
| **12288** | 2852 | **881** | **1971** | **31%** | **9894 (−256)** |
| 8192 | 1922 | **0** | — | **STARVED** | 9702 (−449) |

**8192 is dead** — peak demand under travel is 1971 tiles, the halved atlas holds 1922, so it
physically cannot fit; `free` hits 0 and terrain visibly blurs (lichen speckle smears).
**⚠ The old "44-46% headroom" note was measured at a STATIC SPAWN and is wrong for travel.**
**12288 is safe and recommended**: peak used 1971 = IDENTICAL to stock, i.e. every request was
served, no eviction pressure, no mip fallback. ~192-256 MB on EVERY level (~3.6-4.8 GB hub-wide).
Not defaulted on yet — soak Canyon Offensive / Jungle Fever / Aztec GK first, since peak demand
is content-dependent and 12288's margin is 31%.

⚑ **TRAP: cross-run screenshot diffs CANNOT judge this.** Atlas height needs a restart, so each
config is a cold launch and the diff is dominated by animation phase — 12288-vs-stock measured
6-57% differing pixels, same order as the known-bad 8192, purely from grass sway/lighting settle.
**Judge from the VT telemetry (`VT: free=`/`tiles=`)**; screenshots only confirm what it says.

## Transparent shadow atlas — **RGBA8 REJECTED on measurement** (2026-08-02, engine `94b19b25`)

The backlog's "RGBA8, ~−3 GB" item is **dead — do not re-pick it.** The atlas alpha carries a
depth value used by `TRANSPARENT_SHADOWMAP_SECONDARY_DEPTH_CHECK` (`transparent_shadow.a > cmp`),
and a new counter (harness `SHADOWT:`) proves it is written on every level tested: Amazon 41k
editor/59k game, Foggy 25k/34k, Disruption 48k/77k, Island 21k/32k, **Bounty 59k/632k**. 8-bit
alpha would quantise that compare, and no 4-byte RGBA format has better than 8-bit alpha.

**But the A/B found something bigger.** `SET_TRANSPARENTSHADOWS 0` skips the draws (atlas clears
to `(1,1,1,0)` = white tint + failing depth check = exactly "feature off"). Same-session A/B:
Bounty 0.023% differing vs 0.021% noise floor; Island Showdown 16.493% vs 16.414%; **Operation
Amazon 3.623% vs a 5.531% noise floor — i.e. BELOW frame-to-frame variance.** No measurable
visual difference on any of three, including the two strongest candidates (Island has ocean;
`FILTER_WATER` feeds the same queue). FPS unchanged, so the cost is memory + the per-frame clear.
**So the prize is the WHOLE atlas (~5.3 GB hub-wide), not RGBA8's half** — but that is a
rendering-feature removal, measured on 3/19 demos in editor only, and user content may use
coloured transparent shadows even where stock demos don't. **DEFAULT ON pending Lee's call + a
full-hub sweep.**

⚑ **Method note:** the gate was proven before the A/B was believed — counter frozen exactly
(72276 → 72276) with it off, climbing again when restored. Without that check both screenshots
could have had the feature live and the whole comparison would have been meaningless.

## TEXTURE STREAMING: crashed two demos, ROOT-CAUSED AND FIXED same day (engine 1.73)

Streaming is **ON by default** (engine `53481336` + game `a0fd128a`). The sweep had caught
**Trapped** and **RPG Template** dying ~15 s into load with an AV inside memcpy, which forced a
temporary default-off (`e900f186`). Full autopsy in [[project-wicked-engine-changes]] 1.73 row
and repo `VRAM_HUB_SWEEP.md`; the short version:

**Both leading suspects were WRONG** — not the streaming thread, not the decrypt/re-encrypt
cycle. The engine's mip reduction halves w/h ignoring block-compression alignment; **500 is a
multiple of 4, 250 is not**, and a BC resource's TOP mip must be block-aligned. D3D12's
`GetCopyableFootprints` refuses such a desc and writes `0xFFFF..` sentinels instead of failing;
upstream's guard `rowSizesInBytes[i] > (SIZE_T)-1` **can never be true on 64-bit**, so memcpy
looped 4-billion rows off the end of the buffer. Victim: `DOOR1_surface.dds` 500×500 DXT1.

**What cracked it in one repro: a symbolized stack walk added to `CrashLogger.cpp`** — it showed
the fault was on the MAIN thread in the INITIAL upload, demolishing the streaming-thread theory
instantly. That instrument is permanent and pays off for every future crash.
**Method lesson worth more than the fix: I spent a long stretch reasoning about which code path
*could* overrun, and every candidate checked out fine. The stack walk + a per-load breadcrumb
settled it in two builds. Reach for the instrument sooner than the theory.**
Also fixed: a texture that can shed no mips used to rebuild itself every streaming pass
(Trapped `replaced=260468` → `12`).

## The instrument (engine 1.70)

`wiGraphicsDevice_DX12.cpp` registers every D3D12MA allocation (texture/buffer/BVH) with
bytes + full desc + debug name; erased in `~Resource_DX12` (which is also the pooled-allocator
recycle point — the key would go stale otherwise). Harness `DUMP_VRAM [tag]` writes the sorted
dump to `Files\vram_census*.txt`; `GET_PERF_DATA` gains `VRAM:` and `SUBALLOC:` lines.
**Validation that matters: census total == D3D12MA's own `CalculateStatistics` AllocationBytes,
byte-for-byte, on every dump.** Driver-reported usage runs 1.3–2.2 GB above the census
(block padding, descriptor heaps, PSO cache, swapchain) — quote the right number.

## What the census found (Island Showdown, in-game, 6655 MB census / 8853 MB driver)

Grass strand buffers 2082 MB (31%) | GGTerrain dead-VT pages 887 | mesh suballocator 768 |
terrain SVT tile pool 768 | source texture arrays 650 | content textures 511 (already streamed
by 1.69) | shadow atlases 241 | chunk maps 168. **An INDOOR level (Zombie Cellar) paid the
same ~2.5 GB of terrain machinery** — these are fixed costs, not content costs.

## Shipped fixes (both zero-visual-change, verified)

1. **GGTerrain custom VT is dead code.** Shipping terrain is Wicked's native SVT
   (`ggterrain_use_wicked_terrain = 1`): `GGTerrain_Update` returns before `GGTerrain_DrawPages`
   (the only page writer) and every `customDraw_*` in master_part1.cpp early-returns.
   The 9520² page cache (883.6 MB) and the 32-slice source arrays (427 MB — the terrain DDS
   files were resident TWICE, once here and once as Wicked material textures) now allocate in
   `GGTerrain_EnsurePageAtlas()`, reached only from the legacy tail (Y-key debug toggle).
   `GGTerrain_LoadTextureDDSIntoSlice` validates-without-uploading when the array is absent,
   which also removes up to 96 2K uploads per level load.
2. **Grass `vb_pos_raytracing`** — a full second copy of every strand position (20% of each
   59.5 MB system) read only by a hair BLAS, which needs a scene TLAS (SurfelGI/DDGI/RT
   shadows/RTAO/RT reflections/lightmap bake). MAX enables none, so ~560 MB/frame was written
   and never read. `wi::gg_hair_raytracing = false` shrinks it to a 4-element scratch with
   **no shader change** — out-of-bounds writes to a TYPED buffer UAV are discarded by hardware.
   Set it true BEFORE level load if RT is ever enabled (CreateRenderData bakes the layout).

Measured: island census 6655→4930, driver 8853→7064 (−20.2%); cellar driver 5976→4462 (−25%);
POLYS bit-identical; screenshot delta below the animation noise floor (mean 2.52/765 vs 3.62
between two frames of one session — always establish that floor before trusting a diff on an
animated scene); terrain paint/sculpt/undo re-verified.

## Do NOT re-chase (measured dead ends)

- **Grass Density slider, Grass Draw Distance, vegetation quality preset**: zero VRAM effect.
  Strand allocation is a flat `STAGE1_STRANDS_PER_TIER = 100000` per tier; density only
  hash-masks which cells draw, and the near ring is pinned at 1.5 chunks for every distance value.
- **`SET_GRASS blades` / `SET_GRASS maxstrands`**: dead settings, written but never read (the
  comment block above them describes a model the code no longer uses).

## Grass coverage scaling — ATTEMPTED AND REVERTED 2026-08-01. READ THIS BEFORE RETRYING.

Shipped and reverted the same day: it **visibly wrecked the grass** (dense meadow → "spaced-out
bunches" at Grass Density 100, user-reported with a screenshot). Reverts: game `5b768138` /
`6f4bde82` / `c25fa958`, engine `2b9b989f`. Its VRAM numbers were real (Z Island 10207→7251 MB)
and worthless, because the grass was wrong.

**Why it broke:** the per-type `vertex_lengths` mask was built by point-sampling the paint map
at each chunk-mesh vertex — **67×67 vertices over a 5280-inch chunk = one sample per ~80 inches
(~2 m)** — while the paint map is far finer AND **Match Terrain Color** (ticked on ordinary
levels) resolves grass type PER CELL, interleaving several types at cell resolution. The mask
caught only vertices that happened to land on a type's cells → kept-triangle fraction collapsed
→ strandCount collapsed → the shader's per-cell mask wanted grass where no strands existed.
The conservation algebra was fine but rested on "the mask represents the painted area", which
was never tested.

**DO NOT, next time:**
1. Do NOT build grass coverage from per-vertex point samples of the paint map (2 m spacing vs a
   much finer map).
2. Do NOT accept a mean-colour/brightness proxy as proof density is unchanged — it cannot
   distinguish redistribution-into-clumps from even spread. That is exactly why it passed.
3. Do NOT validate a placement change on one camera in one level. Test hand-painted single-type
   AND Match-Terrain-Color interleaved, at Grass Density 100, close to camera.
4. Do NOT let a conservation proof substitute for measuring its own premise.
5. Do NOT move the emitter mask and strandCount together without checking each (they multiply:
   f vs f²).
6. Do NOT infer "painting still works" from a rebuild path existing — look at the rendered result.

**Safer retries:** (a) *dilated* mask — vertex covered if ANY cell of that type is in its
neighbourhood (area-max, never a point sample); can only keep MORE, so density can't drop;
(b) **one hair system per chunk instead of one per type** — the CS already knows each strand's
cell type, so a per-strand texture-array lookup removes the only reason for N systems: a true
1/N saving with zero placement risk; (c) attack bytes-per-strand (positions are ~64%) rather
than strand count; (d) build a per-cell grass-coverage histogram probe FIRST and gate on it.

**USER'S STATED DIRECTION (2026-08-01, carry this forward):** the goal is to render the GOOD
benchmark scene with less VRAM and *hardly any* visual reduction, and **Grass Draw Distance is
seen as the great dial** — dense grass up front with sparse distance for low-end machines,
extreme distance for high-end. That is the tier system doing its job; today the tier RINGS are
pinned to fixed chunk radii (1.5 / 2.2) regardless of the slider, so the dial can neither free
resources when lowered nor extend full-density grass when raised (measured: 750 and the 1500
default both give 34 systems / 3.40M strands on Island Showdown).
**Ring scaling is a SEPARATE change from the failed coverage mask and is independently
retryable** — it never touches placement *within* a chunk, only which chunk gets which tier.
It shipped in the same batch as the mask, so it is not cleanly exonerated; re-test it ALONE
against the benchmark before trusting it.

## What the reverted attempt actually did (mechanics only — it is NOT a fix)

Each (chunk × painted type) allocates a flat 100K strands over the WHOLE chunk;
the CS masks out the ~80% on other types' cells. The attempt was **both** halves together: stamp
`vertex_lengths` per type (sample the paint map at each chunk-mesh vertex; CreateFromMesh keeps
a triangle if ANY vertex is set, so it dilates safely) AND scale `strandCount` by the **exact
kept-triangle fraction** using that same any-vertex rule. Strands place uniformly over the kept
triangle list, so f×strands over f×area is identical density; strands-alone gives f², mask-alone
gives 1/f. Paint needed NO new code — `GGGrass_TakeDirtyChunks` already erases the tier record
for painted chunks, and the rebuild re-runs the stamp. Kill switch `SET_GRASS coverage 0`.

Measured in game: **Z Island 10207→7251 MB (−29%), Jungle Fever 7682→5665 (−26%), Island
Showdown 7064→5557 (−21%)**; Z Island editor A/B strands 15.88M→3.24M, VRAM 11821→6844,
FPS 71.4→97.3. **Density proved numerically, not by eye**: mean ground colour G 16.64 (old) vs
16.55 (new) vs 16.54 (same-build control) — an 80% thinning would have moved it hard toward bare
terrain. The 37% pixel delta is the per-strand RNG reseeding blade POSITIONS (strandCount
changed), not density — expect a one-time cosmetic reshuffle of where blades sit.

## Ranked backlog (see VRAM_CENSUS.md for detail)
2. Terrain SVT tile pool 768 MB fixed: **measured only ~1000 of 3844 tiles resident on every
   demo**. Wired to **setup.ini `svtatlasheight=8192`** (engine `wi::terrain::gg_svt_atlas_height`,
   default stock 16384; must be set before the atlas is created on first terrain update, which is
   why a harness command can't do it). Verified: tiles 3844→1922, −368 to −400 MB, FPS unchanged,
   44-46% tile headroom left, VT rebuilds cheaper. NOT default — a static spawn can't exercise
   eviction; needs a fast-travel soak on a big level.
3. Transparent shadow atlas RGBA16F→RGBA8 (−80 MB) — caveat: alpha carries a depth value
   compared against the shadow cmp (`TRANSPARENT_SHADOWMAP_SECONDARY_DEPTH_CHECK`), so 8-bit
   quantises that test unless nothing actually renders into the atlas.
