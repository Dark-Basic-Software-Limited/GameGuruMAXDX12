//
// AutomationHarness.cpp - File-based command/response automation for Claude Code testing
//

#include "stdafx.h"
#include "AutomationHarness.h"
#include "GameGuruMain.h"
#include "gameguru.h"
#include "master.h"
#include "..\GameGuru\Imgui\imgui.h"
#include "..\GameGuru\Imgui\imgui_internal.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <windows.h>

// Terrain debug accessor (extern "C" from GGTerrain_part0.cpp)
extern "C" int GGTerrain_GetDrawDebugInfo(int* drawCount, int* exitReason, int* initFlag, int* drawEn, int* updateEn);

// Tree params for the SET_TREES live-tuning command
#include "GGTerrain/GGTrees.h"

// Terrain params + undo enums for the SCULPT_TEST / PAINT_TEST commands
#include "GGTerrain/GGTerrain.h"
#include "M-UndoSys.h"

// Wicked-terrain blend/bridge diagnostics defined in GGTerrainWicked.cpp (GET_PERF_DATA).
// Note: they sit at file scope BEFORE that file's namespace GGTerrain opens — global namespace.
extern uint64_t g_dbgBridgeCalls, g_dbgBridgeChunksMarked, g_dbgBridgeKeysErased,
	g_dbgAutoBlendChunks, g_dbgPaintBlendChunks;
extern size_t g_dbgInvalidatedCensus, g_dbgMergePendingCensus;
extern uint64_t g_dbgAutoSkipNoChunk, g_dbgAutoSkipNoLayers, g_dbgAutoSkipInvalid,
	g_dbgAutoSkipMergePend, g_dbgAutoPassRuns;
extern size_t g_dbgAutoLastPending;
// Grass-hair lifecycle diagnostics (shadow-flicker investigation) — global scope in GGTerrainWicked.cpp.
extern uint64_t g_dbgGrassRecycles, g_dbgGrassFullResets, g_dbgGrassRecreates;
extern size_t g_dbgGrassDeadMeshNow;

// WickedEngine helpers for screenshot and scene interrogation
#include "wiHelper.h"
#include "wiGraphicsDevice.h"
#include "wiApplication.h"
#include "wiScene.h"
#include "wiRenderer.h"

// Direct access to profiler internals for diagnostics
namespace wi::profiler {
	extern bool ENABLED;
	extern bool ENABLED_REQUEST;
}

// The global Master instance
extern Master master;

// State flags from the engine
extern bool bWelcomeScreen_Window;
extern bool bStoryboardWindow;
extern bool bImGuiInTestGame;
extern bool g_bNoGGUntilGameGuruMainCalled;
extern int g_iInitializationSequence;
extern int iLaunchAfterSync;
extern int iStoryboardExecuteKey;
extern cstr TriggerLoadGameProject;
extern bool g_bDisableQuitFlag;
extern bool bProceduralLevel;

// Screen editor (from M-GridEdit_part0.cpp)
extern bool bScreen_Editor_Window;
extern int iScreen_Editor_Node;
extern int g_iAutoExitScreenEditor;

// Toolbar toggle states (from M-GridEdit_part0.cpp)
extern bool bEditorLight;
extern bool bTerrain_Tools_Window;
extern bool Entity_Tools_Window;
extern bool Visuals_Tools_Window;
extern bool Shooter_Tools_Window;
extern bool Game_Settings_Window;
extern bool Weather_Tools_Window;

// Level loading from storyboard
extern char cDirectOpen[260];
extern int iSkibFramesBeforeLaunch;
extern bool bCloseStoryboardAfterLoad;
extern int iLevelEditorFromStoryboardID;

// Global struct for window handle
#include "globstruct.h"
extern GlobStruct* g_pGlob;

// Memory/VRAM helpers
DARKSDK int SMEMAvailable(int iMode);
float GetTotalVramUsage(void);

// Demo game library list and edit trigger
#include "M-GridEditB.h"
extern std::vector<sLibraryList> g_LibraryFileList;
extern bool bTriggerEditDemoGame;

// My Games projects list (populated by GetProjectList("projectbank\\", ...) whenever the
// hub renders; see M-GridEditB_part16.cpp:627). One string per user project folder.
#include <vector>
#include <string>
extern std::vector<std::string> projectbank_list;

// Storyboard data (StoryboardStruct defined in imgui_gg_dx11.h, included via M-GridEditB.h)
extern StoryboardStruct Storyboard;

// Tab force variable
int g_iAutoForceWelcomeTab = -1;

// Demo selection variable (set by SELECT_DEMO, consumed by Welcome_Screen)
char g_sAutoSelectDemo[260] = {0};

// Command/response file paths (absolute, computed at init from exe directory)
static char s_cmdPath[MAX_PATH] = {0};
static char s_rspPath[MAX_PATH] = {0};
static char s_logPath[MAX_PATH] = {0};

// Uptime tracking
static DWORD s_startTick = 0;
static bool s_initialized = false;

// Set to true once the harness processes its first command.
// Keeps app running even when window loses focus, so automation works while alt-tabbed.
bool g_bAutomationActive = false;

// Deferred key-up: hold key down for 2 frames so per-frame sampling detects the press
static int s_pendingKeyUpVK = 0;     // virtual key code waiting for key-up
static int s_pendingKeyUpFrames = 0; // frames remaining before sending WM_KEYUP

// Injected key press for terrain key system — consumed by GGTerrainWicked_Update()
int g_autoHarnessInjectedKey = 0; // VK code of key to inject, 0 = none

static void AutoHarness_InitPaths(void)
{
	// Get the directory containing the exe
	char exePath[MAX_PATH];
	GetModuleFileNameA(NULL, exePath, MAX_PATH);
	// Strip the exe filename to get directory
	char* lastSlash = strrchr(exePath, '\\');
	if (!lastSlash) lastSlash = strrchr(exePath, '/');
	if (lastSlash) lastSlash[1] = 0; // keep trailing slash
	else strcpy(exePath, ".\\");

	_snprintf(s_cmdPath, MAX_PATH, "%sauto_command.txt", exePath);
	_snprintf(s_rspPath, MAX_PATH, "%sauto_result.txt", exePath);
	_snprintf(s_logPath, MAX_PATH, "%sauto_log.txt", exePath);
	s_cmdPath[MAX_PATH - 1] = 0;
	s_rspPath[MAX_PATH - 1] = 0;
	s_logPath[MAX_PATH - 1] = 0;
}

// ---- Internal helpers ----

static void AutoHarness_WriteResult(const char* result)
{
	// Write result file (overwrite)
	FILE* f = fopen(s_rspPath, "w");
	if (f)
	{
		fputs(result, f);
		fclose(f);
	}

	// Append to log with timestamp
	f = fopen(s_logPath, "a");
	if (f)
	{
		time_t now = time(NULL);
		struct tm* lt = localtime(&now);
		char timebuf[64];
		strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", lt);
		fprintf(f, "[%s] RESULT:\n%s\n---\n", timebuf, result);
		fclose(f);
	}
}

static void AutoHarness_LogCommand(const char* cmd)
{
	FILE* f = fopen(s_logPath, "a");
	if (f)
	{
		time_t now = time(NULL);
		struct tm* lt = localtime(&now);
		char timebuf[64];
		strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", lt);
		fprintf(f, "[%s] COMMAND: %s\n", timebuf, cmd);
		fclose(f);
	}
}

// Get current app state as a string
static const char* AutoHarness_GetAppState(void)
{
	if (!g_bNoGGUntilGameGuruMainCalled)
		return "initializing";
	if (bImGuiInTestGame)
		return "game";
	if (bWelcomeScreen_Window)
		return "hub";
	if (bStoryboardWindow)
		return "storyboard";
	if (iLaunchAfterSync >= 200 && iLaunchAfterSync <= 210)
		return "loading";
	return "editor";
}

// Get current tab name
static const char* AutoHarness_GetTabName(int tabId)
{
	switch (tabId)
	{
		case 0: return "demo_games";
		case 1: return "my_games";
		case 3: return "tutorials";
		case 4: return "user_guide";
		case 5: return "live_changelog";
		case 6: return "workshop_uploader";
		case 7: return "workshop";
		case 42: return "community_tutorials";
		default: return "unknown";
	}
}

// Get visible ImGui windows
static int AutoHarness_GetVisibleWindows(char* outBuf, int bufSize)
{
	outBuf[0] = 0;
	int written = 0;
	ImGuiContext* ctx = ImGui::GetCurrentContext();
	if (!ctx) return 0;

	bool first = true;
	for (int i = 0; i < ctx->Windows.Size; i++)
	{
		ImGuiWindow* win = ctx->Windows[i];
		if (win && win->Active && !win->Hidden && win->Name && win->Name[0] != '#')
		{
			int needed = (int)strlen(win->Name) + 2; // comma + space
			if (written + needed >= bufSize - 1) break;
			if (!first)
			{
				outBuf[written++] = ',';
				outBuf[written++] = ' ';
			}
			int len = (int)strlen(win->Name);
			memcpy(outBuf + written, win->Name, len);
			written += len;
			first = false;
		}
	}
	outBuf[written] = 0;
	return written;
}

// ---- Command handlers ----

// We need to read iCurrentOpenTab from part16 - it's static local, so we track it via
// a global that the Welcome_Screen can write to
static int s_lastKnownTab = 0;
// This will be set by Welcome_Screen when it runs
int g_iAutoCurrentTab = 0;

static const char* AutoHarness_NodeTypeName(int type)
{
	switch (type)
	{
		case STORYBOARD_TYPE_NONE: return "none";
		case STORYBOARD_TYPE_SPLASH: return "splash";
		case STORYBOARD_TYPE_SCREEN: return "screen";
		case STORYBOARD_TYPE_LEVEL: return "level";
		case STORYBOARD_TYPE_HUD: return "hud";
		default: return "unknown";
	}
}

// Build a summary of storyboard nodes for GET_STATE
static int AutoHarness_GetStoryboardSummary(char* outBuf, int bufSize)
{
	int written = 0;
	if (!bStoryboardWindow) return 0;

	// Game name
	written += _snprintf(outBuf + written, bufSize - written,
		"PROJECT: %s\n", Storyboard.gamename[0] ? Storyboard.gamename : "(none)");

	// Count and list used nodes
	int nodeCount = 0;
	for (int i = 0; i < STORYBOARD_MAXNODES; i++)
		if (Storyboard.Nodes[i].used) nodeCount++;

	written += _snprintf(outBuf + written, bufSize - written,
		"NODES(%d):\n", nodeCount);

	for (int i = 0; i < STORYBOARD_MAXNODES && written < bufSize - 256; i++)
	{
		if (!Storyboard.Nodes[i].used) continue;
		const StoryboardNodesStruct& n = Storyboard.Nodes[i];
		written += _snprintf(outBuf + written, bufSize - written,
			"  [%d] type=%s title=\"%s\" level=\"%s\"",
			i, AutoHarness_NodeTypeName(n.type),
			n.title[0] ? n.title : "",
			n.level_name[0] ? n.level_name : "");

		// Show output connections
		for (int o = 0; o < STORYBOARD_MAXOUTPUTS; o++)
		{
			if (n.output_title[o][0] && written < bufSize - 128)
			{
				written += _snprintf(outBuf + written, bufSize - written,
					" out%d=\"%s\"->%d", o, n.output_title[o], n.output_linkto[o]);
			}
		}
		if (written < bufSize - 2)
			outBuf[written++] = '\n';
	}

	outBuf[written] = 0;
	return written;
}

static void Cmd_GetState(char* result, int resultSize)
{
	const char* state = AutoHarness_GetAppState();

	const char* tabName = "none";
	if (bWelcomeScreen_Window)
		tabName = AutoHarness_GetTabName(g_iAutoCurrentTab);

	char panels[4096];
	AutoHarness_GetVisibleWindows(panels, sizeof(panels));
	if (panels[0] == 0)
		strcpy(panels, "none");

	DWORD uptime = (GetTickCount() - s_startTick) / 1000;

	int written = _snprintf(result, resultSize,
		"STATE: %s\n"
		"TAB: %s\n"
		"VISIBLE_PANELS: %s\n"
		"ERRORS: none\n"
		"UPTIME: %lu\n",
		state, tabName, panels, uptime);
	if (written < 0) written = 0;

	// Append storyboard details when in storyboard state
	if (bStoryboardWindow && written < resultSize - 1)
	{
		AutoHarness_GetStoryboardSummary(result + written, resultSize - written);
	}

	result[resultSize - 1] = 0;
}

static void Cmd_Navigate(const char* target, char* result, int resultSize)
{
	if (!target || !target[0])
	{
		_snprintf(result, resultSize, "ERROR: NAVIGATE requires a target argument");
		result[resultSize - 1] = 0;
		return;
	}

	// Navigate to hub
	if (_stricmp(target, "hub") == 0)
	{
		bWelcomeScreen_Window = true;
		bStoryboardWindow = false;
		_snprintf(result, resultSize, "OK: Navigated to hub");
		result[resultSize - 1] = 0;
		return;
	}

	// Navigate to hub tabs
	if (_strnicmp(target, "hub.", 4) == 0)
	{
		const char* tabName = target + 4;
		int tabId = -1;

		if (_stricmp(tabName, "demo_games") == 0) tabId = 0;
		else if (_stricmp(tabName, "my_games") == 0) tabId = 1;
		else if (_stricmp(tabName, "tutorials") == 0) tabId = 3;
		else if (_stricmp(tabName, "user_guide") == 0) tabId = 4;
		else if (_stricmp(tabName, "live_changelog") == 0) tabId = 5;
		else if (_stricmp(tabName, "community_tutorials") == 0) tabId = 42;
		else if (_stricmp(tabName, "workshop_uploader") == 0) tabId = 6;
		else if (_stricmp(tabName, "workshop") == 0) tabId = 7;

		if (tabId < 0)
		{
			_snprintf(result, resultSize, "ERROR: Unknown hub tab '%s'", tabName);
			result[resultSize - 1] = 0;
			return;
		}

		bWelcomeScreen_Window = true;
		bStoryboardWindow = false;
		g_iAutoForceWelcomeTab = tabId;

		_snprintf(result, resultSize, "OK: Navigated to hub.%s (tab %d)", tabName, tabId);
		result[resultSize - 1] = 0;
		return;
	}

	// Navigate to storyboard
	if (_stricmp(target, "storyboard") == 0)
	{
		bStoryboardWindow = true;
		bWelcomeScreen_Window = false;
		_snprintf(result, resultSize, "OK: Navigated to storyboard");
		result[resultSize - 1] = 0;
		return;
	}

	_snprintf(result, resultSize, "ERROR: Unknown navigation target '%s'", target);
	result[resultSize - 1] = 0;
}

