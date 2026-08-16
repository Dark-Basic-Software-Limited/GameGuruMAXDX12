void mapfile_savestandalone_stage4 ( void )
{
	// restore dir before proceeding
	SetDir(t.told_s.Get());

	//  CopyAFile (  collection to exe folder )
	t.filesmax = g.filecollectionmax;
	for ( t.fileindex = 1 ; t.fileindex <= t.filesmax; t.fileindex++ )
	{
		t.src_s=t.filecollection_s[t.fileindex];
		char pRealSrc[MAX_PATH];
		strcpy(pRealSrc, t.src_s.Get());
		GG_GetRealPath(pRealSrc, 0);
		if (FileExist(pRealSrc) == 1)
		{
			t.dest_s = t.exepath_s + t.exename_s + "\\Files\\" + t.src_s;
			if (FileExist(t.dest_s.Get()) == 1) DeleteAFile(t.dest_s.Get());
			CopyAFile(pRealSrc, t.dest_s.Get());
		}
	}

	//PE: Make sure needed folders exists.
	t.dest_s = t.exepath_s + t.exename_s + "\\Files\\particlesbank";
	if (PathExist(t.dest_s.Get()) == 0) MakeDirectory(t.dest_s.Get());
	t.dest_s = t.exepath_s + t.exename_s + "\\Files\\particlesbank\\user";
	if (PathExist(t.dest_s.Get()) == 0) MakeDirectory(t.dest_s.Get());


	//PE: Make sure we dont have a empty folder inside scriptbank.
	//PE: We do have needed empty folder so cant just run it on everything like 'levelbank' ...
	t.dest_s = t.exepath_s + t.exename_s + "\\Files\\scriptbank";
	if (PathExist(t.dest_s.Get()))
	{
		//PE: Check if we have any empty folders here.
		removeEmptyFolders(t.dest_s.Get());
	}

	// switch to original root to copy exe files and dependencies
	SetDir ( g.originalrootdir_s.Get() );

	//  Copy game engine and rename it
	t.dest_s=t.exepath_s+t.exename_s+"\\"+t.exename_s+".exe";
	if ( FileExist(t.dest_s.Get()) == 1 ) DeleteAFile (  t.dest_s.Get() );
	if ( FileExist ( "GameGuruMAX.exe") == 1)
		CopyAFile ( "GameGuruMAX.exe", t.dest_s.Get());
	else
		CopyAFile ( "Guru-MapEditor.exe", t.dest_s.Get() );

	// and change the icon using project icons if exists
	char projectico[MAX_PATH];
	char projectfinal_ico[MAX_PATH];
	strcpy(projectico, "projectbank\\");
	strcat(projectico, Storyboard.gamename);
	strcpy(projectfinal_ico, "Files\\");
	strcat(projectfinal_ico, projectico);
	strcat(projectfinal_ico, "\\project256.ico");
	GG_GetRealPath(projectfinal_ico, 1);
	if (FileExist(projectfinal_ico)==1)
	{
		void InjectIconToExe(char* icon, char* exe, int intresourcenumber);
		InjectIconToExe(projectfinal_ico, t.dest_s.Get(), 1);
	}

	// AssImp DLL tied to executable, may also need it for non-DBO model loading?!
	char pCritDLLFilename[MAX_PATH];
	strcpy(pCritDLLFilename, "assimp.dll");
	t.dest_s = t.exepath_s + t.exename_s + "\\" + pCritDLLFilename;
	if ( FileExist(t.dest_s.Get()) == 1 ) DeleteAFile ( t.dest_s.Get() );
	CopyAFile ( pCritDLLFilename, t.dest_s.Get() );

	// Steam DLL now required for authentication step (not needed for standalone game running)
	strcpy(pCritDLLFilename, "steam_api64.dll");
	t.dest_s = t.exepath_s + t.exename_s + "\\" + pCritDLLFilename;
	if (FileExist(t.dest_s.Get()) == 1) DeleteAFile (t.dest_s.Get());
	CopyAFile (pCritDLLFilename, t.dest_s.Get());

	// EPIC DLL required for authentication step (not needed for standalone game running)
	strcpy(pCritDLLFilename, "EOSSDK-Win64-Shipping.dll");
	t.dest_s = t.exepath_s + t.exename_s + "\\" + pCritDLLFilename;
	if (FileExist(t.dest_s.Get()) == 1) DeleteAFile (t.dest_s.Get());
	CopyAFile (pCritDLLFilename, t.dest_s.Get());	

	// Users report that people who don't have Max installed cannot play standalones
	strcpy(pCritDLLFilename, "dxil.dll");
	t.dest_s = t.exepath_s + t.exename_s + "\\" + pCritDLLFilename;
	if (FileExist(t.dest_s.Get()) == 1) DeleteAFile(t.dest_s.Get());
	CopyAFile(pCritDLLFilename, t.dest_s.Get());
	strcpy(pCritDLLFilename, "dxcompiler.dll");
	t.dest_s = t.exepath_s + t.exename_s + "\\" + pCritDLLFilename;
	if (FileExist(t.dest_s.Get()) == 1) DeleteAFile(t.dest_s.Get());
	CopyAFile(pCritDLLFilename, t.dest_s.Get());
	strcpy(pCritDLLFilename, "d3dcompiler_47.dll");
	t.dest_s = t.exepath_s + t.exename_s + "\\" + pCritDLLFilename;
	if (FileExist(t.dest_s.Get()) == 1) DeleteAFile(t.dest_s.Get());
	CopyAFile(pCritDLLFilename, t.dest_s.Get());

	// if exist, copy the OptickCore.dll so we can performance tune!
	strcpy(pCritDLLFilename, "OptickCore.dll");
	t.dest_s = t.exepath_s + t.exename_s + "\\" + pCritDLLFilename;
	if (FileExist(pCritDLLFilename) == 1) CopyAFile(pCritDLLFilename, t.dest_s.Get());

	// for wicked, create fonts and shaders folder
	cstr destExeRoot_s = t.exepath_s + t.exename_s;
	//SetDir ( destExeRoot_s.Get() );
	//if (PathExist("fonts") == 0) MakeDirectory("fonts");
	SetDir ( destExeRoot_s.Get() );
	if (PathExist("shaders") == 0) MakeDirectory("shaders");

	// for wicked, copy shaders folder
	SetDir ( g.originalrootdir_s.Get() );
	SetDir("shaders");
	ChecklistForFiles();
	for ( int c = 1; c <= ChecklistQuantity(); c++ )
	{
		LPSTR pShaderFile = ChecklistString(c);
		if (stricmp(pShaderFile, ".") != NULL && stricmp(pShaderFile, "..") != NULL)
		{
			t.dest_s = t.exepath_s + t.exename_s + "\\shaders\\" + pShaderFile;
			if (FileExist(t.dest_s.Get()) == 1) DeleteAFile(t.dest_s.Get());
			CopyAFile(pShaderFile, t.dest_s.Get());
		}
	}

	// GGMAX 2.69: the loop above lists loose FILES only, so the ffx-fsr2 SUBFOLDER
	// (FSR2 upscaler .cso/.wishadermeta) never shipped — standalones spewed
	// "shader compile FAILED" over the init screen trying to rebuild from dev paths
	SetDir ( g.originalrootdir_s.Get() );
	if ( PathExist("shaders\\ffx-fsr2") == 1 )
	{
		t.dest_s = t.exepath_s + t.exename_s + "\\shaders\\ffx-fsr2";
		if (PathExist(t.dest_s.Get()) == 0) MakeDirectory(t.dest_s.Get());
		SetDir("shaders\\ffx-fsr2");
		ChecklistForFiles();
		for ( int c = 1; c <= ChecklistQuantity(); c++ )
		{
			LPSTR pShaderFile = ChecklistString(c);
			if (stricmp(pShaderFile, ".") != NULL && stricmp(pShaderFile, "..") != NULL)
			{
				t.dest_s = t.exepath_s + t.exename_s + "\\shaders\\ffx-fsr2\\" + pShaderFile;
				if (FileExist(t.dest_s.Get()) == 1) DeleteAFile(t.dest_s.Get());
				CopyAFile(pShaderFile, t.dest_s.Get());
			}
		}
	}

	// restore to original folder
	SetDir ( g.originalrootdir_s.Get() );

	// GGMAX 2.69: ship the engine splash next to the standalone exe — without a
	// splash_screen.png Wicked renders the raw init BACKLOG (debug log text) while
	// the engine initializes; with it, the player sees the same quiet gradient the
	// editor shows
	if ( FileExist("splash_screen.png") == 1 )
	{
		t.dest_s = t.exepath_s + t.exename_s + "\\splash_screen.png";
		if (FileExist(t.dest_s.Get()) == 1) DeleteAFile(t.dest_s.Get());
		CopyAFile("splash_screen.png", t.dest_s.Get());
	}

	// Copy steam files (see above)
	 // No Steam in Photon build

	// copy visuals settings file
	t.visuals=t.gamevisuals ; visuals_save ( );
	
	// if visuals exists, switch to root folder and save to executable folder
	if ( FileExist("visuals.ini") == 1 ) 
	{
		SetDir (  g.fpscrootdir_s.Get() ); // odd this, but already set dir to root further up!
		char pSrcVisFile[MAX_PATH];
		strcpy(pSrcVisFile, "visuals.ini");
		t.dest_s=t.exepath_s+t.exename_s+"\\visuals.ini";

		char pRealVisFile[MAX_PATH];
		strcpy(pRealVisFile, t.dest_s.Get());
		GG_GetRealPath(pRealVisFile, 1);
		t.dest_s = pRealVisFile;
		char pRealSrcVisFile[MAX_PATH];
		strcpy(pRealSrcVisFile, pSrcVisFile);
		GG_GetRealPath(pRealSrcVisFile, 1);
		strcpy(pSrcVisFile, pRealSrcVisFile);

		if ( FileExist(t.dest_s.Get()) == 1 ) DeleteAFile ( t.dest_s.Get() );
		CopyAFile ( pSrcVisFile, t.dest_s.Get() );
	}
	t.visuals=t.editorvisuals  ; visuals_save ( );

	//  Create a setup.ini file here reflecting game
	Dim (  t.setuparr_s,999  );
	t.setupfile_s=t.exepath_s+t.exename_s+"\\setup.ini" ; t.i=0;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "[GAMERUN]" ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "vsync="+Str(g.gvsync) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "superflatterrain="+Str(t.terrain.superflat) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "smoothcamerakeys="+Str(g.globals.smoothcamerakeys) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "occlusionmode="+Str(g.globals.occlusionmode) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "occlusionsize="+Str(g.globals.occlusionsize) ; ++t.i;
	if ( g.vrqcontrolmode != 0 )
	{
		t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "hidelowfpswarning=1" ; ++t.i;
	}
	else
	{
		t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "hidelowfpswarning="+Str(g.globals.hidelowfpswarning) ; ++t.i;
	}
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "hardwareinfomode="+Str(g.ghardwareinfomode) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "fullscreen="+Str(g.gfullscreen) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "aspectratio="+Str(g.gaspectratio) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "dividetexturesize="+Str( g.gdividetexturesize ) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "producelogfiles=0"; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "adapterordinal="+Str( g.gadapterordinal ) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "adapterd3d11only="+Str( g.gadapterd3d11only ) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "hidedistantshadows=0"; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "invmouse="+Str( g.gminvert ) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "disablerightmousehold="+Str( g.gdisablerightmousehold ) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "profileinstandalone="+Str( 0 ) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "allowfragmentation="+Str( t.game.allowfragmentation ) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "pbroverride="+Str( g.gpbroverride ) ; ++t.i;

	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "memskipibr=" + Str(g.memskipibr); ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "underwatermode=" + Str(g.underwatermode); ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "usegrassbelowwater=" + Str(g.usegrassbelowwater); ++t.i;

	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "memskipwatermask=" + Str(g.memskipwatermask); ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "lowestnearcamera=" + Str(g.lowestnearcamera); ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "standalonefreememorybetweenlevels=" + Str(g.standalonefreememorybetweenlevels); ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "videoprecacheframes=" + Str(g.videoprecacheframes); ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "aidisabletreeobstacles=" + Str(g.aidisabletreeobstacles); ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "aidisableobstacles=" + Str(g.aidisableobstacles); ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "skipunusedtextures=" + Str(g.skipunusedtextures); ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "videodelayedload=" + Str(g.videodelayedload); ++t.i;

	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "maxtotalmeshlights=" + Str(g.maxtotalmeshlights); ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "maxpixelmeshlights=" + Str(g.maxpixelmeshlights); ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "terrainoldlight=" + Str(g.terrainoldlight); ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "terrainusevertexlights=" + Str(g.terrainusevertexlights); ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "maxterrainlights=" + Str(g.maxterrainlights); ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "terrainlightfadedistance=" + Str(g.terrainlightfadedistance); ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "showstaticlightinrealtime=" + Str(g.showstaticlightinrealtime); ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "drawcalloptimizer=" + Str(g.globals.drawcalloptimizer); ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "forcenowaterreflection=" + Str(g.globals.forcenowaterreflection); ++t.i;
	
	
	if ( t.DisableDynamicRes == false )
	{
		t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "disabledynamicres="+Str( 0 ) ; ++t.i;
	}
	else
	{
		t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "disabledynamicres="+Str( 1 ) ; ++t.i;
	}

	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "curvedistancescaler=" + Str(g.globals.CurveDistanceScaler); ++t.i;

	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "realshadowdistance=" + Str(g.globals.realshadowdistancehigh); ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "realshadowresolution="+Str(g.globals.realshadowresolution) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "realshadowcascadecount="+Str(g.globals.realshadowcascadecount) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "flashlightshadows=" + Str(g.globals.flashlightshadows); ++t.i;
	
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "realshadowcascade0="+Str(g.globals.realshadowcascade[0]) ; ++t.i;
	if (g.globals.realshadowsize[0] > 0) {
		t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "realshadowsize0=" + Str(g.globals.realshadowsize[0]); ++t.i;	
	}
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "realshadowcascade1="+Str(g.globals.realshadowcascade[1]) ; ++t.i;
	if (g.globals.realshadowsize[1] > 0) {
		t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "realshadowsize1=" + Str(g.globals.realshadowsize[1]); ++t.i;
	}
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "realshadowcascade2="+Str(g.globals.realshadowcascade[2]) ; ++t.i;
	if (g.globals.realshadowsize[2] > 0) {
		t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "realshadowsize2=" + Str(g.globals.realshadowsize[2]); ++t.i;
	}
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "realshadowcascade3="+Str(g.globals.realshadowcascade[3]) ; ++t.i;
	if (g.globals.realshadowsize[3] > 0) {
		t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "realshadowsize3=" + Str(g.globals.realshadowsize[3]); ++t.i;
	}
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "realshadowcascade4="+Str(g.globals.realshadowcascade[4]) ; ++t.i;
	if (g.globals.realshadowsize[4] > 0) {
		t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "realshadowsize4=" + Str(g.globals.realshadowsize[4]); ++t.i;
	}
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "realshadowcascade5="+Str(g.globals.realshadowcascade[5]) ; ++t.i;
	if (g.globals.realshadowsize[5] > 0) {
		t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "realshadowsize5=" + Str(g.globals.realshadowsize[5]); ++t.i;
	}
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "realshadowcascade6="+Str(g.globals.realshadowcascade[6]) ; ++t.i;
	if (g.globals.realshadowsize[6] > 0) {
		t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "realshadowsize6=" + Str(g.globals.realshadowsize[6]); ++t.i;
	}
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "realshadowcascade7="+Str(g.globals.realshadowcascade[7]) ; ++t.i;
	if (g.globals.realshadowsize[7] > 0) {
		t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "realshadowsize7=" + Str(g.globals.realshadowsize[7]); ++t.i;
	}

	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "enableplrspeedmods=" + Str(g.globals.enableplrspeedmods); ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "disableweaponjams=" + Str(g.globals.disableweaponjams); ++t.i;
	//PE: Add new setup.ini functions.
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "ConvertToDDS=1"; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "ConvertToDDSMaxsize=2048"; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "uselodobjects=1"; ++t.i;

	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "" ; ++t.i;

	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "[GAMEMENUOPTIONS]" ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "graphicslowterrain="+g.graphicslowterrain_s; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "graphicslowentity="+g.graphicslowentity_s; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "graphicslowgrass="+g.graphicslowgrass_s; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "graphicsmediumterrain="+g.graphicsmediumterrain_s; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "graphicsmediumentity="+g.graphicsmediumentity_s; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "graphicsmediumgrass="+g.graphicsmediumgrass_s; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "graphicshighterrain="+g.graphicshighterrain_s; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "graphicshighentity="+g.graphicshighentity_s; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "graphicshighgrass="+g.graphicshighgrass_s; ++t.i;

	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "" ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "[CUSTOMIZATIONS]" ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "switchtoalt="+Str(g.ggunaltswapkey1) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "melee key=" + Str(g.ggunmeleekey); ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "zoomholdbreath="+Str(g.gzoomholdbreath) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "keyUP="+Str(t.listkey[1]) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "keyDOWN="+Str(t.listkey[2]) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "keyLEFT="+Str(t.listkey[3]) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "keyRIGHT="+Str(t.listkey[4]) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "keyJUMP="+Str(t.listkey[5]) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "keyCROUCH="+Str(t.listkey[6]) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "keyENTER="+Str(t.listkey[7]) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "keyRELOAD="+Str(t.listkey[8]) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "keyPEEKLEFT="+Str(t.listkey[9]) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "keyPEEKRIGHT="+Str(t.listkey[10]) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "keyRUN="+Str(t.listkey[11]) ; ++t.i;

	// vr extras
	if ( g.vrqcontrolmode != 0 || g.gxbox != 0 )
	{
		// CONTROLLER
		t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "" ; ++t.i;
		t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "[CONTROLLER]" ; ++t.i;
		if ( g.vrqcontrolmode != 0 )
		{
			if ( g.vrqorggcontrolmode == 2 )
			{
				// No controller by default in EDU mode
				t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "xbox=0"; ++t.i;
				t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "xboxcontrollertype=2"; ++t.i;
				t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "xboxinvert=0" ; ++t.i;
				t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "xboxmag=100" ; ++t.i;
			}
			else
			{
				//PE: Could not get standalone working , until i see xbox=1 , should it not be based on original setup.ini g.gxbox ?
				t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "xbox=";+Str(g.gxbox); ++t.i;
				t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "xboxcontrollertype=2"; ++t.i;
				t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "xboxinvert=0" ; ++t.i;
				t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "xboxmag=100" ; ++t.i;
			}
		}
		else
		{
			t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "xbox="+Str(g.gxbox); ++t.i;
			t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "xboxcontrollertype="+Str(g.gxboxcontrollertype); ++t.i;
			t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "xboxinvert="+Str(g.gxboxinvert) ; ++t.i;
			t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "xboxmag="+Str(g.gxboxmag) ; ++t.i;
		}
	}

	if (  FileExist(t.setupfile_s.Get()) == 1  )  DeleteAFile (  t.setupfile_s.Get() );
	SaveArray (  t.setupfile_s.Get(),t.setuparr_s );
	UnDim (  t.setuparr_s );

	// separate VR setup file
	extern bool g_bStandaloneVRMode;
	if (g_bStandaloneVRMode == true)
	{
		Dim (t.setuparr_s, 999);
		t.setupfile_s = t.exepath_s + t.exename_s + "\\setupvr.ini"; t.i = 0;
		t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "[VR]"; ++t.i;
		t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "vrmode=3"; ++t.i;
		t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "vrmodemag=100"; ++t.i;
		t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "vroffsetangx=" + Str(g.gvroffsetangx); ++t.i;
		t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "vrwmroffsetangx=" + Str(g.gvrwmroffsetangx); ++t.i;
		if (FileExist(t.setupfile_s.Get()) == 1)  DeleteAFile (t.setupfile_s.Get());
		SaveArray (t.setupfile_s.Get(), t.setuparr_s);
		UnDim (t.setuparr_s);
	}

	//  Also save out the localisation ptr file
	Dim (  t.setuparr_s,2  );
	t.setupfile_s=t.exepath_s+t.exename_s+"\\userdetails.ini";
	t.setuparr_s[0]="[LOCALIZATION]";
	t.setuparr_s[1]=cstr("language=")+g.language_s;
	SaveArray (  t.setupfile_s.Get(),t.setuparr_s );
	UnDim (  t.setuparr_s );

	//  Itinery of all files in standalone
	t.titineryfile_s=t.exepath_s+t.exename_s+"\\contents.txt";
	SaveArray (  t.titineryfile_s.Get(),t.filecollection_s );

	//  cleanup file array
	UnDim (  t.filecollection_s );
}

