# DX12 Migration Audit - GameGuruWickedMAX

**Audit Date:** 2026-02-14
**Baseline Tag:** `dx11-final`
**Scope:** Complete codebase analysis of all graphics API usage for DX11-to-DX12 migration

---

## 1. Engine Initialization

### 1.1 Application Bootstrap

**File: `Guru-WickedMAX/main.cpp`**
- Line 207: `wiRenderer::SetShaderPath("shaders/")` - Sets shader search directory
- Line 294: `master.SetWindow(hWnd)` - Passes Win32 HWND to WickedEngine's `wi::Application`
- Main loop calls `master.Run()` which drives WickedEngine's frame cycle

**File: `Guru-WickedMAX/master_part0.cpp`**
- Lines 303-309: `Master::Initialize()` calls `__super::Initialize()` (WickedEngine base init)
- Lines 311-505: `Master::InitializeSecondaries()`:
  - Line 323: `masterrenderer.Load()` - Loads RenderPath3D resources
  - Line 325: `ActivatePath(&masterrenderer)` - Activates the 3D render path
  - Line 330: `wiEvent::SetVSync(bVsyncEnabled)` - V-Sync configuration
  - Line 331: `masterrenderer.setAO(RenderPath3D::AO_MSAO)` - Ambient occlusion mode
  - Line 334: `wiRenderer::SetOcclusionCullingEnabled(false)` - Occlusion culling disabled
  - Lines 337-338: Commented-out DX11 device retrieval for ImGui:
    ```cpp
    //m_pD3D = (ID3D11Device*)wiGraphics::GetDevice()->GetDeviceForIMGUI();
    //m_pImmediateContext = (ID3D11DeviceContext*)wiGraphics::GetDevice()->GetImmediateForIMGUI();
    ```

**File: `Guru-WickedMAX/master_part1.cpp`**
- Lines 10-54: `MasterRenderer::Load()`:
  - Line 10: `__super::Load()` - RenderPath3D resource loading
  - Line 13: `wiEvent::SetVSync(false)`
  - Lines 21-46: Render feature toggles:
    - `setSSREnabled(false)`, `setReflectionsEnabled(true)`, `setFXAAEnabled(false)`
    - `setBloomEnabled(true)`, `setShadowsEnabled(true)`, `setLightShaftsEnabled(true)`
    - `wiRenderer::SetTessellationEnabled(false)`, `setBloomThreshold(2.0f)`
    - `setEyeAdaptionEnabled(false)`
  - Lines 62-77: Weather entity creation:
    - `g_weatherEntityID = CreateEntity()`
    - `weather.ambient`, `weather.horizon`, `weather.zenith`, `weather.fogStart`, etc.
    - `weather.SetRealisticSky(true)`, `weather.SetVolumetricClouds(true)`
  - Lines 79-102: Sun light creation:
    - `Entity_CreateLight("sunLight", ...)` with shadow casting, lens flares
    - `lightSun->SetType(LightComponent::DIRECTIONAL)`

**File: `Guru-WickedMAX/GameGuruMain.cpp`**
- Lines 164-169: GPU systems init:
  - `wiGraphics::GetDevice()->BeginCommandList()`
  - `GGTerrain::GGTerrain_Init(cmd)`, `GGTrees::GGTrees_Init()`, `GGGrass::GGGrass_Init()`
  - `Tracers::Initialize()`

### 1.2 DX11 Device Initialization (Legacy Path)

**File: `GameGuru/Source/Common_part0.cpp`**
- Line 693: `ImGui_ImplDX11_Init(m_pD3D, m_pImmediateContext)` - ImGui DX11 backend init
- The DX11 device/context are obtained from WickedEngine's interop layer (`GetDeviceForIMGUI()` / `GetImmediateForIMGUI()`)

---

## 2. Rendering Pipeline

### 2.1 Main Render Loop (WickedEngine Path)

**File: `Guru-WickedMAX/master_part0.cpp`**