static void Cmd_Click(const char* element, char* result, int resultSize)
{
	if (!element || !element[0])
	{
		_snprintf(result, resultSize, "ERROR: CLICK requires an element argument");
		result[resultSize - 1] = 0;
		return;
	}

	if (_stricmp(element, "play_game") == 0)
	{
		// Trigger play game from storyboard
		if (bStoryboardWindow)
		{
			iStoryboardExecuteKey = ' ';
			_snprintf(result, resultSize, "OK: Triggered Play Game from storyboard");
		}
		else
		{
			_snprintf(result, resultSize, "ERROR: Not in storyboard view, cannot Play Game");
		}
		result[resultSize - 1] = 0;
		return;
	}

	if (_stricmp(element, "edit_game") == 0)
	{
		if (bWelcomeScreen_Window)
		{
			// Trigger edit from Demo Games tab (same as double-click)
			bTriggerEditDemoGame = true;
			_snprintf(result, resultSize, "OK: Triggered Edit Game from hub (demo games)");
		}
		else
		{
			_snprintf(result, resultSize, "ERROR: edit_game only works from hub. Use CLICK_NODE <title> to open a level from the storyboard");
		}
		result[resultSize - 1] = 0;
		return;
	}

	if (_stricmp(element, "test_level") == 0)
	{
		const char* state = AutoHarness_GetAppState();
		if (strcmp(state, "editor") == 0)
		{
			iLaunchAfterSync = 1;
			_snprintf(result, resultSize, "OK: Triggered Test Level from editor");
		}
		else
		{
			_snprintf(result, resultSize, "ERROR: test_level only works from editor (current state: %s)", state);
		}
		result[resultSize - 1] = 0;
		return;
	}

	if (_stricmp(element, "new_project") == 0)
	{
		// New project - only from hub
		_snprintf(result, resultSize, "ERROR: new_project not yet implemented via automation");
		result[resultSize - 1] = 0;
		return;
	}

	if (_stricmp(element, "add_level") == 0)
	{
		if (bStoryboardWindow)
		{
			iStoryboardExecuteKey = 'N';
			_snprintf(result, resultSize, "OK: Triggered Add New Level");
		}
		else
		{
			_snprintf(result, resultSize, "ERROR: Not in storyboard view");
		}
		result[resultSize - 1] = 0;
		return;
	}

	if (_stricmp(element, "load_level") == 0)
	{
		if (bStoryboardWindow)
		{
			iStoryboardExecuteKey = 'L';
			_snprintf(result, resultSize, "OK: Triggered Load Level");
		}
		else
		{
			_snprintf(result, resultSize, "ERROR: Not in storyboard view");
		}
		result[resultSize - 1] = 0;
		return;
	}

	if (_stricmp(element, "exit_screen_editor") == 0)
	{
		if (bScreen_Editor_Window)
		{
			g_iAutoExitScreenEditor = 1;
			_snprintf(result, resultSize, "OK: Triggered Exit to Storyboard from screen editor");
		}
		else
		{
			_snprintf(result, resultSize, "ERROR: Screen editor is not open");
		}
		result[resultSize - 1] = 0;
		return;
	}

	_snprintf(result, resultSize, "ERROR: Unknown click element '%s'", element);
	result[resultSize - 1] = 0;
}

static void Cmd_ListDemos(char* result, int resultSize)
{
	if (g_LibraryFileList.size() == 0)
	{
		_snprintf(result, resultSize, "OK: DEMOS(0):\n(none loaded)");
		result[resultSize - 1] = 0;
		return;
	}

	int written = _snprintf(result, resultSize, "OK: DEMOS(%d):\n", (int)g_LibraryFileList.size());
	if (written < 0) written = 0;

	for (int i = 0; i < (int)g_LibraryFileList.size(); i++)
	{
		const char* name = g_LibraryFileList[i].cName.Get();
		// Strip .png extension for display
		char displayName[260];
		strncpy(displayName, name, sizeof(displayName) - 1);
		displayName[sizeof(displayName) - 1] = 0;
		char* dot = strrchr(displayName, '.');
		if (dot) *dot = 0;

		int needed = (int)strlen(displayName) + 4; // "  N\n"
		if (written + needed >= resultSize - 1) break;
		written += _snprintf(result + written, resultSize - written, "  %s\n", displayName);
	}
	result[resultSize - 1] = 0;
}

static void Cmd_SelectDemo(const char* demoName, char* result, int resultSize)
{
	if (!demoName || !demoName[0])
	{
		_snprintf(result, resultSize, "ERROR: SELECT_DEMO requires a demo name argument");
		result[resultSize - 1] = 0;
		return;
	}

	if (!bWelcomeScreen_Window)
	{
		_snprintf(result, resultSize, "ERROR: Not on hub screen, cannot select demo");
		result[resultSize - 1] = 0;
		return;
	}

	// Search g_LibraryFileList for a case-insensitive match (by display name, without extension)
	for (int i = 0; i < (int)g_LibraryFileList.size(); i++)
	{
		const char* name = g_LibraryFileList[i].cName.Get();
		char displayName[260];
		strncpy(displayName, name, sizeof(displayName) - 1);
		displayName[sizeof(displayName) - 1] = 0;
		char* dot = strrchr(displayName, '.');
		if (dot) *dot = 0;

		if (_stricmp(displayName, demoName) == 0)
		{
			// Found it - set the global that Welcome_Screen will consume
			strncpy(g_sAutoSelectDemo, name, sizeof(g_sAutoSelectDemo) - 1);
			g_sAutoSelectDemo[sizeof(g_sAutoSelectDemo) - 1] = 0;

			// Also force to Demo Games tab
			g_iAutoForceWelcomeTab = 0;

			_snprintf(result, resultSize, "OK: Selected demo '%s' (file: %s)", displayName, name);
			result[resultSize - 1] = 0;
			return;
		}
	}

	_snprintf(result, resultSize, "ERROR: Demo '%s' not found in library (%d demos loaded)", demoName, (int)g_LibraryFileList.size());
	result[resultSize - 1] = 0;
}

// ---------------------------------------------------------------------------
// My Games project handlers
// ---------------------------------------------------------------------------
// The My Games tab drives a different code path than demo_games. When the user
// double-clicks a project card at M-GridEditB_part16.cpp:1044, the handler sets
// TriggerLoadGameProject + toggles bWelcomeScreen_Window / bStoryboardWindow.
// process_storeboard() then picks up TriggerLoadGameProject at
// M-GridEditB_part19.cpp:202 and calls load_storyboard() on the next frame.
// We replicate that here — see project memory "harness-open-my-games".

static void Cmd_ListProjects(char* result, int resultSize)
{
	int written = _snprintf(result, resultSize, "PROJECT_COUNT: %d\n", (int)projectbank_list.size());
	if (projectbank_list.empty())
	{
		written += _snprintf(result + written, resultSize - written,
			"NOTE: list not yet populated — call GET_STATE after the hub has rendered at least once\n");
	}
	for (int i = 0; i < (int)projectbank_list.size() && written < resultSize - 128; i++)
	{
		written += _snprintf(result + written, resultSize - written,
			"  [%d] %s\n", i, projectbank_list[i].c_str());
	}
	result[resultSize - 1] = 0;
}

static void Cmd_OpenProject(const char* projectName, char* result, int resultSize)
{
	if (!projectName || !projectName[0])
	{
		_snprintf(result, resultSize, "ERROR: OPEN_PROJECT requires a project name argument");
		result[resultSize - 1] = 0;
		return;
	}

	if (!bWelcomeScreen_Window)
	{
		_snprintf(result, resultSize, "ERROR: OPEN_PROJECT must be run from hub (welcome screen not visible)");
		result[resultSize - 1] = 0;
		return;
	}

	if (projectbank_list.empty())
	{
		_snprintf(result, resultSize,
			"ERROR: projectbank_list is empty — the hub renderer populates it on first frame; try GET_STATE once then retry");
		result[resultSize - 1] = 0;
		return;
	}

	// Case-insensitive name match against the projectbank folder list.
	int foundIndex = -1;
	for (int i = 0; i < (int)projectbank_list.size(); i++)
	{
		if (_stricmp(projectbank_list[i].c_str(), projectName) == 0)
		{
			foundIndex = i;
			break;
		}
	}

	if (foundIndex < 0)
	{
		int written = _snprintf(result, resultSize,
			"ERROR: project '%s' not found (%d available):", projectName, (int)projectbank_list.size());
		for (int i = 0; i < (int)projectbank_list.size() && written < resultSize - 64; i++)
		{
			written += _snprintf(result + written, resultSize - written, " '%s'", projectbank_list[i].c_str());
		}
		result[resultSize - 1] = 0;
		return;
	}

	// Copy the resolved name into a static buffer so TriggerLoadGameProject (cstr) has a
	// stable const char* to consume even if projectbank_list re-sorts before the next
	// frame's process_storeboard() picks up the trigger.
	static char s_openProjectName[260];
	strncpy(s_openProjectName, projectbank_list[foundIndex].c_str(), sizeof(s_openProjectName) - 1);
	s_openProjectName[sizeof(s_openProjectName) - 1] = 0;

	// Replicate the double-click handler at M-GridEditB_part16.cpp:1121:
	//   TriggerLoadGameProject = <name>;
	//   bWelcomeScreen_Window = false;
	//   bStoryboardWindow = true;
	TriggerLoadGameProject = s_openProjectName;
	bWelcomeScreen_Window = false;
	bStoryboardWindow = true;

	_snprintf(result, resultSize,
		"OK: Opening project '%s' (index %d of %d) — storyboard will render on next frame",
		s_openProjectName, foundIndex, (int)projectbank_list.size());
	result[resultSize - 1] = 0;
}

// Convenience: click the FIRST storyboard node whose type=level and level_name
// is non-empty. TESTPROJ1 style — one level, don't want to require the caller
// to know its exact title.
static void Cmd_ClickOnlyLevel(char* result, int resultSize)
{
	if (!bStoryboardWindow)
	{
		_snprintf(result, resultSize, "ERROR: CLICK_ONLY_LEVEL must be run from storyboard view");
		result[resultSize - 1] = 0;
		return;
	}

	for (int i = 0; i < STORYBOARD_MAXNODES; i++)
	{
		if (!Storyboard.Nodes[i].used) continue;
		const StoryboardNodesStruct& n = Storyboard.Nodes[i];
		if (n.type != STORYBOARD_TYPE_LEVEL) continue;
		if (strlen(n.level_name) == 0) continue;

		// Mirror Cmd_ClickNode's level-load path.
		strcpy(cDirectOpen, n.level_name);
		iLaunchAfterSync = 7;
		iSkibFramesBeforeLaunch = 5;
		bCloseStoryboardAfterLoad = true;
		iLevelEditorFromStoryboardID = i;

		_snprintf(result, resultSize,
			"OK: Loading first level node '%s' (file: %s, index: %d)",
			n.title, n.level_name, i);
		result[resultSize - 1] = 0;
		return;
	}

	_snprintf(result, resultSize, "ERROR: No level node with an assigned level file found in storyboard");
	result[resultSize - 1] = 0;
}

static void Cmd_Wait(const char* msStr, char* result, int resultSize)
{
	int ms = 0;
	if (msStr && msStr[0])
		ms = atoi(msStr);
	if (ms > 0 && ms <= 30000)
		Sleep((DWORD)ms);

	// Return state after wait
	Cmd_GetState(result, resultSize);
}

static void Cmd_Screenshot(char* result, int resultSize)
{
	// Use WickedEngine screenshot API
	std::string outPath = wi::helper::screenshot(master.swapChain, "auto_screenshot");
	if (outPath.empty())
	{
		_snprintf(result, resultSize, "ERROR: Screenshot failed");
	}
	else
	{
		_snprintf(result, resultSize, "OK: Screenshot saved to %s", outPath.c_str());
	}
	result[resultSize - 1] = 0;
}

static void Cmd_GetScreenText(char* result, int resultSize)
{
	int written = 0;
	const char* state = AutoHarness_GetAppState();
	written += _snprintf(result + written, resultSize - written, "STATE: %s\n", state);

	if (bStoryboardWindow)
	{
		written += _snprintf(result + written, resultSize - written,
			"PROJECT: %s\n"
			"DESCRIPTION: %.200s\n"
			"READONLY: %d\n",
			Storyboard.gamename[0] ? Storyboard.gamename : "(none)",
			Storyboard.game_description[0] ? Storyboard.game_description : "",
			Storyboard.project_readonly);

		// Detailed node dump
		for (int i = 0; i < STORYBOARD_MAXNODES && written < resultSize - 512; i++)
		{
			if (!Storyboard.Nodes[i].used) continue;
			const StoryboardNodesStruct& n = Storyboard.Nodes[i];
			written += _snprintf(result + written, resultSize - written,
				"NODE[%d]: type=%s title=\"%s\" level=\"%s\" levelnumber=\"%s\" editable=%d\n",
				i, AutoHarness_NodeTypeName(n.type),
				n.title[0] ? n.title : "",
				n.level_name[0] ? n.level_name : "",
				n.levelnumber[0] ? n.levelnumber : "",
				n.iEditEnable);

			// Screen-specific info
			if (n.type == STORYBOARD_TYPE_SCREEN || n.type == STORYBOARD_TYPE_SPLASH)
			{
				if (n.screen_title[0] && written < resultSize - 256)
					written += _snprintf(result + written, resultSize - written,
						"  screen_title=\"%s\"\n", n.screen_title);
			}

			// Output pins
			for (int o = 0; o < STORYBOARD_MAXOUTPUTS && written < resultSize - 256; o++)
			{
				if (n.output_title[o][0])
					written += _snprintf(result + written, resultSize - written,
						"  out[%d]: \"%s\" action=\"%s\" linkto=%d\n",
						o, n.output_title[o], n.output_action[o], n.output_linkto[o]);
			}

			// Input pins
			for (int inp = 0; inp < STORYBOARD_MAXOUTPUTS && written < resultSize - 256; inp++)
			{
				if (n.input_title[inp][0])
					written += _snprintf(result + written, resultSize - written,
						"  in[%d]: \"%s\"\n", inp, n.input_title[inp]);
			}

			// Widgets with labels
			for (int w = 0; w < STORYBOARD_MAXWIDGETS && written < resultSize - 256; w++)
			{
				if (n.widget_used[w] && n.widget_label[w][0])
					written += _snprintf(result + written, resultSize - written,
						"  widget[%d]: label=\"%s\" type=%d\n",
						w, n.widget_label[w], n.widget_type[w]);
			}
		}
	}
	else if (bWelcomeScreen_Window)
	{
		written += _snprintf(result + written, resultSize - written,
			"TAB: %s\n", AutoHarness_GetTabName(g_iAutoCurrentTab));

		// List demos if on demo tab
		if (g_iAutoCurrentTab == 0 && g_LibraryFileList.size() > 0)
		{
			written += _snprintf(result + written, resultSize - written, "DEMOS:\n");
			for (int i = 0; i < (int)g_LibraryFileList.size() && written < resultSize - 256; i++)
			{
				const char* name = g_LibraryFileList[i].cName.Get();
				written += _snprintf(result + written, resultSize - written, "  %s\n", name);
			}
		}
	}
	else
	{
		// Editor state - list visible panels
		char panels[4096];
		AutoHarness_GetVisibleWindows(panels, sizeof(panels));
		written += _snprintf(result + written, resultSize - written,
			"PANELS: %s\n", panels[0] ? panels : "none");

		// Enumerate toolbar buttons with their tooltip labels and toggle states
		ImGuiWindow* toolbarWin = ImGui::FindWindowByName("Toolbar");
		if (toolbarWin && toolbarWin->Active)
		{
			written += _snprintf(result + written, resultSize - written, "TOOLBAR_BUTTONS:\n");
			written += _snprintf(result + written, resultSize - written, "  Back to Game Project Storyboard\n");
			written += _snprintf(result + written, resultSize - written, "  Save Level\n");
			written += _snprintf(result + written, resultSize - written, "  Test Level\n");
			if (g.gvrmode > 0 && g.gvrmodefordevelopers == 1)
				written += _snprintf(result + written, resultSize - written, "  Test Level in VR\n");
			if (t.visuals.bEnableEmptyLevelMode == false)
				written += _snprintf(result + written, resultSize - written, "  Terrain, Painting, Trees and Vegetation [active=%d]\n", bTerrain_Tools_Window ? 1 : 0);
			written += _snprintf(result + written, resultSize - written, "  Object Tools [active=%d]\n", Entity_Tools_Window ? 1 : 0);
			written += _snprintf(result + written, resultSize - written, "  Visual Logic Connections [active=%d]\n", Shooter_Tools_Window ? 1 : 0);
			written += _snprintf(result + written, resultSize - written, "  Environment Effects [active=%d]\n", Visuals_Tools_Window ? 1 : 0);
			written += _snprintf(result + written, resultSize - written, "  Game Settings [active=%d]\n", Game_Settings_Window ? 1 : 0);
			written += _snprintf(result + written, resultSize - written, "  Editor Light [active=%d]\n", bEditorLight ? 1 : 0);
			written += _snprintf(result + written, resultSize - written, "  Camera View\n");
		}

		// Menu bar items
		if (toolbarWin && toolbarWin->Active)
		{
			written += _snprintf(result + written, resultSize - written, "MENU_BAR: File, Edit, Tools, Help\n");
		}
	}

	result[resultSize - 1] = 0;
}

