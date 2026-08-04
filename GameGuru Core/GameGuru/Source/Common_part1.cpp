void FPSC_SetDefaults ( void )
{
	//  Very first task is find and load BUILD.INI (if flagged) 
	g.gcompilestandaloneexe = 0;
	g.gpretestsavemode = 0;

	//  Find and load SETUP.INI settings as global states
	g.grealgameviewstate = 0;
	g.gmultiplayergame = 0;
	g.gexitpromptreportmodestate = 0;
	g.gdebugphysicsstate = 0;
	g.gdebugreportstepthroughstate = 0;
	g.gshowentitygameinfostate = 0;
	g.gshowdebugtextingamestate = 0;
	g.gincludeonlyvideo = 0;
	g.gincludeonlyname_s = "";
	g.gignorefastbone = 0;
	g.glightmappingstate = 0;
	g.glightmappingold = 0;
	g.glightshadowsstate = 0;
	g.glightambientr = 0;
	g.glightambientg = 0;
	g.glightambientb = 0;
	g.glightsunx = 0;
	g.glightsuny = -1;
	g.glightsunz = 0;
	g.glightsunr = 0;
	g.glightsung = 0;
	g.glightsunb = 0;
	g.glightzerorange = 2000;
	g.glightatten = 16000;
	g.glightmaxsize = -1;
	g.glightboost = 4;
	g.glighttexsize = 512;
	g.glightquality = 5;
	g.glightblurmode = 1;
	g.glightthreadmax = 4;
	g.gdynamiclightingstate = 0;
	g.gdynamicshadowsstate = 1;
	g.gdividetexturesize = 0;
	g.goptimizemode = 0;
	g.ghsrmode = 0;
	g.guseskystate = 0;
	g.gusefloorstate = 0;
	g.guseenvsoundsstate = 0;
	g.guseweaponsstate = 0;
	g.gdisplaywidth = t.ScreenW;
	g.gdisplayheight = t.ScreenH;
	g.gdisplaydepth = 32;
	g.gsetupwidth = -1;
	g.gsetupheight = -1;
	g.gsetupdepth = 32;
	g.gfullscreen = 0;
	g.gvsync = 1;
	g.iEditorVSync = 0;
	g.gvrmode = 0;
	g.gvrmodefordevelopers = 0;
	g.gvrmodeoriginal = 0;
	g.gvrmodemag = 100;
	g.gvroffsetangx = 0;
	g.gvrwmroffsetangx = 0;
	g.gmousesensitivity = 100;
	g.guniquesignature = 0;
	g.ggameobjectivetype = 0;
	g.ggameobjectivevalue = 0;
	g.goneshotkills = 0;
	g.numberofplayers = 16;
	g.gspawnrandom = 0;
	g.guniquegamecode_s = "";
	g.guseuniquelynamedentities = 0;
	g.gexportassets = 0;
	g.gproducelogfiles = 0;
	g.gproducelogfilesdir_s = "";
	g.gpbroverride = 0;
	g.memskipwatermask = 0;

	g.maxtotalmeshlights = 20;
	g.maxpixelmeshlights = 10;
	g.terrainoldlight = 0;
	g.terrainusevertexlights = 1;
	g.maxterrainlights = 10;
	g.terrainlightfadedistance = 4000;
	g.showstaticlightinrealtime = 0;

	g.standalonefreememorybetweenlevels = 0;
	g.videoprecacheframes = 2;
	g.videodelayedload = 1;
	g.includeassetstore = 0;
	g.skipupdatecheck = 0;

	g.aidisabletreeobstacles = 0;
	g.aidisableobstacles = 0;
	g.skipunusedtextures = 0;

	g.lowestnearcamera = 2; //PE: default , use setup.ini lowestnearcamera to adjust.
	g.memgeneratedump = 0;
	g.underwatermode = 0;
	g.usegrassbelowwater = 1;
	g.editorsavebak = 0;
	g.gproducetruevidmemreading = 0;
	g.gcharactercapsulescale_f = 1.0;
	g.ggodmodestate = 0;
	g.glevelmax = 1;
	g.level = 1;
	g.glocalserveroverride_s = "";
	g.gbloodonfloor = 0;
	g.gimageblockmode = 0;
	g.shroudsize = 40;
	g.shroudsizedefaultsize = g.shroudsize;
	g.gxbox = 0;
	g.gxboxinvert = 0;
	g.gxboxcontrollertype = 0;
	g.gxboxmag = 1.0;
	g.gsuspendscreenprompts = 0;
	g.gforceloadtestgameshaders = 0;
	g.gshowalluniquetextures = 0;
	g.gaspectratio = 0;
	g.realaspect_f = 1.33f;
	g.gnewblossershaders = 1;
	g.gpostprocessing = 0;
	g.gpostprocessingnotransparency = 0;
	g.gfinalrendercameraid = 0;
	g.gshowaioutlines = 0;
	g.gairadius = 20;
	g.gdisablepeeking = 0;
	g.gsystemmemorycapoff = 0;
	g.gentitytogglingoff = 0;
	g.gextracollisionbuilddisabled = 1;
	g.galwaysconfirmsave = 0;
	g.gsimplifiedcharacterediting = 0;
	g.gantialias = 0;
	g.gminvert = 0;
	g.gdisablerightmousehold = 0;
	g.gparticlesnotused = 0;
	g.gautores = 0;
	g.guseoggoff = 0;
	g.gcapfpson = 0;
	g.createsplashsound = 1;
	g.ghardwareinfomode = 0;
	g.gprofileinstandalone = 0;
	g.greflectionrendersize = 512;
	g.gadapterordinal = 0;
	g.gadapterd3d11only = 0;
	g.ghideallhuds = 0;
	g.gskipobstaclecreation = 0;
	g.gskipterrainobstaclecreation = 0;
	g.gdeletetxpcachesonexit = 0;
	g.gdisablesurfacesnap = 0;
	g.gdefaultterrainheight = GGORIGIN_Y+10;
	g.gdefaultwaterheight = -500.0f; //GGORIGIN_Y;
	g.gdefaultebegridoffsetx = 50;
	g.gdefaultebegridoffsetz = 50;
	g.allowcpuanimations = 0;
	g.ggunmeleekey = 0;
	g.ggunaltswapkey1 = 47;
	g.ggunaltswapkey2 = 0;
	g.gzoomholdbreath = 16;
	g.fTerrainBrushSizeMax = 2000.0f;
	g.fLightmappingQuality = 1.5f;
	g.fLightmappingBlurLevel = 1.0f;
	g.iLightmappingSizeTerrain = 2048;
	g.iLightmappingSizeEntity = 512;
	g.fLightmappingSmoothAngle = 45.0f;
	g.iLightmappingExcludeTerrain = 0;
	g.iLightmappingDeactivateDirectionalLight = 0;
	g.fLightmappingAmbientR = 0.25f;
	g.fLightmappingAmbientG = 0.25f;
	g.fLightmappingAmbientB = 0.25f;
	g.iLightmappingAllTerrainLighting = 0;

	//Dynamic res
	t.DisableDynamicRes = false;
}

void FPSC_LoadSETUPVRINI ( void )
{
	// SETUPVR Info
	t.tfile_s = "setupvr.ini";
	if (FileExist(t.tfile_s.Get()) == 1)
	{
		Dim (t.data_s, 999);
		LoadArray (t.tfile_s.Get(), t.data_s);
		for (t.l = 0; t.l <= 999; t.l++)
		{
			t.line_s = t.data_s[t.l];
			if (Len(t.line_s.Get()) > 0)
			{
				if (cstr(Lower(Left(t.line_s.Get(), 4))) == ";end")  break;
				if (cstr(Left(t.line_s.Get(), 1)) != ";")
				{
					for (t.c = 0; t.c < Len(t.line_s.Get()); t.c++)
					{
						if (t.line_s.Get()[t.c] == '=') { t.mid = t.c + 1; break; }
					}
					t.field_s = Lower(removeedgespaces(Left(t.line_s.Get(), t.mid - 1)));
					t.value_s = removeedgespaces(Right(t.line_s.Get(), Len(t.line_s.Get()) - t.mid));
					for (t.c = 0; t.c < Len(t.value_s.Get()); t.c++)
					{
						if (t.value_s.Get()[t.c] == ',') { t.mid = t.c + 1; break; }
					}
					t.value1 = ValF(removeedgespaces(Left(t.value_s.Get(), t.mid - 1)));
					t.value2_s = removeedgespaces(Right(t.value_s.Get(), Len(t.value_s.Get()) - t.mid));
					if (Len(t.value2_s.Get()) > 0)  t.value2 = ValF(t.value2_s.Get()); else t.value2 = -1;

					// VRMode
					// 0 : off
					// 1 : VR920/iWear
					// 2 : GGVR (OpenVR)
					// 3 : GGVR (Microsoft WMR)
					// 4 : RESERVED - HOLDING VALUE (see code)
					// 5 : detects VR920/iWear (switches OFF if not found)
					// 6 : special case, side by side rendering
					t.tryfield_s = "vrmode";
					if (t.field_s == t.tryfield_s)
					{			
						g.gvrmode = t.value1;
						g.gvrmodeoriginal = t.value1;
						if (g.gvrmode != 0)
						{
							#ifndef GURULIGHTMAPPER
							HWND hThisWnd = g_pGlob->hWnd;
							#endif
						}
					}
					t.tryfield_s = "vrmodefordevelopers"; if (t.field_s == t.tryfield_s)  g.gvrmodefordevelopers = t.value1;
					t.tryfield_s = "vrmodemag"; if (t.field_s == t.tryfield_s)  g.gvrmodemag = t.value1;
					t.tryfield_s = "vroffsetangx"; if (t.field_s == t.tryfield_s)  g.gvroffsetangx = t.value1;
					t.tryfield_s = "vrwmroffsetangx"; if (t.field_s == t.tryfield_s)  g.gvrwmroffsetangx = t.value1;
					t.tryfield_s = "vrsitteradjust"; if (t.field_s == t.tryfield_s)  g.gvrsitteradjust = t.value1;
				}
			}
		}
		UnDim (t.data_s);
	}
}

void FPSC_SaveSETUPVRINI (void)
{
	// modified VR settings, so save to setupvr file
	Dim (t.setuparr_s, 999);
	t.setupfile_s = g.fpscrootdir_s; t.setupfile_s = t.setupfile_s  + "\\setupvr.ini"; t.i = 0;
	t.setuparr_s[t.i] = "[VR]"; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "vrmode=" + Str(g.gvrmode); ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "vrmodemag=" + Str(g.gvrmodemag); ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "vroffsetangx=" + Str(g.gvroffsetangx); ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "vrwmroffsetangx=" + Str(g.gvrwmroffsetangx); ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "vrmodefordevelopers=" + Str(g.gvrmodefordevelopers); ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "vrsitteradjust=" + Str(g.gvrsitteradjust); ++t.i;
	char pRealPath[MAX_PATH];
	strcpy(pRealPath, t.setupfile_s.Get());
	GG_GetRealPath(pRealPath, 1);
	if (FileExist(pRealPath) == 1) DeleteAFile (pRealPath);
	SaveArray (pRealPath, t.setuparr_s);
	UnDim (t.setuparr_s);
}

