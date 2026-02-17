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

## PRE-ANALYZED: DX12 Root Signature Architecture (CRITICAL FINDING)

We have already analyzed the latest Wicked Engine DX12 `globals.hlsli` and `ShaderInterop.h`. This section contains **confirmed findings** that answer most of the critical architecture questions. Use these as ground truth — do not re-investigate what is already answered here.

### The Root Signature (from `globals.hlsli` lines 206-288)

The latest Wicked Engine defines `WICKED_ENGINE_DEFAULT_ROOTSIGNATURE` as a string macro in `globals.hlsli`. It is used by the DXC compiler to embed the root signature into DXIL bytecode. The layout is:

```
Root Parameter 0: RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT)
Root Parameter 1: RootConstants(num32BitConstants=12, b999)     ← Push constants
Root Parameter 2: CBV(b0)                                       ← FrameCB
Root Parameter 3: CBV(b1)                                       ← CameraCB  
Root Parameter 4: CBV(b2)                                       ← On-demand CB slot
Root Parameter 5: DescriptorTable(
                    CBV(b3, numDescriptors=11),                  ← b3-b13
                    SRV(t0, numDescriptors=16),                  ← t0-t15 ONLY
                    UAV(u0, numDescriptors=16)                   ← u0-u15
                  )
Root Parameter 6: DescriptorTable(Sampler(s0, numDescriptors=8)) ← s0-s7
Root Parameter 7: DescriptorTable(Sampler space1, unbounded)     ← Bindless samplers
Root Parameter 8: DescriptorTable(                               ← Bindless resources
                    SRV space2-30 (unbounded each),              ← Bindless textures by type
                    UAV space100-115 (unbounded each),           ← Bindless RW resources
                    SRV space200-208 (unbounded each)            ← Bindless structured buffers
                  )
Static Samplers:  s100-s109                                      ← Engine samplers (always available)
```

### What This Means — Confirmed Compatibility Matrix

#### ✅ WORKS AS-IS (no shader changes needed for these)

| Feature | Why It Works |
|---------|-------------|
| **Input Assembler** | `ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT` is set. All 7+ custom vertex formats with custom semantics (INORMAL, OFFSET, DATA, ID, etc.) survive. |
| **CBV b0 (FrameCB)** | Dedicated root CBV slot. |
| **CBV b1 (CameraCB)** | Dedicated root CBV slot. |
| **CBV b2 (TerrainCB/GrassCB/TreeCB)** | Dedicated root CBV slot. This is the big win — all custom constant buffers at b2 work directly. |
| **CBV b3-b13** | In the descriptor table. Covers b7 (ForwardEntityMaskCB) and b8 (CubemapRenderCB). |
| **SRV t0-t15** | In the descriptor table. Covers terrain debug textures (t0-t6, t13), shadow arrays (t14). |
| **UAV u0-u15** | In the descriptor table. Covers compute shader output (u0). |
| **On-demand samplers s0-s7** | In sampler descriptor table. Covers all custom sampler declarations at s0-s3. |
| **Push constants b999** | 12 root constants. Available if needed. |

#### ❌ BREAKS — Requires Shader Changes

| Feature | Why It Breaks | Scope of Impact |
|---------|--------------|-----------------|
| **SRV t16-t63** | Root signature only has `SRV(t0, numDescriptors=16)` — slots t16+ do not exist in the descriptor table. | **ALL custom texture bindings above t15**: t20-t21 (EntityArray/MatrixArray), t40-t47 (terrain materials), t50-t58 (VT/grass/tree textures) |
| **Old persistent samplers s4-s15** | These slot numbers no longer exist. Engine samplers moved to static samplers at s100-s109. | Any Tier 3 shader that includes `globals.hlsli` and uses `sampler_linear_clamp`, `sampler_cmp_depth` etc. — these now live at s100-s109 instead of s4-s13. |
| **`STRUCTUREDBUFFER` macro for EntityTiles** | Old macro expanded to `StructuredBuffer<type> name : register(t##slot)` with a t-slot. In the new engine, structured buffers are accessed via `bindless_structured_uint[]` in space206. | All Tier 3 shaders using TiledLighting (reads EntityTiles) |
| **EntityArray (t20) and MatrixArray (t21)** | Slot t20/t21 outside the t0-t15 range. In new engine, accessed via `GetFrame().entityArray[index]` and `GetFrame().matrixArray[index]` (stored in FrameCB, accessed via bindless structured buffers). | GGTerrainPS.hlsl (local ShaderEntity copy), plus any shader using shadow cascades |

#### The Bindless Texture Model (New Engine)

In the latest Wicked Engine, textures are accessed via typed unbounded arrays in separate register spaces:

```hlsl
// DX12 path (from globals.hlsli lines 417-465):
SamplerState            bindless_samplers[]              : register(space1);
Texture2D               bindless_textures[]              : register(space2);
ByteAddressBuffer       bindless_buffers[]               : register(space3);
Texture2DArray          bindless_textures2DArray[]       : register(space13);
Texture2D<float>        bindless_textures_float[]        : register(space20);
Texture2D<uint>         bindless_textures_uint[]         : register(space22);
StructuredBuffer<uint>  bindless_structured_uint[]       : register(space206);
// ... etc. for every resource type
```

