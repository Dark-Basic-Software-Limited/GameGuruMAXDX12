//
// VS2017 (32bit) having trouble with large GridEdit.cpp file, so split into two
//

// Includes 
#include "stdafx.h"
#include "gameguru.h"
#include "M-WelcomeSystem.h"
#include "M-Widget.h"
#include "GGVR.h"
#include "M-GridEditB.h"

#include <algorithm>
#include <string>
#include <time.h>
#include <wininet.h>
#include <mmsystem.h>
#include "ShlObj.h"
#include "sha1.h"
#include "sha2.h"
#include "miniz.h"
#include "Nlohmann JSON/json.hpp"

#include "M-RPG.h"
#include "M-Workshop.h"

//#define PETESTING
#ifdef PETESTING
#include "..\Imgui\imgui_demo.cpp"
#endif

// Globals
extern int iGenralWindowsFlags ;
extern bool bBoostIconColors;

// Namespaces
#include "shellapi.h"
#include ".\\..\..\\Guru-WickedMAX\\GPUParticles.h"
using namespace GPUParticles;
#include "GGTerrain/GGTerrain.h"
using namespace GGTerrain;
#include "GGTerrain/GGTrees.h"
using namespace GGTrees;
#include "GGTerrain/GGGrass.h"
using namespace GGGrass;
using namespace wiScene;

#include "..\..\GameGuru\Imgui\imnodes.h"
#include "..\..\GameGuru\Imgui\imnodes_internal.h"
extern ImNodesContext* GImNodes;

#ifdef OPTICK_ENABLE
#include "optick.h"
#endif

// Externs
extern int grideleprof_uniqui_id;
#define MAXTEXTINPUT 1024
extern float g_Storyboard_header_height;
extern char cTmpInput[MAXTEXTINPUT + 1];
extern int g_Storyboard_First_Level_Node;
extern int g_Storyboard_Current_Level;
extern char g_Storyboard_First_fpm[256];
extern char g_Storyboard_Current_fpm[256];
extern char g_Storyboard_Current_lua[256];
extern char g_Storyboard_Current_Loading_Page[256];
extern std::vector<std::string> projectbank_list;
extern std::vector<std::string> projectbank_image;
extern std::vector<int> projectbank_imageid;
extern std::vector<int> projectbank_active;
extern StoryboardStruct Storyboard;
extern StoryboardStruct checkproject;
extern StoryboardStruct202 updateproject202;
StoryboardStruct tempProjectData;
extern std::vector< std::pair<ImFont*, std::string>> StoryboardFonts;
extern bool bScreen_Editor_Window;
extern int iScreen_Editor_Node;
extern int iStoryboardExecuteKey;
extern bool bTriggerSaveAs;
extern bool bTriggerOpenProject;
extern int iDelayTriggerOpenProject;
extern bool bTriggerSaveAsAfterNewLevel;
extern char SaveProjectAsName[256];
extern char SaveProjectAsError[256];
extern bool bTriggerWhatsNewInStoryboard;
extern bool bAddWhatNewToMenu;
extern bool bOpenProjectsFromWelcome;
extern cstr TriggerLoadGameProject;
extern bool bStoryboardFirstRunSetInitPos;
extern bool bStoryboardInitNodes;
extern bool bJustRederedScreenEditor;
extern bool g_bUpdateAppAvailable;
extern bool g_bAdjustPlaneXZUsingSurfaceXZ;
extern bool g_bResetPlaneAfterXZAdjust;
extern bool g_bHoldGridEntityPosWhenManaged;
extern float g_fHoldGridEntityPosX;
extern float g_fHoldGridEntityPosY;
extern float g_fHoldGridEntityPosZ;
extern float g_fLocalTurnRotationForSmartMode ;
extern int g_iStackToSurfaceMode;
extern int g_iOrientToSurfaceMode;
extern sObject* g_selected_editor_object;
extern int g_selected_editor_objectID;
extern XMFLOAT4 g_selected_editor_color;
extern int iEditorGridSizeX;
extern int iEditorGridSizeZ;
extern bool bRenderTabTab;
extern bool bRenderNextFrame;
extern bool bNeedImGuiInput;
extern bool bProfilerEnable;
extern int iExtractMode;
extern float fExtractYValue, fExtractFixedYValue;
extern bool bExtractFixPivot;
extern bool g_bStandaloneSinglePlayer;
extern bool g_bStandaloneMultiPlayer;
extern bool g_bStandaloneVRMode;
extern bool g_bPreviewLighting;
extern int iLastOpenHeader;
extern int iExecuteCTRLkey;
extern int iExecuteALTkey;
extern int iIncludeLeftIconSet;
extern uint64_t g_hovered_dot_entity;
extern sObject* g_hovered_dot_pobject;
extern sObject* g_destination_dot_pobject;
extern sObject* g_source_dot_pobject;
extern sObject* g_selected_middle_dot_pobject;
extern bool bDotMiddleWindow;
extern ImVec2 vDotMiddleWindowPos;
#define MAXDOTMIDDLE 1000
#define DOTOBJECTIDADD 40000
#define DOTCURSOROBJECTID (70001+40000+20000)
#define DOTMIDDLEOBJECTID (70001+40000+20001)
#define MAXDOTARCSOBJECTS 20000
#define DOTARCSOBJECTID (70001+40000+20001+MAXDOTMIDDLE+1000000)
#define RELATIONOBJECTID (70001+40000+20001+MAXDOTMIDDLE)
#define RELATIONOBJECTMAX 1000
extern int iLargestDotObjectID;
extern int iLargestDotCount;
extern bool bDotObjectDragging;
extern int iDotMiddleInfoSource[MAXDOTMIDDLE];
extern int iDotMiddleInfoDestination[MAXDOTMIDDLE];
extern int iDotMiddleColor[MAXDOTMIDDLE];
extern int iDotArceColor[MAXDOTARCSOBJECTS];
extern int iDotMiddleInfoSourceType[MAXDOTMIDDLE];
extern int iDotMiddleInfoDestinationType[MAXDOTMIDDLE];
extern float fLastHitPosition[4];
extern int iCursorDotObject;
extern bool bFactionWindow[16];
extern float fDrawDotCircleTimer;
extern bool bDrawDotCircle;
extern float fDrawDotCircleRadius;
extern int fDrawDotCircleFrom;
extern bool g_bDotsAreVisible;
#define BACKBUFFERIMAGE (g.perentitypromptimageoffset+9000)
extern int BackBufferObjectID;
extern bool BackBufferSnapShotMode;
extern bool BackBufferGrabGameScreen;
extern bool BackBufferParticlesMode;
extern int iBackBufferParticlesTrigger;
extern int BackBufferParticleEmitter;
extern bool bFullScreenBackbuffer;
extern bool bSnapShotModeUseCamera;
extern bool bSnapShotModeUse2D;
extern float fSnapShotModeCameraX, fSnapShotModeCameraY, fSnapShotModeCameraZ;
extern float fSnapShotModeCameraAngX, fSnapShotModeCameraAngY, fSnapShotModeCameraAngZ;
extern cstr g_LastGroupSaved_s;
extern bool g_bBehaviorEditorActive;
extern bool library_createbehavior;
extern char library_newbehaviorname[256];
extern bool BackBufferIsGroup;
extern int BackBufferEntityID;
extern int BackBufferImageID;
extern int BackBufferSizeX;
extern int BackBufferSizeY;
extern float BackBufferRotateY, RestoreBackBufferRotateY;
extern float BackBufferRotateX, RestoreBackBufferRotateX;
extern float BackBufferRotateZ, RestoreBackBufferRotateZ;
extern float BackBufferZoom, RestoreBackBufferZoom;
extern float BackBufferCamMove;
extern float BackBufferCamLeft, RestoreBackBufferCamLeft;
extern float BackBufferCamUp, RestoreBackBufferCamUp;
extern bool bBackBufferAnimated;
extern bool bBackBufferRestoreCamera;
extern bool bEditorInFreeFlightMode;
extern bool bLoopBackBuffer;
extern bool bLoopFullFPS;
extern bool bRotateBackBuffer;
extern cstr BackBufferCacheName;
extern cstr ProjectCacheName;
extern cstr BackBufferSaveCacheName;
extern std::vector<sImageList> g_imageList;
static std::vector<sImageList> g_TempimageList;
extern int iRestoreEntidMaster;
extern int fpe_current_loaded_script;
extern int fpe_current_loaded_script_image;
extern int fpe_current_loaded_script_image_count;

extern bool bReadyToDropEntity;
extern bool bWaitOnMouseRelease;
extern bool bDraggingActive;
extern bool bDraggingActiveInitial;
extern int iDragDropActive;
#define HITPOINTYSTARTPOS GGORIGIN_Y
extern float fHitPointX, fHitPointY, fHitPointZ;
extern float fHitOffsetX, fHitOffsetY, fHitOffsetZ;
extern float fHitRayFrom, fLastHitY;
extern sObject* g_hovered_pobject;
extern bool bTriggerVisibleWidget;
extern bool bMouseInputSystemUsed;
extern int iLastHitObjectID;
extern int iStartMouseX, iStartMouseY;
extern int iObjectMoveMode;
extern int iObjectMoveModeDropSystem;
extern int iObjectMoveModeDropSystemUsing;
extern bool bObjectAllowOverlapping;
extern float fDebug, fDebug1, fDebug2, fDebug3;
extern int i_switch_group_tab;
extern int current_selected_group;
extern bool group_editing_on;
extern bool bCreateNewGroupOnNextDrop;
extern int iLastEntityOnCursor;
extern float fLastRubberBandX1;
extern float fLastRubberBandX2;
extern float fLastRubberBandY1;
extern float fLastRubberBandY2;
extern bool bDetectTerrainOnly;
extern bool bRubberBandCreated;
extern bool bDragCameraActive;
extern bool g_bThumbBankCopyMode;
extern bool g_bRefreshRotationValuesFromObjectOnce;
extern bool g_bLightProbeScaleChanged;
extern int iReusePickObjectID;
extern int iReusePickEntityID;
extern float fReusePickHitX, fReusePickHitY, fReusePickHitZ;
extern sObject* pReusePickObject;
extern std::vector<sLibraryList> g_LibraryFileList;
extern cStr cLastProjectList;
extern cstr cCurrentBackDropImageFile;
extern bool bUseBackDropImage;
extern cstr cUseBackbufferCubemap;
extern bool bBackbufferCubemapActive;
extern int iLastSelectedEntityGroup;
extern int iLastSelectedEntity;
extern int iSetSettingsFocusTab;
extern bool bStoryboardWindow;
extern bool bStoryboardWindowOpenLoad;
extern bool bMarketplace_Window;
extern bool bTriggerCloseEntityWindow;
extern bool bMarketplace_Init;
extern cstr sDefaultImportPath;
extern bool bResetObjectLibrarySize;
extern bool bWelcomeScreen_Window;
extern bool bWelcomeNoBackButton;
extern bool bWelcomeScreen_Init;
extern std::map<std::string, int> selected_library_fpe;
extern bool bProceduralLevel;
extern bool bProceduralLevelFromStoryboard;
extern int iBlackoutForFrames;
extern int iQuitProceduralLevel;
extern bool bProceduralLevelStartup;
extern int g_iUniqueGroupID;
extern cstr sGotoPreviewWithFile;
extern int iGotoPreviewType;
extern int init_Left_Categories_Column_Width;
extern int g_iDevToolsOpen;
extern bool bInvulnerableMode;
extern bool bStartInvulnerableMode;
extern int iWelcomeHeaderType;
extern int iAboutLogoType;
extern bool bTrashcanIconActive, bTrashcanIconActive2;
extern int current_sort_order;
extern int iWidgetSelection;
extern bool bRotScaleAlreadyUpdated;
extern int old_iMSAASampleCount;
extern int old_iFSRMode;
extern int old_iMSAO;
extern float old_fMSAOPower;
extern int old_iShadowSpotCascadeResolution;
extern int old_iShadowSpotResolution;
extern int old_iShadowPointResolution;
extern bool bForceRefreshLightCount;
extern int iUpdateOcean;
extern bool bEditorLight;
extern cStr sNextLevelToLoad;
extern float fMouseWheelZoomFactor;
extern bool g_bResetCameraToFreeFlightOnNewLevel;
extern float fLocalMax;
extern bool g_occluderf9Mode;
extern bool g_bBlackListRemovedSomeEntities;
extern bool gbWelcomeSystemActive;
extern int g_iWelcomeLoopPage;
extern int g_trialStampDaysLeft;
extern int g_tstoreprojectmodifiedstatic;
extern preferences pref;
extern cFolderItem MainEntityList;
extern bool g_occluderf9Mode;
extern bool g_bBlackListRemovedSomeEntities;
extern bool gbWelcomeSystemActive;
extern int g_iWelcomeLoopPage;
extern int g_trialStampDaysLeft;
extern int g_tstoreprojectmodifiedstatic;
extern bool g_bCharacterCreatorPlusActivated;
extern bool g_bDisableQuitFlag;
extern bool bEnableWeather;
extern char cImGuiDebug[2048];
extern bool bForceKey;
extern int iForceScancode;
extern cstr csForceKey;
extern bool bForceKey2;
extern cstr csForceKey2;
extern bool bForceUndo;
extern bool bForceRedo;
extern int iLaunchAfterSync;
extern int iLaunchAfterSyncAction;
extern bool bLaunchTestGameAfterLoad;
extern bool bLaunchSaveStandalonefterLoad;
extern bool bCloseStoryboardAfterLoad;
extern int iLevelEditorFromStoryboardID;
extern char pLaunchAfterSyncPreSelectModel[MAX_PATH];
extern char pLaunchAfterSyncLastImportedModel[MAX_PATH];
extern int iOldLaunchAfterSync;
extern int iSkibFramesBeforeLaunch;
extern DWORD gWindowSizeXOld;
extern DWORD gWindowSizeYOld;
extern DWORD gWindowSizeAddY;
extern DWORD gWindowSizeAddX;
extern DWORD gWindowVisibleOld;
extern DWORD gWindowPosXOld;
extern DWORD gWindowPosYOld;
extern DWORD gWindowMaximized;
extern int xmouseold, ymouseold;
extern bool bImGuiInTestGame;
extern bool bBlockImGuiUntilNewFrame;
extern bool bImGuiRenderWithNoCustomTextures;
extern bool bImGuiFrameState;
extern bool bImGuiReadyToRender;
extern bool bImGuiInitDone;
extern ImVec2 OldrenderTargetSize;
extern ImVec2 OldrenderTargetPos;
extern ImVec2 renderTargetAreaPos;
extern ImVec2 renderTargetAreaSize;
extern bool bImGuiRenderTargetFocus;
extern bool bImGuiGotFocus;
extern bool g_bCascadeQuitFlag;
extern int ImGuiStatusBar_Size;
extern char defaultWriteFolder[260];
extern bool bEntityGotFocus;
extern char cDirectOpen[260];
extern bool imgui_is_running;
extern int refresh_gui_docking;
extern ImGuiID dock_main_tabs, dock_tools_windows;
extern cstr RedockNextWindow;
extern ImGuiViewport* viewport;
extern int toolbar_size;
extern bool g_bInTutorialMode;
extern int g_iCountdownToAlphaBetaMessage;
extern ImVec4 drawCol_toogle;
extern int g_EntityClipboardAnchorEntityIndex;
extern std::vector<int> g_EntityClipboard;
extern preferences pref;
extern bool bExport_Standalone_Window;
extern bool bExport_SaveToGameCloud_Window;
extern bool bExternal_Entities_Window;
extern int iDisplayLibraryType;
extern int iDisplayLibrarySubType;
extern int iLastDisplayLibraryType;
extern cstr sStartLibrarySearchString;
extern cstr sTriggerCategorySelect;
extern int iLibraryStingReturnToID;
extern int iSelectedLibraryStingReturnID;
extern cstr sMakeDefaultSelecting;
extern cstr sSelectedLibrarySting;
extern bool bSelectLibraryViewAll;
extern bool bExternal_Entities_Init;
extern bool bEntity_Properties_Window;
extern bool bProperties_Window_Block_Mouse;
extern bool bCheckForClosing;
extern bool bCheckForClosingForce;
extern bool bBuilder_Properties_Window;
extern bool bBuilder_Left_Window;
extern bool bTerrain_Tools_Window;
extern bool bWaypoint_Window;
extern bool bDownloadStore_Window;
extern bool bImporter_Window;
extern bool bHelpVideo_Window;
extern bool bHelp_Window;
extern char cForceTutorialName[1024];
extern bool bHelp_Menu_Image_Window;
extern bool bAbout_Window;
extern bool bCredits_Window;
extern bool bBug_Reporting_Window;
extern bool bBug_RefreshBugList;
extern bool bAbout_Window_First_Run;
extern bool bCredits_Window_First_Run;
extern bool bAbout_Init;
extern bool Entity_Tools_Window;
extern bool bInfo_Window;
extern bool bInfo_Reload;
extern bool bInfo_Window_First_Run;
extern cstr cInfoMessage;
extern cstr cInfoImage, cInfoImageLast;
extern int iInfoUniqueId;
extern int g_iActiveMonitors;

