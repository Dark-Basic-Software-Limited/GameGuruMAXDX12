---
name: project-shadow-flicker
description: "RESOLVED 2026-07-23 (engine `8c89731b` cascade-blend sync) — USER-CONFIRMED. TESTPRO1 cliff-view 'shadow flicker' (grass + its shadow appear/vanish, FPS 46<->17.5). MECHANISM CONFIRMED: it's the Wicked hair-particle GRASS SIM toggling — the shadow pass reuses the camera-culled visibleHairs, so when a chunk's grass HairParticleSystem loses its emitter mesh (meshID -> removed/recycled terrain chunk), the sim skips it and grass + shadow drop together (off-island where all grass idles = 45 FPS, the user's '46' state). FIX SESSION 2026-07-22: shipped GET_PERF_DATA instrumentation (GRASS_DEADMESH_NOW/RECYCLES/FULLRESETS/RECREATES) + harness SET_CAMERA/MOVE_CAMERA repro levers (commit e482a55a, pushed). Could NOT reproduce the drop across 4 vectors (static-wait, camera sweep/teleport, FULL off-island evict+return, sculpt-regen): DEADMESH_NOW stayed 0, chunk RECYCLES stayed frozen, grass rebuilds complete within a frame — no gap in a healthy build. CONCLUSION: the flicker is a SYMPTOM of the intermittent VT/terrain-generator re-stream (the same 'VT issue reappearing at random' the user shelved) — under generator stress a chunk's mesh lags and its grass goes dead-mesh briefly. Fix pending LIVE confirmation (DEADMESH_NOW>0 the instant it flicks) before touching the fragile grass lifecycle. See body for the confirmation recipe + ranked fix plan."
metadata:
  node_type: memory
  type: project
  originSessionId: 63b96d44-9243-46fb-bc37-9eb4d4be60b9
  modified: 2026-07-23T17:34:54.927Z
---

# Shadow flicker on the TESTPRO1 cliff view — investigation (fix pending)

## THIRD MECHANISM (2026-07-23, `f29e5e08`) — the REAL delayed-shadows terrain flicker: cascade-blend LOD desync

User pinned it precisely: with Delayed Shadows TICKED the cliff/alcove shadow flickers between "two terrain
shapes" at a SAVED camera; a <64u forward nudge stops it; unticking Delayed Shadows stops it. Ran a 4-agent
DX12-trace workflow (`wf_d02f368b`). Verdict (high confidence): **NOT a DX12 resource bug** — `shadowMapAtlas`
is a single persistent non-aliased texture, `LoadOp::LOAD`->`D3D12_..._PRESERVE`, and a skipped cascade freezes
its atlas rect AND `cachedVP` together (wiRenderer.cpp:4785-4790) so the sampled matrix always matches its
depth. The DX11 frozen-cascade technique itself is faithfully preserved in DX12.

**Real cause (terrain-specific, a DX11-vs-DX12 interaction the DX11 version never had):** (1) `GGTerrain_Draw_
ShadowMap` (GGTerrain_part0.cpp:10983-10992) draws each cascade at a different finest terrain LOD floor — was
`0,1,3,5,7,9` — so adjacent cascades cast silhouettes up to 2 LOD levels apart. (2) The lighting shader
DETERMINISTICALLY BLENDS adjacent cascades in the edge-fade band (CASCADE_DITHERING off), so a boundary pixel
samples BOTH. (3) Delta 1.11 refreshes those two cascades on DIFFERENT frames (/3 and /4); their frozen
snapshots desync, amplified by the terrain draw's own per-frame chunk filter (`!pChunk||IsGenerating()||(!IsVisible()&&lod!=lowestLevel)`,
part0.cpp:10999) during VT re-stream — so the boundary blend oscillates between the two LOD silhouettes = the
flicker, at a static camera. +20u shifts the split so the cliff is inside one cascade (no blend); delayed-off
re-renders both cascades from the same instant (in sync).

**FIX ATTEMPT 1 (`f29e5e08`, game-only, "unify cascade LOD"): DID NOT WORK (user tested).** Tightened the
per-cascade LOD floor gap to `0,1,2,3,4,5`. Its FAILURE is diagnostic: the flicker is NOT the LOD *magnitude*,
it's the DESYNC itself — two blended cascades holding snapshots from different frames whose terrain draw-set
differs (the per-frame `IsGenerating()/IsVisible()` chunk filter, part0.cpp:10999, changing during VT churn).
Kept it in (harmless quality bump) but it's not the fix.