int mapfile_savestandalone_continue ( void )
{
	int iSuccess = 0;
	switch ( g_mapfile_iStage )
	{
		case 1 :	g_mapfile_fProgress+=0.1f; 
					if ( g_mapfile_fProgress >= 5.0f )
						g_mapfile_iStage = 20;
					break;

		case 20 :	mapfile_savestandalone_stage2a();
					g_mapfile_fProgress = 6.0f; 
					g_mapfile_iStage = 21;
					break;

		case 21 :	if ( mapfile_savestandalone_stage2b() == 0 )
					{
						g_mapfile_iStage = 22;
					}
					else
					{
						g_mapfile_iStage = 29;
					}
					break;

		case 22: 
		{
			if (mapfile_savestandalone_stage2c() == 0)
			{
				g_mapfile_fProgress += (80.0f / g_mapfile_fProgressSpan);
				if (g_mapfile_fProgress > 84) g_mapfile_fProgress = 84;
			}
			else
			{
				g_mapfile_iStage = 23;
			}
			break;
		}

		case 23 :	mapfile_savestandalone_stage2d();
					g_mapfile_iStage = 21; 
					break;

		case 29 :	mapfile_savestandalone_stage2e(); 
					g_mapfile_iStage = 30; 
					break;

		case 30 :	mapfile_savestandalone_stage3(); 
					g_mapfile_fProgress = 90.0f; 
					g_mapfile_iStage = 40; 
					break;

		case 40 :	mapfile_savestandalone_stage4(); 
					g_mapfile_fProgress = 95.0f; 
					g_mapfile_iStage = 50; 
					break;

		case 50 :	g_mapfile_fProgress+=0.1f; 
					if ( g_mapfile_fProgress >= 100.0f )
						g_mapfile_iStage = 60;
					break;

		case 60 :	g_mapfile_fProgress = 100.0f; 
					g_mapfile_iStage = 99; 
					iSuccess = 1;
					break;
	}
	return iSuccess;
}

