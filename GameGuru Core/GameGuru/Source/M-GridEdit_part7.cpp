void gridedit_updatezoomviewvalues ( void )
{
	//  accepts gridentityinzoomview
	if (  t.gridentityinzoomview>0 ) 
	{
		t.zoomviewcameraangle_f=0.0;
		t.zoomviewcameraheight_f=50.0;
		t.zoomviewcamerarange_f=75.0;
		if (  t.entityelement[t.gridentityinzoomview].obj>0 ) 
		{
			if (  ObjectExist(t.entityelement[t.gridentityinzoomview].obj) == 1 ) 
			{
				t.zoomviewcamerarange_f=ObjectSize(t.entityelement[t.gridentityinzoomview].obj,1)*2.0;
				t.zoomviewcameraheight_f=(ObjectSize(t.entityelement[t.gridentityinzoomview].obj,1)/2.0)-100.0;
				if (  t.zoomviewcameraheight_f<5  )  t.zoomviewcameraheight_f = 5;

				// ensure camera always faces the front of an entity
				t.zoomviewcameraangle_f = (0-ObjectAngleY(t.entityelement[t.gridentityinzoomview].obj))+180.0f;
			}
		}
	}
}

void gridedit_save_test_map ( void )
{
	//  Save map data locally only (not to FPM)
	timestampactivity(0,"SAVETESTMAP: Save map");
	mapfile_savemap ( );

	if (bKeepWindowsResponding)
		EmptyMessages();

	//  Settings specific to the player
	timestampactivity(0,"SAVETESTMAP: Save player config");
	mapfile_saveplayerconfig ( );

	if (bKeepWindowsResponding)
		EmptyMessages();

	//  Save entity elements
	timestampactivity(0,"SAVETESTMAP: Save elements");
	entity_savebank ( );
	entity_savebank_ebe ( );

	if (bKeepWindowsResponding)
		EmptyMessages();

	//PE: Must remove , t.entityprofile[t.entid].ismarker == 12 && systemwide.
	extern StoryboardStruct Storyboard;
	std::vector <entitytype> StoreEntEle(20);
	int storeindex = 1;
	for (int i = 1; i <= g.entityelementlist; i++)
	{
		int tentid = t.entityelement[i].bankindex;
		if (tentid > 0 && t.entityprofile[tentid].ismarker == 12)
		{
			if (t.entityelement[i].eleprof.aimain_s.Len() > 0)
			{
				if (t.entityelement[i].eleprof.systemwide_lua)
				{
					if (strlen(Storyboard.gamename) > 0)
					{
						if (StoreEntEle.capacity() < storeindex + 1)
							StoreEntEle.reserve(storeindex + 10);
						StoreEntEle[storeindex++] = t.entityelement[i];
						t.entityelement[i].old_bankindex = t.entityelement[i].bankindex;
						//PE: These will be added when loading so mark them for reuse in map.ele
						t.entityelement[i].bankindex = 0; //PE: Save to be reused.
					}
					else
					{
						//PE: If we do not have a storyboard just convert systemwide to normal and save as normal map.ele
						t.entityelement[i].eleprof.systemwide_lua = false;
					}
				}
			}
		}
	}

	entity_saveelementsdata ( false );

	if (bKeepWindowsResponding)
		EmptyMessages();


	//PE: Restore systemwidelua.
	for (int i = 1; i <= g.entityelementlist; i++)
	{
		int tentid = t.entityelement[i].old_bankindex;
		if (tentid > 0 && t.entityprofile[tentid].ismarker == 12)
		{
			if (t.entityelement[i].eleprof.aimain_s.Len() > 0)
			{
				if (t.entityelement[i].eleprof.systemwide_lua)
				{
					t.entityelement[i].bankindex = t.entityelement[i].old_bankindex;
				}
			}
		}
	}

	//PE: Save systemwidelua.ele
	if (strlen(Storyboard.gamename) > 0)
	{
		if (bKeepWindowsResponding)
			EmptyMessages();

		timestampactivity(0, "saving systemwidelua.ele");
		cstr storeoldELEfile = t.elementsfilename_s;
		char collectionELEfilename[MAX_PATH];
		strcpy(collectionELEfilename, "projectbank\\");
		strcat(collectionELEfilename, Storyboard.gamename);
		strcat(collectionELEfilename, "\\systemwidelua.ele");
		GG_GetRealPath(collectionELEfilename, 1);
		if (FileExist(collectionELEfilename) == 1) DeleteFileA(collectionELEfilename);

		if (storeindex > 1)
		{
			t.elementsfilename_s = collectionELEfilename;

			std::vector <entitytype> storeentityelement;
			storeentityelement = t.entityelement;

			int iStoreEntEleCount = g.entityelementlist;
			g.entityelementlist = storeindex;
			t.entityelement = StoreEntEle;
			bool bForCollectionELE = true;
			entity_saveelementsdata(bForCollectionELE);

			if (bKeepWindowsResponding)
				EmptyMessages();

			g.entityelementlist = iStoreEntEleCount;
			t.entityelement = storeentityelement;
			storeentityelement.clear();
			t.elementsfilename_s = storeoldELEfile;
		}
	}

	//  Save waypoints
	timestampactivity(0,"SAVETESTMAP: Save waypoints");
	waypoint_savedata ( );

	if (bKeepWindowsResponding)
		EmptyMessages();

	//  Save editor configuration
	timestampactivity(0,"SAVETESTMAP: Save config");
	editor_savecfg ( );

	if (bKeepWindowsResponding)
		EmptyMessages();

	//  Save terrain
	timestampactivity(0,"SAVETESTMAP: Save terrain textures");
	t.tfileveg_s = g.mysystem.levelBankTestMap_s + "vegmask.png";// dds";
	t.tfilewater_s = g.mysystem.levelBankTestMap_s + "watermask.png";// dds"; 
	terrain_savetextures ( );

	if (bKeepWindowsResponding)
		EmptyMessages();

	t.tfileveggrass_s=g.mysystem.levelBankTestMap_s+"TTR0XR0\\vegmaskgrass.dat";

	if (bKeepWindowsResponding)
		EmptyMessages();

	timestampactivity(0,"SAVETESTMAP: Save terrain height data");
	t.tfile_s=g.mysystem.levelBankTestMap_s+"m.dat";
	terrain_save ( t.tfile_s.Get() );

	if (bKeepWindowsResponding)
		EmptyMessages();


	//  this ensures change flag does not use filemap port 1 (avoid freeze in build game)
	t.lastprojectmodified=0;

	//  Set modification flag
	timestampactivity(0,"SAVETESTMAP: Change modified flag of level");
	g.projectmodified = 0; gridedit_changemodifiedflag ( );
	g.projectmodifiedstatic = 0; 
	timestampactivity(0,"SAVETESTMAP: Complete");
}

void gridedit_save_map ( void )
{
	// seems save can cause IMGUI to crash out when rendering a texture that no longer exists
	extern bool bBlockImGuiUntilNewFrame;
	bBlockImGuiUntilNewFrame = true;
	
	extern bool g_bNoSwapchainPresent;
	iBlockRenderingForFrames = 5;
	g_bNoSwapchainPresent = true;

	// Proper saving message to user
	if (  t.recoverdonotuseany3dreferences == 0 ) 
	{
		editor_hideall3d ( );
	}

	// Save only to TESTMAP area (for map testing)
	gridedit_save_test_map ( );

	if (bKeepWindowsResponding)
		EmptyMessages();

	// Now store all part-files into main FPM project
	mapfile_saveproject_fpm ( );

	if (bKeepWindowsResponding)
		EmptyMessages();

	// Add Latest project To Recent List
	gridedit_updateprojectname ( );

	if (bKeepWindowsResponding)
		EmptyMessages();

	// Clear status Text (  )
	//t.statusbar_s="" ; popup_text_close();

	extern std::vector<int> g_smartObjectDummyEntities;
	for (int i = 0; i < t.entityelement.size(); i++)
	{
		if (t.entityelement[i].iIsSmarkobjectDummyObj == 1)
		{
			g_smartObjectDummyEntities.push_back(i);
		}
	}

	// refresh as SAVE can remove entities and segments
	if ( t.entityorsegmententrieschanged == 1 ) 
	{
		// 111115 - and if not exiting GG
		if ( g.savenoreloadflag == 0 )
		{
			gridedit_load_map ( );
		}
		t.entityorsegmententrieschanged=0;
	}
}

