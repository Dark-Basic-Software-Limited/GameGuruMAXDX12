# SCRATCHPAD — Terrain System Port

## Current State (2026-07-17)

The terrain port to the **Wicked Engine DX12 terrain system** is well underway. **Phases 0–4 are complete**, with painted PBR materials rendering across the full terrain and **3D grass rendering through Wicked Engine's `HairParticleSystem`**, placed from GameGuru's painted grass map (`pGrassMap`). The new terrain is now the **default** (Y key toggles back to old terrain).

**This file is the living roadmap.** `TERRAINPORT.md` is the architectural reference (set during Phase 0 design); refer here for the current phase status.

### Completed Milestones

| Date | Milestone | Key Commits |
|------|-----------|-------------|
| 2026-02-25 | **Phase 2 complete**: 4-layer auto materials (base/slope/low-alt/high-alt) via Wicked VT pipeline | `1d403a5f`, `d7eeee94` |
| 2026-02-25 | **Phase 3 complete**: Painted materials (N-layer blendmap injection), verified on Island Showdown (9 extra mats, 143 chunks) | `76b5336d` → `b7f81afc`, `bd8ad61a` |
| 2026-02-25 | Default to new Wicked terrain, Y key toggles old | `20dba693` |
| 2026-02-25 | PRESS_KEY automation harness command | `92389774` |
| 2026-03-01 | **Camera excursion fix**: 3 interacting bugs (spurious restart, stale chunk keys, generation-overwrite race) | `5d647db6` |
| 2026-03-02 | **Chunk scale tuning**: 8 → 40 → 80, near-invisible generation popping, U key wireframe debug | `c23aae39`, `4253914f` |
| 2026-03-03 | **Profiler harness fix + Performance Data panel diagnostic deployed**: `ENABLE_PROFILER` now toggles both `bProfilerEnable` and `wi::profiler::SetEnabled()`; raw `GetTextData()` + stats overlay added to isolate cascading-duplicates bug | `5233fb3c` |
| 2026-03-04 | **Animation System Fix**: pause/play culling via `GGAnimBridge_PreUpdate/PostUpdate` — Update-Wicked 25.81→11.36ms (−56%), FPS 27→36 (+33%) | `039ee1da`, `89873913` |
| 2026-03-04 | **Performance Data panel cascading-duplicates bug fixed** (resolves PERFORMANCE.md Phase 10) | `c4d81543` |
| 2026-05-27 | **Terrain Texture Transition** — small `GGTerrainWicked.cpp` tweak + DarkLUA vcxproj addition | `c6474e94` |
| 2026-05-29 | **Phase 4 complete**: 3D grass via Wicked `HairParticleSystem`, per-chunk placement from `pGrassMap`, distance-LOD chunks, natural green meadow look. Work originally lived only on `claude/frosty-ritchie-f7efe6` and was recovered via cherry-pick on 2026-06-17 after my mirror-overwrite incident destroyed the uncommitted local copy (see [feedback_check_main_repo_status.md](../../../../leeba/.claude/projects/D--max-GameGuruMAXDX12/memory/feedback_check_main_repo_status.md)). | `5070b264`, `f3d6dd92`, `a7fc7618`, `29c979ed`, `c84225e9` |
| 2026-06-18 | **Grass orientation fixed**: blades had been leaning chaotically on slopes. Two **Wicked Engine** bugs found, both exposed by our `Terrain.chunk_scale = 80`: (1.1) `wiTerrain.cpp` cross-product used a bare `+ 1` for the horizontal step instead of `+ chunk_scale`, amplifying slope by 80× and giving near-horizontal stored vertex normals; (1.2) `HairParticleSystem` consumes stored per-vertex normals that are computed from a fixed `(V, V+x, V+z)` reference triangle which doesn't match the actual mesh triangulation, so even with 1.1 fixed the grass simulate CS got chaotic normals. Fix 1.1 applied to `wiTerrain.cpp`; fix 1.2 applied as in-shader face-normal recompute in `hairparticle_simulateCS.hlsl`. Both documented in [WICKED_ENGINE_CHANGES.md](WICKED_ENGINE_CHANGES.md) for upstream brief. | Wicked: `wiTerrain.cpp`, `hairparticle_simulateCS.hlsl` |
| 2026-06-18 | **Per-grass-type DDS textures wired up**: the renderer was using a single Wicked wheat atlas (`grassparticle.png`) for every painted type — so Course Grass / Tall Grass / Daisies / Kelp all rendered identically. Replaced with per-type material cache: each entry in `GGGrass::grassFiles[]` (46 DDS files in `Files/grassbank/`) gets its own `MaterialComponent` + `HairParticleSystem` appearance template (length / width / billboards / stiffness tuned per category — Course/Short/Tall/Wild/Weed/Flower/Kelp/Seaweed). `ProcessGrassChunks` now buckets each chunk's grassy vertices by type id (read from `pGrassMap`, value-2 = `grassFiles[]` index) and spawns one hair entity per used type. Per-chunk strand cap is split across types so a 3-type chunk doesn't triple-spend VRAM. Materials and the `_normal.dds` siblings for kelp/seaweed are loaded lazily on first sighting. Value 1 (legacy "default paint") aliases to type 0 (Course Grass mat1). `SET_GRASS` knobs now apply uniformly across all types. Verified with a Red Flower debug override — every blade rendered as red flower, confirming the per-type path is the active renderer. | `e1a3c7ed` |
| 2026-06-18 | **Grass refinements — 2× size for DX11 parity + smooth sway**: side-by-side DX11 vs DX12 showed our blades half-size and sway visibly choppy. Per-category length AND width values doubled in `BuildGrassAppearance` to match DX11 visual scale. The choppy sway turned out to be Wicked's static-mesh hair default: `CreateFromMesh` picked `R16G16B16A16_UNORM` for the position buffer, then `hairparticle_simulateCS` remaps each frame's per-strand position to UNORM across the whole hair-system AABB (≈5280×5280 inch for one of our chunks at `chunk_scale=80`), giving a quantization step ≈ 0.08 inch. Micro-sway of 1–2 inch tip motion only crossed ~12–25 discrete steps, looking exactly like float→int rounding. Fix: override `hair.position_format = R32G32B32A32_FLOAT` between `CreateFromMesh` and `CreateRenderData`. Trade-off ~2× position-buffer VRAM, bounded by the existing per-chunk strand cap. Visually dense lush grass that draws far into the distance with no perf hit. | `669831a2` |
| 2026-06-18 | **Wicked-terrain brush cursor working** (towards re-enabling terrain paint): the legacy GG terrain pixel shader drew a procedural ring at `terrain_mouseHit.xy ± brushSize·0.02` (color `RGB(37, 245, 43)`), but the new Wicked terrain shader has no such path. Wired a `DecalComponent` entity in `GGTerrainWicked.cpp` that projects a 1024-px brush-ring PNG straight down onto whatever surface sits below it (terrain). Texture generated procedurally to match the DX11 colour exactly (lives at `Files/editors/gfx/brush_ring.png`). Decal entity is created lazily, rotated −90° around X so its local −Z faces world −Y, and its transform/baseColor.w are updated each frame from a new public API `GGTerrainWicked_SetBrushCursor(visible, x, y, z, size)`. Diagnostic discovery: `GGTerrain_Update` has an `if (ggterrain_use_wicked_terrain) return;` short-circuit at line ~9800 that meant any hook past that point was dead — the new brush block sits right before that return and re-does the pick ray via `wiInput::GetMouseState()` + `wiRenderer::GetPickRay()` + `GGTerrain_RayCast()`. Verified working on island.fpm — green ring follows the cursor in grass / paint / sculpt / trees modes. | `7c91786b` |
| 2026-06-18 | **Grass paint live in Wicked mode** — left-click-drag now paints into `pGrassMap` and the Wicked grass renderer rebuilds the affected chunks ~10×/sec mid-stroke + final on release. Painted area updates from a new `GGGrass_TakeMapDirty(minX,minZ,maxX,maxZ)` accessor that returns the accumulated brush AABB; the Wicked side intersects each tracked chunk against the AABB and only invalidates the ones the stroke actually touched. **Big perf killer found and removed**: legacy `GGGrass_UpdateInstances()` ran 64×6250 = 400 000 random instance positions per paint frame, each acquiring `terrainlock` twice for `GetNormal`+`GetHeight`. It feeds the legacy GG grass shader which renders nothing in Wicked mode — pure overhead, and per-frame cost scaled with painted area, taking FPS from 70 → 12 over a 20-sec stroke. Skipped in Wicked mode via `ggterrain_use_wicked_terrain` gate. The 16 MB-per-call `GGGrass_UploadGrassMap` legacy GPU texture write is also gated to Wicked-off mid-stroke (still runs once on release for parity). Final per-stroke cost: ~10 chunk-invalidation publishes/sec, each touching 1–2 chunks. FPS holds at 70 throughout long paint strokes. | `1502723b` |
| 2026-06-18 | **Flower category tuned** — first 4 grass categories looked correct but the Red Flower came out as a flat horizontal smear. Wicked's `hair.width` is a **multiplier on length** (rendered width = `width × xHairAspect × length`), so `width=5` with `length=9` rendered a billboard ~5× wider than tall. Setting `length=60×sf=30` and `width=1.0` for `GCAT_FLOWER` now produces a tall thin stalk silhouette matching DX11 — flowers peek visibly above the surrounding grass. Same width-as-multiplier knowledge will apply to kelp/seaweed when underwater scenes get tested. | (this commit) |
| 2026-06-19 | **Grass paint stability ladder (Stages 1-3)** — paint strokes were reshuffling already-placed grass blades because each per-chunk hair entity was re-allocated and re-seeded every time `vertex_lengths` changed. Three-stage fix: **Stage 1** locked `triangleCount` via vertex_lengths epsilon trick (all set to ε > 0 so `CreateFromMesh` always includes every triangle in the index buffer) + fixed strandCount per LOD tier (independent of paint state); **Stage 2** persists the per-`(chunk, type)` hair entity across paint events instead of deleting/recreating; **Stage 3** added a new public `HairParticleSystem::UpdateVertexLengthsBuffer()` method on the Wicked side that rebuilds only the `vertexBuffer_length` GPU buffer, leaving `generalBuffer` / `simulation_view` / `vb_pos[0/1]` intact so each strand's `prevTail`/`currentTail` simulation state survives the update — no per-paint-event settling pop on the wind animation. Documented in [WICKED_ENGINE_CHANGES.md](WICKED_ENGINE_CHANGES.md) entry 1.3. | `e078c9e9`, `33b6e47b` |
| 2026-06-19 | **Stage 3 Option B — per-strand paint-mask visibility** — at `chunk_scale = 80` the vertex grid sits ~80 inches apart while paint cells are ~5 inches, so a brush-15 blob produced a ~4×-wider visible patch through bary-interpolated vertex_lengths. Option B: each strand samples the GG paint mask (R8_UNORM `texGrassMap`) at its own world XZ in `hairparticle_simulateCS.hlsl` and zeros `strand_length` when the cell type doesn't match its `xHairGrassType`. Engine-side hook added to `HairParticleCB` + `HairParticleSystem` (`grass_type`, `grass_map_inv_world_size`, `grass_map_origin_x/z`, `grass_visibility_texture`); GG sets these per-`(chunk, type)` entity. `grass_type = 0` default makes the feature a no-op for upstream callers — clean upstream-PR candidate (entry 1.4). With per-strand visibility the C++ per-vertex 7×7 multi-sample + coverage scaling became redundant — stripped; `ProcessGrassChunks` now does a single chunk-AABB scan of `pGrassMap` via a new `GGGrass_ScanRegion()` to derive `typesSeen[t]` for create/destroy decisions. Brush footprint == grass footprint, no more 4× amplification. | `5fb06f58`, `e3844c53`, Wicked `6d298e17` |
| 2026-06-19 | **Stage B.5 — DX11-parity blade sizing** — Wicked grass read shorter/wider than DX11 reference (olive straps vs bright-green thin strands). Mapping bug: per-DDS `_SF_x.xx` scaleFactor was being applied to **length** but the legacy DX11 VS (`GGGrassVS.hlsl:45`) applies it to **width** only (`posOrig.x *= scaleFactor`). Length anchored to DX11's `grass_scale = 40` baseline (Course=40, Short=30, Tall=56, Wild=35); width = SF for grass categories (Tall Grass SF=0.87 gives thin strap, Short SF=1.4 gives wider stubby). Within-category visual differences now purely texture-content driven — matches DX11. Later spot-fix: Red Flower hard-coded `length=65` was the only remaining height "hack"; reverted to 40 for DX11 waist-height parity. | `61c2630c` |
| 2026-06-19 | **Stage B.6 — Grass Draw Distance slider wired to the new system** — the editor slider was a vestige from the legacy DX11 path; it set `gggrass_global_params.lod_dist` but nothing in the Wicked-mode grass path consumed it (visible distance was hard-coded by `g_grassLODChunks = 2.5f` + per-entity `viewDistance = 5000`). Slider now drives both knobs: per-strand `viewDistance = slider + 2500` (matches the DX11 `grassRadius = lod_dist + GGGRASS_LOD_TRANSITION` formula), and chunk-entity outer ring `outerC = viewDistance/chunkStride + 1.0` — kept **1 chunk further out than the per-strand cull** so chunks enter the entity ring with their near edges still past viewDistance and strands fade in one-by-one instead of whole chunks popping. Live-update path in `GGTerrainWicked_Update` syncs `viewDistance` on every existing grass entity when the slider changes (drag-responsive). Hysteresis: tier downgrades require ringDist to exceed the boundary by 0.5 chunks before they fire (kills camera-wobble pop-out). Tier transitions on chunks that retain entities now force `fullReset` so strandCount rebuilds at the new tier — fixes a missed case where a chunk coming back from tier 1 to tier 3 kept its sparse 5k-strand entity instead of upgrading to 100k. SET_GRASS `lodchunks N` still hard-overrides via `g_grassLODChunksOverride`. | `d6fe0bf6` |
| 2026-06-19 | **Stage B.8 — kelp/seaweed/weed/flower widths -> pure DX11 mapping** — community-visible regression on the island level: kelp came out as ~10-ft towering magenta plants that tanked FPS from 420 → 15 through overdraw. Same overreach pattern I already fixed for the red flower — hard-coded length past DX11's `grass_scale = 40` baseline plus an arbitrary width multiplier on top of the per-DDS SF value. Full sweep on the remaining categories: kelp length 75→40 + width sf×6→sf, seaweed length 110→40 + width sf×4→sf, flower width sf×2→sf, weed width sf×2→sf. All 8 categories now follow the same pure DX11 mapping: length gives category tier variance, width = scaleFactor from the DDS filename suffix. Within-category size variance is purely texture-content driven, matching what DX11 renders for the same DDS set. | `db6bf3bc` |
| 2026-06-19 | **Stage B.9 — Custom Grass Palette (community feature)** — the palette was hardcoded to 22 stock slots with no way to add user grass textures. Now supports **22 stock + up to 42 custom** (cap set by the `paint_type` uint64 bitmask; total 64). New "Add New Grass" button opens a DDS file dialog and populates the first empty slot ≥ 22; "Delete Grass" clears the highlighted custom slot; stock slots (0-21) protected from deletion by button gating. Custom entries persist to `.fpm` (existing `M-Visuals` save/load handles slots 22..127 already since `sGrassTextures[128]`). **Byte encoding**: paint code stores stock as `grassMaterialTypes[mat][slot]+2` (existing mat-variant behavior preserved), custom as `slot+26` directly (real_type = slot+24, so stock 0..45 and custom 46..87 stay disjoint). **Wicked side**: appearance/material arrays sized `GGGRASS_TOTAL_REAL_TYPES = 88`, `BuildGrassAppearance` gets a second loop that reads custom-slot filenames via a new `GGGrass_SetCustomSlotFilename`/`GetCustomSlotFilename` registry, `BuildGrassMaterial` dispatches (stock DDS path from `grassFiles[]` table, custom DDS path from `Files/<sGrassTextures[slot]>`). **Level-load timing fix**: sync from `sGrassTextures[]` → `GGGrass_SetCustomSlotFilename` hooked into `Wicked_Update_Visuals` (fires once per level load), NOT just when the vegetation panel renders — so custom grass shows immediately without needing to enter grass mode. **Delete+re-add-with-different-DDS fix**: dirty poll doesn't just clear the material cache; it also actively removes existing hair entities for custom real_types across every tracked chunk (short-circuits Phase 2's "type still painted, entity exists → skip" branch that would otherwise keep the OLD material snapshot). Stock entities are left alone. First community-facing extensibility of the new grass system. | `b6857051` |
| 2026-07-10 | **Stage B.10 — Advanced Grass Settings wired to the new system (DX11-baseline port)** — the four "View Advanced Settings" controls (Grass Scale, Grass Start/End Altitude, Grass Start/End Altitude Underwater, Match Terrain Color) were all vestiges from the DX11 path. On the DX11 side they gated `GGGrass_UpdateInstances`, but that function is off in Wicked mode (perf gate, commit `1502719b`), so the sliders + the "Populate Vegetation Everywhere" / "Clear All Vegetation" buttons all shipped-but-dead. Ported one by one, back-to-baseline: **(1) Auto-resolve for `pGrassMap` value `1`** — Match Terrain Color paint left `1` in each cell as an "unresolved" marker; DX11 rewrote each cell with a terrain-material-appropriate real_type (or seaweed 43 if underwater) on the next UpdateInstances pass. `GGGrass_ScanRegion` now does the same rewrite inline during its per-chunk scan; a new `s_gggrass_map_upload_pending` flag + `GGGrass_TakePendingMapUpload()` accessor triggers a single R8 texGrassMap re-upload per frame so the Option B compute-shader visibility check picks up the resolved bytes. **(2) Clear/Populate propagate through the Wicked cache** — `GGGrass_RemoveAll` and `GGGrass_AddAll` write pGrassMap in bulk but never touched the per-chunk hair entity cache OR the GPU texture; new `s_gggrass_full_rebuild_pending` flag drives `ForceGrassRebuild()` from `GGTerrainWicked_Update`, and both bulk writers now also fire `GGGrass_UploadGrassMap()`. **(3) Per-strand slope filter** — DX11 filtered at 0.7 normal-Y in `UpdateInstances`; CPU cell-scans against the DX11 normal map produced ragged cliff edges (coarse per-chunk normal texels vs. ~4.8-unit grass cells) and a per-strand distribution can still fling strands from flat cells onto adjacent cliff triangles. Filter moved into `hairparticle_simulateCS.hlsl` on the face normal of the triangle each strand sits on (already computed in-shader as the fix-1.2 grass-orientation workaround). Gated on `xHairGrassType != 0u` so upstream Wicked hair is unaffected. **(4) Per-strand altitude filter** — DX11 gated by height band above and below the water plane; same shader block now compares `base.y` against `xHairGrassMinHeight` / `xHairGrassMaxHeight` (or the underwater pair, selected by `xHairGrassWaterHeight`). Five new floats on `HairParticleCB` with defaults spanning `[-1e30, +1e30]` = "no filter" for upstream callers. `ApplyGrassAltitude()` in GG syncs the CB values from `gggrass_global_params.*` + `g.gdefaultwaterheight` every frame → sliders drag live without clear/repop. **(5) Live Grass Scale slider** — the DX11 legacy VS multiplied `IN.position * grass_scale`; in Wicked, Stage B.5 hard-anchored per-category lengths to the 40 baseline. Now `BuildGrassAppearance` snapshots each type's baseline `length` into `g_grassBaseLength[]`, and `ApplyGrassScale()` writes `appearance[t].length = base * (slider/40)` on templates + live entities every frame. Because the hair CS derives quad width from length (`quad_width = hair.width * xHairAspect * hair.length`), scaling length alone gives DX11-parity uniform blade scaling. **Filter design decisions doc'd in [WICKED_ENGINE_CHANGES.md](WICKED_ENGINE_CHANGES.md) entry 1.5** — both filters are single branches inside the existing `xHairGrassType != 0u` block, so the whole thing is a candidate upstream-PR delta of ~40 shader lines + 5 CB floats + 5 C++ mirror fields, zero behavioral change for callers that leave `grass_type = 0`. | (this commit) + Wicked `hairparticle_simulateCS.hlsl`, `ShaderInterop_HairParticle.h`, `wiHairParticle.{h,cpp}` |
| 2026-07-12 | **Harness OPEN_PROJECT + CLICK_ONLY_LEVEL** — autonomous cold-launch → TESTPRO1 → screenshot A/B loop (~50s) | `f5c4866a` |
| 2026-07-12/13 | **Trees Phase 5 Stages 1-4.2** — cylinders → real trunk/leaf meshes → 10K ObjectComponent pool + nearest-N-to-camera pick | `7a842b78`..`18a95978` |
| 2026-07-13 | **Terrain grey-mountains fixed** — CPU-side per-vertex DX11-style blend (`ApplyDX11StyleAutoBlend`) | `42e927b8` |
| 2026-07-13 | **Trees Stage 4.3** — Wicked ImpostorComponent retired (its impostor render path ignores IsNotVisibleInReflections; baked white), SetNotVisibleInReflections on tree pool, water reflections clean | `2e0ad1ef` |

