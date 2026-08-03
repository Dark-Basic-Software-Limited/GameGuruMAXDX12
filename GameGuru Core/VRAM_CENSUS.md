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


## SVT atlas halving — SOAKED 2026-08-02: **8192 FAILS, 12288 is the safe setting**

`svtatlasheight` was carrying "−384 MB × 19, ~7 GB hub-wide, needs a fast-travel soak". The soak
was run (`tools/svt_soak.sh`, Z Island, 3 laps over 8 waypoints with 5 s dwells so the atlas must
evict and re-stream). **The old "44-46% tile headroom" note was measured at a STATIC SPAWN and is
wrong for travel.**

| atlas height | tiles | min free under travel | peak used | headroom | driver VRAM at spawn |
|---|---|---|---|---|---|
| 16384 (stock) | 3844 | 1873 | 1971 | 49% | 10150 MB |
| **12288** | **2852** | **881** | **1971** | **31%** | **9894 MB (−256)** |
| 8192 | 1922 | **0** | — | **STARVED** | 9702 MB (−449) |

**8192 is not viable.** Peak tile demand under fast travel is **1971**, and the halved atlas only
has 1922 — it physically cannot hold the working set. `free` hits **0** at the worst waypoint and
single digits (7, 29, 47, 51) at most others, and the terrain visibly blurs: the lichen speckle
smears and path edges lose definition.

**12288 is safe and is the recommendation.** Its peak usage is **1971 — identical to stock**,
which is the key evidence: every tile request was served, so there is no eviction pressure and no
fallback to lower mips. 881 tiles (31%) still spare at the worst waypoint. Visually it retains
the crisp ground detail that 8192 loses. **Worth ~192-256 MB on EVERY level (~3.6-4.8 GB
hub-wide)** — less than the hoped-for 7 GB, but real and on the floor every project pays.

### Measurement trap found here (do not repeat)

Cross-run screenshot diffs **cannot** judge this. Atlas height needs a restart, so each config is
a separate cold launch, and the pixel diff is then dominated by animation phase (grass sway,
ocean, lighting settle) — 12288-vs-stock measured 6-57% differing pixels, i.e. the same order as
the known-bad 8192, purely from animation. **Judge this from the VT telemetry** (`VT: free=` /
`tiles=` in `GET_PERF_DATA`), where the signal is unambiguous, and use screenshots only for
eyeball confirmation of a result the telemetry already established.

**Still to do before defaulting it on:** soak 2-3 more terrain-heavy levels (Canyon Offensive,
Jungle Fever, Aztec Game Kit). Peak demand is content-dependent, and 12288's margin is 31% on
Z Island — a level with denser terrain paint could need more.

### CONTENT: 967 single-mip DDS = 5.5 GB that can never stream (measured 2026-08-01)

Scanning all **12,561 DDS under `Files\entitybank`** (`tools/ddsscan.py`): **967 files larger
than the 64 KB streaming floor have `mipMapCount = 1`, totalling 5,510 MB.** Texture streaming
cannot touch any of them — the engine rejects single-mip textures outright — so each one sits at
full size in VRAM for as long as its entity is loaded.

This is much bigger than the earlier "39 files (Aztec Witches)" note, which undercounted badly.

The worst offenders are character skins with **no mip chain at all**:

| Size | Dimensions | File |
|---|---|---|
| 64 MB | 4096² | `Aztec Game Kit\Characters\Aztec Priest0.dds` (and `Priest1`) |
| 64 MB | 4096² | `Bonus Assets\Futuristic Girl0.dds`, `Futuristic Guy0.dds` |
| 64 MB | 4096² | `indianstrikeforce\charactercreatorplus\isis soldier0.dds`, `loc 10`, `loc shotgun0` |
| 64 MB | 8192² DXT5 | `junglefever\waterfall 8x8.dds` |
| 32 MB | 8192×4096 DXT5 | `junglefever\splash 8x8.dds` |

Aztec Game Kit alone therefore carries 128 MB of un-streamable, un-mipped character texture —
which also explains part of why it is #2 in the VRAM table at 8.1 GB.

Missing mips are not only a VRAM problem: a 4096² texture with no mip chain **aliases and
shimmers** at any distance, and costs full sampling bandwidth up close and far away alike.
**Authoring mip chains on these 967 files is a content-side change that needs no code**, and it
converts 5.5 GB of fixed cost into demand-adaptive streaming. Highest value per effort in the
whole backlog after grass.

For the full picture the same scan reports the cost of the 1.73 block-alignment fix:
**4.0 MB across all 12,561 files** (21 files affected, 17 of them 500×500 DXT1). The fix is
effectively free.

### ★★★ 2026-08-03 EVENING — MERGED GRASS WORKS. TWO BUGS, BOTH FIXED, −2005 MB.

