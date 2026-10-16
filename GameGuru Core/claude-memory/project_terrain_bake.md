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
- ★★★ **… and what else lived inside that component's UPDATE.** (3.73, 2026-09-20) The near-tree
  ObjectComponent pool was destroyed by the bake's teardown (`GGTerrainWicked_Shutdown` ended with
  `GGTrees_WickedShutdown`) **and** could never rebuild, because its per-frame update was the last
  statement of `GGTerrainWicked_Update`, which returns early forever once `gg_no_terrain` is set.
  Measured: SCENE_OBJECTS 1410 -> 104, POOL 443 -> 0, `numTotalTrees` unchanged at 400000 (the DATA
  was never touched - only the renderer pool). Far billboards survived because they are a customDraw
  off the CPU instance arrays. Fixed by moving the pool update to `master_part1.cpp` and gating the
  teardown on `!gg_terrain_bake`. ⚠ **Init and Shutdown had to be paired** (`s_teardownTookTrees`):
  `GGTrees_WickedInit` is a state RESET that FORGETS `g_treePoolEntities[]` without removing them, so
  an unpaired re-init over a live pool orphans every object. The pool has NO terrain dependency at
  all - `GGTrees_part2.cpp` does not mention the terrain once; the coupling was pure code placement.
- ⚠ **A function static survives the shutdown of the system it belongs to.** Same round: the far-tree
  shadow proxies never came back from a bake on/off cycle (proxyChunks 256 -> 0 -> 0, SCENE_OBJECTS
  1410 -> 1172) because `s_proxyStamp` inside `GGTrees_WickedUpdate` still matched
  `g_treeInstanceStamp`, so the rebuild branch - which contains the "post-shutdown" size check meant
  for exactly this - was never entered. Hoist recovery checks OUT of change-detection branches.
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

