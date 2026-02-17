# DX11 to DX12 Shader Porting Plan

## GameGuruMAX Custom Terrain/Vegetation Shaders

**Status:** Phase 1 COMPLETE (Compilation) | Phase 2-4 TODO (Runtime Correctness)
**Date:** 2026-02-17
**Scope:** 66 HLSL shaders + 15 header files in `Guru-WickedMAX/GGTerrain/Shaders/`

### Completion Summary

**Phase 1 (Compilation Infrastructure) - COMPLETE:**
- All 66+ shaders compile with DXC SM 6.0 (was fxc SM 5.0)
- All .cso files contain RTS0 root signature chunks
- Both Debug and Release configurations build and link successfully
- WickedEngine lib rebuilt with SRV expansion (16→64)
- Sampler slots migrated (s4-s13 → s100-s109)
- GGRootSignature.hlsli created and included in all shaders
- GGLighting.hlsli factored from 6+ pixel shaders
- GGBindless.hlsli / GGFrameCompat.hlsli compatibility headers created (not yet integrated)

**Phase 2-4 (Runtime Correctness) - NOT STARTED:**
- Old PBR headers still declare old FrameCB layout (~896 bytes) but C++ fills new layout (~35 KB)
- CameraCB layout also mismatched (old flat fields vs new ShaderCamera[16] array)
- Old PBR functions use old API names (DirectionalLight vs light_directional)
- ShaderEntity struct changed (48→80 bytes)
- Entity iteration, shadow atlas, half-precision packing all different
- Custom GameGuru fields (TreeWind, WaterColor, etc.) need C++ binding

**Impact:** Shaders will LOAD in DX12 (root signature issue fixed) but produce incorrect visual results due to FrameCB/CameraCB binary layout mismatches. This is a step forward from "crash on load" to "loads but renders incorrectly."

---

## 1. Executive Summary

GameGuruMAX uses ~57 custom HLSL shaders for terrain, grass, and tree rendering that sit alongside the Wicked Engine's built-in shader pipeline. These shaders currently compile with **fxc.exe at Shader Model 5.0**, producing DXBC bytecode. Under DX12, the engine requires each shader to embed a **root signature (RTS0 chunk)** in the compiled binary. DXBC produced by fxc.exe does not contain this chunk, causing `D3D12CreateVersionedRootSignatureDeserializer` to fail with `E_INVALIDARG` at load time.

### Root Cause
SM 5.0 DXBC bytecode lacks the RTS0 root signature chunk that DX12 requires. The fix is to recompile all shaders with **DXC at Shader Model 6.0** with an embedded root signature definition.

### Scale
| Category | Files |
|----------|-------|
| Terrain shaders (.hlsl) | 30 (including 2 test shaders) |
| Grass shaders (.hlsl) | 6 |
| Tree shaders (.hlsl) | 22 |
| Custom headers (.hlsli) | 4 |
| PBR headers (.hlsli) | 6 |
| PBR interop headers (.h) | 5 |
| **Total** | **~73 files** |

### Tier Breakdown (Corrected)

| Tier | Count | Description | HLSL Changes |
|------|-------|-------------|-------------|
| Tier 1 | 10 | Shadow map / simple shaders, no PBR includes | Zero (compile-only) |
| Tier 2 | 33 | Include PBR headers; texture slots t16-t63 work via C2 | Minimal (header updates propagate) |
| Tier 3 | 12 | Full PBR lighting pipeline (brdf + lightingHF + entity access) | Heavy (API migration) |
| Dead code | 1 | GGTerrainReadBackVS (no LoadShader reference) | Skip |
| Test | 2 | terraintestPS/VS (not shipped) | N/A |

> **Note:** The original plan had 11 "Tier 2B" shaders requiring bindless texture conversion. With the C2 decision (SRV range expanded to 64), these are merged into Tier 2 — their `register(t50)` etc. declarations work directly through the expanded descriptor table. Tier 2B no longer exists as a separate category.

### Key Decisions
- **Texture binding strategy:** C2 (expand SRV range to 64) -- `DESCRIPTORBINDER_SRV_COUNT` changed from 16 to 64 in `wiGraphicsDevice.h`, root signature updated to `SRV(t0, numDescriptors=64)`. Existing `register(t50)` etc. declarations stay. C1 (bindless via CBs) documented as future optimization. This eliminates Tier 2B as a separate category — all t16-t63 slots now work through the normal descriptor table.
- **b2 CB slot conflict:** Safe -- on-demand slot, never simultaneously bound
- **Compilation:** DXC at SM 6.0 with embedded root signature via `[RootSignature()]` attribute or `-rootsig-define` flag

---

## 2. Architecture Comparison

### 2.1 DX11 Slot-Based Model (Current)

Shaders declare explicit register bindings and the engine binds resources to numbered slots:

```hlsl
Texture2D texColor : register( t40 );       // Slot-based, any number
cbuffer TerrainCB : register( b2 ) { ... }  // CB at slot 2
SamplerState sampler1 : register( s1 );      // Sampler at slot 1
```

The DX11 runtime allows arbitrary register numbers (t0-t127, s0-s15, b0-b13). Shaders are compiled with fxc.exe to SM 5.0 DXBC. No root signature required.

### 2.2 DX12 Hybrid Model (Target)

The Wicked Engine DX12 backend uses a **fixed root signature** with bounded descriptor tables for space0 and unbounded bindless tables for higher spaces:

```
Root Param 0: RootConstants(num32BitConstants=12, b999)    -- Push constants
Root Param 1: CBV(b0)                                      -- FrameCB
Root Param 2: CBV(b1)                                      -- CameraCB
Root Param 3: CBV(b2)                                      -- On-demand (TerrainCB/GrassCB/TreeCB)
Root Param 4: DescriptorTable(CBV b3-b13, SRV t0-t15, UAV u0-u15)
Root Param 5: DescriptorTable(Sampler s0-s7)
Root Param 6: DescriptorTable(Sampler space1, unbounded)   -- Bindless samplers
Root Param 7: DescriptorTable(SRV space2-30, UAV space100-115, SRV space200-208) -- Bindless resources
Static Samplers: s100-s109
```

**Engine hard limits** (from `wiGraphicsDevice_DX12.cpp`):
- `DESCRIPTORBINDER_CBV_COUNT = 14` (b0-b13)
- `DESCRIPTORBINDER_SRV_COUNT = 64` (t0-t63, expanded from 16 via C2 decision)
- `DESCRIPTORBINDER_UAV_COUNT = 16` (u0-u15)
- `DESCRIPTORBINDER_SAMPLER_COUNT = 8` (s0-s7)

### 2.3 Texture Binding: Slots vs Bindless

| Aspect | DX11 (Current) | DX12 (Target) |
|--------|----------------|---------------|
| SRV range | t0-t127 (any slot) | t0-t15 only in descriptor table |
| Textures above t15 | Direct slot binding | Must use bindless: `bindless_textures[descriptor_index]` |
| Descriptor index source | N/A | Custom CB field (e.g., `terrain_texColorIndex`) |
| Sampler range | s0-s15 | s0-s7 (descriptor table) + s100-s109 (static) |

**Update (C2 decision):** With `DESCRIPTORBINDER_SRV_COUNT` expanded to 64, shaders using `register(t16)` through `register(t63)` work directly through the descriptor table. No bindless conversion required. This eliminated the original Tier 2B category.

### 2.4 Sampler Migration

| DX11 Register | DX12 Register | Type |
|--------------|---------------|------|
| s0 | s0 | In descriptor table (s0-s7) |
| s1 | s1 | In descriptor table |
| s4-s13 | s100-s109 | Static samplers |
| s7 | s7 | In descriptor table |

The engine's new globals.hlsli defines static samplers at s100-s109. Shaders referencing old sampler registers s4-s13 must update to s100-s109.

### 2.5 Constant Buffer Slot Changes

| Slot | DX11 Usage | DX12 Usage | Conflict? |
|------|-----------|------------|-----------|
| b0 | FrameCB | FrameCB | No |
| b1 | CameraCB | CameraCB | No |
| b2 | TerrainCB / GrassCB / TreeCB | On-demand (same) | **No** -- see below |
| b2 | ForwardEntityMaskCB (was b7) | ForwardEntityMaskCB | **Safe** -- different passes |
| b3-b13 | Various engine CBs | In descriptor table | No |

