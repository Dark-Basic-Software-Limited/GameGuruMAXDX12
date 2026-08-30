---
name: project-point-shadow-budget-design
description: "Implementation design to reproduce the DX11 GameGuru point-light shadow benefits in the DX12 Wicked shadow atlas (fix the ~24ms 'Shadowmap Rendering' CPU hit from 26 uncapped torch point lights). Phase 1 = revive the dead iShadowPointMax CAP (biggest safe win, ~24->15ms at cap16, correctness pivots on WITHHOLDING the CAST_SHADOW flag for rect-less lights); Phase 2 = static-cache via the existing ggLoadAtlas persistence (~1-3ms keeping 16 shadows); Phase 3 = distance-stagger. NOT YET IMPLEMENTED. Verified injection points inside."
metadata:
  node_type: memory
  type: project
  originSessionId: 63b96d44-9243-46fb-bc37-9eb4d4be60b9
  modified: 2026-07-24T01:26:02.815Z
---

# DX12 point-light shadow budget — implementation design

> **PHASE 1 SHIPPED + VERIFIED 2026-07-24 (engine `32317c63` / game `b4405d6e`, both pushed).**
> Implemented as designed with one simplification: only the CAST_SHADOW flag is withheld for rect-less
> lights (the `shadowmap` bool + shadowAtlasMulAdd are left alone — a denied light's (0,0,0,0) mulAdd is
> harmless because the withheld flag stops the shader ever reading it). The `w>0` gates are all guarded on
> `localShadowBudget < 0` so the engine default is byte-identical stock. **Phase 1.5 hysteresis SHIPPED too
> (engine `38ac3e89`):** each incumbent's score is biased ×1.15 (a torch granted last frame keeps its slot
> unless a challenger beats it by >15%), keyed on Entity via `localShadowGrantedPrev` (a `wi::vector<Entity>`,
> N≤16 linear scan, no new include) — selection-only, negligible cost, stops shadow-pop under camera motion.
> **Measured on the 26-torch ruins view (this level's quality
> tier caps at iShadowPointMax=4): SHADOW_LOCAL_GRANTED=4 CAPPED=21; Shadowmap CPU 24→3.77ms; CPU frame
> 34→19.7ms; GPU 19→13.9ms; FPS 28→57 (52.5 with profiler); screenshot pixel-identical, no black torches.**
> Files: `wiRenderer.cpp` (localShadowBudget global ~164, SetLocalShadowBudget/GetLocalShadowStats after
> SetShadowPropsCube, E1 cap before the pack-retry while, E2 skip in packer, flag gate replace_all x4,
> E5 gate in DrawShadowmaps), `wiRenderer.h` decls, game `M-GridEditB_part3.cpp` (SetLocalShadowBudget
> hoisted after the shadow-props gate), harness `AutomationHarness.cpp` (SHADOW_LOCAL_* readout).
> **PHASE 2 STATIC-CACHE SHIPPED + VERIFIED 2026-07-24 (engine `bb1de044` / game `6a1b0c6a`, pushed).**
> Implemented as an ALL-OR-NOTHING static cache (simpler/lower-risk than the design's per-light scheduler):
> in UpdateVisibility after packing, compare the granted local-shadow layout (entity+rect+pos+range+atlas,
> sorted by entity) to last frame; if unchanged → `localAtlasFullClear=false` → DrawShadowmaps LOADs the atlas
> and SKIPS clearing+re-rendering every cached local light; any change → one full-render frame repopulates.
> Shared-atlas/sun coupling handled: unified `wholeAtlasClear`, OR'd into the sun's `forceAll` before its VP
> freeze, cascade-clear skip guarded on `ggDelayedShadows`. Behind `SetLocalShadowCachingEnabled` (game turns
> it on when the cap is active); byte-identical when off. **Verified: cap 16, all 16 shadows granted,
> RENDERED settles to 0 (cached), correct visuals (no black torches, sun intact), survives camera motion
> (point-light cube maps are camera-independent — a bonus). Shadow CPU consistently lower cached than
> uncached, but the ABSOLUTE saving on this test level is modest+noisy (~1-5ms) because the SUN's staggered
> cascades dominate and oscillate 1-10ms here; the caching payoff is bigger on levels where LOCAL-light
> shadows are the bulk of the cost.** New harness levers: `SET_SHADOW_MAX <n>` (force engine budget, bypass
> the visuals-apply recompute), `SET_SHADOW_CACHE <0|1>`, `SHADOW_LOCAL_CACHE_ENGINE` readout. Key gotcha
> found: the game's shadow-props recompute (which pushes the budget) only runs on a VISUALS-APPLY, not per
> frame — so changing iShadowPointMax needs a visuals-apply to take effect (or the harness force). Also:
> `point_lights_count` (M-Lighting.cpp) buckets the cap; iShadowPointMax only CLAMPS it down.
> **DONE. Remaining perf on this level is grass-sim/ECS (Update-Wicked ~8ms + GPU grass ~4ms), a separate
> layer — see [[project-performance]].**
>
> **PHASE 3 (distance-stagger of LOCAL shadows) = PARKED by user decision 2026-07-24.** Assessed on the
> ruins level with averaged data (cap0 sun-only ~3.0ms; cap16 cache-OFF ~7.1ms; cap16 cache-ON parked
> ~3.3ms≈sun-only) → the local re-render cost is ~4ms and ONLY paid while local shadows are actively
> re-rendering, which on a STATIC torch bank is ~never (parked=0 cached; point cube maps are camera-
> independent so panning/orbit stays cached; only ZOOMING resizes rects → re-render). So stagger buys
> ~nothing here. **RE-SUGGEST Phase 3 only for a level with MANY DYNAMIC shadow-casting local lights**
> (moving lights, lights on vehicles/players, or lights near constantly-animating geometry) or sustained
> camera fly-through of a dense cluster — then the design (bounded round-robin refresh + distance cadence +
> content-dirty promotion) drops onto the existing renderThisFrame scheduler.
>
> **FAR-CASCADE CASTER CULL — DONE + VERIFIED 2026-07-24 (engine `a1851630` / game `c6c76eda`, pushed).**
> The SUN's staggered cascades cost ~5ms every OTHER frame (~1ms alternate): with DELAYED_SHADOWS ON,
> cascade 0 refreshes every frame (~1ms), cascades 1..N together every other frame (wiRenderer.cpp ~4835
> `if((frame%2)!=0) skip cascades 1..N`); the ~5ms spike was culling + draw-recording ALL objects into the
> huge far cascades. **The DX12 port had DROPPED DX11's far-cascade caster culling** (WickedRepo
> DrawShadowmaps 6930-6943: skip engine objects in the last 2 cascades + skip <~5m objects in the 3rd via
> areaXY<38000) — another DX11 shadow optimization lost in the engine upgrade, same class as the point-shadow
> cap. **Ported it into the DX12 per-object `camera_mask` build** (DrawShadowmaps directional case): an object
> doesn't set its mask bit for cascades >=3 (far → terrain-only), and cascade 2 drops sub-~5m objects
> (`sx*sy<38000 && sx*sz<38000 && sy*sz<38000`, GG world units). Gated off when character-dedicated shadows
> append cascades. `shadowFarCascadeCull` (ON by default, DX11 shipped it) + `SetShadowFarCascadeCull`/
> `GetShadowFarCascadeCull`; harness `SET_SHADOW_FARCULL 0|1` + `SHADOW_FARCULL_ENGINE` readout.
> **Measured (sun isolated, cap 0): the every-other-frame spike collapses ~4.3-5.7ms → ~1.8ms; sun-shadow
> CPU averages ~3.5→~1.3ms and the 1↔5ms oscillation is GONE (flat ~1-1.9ms). Visually identical A/B.**
> Original design below (unchanged).

