//----------------------------------------------------
//--- GAMEGURU - Common
//----------------------------------------------------

// Includes 
#include "stdafx.h"
#include "gameguru.h"
#include <stdio.h>
#include <stdlib.h>
#include "shellapi.h"
#include "time.h"
#include "direct.h"
#include <wininet.h>
#include "M-WelcomeSystem.h"
//#include "..\..\Dark Basic Public Shared\Dark Basic Pro SDK\Shared\Objects\ShadowMapping\cShadowMaps.h" DX12
#include "..\..\Dark Basic Public Shared\Include\CObjectsC.h"

//PE: GameGuru IMGUI.
#include "..\Imgui\imgui.h"
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "..\Imgui\imgui_internal.h"
#include "..\Imgui\imgui_impl_win32.h"
#include "..\Imgui\imgui_gg_dx11.h"

//220mb saved.
#define REDUCEMEMUSE

// Used for Free Weekend Promotion Build 
//#define STEAMOWNERSHIPCHECKFREEWEEKEND

// core externs to globals
extern LPSTR gRefCommandLineString;
extern bool gbAlwaysIgnoreShaderBlobFile;

extern bool g_bDisableVRDetectionByUserRequest;
extern bool bStartNewPrompt;

// Globals
int g_PopupControlMode = 0;
int g_trialStampDaysLeft = 0;
char g_trialDiscountCode[1024];
char g_trialDiscountExpires[1024];
int tgamesetismapeditormode = 1; //PE: Need access to this.
// Externs
extern void ImGui_RenderLast(void);

// to enable the use of _e_ in standalone
void SetCanUse_e_ ( int flag );
char g_pCloudKeyErrorString[10240];
char g_pCloudKeyExpiresDate[11];
bool g_bCloudKeyIsHomeEdition = false;

// global to store abs path to converter
char g_pAbsPathToConverter[MAX_PATH];

// flag to control whether mouse pointer clipped and locked to MAX window!
bool g_bClipInForce = false;

// C++ CONVERSION: g contains all variables that were defined as global in dbpro source
Sglobals g;

// C++ CONVERSION: t contains all variables that were considered temporary and subject to change between routines
Stemps t;

void SetCanUse_e_ ( int flag );
void SetWorkshopFolder ( LPSTR pFolder );

//Subroutines
void CheckForNewUpdateWicked(void)
{
	extern bool bAreWeAEditor;
	if (!bAreWeAEditor) return;

	int iUpdateCheckRetValue;
	if ((iUpdateCheckRetValue = CheckExecuteFileDone()) != -1)
	{
		if (iUpdateCheckRetValue == 3 || iUpdateCheckRetValue == 4)
		{
			//Ask.
			if (MessageBoxA(NULL, "A new update is available would you like to install it now. ?", "Update.", MB_YESNO | MB_TOPMOST) == IDYES)
			{
				ExecuteFile("..\\..\\GameGuru MAX Updater.exe", "", "", 0);
				extern bool g_bCascadeQuitFlag;
				g_bCascadeQuitFlag = true;
				PostQuitMessage(0);
				Sleep(1000);
				ExitProcess(0);
			}
		}
	}
}