**b2 Conflict Resolution:** ForwardEntityMaskCB moved from b7 to b2 in the latest engine. This is safe because b2 is an "on-demand" slot: TerrainCB/GrassCB/TreeCB are bound during terrain/vegetation passes, while ForwardEntityMaskCB is bound during forward-lighting passes. They are never simultaneously bound.

**Exception:** Environment probe shaders (`GGTerrainEnvProbePS`, `GGTreesHighEnvProbePS`, `GGTreeBranchesHighEnvProbePS`) previously referenced ForwardEntityMaskCB at b7. These must be updated to reference b2. Since env probe passes do not bind terrain CBs, the b2 slot is available for ForwardEntityMaskCB.

### 2.6 Custom FrameCB Fields (Must Preserve)

These fields exist in the old FrameCB but NOT in the new engine's FrameCB:

| Field | Used By | Proposed Location |
|-------|---------|-------------------|
| `g_xFrame_TreeWind` | GGTreesConstants.hlsli (TreeWaveX/Z) | TreeCB or push constants |
| `g_xFrame_TreeSubSurfaceScattering` | Tree PS shaders | TreeCB |
| `g_xFrame_WaterColor` | Terrain/env PS shaders | TerrainCB |
| `g_xFrame_WaterHeight` | Terrain PS shaders | TerrainCB |
| `g_xFrame_WaterFogMin` | Terrain PS shaders | TerrainCB |
| `g_xFrame_WaterFogMax` | Terrain PS shaders | TerrainCB |
| `g_xFrame_WaterFogMinAmount` | Terrain PS shaders | TerrainCB |
| `g_xFrame_DeSaturate` | Post-process related | TerrainCB or push constants |
| `g_xFrame_FogColor` | Fog rendering | GGFrameCompat.hlsli shim |
| `g_xFrame_FogOpacity` | Fog rendering | GGFrameCompat.hlsli shim |
| `g_xFrame_Voxel_Steps` | GI voxel tracing | GGFrameCompat.hlsli shim |
| `g_xFrame_ObjectShaderSamplerIndex` | Material sampling | GGFrameCompat.hlsli shim |

**Resolution:** Create `GGFrameCompat.hlsli` that provides these as macros/functions sourced from custom CB fields or push constants. Shaders include this header and existing code compiles without changes.

---

## 3. Shared Infrastructure (Do First)

All Tier 2+ shaders depend on shared PBR headers and utility code. This infrastructure must be updated before any shader that includes PBR headers can compile.

### 3.1 PBR Header Update (Critical Path)

The `PBR/` subdirectory contains copies of Wicked Engine headers that must be updated to match the DX12 engine version. This is ~4600 lines of combined header code.

#### Per-File Update Plan

| File | Lines | Changes Required |
|------|-------|------------------|
| `PBR/ShaderInterop.h` | ~600 | Massive restructuring. `ConstantBufferMapping.h`, `SamplerMapping.h`, `ResourceMapping.h` are ELIMINATED. All definitions inlined. |
| `PBR/ShaderInterop_Renderer.h` | ~1200 | FrameCB and CameraCB completely restructured. ShaderEntity 48->80 bytes, color RGBA8->half4. EntityArray/MatrixArray now inline in FrameCB. |
| `PBR/globals.hlsli` | ~800 | Completely rewritten for bindless. Defines `WICKED_ENGINE_DEFAULT_ROOTSIGNATURE`. Samplers s4-s13 become static s100-s109. Adds bindless resource accessors. |
| `PBR/brdf.hlsli` | ~700 | Surface API changes. Material access via getter functions instead of direct struct members. |
| `PBR/lightingHF.hlsli` | ~800 | Entity iteration via `ShaderEntityIterator`. New shadow atlas API. Cascade shadow changes. |
| `PBR/skyAtmosphere.hlsli` | ~200 | Sky luminance LUT access changes. |
| `PBR/skyHF.hlsli` | ~150 | Minor API updates. |
| `PBR/voxelConeTracingHF.hlsli` | ~200 | Voxel GI parameter changes. |
| `PBR/ConstantBufferMapping.h` | ~50 | **DELETE** -- contents merged into ShaderInterop.h |
| `PBR/SamplerMapping.h` | ~30 | **DELETE** -- contents merged into globals.hlsli |
| `PBR/ResourceMapping.h` | ~50 | **DELETE** -- contents merged into ShaderInterop.h |

#### GGFrameCompat.hlsli (New File)

Create `GGFrameCompat.hlsli` to bridge removed FrameCB fields:

```hlsl
// GGFrameCompat.hlsli -- Compatibility shim for custom FrameCB fields
// These fields were in the old Wicked FrameCB but removed in DX12 version.
// They are now sourced from custom CBs (TerrainCB, TreeCB) or push constants.

#ifndef GG_FRAME_COMPAT_HLSLI
#define GG_FRAME_COMPAT_HLSLI

// Tree-related (sourced from TreeCB when available, else push constants)
#ifdef GG_HAS_TREE_CB
    #define g_xFrame_TreeWind         tree_wind
    #define g_xFrame_TreeSubSurfaceScattering tree_subsurfaceScattering
#endif

// Water/fog (sourced from TerrainCB extensions)
#ifdef GG_HAS_TERRAIN_CB
    #define g_xFrame_WaterColor       terrain_waterColor
    #define g_xFrame_WaterHeight      terrain_waterHeight
    // ... etc
#endif

// General (sourced from push constants b999)
#define g_xFrame_FogColor         GetFogColor()
#define g_xFrame_FogOpacity       GetFogOpacity()

#endif
```

### 3.2 GGBindless.hlsli (New File)

Helper macros for bindless texture access, abstracting the pattern:

```hlsl
// GGBindless.hlsli -- Bindless texture access helpers
#ifndef GG_BINDLESS_HLSLI
#define GG_BINDLESS_HLSLI

#include "PBR/globals.hlsli"

// Sample a 2D texture by descriptor index from a custom CB field
#define GG_SAMPLE_TEXTURE2D(cb_index, sampler, uv) \
    bindless_textures[descriptor_index(cb_index)].Sample(sampler, uv)

#define GG_SAMPLE_TEXTURE2D_LOD(cb_index, sampler, uv, lod) \
    bindless_textures[descriptor_index(cb_index)].SampleLevel(sampler, uv, lod)

// Sample a 2D texture array by descriptor index
#define GG_SAMPLE_TEXTURE2DARRAY(cb_index, sampler, uvw) \
    bindless_textures2DArray[descriptor_index(cb_index)].Sample(sampler, uvw)

// Load from a typed buffer by descriptor index
#define GG_LOAD_TEXTURE2D(cb_index, coord) \
    bindless_textures[descriptor_index(cb_index)].Load(coord)

#endif
```

### 3.3 GGLighting.hlsli (New File)

Factor duplicated PBR lighting code from terrain and vegetation pixel shaders. Currently, GGTerrainPS.hlsl, GGTerrainVirtualPS.hlsl, GGTerrainVirtualPBR_PS.hlsl, GGTerrainSpherePS.hlsl, GGTerrainEnvProbePS.hlsl, GGTreesPS.hlsl, GGTreesHighPS.hlsl, GGTreeBranchesHighPS.hlsl, and GGGrassPS.hlsl all contain near-identical PBR lighting loops. Factoring reduces per-shader porting burden.

Key contents:
- Light iteration using new `ShaderEntityIterator` API
- Shadow sampling using new atlas API
- Fog application using compatibility shim
- Environment probe sampling

### 3.4 GGCommonFunctions.hlsli Updates

Current file (`GGCommonFunctions.hlsli`) contains shared utility functions used by terrain and vegetation shaders. Updates needed:
- Replace any references to removed FrameCB fields with compatibility shim calls
- Update sampler register references if any use s4-s13
- Ensure compatibility with new globals.hlsli types

### 3.5 Custom Constants Headers Updates

| File | Changes |
|------|---------|
| `GGTerrainConstants.hlsli` | Add descriptor index fields to TerrainCB for bindless textures. Add water/fog fields from FrameCB compat. |
| `GGGrassConstants.hlsli` | Add descriptor index fields to GrassCB for grass texture array + noise. |
| `GGTreesConstants.hlsli` | Add descriptor index fields to TreeCB for tree textures. Add tree wind/SSS fields from FrameCB compat. Update ForwardEntityMaskCB reference from b7 to b2 in env probe path. |

