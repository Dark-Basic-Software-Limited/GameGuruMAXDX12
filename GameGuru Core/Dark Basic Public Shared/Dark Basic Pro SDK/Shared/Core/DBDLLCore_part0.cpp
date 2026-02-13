//
// DarkDLLCore
//

// Standard Includes
#define _CRT_SECURE_NO_DEPRECATE
#define WINVER 0x0601
#define _USING_V110_SDK71_
#include "windows.h"
#include "math.h"
#include "time.h"

// GG flag header for preprocessor defines
#include "..\..\..\..\GameGuru\Include\preprocessor-flags.h"

// Multiplayer Systems
#ifdef PHOTONMP
#include "PhotonCommands.h"
#else
#include "SteamCommands.h"
#endif

// External Includes
#include "..\error\cerror.h"
#include "..\..\DarkSDK\Core\resource.h"
#include ".\..\Core\SteamCheckForWorkshop.h"
#include "DarkLUA.h"

// Internal Includes
#include "DBDLLCore.h"
#include "DBDLLDisplay.h"
#include "DBDLLCoreInternal.h"
#include "DBDLLArray.h"
#include "RenderList.h"

// Vectors and stack
#include <vector>
#include <stack>

// DBP functions
#include "CGfxC.h"
#include "CObjectsC.h"
#include "CCameraC.h"
#include "CImageC.h"
#include "cVectorC.h"
#include "CLightC.h"
#include "CSpritesC.h"
#include "ConvX.h"
#include "CSoundC.h"
#include "CBasic2DC.h"
#include "CParticleC.h"
#include "CBitmapC.h"
#include "CAnimation.h"
#include "CFileC.h"
#include "CMemblocks.h"
#include "CFTPC.h"
#include "CInputC.h"
#include "CTextC.h"
#include "CSystemC.h"
#include "BulletPhysics.H"
#include "BlitzTerrain.H"
#include "DarkAI.H"
#include "SoftwareCulling.h"
#include "DarkLUA.h"
#include "SimonReloaded.h"
#ifdef PHOTONMP
#include "PhotonCommands.h"
#else
#include "SteamCommands.h"
#endif
#include "LightMapper.h"
#include "Enchancements.h"

#ifdef VRTECH
//Windows Mixed Reality Support
#include "GGVR.h"
#endif

#include <iostream>
#include <fstream>

#ifdef ENABLEIMGUI

//PE: GameGuru IMGUI.
#include "..\..\..\..\GameGuru\Imgui\imgui.h"
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "..\..\..\..\GameGuru\Imgui\imgui_internal.h"
#include "..\..\..\..\GameGuru\Imgui\imgui_impl_win32.h"
#include "..\..\..\..\GameGuru\Imgui\imgui_gg_dx11.h"

#include "CGfxC.h"

//PE: ImGui render if we have a imgui frame.
extern bool bImGuiFrameState;
extern bool bImGuiRenderTargetFocus;
extern bool bImGuiReadyToRender;
extern ImVec2 OldrenderTargetSize;
extern ImVec2 OldrenderTargetPos;
extern ImVec2 renderTargetAreaPos;
extern ImVec2 renderTargetAreaSize;
extern ImVec2 vStartResolution;
extern preferences pref;
extern bool bCenterRenderView;
extern ImVec2 fImGuiScissorTopLeft;
extern ImVec2 fImGuiScissorBottomRight;
extern LPSTR m_pOriginalFolderName;

bool bStartNewPrompt = false;
bool CheckTutorialAction(const char * action, float x_adder = 0.0f);
bool CheckTutorialPlaceit(void);
#endif

#define INCLUDEVRAM
#ifdef INCLUDEVRAM
float GetVramUsage(void);
#endif
const char *pestrcasestr(const char *arg1, const char *arg2);
bool g_bDisableQuitFlag = false;

/// not sure what this wasis?
///DB_ENTER_NS()
///DB_LEAVE_NS()

// External Pointers (for cores own error handling)
extern CRuntimeErrorHandler* g_pErrorHandler;
extern LPGG m_pDX;

// Prototypes
LPSTR GetTypePatternCore ( LPSTR dwTypeName, DWORD dwTypeIndex );
DWORD GetNextSyncDelay();

// Touch System works under XP and Win7 now
bool bDetectAndActivateWindows7TouchSystem = false;

// Global Core Vars
DBPRO_GLOBAL LPSTR			g_pVarSpace					= NULL;
DBPRO_GLOBAL LPSTR			g_pDataSpace				= NULL;

// Global Stack Store Vars
DBPRO_GLOBAL DWORD			g_dwStackStoreSize			= 0;
DBPRO_GLOBAL DWORD*			g_pStackStore				= NULL;

// Global Performance Switches
DBPRO_GLOBAL bool			g_bAlwaysActiveOff			= false;
DBPRO_GLOBAL bool			g_bProcessorFriendly		= false; // leefix - 070403 - patch 4 slowdown bug
DBPRO_GLOBAL bool			g_bAlwaysActiveOneOff		= false; // leeadd - 201204 - flag to draw just once (typically when PAINT refreshes)
DBPRO_GLOBAL bool			g_bSyncOff					= true;
DBPRO_GLOBAL bool			g_bSceneStarted				= false;
DBPRO_GLOBAL bool			g_bCanRenderNow				= true;
DBPRO_GLOBAL DWORD			g_dwSyncMask				= 0xFFFFFFFF;
DBPRO_GLOBAL DWORD			g_dwSyncMaskOverride		= 0xFFFFFFFF;

// Global Sync Settings
DBPRO_GLOBAL DWORD			g_dwManualSuperStepSetting	= 0;
DBPRO_GLOBAL DWORD*         g_pdwSyncRateSetting        = NULL;
DBPRO_GLOBAL DWORD          g_dwSyncRateSettingSize     = 0;
DBPRO_GLOBAL DWORD          g_dwSyncRateCurrent         = 0;

// Global Performance Flags used Internally
DBPRO_GLOBAL bool			g_bCascadeQuitFlag			= false;
DBPRO_GLOBAL DWORD			g_dwRecordedTimer			= 0;

// Global Error Handling and Pointers
DBPRO_GLOBAL LPSTR			g_pCommandLineString		= NULL;
DBPRO_GLOBAL LPVOID			g_ErrorHandler				= NULL;
DBPRO_GLOBAL LPVOID			g_EscapeValue				= NULL;
DBPRO_GLOBAL LPVOID			g_BreakOutPosition			= NULL;

// U71 - added to store structure patterns in core (passed in from EXEBlock)
DBPRO_GLOBAL DWORD			g_dwStructPatternQty		= 0;
DBPRO_GLOBAL LPSTR			g_pStructPatternsPtr		= NULL;

// Global Display Vars
DBPRO_GLOBAL HBITMAP		g_hDisplayBitmap			= NULL;
DBPRO_GLOBAL HDC			g_hdcDisplay				= NULL;
DBPRO_GLOBAL COLORREF		g_colFore					= RGB(255,255,255);
DBPRO_GLOBAL COLORREF		g_colBack					= RGB(0,0,0);
DBPRO_GLOBAL HBRUSH			g_hBrush					= NULL;
DBPRO_GLOBAL DWORD			g_dwScreenWidth				= 0;
DBPRO_GLOBAL DWORD			g_dwScreenHeight			= 0;

DBPRO_GLOBAL HICON			g_hUseIcon					= NULL;
DBPRO_GLOBAL HCURSOR		g_hUseArrow 				= NULL;
DBPRO_GLOBAL HCURSOR		g_hUseHourglass 			= NULL;
DBPRO_GLOBAL HCURSOR		g_hCustomCursors[30];
DBPRO_GLOBAL HCURSOR		g_ActiveCursor 				= NULL;
DBPRO_GLOBAL HCURSOR		g_OldCursor 				= NULL;

// Global Draw Order Flags
DBPRO_GLOBAL bool			g_bDrawAutoStuffFirst		= true;
DBPRO_GLOBAL bool			g_bDrawSpritesFirst			= false;
DBPRO_GLOBAL bool			g_bDrawEntirelyToCamera		= false;

// Global Input Vars
DBPRO_GLOBAL DWORD			g_dwWindowsTextEntrySize	= 0;
DBPRO_GLOBAL DWORD			g_dwWindowsTextEntryPos		= 0;
DBPRO_GLOBAL unsigned char	g_cKeyPressed				= 0;
DBPRO_GLOBAL unsigned char	g_cInkeyCodeKey				= 0;
DBPRO_GLOBAL int			g_iEntryCursorState			= 0;
DBPRO_GLOBAL WORD			g_wWinKey					= 0;

// Global Data Vars
DBPRO_GLOBAL LPSTR			g_pDataLabelStart			= NULL;
DBPRO_GLOBAL LPSTR			g_pDataLabelPtr				= NULL;
DBPRO_GLOBAL LPSTR			g_pDataLabelEnd				= NULL;

// Global Security Data
DBPRO_GLOBAL int			g_iSecurityCode				= 0;

// Global Error Helper Clue String
DBPRO_GLOBAL char			g_strErrorClue[512];

// 291116 - allows a much better timeGetTime() using performance counter precision
DBPRO_GLOBAL LONGLONG		g_lFirstPerfTime			= 0;

// Prototype
DARKSDK void CallEncryptDecrypt( char* pStringAddress, bool bEncryptIfTrue, bool bDoNotUseTempFolder );

// Small helper function to get more accurate timings
class AccurateTimer
{
private:
	UINT  Period;
	DWORD LastTime;

	// Disable copying
	AccurateTimer(const AccurateTimer&);
	AccurateTimer& operator=(const AccurateTimer&);
public:
	AccurateTimer() 
	{
		TIMECAPS caps;
		timeGetDevCaps(&caps, sizeof(caps));
		Period = caps.wPeriodMin;
		timeBeginPeriod(Period);
	}
	~AccurateTimer()
	{
		timeEndPeriod(Period);
	}
	DWORD Get()
	{
		LastTime = timeGetTime();
		return LastTime;
	}
	DWORD Last() const
	{
		return LastTime;
	}
};