void common_init ( void )
{
	/*
	// if a user needs to decrypt their media from an old GG Classic Game, use this code 
	#define EXTRACTENCRYPTEDMEDIA
	#ifdef EXTRACTENCRYPTEDMEDIA
	// store current folder to restore later
	cStr pOldDir = GetDir();
	// Create DBPDATA folder for this process
	GetTempPathA(_MAX_PATH, g_WindowsTempDirectory);
	_chdir(g_WindowsTempDirectory);
	_mkdir("dbpdata");
	// path to location of all files we want to decrypt
	LPSTR pPathToClassicEncryptedFiles = "D:\\DEV\\DOWNLOADS\\game-to-change";
	// set to work path and add everything from Files to a list
	SetDir(pPathToClassicEncryptedFiles);
	addallinfoldertocollection("Files","");
	// go through all files in list and decrypt the ones marked _e_
	SetDir("Files");
	cStr pFilesRootDir = GetDir();
	SetCanUse_e_(1);
	for ( int f = 1; f <= g.filecollectionmax; f++ )
	{
		LPSTR pThisFile = t.filecollection_s[f].Get();
		if (strstr(pThisFile, "\\_e_") != NULL)
		{
			// found an encrypted file
			char pVirtualFilename[MAX_PATH];
			strcpy(pVirtualFilename, pThisFile);

			// decrypt file
			g_pGlob->Decrypt( pVirtualFilename );

			// if temp created successfully
			if (FileExist(pVirtualFilename) == 1)
			{
				// create new filename for the unencrypted version (dasdasdsa\\dsadsad\\_e_dsadas.xxx)
				char pActualFile[MAX_PATH];
				strcpy(pActualFile, "");
				char pNewDecryptedFilename[MAX_PATH];
				strcpy(pNewDecryptedFilename, pThisFile);
				for (int n = strlen(pNewDecryptedFilename)-1; n>0; n--)
				{
					if (pNewDecryptedFilename[n] == '\\' || pNewDecryptedFilename[n] == '/')
					{
						strcpy(pActualFile, pNewDecryptedFilename + n + 4);
						pNewDecryptedFilename[n+1] = 0;
						break;
					}
				}
				if (strlen(pActualFile) > 0)
				{
					// copy new decrypted file
					char pAbsDestFile[MAX_PATH];
					strcpy(pAbsDestFile, pFilesRootDir.Get());
					strcat(pAbsDestFile, pNewDecryptedFilename);
					strcat(pAbsDestFile, pActualFile);

					//strcat(pNewDecryptedFilename, pActualFile);
					//MessageBoxA(NULL, pNewDecryptedFilename, pNewDecryptedFilename, MB_OK);
					//CopyFileA(pVirtualFilename, pNewDecryptedFilename, FALSE);
					CopyFileA(pVirtualFilename, pAbsDestFile, FALSE);

					// if copy was successful, delete old encrypted file
					//if (FileExist(pNewDecryptedFilename) == 1)
					char pAbsSrcFileToDelete[MAX_PATH];
					strcpy(pAbsSrcFileToDelete, pFilesRootDir.Get());
					strcat(pAbsSrcFileToDelete, pThisFile);
					if (FileExist(pAbsDestFile) == 1)
					{
						//DeleteFileA(pThisFile);
						DeleteFileA(pAbsSrcFileToDelete);
					}
				}
			}
		}
	}
	SetCanUse_e_(0);
	// reset the above list 
	g.filecollectionmax = 0;
	Dim ( t.filecollection_s,500 );
	// restore folder and continue
	SetDir(pOldDir.Get());
	#endif
	MessageBoxA(NULL, pPathToClassicEncryptedFiles, "Decrypt Complete!", MB_OK);
	*/

	/*
	// copy contents of playermedia to 'expansionobb'
	bool bConvertPlayerMediaToOBBRoot = false;
	if (bConvertPlayerMediaToOBBRoot == true)
	{
		// store current folder to restore later
		cStr pOldDir = GetDir();
		// create OBB folder
		char pOBBFolder[MAX_PATH];
		strcpy(pOBBFolder, pOldDir.Get());
		strcat(pOBBFolder, "\\Files\\expansionobb\\");
		// find all files in playermedia
		SetDir("Files");
		addallinfoldertocollection("playermedia", "");
		SetDir("playermedia");
		for (int f = 1; f <= g.filecollectionmax; f++)
		{
			LPSTR pThisFile = t.filecollection_s[f].Get();
			if (strlen(pThisFile)>0)
			{
				// convert full relative path to underscored name
				char pUnderscored[MAX_PATH];
				strcpy(pUnderscored, pThisFile);
				for (int n = 0; n < strlen(pUnderscored); n++)
				{
					if (pUnderscored[n] == '\\' || pUnderscored[n] == '/')
						pUnderscored[n] = '_';
				}

				// destination
				char pDestFile[MAX_PATH];
				strcpy(pDestFile, pOBBFolder);
				strcat(pDestFile, pUnderscored);

				//MessageBoxA(NULL, pThisFile, pUnderscored, MB_OK);
				if (FileExist(pDestFile) == 1) DeleteFileA(pDestFile);
				CopyFileA(pThisFile, pDestFile, FALSE);
			}
		}
		// reset the above list 
		g.filecollectionmax = 0;
		Dim (t.filecollection_s, 500);
		// restore folder and continue
		SetDir(pOldDir.Get());
		// and we are done
		PostQuitMessage(0);
		return;
	}
	*/

	// work out and store absolute path to converter in root folder (kept as other code depends on the abs root path) (though Guru-Converter.exe now never called by MAX)
	GetCurrentDirectoryA(MAX_PATH, g_pAbsPathToConverter);
	strcat(g_pAbsPathToConverter, "\\Guru-Converter.exe");

	// determines if EDITOR or GAME right away!
	FPSC_VeryEarlySetup();
	if (stricmp(g.trueappname_s.Get(), "Guru-MapEditor") == NULL)
	{
		// MAP EDITOR
		// Before launch MAX, run the Classic to MAX converter to see if any assets require porting from X files to DBO
		// continues in background, does not wait until finished before launching MAX (small chance of trying to use an old X file first time!)
		// better down the load if this popped up as an IMGUI prompt showing this process happening on startup
		// with an option in the dialog and in the editor settings to switch off this activity by default
		if (1)
		{
			// FOR DEVELOPMENT - Ensures core files all use DBO, no X files!
			// LB: From now, I will manually run the converter if I can introducing X files to the BUILD
			// HINSTANCE hinstance = ShellExecuteA(NULL, "open", g_pAbsPathToConverter, "", "", SW_SHOWDEFAULT);
		}

		// FOR RELEASE - Scans writable folder and convert any found there!
		char pOldDir[MAX_PATH];
		GetCurrentDirectoryA(MAX_PATH, pOldDir);
		char pRealWritableArea[MAX_PATH];
		strcpy(pRealWritableArea, pOldDir);
		strcat(pRealWritableArea, "\\");
		GG_GetRealPath(pRealWritableArea, 1);
		SetDir(pRealWritableArea);
		//PE: FASTLOAD - This takes 5 sec here, if not already loaded ?
		//HINSTANCE hinstance = ShellExecuteA(NULL, "open", g_pAbsPathToConverter, "", "", SW_SHOWDEFAULT);
		SetDir(pOldDir);
	}
	else
	{
		// STANDALONE GAME
		extern bool bSpecialStandalone;
		extern char cSpecialStandaloneProject[MAX_PATH];
		if (bSpecialStandalone)
			SetWriteSameAsRoot(false);
		else
			SetWriteSameAsRoot(true);
	}

	// new image loading system uses both legacy for IMGUI and wicked for rest
	// this image is loaded very early so the old image system can store it for use
	// when other non-UI images are loaded, this allows the old images to 'think'
	// they successfully loaded and permit normal logical flow through the engine
	LoadImage("Files\\Editors\\gfx\\dummy.png", 1);

	// ensures the DWORD to INT conversion always produces a positive value
	SetLocalTimerReset();

	// no more blue, only black
	if ( CameraExist ( 0 ) == 1 ) BackdropColor ( 0 );

	// lee - 170117 - deactivate old school DB collision system (so can use the collision.bActive for ray cast deactivation)
	GlobalColOff();

	// C++ CONVERSION: initialise globals
	// these were the globals previously defined in types
	common_init_globals();

	//  Init app
	SyncOn ( ); SyncRate ( 0 ); FogOff ( );

	//  initialise character animation system
	//t.charanimstate as charanimstatetype;
	char_init ( );
	t.aisystem.usingphysicsforai=1;

	//  Initialise physics tweakables
	physics_inittweakables ( );

	//  Get Actual screen resolution from desktop resolution
	//  Scene Commander - renamed SW and SH to ScreenW and ScreenH as names too short.

	// C++ CONVERSION: incase t.ScreenW is not 0
	t.ScreenW = 0;

	if (  t.ScreenW == 0 || t.ScreenH == 0 ) 
	{
		t.ScreenW = GetSystemMetrics ( 0 );
		t.ScreenH= GetSystemMetrics ( 1 );
	}
	t.Kernel32 = 1;
	t.memptr = PerformanceTimer();

	// some important resets
	strcpy_s ( g_pCloudKeyErrorString, 10240, "Unknown Validation Error");
	strcpy_s ( g_pCloudKeyExpiresDate, 11, "");

	//  flashlight
	g.flashlighton = 0;
	g.flashlightrange = 350;
	g.flashlightred = 255;

	//  limit raycast activity from enemies
	g.gnumberofraycastsallowedincycle = 0;
	g.gnumberofraycastslastoneused = 0;

	//  wobble
	g.wobble_f = 0.0;

	//  new deaths
	g.tiltondeath = 0;
	g.tilton = 0;
	g.tiltspeed_f = 0.0;
	g.temptilt = 0;
	g.thud = 0;
	g.tiltbounce = 10;
	g.justdone = 0;
	//  video
	g.unskip = 0;
	//  guns
	g.crosshairon = 1;
	g.forcedslot = 0;
	//  armour
	g.armour = 100;
	g.armouron = 0;
	g.armx = 18;
	g.army = 8;
	g.bodyon = 0;
	//  air
	g.airon = 0;
	g.airleft = 100;
	g.airmax = 100;
	g.drowntime = 2000;
	g.airtime = 2000;
	g.airtimer = MAXTimer();
	g.drowntimer = MAXTimer();
	g.airx = 24;
	g.airy = 8;
	g.instantdrown = 1;
	g.lastsetair = 0;
	g.drowned = 0;
	//  god mode
	g.isimmune = 0;
	//  new syncrate
	//  compass - knxrb
	g.compassOn = 0;
	g.compassX = 80;
	g.compassY = GetDisplayHeight()- 80;
	g.spritesPasted = 0;
	g.gameStarted = 0;
	g.madeCompass = 0;
	g.needleSpin = 0;
	g.compassSpin = 1;
	g.compassobject = 666666;
	g.needleobject = 666667;
	//  dark ai radar
	g.darkradar = 0;
	g.radarx = GetDisplayWidth()-80;
	g.radary = GetDisplayHeight()-80;
	g.maderadar = 0;
	g.rotateblip = 1;
	g.radarrange = 45;
	g.radarobject = 666669;
	g.blipstart = 666670;
	//  player speed mod
	g.speedmod_f = 100.0;
	//  radar/compass object as objective
	g.objectivemode = 0;
	g.istheobjective = 0;
	g.objectivex = 80;
	g.objectivey = GetDisplayHeight() - 80;
	g.madeobjective = 0;
	g.objectiveobject = 666691;
	g.maxslots = 10;
	//  Scene Commander water performance
	g.waterflec = 400;
	//  Scene Commander culling
	g.cullmode = 1;
	g.cullmodi = 650;
	g.plrfootfall = 1;
	g.forcealtswap = 0;
	g.moveplrx_f = 0.0;
	g.moveplry_f = 0.0;
	g.moveplrz_f = 0.0;
	g.noholster = 1;
	g.noairon = 0;
	g.drowndamage = 1;
	g.pickrange_f = 75.0;
	g.lastpickrange_f = 75.0;
	g.laststrength_f = 4000.0;
	g.lastthrow_f = 100.0;
	g.flashr = 255;
	g.flashg = 255;
	g.flashb = 255;
	g.flashrange = 600;
	g.playerdammult_f = 0.0;
	g.resetonreload = 0;
	g.ecam = 0;
	g.lastcam = 0;
	g.custstart = 0;
	g.custend = 0;
	g.plrreloading = 0;
	g.lockangle = 9999;
	g.fieldoffire = 45;
	g.plrcamoffsetx_f = 0.0;
	g.plrcamoffsety_f = 0.0;
	g.plrcamoffsetz_f = 0.0;
	g.plroffsetanglex_f = 0.0;
	g.plroffsetangley_f = 0.0;
	g.plroffsetanglez_f = 0.0;
	g.linkx = 0;
	g.linky = 0;
	g.linkz = 0;
	g.plrcamoffseton = 0;
	g.eplayercam = 0;
	g.decalrange = 800;
	g.cullmodelast = g.cullmode;
	g.cullmodechange = 0;
	//  Scene Commander - made global as otherwise it is being ignored in functions
	g.weaponammoindex = 0;
	g.ammooffset = 0;
	g.timeelapsed_f = 0.0;
	g.gentityundercursorlocked = 0;

	// `global syncrate=80 rem rem out to fix speed

	g.plrdistance_f = 0.0;
	g.pmaxX_f = 0.0;
	g.pmaxY_f = 0.0;
	g.pmaxZ_f = 0.0;
	g.sizechange = 0;
	g.firstturnjump = 0;
	g.alwaysshowair = 0;
	//  Scene Commander - mouse button timers, for tracking firing and also for conditions which seemed a logical extention condition.
	g.lmbheld = 0;
	g.lmbheldtime = 0;
	g.rmbheld = 0;
	g.rmbheldtime = 0;
	g.jamadjust = 0;
	g.screengrabtimer = MAXTimer();
	g.forcecrouch = 0;

	//  Used to record last best pick 3D coordinate (exact widget pos)

	//  scene commander - average FPS for smoother movement
	g.nextave = 1;
	Dim ( t.fpsstore , 80 ) ; for ( t.f = 1 ; t.f<= 80 ; t.f++ ) t.fpsstore[t.f]=80;
	//  Scene Commander end variables

	// set executable root folder
	if ( g.exeroot_s != "" ) SetDir ( g.exeroot_s.Get() );

	//  Special build flags for 'genre' switch
	g.fpgchud_s = "";
	g.fpgchuds_s = "";
	g.fpgcgenre = 0;
	if ( t.runengineinframe == 1 ) 
	{
		//  FPSC - 260210 - engine in frame to run FPSC in an ActiveX frame
		g.fpgchud_s="gun";
		g.fpgchuds_s="guns";
		g.fpgcgenre=1;
	}
	else
	{
		if ( PathExist("files\\gamecore\\equipment") == 1 ) 
		{
			// FPGC - Equipment Only Genre (no weapons)
			g.fpgchud_s="equipment";
			g.fpgchuds_s="equipment";
			g.fpgcgenre=0;
		}
		else
		{
			// GameGuru Shooter Genre
			g.fpgchud_s="gun";
			g.fpgchuds_s="guns";
			g.fpgcgenre=1;
		}
	}
	g.exeroot_s = GetDir();

	//  Hud Layers
	g.hudHName_s = "";
	
	//  - Global array for string variables
	Dim (  t.uservars,  1 );

	//  Hockeykid - 250210 - Dark AI added type for containers/layers
	Dim ( t.container,20);

	//  NEXTGENBRANCH ; DarkVoices
	t.nextgenbranch=1;

	//  Time stamp outside level scope reset
	g.timestampactivitymax = 0;
	g.timestampactivityflagged = 0;
	g.timestampactivityindex = 0;
	g.timestampactivitymemthen = 0;
	g.timestampactivityvideomemthen = 0;
	Dim(t.timestampactivity_s, 10); //PE: Not needed in wicked.
	//  speed up
	g.timebasepercycle_f = 0;
	g.timebasepercyclestamp = MAXTimer();
	g.timestampactivitymemthen = SMEMAvailable(1);

	//  FPGC - 090909 - mising media collector
	Dim (  t.missingmedia_s, 1  );
	Undim ( t.missingmedia_s );
	g.missingmediacounter = 0;

	//  FPSCV104RC9 - loading time readout to file
	g.gloadreportstate = 0;
	g.gloadreporttime = 0;
	g.gloadreportlasttime = 0;
	g.gloadreportindex = 0;
	g.loadreportarraydimmed = 0;

	//  Scene Commander - set up new animation textures
	g.animationimagestart = 666699;
	Dim (  t.animations,10  );
	for ( t.f = 1 ; t.f<= 10; t.f++ )
	{
		t.animations[t.f].img=g.animationimagestart+t.f;
	}

	//  Hockeykid - 250610 - Ai Factions
	g.FactionArrayMax = 20;
	g.mutualfactionoff = 0;

	//  Resource meter structures
	Dim (  t.resourcemeter,5  );
	Dim (  t.resourcemeter_f,5  );
	Dim (  t.resourcemeterdest_f,5  );

	//  game memory tracker (test game creates, editor uses to show in meter)
	Dim (  t.gamememtable,0  );
	Undim ( t.gamememtable );
	//PE: Box() dont work in wicked anyway.
	g.ghidememorygauge = 1;
	g.gamememactuallyused = 0;
	g.gamememactuallyusedstart = 0;
	g.gamememactualmax = (102400*(10-4));
	g.gamememactualmaxrightnow = g.gamememactualmax;
	g.gamememactualprompttime = 0;
	g.gamememactualprompt_s = "";
	g.gamememactuallyusedrt = 0;
	g.gamememresourceid = 0;
	g.mymousex = GetDisplayWidth()/2;
	g.mymousey = GetDisplayHeight()/2;

	//  Data structure to old player save data
	//t.saveplayerstate as saveplayerstatetype;
	g.hudhaveplayername = 0;
	g.localipaddress_s = "";
	g.serveripaddress_s = "";
	g.playername_s = "";
	g.soundfrequencymodifier = 0;
	//  Team Death Match - Code used by kind permission on Plystire.
	g.cap =0;
	//  V109 BETA3 - added to control change to player jump height
	g.playerdefaultjumpheight = 50;

	//  V118 - store range, aspect and fov globally!
	g.realrange_f = 9000.0f;
	g.realaspect_f = 4.0f/3.0f;
	g.realfov_f = 75.0f;
	
	//t.saveload as saveloadtype;
	Dim (  t.saveloadslot_s,9  );
	Dim (  t.saveloadgamepositionplayerinventory,100  );
	Dim (  t.saveloadgamepositionplayerobjective,99 );
	Dim (  t.saveloadgamepositionweaponslot,20  );
	g.gsaveloadobjectivesloaded = 0;
	g.mefrozentype = 0;
	g.mefrozen = 0;

	Dim (  t.material,100  );
	g.gmaterialmax = 0;


	// wicked MAX engine uses IMGUI in both mapeditor and standalone games, so init earlier
	timestampactivity(0, "Startup ImGui.");
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	//PE: Disable all imgui keyboard navigation here.
	//PE: Disabled for this to work: https://github.com/TheGameCreators/GameGuruRepo/issues/1239
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
	io.ConfigViewportsNoTaskBarIcon = true;
	// no layout saving until complete UI work
	extern char defaultWriteFolder[260];
	extern preferences pref;
	extern int refresh_gui_docking;
	refresh_gui_docking = 0; // reset layout.
	if (pref.save_layout) 
	{
		static char cLayoutFile[MAX_PATH];
		sprintf(cLayoutFile, "%suimax.layout", defaultWriteFolder);
		io.IniFilename = &cLayoutFile[0]; //Enable saving.
		ImGuiContext& g = *GImGui;
		extern int refresh_gui_docking;
		if (DoesFileExist(cLayoutFile)) {
			refresh_gui_docking = 4; // dont update layout.
		}
		
		if (pref.current_version_new_windows != MAXWINDOWSVERSION) {
			pref.current_version_new_windows = MAXWINDOWSVERSION;
			refresh_gui_docking = 0; // reset layout.
		}
		if (pref.current_version != MAXVERSION) {
			pref.current_version = MAXVERSION;
			refresh_gui_docking = 0; // reset layout.
			if (MAXVERSION >= 26) pref.iDisableObjectLibraryViewport = 1;
		}
	}
	else 
	{
		io.IniFilename = NULL; //Disable saving imgui.ini
	}

	// Setup Dear ImGui style
	myDefaultStyles();
	ImGui::StyleColorsDark();
	myDarkStyle(NULL); //for bordersize,padding ...
	myStyle2(NULL); //additional settings before change.

	//Restore style from preferences.
	if(pref.current_style == 0)
		myStyle2(NULL);
	else if (pref.current_style == 1)
	{
		void DarkColorsNoTransparent(void);
		myStyle2(NULL);
		DarkColorsNoTransparent();
	}
	else if (pref.current_style == 9)
	{
		myDarkStyle(NULL);
	}
	else if (pref.current_style == 2)
		ImGui::StyleColorsClassic();
	else if (pref.current_style == 3)
		myLightStyle(NULL);
	else if (pref.current_style == 25)
		myStyleBlue(NULL);
	else if (pref.current_style >= 10)
	{
		myStyle2(NULL);
	}

	extern cstr sDefaultImportPath;
	//We do a copy to sDefaultImportPath , so it can also remember the last place you selected a file.
	sDefaultImportPath = pref.cDefaultImportPath;

	// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
	ImGuiStyle& style = ImGui::GetStyle();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		style.WindowRounding = 0.0f;
	}

	//PE: Only allow moving window from titlebar area. if no titlebar you can still move it the old way.
	io.ConfigWindowsMoveFromTitleBarOnly = true;

	// Setup Platform/Renderer bindings
	ImGui_ImplWin32_Init(g_pGlob->hWnd);
	// TODO Phase 5: Replace with ImGui_ImplDX12_Init() using DX12 device from WickedEngine
	// Phase 4: DX11 device is no longer available — ImGui_ImplDX11_Init will return false (null guard)
	bool bDX11BackendOK = ImGui_ImplDX11_Init(m_pD3D, m_pImmediateContext);
	if (!bDX11BackendOK)
	{
		// DX11 backend failed (no device) — disable multi-viewport to prevent crashes
		// from unregistered renderer viewport callbacks
		io.ConfigFlags &= ~ImGuiConfigFlags_ViewportsEnable;
	}

	//PE: First check forupdates just after imgui is up , if we need some special render.
	CheckForNewUpdateWicked(); //PE: Check if update process is done, and ask if user like to update.

	//  Init FPSC then leap to SETUP.INI loader
	t.leavegamedataalone=0;
	FPSC_Full_Data_Init ( );

	CheckForNewUpdateWicked(); //PE: Check if update process is done, and ask if user like to update.

	FPSC_Setup();

	CheckForNewUpdateWicked(); //PE: Check if update process is done, and ask if user like to update.

	// WickedEngine has own main loop later in sequence - must pass this point to get to it
	if (stricmp(g.trueappname_s.Get(), "Guru-MapEditor") == NULL)
	{
		// and finally launch editor experience
		mapeditorexecutable_init();
	}
	else
	{
		gameexecutable_init();
	}
	extern void init_readouts();
	init_readouts();

}