**Example TerrainCB extension:**
```hlsl
cbuffer TerrainCB : register( b2 )
{
    // ... existing fields ...

    // DX12 bindless descriptor indices (new)
    int terrain_texColorIndex;
    int terrain_texNormalIndex;
    int terrain_texRockColorIndex;
    int terrain_texRockNormalIndex;
    int terrain_texSnowColorIndex;
    int terrain_texSnowNormalIndex;
    int terrain_texSandColorIndex;
    int terrain_texSandNormalIndex;
    int terrain_texSurfaceIndex;
    int terrain_texPageTableIndex;
    int terrain_texPageCacheIndex;
    // ... water/fog compat fields ...
};
```

### 3.6 Compilation Pipeline (fxc to DXC, SM 5.0 to 6.0)

See Section 5 for full details. Summary: switch from `fxc.exe /T vs_5_0` to `dxc.exe /T vs_6_0` with `-rootsig-define` or `[RootSignature()]` attribute.

---

## 4. Per-System Porting Guide

All paths relative to `Guru-WickedMAX/GGTerrain/Shaders/`.

### 4.1 Terrain Shaders (30 files)

#### Tier 1 -- Zero HLSL Changes (8 shaders)

These shaders use only custom CBs (TerrainCB at b2) and/or CameraCB (b1), with no PBR headers. They compile with DXC + root signature embedding and zero source changes.

| Shader | Type | Registers Used | Notes |
|--------|------|---------------|-------|
| `GGTerrainShadowMapVS.hlsl` | VS | b1 (CameraCB) | Simplest shader. Position transform only. |
| `GGTerrainEditBoxVS.hlsl` | VS | b2 (TerrainCB) | Edit box vertex transform. |
| `GGTerrainEditBoxPS.hlsl` | PS | b2 (TerrainCB) | Edit box pixel output. |
| `GGTerrainPageGenVS.hlsl` | VS | b2 (TerrainCB) | Page table generation vertex. |
| `GGTerrainPageGenPS.hlsl` | PS | b2 (TerrainCB) | Page table generation pixel. |
| `GGTerrainPrepassVS.hlsl` | VS | b2 (TerrainCB) | Depth prepass vertex. |
| `GGTerrainPrepassRefVS.hlsl` | VS | b2 (TerrainCB) | Reflection prepass vertex. |
| `GGTerrainRampVS.hlsl` | VS | b2 (TerrainCB) | Ramp vertex transform. |

#### Tier 2A -- PBR Header Dependency Only (10 shaders)

These include `PBR/globals.hlsli` or `PBR/ShaderInterop_Renderer.h` but do not use PBR lighting. After PBR headers are updated (Section 3.1), they compile with minimal or zero additional changes.

| Shader | Type | PBR Include | Textures | Changes Beyond Headers |
|--------|------|------------|----------|----------------------|
| `GGTerrainOverlayVS.hlsl` | VS | b0 (custom FrameCB subset) | None | Inline FrameCB fields -- needs compat for `g_xFrame_CanvasSize` etc. |
| `GGTerrainOverlayPS.hlsl` | PS | `PBR/globals.hlsli` | t0-t5 (within limit) | Sampler updates if any use s4-s13. |
| `GGTerrainVS.hlsl` | VS | b2 (TerrainCB) | None | Includes GGTerrainPageSettings.h. |
| `GGTerrainEnvProbeVS.hlsl` | VS | `PBR/ShaderInterop_Renderer.h` | None | FrameCB/CameraCB struct changes propagate from headers. |
| `GGTerrainPrepassPS.hlsl` | PS | b2 (TerrainCB) | t0-t5 (within limit) | GGTerrainPageSettings.h dependency. |
| `GGTerrainSphereVS.hlsl` | VS | b2 (TerrainCB) | None | Position transform. |
| `GGTerrainSpherePrepassPS.hlsl` | PS | b2 (TerrainCB) | t0-t3 (within limit) | Depth prepass for sphere terrain. |
| `GGTerrainRampPS.hlsl` | PS | b2 (TerrainCB) | t0-t3 (within limit) | Ramp pixel shader. |
| `GGTerrainReadBackVS.hlsl` | VS | None (standalone) | None | Simple vertex. |
| `GGTerrainReadBackPS.hlsl` | PS | None | t0-t5 (within limit) | Includes GGTerrainPageSettings.h only. |

#### Tier 2B -- Bindless Texture Conversion (4 terrain shaders)

These use texture registers above t15 but do NOT use PBR lighting. Texture declarations must convert from slot-based to bindless via CB descriptor indices.

| Shader | Type | High Registers | Bindless Conversion |
|--------|------|---------------|---------------------|
| `GGTerrainQuadPS.hlsl` | PS | t0-t6, t13 (all within t0-t15) | Actually within limit -- **may reclassify to Tier 2A** after verification |
| `GGTerrainQuadVS.hlsl` | VS | b2 (TerrainCB) | Simple vertex, may be Tier 2A |
| `GGTerrainReadBackCS.hlsl` | CS | b2, t0-t3 | Compute shader, UAV access patterns |
| `GGTerrainReadBackMSCS.hlsl` | CS | b2, t0-t1 | Compute shader variant |

#### Tier 3 -- Full PBR Migration (6 terrain shaders)

These include `PBR/brdf.hlsli` and `PBR/lightingHF.hlsli`, use full PBR lighting loops, AND use high texture registers. They require all infrastructure (headers, bindless, lighting refactor).

| Shader | Type | PBR Includes | High Textures | Key Changes |
|--------|------|-------------|---------------|-------------|
| `GGTerrainPS.hlsl` | PS | brdf + lightingHF | t40-t47 (color/normal for terrain materials) | ShaderEntity struct 48->80 bytes, bindless for t40+, entity iteration API, shadow atlas |
| `GGTerrainVirtualPS.hlsl` | PS | brdf + lightingHF | t40-t47 | Same as GGTerrainPS, virtual texture variant |
| `GGTerrainVirtualPBR_PS.hlsl` | PS | brdf + lightingHF | t40-t47 | Full PBR variant of virtual terrain |
| `GGTerrainSpherePS.hlsl` | PS | brdf + lightingHF | t40-t47 | Sphere terrain with full lighting |
| `GGTerrainEnvProbePS.hlsl` | PS | brdf + lightingHF | t40-t47 | Env probe capture. ForwardEntityMaskCB b7->b2 |
| `terraintestPS.hlsl` | PS | (test shader) | Varies | Test shader, low priority |
| `terraintestVS.hlsl` | VS | (test shader) | None | Test shader, low priority |

### 4.2 Grass Shaders (6 files)

#### Tier 1 -- Zero HLSL Changes (2 shaders)

| Shader | Type | Registers | Notes |
|--------|------|-----------|-------|
| `GGGrassShadowMapVS.hlsl` | VS | b2 (GrassCB) | Grass shadow map vertex. No PBR includes. |
| `GGGrassShadowMapPS.hlsl` | PS | b2 (GrassCB), t50, t51 | **t50/t51 are UNUSED** (texture sampling is commented out). Recommend removing/commenting out declarations for clean compile. Otherwise Tier 1. |

#### Tier 2A -- PBR Header Dependency (2 shaders)

| Shader | Type | PBR Include | Textures | Notes |
|--------|------|------------|----------|-------|
| `GGGrassPrepassVS.hlsl` | VS | `PBR/globals.hlsli` | None | Vertex transform with FrameCB. |
| `GGGrassPrepassPS.hlsl` | PS | `PBR/ShaderInterop_Renderer.h` | t50 (grass array) | Alpha test. t50 needs bindless. **Reclassify to Tier 2B.** |

#### Tier 2B -- Bindless Conversion (1 shader)

| Shader | Type | High Textures | Conversion |
|--------|------|--------------|------------|
| `GGGrassPrepassPS.hlsl` | PS | t50 (grass texture array) | `texGrass` -> bindless via GrassCB descriptor index |

#### Tier 3 -- Full PBR Migration (2 shaders)

| Shader | Type | PBR Includes | High Textures | Key Changes |
|--------|------|-------------|---------------|-------------|
| `GGGrassVS.hlsl` | VS | `PBR/globals.hlsli` | None | FrameCB field compat, wind animation |
| `GGGrassPS.hlsl` | PS | brdf + lightingHF | t50, t51 | Full PBR lighting, bindless for t50-t51, entity iteration |

### 4.3 Tree Shaders (22 files)

#### Tier 2A -- PBR Header Dependency (13 shaders)

All tree shaders include at least `PBR/globals.hlsli` or `PBR/ShaderInterop_Renderer.h` via `GGTreesConstants.hlsli`. After PBR header updates, these compile with zero additional changes.