void FPSC_LoadSETUPINI (bool bUseMySystemFolder)
{
	//  SETUP Info
	if (bUseMySystemFolder == true)
	{
		// this means we are using the new Windows 10 style write folder in separate location
		cstr mysystemfolder_s = "My System";
		t.tfile_s = g.myownrootdir_s + "\\" + mysystemfolder_s + "\\" + g.setupfilename_s;
		if (FileExist (t.tfile_s.Get()) == 0)
		{
			// we need to copy the original from the read-only location to the new write location before using it
			CopyFileA (g.setupfilename_s.Get(), t.tfile_s.Get(), FALSE);
		}
	}
	else
	{
		// can use the local relative location of SETUP.INI (such as a Steam IDE or standalone game)
		t.tfile_s = g.setupfilename_s;
	}

	if (FileExist(t.tfile_s.Get()) == 1)
	{
		extern int g_iIgnoreAllErrors;
		g_iIgnoreAllErrors = 0;
		//  Load Data from file
		Dim (t.data_s, 999);
		LoadArray (t.tfile_s.Get(), t.data_s);
		for (t.l = 0; t.l <= 999; t.l++)
		{
			t.line_s = t.data_s[t.l];
			if (Len(t.line_s.Get()) > 0)
			{
				if (cstr(Lower(Left(t.line_s.Get(), 4))) == ";end")  break;
				if (cstr(Left(t.line_s.Get(), 1)) != ";")
				{
					// take fieldname and values
					for (t.c = 0; t.c < Len(t.line_s.Get()); t.c++)
					{
						if (t.line_s.Get()[t.c] == '=') { t.mid = t.c + 1; break; }
					}
					t.field_s = Lower(removeedgespaces(Left(t.line_s.Get(), t.mid - 1)));
					t.value_s = removeedgespaces(Right(t.line_s.Get(), Len(t.line_s.Get()) - t.mid));
					for (t.c = 0; t.c < Len(t.value_s.Get()); t.c++)
					{
						if (t.value_s.Get()[t.c] == ',') { t.mid = t.c + 1; break; }
					}
					t.value1 = ValF(removeedgespaces(Left(t.value_s.Get(), t.mid - 1)));
					t.value2_s = removeedgespaces(Right(t.value_s.Get(), Len(t.value_s.Get()) - t.mid));
					if (Len(t.value2_s.Get()) > 0)  t.value2 = ValF(t.value2_s.Get()); else t.value2 = -1;

					// All SETUP.INI Fields:

// DOCDOC: generatehelpfromdocdoc = Set to 1 to generate new assetitinerary file every time, 2 to prevent CUSTOM_* being added to FPM (dev)
					t.tryfield_s = "generateassetitinerary"; if (t.field_s == t.tryfield_s)  g.globals.generateassetitinerary = t.value1;

					// DOCDOC: generatehelpfromdocdoc = Enable GameGuru to generate new DOCDOC Help (when source relatively available).
					t.tryfield_s = "generatehelpfromdocdoc"; if (t.field_s == t.tryfield_s)  g.globals.generatehelpfromdocdoc = t.value1;

					// DOCDOC: superflatterrain = Set to 1 will force a simplified terrain geometry that is completely flat
					t.tryfield_s = "superflatterrain"; if (t.field_s == t.tryfield_s)  t.terrain.superflat = t.value1;

					// DOCDOC: riftmode = Discontinued
					t.tryfield_s = "riftmode"; if (t.field_s == t.tryfield_s)  g.globals.riftmode = t.value1;

					// DOCDOC: smoothcamerakeys = Add a smoothing function to the position and angle of the main camera
					t.tryfield_s = "smoothcamerakeys"; if (t.field_s == t.tryfield_s)  g.globals.smoothcamerakeys = t.value1;

					// DOCDOC: memorydetector = Activates extra memory usage and monitoring code
					t.tryfield_s = "memorydetector"; if (t.field_s == t.tryfield_s)  g.globals.memorydetector = t.value1;

					// DOCDOC: occlusionmode = Enables the use of the occlusion system to skip rendering of hidden entities
					t.tryfield_s = "occlusionmode"; if (t.field_s == t.tryfield_s)  g.globals.occlusionmode = t.value1;

					// DOCDOC: occlusionsize = Sets the size of the margins around occluders to occlude less of the scene
					t.tryfield_s = "occlusionsize"; if (t.field_s == t.tryfield_s)  g.globals.occlusionsize = t.value1;

					// DOCDOC: obstacleradius = Sets the size of the radius around AI entities for wall avoidance. Default is 18.
					t.tryfield_s = "obstacleradius"; if (t.field_s == t.tryfield_s)  t.aisystem.obstacleradius = t.value1;

					// DOCDOC: showdebugcollisonboxes = Renders the collision boxes associated with physics collision created by model importer
					t.tryfield_s = "showdebugcollisonboxes"; if (t.field_s == t.tryfield_s) g.globals.showdebugcollisonboxes = t.value1;

					// DOCDOC: hideebe = Hide the Builder menu from the main IDE
					t.tryfield_s = "hideebe"; if (t.field_s == t.tryfield_s) g.globals.hideebe = t.value1;

					// DOCDOC: hidedistantshadows = Causes more distant shadows to be hidden to improve performance
					t.tryfield_s = "hidedistantshadows"; if (t.field_s == t.tryfield_s) g.globals.hidedistantshadows = t.value1;


					// DOCDOC: realshadowresolution = Size of the texture plate dimension to render the shadow onto. Default is 2048.
					t.tryfield_s = "realshadowresolution"; if (t.field_s == t.tryfield_s) g.globals.realshadowresolution = t.value1;

					t.tryfield_s = "drawcalloptimizer"; if (t.field_s == t.tryfield_s) g.globals.drawcalloptimizer = t.value1;
					t.tryfield_s = "forcenowaterreflection"; if (t.field_s == t.tryfield_s) g.globals.forcenowaterreflection = t.value1;

					t.tryfield_s = "flashlightshadows"; if (t.field_s == t.tryfield_s)
					{
						g.globals.flashlightshadows = t.value1;
						if (g.globals.flashlightshadows > 1) g.globals.flashlightshadows = 1;
						if (g.globals.flashlightshadows < 0) g.globals.flashlightshadows = 0;
					}

					// DOCDOC: realshadowcascadecount = Set the number of shadow cascades to use. Default is 4, Min is 2 and Max is 8.
					t.tryfield_s = "realshadowcascadecount"; if (t.field_s == t.tryfield_s) g.globals.realshadowcascadecount = t.value1;
					if (g.globals.realshadowcascadecount < 2) g.globals.realshadowcascadecount = 2; //PE: Limit cascades.
					if (g.globals.realshadowcascadecount > 8) g.globals.realshadowcascadecount = 8; //PE: Limit cascades.
					if (g.globals.flashlightshadows == 1)
					{
						if (g.globals.realshadowcascadecount > 7) g.globals.realshadowcascadecount = 7; //PE: Limit cascades.
					}

					// DOCDOC: drawcalloptimizer = Set to 1 to activate the automatic batching of entities to improve performance
					t.tryfield_s = "drawcalloptimizer"; if (t.field_s == t.tryfield_s) g.globals.drawcalloptimizer = t.value1;

					// DOCDOC: forcenowaterreflection = Set to 1 to switch off water reflection internally for improved performance
					t.tryfield_s = "forcenowaterreflection"; if (t.field_s == t.tryfield_s) g.globals.forcenowaterreflection = t.value1;

					// DOCDOC: flashlightshadows = Set to 1 to activate an additional shadow cast from the flashlight (press F to activate flashlight)
					t.tryfield_s = "flashlightshadows"; if (t.field_s == t.tryfield_s)
					{
						g.globals.flashlightshadows = t.value1;
						if (g.globals.flashlightshadows > 1) g.globals.flashlightshadows = 1;
						if (g.globals.flashlightshadows < 0) g.globals.flashlightshadows = 0;
					}

					// DOCDOC: realshadowcascade0 thru realshadowcascade7 = Set the distance as a percentage when cascade kicks in
					t.tryfield_s = "realshadowcascade0"; if (t.field_s == t.tryfield_s) g.globals.realshadowcascade[0] = t.value1;
					t.tryfield_s = "realshadowcascade1"; if (t.field_s == t.tryfield_s) g.globals.realshadowcascade[1] = t.value1;
					t.tryfield_s = "realshadowcascade2"; if (t.field_s == t.tryfield_s) g.globals.realshadowcascade[2] = t.value1;
					t.tryfield_s = "realshadowcascade3"; if (t.field_s == t.tryfield_s) g.globals.realshadowcascade[3] = t.value1;
					t.tryfield_s = "realshadowcascade4"; if (t.field_s == t.tryfield_s) g.globals.realshadowcascade[4] = t.value1;
					t.tryfield_s = "realshadowcascade5"; if (t.field_s == t.tryfield_s) g.globals.realshadowcascade[5] = t.value1;
					t.tryfield_s = "realshadowcascade6"; if (t.field_s == t.tryfield_s) g.globals.realshadowcascade[6] = t.value1;
					t.tryfield_s = "realshadowcascade7"; if (t.field_s == t.tryfield_s) g.globals.realshadowcascade[7] = t.value1;

					// DOCDOC: realshadowsize0 thru realshadowsize7 = Not Used
					t.tryfield_s = "realshadowsize0"; if (t.field_s == t.tryfield_s) g.globals.realshadowsize[0] = t.value1;
					t.tryfield_s = "realshadowsize1"; if (t.field_s == t.tryfield_s) g.globals.realshadowsize[1] = t.value1;
					t.tryfield_s = "realshadowsize2"; if (t.field_s == t.tryfield_s) g.globals.realshadowsize[2] = t.value1;
					t.tryfield_s = "realshadowsize3"; if (t.field_s == t.tryfield_s) g.globals.realshadowsize[3] = t.value1;
					t.tryfield_s = "realshadowsize4"; if (t.field_s == t.tryfield_s) g.globals.realshadowsize[4] = t.value1;
					t.tryfield_s = "realshadowsize5"; if (t.field_s == t.tryfield_s) g.globals.realshadowsize[5] = t.value1;
					t.tryfield_s = "realshadowsize6"; if (t.field_s == t.tryfield_s) g.globals.realshadowsize[6] = t.value1;
					t.tryfield_s = "realshadowsize7"; if (t.field_s == t.tryfield_s) g.globals.realshadowsize[7] = t.value1;

					// DOCDOC: realshadowdistance = Sets the camera depth distance of the shadow render. Default is 5000.
					t.tryfield_s = "realshadowdistance"; if (t.field_s == t.tryfield_s)
					{
						g.globals.realshadowdistance = t.value1;
						g.globals.realshadowdistancehigh = t.value1;
					}
					// DOCDOC: Change XAudio2 sound scaler , higher = you can hear 3d sounds further away. default = 180
					t.tryfield_s = "curvedistancescaler"; if (t.field_s == t.tryfield_s)
					{
						g.globals.CurveDistanceScaler = t.value1;
					}

					t.tryfield_s = "converttodds"; if (t.field_s == t.tryfield_s)
					{
						g.globals.ConvertToDDS = t.value1;
					}
					t.tryfield_s = "disablemessagepump"; if (t.field_s == t.tryfield_s)
					{
						g.globals.DisableMessagePump = t.value1;
					}

					t.tryfield_s = "converttoddsmaxsize"; if (t.field_s == t.tryfield_s)
					{
						g.globals.ConvertToDDSMaxSize = t.value1;
					}

					t.tryfield_s = "ignoreallerrors"; if (t.field_s == t.tryfield_s)
					{
						g_iIgnoreAllErrors = t.value1;
					}

					// DOCDOC: editorusemediumshadows = Sets the editor to render medium level shadows while editing
					t.tryfield_s = "editorusemediumshadows"; if (t.field_s == t.tryfield_s)  g.globals.editorusemediumshadows = t.value1;

					// DOCDOC: hidememorygauge = Hides the memory gauge that displays in the top left of the editor area
					t.tryfield_s = "hidememorygauge"; if (t.field_s == t.tryfield_s)  g.ghidememorygauge = t.value1;

					// DOCDOC: hidelowfpswarning = Prevents the 'low FPS warning' prompt when playing the game
					t.tryfield_s = "hidelowfpswarning"; if (t.field_s == t.tryfield_s)  g.globals.hidelowfpswarning = t.value1;

					// DOCDOC: hardwareinfomode = Enables extra information to be displayed when F11 is pressed in game
					t.tryfield_s = "hardwareinfomode"; if (t.field_s == t.tryfield_s)  g.ghardwareinfomode = t.value1;

					// DOCDOC: profileinstandalone = Enables the debug options to remain in a standalone game
					t.tryfield_s = "profileinstandalone"; if (t.field_s == t.tryfield_s)  g.gprofileinstandalone = t.value1;

					// DOCDOC: allowfragmentation = Set to 0 to force game to relaunch at end of game, 1 to never relaunch the game and 2 to relaunch after every level. Default is 1.
					t.tryfield_s = "allowfragmentation"; if (t.field_s == t.tryfield_s)  t.game.allowfragmentation = t.value1;


					// DOCDOC: reflectionrendersize = Sets the size of the texture plate dimension for rendering the reflections in water. Default is 512.
					t.tryfield_s = "reflectionrendersize"; if (t.field_s == t.tryfield_s)  g.greflectionrendersize = t.value1;

					// DOCDOC: ignoretitlepage = Forces the title page to be skipped in standalone games
					t.tryfield_s = "ignoretitlepage"; if (t.field_s == t.tryfield_s)  t.game.ignoretitle = t.value1;

					// DOCDOC: deactivateconkit = Not Used
					t.tryfield_s = "deactivateconkit"; if (t.field_s == t.tryfield_s)  g.globals.deactivateconkit = t.value1;

					// DOCDOC: disablefreeflight = Disables the ability to use the free flight mode in the editor
					t.tryfield_s = "disablefreeflight"; if (t.field_s == t.tryfield_s)  g.globals.disablefreeflight = t.value1;

					// DOCDOC: fulldebugview = When set to 1, will detect and post the key state value of any key pressed in test game
					t.tryfield_s = "fulldebugview"; if (t.field_s == t.tryfield_s)  g.globals.fulldebugviewofkeymap = t.value1;	

					// DOCDOC: enableplrspeedmods = Enable the ability of weapons to affect the total running speed of the player
					t.tryfield_s = "enableplrspeedmods"; if (t.field_s == t.tryfield_s)  g.globals.enableplrspeedmods = t.value1;

					// DOCDOC: disableweaponjams = Disables the capability of weapons to 'jam' while being repeatedly fired
					t.tryfield_s = "disableweaponjams"; if (t.field_s == t.tryfield_s)  g.globals.disableweaponjams = t.value1;

					// DOCDOC: adapterordinal = Force the choice of DirectX Adapter to use. Set 1-98 to choose an adapter at that index, 99 to prefer the first non-Intel adapter from the list. Default is 0.
					t.tryfield_s = "adapterordinal"; if (t.field_s == t.tryfield_s)  g.gadapterordinal = t.value1;

					// DOCDOC: hideallhuds = Forces all display HUDs to hide when in the game
					t.tryfield_s = "hideallhuds"; if (t.field_s == t.tryfield_s)  g.ghideallhuds = t.value1;

					// DOCDOC: adapterd3d11only = Set to 1 to change the feature levels requested when DirectX is initialised
					t.tryfield_s = "adapterd3d11only"; if (t.field_s == t.tryfield_s)  g.gadapterd3d11only = t.value1;

					// DOCDOC: skipobstaclecreation = Speed up level preparation time by skipping AI obstacle creation. AI will not have pathfinding. Default is 0.
					t.tryfield_s = "skipobstaclecreation"; if (t.field_s == t.tryfield_s)  g.gskipobstaclecreation = t.value1;

					// DOCDOC: skipterrainobstaclecreation = Skips the creation of AI obstacles related to the terrain. Default is 0.
					t.tryfield_s = "skipterrainobstaclecreation"; if (t.field_s == t.tryfield_s)  g.gskipterrainobstaclecreation = t.value1;

					// DOCDOC: vsync = Enables the refresh of the render to match the current adapter refresh rate.
					t.tryfield_s = "vsync"; if (t.field_s == t.tryfield_s)  g.gvsync = t.value1;

					// DOCDOC: vsync = Enables the refresh of the render to match the current adapter refresh rate.
					t.tryfield_s = "editorvsync"; if (t.field_s == t.tryfield_s)  g.iEditorVSync = t.value1;

					// DOCDOC: fullscreen = Attempts to request a full-screen mode from the DirectX adapter.
					t.tryfield_s = "fullscreen"; if (t.field_s == t.tryfield_s)  g.gfullscreen = t.value1;

					// DOCDOC: width = Not Used
					t.tryfield_s = "width"; if (t.field_s == t.tryfield_s) g.gdisplaywidth = t.value1; t.newwidth = t.value1; g.gsetupwidth = t.value1;

					// DOCDOC: height - Not Used
					t.tryfield_s = "height"; if (t.field_s == t.tryfield_s) g.gdisplayheight = t.value1; t.newheight = t.value1; g.gsetupheight = t.value1;

					// DOCDOC: depth = Not Used
					t.tryfield_s = "depth"; if (t.field_s == t.tryfield_s) g.gdisplaydepth = t.value1; t.newdepth = t.value1; g.gsetupdepth = t.value1;

					// DOCDOC: aspectratio = Not Used
					t.tryfield_s = "aspectratio"; if (t.field_s == t.tryfield_s) g.gaspectratio = t.value1; t.newaspectratio = t.value1;

					// DOCDOC: realgameview = Not Used
					t.tryfield_s = "realgameview"; if (t.field_s == t.tryfield_s)  g.grealgameviewstate = t.value1;

					// DOCDOC: multiplayergame = Not Used
					t.tryfield_s = "multiplayergame"; if (t.field_s == t.tryfield_s)  g.gmultiplayergame = t.value1;

					// DOCDOC: exitpromptreport = Not Used
					t.tryfield_s = "exitpromptreport"; if (t.field_s == t.tryfield_s)  g.gexitpromptreportmodestate = t.value1;

					// DOCDOC: debugphysics = Not Used
					t.tryfield_s = "debugphysics"; if (t.field_s == t.tryfield_s)  g.gdebugphysicsstate = t.value1;

					// DOCDOC: debugreportstepthrough = Not Used
					t.tryfield_s = "debugreportstepthrough"; if (t.field_s == t.tryfield_s)  g.gdebugreportstepthroughstate = t.value1;

					// DOCDOC: showentitygameinfo = Displays extra information over the entity in test game
					t.tryfield_s = "showentitygameinfo"; if (t.field_s == t.tryfield_s)  g.gshowentitygameinfostate = t.value1;

					// DOCDOC: showdebugtextingame = Not Used
					t.tryfield_s = "showdebugtextingame"; if (t.field_s == t.tryfield_s)  g.gshowdebugtextingamestate = t.value1;

					// DOCDOC: includeonlyvideo = Enables image loading filter to only load files specified by includeonlyname
					t.tryfield_s = "includeonlyvideo"; if (t.field_s == t.tryfield_s)  g.gincludeonlyvideo = t.value1;

					// DOCDOC: includeonlyname = Sets the name that the includeonlyvideo mode uses to filter all image loading
					t.tryfield_s = "includeonlyname"; if (t.field_s == t.tryfield_s)  g.gincludeonlyname_s = t.value_s;

					// DOCDOC: ignorefastbone = Not Used
					t.tryfield_s = "ignorefastbone"; if (t.field_s == t.tryfield_s)  g.gignorefastbone = t.value1;

					// DOCDOC: loadreport = Not Used
					t.tryfield_s = "loadreport"; if (t.field_s == t.tryfield_s)  g.gloadreportstate = t.value1;

					// using new DocWrite system

					// DOCDOC: optimizemode = Not Used
					t.tryfield_s = "optimizemode"; if (t.field_s == t.tryfield_s)  g.goptimizemode = t.value1;

					// DOCDOC: lightmapping = Not Used 
					t.tryfield_s = "lightmapping"; if (t.field_s == t.tryfield_s)  g.glightmappingstate = t.value1;

					// DOCDOC: lightmapsize = Not Used
					t.tryfield_s = "lightmapsize"; if (t.field_s == t.tryfield_s)  t.glightmapsize = t.value1;

					// DOCDOC: lightmapquality = Not Used
					t.tryfield_s = "lightmapquality"; if (t.field_s == t.tryfield_s)  t.glightmapquality = t.value1;

					// DOCDOC: lightmapold = Not Used
					t.tryfield_s = "lightmapold"; if (t.field_s == t.tryfield_s)  g.glightmappingold = t.value1;

					// DOCDOC: lightmapshadows = Not Used
					t.tryfield_s = "lightmapshadows"; if (t.field_s == t.tryfield_s)  g.glightshadowsstate = t.value1;

					// DOCDOC: lightmapambientr = Not Used
					t.tryfield_s = "lightmapambientr"; if (t.field_s == t.tryfield_s)  g.glightambientr = t.value1;

					// DOCDOC: lightmapambientg = Not Used
					t.tryfield_s = "lightmapambientg"; if (t.field_s == t.tryfield_s)  g.glightambientg = t.value1;

					// DOCDOC: lightmapambientb = Not Used
					t.tryfield_s = "lightmapambientb"; if (t.field_s == t.tryfield_s)  g.glightambientb = t.value1;

					// DOCDOC: lightmapsunx = Not Used
					t.tryfield_s = "lightmapsunx"; if (t.field_s == t.tryfield_s)  g.glightsunx = t.value1;

					// DOCDOC: lightmapsuny = Not Used
					t.tryfield_s = "lightmapsuny"; if (t.field_s == t.tryfield_s)  g.glightsuny = t.value1;

					// DOCDOC: lightmapsunz = Not Used
					t.tryfield_s = "lightmapsunz"; if (t.field_s == t.tryfield_s)  g.glightsunz = t.value1;

					// DOCDOC: lightmapsunr = Not Used
					t.tryfield_s = "lightmapsunr"; if (t.field_s == t.tryfield_s)  g.glightsunr = t.value1;

					// DOCDOC: lightmapsung = Not Used
					t.tryfield_s = "lightmapsung"; if (t.field_s == t.tryfield_s)  g.glightsung = t.value1;

					// DOCDOC: lightmapsunb = Not Used
					t.tryfield_s = "lightmapsunb"; if (t.field_s == t.tryfield_s)  g.glightsunb = t.value1;

					// DOCDOC: lightmapzerorange = Not Used
					t.tryfield_s = "lightmapzerorange"; if (t.field_s == t.tryfield_s)  g.glightzerorange = t.value1;

					// DOCDOC: lightmapatten = Not Used
					t.tryfield_s = "lightmapatten"; if (t.field_s == t.tryfield_s)  g.glightatten = t.value1;

					// DOCDOC: lightmapmaxsize = Not Used
					t.tryfield_s = "lightmapmaxsize"; if (t.field_s == t.tryfield_s)  g.glightmaxsize = t.value1;

					// DOCDOC: lightmapboost = Not Used
					t.tryfield_s = "lightmapboost"; if (t.field_s == t.tryfield_s)  g.glightboost = t.value1;

					// DOCDOC: lightmaptexsize = Not Used
					t.tryfield_s = "lightmaptexsize"; if (t.field_s == t.tryfield_s)  g.glighttexsize = t.value1;

					// DOCDOC: lightmapquality = Not Used
					t.tryfield_s = "lightmapquality"; if (t.field_s == t.tryfield_s)  g.glightquality = t.value1;

					// DOCDOC: lightmapblurmode = Not Used
					t.tryfield_s = "lightmapblurmode"; if (t.field_s == t.tryfield_s)  g.glightblurmode = t.value1;

					// DOCDOC: lightmapthreadmax = Not Used
					t.tryfield_s = "lightmapthreadmax"; if (t.field_s == t.tryfield_s)  g.glightthreadmax = t.value1;

					// DOCDOC: bloodonfloor = Not Used
					t.tryfield_s = "bloodonfloor"; if (t.field_s == t.tryfield_s)  g.gbloodonfloor = t.value1;

					// DOCDOC: imageblockmode = Not Used
					t.tryfield_s = "imageblockmode"; if (t.field_s == t.tryfield_s)  g.gimageblockmode = t.value1;

					// DOCDOC: showalluniquetextures = Not Used
					t.tryfield_s = "showalluniquetextures"; if (t.field_s == t.tryfield_s)  g.gshowalluniquetextures = t.value1;

					// DOCDOC: systemmemorycapoff = Not Used
					t.tryfield_s = "systemmemorycapoff"; if (t.field_s == t.tryfield_s)  g.gsystemmemorycapoff = t.value1;

					// DOCDOC: entitytogglingoff = Disables ability to use the Y key to toggle entity between static and dynamic
					t.tryfield_s = "entitytogglingoff"; if (t.field_s == t.tryfield_s)  g.gentitytogglingoff = t.value1;

					// DOCDOC: extracollisionbuilddisabled = Not Used
					t.tryfield_s = "extracollisionbuilddisabled"; if (t.field_s == t.tryfield_s)  g.gextracollisionbuilddisabled = t.value1;

					// DOCDOC: alwaysconfirmsave = Asks user if they wish to save level before exiting editor
					t.tryfield_s = "alwaysconfirmsave"; if (t.field_s == t.tryfield_s)  g.galwaysconfirmsave = t.value1;

					// DOCDOC: simplifiedcharacterediting = Remove violent-related properties from character entities
					t.tryfield_s = "simplifiedcharacterediting"; if (t.field_s == t.tryfield_s)  g.gsimplifiedcharacterediting = t.value1;

					// DOCDOC: useoggoff = Not Used
					t.tryfield_s = "useoggoff"; if (t.field_s == t.tryfield_s)  g.guseoggoff = t.value1;

					// DOCDOC: cullmode = Not Used
					t.tryfield_s = "cullmode"; if (t.field_s == t.tryfield_s)  g.cullmode = t.value1;

					// DOCDOC: capfpson = Not Used
					t.tryfield_s = "capfpson"; if (t.field_s == t.tryfield_s)  g.gcapfpson = t.value1;

					// DOCDOC: disabledynamicres = Not Used
					t.tryfield_s = "disabledynamicres";
					if (t.field_s == t.tryfield_s)
					{
						if (t.value1 == 1)
							t.DisableDynamicRes = true;
						else
							t.DisableDynamicRes = false;
					}

					// DOCDOC: deletetxpcachesonexit = Enables temporary file deletion when creating custom textures for Builder and Terrain
					t.tryfield_s = "deletetxpcachesonexit"; if (t.field_s == t.tryfield_s)  g.gdeletetxpcachesonexit = t.value1;

					// DOCDOC: disablesurfacesnap = Disables the editor ability for some entities to locate surfaces to snap to
					t.tryfield_s = "disablesurfacesnap"; if (t.field_s == t.tryfield_s)  g.gdisablesurfacesnap = t.value1;

					// DOCDOC: defaultterrainheight = Sets the height at which a flat terrain is created by default. Default is 600.
					//t.tryfield_s = "defaultterrainheight" ; if (  t.field_s == t.tryfield_s  )  g.gdefaultterrainheight = t.value1;

					// DOCDOC: defaultwaterheight = Sets the height of the built-in water plane. Default is 500.
					t.tryfield_s = "defaultwaterheight"; if (t.field_s == t.tryfield_s)  g.gdefaultwaterheight = t.value1;

					// DOCDOC: defaultebegridoffsetx = Sets the grid X offset applied to Builder entities when placing them. Default is 50.
					t.tryfield_s = "defaultebegridoffsetx"; if (t.field_s == t.tryfield_s)  g.gdefaultebegridoffsetx = t.value1;

					// DOCDOC: defaultebegridoffsetz = Sets the grid Z offset applied to Builder entities when placing them. Default is 50.
					t.tryfield_s = "defaultebegridoffsetz"; if (t.field_s == t.tryfield_s)  g.gdefaultebegridoffsetz = t.value1;

					// DOCDOC: xbox = Sets whether the XBOX Style Controller should be detected and used to control the player
					t.tryfield_s = "xbox"; if (t.field_s == t.tryfield_s)  g.gxbox = t.value1;

					// DOCDOC: xboxinvert = Inverts the Y axis of the mouselook stick of an XBOX Style Controller
					t.tryfield_s = "xboxinvert"; if (t.field_s == t.tryfield_s)  g.gxboxinvert = t.value1;

					// DOCDOC: xboxcontrollertype = Sets the type of XBOX Style Controller used by the game, 1 is the old XBOX controller, 2 is the new XBOX Controller, 3 is the Dual Action F310 Controller.
					t.tryfield_s = "xboxcontrollertype"; if (t.field_s == t.tryfield_s)  g.gxboxcontrollertype = t.value1;

					// DOCDOC: xboxmag = Amplifies the sensitivity of the input values coming from the XBOX Style Controller. Default is 100.
					t.tryfield_s = "xboxmag"; if (t.field_s == t.tryfield_s)  g.gxboxmag = (0.0 + t.value1) / 100.0;

					// DOCDOC: mousesensitivity = Not Used
					t.tryfield_s = "mousesensitivity"; if (t.field_s == t.tryfield_s) g.gmousesensitivity = t.value1; t.newmousesensitivity = t.value1;

					// DOCDOC: dynamiclighting = Not Used
					t.tryfield_s = "dynamiclighting"; if (t.field_s == t.tryfield_s)  g.gdynamiclightingstate = t.value1;

					// DOCDOC: dynamicshadows = Not Used
					t.tryfield_s = "dynamicshadows"; if (t.field_s == t.tryfield_s) g.gdynamicshadowsstate = t.value1; t.newdynamicshadows = t.value1;

					// DOCDOC: dividetexturesize = Divides the size of the loaded textures by this value. Default is 0 for no division.
					t.tryfield_s = "dividetexturesize"; if (t.field_s == t.tryfield_s) g.gdividetexturesize = t.value1; t.newdividetexturesize = t.value1;

					// DOCDOC: svtatlasheight = Height of the terrain virtual-texture physical atlas. DEFAULT 12288 (2852 tiles, ~576MB pool); upstream stock is 16384 (3844 tiles, 768MB). Set 16384 to A/B against stock. DO NOT set 8192: a fast-travel soak measured peak demand at 1971 tiles, and 8192 provides only 1922, so the atlas starves (free hits 0) and terrain visibly blurs - see GameGuru Core/VRAM_CENSUS.md.
					t.tryfield_s = "svtatlasheight"; if (t.field_s == t.tryfield_s) { extern void GGSetSVTAtlasHeight(int); GGSetSVTAtlasHeight(t.value1); }

					// DOCDOC: svtemissive = Set 1 to restore the terrain virtual texture's EMISSIVE map (the stock 4th sparse map). DEFAULT 0: MAX never assigns an emissive texture to any terrain material, so the map and its 96MB pool share are dropped and a 1x1 black is bound in its place - zero visual change. Needs a level reload to take effect.
					t.tryfield_s = "svtemissive"; if (t.field_s == t.tryfield_s) { extern void GGSetSVTKeepEmissive(int); GGSetSVTKeepEmissive(t.value1); }

					// DOCDOC: lowvram = Low video memory preset, for cards with about 4GB. DEFAULT 0 (off - full DX12 visuals). Set 1 to trade visual reach for video memory so demos fit inside a 4GB card while holding 60 FPS. This is the MACHINE-WIDE switch (all levels); level authors can instead tick 'Low VRAM Mode (4GB cards)' in Graphics and Performance, saved per-level in the FPM - either switch turns the preset on. Members: grass draw distance cap (lowvramgrassdist), grass density (lowvramgrassdensity), shadow cascade/spot resolution capped at 1024, and SSR forced off. All are CAPS/clamps applied at the same points the level's own visuals are applied. See GameGuru Core/VRAM_FLOOR.md for the full floor analysis and what each knob buys.
					t.tryfield_s = "lowvram"; if (t.field_s == t.tryfield_s) { extern void GGSetLowVRAM(int); GGSetLowVRAM(t.value1); }

					// DOCDOC: lowvramgrassdist = Grass draw distance cap in inches used when lowvram=1. DEFAULT 750, which is the editor Grass Draw Distance slider's own minimum (below it the per-strand fade has no range to work in and grass pops in whole chunks at a time). This is a CAP: a level already asking for less keeps its own value. Ignored when lowvram=0.
					t.tryfield_s = "lowvramgrassdist"; if (t.field_s == t.tryfield_s) { extern void GGSetLowVRAMGrassDist(float); GGSetLowVRAMGrassDist((float)t.value1); }

					// DOCDOC: lowvramgrassdensity = Grass strand density used when lowvram=1, as a PERCENT of normal. DEFAULT 75 - the value that measurably fits every hub demo inside 4GB (2026-08-04 night audit). Grass memory is linear in strand count, so 50 is about half the grass video memory - by far the biggest content lever available, since grass is 17.3GB across the demo hub. This thins the grass EVENLY over the same painted area (it changes how many strands are drawn, never where they may go), so the grass gets sparser rather than patchy. Lower values are a deliberate visual trade for fitting a 4GB card. Ignored when lowvram=0.
					t.tryfield_s = "lowvramgrassdensity"; if (t.field_s == t.tryfield_s) { extern void GGSetLowVRAMGrassDensity(float); GGSetLowVRAMGrassDensity((float)t.value1 * 0.01f); }

					// DOCDOC: producelogfiles = Sets whether the editor and game produces .LOG files which time stamp and track events within the engine
					t.tryfield_s = "producelogfiles"; if (t.field_s == t.tryfield_s)  g.gproducelogfiles = t.value1;

					// DOCDOC: producelogfilesdir = Set a new folder for where the .LOG files will be saved out. Default is the root folder.
					t.tryfield_s = "producelogfilesdir"; if (t.field_s == t.tryfield_s)  g.gproducelogfilesdir_s = t.value_s;

					// DOCDOC: pbroverride = Activates the PBR rendering system. Set to 0 to disable PBR and revert to the DNS texture system. Default is 1.
					t.tryfield_s = "pbroverride"; if (t.field_s == t.tryfield_s)  g.gpbroverride = t.value1;

					// DOCDOC: underwatermode = Activates the advanced underwater rendering and activity mode.
					t.tryfield_s = "underwatermode"; if (t.field_s == t.tryfield_s)  g.underwatermode = t.value1;

					// DOCDOC: usegrassbelowwater = Renders grass below the water line.
					t.tryfield_s = "usegrassbelowwater"; if (t.field_s == t.tryfield_s)  g.usegrassbelowwater = t.value1;

					// DOCDOC: memskipwatermask = Disables the generation of a water mask which reduces the alpha of water as it reaches the shoreline.
					t.tryfield_s = "memskipwatermask"; if (t.field_s == t.tryfield_s)  g.memskipwatermask = t.value1;

					// DOCDOC: standalonefreememorybetweenlevels = Enables the deletion of textures from the previous level before loading the next one.
					t.tryfield_s = "standalonefreememorybetweenlevels"; if (t.field_s == t.tryfield_s)  g.standalonefreememorybetweenlevels = t.value1;

					// DOCDOC: videoprecacheframes = Set the amount of pre-caching each video in a game level uses, lowering memory usage. Default is 1.
					t.tryfield_s = "videoprecacheframes"; if (t.field_s == t.tryfield_s)  g.videoprecacheframes = t.value1;

					// DOCDOC: videodelayedload = Delays the loading of videos until they are needed in game, saving memory.
					t.tryfield_s = "videodelayedload"; if (t.field_s == t.tryfield_s)  g.videodelayedload = t.value1;

					// DOCDOC: includeassetstore = Activates the option to download store items direct into the software
					t.tryfield_s = "includeassetstore"; if (t.field_s == t.tryfield_s)  g.includeassetstore = t.value1;

					// DOCDOC: includeassetstore = Disables check for new updates when this is set to one
					t.tryfield_s = "skipupdatecheck"; if (t.field_s == t.tryfield_s)  g.skipupdatecheck = t.value1;

					// DOCDOC: aidisabletreeobstacles = Disable all AI obstacles for trees, improving level preparation time.
					t.tryfield_s = "aidisabletreeobstacles"; if (t.field_s == t.tryfield_s)  g.aidisabletreeobstacles = t.value1;

					// DOCDOC: aidisableobstacles = Disables all AI obstacles for all entities, improving level preparation time.
					t.tryfield_s = "aidisableobstacles"; if (t.field_s == t.tryfield_s)  g.aidisableobstacles = t.value1;

					// DOCDOC: skipunusedtextures = Skips loading of detail and height textures which are not overly used by most assets.
					t.tryfield_s = "skipunusedtextures"; if (t.field_s == t.tryfield_s)  g.skipunusedtextures = t.value1;

					// DOCDOC: lowestnearcamera = Reduce Z flicker issues by increasing this value. Recommended range for this is 8-14. Default is 1.
					t.tryfield_s = "lowestnearcamera"; if (t.field_s == t.tryfield_s)  g.lowestnearcamera = t.value1;

					// DOCDOC: editorsavebak = Enables the editor to save a .BAK file for .FPM level files
					t.tryfield_s = "editorsavebak"; if (t.field_s == t.tryfield_s)  g.editorsavebak = t.value1;

					// DOCDOC: terrainoldlight = Set to 1 to use the old terrain lighting system. Default is 0 for more than 3 lights on terrain.
					t.tryfield_s = "terrainoldlight"; if (t.field_s == t.tryfield_s)  g.terrainoldlight = t.value1;

					// DOCDOC: terrainusevertexlights = Set terrain to use vertex lighting instead of per pixel lighting for improved performance.
					t.tryfield_s = "terrainusevertexlights"; if (t.field_s == t.tryfield_s)  g.terrainusevertexlights = t.value1;

					// DOCDOC: showstaticlightinrealtime = Renders any static lights in the real-time scene in addition to existing dynamic lights.
					t.tryfield_s = "showstaticlightinrealtime"; if (t.field_s == t.tryfield_s)  g.showstaticlightinrealtime = t.value1;

					// DOCDOC: maxtotalmeshlights = Set the maximum number of lights to be used in the scene. Range is 4-38. Default is 38.
					t.tryfield_s = "maxtotalmeshlights";
					if (t.field_s == t.tryfield_s)
					{
						g.maxtotalmeshlights = t.value1;
						if (g.maxtotalmeshlights > 38) g.maxtotalmeshlights = 38;
						if (g.maxtotalmeshlights < 4) g.maxtotalmeshlights = 4; //PE: Lowest to support old system on terrain
					}

					// DOCDOC: maxpixelmeshlights = Set the maximum number of per pixel lights to be used in the scene. Range is 0-38. Default is 12.
					t.tryfield_s = "maxpixelmeshlights";
					if (t.field_s == t.tryfield_s)
					{
						g.maxpixelmeshlights = t.value1;
						if (g.maxpixelmeshlights > 38) g.maxpixelmeshlights = 38; //PE: Leave 2 vertex based lights per mesh.
					}

					// DOCDOC: maxterrainlights = Set the maximum number of terrain lights to be used in the scene. Range is 0-40. Default is 20.
					t.tryfield_s = "maxterrainlights";
					if (t.field_s == t.tryfield_s)
					{
						g.maxterrainlights = t.value1;
						if (g.maxterrainlights > 40) g.maxterrainlights = 40;
					}

					// DOCDOC: terrainlightfadedistance = Sets the distance at which terrain lights will fade out. Min is 600. Default is 4500.
					t.tryfield_s = "terrainlightfadedistance";
					if (t.field_s == t.tryfield_s)
					{
						g.terrainlightfadedistance = t.value1;
						if (g.terrainlightfadedistance < 600) g.terrainlightfadedistance = 600; //PE: Need atleast a 600 fade distance.
					}

					// DOCDOC: memskipibr = Set to skip the loading of the IBR file, used to pre-process lighting values for PBR rendering.
					t.tryfield_s = "memskipibr"; if (t.field_s == t.tryfield_s)  g.memskipibr = t.value1;

					// DOCDOC: memgeneratedump = Enable the dumping of a list of images loaded after each level.
					t.tryfield_s = "memgeneratedump"; if (t.field_s == t.tryfield_s)  g.memgeneratedump = t.value1;

					// DOCDOC: producetruevidmemreading = Adds better video memory usage stats to the .LOG file when produced
					t.tryfield_s = "producetruevidmemreading"; if (t.field_s == t.tryfield_s)  g.gproducetruevidmemreading = t.value1;

					// DOCDOC: charactercapsulescale = Sets a global scaling percentage to any character physics capsules created. Default is 100.
					t.tryfield_s = "charactercapsulescale"; if (t.field_s == t.tryfield_s)  g.gcharactercapsulescale_f = (t.value1 + 0.0) / 100.0;

					// DOCDOC: hsrmode = Not Used
					t.tryfield_s = "hsrmode"; if (t.field_s == t.tryfield_s)  g.ghsrmode = t.value1;

					// DOCDOC: newblossershaders = Not Used
					t.tryfield_s = "newblossershaders"; if (t.field_s == t.tryfield_s)  g.gnewblossershaders = t.value1;

					// DOCDOC: postprocessing = Enables the use of post processing when rendering the game
					t.tryfield_s = "postprocessing"; if (t.field_s == t.tryfield_s) g.gpostprocessing = t.value1; t.newpostprocessing = t.value1;

					// DOCDOC: showaioutlines = Not Used
					t.tryfield_s = "showaioutlines"; if (t.field_s == t.tryfield_s)  g.gshowaioutlines = t.value1;

					// DOCDOC: airadius = Sets the global radius for all AI bots in the game, within which they will not collide with each other.
					t.tryfield_s = "airadius"; if (t.field_s == t.tryfield_s)  g.gairadius = t.value1;

					// DOCDOC: disablepeeking = Not Used
					t.tryfield_s = "disablepeeking"; if (t.field_s == t.tryfield_s)  g.gdisablepeeking = t.value1;

					// DOCDOC: antialias = Not Used
					t.tryfield_s = "antialias"; if (t.field_s == t.tryfield_s) g.gantialias = t.value1; t.newantialias = t.value1;

					// DOCDOC: invmouse = Inverts the Y axis of the mouse input data.
					t.tryfield_s = "invmouse"; if (t.field_s == t.tryfield_s) g.gminvert = t.value1; t.newmouseinvert = t.value1;

					// DOCDOC: disablerightmousehold = Not Used
					t.tryfield_s = "disablerightmousehold"; if (t.field_s == t.tryfield_s)  g.gdisablerightmousehold = t.value1;

					// DOCDOC: disableparticles = Not Used
					t.tryfield_s = "disableparticles"; if (t.field_s == t.tryfield_s) g.gparticlesnotused = t.value1; t.newparticlesused = t.value1;

					// DOCDOC: autores = Not Used
					t.tryfield_s = "autores"; if (t.field_s == t.tryfield_s) g.gautores = t.value1; t.newautores = t.value1;

					// DOCDOC: terrainbrushsizemax = Sets the maximum size the terrain brush is allowed to go. Default is 2000.
					t.tryfield_s = "terrainbrushsizemax"; if (t.field_s == t.tryfield_s) g.fTerrainBrushSizeMax = t.value1;

					// DOCDOC: allowcpuanimations = Enables the ability for entities to specify CPU bone animations instead of GPU animations.
					t.tryfield_s = "allowcpuanimations"; if (t.field_s == t.tryfield_s) g.allowcpuanimations = t.value1;

					// DOCDOC: lightmappingquality = Sets the lightmapping quality level. Default is 500.
					t.tryfield_s = "lightmappingquality"; if (t.field_s == t.tryfield_s) g.fLightmappingQuality = t.value1 / 100.0f;

					// DOCDOC: lightmappingblurlevel = Sets the amount of blurring applied to the final lightmap. Default is 100.
					t.tryfield_s = "lightmappingblurlevel"; if (t.field_s == t.tryfield_s) g.fLightmappingBlurLevel = t.value1 / 100.0f;

					// DOCDOC: lightmappingsizeterrain = Sets the size of the texture plate used to store the terrain lightmap. Default is 2048.
					t.tryfield_s = "lightmappingsizeterrain"; if (t.field_s == t.tryfield_s) g.iLightmappingSizeTerrain = t.value1;

					// DOCDOC: lightmappingsizeentity = Sets the size of the texture plate used to store the entities lightmaps. Default is 1024.
					t.tryfield_s = "lightmappingsizeentity"; if (t.field_s == t.tryfield_s) g.iLightmappingSizeEntity = t.value1;

					// DOCDOC: lightmappingsmoothangle = Sets the angle within which smoothing will be applied to the edge. Default is 45.
					t.tryfield_s = "lightmappingsmoothangle"; if (t.field_s == t.tryfield_s) g.fLightmappingSmoothAngle = t.value1;

					// DOCDOC: lightmappingexcludeterrain = Set this to skip all terrain lightmapping. Default is 0.
					t.tryfield_s = "lightmappingexcludeterrain"; if (t.field_s == t.tryfield_s) g.iLightmappingExcludeTerrain = t.value1;

					// DOCDOC: lightmappingdeactivatedirectionallight = Disable any directional lighting from the sun within the lightmapping process. Default is 0.
					t.tryfield_s = "lightmappingdeactivatedirectionallight"; if (t.field_s == t.tryfield_s) g.iLightmappingDeactivateDirectionalLight = t.value1;

					// DOCDOC: lightmappingambientred = Sets the ambient Red color percentage to be applied during the lightmapping process. Default is dark grey, 25.
					t.tryfield_s = "lightmappingambientred"; if (t.field_s == t.tryfield_s) g.fLightmappingAmbientR = t.value1 / 100.0f;

					// DOCDOC: lightmappingambientgreen = Sets the ambient Green color percentage to be applied during the lightmapping process. Default is dark grey, 25.
					t.tryfield_s = "lightmappingambientgreen"; if (t.field_s == t.tryfield_s) g.fLightmappingAmbientG = t.value1 / 100.0f;

					// DOCDOC: lightmappingambientblue = Sets the ambient Blue color percentage to be applied during the lightmapping process. Default is dark grey, 25.
					t.tryfield_s = "lightmappingambientblue"; if (t.field_s == t.tryfield_s) g.fLightmappingAmbientB = t.value1 / 100.0f;

					// DOCDOC: lightmappingallterrainlighting = If no directional lightmapping, set this to force lightmap all the terrain area. Default is 0.
					t.tryfield_s = "lightmappingallterrainlighting"; if (t.field_s == t.tryfield_s) g.iLightmappingAllTerrainLighting = t.value1;

					// DOCDOC: suspendscreenprompts = Prevent screen prompts from being rendered to the screen. Default is 0.
					t.tryfield_s = "suspendscreenprompts"; if (t.field_s == t.tryfield_s)  g.gsuspendscreenprompts = t.value1;

					// DOCDOC: forceloadtestgameshaders = Set to 1 to generate new .BLOB files for all loaded shaders, 2 to force all shaders to have new .BLOB files. Default is 0.
					// 0 - off by default
					// 1 - generate new .BLOB files when a shader is loaded
					// 2 - scan effectbank folder and generate ALL NEW .BLOB files
					t.tryfield_s = "forceloadtestgameshaders"; if (t.field_s == t.tryfield_s)  g.gforceloadtestgameshaders = t.value1;

					// DOCDOC: reloadweapongunspecs = Forces a reload of the gun data file in case of buying and using weapons through the store. Default is 0.
					t.tryfield_s = "reloadweapongunspecs"; if (t.field_s == t.tryfield_s)  g.reloadWeaponGunspecs = t.value1;

					// DOCDOC: usesky = Not Used
					t.tryfield_s = "usesky"; if (t.field_s == t.tryfield_s)  g.guseskystate = t.value1;

					// DOCDOC: usefloor = Not Used
					t.tryfield_s = "usefloor"; if (t.field_s == t.tryfield_s)  g.gusefloorstate = t.value1;

					// DOCDOC: useenvsounds = Not Used
					t.tryfield_s = "useenvsounds"; if (t.field_s == t.tryfield_s)  g.guseenvsoundsstate = t.value1;

					// DOCDOC: useweapons = Not Used
					t.tryfield_s = "useweapons"; if (t.field_s == t.tryfield_s)  g.guseweaponsstate = t.value1;

					// DOCDOC: godmode = Enables the use of God Mode, which increases player health to 99999 when the 'I' key is pressed in game.
					t.tryfield_s = "godmode"; if (t.field_s == t.tryfield_s)  g.ggodmodestate = 0;

					// DOCDOC: uniquesignature = Not Used
					t.tryfield_s = "uniquesignature"; if (t.field_s == t.tryfield_s)  g.guniquesignature = t.value1;

					// DOCDOC: gameobjectivetype = Not Used
					t.tryfield_s = "gameobjectivetype"; if (t.field_s == t.tryfield_s)  g.ggameobjectivetype = t.value1;

					// DOCDOC: gameobjectivevalue = Not Used
					t.tryfield_s = "gameobjectivevalue"; if (t.field_s == t.tryfield_s)  g.ggameobjectivevalue = t.value1;

					// DOCDOC: oneshotkills = Not Used
					t.tryfield_s = "oneshotkills"; if (t.field_s == t.tryfield_s)  g.goneshotkills = t.value1;

					// DOCDOC: maxplayers = Not Used
					t.tryfield_s = "maxplayers"; if (t.field_s == t.tryfield_s)  g.numberofplayers = t.value1;

					// DOCDOC: spawnrandom = Not Used
					t.tryfield_s = "spawnrandom"; if (t.field_s == t.tryfield_s)  g.gspawnrandom = t.value1;

					// DOCDOC: uniquegamecode = Not Used
					t.tryfield_s = "uniquegamecode"; if (t.field_s == t.tryfield_s)  g.guniquegamecode_s = t.value_s;

					// DOCDOC: useuniquelynamedentities = Set to 1 so editor will assign unique names to added entities. Default is 0.
					t.tryfield_s = "useuniquelynamedentities"; if (t.field_s == t.tryfield_s)  g.guseuniquelynamedentities = t.value1;

					// DOCDOC: exportassets = Enables the ability for save standalone to include the FPE along with the entities other resources.
					t.tryfield_s = "exportassets"; if (t.field_s == t.tryfield_s)  g.gexportassets = t.value1;

					// DOCDOC: localserver = Not Used
					t.tryfield_s = "localserver"; if (t.field_s == t.tryfield_s)  g.glocalserveroverride_s = t.value_s;

					// DOCDOC: title = Not Used
					t.tryfield_s = "title"; if (t.field_s == t.tryfield_s)  t.titlefpi_s = t.value_s;

					// DOCDOC: global = Not Used
					t.tryfield_s = "global"; if (t.field_s == t.tryfield_s)  t.setupfpi_s = t.value_s;

					// DOCDOC: gamewon = Not Used
					t.tryfield_s = "gamewon"; if (t.field_s == t.tryfield_s)  t.gamewonfpi_s = t.value_s;

					// DOCDOC: gameover = Not Used
					t.tryfield_s = "gameover"; if (t.field_s == t.tryfield_s)  t.gameoverfpi_s = t.value_s;

					// DOCDOC: levelfpi1 = Not Used
					t.tryfield_s = "levelfpi1"; if (t.field_s == t.tryfield_s) t.loadingfpi_s == t.value_s; t.levelfpiinsetup = t.l;

					// DOCDOC: hudr = Not Used
					t.tryfield_s = "hudr"; if (t.field_s == t.tryfield_s)  g.r_f = t.value1;

					// DOCDOC: hudg = Not Used
					t.tryfield_s = "hudg"; if (t.field_s == t.tryfield_s)  g.g_f = t.value1;

					// DOCDOC: hudb = Not Used
					t.tryfield_s = "hudb"; if (t.field_s == t.tryfield_s)  g.b_f = t.value1;

					// DOCDOC: autoswaptrue = Not Used
					t.tryfield_s = "autoswaptrue"; if (t.field_s == t.tryfield_s)  g.autoswap = t.value1;

					// DOCDOC: messagetime = Not Used
					t.tryfield_s = "messagetime"; if (t.field_s == t.tryfield_s)  g.messagetime = t.value1;

					// DOCDOC: allowscope = Not Used
					t.tryfield_s = "allowscope"; if (t.field_s == t.tryfield_s)  g.allowscope_s = t.value1;

					// DOCDOC: serverhostname = Not Used
					t.tryfield_s = "serverhostname"; if (t.field_s == t.tryfield_s)  g.serverhostname = t.value_s;

					// DOCDOC: alwaysrun = Not Used
					t.tryfield_s = "alwaysrun"; if (t.field_s == t.tryfield_s)  g.alwaysrun = t.value1;

					// DOCDOC: matchtype = Not Used
					t.tryfield_s = "matchtype"; if (t.field_s == t.tryfield_s)  g.multi_match_type = t.value1;

					// DOCDOC: multiradar = Not Used
					t.tryfield_s = "multiradar"; if (t.field_s == t.tryfield_s)  g.darkradar = t.value1;

					// DOCDOC: multicompass = Not Used
					t.tryfield_s = "multicompass";; if (t.field_s == t.tryfield_s)  g.compassOn = t.value1;

					// DOCDOC: multicompassx = Not Used
					t.tryfield_s = "multicompassx"; if (t.field_s == t.tryfield_s)  g.compassX = t.value1;

					// DOCDOC: multicompassy = Not Used
					t.tryfield_s = "multicompassy"; if (t.field_s == t.tryfield_s)  g.compassY = t.value1;

					// DOCDOC: multiradarx = Not Used
					t.tryfield_s = "multiradarx"; if (t.field_s == t.tryfield_s)  g.radarx = t.value1;

					// DOCDOC: multiradary = Not Used
					t.tryfield_s = "multiradary"; if (t.field_s == t.tryfield_s)  g.radary = t.value1;

					// DOCDOC: levelmax = Not Used
					if (t.field_s == "levelmax")  g.glevelmax = t.value1;
					if (g.glevelmax > 0)
					{
						for (t.v = 1; t.v <= g.glevelmax; t.v++)
						{
							// DOCDOC: levelfpm = Not Used
							sprintf (t.szwork, "levelfpm%s", Str(t.v));
							t.tryfield_s = t.szwork;
							if (t.field_s == t.tryfield_s)  t.levelfpm_s = t.value_s;

							// DOCDOC: levelfpi = Not used
							sprintf (t.szwork, "levelfpi%s", Str(t.v));
							t.tryfield_s = t.szwork;
							if (t.field_s == t.tryfield_s)
							{
								t.levelfpi_s = t.value_s;
								Dim (t.level_s, t.v);
								t.level_s[t.v].fpm_s = t.levelfpm_s;
								t.level_s[t.v].fpi_s = t.levelfpi_s;
							}
						}
					}

					// DOCDOC: melee key = Specifies melee key for this weapon
					if (t.field_s == "melee key")  g.ggunmeleekey = t.value1;

					// DOCDOC: switchtoalt = Not Used
					if (t.field_s == "switchtoalt")
					{
						if (t.value1 != 0) g.ggunaltswapkey1 = t.value1;
						g.ggunaltswapkey2 = t.value2;
						if (t.value2 == 0)  g.ggunaltswapkey2 = -1;
					}

					// DOCDOC: zoomholdbreath = The keymap value to be used to hold breath while zooming. Default is 16 (letter Q).
					if (t.field_s == "zoomholdbreath")  g.gzoomholdbreath = t.value1;

					// DOCDOC: key1 thru key11 = Old style mapping of control keys to be used in the game, now depreciated.
					for (t.num = 1; t.num <= 11; t.num++)
					{
						sprintf (t.szwork, "key%s", Str(t.num));
						t.tryfield_s = t.szwork;
						if (t.field_s == t.tryfield_s)  t.listkey[t.num] = t.value1;
					}

					for (t.num = 1; t.num <= 11; t.num++)
					{
						// DOCDOC: keyup = Assigns a new keymap value to represent the indicated control action. Default is 17.
						if (t.num == 1)  t.tryfield_s = "keyup";
						// DOCDOC: keydown = Assigns a new keymap value to represent the indicated control action. Default is 31.
						if (t.num == 2)  t.tryfield_s = "keydown";
						// DOCDOC: keyleft = Assigns a new keymap value to represent the indicated control action. Default is 30.
						if (t.num == 3)  t.tryfield_s = "keyleft";
						// DOCDOC: keyright = Assigns a new keymap value to represent the indicated control action. Default is 32.
						if (t.num == 4)  t.tryfield_s = "keyright";
						// DOCDOC: keyjump = Assigns a new keymap value to represent the indicated control action. Default is 57.
						if (t.num == 5)  t.tryfield_s = "keyjump";
						// DOCDOC: keycrouch = Assigns a new keymap value to represent the indicated control action. Default is 46.
						if (t.num == 6)  t.tryfield_s = "keycrouch";
						// DOCDOC: keyenter = Assigns a new keymap value to represent the indicated control action. Default is 28.
						if (t.num == 7)  t.tryfield_s = "keyenter";
						// DOCDOC: keyreload = Assigns a new keymap value to represent the indicated control action. Default is 19.
						if (t.num == 8)  t.tryfield_s = "keyreload";
						// DOCDOC: keypeekleft = Assigns a new keymap value to represent the indicated control action. Default is 16.
						if (t.num == 9)  t.tryfield_s = "keypeekleft";
						// DOCDOC: keypeekright = Assigns a new keymap value to represent the indicated control action. Default is 18.
						if (t.num == 10)  t.tryfield_s = "keypeekright";
						// DOCDOC: keyrun = Assigns a new keymap value to represent the indicated control action. Default is 42.
						if (t.num == 11)  t.tryfield_s = "keyrun";
						if (t.field_s == t.tryfield_s)  t.listkey[t.num] = t.value1;
					}

					// DOCDOC: slot1 thru slot9 = Pre-assign weapon ID values to the nine available gun slots in the game
					for (t.num = 1; t.num <= 9; t.num++)
					{
						sprintf (t.szwork, "slot%i", t.num);
						t.tryfield_s = t.szwork;
						if (t.field_s == t.tryfield_s)  t.gunslots_s[t.num] = t.value_s;
					}

					// DOCDOC: taunt1 thru taunt30 = Not used
					for (t.num = 1; t.num <= 30; t.num++)
					{
						sprintf (t.szwork, "taunt%i", t.num);
						t.tryfield_s = t.szwork;
						if (t.field_s == t.tryfield_s)  t.taunt_s[t.num] = t.value_s;
					}

					// DOCDOC: language = Sets the language folder inside 'languagebank' to use for the game, defaults to 'English'.
					t.tryfield_s = "language"; if (t.field_s == t.tryfield_s)  g.language_s = t.value_s;

					// DOCDOC: graphicslowterrain = Pre-assign the terrain shader level to use when the in-game menu selects LOW for graphics.
					t.tryfield_s = "graphicslowterrain"; if (t.field_s == t.tryfield_s)  g.graphicslowterrain_s = t.value_s;

					// DOCDOC: graphicslowentity = Pre-assign the entity shader level to use when the in-game menu selects LOW for graphics.
					t.tryfield_s = "graphicslowentity"; if (t.field_s == t.tryfield_s)  g.graphicslowentity_s = t.value_s;

					// DOCDOC: graphicslowgrass = Pre-assign the grass shader level to use when the in-game menu selects LOW for graphics.
					t.tryfield_s = "graphicslowgrass"; if (t.field_s == t.tryfield_s)  g.graphicslowgrass_s = t.value_s;

					// DOCDOC: graphicsmediumterrain = Pre-assign the terrain shader level to use when the in-game menu selects MEDIUM for graphics.
					t.tryfield_s = "graphicsmediumterrain"; if (t.field_s == t.tryfield_s)  g.graphicsmediumterrain_s = t.value_s;

					// DOCDOC: graphicsmediumentity = Pre-assign the entity shader level to use when the in-game menu selects MEDIUM for graphics.
					t.tryfield_s = "graphicsmediumentity"; if (t.field_s == t.tryfield_s)  g.graphicsmediumentity_s = t.value_s;

					// DOCDOC: graphicsmediumgrass = Pre-assign the grass shader level to use when the in-game menu selects MEDIUM for graphics.
					t.tryfield_s = "graphicsmediumgrass"; if (t.field_s == t.tryfield_s)  g.graphicsmediumgrass_s = t.value_s;

					// DOCDOC: graphicshighterrain = Pre-assign the terrain shader level to use when the in-game menu selects HIGH for graphics.
					t.tryfield_s = "graphicshighterrain"; if (t.field_s == t.tryfield_s)  g.graphicshighterrain_s = t.value_s;

					// DOCDOC: graphicshighentity = Pre-assign the entity shader level to use when the in-game menu selects HIGH for graphics.
					t.tryfield_s = "graphicshighentity"; if (t.field_s == t.tryfield_s)  g.graphicshighentity_s = t.value_s;

					// DOCDOC: graphicshighgrass = Pre-assign the grass shader level to use when the in-game menu selects HIGH for graphics.
					t.tryfield_s = "graphicshighgrass"; if (t.field_s == t.tryfield_s)  g.graphicshighgrass_s = t.value_s;

					// DOCDOC: graphicshighgrass = Pre-assign the grass shader level to use when the in-game menu selects HIGH for graphics.
					extern int g_iDevToolsOpen;
					t.tryfield_s = "superdevelopermode"; if (t.field_s == t.tryfield_s)
					{
						// dev tools open is now saved in pref, but settings only allow 0 or 1. 
						if (t.value1 > 1)
						{
							g_iDevToolsOpen = t.value1;
						}
						else
						{
							// Will use the pref value instead
						}
					}
					extern int g_iUseLODObjects;
					t.tryfield_s = "uselodobjects"; if (t.field_s == t.tryfield_s) g_iUseLODObjects = t.value1;

					// DOCDOC: graphicshighgrass = Pre-assign the grass shader level to use when the in-game menu selects HIGH for graphics.
					t.tryfield_s = "globalhudscale"; if (t.field_s == t.tryfield_s)  g.globalhudscale = t.value1 / 100.0f;

					extern int g_iDisableTerrainSystem;
					t.tryfield_s = "disableterrainsystem"; if (t.field_s == t.tryfield_s) g_iDisableTerrainSystem = t.value1;

					extern int g_iDisableWParticleSystem;
					t.tryfield_s = "disablewparticlesystem"; if (t.field_s == t.tryfield_s) g_iDisableWParticleSystem = t.value1;
				}
			}
		}
		UnDim (t.data_s);

		g.ghidememorygauge = 1; //PE: Box() dont work in wicked anyway.
		//  V118 - 160810 - knxrb - Auto Resolution
		if (g.gautores == 1)
		{
			g.gdisplaywidth = GetDesktopWidth();
			g.gdisplayheight = GetDesktopHeight();
		}
	}
	else
	{
		//  No SETUP.INI, default is a standalone game
		g.grealgameviewstate = 1;
		g.gdynamiclightingstate = 1;
		g.guseskystate = 1;
		g.gusefloorstate = 0;
		g.guseenvsoundsstate = 1;
		g.guseweaponsstate = 1;
	}

	// send shadowmap details to engine
	int rs0 = g.globals.realshadowsize[0];
	int rs1 = g.globals.realshadowsize[1];
	int rs2 = g.globals.realshadowsize[2];
	int rs3 = g.globals.realshadowsize[3];
	int rs4 = g.globals.realshadowsize[4];
	int rs5 = g.globals.realshadowsize[5];
	int rs6 = g.globals.realshadowsize[6];
	int rs7 = g.globals.realshadowsize[7];
	float dist = g.globals.realshadowdistance;
	int flash = g.globals.flashlightshadows;
	int speed = 0;
	//InitShadowMapDetails(rs0, rs1, rs2, rs3, rs4, rs5, rs6, rs7, dist, flash, speed); // function no longer exists

	// special global flag which can affect how shaders are loaded
	if (g.gforceloadtestgameshaders != 0) gbAlwaysIgnoreShaderBlobFile = true;
}