**Splash Screen Rendering (Lines 1218-1314):**
- `CommandList cmd = wiGraphics::GetDevice()->BeginCommandList()`
- `wiGraphics::GetDevice()->RenderPassBegin(&swapChain, cmd)`
- `wiImage::SetCanvas(canvas)`, `wiFont::SetCanvas(canvas)`
- `wiGraphics::GetDevice()->BindViewports(1, &viewport, cmd)`
- `wiImage::Draw(&g_pSplashTexture, fx, cmd)` - 2D image draw
- `wiGraphics::GetDevice()->RenderPassEnd(cmd)`
- `wiGraphics::GetDevice()->SubmitCommandLists()`

**Main Frame Composition (Lines 1340-1403):**
- `Render()` - Offscreen 3D rendering (via RenderPath3D)
- `CommandList cmd = wiGraphics::GetDevice()->BeginCommandList()`
- `wiGraphics::GetDevice()->RenderPassBegin(&swapChain, cmd)` - Begin backbuffer pass
- ImGui rendering (DX11 calls within this pass)
- `wiProfiler::EndFrame(cmd)`
- `wiGraphics::GetDevice()->SubmitCommandLists()`

**File: `Guru-WickedMAX/master_part1.cpp`**

**MasterRenderer::Update() (Lines 109-179):**
- `wiProfiler::BeginRangeCPU("Update - Logic")` / `EndRange(range)`
- `GuruLoopLogic()` - Game logic
- `gpup_update(deltaTime, cmd)` - GPU particles
- `GGTerrain_Update(...)`, `GGTrees_Update(...)`, `GGGrass_Update(...)` - Terrain systems
- `wiInput::Update(window, canvas)` - Input processing
- `wiEvent::FireEvent(EVENT_THREAD_SAFE_POINT, 0)` - Thread sync
- `__super::Update(dt)` - WickedEngine RenderPath3D update

**MasterRenderer::Compose() / Render() (Lines 349-360):**
- `__super::Compose(cmd)` - WickedEngine final composition
- `__super::Render()` - WickedEngine offscreen rendering

### 2.2 Outline Rendering (Custom Render Passes)

**File: `Guru-WickedMAX/master_part1.cpp`**

**ResizeBuffers() (Lines 204-279):**
- Creates `rt_Outline`, `rt_Outline_Red`, `rt_Outline_Blue` textures:
  - `Format::R8_UNORM`, `BindFlag::RENDER_TARGET | BindFlag::SHADER_RESOURCE`
  - `device->CreateTexture(&desc, nullptr, &rt_Outline)`
- Creates `RenderPass` objects with color + depth attachments:
  - `device->CreateRenderPass(&desc, &renderpass_Outline)`

**RenderOutlineHighlighters() (Lines 364-490):**
- `CommandList cmd = device->BeginCommandList()`
- `device->EventBegin("GGMax - Selection Outline Mask", cmd)` - GPU event marker
- `device->BindViewports(1, &vp, cmd)`
- `device->RenderPassBegin(&renderpass_Outline, cmd)`
- `wiImage::Draw(wiTextureHelper::getWhite(), fx, cmd)` - Mask drawing
- `device->RenderPassEnd(cmd)`
- `wiRenderer::BindCommonResources(cmd)`
- `wiRenderer::Postprocess_Outline(rt_Outline, cmd, ...)` - Post-process outline

### 2.3 VR Rendering Path

**File: `Guru-WickedMAX/master_part0.cpp` (Lines 1549-1831)**
- Dual-eye rendering with `OpenXRGetViewMat()` / `OpenXRGetProjMat()`
- `wiScene::GetCamera().SetCustomProjectionEnabled(true)`
- `wiScene::GetCamera().TransformCamera(camera_transform)`
- `wiScene::GetCamera().UpdateCamera()`
- `GGTrees_UpdateFrustumCulling(&wiScene::GetCamera())`
- **DX11 INTEROP:** `ID3D11RenderTargetView* leftView = OpenXRStartRender(OPENXR_RENDER_LEFT)` - Gets DX11 RT from OpenXR
- Per-eye `Render()` calls with separate command lists
- `wiProfiler::BeginFrame()` / `wiProfiler::EndFrame(cmd)`

