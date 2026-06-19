# GameGuruWickedMAX - Claude Code Instructions

## Session Start
- **Always read `WETEST.md` at the start of every session** before executing any commands.
- If asked to "run MAX" or "launch MAX", execute the **Execution Pattern** from WETEST.md steps 1-3 verbatim. Use the exact bash commands shown — do NOT substitute with `cmd`, `dir`, or any other commands. Do NOT verify paths, check if the EXE exists, or build first. Just run the steps. If the launch fails, STOP and report the error — do not retry with alternative commands.
- For all other tasks (testing, building, debugging, etc.), work autonomously as normal — use judgment, retry, and problem-solve without stopping to ask.
- Do NOT build or compile unless explicitly told to.

## Project Overview
This is a large C++ Windows x64 project (game engine) built with MSVC.
Solution file: `GameGuruWickedMAX.sln`

## Active Work (as of 2026-06-19)

Terrain port to Wicked Engine is in steady state through **Phase 4 + the Stage 3 Option B arc** (Wicked `HairParticleSystem` grass placed from GG's painted grass map; per-strand visibility, DX11-parity sizing, slider wired). Active tracks:

- **Phase 4+ — Grass complete in editor + Option B**: orientation fix (2026-06-18, Wicked-side, see [WICKED_ENGINE_CHANGES.md](WICKED_ENGINE_CHANGES.md) entries 1.1-1.4), per-grass-type DDS textures, FP32 sway, brush ring cursor, grass paint live (left-click-drag writes `pGrassMap`, Wicked renderer rebuilds affected chunks ~10×/sec). **Stage 3 Option B** (2026-06-19): per-strand grass-map sampling in `hairparticle_simulateCS.hlsl` — each blade checks the paint mask at its own world XZ instead of relying on bary-interpolated vertex_lengths, so brush footprint == grass footprint with no ~4× amplification. **Stage B.5** anchors blade lengths to DX11 `grass_scale = 40` and applies the `_SF_x.xx` scaleFactor to width per the legacy `GGGrassVS.hlsl:45` formula. **Stage B.6** wires the editor's **Grass Draw Distance** slider (previously a no-op vestige of the legacy path) into both per-strand `viewDistance` and the chunk-entity outer ring, with the outer ring kept 1 chunk further out so strands fade in one-by-one as the camera approaches instead of whole chunks popping. **Phase 5 next**: trees / colored cylinder placeholders driven from `pAllTrees[]`.
- **Phase 5 — Trees**: colored-cylinder placeholders driven from `pAllTrees[]` (`Guru-WickedMAX/GGTerrain/GGTrees_part0.cpp`); LOD tree meshes are post-Phase-6 work.
- **Phase 6 — Sculpt/Paint Invalidation**: hook `GGTerrain_InvalidateRegion()` → mark Wicked chunks invalidated + clear from `processedChunkKeys`.
- **Performance tuning**: pursue the items in `PERFORMANCE.md` → "Active Performance Targets" — engine-side animation caching (~17–20 ms potential) and the AI cost gap (24× DX11→DX12).

`SCRATCHPAD.md` is the living roadmap; `TERRAINPORT.md` is the architectural reference; `PERFORMANCE.md` carries the perf history and active targets.

## Build Commands
Invoke `build.bat` using its full quoted path (required because the project root contains a space):

```
"D:/max/GameGuruMAXDX12/GameGuru Core/build.bat" Debug          # Build Debug x64
"D:/max/GameGuruMAXDX12/GameGuru Core/build.bat" Release        # Build Release x64
"D:/max/GameGuruMAXDX12/GameGuru Core/build.bat" Debug rebuild  # Clean rebuild Debug x64
"D:/max/GameGuruMAXDX12/GameGuru Core/build.bat" Release rebuild # Clean rebuild Release x64
```

## Build System
- **Compiler**: MSVC v143 (VS 2022 toolset) via Visual Studio 2026 Community
- **Platform**: x64 only
- **Solution**: MSBuild-based (.sln/.vcxproj)
- **VS Install Path**: C:\Program Files\Microsoft Visual Studio\18\Community

## File Structure Notes
- Large .cpp files have been split into `_partN.cpp` files (e.g., `Entity_part0.cpp`, `Entity_part1.cpp`)
- When fixing compiler errors in split files, check if missing includes or forward declarations need to be added to the top of each part file
- Original header files (.h) are NOT split

## Important
- Always use `build.bat` to compile — it sets up the MSVC environment automatically
- Build errors will use the standard MSVC format and appear in the terminal
- The `$msCompile` problem matcher is configured in VS Code tasks

## Automation Test Harness
- See `WETEST.md` for full documentation of the file-based automation harness
- **Source**: `Guru-WickedMAX/AutomationHarness.cpp` — command/response via `auto_command.txt` / `auto_result.txt` in the EXE directory
- **EXE directory**: `D:\DEV\BUILD\GameGuru Wicked MAX Build Area\Max`
- **Key commands**: `GET_STATE`, `NAVIGATE`, `CLICK`, `CLICK_NODE`, `SELECT_DEMO`, `GET_PERF_DATA`, `SCREENSHOT`, `PRESS_KEY`, `PRESS_ESCAPE`, `ENABLE_PROFILER`, `DISABLE_PROFILER`, `GET_PROFILER_STATUS`
- **CLICK targets**: `play_game`, `edit_game`, `test_level`, `add_level`, `load_level`, `exit_screen_editor`
- **CLICK_NODE**: works with level nodes (loads into editor), screen/splash nodes (opens screen editor)
- **Profiler commands**: `ENABLE_PROFILER` / `DISABLE_PROFILER` control both `bProfilerEnable` (GameGuru flag) and `wi::profiler::SetEnabled()` (Wicked flag). Both must be set — editor code in `M-GridEditB_part3.cpp:75-78` actively disables the profiler every frame if `bProfilerEnable` is false
- **PRESS_KEY in editor mode**: Keys are injected into the terrain key system via `g_autoHarnessInjectedKey` (bypasses the `bImGuiGotFocus` gate that blocks `GGTerrain_CheckKeys()` in editor mode)
- **Crash diagnosis**: check `Guru-Crash.log` in the EXE directory for crash source file and line number

## DX12 Image/Texture System

The UI uses a two-layer image system:

1. **DarkBasic image list** (`m_List` in `CImageC_part0.cpp`) — stores image metadata, filenames, and (in DX11 mode) textures. In DX12 mode, textures are NULL (lazy-loaded).
2. **DX12 texture cache** (`g_TextureCache` in `GameGuru/Imgui/imgui_gg_dx12_bridge.cpp`) — `unordered_map<int, DX12CachedTexture>` keyed by image ID. GPU textures are loaded lazily on first render via `ImGui_DX12_GetOrLoadTexture()`.

### Key functions

| Function | File | Role |
|----------|------|------|
| `LoadImage` / `LoadImageSize` | `CImageC_part2.cpp` | Creates `m_List` entry with filenames. In DX12 mode, only reads file dimensions (no GPU texture yet) |
| `RemoveImage` | `CImageC_part0.cpp` | Deletes from `m_List` AND evicts from `g_TextureCache` |
| `DeleteImage` | `CImageC_part2.cpp` | Calls `RemoveImage` (via `DeleteImageCore`) |
| `ImGui_DX12_GetOrLoadTexture` | `imgui_gg_dx12_bridge.cpp` | Cache-first lazy loader: returns cached GPU texture or loads from disk |
| `ImGui_DX12_RemoveTexture` | `imgui_gg_dx12_bridge.cpp` | Evicts a single entry from `g_TextureCache` by image ID (deferred — see below) |

### Important: cache invalidation

When replacing an image at an existing ID (e.g., storyboard project switch), you MUST call `DeleteImage`/`RemoveImage` first. This evicts the DX12 texture cache entry so the new file gets loaded. Without eviction, `GetOrLoadTexture` returns the stale cached texture (fixed in commit 25713c1f).

### Important: deferred GPU resource deletion

`ImGui_DX12_RemoveTexture` does NOT free GPU resources immediately. It moves the `DX12CachedTexture` to a `g_PendingDeletes` queue with a 4-frame countdown (`DEFERRED_DELETE_FRAMES`). `ProcessPendingDeletes()` runs at the start of each `ImGui_DX12_RenderBridge()` call, decrementing counters and freeing textures + recycling SRV descriptor slots only after enough frames have passed.

This is required because DX12 has 2 frames in flight (`NUM_FRAMES_IN_FLIGHT = 2`). The GPU may still be executing draw commands from a previous frame that reference the texture. Freeing the `ID3D12Resource` while in-flight causes `DXGI_ERROR_DEVICE_REMOVED` (fixed in commit c8ec1739).

## Terrain System — Porting to New Wicked Engine Terrain

### Current status

The existing terrain uses a custom virtual texture system with known, unfixable LOD seam artifacts (see `SCRATCHPAD.md` for failed fix attempts). The plan is to **port to the new Wicked Engine DX12 terrain system** rather than continue fixing the old virtual texture pipeline.

### Reference Color Rendering Mode

A debug visualization mode that bypasses the entire virtual texture system and shows flat material colors directly from the terrain's paint data. This is the primary validation tool for the terrain port — it shows exactly what materials are painted where, independent of any LOD or virtual texture state. Currently hardcoded on (`ggterrain_render_reference = 1`).

**How it works**:
- Flag: `GGTERRAIN_SHADER_FLAG2_REFERENCE_COLOR` (0x0080) in `GGTerrainConstants.hlsli`
- C++ toggle: `ggterrain_render_reference` in `GGTerrain_part0.cpp:4198` (hardcoded on)
- Shader early-out in `GGTerrainVirtualPBR_PS.hlsl` — before any page table lookup
- Samples `texMaterialMap` (4096x4096, register t55) directly using world position
- For unpainted areas (materialMap == 0): derives material from height/slope layer rules using `IN.worldPos.y` and `IN.normal.y` from the mesh vertex (consistent at all distances)
- For painted areas: converts 1-based material index to 0-based and looks up a 32-color palette
- Applies basic ambient lighting (half-lambert with up vector) so terrain shape is visible
- Editor overlays (brush circle, map border) still render on top

**Why it has no LOD seams**: Height comes from the mesh vertex (`IN.worldPos.y`), not LOD height maps. The material map is a single global texture with fixed world mapping. Nothing LOD-dependent touches the output.

### Grass & Tree Reference Overlays

Layered on top of the reference color terrain view, showing where vegetation is placed. Both are currently hardcoded on alongside reference color mode.

**Grass overlay** (`GGTERRAIN_SHADER_FLAG2_SHOW_GRASS_MAP`, 0x0100):
- Source data: `pGrassMap` — a 4096x4096 `uint8_t` array in `GGGrass.cpp` (each byte = grass type, bit 0x80 = flattened flag)
- GPU texture: `texGrassMap` (R8_UNORM, register t56), uploaded via `GGGrass_UploadGrassMap()` which re-creates the texture from CPU data
- Upload triggers: init, `GGGrass_SetData()` (level load), `GGGrass_UpdateFlatArea()`, `GGGrass_Update_Painting()` (brush strokes)
- Shader: samples grass map at `worldPos.xz / terrain_mapEditSize * 0.5 + 0.5`, non-zero values tinted green via `referenceColors[type & 31] * float3(0.5, 1.0, 0.5)`, blended 70% over terrain color
- Visual: green-tinted patterns showing grass type distribution across the terrain

**Tree overlay** (`GGTERRAIN_SHADER_FLAG2_SHOW_TREE_MAP`, 0x0200):
- Source data: `pAllTrees[400000]` — `InstanceTree` structs with world positions and type IDs in `GGTrees_part0.cpp`
- CPU rasterization: `GGTrees_RasterizeTreeMap()` iterates all visible/valid trees, stamps 3-pixel-radius circles into `pTreeMap[4096*4096]` with value = tree type + 1
- GPU texture: `texTreeMap` (R8_UNORM, register t57), uploaded via `GGTrees_UploadTreeMap()`
- Upload triggers: after `GGTrees_RepopulateInstances()` (init), after `GGTrees_SetData()` (level load)
- Shader: samples tree map, non-zero values shown as solid `referenceColors[type & 31]` (full replacement, not blended)
- Visual: colored dots at each tree position, color = tree type

**Use during port**: Compare all three overlays (material + grass + tree) against the new Wicked Engine terrain to verify that the port preserves all painted vegetation data alongside terrain materials.

| Overlay | Texture | Slot | Flag | Source |
|---------|---------|------|------|--------|
| Material | `texMaterialMap` | t55 | 0x0080 | `pMaterialMap` (terrain paint) |
| Grass | `texGrassMap` | t56 | 0x0100 | `pGrassMap` (grass paint) |
| Tree | `texTreeMap` | t57 | 0x0200 | `pAllTrees` (rasterized) |

### Legacy virtual texture system (to be replaced)

The old system is documented in `SCRATCHPAD.md` along with the investigation into its unfixable LOD seam artifacts. Key files for reference during port:

| File | Content |
|------|---------|
| `Guru-WickedMAX/GGTerrain/GGTerrain_part0.cpp` | Page table management, CPU fallback generation, LOD level management, reference color flags |
| `Guru-WickedMAX/GGTerrain/GGTerrainPageSettings.h` | Atlas dimensions, page table constants |
| `Guru-WickedMAX/GGTerrain/GGTerrain.h` | LOD level count, segment size, segments per chunk, extern for `ggterrain_render_reference` |
| `Guru-WickedMAX/GGTerrain/Shaders/GGTerrainVirtualPBR_PS.hlsl` | Pixel shader — page table lookup + reference color mode + grass/tree overlays |
| `Guru-WickedMAX/GGTerrain/Shaders/GGTerrainConstants.hlsli` | Shader flags for reference color, grass map, tree map |
| `Guru-WickedMAX/GGTerrain/GGGrass.cpp` | Grass map CPU data (`pGrassMap`), GPU upload (`GGGrass_UploadGrassMap`), bind function |
| `Guru-WickedMAX/GGTerrain/GGTrees_part0.cpp` | Tree data (`pAllTrees`), tree map rasterization/upload (`GGTrees_RasterizeTreeMap`), bind function |
| `Guru-WickedMAX/master_part1.cpp:152-158` | GPU readback disabled (commented out) |

## Third-Party Dependencies
- **WickedEngineDX12** is located at `../WickedEngineDX12` (sibling folder at `D:\max\WickedEngineDX12`)
- This is the rendering engine the project depends on
- Reference this repo when resolving includes, engine API calls, or tracking down type definitions
- Do NOT modify files in WickedEngineDX12 unless explicitly asked
- **Building WickedEngine**: `build_wicked.bat` defaults to **Debug** if no argument is passed. GameGuru Release links against the Release `.lib`, so always pass `Release` explicitly: `cmd //C "D:\\max\\WickedEngineDX12\\build_wicked.bat Release"`. After rebuilding WickedEngine, do a **clean rebuild** of GameGuru (`build.bat Release rebuild`) — incremental builds may not detect the `.lib` change and skip relinking.

## File Editing Rules

1. **Always read the target lines immediately before editing.** Use `sed -n 'START,ENDp' file` to confirm exact content before any Update call.

2. **Keep match strings short and unique.** Match on a single unique line when possible, never multi-line blocks with closing braces (`}`), as these are rarely unique and prone to whitespace mismatches.

3. **If an Update fails once, do NOT retry Update with a different guess.** Instead:
   - Use `sed -n 'l'` or `cat -A` to inspect the exact bytes (tabs vs spaces, CRLF vs LF)
   - Then use `sed -i` to perform the edit by line number, OR
   - Re-read the exact lines and retry Update with a single-line match

4. **Never attempt more than 2 Update calls on the same file without re-reading it.** Failed edits can leave the file in an unexpected state.

5. **These project files use tabs for indentation and may have CRLF line endings.** Do not assume spaces or LF-only.