float mapfile_savestandalone_getprogress ( void )
{
	return g_mapfile_fProgress;
}

void mapfile_savestandalone_finish ( void )
{
	// completed exclusive use of filecollection array (some remote project code would have tried to reuse this array)
	g_bMakingAStandaloneUsingFileCollectionArray = false;

	// encrypt media
	t.dest_s=t.exepath_s+t.exename_s;
	if ( g.gexportassets == 0 ) 
	{
		if ( PathExist( cstr(t.dest_s + "\\Files").Get() ) ) 
		{
			//  NOTE; Need to exclude lightmaps from encryptor  set encrypt ignore list "lightmaps"
			EncryptAllFiles ( cstr(t.dest_s + "\\Files").Get() );
		}
	}

	//  if not tignorelevelbankfiles, copy unencrypted files
	if (  t.tignorelevelbankfiles == 0 ) 
	{
		//  now copy the files we do not want to encrypt
		g.filecollectionmax = 0;
		Dim (  t.filecollection_s,500  );

		//  lightmap DBOs
		SetDir (  cstr(g.fpscrootdir_s+"\\Files\\").Get() );
		t.tfurthestobjnumber=g.lightmappedobjectoffset;
		for ( t.tobj = g.lightmappedobjectoffset; t.tobj<= g.lightmappedobjectoffset+99999 ; t.tobj+= 100 )
		{
			t.tname_s = ""; t.tname_s = t.tname_s + "levelbank\\testmap\\lightmaps\\"+"object"+Str(t.tobj)+".dbo";
			if (  FileExist(t.tname_s.Get()) == 1  )  t.tfurthestobjnumber = t.tobj+100;
		}
		for ( t.tobj = g.lightmappedobjectoffset; t.tobj <= t.tfurthestobjnumber; t.tobj++ )
		{
			t.tname_s = ""; t.tname_s = t.tname_s + "levelbank\\testmap\\lightmaps\\"+"object"+Str(t.tobj)+".dbo";
			if (  FileExist(t.tname_s.Get()) == 1  )  addtocollection(t.tname_s.Get());
		}
		for ( t.tobj = g.lightmappedobjectoffset; t.tobj<= g.lightmappedobjectoffsetfinish; t.tobj++ )
		{
			t.tname_s = ""; t.tname_s = t.tname_s + "levelbank\\testmap\\lightmaps\\"+"object"+Str(t.tobj)+".dbo";
			if (  FileExist(t.tname_s.Get()) == 1  )  addtocollection(t.tname_s.Get());
		}
		t.nmax=500;
		for ( t.n = 0 ; t.n<=  5000 ; t.n+= 100 )
		{
			t.tfile_s=cstr("levelbank\\testmap\\lightmaps\\")+Str(t.n)+".dds";
			if (  FileExist(t.tfile_s.Get()) == 1  )  t.nmax = t.n+100;
		}
		for ( t.n = 0 ; t.n<=  t.nmax; t.n++ )
		{
			t.tfile_s=cstr("levelbank\\testmap\\lightmaps\\")+Str(t.n)+".dds";
			if (  FileExist(t.tfile_s.Get()) == 1  )  addtocollection(t.tfile_s.Get());
		}
		t.tfile_s="levelbank\\testmap\\lightmaps\\objectlist.dat" ; addtocollection(t.tfile_s.Get());
		t.tfile_s="levelbank\\testmap\\lightmaps\\objectnummax.dat" ; addtocollection(t.tfile_s.Get());

		//  Copy the 'unencrypted files' collection to exe folder
		timestampactivity(0, cstr(cstr("filecollectionmax=")+Str(g.filecollectionmax)).Get() );
		SetDir ( cstr(t.exepath_s+t.exename_s+"\\Files\\levelbank\\testmap").Get() );
		if (  PathExist("lightmaps") == 0  )  MakeDirectory (  "lightmaps" );
		SetDir (  cstr(g.fpscrootdir_s+"\\Files\\").Get() );
		for ( t.fileindex = 1 ; t.fileindex <= g.filecollectionmax; t.fileindex++ )
		{
			t.src_s=t.filecollection_s[t.fileindex];
			if (  FileExist(t.src_s.Get()) == 1 ) 
			{
				t.dest_s=t.exepath_s+t.exename_s+"\\Files\\"+t.src_s;
				if (  FileExist(t.dest_s.Get()) == 1  )  DeleteAFile (  t.dest_s.Get() );
				CopyAFile (  t.src_s.Get(),t.dest_s.Get() );
			}
		}
	}

	// restore directory, restore original level and close up
	mapfile_savestandalone_restoreandclose();
}