Awaiting Lee's visual sign-off; `grassmerge` still defaults to 0. Flip = one line in
`GGTerrainWicked.cpp` (`bool gg_grass_merge = false;`) or `setup.ini grassmerge=1`.

| | per-type (shipping) | merged + both fixes |
|---|---|---|
| grass buffers | 2172.1 MB in 58 systems | **198.1 MB in 5** |
| driver VRAM | 4965.0 MB | **2960.4 MB (−2005, −40 %)** |
| strands | 4,816,000 | **418,000 (−91 %)** |
| FPS | 67.2 | **82.5 (+15.3)** |
| POLYS | 2,652,319 | 2,652,319 identical |
| coverage | 4.913 % | 4.671 % (−0.24 pp) |
| clumpCV | 1.083 | 1.022 (**down** = more uniform, not clumping) |
| bands near..far | 5.15 5.80 9.69 5.77 3.03 | 4.96 5.87 9.47 5.41 2.29 |

**Band deltas: −0.19, +0.07, −0.22, −0.37, −0.74.** Near field holds; it thins progressively with
distance. That is the "dense up close, sparse in the distance" curve this document names as the
intended shape, and it is what Lee asked for on 2026-08-03: *"I am happy if the optimization
thinned out the grass more into the distance but I do not want the result you gave last time which
made most of the grass everywhere thin right down."*

Against the **literal** stored gate it is outside on coverage (−0.24 vs ±0.20), on the two far
bands, and on clumpCV magnitude (−0.061 vs ±0.01) — but every miss is in the *thinner / more
uniform* direction, and the gate's clumpCV clause says to "treat any RISE as a failure". Judgement
call, Lee's to make.

#### Bug 1 — the queueing predicate ignored the merged slot (game `c9e7610f`)

`hasExistingEntities` (`GGTerrainWicked.cpp:1812`) scanned only `perType[]`, never `.merged`. In
merged mode `perType[]` is always empty, so it read false forever and a chunk never got the repair
pass that regrows grass after Wicked recycles its entity. 1.74 added the merged slot and updated
every *teardown* path — there is a comment saying so — but missed this one *queueing* site.
Result: 9 systems created, 9 killed by normal recycling, never rebuilt. **Zero grass, not
over-dense grass.**

Named by the engine 1.85 hair-kill tracer in one run: 9 removals, **all reason=1** (recursive child
of the `Component_Attach`ed chunk), `Scene::Clear` wiped 0, grass teardown `fullResets=0`. Per-type
survives the same 574 recycles precisely because that predicate can see its entities.

#### Bug 2 — cap vertices ignored the billboard collapse (engine `55bee4d7`, 1.86)

A merged system carries `billboardCount` = the MAX across the chunk's types, and a strand whose own
type wants fewer must collapse the surplus to zero area. The ROOT half of each segment
(`vertexID 0..1`) did. The CAP half (`vertexID 2..3`, the loop at
`hairparticle_simulateCS.hlsl:569`) had **no `billboardID >= gg_billboards` guard at all** — so
every surplus billboard drew a zero-width root with a **full-width cap**: a wedge with real screen
area instead of nothing. Coverage 6.185 → 4.671 pp on that one guard.

That is the over-density the gate had been failing on since 08-02, and it explains why clumpCV was
always *clean*: nothing was being redistributed, there was simply extra geometry.

#### OPEN: scene-wide grass flicker in merged mode (2026-08-03 night)

Lee reported it against a zoomed screenshot: small square patches of what look like other grass
types' texture inside a blade, moving/vanishing frame to frame, across all grass. **Quantified,
merged-only:**

| consecutive-frame diff | pixels differing | **meanAbsDiff** |
|---|---|---|
| per-type | 6.2–8.3 % | **0.39–0.58** / 765 |
| merged | 17.5–19.5 % | **12.31–12.49** / 765 |

Same wind, same sway, same camera — **21–32× more per-frame churn**.

**It is a constant noise FLOOR, not alternation and not animation.** Diffing frame 0 against
1/2/3/4 gives merged 12.31 / 12.57 / 12.55 / 12.94 (flat) while per-type climbs
0.39 / 0.65 / 0.91 / 1.31 (accumulating, i.e. the grass swaying). A flat floor independent of
temporal distance means something is **re-randomised every frame**. Double-buffering is therefore
out — a ping-pong bug would show a LOW frame-0→2 diff.