bool IsArraySingleDim ( DWORD dwArrayPtr )
{
	// Detect if array has single dimension only, false if multi or irregular array
	DWORD* pOldHeader = (DWORD*)(((LPSTR)dwArrayPtr)-HEADERSIZEINBYTES);
	DWORD dwSizeOfOneDataItem = pOldHeader[11];
	if ( dwSizeOfOneDataItem > 1024000 ) return false;
	if ( pOldHeader [ 1 ] > 0 ) return false;
	return true;
}

DARKSDK DWORD ProcessMessagesOnly(void)
{
	// U76 - Windows 7 touch has no 'touch-release' via WM_MOUSE commands
	// so create an artificial persistence so MOUSECLICK(DX) can detect it
	if ( g_pGlob->dwWindowsMouseLeftTouchPersist > 0 )
		if ( timeGetTime() > g_pGlob->dwWindowsMouseLeftTouchPersist )
			g_pGlob->dwWindowsMouseLeftTouchPersist=0;

	// Vars
	MSG msg;

	// Cascade means it will continue to quit (for rapid exit)
	if(g_bCascadeQuitFlag==true)
		return 1;
	
	// Message Pump
	while(TRUE)
	{
		// Standard Windows Processing
		if(PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
		{
			if(msg.message!=WM_QUIT)
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			else
			{
				// special case to ignore QUIT message if derived from masterroot (just want to leave test game or similar)
				if (g_bDisableQuitFlag == false)
				{
					g_bCascadeQuitFlag = true;
					return 1;
				}
			}
		}
		else
		{
			// Processor Friendly
			if(g_bProcessorFriendly) Sleep(1);
			break;
		}
	}

	// Complete
	return 0;
}

DARKSDK void ConstantNonDisplayUpdate(void)
{
	// Update All NonVisuals (this gets called about six times because of processmessage calls..)
	UpdateSound();
	#ifndef NOSTEAMORVIDEO
	UpdateAllAnimation();
	#endif
}

#ifdef WICKEDENGINE
#define SHOWDEBUGMEM
#ifdef SHOWDEBUGMEM
void ShowMemDebug(void);
#endif
#endif
void ImGui_RenderLast(void)
{
#ifdef ENABLEIMGUI

#ifdef USERENDERTARGET
	//PE: There are many single Sync FastSync that do not follow normal render, so:
	extern bool bImGuiInTestGame;
	extern bool bImGuiInitDone;
	#ifdef WICKEDENGINE
	extern bool bRenderTabTab;
	#endif
	extern bool bBlockImGuiUntilNewFrame;
	extern bool bImGuiRenderWithNoCustomTextures;
	#ifdef WICKEDENGINE
	if (bImGuiInitDone && !bImGuiFrameState && !bImGuiInTestGame && !bRenderTabTab) // lee added !bRenderTabTab to prevent double newframe
	#else
	if (bImGuiInitDone && !bImGuiFrameState && !bImGuiInTestGame)
	#endif
	{
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
		bImGuiFrameState = true;
		bBlockImGuiUntilNewFrame = false;
		bImGuiRenderWithNoCustomTextures = false;
		//######################################################################
		//#### Default dockspace setup, how is our windows split on screen. ####
		//######################################################################

		ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking; //ImGuiWindowFlags_MenuBar
		ImGuiViewport* viewport;
		viewport = ImGui::GetMainViewport();
		extern int toolbar_size;
		extern int ImGuiStatusBar_Size;
		ImGui::SetNextWindowPos(viewport->Pos + ImVec2(0, toolbar_size));
		ImGui::SetNextWindowSize(viewport->Size - ImVec2(0, toolbar_size + ImGuiStatusBar_Size));
		ImGui::SetNextWindowViewport(viewport->ID);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		bool dockingopen = true;
		ImGui::Begin("DockSpaceAGK", &dockingopen, window_flags);
		ImGui::PopStyleVar();
		ImGui::PopStyleVar(2);
		static ImGuiID dock_id_bottom;

		if (ImGui::DockBuilderGetNode(ImGui::GetID("MyDockspace")) != NULL) 
		{
			ImGuiID dockspace_id = ImGui::GetID("MyDockspace");
			ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
		}
		ImGui::End();

		ImGui::Begin(TABENTITYNAME);
		ImGui::End();
#ifdef USELEFTPANELSTRUCTUREEDITOR
		extern bool bBuilder_Left_Window;
		if (bBuilder_Left_Window) {
			ImGui::Begin("Structure Editor##LeftPanel");
			ImGui::End();
		}
#endif

		extern bool bHelpVideo_Window;
		extern bool bHelp_Window;
		if (bHelpVideo_Window && bHelp_Window)
		{
			ImGui::Begin("Tutorial Video##HelpVideoWindow");
			ImGui::End();
			ImGui::Begin("Tutorial Steps##HelpWindow");
			ImGui::End();
		}

		bImGuiReadyToRender = true;
	}
#endif

	//PE: ImGui render if we have a imgui frame.
	if (bImGuiFrameState && bImGuiReadyToRender)
	{
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		ImVec2 iOldWindowPadding = ImGui::GetStyle().WindowPadding;
		bImGuiRenderTargetFocus = false;
		bool bPopModalOpen = false;

#ifdef WICKEDENGINE
		// Wicked variant only needs size to pass to SetScissorArea function hook
		ImGui::Begin(TABEDITORNAME, NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		renderTargetAreaPos = ImGui::GetWindowPos();
		renderTargetAreaSize = ImGui::GetContentRegionAvail() + ImVec2(8, 4);

		// account for position and size of main window
		ImGuiViewport* mainViewport = ImGui::GetMainViewport();
		renderTargetAreaPos -= (mainViewport->Pos - ImVec2(0, 23));
		if (!bImGuiInTestGame)
		{
			//Dont touch Scissor in test game.
			fImGuiScissorTopLeft = renderTargetAreaPos;
			fImGuiScissorBottomRight = renderTargetAreaPos + renderTargetAreaSize;
		}
		if (!bPopModalOpen)
		{
			if (OldrenderTargetSize.x != renderTargetAreaSize.x || OldrenderTargetSize.y != renderTargetAreaSize.y ||
				OldrenderTargetPos.x != renderTargetAreaPos.x || OldrenderTargetPos.y != renderTargetAreaPos.y)
			{
				OldrenderTargetSize = renderTargetAreaSize;
				OldrenderTargetPos = renderTargetAreaPos;
			}
		}
		if (ImGui::IsWindowHovered() || ImGui::IsAnyItemHovered())
			bImGuiRenderTargetFocus = true;

		//LB: for some reason, no hover window detected when in Welcome Screen (unable to click buttons!)
		extern int iTriggerWelcomeSystemStuff;
		if (iTriggerWelcomeSystemStuff > 0)
			bImGuiRenderTargetFocus = true;

		ImRect bb = { ImGui::GetWindowContentRegionMin() + ImGui::GetWindowPos(),ImGui::GetWindowContentRegionMax() + ImGui::GetWindowPos() };

		if (pref.iEnableDragDropEntityMode)
		{
			if (ImGui::BeginDragDropTargetCustom(bb, 123456))
			{
				//Hightlight Here!
				int Get_t_gridentityobj(void);
				int Get_t_gridentity(void);
				if (Get_t_gridentityobj() == 0 && Get_t_gridentity() == 0)
				{
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_MODEL_DROP_TARGET", ImGuiDragDropFlags_AcceptBeforeDelivery)) // ImGuiDragDropFlags_AcceptNoDrawDefaultRect
					{
						extern bool bReadyToDropEntity;
						extern bool bWaitOnMouseRelease;
						extern int iDragDropActive;
						//ImGui::IsDragDropPayloadBeingAccepted();
						//PE: Add to lib.
						//PE: Convert to new entity cursor.
						//PE: Move while mouse down.
						AddPayLoad((ImGuiPayload*)payload, true);
						ImGui::ClearDragDrop();
						bReadyToDropEntity = true;
						//Prevent triggering another drag until mouse release.
						bWaitOnMouseRelease = true;
						iDragDropActive = 0;
					}
				}
			}
		}

		extern bool g_bCharacterCreatorPlusActivated;
		if (g_bCharacterCreatorPlusActivated)
		{
			extern bool bBoostIconColors;
			ImVec2 vCurPos = ImGui::GetCursorPos();
			float fFontSize = ImGui::GetFontSize();
			int icon_size = ImGui::GetFontSize() * 3.0;
			ImVec2 VIconSize = { (float)icon_size, (float)icon_size };
			if (ImGui::ImgBtn(TOOL_GOBACK, VIconSize, ImVec4(0, 0, 0, 0), ImVec4(1.0f, 1.0f, 1.0f, 1.0f), ImVec4(1.0f, 1.0f, 1.0f, 0.5f), ImVec4(0.5f, 0.5f, 0.5f, 0.5f), 0, 0, 0, 0, false, false, false, false, false, bBoostIconColors))
			{
				g_bCharacterCreatorPlusActivated = false;
			}
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", "Exit Character Creator");
		}

		extern bool bImporter_Window;
		if (bImporter_Window)
		{
			extern bool bBoostIconColors;
			ImVec2 vCurPos = ImGui::GetCursorPos();
			int icon_size = ImGui::GetFontSize() * 3.0;
			ImVec2 VIconSize = { (float)icon_size, (float)icon_size };
			if (ImGui::ImgBtn(TOOL_GOBACK, VIconSize, ImVec4(0, 0, 0, 0), ImVec4(1.0f, 1.0f, 1.0f, 1.0f), ImVec4(1.0f, 1.0f, 1.0f, 0.5f), ImVec4(0.5f, 0.5f, 0.5f, 0.5f), 0, 0, 0, 0, false, false, false, false, false, bBoostIconColors))
			{
				extern int iDelayedExecute;
				iDelayedExecute = 2;
			}
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", "Exit Importer");
		}
#ifdef PENEWLAYOUT
		ImVec2 winpos = ImGui::GetWindowPos();
		ImVec2 winsize = ImGui::GetWindowSize();
		ImVec2 winsizeavail = ImGui::GetContentRegionAvail();
#endif
		ImGui::End();

#ifdef PENEWLAYOUT
		extern bool bTerrain_Tools_Window;
		extern bool bWelcomeScreen_Window;
		extern bool bStoryboardWindow;
		extern bool bMarketplace_Window;
		extern bool bScreen_Editor_Window;
		if ( (pref.iSmallToolbar >= 1) && !g_bCharacterCreatorPlusActivated && !bImporter_Window && !bTerrain_Tools_Window
			&& !bWelcomeScreen_Window && !bStoryboardWindow && !bMarketplace_Window && !bScreen_Editor_Window)
		{
			//PE: VS2022 style
			float r = pref.highlight_color.x; // = ImVec4((1.0f / 255.0f) * 14, (1.0f / 255.0f) * 99, (1.0f / 255.0f) * 156, 1.0);
			float g = pref.highlight_color.y; // (1.0f / 255.0f) * 99;
			float b = pref.highlight_color.z; // (1.0f / 255.0f) * 156;
			if (pref.current_style == 25)
			{
				r = (1.0f / 255.0f) * 43;
				g = (1.0f / 255.0f) * 79;
				b = (1.0f / 255.0f) * 106;
			}
			else if (pref.current_style != 1)
			{
				ImVec4* colors = ImGui::GetStyle().Colors;
				r = colors[ImGuiCol_WindowBg].x;
				g = colors[ImGuiCol_WindowBg].y;
				b = colors[ImGuiCol_WindowBg].z;
			}

			ImVec4 IconColor = ImVec4(1.0, 1.0, 1.0, 0.8);
			ImVec4 IconColorSelected = ImVec4(1.0, 1.0, 1.0, 0.9);
			const ImVec4 bgColor = ImVec4(0.0, 0.0, 0.0, 0.125);
			const ImVec4 bgColorSelected = ImVec4(r, g, b, 1);
			if (pref.current_style == 25)
			{
				IconColor = ImVec4(1.0, 1.0, 1.0, 0.9);
				IconColorSelected = ImVec4(1.0, 1.0, 1.0, 1.0);
			}
			const float groupspacer = 8.0f;
			float smalltoolbariconsize = 22.0f;
			float boxwidth = 200 + groupspacer;
			float fInputTextWidth = 36.0f;
			if (pref.iSmallToolbar == 1)
				fInputTextWidth = 37.0f;

			if (pref.iAdvancedGridModeSettings == 0)
			{
				boxwidth += fInputTextWidth;
			}
			if (pref.iAdvancedGridModeSettings == 0 && pref.iSmallToolbar == 1)
				boxwidth -= (smalltoolbariconsize - 2);
			else
				boxwidth -= (smalltoolbariconsize - 1);

			ImGuiViewport* viewport = ImGui::GetMainViewport();
			if (pref.iSmallToolbar == 2)
			{
				boxwidth = 40;
				smalltoolbariconsize = 36.0f;
				ImGui::SetNextWindowPos(winpos + ImVec2(winsizeavail.x - boxwidth, 22 + ((boxwidth - smalltoolbariconsize)*2.0f)), ImGuiCond_Always, ImVec2(0, 0));
				ImGui::SetNextWindowSize(ImVec2(smalltoolbariconsize,-1), ImGuiCond_Always);
			}
			else if (pref.iSmallToolbar == 3)
			{
				boxwidth = 30;
				ImGui::SetNextWindowPos(winpos + ImVec2(winsizeavail.x - boxwidth, 22 + ((boxwidth - smalltoolbariconsize) * 2.0f)), ImGuiCond_Always, ImVec2(0, 0));
				ImGui::SetNextWindowSize(ImVec2(smalltoolbariconsize, -1), ImGuiCond_Always);
			}
			else if (pref.iSmallToolbar == 4)
			{
				int icon_size = 44; // 50;
				boxwidth = (icon_size * 8.0f) + (groupspacer * 2.0f);
				smalltoolbariconsize = icon_size;
				ImVec2 viewPortPos = ImGui::GetMainViewport()->Pos;
				ImVec2 viewPortSize = ImGui::GetMainViewport()->Size;
				float fsy = ImGui::CalcTextSize("#").y;
				int toolbar_size = icon_size + (fsy * 2.0) + 2;
				float center = (viewPortSize.x * 0.5f) - (boxwidth * 0.5f);
				float menubarsize = 27.0f + ( (50.0f - icon_size) * 0.5f );
				ImGui::SetNextWindowPos( viewPortPos + ImVec2(center, menubarsize), ImGuiCond_Always);
				ImGui::SetNextWindowSize( ImVec2(boxwidth, icon_size) );
			}
			else
			{
				ImGui::SetNextWindowPos(winpos + ImVec2(winsizeavail.x - boxwidth, 0), ImGuiCond_Always, ImVec2(0, 0));
			}
			ImGui::SetNextWindowViewport(viewport->ID);
			if (pref.iSmallToolbar == 2 || pref.iSmallToolbar == 3 || pref.iSmallToolbar == 4)
				ImGui::SetNextWindowBgAlpha(0.35f);
			else
				ImGui::SetNextWindowBgAlpha(0.0f);

			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 0 });
			if (pref.iSmallToolbar == 2 || pref.iSmallToolbar == 3)
				ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
			else
				ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 0, 0 });
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0, 0 });
			ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 0);
			
			bool bWidgetEnabled = pref.iEnableDragDropWidgetSelect;
			void SetWidgetMode(int mode);
			int GetWidgetMode(void);
			int GetEntitySelected(void);
			int GetEntityGridMode(void);
			int GetActiveEditorObject(void);
			void GridPopup(ImVec2 wpos);
			void widget_hide(void);
			void widget_show_widget(void);
			bool bAlwaysOpen = true;
			ImVec2 popup_pos = { 0,0 };
			uint32_t flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
			if (pref.iSmallToolbar == 4)
			{
				flags |= ImGuiWindowFlags_Tooltip;
			}
			if (ImGui::Begin("##smalltoolbarineditor", &bAlwaysOpen, flags))
			{
				bool bSelected = (GetWidgetMode() == 0 && bWidgetEnabled);
				ImGui::SetItemAllowOverlap();
				if (ImGui::ImgBtn(TOOLBAR_POSITION, ImVec2(smalltoolbariconsize, smalltoolbariconsize), bSelected ? bgColorSelected : bgColor, bSelected ? IconColorSelected : IconColor, ImVec4(0.7, 0.7, 0.7, 0.7), ImVec4(0.7, 0.7, 0.7, 0.7), 0, 0, 0, 0, false, false, false, false, false, false))
				{
					pref.iEnableDragDropWidgetSelect = true;
					SetWidgetMode(0);
					widget_show_widget();
				}

				if (pref.iSmallToolbar == 1 || pref.iSmallToolbar == 4)
					ImGui::SameLine();
				bSelected = (GetWidgetMode() == 1 && bWidgetEnabled);
				ImGui::SetItemAllowOverlap();
				if (ImGui::ImgBtn(TOOLBAR_ROTATE, ImVec2(smalltoolbariconsize, smalltoolbariconsize), bSelected ? bgColorSelected : bgColor, bSelected ? IconColorSelected : IconColor, ImVec4(0.7, 0.7, 0.7, 0.7), ImVec4(0.7, 0.7, 0.7, 0.7), 0, 0, 0, 0, false, false, false, false, false, false))
				{
					pref.iEnableDragDropWidgetSelect = true;
					SetWidgetMode(1);
					widget_show_widget();
				}
				if (pref.iSmallToolbar == 1 || pref.iSmallToolbar == 4)
					ImGui::SameLine();
				bSelected = (GetWidgetMode() == 2 && bWidgetEnabled);
				ImGui::SetItemAllowOverlap();
				if (ImGui::ImgBtn(TOOLBAR_SCALE, ImVec2(smalltoolbariconsize, smalltoolbariconsize), bSelected ? bgColorSelected : bgColor, bSelected ? IconColorSelected : IconColor, ImVec4(0.7, 0.7, 0.7, 0.7), ImVec4(0.7, 0.7, 0.7, 0.7), 0, 0, 0, 0, false, false, false, false, false, false))
				{
					pref.iEnableDragDropWidgetSelect = true;
					SetWidgetMode(2);
					widget_show_widget();
				}
				if (pref.iSmallToolbar == 1 || pref.iSmallToolbar == 4)
					ImGui::SameLine();

				if (pref.iSmallToolbar == 1 || pref.iSmallToolbar == 4)
					ImGui::SetCursorPosX(ImGui::GetCursorPosX() + groupspacer);
				else
					ImGui::SetCursorPosY(ImGui::GetCursorPosY() + groupspacer);

				extern int iObjectMoveMode;
				bSelected = false;
				if (!bWidgetEnabled && iObjectMoveMode == 2)
					bSelected = true;

				if (ImGui::ImgBtn(TOOLBAR_SURFACE, ImVec2(smalltoolbariconsize, smalltoolbariconsize), bSelected ? bgColorSelected : bgColor, bSelected ? IconColorSelected : IconColor, ImVec4(0.7, 0.7, 0.7, 0.7), ImVec4(0.7, 0.7, 0.7, 0.7), 0, 0, 0, 0, false, false, false, false, false, false))
				{
					pref.iEnableDragDropWidgetSelect = false;
					iObjectMoveMode = 2;
					widget_hide();
				}
				if (pref.iSmallToolbar == 1 || pref.iSmallToolbar == 4)
					ImGui::SameLine();
				bSelected = false;
				if (!bWidgetEnabled && iObjectMoveMode == 0)
					bSelected = true;
				if (ImGui::ImgBtn(TOOLBAR_HORI, ImVec2(smalltoolbariconsize, smalltoolbariconsize), bSelected ? bgColorSelected : bgColor, bSelected ? IconColorSelected : IconColor, ImVec4(0.7, 0.7, 0.7, 0.7), ImVec4(0.7, 0.7, 0.7, 0.7), 0, 0, 0, 0, false, false, false, false, false, false))
				{
					pref.iEnableDragDropWidgetSelect = false;
					iObjectMoveMode = 0;
					widget_hide();
				}
				if (pref.iSmallToolbar == 1 || pref.iSmallToolbar == 4)
					ImGui::SameLine();
				bSelected = false;
				if (!bWidgetEnabled && iObjectMoveMode == 1)
					bSelected = true;
				if (ImGui::ImgBtn(TOOLBAR_VERT, ImVec2(smalltoolbariconsize, smalltoolbariconsize), bSelected ? bgColorSelected : bgColor, bSelected ? IconColorSelected : IconColor, ImVec4(0.7, 0.7, 0.7, 0.7), ImVec4(0.7, 0.7, 0.7, 0.7), 0, 0, 0, 0, false, false, false, false, false, false))
				{
					pref.iEnableDragDropWidgetSelect = false;
					iObjectMoveMode = 1;
					widget_hide();
				}
				if (pref.iSmallToolbar == 1 || pref.iSmallToolbar == 4)
					ImGui::SameLine();

				// Snap Mode Toggle
				bool bSnapSelected = false;
				if (pref.iGridEnabled == true && pref.iGridMode == 1)
					bSnapSelected = true;
				if (GetEntityGridMode() == 1)
					bSnapSelected = true;
				if (ImGui::ImgBtn(TOOLBAR_SNAP, ImVec2(smalltoolbariconsize, smalltoolbariconsize), bSnapSelected ? bgColorSelected : bgColor, bSnapSelected ? IconColorSelected : IconColor, ImVec4(0.7, 0.7, 0.7, 0.7), ImVec4(0.7, 0.7, 0.7, 0.7), 0, 0, 0, 0, false, false, false, false, false, false))
				{
					SetWidgetMode(4); //Toggle snap on off.
					if (pref.iGridMode == 0) pref.iGridEnabled = false;
				}

				if (pref.iSmallToolbar == 1 || pref.iSmallToolbar == 4)
					ImGui::SameLine();

				if (pref.iSmallToolbar == 1 || pref.iSmallToolbar == 4)
					ImGui::SetCursorPosX(ImGui::GetCursorPosX() + groupspacer);
				else
					ImGui::SetCursorPosY(ImGui::GetCursorPosY() + groupspacer);

				bSelected = false;
				if (pref.iGridEnabled == true && pref.iGridMode == 2)
					bSelected = true;

				if (ImGui::ImgBtn(TOOLBAR_GRID, ImVec2(smalltoolbariconsize, smalltoolbariconsize), bSelected ? bgColorSelected : bgColor, bSelected ? IconColorSelected : IconColor, ImVec4(0.7, 0.7, 0.7, 0.7), ImVec4(0.7, 0.7, 0.7, 0.7), 0, 0, 0, 0, false, false, false, false, false, false))
				{
					SetWidgetMode(5); //Toggle grid on off.
					if (pref.iGridMode == 0) pref.iGridEnabled = false;
				}
				if (pref.iGridEnabled == true && pref.iGridMode == 2)
					bSelected = true;

				//PE: Not needed anymore grid settings is always visible in all configurations.
				/*
				if (pref.iSmallToolbar == 1 || pref.iSmallToolbar == 4)
					ImGui::SameLine();

				if (!(pref.iAdvancedGridModeSettings == 0 && (pref.iSmallToolbar == 1 || pref.iSmallToolbar == 2)))
				{
					popup_pos = ImGui::GetCursorScreenPos();
					if (ImGui::ImgBtn(TOOLBAR_GRIDSETTINGS, ImVec2(smalltoolbariconsize, smalltoolbariconsize), bSelected ? bgColorSelected : bgColor, bSelected ? IconColorSelected : IconColor, ImVec4(0.7, 0.7, 0.7, 0.7), ImVec4(0.7, 0.7, 0.7, 0.7), 0, 0, 0, 0, false, false, false, false, false, false))
					{
						//PE: Popup grid settings.
						ImGui::OpenPopup("Grid##GridSettings");
					}
				}
				*/
				if (pref.iSmallToolbar == 1 || pref.iSmallToolbar == 4)
					popup_pos.y = ImGui::GetCursorScreenPos().y;
				else
					popup_pos.x = ImGui::GetCursorScreenPos().x + smalltoolbariconsize;

				if (pref.iAdvancedGridModeSettings == 0 && (pref.iSmallToolbar == 1 || pref.iSmallToolbar == 2))
				{
					if (pref.iSmallToolbar == 1 || pref.iSmallToolbar == 4)
						ImGui::SameLine();
					if(pref.iSmallToolbar == 1)
						ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 2.0f, 1.5f });
					else
						ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 1.5f, 1.5f });
					if (!(pref.iGridEnabled == true && pref.iGridMode == 2))
					{
						ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
						ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
					}

					ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2.0f);
					if (pref.iSmallToolbar == 1)
						ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 2.0f);

					ImGui::PushItemWidth(fInputTextWidth);
					std::string precision = "%.2f";
					if (pref.fEditorGridSizeX >= 100.0f)
					{
						precision = "%.1f";
					}
					if (ImGui::InputFloat("##XYZgridsizeXYZ", &pref.fEditorGridSizeX, 0.0f, 0.0f, precision.c_str()))
					{
						// can never have a grid size below one
						if (pref.fEditorGridSizeX <= 1) pref.fEditorGridSizeX = 1.0f;
						// and all grid dimensions the same!
						pref.fEditorGridOffsetX = 0;
						pref.fEditorGridOffsetY = 0;
						pref.fEditorGridOffsetZ = 0;
						pref.fEditorGridSizeY = pref.fEditorGridSizeX;
						pref.fEditorGridSizeZ = pref.fEditorGridSizeX;
					}
					if (!pref.iTurnOffEditboxTooltip && ImGui::IsItemHovered()) ImGui::SetTooltip("Change Grid Size");

					if (!(pref.iGridEnabled == true && pref.iGridMode == 2))
					{
						ImGui::PopItemFlag();
						ImGui::PopStyleVar();
					}

					ImGui::PopItemWidth();
					ImGui::PopStyleVar();
					if (pref.iSmallToolbar == 2)
						ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2.0f);

				}

			}

			ImGui::PopStyleVar(5);
			GridPopup(popup_pos);
			ImGui::End();

			bool GetEnableEmptyLevelMode(void);
			if (GetEnableEmptyLevelMode())
			{
				extern float fEmptyLevelFloorY;
				extern bool bEmptyLevelGrid;

				flags |= ImGuiWindowFlags_Tooltip;
				flags |= ImGuiWindowFlags_NoBackground;
				flags |= ImGuiWindowFlags_NoScrollbar;

				float boxwidth = 190;
				if (pref.iAdvancedGridModeSettings == 0)
				{
					boxwidth = 230;
				}
				//boxwidth = 120.0f;
				int icon_size = 44.0f;
				float menubarsize = 27.0f + ((50.0f - icon_size) * 0.5f);
				ImVec2 viewPortPos = ImGui::GetMainViewport()->Pos;
				//ImVec2 viewPortSize = ImGui::GetMainViewport()->Size;
				//float center = (viewPortSize.x * 0.5f) - (boxwidth * 0.5f);
				//winsizeavail.x - boxwidth
				ImVec2 placeit = ImVec2(winpos.x + (winsizeavail.x - boxwidth), viewPortPos.y + menubarsize);
				ImGui::SetNextWindowPos(placeit, ImGuiCond_Always, ImVec2(0, 0));

				//ImGui::SetNextWindowPos(viewPortPos + ImVec2(center, menubarsize), ImGuiCond_Always);
				ImGui::SetNextWindowSize(ImVec2(boxwidth, 50));

				if (ImGui::Begin("##additionalEnableEmptyLevelMode", &bAlwaysOpen, flags))
				{
					ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 2.0f, 2.0f });

					ImGui::Checkbox(" Floor Grid", &bEmptyLevelGrid);

					ImGui::Text("Pos Floor Y");
					ImGui::SameLine();
					ImGui::SetCursorPos(ImGui::GetCursorPos() - ImVec2(0.0f, 3.0f));
					ImGui::PushItemWidth(70);
					if (ImGui::InputFloat("##fEmptyLevelFloorY", &fEmptyLevelFloorY, -100000.0f, 100000.0f, "%.2f")) //"%.2f"
					{

					}
					ImGui::PopItemWidth();
					ImGui::PopStyleVar();
				}
				ImGui::End();

			}

			if (pref.iGridEnabled == true && pref.iGridMode == 2)
			{
				if (pref.iAdvancedGridModeSettings == 1 || !((pref.iSmallToolbar == 1 || pref.iSmallToolbar == 2)))
				{
					bool bButSpacer = true;
					const float button_width_fix = 5.0f;
					float but_gadget_size = ImGui::GetFontSize() * 14.0;
					float input_text_width = 60.0f;
					int icon_size = 44; // 50;
					boxwidth = (icon_size * 8.0f) + (groupspacer * 2.0f);
					smalltoolbariconsize = icon_size;
					ImVec2 viewPortPos = ImGui::GetMainViewport()->Pos;
					ImVec2 viewPortSize = ImGui::GetMainViewport()->Size;
					float fsy = ImGui::CalcTextSize("#").y;
					int toolbar_size = icon_size + (fsy * 2.0) + 2;
					float menubarsize = 27.0f + ((50.0f - icon_size) * 0.5f);
					if (pref.iAdvancedGridModeSettings == 0)
					{
						menubarsize += 12.0f;
						boxwidth = 150.0f;
						icon_size = 28.0;
					}
					else
					{
						menubarsize += 2;
						boxwidth = 620.0f;
						icon_size = 50.0f;
					}
					float center = (viewPortSize.x * 0.5f) - (boxwidth * 0.5f);
					ImGui::SetNextWindowPos(viewPortPos + ImVec2(center, menubarsize), ImGuiCond_Always);
					ImGui::SetNextWindowSize(ImVec2(boxwidth, icon_size));

					flags |= ImGuiWindowFlags_Tooltip;
					flags |= ImGuiWindowFlags_NoBackground;

					if (ImGui::Begin("##additionaltoolbarineditor", &bAlwaysOpen, flags))
					{
						int GetEntityObject(int iEntityIndex);
						void GetEntityPosition(int iEntityIndex, float& x, float& y, float& z);
						int GetRubberbandSize(void);

						int iEntityIndex = GetEntitySelected();
						int iActiveObj = GetActiveEditorObject();

						ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 2.0f, 2.0f });

						//PE: Additional window corering the original toolbar.
						if (pref.iAdvancedGridModeSettings == 0)
						{
							// Simple Grid Mode
							ImGui::Text("Grid Size");
							float w = ImGui::GetContentRegionAvail().x;
							float inputsize = w / 4.0f;
							ImGui::SameLine();
							//ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((w / 2) - (inputsize / 2), 0.0f));
							ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 2.0f);
							ImGui::PushItemWidth(input_text_width);
							ImGui::InputFloat("##XYZgridsizeXYZ2", &pref.fEditorGridSizeX, 0.0f, 0.0f, "%.1f");
							//if (!pref.iTurnOffEditboxTooltip && ImGui::IsItemHovered()) ImGui::SetTooltip("Change Grid Size");
							ImGui::PopItemWidth();

							// can never have a grid size below one
							if (pref.fEditorGridSizeX <= 1) pref.fEditorGridSizeX = 1.0f;

							// and all grid dimensions the same!
							pref.fEditorGridOffsetX = 0;
							pref.fEditorGridOffsetY = 0;
							pref.fEditorGridOffsetZ = 0;
							pref.fEditorGridSizeY = pref.fEditorGridSizeX;
							pref.fEditorGridSizeZ = pref.fEditorGridSizeX;
						}
						else
						{
							// Advanced Grid Mode functions and settings
							float start_cursor_x = 80.0f;
							ImGui::Text("Grid Offset");
							ImGui::SameLine();
							float w = ImGui::GetContentRegionAvail().x;
							float inputsize = w / 3.0f;
							inputsize -= 10.0f; //For text.
							inputsize -= 5.0f; //For padding.

							//ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 2.0f);
							ImGui::SetCursorPosX(start_cursor_x);
							ImGui::Text("X");
							ImGui::SameLine();
							ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(0.0f, -3.0f));
							ImGui::PushItemWidth(input_text_width);
							ImGui::InputFloat("##XYZgridoffsetX", &pref.fEditorGridOffsetX, 0.0f, 0.0f, "%.1f");
							if (!pref.iTurnOffEditboxTooltip && ImGui::IsItemHovered()) ImGui::SetTooltip("Change Grid Offset X");
							ImGui::PopItemWidth();
							ImGui::SameLine();
							ImGui::Text("Y");
							ImGui::SameLine();
							ImGui::PushItemWidth(input_text_width);
							ImGui::InputFloat("##XYZgridoffsetY", &pref.fEditorGridOffsetY, 0.0f, 0.0f, "%.1f");
							if (!pref.iTurnOffEditboxTooltip && ImGui::IsItemHovered()) ImGui::SetTooltip("Change Grid Offset Y");
							ImGui::PopItemWidth();
							ImGui::SameLine();
							ImGui::Text("Z");
							ImGui::SameLine();
							ImGui::PushItemWidth(input_text_width);
							ImGui::InputFloat("##XYZgridoffsetZ", &pref.fEditorGridOffsetZ, 0.0f, 0.0f, "%.1f");
							if (!pref.iTurnOffEditboxTooltip && ImGui::IsItemHovered()) ImGui::SetTooltip("Change Grid Offset Z");
							ImGui::PopItemWidth();
							ImGui::SameLine();
							if (ImGui::StyleButton("Default Grid Settings", ImVec2(284, 0)))
							{
								pref.fEditorGridOffsetX = 50;
								pref.fEditorGridOffsetY = 0;
								pref.fEditorGridOffsetZ = 50;
								pref.fEditorGridSizeX = 100;
								pref.fEditorGridSizeY = 10;
								pref.fEditorGridSizeZ = 100;
							}


							ImGui::Text("Grid Size");
							ImGui::SameLine();
							//ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(0.0f, 3.0f));
							ImGui::SetCursorPosX(start_cursor_x);
							ImGui::Text("X");
							ImGui::SameLine();
							ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(0.0f, -3.0f));
							ImGui::SameLine();
							ImGui::PushItemWidth(input_text_width);
							ImGui::InputFloat("##XYZgridsizeX", &pref.fEditorGridSizeX, 0.0f, 0.0f, "%.1f");
							if (!pref.iTurnOffEditboxTooltip && ImGui::IsItemHovered()) ImGui::SetTooltip("Change Grid Size X");
							ImGui::PopItemWidth();
							ImGui::SameLine();
							ImGui::Text("Y");
							ImGui::SameLine();
							ImGui::PushItemWidth(input_text_width);
							ImGui::InputFloat("##XYZgridsizeY", &pref.fEditorGridSizeY, 0.0f, 0.0f, "%.1f");
							if (!pref.iTurnOffEditboxTooltip && ImGui::IsItemHovered()) ImGui::SetTooltip("Change Grid Size Y");
							ImGui::PopItemWidth();
							ImGui::SameLine();
							ImGui::Text("Z");
							ImGui::SameLine();
							ImGui::PushItemWidth(input_text_width);
							ImGui::InputFloat("##XYZgridsizeZ", &pref.fEditorGridSizeZ, 0.0f, 0.0f, "%.1f");
							if (!pref.iTurnOffEditboxTooltip && ImGui::IsItemHovered()) ImGui::SetTooltip("Change Grid Size Z");
							ImGui::PopItemWidth();

							bButSpacer = false;

							// clever button to align grid to object (for older levels with arbitary alignments mixed together)
							if (iEntityIndex > 0 && GetRubberbandSize() == 0)
							{
								ImGui::SameLine();
								//ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((w * 0.5) - ((but_gadget_size * 0.5) + button_width_fix), 0.0f));
								if (ImGui::StyleButton("Align Offset To Object", ImVec2(140, 0)))
								{
									float x = 0;// t.entityelement[iEntityIndex].x;
									float y = 0;// t.entityelement[iEntityIndex].y;
									float z = 0;// t.entityelement[iEntityIndex].z;
									GetEntityPosition(iEntityIndex, x, y, z);

									int iSizeRoundedX = int(x / pref.fEditorGridSizeX) * pref.fEditorGridSizeX;
									pref.fEditorGridOffsetX = x - iSizeRoundedX;
									int iSizeRoundedY = int(y / pref.fEditorGridSizeY) * pref.fEditorGridSizeY;
									pref.fEditorGridOffsetY = y - iSizeRoundedY;
									int iSizeRoundedZ = int(z / pref.fEditorGridSizeZ) * pref.fEditorGridSizeZ;
									pref.fEditorGridOffsetZ = z - iSizeRoundedZ;
								}
								//ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((w * 0.5) - ((but_gadget_size * 0.5) + button_width_fix), 0.0f));
								ImGui::SameLine();
								ImGui::SetCursorPosX(ImGui::GetCursorPosX() - 4.0f);
								if (ImGui::StyleButton("Align Size To Object", ImVec2(140, 0)))
								{
									int obj = GetEntityObject(iEntityIndex);
									float sx = ObjectSizeX(obj, 1);
									float sy = ObjectSizeY(obj, 1);
									float sz = ObjectSizeZ(obj, 1);
									pref.fEditorGridSizeX = sx;
									pref.fEditorGridSizeY = sy;
									pref.fEditorGridSizeZ = sz;
								}
							}

							// can never have a grid size below one
							if (pref.fEditorGridSizeX <= 1) pref.fEditorGridSizeX = 1.0f;
							if (pref.fEditorGridSizeY <= 1) pref.fEditorGridSizeY = 1.0f;
							if (pref.fEditorGridSizeZ <= 1) pref.fEditorGridSizeZ = 1.0f;
						}
						ImGui::PopStyleVar();
					}
					ImGui::End();
				}
			}
		}
