---
name: project-chunk-scale
description: "Terrain chunk_scale final value = 80 (generation = 10). GG is inch-scaled, Wicked assumes meters; two local Wicked patches exist because of this value."
metadata: 
  node_type: memory
  type: project
  originSessionId: 63b96d44-9243-46fb-bc37-9eb4d4be60b9
---

# Terrain chunk scale — final: 80

GG uses 1 unit = 1 inch; Wicked assumes 1 unit = 1 meter, so Wicked-default chunks were ~13m and visibly popped in at distance.

**Final values:** `chunk_scale = 80.0f` (~5280 units ≈ 132m per chunk), `generation = 10` (maintains ~52800-unit coverage). Fewer, larger chunks = less generation overhead + less visible popping. Evolution was 8 → 40 (gen=20) → 80 (gen=10).

`chunk_scale` is a built-in `wi::terrain::Terrain` member (`wiTerrain.h`) that scales chunk world positions, vertex spacing, and shader constants throughout the pipeline.

Two local Wicked patches exist BECAUSE of chunk_scale=80 (both dormant at scale 1) — see [[project-wicked-engine-changes]].

Debug: U key toggles global wireframe (`wi::renderer::SetWireframeMode(WIREFRAME_OVERLAY)`) in `GGTerrainWicked_Update()`.