Textures are identified by **descriptor index** (an integer stored in material/frame data). Access pattern:
```hlsl
// Old (slot-based):
Texture2DArray texGrass : register(t50);
color = texGrass.Sample(sampler1, uv);

// New (bindless):
Texture2DArray texGrass = bindless_textures2DArray[descriptor_index(grass_textureIndex)];
color = texGrass.Sample(sampler_aniso_wrap, uv);  // static sampler at s107
```

Engine static samplers (always available, no binding needed):
```hlsl
SamplerState          sampler_linear_clamp   : register(s100);
SamplerState          sampler_linear_wrap    : register(s101);
SamplerState          sampler_linear_mirror  : register(s102);
SamplerState          sampler_point_clamp    : register(s103);
SamplerState          sampler_point_wrap     : register(s104);
SamplerState          sampler_point_mirror   : register(s105);
SamplerState          sampler_aniso_clamp    : register(s106);
SamplerState          sampler_aniso_wrap     : register(s107);
SamplerState          sampler_aniso_mirror   : register(s108);
SamplerComparisonState sampler_cmp_depth     : register(s109);
```

### Confirmed Porting Strategy Per Category

Based on the root signature analysis, here is what each category of change requires:

**Category A — Tier 1 shaders that use ONLY b0/b1/b2 and no textures:**
→ Recompile with DXC + embed `WICKED_ENGINE_DEFAULT_ROOTSIGNATURE`. **Zero shader code changes.** This covers 13 shaders (all Tier 1).

**Category B — Shaders using t0-t15 textures (terrain debug, shadow arrays):**
→ Recompile with DXC + embed root signature. **Zero or minimal shader code changes.** Slots t0-t15 are in the descriptor table.

**Category C — Shaders using t16+ textures (the majority of Tier 2 and all Tier 3):**
→ Must convert texture declarations from `register(t##)` to **bindless lookups**. Two sub-strategies:

  - **C1 (Recommended — Minimal Change):** Pass descriptor indices through the existing custom CBs (TerrainCB, GrassCB, TreeCB). Add `int` fields for each texture index. Shader declares textures via `bindless_textures2DArray[descriptor_index(cb_field)]` instead of `register(t50)`. C++ side stores descriptor heap indices instead of calling `BindResource(slot, texture)`.

  - **C2 (Alternative — Expand Root Signature):** Modify `WICKED_ENGINE_DEFAULT_ROOTSIGNATURE` to increase `numDescriptors` from 16 to 64 for SRVs: `SRV(t0, numDescriptors=64)`. This would make t0-t63 available and preserve all slot-based bindings. **Risk:** this may conflict with how the engine's DX12 backend manages its descriptor heap, and requires changes to `wiGraphicsDevice_DX12.cpp` descriptor table setup. Investigate feasibility.

**Category D — Samplers:**
→ On-demand samplers at s0-s3 still work (s0-s7 descriptor table exists). Persistent engine samplers have moved from s4-s13 to static samplers at s100-s109. For custom shaders that reference engine samplers by old slot names (via `globals.hlsli`), the updated `globals.hlsli` handles this automatically since the sampler variables are declared at s100+. For custom shaders that declare their own `SamplerState` at s0-s3, no change needed.

**Category E — EntityArray/MatrixArray/EntityTiles (structured buffers):**
→ Old: `StructuredBuffer<ShaderEntity> EntityArray : register(t20)`. New: accessed via `load_entity(index)`, `load_entitymatrix(index)`, `load_entitytile(index)` macros that use bindless structured buffers. Custom shaders must migrate to these accessor macros or equivalent bindless lookups.

### Important Note on the PBR/ Headers

The custom shaders include **old versions** of `PBR/globals.hlsli`, `PBR/ShaderInterop.h`, `PBR/brdf.hlsli`, `PBR/lightingHF.hlsli`, etc. These are the DX11-era headers from the old Wicked Engine fork. The **latest versions** of these files define bindless resources, the root signature, new Surface/Lighting APIs, new FrameCB/CameraCB layouts, etc.

**The PBR/ headers MUST be updated to the latest Wicked Engine versions** (or a compatible subset). This is not optional — the root signature is defined in `globals.hlsli`, and the old `globals.hlsli` declares slot-based textures/samplers that conflict with the DX12 root signature.

This header update will cascade changes through all Tier 3 shaders because:
- `Surface` struct API has changed (new fields, different `create*` methods)
- `Lighting` struct and light evaluation functions have changed
- `ShaderEntity` struct has changed (new packing, different accessor methods)
- `FrameCB`/`CameraCB` layouts have changed (different field names, new fields)
- Fog, ambient, and sky functions have changed
- Entity iteration now uses `ShaderEntityIterator` and `load_entity()` instead of array indexing

**This PBR header update is the largest single piece of work in the entire port**, affecting all 12 Tier 3 shaders and several Tier 2 shaders that include `globals.hlsli`.

---

## Complete Shader Inventory (All 62 Files Analyzed)