**Eliminated so far:**
- *Atlas-rect bleeding* — dead by inspection, no build spent. GG clears `atlas_rects` for every
  grass appearance (`GGTerrainWicked.cpp:1236/1345`, "each material is a single-sprite DDS — no
  atlas"), so `wiHairParticle.cpp:576-582` gives every type identity `texMulAdd (1,1,0,0)`, rect
  count 1. There is no neighbouring tile to bleed from.
- *Surplus billboard slots* (the leading theory) — `SET_GRASS billboards 1` makes every type want
  one billboard, so merged's max-across-types stride has no surplus at all. Flicker persists at
  **11.5–11.7**. **CAVEAT: `HAIR_BILLBOARD` was not read back, so it is not confirmed the live
  systems rebuilt with the new count.** Re-verify before treating this as closed.

**Eliminated by inspection, no builds spent (record these so nobody re-walks them):**
- *Uninitialised per-type table.* `wiHairParticle.cpp:625-628` explicitly zero-fills
  `xHairGrassTypes[t]` for every entry past what the caller supplied, so there is no stale
  constant-buffer region for a resolved type to index into.
- *`grass_type` used as an out-of-bounds index.* Both sites that do it
  (`GGTerrainWicked.cpp:1401` and `:1464`) are guarded with
  `if (typeIdx >= GGGRASS_TOTAL_REAL_TYPES) continue;`, and merged's sentinel 0xFFFFFFFF fails
  that test cleanly. **But note the side effect: merged systems are therefore SKIPPED by both
  live-update loops**, so `viewDistance` and `length` edits — including the shipped low-VRAM
  draw-distance lever — never reach a merged system. Real gap, separate from the flicker.

**FAILED EXPERIMENT, 2026-08-03 — do not repeat it this way.** Forcing every type's geometry
parameters uniform (to make type resolution unable to change a strand's shape) was run with
`SET_GRASS segments 1` and `billboards 2` included. Those two determine the **vertex buffer
stride**, and pushing them onto live systems updates the component in place without reallocating
the buffers. Frame-to-frame churn rose to **31.9–33.6** — my own test corrupting the geometry, not
the bug. The run proves nothing about the flicker.

It did establish two useful things:
1. **The readback method works and the params DO apply live** — `HAIR_SEG: 1  HAIR_BILLBOARD: 2
   HAIR_VIEWDIST: 4747` came back changed, which is the verification the earlier billboards test
   lacked. Always read `HAIR_SEG`/`HAIR_BILLBOARD` back after a `SET_GRASS`.
2. **`HAIR_LEN` read 63.5 after `SET_GRASS length 260`** — it did not take, or is rescaled
   somewhere. Unexplained; worth a look on its own.
3. **A lead worth chasing:** if writing parameters in place on a merged system (without a rebuild)
   reliably produces exactly this class of per-frame churn, then the question becomes *what in the
   normal frame loop writes merged systems in place?* The two type-indexed loops skip them — but
   anything that does not index by type does not.

**Next candidate, still untested: per-strand `gg_resolved_type` instability.** It alone would explain all
three symptoms — a strand flipping type between frames changes its texture (squares of another
type's blade), its length/width/viewDistance (appear/disappear), and would re-randomise every
frame (the flat floor). The instrument is a debug UAV histogram of resolved type per frame: if the
counts jitter frame to frame with a static camera, that is the bug.

**Do NOT trust `SET_GRASS <param>` A/Bs without reading the value back** — the same lesson
`mablockmb` taught. An inert knob and an exonerated theory are indistinguishable.

#### Method note

Three source-read theories died on this before either bug was found (parenting, `Scene::Clear`,
the grass teardown paths). Both bugs fell to instruments that **echo state back** — the hair-kill
tracer recording `_ReturnAddress()` plus the recursive parent, and `GRASS_MERGE:` echoing the live
flag alongside its per-exit counters. The indirect-draw-args readback this document previously
nominated as the next step was **not** needed and would have misled: `visible` at simulateCS:288
excludes `strand_length`, so paint-masked zero-area strands contribute their full index range.

### ⚠ Superseded — the "no grass at all" measurement that led to bug 1

Re-measured on the current build (engine `26fcc5c5`) against the resaved TESTPRO1, via a new
setup.ini key `grassmerge=1` (cold launch, so the reload path is not inside the measurement):

| | grassmerge=0 | grassmerge=1 |
|---|---|---|
| HAIR_SYSTEMS | 58 | **0** |
| grass buffers | 2172.1 MB | **0.0 MB** |
| driver VRAM | 4965.0 | 2761.7 |
| coverage | 4.913 % | **0.020 %** |
| POLYS | 2,652,319 | 2,652,319 (identical — see the POLYS warning in GRASS_BENCHMARK.md) |

**This is not the "uniformly too dense" failure documented below — there is no grass whatsoever.**
Something regressed between 2026-08-02 and now.

Engine/game 1.84 added `GRASS_MERGE:` to `GET_PERF_DATA` — per-exit counters inside
`ProcessGrassChunkMerged` plus **the live flag value** (the `mablockmb` lesson: an inert knob and
a broken feature are indistinguishable from the effect alone). It reads:

```
GRASS_MERGE: flag=1 calls=9 notypes=0 reused=0 nomat=0 created=9
             | fullResets=0 recycles=567 recreates=9 deadMeshNow=0
```

So, established:

- the flag **binds correctly** (`flag=1`) — not an ordering trap this time
- `ProcessGrassChunkMerged` runs (`calls=9`) and takes **no early exit** (notypes/reused/nomat all 0)
- it **creates 9 merged hair entities** (`created=9`, `recreates=9`)
- **the grass code removes none of them** (`fullResets=0`)
- yet `HAIR_SYSTEMS` is 0, and stays 0 across probes 12 s apart — so they are destroyed once,
  early, by something outside the grass teardown paths, and the chunk is never re-queued

**Eliminated so far:** parenting is NOT the difference — both the merged path
(`GGTerrainWicked.cpp:1681`) and the per-type path (`:2052`) do the same
`Component_Attach(grassEntity, pc.entity, true)`, so child-of-a-recycled-chunk cannot explain why
only merged dies. `recycles=567` confirms chunk recycling is heavy on this scene, and the
per-type path survives it.

**Next step is the next instrument, not a theory**: count entity destruction at the source —
trace who removes these 9 entities (an ECS-level removal hook or a breadcrumb on the merged
entity IDs), rather than guessing which subsystem it is.

### The 2026-08-02 measurement, kept for the record: BUILT, WORKED, **FAILED THE DENSITY GATE**

Implemented end to end (engine 1.74 + game). Harness `SET_GRASSMERGE <0|1>`, then reload the
level. **Default is 0 and must stay 0 until the gate passes.**

**The premise was verified first, and it was better than predicted.** A new
`GRASS_CHUNKS:` line in `GET_PERF_DATA` reports the per-chunk type histogram:

```
GRASS_CHUNKS: chunks=5 systems=52  types_per_chunk 7:2 8+:3  merged_would_be=5
```

Five chunks carry all 52 systems — 7 to ~13 painted types each, so the per-type split was
allocating and simulating ~10x what it drew, not the 4x the original plan assumed.

**What it delivers (measured on the TESTPRO1 benchmark):**

| metric | per-type (shipped) | merged | delta |
|---|---|---|---|
| HAIR_SYSTEMS | 52 | **5** | −90% |
| HAIR_TOTAL_STRANDS | 4,216,000 | **418,000** | −90% |
| driver VRAM | 6536 MB | **4969 MB** | **−1567 MB (−24%)** |
| editor FPS | 60.0 | **75.0** | +25% |
| POLYS | 2,714,197 | 2,714,197 | identical |

**Why it is not on: the density gate fails.** Coverage **10.96% vs 9.40%** (+1.56 pp against a
±0.15 limit — 10x over). Bands show near denser and far sparser:
`17.03 19.03 18.71 8.25 2.67` vs `13.74 14.61 16.72 7.98 3.16`. clumpCV is fine (0.863 vs 0.873),
so it is NOT the clumping failure mode of the reverted 2026-08-01 attempt — the grass is
uniformly *too dense*, which the reference screenshots confirm by eye (merged blades read
shorter and more numerous).

**Hypotheses tested and ELIMINATED — do not re-chase these:**
1. *Texture aspect dropped from the width term.* `xHairAspect = width x texW/texH`, and the
   merged path uses per-type `width` alone. **Measured all 52 stock grass DDS: every one is
   square**, so the aspect factor is 1.0 and this changes nothing.
2. *Types present in a cell but absent from the chunk scan render in merged mode but not
   per-type.* Added a `present` flag so absent types are killed exactly as before. **No effect**
   (10.85 -> 10.96, inside noise), so the scan was already complete.
3. *Strand-LOD coupling.* This one was REAL and is fixed: the host computes the LOD step
   distances from the SYSTEM viewDistance, which merged mode sets to the max across types, while
   GG deliberately halves viewDistance for FLOWER. The shader now rescales per strand to its own
   type's view distance. Worth 11.19 -> 10.85 pp, but not the whole gap.

**The next step is an INSTRUMENT, not another theory.** Visible strand count should be identical
by construction (strand i occupies the same position in every per-type system; a cell holds one
type; so the union of what N systems drew is what the merged one draws). Coverage is 16% higher
with the same strand count, which means either that identity is false or the blades are bigger.
**Read back the hair indirect-draw args to get the actual drawn index count per system** and
compare the two modes. That single number decides count-vs-size and ends the guessing — the same
lesson that cracked the streaming crash in two builds after hours of wrong theory.

**Safety property, verified:** with the switch off the shipped path is untouched — 52 systems,
4,216,000 strands, coverage 9.376 / clumpCV 0.873 / bands within 0.1 pp of the pre-change
baseline. Nothing about the default build changed.

### Original design note (still accurate for the placement argument)

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