extern bool Visuals_Tools_Window;
extern bool Weather_Tools_Window;
extern int iRestoreLastWindow;
extern std::vector<sRubberBandType> vEntityLockedList;
#define MAXGROUPSLISTS 100
extern cstr sEntityGroupListName[MAXGROUPSLISTS];
extern std::vector<sRubberBandType> vEntityGroupList[MAXGROUPSLISTS];
extern int iEntityGroupListImage[MAXGROUPSLISTS];
extern bool bPreferences_Window;
extern char cPreferencesMessage[MAX_PATH];
extern bool Shooter_Tools_Window; 
extern bool Puzzle_Tools_Window;
extern bool RPG_Tools_Window;
extern char cNextWindowFocus[256];
extern bool bEditGameSettings;
extern int media_icon_size_leftpanel;
extern int iColumnsWidth_leftpanel;
extern int iColumns_leftpanel;
extern bool bDisplayText_leftpanel;
extern float fFontSize_leftpanel;
extern cFolderItem::sFolderFiles *pDragDropFile;
extern int iOldgridentity;
extern float fPropertiesColoumWidth;
extern bool bTriggerMessage;
extern int iTriggerMessageDelay;
extern int iTriggerMessageY;
extern char cTriggerMessage[MAX_PATH];
extern int iMessageTimer;
extern ImVec4 drawCol_back;
extern ImVec4 drawCol_normal;
extern ImVec4 drawCol_hover;
extern ImVec4 drawCol_Down;
extern ImVec4 drawCol_black;
extern ISpObjectToken* CCP_SelectedToken;
extern LPSTR pCCPVoiceSet;
extern char CCP_SpeakText[1024];
extern wchar_t CCP_SpeakText_w[1024];
extern int CCP_Speak_Rate;
extern std::vector<cstr> tutorial_list;
extern std::map<std::string, std::string> tutorial_files;
extern std::map<std::string, std::string> tutorial_videos;
extern std::map<std::string, std::string> tutorial_description;
extern std::vector<cstr> about_text;
extern bool bTutorial_Init;
extern int current_tutorial;
extern int selected_tutorial;
extern bool bVideoPlayerMaximized;
extern bool bSmallVideoPlayerMaximized;
extern bool bLastSmallVideoPlayerMaximized;
extern bool bVideoResumePossible;
extern bool bVideoPerccentStart;
extern int iVideoFindFirstFrame;
extern int iVideoDelayExecute;
extern bool bTutorialCheckAction;
extern int bDelayedTutorialCheckAction;
extern int iDelayedCameraRestore;
#define TUTORIALMAXTEXT 1024
#define TUTORIALMAXSTEPS 20
extern char cForceTutorialName[1024];
extern char cTutorialName[TUTORIALMAXTEXT];
extern cstr cVideoDescription;
extern ActiveTutorial tut;
extern bool bTutorialRendered;
extern bool bSmallVideoFrameStart;
extern bool bSetTutorialSectionLeft;
extern int iLastTooltipSelection;
extern int iTooltipTimer;
extern int iTooltipHoveredTimer;
extern int iTooltipLastObjectId;
extern bool iTooltipAlreadyLoaded;
extern bool iTooltipObjectReady;
extern float lastKeyTime;
extern char cHelpMenuImage[MAX_PATH];
extern bool bLostFocus;
extern bool bRenderTargetModalMode;
extern int iStartupTime;
extern cstr CurrentWinTitle;
extern int speech_ids[5];
extern bool bWaypointDrawmode;
extern float custom_back_color[4];
extern bool bUpdateVeg;
extern int iLastUpdateVeg;
extern int iTriggerWelcomeSystemStuff;
extern int iCountDownToShowQuickStartDialog;
extern ImVec2 back_renderTargetAreaPos;
extern ImVec2 back_renderTargetAreaSize;
extern int back_iLastResolutionWidth;
extern int back_iLastResolutionHeight;
extern bool bFakeStandaloneTest;
extern int iTriggerGrassTreeUpdate;
extern bool Shooter_Tools_Window_Active;

typedef std::map<std::string, ISpObjectToken *> VoiceMap_t;
extern VoiceMap_t VoiceMap;
extern std::vector <cstr> g_voiceList_s;
extern std::vector <ISpObjectToken *> g_voicetoken;

extern std::vector<std::string> readoutTitles;
extern std::vector<STORYBOARD_WIDGET_> readoutWidgetTypes;
extern std::vector<ReadoutLayers> readoutLayers;
extern std::vector<ReadoutTypes> readoutTypes;
extern std::vector<std::function<void()>> readoutCallbacks;

extern int g_iAbortedAsEntityIsGroupFileModeStubOnly;
extern int g_iAbortedAsEntityIsGroupCreate;

extern bool bPreviewWPE;
extern uint32_t PreviewWPERoot;


bool bDigAHoleToHWND = false;
bool g_bSelectedMapImageTypeSpecialHelp = false;
bool bSortProjects = true;
bool bResetProjectThumbnails = false; //PE: bSortProjects will set this no need to start with true.
int g_iCheckExistingFilesModifiedDelayed = 0;
ImRect g_rStealMonitorArea;
bool bUpgradeAndBackupOldProject = false;
bool g_bTemporarilyDisableFullDecalEffectLoading = false;

std::vector<cstr> lutImages_s;

// helps track myglobals and use them in dropdowns for storyboard screen editor
bool g_bRefreshGlobalList = false;
std::vector<int> g_gameGlobalListNodeId;
std::vector<int> g_gameGlobalListIndex;
std::vector<int> g_gameGlobalListValue;
std::vector<std::string> g_gameGlobalListValueString;

// storyboard screen animation control
int g_iStoryboardScreenVideoID = 0;

// can trigger a HUD Screen to be renamed
int g_iRenameHUDScreenID = -1;
char g_pRenameHUDName[256] = "\0";
char g_pRenameHUDScreenError[256] = "\0";

bool g_bMappingKeyWindow = false;
int g_iMappingKeyToChange = -1;

bool bIncludeDocumentFolderInRemoteProject = false;
int CurrentMonitorResolutionX, CurrentMonitorResolutionY;
void GetActiveMonitorResolution( void );



void imgui_set_openproperty_flags(int iMasterID)
{
	//  Open property window
	t.editorinterfaceactive = t.e;

	//  Setup usage flags
	t.tsimplecharview = 0;
	t.tflaglives = 0; t.tflaglight = 0; t.tflagobjective = 0; t.tflagtdecal = 0; t.tflagdecalparticle = 0; t.tflagspawn = 0; t.tflagifused = 0;
	t.tflagnewparticle = 0;
	t.tflagvis = 0; t.tflagchar = 0; t.tflagweap = 0; t.tflagammo = 0; t.tflagai = 1; t.tflagsound = 0; t.tflagsoundset = 0; t.tflagnosecond = 0;
	t.tflagmobile = 0; t.tflaghurtfall = 0; t.tflaghasweapon = 0; t.tflagammoclip = 0; t.tflagstats = 0; t.tflagquantity = 0;
	t.tflagvideo = 0;
	t.tflagplayersettings = 0;
	t.tflagusekey = 0;
	t.tflagteamfield = 0;
	int tflagtext = 0;
	int tflagimage = 0;

	//  If its static and arena mode, only do optional visuals, ignore rest
	t.tstatic = 0;

	// 070510 - simplified character properties
	if (g.gsimplifiedcharacterediting == 1 && t.entityprofile[iMasterID].ischaracter == 1)
	{
		//  flag the simple character properties layout (FPGC)
		t.tsimplecharview = 1;
	}
	else
	{
		//  FPGC - 260310 - new entitylight indicated with new flag
		if (t.entityprofile[iMasterID].islightmarker == 1)
		{
			t.tflaglight = 1;
		}
		else
		{
			if (t.entityprofile[iMasterID].ismarker == 0)
			{
				t.tflagvis = 1; t.tflagmobile = 1; t.tflagobjective = 1; t.tflagsound = 1; t.tflagstats = 1; t.tflagspawn = 1;
				if (t.entityprofile[iMasterID].ischaracter > 0) { t.tflagchar = 1; t.tflagsoundset = 1; t.tflagsound = 0; }
				if (Len(t.entityprofile[iMasterID].isweapon_s.Get()) > 2) { t.tflagweap = 1; t.tflagammoclip = 1; t.tflagsound = 0; }
				if (t.entityprofile[iMasterID].isammo > 0) { t.tflagammo = 1; t.tflagobjective = 0; t.tflagsound = 0; }
				t.tflagusekey = 1;
			}
			else
			{
				if (t.entityprofile[iMasterID].ismarker == 1)
				{
					t.tflagai = 0;
					//  FPGC - 160909 - filtered fpgcgenre=1 is shooter genre
					if (g.fpgcgenre == 1)
					{
						//  Shooter legacy properties for player start
						if (t.entityprofile[iMasterID].lives > 0)
						{
							t.tflagstats = 1; t.tflaglives = 1; t.tflagsoundset = 1; t.tflaghurtfall = 1; t.tflaghasweapon = 1; t.tflagquantity = 1;
							t.tflagplayersettings = 1;
							t.tflagnosecond = 1;
						}
						else
						{
							t.tflagsound = 1; t.tflagnosecond = 1;
						}
					}
					else
					{
						//  Other genre's have no ammo quantity and weapon is renamed as equipment
						if (t.entityprofile[iMasterID].lives == -1)
						{
							//  checkpint marker is type 1
							t.tflagsound = 1; t.tflagnosecond = 1;
						}
						else
						{
							t.tflagstats = 1; t.tflaglives = 1; t.tflagsoundset = 1; t.tflaghurtfall = 1; t.tflaghasweapon = 1;
						}
					}
				}
				if (t.entityprofile[iMasterID].ismarker == 3 || t.entityprofile[iMasterID].ismarker == 6 || t.entityprofile[iMasterID].ismarker == 8)
				{
					t.tflagnosecond = 1; t.tflagifused = 1;
				}
				if (t.entityprofile[iMasterID].ismarker == 4) { t.tflagtdecal = 1; t.tflagdecalparticle = 1; }
				if (t.entityprofile[iMasterID].ismarker == 3)
				{
					if (t.entityprofile[iMasterID].markerindex <= 1)
					{
						if (t.entityprofile[iMasterID].markerindex == 1)
						{
							// video
							t.tflagvideo = 1;
						}
						else
						{
							// sound
							t.tflagsound = 1;
						}
					}
					else
					{
						if (t.entityprofile[iMasterID].markerindex == 2) tflagtext = 1;
						if (t.entityprofile[iMasterID].markerindex == 3) tflagimage = 1;
					}
				}
				if (t.entityprofile[iMasterID].ismarker == 7)
				{
					//  multiplayer start marker
					t.tflagstats = 1;
					t.tflaghurtfall = 1;
					t.tflagplayersettings = 1;
					t.tflagteamfield = 1;
				}
				if (t.entityprofile[iMasterID].ismarker == 8)
				{
					// floor zone marker
					t.tflagsound = 0;
				}
				if (t.entityprofile[iMasterID].ismarker == 9)
				{
					// cover zone marker
					t.tflagifused = 1;
				}
			}
		}
	}

	// parental control removes weapons and violence properties
	if (g.quickparentalcontrolmode == 2)
	{
		t.tflagweap = 0;
		t.tflagammo = 0;
	}

	//PE: New flags check.
	// special VR mode can remove even more
	t.tflagnotionofhealth = 1;
	t.tflagsimpler = 0;
	//if ( bVRQ2ZeroViolenceMode == true )
	//{
	//	t.tflaglives=0; 
	//	t.tflaghurtfall=0; 
	//	t.tflaghasweapon=0; 
	//	t.tflagammoclip=0;
	//	t.tflagnotionofhealth=0;
	//	t.tflagsimpler = 1;
	//}


}

