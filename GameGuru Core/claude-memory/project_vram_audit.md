---
name: project-vram-audit
description: "Categorised 19-demo VRAM audit of GameGuruMAX DX12 (2026-08-02) that decides WHICH KNOBS to give low-end users. Base cost is ~3.3 GB before any asset and barely varies; grass is the entire spread (0-4297 MB); TREES ARE A NON-ISSUE at 63-85 MB flat so a tree VRAM knob would be theatre; nonres 836 MB is nearly all pipeline state objects. The shipped low-VRAM preset reaches only 5 of 19 demos inside 4 GB, and the blocker is FLOOR not content - Bounty has zero grass and still misses. Repo GameGuru Core/VRAM_AUDIT.md holds the table, the projections and the Tier A/Tier B knob plan."
metadata:
  type: project
---

**Repo `GameGuru Core/VRAM_AUDIT.md` is the authority** (full table + projections + knob plan).
Tools committed: `tools/vram_audit.sh` + `tools/vram_audit.awk`. This is the summary.
Companions: [[project-vram-floor]] (why the floor exists), [[project-vram-census]] (instrument),
[[project-grass-benchmark]] (the clumpCV gate).

## The four findings that decide the knob plan

1. **Base cost ~3.3 GB before a single asset**, and it barely moves: FLOOR 1548 + mesh pool 552
   + nonres 836 + padding 363 (mean of 19). FLOOR spans only **1433-1670** across an indoor
   cellar, a 10M-poly teaser and a tropical island. On a 4 GB card (~3.5 GB usable) that leaves
   **~200 MB for content** — which is why even Switch Escape (255 MB content) totals 3780.
2. **Grass is the entire spread**: 0 to 4297 MB, mean 912, six demos have none. Biggest-minus-
   smallest demo is 5219 MB and grass alone is 4297.
3. **TREES ARE A NON-ISSUE — 63-85 MB on EVERY demo, 22 MB spread hub-wide. Do NOT build a tree
   VRAM knob.** (Tree mesh bytes sit in the shared suballocator, which is itself near-constant,
   so the TREES column is textures only — stated as a caveat in the doc.)
4. **nonres 836 MB mean is nearly all pipeline state objects.** Lazy PSOs take it to ~200 with
   zero visual cost.

## The uncomfortable conclusion

Projected against ~3500 MB usable: the shipped preset gets **5/19 demos inside 4 GB at grass
density 75, 6/19 at 50**. It takes ~1 GB off a typical demo but **does not reach the goal**.
The arithmetic: after lazy PSOs the immovable base is still ~2666 MB against content+misc
averaging 797. **Bounty has NO GRASS AT ALL and still misses** — that is the proof that the
remaining work is **floor reduction, not content reduction**. Squeezing grass harder cannot fix it.

## Knob plan (detail in the repo doc)

**Tier A — fixed cost, no visual trade, everyone benefits:** **A0 legacy grass/tree blade atlases
on demand −220 (SHIPPED + VERIFIED, game `0bf21490`)** · lazy object PSOs −633 (SHIPPED) ·
suballocate terrain chunk mesh buffers −125 (incl. **72 MB of 4-byte empty vertex streams**) ·
mesh pool granularity 256→128 MB −128 · **terrain machinery allocated on demand −1111 on
terrain-free levels** (biggest single item) · tessellation+voxelize PSO axes ~−200.

**Tier B — user-facing dials:** grass density (SHIPPED, clumpCV-gated) · grass draw distance
(SHIPPED) · terrain detail 1024²→512² −165 · terrain paint res 4096²→2048² −49 · shadow
resolution −60 · texture budget cap by card size (matters most on Aztec, 1038 MB content) ·
wetmap opt-out when no weather −52 · **SVT atlas + mip bias PAIRED −288 (atlas alone starves
below 12288; needs its own fast-travel soak)**.

**With Tier A + B the base falls to ~1800-1900 MB** and 4 GB becomes reachable for everything
except the grass giants (Z Island, Aztec Game Kit, Jungle Fever).


## A0 shipped — and a CORRECTION to this file's own naming

The 220 MB bucket labelled `FLOOR terrain src arrays` is **NOT terrain**. It is five legacy blade
atlases: `texGrass` (1024² × 46 = 63.2 MB) + `texTree` / `texBranchesHigh` / `texTreeNormal` /
`texTreeHigh` (1024² × 38 = 156.6 MB). Byte-identical on every demo because they are sized from
the fixed grass/tree BANKS, not from level content.

**Provably dead, statically:** created EMPTY; their only writer
(`GGGrass_LoadTextureDDSIntoSlice` / `GGTrees_…`) is a **no-op stub** with the real body inside an
`#if 0`; their only readers are `GGGrass_BindGrassArray` (called solely from GGTerrain's own
draw), `GGGrass_Draw_Prepass` (declared, never registered) and GGTrees' draws — and **every
`customDraw_*` in master_part1.cpp early-returns while `ggterrain_use_wicked_terrain` is set**.

Now behind `GGGrass_EnsureLegacyTexArray()` / `GGTrees_EnsureLegacyTexArrays()`, hooked to the
same legacy-activation point as the 1.70 page atlas, so the Y-key toggle still works.