//Against Steam Policy!
//bool g_bOfferSteamReviewReminder = false;

bool g_bOfferLatestUpdate = false;
void common_autoupdatecheck(void)
{
	if (g.skipupdatecheck == 0)
	{
		// Special piece of code which can update the updater if the build included a newer updater
		if (FileExist("..\\..\\GameGuru MAX Updater (new).exe") == 1)
		{
			// found a newer updater, replace existing one
			if (CopyFileA("..\\..\\GameGuru MAX Updater (new).exe", "..\\..\\GameGuru MAX Updater.exe", FALSE) == TRUE)
			{
				// only if replace successful, delete the newer updater copy
				DeleteFileA("..\\..\\GameGuru MAX Updater (new).exe");
			}
		}

		// In editor, also check for any new versions of the software

		CheckForNewUpdateWicked();

	}
}

bool bSkipAllGameLogic = false;
void common_loop_logic(void)
{
	extern bool bJustRederedScreenEditor;
	bJustRederedScreenEditor = false;
	if (bSkipAllGameLogic) return;

	if ( t.game.gameisexe == 0 )
		mapeditorexecutable_loop();
	else
		gameexecutable_loop();
}

void common_loop_render(void)
{
	// for standalone game, need sprites for title system (etc)
	if ( t.game.gameisexe == 1 )
	{
		// so can draw sprites and render using IMGUI
		extern bool	g_bDrawSpritesFirst;
		if (!g_bDrawSpritesFirst) UpdateSprites();
		extern bool bRenderTabTab;
		bRenderTabTab = true;
		ImGui_RenderLast();
		bRenderTabTab = false;
	}
	else
	{
		// eventually move render activity here so can draw LAST!
		extern bool bImGuiInTestGame;
		if (bImGuiInTestGame)
		{
			extern bool	g_bDrawSpritesFirst;
			if (!g_bDrawSpritesFirst) UpdateSprites(); //Draw all batched sprites. (like hand in pickupsimple.lua).
		}
		ImGui_RenderLast();
	}
}

void common_finish(void)
{
	if ( t.game.gameisexe == 0 )
		mapeditorexecutable_finish();
	else
		gameexecutable_finish();
}

const char *pestrcasestr(const char *arg1, const char *arg2)
{
	if (!arg1)
		return NULL;
	if (!arg2)
		return NULL;
	if (strlen(arg2) > strlen(arg1))
		return NULL;

	const char *a, *b;
	for (;*arg1;*arg1++) {

		a = arg1;
		b = arg2;

		while ((*a++ | 32) == (*b++ | 32))
			if (!*b)
				return (arg1);
	}
	return(NULL);
}