### What's Working Now

- **Height/slope auto-materials**: 4 PBR texture layers blend by slope/altitude via Wicked's VT compute shader
- **Painted materials**: Up to 32 unique materials, blendmap-injected per-vertex into Wicked chunks after generation
- **Camera excursion stability**: Entity tracking detects chunk recreation, smart OnPaintDataChanged avoids spurious restarts, painting runs after generation to prevent overwrite race
- **Chunk scale**: `chunk_scale=80` (~5280 units/chunk, ~132m), `generation=10` — fewer large chunks = less popping + better performance
- **Debug tools**: Y key toggles old/new terrain, U key wireframe overlay, reference color mode still available on old terrain
- **All 19 demo levels**: Confirmed working with new terrain (tested through automation harness)

---

## Why the Old System is Unfixable

The virtual texture system bakes material content into atlas pages using LOD-specific height maps (512×512 each, camera-centered, doubling coverage per level). Material selection (sand/grass/rock) depends on sampled height, so pages at different LOD levels bake different materials for the same world position. The page shift mechanism preserves page content across camera moves without regeneration, requiring content to be camera-independent — but the height map coverage is camera-centered. These constraints are fundamentally incompatible. Three fix attempts all failed:

1. **`heightLevel = 0`** — broke distant terrain after camera movement (do-while loop made heightLevel camera-dependent for far pages)
2. **`heightLevel` cap at 2** — same corruption pattern, just shifted further out
3. **Remove mesh LOD clamp in render shader** — distant textures broke because coarse chunks searched for nonexistent fine LOD pages

