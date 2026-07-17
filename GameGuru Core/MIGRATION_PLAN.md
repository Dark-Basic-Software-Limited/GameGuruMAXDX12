# DX12 Migration Plan - GameGuruWickedMAX

> HISTORICAL record of the Feb 2026 DX12 migration. Deferred items recorded here (5 custom ImGui pixel shaders, multi-viewport, OpenXR VR path) are now tracked in SCRATCHPAD.md → Tech Debt.

**Created:** 2026-02-14
**Baseline Tag:** `dx11-final`
**Reference:** See `DX12_AUDIT.md` for detailed API usage analysis

---

## Executive Summary

The migration from DX11 to DX12 is significantly de-risked by WickedEngine's abstraction layer. All 3D rendering, scene management, resource creation, and shader pipeline operations already go through WickedEngine's API, which supports DX12 natively. The primary migration work centers on:

1. **ImGui DX11 backend** (9,249 lines of direct DX11 code)
2. **GG* macro layer** (200+ macros mapping to DX11 types)
3. **Shader recompilation** (63+ HLSL files to SM 6.0+)
4. **OpenXR VR path** (DX11 interop for eye rendering)

---

## Migration Phases

### Phase 1: Shader Recompilation (Low Risk)

**Goal:** Recompile all custom HLSL shaders to Shader Model 6.0+ DXIL format.

**Tasks:**
1. Set up DXC (DirectX Shader Compiler) build pipeline alongside existing FXC
2. Recompile GGTerrain shaders (40+ files in `GGTerrain/Shaders/`)
3. Recompile custom object shaders (15+ files in `GGTerrain/CustomShaders/`)
4. Recompile GPU particle shaders (8 files in `Particles/Shaders/`)
5. Verify all .cso output files load via `wiRenderer::LoadShader()`
6. Test terrain, grass, trees, particles, water, glass rendering

**Complexity:** Low - mechanical recompilation, same HLSL source
**Risk:** Low - HLSL is forward-compatible; SM 5.0 constructs work in SM 6.0+
**Files affected:** 63+ .hlsl files (read-only source, new .cso outputs)
**Estimated scope:** Shader build pipeline change only

---

### Phase 2: GG* Macro Layer Assessment (Medium Risk)

**Goal:** Determine which GG* macros are actively used vs. dead code, and plan their elimination or redefinition.

**Tasks:**
1. Audit all 200+ GG* macro usages across 30+ source files
2. Categorize each usage:
   - **Dead code:** GG* macros referenced but functionality is stubbed/unused
   - **UI code:** Used only in ImGui/editor paths (handled in Phase 4)
   - **Active rendering:** Used in live rendering paths
3. For dead code: Remove or stub out
4. For active DarkSDK rendering code: Determine if it routes through WickedEngine (most does) or calls DX11 directly
5. Document which DarkSDK modules have live DX11 rendering vs. pure data management

**Complexity:** Medium - requires careful analysis of each usage site
**Risk:** Medium - some macros may be used in subtle ways
**Files affected:** `SDK/DirectX/directx-macros.h`, 30+ consuming source files
**Key concern:** Many DarkSDK modules (Objects, Image, Camera, Light) use GG* types in their headers but the actual DX11 device calls may be disabled/redirected to WickedEngine

---

### Phase 3: DarkSDK Module DX11 Elimination (Medium-High Risk)

**Goal:** Remove or redirect remaining DX11 usage in DarkSDK wrapper modules.

**Tasks:**
1. Audit each DarkSDK module for live DX11 device/context calls:
   - `Common-Images.cpp` - Cube map rendering (ID3D11Texture2D, RenderTargetView, ShaderResourceView)
   - `CObjectManagerWicked_part*.cpp` - Object management (may have DX11 texture refs)
   - `CImageC.cpp` - Image loading (may have DX11 texture creation)
   - `CLightC.cpp` - Light management
2. For each live DX11 call, determine:
   - Can it be replaced with a WickedEngine equivalent?
   - Is it part of an unused code path?
   - Does it need a new abstraction?
3. Replace DX11 calls with WickedEngine API equivalents where possible
4. Stub out or remove functionality that has no DX12 equivalent and isn't needed

