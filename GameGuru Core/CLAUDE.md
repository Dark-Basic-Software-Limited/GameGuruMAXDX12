# GameGuruWickedMAX - Claude Code Instructions

## Session Start
- **Always read `WETEST.md` at the start of every session** before executing any commands.
- If asked to "run MAX" or "launch MAX", execute the **Execution Pattern** from WETEST.md steps 1-3 verbatim. Use the exact bash commands shown — do NOT substitute with `cmd`, `dir`, or any other commands. Do NOT verify paths, check if the EXE exists, or build first. Just run the steps. If the launch fails, STOP and report the error — do not retry with alternative commands.
- For all other tasks (testing, building, debugging, etc.), work autonomously as normal — use judgment, retry, and problem-solve without stopping to ask.
- Do NOT build or compile unless explicitly told to.

## Project Overview
This is a large C++ Windows x64 project (game engine) built with MSVC.
Solution file: `GameGuruWickedMAX.sln`

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
- **Key commands**: `GET_STATE`, `NAVIGATE`, `CLICK`, `CLICK_NODE`, `SELECT_DEMO`, `GET_PERF_DATA`, `SCREENSHOT`, `PRESS_ESCAPE`
- **CLICK targets**: `play_game`, `edit_game`, `test_level`, `add_level`, `load_level`, `exit_screen_editor`
- **CLICK_NODE**: works with level nodes (loads into editor), screen/splash nodes (opens screen editor)
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

## DX12 Terrain Virtual Texture System

The terrain uses a virtual texture system with a physical page atlas and a multi-level page table.

### Architecture

| Component | Description | Location |
|-----------|-------------|----------|
| `texPageTableArray` | 2D texture array, `numLODLevels - 1` slices (default 8), each 256x256. One slice per LOD level for fine-detail pages | `GGTerrain_part0.cpp:846` |
| `texPageTableFinal` | Single 256x256 texture with 6 mip levels (256x256 down to 8x8). Coarsest LOD level | `GGTerrain_part0.cpp:847` |
| Physical atlas | 70x70 = 4900 pages (4899 usable) | `GGTerrainPageSettings.h:43-44` |
| Page budget | 128 pages/frame (`GGTERRAIN_REPLACEMENT_PAGE_MAX`) | `GGTerrain_part0.cpp:893` |

### GPU readback is disabled in DX12

The GPU readback feedback loop (`customDraw_AfterPrepass` → `GGTerrain_VirtualTexReadBack`) is commented out at `master_part1.cpp:156-158` because the DX12 prepass only has 1 color render target. `readBackValid` is permanently 0.

### CPU fallback page generation (sole mechanism in DX12)

Located at `GGTerrain_part0.cpp:10037-10142`. Runs in two phases across multiple frames:

**Phase 1** (lines 10053-10088): Fills final page table mip levels 4 and 3 (coarse base coverage). Mip 5 (8x8) is filled during initial `ShouldRegeneratePages()`. Consumes ~1344 pages total.

**Phase 2** (lines 10090-10135): Generates camera-centered pages at each array LOD level. Key design decisions:
- **Iterates outermost LOD first** (LOD 7→0) so coarse coverage is established before fine detail, preventing multi-level mip jumps at boundaries
- **Inner LODs get larger radii**: formula `14 - LOD` (min 6). Inner LODs need more pages for fine detail; outer LODs need fewer because each page covers more world area
- **Centers on camera's actual page-table coordinate**, not the fixed grid center (128,128). Each LOD level's grid center snaps to chunk boundaries and can be up to ~40 pages offset from the camera

### Critical: page-table ↔ world coordinate mapping

Each LOD level's page table maps to world space via:
```
LODHalfSize = segments_per_chunk * segSize * 4
page_x = (worldX - centerX + LODHalfSize) / (LODHalfSize * 2) * 256
page_y = (1.0 - (worldZ - centerZ + LODHalfSize) / (LODHalfSize * 2)) * 256
```
The shader (`GGTerrainVirtualPBR_PS.hlsl:77-110`) uses `terrain_LOD[level].x/z/size` set at `GGTerrain_part0.cpp:9788-9790`. When a page is missing (entry = 0), the shader increments `detailLevel` and tries the next coarser LOD.

### Key source files

| File | Content |
|------|---------|
| `Guru-WickedMAX/GGTerrain/GGTerrain_part0.cpp` | Page table management, CPU fallback generation, LOD level management |
| `Guru-WickedMAX/GGTerrain/GGTerrainPageSettings.h` | Atlas dimensions, page table constants |
| `Guru-WickedMAX/GGTerrain/GGTerrain.h` | LOD level count, segment size, segments per chunk |
| `Guru-WickedMAX/GGTerrain/Shaders/GGTerrainVirtualPBR_PS.hlsl` | Pixel shader page table lookup |
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