**Shadow Map Shaders (reclassified from Tier 1 -- include PBR via GGTreesConstants.hlsli):**

| Shader | Type | PBR Include (via) | Notes |
|--------|------|-------------------|-------|
| `GGTreesShadowMapVS.hlsl` | VS | globals.hlsli | Wind animation needs `g_xFrame_TreeWind` compat |
| `GGTreesHighShadowMapVS.hlsl` | VS | globals.hlsli | Same wind dependency |
| `GGTreeBranchesHighShadowMapVS.hlsl` | VS | globals.hlsli | **Byte-for-byte identical to GGTreesHighShadowMapVS** |
| `GGTreesShadowMapPS.hlsl` | PS | ShaderInterop_Renderer.h | Alpha test, textures within t0-t15 |
| `GGTreesHighShadowMapPS.hlsl` | PS | ShaderInterop_Renderer.h | Alpha test variant |
| `GGTreeBranchesHighShadowMapPS.hlsl` | PS | ShaderInterop_Renderer.h | Branch alpha test |

**Prepass Shaders:**

| Shader | Type | PBR Include (via) | Notes |
|--------|------|-------------------|-------|
| `GGTreesPrepassVS.hlsl` | VS | globals.hlsli | Vertex transform |
| `GGTreesPrepassPS.hlsl` | PS | ShaderInterop_Renderer.h | Alpha test |
| `GGTreesHighPrepassVS.hlsl` | VS | globals.hlsli | High-detail variant |
| `GGTreesHighPrepassPS.hlsl` | PS | ShaderInterop_Renderer.h | High-detail alpha test |
| `GGTreeBranchesHighPrepassVS.hlsl` | VS | globals.hlsli | Branch prepass |
| `GGTreeBranchesHighPrepassPS.hlsl` | PS | ShaderInterop_Renderer.h | Branch alpha test |

**Env Probe Vertex Shaders:**

| Shader | Type | PBR Include (via) | Notes |
|--------|------|-------------------|-------|
| `GGTreesHighEnvProbeVS.hlsl` | VS | ShaderInterop_Renderer.h | Env probe vertex |
| `GGTreeBranchesHighEnvProbeVS.hlsl` | VS | ShaderInterop_Renderer.h | Branch env probe vertex |

#### Tier 2B -- Bindless Conversion (3 shaders)

| Shader | Type | High Textures | Conversion Needed |
|--------|------|--------------|-------------------|
| `GGTreesVS.hlsl` | VS | None directly, but FrameCB compat | Wind + `g_xFrame_TreeWind` |
| `GGTreesHighVS.hlsl` | VS | None directly | Wind compat |
| `GGTreeBranchesHighVS.hlsl` | VS | None directly | Wind + branch animation compat |

#### Tier 3 -- Full PBR Migration (6 shaders)

| Shader | Type | PBR Includes | Key Changes |
|--------|------|-------------|-------------|
| `GGTreesPS.hlsl` | PS | brdf + lightingHF | Full PBR, entity iteration, shadow atlas, bindless textures |
| `GGTreesHighPS.hlsl` | PS | brdf + lightingHF | High-detail PBR with subsurface scattering |
| `GGTreeBranchesHighPS.hlsl` | PS | brdf + lightingHF | Branch PBR with alpha |
| `GGTreesHighEnvProbePS.hlsl` | PS | brdf + lightingHF | Env probe PBR. ForwardEntityMaskCB b7->b2 |
| `GGTreeBranchesHighEnvProbePS.hlsl` | PS | brdf + lightingHF | Branch env probe PBR. ForwardEntityMaskCB b7->b2 |
| `GGTreesHighEnvProbePS.hlsl` | PS | brdf + lightingHF | Already listed above -- 5 unique tree Tier 3 PS |

**Corrected Tier 3 tree shaders (5 unique):**
1. `GGTreesPS.hlsl`
2. `GGTreesHighPS.hlsl`
3. `GGTreeBranchesHighPS.hlsl`
4. `GGTreesHighEnvProbePS.hlsl`
5. `GGTreeBranchesHighEnvProbePS.hlsl`

### 4.4 Shared Headers (15 files)

#### Custom Headers (4 files)

| File | Used By | Changes Required |
|------|---------|------------------|
| `GGTerrainConstants.hlsli` | 20+ terrain shaders | Add bindless descriptor index fields to TerrainCB. Add water/fog compat fields. |
| `GGGrassConstants.hlsli` | 6 grass shaders | Add bindless descriptor index fields to GrassCB. |
| `GGTreesConstants.hlsli` | 22 tree shaders | Add bindless descriptor index fields to TreeCB. Add tree wind/SSS compat fields. **Includes PBR/ShaderInterop_Renderer.h** at top -- this creates the Tier 2A dependency for all tree shaders. |
| `GGCommonFunctions.hlsli` | ~12 lit PS shaders | Update FrameCB field references to use compat shim. |

#### PBR Headers (11 files)

| File | Action | Priority |
|------|--------|----------|
| `PBR/globals.hlsli` | **Replace** with DX12 version | Critical (blocks all Tier 2A+) |
| `PBR/ShaderInterop.h` | **Replace** with DX12 version | Critical |
| `PBR/ShaderInterop_Renderer.h` | **Replace** with DX12 version | Critical |
| `PBR/brdf.hlsli` | **Replace** with DX12 version | Critical (blocks all Tier 3) |
| `PBR/lightingHF.hlsli` | **Replace** with DX12 version | Critical (blocks all Tier 3) |
| `PBR/skyAtmosphere.hlsli` | **Replace** with DX12 version | Medium |
| `PBR/skyHF.hlsli` | **Replace** with DX12 version | Medium |
| `PBR/voxelConeTracingHF.hlsli` | **Replace** with DX12 version | Medium |
| `PBR/ConstantBufferMapping.h` | **Delete** (merged into ShaderInterop.h) | Critical |
| `PBR/SamplerMapping.h` | **Delete** (merged into globals.hlsli) | Critical |
| `PBR/ResourceMapping.h` | **Delete** (merged into ShaderInterop.h) | Critical |

---

## 5. Compilation and Build

### 5.1 Current System

- **Compiler:** fxc.exe (D3DCompiler)
- **Shader Model:** 5.0
- **Output format:** DXBC (.cso files)
- **Build integration:** MSBuild FxCompile rules in .vcxproj
- **Output path:** `$(GG_MAX_BUILD_PATH)\Max\shaders\%(Filename).cso`
- **Runtime recompilation:** Wicked Engine has a `LoadShader -> IsShaderOutdated -> DXC compile` path, but GGTerrain shaders are NOT in the `SHADERSOURCEPATH`
- **Root signature:** None -- no existing custom shader uses `[RootSignature(...)]`

### 5.2 Target System

- **Compiler:** dxc.exe (DirectX Shader Compiler)
- **Shader Model:** 6.0
- **Output format:** DXIL (.cso files, same extension)
- **Root signature:** Embedded via `[RootSignature()]` attribute or `-rootsig-define` compiler flag

### 5.3 Root Signature Embedding

Every DX12 shader must contain the engine's root signature. Two approaches:

**Option A: `[RootSignature()]` attribute in HLSL**

Add to each shader's entry point:
```hlsl
#include "PBR/globals.hlsli"  // defines WICKED_ENGINE_DEFAULT_ROOTSIGNATURE

[RootSignature(WICKED_ENGINE_DEFAULT_ROOTSIGNATURE)]
float4 main(PixelIn IN) : SV_TARGET { ... }
```

Pros: Self-documenting, validated at compile time.
Cons: Requires modifying every shader file.

**Option B: `-rootsig-define` compiler flag**

Pass the root signature as a compiler define:
```batch
dxc.exe -T ps_6_0 -rootsig-define WICKED_ENGINE_DEFAULT_ROOTSIGNATURE -D WICKED_ENGINE_DEFAULT_ROOTSIGNATURE="..." shader.hlsl
```

Pros: Zero HLSL changes for Tier 1.
Cons: Complex command line, hard to maintain.

**Option C: Separate root signature blob + manual embedding**

Compile root signature separately, embed into shader binary post-compilation.

**Recommendation:**
- **Tier 1 (10 shaders):** Option B (truly zero HLSL changes) or Option A via a thin wrapper include
- **Tier 2+ (all others):** Option A -- shaders already need `PBR/globals.hlsli` which defines `WICKED_ENGINE_DEFAULT_ROOTSIGNATURE`