### Shared Engine Headers (PBR/ subfolder — OLD versions, must be updated)

| File | Role |
|------|------|
| `PBR/ShaderInterop.h` | Master header — old version has separate mapping headers, TEXTURE2D/SAMPLERSTATE macros |
| `PBR/ConstantBufferMapping.h` | CB slot #defines — **removed in latest engine** (inlined into ShaderInterop.h) |
| `PBR/SamplerMapping.h` | Sampler slot #defines — **removed in latest engine** |
| `PBR/ResourceMapping.h` | Texture/SRV slot #defines — **removed in latest engine** |
| `PBR/ShaderInterop_Renderer.h` | FrameCB, CameraCB, ShaderEntity, etc. — **significantly changed** |
| `PBR/globals.hlsli` | Engine textures/samplers/buffers — **completely rewritten** for bindless |
| `PBR/brdf.hlsli` | BRDF + Surface struct — **Surface API changed** |
| `PBR/lightingHF.hlsli` | Lighting functions — **significantly changed** |
| `PBR/skyHF.hlsli` | Sky rendering — changed |
| `PBR/skyAtmosphere.hlsli` | Atmospheric scattering — may have changed |
| `PBR/voxelConeTracingHF.hlsli` | Voxel GI — may have changed or been replaced |

### Shared GameGuru Headers

| File | Role |
|------|------|
| `GGCommonFunctions.hlsli` | `ApplyFogCustom()` — references FrameCB fields that may have been renamed |
| `GGTerrainPageSettings.h` | Virtual texture constants — CPU/GPU shared, likely unchanged |

### Constant Buffers Used

| Slot | Name | Contents | Used By | DX12 Status |
|------|------|----------|---------|-------------|
| `b0` | `FrameCB` | Sun, fog, time, options, etc. | All shaders | ✅ Root CBV |
| `b0` | Custom `viewProj` | Just a float4x4 | `GGTerrainReadBackVS` only | ⚠️ Conflicts, isolated pass |
| `b1` | `CameraCB` | VP matrix, cam pos, clip plane | Most VS/PS | ✅ Root CBV |
| `b2` | `TerrainCB` | LOD levels, layers, slopes, masks | All terrain shaders | ✅ Root CBV |
| `b2` | `GrassCB` | Rotation matrices, grass types | All grass shaders | ✅ Root CBV |
| `b2` | `TreeCB` | Rotation matrices, tree types | All tree shaders | ✅ Root CBV |
| `b7` | `ForwardEntityMaskCB` | xForwardLightMask | Env probe PS | ✅ b3-b13 table, but ⚠️ slot changed to b2 in latest |
| `b8` | `CubemapRenderCB` | xCubemapRenderCams[6] | Env probe VS | ✅ b3-b13 table |

**⚠️ SLOT CONFLICT: `CBSLOT_RENDERER_FORWARD_LIGHTMASK` changed from 7 to 2 in latest `ShaderInterop.h`.** This means ForwardEntityMaskCB may now conflict with TerrainCB/GrassCB/TreeCB at b2. Investigate how the latest engine resolves this.

### Custom Texture Slot Usage

| System | Slots | Resources | DX12 Status |
|--------|-------|-----------|-------------|
| **Terrain (non-VT)** | t40-t47 | Color/Normal pairs | ❌ Above t15 |
| **Terrain (VT)** | t50-t58 | Page cache, tables, LOD data | ❌ Above t15 |
| **Terrain (debug)** | t0-t6, t13 | Debug quad textures | ✅ Within t0-t15 |
| **Terrain (compute)** | t50 + u0 | Input + UAV | ❌ t50; ✅ u0 |
| **Terrain (non-VT PS)** | t14, t20-t21 | Shadows, Entity/Matrix arrays | ⚠️ t14 ✅; t20-t21 ❌ |
| **Grass** | t50-t51, t53 | Textures | ❌ Above t15 |
| **Billboard trees** | t50-t51, t53 | Textures | ❌ Above t15 |
| **High-detail trees** | t51-t52 | Textures | ❌ Above t15 |
| **High-detail branches** | t51, t54 | Textures | ❌ Above t15 |

### Sampler Usage

| Slot | Old Name | New Static Sampler | Status |
|------|----------|-------------------|--------|
| s0-s3 | Custom on-demand | N/A | ✅ In s0-s7 table |
| s4 | sampler_linear_clamp | s100 | ⚠️ Auto-fixed by header update |
| s5 | sampler_linear_wrap | s101 | ⚠️ Auto-fixed by header update |
| s7 | sampler_point_clamp | s103 | ⚠️ Auto-fixed by header update |
| s13 | sampler_cmp_depth | s109 | ⚠️ Auto-fixed by header update |

### Vertex Input Formats — All ✅ Confirmed Working

All use input assembler (`ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT` is set):
Terrain mesh, Terrain sphere, Terrain page gen, Billboard tree, High-detail tree, Tree env probe, Grass, Grass shadow, Simple quad/overlay, Edit box/ramp, Readback.

---

### Complete File Inventory with Tiers and DX12 Status

#### Tier 1 — Category A: ZERO code changes (recompile + root signature only)