**FIX ATTEMPT 2 (`8c89731b` ENGINE, WORKS in principle — user to confirm): SYNC the blended cascades.**
wiRenderer.cpp delayed-shadow decision (~4754): replaced the per-cascade `/2 /3 /4 /9` cadence + load-leveling
(which DELIBERATELY desynced cascades 1/2/3) with: cascade 0 every frame, cascades 1..N refreshed TOGETHER
every OTHER frame. Now the two cascades a boundary pixel lerps between always hold the same-age snapshot ->
blend can't oscillate -> no flicker, regardless of LOD or VT churn. Still ~halves the staggered cascades'
cost (they skip every other frame). Verified ~47 FPS with delayed shadows ON + shadows render correctly.
**Could not solo-repro the flicker** (needs the user's exact saved alcove camera on a cascade boundary + live
VT-stream + delayed ON), so user confirms. Remaining lever if a trace persists at the very-near 0/1 boundary
(1-frame desync there): also sync cascade 0, or the terrain-always-live option (drop the 7029 terrain skip).
Engine rebuild recipe applies (build_wicked.bat then game).

## SECOND MECHANISM (2026-07-23, `d770ed00`) — "two terrain shapes": shadow LOD override

User (correctly) observed the cliff-alcove shadow "is based on two terrain shapes as though the geometry
that renders the shadow changes." CONFIRMED root: Wicked's **`SHADOW_LOD_OVERRIDE` (default TRUE,
wiRenderer.cpp:158, GameGuru never disabled it)** makes the shadow pass render each object into the
cascades at a **per-cascade projected-size LOD** (`ComputeObjectLODForView`, wiScene.cpp:9002 — hard
`uint32_t` truncation, NO hysteresis) **independent of the visible mesh's LOD**. The terrain chunk mesh
has multiple LOD subsets (wiTerrain.cpp:27-171). A chunk straddling an LOD threshold in the cascade casts
LOD-K (fine) vs LOD-K+1 (coarse) on different frames = two terrain shapes. Wicked's own header comment
warns it "can result in shadow mismatch" (wiRenderer.h:1234). It COMPOUNDS with delayed cascades (a frozen
cascade refreshes at a different LOD than frozen) — **turning off EITHER stops the flicker; the user
confirmed unticking Delayed Shadows alone already halts it.**

**FIX (`d770ed00`):** tie `SetShadowLODOverrideEnabled` to `g_bDelayedShadows` at the SAME 3 points as the
cascade staggering (M-Visuals apply / checkbox / sun creation). At HIGHEST quality (g_bDelayedShadows=false)
the shadow now uses the stable main-view LOD -> shadow matches the visible terrain -> no shape flicker.
Delayed-ON projects unchanged (override stays on, no perf regression). Added `SHADOW_LOD_OVERRIDE` harness
toggle + `SHADOW_LOD_OVERRIDE_ENGINE` readout. **Could NOT reproduce the override's isolated effect solo**
(my parked SET_CAMERA sits in a coincident-LOD spot: override on/off gave identical cliff shadow; the FPS
A/B across all 4 configs was flat ~41-46; the user's closer/threshold view is where the two LODs diverge).
So this is the CORRECT fix for the mechanism the user identified + a quality improvement, but its isolated
contribution is user-confirmed, not solo-reproduced. The delayed-shadows fix (`a756c627`) is the confirmed
primary fix.

## ROOT CAUSE FOUND + FIX SHIPPED (2026-07-22, `a756c627`) — it was NOT grass, it was delayed shadows

**The grass investigation below was a WRONG TRACK** (an old FPS-vs-grass-sim correlation misled me; the
flickering view has `SCENE_HAIRS=0` — no grass, as the user insisted). The real cause:

**The engine's staggered directional shadow-cascade refresh (delta 1.11 — c0 every frame, then /2 /3 /4
/9; skipped cascades sample a FROZEN view-projection) was HARD-FORCED ON at sun creation**
(`master_part1.cpp` `SetDelayedShadowCascadesEnabled(true)`), **ignoring the project's "Delayed Shadows"
setting.** At HIGHEST quality the visuals set `g_bDelayedShadows=false` (M-Visuals_part1.cpp:654 — user
runs highest quality), but in DX12 the "Delayed Shadows" checkbox only drove the LEGACY point-shadow
booleans (`bEnableDelayPointShadow`), NOT the directional cascades. So turning it off did nothing → the
staggered far cascades lag their frozen VP under ANY camera movement and SNAP on refresh = the shadow
"there then not there" flicker the user couldn't switch off ("I turned off the flags but it still
flickers — like we never did anything").