void mapfile_savestandalone_restoreandclose ( void )
{
	// Restore directory
	SetDir ( t.told_s.Get() );

	// restore original level FPM files
	timestampactivity(0, cstr(cstr("check '")+g.projectfilename_s+"' vs '"+t.tmasterlevelfile_s+"'").Get() );
	if ( g.projectfilename_s != t.tmasterlevelfile_s || restore_old_map )
	{
		restore_old_map = false;
		if ( Len(t.tmasterlevelfile_s.Get()) > 1 )
		{
			g.projectfilename_s=t.tmasterlevelfile_s;
			// need to load EVERYTHING back in
			gridedit_load_map ( );
		}
	}

	// no longer making standalone
	t.levelsforstandalone = 0;

	// no longer exclusive use of file collection array
	g_bMakingAStandaloneUsingFileCollectionArray = false;
}

void scanscriptfileandaddtocollection ( char* tfile_s , char *pPath)
{
	cstr tscriptname_s =  "";
	cstr tlinethis_s =  "";
	int lookforlen = 0;
	cstr lookfor_s =  "";
	int lookforlen2 = 0;
	cstr lookfor2_s = "";
	cstr lookfor3_s = "";
	cstr tline_s =  "";
	int l = 0;
	int c = 0;
	int tt = 0;
	std::vector <cstr> scriptpage_s; //Allow us to run recursively
	Dim (  scriptpage_s,10000  );
	if (  FileExist(tfile_s) == 1 ) 
	{
		LoadArray (  tfile_s,scriptpage_s );

		lookfor_s=Lower("Include(") ; lookforlen=Len(lookfor_s.Get());
		lookfor2_s = Lower("require \""); lookforlen2 = Len(lookfor2_s.Get());
		lookfor3_s = Lower("Include (");

		for ( l = 0 ; l < scriptpage_s.size() ; l++ )
		{
			tline_s=Lower(scriptpage_s[l].Get());

			for (c = 0; c <= Len(tline_s.Get()) - lookforlen2 - 1; c++)
			{
				tlinethis_s = Right(tline_s.Get(), Len(tline_s.Get()) - c);

				// ignore commented out lines
				if (cstr(Left(tlinethis_s.Get(), 2)) == "--") break;

				if (cstr(Left(tlinethis_s.Get(), lookforlen2)) == lookfor2_s.Get() || cstr(Left(tlinethis_s.Get(), lookforlen2)) == lookfor3_s.Get())
				{
					//  found script has included ANOTHER script
					// skip spaces and quotes 
					int i = lookforlen2 + 1;

					while (i < Len(tlinethis_s.Get()) &&
						(cstr(Mid(tlinethis_s.Get(), i)) == " " ||
							cstr(Mid(tlinethis_s.Get(), i)) == "\""))
					{
						i++;
					};

					// if couldn't find the script name skip this line
					if (i == Len(tlinethis_s.Get())) break;

					tscriptname_s = Right(tline_s.Get(), Len(tline_s.Get()) - c - i + 1);

					for (int il = Len(tscriptname_s.Get()); il > 0; il--) {
						if (cstr(Mid(tscriptname_s.Get(), il)) == "\"") {
							tscriptname_s = Left(tscriptname_s.Get(), il-1);
							break;
						}
					}

					std::string script_name = tscriptname_s.Get();
					replaceAll(script_name, "\\\\", "\\");
					replaceAll(script_name, "scriptbank\\", "");
					tscriptname_s = script_name.c_str();

					if( !pestrcasestr(tscriptname_s.Get(),".lua"))
						tscriptname_s += ".lua";

					for (tt = Len(tscriptname_s.Get()); tt >= 4; tt += -1)
					{
						if (cstr(Mid(tscriptname_s.Get(), tt - 0)) == "a" && cstr(Mid(tscriptname_s.Get(), tt - 1)) == "u" && cstr(Mid(tscriptname_s.Get(), tt - 2)) == "l" && cstr(Mid(tscriptname_s.Get(), tt - 3)) == ".")
						{
							break;
						}
					}
					tscriptname_s = Left(tscriptname_s.Get(), tt);

					if (addtocollection(cstr(cstr("scriptbank\\") + tscriptname_s).Get()) == true) {
						//Newly added , also scan this entry.
						if (pPath)
						{
							scanscriptfileandaddtocollection(cstr(cstr(pPath)+cstr("scriptbank\\") + tscriptname_s).Get(), pPath);
						}
						else
							scanscriptfileandaddtocollection(cstr(cstr("scriptbank\\") + tscriptname_s).Get());
					}
				}
			}

			for ( c = 0 ; c<=  Len(tline_s.Get())-lookforlen-1; c++ )
			{
				tlinethis_s=Right(tline_s.Get(),Len(tline_s.Get())-c);

				// ignore commented out lines
				if ( cstr( Left( tlinethis_s.Get(), 2 )) == "--" ) break;

				if (  cstr( Left( tlinethis_s.Get(), lookforlen )) == lookfor_s.Get() )
				{
					//  found script has included ANOTHER script
					// skip spaces and quotes 
					int i = lookforlen + 1;

					while ( i < Len( tlinethis_s.Get() ) &&
						   ( cstr( Mid( tlinethis_s.Get(), i )) == " " ||
						     cstr( Mid( tlinethis_s.Get(), i )) == "\"" ) ) 
					{
						i++;
					};
			
					// if couldn't find the script name skip this line
					if (i == Len(tlinethis_s.Get())) break;

					tscriptname_s=Right(tline_s.Get(),Len(tline_s.Get())-c-i+1);
					for (tt = Len(tscriptname_s.Get()); tt >= 4; tt += -1)
					{
						if (cstr(Mid(tscriptname_s.Get(), tt - 0)) == "a" && cstr(Mid(tscriptname_s.Get(), tt - 1)) == "u" && cstr(Mid(tscriptname_s.Get(), tt - 2)) == "l" && cstr(Mid(tscriptname_s.Get(), tt - 3)) == ".")
						{
							break;
						}
					}
					tscriptname_s = Left(tscriptname_s.Get(), tt);

					if (addtocollection(cstr(cstr("scriptbank\\") + tscriptname_s).Get()) == true) {
						//Newly added , also scan this entry.
						if (pPath)
						{
							scanscriptfileandaddtocollection(cstr(cstr(pPath) + cstr("scriptbank\\") + tscriptname_s).Get(), pPath);
						}
							scanscriptfileandaddtocollection(cstr(cstr("scriptbank\\") + tscriptname_s).Get());
					}
				}
			}
		}
	}
	UnDim (  scriptpage_s );
}

