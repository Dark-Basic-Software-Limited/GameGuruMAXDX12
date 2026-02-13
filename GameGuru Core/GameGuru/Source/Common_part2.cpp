void FPSC_Setup(void)
{
	// prepare all default values 
	InitCTextC(); //PE: Init new CTextC.
	FPSC_SetDefaults();

	// moved detection of EDITOR vs Game further up
	FPSC_VeryEarlySetup();

	// quick Start state
	g.gfirsttimerun = 0;
	g.gshowonstartup = 0;
	///MessageBox(NULL, "", "", MB_OK);
	g.gshowannouncements = 1;
	if (g.trueappname_s == "Guru-MapEditor")
	{
		// startup file
		t.tfile_s = "showonstartup.ini";
		if (FileOpen(1) == 1) CloseFile(1);
		if (FileExist(t.tfile_s.Get()) == 1)
		{
			OpenToRead(1, t.tfile_s.Get());
			t.tshowonstart_s = ReadString(1);
			CloseFile(1);
			g.gshowonstartup = ValF(t.tshowonstart_s.Get());
		}
		else
		{
			// first time run (for welcome and serial code system)
			g.gfirsttimerun = 1;
			g.gshowonstartup = 1;
			OpenToWrite(1, t.tfile_s.Get());
			WriteString(1, "1");
			CloseFile(1);
		}

		// news announcement flag
		t.tfile_s = "showannouncements.ini";
		if (FileOpen(1) == 1) CloseFile(1);
		if (FileExist(t.tfile_s.Get()) == 1)
		{
			OpenToRead(1, t.tfile_s.Get());
			cstr tshowannouncements_s = ReadString(1);
			CloseFile(1);
			g.gshowannouncements = ValF(tshowannouncements_s.Get());
		}
		else
		{
			OpenToWrite(1, t.tfile_s.Get());
			WriteString(1, "1");
			CloseFile(1);
		}
	}

	// 050416 - get parental control flag and any password
	g.quickparentalcontrolmode = 0;

	// 050416 - get VRQ control flag and serial code
	g.vrqorggcontrolmode = 0;
	g.vrqcontrolmode = 0;
	g.vrqcontrolmodeserialcode = "";
	g.cleverbooksmodeusername = "";
	g.cleverbooksmodepassword = "";
	g.cleverbooksmodeentryindex = 0;
	g.vrqTriggerSerialCodeEntrySystem = 0;
	g.iTriggerSoftwareToQuit = 0;
	g.bCleverbooksBundleMode = false;
	if (g.trueappname_s != "Guru-MapEditor")
	{
		// standalone games need no serial code
	}
	else
	{
		t.tfile_s = "ggcontrolmode.ini";
		if (FileExist(t.tfile_s.Get()) == 0)
		{
			OpenToWrite(1, t.tfile_s.Get());
			WriteString(1, "ALPHA");
			CloseFile(1);
		}
		g.vrqorggcontrolmode = 2;

		if (g.vrqorggcontrolmode > 0)
		{
			// cleverbooks or vrq standalone
			if (g.bCleverbooksBundleMode == true)
			{
				// read username and password (default is "username" "password" if not used yet)
				g.vrqcontrolmode = 1;
				if (FileOpen(1) == 1) CloseFile(1);
				OpenToRead(1, t.tfile_s.Get());
				g.cleverbooksmodeusername = ReadString(1);
				g.cleverbooksmodepassword = ReadString(1);
				CloseFile(1);
			}
			else
			{
				// read serial code from controlmode file
				g.vrqcontrolmode = 1;
				if (FileOpen(1) == 1) CloseFile(1);
				OpenToRead(1, t.tfile_s.Get());
				g.vrqcontrolmodeserialcode = ReadString(1);
				CloseFile(1);
			}

			// ensure serial code has not expired, otherwise ask for new serial code
			bool bHaveValidAuthCredentialsStored = false;
			if (g.bCleverbooksBundleMode == true)
			{
				// fresh install stores username as "username"
				if (stricmp(g.cleverbooksmodeusername.Get(), "username") != NULL)
					bHaveValidAuthCredentialsStored = true;
			}
			else
			{
				if (strlen(g.vrqcontrolmodeserialcode.Get()) > 1)
					bHaveValidAuthCredentialsStored = true;
			}
			if (bHaveValidAuthCredentialsStored == true)
			{
				// determine valid code
				int iValidCode = 0;
				if (g.bCleverbooksBundleMode == true)
					iValidCode = common_isserialcodevalid(g.cleverbooksmodepassword.Get(), g.cleverbooksmodeusername.Get());
				else
					iValidCode = common_isserialcodevalid(g.vrqcontrolmodeserialcode.Get(), NULL);

				// is code valid
				if (iValidCode <= 0)
				{
					// serial code expired
///MessageBoxA(NULL, "Build has expired! Find out more at https://www.game-guru.com/max", "Activation Error", MB_OK);
					if (iValidCode == -1)
					{
						// no internet connection so cannot check key, just quit!
						g.iTriggerSoftwareToQuit = 4;
					}
					else
					{
						g.vrqTriggerSerialCodeEntrySystem = 1;
						g.iTriggerSoftwareToQuit = 1;
					}
				}
				else
				{
					// serial code valid - continue
					g.vrqTriggerSerialCodeEntrySystem = 0;
				}
			}
			else
			{
				// no serial code found, ask for one
				g.vrqTriggerSerialCodeEntrySystem = 1;
				g.iTriggerSoftwareToQuit = 1;
			}
			g.vrqTriggerSerialCodeEntrySystem = 0;
			g.iTriggerSoftwareToQuit = 0;

			// all VRQ is restricted content mode
			g.quickparentalcontrolmode = 2;
		}

		//g.vrqTriggerSerialCodeEntrySystem = 0;
		//g.iTriggerSoftwareToQuit = 0;

		g.quickparentalcontrolmode = 0;
	}

	// standalones need to know we are running in VR flavor
	if (g.iTriggerSoftwareToQuit == 0)
	{
		g.vrqcontrolmode = 1;
	}

	//  Review Request Reminder state
	g.reviewRequestReminder = 0;

	//  Version Control - TEST GAME Mode
	g.gtestgamemodefromeditor = 0;
	g.gtestgamemodefromeditorokaypressed = 0;
	//version_commandlineprompt ( );

	//  get version information from version file 
	g.version_s = "";
	g.gversion = 10000;
	if (FileExist("versionauto.ini") == 1)
	{
	}
	if (FileExist("version.ini") == 1)
	{
		Dim(t.data_s, 99);
		LoadArray("version.ini", t.data_s);
		t.version_s = "";
		for (t.n = 1; t.n <= Len(t.data_s[0].Get()); t.n++)
		{
			t.c_s = Mid(t.data_s[0].Get(), t.n);
			if (t.c_s != ".")  t.version_s = t.version_s + t.c_s;
		}
		g.version_s = t.data_s[0].Get();
		g.gversion = ValF(t.version_s.Get());
		UnDim(t.data_s);
	}

	// store version.ini for potential crash log
	extern char g_pCrashVersionINIValue[256];
	strcpy ( g_pCrashVersionINIValue, g.version_s.Get() );

	//  defeat 'cumilative virtual memory limit of 1.8GB' by terminating
	//  FPSC-Game.exe each complete level, and re-launching to start new heap each time
	g.startbuildinglevelfromdir_s = GetDir();
	g.startbuildingleveloverride = 1;

	t.tcommandline_s = gRefCommandLineString;
	if (strcmp(Lower(Left(t.tcommandline_s.Get(), 3)), "-bl") == 0)
	{
		g.startbuildingleveloverride = ValF(Right(t.tcommandline_s.Get(), Len(t.tcommandline_s.Get()) - 3));
		t.tcommandline_s = "-b";
	}

	//  COMMANDLINE Info
	g.grestoreeditorsettings = 0;
	if (strcmp(Lower(gRefCommandLineString), "-r") == 0)
	{
		//  RESTORE MAP Mode - exited editor prematurely
		g.grestoreeditorsettings = 1;
	}

	// can reload standalone to clear fragmentation 
	g.iStandaloneIsReloading = 0;
	g.sStandaloneIsReloadingLevel = "";
	g.sStandaloneIsReloadingLevelGameStatChange = "";
	if (strcmp(Lower(Left(gRefCommandLineString, 17)), "-reloadstandalone") == 0)
	{
		if (strcmp(Lower(Left(gRefCommandLineString, 22)), "-reloadstandalonelevel") == 0)
		{
			// load into a specific level
			g.sStandaloneIsReloadingLevel = gRefCommandLineString + 22;
			// and extract any advancelevelfilename (from LOAD GAME functionality)
			char pSliceUp[2048];
			strcpy(pSliceUp, g.sStandaloneIsReloadingLevel.Get());
			LPSTR pAdvance = strstr(pSliceUp, ":");
			if (pAdvance > 0)
			{
				g.sStandaloneIsReloadingLevelGameStatChange = pAdvance + 1;
				*pAdvance = 0;
				g.sStandaloneIsReloadingLevel = pSliceUp;
			}
			g.iStandaloneIsReloading = 2;
		}
		else
		{
			// load to the main menu
			g.iStandaloneIsReloading = 1;
		}
	}

	// Check and load SETUP.INI defaults
// using new DocWrite system

	FPSC_LoadSETUPINI(false);
	FPSC_LoadSETUPVRINI();

	//PE: setup overwrite for standalone.
	extern bool bSpecialStandalone;
	extern int g_iDevToolsOpen;
	extern preferences pref;
	if (bSpecialStandalone)
	{
		t.game.ignoretitle = 0;
		g_iDevToolsOpen = 0;
	}
	else
	{
		g_iDevToolsOpen = pref.iDevToolsOpen;
	}

	// load key bindings - moved to common_mustreload_foreachnewproject so can be reloaded based on project
	// FPSC_LoadKEYMAP();

	// 250917 - set default CPU animation flag for engine
	if ( g.allowcpuanimations == 0 )
		SetDefaultCPUAnimState ( 3 );
	else
		SetDefaultCPUAnimState ( 0 );

	// set adapter ordinal for next time display mode is set (below)
	if ( g.gadapterordinal>0 ) 
	{
		ForceAdapterOrdinal ( g.gadapterordinal );
	}
	ForceAdapterD3D11ONLY ( g.gadapterd3d11only );

	// true name of app to log

	backuptimestampactivity();
	sprintf(t.szwork, "Name of true app: %s", g.trueappname_s.Get());
	timestampactivity(0, t.szwork);

	//  Indicate a time stamp for app start time
	timestampactivity(0,"Flag to produce log files set in SETUP.INI (producelogfiles=1)");

	//  FPGC - 020810 - sub-PS2.0 cards CANNOT have post processing
	if (  g.gpostprocessing>0 ) 
	{
		t.tpsv_f=GetPixelShaderVersion();
		if (  t.tpsv_f<2.0  )  g.gpostprocessing = 0;
	}
	sprintf ( t.szwork , "postprocessing flag at %i and pixel shader version of %.2f" , g.gpostprocessing , t.tpsv_f );
	timestampactivity(0,t.szwork);

	//  Language neutral ( USERDETAILS Info )
	g.language_s = "";
	t.tfile_s="userdetails.ini";
	if (  FileExist(t.tfile_s.Get()) == 1 ) 
	{
		Dim (  t.data_s,999  );
		LoadArray (  t.tfile_s.Get() ,t.data_s);
		for ( t.l = 0 ; t.l<=  999; t.l++ )
		{
			t.line_s=t.data_s[t.l];
			if (  Len(t.line_s.Get())>0 ) 
			{
				if (  cstr(Lower(Left(t.line_s.Get(),4))) == ";end"  )  break;
				if (  cstr(Left(t.line_s.Get(),1)) != ";" )
				{
					//  take fieldname and values
					for ( t.c = 0 ; t.c <  Len(t.line_s.Get()); t.c++ )
					{
						if (  t.line_s.Get()[t.c] == '=' ) { t.mid = t.c+1 ; break; }
					}
					t.field_s=Lower(removeedgespaces(Left(t.line_s.Get(),t.mid-1)));
					t.value_s=removeedgespaces(Right(t.line_s.Get(),Len(t.line_s.Get())-t.mid));
					for ( t.c = 0 ; t.c <  Len(t.value_s.Get()); t.c++ )
					{
						if (  t.value_s.Get()[t.c] == ',' ) { t.mid = t.c+1  ; break; }
					}
					t.value1=ValF(removeedgespaces(Left(t.value_s.Get(),t.mid-1)));
					t.value2_s=removeedgespaces(Right(t.value_s.Get(),Len(t.value_s.Get())-t.mid));
					if (  Len(t.value2_s.Get())>0  )  t.value2 = ValF(t.value2_s.Get()); else t.value2 = -1;
	
					//  localization data
					t.tryfield_s = "language" ; if (  t.field_s == t.tryfield_s  )  g.language_s = t.value_s;
				}
			}
		}
		UnDim (  t.data_s );
	}

	//  Physics debug mode (to see all collision shapes)
	if (  g.gdebugphysicsstate == 1  )  g.physicsdebug = 1;

	//  Forced switches
	if (  g.gcompilestandaloneexe == 1 ) 
	{
		//  Cannot be real game, must optimize
		g.grealgameviewstate=0;
		g.goptimizemode=1;

		//  Wipe any imported unique code (generate when build EXE)
		g.guniquegamecode_s="";
	}
	if (  g.gtestgamemodefromeditor == 1 ) 
	{
		g.grealgameviewstate=0;
	}

	// The MyGames folder precedes Files
	g.exedir_s = g.myownrootdir_s;//GetDir();
	g.exedir_s += "\\MyGames\\";
	sprintf ( t.szwork , "The executable folder will be: %s" , g.exedir_s.Get() );
	timestampactivity(0,t.szwork);

	// record location of lightmapper executable
	g.originalrootdir_s = GetDir();
	g.lightmapperexefolder_s = GetDir();

	// First task for any program is to enter the Files Folder
	t.tnopathprotomode=0;
	if ( PathExist("Files") == 1 ) 
		SetDir ( "Files" );
	else
		t.tnopathprotomode=1;
	sprintf ( t.szwork , "Entering 'Files' folder, now at: %s" , GetDir() );
	timestampactivity(0,t.szwork);

	//  Basic globals for all programs
	g.gmapeditmode = 0;

	//  Establish global files and folders
	g.imgext_s = "tga";
	g.rootdir_s = GetDir();
	g.rootdir_s +="\\";
	g.browserexe_s = "Guru-Browser.exe";
	g.browsername_s = "Guru TEST Browser";
	g.segeditexe_s = "Guru-Segments.exe";
	g.segeditname_s = "Guru TEST Segment Editor";
	g.gameexe_s = "Guru-Game.exe";
	g.gamename_s = "Guru Game";

	//  Default directories
	g.currentmeshdir_s = g.rootdir_s+"meshbank\\";
	g.currententitydir_s = g.rootdir_s+"entitybank\\";
	g.currenttexdir_s = g.rootdir_s+"texturebank\\";
	g.currentfxdir_s = g.rootdir_s+"effectbank\\";
	g.currentpredir_s = g.rootdir_s+"prefabs\\";
	g.currentsegdir_s = g.rootdir_s+"segments\\";
	g.currentvideodir_s = g.rootdir_s+"videobank\\";

	//  rem Right away set LEGACY CONVERT MODE (LOAD OBJECT should not load extra UV layers)
	timestampactivity(0,"set legacy convert mode");
	SetLegacyMode (  1 );

	//  reserve chunks of memory early to avoid fragmentation errors
	timestampactivity(0,"_obs_memorychunkinit");
	obs_memorychunkinit ( );

	t.game.fakegameisexe = 0;

	//  And finally switch the resolution if different from default
	if ( 1 ) 
	{
		timestampactivity(0, g.trueappname_s.Get());
		if ( g.trueappname_s == "Guru-MapEditor" )
		{
			// MAP EDITOR MODE
			timestampactivity(0, "detected map editor is in effect");
			t.game.gameisexe=0;
			t.game.set.ismapeditormode=1;
			tgamesetismapeditormode = 1;
			// set backbuffer for editor
			t.bkwidth=GetDesktopWidth() ; t.bkheight=GetDesktopHeight();
			t.thevrmodeflag = 0; if ( g.gvrmode == 1 || g.gvrmode == 5 || g.gvrmode == 6 ) t.thevrmodeflag = 1;
			timestampactivity(0, "setting display mode via DirectX");
			if ( t.thevrmodeflag != 0 )
			{
				SetDisplayModeVR ( GetDesktopWidth(),GetDesktopHeight(),GetDisplayDepth(), g.gvsync,0,0,0,0,t.thevrmodeflag );
			}
			else
			{
				SetDisplayModeMODBACKBUFFER (  GetDesktopWidth(),GetDesktopHeight(),GetDisplayDepth(),g.gvsync,0,0,t.bkwidth,t.bkheight );
			}

			// if flag set to generate DOCDOC help, do this here
			if ( g.globals.generatehelpfromdocdoc == 1 )
			{
				timestampactivity(0, "GenerateDOCDOCHelpFiles");
				GenerateDOCDOCHelpFiles();
			}
		}
		else
		{
			// NOT MAP EDITOR - GAME EXECUTABLE
			timestampactivity(0, "detected game executable in effect");

			// we start a new IMGUI frame right away to handle any paste commands to the 2D screen
			extern bool bImGuiFrameState;
			if (!bImGuiFrameState)
			{
				//PE: All fonts needed also in standalone.
				ChangeGGFont("editors\\uiv3\\Roboto-Medium.ttf", 15);
				ImGui_ImplDX11_NewFrame();
				ImGui_ImplWin32_NewFrame();
				ImGui::NewFrame();
				bImGuiFrameState = true;
			}

			//  STANDALONE GAME MODE
			t.game.gameisexe=1;
			t.game.onceonlyshadow=1;
			t.game.set.ismapeditormode=0;
			tgamesetismapeditormode = 0;

			// Allow _e_ usage
			SetCanUse_e_(1);
	
			//  do not need for loading page
			timestampactivity(0,"_game_hidemouse");
			game_hidemouse ( );
	
			//  Set FULLSCREEN MODE (or keep windowed mode)
			if (  g.gfullscreen == 1  )  SetWindowModeOff (  );
	
			//  set resolution for game
			timestampactivity(0,"set resolution for game");
			t.tdisplaymodeselected=0;
			if ( g.globals.riftmode>0 ) 
			{
				g.globals.riftmoderesult = 0;
				if ( g.globals.riftmoderesult == 0 ) 
				{
					//  Rift Display Mode
					g.gdisplaywidth=0;
					g.gdisplayheight=0;
					SetDisplayMode (  g.gdisplaywidth, g.gdisplayheight, 32, g.gvsync );
					t.tdisplaymodeselected=1;
				}
				else
				{
					sprintf ( t.szwork , "Rif Mode Result: %s" , Str(g.globals.riftmoderesult) );
					timestampactivity(0,t.szwork);
				}
			}
			timestampactivity(0,"set display mode");
			if ( t.tdisplaymodeselected == 0 ) 
			{
				// If rift not selected display mode, choose default
				if ( t.tnopathprotomode == 0 || g.gvrmode != 0 ) 
				{
					// detect -1,-1 in which case use GetDesktopWidth (  and height )
					if ( g.gdisplaywidth == -1 || g.gdisplayheight == -1 ) { g.gdisplaywidth = GetDesktopWidth() ; g.gdisplayheight = GetDesktopHeight(); }
					if ( g.gdisplaywidth != 640 || g.gdisplayheight != 480 || g.gdisplaydepth != 32 || g.gvrmode != 0 ) 
					{
						t.thevrmodeflag = 0; if ( g.gvrmode == 1 || g.gvrmode == 5 || g.gvrmode == 6 ) t.thevrmodeflag = 1;
						if ( t.thevrmodeflag != 0 )
						{
							SetDisplayModeVR ( g.gdisplaywidth, g.gdisplayheight, g.gdisplaydepth, g.gvsync,0,0,0,0,t.thevrmodeflag );
						}
						else
						{
							// Replaced the call with this since the previous one screws up
							t.bkwidth=GetDesktopWidth() ; t.bkheight=GetDesktopHeight();
							SetDisplayModeMODBACKBUFFER (  GetDesktopWidth(),GetDesktopHeight(),GetDisplayDepth(),g.gvsync,0,0,t.bkwidth,t.bkheight );
							g.gdisplaywidth=GetDesktopWidth();
							g.gdisplayheight=GetDesktopHeight();
						}
					}
				}
			}
		}

		//  option use use correct aspect ratio?
		if (  g.gaspectratio == 1 ) 
		{
			t.aspect_f=GetDesktopWidth() ; t.aspect_f=t.aspect_f/GetDesktopHeight();
			g.realaspect_f=t.aspect_f;
		}
	}

	//PE: IMGUI init.
	if (t.game.set.ismapeditormode == 1) 
	{
		// done earlier as standalone games need this too in wicked MAX engine
	}

	extern bool bImGuiInTestGame;
	if(t.game.gameisexe == 1)
		bImGuiInTestGame = true;

	t.newwidth=g.gdisplaywidth ; t.newheight=g.gdisplayheight ; t.newdepth=g.gdisplaydepth;
	g.gratiox_f = g.gdisplaywidth;
	g.gratioy_f = g.gdisplayheight;
	
	//  realaspect# also used in VISUALS.DBA
	timestampactivity(0,"set camera aspect");
	SetCameraAspect (  g.realaspect_f );

	//  special case, VRMODE=5 detects VR920 (switches OFF if not found)
	sprintf ( t.szwork , "Special VR-Mode Flag: %s" , Str(g.gvrmode));
	timestampactivity(0,t.szwork);
	if (  g.gvrmode == 5 ) 
	{
		t.vr920exist=ResetLeftEye();
		if (  t.vr920exist == 1 ) 
			g.gvrmode=4;
		else
			g.gvrmode=0;
	}

	//  FPGC - 130411 - special case, VRMODE=6 assumes new device
	g.vrsidebysidestereo = 0;
	if (  g.gvrmode == 6 ) 
	{
		g.vrsidebysidestereo=1;
		ResizeSprite ( 0, 0, 0.5 );
		g.gvrmode=4;
	}

	//  Set editor to use a true 1:1 pixel mapping
	timestampactivity(0,"pixel states");
	SetChildWindowTruePixel ( 1 );

	// 230517 - after display created, before any visual elements, load all core shaders
	SETUPLoadAllCoreShadersFIRST(g.gforceloadtestgameshaders);

	// loading sequence needs welcome asset art (even if welcome panel not on startup)
	if ( t.game.gameisexe == 0 )
	{
		timestampactivity(0, "welcome_init(1)");
		welcome_init(1);
		timestampactivity(0, "welcome_staticbackdrop();");
		//PE: This will resize backbuffer after a few call to sync so we dont get strange results later. from: SetChildWindowTruePixel ( 1 );
		welcome_staticbackdrop();
		timestampactivity(0, "welcome_init(2)");
		welcome_init(2);
		common_loadfonts();
		welcome_updatebackdrop("");
	}
	else
	{
		common_loadfonts();

		//PE: Need to reload lensflare images for standalone to support encryption.
		ReloadLensFlareImages();
	}

	//  reposition default radar,compass and objective based on new screen resolution
	g.radarx =(g.gdisplaywidth/100.0)*90;
	g.radary = (g.gdisplayheight/100.0)*90;
	g.compassX =(g.gdisplaywidth/100.0)*10;
	g.compassY = (g.gdisplayheight/100.0)*90;
	g.objectivex= g.compassX;
	g.objectivey= g.compassY;
	g.armx = (g.gdisplaywidth/100.0);
	g.army = (g.gdisplayheight/100.0);
	g.airx = (g.gdisplaywidth/100.0);
	g.airy = (g.gdisplayheight/100.0);

	//  Ensure default language chosen
	if (  g.language_s == ""  )  g.language_s = "english";

	//  Indicate a time stamp for app start time
	sprintf ( t.szwork , "Just about to read languagebank\\%s\\textfiles\\guru-wordcount.ini" , g.language_s.Get() );
	timestampactivity(0,t.szwork);

	// Translation Component (load strarr data)
	if ( t.tnopathprotomode == 0 ) 
	{
		// 250618 - this is the old translation system, capable of translating interface into many languages
		t.stdir_s=GetDir();
		sprintf ( t.szwork , "languagebank\\%s\\textfiles\\" , g.language_s.Get() );
		SetDir ( t.szwork );
		sprintf ( t.szwork , "Language File Path:%s (exist=%s)" , GetDir() , Str(FileExist("guru-wordcount.ini")) );
		timestampactivity(0,t.szwork);
		if (  FileExist("guru-wordcount.ini") == 1 ) 
		{
			OpenToRead (  1,"guru-wordcount.ini" );
			t.wordlibmax_s = ReadString (  1 ); g.wordlibmax=ValF(t.wordlibmax_s.Get());
			CloseFile (  1 );
			t.strarrmax=1;
			Dim (  t.strarr_s,g.wordlibmax  );
			Dim (  t.wordlib_s,g.wordlibmax  );
			LoadArray (  "guru-words.txt",t.wordlib_s );
			for ( t.n = 0 ; t.n<=  g.wordlibmax; t.n++ )
			{
				for ( t.c = 1 ; t.c<=  Len(t.wordlib_s[t.n].Get()); t.c++ )
				{
					if (  cstr(Mid(t.wordlib_s[t.n].Get(),t.c)) == "=" ) 
					{
						t.strarri=ValF(Left(t.wordlib_s[t.n].Get(),t.c-1));
						if (  t.strarri <= ArrayCount(t.strarr_s) ) 
						{
							t.strarr_s[t.strarri]=Right(t.wordlib_s[t.n].Get(),Len(t.wordlib_s[t.n].Get())-t.c);
							if (  t.strarri>t.strarrmax  )  t.strarrmax = t.strarri;
						}
						break;
					}
				}
			}
			UnDim (  t.wordlib_s );
		}
		else
		{
			//  language file missing
			ExitPrompt (  "FAIL","CANNOT FIND LANGUAGE FILE!" );
			ExitProcess ( 0 );
		}
		SetDir (  t.stdir_s.Get() );
	}

	// MapEditor or Standalone Game
	bool bIsThisMapEditor = false;
	sprintf ( t.szwork , "trueappname_s=%s" , g.trueappname_s.Get() );
	timestampactivity(0,t.szwork);
	if (  g.trueappname_s == "Guru-MapEditor" ) 
		bIsThisMapEditor = true;
	
	// Common redirections to new My System write/read folder
	cstr mysystemfolder_s = "My System";
	// using new DocWrite system
	g.mysystem.root_s = g.myownrootdir_s + "\\" + mysystemfolder_s + "\\";
	// using new DocWrite system
	g.mysystem.levelBankTestMap_s = "levelbank\\testmap\\";
	g.mysystem.levelBankTestMapAbs_s = g.fpscrootdir_s+"\\Files\\levelbank\\testmap\\";
	g.mysystem.editorsGridedit_s = "editors\\gridedit\\";
	g.mysystem.editorsGrideditAbs_s = g.fpscrootdir_s+"\\Files\\editors\\gridedit\\";
	g.mysystem.mapbank_s = "mapbank\\";
	g.mysystem.mapbankAbs_s = g.fpscrootdir_s+"\\Files\\mapbank\\";

	g.mysystem.thumbbank_s = g.fpscrootdir_s + "\\Files\\thumbbank\\";
	char pCacheFolder[MAX_PATH];
	strcpy(pCacheFolder, g.mysystem.thumbbank_s.Get());
	GG_SetWritablesToRoot(true);
	GG_GetRealPath(pCacheFolder, 1); //make sure it exists.
	GG_SetWritablesToRoot(false);
	g.mysystem.thumbbank_s = pCacheFolder;

	// also create particlebank\user folder for Particle Editor Exports
	cstr particlesbankfolder_s = g.fpscrootdir_s + "\\Files\\particlesbank\\user\\";
	strcpy(pCacheFolder, particlesbankfolder_s.Get());
	GG_GetRealPath(pCacheFolder, 1); //make sure it exists.

	cstr emitterbankfolder_s = g.fpscrootdir_s + "\\Files\\emitterbank\\";
	strcpy(pCacheFolder, emitterbankfolder_s.Get());
	GG_GetRealPath(pCacheFolder, 1); //make sure it exists.
	emitterbankfolder_s = g.fpscrootdir_s + "\\Files\\emitterbank\\user\\";
	strcpy(pCacheFolder, emitterbankfolder_s.Get());
	GG_GetRealPath(pCacheFolder, 1); //make sure it exists.


	// also create
	cstr scriptbankfolder_s = g.fpscrootdir_s + "\\Files\\scriptbank\\user\\";
	strcpy(pCacheFolder, scriptbankfolder_s.Get());
	GG_GetRealPath(pCacheFolder, 1); //make sure it exists.

	//  LEAP POINT (detect if running as Guru-Game.exe or Guru-MapEditor.exe)
	if ( bIsThisMapEditor == true ) 
	{
		// MAP EDITOR

		// wicked uses DocWrite system
		char pNewLevelBankFolder[MAX_PATH];
		strcpy(pNewLevelBankFolder, g.mysystem.levelBankTestMapAbs_s.Get());
		GG_GetRealPath(pNewLevelBankFolder, 1);
		g.mysystem.levelBankTestMap_s = pNewLevelBankFolder;
		g.mysystem.levelBankTestMapAbs_s = pNewLevelBankFolder;

		// create a mapbank folder, and copy all read-only ones to writables (if level does not exist)
		bool bCopyMapBankToWritableArea = true;
		if (bCopyMapBankToWritableArea == true)
		{
			// store current dir
			char pOldDir[MAX_PATH];
			GetCurrentDirectoryA(MAX_PATH, pOldDir);

			// set new absolute path
			char pNewMapBankFolder[MAX_PATH];
			strcpy(pNewMapBankFolder, g.mysystem.mapbankAbs_s.Get());
			GG_GetRealPath(pNewMapBankFolder, 1);

			// copy any mapbank levels over to new write folder (so mapbank default location is the writable area)
			// Gather ALL files\mapbank in original Files folder
			Undim(t.filecollection_s);
			g.filecollectionmax = 0;
			Dim(t.filecollection_s, 500);

			// initial folders in Files
			SetDir(g.fpscrootdir_s.Get());
			SetDir("Files");
			addallinfoldertocollection("mapbank", "mapbank");

			// Copy to System folder if file not exist (leave any existing files alone)
			SetDir(pNewMapBankFolder);
			createallfoldersincollection();
			for (int f = 1; f <= g.filecollectionmax; f++)
			{
				LPSTR pDestFile = t.filecollection_s[f].Get();
				char pStripLeadingSlash[MAX_PATH];
				if ( pDestFile[0] == '\\' )
					strcpy(pStripLeadingSlash, pDestFile + 1);
				else
					strcpy(pStripLeadingSlash, pDestFile);
				cstr pSrcFile = g.fpscrootdir_s + "\\Files\\mapbank\\" + pStripLeadingSlash;
				//PE: FALSE do not work here ?
				if (!FileExist(pStripLeadingSlash)) {
					CopyFileA(pSrcFile.Get(), pStripLeadingSlash, FALSE); // FALSE = do not copy if already exists (protecting changes from being overwritten)
				}
			}

			// set new absolute path
			g.mysystem.mapbankAbs_s = pNewMapBankFolder;

			// restore directory
			SetDir(pOldDir);
		}

		// create itinerary file if first time, or just read it in
		CreateItineraryFile();

		// Write latest location of software to registry (for future patch installers)
		HKEY hKeyNames = 0;
		LPCSTR pSubKeyName = "Software\\GameGuruMAX";
		LPSTR pThisVersion = g.version_s.Get();
		LPSTR pThisPath = g.fpscrootdir_s.Get();
		DWORD dwDisposition;
		DWORD Status = RegCreateKeyExA(HKEY_CURRENT_USER, pSubKeyName, 0L, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS | KEY_WRITE, NULL, &hKeyNames, &dwDisposition);
		if ( Status == ERROR_SUCCESS )
		{
			if ( dwDisposition == REG_OPENED_EXISTING_KEY )
			{
				RegCloseKey ( hKeyNames );
				Status = RegOpenKeyExA(HKEY_CURRENT_USER, pSubKeyName, 0L, KEY_WRITE, &hKeyNames);
			}
		}
		if ( hKeyNames != 0 )
		{
			if ( Status == ERROR_SUCCESS )
			{
				Status = RegSetValueExA(hKeyNames, "Version", 0, REG_SZ, (LPBYTE)pThisVersion, (strlen(pThisVersion)+1)*sizeof(char));
				Status = RegSetValueExA(hKeyNames, "LatestInstallPath", 0, REG_SZ, (LPBYTE)pThisPath, (strlen(pThisPath)+1)*sizeof(char));
			}
			RegCloseKey(hKeyNames);
		}

		// using new DocWrite system
		char pRealRecompile[MAX_PATH];
		strcpy(pRealRecompile, g.fpscrootdir_s.Get());
		strcat(pRealRecompile, "\\ggprecompile.lua");
		GG_GetRealPath(pRealRecompile, 1);
		cstr ggprecompilefile_s = g.fpscrootdir_s + "\\ggprecompile.lua";
		CopyFileA ( ggprecompilefile_s.Get(), pRealRecompile, TRUE );

		// Now display version number and resumer prompt
		LPSTR pFirstTextToShow = g.version_s.Get();
		if ( g.grestoreeditorsettings == 1 ) pFirstTextToShow = "RESUMING PREVIOUS SESSION";
		welcome_updatebackdrop(pFirstTextToShow);

		// Init MP System (and Activate Steam (always so single player can do snapshots and get Steam notifications) if Steam)
		mp_init ( );

		// Init default material sounds
		material_init ( );
		material_startup ( );

		// Write test!
		OpenToWrite ( 1,"testwrite.dat" );
		WriteString ( 1,"we can write to the Game Guru folder! Good." );
		CloseFile ( 1 );
		if ( FileExist("testwrite.dat") == 1 ) 
		{
			timestampactivity(0,"We can write to the Game Guru folder!");
			DeleteAFile ( "testwrite.dat" );
		}
		else
		{
			ExitPrompt ( "Game Guru cannot write files to the Files area. Exit the software, right click on the Game Guru icon, and select 'Run As Administrator'", "Init Error" );
		}

		// at very start for wicked, create user folders so dont need to exit and return to see any creations
		char pUserFolder[MAX_PATH];
		// entitybank writables
		strcpy(pUserFolder, g.fpscrootdir_s.Get());
		strcat(pUserFolder, "\\Files\\entitybank\\user\\");
		GG_GetRealPath(pUserFolder,1);

		strcpy(pUserFolder, g.fpscrootdir_s.Get());
		strcat(pUserFolder, "\\Files\\imagebank\\user\\");
		GG_GetRealPath(pUserFolder, 1);

		strcpy(pUserFolder, g.fpscrootdir_s.Get());
		strcat(pUserFolder, "\\Files\\entitybank\\user\\charactercreatorplus\\");
		GG_GetRealPath(pUserFolder,1);

		strcpy(pUserFolder, g.fpscrootdir_s.Get());
		strcat(pUserFolder, "\\Files\\entitybank\\user\\ebestructures\\");
		GG_GetRealPath(pUserFolder,1);

		// and audiobank writables
		strcpy(pUserFolder, g.fpscrootdir_s.Get());
		strcat(pUserFolder, "\\Files\\audiobank\\recordings\\");
		GG_GetRealPath(pUserFolder,1);

		//PE: Import activated.
		strcpy(pUserFolder, g.fpscrootdir_s.Get());
		strcat(pUserFolder, "\\Files\\audiobank\\user\\");
		GG_GetRealPath(pUserFolder, 1);

		//PE: Video import activated.
		strcpy(pUserFolder, g.fpscrootdir_s.Get());
		strcat(pUserFolder, "\\Files\\videobank\\user\\");
		GG_GetRealPath(pUserFolder, 1);
	
		//  New security requires Steam client to be running (for ownership check)
		g.iFreeVersionModeActive = 0;
		#ifdef STEAMOWNERSHIPCHECKFREEWEEKEND
		 g.iFreeVersionModeActive = 1;
		 bool bSteamRunningAndGameGuruOwned = false;
		 if ( g.mp.isRunning == 1 )
		 {
			if ( SteamOwned() == true ) 
				bSteamRunningAndGameGuruOwned = true;
		 }
		 if ( bSteamRunningAndGameGuruOwned == false )
		 {
			g.iTriggerSoftwareToQuit = 2;
		 }
		#endif
		#ifdef FREETRIALVERSION
		 // free trial version mode
		 g.iFreeVersionModeActive = 2;
		 // discount code strings
		 strcpy ( g_trialDiscountCode, "" );
		 strcpy ( g_trialDiscountExpires, "" );
		 // countdown to trial ending
		 time_t now = time(0);
		 tm *ltm = localtime(&now);
		 int iDay   = ltm->tm_mday;
		 int iMonth = ltm->tm_mon;
		 int iYear  = ltm->tm_year-100;
		 // work out single value to represent days
		 int iTotalDays = (iYear*365)+(iMonth*31)+iDay;
		 // handle time stamp file
		 LPSTR pTrialStampFile = "..\\trialstamp.txt";
		 if ( FileExist ( pTrialStampFile ) == 0 )
		 {
			// get 7-day discount code
			DWORD dwDataReturnedSize = 0;
			char pDataReturned[10240];
			memset ( pDataReturned, 0, sizeof(pDataReturned) );

			char pGetDataString[1024];
			strcpy ( pGetDataString, "/api/discount/codes/generate?" );
			strcat ( pGetDataString, DISCOUNTKEY );
			strcat ( pGetDataString, "&discount=gamegurutrial" );
			UINT iError = GetURLData ( pDataReturned, &dwDataReturnedSize, pGetDataString );
			if ( iError <= 0 && *pDataReturned != 0 && strchr(pDataReturned, '{') != 0 && dwDataReturnedSize < 10240 )
			{
				// break up response string
				// {
				// "success": true,
				// "discount_code": "GGTRIAL507CD20E3B2",
				// "expires": "2019-08-13 10:17:40"
				// }
				char pCodeText[10240];
				strcpy ( pCodeText, "" );
				char pExpiryText[10240];
				strcpy ( pExpiryText, "" );

				// work through data returned
				char pWorkStr[10240];
				strcpy ( pWorkStr, pDataReturned );
				if ( pWorkStr[0]=='{' ) strcpy ( pWorkStr, pWorkStr+1 );
				int n = 10200;
				for (; n>0; n-- ) if ( pWorkStr[n] == '}' ) { pWorkStr[n] = 0; break; }
				char* pChop = strstr ( pWorkStr, "," );
				char pStatusStr[10240];
				strcpy ( pStatusStr, pWorkStr );
				if ( pChop ) pStatusStr[pChop-pWorkStr] = 0;
				if ( pChop[0]==',' ) pChop += 1;
				if ( strstr ( pStatusStr, "success" ) != NULL )
				{
					// success
					// code
					pChop = strstr ( pChop, ":" ) + 2;
					strcpy ( pCodeText, pChop );
					char pEndOfChunk[4];
					pEndOfChunk[0]='"';
					pEndOfChunk[1]=',';
					pEndOfChunk[2]='"';
					pEndOfChunk[3]=0;
					char* pCodeTextEnd = strstr ( pCodeText, pEndOfChunk );
					pCodeText[pCodeTextEnd-pCodeText] = 0;
					pChop += strlen(pCodeText);

					// expiry
					pChop = strstr ( pChop, ":" ) + 2;
					strcpy ( pExpiryText, pChop );
					LPSTR pFindSpaceBetweenDateAndTime = strstr ( pExpiryText, " " );
					if ( pFindSpaceBetweenDateAndTime ) *pFindSpaceBetweenDateAndTime = 0;

					// copy to globals
					strcpy ( g_trialDiscountCode, pCodeText );
					strcpy ( g_trialDiscountExpires, pExpiryText );

					// only when get code can trial countdown start
					// create time stamp
					OpenToWrite ( 1, pTrialStampFile );
					WriteLong ( 1, iTotalDays );
					WriteString ( 1, g_trialDiscountCode );
					WriteString ( 1, g_trialDiscountExpires );
					CloseFile ( 1 );
				}
				else
				{
					// error
					char* pMessageValue = strstr ( pChop, ":" ) + 1;
				}
			}

			// no code, no trial start!
			if ( strcmp ( g_trialDiscountCode, "" ) == NULL )
			{
				strcpy ( g_trialDiscountCode, "No Discount" );
				strcpy ( g_trialDiscountExpires, "Unable To Get Code" );
			}

			// starts at 7 days
			g_trialStampDaysLeft = 7;
		 }
		 else
		 {
			OpenToRead ( 1, pTrialStampFile );
			int iDateTrialFirstUsed = ReadLong ( 1 );
			LPSTR pCode = ReadString ( 1 );
			LPSTR pExpiry = ReadString ( 1 );
			CloseFile ( 1 );
			strcpy ( g_trialDiscountCode, pCode );
			strcpy ( g_trialDiscountExpires, pExpiry );
			g_trialStampDaysLeft = 7-(iTotalDays-iDateTrialFirstUsed);
		 }
		 if ( g_trialStampDaysLeft <= 0 )
		 {
			g.iTriggerSoftwareToQuit = 2;
		 }
		#endif

		// 100718 - generate all new .BLOB files (used when making builds)
		if ( g.gforceloadtestgameshaders == 2 )
		{
			// scan effectsbank folder
			cstr pOldDir = GetDir();
			SetDir("effectbank\\reloaded");
			ChecklistForFiles();
			SetDir(pOldDir.Get());
			cstr ShaderPath = cstr("effectbank\\reloaded\\");
			for ( int c = 1; c < ChecklistQuantity(); c++ )
			{
				cstr file_s = ChecklistString(c);
				LPSTR pFilename = file_s.Get();
				if ( Len ( pFilename ) > 3 )
				{
					if ( strnicmp ( pFilename + strlen(pFilename) - 3, ".fx", 3 ) == NULL )
					{
						// some core shaders are except, but compile the rest
						bool bExempt = false;
						if ( stricmp ( pFilename, "apbr_core.fx" ) == NULL ) bExempt = true;
						if ( stricmp ( pFilename, "cascadeshadows.fx" ) == NULL ) bExempt = true;
						if ( bExempt == false )
						{
							// show which shader is being compiled into a blob
							t.tsplashstatusprogress_s = pFilename;
							timestampactivity(0,t.tsplashstatusprogress_s.Get());
							version_splashtext_statusupdate ( );

							// load shader to create blob file
							cstr ShaderFX_s = ShaderPath + file_s;
							char pShaderBLOB[2048];
							strcpy ( pShaderBLOB, file_s.Get() );
							pShaderBLOB[strlen(pShaderBLOB)-3]=0;
							strcat ( pShaderBLOB, ".blob" );
							cstr ShaderBLOB_s = ShaderPath + pShaderBLOB;
							SETUPLoadShader ( ShaderFX_s.Get(), ShaderBLOB_s.Get(), 0 );
						}
					}
				}
			}
			t.tsplashstatusprogress_s = "Finished compiling";
			timestampactivity(0,t.tsplashstatusprogress_s.Get());
			version_splashtext_statusupdate ( );
			g.iTriggerSoftwareToQuit = 3;
			g.gforceloadtestgameshaders = 0;
		}

		//  Enter Map Editor specific code
		SETUPLoadAllCoreShadersREST(g.gforceloadtestgameshaders,g.gpbroverride);
		material_loadsounds ( 1 );

		// finally launch editor 
		// wicked has own loop, do not use regular internal loop! (see GuruLoop)
	}
	else
	{
		//  ACTUAL GAME EXE

		//  Debug report status
		timestampactivity(0, "main game executable");

		//  Activate Steam (always so single player can do snapshots and get Steam notifications)
		mp_init();

		// VR Mode Initialisation
		g.globals.riftmode = 0;
		g.vrglobals.GGVREnabled = 0;
		g.vrglobals.GGVRUsingVRSystem = 1;
		if (bSpecialStandalone == false)
		{
			if (g.gvrmode == 2) g.vrglobals.GGVREnabled = 1; // OpenVR (Steam)
			if (g.gvrmode == 3) g.vrglobals.GGVREnabled = 2; // Windows Mixed Reality (Microsoft)
		}
		char pVRSystemString[1024];
		sprintf(pVRSystemString, "choose VR system with mode %d", g.vrglobals.GGVREnabled);
		timestampactivity(0, pVRSystemString);
		int iErrorCode = GGVR_ChooseVRSystem(g.vrglobals.GGVREnabled, g.gproducelogfiles, "");// cstr(g.fpscrootdir_s + "\\GGWMR.dll").Get());
		if (iErrorCode > 0)
		{
			char pErrorStr[1024];
			sprintf(pErrorStr, "Error Choosing VR System : Code %d", iErrorCode);
			timestampactivity(0, pErrorStr);
			timestampactivity(0, "switching VR off, headset not detected");
			g.vrglobals.GGVREnabled = 0;
		}
		else
		{
			// Give portal enough time to start its launch, then get rid of GameWindow until we need it!
			Sleep(1900);
			CloseWindow(g_pGlob->hOriginalhWnd);
			Sleep(100);
			g_pGlob->hOriginalhWnd = NULL;
			SetWindowPos(g_pGlob->hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		}

		// Need editor 14.PNG for teleport graphic
		LoadImage("editors\\gfx\\14-white.png", g.editorimagesoffset + 14);
		if (!GetImageExistEx(g.editorimagesoffset + 14)) LoadImage("editors\\gfx\\14.png", g.editorimagesoffset + 14);
		LoadImage("editors\\gfx\\14-red.png", g.editorimagesoffset + 16);
		LoadImage("editors\\gfx\\14-green.png", g.editorimagesoffset + 17);

		// Init default material sounds
		material_init();
		material_startup();

		//  Set device to get multisampling AA active in editor
		t.multisamplingfactor = 0; t.multimonitormode = 0;

		//  Init app
		SyncOn();
		SyncRate(0);
		SetAutoCamOff();
		AlwaysActiveOff();

		//  Camera aspect ratio adjustment for desktop resolution
		t.aspect_f = GetDesktopWidth(); t.aspect_f = t.aspect_f / GetDesktopHeight();
		SetCameraAspect(t.aspect_f);

		//  set-up test game screen prompt assets (for printscreenprompt())
		int iUseVRTest = 0;
		if (g.vrglobals.GGVREnabled > 0) iUseVRTest = 1;
		loadscreenpromptassets(iUseVRTest);
		printscreenprompt("");

		// delayed material load to after logo splash
		SETUPLoadAllCoreShadersREST(g.gforceloadtestgameshaders, g.gpbroverride);

		//  Generic asset loading common to editor and game
		common_loadfonts();
		common_loadcommonassets(1);
		common_loadcommonassets_delayed(1);
		extern bool g_bCommonAssetsLoadOnce;
		g_bCommonAssetsLoadOnce = false;

		// This used by 3D prompts in standalone
		g.guishadereffectindex = loadinternaleffect("effectbank\\reloaded\\gui_basic.fx");

		//  Load terrain from terrain temp save file
		terrain_createactualterrain();
		t.screenprompt_s = t.screenprompt_s + ".";
		printscreenprompt(t.screenprompt_s.Get());
		timestampactivity(0, "_terrain_load");
		t.tfile_s = g.mysystem.levelBankTestMap_s + "m.dat"; //"levelbank\\testmap\\m.dat";
		if (FileExist(t.tfile_s.Get()) == 1)
		{
			terrain_load(t.tfile_s.Get());
		}

		//  Call visuals loop once to set shader constants
		t.visuals = t.gamevisuals;
		t.visuals.refreshshaders = 1;
		visuals_loop();

		// may not be needed if this is called deeper in!
		void gun_gatherslotorder_load (void);
		gun_gatherslotorder_load();

		//  Main loop
		timestampactivity(0, "Main Game Executable Loop Starts");

		// seems without this, HUD in VR could never Grab backbuffer image and show in screenHUD object
		extern bool bImGuiInitDone;
		bImGuiInitDone = true;

		//  One-off variable settings
		t.game.set.resolution = 0;
		t.game.set.initialsplashscreen = 0;

		//  ensure no collision from DBP!
		AutomaticCameraCollision(0, 0, 0);
		SetGlobalCollisionOff();

		//  Setup game view camera?
		SetCameraFOV(75);
		SetCameraRange(1, 4000);
		g.grav_f = -5.0;

		// temporarily hide main screen (post process will show it when ready)
		SetCameraView(0, 0, 0, 1, 1);

		// full speed
		SyncRate(0);

		//
		//  Launch game in EXE mode
		//
		// allowed to leave, so gamexecutable_init, loop and finish can handle replacing the old internal loop
	}
}

void common_justbeforeend ( void )
{
	// clear TXP caches before exit
	if ( g.gdeletetxpcachesonexit == 1 )
	{
		// scan "Files\ebebank\default" and delete any textures associated with present "TXP" files
		if (PathExist("ebebank\\default") == 1)
		{
			t.tolddir_s = GetDir();
			SetDir ("ebebank\\default");
			ChecklistForFiles ();
			t.strwork = ""; t.strwork = t.strwork + "Clearing " + Str(ChecklistQuantity()) + " TXP cache files";
			timestampactivity(0, t.strwork.Get());
			for (t.c = 1; t.c <= ChecklistQuantity(); t.c++)
			{
				t.tfile_s = ChecklistString(t.c);
				if (t.tfile_s != "." && t.tfile_s != "..")
				{
					if (stricmp (Right (t.tfile_s.Get(), 4), ".txp") == NULL && stricmp (t.tfile_s.Get(), "textures_profile.txp") != NULL && stricmp (t.tfile_s.Get(), "original_profile.txp") != NULL)
					{
						// delete TXP file
						DeleteAFile (t.tfile_s.Get());

						// also delete associated textures belonging to this TXP file
						cStr tfilettex_s = cstr(Left (t.tfile_s.Get(), strlen(t.tfile_s.Get()) - 4)) + cstr("_color.dds");
						DeleteAFile (tfilettex_s.Get());
						tfilettex_s = cstr(Left (t.tfile_s.Get(), strlen(t.tfile_s.Get()) - 4)) + cstr("_normal.dds");
						DeleteAFile (tfilettex_s.Get());
						tfilettex_s = cstr(Left (t.tfile_s.Get(), strlen(t.tfile_s.Get()) - 4)) + cstr("_surface.dds");
						DeleteAFile (tfilettex_s.Get());
					}
				}
			}
			timestampactivity(0, "Clearing complete.");
			SetDir (t.tolddir_s.Get());
		}
	}

	// save number of minutes user been in session (added to global recorded when we entered)
}

void common_loadfonts ( void )
{
	//  Bitmap Font Image Start
	t.tsplashstatusprogress_s="LOADING FONTS";
	timestampactivity(0,t.tsplashstatusprogress_s.Get());
	loadallfonts();
}

void common_loadcommonassets(int iShowScreenPrompts)
{
	//PE: Per country text in standalone.
	t.screenprompt_s = "PREPARING CORE FILES";
	sprintf(t.szwork, "languagebank\\%s\\inittext.ssp", g.language_s.Get());
	auto fp = fopen(t.szwork, "r");
	if (fp)
	{
		char ctmp[512];
		fgets(ctmp, 512, fp);
		fclose(fp);
		if (strlen(ctmp) > 0)
			t.screenprompt_s = ctmp;
	}

	//  Bitmap Font Image Start
	t.tsplashstatusprogress_s = "LOADING FONTS";
	timestampactivity(0, t.tsplashstatusprogress_s.Get());
	version_splashtext_statusupdate();
	timestampactivity(0, "initbitmapfont");
	loadallfonts();

	//  Setup visual settings
	t.tsplashstatusprogress_s = "INIT VECTORS";
	timestampactivity(0, t.tsplashstatusprogress_s.Get());
	version_splashtext_statusupdate();
	common_vectorsinit();

	// rest loaded via common_loadcommonassets_delayed
	return;
}

void common_loadcommonassets_delayed(int iShowScreenPrompts)
{
	t.tsplashstatusprogress_s = "INIT TERRAIN ASSETS";
	timestampactivity(0, t.tsplashstatusprogress_s.Get());
	version_splashtext_statusupdate();
	terrain_initstyles();

	t.tsplashstatusprogress_s = "INIT GAME VISUAL ASSETS";
	timestampactivity(0, t.tsplashstatusprogress_s.Get());
	version_splashtext_statusupdate();
	visuals_init();

	t.tsplashstatusprogress_s = "INIT DECAL ASSETS";
	timestampactivity(0, t.tsplashstatusprogress_s.Get());
	version_splashtext_statusupdate();
	decal_init();

	//  Setup default paths
	t.levelmapptah_s = g.mysystem.levelBankTestMap_s;
	g.projectfilename_s = "";

	//  Get list of guns and flak for data
	t.screenprompt_s = t.screenprompt_s + ".";
	if (iShowScreenPrompts == 1) printscreenprompt(t.screenprompt_s.Get());
	t.tsplashstatusprogress_s = "SCANNING G-LIST";
	timestampactivity(0, t.tsplashstatusprogress_s.Get());
	version_splashtext_statusupdate();
	gun_scaninall_ref();
	gun_scaninall_dataonly();

	//  Load in default player sounds (default style) , PE: 0.5 sec so moved here while loading in background.
	t.tsplashstatusprogress_s = "LOAD PLAYER SOUNDS";
	timestampactivity(0, t.tsplashstatusprogress_s.Get());
	version_splashtext_statusupdate();
	g.gplayerstyle_s = ""; material_loadplayersounds();

	//  Create terrain (eventually default terrain randomised)
	t.screenprompt_s = t.screenprompt_s + ".";
	if (iShowScreenPrompts == 1) printscreenprompt(t.screenprompt_s.Get());
	t.tsplashstatusprogress_s = "CREATING TERRAIN";
	timestampactivity(0, t.tsplashstatusprogress_s.Get());
	version_splashtext_statusupdate();
	terrain_setupedit();

	if (t.game.gameisexe == 0)
	{
		t.terrain.terrainobjectindex = t.terrain.objectstartindex + 3;
		t.terrain.waterliney_f = g.gdefaultwaterheight;
		if (GetEffectExist(t.terrain.effectstartindex + 0) == 0)
		{
			LPSTR pEffectToUse = "effectbank\\reloaded\\terrain_basic.fx";
			if (g.gpbroverride == 1) pEffectToUse = "effectbank\\reloaded\\apbr_terrain.fx";
			LoadEffect(pEffectToUse, t.terrain.effectstartindex + 0, 0);
			timestampactivity(0, cstr(cstr("Terrain Shader:") + pEffectToUse).Get());
		}
		terrain_assignnewshader();
		SetEffectShadowMappingMode(255);
		SetEffectToShadowMappingEx(t.terrain.terrainshaderindex, g.shadowdebugobjectoffset, g.guidepthshadereffectindex, g.globals.hidedistantshadows, 0, g.globals.realshadowresolution, g.globals.realshadowcascadecount, g.globals.realshadowcascade[0], g.globals.realshadowcascade[1], g.globals.realshadowcascade[2], g.globals.realshadowcascade[3], g.globals.realshadowcascade[4], g.globals.realshadowcascade[5], g.globals.realshadowcascade[6], g.globals.realshadowcascade[7]);
		if (t.game.runasmultiplayer == 1) mp_refresh();
		t.terrain.WaterCamY_f = 0.0;
		t.terrain.waterliney_f = g.gdefaultwaterheight;

		//Create blank terrain here, while we also load in the background. this can take 2 sec.
		t.inputsys.donewflat = 1;
		t.inputsys.donew == 1; //?
		gridedit_new_map_quick();
		t.inputsys.donewflat = 0;
		t.inputsys.donew = 0;

		terrain_make_image_only();
		if (GetImageExistEx(t.terrain.imagestartindex + 13) && GetImageExistEx(t.terrain.imagestartindex + 21))
		{
			TextureObject(t.terrain.terrainobjectindex, 2, t.terrain.imagestartindex + 13);
			TextureObject(t.terrain.terrainobjectindex, 4, t.terrain.imagestartindex + 21);
		}
		else
		{
			terrain_changestyle();
		}
		g.vegstyleindex = t.visuals.vegetationindex;
	}
	else
	{
		terrain_make();
	}

	// Sky details for terrain lighting
	if (t.game.gameisexe == 0)
	{
		t.screenprompt_s = t.screenprompt_s + ".";
		if (iShowScreenPrompts == 1) printscreenprompt(t.screenprompt_s.Get());
		t.tsplashstatusprogress_s = "SCANNING SKY SETTINGS";
		timestampactivity(0, t.tsplashstatusprogress_s.Get());
		version_splashtext_statusupdate();
		sky_skyspec_init();
	}

	//  Create post process shader and apply
	t.tsplashstatusprogress_s="INIT POST PROCESSING";
	timestampactivity(0,t.tsplashstatusprogress_s.Get());
	version_splashtext_statusupdate ( );
	postprocess_general_init ( );

	//  Initialise ragdoll resources
	t.tsplashstatusprogress_s="INIT RAGDOLL SYSTEM";
	timestampactivity(0,t.tsplashstatusprogress_s.Get());
	version_splashtext_statusupdate ( );
	ragdoll_init ( );

	//  temporary call to load projectiles (RPG)
	t.tsplashstatusprogress_s="CREATING P-LIST";
	timestampactivity(0,t.tsplashstatusprogress_s.Get());
	version_splashtext_statusupdate ( );
	weapon_projectile_init ( );

	//  explosions and fire
	t.tsplashstatusprogress_s="CREATING E-LIST";
	timestampactivity(0,t.tsplashstatusprogress_s.Get());
	version_splashtext_statusupdate ( );
	explosion_init ( );

	//  Load all particle animation images
	t.tsplashstatusprogress_s="LOADING INTO P-LIST";
	timestampactivity(0,t.tsplashstatusprogress_s.Get());
	version_splashtext_statusupdate ( );
	ravey_particles_load_images ( );

	// load any resources that could be different if sitting inside a remote project rather than stock assets
	common_mustreload_foreachnewproject();
}

void common_mustreload_foreachnewproject(void)
{
	// this is required as remote projects may have their own completely different 'stock' resources
 
	// also refresh SKY LIST as custom skies might exist there
	t.tsplashstatusprogress_s = "REFRESH SKY ASSETS";
	timestampactivity(0, t.tsplashstatusprogress_s.Get());
	version_splashtext_statusupdate();
	sky_init();

	// Load all blood splat resources from databank
	t.tsplashstatusprogress_s = "REFRESH B-LIST ASSETS";
	timestampactivity(0, t.tsplashstatusprogress_s.Get());
	version_splashtext_statusupdate ();
	blood_damage_init ();

	// key bindings can change between local and remote project (each remote can have their own custom set)
	FPSC_LoadKEYMAP();
}

void common_hide_mouse ( void )
{
	OpenFileMap (  1, "FPSEXCHANGE" );
	SetEventAndWait (  1 );
	SetFileMapDWORD (  1, 44, 0 );
	SetFileMapDWORD (  1, 48, 2 );
	SetFileMapDWORD (  1, 52, GetChildWindowWidth()/2 );
	SetFileMapDWORD (  1, 56, GetChildWindowHeight()/2 );
	SetFileMapDWORD (  1, 704,GetChildWindowWidth() );
	SetFileMapDWORD (  1, 708,GetChildWindowHeight() );
}

void common_show_mouse ( void )
{
	// This does crazy cool stuff
	OpenFileMap (  1, "FPSEXCHANGE" );
	SetEventAndWait (  1 );
	SetFileMapDWORD (  1, 44, 1 );
	SetFileMapDWORD (  1, 48, 0 );
	SetFileMapDWORD (  1, 52, GetChildWindowWidth()/2 );
	SetFileMapDWORD (  1, 56, GetChildWindowHeight()/2 );
	SetFileMapDWORD (  1, 704,GetChildWindowWidth() );
	SetFileMapDWORD (  1, 708,GetChildWindowHeight() );
}

void common_vectorsinit ( void )
{
	//  One-off creation of vectors
	t.r=MakeMatrix(g.m4_view);
	t.r=MakeMatrix(g.m4_projection);
	t.r=MakeMatrix(g.m4_viewproj);
	t.r=MakeVector4(g.v4_far);
	t.r=MakeVector3(g.v3_far);
	t.r=MakeVector4(g.universalvectorindex);
	t.r=MakeVector4(g.terrainvectorindex);
	t.r=MakeVector4(g.terrainvectorindex1);
	t.r=MakeVector4(g.terrainvectorindex2);
	t.r=MakeVector4(g.terrainvectorindex3);
	t.r=MakeVector4(g.vegetationvectorindex);
	t.r=MakeVector4(g.weaponvectorindex);
	t.r=MakeVector4(g.generalvectorindex+1); // ScaleIn;
	t.r=MakeVector4(g.generalvectorindex+2); // Scale;
	t.r=MakeVector4(g.generalvectorindex+3); // HmdWarpParam;
	t.r=MakeVector4(g.generalvectorindex+4); // ScreenCenter;
	t.r=MakeVector4(g.generalvectorindex+5); // Len ( sCenter );
	t.r=MakeVector4(g.postprocesseffectoffset+1);
}

void common_wipeeffectifnotexist ( void )
{
	//  intercept classic shaders and redirect
	if ( FileExist(t.tfile_s.Get()) == 1 ) 
	{
		t.tokay=0;
		if (  cstr(Lower(t.tfile_s.Get())) == "effectbank\\ps_2_0\\bump.fx"  )  t.tokay = 1;
		if (  cstr(Lower(t.tfile_s.Get())) == "effectbank\\ps_2_0\\bumpent.fx"  )  t.tokay = 1;
		if (  cstr(Lower(t.tfile_s.Get())) == "effectbank\\ps_2_0\\illuminationent.fx"  )  t.tokay = 1;
		if (  cstr(Lower(t.tfile_s.Get())) == "effectbank\\ps_2_0\\bumpbone.fx"  )  t.tokay = 2;
		if (  cstr(Lower(t.tfile_s.Get())) == "effectbank\\ps_2_0\\bumpbonerev.fx"  )  t.tokay = 2;
		if (  cstr(Lower(t.tfile_s.Get())) == "effectbank\\ps_2_0\\fastbone.fx"  )  t.tokay = 2;
		if (  t.tokay == 1  )  t.tfile_s = "effectbank\\reloaded\\entity_basic.fx";
		if (  t.tokay == 2  )  t.tfile_s = "effectbank\\reloaded\\character_basic.fx";
	}

	// if file not exist, use universal
	if (  FileExist(t.tfile_s.Get()) == 0 ) 
	{
		// use reloaded universal shader
		t.tfile_s="effectbank\\reloaded\\entity_basic.fx";
		if (  FileExist(t.tfile_s.Get()) == 0 ) 
		{
			// something went VERY wrong
			t.tfile_s="";
		}
	}
}

void common_makeeffecttextureset ( void )
{
	//  used by entitycore and segmentmaker (takes tfile$ and texdir$)
	common_wipeeffectifnotexist ( );

	//  reset output
	t.absolutelyrequirethistexture=0;
	t.ensureclearlayermax=0;
	t.texdir1_s="";
	t.texdir2_s="";
	t.texdir3_s="";
	t.texdir4_s="";
	t.texdir5_s="";

	//  V117 - 110110 - new full shader style means D2 preferred over D (thanks to Mark Blosser PS2 shaders)
	t.texturingtoken_s="D";
	if (  g.gnewblossershaders == 1 ) 
	{
		//  Abolishes use of unshaded D textures
		t.texturingtoken_s="D2";
	}

	//  determine effect textures to use
	if (  t.tfile_s.Get() != "" && t.segnoeffects == 0 ) 
	{

		//  D2=D+I+N+S
		t.tstr0_s=Left(t.texdir_s.Get(),Len(t.texdir_s.Get())-4);
		t.tstr1_s=Left(t.texdir_s.Get(),Len(t.texdir_s.Get())-6);
		t.tstr2_s=Right(t.texdir_s.Get(),4);
		if (  Len(t.tstr2_s.Get()) <= 1  )  t.tstr2_s = ".dds";

		//  D2 must be present to indicate existence of D, N, S, I, etc
		if (  cstr(Upper(Right(t.tstr0_s.Get(),2))) == "D2" ) 
		{
			//  The system used by FPSC is D.I.N.S for 0,1,2,3
			t.hardcodedtexturestages=0;

			//  related to scene universe (lightmap stage present)
			t.ttt_s = "illuminationmap.fx" ; if (  t.ttt_s  == Lower(Right(t.tfile_s.Get(),Len(t.ttt_s.Get()))) )  t.hardcodedtexturestages = 1;
			t.ttt_s = "bump.fx" ; if (  t.ttt_s == Lower(Right(t.tfile_s.Get(),Len(t.ttt_s.Get())))  )  t.hardcodedtexturestages = 2;

			//  related to entities (lightmap stage NOT present)
			t.ttt_s = "illuminationent.fx" ; if (   t.ttt_s == Lower(Right(t.tfile_s.Get(),Len(t.ttt_s.Get())))  )  t.hardcodedtexturestages = 11;
			t.ttt_s = "bumpent.fx" ; if (  t.ttt_s == Lower(Right(t.tfile_s.Get(),Len(t.ttt_s.Get()))) )  t.hardcodedtexturestages = 12;
			t.ttt_s = "bumpbone.fx" ; if ( t.ttt_s == Lower(Right(t.tfile_s.Get(),Len(t.ttt_s.Get())))  )  t.hardcodedtexturestages = 13;

			if (  t.hardcodedtexturestages>0 ) 
			{
				if (  t.hardcodedtexturestages == 1 ) 
				{
					//  [LM]+[DIFF+ILLU]
					t.ensureclearlayermax=2;
					t.texdir1_s=t.tstr1_s+"D2"+t.tstr2_s;
					t.texdir2_s=t.tstr1_s+"D2"+t.tstr2_s;
					t.texdir3_s=t.tstr1_s+"I"+t.tstr2_s;
				}
				if (  t.hardcodedtexturestages == 2 ) 
				{
					//  [LM]+[DIFF+NORM]
					t.ensureclearlayermax=2;
					t.texdir1_s=t.tstr1_s+t.texturingtoken_s+t.tstr2_s;
					t.texdir2_s=t.tstr1_s+t.texturingtoken_s+t.tstr2_s;
					t.texdir3_s=t.tstr1_s+"N"+t.tstr2_s;
					//  Absolutely must have a NORMAL MAP, or fail this effect
					t.absolutelyrequirethistexture=3;
				}
				if (  t.hardcodedtexturestages == 11 ) 
				{
					//  [DIFF+ILLU]
					t.ensureclearlayermax=1;
					t.texdir1_s=t.tstr1_s+"D2"+t.tstr2_s;
					t.texdir2_s=t.tstr1_s+"I"+t.tstr2_s;
				}
				if (  t.hardcodedtexturestages == 12 ) 
				{
					//  [DIFF+NORM]
					t.ensureclearlayermax=1;
					t.texdir1_s=t.tstr1_s+t.texturingtoken_s+t.tstr2_s;
					t.texdir2_s=t.tstr1_s+"N"+t.tstr2_s;
					//  Absolutely must have a NORMAL MAP, or fail this effect
					t.absolutelyrequirethistexture=2;
				}
				if (  t.hardcodedtexturestages == 13 ) 
				{
					//  [DIFF+NORM+SPEC]
					t.ensureclearlayermax=2;
					t.texdir1_s=t.tstr1_s+t.texturingtoken_s+t.tstr2_s;
					t.texdir2_s=t.tstr1_s+"N"+t.tstr2_s;
					t.texdir3_s=t.tstr1_s+"S"+t.tstr2_s;
					//  Absolutely must have a NORMAL MAP, or fail this effect
					t.absolutelyrequirethistexture=2;
				}
			}
			else
			{
				//  Standard DINS system
				if (  t.teffectuseslightmapstage == 1 ) 
				{
					//  [LM]+[DIFF+ILLU+NORM+SPEC+BRIH]
					t.texdir1_s=t.tstr1_s+t.texturingtoken_s+t.tstr2_s;
					t.texdir2_s=t.tstr1_s+t.texturingtoken_s+t.tstr2_s;
					t.texdir3_s=t.tstr1_s+"I"+t.tstr2_s;
					t.texdir4_s=t.tstr1_s+"N"+t.tstr2_s;
					t.texdir5_s=t.tstr1_s+"S"+t.tstr2_s;
				}
				else
				{
					//  [DIFF+ILLU+NORM+SPEC+BRIH]
					t.texdir1_s=t.tstr1_s+t.texturingtoken_s+t.tstr2_s;
					t.texdir2_s=t.tstr1_s+"I"+t.tstr2_s;
					t.texdir3_s=t.tstr1_s+"N"+t.tstr2_s;
					t.texdir4_s=t.tstr1_s+"S"+t.tstr2_s;
					t.texdir5_s=t.tstr1_s+"B"+t.tstr2_s;
				}
			}

			//  FPGC - 070710 - some entities do not follow pure naming conventions and are slightly modified
			//  such as Light4_G_G2.tga using the multi-use Light4_I.tga for the illumination, so we account for this
			for ( t.tcheck = 1 ; t.tcheck<=  4; t.tcheck++ )
			{
				if (  t.tcheck == 1  )  t.tcheck_s = t.texdir2_s;
				if (  t.tcheck == 2  )  t.tcheck_s = t.texdir3_s;
				if (  t.tcheck == 3  )  t.tcheck_s = t.texdir4_s;
				if (  t.tcheck == 4  )  t.tcheck_s = t.texdir5_s;
				if (  cstr(Right(Lower(t.tcheck_s.Get()),4)) == ".tga" ) {  t.tcheck_s = Left(t.tcheck_s.Get(),Len(t.tcheck_s.Get())-4); t.tcheck_s += ".dds"; }
				if (  FileExist(t.tcheck_s.Get()) == 0 ) 
				{
					//  assuming the format name_letter_D2.tga was used (therefore name_letter_I.tga)
					//  and the intended 'common support texture' was name_I.tga or name_N.tga or name_S.tga
					//  we can detect for and change it here if the alternative filename exists
					t.tcheck_s=Left(t.tcheck_s.Get(),Len(t.tcheck_s.Get())-7); t.tcheck_s += Right(t.tcheck_s.Get(),5);
					if (  cstr(Right(Lower(t.tcheck_s.Get()),4)) == ".tga"  )  { t.tcheck_s = Left(t.tcheck_s.Get(),Len(t.tcheck_s.Get())-4); t.tcheck_s += ".dds"; }
					if (  FileExist(t.tcheck_s.Get()) == 1 ) 
					{
						if (  t.tcheck == 1  )  t.texdir2_s = t.tcheck_s;
						if (  t.tcheck == 2  )  t.texdir3_s = t.tcheck_s;
						if (  t.tcheck == 3  )  t.texdir4_s = t.tcheck_s;
						if (  t.tcheck == 4  )  t.texdir5_s = t.tcheck_s;
					}
				}
			}
		}
		else
		{
			//  not D2.XXX, so just use texdir$ as base texture
			t.texdir1_s=t.texdir_s;
		}
	}
	else
	{
		//  use no effect this time
		t.tfile_s="";
	}
}

void version_splashtext_statusupdate ( void )
{
	// Update Splash Text (  with update on what is being loaded (startup IDE) )
	if ( t.game.gameisexe != 1 ) 
	{
		// and only if not running standalone
		welcome_updatebackdrop(t.tsplashstatusprogress_s.Get());
	}
}

//Functions

void popup_text_close ( void )
{
	#if defined(ENABLEIMGUI) && !defined(USEOLDIDE)
		return;
	#endif
	OpenFileMap (  2, "FPSPOPUP" );
	SetFileMapDWORD (  2, 8, 1 );
	SetEventAndWait (  2 );
	g_PopupControlMode = 0;
	Sleep(100);
}

void popup_text_change ( char* statusbar_s )
{
	#if defined(ENABLEIMGUI) && !defined(USEOLDIDE)
	//PE: Update prompt.
	popup_text(statusbar_s);
	return;
	#endif

	OpenFileMap (  2, "FPSPOPUP" );
	SetEventAndWait (  2 );
	if (  GetFileMapDWORD( 2, 0 )  ==  1 ) 
	{
		SetFileMapString ( 2, 1000, statusbar_s );
		SetFileMapDWORD ( 2, 4, 1 );
		SetEventAndWait ( 2 );
	}
}

char promptText[1024] = "\0";

void popup_text ( char* statusbar_s )
{
	// below cases ImGui::DockSpace to be called twice in same frame (assert)
	// and wicked does not prompt in this way any more
}

void loadresource ( void )
{
	cstr memoryusagetable_s =  "";
	int numberofitems = 0;
	int memused = 0;
	cstr name_s =  "";
	int n = 0;

	//  Load previously captured resource data
	memoryusagetable_s=g.mysystem.editorsGrideditAbs_s+"memusedtable.dat";//g.rootdir_s+"editors\\gridedit\\memusedtable.dat";
	if (  FileExist(memoryusagetable_s.Get()) == 1 ) 
	{
		OpenToRead (  1,memoryusagetable_s.Get() );
			numberofitems = ReadLong (  1 );
			if (  numberofitems>0 ) 
			{
				Dim (  t.gamememtable,numberofitems-1 );
				for ( n = 0 ; n<=  numberofitems-1; n++ )
				{
					name_s = ReadString (  1 ); t.gamememtable[n].name_s=name_s;
					memused = ReadLong ( 1 ); t.gamememtable[n].memused=memused;
				}
			}
			else
			{
				Undim ( t.gamememtable );
			}
		CloseFile (  1 );
	}
}

void saveresource ( void )
{
	cstr memoryusagetable_s =  "";
	int numberofitems = 0;
	int memused = 0;
	cstr name_s =  "";
	int n = 0;

	//  Save out resource captures
	memoryusagetable_s=g.mysystem.editorsGrideditAbs_s+"memusedtable.dat";//g.rootdir_s+"editors\\gridedit\\memusedtable.dat";
	if (  FileExist(memoryusagetable_s.Get()) == 1  )  DeleteAFile (  memoryusagetable_s.Get() );
	OpenToWrite (  1,memoryusagetable_s.Get() );
	numberofitems=1+ArrayCount(t.gamememtable);
	WriteLong (  1,numberofitems );
	if (  numberofitems>0 ) 
	{
		for ( n = 0 ; n<=  numberofitems-1; n++ )
		{
			name_s=t.gamememtable[n].name_s ; WriteString (  1,name_s.Get() );
			memused=t.gamememtable[n].memused ; WriteLong (  1,memused );
		}
	}
	CloseFile (  1 );
	if (  1 ) 
	{
		memoryusagetable_s=g.mysystem.editorsGrideditAbs_s+"memusedtable.log";//g.rootdir_s+"editors\\gridedit\\memusedtable.log";
		if (  FileExist(memoryusagetable_s.Get()) == 1  )  DeleteAFile (  memoryusagetable_s.Get() );
		OpenToWrite (  1,memoryusagetable_s.Get() );
			numberofitems=1+ArrayCount(t.gamememtable);
			sprintf ( t.szwork , "COUNT=%i" , numberofitems );
			WriteString (  1, t.szwork );
			if (  numberofitems>0 ) 
			{
				for ( n = 0 ; n<=  numberofitems-1; n++ )
				{
					sprintf ( t.szwork , "%i:%s[%i]" , n , t.gamememtable[n].name_s.Get() , t.gamememtable[n].memused );
					WriteString (  1, t.szwork );
				}
			}
			WriteString (  1,"END" );
		CloseFile (  1 );
	}
}

int openresource ( char* name_s )
{
	int actuallyused = 0;
	int gamememstamp = 0;
	int n;
	//  find existing slot, or free slot
	if (  Len(name_s)>1 ) 
	{
		actuallyused=0;
		g.gamememresourceid=-1;
		name_s=Lower(name_s);
		if (  ArrayCount(t.gamememtable) >= 0 ) 
		{
			for ( n = 0 ; n<=  ArrayCount(t.gamememtable); n++ )
			{
				if (  t.gamememtable[n].name_s == name_s ) 
				{
					g.gamememresourceid=n ; break;
				}
			}
		}

		//  test game records size of memory usage (start)
		if (  g.gamememresourceid == -1 ) 
		{
			n=ArrayCount(t.gamememtable)+1;
			Dim (  t.gamememtable,n );
			t.gamememtable[n].name_s=name_s;
			t.gamememtable[n].memused=-1;
			g.gamememresourceid=n;
		}
		gamememstamp=SMEMAvailable(1);
	}
	else
	{
		gamememstamp=SMEMAvailable(1);
		g.gamememresourceid=-1;
	}
	return actuallyused;
}

int closeresource ( void )
{
	int gamememstamp = 0;
	int memoryused = 0;
	if (  g.gamememresourceid >= 0 ) 
	{
		//  test game records size of memory usage (end)
		if (  t.gamememtable[g.gamememresourceid].memused == -1 ) 
		{
			//  and only if not filled in do we write the memory used
			memoryused=SMEMAvailable(1)-gamememstamp;
			t.gamememtable[g.gamememresourceid].memused=memoryused;
		}
		g.gamememresourceid=-1;
	}
	else
	{
		memoryused=SMEMAvailable(1)-gamememstamp;
	}
	return memoryused;
}

// 
//  SUBROUTINES AND FUNCTIONS FOR DEBUG ONLY
// 

void hide3d ( void )
{
	int o;
	BackdropOff ( ); SyncOff ( );
	SetCurrentBitmap (  0 );
	for ( o = 1 ; o <= 50000; o++ )
	{
		if ( ObjectExist(o) == 1 ) 
		{
			HideObject ( o );
		}
	}
}

void show3d ( void )
{
	int o;
	BackdropOn ( ); SyncOn ( );
	for ( o = 1 ; o <= 50000; o++ )
	{
		if (  ObjectExist(o) == 1 ) 
		{
			ShowObject (  o );
		}
	}
}

void debugfilename ( char* tfile_s, char* desc_s )
{
	cstr tryfile_s =  "";
	int texit = 0;
	texit=0;
	tryfile_s=tfile_s;
	if ( cstr( Lower(Right(tfile_s,3))) == "tga"  )  { tryfile_s = Left(tryfile_s.Get(),Len(tryfile_s.Get())-3); tryfile_s += g.imgext_s; }
	if (  FileExist(tfile_s) == 0 && FileExist(tryfile_s.Get()) == 0 ) 
	{
		sprintf ( t.szwork , "%s%s) info:%s" , t.strarr_s[53].Get() , tfile_s , desc_s );
		timestampactivity(0, t.szwork);
		g.timestampactivityflagged=1;
		//  FPGC - 090909 - add this to missingmedia report
		++g.missingmediacounter;
		Dim (  t.missingmedia_s,g.missingmediacounter  );
		t.missingmedia_s[g.missingmediacounter]=tfile_s;
	}
}

void debugstring ( char* tfile_s, char* desc_s )
{
	sprintf ( t.szwork , "%s%s) info:%s" , t.strarr_s[54].Get() , tfile_s , desc_s );
	timestampactivity(0, t.szwork);
}

void debugseevar ( int var )
{
	sprintf ( t.szwork , "%s%i)" , t.strarr_s[55].Get() , var );
	timestampactivity(0, t.szwork);
}

//Progress Report Debug Function

void debugviewprogressmax ( int progressmax )
{
	int gprogresscounter = 0;
	Dim (  t.mshot,progressmax  );
	g.lastmshotmem=0;
	g.gprogressmax=progressmax;
	gprogresscounter=0;
}

