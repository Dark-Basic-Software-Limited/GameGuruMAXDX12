---
name: project-dx11-parity-baseline
description: The visual acceptance criterion for the DX12 port — TESTPRO1 island level with a saved camera; DX11 vs DX12 screenshots must be indistinguishable so upgrading users see no surprises.
metadata: 
  node_type: memory
  type: project
  originSessionId: 63b96d44-9243-46fb-bc37-9eb4d4be60b9
---

The DX12 port's success criterion is a **screenshot A/B against DX11 on a fixed baseline**. Users upgrading from DX11 → DX12 must not see surprises — same level, same camera, same look.

## Baseline

- **Level**: TESTPRO1 island level (already the perf-testing target — see `GameGuru Core/WETEST.md` section "Performance Testing: TESTPRO1 Island Level"). Default project on launch (My Games tab).
- **Camera**: SAVED IN THE LEVEL. Loading the level restores the camera position/rotation — no harness commands, no manual framing needed.
- **A/B workflow**: user loads DX11 GameGuru MAX → island → snapshot. Then loads DX12 GameGuru MAX → same island → snapshot. Compare side by side.

## Reference DX11 shot (2026-07-12)

Rich foliage on mountain slopes, mid-ground palms/pines by the river, sandy shore with scattered trees, waterfall on the left, calm river mouth centre-right. FPS ~390 on the user's rig. Trees have real trunks + branches + billboards visible in the deep background — this is the LOD chain we need to reproduce.

## Parity scorecard (updated 2026-07-17)

- **MET** — Real tree meshes: trunk + branches per type, all 38 in `TreeMeshes/TreeMeshesLOD0.h` (Stage 2-3, 2026-07-12).
- **MET** — Same tree POSITIONS (`pAllTrees[]` is the source of truth in both engines; nearest-N pick covers the visible area, Stage 4.2).
- **MET** — Terrain surface colour (`42e927b8`, green mountains).
- **MET** — Grass (Stage B.10).
- **~80% MET (2026-07-18, `841e54cf`)** — Far-distance treeline: alpha-mip erosion fixed (alphaRef 0.85, Wicked semantics inverted) + pool 20K → red carpet across the far ridge matches DX11 at ~80%. Remaining: highest slope band + far-horizon hills (billboard discussion if user wants the last 20%).
- **OPEN** — Wind/sway (Tree Wind slider drives DX11's per-vertex vertex shader animation).
- **MET (2026-07-17)** — Leaf colour: the permanent green branch tint was removed (`4597de53`); leaf DDS colours pass through as authored, red autumn birches match DX11 on the new baseline scene.
- **NEW BASELINE (2026-07-17)**: the user rebuilt the A/B scene in DX11 — daytime, red autumn birches near+far, cacti, pink flowers, Brick Pyramid entity. Saved as .ele v342.
- **MET (2026-07-18)** — Entities: v342 ported (`ed919e44`), the Brick Pyramid loads and renders in DX12 matching the DX11 shot ([[project-level-version-debt]] RESOLVED).
- **UNVERIFIED** — Shadow behaviour (individual tree shadows visible in near ground on the DX11 shot); tree reflections excluded by design in DX12 (SetNotVisibleInReflections) — check DX11's water for whether trees reflect there.

## How to apply

- Every DX12 change touching trees, foliage, terrain colour, or lighting gets A/B'd on this baseline before it counts as done.
- If a specific finding (e.g. "shadows are darker in DX12") comes up, capture BOTH shots at that moment for the record — memory-of-visuals rots faster than screenshots do.
- **Baseline integrity warning (2026-07-17):** production DX11 writes .ele v342, which DX12 silently drops all entities from ([[project-level-version-debt]]). If TESTPRO1 is ever re-saved in DX11, the DX12 side of the A/B may silently lose entities — make the version drop loud before trusting further comparisons.
- **Saved camera note (2026-07-18, corrected):** island.fpm's stored camera is now the user's shadow-test **cliff view at (-8631, 1609, 4238)** (beach curve + waterfall cliffs), superseding the classic panorama (-5810, 550, 3778). The intermediate "dark plateau" scare was NOT a camera change — it was the same cliff camera looking at WRONG-HEIGHT terrain from the reverted pregeneration bug (`cc4b23e6`). A/B screenshots should use the cliff view until the user re-saves a different camera.
