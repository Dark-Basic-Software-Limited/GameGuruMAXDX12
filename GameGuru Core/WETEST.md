# Automation Test Harness - Quick Start

## Overview

GameGuruMAX has a built-in file-based automation harness (`AutomationHarness.cpp`) that accepts commands via text files. It runs in the main loop, checking for commands every tick with zero overhead when idle.

## Key Paths

| Item | Path |
|------|------|
| **EXE directory (runtime root)** | `D:\DEV\BUILD\GameGuru Wicked MAX Build Area\Max` |
| **Executable** | `GameGuruMAX.exe` (in the EXE directory) |
| **Command file** (write commands here) | `auto_command.txt` (in EXE directory) |
| **Response file** (read results here) | `auto_result.txt` (in EXE directory) |
| **Log file** (append-only timestamped log) | `auto_log.txt` (in EXE directory) |
| **Source code** | `D:\max\GameGuruMAXDX12\GameGuru Core\Guru-WickedMAX\AutomationHarness.cpp` |
| **Header** | `D:\max\GameGuruMAXDX12\GameGuru Core\Guru-WickedMAX\AutomationHarness.h` |

## Building

Always build **Release** — the Release configuration outputs directly to the runtime EXE directory, no manual copy needed:

```bash
"D:/max/GameGuruMAXDX12/GameGuru Core/build.bat" Release
```

**IMPORTANT**: The app must NOT be running when you build — the linker cannot overwrite the exe while it's locked. Quit the app first (via `QUIT` command or force kill), then build.

Do NOT use Debug builds for testing. Debug builds output to `Build/Debug/` and the exe will fail with exit code 3 when placed in the runtime directory.

## Quitting / Force Killing the App

```bash
# Force kill is the most reliable way to quit (note: //IM with double slashes for MSYS bash)
taskkill.exe //IM GameGuruMAX.exe //F

# Check if running
tasklist.exe 2>/dev/null | grep -qi "GameGuruMAX" && echo "RUNNING" || echo "NOT RUNNING"
```

**Note**: Prefer `taskkill` over the `QUIT` harness command. The `QUIT` command can fail to be consumed if the app is in a state where the harness isn't polling (e.g., during level loads). `taskkill` always works.

## Protocol

1. Write a single command line to `auto_command.txt`
2. The app reads it, deletes the file, and writes the result to `auto_result.txt`
3. Poll for `auto_result.txt` to appear (it is overwritten each time)
4. All commands and results are also appended to `auto_log.txt` with timestamps

**Important**: If the command file is empty, the harness leaves it alone (handles writer flush race condition). Only non-empty files are consumed and deleted.

## Available Commands

| Command | Arguments | Description |
|---------|-----------|-------------|
| `GET_STATE` | (none) | Returns STATE, TAB, VISIBLE_PANELS, ERRORS, UPTIME. In storyboard state also returns PROJECT name and NODES summary |
| `GET_SCREEN_TEXT` | (none) | Detailed dump of all on-screen content: project info, node details, widget labels, connections |
| `NAVIGATE` | `hub` / `hub.<tab>` / `storyboard` | Switch between hub, hub tabs, or storyboard |
| `CLICK` | `play_game` / `edit_game` / `test_level` / `add_level` / `load_level` | Simulate button clicks (context-dependent) |
| `CLICK_NODE` | `<node title>` | Click a storyboard node by title (case-insensitive). Level nodes load into editor |
| `SELECT_DEMO` | `<demo name>` | Select a demo by display name (case-insensitive) |
| `LIST_DEMOS` | (none) | List all available demo games |
| `WAIT` | `<milliseconds>` | Sleep up to 30000ms, then return state |
| `SCREENSHOT` | (none) | Capture screenshot to `auto_screenshot.*` |
| `GET_PERF_DATA` | (none) | Returns FPS, frame time, memory, VRAM, GPU adapter, scene counts, and visibility counts. Works in any state |
| `TOGGLE_PROFILER` | (none) | Cycles the in-game TAB mode (0=normal, 1=visuals panel, 2=performance panel). Game state only |
| `PRESS_ESCAPE` | (none) | Exits test game back to editor (sets gameloop/levelloop/masterloop=0). Game state only |
| `QUIT` | (none) | Gracefully close the application |

## Application States

`GET_STATE` returns one of: `initializing`, `hub`, `storyboard`, `editor`, `game`, `loading`

## GET_STATE Storyboard Output