**Complexity:** Medium-High - large codebase surface, need to trace each call path
**Risk:** Medium - some modules may have hidden DX11 dependencies
**Files affected:** 20+ DarkSDK .cpp files, `directx-macros.h`
**Depends on:** Phase 2 (macro audit results)

---

### Phase 4: ImGui Backend Migration (High Risk, High Effort)

**Goal:** Replace the DX11 ImGui rendering backend with a DX12-compatible solution.

**Options (choose one):**

**Option A: ImGui DX12 Backend (Recommended)**
- Use Dear ImGui's official `imgui_impl_dx12.cpp` backend
- Port the 5 custom pixel shader variants (blur, nowhite, noalpha, boost25) to DX12
- Update the DX11 state backup/restore to DX12 equivalents
- Handle descriptor heap management for ImGui textures

**Option B: WickedEngine UI Rendering**
- Route ImGui rendering through WickedEngine's `wiImage::Draw()` and `wiFont::Draw()`
- Eliminates all direct graphics API code from ImGui
- May have performance implications for complex editor UI

**Option C: Shared DX11 Device (Interim)**
- Keep DX11 device for ImGui via DX11-on-12 interop
- Least migration effort but adds complexity and potential performance overhead
- Not recommended as a long-term solution

**Tasks (Option A):**
1. Integrate `imgui_impl_dx12.cpp` from Dear ImGui repository
2. Create DX12 descriptor heap for ImGui font/texture SRVs
3. Port custom pixel shaders from D3DCompile (vs_5_0/ps_5_0) to offline DXC compilation
4. Port multi-viewport support (per-viewport swap chain + RTV)
5. Replace `D3D11_MAP_WRITE_DISCARD` buffer updates with upload heap pattern
6. Remove DX11 state save/restore (DX12 uses PSOs, no mutable state)
7. Update `ImGui_ImplDX11_Init()` entry point to DX12 equivalent
8. Update `Common_part0.cpp` initialization call

**Complexity:** High - 9,249 lines to replace, custom shader variants, multi-viewport
**Risk:** High - editor UI is critical for usability; regressions directly visible
**Files affected:** 6 `imgui_gg_dx11_part*.cpp` files, `imgui_gg_dx11.h`, `Common_part0.cpp`
**Depends on:** Phase 1 (shader pipeline), Phase 3 (DX11 device removal)

---

### Phase 5: OpenXR VR Path Migration (Medium Risk)

**Goal:** Update VR rendering from DX11 interop to native DX12.

**Tasks:**
1. Update `OpenXRInit()` to pass DX12 device instead of DX11 device
2. Replace `ID3D11RenderTargetView* leftView = OpenXRStartRender(...)` with DX12 equivalent
3. Update eye texture submission to use DX12 textures
4. Verify stereo rendering pipeline with DX12 command lists
5. Test with VR headset

**Complexity:** Medium - OpenXR has native DX12 support
**Risk:** Medium - VR is secondary feature; can be deferred
**Files affected:** `master_part0.cpp` (VR rendering section), GGVR module
**Depends on:** Phase 3 (DX11 device removal)

---

### Phase 6: Final DX11 Removal and Cleanup (Low Risk)

**Goal:** Remove all DX11 headers, libraries, and dead code.

**Tasks:**
1. Remove `#include <d3d11.h>`, `#include <D3DX11.h>` from all files
2. Remove or redefine `directx-macros.h` (eliminate DX11 type mappings)
3. Remove DX11 interop methods from WickedEngine (`GetDeviceForIMGUI`, `GetImmediateForIMGUI`)
4. Remove `d3d11.lib`, `d3dcompiler.lib` from linker inputs
5. Remove `SDK/DirectX/d3dx11effect.h` and related Effects11 headers
6. Clean up preprocessor defines (`#define DX11`)
7. Full rebuild and regression test

**Complexity:** Low - mechanical cleanup
**Risk:** Low - by this phase all functionality has been migrated
**Files affected:** vcxproj files, header files, `directx-macros.h`
**Depends on:** All previous phases

---

## Risk Assessment

### Critical Risks

