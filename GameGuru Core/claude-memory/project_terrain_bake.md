---
name: project-terrain-bake
description: "Terrain Bake and Water Bake — the 3.25 low-spec conversion of terrain into plain meshes + BC1 textures, its two-tier resolution scheme, and the nine defects it shipped through"
metadata: 
  node_type: memory
  type: project
  originSessionId: 9a28c586-4c13-4447-916e-7fb51301bfa8
  modified: 2026-08-26T14:16:02.209Z
---

# Terrain Bake / Water Bake (GGMAX 3.25, 2026-08-26)

Built for Lee's **6-year-old AMD card**, where switching terrain off took a frame 22 → 12 ms while
the tree billboard pass stayed nearly free. The inference — and it was right — is that the cost is
the MACHINERY, not the triangles. Keep the triangles, throw the machinery away.
Full narrative: `GameGuru Core/NIGHT_INVESTIGATIONS_2026-08-12.md` §3.25–§3.25l.

## What it is

- **Terrain Bake** (replaces "Terrain Off"): every chunk → a plain mesh + a **BC1_UNORM_SRGB**
  texture, then the 2.94 teardown removes the Wicked terrain. Custom draw modelled on the far-tree
  billboard pass: one shared index buffer, per-chunk frustum cull, one draw call each, zero ECS.
  Runtime only, nothing on disk.
- **Two-tier resolution**: FAR **256** for distant scenery, NEAR **8192** for the play area, capped
  by `terrainbakenearbudget` (**1024 MB**), promoted densest-first. Mipmapped.
- **Water Bake** (replaces "Water Off"): ocean + planar reflection removed, flat plane at the
  authored Water Base Color RGBA in its place.

Knobs: `terrainbakeres` / `terrainbakeresnear` / `terrainbakenearbudget` / `waterbakealpha` /
`waterbaketint`. Harness: `SET_BAKETERRAIN`, `SET_BAKEWATER`, `SET_BAKERES`, `SET_BAKERESNEAR`,
`SET_BAKENEARBUDGET`, `SET_WATERBAKEALPHA`, `SET_WATERBAKETINT`, `SET_WATERBAKEDEBUG`, `DUMP_BAKE`.
Test scripts: `tools/baketest.sh`, `bakestress.sh`, `bakeres.sh`, `baketier.sh`, `watertest.sh`.

## ★★★ The rules this paid for

- **BC1 into a COMMITTED texture does not need sparse aliasing.** `wi::renderer::BlockCompress`
  dispatches into a shared `R32G32_UINT` scratch and does a uint2→BC1 `CopyTexture` — a legal
  cross-format block copy. The sparse alias in the terrain atlas exists only to avoid that copy for
  a huge constantly-updated atlas. ⚠ I rejected BC1 on this false premise first and paid 8× memory.
- **A `customDraw_*` hook may record DRAWS only.** A compute `Dispatch` inside a render pass →
  `DXGI_ERROR_INVALID_CALL`, app alive but deaf. Compute takes its own command list on the main
  thread. Sibling of the resource-creation rule.
- **`gpup_draw` CLOBBERS b0/b1 (FrameCB/CameraCB) and never restores them.** Anything in
  `customDraw_Transparent` that reads `g_xCamera_*` must draw BEFORE it. The engine only repairs the
  CB after the whole hook returns. ⚠ `GGTerrain_Draw_Transparent` has the same latent exposure.
- **A GG module's shaders must load at startup init** (beside `GGGrass_Init`), never on first use:
  `SHADERPATH` is relative `"shaders/"` and the CWD moves to `Max/Files`.
- **A switch that removes a component must ask what else reads that component's EXISTENCE.**
  `GG_GetTerrainViewRadius()` returns 0 when `terrains.GetCount()==0`, which silently disabled the
  tree billboards' "never draw past the terrain" cull.
- **`entityelement[].active` is RUNTIME game state and is 0 in the editor.** The editor's test for
  "this slot holds a placed object" is `bankindex > 0`. **Placed-ness ≠ aliveness.**
- **A play area must be a DENSITY measure, not a bounding box** — a box is decided by its outliers
  (measured X −105279..12093 on a ±63360 terrain). Count entities per chunk, promote densest-first,
  3×3 blur so the detail does not end in a hard seam.
- **Roll back BEFORE allocating, not after.** The all-or-nothing bake allocated ~227 MB then
  discarded it, every frame, forever. Decide readiness in a pass that allocates nothing.
- **`WaterAlpha_f` was never persisted** in any version of GG (write-only, reset to 0 each load).
  Now saved as `visuals.Wateralpha`; default stays 0 because it feeds the real ocean.
- Resolution is a **memory** dial, not a frame-time one: 193/197/190 FPS at 256/512/1024.

## Measured (fast card — the target is Lee's AMD)

| | |
|---|---|
| Canyon, bake on | FPS 174 → 228, VRAM 3.17 → 2.35 GB |
| TESTPRO2, two-tier 8192 | 24 chunks near, 1113 MB bake, 211 FPS vs 220 real, **total VRAM 4213 → 3357 MB** |
| BC1 saving | 256: 220→84 MB · 512: 689→142 MB · 1024: 2564→377 MB |

★ Total VRAM FALLS with the bake on even at 1113 MB — the SVT atlas removed is larger than the
bake replacing it.

## Still open

- ⚠ Everything above is UNVERIFIED on the AMD card it was built for.
- The 70-second `GPU Idle + unranged` decay Lee saw after unticking water is **not reproduced and
  not fixed**; if it survives it belongs to the ocean teardown.
- Editor overlay lines (waypoints/zones) looked absent over baked terrain in one screenshot —
  unverified, worth a glance.
- Reduction Scale is implemented but **unmeasured** on hardware where skinning costs anything.