When in storyboard state, `GET_STATE` appends:
```
PROJECT: Switch Escape
NODES(14):
  [0] type=splash title="Splash Screen" level="" out0=" Connect to Scene "->50001
  [7] type=level title="switch escape" level="mapbank\switch escape.fpm" out0="..."->50005
  ...
```

## GET_SCREEN_TEXT Output

Returns detailed info depending on current state:
- **Storyboard**: PROJECT, DESCRIPTION, READONLY, and per-node details including title, type, level, levelnumber, editable flag, all output/input pins with actions and link targets, and all widget labels with types
- **Hub**: TAB name and demo list (when on demo_games tab)
- **Editor**: PANELS list, TOOLBAR_BUTTONS with toggle states, MENU_BAR items

### Editor Output Example

```
STATE: editor
PANELS: DockSpaceAGK, ..., Object Tools##EntityToolsWindow, ..., Toolbar, Statusbar
TOOLBAR_BUTTONS:
  Back to Game Project Storyboard
  Save Level
  Test Level
  Terrain, Painting, Trees and Vegetation [active=0]
  Object Tools [active=1]
  Visual Logic Connections [active=0]
  Environment Effects [active=0]
  Game Settings [active=0]
  Editor Light [active=0]
  Camera View
MENU_BAR: File, Edit, Tools, Help
```

Toggle buttons show `[active=0/1]` to indicate their current state. Action-only buttons (Back, Save, Test Level, Camera View) have no toggle state. "Test Level in VR" only appears when VR developer mode is enabled. "Terrain" only appears when not in empty level mode.

## Hub Tab Names (for NAVIGATE hub.\<tab\>)

`demo_games` (0), `my_games` (1), `tutorials` (3), `user_guide` (4), `live_changelog` (5), `workshop_uploader` (6), `workshop` (7), `community_tutorials` (42)

## CLICK Context Rules

- `edit_game` from **hub** only: triggers `bTriggerEditDemoGame` flag (opens storyboard for selected demo). Does NOT work from storyboard — use `CLICK_NODE` to load a level from the storyboard
- `test_level` from **editor** only: triggers `iLaunchAfterSync = 1` (runs the level as a playable test game). `GET_STATE` returns `STATE: game` while running. Harness remains responsive during game
- `play_game` from **storyboard**: triggers space key (TEST GAME)
- `add_level` from **storyboard**: triggers 'N' key
- `load_level` from **storyboard**: triggers 'L' key

## GET_PERF_DATA Output

Returns performance metrics. Works in any state. Example output during game:

```
STATE: game
FPS: 236.6
FRAME_TIME_MS: 4.23
SYSTEM_MEM_MB: 5793456
SYSTEM_MEM_GB: 5657.67
VRAM_MB: 37.3
VRAM_GB: 0.04
GPU_ADAPTER: AMD Radeon RX 9060 XT
SCENE_OBJECTS: 734
SCENE_MESHES: 586
SCENE_MATERIALS: 587
SCENE_LIGHTS: 22
SCENE_TRANSFORMS: 1767
SCENE_CAMERAS: 0
SCENE_EMITTERS: 0
SCENE_HAIRS: 0
SCENE_ANIMATIONS: 13
SCENE_ARMATURES: 34
SCENE_DECALS: 0
SCENE_PROBES: 9
SCENE_SOUNDS: 0
SCENE_COLLIDERS: 0
SCENE_RIGIDBODIES: 0
SCENE_SOFTBODIES: 0
SCENE_SCRIPTS: 0
SCENE_WEATHERS: 1
VISIBLE_OBJECTS: 330
VISIBLE_LIGHTS: 6
VISIBLE_DECALS: 0
VISIBLE_ENVPROBES: 4
VISIBLE_EMITTERS: 0
VISIBLE_HAIRS: 0
TAB_MODE: 0
```

**Note**: SYSTEM_MEM_MB is process working set from Windows, not free RAM. VRAM_MB is dedicated GPU memory usage from DXGI. VISIBLE_* counts change per frame based on camera frustum culling.

## TOGGLE_PROFILER (Game State)

Cycles `g.tabmode` (0→1→2→0), mirroring the TAB key in test game mode:
- **tabmode=0**: Normal game view
- **tabmode=1**: Visuals panel (right side — Customize Sky, Water, Weather, etc.)
- **tabmode=2**: Performance panel (top-left — FPS, draw calls, triangles) + visuals panel