#endif



		#ifdef SHOWDEBUGMEM
		ShowMemDebug();
		#endif

		#else
		#ifdef USERENDERTARGET
		//PE: This must be last , as we need it to include everything.
		ImGui::GetStyle().WindowPadding = { 0.0f,0.0f };
		bool bVisible = true;
		extern bool bRenderTargetModalMode;
		extern int refresh_gui_docking;
		if (bRenderTargetModalMode) {
			//ImGuiWindowFlags_Popup
			ImGui::OpenPopup("##DisableEverythingModalMode");
			bool bAlwaysOpen = true;
			if (refresh_gui_docking >= 4 && OldrenderTargetSize.x > 64 && OldrenderTargetSize.y > 64) {
				extern int ImGuiStatusBar_Size;
				float titlesizey = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0f;
				ImGui::SetNextWindowSize(OldrenderTargetSize - ImVec2(0, 2), ImGuiCond_Always); // - ImVec2(0, ImGuiStatusBar_Size)
				ImGui::SetNextWindowPos(OldrenderTargetPos - ImVec2(0, titlesizey), ImGuiCond_Always);
				bPopModalOpen = ImGui::BeginPopupModal("##DisableEverythingModalMode", &bAlwaysOpen, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings);
				if (!bPopModalOpen)
					ImGui::Begin(TABEDITORNAME, NULL, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
			}
			else {
				ImGui::Begin(TABEDITORNAME, NULL, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
			}
		}
		else 
		{
			ImGui::Begin(TABEDITORNAME, NULL, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse); //ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar
		}

		if(bPopModalOpen)
			ImGui::BeginChild("GGFinalRenderTarget", OldrenderTargetSize , false, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNavInputs);
		else
		{
			//#ifdef WICKEDENGINE
			//ImGui::BeginChild("GGFinalRenderTarget", ImGui::GetWindowSize(), false, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNavInputs); //ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar
			//#else
			ImGui::BeginChild("GGFinalRenderTarget", ImGui::GetWindowSize(), false, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNavInputs); //ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar
			//#endif
		}

		int iId = 21; //g.postprocessimageoffset;
		ID3D11ShaderResourceView* lpTexture = GetImagePointerView(iId);
		ImGuiWindow* window = ImGui::GetCurrentWindow();

		ImVec2 renderTargetSize = ImGui::GetContentRegionAvail();
		ImVec2 renderTargetPos = ImGui::GetWindowPos();

		//Dont go below 100x100 rendertarget.
		if (renderTargetSize.x < 100)
			renderTargetSize.x = 100;
		if (renderTargetSize.y < 100)
			renderTargetSize.y = 100;
		//Dont update when in modal popup
		if (!bPopModalOpen) {
			if (OldrenderTargetSize.x != renderTargetSize.x || OldrenderTargetSize.y != renderTargetSize.y ||
				OldrenderTargetPos.x != renderTargetPos.x || OldrenderTargetPos.y != renderTargetPos.y)
			{
				OldrenderTargetSize = renderTargetSize;
				OldrenderTargetPos = renderTargetPos;
				//SetCameraToImage(0, 21, renderTargetSize.x, renderTargetSize.y, 2);
			}
		}
		if (lpTexture) {
			ImVec4 drawCol_normal = ImColor(255, 255, 255, 255);

			int iBorderSize = 15;
			float fPreviewWidth = (int)renderTargetSize.x; // -iBorderSize;
			float fPreviewHeight = (int)renderTargetSize.y; // -iBorderSize;
			int iImgW = GetDisplayWidth();
			int iImgH = GetDisplayHeight();
			float fRatio;

			//if ((fPreviewWidth / iImgW) < (fPreviewHeight / iImgH))
			//fRatio = fPreviewWidth / iImgW;
			//else
			fRatio = fPreviewHeight / iImgH;
			if (iImgW*fRatio < fPreviewWidth)
				fRatio = fPreviewWidth / iImgW;

			float fCenterX = (fPreviewWidth - iImgW*fRatio) * 0.5;
			float fCenterY = (fPreviewHeight - iImgH*fRatio) * 0.5;
			if (fCenterY < 0.0)
				fCenterY = 0.0;

			if (!bCenterRenderView) {
			fCenterX = 0.0;
			fCenterY = 0.0;
			}

			renderTargetAreaPos = renderTargetPos + ImVec2(fCenterX, fCenterY);
			renderTargetAreaSize = ImVec2(iImgW*fRatio, iImgH*fRatio);

			window->DrawList->AddImage((ImTextureID)lpTexture, renderTargetAreaPos, renderTargetPos + renderTargetAreaSize + ImVec2(fCenterX, fCenterY), ImVec2(0, 0), ImVec2(1, 1), ImGui::GetColorU32(drawCol_normal));
		}

		if (ImGui::IsWindowHovered() || ImGui::IsAnyItemHovered()) //ImGui::IsWindowFocused()
			bImGuiRenderTargetFocus = true;

		ImGui::EndChild();

		//Drag/Drop models.
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_MODEL_DROP_TARGET", 0)) // ImGuiDragDropFlags_AcceptNoDrawDefaultRect
			{
				AddPayLoad( (ImGuiPayload*) payload,true);
			}
		}

		if (ImGui::IsWindowHovered() || ImGui::IsAnyItemHovered()) //ImGui::IsWindowFocused()
			bImGuiRenderTargetFocus = true;

		//Check if we need to point in the game scene.
		bImGuiReadyToRender = false;
		CheckTutorialPlaceit();
		bImGuiReadyToRender = true;

		if (bPopModalOpen) {
			ImGui::EndPopup();
		}
		else {
			ImGui::End();
		}

		ImGui::GetStyle().WindowPadding = iOldWindowPadding;

		extern char promptText[1024];
		if ( promptText[0] != 0 ) {
			extern int iMessageTimer;

			if (bStartNewPrompt || iMessageTimer == 0 ) {
				bStartNewPrompt = false;
				iMessageTimer = MAXTimer();
			}

			//	ImVec2 wpos = ImVec2(( GetChildWindowWidth(-1) - 460) * 0.5, 50) + ImGui::GetMainViewport()->Pos;

			ImGui::SetNextWindowPos(OldrenderTargetPos + ImVec2(50, 50), ImGuiCond_Always); //ImGuiCond_Always
			ImGui::SetNextWindowSize(ImVec2(OldrenderTargetSize.x - 100, 0), ImGuiCond_Always); //ImGuiCond_Always
			bool winopen = true;

			ImVec4* style_colors = ImGui::GetStyle().Colors;
			ImVec4 oldBgColor = style_colors[ImGuiCol_WindowBg];
			ImVec4 oldTextColor = style_colors[ImGuiCol_Text];

			float fader = ((float)MAXTimer() - (float)iMessageTimer) / 500.0f;
			fader -= 1.0;
			if (fader < 0) {
				fader = 0.0001;
			}
			fader /= 3.0;

			fader = 1.0 - fader;
			if (fader < 0.1 || MAXTimer() - iMessageTimer > 3500 ) {
				strcpy(promptText, "");
			}
			style_colors[ImGuiCol_WindowBg].x = 0.0;
			style_colors[ImGuiCol_WindowBg].y = 0.0;
			style_colors[ImGuiCol_WindowBg].z = 0.0;
			style_colors[ImGuiCol_WindowBg].w *= (fader*0.25);

			style_colors[ImGuiCol_Text].x = 1.0;
			style_colors[ImGuiCol_Text].y = 1.0;
			style_colors[ImGuiCol_Text].z = 1.0;
			style_colors[ImGuiCol_Text].w *= fader;

			ImGui::Begin("##Messageinfo2", &winopen, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoInputs);
			ImGui::SetWindowFontScale(2.0);
			ImGui::Text(" ");
			//Center Text.
			float fTextSize = ImGui::CalcTextSize(promptText).x;
			ImGui::SetCursorPos(ImVec2((ImGui::GetWindowSize().x*0.5) - (fTextSize*0.5), ImGui::GetCursorPos().y));

			ImGui::Text(promptText);
			ImGui::Text(" ");
			ImGui::SetWindowFontScale(1.0);
			ImGui::End();
			style_colors[ImGuiCol_WindowBg] = oldBgColor;
			style_colors[ImGuiCol_Text] = oldTextColor;
		}
		#endif
		#endif

		#ifdef WICKEDENGINE
		ImGui::Render();
		//ImGui_ImplDX11_RenderDrawData_Delayed(ImGui::GetDrawData()); GetDrawData() used by ImpDX11 when called from Wicked hook
		#else
		RunCode(0); //Draw imgui to backbuffer.
		ImGui::Render();
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
		#endif

		// Update and Render additional Platform Windows
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();

			//PE: We need to restore the rendertarget as we have been outside the main window.
			//			ID3D11RenderTargetView* pTmpCurrentRenderTarget;
			//			ID3D11DepthStencilView* pTmpCurrentDepthTarget;
			//			pTmpCurrentRenderTarget = g_pGlob->pCurrentRenderView;
			//			pTmpCurrentDepthTarget = g_pGlob->pCurrentDepthView;
			//			//PE: Restore rendertarget.
			//			m_pImmediateContext->OMSetRenderTargets(1, &pTmpCurrentRenderTarget, pTmpCurrentDepthTarget);

//			DBPRO_GLOBAL tagCameraData* m_ptrCam;
//			extern CCameraManager m_CameraManager;
//			m_ptrCam = m_CameraManager.GetData(0);
//			if (m_ptrCam) {
//				if (m_ptrCam->iCameraToImage > 0) {
//					SetRenderAndDepthTarget(m_ptrCam->pCameraToImageSurfaceView, m_ptrCam->pImageDepthSurfaceView);
//				}
//			}
			//UpdateSprites();
			//CameraToImage();
			//StartSceneEx(1);
		}		
		#ifdef WICKEDENGINE
		#else
		RunCode(1); // switch back to render to camera.
		#endif
		bImGuiFrameState = false;
		#ifdef WICKEDENGINE
		extern bool bRenderTabTab;
		bRenderTabTab = false;
		#endif
	}
	else
	{
		#ifdef WICKEDENGINE
		extern bool bRenderTabTab;
		extern bool bRenderNextFrame;
		if (bRenderTabTab) 
		{
			#ifdef SHOWDEBUGMEM
			ShowMemDebug();
			#endif

			ImGuiIO& io = ImGui::GetIO(); (void)io;
			ImGui::Render();
			// Update and Render additional Platform Windows
			if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
			{
				ImGui::UpdatePlatformWindows();
				ImGui::RenderPlatformWindowsDefault();
			}
			bRenderTabTab = false;
			bRenderNextFrame = true;

			// added this so title system could keep creating new frames each cycle
			bImGuiFrameState = false;
		}
		else {
			//PE: Now if any sprites commands it will be set to true.
			bRenderNextFrame = false;
		}
		#endif
	}
