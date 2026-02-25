# Terrain System Port: Old Virtual Texture -> Wicked Engine Terrain

## Context

The old terrain uses a custom virtual texture pipeline with **unfixable LOD seam artifacts** caused by an architectural conflict: page content must be camera-independent for shift reuse, but height maps are camera-centered. Three fix attempts all failed (see SCRATCHPAD.md git history). The plan is to port to the Wicked Engine's built-in DX12 terrain system, which handles LOD transitions natively.

The port must be **incremental** — each phase independently verifiable with A/B toggle comparison against the old terrain's reference color mode.

---

## Key Architectural Decisions (Informed by Devil's Advocate)

### 1. Material system: NOT limited to 4 layers

**Finding**: Wicked Engine already supports N blendmap layers via `ChunkData::enable_blendmap_layer(materialIndex)` and the spline material system. The virtual texture update compute shader iterates all layers with early-exit. No 4-material limit.

**Decision**: Use N layers (up to 32) to map all GameGuru painted materials. Each unique material used on the terrain gets its own `materialEntity` and blendmap layer. Chunks with no painting have only 4 layers (zero overhead). This requires **zero modification** to Wicked Engine's blending shader.

### 2. Height data: Custom Modifier, not HeightmapModifier

**Finding**: Wicked's `HeightmapModifier::Apply()` uses integer truncation (no bilinear interpolation) and cannot express GameGuru's composite height (fractal + sculpt overlay + flat areas).

**Decision**: Create a `GGHeightModifier : public wi::terrain::Modifier` that calls GameGuru's existing `CalculateHeightWithHeightmap()` for each vertex. This function already combines fractal noise, base heightmap, sculpt edits, AND flat area stamps. Flat areas work for free.

### 3. Height queries: Keep old CPU data, skip old GPU rendering

**Finding**: `GGTerrain_GetHeight()` and `GGTerrain_RayCast()` use CPU-side chunk data, not GPU. They work regardless of which terrain renders. 42+ call sites across 9+ files depend on them.

**Decision**: Keep `GGTerrain_Update()` running for CPU data maintenance (chunk generation for height queries). Skip only the virtual texture page management and GPU atlas updates. Old height queries stay untouched.

### 4. Toggle: Runtime switchable, suppresses old draw calls

**Decision**: New global `ggterrain_use_wicked_terrain`. When active, set `ggterrain_draw_enabled = 0` (all 8 old draw callbacks return immediately — they already check this flag). Wicked terrain chunks render through the normal scene pipeline (ObjectComponent entities) with zero custom callbacks needed.

### 5. Zero Wicked Engine modifications — post-process via public API

**Constraint**: Wicked Engine source must not be modified (upstream changes would overwrite our additions).

**Solution**: Follow GameGuru's existing abstraction pattern (same as GGAnimBridge pre/post update hooks, wickedcalls wrapper, custom draw callbacks). After `Generation_Update()` returns on the main thread, iterate newly-generated chunks and override their data using Wicked's public members:

- `wickedTerrain.chunks` — public `unordered_map<Chunk, ChunkData>`, iterable
- `chunk_data.blendmap_layers` — public `vector<BlendmapLayer>`, writable
- `chunk_data.enable_blendmap_layer(index)` — public method, extends layers
- `wickedTerrain.CreateChunkRegionTexture(chunk_data)` — public method, re-uploads blendmap GPU texture
- `chunk_data.grass.vertex_lengths` — public vector, writable
- `chunk_data.grass.CreateFromMesh(mesh)` — public method, regenerates grass particles
- `chunk_data.invalidated` — public bool, triggers chunk regeneration

The post-process runs one frame after chunk generation (imperceptible). Track known chunks between frames to detect newly-generated ones that need overrides.

---

## Phase 0: Toggle Infrastructure + Empty Wicked Terrain

**Goal**: Both terrain systems coexist. Toggle shows flat Wicked terrain chunks or old terrain.