---

## 3. Resource Management

### 3.1 Texture Loading & Creation

**wiResourceManager::Load() usage:**

| File | Line(s) | Usage |
|------|---------|-------|
| `wickedcalls_part0.cpp` | 301-462 | `wiResourceManager::Load(pFilenameToLoad, flag, data.data(), data.size())` |
| `master_part1.cpp` | 100 | Lens flare textures: `wiResourceManager::Load(fileName)` |
| `master_part0.cpp` | 222-247 | Material textures: BASECOLORMAP, SURFACEMAP, NORMALMAP, DISPLACEMENTMAP |
| `master_part0.cpp` | 1176-1189 | Splash texture with DDS fallback |
| `ModelImporter_OBJ.cpp` | 69-82 | OBJ material textures |

**Direct device->CreateTexture() usage:**

| File | Line(s) | Usage |
|------|---------|-------|
| `GPUParticles_part0.cpp` | 558-590 | `GPUP_LoadTexture()` - R8G8B8A8_UNORM, SHADER_RESOURCE + UNORDERED_ACCESS |
| `GPUParticles_part0.cpp` | 592-621 | `GPUP_CreateRenderTexture()` - RENDER_TARGET + SHADER_RESOURCE + UNORDERED_ACCESS |
| `master_part1.cpp` | 204-230 | Outline render targets - R8_UNORM |
| `GGTerrain_part0.cpp` | 4174-4226 | Virtual texture system - DDS/PNG loading |

**Texture types used in GGTerrain (GGTerrain_part0.cpp lines 796-829):**
- `texLODHeightMapArray`, `texLODNormalMapArray` - LOD heightmap arrays
- `texColorArray`, `texNormalsArray` - Surface textures
- `texSurfaceArray`, `texRoughnessMetalnessArray`, `texAOArray` - PBR texture arrays
- `texPageTableArray`, `texPageTableFinal` - Virtual texture page tables
- `texMaterialMap` - Material ID map
- `texPagesColorAndMetal`, `texPagesNormalsRoughnessAO` - Physical page textures
- `texReadBackCompute`, `texReadBackStaging` - GPU readback textures

### 3.2 Buffer Creation & Updates

**GPUBuffer creation (GPUParticles_part0.cpp lines 1942-2027):**
- Constant buffers: `mainVSConstants`, `mainPSConstants`, `speedConstants`, `posConstants`, `noiseConstants`
  - `Usage::DEFAULT`, `BindFlag::CONSTANT_BUFFER`
  - `device->CreateBuffer(&bd, nullptr, &buffer)`
- Vertex/index buffers: `mainVertexBufferObj0/1`, `mainIndexBufferObj0/1`, `quadVertexBuffer`
  - `SubresourceData data` with initial data
  - `device->CreateBuffer(&bd, &data, &buffer)`
- Samplers: `samplerPoint`, `samplerLinear`, `samplerLinearWrap`
  - `device->CreateSampler(&samplerDesc, &sampler)`

**GGTerrain buffers (GGTerrain_part0.cpp):**
- `instanceBuffer`, `terrainConstantBuffer`, `pageGenVertexBuffer` (line 771-855)
- `sphereVertexBuffer`, `sphereIndexBuffer` (lines 969-970)
- `quadVertexBuffer`, `quadVSConstantBuffer`, `quadPSConstantBuffer` (lines 997-1013)

**GGGrass buffers (GGGrass.cpp):**
- `grassConstantBuffer` (line 778)
- Instance buffers (lines 414-420)

**Buffer update pattern (used throughout):**
```cpp
device->UpdateBuffer(&buffer, &data, cmd, sizeof(data));
```