void gridedit_updatemapbeforeedit ( void )
{
	//  Completely reset filemap (and interface parts ie library)
	editor_filemapinit ( );

	//  Newly loaded map starts at layer X
	t.gridlayer=5 ; t.refreshgrideditcursor=1;
}

void gridedit_clear_settings ( void )
{
	//  Default settings
	gridedit_clear_configsettings ( );
	t.gridscale_f=((800/2)/8)/t.gridzoom_f;
	t.currentprojectfilename_s="";
	t.gridground=0;
	t.gridselection=1;
	t.bufferlayer=-1;
	g.gridlayershowsingle=0;
	t.grideditartwidth=1;
	t.grideditartwidthx=1;
	t.grideditartwidthy=1;
	t.locallibrarysegidmaster=0;
	t.locallibraryentidmaster=0;
}

void gridedit_clear_configsettings ( void )
{
	// defaults
	t.borderx_f = 1024.0*50.0; t.cx_f = GGORIGIN_X;
	t.bordery_f = 1024.0*50.0; t.cy_f = GGORIGIN_Z;

	//  Default zoom
	t.gridzoom_f=3.0 ; t.clipheight_f=655;

	//  default grideditselect
	//  0=terrain mode
	//  4=zoom mode
	//  5=entity editing
	//  6=waypoint mode
	t.grideditselect=0;
}

void gridedit_clear_map ( void )
{
	//Stop delete any particle effects.
	gpup_deleteAllEffects();

	//  Delete any old entity objects
	gridedit_deletelevelobjects ( );

	//  Delete any old weapon objects (and reload original data in case last level edited them)
	gun_releaseresources ( );
	gun_scaninall_dataonly ( );

	//  Remove any shader lighting
	lighting_free ( );

	//  delete any conkit objects
	///conkit_saveload_clear ( );

	//  Ensure whether New or Load, physics tweakables for player are reset
	physics_inittweakables ( );

	// new and loaded levels need a refreshed gunlist
	extern bool g_bGunListNeedsRefreshing;
	g_bGunListNeedsRefreshing = true;

	//  Set modification flag
	g.projectmodified = 0 ; gridedit_changemodifiedflag ( );
	g.projectmodifiedstatic = 0;

	//  ensure no leftovers from last edit session
	t.tlasttentitytoselect=-1;

	//  Must generate super texture when do test level for this new level map
	t.terrain.generatedsupertexture=0;

	//  reset free flight mode
	t.editorfreeflight.mode=0 ; t.updatezoom=1;
	t.editorfreeflight.sused=0;
	t.gridentityhidemarkers=0;
	t.cameraviewmode=0;
}

void gridedit_resetmemortracker ( void )
{
	// 121115 - good place to reset memory tracking
	int iMemoryLostFromActivitySoFar = g.gamememactuallyusedstart - SMEMAvailable(1);
	g.gamememactualmaxrightnow = g.gamememactualmaxrightnow - iMemoryLostFromActivitySoFar;
	g.gamememactuallyusedstart = SMEMAvailable(1);
}

void gridedit_emptyallcustomfiles ( void )
{
	ChecklistForFiles();
	for ( t.c = 1 ; t.c <= ChecklistQuantity(); t.c++ )
	{
		t.tfile_s = ChecklistString(t.c);
		if ( t.tfile_s != "." && t.tfile_s != ".." ) 
		{
			// only if a CUSTOM file - needs clearing when new level created
			if ( strnicmp ( t.tfile_s.Get(), "CUSTOM_", 7 ) == NULL )
			{
				DeleteAFile ( t.tfile_s.Get() );
			}
		}
	}
}
void gridedit_emptyrogueoldfiles(void)
{
	ChecklistForFiles();
	for (t.c = 1; t.c <= ChecklistQuantity(); t.c++)
	{
		t.tfile_s = ChecklistString(t.c);
		if (t.tfile_s != "." && t.tfile_s != "..")
		{
			if (strlen(t.tfile_s.Get()) > 4)
			{
				if (strnicmp (t.tfile_s.Get() + strlen(t.tfile_s.Get()) - 4, ".ele", 4) == NULL)
				{
					timestampactivity(0, t.tfile_s.Get());
					DeleteAFile (t.tfile_s.Get());
				}
			}
		}
	}
}

void gridedit_emptyallterrainobjfiles (void)
{
	// Delete all terrainobj files so fresh caches can be created
	ChecklistForFiles();
	for (t.c = 1; t.c <= ChecklistQuantity(); t.c++)
	{
		t.tfile_s = ChecklistString(t.c);
		if (t.tfile_s != "." && t.tfile_s != "..")
		{
			// only if a CUSTOM file - needs clearing when new level created
			if (strnicmp (t.tfile_s.Get(), "terrainobj", 10) == NULL)
			{
				DeleteAFile (t.tfile_s.Get());
			}
		}
	}
}

// force new level camera into free flight mode
void gridedit_resetcameraanddynamicsky(void)
{
	// set camera to free flight on specific angle and starting defaults
	g_bResetCameraToFreeFlightOnNewLevel = true;
	t.editorfreeflight.sused = 0;
}