bool addtocollection ( char* file_s )
{
	int tarrsize = 0;
	int tfound = 0;
	int f = 0;
	file_s=Lower(file_s);
	//  Ensure this entry is not already present
	tfound=0;
	for ( f = 1 ; f<=  g.filecollectionmax; f++ )
	{
		if (  t.filecollection_s[f] == cstr(file_s)  )  tfound = 1;
	}
	if (  tfound == 0 ) 
	{
		//  Expand file collection array if nearly full
		++g.filecollectionmax;
		tarrsize=ArrayCount(t.filecollection_s);
		if (  g.filecollectionmax>tarrsize-10 ) 
		{
			Dim (  t.filecollection_s,tarrsize+50  );
		}
		t.filecollection_s[g.filecollectionmax]=file_s;
		return true;
	}
	return false;
}

void removefromcollection ( char* file_s )
{
	int tfound = 0;
	file_s=Lower(file_s);
	for ( int f = 1 ; f <= g.filecollectionmax; f++ )
		if ( t.filecollection_s[f] == cstr(file_s)  )  
			tfound = f;
	if ( tfound > 0 ) 
	{
		// remove from consideration
		t.filecollection_s[tfound] = "";
	}
}

void removeanymatchingfromcollection ( char* folderorfile_s )
{
	int tfound = 0;
	folderorfile_s = Lower(folderorfile_s);
	for ( int f = 1; f <= g.filecollectionmax; f++ )
	{
		if ( strnicmp ( t.filecollection_s[f].Get(), folderorfile_s, strlen(folderorfile_s) ) == NULL )
		{
			// remove from consideration
			t.filecollection_s[f] = "";
		}
	}
}

bool g_bNormalOperations = true;

void addfoldertocollection ( char* path_s )
{
	cstr tfile_s =  "";
	cstr told_s =  "";
	cstr usePath = path_s;
	int c = 0;
	told_s = GetDir();

	//PE: Some docwrite folders have additional files that need to go into standalone.
	bool bAddAdditionalFilesFromDocWrite = false;
	if (pestrcasestr(path_s, "gamecore\\decals"))
		bAddAdditionalFilesFromDocWrite = true;
	if (pestrcasestr(path_s, "gamecore\\hands\\Animations"))
		bAddAdditionalFilesFromDocWrite = true;

	//PE: In wicked also check if folder is inside docwrite.
	if (!PathExist(usePath.Get()))
	{
		if (g_bNormalOperations == true)
		{
			extern char szWriteDir[MAX_PATH];
			cstr testPath = cstr(szWriteDir) + "Files\\" + path_s;// usePath;
			if (PathExist(testPath.Get()))
			{
				usePath = testPath;
			}
		}
	}
	if ( PathExist (usePath.Get()) )
	{
		SetDir (usePath.Get());
		ChecklistForFiles (  );
		if (ChecklistQuantity() <= 2)
		{
			// Try writable folder instead (sometimes, there will be an empty user folder in the max install, which causes this process to ignore the writable user folder!)
			if (g_bNormalOperations == true)
			{
				extern char szWriteDir[MAX_PATH];
				cstr testPath = cstr(szWriteDir) + "Files\\" + path_s;// usePath;
				SetDir(told_s.Get());
				if (PathExist(testPath.Get()))
				{
					SetDir(testPath.Get());
					ChecklistForFiles();
				}
			}
		}
		for ( c = 1 ; c<=  ChecklistQuantity(); c++ )
		{
			if (  ChecklistValueA(c) == 0 ) 
			{
				tfile_s=ChecklistString(c);
				if (  tfile_s != "." && tfile_s != ".." ) 
				{
					//PE: Still adding using the relative path path_s
					addtocollection( cstr(cstr(path_s)+"\\"+tfile_s).Get() );
				}
			}
		}
		SetDir (  told_s.Get() );
	}
	else
	{
		//timestampactivity(0, cstr(cstr("Tried adding path that does not exist: ") + path_s).Get());
		char pDebugPath[10240];
		sprintf(pDebugPath, "Tried adding path '%s' that does not exist here: %s", path_s, usePath.Get());
		timestampactivity(0, pDebugPath);
	}

	//PE: Add additional files from docwrite folder.
	if(bAddAdditionalFilesFromDocWrite)
	{
		if (g_bNormalOperations == true)
		{
			extern char szWriteDir[MAX_PATH];
			cstr testPath = cstr(szWriteDir) + "Files\\" + path_s;// usePath;
			if (PathExist(testPath.Get()))
			{
				usePath = testPath;
				SetDir(usePath.Get());
				ChecklistForFiles();
				for (c = 1; c <= ChecklistQuantity(); c++)
				{
					if (ChecklistValueA(c) == 0)
					{
						tfile_s = ChecklistString(c);
						if (tfile_s != "." && tfile_s != "..")
						{
							//PE: Still adding using the relative path path_s
							addtocollection(cstr(cstr(path_s) + "\\" + tfile_s).Get());
						}
					}
				}
				SetDir(told_s.Get());
			}
		}
	}
}