Full investigation details preserved in git history.

---

## Reference Visualization (Working — Old Terrain Debug Tool)

A flat-color debug rendering mode bypasses the virtual texture system entirely and displays raw terrain data directly. Still available as a validation tool when toggled to old terrain (Y key).

### Three overlays, all confirmed working

| Overlay | Source data | GPU texture slot | Visual |
|---|---|---|---|
| **Material** | `texMaterialMap` (painted material index per texel) + height/slope layer fallback from mesh vertex data | t55 | 32-color palette, one color per material |
| **Grass** | `pGrassMap` (4096×4096 uint8, bits 0-6 = type, bit 7 = flattened) | t56 | Green-tinted palette over terrain color, 70% lerp |
| **Tree** | `pTreeMap` (4096×4096 uint8, rasterized from `InstanceTree` positions, 3px radius circles, value = type+1) | t57 | Full palette color replacement at tree positions |

---

## Key Technical Patterns Established

### Two-phase chunk processing (Phase 3)
1. **Scan phase** (main-thread safe): Iterate `scene.objects` to find terrain chunks not yet processed
2. **Modify phase** (after `Generation_Cancel()`): Access `terrain->chunks` to write blendmap data

### Critical ordering: Paint AFTER Generation_Update()
`ProcessPaintedChunkBlendmaps()` must run AFTER `Generation_Update()`. Painting before generation means the default blending stage overwrites our blendmaps. VT invalidation from painting is picked up on the next frame (imperceptible 1-frame delay).

