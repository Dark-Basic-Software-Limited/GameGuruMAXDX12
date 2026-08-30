---
name: project-vt-zoom-squares
description: "RESOLVED 2026-07-19 — violent-zoom VT square artifacts fixed by delta 1.21 (expandable VT working set: ring 4 + removal margin 12). USER-CONFIRMED next day on a fresh launch: 'happy with this version', modest camera bursts no longer trigger it. Six real mechanisms fixed en route (deltas 1.15-1.21, all keepers, incl a STOCK dangling-pointer bug 1.19). Contains the DO-NOT list + resume directions if it ever regresses. Separately: Island Showdown 'messed up' (blue stretched geometry) on an IN-PLACE level switch = stale GPU/water state carried from the prior session, clears on a clean restart — NOT a code bug and NOT the VT."
metadata: 
  node_type: memory
  type: project
  originSessionId: 63b96d44-9243-46fb-bc37-9eb4d4be60b9
  modified: 2026-07-19T17:15:42.796Z
---

# VT zoom squares — RESOLVED (delta 1.21, user-confirmed 2026-07-19)

**STATUS: FIXED.** After a fresh launch the next day, the user tested camera
bursts and reported: *"Even modest bursts of speed on the camera does not show
the old VT artifact, I think we can be happy with this version."* Delta 1.21
(the expandable VT working set — ring 4 + removal margin 12) is the accepted
fix. Do NOT reopen unless it regresses; the resume directions below are the
fallback if it ever does.

**Original symptom (now fixed):** zooming the editor camera in/out at high
speed showed square texture artifacts (wrong/stale/soft tile content) that
healed when the camera settled. Slow movement was always clean. Root cause was
a stack of mechanisms (see keepers list) topped by the VT working set being too
small — the camera outran the streaming pace. Enlarging the resident ring
(1.21) so the camera travels ±4 chunks with ZERO resolution transitions closed
the gap.

**Also nailed same session — the Island Showdown "solid blue streaks" scare:**
loading Island Showdown *into an instance that had TESTPRO1 open for ~45 min*
showed blue stretched-geometry fans (vertex explosion, water/ocean rendering —
NOT terrain, NOT the VT). A clean restart + fresh load rendered perfectly. Verdict:
stale GPU/water state leaked across the in-place level switch; not a regression.
If it recurs on water-heavy levels, look at water/ocean state reset on level
switch, not the terrain path.

## What was actually fixed en route (ALL KEEPERS — do not revert)

- **1.19 part 1 (`b141786d`) — REAL STOCK BUG, upstream PR candidate:**
  `PhysicalTile::last_used` is a raw pointer into the owning
  `VirtualTexture::tiles` vector; `free()` left it dangling and heap reuse made
  `check_tile_resident` spuriously true → page table mapped tiles recycled to
  OTHER chunks + re-render skipped = persistent foreign squares. `free()` now
  nulls matching back-pointers before `tiles.clear()`.
- **1.20 hotfix (`948a2e4d`) — tail-invalid self-heal:** a chunk whose
  tail-tile allocation failed (pool exhausted) rendered flat untextured squares
  FOREVER (stock has no retry). Now re-inits automatically when the pool
  refills. Also fixes stock fragility.
- **1.17 (`4c789814`) born-correct chunks** and **1.15/1.16** (sculpt
  preservation + repaint latch) — see [[project-wicked-engine-changes]].
- **1.18/1.19 hysteresis + 1.20 recycle threshold** (`gg_vt_upgrade_hysteresis`):
  resolution changes frozen while `gg_center_stable_frames < 10` (upgrades
  budgeted 4/frame on settle); mid-motion only tiles unused 60+ frames may be
  recycled. These reduce churn and are harmless — keep, but they did NOT kill
  the residual artifact.

## DO-NOT list (paid for in blood tonight)

1. **Do NOT freeze tile aging (`free_frames++`).** It starves the recycle
   pool's REFILL: after ~2 min of sustained zooming NOTHING crossed the
   threshold, chunk inits failed allocation, and the whole island degraded to
   flat untextured squares. Protect a pool by throttling the DRAIN (threshold),
   never by freezing the refill.
2. **Do NOT let any allocation failure go without a retry path.** Stock
   silently never retries a failed tail allocation (now covered by
   gg_tail_invalid — keep it).
3. **Do NOT keep tuning steal/streaming heuristics blind.** THREE attempts
   (1.18, 1.19 part 2, 1.20) each fixed something real yet the residual
   persisted. Next debugging step, if needed, is INSTRUMENTATION: a tile-steal
   tripwire in `allocate_tile` — log owner VT, thief VT, and `free_frames` at
   steal time whenever a stolen tile's old owner is still in
   `virtual_textures_in_use` with a small idle count. Catch the mechanism in
   the act, then fix THAT.
4. **Do NOT drive MAX or rebuild while the user is hands-on testing** (check
   `Get-Process GameGuruMAX` MainWindowTitle for the unsaved asterisk).

## Structural context (why this is hard)

- The high-res ring is binary: `required_resolution = dist < 2 ? 65536 : min`
  (wiTerrain.cpp ~line 1795 area, GGMAX comment block has earlier graded-falloff
  attempts, reverted). GG is inch-scaled → chunkStride ≈ 5040 units → the
  65536-res ring is tiny and the editor camera crosses chunks in milliseconds.
- GPU feedback keep-alive/requests arrive via 2-3 frame delayed CPU readback —
  the design assumes the view changes slower than that loop.
- Atlas tile pool: 16384×16384 physical, ~120×120 tiles shared by everything.
- GG islands are BOUNDED (fixed editable area, ~650-700 chunks steady state) —
  the infinite-streaming design is oversized for the actual need.

## Direction 2 EXECUTED (2026-07-19 afternoon) — Wicked delta 1.21

**Enlarge-the-buffer landed** (engine `8ea7d29e`, game `28c0e00e`):
`gg_near_ring_dist` (full-res VT zone radius, stock 2) + `gg_removal_margin`
(extra chunks before destruction, stock 0). GG ships **ring 4 + margin 12**:
the camera travels ±4 chunks (~20k units) in ANY direction with ZERO VT
resolution transitions (double stock headroom) and chunks survive zoom travel
(no destroy/recreate cache flushes — the two churn classes behind the squares).
- **The atlas texture CANNOT grow — 16384² is the D3D12 dimension limit.** The
  working set grows via ring width instead; tile residency is feedback-driven
  (≈ screen coverage) so the physical pool cost is UNCHANGED (VRam flat 6.37).
- **Measured cost is per-chunk residency structures + per-frame update work:
  ring 6 (169 chunks) = ~17 FPS LOST on TESTPRO1 (33 vs 50); ring 4 (49
  chunks) = baseline 46.9 FPS.** Don't raise past 4 without re-measuring.
- False alarm avoided: first FPS reading after a fresh-day launch is polluted
  by the "Scanning FPE Files" background pass (status bar shows it) — wait it
  out before judging.

## Remaining resume directions if squares persist at ring 4

1. **Cap the editor camera top speed** to the pace the VT can absorb —
   cheapest, game-side only, no engine risk. (Editor freeflight speed lives in
   GG camera code; possibly scale by zoom distance.) User said they can also
   do this manually.
2. The tile-steal tripwire from the DO-NOT list, then fix what it names.
3. Ring 5 as a compromise if FPS allows on the user's machine (re-measure!).

Related: [[project-wicked-engine-changes]], [[project-terrain-editing]],
[[project-next-action-immediate]].