LPSTR FindFileFromEntityBank ( LPSTR pFindThisFilename )
{
	// look through entire file collection for this file
	for ( int f = 1; f <= g.filecollectionmax; f++ )
	{
		LPSTR pFile = t.filecollection_s[f].Get();
		LPSTR pFileNameOnly = NULL;
		for ( int n = strlen(pFile); n > 0; n-- )
		{
			if ( pFile[n] == '\\' || pFile[n] == '/' )
			{
				pFileNameOnly = pFile+n+1;
				break;
			}
		}
		if ( pFileNameOnly )
		{
			if ( stricmp ( pFileNameOnly, pFindThisFilename ) == NULL )
			{
				// found the file!
				LPSTR pReturnAbsPathToFile = new char[2048];
				strcpy ( pReturnAbsPathToFile, g.fpscrootdir_s.Get() );
				strcat ( pReturnAbsPathToFile, "\\Files\\entitybank\\Objects - Copy\\" );
				strcat ( pReturnAbsPathToFile, pFile );
				return pReturnAbsPathToFile;
			}
		}
	}
	return NULL;
}

void FPSC_LoadKEYMAP ( void )
{
	// first reset back to default no matter what
	for (t.num = 256; t.num >= 1; t.num--)
	{
		g.keymap[t.num] = (unsigned char)t.num;
	}

	// look in editors\keymap\ to find a non-default.ini to prefer
	if (PathExist("editors\\keymap") == 1 )
	{
		cstr pOldDir = GetDir();
		char pWritableKeyMapFile[MAX_PATH];
		strcpy(pWritableKeyMapFile, "editors\\keymap\\");
		GG_GetRealPath(pWritableKeyMapFile, 0);
		SetDir(pWritableKeyMapFile);
		ChecklistForFiles();
		cstr useRelThisKeyMapFile = "editors\\keymap\\default.ini";
		for (t.c = 1; t.c <= ChecklistQuantity(); t.c++)
		{
			t.tfile_s = ChecklistString(t.c);
			if (Len(t.tfile_s.Get()) > 2)
			{
				if (stricmp(t.tfile_s.Get(), "default.ini") != NULL)
				{
					useRelThisKeyMapFile = "editors\\keymap\\";
					useRelThisKeyMapFile += t.tfile_s;
					break;
				}
			}
		}
		SetDir(pOldDir.Get());

		//  editors\keymap\default.ini
		strcpy(pWritableKeyMapFile, useRelThisKeyMapFile.Get());
		GG_GetRealPath(pWritableKeyMapFile, 0);
		t.tfile_s = pWritableKeyMapFile;
		if ( FileExist(t.tfile_s.Get()) == 1 )
		{
			Dim (  t.data_s,999  );
			LoadArray (  t.tfile_s.Get(),t.data_s );
			for ( t.l = 0 ; t.l<=  999; t.l++ )
			{
				t.line_s=t.data_s[t.l];
				if (  Len(t.line_s.Get())>0 ) 
				{
					if (  cstr(Lower(Left(t.line_s.Get(),4))) == ";end"  )  break;
					if (  cstr(Left(t.line_s.Get(),1)) != ";" ) 
					{
						//  take fieldname and values
						for ( t.c = 0 ; t.c < Len(t.line_s.Get()); t.c++ )
						{
							if (  t.line_s.Get()[t.c] == '=' ) { t.mid = t.c+1; break; }
						}
						t.field_s=Lower(removeedgespaces(Left(t.line_s.Get(),t.mid-1)));
						t.value_s=removeedgespaces(Right(t.line_s.Get(),Len(t.line_s.Get())-t.mid));
						for ( t.c = 0 ; t.c < Len(t.value_s.Get()); t.c++ )
						{
							if ( t.value_s.Get()[t.c] == ',' ) { t.mid = t.c+1; break; }
						}
						t.value1=ValF(removeedgespaces(Left(t.value_s.Get(),t.mid-1)));
						t.value2_s=removeedgespaces(Right(t.value_s.Get(),Len(t.value_s.Get())-t.mid));
						if (  Len(t.value2_s.Get())>0  )  t.value2 = ValF(t.value2_s.Get()); else t.value2 = -1;

						// try to match with key256 thru key1
						for ( t.num = 256; t.num >= 1; t.num-- )
						{
							sprintf ( t.szwork, "key%s", Str(t.num) );
							t.tryfield_s = t.szwork;
							if ( strnicmp ( t.field_s.Get(), t.tryfield_s.Get(), strlen(t.tryfield_s.Get()) ) == NULL )  
							{
								// and ensure its the exact value, is there a non-num after matched part
								char pDigitAfterNumber = t.field_s.Get()[strlen(t.szwork)];
								char pStartNum = '1';
								char pFinishNum = '0';
								if ( pDigitAfterNumber<pStartNum || pDigitAfterNumber>pFinishNum )
								{
									g.keymap[t.num] = (unsigned char)t.value1;
									break;
								}
							}
						}
					}
				}
			}
			UnDim ( t.data_s );
		}
	}
}

