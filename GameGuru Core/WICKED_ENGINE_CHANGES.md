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
| 1.12 Terrain ChunkData::merge_pending stale-mesh flag | applied 2026-07-18, lib rebuilt | candidate for upstream PR — passive field, closes the regen-vs-merge window for mesh-baking consumers (GG blendmap passes) |
| 2.1 hairparticlePS white | reverted 2026-06-18 | none — shader back to upstream state |
| 2.2 hairparticle_simulateCS overrides | reverted 2026-06-18 | none — shader back to upstream state |
| 2.3 hairparticlePS_prepass alpha=1 | reverted 2026-06-18 | none — shader back to upstream state |

Update this table any time we add, revert, or commit a change to the
`WickedEngineDX12` clone.