### Changes

**New files**:
- `Guru-WickedMAX/GGTerrain/GGTerrainWicked.h` — declarations for init/update/shutdown
- `Guru-WickedMAX/GGTerrain/GGTerrainWicked.cpp` — `wi::terrain::Terrain` instance, wrapper functions

**Modified files**:
- `Guru-WickedMAX/GGTerrain/GGTerrain.h` — add `extern int ggterrain_use_wicked_terrain;`
- `Guru-WickedMAX/GGTerrain/GGTerrain_part0.cpp` — add `int ggterrain_use_wicked_terrain = 0;` near line 4198
- `Guru-WickedMAX/master_part1.cpp` — gate all draw callback lambdas (lines 130-151) with `if (ggterrain_use_wicked_terrain) return;`. Add `GGTerrainWicked_Update(camera)` call in Update (line 200 area)
- `Guru-WickedMAX/GameGuruMain.cpp` — call `GGTerrainWicked_Init()` after `GGTerrain_Init()` (line 168 area)

### GGTerrainWicked.cpp skeleton

```cpp
#include "wiTerrain.h"
static wi::terrain::Terrain wickedTerrain;

void GGTerrainWicked_Init() {
    wickedTerrain.scene = &wi::scene::GetScene();
    wickedTerrain.SetCenterToCamEnabled(true);
    wickedTerrain.SetRemovalEnabled(true);
    wickedTerrain.SetGrassEnabled(false);       // Phase 0: no grass yet
    wickedTerrain.SetPhysicsEnabled(false);      // keep Bullet physics
    wickedTerrain.chunk_scale = 8.0f;            // ~512 units/chunk, similar to old terrain
    wickedTerrain.generation = 100;              // cover ~52800 units each direction
    wickedTerrain.bottomLevel = -20000.0f;       // match GG height range
    wickedTerrain.topLevel = 20000.0f;
    // No modifiers yet — terrain is flat at height 0
    wickedTerrain.Generation_Restart();
}

void GGTerrainWicked_Update(const wi::scene::CameraComponent& camera) {
    wickedTerrain.Generation_Update(camera);
}

void GGTerrainWicked_Shutdown() {
    wickedTerrain.Generation_Cancel();
    // Remove entities from scene...
}
```

### Toggle logic in master_part1.cpp Update()

```cpp
// existing line 200:
GGTerrain_Update(camera.Eye.x, camera.Eye.y, camera.Eye.z, cmd, bImGuiRenderTargetFocus);
// add after:
if (ggterrain_use_wicked_terrain) {
    ggterrain_draw_enabled = 0;  // suppress all old draw callbacks
    GGTerrainWicked_Update(camera);
} else {
    ggterrain_draw_enabled = 1;
}
```

### Skip virtual texture work when toggle active

In `GGTerrain_Update()` (GGTerrain_part0.cpp ~line 9777+), add early-out after chunk generation and env probes but before virtual texture page table management:
```cpp
if (ggterrain_use_wicked_terrain) return;  // skip page gen, atlas, constant buffers
```

### Verify

- Toggle with key binding (e.g., `T` in terrain debug mode)
- Old terrain: reference color mode as before
- New terrain: flat white/default Wicked terrain chunks at world origin
- No crashes on toggle in either direction
- Height queries (`GGTerrain_GetHeight`) still work when new terrain is active

---

## Phase 1: Feed GameGuru Heightmap into Wicked Terrain

**Goal**: Wicked terrain mesh matches old terrain shape exactly.

### Changes

**Modified files**:
- `Guru-WickedMAX/GGTerrain/GGTerrainWicked.cpp` — add GGHeightModifier
- `Guru-WickedMAX/GGTerrain/GGTerrain_part0.cpp` — expose `CalculateHeightWithHeightmap` as a public function (currently static/internal to chunk class)

### GGHeightModifier

