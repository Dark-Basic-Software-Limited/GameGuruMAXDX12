---
name: project-terrain-texture-mismatch
description: "RESOLVED 2026-07-13 (commit 42e927b8). TESTPRO1 island terrain now matches DX11 baseline (green mountains) via CPU-side DX11-style blendmap override. Kept as reference for how the fix works and for future levels with unusual layer setups."
metadata: 
  node_type: memory
  type: project
  originSessionId: 63b96d44-9243-46fb-bc37-9eb4d4be60b9
---

# Terrain texture mismatch — RESOLVED via CPU-side DX11 blend override

## Status

**RESOLVED 2026-07-13** — commit `42e927b8` on `main`. TESTPRO1 island terrain surface now matches DX11 baseline: green mountains, grey rock only on near-vertical cliff faces, sand on beach. (Stage 4 impostor work was subsequently retired entirely — see [[project-trees-phase5]] Stage 4.3; the successor thread is hand-rolled distant-tree billboards, if the A/B warrants them.)

## Root cause

DX11 and Wicked use fundamentally different terrain-blend models:

- **DX11 shader** ([GGTerrainPageGenPS.hlsl:216-246](GameGuru Core/Guru-WickedMAX/GGTerrain/Shaders/GGTerrainPageGenPS.hlsl:216)): loops through **5 explicit height layers** with per-layer `start`+`transition` doing REPLACE-or-LERP, then 2 slope layers same shape. Every layer's height band is independent.
- **Wicked generator** ([wiTerrain.cpp:1121-1129](../WickedEngineDX12/WickedEngine/wiTerrain.cpp:1121)): 4 fixed slots (base/slope/low_alt/high_alt) blended via `smoothstep(0, regionN, x)`. Left edge hardcoded to 0, so any non-zero `slopeStart`/`layerStart` bleeds material from just past sea level upward.

TESTPRO1 island specifically:
- `layer[1]` mat 0 (mid-material, heights 180-360) was **ignored entirely** by the mapper — Wicked only has 4 auto slots so layer[1] didn't map to any. DX11 fully REPLACES base with mat 0 above height 360, giving mountains their green mid-material tint. Skipping it in DX12 made mountains bare grey rock.
- `region1 = slopeEnd[0] = 0.4` made slope material bleed in from any incline >0 (smoothstep from 0, not from `slopeStart=0.2` as DX11 does).
- `region3 = 1.0` made high-altitude rock bleed in from just above sea level (smoothstep(0, 1.0, height/topLevel)) instead of only kicking in at `layerStartHeight[2] = 5736`.

## The fix — Path A (CPU blendmap override)

In [GGTerrainWicked.cpp](GameGuru Core/Guru-WickedMAX/GGTerrain/GGTerrainWicked.cpp):

1. **Add extra slot for layer[1] mat** — `SetupWickedTerrainMaterials()` registers `layerMatIndex[1]` as slot 4 (past the 4 auto slots), so Wicked can render 5 auto materials instead of 4. Painted materials shift to slot 5+.
2. **`ApplyDX11StyleAutoBlend(terrain)`** — new function running after `Generation_Update` and before `ProcessPaintedChunkBlendmaps`. For every terrain chunk, iterates each vertex, computes DX11's exact per-layer/per-slope weight sequence:
   ```
   w[base] = 1, all others = 0
   for layer i in 0..4: t = clamp((height-start)/(end-start), 0, 1); if t > 0: w *= (1-t); w[targetSlot] += t
   for slope i in 0..1: t = clamp((normaly-start)/(end-start), 0, 1); if t > 0: w *= (1-t); w[slopeSlot] += t
   ```
   Writes results (× 255) to `chunk_data->blendmap_layers[0..4].pixels[vi]`, invalidates the chunk's blendmap GPU texture and VT.
3. **Per-chunk tracking** — `dx11BlendProcessedKeys` set + `dx11BlendChunkKeyToEntity` map detect regenerated chunks (Wicked destroys distant chunks and recreates them on camera return) so we reapply weights on new entity IDs.

Wicked's built-in generator still fills weights initially (based on region1/2/3 defaults) — our override runs on the next frame after generation completes and overwrites them. Painted materials still win on painted cells because `ProcessPaintedChunkBlendmaps` runs AFTER our pass.

## Why this preserves smooth transitions

Wicked already computes per-vertex weights and lets the rasterizer interpolate across triangles. We just replace the formula, not the pipeline. Terrain shader material sampling, mask/noise breakup, GPU-side blending — all unchanged. Only the vertex-weight *values* change.

The one visible difference: DX11's linear-ramp `clamp((x-start)/(end-start), 0, 1)` vs Wicked's Hermite `smoothstep(0, region, x)`. Since DX11 is the baseline that looks correct, its curve shape is what we want.

## Limitations / future work

- **5-slot cap.** Our override registers layer[1] as slot 4, so we have 5 auto materials total (base, slope, layer[0], layer[1], layer[2]). GG has 5 layer slots + 2 slope slots — if a future level assigns distinct materials to layers 3, 4, or slope 1, they get mapped to `highSlot` and `slopeSlot` respectively (see `applyLayer(3, highSlot)` etc in `ApplyDX11StyleAutoBlend`). This means overloaded slots. TESTPRO1 doesn't hit this; other production levels might.
- **`region1/2/3` are still set on `terrain->` but unused for TESTPRO1**. Kept as safe defaults in case any vertex misses our override (shouldn't happen unless generation lags).
- **Perf**: CPU pass runs once per new chunk, then never again for that chunk. Costs ~vertexCount * chunkStride ~1-2ms per chunk-add during camera pan. Insignificant vs Wicked's own terrain generation work.

## How to test on other levels

Load any GG level via `OPEN_PROJECT <name>` → `CLICK_ONLY_LEVEL` → visual A/B against DX11 with vegetation cleared. If mountains/regions look wrong, dump `ggterrain_global_render_params` (diagnostic block previously at [GGTerrainWicked.cpp:207](GameGuru Core/Guru-WickedMAX/GGTerrain/GGTerrainWicked.cpp:207) — stripped in commit but preserved in git history for reference) and check whether the mapping to slots 0-4 covers the level's actual layer setup.

Related: [[project-trees-phase5]] (unblocked), [[project-dx11-parity-baseline]] (this is the level we A/B against), [[feedback-two-attempts-change-approach]] and [[feedback-dont-thrash-on-automation]].