void gridedit_new_map(void)
{
	// seems new 'may' cause IMGUI to crash out when rendering a texture that no longer exists
	extern bool bBlockImGuiUntilNewFrame;
	bBlockImGuiUntilNewFrame = true;
	ClearAllGroupLists();
	t.widget.pickedEntityIndex = 0;
	t.gridentity = 0;
	
	//PE: These need to be reset.
	t.gridentityinzoomview = 0;
	t.tforceentityfindfloor = 0;

	// ensure tab mode vars reset (no carry from previous session)
	g.tabmode = 0; //TABTAB mode
	g.tabmodeshowfps = 0; //F11 mode
	g.tabmodehidehuds = 0;
	g.mouseishidden = 0;
	t.terrain.terrainpaintermode = 1;

	// reset weather display flag
	bEnableWeather = false;

	//  Start time profiling
	timestampactivity(0, "NEWMAP: Starting new map");

	//  No project - new map
	g.projectfilename_s = "";
	g.projectmodified = 0; t.lastprojectmodified = 0;
	g.projectmodifiedstatic = 0;
	gridedit_updateprojectname();

	// hide EBE if starting new map
	ebe_hide();
	ebe_hardreset();

	// hide terrain texture panel
	terrain_paintselector_hide();

	//if ( gbWelcomeSystemActive == false ) 
	Sync();

	//  Reset visual settings for new map
	visuals_newlevel();

	// Reset all water settings.
	visuals_water_reset();

	//  Ensure default terrain and veg graphics
	terrain_changestyle();
	g.vegstyleindex = t.visuals.vegetationindex;

	//  Load map data
	editor_hideall3d();

	//  Clear all settings
	timestampactivity(0, "NEWMAP: _gridedit_clear_settings");
	gridedit_clear_settings();

	//  Empty the lightmap folder
	timestampactivity(0, "NEWMAP: mapfile_emptylightmapandttsfilesfolder_wicked");
	void mapfile_emptylightmapandttsfilesfolder_wicked(void);
	mapfile_emptylightmapandttsfilesfolder_wicked();

	// Reset texture/profile in EBE folder
	ebe_restoreebedefaulttextures();

	// Empty EBEs from testmap folder
	cstr pStoreOld = GetDir(); 
	if ( PathExist ( g.mysystem.levelBankTestMap_s.Get() ) == 0 )
	{
		// somehow levelbank\testmap folder gone (can be deleted sometimes)
		SetDir ( cstr(g.fpscrootdir_s + "\\Files\\").Get() );
		if ( PathExist ( "levelbank" ) == 0 )
		{
			MakeDirectory ( "levelbank" );
			SetDir ( "levelbank" );
		}
		if ( PathExist ( "testmap" ) == 0 )
		{
			MakeDirectory ( "testmap" );
			SetDir ( "testmap" );
		}
	}
	else
		SetDir ( g.mysystem.levelBankTestMap_s.Get() );

	// Delete any EBE files for new levels
	timestampactivity(0,"NEWMAP: mapfile_emptyebesfromtestmapfolder");
	mapfile_emptyebesfromtestmapfolder(false);

	// Delete any CUSTOM files for new levels, otherwise messes up new asset addition work
	timestampactivity(0,"NEWMAP: gridedit_emptyallcustomfiles");
	gridedit_emptyallcustomfiles ( );

	// Delete any MAP.ELE old file as this might introduce old entities such as weapons!
	timestampactivity(0, "NEWMAP: gridedit_emptyrogueoldfiles");
	gridedit_emptyrogueoldfiles ();

	// empty all terrain obj files if any
	gridedit_emptyallterrainobjfiles();

	// restore folder to default 
	SetDir ( pStoreOld.Get() );

	// Empty terraintexture files from testmap folder
	SetDir ( g.mysystem.levelBankTestMap_s.Get() );
	if ( FileExist ( "superpalette.ter" ) == 1 ) DeleteFileA ( "superpalette.ter" );
	if ( FileExist ( "Texture_D.dds" ) == 1 ) DeleteFileA ( "Texture_D.dds" );
	if ( FileExist ( "Texture_D.jpg" ) == 1 ) DeleteFileA ( "Texture_D.jpg" );
	if ( FileExist ( "Texture_N.dds" ) == 1 ) DeleteFileA ( "Texture_N.dds" );
	if ( FileExist ( "Texture_N.jpg" ) == 1 ) DeleteFileA ( "Texture_N.jpg" );
	if ( FileExist ( "globalenvmap.dds" ) == 1 ) DeleteFileA ( "globalenvmap.dds" );
	SetDir ( pStoreOld.Get() );

	// ensures new terrain in new map is loaded into terrain texture panel when shown
	terrain_resetfornewlevel();

	//  Clear map first
	timestampactivity(0,"NEWMAP: _gridedit_clear_map");
	gridedit_clear_map ( );

	// 121115 - Reset memory tracker
	gridedit_resetmemortracker ( );

	//  Delete all assets of map work
	timestampactivity(0,"NEWMAP: _waypoint_deleteall");
	waypoint_deleteall ( );
	mapfile_newmap ( );
	// Cleanup any visual logic connection objects.
	void deleterelationobjects();
	deleterelationobjects();

	//  Update remaining map data before editing
	timestampactivity(0,"NEWMAP: _gridedit_updatemapbeforeedit");
	gridedit_updatemapbeforeedit ( );

	//  Some default setup for new scene (load markers)
	timestampactivity(0,"NEWMAP: _editor_filemapdefaultinitfornew");
	editor_filemapdefaultinitfornew ( );

	//  Recreate terrain to remove links to old LOD1 objects
	timestampactivity(0,"NEWMAP: _terrain_createactualterrain");
	terrain_createactualterrain ( );

	//  Randomise/Flatten terrain when NEW level created
	if (  t.inputsys.donewflat == 1 ) 
	{
		timestampactivity(0,"NEWMAP: Save newly flattened terrain");
		terrain_flattenterrain ( );
	}
	else
	{
		timestampactivity(0,"NEWMAP: Save newly randomised terrain");
		terrain_randomiseterrain ( );
	}
	t.tfile_s=g.mysystem.levelBankTestMap_s+"m.dat";
	terrain_save ( t.tfile_s.Get() );

	timestampactivity(0,"NEWMAP: Save terrain data");
	t.tfileveg_s = g.mysystem.levelBankTestMap_s + "vegmask.png";// dds";
	t.tfilewater_s = g.mysystem.levelBankTestMap_s + "watermask.png";// dds";
	t.tgeneratefreshwatermaskflag=1;
	terrain_generatevegandmaskfromterrain ( );
	timestampactivity(0,"NEWMAP: Save terrain mask data");
	t.tfileveggrass_s=g.mysystem.levelBankTestMap_s+"TTR0XR0\\vegmaskgrass.dat";

	timestampactivity(0,"NEWMAP: Finish t.terrain generation");
	
	//  Set standard start height for camera
	t.gridzoom_f=3.0 ; t.clipheight_f=655 ; t.updatezoom=1;

	//  Reset cursor
	t. grideditselect = 0 ; editor_refresheditmarkers ( );

	//  Clear widget status
	t.widget.pickedObject=0 ; widget_updatewidgetobject ( );

	//  Reset UNDO/REDO buffer
	t.entityundo.action=0;
	t.entityundo.entityindex=0;
	t.entityundo.bankindex=0;
	t.entityundo.undoperformed=0;
	t.terrainundo.bufferfilled=0;
	t.terrainundo.mode=0;

	gridedit_resetcameraanddynamicsky();
	bForceRefreshLightCount = true;

	//  Finished new map
	timestampactivity(0,"NEWMAP: Finish creating new map");
}

void gridedit_new_map_quick(void)
{
	ClearAllGroupLists();
	t.widget.pickedEntityIndex = 0;
	t.gridentity = 0;
	// ensure tab mode vars reset (no carry from previous session)
	g.tabmode = 0; //TABTAB mode
	g.tabmodeshowfps = 0; //F11 mode
	g.tabmodehidehuds = 0;
	g.mouseishidden = 0;
	t.terrain.terrainpaintermode = 1;

	//LB: These need to be reset also
	t.gridentityinzoomview = 0;
	t.tforceentityfindfloor = 0;

	//  Start time profiling
	timestampactivity(0, "NEWMAP: Starting new map");

	//  No project - new map
	g.projectfilename_s = "";
	g.projectmodified = 0; t.lastprojectmodified = 0;
	g.projectmodifiedstatic = 0;
	gridedit_updateprojectname();

	// hide EBE if starting new map
	ebe_hide();
	ebe_hardreset();

	// hide terrain texture panel
	terrain_paintselector_hide();

	//  Reset visual settings for new map
	visuals_newlevel();

	// Reset all water settings.
	visuals_water_reset();

	//  Reset visual settings for new map
	t.visuals.refreshshaders = 1;
	
	//  Load map data
	editor_hideall3d();

	//  Clear all settings
	timestampactivity(0, "NEWMAP: _gridedit_clear_settings");
	gridedit_clear_settings();

	// Empty EBEs from testmap folder
	cstr pStoreOld = GetDir();
	if (PathExist(g.mysystem.levelBankTestMap_s.Get()) == 0)
	{
		// somehow levelbank\testmap folder gone (can be deleted sometimes)
		SetDir(cstr(g.fpscrootdir_s + "\\Files\\").Get());
		if (PathExist("levelbank") == 0)
		{
			MakeDirectory("levelbank");
			SetDir("levelbank");
		}
		if (PathExist("testmap") == 0)
		{
			MakeDirectory("testmap");
			SetDir("testmap");
		}
	}
	else {
		SetDir(g.mysystem.levelBankTestMap_s.Get());
		//  Empty the lightmap folder
		timestampactivity(0, "NEWMAP: cleantestmapfolder");
		if (FileExist("superpalette.ter") == 1) DeleteFileA("superpalette.ter");
		if (FileExist("Texture_D.dds") == 1) DeleteFileA("Texture_D.dds");
		if (FileExist("Texture_D.jpg") == 1) DeleteFileA("Texture_D.jpg");
		if (FileExist("Texture_N.dds") == 1) DeleteFileA("Texture_N.dds");
		if (FileExist("Texture_N.jpg") == 1) DeleteFileA("Texture_N.jpg");
		if (FileExist("globalenvmap.dds") == 1) DeleteFileA("globalenvmap.dds");
		//  Ensure no old OBS file and OBS triggers to generate
		if (t.tignoreinvalidateobstacles == 0) {
			if (FileExist("map.obs") == 1) DeleteFileA("map.obs");
			t.aisystem.generateobs = 1;
		}

		void mapfile_emptylightmapandttsfilesfolder_wicked(void);
		mapfile_emptylightmapandttsfilesfolder_wicked();

		// Delete any EBE files for new levels
		mapfile_emptyebesfromtestmapfolder(false);
		gridedit_emptyallcustomfiles();
		gridedit_emptyallterrainobjfiles();
	}

	// restore folder to default 
	SetDir(pStoreOld.Get());

	// ensures new terrain in new map is loaded into terrain texture panel when shown
	terrain_resetfornewlevel();

	//  Clear map first
	t.tlasttentitytoselect = -1;
	g.projectmodified = 0; gridedit_changemodifiedflag();
	g.projectmodifiedstatic = 0;

	//Stop delete any particle effects.
	gpup_deleteAllEffects();

	lighting_free();
	gridedit_deletelevelobjects();

	// 121115 - Reset memory tracker
	gridedit_resetmemortracker();

	//  Delete all assets of map work
	mapfile_newmap();

	//  Update remaining map data before editing
	if (t.game.gameisexe == 0)
	{
		// for now, it seems the standalone can call this function!!
		editor_clearlibrary();
		g.entidmaster = 0;
		editor_filllibrary();
		editor_leftpanelreset();
		t.gridlayer = 5; t.refreshgrideditcursor = 1;
	}

	//  Recreate terrain to remove links to old LOD1 objects
	timestampactivity(0, "NEWMAP: _terrain_createactualterrain");
	terrain_createactualterrain();

	//  Randomise/Flatten terrain when NEW level created
	if (t.inputsys.donewflat == 1)
	{
		timestampactivity(0, "NEWMAP: Save newly flattened terrain");
		terrain_flattenterrain();
	}
	else
	{
		timestampactivity(0, "NEWMAP: Save newly randomised terrain");
		terrain_randomiseterrain();
	}
	t.tfile_s = g.mysystem.levelBankTestMap_s + "m.dat";
	terrain_save ( t.tfile_s.Get() );
	timestampactivity(0, "NEWMAP: Save terrain data");
	t.tfileveg_s = g.mysystem.levelBankTestMap_s + "vegmask.png";// dds";
	t.tfilewater_s = g.mysystem.levelBankTestMap_s + "watermask.png";// dds";
	t.tgeneratefreshwatermaskflag = 1;
	terrain_generatevegandmaskfromterrain();
	t.tfileveggrass_s = g.mysystem.levelBankTestMap_s + "TTR0XR0\\vegmaskgrass.dat";

	//  Set standard start height for camera
	t.gridzoom_f = 3.0; t.clipheight_f = 655; t.updatezoom = 1;

	//  Reset cursor
	t.grideditselect = 0; editor_refresheditmarkers();

	//  Clear widget status
	t.widget.pickedObject = 0; widget_updatewidgetobject();

	//  Reset UNDO/REDO buffer
	t.entityundo.action = 0;
	t.entityundo.entityindex = 0;
	t.entityundo.bankindex = 0;
	t.entityundo.undoperformed = 0;
	t.terrainundo.bufferfilled = 0;
	t.terrainundo.mode = 0;

	gridedit_resetcameraanddynamicsky();
	bForceRefreshLightCount = true;

	//  Finished new map
	timestampactivity(0, "NEWMAP: Finish creating new map");
}