static void Cmd_ClickNode(const char* nodeTitle, char* result, int resultSize)
{
	if (!nodeTitle || !nodeTitle[0])
	{
		_snprintf(result, resultSize, "ERROR: CLICK_NODE requires a node title argument");
		result[resultSize - 1] = 0;
		return;
	}

	if (!bStoryboardWindow)
	{
		_snprintf(result, resultSize, "ERROR: Not in storyboard view, cannot click node");
		result[resultSize - 1] = 0;
		return;
	}

	// Find the node by title (case-insensitive)
	for (int i = 0; i < STORYBOARD_MAXNODES; i++)
	{
		if (!Storyboard.Nodes[i].used) continue;
		if (_stricmp(Storyboard.Nodes[i].title, nodeTitle) != 0) continue;

		const StoryboardNodesStruct& n = Storyboard.Nodes[i];

		if (n.type == STORYBOARD_TYPE_LEVEL && strlen(n.level_name) > 0)
		{
			// Load level and exit to editor (mirrors storyboard click logic)
			strcpy(cDirectOpen, n.level_name);
			iLaunchAfterSync = 7; // Direct load
			iSkibFramesBeforeLaunch = 5;
			bCloseStoryboardAfterLoad = true;
			iLevelEditorFromStoryboardID = i;
			_snprintf(result, resultSize, "OK: Loading level node '%s' (file: %s)", n.title, n.level_name);
		}
		else if (n.type == STORYBOARD_TYPE_LEVEL)
		{
			_snprintf(result, resultSize, "ERROR: Level node '%s' has no level file assigned", n.title);
		}
		else if (n.type == STORYBOARD_TYPE_SCREEN || n.type == STORYBOARD_TYPE_SPLASH)
		{
			// Open the screen editor for this node
			bScreen_Editor_Window = true;
			iScreen_Editor_Node = i;
			_snprintf(result, resultSize, "OK: Opened screen editor for node '%s' (type=%s, index=%d)",
				n.title, AutoHarness_NodeTypeName(n.type), i);
		}
		else
		{
			_snprintf(result, resultSize, "OK: Clicked node '%s' (type=%s) - note: only level/screen nodes trigger actions",
				n.title, AutoHarness_NodeTypeName(n.type));
		}
		result[resultSize - 1] = 0;
		return;
	}

	_snprintf(result, resultSize, "ERROR: Node '%s' not found in storyboard", nodeTitle);
	result[resultSize - 1] = 0;
}

static void Cmd_GetPerfData(char* result, int resultSize)
{
	int written = 0;
	const char* state = AutoHarness_GetAppState();

	// FPS and frame time
	float fps = ImGui::GetIO().Framerate;
	float frameTimeMs = (fps > 0.0f) ? (1000.0f / fps) : 0.0f;
	written += _snprintf(result + written, resultSize - written,
		"STATE: %s\n"
		"FPS: %.1f\n"
		"FRAME_TIME_MS: %.2f\n",
		state, fps, frameTimeMs);

	// System memory (MB)
	int memMB = SMEMAvailable(1);
	float memGB = (float)memMB / 1024.0f;
	written += _snprintf(result + written, resultSize - written,
		"SYSTEM_MEM_MB: %d\n"
		"SYSTEM_MEM_GB: %.2f\n",
		memMB, memGB);

	// VRAM (MB)
	float vramMB = GetTotalVramUsage();
	written += _snprintf(result + written, resultSize - written,
		"VRAM_MB: %.1f\n"
		"VRAM_GB: %.2f\n",
		vramMB, vramMB / 1024.0f);

	// GPU adapter name
	auto* device = wi::graphics::GetDevice();
	if (device)
	{
		written += _snprintf(result + written, resultSize - written,
			"GPU_ADAPTER: %s\n", device->GetAdapterName().c_str());
	}

	// Scene component counts
	wi::scene::Scene* pScene = master.masterrenderer.scene;
	if (pScene && written < resultSize - 512)
	{
		written += _snprintf(result + written, resultSize - written,
			"SCENE_OBJECTS: %d\n"
			"SCENE_MESHES: %d\n"
			"SCENE_MATERIALS: %d\n"
			"SCENE_LIGHTS: %d\n"
			"SCENE_TRANSFORMS: %d\n"
			"SCENE_CAMERAS: %d\n"
			"SCENE_EMITTERS: %d\n"
			"SCENE_HAIRS: %d\n"
			"SCENE_ANIMATIONS: %d\n"
			"SCENE_ARMATURES: %d\n"
			"SCENE_DECALS: %d\n"
			"SCENE_PROBES: %d\n"
			"SCENE_SOUNDS: %d\n"
			"SCENE_COLLIDERS: %d\n"
			"SCENE_RIGIDBODIES: %d\n"
			"SCENE_SOFTBODIES: %d\n"
			"SCENE_SCRIPTS: %d\n"
			"SCENE_WEATHERS: %d\n",
			(int)pScene->objects.GetCount(),
			(int)pScene->meshes.GetCount(),
			(int)pScene->materials.GetCount(),
			(int)pScene->lights.GetCount(),
			(int)pScene->transforms.GetCount(),
			(int)pScene->cameras.GetCount(),
			(int)pScene->emitters.GetCount(),
			(int)pScene->hairs.GetCount(),
			(int)pScene->animations.GetCount(),
			(int)pScene->armatures.GetCount(),
			(int)pScene->decals.GetCount(),
			(int)pScene->probes.GetCount(),
			(int)pScene->sounds.GetCount(),
			(int)pScene->colliders.GetCount(),
			(int)pScene->rigidbodies.GetCount(),
			(int)pScene->softbodies.GetCount(),
			(int)pScene->scripts.GetCount(),
			(int)pScene->weathers.GetCount());
	}

	// Visibility counts from the main render pass
	if (written < resultSize - 256)
	{
		written += _snprintf(result + written, resultSize - written,
			"VISIBLE_OBJECTS: %d\n"
			"VISIBLE_LIGHTS: %d\n"
			"VISIBLE_DECALS: %d\n"
			"VISIBLE_ENVPROBES: %d\n"
			"VISIBLE_EMITTERS: %d\n"
			"VISIBLE_HAIRS: %d\n",
			(int)master.masterrenderer.visibility_main.visibleObjects.size(),
			(int)master.masterrenderer.visibility_main.visibleLights.size(),
			(int)master.masterrenderer.visibility_main.visibleDecals.size(),
			(int)master.masterrenderer.visibility_main.visibleEnvProbes.size(),
			(int)master.masterrenderer.visibility_main.visibleEmitters.size(),
			(int)master.masterrenderer.visibility_main.visibleHairs.size());
	}

	// Delayed (staggered) directional shadow cascades — should follow the project's Delayed Shadows
	// setting (OFF at HIGHEST quality). ON = staggered refresh (can lag/flicker shadows under camera
	// movement); OFF = every-frame cascades (stable).
	written += _snprintf(result + written, resultSize - written,
		"DELAYED_SHADOWS_ENGINE: %s\n",
		wi::renderer::GetDelayedShadowCascadesEnabled() ? "ON (staggered)" : "OFF (every-frame)");
	// Shadow LOD override: ON = terrain casts shadows at a per-cascade LOD that can oscillate vs the
	// visible chunk (the "two terrain shapes" flicker); OFF = shadows use the stable main-view LOD.
	written += _snprintf(result + written, resultSize - written,
		"SHADOW_LOD_OVERRIDE_ENGINE: %s\n",
		wi::renderer::IsShadowLODOverrideEnabled() ? "ON (per-cascade LOD, can flicker)" : "OFF (view LOD)");
	// Far-cascade caster cull (DX11 parity): ON = objects only shadow into the near cascades (far
	// cascades render terrain only) - trims the every-other-frame staggered-cascade CPU spike.
	written += _snprintf(result + written, resultSize - written,
		"SHADOW_FARCULL_ENGINE: %s\n",
		wi::renderer::GetShadowFarCascadeCull() ? "ON (near cascades only)" : "OFF (all cascades)");
	// GG Phase 1 point-light shadow budget (revived iShadowPointMax). GRANTED = local casters that won
	// an atlas shadow slot this frame; CAPPED = casters denied a slot (rendered fully lit, DX11-style);
	// RENDERED = local shadows actually re-drawn this frame. Budget -1 => uncapped (stock, all render).
	{
		int shGranted = 0, shCapped = 0, shRendered = 0;
		wi::renderer::GetLocalShadowStats(shGranted, shCapped, shRendered);
		written += _snprintf(result + written, resultSize - written,
			"SHADOW_LOCAL_GRANTED: %d\n"
			"SHADOW_LOCAL_CAPPED: %d\n"
			"SHADOW_LOCAL_RENDERED: %d\n"
			"SHADOW_LOCAL_CACHE_ENGINE: %s\n",
			shGranted, shCapped, shRendered,
			wi::renderer::GetLocalShadowCachingEnabled() ? "ON (static-cache)" : "OFF");
	}

	// Camera position and orientation (for diagnosing camera issues)
	{
		auto& cam = wiScene::GetCamera();
		written += _snprintf(result + written, resultSize - written,
			"CAMERA_EYE: %.1f, %.1f, %.1f\n"
			"CAMERA_AT: %.3f, %.3f, %.3f\n"
			"CAMERA_UP: %.3f, %.3f, %.3f\n",
			cam.Eye.x, cam.Eye.y, cam.Eye.z,
			cam.At.x, cam.At.y, cam.At.z,
			cam.Up.x, cam.Up.y, cam.Up.z);
	}

	// Caustic debug: the actual value being uploaded to GGCustomFrameCB (b4) each frame.
	// 0 => Wicked_Update_Visuals never populated it; ~0.00067 => default Caustic Size 3.0.
	{
		written += _snprintf(result + written, resultSize - written,
			"CAUSTIC_SCALE: %.4f (ShaderOcean.caustic_scale -> engine lightingHF seabed caustics)\n"
			"CAUSTIC_PATCHLEN: %.2f (drives waves; caustic size is now independent of it)\n"
			"CAUSTIC_SIZE_FROM_SLIDER: %.2f (what the visuals/slider path delivered; -1 = never ran)\n",
			wiScene::GetScene().ocean.params.caustic_scale,
			wiScene::GetScene().ocean.params.patch_length,
			[]() -> float { extern float g_dbgCausticSizeFromVisuals; return g_dbgCausticSizeFromVisuals; }());
	}

	// Water-height trace: the GG water line from source (g.gdefaultwaterheight) through the
	// weather component and the per-frame blended scene.weather singleton, to the value actually
	// uploaded to GGCustomFrameCB (b4 -> g_xFrame_WaterHeight in the terrain/grass/tree shaders).
	// If WH_GGCB diverges from WH_SCENE_WEATHER the b4 upload path is broken (it was, in Wicked
	// mode, until GGCustomFrame_Update was hooked before the terrain early-return).
	{
		extern wi::ecs::Entity g_weatherEntityID;
		wiScene::WeatherComponent* whComp = wiScene::GetScene().weathers.GetComponent(g_weatherEntityID);
		written += _snprintf(result + written, resultSize - written,
			"WH_GDEFAULT: %.2f (g.gdefaultwaterheight - GG source of truth)\n"
			"WH_COMPONENT: %.2f (weathers[g_weatherEntityID].oceanParameters.waterHeight - written by Wicked_Update_Visuals)\n"
			"WH_SCENE_WEATHER: %.2f (scene.weather.oceanParameters.waterHeight - read by GGCustomFrame_Update)\n"
			"WH_GGCB: %.2f (actual value uploaded to GGCustomFrameCB b4)\n",
			g.gdefaultwaterheight,
			whComp ? whComp->oceanParameters.waterHeight : -99999.0f,
			wiScene::GetScene().weather.oceanParameters.waterHeight,
			GGTerrain::GGCustomFrame_GetWaterHeight());
	}

	// Surface water colour diagnostics: what the ocean SURFACE (oceanSurfacePS) receives. The rgb
	// is the base/albedo colour, a is the surface opacity (drives refraction fog); extinction is the
	// per-channel absorption tint. If WATER_ALPHA is ~0 the base colour barely shows from above.
	{
		auto& op = wiScene::GetScene().weather.oceanParameters; // what oceanSurfacePS is marshalled from
		written += _snprintf(result + written, resultSize - written,
			"WATER_COLOR: %.3f, %.3f, %.3f\n"
			"WATER_ALPHA: %.3f (surface opacity; ~0 = fully transparent, base colour won't show from above)\n"
			"WATER_EXTINCTION: %.3f, %.3f, %.3f (absorption tint)\n",
			op.waterColor.x, op.waterColor.y, op.waterColor.z, op.waterColor.w,
			op.extinctionColor.x, op.extinctionColor.y, op.extinctionColor.z);
	}

	// Hair/grass simulation diagnostics — the hair simulate compute pass dispatches one thread per
	// STRAND per hair system every frame (regardless of how many are drawn), so total strand count
	// is the GPU cost driver. Report totals + the per-system params that scale per-strand cost.
	{
		auto& sc = wiScene::GetScene();
		uint64_t totalStrands = 0, maxStrands = 0;
		uint32_t seg = 0, bb = 0; float vd = 0, len = 0;
		for (size_t i = 0; i < sc.hairs.GetCount(); i++)
		{
			auto& h = sc.hairs[i];
			totalStrands += h.strandCount;
			if (h.strandCount > maxStrands) maxStrands = h.strandCount;
			if (i == 0) { seg = h.segmentCount; bb = h.billboardCount; vd = h.viewDistance; len = h.length; }
		}
		written += _snprintf(result + written, resultSize - written,
			"HAIR_SYSTEMS: %d\n"
			"HAIR_TOTAL_STRANDS: %llu (one sim thread each, every frame)\n"
			"HAIR_MAX_STRANDS_PER_SYS: %llu\n"
			"HAIR_SEG: %u  HAIR_BILLBOARD: %u  HAIR_VIEWDIST: %.0f  HAIR_LEN: %.1f\n",
			(int)sc.hairs.GetCount(), (unsigned long long)totalStrands, (unsigned long long)maxStrands,
			seg, bb, vd, len);
	}

	// Terrain debug info
	{
		int drawCount=0, exitReason=0, initFlag=0, drawEn=0, updateEn=0;
		GGTerrain_GetDrawDebugInfo(&drawCount, &exitReason, &initFlag, &drawEn, &updateEn);
		written += _snprintf(result + written, resultSize - written,
			"TERRAIN_DRAW_COUNT: %d\n"
			"TERRAIN_DRAW_EXIT: %d\n"
			"TERRAIN_INIT: %d\n"
			"TERRAIN_DRAW_EN: %d\n"
			"TERRAIN_UPDATE_EN: %d\n",
			drawCount, exitReason, initFlag, drawEn, updateEn);
	}

	// PERF P.3: per-frame editor entity-pick reuse cache effectiveness (cumulative since launch)
	{
		extern int g_pickRealRuns, g_pickCacheHits, g_pickMissMask, g_pickMissRay;
		written += _snprintf(result + written, resultSize - written,
			"PICK_REAL_RUNS: %d (scene raycasts actually executed)\n"
			"PICK_CACHE_HITS: %d (reused - ray unchanged)\n"
			"PICK_MISS_MASK: %d (miss: layer/output pattern differed)\n"
			"PICK_MISS_RAY: %d (miss: cursor/camera differed)\n",
			g_pickRealRuns, g_pickCacheHits, g_pickMissMask, g_pickMissRay);
	}

	// PERF P.3: editor placement state (tells us which gridedit_mapediting path runs at idle)
	written += _snprintf(result + written, resultSize - written,
		"EDIT_STATE: gridentity=%d gridentityobj=%d pickedSection=%d mclick=%d selstage=%d grideditselect=%d\n",
		t.gridentity, t.gridentityobj, t.widget.pickedSection, t.inputsys.mclick,
		t.selstage, t.grideditselect);

	// Wicked-terrain blend/bridge diagnostics (cumulative counters + last-frame censuses)
	{
		using namespace GGTerrain;
		written += _snprintf(result + written, resultSize - written,
			"TERRAINW_BRIDGE_CALLS: %llu\n"
			"TERRAINW_BRIDGE_MARKED: %llu\n"
			"TERRAINW_BRIDGE_KEYS_ERASED: %llu\n"
			"TERRAINW_AUTOBLEND_CHUNKS: %llu\n"
			"TERRAINW_PAINTBLEND_CHUNKS: %llu\n"
			"TERRAINW_INVALIDATED_NOW: %llu\n"
			"TERRAINW_MERGEPENDING_NOW: %llu\n"
			"TERRAINW_AUTO_RUNS: %llu\n"
			"TERRAINW_AUTO_LASTPENDING: %llu\n"
			"TERRAINW_AUTO_SKIP_NOCHUNK: %llu\n"
			"TERRAINW_AUTO_SKIP_NOLAYERS: %llu\n"
			"TERRAINW_AUTO_SKIP_INVALID: %llu\n"
			"TERRAINW_AUTO_SKIP_MERGEPEND: %llu\n"
			"GRASS_RECYCLES: %llu\n"
			"GRASS_FULLRESETS: %llu\n"
			"GRASS_RECREATES: %llu\n"
			"GRASS_DEADMESH_NOW: %llu\n",
			(unsigned long long)g_dbgBridgeCalls,
			(unsigned long long)g_dbgBridgeChunksMarked,
			(unsigned long long)g_dbgBridgeKeysErased,
			(unsigned long long)g_dbgAutoBlendChunks,
			(unsigned long long)g_dbgPaintBlendChunks,
			(unsigned long long)g_dbgInvalidatedCensus,
			(unsigned long long)g_dbgMergePendingCensus,
			(unsigned long long)g_dbgAutoPassRuns,
			(unsigned long long)g_dbgAutoLastPending,
			(unsigned long long)g_dbgAutoSkipNoChunk,
			(unsigned long long)g_dbgAutoSkipNoLayers,
			(unsigned long long)g_dbgAutoSkipInvalid,
			(unsigned long long)g_dbgAutoSkipMergePend,
			(unsigned long long)g_dbgGrassRecycles,
			(unsigned long long)g_dbgGrassFullResets,
			(unsigned long long)g_dbgGrassRecreates,
			(unsigned long long)g_dbgGrassDeadMeshNow);
	}

	// Tab mode (profiler panel state)
	written += _snprintf(result + written, resultSize - written,
		"TAB_MODE: %d\n", g.tabmode);

	// Fog / atmosphere debug (level visuals + live weather component values)
	{
		extern wi::ecs::Entity g_weatherEntityID;
		wi::scene::WeatherComponent* weather = wi::scene::GetScene().weathers.GetComponent(g_weatherEntityID);
		written += _snprintf(result + written, resultSize - written,
			"FOG_VIS: near=%.1f far=%.1f a=%.3f rgb=(%.0f,%.0f,%.0f)\n"
			"FOG_WEATHER: start=%.1f density=%.8f overrideCol=%d horizon=(%.2f,%.2f,%.2f)\n",
			t.visuals.FogNearest_f, t.visuals.FogDistance_f, t.visuals.FogA_f,
			t.visuals.FogR_f, t.visuals.FogG_f, t.visuals.FogB_f,
			weather ? weather->fogStart : -1.0f,
			weather ? weather->fogDensity : -1.0f,
			weather ? (weather->IsOverrideFogColor() ? 1 : 0) : -1,
			weather ? weather->horizon.x : -1.0f,
			weather ? weather->horizon.y : -1.0f,
			weather ? weather->horizon.z : -1.0f);
	}

	// Profiler timing data (if profiler is enabled)
	if (wi::profiler::IsEnabled() && written < resultSize - 256)
	{
		float cpuMs = wi::profiler::GetCPUFrameTime();
		float gpuMs = wi::profiler::GetGPUFrameTime();
		written += _snprintf(result + written, resultSize - written,
			"CPU_FRAME_MS: %.2f\n"
			"GPU_FRAME_MS: %.2f\n",
			cpuMs, gpuMs);

		// Use cached text from Compose() which captures all GPU sub-ranges.
		// GetTextData() called here during Update would miss GPU ranges
		// because BeginFrame() clears in_use before Render creates them.
		extern std::string GGPerf_GetCachedProfilerText();
		std::string profText = GGPerf_GetCachedProfilerText();
		if (!profText.empty() && written + (int)profText.size() < resultSize - 32)
		{
			written += _snprintf(result + written, resultSize - written,
				"PROFILER_DATA:\n%s", profText.c_str());
		}
	}

	result[resultSize - 1] = 0;
}