void addallinfoldertocollection ( cstr subThisFolder_s, cstr subFolder_s )
{
	// could be nesteds folder path passed in
	cstr olddir = "";
	if (subThisFolder_s.Len() > 0)
	{
		olddir = GetDir();
		SetDir (subThisFolder_s.Get());
	}

	// first scan and record all files and folders - store folders locally
	ChecklistForFiles();
	int iFoldersCount = ChecklistQuantity();
	cstr* pFolders = new cstr[iFoldersCount+1];
	for ( int c = 1; c <= iFoldersCount; c++ )
	{
		pFolders[c] = "";
		LPSTR pFileFolderName = ChecklistString(c);
		if ( strcmp ( pFileFolderName, "." ) != NULL && strcmp ( pFileFolderName, ".." ) !=NULL )
		{
			if ( ChecklistValueA(c) == 1 )
			{
				pFolders[c] = pFileFolderName;
			}
			else
			{
				cstr relativeFilePath_s = subFolder_s + "\\"; // subFolder_s always populated with the first folder of the recursive traverse
				relativeFilePath_s += pFileFolderName;
				addtocollection ( relativeFilePath_s.Get() );
			}
		}
	}

	// now use local folder list and investigate each one
	for ( int f = 1; f <= iFoldersCount; f++ )
	{
		cstr pFolderName = pFolders[f];
		if ( pFolderName.Len() > 0 )
		{
			cstr relativeFolderPath_s = subFolder_s;
			if (strlen(subFolder_s.Get()) > 0) relativeFolderPath_s += "\\";
			relativeFolderPath_s += pFolderName;
			addallinfoldertocollection ( pFolderName, relativeFolderPath_s );
		}
	}

	// back out of folder (could be nesteds folder path passed in)
	if (olddir.Len() > 0)
	{
		SetDir(olddir.Get());
	}

	// finally free resources
	delete[] pFolders;
}

void createallfoldersincollection ( void )
{
	cstr pOldDir = GetDir();
	t.strwork = ""; t.strwork = t.strwork + "Create full path structure ("+Str(t.filesmax)+") for standalone executable";
	timestampactivity(0, t.strwork.Get() );
	t.filesmax = g.filecollectionmax;
	for ( t.fileindex = 1 ; t.fileindex <= t.filesmax; t.fileindex++ )
	{
		t.olddir_s=GetDir();
		t.src_s=t.filecollection_s[t.fileindex];
		t.srcstring_s = t.src_s;
		while (Len(t.srcstring_s.Get()) > 0)
		{
			for (t.c = 1; t.c <= Len(t.srcstring_s.Get()); t.c++)
			{
				if (cstr(Mid(t.srcstring_s.Get(), t.c)) == "\\" || cstr(Mid(t.srcstring_s.Get(), t.c)) == "/")
				{
					t.chunk_s = Left(t.srcstring_s.Get(), t.c - 1);
					if (Len(t.chunk_s.Get()) > 0)
					{
						if (PathExist(t.chunk_s.Get()) == 0)  MakeDirectory(t.chunk_s.Get());
						if (PathExist(t.chunk_s.Get()) == 0)
						{
							timestampactivity(0, cstr(cstr("Path:") + t.src_s).Get());
							timestampactivity(0, cstr(cstr("Unable to create folder:'") + t.chunk_s + "' [error code " + Mid(t.srcstring_s.Get(), t.c) + ":" + Str(t.c) + ":" + Str(Len(t.srcstring_s.Get())) + "]").Get());
						}
						if (PathExist(t.chunk_s.Get()) == 1)
						{
							// sometimes an absolute path can be inserted into path sequence (i.e lee\fred\d;\blob\doug)
							SetDir(t.chunk_s.Get());
						}
					}
					t.srcstring_s = Right(t.srcstring_s.Get(), Len(t.srcstring_s.Get()) - t.c);
					t.c = 1; // start from beginning as string has been cropped
					break;
				}
			}
			if (t.c > Len(t.srcstring_s.Get())) break;
		}

		SetDir ( t.olddir_s.Get() );
	}
	SetDir ( pOldDir.Get() );
}