---

# DX12 point-light shadow budget — original design (NOT YET BUILT when written)

Reproduces the three DX11 shadow optimizations the DX12 Wicked upgrade dropped (see
[[project-shader-build-pipeline]]-adjacent finding; root cause: DX11 `WickedRepo` capped
point shadows at `iShadowPointMax` via `SetShadowPropsCube(res,count)` + culled/staggered
them; the DX12 `WickedEngineDX12` atlas has no cap and renders every visible non-static
shadow light every frame → 26 torches × 6 cube faces = 156 passes = ~24ms CPU, 28 FPS).
Goal: 26 static torches cost ~1-3ms not 24ms, honoring the now-DEAD `iShadowPointMax`.

**Root-cause facts (proven this session):** both builds are Wicked; DX11 links
`D:/max/WickedRepo` (older, `SHADOWCOUNT_CUBE=2`, cap+cull+stagger), DX12 links
`D:/max/WickedEngineDX12` (dynamic atlas, no cap). Game-side the cap is COMPUTED then
DISCARDED at `M-GridEditB_part3.cpp:1369-1372` (both branches call `SetShadowPropsCube(res)`
without the count arg; spot is `// - REMOVED` at 1345). `iShadowPointMax` (Types.h:3887,
default 16) is a dead knob. Live baseline captured: CPU 34ms / GPU 19ms, "Shadowmap
Rendering" CPU 24-28ms; camera-move A/B: fly off cluster → VISIBLE_LIGHTS 26→1, shadow CPU
25→~1ms, FPS 28→53 (proves cost is COUNT-driven).