void GenerateDOCDOCHelpFiles ( void )
{
	// init string list for help file
	std::vector<cstr> pHelpItems;

	// load in a source file to scan
	LPSTR pSourceFile = "..\\..\\GameGuru Core\\GameGuru\\Source\\Common.cpp";
	if ( FileExist ( pSourceFile ) == 1 )
	{
		// read source file, look for DOCDOC
		if ( FileOpen(1) == 1 ) CloseFile (  1 );
		if ( FileExist(pSourceFile) == 1 ) 
		{
			OpenToRead ( 1, pSourceFile );
			while ( FileEnd(1) == 0 )
			{
				LPSTR pLine = ReadString (1);
				LPSTR pToken = "// DOCDOC: ";
				LPSTR pDocDocLine = strstr ( pLine, pToken );
				if ( pDocDocLine != NULL )
				{
					cstr sRestOfLine = cstr(pDocDocLine+strlen(pToken));
					if ( strlen( sRestOfLine.Get() ) > 6 )
					{
						// advance past token and collect rest as valid help line
						pHelpItems.push_back ( sRestOfLine );
					}
				}
			}
			CloseFile ( 1 );
		}

		// save new help file in DOCS folder
		LPSTR pHelpFile = "..\\Docs\\SETUP INI Description.txt";
		if ( FileExist ( pHelpFile ) == 1) DeleteFileA ( pHelpFile );
		OpenToWrite ( 1, pHelpFile );
		WriteString ( 1, "SETUP.INI Field Descriptions" );
		WriteString ( 1, "============================" );
		WriteString ( 1, "" );
		for ( int n = 0; n < pHelpItems.size(); n++ )
		{
			WriteString ( 1, pHelpItems[n].Get() );
		}
		CloseFile ( 1 );
	}
}

