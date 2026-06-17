# GameGuruWickedMAX - Project Map

## Solution Structure

**Primary Solution:** `GameGuru Core\GameGuruWickedMAX.sln`
**Build:** `build.bat Debug|Release [rebuild]` (MSVC v143, x64 only)

### .vcxproj Files in the Solution

| Project | Path | Purpose |
|---------|------|---------|
| **Wicked-MAX** | `Guru-WickedMAX\Template_Windows.vcxproj` | Main engine executable - ties everything together |
| DBDLLCore | `Dark Basic Public Shared\...\DarkSDK\Core\DBDLLCore.vcxproj` | Core runtime library |
| Objects | `DarkSDK\Objects\Objects.vcxproj` | 3D object management |
| Camera | `DarkSDK\Camera\Camera.vcxproj` | Camera system |
| Image | `DarkSDK\Image\Image.vcxproj` | Image/texture management |
| Light | `DarkSDK\Light\Light.vcxproj` | Lighting system |
| Sound | `DarkSDK\Sound\Sound.vcxproj` | Audio playback |
| Music | `DarkSDK\Music\Music.vcxproj` | Music playback |
| Input | `DarkSDK\Input\Input.vcxproj` | Keyboard/mouse input |
| File | `DarkSDK\File\File.vcxproj` | File I/O operations |
| Text | `DarkSDK\Text\Text.vcxproj` | Text rendering |
| Basic2D | `DarkSDK\Basic2D\Basic2D.vcxproj` | 2D drawing primitives |
| Bitmap | `DarkSDK\Bitmap\Bitmap.vcxproj` | Bitmap handling |
| Sprites | `DarkSDK\Sprites\Sprites.vcxproj` | 2D sprite system |
| Animation | `DarkSDK\Animation\Animation.vcxproj` | Animation playback |
| Particles | `DarkSDK\Particles\Particles.vcxproj` | Legacy particle system |
| Bullet | `DarkSDK\Bullet\Bullet.vcxproj` | Bullet physics integration |
| Vectors | `DarkSDK\Vectors\Vectors.vcxproj` | Vector/matrix math |
| Matrix | `DarkSDK\Matrix\Matrix.vcxproj` | Matrix terrain (legacy) |
| AdvancedMatrix | `DarkSDK\AdvancedMatrix\AdvancedMatrix.vcxproj` | Extended matrix operations |
| Transforms | `DarkSDK\Transforms\Transforms.vcxproj` | Transform utilities |
| Memblocks | `DarkSDK\Memblocks\Memblocks.vcxproj` | Raw memory block access |
| Setup | `DarkSDK\Setup\Setup.vcxproj` | System setup/init |
| System | `DarkSDK\System\System.vcxproj` | System utilities |
| Conv3DS | `DarkSDK\Conv3DS\Conv3DS.vcxproj` | 3DS format converter |
| ConvX | `DarkSDK\ConvX\ConvX.vcxproj` | .X format converter |
| FTP | `DarkSDK\FTP\FTP.vcxproj` | FTP client |
| Multiplayer | `DarkSDK\Multiplayer\Multiplayer.vcxproj` | Multiplayer base |
| MultiplayerPlus | `DarkSDK\MultiplayerPlus\MultiplayerPlus.vcxproj` | Extended multiplayer |
| DarkLUA | `DarkSDKMore\DarkLUA\DarkLUA.vcxproj` | Lua scripting engine |
| DarkMind | `DarkSDKMore\DarkAI\DarkMind.vcxproj` | AI pathfinding/behavior |
| LightMapper | `DarkSDKMore\DarkLIGHTS\LightMapper.vcxproj` | Lightmap baking |
| BlitzTerrain | `DarkSDKMore\BlitzTerrain\BlitzTerrain.vcxproj` | Legacy terrain system |
| BlitzTerrain_RTTMSPlugin | `DarkSDKMore\BlitzTerrain_RTTMSPlugin\BlitzTerrain_RTTMSPlugin.vcxproj` | Terrain plugin |
| Enhancements (Main) | `DarkSDKMore\Enhancements\Main.vcxproj` | Misc enhancements (zip, etc.) |
| Ogg Vorbis | `DarkSDKMore\Enhancements\ogg vorbis\Ogg Vorbis.vcxproj` | Audio codec |
| ZipArchive | `DarkSDKMore\Enhancements\zip\ZipArchive.vcxproj` | ZIP file support |
| GameFX | `DarkSDKMore\GameFX\GameFX.vcxproj` | Game effects |
| SimonReloaded | `DarkSDKMore\SimonReloaded\SimonReloaded.vcxproj` | CSG/geometry operations |
| SimonCSG | `DarkSDKMore\SimonCSG\SimonCSG.vcxproj` | CSG operations |
| CPU3DTest | `DarkSDKMore\CPU3D\CPU3DTest.vcxproj` | Software rasterizer |
| GGVR | `DarkSDKMore\GGVR\GGVR.vcxproj` | VR support |
| GGWMR | `DarkSDKMore\GGWMR\GGWMR.vcxproj` | Windows Mixed Reality |
| PhotonMultiplayer | `DarkSDKMore\PhotonMultiplayer\PhotonMultiplayer.vcxproj` | Photon networking |
| SteamMultiplayer | `DarkSDKMore\SteamMultiplayer\SteamMultiplayer.vcxproj` | Steam networking |
| DirectXTex | `SDK\DirectXTex\DirectXTex\DirectXTex_Desktop_2022.vcxproj` | Texture processing lib |
| libogg | `SDK\OGG\libogg\win32\VS2022\libogg.vcxproj` | OGG container format |
| libvorbis_static | `SDK\OGG\libvorbis\...\libvorbis_static.vcxproj` | Vorbis audio codec |
| libvorbisfile_static | `SDK\OGG\libvorbis\...\libvorbisfile_static.vcxproj` | Vorbis file I/O |