static void Cmd_ToggleProfiler(char* result, int resultSize)
{
	const char* state = AutoHarness_GetAppState();
	if (strcmp(state, "game") != 0)
	{
		_snprintf(result, resultSize, "ERROR: TOGGLE_PROFILER only works in game state (current state: %s)", state);
		result[resultSize - 1] = 0;
		return;
	}

	// Cycle g.tabmode exactly like the TAB key does (M-Game_part3.cpp line 93)
	g.tabmode = g.tabmode + 1;
	if (g.tabmode > 2)
		g.tabmode = 0;

	const char* modeName = "game (normal)";
	if (g.tabmode == 1) modeName = "visuals panel";
	if (g.tabmode == 2) modeName = "performance panel";

	_snprintf(result, resultSize, "OK: Toggled to tabmode=%d (%s)", g.tabmode, modeName);
	result[resultSize - 1] = 0;
}

static void Cmd_PressEscape(char* result, int resultSize)
{
	const char* state = AutoHarness_GetAppState();
	if (strcmp(state, "game") == 0)
	{
		// Same exit path as ESC key in M-Game_part1.cpp line 1523
		t.game.gameloop = 0;
		t.game.levelloop = 0;
		t.game.masterloop = 0;
		_snprintf(result, resultSize, "OK: Triggered escape from test game (gameloop=0, levelloop=0, masterloop=0)");
	}
	else
	{
		_snprintf(result, resultSize, "ERROR: PRESS_ESCAPE only works in game state (current state: %s)", state);
	}
	result[resultSize - 1] = 0;
}

static int AutoHarness_ParseKeyName(const char* name)
{
	if (!name || !name[0]) return -1;

	// Single character: A-Z, 0-9
	if (name[1] == 0)
	{
		char c = name[0];
		if (c >= 'a' && c <= 'z') return 0x41 + (c - 'a');
		if (c >= 'A' && c <= 'Z') return 0x41 + (c - 'A');
		if (c >= '0' && c <= '9') return 0x30 + (c - '0');
		return -1;
	}

	// Named keys (case-insensitive)
	if (_stricmp(name, "ESCAPE") == 0) return 0x1B;
	if (_stricmp(name, "ESC") == 0) return 0x1B;
	if (_stricmp(name, "ENTER") == 0) return 0x0D;
	if (_stricmp(name, "RETURN") == 0) return 0x0D;
	if (_stricmp(name, "SPACE") == 0) return 0x20;
	if (_stricmp(name, "TAB") == 0) return 0x09;
	if (_stricmp(name, "BACKSPACE") == 0) return 0x08;
	if (_stricmp(name, "DELETE") == 0) return 0x2E;
	if (_stricmp(name, "INSERT") == 0) return 0x2D;
	if (_stricmp(name, "HOME") == 0) return 0x24;
	if (_stricmp(name, "END") == 0) return 0x23;
	if (_stricmp(name, "PAGEUP") == 0) return 0x21;
	if (_stricmp(name, "PAGEDOWN") == 0) return 0x22;
	if (_stricmp(name, "LEFT") == 0) return 0x25;
	if (_stricmp(name, "UP") == 0) return 0x26;
	if (_stricmp(name, "RIGHT") == 0) return 0x27;
	if (_stricmp(name, "DOWN") == 0) return 0x28;
	if (_stricmp(name, "SHIFT") == 0) return 0x10;
	if (_stricmp(name, "CONTROL") == 0) return 0x11;
	if (_stricmp(name, "CTRL") == 0) return 0x11;
	if (_stricmp(name, "ALT") == 0) return 0x12;

	// Function keys F1-F12
	if ((name[0] == 'F' || name[0] == 'f') && name[1] >= '1' && name[1] <= '9')
	{
		int fnum = atoi(name + 1);
		if (fnum >= 1 && fnum <= 12) return 0x70 + (fnum - 1);
	}

	return -1;
}

static void Cmd_PressKey(const char* arg, char* result, int resultSize)
{
	if (!arg || !arg[0])
	{
		_snprintf(result, resultSize, "ERROR: PRESS_KEY requires a key name (e.g. PRESS_KEY Y, PRESS_KEY F1, PRESS_KEY ESCAPE)");
		result[resultSize - 1] = 0;
		return;
	}

	if (!g_pGlob || !g_pGlob->hWnd)
	{
		_snprintf(result, resultSize, "ERROR: No window handle available");
		result[resultSize - 1] = 0;
		return;
	}

	int vk = AutoHarness_ParseKeyName(arg);
	if (vk < 0)
	{
		_snprintf(result, resultSize, "ERROR: Unknown key name '%s'. Use A-Z, 0-9, F1-F12, ESCAPE, ENTER, SPACE, TAB, SHIFT, CONTROL, ALT, arrows, etc.", arg);
		result[resultSize - 1] = 0;
		return;
	}

	// Post WM_KEYDOWN to the main window for the game's own input handling
	UINT scanCode = MapVirtualKey(vk, MAPVK_VK_TO_VSC);
	LPARAM lParamDown = (1 /*repeat=1*/) | (scanCode << 16);
	PostMessage(g_pGlob->hWnd, WM_KEYDOWN, (WPARAM)vk, lParamDown);

	// Also set ImGui key state directly — during test game mode, the ImGui WndProc
	// handler is bypassed so WM_KEYDOWN never reaches io.KeysDown[]. This ensures
	// terrain key sampling (GGTerrain_CheckKeys) detects the press.
	if (vk < 256)
		ImGui::GetIO().KeysDown[vk] = true;

	// Set injected key for terrain key system — in editor mode, the timing
	// between harness execution and GGTerrain_CheckKeys() can cause the rising
	// edge detection to miss the press from io.KeysDown[] alone.
	// GGTerrainWicked_Update() consumes this and clears it.
	extern int g_autoHarnessInjectedKey;
	g_autoHarnessInjectedKey = vk;

	s_pendingKeyUpVK = vk;
	s_pendingKeyUpFrames = 2;

	_snprintf(result, resultSize, "OK: Pressed key '%s' (VK=0x%02X)", arg, vk);
	result[resultSize - 1] = 0;
}

static void Cmd_Quit(char* result, int resultSize)
{
	_snprintf(result, resultSize, "OK: Quitting application");
	result[resultSize - 1] = 0;
	AutoHarness_WriteResult(result);

	// Post WM_CLOSE to the main window
	if (g_pGlob && g_pGlob->hWnd)
	{
		g_bDisableQuitFlag = false;
		PostMessage(g_pGlob->hWnd, WM_CLOSE, 0, 0);
	}
}

// ---- Scene interrogation commands ----

static void Cmd_ListEntities(const char* filter, char* result, int resultSize)
{
	int written = 0;
	const char* state = AutoHarness_GetAppState();

	bool filterLights = (filter && _stricmp(filter, "lights") == 0);

	written += _snprintf(result + written, resultSize - written,
		"STATE: %s\nTOTAL_ENTITY_SLOTS: %d\n", state, g.entityelementlist);

	int count = 0;
	int lightCount = 0;
	for (int e = 1; e <= g.entityelementlist && written < resultSize - 512; e++)
	{
		int bankindex = t.entityelement[e].bankindex;
		if (bankindex <= 0) continue;
		if (!t.entityelement[e].active) continue;

		bool isLight = (t.entityprofile[bankindex].ismarker == 2);
		bool isLightMarker = (t.entityprofile[bankindex].islightmarker == 1);
		if (isLight || isLightMarker) lightCount++;

		if (filterLights && !isLight && !isLightMarker) continue;

		count++;

		const char* entityName = t.entityelement[e].eleprof.name_s.Get();
		if (!entityName || !entityName[0]) entityName = "";

		written += _snprintf(result + written, resultSize - written,
			"  [%d] name=\"%.40s\" bank=%d pos=(%.1f,%.1f,%.1f) marker=%d",
			e, entityName, bankindex,
			t.entityelement[e].x, t.entityelement[e].y, t.entityelement[e].z,
			t.entityprofile[bankindex].ismarker);

		if (isLight || isLightMarker)
		{
			DWORD col = t.entityelement[e].eleprof.light.color;
			written += _snprintf(result + written, resultSize - written,
				" LIGHT(range=%d rgb=(%d,%d,%d) islit=%d spot=%d lidx=%d)",
				t.entityelement[e].eleprof.light.range,
				RgbR(col), RgbG(col), RgbB(col),
				t.entityelement[e].eleprof.light.islit,
				t.entityelement[e].eleprof.usespotlighting,
				t.entityelement[e].eleprof.light.index);
		}

		if (written < resultSize - 2)
			result[written++] = '\n';
	}

	if (written < resultSize - 128)
		written += _snprintf(result + written, resultSize - written,
			"LISTED: %d entities (LIGHT_ENTITIES: %d)\n", count, lightCount);

	result[resultSize - 1] = 0;
}