**Important**: Does NOT call `wi::profiler::DrawData()` — that causes GPU TDR crashes. The performance panel uses safe ImGui text rendering only.

## PRESS_ESCAPE (Game State)

Exits test game back to editor by setting `t.game.gameloop=0`, `t.game.levelloop=0`, `t.game.masterloop=0`. This is the same exit path as pressing ESC during a test game (M-Game_part1.cpp line 1523).

## CLICK_NODE (Storyboard)

Clicks a storyboard node by title to trigger its action. For **level** nodes with a level file assigned, this loads the level into the editor via `cDirectOpen` + `iLaunchAfterSync = 7` (same as clicking the node thumbnail in the UI). The harness is unresponsive during the synchronous level load. After load completes, `GET_STATE` returns `STATE: editor`.

## Standard Sequence: Launch -> Hub -> Storyboard -> Editor

This is the proven sequence (tested 2026-02-15, ~34s launch to editor).

Use inline paths — do NOT rely on bash helper functions with `$EXE_DIR` expansion as they have scoping issues in MSYS bash. Use a short variable `D` set at the top of each script block.

```bash
D="D:/DEV/BUILD/GameGuru Wicked MAX Build Area/Max"

# Launch the app (background)
cd "$D" && ./GameGuruMAX.exe &

# Wait briefly then poll for hub (hub appears in ~5-10s)
sleep 3
rm -f "$D/auto_result.txt"; echo "GET_STATE" > "$D/auto_command.txt"
t=0; while [ $t -lt 30 ]; do [ -f "$D/auto_result.txt" ] && { cat "$D/auto_result.txt"; break; }; sleep 1; t=$((t+1)); done
# Expected: STATE: hub / TAB: demo_games

# Select demo
sleep 2
rm -f "$D/auto_result.txt"; echo "SELECT_DEMO Switch Escape" > "$D/auto_command.txt"
t=0; while [ $t -lt 15 ]; do [ -f "$D/auto_result.txt" ] && { cat "$D/auto_result.txt"; break; }; sleep 1; t=$((t+1)); done

# Edit Game (hub -> storyboard)
sleep 2
rm -f "$D/auto_result.txt"; echo "CLICK edit_game" > "$D/auto_command.txt"
t=0; while [ $t -lt 30 ]; do [ -f "$D/auto_result.txt" ] && { cat "$D/auto_result.txt"; break; }; sleep 1; t=$((t+1)); done

# Confirm storyboard
sleep 5
rm -f "$D/auto_result.txt"; echo "GET_STATE" > "$D/auto_command.txt"
t=0; while [ $t -lt 30 ]; do [ -f "$D/auto_result.txt" ] && { cat "$D/auto_result.txt"; break; }; sleep 1; t=$((t+1)); done
# Expected: STATE: storyboard / PROJECT: Switch Escape / NODES(14): ...

# Click level node to load into editor
rm -f "$D/auto_result.txt"; echo "CLICK_NODE switch escape" > "$D/auto_command.txt"
t=0; while [ $t -lt 15 ]; do [ -f "$D/auto_result.txt" ] && { cat "$D/auto_result.txt"; break; }; sleep 1; t=$((t+1)); done

# Wait for level load then confirm editor
sleep 10
rm -f "$D/auto_result.txt"; echo "GET_STATE" > "$D/auto_command.txt"
t=0; while [ $t -lt 60 ]; do [ -f "$D/auto_result.txt" ] && { cat "$D/auto_result.txt"; break; }; sleep 1; t=$((t+1)); done
# Expected: STATE: editor
```

## Timing Guide

| Phase | Sleep | Notes |
|-------|-------|-------|
| After launch | 3s | Hub appears in ~5-10s, the poll loop handles the rest |
| Between hub commands | 2s | Small gap for UI to settle |
| After CLICK edit_game (hub->storyboard) | 5s | Storyboard loads quickly |
| After CLICK_NODE (level load) | 10s | Synchronous load, harness unresponsive during load |

**Tested**: Launch to editor in ~34 seconds total (2026-02-15).

## Crash Diagnosis Using Log Files

When the app crashes or triggers a GPU device reset (TDR), several log files in the EXE directory provide diagnostic information. Cross-reference these with the source code to identify the root cause.

### Log Files

| File | Description |
|------|-------------|
| `Guru-Crash.log` | **Primary crash log.** Append-only, survives across sessions. Records exception code, crash address, and **source file + line number** for each crash. Check this first |
| `crashdump.dmp` | Windows minidump for debugger analysis. Overwritten each crash |
| `auto_log.txt` | Automation harness log with timestamps. Useful for correlating crash timing with commands issued |