void gridedit_updatestatusbar ( void )
{
}

void gridedit_load_map ( void )
{
	TDRTrace("[LOADMAP] gridedit_load_map: ENTER");
	ClearAllGroupLists();
	t.widget.pickedEntityIndex = 0;
	t.gridentity = 0;

	//Stop delete any particle effects.
	TDRTrace("[LOADMAP] gpup_deleteAllEffects");
	gpup_deleteAllEffects();

	//  Load map data
	TDRTrace("[LOADMAP] editor_hideall3d");
	editor_hideall3d ( );

	//LB: These need to be reset (probably can put these in a common 'new something' area
	t.gridentityinzoomview = 0;
	t.tforceentityfindfloor = 0;

	// hide terrain texture panel
	TDRTrace("[LOADMAP] terrain_paintselector_hide + Sync");
	terrain_paintselector_hide(); Sync();
	TDRTrace("[LOADMAP] Sync returned");

	// ensure NO old flat area items in list
	TDRTrace("[LOADMAP] GGTerrain_RemoveAllFlatAreas");
	timestampactivity(0, "GGTerrain_RemoveAllFlatAreas:1");
	GGTerrain_RemoveAllFlatAreas();

	if (bKeepWindowsResponding)
		EmptyMessages();

	//  Reset visual settings for new map
	if (  t.skipfpmloading == 0 )
	{
		TDRTrace("[LOADMAP] visuals_newlevel");
		visuals_newlevel ( );
	}

	//  Force the zoom to be updated to prevent black screen bug, due to old camera range
	t.updatezoom=1;

	//  Load FPM project into testmap files area
	t.tloadsuccessfully=1;
	if (  t.skipfpmloading == 1 )
	{
		//  replace NEW with RELOAD
		TDRTrace("[LOADMAP] skip FPM loading - RELOAD path");
		OpenFileMap (  1,"FPSEXCHANGE" );
		SetFileMapDWORD (  1, 408, 0 );
		SetEventAndWait (  1 );
	}
	else
	{
		//  this setstloadsuccessfully to zero if failed to load FPM (corrupt zipfile)
		TDRTrace("[LOADMAP] mapfile_loadproject_fpm");
		mapfile_loadproject_fpm ( );
		TDRTrace("[LOADMAP] mapfile_loadproject_fpm done, success=%d", t.tloadsuccessfully);
	}

	//  Loaded successfully
	if ( t.tloadsuccessfully == 1 )
	{
		//  Clear map first
		TDRTrace("[LOADMAP] gridedit_clear_map");
		gridedit_clear_map ( );

		if (bKeepWindowsResponding)
			EmptyMessages();

		// 121115 - Reset memory tracker
		TDRTrace("[LOADMAP] gridedit_resetmemortracker");
		gridedit_resetmemortracker ( );

		//  Determine if FPM is accompanied by .REPLACE file
		t.treplacefilename_s = "" ; t.treplacefilename_s = t.treplacefilename_s + Left(g.projectfilename_s.Get(),Len(g.projectfilename_s.Get())-4)+".replace";
		if (  FileExist(t.treplacefilename_s.Get()) == 1 ) 
		{
			t.editor.replacefilepresent_s=t.treplacefilename_s;
		}
		else
		{
			t.editor.replacefilepresent_s="";
		}

		// as load from level to level, cannot carry over E or ENTID refs from any previous level stoerd in collection item list
		for (int n = 0; n < g_collectionList.size(); n++)
		{
			g_collectionList[n].iEntityID = 0;
			g_collectionList[n].iEntityElementE = 0;
		}

		//  Load entity bank and elements
		TDRTrace("[LOADMAP] entity_loadbank");
		popup_text_change(t.strarr_s[611].Get());
		entity_loadbank ( );
		TDRTrace("[LOADMAP] entity_loadbank done");
		if (bKeepWindowsResponding)
			EmptyMessages();

		TDRTrace("[LOADMAP] entity_loadelementsdata");
		timestampactivity(0, "s:entity_loadelementsdata()");
		entity_loadelementsdata ( );
		TDRTrace("[LOADMAP] entity_loadelementsdata done");

		if (bKeepWindowsResponding)
			EmptyMessages();

		timestampactivity(0, "e:entity_loadelementsdata()");
		t.editor.replacefilepresent_s="";

		//  Load waypoints
		TDRTrace("[LOADMAP] waypoint_loaddata");
		popup_text_change(t.strarr_s[612].Get());
		waypoint_loaddata ( );

		if (bKeepWindowsResponding)
			EmptyMessages();

		TDRTrace("[LOADMAP] waypoint_recreateobjs");
		waypoint_recreateobjs ( );

		if (bKeepWindowsResponding)
			EmptyMessages();

		//  Load data
		TDRTrace("[LOADMAP] mapfile_loadmap");
		popup_text_change(t.strarr_s[613].Get());
		mapfile_loadmap ( );
		TDRTrace("[LOADMAP] mapfile_loadmap done");

		if (bKeepWindowsResponding)
			EmptyMessages();

		//  Load player settings
		TDRTrace("[LOADMAP] mapfile_loadplayerconfig");
		timestampactivity(0,"Load player config");
		mapfile_loadplayerconfig ( );

		if (bKeepWindowsResponding)
			EmptyMessages();

		//  Load terrain
		TDRTrace("[LOADMAP] terrain_createactualterrain");
		popup_text_change(t.strarr_s[610].Get());
		timestampactivity(0, "Create Terrain");
		terrain_createactualterrain ( );
		TDRTrace("[LOADMAP] terrain_createactualterrain done");
		TDRTrace("[LOADMAP] terrain_loaddata");
		terrain_loaddata ( );
		TDRTrace("[LOADMAP] terrain_loaddata done");

		if (bKeepWindowsResponding)
			EmptyMessages();

		// ensure firerate settings updated with any overrides set by developer mode changes
		TDRTrace("[LOADMAP] entity_init_overwritefireratesettings");
		entity_init_overwritefireratesettings();

		if (bKeepWindowsResponding)
			EmptyMessages();

		//  Update remaining map data before editing
		TDRTrace("[LOADMAP] gridedit_updatemapbeforeedit");
		timestampactivity(0, "Reset Editor.");
		gridedit_updatemapbeforeedit ( );
		TDRTrace("[LOADMAP] gridedit_updatemapbeforeedit done");

		if (bKeepWindowsResponding)
			EmptyMessages();

		//  Load editor configuration
		TDRTrace("[LOADMAP] editor_loadcfg");
		int iOldGE = t.grideditselect;
		editor_loadcfg ( true );

		extern bool g_bNeedToConvertClassicPositionsToMAX;
		if (g_bNeedToConvertClassicPositionsToMAX == true)
		{
			// new terrain system is located at 0,0,0 (not 25600x600x25600), so shift to new location
			GGVECTOR3 vToMAXShift = GGVECTOR3(25600, 600, 25600);
			if (g.entityelementlist > 0)
			{
				// shift all entity elements to new positions
				for (int e = 1; e <= g.entityelementlist; e++)
				{
					t.entityelement[e].x -= vToMAXShift.x;
					t.entityelement[e].y -= vToMAXShift.y;
					t.entityelement[e].z -= vToMAXShift.z;
				}
			}

			// LB: also shift camera position to match (if seemingly the old coordinate system) [need a better way to detect 'classic' levels and 'old MAX' levels!!
			if (fabs(t.cx_f-25600)<1000.0f && fabs(t.cy_f - 25600) < 1000.0f)
			{
				t.cx_f -= vToMAXShift.x;
				t.cy_f -= vToMAXShift.z;
			}
		}

		//In wicked keep current window open, terrain , entity...
		t.grideditselect = iOldGE;
		//  Load segments/prefab/entities into window
		TDRTrace("[LOADMAP] editor_filllibrary");
		OpenFileMap (  1,"FPSEXCHANGE" );
		editor_filllibrary ( );

		if (bKeepWindowsResponding)
			EmptyMessages();

		//  Add Latest project To Recent List
		gridedit_updateprojectname ( );
		TDRTrace("[LOADMAP] gridedit_load_map: SUCCESS - load complete");
	}
	else
	{
		//  FPM could not be extracted (likely a corrupt zipfile)
		if (  t.tloadsuccessfully == 0 ) 
		{
			t.strwork = ""; t.strwork = t.strwork + t.strarr_s[614]+" : "+Right(g.projectfilename_s.Get(),Len(g.projectfilename_s.Get())-Len(g.fpscrootdir_s.Get()));
			popup_text_change( t.strwork.Get() );
		}
		if (  t.tloadsuccessfully == 2 ) 
		{
			popup_text_change("The FPM was not created with Game Guru");
		}
		SleepNow (  2000 );

		//  Create blank in this case
		t.inputsys.donewflat=1;
		gridedit_new_map ( );
	}

	if (bKeepWindowsResponding)
		EmptyMessages();

	//  Popup warning if load found some missing files
	if ( g.timestampactivityflagged == 1 ) 
	{
		//  message prompt
		g.timestampactivityflagged=0;

		//  copy time stamp log to map bank log
		if (  ArrayCount(t.missingmedia_s) >= 0 ) 
		{
			t.tmblogfile_s = "" ; t.tmblogfile_s=t.tmblogfile_s + Left(g.projectfilename_s.Get(),Len(g.projectfilename_s.Get())-4)+".log";
			if (  FileExist(t.tmblogfile_s.Get()) == 1  )  DeleteAFile (  t.tmblogfile_s.Get() );
			if (  Len(t.tmblogfile_s.Get())>4 ) 
			{
				t.missingmedia_s[0]="MISSING MEDIA:";
				for ( t.m = 1 ; t.m <= ArrayCount(t.missingmedia_s); t.m++ )
				{
					if (  Len(t.missingmedia_s[t.m].Get())>2 ) 
					{
						t.missingmedia_s[t.m]=t.missingmedia_s[t.m]+"=replace"+t.missingmedia_s[t.m];
					}
				}
				SaveArray (  t.tmblogfile_s.Get() ,t.missingmedia_s );
			}
		}
	}

	// a new global folder has been introduced, and some script files moved there
	// so need to ensure older levels using the old location are redirected on load
	bool bFindAnyMissingScriptsThatMayHaveMovedToGlobalFolder = true;
	if (bFindAnyMissingScriptsThatMayHaveMovedToGlobalFolder == true)
	{
		bool bReplacedScript = false;
		if (g.entityelementlist > 0)
		{
			for (int e = 1; e <= g.entityelementlist; e++)
			{
				LPSTR pOriginal = t.entityelement[e].eleprof.aimain_s.Get();
				LPSTR pFileOnly = NULL;
				for (int n = strlen(pOriginal); n > 0; n--)
				{
					if (pOriginal[n] == '\\' || pOriginal[n] == '/')
					{
						pFileOnly = pOriginal + n + 1;
						break;
					}
				}
				if (pFileOnly)
				{
					char pTryInGlobal[MAX_PATH];
					strcpy(pTryInGlobal, "global\\");
					strcat(pTryInGlobal, pFileOnly);
					char pTryInGlobalAbs[MAX_PATH];
					strcpy(pTryInGlobalAbs, "scriptbank\\");
					strcat(pTryInGlobalAbs, pTryInGlobal);
					GG_GetRealPath(pTryInGlobalAbs, false);
					if (FileExist(pTryInGlobalAbs) == 1)
					{
						// we found a script reference that exists in the global folder
						// so we use the global folder version!
						t.entityelement[e].eleprof.aimain_s = pTryInGlobal;
						bReplacedScript = true;
					}
				}
			}
		}
		if (bReplacedScript == true)
		{
			strcpy(cTriggerMessage, "Some behavior(s) have been moved to the new global category");
			iTriggerMessageDelay = 10;
			bTriggerMessage = true;
			iMessageTimer = 0;
		}
	}
	
	bool bAutoCleanUpOldCommunityCoreReferencesBackToStockLatest = true;
	if (bAutoCleanUpOldCommunityCoreReferencesBackToStockLatest == true)
	{
		bool bReplacedAnyScript = false;
		if (g.entityelementlist > 0)
		{
			for (int e = 1; e <= g.entityelementlist; e++)
			{
				char pCurrentScript[MAX_PATH];
				strcpy(pCurrentScript, t.entityelement[e].eleprof.aimain_s.Get());
				strlwr(pCurrentScript);
				LPSTR pPatternToMatch = "community\\6704278\\core\\";
				if ( strstr(pCurrentScript, pPatternToMatch) != NULL )
				{
					strcpy(pCurrentScript, t.entityelement[e].eleprof.aimain_s.Get() + strlen(pPatternToMatch));
					t.entityelement[e].eleprof.aimain_s = pCurrentScript;
					bReplacedAnyScript = true;
				}
			}
		}
		if (bReplacedAnyScript == true)
		{
			strcpy(cTriggerMessage, "Some core behaviors have been updated to the latest version");
			iTriggerMessageDelay = 10;
			bTriggerMessage = true;
			iMessageTimer = 0;
		}
	}

	// free usages
	if ( ArrayCount(t.missingmedia_s) >= 0 ) 
	{
		UnDim (  t.missingmedia_s );
	}
	g.missingmediacounter=0;

	//  Quick update of cursor
	t.lastgrideditselect=-1 ; editor_refresheditmarkers ( );

	//  Recreate all entities in level
	TDRTrace("[LOADMAP] gridedit_updateentityobj loop: %ld entities", g.entityelementlist);
	char debug[MAX_PATH];
	sprintf(debug, "Setup objects: %ld", g.entityelementlist);
	timestampactivity(0, debug);
	extern bool bNoHierarchySorting;
	bNoHierarchySorting = true;
	extern int iInstancedTotal;
	iInstancedTotal = 0;

	if (bKeepWindowsResponding)
		EmptyMessages();

	for ( t.e = 1 ; t.e <=  g.entityelementlist; t.e++ )
	{
		t.tupdatee=t.e ; gridedit_updateentityobj ( );
		if (t.e % 20 == 0)
		{
			if (bKeepWindowsResponding)
				EmptyMessages();
		}
	}
	TDRTrace("[LOADMAP] entity setup done, instanced=%ld", iInstancedTotal);
	timestampactivity(0, "End Setup objects:");

	sprintf(debug, "Instanced objects: %ld", iInstancedTotal);
	timestampactivity(0, debug);
	bNoHierarchySorting = false;
	TDRTrace("[LOADMAP] lighting_refresh");
	lighting_refresh ( );

	//  Ensure newly updated entity does not trigger a terrain update!
	t.terrain.terrainpainteroneshot=0;

	//  Ensure visual indices for sky, terrain and veg up to date (for when we use test game)
	TDRTrace("[LOADMAP] visuals_updateskyterrainvegindex");
	visuals_updateskyterrainvegindex ( );

	//  Refresh any 'shaders' that associat with new entities loaded in
	TDRTrace("[LOADMAP] visuals_justshaderupdate");
	visuals_justshaderupdate ( );

	//  Ensure editor zoom refreshes
	t.updatezoom=1;

	// 161115 - in any event, ensure we generate super texture for 'distant' terrain texture 
	t.visuals.refreshterrainsupertexture = 2;

	//LB: clean any corrupt references out of editor locked list
	for (int i = 0; i < vEntityLockedList.size(); i++)
	{
		int e = vEntityLockedList[i].e;
		if (e < 0 || e >= t.entityelement.size())
		{
			// remove this entry
			vEntityLockedList.erase(vEntityLockedList.begin() + i);
			i--; // adjust index after removal
			continue;
		}
	}

	//PE: Restore locked state. from locked.cfg
	for (int i = 0; i < vEntityLockedList.size(); i++)
	{
		int e = vEntityLockedList[i].e;
		if(e < t.entityelement.size())
			t.entityelement[e].editorlock = 1;
	}
	bForceRefreshLightCount = true;

	// Level has finished loading, so no longer need to store the smart object dummy OBJs
	extern std::vector<int> g_smartObjectDummyEntities;
	g_smartObjectDummyEntities.clear();

	if (bKeepWindowsResponding)
		EmptyMessages();

	// call files modify check function and reset file timestamp map
	extern void CheckExistingFilesModified(bool);
	CheckExistingFilesModified(true);
	TDRTrace("[LOADMAP] gridedit_load_map: EXIT");
}