**Separate solutions (not part of main build):**
- `GameGuru Launcher MAX\GameGuru Launcher MAX.sln` - Launcher application
- `Simple Sound Recorder\voicerecorder.sln` - Audio recording utility

---

## Folder Structure & Module Purposes

### `Guru-WickedMAX/` - WickedEngine Integration Layer
The primary bridge between GameGuru and the WickedEngine renderer. Contains the main executable project.

| Subfolder/File | Purpose |
|----------------|---------|
| `main.cpp` | Application entry point; creates `Master` instance, parses startup args |
| `master.h` / `master_part*.cpp` | `MasterRenderer` class (extends `RenderPath3D`); scene setup, render passes, post-processing, VR |
| `GameGuruMain.h` / `GameGuruMain.cpp` | Game loop: `GuruMain()`, `GuruLoopLogic()`, `GuruLoopRender()`, `GuruUpdate()`, `GuruFinish()` |
| `wickedcalls.h` / `wickedcalls_part0-4.cpp` | **200+ wrapper functions** bridging GameGuru objects to WickedEngine scene/ECS |
| `GGTerrain/` | Procedural terrain engine (terrain, grass, trees) with custom HLSL shaders |
| `GGRecastDetour/` | Navigation mesh system (Recast/Detour integration for AI pathfinding) |
| `Particles/` | GPU particle system (`GPUParticles.h`) with custom shaders |
| `tracers/` | Bullet tracer rendering (TracerManager) |
| `Nlohmann JSON/` | JSON parsing library (header-only) |
| `Wicked-MAX/x64/` | Pre-built WickedEngine library binaries |
| `BulletDebugDrawer.cpp/.h` | Physics debug visualization |
| `CrashLogger.h` | Crash reporting |
| `Encryptor.h` | File encryption utilities |
| `GGThread.h` | Threading helpers |
| `ModelImporter_OBJ.cpp` | OBJ model loading into WickedEngine scene |
| `CustomShaders.cpp` | Custom shader compilation and registration |
| `PNGToDDS*.cpp` | Texture format conversion pipeline |

### `GameGuru/` - Game Logic & Editor
Core gameplay systems, editor UI, and game mechanics.

