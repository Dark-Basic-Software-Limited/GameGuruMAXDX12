---
name: project-terrain-editing
description: "Terrain SCULPT + PAINT working in DX12 since 2026-07-18 (d37342bd, Phase 6 bridge): Wicked-mode editor dispatch inside GGTerrain_Update + GGTerrainWicked_InvalidateRegion (invalidated chunks regen in place; processed-keys erased so blend passes re-run). Harness SCULPT_TEST/PAINT_TEST/UNDO/REDO. Also: brush ring hidden in single-tree tools, tree thumbnail full-cell clicks via ImgBtn frame_padding, brush default 100, VRAM readout from D3D12MA."
metadata: 
  node_type: memory
  type: project
  originSessionId: 63b96d44-9243-46fb-bc37-9eb4d4be60b9
  modified: 2026-08-05T15:17:24.175Z
---

# Terrain editing in DX12 — how it works (landed 2026-07-18)

## ⚡ 2026-08-05: SCULPT-GRASS FIX (game `1e9fb978`) — grass vanished permanently on sculpt

User repro: sculpt a hill on TESTPRO1 → local grass gone in a radius, forever (fresh reload
regrew fine — live-edit only). **Root cause: in-place chunk regen (`invalidated` →
Generation_Update, entity REUSED) recursively kills the attached grass hair child
(`Scene::Entity_Remove` recursion, hair-kill tracer reason=1) but the chunk returns under the
SAME entity id — so 1.85's `entityChanged` detector never fires and
`grassChunkKeyToGrassEntities` holds a DEAD merged entity with `currentTier == targetTier`;
the "already correct" early-out skips the chunk for the rest of the session.**
Fix in the ProcessGrassChunks scan: validate the recorded entities are still ALIVE; a record
holding only dead ids is dropped, the chunk is marked bare, and STAMPED so regrowth defers to
the settle gate (strokes keep re-killing; grass returns once, after the stroke ends). Heals
ANY external killer. New counter `GRASS_EXTKILLS` in GET_PERF_DATA.
Verified: sculpt/undo/re-sculpt all 6→6 systems, strands exactly 518,000 restored each cycle;
density gate baseline 6.877%/0.957 vs fixed-post-undo 6.846%/0.952 (broken build measured
0.019% — the bug in numbers). **RULE (third time this shape has bitten): a grass/terrain map
record is NOT proof the entity exists — validate liveness, never identity alone.**

Commits: `ea449c02` (brush ring + VRAM) → `d37342bd` (sculpt/paint bridge) → `e5b3fca7` (thumbnails + brush default) → `c5baaac6` (**review-fix batch** — an adversarial multi-agent review of the diff confirmed 18 findings; the five real defects are fixed here, incl. Wicked delta 1.12 `b0518c6f`). All verified on the island beach: raise spike + grass-texture disc appear correctly textured/shadowed, UNDOs restore the exact baseline.

## Review-fix batch facts (`c5baaac6` + engine `b0518c6f`)