### Chunk recreation detection
`chunkKeyToEntity` map tracks which entity occupied each grid key. When Wicked removes distant chunks and regenerates with new entity IDs, the mismatch triggers repainting.

### Smart OnPaintDataChanged
Always clears `processedChunkKeys` (forces repaint), but only triggers `Generation_Restart()` if paint data contains material indices not already in `materialToSlot`. Prevents spurious restarts from deferred events.

### editableSize is already half-size
`GGTerrain_GetEditableSize()` returns the distance from center to edge (e.g., 9842.5 for Island Showdown). Editable area extends from `-editableSize` to `+editableSize`. Do NOT halve again.

---

## Next Steps

### Phase 4: Grass via Wicked Engine HairParticleSystem
**Goal**: Grass renders through Wicked Engine, placed from GameGuru's `pGrassMap`.

- Enable `wickedTerrain.SetGrassEnabled(true)` with tuned `grass_properties`
- Override `chunk_data.grass.vertex_lengths` from `pGrassMap` in the same post-process loop as Phase 3
- Call `chunk_data.grass.CreateFromMesh(*mesh)` to regenerate grass particles
- Flattened areas (bit 0x80) get length=0, unpainted areas get length=0
- 46 grass types collapse to 1 visual type initially (Wicked has one grass material per terrain)