**`GameGuru/Source/`** (~155 .cpp files, many split into `_partN.cpp` chunks):

| Module Prefix | Files | Purpose |
|---------------|-------|---------|
| `Common*` | `Common_part0-3.cpp`, `Common-File.cpp`, `Common-Fonts.cpp`, `Common-Images.cpp`, `Common-Input.cpp`, `Common-Keys.cpp`, `Common-Sounds.cpp`, `Common-Strings.cpp` | Foundation utilities, file I/O, input, image loading, DX11 device init |
| `G-Entity*` | `G-Entity_part0-3.cpp` | Core entity runtime (spawning, lifecycle, damage, attachments) |
| `G-Gun*` | `G-Gun_part0-3.cpp` | Core weapon/gun runtime |
| `G-Lighting` | `G-Lighting.cpp` | Runtime lighting |
| `M-Game*` | `M-Game_part0-3.cpp` | Game state machine, level loading, main game loop |
| `M-Entity*` | `M-Entity_part0-5.cpp` | Entity management (loading, saving, selection, properties, materials) |
| `M-GridEdit*` | `M-GridEdit_part0-8.cpp` | Level editor core (camera, preview, editing) |
| `M-GridEditB*` | `M-GridEditB_part0-24.cpp` | Extended editor UI (the largest module, ~25 part files) |
| `M-Gun*` | `M-Gun_part0-1.cpp` | Weapon management |
| `M-DAI*` / `M-DAINew*` | `M-DAI_part0-1.cpp`, `M-DAINew_part0-1.cpp` | AI systems (legacy and new) |
| `M-CharacterCreatorPlus*` | 4 part files + TTS | Character creation system |
| `M-Importer*` | `M-Importer_part0-7.cpp` | Asset importing (FBX, OBJ via Assimp) |
| `M-EBE*` | `M-EBE_part0-2.cpp` | Easy Building Editor (constructive geometry) |
| `M-Lightmapping*` | 2 part files | Lightmap baking system |
| `M-Physics*` | Split files | Bullet physics integration, player/character physics |
| `MapEditor.cpp` | 1 file | Map editor entry point |
| Other `M-*` files | 1 file each | Audio volumes, bullet holes, character sound, debug, decals, explosions/fire, HUD, interactive objects, lighting, postprocess, ragdoll, sky, etc. |

**`GameGuru/Include/`** (71 header files):

| Header | Role |
|--------|------|
| `gameguru.h` | **Master include** - pulls in all 60+ module headers plus DarkSDK and wickedcalls |
| `Types.h` | Core type definitions, constants (`MAXTEXTURESIZE`, `MAXPATH`), weapon/animation enums |
| `Common.h` | Common init, setup, and main loop function declarations |
| `preprocessor-flags.h` / `preprocessor-moreflags.h` | Compile-time feature flags |
| `G-Entity.h` | Entity runtime API |
| `G-Gun.h` | Weapon runtime API |
| `M-Game.h` | Game init, main loop, level loading, state management |
| `M-Entity.h` | Entity management (loading, saving, selection, properties) |
| `M-GridEdit.h` / `M-GridEditB.h` | Editor functionality |
| `M-Physics.h` | Physics simulation, player control, damage |
| `M-LUA.h` / `M-LUA-General.h` / `M-LUA-Entity.h` | Lua scripting engine and bindings |
| `M-Weapon.h` | Weapon loading, animation, firing, projectiles |
| `M-Particles.h` | Ravey particle system (emitters, effects) |
| `M-DAINew.h` / `M-DAI.h` | AI pathfinding, animation, behavior |
| `M-Terrain.h` / `M-TerrainNew.h` | Terrain editing and painting |
| `M-Importer.h` | Model importer (`sImportedObjectData` struct) |
| `M-HUD.h` | HUD layers, blood, damage markers |
| `M-MapFile.h` | Map load/save, project management, standalone export |
| `M-Material.h` | PBR material properties |
| `M-Ragdoll.h` | Ragdoll physics |
| `M-CharacterCreatorPlus.h` | Character creation |
| `M-MP.h` | Multiplayer |
| `M-RPG.h` | RPG gameplay systems |
| `M-OBS.h` | OBS streaming integration |
| `M-UndoSys*.h` | Undo/redo for objects and terrain |
| `cStr.h` | String utility class |