## The correctness lynchpin (independently verified against live source)
The HLSL shadow branch is gated ONLY on `light.IsCastingShadow() && surface.IsReceiveShadow()`
with **no rect-validity check** — `lightingHF.hlsli:68` (dir/spot) and `:228` (point,
`shadow_cube`). There is NO "no-shadow" sentinel. `shadowAtlasMulAdd` (ShaderInterop_Renderer.h:882)
zeroed → `mad(uv,0,0)=(0,0)` samples atlas texel (0,0) = a NEIGHBOUR's depth = wrong/black torch.
And today `wiRenderer.cpp:4974` sets the flag on `if (light.IsCastingShadow())` **unconditionally**
(not gated on a rect). **Therefore a capped light MUST have the CAST_SHADOW flag WITHHELD, not
merely its rect zeroed.** Verified: flag def `ENTITY_FLAG_LIGHT_CASTING_SHADOW = 1<<1`
(ShaderInterop_Renderer.h:863, read by IsCastingShadow at :970); packer loop at
`wiRenderer.cpp:4005-4015` already computes `dist`+`range` (so priority `range/dist` is free);
`shadowmap` bool at :4948; DrawShadowmaps gate at :6875. All injection points real.

## Policy
- **Scope:** POINT/SPOT/RECT non-static casters. DIRECTIONAL (sun) NEVER capped/cached/staggered —
  keeps its own `delayedShadowState` cascade path. `IsStatic()` = LIGHTMAPONLY (baked), NOT
  "stationary"; the torches are `IsStatic()==false`. **Do NOT mark torches IsStatic — that deletes
  their shadows.**
- **(a) Cap N** = the already-computed bucketed/clamped `shadowscube` (M-GridEditB_part3.cpp:1350-1358).
  `N<0` uncapped (engine default = stock no-op); `N==0` point shadows off.
- **(b) Priority:** `vis.visibleLights` is UNSORTED (`wi::vector<uint32_t>`) — must compute explicitly.
  `score = range / max(0.001, Distance(camEye, light.position))`, grant top-N desc, tie-break Entity
  id asc. (DX11-faithful alt = pure ascending distance; both OK.)
- **(c) Hysteresis (ships WITH the cap):** keep last frame's granted Entity SET; incumbent keeps slot
  unless challenger score > incumbent × 1.15 (mirrors gg_vt_upgrade_hysteresis). Key on
  `lights.GetEntity(lightIndex)`, NEVER lightIndex (reused per-frame slot). Kills boundary pop.
- **(d) Cull-unchanged (Phase 2):** granted light re-renders iff !valid|rect moved|atlas resized|light
  moved|content-dirty|whole-atlas-clear|stagger-due; else reuse cached texels. Do NOT port DX11
  history/delayed_shadow occlusion fields (need GPU→CPU readback DX12 lacks; Phase 4).
- **(e) Stagger (Phase 3):** distance cadence ×~40 (inch units): period <400→2,<1000→3,<2000→4,
  <3000→5,<4000→6,else 7; phase-offset by priority rank; content-dirty promotes to immediate.

## Phase 1 — CAP (independently shippable, touches NO persistence). Est. 24→~15ms @16; ≈ 1 + N·0.92 ms.
Single source of truth: **`rect.w > 0` = "won an atlas slot"** (visibleLightShadowRects clear+resize'd
to {0,0,0,0} every frame at 3682-3683; granted+packed point light ends w≥2; denied never enters packer).
5 edits, all in `D:/max/WickedEngineDX12/WickedEngine/wiRenderer.cpp` unless noted:
- **E1** (~before the pack-retry `while` at 4005): build granted set. Iterate visibleLights, skip
  inactive/non-casting/static/DIRECTIONAL; `score=range/dist`; if count>N, bias incumbents ×1.15,
  `nth_element` to partition top-N, mark the rest `granted[idx]=0`. Static thread_local scratch.
  `std::swap(grantedThisFrame, grantedLastFrame)`.
- **E2** (edit skip at 4010-4011): `... || (localShadowBudget>=0 && type!=DIRECTIONAL && granted[lightIndex]==0)` → continue.
- **E3** (point entity write, 4948-4949): move `shadow_rect` fetch above `shadowmap`; add `&& shadow_rect.w>0`
  to the `shadowmap` bool. **E4** (4974): change `if (light.IsCastingShadow())` → `if (shadowmap)` for
  SetFlags(CAST_SHADOW). Mirror the flag gate in SPOT (~4897) and RECT (~5047). Leave DIRECTIONAL (4811) alone.
- **E5** (DrawShadowmaps gate, 6875): `shadow = IsCastingShadow() && !IsStatic() && (type==DIRECTIONAL || shadow_rect.w>0)`.
  This is where the ~24→15ms is reaped (bypasses 6-face cam setup + aabb_objects cull + draw record).