void findalltexturesinmodelfile ( char* inputfile_s, char* folder_s, char* texpath_s )
{
	cstr returntexfile_s =  "";
	int tfoundpiccy = 0;
	cstr texfile_s =  "";
	int filesize = 0;
	int mbi = 0;
	int a = 0;
	int b = 0;
	int c = 0;
	int d = 0;

	// do two passes, second one if there is an accompanying DBO (which has unscrambled data to find texture names)
	for ( int iPass = 0; iPass < 2; iPass++ )
	{
		// filename to attempt a scan of
		cstr filestr = inputfile_s;
		if (iPass == 1)
		{
			LPSTR pFilename = filestr.Get();
			if (stricmp (pFilename + strlen(pFilename) - 2, ".x") == NULL)
			{
				filestr = cstr(Left(pFilename, strlen(pFilename) - 2)) + ".dbo";
			}
		}
		char* file_s = filestr.Get();

		// To determine if a model file requires texture files, we scan the file for a
		// match to the Text ( .TGA or .JPG (and use texfile$) )
		returntexfile_s="";
		if (FileExist(file_s) == 1)
		{
			filesize = FileSize(file_s);
			mbi = 255;
			OpenToRead (11, file_s);
			if (FileOpen(11) == 1)
			{
				MakeMemblockFromFile(mbi, 11);
				CloseFile(11);
				for (b = 0; b <= filesize - 4; b++)
				{
					if (ReadMemblockByte(mbi, b + 0) == Asc("."))
					{
						tfoundpiccy = 0;
						if (ReadMemblockByte(mbi, b + 1) == Asc("T") || ReadMemblockByte(mbi, b + 1) == Asc("t"))
						{
							if (ReadMemblockByte(mbi, b + 2) == Asc("G") || ReadMemblockByte(mbi, b + 2) == Asc("g"))
							{
								if (ReadMemblockByte(mbi, b + 3) == Asc("A") || ReadMemblockByte(mbi, b + 3) == Asc("a"))
								{
									tfoundpiccy = 1;
								}
							}
						}
						if (ReadMemblockByte(mbi, b + 1) == Asc("J") || ReadMemblockByte(mbi, b + 1) == Asc("j"))
						{
							if (ReadMemblockByte(mbi, b + 2) == Asc("P") || ReadMemblockByte(mbi, b + 2) == Asc("p"))
							{
								if (ReadMemblockByte(mbi, b + 3) == Asc("G") || ReadMemblockByte(mbi, b + 3) == Asc("g"))
								{
									tfoundpiccy = 1;
								}
							}
						}
						if (ReadMemblockByte(mbi, b + 1) == Asc("D") || ReadMemblockByte(mbi, b + 1) == Asc("d"))
						{
							if (ReadMemblockByte(mbi, b + 2) == Asc("D") || ReadMemblockByte(mbi, b + 2) == Asc("d"))
							{
								if (ReadMemblockByte(mbi, b + 3) == Asc("S") || ReadMemblockByte(mbi, b + 3) == Asc("s"))
								{
									tfoundpiccy = 1;
								}
							}
						}
						if (ReadMemblockByte(mbi, b + 1) == Asc("B") || ReadMemblockByte(mbi, b + 1) == Asc("b"))
						{
							if (ReadMemblockByte(mbi, b + 2) == Asc("M") || ReadMemblockByte(mbi, b + 2) == Asc("m"))
							{
								if (ReadMemblockByte(mbi, b + 3) == Asc("P") || ReadMemblockByte(mbi, b + 3) == Asc("p"))
								{
									tfoundpiccy = 1;
								}
							}
						}
						if (ReadMemblockByte(mbi, b + 1) == Asc("P") || ReadMemblockByte(mbi, b + 1) == Asc("p"))
						{
							if (ReadMemblockByte(mbi, b + 2) == Asc("N") || ReadMemblockByte(mbi, b + 2) == Asc("n"))
							{
								if (ReadMemblockByte(mbi, b + 3) == Asc("G") || ReadMemblockByte(mbi, b + 3) == Asc("g"))
								{
									tfoundpiccy = 1;
								}
							}
						}
						//PE: mainly from the building pack they are recorded as psd.
						if (ReadMemblockByte(mbi, b + 1) == Asc("P") || ReadMemblockByte(mbi, b + 1) == Asc("p"))
						{
							if (ReadMemblockByte(mbi, b + 2) == Asc("S") || ReadMemblockByte(mbi, b + 2) == Asc("s"))
							{
								if (ReadMemblockByte(mbi, b + 3) == Asc("D") || ReadMemblockByte(mbi, b + 3) == Asc("d"))
								{
									tfoundpiccy = 1;
								}
							}
						}
						if (tfoundpiccy == 1)
						{
							//  track back
							for (c = b; c >= b - 255; c += -1)
							{
								if (ReadMemblockByte(mbi, c) >= Asc(" ") && ReadMemblockByte(mbi, c) <= Asc("z") && ReadMemblockByte(mbi, c) != 34)
								{
									//  part of filename
								}
								else
								{
									//  no more filename
									break;
								}
							}
							texfile_s = "";
							for (d = c + 1; d <= b + 3; d++)
							{
								texfile_s = texfile_s + Chr(ReadMemblockByte(mbi, d));
							}
							texfile_s = Lower(texfile_s.Get());

							LPSTR pTextFilePtr = texfile_s.Get();
							if (strlen(pTextFilePtr) > 4)
							{
								// scan for telltail abs path
								bool bUsesAbsPath = false;
								for (int nn = 0; nn < strlen(pTextFilePtr); nn++)
								{
									if (pTextFilePtr[nn] == ':')
									{
										// this texture filename uses an absolutely path
										bUsesAbsPath = true;
										break;
									}
								}
								if (bUsesAbsPath == true )
								{
									// truncate to JUST the texture filename, remove all path (depend on folder_s)!
									for (int nnn = strlen(pTextFilePtr)-1; nnn > 0; nnn--)
									{
										if (pTextFilePtr[nnn] == '\\' || pTextFilePtr[nnn] == '/')
										{
											texfile_s = pTextFilePtr + nnn + 1;
											break;
										}
									}
								}
							}

							if (strnicmp(texfile_s.Get(), "effectbank\\", 11) == NULL)
							{
								addtocollection(texfile_s.Get());
							}
							else
							{
								// detect PBR texture set
								bool bDetectedPBRTextureSetName = false;
								cstr texfilenoext_s = cstr(Left(texfile_s.Get(), Len(texfile_s.Get()) - 4));
								if (strnicmp(texfilenoext_s.Get() + strlen(texfilenoext_s.Get()) - 6, "_color", 6) == NULL) { texfilenoext_s = Left(texfilenoext_s.Get(), strlen(texfilenoext_s.Get()) - 6); bDetectedPBRTextureSetName = true; }
								if (strnicmp(texfilenoext_s.Get() + strlen(texfilenoext_s.Get()) - 7, "_normal", 7) == NULL) { texfilenoext_s = Left(texfilenoext_s.Get(), strlen(texfilenoext_s.Get()) - 7); bDetectedPBRTextureSetName = true; }
								if (strnicmp(texfilenoext_s.Get() + strlen(texfilenoext_s.Get()) - 10, "_metalness", 10) == NULL) { texfilenoext_s = Left(texfilenoext_s.Get(), strlen(texfilenoext_s.Get()) - 10); bDetectedPBRTextureSetName = true; }
								if (strnicmp(texfilenoext_s.Get() + strlen(texfilenoext_s.Get()) - 10, "_roughness", 10) == NULL) { texfilenoext_s = Left(texfilenoext_s.Get(), strlen(texfilenoext_s.Get()) - 10); bDetectedPBRTextureSetName = true; }
								if (strnicmp(texfilenoext_s.Get() + strlen(texfilenoext_s.Get()) - 6, "_gloss", 6) == NULL) { texfilenoext_s = Left(texfilenoext_s.Get(), strlen(texfilenoext_s.Get()) - 6); bDetectedPBRTextureSetName = true; }
								if (strnicmp(texfilenoext_s.Get() + strlen(texfilenoext_s.Get()) - 3, "_ao", 3) == NULL) { texfilenoext_s = Left(texfilenoext_s.Get(), strlen(texfilenoext_s.Get()) - 3); bDetectedPBRTextureSetName = true; }
							
								if (strnicmp(texfilenoext_s.Get() + strlen(texfilenoext_s.Get()) - 3, "_surface", 3) == NULL) { texfilenoext_s = Left(texfilenoext_s.Get(), strlen(texfilenoext_s.Get()) - 3); bDetectedPBRTextureSetName = true; }
					
								if (bDetectedPBRTextureSetName == true)
								{
									//PE: Need to check filename only and current object folder.
									bool tex_found = false;
									int pos = 0;
									for (pos = texfilenoext_s.Len(); pos > 0; pos--) 
									{
										if (cstr(Mid(texfilenoext_s.Get(), pos)) == "\\" || cstr(Mid(texfilenoext_s.Get(), pos)) == "/")
											break;
									}
									if (pos > 0) 
									{
										cstr directfile = Right(texfilenoext_s.Get(), texfilenoext_s.Len() - pos);
										cstr tmp = cstr(cstr(folder_s) + directfile + "_color.dds").Get();
										if (FileExist(tmp.Get())) 
										{
											addtocollection(tmp.Get());
											tmp = cstr(cstr(folder_s) + directfile + "_normal.dds").Get();
											addtocollection(tmp.Get());
											tmp = cstr(cstr(folder_s) + directfile + "_metalness.dds").Get();
											addtocollection(tmp.Get());
											tmp = cstr(cstr(folder_s) + directfile + "_gloss.dds").Get();
											addtocollection(tmp.Get());
											tmp = cstr(cstr(folder_s) + directfile + "_ao.dds").Get();
											addtocollection(tmp.Get());
											tmp = cstr(cstr(folder_s) + directfile + "_illumination.dds").Get();
											addtocollection(tmp.Get());
									
											tmp = cstr(cstr(folder_s) + directfile + "_surface.dds").Get();
											addtocollection(tmp.Get());
									
											tex_found = true;
										}
										tmp = cstr(cstr(folder_s) + directfile + "_color.png").Get();
										if (FileExist(tmp.Get())) 
										{
											addtocollection(tmp.Get());
											tmp = cstr(cstr(folder_s) + directfile + "_normal.png").Get();
											addtocollection(tmp.Get());
											tmp = cstr(cstr(folder_s) + directfile + "_metalness.png").Get();
											addtocollection(tmp.Get());
											tmp = cstr(cstr(folder_s) + directfile + "_gloss.png").Get();
											addtocollection(tmp.Get());
											tmp = cstr(cstr(folder_s) + directfile + "_ao.png").Get();
											addtocollection(tmp.Get());
											tmp = cstr(cstr(folder_s) + directfile + "_illumination.png").Get();
											addtocollection(tmp.Get());
									
											//PE: surface still .dds
											tmp = cstr(cstr(folder_s) + directfile + "_surface.dds").Get();
											addtocollection(tmp.Get());
										
											tex_found = true;
										}
									}

									//PE: We get some strange folder created in the standalone from here.
									// add other PBR textures just in case not detected in model data
									if (!tex_found)
									{
										cstr texfileColor_s = texfilenoext_s + "_color.dds";
										//Only if the src is exists.
										if (FileExist(cstr(cstr(folder_s) + texpath_s + texfileColor_s).Get()) || FileExist(cstr(cstr(folder_s) + texfileColor_s).Get())) 
										{
											addtocollection(cstr(cstr(folder_s) + texpath_s + texfileColor_s).Get());
											addtocollection(cstr(cstr(folder_s) + texfileColor_s).Get());
											cstr texfileNormal_s = texfilenoext_s + "_normal.dds";
											addtocollection(cstr(cstr(folder_s) + texpath_s + texfileNormal_s).Get());
											addtocollection(cstr(cstr(folder_s) + texfileNormal_s).Get());
											cstr texfileMetalness_s = texfilenoext_s + "_metalness.dds";
											addtocollection(cstr(cstr(folder_s) + texpath_s + texfileMetalness_s).Get());
											addtocollection(cstr(cstr(folder_s) + texfileMetalness_s).Get());
											cstr texfileGloss_s = texfilenoext_s + "_gloss.dds";
											addtocollection(cstr(cstr(folder_s) + texpath_s + texfileGloss_s).Get());
											addtocollection(cstr(cstr(folder_s) + texfileGloss_s).Get());
											cstr texfileAO_s = texfilenoext_s + "_ao.dds";
											addtocollection(cstr(cstr(folder_s) + texpath_s + texfileAO_s).Get());
											addtocollection(cstr(cstr(folder_s) + texfileAO_s).Get());
											cstr texfileIllumination_s = texfilenoext_s + "_illumination.dds";
											addtocollection(cstr(cstr(folder_s) + texpath_s + texfileIllumination_s).Get());
											addtocollection(cstr(cstr(folder_s) + texfileIllumination_s).Get());
										
											cstr texfilesurface_s = texfilenoext_s + "_surface.dds";
											addtocollection(cstr(cstr(folder_s) + texpath_s + texfilesurface_s).Get());
											addtocollection(cstr(cstr(folder_s) + texfilesurface_s).Get());
									
										}
									}
								}

								if (FileExist(cstr(cstr(folder_s) + texpath_s + texfile_s).Get()))
									addtocollection(cstr(cstr(folder_s) + texpath_s + texfile_s).Get());
								if (FileExist(cstr(cstr(folder_s) + texfile_s).Get()))
									addtocollection(cstr(cstr(folder_s) + texfile_s).Get());

								if (cstr(Right(texfile_s.Get(), 4)) != ".dds")
								{
									//  also convert to DDS and add those too
									if (FileExist(cstr(cstr(folder_s) + texfile_s + ".png").Get()))
										addtocollection(cstr(cstr(folder_s) + texfile_s + ".png").Get());
									texfile_s = cstr(Left(texfile_s.Get(), Len(texfile_s.Get()) - 4)) + ".dds";
									if (FileExist(cstr(cstr(folder_s) + texpath_s + texfile_s).Get()))
										addtocollection(cstr(cstr(folder_s) + texpath_s + texfile_s).Get());
									if (FileExist(cstr(cstr(folder_s) + texfile_s).Get()))
										addtocollection(cstr(cstr(folder_s) + texfile_s).Get());
								}
							}
							b += 4;
						}
					}
				}
				DeleteMemblock(mbi);
			}
		}
	}
}

