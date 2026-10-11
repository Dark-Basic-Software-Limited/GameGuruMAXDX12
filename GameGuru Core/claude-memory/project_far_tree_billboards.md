---
name: project-far-tree-billboards
description: "The DX12 distant-tree billboard campaign (2.95-3.07) - restored system, pool cap, VRAM slices, and the three rendering defects it shipped with."
metadata: 
  node_type: memory
  type: project
  originSessionId: 9a28c586-4c13-4447-916e-7fb51301bfa8
  modified: 2026-08-24T04:56:47.764Z
---

Full narrative: `GameGuru Core/NIGHT_INVESTIGATIONS_2026-08-12.md` §2.95-§3.07.
Design: `GameGuru Core/DESIGN_FAR_TREES.md`. Related: [[project-terrain-vt-perf]].

## What shipped

DX12 drew **no** far trees; DX11 renders LOD0 meshes only to `lod_dist` (3000in = **76 m**) and
billboards everything beyond. Our pool spent 6000 ECS objects out to 630 m — the LOD budget was
inverted. 2.96 reconnected DX11's own billboard system on the `customDraw_*` hooks: zero scene
entities, one draw per visible chunk. `gg_far_tree_pass`, default ON.

- **2.97 pool cap** — real meshes now stop at `lod_dist + 2*500` = 4000u where billboards take
  over (was 24,812u, so both drew across a 550 m band). −40% POLYS, +7% FPS, identical picture.
- **2.99 per-level atlas slices** — atlases used to allocate AND upload all 38 tree types
  regardless of what the level places. Now scans placed instances and uploads only the used count
  (13 of 38 on spotshadowtest). −51.5 MB, zero quality cost. Aztec in-game 4026.6 → 3941.2 MB.
  (3.102: no longer one-shot - `GGTrees_RefreshBillboardAtlasTypes` rebuilds it when a placed type has no slice.)
- **3.03 handover fade** — `ObjectComponent::draw_distance` (defaults FLT_MAX, so every pool tree
  had opted out) now dissolves the mesh out where the billboard has dissolved in.
- **3.07 shade wrap** — `gg_tree_shade_wrap`, default **0.5** (Lee's pick).

## Knobs

`SET_FARTREES`, `SET_TREEPOOLCAP`, `SET_TREEMESHFADE`, `SET_TREEPREPASSREACH`,
`SET_TREEDEBUGSOLID 0..5`, `SET_TREESHADEWRAP`. setup.ini `treedebugsolid`, `treeshadewrap`.
★ `SET_TREEDEBUGSOLID` modes 4 (no prepass depth write) and 5 (colour depth test ALWAYS) are what
cracked the flicker — keep them.

## The billboard shading technique (deliberate, do not "fix")

One image per tree, made to read as 3D from any angle by three choices in `GGTreesPS`:
`normal.y = abs(normal.y)` forces every baked normal upward; the normals are **rotated with the
quad** so the shading spins with it; and `lerp(dir, normal, 2)` — t=2 is **extrapolation**,
amplifying deviation from the flat quad normal. Measured: stays 24–69% shaded at every sun
azimuth, never fully lit or fully black, half the brightness swing of a flat quad (which flips
0%↔100% and goes black for half the compass).

## Gotchas earned here

- ⚠ **POLYS bit-identical is DEAD as an acceptance test for tree work** — changed on 13/19 demos
  by design. Needs replacing with POLYS-excluding-far-trees + a billboard instance count.
- ⚠ Billboard **shadowmap and envprobe VS have no clip output at all**, so far trees still cast
  shadows and appear in probes past the terrain edge. Unresolved, probably invisible.
- ⚠ Don't reach for `GGTrees_HideAll` / `GGGrass_RemoveAll` / `bWaterEnable` as content levers —
  they are LEVEL-DATA MUTATORS.
- ★★★ **3.102 (2026-10-10, first tester report):** the legacy palette-change upload (`GGTrees_LoadTextures`) still wrote billboards at slice = TYPE index into the per-level atlas - Autumn Birch 2/3 = slices 36/37 of 12 = GPU memory corruption, terrain torn into white sheets. Upload removed, loader bounds-checked, atlas rebuilds for new types. ⚠ `GGTreesShadowMapPS.hlsl` still samples by TYPE index (unwired today). `SET_FARTREES` with no arg is now READ-ONLY (it used to switch the pass off). Notes §3.102.

★ 3.108 (2026-10-11, tester report #5): a pool tree's MESH shadow is cut in ONE frame - at Tree Shadow LOD Distance (XZ) or, first, at the pool end (engine caster cull at draw_distance + radius; the shadow pass ignores the distance dither, batches use distance 0). Then only the chunk proxy's two crossed cards cast, and in cascade 2 they cannot self-shadow the tree -> it pops lighter. The pool end follows the QUALITY PRESET (lod_dist 1000/2000/3000/4000, cap +1000), so the slider is dead above ~lod_dist+900; Lee's Test Game runs LOW (testgamequality=0) -> pop at ~1850 u. Proxies also cast for NEAR trees (double shadow). DX11 dissolved both shadows over ~1000 u with noise and discarded near card shadows. Tools: tree_shadow_sweep(_game).sh.