| File | Type | CBs | Notes |
|------|------|-----|-------|
| GGTerrainShadowMapVS | VS | b1 | Simplest possible |
| GGTerrainPrepassRefVS | VS | b1,b2 | Clip distance only |
| GGTerrainQuadVS | VS | — | No cbuffers |
| GGTerrainRampVS | VS | b1,b2 | Editor |
| GGTerrainRampPS | PS | b2 | Editor |
| GGTerrainEditBoxVS | VS | b1,b2 | Editor |
| GGTerrainEditBoxPS | PS | b2 | Editor |
| GGTerrainSpherePrepassPS | PS | — | Returns zeros |
| GGGrassShadowMapVS | VS | b1,b2 | |
| GGGrassShadowMapPS | PS | b2 | |
| GGTreesShadowMapVS | VS | b1,b2 | |
| GGTreesHighShadowMapVS | VS | b1,b2 | |
| GGTreeBranchesHighShadowMapVS | VS | b1,b2 | |

**13 shaders — IMMEDIATE QUICK WIN**

#### Tier 2A — Category A/B: ZERO code changes (VS with no textures, or textures ≤ t15)

| File | Type | CBs | Tex Status | Notes |
|------|------|-----|-----------|-------|
| GGTerrainVS | VS | b1,b2 | N/A | |
| GGTerrainPrepassVS | VS | b1,b2 | N/A | |
| GGTerrainEnvProbeVS | VS | b2,b8 | N/A | |
| GGTerrainOverlayVS | VS | b0(partial) | N/A | Must update partial FrameCB |
| GGTerrainPageGenVS | VS | b2 | N/A | |
| GGTerrainSphereVS | VS | b1 | N/A | |
| GGTerrainReadBackPS | PS | — | ✅ t3 | |
| GGTerrainOverlayPS | PS | b2 | ✅ t0-t2 | |
| GGTerrainQuadPS | PS | b2 | ✅ t0-t6,t13 | |
| GGGrassVS | VS | b1,b2 | N/A | |
| GGGrassPrepassVS | VS | b1,b2 | N/A | |
| GGTreesVS | VS | b1,b2 | N/A | |
| GGTreesPrepassVS | VS | b1,b2 | N/A | |
| GGTreesHighVS | VS | b1,b2 | N/A | |
| GGTreesHighPrepassVS | VS | b1,b2 | N/A | |
| GGTreeBranchesHighVS | VS | b1,b2 | N/A | |
| GGTreeBranchesHighPrepassVS | VS | b1,b2 | N/A | |
| GGTreesHighEnvProbeVS | VS | b2,b8 | N/A | |
| GGTreeBranchesHighEnvProbeVS | VS | b2,b8 | N/A | |

**19 shaders — ZERO or minimal code changes** (only GGTerrainOverlayVS needs partial FrameCB fix)

#### Tier 2B — Category C: Bindless texture conversion needed

| File | Type | CBs | Broken Tex | Notes |
|------|------|-----|-----------|-------|
| GGTerrainReadBackVS | VS | b0(custom!) | — | ⚠️ b0 conflict, isolated pass |
| GGTerrainReadBackCS | CS | b2 | t50 | Compute shader |
| GGTerrainReadBackMSCS | CS | b2 | t50(MS) | Compute, multisample |
| GGTerrainPrepassPS | PS | b2 | t53,t54 | VT readback |
| GGGrassPrepassPS | PS | b2 | t50-t51 | Alpha test + LOD |
| GGTreesShadowMapPS | PS | b2 | t50-t51 | Alpha test + LOD |
| GGTreesHighShadowMapPS | PS | b2 | t51 | LOD fade |
| GGTreeBranchesHighShadowMapPS | PS | b2 | t51,t54 | Alpha test + LOD |
| GGTreesPrepassPS | PS | b2 | t50-t51 | |
| GGTreesHighPrepassPS | PS | b2 | t51 | |
| GGTreeBranchesHighPrepassPS | PS | b2 | t51,t54 | |

**11 shaders — need bindless texture conversion (but no PBR chain changes)**

#### Tier 3 — Category C+E: Full migration (bindless + PBR headers + entity access)

| File | Type | CBs | Broken Tex | Lighting | Notes |
|------|------|-----|-----------|----------|-------|
| GGTerrainPS | PS | b2 | t40-t47,t20-t21 | Custom shadow | Local ShaderEntity, Entity/MatrixArray |
| GGTerrainPageGenPS | PS | b2 | t50-t58 | None | Complex material blending |
| GGTerrainVirtualPS | PS | b2 | t50-t54 | Tiled | VT + tiled lighting |
| GGTerrainVirtualPBR_PS | PS | b2 | t50-t54 | Tiled | 424 lines, most complete |
| GGTerrainEnvProbePS | PS | b2,b7 | t50-t54 | Forward | ⚠️ b7 slot changed |
| GGTerrainSpherePS | PS | b2 | t50-t52 | Tiled | Material preview |
| GGGrassPS | PS | b2 | t50-t51,t53 | Tiled | Grass + LOD + fog |
| GGTreesPS | PS | b2 | t50-t51,t53 | Tiled | Billboard trees |
| GGTreesHighPS | PS | b2 | t51-t52 | Tiled | High-detail trees |
| GGTreeBranchesHighPS | PS | b2 | t51,t54 | Tiled | Branches + alpha |
| GGTreesHighEnvProbePS | PS | b2,b7 | t52 | Forward | ⚠️ b7 slot changed |
| GGTreeBranchesHighEnvProbePS | PS | b2,b7 | t54 | Forward | ⚠️ b7 slot changed |