static void Cmd_GetEntity(const char* indexStr, char* result, int resultSize)
{
	if (!indexStr || !indexStr[0])
	{
		_snprintf(result, resultSize, "ERROR: GET_ENTITY requires an entity index argument");
		result[resultSize - 1] = 0;
		return;
	}

	int e = atoi(indexStr);
	if (e < 1 || e > g.entityelementlist)
	{
		_snprintf(result, resultSize, "ERROR: Entity index %d out of range (1-%d)", e, g.entityelementlist);
		result[resultSize - 1] = 0;
		return;
	}

	int written = 0;
	int bankindex = t.entityelement[e].bankindex;

	const char* entityName = t.entityelement[e].eleprof.name_s.Get();
	if (!entityName || !entityName[0]) entityName = "";

	written += _snprintf(result + written, resultSize - written,
		"ENTITY[%d]:\n"
		"  name=\"%s\"\n"
		"  active=%d\n"
		"  bankindex=%d\n"
		"  obj=%d\n"
		"  pos=(%.2f, %.2f, %.2f)\n"
		"  rot=(%.2f, %.2f, %.2f)\n"
		"  scale=(%.2f, %.2f, %.2f)\n"
		"  staticflag=%d\n",
		e, entityName,
		t.entityelement[e].active,
		bankindex,
		t.entityelement[e].obj,
		t.entityelement[e].x, t.entityelement[e].y, t.entityelement[e].z,
		t.entityelement[e].rx, t.entityelement[e].ry, t.entityelement[e].rz,
		t.entityelement[e].scalex, t.entityelement[e].scaley, t.entityelement[e].scalez,
		t.entityelement[e].staticflag);

	if (bankindex > 0 && written < resultSize - 512)
	{
		written += _snprintf(result + written, resultSize - written,
			"  PROFILE:\n"
			"    ismarker=%d\n"
			"    islightmarker=%d\n"
			"    usespotlighting=%d\n"
			"    castshadow=%d\n",
			t.entityprofile[bankindex].ismarker,
			t.entityprofile[bankindex].islightmarker,
			t.entityprofile[bankindex].usespotlighting,
			t.entityprofile[bankindex].castshadow);
	}

	// Light data
	if (written < resultSize - 512)
	{
		DWORD col = t.entityelement[e].eleprof.light.color;
		written += _snprintf(result + written, resultSize - written,
			"  LIGHT_DATA:\n"
			"    index=%d\n"
			"    islit=%d\n"
			"    range=%d\n"
			"    color=0x%08X rgb=(%d,%d,%d)\n"
			"    offsetup=%d\n"
			"    offsetz=%d\n"
			"    usespotlighting=%d\n"
			"    fLightHasProbe=%.1f\n"
			"    fProbeBrightness=%.2f\n",
			t.entityelement[e].eleprof.light.index,
			t.entityelement[e].eleprof.light.islit,
			t.entityelement[e].eleprof.light.range,
			t.entityelement[e].eleprof.light.color,
			RgbR(col), RgbG(col), RgbB(col),
			t.entityelement[e].eleprof.light.offsetup,
			t.entityelement[e].eleprof.light.offsetz,
			t.entityelement[e].eleprof.usespotlighting,
			t.entityelement[e].eleprof.light.fLightHasProbe,
			t.entityelement[e].eleprof.light.fProbeBrightness);
	}

	// If this entity has an infinilight, show it
	int lightIdx = t.entityelement[e].eleprof.light.index;
	if (lightIdx > 0 && lightIdx <= g.infinilightmax && written < resultSize - 512)
	{
		infinilighttype* pLight = &t.infinilight[lightIdx];
		written += _snprintf(result + written, resultSize - written,
			"  INFINILIGHT[%d]:\n"
			"    used=%d islit=%d e=%d\n"
			"    pos=(%.1f,%.1f,%.1f) range=%.0f\n"
			"    rgb=(%d,%d,%d)\n"
			"    spot=%d shadow=%d\n"
			"    wickedlightindex=%llu\n",
			lightIdx,
			pLight->used, pLight->islit, pLight->e,
			pLight->x, pLight->y, pLight->z, pLight->range,
			pLight->colrgb.r, pLight->colrgb.g, pLight->colrgb.b,
			pLight->is_spot_light ? 1 : 0, pLight->bCanShadow ? 1 : 0,
			(unsigned long long)pLight->wickedlightindex);

		// WickedEngine cross-reference
		wi::scene::Scene* pScene = master.masterrenderer.scene;
		if (pScene && pLight->wickedlightindex > 0)
		{
			wi::scene::LightComponent* lightComp = pScene->lights.GetComponent(pLight->wickedlightindex);
			if (lightComp)
			{
				const char* typeName = "UNKNOWN";
				switch (lightComp->GetType())
				{
					case wi::scene::LightComponent::DIRECTIONAL: typeName = "DIRECTIONAL"; break;
					case wi::scene::LightComponent::POINT: typeName = "POINT"; break;
					case wi::scene::LightComponent::SPOT: typeName = "SPOT"; break;
					case wi::scene::LightComponent::RECTANGLE: typeName = "RECTANGLE"; break;
				}
				written += _snprintf(result + written, resultSize - written,
					"  WICKED_LIGHT:\n"
					"    type=%s\n"
					"    color=(%.3f,%.3f,%.3f)\n"
					"    intensity=%.2f\n"
					"    range=%.1f\n"
					"    castShadow=%d\n"
					"    inactive=%d\n"
					"    position=(%.1f,%.1f,%.1f)\n",
					typeName,
					lightComp->color.x, lightComp->color.y, lightComp->color.z,
					lightComp->intensity,
					lightComp->range,
					lightComp->IsCastingShadow() ? 1 : 0,
					lightComp->IsInactive() ? 1 : 0,
					lightComp->position.x, lightComp->position.y, lightComp->position.z);
			}
			else
			{
				written += _snprintf(result + written, resultSize - written,
					"  WICKED_LIGHT: NOT_FOUND (entity %llu missing from scene)\n",
					(unsigned long long)pLight->wickedlightindex);
			}
		}
	}

	result[resultSize - 1] = 0;
}

static void Cmd_ListLights(char* result, int resultSize)
{
	int written = 0;
	const char* state = AutoHarness_GetAppState();

	wi::scene::Scene* pScene = master.masterrenderer.scene;
	int wickedLightCount = pScene ? (int)pScene->lights.GetCount() : 0;

	written += _snprintf(result + written, resultSize - written,
		"STATE: %s\n"
		"INFINILIGHT_MAX: %d\n"
		"WICKED_SCENE_LIGHTS: %d\n"
		"VISIBLE_LIGHTS: %d\n",
		state, g.infinilightmax, wickedLightCount,
		(int)master.masterrenderer.visibility_main.visibleLights.size());

	for (int l = 1; l <= g.infinilightmax && written < resultSize - 512; l++)
	{
		infinilighttype* pLight = &t.infinilight[l];

		written += _snprintf(result + written, resultSize - written,
			"LIGHT[%d]: e=%d used=%d islit=%d pos=(%.1f,%.1f,%.1f) range=%.0f "
			"rgb=(%d,%d,%d) spot=%d shadow=%d wkID=%llu",
			l, pLight->e, pLight->used, pLight->islit,
			pLight->x, pLight->y, pLight->z,
			pLight->range,
			pLight->colrgb.r, pLight->colrgb.g, pLight->colrgb.b,
			pLight->is_spot_light ? 1 : 0,
			pLight->bCanShadow ? 1 : 0,
			(unsigned long long)pLight->wickedlightindex);

		// Cross-reference with WickedEngine
		if (pScene && pLight->wickedlightindex > 0)
		{
			wi::scene::LightComponent* lightComp = pScene->lights.GetComponent(pLight->wickedlightindex);
			if (lightComp)
			{
				const char* typeName = "?";
				switch (lightComp->GetType())
				{
					case wi::scene::LightComponent::DIRECTIONAL: typeName = "DIR"; break;
					case wi::scene::LightComponent::POINT: typeName = "PT"; break;
					case wi::scene::LightComponent::SPOT: typeName = "SP"; break;
					case wi::scene::LightComponent::RECTANGLE: typeName = "RC"; break;
				}
				written += _snprintf(result + written, resultSize - written,
					" W(%s int=%.1f rng=%.1f rgb=(%.2f,%.2f,%.2f) shd=%d%s)",
					typeName,
					lightComp->intensity,
					lightComp->range,
					lightComp->color.x, lightComp->color.y, lightComp->color.z,
					lightComp->IsCastingShadow() ? 1 : 0,
					lightComp->IsInactive() ? " INACTIVE" : "");
			}
			else
			{
				written += _snprintf(result + written, resultSize - written, " W(MISSING!)");
			}
		}
		else if (pLight->wickedlightindex == 0)
		{
			written += _snprintf(result + written, resultSize - written, " W(NONE)");
		}

		if (written < resultSize - 2)
			result[written++] = '\n';
	}

	result[resultSize - 1] = 0;
}

// DUMP_SKIN: full armature/skinning/animation report written to auto_skin.txt (no size cap)
// to diagnose skinned-mesh corruption (the intermittent "exploded parrot" on Island Showdown).
// Order: skinned objects first (SUSPECT flag when AABB is huge), armature summaries,
// per-bone detail for suspect/filter-matched armatures, then one line per animation.
// Optional arg = case-insensitive name filter on skinned-object names (adds those
// armatures to the detail pass even when not suspect). Summary goes to auto_result.txt.
static void Cmd_DumpSkin(const char* arg, char* result, int resultSize)
{
	wi::scene::Scene* pScene = master.masterrenderer.scene;
	if (!pScene)
	{
		_snprintf(result, resultSize, "ERROR: no scene");
		result[resultSize - 1] = 0;
		return;
	}

	// report file lives next to auto_result.txt
	char skinPath[MAX_PATH];
	strncpy(skinPath, s_rspPath, MAX_PATH);
	skinPath[MAX_PATH - 1] = 0;
	char* slash = strrchr(skinPath, '\\');
	if (!slash) slash = strrchr(skinPath, '/');
	if (slash) slash[1] = 0; else skinPath[0] = 0;
	strncat(skinPath, "auto_skin.txt", MAX_PATH - strlen(skinPath) - 1);

	FILE* f = fopen(skinPath, "w");
	if (!f)
	{
		_snprintf(result, resultSize, "ERROR: cannot write %s", skinPath);
		result[resultSize - 1] = 0;
		return;
	}

	// --- Pass 1: skinned objects (mesh has armatureID), collect suspects for detail pass ---
	const float SUSPECT_HALFWIDTH = 5000.0f;
	std::vector<wi::ecs::Entity> detailArmatures;
	int skinnedCount = 0, suspectCount = 0;
	char suspectNames[512] = { 0 };

	for (size_t o = 0; o < pScene->objects.GetCount(); ++o)
	{
		wi::scene::ObjectComponent& obj = pScene->objects[o];
		if (obj.meshID == wi::ecs::INVALID_ENTITY) continue;
		wi::scene::MeshComponent* mesh = pScene->meshes.GetComponent(obj.meshID);
		if (!mesh || mesh->armatureID == wi::ecs::INVALID_ENTITY) continue;
		skinnedCount++;

		wi::ecs::Entity objEntity = pScene->objects.GetEntity(o);
		wi::scene::NameComponent* name = pScene->names.GetComponent(objEntity);
		const char* objName = name ? name->name.c_str() : "?";

		bool armaExists = pScene->armatures.GetComponent(mesh->armatureID) != nullptr;
		const wi::primitive::AABB& aabb = pScene->aabb_objects[o];
		XMFLOAT3 c = aabb.getCenter(), h = aabb.getHalfWidth();

		bool suspect = (h.x > SUSPECT_HALFWIDTH || h.y > SUSPECT_HALFWIDTH || h.z > SUSPECT_HALFWIDTH) && obj.IsRenderable();
		bool filtered = (arg && arg[0] && pestrcasestr(objName, arg));
		if (suspect)
		{
			suspectCount++;
			if (strlen(suspectNames) < sizeof(suspectNames) - 64)
			{
				strncat(suspectNames, objName, 48);
				strncat(suspectNames, " ", 2);
			}
		}
		if ((suspect || filtered) && detailArmatures.size() < 16)
		{
			bool have = false;
			for (auto e : detailArmatures) if (e == mesh->armatureID) have = true;
			if (!have) detailArmatures.push_back(mesh->armatureID);
		}

		fprintf(f, "SKINOBJ ent=%llu name='%s' mesh=%llu arma=%llu%s renderable=%d "
			"aabbC=(%.0f,%.0f,%.0f) aabbH=(%.0f,%.0f,%.0f)%s\n",
			(unsigned long long)objEntity, objName,
			(unsigned long long)obj.meshID, (unsigned long long)mesh->armatureID,
			armaExists ? "" : " ARMA_MISSING!",
			obj.IsRenderable() ? 1 : 0,
			c.x, c.y, c.z, h.x, h.y, h.z,
			suspect ? "  <<< SUSPECT" : "");
	}

	// --- Pass 2: armature summaries ---
	fprintf(f, "ARMATURES: %d\n", (int)pScene->armatures.GetCount());
	for (size_t a = 0; a < pScene->armatures.GetCount(); ++a)
	{
		wi::scene::ArmatureComponent& arma = pScene->armatures[a];
		wi::ecs::Entity armaEntity = pScene->armatures.GetEntity(a);

		int missing = 0, invalid = 0, nans = 0;
		float minT = FLT_MAX, maxT = -FLT_MAX;
		for (wi::ecs::Entity boneEntity : arma.boneCollection)
		{
			if (boneEntity == wi::ecs::INVALID_ENTITY) { invalid++; continue; }
			wi::scene::TransformComponent* bone = pScene->transforms.GetComponent(boneEntity);
			if (!bone) { missing++; continue; }
			float tx = bone->world._41, ty = bone->world._42, tz = bone->world._43;
			if (isnan(tx) || isnan(ty) || isnan(tz)) { nans++; continue; }
			float mag = sqrtf(tx * tx + ty * ty + tz * tz);
			if (mag < minT) minT = mag;
			if (mag > maxT) maxT = mag;
		}

		// boneData = final skin matrices (armature-local); huge translations here = exploded skin
		float minB = FLT_MAX, maxB = -FLT_MAX;
		int nanB = 0;
		for (const auto& bd : arma.boneData)
		{
			float tx = bd.mat0.w, ty = bd.mat1.w, tz = bd.mat2.w;
			if (isnan(tx) || isnan(ty) || isnan(tz)) { nanB++; continue; }
			float mag = sqrtf(tx * tx + ty * ty + tz * tz);
			if (mag < minB) minB = mag;
			if (mag > maxB) maxB = mag;
		}

		wi::scene::TransformComponent* armaT = pScene->transforms.GetComponent(armaEntity);
		fprintf(f, "ARM[%d] ent=%llu bones=%d invBind=%d boneData=%d miss=%d inval=%d nan=%d "
			"|boneW|=%.1f..%.1f |skinT|=%.1f..%.1f nanSkin=%d armaPos=(%.1f,%.1f,%.1f)\n",
			(int)a, (unsigned long long)armaEntity,
			(int)arma.boneCollection.size(), (int)arma.inverseBindMatrices.size(), (int)arma.boneData.size(),
			missing, invalid, nans,
			minT == FLT_MAX ? 0.0f : minT, maxT == -FLT_MAX ? 0.0f : maxT,
			minB == FLT_MAX ? 0.0f : minB, maxB == -FLT_MAX ? 0.0f : maxB, nanB,
			armaT ? armaT->world._41 : 0.0f, armaT ? armaT->world._42 : 0.0f, armaT ? armaT->world._43 : 0.0f);
	}

	// --- Pass 3: per-bone detail for suspect/filtered armatures ---
	for (wi::ecs::Entity armaEntity : detailArmatures)
	{
		wi::scene::ArmatureComponent* arma = pScene->armatures.GetComponent(armaEntity);
		if (!arma) { fprintf(f, "DETAIL arma=%llu MISSING\n", (unsigned long long)armaEntity); continue; }

		wi::scene::TransformComponent* armaT = pScene->transforms.GetComponent(armaEntity);
		wi::scene::HierarchyComponent* armaH = pScene->hierarchy.GetComponent(armaEntity);
		fprintf(f, "DETAIL arma=%llu parent=%llu world=(%.1f,%.1f,%.1f) bones=%d\n",
			(unsigned long long)armaEntity,
			(unsigned long long)(armaH ? armaH->parentID : 0),
			armaT ? armaT->world._41 : 0.0f, armaT ? armaT->world._42 : 0.0f, armaT ? armaT->world._43 : 0.0f,
			(int)arma->boneCollection.size());

		for (size_t b = 0; b < arma->boneCollection.size(); ++b)
		{
			wi::ecs::Entity boneEntity = arma->boneCollection[b];
			wi::scene::TransformComponent* bone = pScene->transforms.GetComponent(boneEntity);
			wi::scene::NameComponent* bname = boneEntity ? pScene->names.GetComponent(boneEntity) : nullptr;
			wi::scene::HierarchyComponent* bh = boneEntity ? pScene->hierarchy.GetComponent(boneEntity) : nullptr;

			float ibx = 0, iby = 0, ibz = 0;
			if (b < arma->inverseBindMatrices.size())
			{
				ibx = arma->inverseBindMatrices[b]._41; iby = arma->inverseBindMatrices[b]._42; ibz = arma->inverseBindMatrices[b]._43;
			}
			float sx = 0, sy = 0, sz = 0;
			if (b < arma->boneData.size())
			{
				sx = arma->boneData[b].mat0.w; sy = arma->boneData[b].mat1.w; sz = arma->boneData[b].mat2.w;
			}

			if (bone)
			{
				fprintf(f, "  B[%d] ent=%llu '%s' parent=%llu W=(%.1f,%.1f,%.1f) L=(%.1f,%.1f,%.1f) "
					"LR=(%.2f,%.2f,%.2f,%.2f) LS=(%.2f,%.2f,%.2f) IB=(%.1f,%.1f,%.1f) SK=(%.1f,%.1f,%.1f)\n",
					(int)b, (unsigned long long)boneEntity, bname ? bname->name.c_str() : "?",
					(unsigned long long)(bh ? bh->parentID : 0),
					bone->world._41, bone->world._42, bone->world._43,
					bone->translation_local.x, bone->translation_local.y, bone->translation_local.z,
					bone->rotation_local.x, bone->rotation_local.y, bone->rotation_local.z, bone->rotation_local.w,
					bone->scale_local.x, bone->scale_local.y, bone->scale_local.z,
					ibx, iby, ibz, sx, sy, sz);
			}
			else
			{
				fprintf(f, "  B[%d] ent=%llu '%s' NO_TRANSFORM IB=(%.1f,%.1f,%.1f) SK=(%.1f,%.1f,%.1f)\n",
					(int)b, (unsigned long long)boneEntity, bname ? bname->name.c_str() : "?",
					ibx, iby, ibz, sx, sy, sz);
			}
		}
	}

	// --- Pass 4: animations, compact line each ---
	fprintf(f, "ANIMATIONS: %d\n", (int)pScene->animations.GetCount());
	for (size_t i = 0; i < pScene->animations.GetCount(); ++i)
	{
		wi::scene::AnimationComponent& anim = pScene->animations[i];
		wi::ecs::Entity animEntity = pScene->animations.GetEntity(i);
		wi::scene::NameComponent* name = pScene->names.GetComponent(animEntity);

		int chBadTarget = 0, chNoData = 0;
		for (const auto& ch : anim.channels)
		{
			if (ch.target == wi::ecs::INVALID_ENTITY || !pScene->transforms.GetComponent(ch.target)) chBadTarget++;
			if (ch.samplerIndex < (int)anim.samplers.size())
			{
				if (!pScene->animation_datas.GetComponent(anim.samplers[ch.samplerIndex].data)) chNoData++;
			}
		}

		// first valid channel target tells us which hierarchy this anim drives
		wi::ecs::Entity firstTarget = anim.channels.empty() ? 0 : anim.channels[0].target;

		wi::scene::ObjectComponent* cullObj = pScene->objects.GetComponent(anim.objectIndex);
		fprintf(f, "ANIM[%d] ent=%llu '%s' play=%d loop=%d t=%.2f s=%.1f e=%.1f amt=%.2f spd=%.1f "
			"cullObj=%llu(%s) ch=%d badTgt=%d noData=%d tgt0=%llu\n",
			(int)i, (unsigned long long)animEntity, name ? name->name.c_str() : "?",
			anim.IsPlaying() ? 1 : 0, anim.IsLooped() ? 1 : 0,
			anim.timer, anim.start, anim.end, anim.amount, anim.speed,
			(unsigned long long)anim.objectIndex,
			cullObj ? (cullObj->IsRenderable() ? "vis" : "HIDDEN") : "none",
			(int)anim.channels.size(), chBadTarget, chNoData,
			(unsigned long long)firstTarget);
	}

	// --- Pass 5: animation keyframe data garbage scan ---
	// If garbage lives HERE the corruption is baked at load (keyframe build);
	// if all data is clean yet bone locals are garbage, something writes transforms directly.
	int dataBad = 0;
	fprintf(f, "ANIMDATA: %d\n", (int)pScene->animation_datas.GetCount());
	for (size_t i = 0; i < pScene->animation_datas.GetCount(); ++i)
	{
		auto& ad = pScene->animation_datas[i];
		float maxAbs = 0.0f;
		int bad = 0, firstBad = -1;
		for (size_t k = 0; k < ad.keyframe_data.size(); ++k)
		{
			float v = ad.keyframe_data[k];
			if (isnan(v) || fabsf(v) > 1.0e6f)
			{
				bad++;
				if (firstBad < 0) firstBad = (int)k;
			}
			else if (fabsf(v) > maxAbs) maxAbs = v < 0 ? -v : v;
		}
		float maxTime = ad.keyframe_times.empty() ? 0.0f : ad.keyframe_times.back();
		if (bad > 0)
		{
			dataBad++;
			fprintf(f, "ANIMDATA[%d] ent=%llu keys=%d data=%d maxT=%.0f BAD=%d firstBad=%d sample=(%g,%g,%g,%g)\n",
				(int)i, (unsigned long long)pScene->animation_datas.GetEntity(i),
				(int)ad.keyframe_times.size(), (int)ad.keyframe_data.size(), maxTime, bad, firstBad,
				firstBad >= 0 && firstBad + 3 < (int)ad.keyframe_data.size() ? ad.keyframe_data[firstBad] : 0.0f,
				firstBad >= 0 && firstBad + 3 < (int)ad.keyframe_data.size() ? ad.keyframe_data[firstBad + 1] : 0.0f,
				firstBad >= 0 && firstBad + 3 < (int)ad.keyframe_data.size() ? ad.keyframe_data[firstBad + 2] : 0.0f,
				firstBad >= 0 && firstBad + 3 < (int)ad.keyframe_data.size() ? ad.keyframe_data[firstBad + 3] : 0.0f);
		}
	}
	fprintf(f, "ANIMDATA_BAD_TOTAL: %d\n", dataBad);

	fclose(f);

	_snprintf(result, resultSize,
		"OK: DUMP_SKIN wrote %s\nSKINNED_OBJECTS: %d  ARMATURES: %d  ANIMATIONS: %d  BAD_ANIMDATA: %d\nSUSPECTS: %d %s",
		skinPath, skinnedCount, (int)pScene->armatures.GetCount(), (int)pScene->animations.GetCount(),
		dataBad, suspectCount, suspectNames);
	result[resultSize - 1] = 0;
}