### 5.4 Build Integration Options

**Option 1: MSBuild DXC (Lowest Friction)**

Update `.vcxproj` FxCompile items:
```xml
<FxCompile Include="GGTerrain\Shaders\GGTerrainShadowMapVS.hlsl">
    <ShaderType>Vertex</ShaderType>
    <ShaderModel>6.0</ShaderModel>        <!-- was 5.0 -->
    <AdditionalOptions>/rootsig-define RS</AdditionalOptions>
</FxCompile>
```

MSBuild 17+ automatically uses dxc.exe for SM 6.0+.

**Option 2: Manual DXC Batch Script**

Create `compile_shaders.bat` with explicit dxc.exe invocations. Full control over flags. Example:
```batch
set DXC="%WindowsSdkDir%bin\x64\dxc.exe"
%DXC% -T vs_6_0 -E main -Fo shaders/GGTerrainShadowMapVS.cso GGTerrainShadowMapVS.hlsl
```

**Option 3: Runtime Compilation**

Add GGTerrain shader directory to `SHADERSOURCEPATH`. Engine's `LoadShader` auto-compiles outdated shaders.

**Recommendation:** Option 1 for production, Option 2 as fallback for debugging. Option 3 only for development iteration.

### 5.5 Shader Type Mapping

| Extension Pattern | FxCompile ShaderType | DXC Target |
|------------------|---------------------|------------|
| `*VS.hlsl` | Vertex | `vs_6_0` |
| `*PS.hlsl` | Pixel | `ps_6_0` |
| `*CS.hlsl` | Compute | `cs_6_0` |

---

## 6. Testing Milestones

### Milestone 1: Tier 1 Shaders Compile with DXC
**Acceptance:** 10 Tier 1 shaders produce valid .cso files with DXC at SM 6.0. Root signature embedded. Shadow maps render correctly in DX12 mode.

**Test:** Load any level with terrain. Shadow maps should render without artifacts. No `E_INVALIDARG` from root signature deserialization.

### Milestone 2: PBR Headers Updated, Tier 2A Shaders Compile
**Acceptance:** All 22 Tier 2A shaders compile after PBR header update. Terrain prepass, overlay, and tree prepass/shadow renders work.

**Test:** Load a level with terrain and trees. Depth prepass, overlays, and tree shadows render correctly. No missing geometry.

### Milestone 3: Texture Binding Infrastructure Complete
**Acceptance:** `GGBindless.hlsli` created. TerrainCB/GrassCB/TreeCB extended with descriptor index fields. C++ code populates descriptor indices. At least one Tier 2B shader compiles and renders.

**Test:** Single terrain shader (e.g., `GGTerrainQuadPS`) renders textured terrain via bindless path. Visual comparison with DX11 screenshot.

### Milestone 4: Tier 2B Shaders Ported -- FULL DEMO FPS TEST
**Acceptance:** All 11 Tier 2B shaders compile and render. Terrain texturing, grass prepass, tree vertex shaders all functional.

**Test:**
- Load Full Demo level
- Navigate to area with terrain + grass + trees visible
- Verify no visual artifacts
- **FPS test:** Compare DX12 frame rate against DX11 baseline. Target: within 10% of DX11 performance.
- **Automation:** Use `GET_PERF_DATA` command to capture frame times

### Milestone 5: Tier 3 Terrain Shaders Ported
**Acceptance:** All 6 Tier 3 terrain shaders compile and render with full PBR lighting, shadows, and environment probes.

**Test:** Terrain renders with correct lighting, shadows, material blending. Compare screenshots against DX11 reference. Check specular highlights, shadow cascades, fog.

### Milestone 6: Tier 3 Vegetation Shaders Ported
**Acceptance:** All 5 Tier 3 tree shaders + 2 Tier 3 grass shaders compile and render with full PBR lighting, subsurface scattering, and environment probes.

**Test:** Trees and grass render with correct lighting, wind animation, alpha transparency. Subsurface scattering on tree leaves. Environment probes capture vegetation correctly.

### Milestone 7: All Shaders Ported, Visual Parity -- FULL DEMO FPS TEST
**Acceptance:** All 57 shaders compile and render. Full visual parity with DX11 mode.

**Test:**
- Load Full Demo level
- Navigate through all areas (terrain, forest, grassland, water edge)
- **Screenshot comparison:** Automated pixel-diff against DX11 reference screenshots
- **FPS test:** DX12 performance should meet or exceed DX11
- **Stress test:** Rapid camera movement, zoom in/out, enter/exit buildings
- **Automation:** Use `SCREENSHOT` + `GET_PERF_DATA` commands

---

## 7. Porting Order

### Phase 1: Compilation Infrastructure + Tier 1 (Parallel)

**Duration estimate:** First phase
**Dependencies:** None
**Can parallelize:** Yes

| Task | Details | Blocked By |
|------|---------|-----------|
| 1A. Set up DXC compilation pipeline | MSBuild or batch script, SM 6.0, root signature define | Nothing |
| 1B. Compile 10 Tier 1 shaders | Zero HLSL changes, DXC + root sig only | 1A |
| 1C. Runtime test Tier 1 | Load level, verify shadow maps | 1B |

**Tier 1 shader list (10):**
1. `GGTerrainShadowMapVS.hlsl`
2. `GGTerrainEditBoxVS.hlsl`
3. `GGTerrainEditBoxPS.hlsl`
4. `GGTerrainPageGenVS.hlsl`
5. `GGTerrainPageGenPS.hlsl`
6. `GGTerrainPrepassVS.hlsl`
7. `GGTerrainPrepassRefVS.hlsl`
8. `GGTerrainRampVS.hlsl`
9. `GGGrassShadowMapVS.hlsl`
10. `GGGrassShadowMapPS.hlsl` (remove unused t50/t51 declarations)

### Phase 2: PBR Headers + Tier 2A (After Headers Done)

**Dependencies:** Phase 1 complete, PBR headers updated
**Critical path:** PBR header update blocks ALL subsequent phases

| Task | Details | Blocked By |
|------|---------|-----------|
| 2A. Update all PBR/ headers | Replace 8 headers, delete 3 obsolete | Nothing (can start in parallel with Phase 1) |
| 2B. Create GGFrameCompat.hlsli | Compatibility shim for removed FrameCB fields | 2A |
| 2C. Update GGTreesConstants.hlsli | Fix `g_xFrame_TreeWind` references | 2A, 2B |
| 2D. Compile 22 Tier 2A shaders | Should compile after header updates | 2A, 2B, 2C |
| 2E. Runtime test Tier 2A | Prepass, overlays, tree shadows | 2D |

**Tier 2A shader list (22):**

*Terrain (10):*
1. `GGTerrainOverlayVS.hlsl`
2. `GGTerrainOverlayPS.hlsl`
3. `GGTerrainVS.hlsl`
4. `GGTerrainEnvProbeVS.hlsl`
5. `GGTerrainPrepassPS.hlsl`
6. `GGTerrainSphereVS.hlsl`
7. `GGTerrainSpherePrepassPS.hlsl`
8. `GGTerrainRampPS.hlsl`
9. `GGTerrainReadBackVS.hlsl`
10. `GGTerrainReadBackPS.hlsl`

*Vegetation (12):*
1. `GGTreesShadowMapVS.hlsl`
2. `GGTreesHighShadowMapVS.hlsl`
3. `GGTreeBranchesHighShadowMapVS.hlsl`
4. `GGTreesShadowMapPS.hlsl`
5. `GGTreesHighShadowMapPS.hlsl`
6. `GGTreeBranchesHighShadowMapPS.hlsl`
7. `GGTreesPrepassVS.hlsl`
8. `GGTreesPrepassPS.hlsl`
9. `GGTreesHighPrepassVS.hlsl`
10. `GGTreesHighPrepassPS.hlsl`
11. `GGTreeBranchesHighPrepassVS.hlsl`
12. `GGTreeBranchesHighPrepassPS.hlsl`

*Plus 2 env probe VS (also Tier 2A):*
13. `GGTreesHighEnvProbeVS.hlsl`
14. `GGTreeBranchesHighEnvProbeVS.hlsl`

*Plus 1 grass prepass VS:*
15. `GGGrassPrepassVS.hlsl`

### Phase 3: Bindless Texture Infrastructure + Tier 2B

**Dependencies:** Phase 2 complete
**Key deliverable:** C++ descriptor index population + GGBindless.hlsli