**12 shaders — all need significant work**

**Grand total: 55 shader files + 7 header files = 62 files**
**Breakdown: 32 zero-change + 11 bindless-only + 12 full-migration = 55 shaders**

### Key Quirks

1. **`GGTerrainPS.hlsl`** — local `ShaderEntity` copy + manual `EntityArray : register(t20)`, `MatrixArray : register(t21)`. Must migrate to `load_entity()`/`load_entitymatrix()`.
2. **`GGTerrainReadBackVS.hlsl`** — `cbuffer constants : register(b0)` conflicts with FrameCB.
3. **`GGTerrainOverlayVS.hlsl`** — partial FrameCB re-declaration.
4. **`GGTreesConstants.hlsli`** — uses `g_xFrame_TreeWind`, `g_xFrame_Time` from FrameCB. Field names may have changed.
5. **`CBSLOT_RENDERER_FORWARD_LIGHTMASK` = 2** in latest engine (was 7). Potential b2 conflict.
6. **`DISABLE_WAVE_INTRINSICS`** — can be removed for SM 6.0+ performance.
7. **Duplicated TiledLighting/ForwardLighting** — copy-pasted into 12 Tier 3 shaders. Recommend factoring into shared header.

---

## Your Task

Using the inventory and root signature analysis above, investigate the **engine C++ code** and produce a detailed porting plan.

### Step 1: Examine the C++ Integration

The root signature and shader-side architecture are already analyzed above. Now find the C++ code:

1. **Shader compilation/loading** — how are the 62 shaders compiled? Build step or runtime?
2. **Pipeline state creation** — input layout setup for custom semantics
3. **Constant buffer binding** — how does C++ bind TerrainCB/GrassCB/TreeCB to b2?
4. **Texture binding** — how does C++ currently bind textures to t40-t58? **This must change.**
5. **Draw calls** — render pass structure
6. **Descriptor heap management** — how does the DX12 backend allocate descriptor indices?
7. **Forward light mask** — is it still via b7 or has it changed?

### Step 2: Determine Texture Binding Strategy

Two options — investigate feasibility and recommend one:

**Option C1: Bindless via custom CBs** — Add descriptor index fields to TerrainCB/GrassCB/TreeCB. Shaders use `bindless_textures2DArray[descriptor_index(field)]`. C++ stores descriptor heap indices. Clean, future-proof.

**Option C2: Expand SRV range** — Change `SRV(t0, numDescriptors=16)` to `SRV(t0, numDescriptors=64)`. Keep `register(t##)` declarations. Investigate if this breaks engine descriptor heap management.

### Step 3: Write the Plan

#### Section 1: Executive Summary
- 32 zero-change + 11 bindless-only + 12 full-migration = 55 shaders
- Texture binding strategy recommendation
- Key risks: PBR header drift, FrameCB field renames, b7→b2 conflict

#### Section 2: Architecture Comparison
- Old DX11 slot-based vs new DX12 hybrid (CBs slot-based, textures bindless)
- Root signature summary (confirmed above)
- Texture binding strategy decision with rationale

#### Section 3: Shared Infrastructure (do first)
- Update PBR/ headers to latest Wicked Engine versions
- Root signature embedding
- Compilation pipeline (fxc→dxc)
- Create `GGBindless.hlsli` helper if using C1
- Create `GGLighting.hlsli` from duplicated code
- Update GGCommonFunctions.hlsli
- C++ descriptor index infrastructure

#### Section 4: Per-System Porting Guide
- 4.1 Terrain (28 files), 4.2 Grass (8 files), 4.3 Trees (23 files), 4.4 Shared Headers

#### Section 5: Compilation & Build
#### Section 6: Testing Milestones (7 milestones from shadow→env probes)
#### Section 7: Porting Order
1. PBR/ headers (blocks Tier 2+)
2. Compilation infrastructure
3. 13 Tier 1 shaders (zero changes)
4. Texture binding infrastructure
5. 19 Tier 2A shaders (zero changes)
6. 11 Tier 2B shaders (bindless conversion)
7. 12 Tier 3 shaders (full migration)

## Output

Save as `DX11_to_DX12_Shader_Porting_Plan.md` in the project root.

## Critical Reminders

- Root signature analysis is **confirmed** — verify against `wiGraphicsDevice_DX12.cpp` but don't re-derive.
- **32 shaders need zero code changes** — call this out as the immediate win.
- **PBR header update is the critical path** — estimate scope carefully.
- **b7→b2 conflict** for ForwardEntityMaskCB is a potential blocker. Investigate immediately.
- **GGTerrainPS.hlsl** local ShaderEntity must migrate to `load_entity()`/`load_entitymatrix()`.
- Recommend **factoring duplicated lighting** into shared headers.
- Recommend **enabling wave intrinsics** for SM 6.0+.
- Be specific: filenames, line numbers, C++ function/class names, register slots.

