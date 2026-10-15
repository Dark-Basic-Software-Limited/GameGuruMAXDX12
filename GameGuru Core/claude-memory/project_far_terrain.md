---
name: project-far-terrain
description: "GGMAX 3.114 far terrain (GGTerrainFar) - the DX11 horizon drawn from the still-generated legacy GG clipmap past the live Wicked chunk ring; coverage mask, sink+discard contract, reflection PSOs, cost, and the traps the review found"
metadata:
  node_type: memory
  type: project
  originSessionId: 9a28c586-4c13-4447-916e-7fb51301bfa8
  modified: 2026-10-15T15:59:42.523Z
---

**What:** DX12's live terrain is a camera-centred Wicked ring (25x25 chunks of 5280 units, ~63k radius = 1.6 km); DX11 drew terrain to 500k. Raising the ring is quadratic and cannot get there. The legacy GG clipmap (9 levels x 64 chunks) is STILL generated every frame in DX12 (GGTerrain_Update) - only its draw was gated off. `GGTerrain/GGTerrainFar.cpp` draws its chunks wherever live (or baked) chunks are not. Notes §3.114, checklist 244. Lee asked for it 2026-10-15 ("as close to 100% parity with DX11 ... to win over existing users").

**Contract:** 64x64-bit coverage mask (cbuffer b5) -> VS sinks far vertices on covered cells by segBase*2^lod; BOTH pixel shaders discard when cell + 8 neighbours are covered (shared hlsli - must stay identical); CPU `ChunkHidden` is the whole-chunk twin. Albedo = GG layer rule over the live terrain's own five Color.dds (t51..t55, sRGB view).

**Traps (each one hit or found by the 3.114 review):**
- Shaders MUST load at startup (GGTerrainFar_Init beside GGTerrainBake_Init) - lazy load after the CWD move never finds the .cso.
- The legacy `texColorArray` only exists after `GGTerrain_EnsurePageAtlas` (1.3 GB legacy atlas) - never depend on it.
- Never walk `terrain->chunks` on the main thread: the generator job inserts into that flat_hash_map on a worker, unlocked. Use main-scene `FILTER_TERRAIN` objects (only wiTerrain sets it; chunk sits at c*chunkSize).
- Build the mask AFTER `__super::Update` (Scene::Update's own Generation_Update merges/removes chunks). Bridge site only calls `GGTerrainFar_RequestUpdate`.
- The four customDraw hooks record on parallel jobsystem workers - shared mutable scratch = race; snapshot on the main thread.
- Resource keys are raw strings: the exe dir must pop its trailing separator exactly as GGTerrainWicked_Init does, or "cache hits" duplicate textures.
- Planar reflection FLIPS winding (own PSO pair). In the reflection, draw far terrain BEFORE billboards/bake (they have no reflection prepass and write no depth).
- Object Detail Distance caps terrain chunks too - a chunk past its draw_distance is not ground; leave the cell to the far field.

**Cost (RX 9060 XT, DX12TESTLEVEL):** +0.63 ms total; 0.24 ms without the water reflection (`terrainfarreflect=0`). The reflection share is raster in a 1024-wide 4x MSAA target, NOT shading (`SET_TERRAINFAR_DEBUG 9` flat PS = same FPS); two cut attempts failed. Related: [[project-fog-two-paths]] (the 3.114 DX11 fog curve), [[project-terrain-bake]], [[project-customdraw-hook-inventory]], [[project-measuring-rules]].