- **`ChunkData::merge_pending` (Wicked delta 1.12)**: a regenerated chunk's main-scene mesh is STALE until the next Generation_Update merges it, but `invalidated` already reads false in that window — blend passes skip `merge_pending` too, and chunkSig includes it. Without this, a single-click sculpt could latch pre-sculpt blend weights permanently. **CRITICAL HOTFIX `df3c10e0` (engine) + `52a4e10a` (game)**: the original 1.12 set the flag on every chunk the generation spiral VISITED (all of them, every job run) — GG's blend passes then skipped the ENTIRE map forever on any level that regenerates chunks during load = the Island Showdown all-grey-terrain regression. The flag must only be set when the visit actually took the generation branch. Diagnosed via the TERRAINW_* GET_PERF_DATA counters (97k skips vs census 0 — the census reads after the merge-clear, the pass reads after the job re-marked). Bisect trail: 173c88e3 GOOD → d37342bd GOOD → c5baaac6 BAD.
- **The TERRAINW_* counters in GET_PERF_DATA are permanent tripwires** — bridge calls/marked/keys-erased, auto+paint blend totals, invalidated/merge_pending censuses, and per-reason auto-pass skip counters. If terrain blending ever looks stuck again, read them FIRST (a growing SKIP_* with a clean census = flag lifecycle bug; static totals with pending>0 = gate latch bug).
- The bridge's slot-less-material re-setup check is **gated on `edit_mode == GGTERRAIN_EDIT_PAINT`** — sculpt/undo carry the TEXTURES flag, and ungated it was a Generation_Restart livelock whenever an unpainted palette texture was selected.
- The Wicked-mode undo finalize is keyed on **which system ARMED the snapshot** (`s_wickedUndoArmType`), not the current edit_mode (which the tools window zeroes on hover — the leak corrupted cross-type undo).
- **`GGTERRAIN_INVALIDATE_NO_WICKED` flag**: the level-load delayed full invalidate passes it while the reveal is held; `check_new_terrain_parameters`'s immediate call and `reset_terrain_paint_date` pass it always. Mass regens (>32 chunks pending) engage a 50ms turbo budget + high-priority pool + 30-frame blend cadence via `s_pendingRegenCount`.
- Bridge overlap test = **tight chunk AABB + one-cell seam pad** (the bounding-sphere test made every brush dab a 3×3 chunk regen).
- Tree thumbnail cell child has **NoScrollWithMouse** (the full-cell button padding made it wheel-scrollable → permanent image crop).
- **Paint latency FIXED (`efe1a3b3` + Wicked delta 1.13 `b6f6c69a`)**: user-reported 4-5s click-to-visible on texture paint. Cause: the blend passes ended with `vt->invalidate()`, dropping sparse-VT residency and re-streaming the chunk via GPU-feedback round-trips. Both passes now set `pending_repaint_blendmap` on residency-backed chunks — the engine rebinds the rebuilt blendmap and re-renders resident tiles in place (next-frame visible). FIRST-stroke-with-new-texture glitch FIXED (`f62750ec`): RegisterPaintedMaterialSlot() assigns the blendmap slot incrementally (scene material entity + materialEntities append + materialToSlot) — NO Generation_Restart. Safe because the VT tile renderer resolves layer materials LIVE from the scene per render. Level LOAD keeps the full setup+restart path. SECOND first-use glitch source (`9b2fc376`): the ONLYLOADWHENUSED loader (GGTerrain_CheckMaterialUsed) fires a FULL-MAP InvalidateEverything(TEXTURES) after uploading a first-use texture slice — now NO_WICKED (legacy pages only). GGTerrain_ReloadTextures (Change Texture Folder) also NO_WICKED + GGTerrainWicked_OnTextureSetChanged() hook (full re-setup, correct there). The harness PAINT_TEST now calls GGTerrain_TriggerPaintTextureLoad() so it exercises the REAL first-use path (skipping it is how this escaped testing). Verified: full 32-material sweep through the real path at 58-61 FPS, per-stroke deltas 20-40 keys, zero full-map signatures. THIRD and FINAL trigger (`fde5bce3` + Wicked delta 1.14 `cbce724d`): the ENGINE restarts generation (full chunk teardown) whenever any terrain material is DIRTY — incremental registration leaves the new material dirty one frame; REAL strokes register before the same frame terrain update (restart fires = 4-5s flicker), harness strokes after (scene update launders first = never fires). Frame-order dependent — invisible to every sweep; only the user could repro. Fixed by construction: terrain.generation_restart_on_dirty_materials = false at init. LESSON: when a user repros what the harness cannot, suspect FRAME-ORDER between the real input path and the harness tick. The sculpt default-blend transient now heals as soon as the chunk regen merges.
- **Accepted no-fix findings**: editor dispatch stays live during synthetic harness tests (unattended use); legacy-LOD pHeightMapEdit read race (invisible terrain); VRAM figure is LOCAL-segment only (that IS the video memory number).

## The two halves of the sculpt/paint fix

1. **Editor dispatch runs in Wicked mode** (GGTerrain_part0.cpp, the `ggterrain_use_wicked_terrain` block in `GGTerrain_Update` ~line 9790): full legacy input capture into `ggterrain_internal_params.mouseLeft*` (this block is its ONLY writer — trees/grass keep their own copies), the sculpt-drag plane pick, the SCULPT/PAINT/flat-area dispatch, and the undo finalize on mouse release (sculpt/paint only — grass creates its own undo action). The legacy tail below the early return still never runs.
2. **`GGTerrainWicked_InvalidateRegion(minX,minZ,maxX,maxZ,flags)`** — called from inside `GGTerrain_InvalidateRegion` (so undo/reset/import paths bridge automatically). CHUNKS flag → overlapping chunks (bounding-sphere test, radius overshoot deliberately catches seam neighbours) get `cd.invalidated = true` → upstream `Generation_Update` regenerates their meshes **in place, entity reused** (the spline-editing machinery). Any flag → erase the chunk's keys from `processedChunkKeys`/`dx11BlendProcessedKeys` (+ entity maps) so both blendmap passes re-run. TEXTURES-only edits (texture paint) skip the mesh regen entirely. Painting a material with **no blendmap slot yet** trips `wickedTerrainMaterialsSetup = false` (same full re-setup as level load).

## Invariants that keep it correct (do not remove)