**`GameGuru/Imgui/`** (~40 files) - Editor UI framework:

| Files | Purpose |
|-------|---------|
| `imgui.h` / `imgui_part0-7.cpp` | Dear ImGui core (split into 8 parts) |
| `imgui_widgets_part0-4.cpp` | ImGui widgets |
| `imgui_draw_part0-1.cpp` | ImGui draw system |
| `imgui_gg_dx11.h` / `imgui_gg_dx11_part0-5.cpp` | **DX11 rendering backend for ImGui** (the primary DX11 code in the project) |
| `imgui_impl_win32.cpp/.h` | Win32 platform backend |
| `imnodes.h` / `imnodes_part0-1.cpp` | Node editor (used for storyboard) |

### `Dark Basic Public Shared/` - Legacy DarkBasic SDK
Wrapper libraries originally from Dark Basic Professional, adapted for DX11/WickedEngine.

| Folder | Key Files | Purpose |
|--------|-----------|---------|
| `DarkSDK/Core/` | `DBDLLCore.h/.cpp` | Core runtime, device management |
| `DarkSDK/Objects/` | `CObjectsC.h`, `CObjectManagerWicked_part*.cpp` | 3D object management (WickedEngine-adapted) |
| `DarkSDK/Camera/` | `CCameraC.h/.cpp` | Camera positioning and control |
| `DarkSDK/Sound/` | `CSoundC.h/.cpp` | Sound loading, playback, 3D audio |
| `DarkSDK/Input/` | `CInputC.h/.cpp` | DirectInput keyboard/mouse |
| `DarkSDK/Image/` | `CImageC.h/.cpp` | Image/texture resource management |
| `DarkSDK/Light/` | `CLightC.h/.cpp` | Light creation and properties |
| `DarkSDKMore/DarkLUA/` | `DarkLUA_part*.cpp` | Lua scripting with game engine bindings |
| `DarkSDKMore/DarkAI/` | `DarkMind.vcxproj` | AI pathfinding and entity behavior |
| `Shared/Objects/` | `CObjectDataC.h` | Core `sObject`/`sMesh` data structures shared across all modules |
| `Include/` | All `C*C.h` headers | Public API for each DarkSDK module |

### `SDK/` - Third-Party Libraries

| Folder | Purpose |
|--------|---------|
| `BULLET/` | Bullet Physics v3.19 - rigid body, collision, constraints |
| `DirectX/` | DirectX SDK headers + Effects11 framework |
| `DirectXTex/` | Microsoft DirectXTex - texture loading/conversion/compression |
| `EOS-SDK/` | Epic Online Services - platform features |
| `NVAPI/` | NVIDIA GPU-specific optimizations |
| `OGG/` | OGG Vorbis audio codec (libogg + libvorbis) |
| `OpenXR/` | VR/XR runtime support |
| `OPTICK/` | CPU/GPU performance profiler |
| `PhotonSDK/` | Photon real-time multiplayer networking |
| `RecastContrib/` | Recast/Detour navmesh contrib (fastlz, SDL) |
| `Steamworks SDK/` | Steam platform integration (achievements, matchmaking) |
| `THEORA/` | Theora video codec |
| `BaseClasses/` | DirectShow base classes |
| `DB3/` | DarkBasic 3 legacy support |

### `GameGuruPreprocessor/`
Contains `preprocessor-flags-wicked.h` - compile-time feature flags for WickedEngine integration.

### Parent-Level Directories (`D:\max\GameGuruMAXDX12\`)