void gridedit_changemodifiedflag ( void )
{
	// project flag changed, update window Text (  )
	if ( t.game.gameisexe == 0 ) 
	{
		if ( t.lastprojectmodified != g.projectmodified ) 
		{
			t.lastprojectmodified=g.projectmodified;
			gridedit_updateprojectname ( );
		}
		if ( g.projectmodified == 1 && g.projectmodifiedstatic == 1 ) 
		{
			// trigger actions if any modification made
			g.projectmodifiedstatic = 0;
		}
	}
}

void gridedit_updateprojectname ( void )
{
	OpenFileMap (  1,"FPSEXCHANGE" );

	//  add to project title
	if ( strcmp ( Lower(Left(g.projectfilename_s.Get(),Len(g.rootdir_s.Get()))) , Lower(g.rootdir_s.Get()) ) == 0 ) 
	{
		t.tprojname_s=Right(g.projectfilename_s.Get(),Len(g.projectfilename_s.Get())-Len(g.rootdir_s.Get()));
	}
	else
	{
		t.tprojname_s=g.projectfilename_s;
	}
	if (  g.projectmodified != 0  )  t.tprojname_s = t.tprojname_s+"*";

	// 011215 - Add which mode you are in
	int iEditingMode = 0;
	if ( t.grideditselect==0 ) iEditingMode = 1; // terrain
	if ( t.grideditselect==5 && t.gridentitymarkersmodeonly==0 ) iEditingMode = 2; // entity
	if ( t.grideditselect==5 && t.gridentitymarkersmodeonly==1 ) iEditingMode = 3; // markers
	if ( t.grideditselect==6 ) iEditingMode = 4; // waypoints
	switch ( iEditingMode )
	{
		case 1 : t.tprojname_s = t.tprojname_s + cstr("] - [Terrain Editing Mode"); break;
		case 2 : t.tprojname_s = t.tprojname_s + cstr("] - [Entity Editing Mode"); break;
		case 3 : t.tprojname_s = t.tprojname_s + cstr("] - [Marker Only Editing Mode"); break;
		case 4 : t.tprojname_s = t.tprojname_s + cstr("] - [Waypoint Editing Mode"); break;
	}

	// send window title text to IDE
	OpenFileMap(1, "FPSEXCHANGE");
	SetFileMapString (  1, 1000, t.tprojname_s.Get() );
	SetFileMapDWORD (  1, 416, 1 );
	SetEventAndWait (  1 );
	while (  GetFileMapDWORD(1, 416) == 1 ) 
	{
		SetEventAndWait (  1 );
	}
	//  add to recent files list
	if (  g.projectfilename_s != "" ) 
	{
		// 091215 - if folder exists
		if ( PathExist(g.projectfilename_s.Get()) == 1 )
		{
			SetFileMapString(1, 1000, g.projectfilename_s.Get());
			SetFileMapDWORD(1, 438, 1);
			SetEventAndWait(1);
			while (GetFileMapDWORD(1, 438) == 1)
			{
				SetEventAndWait(1);
			}
		}
	}
}