### Phase 5: Colored Cylinder Trees
**Goal**: Tree positions visualized as colored cylinders on the new terrain.

- Create shared cylinder MeshComponent, entity pool (~10000)
- Iterate `pAllTrees[]`, place cylinders at tree positions with type-based colors
- Can be done in parallel with Phase 4

### Phase 6: Sculpt/Paint Invalidation Bridge
**Goal**: Editor sculpting and painting update the Wicked terrain in real-time.

- Hook `GGTerrain_InvalidateRegion()` → mark affected Wicked chunks as `invalidated = true`
- Clear from `processedChunkKeys` so they get re-processed with updated data
- Requires Phase 3 (already complete)

### Future Work (Post-Phase 6)
- Replace cylinder trees with real LOD-based tree models (and eventually rocks)
- Grass variety — map GG's 46 grass types to different visual appearances
- Strip old virtual texture page generation code once port is fully validated
- Smooth blending at paint boundaries (currently all-or-nothing per vertex)

---

## Phase Completion Status

```
Phase 0: Toggle + Empty Terrain          ✓ COMPLETE
Phase 1: Heightmap Feed                  ✓ COMPLETE
Phase 2: 4-Layer Auto Materials          ✓ COMPLETE (2026-02-25)
Phase 3: Painted Materials (N-layer)     ✓ COMPLETE (2026-02-25, excursion fix 2026-03-01)
Phase 4: Grass via HairParticleSystem    ✓ COMPLETE (2026-05-29; recovered to main 2026-06-17)
Phase 5: Trees — Stage 4.3 DONE (real meshes, 10K pool, nearest-N pick, impostors retired). Open: measure DX11 A/B horizon delta (maybe hand-rolled billboards), tree types >= 38, wind sway.
Phase 6: Sculpt/Paint Invalidation       ✓ COMPLETE (2026-07-18, d37342bd) sculpt+paint+undo verified on island beach; GGTerrainWicked_InvalidateRegion + Wicked-mode editor dispatch in GGTerrain_Update
Phase 4+: Grass rendering improvements   ← NEXT     orientation fix landed 2026-06-18 (see Wicked changes 1.1/1.2). Remaining: subsurface, density, atlas variety, paint UX. See "Phase 4 Notes" below
Perf:    Animation engine-side caching   ← ACTIVE   first step: PERFORMANCE.md "Active Performance Targets" — ScanAnimationDependencies + keyframe search
```