| Folder | Purpose |
|--------|---------|
| `GameGuru Core/` | All engine source (this project) |
| `WickedEngineDX12/` | WickedEngine rendering engine (sibling dependency, DO NOT MODIFY) |
| `GameGuru Launcher MAX/` | Launcher application |
| `Guides/` | User guides (Behaviors, Characters, Weapons, NPC, Quest, HUD, etc.) |
| `Scripts/` | Game content banks (`imagebank/`, `scriptbank/`, `titlesbank/`) |
| `Max Collection Misc/` | Test models |
| `Simple Sound Recorder/` | Audio recording utility |

---

## WickedEngine Integration Points

### Architecture Overview

```
main.cpp
  └─> Master : MainComponent              (master.h)
        └─> MasterRenderer : RenderPath3D  (master.h)
              │   Load() / Update() / Render() / Compose()
              │
              ├─> wickedcalls (200+ bridge functions)
              │     ├─> wiScene::GetScene()     (ECS access)
              │     ├─> wiResourceManager        (asset loading)
              │     ├─> wiRenderer               (picking, device, postprocess)
              │     └─> wiEmittedParticle        (particles)
              │
              ├─> GGTerrain system              (custom terrain + shaders)
              ├─> GPUParticles system            (custom GPU particles)
              ├─> TracerManager                  (bullet tracers)
              └─> CustomShaders                  (runtime shader compilation)
```

### WickedEngine Include Path
`wickedcalls.h` line 29: `#include "../../../WickedEngineDX12/WickedEngine/WickedEngine.h"`

The project uses the backwards compatibility layer (`WICKEDENGINE_BACKWARDS_COMPATIBILITY_0_59`) providing namespace aliases:
- `wiScene` = `wi::scene`, `wiRenderer` = `wi::renderer`, `wiECS` = `wi::ecs`, etc.

### Key Integration Files

| File | WickedEngine Subsystems Used |
|------|------------------------------|
| `master.h` / `master_part0-1.cpp` | `RenderPath3D`, `MainComponent`, `wiGraphics::CommandList`, `wiGraphics::Texture`, `wiRenderer`, `wiScene` (cameras, lights) |
| `wickedcalls_part0.cpp` | `wiResourceManager::Load/FreeResource`, `wiScene` (ObjectComponent, TransformComponent, MeshComponent, MaterialComponent, AnimationComponent, ArmatureComponent) |
| `wickedcalls_part1.cpp` | `wiScene::MeshComponent`, `MaterialComponent` (texturing) |
| `wickedcalls_part2.cpp` | `wiScene` components (shadows, outlines, transparency, render layers) |
| `wickedcalls_part3.cpp` | `wiRenderer::GetPickRay`, `wiScene::Pick_OLD`, ray casting |
| `wickedcalls_part4.cpp` | `wiScene::EmitterComponent`, `wiEmittedParticle` (particle emitters) |
| `GGTerrain/GGTerrain_part0-1.cpp` | `wiGraphicsDevice`, `wiScene::CameraComponent`, `wiRenderer` |
| `GGTerrain/GGTerrainWicked.cpp/.h` | `wi::terrain::Terrain`, `ChunkData`, `BlendmapLayer`, custom `Modifier` — Phase 0–3 port of terrain to Wicked Engine native pipeline. `SCRATCHPAD.md` is the living roadmap |
| `GGTerrain/GGTrees_part0-1.cpp` | `wiScene::GetCamera`, `wiRenderer`, `AnimationComponent` |
| `GGTerrain/GGGrass.cpp/.h` | `wiGraphicsDevice`, `wiScene`, `wiRenderer` |
| `GGAnimBridge.cpp/.h` | `wiScene::AnimationComponent` Pause/Play — Phase 9 animation culling (visibility check + 30 fps half-rate throttle). PreUpdate culls, PostUpdate restores. Update-Wicked −56%, FPS +33% |
| `GPUParticles_part0.cpp` | `wiScene`, `wiEmittedParticle`, `TransformComponent` |
| `tracers/TracerManager.cpp` | `wiGraphics::CommandList`, `wiRenderer`, Pipeline State Objects |
| `ModelImporter_OBJ.cpp` | `wiScene::MeshComponent`, `MaterialComponent`, `LayerComponent`, `ArmatureComponent` |
| `CustomShaders.cpp` | `wiRenderer` (shader compilation) |
| `GameGuruMain.cpp` | `wiProfiler::BeginRangeCPU` |

