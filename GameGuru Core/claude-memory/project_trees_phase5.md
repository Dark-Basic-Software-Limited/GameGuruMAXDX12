---
name: project-trees-phase5
description: Terrain port Phase 5 tree work. Stages 1-4.3 done. Steady state = 10K pool + nearest-N-to-camera pick, ImpostorComponent removed entirely, SetNotVisibleInReflections on pool objects, green branch baseColor. Water reflections clean. Distant-hillside coverage sacrificed by design — pool covers what's visible.
metadata: 
  node_type: memory
  type: project
  originSessionId: 63b96d44-9243-46fb-bc37-9eb4d4be60b9
---

Terrain port Phase 5 is **tree rendering** on the new Wicked terrain, tracked against the DX11 baseline shot of TESTPRO1 `island.fpm` (see [[project-dx11-parity-baseline]]).

## Stage 1: Cylinder placeholders — DONE 2026-07-12 (commit 7a842b78)

10000-slot ObjectComponent pool + one shared cylinder MeshComponent. Confirmed placement/scale/transform math against `pAllTrees[]` positions.

## Stage 2: Real per-type trunk meshes — DONE 2026-07-12 (commit f2f3e467)

38 shared MeshComponents from `TreeMeshHigh` (VertexTreeHigh: pos + packed R8G8B8A8_UNORM normal + UV). Trunk DDS from `Files/treebank/textures/<textureName>` wired as basecolor. FPS 38 on TESTPRO1 island.

## Stage 3: Alpha-tested leaves on branches — DONE 2026-07-12 (commit 89034316)

Per-type mesh grew a second subset for branches, own material with `SetAlphaRef(0.5f)` + `SetDoubleSided(true)`. Full canopies visible on near+mid trees. FPS 27.

## Stage 3.1: Pool cap 10000 → 100000 → 30000 (commit 19927366)

DX11 baseline has tens of thousands of visible trees; 10000 pool caught only near-camera. 100000 covered all but crashed FPS to 10.6. Settled on 30000 as workable-editor compromise. **Superseded by Stage 4.2** which achieved full visible coverage at 10K by picking nearest-to-camera each frame instead of first-N-in-array-order.

## Stage 4: Distance impostor billboards via ImpostorComponent — DONE MECHANISM, BROKEN COLOR (commit 95d622e8)

`wi::scene::ImpostorComponent` attached to each per-type mesh entity with `swapInDistance = 2000` inches. Wicked auto-bakes 36 angles into `impostorArray`, swaps far objects to camera-facing quads.

**PROBLEM (unresolved):** the atlas bakes leaves as opaque WHITE blobs instead of the leaves' green. Impostors kick in past 2000 units and render white rectangles.

## Stage 4.1: RECAPTURE + GREEN FALLBACK attempts — REVERTED (was commit 29fa2564, undone by 18a95978)

Two impostor-colour fix attempts (30-frame recapture countdown, green baseColor fallback) — neither fixed the white blobs. Reverted in 18a95978 as dead code; they were never load-bearing.

## Stage 4.2: Nearest-N-to-camera pool fill + 10K pool — DONE 2026-07-13 (commit 18a95978) — MILESTONE

**This is the current steady state.** GG_TREE_POOL_SIZE = 10000. Every frame:

1. Iterate `pAllTrees[]`, filter by `IsVisible && !IsInvalid && !IsFlattened` + type < GG_TREE_TYPES (38) + mesh valid
2. For each surviving candidate, compute XZ squared distance to `wi::scene::GetCamera().Eye`
3. If more than 10K candidates, `std::nth_element` to partition the 10K nearest to the front
4. Bind those 10K to `g_treePoolEntities[0..N-1]` — set meshID, scale, translate, UpdateTransform
5. Hide remaining pool slots (`SetRenderable(false)`)

**Cost:** ~few ms CPU for 400K distance calcs + partial sort per frame. Negligible vs the ECS overhead we save by keeping the pool at 10K.

**Visible outcome:** as the camera moves, the 10K "budget" is always spent on the trees the player can actually see. Full-coverage visible-area foliage, no more "beach with 6 trees" sparse look.

**Trade-off:** distant trees beyond the nearest-N radius get NO representation — no mesh, no impostor (the impostor mechanism was retired in Stage 4.3, not fixed). If the DX11 A/B shows a horizon gap that matters, the path is hand-rolled billboards, after which the pool could grow again.

## Key file references