#endif
}

#ifdef SHOWDEBUGMEM
extern int g_iDevToolsOpen;
void ShowMemDebug(void)
{
	int get_gameisexe(void);
	if (get_gameisexe() == 1)
	{
		return;
	}
	/* community wants to see this despite HIDEHUD!
	extern int get_hidehudstate();
	if (get_hidehudstate() > 0)
	{
		return;
	}
	*/
	if (g_iDevToolsOpen < 3 && !pref.iEnableFpsMemMonitor ) return;
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImVec2 window_pos = ImVec2( (viewport->Pos.x + viewport->Size.x - 10.0f) , (viewport->Pos.y + 10.0f));
	ImDrawList* draw = ImGui::GetForegroundDrawList();
	extern ImFont* customfont;
	if (draw && customfont)
	{
		static uint32_t memupdatecount = 0;
		float wide = 200.0f;// 160;
		static float fGBMemUsed = 0;
		static float oldfps = ImGui::GetIO().Framerate;
		char memtmp[255];
		memupdatecount++;
		if(memupdatecount % 10 == 0)
			fGBMemUsed = (float)SMEMAvailable(1) / 1024.0 / 1024.0;

#ifdef INCLUDEVRAM
		//PE: Always show dedicated VRAM + system RAM GPU use.
		static float tvram = 0;
		if (memupdatecount % 13 == 0)
		{
			float GetTotalVramUsage(void);
			tvram = GetTotalVramUsage();
		}
		sprintf(memtmp, "FPS %.1f Mem %.2f VRam %.2f", (ImGui::GetIO().Framerate + oldfps) * 0.5f, fGBMemUsed, tvram / 1024.0f);
		wide = 210.0f;
#else
		//sprintf(memtmp, "FPS: %.1f Mem GB: %.3f", ImGui::GetIO().Framerate, fGBMemUsed);
		sprintf(memtmp, "FPS: %.1f Mem GB: %.3f", (ImGui::GetIO().Framerate + oldfps) * 0.5f, fGBMemUsed);
#endif
		oldfps = ImGui::GetIO().Framerate;
		draw->AddText(customfont, 15, ImVec2(window_pos.x - wide, viewport->Pos.y+4.0), IM_COL32(255, 255, 255, 255), memtmp);
	}
}
#endif