| Risk | Impact | Likelihood | Mitigation |
|------|--------|------------|------------|
| ImGui editor becomes non-functional during migration | High - blocks all development | Medium | Keep DX11 path functional until DX12 path is verified; use feature flag |
| Custom shaders fail under SM 6.0+ | Medium - visual artifacts | Low | HLSL is forward-compatible; test each shader individually |
| Performance regression with DX12 ImGui | Medium - editor feels sluggish | Low | Profile before/after; DX12 ImGui is well-tested upstream |
| OpenXR DX12 binding issues | Low - VR is secondary | Medium | Can defer VR migration; DX11 interop can be interim solution |

### Low Risks (Already Mitigated)

| Area | Why Low Risk |
|------|-------------|
| 3D scene rendering | Fully abstracted by WickedEngine |
| Terrain/grass/trees | Use WickedEngine device API (abstracted) |
| GPU particles | Use WickedEngine device API (abstracted) |
| ECS/scene management | Pure WickedEngine API |
| Resource loading | Pure WickedEngine API |
| Physics | Bullet library, no graphics API |
| Audio | DirectSound/XAudio2, no DX11 dependency |
| Networking | Stubs only |
| Lua scripting | No graphics API usage |

---

## Complexity by Module

| Module | Lines of DX11 Code | Complexity | Migration Phase |
|--------|-------------------|------------|-----------------|
| ImGui DX11 backend | 9,249 | **High** | Phase 4 |
| GG* macro layer | ~750 (header) + 30+ consumers | **Medium-High** | Phase 2-3 |
| Custom HLSL shaders | 63+ files | **Low** | Phase 1 |
| VR/OpenXR path | ~300 lines | **Medium** | Phase 5 |
| GGTerrain system | 0 (uses WickedEngine API) | **None** | N/A |
| GPU Particles | 0 (uses WickedEngine API) | **None** | N/A |
| wickedcalls bridge | 0 (uses WickedEngine API) | **None** | N/A |
| DarkSDK modules | ~500 lines (estimated live DX11) | **Medium** | Phase 3 |
| Physics (Bullet) | 0 | **None** | N/A |
| Audio | 0 | **None** | N/A |
| Input | 0 (uses wiInput) | **None** | N/A |

---

## Recommended Migration Order

```
Phase 1: Shader Recompilation .............. [Can start immediately]
    |
Phase 2: GG* Macro Audit .................. [Can start immediately, parallel with Phase 1]
    |
Phase 3: DarkSDK DX11 Elimination ......... [After Phase 2]
    |
Phase 4: ImGui Backend Migration .......... [After Phases 1 & 3]
    |
Phase 5: OpenXR VR Path ................... [After Phase 3, can parallel with Phase 4]
    |
Phase 6: Final Cleanup .................... [After all phases complete]
```

Phases 1 and 2 can run in parallel. Phase 4 is the critical path with highest effort.

---

## Potential Blockers

1. **WickedEngine DX11 interop removal:** The engine currently exposes `GetDeviceForIMGUI()` and `GetImmediateForIMGUI()` to provide DX11 device/context to ImGui. Removing this requires the ImGui DX12 backend to be ready first.

2. **Custom ImGui pixel shaders:** The project uses 5 custom pixel shader variants for editor rendering (blur, transparency effects). These must be ported to the new backend.

3. **Multi-viewport ImGui:** The editor uses ImGui multi-viewport with per-viewport DX11 swap chains. The DX12 multi-viewport implementation needs careful descriptor heap management.

4. **GG* macro consumers in DarkSDK:** Some DarkSDK modules may have subtle DX11 dependencies hidden behind the macro layer. Full audit required before removal.

5. **GGFMT_R8G8B8 format:** This format maps to `0` (unsupported) in the current macro layer. Any code using 24-bit RGB textures must be updated to use 32-bit RGBA.

6. **D3DX11 dependency:** `directx-macros.h` includes `<D3DX11.h>` which is deprecated. Any code using D3DX11 functions (texture loading, math) must be replaced with DirectXTex/DirectXMath equivalents.

---

## Success Criteria