| File | Approximate Count |
|------|-------------------|
| `GPUParticles_part0.cpp` | 7 UpdateBuffer calls |
| `GGTerrain_part0.cpp` | 6 UpdateBuffer calls |
| `GGGrass.cpp` | 1 UpdateBuffer call |
| `GGTrees_part0.cpp` | 2 UpdateBuffer calls |

### 3.3 Resource Lifetime

- WickedEngine handles automatic cleanup via reference counting
- `GPUP_DeleteTexture()` (GPUParticles_part0.cpp:623-626) - Intentionally empty body
- Entity removal triggers resource cleanup: `wiScene::GetScene().Entity_Remove(entity)`

---

## 4. Scene / ECS Component Usage

### 4.1 Entity Creation Patterns

| Factory Method | Files Used In |
|---------------|---------------|
| `scene.Entity_CreateObject(name)` | wickedcalls_part0.cpp, master_part0.cpp, ModelImporter_OBJ.cpp |
| `scene.Entity_CreateMesh(name)` | wickedcalls_part0.cpp, master_part0.cpp, ModelImporter_OBJ.cpp |
| `scene.Entity_CreateMaterial(name)` | wickedcalls_part0.cpp, master_part0.cpp, ModelImporter_OBJ.cpp |
| `scene.Entity_CreateLight(name, pos, color, intensity, range)` | master_part1.cpp, wickedcalls_part3.cpp |
| `CreateEntity()` | wickedcalls_part0.cpp, wickedcalls_part4.cpp, master_part1.cpp |

### 4.2 Component Access Patterns

**TransformComponent:**
- `scene.transforms.GetComponent(entity)` / `scene.transforms.Create(entity)`
- Methods: `MatrixTransform()`, `GetPosition()`, `ClearTransform()`
- Files: wickedcalls_part4.cpp, wickedcalls_part0.cpp

**ObjectComponent:**
- `scene.objects.GetComponent(entity)`
- Properties: `meshID`, render flags
- Files: wickedcalls_part0.cpp, ModelImporter_OBJ.cpp

**MeshComponent:**
- `scene.meshes.GetComponent(meshEntity)`
- Properties: `subsets`, vertex data (positions, normals, UVs, tangents, bone data)
- Methods: `CreateRenderData()`
- Files: wickedcalls_part0.cpp, wickedcalls_part1.cpp, master_part0.cpp

**MaterialComponent:**
- `scene.materials.GetComponent(materialEntity)`
- Properties: `baseColor`, `roughness`, `metalness`, `emissiveColor`, `reflectance`, `shaderType`
- Texture slots: `BASECOLORMAP`, `SURFACEMAP`, `NORMALMAP`, `DISPLACEMENTMAP`
- Methods: `SetRoughness()`, `SetMetalness()`, `SetDirty(true)`, `SetReflectance()`
- Files: wickedcalls_part1.cpp, wickedcalls_part2.cpp, ModelImporter_OBJ.cpp

**AnimationComponent:**
- `scene.animations.GetComponent(entity)` / `scene.animations.Create(entity)`
- Samplers: `AnimationSampler::Mode::LINEAR`
- Channels: `Path::TRANSLATION`, `Path::ROTATION`, `Path::SCALE`
- Methods: `IsPlaying()`, `Play()`, `Stop()`, `SetLooped()`, `SetPlaybackSpeed()`
- Files: wickedcalls_part0.cpp (17+ access locations)

**ArmatureComponent:**
- `scene.armatures.Create(entity)` / `scene.armatures.GetComponent(entity)`
- Properties: `boneCollection`
- Files: wickedcalls_part0.cpp

**LightComponent:**
- `scene.lights.GetComponent(entity)`
- Properties: `direction`, `color`, `intensity`, `range`, `lensFlareRimTextures`
- Methods: `SetCastShadow(true)`, `SetType(DIRECTIONAL)`, `SetVolumetricsEnabled(true)`
- Files: master_part1.cpp, wickedcalls_part3.cpp

