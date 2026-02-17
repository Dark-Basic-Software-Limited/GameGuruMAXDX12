# Task: Create a Detailed Plan to Port GameGuru MAX Custom DX11 Shaders to Wicked Engine DX12

## Context

GameGuru MAX uses a modified version of Wicked Engine (v0.71.x). The product has **62 custom shader and shader header files** (~350KB total) located in:

```
D:\max\GameGuruMAXDX12\GameGuru Core\Guru-WickedMAX\GGTerrain\Shaders\
```

with PBR/engine shared headers in a `PBR/` subfolder beneath that.

These shaders were written for the **DX11 rendering path** of an older Wicked Engine fork. They are now being loaded by the DX12 backend and **failing at runtime**:

1. `D3D12CreateVersionedRootSignatureDeserializer` fails with `E_INVALIDARG (0x80070057)` — the `.cso` files are SM 5.0 DXBC bytecode (compiled by `fxc.exe`), lacking the `RTS0` root signature chunk that the DX12 backend requires.
2. Pipeline state creation fails, cascading into disabled features and visual corruption.

## Priority

**Get it compiling and running first.** Minimum viable port — shaders compile with DXC to SM 6.0+ DXIL, load without errors, and render correctly. Optimization later.

---

## Complete Shader Inventory (All 62 Files Analyzed)

### Shared Engine Headers (PBR/ subfolder)

| File | Role |
|------|------|
| `PBR/ShaderInterop.h` | Master header — CBUFFER/TEXTURE2D/SAMPLERSTATE macros; has `#ifdef HLSL6`/`SPIRV` paths for PUSHCONSTANT and BINDLESS |
| `PBR/ConstantBufferMapping.h` | CB slot #defines (b0=Frame, b1=Camera, b2-b10=on-demand) |
| `PBR/SamplerMapping.h` | Sampler slot #defines (s0-s3=on-demand, s4-s15=persistent) |
| `PBR/ResourceMapping.h` | Texture/SRV slot #defines (t0-t29=engine, t30-t63=on-demand, TEXSLOT_COUNT=64) |
| `PBR/ShaderInterop_Renderer.h` | FrameCB, CameraCB, MaterialCB, ShaderEntity, ShaderMaterial, ForwardEntityMaskCB, CubemapRenderCB structs |
| `PBR/globals.hlsli` | Declares all engine textures/samplers/buffers via macros; utility functions (fog, tonemap, dither, ray tracing helpers) |
| `PBR/brdf.hlsli` | BRDF functions + Surface struct (includes globals.hlsli) |
| `PBR/lightingHF.hlsli` | Lighting struct, DirectionalLight, PointLight, SpotLight, shadow sampling (includes brdf, voxelConeTracingHF, skyHF) |
| `PBR/skyHF.hlsli` | GetDynamicSkyColor, sky rendering |
| `PBR/skyAtmosphere.hlsli` | Atmospheric scattering math |
| `PBR/voxelConeTracingHF.hlsli` | Voxel GI cone tracing |

### Shared GameGuru Headers

| File | Role |
|------|------|
| `GGCommonFunctions.hlsli` | `ApplyFogCustom()` — used by all Tier 3 main-render PS shaders |
| `GGTerrainPageSettings.h` | Virtual texture constants (page sizes, padding, filtering) — shared CPU/GPU |

### Binding Model Architecture

The macros in `ShaderInterop.h` expand differently per platform:

```hlsl
// Shader side (current DX11 path):
CBUFFER(name, slot)                    → cbuffer name : register(b##slot)
TEXTURE2D(name, type, slot)            → Texture2D<type> name : register(t##slot)
STRUCTUREDBUFFER(name, type, slot)     → StructuredBuffer<type> name : register(t##slot)
SAMPLERSTATE(name, slot)               → SamplerState name : register(s##slot)

// C++ side:
CBUFFER(name, slot) → static const int CB_GETBINDSLOT(name) = slot; struct alignas(16) name

// Already exists but unused by custom shaders:
#ifdef HLSL6
  PUSHCONSTANT(name, type) → ConstantBuffer<type> name : register(b999)
#endif
```

