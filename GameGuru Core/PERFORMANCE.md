# Performance Profiling & Optimization Plan

## Stage P.6 — 2026-07-26 autonomous "120 FPS push": the frame is GPU-WALL-BOUND; safe ceiling ≈ 77-80, the rest is mapped

**Headline finding (delta 1.48a phase instrumentation, corrects the P.5 model):** the ~3ms
"App-SubmitPresent tail" is ~95% the END STALL — the next-buffer frame fence, i.e. the GPU is still
busy when the CPU finishes. **The TESTPRO1 temple-gate editor frame is GPU-WALL-bound at ~13.6ms**
(GPU busy ~11.6 w/ profiler + cross-queue latency), CPU wall ~10.5. This retroactively explains the
P.5 anomalies: BUFFERCOUNT 2→3 flat, resolutionScale flat, hairskip's clean ±1.4ms FPS coupling.
**Consequence: CPU-side cuts yield ~ZERO FPS on this rig until the GPU wall drops below ~10.5ms.**

**Steady state (6-min settle curve, parked camera): 72 ± 1 FPS.** GPU shares by elimination probe:
grass draw+sim ~2.0ms, hair sim cadence ~1.7ms (BANNED — 1.37 flicker conviction, needs the queued
PIX hunt), tree pool ~1.6ms (user's density choice). GPU busy: Opaque 2.73, HairSim 2.69, Z-Prepass
2.21, Skinning 0.75, SceneMIP 0.54, Occlusion 0.75, UpdateBuffers 0.53, Shadowmap 0.36+1.04(stagger).

**Landed (all knob-gated, see WETEST.md):**
| Delta | What | Verdict |
|---|---|---|
| 1.48a | SubmitCommandLists phase timers + batch/dep counters (`SUBMIT_PHASES_MS`) | instrumentation; found the GPU-wall truth |
| 1.48b | `SET_SINGLEQUEUE` — everything on graphics, no fences | **NEGATIVE −4.7 FPS** (async overlap is real) — default OFF |
| 1.48c | `SET_LEANASYNC` — only tiny helper lists on graphics | **NEGATIVE −4 FPS** — default OFF. Submission overhead = dead end on this GPU |
| 1.49 | Grass strand LOD: 2×/4× far decimation + width compensation (`SET_GRASSLOD`) | **+5 FPS (72.1→77.2)** steady-state ABAB; visually clean at test camera; **default OFF — user's visual call** |
| game | Terrain idle gate: Generation_Update 1-in-8 when quiescent (`SET_TERRAINIDLE`) | Update-Terrain 0.92→0.09ms, CPU frame −~1.4ms; 0 FPS (GPU-bound) = CPU headroom; default ON |

**Also established:** sculpt+undo grass dropout (SCENE_HAIRS −3 until camera moves) is PRE-EXISTING
(reproduced with the gate hard-off) — spun off as its own task. `SET_TREES draw 0` is ONE-WAY
(HideAll mutates source flags; reload restores).

**The honest arithmetic to 120 FPS (8.33ms) from 12.9ms (grass LOD on):** GPU must lose ~4.5ms more —
hair sim 1.7-2.7 (blocked on the PIX flicker hunt), trees 1.6 (visual trade / impostor work), remaining
opaque+prepass ~3.5 (structural drawcall/LOD surgery), cross-frame GPU overlap 1-2ms (Wicked's
end-of-frame all-queue sync forbids it — removing it is a cross-frame resource-race hazard class);
AND CPU wall 10.5 must drop ≤8.3 (S1 anim 2.2, S4 1.5 — needs deterministic meshlet offsets first,
VisMain 1.3, RenderWait 1.35 — all structural). None of these fit "safe + unattended".

## Stage P.5 — 2026-07-25 autonomous deep-optimization session: 53.7 → ~77 FPS (+45%) on the TESTPRO1 temple gate

**The months-old "fixed ~9.5ms Render CPU pole" is SOLVED.** New per-stage/per-job instrumentation
(Wicked delta 1.32) attributed it in one measurement: the terrain virtual-texture background job ran
**16.4ms EVERY frame on one worker thread** (14.2ms = an uncached memcpy loop rewriting every resident
chunk's FULL page table each frame + 2.1ms free-list rebuild+sort), and the render path blocked on it.
Delta 1.33 (incremental VT bookkeeping: per-VT dirty tracking with steal-victim marking, lazy freesort)
took it to **0.4ms** — Render CPU 9.72 → 1.71ms, 52 → 62-70 FPS from this alone.

**All deltas landed this session (each with a runtime A/B knob, see WETEST.md):**
| Delta | What | Measured |
|---|---|---|
| 1.33 | Incremental terrain-VT bookkeeping (`SET_VTINC`) | VT job 16.4→0.4ms; 52→62-70 FPS |
| 1.34 | Dead anim-dependency inserts removed | AnimDeps 0.82→0.00ms |
| 1.35 | Frustum-visibility animation pause (`SET_ANIMVIS`) | +2 FPS; unseen characters stop evaluating |
| 1.36 | Subtree-parallel hierarchy update (`SET_HIERLO`) | Scene-S2 4.97→2.22ms; +2-5 FPS |
| 1.37 | Hair/grass sim static-skip + wind cadence (`SET_HAIRSKIP`) | 70.9↔78.3 FPS A/B; sway alive in slow-mo |
| 1.38 | Object-update decompose → row lengths | 8722 decomposes/frame removed |
| 1.39 | Underwater postprocess skip above waterline | −0.15ms GPU |
| 1.40 | Command-list merges (`SET_MERGELISTS`) | NEUTRAL (kept — less overhead) |
| 1.41 | ShaderMaterial recompose cache + streaming epoch (`SET_MATCACHE`) | NEUTRAL here (kept — correct, helps material-heavy scenes) |
| game | Editor entity_loopanim half-rate | Logic - common_loop 1.35→0.71ms |
| game | Harness ImGui-null-context crash FIX | the recurring silent test-loop deaths |

**Decisive negative results (do not re-chase):** resolutionScale 0.85 ≈ FLAT (the GPU cost is grass
VERTEX/raster work — 2M strands of billboard quads in prepass+opaque — not pixels); BUFFERCOUNT 2→3
zero effect (reverted; the frame tail is NOT a fence wait); command-list merges no FPS on this rig.

**The measured frame at session end:** CPU-in-frame ~10.5ms + **App-SubmitPresent tail ~3ms**
(queue submits + Present on the main thread — new `APP_SUBMIT_PRESENT_MS` harness readout) + GPU ~9.8ms.

**The documented path to 120 FPS (all structural — do SUPERVISED):**
1. Off-thread `SubmitCommandLists` or submit-batch reduction (the VT copy/compute cross-queue splits) — ~2-3ms.
2. Grass vertex LOD (strand count / draw distance by distance) — ~1.5-2ms GPU; visual trade to tune.
3. Update residue: mesh-geometry recompose (S2) + object instance-write staging buffer (S4) — ~1.5-2ms.
4. Frame pipelining (Update(N+1) overlapped with GPU(N) wait) — the deep one.

## Status Summary (as of 2026-07-17)

| Phase | Topic | Status |
|---|---|---|
| 1–2 | Profiler enablement + scene baseline | ✓ COMPLETE |
| 3 | A/B feature-toggle measurements (shadows = +30 FPS, etc.) | ✓ COMPLETE |
| 4–5 | DX11 vs DX12 comparative measurements | ✓ COMPLETE |
| 6–7 | Animation system deep-dive — root causes identified | ✓ COMPLETE |
| 8 | Engine-side animation caching (ScanAnimationDependencies, keyframe search) | PLANNED — see "Active Performance Targets" below |
| 9 | Animation culling via `GGAnimBridge` pause/play | ✓ COMPLETE (`039ee1da`, `89873913`) — Update-Wicked 25.81→11.36 ms (−56%), FPS 27→36 (+33%) |
| 10 | Performance Data panel cascading-duplicates display bug | ✓ RESOLVED (`c4d81543`) — diagnostic from `5233fb3c` no longer needed |

The next perf focus is the **Active Performance Targets** listed near the end of this document.

## Problem Statement

The same scene renders at **~135 FPS** in the new DX12 Wicked Engine build vs **~450 FPS** in the old DX11 build (which also included trees and grass). That's a **3.3x slowdown**.

Initial diagnosis with the O key toggle (terrain on/off) showed terrain rendering itself only costs ~30 FPS (135→165). The I key (normal-only visualization, bypassing all texturing/lighting) showed no FPS improvement, confirming texturing is not the bottleneck either. **Most of the cost is elsewhere in the pipeline.**

## Current Rendering Configuration

| Setting | Value | Notes |
|---------|-------|-------|
| MSAA | 1 (off) | `setMSAASampleCount(1)` |
| FXAA | off | Comment says "already have MSAA 8" but MSAA is 1 |
| SSR | off | |
| Reflections | on | Planar reflections enabled |
| Bloom | on | Threshold 2.0 |
| Shadows | on | Sun casts shadows, cascaded shadow maps |
| Light Shafts | on | Sun volumetrics enabled |
| Eye Adaptation | off | |
| AO | MSAO | Multi-Scale Ambient Occlusion |
| Realistic Sky | on | Atmospheric scattering model |
| Volumetric Clouds | on | Coverage 0.0 (effectively empty but pipeline still runs) |
| Occlusion Culling | off | "Lag is clearly visible, causing obvious gaps" |
| Tessellation | off | "Doesn't work like this, has to be set per mesh" |
| Volumetrics on Sun | on | `SetVolumetricsEnabled(true)` |

**Critical note:** The built-in profiler overlay (`wi::profiler::DrawData()`) is DISABLED because it causes GPU access violations during `Compose()`. Profiler timing data is still collected; it must be read via `GetTextData()` or `GetCPUFrameTime()`/`GetGPUFrameTime()`.

## Phase 1: Enable Profiler Data Collection via Automation Harness

The Wicked Engine profiler is already fully instrumented with ~95 GPU/CPU ranges. The automation harness already supports `TOGGLE_PROFILER` and `GET_PERF_DATA` commands that return `wi::profiler::GetTextData()`.

### Step 1.1: Add P key toggle for profiler + file dump

Add a **P key** toggle in `GGTerrainWicked_Update()` (alongside U/I/O keys) that:
1. Calls `wi::profiler::SetEnabled(true/false)`
2. When disabling, writes `wi::profiler::GetTextData()` to `terrain_perf.log` in the EXE directory
3. This gives us a text dump of all 95+ profiled ranges with averaged timings

**File:** `Guru-WickedMAX/GGTerrain/GGTerrainWicked.cpp`

### Step 1.2: Collect baseline profiler dumps

Using the automation harness or P key:
1. Load a representative demo level (e.g., Island Showdown)
2. Position camera at a consistent viewpoint
3. Enable profiler, wait 5 seconds for averages to stabilize
4. Disable profiler → dump to file
5. Repeat with terrain OFF (O key) for comparison

### Expected profiler output categories

Based on Wicked Engine's instrumented ranges, the dump will show timing for:

**CPU ranges:**
- `Frustum Culling`
- `Shadowmap packing`
- `Update Buffers (CPU)`
- `BLAS Update (CPU)`, `TLAS Update (CPU)`
- `Shadowmap Rendering (CPU)`
- GameGuru custom: `Update - Logic`, `Update - Terrain`, `Update - Particles`, `Update - Emitters`, `Update - Render`, `Update - Wicked`

**GPU ranges:**
- `Update Buffers (GPU)` — scene constant buffer upload
- `Skinning and Morph` — vertex skinning
- `Terrain - UpdateVirtualTexturesGPU` — VT atlas compute dispatches
- `Shadowmap Rendering (GPU)` — shadow map draw calls
- `Entity Culling` — tiled light culling
- `MSAO` — multi-scale ambient occlusion
- `Visibility (Prepare/Surface/Shade/Velocity)` — visibility buffer passes
- `Bloom` — bloom post-process
- `Luminance` — eye adaptation luminance
- `SkyAtmosphere Textures` — realistic sky compute
- `Volumetric Clouds` — cloud raymarching (even with 0% coverage)
- Various post-process passes

## Phase 2: Systematic Feature Toggle Keys

Add toggle keys to disable expensive features one at a time, measuring FPS impact of each. All toggles go in `GGTerrainWicked_Update()` using the same pattern as U/I/O.

| Key | Feature | API Call | Expected Cost |
|-----|---------|----------|---------------|
| O | Terrain (already done) | `SetRenderable(false)` + skip Generation | ~30 FPS |
| P | Profiler on/off + dump | `wi::profiler::SetEnabled()` | Diagnostic |
| 1 | Shadows on/off | `wi::renderer::SetShadowsEnabled()` | HIGH — cascaded shadow maps re-render entire scene |
| 2 | AO on/off | `masterrenderer.setAO(AO_DISABLED/AO_MSAO)` | MEDIUM — full-screen compute pass |
| 3 | Bloom on/off | `masterrenderer.setBloomEnabled()` | LOW-MEDIUM — downsample chain |
| 4 | Volumetric Clouds on/off | `weather.SetVolumetricClouds()` | MEDIUM — raymarching even at 0% coverage |
| 5 | Realistic Sky on/off | `weather.SetRealisticSky()` | MEDIUM — atmospheric scattering LUTs |
| 6 | Light Shafts on/off | `masterrenderer.setLightShaftsEnabled()` | LOW-MEDIUM |
| 7 | Reflections on/off | `masterrenderer.setReflectionsEnabled()` | MEDIUM — re-renders scene for planar reflections |
| 8 | Volumetric Lights on/off | `lightSun->SetVolumetricsEnabled()` | LOW-MEDIUM |

**File:** `Guru-WickedMAX/GGTerrain/GGTerrainWicked.cpp`

### Implementation notes

- Each toggle needs access to the appropriate object (renderer globals, masterrenderer, weather, sun light)
- Renderer globals (shadows) are simple static calls
- Weather/light components need entity lookup from the scene
- masterrenderer settings need an extern or global pointer
- Print toggle state to debug output so user knows what changed

## Phase 3: Automated A/B Test Protocol

Once toggle keys are implemented, run a systematic test:

```
Test Protocol:
1. Load Island Showdown demo, navigate to a fixed camera position
2. Record baseline FPS (all features ON) — should be ~135 FPS
3. For each feature toggle (1-8):
   a. Toggle feature OFF
   b. Wait 3 seconds for FPS to stabilize
   c. Record FPS
   d. Toggle feature back ON
4. Record cumulative: toggle ALL features off, record FPS
5. Compare cumulative OFF vs DX11 baseline (450 FPS)
```

This can be done manually with the number keys, or automated via the harness:
```
TOGGLE_PROFILER
PRESS_KEY 49    (1 = shadows off)
GET_PERF_DATA
PRESS_KEY 49    (1 = shadows on)
... repeat for each key ...
```

## Phase 4: Analysis Framework

### Known suspects (high cost, ordered by likely impact)

1. **Shadow Maps** — The sun has `SetCastShadow(true)`. Cascaded shadow maps re-render the entire scene multiple times (typically 3-4 cascades). In the old DX11 engine, shadow quality/resolution may have been much lower. This is often the single biggest cost in outdoor scenes.

2. **Volumetric Clouds pipeline** — `SetVolumetricClouds(true)` with coverage 0.0 still runs the full raymarching pipeline (allocates textures, dispatches compute shaders, composites). The old engine didn't have this.

3. **Realistic Sky** — Atmospheric scattering LUT generation is a multi-pass compute pipeline. The old engine used a simple skybox.

4. **MSAO (Ambient Occlusion)** — Full-screen multi-scale compute pass. May not have existed in DX11 build.

5. **Planar Reflections** — `setReflectionsEnabled(true)` re-renders the entire scene from a reflected camera. Even if no reflective surfaces exist, the culling + setup cost is paid.

6. **DX12 API overhead** — DX12 has higher CPU-side overhead than DX11 for descriptor management, root signature binding, and pipeline state management. With the Wicked Engine's bindless design, this may be a fixed per-frame cost.

7. **Visibility Buffer pipeline** — The modern Wicked Engine uses a visibility buffer approach (primitive ID prepass → compute shading). This is more scalable but has fixed overhead from multiple passes that the old forward renderer didn't have.

8. **Virtual Texture system** — Even with blendmap terrain, the VT atlas update compute shaders run every frame for residency maps, tile requests, and page allocation.

### DX12 vs DX11 structural differences

The old DX11 Wicked Engine was a simpler forward renderer. The new DX12 engine adds:
- Visibility buffer / deferred compute shading pipeline
- Bindless resource management (descriptor heaps)
- Async compute queues (VT updates, AO, culling overlap)
- GPU-driven rendering (indirect draws, mesh shaders where available)
- More sophisticated post-processing (volumetric clouds, realistic sky, FSR2 support)
- GPU timestamp queries + profiler infrastructure

Some of these have fixed per-frame costs regardless of scene complexity.

## Phase 5: Optimization Targets

Once profiling data identifies the top bottlenecks, potential optimizations:

### Quick wins (configuration changes, no code)
- Disable volumetric clouds entirely if coverage is 0%
- Disable planar reflections if no reflective surfaces exist in scene
- Reduce shadow map resolution or cascade count
- Switch AO to SSAO (cheaper) or disable entirely
- Disable realistic sky if visual difference is acceptable

### Medium effort (code changes in GameGuru)
- Conditionally enable features based on scene content (e.g., only enable reflections if water is present)
- Reduce shadow cascade count for editor mode
- Lazy-enable volumetric clouds only when weather system activates them
- Skip VT GPU updates when no new tile requests exist

### Larger effort (Wicked Engine modifications)
- Early-out in volumetric cloud pipeline when coverage is 0%
- Reduce atmospheric scattering LUT resolution
- Add LOD-based shadow culling (skip distant objects in shadow maps)
- Profile and optimize the visibility buffer pipeline
- Consider a simpler forward path option for less complex scenes

## Debug Key Reference

| Key | Function | Status |
|-----|----------|--------|
| U | Wireframe overlay | Implemented |
| I | Normal visualization (skip texturing) | Implemented |
| O | Terrain on/off (all terrain systems) | Implemented |
| P | Profiler toggle + file dump | Implemented |
| 1 | Shadows on/off | Implemented |
| 2 | AO on/off | Implemented |
| 3 | Bloom on/off | Implemented |
| 4 | Volumetric Clouds on/off | Implemented |
| 5 | Realistic Sky on/off | Implemented |
| 6 | Light Shafts on/off | Implemented |
| 7 | Reflections on/off | Implemented |
| 8 | Volumetric Lights on/off | Implemented |
| Y | Old terrain toggle | Pre-existing |

---

## Phase 3 Results: A/B Feature Toggle Test (2026-03-03)

**Test scene:** Switch Escape demo, test game mode
**Hardware:** AMD Radeon RX 9060 XT
**Method:** Automated via `perf_test.sh` — baseline (3 samples), then each feature toggled OFF (3 samples), then restored

### FPS Impact per Feature

| Key | Feature | FPS OFF (samples) | FPS ON (baseline) | Delta | Impact |
|-----|---------|-------------------|-------------------|-------|--------|
| — | Baseline (all ON) | — | 119, 122, 125 | — | — |
| 1 | Shadows | 150, 152, 154 | ~122 | **+30** | **MAJOR** |
| 2 | AO (MSAO) | 124, 126, 127 | ~122 | +4 | Minor |
| 3 | Bloom | 122, 124, 125 | ~122 | +2 | Negligible |
| 4 | Volumetric Clouds | 123, 125, 126 | ~122 | +3 | Minor |
| 5 | Realistic Sky | 121, 123, 124 | ~122 | +1 | Negligible |
| 6 | Light Shafts | 122, 124, 125 | ~122 | +2 | Negligible |
| 7 | Reflections | 124, 126, 128 | ~122 | +5 | Minor |
| 8 | Volumetric Lights | 122, 124, 125 | ~122 | +2 | Negligible |
| All | All 1-8 OFF | 143, 145, 147 | ~122 | +23 | Combined |

**Key finding:** Shadows alone account for +30 FPS — more than all other features combined (+23 FPS with ALL off). This is because shadow map rendering is overwhelmingly CPU-bound (see profiler data below).

### Full Profiler Dump (P key, Switch Escape test game)

```
CPU Frame: 8.30 ms
    Update - Logic: 1.57 ms
        Update - Logic - LUA: 0.54 ms
        Update - Logic - Physics: 0.76 ms
        Update - Logic - AI: 0.00 ms
        Update - Logic - Objects: 0.05 ms
    Fixed Update: 0.01 ms
    Render: 2.52 ms
        Shadowmap Rendering (CPU): 2.25 ms
        Shadowmap packing: 0.01 ms
    Update - Wicked: 3.79 ms
        Update Buffers (CPU): 0.07 ms
        Spline Update: 0.00 ms
        Spring Dependencies: 0.00 ms
        Animations: 0.16 ms
        Frustum Culling (2x): 0.11 ms
        Script Components: 0.00 ms
        Physics: 0.00 ms
        Procedural Animations: 0.00 ms
        Animation Dependencies: 0.07 ms
        Input: 0.33 ms
    Update - Terrain: 0.69 ms
    Update - Particles: 0.00 ms
    Update - Emitters: 0.00 ms
    Update - Render: 0.03 ms
    GUI Update: 0.00 ms
    Compose: 0.73 ms

GPU Frame: 3.10 ms
    Update Buffers (GPU): 0.33 ms
    Skinning and Morph: 0.00 ms
    Wind: 0.01 ms
    Entity Culling: 0.04 ms
    Z-Prepass: 0.06 ms
    Visibility_Prepare: 0.10 ms
    Opaque Scene: 0.48 ms
    Transparent Scene: 0.30 ms
    Shadowmap Rendering (GPU): 0.44 ms
    Environment Probe Refresh: 0.60 ms
    MSAO: 0.10 ms
    Bloom: 0.05 ms
    Scene MIP Chain: 0.06 ms
    Occlusion Culling Render: 0.10 ms
    Occlusion Culling: 0.13 ms
    Terrain - UpdateVirtualTexturesGPU: 0.04 ms
    GUI Background Blur: 0.08 ms
    Caustics: 0.00 ms
    EmittedParticles - Render: 0.00 ms
    EmittedParticles - Render (Distortion): 0.00 ms
    Meshlet prepare: 0.00 ms
```

---

## Phase 4: Analysis

### CPU vs GPU Balance

| Metric | Time | Target for 120 FPS |
|--------|------|---------------------|
| **CPU Frame** | **8.30 ms** | 8.33 ms |
| **GPU Frame** | **3.10 ms** | 8.33 ms |
| **FPS** | ~120 | 120 |

**The system is heavily CPU-bound.** The GPU has 5.2 ms of idle headroom per frame. At 120 FPS, the CPU frame time (8.30 ms) is right at the limit of 8.33 ms (1000/120). Any CPU spike above 8.33 ms drops below 120 FPS.

### CPU Bottleneck Breakdown

| CPU Range | Time | % of Frame | Notes |
|-----------|------|------------|-------|
| Update - Wicked | 3.79 ms | 45.7% | Engine update (animations, culling, physics, buffers) |
| Shadowmap Rendering (CPU) | 2.25 ms | 27.1% | **Single largest CPU cost** — draw call submission for shadow cascades |
| Update - Logic | 1.57 ms | 18.9% | Game logic (LUA 0.54ms, Physics 0.76ms) |
| Compose | 0.73 ms | 8.8% | ImGui rendering + overlay compositing |
| Update - Terrain | 0.69 ms | 8.3% | Terrain chunk management |

**Shadowmap CPU cost (2.25 ms) is the #1 optimization target.** This is pure draw call submission — the CPU re-renders the entire scene into each shadow cascade. With 3-4 cascades, this means the CPU issues draw calls for the full scene 3-4 additional times beyond the main pass.

### GPU Cost is Low

The GPU is underutilized at 3.10 ms total. Top GPU costs:

| GPU Range | Time | Notes |
|-----------|------|-------|
| Environment Probe Refresh | 0.60 ms | Re-renders scene into cubemap faces for env probes |
| Opaque Scene | 0.48 ms | Main scene rendering (visibility buffer shading) |
| Shadowmap Rendering (GPU) | 0.44 ms | Shadow map rasterization on GPU side |
| Update Buffers (GPU) | 0.33 ms | Constant buffer / structured buffer uploads |
| Transparent Scene | 0.30 ms | Transparent object rendering |

None of these are individually large. The GPU has significant headroom.

### Why Shadows Have Such a Large FPS Impact

Disabling shadows saves +30 FPS (122 → 152), which translates to:
- CPU frame drops from ~8.3 ms to ~6.0 ms (saving the 2.25 ms shadow CPU cost)
- At 6.0 ms CPU frame time, the system can achieve ~166 FPS (limited by other CPU work)
- The measured 150-154 FPS is consistent with this

Shadows are expensive because:
1. **CPU draw call re-submission**: Each shadow cascade re-renders the entire scene, multiplying CPU draw call cost by 3-4x
2. **No shadow LOD**: All objects are rendered at full detail in shadow maps regardless of distance
3. **CPU-bottlenecked scene**: Since the GPU has headroom, the extra GPU shadow work is "free" — but the CPU submission cost is devastating

### Optimization Recommendations (Priority Order)

#### 1. Shadow Map CPU Cost Reduction (HIGH — saves ~2 ms CPU)
- **Reduce shadow cascade count** from 4 to 2 for test game / lower quality settings
- **Shadow distance culling**: Skip distant objects in shadow draw calls (CPU-side cull before submission)
- **Shadow caching**: Only re-render shadow cascades that have changed (far cascades can be cached for multiple frames)
- **Object shadow LOD**: Use simpler proxy geometry for shadow maps of distant objects

#### 2. Update-Wicked Optimization (MEDIUM — 3.79 ms is large)
- Profile sub-ranges within `Update-Wicked` to identify which system dominates
- `Input: 0.33 ms` seems high for input processing — investigate what it includes
- `Animations: 0.16 ms` and `Frustum Culling: 0.11 ms` are reasonable for the scene size

#### 3. Environment Probe Refresh (LOW — GPU cost, system is CPU-bound)
- 0.60 ms GPU for env probes is the single largest GPU cost
- Only relevant if CPU optimizations shift the bottleneck to GPU
- Can be reduced by lowering probe resolution or refreshing fewer faces per frame

#### 4. Quick Wins (Configuration Changes)
- **Volumetric clouds at 0% coverage**: Still costs ~3 FPS. Could skip the entire pipeline when coverage is 0
- **Reflections with no reflective surfaces**: Costs ~5 FPS for planar reflection setup even if nothing reflects. Could conditionally enable only when water/mirrors exist
- **MSAO at 0.10 ms GPU**: Already cheap, but could switch to SSAO for an additional small saving

---

## Phase 5: Island Showdown Editor Mode Profiler (2026-03-03)

**Test scene:** Island Showdown, editor mode (loaded via automation harness)
**Hardware:** AMD Radeon RX 9060 XT
**Method:** `ENABLE_PROFILER` → wait 5s → `GET_PERF_DATA` via automation harness
**Note:** Profiler itself adds significant overhead (FPS drops from ~112 to ~28 with profiler enabled). Relative proportions remain informative.

### Scene Metrics

| Metric | Value |
|--------|-------|
| FPS (profiler OFF) | 112 |
| FPS (profiler ON) | 28 |
| Scene objects | 2371 |
| Scene meshes | 1817 |
| Visible objects | 1940 |
| Lights | 4 |
| Materials | 1871 |
| Animations | 96 |
| Armatures | 280 |
| Env probes | 9 |

### Full Profiler Dump (Island Showdown, editor mode)

```
CPU Frame: 35.30 ms
    Update - Logic: 19.83 ms
    Fixed Update: 0.01 ms
    Render: 1.74 ms
        Shadowmap Rendering: 0.95 ms
        Shadowmap packing: 0.01 ms
    Update - Wicked: 12.60 ms
        Update Buffers (CPU): 0.06 ms
        Spline Update (2x): 0.00 ms
        Spring Dependencies (2x): 0.00 ms
        Animations (2x): 10.90 ms
        Frustum Culling (2x): 0.23 ms
        Script Components: 0.00 ms
        Physics: 0.01 ms
        Procedural Animations (2x): 0.01 ms
        Animation Dependencies (2x): 7.04 ms
        Input: 0.41 ms
    Update - Terrain: 1.04 ms
    Update - Particles: 0.00 ms
    Compose: 0.83 ms
    Update - Render: 0.04 ms
    GUI Update: 0.00 ms
    Max - Tree Update: 0.15 ms

GPU Frame: 7.50 ms
    Opaque Scene: 2.91 ms
    Z-Prepass: 1.12 ms
    Occlusion Culling Render: 0.67 ms
    Occlusion Culling: 0.68 ms
    Update Buffers (GPU): 0.55 ms
    Transparent Scene: 0.34 ms
    Shadowmap Rendering: 0.32 ms
    Skinning and Morph: 0.22 ms
    Ocean - Simulate: 0.21 ms
    Volumetric Lights: 0.19 ms
    Visibility_Prepare: 0.11 ms
    MSAO: 0.10 ms
    Scene MIP Chain: 0.06 ms
    Terrain - UpdateVirtualTexturesGPU: 0.06 ms
    Entity Culling: 0.04 ms
    Bloom: 0.04 ms
    Wind: 0.02 ms
    GUI Background Blur: 0.02 ms
    Underwater: 0.02 ms
    Meshlet prepare: 0.01 ms
```

### Analysis: Island Showdown vs Switch Escape

| Metric | Switch Escape | Island Showdown | Scale Factor |
|--------|---------------|-----------------|--------------|
| Scene objects | ~500 | 2371 | 4.7x |
| Visible objects | ~300 | 1940 | 6.5x |
| CPU Frame | 8.30 ms | 35.30 ms | 4.3x |
| GPU Frame | 3.10 ms | 7.50 ms | 2.4x |
| FPS (no profiler) | ~122 | ~112 | 0.9x |
| Animations | ~few | 96 | many more |
| Armatures | ~few | 280 | many more |

**Key findings for Island Showdown:**

1. **Animations are the #1 CPU cost (17.94 ms total):** `Animations (2x): 10.90 ms` + `Animation Dependencies (2x): 7.04 ms` = 17.94 ms combined. The `(2x)` suffix indicates these run twice per frame. With 96 animations and 280 armatures, this dominates the CPU budget.

2. **Update - Logic is 19.83 ms:** Likely driven by the larger object count and AI/entity processing for the island level.

3. **GPU scales moderately:** GPU went from 3.10 ms to 7.50 ms (2.4x) despite 6.5x more visible objects. The GPU pipeline scales well. `Opaque Scene: 2.91 ms` is now the largest GPU cost.

4. **Shadowmap CPU cost dropped relatively:** 0.95 ms in Island Showdown editor vs 2.25 ms in Switch Escape test game. This is because the editor may use different shadow settings or the game mode has additional shadow-related work.

5. **Occlusion Culling active:** 1.35 ms total GPU (Render + Culling). This wasn't in the Switch Escape numbers — may be editor-mode specific or newly enabled.

6. **Profiler overhead is massive:** FPS drops from 112 to 28 (75% reduction). The profiler's GPU timestamp queries and data collection add ~26 ms per frame. Profiler data should be collected briefly, not left on.

### Optimization Priority Update (Island Showdown)

1. **Animation system (HIGH — 17.94 ms, 51% of CPU):** The double-update `(2x)` pattern and 280 armatures make this the dominant cost. Investigate why animations update twice per frame and whether distant animated objects can be culled or updated at reduced frequency.

2. **Update-Logic (HIGH — 19.83 ms, 56% of CPU):** Entity processing at this scale. May overlap with animation cost (armatures processed during logic phase).

3. **GPU Opaque Scene (MEDIUM — 2.91 ms):** With 1940 visible objects, the main opaque pass is the GPU bottleneck. Mesh LOD and draw call batching could help.

4. **Occlusion Culling (LOW-MEDIUM — 1.35 ms GPU):** The occlusion system costs more than it likely saves in this open scene. Consider disabling for outdoor-heavy levels.

---

## Phase 6: DX11 vs DX12 Comparative Analysis (2026-03-03)

### Test Scene

**TESTPRO1 island level** — the default project in My Games tab. Tested at two load levels:
- **Simple**: Empty area, 768 objects, 135 visible, 2 animations, 11 armatures
- **Fully loaded**: Dense island, 2800 objects, 2195 visible, 101 animations, 346 armatures

Both engines tested at the same camera position on the same island level. DX11 screenshots saved as `PerfAt6msForUpdateDX11.jpg`, DX12 as `PerfAt6msForUpdate.jpg`.

### Simple Scene Comparison

| Metric | Old DX11 | New DX12 | Ratio |
|--------|----------|----------|-------|
| FPS | 443.9 | 127.7 | 3.5x slower |
| CPU Frame | 1.61 ms | 7.91 ms | 4.9x slower |
| Update - Wicked | 0.45 ms | 3.85 ms | 8.6x slower |
| Update - Logic | 0.74 ms | 0.94 ms | 1.3x |
| Render | 0.24 ms | 0.49 ms | 2.0x |
| Compose | 0.02 ms | 0.76 ms | 38x |
| GPU Frame | 0.91 ms | 3.10 ms | 3.4x |

Simple scene shows high fixed overhead in DX12 — 3.4 ms of "dark matter" in Update-Wicked from ECS traversal, GPU buffer prep, instance array zero-init, and 30+ subsystem updates every frame.

### Fully Loaded Scene Comparison

| Metric | Old DX11 | New DX12 | Ratio |
|--------|----------|----------|-------|
| **FPS** | **75.2** | **27.4** | **2.7x slower** |
| CPU Frame | 8.92 ms | ~36.5 ms | 4.1x slower |
| **Update - Wicked** | **2.57 ms** | **25.81 ms** | **10x slower** |
| Update - Logic | 2.80 ms | 10.41 ms | 3.7x slower |
| Render (shadows) | 3.22 ms | 2.51 ms | 0.8x (faster!) |
| Compose | 0.04 ms | 0.88 ms | 22x |
| GPU Frame | 4.07 ms | 10.06 ms | 2.5x slower |

### Scaling: DX12 Gets WORSE Under Load (Not Better)

The hypothesis that DX12's fixed overhead would be amortized under full load was **disproven**. The DX12 engine scales worse than DX11:

| Scaling (simple → loaded) | DX11 | DX12 | DX12 penalty |
|---------------------------|------|------|-------------|
| CPU total increase | +7.3 ms | +28.6 ms | 3.9x worse |
| Update-Wicked increase | +2.1 ms | +22.0 ms | 10.5x worse |
| GPU increase | +3.2 ms | +7.0 ms | 2.2x worse |

### Root Cause: Animation System (25.81 ms / 71% of CPU frame)

The DX12 profiler breakdown inside `Update - Wicked: 25.81 ms`:

| Sub-range | Time | Notes |
|-----------|------|-------|
| **Animations** | **17.40 ms** | `RunAnimationUpdateSystem()` — processes 101 animations, 346 armatures |
| **Animation Dependencies** | **12.16 ms** | `ScanAnimationDependencies()` — resolves bone hierarchy chains |
| Frustum Culling (2x) | 0.83 ms | Runs twice per frame |
| Input | 0.16 ms | |
| All other subsystems | < 0.1 ms each | Transform, mesh, material, weather, etc. |

Sub-ranges overlap due to job system parallelism; wall-clock total is 25.81 ms. Animations + Animation Dependencies account for virtually all of it.

**The old DX11 engine processed the same animated objects within 2.57 ms total Update-Wicked.** The new animation system is roughly **10x more expensive per armature**.

### DX12 Full Profiler Dump (TESTPRO1 Island, Fully Loaded, Test Game)

Scene: 2800 objects, 2248 meshes, 2302 materials, 4 lights, 101 animations, 346 armatures, 9 env probes. Camera at (6525, -12, 7306). 2195 visible objects.

```
CPU Frame: 37.84 ms
    Update - Logic: 10.41 ms
        Update - Logic - LUA: 1.02 ms
        Update - Logic - Physics: 1.96 ms
        Update - Logic - AI: 6.54 ms
        Update - Logic - Objects: 0.50 ms
    Fixed Update: 0.01 ms
    Render: 2.51 ms
        Shadowmap Rendering: 1.19 ms
        Shadowmap packing: 0.03 ms
    Update - Wicked: 25.81 ms
        Update Buffers (CPU): 0.08 ms
        Spline Update: 0.00 ms
        Spring Dependencies: 0.00 ms
        Animations: 17.40 ms
        Animation Dependencies: 12.16 ms
        Frustum Culling (2x): 0.83 ms
        Script Components: 0.00 ms
        Physics: 0.01 ms
        Procedural Animations: 0.01 ms
        Input: 0.16 ms
    Update - Terrain: 1.20 ms
    Update - Particles: 0.00 ms
    Update - Emitters: 0.00 ms
    Update - Render: 0.06 ms
    Compose: 0.88 ms
    Max - Tree Update: 0.09 ms

GPU Frame: 10.06 ms
    Opaque Scene: 1.46 ms
    Planar Reflections: 2.93 ms
    Planar Reflections Z-Prepass: 1.44 ms
    Occlusion Culling: 0.71 ms
    Occlusion Culling Render: 0.70 ms
    Update Buffers (GPU): 0.70 ms
    Z-Prepass: 0.58 ms
    Shadowmap Rendering: 0.34 ms
    Skinning and Morph: 0.28 ms
    Volumetric Lights: 0.25 ms
    Ocean - Simulate: 0.22 ms
    Visibility_Prepare: 0.13 ms
    MSAO: 0.12 ms
    Underwater: 0.10 ms
    Entity Culling (2x): 0.06 ms
    Scene MIP Chain: 0.06 ms
    Terrain - UpdateVirtualTexturesGPU: 0.06 ms
    Transparent Scene: 0.03 ms
    Wind: 0.02 ms
    GUI Background Blur: 0.02 ms
    Meshlet prepare: 0.01 ms
```

### DX11 Profiler Data (from PerfAt6msForUpdateDX11.jpg)

Same island, same camera position. FPS 75.2, Draw Calls 884, Triangles 3,126,414.

```
CPU Frame: 8.92 ms (10.98 ms avg)
    Update: 5.60 ms (6.51 ms)
    Update - Logic: 2.80 ms (3.68 ms)
        Update - Logic - Physics: 0.82 ms (1.25 ms)
        Update - Logic - LUA: 0.89 ms (1.07 ms)
        Update - Logic - Objects: 0.42 ms (0.51 ms)
        Update - Logic - AI: 0.27 ms (0.70 ms)
    Update - Particles: 0.02 ms (0.04 ms)
    Update - Terrain: 0.09 ms (0.16 ms)
    Max - Terrain Read Back (All): 0.04 ms (0.05 ms)
    Max - Tree Update: 0.02 ms (0.04 ms)
    Update - Emitters: 0.00 ms (0.01 ms)
    Update - Render: 0.02 ms (0.03 ms)
    Update - Wicked: 2.57 ms (2.44 ms)
    Frustum Culling: 0.03 ms (0.09 ms)
    Render: 3.22 ms (4.16 ms)
    Compose: 0.04 ms (0.43 ms)

GPU Frame: 4.07 ms (5.81 ms)
    Skinning: 0.32 ms (0.45 ms)
    Ocean - Simulate: 1.00 ms (1.27 ms)
    Z-Prepass - Scene: 0.75 ms (2.32 ms)
    Z-Prepass - Terrain: 0.04 ms (0.11 ms)
    Z-Prepass - Trees Low: 0.02 ms (0.09 ms)
    Occlusion Culling Render: 0.27 ms (0.53 ms)
    MSAO: 0.08 ms (1.41 ms)
    Depth Pyramid: 0.03 ms (0.04 ms)
```

### Secondary CPU Cost: Update-Logic AI (6.54 ms in DX12 vs 0.27 ms in DX11)

AI processing is 24x more expensive in the DX12 build. This could be GameGuru-side logic changes (not engine overhead), but worth investigating alongside animations. Together, Animations (25.81 ms) + AI (6.54 ms) account for ~32 ms of the 36.5 ms frame.

### GPU Analysis (DX12 fully loaded)

GPU at 10.06 ms is not the bottleneck (CPU is 36.5 ms), but notable costs:

| GPU Range | Time | Notes |
|-----------|------|-------|
| Planar Reflections | 2.93 ms | Re-renders scene from reflected camera — **29% of GPU** |
| Planar Reflections Z-Prepass | 1.44 ms | Z-prepass for the reflection pass |
| Opaque Scene | 1.46 ms | Main visibility buffer shading |
| Occlusion Culling (total) | 1.41 ms | Render + resolve |
| Update Buffers (GPU) | 0.70 ms | Instance/material/geometry upload |

Planar reflections alone cost 4.37 ms GPU (43%). If the scene has water, this is expected. If not, disabling reflections would nearly halve GPU cost.

### Key Files for Animation Investigation

| File | Content |
|------|---------|
| `WickedEngineDX12/WickedEngine/wiScene.cpp:253` | `RunAnimationUpdateSystem(ctx)` — the 17.40 ms call |
| `WickedEngineDX12/WickedEngine/wiScene.cpp:9154` | `ScanAnimationDependencies()` — the 12.16 ms call |
| `WickedEngineDX12/WickedEngine/wiScene.cpp:34` | `Scene::Update()` — full update pipeline with 30+ subsystems |
| `WickedEngineDX12/WickedEngine/wiScene.cpp:3831` | `RunArmatureUpdateSystem()` — bone matrix computation + GPU upload |
| `WickedEngineDX12/WickedEngine/wiScene_Components.h` | `AnimationComponent`, `ArmatureComponent` definitions |
| `WickedRepo/WickedEngine/wiScene.cpp:2493` | Old DX11 `RunAnimationUpdateSystem()` — the comparison baseline |
| `WickedRepo/WickedEngine/wiScene.cpp:1720` | Old DX11 `Scene::Update()` — simpler pipeline, fewer systems |
| `WickedRepo/WickedEngine/wiScene.cpp:3327` | Old DX11 `RunArmatureUpdateSystem()` — per-armature bone buffer |

---

## Phase 7: DX11 vs DX12 Animation System Deep Comparison (2026-03-03)

### Source Locations

- **DX11 (old)**: `D:/max/WickedRepo/WickedEngine/wiScene.cpp`
- **DX12 (new)**: `D:/max/WickedEngineDX12/WickedEngine/wiScene.cpp`

### Scene::Update() — Pipeline Differences

**DX11** has a minimal update pipeline:
```
RunPreviousFrameTransformUpdateSystem(ctx)
RunAnimationUpdateSystem(ctx)          ← flat for-loop, no jobs
RunTransformUpdateSystem(ctx)
Wait(ctx)
RunHierarchyUpdateSystem(ctx)
RunArmatureUpdateSystem(ctx)
RunMeshUpdateSystem(ctx)
RunMaterialUpdateSystem(ctx)
Wait(ctx)
RunObjectUpdateSystem(ctx)
Wait(ctx)
```

**DX12** has 30+ subsystems:
```
UpdateHumanoidFacings()
RunScriptUpdateSystem(ctx)
RunSplineUpdateSystem(ctx)
ScanAnimationDependencies()            ← NEW, async kick-off (12.16 ms)
RunCharacterUpdateSystem(ctx)          ← NEW
RunAnimationUpdateSystem(ctx)          ← job dispatch + Wait (17.40 ms)
RunPhysicsUpdateSystem(ctx)
RunTransformUpdateSystem(ctx)
Wait(ctx)
RunHierarchyUpdateSystem(ctx)
RunExpressionUpdateSystem(ctx)         ← NEW
RunMeshUpdateSystem(ctx)
RunVideoUpdateSystem(ctx)              ← NEW
RunMaterialUpdateSystem(ctx)
Wait(ctx)
WaitBuildTopDownHierarchy()            ← NEW
RunProceduralAnimationUpdateSystem(ctx) ← NEW (IK, springs)
RunArmatureUpdateSystem(ctx)
RunWeatherUpdateSystem(ctx)
Wait(ctx)
RunObjectUpdateSystem(ctx)
RunCameraUpdateSystem(ctx)
Wait(ctx)
```

### Root Cause #1: `ScanAnimationDependencies` — 12.16 ms (DX12 only, does not exist in DX11)

DX12 rebuilds an animation dependency graph **every single frame**. It groups animations into "queues" — animations targeting the same entity go in the same queue (must run serially), independent ones get separate queues (can theoretically run in parallel via job dispatch).

```cpp
// DX12: wiScene.cpp:9154 — runs EVERY FRAME
void Scene::ScanAnimationDependencies()
{
    animation_queue_count = 0;  // reset every frame
    wi::jobsystem::Execute(animation_dependency_scan_workload, [&](wi::jobsystem::JobArgs args) {
        for (size_t i = 0; i < animations.GetCount(); ++i) {
            AnimationComponent& animationA = animations[i];
            if (!animationA.IsPlaying() && animationA.last_update_time == animationA.timer)
                continue;
            bool dependency = false;
            for (size_t queue_index = 0; queue_index < animation_queue_count; ++queue_index) {
                AnimationQueue& queue = animation_queues[queue_index];
                for (auto& channelA : animationA.channels) {      // inner loop over ALL channels
                    if (dependency) {
                        queue.entities.insert(channelA.target);    // unordered_set insert
                    } else if (queue.entities.find(channelA.target) != queue.entities.end()) {
                        dependency = true;                         // unordered_set find
                        queue.animations.push_back(&animationA);
                    }
                }
                if (dependency) break;
            }
            if (!dependency) {
                // Create new queue, insert all channel targets into unordered_set
                animation_queues[animation_queue_count].entities.clear();
                for (auto& channelA : animationA.channels)
                    animation_queues[animation_queue_count].entities.insert(channelA.target);
                animation_queue_count++;
            }
        }
    });
}
```

**Complexity**: O(N_animations × N_queues × N_channels) with `unordered_set::find` + `insert` per channel per animation. With 101 animations and 346 armatures sharing bone entities, most animations collapse into a **single queue** — making the parallel job dispatch pointless while still paying the full scanning cost every frame.

**DX11 approach**: No dependency graph at all. Just a flat `for` loop over all animations, single-threaded. Fast.

### Root Cause #2: Keyframe Search Regression — O(N) vs O(1) Amortized

**DX11** caches the last keyframe position (`prevKeyRight`):
```cpp
// DX11: wiScene.cpp — O(1) amortized keyframe search
keyRight = animationdata->prevKeyRight;
if (animationdata->keyframe_times[keyRight] >= fAnimTimeEnd)
    keyRight = 0;                                  // wrap only if looped past end
while (animationdata->keyframe_times[keyRight++] < fAnimTimeEnd) {}
keyRight--;
animationdata->prevKeyRight = keyLeft;             // save for next frame
```
Since animation timers advance monotonically, this nearly always hits in 0–1 iterations.

**DX12** removed this optimization — full linear scan from index 0, every channel, every frame:
```cpp
// DX12: wiScene.cpp:1928+ — O(N_keyframes) per channel per frame
for (int k = 0; k < (int)animationdata->keyframe_times.size(); ++k) {
    const float time = animationdata->keyframe_times[k];
    if (time < timeFirst) timeFirst = time;
    if (time > timeLast)  timeLast = time;
    if (time <= animation.timer && time > timeLeft)  { timeLeft = time;  keyLeft = k; }
    if (time >= animation.timer && time < timeRight) { timeRight = time; keyRight = k; }
}
```
With many channels per armature and potentially hundreds of keyframes per animation, this is a massive per-frame cost multiplier.

### Root Cause #3: No Animation Culling (DX12 removed all culling from DX11)

**DX11 had three culling mechanisms:**

```cpp
// DX11: wiScene.cpp — 30fps halving
if (bEnable30FpsAnimations && !animation.updateonce) {
    if ((iAnimFrames + i) % 2 == 0) {
        iCulledAnimations++;
        bCulled = true;
    }
}

// DX11: wiScene.cpp — occlusion/visibility culling
if (bEnableAnimationCulling) {
    ObjectComponent* object = objects.GetComponent(animation.objectIndex);
    if (object && ((object->IsOccluded() && bEnableObjectCulling) || object->IsCulled())) {
        iCulledAnimations++;
        bCulled = true;
    }
}
```

| DX11 Mechanism | Effect | DX12 Equivalent |
|---|---|---|
| `bEnable30FpsAnimations` | Skip every other frame per animation (halves cost) | **None** |
| `bEnableAnimationCulling` | Skip animations on occluded/culled objects | **None** |
| `updateonce` guard | One-shot animations only process once after trigger | **None** |
| `animation.objectIndex` | Direct link from animation → object for culling checks | **None** (no object linkage) |

DX12 processes all 101 animations with all 346 armatures unconditionally every frame, regardless of visibility, distance, or occlusion.

### Root Cause #4: Larger Per-Channel Code Path

DX12's `RunAnimationUpdateSystem` handles **10+ animation target types** vs DX11's ~4:

| Target Type | DX11 | DX12 |
|---|---|---|
| Transform (translate/rotate/scale) | Yes | Yes |
| Mesh morph weights | Yes | Yes |
| Light (color, intensity, range, cone) | No | Yes |
| Sound (play, stop, volume) | No | Yes |
| Emitter (emit count) | No | Yes |
| Camera (FOV, focal length, aperture) | No | Yes |
| Script (play, stop) | No | Yes |
| Material (color, emissive, roughness, etc.) | No | Yes |
| Event dispatch | No | Yes |
| Retargeting (cross-skeleton) | No | Yes |
| Root motion (bone delta extraction) | No | Yes |
| PingPong mode | No | Yes |
| Cross-scene sampler | No | Yes |

Even when no animations use the new types, the larger function body means worse instruction cache behavior and more branch evaluation per channel.

### Armature System Differences (RunArmatureUpdateSystem)

The bone matrix math itself is essentially identical — `B * W * R` (inverseBindPose × boneWorld × armatureInverseWorld). The GPU upload changed from per-armature buffers (DX11) to a single shared skinning upload buffer with atomic allocation (DX12), which is actually better for GPU throughput. **The armature system is NOT the bottleneck** — it's the animation evaluation that feeds it.

### Cost Summary

| Cost Source | DX11 | DX12 | Delta |
|---|---|---|---|
| `ScanAnimationDependencies` | **0 ms** (doesn't exist) | **12.16 ms** | +12.16 ms |
| `RunAnimationUpdateSystem` | ~2.5 ms | **17.40 ms** | +14.9 ms |
| — Keyframe search | O(1) amortized (`prevKeyRight`) | O(N) linear scan from 0 | Major multiplier |
| — Animation culling | 30fps + occlusion + visibility | None | ~2x objects processed |
| — Code path size | 4 target types | 10+ target types | Icache/branch overhead |
| **Total Update-Wicked** | **2.57 ms** | **25.81 ms** | **10x slower** |

### Answers to Phase 6 Investigation Questions

1. **Why 17.40 ms for 101 animations?** Full linear keyframe scan O(N) per channel per frame (no caching), no animation culling (all armatures process regardless of visibility), and 10+ target type code paths per channel.

2. **Why 12.16 ms for `ScanAnimationDependencies`?** It rebuilds the entire dependency graph from scratch every frame using O(N²×channels) scan with `unordered_set` operations. With 346 armatures sharing bones, most animations collapse into one queue anyway.

3. **Does the `(2x)` pattern apply to animations?** No — `RunAnimationUpdateSystem` is called once per frame in `Scene::Update()`. The `(2x)` on Frustum Culling is from separate culling passes (main + shadow). Animation and Animation Dependencies each run once but overlap due to the async job kick-off.

4. **Is there animation distance culling?** No. DX12 has zero animation culling. DX11 had `bEnableAnimationCulling` (occlusion), `bEnable30FpsAnimations` (half-rate), and `updateonce` guards. All removed in DX12.

5. **What changed in bone matrix math?** The bone matrix math itself is identical (`B * W * R`). The slowdown is entirely in animation evaluation (keyframe search, dependency scanning, lack of culling), not in the armature/skinning pipeline.

6. **DX11 source location confirmed**: `D:/max/WickedRepo/WickedEngine/wiScene.cpp` contains the old animation system for direct comparison.

---

## Phase 8: Animation Optimization Plan

### Priority 1: Cache `ScanAnimationDependencies` (Engine-side, saves ~12 ms)

**Impact**: Eliminates 12.16 ms per frame — the single largest optimization opportunity.

**Current behavior**: Rebuilds the entire animation dependency graph from scratch every frame, even when no animations have been added, removed, started, or stopped.

**Proposed fix**: Cache the dependency graph and only invalidate when the animation set changes.

**File**: `WickedEngineDX12/WickedEngine/wiScene.cpp:9154`

**Implementation**:
1. Add a dirty flag `animation_dependency_dirty` to `Scene`, initialized to `true`
2. Set the flag to `true` when: animations are added/removed from the ComponentManager, `AnimationComponent::Play()` or `Stop()` is called, animation channels are modified
3. In `ScanAnimationDependencies()`, early-out if `!animation_dependency_dirty`
4. After successful scan, set `animation_dependency_dirty = false`
5. The `AnimationQueue` vectors and `animation_queue_count` persist across frames when not dirty

**Risk**: Low. If an edge case is missed, the worst case is a single frame with a stale queue (animations still process, just potentially in wrong queue order). Can add a periodic forced rescan (e.g., every 60 frames) as safety net.

### Priority 2: Restore Keyframe Search Caching (Engine-side, saves ~5-8 ms estimated)

**Impact**: Reduces per-channel keyframe search from O(N_keyframes) to O(1) amortized.

**Current behavior**: Full linear scan from index 0 across all keyframe times, every channel, every frame.

**Proposed fix**: Restore the DX11 `prevKeyRight` caching pattern.

**File**: `WickedEngineDX12/WickedEngine/wiScene.cpp` inside `RunAnimationUpdateSystem`, and `wiScene_Components.h` for `AnimationDataComponent`

**Implementation**:
1. Add `mutable int prevKeyRight = 0;` to `AnimationDataComponent` (same as DX11 had)
2. Replace the linear scan loop with the DX11 pattern:
   ```cpp
   keyRight = animationdata->prevKeyRight;
   if (animationdata->keyframe_times[keyRight] >= animation.timer)
       keyRight = 0;  // wrapped past end, restart
   while (keyRight < keyframe_count && animationdata->keyframe_times[keyRight] < animation.timer)
       keyRight++;
   keyLeft = (keyRight > 0) ? keyRight - 1 : 0;
   animationdata->prevKeyRight = keyRight;
   ```
3. Handle edge cases: animation loop/pingpong resets `prevKeyRight` to 0

**Risk**: Low. The DX11 version used this pattern for years. The only concern is thread safety if multiple animations share the same `AnimationDataComponent` (the `mutable` field would have a race). Mitigate by making `prevKeyRight` per-channel or per-animation instead of per-data if needed.

**Alternative**: Binary search (`std::lower_bound`) would be O(log N) with no stored state and zero thread-safety concerns. Simpler to implement, slightly less optimal than O(1) caching but far better than O(N) linear scan.

### Priority 3: Re-add Animation Culling (GameGuru-side, saves ~3-5 ms estimated)

**Impact**: Reduces the number of animations that actually process each frame by skipping invisible/distant objects.

**Current behavior**: All 101 animations process every frame regardless of visibility.

**Proposed fix**: Re-implement the DX11 culling patterns on the GameGuru side, since DX12 `AnimationComponent` no longer has `objectIndex` or culling fields.

**File**: GameGuru-side code that calls `animation.Play()` / manages animation state

**Implementation options**:

**Option A — GameGuru-side Pause/Play culling**:
1. Each frame, iterate GameGuru's entity list
2. For entities with animations that are occluded or beyond a distance threshold, call `animation.Pause()`
3. For entities that become visible again, call `animation.Play()`
4. This requires no WickedEngine changes — it uses the existing `IsPlaying()` guard that DX12 already checks

**Option B — 30fps halving (GameGuru-side)**:
1. Maintain a frame counter
2. Each frame, pause half the animations (odd/even split by entity index)
3. Next frame, swap which half is paused
4. Effectively halves animation CPU cost with minimal visual impact (characters animate at 30fps instead of 60fps)

**Option C — Engine-side culling restoration** (requires WickedEngine modification):
1. Add `objectIndex` back to `AnimationComponent` (or use entity lookup)
2. Add `bEnableAnimationCulling` flag
3. Check `ObjectComponent::IsOccluded()` / `IsCulled()` before processing each animation
4. Most faithful to the DX11 approach

**Recommendation**: Start with Option A (GameGuru-side, no engine changes). If insufficient, add Option B. Option C is the cleanest but requires engine modification.

### Priority 4: Reduce Per-Channel Overhead (Engine-side, saves ~1-2 ms estimated)

**Impact**: Minor but cumulative — reduces branch mispredictions and instruction cache pressure.

**Proposed fix**: Early-classify channels by target type and batch-process. Currently, every channel goes through a long if/else chain checking all 10+ target types.

**File**: `WickedEngineDX12/WickedEngine/wiScene.cpp` inside `RunAnimationUpdateSystem`

**Implementation**: Sort channels by `Path` type during animation setup (not per-frame). Process transform channels in a tight loop, then morph channels, etc. This improves branch prediction and instruction cache locality.

**Risk**: Low but requires careful testing of channel ordering assumptions.

### Expected Combined Impact

| Optimization | Estimated Savings | Cumulative |
|---|---|---|
| Cache dependency scan | ~12 ms | 25.81 → ~14 ms |
| Keyframe search caching | ~5-8 ms | ~14 → ~7 ms |
| Animation culling (50% culled) | ~3-4 ms | ~7 → ~4 ms |
| Per-channel batching | ~1 ms | ~4 → ~3 ms |
| **Total** | **~21-25 ms saved** | **25.81 → ~3 ms** |

Target: bring DX12 Update-Wicked close to the DX11 baseline of 2.57 ms for the same scene. Priorities 1 and 2 alone should recover ~17-20 ms (engine-side changes). Priority 3 can be done entirely on the GameGuru side with no engine modifications.

## Phase 9: Animation Culling Implementation (2026-03-04) — COMPLETE

Priority 3 (GameGuru-side animation culling) has been implemented using Option A + Option B combined.

### What was implemented

1. **Visibility-based culling (Option A)**: Animations for objects that are not renderable (culled/occluded by Wicked's frustum/occlusion system) are temporarily paused each frame. Uses `ObjectComponent::IsRenderable()` check.

2. **30fps half-rate throttle (Option B)**: Visible animations alternate frames using `(frameCounter + animIndex) % 2`, effectively halving animation CPU cost with minimal visual impact.

3. **Mechanism**: Pause/Play pattern in `GGAnimBridge_PreUpdate()` / `GGAnimBridge_PostUpdate()`. Temporarily `Pause()` culled animations and set `last_update_time = timer` so Wicked's skip condition (`!IsPlaying() && last_update_time == timer`) triggers. `Play()` restores them in PostUpdate before game logic checks `IsPlaying()`.

4. **Animation-to-object linkage**: `GGAnimBridge_SetAnimObjectLink()` maps each animation entity to its first `ObjectComponent` entity, enabling visibility queries. Registered centrally in `WickedCall_RefreshObjectAnimations()` (not `AddObject`) so all 8+ loading paths are covered. Cleanup via `GGAnimBridge_ClearAnimObjectLink()` before entity removal.

### Files modified

| File | Changes |
|------|---------|
| `GGAnimBridge.cpp` | Culling mechanism (Pause/Play + g_CullPausedAnims vector), ClearAnimObjectLink function, extern flag declarations |
| `GGAnimBridge.h` | ClearAnimObjectLink declaration |
| `wickedcalls_part0.cpp` | SetAnimObjectLink in RefreshObjectAnimations, ClearAnimObjectLink cleanup calls, bEnable30FpsAnimations default=true |

### Results (TESTPRO1 Island Showdown, 101 animations)

| Metric | Before (baseline) | After (culling active) | Improvement |
|--------|-------------------|----------------------|-------------|
| Update-Wicked | 25.81 ms | 11.36 ms | **-56%** |
| Animations | 17.40 ms | 4.47 ms | **-74%** |
| Animation Dependencies | 12.16 ms | 2.45 ms | **-80%** |
| FPS | ~27 | ~36 | **+33%** |

### Key lessons

- **Partial map population bug**: Initial implementation only wired `SetAnimObjectLink` in `WickedCall_AddObject`, but `WickedCall_RefreshObjectAnimations` is called from 8+ other code paths (AI, entity, gun, importer, grid editor). Moving registration inside `RefreshObjectAnimations` itself doubled the savings.
- **Dependency scan scales with animation count**: Even though `ScanAnimationDependencies` is an engine-side O(N²) scan, reducing the number of playing animations from 101 to ~50 (via culling) cuts dependency scan time by 80% because paused animations are skipped entirely.

### Remaining priorities

Priorities 1 (cache dependency scan) and 2 (keyframe search caching) require **WickedEngine-side changes** and would save an additional ~17-20 ms estimated. With those, Update-Wicked could reach ~3 ms (matching DX11's 2.57 ms).

---

## Phase 10: Performance Panel Display Bug Investigation

### Problem

The "Performance data" ImGui panel shows cascading/staircase duplicate entries. Instead of a clean list of CPU and GPU profiler entries, the panel displays:
- 1 CPU Frame header + entries
- Then 5-6 repeated GPU Frame headers, each with progressively fewer CPU-named entries

See `PerformanceDataPanel.jpg` in the repo root for the visual artifact.

### Investigation findings

**Clean data from harness**: `GET_PERF_DATA` (which reads `g_cachedProfilerText` set during `Compose()`) returns **correct, non-duplicated** profiler data. 1 CPU Frame + 1 GPU Frame header, ~28 CPU entries, ~24 GPU entries.

**Display call path**:
```
game_main_loop() → sliders_loop() → tab_tab_visuals(iPage, 1)
  → DisplayPerformanceData() → DrawProfilerDataColored_FirstMsOnly()
    → wi::profiler::GetTextData()  ← called during Update(), not Compose()
```

**Eliminated causes**:
- Multiple `DisplayPerformanceData` calls per frame — confirmed only 1 call via single `sliders_loop()` call
- Duplicate ImGui window IDs — unique ID `"Performance data##DisplayPerformanceData"`
- `wi::profiler::DrawData()` overlay — properly suppressed via `DisableDrawForThisFrame()` in `MasterRenderer::Compose()`
- Editor call path conflict — editor loop returns early during test game mode

**Primary suspect: Thread-unsafe `GetTextData()` iteration**

`GetTextData()` (`wiProfiler.cpp:686`) iterates the `ranges` flat_hash_map (`ska::flat_hash_map<size_t, Range>`) **without acquiring the profiler mutex**, while `BeginRangeCPU()` (line 203) **does acquire the lock** and can insert into/rehash the map from other threads. During `Update()`, profiler ranges are actively being started/ended by the engine.

The harness data is clean because `g_cachedProfilerText` is captured during `Compose()` (end of frame, all ranges complete). The display data is captured during `Update()` (mid-frame, ranges actively being modified).

**Secondary issue**: The `entryName == "Update"` skip in `DrawProfilerDataColored_FirstMsOnly()` doesn't appear to work in the screenshot, despite the `ExtractEntryName` logic looking correct.

### Diagnostic deployed (commit 5233fb3c)

Replaced `DrawProfilerDataColored_FirstMsOnly()` with raw `GetTextData()` display + a DIAG stats line showing:
- Call count (increments each invocation)
- Text length
- Line count
- CPU Frame header count
- GPU Frame header count

If the raw text shows `CPU=1 GPU=1`, the bug is in `DrawProfilerDataColored_FirstMsOnly()`. If it shows `GPU>1`, the bug is in `GetTextData()` (thread safety).

### Resolution (commit `c4d81543`)

Phase 10 resolved. See commit `c4d81543` ("FIxed Performance Data Panel") for the chosen fix. The diagnostic deployed in `5233fb3c` is no longer needed for this issue and can be removed if not useful for future investigations.

---

## Active Performance Targets

These are the remaining optimization opportunities surfaced by Phases 6–9. Each is a candidate for the upcoming perf-tuning pass.

### 1. ScanAnimationDependencies caching (~12 ms potential, engine-side)

Phase 6 / Phase 7 analysis identified `ScanAnimationDependencies` running every frame with O(N²) cost over the full animation set. Caching the dependency map (invalidate only when animation set changes) should reclaim ~12 ms.

Superseded — five Wicked-side changes are already carried locally (WICKED_ENGINE_CHANGES.md 1.1-1.5), so engine-side animation caching is implementable.

### 2. Keyframe search caching (~5–8 ms potential, engine-side)

DX12 regressed to linear scan over keyframes; DX11 had a cached search. Same engine-side caveat as above.

### 3. AI cost gap — DX11 0.27 ms vs DX12 6.54 ms (24× slower)

Separate investigation needed. Not addressed by Phase 6–9. Likely candidates: pathfinding rebuild frequency, AI script polling cadence, or an unintentionally expensive query inside the AI tick.

### 4. Verify Phase 9 culling holds with Phase 4 grass + Phase 5 trees enabled — NOW DUE

Grass (Stage B.10) and trees (Stage 4.3, 10K nearest-N pool) are now online, adding new per-frame work. Re-measure Update-Wicked / GPU-Frame on TESTPRO1 and re-verify the Phase 9 culling still holds under the new load — confirm the 36 FPS floor doesn't regress.

### 5. Tree pool CPU tax (added 2026-07-17 code audit)

- `GGTrees_WickedUpdate` scans the full 400K-instance array every frame regardless of live tree count
- `nth_element` runs over up to 400K candidates per frame for the nearest-N pick
- 10K ECS component lookups + transform rebuilds per frame for the pool slots
- Unstable slot→tree assignment breaks motion vectors (TAA/motion-blur ghosting) — fix by keying slots to tree index
- `ApplyDX11StyleAutoBlend` + `ProcessPaintedChunkBlendmaps` scan ALL `scene.objects` per frame with a per-object mesh lookup, inflated by the 10K pool (consider tagging chunk entities instead of the vertex-count heuristic)

### Reference: how to measure

- Inject `ENABLE_PROFILER` via automation harness (also sets `bProfilerEnable = true`, see CLAUDE.md profiler section)
- Read via `GET_PERF_DATA` — returns `g_cachedProfilerText` snapshot from end-of-frame (`master_part1.cpp` cache)
- Disable promptly afterward; profiler overhead is ~75% FPS drop