// ---- SKIN_WATCH: per-frame scan for garbage bone rotations (parrot corruption hunt) ----
// Enabled via the SKIN_WATCH command BEFORE loading a level. Each frame, scans every
// TransformComponent for a garbage rotation_local (|component| > 1e3 or NaN) and logs
// the FIRST detection per entity with a frame counter and scene component counts, so the
// load phase where the write lands can be identified. Output: auto_skinwatch.txt.
static bool s_bSkinWatch = false;
static uint32_t s_skinWatchFrame = 0;
static FILE* s_skinWatchFile = nullptr;
static std::vector<uint64_t> s_skinWatchSeen;

static void AutoHarness_SkinWatchTick(void)
{
	if (!s_bSkinWatch) return;
	s_skinWatchFrame++;
	wi::scene::Scene* pScene = master.masterrenderer.scene;
	if (!pScene) return;

	if (!s_skinWatchFile)
	{
		char path[MAX_PATH];
		strncpy(path, s_rspPath, MAX_PATH);
		path[MAX_PATH - 1] = 0;
		char* slash = strrchr(path, '\\');
		if (!slash) slash = strrchr(path, '/');
		if (slash) slash[1] = 0; else path[0] = 0;
		strncat(path, "auto_skinwatch.txt", MAX_PATH - strlen(path) - 1);
		s_skinWatchFile = fopen(path, "w");
		if (!s_skinWatchFile) { s_bSkinWatch = false; return; }
	}

	int newHits = 0;
	for (size_t i = 0; i < pScene->transforms.GetCount(); ++i)
	{
		wi::scene::TransformComponent& tr = pScene->transforms[i];
		float rx = tr.rotation_local.x, ry = tr.rotation_local.y, rz = tr.rotation_local.z, rw = tr.rotation_local.w;
		bool bad = isnan(rx) || isnan(ry) || isnan(rz) || isnan(rw) ||
			fabsf(rx) > 1000.0f || fabsf(ry) > 1000.0f || fabsf(rz) > 1000.0f || fabsf(rw) > 1000.0f;
		if (!bad) continue;

		uint64_t entity = (uint64_t)pScene->transforms.GetEntity(i);
		bool seen = false;
		for (uint64_t e : s_skinWatchSeen) if (e == entity) { seen = true; break; }
		if (seen) continue;
		s_skinWatchSeen.push_back(entity);
		newHits++;

		wi::scene::NameComponent* name = pScene->names.GetComponent((wi::ecs::Entity)entity);
		fprintf(s_skinWatchFile, "HIT frame=%u ent=%llu '%s' LR=(%g,%g,%g,%g) LS=(%g,%g,%g) LT=(%g,%g,%g) "
			"[transforms=%d objects=%d armatures=%d anims=%d animdatas=%d]\n",
			s_skinWatchFrame, (unsigned long long)entity, name ? name->name.c_str() : "?",
			rx, ry, rz, rw,
			tr.scale_local.x, tr.scale_local.y, tr.scale_local.z,
			tr.translation_local.x, tr.translation_local.y, tr.translation_local.z,
			(int)pScene->transforms.GetCount(), (int)pScene->objects.GetCount(),
			(int)pScene->armatures.GetCount(), (int)pScene->animations.GetCount(),
			(int)pScene->animation_datas.GetCount());
	}

	if (newHits > 0 || (s_skinWatchFrame % 300) == 0)
	{
		fprintf(s_skinWatchFile, "TICK frame=%u hits=%d totalSeen=%d transforms=%d objects=%d armatures=%d anims=%d\n",
			s_skinWatchFrame, newHits, (int)s_skinWatchSeen.size(),
			(int)pScene->transforms.GetCount(), (int)pScene->objects.GetCount(),
			(int)pScene->armatures.GetCount(), (int)pScene->animations.GetCount());
		fflush(s_skinWatchFile);
	}
}

// ---- SCULPT_TEST / PAINT_TEST: synthetic terrain-edit strokes ----
// Drives the same GGTerrain_Update_Sculpting/Painting apply functions the editor
// dispatch calls, so the edit -> InvalidateRegion -> Wicked chunk regen/repaint
// chain is exercised without real mouse input. One brush tick per frame.
namespace GGTerrain {
	extern GGTerrainInternalParams ggterrain_internal_params;
	extern int ggterrain_initialised;
	void GGTerrain_Update_Sculpting(float pickX, float pickY, float pickZ);
	void GGTerrain_Update_Painting(float pickX, float pickY, float pickZ);
	void GGTerrain_TriggerPaintTextureLoad(void);
}
void GGTerrain_CreateUndoRedoAction(int type, int eList, bool bUserAction = true, void* pEventData = nullptr);
extern int g_iCalculatingChangeBounds;

// terrain tests only make sense in the level editor with the terrain system live
static bool AutoHarness_TerrainEditAllowed(void)
{
	if (bWelcomeScreen_Window || bStoryboardWindow || bImGuiInTestGame) return false;
	if (!GGTerrain::ggterrain_initialised) return false;
	return true;
}

static int   s_terrainTestFrames = 0;
static int   s_terrainTestPaint = 0;       // 0 = sculpt run, 1 = paint run
static int   s_terrainTestSculptMode = 0;  // GGTERRAIN_SCULPT_* for sculpt runs
static float s_terrainTestX = 0.0f, s_terrainTestZ = 0.0f;
static int   s_terrainTestMaterial = 0;
static int   s_terrainTestSavedEditMode = 0;
static int   s_terrainTestSavedSculptMode = 0;
static int   s_terrainTestSavedPaintMat = 0;

static void AutoHarness_TerrainEditTick(void)
{
	using namespace GGTerrain;
	if (s_terrainTestFrames <= 0) return;
	if (!AutoHarness_TerrainEditAllowed())
	{
		// state changed under us (test game started, level closed) — cut the run short;
		// fall through with no more apply calls so the end-of-run block below still
		// finalizes any armed undo and hands the editor its modes back
		s_terrainTestFrames = 0;
	}
	else
	{
	s_terrainTestFrames--;

	float y = 0.0f;
	GGTerrain_GetHeight(s_terrainTestX, s_terrainTestZ, &y);

	if (s_terrainTestPaint == 0)
	{
		ggterrain_extra_params.edit_mode = GGTERRAIN_EDIT_SCULPT;
		ggterrain_extra_params.sculpt_mode = s_terrainTestSculptMode;
		ggterrain_internal_params.mouseLeftState = 1;   // consumed inside the apply call
		GGTerrain_Update_Sculpting(s_terrainTestX, y, s_terrainTestZ);
	}
	else
	{
		ggterrain_extra_params.edit_mode = GGTERRAIN_EDIT_PAINT;
		ggterrain_extra_params.paint_material = s_terrainTestMaterial;
		ggterrain_internal_params.mouseLeftState = 1;
		// run the same first-use texture-load trigger as the real editor dispatch —
		// skipping it is how the first-stroke texture-load glitch escaped testing
		GGTerrain_TriggerPaintTextureLoad();
		GGTerrain_Update_Painting(s_terrainTestX, y, s_terrainTestZ);
	}
	}

	if (s_terrainTestFrames <= 0)
	{
		// stroke over: finalize the undo action the way the editor release path does,
		// then hand the editor its modes back (skip the undo if the terrain went away
		// under an abandoned run — the snapshot buffers may be gone with it)
		if (g_iCalculatingChangeBounds && ggterrain_initialised)
		{
			g_iCalculatingChangeBounds = 0;
			GGTerrain_CreateUndoRedoAction(s_terrainTestPaint == 0 ? eUndoSys_Terrain_Sculpt : eUndoSys_Terrain_Paint, eUndoSys_UndoList);
		}
		ggterrain_internal_params.mouseLeftState = 0;
		ggterrain_extra_params.edit_mode = s_terrainTestSavedEditMode;
		ggterrain_extra_params.sculpt_mode = s_terrainTestSavedSculptMode;
		ggterrain_extra_params.paint_material = s_terrainTestSavedPaintMat;
	}
}

// ---- Main entry point (called once per tick) ----