**WeatherComponent:**
- `scene.weathers.Create(entity)` / `scene.weathers.GetComponent(entity)`
- Properties: `ambient`, `horizon`, `zenith`, `fogStart`, `fogDensity`, `wind`, `volumetricCloudParameters`
- Methods: `SetRealisticSky(true)`, `SetVolumetricClouds(true)`
- Files: master_part1.cpp, wickedcalls_part3.cpp

**EmitterComponent / wiEmittedParticle:**
- `scene.emitters.GetComponent(entity)` / `scene.emitters.GetCount()` / `scene.emitters.GetEntity(i)`
- Properties: `count`, `life`, `size`, `gravity`
- Methods: `Burst()`, `SetPaused()`, `Restart()`
- Files: wickedcalls_part4.cpp

**LayerComponent:**
- `scene.layers.GetComponent(entity)` / `scene.layers.Create(entity)`
- Properties: `layerMask`
- Files: wickedcalls_part0.cpp, wickedcalls_part2.cpp, wickedcalls_part4.cpp

**HierarchyComponent:**
- `scene.hierarchy.GetComponent(entity)` / `scene.hierarchy.Contains(entity)`
- Methods: `scene.Component_Attach()`, `scene.Component_DetachChildren()`
- Files: wickedcalls_part3.cpp, wickedcalls_part4.cpp

**CameraComponent:**
- `wiScene::GetCamera()` - Global camera access
- Methods: `SetCustomProjectionEnabled()`, `TransformCamera()`, `UpdateCamera()`
- Files: master_part0.cpp, master_part1.cpp, GPUParticles_part0.cpp

### 4.3 Scene Operations

- `wiScene::GetScene()` - Global scene access (all wickedcalls files)
- `scene.Entity_Remove(entity)` - Entity cleanup with cascading resource release
- `scene.Update(0)` - Force scene update after modifications
- `scene.Merge(otherScene)` - Scene merging for model import
- `scene.Component_Attach(entity, parent)` - Hierarchy attachment

---

## 5. Custom Shader System

### 5.1 Shader Loading

**wiRenderer::LoadShader() calls:**

| File | Shaders Loaded |
|------|---------------|
| `CustomShaders.cpp` | `objectVS_common_tree.cso`, `objectPS_custom_water.cso`, `objectPS_transparent_glass.cso`, `objectPS_grid.cso`, `damageBloodVS.cso`, `damageBloodPS.cso` |
| `GPUParticles_part0.cpp` | `GPUP_QuadVS.cso`, `QuadDefaultPS.cso`, `GPUP_NoisePS.cso`, `GPUP_SpeedPS.cso`, `GPUP_PosPS.cso`, `GPUP_MainPS.cso` |
| `GGTerrain_part0.cpp` | `GGTerrainVS.cso`, `GGTerrainVirtualPBR_PS.cso`, `GGTerrainPrepassVS.cso`, `GGTerrainReadBackMSCS.cso`, `GGTerrainReadBackCS.cso` + more |
| `GGGrass.cpp` | `GGGrassVS.cso`, `GGGrassPS.cso` + shadow/prepass variants |
| `GGTrees_part0.cpp` | `GGTreesVS.cso`, `GGTreesPS.cso` + shadow/prepass variants |

### 5.2 Pipeline State Objects

**PipelineState creation pattern:**
```cpp
PipelineStateDesc desc;
desc.vs = &shaderVS;
desc.ps = &shaderPS;
desc.rs = &rasterizerState;
desc.bs = &blendState;
desc.dss = &depthStencilState;
device->CreatePipelineState(&desc, &pso);
```

| File | PSO Count |
|------|-----------|
| `CustomShaders.cpp` | 6+ PSOs (tree, water, glass, grid, blood) |
| `GPUParticles_part0.cpp` | 7+ PSOs (psoAlpha, psoAdd, psoOpaque, psoQuad*) |
| `GGTerrain_part0.cpp` | 10+ PSOs (main, prepass, shadow, virtual, env probe, ramp) |
| `GGGrass.cpp` | 4+ PSOs (main, prepass, shadow) |
| `GGTrees_part0.cpp` | 4+ PSOs (main, prepass, shadow) |