DARKSDK void ExternalDisplaySync ( int iSkipSyncRateCodeAkaFastSync )
{
	// Skip this phase if app has been shut down (always active off)
	if ( g_bAlwaysActiveOneOff ) 
		return;

	// If display not ready, can skip this
	#ifdef DX11
	if ( m_pImmediateContext == NULL )
		return;
	#endif

	// V111 - 110608 - FASTSYNC should not use sync delay!
	if ( iSkipSyncRateCodeAkaFastSync==0 )
	{
		AccurateTimer Timer;

		// Skip refreshes causing even faster FPS rates!
		if(g_dwManualSuperStepSetting>0)
		{
			if(Timer.Get()-g_dwRecordedTimer<g_dwManualSuperStepSetting)
				return;
		}

		// Force FPS
		if(g_dwSyncRateSettingSize>0)
		{
			DWORD dwDifference = GetNextSyncDelay();
			while(Timer.Get()-g_dwRecordedTimer < dwDifference)
				if(ProcessMessagesOnly()==1) return;
		}
		else
		{
			// Need to ad least process these monitors
			ConstantNonDisplayUpdate();
		}

		// u74b7 - Only update the sync timer if not using fastsync
		// Record time of update
		g_dwRecordedTimer = Timer.Last();
	}

	// leefix - 260604 - u54 - in case input wants single data-grab functionality
	ClearData();

	// 270515 - calls this to grab latest viewproj and record previous viewproj
	if ( iSkipSyncRateCodeAkaFastSync==0 )
		UpdateViewProjForMotionBlur();

	// If using External Graphics API
	// camera zero off suspends normal operations
	bool bSuspendScreenOperations = false;
	if ( !(g_dwSyncMask & 1) )
	{
		// flag the suspension of regular screen zero activity
		bSuspendScreenOperations = true;
	}
	
	if ( bSuspendScreenOperations==false )
	{
		// If BSP used, compute responses
		AutomaticEnd();

		// Draw Phase : Store backbuffer before any 3D is drawn..
		SaveSpritesBack();

		// Draw Phase : Draw Sprites Last
		if (g_bDrawSpritesFirst == false) 
		{
			UpdateSprites();
		}
			
		// Ensures AutoStuff is first to be rendered
		if(g_bDrawAutoStuffFirst==true)
		{
			if(g_bSceneStarted)
			{
				End();
				if (g_bCanRenderNow) 
				{
					ImGui_RenderLast();
					Render();
				}
			}
			g_bSceneStarted=true;
			Begin();

			// restore before-Sprites-drawn on new screen render
			RestoreSpritesBack();
		}

		// Draw Phase : Draw Sprites First
		if (g_bDrawSpritesFirst == true) 
		{
			UpdateSprites();
		}
	}

	// Draw Phase : Draw 3D Gempoetry
	// Reset polycount and drawprim count
	if ( g_pGlob ) g_pGlob->dwNumberOfPolygonsDrawn=0;
	if ( g_pGlob ) g_pGlob->dwNumberOfPrimCalls=0;

	// Disable backdrop if camera zero disabled
	int iMode = 0; if ( bSuspendScreenOperations ) iMode = 1;

	// U75 - 080410 - ensure animation in scene only calculated once (on SYNC)
	if ( iSkipSyncRateCodeAkaFastSync==0 )
		UpdateAnimationCycle();

	// Special VR rendering camera order (skip zero and copy contents of camera 6 to zero at end) 
	tagCameraData* pCam6 = NULL;
	bool bSpecialQuickVRRendering = false;
	if ( CameraExist ( 6 ) == 1 ) 
	{
		pCam6 = (tagCameraData*)GetCameraInternalData ( 6 );
		if ( pCam6 )
			if ( pCam6->iCameraToImage == -2 )
				bSpecialQuickVRRendering = true;
	}

	// Draw all 3D - all cameras loop
	StartSceneEx ( iMode, false );
	do 
	{
		int iThisCamera = 1 + GetRenderCamera();
		if ( iThisCamera <= 32 )
		{
			// camera 0 - 31 can be masked
			DWORD dwCamBit = 1;
			if ( iThisCamera > 1 ) dwCamBit = dwCamBit << (DWORD)(iThisCamera-1);
			dwCamBit = dwCamBit & g_dwSyncMask;
			if ( dwCamBit==0 ) iThisCamera = 0;
		}
		if ( iThisCamera > 0 )
		{
			// Push all polygons for 3D components
			if ( iThisCamera == 1 && bSpecialQuickVRRendering == true )
			{
				// simplified camera zero handling, render nout!
			}
			else
			{
				// regular rendering
				ExecuteRenderList();	
			}
		}
		// Next camera or finish..
	} while (FinishSceneEx ( false, false )==0);

	// on special VR mode, copy camera 6 to camera 0 (saves rendering it all)
	if ( bSpecialQuickVRRendering == true && pCam6 )
	{
		// copy camera 6 to camera 0
		if ( g_dwSyncMask & 1 )
		{
			// but only if camera zero is being rendered (i.e. SYNC call, not FASTSYNC call)
			LPGGTEXTUREREF pCam6TextureView = pCam6->pImageDepthResourceView;
			int iCam6Width = g_pGlob->iScreenWidth;
			int iCam6Height = g_pGlob->iScreenHeight;
			// correct for HMD aspect ratio
			float fCam6FinalWidth = iCam6Height;
			float fCam6FinalHeight = iCam6Height;
			// now expand so width is completely filling screen
			float fDiff = (iCam6Width-fCam6FinalWidth);
			float fDiffPerc = (fDiff/(float)iCam6Width)*2.0f;
			fCam6FinalWidth *= (1.0f+fDiffPerc);
			fCam6FinalHeight *= (1.0f+fDiffPerc);
			float fDiffX = (iCam6Width-fCam6FinalWidth);
			float fDiffY = (iCam6Height-fCam6FinalHeight);
			float fX = fDiffX/2.0f;
			float fY = fDiffY/2.0f;
			// and finally draw it
			PasteImageRaw ( pCam6TextureView, fCam6FinalWidth, fCam6FinalHeight, fX, fY, 1, 1, 0 );
		}
	}

	// After 3D operations, direct whether SPRITES/2D/IMAGE
	// drawing is to take place by default (bitmap or camera zero)
	if ( g_bDrawEntirelyToCamera==true ) RunCode ( 1 );

#ifdef USERENDERTARGET
	extern bool bImGuiInTestGame;
	if(!bImGuiInTestGame && !g_bDrawEntirelyToCamera)
		RunCode(1);
#endif

	// not suspended
	if ( bSuspendScreenOperations==false )
	{
		// Ensures AutoStuff is last to be rendered
		if(g_bDrawAutoStuffFirst==false)
		{
			if(g_bSceneStarted)
			{
				End();
				if (g_bCanRenderNow) 
				{
					ImGui_RenderLast();
					Render();
				}
			}
			g_bSceneStarted=true;
			Begin();

			// restore before-Sprites-drawn on new screen render
			RestoreSpritesBack();
		}

		// If BSP used, set response check
		AutomaticStart();
	}
}

