---
name: project-grass-benchmark
description: "THE acceptance gate for any grass/VRAM work (created 2026-08-01 after a grass regression had to be reverted). Lee saved a TESTPRO1 scene with PINK FLOWERS so grass density is measurable, not opinion. Repo GameGuru Core/GRASS_BENCHMARK.md is the authority; tools/grassdensity.ps1 reports coverage + clumpCV + near-to-far bands. GOOD reference: driver VRAM 6536-6553 MB, 52 hair systems, 4.216M strands, grass = 1945 MB (38.9%). Gate: coverage 9.5% +/-0.15pp, clumpCV 0.871 +/-0.01 (THE clumping detector), bands +/-0.25pp. Goal: same scene, much less VRAM, hardly any visual change; Lee's preferred dial is Grass Draw Distance (dense near, sparse far for low-end, extreme distance for high-end)."
metadata: 
  node_type: memory
  type: project
  originSessionId: 9a28c586-4c13-4447-916e-7fb51301bfa8
  modified: 2026-08-01T14:07:37.294Z
---

**Repo `GameGuru Core/GRASS_BENCHMARK.md` is the authority.** This is the orientation copy.

## Why it exists

On 2026-08-01 a grass VRAM change shipped and had to be reverted the same day — it turned a dense
meadow into "spaced-out bunches". It passed my review because I validated density with a
**mean-colour proxy**, which cannot distinguish grass that has been *redistributed into clumps*
from grass that is evenly spread. Lee then saved a benchmark scene using **pink flowers**
precisely so density is visible and countable. Never again judge a grass change without this.

## The scene + how to run it

`OPEN_PROJECT TESTPRO1` → `CLICK_ONLY_LEVEL`. The saved editor camera frames the flower meadow,
so shots are directly comparable with no camera setup. Grass Draw Distance saved at 2247
(per-strand cull 4747 in), Density 100, Match Terrain Color ON (this matters — it interleaves
grass types per cell, which is what broke the reverted change).

## GOOD reference (engine `2b9b989f`, game `207e4b4d`)

driver VRAM **6536-6553 MB** | census ~4997 MB | grass **1945 MB / 38.9%** (37 systems @ 47.4 MB
+ 3 @ 25.2 + 12 @ 8.6) | HAIR_SYSTEMS 52 | strands 4,216,000 | FPS 59.9 | POLYS 2,714,197.

## The gate — `GameGuru Core/tools/grassdensity.ps1`

Counts flower-pink pixels in the viewport. Reports **coverage** (overall density), **clumpCV**
(coefficient of variation over an 8x6 tile grid — *the clumping detector*, the thing the failed
change needed and lacked), and **bands** (coverage near→far = the draw-distance curve).

Noise floor, 3 identical runs: coverage 9.436-9.554%, clumpCV 0.870-0.872, bands stable to ~2%.
**Pass requires ALL of:** coverage ±0.15 pp, **clumpCV ±0.01**, each band ±0.25 pp.
Sensitivity proven: `SET_GRASS viewdist` 4747→2000→1000 collapses the far bands
(3.14 → 0.68 → 0.00) while near bands hold at ~14.

## Gate PROVEN against the real regression

The reverted build was temporarily rebuilt and run on this scene: coverage 9.554 → **6.695%**,
**clumpCV 0.870 → 1.144**, every band down, strands 4.216M → 0.92M. It fails every threshold by
19-27×. Then the good build was restored and re-verified (coverage 9.426, clumpCV 0.873,
4,216,000 strands, 6552.4 MB — all inside the noise band). So the gate demonstrably catches the
exact failure a mean-colour proxy waved through.

## THE RETRY (verified feasible, not yet implemented) — one hair system per chunk

**Placement is BIT-IDENTICAL, which the failed attempt never was.** All per-type systems in a
chunk already share `randomSeed` (always 1), the emitter mesh, the index list (all-1.0
`vertex_lengths`) and `strandCount` (per chunk, not per type) — so strand *i* lands on the same
triangle with the same barycentrics in each; a paint cell holds one type, so the surviving sets
are disjoint and their union IS the painted strands. Merging renders the same blades.
Per-type length/width/stiffness/drag/viewDistance/texture fit with **no buffer growth**: type
rides in **`vb_nor.w`** (written as constant 0, read by nothing — do NOT use `vb_uvs.zw`, it has a
live SSR/SSGI consumer), scalars go in a `HairParticleCB` table (~2.8 KB of a 64 KB budget; DX11
shipped this exact design), texture via the **bindless SRV index**, no texture array needed.
Hazards: `billboardCount` (2 vs 1 for weed/kelp/seaweed) is the only structural difference — take
max over present types and collapse extra billboards to zero-area; patch ALL FOUR hair pixel
shaders; keep frustum radius consistent with per-strand length. **Mutually exclusive with the
reverted coverage scaling.** Expected 1945 → ~500 MB. Full plan in repo `VRAM_CENSUS.md`.

**Free 61 MB found alongside:** `GGGrass.cpp:1089` allocates a 1024²×46 BC3 array whose every
writer is a disabled stub (`#if 0`, tinyddsloader removed in the DX12 port) and whose only binder
is the dead legacy draw path. Make it lazy like the other dead-VT arrays.

## Where this scene's grass memory sits (per 47.4 MB tier-3 system, ~497 B/strand)

positions vb_pos[0]+[1] **25.6 MB (54%)** | ib_culled+prim_view 9.6 (20%) | uvs 6.4 (13%) |
simulation 3.2 | normals 3.2 | wetmap 1.6. The raytracing position copy (12.2 MB) is already
gone (engine 1.70). **37 tier-3 systems ≈ 9 chunks × ~4 painted types** — that multiplication is
the target; see [[project-vram-census]] for the ranked backlog and the do-not list.