### WickedEngine ECS Components Used
- `ObjectComponent` - Entity representation
- `TransformComponent` - Position/rotation/scale
- `MeshComponent` - Vertex/index data with subsets
- `MaterialComponent` - PBR properties (roughness, metalness, emissive, reflectance)
- `AnimationComponent` - Keyframe animation
- `ArmatureComponent` - Skeletal hierarchies
- `LightComponent` - Directional/spot/point lights
- `CameraComponent` - View/projection
- `EmitterComponent` - Particle emitters
- `LayerComponent` - Render layer masks (via `GGRENDERLAYERS` enum)
- `NameComponent` - Entity naming
- `HierarchyComponent` - Parent-child relationships
- `ColliderComponent` - Physics colliders
- `RigidBodyComponent` - Rigid body simulation

### WickedEngine Dependency Location
`D:\max\WickedEngineDX12\WickedEngine\` provides ~100 public headers:
- `wiRenderer.h` - Rendering pipeline
- `wiScene.h` - Scene graph / ECS
- `wiGraphicsDevice_DX12.h` - DX12 graphics backend
- `wiResourceManager.h` - Asset loading
- `wiImage.h` / `wiFont.h` - 2D rendering
- `wiInput.h` - Input abstraction
- `wiAudio.h` / `wiAudioMAX.h` - Audio system
- `wiPhysics.h` - Physics integration
- `wiTerrain.h` - Built-in terrain (GameGuru uses its own)
- `wiProfiler.h` - Performance profiler
- `wiGUI.h` - UI framework
- `wiLua.h` - Lua integration

---

## DX11-Specific API Usage

Despite the project name referencing DX12 and using WickedEngine (which renders via DX12), there is **significant legacy Direct3D 11 code** throughout the codebase.

### DX11 Abstraction Macro Layer

**File:** `SDK/DirectX/directx-macros.h`

This file defines `#define DX11` and maps all engine graphics types to D3D11:

```cpp
#define LPGGDEVICE          IGGDevice*              // = ID3D11Device*
#define LPGGIMMEDIATECONTEXT ID3D11DeviceContext*
#define LPGGTEXTUREREF      ID3D11ShaderResourceView*
#define LPGGRENDERTARGETVIEW ID3D11RenderTargetView*
#define LPGGDEPTHSTENCILVIEW ID3D11DepthStencilView*
#define LPGGSWAPCHAIN       IDXGISwapChain*
#define LPGGVERTEXBUFFER    ID3D11Buffer*
#define LPGGINDEXBUFFER     ID3D11Buffer*
// ... 40+ more macros mapping DX9 names -> DX11 types
```

These `GG*` macros are used extensively across the DarkSDK modules and GameGuru source to abstract the graphics API. All resolve to DX11 types.

### ImGui DX11 Backend (Primary DX11 Code)

**Files:** `GameGuru/Imgui/imgui_gg_dx11.h` + `imgui_gg_dx11_part0-5.cpp`

This is the editor's rendering backend and contains direct DX11 API calls:

**Device/Context globals** (`imgui_gg_dx11_part0.cpp:87-107`):
```cpp
static ID3D11Device*            g_pd3dDevice = NULL;
static ID3D11DeviceContext*     g_pd3dDeviceContext = NULL;
static IDXGIFactory*            g_pFactory = NULL;
static ID3D11Buffer*            g_pVB = NULL;
static ID3D11Buffer*            g_pIB = NULL;
static ID3D11VertexShader*      g_pVertexShader = NULL;
static ID3D11PixelShader*       g_pPixelShader = NULL;
// + multiple additional pixel shader variants (blur, nowhite, noalpha, boost25)
```