DARKSDK void ExternalDisplayUpdate(void)
{
	// Call external sync if automatic
	if(g_bSyncOff) ExternalDisplaySync(0);
}

LRESULT CALLBACK EmptyWindowProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	// Default Action
	return DefWindowProc(hWnd, message, wParam, lParam);
}

#ifndef WICKEDENGINE
#ifdef ENABLEIMGUI
LRESULT CALLBACK ImguiWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	extern bool bImGuiInTestGame;
	extern bool bRenderTargetModalMode;

	//	if (bImGuiInTestGame) {
	//		return DefWindowProc(hWnd, message, wParam, lParam);
	//	}

		//PE: IMGUI handle messages.
	extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	if (!bImGuiInTestGame) {
		if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam)) {
			//		//return true;
		}
	}

	if (bRenderTargetModalMode)
	{
		//When in welcome system and modal mode.
		if (message == WM_SYSCOMMAND && (wParam & SC_MOVE) == SC_MOVE)
		{
			return TRUE; // Ignore if move
		}
		if (message == WM_NCLBUTTONDBLCLK) //PE: prevent window resize by mouse doubleclick on titlebar.
		{
			return TRUE;
		}
		if (message == WM_SYSCOMMAND && (wParam == SC_RESTORE || wParam == SC_MINIMIZE || wParam == SC_MAXIMIZE) )
			return TRUE; //Ignore any resize of window.
	}

	switch (message)
	{
	case WM_SETTEXT:
	{
	}

	case WM_ACTIVATE:
	{
		// 20/7/11 - Win7 - ensure we register for TOUCH over GESTURE (also allows LBUTTONDOWN to happen instantly!)
		HWND hwndPrevious = (HWND)lParam;
		if (bDetectAndActivateWindows7TouchSystem == false)
		{
			bDetectAndActivateWindows7TouchSystem = true;
			OSVERSIONINFO osvi;
			BOOL bIsWindows7orLater;
			ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
			osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
			GetVersionEx(&osvi);
			bIsWindows7orLater = ((osvi.dwMajorVersion > 6) || ((osvi.dwMajorVersion == 6) && (osvi.dwMinorVersion >= 1)));
			if (bIsWindows7orLater == TRUE)
			{
				// must dynamically find the user32.dll function and call it IF Windows 7 (allows Windows XP to run)
				typedef UINT(CALLBACK* sRegisterTouchWindowFnc)(HWND, ULONG);
				HMODULE hWinUserDLL = LoadLibrary("user32.dll");
				if (hWinUserDLL)
				{
					sRegisterTouchWindowFnc pRegTouchWin = (sRegisterTouchWindowFnc)GetProcAddress(hWinUserDLL, "RegisterTouchWindow");
					if (pRegTouchWin) BOOL bRes = pRegTouchWin(g_pGlob->hWnd, 0);
					FreeLibrary(hWinUserDLL);
				}
			}
		}

		break;
	}

	case WM_CLOSE:
	{
#ifdef DARKSDK_COMPILE
		g_iDarkGameSDKQuit = 1;
#endif
		//PostQuitMessage(0);
		if (g_pGlob->hWnd == hWnd)
		{
			// only if window being closed is main one, not the VR secondary window!
			PostQuitMessage(0);
		}
		if (g_pGlob->hOriginalhWnd == hWnd)
		{
			// clear out this invalid HWND, allows software to re-init VR!
			g_pGlob->hOriginalhWnd = NULL;
		}
		return TRUE;
	}

	case WM_DESTROY:
	case WM_NCDESTROY:
	{
		PostQuitMessage(0);
		break;
	}

	case WM_ERASEBKGND:
	{
		// Clear Device
		PAINTSTRUCT ps;
		RECT rc;
		HDC hdcClient = BeginPaint(hWnd, &ps);
		if (hdcClient) {
			GetClientRect(hWnd, &rc);
			HBRUSH bGrey = GetSysColorBrush(COLOR_3DFACE);
			HBRUSH bOld = (HBRUSH)SelectObject(hdcClient, bGrey);
			Rectangle(hdcClient, -5, -5, rc.right + 5, rc.bottom + 5);
			SelectObject(hdcClient, bOld);
			EndPaint(hWnd, &ps);
		}
	}
	return TRUE;

	case WM_SIZE:
	case WM_SIZING:
	case WM_MOVE:
	case WM_MOVING:
	case WM_PAINT:
	{
		// 180214 - record new size in glob struct
		RECT rc;

		// GDI Paint
		PAINTSTRUCT ps;
		HDC hdcClient = BeginPaint(hWnd, &ps);
		if (hdcClient)
		{
			if (g_hdcDisplay)
			{
				GetClientRect(hWnd, &rc);
				HGDIOBJ hdcOld = SelectObject(g_hdcDisplay, g_hDisplayBitmap);
				BitBlt(hdcClient, rc.left, rc.top, rc.right, rc.bottom, g_hdcDisplay, 0, 0, SRCCOPY);
				SelectObject(g_hdcDisplay, hdcOld);
			}
			else
			{
				// 210203 - if array of protected boxes setup (from controls requiring primary surface)
				if (g_pGlob->dwSafeRectMax>0)
				{
					// Clear Device
					GetClientRect(hWnd, &rc);
					HBRUSH bGrey = GetSysColorBrush(COLOR_3DFACE);
					HBRUSH bOld = (HBRUSH)SelectObject(hdcClient, bGrey);
					Rectangle(hdcClient, -5, -5, rc.right + 5, rc.bottom + 5);
					SelectObject(hdcClient, bOld);
				}
			}
			EndPaint(hWnd, &ps);
		}

		// Ensures rendered areas are retained (when moving window or menu refreshing)
		if (g_pGlob->dwAppDisplayModeUsing == 1)
		{
			// only dwDisplayMode=1 (window) should do this (otherwise render several times!!)
			// ensure refresh is not done in middle of draw-phase
			End(); Render(); Begin();
		}
	}
	return TRUE;

	case WM_MOUSEMOVE:
	{
		// Get Client Raw Mouse Position
		g_pGlob->iWindowsMouseX = LOWORD(lParam);  // horizontal position of cursor 
		g_pGlob->iWindowsMouseY = HIWORD(lParam);  // vertical position of cursor 

												   // Special Scale for When Windows Stretch Beyond Physical Size of Backbuffer
		RECT rc;
		GetClientRect(hWnd, &rc);
		float xRatio = (float)g_pGlob->dwWindowWidth / (float)rc.right;
		float yRatio = (float)g_pGlob->dwWindowHeight / (float)rc.bottom;
		g_pGlob->iWindowsMouseX = (int)((float)g_pGlob->iWindowsMouseX * xRatio);
		g_pGlob->iWindowsMouseY = (int)((float)g_pGlob->iWindowsMouseY * yRatio);

		// Restore cursor when move mouse
		//if (g_ActiveCursor != NULL) SetCursor(g_ActiveCursor);

	}
	break;

	case WM_LBUTTONDOWN:
		g_pGlob->iWindowsMouseClick |= 1;
		g_pGlob->dwWindowsMouseLeftTouchPersist = timeGetTime() + 250; // U76 - many cycles
		if (GetFocus() != hWnd)
		{
			SetFocus(hWnd);
		}
		break;

	case WM_RBUTTONDOWN:
		g_pGlob->iWindowsMouseClick |= 2;
		if (GetFocus() != hWnd) SetFocus(hWnd);
		break;

		// aaron - 20120811 - Potential issues when using xor depending on obscure and rare window interaction
	case WM_LBUTTONUP:
		g_pGlob->iWindowsMouseClick &= ~1UL;
		break;

	case WM_RBUTTONUP:
		g_pGlob->iWindowsMouseClick &= ~2UL;
		break;

	case WM_SYSKEYDOWN:
		g_wWinKey = wParam;
		break;

	case WM_KEYDOWN:
		g_wWinKey = wParam;
		//PE: Not in imgui.
//		if ((int)wParam == VK_ESCAPE)
//		{
//			if (g_EscapeValue) *(DWORD*)g_EscapeValue = 1;
//			if (g_pGlob->bEscapeKeyEnabled)
//			{
//				PostQuitMessage(0);
//			}
//		}
		return TRUE;

	case WM_SYSKEYUP:
		g_wWinKey = 0;
		return TRUE;

	case WM_KEYUP:
		g_cInkeyCodeKey = 0;
		g_wWinKey = 0;
		return TRUE;

	case WM_CHAR:

		// If win string cleared externally (InputDLL)
		if (g_pGlob->pWindowsTextEntry)
			if (g_pGlob->pWindowsTextEntry[0] == 0)
				g_dwWindowsTextEntryPos = 0;

		// Key that was pressed
		g_cKeyPressed = (unsigned char)wParam;
		g_cInkeyCodeKey = g_cKeyPressed;

		// remove this as main app can get windows entry again (no more IDE)
		//return TRUE;

		// Ensure string is always big enough
		if (g_pGlob->pWindowsTextEntry == NULL)
		{
			g_dwWindowsTextEntrySize = 32;
			g_pGlob->pWindowsTextEntry = new char[g_dwWindowsTextEntrySize];
			g_dwWindowsTextEntryPos = 0;
		}
		if (g_dwWindowsTextEntryPos>g_dwWindowsTextEntrySize - 4)
		{
			g_dwWindowsTextEntrySize = g_dwWindowsTextEntrySize * 2;
			LPSTR pNewString = new char[g_dwWindowsTextEntrySize];
			strcpy(pNewString, g_pGlob->pWindowsTextEntry);
			delete[] g_pGlob->pWindowsTextEntry;
			g_pGlob->pWindowsTextEntry = pNewString;
		}

		// Add character to entry string
		g_pGlob->pWindowsTextEntry[g_dwWindowsTextEntryPos] = g_cKeyPressed;
		g_dwWindowsTextEntryPos++;
		g_pGlob->pWindowsTextEntry[g_dwWindowsTextEntryPos] = 0;

		return TRUE;

	case WM_USER + 1: // Show/Hide Cursor
		if (wParam == 0) ShowCursor(FALSE);
		if (wParam == 1) ShowCursor(TRUE);
		return TRUE;

	case WM_DROPFILES:
	case WM_COPYDATA:
		{
			if (message == WM_COPYDATA) {

				COPYDATASTRUCT * cds = (COPYDATASTRUCT *)lParam;

				char * myString = (char *)(cds->lpData);

				if (cds->cbData > 0) {
					int size = strlen(myString);
					char* CharStr = new char[size + 1];
					strcpy(CharStr, myString);
					CharStr[size] = 0;
					if (pestrcasestr(CharStr, ".fpm")) 
					{
						extern char cDirectOpen[260];
						extern int iLaunchAfterSync;
						extern int iSkibFramesBeforeLaunch;
						//remove quotes.
						if (CharStr[0] == '\"')
							strcpy(cDirectOpen, CharStr+1);
						else
							strcpy(cDirectOpen, CharStr);
						if (cDirectOpen[strlen(cDirectOpen) - 1] == '\"')
							cDirectOpen[strlen(cDirectOpen) - 1] = 0;

						iLaunchAfterSync = 7; //Direct open.
						iSkibFramesBeforeLaunch = 2;
					}
					delete CharStr;
				}
			}
			else {
				HDROP   hDrop;
				hDrop = (HDROP)wParam;
				UINT nCnt = DragQueryFileA(hDrop, (UINT)-1, NULL, 0);
				for (int nIndex = 0; nIndex < (int)nCnt; ++nIndex) {
					UINT nSize;
					if (0 == (nSize = DragQueryFileA(hDrop, nIndex, NULL, 0)))
						continue;

					CHAR *pszFileName = new CHAR[++nSize];
					if (DragQueryFileA(hDrop, nIndex, pszFileName, nSize))
					{
						if (pszFileName && strlen(pszFileName) > 0) {
							struct stat s;
							if (stat(pszFileName, &s) == 0)
							{
								if (s.st_mode & S_IFDIR)
								{
									//PE: directory drop not used yet.
								}
								else if (s.st_mode & S_IFREG)
								{
									//PE: file dropped validate.
									if (pestrcasestr(pszFileName, ".fpm")) {
										extern char cDirectOpen[260];
										extern int iLaunchAfterSync;
										extern int iSkibFramesBeforeLaunch;
										strcpy(cDirectOpen, pszFileName);
										iLaunchAfterSync = 7; //Direct open.
										iSkibFramesBeforeLaunch = 2;
									}
								}
							}
						}
					}
					delete[] pszFileName;
				}
				DragFinish(hDrop);
			}

			return TRUE;
		}

	}

	// Default Action
	return DefWindowProc(hWnd, message, wParam, lParam);
}
