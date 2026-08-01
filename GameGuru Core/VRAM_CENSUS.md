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

## FAILED ATTEMPT — grass coverage scaling, REVERTED 2026-08-01 (read before retrying)

An attempt at backlog item 1 shipped and was reverted the same day because it **visibly wrecked
the grass**: dense meadow became "spaced-out bunches" at Grass Density 100. Reverted in game
`5b768138` / `6f4bde82` / `c25fa958` and engine `2b9b989f`. The numbers it produced were real
(Z Island 10207 → 7251 MB in game, −29%) — the grass was simply wrong, so they are worthless.

**What it did:** stamped `vertex_lengths` per grass type by point-sampling the paint map at each
chunk-mesh vertex, then scaled `strandCount` by the resulting kept-triangle fraction.

**Why it broke.** Chunk meshes are 67×67 vertices over a 5280-inch chunk — **one sample every
~80 inches (~2 m)**. The grass paint map is far finer (cells of a few units). Worse, **Match
Terrain Color** — ticked on ordinary levels — auto-resolves grass type *per cell*, so several
types interleave at cell resolution. Point-sampling that at 2 m spacing catches only the
vertices that happen to land on a given type's cells, so the kept-triangle fraction collapses
far below true coverage, strand count collapses with it, and the shader's per-cell mask is left
wanting grass in places where no strands were ever allocated. Hence clumps.

The conservation argument (`N × f_cell` before, `(N × f_tri) × (f_cell / f_tri)` after) is
algebraically fine but **conditional on the mask faithfully representing the painted area** —
the one thing that was never tested.

### What NOT to do next time

1. **Do not derive grass coverage from per-vertex point samples of the paint map.** Chunk
   vertices are ~2 m apart and the paint map is far finer. Any mask built that way
   under-reports coverage, badly, on interleaved paint.
2. **Do not use a mean-colour / average-brightness proxy as proof that density is unchanged.**
   It cannot tell "same amount of grass, redistributed into clumps" from "evenly spread grass" —
   which is precisely the failure that occurred, and precisely why it passed. If placement
   changes, measure *spatial* statistics (per-tile coverage variance / gap-size distribution),
   or look at several viewpoints with fresh eyes.
3. **Do not validate a grass placement change on one camera in one level.** Levels differ
   enormously: hand-painted single-type regions behave nothing like Match-Terrain-Color
   interleaving. Test at minimum one of each, at Grass Density 100, close to the camera.
4. **Do not let a conservation proof stand in for measurement** when it rests on an assumption
   you have not measured. State the assumption, then go and test *that*.
5. **Do not change the emitter mask and `strandCount` in one step without checking each
   independently.** They multiply (f vs f²), so an error in one is easily misread as success.
6. **Do not conclude "painting still works" from the existence of a rebuild path.** Verify the
   rendered result after a real stroke, at cell resolution.

### Safer approaches for the retry

- **Conservative (dilated) mask.** Mark a vertex covered if *any* cell of that type lies within
  its neighbourhood — an area-max, not a point sample. It can only ever keep MORE triangles than
  needed, so density can never drop; savings then appear exactly where a type occupies a
  distinct region, which is where the waste actually is. Safe by construction.
- **One hair system per chunk instead of one per type** (the structural fix). The simulate CS
  already samples the paint map per strand and knows each strand's cell type; the *only* reason
  for N systems is N materials/textures. Drive the blade texture from a texture array indexed by
  that per-strand cell type and one system serves every type in the chunk — a true 1/N saving
  with zero change to placement or density. Bigger job (material/atlas + shader), no density risk.
- **Attack bytes per strand, not strand count.** Position data is ~64% of each 59.5 MB system
  and the raytracing copy is already gone (−20%). `vb_pos[0]/[1]` are forced FP32 for sway
  quality; a narrower-but-not-16-bit format could halve them without touching placement — mind
  the choppy-sway history that forced FP32 in the first place.
- **Build the instrument first.** A per-cell "grass coverage histogram" probe (allocated and
  drawn strands per region) would have caught this in one run. Any future placement change
  should be gated on that, not on screenshots.

## Cross-demo picture after the first two fixes (in-game)

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

## Ranked backlog (not applied)

### THE RETRY: one hair system per chunk (verified feasible 2026-08-01)

**Placement comes out bit-identical — not "statistically equivalent" — which is exactly the
property the failed attempt lacked.** Proof: every per-type system in a chunk already shares the
same `randomSeed` (always 1 — never assigned in Guru-WickedMAX), the same emitter mesh and the
same index list (`vertex_lengths` all-1.0 keeps every triangle), and `strandCount` is per *chunk*,
not per type. So for a given thread id, strand *i* lands on the identical triangle with identical
barycentrics in every type's system. A paint cell holds exactly one type, so the surviving sets
are **disjoint and their union is exactly "strands on painted cells"** — one merged system with
the same `strandCount` renders precisely those same blades. Corollary: today ~75% of strands in a
4-type chunk run full physics and then emit degenerate quads; merging deletes that work.

