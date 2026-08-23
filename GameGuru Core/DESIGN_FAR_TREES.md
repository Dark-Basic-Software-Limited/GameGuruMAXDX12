# Design: distant-tree billboards for DX12 ("Far Forest")

Status: **PROPOSAL for discussion.** No code written. 2026-08-23.

Problem: DX12 draws real tree meshes for the nearest 6000 trees and **nothing at all** beyond
them, so hillsides are bare. DX11's are forested to the horizon. Wicked's impostor system is
ruled out (slow, artifacts, and it ignores per-object reflection filtering).

---

## 1. The headline: we are not building this, we are reconnecting it

Two things turned up while researching this, and together they change the whole shape of the job.

**(a) The DX11 billboard shaders are already in the DX12 tree.** All of them:

    Guru-WickedMAX/GGTerrain/Shaders/
      GGTreesVS.hlsl  GGTreesPS.hlsl              <- the billboard pass
      GGTreesPrepassVS.hlsl  GGTreesPrepassPS.hlsl
      GGTreesHighShadowMapVS.hlsl  GGTreesHighShadowMapPS.hlsl
      GGTreesHighEnvProbeVS.hlsl   GGTreesHighEnvProbePS.hlsl
      GGTreesConstants.hlsli

**(b) The DX12 build still maintains DX11's per-chunk instance data, every regen, and draws
none of it.** `TreeChunk` (GGTrees_part0.cpp:431) still carries `GPUBuffer bufferInstances` and
`uint32_t numValid`, and `TreeChunk::Update()` (~:634) still fills it with `InstanceTreeGPU`
records. It has no consumer.

So the distant forest was ported and then orphaned when the GGTerrain custom-draw path was
declared dead. **The proposal is to reconnect it, not to invent a billboard system.**

---

## 2. What DX11 actually does (now fully reverse-engineered)

| Aspect | DX11 implementation |
|---|---|
| Geometry | ONE shared 4-vertex / 6-index unit quad, global, origin at the base |
| Per-tree data | 16-byte instance record: world x/y/z + one packed uint (type 6b, variation 3b, scale 7b, flags) |
| Batching | **One `DrawIndexedInstanced(6, numValid)` per chunk.** All 38 tree types in the SAME draw — type is a `Texture2DArray` slice index resolved in the shader |
| Camera facing | Y-axis (cylindrical) billboard rotated **in the vertex shader** from the camera position in a constant buffer. Nothing on the CPU rotates anything |
| LOD swap | **Per-pixel, in the pixel shader.** Billboard discards fragments nearer than `lod_dist`; the mesh shader discards fragments further than it |
| Swap quality | Noise-dithered dissolve, `GGTREES_LOD_TRANSITION = 500` units (12.7 m), AlphaToCoverage. The noise is sampled in TEXTURE space so the dissolve is anchored to the tree and is temporally stable |
| Overlap | The two bands **deliberately do not coincide** — across one 500-unit band the mesh is still fully drawn while the billboard dissolves in. Trees are double-drawn there, never gapped |
| Reflections | The near-discard is **skipped** under a clip plane, so reflections get billboards at every distance |
| ECS cost | **Zero.** It is a raw draw pass. 400,000 trees = 6.4 MB of static vertex buffer |
| Rebuilds | Instance buffers are written only when tree DATA changes. Camera motion touches nothing but a 176-byte constant buffer |

### ★ The number that reframes everything

`lod_dist = 3000` **inches = 76 m** (GGTrees.h:33; 1 unit = 1 inch).

**DX11 draws real tree meshes only within 76 m** and billboards everything from there to the
5.08 km edge of the tree area. DX12's pool spends **6000 ECS objects** on real meshes out to
**630 m** — about 8x further than DX11 ever draws one — and then draws nothing.

The LOD budget is inverted. We are paying a fortune for near-field detail nobody asked for and
getting nothing for the distance, which is the part you actually notice.

---

## 3. Proposed design

**Route: the `customDraw_*` hooks, not the ECS.**

GGMAX already wires `customDraw_Prepass`, `customDraw_Opaque`, `customDraw_ShadowMap`,
`customDraw_Prepass_Reflections` and `customDraw_EnvProbe` in `master_part1.cpp` (:434-465), and
`GGTerrain_Draw` proves the route works end to end. Each is currently short-circuited by
`if (ggterrain_use_wicked_terrain) return;`.

The far forest goes on those hooks. That gives us DX11's properties exactly:

- **Zero scene entities.** Nothing enters `Scene::Update`, nothing is culled per-object, nothing
  adds to the instance-slot blanking pass. This is the requirement that rules out the merged-mesh
  alternative (see §5).
- **One draw per visible chunk per pass**, bounded at 256, typically 40-90 for a normal FOV.
- **Static buffers**, rewritten only when tree data changes — which the DX12 build is already doing.
- **Camera-facing quads** with per-tree scale and per-type aspect, all resolved in the VS.

### The one genuinely hard problem, and why it is easy here

DX11 hides the seam by putting a matching **far-discard in the tree MESH pixel shader** — it
carries the test in five shaders. We cannot do that: our near trees are ordinary Wicked
`ObjectComponent`s using the stock object shader, and adding a custom PSO to them is a large,
precedent-free job (the one existing `CustomShader` registration in this codebase is malformed
and renders depth-only).