```cpp
struct GGHeightModifier : public wi::terrain::Modifier {
    GGHeightModifier() {
        type = Type::Heightmap;
        blend = BlendMode::Normal;
        weight = 1.0f;
    }
    void Apply(const XMFLOAT2& world_pos, float& height) override {
        // world_pos.x = world X, world_pos.y = world Z
        float ggHeight = GGTerrain_CalculateHeight(world_pos.x, world_pos.y);
        // Convert to 0-1 range for Wicked's lerp(bottomLevel, topLevel, height)
        height = wi::math::InverseLerp(bottomLevel_gg, topLevel_gg, ggHeight);
    }
};
```

In `GGTerrainWicked_Init()`, after level data is loaded:
```cpp
auto modifier = std::make_shared<GGHeightModifier>();
wickedTerrain.modifiers.push_back(modifier);
wickedTerrain.Generation_Restart();
```

### Thread safety note

`CalculateHeightWithHeightmap` reads `pHeightMapEdit` which is modified during sculpting. The Wicked generation thread may read stale data momentarily — this is cosmetic only and self-corrects on next chunk regeneration.

### Verify

- Toggle between old and new terrain at same camera position
- Hills, valleys, flat areas should match
- Walk along the terrain boundary to check coverage
- Sculpted areas should appear correctly

---

## Phase 2: 4-Layer Material Rendering (Height/Slope Rules)

**Goal**: Wicked terrain shows correct automatic materials for unpainted areas.

### Changes

**Modified files**:
- `Guru-WickedMAX/GGTerrain/GGTerrainWicked.cpp` — material entity creation, region parameter mapping

### Material setup

Create 4 `MaterialComponent` entities, each loading PBR textures from GameGuru's `terraintextures/mat{N}/` folders:

| Wicked Slot | GG Source Parameter | Default GG Index |
|---|---|---|
| MATERIAL_BASE | `ggterrain_global_render_params.baseLayerMaterial & 0xFF` | 17 |
| MATERIAL_SLOPE | `ggterrain_global_render_params.slopeMatIndex[0] & 0xFF` | 4 |
| MATERIAL_LOW_ALTITUDE | `ggterrain_global_render_params.layerMatIndex[0] & 0xFF` | 2 |
| MATERIAL_HIGH_ALTITUDE | `ggterrain_global_render_params.layerMatIndex[2] & 0xFF` | 20 |

For each, load `Color.dds` -> BASECOLORMAP, `Normal.dds` -> NORMALMAP, `Surface.dds` -> SURFACEMAP using `wi::resourcemanager::Load()`.

### Region parameter mapping

Map GG's height/slope layer thresholds to Wicked's region1/2/3:
```cpp
wickedTerrain.region1 = /* derived from slopeStart[0], slopeEnd[0] */;
wickedTerrain.region2 = /* derived from layerStartHeight[0] */;
wickedTerrain.region3 = /* derived from layerStartHeight[2] */;
```

### Verify

- Compare unpainted areas between old (reference color) and new terrain
- Low areas show low-altitude material, steep areas show slope material, etc.
- Transitions won't be pixel-identical (different blending math) but should be similar

### Phase 2 Progress (2026-02-25)

**Status**: COMPLETE — 4-layer material rendering working via Wicked Engine's virtual texture pipeline.

#### What was done

1. **Material entity setup** (`SetupWickedTerrainMaterials`): Creates 4 material entities from GG render params, attaches them to the terrain entity, loads `Color.dds` and `Normal.dds` for each. Uses `SetTextureStreamingDisabled(true)` + `CreateRenderData()` for synchronous texture loading before `Generation_Restart()`.

2. **Virtual texture pipeline working correctly**: The VT system runs as designed — `Generation_Update()` detects valid textures on `materialEntities[0-3]`, runs `UpdateVirtualTexturesCPU/GPU`, and the compute shader blends materials into the sparse atlas based on slope/altitude blendmaps. No bypass hacks needed.