| Task | Details | Blocked By |
|------|---------|-----------|
| 3A. Create GGBindless.hlsli | Bindless access macros | 2A |
| 3B. Extend custom CBs | Add descriptor index fields to TerrainCB/GrassCB/TreeCB | Nothing (can start early) |
| 3C. C++ texture binding | Store descriptor heap indices, populate CB fields | 3B |
| 3D. Port Tier 2B shaders | Convert high-register textures to bindless | 3A, 3C |
| 3E. Runtime test + FPS | **FULL DEMO FPS TEST** | 3D |

**Tier 2B shader list (11):**

*Terrain (4):*
1. `GGTerrainQuadVS.hlsl`
2. `GGTerrainQuadPS.hlsl`
3. `GGTerrainReadBackCS.hlsl`
4. `GGTerrainReadBackMSCS.hlsl`

*Grass (1):*
5. `GGGrassPrepassPS.hlsl`

*Trees (3 VS + 3 already covered):*
6. `GGTreesVS.hlsl`
7. `GGTreesHighVS.hlsl`
8. `GGTreeBranchesHighVS.hlsl`

### Phase 4: Tier 3 Shaders (Hardest Phase)

**Dependencies:** Phase 3 complete
**Key challenge:** Full PBR lighting API migration

| Task | Details | Blocked By |
|------|---------|-----------|
| 4A. Create GGLighting.hlsli | Factor shared PBR lighting code | 2A (PBR headers) |
| 4B. Port Tier 3 terrain (6) | Full PBR migration | 3C, 4A |
| 4C. Port Tier 3 vegetation (7) | Full PBR migration | 3C, 4A |
| 4D. Runtime test terrain lighting | Screenshot comparison | 4B |
| 4E. Runtime test vegetation lighting | Screenshot comparison | 4C |

**Tier 3 shader list (13):**

*Terrain (6 + 2 test):*
1. `GGTerrainPS.hlsl` -- Main terrain pixel shader, highest complexity
2. `GGTerrainVirtualPS.hlsl` -- Virtual texture terrain
3. `GGTerrainVirtualPBR_PS.hlsl` -- Virtual texture PBR
4. `GGTerrainSpherePS.hlsl` -- Sphere terrain
5. `GGTerrainEnvProbePS.hlsl` -- Env probe (b7->b2 fix)
6. `terraintestPS.hlsl` -- Test shader (low priority)
7. `terraintestVS.hlsl` -- Test shader (low priority)

*Grass (2):*
8. `GGGrassVS.hlsl` -- Wind animation + FrameCB compat
9. `GGGrassPS.hlsl` -- Full PBR lighting

*Trees (5):*
10. `GGTreesPS.hlsl` -- Full PBR + SSS
11. `GGTreesHighPS.hlsl` -- High-detail PBR + SSS
12. `GGTreeBranchesHighPS.hlsl` -- Branch PBR + alpha
13. `GGTreesHighEnvProbePS.hlsl` -- Env probe PBR (b7->b2)
14. `GGTreeBranchesHighEnvProbePS.hlsl` -- Branch env probe PBR (b7->b2)

### Phase 5: Validation

| Task | Details | Blocked By |
|------|---------|-----------|
| 5A. Full Demo FPS test | DX12 vs DX11 performance comparison | 4B, 4C |
| 5B. Screenshot comparison | Pixel-diff all major views | 4B, 4C |
| 5C. Stress testing | Rapid navigation, zoom, scene transitions | 5A |
| 5D. Edge cases | Water/terrain boundary, LOD transitions, shadow edges | 5A |

---

## 8. Risk Register

| # | Risk | Likelihood | Impact | Mitigation | Owner |
|---|------|-----------|--------|------------|-------|
| R1 | PBR header update breaks existing DX11 compilation | High | Critical | Maintain separate DX11/DX12 header sets via preprocessor `#ifdef WICKEDENGINE_BUILD_DX12` or separate directories | Infrastructure Lead |
| R2 | ShaderEntity struct size change (48->80 bytes) breaks lighting loops | High | High | Update all manual ShaderEntity struct declarations in terrain shaders. Search for `struct ShaderEntity` in all .hlsl files. | Terrain Porter |
| R3 | Bindless texture access produces incorrect results (wrong descriptor index) | Medium | High | Add debug visualization mode: output descriptor index as color. Validate C++ side populates correct heap indices. | C++ Engineer |
| R4 | Root signature mismatch between shader and engine | Medium | Critical | Use same root signature source as engine (extract from wiGraphicsDevice_DX12.cpp). Never hand-author root signature strings. | Infrastructure Lead |
| R5 | DXC compilation errors on SM 6.0 features not available in SM 5.0 code | Low | Medium | SM 6.0 is a superset of 5.0. Only new features need SM 6.0. Existing SM 5.0 code compiles fine. | All Porters |
| R6 | `g_xFrame_TreeWind` and other removed FrameCB fields cause runtime errors | High | Medium | GGFrameCompat.hlsli shim must be tested before any tree shader is loaded. C++ must populate replacement CB fields. | Infrastructure Lead |
| R7 | Sampler register changes (s4-s13 to s100-s109) missed in some shader | Medium | Medium | Global search for `register( s4` through `register( s13` in all .hlsl/.hlsli files. | Quality Gate |
| R8 | Compute shaders (GGTerrainReadBackCS/MSCS) have different root signature requirements | Low | Medium | Verify compute shaders use same root signature. Check UAV descriptor table compatibility. | Infrastructure Lead |
| R9 | Performance regression from bindless texture access vs direct slots | Low | Medium | Benchmark bindless vs slot-based on representative terrain. Bindless typically has negligible overhead. | C++ Engineer |
| R10 | ForwardEntityMaskCB b7->b2 update missed in env probe shaders | Medium | High | Explicitly search for `b7` in all env probe shaders. Test env probe rendering after port. | Vegetation Porter |
| R11 | GGTerrainPageSettings.h (shared with C++) breaks during header update | Medium | Medium | This file is shared between C++ and HLSL. Ensure `__cplusplus` guards are preserved. | Infrastructure Lead |
| R12 | Two-pass compilation needed (DX11 fxc for DX11 mode + DXC for DX12 mode) | High | Medium | If DX11 mode must be preserved, maintain dual build paths. Alternatively, DXC can compile to DXBC for DX11 compatibility with `-target-env vulkan` flags -- investigate. | Build Engineer |
| R13 | `GGTreeBranchesHighShadowMapVS` is byte-for-byte identical to `GGTreesHighShadowMapVS` | Low | Low | Consider deduplicating or leaving as-is. Both get same changes. Document in code. | Vegetation Porter |
| R14 | Entity iteration API change (`ShaderEntityIterator`) requires complete rewrite of lighting loops | High | High | GGLighting.hlsli factoring mitigates: rewrite once, all shaders benefit. | Terrain Porter + Vegetation Porter |
| R15 | Shadow atlas API changes break cascade shadow lookups | High | High | Map old cascade indexing to new atlas-based system in GGLighting.hlsli. Test with multiple shadow cascades visible. | Terrain Porter |

---

## 9. FrameCB/CameraCB Layout Mismatch Analysis (Phase 2 Blocker)

This section documents the detailed findings from the Phase 1 completion analysis.

### 9.1 The Problem

The old `PBR/ShaderInterop_Renderer.h` declares a FrameCB struct (~896 bytes at b0) and CameraCB (~700 bytes at b1). The new WickedEngine DX12 code fills these same CB slots with completely different struct layouts:

| CB | Old Size | New Size | Fields at Same Offset? |
|----|----------|----------|----------------------|
| FrameCB (b0) | ~896 bytes | ~35 KB | Only `options` at offset 0 (partially) |
| CameraCB (b1) | ~700 bytes | ~16 KB | Only `view_projection` at offset 0 |

### 9.2 Key Field Offset Changes (FrameCB)