**We do not need to.** Our mesh side already has a distance limit that DX11's does not: the
nearest-N pool. Instead of discarding far mesh pixels in a shader, **cap the pool radius** so the
meshes simply stop where the billboards start.

    meshes:      selected by the pool, hard radius R
    billboards:  drawn for ALL trees, per-pixel discard inside (R - transition)
    overlap:     one transition band where both draw — exactly DX11's arrangement

That replaces shader surgery with one number, and it is a **large CPU saving in its own right**:
at DX11's 76 m the pool would hold a few hundred trees instead of 6000.

### What it replaces

The merged billboard shadow proxies (`GGTrees_BuildShadowProxyChunk`, 244 objects) become
redundant — DX11's system carries its own shadow pass. Retiring them removes 244 objects, 244
meshes and 244 transforms from the scene and deletes a whole code path. **Net ECS change is
negative.**

---

## 4. Cost model

| | DX11 (measured from source) | Proposed DX12 | Today's DX12 |
|---|---|---|---|
| Scene entities | 0 | **0** | 6000 pool + 244 proxies |
| Draws, far trees, per pass | 1 per visible chunk (≤256) | same | 0 (nothing drawn) |
| Static VRAM | 6.4 MB @ 400K trees | same | n/a |
| Per-frame CPU | one 176-byte CB write | same | — |
| Pool objects | trees within 76 m | trees within R | trees within 630 m |

The pool shrink is the performance story: 6000 objects at a measured ~49 ns each in
`Scene::Update` is ~0.29 ms, before their draw calls, prepass, and shadow-cascade passes.

---

## 5. The alternative I am NOT recommending

Promote the existing merged billboard proxy chunks (crossed static quads, one `ObjectComponent`
per chunk) to main-camera visibility. I prototyped this; it fails on three counts:

1. **Draw calls.** A `MeshComponent`'s unit of material binding is the subset, and one subset per
   tree type per chunk means 244 chunks x 4-12 types x 2 passes = 2,000-6,000 draws where DX11
   spends at most 256. Structural, not tunable — unless we build a billboard texture atlas, which
   is most of the work of doing it properly anyway.
2. **The quads cannot face the camera** — orientation is baked into the merged vertices. Fixed
   crossed quads go edge-on from above and swing 29% in apparent width with azimuth.
3. **Chunk-granular swap.** Chunks are 317 m against a 630 m pool radius, so per-chunk visibility
   either gaps or double-draws, and any Wicked-native cross-fade pops a whole 317 m tile.
   (Wicked's `fadeDistance` also saturates past 1.66 km on an fp16 clamp.)

It costs ~18x DX11's GPU memory for a worse picture. The only thing it has going for it is that
it already exists.

---

## 6. Risks and open questions

1. ★ **The POLYS acceptance gate will change.** "POLYS bit-identical" has been the standing test
   for every pipeline change in this project. Adding the far forest adds triangles by design. We
   need a replacement gate before this lands — probably POLYS-excluding-far-trees plus a separate
   far-tree triangle count.
2. **The near-discard must be duplicated identically into the prepass.** DX11 carries it in five
   shaders. Miss one and depth is written for fragments the colour pass discards.
3. **Choosing R.** DX11's 76 m is aggressive for a modern card and will show more billboard than
   mesh. I would start at DX11 parity to establish the look, then raise R until the CPU cost of
   the pool becomes visible. This wants your eye, not a measurement.
4. **Top-down.** Y-axis billboards go edge-on when you look straight down. DX11 lives with it
   because its swap is at 76 m; if we choose a larger R the editor camera will spend more time
   looking down at billboards. Worth checking early.
5. **Reflections.** DX11 puts billboards in reflections at every distance. Our proxies and pool
   both currently set `NotVisibleInReflections(true)`, so DX12 water has no trees at all. Ours
   would be filterable per pass because it draws through the hooks — the impostor bug cannot
   recur here, and that is a property of the architecture, not of a flag.
6. **Sculpt/paint churn.** Terrain edits call `GGTrees_UpdateInstances`, which dirties all 256
   chunks. Today that rebuilds invisible shadow geometry; afterwards it rebuilds the visible
   forest. Needs the same deferred batching the proxies already use.
7. **`ggterrain_use_wicked_terrain` gates the hooks.** The far-tree hooks must be split out from
   that flag so they run on the shipping Wicked-terrain path.

---

## 7. Suggested staging

1. **Prove the route.** Draw one chunk's billboards through `customDraw_Opaque` with the existing
   buffers and shaders. Answers "do the ported shaders still compile and bind" in an afternoon.
2. **All chunks, colour only**, frustum-culled, no near-discard, pool radius untouched. Expect
   double-drawn trees inside 630 m — that is fine and expected at this stage. Screenshot vs DX11.
3. **Add the per-pixel dither discard** to billboard and prepass. Cap the pool radius R. This is
   the point where it should look like DX11.
4. **Shadow + reflection + env-probe passes**, then retire the merged proxies.
5. **Re-gate**: new POLYS accounting, 19-demo sweep, VRAM check.

Steps 1-2 are cheap and answer most of the risk. I would not commit to 3-5 before seeing 2.