**Why I couldn't reproduce it:** the flicker needs camera MOVEMENT (the far cascades only lag when the
view moves; a >64" move force-refreshes them). My harness `SET_CAMERA` parks the camera PERFECTLY static
(exact same value every frame) → no lag → no flicker. The user's hand-held free-flight camera always has
small movement → constant lag/snap. **CONFIRMED via A/B at matched camera positions under sub-64" jitter:
staggered (DELAYED_SHADOWS 1) renders the cliff shadows differently from every-frame (0) by mean 5/255,
p99 70/255, the difference tracing the cliff + tree-shadow contours** (scratchpad delayed_vs_everyframe.png).

**How I finally saw it:** stopped theorising, captured 100+ rapid screenshots (fixed the second-
granularity filename OVERWRITE bug first), and did FPS-split / variance / peak-to-peak / montage analysis
in Python (PIL) on the 3D-viewport crop. Profiler also showed `SCENE_HAIRS=0` + CPU-bound + `Shadowmap
Rendering` on the CPU. Reading delta 1.11's decision code (wiRenderer.cpp:4732-4778) surfaced the frozen-
VP + 64" refresh threshold, and M-Visuals_part1.cpp:654 (`g_bDelayedShadows=false` at HIGHEST) + the
unwired checkbox (M-GridEditB_part24.cpp:61) sealed it.

**FIX (`a756c627`, pushed, game-only build):** make the engine respect `g_bDelayedShadows` at 3 points —
(1) M-Visuals_part1.cpp visuals->engine apply block (`SetDelayedShadowCascadesEnabled(g_bDelayedShadows)`
beside SetOcclusionCullingEnabled, ~line 751); (2) M-GridEditB_part24.cpp checkbox handler (live toggle);
(3) master_part1.cpp sun creation uses `g_bDelayedShadows` not `true`. Plus a `DELAYED_SHADOWS_ENGINE`
readout in GET_PERF_DATA to verify. Projects that keep Delayed Shadows ON are unchanged (no perf regression).

