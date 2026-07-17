# Automation Map - UI State Flags and Control Variables

> Design-time internals notes (Feb 2026) for extending the harness — line numbers have drifted; see WETEST.md for the current, verified command surface.

## App State Machine

The app has a clear state hierarchy:

1. **Initialization** (`GuruLoopLogic` in `GameGuruMain.cpp:112`)
   - `g_iInitializationSequence` (int, `GameGuruMain.cpp:24`): 0-3 during startup
   - `g_bNoGGUntilGameGuruMainCalled` (bool, `GameGuruMain.cpp:25`): false during init, true when ready
   - Once init is done, `common_loop_logic()` runs every tick (`Common_part0.cpp:773`)

2. **Editor vs Standalone**: `t.game.gameisexe` (int)
   - 0 = editor mode (calls `mapeditorexecutable_loop()`)
   - 1 = standalone game (calls `gameexecutable_loop()`)

3. **Within Editor** - three main states controlled by booleans:
   - **Hub/Welcome Screen**: `bWelcomeScreen_Window` = true
   - **Storyboard/Project**: `bStoryboardWindow` = true
   - **Editor (level editing)**: both false, editing a level
   - **Test Game (F9)**: `bImGuiInTestGame` = true

---

## Hub / Welcome Screen

### Function
`Welcome_Screen()` in `M-GridEditB_part16.cpp:1`

### Key Variables

| Variable | Type | Defined In | Purpose |
|----------|------|-----------|---------|
| `bWelcomeScreen_Window` | bool | `M-GridEdit_part0.cpp:305` | Master toggle for the Welcome/Hub screen |
| `bWelcomeNoBackButton` | bool | `M-GridEdit_part0.cpp:306` | Hides back button on welcome screen |
| `bWelcomeScreen_Init` | bool | `M-GridEdit_part0.cpp:307` | Init flag for welcome screen |
| `gbWelcomeSystemActive` | bool | extern in `M-GridEdit_part0.cpp:366` | Whether the intro/welcome system is active |
| `iCurrentOpenTab` | int (static) | `M-GridEditB_part16.cpp:507` | Which tab is currently active |

### Tab IDs (`iCurrentOpenTab` values)

| Value | Tab Name | Set At |
|-------|----------|--------|
| 0 | Demo Games | line 796 |
| 1 | My Games | line 892 |
| 3 | Tutorials | line 1232 |
| 4 | User Guide | line 1167 |
| 5 | Live Changelog | line 1463 |
| 6 | Workshop Uploader | line 1616 |
| 7 | Workshop | line 1709 |
| 42 | Community Tutorials | line 1386 |

**IMPORTANT**: `iCurrentOpenTab` is a `static` local variable inside the `else if (bWelcomeScreen_Window)` block (line 507). It is NOT directly settable from outside. Tab selection is driven by ImGui's `BeginTabItem` with `ImGuiTabItemFlags_SetSelected`. To change tabs programmatically, we'd need to either:
- Make `iCurrentOpenTab` non-static (expose it as extern), or
- Set a flag that the Welcome_Screen function checks to force-select a tab

The tab bar ID is `"welcomescreentabbar"` (line 785).

### Tab Selection Mechanism
ImGui tab selection uses `tabflags` with `ImGuiTabItemFlags_SetSelected`. Currently only "My Games" uses this conditionally (line 840). To programmatically select tabs, we need to add an `extern int g_iForceWelcomeTab` variable that the Welcome_Screen function checks.

---

## Storyboard / Project Window

### Variables

| Variable | Type | Defined In | Purpose |
|----------|------|-----------|---------|
| `bStoryboardWindow` | bool | `M-GridEdit_part0.cpp:296` | Toggles storyboard/project view |
| `bStoryboardWindowOpenLoad` | bool | extern `M-GridEditB_part0.cpp:246` | Triggers opening the load dialog |
| `TriggerLoadGameProject` | cstr | `M-GridEdit_part0.cpp:55` | Set to project name to trigger load |
| `iStoryboardExecuteKey` | int | extern `M-GridEditB_part0.cpp:78` | Simulates key press in storyboard |