void gridedit_import_ask ( void )
{
	OpenFileMap (  1, "FPSEXCHANGE" );
	SetEventAndWait (  1 );
	do
	{
		t.inputsys.kscancode=GetFileMapDWORD( 1, 100 );

		//PE: Virtual keys should not be included , as if you press a cancel (in IDE) afer that import , import will not open.
	} while ( (  t.inputsys.kscancode > 3 ) );

	// if not already loaded
	if (  t.importer.loaded == 0 ) 
	{
		OpenFileMap (  1,"FPSEXCHANGE" );
		if ( strlen ( t.timporterpath_s.Get() ) == 0 )
		{
			t.strwork = ""; t.strwork = t.strwork + g.rootdir_s+"entitybank\\";
		}
		else
		{
			t.strwork = t.timporterpath_s + "\\";
		}
		SetFileMapString (  1, 1000, t.strwork.Get() );
		t.tdone = 0;
		while (  t.tdone  !=  2 ) 
		{
			if (  t.tdone  ==  0 ) 
			{
				SetFileMapString ( 1 , 1256 , "Choose an X or FBX file for a new object or an .fpe file for existing (*.*)" );
				SetFileMapString (  1, 1512, "Import New Entity" );
			}
			else
			{
				SetFileMapString (  1, 1256, "Please try again ) You must choose either an X or FBX file or an .fpe file! (*.*)" );
				SetFileMapString (  1, 1512, "Invalid File, Please t.try again" );
			}
			SetFileMapDWORD (  1, 424, 1 );
			SetEventAndWait (  1 );
			while (  GetFileMapDWORD(1, 424) == 1 ) 
			{
				SetEventAndWait (  1 );
			}
			t.returnstring_s=GetFileMapString(1, 1000);
			t.tdone = 1;
			if (	strcmp ( Lower(Right(t.returnstring_s.Get(),2)) , ".x" ) == 0 
			||		strcmp ( Lower(Right(t.returnstring_s.Get(),4)) ,".fbx" ) == 0 
			||		strcmp ( Lower(Right(t.returnstring_s.Get(),4)) , ".dbo" ) == 0 
			||		strcmp ( Lower(Right(t.returnstring_s.Get(),4)) , ".fpe" ) == 0 
			||		t.returnstring_s  ==  ""  )  
			{
				t.tdone  =  2;
			}
		}

		// refresh 3d view so dialog Box (  (  not left black Box ) )
		for ( t.tsync = 1 ; t.tsync <=  5 ; t.tsync++ ) { Sync ( ); SleepNow ( 10 );  }

		// if successfully selected a good file extension
		if (  t.returnstring_s != "" ) 
		{
			// load the model
			t.timporterfile_s = t.returnstring_s;
			importer_loadmodel ( );

			// and remember folder we arrived at, so can restore next time we use importer
			LPSTR pReturnedFile = t.timporterfile_s.Get();
			for ( int n = strlen(pReturnedFile); n>0; n-- )
			{
				if ( pReturnedFile[n] == '\\' || pReturnedFile[n] == '/' )
				{
					t.timporterpath_s = t.timporterfile_s;
					LPSTR pImporterPath = t.timporterpath_s.Get();
					pImporterPath[n] = 0;
					break;
				}
			}
		}
	}
}