- [ ] All custom shaders compile with SM 6.0+ / DXC
- [ ] GG* macro layer fully audited and categorized
- [ ] No live DX11 device/context calls outside ImGui
- [ ] ImGui editor renders correctly via DX12 backend
- [ ] No `d3d11.h` includes remaining in codebase
- [ ] VR rendering functional with DX12 (or explicitly deferred)
- [ ] Full Debug + Release build with zero DX11 linker dependencies
- [ ] Editor and game runtime regression-tested

---

## Phase 4 Status: Engine Initialization Migrated to DX12

**Date:** 2026-02-14
**Branch:** `phase4-dx12-init`

### Key Finding

WickedEngine DX12 was **already the active rendering backend**. The `Master` class extends `MainComponent` which internally creates the DX12 device, swap chain, command queues, and descriptor heaps during `SetWindow()` and `Initialize()`. The DX11 device globals (`m_pD3D`, `m_pImmediateContext`) were already NULL — the `GetDeviceForIMGUI()` and `GetImmediateForIMGUI()` calls had been commented out previously and don't exist in WickedEngineDX12.

### What Was Changed

1. **`imgui_gg_dx11_part0.cpp`** — Added null guards to 4 functions:
   - `ImGui_ImplDX11_Init()`: Returns false if device or context is NULL
   - `ImGui_ImplDX11_NewFrame()`: Returns early if `g_pd3dDevice` is NULL
   - `ImGui_ImplDX11_RenderDrawData()`: Returns early if device/context is NULL
   - `ImGui_ImplDX11_Shutdown()`: Added TODO comment (already null-safe)

2. **`Common_part0.cpp`** — Added TODO comments around `ImGui_ImplDX11_Init()` call at line 693

3. **`master_part0.cpp`** — Added TODO comments to:
   - DX11 device globals (lines 91-108): `m_pD3D`, `m_pImmediateContext`, `m_pDX`, and all DX11 state objects
   - Commented-out device retrieval (lines 336-338)

### Surprises

- **WickedEngine DX12 was already running** — 3D rendering, splash screen, terrain, particles all use WickedEngine API which routes to DX12 internally. No initialization migration was needed for core rendering.
- **ImGui DX11 was being called with NULL pointers** — `ImGui_ImplDX11_Init(m_pD3D, m_pImmediateContext)` was called with both pointers NULL, which would crash without the null guards added.
- **DX11 globals are referenced in ~50 files** — but all via `extern` and never assigned non-NULL values. They are effectively dead code.
- **No build configuration changes needed** — the project was already linking WickedEngine_Windows.lib (DX12 version).

### Blockers for Phase 5 (ImGui DX12 Backend)

1. ~~Need to expose `ID3D12Device*`, `ID3D12CommandQueue*`, and SRV descriptor heap from WickedEngine~~ **RESOLVED** — Added `GetDX12Device()` and `GetDX12GraphicsCommandList()` to WickedEngine; `GetGraphicsCommandQueue()` already existed
2. ~~Fresh ImGui DX12 backend available at `D:\max\imgui\backends\imgui_impl_dx12.h/cpp`~~ **INCOMPATIBLE** — requires ImGui 1.92+, project uses 1.73. Wrote custom bridge instead.
3. 5 custom pixel shader variants in ImGui backend need DX12 port — **DEFERRED** (TODO in bridge code)
4. ~~`ImGui_ImplDX11_NewFrame()` called from ~9 different source files~~ **RESOLVED** — redirected via shim in `ImGui_ImplDX11_NewFrame()` to `ImGui_DX12_NewFrame()`
5. Multi-viewport support needs DX12 per-viewport swap chains — **DISABLED** for now (future phase)

---

## Phase 5 Status: ImGui Migrated to DX12 Backend

**Date:** 2026-02-14
**Branch:** `phase5-imgui-dx12`

### Crash Fix (Pre-requisite)

Fixed three crash vectors caused by NULL DX11 device pointers in the DX12 init path:
- `ImGuiHook_RenderCall_Direct()` — NULL `d3dptr` used as `ID3D11Device*`
- `ImGuiHook_RenderCall()` — NULL `ctxptr` used as `ID3D11DeviceContext*`
- `ImGuiConfigFlags_ViewportsEnable` — multi-viewport enabled without registered renderer callbacks

### DX12 Device Acquisition