### Constant Buffers Used

| Slot | Name | Contents | Used By |
|------|------|----------|---------|
| `b0` | `FrameCB` | Sun, fog, time, options, atmosphere, water, entity counts, voxel GI, etc. (~660 bytes) | All shaders via `globals.hlsli` |
| `b0` | Custom `viewProj` | Just a `float4x4` — **conflicts with FrameCB** | `GGTerrainReadBackVS` only (isolated pass) |
| `b1` | `CameraCB` | VP matrix, cam pos, clip plane, frustum, jitter, prev-frame matrices | Most VS/PS |
| `b2` | `TerrainCB` | LOD levels[16], layers[5], slopes[2], mask rotation[64], ramp world matrix | All terrain shaders |
| `b2` | `GrassCB` | Rotation matrices[32], grass types[46], LOD distance, scale, flags | All grass shaders |
| `b2` | `TreeCB` | Rotation matrices[8+1], tree types[38], player pos, LOD distances | All tree shaders |
| `b7` | `ForwardEntityMaskCB` | `xForwardLightMask` (uint2), decal/envprobe masks | Env probe PS shaders |
| `b8` | `CubemapRenderCB` | `xCubemapRenderCams[6]` VP matrices | Env probe VS shaders |

### Custom Texture Slot Usage

| System | Slots | Resources |
|--------|-------|-----------|
| **Terrain (non-VT)** | t40-t47 | Color/Normal pairs for grass, rock, snow, sand materials |
| **Terrain (VT)** | t50-t58 | Page cache textures, page tables, LOD height/normals, mask, material map, grass atlas |
| **Terrain (debug)** | t0-t6, t13 | Reuses low engine slots for debug quad rendering |
| **Terrain (compute)** | t50 + u0 | Input texture + UAV output |
| **Terrain (non-VT PS)** | t14, t20-t21 | Shadow arrays, EntityArray, MatrixArray (manually declared, not via macros) |
| **Grass** | t50-t51, t53 | texGrass (2DArray), texNoise, texGrassNormal (2DArray) |
| **Billboard trees** | t50-t51, t53 | texTree (2DArray), texNoise, texTreeNormal (2DArray) |
| **High-detail trees** | t51-t52 | texNoise, texTreeHigh (2DArray) |
| **High-detail branches** | t51, t54 | texNoise, texBranchesHigh (2DArray) |

### Sampler Usage

Custom shaders declare on-demand samplers at s0-s3:
```hlsl
SamplerState samplerPointWrap : register( s0 );      // or samplerBilinearWrap
SamplerState samplerTrilinearClamp : register( s1 );  // or samplerBiClamp
SamplerState samplerTrilinearWrap : register( s2 );   // or samplerBiWrap
SamplerState samplerPointClamp : register( s3 );
```

Engine persistent samplers (s4-s15) available via `globals.hlsli`.

### Vertex Input Formats

**7 distinct vertex formats** (all using input assembler with custom semantics):