UINT GetURLData ( LPSTR pDataReturned, DWORD* pReturnDataSize, LPSTR urlWhere )
{
	UINT iError = 0;
	unsigned int dwDataLength = 0;
	HINTERNET m_hInet = InternetOpenA( "InternetConnection", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0 );
	if ( m_hInet == NULL )
	{
		iError = GetLastError( );
	}
	else
	{
		unsigned short wHTTPType = INTERNET_DEFAULT_HTTPS_PORT;
		HINTERNET m_hInetConnect = InternetConnectA( m_hInet, "www.thegamecreators.com", wHTTPType, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0 );
		if ( m_hInetConnect == NULL )
		{
			iError = GetLastError( );
		}
		else
		{
			int m_iTimeout = 2000;
			InternetSetOption( m_hInetConnect, INTERNET_OPTION_CONNECT_TIMEOUT, (void*)&m_iTimeout, sizeof(m_iTimeout) );  
			HINTERNET hHttpRequest = HttpOpenRequestA( m_hInetConnect, "GET", urlWhere, "HTTP/1.1", NULL, NULL, INTERNET_FLAG_IGNORE_CERT_CN_INVALID | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_SECURE, 0 );
			if ( hHttpRequest == NULL )
			{
				iError = GetLastError( );
			}
			else
			{
				HttpAddRequestHeadersA( hHttpRequest, "Content-Type: application/x-www-form-urlencoded", -1, HTTP_ADDREQ_FLAG_ADD | HTTP_ADDREQ_FLAG_REPLACE );
				int bSendResult = 0;
				bSendResult = HttpSendRequest( hHttpRequest, NULL, -1, NULL, 0 );//(void*)(m_szPostData), strlen(m_szPostData) );
				if ( bSendResult == 0 )
				{
					iError = GetLastError( );
				}
				else
				{
					int m_iStatusCode = 0;
					char m_szContentType[150];
					unsigned int dwBufferSize = sizeof(int);
					unsigned int dwHeaderIndex = 0;
					HttpQueryInfo( hHttpRequest, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, (void*)&m_iStatusCode, (LPDWORD)&dwBufferSize, (LPDWORD)&dwHeaderIndex );
					dwHeaderIndex = 0;
					unsigned int dwContentLength = 0;
					HttpQueryInfo( hHttpRequest, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, (void*)&dwContentLength, (LPDWORD)&dwBufferSize, (LPDWORD)&dwHeaderIndex );
					dwHeaderIndex = 0;
					unsigned int ContentTypeLength = 150;
					HttpQueryInfo( hHttpRequest, HTTP_QUERY_CONTENT_TYPE, (void*)m_szContentType, (LPDWORD)&ContentTypeLength, (LPDWORD)&dwHeaderIndex );
					char pBuffer[ 20000 ];
					for(;;)
					{
						unsigned int written = 0;
						if( !InternetReadFile( hHttpRequest, (void*) pBuffer, 2000, (LPDWORD)&written ) )
						{
							// error
						}
						if ( written == 0 ) break;
						if ( dwDataLength + written > 10240 ) written = 10240 - dwDataLength;
						memcpy( pDataReturned + dwDataLength, pBuffer, written );
						dwDataLength = dwDataLength + written;
						if ( dwDataLength >= 10240 ) break;
					}
					InternetCloseHandle( hHttpRequest );
				}
			}
			InternetCloseHandle( m_hInetConnect );
		}
		InternetCloseHandle( m_hInet );
	}
	if ( iError > 0 )
	{
		char *szError = 0;
		if ( iError > 12000 && iError < 12174 ) 
			FormatMessageA( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE, GetModuleHandleA("wininet.dll"), iError, 0, (char*)&szError, 0, 0 );
		else 
			FormatMessageA( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, iError, 0, (char*)&szError, 0, 0 );
		if ( szError )
		{
			LocalFree( szError );
		}
	}

	// complete
	*pReturnDataSize = dwDataLength;
	return iError;
}