void gridedit_intercept_savefirst ( void )
{
	t.editorcanceltask=0;
	if (  g.projectmodified == 1 ) 
	{
		OpenFileMap (  1,"FPSEXCHANGE" );
		SetFileMapString (  1, 1000, t.strarr_s[369].Get() );
		SetFileMapString (  1, 1256, t.strarr_s[370].Get() );
		SetFileMapDWORD (  1, 900, 2 );
		SetEventAndWait (  1 );
		while (  GetFileMapDWORD(1, 900) != 0 ) 
		{
			SetEventAndWait (  1 );
		}
		t.tokay=GetFileMapDWORD(1, 904);

		//  refresh 3d view so dialog Box (  (  not left black Box ) )
		for ( t.tsync = 1 ; t.tsync <= 5 ; t.tsync++ ) {  Sync (   ); SleepNow (  10  ); }

		if (  t.tokay == 1 ) 
		{
			//  yes save first
			gridedit_save_map_ask ( );
			g.projectmodified=0  ; gridedit_changemodifiedflag ( );
			g.projectmodifiedstatic = 0;
		}
		if (  t.tokay == 2 ) 
		{
			//  task cancelled
			t.editorcanceltask=1;
		}
	}
}

void gridedit_intercept_savefirst_noreload ( void )
{
	g.savenoreloadflag = 1;
	gridedit_intercept_savefirst();
	g.savenoreloadflag = 0;
}

void gridedit_open_map_ask ( void )
{
	//  SAVE CURRENT (IF ANY)
	t.editorcanceltask=0;
	if (  g.projectmodified == 1 ) 
	{
		//  If project modified, ask if want to save first
		gridedit_intercept_savefirst ( );
	}
	if (  t.editorcanceltask == 0 ) 
	{
		//  OPEN FPM
		OpenFileMap (  1,"FPSEXCHANGE" );
		t.strwork = g.mysystem.mapbankAbs_s;		
		SetFileMapString (  1, 1000, t.strwork.Get() );
		SetFileMapString (  1, 1256, t.strarr_s[371].Get() );
		SetFileMapString (  1, 1512, t.strarr_s[372].Get() );
		SetFileMapDWORD (  1, 424, 1 );
		SetEventAndWait (  1 );
		while (  GetFileMapDWORD(1, 424) == 1 ) 
		{
			SetEventAndWait (  1 );
		}
		t.returnstring_s=GetFileMapString(1, 1000);

		//  refresh 3d view so dialog Box (  (  not left black Box ) )
		for ( t.tsync = 1 ; t.tsync <=  5 ; t.tsync++ ) { Sync ( ); SleepNow ( 10 ); }

		if (  t.returnstring_s != "" ) 
		{
			if (  cstr(Lower(Right(t.returnstring_s.Get(),4))) == ".fpm" ) 
			{
				g.projectfilename_s=t.returnstring_s;
				gridedit_load_map ( );
			}
		}
	}
}

void gridedit_new_map_ask ( void )
{
	//  SAVE CURRENT (IF ANY)
	t.editorcanceltask=0;
	if (  g.projectmodified == 1 ) 
	{
		//  If project modified, ask if want to save first
		gridedit_intercept_savefirst ( );
	}

	if (  t.editorcanceltask == 0 ) 
	{
		//  NEW MAP
		gridedit_new_map ( );
	}
}

void gridedit_save_map_ask ( void )
{
	if (  g.projectfilename_s == "" ) 
	{
		gridedit_saveas_map ( );
	}
	else
	{
		gridedit_save_map ( );
	}
}

void gridedit_saveas_map ( void )
{
	//  SAVE AS DIALOG
	OpenFileMap (  1,"FPSEXCHANGE" );
	t.strwork = g.mysystem.mapbankAbs_s;
	SetFileMapString (  1, 1000, t.strwork.Get() );
	SetFileMapString (  1, 1256, t.strarr_s[373].Get() );
	SetFileMapString (  1, 1512, t.strarr_s[374].Get() );
	SetFileMapDWORD (  1, 428, 1 );
	SetEventAndWait (  1 );
	while (  GetFileMapDWORD(1, 428) == 1 ) 
	{
		SetEventAndWait (  1 );
	}
	t.returnstring_s=GetFileMapString(1, 1000);

	//  refresh 3d view so dialog Box (  (  not left black Box ) )
	for ( t.tsync = 1 ; t.tsync <=  5 ; t.tsync++ ) { Sync ( ); SleepNow ( 10 ); }

	if (  t.returnstring_s != "" ) 
	{
		if (  cstr(Lower(Right(t.returnstring_s.Get(),4))) != ".fpm"  )  t.returnstring_s = t.returnstring_s+".fpm";
		g.projectfilename_s=t.returnstring_s;
		gridedit_save_map ( );
	}
}

void gridedit_addentitytomap(void)
{
	extern bool bUpdateObjectList;
	bUpdateObjectList = true;
	// mark as static if it was
	if (t.gridentitystaticmode == 1) g.projectmodifiedstatic = 1;
	entity_addentitytomap();

	//PE: we loose status somewhere, so force it off after adding a entity to map.
	extern bool bCubesVisible;
	if (bCubesVisible == false) bCubesVisible = true; //Force.

	if (g_UndoSysObjectRememberBeforeMove == true)
	{
		// this happens when object deleted from level (addtocursor) but it could be a move event
		// if the object has not moved/rotated/scaled, we can skip adding a move event
		if (g_UndoSysObjectRememberBeforeMovePX == t.entityelement[t.e].x
			&&  g_UndoSysObjectRememberBeforeMovePY == t.entityelement[t.e].y
			&&  g_UndoSysObjectRememberBeforeMovePZ == t.entityelement[t.e].z
			&&  g_UndoSysObjectRememberBeforeMoveRX == t.entityelement[t.e].rx
			&&  g_UndoSysObjectRememberBeforeMoveRY == t.entityelement[t.e].ry
			&&  g_UndoSysObjectRememberBeforeMoveRZ == t.entityelement[t.e].rz
			&&  g_UndoSysObjectRememberBeforeMoveQuatMode == t.entityelement[t.e].quatmode
			&&  g_UndoSysObjectRememberBeforeMoveQuatX == t.entityelement[t.e].quatx
			&&  g_UndoSysObjectRememberBeforeMoveQuatY == t.entityelement[t.e].quaty
			&&  g_UndoSysObjectRememberBeforeMoveQuatZ == t.entityelement[t.e].quatz
			&&  g_UndoSysObjectRememberBeforeMoveQuatW == t.entityelement[t.e].quatw
			&&  g_UndoSysObjectRememberBeforeMoveSX == t.entityelement[t.e].scalex
			&&  g_UndoSysObjectRememberBeforeMoveSY == t.entityelement[t.e].scaley
			&&  g_UndoSysObjectRememberBeforeMoveSZ == t.entityelement[t.e].scalez )
		{
			// object has not moved, rotated or scaled
		}
		else
		{
			// object has moved, create a move event
			if (g.entityrubberbandlist.size() == 0)
			{
				// but only if single object move, as rubberband has its own multi object move events created when move a rubberband group
				undosys_object_changeposrotscl (g_UndoSysObjectRememberBeforeMoveE,
					g_UndoSysObjectRememberBeforeMovePX,
					g_UndoSysObjectRememberBeforeMovePY,
					g_UndoSysObjectRememberBeforeMovePZ,
					g_UndoSysObjectRememberBeforeMoveRX,
					g_UndoSysObjectRememberBeforeMoveRY,
					g_UndoSysObjectRememberBeforeMoveRZ,
					g_UndoSysObjectRememberBeforeMoveQuatMode,
					g_UndoSysObjectRememberBeforeMoveQuatX,
					g_UndoSysObjectRememberBeforeMoveQuatY,
					g_UndoSysObjectRememberBeforeMoveQuatX,
					g_UndoSysObjectRememberBeforeMoveQuatW,
					g_UndoSysObjectRememberBeforeMoveSX,
					g_UndoSysObjectRememberBeforeMoveSY,
					g_UndoSysObjectRememberBeforeMoveSZ);
			}
		}

		// and return undo sys to normal after this special case
		g_UndoSysObjectRememberBeforeMove = false;
	}
	else
	{
		// regular object addition
		entity_createundoaction(eUndoSys_Object_Add, t.e);
	}

	// if entity is a light, has a probe
	int entid = t.entityelement[t.e].bankindex;
	if (entid > 0)
	{
		if (t.entityprofile[entid].ismarker == 2)
		{
			if (t.entityelement[t.e].eleprof.light.fLightHasProbe >= 50.0f)
			{
				g_bLightProbeScaleChanged = true;
			}
		}

		if (t.entityprofile[entid].ismarker == 12)
		{
			t.entityelement[t.e].eleprof.thumb_aimain_s = "";
		}
	}
	// clear any gridentity light if gridentity no longer used
	if (t.gridentitywickedlightindex > 0)
	{
		WickedCall_DeleteLight(t.gridentitywickedlightindex);
		t.gridentitywickedlightindex = 0;
	}
}