| Format | Semantics | Used By |
|--------|-----------|---------|
| Terrain mesh | `POSITION`(f3), `INORMAL`(f4), `ID`(u1) | TerrainVS, TerrainPrepassVS, TerrainShadowMapVS, etc. |
| Terrain sphere | `POSITION`(f3), `NORMAL`(f3), `UV`(f2), `SV_InstanceID` | TerrainSphereVS |
| Terrain page gen | `POSITION`(f2), `UV`(f2), `HEIGHTUV`(f2), `WORLDPOS`(f2), `CHUNKID`(u1) | TerrainPageGenVS |
| Billboard tree | `POSITION`(f2), `OFFSET`(f3), `DATA`(u1) | GGTreesVS, GGTreesShadowMapVS, GGTreesPrepassVS |
| High-detail tree | `POSITION`(f3), `INORMAL`(f4), `UV`(f2), `OFFSET`(f3), `DATA`(u1) | GGTreesHighVS, GGTreeBranchesHighVS, and all shadow/prepass variants |
| Tree env probe | Same as high-detail + `INSTANCEDATA`(u1) for cubeFaceID | GGTreesHighEnvProbeVS, GGTreeBranchesHighEnvProbeVS |
| Grass | `POSITION`(f2), `OFFSET`(f3), `DATA`(u1), `SV_InstanceID` | GGGrassVS, GGGrassPrepassVS |
| Grass shadow | `POSITION`(f3), `UV`(f2), `OFFSET`(f3), `DATA`(u1) | GGGrassShadowMapVS |
| Simple quad/overlay | `POSITION`(f2) | GGTerrainQuadVS, GGTerrainOverlayVS |
| Edit box/ramp | `POSITION`(f3) | GGTerrainEditBoxVS, GGTerrainRampVS |
| Readback | `POSITION`(f3), `ID`(u1) | GGTerrainReadBackVS |

---

### Complete File Inventory with Tiers

#### Tier 1 — Simple (recompile + root signature; no PBR, minimal bindings)

| File | Type | CBs | Custom Tex | Notes |
|------|------|-----|------------|-------|
| GGTerrainShadowMapVS | VS | b1 | — | Simplest possible |
| GGTerrainPrepassRefVS | VS | b1,b2 | — | Clip distance only |
| GGTerrainQuadVS | VS | — | — | No cbuffers at all |
| GGTerrainRampVS | VS | b1,b2 | — | Editor visualization |
| GGTerrainRampPS | PS | b2 | — | Editor visualization |
| GGTerrainEditBoxVS | VS | b1,b2 | — | Editor visualization |
| GGTerrainEditBoxPS | PS | b2 | — | Editor visualization |
| GGTerrainSpherePrepassPS | PS | — | — | Returns zeros |
| GGGrassShadowMapVS | VS | b1,b2 | — | |
| GGGrassShadowMapPS | PS | b2 | — | Alpha test commented out |
| GGTreesShadowMapVS | VS | b1,b2 | — | Billboard tree shadow |
| GGTreesHighShadowMapVS | VS | b1,b2 | — | High-detail tree shadow |
| GGTreeBranchesHighShadowMapVS | VS | b1,b2 | — | Branches shadow |

**Total: 13 shaders**

#### Tier 2 — Medium (engine globals and/or custom textures, no PBR chain)

| File | Type | CBs | Custom Tex | Notes |
|------|------|-----|------------|-------|
| GGTerrainVS | VS | b1,b2 | — | Uses GGTerrainPageSettings.h |
| GGTerrainPrepassVS | VS | b1,b2 | — | |
| GGTerrainEnvProbeVS | VS | b2,b8 | — | Uses xCubemapRenderCams |
| GGTerrainOverlayVS | VS | b0(partial) | — | Re-declares partial FrameCB |
| GGTerrainPageGenVS | VS | b2 | — | |
| GGTerrainReadBackVS | VS | b0(custom!) | — | Own viewProj at b0 |
| GGTerrainSphereVS | VS | b1 | — | Instanced spheres |
| GGTerrainReadBackCS | CS | b2 | t50,u0 | Compute shader |
| GGTerrainReadBackMSCS | CS | b2 | t50(MS),u0 | Compute, multisample |
| GGTerrainPrepassPS | PS | b2 | t53,t54 | VT readback |
| GGTerrainReadBackPS | PS | — | t3 | |
| GGTerrainOverlayPS | PS | b2 | t0-t2 | Includes globals.hlsli |
| GGTerrainQuadPS | PS | b2 | t0-t6,t13 | Debug visualization |
| GGGrassVS | VS | b1,b2 | — | Includes globals.hlsli |
| GGGrassPrepassVS | VS | b1,b2 | — | |
| GGGrassPrepassPS | PS | b2 | t50-t51 | Alpha test + LOD fade |
| GGTreesVS | VS | b1,b2 | — | Billboard trees |
| GGTreesPrepassVS | VS | b1,b2 | — | |
| GGTreesHighVS | VS | b1,b2 | — | |
| GGTreesHighPrepassVS | VS | b1,b2 | — | |
| GGTreeBranchesHighVS | VS | b1,b2 | — | |
| GGTreeBranchesHighPrepassVS | VS | b1,b2 | — | |
| GGTreesShadowMapPS | PS | b2 | t50-t51 | Alpha test + LOD fade |
| GGTreesHighShadowMapPS | PS | b2 | t51 | LOD fade only |
| GGTreeBranchesHighShadowMapPS | PS | b2 | t51,t54 | Alpha test + LOD fade |
| GGTreesPrepassPS | PS | b2 | t50-t51 | |
| GGTreesHighPrepassPS | PS | b2 | t51 | |
| GGTreeBranchesHighPrepassPS | PS | b2 | t51,t54 | |
| GGTreesHighEnvProbeVS | VS | b2,b8 | — | Uses xCubemapRenderCams |
| GGTreeBranchesHighEnvProbeVS | VS | b2,b8 | — | Uses xCubemapRenderCams |

