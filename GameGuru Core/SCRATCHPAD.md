# SCRATCHPAD — Terrain System Port

## Current State (2026-06-18)

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
Phase 5: Colored Cylinder Trees          ← ACTIVE   first step: shared cylinder MeshComponent + entity pool, iterate pAllTrees[]
Phase 6: Sculpt/Paint Invalidation       ← ACTIVE   first step: hook GGTerrain_InvalidateRegion() → mark Wicked chunks invalidated + clear from processedChunkKeys
Phase 4+: Grass rendering improvements   ← NEXT     improve Wicked grass look/behavior (subsurface tuning, density, wind, atlas variety, paint integration). See "Phase 4 Notes" below
Perf:    Animation engine-side caching   ← ACTIVE   first step: PERFORMANCE.md "Active Performance Targets" — ScanAnimationDependencies + keyframe search
```

## Phase 4 Notes (Grass — what's in vs what's not)

**What's rendering**: Wicked Engine `HairParticleSystem` instances created per-chunk in `ProcessGrassChunks` (GGTerrainWicked.cpp). Placement reads `pGrassMap` via `GGGrass_GetGrassMap(x, z)`. Distance-LOD chunks keep VRAM safe (near chunks dense, far chunks scaled or none past `g_grassLODChunks`). Material/appearance template set up in `SetupWickedGrass`. SET_GRASS automation command exposed for live tuning.

**What's effectively dead**: the OLD GGGrass.cpp `GGGrass_Draw*` callbacks. They still exist and run every frame (`extern "C"` hooked into Wicked via alternate-name linkage) but produce no visible output — the new system has taken over visually. `gggrass_global_params.draw_enabled` (Z key) toggles them but no visible change. Keep them around for now — `pGrassMap` data layer is still authoritative, and painting tools depend on `GGGrass_Update`. Defer "delete old grass code" until Wicked grass is fully tuned and we're sure nothing else references the old draw path.

**Improvements to consider** (future passes):
- Per-grass-type appearance (current is single visual; GG paints up to 46 grass types into pGrassMap but we render one look)
- Tighter subsurface / wind tuning to match user expectations
- Paint-brush integration with the new system (currently the OLD GGGrass_Update_Painting writes to pGrassMap and the new system picks it up next chunk gen, but UX may not be smooth)

---

## Critical Files Reference

All modifications are GameGuru-side only. **Zero Wicked Engine files are modified.**

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