Verified — POLYS bit-identical on all three, zero `tex` records left, density gate passed
(clumpCV 1.102 vs ref 1.104): TESTPRO1 5926-5990→**5710.4** · Island Showdown 4328→**4144.6** ·
Foggy Forest 5269→**5036.8**.

**Knob B3 (terrain detail 1024²→512²) is WITHDRAWN** — it was aimed at these very arrays.

**Build trap hit again:** a block-scope `extern` in GGTerrain_part0.cpp resolved as
`GGTerrain::…` and failed to link. These live in the `GGGrass` / `GGTrees` namespaces — declare
in their headers, never as a block-scope extern. See [[feedback-edit-worktree-path]] neighbours.


## Tier A shipped 2026-08-02 (A0, A2, A3, A5) — cumulative, POLYS bit-identical, zero pixels cost

| Level | audit baseline | after Tier A | saved |
|---|---|---|---|
| Switch Escape | 3780 | **3116** | −664 (−17.6 %) |
| Island Showdown | 4328 | **3841** | −487 |
| Foggy Forest | 5269 | **4701** | −568 |
| TESTPRO1 | 5926-5990 | **5391** | ~−535 |

- **A0** game `0bf21490` — legacy grass/tree blade atlases on demand, −220
- **A2** game `e1b437a4` — `GGTerrainChunk::Reset()` RELEASES its buffers instead of allocating
  4-byte stand-ins. **D3D12's minimum allocation is 64 KB, so 576×2 4-byte buffers cost 72 MB of
  nothing.** Watch for this pattern elsewhere.
- **A3** engine `e0287386` — `GPUSubAllocator::blocksize` 256→128 MB; −128 on small levels, no
  change on levels that genuinely need ~470.
- **A5** engine `e0287386` — skip the tessellation and RENDERPASS_VOXELIZE object-PSO axes,
  gated on the LIVE flags (`tessellationEnabled`, `VXGI_ENABLED`); eager pipelines **6337→4033**.
  Needed `SetTessellationEnabled(false)` moved into `main()` — master_part1 set it long after
  LoadShaders. **Third time the "flag set after LoadShaders" ordering trap has bitten.**

## A4 (SVT tile pool) — INVESTIGATED, deliberately NOT shipped

Pool is **576 MB**, one buffer sized for the whole atlas across **four** maps.

**Option 1, recommended: drop the EMISSIVE map = −144 MB, bounded, low risk.**
`VirtualTextureAtlas::maps[4]` is basecolor/normal/surface/**emissive**, and the pool is the sum
over all four. **MAX never sets EMISSIVEMAP on a terrain material** — `GGTerrainWicked.cpp` uses
only BASECOLORMAP (334), NORMALMAP (335), SURFACEMAP (339). ~20 lines: guard the three map loops
(wiTerrain.cpp ~1761 creation, ~1843 tile mapping, ~1925 material binding) + the ~2350 dispatch.

**Option 2: grow the pool on demand = up to −384 MB, but a real refactor.** Cellar peaks at
**890 of 2852 tiles**. Blocked by `atlas.tile_pool` being a SINGLE GPUBuffer threaded through
`SetTextureVirtual` and `commands[0].tile_pool` — multi-pool means tracking which pool backs each
tile through the sparse-residency bookkeeping.

**Why nothing shipped:** both touch terrain residency and the brief was "with a soak". A
fast-travel soak is what catches tile starvation (it is how `svtatlasheight=8192` was caught, and
only after sustained travel) and cannot be compressed. Half-soaked terrain change < no change.


## A4 option 1 (drop EMISSIVE map) — TRIED 2026-08-02 and REVERTED

**Saving is −96 MB, NOT −144.** The pool is `sum(total_tile_count * sparse_page_size)` per map and
the formats differ: BASECOLORMAP and EMISSIVEMAP share a `case` and are **BC1** (4 bpp), NORMALMAP
is BC5 and SURFACEMAP is BC3 (8 bpp). `X+2X+2X+X = 6X = 576` ⇒ X = 96 — **emissive is the cheapest
map, 1/6 of the pool.** Measured 576.0 → 480.0 MB, atlas sparse textures 8 → 6. Mechanism worked.

**It blows the entire scene out to white** (Canyon AND Z Island). Reverted; engine back at the
committed state, EXE rebuilt and re-verified.

**The soak PASSED** — 3 laps Z Island, MIN_FREE 881/2852, peak used 1971 vs ~2001 before. So it is
**not starvation and not a residency bug — it is a shading bug.**

**Suspected, NOT confirmed:** the material binding loop skipped emissive with a bare `continue`,
leaving `material->textures[EMISSIVEMAP]` unbound; an unbound bindless slot resolving to the
engine's default WHITE texture would put full-white emissive on every terrain pixel.

**Retry:** (1) `SET_TANGENTVIS 15` renders emissive directly — confirm what a missing slot samples;
(2) bind an explicit 1×1 BLACK texture (or zero the terrain material emissive colour) instead of
leaving it unbound; (3) verify with SCREENSHOTS as well as numbers. The four guarded loops in
`wiTerrain.cpp` were correct — only the material-binding half needs fixing.

**THE LESSON: numbers are not verification.** This hit every numeric target — −96 MB, clean soak,
POLYS identical, FPS up — while rendering the scene pure white. Only the screenshot caught it.
