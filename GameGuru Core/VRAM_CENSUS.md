# VRAM census — where GameGuruMAX DX12 video memory actually goes

Instrument + findings from the 2026-08-01 deep-dive. The census is permanent tooling:
re-run it any time VRAM is in question.

## The instrument (engine delta 1.70)

`wiGraphicsDevice_DX12.cpp` keeps a live registry of **every D3D12MA-backed allocation**
(texture / buffer / raytracing structure). Each record carries the allocated byte count,
the full desc (dimensions, format, mips, array size, bind/misc flags, heap usage) and the
debug name, filled in when `SetName` arrives — so content textures show their source file
path and engine internals show their Wicked name. Records are erased in `~Resource_DX12`,
which is also the pooled-allocator recycle point, so keys can never go stale.

**Validation:** the census total equals D3D12MA's own `CalculateStatistics().Total.Stats.AllocationBytes`
byte-for-byte on every dump taken so far. Nothing is missed or double counted. Aliased
resources record 0 bytes (they share a donor's memory) and are flagged; sparse textures
record 0 (their memory is the tile pool, which is itself counted).

| Command | What it gives you |
|---|---|
| `DUMP_VRAM [tag]` | Full per-resource dump, largest first → `Files\vram_census[_tag].txt`. Header reconciles census vs D3D12MA vs driver-reported usage. |
| `GET_PERF_DATA` → `VRAM:` | census MB, default-heap MB, resource count, driver usage/budget |
| `GET_PERF_DATA` → `SUBALLOC:` | mesh-data suballocator blocks: total / free / used |

Dump line format (space separated, name last and quoted, so `awk` works):
`kind bytes w h d mips arr samples fmt bind misc usage alias "name"` — kind is `T`exture,
`B`uffer or `A`cceleration structure; `usage` 0 = DEFAULT heap (true VRAM), 1 = UPLOAD,
2 = READBACK. Analysis helpers live in the session scratchpad (`vram_categorize.sh`).

**Read the driver number, not just the census.** `driver_usage` runs ~1.3–2.2 GB above the
census: D3D12MA block padding (reported separately as `d3d12ma_blocks`), descriptor heaps,
command allocators, the swapchain, and the PSO cache — none of which are resource
allocations. PSO count is reported as a proxy (`gg_dbg_pso_creates`).

## Baseline measurements (2026-08-01, 1536x864 internal res)

Census totals, in-game unless noted:

| Demo | census | driver usage |
|---|---|---|
| Island Showdown | 6655 MB | 8853 MB |
| Horseshoe Bend | 4607 MB | 6650 MB |
| Zombie Cellar (editor) | 4598 MB | 5976 MB |

Island Showdown line items:

| MB | % | Item |
|---|---|---|
| 2082 | 31.3 | grass strand buffers (`HairParticleSystem::generalBuffer`) |
| 887 | 13.3 | **GGTerrain custom-VT physical pages — dead path** |
| 768 | 11.5 | mesh-data suballocator blocks (256 MB granularity) |
| 768 | 11.5 | engine terrain SVT tile pool (fixed 16384² × 4 maps, sparse) |
| 650 | 9.8 | GGTerrain/grass/tree source texture arrays (**427 MB of it dead path**) |
| 511 | 7.7 | content textures (already mip-streamed, delta 1.69) |
| 241 | 3.6 | shadow atlases (160 transparent RGBA16F + 81 depth D32) |
| 168 | 2.5 | terrain chunk blendmaps / wetmaps / heightmaps (841 chunks) |
| 134 | 2.0 | terrain SVT bookkeeping buffers |
| 65 | 1.0 | skinned mesh streamout |
| ~65 | 1.0 | render targets + postFX chain |

**The fixed terrain cost lands on every level regardless of content.** Zombie Cellar is an
indoor level and still paid 883.6 MB of dead page cache + 768 MB SVT pool + 650 MB source
arrays + 187 MB chunk/bookkeeping data ≈ **2.5 GB of terrain machinery for a cellar**.

## Fixes applied 2026-08-01

### 1. GGTerrain custom virtual-texture machinery is dead — allocate it lazily (−1.31 GB)

The shipping terrain is Wicked's native SVT path (`ggterrain_use_wicked_terrain = 1`).
`GGTerrain_Update` returns before `GGTerrain_DrawPages` (the only writer of the page cache),
and every `customDraw_*` callback registered in `master_part1.cpp` early-returns on the same
flag — `GGTerrain_Draw_EnvProbe` additionally has an unconditional `return` for a DX12
deadlock. So nothing renders into or samples:

- `texPagesColorAndMetal` + `texPagesNormalsRoughnessAO` — 9520² RGBA8 ×2 = **883.6 MB**
- `texColorArray` / `texNormalsArray` / `texSurfaceArray` — 2048² × 32 slices = **427 MB**

Both groups now allocate in `GGTerrain_EnsurePageAtlas()`, called only from
`GGTerrain_Update`'s legacy tail — i.e. only if the **Y-key debug toggle** flips
`ggterrain_use_wicked_terrain` at runtime (the sole other writer of that flag; it is not
settable from setup.ini or a level file). `GGTerrain_DrawPages` also hard-guards on the
atlas being valid, and the atlas creation re-arms `g_iDeferTextureUpdateToNow` so the
legacy path repopulates its source arrays on the next update.

`GGTerrain_LoadTextureDDSIntoSlice` validates the DDS header and reports failures exactly as
before when the destination array is absent, but skips the staging texture and copies —
so this also removes up to **96 2K texture uploads per level load**.

Note the source textures were resident **twice**: once in these arrays for the dead path,
once as individual `terraintextures/matN/*.dds` resources that Wicked's terrain bakes into
its own atlas.

### 2. Grass raytracing position copy is write-only — gate it (−12.2 MB per grass system)

`HairParticleSystem::generalBuffer` packs ten regions; `vb_pos_raytracing` is a **full second
copy of every strand position** (20% of the allocation) that only the hair BLAS consumes.
The BLAS is only built when the scene has a TLAS — SurfelGI, DDGI, RT shadows, RTAO, RT
reflections or a lightmap bake — and MAX enables none of them. On Island Showdown that was
~0.5 GB of memory written every frame and never read.

Engine flag `wi::gg_hair_raytracing` (default **false**) shrinks the region to a 4-element
scratch. The simulate CS still writes `vertexBuffer_POS_RT` unconditionally, but it is a
**typed** buffer UAV (`RWBuffer<float4>`) and out-of-bounds typed-UAV writes are discarded by
the hardware — so no shader change is needed. `CreateRaytracingRenderData` refuses to build a
BLAS from the scratch-sized region. If an RT feature is ever enabled, set the flag **before**
level load: `CreateRenderData` bakes the buffer layout.

## Measured result of the two fixes (driver-reported VRAM, in-game unless noted)

| Demo | before | after | saved |
|---|---|---|---|
| Island Showdown | 8853 MB | 7064 MB | **−1789 MB (−20.2%)** |
| Horseshoe Bend | 6650 MB | 5352 MB | −1298 MB (−19.5%) |
| Zombie Cellar (editor) | 5976 MB | 4462 MB | −1513 MB (−25.3%) |
| Switch Escape | 5968 MB | 4671 MB | −1297 MB (−21.7%) |

Verification per demo: POLYS bit-identical, and the before/after screenshot delta sits at or
below the animation noise floor. **Always establish that floor before trusting a screenshot
diff on an animated scene** — two frames 4 s apart in one session on Island Showdown differ by
5.9% of pixels / mean 3.62 per 765, while before-vs-after differed by mean **2.52**, i.e. less
than the scene's own churn. Cellar and Horseshoe came in at mean 0.18 and 0.07.

FPS moved only where grass exists, which is exactly what the raytracing-copy fix predicts
(that buffer was being written every frame): Island Showdown 71.5 → 88.9 in-game and 62.2 →
76.3 in the editor; Horseshoe Bend, which has no grass systems at all, stayed at 68 → 69.

## Cross-demo picture on the fixed build (in-game)

| Demo | census | driver | grass buffers | grass share |
|---|---|---|---|---|
| The Mystery of Z Island | 8351 MB | **10207 MB** | 4297 MB | **51%** |
| Jungle Fever | 5765 MB | 7682 MB | 2403 MB | 42% |
| Island Showdown | 4930 MB | 7064 MB | 1659 MB | 34% |
| Operation Amazon | 4850 MB | 6632 MB | 963 MB | 20% |
| Snowy Mountain Stroll | 3449 MB | 5491 MB | 333 MB | 10% |
| A Grand Canyon Adventure | 3353 MB | 5229 MB | 506 MB | 15% |
| Horseshoe Bend | 3301 MB | 5352 MB | 0 | 0% |
| Zombie Cellar | 3083 MB | 4462 MB | 816 MB | 26% |
| Switch Escape | 2613 MB | 4671 MB | 0 | 0% |

The spread is almost entirely grass. Everything else is now within a few hundred MB of a
fixed floor (~2.4 GB of engine/terrain machinery + content). **Z Island would have been
~12.8 GB of driver VRAM before tonight's fixes** — over budget on an 8 GB card — and it is
still the level to worry about, because half of it is one over-allocating system.

## Grass Draw Distance — what was wrong, and what changed (2026-08-01)

User report: moving the slider from 750 to 7000 does not change how far grass renders. It is
worse than that — measured on Island Showdown with `SET_GRASS drawdist`:

| slider | grass systems | strands | note |
|---|---|---|---|
| 750 | 34 | 3.40M | **identical to default — lowering it freed nothing** |
| 1500 (default) | 34 | 3.40M | |
| 7000 | 88 | 3.83M | +54 systems, 87 → 56 FPS, screenshots pixel-identical |

Cause: the density rings were hard-coded at 1.5 / 2.2 chunks whatever the slider said. Only
the per-strand cull radius and the sparse outer shell moved. So lowering the slider could not
shrink the full-density ring (`nearC = min(1.5, outerC)` and `outerC` never drops below ~1.6),
and raising it bought 54 extra chunk systems at the 5% / 18% tier densities — about one blade
per 3.6 m² spread over a 134 m chunk, invisible past 200 m, yet each still costs a full
simulate dispatch every frame.

Fix (game `GGTerrainWicked.cpp` + engine 1.72 `hairparticle_simulateCS.hlsl`):
- Both density rings now scale with the slider, **anchored at its default so the shipped 1500
  look is unchanged** (verified: same 34 systems / 3.40M strands, screenshot delta below the
  animation noise floor). Clamped to 0.75–2.0 chunks near / up to 3.0 mid.
- A **graceful fade**: strand length now tapers smoothly to zero across the outer 12% of the
  draw distance, so blades sink out of view instead of the cull switching them off. Done in the
  simulate CS so it applies identically to the colour pass, the prepass and shadows, with no
  alpha blending; it overlaps the existing 20% dither ramp in the vertex shader.

Measured after (Island Showdown, in game):

| slider | strands | driver VRAM | vs default |
|---|---|---|---|
| 750 | 1.35M | 5716 MB | **−1139 MB** |
| 1500 | 3.40M | 6856 MB | unchanged from before the fix |
| 7000 | 5.36M | 7554 MB | +699 MB, now spent on full-density grass |

**Honest limit of the verification:** I could not produce a screenshot where grass visibly
extends further at 7000, because on every demo tried (Island Showdown, Jungle Fever) grass is
painted in near-field patches — a same-setting control diff matched the 750-vs-7000 diff
exactly, i.e. all of it was grass sway. The resource behaviour is proven by the strand/VRAM
numbers above; the visible-range benefit is reasoned from the mechanism (full-density ring
7920 → 10560 inches, cull 4000 → 9500 inches) and still wants eyes on a level with grass
painted into the distance.

## Grass per-type over-allocation — FIXED 2026-08-01 (was backlog #1)

Grass systems are created per (terrain chunk × distinct grass type painted in it). Each one
used to get a flat `STAGE1_STRANDS_PER_TIER = 100000` strands spread over the **whole** chunk,
with the simulate CS masking out the ~80% that landed on another type's cells. Five types in
one chunk therefore cost 5 × 59.5 MB to draw one chunk's worth of grass.

The fix needs **both** halves — either one alone is wrong:
- Stamp `vertex_lengths` per type (sample the paint map at each chunk-mesh vertex) so
  `CreateFromMesh` only emits triangles covering that type. It keeps a triangle when ANY of its
  three vertices is set, so the mask dilates by up to one vertex ring (~80 in) — always in the
  safe direction.
- Scale `strandCount` by the **exact kept-triangle fraction**, computed with that same
  any-vertex rule. Strands are placed uniformly over the kept triangle list
  (`rng.next_uint(triangleCount)` in the simulate CS), so f × strands over f × area is
  identical blades per square inch.

Why both: scaling strands *without* restricting triangles yields f² of the original density
(f× fewer strands, of which only f× land on the type); restricting triangles *without* scaling
strands raises density by 1/f. Visible blade count is conserved exactly — before `N × f_cell`,
after `(N × f_tri) × (f_cell / f_tri)`.

Paint needs no new machinery: `GGGrass_TakeDirtyChunks` already erases the tier record for
chunks the brush wrote into, forcing those grass entities to rebuild — and the rebuild re-runs
the stamp, so newly painted cells get grass exactly as before.

Kill switch `SET_GRASS coverage 0` (needs a grass rebuild). Measured A/B on The Mystery of
Z Island, editor, same camera:

| | coverage OFF (old) | coverage ON (new) |
|---|---|---|
| strands | 15,876,000 | **3,236,381 (−80%)** |
| census | 9735 MB | **5390 MB** |
| driver VRAM | 11,821 MB | **6844 MB (−4977 MB, −42%)** |
| FPS | 71.4 | **97.3** |

Density was verified numerically rather than by eye: the mean colour of a ground band reads
R 18.75 / G 16.64 / B 8.37 on the old code and R 19.01 / G 16.55 / B 8.47 on the new — inside
the variation between two shots of the *same* build (R 19.02 / G 16.54 / B 8.46). An 80%
density loss would have shifted the ground decisively toward bare terrain. The whole-frame
pixel delta is large (37%) purely because changing `strandCount` reseeds the per-strand RNG, so
blades land in different spots: a one-time cosmetic reshuffle, not a thinning.

## Ranked backlog (not applied)

1. **Terrain SVT tile pool, 768 MB fixed** — *measured, wired to a switch, awaiting your soak.*
   The atlas is 16384² × 4 sparse map types and its pool is fully committed regardless of use.
   **Measured residency: `VT:` reports 2758–2954 free of `tiles=3844` on every demo — Island
   Showdown, Horseshoe Bend, Zombie Cellar and Switch Escape all settle at ~890–1090 resident
   tiles, about 26% of the atlas.**

   Set **`svtatlasheight=8192` in setup.ini** to halve it (stock 16384; the value must be read
   before the atlas is created on the first terrain update, which is why it is a setup.ini key
   and not a harness command — engine `wi::terrain::gg_svt_atlas_height`).

   Measured on Island Showdown at 8192: driver VRAM **7064 → 6664 MB (−400 MB)**, FPS 88.9 →
   89.0, POLYS bit-identical, resident tiles unchanged at 1076 with 846 still free (44%
   headroom), and the VT free-list rebuild got cheaper (cumulative sort 157 → 32 ms, scan
   36 → 10 ms). Terrain crops are visually indistinguishable. **The caveat that stops me
   shipping it as the default:** the whole-frame pixel delta (20.3% of pixels, mean 4.06/765)
   sits above the run-to-run noise floor (7.7%, mean 3.19), so something does differ slightly.
   A static spawn point cannot exercise the real risk anyway — **soak it with fast camera
   travel across a big level and watch for terrain-detail pop as tiles evict.**
2. **Transparent shadow atlas, RGBA16F → RGBA8: −80 MB** (Island). Cheap, *but* the alpha
   channel carries a depth value compared against the shadow `cmp`
   (`TRANSPARENT_SHADOWMAP_SECONDARY_DEPTH_CHECK` is defined in `objectHF.hlsli`), so 8-bit
   alpha quantises that test. Verify no MAX material actually renders into it first — if
   none does, any format is safe.
3. **Suballocator granularity.** Blocks are 256 MB and are only released when *completely*
   empty, so one pinned allocation can hold a whole block. `SUBALLOC:` reports occupancy.
4. **`texMaterialMap` / `texGrassMap` / `texTreeMap`** — 3 × 16 MB 4096² R8. The GPU copies
   are read only by the dead path (the CPU-side maps are what editing uses).

## Knobs that do NOT reduce VRAM (measured, so nobody re-chases them)

- **Grass Density slider** — hash-thresholds which *cells* get painted. Strand allocation is
  fixed per tier. Zero VRAM effect.
- **Grass Draw Distance / vegetation quality preset** — the near (tier-3) ring is pinned at
  1.5 chunks for every slider value; distance only adds/removes the cheap outer tiers.
- **`SET_GRASS blades` / `SET_GRASS maxstrands`** — dead settings, written but never read.
  The comment block above them in `GGTerrainWicked.cpp` describes a model the code no longer
  uses.
