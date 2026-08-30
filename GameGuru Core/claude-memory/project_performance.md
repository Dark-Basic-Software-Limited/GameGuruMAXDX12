---
name: project-performance
description: "Performance thread. LATEST Stage P.6 2026-07-26 (the '120 FPS push'): TESTPRO1 temple-gate is GPU-WALL-BOUND ~13.6ms (the P.5 'submit tail' was the end STALL = GPU still busy — 1.48a instrumentation). Steady state 72 FPS; grass strand LOD 1.49b DEFAULT ON since 2026-07-27 (+ tier AUTO coupling); 1.50 wetmap opt-out fixed the dark-on-reveal grass (USER-CONFIRMED); single-queue AND lean-async both measured NEGATIVE (async overlap is real on the RX 9060 XT); terrain idle gate landed (CPU headroom only). 120 needs items across the risky border (hair-sim PIX hunt, tree density, opaque surgery, cross-frame GPU overlap) — full arithmetic in Stage P.6 below + PERFORMANCE.md. Earlier: P.5 53.7->~77 (VT job fix), P.4 54->72 tree pool, P.3 45->61, P.1 24->60+."
metadata: 
  node_type: memory
  type: project
  originSessionId: 63b96d44-9243-46fb-bc37-9eb4d4be60b9
  modified: 2026-08-09T17:21:42.286Z
---

# Performance (DX12 vs DX11)

## ★ 2026-08-06 MEASUREMENT RULE: levels WARM UP for 2-3 minutes now — a 30 s soak is not steady state
Lazy object PSOs (engine 1.82) went DEFAULT-ON after the 08-01 baseline, so pipeline states compile
on first use and editor FPS climbs 10-12% between 30 s and 180 s, flat by 300 s (Trapped 132→148,
Switch Escape 122.4→135.5→134.4, Zombie Cellar 119.9→132.7→132.7). This silently manufactured four
"regressions" in the 08-06 hub sweep, all on the FASTEST levels (a fixed ~1 ms costs 17% of a 6.5 ms
frame and 6% of a 12 ms one). **Any A/B must compare arms at the SAME warmth**, and the cheapest guard
is three arms — A → B → A. My first two-arm read said a change cost 11%; the third arm showed the first
arm was simply cold and the true delta was +0.8%. See [[project-demo-fps-baseline]].

## 2026-07-29: post-load FPS dip FIXED (`b865bad6`)
User-reported: ~5s after level load, FPS halves ~10s then recovers. Profiler-convicted on Canyon
Adventure: NOT the generator — after initialBuild ends (60% of ring), chunkSig churns every frame
until the ring completes, so the AutoBlend (4.66ms) + PaintedBlend (1.0ms) scans ran EVERY frame,
each pass's Generation_Cancel chopping the generator and prolonging the churn. Fix: scans paced to
every 4th frame outside build turbo (`g_blendScanInterval`, harness `SET_BLENDSCAN <1-60>`, 1=stock)
+ pass-local entity→ChunkData map (was O(pending×chunks)). Dip 120→71 → now 117→94 FPS; paint/sculpt
verified (+≤3 frames latency). Timelines in scratchpad demo_fps/dip_*.log; baseline chart in
[[project-demo-fps-baseline]].


Living backlog: `GameGuru Core/PERFORMANCE.md`. Baseline gap was DX11 ~75-400 FPS vs DX12 ~25-40.

## P.6 addendum — 2026-07-27 grass polish, USER-CONFIRMED (game `9aae00d8`, engine `8b1a587a`)