// 
//  PROPERTIES
// 

void interface_openpropertywindow ( void )
{
	//  Open proprty window
	OpenFileMap (  1, "FPSEXCHANGE" );
	SetFileMapDWORD (  1, 978, 1 );
	SetFileMapDWORD (  1, 458, 0 );
	SetEventAndWait (  1 );
	t.editorinterfaceactive=t.e;

	//  open the entity file map
	OpenFileMap (  2, "FPSENTITY" );
	SetEventAndWait (  2 );

	//  wait until the entity window is read
	if (  GetFileMapDWORD( 2, ENTITY_SETUP )  ==  1 ) 
	{
		// special VRQ2 mode also hides concepts of lives, health, blood, violence (substitute health for strength)
		bool bVRQ2ZeroViolenceMode = false;
		if ( g.vrqcontrolmode != 0 ) bVRQ2ZeroViolenceMode = true;//if ( g.gvrmode == 3 ) bVRQ2ZeroViolenceMode = true;

		//  Setup usage flags
		t.tsimplecharview=0;
		t.tflaglives=0 ; t.tflaglight=0 ; t.tflagobjective=0 ; t.tflagtdecal=0 ; t.tflagdecalparticle=0 ; t.tflagspawn=0 ; t.tflagifused=0;
		t.tflagnewparticle = 0;
		t.tflagvis=0 ; t.tflagchar=0 ; t.tflagweap=0 ; t.tflagammo=0 ; t.tflagai=1 ; t.tflagsound=0 ; t.tflagsoundset=0 ; t.tflagnosecond=0;
		t.tflagmobile=0 ; t.tflaghurtfall=0 ; t.tflaghasweapon=0 ; t.tflagammoclip=0 ; t.tflagstats=0 ; t.tflagquantity=0;
		t.tflagvideo=0;
		t.tflagplayersettings=0;
		t.tflagusekey=0;
		t.tflagteamfield=0;
		int tflagtext=0;
		int tflagimage=0;

		//  If its static and arena mode, only do optional visuals, ignore rest
		t.tstatic=0;

		// 070510 - simplified character properties
		if (  g.gsimplifiedcharacterediting == 1 && t.entityprofile[t.gridentity].ischaracter == 1 ) 
		{
			//  flag the simple character properties layout (FPGC)
			t.tsimplecharview=1;
		}
		else
		{
			//  FPGC - 260310 - new entitylight indicated with new flag
			if (  t.entityprofile[t.gridentity].islightmarker == 1 ) 
			{
				t.tflaglight=1;
			}
			else
			{
				if (  t.entityprofile[t.gridentity].ismarker == 0 ) 
				{
					t.tflagvis=1 ; t.tflagmobile=1 ; t.tflagobjective=1 ; t.tflagsound=1 ; t.tflagstats=1 ; t.tflagspawn=1;
					// 070115 - removed until UBER character (multiweapon) is ready for action
					// t.entityprofile[t.gridentity].ischaracter>0 then t.tflagchar = 1  ) ; t.tflaghasweapon = 1 ; t.tflagsoundset = 1 ; t.tflagsound = 0
					if (  t.entityprofile[t.gridentity].ischaracter>0 ) { t.tflagchar = 1  ; t.tflagsoundset = 1 ; t.tflagsound = 0; }
					if (  Len(t.entityprofile[t.gridentity].isweapon_s.Get())>2 ) { t.tflagweap = 1  ; t.tflagammoclip = 1 ; t.tflagsound = 0; }
					if (  t.entityprofile[t.gridentity].isammo>0 ) { t.tflagammo = 1  ; t.tflagobjective = 0 ; t.tflagsound = 0; }
					t.tflagusekey=1;
				}
				else
				{
					if (  t.entityprofile[t.gridentity].ismarker == 1 ) 
					{
						t.tflagai=0;
						//  FPGC - 160909 - filtered fpgcgenre=1 is shooter genre
						if (  g.fpgcgenre == 1 ) 
						{
							//  Shooter legacy properties for player start
							if (  t.entityprofile[t.gridentity].lives>0 ) 
							{
								t.tflagstats=1 ; t.tflaglives=1 ; t.tflagsoundset=1 ; t.tflaghurtfall=1 ; t.tflaghasweapon=1 ; t.tflagquantity=1;
								t.tflagplayersettings=1;
								t.tflagnosecond=1;
							}
							else
							{
								t.tflagsound=1 ; t.tflagnosecond=1;
							}
						}
						else
						{
							//  Other genre's have no ammo quantity and weapon is renamed as equipment
							if (  t.entityprofile[t.gridentity].lives == -1 ) 
							{
								//  checkpint marker is type 1
								t.tflagsound=1 ; t.tflagnosecond=1;
							}
							else
							{
								t.tflagstats=1 ; t.tflaglives=1 ; t.tflagsoundset=1 ; t.tflaghurtfall=1 ; t.tflaghasweapon=1;
							}
						}
					}
					if (  t.entityprofile[t.gridentity].ismarker == 3 || t.entityprofile[t.gridentity].ismarker == 6 || t.entityprofile[t.gridentity].ismarker == 8 ) 
					{
						t.tflagnosecond=1 ; t.tflagifused=1;
					}
					if (  t.entityprofile[t.gridentity].ismarker == 4 ) { t.tflagtdecal = 1  ; t.tflagdecalparticle = 1; }
					// handled next to Behavior component for MAX
					if (  t.entityprofile[t.gridentity].ismarker == 3 ) 
					{
						if (  t.entityprofile[t.gridentity].markerindex <= 1 ) 
						{
							if (  t.entityprofile[t.gridentity].markerindex == 1 ) 
							{
								// video
								t.tflagvideo=1;
							}
							else
							{
								// sound
								t.tflagsound=1;
							}
						}
						else
						{
							if ( t.entityprofile[t.gridentity].markerindex == 2 ) tflagtext=1;
						}
					}
					if (  t.entityprofile[t.gridentity].ismarker == 7 ) 
					{
						//  multiplayer start marker
						t.tflagstats=1;
						t.tflaghurtfall=1;
						t.tflagplayersettings=1;
						t.tflagteamfield=1;
					}
					if (  t.entityprofile[t.gridentity].ismarker == 8 ) 
					{
						// floor zone marker
						t.tflagsound=0;
					}
					if (  t.entityprofile[t.gridentity].ismarker == 9 ) 
					{
						// cover zone marker
						t.tflagifused = 1;
					}
				}
			}
		}

		// parental control removes weapons and violence properties
		if ( g.quickparentalcontrolmode == 2 )
		{
			t.tflagweap = 0;
			t.tflagammo = 0;
		}

		// special VR mode can remove even more
		t.tflagnotionofhealth = 1;
		t.tflagsimpler = 0;
		if ( bVRQ2ZeroViolenceMode == true )
		{
			t.tflaglives=0; 
			t.tflaghurtfall=0; 
			t.tflaghasweapon=0; 
			t.tflagammoclip=0;
			t.tflagnotionofhealth=0;
			t.tflagsimpler = 1;
		}

		//  set array and counters to track scope of contents of each group
		Dim (  t.propfield,16  );
		for ( t.t = 0 ; t.t <= 16 ; t.t++ ) t.propfield[t.t]=0 ; 

		//  set the window title
		setpropertybase(ENTITY_WINDOW_TITLE,t.strarr_s[411].Get());

		//  FPGC - 070510 - open entity properties filemap and wait for signal to write
		OpenFileMap ( 3, "ENTITYPROPERTIES" );
		g.g_filemapoffset = 8;
		if ( DLLExist(1) == 0 )  DLLLoad (  "Kernel32.dll", 1 );
		while (  GetFileMapDWORD(3,0)  ==  1 ) 
		{
			CallDLL (  1,"Sleep",10 );
		}

		if (  t.tsimplecharview == 1 ) 
		{
			//  Wizard (simplified) property editing
			t.group=0 ; startgroup("Character Info") ; t.controlindex=0;
			setpropertystring2(t.group,t.grideleprof.name_s.Get(),t.strarr_s[413].Get(),"Choose a unique name for this character") ; ++t.controlindex;
			setpropertylist2(t.group,t.controlindex,t.grideleprof.aimain_s.Get(),"Behaviour","Select a behaviour for this character",11) ; ++t.controlindex;
			setpropertyfile2(t.group,t.grideleprof.soundset1_s.Get(),"Voiceover","Select t.a WAV or OGG file this character will use during their behavior","audiobank\\") ; ++t.controlindex;
			setpropertystring2(t.group,t.grideleprof.ifused_s.Get(),"If Used","Sometimes used to specify the name of an entity to be activated") ; ++t.controlindex;
		}
		else
		{
			//  Name
			t.group=0 ; startgroup(t.strarr_s[412].Get()) ; t.controlindex=0;
			if ( t.entityprofile[t.gridentity].ischaracter > 0 )
			{
				setpropertystring2(t.group,t.grideleprof.name_s.Get(),t.strarr_s[478].Get(),t.strarr_s[204].Get());
			}
			else
			{
				if ( t.entityprofile[t.gridentity].ismarker > 0 )
				{
					if ( t.entityprofile[t.gridentity].islightmarker > 0 )
						setpropertystring2(t.group,t.grideleprof.name_s.Get(),t.strarr_s[483].Get(),t.strarr_s[204].Get());
					else
						setpropertystring2(t.group,t.grideleprof.name_s.Get(),t.strarr_s[479].Get(),t.strarr_s[204].Get());
				}
				else
					setpropertystring2(t.group,t.grideleprof.name_s.Get(),t.strarr_s[413].Get(),t.strarr_s[204].Get());
			}
			++t.controlindex;
			if (  t.entityprofile[t.gridentity].ismarker == 0 || t.entityprofile[t.gridentity].islightmarker == 1 ) 
			{
				if (  g.gentitytogglingoff == 0 ) 
				{
					t.tokay=1;
					if (  ObjectExist(g.entitybankoffset+t.gridentity) == 1 ) 
					{
						if (  GetNumberOfFrames(g.entitybankoffset+t.gridentity)>0 ) 
						{
							t.tokay=0;
						}
					}
					if (  t.tokay == 1 ) 
					{
						//PE: 414=Static Mode
						setpropertylist2(t.group,t.controlindex,Str(t.gridentitystaticmode),t.strarr_s[414].Get(),t.strarr_s[205].Get(),0) ; ++t.controlindex;
					}
				}
			}

			// 101016 - Additional General Parameters
			if ( t.tflagchar == 0 && t.tflagvis == 1 ) 
			{
				if ( t.tflagsimpler == 0 )
				{
					setpropertylist2(t.group,t.controlindex,Str(t.grideleprof.isocluder),"Occluder","Set to YES makes this object an occluder",0) ; ++t.controlindex;
					setpropertylist2(t.group,t.controlindex,Str(t.grideleprof.isocludee),"Occludee","Set to YES makes this object an occludee",0) ; ++t.controlindex;
				}
			}

			//  Basic AI
			if (  t.tflagai == 1 ) 
			{
				// can redirect to better folders if in g.quickparentalcontrolmode
				LPSTR pAIRoot = "scriptbank\\";
				if ( g.quickparentalcontrolmode == 2 )
				{
					if ( t.entityprofile[t.gridentity].ismarker == 0 ) 
					{
						if ( t.tflagchar == 1 )
							pAIRoot = "scriptbank\\people\\";
						else
							pAIRoot = "scriptbank\\objects\\";
					}
					else
					{
						pAIRoot = "scriptbank\\markers\\";
					}
				}

				t.propfield[t.group]=t.controlindex;
				++t.group ; startgroup(t.strarr_s[415].Get()) ; t.controlindex=0;
				setpropertyfile2(t.group,t.grideleprof.aimain_s.Get(),t.strarr_s[417].Get(),t.strarr_s[207].Get(),pAIRoot) ; ++t.controlindex;
			}

			//  Has Weapon
			if (  t.tflaghasweapon == 1 && t.playercontrol.thirdperson.enabled == 0 && g.quickparentalcontrolmode != 2 ) 
			{
				setpropertylist2(t.group,t.controlindex,t.grideleprof.hasweapon_s.Get(),t.strarr_s[419].Get(),t.strarr_s[209].Get(),1) ; ++t.controlindex;
			}

			//  Is Weapon (FPGC - 280809 - filtered fpgcgenre=1 is shooter genre)
			if (  t.tflagweap == 1 && g.fpgcgenre == 1 ) 
			{
				setpropertystring2(t.group,Str(t.grideleprof.damage),t.strarr_s[420].Get(),t.strarr_s[210].Get()) ; ++t.controlindex;
				setpropertystring2(t.group,Str(t.grideleprof.accuracy),t.strarr_s[421].Get(),"Increases the inaccuracy of conical distribution by 1/100th of t.a degree") ; ++t.controlindex;
				if (  t.grideleprof.weaponisammo == 0 ) 
				{
					setpropertystring2(t.group,Str(t.grideleprof.reloadqty),t.strarr_s[422].Get(),t.strarr_s[212].Get()) ; ++t.controlindex;
					setpropertystring2(t.group,Str(t.grideleprof.fireiterations),t.strarr_s[423].Get(),t.strarr_s[213].Get()) ; ++t.controlindex;
					setpropertystring2(t.group,Str(t.grideleprof.range),"Range","Maximum range of bullet travel") ; ++t.controlindex;
					setpropertystring2(t.group, Str(t.grideleprof.dropoff), "Dropoff", "Amount in inches of vertical dropoff per 100 feet of bullet travel"); ++t.controlindex;
					setpropertystring2(t.group, Str(t.grideleprof.clipcapacity), "Clip Capacity", "The total maximum number of clips the player can carry for this weapon"); ++t.controlindex;
				}
				else
				{
					setpropertystring2(t.group,Str(t.grideleprof.lifespan),t.strarr_s[424].Get(),t.strarr_s[214].Get()) ; ++t.controlindex;
					setpropertystring2(t.group,Str(t.grideleprof.throwspeed),t.strarr_s[425].Get(),t.strarr_s[215].Get()) ; ++t.controlindex;
					setpropertystring2(t.group,Str(t.grideleprof.throwangle),t.strarr_s[426].Get(),t.strarr_s[216].Get()) ; ++t.controlindex;
					setpropertystring2(t.group,Str(t.grideleprof.bounceqty),t.strarr_s[427].Get(),t.strarr_s[217].Get()) ; ++t.controlindex;
					setpropertylist2(t.group,t.controlindex,Str(t.grideleprof.explodeonhit),t.strarr_s[428].Get(),t.strarr_s[218].Get(),0) ; ++t.controlindex;
				}
				if ( t.tflagsimpler == 0 )
				{
					setpropertylist2(t.group,t.controlindex,Str(t.grideleprof.usespotlighting),"Spot Lighting","Set whether emits dynamic spot lighting",0) ; ++t.controlindex;
				}
			}

			//  Is Character
			if (  t.tflagchar == 1 ) 
			{
				if ( t.tflagsimpler == 0 )
				{
					// 020316 - special check to avoid offering can take weapon if no HUD.X
					t.tfile_s = cstr("gamecore\\guns\\") + t.grideleprof.hasweapon_s + cstr("\\HUD.X");
					if ( FileExist(t.tfile_s.Get()) == 1 ) 
					{
						setpropertylist2(t.group,t.controlindex,Str(t.grideleprof.cantakeweapon),t.strarr_s[429].Get(),t.strarr_s[219].Get(),0) ; ++t.controlindex;
						setpropertystring2(t.group,Str(t.grideleprof.quantity),t.strarr_s[430].Get(),t.strarr_s[220].Get()) ; ++t.controlindex;
					}
					setpropertystring2(t.group,Str(t.grideleprof.rateoffire),t.strarr_s[431].Get(),t.strarr_s[221].Get()) ; ++t.controlindex;
				}
			}
			if ( t.tflagquantity == 1 && g.quickparentalcontrolmode != 2 ) 
			{ 
				setpropertystring2(t.group,Str(t.grideleprof.quantity),t.strarr_s[432].Get(),t.strarr_s[222].Get())  ; ++t.controlindex; 
			}

			//  AI Extra
			if (  t.tflagvis == 1 && t.tflagai == 1 ) 
			{
				if (  t.tflagchar == 1 ) 
				{
					setpropertystring2(t.group,Str(t.grideleprof.coneangle),t.strarr_s[434].Get(),t.strarr_s[224].Get()) ; ++t.controlindex;
					setpropertystring2(t.group,Str(t.grideleprof.conerange),t.strarr_s[476].Get(),"The range within which the AI may see the player. Zero triggers the characters default range.") ; ++t.controlindex;
					setpropertystring2(t.group,t.grideleprof.ifused_s.Get(),t.strarr_s[437].Get(),t.strarr_s[226].Get()) ; ++t.controlindex;
					if ( g.quickparentalcontrolmode != 2 )
					{
						setpropertylist2(t.group,t.controlindex,Str(t.grideleprof.isviolent),"Blood Effects","Sets whether blood and screams should be used",0) ; ++t.controlindex;
					}
					if ( t.tflagsimpler == 0 )
					{
						setpropertylist2(t.group,t.controlindex,Str(t.grideleprof.colondeath),"End Collision","Set to NO switches off collision when die",0) ; ++t.controlindex;
					}
				}
				else
				{
					if (  t.tflagweap == 0 && t.tflagammo == 0 ) 
					{
						t.propfield[t.group]=t.controlindex;
						++t.group ; startgroup(t.strarr_s[435].Get()) ; t.controlindex=0;
						setpropertystring2(t.group,t.grideleprof.usekey_s.Get(),t.strarr_s[436].Get(),t.strarr_s[225].Get()) ; ++t.controlindex;
						if ( t.tflagsimpler != 0 & t.entityprofile[t.gridentity].ismarker == 3 && t.entityprofile[t.gridentity].trigger.stylecolor == 1 )
						{
							// only one level - no winzone chain option
						}
						else
						{
							setpropertystring2(t.group,t.grideleprof.ifused_s.Get(),t.strarr_s[437].Get(),t.strarr_s[226].Get()) ; ++t.controlindex;
						}
					}
				}
			}
			if (  t.tflagifused == 1 ) 
			{
				if (  t.tflagusekey == 1 ) 
				{
					setpropertystring2(t.group,t.grideleprof.usekey_s.Get(),t.strarr_s[436].Get(),t.strarr_s[225].Get()) ; ++t.controlindex;
				}
				if ( t.tflagsimpler != 0 & t.entityprofile[t.gridentity].ismarker == 3 && t.entityprofile[t.gridentity].trigger.stylecolor == 1 )
				{
					// only one level - no winzone chain option
				}
				else
				{
					setpropertystring2(t.group,t.grideleprof.ifused_s.Get(),t.strarr_s[437].Get(),t.strarr_s[227].Get()) ; ++t.controlindex;
				}
			}

			//  Spawn Settings
			if (  t.tflagspawn == 1 ) 
			{
				t.propfield[t.group]=t.controlindex;
				++t.group ; startgroup(t.strarr_s[439].Get()) ; t.controlindex=0;
				setpropertylist2(t.group,t.controlindex,Str(t.grideleprof.spawnatstart),t.strarr_s[562].Get(),t.strarr_s[563].Get(),0) ; ++t.controlindex;
			}

			//  Statistics
			if (  (t.tflagvis == 1 || t.tflagobjective == 1 || t.tflaglives == 1 || t.tflagstats == 1) && t.tflagweap == 0 && t.tflagammo == 0 ) 
			{
				t.propfield[t.group]=t.controlindex;
				++t.group ; startgroup(t.strarr_s[451].Get()) ; t.controlindex=0;
				if (  t.tflagplayersettings == 1 ) 
				{
					setpropertystring2(t.group,Str(t.playercontrol.jumpmax_f),"Jump Speed","Sets the jump speed of the player which controls overall jump height") ; ++t.controlindex;
					setpropertystring2(t.group,Str(t.playercontrol.gravity_f),"Gravity","Sets the modified force percentage of the players own gravity") ; ++t.controlindex;
					setpropertystring2(t.group,Str(t.playercontrol.fallspeed_f),"Fall Speed","Sets the maximum speed percentage at which the player will fall") ; ++t.controlindex;
					setpropertystring2(t.group,Str(t.playercontrol.climbangle_f),"Climb Angle","Sets the maximum angle permitted for the player to ascend a slope") ; ++t.controlindex;
					if (  t.playercontrol.thirdperson.enabled == 0 ) 
					{
						setpropertystring2(t.group,Str(t.playercontrol.wobblespeed_f),"Wobble Speed","Sets the rate of motion applied to the camera when moving") ; ++t.controlindex;
						setpropertystring2(t.group,Str(t.playercontrol.wobbleheight_f*100),"Wobble Height","Sets the degree of motion applied to the camera when moving") ; ++t.controlindex;
						setpropertystring2(t.group,Str(t.playercontrol.footfallpace_f*100),"Footfall Pace","Sets the rate at which the footfall sound is played when moving") ; ++t.controlindex;
					}
					setpropertystring2(t.group,Str(t.playercontrol.accel_f*100),"Acceleration","Sets the acceleration curve used when t.moving from t.a stood position") ; ++t.controlindex;
				}
				if ( t.tflagmobile == 1 ) { setpropertylist2(t.group,t.controlindex,Str(t.grideleprof.isimmobile),t.strarr_s[457].Get(),t.strarr_s[247].Get(),0); ++t.controlindex; }
				if ( t.tflagmobile == 1 ) 
				{ 
					if ( t.tflagsimpler == 0 )
					{
						setpropertystring2(t.group,Str(t.grideleprof.lodmodifier),"LOD Modifier","Modify when the LOD transition takes effect. The default value is 0, increase this to a percentage reduce the LOD effect.") ; ++t.controlindex; 
					}
				}
			}

			//  Team field

			//  Physics Data (non-multiplayer)
			if (  t.entityprofile[t.gridentity].ismarker == 0 && t.entityprofile[t.gridentity].islightmarker == 0 ) 
			{
				t.propfield[t.group]=t.controlindex;
				++t.group ; startgroup(t.strarr_s[596].Get()) ; t.controlindex=0;
				if (  t.grideleprof.physics != 1  )  t.grideleprof.physics = 0;
				setpropertylist2(t.group,t.controlindex,Str(t.grideleprof.physics),t.strarr_s[580].Get(),t.strarr_s[581].Get(),0) ; ++t.controlindex;
				setpropertylist2(t.group,t.controlindex,Str(t.grideleprof.phyalways),t.strarr_s[582].Get(),t.strarr_s[583].Get(),0) ; ++t.controlindex;
				setpropertystring2(t.group,Str(t.grideleprof.phyweight),t.strarr_s[584].Get(),t.strarr_s[585].Get()) ; ++t.controlindex;
				setpropertystring2(t.group,Str(t.grideleprof.phyfriction),t.strarr_s[586].Get(),t.strarr_s[587].Get()) ; ++t.controlindex;
				if ( t.tflagsimpler == 0 )
				{
					setpropertylist2(t.group,t.controlindex,Str(t.grideleprof.explodable),t.strarr_s[592].Get(),t.strarr_s[593].Get(),0) ; ++t.controlindex;
					setpropertystring2(t.group,Str(t.grideleprof.explodedamage),t.strarr_s[594].Get(),t.strarr_s[595].Get()) ; ++t.controlindex;
				}
			}

			//  Ammo data (FPGC - 280809 - filtered fpgcgenre=1 is shooter genre
			if (  g.fpgcgenre == 1 ) 
			{
				if (  t.tflagammo == 1 || t.tflagammoclip == 1 ) 
				{
					t.propfield[t.group]=t.controlindex;
					++t.group ; startgroup(t.strarr_s[459].Get()) ; t.controlindex=0;
					setpropertystring2(t.group,Str(t.grideleprof.quantity),t.strarr_s[460].Get(),t.strarr_s[249].Get()) ; ++t.controlindex;
				}
			}

			//  Light data
			if (  t.tflaglight == 1 ) 
			{
				t.propfield[t.group]=t.controlindex;
				++t.group ; startgroup(t.strarr_s[461].Get()) ; t.controlindex=0; //PE: 461=Light
				setpropertystring2(t.group,Str(t.grideleprof.light.range),t.strarr_s[462].Get(),t.strarr_s[250].Get()) ; ++t.controlindex; //PE: 462=Light Range
				setpropertycolor2(t.group,t.grideleprof.light.color,t.strarr_s[463].Get(),t.strarr_s[251].Get()) ; ++t.controlindex; //PE: 463=Light Color
				if ( t.tflagsimpler == 0 )
				{
					setpropertylist2(t.group, t.controlindex, Str(t.grideleprof.usespotlighting), "Spot Lighting", "Change dynamic light to spot lighting", 0); ++t.controlindex;
				}
			}

			//  Decal data
			if (  t.tflagtdecal == 1 ) 
			{
				t.propfield[t.group]=t.controlindex;

				//  Decal Particle data
				if (  t.tflagdecalparticle == 1 ) 
				{
					++t.group ; startgroup("Decal Particle") ; t.controlindex=0;
					setpropertylist2(t.group,t.controlindex,Str(t.grideleprof.particleoverride),"Custom Settings","Whether you wish to override default settings",0) ; ++t.controlindex;
					setpropertystring2(t.group,Str(t.grideleprof.particle.offsety),"OffsetY","Vertical adjustment of start position") ; ++t.controlindex;
					setpropertystring2(t.group,Str(t.grideleprof.particle.scale),"Scale","A value from 0 to 100, denoting size of particle") ; ++t.controlindex;
					setpropertystring2(t.group,Str(t.grideleprof.particle.randomstartx),"Random Start X","Random start area") ; ++t.controlindex;
					setpropertystring2(t.group,Str(t.grideleprof.particle.randomstarty),"Random Start Y","Random start area") ; ++t.controlindex;
					setpropertystring2(t.group,Str(t.grideleprof.particle.randomstartz),"Random Start Z","Random start area") ; ++t.controlindex;
					setpropertystring2(t.group,Str(t.grideleprof.particle.linearmotionx),"Linear Motion X","Constant motion direction") ; ++t.controlindex;
					setpropertystring2(t.group,Str(t.grideleprof.particle.linearmotiony),"Linear Motion Y","Constant motion direction") ; ++t.controlindex;
					setpropertystring2(t.group,Str(t.grideleprof.particle.linearmotionz),"Linear Motion Z","Constant motion direction") ; ++t.controlindex;
					setpropertystring2(t.group,Str(t.grideleprof.particle.randommotionx),"Random Motion X","Random motion direction") ; ++t.controlindex;
					setpropertystring2(t.group,Str(t.grideleprof.particle.randommotiony),"Random Motion Y","Random motion direction") ; ++t.controlindex;
					setpropertystring2(t.group,Str(t.grideleprof.particle.randommotionz),"Random Motion Z","Random motion direction") ; ++t.controlindex;
					setpropertystring2(t.group,Str(t.grideleprof.particle.mirrormode),"Mirror Mode","Set to one to reverse the particle") ; ++t.controlindex;
					setpropertystring2(t.group,Str(t.grideleprof.particle.camerazshift),"Camera Z Shift","Shift t.particle towards camera") ; ++t.controlindex;
					setpropertystring2(t.group,Str(t.grideleprof.particle.scaleonlyx),"Scale Only X","Percentage X over Y scale") ; ++t.controlindex;
					setpropertystring2(t.group,Str(t.grideleprof.particle.lifeincrement),"Life Increment","Control lifespan of particle") ; ++t.controlindex;
					setpropertystring2(t.group,Str(t.grideleprof.particle.alphaintensity),"Alpha Intensity","Control alpha percentage of particle") ; ++t.controlindex;
					//  V118 - 060810 - knxrb - Decal animation setting (Added animation choice setting).
					setpropertylist2(t.group,t.controlindex,Str(t.grideleprof.particle.animated),"Animated Text (  ( ure","Sets whether the t.particle t.decal Texture is animated or static.", 0)  ; ++t.controlindex;
				}
			}

			// Sound
			if ( t.tflagsound == 1 || t.tflagsoundset == 1 || tflagtext == 1 || tflagimage == 1 ) 
			{
				t.propfield[t.group]=t.controlindex;
				++t.group ;
				if ( tflagtext == 1 || tflagimage == 1 )
				{
					if ( tflagtext == 1 ) startgroup("Text");
					if ( tflagimage == 1 ) startgroup("Image");
				}
				else
				{
					startgroup("Media");
				}
				t.controlindex=0;
				if ( g.fpgcgenre == 1 ) 
				{
					if ( g.vrqcontrolmode != 0 )
					{
						if ( t.tflagsound == 1 ) { setpropertyfile2(t.group,t.grideleprof.soundset_s.Get(),t.strarr_s[469].Get(),t.strarr_s[253].Get(),"audiobank\\")  ; ++t.controlindex; }
					}
					else
					{
						if ( t.tflagsound == 1 ) { setpropertyfile2(t.group,t.grideleprof.soundset_s.Get(),t.strarr_s[467].Get(),t.strarr_s[253].Get(),"audiobank\\")  ; ++t.controlindex; }
					}
					if ( t.tflagsoundset == 1 ) { setpropertyfile2(t.group,t.grideleprof.soundset_s.Get(),t.strarr_s[469].Get(),t.strarr_s[255].Get(),"audiobank\\voices\\")  ; ++t.controlindex; }
					if ( tflagtext == 1 ) { setpropertystring2(t.group,t.grideleprof.soundset_s.Get(),"Text to Appear","Enter text to appear in-level") ; ++t.controlindex; }
					if ( tflagimage == 1 ) { setpropertyfile2(t.group,t.grideleprof.soundset_s.Get(),"Image File","Select image to appear in-level","scriptbank\\images\\imagesinzone\\") ; ++t.controlindex; }
					if ( t.tflagnosecond == 0 ) 
					{
						if ( t.tflagsound == 1 || t.tflagsoundset == 1 )
						{ 
							setpropertyfile2(t.group,t.grideleprof.soundset1_s.Get(),t.strarr_s[468].Get(),t.strarr_s[254].Get(),"audiobank\\")  ; ++t.controlindex; 
							setpropertyfile2(t.group,t.grideleprof.soundset2_s.Get(),t.strarr_s[480].Get(),t.strarr_s[254].Get(),"audiobank\\")  ; ++t.controlindex; 
							setpropertyfile2(t.group,t.grideleprof.soundset3_s.Get(),t.strarr_s[481].Get(),t.strarr_s[254].Get(),"audiobank\\")  ; ++t.controlindex; 
							setpropertyfile2(t.group, t.grideleprof.soundset5_s.Get(), t.strarr_s[482].Get(), t.strarr_s[254].Get(), "audiobank\\"); ++t.controlindex;
							setpropertyfile2(t.group, t.grideleprof.soundset6_s.Get(), "Sound5", t.strarr_s[254].Get(), "audiobank\\"); ++t.controlindex;
						}
					}
				}
				else
				{
					if ( t.tflagsoundset == 1 ) 
					{
						setpropertyfile2(t.group,t.grideleprof.soundset_s.Get(),t.strarr_s[469].Get(),t.strarr_s[255].Get(),"audiobank\\voices\\") ; ++t.controlindex;
					}
					else
					{
						setpropertyfile2(t.group,t.grideleprof.soundset_s.Get(),t.strarr_s[467].Get(),t.strarr_s[253].Get(),"audiobank\\") ; ++t.controlindex;
					}
					setpropertyfile2(t.group,t.grideleprof.soundset1_s.Get(),t.strarr_s[468].Get(),t.strarr_s[254].Get(),"audiobank\\") ; ++t.controlindex;
				}
			}

			// Video
			if ( t.tflagvideo == 1 ) 
			{
				t.propfield[t.group]=t.controlindex;
				++t.group ; startgroup(t.strarr_s[597].Get()) ; t.controlindex=0;
				setpropertyfile2(t.group,t.grideleprof.soundset_s.Get(),t.strarr_s[469].Get(),t.strarr_s[599].Get(),"audiobank\\") ; ++t.controlindex;
				setpropertyfile2(t.group,t.grideleprof.soundset1_s.Get(),"Video Slot",t.strarr_s[601].Get(),"videobank\\") ; ++t.controlindex;
			}

			//  Third person settings
			if (  t.tflagplayersettings == 1 && t.playercontrol.thirdperson.enabled == 1 ) 
			{
				t.propfield[t.group]=t.controlindex;
				++t.group ; startgroup("Third Person") ; t.controlindex=0;
				t.livegroupforthirdperson=t.group;
				setpropertylist2(t.group,t.controlindex,Str(t.playercontrol.thirdperson.cameralocked),"Camera Locked","Fixes camera height and angle for third person view",0) ; ++t.controlindex;
				setpropertystring2(t.group,Str(t.playercontrol.thirdperson.cameradistance),"Camera Distance","Sets the distance of the third person camera") ; ++t.controlindex;
				setpropertystring2(t.group,Str(t.playercontrol.thirdperson.camerashoulder),"Camera X Offset","Sets the distance to shift the camera over shoulder") ; ++t.controlindex;
				setpropertystring2(t.group,Str(t.playercontrol.thirdperson.cameraheight),"Camera Y Offset","Sets the vertical height of the third person camera. If more than twice the camera distance, camera collision disables") ; ++t.controlindex;
				setpropertystring2(t.group,Str(t.playercontrol.thirdperson.camerafocus),"Camera Focus","Sets the camera X angle offset to align focus of the third person camera") ; ++t.controlindex;
				setpropertystring2(t.group,Str(t.playercontrol.thirdperson.cameraspeed),"Camera Speed","Sets the retraction speed percentage of the third person camera") ; ++t.controlindex;
				setpropertylist2(t.group,t.controlindex,Str(t.playercontrol.thirdperson.camerafollow),"Run Mode","If set to yes, protagonist uses WASD t.movement mode",0) ; ++t.controlindex;
				setpropertylist2(t.group,t.controlindex,Str(t.playercontrol.thirdperson.camerareticle),"Show Reticle","Show the third person 'crosshair' reticle Dot ( ",0)  ; ++t.controlindex;
			}

		}

		//  End of data
		t.propfield[t.group]=t.controlindex;
		t.propfieldgroupmax=t.group;

		//  FPGC - 070510 - finish bulk entity properties population
		SetFileMapDWORD (  3,g.g_filemapoffset,0  ); g.g_filemapoffset += 4;
		SetFileMapDWORD (  3,0,1 );

	}

	//  FPGC - 070510 - close bulk file map
	SetEventAndWait ( 2 );
}

