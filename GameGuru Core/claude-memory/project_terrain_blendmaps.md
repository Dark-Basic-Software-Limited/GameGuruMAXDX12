---
name: project-terrain-blendmaps
description: Durable rules for the terrain blendmap pipeline (painted materials on Wicked terrain). All investigations resolved by 5d647db6; these invariants must hold for any future blendmap work.
metadata: 
  node_type: memory
  type: project
  originSessionId: 63b96d44-9243-46fb-bc37-9eb4d4be60b9
---

# Terrain blendmap pipeline — durable rules

Painted PBR materials render via per-chunk blendmap override. The Phase 3 / camera-excursion / test-level investigations are all resolved (final commit `5d647db6` — the camera-excursion fix also cured test-level corruption; no separate issue existed). Rules that MUST hold:

- `SetupWickedTerrainMaterials` runs AFTER level load populates `pMaterialMap` (hook: `GGTerrainWicked_OnPaintDataChanged` from `GGTerrain_SetPaintData`). Only restart generation when paint data has materials not already in `materialToSlot` — unconditional restarts caused corruption.
- Paint AFTER `Generation_Update()` — generation otherwise overwrites blendmaps. Skip chunks with empty `blendmap_layers` WITHOUT marking them processed (retry next frame). VT invalidation is picked up next frame (1-frame delay, imperceptible).
- Never iterate `terrain->chunks` without `Generation_Cancel()` — the generator thread mutates the unordered_map concurrently.
- Add keys to `processedChunkKeys` only after ALL validity checks pass. Wicked removes distant chunks and regenerates them with NEW entity IDs at the same grid position — keep a `chunkKeyToEntity` map and reprocess on entity change.
- `GGTerrain_GetEditableSize()` returns HALF-size (center→edge, NOT full width). Editable area is ±editableSize from origin. Do NOT halve it again.
- Reference data point: Island Showdown = 13 painted materials (9 extra), editableSize 9842.5, 5M painted pixels.

- **Gating + batching (2026-07-18, `fc1575e2` + `03e2296e`):** both blend passes are gated behind a chunk-set signature + processed-map-size check (perf — they were 3.6ms/frame of idle scanning), return bool "caught up", and cap each batch at 64 chunks (the gate refires until drained — a capped pass must NOT latch the gate cache). During the initial terrain build they run only every 30th frame — **calling Generation_Cancel per frame while chunks stream in was the main cause of the 30-second radial build** (each cancel killed the generator after 1-2 chunks). Any new per-chunk pass must follow the same pattern: never cancel generation every frame during the build.

The DX11-style auto-blend that fixed grey mountains lives on top of this pipeline — see [[project-terrain-texture-mismatch]] for its design and the 5-slot-cap limitation.
