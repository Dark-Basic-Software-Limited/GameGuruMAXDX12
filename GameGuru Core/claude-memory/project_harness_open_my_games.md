---
name: project-harness-open-my-games
description: DONE 2026-07-12 — harness can now open a My Games project autonomously via OPEN_PROJECT + CLICK_ONLY_LEVEL. Full TESTPRO1 → island → editor → screenshot sequence verified in ~50s from cold launch.
metadata: 
  node_type: memory
  type: project
  originSessionId: 63b96d44-9243-46fb-bc37-9eb4d4be60b9
---

## Status: DONE 2026-07-12

Three new commands land in `AutomationHarness.cpp`:

- **`LIST_PROJECTS`** — dumps `projectbank_list[]` (populated by the hub renderer at [M-GridEditB_part16.cpp:627](GameGuru Core/GameGuru/Source/M-GridEditB_part16.cpp:627) each frame). Use to discover project names.
- **`OPEN_PROJECT <name>`** — case-insensitive match against `projectbank_list`. Replicates the double-click handler at [M-GridEditB_part16.cpp:1121](GameGuru Core/GameGuru/Source/M-GridEditB_part16.cpp:1121): sets `TriggerLoadGameProject`, flips `bWelcomeScreen_Window`/`bStoryboardWindow`. `process_storeboard()` picks up the trigger and calls `load_storyboard()` on the next frame. Only works from hub state.
- **`CLICK_ONLY_LEVEL`** — walks `Storyboard.Nodes[]` for the first `type=LEVEL` entry with a non-empty `level_name`, clicks it. Convenience for single-level projects like TESTPRO1 — caller doesn't need to know the level's title.

## Why the previous approach failed

`CLICK edit_game` fires `bTriggerEditDemoGame` which reaches [M-GridEditB_part16.cpp:2636](GameGuru Core/GameGuru/Source/M-GridEditB_part16.cpp:2636). That code requires `bProjectExistsAndValidToUse == true` and `iFileListEntry >= 0`, both only set by an in-app mouse click on the project card. On the My Games tab those flags never fire from the flag alone, so `CLICK edit_game` silently no-ops for TESTPRO1.

The new `OPEN_PROJECT` bypasses that entirely by driving the storyboard-load trigger directly with a stable static-buffer name.

## Verified sequence

See [WETEST.md](GameGuru Core/WETEST.md) — "Performance / Baseline Testing: TESTPRO1 Island Level" section. Cold-launch → TESTPRO1 → island → editor → screenshot completes autonomously in ~50 seconds.

## How to apply

Every DX11-vs-DX12 tree/foliage/terrain iteration now runs the full loop without human intervention:

1. Kill MAX
2. Rebuild
3. Launch → `OPEN_PROJECT TESTPRO1` → `CLICK_ONLY_LEVEL` → SCREENSHOT
4. Read screenshot, diff against DX11 baseline
5. Adjust code, loop

See [project-dx11-parity-baseline](project_dx11_parity_baseline.md) for the acceptance criterion.

## Other harness facts (consolidated from MEMORY.md inline sections, 2026-07-17)

- **PRESS_KEY in editor mode**: `GGTerrain_CheckKeys()` is gated by `!bImGuiGotFocus` and never runs in editor mode. The harness fix: `g_autoHarnessInjectedKey` is consumed OUTSIDE that gate and sets `ggterrain_key_pressed[]` directly, cleared next frame via `s_clearInjectedKeyNextFrame`. **LTCG gotcha**: new functions in unity-build part files with ONLY external callers get stripped — use existing functions or globals with internal callers.
- **Profiler commands**: `ENABLE_PROFILER` sets BOTH `bProfilerEnable` AND `wi::profiler::SetEnabled` — editor code disables the profiler every frame otherwise. Overhead ~75% FPS: enable briefly, collect via `GET_PERF_DATA` (includes full `PROFILER_DATA` breakdown), disable immediately.
- **SET_GRASS `<param> <value>`** exists in the harness (~17 params: length/width/stiffness/drag/blades/maxstrands/segments/billboards/viewdist/sss/alpha/tint rgb/sss rgb) but is missing from WETEST.md's command table (repo-doc sync pending — see [[project-review-2026-07-17]]).
- **DEMO levels (verified 2026-07-18):** `LIST_DEMOS` → `SELECT_DEMO <name>` (case-insensitive, e.g. "Island Showdown"; forces the Demo Games tab) → `CLICK edit_game` → storyboard → `CLICK_ONLY_LEVEL` (~40s to editor). Island Showdown facts: 96 animations + 4 lights + 34 hairs → ~27 FPS (the animation-caching/AI backlog items are LIVE here, unlike TESTPRO1); known deltas vs DX11: flat grey sky (DX11 shows dusk gradient + pink horizon), murkier water, cooler flatter tone.