**DX11 API calls used:**
- `D3DCompile()` - Runtime shader compilation (vs_5_0, ps_5_0)
- `g_pd3dDevice->CreateVertexShader()` / `CreatePixelShader()`
- `g_pd3dDevice->CreateBuffer()` - Vertex/index/constant buffers
- `g_pd3dDevice->CreateTexture2D()` - Texture creation
- `g_pd3dDevice->CreateSamplerState()`
- `g_pd3dDevice->CreateInputLayout()`
- `g_pd3dDevice->CreateRenderTargetView()`
- `ctx->Map()` / `ctx->Unmap()` - Buffer updates (`D3D11_MAP_WRITE_DISCARD`)
- `ctx->IASetInputLayout()`, `IASetVertexBuffers()`, `IASetIndexBuffers()`
- `ctx->VSSetShader()`, `PSSetShader()`, `VSSetConstantBuffers()`
- `ctx->PSSetSamplers()`, `PSSetShaderResources()`
- `ctx->RSSetViewports()`, `RSSetScissorRects()`
- `ctx->OMSetBlendState()`, `OMSetDepthStencilState()`, `OMSetRenderTargets()`
- `ctx->DrawIndexed()`

**Init entry point** (`Common_part0.cpp:693`):
```cpp
ImGui_ImplDX11_Init(m_pD3D, m_pImmediateContext);
```

### DX11 Types in GameGuru Source Files

The `GG*` macros from `directx-macros.h` propagate DX11 types into 30+ source files:

- `Common-Images.cpp` - `ID3D11Texture2D*`, `ID3D11RenderTargetView*`, `ID3D11ShaderResourceView*` for cube map rendering
- `M-GridEdit_part*.cpp` - `ID3D11ShaderResourceView*` for editor texture references (30+ usages)
- `M-GridEditB_part*.cpp` - Same pattern for extended editor
- All DarkSDK modules via `LPGGDEVICE`, `LPGGTEXTUREREF`, etc.

### DX11 Includes

- `imgui_gg_dx11_part0.cpp:25-27`: `#include <d3d11.h>`, `#include <d3dcompiler.h>`, `#include <dxgi.h>`
- `SDK/DirectX/directx-macros.h:13-16`: `#include <D3D11.h>`, `#include <D3DX11.h>`, `#include "d3dcompiler.h"`, `#include "d3dx11effect.h"`

### DX12 Usage

WickedEngine handles all DX12 rendering internally. The GameGuru codebase itself has **no direct DX12 API calls**. DX12 references exist only in:
- `SDK/OPTICK/src/optick_gpu.d3d12.cpp` (profiler sample)
- `SDK/OPTICK/samples/WindowsD3D12/` (sample code)

### Summary: Dual-Graphics Architecture

```
  WickedEngine (DX12)                    GameGuru Legacy (DX11)
  ═══════════════════                    ══════════════════════
  3D scene rendering                     ImGui editor UI rendering
  Terrain/grass/trees                    GG* macro abstraction layer
  Particles, post-processing             DarkSDK module internals
  Lighting, shadows                      Texture/RT view creation
  Model loading                          Buffer map/unmap for UI
```

The engine runs a **hybrid architecture**: WickedEngine provides the DX12 rendering pipeline for all 3D content, while the editor UI (ImGui) and many DarkSDK subsystems still operate through a DX11 device and context obtained from WickedEngine's interop layer.

---

## Module Graphics Dependency Classification

Modules are classified by their dependency on graphics APIs, relevant for the DX12 migration.

### Graphics-Heavy Modules (Require Migration Attention)

