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

## Building WickedEngine LIB from Claude Code

### Shell and CMD issues

1. **Never try to call `VsDevCmd.bat` directly from bash** — it doesn't work. Always use `cmd //C` to wrap Windows batch commands.

2. **Never use `call` inside a bash command** — `call` is a CMD built-in and won't work in the Claude Code shell. Instead, write a `.bat` file and execute that via `cmd //C`.

3. **Always use absolute paths for `.sln` files in msbuild** — relative paths fail because the working directory context is unreliable when invoking through `cmd //C`. Use the full path like `msbuild "D:\max\WickedEngineDX12\WickedEngine.sln"`.

4. **Always `cd /D` to the project directory inside the batch file** — don't rely on the bash `cd` command carrying over into the CMD context. Put `cd /D "D:\max\WickedEngineDX12"` as the first line after `@echo off`.

5. **The correct working build command is:**

```bash
cmd //C "D:\\max\\WickedEngineDX12\\build_wicked.bat" 2>&1 | tail -20
```

Where `build_wicked.bat` contains:

```batch
@echo off
cd /D "D:\max\WickedEngineDX12"
call "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat" -arch=amd64 >nul 2>&1
msbuild "D:\max\WickedEngineDX12\WickedEngine.sln" /p:Configuration=Release /p:Platform=x64 /t:WickedEngine_Windows /m /verbosity:minimal
```

6. **Use `| tail -20` on build commands** to avoid flooding the context window with thousands of lines of compiler output. If the build fails, use `| tail -40` or grep for "error" to find the issue.

### Build order and verification

7. **WickedEngine must be built BEFORE GameGuru MAX** — the GameGuru build depends on the WickedEngine `.lib` output. Always build WickedEngine first, confirm it succeeded, then run the GameGuru `build.bat`.

8. **Always verify new symbols are in the `.lib` after rebuilding WickedEngine.** A successful build does NOT guarantee your new functions are linked in. The `.lib` can be stale or the functions can be optimized away by LTCG. After building, run dumpbin to confirm:

```bash
"C:/Program Files/Microsoft Visual Studio/18/Community/VC/Tools/MSVC/14.50.35717/bin/Hostx64/x64/dumpbin.exe" /linkermember:1 "D:/max/WickedEngineDX12/BUILD/x64/Release/WickedEngine_Windows.lib" | grep YourFunctionName
```

Use `/linkermember:1` not `/symbols` or `/exports` — those return empty for static `.lib` files.

9. **The WickedEngine `.lib` output path is `D:\max\WickedEngineDX12\BUILD\x64\Release\WickedEngine_Windows.lib`** — don't waste time searching in `x64/Release/` or `WickedEngine/x64/Release/`, they don't exist. The output directory is set in the `.vcxproj` as `$(SolutionDir)BUILD\$(Platform)\$(Configuration)\`.

10. **Source files are not listed directly in `WickedEngine_Windows.vcxproj`** — they come from `WickedEngine_SOURCE.vcxitems` (imported at line 40 of the `.vcxproj`). If you need to check whether a `.cpp` file is included in the build, search the `.vcxitems` file, not the `.vcxproj`.

11. **If you add new functions to WickedEngine headers but get linker errors in GameGuru**, the most likely cause is a stale `.lib`. Do a clean rebuild of WickedEngine and verify symbols with dumpbin before investigating anything else.

## Full Demo FPS Test (FULL TEST)

Automated test that launches all 19 demos, enters test-game mode, collects FPS samples, and reports results. Tested 2026-02-15: 18/19 passed autonomously in ~15 minutes.

### How to Run

1. **Ensure the app is NOT running** — `taskkill.exe //IM GameGuruMAX.exe //F`
2. **Launch the app** and wait for hub state
3. **Write the test script** to a `.sh` file (see below) and run it as a **background task** with a 600000ms (10 min) timeout
4. **Check progress** every 2-4 minutes by tailing the background output file
5. **Kill the app** after the script completes

### MSYS Bash Compatibility — CRITICAL

- **Do NOT use `${!array[@]}`** — MSYS bash does not support it. Use a `while read` loop with a heredoc instead
- **Do NOT use bash arrays for the demo list** — use a heredoc `DEMOLIST` block fed into `while IFS= read -r demo`
- **Use `for s in 1 2 3 4 5`** instead of `for s in $(seq 1 5)` if seq is unavailable
- **Always use a short variable** `D="D:/DEV/BUILD/GameGuru Wicked MAX Build Area/Max"` at the top — do NOT use `$EXE_DIR` or functions that reference it

