# Wicked Engine Side Changes — Brief for Upstream Author

This document tracks every change we have made to the `WickedEngineDX12`
clone at `D:\max\WickedEngineDX12` while porting GameGuru MAX to Wicked,
from the point this log began. Engine changes made during the original
port, before this log existed, are summarised in section 1.0 below.

Two categories:

1. **Genuine bug fixes** — should be considered for upstream merge.
2. **Temporary debug overrides** — active during analysis; must be reverted
   before any test is treated as final and before any upstream brief.

The WickedEngineDX12 repo is a separate git repo from GameGuruMAXDX12.
Tracking the changes here (inside GameGuru Core) so they survive in source
control even if the Wicked clone is reset or re-cloned.

---

## 1.0 Pre-existing port-era engine deltas (before this log began)

The clone already carried GG-specific engine modifications from the
original DX12 port before entry 1.1 was written. Pointers only, not
full diffs:

- **RenderPath3D `customDraw` function-pointer hooks** — Wicked commit `9118befe`
- **`customDraw_ShadowMap` / `customDraw_EnvProbe` hooks** — Wicked commits `0a8ea4fb`, `b84ff4fe`
- **SRV count 16 → 64 + sampler expansion**
- **`GetDX12Device` / `GetDX12GraphicsCommandList` accessors**
- **/MTd runtime change**

Full details in MIGRATION_PLAN.md and DX11_to_DX12_Shader_Porting_Plan.md
§13.9. WITHOUT these, a re-cloned WickedEngineDX12 will not render at
all — this doc alone is not sufficient to restore the clone.

## 1. Bug fixes (candidates for upstream)

### 1.28 Terrain: soften SVT mip bias −2.0 → 0.0 (fix mid/long pixel-swim) — supersedes 1.27