void FPSC_VeryEarlySetup(void)
{
	// Determine if GAME or MAPEDITOR here, so can set display mode accordingly
	g.trueappname_s = "Guru-Game";
	// still called guru-mapeditor.exe for now
	if (strcmp(Lower(Right(Appname(), 18)), "guru-mapeditor.exe") == 0
	||  strcmp(Lower(Right(Appname(), 16)), "vr quest app.exe") == 0
	||  strcmp(Lower(Right(Appname(), 15)), "gamegurumax.exe") == 0)
	{
		g.trueappname_s = "Guru-MapEditor";
	}
}

void ReloadLensFlareImages (void)
{
	extern wiECS::Entity g_entitySunLight;
	wiScene::LightComponent* lightSun = wiScene::GetScene().lights.GetComponent(g_entitySunLight);
	int iFlareCount = 3;
	lightSun->lensFlareRimTextures.resize(iFlareCount);
	lightSun->lensFlareNames.resize(iFlareCount);
	for (int iFlareChain = 0; iFlareChain < iFlareCount; iFlareChain++)
	{
		std::string fileName;
		if (iFlareChain == 0) fileName = "lensflares\\flare1.jpg";
		if (iFlareChain == 1) fileName = "lensflares\\flare2.jpg";
		if (iFlareChain == 2) fileName = "lensflares\\flare3.jpg";
		WickedCall_DeleteImage(fileName);
		lightSun->lensFlareRimTextures[iFlareChain] = WickedCall_LoadImage(fileName, IMAGERES_LEVEL, false); // no streaming: lens-flare shader writes no mip feedback
		lightSun->lensFlareNames[iFlareChain] = fileName;
	}
}