// New function to initialise all globals that were previously set up in types outside of any function/subroutine
void common_init_globals ( void )
{
	// Grab current folder
	g.fpscrootdir_s = GetDir();
	g.mydocumentsdir_s = Mydocdir();
	g.mydocumentsdir_s += "\\";
	g.myfpscfiles_s = "";
	g.myownrootdir_s = g.fpscrootdir_s + "\\";// +g.myfpscfiles_s + "\\";
	char pConfirmDestinationCreated[MAX_PATH];
	strcpy(pConfirmDestinationCreated, g.myownrootdir_s.Get());
	GG_GetRealPath(pConfirmDestinationCreated, 1);
	g.myownrootdir_s = pConfirmDestinationCreated;

	// Store globally (for custom content loading inside SteamCheckForWorkshop)
	SetWorkshopFolder ( g.fpscrootdir_s.Get() );

	//  Image Resources
	//  1-20 images used somewhere (terrain heightdata?)
	g.postprocessimageoffset = 21;
	//  +0 = camera zero image (central)
	//  +1 = light ray camera image
	//  +2 = sun image
	//  +3 = camera six image (left eye)
	//  +4 = camera seven image (right eye)
	//  +5 = dynamic terrain shadow camera image
	//  +6 = dynamic terrain shadow height map image
	g.titlesimageoffset = 300;
	//  +0 = backdrop image
	g.bulletholeimage = 386;
	g.savescreenshotimage = 387;
	g.quaddefaultimage = 388;
	g.testgamesplashimage = 389;
	// `global bitmapfontimagetart=390 moved further down to make MP name labels in FRONT of grass
	g.effectmenuimagestart = 400;
	g.muzzlebankoffset = 500;
	g.imagebankoffset = 550;
	g.prompt3dimageoffset = 799;
	g.promptimageimageoffset = 800;
	g.hudlayersimageoffset = 900;
	//  +1,2,3,4 = Jetpack textures (x8)
	//  .. 32 (for all 8 jet pack textures)
	g.weaponstempimageoffset = 999;
	g.weaponsimageoffset = 1000;
	g.particlesimageoffset = 1400;
	// reserve 200 particles 1400-1599
	g.ebeimageoffset = 1900;
	g.texturebankoffset = 2000;
	// ARG! Texture bank will eventually EAT into 3000 relatively quickly! Movec from 3000 to 43000
	//PE: 43000+ reserved for UIV3.
	//PE: 44000+ reserved for UIV3 preview images.
	//PE: 50000+ to be used for internal images inside dbo's.
	g.internalshadowdynamicterrain = 59950;
	g.internalshadowdebugimagestart = 59951;
	g.internalocclusiondebugimagestart = 59961;
	g.conkitimagebankoffset = 60000;
	g.luadrawredirectimageoffset = 62999;
	g.tempimageoffset = 63000;
	g.iconimagebankoffset = 63090;
	g.widgetimagebankoffset = 63100;
	g.huddamageimageoffset = 63200;
	//g.importerextraimageoffset = 63249;	//	For sphere in model importer.
	g.importermenuimageoffset = 63250; // was 63400
	g.importerextraimageoffset = 63310; //PE: was 63300 the same as used by importer/ccp/gridedit grab. caused problems with imgui when using deleteimage.
	//PE: 63340 reserved for terrain generator
	g.coneofsightimageoffset = 63349;
	g.visuallogicimageoffset = 63350;
	g.slidersmenuimageoffset = 63500;
	g.terrainimageoffset = 63600;
	g.gamehudimagesoffset = 64700;
	g.editorimagesoffset = 65110;
	g.editordrawlastimagesoffset = 75100;
	g.interactiveimageoffset = 75110;
	g.videothumbnailsimageoffset = 84900;
	g.explosionsandfireimagesoffset = 85000;
	g.bitmapfontimagetart = 89500;
	g.panelimageoffset = 90000;
	//PE: As g.charactercreatorimageoffset substract the -t.characterkitcontrol.bankOffset , it can overwrite lower ID like g.bitmapfontimagetart. Add 2000
	//PE: g.charactercreatorimageoffset still goes as low as 90020 so they are used.
	g.charactercreatorimageoffset = 92020;
	g.charactercreatorEditorImageoffset = 95000;
	g.LUAImageoffset = 96000;
	g.LUAImageoffsetMax = 105999;
	g.perentitypromptimageoffset = 110000; // allow 10,000 slots

	// Sprite ( Resource markers )
	g.ammopanelsprite = 63400;
	g.healthpanelsprite = 63401;
	g.steamchatpanelsprite = 63410;
	g.LUASpriteoffset = 63500;
	g.LUASpriteoffsetMax = 73499;
	g.ebeinterfacesprite = 74001;
	g.terrainpainterinterfacesprite = 74051;

	//  Mesh Resources
	g.meshgeneralwork2 = 991;
	g.meshebemarker = 992;
	g.meshebe = 993;
	g.meshebe1 = 994;
	g.meshebe2 = 995;
	g.meshsteam = 996;
	g.meshgeneralwork = 997;
	g.meshlightmapwork = 998;
	g.meshlightmapwork2 = 999;
	g.meshbankoffset = 1000;

	//  Effect Resources
	g.terraineffectoffset = 1;
	g.decaleffectoffset = 796;
	g.staticlightmapeffectoffset = 797;
	g.staticshadowlightmapeffectoffset = 798;
	g.jetpackeffectoffset = 799;
	g.quadeffectoffset = 800;
	g.postprocesseffectoffset = 901;
	g.postprocessobjectoffset0laststate = 0;
	//  +X = see postprocessimages for assignment
	g.effectbankoffset = 1000;
	g.explosionandfireeffectbankoffset = 1100;
	g.lightmappbreffectillum = 1295;
	g.controllerpbreffect = 1296;
	g.lightmappbreffect = 1297;
	g.thirdpersonentityeffect = 1298;
	g.thirdpersoncharactereffect = 1299;
	g.charactercreatoreffectbankoffset = 1300;

	//  Sound Resources
	g.soundbankoffset = 1;
	g.soundbankoffsetfinish = 8798;
	g.temppreviewsoundoffset = 8799;
	g.introsoundsoundoffset = 8809;
	g.titlessoundoffset = 8810;
	g.weaponssoundoffset = 8900;
	g.playercontrolsoundoffset = 9000;
	g.silentsoundoffset = 10000;
	g.materialsoundoffset = 10001;
	g.materialsoundoffsetend = 11000;
	g.explodesoundoffset = 11001;
	g.meleethumpsoundoffset = 11101;
	g.musicsoundoffset = 22000;
	g.musicsoundoffsetend = 22999;
	g.charactersoundoffset = 23000;
	g.explosionsandfiresoundoffset = 24000;
	g.projectilesoundoffset = 25000;
	g.steamsoundoffset = 30000;
	g.globalsoundoffset = 40000;
	// reserve 100 sound slots for HUD0.lua (40000 + 29000 (to29999) = hud0_sounds_levelup, etc)

	//  Object Resources
	//  1-10 - editor objects
	g.entitybankmax = 100;
	g.debugandmiscobjects = 1000; // 1000-4100
	// 1000+0001/1000 - visual character object
	// 1000+1001/2000 - AI entity ghost object (not currently used now)
	// 1000+2001/3000 - debug object to show an AI Go To
	g.virtualtreeobjectstart = 3400;
	g.virtualtreeobjectfinish = 3431;
	g.bulletholesobject = 3489;
	g.gameplayparentobjects = 3490; // 3490 thru 3496
	g.ghostcursorobjectoffset = 3497;
	g.waypointdetectworkobject = 3498;
	g.entityworkobjectoffset = 3499;
	g.entityattachmentsoffset = 3500; // 3500-3999
	g.entityattachmentindex = 0;
	// 1000+3001/4000 - debug objects to create AI entity ghost objs
	// 1000+3001 - debugentitymesh
	// 1000+3002 - debugentitymesh2
	// 1000+3003 - debugentityworkobj
	// 1000+3004 - debugentityworkobj2
	g.debugraycastvisual = 4009;
	g.debugconeofsightstart = 4010;
	g.debugconeofsightfinish = 5396;
	g.entityattachments2offset = 5400; // 5400-5899
	g.hudscreen3dobjectoffset = 5997;
	g.video3dobjectoffset = 5998;
	g.prompt3dobjectoffset = 5999;
	g.terrainobjectoffset = 6000;
	g.hudlayersbankoffset = 16000;
	//  +1 ; jetpack1
	g.hudbankoffset = 16050;
	g.gunbankoffset = 16100;
	g.brassbankoffset = 16250; // was 16150 - increased space for guns in level (more than 50)
	g.smokebankoffset = 16300;
	g.decalbankoffset = 16450;
	g.decalelementoffset = 16500;
	g.gunbankextraobjoffset = 16850;
	g.fragmentobjectoffset = 17000;
	g.explodedecalobjstart = 17500;
	g.characterkitobjectoffset = 17800;
	g.shadowdebugobjectoffset = 17890;
	g.importermenuobjectoffset = 17900;	
	g.importerextraobjectoffset = 17990;
	//PE: 17998 reserved for terrain genewrator.
	g.editorwaypointoffset = 18001;
	g.editorwaypointoffsetmax = 18499;
	g.debugobjectoffset = 18500;
	g.gamerealtimeobjoffset = 19300;
	g.gamerealtimeobjoffsetmax = 27999;
	g.conkitobjectbankoffset = 28000; // CONKIT REDUNDANT
	//  DON'T insert a new value in here. Conkit uses entitybankoffset as it's upper limit for object IDs
	g.entitybankoffset = 50000;
	g.temporarymeshobject = 65500;
	g.temporarydarkaiobject = 65535; // used as hard coded value in debug recastdetour!
	g.temporarydarkaiobjectend = 65634; // and reserved 100 objects for 65535 thru 65634
	g.ragdollplussystemobjstart = 69470;
	g.ragdollplussystemobjfinish = 69496;
	g.ragdollplussystemdebugobj = 69497;
	g.luadrawredirectobjectoffset = 69498;
	g.projectorsphereobjectoffset = 69499;
	g.tempobjectoffset = 69500; 
	g.temp2objectoffset = 69501; // fixed - value hard coded elsewhere (where g. not available)
	g.instancestampworkobject = 69991;
	g.darkaiobsboxobject = 69992;
	g.tempimporterlistobject = 69993;
	g.entityviewcursorobj = 70000;
	g.entityviewstartobj = 70001;
	g.entityviewendobj = 0;
	g.entityviewcurrentobj = g.entityviewstartobj;
	//PE: 87000 is used for draw call optimizer.
	g.weaponsobjectoffset = 90000;
	g.widgetobjectoffset = 91000;
	g.lightmappedobjectoffset = 92000;
	g.lightmappedobjectoffsetfinish = 92000;
	g.lightmappedterrainoffset = -1;
	g.lightmappedterrainoffsetfinish = -1;
	//PE: Wicked use 110000+ for yellow object dots.
	//PE: Wicked use 130000 for arcs.
	g.lightmappedobjectoffsetlast = 150000;
	g.postprocessobjectoffset = 150001;
	//  +0 = post process quad
	//  +1 = light ray scatter quad
	//  +2 = light ray sun plane object quad
	//  +3 = virtual reality RIFT second eye quad
	//  +5 = dynamic terrain shadow camera image
	//  [be aware anything added after 150001 might mess up post process?!] 
	g.batchobjectoffset = 85000; //160001;
	g.explosionsandfireobjectoffset = 170001;
	g.raveyparticlesobjectoffset = 180001;
	g.ebeobjectbankoffset = 189901;
	g.occlusionboxobjectoffset = 190001;
	g.occlusionboxobjectoffsetfinish = 199999;
	g.steamplayermodelsoffset = 200000;
	g.charactercreatorrmodelsbankoffset = 200000;
	g.charactercreatorrmodelsoffset = 201000;
	g.charactercreatorrmodelsoffsetEnd = 203000;
	g.perentitypromptoffset = 210000; // allow 10,000 slots

	g.physicssecondariesoffset = 220000; // used for hybrid entities that need a secondary object (door frames that are static)
	g.physicssecondariesoffsetend = 299999;

	g.physicsdebugdraweroffset = 300000;

	//  Particle Resources
	g.particlebankoffset = 1;

	//  Vector Resources
	g.m4_view = 1;
	g.m4_projection = 2;
	g.m4_viewproj = 3;
	g.v4_near = 4;
	g.v4_far = 5;
	g.v3_far = 6;
	g.universalvectorindex = 10;
	g.terrainvectorindex = 11;
	g.terrainvectorindex1 = 12;
	g.terrainvectorindex2 = 13;
	g.terrainvectorindex3 = 14;
	g.vegetationvectorindex = 15;
	g.weaponvectorindex = 16;
	g.generalvectorindex = 20;
	g.widgetvectorindex = 30;
	g.widgetStartMatrix = 40;
	g.characterkitvector = 46;
	g.ragdollvectoroffset = 100;

	//  Bitmap Resources
	g.terrainworkbitmapindex = 2;

	//  Camera Resources
	//  0 - main camera
	//  1 - reserved [for possible refraction camera]
	//  2 - reflection camera
	//  3 - post process camera
	//  4 - light ray camera
	//  5 - NOT USED FROM MAR2018 dynamic terrain shadow texture cam (cheap shadow)
	//  6 - left eye camera [rift]
	//  7 - right eye camera [rift]
	//  9 - map editor
	//  21 - generating CONKIT previews
	//  29 - work bitmap(create/delete)
	//  31 - shadow cameras+
	//  32 - used sometmies
	
	//Resource Bank Arrays
	

	//  Resource Banks
	g.soundbankmax = 0;
	Dim (  t.soundbank_s,10  );
	g.imagebankmax = 0;
	Dim (  t.imagebank_s,500  );
	g.effectbankmax = 0;
	Dim (  t.effectbank_s,100  );
	g.texturebankmax = 0;
	Dim (  t.texturebank_s,100  );
	g.gunbankmax = 0;
#ifdef REDUCEMEMUSE
	g.gunbankmaxlimit = 400;
#else
	g.gunbankmaxlimit = 1000;
#endif
	Dim ( t.gunbank_s, g.gunbankmaxlimit );
	g.muzzlebankmax = 0;
	Dim (  t.muzzlebank_s,100  );
	g.brassbankmax = 0;
	Dim (  t.brassbank_s,100  );
	g.smokebankmax = 0;
	Dim (  t.smokebank_s,100  );
	g.luabankmax = 0;
	Dim (  t.luabank_s,100  );

	
	//Global General Arrays
	

	//  General purpose global arrays
	Dim (  t.filelist_s,0  );

	//  General purpose global variables
	g.setupfilename_s = "setup.ini";
	g.fpgchud_s = "gun";
	g.fpgchuds_s = "guns";
	g.fpgcgenre = 1;
	g.lowfpswarning = 0;
	g.aidetectnearbymode = 0;
	g.aidetectnearbycount = 0;
	g.gphysicssessionactive = 0;

	//t.interactive as interactivetype;
	Dim (  t.interactivesequencemaxhistory,10  );
	Dim (  t.tutorialmaps_s,24  );

	//t.promptimage as promptimagetype;
	t.promptimage.show=0;

	Dim (  t.ccSamplePointX,3 );
	Dim (  t.ccSamplePointY,3 );
	Dim (  t.ccSampleSprite,3 );

	Dim (  t.mp_respawn_timed,MP_RESPAWN_TIME_OBJECT_LIST_SIZE  );

	Dim (  t.mp_destroyedObjectList,MP_DESTROYED_OBJECT_LIST_SIZE  );
	Dim (  t.mp_bullets,160   );
	Dim (  t.mp_bullets_send_time,160  );
	Dim (  t.mp_attachmentobjects,100   );
	Dim (  t.mp_gunobj,100   );
	Dim (  t.mp_gunname,100  );
	Dim (  t.mp_team,MP_MAX_NUMBER_OF_PLAYERS  );
	Dim (  t.mp_joined,MP_MAX_NUMBER_OF_PLAYERS );
	Dim (  t.mp_kills,MP_MAX_NUMBER_OF_PLAYERS   );
	Dim (  t.mp_deaths,MP_MAX_NUMBER_OF_PLAYERS  );
	Dim (  t.mp_lastIdleY,MP_MAX_NUMBER_OF_PLAYERS  );
	Dim (  t.mp_lastIdleReset,MP_MAX_NUMBER_OF_PLAYERS  );
	Dim (  t.mp_reload,MP_MAX_NUMBER_OF_PLAYERS  );
	Dim (  t.mp_playerShooting,MP_MAX_NUMBER_OF_PLAYERS  );
	Dim (  t.mp_playerAttachmentIndex,MP_MAX_NUMBER_OF_PLAYERS  );
	Dim (  t.mp_playerIsRagdoll,MP_MAX_NUMBER_OF_PLAYERS  );
	Dim (  t.mp_playerAttachmentObject,MP_MAX_NUMBER_OF_PLAYERS  );
	Dim (  t.mp_playerHasSpawned,MP_MAX_NUMBER_OF_PLAYERS  );
	Dim (  t.mp_oldAppearance,MP_MAX_NUMBER_OF_PLAYERS  );
	Dim (  t.mp_playingAnimation,MP_MAX_NUMBER_OF_PLAYERS  );
	Dim (  t.mp_playingRagdoll,MP_MAX_NUMBER_OF_PLAYERS  );
	Dim (  t.mp_oldplayerx,MP_MAX_NUMBER_OF_PLAYERS  );
	Dim (  t.mp_oldplayery,MP_MAX_NUMBER_OF_PLAYERS  );
	Dim (  t.mp_oldplayerz,MP_MAX_NUMBER_OF_PLAYERS  );
	Dim (  t.mp_meleePlaying,MP_MAX_NUMBER_OF_PLAYERS  );
	Dim (  t.mp_jetpackparticles,MP_MAX_NUMBER_OF_PLAYERS  );

	Dim (  t.mp_isDying,MP_MAX_NUMBER_OF_PLAYERS  );
	Dim (  t.mp_jetpackOn,MP_MAX_NUMBER_OF_PLAYERS  );
	Dim (  t.mp_lobbies_s,MP_MAX_NUMBER_OF_LOBBIES  );
	Dim ( t.mp_playerEntityID,MP_MAX_NUMBER_OF_PLAYERS  );
	Dim ( t.mp_forcePosition,MP_MAX_NUMBER_OF_PLAYERS  );
	Dim (  t.mp_health,MP_MAX_NUMBER_OF_PLAYERS  );
	Dim (  t.mp_chat,MP_MAX_CHAT_LINES  );
	Dim (  t.mp_subbedItems,20  );
	Dim (  t.mp_playerAvatars_s,MP_MAX_NUMBER_OF_PLAYERS  );
	Dim (  t.mp_playerAvatarOwners_s,MP_MAX_NUMBER_OF_PLAYERS  );
	Dim (  t.mp_playerAvatarLoaded,MP_MAX_NUMBER_OF_PLAYERS  );

	Dim (  t.mpmultiplayerstart,MP_MAX_NUMBER_OF_PLAYERS );

	//  RealSense Constants
	//-- Note; colour stream currently does nothing and should not be initialised;
	//-- Note; face recognition currently not fully implemented;
	//-- Note; USING_GESTURE is needed to enable depth image for body mass and finger tracking;
	//t.USING_GESTURE = 1 << 0;
	//t.USING_VOICE_RECOGNITION = 1 << 1;
	//t.USING_FACE_RECOGNITION = 1 << 2;
	//t.USING_BODY_MASS = 1 << 3;
	//t.USING_FINGER_TRACKING = 1 << 4;
	//t.USING_COLOUR_STREAM = 1 << 5;
	//g.FEATURES = t.USING_GESTURE | t.USING_VOICE_RECOGNITION | t.USING_FINGER_TRACKING | t.USING_BODY_MASS;
	//t.realsense as realsensetype;

	g.weaponSystem.numWeapons = WEAPON_MAXWEAPONS;
	g.weaponSystem.numProjectileBases = WEAPON_PROJECTILETYPES;
	g.weaponSystem.numProjectiles = WEAPON_MAXPROJECTILES;
	g.weaponSystem.numSounds = WEAPON_MAXSOUNDS;
	Dim (  t.Weapon , WEAPON_MAXWEAPONS  );
	Dim2(  t.WeaponAnimation , WEAPON_MAXWEAPONS ,  WEAPON_MAXANIMATIONS  );
	Dim (  t.WeaponProjectileBase,WEAPON_PROJECTILETYPES  );
	Dim (  t.WeaponProjectile,WEAPON_MAXPROJECTILES  );
	Dim (  t.WeaponSound,WEAPON_MAXSOUNDS  );

	//t.conkit as conkittype;
	t.conkit.editmodeactive=0;
	t.conkit.entityeditmode=0;
	t.conkit.objectstartnumber=g.conkitobjectbankoffset;
	t.conkit.imagestartnumber=g.conkitimagebankoffset;

	//t.widget as widgettype;
	t.widget.imagestart=g.widgetimagebankoffset;
	//  +1 = pos button
	//  +2 = rot button
	//  +3 = scl button
	//  +4 = prp button

	//t.characterkit as characterkitttype;
	t.characterkit.objectstart=g.characterkitobjectoffset;
	Dim (  t.characterkit_meshes_s,4  );
	Dim (  t.characterkit_diffuse_s,4 );
	Dim (  t.characterkit_normal_s,4  );
	Dim (  t.characterkit_mask_s,4 );
	//  1 - body obj
	//  2 - head obj
	//  0 - common specular
	//  1 - common occlusion
	//  2 - common illumination/blank
	//  11 - body diffuse
	//  12 - body normal
	//  13 - head diffuse
	//  14 - head normal
	g.characterkitbodymax = 0;
	g.characterkitbodyindex = 0;
	Dim (  t.characterkitbodybank_s , 1 );
	g.characterkitheadmax = 0;
	g.characterkitheadindex = 0;
	Dim (  t.characterkitheadbank_s , 1 );

	Dim (  t.importerTabs, 12  );

	Dim ( t.importerTextures, IMPORTERTEXTURESMAX  );

	Dim (  t.importerCollision,MACIMPORTERCOLLISIONSHAPES );
	Dim (  t.importerGridObject,10  );
	Dim (  t.selectedObjectMarkers,10  );

	Dim (  t.importerShaderFiles , IMPORTERSHADERFILESMAX  );
	Dim (  t.importerScriptFiles , IMPORTERSCRIPTFILESMAX  );

	g.obsmax = 10;
	g.obsindex = 0;
	Dim (  t.obs , g.obsmax );

	//t.terrainundo as terrainundotype;
	//PE: Not used in wicked. REDUCEMEMUSE
	Dim2(  t.terrainundobuffer,1024, 1024  );
	Dim2(  t.terrainredobuffer,1024, 1024 );

	t.terrainundo.mode=0;

	//t.entityundo as entityundotype;
	t.entityundo.undoperformed=0;
	t.entityundo.action=0;

	Dim (  t.screenblood , 41 );

	Dim (  t.damagemarker,40  );

	//t.huddamage as huddamagetype;
	t.huddamage.indicator=g.huddamageimageoffset;
	t.huddamage.bloodstart=g.huddamageimageoffset+1;
	t.huddamage.bloodtotal=0;
	t.huddamage.immunity=0;

	g.globals.riftmode = 0;
	g.globals.occlusionmode = 0;
	g.globals.occlusionsize = 5000;
	t.aisystem.obstacleradius = 18;

	g.globals.generateassetitinerary = 0;
	g.globals.generatehelpfromdocdoc = 0;

	t.postprocessings.fadeinvalue_f=0;

	t.game.ignoretitle=0;

	Dim2(  t.titlesbutton,20, 10  );
	g.titlesettings.graphicsettingslevel=2;
	Dim2(  t.titlesbar,20, 10  );
	g.titlessavefile_s = "settings.ini";
	
	//t.editor as editortype;
	t.editor.objectstartindex=0;
	//  objectstartindex; (1-10)
	//  +5 = Work area entity cursor
	//  +7 = cylinder to indicate resources

	g.hudlayerlistmax = 10;
	Dim (  t.hudlayerlist,g.hudlayerlistmax );

	g.ragdollvectorindex = g.ragdollvectoroffset;
	Dim (  t.ragdollvector,0  );
	g.RagdollsMax = 0;
	Dim (  t.Ragdolls,g.RagdollsMax );
	t.Ragdolls[g.RagdollsMax].obj=0;

	//  Hockeykid - 250610 - Ai Factions
	g.FactionArrayMax = 20;
	g.mutualfactionoff = 0;

	Dim (  t.material , 100 );
	g.gmaterialmax = 0;

	g.gentityprofileversion = 20100721;

	Dim2(  t.charactergunpose,100, 36  );

	g.maxslots = 10;
	g.autoswap = 1;

	Dim (  t.ammopool,100 );

	g.decalmax = 10;
	Dim (  t.decal,g.decalmax );
	t.decal[1].name_s="";
	//PE: For now until we found out why wicked use all that "update" time on non visible objects.
	//PE: REDUCEMEMUSE
	#ifdef REDUCEMEMUSE
	g.decalelementmax = 100; //PE: 120 lowered a bit as we now use particle effects more.
	#else
	g.decalelementmax = 199; //499;
	#endif
	Dim (  t.decalelement,g.decalelementmax );

	Dim (  t.weaponslot,12 ); //PE: 11 to be used for interaction hands, t.weaponammo[slot+10] still works.
	Dim (  t.weaponammo,20 );
	Dim (  t.weaponclipammo,20 );
	Dim (  t.weaponhud,12 );
	for (t.ws = 1; t.ws <= 12; t.ws++)
	{
		t.weaponslot[t.ws].pref = 0;
		t.weaponslot[t.ws].got = 0;
	}

	//  Gun Data
	g.noholster = 1;
	Dim (  t.gunslots_s, 10 );
	Dim ( t.listkey , 32 );
	for ( t.n = 0 ; t.n <= 32 ; t.n++ ) t.listkey[t.n]=0 ;
	Dim (  t.list_s, 100   );
	Dim (  t.smokeframe_f,30  );
	g.firemode = 0;
	g.bulletlimbsmax = 0;

	g.maxgunsinengine = g.gunbankmaxlimit;
	Dim ( t.gun,g.maxgunsinengine  );
	Dim2 ( g.firemodes,g.maxgunsinengine, 1 );
	Dim2 ( t.gunsound,g.maxgunsinengine, 15 );
	Dim3 ( t.gunsoundcompanion,g.maxgunsinengine, 15, 2 );
	Dim2 ( t.gunsounditem,g.maxgunsinengine, 100  );

	Dim (  t.brassfallcount_f,30 );
	g.autoloadgun = 0;
	g.gunslotmax = 0;

	Dim (t.soundloopcheckpoint,65535);
	Dim (t.soundloopgamemenu, 65535);
	Dim (t.soundloopstore, 65535);	

	g.playermax = 1;
	g.playertrailmax = 0;
	g.arrowkeyson = 1;
	g.jumponkey = 1;
	g.crouchonkey = 1;
	g.peekonkeys = 1;
	g.walkonkeys = 1;
	g.runkeys = 1;
	g.playeraction = 0;
	g.forcemove = 0;
	g.forcedamageon = 1;
	Dim (  t.player,g.playermax  );
	Dim2(  t.playersound,g.playermax, 520  );
	Dim2(  t.playersoundtimeused,g.playermax, 520  );
	Dim (  t.playersoundset_s,g.playermax  );
	Dim (  t.playermovementstep,g.playermax   );
	Dim2(  t.playerinventory,g.playermax, 100  );
	t.playersoundset_s[1]="";
	Dim (  t.playersoundsetindex,g.playermax  );
	g.soundsetlistmax = 0;
	Dim (  t.soundsetlist_s,g.soundsetlistmax  );
	Dim (  t.soundsetlist,g.soundsetlistmax  );
	Dim (  t.soundvolumes,2 );
	Dim (  t.playerobjective,99 );

	g.slidersprotoworkmode = 0;
	g.slidersmenumax = 0;
	Dim ( t.slidersmenu, 50 );
	Dim2( t.slidersmenuvalue, 50, 20 );
	g.sliderspecialview = 0;
	g.slidersmenufreshclick = 0;
	g.slidersmenudropdownscroll_f = 1;

	Dim (  t.nearestlightindex,4  );

	t.editorfreeflight.mode=0;

	Dim(t.objmeta, 1);

	Dim (  t.objinterestlist, 1   );
	Undim ( t.objinterestlist );

	//  Populate Infini Lights On The Fly In The Map Editor (see _realtimelightmapping_refreshinfiniwithe)
	Dim (  t.infinilight,0 );
	Undim ( t.infinilight );
	g.infinilightmax = 0;

	Dim2(  t.bitmapfont,10, 255 );

	Dim (  t.effectparamarray,1300 );
	t.terrain.waterlineyadjustforclip_f=0.0f;
	t.terrain.adjaboveground_f=30.0; // 070116 - caued spawned entity drop from sky 50.0f
	t.terrain.superflat=0;

	// 100417 - actually used for terrain default generation
	t.terrain.waterliney_f = 0;

	Dim2(  t.vegarea,t.terrain_vegarea_size+3, t.terrain_vegarea_size+3  );
	t.terrain.grassmemblock = 44;

	if (MemblockExist(t.terrain.grassmemblock) == 1) DeleteMemblock(t.terrain.grassmemblock);

	//  Resource start indices
	// `terrain.paintcameraindex=11

	t.terrain.objectstartindex=g.terrainobjectoffset;
	//  +0 - sphere acting as paint brush for paint camera
	//  +1 - RESERVED - NOT USED NOW [[plane to paste vegshadow to paint camera]]
	//  +2 - water plane for editor
	//  +3 - terrain object itself
	//  +4 - sky Box (  object )
	//  +5 - water plane for in-game
	//  +6 - grass parent object (and mesh index)
	//  +7 - grass mesh index for veg creation
	//  +8 - sky Box (  second object (for skybox fades) )
	//  +9 - sky Box (  skyscroll plane )
	//  +10 - sky Box (  skyscroll cloud portal image )
	//  +101/999 - unused
	//  +1000/5096 - terrain Physics Collision Meshes (LOD0=4096/LOD1=1024)
	//  +5097/6199 - unused but can be filled when terrain physics meshes reduced
	//  +6201/10000 - grass objects that cover all terrain
	//  10000 - MAX OBJECT USAGE

	t.terrain.imagestartindex = g.terrainimageoffset;
	//  +0 - water texture
	//  +1 - RESERVED - OLD [[[veg shadow RT texture]]] NOT USED NOW
	//  +2 - NEW veg shadow RT image
	//  +3 - initial height map texture
	//  +4 - water mask
	//  +5 - refraction camera image
	//  +6 - reflection camera image
	//  +7 - water normal texture
	//  +8 - grass texture
	//  +9 - skyscoll texture
	//  +13 - terrain diffuse map (4x4)
	//  +14 - R=AO, G=Gloss, B=hEight, A=Detail (AGED)
	//  +15 - Color Specular map
	//  +17 - highlighter texture
	//  +21 - terrain normal map (4x4)
	//  +24 - optional detail map for terrain
	//  +31 - PBR global env map
	//  +32 - PBR IBR curve lookup

	t.terrain.effectstartindex=g.terraineffectoffset;
	//  +0 - terrain lighting shader NON-PBR
	//  +1 - water shader
	//  +2 - vegetation shader NON-PBR
	//  +3 - terrain color shader
	//  +4 - sky shader
	//  +5 - terrain shader PBR
	//  +6 - vegetation shader PBR
	//  +9 - skyscroll shader

	//t.sky as skytype;
	g.skymax = 0;
	g.skyindex = 0;
	Dim ( t.skybank_s,1 );

	//  Terrain Texture Structure
	g.terrainstylemax = 0;
	g.terrainstyle_s = "";
	g.terrainstyleindex = 1;
	Dim ( t.terrainstylebank_s,g.terrainstylemax );

	//  Vegetation Texture Structure
	g.vegstylemax = 0;
	g.vegstyle_s = "";
	g.vegstyleindex = 1;
	Dim ( t.vegstylebank_s,g.vegstylemax );

	//t.gridedit as gridedittype;
	t.gridedit.autoflatten=0;
	t.gridedit.entityspraymode=0;
	t.gridedit.entitysprayrange=200;

	t.aisystem.terrainobsnum=1;
	t.aisystem.obs=2;

	//  Resource start indices
	t.aisystem.objectstartindex = g.debugandmiscobjects;
	//  +0 - AI player ghost object
	//  +0001/1000 - visual character object
	//  +1001/2000 - AI entity ghost object (not currently used now)
	//  +2001/3000 - debug object to show an AI Go To
	//  +3001/4000 - debug objects to create AI entity ghost objs
	//  +3001 - debugentitymesh
	//  +3002 - debugentitymesh2
	//  +3003 - debugentityworkobj
	//  +3004 - debugentityworkobj2
	t.aisystem.debugentitymesh = t.aisystem.objectstartindex + 3001;
	t.aisystem.debugentitymesh2 = t.aisystem.objectstartindex + 3002;
	t.aisystem.debugentityworkobj = t.aisystem.objectstartindex + 3003;
	t.aisystem.debugentityworkobj2 = t.aisystem.objectstartindex + 3004;
	// will be using g.debugandmiscobjects [1000] +3010 thru 5000 (real values 4010 thru 6000)
	// 4009 is g.debugraycastvisual
	//t.aisystem.debugconeofsightobj = g.debugconeofsightstart; // is 4010 g.debugconeofsightstart used directly now!!
	t.aisystem.imagestartindex=111;
	//  +0 - default character D texture (proto)
	//  +1 - default character I texture (proto)
	//  +2 - default character N texture (proto)
	//  +3 - default character S texture (proto)
	//  +4 - default building D texture (proto)
	//  +5 - default building N texture (proto)
	//  +6 - default building S texture (proto)
	//  +7 - default building S texture (proto)
	//  +11/99 - default character images (proto)
	t.aisystem.effectstartindex=4;
	//  +0 - character shader
	//  +1 - building shader
	t.aisystem.soundstartindex=1;
	//  +0 - player shot
	//  +1/1000 - character shot
	//  +1001 - player material footfalls

	g.charanimindex = 0;
	g.charanimindexmax = 0;

	//t.weapons as weaponstype;
	t.weapons.objectstartindex=3900;
	//  +0 - shot projection object for accurate Line (  of sight (hidden) )
	t.weapons.imagestartindex=3900;
	//  +0 -
	t.weapons.effectstartindex=5;
	//  +0 -
	t.weapons.soundstartindex=g.weaponssoundoffset;

	//  soundstartindex (see material_loadplayersounds)
	t.playercontrol.soundstartindex=g.playercontrolsoundoffset;

	Dim (  t.musictrack,MUSICSYSTEM_MAXTRACKS );

	g.characterSoundCount = 0;
	g.characterSoundBankCount = 0;
	
	g.characterSoundStackSize = 0;
	
	g.characterSoundCurrentPlayingNumber = 0;
	g.characterSoundCurrentPlayingType_s = "";
	g.characterSoundPrevPickedNumber = 0;
	
	Dim (  t.characterSoundName,CHARACTERSOUND_MAX_BANK  );
	Dim3(  t.characterSound,CHARACTERSOUND_MAX_BANK, CHARACTERSOUND_SIZE, CHARACTERSOUND_MAX_BANK_MAX_SOUNDS+1 );
	Dim (  t.characterSoundStackEntity,CHARACTERSOUND_STACK_SIZE  );
	Dim (  t.characterSoundStackType_s,CHARACTERSOUND_STACK_SIZE  );
	
	g.camshake_f = 0.0;
	
	//  flash
	g.lightrange = 100;
	g.lightmax = 600;
	g.lightmin = 0;
	g.lightspeed = 6;
	//  Maxemit=10 emitters, with totalpart of 260 for each emitter.
	g.totalpart = 260;
	g.maxemit = 10;
	//  Max Debris
	g.debrismax = 30;

	//  make and prepare particle and debris objects
	Dim (  t.ravey_particle_emitters,RAVEY_PARTICLE_EMITTERS_MAX  );
	Dim (  t.ravey_particles,RAVEY_PARTICLES_MAX  );
	g.ravey_particles_next_particle = 0;
	g.ravey_particles_old_time = 0;
	g.ravey_particles_time_passed = 0;
}