void gridedit_deleteentityfrommap ( void )
{
	// can intercept delete if char+start marker
	t.tstoretentitytoselect=t.tentitytoselect;
	if (  t.playercontrol.thirdperson.enabled == 1 ) 
	{
		if (  t.gridentity == 0 || (t.gridentity>0 && t.entityprofile[t.gridentity].ismarker != 1) ) 
		{
			t.tstmrke=t.playercontrol.thirdperson.startmarkere;
			if (  t.tentitytoselect == t.tstmrke ) 
			{
				//  first delete char on start marker, and restore marker
				t.tentitytoselect=t.playercontrol.thirdperson.charactere;
				t.tstmrkobj=t.entityelement[t.tstmrke].obj;
				if (  t.tstmrkobj>0 ) 
				{
					if (  ObjectExist(t.tstmrkobj) == 1 ) 
					{
						DisableObjectZDepth (  t.tstmrkobj );
						DisableObjectZWrite (  t.tstmrkobj );
						DisableObjectZRead (  t.tstmrkobj );
					}
				}
				//  and reset third person settings
				t.playercontrol.thirdperson.enabled=0;
				t.playercontrol.thirdperson.charactere=0;
				t.playercontrol.thirdperson.startmarkere=0;
			}
		}
	}

	// if entity is a light, has a probe
	int entid = t.entityelement[t.tentitytoselect].bankindex;
	if (entid > 0)
	{
		if (t.entityprofile[entid].ismarker == 2)
		{
			if (t.entityelement[t.tentitytoselect].eleprof.light.fLightHasProbe >= 50.0f)
			{
				g_bLightProbeScaleChanged = true;
			}
		}
	}

	//If particle delete the effect.
	if (g_UndoSysObjectIsBeingMoved != true)
	{
		if (entid > 0 && t.entityprofile[entid].ismarker == 10)
		{
			int iParticleEmitter = t.entityelement[t.tentitytoselect].eleprof.newparticle.emitterid;
			if (iParticleEmitter != -1)
			{
				gpup_deleteEffect(iParticleEmitter);
				t.entityelement[t.tentitytoselect].eleprof.newparticle.emitterid = -1;
			}
		}
	}

	// mark as static if it was
	if ( t.entityelement[t.tentitytoselect].staticflag == 1 ) g.projectmodifiedstatic = 1;
	int te = t.tentitytoselect;
	if (g_UndoSysObjectIsBeingMoved == true)
	{
		// its move - so we store the change posrotscl event so we know where the entity came from
		g_UndoSysObjectRememberBeforeMove = true;
		g_UndoSysObjectRememberBeforeMoveE = te;
		g_UndoSysObjectRememberBeforeMovePX = t.entityelement[te].x;
		g_UndoSysObjectRememberBeforeMovePY = t.entityelement[te].y;
		g_UndoSysObjectRememberBeforeMovePZ = t.entityelement[te].z;
		g_UndoSysObjectRememberBeforeMoveRX = t.entityelement[te].rx;
		g_UndoSysObjectRememberBeforeMoveRY = t.entityelement[te].ry;
		g_UndoSysObjectRememberBeforeMoveRZ = t.entityelement[te].rz;
		g_UndoSysObjectRememberBeforeMoveQuatMode = t.entityelement[te].quatmode;
		g_UndoSysObjectRememberBeforeMoveQuatX = t.entityelement[te].quatx;
		g_UndoSysObjectRememberBeforeMoveQuatY = t.entityelement[te].quaty;
		g_UndoSysObjectRememberBeforeMoveQuatZ = t.entityelement[te].quatz;
		g_UndoSysObjectRememberBeforeMoveQuatW = t.entityelement[te].quatw;
		g_UndoSysObjectRememberBeforeMoveSX = t.entityelement[te].scalex;
		g_UndoSysObjectRememberBeforeMoveSY = t.entityelement[te].scaley;
		g_UndoSysObjectRememberBeforeMoveSZ = t.entityelement[te].scalez;
	}
	else
	{
		if (t.entityelement[te].eleprof.trigger.waypointzoneindex > 0)
		{
			// if its a zone, create multiple events - one for middle object and one for the waypoint zone data so it can be restored later.
			undosys_multiplevents_start();
			entity_createundoaction(eUndoSys_Object_Delete, te);
			entity_createundoaction(eUndoSys_Object_DeleteWaypoint, te);
			undosys_multiplevents_finish();
		}
		else
		{
			entity_createundoaction(eUndoSys_Object_Delete, te);
		}
			
		
	}
	entity_deleteentityfrommap ( );

	//  restore tentitytoselect in case switched it
	t.tentitytoselect=t.tstoretentitytoselect;
}

void gridedit_deleteentityrubberbandfrommap ( void )
{
	undosys_multiplevents_start();

	// will delete all entities in rubber band list, and preserve them into undo buffer
	for ( int i = 0; i < (int)g.entityrubberbandlist.size(); i++ )
	{
		t.tentitytoselect = g.entityrubberbandlist[i].e;
		if ( t.tentitytoselect > 0 && t.entityelement[t.tentitytoselect].editorlock == 0)
		{
			DeleteEntityFromLists(t.tentitytoselect);

			if ( t.entityelement[t.tentitytoselect].staticflag == 1 ) g.projectmodifiedstatic = 1;
			gridedit_deleteentityfrommap ( );
			g.entityrubberbandlistundo.push_back ( t.entityundo );
		}
	}
	undosys_multiplevents_finish();
}

void gridedit_moveentityrubberband ( void )
{
	// will move all entities in rubber band list, and preserve them into undo buffer
	undosys_multiplevents_start();
	for ( int i = 0; i < (int)g.entityrubberbandlist.size(); i++ )
	{
		int te = g.entityrubberbandlist[i].e;
		undosys_object_changeposrotscl (te, g.entityrubberbandlist[i].px,
			g.entityrubberbandlist[i].py,
			g.entityrubberbandlist[i].pz,
			g.entityrubberbandlist[i].rx,
			g.entityrubberbandlist[i].ry,
			g.entityrubberbandlist[i].rz,
			g.entityrubberbandlist[i].quatmode,
			g.entityrubberbandlist[i].quatx,
			g.entityrubberbandlist[i].quaty,
			g.entityrubberbandlist[i].quatz,
			g.entityrubberbandlist[i].quatw,
			g.entityrubberbandlist[i].scalex,
			g.entityrubberbandlist[i].scaley,
			g.entityrubberbandlist[i].scalez);
	}
	undosys_multiplevents_finish();

	// also, update rubberband to new entity states, so can move multiple times and undo them
	for (int i = 0; i < (int)g.entityrubberbandlist.size(); i++)
	{
		int te = g.entityrubberbandlist[i].e;
		if (te > 0 && te < t.entityelement.size())
		{
			g.entityrubberbandlist[i].px = t.entityelement[te].x;
			g.entityrubberbandlist[i].py = t.entityelement[te].y;
			g.entityrubberbandlist[i].pz = t.entityelement[te].z;
			g.entityrubberbandlist[i].rx = t.entityelement[te].rx;
			g.entityrubberbandlist[i].ry = t.entityelement[te].ry;
			g.entityrubberbandlist[i].rz = t.entityelement[te].rz;
			g.entityrubberbandlist[i].quatmode = t.entityelement[te].quatmode;
			g.entityrubberbandlist[i].quatx = t.entityelement[te].quatx;
			g.entityrubberbandlist[i].quaty = t.entityelement[te].quaty;
			g.entityrubberbandlist[i].quatz = t.entityelement[te].quatz;
			g.entityrubberbandlist[i].quatw = t.entityelement[te].quatw;
			g.entityrubberbandlist[i].scalex = t.entityelement[te].scalex;
			g.entityrubberbandlist[i].scaley = t.entityelement[te].scaley;
			g.entityrubberbandlist[i].scalez = t.entityelement[te].scalez;
		}
		else
		{
			// bug somewhere, e was assigned to rubberband but this entity does not exist!!
			g.entityrubberbandlist[i].e = 0;
		}
	}
}

void gridedit_updateentityobj ( void )
{
	//  moved to m-entity
	entity_updateentityobj ( );
}