- **Engine API** (wiRenderer.h near 1130, keep single-arg SetShadowPropsCube intact):
  `SetLocalShadowBudget(int)`, plus Phase2/3 `SetLocalShadowCachingEnabled(bool)`, `InvalidateLocalShadows()`,
  `GetLocalShadowStats(int&granted,int&rendered)`.
- **Game** (M-GridEditB_part3.cpp:1369-1372): keep `SetShadowPropsCube(res)`; ADD
  `SetLocalShadowBudget(iShadowPointMax==0 ? 0 : shadowscube)`; **hoist it OUTSIDE the `if` at 1360** so a
  LOWERED cap always applies; `InvalidateLocalShadows()` on change. Engine default budget -1 = stock byte-identical.

## Phase 2 — STATIC-CACHE (reuse via ggLoadAtlas). Est. ~15→~1-3ms keeping ≤16 shadows.
CAP-ONLY at 16 is still ~15ms (all 16 re-render every frame); the CACHE is what hits ~1-3ms while KEEPING
16 live shadows. Reuses the existing GG directional persistence infra (delayedShadowState/ggLoadAtlas/
PSO_shadowClear_GG, wiRenderer.cpp:6787-6867). Requirements/crux:
- **Layout stability:** quantize local rect size to coarse buckets (or fixed max_shadow_resolution_cube for
  Phase 2) so ordinary camera drift doesn't repack. **Correctness invariant:** reuse texels ONLY if rect
  unchanged; force-refresh any MOVED rect the same frame (instability degrades to "render more", never
  corruption). Point/spot SHCAMs derive from light.position+range (7186-7189) NOT camera → no VP-freeze
  needed (unlike directional).
- **Shared-atlas coupling fix (defeats both judge-flagged bugs):** one unified full-clear flag. Compute
  `localAtlasFullClear` in UpdateVisibility; **OR it into the directional `forceAll` at 4743** (BEFORE the
  cascade VP-freeze at 4785-4793) so the sun never re-renders vs a stale frozen VP; scheduler reads unified
  `wholeAtlasCleared`. Broaden ggLoadAtlas LOAD (6795) to `(ggDelayed || (caching && !wholeAtlasCleared))`;
  gate the partial-clear loop per-light on `renderThisFrame` (don't blank cached lights); add `ggDelayedShadows &&`
  guard at 6850 so the sun still clears when directional-delay is OFF but local caching forces LOAD.
- New per-Entity `LocalShadowState` (lastRect, atlasW/H, lastPos, lastRange, valid, renderThisFrame) in a
  `wi::unordered_map<Entity,...>`; scheduler `DecideLocalShadowRefresh(vis)` at end of UpdatePerFrameData.
  Behind `SetLocalShadowCachingEnabled`, default OFF, bisectable.

## Phase 3 — STAGGER + cull-unchanged. Bounds cost under motion. contentDirty = light range-sphere ∩ moved
dynamic casters; fallback = single global "any dynamic caster moved" bool (still bounded-safe).

## Phase 4 (deferred): SPOT/RECT budget via iShadowSpotMax; optional DX11 occlusion history (needs GPU→CPU query).

## Validation (harness)
Add `SHADOW_LOCAL_GRANTED/RENDERED/CAPPED` + `LOCAL_SHADOW_CACHE_ENGINE` to GET_PERF_DATA (mirror
DELAYED_SHADOWS_ENGINE at AutomationHarness.cpp:1044). NOTE: LIST_LIGHTS `shd=` reads the COMPONENT flag
(we don't clear it) — grant status must come from the new counters. Tests: baseline (~24ms/28fps);
iShadowPointMax sweep 16/8/4/2/0 → CPU ≈ 1+N·0.92ms; priority correctness (granted = N nearest);
visual A/B (nearest-N shadowed, rest LIT not black — proves E3/E4); camera-move popping (hysteresis kills
it); Phase2 caching (static camera → ~1-3ms, fly-off → ~1ms, no tearing); **sun-coupling regression**
(toggle Delayed Shadows, watch the cliff "two terrain shapes" flicker — highest risk); regression guard
(cap≥count → pixel-identical; default -1 → byte-identical).

## Judge outcome + build order
MVP cap-only scored feasibility 5 / risk 2 / correctness 4, no fatal flaws — "smartest architectural
decision is deferring the risky cache." Ship Phase 1 (+hysteresis) first. **Build order: engine
`cd D:/max/WickedEngineDX12 && ./build_wicked.bat Release` FIRST, then the game** (game build doesn't
rebuild the engine lib). Full design was produced by a workflow + verified against live source this session.
Related: [[project-shadow-system]], [[project-shadow-flicker]], [[project-performance]], [[project-wicked-engine-changes]].