---

## Agent Team Structure for Execution

This porting task involves 62 shader/header files (~8,000 lines HLSL), 8 C++ integration files (~18,000 lines), and a critical-path PBR header update (~4,600 lines). The following 6-agent team is designed for maximum safety, parallelism, and quality on a task where one wrong constant buffer layout or register slot can cascade into silent visual corruption or GPU device removal.

### Team Composition

#### 1. COORDINATOR (Team Lead) — `general-purpose` agent

**Role**: Non-coding orchestrator. Manages the task list, enforces porting order, resolves inter-agent conflicts, and is the single source of truth on project status.

**Responsibilities**:
- Create and maintain the task list based on the 7-phase porting order (Section 7 of the plan)
- Enforce the dependency chain: PBR headers → compilation infra → Tier 1 → texture infra → Tier 2A → Tier 2B → Tier 3
- Assign tasks to agents based on readiness (no blocked tasks assigned)
- Request builds after each milestone and route build results to the Quality Gate agent
- Coordinate handoffs: Infrastructure Lead must complete PBR headers before Shader Porters begin Tier 2+ work
- Track the b7→b2 ForwardEntityMaskCB conflict resolution — this is a cross-cutting concern that affects both C++ and shader agents
- Request the Full Demo FPS Test (WETEST.md) at milestones 4 and 7 to catch regressions
- **Never writes or edits code directly** — delegates all implementation to specialists

**Key rule**: If any agent proposes a change that affects the root signature, constant buffer layout, or descriptor heap management, the Coordinator MUST halt that agent and route the proposal through the Quality Gate for review BEFORE implementation.

#### 2. INFRASTRUCTURE LEAD — `general-purpose` agent

**Role**: Owns all shared foundation work that blocks other agents. This is the critical-path agent.

**Responsibilities**:
- **Phase 1**: Update all 11 PBR/ header files to match the latest Wicked Engine DX12 versions:
  - `PBR/globals.hlsli` (805 lines) — root signature macro, bindless resource declarations, static samplers at s100-s109
  - `PBR/ShaderInterop.h` — master header, remove old mapping #includes
  - `PBR/ShaderInterop_Renderer.h` (787 lines) — FrameCB, CameraCB, ShaderEntity struct updates
  - `PBR/brdf.hlsli` (449 lines) — Surface struct API changes
  - `PBR/lightingHF.hlsli` (930 lines) — new lighting evaluation, ShaderEntityIterator
  - Remove obsolete: `ConstantBufferMapping.h`, `SamplerMapping.h`, `ResourceMapping.h`
  - Update: `skyHF.hlsli`, `skyAtmosphere.hlsli`, `voxelConeTracingHF.hlsli`
- **Phase 2**: Create `GGBindless.hlsli` — helper macros for descriptor index conversion, wrapping `bindless_textures2DArray[descriptor_index(field)]` patterns
- **Phase 2**: Create `GGLighting.hlsli` — factor the duplicated TiledLighting/ForwardLighting code from the 12 Tier 3 shaders into a single shared header
- **Phase 2**: Update `GGCommonFunctions.hlsli` — fix `ApplyFogCustom()` FrameCB field references
- **Phase 2**: Set up DXC compilation pipeline (fxc→dxc, SM 5.0→SM 6.0+, root signature embedding)
- Resolve the `CBSLOT_RENDERER_FORWARD_LIGHTMASK` b7→b2 conflict before any Tier 3 work begins

**Key files**: All 11 files in `Shaders/PBR/`, plus `GGCommonFunctions.hlsli`, new `GGBindless.hlsli`, new `GGLighting.hlsli`

**Critical constraint**: No other agent touches PBR/ headers. All PBR header questions route through this agent.

#### 3. C++ INTEGRATION ENGINEER — `general-purpose` agent

**Role**: Owns all C++ side changes. Modifies the engine integration layer so the DX12 backend correctly binds resources for the ported shaders.

**Responsibilities**:
- Investigate and document how shaders are currently compiled (build step vs runtime) — `GGTerrain_part0.cpp` (11,255 lines)
- Investigate descriptor heap management in `wiGraphicsDevice_DX12.cpp` — determine feasibility of Option C1 (bindless via CBs) vs Option C2 (expand SRV range)
- Implement the chosen texture binding strategy on the C++ side:
  - If C1: Add descriptor index fields to `TerrainCB`, `GrassCB`, `TreeCB` structs; store descriptor heap indices instead of calling `BindResource(slot, texture)`
  - If C2: Modify root signature SRV range and descriptor table setup
- Update pipeline state creation for new input layouts if needed
- Update constant buffer binding code in `GGTerrain_part0.cpp`, `GGGrass.cpp` (2,011 lines), `GGTrees_part0.cpp` (2,978 lines)
- Ensure ForwardEntityMaskCB binding matches whatever slot the Infrastructure Lead resolves it to