void interface_copydatatoentity ( void )
{
	//  go through all active fields
	for ( t.iGroup = 0 ; t.iGroup<=  t.propfieldgroupmax; t.iGroup++ )
	{
		for ( t.iControl = 0 ; t.iControl<=  t.propfield[t.iGroup]-1; t.iControl++ )
		{

			//  Get data
			t.tfield_s = getpropertyfield(t.iGroup,t.iControl);
			t.tdata_s = getpropertydata(t.iGroup,t.iControl);

			//  If tdata$ was absolute file, truncate to remove first part
			if ( t.tdata_s.Get()[1] == ':' )
			{
				t.chopthis_s=g.rootdir_s;
				if (  strcmp ( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[413].Get()) ) == 0 )  t.chopthis_s = t.chopthis_s+"scriptbank\\";
				if (  strcmp ( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[478].Get()) ) == 0 )  t.chopthis_s = t.chopthis_s+"scriptbank\\";
				if (  strcmp ( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[479].Get()) ) == 0 )  t.chopthis_s = t.chopthis_s+"scriptbank\\";
				if (  strcmp ( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[416].Get()) ) == 0 )  t.chopthis_s = t.chopthis_s+"scriptbank\\";
				if (  strcmp ( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[561].Get()) ) == 0 )  t.chopthis_s = t.chopthis_s+"scriptbank\\";
				if (  strcmp ( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[417].Get()) ) == 0 )  t.chopthis_s = t.chopthis_s+"scriptbank\\";
				if (  strcmp ( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[418].Get()) ) == 0 )  t.chopthis_s = t.chopthis_s+"scriptbank\\";
				if (  strcmp ( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[433].Get()) ) == 0 )  t.chopthis_s = t.chopthis_s+"scriptbank\\";
				if (  strcmp ( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[464].Get()) ) == 0 )  t.chopthis_s = t.chopthis_s+"gamecore\\decals\\";
				LPSTR pPreferredFolder = "audiobank\\voices\\";
				if (  strcmp ( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[469].Get()) ) == 0 )  
				{
					// 151016 - determine if data points to voices folder
					if ( strnicmp ( t.tdata_s.Get() + strlen(t.chopthis_s.Get()), pPreferredFolder, strlen(pPreferredFolder) ) == NULL )
					{
						// default soundset entry into voices folder
						t.chopthis_s = t.chopthis_s + "audiobank\\voices\\";
					}
					else
					{
						// allow normal WAV sounds to be placed in character SoundSet slot (zombies)
						t.chopthis_s = t.chopthis_s;
						pPreferredFolder = NULL;
					}
				}
				t.tdata_s=Right(t.tdata_s.Get(),Len(t.tdata_s.Get())-Len(t.chopthis_s.Get()));
				if ( cstr(Lower(t.tfield_s.Get())) == cstr(Lower(t.strarr_s[464].Get())) 
				||	(cstr(Lower(t.tfield_s.Get())) == cstr(Lower(t.strarr_s[469].Get())) && pPreferredFolder != NULL) ) 
				{
					//  get path (folder name) only
					t.tdata_s=getpath(t.tdata_s.Get()) ; t.tdata_s=Left(t.tdata_s.Get(),Len(t.tdata_s.Get())-1);
				}
			}

			//  All YES and NO strings are auto converted if value expected
			t.tokay=1;
			if (  strcmp ( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[413].Get()) ) == 0 )  t.tokay = 0;
			if (  strcmp ( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[478].Get()) ) == 0 )  t.tokay = 0;
			if (  strcmp ( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[479].Get()) ) == 0 )  t.tokay = 0;
			if (  strcmp ( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[436].Get()) ) == 0 )  t.tokay = 0;
			if (  strcmp ( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[437].Get()) ) == 0 )  t.tokay = 0;
			if (  strcmp ( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[464].Get()) ) == 0 )  t.tokay = 0;
			if (  strcmp ( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[469].Get()) ) == 0 )  t.tokay = 0;
			if (  strcmp ( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[469].Get()) ) == 0 )  t.tokay = 0;
			if (  t.tokay == 1 ) 
			{
				if (  strcmp ( Lower(t.tdata_s.Get()) , Lower(t.strarr_s[470].Get()) ) == 0 )  t.tdata_s = "1";
				if (  strcmp ( Lower(t.tdata_s.Get()) , Lower(t.strarr_s[471].Get()) ) == 0 )  t.tdata_s = "0";
				if (  strcmp ( Lower(t.tdata_s.Get()) , Lower("no") ) == 0 )  t.tdata_s = "0";
				if (  strcmp ( Lower(t.tdata_s.Get()) , Lower("a") ) == 0 )  t.tdata_s = "1";
				if (  strcmp ( Lower(t.tdata_s.Get()) , Lower("b") ) == 0 )  t.tdata_s = "2";
			}

			//  FPGC - 070510 - add behaviour folder back, along with FPI (from combo friendly name to script filename)
			if (  cstr(Lower(t.tfield_s.Get())) == "behaviour" ) 
			{
				t.tdata_s = ""; t.tdata_s=t.tdata_s+"behaviours\\"+t.tdata_s+".fpi";
			}

			//  Clipped alternative
			t.tdataclipped_s=Left(t.tdata_s.Get(),63);

			//  get field data
			if (  strcmp( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[413].Get()) ) == 0 )  t.grideleprof.name_s = t.tdataclipped_s;
			if (  strcmp( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[478].Get()) ) == 0 )  t.grideleprof.name_s = t.tdataclipped_s;
			if (  strcmp( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[479].Get()) ) == 0 )  t.grideleprof.name_s = t.tdataclipped_s;
			if (  strcmp( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[414].Get()) ) == 0 )  t.gridentitystaticmode = ValF(t.tdata_s.Get());
			if (  strcmp( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[417].Get()) ) == 0 )  t.grideleprof.aimain_s = t.tdataclipped_s;
			if (  strcmp( Lower(t.tfield_s.Get()) , "behaviour" ) == 0 )  t.grideleprof.aimain_s = t.tdataclipped_s;
			if (  strcmp( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[434].Get()) ) == 0 )  t.grideleprof.coneangle = ValF(t.tdata_s.Get());
			if (  strcmp( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[476].Get()) ) == 0 )  t.grideleprof.conerange = ValF(t.tdata_s.Get());
			if (  strcmp( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[419].Get()) ) == 0 )  t.grideleprof.hasweapon_s = t.tdataclipped_s;
			if (  strcmp( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[436].Get()) ) == 0 )  t.grideleprof.usekey_s = t.tdataclipped_s;
			if (  strcmp( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[437].Get()) ) == 0 )  t.grideleprof.ifused_s = t.tdataclipped_s;
			if (  strcmp( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[562].Get()) ) == 0 )  t.grideleprof.spawnatstart = ValF(t.tdata_s.Get());
			if (  strcmp( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[440].Get()) ) == 0 )  t.grideleprof.spawnmax = ValF(t.tdata_s.Get());
			if (  strcmp( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[441].Get()) ) == 0 )  t.grideleprof.spawnupto = ValF(t.tdata_s.Get());
			if (  strcmp( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[442].Get()) ) == 0 )  t.grideleprof.spawnafterdelay = ValF(t.tdata_s.Get());
			if (  strcmp( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[443].Get()) ) == 0 )  t.grideleprof.spawnwhendead = ValF(t.tdata_s.Get());
			if (  strcmp( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[444].Get()) ) == 0 )  t.grideleprof.spawndelay = ValF(t.tdata_s.Get());
			if (  strcmp( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[445].Get()) ) == 0 )  t.grideleprof.spawnqty = ValF(t.tdata_s.Get());
			if (  strcmp( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[564].Get()) ) == 0 )  t.grideleprof.spawndelayrandom = ValF(t.tdata_s.Get());
			if (  strcmp( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[566].Get()) ) == 0 )  t.grideleprof.spawnqtyrandom = ValF(t.tdata_s.Get());
			if (  strcmp( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[568].Get()) ) == 0 )  t.grideleprof.spawnvel = ValF(t.tdata_s.Get());
			if (  strcmp( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[570].Get()) ) == 0 )  t.grideleprof.spawnvelrandom = ValF(t.tdata_s.Get());
			if (  strcmp( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[572].Get()) ) == 0 )  t.grideleprof.spawnangle = ValF(t.tdata_s.Get());
			if (  strcmp( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[574].Get()) ) == 0 )  t.grideleprof.spawnanglerandom = ValF(t.tdata_s.Get());
			if (  strcmp( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[576].Get()) ) == 0 )  t.grideleprof.spawnlife = ValF(t.tdata_s.Get());
			if (  strcmp( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[447].Get()) ) == 0 )  t.grideleprof.texd_s = t.tdataclipped_s;
			if (  strcmp( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[448].Get()) ) == 0 )  t.grideleprof.texaltd_s = t.tdataclipped_s;
			if (  strcmp( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[578].Get()) ) == 0 )  t.grideleprof.effect_s = t.tdataclipped_s;
			if (  strcmp( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[449].Get()) ) == 0 )  t.grideleprof.transparency = ValF(t.tdata_s.Get());
			if (  strcmp( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[450].Get()) ) == 0 )  t.grideleprof.reducetexture = ValF(t.tdata_s.Get());
			if (  strcmp( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[454].Get()) ) == 0 )  t.grideleprof.strength = ValF(t.tdata_s.Get());
			if (  strcmp( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[453].Get()) ) == 0 )  t.grideleprof.strength = ValF(t.tdata_s.Get());
			if (  strcmp( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[457].Get()) ) == 0 )  t.grideleprof.isimmobile = ValF(t.tdata_s.Get());
			if (  strcmp( Lower(t.tfield_s.Get()) , Lower("LOD Modifier") ) == 0 ) 
			{
				// 301115 - new LOD Modifie value for this entity parent, so propagate to ALL other entities of this parent
				t.grideleprof.lodmodifier = ValF(t.tdata_s.Get());
				int iThisBankIndex = t.gridentity;
				if ( t.entityprofile[iThisBankIndex].addhandlelimb==0 )
				{
					for ( int e=1; e<=g.entityelementlist; e++ )
					{
						if ( t.entityelement[e].bankindex==iThisBankIndex )
						{
							t.entityelement[e].eleprof.lodmodifier = t.grideleprof.lodmodifier;
							entity_calculateentityLODdistances ( iThisBankIndex, t.entityelement[e].obj, t.entityelement[e].eleprof.lodmodifier );
						}
					}
					int iParentSrcObj = g.entitybankoffset + iThisBankIndex;
					entity_calculateentityLODdistances ( iThisBankIndex, iParentSrcObj, t.grideleprof.lodmodifier );
				}
			}
			if (  strcmp( Lower(t.tfield_s.Get()) , Lower("Occluder") ) == 0 )  t.grideleprof.isocluder = ValF(t.tdata_s.Get());
			if (  strcmp( Lower(t.tfield_s.Get()) , Lower("Occludee") ) == 0 )  t.grideleprof.isocludee = ValF(t.tdata_s.Get());
			
			if (  strcmp( Lower(t.tfield_s.Get()) , Lower("End Collision") ) == 0 )  t.grideleprof.colondeath = ValF(t.tdata_s.Get());
			if (  strcmp( Lower(t.tfield_s.Get()) , Lower("Parent Index") ) == 0 )  t.grideleprof.parententityindex = ValF(t.tdata_s.Get());
			if (  strcmp( Lower(t.tfield_s.Get()) , Lower("Parent Limb") ) == 0 )  t.grideleprof.parentlimbindex = ValF(t.tdata_s.Get());

			if (  strcmp( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[429].Get()) ) == 0 )  t.grideleprof.cantakeweapon = ValF(t.tdata_s.Get());
			if (  strcmp( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[430].Get()) ) == 0 )  t.grideleprof.quantity = ValF(t.tdata_s.Get());
			if (  strcmp( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[431].Get()) ) == 0 )  t.grideleprof.rateoffire = ValF(t.tdata_s.Get());
			if (  strcmp( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[420].Get()) ) == 0 )  t.grideleprof.damage = ValF(t.tdata_s.Get());
			if (  strcmp( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[421].Get()) ) == 0 )  t.grideleprof.accuracy = ValF(t.tdata_s.Get());
			if (  strcmp( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[422].Get()) ) == 0 )  t.grideleprof.reloadqty = ValF(t.tdata_s.Get());
			if (  strcmp( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[423].Get()) ) == 0 )  t.grideleprof.fireiterations = ValF(t.tdata_s.Get());
			if (  strcmp( Lower(t.tfield_s.Get()) , Lower("Range") ) == 0 )  t.grideleprof.range = ValF(t.tdata_s.Get());
			if (  strcmp( Lower(t.tfield_s.Get()) , Lower("Dropoff") ) == 0 )  t.grideleprof.dropoff = ValF(t.tdata_s.Get());
			if (  strcmp( Lower(t.tfield_s.Get()) , Lower("Spot Lighting") ) == 0 )  t.grideleprof.usespotlighting = ValF(t.tdata_s.Get());
			if (strcmp(Lower(t.tfield_s.Get()), Lower("Clip Capacity")) == 0)  t.grideleprof.clipcapacity = ValF(t.tdata_s.Get());

			if (  strcmp( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[424].Get()) ) == 0 )  t.grideleprof.lifespan = ValF(t.tdata_s.Get());
			if (  strcmp( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[425].Get()) ) == 0 )  t.grideleprof.throwspeed = ValF(t.tdata_s.Get());
			if (  strcmp( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[426].Get()) ) == 0 )  t.grideleprof.throwangle = ValF(t.tdata_s.Get());
			if (  strcmp( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[427].Get()) ) == 0 )  t.grideleprof.bounceqty = ValF(t.tdata_s.Get());
			if (  strcmp( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[428].Get()) ) == 0 )  t.grideleprof.explodeonhit = ValF(t.tdata_s.Get());
			if (  strcmp( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[455].Get()) ) == 0 )  t.grideleprof.speed = ValF(t.tdata_s.Get());
			if (  strcmp ( Lower(t.tfield_s.Get()) , Lower("Anim Speed") ) == 0 ) 
			{
				t.grideleprof.animspeed=ValF(t.tdata_s.Get());
			}
			if (  strcmp ( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[432].Get()) ) == 0 )  t.grideleprof.quantity = ValF(t.tdata_s.Get());
			if (  strcmp ( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[460].Get()) ) == 0 )  t.grideleprof.quantity = ValF(t.tdata_s.Get());
			if (  strcmp ( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[452].Get()) ) == 0 )  t.grideleprof.lives = ValF(t.tdata_s.Get());
			if (  strcmp ( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[456].Get()) ) == 0 )  t.grideleprof.hurtfall = ValF(t.tdata_s.Get());
			if (  strcmp ( Lower(t.tfield_s.Get()) , Lower("Blood Effects")  ) == 0 ) t.grideleprof.isviolent = ValF(t.tdata_s.Get());
			if (  strcmp ( Lower(t.tfield_s.Get()) , Lower("Regeneration Rate")  ) == 0 ) t.playercontrol.regenrate = ValF(t.tdata_s.Get());
			if (  strcmp ( Lower(t.tfield_s.Get()) , Lower("Regeneration Speed")  ) == 0 ) t.playercontrol.regenspeed = ValF(t.tdata_s.Get());
			if (  strcmp ( Lower(t.tfield_s.Get()) , Lower("Regeneration Delay")  ) == 0 ) t.playercontrol.regendelay = ValF(t.tdata_s.Get());
			if (  strcmp ( Lower(t.tfield_s.Get()) , Lower("Jump Speed")  ) == 0 ) t.playercontrol.jumpmax_f = ValF(t.tdata_s.Get());
			if (  strcmp ( Lower(t.tfield_s.Get()) , Lower("Gravity")  ) == 0 ) t.playercontrol.gravity_f = ValF(t.tdata_s.Get());
			if (  strcmp ( Lower(t.tfield_s.Get()) , Lower("Fall Speed")  ) == 0 ) t.playercontrol.fallspeed_f = ValF(t.tdata_s.Get());
			if (  strcmp ( Lower(t.tfield_s.Get()) , Lower("Climb Angle")  ) == 0 ) t.playercontrol.climbangle_f = ValF(t.tdata_s.Get());
			if (  strcmp ( Lower(t.tfield_s.Get()) , Lower("Wobble Speed")  ) == 0 ) t.playercontrol.wobblespeed_f = ValF(t.tdata_s.Get());
			if (  strcmp ( Lower(t.tfield_s.Get()) , Lower("Wobble Height")  ) == 0 ) t.playercontrol.wobbleheight_f = ValF(t.tdata_s.Get())/100.0;
			if (  strcmp ( Lower(t.tfield_s.Get()) , Lower("Footfall Pace")  ) == 0 ) t.playercontrol.footfallpace_f = ValF(t.tdata_s.Get())/100.0;
			if (  strcmp ( Lower(t.tfield_s.Get()) , Lower("Acceleration")  ) == 0 ) t.playercontrol.accel_f = ValF(t.tdata_s.Get())/100.0;
			if (  strcmp ( Lower(t.tfield_s.Get()) , Lower("Camera Locked")  ) == 0 ) t.playercontrol.thirdperson.cameralocked = ValF(t.tdata_s.Get());
			if (  strcmp ( Lower(t.tfield_s.Get()) , Lower("Camera Distance")  ) == 0 ) t.playercontrol.thirdperson.cameradistance = ValF(t.tdata_s.Get());
			if (  strcmp ( Lower(t.tfield_s.Get()) , Lower("Camera Y Offset")  ) == 0 ) t.playercontrol.thirdperson.cameraheight = ValF(t.tdata_s.Get());
			if (  strcmp ( Lower(t.tfield_s.Get()) , Lower("Camera Focus")  ) == 0 ) t.playercontrol.thirdperson.camerafocus = ValF(t.tdata_s.Get());
			if (  strcmp ( Lower(t.tfield_s.Get()) , Lower("Camera Speed")  ) == 0 ) t.playercontrol.thirdperson.cameraspeed = ValF(t.tdata_s.Get());
			if (  strcmp ( Lower(t.tfield_s.Get()) , Lower("Camera X Offset")  ) == 0 ) t.playercontrol.thirdperson.camerashoulder = ValF(t.tdata_s.Get());
			if (  strcmp ( Lower(t.tfield_s.Get()) , Lower("Run Mode")  ) == 0 ) t.playercontrol.thirdperson.camerafollow = ValF(t.tdata_s.Get());
			if (  strcmp ( Lower(t.tfield_s.Get()) , Lower("Show Reticle")  ) == 0 ) t.playercontrol.thirdperson.camerareticle = ValF(t.tdata_s.Get());
			if (  strcmp ( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[458].Get()) ) == 0 )  t.grideleprof.isobjective = ValF(t.tdata_s.Get());

			//  FPGC - 300710 - read data changes back into grideleprof
			if (  strcmp ( Lower(t.tfield_s.Get()) , "custom settings"  ) == 0 ) t.grideleprof.particleoverride = ValF(t.tdata_s.Get());
			if (  strcmp ( Lower(t.tfield_s.Get()) , "offsety"  ) == 0 )  t.grideleprof.particle.offsety = ValF(t.tdata_s.Get());
			if (  strcmp ( Lower(t.tfield_s.Get()) , "scale"  ) == 0 )  t.grideleprof.particle.scale = ValF(t.tdata_s.Get());
			if (  strcmp ( Lower(t.tfield_s.Get()) , "random start x"  ) == 0 )  t.grideleprof.particle.randomstartx = ValF(t.tdata_s.Get());
			if (  strcmp ( Lower(t.tfield_s.Get()) , "random start y"  ) == 0 )  t.grideleprof.particle.randomstarty = ValF(t.tdata_s.Get());
			if (  strcmp ( Lower(t.tfield_s.Get()) , "random start z"  ) == 0 )  t.grideleprof.particle.randomstartz = ValF(t.tdata_s.Get());
			if (  strcmp ( Lower(t.tfield_s.Get()) , "linear motion x"  ) == 0 )  t.grideleprof.particle.linearmotionx = ValF(t.tdata_s.Get());
			if (  strcmp ( Lower(t.tfield_s.Get()) , "linear motion y"  ) == 0 )  t.grideleprof.particle.linearmotiony = ValF(t.tdata_s.Get());
			if (  strcmp ( Lower(t.tfield_s.Get()) , "linear motion z"  ) == 0 )  t.grideleprof.particle.linearmotionz = ValF(t.tdata_s.Get());
			if (  strcmp ( Lower(t.tfield_s.Get()) , "random motion x"  ) == 0 )  t.grideleprof.particle.randommotionx = ValF(t.tdata_s.Get());
			if (  strcmp ( Lower(t.tfield_s.Get()) , "random motion y"  ) == 0 )  t.grideleprof.particle.randommotiony = ValF(t.tdata_s.Get());
			if (  strcmp ( Lower(t.tfield_s.Get()) , "random motion z"  ) == 0 )  t.grideleprof.particle.randommotionz = ValF(t.tdata_s.Get());
			if (  strcmp ( Lower(t.tfield_s.Get()) , "mirror mode"  ) == 0 )  t.grideleprof.particle.mirrormode = ValF(t.tdata_s.Get());
			if (  strcmp ( Lower(t.tfield_s.Get()) , "camera z shift"  ) == 0 )  t.grideleprof.particle.camerazshift = ValF(t.tdata_s.Get());
			if (  strcmp ( Lower(t.tfield_s.Get()) , "scale only x"  ) == 0 )  t.grideleprof.particle.scaleonlyx = ValF(t.tdata_s.Get());
			if (  strcmp ( Lower(t.tfield_s.Get()) , "life increment"  ) == 0 )  t.grideleprof.particle.lifeincrement = ValF(t.tdata_s.Get());
			if (  strcmp ( Lower(t.tfield_s.Get()) , "alpha intensity"  ) == 0 )  t.grideleprof.particle.alphaintensity = ValF(t.tdata_s.Get());
			if (  strcmp ( Lower(t.tfield_s.Get()) , "animated texture"  ) == 0 )  t.grideleprof.particle.animated , ValF(t.tdata_s.Get()) ;

			if (  strcmp ( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[467].Get()) ) == 0 )  t.grideleprof.soundset_s = t.tdataclipped_s;
			if (  strcmp ( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[468].Get()) ) == 0 )  t.grideleprof.soundset1_s = t.tdataclipped_s;
			if (  strcmp ( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[480].Get()) ) == 0 )  t.grideleprof.soundset2_s = t.tdataclipped_s;
			if (  strcmp ( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[481].Get()) ) == 0 )  t.grideleprof.soundset3_s = t.tdataclipped_s;
			if (  strcmp ( Lower(t.tfield_s.Get()) , Lower("Sound5")) == 0)					t.grideleprof.soundset5_s = t.tdataclipped_s;
			if (  strcmp ( Lower(t.tfield_s.Get()) , Lower("Sound6")) == 0)					t.grideleprof.soundset6_s = t.tdataclipped_s;
			if (  strcmp ( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[469].Get()) ) == 0 )  t.grideleprof.soundset_s = t.tdataclipped_s;
			if (  strcmp ( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[598].Get()) ) == 0 )  t.grideleprof.soundset_s = t.tdataclipped_s;
			if (  strcmp ( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[600].Get()) ) == 0 )  t.grideleprof.soundset1_s = t.tdataclipped_s;
			if (  strcmp ( Lower(t.tfield_s.Get()) , "voiceover"  ) == 0 ) t.grideleprof.soundset1_s = t.tdataclipped_s;
			if (  strcmp ( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[462].Get()) ) == 0 )  t.grideleprof.light.range = ValF(t.tdata_s.Get());
			if (  strcmp ( Lower(t.tfield_s.Get()) , Lower("Text to Appear") ) == 0 )  t.grideleprof.soundset_s = t.tdataclipped_s;
			if (  strcmp ( Lower(t.tfield_s.Get()) , Lower("Image File") ) == 0 )  t.grideleprof.soundset_s = t.tdataclipped_s;
			if (  strcmp ( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[469].Get()) ) == 0 )  t.grideleprof.soundset_s = t.tdataclipped_s;
			if (  strcmp ( Lower(t.tfield_s.Get()) , Lower("Video Slot") ) == 0 )  t.grideleprof.soundset1_s = t.tdataclipped_s;

			if (  strcmp ( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[580].Get()) ) == 0 )  t.grideleprof.physics = ValF(t.tdata_s.Get());
			if (  t.grideleprof.physics != 1  )  t.grideleprof.physics = 2;
			if (  strcmp ( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[582].Get()) ) == 0 )  t.grideleprof.phyalways = ValF(t.tdata_s.Get());
			if (  strcmp ( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[584].Get()) ) == 0 )  t.grideleprof.phyweight = ValF(t.tdata_s.Get());
			if (  strcmp ( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[586].Get()) ) == 0 )  t.grideleprof.phyfriction = ValF(t.tdata_s.Get());
			if (  strcmp ( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[588].Get()) ) == 0 )  t.grideleprof.phyforcedamage = ValF(t.tdata_s.Get());
			if (  strcmp ( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[590].Get()) ) == 0 )  t.grideleprof.rotatethrow = ValF(t.tdata_s.Get());
			if (  strcmp ( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[592].Get()) ) == 0 )  t.grideleprof.explodable = ValF(t.tdata_s.Get());
			if (  strcmp ( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[594].Get()) ) == 0 )  t.grideleprof.explodedamage = ValF(t.tdata_s.Get());

			if (  strcmp ( Lower(t.tfield_s.Get()) , Lower(t.strarr_s[463].Get()) ) == 0 ) 
			{
				t.tr_s=t.tdata_s;
				for ( t.t = 1 ; t.t<=  Len(t.tr_s.Get()); t.t++ )
				{
					if (  t.tr_s.Get()[t.t-1] == ' ' ) { t.tr_s = Left(t.tr_s.Get(),t.t) ; break; }
				}
				t.tdata_s=Right(t.tdata_s.Get(),(Len(t.tdata_s.Get())-Len(t.tr_s.Get())));
				t.tg_s=t.tdata_s;
				for ( t.t = 1 ; t.t<=  Len(t.tg_s.Get()); t.t++ )
				{
					if (  t.tg_s.Get()[t.t-1] == ' ' ) { t.tg_s = Left(t.tg_s.Get(),t.t)  ; break; }
				}
				t.tb_s=Right(t.tdata_s.Get(),(Len(t.tdata_s.Get())-Len(t.tg_s.Get())));
				t.grideleprof.light.color=Rgb(ValF(t.tr_s.Get()),ValF(t.tg_s.Get()),ValF(t.tb_s.Get()));
			}

		}
	}
}