**User hands-on in game mode found and we fixed, confirmed by their walk ("worked great, no
appearing grid grass squares and no grass flicker"):**
1. **1.49 v1 two-shade shimmer** (hard LOD rings swept by camera motion) → 1.49b: per-strand
   jittered drop radii + EXACT hyperbolic coverage ramp (adversarial 3-lens workflow caught the
   linear ramp's ±16-26% static ring error + the 1.8-boost deficit BEFORE deploy; boost 2.0 =
   coverage-neutral).
2. **Square grass pop** = tier-3 chunk rebuild (5.5× density, one frame) at the hardcoded 1.0
   chunk ring → tier boundaries AUTO-coupled to SET_GRASSLOD (off=1.0/1.7 stock, on=1.5/2.2;
   explicit SET_GRASS tier3/tier2 overrides). Matrix: stock 72 / bundle ~70 / LOD+stock-tiers ~81
   / tiers-out-no-LOD 53 (never ship that).
3. **Travel-churn walking flicker** (30s dissipation) = regrowth queue in arbitrary order →
   near-first priority pass (budget fills gaps in front of the player first).
4. **Hysteresis deadlock fix**: downgrade hysteresis yields during tier-shrink sweeps, else an
   OFF toggle stranded 2.46M strands/48 FPS at a parked camera.
**LESSONS:** editor FPS swings ±8 between LAUNCHES (order-dependent streaming/atlas state) —
only within-session A/Bs count; and grass has TWO stacked LOD systems (per-chunk tiers + 1.49b
per-strand) — tune them TOGETHER, the tiers make the squares, the strand LOD makes it smooth.
**OPEN:** bundle is still opt-in (session-scoped SET_GRASSLOD 1); default-ON decision + soak
pending user. Escalation if flicker resurfaces: retarget recycled chunks' live grass to the new
mesh (never disappears) — unproposed.

**Evening 2026-07-27 (both USER-CONFIRMED):** (5) bundle flipped to **DEFAULT ON** (user: "make it
default"; engine `4792a97e`) — 2.46M strands out of the box, SET_GRASSLOD 0 = kill switch. (6) **the
dark-on-reveal fade + the SHADE half of every historic two-shade flicker = Wicked WETMAP, fixed as
delta 1.50** (engine `d5706805`): the GG sim early-out leaves culled/LOD-dropped strands' ping-pong
positions at raw zero = world origin = below the waterline → ocean wetmap ratcheted them to wet≈0.8
(drying disabled) → hairparticlePS lerps wet albedo toward BLACK; drying 0.02-0.08/s only while drawn
= the 15-30s brighten. GG grass now force-dried (DX11 parity, `SET_GRASSWET 1` = bug demo). Any
REMAINING high-speed far-grass flicker is pure geometric strand drop/reappear — first fine-tuning
target. Diagnosed by a 4-reader brightness-pipeline workflow; streaming/mips empirically exonerated
(grassbank DDS low mips are NOT darker; grass opts out of streaming with a unique cache path).

## Stage P.6 — 2026-07-26 evening (8h autonomous "120 FPS push"): GPU-WALL-BOUND truth + safe wins landed; 120 mapped, not reached

**User mission: 120 FPS on TESTPRO1 temple-gate (parked saved camera), safest routes only, stability
untouchable. Outcome: the frame model was WRONG and is now corrected; +5 FPS opt-in landed; 120 needs
border-crossing items (list below). Engine `a16548d0`, game `fe7cdb27`.**

**THE MODEL FIX (delta 1.48a — SubmitCommandLists phase timers, `SUBMIT_PHASES_MS` readout):** the P.5
"~3ms submit+Present tail" is ~95% the END STALL (next-buffer frame fence) — close 0.18 / present 0.11 /
stall ~3.2. The GPU is still busy when the CPU finishes: **frame = GPU-WALL-bound ~13.6ms** (GPU busy
~11.6 w/ profiler; CPU wall ~10.5 after the idle gate). Retroactively explains: BUFFERCOUNT 2→3 flat,
resolutionScale flat, hairskip's ±1.4ms FPS coupling. **CPU cuts = ZERO FPS here until GPU wall < CPU wall.**

**Steady state: 72 ± 1 FPS (6-min settle curve, flat).** GPU elimination shares: grass ~2.0ms, hair-sim
cadence ~1.7ms (BANNED — 1.37 flicker conviction), trees ~1.6ms (user density choice). GPU busy top:
Opaque 2.73, HairSim 2.69, Z-Prepass 2.21, Skinning 0.75, occlusion pair 0.75, SceneMIP 0.54.

**Landed (all knob-gated):**
- **1.49→1.49b grass strand LOD (+5 FPS, 72.1→77.2 clean ABAB; default OFF = user's visual call).**
  v1 (hard rings at 0.35/0.60×viewDist) FAILED IN THE FIELD next morning: camera motion swept a
  synchronized band of strands flipping normal↔wide = two-shade mid-field shimmer (user-reported),
  decaying with the editor camera glide. **1.49b (engine `8b1a587a`, 2026-07-27): per-strand
  hash-jittered drop radii (±15%) + EXACT hyperbolic coverage ramp 1/(1−0.5t) aligned to the drop
  window + boost endpoint 2.0 = coverage-neutral (<2 = documented thinning) + step4≤0 hardening —
  design verified by a 3-lens adversarial workflow whose coverage lens caught v1's +16%/−26% static
  ring pattern BEFORE deploy. Shader-only pickup at next MAX launch (refresh_shaders deleted 7 cso);
  until the next game relink the EXE default boost is 1.8 → exact config = `SET_GRASSLOD 1 0.35 0.60 2.0`.
  LESSON: "stable at a parked camera" is not enough for LOD selection — audit the MOVING camera
  (band edges must be per-item jittered, compensations must be continuous AND exactly shaped).**
- **Terrain idle gate (game, default ON):** Generation_Update 1-in-8 when fully quiescent (camera parked
  45+ frames, no pending/invalidated/merge chunks, sig stable, no edit pings; pings from InvalidateRegion/
  OnPaintDataChanged/OnTextureSetChanged). Update-Terrain 0.92→0.09ms. Sculpt breaks it same-frame
  (calm 6100→19, verified). `SET_TERRAINIDLE`, `TERRAINW_IDLE` readout. FPS +0 (GPU-bound) = headroom.
- **1.48b/c NEGATIVE pair (both default OFF, kept as probes):** single-queue (batches 15→1, deps 12→0)
  = −4.7 FPS; lean-async (only the 4 tiny helper lists moved to graphics, deps 12→~5) = −4 FPS.
  ~~**Wicked's async-queue structure fully earns its keep on this AMD card — submission overhead is a
  dead end. Do NOT re-chase.**~~
  ⚠⚠ **THIS VERDICT IS STALE — OVERTURNED 2026-08-09.** Re-measured on the current engine:
  single-queue is **+11.9%** on Switch Escape (201→225.3) and **+3.8%** on Island Showdown
  (91.6→95.1), lean-async **+5.5%** — no visual change. I expected a light-vs-heavy crossover and
  tested a heavy level for it; there isn't one, it wins on both. ★ **RULE: queue-structure verdicts
  have a shelf life — re-measure after renderer changes instead of citing the old number** (this one
  sat on a +12% lever for six weeks). Default still OFF pending a tail-gated sweep; the frame is
  GPU-FENCE-BOUND (99.4% of frames stall, mean 0.89 ms). See [[project-switchescape-perf]].

**PATH TO 120 (8.33ms) from 12.9ms (grass LOD on) — everything left crosses the border:**
GPU −4.5ms needed: hair sim 1.7-2.7 (blocked on the PIX flicker hunt), trees 1.6 (visual/impostor),
opaque+prepass remainder ~3.5 (structural drawcall/LOD surgery), cross-frame GPU overlap 1-2ms (the
end-of-frame all-queue sync in SubmitCommandLists deliberately forbids it — removing = cross-frame
resource-race hazard). AND CPU wall 10.5→≤8.3: S1 anim 2.2, S4 instance writes 1.5 (needs deterministic
meshlet offsets — the per-frame `meshletAllocator.fetch_add` makes instance data frame-unstable, any
skip-clean cache must first stabilize offsets), VisMain cull 1.3, RenderWait 1.35.

**Session lessons:** (1) settle drift + a leaked parallel script can silently poison an A/B round —
interleave ABAB and verify no orphan runners (round 3 here was two scripts racing one instance; its
84.8-FPS 'mystery' was the orphan's GRASSLOD window). (2) `SET_TREES draw 0` is ONE-WAY (HideAll mutates
source flags) — reload to restore; documented in WETEST. (3) Pre-existing bug found: sculpt+undo drops
the affected chunks' grass systems (SCENE_HAIRS −3) until any camera move >8 units — reproduced with
the gate hard-off; spun off as a task chip (suspect: ProcessGrassChunks settle-gate re-arm misses the
undo path; GGTerrainWicked.cpp ~1377/~2500).

## Stage P.5 — 2026-07-25 (8h autonomous session): TESTPRO1 temple-gate 53.7 → 74-76 FPS; the "fixed Render pole" SOLVED

**User mission: 120 FPS at ≥90% visuals on the updated TESTPRO1 (temple gate camera, 122 anims/670
armatures/2M grass strands/27 lights). Engine `32573850`+`97e61948`(+review-fix commit), game `7d93ba39`.**

**THE BIG ONE — delta 1.33 (engine): the months-old "fixed ~9.5ms Render CPU" was NEVER recording cost.**
1.32 instrumentation (Scene-S1..S5 stage ranges at the jobsystem barriers, RP3D per-recording-job ranges,
VT-job internals) exposed it in one measurement: `RP3D-rec PrepareAsync` blocks on
`wi::jobsystem::Wait(virtual_texture_ctx)` (wiTerrain.cpp UpdateVirtualTexturesGPU head) and the terrain
VT background job ran **16.4ms EVERY frame on one worker**: 14.2ms = uncached 2-byte memcpy loop
rewriting EVERY resident chunk-VT's FULL page table every frame; 2.1ms = free-list rebuild + LRU sort.
Fix: per-VT page-table dirty tracking (PhysicalTile::gg_owner; allocate_tile reports steal victims;
winner+victim marked dirty; clean VTs skip CPU write + GPU copy + residency dispatch; rotating heartbeat
insurance; lazy freesort). VT-job 16.39→0.41ms, Render 9.72→1.71ms, 52→62-70 FPS alone.
Master switch `wi::terrain::gg_vt_incremental`, harness `SET_VTINC 0|1`.

**Delta 1.36: subtree-parallel hierarchy** — stock RunHierarchyUpdateSystem = O(N×depth) full ancestor
chain rebuild per entry (18K bones × depth 8-15). Rewrite: one job per subtree ROOT, DFS carrying
world+layer-mask down the chain (topdown_hierarchy map). Scene-S2 4.97→2.22ms. **A first LEVEL-ORDER
design (barrier per depth level) was 4ms SLOWER + implicated in a crash — 30-60 bone-depth levels of
Dispatch+Wait; discarded.** `SET_HIERLO 0|1`. **Adversarial-review fixes (MUST KEEP): fast path gated on
gg_hier_snapshot_count == live count (falls back to stock for Scene::Instantiate temp scenes / Lua /
mid-frame ragdoll attach-detach — silent no-op otherwise!) and the DFS seed uses parent
GetLocalMatrix() NOT stored world (physics calls this system BEFORE TransformUpdateSystem).**

**Deltas 1.34/1.35:** dead AnimationQueue::entities inserts removed (AnimDeps 0.82→0.00);
frustum-visibility animation pause (gg_last_visible_frame stamp in UpdateVisibility; unseen >3 frames
+ >2000 units → pause EVALUATION, timers advance, resume-in-sync; `SET_ANIMVIS <frames> [neardist]`).
Accepted trades: off-screen characters' cast shadows freeze; resume is 1 frame late.

**HARNESS CRASH FIX (game): recurring silent test-loop deaths were `Cmd_GetPerfData` →
`ImGui::GetIO()` on a NULL context** when a command lands during startup/shutdown (days of
Guru-Crash.log entries at AutomationHarness.cpp:946-959). Null-context guards added at all 3 GetIO sites.

**Second half of the session (batches 3-7) — landed 1.37-1.41 + two decisive negative results:**
- **1.37 hair/grass sim cadence** (`8fadcf5b`): TESTPRO1 runs FULL default wind dir=(1,0,1) speed=1
  (WEATHER_WIND readout) so a pure static-skip never engages — instead, camera parked ≥4 frames →
  no wind = full skip; wind = sim every 4th frame (slow-motion sway, verified alive: grass-band
  motion 2.29 vs stock 4.12). **70.9 ↔ 78.3 FPS live A/B** (`SET_HAIRSKIP <0|1> [interval]`).
  **1.37b (`6d4ab66c`, USER-CONFIRMED FIX): 1.37 shipped a flicker regression — the vb_pos ping-pong
  swapped every WALL frame while the sim ran on a cadence, so IRREGULAR sim gaps (grass streaming at
  load, painting new grass — both force intermittent full-rate) flipped write/read parity per sim →
  position pops = flicker that 'cured' on camera move (movement = every-frame sim). Fix: swap gated
  on `gg_sim_ran_last_frame` — ping-pong advances ONLY on sim frames, every sim reads exactly its
  predecessor at any gap length. LESSON: a buffer swap that partners a decimated writer must be
  driven by the WRITER's cadence, not the frame clock.**
- **1.38** decompose→row-lengths (8722/frame); **1.39** underwater skip above waterline;
  **game loopanim half-rate** (Logic 1.35→0.71) (`b8085ed0`+`9b350096`). CPU-side wins masked at the plateau.
- **1.40 cmdlist merges** (`a9a30acf`): 17→14 lists — **MEASURED NEUTRAL** (submit bubbles were NOT
  the gap); kept default-on. **1.41 ShaderMaterial recompose cache** with texture-streaming epoch
  (`8fe42f47`): **NEUTRAL FPS** here but correct (visual identity 0.027) — material recompose was
  NOT the S2 pole. `SET_MERGELISTS` / `SET_MATCACHE`.
- **THE FRAME TAIL FOUND (1.32c `3bb8017d`): `App-SubmitPresent` = 1.7-5.2ms (avg ~3)** — queue
  submits + Present on the MAIN THREAD, the gap between profiler CPU-frame (~10.5) and real frame
  (~13). **BUFFERCOUNT 2→3 experiment: ZERO effect — REVERTED** (it is NOT a fence wait; it is the
  raw cost of ~15 submit batches + Present). Real fix = submit-batch reduction (VT copy/compute
  cross-queue splits) or off-thread SubmitCommandLists — STRUCTURAL, do supervised.
- **resolutionScale 0.85 ≈ FLAT** → the GPU side is grass VERTEX/raster work (2M strands × billboard
  quads in prepass+opaque), not pixels. GPU cuts need strand/draw-distance LOD (visual trade) or
  mesh-grass far representation.

**FINAL STATE: ~76-78 FPS steady (53.7 baseline = +45%), CPU-frame ~10.5 + ~3 submit tail, GPU ~9.8.
PATH TO 120 (all documented, none quick):** (1) off-thread submit / batch reduction (~2-3ms),
(2) grass vertex LOD (~1.5-2ms GPU), (3) Update residue: S2 mesh-geometry part + S4 instance-write
staging (~1.5-2ms), (4) frame pipelining Update(N+1)∥GPU(N). Workflow ranked plan + 59 findings in
the session log (scratchpad session_log.md); harness knobs let every delta be A/B'd in seconds.

## Stage P.4 — DONE (2026-07-24): TESTPRO1 grassy-island editor ~54 → ~72 FPS via the TREE POOL knob (game `1d9392d3`)

**THE ONE BIG LEVER = the tree pool.** The editor was **CPU-bound** (CPU 17.3ms / GPU 9.3ms — GPU ~40% idle).
`GG_TREE_POOL_SIZE` was a hardcoded **20000 ECS ObjectComponents** = ~90% of the scene's 22133 objects; each pays
per-frame Wicked ECS object/transform updates whether the tree is drawn or PARKED (parked slots only get
`SetRenderable(false)` — they still exist + cost `RunObjectUpdateSystem`). Made it a runtime knob **`g_treePoolSize`**
(default **6000**, array cap `GG_TREE_POOL_MAX=20000`) in `GGTrees_part2.cpp` — the pool now CREATES only that many
entities at setup. **FPS/pool: 20k=54.5, 8k=67.5, 6k=71.6, 4k≈73, 2k=76.** Applies at pool setup (startup / fresh
level load from hub — a mid-editor reload isn't available; live-resize would need a pool rebuild, deferred).
Knobs added (harness): **`SET_TREES pool <N>`**, **`SET_RESSCALE <0.4..1.0>`** (GPU res-scale; 0 FPS effect here
but kept for GPU-bound cases). Default 6000 = +31% with trees still present; tune down for FPS, up for density.

**MEASURED-ZERO levers (do NOT re-chase — all ≈0 FPS at this CPU-bound frame):** grass (any param), resolutionScale
1.0→0.5 (visibly low-res, same FPS ⇒ NOT GPU-bound), reflections on/off, object draw-distance/count, and the editor
key-toggles shadows(1)/AO(2)/bloom(3)/sun-vol(8)/occlusion(9). ⇒ Beyond the tree pool there is **no single silver
bullet**; the remaining ~13ms is DIFFUSE per-frame CPU. Frame model: `serial(~3.6ms) + max(Update 5.71, Render 9.48)`
concurrent (Update∥Render overlap — **cutting whichever is NOT the pole yields ~0 FPS**). Render 9.48ms is the FIXED
pole = `PreRender` serial (2nd reflection frustum-cull + `UpdatePerFrameData`, scale with TOTAL entity count) + a
blocking `jobsystem::Wait` on the critical command-list — NOT a drawable feature (shadows/reflections toggle = 0).

**VETTED PATH-TO-100 (workflow `wmbettnmb`; realistic SAFE ceiling ~80-85, 100 needs the RISKY render-pole cut):**
1. **[SAFE] Instrument the 9.48ms Render pole** — per-command-list CPU timers in `RenderPath3D::Render` (wiRenderPath3D.cpp:852-1780) + PreRender `UpdateVisibility(main@383/reflection@400)` + `UpdatePerFrameData@405` + wall-clock across `Wait@1777`. Print-only (pixels identical). MANDATORY prerequisite — every blind lever measured 0. **Do WITH user watching.** Shadows(key1)+reflections(key7) already RULED OUT as the pole → likely `UpdatePerFrameData` serial or the Wait/cmd-list overhead.
2. **[SAFE] Tree-pool default** on the FPS/density curve (knob shipped).
3. **[MODERATE, VT-fragile] Terrain idle-gate** — decimate `Generation_Update` (GGTerrainWicked.cpp:2196) to 1-in-8 when camera basis (Eye+At+Up) static + generator idle + no pending/merge/invalidated chunks + no brush; full-rate on ANY move; NEVER hard-freeze VT (memory warns: never freeze tile aging). ~+4-5 FPS at REST, 0 moving. Behind a runtime toggle; A/B the zoom-then-stop (tiles must still sharpen, no VT squares).
4. **[SAFE, ceiling-only] ECS micro-wins** (wiScene.cpp): XMMatrixDecompose→3 row-lengths @~4883 (bit-identical for +determinant); parallelize the 5.7MB single-thread instance-init memcpy @240-248; dirty-gate the unconditional material GPU write. **~0 FPS at pool≤6000** (Update hides under Render pole) — value is buying back tree density at the SAME FPS.
5. **[RISKY — supervised only] Cut the Render pole** (the ONLY route past ~85 to 100), guided by step 1: cache PreRender's 2nd reflection cull when camera+water static, or extend the delayed-cascade shadow stagger. CPU 13.1→~10.5-11.5 ⇒ ~87-95 IF a single ~3-4ms job halves; unknown until step 1 measures.
**Session detail (temp): scratchpad `perf_experiments.md`. Do-not list: don't re-test grass/resolution/reflections/object-count for FPS (all measured 0); don't gamble on VT/terrain or command-list surgery unsupervised.**

## Stage P.3 — DONE (2026-07-23): TESTPRO1 editor CPU 45 → 61 FPS, visuals identical

**LANDED. The editor per-frame CPU logic ("Logic - common_loop") went 5.90ms → ~0.5ms; CPU frame ~22 → ~16ms;
FPS 45 → 61 in the CPU-bound (no-grass) state. Screenshot A/B: pixel-identical.** Two fixes, both game-side
(no engine change):

1. **THE BIG WIN — gate `widget_getplanepos()` (M-GridEdit_part6.cpp ~line 946).** The editor's entity-placement
   path called `widget_getplanepos()` EVERY FRAME in drag-drop mode, even at a fully idle cursor. That function
   does `WickedCall_UpdateSceneForPick()` = **`wiScene::GetScene().Update(0)` — a full ECS scene update over all
   21K objects (~3.9ms)** — purely to make a freshly-moved placement plane pickable, then a plane pick. The result
   is only consumed while actually placing/moving an entity or dragging a gizmo handle. Gated it on real placement
   activity: `if (t.gridentity!=0 || t.gridentityobj!=0 || bDraggingActive || t.widget.activeObject!=0 ||
   t.inputsys.mclick!=0)`. Idle editor now pays 0 for it. Saved ~3.9ms. **NEEDS USER HANDS-ON: verify entity
   place/move/gizmo-drag still feel right (harness can't drive real mouse; logic says the gate is true whenever
   anything is being placed/clicked/dragged, so it should be transparent).**
2. **Pick reuse cache — `WickedCall_GetPick` wrapper (wickedcalls_part3.cpp).** `findentitycursorobj` (hover pick)
   ran `wiScene::Pick` over the whole scene every frame. Added a small **8-slot** cache keyed on raw ray inputs
   (screen pointer + camera Eye/At/Up + layer mask + which outputs requested) — NOT the derived pick ray (its
   projection carries per-frame TAA jitter, so it never repeats — see wiScene_Components.cpp:2718). When the ray
   is unchanged, replay cached outputs and skip the raycast; hover globals (g_hovered_*) live inside the skipped
   GetPick2 so they persist correctly. **Multi-slot is load-bearing: a single slot thrashed (measured 0 hits,
   ~100% mask-miss) because 2-3 different pick masks fire per frame — the 8 slots hold one per (mask,output).**
   At a parked camera PICK_REAL_RUNS freezes (~66) and CACHE_HITS climbs. Saved ~1.5ms. Harness readouts
   `PICK_REAL_RUNS/PICK_CACHE_HITS/PICK_MISS_RAY` + `EDIT_STATE` added.
   **FOLLOW-UP FIX (game `1fbb4ec6`): bypass the cache while ANY mouse button is held.** The cache feeds the
   terrain EDIT cursor (`WickedCall_GetPick`→`t.tx_f`→`t.inputsys.localx_f`→`t.terrain.X_f`), and grass/terrain
   PAINT (M-Terrain_part1.cpp:591) only applies when that cursor CHANGES each frame — a replayed pick froze it
   so paints silently never registered. Now `wiInput::Down(MOUSE_BUTTON_*)` → real pick every frame during any
   paint/sculpt/drag; parked idle (no button) still caches. (NB: this fixed a real cursor-freeze during editing
   but was NOT the "grass gone / not rendering" issue — that was the separate **stale-CSO** bug, see
   [[project-shader-build-pipeline]] and [[project-next-action-immediate]].)

Also added permanent profiler sub-ranges that pinpointed all this: `CL-*` (partition common_loop's ImGui in
M-GridEdit_part1) and `P2-*` (per editor-call in M-GridEdit_part2). BeginRangeCPU early-outs when the profiler is
off, so they're free in normal use.

**KEY REMAINING BOTTLENECK — the level is GPU-bound on GRASS when grass is present.** The alcove/cliff view has
grass that streams in/out (the documented churn: FPS oscillates 61↔~13). When grass is present, GPU = ~75ms with
**`HairParticles - Simulate: ~59ms` (1.17M strands)** — the [[project-performance]] Stage P.2 issue. My CPU win
only helps the no-grass (CPU-bound) frames. **To make 60 FPS STABLE at a grassy parked view, the grass hair sim
needs the Stage P.2 "skip-when-static" optimization (reuse grass GPU buffers when camera parked + grass unedited
+ wind≈0) — flagged there as needing user sign-off (touches the hair vb double-buffer).** That is the natural
next task. My gate did NOT cause grass-always-present (fresh loads show SCENE_HAIRS=0 @ 61 FPS); presence is the
pre-existing streaming variance.

### Stage P.3 — original investigation notes (kept for reference)

User asks to get the shadow-flicker test level back to 60 FPS. Profiled the level's **saved alcove camera**
(EYE `3167,489,3366`; the close cliff/alcove view — NOT the old far vista). Steady **~45 FPS / 22.2ms**
(the 15.5 FPS in the user's earlier screenshot was a transient; steady state is 45). **NO grass**
(SCENE_HAIRS 0 — grass sim is NOT a factor in this level, unlike Stage P.2). 21254 objects, **6810 visible**,
1 light (sun). **CPU-BOUND: CPU 22.0ms vs GPU 13.1ms.** To hit 60 FPS (16.7ms) must cut **~5.5ms off the CPU.**

**CPU breakdown (the bottleneck, from ENABLE_PROFILER):**
- `Render` (Wicked cmd recording) **9.3ms** — biggest single range; scales with the 6810 visible objects/drawcalls.
- `Update` **11.4ms** — of which `Update - Wicked` (ECS, 21K objects) **4.58ms**, terrain (`Wicked Bridge` 0.73
  + `Update-Terrain` 0.75 + `Generation_Update` 0.70) ~2.2ms, `Frustum Culling (2x)` 0.63ms.
- `Update - Logic (Total)` / `Logic - common_loop` **5.9ms** — editor ImGui/logic.
- `Shadowmap Rendering` (CPU) 0.8ms, `Compose` 1.0ms.
- (These top-level ranges overlap/nest; frame total is 22ms.)

**GPU breakdown (HEADROOM — 13.1ms, becomes the bound at ~77 FPS once CPU drops):** Opaque 6.48, Z-Prepass
2.58, Update Buffers 0.81, Clouds 0.45, Shadowmap 0.37, Planar Reflections 0.30, Underwater 0.29, VT 0.27,
Ocean 0.19, MSAO 0.10. Nothing pathological; cutting CPU is the whole game.

**CONSTRAINT (user rule, same as Stage P.1): the visual end product must NOT change.** So billboards /
density cuts are off-limits unless truly imperceptible; target redundant/inefficient CPU work.

**Prioritized things to TRY next session (ranked by leverage × ease):**
1. **`Logic - common_loop` 5.9ms — highest leverage, easiest.** Known suspect (Stage P.1 caveats): the
   **Tutorial video panel** (Object Tools → Tutorial → "LEVEL EDITOR" thumbnail) decoding every frame. TEST:
   collapse/close that panel and re-profile — if common_loop drops, gate the video decode/render to when the
   panel is expanded+visible (or decode at ~10Hz). Also: common_loop is only TOP-LEVEL instrumented — **add
   sub-range timers** inside it to see what the 5.9ms actually is (ImGui panel construction, Level Objects
   list rebuild, etc.). Likely 2-4ms recoverable, zero visual change.
2. **`Render` 9.3ms — biggest range.** 6810 visible objects = many drawcalls (CPU-recorded). Investigate the
   composition (tree pool vs terrain chunks vs scene rocks/foliage) and the drawcall/instance count. Try:
   (a) confirm trees/foliage are INSTANCED (batched) not per-object; (b) tighter per-object shadow/view cull
   for tiny distant props (fewer visible); (c) check the 20K tree-pool draw path. **Add a "visible drawcalls"
   / per-category visible readout** to guide this. 2-4ms if drawcalls can be cut.
3. **`Update - Wicked` (ECS) 4.58ms — scales with the 20K tree pool.** The ECS updates all 21K objects'
   transform/aabb each frame. Trees are static (pool slots don't move once bound). Try a **dirty-flag / skip
   updating static pool objects** (only update changed slots) — Stage P.1 already made slot binding stable, so
   most frames nothing changes. 1-3ms, no visual change.
4. **Terrain update ~2.2ms** (`Wicked Bridge` + `Update-Terrain` + `Generation_Update`) — the per-frame terrain
   maintenance. Likely already gated (Stage P.1). Low priority; re-check the gates aren't firing needlessly at
   a static camera.
5. **Shadowmap (my `8c89731b` sync)**: the flicker fix refreshes cascades 1-4 every 2 frames (vs old /3/4/9),
   a small CPU/GPU shadow increase (Shadowmap CPU 0.8 / GPU 0.37 — tiny here). If FPS is critical AND shadows
   allow, could relax to every 3 frames while KEEPING cascades 1-4 synced (refresh together) — do NOT re-split
   them or the flicker returns. Minor lever, keep for last.

**Recommended first move next session:** add sub-timers to `common_loop` and `Render` (they're the two biggest
and least-broken-down), + a per-category visible-object/drawcall readout — THEN attack the largest confirmed
sub-cost. Re-profile the alcove camera after each change; visuals must stay byte-identical (A/B the baseline).
Related: [[project-trees-phase5]] (tree pool), [[project-dx11-parity-baseline]] (visual A/B).

## Stage P.2 — 2026-07-21: TESTPRO1 heavy island vista is GPU-bound on the GRASS HAIR SIM

Profiled the user's saved full-island editor vista (~16.4 FPS, frame ~61ms). **GPU 55-59ms vs CPU
18ms — GPU-bound.** GPU breakdown: **`HairParticles - Simulate: ~48-52ms`** (the entire wall);
everything else tiny (Opaque 1.6, Z-prepass 0.5, shadows 0.5, underwater 0.05...). So the frame is
~7ms of real rendering + a giant grass-sim tax.

**Root cause (measured via new `GET_PERF_DATA` HAIR_* readouts):** the hair/grass simulate compute
pass (`hairparticle_simulateCS.hlsl`, dispatched once/frame for the MAIN view's visible hairs at
wiRenderer.cpp ~5421) runs **one thread per STRAND per system every frame**, and there are **~905,000
strands** (16 systems, 100K max/system, viewDistance 4563). 905K threads @ 48ms = pathologically slow
(~65ns/strand) = latency-bound (random emitter-mesh + paint-mask texture reads, low occupancy on a
huge register-heavy shader), NOT raw compute. Cost is bound by total strand count, which is the user's
**deliberate grass density** — reducing it is a visual change (off-limits per the ask). CRITICAL:
every strand runs the full billboard-write + physics EVEN when distance/frustum-culled (the per-strand
cull only gates DRAWING, not simulation).

**Delta 1.25 (engine `3273b651`) — SAFE, applied:** hair sim early-outs the billboard-write + physics
for non-drawn (culled) strands. Visuals byte-identical (drawn strands untouched). Only ~7% at this
aerial benchmark (most grass on-screen) → **more when zoomed into a small edit area** (most grass
off-frustum). 52.0 → 48.3ms.

**Bigger levers, BOTH need user sign-off (flagged, NOT done — the "don't destroy functionality" rule):**
1. **Skip-when-static** — reuse the grass GPU buffers when the camera is parked + grass unedited +
   wind≈0 (GG sets no wind, so grass is likely static). Could take a PARKED editor view toward 100 FPS
   (sim → ~0), zero visual change. RISK: touches the hair `vb_pos[0]/[1]` motion-vector double-buffer
   swap (wiHairParticle.cpp:432 UpdateCPU) coordinated with the GPU sim skip + a dirty/heartbeat gate;
   only helps parked cameras, not active editing.
2. **Distance-LOD the grass strand count** — simulate/draw fewer blades for distant chunks; big perf,
   near-imperceptible if tuned to the existing fade, but it DOES touch grass density.

## Stage P.1 — DONE 2026-07-18 (`fc1575e2`, engine `b96e617a`): 24.2 → 60+ FPS on the TESTPRO1 A/B view

Profiler first (never guess): "Update - Terrain" was 25.95ms of a 41ms frame (historic 0.7-1.2ms). Attribution by sub-range instrumentation (ranges kept in the build), then six fixes — **visuals pixel-identical throughout**:

1. **Tree pool stable slot binding** — slots keep their tree until evicted by the nearest-N set; only changed slots get component writes. Was 20K rebinds+UpdateTransform per frame = ~20ms ("TerrainW - Tree Pool" 19.7 → 0.0 steady-state). Also fixes TAA/motion-vector ghosting (review item). Selection (400K scan + nth_element) throttled: camera move >8", `g_treeInstanceStamp` (bumped in `GGTrees_UpdateInstances`), pool reset, or 256-frame heartbeat.
2. **Pool correctness fixes folded in**: respects `draw_enabled` (parks, force-rebinds on re-show), `GGTrees_SetData` arms iUpdateTrees=5 on failure, setup latch guarded against zero types.
3. **Blendmap + grass chunk scans gated** — AutoBlend/PaintedBlend/ProcessGrassChunks (each a 21K-object scene scan) run only when a chunk-set signature (FNV over entity + blendmap_layers.size per chunk), their processed-map size, a dirty signal, or (grass) camera move changes. ~5ms → ~0.
4. **Dead DX11 draw-path work skipped under Wicked**: `GGTrees_Update` tail (shadow gather + per-type `CreateBuffer` EVERY FRAME + tree CB) and all of `GGTrees_UpdateFrustumCulling`. ~1.7ms + allocation churn.
5. **Tree shadows band-limited**: only LOD0-band (<2500") trees cast — exact DX11 `lod_dist_shadow` default parity. Shadowmap CPU 3.2 → 0.6ms.
6. **Occlusion-query opt-out for pool objects** (Wicked delta #6, `OCCLUSION_QUERY_DISABLED`): occlusion GPU 3.9 → 0.16ms. Global occlusion still follows `t.visuals.bOcclusionCulling` for real entities (applied from M-Visuals_part1/M-GridEditB; a global kill would be wrong). **Key 9** = global occlusion A/B toggle.

## Current frame (TESTPRO1 editor, 60 FPS)

CPU ~15.8ms with profiler: Logic - common_loop 5-7 (editor ImGui/logic), Render (Wicked cmd recording) ~5, Update - Wicked (ECS, 21K objects) ~4.4, Update - Terrain ~0.9. GPU 13.4: Opaque 6.0, Z-prepass 2.4, everything else small.

## Next levers (if >60 needed, or for game mode)

1. **Camera-move spike**: selection rescan costs ~8-10ms on frames where the camera moves — flythrough dips into the 40s-50s. Fix = spatial-grid gather (bin valid trees once per data change, ring-gather around camera) instead of the 400K linear scan + nth_element.
2. **Logic - common_loop 5-7ms** — editor ImGui construction/logic; instrumented only at top level so far. (Tutorial video panel decode is a suspect worth one test.)
3. **Update - Wicked 4.4ms** — engine ECS tax scales with the 20K pool; shrink pool only with a far-tree substitute (billboards) or engine work.
4. **Animation caching (~17-20ms) + AI 24x** — NOT hit on TESTPRO1 (SCENE_ANIMATIONS=0); they re-emerge on character-heavy game levels. Old plan (ScanAnimationDependencies O(N²), keyframe caching) still valid there.

## Caveats / to sanity-test in normal editing

- The scan gates rely on: chunk signature, processed-map sizes, `GGTerrainWicked_OnPaintDataChanged`, grass dirty signals, `g_treeInstanceStamp`. If the user reports stale terrain paint or grass after an edit, suspect a mutation path that bypasses the signals (fallback: trees have a 256-frame heartbeat; blendmap/grass gates have none).
- **Tree paint latency BUG FOUND AND FIXED same day (`c6134185`)**: paint/spray/erase/flatten mutate `pAllTrees` flags in place (fixed 400K array, no count change, no UpdateInstances call) — none of the original signals fired, so painted trees waited for the ~4.3s heartbeat. Fix: `InstanceTree` setters (SetVisible/SetFlattened/SetInvalid/SetType/SetScale/SetFlags) bump `g_treeInstanceStamp` on any real state change; hover highlight excluded; all callers verified event-driven. Lesson: in-place bitfield mutation is the editing model here — new gates must hook the SETTERS, not the bulk-rebuild functions. Direct field writes (x/y/z) still bypass stamps → heartbeat is the backstop (undo via memcpy would be such a path).
- Occlusion culling for interiors: real entities still query; only tree pool is exempt.

## Done earlier

- Animation culling (2026-03-04): Pause/Play trick in GGAnimBridge, FPS 27→36 (details in git history of PERFORMANCE.md).
- "Performance data" panel duplicates: RESOLVED `c4d81543` (2026-03-28). Do NOT re-investigate.

## Profiler usage

- `ENABLE_PROFILER` sets BOTH `bProfilerEnable` AND `wi::profiler::SetEnabled` (editor disables it every frame otherwise). Collect via `GET_PERF_DATA`, disable after. Overhead is modest (~1-2ms), earlier "75%" note was overstated at this frame rate.
- Sub-ranges now built in: Terrain - GG Core / Wicked Bridge, TerrainW - Generation_Update / AutoBlend Scan / PaintedBlend Scan / Grass Maint / Tree Pool, Trees - FrustumCull, Grass - GG Update, Logic - common_loop / ConstantNonDisplay.

Related: [[project-trees-phase5]] (pool design + stable slots), [[project-wicked-engine-changes]] (delta #6), [[project-dx11-parity-baseline]].


## Editor: selecting an entity costs a FULL Scene::Update (2026-08-05, game `31710edc`)

Symptom: clicking any entity in the 3D editor drops island.fpm from ~90 to ~45 FPS.
Profiler: CPU Frame 21.97 ms, `Logic - common_loop` **14.54 ms**, GPU Frame only 7.60 ms.
`Update - Emitters` and `Update - Particles` both **0.00 ms** (so not the WPE particle work -
that was the trigger for the investigation and was fully exonerated).

**Root cause = DX11->DX12 port regression.** `WickedCall_UpdateSceneForPick`
(`wickedcalls_part2.cpp`) still carries the DX11 comment *"//PE: Only transform update
needed."* but calls the **full** `wiScene::Scene::Update(0)`. The DX11 fork had
`Scene::UpdateSceneTransform(0)` (transform systems only, `WickedRepo/wiScene.cpp:1593`);
modern Wicked has no equivalent so the migration swapped in the whole-scene update.
Called every frame from `widget_getplanepos` (`M-Widget.cpp:390`) while an entity is
selected, and it runs BEFORE the pick, so the Perf-P.3 pick cache never helped it.

**MEASURED: 4.085 ms per call** on island.fpm (8982 objects) via the new `FORCE_PICKUPDATE`.
Matches the "~4ms" estimate already written at `M-GridEdit_part6.cpp:950`.

Fix: skip when cursor+camera unchanged and no mouse button held (same correctness rule as the
pick cache; NEVER skip while a button is down or paint/sculpt/drag freeze). A transform-only
update was rejected - the pick tests `aabb_objects`, produced by RunObjectUpdateSystem.

**Only ~4.1 of the ~11.3 ms** (90.8 FPS = 11.01 ms -> 44.9 FPS = 22.27 ms). Expect ~45 -> ~55,
NOT a full restore. **~7 ms inside common_loop is still unattributed** - next step is to add
profiler sub-ranges inside `mapeditorexecutable_loop`, since the existing `CL-*` ranges only
partition the ImGui block (they summed to 0.85 ms of the 14.54 ms).

Harness: `SET_PICKUPDATE <0|1|2>` (1=gated/fix, 2=always/pre-fix for A/B, 0=never),
`FORCE_PICKUPDATE <n>` (times n full Scene::Update(0) calls directly).

Also found in passing (UI-audit backlog, not perf): the editor selection OUTLINE is dead code
in DX12 - `MasterRenderer::RenderOutlineHighlighers` has no caller, so
`pref.iEnableEditorOutlineSelection` and the "Use Outline For Selected Editor Objects"
checkbox + thickness slider are dead UI.

### Instrumentation + verification (2026-08-05, game `982d98cc`)

**The attribution gap was real and is now closed.** `CL-PreBlock` began at
`M-GridEdit_part1.cpp:79` but `mapeditorexecutable_loop` starts at line 6, so everything above
(including `commonexecutable_loop_for_game`) was unranged - which is why the `CL-*` rows summed
to 0.85 ms of a 14.54 ms parent and the panel looked like it was lying. Added `CL-GameLoop`.

**New harness commands (all needed to reproduce editor-selection costs at all):**
- `DUMP_PROFILER` - cached profiler text in ANY state (`GET_PROFILER_STATUS` is game-only).
- `SELECT_ENTITY <idx>` - CLICK only targets named ImGui widgets, so "an entity is selected"
  was previously unreproducible from the harness. **Must call
  `widget_check_for_new_object_selection()`, NOT `widget_updatewidgetobject()`** - the latter
  only CLEARS a stale selection and leaves `activeObject` at 0. That cost a build cycle.
- `SET_PICKUpdate <0|1|2>` / `FORCE_PICKUPDATE <n>` - same-session A/B of the pick scene update.

**Same-session A/B, entity selected, island.fpm:**

| | mode 2 (pre-fix) | mode 1 (fix) |
|---|---|---|
| CPU Frame | 13.97 ms | 10.29 ms |
| `Logic - common_loop` | 6.72 ms | 1.89 ms |
| `P2-mapediting` | **5.09 ms** | **0.02 ms** |
| `Animations` | **(2x) 1.12 ms** | 0.22 ms (no 2x) |
| FPS | 77.5 | 91.8 |

**The `(2x)` marker is the proof of the double `Scene::Update`** - wiProfiler differentiates
same-named ranges within a frame (`wiProfiler.cpp:341-368`) and `GetTextData` prints
`Name (2x)`. Watch for it whenever a per-frame cost seems to come from nowhere.

**STILL OPEN:** this does NOT explain the user's original capture. Theirs read
`common_loop 14.54 ms` with `P2-mapediting 0.00 ms`; my repro produces 5.09 ms WITH
P2-mapediting fat. So their exact state (properties panel open on a real mouse click) has a
second contributor that harness-driven selection does not trigger. One `DUMP_PROFILER` in that
state on the new build should name it.

### ~~THE REMAINING ~7 ms FOUND: hovering the viewport costs 6.5 ms~~ **WRONG - SEE BELOW** (2026-08-05)

`P2-calclocalcursor` (`input_calculatelocalcursor`, `M-GridEdit_part3.cpp:1357`) costs
**6.4-6.8 ms/frame** and is gated on **nothing but the mouse cursor being over the 3D
viewport**. Nothing selected, nothing moving, no button held.

| state (island.fpm, real OS cursor driven from PowerShell) | FPS | P2-calclocalcursor |
|---|---|---|
| cursor OUTSIDE viewport, nothing selected | 91-92 | ~0 |
| cursor OVER viewport, nothing selected | 66.7 | **6.39 ms** |
| cursor OVER viewport + entity selected | 64.5 | **6.79 ms** |

**WHY EVERY EARLIER HARNESS MEASUREMENT MISSED IT: automation leaves the cursor parked
outside the MAX window.** Any editor-perf work driven purely by the harness will read ~92 FPS
and see nothing. To reproduce a user's editor FPS you MUST place the OS cursor over the
viewport first:
`powershell -Command "Add-Type -AssemblyName System.Windows.Forms; [System.Windows.Forms.Cursor]::Position = New-Object System.Drawing.Point(900,500)"`

This closes the arithmetic on the user's 14.54 ms capture:
6.5 (cursor hover) + 4.6 (widget Scene::Update, fixed in `31710edc`) = 11.1 ms over a ~1.4 ms
idle baseline.

Note the pick counters do NOT reveal it: 30,912 cache hits vs 98 real runs while it was
costing 6.4 ms, so this path's raycasting is not going through `WickedCall_GetPick`'s cache.

**NOT YET FIXED** - `input_calculatelocalcursor` carries the same correctness constraints as
the pick cache (paint/sculpt/drag read the pick-derived cursor and gate on it CHANGING each
frame), so it needs its own pass, not a bolt-on.

### CORRECTION + REAL ANSWER: it is `P2-widget_loop`, and it is TRIGGER-ZONE-SPECIFIC

**The cursor-hover conclusion above was WRONG.** The user refuted it with a cleaner experiment:
cursor over the viewport with a BARREL selected = 90 FPS; click the TRIGGER ZONE = 53 FPS; then
move the cursor OUT of the viewport and it STAYS at 53. Cursor position is not the variable -
**which entity is selected** is. My measurement was confounded (I compared cursor-in/cursor-out
with nothing selected, against a different baseline).

**Reproduced exactly** on `island_150.fpm` (entity 1199 = "Barrel (Explosive) break",
entity 1203 = "Trigger Zone"), via `SELECT_ENTITY`:

| selected | FPS | CPU Frame | `common_loop` | `P2-widget_loop` |
|---|---|---|---|---|
| Barrel 1199 | 92.8 | 10.29 ms | 1.72 ms | **0.20 ms** |
| Trigger Zone 1203 | 48.8 | 20.21 ms | 12.33 ms | **11.07 ms** |

(user measured 90.5 vs 53.0 - matches)

**RULED OUT by measurement**, do not re-chase:
- the widget pick `Scene::Update` - `SET_PICKUPDATE 0` (never update) left it at 49.4 FPS /
  widget_loop 11.19 ms, i.e. unchanged;
- `P2-calclocalcursor` 0.02 ms, `P2-mapediting` 0.02 ms in this state;
- particles/emitters 0.00 ms throughout.

**NEXT STEP:** `widget_loop` (`M-Widget.cpp:441`) needs sub-ranges. Prime suspects are the
per-frame `widget_show_widget` tail (`:1237+`) which loops `widgetMAXObj` calling
`HideObject` + `WickedCall_SetObjectCastShadows` every frame, and the marker-specific branches
at `:866-873` / `:1117` / `:1358` (`ismarker`). A Trigger Zone is a marker with a large zone
box; a barrel is `ismarker == 0`.

### SOLVED: it was the zone waypoint rebuild (2026-08-05, game `9f37dab8`)

`widget_loop` sub-ranges (`WL-Setup`/`WL-PickSection`/`WL-Interact`/`WL-Waypoint`) landed it in
one run: **`WL-Waypoint` = 10.91 ms**, everything else 0.00.

**The line**: `M-Widget.cpp` `widget_loop`, the block commented *"update waypoint object when
widget MOVES entity"* - it ran EVERY FRAME while a zone was merely SELECTED.
`waypoint_movetothiscoordinate()` (`M-Waypoint.cpp`) walks every node of the zone and, with
`g_bZonesFollowTerrain` on, issues **up to two `BT_GetGroundHeight` terrain raycasts per node**,
then refreshes the zone object. A non-zone entity has no `waypointzoneindex` so it skips the
block entirely - which is exactly why a barrel was free and a Trigger Zone cost 11 ms.

Fix: only redo when the entity actually moved (index or x/y/z changed); always redo while a
mouse button is held so dragging still tracks live (same rule as the pick cache).

|  | barrel | zone BEFORE | zone AFTER |
|---|---|---|---|
| FPS | 91.0 | 49.8 | **91.1** |
| `common_loop` | 1.55 ms | 12.26 ms | 1.20 ms |
| `P2-widget_loop` | 0.21 ms | 11.19 ms | 0.23 ms |
| `WL-Waypoint` | 0.00 ms | 10.91 ms | 0.00 ms |

**METHOD LESSON (I got this wrong twice first):** I twice announced a cause from inference
(cursor-hover; the widget `Scene::Update`) and the user refuted both with a cleaner A/B. What
actually worked was *bisecting with profiler ranges* - `P2-widget_loop` named the function,
`WL-Waypoint` named the block, one build each. Instrument and bisect; do not reason about which
candidate "feels" expensive.

**Harness pattern that made it possible:** `SELECT_ENTITY <idx>` + `GET_ENTITY <idx>` to find
the entity by name (1199 barrel / 1203 Trigger Zone in `island_150.fpm`), then `DUMP_PROFILER`.
Swap a level in from `Documents\...\mapbank\_automatedbackups\` with an md5 backup+restore of
the user's own `island.fpm`.

**USER-CONFIRMED 2026-08-05:** dragging a zone and dragging its individual waypoint nodes still
work correctly. Fix is signed off.
