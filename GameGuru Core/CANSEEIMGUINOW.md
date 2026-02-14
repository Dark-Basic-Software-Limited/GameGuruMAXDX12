# DX12 Migration - Current State (2026-02-14)

## Status: Editor launches and idles stable. Crashes occur when navigating deeper into functionality.

The GameGuru MAX engine has been migrated from DX11 to DX12 via WickedEngine's DX12 backend. The app launches, initializes, shows the editor UI (ImGui), and idles without crashing. Shader caching is fully operational (139 ms cached init vs 9 seconds uncached). The next phase of work is fixing crashes and issues that occur when interacting with the editor beyond the initial idle state.

## Build Instructions

- **GameGuru**: `run_build.cmd` in project root (builds Release x64, copies WickedEngine lib)
- **WickedEngine**: `D:\max\WickedEngineDX12\build_release.cmd` (force rebuilds WickedEngine_Windows.lib)
- Output EXE: `D:\DEV\BUILD\GameGuru Wicked MAX Build Area\Max\GameGuruMAX.exe`
- Crash log: `D:\DEV\BUILD\GameGuru Wicked MAX Build Area\Max\Guru-Crash.log`
- App log: `D:\DEV\BUILD\GameGuru Wicked MAX Build Area\Max\log.txt`

## What Works
- WickedEngine DX12 initialization (AMD Radeon RX 9060 XT)
- **Shader caching**: 392 core shaders + custom shaders cached as .cso files with .wishadermeta dependency tracking. First launch compiles (~9s), subsequent launches load from cache (~139 ms total engine init)
- 8 custom shaders compile OK (trees, water, glass, grid, blood effects)
- ImGui rendering via custom DX12 bridge (`imgui_gg_dx12_bridge.cpp`)
- Editor UI appears and is interactive at idle
- Font atlas rebuilding when fonts change (ChangeGGFont)
- Crash handler logs to `Guru-Crash.log` with source file/line resolution

## Next Phase: Runtime Stability Beyond Idle

The editor launches and idles successfully, but crashes and issues occur when navigating further into the software — opening menus, loading levels, placing entities, using editor tools, etc. This is the current work frontier.

### Expected problem areas
- **DX11 texture/resource code paths**: Many editor operations (entity previews, terrain painting, lightmap baking) go through legacy DX11 `GG*` macro APIs that expect a live `ID3D11Device*`. These will hit null pointers or invalid calls under DX12.
- **ImGui texture references**: Editor panels that display 3D previews or thumbnails may pass DX11 `ID3D11ShaderResourceView*` handles to ImGui, which now expects DX12 descriptors.
- **Disabled subsystems**: Terrain, GPU particles, bullet tracers, and video playback are all guarded off. Editor workflows that depend on these will fail or behave unexpectedly.
- **State machine transitions**: The game state machine (`M-Game_part*.cpp`) moves through loading, editing, and test-game states that may exercise unguarded DX11 code paths.
- **Asset loading**: Model/texture import pipelines may use DX11 device for format conversion or mipmap generation.

### Approach
- Run the app, interact with different editor features, and collect crash logs
- Triage crashes by examining `Guru-Crash.log` (includes source file/line) and `log.txt`
- Fix crashes with null guards, DX12 equivalents, or graceful disabling as appropriate
- Prioritize crashes that block the most common editor workflows

## What Is Disabled (needs DX12 root signature porting)
1. **Terrain system** (`g_iDisableTerrainSystem = 1` in `M-GridEdit_part0.cpp:329`)
   - ~25 GGTerrain shaders are DX11-style without DX12 root signatures
   - `GGTerrain_Init`, `GGTrees_Init`, `GGGrass_Init` all skipped
   - `GGTerrain_Update` still called but exits early via `ggterrain_initialised` guard
   - `GGTerrain_ResetSculpting` guarded against null heightmap buffers