void interface_closepropertywindow ( void )
{
	//  Close proprty window
	if (  t.editorinterfaceactive>0 )
	{
		//  Close dialog
		OpenFileMap (  1, "FPSEXCHANGE" );
		SetFileMapDWORD (  1, 978, 2 );
		SetFileMapDWORD (  1, 462, 0 );
		SetEventAndWait (  1 );
		t.editorinterfaceactive=0;
	}
}

void interface_handlepropertywindow ( void )
{
	//  If interface active
	if (  t.editorinterfaceactive>0 ) 
	{
		//  Open for management
		OpenFileMap (  2, "FPSENTITY" );
		SetEventAndWait (  2 );

		//  if APPLY clicked, copy data to entity
		if (  GetFileMapDWORD( 2, 112 ) == 1 ) 
		{
			interface_copydatatoentity ( );
			SetFileMapDWORD (  2, 112, 0 );
			SetEventAndWait (  2 );
			t.editorinterfaceleave=1;
			t.interactive.applychangesused=1;
		}

		//  see if the user clicked on the close button
		if (  GetFileMapDWORD( 2, 108 )  ==  1 ) 
		{
			SetFileMapDWORD (  2, 108, 0 );
			SetEventAndWait (  2 );
			t.editorinterfaceleave=1;
		}

		//  see if the user clicked on the CANCEL button
		if (  GetFileMapDWORD( 2, 116 )  ==  1 ) 
		{
			SetFileMapDWORD (  2, 116, 0 );
			SetEventAndWait (  2 );
			t.editorinterfaceleave=1;
		}
	}
}

