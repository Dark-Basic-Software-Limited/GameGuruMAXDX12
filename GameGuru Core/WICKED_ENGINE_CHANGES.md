# Wicked Engine Side Changes — Brief for Upstream Author

This document tracks every change we have made to the `WickedEngineDX12`
clone at `D:\max\WickedEngineDX12` while porting GameGuru MAX to Wicked.

Two categories:

1. **Genuine bug fixes** — should be considered for upstream merge.
2. **Temporary debug overrides** — active during analysis; must be reverted
   before any test is treated as final and before any upstream brief.

The WickedEngineDX12 repo is a separate git repo from GameGuruMAXDX12.
Tracking the changes here (inside GameGuru Core) so they survive in source
control even if the Wicked clone is reset or re-cloned.

---

## 1. Bug fixes (candidates for upstream)

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
| 2.1 hairparticlePS white | reverted 2026-06-18 | none — shader back to upstream state |
| 2.2 hairparticle_simulateCS overrides | reverted 2026-06-18 | none — shader back to upstream state |
| 2.3 hairparticlePS_prepass alpha=1 | reverted 2026-06-18 | none — shader back to upstream state |

Update this table any time we add, revert, or commit a change to the
`WickedEngineDX12` clone.