**USER ACTION to confirm:** load the level, uncheck **Graphics & Performance → Delayed Shadows** (or it's
auto-off at HIGHEST quality once the project re-saves) → staggering off → every-frame cascades → stable
shadows → flicker gone. (I could not do the final before/after myself — static SET_CAMERA doesn't flicker.)

**Validated:** the engine control path works (DELAYED_SHADOWS 0/1 toggles it, readout confirms; the
checkbox now makes the same call) and delayed-OFF tracks shadows smoothly under jitter while ON lags.
**Open item:** confirm the load path applies a SAVED-off setting without a re-toggle (Fix 1 should; if
not, add a per-frame `GetDelayedShadowCascadesEnabled()!=g_bDelayedShadows` sync — was left out to avoid
fighting the DELAYED_SHADOWS harness A/B command).

---
## (SUPERSEDED WRONG TRACK BELOW — grass hypothesis, kept for history)


## UPDATE — FIX SESSION 2026-07-22 (instrumentation shipped, repro wall hit)

**Shipped (commit `e482a55a`, main, pushed):**
- **GET_PERF_DATA counters** (zero-risk, global in GGTerrainWicked.cpp): `GRASS_RECYCLES`
  (chunk-object entity-changed events), `GRASS_FULLRESETS` (grass torn down), `GRASS_RECREATES`
  (grass rebuilt), **`GRASS_DEADMESH_NOW`** (per-frame census of live GG-grass hairs whose emitter
  mesh is GONE this frame — **>0 == the flicker is happening**).
- **Harness camera levers** (editor only): `SET_CAMERA <x> <y> <z> [angx angy]` and
  `MOVE_CAMERA <dx> <dy> <dz>` — write `t.editorfreeflight.c.*` (same as the editor's "travel to").
  The cliff camera is `SET_CAMERA 3954 411.5 3877 3.2 -1252.7`.

**The active grass IS Wicked hair particles grown per terrain chunk** (GGTerrainWicked.cpp
`ProcessGrassChunks`, `hair.meshID = pc.entity` at ~line 1531; NOT the legacy `GGGrass.cpp`
`GGGrass_Draw*` renderer, which is dead in this level). 17 hair systems, ~910K strands.

**Mechanism re-confirmed with hard data:** off-island (VH=0, all grass idle) = **FPS 45** — this IS
the user's "46" state (grass sim off). On return VH=13 immediately but FPS ramps 42->15 over ~30s as
chunk tiers upgrade sparse->dense (sim cost = f(total strands), VH constant). So "46<->17.5" is
unambiguously the grass sim load; the shadow tracks it because the shadow pass draws hair from the
same camera-culled `visibleHairs`/`ib_culled`.

**REPRO WALL — could NOT make grass drop at a static camera.** Tried 4 vectors, `DEADMESH_NOW`
stayed **0** and `RECYCLES` stayed **frozen** in every one:
1. static wait (140 samples) — dead steady.
2. camera sweep +/-2400 in X/Z, x3 — VH wobbles 12-14 (frustum), no dead-mesh, no recycle.
3. **full off-island evict (20000,2000,20000) 12s then return** — strongest re-stream; grass cleanly
   removed (FULLRESET) then cleanly recreated (RECREATE), DEADMESH 0 throughout, RECYCLES never moved.
4. sculpt-regen at (3900,3000) — in-place regen keeps the chunk entity (delta 1.15), meshID stays
   valid, DEADMESH 0.
In a healthy build the grass rebuild completes **within a frame** — no gap. **`RECYCLES` (chunk-entity
recycle) only fired during initial LOAD (922), never from camera motion** — delta 1.21's expanded
working set keeps cliff chunks resident, so they don't stream out+back. viewdist sweep also showed the
distance cull is fully deterministic (no oscillation at any value).

**REFRAME (important):** the shadow flicker is a **symptom of the intermittent VT/terrain-generator
re-stream** — the very "old VT issue reappearing at random" the user told me to shelve. They are the
SAME root: when the generator is stressed and a batch of chunks re-streams, a chunk's mesh lags, its
grass goes dead-mesh for the gap, and grass+shadow drop together. That stressed state is intermittent
and I could not reproduce it solo. So there may be nothing SEPARATE to fix — VT/generator stability IS
the flicker fix.

## MECHANISM PINNED (2026-07-22, session 2b) — it's Scenario X, and DEADMESH is the WRONG signal

Read the engine end-to-end. The vanish is NOT a dead-mesh freeze:
- A hair whose `meshID` mesh is missing is EXCLUDED from the sim `items` (wiRenderer.cpp:5427-5442),
  so its `indirect_view` count is never re-zeroed and it keeps drawing **last-good** — it FREEZES,
  it does NOT vanish. So `GRASS_DEADMESH_NOW` (my census of dead-mesh hairs) will **stay 0** during
  the real flick — **it is the wrong counter to watch.**
- The real vanish is **Scenario X**: `Scene::Entity_Remove(entity, recursive=TRUE default)`
  (wiScene.h:343) and wiTerrain.cpp:835 removes a recycled chunk **recursively** → the GG grass hair,
  which is a CHILD of the chunk entity (`Component_Attach(grassEntity, pc.entity, true)`,
  GGTerrainWicked.cpp ~1592), is **removed WITH the chunk**. The hair ceases to exist →
  `VISIBLE_HAIRS` drops, its ~48ms sim is skipped (FPS jumps), and its shadow goes (shadow pass reuses
  visibleHairs) — grass + shadow vanish together until `ProcessGrassChunks` recreates it (which lags
  when the generator is behind = the stressed VT state).

**CORRECTED WATCH-SIGNAL:** on a live-flickering session, watch **`VISIBLE_HAIRS` (drops during the
flick)** and **`GRASS_RECYCLES` (climbs as chunks recycle at a static camera)**. `DEADMESH_NOW` stays 0.

**FIX (analyzed, NOT shipped — invasive + un-validatable without the live repro):** the clean cure is
to make GG grass entities **roots with an absolute transform** (bake `pc.world`; `world` stays
identical to the current `Component_Attach(...,true)` result, so placement is provably unchanged)
instead of chunk-children — then they SURVIVE Wicked's recursive chunk removal (engine freeze-last-good
keeps them visible), and `ProcessGrassChunks` **rebinds `meshID`** to the new chunk on `entityChanged`
(carry `recycled`/`tierChanged` in `PendingGrass`; rebind when `recycled && !tierChanged && tier!=0`).
BUT this opens a **grass LEAK**: a root grass whose chunk streams out permanently is never cleaned up
(Wicked's recursion did that for free) → floating grass + accumulating entities. So it ALSO needs a
compensating **orphan sweep** (remove tracked grass whose chunk key is absent from scene.objects for N
frames — with hysteresis so a momentary recycle gap doesn't re-trigger the vanish). That's 3 coupled
changes across the whole grass lifecycle, blast-radius = all grass, and A/B needs the VT-stress repro I
can't trigger solo. Per the "don't destroy ANY functionality" rule, NOT shipped blind. `MatrixTransform`
/`UpdateTransform` (wiScene_Components.h:87/102) are the APIs to bake the root transform.

**NEXT SESSION — confirm live, THEN fix (do NOT ship blind):**
1. On a session that IS flickering, poll `GET_PERF_DATA` (~2-4 Hz) and watch **`GRASS_DEADMESH_NOW`**.
   During a flick it should read **>0** (mechanism proven) with `RECYCLES`/`FULLRESETS` climbing at a
   static camera (trigger = re-stream churn). If DEADMESH stays 0 while FPS still swings, the drop is a
   `strandCount`/tier oscillation instead — check whether a chunk's tier flips (add a tier-oscillation
   counter).
2. Ranked fix plan (from the multi-agent diagnosis, all need the live repro to A/B):
   - **(root, best) VT/generator stability** — stop the boundary chunk re-streaming under stress
     (delta 1.20/1.21 family, wiTerrain removal margin/hysteresis). Fixes flicker AND the VT issue.
   - **(grass-side) rebind, don't recreate** — in `ProcessGrassChunks` fullReset, when `entityChanged`
     but geometry-equivalent, rebind the surviving hair's `meshID` to the new chunk entity instead of
     Entity_Remove+recreate (mirror wiTerrain gg_material_rebind). Fragile lifecycle — gate on repro.
   - **(engine) freeze-last-good for dead-mesh hairs** — when `meshID` mesh is missing, keep last
     frame's `ib_culled`/indirect count (don't zero it) so the grass draws stale-but-correct across the
     gap instead of vanishing. Needs engine rebuild + validation.
3. Ruled OUT this session: delta 1.11 delayed shadows (LoadOp::LOAD preserves skipped cascades;
   cascade 0 refreshes every frame so shadow just tracks grass 1:1); the Game Settings "Delayed
   Shadows" checkbox is UNWIRED in DX12; occlusion culling never touches hairs. TAA-jitter frustum flip
   ruled out by timescale (per-frame shimmer, not a slow flicker) + deterministic distance cull.

---
### Original investigation notes (session 1) below


User set the saved TESTPRO1 camera to a cliff-with-trees view (camera Eye ~3954,411,3877,
looking ~-Z). Reported a "steady slow flicker" — **shadows present in one moment, absent the
next** — with two shots at **FPS 46.1 vs 17.5** (same camera; the X:/Z: in the status bar are the
mouse-grid hover, not the camera). Asked: find why, make notes, then compact + fix. Also said "the
old VT issue is reappearing at random — shelve that."

## Measured facts (this session)
- **The frame is GPU-bound and the FPS swing IS the grass hair sim toggling.** Profiled: GPU
  ~63ms, CPU ~19ms; `HairParticles - Simulate` ~48-50ms = essentially the entire GPU frame
  (everything else ~7ms). 46 FPS = ~21ms = a frame with the grass sim ~0; 17.5 FPS = ~57ms = a
  frame with the grass sim full. So the two flicker states = **grass simulated vs grass absent.**
- **Grass hairs: 16 systems, ~905K total strands, 100K max/system, viewDistance 4563** (from the
  `HAIR_*` GET_PERF_DATA readouts added this session). See [[project-performance]] Stage P.2.

## Root MECHANISM (confirmed by code)
`visibleHairs` (the set that gets simulated + drawn) is built in `wiRenderer.cpp` ~3890-3901:
a hair is included ONLY if — unless `regenerate_frame` — `dist - aabb.radius <= hair.viewDistance`
(distance cull), AND `hair.meshID != INVALID_ENTITY`, AND `vis.frustum.CheckBoxFast(hair.aabb)`
(frustum). **Hair has NO occlusion query** (grep of wiHairParticle: none). An invalid/absent hair
is skipped entirely — `wiHairParticle.cpp` sim loop ~587 `if (strandCount==0 || !generalBuffer.IsValid()) continue`,
so it costs ~0. And the SHADOW pass draws hair from the SAME camera-culled `ib_culled` +
indirect args (`HairParticleSystem::Draw`, wiHairParticle.cpp:37/39, used for RENDERPASS_SHADOW at
wiRenderer.cpp:7017/7166). **So when a grass system drops out of visibleHairs: it's not simulated
(FPS jumps), not drawn, AND casts no shadow — grass + its shadow vanish together.** That is the
flicker.

## What it is NOT
- **NOT my delta 1.25 cull-skip** (`hairparticle_simulateCS.hlsl` `if(!visible && !regenerate_frame) return`).
  That only skips per-strand work for VALID, simulated hairs; an invalid/dropped hair isn't
  dispatched at all (pre-existing). The FPS-jump-when-grass-drops is pre-existing behaviour.
- **NOT a deterministic shader/per-frame bug.** In a FRESH static load at the same camera it does
  NOT reproduce: 33s time-series steady at FPS 14.8, `VISIBLE_HAIRS` steady 13, `TERRAINW_AUTO_RUNS`
  frozen at 399, `INVALIDATED_NOW` 0, grass sim steady ~49ms. A deterministic shader flicker would
  reproduce here. So it's STATE-dependent — the user's session was in a state mine isn't.

## Two candidate TRIGGERS for the grass drop-out (must distinguish next session)
1. **Terrain chunk emitter-mesh invalidation/regen** — the grass hair's emitter is a terrain chunk
   mesh; when a chunk regenerates (GGTerrainWicked regen / VT re-streaming), `hair.meshID` goes
   INVALID or the aabb changes while rebuilding, so the hair drops from visibleHairs until it
   re-creates. This would make it **the SAME root as the shelved "VT issue"** (terrain streaming
   instability). Tell-tale: `TERRAINW_AUTO_RUNS` / `INVALIDATED_NOW` INCREMENT during the flicker
   (a chunk stuck re-invalidating instead of converging). My clean load had these frozen.
2. **A grass chunk oscillating at the viewDistance/frustum BOUNDARY** — a chunk whose
   `dist - radius ≈ 4563` (viewDistance) or whose aabb straddles a frustum plane could flip
   in/out of the distance/frustum test. If `vis.frustum` is built from the TAA-JITTERED projection,
   the frustum planes wobble per-frame → an edge chunk flips → per-frame/slow flicker at a STATIC
   camera. Tell-tale: `VISIBLE_HAIRS` oscillates (e.g. 12<->13) while `AUTO_RUNS` stays FROZEN
   (no regen). The user's exact camera may sit a hair's aabb right on the boundary; mine sits just
   off it (steady 13).

## Reproduction gap + how to nail it next session
Does NOT reproduce in a clean static load. To catch it:
- Load the level, DON'T touch anything, and run a LONG (minutes) time-series of
  `FPS | VISIBLE_HAIRS | TERRAINW_AUTO_RUNS | TERRAINW_INVALIDATED_NOW` (hcmd GET_PERF_DATA loop),
  watching for FPS→~40+ moments. When one hits: is `VISIBLE_HAIRS` down AND is `AUTO_RUNS`
  incrementing (=> trigger 1) or frozen (=> trigger 2)?
- OR nudge the camera to force VT/terrain re-streaming (memory: fast zoom sweeps the high-res
  ring) and see if grass+shadow flick out during the re-stream (=> trigger 1). (No harness
  SET_CAMERA yet; PRESS_KEY drives the terrain-editor key handler, not camera fly — may need to
  add a camera-move/SET_CAMERA harness command, or reproduce by hand with the user watching.)
- Match the user's flags exactly (ALL optimisation flags OFF in Game Settings) in case one masks it.

## Fix directions (per trigger, for after we confirm)
- Trigger 1 (regen/streaming): keep the hair alive across an in-place chunk regen — don't drop
  visibleHairs while the emitter mesh rebuilds (mirror the terrain's "preserve on regen" idea, see
  [[project-terrain-editing]] delta 1.15 gg_preserve_blendmap_on_regen), and/or stop the spurious
  re-invalidation of the culprit chunk (find WHY a chunk keeps re-invalidating at a static camera —
  the scan/regen gates, [[project-performance]] caveats). Links to the shelved VT issue.
- Trigger 2 (boundary flip): give the hair distance/frustum cull hysteresis, or build the
  hair-culling frustum from the UN-jittered projection so TAA jitter can't flip edge chunks.

Related: [[project-performance]] (grass sim is the whole GPU frame; HAIR_* readouts), [[project-vt-zoom-squares]]
(the shelved VT streaming issue — likely the same family as trigger 1), [[project-terrain-editing]].