3. **Correct material indices from GG render params**: Base=mat18 (baseMat=17), Slope=mat5 (slopeMat=4), LowAlt=mat3 (lowMat=2), HighAlt=mat21 (highMat=20). Multiple distinct textures visible on terrain surface, blending by height and slope.

4. **Region parameters derived from GG params**: `region1 = slopeEnd[0]` (0.4), `region2 = 2.0` (low altitude — poor mapping since GG's low layer uses positive heights), `region3 = 1.0` (high altitude — tighter transition for visible mountain material).

#### Key discoveries

1. **`CreateRenderData()` is required**: Unlike the Wicked Editor (which lets textures load lazily across frames), our code calls `Generation_Restart()` on the same frame as material setup. `Generation_Update()` checks `resource.IsValid()` immediately to compute `virtual_texture_any`. Without `CreateRenderData()`, textures aren't loaded yet and the VT pipeline stays disabled → white terrain.

2. **Texture path resolution via EXE directory**: `wi::resourcemanager::Load()` resolves relative to CWD, which changes during the game lifecycle. Fixed by using `wi::helper::GetDirectoryFromPath(wi::helper::GetExecutablePath())` at init time to build absolute paths from the EXE directory, which is CWD-independent.

3. **Material setup must happen before `Generation_Restart()`**: The restart deep-copies `MaterialComponent` data (including resource shared_ptrs) into internal storage. Materials configured after restart are ignored until the next restart.

4. **`Component_Attach` required**: Material entities must be attached to the terrain entity (`scene.Component_Attach(materialEntities[i], terrainEntity)`) following the Wicked Editor pattern.

#### Diagnostic confirmed (terrain_diag.txt at frame 120)

```
matEntity[0] color=VALID normal=VALID name=.../mat18/Color.dds
matEntity[1] color=VALID normal=VALID name=.../mat5/Color.dds
matEntity[2] color=VALID normal=VALID name=.../mat3/Color.dds
matEntity[3] color=VALID normal=VALID name=.../mat21/Color.dds
chunk[0] hasColorTex=YES texMulAdd=(0.016,0.016,0.177,0.983)
chunk[1] hasColorTex=YES texMulAdd=(0.016,0.016,0.532,0.983)
totalChunks=59
```

All 4 material textures valid, chunks have VT atlas textures assigned with correct texMulAdd coordinates.

#### Phase 2 cleanup (completed)

- Texture paths now use EXE directory (`wi::helper::GetExecutablePath()`) — CWD-independent
- Debug quad (`GGTerrainWicked_DebugDraw`) removed from code and Compose callback
- Diagnostic code (terrain_diag.txt writer) removed
- Surface maps enabled — GG Surface.dds uses R=AO, G=Roughness, B=Metalness, A=255 which matches Wicked's SURFACEMAP convention (A=255 means reflectance multiplier is 1.0)
- Region params derived from GG render params where possible (slope from `slopeEnd[0]`)

#### Known limitations (acceptable for Phase 2)

- Region2/3 (altitude blending) can't exactly match GG's independent start/end thresholds — Wicked uses smoothstep from 0, while GG uses per-layer start/end heights
- Blending transitions use cubic Hermite (Wicked) vs linear clamp (GG) — visually similar but not pixel-identical

---

## Phase 3: Painted Material Support via N-Layer Blendmaps

**Goal**: GameGuru's `pMaterialMap` brush painting works on the new terrain.

### Changes

**Modified files** (GameGuru only — zero Wicked Engine changes):
- `Guru-WickedMAX/GGTerrain/GGTerrainWicked.cpp` — post-process loop, material entity expansion

### Post-process pattern (runs on main thread after Generation_Update)

Track known chunks between frames. After `Generation_Update()` returns, detect newly-generated chunks and override their blendmap weights from `pMaterialMap`:

```cpp
// In GGTerrainWicked_Update(), after Generation_Update():
for (auto& [chunk, chunk_data] : wickedTerrain.chunks) {
    if (knownChunks.count(chunk.raw)) continue;  // already processed
    knownChunks.insert(chunk.raw);

    // Get the mesh to read vertex world positions
    auto* mesh = wickedTerrain.scene->meshes.GetComponent(chunk_data.entity);
    if (!mesh) continue;

    bool needsBlendmapUpdate = false;
    for (uint32_t index = 0; index < vertexCount; index++) {
        float wx = chunk_data.position.x + mesh->vertex_positions[index].x;
        float wz = chunk_data.position.z + mesh->vertex_positions[index].z;

        // Sample pMaterialMap at world position
        float mapU = (wx / editable_size) * 0.5f + 0.5f;
        float mapV = (wz / editable_size) * 0.5f + 0.5f;
        int mapX = clamp(int(mapU * 4096), 0, 4095);
        int mapZ = clamp(int(mapV * 4096), 0, 4095);
        uint8_t mat = pMaterialMap[mapZ * 4096 + mapX];

        if (mat > 0) {
            int slot = materialToSlot[mat - 1];
            chunk_data.enable_blendmap_layer(slot);
            for (size_t i = 0; i < chunk_data.blendmap_layers.size(); i++)
                chunk_data.blendmap_layers[i].pixels[index] = (i == slot) ? 255 : 0;
            needsBlendmapUpdate = true;
        }
    }

    if (needsBlendmapUpdate) {
        wickedTerrain.CreateChunkRegionTexture(chunk_data);  // re-upload GPU texture
    }
}
```

### Material entity expansion

At terrain init, scan `pMaterialMap` (using existing `GGTerrain_CheckMaterialUsed()` at GGTerrain_part0.cpp:11525) to find all unique material indices. Create a `materialEntity` for each:
- Slots 0-3: the 4 automatic materials (base, slope, low alt, high alt)
- Slots 4+: one per additional painted material found in the map
- Extend `wickedTerrain.materialEntities` vector to include all

Build `materialToSlot[32]` lookup table mapping GG material index -> Wicked blendmap layer index.

### Verify

- Paint a large area with a distinctive material on old terrain
- Toggle to new terrain: painted area should show the correct material
- Unpainted areas should still use automatic rules
- Toggle back: reference color mode confirms same paint data

### Phase 3 Implementation Notes (from Phase 2 experience)

#### Current architecture to build on

- `GGTerrainWicked.cpp` is 220 lines, clean. The post-process loop goes in `GGTerrainWicked_Update()` after `Generation_Update(camera)`.
- `SetupTerrainMaterial()` already handles loading any GG material by 0-based index. Reuse it for painted material entities.
- `wickedTerrainExeDir` (static string) has the EXE directory for building texture paths. Already used by `SetupTerrainMaterial()`.
- Terrain is accessed via `GetWickedTerrain()` which returns `wi::terrain::Terrain*` from the scene's component manager.
- `Generation_Restart()` deep-copies materials — must be called after adding new materialEntities for painted materials.

#### Critical lessons from Phase 2

1. **`CreateRenderData()` + `SetTextureStreamingDisabled(true)` is mandatory** before `Generation_Restart()`. Without it, textures aren't loaded when `Generation_Update()` checks `resource.IsValid()` → VT pipeline stays off → white terrain.
2. **`Component_Attach(materialEntity, terrainEntity)` is required** for each material entity.
3. **The VT system must NOT be bypassed** — it handles texture blending via compute shader. Our job is to provide correct materials and blendmap weights; the VT system does the rest.
4. **Take incremental steps** — the Phase 2 approach of small verifiable changes worked well. Do the same for Phase 3: first just detect painted chunks and log counts, then override blendmaps, then verify visually.

#### Key data access patterns

- `pMaterialMap` — declared in `GGTerrain_part0.cpp`, need to extern or expose via a function. 4096x4096 `uint8_t` array, value 0 = unpainted, 1-32 = material index (1-based).
- `ggterrain_global_render_params.mapEditSize` or equivalent — the world-space size of the editable terrain area. Needed for UV mapping: `u = (worldX / editSize) * 0.5 + 0.5`.
- `GGTerrain_CheckMaterialUsed()` at `GGTerrain_part0.cpp:11525` — scans `pMaterialMap` and returns which material indices are in use. Call once at setup time.

#### Wicked Engine API to verify before coding

Before writing the post-process loop, verify these members are accessible:
- `terrain->chunks` — `unordered_map<Chunk, ChunkData>`, iterable with structured bindings
- `chunk_data.blendmap_layers` — `vector<BlendmapLayer>`, each has `pixels` vector
- `chunk_data.enable_blendmap_layer(index)` — extends blendmap_layers if needed
- `terrain->CreateChunkRegionTexture(chunk_data)` — re-uploads blendmap GPU texture
- `chunk_data.position` — world position of chunk origin
- Mesh vertex positions — `scene.meshes.GetComponent(chunk_data.entity)->vertex_positions`

#### Chunk generation threading concern

`Generation_Update()` runs chunk generation on background threads. The post-process loop runs on the main thread AFTER `Generation_Update()` returns, but newly-generated chunks may still be in flight. Need to verify whether `chunks` map is safe to iterate after `Generation_Update()`, or if we need to use `chunk_data.vt` state to detect fully-generated chunks.

#### materialEntities beyond slot 3

Phase 2 uses `materialEntities[0-3]` (the 4 auto-material slots). For painted materials, we need additional entities. The `materialEntities` member is a fixed array (`wi::ecs::Entity materialEntities[MATERIAL_COUNT]`) where `MATERIAL_COUNT = 4`. If Wicked Engine doesn't support more than 4 material entities natively, we may need to use the spline material system (`chunk_data.blendmap_layers` can have N layers, each referencing a material beyond slot 3 via `enable_blendmap_layer(index)`). **Verify this in wiTerrain.h before implementing.**

---

## Phase 4: Grass via HairParticleSystem

**Goal**: Grass renders through Wicked Engine, placed from GameGuru's `pGrassMap`.

### Changes

**Modified files** (GameGuru only):
- `Guru-WickedMAX/GGTerrain/GGTerrainWicked.cpp` — grass density override in post-process loop

### Implementation

Extend the same post-process loop from Phase 3. For each newly-generated chunk, also override grass `vertex_lengths` from `pGrassMap`:

```cpp
// Inside the per-chunk post-process loop (same loop as Phase 3):
bool needsGrassUpdate = false;
for (uint32_t index = 0; index < vertexCount; index++) {
    // ... (wx, wz, mapX, mapZ already computed for materials)
    uint8_t grassVal = pGrassMap[mapZ * 4096 + mapX];
    if (grassVal > 0 && !(grassVal & 0x80)) {  // non-zero, not flattened
        chunk_data.grass.vertex_lengths[index] = 1.0f;
        needsGrassUpdate = true;
    } else {
        chunk_data.grass.vertex_lengths[index] = 0.0f;
    }
}
if (needsGrassUpdate && mesh) {
    chunk_data.grass.CreateFromMesh(*mesh);  // regenerate grass particles
}
```

Enable grass on the Wicked terrain:
```cpp
wickedTerrain.SetGrassEnabled(true);
wickedTerrain.grass_properties.length = 30.0f;  // tune to match old grass visual scale
```

### Limitations (acceptable for this phase)

- 46 grass types collapse to 1 visual type (Wicked HairParticleSystem has one material)
- Grass density is per-vertex (~67x67 per chunk) not per-texel (4096x4096 global)
- Grass variety can be improved in a future phase

### Verify

- Paint grass on old terrain, toggle to new: grass appears in painted areas
- Flattened areas (under buildings) should have no grass
- Unpainted areas should have no grass (unlike Wicked's default Perlin-based placement)

---

## Phase 5: Colored Cylinder Trees

**Goal**: Tree positions visualized as colored cylinders on the new terrain.

### Changes

**Modified files**:
- `Guru-WickedMAX/GGTerrain/GGTerrainWicked.cpp` — cylinder mesh creation, instanced tree rendering
- `Guru-WickedMAX/master_part1.cpp` — add tree draw call in `customDraw_Opaque` when toggle active

### Implementation

Create a shared cylinder MeshComponent at init. For each frame when `ggterrain_use_wicked_terrain`:
1. Iterate `pAllTrees[0..numTrees]` for visible/valid trees within camera range
2. Create/update ObjectComponent entities at tree (x, y, z) positions
3. Material color = `referenceColors[treeType & 31]` (same palette as reference color mode)
4. Scale cylinder height/radius by tree scale factor from `InstanceTree::data` bits [17-24]

Use entity pooling: pre-create a fixed pool (e.g., 10000 entities), show/hide per frame.

### Verify

- Toggle to new terrain: colored cylinders at tree positions
- Compare positions against old terrain's tree overlay dots — should match
- Different tree types = different colors

---

## Phase 6: Sculpt/Paint Invalidation Bridge

**Goal**: Editor sculpting and painting update the Wicked terrain in real-time.

### Changes

**Modified files**:
- `Guru-WickedMAX/GGTerrain/GGTerrain_part0.cpp` — add bridge calls in `GGTerrain_InvalidateRegion()` and `GGTerrain_InvalidateEverything()`
- `Guru-WickedMAX/GGTerrain/GGTerrainWicked.cpp` — implement `GGTerrainWicked_InvalidateRegion()`

### Implementation

When terrain is invalidated (sculpt or paint), convert the world region to Wicked chunk coordinates and mark those chunks as `invalidated = true`. The next `Generation_Update()` call will regenerate them (including re-running the post-process with updated pMaterialMap/pGrassMap data).

```cpp
void GGTerrainWicked_InvalidateRegion(float minX, float minZ, float maxX, float maxZ) {
    for (auto& [chunk, chunk_data] : wickedTerrain.chunks) {
        float cx = chunk.x * (chunk_width - 1) * wickedTerrain.chunk_scale;
        float cz = chunk.z * (chunk_width - 1) * wickedTerrain.chunk_scale;
        float halfSize = (chunk_width - 1) * wickedTerrain.chunk_scale * 0.5f;
        if (cx + halfSize >= minX && cx - halfSize <= maxX &&
            cz + halfSize >= minZ && cz - halfSize <= maxZ) {
            chunk_data.invalidated = true;
        }
    }
}
```

Also clear the chunk from `knownChunks` tracking set so it gets post-processed again after regeneration.

### Verify

- Toggle to new terrain, sculpt (raise/lower) — mesh updates within a few frames
- Paint a material — blendmap updates on affected chunks
- Paint grass — grass appears/disappears

---

## Phase Dependency Graph

```
Phase 0: Toggle + Empty Terrain
  |
  +---> Phase 1: Heightmap Feed
  |       |
  |       +---> Phase 2: 4-Layer Auto Materials
  |       |       |
  |       |       +---> Phase 3: Painted Materials (N-layer)
  |       |               |
  |       |               +---> Phase 4: Grass
  |       |               |
  |       |               +---> Phase 6: Sculpt/Paint Invalidation
  |       |
  |       +---> Phase 5: Colored Cylinder Trees (parallel with 2-4)
```

Phases 2-4 are sequential (each builds on previous). Phase 5 can be done in parallel after Phase 1. Phase 6 requires Phase 3.

---

## Devil's Advocate Risks — Addressed

| Risk | Rating | Mitigation |
|---|---|---|
| **Material 31->4 limit** | ~~CRITICAL~~ **RESOLVED** | N-layer blendmap support confirmed. `enable_blendmap_layer()` extends beyond 4. |
| **Flat areas missing** | ~~CRITICAL~~ **RESOLVED** | Custom Modifier calls `CalculateHeightWithHeightmap()` which includes flat area stamps. |
| **Height query consistency** | HIGH -> LOW | Keep old CPU data running. Height queries unchanged. Same source data feeds both systems. |
| **Heightmap bilinear interp** | HIGH -> LOW | Custom Modifier bypasses Wicked's `HeightmapModifier` truncation. Uses GG's interpolated height function. |
| **Physics collision** | MEDIUM -> LOW | Keep Bullet physics + old `GGTerrainPhysicsShape`. Disable Wicked physics (`SetPhysicsEnabled(false)`). Same CPU height data = same collision. |
| **Coordinate mismatch** | HIGH | Set `bottomLevel=-20000`, `topLevel=20000`, `chunk_scale=8`. Validate in Phase 1 with visual comparison. |
| **Performance (dual)** | HIGH -> MEDIUM | Skip old virtual texture atlas (biggest GPU cost) when toggle active. CPU overhead (~83MB heightmap data) is acceptable. |
| **Editor tools** | HIGH -> MEDIUM | Tools still write to old data structures (`pMaterialMap`, `pGrassMap`, sculpt arrays). Phase 6 bridges invalidation to Wicked chunks. |
| **Save/load** | LOW | Save/load format unchanged — still uses old data structures. Wicked terrain is reconstructed from old data on each load. No new save format needed. |
| **Grass 46->1 types** | MEDIUM | Accepted for initial port. Density placement is correct. Visual variety is a future improvement. |
| **Render pipeline conflicts** | LOW | Old draw callbacks return early when toggle active. Wicked renders through scene pipeline. No overlap. |
| **Water interaction** | LOW | Sync `wickedTerrain.weather.oceanParameters.waterHeight` from GG's water height each frame. |

---

## Verification Strategy

Each phase has an A/B test:
1. Launch MAX, load a level with terrain
2. Toggle to old terrain: observe reference color mode
3. Toggle to new terrain: compare same camera position
4. Check: terrain shape, material placement, grass, trees, editor tools

Use the automation harness (`WETEST.md`) for scripted comparisons across demos.

---

## Critical Files Reference

All modifications are GameGuru-side only. **Zero Wicked Engine files are modified.**

| File | Role in Port |
|---|---|
| `Guru-WickedMAX/GGTerrain/GGTerrainWicked.cpp` (NEW) | All new Wicked terrain wrapper code: init, update, post-process, cylinder trees |
| `Guru-WickedMAX/GGTerrain/GGTerrainWicked.h` (NEW) | Header for above |
| `Guru-WickedMAX/GGTerrain/GGTerrain_part0.cpp` | Toggle var, skip virtual tex, expose height calc, invalidation bridge |
| `Guru-WickedMAX/GGTerrain/GGTerrain.h` | Toggle extern, height calc wrapper declaration |
| `Guru-WickedMAX/master_part1.cpp` | Draw callback gating, update loop integration |
| `Guru-WickedMAX/GameGuruMain.cpp` | Init call |
| `Guru-WickedMAX/GGTerrain/GGGrass.cpp` | pGrassMap data (read by post-process loop) |
| `Guru-WickedMAX/GGTerrain/GGTrees_part0.cpp` | pAllTrees data (read by cylinder renderer) |

**Wicked Engine files used (read-only, via public API)**:
| File | What we use |
|---|---|
| `wiTerrain.h` | `Terrain` struct, `ChunkData`, `Modifier`, `Chunk`, `BlendmapLayer` — all public |
| `wiTerrain.cpp` | `Generation_Update()`, `CreateChunkRegionTexture()` — called via public API |