### The Complete Test Script

Write this to a `.sh` file and execute with `bash <file>`. Run as a background task.

```bash
#!/bin/bash
D="D:/DEV/BUILD/GameGuru Wicked MAX Build Area/Max"
RESULTS_FILE="/tmp/fps_results.txt"

send_cmd() {
  rm -f "$D/auto_result.txt"
  echo "$1" > "$D/auto_command.txt"
  local t=0
  local timeout=${2:-30}
  while [ $t -lt $timeout ]; do
    if [ -f "$D/auto_result.txt" ]; then
      cat "$D/auto_result.txt"
      return 0
    fi
    sleep 1
    t=$((t+1))
  done
  echo "TIMEOUT"
  return 1
}

echo "DEMO | BEST_FPS | AVG_FPS | SAMPLES" > "$RESULTS_FILE"
echo "--- | --- | --- | ---" >> "$RESULTS_FILE"

TOTAL=19
PASSED=0
FAILED=0
FAILED_NAMES=""
num=0

while IFS= read -r demo; do
  num=$((num+1))
  echo ""
  echo "============================================"
  echo "[$num/$TOTAL] Testing: $demo"
  echo "============================================"

  # Step 1: Navigate to hub
  echo "  -> Navigating to hub..."
  result=$(send_cmd "NAVIGATE hub" 15)
  sleep 2

  # Step 2: Confirm hub state
  result=$(send_cmd "GET_STATE" 15)
  state=$(echo "$result" | grep "^STATE:" | head -1 | sed 's/STATE: //')
  echo "  -> State: $state"
  if [ "$state" != "hub" ]; then
    echo "  !! FAILED - not in hub state, got: $state"
    FAILED=$((FAILED+1))
    FAILED_NAMES="$FAILED_NAMES|$demo (not in hub)"
    echo "$demo | FAIL | - | not in hub" >> "$RESULTS_FILE"
    continue
  fi

  # Step 3: Select demo
  echo "  -> Selecting demo..."
  result=$(send_cmd "SELECT_DEMO $demo" 15)
  echo "  -> $result"
  sleep 2

  # Step 4: Edit game (hub -> storyboard)
  echo "  -> Clicking edit_game..."
  result=$(send_cmd "CLICK edit_game" 15)
  sleep 5

  # Step 5: Confirm storyboard and find level node
  result=$(send_cmd "GET_STATE" 30)
  state=$(echo "$result" | grep "^STATE:" | head -1 | sed 's/STATE: //')
  echo "  -> State: $state"
  if [ "$state" != "storyboard" ]; then
    echo "  !! FAILED - not in storyboard, got: $state"
    FAILED=$((FAILED+1))
    FAILED_NAMES="$FAILED_NAMES|$demo (not in storyboard)"
    echo "$demo | FAIL | - | not in storyboard" >> "$RESULTS_FILE"
    continue
  fi

  # Step 6: Extract level node title (must have type=level AND level="mapbank...)
  level_node=$(echo "$result" | grep 'type=level' | grep 'level="mapbank' | head -1 | sed 's/.*title="\([^"]*\)".*/\1/')
  echo "  -> Level node: '$level_node'"
  if [ -z "$level_node" ]; then
    echo "  !! FAILED - no level node with mapbank found"
    FAILED=$((FAILED+1))
    FAILED_NAMES="$FAILED_NAMES|$demo (no level node)"
    echo "$demo | FAIL | - | no level node" >> "$RESULTS_FILE"
    continue
  fi

  # Step 7: Click level node to load into editor
  echo "  -> Loading level '$level_node'..."
  result=$(send_cmd "CLICK_NODE $level_node" 15)
  echo "  -> $result"

  # Step 8: Wait for level load + confirm editor (long timeout for big levels)
  sleep 12
  result=$(send_cmd "GET_STATE" 60)
  state=$(echo "$result" | grep "^STATE:" | head -1 | sed 's/STATE: //')
  echo "  -> State after load: $state"

  # If still loading, wait more (some levels like Aztec Game Kit take 20s+)
  if [ "$state" = "loading" ] || [ "$state" = "storyboard" ]; then
    echo "  -> Still loading, waiting more..."
    sleep 15
    result=$(send_cmd "GET_STATE" 60)
    state=$(echo "$result" | grep "^STATE:" | head -1 | sed 's/STATE: //')
    echo "  -> State: $state"
  fi

  if [ "$state" != "editor" ]; then
    echo "  !! FAILED - not in editor, got: $state"
    FAILED=$((FAILED+1))
    FAILED_NAMES="$FAILED_NAMES|$demo (not in editor, got $state)"
    echo "$demo | FAIL | - | not in editor ($state)" >> "$RESULTS_FILE"
    continue
  fi

  # Step 9: Test level (enter game mode)
  echo "  -> Testing level..."
  result=$(send_cmd "CLICK test_level" 15)
  sleep 8

  # Step 10: Confirm game state
  result=$(send_cmd "GET_STATE" 30)
  state=$(echo "$result" | grep "^STATE:" | head -1 | sed 's/STATE: //')
  echo "  -> State: $state"

  if [ "$state" != "game" ]; then
    sleep 5
    result=$(send_cmd "GET_STATE" 30)
    state=$(echo "$result" | grep "^STATE:" | head -1 | sed 's/STATE: //')
    echo "  -> State (retry): $state"
  fi

  if [ "$state" != "game" ]; then
    echo "  !! FAILED - not in game state, got: $state"
    FAILED=$((FAILED+1))
    FAILED_NAMES="$FAILED_NAMES|$demo (not in game, got $state)"
    echo "$demo | FAIL | - | not in game ($state)" >> "$RESULTS_FILE"
    send_cmd "PRESS_ESCAPE" 10 > /dev/null 2>&1
    sleep 2
    continue
  fi

  # Step 11: Collect FPS (5 samples over ~15 seconds)
  echo "  -> Collecting FPS..."
  best_fps="0"
  sample_count=0
  all_fps=""

  for s in 1 2 3 4 5; do
    sleep 3
    result=$(send_cmd "GET_PERF_DATA" 10)
    fps_val=$(echo "$result" | grep "^FPS:" | sed 's/FPS: //')
    if [ -n "$fps_val" ]; then
      echo "    Sample $s: $fps_val FPS"
      all_fps="$all_fps $fps_val"
      sample_count=$((sample_count+1))

      # Compare for best (remove decimal point for integer comparison)
      fps_x10=$(echo "$fps_val" | tr -d '.')
      best_x10=$(echo "$best_fps" | tr -d '.')
      fps_x10=${fps_x10##0}
      best_x10=${best_x10##0}
      if [ "${fps_x10:-0}" -gt "${best_x10:-0}" ] 2>/dev/null; then
        best_fps="$fps_val"
      fi
    fi
  done

  # Calculate average
  if [ $sample_count -gt 0 ]; then
    avg_fps=$(echo "$all_fps" | tr ' ' '\n' | grep -v '^$' | awk '{sum+=$1; n++} END {printf "%.1f", sum/n}')
  else
    avg_fps="N/A"
  fi

  echo "  ==> Best: $best_fps | Avg: $avg_fps | Samples: $sample_count"
  echo "$demo | $best_fps | $avg_fps | $sample_count" >> "$RESULTS_FILE"
  PASSED=$((PASSED+1))

  # Step 12: Exit game back to editor
  echo "  -> Pressing escape..."
  result=$(send_cmd "PRESS_ESCAPE" 10)
  sleep 3

  # Confirm recovery
  result=$(send_cmd "GET_STATE" 15)
  state=$(echo "$result" | grep "^STATE:" | head -1 | sed 's/STATE: //')
  echo "  -> Back to: $state"

done <<'DEMOLIST'
Aztec Game Kit Teaser
Aztec Game Kit
Bounty
Horseshoe Bend
Island Showdown
Operation Amazon
River Raiders
Snowy Mountain Stroll
A Grand Canyon Adventure
Disruption
Foggy Forest
Indian Strike Force
Switch Escape
Canyon Offensive
Escape from the Zombie Cellar
Jungle Fever
RPG Template
The Mystery of Z Island
Trapped
DEMOLIST

echo ""
echo "============================================"
echo "ALL TESTS COMPLETE"
echo "Passed: $PASSED / $TOTAL"
echo "Failed: $FAILED / $TOTAL"
if [ -n "$FAILED_NAMES" ]; then
  echo "Failed demos:"
  echo "$FAILED_NAMES" | tr '|' '\n' | grep -v '^$' | while read -r line; do echo "  - $line"; done
fi
echo "============================================"
echo ""
echo "=== RESULTS TABLE ==="
cat "$RESULTS_FILE"
```