**Total: 30 shaders**

#### Tier 3 — Complex (full PBR lighting: brdf + lightingHF + sky + voxelConeTracing)

| File | Type | CBs | Custom Tex | Lighting | Notes |
|------|------|-----|------------|----------|-------|
| GGTerrainPS | PS | b2 | t40-t47,t14,t20-t21 | Custom shadow | Local ShaderEntity copy, own cascade code |
| GGTerrainPageGenPS | PS | b2 | t50-t58 | None | Complex multi-layer material blending |
| GGTerrainVirtualPS | PS | b2 | t50-t54 | Tiled | VT + tiled lighting |
| GGTerrainVirtualPBR_PS | PS | b2 | t50-t54 | Tiled | Most complete: 424 lines, VT+tiled+fog+editor |
| GGTerrainEnvProbePS | PS | b2,b7 | t50-t54 | Forward | Env probe rendering |
| GGTerrainSpherePS | PS | b2 | t50-t52 | Tiled | Material preview spheres |
| GGGrassPS | PS | b2 | t50-t51,t53 | Tiled | Grass + LOD fade + custom fog |
| GGTreesPS | PS | b2 | t50-t51,t53 | Tiled | Billboard trees (218 lines) |
| GGTreesHighPS | PS | b2 | t51-t52 | Tiled | High-detail trees (272 lines) |
| GGTreeBranchesHighPS | PS | b2 | t51,t54 | Tiled | Branches + alpha test (277 lines) |
| GGTreesHighEnvProbePS | PS | b2,b7 | t52 | Forward | Trees env probe |
| GGTreeBranchesHighEnvProbePS | PS | b2,b7 | t54 | Forward | Branches env probe + alpha |

**Total: 12 shaders**

**Grand total: 55 shader files + 7 header files = 62 files**

### Lighting Variants (Duplicated Code)

Two lighting approaches are copy-pasted into each Tier 3 shader (not shared via include):

1. **TiledLighting** — main render shaders. Reads `EntityTiles` via `STRUCTUREDBUFFER(EntityTiles, uint, TEXSLOT_RENDERPATH_ENTITYTILES)`. Iterates entity buckets with `WaveReadLaneFirst`/`WaveActiveBitOr` (note: these are no-ops via `#define DISABLE_WAVE_INTRINSICS` in ShaderInterop.h).

2. **ForwardLighting** — env probe shaders. Uses `xForwardLightMask` from `ForwardEntityMaskCB` (b7). Simpler loop.

### Key Quirks