- `GetGraphicsCommandQueue()` was already public in `GraphicsDevice_DX12`
- Added `GetDX12Device()` and `GetDX12GraphicsCommandList(CommandList cmd)` (2 methods, ~10 lines total)
- `dynamic_cast<GraphicsDevice_DX12*>(GetDevice())` used to obtain the DX12 device from WickedEngine's abstract interface
- Rebuilt WickedEngine_Windows.lib with `/MTd` (static CRT) to match GameGuru

### ImGui Version Incompatibility

- Project uses ImGui **1.73 WIP** (version 17203)
- Stock `imgui_impl_dx12.cpp` from `D:\max\imgui\backends\` requires ImGui **1.92+** (uses `ImTextureData`, `ImGuiBackendFlags_RendererHasTextures`, `ImDrawData::Textures`)
- **Solution:** Created `imgui_gg_dx12_bridge.cpp` — a self-contained DX12 ImGui renderer compatible with ImGui 1.73

### What Was Changed

1. **`imgui_gg_dx12_bridge.h/cpp`** (NEW, ~600 lines) — Self-contained DX12 ImGui renderer:
   - Dedicated 64-slot shader-visible SRV descriptor heap with free-list allocator
   - Runtime-compiled HLSL vertex/pixel shaders (vs_5_0/ps_5_0) via `D3DCompile`
   - Root signature: 32-bit constants (MVP) + SRV descriptor table + static linear sampler
   - Font texture upload via temporary command list with synchronous fence wait
   - Double-buffered vertex/index upload buffers
   - Full draw command rendering loop with scissor rects, texture binding, indexed draws

2. **`master_part1.cpp`** — Added ImGui rendering in `MasterRenderer::Compose()`:
   - Gets native DX12 command list via `GetDX12GraphicsCommandList(cmd)`
   - Calls `ImGui_DX12_RenderBridge(nativeCmdList)` after `__super::Compose(cmd)`
   - Protected by `bImGuiInitDone` and `ImGui_DX12_IsInitialized()` guards

3. **`Common_part0.cpp`** — Replaced DX11 init with DX12 bridge:
   - Calls `ImGui_DX12_InitBridge()` instead of `ImGui_ImplDX11_Init()`
   - Multi-viewport disabled in both success and failure paths

4. **`imgui_gg_dx11_part0.cpp`** — NewFrame redirect:
   - `ImGui_ImplDX11_NewFrame()` now forwards to `ImGui_DX12_NewFrame()` if bridge is initialized
   - All ~9 call sites transparently redirected without modification

5. **`wickedcalls_part3.cpp`** — `WickedCall_DrawImguiNow()` made into no-op:
   - Old DX11 rendering path commented out with `// TODO: removed DX11 ImGui path`
   - Rendering now happens in `MasterRenderer::Compose()` via the WickedEngine pipeline

6. **`Template_Windows.vcxproj`** — Added `imgui_gg_dx12_bridge.cpp` to compilation

### WickedEngine Changes (Minimal)

- `wiGraphicsDevice_DX12.h` — Added 2 public accessor method declarations
- `wiGraphicsDevice_DX12.cpp` — Added 2 method implementations (~10 lines)
- `WickedEngine_Windows.vcxproj` — Added `<RuntimeLibrary>MultiThreadedDebug</RuntimeLibrary>` to Debug config

### Custom Pixel Shaders (DEFERRED)

The DX11 ImGui backend uses 5 custom pixel shader variants for editor effects:
- `blur` — Background blur behind panels
- `nowhite` — Transparency without white
- `noalpha` — Opaque rendering mode
- `boost25` — Brightness boost
- Standard — Default textured rendering

Currently only the standard shader is implemented in the DX12 bridge. Custom shader callbacks in `ImGui_DX12_RenderBridge()` are skipped with a TODO comment. These can be ported in a future iteration.

### Multi-Viewport (DISABLED)

Multi-viewport (`ImGuiConfigFlags_ViewportsEnable`) requires per-viewport DX12 swap chains and render targets. This is disabled for now — noted as a future phase task.

### Build Status

- **Debug x64:** COMPILES AND LINKS SUCCESSFULLY
- Pre-existing warnings only (vorbis CRT mismatch, float truncation)
- No new compilation errors or warnings from Phase 5 changes