## Advanced Grass Settings — DX11 Baseline Port Plan (COMPLETE 2026-07-10, see Stage B.10 milestone row)

The vegetation panel's **View Advanced Settings** checkbox exposes four extra controls. Testing on `island.fpm` (custom slot painted with a blue plant) shows all four are effectively dead in Wicked mode — they live-edit the DX11 legacy grass path but the new Wicked hair-particle path never reads them (or reads a hardcoded value). Rather than wire them one-by-one on top of the shortcuts we already took, we go back to the beginning and reproduce the DX11 auto-populate / auto-resolve / gating logic in Wicked mode first. Then all four sliders (and any future advanced ones) plug into a system that actually consumes them.

### The four Advanced Settings — what they drive in DX11 vs Wicked today

| Label | Variable | DX11 path | Wicked path today |
|---|---|---|---|
| **Grass Scale** | `gggrass_global_params.grass_scale` (default 40) | Written into `grassConstantData.grass_scale` → consumed by `GGGrassVS.hlsl` per-vertex | **Ignored**. Stage B.5 hard-anchored per-category `length` to the DX11 `grass_scale=40` baseline in `BuildGrassAppearance` (`GGTerrainWicked.cpp`). The slider moves but nothing rebuilds. |
| **Grass Start/End Altitude** | `gggrass_global_params.min_height` / `max_height` | Gated per-instance in `GGGrass_UpdateInstances()` (`GGGrass.cpp:447`) — instances outside the altitude band are skipped | **Ignored**. `UpdateInstances` is gated OFF in Wicked mode (commit `1502723b`, "Big perf killer found and removed"). Wicked's per-strand paint mask (`hairparticle_simulateCS.hlsl`, Stage 3 Option B) has no altitude gate. |
| **Grass Start/End Altitude Underwater** | `gggrass_global_params.min_height_underwater` / `max_height_underwater` | Same gate as above, active branch when `height ≤ waterheight` (`GGGrass.cpp:452`) | **Ignored** (same reason). |
| **Match Terrain Color** | `gggrass_global_params.paint_material` | When set, paint writes value `1` to `pGrassMap`. `UpdateInstances` then reads `1`, looks up the terrain material at that XZ (or forces seaweed if underwater), and rewrites the cell with the appropriate real type (`GGGrass.cpp:455-473`). This is the DX11 **auto-resolve**. | **Broken workaround**. Painted-1 cells are aliased to type 0 (Course Grass) in `ProcessGrassChunks` (SCRATCHPAD line 25). No material lookup, no underwater→seaweed, no rewrite. |