//  Subroutine to completely construct FPSCData
void FPSC_Full_Data_Init ( void )
{
	//  performance counters
	g.deactivatecollision = 0;
	g.entitysystemdisabled = 0;
	g.lightingsystemdisabled = 0;
	g.gameperftimetracker= MAXTimer();

	//  Water
	g.waterobj = 11;
	g.wateron = 0;
	g.oldwaterheight_f = -1.0;
	g.prevwaterheight_f = -1.0;
	g.waterheight_f = -1.0;
	g.tupdatewater = 0;
	g.waterfx = 11;
	g.waterbump_f = 0.2f;
	g.playerunderwater = 0;
	g.tnearsurfaceofwater = 100;
	g.waterred = 255;
	g.watergreen = 255;
	g.waterblue = 255;
	g.excludewatercams = 0;
	//  Scene Commander
	g.watercurrent = 0;
	g.waterflow = 1;

	//  logic control
	g.logicprioritycount = 0;
	g.logicprioritymax_f = 0.0;

	//  memory counters
	Dim (  t.mshot,500  );
	g.mshoti = 0;
	g.mshotmem = 0;
	g.mshotfirst = 0;
	g.mshotmemlargest = 0;
	g.lastmshoti = 0;
	g.lastmshotmem = 0;
	//  TDM - Plystire
	//  workload counters
	g.wshoti = 0;

	//  raw Text (  for HUD )
	g.grawtextr = 255;
	g.grawtextg = 0;
	g.grawtextb = 255;
	g.grawtextx = 50;
	g.grawtexty = 50;
	g.grawtextsize = 0;
	g.grawtextsizelast = 0;
	g.grawtextfont_s = "";
	g.grawtextfontlast_s = "";
	g.grawtextcount = 0;

	//Editors Data
	//  Browser Folder History
	Dim (  t.browserfolderhistory_s,10  );
	g.localdesc_s = "";

	//  Globals for FPG handling
	g.currentSMFPGtype = 1;
	g.currentSMFPG_s = "mygame.fpg";
	g.currentAMFPGtype = 2;
	g.currentAMFPG_s = "myarena.fpg";
	g.currentFPGtype = g.currentSMFPGtype;
	g.currentFPG_s = g.currentSMFPG_s;

	//  Other structres
	Dim (  t.taunt_s ,30 );

	//  Project working on 
	g.projectfilename_s = "";
	g.projectmodified = 0;
	g.projectmodifiedstatic = 0;

	//Global Data and Arrays

	//  additional globals for BUILD GAME speed-ups
	g.globalsmallsound = 0;
	g.currentlyintheAISCIPTloader = 0;

	g.particlebankmax = 0;
	g.materialsoundmax = 0;
	g.explodesoundmax = 0;
	Dim (  t.entitybank_s,g.entitybankmax  );

	//Map Data

	//  Define 50Kx50K Area (500*100)
	t.maxx=500 ; t.maxy=500;

	//  Visible-Col-Map used for per-cycle quick entity collision checks
	t.viscolx=160 ; t.viscoly=20 ; t.viscolz=160;


	//  Default settings
	g.gridlayershowsingle = 0;
	t.gridzoom_f=1.0;
	t.gridground=0;
	t.gridselection=1;
	t.nogridsmart=-1;
	t.gridlayer=5;
	t.bufferlayer=-1;
	t.grideditartwidth=1;
	t.grideditartwidthx=1;
	t.grideditartwidthy=1;
	t.locallibrarysegidmaster=0;
	t.locallibraryentidmaster=0;
	t.locallibraryentindex=0;
	Dim (  t.locallibraryent,t.locallibraryentindex  );

	//  resource counter to help prevent kids adding crazy amounts of stuff
	g.editorresourcecounter_f = 0;
	g.editorresourcecounterpacer = 0;

	g.animmax = 700;
	g.footfallmax = 200;
	#ifdef DEFAULTMASTERENTITY
	g.entidmastermax = DEFAULTMASTERENTITY;
	#else
	g.entidmastermax = 100;
	#endif

	//Dave fix - 100 was not enough for some stress test levels
	Dim2(  t.entityphysicsbox,MAX_ENTITY_PHYSICS_BOXES*2, MAX_ENTITY_PHYSICS_BOXES  );
	Dim2(  t.entitybodypart, g.entidmastermax, 100  );
	Dim2(  t.entityappendanim, g.entidmastermax, 100 );
	Dim2(  t.entityanim, g.entidmastermax, g.animmax );
	Dim2(  t.entityfootfall, g.entidmastermax, g.footfallmax  );
	Dim (  t.entityprofileheader, g.entidmastermax);
	Dim (  t.entityprofile, g.entidmastermax);
	Dim2(  t.entitydecal_s, g.entidmastermax, 100 );
	Dim2(  t.entitydecal, g.entidmastermax, 100 );

	g.entityelementlist = 0;
	g.entityelementmax = 100;
	Dim (  t.entityelement,g.entityelementmax  );
	Dim2(  t.entitybreadcrumbs,g.entityelementmax, 50  );

	//  New entity based shader variable array
	g.globalselectedshadermax = 4;
	g.globalselectedshadervar = 1;

	//  Segment and EntityProfile/EntityelementList Vars
	g.preidmaster = 0;
	g.segidmaster = 0;
	g.entidmaster = 0;
	g.entityelementlist = 0;
	g.aiindexmaster = 0;
	g.waypointmax = 0;
	g.wayppointoneonlyflaw = 0;
	g.gheadshotdamage = 65500;

	//  V109 BETA3 - 210408 - AI variables
	g.aivariablemode = 0;
	g.aivariableindex = 0;
	Dim (  t.aiglobals,99  );
	Dim2(  t.ailocals,1, 99   );
	Dim2(  t.aiuserlocals,1, 99  );

	//  AI Counters
	g.actstringmax = 0;
	g.conindexcount = 0;
	g.aicondseqcount = 0;
	g.actindexcount = 0;
	g.aiactseqcount = 0;
	g.hudmax = 0;
	g.hudfadeoutoneatatime = 0;
	g.internalloaderhud = 0;
	g.internaleyehud = 0;
	g.internalfaderhud = 0;

	//  AI BC Sound
	//Dim (  t.aiactionseq,10000 ); //PE: Not used.
	Dim (  t.aiaction,500 );
	//Dim (  t.actstring_s,g.actstringmax  ); //PE: Got crash here, not used anyway ?

	//  AI conditions
	//Dim (  t.aiconditionseq,10000  ); //PE: Not used.
	Dim (  t.aicond,500 );

	//  AI Library List
	Dim (  t.ailist,200 );

	//  AI Library Count
	if (  t.leavegamedataalone == 0 ) 
	{
		Dim (  t.scriptbank_s,100  );
		g.aiindexmaster=0;
	}

	Dim (  t.waypointcoord,1000 );
	Dim (  t.waypoint,10 );
	g.waypointeditheight_f = 0;
	g.waypointcoordmax = 0;
	g.waypointmax = 0;

	//  Infini lights
	Dim (  t.infinilight,0 );
	Undim(t.infinilight);
	Dim (  t.infinilightshortlist,0  );
	Undim (t.infinilightshortlist);

	//  Shadow Lights Data Structure
	Dim (  t.shadowlight,0 );

	//  Data structure for Bit-Fragments
	Dim (  t.bitdetails,10  );
	Dim2(  t.bitoffset,10, 8 );

	//  Explosion Data Structure
	g.explodermax = 4;
	Dim (  t.exploder,g.explodermax );

	//  GUI Visual Settings
	t.guivisualsettings.ambienceoverride=-1;

	//  HUD
	Dim (  t.hud,10 );
	g.saveloadgamehudmax = 0;
	Dim (  t.saveloadgamehud,10 );

	//  FPSCV104 Fog globals
	g.hudfognear = -2000.0;
	g.hudfogfar = -10000.0;

	//  Water Fog globals
	g.waterfogfar = 2000.0;
	g.waterfogred = 55;
	g.waterfoggreen = 65;
	g.waterfogblue = 75;

	//  LightRay Addition
	//  LRMod globals
	g.rotvar_f = 0.0;
	g.lrsamples = 1;
	g.lroldsamples = g.lrsamples;
	g.lrswitchsamples = 1;
	g.lrbloomactive = 1;
	g.lrswitchbloomactive = 1;
	g.lroldbloomactive = g.lrbloomactive;
	g.lrdebugdeactive = 0;

	//  World Physics Settings
	g.physicson = 1;
	g.physicsdebug = 0;
	g.physicsgravx_f = 0.0;
	g.physicsgravy_f = -40.0;
	g.physicsgravz_f = 0.0;
	g.physicsplayerweight_f = 500.0;
	g.grav_f = 0;
	g.camerapositionx = 0;
	g.camerapositiony = 0;
	g.camerapositionz = 0;
	g.cameraspeed = 0;
	g.camerapickup = 1;
	g.cameraholding = 0;
	g.camerapickupkeyrelease = 0;
	g.camerareach_f = 0;
	g.camerareachatrun_f = 0;
	g.camerareachmax_f = 75;
	g.camerapickedangle_f = 0;
	g.camerapicked = 0;
	g.camerapickede = 0;
	g.camerapickeddrop = 0;
	g.camerapickedthrown = 0;
	g.camerathrow_f = 100.0;
	g.camerathrowelev_f = 0.0;
	g.cameradampen_f = 1.0;
	g.cameradampenactive = 0;
	g.cameracarryweight_f = 4000;
	Dim (  t.phyobjvelocity_f,1  );
	Dim (  t.phylasttravelled_f,1  );
	Dim (  t.phylastfloorstop_f,1  );
	Dim (  t.phyobjsounding,1  );
	Dim (  t.phyobjremove,1  );
	Dim (  t.phyobjele,1  );
	Dim (  t.shadowobj,1  );

	//  Respawn array for arena game
	Dim (  t.respawn,16 );
	g.respawnmax = 0;

	//  Multiplayer globals and structures
	g.hudiplistmax = -1;
	Dim2(  t.hudiplist_s,20, 1  );
	g.repeatsamelevel = 0;
	g.winnersname_s = "";
	g.servername_s = "FPSC Creator Portal";
	//  talktoaster arrays
	Dim (  t.talkscript_s,10  );
	Dim (  t.talkscriptcount,10  );
	Dim (  t.talkscriptwho,10  );
	//  Characters (chosen is indexed by iLocalEL, list is flaglist of used identities)
	//g.multiplayermax = 16;
	g.multiplayermax = 1; //PE: Do not think any of these are used.
	Dim (  t.characterchosen,g.multiplayermax  );
	Dim (  t.characterchoiceentityindex,g.multiplayermax  );
	Dim (  t.characterlist_s,g.multiplayermax );
	Dim (  t.characterlist,g.multiplayermax  );
	Dim (  t.characterlistentity,g.multiplayermax  );
	//  Dead reckoning temp arrays
	Dim (  t.cpx_f,4  );
	Dim (  t.cpy_f,4  );
	Dim (  t.cpz_f,4  );
	Dim (  t.stategetready,g.multiplayermax  );
	Dim (  t.statex,g.multiplayermax  );
	Dim (  t.statey,g.multiplayermax  );
	Dim (  t.statez,g.multiplayermax  );
	Dim (  t.statea,g.multiplayermax  );
	Dim (  t.stateanim,g.multiplayermax  );
	Dim (  t.stateanimdir,g.multiplayermax  );
	Dim (  t.statecolmaterialtype,g.multiplayermax  );
	Dim (  t.stateanimwait,g.multiplayermax  );
	Dim (  t.statewhodidit,g.multiplayermax  );
	Dim (  t.stateplayagain,g.multiplayermax  );
	Dim (  t.stateviewy,g.multiplayermax  );
	Dim (  t.statesviewdy,g.multiplayermax );
	Dim (  t.stateweapon,g.multiplayermax  );
	Dim (  t.stateannounce,g.multiplayermax  );
	Dim (  t.statesx,g.multiplayermax  );
	Dim (  t.statesy,g.multiplayermax  );
	Dim (  t.statesz,g.multiplayermax  );
	Dim (  t.statesa,g.multiplayermax  );
	Dim (  t.statesvel,g.multiplayermax  );
	Dim (  t.statetx,g.multiplayermax  );
	Dim (  t.statety,g.multiplayermax  );
	Dim (  t.statetz,g.multiplayermax  );
	Dim (  t.statedx,g.multiplayermax  );
	Dim (  t.statedy,g.multiplayermax  );
	Dim (  t.statedz,g.multiplayermax  );
	Dim (  t.statemove,g.multiplayermax  );
	Dim (  t.statelag,g.multiplayermax  );
	Dim (  t.statemsgap,g.multiplayermax  );
	Dim (  t.statemytimer,g.multiplayermax  );
	Dim2(  t.statecodeupdate,g.multiplayermax, 4  );
	Dim (  t.stateraycastpace,g.multiplayermax  );

	//  Server Scores
	Dim (  t.frags,g.multiplayermax );
}