| Field | Old Offset | New Location | New Offset |
|-------|-----------|-------------|-----------|
| `g_xFrame_Time` | 232 | `g_xFrame.time` | 4 |
| `g_xFrame_SunDirection` | 48 (float3) | `g_xFrame.scene.weather.sun_direction` | ~544 (uint2, half-packed) |
| `g_xFrame_SunColor` | 32 (float3) | `g_xFrame.scene.weather.sun_color` | ~552 (uint2, half-packed) |
| `g_xFrame_Ambient` | 96 (float3) | `g_xFrame.scene.weather.ambient` | ~560 (uint2, half-packed) |
| `g_xFrame_Fog` | 112 (float4) | `g_xFrame.scene.weather.fog` | ~672 (ShaderFog struct) |
| `g_xFrame_EnvProbeArrayOffset` | 280 | `g_xFrame.probes` | ~2048 (packed ShaderEntityIterator) |
| `g_xFrame_LightArrayOffset` | 256 | `g_xFrame.lights` | ~2068 (packed ShaderEntityIterator) |
| `g_xFrame_EntityCullingTileCount` | 208 (uint3) | `g_xCamera.cameras[0].entity_culling_tilecount` | Moved to CameraCB (uint2) |
| `g_xFrame_InternalResolution` | 16 (float2) | `g_xCamera.cameras[0].internal_resolution` | Moved to CameraCB |
| `g_xFrame_TreeWind` | 392 | REMOVED | N/A (GameGuru custom) |
| `g_xFrame_WaterColor` | 400 | `g_xFrame.scene.weather.ocean.water_color` | ~720 |
| `g_xFrame_WaterHeight` | 412 | `g_xFrame.scene.weather.ocean.water_height` | ~752 |

### 9.3 New Engine Architectural Changes

1. **Half-precision packing**: Sun direction, color, horizon, zenith, ambient stored as `uint2` (packed half3), not `float3`
2. **Camera data split**: Screen resolution, entity culling tiles, temporal AA moved from FrameCB to CameraCB
3. **Inlined entity arrays**: EntityArray[256] (16KB) and MatrixArray[256] (16KB) inlined directly in FrameCB instead of separate structured buffers
4. **Entity iteration**: Old `offset+count` replaced by `ShaderEntityIterator` (packed uint with bitfield accessors)
5. **Shadow atlas**: Old per-cascade shadow maps replaced by single shadow atlas with `shadow_atlas_resolution`
6. **Bindless resources**: All textures accessed via `bindless_textures[descriptor_index(...)]` instead of explicit `register(t##)`
7. **PBR function rename**: `DirectionalLight()` → `light_directional()`, `PointLight()` → `light_point()`, `SpotLight()` → `light_spot()`
8. **Surface/Lighting structs changed**: New field names, half-precision, new accessor patterns

### 9.4 Recommended Approach for Phase 2

**Option A (Recommended): Surgical PBR Header Update**
1. Update `PBR/ShaderInterop_Renderer.h` to declare new FrameCB/CameraCB/ShaderEntity layouts
2. Add `#define` macros mapping old `g_xFrame_*` names to new nested struct accessors with half-unpacking
3. Add `#define` macros mapping old `g_xCamera_*` names to `GetCamera().new_name`
4. Update `PBR/globals.hlsli` helper functions to use new accessor patterns
5. Update `PBR/lightingHF.hlsli` to match new ShaderEntity layout and function signatures
6. Keep old function names as wrappers around new engine functions
7. Test each shader tier incrementally

**Option B: Full Engine Header Integration**
- Replace all `PBR/` headers with includes from new WickedEngine shader directory
- Add WickedEngine shaders to include path
- Use GGFrameCompat.hlsli for name compatibility
- Higher risk but cleaner long-term

### 9.5 Shaders Affected by Runtime Mismatch

All 30+ shaders that include any PBR header (directly or transitively) will read incorrect data at runtime:
- **12 Tier 3 PS shaders**: Full PBR lighting pipeline, maximum impact
- **~20 Tier 2 VS/PS shaders**: Shadow maps, prepass, env probes - varying impact
- **10 Tier 1 shaders**: Minimal/no PBR data access, likely WORKING correctly
- **GGTerrainOverlayVS**: Special case - declares own inline FrameCB with 4 fields, all now in CameraCB

---

## Appendix A: Complete File Inventory

### Terrain Shaders (30)

| # | File | Type | Tier | PBR Includes | High Textures |
|---|------|------|------|-------------|---------------|
| 1 | GGTerrainShadowMapVS.hlsl | VS | 1 | None | None |
| 2 | GGTerrainEditBoxVS.hlsl | VS | 1 | None | None |
| 3 | GGTerrainEditBoxPS.hlsl | PS | 1 | None | None |
| 4 | GGTerrainPageGenVS.hlsl | VS | 1 | None | None |
| 5 | GGTerrainPageGenPS.hlsl | PS | 1 | None | None |
| 6 | GGTerrainPrepassVS.hlsl | VS | 1 | None | None |
| 7 | GGTerrainPrepassRefVS.hlsl | VS | 1 | None | None |
| 8 | GGTerrainRampVS.hlsl | VS | 1 | None | None |
| 9 | GGTerrainOverlayVS.hlsl | VS | 2A | FrameCB (inline) | None |
| 10 | GGTerrainOverlayPS.hlsl | PS | 2A | globals.hlsli | t0-t5 |
| 11 | GGTerrainVS.hlsl | VS | 2A | None (TerrainCB only) | None |
| 12 | GGTerrainEnvProbeVS.hlsl | VS | 2A | ShaderInterop_Renderer.h | None |
| 13 | GGTerrainPrepassPS.hlsl | PS | 2A | None (TerrainCB only) | t0-t5 |
| 14 | GGTerrainSphereVS.hlsl | VS | 2A | None (TerrainCB only) | None |
| 15 | GGTerrainSpherePrepassPS.hlsl | PS | 2A | None (TerrainCB only) | t0-t3 |
| 16 | GGTerrainRampPS.hlsl | PS | 2A | None (TerrainCB only) | t0-t3 |
| 17 | GGTerrainReadBackVS.hlsl | VS | 2A | None | None |
| 18 | GGTerrainReadBackPS.hlsl | PS | 2A | None | t0-t5 |
| 19 | GGTerrainQuadVS.hlsl | VS | 2B | None (TerrainCB only) | None |
| 20 | GGTerrainQuadPS.hlsl | PS | 2B | None | t0-t6, t13 |
| 21 | GGTerrainReadBackCS.hlsl | CS | 2B | None (TerrainCB only) | t0-t3 |
| 22 | GGTerrainReadBackMSCS.hlsl | CS | 2B | None (TerrainCB only) | t0-t1 |
| 23 | GGTerrainPS.hlsl | PS | 3 | brdf + lightingHF | t14, t40-t47 |
| 24 | GGTerrainVirtualPS.hlsl | PS | 3 | brdf + lightingHF | t40-t47 |
| 25 | GGTerrainVirtualPBR_PS.hlsl | PS | 3 | brdf + lightingHF | t40-t47 |
| 26 | GGTerrainSpherePS.hlsl | PS | 3 | brdf + lightingHF | t40-t47 |
| 27 | GGTerrainEnvProbePS.hlsl | PS | 3 | brdf + lightingHF | t40-t47 |
| 28 | terraintestPS.hlsl | PS | Test | Varies | Varies |
| 29 | terraintestVS.hlsl | VS | Test | None | None |
| 30 | (GGTerrainShadowMapPS -- not found, shadow map is VS-only) | -- | -- | -- | -- |

### Grass Shaders (6)

| # | File | Type | Tier | PBR Includes | High Textures |
|---|------|------|------|-------------|---------------|
| 1 | GGGrassShadowMapVS.hlsl | VS | 1 | None | None |
| 2 | GGGrassShadowMapPS.hlsl | PS | 1 | None | t50, t51 (UNUSED) |
| 3 | GGGrassPrepassVS.hlsl | VS | 2A | globals.hlsli | None |
| 4 | GGGrassPrepassPS.hlsl | PS | 2B | ShaderInterop_Renderer.h | t50 |
| 5 | GGGrassVS.hlsl | VS | 3 | globals.hlsli | None |
| 6 | GGGrassPS.hlsl | PS | 3 | brdf + lightingHF | t50, t51 |

### Tree Shaders (22)