- ★★★ **… and what HANGS OFF the components it removes.** (3.112, 2026-10-14) Every DX12 grass hair grows
  from a terrain chunk's MeshComponent and is Component_Attach'ed to it, so the bake's recursive chunk removal
  deleted ALL grass (HAIR_SYSTEMS 22 -> 0; part of the bake's "speed-up" was the grass vanishing), and the grass
  manager sat below GGTerrainWicked_Update's early return - the 3.73 shape again. Fix: grassy editable chunks MOVE
  their mesh to mesh-only "GGGrassEmitter" entities (no ObjectComponent; hairs re-parented, untouched), bare chunks
  keep CPU copies promoted on paint/Populate/grass-on, Grass Off demotes. ⚠ A moved chunk mesh carries a CPU
  triangle BVH of ~1.15 MB (wiTerrain builds one per chunk) - drop it, or 361 emitters = ~415 MB RAM.
  ⚠ GGGrass_ScanRegion's Match-Terrain-Color auto-resolve is a height+material lookup PER CELL - over a whole map in
  one frame it froze 4.4 s; use the raw GGGrass_RegionHasGrass. ⚠ Completely Empty levels never run
  GGTerrainWicked_Update, so per-bake state must be released from GGTerrainBake_Clear, not only Init.

- ★★★ **Baked vs live COLOUR (3.113, 2026-10-14).** Bake normal is packed n*0.5+0.5 in R8G8B8A8_UNORM and
  was never unpacked (flat ground lit as if tilted 35 deg) - plus SSAO / probe ambient / full sun / live material
  (GG_OPT_BAKE_* bits in ggCustomFrame.ggOptions). ★ The LIVE terrain was wrong too: GG's DX11-style auto-blend
  writes SPLAT weights (sum 1) but Wicked's compositor is a DECAL/opacity stack (stock keeps base = 1) - blended
  ground up to 25% dark and BC5 normals collapsed toward (-1,-1). Fixed with an engine splat-average mode
  (wi::terrain::gg_splat_blend, gg_tile_share bit 0); a per-vertex opacity CONVERSION is not enough (bilinear
  sampling of the per-vertex blendmap interpolates opacities non-linearly). ★ The engine's SET_TANGENTVIS lighting
  views encode c/(1+c) and SATURATE on a bright sun - use views 25/26/27 (final-colour summands). ⚠ Invalidating
  every chunk VT at once crashed WritebackTileRequestsGPU - use GGResetTerrainChunks.

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

## 3.115 (2026-10-15): sharp up close
- The bake PS sampled through s1, which the bake NEVER binds (inherited GGTrees' isotropic trilinear, or the null sampler = mip 0 only with trees off). Now bindless via push constants (b999, GGTerrainBakePush.hlsli) + static aniso s106. ★ Any GG custom draw sampling a bindable sampler slot it does not bind itself has this hidden dependency.
- An 8192 near tier is material mip 3; the live near ring samples mip 0 - magnified 8x inside ~15 m. Fix = near detail: re-run terrainBakeChunkCS's blend per pixel from RETAINED layer materials + blendmaps, faded in the bake's own LOD (lod 0 = bake mip 0 = material mip 3, same information).
- ★ Zero-frame capture window: GGTerrainWicked_Shutdown frees blendmaps and layer MaterialComponents LATER IN THE SAME FRAME as BuildFromTerrain - capture there or never. Layer table per frame via AllocateGPU (Texture Detail swaps textures inside Resources). Freeze bake-time settings (splat rule, repeats) - nothing re-bakes on a change.
- 4096 near tier + detail = 8192 + detail in every band (288 MB saved on DX12TESTLEVEL) - default change pending Lee. Probe: tools/harness/bake_sharpness_probe.sh. [[project-far-terrain]]


**3.119 Water Bake DX11 water fog (Lee asked 2026-10-16, Lee-confirmed 2026-10-16, game 5bee9345, notes §3.119, checklist 249):** DX11's "cheap water" is the FFT ocean with reflections off; GG's water fog there showed the bed by fade = min(1 - WaterFogMinAmount, exp(-4 (t - fogMin)/(fogMax - fogMin))), t = view-Z gap surface->bed boosted by t*(1+V.y) (that 1/V.y IS the distance effect), colour lit by sun/PI + 0.5 ambient, lerped toward BLACK by 0.5F. The baked plane now does exactly that as alpha a = 1 - fade*keep from the engine's rtLinearDepth (GetCamera().texture_lineardepth_index, pixel-readable in the transparent pass - the bed whether terrain live or baked): one load per water pixel, +0.008 ms. Water Fog sliders restored (Advanced Water Settings); picker A = minimum opacity (0 = DX11). Known: with Terrain Bake an entity buried under the bed reads deeper (engine resolve keeps its PrimitiveID; fix = max(depth, customDepth) in visibility_resolveCS, not taken). Harness SET_WATERBAKEDEPTH, tool water_bake_fog_probe.sh. ★ A GG compat shader can read engine bindless depth via GetCamera() indices - no new binding needed.


**3.120 baked UNDERWATER (Lee asked 2026-10-16, Lee-confirmed 2026-10-16, game 01a3a5ba / engine 84fce6e1, notes §3.120, checklist 250):** the live underwater look = ocean surface from below (Water Base Color + 0.765 x mirrored seabed) + the engine underwaterCS post (fog toward Underwater Color, rho = Underwater Fog x 0.00015, ext (1,0.1,0)). Water Bake disabled the ocean = no underwater post (its ONLY gate) and stale underwater values. Fix: engine `gg_underwater_without_ocean` runs the stock pass with the ocean off + `gg_underwater_ceiling_exact` (GG ceiling not in texture_depth) + hook `customDraw_WaterSurface` in the ocean's slot (before transparents - drawing the opaque ceiling after them painted over hair/foliage); game draws an opaque depth-writing ceiling there and writes the underwater values in both visuals branches. Bands within 1-6 of live, 0.03 ms. ★ customDraw_Opaque was the wrong slot: the volumetric-cloud upsample later in that pass keys off texture_depth and paints over anything not in it.
