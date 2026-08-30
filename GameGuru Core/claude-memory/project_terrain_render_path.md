---
name: project-terrain-render-path
description: "Which terrain renderer is LIVE in GameGuru MAX DX12: the shipping/visible terrain is WICKED'S NATIVE SVT path, NOT GGTerrain's custom virtual-texture draw. ggterrain_use_wicked_terrain=1 (default, GGTerrain_part0.cpp:4209) makes MasterRenderer::Update set ggterrain_draw_enabled=0 (master_part1.cpp:249-259), so GGTerrain_Draw early-returns (GGTerrain_part0.cpp:11177, exitReason 3) and the master_part1.cpp customDraw callbacks all `if (ggterrain_use_wicked_terrain) return;`. Therefore GGTerrainVirtualPBR_PS.hlsl, the GGTERRAIN_TEXTURE_FILTERING macro, and GGTerrainPageSettings.h are DEAD CODE for the visible terrain — do NOT change them to affect terrain rendering. The real terrain material/sampler/filtering live in ENGINE wiTerrain.cpp. Confirm empirically with harness GET_PERF_DATA -> TERRAIN_DRAW_EN 0 / TERRAIN_DRAW_COUNT 0."
metadata:
  node_type: memory
  type: project
  originSessionId: 63b96d44-9243-46fb-bc37-9eb4d4be60b9
  modified: 2026-07-24T05:20:36.780Z
---

# Which terrain renderer is LIVE (GGTerrain custom VT vs Wicked native SVT)

**The visible terrain in GameGuru MAX DX12 is rendered by WICKED'S NATIVE SVT terrain path**
(`GGTerrainWicked_Update` builds standard MaterialComponents the engine renders via the generic
object/SVT pipeline). GGTerrain's own custom virtual-texture draw is **gated off** and does not run.

**The gate (verified in code + at runtime):**
- `ggterrain_use_wicked_terrain = 1` by default (GGTerrain_part0.cpp:4209).
- `MasterRenderer::Update` (master_part1.cpp:249-259): when that flag is set → `ggterrain_draw_enabled = 0`
  ("suppress all old draw callbacks") and it calls `GGTerrainWicked_Update(camera)` instead.
- `GGTerrain_Draw` (GGTerrain_part0.cpp:11167) bails at :11177 `if (!ggterrain_draw_enabled) return;` (exitReason 3).
- Every terrain customDraw callback in master_part1.cpp:169-197 begins `if (ggterrain_use_wicked_terrain) return;`.
- Runtime proof: harness `GET_PERF_DATA` → `TERRAIN_DRAW_COUNT: 0`, `TERRAIN_DRAW_EN: 0`, `TERRAIN_UPDATE_EN: 1`.
  (Y-key debug toggle at GGTerrain_part0.cpp:9734 flips the flag — the ONLY way GGTerrain_Draw runs.)

**Consequence — DEAD CODE for the visible terrain (do NOT edit these to change terrain rendering):**
- `GGTerrainVirtualPBR_PS.hlsl` / `GGTerrainVirtualPS.hlsl` (the custom terrain pixel shaders).
- `GGTERRAIN_TEXTURE_FILTERING` macro + all of `GGTerrainPageSettings.h` (page padding, physTexSize…).
- `samplerAnisotropicWrap` / `samplerBilinear` in GGTerrain_part0.cpp (~7353-7361, bound at ~11207).

**Where the REAL terrain rendering lives = ENGINE `D:\max\WickedEngineDX12\WickedEngine\wiTerrain.cpp`:**
- Terrain chunk sampler created there (Filter::ANISOTROPIC) and assigned to each chunk material's
  `sampler_descriptor`; flows material→cached_wrapSampler (wiScene.cpp)→push.wrapSamplerIndex
  (wiRenderer.cpp)→`sampler_objectshader` (objectHF.hlsli) which samples baseColor/normal/surface.
- **Terrain LOD/filtering is driven by `SVT_MIP_BIAS` + `get_lod(GetAnisotropy())` in
  `shaders/ShaderInterop_Renderer.h`, NOT by the wiTerrain sampler.** The SVT fetches the physical atlas via
  `SampleLevel(sampler, atlas_uv, 0)` (explicit LOD) → the sampler's anisotropy AND mip_lod_bias are IGNORED.
  **Swim fix = [[project-wicked-engine-changes]] delta 1.28 (`fd4a0399`): `SVT_MIP_BIAS` −2.0 → 0.0** (stock
  −2.0 biases terrain 2 mips SHARPER than correct → mid/long "swim"; 0.0 = Nyquist-correct). Tune toward +1.0
  = more distance stability (foreground softens), toward −2.0 = crisper + swim. The earlier ×4→×8 sampler
  change (delta 1.27, `3a6fc38c`) was a **confirmed no-op** (SampleLevel ignores sampler aniso) and was
  REVERTED — a lesson: confirm the sampling method (`Sample` vs `SampleLevel`) before touching sampler state.

**LESSON (why this note exists):** a multi-agent workflow's adversarial verify-agent confidently REFUTED
the correct finding and claimed GGTerrain_Draw was the live path — it read the sampler bind at 11207 but
missed that `ggterrain_draw_enabled=0` early-returns the whole function. The `TERRAIN_DRAW_EN`/`_COUNT`
harness counters settled it in seconds. **Always confirm the live terrain path empirically before editing
terrain shaders/samplers.** Related: [[project-terrain-editing]] (Wicked-mode sculpt/paint dispatch),
[[project-vt-zoom-squares]] (the engine VirtualTexture this path streams through).
