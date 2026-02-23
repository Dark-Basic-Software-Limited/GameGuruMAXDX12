# SCRATCHPAD — Terrain System Port

## Current State (2026-02-21)

The old virtual texture terrain pipeline has an **architectural LOD seam bug** that cannot be fixed without fundamental redesign. We are **porting terrain rendering to the new Wicked Engine DX12 terrain system** instead.

### Why the old system is unfixable

The virtual texture system bakes material content into atlas pages using LOD-specific height maps (512×512 each, camera-centered, doubling coverage per level). Material selection (sand/grass/rock) depends on sampled height, so pages at different LOD levels bake different materials for the same world position. The page shift mechanism preserves page content across camera moves without regeneration, requiring content to be camera-independent — but the height map coverage is camera-centered. These constraints are fundamentally incompatible. Three fix attempts all failed:

1. **`heightLevel = 0`** — broke distant terrain after camera movement (do-while loop made heightLevel camera-dependent for far pages)
2. **`heightLevel` cap at 2** — same corruption pattern, just shifted further out
3. **Remove mesh LOD clamp in render shader** — distant textures broke because coarse chunks searched for nonexistent fine LOD pages

Full investigation details preserved in git history.

---

## Reference Visualization (Working)

A flat-color debug rendering mode bypasses the virtual texture system entirely and displays raw terrain data directly. This is the **ground truth** for validating the new terrain system during the port.

### Three overlays, all confirmed working

| Overlay | Source data | GPU texture slot | Visual |
|---|---|---|---|
| **Material** | `texMaterialMap` (painted material index per texel) + height/slope layer fallback from mesh vertex data | t55 | 32-color palette, one color per material |
| **Grass** | `pGrassMap` (4096×4096 uint8, bits 0-6 = type, bit 7 = flattened) | t56 | Green-tinted palette over terrain color, 70% lerp |
| **Tree** | `pTreeMap` (4096×4096 uint8, rasterized from `InstanceTree` positions, 3px radius circles, value = type+1) | t57 | Full palette color replacement at tree positions |

### Toggle

`ggterrain_render_reference = 1` with `pref.iTerrainDebugMode` active, `R` key. Grass and tree overlays hardcoded on via `ggterrain_show_grass_map = 1`, `ggterrain_show_tree_map = 1`.

### Key implementation details

- **Material fallback**: Unpainted areas use height (`IN.worldPos.y`) and slope (`IN.normal.y`) layer rules — same logic as page gen shader but from mesh vertices, so no LOD dependency
- **Grass map upload**: `GGGrass_UploadGrassMap()` re-creates texture via `CreateTexture` + `SubresourceData` (old `UpdateTexture` API gone in this Wicked version). Uploads at: init, level load, flat area updates, paint brush strokes
- **Tree map rasterization**: `GGTrees_RasterizeTreeMap()` iterates all visible/valid `InstanceTree` structs → stamps into `pTreeMap` → uploads. Runs at init and level load only (no live paint update yet)

### Files modified for reference viz

| File | What |
|---|---|
| `GGTerrainConstants.hlsli` | `GGTERRAIN_SHADER_FLAG2_REFERENCE_COLOR` (0x0080) |
| `GGTerrainVirtualPBR_PS.hlsl` | Material map sampling (t55), grass (t56), tree (t57), 32-color palette, early-out block with ambient + editor overlays |
| `GGTerrain_part0.cpp` | Toggle variable, R key binding, flag bit, texture binds (t55/t56/t57) in main draw |
| `GGTerrain.h` | Extern for toggle variable |
| `GGGrass.cpp` | `GGGrass_UploadGrassMap()` implementation and call sites |
| `GGTrees.cpp` | `pTreeMap` array, `GGTrees_RasterizeTreeMap()`, `texTreeMap` creation/upload |

---

## Next Step: Port to New Wicked Engine Terrain System

### Goal

Replace the old virtual texture terrain rendering with the new Wicked Engine DX12 terrain system. The new system handles LOD transitions natively without a custom page-based pipeline.

### Validation plan

Run old terrain (reference color mode) and new Wicked Engine terrain side-by-side. Compare at matching world positions to verify the port preserves:

1. **Material layer assignments** — height-based rules, slope-based rules, and painted overrides all match
2. **Grass placement** — positions, types, and flattened state reproduced correctly
3. **Tree positions** — locations and type indices match

### Answers to port questions

**1. Material/texture layer blending (height & slope rules)**
The Wicked Engine repo is connected to the project. Use the new engine's terrain source directly to determine how to apply material/texture layer blending and map the existing height-based and slope-based layer rules onto it. The repo is the reference for how the new system exposes blending.

**2. Feeding painted material data into the new pipeline**
Same approach — the Wicked Engine repo will show how the new terrain material pipeline accepts input. Map `texMaterialMap` (the existing painted material index texture) into whatever mechanism the new system provides.

**3. Grass: adopt Wicked Engine's grass system**
The new Wicked Engine has its own grass system which is significantly better and should be adopted for rendering. The existing `GGGrass` system keeps its data structures and painting functions (brush tools, `pGrassMap`, type/flatten logic), but the actual rendering of grass geometry switches to the new Wicked Engine methods. Data flows: `pGrassMap` → new engine grass placement, existing paint tools → `pGrassMap` → upload to new engine.

**4. Trees: placeholder cylinders for this phase**
The tree system (`InstanceTree` structs with world positions) is confirmed independent of the terrain rendering pipeline — no coupling to old virtual texture UVs or page system. The new Wicked Engine does not have native tree support. For this phase, render **coloured cylinders** at each tree position to represent placement and scale, with colour denoting tree type. Future phases will replace cylinders with real LOD-based tree models, and eventually rocks.

### After port is validated

- Reference visualization can be removed or kept as a permanent debug tool
- Old virtual texture page generation code can be stripped out