### The DX11 "automated grass population" that isn't yet in Wicked

Two entry points feed the same core logic in `GGGrass_UpdateInstances()`:

1. **Interactive paint with Match Terrain Color** — brush writes value `1` per cell → `UpdateInstances` resolves it to a real type based on terrain material or underwater state.
2. **"Populate Vegetation Everywhere" button** (`GGGrass_AddAll`, `M-TerrainNew_part1.cpp:1145`) — density-based bitmap fill of `pGrassMap`. **NB**: `AddAll` itself has no slope / altitude / underwater gating — it fills every cell that passes the density hash. It relies on `UpdateInstances` running afterwards to filter out unsuitable cells (slope `ny < 0.7` at `GGGrass.cpp:438`, altitude bands at `:447`/`:452`, water→seaweed remap at `:460`). In Wicked mode `UpdateInstances` never runs, so **AddAll currently populates cliffs, mountaintops, and underwater seabed with the same grass**.

### The plan — "back to the beginning" before wiring sliders

Port the DX11 baseline into the new Wicked pipeline in this order. Each step is testable on its own before we touch the advanced-settings sliders:

1. **Auto-resolve for painted `pGrassMap` value `1`** — during the per-chunk `GGGrass_ScanRegion` / `ProcessGrassChunks` pass, when we see a cell with value `1`, look up the terrain material at that XZ (or force seaweed 43 if underwater) and rewrite the cell just like `UpdateInstances` does. Replaces the Stage-3-era "alias to type 0" workaround. Makes **Match Terrain Color** produce material-appropriate variety in Wicked mode.
2. **Slope gate** (`ny < 0.7`) — apply in the same scan pass; painted cells on steep terrain get cleared to 0. Fixes the "grass on cliffs" case that DX11 never had.
3. **Altitude gates** (above-water and underwater bands) — apply in the same scan pass, reading the four `min_height` / `max_height` / `_underwater` globals. Makes the two altitude sliders live in Wicked.
4. **`GGGrass_AddAll` — also apply slope + altitude filtering** at write time (or accept that step 1-3 will filter them on the next scan; either works — pick whichever gives a snappier UX).
5. **Grass Scale slider** — treat as a category-scale multiplier applied on top of the DX11-baseline per-category `length` in `BuildGrassAppearance`. Live-update: refresh `MaterialComponent` / `HairParticleSystem` `length` fields when the slider moves (same pattern as B.6's live `viewDistance` sync).

After steps 1-5, all four Advanced Settings will do what their labels promise, and the community-facing "advanced per-category / per-slot tuning UI" (the B.9 hand-off item) can layer on top without re-doing baseline work.

### Files that will change

- `Guru-WickedMAX/GGTerrain/GGTerrainWicked.cpp` — `ProcessGrassChunks`, `BuildGrassAppearance` (live scale + gating)
- `Guru-WickedMAX/GGTerrain/GGGrass.cpp` — `GGGrass_ScanRegion` (currently just types-seen; needs to resolve `1` → real type, apply gates), possibly `GGGrass_AddAll` write-time filtering
- No Wicked-side changes anticipated for this track.

---

## Phase 4 Notes (Grass — what's in vs what's not)

**What's rendering**: Wicked Engine `HairParticleSystem` instances created per-chunk in `ProcessGrassChunks` (GGTerrainWicked.cpp). Placement reads `pGrassMap` via `GGGrass_GetGrassMap(x, z)`. Distance-LOD chunks keep VRAM safe (near chunks dense, far chunks scaled or none past `g_grassLODChunks`). Material/appearance template set up in `SetupWickedGrass`. SET_GRASS automation command exposed for live tuning.

**What's effectively dead**: the OLD GGGrass.cpp `GGGrass_Draw*` callbacks. They still exist and run every frame (`extern "C"` hooked into Wicked via alternate-name linkage) but produce no visible output — the new system has taken over visually. `gggrass_global_params.draw_enabled` (Z key) toggles them but no visible change. Keep them around for now — `pGrassMap` data layer is still authoritative, and painting tools depend on `GGGrass_Update`. Defer "delete old grass code" until Wicked grass is fully tuned and we're sure nothing else references the old draw path.

**Improvements to consider** (future passes):
- Per-grass-type appearance (current is single visual; GG paints up to 46 grass types into pGrassMap but we render one look)
- Tighter subsurface / wind tuning to match user expectations
- Paint-brush integration with the new system (currently the OLD GGGrass_Update_Painting writes to pGrassMap and the new system picks it up next chunk gen, but UX may not be smooth)

---

## Critical Files Reference

All GameGuru Core modifications are GG-side only. Wicked Engine changes are tracked separately in [WICKED_ENGINE_CHANGES.md](WICKED_ENGINE_CHANGES.md) — currently five applied changes — see the WICKED_ENGINE_CHANGES.md status table.

| File | Role in Port |
|---|---|
| `Guru-WickedMAX/GGTerrain/GGTerrainWicked.cpp` | All Wicked terrain wrapper code: init, update, material setup, blendmap injection |
| `Guru-WickedMAX/GGTerrain/GGTerrainWicked.h` | Header for above |
| `Guru-WickedMAX/GGTerrain/GGTerrain_part0.cpp` | Toggle var, skip virtual tex, material map accessors, OnPaintDataChanged callback |
| `Guru-WickedMAX/GGTerrain/GGTerrain.h` | Toggle extern, accessor declarations |
| `Guru-WickedMAX/master_part1.cpp` | Draw callback gating, update loop integration |
| `Guru-WickedMAX/GameGuruMain.cpp` | Init call |
| `Guru-WickedMAX/GGTerrain/GGGrass.cpp` | pGrassMap data layer (read by `ProcessGrassChunks`); OLD draw callbacks still present but produce no visible output |
| `Guru-WickedMAX/GGTerrain/GGTrees_part0.cpp` | pAllTrees data (Phase 5 will read this) |

Full architectural details in `TERRAINPORT.md`.

---

## Tech Debt (living list)

- **GPU particles fully disabled** — `GPUParticles_part0.cpp:1891` unconditional early return; shipping-visible feature gap (from `DX11_to_DX12_Shader_Porting_Plan.md` §10-11)
- **5 custom ImGui pixel shaders deferred** — blur/nowhite/noalpha/boost25/standard (`MIGRATION_PLAN.md`)
- **ImGui multi-viewport disabled** (`MIGRATION_PLAN.md`)
- **OpenXR VR path unmigrated** (`MIGRATION_PLAN.md`)
- **.ele level version debt** — DX12 reads max v341, production DX11 writes v342, entities silently don't load (crash guard only; `M-Entity_part3.cpp`)
- **Pre-existing E_INVALIDARG CreateTexture engine errors** (`DX11_to_DX12_Shader_Porting_Plan.md` §12)
- **Ten unconditional debug hotkeys** U/I/O/P/1-8/G in `GGTerrainWicked_Update` — gate behind a dev flag before release
