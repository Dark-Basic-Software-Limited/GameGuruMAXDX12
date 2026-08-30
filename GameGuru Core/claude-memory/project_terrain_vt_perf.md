---
name: project-terrain-vt-perf
description: "Phase-2 performance campaign - the four brutal off-switches, the terrain virtual-texture cost that was hiding as GPU idle, and what a baked terrain is actually worth."
metadata: 
  node_type: memory
  type: project
  originSessionId: 9a28c586-4c13-4447-916e-7fb51301bfa8
  modified: 2026-08-24T04:57:15.686Z
---

Full narrative: `GameGuru Core/NIGHT_INVESTIGATIONS_2026-08-12.md` §2.94-§2.94f, §3.02.
Sweeps: `GameGuru Core/DEMO_FPS_SWEEP.md`. Related: [[project-far-tree-billboards]].

## The four off-switches (2.94)

Terrain / Trees / Grass / Water Off in Graphics & Performance. Session globals + setup.ini
`noterrain` / `notrees` / `nograss` / `nowater` + harness `SET_*OFF`. These do not merely hide:
the elements never enter the scene, so no culling, no `Scene::Update`, no shadow casting, no VT
feedback, no VRAM. Aztec editor **15.60 → 5.85 ms, 63 → 167 FPS**.

Per-switch on Aztec: terrain −9.7 ms, trees −2.9, grass −1.3, water 0.00 (no water on that level,
so **water is UNVERIFIED** there — elsewhere it measured 4.37 ms of a 10.06 ms frame, the biggest
of the four, because it takes the whole planar-reflection pass with it).

## ★★★ The finding that mattered: it was never idle, it was UNMEASURED

Terrain-off moved "GPU Idle" 6.2 → 1.3 ms. The three terrain VT passes carried only
`device->EventBegin` — a **PIX marker, invisible to `wi::profiler`** — so a pass instrumented that
way is *guaranteed* to read as idle. That hid **3.96 ms/frame** for the whole project:
`TerrainVT - WritebackTileRequests` was doing 4 `CopyResource` per VT per frame over a 625-chunk
ring, ~2,500 copies/frame, purely for residency bookkeeping.

**Fixed 2.94d**: `wi::terrain::gg_vt_writeback_interval = 4` (harness `SET_VTWRITEBACK`).
Aztec 15.58 → 12.30 ms, TESTPRO2 9.27 → 5.87 ms, POLYS identical.
★ Allocate and Writeback MUST share the cadence — writeback is what CLEARS the feedback/request/
allocation buffers, so gating writeback alone gives duplicate requests and free-tile exhaustion.

**2.94f engine-side gate**: the GGMAX idle gate only ever gated the *bridge's*
`Generation_Update`; `Scene::Update` calls it again and that caller was ungated. Bridge now
publishes `wi::terrain::gg_skip_generation_update`, CONSUME-AND-CLEAR (a sticky true would gate
terrain forever). ⚠ only ~0.30 ms of the win is main-thread; ~1.3 ms is worker — do not quote it
as frame time.

## Baked terrain — answered, not built

VT on a chunk is **one integer**: `sparse_residencymap_descriptor` (−1 = plain `tex.Sample`).
`Terrain::gg_near_ring_dist = 0` reproduces a bake's runtime state exactly, no baker needed.

Worth **~67% of terrain's cost at identical polys** (4.66 → 3.50 ms, 214 → 286 FPS).
★★ **The cliff is TILE COUNT, not resolution**: `SVT_TILE_SIZE` = 256 and
`if (tile_count > 1) allocate_residency` (wiTerrain.cpp:499) — so 256 = one tile = no residency =
**+35%**, while 512 and 1024 both = **+19%**. A real bake binds a plain SRV outside the VT system
and should hold +35% at any resolution.

★ Dropping VT buys GPU; merging 625→49 chunks buys CPU (−0.90 ms, −52%) and only −0.24 GPU.
Neither buys the other. ⚠ merging kills per-object LOD/frustum/occlusion.
⚠⚠ **NEXT ACTION if resumed: do the VRAM arithmetic FIRST** — 625 chunks × 1024² BC ≈ 300–600 MB,
not affordable against the 4 GB floor. Bake inside a radius, or at 512, or to a shared atlas.

## Still open

- `GGTerrainWicked_Update` has **no editor/game discriminator** — an exported exe runs the full
  sculpt/paint bridge every frame. Gating on `gameisexe` is tempting but the chunkSig/blendmap
  scans still need to run (the ring follows the camera), and an exported build cannot be verified
  from the harness. HELD for Lee.
- `gg_instinit_parallel` defaults OFF; A/B ran but did not resolve.
- SVT atlas commits ~480 MB at boot on EVERY level, including indoor.
- 0822 gate is PROVISIONAL on one pass — pass 2 aborted when a MAX instance wedged at 6.9 GB.
