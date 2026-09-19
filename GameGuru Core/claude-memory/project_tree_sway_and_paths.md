---
name: project-tree-sway-and-paths
description: "DX12 re-architected near trees from GGTrees' instanced pass into engine ObjectComponents, which silently dropped tree sway — no live DX12 draw path executes TreeWave at all."
metadata: 
  node_type: memory
  type: project
  originSessionId: 9a28c586-4c13-4447-916e-7fb51301bfa8
  modified: 2026-09-19T13:26:08.401Z
---

**MILESTONE `tree-sway-complete-2026-09-19` — BOTH paths fixed (game `873cf767`, engine `b80d5490`).**
Revert point before the entity work: `tree-sway-vegetation-2026-09-19`.
The architectural fact below matters well beyond sway.

★★ **Entity trees (3.58b)**: DX11's per-material "Object Wind" now drives the engine wind via
`MaterialComponent::userdata.x` (float bits; the field was unused by engine AND game).
`gg_tree_wind_amplitude()` reproduces DX11 exactly — global wins when non-zero, material SQUARES
itself when the global is 0 — so the slider stays live. `gg_tree_wind_weight()` computes the
`(y-120)` ramp **in the shader** from `pos_wind.y` (still object-space there), so arbitrary
imported meshes need no rebuild and no weight stream.
⚠ Detect intent by **`customShaderID == 1`**, NEVER `param1 != 0` — param1 defaults to **1** on
every entity in the game; it is `customShaderID = -1` that makes stock media inert.
⚠ DX11's 120 pedestal is hardcoded with **no per-object adjustment**, so imported trees under 120
scaled units never swayed in DX11 either. Reproduced, not fixed.
⚠ The param drives **speed AND amplitude** in DX11 (`wind*6`, `wind*0.35`); our port decouples
them deliberately (Lee's call) — amplitude from the param, rate from Wind Speed.

★★★ **DX11 has NO tree pool.** In DX11, near trees are drawn by **GGTrees' own high-detail
instanced pass** (`GGTreesHigh*VS` / `GGTreeBranchesHigh*VS`), which is exactly where the
`TreeWaveX/Z` sway calls live. **DX12 replaced that whole pass with Wicked `ObjectComponent`s
from a tree pool**, rendered by the engine's stock object shaders — which contain no TreeWave.
The sway was not deleted; the geometry moved out from under it.

**The three DX12 tree paths, and why none sways:**
1. **Near trees = tree pool ObjectComponents** (live, every frame). Stock engine object VS.
   No TreeWave. Materials never set `customShaderID`.
2. **Far trees = GGTrees billboard pass** (live). `GGTreesVS.hlsl` — never called TreeWave,
   **in either DX11 or DX12**. Far trees have never swayed; not a regression.
3. **Imported tree ENTITIES = "Tree Animate Doublesided" custom shader.** Dead in DX12:
   `CustomShaders.cpp:62-63` pushes the ENGINE shader dir **before** the GG dir and compiles the
   stubs **at runtime from an in-memory buffer with no source-dir context**, so
   `#include "objectHF.hlsli"` binds the engine header, which has no TreeWave. Proof:
   `objectVS_common_tree.cso` is **bit-identical** to stock `objectVS_common.cso`, and the
   `.wishadermeta` dependency list names the engine header. In DX11 these are `<FxCompile>`
   items compiled from a real path, so the sibling GG header wins. Also `customShaderParam1`
   does not exist in the DX12 engine, and the PSOs are VS-only shells.

★ **The fix**: engine per-vertex wind, the only mechanism reaching ObjectComponents. Weights
synthesized in `AppendTreeVerts` from DX11's `(y-120)` ramp; `SetUseWind(true)` on **both** trunk
and branch materials; two object-path scalars in `ShaderWind`'s spare padding so GG's 39.37
units/metre scale works without inflating the shared `windDirection`. Harness: `SET_TREEWIND`
(int percent, accepts >100 for diagnostics) / `GET_TREEWIND`.
⚠ **An empty `vertex_windweights` means weight 1.0, not 0** — the tree would translate rigidly.

⚠ **`GGTrees_Draw` / `_Draw_Prepass` hit an unconditional `return` before the high-detail block**,
and `GGTrees_Draw_ShadowMap` / `_Draw_EnvProbe` are hard-disabled outright
(`// DISABLED: causes GPU hang due to PSO compilation failure in DX12`).
⚠⚠ **Do NOT report this as "trees cast no shadows".** Lee confirmed 2026-09-19 that trees DO cast
shadows and DO appear in env probes — those come from the tree-pool **ObjectComponent**s via the
engine pipeline, which the disabled GGTrees passes have nothing to do with. Those `return`s only
kill the already-dead high-detail path. I got this wrong once; the visible behaviour is the
authority, not the `return`.

★★★ **HOW DX11 SWAYS WITH `tree_wind = 0` (answered 2026-09-19).** Every level on disk stores
`visuals.tree_wind=0`, yet DX11 shows swaying trees. The two paths behave completely differently
at zero, and this is the key to the user-facing regression:
- **Painted GGTrees vegetation does NOT sway at 0 in DX11 either** — `GGTreesConstants.hlsli`
  opens `if (g_xFrame_TreeWind <= 0) return (0);`, a hard gate with no fallback. So hub demos
  have still vegetation in DX11 too. **No regression on this path.**
- **Tree ENTITIES with the "Tree Animate Doublesided" material DO sway at 0**, via a deliberate
  non-zero fallback baked in when the user assigns the shader — `M-Importer.cpp:11450-11453`:
  `if (t.visuals.tree_wind > 0) param1 = 1.0f; else param1 = 0.20f;`. The shader then does
  `baseWind = isZeroWind*param1 + isNonZeroWind*treeWind; wind = baseWind * param1`, so at
  treeWind 0 it is `0.20 * 0.20 = 0.04` — a permanent gentle sway **independent of the slider**.
  ⚠ **This path is dead in DX12** (the objectHF include bug above), and 3.57 does NOT fix it —
  3.57 covers pool/vegetation materials via `BuildTreeMaterial`, not entity materials.
No stock FPE sets `customshaderid` (0 of the shipped entitybank), so it is user-assigned per
level and saved in level entity data, not in the FPE.

**The wind VALUE chain is mostly fine** — ⚠ I previously reported "no writer for treeWind"; that
was a grep scoped to `Guru-WickedMAX`. The writer is `M-GridEditB_part3.cpp:2142`
(`ggCustomFrameStaging.treeWind = visuals->tree_wind;`), uploaded at `GGTerrain_part0.cpp:812`,
bound to b4 at :823. 11 of 16 `GGCustomFrameData` fields are written there. Still broken:
the editor slider (`M-TerrainNew_part1.cpp:1056` commented out, no replacement, so it only lands
on the next unrelated visuals refresh), and Lua `SetTreeWind` (routes through the gutted
`WickedCall_UpdateTreeWind`, `wickedcalls_part3.cpp:1939`) — which breaks shipped
`weather_event.lua`. ⚠ Default `tree_wind` is **0.0** and shipped `visuals.ini` has no key, so
any A/B must set it first or a correct fix reads as "no change".

**Engine native wind is the recommended vehicle** — applied in the ONE shared
`VertexSurface::create`, so prepass/colour/shadow/envprobe coverage is correct *by construction*,
and motion vectors come free via `sample_wind_prev`. Two real obstacles: GG tree meshes write no
`vertex_windweights` (**empty means weight 1.0, not 0** — the whole tree would translate rigidly,
trunk included), and ⚠ **the engine's wind is scaled for 1 unit/metre while GG is 39.37** — the
default gives ~2.5 cm displacement with a ~5 cm spatial period, i.e. shimmer. `windDirection` is
**shared with grass, rain and spring bones**, so it must not be inflated to compensate; add
scale scalars in `ShaderWind`'s two spare padding floats instead.

DX11 calibration: a 600-unit tree's canopy peaks at **34 units** displacement at wind 0.5, 68 at
1.0. The `clamp((posy-120)*0.35, 0, posy)` pedestal is what keeps short props safe (a 137.5-unit
cactus moves 2.5 units). LOD handover is at `tree_lodDist` **default 3000**, not 500 — 500 is the
dither WIDTH of a 1000-unit cross-dissolve.

Related: [[project-customdraw-hook-inventory]], [[project-trees-phase5]],
[[project-far-tree-billboards]], [[project-rules-rendering-dx12]]