### 5.3 Compute Shaders

**GGTerrain_part0.cpp:**
- `device->BindComputeShader(&shaderReadBackMSCS, cmd)` (line 10238)
- `device->BindComputeShader(&shaderReadBackCS, cmd)` (line 10239)
- Used for virtual texture readback operations

### 5.4 HLSL Source Files

| Directory | File Count | Purpose |
|-----------|-----------|---------|
| `GGTerrain/Shaders/` | 40+ .hlsl | Terrain, grass, trees rendering + shadow/prepass variants |
| `GGTerrain/CustomShaders/` | 15+ .hlsl | Custom object shaders (water, glass, tree animate, blood) |
| `Particles/Shaders/` | 8 .hlsl | GPU particle shaders |

---

## 6. ImGui DX11 Backend (Direct DX11 Code)

### 6.1 Scope

**Files:** `GameGuru/Imgui/imgui_gg_dx11.h` + `imgui_gg_dx11_part0-5.cpp`
**Total Lines:** 9,249 across 6 part files

### 6.2 DX11 Global State

```cpp
// imgui_gg_dx11_part0.cpp lines 87-114
static ID3D11Device*            g_pd3dDevice;
static ID3D11DeviceContext*     g_pd3dDeviceContext;
static IDXGIFactory*            g_pFactory;
static ID3D11Buffer*            g_pVB, *g_pIB;
static ID3D11VertexShader*      g_pVertexShader;
static ID3D11InputLayout*       g_pInputLayout;
static ID3D11Buffer*            g_pVertexConstantBuffer;
static ID3D11PixelShader*       g_pPixelShader;           // + 4 variants (blur, nowhite, noalpha, boost25)
static ID3D11SamplerState*      g_pFontSampler;
static ID3D11ShaderResourceView* g_pFontTextureView;
static ID3D11RasterizerState*   g_pRasterizerState;
static ID3D11BlendState*        g_pBlendState;
static ID3D11DepthStencilState* g_pDepthStencilState;
```

### 6.3 DX11 API Functions Called

| Category | Functions |
|----------|-----------|
| **Device Creation** | `CreateBuffer`, `CreateTexture2D`, `CreateShaderResourceView`, `CreateVertexShader`, `CreatePixelShader`, `CreateInputLayout`, `CreateSamplerState`, `CreateRasterizerState`, `CreateBlendState`, `CreateDepthStencilState`, `CreateRenderTargetView` |
| **Buffer Operations** | `Map` (D3D11_MAP_WRITE_DISCARD), `Unmap` |
| **Shader Compilation** | `D3DCompile` (vs_5_0, ps_5_0) |
| **Input Assembly** | `IASetInputLayout`, `IASetVertexBuffers`, `IASetIndexBuffers`, `IASetPrimitiveTopology` |
| **Shader Binding** | `VSSetShader`, `PSSetShader`, `VSSetConstantBuffers`, `PSSetSamplers`, `PSSetShaderResources` |
| **Rasterizer** | `RSSetViewports`, `RSSetScissorRects`, `RSSetState` |
| **Output Merger** | `OMSetBlendState`, `OMSetDepthStencilState`, `OMSetRenderTargets` |
| **Draw** | `DrawIndexed` |
| **SwapChain** | `GetBuffer`, `ResizeBuffers` |
| **State Backup** | Full DX11 state save/restore struct (`BACKUP_DX11_STATE`) |

### 6.4 Multi-Viewport Support (DX11)

**imgui_gg_dx11_part5.cpp:**
- Creates per-viewport `IDXGISwapChain` + `ID3D11RenderTargetView`
- `g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &data->RTView)`
- `g_pd3dDeviceContext->OMSetRenderTargets(1, &data->RTView, NULL)`

---

## 7. GG* DX11 Macro Layer

**File:** `SDK/DirectX/directx-macros.h` (~750 lines)