2. **GPU Particles** (gracefully disabled in `GPUParticles_part0.cpp`)
   - 7 GPUP shaders compile from `Particles/Shaders/` directory but lack DX12 root signatures
   - `LoadGPUPShader()` function created (based on `LoadCustomShader` pattern)
   - System checks shader results and sets `gpu_particles_initialised = 0` on failure
   - `gpup_update`, `gpup_draw`, `gpup_draw_init` all return early when disabled
3. **Bullet Tracers** (gracefully disabled in `TracerManager.cpp`)
   - `BulletTracerVS/PS.hlsl` not found in WickedEngine shaders dir
   - Falls back to DX11 .cso files which lack root signatures
   - `tracerSystemReady` flag prevents Draw() from binding invalid PSO
4. **Video playback** (DX11-only, guarded in `CAnimation_part0.cpp`)
   - `CoreLoadAnimation()` returns FALSE when `m_pD3D` (DX11 device) is null

## Key Fixes Applied

### Phase 5B: Shader Caching (2026-02-14)
- **wiShaderCompiler.cpp:797** (WickedEngine): Removed `return true;` from `IsShaderOutdated()` that forced all 392 shaders to recompile every launch. The timestamp-based cache validation (comparing .cso timestamps against .wishadermeta dependency lists) now runs properly, loading cached shaders in ~139 ms instead of recompiling in ~9 seconds
- **CustomShaders.cpp**: Added `RegisterShader`/`IsShaderOutdated` caching to `LoadCustomShader()` — custom shaders now load from .cso cache when HLSL sources haven't changed
- **GPUParticles_part0.cpp**: Same caching pattern added to `LoadGPUPShader()`

### Phase 5A: DX12 Migration and Stability
#### WickedEngine modifications (`D:\max\WickedEngineDX12\WickedEngine\`)
- **wiGraphicsDevice_DX12.cpp ~line 4139**: Null guard in `CreatePipelineState` - returns false when `rootSignature` or `rootsig_desc` is null (shaders without DX12 root signatures)
- **wiGraphicsDevice_DX12.cpp ~line 3944**: Same null guard in `CreateShader` for CS/LIB shaders

#### GameGuru modifications
- **imgui_gg_dx12_bridge.cpp:357**: Changed `dynamic_cast` to `static_cast` for `GraphicsDevice_DX12*` (WickedEngine compiled with `/GR-`, RTTI disabled)
- **master_part1.cpp:401**: Same `dynamic_cast` to `static_cast` fix in Compose()
- **imgui_gg_dx12_bridge.cpp (ImGui_DX12_NewFrame)**: Added font atlas rebuild detection - checks `io.Fonts->IsBuilt()` and recreates GPU font texture when fonts are cleared/re-added
- **CInputC.cpp**: Null guards for `m_lpDI` in `SetupMouseEx()` and `SetupKeyboardEx()` - returns instead of calling `InputDestructor()`
- **GPUParticles_part0.cpp**: `LoadGPUPShader()` function + shader validation + graceful disable
- **TracerManager.cpp**: `tracerSystemReady` flag, shader/PSO validation, Draw() guard
- **CAnimation_part0.cpp**: `m_pD3D == NULL` guard at top of `CoreLoadAnimation()`
- **M-GridEdit_part0.cpp**: `g_iDisableTerrainSystem = 1` (was 0)
- **GGTerrain_part0.cpp**: Null guard in `GGTerrain_ResetSculpting()`

## Known Non-Crashing Issues
- Two `CreateResource` errors at `wiGraphicsDevice_DX12.cpp:3769` (HRESULT 0x80070057 "parameter is incorrect") - likely 0-dimension render targets, logged but non-fatal
- `BulletTracerVS/PS.hlsl` source files missing from WickedEngine shaders directory (falls back to .cso)
- LNK4020 warnings about corrupted PDB type records in Jolt Physics objects (cosmetic)
- LNK4286 warnings about symbol redefinition in vorbis_static.lib (cosmetic)