### Reading Guru-Crash.log

Each entry looks like:
```
==== GAMEGURU MAX CRASH DETECTED ====
Time:            2026-02-15 17:55:18
Exception code:  0xc0000005
Source Code:     D:\max\WickedEngineDX12\WickedEngine\wiProfiler.cpp:498
=====================================
```

- **Exception 0xc0000005** = access violation (read/write to invalid memory). Most common crash type
- **Source Code line** points to the exact C++ line. Read that line in context to understand what went wrong
- Multiple entries with the **same source file:line** across sessions indicate a reproducible bug, not a random glitch
- Multiple **different crash locations** in the same session often indicate a cascade — the first crash corrupts state, causing subsequent crashes elsewhere

### Common Crash Patterns

**GPU Allocator failures (`AllocateGPU` → `memcpy` crash)**:
Crashes at `std::memcpy` into `allocation.data` in files like `wiProfiler.cpp`, `wiGraphicsDevice.h`, or `wiRenderer.cpp` mean `AllocateGPU()` returned an allocation with a null `data` pointer. The GPU frame allocator failed to provide mapped memory. This typically happens when:
- A GPU resource is created or used in an invalid context (e.g., inside an active RenderPass)
- The GPU device has already been removed (TDR occurred, and subsequent operations crash)
- A feature was enabled mid-frame that requires GPU resources not yet initialized

**TDR / Hard GPU freeze**:
When the PC resets or the display driver recovers, the crash log may show the *symptom* rather than the *cause*. The access violation from a failed GPU allocation corrupts the DX12 command list (render pass started but never ended, command list never submitted), which hangs the GPU. Look for what was **enabled or changed** just before the first crash in the log, not just the crash location itself.

### Diagnosis Workflow

1. **Read `Guru-Crash.log`** — identify the crash source file and line number
2. **Read the source code** at that line — understand what operation failed (null pointer, bad allocation, etc.)
3. **Look for patterns** — if the same line crashes repeatedly, it's a reliable reproduction. If multiple different lines crash, find the earliest/most frequent one as the root cause
4. **Trace the call chain backwards** — from the crash site, trace what function called it, what enabled the feature, and what user action triggered it
5. **Check the frame lifecycle** — WickedEngine frame order is: `BeginFrame` → `Update` (game logic) → `Render` (scene) → `Compose` (overlays, ImGui) → `EndFrame`. Features enabled during Update take effect in the same or next frame's Compose

### Key Architecture Notes for Crash Tracing

- **WickedEngine frame loop** (`wiApplication.cpp`): `profiler::BeginFrame()` → `Update()` → `Render()` → `RenderPassBegin` → `Compose(cmd)` → `RenderPassEnd` → `profiler::EndFrame(cmd)` → `SubmitCommandLists`
- **MasterRenderer::Compose** (`master_part1.cpp`): calls `__super::Compose(cmd)` (which chains through `Application::Compose` where `wi::profiler::DrawData` runs), then `ImGui_DX12_RenderBridge(cmd)`
- **Game logic runs during Update**: `GuruLoopLogic()` → `common_loop_logic()` → `mapeditorexecutable_loop()` or `gameexecutable_loop()`. The `iLaunchAfterSync` state machine in `M-GridEdit_part1.cpp` controls app flow (states: 0=idle, 7=load requested, 502=loading, 80=post-load, 1=init, 201/202=test game)
- **ImGui frame lifecycle**: `ImGui::NewFrame()` is called during Update, `ImGui::Render()` during `ImGui_RenderLast()`, and the actual DX12 draw happens in `ImGui_DX12_RenderBridge()` during Compose
- **wiProfiler**: When enabled via `SetEnabled(true)`, initialization happens on the *next* frame's `BeginFrame()`. `DrawData()` runs during Compose inside an active RenderPass. Enabling mid-game can crash if the profiler's GPU allocations fail in that context

## Notes

- The harness response confirms the command was accepted, not that the resulting operation completed — always follow up with `GET_STATE` after waits
- All 19 demos were successfully tested through the hub->storyboard sequence on 2026-02-14
- `GET_SCREEN_TEXT` provides full widget/button labels for every storyboard node — use this to verify screen content without screenshots
- When polling after a level load, use a longer timeout (60s) on the `GET_STATE` poll to account for the synchronous load
