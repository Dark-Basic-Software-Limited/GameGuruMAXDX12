# DX12 Migration - Current State (2026-02-14)

## Status: App runs stable for 120+ seconds, zero crashes

The GameGuru MAX engine has been migrated from DX11 to DX12 via WickedEngine's DX12 backend. The app launches, initializes, shows the editor UI (ImGui), and runs without crashing.

## Build Instructions

- **GameGuru**: `run_build.cmd` in project root (builds Release x64, copies WickedEngine lib)
- **WickedEngine**: `D:\max\WickedEngineDX12\build_release.cmd` (force rebuilds WickedEngine_Windows.lib)
- Output EXE: `D:\DEV\BUILD\GameGuru Wicked MAX Build Area\Max\GameGuruMAX.exe`
- Crash log: `D:\DEV\BUILD\GameGuru Wicked MAX Build Area\Max\Guru-Crash.log`
- App log: `D:\DEV\BUILD\GameGuru Wicked MAX Build Area\Max\log.txt`

## What Works
- WickedEngine DX12 initialization (AMD Radeon RX 9060 XT)
- All WickedEngine core shaders compile at runtime
- 8 custom shaders compile OK (trees, water, glass, grid, blood effects)
- ImGui rendering via custom DX12 bridge (`imgui_gg_dx12_bridge.cpp`)
- Editor UI appears and is interactive
- Font atlas rebuilding when fonts change (ChangeGGFont)
- Crash handler logs to `Guru-Crash.log` with source file/line resolution

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

### WickedEngine modifications (`D:\max\WickedEngineDX12\WickedEngine\`)
- **wiGraphicsDevice_DX12.cpp ~line 4139**: Null guard in `CreatePipelineState` - returns false when `rootSignature` or `rootsig_desc` is null (shaders without DX12 root signatures)
- **wiGraphicsDevice_DX12.cpp ~line 3944**: Same null guard in `CreateShader` for CS/LIB shaders

### GameGuru modifications
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

## Architecture Notes
- **Init sequence** (`GameGuruMain.cpp`): Case 0 (editor window) -> Case 1 (GPU particles) -> Case 2 (terrain + tracers) -> Case 3 (GuruMain/common_init)
- **Render loop**: `Master::RunCustom()` -> `Run()` -> `MasterRenderer::Update()` (calls `GuruLoopLogic`) -> `__super::Update(dt)` -> `MasterRenderer::Render()` -> `MasterRenderer::Compose()` (ImGui drawn here)
- **ImGui DX11->DX12 redirect**: `ImGui_ImplDX11_NewFrame()` checks `ImGui_DX12_IsInitialized()` and redirects to `ImGui_DX12_NewFrame()`
- **Custom shader pattern**: `LoadCustomShader()` in `CustomShaders.cpp` compiles HLSL from custom directories with WickedEngine include paths
- **WickedEngine lib flow**: Build WickedEngine_Windows.lib -> copy to GameGuru Lib64 -> link into GameGuruMAX.exe