- [GGTrees_part2.cpp:28](GameGuru Core/Guru-WickedMAX/GGTerrain/GGTrees_part2.cpp:28) — `GG_TREE_POOL_SIZE = 10000`
- [GGTrees_part2.cpp GGTrees_WickedUpdate](GameGuru Core/Guru-WickedMAX/GGTerrain/GGTrees_part2.cpp) — nearest-N pick + pool bind
- [GGTrees_part0.cpp:225](GameGuru Core/Guru-WickedMAX/GGTerrain/GGTrees_part0.cpp:225) — `InstanceTree::GetType()` masks 6 bits (0-63), but `g_GGTrees[]` only has 38 entries. Trees with saved type >= 38 (custom palette slots not yet ported) get silently dropped by our type filter. Documented for future work.

## Stage 4.3: Drop ImpostorComponent + reflection exclusion + green branch tint — DONE 2026-07-13 (commit 2e0ad1ef)

The Stage 4 impostor billboard mechanism is retired, not fixed.

**Root cause the user's testing exposed:** `wiRenderer.cpp:7298 RenderImpostors` uses a separate `DrawIndexedInstancedIndirect` path that does NOT check `ObjectComponent::IsNotVisibleInReflections`. Setting that flag on our pool objects correctly hid near-mesh trees from the water reflection pass, but the impostor billboards still rendered into the water via their own indirect draw, showing up as bright white splats. Also: the atlas capture bakes with `color = 1` fallback (BASECOLORMAP not loaded at capture time) which then Lambert-lights to sun-bright white in the impostor render.

**Three changes landed:**

1. **`scene.impostors.Create(...)` and `swapInDistance` REMOVED.** No more Wicked ImpostorComponent per tree mesh. Wicked's atlas bake, indirect draw, and reflection artifact are all gone.
2. **`obj.SetNotVisibleInReflections(true)` set once at pool creation.** Suppresses near-mesh trees from water reflection. Persists across every `SetRenderable(true/false)` from the update loop.
3. **Green branch baseColor `(0.20, 0.45, 0.15)`.** Tints the branch material's fallback colour if the DDS fails to load. Was tried in reverted Stage 4.1 and dismissed; kept here as a defensive tint even though ImpostorComponent removal makes the impostor case moot.

**Result:** clean night-time water reflection at TESTPRO1 shore, ~37 FPS, no white splats. Distant-hillside foliage coverage sacrificed — trees only render within the nearest-N-to-camera radius.

**Trade-off accepted:** the pixelated impostor treeline on the far horizon is gone. If you want it back later, the path is NOT to re-enable Wicked's ImpostorComponent (broken colour + reflection artifact) but to roll our own billboards using GG's authored `billboardFilename` DDS (`*_BB_SF_*_color.dds`). We control the material, the alpha edges, and reflection participation.

## Future work (post-hand-rolled-billboards)