void interface_live_updates(void)
{
	//  constantly open access to properties values
	//  so can represent the values prior to using APPLY CHANGES
	if (MAXTimer() > t.lastliveupdatestimer)
	{
		t.lastliveupdatestimer = MAXTimer() + 200;
		OpenFileMap(2, "FPSENTITY");
		SetEventAndWait(2);
		t.iGroup = t.livegroupforthirdperson;
		t.iControl = 1; t.tfield_s = getpropertyfield(t.iGroup, t.iControl); t.tdata_s = getpropertydata(t.iGroup, t.iControl);
		if (cstr(Lower(t.tfield_s.Get())) == Lower("Camera Distance"))  t.playercontrol.thirdperson.livecameradistance = ValF(t.tdata_s.Get());
		t.iControl = 2; t.tfield_s = getpropertyfield(t.iGroup, t.iControl); t.tdata_s = getpropertydata(t.iGroup, t.iControl);
		if (cstr(Lower(t.tfield_s.Get())) == Lower("Camera X Offset"))  t.playercontrol.thirdperson.livecamerashoulder = ValF(t.tdata_s.Get());
		t.iControl = 3; t.tfield_s = getpropertyfield(t.iGroup, t.iControl); t.tdata_s = getpropertydata(t.iGroup, t.iControl);
		if (cstr(Lower(t.tfield_s.Get())) == Lower("Camera Y Offset"))  t.playercontrol.thirdperson.livecameraheight = ValF(t.tdata_s.Get());
		t.iControl = 4; t.tfield_s = getpropertyfield(t.iGroup, t.iControl); t.tdata_s = getpropertydata(t.iGroup, t.iControl);
		if (cstr(Lower(t.tfield_s.Get())) == Lower("Camera Focus"))  t.playercontrol.thirdperson.livecamerafocus = ValF(t.tdata_s.Get());
	}
}