1. **`GGTerrainPS.hlsl`** — re-declares `ShaderEntity` as a local struct (copy-pasted from `ShaderInterop_Renderer.h`) and manually binds `EntityArray : register(t20)` and `MatrixArray : register(t21)` instead of using the `STRUCTUREDBUFFER` macro.
2. **`GGTerrainReadBackVS.hlsl`** — declares `cbuffer constants : register(b0)` with just a `float4x4 viewProj`, conflicting with `FrameCB` at `b0`. Used in an isolated render pass.
3. **`GGTerrainOverlayVS.hlsl`** — re-declares a partial `FrameCB` at `b0` containing only the resolution fields.
4. **`GGTreesConstants.hlsli`** — includes `PBR/ShaderInterop_Renderer.h` inside a `#ifndef WI_SHADERINTEROP_RENDERER_H` guard, and uses `g_xFrame_TreeWind` and `g_xFrame_Time` from `FrameCB` in its wind animation functions.
5. **`DISABLE_WAVE_INTRINSICS`** — defined in `ShaderInterop.h`, makes `WaveReadLaneFirst(a)` → `(a)` and `WaveActiveBitOr(a)` → `(a)`. All TiledLighting code uses these wave intrinsics but they're currently disabled.

### Include Dependency Chain (deepest path)

```
GGTerrainVirtualPBR_PS.hlsl
├── GGTerrainConstants.hlsli (TerrainCB at b2)
├── ../GGTerrainPageSettings.h (virtual texture constants)
├── PBR/brdf.hlsli
│   └── PBR/globals.hlsli
│       ├── PBR/ShaderInterop.h
│       │   ├── PBR/ConstantBufferMapping.h
│       │   ├── PBR/SamplerMapping.h
│       │   └── PBR/ResourceMapping.h
│       └── PBR/ShaderInterop_Renderer.h
├── PBR/lightingHF.hlsli
│   ├── PBR/voxelConeTracingHF.hlsli
│   └── PBR/skyHF.hlsli → PBR/skyAtmosphere.hlsli
└── GGCommonFunctions.hlsli (ApplyFogCustom)
```

---

## Your Task

Using the complete inventory above, investigate the **engine C++ code** and the **DX12 backend**, then produce a detailed porting plan.

### Step 1: Examine the C++ Integration

Find the engine C++ code that manages these custom shaders:

1. **Shader compilation/loading** — search for `.cso`, `LoadShader`, `CreateShader`, `fxc`. How are the 62 shaders compiled? Build step or runtime?
2. **Pipeline state creation** — search for `CreatePipelineState`, input layout setup referencing custom semantics (`INORMAL`, `OFFSET`, `DATA`, `ID`, `CHUNKID`, `WORLDPOS`, `HEIGHTUV`, `INSTANCEDATA`)
3. **Constant buffer binding** — search for `BindConstantBuffer` with slot 2 (GrassCB/TerrainCB/TreeCB)
4. **Texture binding** — search for `BindResource` with slots 40-58
5. **Draw calls** — find the render passes using these shaders
6. **Existing DX12 work** — search for `HLSL6`, `dxc`, `dxcompiler`, SM 6.0 references

### Step 2: Examine the DX12 Backend

Answer these critical questions by reading the code:

1. **Root signature generation** — examine `wiGraphicsDevice_DX12.cpp` around line 3927. Does it extract root signatures from bytecode only, or can it auto-generate them?
2. **Root signature layout** — what register ranges (b, t, u, s) does it cover? Do t40-t58 and b2 fit?
3. **Input layout support** — does the DX12 backend support `D3D12_INPUT_ELEMENT_DESC` / input assembler, or only manual vertex fetching?
4. **Shader compilation** — what target profile and DXC flags does `wi::shadercompiler` use?
5. **Custom shader registration** — how does GameGuru MAX load its non-built-in shaders?

### Step 3: Answer the Critical Architecture Questions

Document the answer to each before writing the plan:

1. **Can slot-based binding survive?** If the DX12 root signature covers b0-b10, t0-t63, s0-s15, u0-u7, then recompiling with DXC may be sufficient for most shaders.
2. **Can input assembler vertex input survive?** The 7 custom vertex formats use non-standard semantics.
3. **Do custom texture slots t40-t58 fit in the root signature?** ResourceMapping.h says TEXSLOT_COUNT=64.
4. **Any sampler conflicts at s0-s3?**
5. **Is the GGTerrainReadBackVS b0 conflict safe** in an isolated render pass?
6. **Root signature strategy:** single shared root signature covering all slots, per-shader root signatures, or use the engine's existing one?

