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

Do NOT use Debug builds for testing. Debug builds output to `Build/Debug/` and the exe will fail with exit code 3 when placed in the runtime directory.

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
| `CLICK` | `play_game` / `edit_game` / `add_level` / `load_level` | Simulate button clicks (context-dependent) |
| `SELECT_DEMO` | `<demo name>` | Select a demo by display name (case-insensitive) |
| `LIST_DEMOS` | (none) | List all available demo games |
| `WAIT` | `<milliseconds>` | Sleep up to 30000ms, then return state |
| `SCREENSHOT` | (none) | Capture screenshot to `auto_screenshot.*` |
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
- **Editor**: PANELS list

## Hub Tab Names (for NAVIGATE hub.\<tab\>)

`demo_games` (0), `my_games` (1), `tutorials` (3), `user_guide` (4), `live_changelog` (5), `workshop_uploader` (6), `workshop` (7), `community_tutorials` (42)

## CLICK Context Rules

- `edit_game` from **hub**: triggers `bTriggerEditDemoGame` flag (opens storyboard for selected demo)
- `edit_game` from **storyboard**: triggers 'E' key (loads selected level into editor)
- `play_game` from **storyboard**: triggers space key (TEST GAME)
- `add_level` from **storyboard**: triggers 'N' key
- `load_level` from **storyboard**: triggers 'L' key

## Standard Sequence: Hub -> Demo -> Storyboard (EDIT GAME)

This is the proven sequence. Use it as the starting point for any test scenario.

```bash
EXE_DIR="D:/DEV/BUILD/GameGuru Wicked MAX Build Area/Max"
CMD="$EXE_DIR/auto_command.txt"
RSP="$EXE_DIR/auto_result.txt"

# Helper: send command and wait for response
send_cmd() {
    rm -f "$RSP"
    echo "$1" > "$CMD"
    local t=0
    while [ $t -lt ${2:-30} ]; do
        [ -f "$RSP" ] && { cat "$RSP"; return 0; }
        sleep 1; t=$((t+1))
    done
    echo "TIMEOUT"; return 1
}

# Launch the app
cd "$EXE_DIR"
./GameGuruMAX.exe &
sleep 30

# Confirm hub is ready
send_cmd "GET_STATE" 30
# Expected: STATE: hub / TAB: demo_games

# Navigate to demo games tab
sleep 2
send_cmd "NAVIGATE hub.demo_games" 15

# Select the demo (e.g. Switch Escape)
sleep 2
send_cmd "SELECT_DEMO Switch Escape" 15

# Click Edit Game (hub -> storyboard)
sleep 2
send_cmd "CLICK edit_game" 120

# Wait for storyboard to appear
sleep 30
send_cmd "GET_STATE" 30
# Expected: STATE: storyboard / PROJECT: Switch Escape / NODES(14): ...
```

**At this point you are at the storyboard with the selected demo project loaded.** Continue from here with additional commands (e.g. `CLICK edit_game` to load a level into the editor, or `CLICK play_game` to TEST GAME).

## Process Monitoring

Check if the app is still alive (for crash detection):

```bash
tasklist.exe 2>/dev/null | grep -qi "GameGuruMAX"
```

## Notes

- The app takes ~20-30 seconds to initialize and reach the hub
- `CLICK edit_game` from hub is synchronous — the harness blocks during load (~25-30s for most demos)
- The harness response confirms the command was accepted, not that the resulting operation completed — always follow up with `GET_STATE` after waits
- All 19 demos were successfully tested through this sequence on 2026-02-14
- `GET_SCREEN_TEXT` provides full widget/button labels for every storyboard node — use this to verify screen content without screenshots