//
// Scan default installation, keep core copy of default files for reference (so know custom content when we see it)
//

void CreateItineraryFile ( void )
{
	g_sDefaultAssetFiles.clear();
}

void scanallfolder ( cstr subThisFolder_s, cstr subFolder_s )
{
	// into folder
	if ( subThisFolder_s.Len() > 0 ) SetDir ( subThisFolder_s.Get() );

	// first scan all files and folders - store folders locally
	ChecklistForFiles();
	int iFoldersCount = ChecklistQuantity();
	cstr* pFolders = new cstr[iFoldersCount+1];
	for ( int c = 1; c <= iFoldersCount; c++ )
	{
		pFolders[c] = "";
		LPSTR pFileFolderName = ChecklistString(c);
		if ( strcmp ( pFileFolderName, "." ) != NULL && strcmp ( pFileFolderName, ".." ) !=NULL )
		{
			if ( ChecklistValueA(c) == 1 )
			{
				pFolders[c] = pFileFolderName;
			}
			else
			{
				// found file reference
				cstr relativeFilePath_s = subFolder_s + "\\" + pFileFolderName;

				// clean up string
				LPSTR pOldStr = relativeFilePath_s.Get();
				LPSTR pCleanStr = new char[strlen(pOldStr)+1];
				int nn = 0;
				for ( int n = 0; n < strlen(pOldStr); n++ )
				{
					if ( pOldStr[n] == '\\' && pOldStr[n+1] == '\\' ) n++; // skip duplicate backslashes
					pCleanStr[nn++] = pOldStr[n];
				}
				pCleanStr[nn] = 0; 

				// add to master asset list of known stock assets
				g_sDefaultAssetFiles.push_back ( pCleanStr );
			}
		}
	}

	// now use local folder list and investigate each one
	for ( int f = 1; f <= iFoldersCount; f++ )
	{
		cstr pFolderName = pFolders[f];
		if ( pFolderName.Len() > 0 )
		{
			cstr relativeFolderPath_s = pFolderName;
			if ( subFolder_s.Len() > 0 ) relativeFolderPath_s = subFolder_s + "\\" + pFolderName;
			scanallfolder ( pFolderName, relativeFolderPath_s );
		}
	}

	// back out of folder
	if ( subThisFolder_s.Len() > 0 ) SetDir ( ".." );

	// finally free resources
	delete[] pFolders;
}

bool IsFileAStockAsset ( LPSTR pCheckThisFile )  
{
	// check if this file exists in stock assets
	char pFileToCheck[2048];
	strcpy ( pFileToCheck, pCheckThisFile );
	for ( int n = 0; n < g_sDefaultAssetFiles.size(); n++ )
	{
		LPSTR pCompare = g_sDefaultAssetFiles[n].Get();
		if ( stricmp ( pFileToCheck, pCompare ) == NULL )
			return true;
	}
	// before we return false, check special case for DBO files that have X files
	bool bSecondaryCheck = false;
	if ( strnicmp ( pCheckThisFile + strlen(pCheckThisFile) - 4, ".dbo", 4 ) == NULL )
	{
		pFileToCheck[strlen(pFileToCheck)-4] = 0;
		strcat ( pFileToCheck, ".x" );
		bSecondaryCheck = true;
	}
	if ( bSecondaryCheck == true )
	{
		for ( int n = 0; n < g_sDefaultAssetFiles.size(); n++ )
		{
			LPSTR pCompare = g_sDefaultAssetFiles[n].Get();
			if ( stricmp ( pFileToCheck, pCompare ) == NULL )
				return true;
		}
	}
	return false;
}