### Storyboard Execute Keys
| Value | Action |
|-------|--------|
| ' ' (space) | Play the game (start test game) |
| 'N' | Add new level |
| 'L' | Load level |
| 'E' | Edit level |
| '!' | Export standalone |

---

## Edit Game / Play Game Transitions

### From My Games tab (lines 2320-2342):
```
"Play Game" button:
  TriggerLoadGameProject = current_project_selected
  bWelcomeScreen_Window = false
  bStoryboardWindow = true
  iStoryboardExecuteKey = ' '   // triggers immediate play

"Edit Game" button:
  TriggerLoadGameProject = current_project_selected
  bWelcomeScreen_Window = false
  bStoryboardWindow = true      // opens storyboard for editing
```

### From Demo Games tab (lines 2618-2659):
Similar pattern but uses `g_LibraryFileList[iFileListEntry].cProject` or falls back to direct file load via `cDirectOpen` + `iLaunchAfterSync = 7`.

### Create New Game Project (line 2829):
```
process_storeboard(true)   // init new project
bTriggerSaveAsAfterNewLevel = true
bTriggerSaveAs = true
```

---

## Test Game Mode

### Variables

| Variable | Type | Defined In | Purpose |
|----------|------|-----------|---------|
| `bImGuiInTestGame` | bool | extern `M-GridEdit_part0.cpp:405` | True when in F9 test game mode |
| `iLaunchAfterSync` | int | extern `M-GridEditB_part0.cpp:313` | State machine for launching test game |
| `g_bDisableQuitFlag` | bool | extern `M-GridEdit_part0.cpp:373` | Prevents WM_CLOSE during test game |
| `bFakeStandaloneTest` | bool | `M-GridEdit_part0.cpp:1449` | Standalone simulation mode |

### Launch Sequence
1. `iLaunchAfterSync = 201` triggers test game init (`M-GridEdit_part0.cpp:1543`)
2. Sets `bImGuiInTestGame = true`, configures resolution
3. Advances to `iLaunchAfterSync = 202`
4. 202 runs `editor_previewmap_loopcode(0)` which is the game loop
5. When loop exits, calls `mapeditorexecutable_loop_leavetestgame()` (line 1425)

---

## Direct Level Load

| Variable | Type | Defined In | Purpose |
|----------|------|-----------|---------|
| `cDirectOpen` | char[260] | `M-GridEdit_part0.cpp:421` | Path to directly open a level file |
| `iLaunchAfterSync` | int | used everywhere | 7 = direct load |
| `iSkibFramesBeforeLaunch` | int | extern `M-GridEditB_part0.cpp:322` | Delay before launch (frames) |

---

## Window Handle

`g_pGlob->hWnd` (HWND) - the main window handle, set in `main.cpp:316`.

---

## Screenshot

WickedEngine provides `wi::helper::screenshot(swapChain, name)` in `wiHelper.h:68`.
The swapchain is `master.swapChain` (Master inherits from MainComponent which has `wi::graphics::SwapChain swapChain`).
`master` is the global Master instance defined in `main.cpp:72`.

---

## Key File Locations

| File | Role |
|------|------|
| `GameGuruMain.cpp` | Contains `GuruLoopLogic()` - where we hook the automation check |
| `M-GridEdit_part0.cpp` | Declares most state booleans |
| `M-GridEditB_part0.cpp` | Externs for state variables |
| `M-GridEditB_part16.cpp` | Welcome_Screen() function with all hub tabs |
| `M-GridEditB_part19.cpp` | Storyboard window rendering |
| `M-GridEdit_part1.cpp` | `mapeditorexecutable_loop()` |
| `Common_part0.cpp` | `common_loop_logic()` dispatcher |
| `main.cpp` | WM_CLOSE handler, window creation |
| `master.h` / `master_part0.cpp` | Master class with swapchain |

---

## Automation Approach

To navigate tabs, we cannot directly set `iCurrentOpenTab` (it's static local). Instead:
1. Add `extern int g_iAutoForceWelcomeTab` to `M-GridEditB_part16.cpp`
2. In `Welcome_Screen()`, check this variable and set `tabflags = ImGuiTabItemFlags_SetSelected` for the corresponding tab
3. Reset the variable after use

For state transitions (Hub -> Storyboard -> Editor), we can directly set the boolean flags since they are global.
