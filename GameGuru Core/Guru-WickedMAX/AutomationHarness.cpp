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

// WickedEngine helpers for screenshot and scene interrogation
#include "wiHelper.h"
#include "wiGraphicsDevice.h"
#include "wiApplication.h"
#include "wiScene.h"

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

	// Tab mode (profiler panel state)
	written += _snprintf(result + written, resultSize - written,
		"TAB_MODE: %d\n", g.tabmode);

	// Profiler timing data (if profiler is enabled)
	if (wi::profiler::IsEnabled() && written < resultSize - 256)
	{
		float cpuMs = wi::profiler::GetCPUFrameTime();
		float gpuMs = wi::profiler::GetGPUFrameTime();
		written += _snprintf(result + written, resultSize - written,
			"CPU_FRAME_MS: %.2f\n"
			"GPU_FRAME_MS: %.2f\n",
			cpuMs, gpuMs);

		std::string profText = wi::profiler::GetTextData();
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

// ---- Main entry point (called once per tick) ----

void AutoHarness_CheckForCommand(void)
{
	// Init paths and uptime tracking on first call
	if (!s_initialized)
	{
		AutoHarness_InitPaths();
		s_startTick = GetTickCount();
		s_initialized = true;
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