## DX12 Root Signature Problem (core blocker for disabled systems)
DX11 shaders don't embed root signatures. DX12 requires them. When `wiRenderer::LoadShader()` loads a .cso compiled for DX11, `D3D12CreateVersionedRootSignatureDeserializer` fails. The shaders need to be recompiled with DX12-compatible HLSL that includes root signature definitions, or root signatures need to be created programmatically. This affects:
- All GPUP shaders (7 shaders in `Particles/Shaders/`)
- All GGTerrain shaders (~25 shaders in `GGTerrain/Shaders/`)
- BulletTracer shaders (2 shaders in `tracers/`)

## Automation Harness (Phase 6A)
- **File-based command/response** system for Claude Code testing (`AutomationHarness.cpp/.h`)
- Command file: `auto_command.txt`, Response file: `auto_result.txt`, Log: `auto_log.txt` (all in exe directory)
- Hooked at top of `GuruLoopLogic()` — zero overhead when no command file present (`GetFileAttributesA` fast-path)
- Commands: `GET_STATE`, `NAVIGATE hub|hub.<tab>|storyboard`, `CLICK <element>`, `WAIT <ms>`, `SCREENSHOT`, `QUIT`, `LIST_DEMOS`, `SELECT_DEMO <name>`
- `LIST_DEMOS` enumerates all demo games by display name (19 games found)
- `SELECT_DEMO` selects a demo by name (case-insensitive), forces Demo Games tab active via `g_sAutoSelectDemo` consumed in `Welcome_Screen()`
- `CLICK edit_game` works from both storyboard (via `iStoryboardExecuteKey`) and hub/Demo Games tab (via `bTriggerEditDemoGame` flag)
- Tab navigation via `g_iAutoForceWelcomeTab` injecting `ImGuiTabItemFlags_SetSelected` into Welcome_Screen tab bar
- Tab tracking via `g_iAutoCurrentTab` exposing the static `iCurrentOpenTab` to GET_STATE
- Race condition fix: empty command files are skipped (not deleted) to handle writer flush timing
- See `AUTOMATION_MAP.md` for full UI state flag documentation

## Automation Test Results (Phase 6B)
Tested via harness: SELECT_DEMO "Switch Escape" → CLICK edit_game from hub

- **Result**: App freezes during level load. No crash (no new entry in `Guru-Crash.log`), but `GuruLoopLogic()` stops ticking, making the harness unresponsive.
- **Load path taken**: `iLaunchAfterSync = 7` (direct file load of `mapbank\Switch Escape.fpm`) — this is the non-project path since demo games don't have `.cProject` set.
- **Log output during freeze**: Repeated `D3D12CreateVersionedRootSignatureDeserializer` failures (0x80070057) for terrain/particle shaders, then `CreatePipelineState failed: shader missing DX12 root signature`. The loading sequence hits disabled subsystems (terrain, GPU particles) which fail gracefully, but the load state machine itself appears to hang.
- **Implication**: The synchronous level loading path blocks the main loop. Fixing this requires either making the load async or fixing the specific DX11 code paths in the load sequence that cause the hang.

## Level Load Freeze - Investigation Notes (Phase 6B)

### How to reproduce
```
echo "GET_STATE" > auto_command.txt          # confirm hub/demo_games
echo "SELECT_DEMO Switch Escape" > auto_command.txt
echo "CLICK edit_game" > auto_command.txt    # app freezes here
```

### What happens
The Demo Games "Edit Game" button (or `bTriggerEditDemoGame` flag) triggers the direct load path because demo games don't have `.cProject` set. The flow is:
1. `cDirectOpen = "mapbank\Switch Escape.fpm"` + `iLaunchAfterSync = 7` + `bWelcomeScreen_Window = false` (M-GridEditB_part16.cpp:2662-2666)
2. State 7 handler (M-GridEdit_part1.cpp:296-331): validates filename, sets `sNextLevelToLoad`, transitions to `iLaunchAfterSync = 502` with 3-frame delay
3. State 502 handler (M-GridEdit_part1.cpp:157-241): calls `gridedit_load_map()` **synchronously** — this blocks the main loop