### Step 4: Write the Plan

#### Section 1: Executive Summary
- 13 Tier 1 + 30 Tier 2 + 12 Tier 3 = 55 shaders across 3 systems + 7 headers
- High-level approach, estimated effort, key risks

#### Section 2: Architecture Comparison
- DX11 vs DX12 for: compilation, binding, vertex input, root signatures
- Answers to the 6 critical questions

#### Section 3: Shared Infrastructure (do first)
- Compilation pipeline (fxc→dxc, SM 5.0→6.0+)
- Root signature strategy
- Changes to ShaderInterop.h / mapping headers
- C++ infrastructure changes
- Compatibility bridge approach

#### Section 4: Per-System Porting Guide

**4.1 Terrain System (28 files: 8 Tier 1, 13 Tier 2, 7 Tier 3)**
- Special cases: GGTerrainReadBackVS (b0 conflict), GGTerrainOverlayVS (partial FrameCB), compute shaders, GGTerrainPS (local ShaderEntity)

**4.2 Grass System (8 files: 2 Tier 1, 3 Tier 2, 1 Tier 3 + 2 headers)**

**4.3 Tree System (23 files: 3 Tier 1, 14 Tier 2, 4 Tier 3 + 2 headers)**
- Three rendering variants: billboard (GGTrees*), high-detail trunk (GGTreesHigh*), high-detail branches (GGTreeBranchesHigh*)
- Each has VS/PS/ShadowMapVS/ShadowMapPS/PrepassVS/PrepassPS + env probe VS/PS
- Two vertex formats: billboard (f2 position) vs high-detail (f3 position + INORMAL + UV)
- Note: GGTreesConstants.hlsli auto-includes ShaderInterop_Renderer.h and uses FrameCB globals for wind

**4.4 Shared Headers**
- PBR/ headers: any drift from latest Wicked Engine versions?
- GGCommonFunctions.hlsli, GGTerrainPageSettings.h

#### Section 5: Compilation & Build
- DXC flags per shader type
- Include path setup (-I for PBR/, parent dir for GGTerrainPageSettings.h)
- Root signature embedding
- `#define HLSL6` activation

#### Section 6: Testing Milestones
1. Tier 1 terrain shadow → validates compilation pipeline
2. Terrain renders (GGTerrainPS) → validates vertex input + cbuffers
3. Virtual texture terrain (GGTerrainVirtualPBR_PS) → validates complex PBR
4. Grass renders → validates instancing + LOD
5. Billboard trees render → validates tree system
6. High-detail trees + branches render → validates all tree variants
7. Env probes render → validates forward lighting + cubemap

#### Section 7: Porting Order
1. Shared infrastructure
2. Tier 1 terrain (quickest validation)
3. Tier 2 terrain → Tier 3 terrain
4. Grass (all tiers)
5. Trees (all tiers, all 3 variants)
- Dependency graph

## Output

Save as `DX11_to_DX12_Shader_Porting_Plan.md` in the project root.

## Critical Reminders

- Be **specific** — actual filenames, line numbers, function names, register slots, C++ class names.
- **Tier 1 shaders are the quick win** — if recompiling with DXC + compatible root signature works, that's 13 shaders done immediately.
- If the DX12 root signature is **fundamentally incompatible** with slot-based binding, say so immediately.
- Note that **GGTerrainPS.hlsl has a local ShaderEntity copy** — should it be replaced with an include?
- Note that **TiledLighting/ForwardLighting are duplicated** across 12 Tier 3 shaders — recommend factoring into shared headers?
- Note that **DISABLE_WAVE_INTRINSICS** makes wave ops no-ops — should this be removed for DX12 SM 6.0+ where wave intrinsics are natively supported?
- The plan must be **actionable, unambiguous, and ordered by dependency**.