| # | File | Type | Tier | PBR Includes | High Textures |
|---|------|------|------|-------------|---------------|
| 1 | GGTreesShadowMapVS.hlsl | VS | 2A | globals.hlsli (via constants) | None |
| 2 | GGTreesShadowMapPS.hlsl | PS | 2A | ShaderInterop_Renderer.h (via constants) | None |
| 3 | GGTreesHighShadowMapVS.hlsl | VS | 2A | globals.hlsli (via constants) | None |
| 4 | GGTreesHighShadowMapPS.hlsl | PS | 2A | ShaderInterop_Renderer.h (via constants) | None |
| 5 | GGTreeBranchesHighShadowMapVS.hlsl | VS | 2A | globals.hlsli (via constants) | None |
| 6 | GGTreeBranchesHighShadowMapPS.hlsl | PS | 2A | ShaderInterop_Renderer.h (via constants) | None |
| 7 | GGTreesPrepassVS.hlsl | VS | 2A | globals.hlsli | None |
| 8 | GGTreesPrepassPS.hlsl | PS | 2A | ShaderInterop_Renderer.h (via constants) | None |
| 9 | GGTreesHighPrepassVS.hlsl | VS | 2A | globals.hlsli | None |
| 10 | GGTreesHighPrepassPS.hlsl | PS | 2A | ShaderInterop_Renderer.h (via constants) | None |
| 11 | GGTreeBranchesHighPrepassVS.hlsl | VS | 2A | globals.hlsli | None |
| 12 | GGTreeBranchesHighPrepassPS.hlsl | PS | 2A | ShaderInterop_Renderer.h (via constants) | None |
| 13 | GGTreesHighEnvProbeVS.hlsl | VS | 2A | ShaderInterop_Renderer.h (via constants) | None |
| 14 | GGTreeBranchesHighEnvProbeVS.hlsl | VS | 2A | ShaderInterop_Renderer.h (via constants) | None |
| 15 | GGTreesVS.hlsl | VS | 2B | globals.hlsli | None |
| 16 | GGTreesHighVS.hlsl | VS | 2B | globals.hlsli | None |
| 17 | GGTreeBranchesHighVS.hlsl | VS | 2B | globals.hlsli | None |
| 18 | GGTreesPS.hlsl | PS | 3 | brdf + lightingHF | None (textures via obj) |
| 19 | GGTreesHighPS.hlsl | PS | 3 | brdf + lightingHF | None (textures via obj) |
| 20 | GGTreeBranchesHighPS.hlsl | PS | 3 | brdf + lightingHF | None (textures via obj) |
| 21 | GGTreesHighEnvProbePS.hlsl | PS | 3 | brdf + lightingHF | None |
| 22 | GGTreeBranchesHighEnvProbePS.hlsl | PS | 3 | brdf + lightingHF | None |

### Header Files (15)

| # | File | Type | Action |
|---|------|------|--------|
| 1 | GGTerrainConstants.hlsli | Custom CB | Extend with descriptor indices + compat fields |
| 2 | GGGrassConstants.hlsli | Custom CB | Extend with descriptor indices |
| 3 | GGTreesConstants.hlsli | Custom CB | Extend with descriptor indices + wind/SSS compat |
| 4 | GGCommonFunctions.hlsli | Utility | Update FrameCB field references |
| 5 | PBR/globals.hlsli | Engine | Replace with DX12 version |
| 6 | PBR/ShaderInterop.h | Engine | Replace with DX12 version |
| 7 | PBR/ShaderInterop_Renderer.h | Engine | Replace with DX12 version |
| 8 | PBR/brdf.hlsli | Engine | Replace with DX12 version |
| 9 | PBR/lightingHF.hlsli | Engine | Replace with DX12 version |
| 10 | PBR/skyAtmosphere.hlsli | Engine | Replace with DX12 version |
| 11 | PBR/skyHF.hlsli | Engine | Replace with DX12 version |
| 12 | PBR/voxelConeTracingHF.hlsli | Engine | Replace with DX12 version |
| 13 | PBR/ConstantBufferMapping.h | Engine (obsolete) | Delete |
| 14 | PBR/SamplerMapping.h | Engine (obsolete) | Delete |
| 15 | PBR/ResourceMapping.h | Engine (obsolete) | Delete |

### New Files to Create (4)

| # | File | Purpose |
|---|------|---------|
| 1 | GGFrameCompat.hlsli | Compatibility shim for removed FrameCB fields |
| 2 | GGBindless.hlsli | Bindless texture access macros |
| 3 | GGLighting.hlsli | Factored PBR lighting code (shared by all Tier 3 PS) |
| 4 | GGRootSignature.hlsli | Root signature definition for Tier 1 shaders (wraps engine RS) |

---

## Appendix B: C++ Integration Points

### Key C++ Files

| File | Role | Changes Needed |
|------|------|---------------|
| `Guru-WickedMAX/GGTerrain/GGTerrain.cpp` | Terrain rendering, CB population | Populate descriptor index fields in TerrainCB |
| `Guru-WickedMAX/GGTerrain/GGGrass.cpp` | Grass rendering, CB population | Populate descriptor index fields in GrassCB |
| `Guru-WickedMAX/GGTerrain/GGTrees.cpp` | Tree rendering, CB population | Populate descriptor index fields in TreeCB, add wind/SSS fields |
| `Guru-WickedMAX/GGTerrain/GGTerrainPageSettings.h` | Shared C++/HLSL constants | Ensure `__cplusplus` guards preserved during header update |
| `Guru-WickedMAX/GGTerrain/GGTerrain_Render.cpp` | Shader loading, PSO creation | Update shader compilation calls for DXC |

### Descriptor Index Population Pattern

```cpp
// In GGTerrain.cpp, when setting up terrain draw:
TerrainCB cb = {};
// ... existing field population ...

// NEW: populate bindless descriptor indices
cb.terrain_texColorIndex = device->GetDescriptorIndex(texColor, SubresourceType::SRV);
cb.terrain_texNormalIndex = device->GetDescriptorIndex(texNormal, SubresourceType::SRV);
cb.terrain_texRockColorIndex = device->GetDescriptorIndex(texRockColor, SubresourceType::SRV);
// ... etc for all high-register textures ...

device->BindDynamicConstantBuffer(cb, CB_SLOT_2, cmd);
```

### Texture Loading Changes

Currently textures are bound to specific SRV slots:
```cpp
device->BindResource(&texColor, 40, cmd);  // t40
```

New approach -- textures registered in bindless heap, index stored in CB:
```cpp
// At texture creation time:
int descriptorIndex = device->CreateSubresource(&texColor, SubresourceType::SRV, ...);
// Store descriptorIndex for later CB population
```

---

## Appendix C: Key Register Mappings

### Terrain Material Textures (Currently t40-t47)

| Old Register | Texture | New Access |
|-------------|---------|------------|
| t40 | texColor (base terrain color) | `bindless_textures[terrain_texColorIndex]` |
| t41 | texNormal (base terrain normal) | `bindless_textures[terrain_texNormalIndex]` |
| t42 | texRockColor | `bindless_textures[terrain_texRockColorIndex]` |
| t43 | texRockNormal | `bindless_textures[terrain_texRockNormalIndex]` |
| t44 | texSnowColor | `bindless_textures[terrain_texSnowColorIndex]` |
| t45 | texSnowNormal | `bindless_textures[terrain_texSnowNormalIndex]` |
| t46 | texSandColor | `bindless_textures[terrain_texSandColorIndex]` |
| t47 | texSandNormal | `bindless_textures[terrain_texSandNormalIndex]` |

### Grass Textures (Currently t50-t51)

| Old Register | Texture | New Access |
|-------------|---------|------------|
| t50 | texGrass (Texture2DArray) | `bindless_textures2DArray[grass_texGrassIndex]` |
| t51 | texNoise (Texture2D) | `bindless_textures[grass_texNoiseIndex]` |

### Engine Textures Used by Custom Shaders (t0-t15, stay as-is)

| Register | Texture | Used By | Change |
|----------|---------|---------|--------|
| t0 | Read-back / page data | Terrain quad/readback | None (within limit) |
| t1-t6 | Page table, cache, normals, mask, LOD normals | Terrain rendering | None (within limit) |
| t13 | texture_skyluminancelut | Terrain quad PS | None (within limit) |
| t14 | texture_shadowarray_2d | Terrain PS (shadow maps) | None (within limit) |

### Sampler Mappings

| Old Register | New Register | Sampler Type | Notes |
|-------------|-------------|-------------|-------|
| s0 | s0 | Point/wrap | In descriptor table |
| s1 | s1 | Trilinear/clamp | In descriptor table |
| s7 | s7 | Point/clamp | In descriptor table |
| s4 | s100 | sampler_linear_clamp | Static sampler |
| s5 | s101 | sampler_linear_wrap | Static sampler |
| s6 | s102 | sampler_linear_mirror | Static sampler |
| s8 | s104 | sampler_aniso_clamp | Static sampler |
| s9 | s105 | sampler_aniso_wrap | Static sampler |
| s10 | s106 | sampler_aniso_mirror | Static sampler |
| s11 | s107 | sampler_cmp_depth | Static sampler |

---

*End of DX11 to DX12 Shader Porting Plan*