### 7.1 Key Type Mappings

| GG Macro | DX11 Type | Category |
|----------|-----------|----------|
| `LPGGDEVICE` | `ID3D11Device*` | Device |
| `LPGGIMMEDIATECONTEXT` | `ID3D11DeviceContext*` | Context |
| `LPGGTEXTURE` | `ID3D11Resource*` | Texture |
| `LPGGTEXTUREREF` | `ID3D11ShaderResourceView*` | Texture View |
| `LPGGCUBETEXTURE` | `ID3D11Resource*` | Cube Texture |
| `IGGVertexBuffer` | `ID3D11Buffer` | Vertex Buffer |
| `IGGIndexBuffer` | `ID3D11Buffer` | Index Buffer |
| `LPGGSURFACE` | `ID3D11Texture2D*` | Surface |
| `LPGGRENDERTARGETVIEW` | `ID3D11RenderTargetView*` | Render Target |
| `LPGGDEPTHSTENCILVIEW` | `ID3D11DepthStencilView*` | Depth Stencil |
| `LPGGRASTERIZERSTATE` | `ID3D11RasterizerState*` | Rasterizer State |
| `LPGGBLENDSTATE` | `ID3D11BlendState*` | Blend State |
| `LPGGDEPTHSTENCILSTATE` | `ID3D11DepthStencilState*` | Depth Stencil State |
| `GGSURFACE_DESC` | `D3D11_TEXTURE2D_DESC` | Texture Description |
| `GGLOCKED_RECT` | `D3D11_MAPPED_SUBRESOURCE` | Mapped Resource |

### 7.2 Format Mappings

| GG Format | DXGI Format |
|-----------|-------------|
| `GGFMT_A8R8G8B8` | `DXGI_FORMAT_B8G8R8A8_UNORM` |
| `GGFMT_DXT1` | `DXGI_FORMAT_BC1_UNORM` |
| `GGFMT_DXT5` | `DXGI_FORMAT_BC3_UNORM` |
| `GGFMT_D24S8` | `DXGI_FORMAT_D24_UNORM_S8_UINT` |
| `GGFMT_R16F` | `DXGI_FORMAT_R16_FLOAT` |
| `GGFMT_R32F` | `DXGI_FORMAT_R32_FLOAT` |
| `GGFMT_A16B16G16R16F` | `DXGI_FORMAT_R16G16B16A16_FLOAT` |
| `GGFMT_R8G8B8` | `0` (unsupported - must use RGBA) |

### 7.3 Usage Scope

The GG* macros propagate DX11 types into 30+ source files across:
- All DarkSDK modules (Objects, Camera, Image, Light, etc.)
- `Common-Images.cpp` - Cube map rendering
- `M-GridEdit_part*.cpp` - Editor texture references (30+ usages)
- `M-GridEditB_part*.cpp` - Extended editor

---

## 8. Other Engine Systems

### 8.1 Input System

- `wiInput::GetPointer()` - Mouse position (wickedcalls_part3.cpp)
- `wiInput::GetMouseState()` - Mouse button state (GGGrass.cpp, GGTerrain_part0.cpp, GGTrees_part0.cpp)
- `wiInput::Update(window, canvas)` - Per-frame update (master_part0.cpp)
- **Status:** Graphics-API-independent, no migration needed

### 8.2 Physics System

- Uses Bullet Physics library directly (not WickedEngine physics)
- `BulletDebugDrawer.cpp` has minimal WickedEngine integration
- No `RigidBodyComponent` or `ColliderComponent` usage in main codebase
- **Status:** Graphics-API-independent, no migration needed

### 8.3 Audio System

- Uses DirectSound/XAudio2 directly (not WickedEngine wiAudio)
- **Status:** No migration needed

### 8.4 Scripting (Lua)

- DarkLUA module, not integrated with WickedEngine graphics
- **Status:** No migration needed

### 8.5 Networking