void AutoHarness_CheckForCommand(void)
{
	AutoHarness_SkinWatchTick();
	AutoHarness_TerrainEditTick();
	// Init paths and uptime tracking on first call
	if (!s_initialized)
	{
		AutoHarness_InitPaths();
		s_startTick = GetTickCount();
		s_initialized = true;
	}

	// Process deferred key-up: send WM_KEYUP after key was held down for enough frames
	if (s_pendingKeyUpVK != 0 && g_pGlob && g_pGlob->hWnd)
	{
		s_pendingKeyUpFrames--;
		if (s_pendingKeyUpFrames <= 0)
		{
			UINT scanCode = MapVirtualKey(s_pendingKeyUpVK, MAPVK_VK_TO_VSC);
			LPARAM lParamUp = (1) | (scanCode << 16) | (1 << 30) | (1 << 31);
			PostMessage(g_pGlob->hWnd, WM_KEYUP, (WPARAM)s_pendingKeyUpVK, lParamUp);
			if (s_pendingKeyUpVK < 256)
				ImGui::GetIO().KeysDown[s_pendingKeyUpVK] = false;
			s_pendingKeyUpVK = 0;
		}
	}

	// Fast check: does the command file exist?
	DWORD attr = GetFileAttributesA(s_cmdPath);
	if (attr == INVALID_FILE_ATTRIBUTES)
		return; // No file, return immediately (zero overhead path)

	// Read the command file
	char cmdBuf[1024];
	cmdBuf[0] = 0;
	FILE* f = fopen(s_cmdPath, "r");
	if (!f) return;
	if (!fgets(cmdBuf, sizeof(cmdBuf), f))
		cmdBuf[0] = 0;
	fclose(f);

	// Trim trailing newline/carriage return
	int len = (int)strlen(cmdBuf);
	while (len > 0 && (cmdBuf[len - 1] == '\n' || cmdBuf[len - 1] == '\r'))
		cmdBuf[--len] = 0;

	// If file was empty, don't delete it — writer may still be flushing (race condition).
	// We'll pick it up on the next tick.
	if (len == 0) return;

	// Delete the command file now that we have valid content
	DeleteFileA(s_cmdPath);

	// Log the command
	AutoHarness_LogCommand(cmdBuf);

	// Mark harness as active so main loop keeps running even when window loses focus
	g_bAutomationActive = true;

	// Parse command and argument
	char cmd[256];
	char arg[768];
	cmd[0] = 0;
	arg[0] = 0;

	// Find first space
	char* space = strchr(cmdBuf, ' ');
	if (space)
	{
		int cmdLen = (int)(space - cmdBuf);
		if (cmdLen >= (int)sizeof(cmd)) cmdLen = (int)sizeof(cmd) - 1;
		memcpy(cmd, cmdBuf, cmdLen);
		cmd[cmdLen] = 0;

		// Skip spaces
		const char* a = space + 1;
		while (*a == ' ') a++;
		strncpy(arg, a, sizeof(arg) - 1);
		arg[sizeof(arg) - 1] = 0;
	}
	else
	{
		strncpy(cmd, cmdBuf, sizeof(cmd) - 1);
		cmd[sizeof(cmd) - 1] = 0;
	}

	// Dispatch command (32KB buffer for entity/light listing commands)
	char result[32768];
	result[0] = 0;

	if (_stricmp(cmd, "GET_STATE") == 0)
	{
		Cmd_GetState(result, sizeof(result));
	}
	else if (_stricmp(cmd, "GET_SCREEN_TEXT") == 0)
	{
		Cmd_GetScreenText(result, sizeof(result));
	}
	else if (_stricmp(cmd, "NAVIGATE") == 0)
	{
		Cmd_Navigate(arg, result, sizeof(result));
	}
	else if (_stricmp(cmd, "CLICK") == 0)
	{
		Cmd_Click(arg, result, sizeof(result));
	}
	else if (_stricmp(cmd, "CLICK_NODE") == 0)
	{
		Cmd_ClickNode(arg, result, sizeof(result));
	}
	else if (_stricmp(cmd, "WAIT") == 0)
	{
		Cmd_Wait(arg, result, sizeof(result));
	}
	else if (_stricmp(cmd, "SCREENSHOT") == 0)
	{
		Cmd_Screenshot(result, sizeof(result));
	}
	else if (_stricmp(cmd, "LIST_DEMOS") == 0)
	{
		Cmd_ListDemos(result, sizeof(result));
	}
	else if (_stricmp(cmd, "SELECT_DEMO") == 0)
	{
		Cmd_SelectDemo(arg, result, sizeof(result));
	}
	else if (_stricmp(cmd, "LIST_PROJECTS") == 0)
	{
		Cmd_ListProjects(result, sizeof(result));
	}
	else if (_stricmp(cmd, "OPEN_PROJECT") == 0)
	{
		Cmd_OpenProject(arg, result, sizeof(result));
	}
	else if (_stricmp(cmd, "CLICK_ONLY_LEVEL") == 0)
	{
		Cmd_ClickOnlyLevel(result, sizeof(result));
	}
	else if (_stricmp(cmd, "GET_PERF_DATA") == 0)
	{
		Cmd_GetPerfData(result, sizeof(result));
	}
	else if (_stricmp(cmd, "TOGGLE_PROFILER") == 0)
	{
		Cmd_ToggleProfiler(result, sizeof(result));
	}
	else if (_stricmp(cmd, "PRESS_ESCAPE") == 0)
	{
		Cmd_PressEscape(result, sizeof(result));
	}
	else if (_stricmp(cmd, "PRESS_KEY") == 0)
	{
		Cmd_PressKey(arg, result, sizeof(result));
	}
	else if (_stricmp(cmd, "SET_CAMERA") == 0)
	{
		// SET_CAMERA <x> <y> <z> [angx angy] — relocate the editor free-flight camera (same state
		// the editor's own "travel to" writes). Used to force VT/terrain chunk re-streaming so the
		// shadow-flicker (chunk-recycle) trigger can be reproduced on demand. Editor mode only.
		float cxp = 0, cyp = 0, czp = 0, cax = 0, cay = 0;
		int got = sscanf_s(arg, "%f %f %f %f %f", &cxp, &cyp, &czp, &cax, &cay);
		const char* state = AutoHarness_GetAppState();
		if (strcmp(state, "editor") != 0)
		{
			_snprintf(result, sizeof(result), "ERROR: SET_CAMERA only works in the level editor (state: %s)", state);
		}
		else if (got < 3)
		{
			_snprintf(result, sizeof(result), "ERROR: SET_CAMERA needs <x> <y> <z> [angx angy]");
		}
		else
		{
			t.editorfreeflight.mode = 1; // free-flight so the fields below are applied each frame
			t.editorfreeflight.c.x_f = cxp;
			t.editorfreeflight.c.y_f = cyp;
			t.editorfreeflight.c.z_f = czp;
			if (got >= 5) { t.editorfreeflight.c.angx_f = cax; t.editorfreeflight.c.angy_f = cay; }
			t.cx_f = t.editorfreeflight.c.x_f;
			t.cy_f = t.editorfreeflight.c.z_f;
			_snprintf(result, sizeof(result), "OK: SET_CAMERA pos=(%.1f,%.1f,%.1f) ang=(%.1f,%.1f)",
				cxp, cyp, czp, t.editorfreeflight.c.angx_f, t.editorfreeflight.c.angy_f);
		}
		result[sizeof(result) - 1] = 0;
	}
	else if (_stricmp(cmd, "MOVE_CAMERA") == 0)
	{
		// MOVE_CAMERA <dx> <dy> <dz> — offset the editor free-flight camera (force chunk re-stream).
		float dxp = 0, dyp = 0, dzp = 0;
		const char* state = AutoHarness_GetAppState();
		if (strcmp(state, "editor") != 0)
		{
			_snprintf(result, sizeof(result), "ERROR: MOVE_CAMERA only works in the level editor (state: %s)", state);
		}
		else if (sscanf_s(arg, "%f %f %f", &dxp, &dyp, &dzp) != 3)
		{
			_snprintf(result, sizeof(result), "ERROR: MOVE_CAMERA needs <dx> <dy> <dz>");
		}
		else
		{
			t.editorfreeflight.mode = 1;
			t.editorfreeflight.c.x_f += dxp;
			t.editorfreeflight.c.y_f += dyp;
			t.editorfreeflight.c.z_f += dzp;
			t.cx_f = t.editorfreeflight.c.x_f;
			t.cy_f = t.editorfreeflight.c.z_f;
			_snprintf(result, sizeof(result), "OK: MOVE_CAMERA -> (%.1f,%.1f,%.1f)",
				t.editorfreeflight.c.x_f, t.editorfreeflight.c.y_f, t.editorfreeflight.c.z_f);
		}
		result[sizeof(result) - 1] = 0;
	}
	else if (_stricmp(cmd, "SET_GRASS") == 0)
	{
		char gp[64] = { 0 }; float gv = 0.0f;
		if (sscanf_s(arg, "%63s %f", gp, (unsigned)sizeof(gp), &gv) == 2)
		{
			extern void GGTerrainWicked_SetGrassParam(const char* param, float value);
			GGTerrainWicked_SetGrassParam(gp, gv);
			_snprintf(result, sizeof(result), "OK: SET_GRASS %s = %.3f", gp, gv);
		}
		else
		{
			_snprintf(result, sizeof(result), "ERROR: SET_GRASS needs <param> <value> (length|width|stiffness|drag|blades|maxstrands|segments|billboards|viewdist|sss|alpha|tintr|tintg|tintb|sssr|sssg|sssb)");
		}
		result[sizeof(result) - 1] = 0;
	}
	else if (_stricmp(cmd, "SET_SHADOW_MAX") == 0)
	{
		// Drive the local point-shadow cap for testing the Phase 1/2 budget+cache without the UI. The
		// game's shadow-props recompute only runs on a visuals-apply, so force the engine budget directly.
		int n = atoi(arg);
		if (n < 0) n = 0;
		t.visuals.iShadowPointMax = n;
		t.gamevisuals.iShadowPointMax = n;
		wi::renderer::SetLocalShadowBudget(n); // immediate: GRANTED = min(visible local casters, n)
		_snprintf(result, sizeof(result), "OK: SET_SHADOW_MAX engine budget forced to %d", n);
		result[sizeof(result) - 1] = 0;
	}
	else if (_stricmp(cmd, "SET_SHADOW_CACHE") == 0)
	{
		// A/B toggle for the Phase 2 static-cache (forces the engine flag directly).
		bool on = (arg[0] != '0');
		wi::renderer::SetLocalShadowCachingEnabled(on);
		_snprintf(result, sizeof(result), "OK: SET_SHADOW_CACHE %s", on ? "ON" : "OFF");
		result[sizeof(result) - 1] = 0;
	}
	else if (_stricmp(cmd, "SET_SHADOW_FARCULL") == 0)
	{
		// A/B toggle for the far-cascade caster cull (DX11 parity). On by default.
		bool on = (arg[0] != '0');
		wi::renderer::SetShadowFarCascadeCull(on);
		_snprintf(result, sizeof(result), "OK: SET_SHADOW_FARCULL %s", on ? "ON" : "OFF");
		result[sizeof(result) - 1] = 0;
	}
	else if (_stricmp(cmd, "SET_REFLECTIONS") == 0)
	{
		// A/B lever for the water reflection stabilisation fix. Toggles planar reflections exactly like the
		// editor "Reflections" checkbox (M-GridEditB_part23). With the engine guard (wiRenderPath3D PreRender),
		// OFF forces texture_reflection_index=-1 so the ocean PS takes its EnvironmentReflection_Global
		// fallback (reflect the sky/global probe) instead of sampling the stale reflection texture = garbage.
		extern MasterRenderer * master_renderer;
		bool on = (arg[0] != '0');
		t.visuals.bReflectionsEnabled = on;
		t.gamevisuals.bReflectionsEnabled = on;
		if (master_renderer) master_renderer->setReflectionsEnabled(on);
		_snprintf(result, sizeof(result), "OK: SET_REFLECTIONS %s", on ? "ON" : "OFF");
		result[sizeof(result) - 1] = 0;
	}
	else if (_stricmp(cmd, "SET_RESSCALE") == 0)
	{
		// GPU lever: render the 3D scene at a fraction of native resolution. RenderPath2D::Update
		// auto-detects the resolutionScale change and ResizeBuffers() next frame (safe). 1.0 = native;
		// lower = less GPU pixel work (Z-prepass/opaque/postproc all scale), softer image (FSR can upscale).
		extern MasterRenderer * master_renderer;
		float s = (float)atof(arg);
		if (s < 0.4f) s = 0.4f;
		if (s > 1.0f) s = 1.0f;
		if (master_renderer) master_renderer->resolutionScale = s;
		_snprintf(result, sizeof(result), "OK: SET_RESSCALE %.3f (auto-resizes next frame)", s);
		result[sizeof(result) - 1] = 0;
	}
	else if (_stricmp(cmd, "SET_FSR") == 0)
	{
		// A/B the FSR1 spatial-upscale modes without the ImGui panel (mirrors the Graphics & Performance
		// combo exactly: resolutionScale -> setFSREnabled -> ResizeBuffers). 0=None 1=UltraQuality 2=Quality
		// 3=Balanced 4=Performance. Reports internal vs physical res == direct proof the 3D renders smaller;
		// pair with ENABLE_PROFILER + GET_PERF_DATA to confirm the 'FSR' GPU range appears (Postprocess_FSR).
		extern MasterRenderer * master_renderer;
		int mode = atoi(arg);
		if (mode < 0) mode = 0;
		if (mode > 4) mode = 4;
		const float scaleTab[5] = { 1.0f, 1.0f / 1.3f, 1.0f / 1.5f, 1.0f / 1.7f, 1.0f / 2.0f };
		if (master_renderer)
		{
			master_renderer->resolutionScale = scaleTab[mode];
			master_renderer->setFSREnabled(mode != 0);
			master_renderer->ResizeBuffers();
			_snprintf(result, sizeof(result),
				"OK: SET_FSR mode=%d scale=%.3f fsrEnabled=%d internal=%dx%d physical=%dx%d",
				mode, scaleTab[mode], master_renderer->getFSREnabled() ? 1 : 0,
				(int)master_renderer->GetInternalResolution().x, (int)master_renderer->GetInternalResolution().y,
				(int)master_renderer->GetPhysicalWidth(), (int)master_renderer->GetPhysicalHeight());
		}
		else
		{
			_snprintf(result, sizeof(result), "ERROR: no master_renderer");
		}
		result[sizeof(result) - 1] = 0;
	}
	else if (_stricmp(cmd, "SET_FSRSHARP") == 0)
	{
		// Diagnostic for the panel "FSR Sharpness" slider. Value is FFX RCAS *stops*: 0.0 = MAX sharpening
		// (linear exp2(0)=1.0), higher = softer (2.0 -> 1/4). RCAS always runs; this only scales strength,
		// and it's inverted vs intuition. Only visible while an FSR mode is active (rtFSR valid + fsrEnabled).
		extern MasterRenderer * master_renderer;
		float sh = (float)atof(arg);
		if (sh < 0.0f) sh = 0.0f;
		if (master_renderer)
		{
			master_renderer->setFSRSharpness(sh);
			_snprintf(result, sizeof(result), "OK: SET_FSRSHARP %.3f (RCAS stops; 0=sharpest, higher=softer) fsrEnabled=%d",
				sh, master_renderer->getFSREnabled() ? 1 : 0);
		}
		else
		{
			_snprintf(result, sizeof(result), "ERROR: no master_renderer");
		}
		result[sizeof(result) - 1] = 0;
	}
	else if (_stricmp(cmd, "SET_ANIM30FPS") == 0)
	{
		// A/B the "Lower Animation & LUA Speed" 30fps ANIMATION throttle (delta 1.29). The checkbox global
		// is mirrored into the engine each frame by MasterRenderer::Update. Args: <0|1 enable> [farDist].
		// farDist = scene-units distance beyond which on-screen armatures throttle to ~30fps; 0 = throttle
		// ALL eligible ((b)); >0 = keep near-camera armatures full-rate ((c) distance gate). Omit farDist to keep it.
		extern bool bEnable30FpsAnimations;
		extern float g_animThrottleFarDist;
		int en = -1; float fd = -1.0f;
		int n = sscanf(arg, "%d %f", &en, &fd);
		if (n >= 1 && (en == 0 || en == 1)) bEnable30FpsAnimations = (en == 1);
		if (n >= 2 && fd >= 0.0f) g_animThrottleFarDist = fd;
		_snprintf(result, sizeof(result), "OK: SET_ANIM30FPS enabled=%d farDist=%.1f (0=throttle all, >0=only beyond)",
			bEnable30FpsAnimations ? 1 : 0, g_animThrottleFarDist);
		result[sizeof(result) - 1] = 0;
	}
	else if (_stricmp(cmd, "SET_APPARENTSIZE") == 0)
	{
		// A/B the apparent-size object cull (delta 1.30). Objects whose world bounding radius / camera
		// distance falls below the threshold are dropped from the main visible set. Two forms:
		//   SET_APPARENTSIZE <tangent>          DIRECT override of the tangent (radius/dist cutoff); <0 = restore slider.
		//   SET_APPARENTSIZE slider <fASize>    exercise the REAL editor-slider path: sets maxApparentSize = fASize/10000
		//                                       (fASize is the 0.02..2.0 UI value) and clears the direct override, so the
		//                                       game maps it through g_apparentCullK exactly as moving the slider would.
		// NOTE: visible/total counts reflect the PRIOR frame (runs before this frame's UpdateVisibility) — screenshot
		// or re-query after a couple of frames to see the effect.
		extern float g_apparentCullDirect;
		extern float g_apparentCullK;
		extern float maxApparentSize;
		char sub[64] = { 0 };
		float v = 0.0f;
		if (sscanf(arg, "%63s %f", sub, &v) == 2 && _stricmp(sub, "slider") == 0)
		{
			// clamp like the editor slider + M-GridEditB_part3 load guard
			float fA = v; if (fA < 0.02f) fA = 0.02f; if (fA > 2.0f) fA = 2.0f;
			maxApparentSize = fA / 10000.0f;
			g_apparentCullDirect = -1.0f;
			float over = maxApparentSize - 0.000008f;
			float tangent = (over > 0.0f) ? over * g_apparentCullK : 0.0f;
			_snprintf(result, sizeof(result),
				"OK: SET_APPARENTSIZE slider fASize=%.3f -> maxApparentSize=%.7f -> tangent=%.5f (K=%.0f)  visible=%d/%d (prior frame)",
				fA, maxApparentSize, tangent, g_apparentCullK,
				(int)master.masterrenderer.visibility_main.visibleObjects.size(),
				master.masterrenderer.scene ? (int)master.masterrenderer.scene->objects.GetCount() : 0);
		}
		else
		{
			float tg = -1.0f;
			int n = sscanf(arg, "%f", &tg);
			if (n >= 1) g_apparentCullDirect = tg;
			_snprintf(result, sizeof(result),
				"OK: SET_APPARENTSIZE tangent=%.5f (%s)  visible=%d/%d (prior frame)",
				g_apparentCullDirect,
				(g_apparentCullDirect >= 0.0f) ? "forced override" : "slider mapping restored",
				(int)master.masterrenderer.visibility_main.visibleObjects.size(),
				master.masterrenderer.scene ? (int)master.masterrenderer.scene->objects.GetCount() : 0);
		}
		result[sizeof(result) - 1] = 0;
	}
	else if (_stricmp(cmd, "SAVE_LEVEL") == 0)
	{
		// Repro lever for the save-crash: runs the same File>Save path (gridedit_save_map).
		// If it crashes, this result is never written and auto_command.txt is never consumed.
		extern void gridedit_save_map(void);
		const char* state = AutoHarness_GetAppState();
		if (strcmp(state, "editor") != 0)
		{
			_snprintf(result, sizeof(result), "ERROR: SAVE_LEVEL only works in the editor (state: %s)", state);
		}
		else
		{
			gridedit_save_map();
			_snprintf(result, sizeof(result), "OK: SAVE_LEVEL completed without crashing");
		}
		result[sizeof(result) - 1] = 0;
	}
	else if (_stricmp(cmd, "LIST_ENTITIES") == 0)
	{
		Cmd_ListEntities(arg, result, sizeof(result));
	}
	else if (_stricmp(cmd, "GET_ENTITY") == 0)
	{
		Cmd_GetEntity(arg, result, sizeof(result));
	}
	else if (_stricmp(cmd, "LIST_LIGHTS") == 0)
	{
		Cmd_ListLights(result, sizeof(result));
	}
	else if (_stricmp(cmd, "DUMP_SKIN") == 0)
	{
		Cmd_DumpSkin(arg, result, sizeof(result));
	}
	else if (_stricmp(cmd, "SKIN_WATCH") == 0)
	{
		s_bSkinWatch = (arg[0] != '0');
		_snprintf(result, sizeof(result), "OK: SKIN_WATCH %s (frame=%u seen=%d)",
			s_bSkinWatch ? "ON" : "OFF", s_skinWatchFrame, (int)s_skinWatchSeen.size());
		result[sizeof(result) - 1] = 0;
	}
	else if (_stricmp(cmd, "UNDO") == 0 || _stricmp(cmd, "REDO") == 0)
	{
		// route through the editor's own Ctrl+Z/Ctrl+Y handlers — they divert to the
		// Easy Building Editor's stack when EBE is open and do the selection cleanup.
		// Editor mode only: an undo fired during test game / storyboard would rewrite
		// editor state under the running game.
		if (bWelcomeScreen_Window || bStoryboardWindow || bImGuiInTestGame)
		{
			_snprintf(result, sizeof(result), "ERROR: %s only works in the level editor", cmd);
		}
		else
		{
			extern void editor_undo(void);
			extern void editor_redo(void);
			if (_stricmp(cmd, "UNDO") == 0) editor_undo(); else editor_redo();
			_snprintf(result, sizeof(result), "OK: %s performed", cmd);
		}
		result[sizeof(result) - 1] = 0;
	}
	else if (_stricmp(cmd, "SCULPT_TEST") == 0)
	{
		// SCULPT_TEST <worldX> <worldZ> <frames> [mode] — synthetic RAISE (default) stroke
		// at the given position for N frames; mode = GGTERRAIN_SCULPT_* int to override
		float sx = 0, sz = 0; int sf = 0, sm = GGTERRAIN_SCULPT_RAISE;
		int got = sscanf_s(arg, "%f %f %d %d", &sx, &sz, &sf, &sm);
		if (!AutoHarness_TerrainEditAllowed())
		{
			_snprintf(result, sizeof(result), "ERROR: SCULPT_TEST needs the level editor with terrain initialised");
		}
		else if (s_terrainTestFrames > 0)
		{
			_snprintf(result, sizeof(result), "ERROR: a terrain test is already running (%d frames left)", s_terrainTestFrames);
		}
		else if (g_iCalculatingChangeBounds != 0)
		{
			_snprintf(result, sizeof(result), "ERROR: a real terrain stroke is in progress — release the mouse first");
		}
		else if (got >= 3 && sf > 0)
		{
			s_terrainTestSavedEditMode = GGTerrain::ggterrain_extra_params.edit_mode;
			s_terrainTestSavedSculptMode = GGTerrain::ggterrain_extra_params.sculpt_mode;
			s_terrainTestSavedPaintMat = GGTerrain::ggterrain_extra_params.paint_material;
			s_terrainTestX = sx; s_terrainTestZ = sz;
			s_terrainTestSculptMode = sm;
			s_terrainTestPaint = 0;
			s_terrainTestFrames = sf;
			_snprintf(result, sizeof(result), "OK: SCULPT_TEST at (%.0f, %.0f) mode %d for %d frames (brushSize=%.0f)",
				sx, sz, sm, sf, GGTerrain::ggterrain_global_render_params2.brushSize);
		}
		else
		{
			_snprintf(result, sizeof(result), "ERROR: SCULPT_TEST needs <worldX> <worldZ> <frames> [sculptmode]");
		}
		result[sizeof(result) - 1] = 0;
	}
	else if (_stricmp(cmd, "PAINT_TEST") == 0)
	{
		// PAINT_TEST <worldX> <worldZ> <material> <frames> — synthetic texture-paint stroke;
		// material is the GG source-texture index + 1 (0 erases back to auto materials)
		float px = 0, pz = 0; int pm = 0, pf = 0;
		int got = sscanf_s(arg, "%f %f %d %d", &px, &pz, &pm, &pf);
		if (!AutoHarness_TerrainEditAllowed())
		{
			_snprintf(result, sizeof(result), "ERROR: PAINT_TEST needs the level editor with terrain initialised");
		}
		else if (s_terrainTestFrames > 0)
		{
			_snprintf(result, sizeof(result), "ERROR: a terrain test is already running (%d frames left)", s_terrainTestFrames);
		}
		else if (g_iCalculatingChangeBounds != 0)
		{
			_snprintf(result, sizeof(result), "ERROR: a real terrain stroke is in progress — release the mouse first");
		}
		else if (got == 4 && pf > 0)
		{
			s_terrainTestSavedEditMode = GGTerrain::ggterrain_extra_params.edit_mode;
			s_terrainTestSavedSculptMode = GGTerrain::ggterrain_extra_params.sculpt_mode;
			s_terrainTestSavedPaintMat = GGTerrain::ggterrain_extra_params.paint_material;
			s_terrainTestX = px; s_terrainTestZ = pz;
			s_terrainTestMaterial = pm;
			s_terrainTestPaint = 1;
			s_terrainTestFrames = pf;
			_snprintf(result, sizeof(result), "OK: PAINT_TEST at (%.0f, %.0f) material %d for %d frames (brushSize=%.0f)",
				px, pz, pm, pf, GGTerrain::ggterrain_global_render_params2.brushSize);
		}
		else
		{
			_snprintf(result, sizeof(result), "ERROR: PAINT_TEST needs <worldX> <worldZ> <material> <frames>");
		}
		result[sizeof(result) - 1] = 0;
	}
	else if (_stricmp(cmd, "SET_TREES") == 0)
	{
		// live-tune tree shadow params (mirrors the Terrain Tools debug sliders):
		// SET_TREES shadowdist|shadowrange|drawshadows <value>
		char tp[64] = { 0 }; float tv = 0.0f;
		if (sscanf_s(arg, "%63s %f", tp, (unsigned)sizeof(tp), &tv) == 2)
		{
			bool known = true;
			if (_stricmp(tp, "shadowdist") == 0) GGTrees::ggtrees_global_params.lod_dist_shadow = tv;
			else if (_stricmp(tp, "shadowrange") == 0) GGTrees::ggtrees_global_params.tree_shadow_range = (int)tv;
			else if (_stricmp(tp, "drawshadows") == 0) GGTrees::ggtrees_global_params.draw_shadows = (int)tv;
			else if (_stricmp(tp, "stress") == 0) GGTrees::g_treePoolStressFrames = (int)tv;
			else if (_stricmp(tp, "pool") == 0)
			{
				// Perf knob: effective tree pool size (nearest-N trees drawn as real ECS objects).
				// Each pool entity costs per-frame ECS, so this is the dominant editor CPU lever.
				// Applies on the next pool setup (level reload). Lower = fewer trees + big CPU win.
				int n = (int)tv; if (n < 1) n = 1; if (n > 20000) n = 20000;
				GGTrees::g_treePoolSize = (uint32_t)n;
			}
			else known = false;
			if (known)
				_snprintf(result, sizeof(result), "OK: SET_TREES %s = %.1f (shadowdist=%.0f shadowrange=%d drawshadows=%d)",
					tp, tv, GGTrees::ggtrees_global_params.lod_dist_shadow,
					GGTrees::ggtrees_global_params.tree_shadow_range,
					GGTrees::ggtrees_global_params.draw_shadows);
			else
				_snprintf(result, sizeof(result), "ERROR: SET_TREES unknown param '%s' (shadowdist|shadowrange|drawshadows)", tp);
		}
		else
		{
			_snprintf(result, sizeof(result), "ERROR: SET_TREES needs <param> <value> (shadowdist|shadowrange|drawshadows)");
		}
		result[sizeof(result) - 1] = 0;
	}
	else if (_stricmp(cmd, "DELAYED_SHADOWS") == 0)
	{
		bool on = (arg[0] != '0');
		wi::renderer::SetDelayedShadowCascadesEnabled(on);
		_snprintf(result, sizeof(result), "OK: DELAYED_SHADOWS %s (staggered cascade refresh %s)",
			on ? "ON" : "OFF", wi::renderer::GetDelayedShadowCascadesEnabled() ? "enabled" : "disabled");
		result[sizeof(result) - 1] = 0;
	}
	else if (_stricmp(cmd, "SHADOW_LOD_OVERRIDE") == 0)
	{
		// A/B the "two terrain shapes" shadow flicker: ON = per-cascade shadow LOD (can oscillate vs
		// the visible terrain), OFF = shadows use the stable main-view LOD.
		bool on = (arg[0] != '0');
		wi::renderer::SetShadowLODOverrideEnabled(on);
		_snprintf(result, sizeof(result), "OK: SHADOW_LOD_OVERRIDE %s (%s)",
			on ? "ON" : "OFF", wi::renderer::IsShadowLODOverrideEnabled() ? "per-cascade LOD" : "view LOD");
		result[sizeof(result) - 1] = 0;
	}
	else if (_stricmp(cmd, "SET_OCEAN") == 0)
	{
		// live-tune ocean foam: SET_OCEAN foamscale|foamamount <value>
		// (ocean CB refills from oceanParameters every frame, applies instantly)
		char op[64] = { 0 }; float ov = 0.0f;
		extern float g_fWaterFoamUnitScale;
		extern float g_fWaterFoamAmount;
		extern wi::ecs::Entity g_weatherEntityID;
		if (sscanf_s(arg, "%63s %f", op, (unsigned)sizeof(op), &ov) == 2)
		{
			bool known = true;
			static float s_harnessCausticScale = -1.0f;
				static float s_harnessWaterHeight = -99999.0f;
				static float s_harnessUwDensity = -1.0f;
				static float s_wcR = -1.0f, s_wcG = -1.0f, s_wcB = -1.0f, s_wcA = -1.0f;
				static float s_wcDepth = -1.0f;
			if (_stricmp(op, "foamscale") == 0) g_fWaterFoamUnitScale = ov;
			else if (_stricmp(op, "foamamount") == 0) g_fWaterFoamAmount = ov;
			else if (_stricmp(op, "causticscale") == 0) s_harnessCausticScale = ov;
				else if (_stricmp(op, "waterheight") == 0) s_harnessWaterHeight = ov;
				else if (_stricmp(op, "uwdensity") == 0) s_harnessUwDensity = ov;
				else if (_stricmp(op, "wcr") == 0) s_wcR = ov;
				else if (_stricmp(op, "wcg") == 0) s_wcG = ov;
				else if (_stricmp(op, "wcb") == 0) s_wcB = ov;
				else if (_stricmp(op, "wca") == 0) s_wcA = ov;
				else if (_stricmp(op, "wcdepth") == 0) s_wcDepth = ov;
			else known = false;
			wi::scene::WeatherComponent* weather = wi::scene::GetScene().weathers.GetComponent(g_weatherEntityID);
			if (weather)
			{
				weather->oceanParameters.foam_unit_scale = g_fWaterFoamUnitScale;
				weather->oceanParameters.foam_amount = g_fWaterFoamAmount;
				// Live-tune the seabed caustic scale for quick A/B (see SET_OCEAN causticscale).
				// The Water panel "Caustic Size" slider re-takes control the next time it changes.
				if (s_harnessCausticScale > 0.0f)
					weather->oceanParameters.caustic_scale = s_harnessCausticScale;
					// Test hook: raise/lower the ocean water line (SET_OCEAN waterheight <y>) to
					// submerge the editor camera and verify Wicked's underwaterCS post-process. Reset
					// with the real water line; Wicked_Update_Visuals re-takes control on next change.
					if (s_harnessWaterHeight > -99998.0f)
						weather->oceanParameters.waterHeight = s_harnessWaterHeight;
					// Live-tune the decoupled underwater fog density (SET_OCEAN uwdensity <v>) to find
					// a good default against GG's inch scale, then bake into the visuals default.
					if (s_harnessUwDensity >= 0.0f)
						weather->oceanParameters.underwater_fog_density = s_harnessUwDensity;
					// Live-set the surface water base colour (0..1 each) to reproduce/verify the
					// "Water Base Color doesn't tint from above" issue without the UI picker.
					if (s_wcR >= 0.0f) weather->oceanParameters.waterColor.x = s_wcR;
					if (s_wcG >= 0.0f) weather->oceanParameters.waterColor.y = s_wcG;
					if (s_wcB >= 0.0f) weather->oceanParameters.waterColor.z = s_wcB;
					if (s_wcA >= 0.0f) weather->oceanParameters.waterColor.w = s_wcA;
					if (s_wcDepth >= 0.0f) weather->oceanParameters.water_color_depth = s_wcDepth;
			}
			if (known)
				_snprintf(result, sizeof(result), "OK: SET_OCEAN %s = %.4f (foamscale=%.4f foamamount=%.2f weather=%s)",
					op, ov, g_fWaterFoamUnitScale, g_fWaterFoamAmount, weather ? "live" : "MISSING");
			else
				_snprintf(result, sizeof(result), "ERROR: SET_OCEAN unknown param '%s' (foamscale|foamamount|causticscale|waterheight|uwdensity)", op);
		}
		else
		{
			_snprintf(result, sizeof(result), "ERROR: SET_OCEAN needs <param> <value> (foamscale|foamamount)");
		}
		result[sizeof(result) - 1] = 0;
	}
	else if (_stricmp(cmd, "ENABLE_PROFILER") == 0)
	{
		// Must set bProfilerEnable too — M-GridEditB_part3.cpp actively disables the
		// profiler every frame in editor mode if bProfilerEnable is false
		extern bool bProfilerEnable;
		bProfilerEnable = true;
		wi::profiler::SetEnabled(true);
		_snprintf(result, sizeof(result), "OK: Profiler enabled (bProfilerEnable=true, SetEnabled=true)");
		result[sizeof(result) - 1] = 0;
	}
	else if (_stricmp(cmd, "DISABLE_PROFILER") == 0)
	{
		extern bool bProfilerEnable;
		bProfilerEnable = false;
		wi::profiler::SetEnabled(false);
		_snprintf(result, sizeof(result), "OK: Profiler disabled");
		result[sizeof(result) - 1] = 0;
	}
	else if (_stricmp(cmd, "GET_PROFILER_STATUS") == 0)
	{
		_snprintf(result, sizeof(result),
			"OK: REQ=%d EN=%d IsEnabled=%d CPU=%.2f GPU=%.2f",
			wi::profiler::ENABLED_REQUEST?1:0, wi::profiler::ENABLED?1:0,
			wi::profiler::IsEnabled()?1:0,
			wi::profiler::GetCPUFrameTime(),
			wi::profiler::GetGPUFrameTime());
		result[sizeof(result) - 1] = 0;
	}
	else if (_stricmp(cmd, "QUIT") == 0)
	{
		Cmd_Quit(result, sizeof(result));
		return; // Don't write result again, Cmd_Quit already did
	}
	else
	{
		_snprintf(result, sizeof(result), "ERROR: Unknown command '%s'", cmd);
		result[sizeof(result) - 1] = 0;
	}

	AutoHarness_WriteResult(result);
}