// 
//  Interface Properties Functions
// 

char* imgui_setpropertyfile2_ex_dlua(int group, char* data_s, char* field_s, char* desc_s, char* within_s, int* piEditedField, char* pButtonControlIfBlocked)
{
	char *cRet;
	cstr ldata_s = data_s, ldesc_s = desc_s, lfields_s = field_s, lwithin_s = within_s;

	if (cstr(data_s) == "" || !data_s)  ldata_s = "";
	if (cstr(desc_s) == "" || !desc_s)  ldesc_s = "";
	if (cstr(field_s) == "" || !field_s)  lfields_s = "";
	if (cstr(within_s) == "" || !within_s)  lwithin_s = "";

	std::string uniquiField = "";
	uniquiField = uniquiField + "##" + lfields_s.Get();
	uniquiField = uniquiField + std::to_string(grideleprof_uniqui_id++);

	if (lfields_s != "") {
		ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));
		ImGui::Text(lfields_s.Get());
		ImGui::SameLine();
		ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() - 3));
	}
	strcpy(cTmpInput, ldata_s.Get());

	ImGui::PushItemWidth(-ImGui::CalcTextSize(lfields_s.Get()).x);

	// Display the filename only.
	char filename[MAX_PATH];
	int iCopyLocation = 0;
	for (int i = strlen(cTmpInput) - 1; i >= 0; i--)
	{
		if (cTmpInput[i] == '/' || cTmpInput[i] == '\\')
		{
			iCopyLocation = i+1;
			break;
		}
	}
	strcpy(filename, cTmpInput + iCopyLocation);
	ImGui::InputText(uniquiField.c_str(), &filename[0], MAXTEXTINPUT, ImGuiInputTextFlags_ReadOnly);

	if (!pref.iTurnOffEditboxTooltip && ImGui::IsItemHovered() && ldesc_s != "") ImGui::SetTooltip("%s", ldesc_s.Get());
	if (ImGui::MaxIsItemFocused()) bImGuiGotFocus = true;

	ImGui::PopItemWidth();
	ImGui::SameLine();

	uniquiField = "...";
	uniquiField = uniquiField + "##";
	uniquiField = uniquiField + std::to_string(grideleprof_uniqui_id++);

	ImGui::PushItemWidth(ImGui::GetFontSize()*2.0);

	bool bAudio = false;
	bool bImage = false;
	bool bVideo = false;
	bool bScript = false;
	bool bParticle = false;
	bool bUseNewAudioWindow = false;
