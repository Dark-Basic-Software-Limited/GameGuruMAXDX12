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

## Architecture Notes
- **Init sequence** (`GameGuruMain.cpp`): Case 0 (editor window) -> Case 1 (GPU particles) -> Case 2 (terrain + tracers) -> Case 3 (GuruMain/common_init)
- **Render loop**: `Master::RunCustom()` -> `Run()` -> `MasterRenderer::Update()` (calls `GuruLoopLogic`) -> `__super::Update(dt)` -> `MasterRenderer::Render()` -> `MasterRenderer::Compose()` (ImGui drawn here)
- **ImGui DX11->DX12 redirect**: `ImGui_ImplDX11_NewFrame()` checks `ImGui_DX12_IsInitialized()` and redirects to `ImGui_DX12_NewFrame()`
- **Custom shader pattern**: `LoadCustomShader()` in `CustomShaders.cpp` compiles HLSL from custom directories with WickedEngine include paths; uses `RegisterShader`/`IsShaderOutdated` to skip recompilation when cached .cso is up-to-date
- **WickedEngine lib flow**: Build WickedEngine_Windows.lib -> copy to GameGuru Lib64 -> link into GameGuruMAX.exe