**Files:**
- `WickedEngine/shaders/ShaderInterop_Renderer.h` (`SVT_MIP_BIAS` −2.0 → 0.0)
- `WickedEngine/wiTerrain.cpp` (revert 1.27's no-op sampler `max_anisotropy` 8 → 4)

**Why:** the terrain is Wicked's virtual-textured (SVT) terrain, which computes its own LOD in software — `virtual_lod = get_lod(dim, ddx(uv), ddy(uv), GetAnisotropy()) + SVT_MIP_BIAS`, then fetches the physical atlas via `SampleLevel(sampler, atlas_uv, 0)` (explicit LOD). Stock Wicked sets `SVT_MIP_BIAS = -2.0` — biasing terrain TWO mip levels sharper than the Nyquist-correct LOD for crispness. That over-sharpening under-samples the ground at mid/long range → it "swims"/shimmers as the camera creeps. **Because the atlas fetch is `SampleLevel` (explicit LOD), the material sampler's anisotropy AND mip_lod_bias are IGNORED — so 1.27's ×4→×8 sampler bump was a confirmed visual no-op (reverted here).** The real terrain LOD lever is `SVT_MIP_BIAS`, which biases the streaming FEEDBACK and the SAMPLE together (in sync). Set to 0.0 = Nyquist-correct: mip-based swim can't occur, near field stays crisp.

**Behaviour:** affects ALL SVT sampling (terrain is GG's only SVT user). 0.0 vs stock −2.0 = terrain ~2 mips softer at a given distance but stable; near field near-correct (crisp). User-tuned on TESTPRO1 (A/B'd −1.0/0.0/+1.0 and a +4.0 extreme that confirmed the lever); picked 0.0. Shared header — a GAME rebuild (or engine relaunch) recompiles the ~375 engine shaders that include it (`refresh_shaders` handles staleness). Tune toward +1.0 for more distance stability (foreground softens too), toward −2.0 for more crispness (swim returns).

### 1.27 Terrain: raise Wicked SVT terrain anisotropy ×4 → ×8 (fix grazing pixel-swim) — REVERTED (no-op, see 1.28)

**Files:**
- `WickedEngine/wiTerrain.cpp` (the terrain SVT sampler in the material-update loop: `samplerDesc.max_anisotropy = 4` → `8`, assigned to every terrain chunk material's `sampler_descriptor`)

**Why:** GameGuru's shipping terrain renders through Wicked's NATIVE SVT terrain path — the GGTerrain custom-draw path is gated OFF: `ggterrain_use_wicked_terrain = 1` (default) forces `ggterrain_draw_enabled = 0` in MasterRenderer::Update (master_part1.cpp), so `GGTerrain_Draw` early-returns and never textures anything (confirmed live via harness `TERRAIN_DRAW_EN 0` / `TERRAIN_DRAW_COUNT 0`). **So the entire GGTerrain page-cache / GGTERRAIN_TEXTURE_FILTERING machinery is dead code on the shipping image — the real terrain sampler is this one in wiTerrain.cpp.** Stock Wicked creates it as anisotropic ×4, but every OTHER surface in the scene samples at ×16 (SAMPLER_OBJECTSHADER). Terrain is the surface most often viewed near edge-on, and ×4 only resolves a 4:1 minified footprint — at grazing angles the true per-pixel footprint far exceeds that, so the minor axis is under-sampled and the ground "swims" as the camera creeps. ×8 doubles the resolved footprint (8:1), covering the bulk of near-ground horizon views, and also sharpens the normal map (same sampler) reducing specular shimmer.

**Behaviour:** sampler-state only, fully reversible, affects only terrain-chunk materials (and decals that reuse the base material's sampler — a harmless quality bump). No VRAM change, no shader recompile (bindless sampler read at runtime). ×8 stays within the 4-texel SVT tile border (`SVT_TILE_BORDER`) so aniso taps don't spill across page edges — no tile seams (verified on the TESTPRO1 grazing view). ×16 (matching the rest of the scene) is the escalation if grazing is still soft, but at ×16 the kernel can reach past the 4px border at extreme grazing (faint tile-seam risk) unless the border is also widened; ×8 is the safe default. Fixes the user-reported "a lot of pixel swim on the terrain when moving the camera slowly." NOTE for future work: do NOT chase the GGTERRAIN_TEXTURE_FILTERING macro or GGTerrain_part0.cpp samplerAnisotropicWrap for terrain filtering — that path does not render.

### 1.26 Ocean: env-map reflection fallback when planar reflections are OFF (fix garbage-water corruption)

**Files:**
- `WickedEngine/wiRenderPath3D.cpp` (`RenderPath3D::PreRender()` — guard the main camera's `texture_reflection_index` / `texture_reflection_depth_index` writes with the SAME condition that gates the planar reflection render below, `getReflectionsEnabled() && visibility_main.IsRequestedPlanarReflections()`; force both to -1 in the else)

**Why:** the DX12 ocean PS (`oceanSurfacePS.hlsl:63`) branches on `texture_reflection_index >= 0` — planar reflection when set, else the stable `EnvironmentReflection_Global` sky/global-probe fallback. But `PreRender` wrote that index UNCONDITIONALLY every frame from `rtReflection_resolved`, with no reflections-enabled guard (unlike the adjacent VXGI-specular field, which IS guarded). When the user unticks "Reflections", `setReflectionsEnabled(false)` frees `rtReflection`/`depthBuffer_Reflection` but NOT `rtReflection_resolved`/`depthBuffer_Reflection_resolved`, so `GetDescriptorIndex` kept returning a valid index >= 0 while the planar render that refills that texture was gated off — the ocean took the planar branch and sampled a stale/uninitialised texture = bright blocky garbage on the water. DX11 never hit this: its ocean sampled a fixed slot and the renderer bound `wiTextureHelper::getTransparent()` when reflections were off — a safety net the DX12 bindless-index rewrite dropped. The guard restores it: the index becomes the -1 sentinel the shader keys on, so the ocean uses its env-map fallback (reflect the sky/probe) — stable and ~free (skips the whole planar scene re-render).

**Behaviour:** reflections ON is byte-identical — the guard condition is the exact same as the planar-render gate, so whenever the mirror is actually rendered the same index is written as before. Reflections OFF now shows env-map water (sky/probe reflection, matching the DX11 look) instead of garbage, AND is cheaper: measured on the TESTPRO1 island editor, OFF 57.9 FPS vs ON 51.3 FPS (skips the planar scene re-render). Also closes a latent case where reflections are enabled but no planar reflection is requested that frame (the index would otherwise point at the stale resolved texture). C++-only — no shader recompile. Fixes the user-reported "unticking reflection corrupts the water". Harness A/B lever: `SET_REFLECTIONS 0|1` (game `AutomationHarness.cpp`).

### 1.25 Hair particles: skip billboard-write + physics for non-drawn (culled) strands

**Files:**
- `WickedEngine/shaders/hairparticle_simulateCS.hlsl` (early-out after the wave-atomic index append: `if (!visible && !regenerate_frame) return;`)

**Why:** the hair/grass simulate compute shader ran the full per-strand billboard vertex writes + physics integration for EVERY strand, even those culled from the draw (distance/frustum) — their output is never referenced by the culled index buffer, so it's pure wasted GPU work. The early-out bails for non-visible strands. The wave-atomic index append above it still runs for every lane (wave-safe), and `regenerate_frame` strands never bail (they must seed sim state + build the static primitive buffer). **Perf/upstream-worthy, not a GG behaviour change.**

**Behaviour:** drawn (visible) strands are byte-identical, so the rendered grass is unchanged; a culled strand's sim state freezes until it re-enters view (imperceptible under low wind, none for a static camera). Measured on the TESTPRO1 editor: grass sim 52.0 → 48.3 ms (the dominant cost is the ~905K simulated strand count itself, which is density-bound). Saves proportional to the off-screen strand fraction — more when zoomed into a small edit area.

### 1.24 Ocean: "Water Base Color" tints transparent water (depth-based surface tint)

**Files:**
- `WickedEngine/shaders/ShaderInterop_Ocean.h` (`OceanCB::xOceanWaterColorDepth` + 3 pad floats)
- `WickedEngine/wiOcean.h` (`OceanParameters::water_color_depth` default 0 = stock)
- `WickedEngine/wiOcean.cpp` (`GetOceanCBAtDim` fills `xOceanWaterColorDepth` from the param)
- `WickedEngine/shaders/oceanSurfacePS.hlsl` (lerps `surface.refraction.rgb` toward `xOceanWaterColor.rgb` by `saturate(abs(water_depth) * xOceanWaterColorDepth)`, right after the extinction/transmittance step)

**Why:** on the ocean SURFACE the base colour (`xOceanWaterColor.rgb`) only reaches the eye through opacity — `surface.albedo` blended by `refraction.a = exp(-water_depth * color.a)` and a depth-tinted reflection, all gated by `color.a` (WaterAlpha). GameGuru's water runs at **WaterAlpha 0** (fully transparent, so you can see the seabed), which means the base colour contributes essentially nothing — setting "Water Base Color" to red left the sea unchanged from above. This is the ocean CB used by `oceanSurfacePS` (NOT the `ShaderOcean` in ShaderInterop_Weather.h; that one feeds underwaterCS/lighting). `water_color_depth` tints the see-through refraction toward the base colour with depth, alpha-independent — clear shallows, coloured depths, like real water absorbing light — so the colour is visible on transparent water. Tint scales with the colour's own saturation (a subtle blue tints gently, a bold colour reads strongly).

**Behaviour:** no stock behaviour change — `water_color_depth` defaults to 0, which skips the lerp (`saturate(0) == 0`). GG sets 0.005 (M-GridEditB_part3.cpp) for all water-enabled levels, so existing GG water now shows its base colour with depth (an improvement — most water_color values are sensible blues; a level that relied on the colour being invisible will now show it). Paired with the same commit's picker fix (the Water Base Color picker preserves WaterAlpha instead of forcing 255), so changing the colour no longer forces the surface opaque OR leaves it colourless — it stays transparent AND takes the colour.

### 1.23 Ocean: decouple the underwater view (fog colour + density) from the water surface

**Files:**
- `WickedEngine/shaders/ShaderInterop_Weather.h` (`ShaderOcean::underwater_fog_density` reuses the `caustic_pad0` slot; new `ShaderOcean::underwater_color` float4)
- `WickedEngine/wiOcean.h` (`OceanParameters::underwater_fog_density` default 0 = stock; `underwater_color` default deep blue-green)
- `WickedEngine/wiScene.cpp` (marshals both into `ShaderOcean`, next to `caustic_scale`)
- `WickedEngine/shaders/underwaterCS.hlsl` (computes `uw_density`/`uw_color` with a stock fallback; uses them for `waterfog`, `transmittance` and the waterline intersection band instead of `water_color.a`/`water_color.rgb`)

**Why:** the `underwaterCS` post-process (the submerged fog/tint/extinction) took its colour from `water_color.rgb` and its density from `water_color.a` — the SAME values that drive the water SURFACE (`oceanSurfacePS`). So the underwater look could not be tuned independently, and — worse — GameGuru's "Water Base Color" picker, on any edit, wrote `WaterAlpha = 255` (it read the picker alpha as a hard-coded 1.0), which via `water_color.a` turned the surface fully opaque/solid ("lost all transparency"). Decoupling lets the surface keep transparent water while the submerged view fogs to its own colour/distance.

**Behaviour:** no stock behaviour change — `underwater_fog_density` defaults to 0, and the shader then falls back to the original `water_color.a` / `water_color.rgb` path exactly. GG sets density > 0 (from the new **"Underwater Fog"** slider) and **"Underwater Color"** picker, and also fixes the surface picker to preserve alpha so changing the water colour no longer forces it opaque. Density scale tuned on the TESTPRO1 deep-water vista (slider 0..100 -> density 0..0.015; default 20 -> 0.003 shows seabed + caustics with a natural blue depth fade).

### 1.22 Ocean: decouple seabed caustic size from the wave patch size

**Files:**
- `WickedEngine/shaders/ShaderInterop_Weather.h` (`ShaderOcean::caustic_scale` + 3 pad floats)
- `WickedEngine/wiOcean.h` (`OceanParameters::caustic_scale` default 1.0 = stock)
- `WickedEngine/wiScene.cpp` (marshals `ocean.caustic_scale` into `ShaderOcean`, next to `patch_size_rcp`)
- `WickedEngine/shaders/lightingHF.hlsli` (the `texture_caustics` seabed lookup samples `ocean_uv * ocean.caustic_scale`; the displacement / water-height lookup deliberately keeps the UNscaled `ocean_uv`)

**Why:** the underwater "caustic" light ripples on the sea floor are the `texture_caustics` term inside `light_directional`, sampled at `surface.P.xz * ocean.patch_size_rcp`. Because that UV is tied to `patch_length`, the only in-editor control over caustic size was **Water Tiling Patch Size** — which ALSO drives the wave FFT, so the ripples could not be enlarged without blowing the waves up to absurd size. `caustic_scale` multiplies ONLY the caustic lookup, so ripple size is now independent of wave size. GG drives it from a new Water-panel **"Caustic Size"** slider (`visuals.fWaterCausticSize`, scale = 1/size). Measured, patch length pinned: 1.0 = fine stock speckle, 0.3 = larger cells, 0.1 = broad soft blobs.

**Behaviour:** no stock behaviour change — `caustic_scale` defaults to 1.0, which samples identically to before.

**Diagnostic note (this cost hours):** the terrain's LIVE seabed caustics are in THIS engine `lightingHF.hlsli`, NOT the game copy `GGTerrain/Shaders/PBR/lightingHF.hlsli`. `GGTerrainVirtualPBR_PS.hlsl` comments out `ForwardLighting` and calls `GGTiledLightingWithAmbient` (GGLighting.hlsli), whose `light_directional` resolves here — so the game's `lightingHF.hlsli` is DEAD CODE for terrain. Earlier caustic edits made to the game copy (commits `14b90e0a` / `565f704b`) never ran; a follow-up should strip that dead plumbing (GGCustomFrameCB `causticScale`, GGFrameCompat macro, the game `lightingHF` block).

### 1.21 VT: expanded working set — island-wide full-res zone + removal margin

**Files:**
- `WickedEngine/wiTerrain.h` (`Terrain::gg_near_ring_dist` default 2 = stock; `Terrain::gg_removal_margin` default 0 = stock)
- `WickedEngine/wiTerrain.cpp` (`required_resolution = dist < gg_near_ring_dist ? max : min`; `removal_threshold = generation + 2 + gg_removal_margin`)

**Why:** the user's "bigger buffer" insight, translated. The residual violent-zoom squares came from the full-resolution zone being only ±2 chunks (~10k inch-units) — a fast camera crosses it in milliseconds, and every crossing is a VT cache re-reference (re-init, residency reset, re-stream) no heuristic can fully hide. The atlas itself cannot grow (16384² is the D3D12 texture-dimension limit) but it never needed to: tile residency is feedback-driven (≈ what's on screen), so widening the zone costs almost no physical tiles — only per-chunk residency structures. GG sets the zone to ±6 chunks: the whole island lives INSIDE it, so camera travel over the island never crosses a resolution boundary and near/correct tiles are never re-referenced — only genuinely new far detail streams in. The removal margin (+12) keeps chunks alive across zoom travel, eliminating destroy/recreate cache flushes.

**Behaviour:** no stock behaviour change — both fields default to stock values.

### 1.20 VT: tile allocator freeze while the camera is in motion

**Files:**
- `WickedEngine/wiTerrain.cpp` (`UpdateVirtualTexturesCPU`: while `gg_center_stable_frames < 10` — tile aging (free_frames++) suspended; the reusable-tile list only admits tiles unused for 60+ frames (provably off-screen) instead of 1+; the free list entering the chunk loop is filtered the same way so chunk-init tail allocations can't steal either)

**Why:** the final piece of the violent-zoom "random squares". The GPU feedback that keeps a visible tile alive arrives on a 2-3 frame delayed readback; during fast camera moves the keep-alive gaps let STILL-VISIBLE, correct tiles age into the free list, and the flood of misses from newly-visible terrain then steals them — another chunk renders its pixels into a square that was correct and on screen. Now, while the camera is crossing chunk boundaries, nothing recently used can be recycled: correct terrain keeps every tile unconditionally, and newly approached terrain streams from the genuinely stale pool (or stays coarse if it drains — "under construction", exactly the acceptable behavior). Full streaming resumes the moment the camera settles; dropped requests cost nothing since the GPU re-requests missing pages every frame.

**Behaviour:** gated by `gg_vt_upgrade_hysteresis` (default false = stock; the threshold falls back to stock `>= 1` when the camera is stable or the flag is off).

**HOTFIX (same day):** the first version ALSO froze tile aging (`free_frames++`) while the camera moved — fatal under sustained motion: nothing ever crossed the recycle threshold, the pool starved, chunk inits failed to allocate their tail tiles and (stock has no retry path) the whole terrain degraded to flat untextured squares within ~2 minutes of continuous zooming. Aging now always runs — the 60-frame threshold alone is the protection (visible tiles oscillate at 0-3 frames via keep-alives, unreachable; off-screen tiles keep flowing into the pool). Added `gg_tail_invalid` self-heal: a chunk whose tail-tile allocation failed re-inits automatically once the pool has tiles again (fixes stock's silent no-retry fragility on pool exhaustion too).

### 1.19 VT: stale tile-identity release + full freeze-while-moving

**Files:**
- `WickedEngine/wiTerrain.h` (`VirtualTexture::free`: releases `physical_tiles[].last_used` ownership by identity before clearing the tiles vector; 1.18 flag comment updated)
- `WickedEngine/wiTerrain.cpp` (`UpdateVirtualTexturesCPU`: the 1.18 defer logic now freezes BOTH directions — downgrades as well as upgrades — while the camera is crossing chunk boundaries; when stable, downgrades run immediately and upgrades stay budgeted)

**Why (part 1, the real bug — stock, upstream-worthy):** `PhysicalTile::last_used` stores a raw pointer into the owning `VirtualTexture::tiles` vector and `check_tile_resident` compares by POINTER IDENTITY. `free()` cleared the vector without releasing those back-pointers, and a later `tiles` allocation (fast zoom in/out = min→max→min re-inits with identical vector sizes) frequently reuses the same heap block — the stale `last_used` then matches a brand-new tile by address. Consequences: the page table keeps mappings to physical tiles that were freed and recycled to OTHER chunks, and `request_residency` skips the re-render because the tile "is already resident" — random squares of another chunk's pixels, healing gradually as GPU feedback re-requests. Fixed by nulling matching `last_used` entries in `free()` before `tiles.clear()`.

**Why (part 2):** 1.18 froze only upgrades; downgrades still re-initialized mid-motion, churning the pool and (via part 1) seeding stale identities. Now no VT reference changes at all while the camera is moving — chunks render their existing correct tiles; the ring re-balances only after the camera settles.

**Behaviour:** part 1 is an unconditional correctness fix (stale-identity sampling was never intended); part 2 stays behind `gg_vt_upgrade_hysteresis` (default false = stock).

### 1.18 Terrain: VT residency upgrade hysteresis

**Files:**
- `WickedEngine/wiTerrain.h` (`Terrain::gg_vt_upgrade_hysteresis` + `gg_prev_center_chunk`/`gg_center_stable_frames`)
- `WickedEngine/wiTerrain.cpp` (`Generation_Update`: center-chunk stability counter; `UpdateVirtualTexturesCPU`: residency UPGRADES (min→max when a chunk enters the dist<2 near ring) deferred until the camera holds one chunk for 10 frames, then budgeted at 4 per frame)

**Why:** the near ring is only ±2 chunks; a fast camera zoom sweeps that boundary across the island and every chunk crossing far→near called `vt.init()` mid-motion — residency reset, tail-mip rendering, detail tiles landing over several GPU-feedback frames = square tiles of mixed sharpness (and the odd recycled tile showing a neighbour's old pixels for a frame) flickering while the camera moved, calming on stop. With hysteresis, crossing chunks keep rendering their existing correct low-res tile (soft, stable); once the camera settles, the ring sharpens over a few frames. Downgrades (frees tile-pool memory), fresh chunks (resolution 0) and unbound-material chunks (in-place regen rebinds) are never deferred.

**Behaviour:** no stock behaviour change — flag defaults false.

### 1.17 Terrain: gg_generate_blendmap — chunks born with game-correct blendmaps

**Files:**
- `WickedEngine/wiTerrain.h` (`Terrain::gg_generate_blendmap` callback + `ChunkData::gg_blendmap_generated`)
- `WickedEngine/wiTerrain.cpp` (`Generation_Update` regen branch: after the chunk's vertex data completes and before `CreateChunkRegionTexture`, non-preserved chunks call the callback; its success is recorded in `gg_blendmap_generated`)

**Why:** Wicked streams chunks — beyond `removal_threshold` the whole ChunkData is erased, and a fast camera zoom re-creates those chunks from scratch. They were born with the ENGINE-default region weights (grass-heavy `region_base`) and rendered that for at least one frame — several during bursts — until GG's main-thread blend passes rewrote them: chunk-shaped green default-blend squares flickering on sand during quick zooms, calming when the camera stopped. With the callback, the generator thread fills GG's DX11-style auto weights + painted overrides BEFORE the region texture is built, so the first pixel a streamed chunk ever renders is already correct. The game latches `gg_blendmap_generated` chunks in both passes (keys only, no rewrite, no VT churn) and clears the flag from the edit bridge so real edits reprocess. Callback returning false (e.g. GG materials not set up during initial level load) falls back to engine-default weights + the bulk passes — load behavior unchanged.

**Behaviour:** no stock behaviour change — the callback is null by default.

### 1.16 VT repaint flag: main-thread latch (lost-update race fix)

**Files:**
- `WickedEngine/wiTerrain.h` (`VirtualTexture::gg_repaint_blendmap_latched`)
- `WickedEngine/wiTerrain.cpp` (`UpdateVirtualTexturesCPU`: the main-thread pickup consumes `pending_repaint_blendmap` — clears it, sets the latch, re-binds `vt.blendmap`; the async job's repaint loop consumes the LATCH instead of the live flag)

**Why:** found by adversarial multi-agent review of 1.15 (three independent reviewers converged on it; 8/9 verifiers confirmed against the code). The 1.13 flag had two unsynchronized consumers: the main-thread re-bind (runs inside Generation_Update BEFORE the async `virtual_texture_ctx` job launches) and the job's repaint loop, which blind-cleared the live flag. GG's blend passes run AFTER Generation_Update in the same frame and set the flag while that frame's Low-priority job may still be queued or running (the Low pool is THREAD_PRIORITY_LOWEST and starves under load). If the job consumed a freshly-set flag, it re-rendered tiles against the OLD `vt.blendmap` binding and dropped the request — the edit never landed, and the chunk stayed visually pre-edit until the next edit touched it. Mid-drag the bridge's key-erase re-fires the pass so it self-healed; the FINAL settle of a drag/paint burst had no retry — and 1.15 routes exactly that one-shot settle through this path. Ownership after the fix: game code only SETS the live flag; the main-thread block (previous job already joined at function top) transfers it to the latch; only the job clears the latch. Delivery timing unchanged (next frame).

**Behaviour:** no stock behaviour change — both flags are passive unless a consumer sets them.

### 1.15 Terrain: preserve blendmap + VT residency across in-place chunk regen

**Files:**
- `WickedEngine/wiTerrain.h` (`Terrain::gg_preserve_blendmap_on_regen`, default false = stock)
- `WickedEngine/wiTerrain.cpp` (`Generation_Update`: regen branch skips the blendmap_layers resize/overwrite and the GPU `blendmap = {}` reset when the flag is up and the old chunk has usable layers; merge epilogue skips `vt->invalidate()` for removable/regenerated chunks; `UpdateVirtualTexturesCPU`: `gg_material_rebind` detects the freshly-merged bindingless MaterialComponent and re-runs the atlas bind block against the EXISTING vt without `vt.init()`)

**Why:** sculpting in GG regenerates the brush's chunks in place EVERY drag frame. Stock regen (a) overwrites the chunk's blendmap layers with engine-default base/slope/altitude region weights — clobbering GG's DX11-style multi-layer blendmap and flashing wrong textures — and (b) `vt->invalidate()` in the merge epilogue resets the sparse VT's residency, so all tiles re-stream through multi-frame GPU feedback round-trips = chunk-shaped blur/checker for the whole duration of a sculpt drag, healing only after mouse release. With the flag up, only the mesh regenerates; blendmap layers, the GPU blendmap texture and VT residency all survive. The one wrinkle: the merge still replaces the chunk's MaterialComponent with a fresh (bindingless) one — `gg_material_rebind` re-binds it to the surviving vt in the same frame, no reset. GG's blend passes rewrite the weights right after regen anyway (the bridge erases the processed keys), delivered via the 1.13 resident-tile repaint.

**Behaviour:** no stock behaviour change — flag defaults false.

### 1.14 Terrain: optional generation restart on dirty materials

**Files:**
- `WickedEngine/wiTerrain.h` (`Terrain::generation_restart_on_dirty_materials`, default true = stock)
- `WickedEngine/wiTerrain.cpp` (`Generation_Update` material-dirty check guarded by the flag)

**Why:** stock Wicked restarts generation (full chunk teardown + rebuild) whenever any terrain material component is dirty — an editor convenience for material parameter tweaks. GG registers painted-material blendmap slots at RUNTIME (incremental, no restart), and the freshly-created material is dirty for one frame until the scene update launders it. Whether the restart fired depended on frame ordering: real editor paint strokes (GGTerrain_Update runs before GGTerrainWicked_Update in the same frame) hit it; harness strokes did not — a 4-5s full-terrain flicker on the first stroke with each new texture that only reproduced by hand. GG sets the flag false at terrain init: GG owns its blendmaps, material dirt must never rebuild the island.

### 1.13 VirtualTexture::pending_repaint_blendmap — instant blendmap-edit refresh

**Files:**
- `WickedEngine/wiTerrain.h` (`VirtualTexture::pending_repaint_blendmap` flag)
- `WickedEngine/wiTerrain.cpp` (`UpdateVirtualTexturesCPU`: main-thread blendmap rebind + update-job emits UpdateRequests for every currently RESIDENT tile, then clears the flag)

**Why:** after a terrain paint/blend edit, GG rebuilds the chunk's blendmap texture and previously called `vt->invalidate()` — which resets the sparse VT's residency and re-streams the whole chunk through multi-frame GPU-feedback round-trips. On a 65536-res near chunk that took **4-5 seconds** for the paint stroke to become visible. The new flag keeps residency intact: the blendmap is rebound and all resident tiles are re-rendered in place, so the edit lands on screen the next frame. Far single-tile chunks (no residency) keep using `invalidate()` (cheap for them).

**Behaviour:** no stock behaviour change — the flag is passive unless a consumer sets it.

### 1.12 Terrain ChunkData::merge_pending — stale-mesh window flag

**Files:**
- `WickedEngine/wiTerrain.h` (`ChunkData::merge_pending` field)
- `WickedEngine/wiTerrain.cpp` (set alongside the `invalidated = false` flip when the generator finishes a chunk; cleared for all chunks right after `MergeFastInternal` in `Generation_Update`)

**Why:** when a chunk is invalidated (spline edit upstream; GG sculpt/paint bridge in the fork), the generator regenerates it in place reusing the entity, but the regenerated mesh only replaces the main-scene mesh at the NEXT `Generation_Update`'s merge. In that window `invalidated` is already false while the main-scene `MeshComponent` is still the pre-regeneration version. Consumers that bake data from the chunk mesh (GG's DX11-style + painted blendmap passes) would read the stale mesh and latch wrong results permanently (entity reuse means entity-churn detection never fires). `merge_pending` brackets the window exactly: set on the generator thread when the chunk's regen completes, cleared on the main thread once the merge lands. Consumers skip chunks with either flag up.

**Behaviour:** no functional change to stock Wicked rendering — the flag is passive unless something reads it.

### 1.11 Delayed shadow cascades: staggered per-cascade refresh

**Files:**
- `WickedEngine/wiRenderer.h` (`SetDelayedShadowCascadesEnabled` / `GetDelayedShadowCascadesEnabled` / `InvalidateDelayedShadowCascades`)
- `WickedEngine/wiRenderer.cpp` (cadence + frozen matrices in `UpdatePerFrameData`; `DrawShadowmaps` LoadOp::LOAD + per-rect clear draws + per-cascade skips; `PSO_shadowClear_GG`)
- `WickedEngine/shaders/shadowClearPS.hlsl` (new — in-renderpass rect clear, paired with screenVS + DSSTYPE_WRITEONLY)

**Date:** 2026-07-18 (Wicked commit `38a9e82a`)

**MODIFIED 2026-07-23 (Wicked commit `8c89731b`) — cascade-blend SYNC (fixes terrain-shadow flicker):**
the original `%2 %3 %4 %9` cadence + load-leveling deliberately DESYNCED cascades 1/2/3. But the lighting
shader deterministically blends adjacent cascades in the edge-fade band, so a boundary pixel lerps between
two cascades holding different-frame snapshots — whose silhouettes differ by per-cascade terrain LOD floor
(GGTerrain_part0.cpp) and by the terrain draw's per-frame `IsGenerating()/IsVisible()` chunk set during VT
re-stream — producing a "two terrain shapes" flicker even at a static camera. New cadence: cascade 0 every
frame, cascades 1..N refreshed TOGETHER every other frame, so every far-cascade boundary is always
self-consistent (still ~halves the staggered cascades' cost). USER-CONFIRMED fix. See
[[project-shadow-flicker]]. (Also: the DX12 "Delayed Shadows" UI checkbox is now wired to
`SetDelayedShadowCascadesEnabled` — game `a756c627` — and the shadow LOD override to it too — `d770ed00`.)

#### Use case in GameGuru MAX

Port of production DX11's "delayed shadows" (`g_bDelayedShadows`, old
WickedRepo wiRenderer.cpp GGREDUCED blocks): sun cascades refresh at
60/30/20/15/6.7 fps (c0 every frame, then %2 %3 %4 %9) with the DX11
load leveler and 64-inch camera-translation override. Skipped cascades
keep their atlas contents (the atlas is LOADED, and each rect that
renders clears itself with a scissored draw) and sample with frozen
matrices. Forced full refresh on: atlas grow/repack, directional rect
move, sun rotation and cascade-split changes (change-latched GG hooks in
`WickedCall_SetSunDirection` / `WickedCall_SetShadowRange`), and
`InvalidateDelayedShadowCascades()`. GG enables it at sun creation
(master_part1.cpp); the `DELAYED_SHADOWS 0|1` harness command A/Bs it
live. Measured on TESTPRO1 (static camera): Shadowmap Rendering CPU
2.32 -> 1.04 ms, GPU 0.62 -> 0.17 ms, visuals unchanged. Default OFF =
stock behaviour bit-for-bit.

### 1.10 Ocean: world-unit scale + intensity knob for shore/wave foam

**Files:**
- `WickedEngine/wiOcean.h` (`float foam_unit_scale = 1` + `float foam_amount = 1` on OceanParameters)
- `WickedEngine/wiOcean.cpp` (`GetOceanCBAtDim` fills the two new CB fields)
- `WickedEngine/shaders/ShaderInterop_Ocean.h` (CB padding slots become `xOceanFoamUnitScale` / `xOceanFoamAmount`)
- `WickedEngine/shaders/oceanSurfacePS.hlsl` (FOAM block: depth differences, shallow-water gate and noise positions multiplied by the unit scale; final foam multiplied by the amount)

**Date:** 2026-07-18 (Wicked commit `da60bfad`)

#### Use case in GameGuru MAX

The stock foam math is tuned for meters: shore band `exp(-depth_diff * 2)`,
wave foam gated on ~10m shallows, foam noise sampled per-meter. GG's world
is inch-scaled, so the shore foam band was ~40x too thin (a hairline at the
water's edge) and the noise repeated every inch. GG passes
`foam_unit_scale = 0.08` and `foam_amount = 1.3` (globals
`g_fWaterFoamUnitScale` / `g_fWaterFoamAmount` in M-GridEditB_part3.cpp,
live-tunable via the `SET_OCEAN` harness command). Note 0.08 is deliberately
NOT the pure inch conversion (0.0254): GG beaches are so shallow that a
true 1.5m-deep foam band covers a huge horizontal area — 0.08 was picked
visually on TESTPRO1 (bold shore line + foam collar around protruding
rocks, no milky blanket). Defaults 1/1 = zero change for stock scenes.

### 1.9 Animation/transform hardening: unit-quaternion guards + decompose validation

**Files:**
- `WickedEngine/wiScene_Components.cpp` (`TransformComponent::ApplyTransform` validates the `XMMatrixDecompose` result; on garbage keeps previous rotation/scale, takes translation from the world matrix; tripwire logs a symbolized call stack to `applytransform_garbage.txt`)
- `WickedEngine/wiScene.cpp` (both rotation write-back sites in `RunAnimationUpdateSystem` enforce a unit quaternion — renormalize, identity for zero/NaN/inf; tripwire logs inputs to `anim_garbage.txt`)

**Date:** 2026-07-18 (Wicked commit `a4539a76`)

#### Use case in GameGuru MAX

Fixes the intermittent "exploded skinned model" corruption (the Island
Showdown parrots): during level-load bursts, garbage rotations (a valid
axis scaled by thousands — an unnormalized accumulation) could be baked
into a few bones' `rotation_local` and persist for the entire session
whenever that bone's animation never re-evaluated (the stopped/culled
case), rendering the model as giant coloured sheets across the sky.
`XMQuaternionSlerp` propagates garbage instead of healing it, and
`XMMatrixDecompose` on a sheared/torn matrix emits huge quaternion
components — these guards make the corrupt pose impossible to store or
propagate. Paired with the GG-side `WickedCall_SanitizeSkeletons()`
(end of level load: repairs non-unit bone rotations + forces one
evaluation of every animation under the reveal cover). The tripwire
files only ever get written when a guard actually fires — if they appear
next to the exe, the original writer can finally be identified from them.

### 1.8 Terrain: optional high-priority generation jobs

**Files:**
- `WickedEngine/wiTerrain.h` (`bool generation_high_priority = false` on Terrain)
- `WickedEngine/wiTerrain.cpp` (generator workload + per-chunk vertex dispatch contexts pick High vs Low pool from the flag)

**Date:** 2026-07-18

#### Use case in GameGuru MAX

The generation workload AND its per-chunk parallel dispatches run on the
Low job pool, whose threads are `THREAD_PRIORITY_LOWEST` — the OS starves
them whenever the CPU is busy, which is precisely the level-load window
where terrain generation matters most. With the flag on, generation runs
on the High pool. GG enables it only while the camera-facing cone is
still building (< 40% of the ring total): combined with deltas 1.7 +
the GG-side budget turbo, the visible terrain on TESTPRO1 is COMPLETE
by the time the loading screen dismisses (generation races ahead behind
the load). Dropped back to Low afterwards — leaving it High during the
off-camera fill cost the editor 40-55 → 19-43 FPS. Default false =
stock behaviour.

### 1.7 Terrain: camera view-cone priority for chunk generation

**Files:**
- `WickedEngine/wiTerrain.h` (`bool generation_view_cone_priority = false` on Terrain)
- `WickedEngine/wiTerrain.cpp` (Generation_Update captures the camera's horizontal look direction; the generation job runs a cone-filtered spiral pre-pass before the normal outward spiral)

**Date:** 2026-07-18

#### Use case in GameGuru MAX

The stock generation spiral is omnidirectional, so after a level load the
chunks BEHIND the camera build at the same priority as the mountain the
user is staring at. With the flag enabled, a pre-pass sweeps the same
spiral restricted to chunks within ~70° of the camera's look direction
(plus the two rings immediately around the camera), then the normal
spiral fills the rest. `request_chunk` fast-skips existing chunks, and
the per-launch time budget makes each job launch resume where the last
ran out — the job also re-captures the camera each launch, so rotating
the camera re-aims the priority cone within ~one budget period.
Combined with a raised `generation_time_budget_milliseconds` during the
initial build (GG-side), the visible terrain on TESTPRO1 completes ~2-3
seconds after the editor appears (down from 30+ at stock settings).
Default false = zero behaviour change for stock scenes.

### 1.6 ObjectComponent: per-object opt-out from GPU occlusion queries

**Files:**
- `WickedEngine/wiScene_Components.h` (`OCCLUSION_QUERY_DISABLED = 1 << 11` flag + `SetOcclusionQueryDisabled` / `IsOcclusionQueryDisabled`)
- `WickedEngine/wiRenderer.cpp` (frustum-visibility job: flagged objects get `occlusionHistory |= 1` each frame instead of a query allocation)

**Date:** 2026-07-18

#### Use case in GameGuru MAX

The 20K-slot tree pool made per-object occlusion queries a net loss:
~2.5ms CPU + ~1.3ms GPU per frame of query bookkeeping + proxy-box
rendering on TESTPRO1, while foliage occludes almost nothing. Globally
disabling occlusion culling would take the feature away from regular
entities (and the user-facing `t.visuals.bOcclusionCulling` setting),
so instead pool objects carry the new flag and are simply always
treated as visible. Measured on TESTPRO1: "Occlusion Culling" +
"Occlusion Culling Render" GPU ranges 1.96 + 1.94ms → 0.08 + 0.08ms;
part of the 24 → 60 FPS editor perf push.

The `occlusionHistory |= 1` write is required: history shifts left every
frame, so a flagged object that never allocates a query would otherwise
decay to "occluded" after 32 frames and vanish whenever occlusion
culling is enabled.

### 1.5 HairParticleSystem: per-strand slope + altitude filters for grass entities

**Files:**
- `WickedEngine/shaders/hairparticle_simulateCS.hlsl` (two extra branches inside the existing `xHairGrassType != 0u` block)
- `WickedEngine/shaders/ShaderInterop_HairParticle.h` (5 new floats + 3 padding on the CB for the altitude cutoffs)
- `WickedEngine/wiHairParticle.h` (5 mirror fields on `HairParticleSystem`)
- `WickedEngine/wiHairParticle.cpp` (copy mirror fields into the CB alongside the Option B fields)

**Date:** 2026-07-10

#### Use case in GameGuru MAX

DX11 filtered grass instances by terrain normal — `GGGrass_UpdateInstances`
skipped instances whose sampled `ny < 0.7` (≈45°), so no grass grew on
cliff faces or steep mountain flanks. In Wicked mode `UpdateInstances`
is gated off (perf), and reproducing the same filter on the CPU turns
out to be structurally hopeless:

- The DX11 normal map is stored at per-chunk texel resolution (hundreds
  of world units per texel), so a per-cell scan of the grass map (~4.8
  units per cell on a typical island level) reads bilinearly-averaged
  false-flat values along cliff edges.
- Computing a local slope from three `GGTerrain_GetHeight` samples per
  cell fixes the resolution mismatch but doesn't fix the placement
  mismatch: a strand belonging to a *flat* paint cell can still land on
  the adjacent *cliff* triangle via the hair system's random-barycentric
  distribution, so the "grass right on the cliff edge" case survives.

#### Mechanism

The correct filter is per-strand, using the exact face normal of the
triangle the strand sits on. Fix 1.2 already computes that face normal
in-shader (from three vertex positions, replacing the stored per-vertex
normal that was invalid at `chunk_scale > 1`). The value is available
in `target` at the point where the paint mask check runs, so the slope
filter is a single extra branch inside the existing `xHairGrassType != 0u`
block:

```hlsl
if (target.y < (half)0.7)
{
    strand_length = 0;
}
```

Cliffs go strand-by-strand grass-free at exact triangle granularity.
Gentle slopes keep their grass. No paint cells are mutated, so a later
sculpt that flattens a cliff face doesn't require repainting.

#### Companion: per-strand altitude filter (added same day)

Same design, same gate — an altitude band drives the visibility of each
strand based on its world Y (`base.y`, which for terrain-mounted grass IS
the terrain height at that XZ). Above-water and underwater use disjoint
`[min, max]` pairs, selected by a comparison against a water plane. The
CB row layout:

```hlsl
float xHairGrassWaterHeight;
float xHairGrassMinHeight;
float xHairGrassMaxHeight;
float xHairGrassMinHeightUnderwater;
float xHairGrassMaxHeightUnderwater;
```

Defaults span the full range on the C++ side (`grass_min_height = -1e30f`,
`grass_max_height = 1e30f`, likewise for the underwater pair) so callers
who don't set them get "no filter" — zero behavioral change out of the
box. The shader branch sits inside the existing `xHairGrassType != 0u`
gate so upstream Wicked hair (`grass_type == 0`) still bypasses it.

#### Why this is genuinely useful upstream

The check runs only when `xHairGrassType != 0u` — so upstream callers
that don't opt into the grass-type feature see zero behavior change.
Callers that DO use the grass-type hook (Option B / entry 1.4) get an
extra "filter strands by triangle slope" tool for free. Handy for any
system that wants to place vegetation on terrain-shaped meshes without
manual cliff masking. The threshold is currently hardcoded at 0.7 to
match DX11; a runtime constant would be a small extension if configurability
is needed later.

### 1.4 HairParticleSystem: per-strand visibility from an external paint mask (Stage 3 Option B)

**Files:**
- `WickedEngine/shaders/ShaderInterop_HairParticle.h` (CB field additions)
- `WickedEngine/shaders/hairparticle_simulateCS.hlsl` (SRV declaration + sample)
- `WickedEngine/wiHairParticle.h` (public field additions)
- `WickedEngine/wiHairParticle.cpp` (placeholder texture + CB write + SRV bind)

**Date:** 2026-06-19

#### Use case in GameGuru MAX

Grass is placed per-chunk over a `wi::HairParticleSystem` whose strand
positions are derived from random barycentric coordinates on the chunk
mesh's triangulation. With paint-mask gating only at the vertex level
(via `vertex_lengths`), every triangle that *contains any painted
vertex* gets strands distributed across its FULL surface — producing
a roughly 4×-brush-wide footprint when the brush is small (~30 in) but
the vertex grid is coarse (~80 in spacing at `chunk_scale = 80`).

To match the DX11 grass behaviour where a brush blob renders blades
ONLY inside the painted footprint, each strand needs to check the
paint mask at *its own* world XZ, not just at the triangle vertices.

#### Mechanism

The upstream `HairParticleCB` is extended with one 16-byte row carrying
the world-space-to-UV transform for an external paint mask plus a
"which type does this hair entity represent" tag:

```hlsl
uint  xHairGrassType;            // 1..N = active; 0 = disabled (upstream behavior)
float xHairGrassMapInvWorldSize; // 1.0 / world extent (matches GG_GetGrassMap CPU formula)
float xHairGrassMapOriginX;      // world XZ of map center (0 for centered maps)
float xHairGrassMapOriginZ;
```

A new SRV slot (`Texture2D<float> texHairGrassMap : register(t4)`) is
declared in the simulate CS. `HairParticleSystem` gains mirroring
public fields (`grass_type`, `grass_map_inv_world_size`,
`grass_map_origin_x/z`, `grass_visibility_texture`). When the caller
sets `grass_type` to a non-zero value, the simulate CS samples the
mask at the strand's world XZ and zeros `strand_length` if the cell
encodes a different type. A 1×1 zero-init placeholder bound by
default keeps DX12 validation happy for non-GG hair entities, which
leave `grass_type` at 0 and never reach the sample branch.

#### Why this is genuinely useful upstream

Any caller that wants per-strand visibility against an external mask
gets it for free with a CB write + one SRV bind. The default of
`grass_type = 0` makes the feature a pure no-op for existing usage —
no shader branch is taken, no perf cost. The R8_UNORM byte decode in
the shader is GG-specific (`flattened | type-with-+2-offset`), but
the *mechanism* — passing a typed paint mask plus a world-XZ-to-UV
transform into the simulate CS — generalises directly to other use
cases (e.g. lawn mowing, footprint trails, multi-zone variation).

### 1.3 HairParticleSystem has no targeted vertex_lengths update — `CreateRenderData` is destructive

**Files:** `WickedEngine/wiHairParticle.h`, `WickedEngine/wiHairParticle.cpp`
**Date diagnosed:** 2026-06-19

#### Symptom in GameGuru MAX

When the player paints grass, each affected chunk's hair entity needs
the per-vertex paint mask refreshed on the GPU. Wicked's only existing
upload path is `CreateRenderData()`, which calls `DeleteRenderData()`
first — destroying `generalBuffer` (the suballocated GPU memory that
hosts `simulation_view`, `vb_pos[0/1]`, `vb_nor`, …) and setting
`regenerate_frame = true`. The next simulate-CS dispatch sees garbage
`prevTail` / `currentTail` plus the regenerate flag and snaps every
strand's animated tip to the rest position for one frame. Result: a
visible "settling pop" on the wind animation every time we update
paint mask, even when nothing about strand count, mesh, or index list
has changed.

#### Fix (this entry)

Add a public method `HairParticleSystem::UpdateVertexLengthsBuffer()`
that recreates **only** the `vertexBuffer_length` GPU buffer from the
current `vertex_lengths` data. It leaves every other resource alone:
`generalBuffer`, `simulation_view`, `vb_pos[0/1]`, `vb_nor`, `vb_uvs`,
`wetmap`, `ib_culled`, `prim_view`, `indirect_view`,
`vb_pos_raytracing`, `indexBuffer`, `BLAS`, `regenerate_frame`,
`_flags`. The simulate CS rebinds `vertexBuffer_length` per dispatch
(`wiHairParticle.cpp` ~line 560), so the next frame's dispatch picks
up the new buffer automatically and per-strand simulation state stays
alive across the update.

Falls back to a full `CreateRenderData()` call if `generalBuffer`
isn't valid yet (caller skipped initial setup), so misuse is safe.

This is genuinely useful upstream — any caller that needs to mutate
the paint mask on a live hair system gets free animation continuity.

### 1.2 HairParticleSystem reads stored vertex normals that don't match the mesh triangulation

**File:** `WickedEngine/shaders/hairparticle_simulateCS.hlsl`
**Lines:** ~81-82 (the `target = ...` block)
**Date diagnosed:** 2026-06-18

#### Symptom in GameGuru MAX

With Terrain `chunk_scale = 80`, grass blades placed on the chunk
appeared at chaotic orientations — many leaning at ~45-90° on what
should be mild slopes, with neighbouring blades pointing in different
directions despite the underlying terrain being smooth.

#### Root cause

The simulate compute shader reads the three stored vertex normals
(`nor0`, `nor1`, `nor2`) from the chunk mesh, barycentrically
interpolates them, applies the emitter transform adjoint and uses the
result as the blade's up-axis (`target`). The problem is that
`wiTerrain.cpp` writes each vertex's stored normal as the face normal
of a **fixed reference triangle** `(V, V+x, V+z)` — NOT as an average
of the face normals of the actual mesh triangles touching that vertex.

For the terrain mesh's first triangle in each quad
(`topLeft, lowerLeft, lowerRight`):
- `nor1` (lowerLeft) happens to match — Wicked's reference triangle at
  lowerLeft is `(lowerLeft, lowerRight, topLeft)`, which is the same
  three vertices.
- `nor0` (topLeft) is computed from the triangle
  `(topLeft, topRight, topLeft+1z)` — vertices entirely in adjacent
  quads, **not in the triangle being sampled**.
- `nor2` (lowerRight) is similar — reference triangle is in the
  right-hand neighbour quad.

For the second triangle (`topLeft, lowerRight, topRight`) all three
stored normals are for unrelated phantom triangles.

At `chunk_scale = 1` (Wicked default) the height delta over one V→V+1
hop is tiny, so every reference triangle in a small area has nearly
the same normal — the structural mismatch is invisible. At
`chunk_scale = 80` the same hop spans 80 world units, so the height
deltas (and resulting normals) of two different reference triangles
near the same point can differ by tens of degrees.

#### Fix (this entry)

In the simulate CS, replace `target` with the **actual face normal of
the triangle the blade sits on**, computed in-shader from the three
vertex positions (`pos0`, `pos1`, `pos2`) after the same
`xHairBaseMeshUnormRemap.GetMatrix()` step that `position` goes
through:

```hlsl
float3 P0 = mul(xHairBaseMeshUnormRemap.GetMatrix(), float4(pos0, 1)).xyz;
float3 P1 = mul(xHairBaseMeshUnormRemap.GetMatrix(), float4(pos1, 1)).xyz;
float3 P2 = mul(xHairBaseMeshUnormRemap.GetMatrix(), float4(pos2, 1)).xyz;
target = (half3)normalize(cross(P2 - P0, P1 - P0));
```

The operand order matters — the terrain index winding makes
`cross(P1 - P0, P2 - P0)` point DOWN, so the cross product operands
are swapped to give +Y on flat ground.

This is a **GameGuru-side workaround**. Only the grass simulate path
is affected. The structural Wicked issue (stored vertex normals not
matching the mesh triangulation) still affects terrain shading and
slope-based material weighting — those would benefit from a proper
upstream fix that averages face normals across all triangles touching
each vertex.

### 1.1 Terrain chunk per-vertex normals ignore `chunk_scale`

**File:** `WickedEngine/wiTerrain.cpp`
**Lines:** ~1100-1107 (inside the second `wi::jobsystem::Dispatch` lambda
that fills `mesh.vertex_normals`)
**Date diagnosed:** 2026-06-18

#### Symptom in GameGuru MAX

When `Terrain::chunk_scale` is set above 1 (we use `chunk_scale = 80.0f`
in `GGTerrainWicked.cpp` to reduce chunk popping), the per-vertex normals
written into the terrain chunk meshes become almost horizontal. Anything
that reads those normals sees a near-vertical-cliff slope wherever the
ground is mildly sloped:

- `Terrain::HairParticleSystem`-based grass renders blades laid on their
  side rather than upright (this is how we first noticed).
- `Terrain` slope-based auto-material weighting (`slope_amount = 1.0 -
  saturate(normal.y)` at the same callsite) flips into "slope" material
  for almost the entire chunk.
- Object/terrain lighting uses these normals, so terrain shading also
  reads as if every triangle were a vertical wall.

The visible terrain *geometry* is correct because
`mesh.vertex_positions` (set further down at line ~1133) uses the
`chunk_scale`-aware local x,z. Only the normals are wrong.

#### Root cause

```cpp
const float x = (float(coord.x) - chunk_half_width) * chunk_scale;
const float z = (float(coord.y) - chunk_half_width) * chunk_scale;
const float height = heights_padded[coord.x][coord.y];
const XMVECTOR corners[3] = {
    XMVectorSet(chunk_data.position.x + x,     height,                                       chunk_data.position.z + z,     0),
    XMVectorSet(chunk_data.position.x + x + 1, heights_padded[coord.x + 1][coord.y],        chunk_data.position.z + z,     0),
    XMVectorSet(chunk_data.position.x + x,     heights_padded[coord.x][coord.y + 1],        chunk_data.position.z + z + 1, 0),
};
const XMVECTOR T = XMVectorSubtract(corners[1], corners[2]);
const XMVECTOR B = XMVectorSubtract(corners[0], corners[1]);
const XMVECTOR N = XMVector3Normalize(XMVector3Cross(T, B));
```

The horizontal step between `corners[0]` and `corners[1]` (and between
`corners[0]` and `corners[2]`) is a literal `+ 1`, but the height
sampled is the **real world height** at the next vertex, which sits
`chunk_scale` world units away. The cross product therefore sees the
height delta over a run of `1` instead of `chunk_scale` — slopes get
amplified `chunk_scale`-fold.

With `chunk_scale = 80`, a real 5° slope (height rise ~7 units over an
80-unit run) is interpreted as `atan(7/1) ≈ 82°`. Hence near-horizontal
normals everywhere.

When `chunk_scale = 1` (engine default in `wiTerrain.h:327`) the formula
happens to be correct, which is why this bug has not been noticed before.

#### Fix

Replace the two literal `+ 1` offsets with `+ chunk_scale` so that the
horizontal steps match the actual vertex spacing:

```cpp
XMVectorSet(chunk_data.position.x + x + chunk_scale, heights_padded[coord.x + 1][coord.y], chunk_data.position.z + z,              0),
XMVectorSet(chunk_data.position.x + x,               heights_padded[coord.x][coord.y + 1], chunk_data.position.z + z + chunk_scale, 0),
```

`chunk_scale` is already captured in the dispatch lambda via `[&]` and
is used immediately above at the same indentation (lines 1097, 1098).

This change keeps the orientation of the cross product unchanged (only
the y-component grows from `1` to `chunk_scale^2` after the cross, and
normalization restores the magnitude). The fix has zero effect when
`chunk_scale = 1`, so it does not regress the default Wicked terrain.

---

## 2. Temporary debug overrides (MUST be reverted before any upstream brief)

These are diagnostic edits made during the grass-rendering analysis on
2026-06-18. They live in the same WickedEngineDX12 clone but should
never reach upstream. Backups of the original compiled shaders sit
alongside the live ones in
`D:\DEV\BUILD\GameGuru Wicked MAX Build Area\Max\shaders\` with a
`.bak` suffix.

### 2.1 `WickedEngine/shaders/hairparticlePS.hlsl`

Force grass pixels to opaque white at the end of `main()`. Used to
confirm the shader-edit → DXC-auto-recompile loop is intact.

Revert by restoring the original `return color;` and deleting the
debug comment block.

All debug overrides have been reverted as of 2026-06-18 18:28 (UK
local). See `git status` of `D:\max\WickedEngineDX12` — only
`wiTerrain.cpp` (#1.1) and `hairparticle_simulateCS.hlsl` (#1.2) are
modified, both genuine bug fixes.

---

## 3. Status

| Item | Status | Action |
|---|---|---|
| 1.1 Terrain chunk normal fix | applied 2026-06-18 in Wicked commit `6068a1bb`, lib rebuilt | candidate for upstream PR |
| 1.2 HairParticleSystem face-normal override | applied 2026-06-18 in Wicked commit `5329fc8b`, shader auto-recompiles | candidate for upstream PR; long-term Wicked fix is to average vertex normals properly |
| 1.3 HairParticleSystem UpdateVertexLengthsBuffer | applied 2026-06-19, lib rebuilt | candidate for upstream PR — pure addition, no behavioural change for existing callers |
| 1.4 HairParticleSystem external paint mask (Option B) | applied 2026-06-19, lib + shaders rebuilt | candidate for upstream PR — `grass_type == 0` default means zero behavior change for existing callers; adds general-purpose per-strand visibility hook |
| 1.5 HairParticleSystem per-strand slope + altitude filters | applied 2026-07-10, lib + shaders rebuilt | candidate for upstream PR — both gated on `xHairGrassType != 0u`, zero effect on non-GG hair; slope reuses the face-normal fix 1.2 already computes, altitude adds 5 CB floats with permissive defaults |
| 1.6 ObjectComponent occlusion-query opt-out flag | applied 2026-07-18, lib rebuilt | candidate for upstream PR — flag defaults off, zero behaviour change for existing scenes; new flag consumers must set `occlusionHistory |= 1` semantics (already handled in the visibility job) |
| 1.7 Terrain view-cone chunk generation priority | applied 2026-07-18, lib rebuilt | candidate for upstream PR — flag defaults off; pure generation-ORDER change, the generated content is identical |
| 1.8 Terrain high-priority generation jobs | applied 2026-07-18, lib rebuilt | candidate for upstream PR — flag defaults off; burst-scenario knob, GG enables it only while the view cone is incomplete |
| 1.9 Animation/transform hardening (unit-quaternion guards + decompose validation) | applied 2026-07-18 in Wicked commit `a4539a76`, lib rebuilt | candidate for upstream PR — guards only fire on already-corrupt data (zero behaviour change for healthy scenes); tripwire files `anim_garbage.txt`/`applytransform_garbage.txt` appear next to the exe only if a guard fires |
| 1.10 Ocean foam world-unit scale + intensity knob | applied 2026-07-18 in Wicked commit `da60bfad`, lib rebuilt, shader auto-recompiles | candidate for upstream PR — both params default 1 = stock behaviour; fills existing CB padding so no layout change |
| 1.11 Delayed shadow cascades (staggered refresh) | applied 2026-07-18 in Wicked commit `38a9e82a`, lib rebuilt, new shadowClearPS shader auto-compiles | candidate for upstream PR — default OFF preserves stock behaviour bit-for-bit; GG enables at sun creation; single-directional-light assumption documented in code |
| 1.12 Terrain ChunkData::merge_pending stale-mesh flag | applied 2026-07-18, lib rebuilt; **hotfix `df3c10e0` same day: flag only on chunks actually (re)generated** — the spiral visits every chunk every job run and the original unconditional set made GG's blend passes skip the whole map forever (Island Showdown grey terrain) | candidate for upstream PR — passive field, closes the regen-vs-merge window for mesh-baking consumers (GG blendmap passes) |
| 1.13 VirtualTexture::pending_repaint_blendmap instant refresh | applied 2026-07-18, lib rebuilt | candidate for upstream PR — passive flag; paint strokes visible next frame instead of after seconds of feedback re-streaming |
| 1.14 Optional generation restart on dirty materials | applied 2026-07-19 (`cbce724d`), lib rebuilt | candidate for upstream PR — default true = stock; GG disables (runtime material registration must not rebuild the island) |
| 1.15 Preserve blendmap + VT residency on in-place regen | applied 2026-07-19, lib rebuilt | GG-specific (default false = stock) — sculpt-drag chunks keep GG blendmaps and resident tiles; fresh merged material re-bound without vt.init() |
| 1.16 VT repaint flag main-thread latch | applied 2026-07-19, lib rebuilt | race fix for 1.13's flag (found by multi-agent review of 1.15) — async job consumes a main-thread-latched copy, never the live flag |
| 1.17 gg_generate_blendmap born-correct chunks | applied 2026-07-19, lib rebuilt | GG-specific (null by default) — generator thread fills GG auto+painted weights before the region texture is built; kills the green default-blend flicker on fast camera zooms |
| 1.18 VT residency upgrade hysteresis | applied 2026-07-19, lib rebuilt | GG-specific (default false = stock) — residency upgrades wait for the camera to settle (10 stable frames), then 4/frame; kills the square sharpness-flicker while zooming |
| 1.19 VT stale tile-identity release + full freeze-while-moving | applied 2026-07-19, lib rebuilt | part 1 = stock bug, CANDIDATE FOR UPSTREAM PR (dangling last_used pointer → foreign-content squares); part 2 extends 1.18's freeze to downgrades |
| 1.20 VT tile allocator freeze while camera in motion | applied 2026-07-19 + same-day hotfix (aging always runs, tail-invalid self-heal) | 60-frame recycle threshold mid-motion — visible tiles can never be stolen by the miss flood |
| 1.21 VT expanded working set (island-wide full-res zone) | applied 2026-07-19, lib rebuilt | GG-specific (defaults = stock) — gg_near_ring_dist 6 + gg_removal_margin 12: camera travel over the island never crosses a VT resolution boundary; chunks survive zoom travel |
| 1.22 Ocean caustic size decoupled from patch size | applied 2026-07-20, lib rebuilt | GG-specific (default 1.0 = stock) — new ShaderOcean.caustic_scale scales ONLY the seabed caustic lookup; Water-panel "Caustic Size" slider tunes ripple size without touching the waves |
| 1.23 Ocean underwater fog colour+density decoupled from surface | applied 2026-07-21, lib rebuilt | GG-specific (default density 0 = stock fallback) — new ShaderOcean.underwater_color + underwater_fog_density; Water-panel "Underwater Color" + "Underwater Fog" control the submerged view independently, so the surface can stay transparent when the water colour changes |
| 1.24 Ocean water base colour tints transparent water (depth) | applied 2026-07-21, lib rebuilt | GG-specific (default 0 = stock) — new OceanCB.xOceanWaterColorDepth; oceanSurfacePS lerps refraction toward the base colour by depth so "Water Base Color" is visible on WaterAlpha-0 water (GG sets 0.005). Fixes "set red, sea didn't change from above" |
| 1.25 Hair particles skip billboard-write + physics for culled strands | applied 2026-07-22 in Wicked commit `3273b651`, lib rebuilt, shader auto-recompiles | candidate for upstream PR — drawn (visible) strands byte-identical; a culled strand's sim freezes until it re-enters view; saves proportional to off-screen strand fraction |
| 1.28 Terrain SVT mip bias −2.0 → 0.0 (mid/long swim) | applied 2026-07-24, lib rebuilt | GG value-change on stock Wicked `SVT_MIP_BIAS` (ShaderInterop_Renderer.h) — stock −2.0 biases SVT terrain 2 mips SHARPER than correct → mid/long pixel-swim; 0.0 = Nyquist-correct, swim gone, near field stays crisp. This is the REAL terrain-LOD lever (SVT fetches via SampleLevel, so the sampler's aniso/mip-bias are ignored). Also reverts 1.27. Shared header → recompiles ~375 engine shaders |
| 1.29 Animation: wire the 30fps throttle to the "Lower Animation & LUA Speed" checkbox + distance gate | applied 2026-07-24, lib rebuilt (wiScene.cpp only) | The every-other-frame animation-skip in `Scene::RunAnimationUpdateSystem` (wiScene.cpp) was gated on a hardcoded lambda-`static bEnable30FpsAnimations=0` — unwired ever since the game-side GGAnimBridge was removed as too slow (89873913). Added namespace globals `gg_anim30fps_enabled/frame/dist2`, driven per-frame by the game (`MasterRenderer::Update`) from the editor checkbox BEFORE `__super::Update`. (b) Fixed the skip parity to a real per-frame counter (the old `iAnimFrames` was a racy per-job lambda-static → non-deterministic skip). (c) Added a per-object distance gate (`gg_anim30fps_dist2`, cmp `aabb_objects[oi].getCenter()` vs `GetCamera().Eye`) so near-camera armatures stay full-rate; default farDist 2000 (game `g_animThrottleFarDist`, clamped ≤60000 for the uint32 dist² guard). Timer still advances on skipped frames (true 30fps, not slow-motion); keeps the `!updateonce` guard + unit-quaternion guards → parrot-safe (no tripwire). Measured: throttle-all halves anim sampling 0.61→0.20ms; distance gate modulates by distance (crowd all <~4000 units on TESTPRO1). Harness `SET_ANIM30FPS <0|1> [farDist]`. Absolute distance is scene-scale-dependent → wants a UI slider / apparent-size gate follow-up |
| 1.27 Terrain SVT anisotropy ×4 → ×8 (grazing swim) | **REVERTED 2026-07-24 (no-op), superseded by 1.28** — was Wicked commit `3a6fc38c` | The SVT terrain samples via `SampleLevel` (explicit LOD), which ignores the sampler's anisotropy — so the ×4→×8 bump had NO visible effect. Reverted to stock ×4. The real swim fix is the mip bias (1.28). Kept in history as a lesson: confirm the sampling path (Sample vs SampleLevel) before touching sampler state on SVT |
| 1.26 Ocean env-map reflection fallback when planar reflections off | applied 2026-07-24 in Wicked commit `bf09e448`, lib rebuilt | CANDIDATE FOR UPSTREAM PR — fixes garbage-water when reflections are toggled off (unguarded `texture_reflection_index` left dangling at a stale `rtReflection_resolved`); guard matches the planar-render gate so reflections-ON is byte-identical, OFF uses the existing `EnvironmentReflection_Global` path and skips the planar pass |
| 1.30 Apparent-size object cull in UpdateVisibility | applied 2026-07-24 in Wicked commit `8ad10e54`, lib rebuilt | GG-specific (default `gg_apparent_cull_bits`=0 = disabled = stock) — new atomic threshold in the main-view object cull loop (`wiRenderer::UpdateVisibility`): an object that passes the frustum but whose world AABB `getRadius()/distance` < tangent is dropped from `visibleObjects` (renders as a distant speck). World AABB radius bakes in original size × scale, /distance = on-screen apparent size → scale-invariant. Squared-tangent precomputed per view (no per-object sqrt); `distSq > radiusSq` guard keeps near/inside objects. Affects the 3 UpdateVisibility callers (main + planar-reflection + render-to-texture cam) — each culls specks by its own camera; shadow cascades cull separately so a culled speck can still cast its sub-pixel shadow. Driven per-frame by game `MasterRenderer::Update` from the editor "Apparent Size" slider (`maxApparentSize` → tangent via `g_apparentCullK`=59; default slider = tangent 0 = no cull; slider 1.0 → tangent ~0.0054). Harness `SET_APPARENTSIZE <tangent>` / `SET_APPARENTSIZE slider <fASize>`. CANDIDATE FOR UPSTREAM PR — a generic projected-size cull, flag-gated off by default |
| 1.31 Configurable delayed-shadow cascade interval ("Laptop" mode) | applied 2026-07-25 in Wicked commit `de50638b`, lib rebuilt | GG-specific (default 2 = stock delta-1.11 behaviour). The far directional cascades (1..N) refreshed every other frame (hardcoded `frame%2`); made the period a runtime knob `delayedShadowCascadeInterval` (setter `SetDelayedShadowCascadeInterval`, clamped >=2) so the game can request an every-4th-frame "Laptop" cadence. Cascades 1..N still refresh TOGETHER on the same frame → the cascade-blend SYNC that cured the "two terrain shapes" flicker (see [[project-shadow-flicker]]) holds at any interval; cascade 0 stays every-frame and the >64" camera-move force-refresh is unchanged. Driven per-frame by game `MasterRenderer::Update` from `g_bDelayedShadowsLaptop` (2 or 4). Verified: interval-4 shadows are statistically identical to interval-2 at a static camera (cross-interval diff below the scene's animation noise floor). Harness `SET_LAPTOP_SHADOWS <0|1>` |
| 1.32 Frame-cost CPU instrumentation (Scene stages, RP3D recording jobs, VT job) | applied 2026-07-25, lib rebuilt (wiScene.cpp, wiRenderPath3D.cpp, wiTerrain.cpp) | Instrumentation only — `wi::profiler` CPU ranges, free when the profiler is off. Scene::Update partitioned at its jobsystem::Wait barriers (`Scene-S1 Anim+Transform` … `Scene-S5 Tail+ShaderScene`); RenderPath3D PreRender (`RP3D-VisMain/VisRefl/PerFrameData`) + Render serial/wait spans + a range inside every command-list recording job (`RP3D-rec *`); terrain VT job internals (`VT-job Total/FreeSort/PageBuf`, `VT-WaitInRecord`, `VT-record`). THIS FOUND THE FRAME POLE: the months-old "fixed ~9.5ms Render CPU" was `RP3D-rec PrepareAsync` blocking on the terrain VT background job — **16.4ms EVERY frame on one worker thread** (14.2ms uncached page-table memcpy loop + 2.1ms free-list rebuild+sort) |
| 1.33 Incremental terrain-VT bookkeeping (16.4ms/frame → 0.4ms) | applied 2026-07-25, lib rebuilt (wiTerrain.h/.cpp) | GG-specific, master switch `wi::terrain::gg_vt_incremental` (false = stock). A chunk-VT's page table only changes when a tile is allocated INTO it or stolen FROM it: `PhysicalTile::gg_owner` tracks the owning VT; `allocate_tile` reports the steal VICTIM so both winner and victim are marked `gg_page_dirty`. Clean VTs skip (a) the huge uncached upload-buffer write loop, (b) the GPU `pageBuffer` CopyResource, (c) the residency-map recompute dispatch — each pending flag has exactly one consumer on its own command list. A rotating heartbeat force-refreshes ONE resident VT per frame as insurance. Free-list rebuild+sort (2.1ms) is now lazy: only on frames with allocation requests / after consumption (`gg_free_dirty`) / rate-limited low-water refill. VT::free() clears `gg_owner` (no dangling deref). MEASURED: VT-job 16.39→0.41ms, CPU "Render" 9.72→1.71ms, TESTPRO1 gate **52 → 62-70 FPS**; live A/B via harness `SET_VTINC 0|1` (stock 50-52 / incremental 59-70). Stress-verified: 3× violent teleport cycles → no foreign squares, terrain streams sharp; sculpt+paint+undo clean. The frozen-mode main-thread filter and tile AGING cadence are untouched (VT do-not rules respected) |
| 1.34 Remove dead entity-set inserts in ScanAnimationDependencies | applied 2026-07-25, lib rebuilt (wiScene.cpp) | The animation-dependency check (sole READER of `AnimationQueue::entities`) was already commented out by the earlier "assume no dependencies" GG delta — but the writer survived: `queue.entities.clear()` + per-channel hash-set inserts for every playing animation ≈ 10-25k write-only ops/frame at 122 anims (~0.8ms of the "Animation Dependencies" range). Deleted the writes; restore together with the dependency block if it ever returns |
| 1.35 Frustum-visibility animation pause | applied 2026-07-25, lib rebuilt (wiScene_Components.h, wiRenderer.cpp, wiScene.cpp) | GG-specific, off when `gg_anim_vis_pause_frames`=0. `ObjectComponent::gg_last_visible_frame` stamped in the main-view (and reflection) `UpdateVisibility` cull; `RunAnimationUpdateSystem` pauses EVALUATION of animations whose target object hasn't passed the cull for >N frames AND is beyond the near guard (`gg_anim_vis_pause_neardist2`, protects just-off-frame characters with on-screen shadows). Timers keep advancing (exact same semantics as the long-shipped `IsRenderable` cull just above it) → characters resume in perfect sync on re-entry. Driven per-frame by `MasterRenderer::Update` from game globals `g_animVisPauseFrames`=3 / `g_animVisPauseNearDist`=2000. Known accepted trade: a fully off-screen character's cast shadow freezes while unseen. Harness `SET_ANIMVIS <frames> [neardist]` |
| 1.36 Subtree-parallel hierarchy update (O(N×depth) → O(N)) | applied 2026-07-25, lib rebuilt (wiScene.h/.cpp) | GG-specific, master switch `wi::scene::gg_hierarchy_levelorder` (false = stock chain walk). Stock `RunHierarchyUpdateSystem` rebuilds the ENTIRE ancestor chain per hierarchy entry (`GetLocalMatrix` + multiply per level ≈ millions of ops at 18k bone entries × depth 8-15). Now the topdown workload also collects the subtree ROOTS (entries whose parent is INVALID or not itself an entry); the update dispatches ONE JOB PER ROOT and each job walks its own subtree depth-first via `topdown_hierarchy`, CARRYING the accumulated world matrix + ancestor layer mask down the chain — one local-matrix build + one multiply per entity, zero cross-job dependencies, zero barriers (a first level-order design with a jobsystem barrier per depth level was discarded: 30-60 bone-depth levels × Dispatch+Wait ate the win). Float note: different association order ⇒ last-ulp differences vs stock, visually identical. Layer propagation carried exactly per stock semantics incl. layer-less/transform-less mid-chain ancestors and no-hierarchy root parents; 100k-node cycle guard. Harness `SET_HIERLO 0|1` |
| 1.37(+b/c/d) Hair/grass simulation static-skip + wind cadence | **DISABLED BY DEFAULT 2026-07-25 (`gg_hair_sim_static_skip=false`) — a full manual knob bisect (6 knobs, fresh-launch rounds A-E) convicted this delta ALONE of the intermittent load/paint grass flicker. Fix attempts b (ping-pong parity — real bug, kept as dormant hardening), c (full-rate hysteresis), d (frozen-velocity vb_pre patch for TAA) reduced but did not eliminate it; the residual mechanism needs a PIX/RenderDoc frame capture. Re-enable via SET_HAIRSKIP for future investigation; costs ~1.4ms GPU parked (~78→~73 FPS on TESTPRO1).** Original delta: applied 2026-07-25 (+b `6d4ab66c`, +c `d7cea7f5`, +d), lib rebuilt (wiHairParticle.h/.cpp, wiRenderer.cpp, wiScene.cpp) | GG-specific, master switch `wi::renderer::gg_hair_sim_static_skip` (false = stock every-frame sim). The 2M-strand grass simulate dispatch (~1.9ms GPU on TESTPRO1) recomputes a near-identical result when the camera is parked (the sim's only view inputs are per-strand culling + camera-bend). Camera parked ≥4 frames AND every visible system settled (`gg_sim_runs ≥ 4` since creation / CreateRenderData regeneration / transform move): **no wind → skip the batch entirely; wind active (GG levels default to dir=(1,0,1) speed=1) → simulate every `gg_hair_sim_wind_interval`-th frame (default 4)** — sway continues in gentle slow-motion (the sim integrates per-dispatch dt). Any new/regenerated/moved system or camera change resumes full-rate instantly. **1.37b (the load/paint grass-flicker fix):** the vb_pos ping-pong swap is gated on `gg_sim_ran_last_frame` — swapping on frames the sim skipped made write/read parity depend on the GAP LENGTH between sims; irregular gaps (grass streaming, paint creating systems → intermittent forced full-rate) flipped parity per sim → position pops = flicker (camera movement 'cured' it by forcing every-frame sim). The ping-pong now only advances on sim frames, so every sim reads exactly its predecessor. **1.37c (`d7cea7f5`, residual transition flicker): forcing events (camera motion, any unsettled system) arm a 60-frame FULL-RATE cooldown — the cadence may only engage after everything has been calm that long, so load-streaming/painting windows run bit-for-bit stock instead of bouncing between full-rate and skipping (irregular sway hops + erratic motion vectors = the ~8s intermittent flicker).** Verified: every post-materialization load pair ≤1.3 mean (1.37b had 6.7-6.8 spikes; stock reference 1.5-3.1); cadence still engages (settled FPS ~68+ vs stock ~50). Harness `SET_HAIRSKIP <0|1> [windInterval]` |
| 1.38 RunObjectUpdateSystem: row-lengths instead of XMMatrixDecompose | applied 2026-07-25, lib rebuilt (wiScene.cpp) | Per-object per-frame `XMMatrixDecompose` whose output was ONLY used for the scale magnitude (`size = max(S.xyz)`); basis row lengths give the identical value without the quaternion extraction, and unlike the decompose they also work for negative-determinant matrices. 8722 decomposes/frame removed |
| 1.39 Skip underwater postprocess above the waterline | applied 2026-07-25, lib rebuilt (wiRenderPath3D.cpp) | The underwater full-screen pass (dispatch + ClearUAV + barriers, ~0.15ms GPU) ran whenever the OCEAN existed, even with the camera hundreds of meters above water — it only affects a SUBMERGED camera. Skip when `camera.Eye.y > waterHeight + 200` (margin covers wave displacement); chain-safe (postprocess chain treats a skipped stage as never existing). `wi::gg_skip_underwater_above_water` (false = stock) |
| 1.40 Command-list merges (submit-fragmentation cut) | applied 2026-07-25, lib rebuilt (wiRenderPath3D.cpp) | GG-specific, master switch `wi::gg_render_merge_lists` (false = stock structure). Each `BeginCommandList` is a real allocator+list and every cross-list wait splits the queue submission — ~17 lists / ~15 submits per frame leave GPU idle bubbles between submits (measured frame 12.6ms vs CPU 9.8 / GPU 9.8 busy). Merges: (a) the occlusion-culling pass records at the TAIL of the main prepass list (same depth target, same-list sequencing; the ocean's wait retargets to the prepass list); (b) transparents + postprocess chain record into ONE list, with `RenderCameraComponents` hoisted before it (camera render-to-texture output is consumed by materials next frame either way). Harness `SET_MERGELISTS 0|1` |
| 1.41 ShaderMaterial recompose cache | applied 2026-07-25, lib rebuilt (wiScene_Components.h, wiScene.cpp, wiResourceManager.h/.cpp) | GG-specific, master switch `wi::scene::gg_material_cache` (false = stock). `RunMaterialUpdateSystem` ran the FULL `WriteShaderMaterial` recompose (half-packs, sin/cos, ~16 per-slot `GetDescriptorIndex` lookups) for every material every frame (2225 on TESTPRO1). The composed struct only changes when the material is dirty or a texture-streaming event moves GPU descriptors. New `wi::resourcemanager::gg_streaming_descriptor_epoch` bumps on BOTH streaming mutation sites (texture-object replacement and min-lod subresource recreation) — the cache is valid only within an epoch, so streamed descriptor churn can never serve a stale index. Recompose triggers: dirty flag, epoch move, per-material staggered 64-frame heartbeat (insurance for direct field writes that skip SetDirty), or video/camera slot attachments; otherwise a 256-byte memcpy of the cached composition into the (per-frame cycled) mapped array — the mapped write itself always happens. Harness `SET_MATCACHE 0|1` |
| 1.42 SetDirty on terrain VT rebind | applied 2026-07-25 in Wicked commit `683fa37c`, lib rebuilt (wiTerrain.cpp) | The terrain virtual-texture (re)bind block writes descriptor indices / `texMulAdd` / `lod_clamp` straight onto the chunk `MaterialComponent` WITHOUT `SetDirty`. Harmless before 1.41 (every material recomposed every frame); with the 1.41 cache the STALE composition could be served for up to 63 frames after any rebind (load/reload storms, resolution-ring crossings at `gg_near_ring_dist`, `gg_tail_invalid` retries), pointing a chunk at a pooled residency/feedback map possibly already re-issued to ANOTHER chunk = grey / foreign chunk textures. One `material->SetDirty()` at the end of the bind block closes it. Found + adversarially verified by the terrain-corruption audit workflow |
| 1.43 `GraphicsDevice::FlushDeferredDestroys` | applied 2026-07-26 in Wicked commit `3af8655c` (wiGraphicsDevice.h, wiGraphicsDevice_DX12.h/.cpp) — **PRESENT BUT DISABLED AT THE CALL SITE, do NOT re-enable without fixing the frame-boundary problem** | Virtual (default no-op), DX12 impl = full-queue `WaitForGPU` + `allocationhandler->Update(FRAMECOUNT+1, 0)` = release the ENTIRE deferred-destroy backlog immediately (returns the drained count). Intended to give in-place level reloads a cold-load-clean device. **Result: it did NOT fix any corruption (~16.5k items drained per reload, artifacts unchanged) and is a SUSPECTED DEVICE-REMOVAL CAUSE** — the game calls it from `gridedit_load_map`, which runs mid-frame while the editor still pumps/renders, so freeing everything can destroy resources that recorded-but-unsubmitted command lists reference (user hit `DXGI_ERROR_DEVICE_HUNG` / `Device Lost on Present 0x887a0006`; harness repro then crashed 2/2 during load). Game-side gate `gg_enable_deferred_flush=false` (game `2ee384f9`). Kept in the engine as a correct-if-called-at-a-frame-boundary primitive |
| 1.44 Texture-streaming reload guard | applied 2026-07-26 in Wicked commit `3af8655c`, lib rebuilt (wiResourceManager.h/.cpp) | GG-specific. `wi::resourcemanager::GGReloadGuardBegin()` = pause streaming (`gg_streaming_paused`) + `Wait(streaming_ctx)` (join the in-flight streaming job) + DROP `streaming_texture_replacements` (they were computed against the DYING session's resources) + clear `streaming_texture_jobs`; `GGReloadGuardEnd()` resumes. Called by the game around an in-place level reload. **Why: the streaming system was proven to write ANOTHER FILE'S BYTES into live textures across reloads** — deterministic 'blue palms' (their own normal-map bytes showing as basecolor) at reload>=2, healed IN PLACE by `SetOutdated`+re-`Load` (harness `REUPLOAD_TEXTURE`), and eliminated entirely with streaming paused (harness `SET_STREAMING 0`). `gg_streaming_paused` doubles as that A/B knob. Full forensic chain in the game repo's memory notes |
| 1.45 Join the async VT job before terrain teardown | applied 2026-07-26 in Wicked commit `12a64a39`, lib rebuilt (wiTerrain.cpp) | **Fixes a real use-after-free that predates the P.5 perf push.** The terrain freed `VirtualTexture` objects while the async VT job could still be running — that job holds raw VT pointers (last frame's `virtual_textures_in_use`) and mutates the shared tile atlas, so `free()`+`erase` underneath it corrupts GPU sparse-resource bookkeeping. Now `wi::jobsystem::Wait(virtual_texture_ctx)` runs first at all three teardown sites: the per-frame chunk-removal branch (dominant during sustained camera travel — one-shot `gg_vt_teardown_joined` flag so only frames that actually remove pay, others keep full job overlap), `Generation_Restart` (level switch / material re-setup), and the `terrainEntity==INVALID` path. Matches the user repro: fly the editor camera at full speed far outside the playable area, leave without saving, load a different level -> missing chunk underfoot + far-field geometry corruption (+ one AMD `DEVICE_HUNG`); a clean relaunch of the same level always rendered fine. **Explains the long-standing 'occasional far-distance corruption'. NEEDS EXTENDED USER SOAK to confirm** |
| 1.46 GPU suballocator guards + op-log tripwire | applied 2026-07-26 (wiAllocator.h), lib rebuilt | **The mesh/hair GPU suballocator (one shared global buffer, every mesh a suballocated alias — wiRenderer `SuballocateGPUBuffer`) is the PRIME SUSPECT for the travel-churn geometry corruption**: one instrumented island load caught **596 overlapping grants + 172 out-of-bounds grants** (out-of-bounds = the `CreateAliasingResource` E_INVALIDARG → `DXGI_ERROR_INVALID_CALL` device-removal observed same day; overlap = two live meshes sharing bytes = the stretched-blade/missing-chunk stomps with all CPU records correct). INTERMITTENT: subsequent loads + 3 churn-reload cycles (27K ops) replayed perfectly clean, storm correlated with the shader-recompile-burdened first load (timing-sensitive race). Guards: (a) out-of-range grants rejected gracefully (caller falls back to standalone buffer instead of device removal); (b) live-range overlap tripwire + full A/D/F/R op log to `alloc_tripwire.txt` (replayer: scratchpad `replay_alloc.py`); (c) freed ranges held `gg_deferred_extra_hold=8` extra frames (mid-frame load pumps make the bare buffercount window unsafe); (d) `is_empty()` storageReport now under the allocator lock (was a racy read). Knobs: `wi::allocator::gg_alloc_tripwire`, `gg_deferred_extra_hold`. |
| 1.46b Allocator node/thread logging + Reset steal-guard | applied 2026-07-26 in Wicked commit `1b7709ae` (wiAllocator.h), lib rebuilt | **ORGANIC CAPTURE CONFIRMED THE ALLOCATOR (user session, plain island load): 1369 overlap + 411 OOB grants, op history shows the free-list bin SELF-LOOPING — offset 129 granted 20+× consecutively with no frees between = aftermath of a double-free double-inserting a node.** VERIFY_MESH on that session: 294/1012 terrain chunk buffers held FOREIGN bytes (ib containing float vertex data), healed in place, FPS 7.3→64.7. This delta: (a) op log lines gain `m=<nodeindex> t=<threadid>` so the next capture shows the node-level double-free + colliding threads directly (`replay_alloc.py` in GameGuru Core models nodes too); (b) `Allocation::Reset()` atomically steals `internal_state` (candidate mechanism: racing double-Reset on the same object double-decrements the refcount → frees someone else's node). |
| 1.47 **OffsetAllocator port bug — THE reload/travel-churn corruption ROOT CAUSE** | applied 2026-07-26 (Utility/offsetAllocator.cpp), lib rebuilt | **UPSTREAM-WICKED-WORTHY (port bug, like 1.19).** `insertNodeIntoBin` recycles a node from the freelist; sebbbi's original resets the whole node via aggregate assignment (`m_nodes[i] = {.dataOffset=..., .dataSize=..., .binListNext=...}` → used/binListPrev/neighbor* implicitly cleared); Wicked's MSVC-compat port rewrote it as THREE FIELD WRITES, leaving `used`, `binListPrev`, `neighborPrev/Next` STALE on every recycled node. Consequences: refused neighbor merges, unlinks through stale binListPrev, self-looped bin lists (same node granted dozens of times consecutively), overlapping live suballocations → one mesh's upload stomps another's GPU bytes with all CPU records correct = the slabs/wedges/missing-chunks/blue-palms-family corruption + the CreateAliasingResource E_INVALIDARG device removals. **Proven end-to-end**: lossless 1.46c op capture from the user's triple-level repro → eventual first-divergence at a legal free (op #3592) via a standalone testbench replaying the REAL vendored allocator with per-op node-graph integrity checks → one-line fix (`m_nodes[nodeIndex] = Node();` before the field writes) → same 7629-op workload replays with ZERO integrity failures. Fix = full node reset before reuse; testbench archived in game repo `GameGuru Core/alloc_testbench/`. |
| 1.48a Submit-tail phase attribution | applied 2026-07-26 in Wicked commit `8209a359` (wiGraphicsDevice_DX12.cpp), lib rebuilt | Instrumentation only. `SubmitCommandLists` phase timers (close+execute / frame fences / Present / cross-queue sync / end stall) + per-frame counts (lists closed, `ExecuteCommandLists` batches, dependency-carrying lists) published as `wi::graphics::gg_submit_*`, printed by the harness `SUBMIT_PHASES_MS` line. **FINDING (kills the P.5 'submit batch cost' theory): the ~2-4ms tail is ~95% the END STALL — the next-buffer frame fence, i.e. the GPU is still busy. The TESTPRO1 editor frame is GPU-WALL-bound (~13.6ms wall vs ~11.7 GPU busy w/ profiler), which retroactively explains BUFFERCOUNT 2→3 flat, resolutionScale flat, and hairskip's clean ±1.4ms FPS coupling. CPU-side cuts yield ~zero FPS here until GPU wall < CPU wall** |
| 1.48b Single-queue A/B knob | applied 2026-07-26 in Wicked commit `8209a359` (wiGraphicsDevice_DX12.cpp), lib rebuilt — **default OFF, measured NEGATIVE, keep as probe** | `wi::graphics::gg_single_queue`: `BeginCommandList(COMPUTE/COPY)` → GRAPHICS + same-queue `WaitCommandList` fences dropped (correct because waits only point at LOWER list ids and submission is in id order). Batches 15→1, deps 12→0 — and FPS 71.5→66.8: **the async-queue overlap genuinely wins ~1ms of GPU wall on the RX 9060 XT; cross-queue fences are NOT bubbles here.** Harness `SET_SINGLEQUEUE 0|1` for low-end-GPU probes |
| 1.48c Lean-async queue routing | applied 2026-07-26 (wiGraphicsDevice_DX12.cpp, wiRenderPath3D.cpp), lib rebuilt — **default OFF, measured NEGATIVE, keep as probe** | `wi::graphics::gg_lean_async`: the four tiny helper lists (terrain VT copy-pages, ocean sim + readback copy, VT tile-request + writeback) run on GRAPHICS while the two big compute lists (prepare-async, compute-effects) stay async; same-queue fences dropped (in-order-submission guarantee). Deps 12→~5 — and clean steady-state ABAB says **71.6 → 67.5 FPS (−0.9ms)**: even the tiny lists' async/DMA overlap is worth more than their fence cost on the RX 9060 XT. Together with 1.48b: Wicked's cross-queue split structure is FULLY EARNING ITS KEEP on this hardware — submission overhead is a dead end for perf here. Harness `SET_LEANASYNC 0|1` |
| 1.49(+b) Grass strand LOD (far-distance decimation + width compensation) | 1.49 applied 2026-07-26 in Wicked commit `8209a359`; **1.49b rework applied 2026-07-27** (ShaderInterop_HairParticle.h padding fields, hairparticle_simulateCS.hlsl, wiHairParticle.cpp), lib rebuilt — **DEFAULT ON since 2026-07-27 (user-confirmed in a game-mode walk with the game-side tier coupling — no square pops, no flicker — then explicitly requested as default; `SET_GRASSLOD 0` = kill switch, also pulls the AUTO tiers back to stock)** | `wi::gg_grass_lod` + step2/step4 viewDistance fractions + width boost. GG grass systems only (`grass_type != 0`), character hair untouched; dropped strands take the existing 1.25 early-out (sim work saved too); draw savings flow to EVERY pass reading the culled buffer (prepass/opaque/shadow). MEASURED (v1) TESTPRO1 gate view, clean steady-state ABAB ×2: **72.1 → 77.2 FPS (+0.9ms)** at 0.35/0.60. **v1 FIELD FAILURE (user-reported, same day): two hard rings meant camera motion swept a synchronized band of strands flipping normal↔wide = two-shade shimmer in the mid field, decaying with the editor camera's glide after stopping. 1.49b fix (verified by a 3-lens adversarial workflow BEFORE deploy): (a) per-strand hash-jittered drop radii (±15% band) — transitions become sparse single-blade events, no ring; (b) EXACT hyperbolic coverage ramp 1/(1−0.5t) aligned to the drop window (the review's coverage lens proved v1's linear ramp left a static +16%/−26% lush/sparse ring pattern and boost 1.8 a permanent −10/−19% deficit); (c) boost default → 2.0 = coverage-neutral in expectation, <2.0 = documented deliberate thinning; (d) step4<=0 hardening (was: drops 25% of strands at ALL distances if a future caller zeroed it) + host-side step4>=step2 clamp. Residual accepted risks (review-graded minor/theoretical): un-dropping strand shows one stale-vb_pre frame (same class as the stock cull edge), threshold blades can re-blink under to-and-fro camera dither (single blades, width-compensated).** Harness `SET_GRASSLOD <0|1> [f2 f4 boost]`; until the next game relink the EXE's default boost is still 1.8 — use `SET_GRASSLOD 1 0.35 0.60 2.0` for the exact-neutral config |
| 1.50 GG grass wetmap opt-out (dark-on-reveal fix) | applied 2026-07-27 (ShaderInterop_Renderer.h `WetmapPush.padding` → `gg_force_dry`, wetmap_updateCS.hlsl early-out, wiRenderer.cpp knob + RefreshWetmaps hair-loop gate), lib rebuilt — **default ON (grass force-dried); USER-CONFIRMED 2026-07-27 evening (revealed grass now full brightness immediately, no dark fade)** | **ROOT CAUSE of the "grass renders flat-DARK on reveal, then VERY slowly brightens over 15-30s" editor/game artifact (and the shade component of the original two-shade flicker reports)**: the GG perf early-out in `hairparticle_simulateCS` skips ALL vertex writes for strands that are distance/frustum/1.49-LOD culled, so a never-yet-drawn strand's entries in the alternating ping-pong position buffer stay at the cleared raw ZERO = world (0,0,0) (GG grass = FP32 positions, identity instance transform) = deep below the island waterline. `RefreshWetmaps` (every frame while an ocean exists — MAX enables the Wicked ocean whenever editor water is on) dispatches over ALL vertices of every visible hair system, reads those zeros as submerged, and ratchets them to wet≈0.8 with drying disabled (`max()` + `drying_enabled=false`); `hairparticlePS` renders wet strands with `albedo = lerp(albedo, 0, wet)` = flat ×0.2 darkening (no shadow shape), drying only ticks while the strand is DRAWN at 0.02–0.08/s = the 15-30s fade. Fast-motion flicker = strands crossing the jittered LOD radii swapping between the wet(dark, undrawn) and dried(normal, drawn) populations. Fix: GG grass (`grass_type != 0`) wetmap dispatches carry `gg_force_dry=1` → CS writes wet=0 and exits — grass permanently dry = DX11 parity (DX11 grass never had wetmaps; kelp/seaweed lose wet-darkening too, acceptable). Character hair (`grass_type==0`) keeps stock wetting. `wi::renderer::gg_grass_wetmap=true` restores stock wetting for the bug demo; harness `SET_GRASSWET <0|1>` (0 = default force-dry; recovery on toggle-back is instant for visible systems). Diagnosed by a 4-reader brightness-pipeline mapping workflow, all links verified in code |
| 1.51 DRED post-mortem capture in Release (dred.txt file flag) | applied 2026-07-27 (wiGraphicsDevice_DX12.cpp), lib rebuilt — **armed on the dev rig for the canyon TDR hunt** | Stock Wicked only enables DRED (D3D12 auto-breadcrumbs + page-fault tracking) together with the debug layer (`validationMode != Disabled`), so a Release device removal dumps nothing — exactly what happened with the 2026-07-27 23:07 `DXGI_ERROR_DEVICE_HUNG` TDR during the Canyon Adventure terrain build (unreproduced ×2 afterwards; allocator/streaming/quiesce tripwires all clean; evidence archived in scratchpad crash_2307). Now: an empty **`dred.txt` next to the EXE** arms DRED settings at device creation WITHOUT the debug layer (`gg_dred_armed`, launch log line confirms; delete the file to disarm — breadcrumbs cost a few % GPU while armed), and OnDeviceRemoved APPENDS the full report (removal reason, last in-flight op per command list, page-fault VA + owning live/freed resources) to exe-dir **`dred_report.txt`** — persists across the process Exit(), backlog flush not required. Complements AMD Radeon GPU Detective (driver-level capture) for the same hunt |
| 2.1 hairparticlePS white | reverted 2026-06-18 | none — shader back to upstream state |
| 2.2 hairparticle_simulateCS overrides | reverted 2026-06-18 | none — shader back to upstream state |
| 2.3 hairparticlePS_prepass alpha=1 | reverted 2026-06-18 | none — shader back to upstream state |

Update this table any time we add, revert, or commit a change to the
`WickedEngineDX12` clone.