- Photon and Steam stubs only (`photon_stubs.cpp`)
- **Status:** No migration needed

### 8.6 Profiling

- `wiProfiler::BeginFrame()` / `EndFrame(cmd)` - Frame markers
- `wiProfiler::BeginRangeCPU("name")` / `EndRange(range)` - CPU ranges
- **Status:** WickedEngine-abstracted, should work with DX12

---

## 9. Gap Analysis: Old API to New API Mapping

### 9.1 WickedEngine API (Already Abstracted)

| Current Usage | WickedEngine DX12 Equivalent | Migration Effort |
|--------------|------------------------------|-----------------|
| `wiGraphics::GetDevice()` | Same API, DX12 backend | **None** - abstracted |
| `CommandList` begin/end | Same API | **None** - abstracted |
| `RenderPass` begin/end | Same API | **None** - abstracted |
| `wiImage::Draw()` | Same API | **None** - abstracted |
| `wiFont::Draw()` | Same API | **None** - abstracted |
| `wiScene::GetScene()` | Same API | **None** - abstracted |
| `wiResourceManager::Load()` | Same API | **None** - abstracted |
| `wiRenderer::LoadShader()` | Same API (loads .cso files) | **Recompile shaders** |
| `device->CreateTexture()` | Same API | **None** - abstracted |
| `device->CreateBuffer()` | Same API | **None** - abstracted |
| `device->UpdateBuffer()` | Same API | **None** - abstracted |
| `device->CreatePipelineState()` | Same API | **None** - abstracted |
| `device->CreateSampler()` | Same API | **None** - abstracted |
| `device->BindResource()` | Same API | **None** - abstracted |
| `device->BindComputeShader()` | Same API | **None** - abstracted |
| ECS component access | Same API | **None** - abstracted |

### 9.2 Direct DX11 Code (Requires Migration)

| Current DX11 Usage | DX12 Equivalent | Migration Effort |
|--------------------|-----------------|-----------------|
| ImGui DX11 backend (9,249 lines) | ImGui DX12 backend or WickedEngine UI | **HIGH** |
| `ID3D11Device*` / `ID3D11DeviceContext*` globals | `ID3D12Device*` / command lists | **HIGH** |
| `D3D11_MAP_WRITE_DISCARD` buffer mapping | Upload heap + copy | **MEDIUM** |
| `D3DCompile()` runtime shader compilation | Offline compilation or DXC | **MEDIUM** |
| `CreateRenderTargetView` per-viewport | Descriptor heap management | **HIGH** |
| DX11 state backup/restore | Pipeline state objects | **HIGH** |
| `ctx->DrawIndexed()` | Command list recording | **MEDIUM** |
| GG* macro layer (200+ macros) | Redefine to DX12 types or remove | **HIGH** |
| `GetDeviceForIMGUI()` / `GetImmediateForIMGUI()` | DX12 interop or eliminate | **HIGH** |
| OpenXR DX11 integration (VR path) | OpenXR DX12 binding | **MEDIUM** |

### 9.3 Shader Recompilation

| Shader Set | Count | Format Change |
|------------|-------|---------------|
| GGTerrain HLSL | 40+ | Recompile with SM 6.0+ target |
| Custom Object Shaders | 15+ | Recompile with SM 6.0+ target |
| GPU Particle Shaders | 8 | Recompile with SM 6.0+ target |
| ImGui inline shaders | 5+ | Replace D3DCompile with offline/DXC |

---

## 10. Statistics Summary

| Category | Count |
|----------|-------|
| WickedEngine API touch points | 150+ |
| Direct DX11 API functions used | 35+ |
| ImGui DX11 backend lines | 9,249 |
| GG* macro definitions | 200+ |
| HLSL shader files | 63+ |
| PSO objects created | 30+ |
| GPUBuffer objects | 20+ |
| Texture objects (custom) | 25+ |
| ECS component types used | 14 |
| Source files with DX11 types | 30+ |
| Graphics-independent modules | Physics, Audio, Lua, Networking, Input |