### Execution Pattern

```bash
# 1. Kill any running instance
taskkill.exe //IM GameGuruMAX.exe //F

# 2. Launch fresh
D="D:/DEV/BUILD/GameGuru Wicked MAX Build Area/Max"
cd "$D" && ./GameGuruMAX.exe &

# 3. Wait for hub
sleep 5
rm -f "$D/auto_result.txt"; echo "GET_STATE" > "$D/auto_command.txt"
t=0; while [ $t -lt 30 ]; do [ -f "$D/auto_result.txt" ] && { cat "$D/auto_result.txt"; break; }; sleep 1; t=$((t+1)); done

# 4. Write the script to a .sh file (Write tool), then run as background task:
bash "$D/run_all_tests.sh"
# Use run_in_background=true, timeout=600000

# 5. Check progress every 2-4 minutes by tailing the background output file

# 6. After completion, kill the app
taskkill.exe //IM GameGuruMAX.exe //F
```

### Per-Demo Flow (what the script does for each demo)

```
NAVIGATE hub -> GET_STATE (confirm hub) -> SELECT_DEMO <name> -> CLICK edit_game
-> GET_STATE (confirm storyboard, extract level node title) -> CLICK_NODE <title>
-> sleep 12 + GET_STATE (confirm editor, retry with +15s if still loading)
-> CLICK test_level -> GET_STATE (confirm game) -> 5x GET_PERF_DATA (3s apart)
-> PRESS_ESCAPE -> GET_STATE (confirm editor recovery) -> next demo
```

