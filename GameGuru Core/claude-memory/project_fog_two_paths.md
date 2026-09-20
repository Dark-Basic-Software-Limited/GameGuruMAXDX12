---
name: project-fog-two-paths
description: "GameGuru MAX DX12 has TWO fog implementations over one scene — the engine's fogHF.hlsli GetFogAmount() for terrain/entities/near trees, and GG's ApplyFogCustom() for every customDraw (billboard trees, grass, baked terrain, baked water) — and the compat shim that fed the second one quartered the user's Fog Range, so custom draws fogged 4x too hard."
metadata:
  node_type: memory
  type: project
  originSessionId: 9a28c586-4c13-4447-916e-7fb51301bfa8
  modified: 2026-09-20T13:15:54.018Z
---

# Two fog implementations, one scene

| what is drawn | by | fogged by |
|---|---|---|
| terrain (Wicked native SVT), entities, **near** trees | the ENGINE object shader | `WickedEngine/shaders/fogHF.hlsli` `GetFogAmount()` |
| **billboard** trees, grass, baked terrain, baked water, terrain sphere, env-probe variants | GG **customDraw** shaders | `GGTerrain/Shaders/GGCommonFunctions.hlsli` `ApplyFogCustom()` |

★ **Anything that looks wrong about fog on ONE class of object and right on another is this split.**
The terrain path is [[project-terrain-render-path]] — it is the engine's, so the engine's fog is the
reference the other half must match.

## The defect (found 2026-09-20 from two of Lee's screenshots, fixed same day, notes §3.72)

GG's C++ stores the user's Fog Range as an engine DENSITY — `M-GridEditB_part3.cpp`
`Wicked_ApplyFogModel`: **`fogDensity = 4.0f / (fogFar - fogNear)`**. `GGFrameCompat.hlsli` then
inverted it for the GG shaders as `end = start + 1.0/density`. But `ApplyFogCustom`'s own curve is
`exp(dist * 4.0 / (fogMin - fogMax))` — **the 4 divides straight back out**, so the inverse had to be
`start + 4/density`. The shim handed back a range a QUARTER as long as the user set, and every GG
custom draw ran at **four times** the engine's optical depth. At Fog Range 0-100 (near 0, far 50000;
`M-Sliders.cpp` scales the second slider ×500) a tree 10 km out sat at 96% fog against terrain at 55%
— solid red billboards on barely-tinted ground.

**Fixed by mirroring the engine curve outright**, not by fixing the reconstruction: correcting
`1/density` → `4/density` makes the two agree ONLY when fogStart is 0. Above that the engine's
`startDistanceFalloff = saturate((d-start)/start)` applied over the FULL distance diverges from GG's
`(d-start)`, and the engine's height-fog branch had no GG counterpart at all. `ApplyFogCustom` now
calls `GGEngineFogAmount()`, a line-for-line copy of `GetFogAmount()`. ⚠ **Keep the two in step** —
that warning is written at the top of the function.

The COLOUR path was already correct: GG's realistic-sky branch reaches the same skyviewlut through
`GetDynamicSkyColor(..., stationary)` that the engine's `GetFog()` samples inline, and
`lerp(C, lerp(C,F,o), a)` == the engine's `amount *= opacity`. Only the amount was ever wrong.

## Rules this leaves behind

- ★★★ **A port-time compat shim is where a value gets quietly RE-DERIVED, and a re-derivation is a
  claim that nobody tests.** The macro's own comment stated the claim plainly ("Reconstruct old end =
  start + 1/density") and was wrong by a single constant that lived in the consumer one file away.
  Same shape as [[project-customdraw-hook-inventory]], [[project-tree-sway-and-paths]] and the
  billboard atlas: nothing broke, something was *converted*, and the conversion was never checked
  against what consumes it.
- **Grass and the bakes hid it.** Grass is near the camera. A baked level fogs its terrain through
  the same wrong curve, so its two halves agreed with each other and disagreed with everything else.
  ★ A defect that is self-consistent within a subsystem is invisible until something ELSE renders
  beside it.
- Diagnose with harness **`SET_FOGDENS`** — `-1` restores the level's model and prints
  `start=` / `density=`; a positive value writes density live; `0` is fog fully off.
- Repro level: **TESTPRO2 / `testpro2desertlevel2`** (storyboard node of the same name — `CLICK_NODE
  testpro2desertlevel2`, NOT `CLICK_ONLY_LEVEL`, which picks `testpro2level`). Script
  `tools/fogsync_check.sh`.