| Module | Graphics API | Notes |
|--------|-------------|-------|
| `Guru-WickedMAX/master_part0-1.cpp` | WickedEngine (abstracted) + DX11 interop for VR | VR path uses DX11 RTVs via OpenXR |
| `Guru-WickedMAX/wickedcalls_part0-4.cpp` | WickedEngine (abstracted) | 200+ bridge functions, all through WickedEngine API |
| `Guru-WickedMAX/GGTerrain/` | WickedEngine device API | Custom shaders, pipeline states, GPU buffers, compute shaders |
| `Guru-WickedMAX/GPUParticles_part0.cpp` | WickedEngine device API | Custom shaders, pipeline states, GPU buffers, samplers |
| `Guru-WickedMAX/CustomShaders.cpp` | WickedEngine shader API | Runtime shader loading and PSO creation |
| `Guru-WickedMAX/ModelImporter_OBJ.cpp` | WickedEngine scene/ECS | Material and mesh creation |
| `GameGuru/Imgui/imgui_gg_dx11_part0-5.cpp` | **Direct DX11** (9,249 lines) | Primary DX11 code - must be migrated to DX12 |
| `GameGuru/Source/Common-Images.cpp` | DX11 via GG* macros | Cube map rendering, texture creation |
| `GameGuru/Source/M-GridEdit_part*.cpp` | DX11 via GG* macros | Editor texture references (30+ usages) |
| `SDK/DirectX/directx-macros.h` | DX11 type definitions | 200+ macros mapping GG* names to DX11 types |

### Graphics-Light Modules (Minimal or No Migration)

| Module | Graphics API | Notes |
|--------|-------------|-------|
| `Guru-WickedMAX/main.cpp` | WickedEngine (abstracted) | Window setup and main loop only |
| `Guru-WickedMAX/GameGuruMain.cpp` | WickedEngine (abstracted) | Game loop orchestration |
| `Guru-WickedMAX/tracers/` | WickedEngine (abstracted) | Uses CommandList and wiRenderer |
| `GameGuru/Source/M-Game_part*.cpp` | None | Game state machine, level loading |
| `GameGuru/Source/G-Entity_part*.cpp` | None | Entity runtime (spawning, damage, lifecycle) |
| `GameGuru/Source/G-Gun_part*.cpp` | None | Weapon runtime |
| `GameGuru/Source/M-Entity_part*.cpp` | Minimal (GG* texture refs) | Entity management |

### Graphics-Independent Modules (No Migration Needed)

| Module | Notes |
|--------|-------|
| `DarkSDK/Sound/`, `DarkSDK/Music/` | DirectSound/XAudio2 - no DX11 |
| `DarkSDK/Input/` | DirectInput - no DX11 |
| `DarkSDK/File/`, `DarkSDK/Memblocks/` | Pure data I/O |
| `DarkSDK/Vectors/`, `DarkSDK/Matrix/`, `DarkSDK/Transforms/` | Pure math |
| `DarkSDKMore/DarkLUA/` | Lua scripting - no graphics |
| `DarkSDKMore/DarkAI/` | AI pathfinding/behavior - no graphics |
| `DarkSDKMore/Enhancements/` | ZIP, OGG Vorbis - no graphics |
| `DarkSDKMore/SimonReloaded/`, `DarkSDKMore/SimonCSG/` | CSG geometry - no graphics |
| `DarkSDKMore/PhotonMultiplayer/`, `DarkSDKMore/SteamMultiplayer/` | Networking stubs |
| `DarkSDK/Bullet/` | Bullet Physics - no graphics |
| `GameGuru/Source/M-Physics_part*.cpp` | Bullet Physics integration |
| `GameGuru/Source/M-DAI*.cpp`, `M-DAINew*.cpp` | AI systems |
| `GameGuru/Source/M-CharacterCreatorPlus*.cpp` | Character creation logic |
| `GameGuru/Source/M-Importer_part*.cpp` | Asset importing (Assimp) |
| `GameGuru/Source/M-MapFile.cpp` | Map save/load |
| `GameGuru/Source/M-LUA*.cpp` | Lua bindings |
| `SDK/BULLET/` | Physics library |
| `SDK/OGG/` | Audio codec |
| `SDK/DirectXTex/` | Texture processing (DXGI-based, API-neutral) |
| `Guru-WickedMAX/GGRecastDetour/` | Navigation mesh - no graphics |
| `Guru-WickedMAX/Nlohmann JSON/` | JSON parsing |

### Migration Reference Documents

- **DX12_AUDIT.md** - Detailed audit of every graphics API call in the codebase
- **MIGRATION_PLAN.md** - Ordered migration plan with phases, risks, and dependencies