### Known Behaviours

| Behaviour | Details |
|-----------|---------|
| **First FPS sample is often low** | Shader compilation / GPU warmup on the first frame. Sample 1 may read 3-4 FPS. The best/avg calculation handles this — report best FPS as the headline number |
| **Island Showdown may timeout** | This is the largest level. The 12s sleep + 60s poll may not be enough. If it fails, increase the initial sleep to 20s or add a third retry with another 15s wait |
| **Zombie Cellar locks at 60 FPS** | This demo has vsync enabled in its settings. 60.0 FPS is correct, not a bug |
| **PRESS_ESCAPE sometimes returns to `loading`** | Transitional state. The next `NAVIGATE hub` command at the top of the loop recovers from this |
| **Level node title != demo name** | The script extracts the level node title from `GET_STATE` storyboard output (grep for `type=level` with `level="mapbank`). This handles cases like "Escape from the Zombie Cellar" whose level node is titled "zombie cellar demo - level1" |
| **Some levels need extra load time** | Aztec Game Kit, Horseshoe Bend, Indian Strike Force, etc. take 15-25s to load. The script retries with +15s if state is still `storyboard` or `loading` after the first check |

### Baseline Results (2026-02-15, AMD Radeon RX 9060 XT)

| # | Demo | Best FPS | Avg FPS |
|---|------|----------|---------|
| 1 | Aztec Game Kit Teaser | 137.3 | 135.1 |
| 2 | Aztec Game Kit | 77.1 | 61.8 |
| 3 | Bounty | 147.5 | 145.9 |
| 4 | Horseshoe Bend | 22.3 | 9.4 |
| 5 | Island Showdown | FAIL | level load timeout |
| 6 | Operation Amazon | 83.1 | 81.8 |
| 7 | River Raiders | 90.0 | 88.6 |
| 8 | Snowy Mountain Stroll | 38.4 | 17.3 |
| 9 | A Grand Canyon Adventure | 84.1 | 83.4 |
| 10 | Disruption | 109.3 | 108.3 |
| 11 | Foggy Forest | 67.2 | 66.7 |
| 12 | Indian Strike Force | 46.8 | 46.5 |
| 13 | Switch Escape | 197.4 | 195.3 |
| 14 | Canyon Offensive | 59.8 | 47.8 |
| 15 | Escape from the Zombie Cellar | 60.0 | 59.9 |
| 16 | Jungle Fever | 142.3 | 139.6 |
| 17 | RPG Template | 163.6 | 161.0 |
| 18 | The Mystery of Z Island | 75.1 | 73.7 |
| 19 | Trapped | 228.3 | 225.1 |

## Notes

- The harness response confirms the command was accepted, not that the resulting operation completed — always follow up with `GET_STATE` after waits
- All 19 demos were successfully tested through the hub->storyboard sequence on 2026-02-14
- 18/19 demos passed the full FPS test (hub->storyboard->editor->game->FPS->escape) on 2026-02-15
- `GET_SCREEN_TEXT` provides full widget/button labels for every storyboard node — use this to verify screen content without screenshots
- When polling after a level load, use a longer timeout (60s) on the `GET_STATE` poll to account for the synchronous load