- Both blend passes **skip chunks with `invalidated == true`** without marking them processed — painting a mesh that is about to be regenerated silently loses the work.
- `chunkSig` (the blend-gate change signature in GGTerrainWicked_Update) **includes the invalidated bit** — invalidation fires the gates once (passes skip + latch), regen completion flips the bit back and fires them again to actually repaint. Without this the repaint never happens (entity is reused, layer count unchanged → sig would never move).
- `GGTerrainWicked_InvalidateRegion` calls `Generation_Cancel` before touching the chunks map (generator thread mutates it) and early-outs when the map is empty (protects the initial build from the load-time InvalidateEverything calls).
- Heights truth is CPU-side (`pHeightMapEdit` on top of readback base) — chunk regen picks it up via GGHeightModifier; no legacy regen needed.

## Editor polish facts

- **Brush highlight ring hidden in single-tree tools** (ADD/REMOVE/MOVE/SCALE — `ggtrees_global_params.paint_mode` 1/2/3/5); spray modes + sculpt/paint/grass keep it. Decided in the same GGTerrain_Update block (`brushVisibleW`).
- **Tree thumbnail full-cell clicks** (M-TerrainNew_part2.cpp ~1590): the billboard image is narrower than the cell (`imageScaleX` 0.3-0.8) and used to BE the button. Fix: pass the centering offset as `ImgBtn` **frame_padding** — the button bb widens to the full cell (child window clips the vertical overshoot), the image draws exactly where it always did. No per-pixel alpha testing was ever involved.
- **Brush size default 100** (GGTerrain.h `GGTerrainRenderParams2.brushSize`, was 500) — one shared variable for sculpt/paint/tree/grass; no persistence overrides it.
- **VRam figure (top-right) reads the Wicked device now**: `GetTotalVramUsage` (DBDLLCore_part2.cpp) → `WickedCall_GetVRAMUsageMB` (wickedcalls_part3.cpp) → `wiGraphics::GetDevice()->GetMemoryUsage().usage` (D3D12MA). The old standalone DXGI EnumAdapters(0) query watched the wrong GPU; kept only as fallback while the device is null. TESTPRO1 editor reads ~5.5 GB.

## Harness additions (docs in WETEST.md)

`SCULPT_TEST <x> <z> <frames> [mode]` / `PAINT_TEST <x> <z> <material> <frames>` — synthetic strokes through the REAL apply functions (one brush tick per frame, proper undo finalize, restores editor modes after). `UNDO` / `REDO` — editor entry points (`undosys_undoevent`/`undosys_redoevent`). Get a target position from `GET_PERF_DATA`'s CAMERA_EYE/CAMERA_AT (project the ray to y=0).

## Sculpt-drag glitch fix (2026-07-19, Wicked delta 1.15 — engine `784ad539`, game `528b2bfb`)

User repro: raise/lower with LMB held = chunk-shaped blur/checker + wrong-texture flash under the brush for the WHOLE drag, healing on release. TWO stacked engine causes, both per drag frame (the bridge invalidates the brush chunks every tick, the engine regenerates them in place): (1) the regen branch overwrote `blendmap_layers` with engine-default base/slope/altitude weights (resize(4) also truncated GG's N-layer maps) and recreated the GPU blendmap = wrong-texture flash; (2) the merge epilogue for removable/regenerated chunks called `vt->invalidate()` = sparse-VT residency reset = multi-frame tile re-stream = the blur. Fix `gg_preserve_blendmap_on_regen` (Terrain flag, default false = stock): regen touches ONLY the mesh — layers, GPU blendmap texture and VT residency all survive; the merge still replaces the chunk MaterialComponent with a fresh bindingless one, caught by `gg_material_rebind` in UpdateVirtualTexturesCPU (`vt.resolution == required && !material->textures[0].resource.IsValid()`) which re-runs the atlas bind block WITHOUT `vt.init()`. Resulting lifecycle: blendmap FROZEN during the drag (stretches over the moving mesh — no live slope retexture), ONE auto+paint blend recompute lands at release via the bridge-erased keys (verified by counters: AUTOBLEND +0 across 262 drag ticks, +1 at release; census back to 0). Mid-drag screenshots rock-solid. If live-during-drag slope retexture is ever wanted, the gate to relax is the pass skip on `invalidated` chunks — NOT the engine preservation.

## Known edges (untested / accepted)

- Trees/grass sitting on sculpted ground: grass hair re-samples the regenerated chunk mesh automatically; tree Y positions come from the tree system's own height sampling — if trees float after a big sculpt, that's the follow-up (check DX11 behavior first).
- Sculpting during the initial level build is gated only by the empty-map early-out; normal editing is the tested path.
- Paint boundaries are per-vertex all-or-nothing (pre-existing Phase 3 behavior).

Related: [[project-terrain-blendmaps]], [[project-terrain-texture-mismatch]], [[project-next-action-immediate]].