#ifdef USENEWMEDIASELECTWINDOWS
	if (pestrcasestr(lwithin_s.Get(), "audiobank"))
	{
		bUseNewAudioWindow = true;
		bAudio = true;
	}
	if (pestrcasestr(lwithin_s.Get(), "\\imagesinzone"))
	{
		bUseNewAudioWindow = true;
		bImage = true;
	}
	if (pestrcasestr(lwithin_s.Get(), "imagebank"))
	{
		bUseNewAudioWindow = true;
		bImage = true;
	}
	if (pestrcasestr(lwithin_s.Get(), "videobank"))
	{
		bUseNewAudioWindow = true;
		bVideo = true;
	}
	if (pestrcasestr(lwithin_s.Get(), "scriptbank"))
	{
		bUseNewAudioWindow = true;
		bScript = true;
	}
	if (pestrcasestr(lwithin_s.Get(), "particlesbank"))
	{
		bUseNewAudioWindow = true;
		bParticle = true;
	}

#endif

	if (bUseNewAudioWindow )
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();

		bool bProceed = true;
		if (piEditedField)
		{
			if (*piEditedField == 2)
			{
				bProceed = false;
			}
		}
		if (!bProceed)
		{
			ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
		}
		if (ImGui::StyleButton(uniquiField.c_str(), ImVec2(ImGui::GetFontSize()*1.48, 0)) || iSelectedLibraryStingReturnID == window->GetID(uniquiField.c_str()))
		{
			if (bProceed == true)
			{
				cStr tOldDir = GetDir();
				if (iSelectedLibraryStingReturnID == window->GetID(uniquiField.c_str()))
				{
					char * cFileSelected = sSelectedLibrarySting.Get();

					SetDir(tOldDir.Get());

					if (cFileSelected && strlen(cFileSelected) > 0)
					{
						if (piEditedField) *piEditedField = 1;
						std::string relative = cFileSelected;
						std::string fullpath = tOldDir.Get();
						fullpath += "\\";
						if (pestrcasestr(lwithin_s.Get(), "scriptbank"))
							fullpath += lwithin_s.Get();
						replaceAll(relative, fullpath, "");
						strcpy(cTmpInput, relative.c_str());
					}

					iSelectedLibraryStingReturnID = -1; //disable.
					sSelectedLibrarySting = "";
				}
				else
				{
					bExternal_Entities_Window = true;
					iDisplayLibraryType = 0;
					iDisplayLibrarySubType = 0;
					if (bAudio)
						iDisplayLibraryType = 1;
					if (bImage)
						iDisplayLibraryType = 2;
					if (bVideo)
						iDisplayLibraryType = 3;
					if (bScript)
						iDisplayLibraryType = 4;
					if (bParticle)
						iDisplayLibraryType = 5;

					iLibraryStingReturnToID = window->GetID(uniquiField.c_str());
					if (iDisplayLibraryType > 0)
					{
						if (strlen(cTmpInput) > 0)
							sMakeDefaultSelecting = cTmpInput;
					}

				}
			}
		}

		if (!bProceed)
		{
			ImGui::PopItemFlag();
			ImGui::PopStyleVar();
		}

	}
	else
	{
		if (ImGui::StyleButton(uniquiField.c_str(), ImVec2(ImGui::GetFontSize()*1.48, 0)))
		{
			bool bProceed = true;
			if (piEditedField)
			{
				if (*piEditedField == 2)
				{
					MessageBoxA(NULL, pButtonControlIfBlocked, "Notification", MB_OK);
					bProceed = false;
				}
			}
			if (bProceed == true)
			{
				cStr tOldDir = GetDir();
				char * cFileSelected;
				cFileSelected = (char *)noc_file_dialog_open(NOC_FILE_DIALOG_OPEN, "All\0*.*\0", lwithin_s.Get(), NULL);
				SetDir(tOldDir.Get());
				if (cFileSelected && strlen(cFileSelected) > 0)
				{
					if (piEditedField) *piEditedField = 1;
					std::string relative = cFileSelected;
					std::string fullpath = tOldDir.Get();
					fullpath += "\\";
					if (pestrcasestr(lwithin_s.Get(), "scriptbank"))
						fullpath += lwithin_s.Get();
					replaceAll(relative, fullpath, "");
					strcpy(cTmpInput, relative.c_str());
				}
			}
		}
	}
	ImGui::PopItemWidth();

	return &cTmpInput[0];

}

char* imgui_setpropertyfile2_dlua(int group, char* data_s, char* field_s, char* desc_s, char* within_s)
{
	return imgui_setpropertyfile2_ex_dlua(group, data_s, field_s, desc_s, within_s, NULL, NULL);
}

char * imgui_setpropertyfile2(int group, char* data_s, char* field_s, char* desc_s, char* within_s)
{
	char *cRet;
	cstr ldata_s = data_s, ldesc_s = desc_s, lfields_s = field_s, lwithin_s = within_s;

	if (cstr(data_s) == "" || !data_s)  ldata_s = "";
	if (cstr(desc_s) == "" || !desc_s)  ldesc_s = "";
	if (cstr(field_s) == "" || !field_s)  lfields_s = "";
	if (cstr(within_s) == "" || !within_s)  lwithin_s = "";

	std::string uniquiField = "";
	uniquiField = uniquiField + "##" + lfields_s.Get();
	uniquiField = uniquiField + std::to_string(grideleprof_uniqui_id++);

	if (lfields_s != "") {
		ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));
		ImGui::Text(lfields_s.Get());
		ImGui::SameLine();
		ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() - 3));
		ImGui::SetCursorPos(ImVec2(fPropertiesColoumWidth, ImGui::GetCursorPosY()));
	}
	strcpy(cTmpInput, ldata_s.Get());

	bool bSoundSet = false;
	if (pestrcasestr(lfields_s.Get(), "soundset") || lfields_s == "Type") {
		bSoundSet = true;
	}
	if (bSoundSet && t.entityprofile[t.gridentity].ischaracter > 0) {

		ImGui::PushItemWidth(-10);

		//Only displayt Male,FeMale selection.
		const char* items[] = { "Male", "Female" };
		int item_current_type_selection = 0; //Default Custom.
		if (pestrcasestr(cTmpInput, "Female")) {
			item_current_type_selection = 1;
		}
		if (ImGui::Combo(uniquiField.c_str(), &item_current_type_selection, items, IM_ARRAYSIZE(items))) {
			strcpy(cTmpInput, items[item_current_type_selection]);
		}
		if (ImGui::IsItemHovered() && ldesc_s != "") ImGui::SetTooltip("%s", ldesc_s.Get());

		ImGui::PopItemWidth();
		return &cTmpInput[0];

	}

	ImGui::PushItemWidth( -10 - (ImGui::GetFontSize()*2.0) ); //-6 padding.

	ImGui::InputText(uniquiField.c_str(), &cTmpInput[0], MAXTEXTINPUT);
	if (ImGui::IsItemHovered() && ldesc_s != "") ImGui::SetTooltip("%s", ldesc_s.Get());
	if (ImGui::MaxIsItemFocused()) bImGuiGotFocus = true;

	ImGui::PopItemWidth();
	ImGui::SameLine();

	uniquiField = "...";
	uniquiField = uniquiField + "##";
	uniquiField = uniquiField + std::to_string(grideleprof_uniqui_id++);

	ImGui::PushItemWidth(ImGui::GetFontSize()*2.0);

	if (ImGui::StyleButton(uniquiField.c_str(), ImVec2(ImGui::GetFontSize()*1.48, 0))) { //ImVec2(ImGui::GetFontSize()*2.0,0)
		//PE: filedialogs change dir so.
		cStr tOldDir = GetDir();
		char * cFileSelected;
		cFileSelected = (char *)noc_file_dialog_open(NOC_FILE_DIALOG_OPEN, "All\0*.*\0", lwithin_s.Get(), NULL);

		SetDir(tOldDir.Get());

		if (cFileSelected && strlen(cFileSelected) > 0) {
			std::string relative = cFileSelected;
			std::string fullpath = tOldDir.Get();
			fullpath += "\\";

			// scriptbank\  //
			if (bSoundSet || pestrcasestr(lwithin_s.Get(), "scriptbank")) {
				if( pestrcasestr(cFileSelected,".lua"))
					fullpath += "scriptbank\\"; //lwithin_s.Get(); PE: This can change in parent mode 2
			}

			replaceAll(relative, fullpath , "");
			strcpy(cTmpInput, relative.c_str() );

			if (bSoundSet) {
				char *found = (char *) pestrcasestr(cTmpInput, "\\");
				if (found)
					found[0] = 0;
				//Remove everything after \\

			}
		}
		//File Selector.
	}

	ImGui::PopItemWidth();

	return &cTmpInput[0];

}

char * imgui_setpropertystring2(int group, char* data_s, char* field_s, char* desc_s)
{
	char *cRet;
	cstr ldata_s = data_s, ldesc_s = desc_s , lfields_s = field_s;

	if (cstr(data_s) == "" || !data_s)  ldata_s = "";
	if (cstr(desc_s) == "" || !desc_s)  ldesc_s = "";
	if (cstr(field_s) == "" || !field_s)  lfields_s = "";

	std::string uniquiField = ""; //lfields_s.Get();
	uniquiField = uniquiField + "##" + lfields_s.Get();
	uniquiField  = uniquiField+ std::to_string(grideleprof_uniqui_id++);

	if (lfields_s != "") {
		ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));
		ImGui::Text(lfields_s.Get());
		ImGui::SameLine();
		ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() - 3));
		ImGui::SetCursorPos(ImVec2(fPropertiesColoumWidth, ImGui::GetCursorPosY()));
	}
	ImGui::PushItemWidth(-10);

	int inputFlags = 0;

	strcpy(cTmpInput, ldata_s.Get());
	if (ImGui::InputText(uniquiField.c_str(), &cTmpInput[0], MAXTEXTINPUT, inputFlags)) {
		bImGuiGotFocus = true;
	}
	if (ImGui::IsItemHovered() && ldesc_s != "" ) ImGui::SetTooltip("%s", ldesc_s.Get());
	if (ImGui::MaxIsItemFocused()) bImGuiGotFocus = true;

	ImGui::PopItemWidth();

	return &cTmpInput[0];
}