### State machine location
**M-GridEdit_part1.cpp** contains the `iLaunchAfterSync` state machine, processed inside `mapeditorexecutable_loop()`:
- Line 130-132: Delay gate (`iSkibFramesBeforeLaunch` counts down before state is processed)
- Line 157-241: **State 502** — the actual level load (synchronous, blocking)
- Line 296-331: **State 7** — "Direct Open" setup, validates `cDirectOpen`, transitions to 502

### Synchronous load call chain
```
State 502 (M-GridEdit_part1.cpp:157)
  → gridedit_load_map() (M-GridEdit_part7.cpp:768)
    → mapfile_loadproject_fpm() (M-MapFile_part0.cpp:920)
      → ExtractZipThread + WaitForAll() (M-MapFile_part0.cpp:1013) — BLOCKS
      → entity_loadbank()
      → entity_loadelementsdata()
      → waypoint_loaddata()
      → [all synchronous, all on main thread]
```

### Key variables
| Variable | Type | Defined In | Purpose |
|----------|------|-----------|---------|
| `iLaunchAfterSync` | int | M-GridEdit_part0.cpp:393 | Load state machine state |
| `iSkibFramesBeforeLaunch` | int | M-GridEdit_part0.cpp:394 | Frame delay before processing state |
| `cDirectOpen` | char[260] | M-GridEdit_part0.cpp:421 | Path to level file to load |
| `sNextLevelToLoad` | string | M-GridEdit_part1.cpp (extern) | Staged level path for state 502 |
| `bTriggerEditDemoGame` | bool | M-GridEditB_part15.cpp:4 | Triggers Edit Game from Demo Games tab |

### Where `iLaunchAfterSync = 7` is set (all direct load triggers)
- M-GridEditB_part16.cpp:2616, 2664 (Demo Games tab play/edit)
- M-GridEditB_part19.cpp:2038, 2124, 2582, 4292, 4379, 4468 (Storyboard actions)
- M-GridEdit_part1.cpp:1300 (Recent files menu)
- DBDLLCore_part0.cpp:1900, 1934 (Command line / drag-and-drop)

### Why it freezes (not crashes)
The load is fully synchronous on the main thread. During `gridedit_load_map()`, `GuruLoopLogic()` never returns, so `AutoHarness_CheckForCommand()` never runs. The app doesn't crash — it's blocked inside the load. The DX12 root signature errors in the log are from terrain/particle shader loading during the level load (expected failures for disabled subsystems). The freeze likely occurs in one of:
- Entity loading hitting DX11 texture/model code paths
- Terrain init attempting DX11 resource creation
- Asset import using null DX11 device for format conversion

### Next steps
- Attach a debugger or add logging inside `gridedit_load_map()` / `mapfile_loadproject_fpm()` to find exactly where it hangs
- Check if `EmptyMessages()` (M-MapFile_part0.cpp:826) is being reached or if execution stalls before it
- Look for DX11 device calls (`m_pD3D`, `GG*` macros, `ID3D11Device*`) in the entity/terrain loading functions that would block or deadlock under DX12

## Architecture Notes
- **Init sequence** (`GameGuruMain.cpp`): Case 0 (editor window) -> Case 1 (GPU particles) -> Case 2 (terrain + tracers) -> Case 3 (GuruMain/common_init)
- **Render loop**: `Master::RunCustom()` -> `Run()` -> `MasterRenderer::Update()` (calls `GuruLoopLogic`) -> `__super::Update(dt)` -> `MasterRenderer::Render()` -> `MasterRenderer::Compose()` (ImGui drawn here)
- **ImGui DX11->DX12 redirect**: `ImGui_ImplDX11_NewFrame()` checks `ImGui_DX12_IsInitialized()` and redirects to `ImGui_DX12_NewFrame()`
- **Custom shader pattern**: `LoadCustomShader()` in `CustomShaders.cpp` compiles HLSL from custom directories with WickedEngine include paths; uses `RegisterShader`/`IsShaderOutdated` to skip recompilation when cached .cso is up-to-date
- **WickedEngine lib flow**: Build WickedEngine_Windows.lib -> copy to GameGuru Lib64 -> link into GameGuruMAX.exe