- **Grow pool past 10K** — only worthwhile once a custom billboard system covers distant trees cheaply (impostors are retired, not fixed — never re-attach ImpostorComponent). Test 30K, 50K to find the ECS-overhead vs coverage sweet spot.
- **Custom palette tree slots** — production levels may use tree types >= 38 (GG's palette can extend beyond 38 stock, similar to grass custom slots). Port the custom-tree loading path so those types render instead of being silently dropped; at minimum add a one-time warning when the type filter drops trees.
- **Wind sway on branches** — DX11 has it via per-vertex vertex-shader animation.
- **Y=0 after load** — see "2026-07-17 review corrections" below for the actual mechanics and the two cheap fixes.

## Stage 4.4: Alpha-mip fix + 20K pool = distant red treeline — DONE 2026-07-18 (commit 841e54cf)

**The distant-treeline gap was mostly NOT a pool-coverage problem — it was alpha-test mip erosion.** Nearest-N reached the far ridge, but mipmapped leaf alpha averages toward mid/low values and the clip threshold erased nearly every canopy pixel at distance (mountains read bare green while DX11 showed a red carpet).

**CRITICAL API FACT: Wicked's `SetAlphaRef` is INVERTED** — the shader clips at `(1 - alphaRef)`, so HIGHER alphaRef keeps MORE texels. Setting 0.15 (intending a loose cutoff) actually clips at 0.85 and strips canopies bare. Current setting: `alphaRef 0.85` = clip below alpha 0.15.

Pool sweep with fixed alpha (TESTPRO1 red-tree baseline): 10K = 29.4 FPS / mid-ridge carpet only; **20K = 22.3 FPS / full carpet to upper slopes (SETTLED)**; 30K = 16.4 FPS / marginal gain. Remaining delta vs DX11 (~20%): highest slope band + far-horizon hills — that's the hand-rolled-billboards discussion, not worth 30K's FPS.

## Stage P.1: Stable slot binding + all three review-flagged pool bugs FIXED — 2026-07-18 (commit fc1575e2)

The perf push rewrote `GGTrees_WickedUpdate` (see [[project-performance]] for numbers):

- **Stable slot→tree binding with change-diff** — slots keep their tree until the nearest-N set evicts them; per-slot cache (x/y/z/scale/type/band) verifies in-place edits; LOD band crossings swap meshID only. This IS the TAA-ghosting fix (review item 3a).
- **Selection throttle** — the 400K scan + nth_element run only on camera move >8", `g_treeInstanceStamp` change (bumped at `GGTrees_UpdateInstances` entry — the single choke point for instance rebuilds), pool reset, or a 256-frame heartbeat. Camera-move frames still pay ~8-10ms (spatial grid = future fix).
- **draw_enabled respected** (review fix a): pool parks when hidden, force-rebinds on re-show (heights were regenerated meanwhile).
- **iUpdateTrees=5 armed on SetData failure** (review fix b).
- **Setup latch guarded** (review item 3b): won't latch true with zero built types.
- **Shadows band-limited**: only band-0 (<2500") trees cast — matches DX11 `lod_dist_shadow` default exactly.
- **Occlusion queries off per-object** on pool entities (Wicked delta #6).

## 2026-07-17 review corrections (code audit — read before resuming tree work)

A code-vs-notes audit verified all Stage 4.2/4.3 claims against main HEAD and corrected three understandings:

1. **The green branch tint — CONFIRMED as a parity bug and REMOVED (commit `4597de53`, same day).** Branch baseColor (0.20, 0.45, 0.15) multiplied every leaf DDS permanently; green leaves survived it looking plausible, but red autumn birch textures went muddy brown (red × green = dark brown) — exposed the moment the user painted red trees on the new A/B baseline. Control that clinched it: grass-palette flowers rendered correctly pink (no tint on that path). Branches are now white (1,1,1,1) and red trees match DX11. Do NOT reintroduce a branch tint. (The stale comments in part2.cpp were also fixed in `72b2de27`.)
2. **Y=0 after load is normally TRANSIENT, not permanent.** An engine-side retry exists: `GGTrees_Update` (GGTrees_part0.cpp:2237-2291) re-runs `GGTrees_UpdateInstances(0)` when `ggterrain_extra_params.iUpdateTrees` counts down (5-frame backoff on failure), and terrain LOD regen sets `iUpdateTrees=20` (GGTerrain_part0.cpp:3604/3837/3860) — so a level load self-heals ~20+ frames later with approximate heights. It becomes PERMANENT only when trees are hidden: `GGTrees_Update` early-outs on `!draw_enabled && hide_until_update==0` while `GGTrees_WickedUpdate` ignores both flags and keeps rendering the pool. **Two cheap fixes when resuming:** (a) make `GGTrees_WickedUpdate` respect `draw_enabled`/`hide_until_update`; (b) in `GGTrees_SetData`, if `GGTrees_UpdateInstances(1)` returns 0, set `iUpdateTrees = 5` explicitly.
3. **Two more latent pool hazards:** (a) slot→tree assignment is UNSTABLE frame to frame (nth_element order is arbitrary), so each pool entity's previous-frame matrix belongs to a different tree → wrong motion vectors → expect TAA/motion-blur ghosting on trees until slots are keyed by tree index (fix during tree work — it is both a parity and a perf item); (b) `GGTrees_WickedSetup` latches `g_wickedTreesSetup=true` even if the tree bank isn't loaded yet (typesBuilt=0 → zero trees forever if setup wins that race) — guard so setup retries until at least one type builds.

## How to apply

**Terrain surface parity is fixed** (commit `42e927b8`, see [[project-terrain-texture-mismatch]]).
**Tree density + coverage in visible area is fixed** (commit `18a95978`).
**Water reflection / impostor artifacts are fixed** (commit `2e0ad1ef`).

**Next open thread (Monday):** distant-hillside foliage coverage. Nearest-N pick + 10K pool sacrifices trees beyond the pool radius. If the DX11 A/B baseline still shows a noticeable delta on far horizons, the right path is a hand-rolled billboard system driven by GG's authored `billboardFilename` DDS per tree type. Do NOT re-enable Wicked's ImpostorComponent — the reflection path is broken engine-side.

Related: [[project-dx11-parity-baseline]], [[feedback-two-attempts-change-approach]], [[feedback-dont-thrash-on-automation]].