**Key files**: `GGTerrain_part0.cpp`, `GGTerrain_part1.cpp`, `GGGrass.cpp`, `GGTrees_part0.cpp`, `GGTrees_part1.cpp`, `GGTerrainFile.cpp`, and any Wicked Engine DX12 backend files (with permission)

**Critical constraint**: Every CB struct change must be coordinated with the corresponding shader agent to ensure byte-level layout agreement. The Quality Gate agent reviews all CB struct changes.

#### 4. TERRAIN SHADER PORTER — `general-purpose` agent

**Role**: Ports all 28 terrain shader files across all tiers.

**Responsibilities**:
- **Tier 1** (immediate, no dependencies): `GGTerrainShadowMapVS`, `GGTerrainPrepassRefVS`, `GGTerrainQuadVS`, `GGTerrainRampVS/PS`, `GGTerrainEditBoxVS/PS`, `GGTerrainSpherePrepassPS` — recompile with DXC + embed root signature, zero code changes
- **Tier 2A** (after PBR headers): `GGTerrainVS`, `GGTerrainPrepassVS`, `GGTerrainEnvProbeVS`, `GGTerrainOverlayVS/PS`, `GGTerrainPageGenVS`, `GGTerrainSphereVS`, `GGTerrainReadBackPS`, `GGTerrainQuadPS` — zero or minimal code changes
- **Tier 2B** (after texture infra): `GGTerrainReadBackVS` (b0 conflict), `GGTerrainReadBackCS`, `GGTerrainReadBackMSCS`, `GGTerrainPrepassPS` — bindless texture conversion
- **Tier 3** (after PBR headers + texture infra + GGLighting.hlsli): `GGTerrainPS` (326 lines, local ShaderEntity→load_entity()), `GGTerrainPageGenPS` (293 lines), `GGTerrainVirtualPS`, `GGTerrainVirtualPBR_PS` (424 lines, most complex shader), `GGTerrainEnvProbePS`, `GGTerrainSpherePS`

**Key challenge**: `GGTerrainPS.hlsl` has a local copy of `ShaderEntity` and direct `EntityArray : register(t20)` / `MatrixArray : register(t21)` declarations. Must migrate to `load_entity()`/`load_entitymatrix()` macros.

**Critical constraint**: Must not begin Tier 2B/3 work until Infrastructure Lead confirms PBR headers are finalized and C++ Integration Engineer confirms texture binding strategy.

#### 5. VEGETATION SHADER PORTER — `general-purpose` agent

**Role**: Ports all 24 grass and tree shader files across all tiers.

**Responsibilities**:
- **Tier 1** (immediate): `GGGrassShadowMapVS/PS`, `GGTreesShadowMapVS`, `GGTreesHighShadowMapVS`, `GGTreeBranchesHighShadowMapVS` — 5 shaders, zero code changes
- **Tier 2A** (after PBR headers): `GGGrassVS`, `GGGrassPrepassVS`, `GGTreesVS`, `GGTreesPrepassVS`, `GGTreesHighVS`, `GGTreesHighPrepassVS`, `GGTreeBranchesHighVS`, `GGTreeBranchesHighPrepassVS`, `GGTreesHighEnvProbeVS`, `GGTreeBranchesHighEnvProbeVS` — 10 shaders, zero code changes
- **Tier 2B** (after texture infra): `GGGrassPrepassPS`, `GGTreesShadowMapPS`, `GGTreesHighShadowMapPS`, `GGTreeBranchesHighShadowMapPS`, `GGTreesPrepassPS`, `GGTreesHighPrepassPS`, `GGTreeBranchesHighPrepassPS` — 7 bindless conversions
- **Tier 3** (after PBR + texture + GGLighting.hlsli): `GGGrassPS` (316 lines), `GGTreesPS`, `GGTreesHighPS` (272 lines), `GGTreeBranchesHighPS`, `GGTreesHighEnvProbePS`, `GGTreeBranchesHighEnvProbePS` — 6 full migrations
- Also owns: `GGGrassConstants.hlsli`, `GGTreesConstants.hlsli` — update FrameCB field references (`g_xFrame_TreeWind`, `g_xFrame_Time`)

**Critical constraint**: Same as Terrain Porter — no Tier 2B/3 until shared infrastructure is confirmed.

#### 6. QUALITY GATE (Devil's Advocate) — `general-purpose` agent

**Role**: Reviews every code change before it is committed. Catches register slot mismatches, CB layout drift, missing root signature embedding, and regression risks. Has VETO power — can block any commit.

**Responsibilities**:

**Pre-commit review for EVERY change** — no code merges without Quality Gate sign-off:
- Verify all register slot assignments match the root signature (t0-t15 only in descriptor table, s100-s109 for static samplers, etc.)
- Verify constant buffer struct layouts match byte-for-byte between C++ and HLSL (padding, alignment, field order)
- Verify `[RootSignature(WICKED_ENGINE_DEFAULT_ROOTSIGNATURE)]` is present on every shader entry point
- Verify no texture bindings above t15 remain (unless using bindless)
- Verify all sampler references use s0-s7 (on-demand) or s100-s109 (static), not the old s4-s13 slots
- Verify `#include` paths are correct for updated PBR headers
- Check for CRLF/tab consistency (project uses tabs + CRLF)