Six things vary per type — `length`, `width`/aspect, `stiffness`, `drag`, `viewDistance` and the
blade texture — and all fit **without growing any buffer**:
- Carry the per-strand type in **`vb_nor.w`**: `R8G8B8A8_SNORM`, currently written as constant 0
  and read by nothing anywhere (127 codes ≥ 88 types). Do *not* use `vb_uvs.zw` — it has a live
  consumer in `surfaceHF.hlsli` when SSR/SSGI are on.
- Put the per-type scalars in a `HairParticleCB` table. The CB is ~2272 B against a 64 KB limit;
  an 88-entry table adds ~2816 B. DX11 shipped exactly this design (`GGGrassConstants.hlsli`
  `GrassType grass_type[46]`).
- **No texture array needed.** Store each type's bindless SRV index in that table and sample
  `bindless_textures_half4[NonUniformResourceIndex(...)]` — reuses the already-loaded per-type
  textures for zero extra VRAM and no size/format uniformity requirement.

Hazards to respect: `billboardCount` is the one structural difference (2 for most, 1 for
weed/kelp/seaweed) — take `max` over the types present and suppress extra billboards per strand
by collapsing them to zero-area quads, so single-type chunks stay byte-identical; patch **all
four** hair pixel shaders (lit, prepass, prepass-depthonly, shadow) or depth silhouettes come
from the wrong sprite; keep the frustum-cull radius consistent with per-strand length or tall
grass pops at chunk edges; and `xHairAspect` folds in the texture aspect ratio, which is 1.0 for
all 46 stock DDS but not for custom user slots.

**This is mutually exclusive with the reverted coverage scaling** — the identity proof depends on
every system in a chunk keeping the same `strandCount` and all-1.0 `vertex_lengths`.

Expected on the benchmark: 37 tier-3 systems → ~9, grass 1945 MB → ~500 MB, plus an FPS win.
Falsifiable prediction to gate on: coverage 9.5 ±0.15 pp, **clumpCV 0.871 ±0.01**, every band
±0.25 pp — i.e. inside the noise floor. `HAIR_SYSTEMS` and `HAIR_TOTAL_STRANDS` should fall ~4×
while the *rendered* blade count does not change. Any clumpCV rise at all means the identity
argument is broken somewhere: stop.

### Free win found alongside it: `texGrass` is 61 MB of never-written, never-read VRAM

`GGGrass.cpp:1089` allocates a 1024² × 46-slice BC3 array, but **every writer is a stub** —
`GGGrass_LoadTextureDDSIntoSlice` is `{ // stub - DDS loading disabled }` and the real body sits
under `#if 0` ("TODO: DX12 - tinyddsloader removed"). Its only binder, `GGGrass_BindGrassArray`,
is called solely from `GGTerrain_DrawPages`, which lives below the Wicked-mode early return —
i.e. the dead legacy path. Make it lazy behind `GGTerrain_EnsurePageAtlas()` exactly as the
2026-08-01 dead-VT work did. Independent of everything else and trivially verifiable.
(Also noted: the grass `NORMALMAP` textures are never sampled by the hair render path.)

1. **Grass per-type over-allocation, ~4-5× — still the biggest single item in the game.**
   Grass systems are created per (terrain chunk × distinct grass type painted in it), each with
   a flat `STAGE1_STRANDS_PER_TIER = 100000` strands spread over the *whole* chunk; the shader
   then masks out the ~80% that land on cells of another type. Five types in one chunk costs
   5 × 59.5 MB to draw one chunk of grass. On The Mystery of Z Island that is **4.3 GB, 51% of
   the level's entire VRAM**. **One attempt already failed and was reverted — read the FAILED
   ATTEMPT section above before touching this**, in particular the do-not list and the safer
   approaches (dilated mask; or one system per chunk with a per-strand texture-array lookup,
   which carries no density risk at all).
2. **Terrain SVT tile pool, 768 MB fixed** — *measured, wired to a switch, awaiting your soak.*
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
3. **Transparent shadow atlas, RGBA16F → RGBA8: −80 MB** (Island). Cheap, *but* the alpha
   channel carries a depth value compared against the shadow `cmp`
   (`TRANSPARENT_SHADOWMAP_SECONDARY_DEPTH_CHECK` is defined in `objectHF.hlsli`), so 8-bit
   alpha quantises that test. Verify no MAX material actually renders into it first — if
   none does, any format is safe.
4. **Suballocator granularity.** Blocks are 256 MB and are only released when *completely*
   empty, so one pinned allocation can hold a whole block. `SUBALLOC:` reports occupancy.
5. **`texMaterialMap` / `texGrassMap` / `texTreeMap`** — 3 × 16 MB 4096² R8. The GPU copies
   are read only by the dead path (the CPU-side maps are what editing uses).

## Knobs that do NOT reduce VRAM (measured, so nobody re-chases them)

- **Grass Density slider** — hash-thresholds which *cells* get painted. Strand allocation is
  fixed per tier. Zero VRAM effect.
- **Grass Draw Distance / vegetation quality preset** — the near (tier-3) ring is pinned at
  1.5 chunks for every slider value; distance only adds/removes the cheap outer tiers.
- **`SET_GRASS blades` / `SET_GRASS maxstrands`** — dead settings, written but never read.
  The comment block above them in `GGTerrainWicked.cpp` describes a model the code no longer
  uses.