**Cross-agent consistency checks**:
- When C++ Integration Engineer changes a CB struct, verify the corresponding HLSL `cbuffer` declaration matches
- When Infrastructure Lead updates a PBR header, verify all dependent shaders still compile
- When a Shader Porter uses `load_entity()` or `bindless_textures2DArray[]`, verify the access pattern matches the latest engine API

**Build validation**:
- After each milestone, request a build via `build.bat` and verify zero errors/warnings
- At milestones 4 and 7, coordinate with the Coordinator to run the Full Demo FPS Test (19 demos, ~20 min)
- Compare FPS results against the baseline table in WETEST.md — flag any demo with >10% regression

**Adversarial review questions** (ask these for every Tier 3 shader change):
1. "What happens if this shader is loaded but the C++ side hasn't been updated yet? Will it crash or silently render wrong?"
2. "Is there a frame where the old and new constant buffer layouts could coexist? Could that cause a device removed error?"
3. "If this bindless descriptor index is wrong/stale, what's the failure mode — black screen, crash, or garbage?"
4. "Does this change preserve the exact same visual output as the DX11 version, or does it intentionally change rendering?"
5. "Are there any static_assert or compile-time checks we can add to catch CB layout drift?"

**Key authority**: The Quality Gate agent can request any other agent to:
- Re-read a file before editing (enforce CLAUDE.md rule #1)
- Add a `static_assert(sizeof(TerrainCB) == EXPECTED_SIZE)` to C++ code
- Add comments explaining non-obvious register slot choices
- Revert a change if it introduces a risk that wasn't discussed with the Coordinator

### Parallelism and Dependency Map

```
Phase 1 (parallel):
  Infrastructure Lead: PBR headers ──────────────────────────────┐
  C++ Integration: Investigate shader compilation & descriptors ─┤
  Terrain Porter: Tier 1 (8 shaders, zero changes) ──────────────┤
  Vegetation Porter: Tier 1 (5 shaders, zero changes) ───────────┤
  Quality Gate: Review all Tier 1 changes ───────────────────────┘

Phase 2 (after PBR headers done):
  Infrastructure Lead: GGBindless.hlsli + GGLighting.hlsli + DXC pipeline ──┐
  C++ Integration: Implement texture binding strategy ──────────────────────┤
  Terrain Porter: Tier 2A (10 shaders) ─────────────────────────────────────┤
  Vegetation Porter: Tier 2A (10 shaders) ──────────────────────────────────┤
  Quality Gate: Review all changes, verify builds ──────────────────────────┘

Phase 3 (after texture infra done):
  Terrain Porter: Tier 2B (4 shaders) ──────────────────────────────────────┐
  Vegetation Porter: Tier 2B (7 shaders) ───────────────────────────────────┤
  Quality Gate: Review bindless conversions ────────────────────────────────┘

Phase 4 (after GGLighting.hlsli done — this is the hardest phase):
  Terrain Porter: Tier 3 (6 shaders, including GGTerrainVirtualPBR_PS) ─────┐
  Vegetation Porter: Tier 3 (6 shaders) ────────────────────────────────────┤
  Quality Gate: Intensive review, adversarial questions on every change ────┤
  C++ Integration: Support any CB struct adjustments discovered during T3 ──┘

Phase 5 (validation):
  Quality Gate: Full Demo FPS Test (19 demos)
  Coordinator: Final status report
```

### Communication Protocol

1. **All CB struct changes** require a message to BOTH the corresponding shader agent AND the Quality Gate, with the exact struct layout in bytes
2. **All register slot changes** require a message to the Quality Gate with before/after slot numbers
3. **Build failures** are reported to the Coordinator immediately — the Coordinator decides whether to pause other agents
4. **The Quality Gate may issue a HOLD** on any agent's work — the agent must stop and wait for resolution
5. **Phase transitions** (e.g., Phase 1→2) require explicit Coordinator approval after Quality Gate confirms all Phase N work is clean

### Risk Mitigations

| Risk | Mitigation | Owner |
|------|-----------|-------|
| PBR header update breaks everything downstream | Infrastructure Lead completes headers in isolation; Quality Gate compiles a test shader against new headers before Phase 2 begins | Infrastructure Lead + Quality Gate |
| CB layout mismatch between C++ and HLSL | Quality Gate reviews every CB change pair; add `static_assert` on C++ side | Quality Gate |
| Bindless descriptor indices point to wrong textures | C++ Integration Engineer adds debug validation (descriptor index bounds checking in debug builds) | C++ Integration + Quality Gate |
| b7→b2 conflict silently corrupts rendering | Infrastructure Lead resolves this in Phase 1; Quality Gate verifies the resolution propagates to all 3 env probe shaders | Infrastructure Lead + Quality Gate |
| Regression in demo FPS after porting | Full Demo FPS Test at milestones 4 and 7; >10% regression triggers investigation | Coordinator + Quality Gate |
| Agent edits wrong file or wrong lines | Enforce CLAUDE.md rule: always read target lines immediately before editing; Quality Gate spot-checks | All agents |
