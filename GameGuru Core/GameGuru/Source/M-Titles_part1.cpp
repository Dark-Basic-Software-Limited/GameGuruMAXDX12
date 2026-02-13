void GetStoryboardCustomScreenNodeName(int iNode, char* pRealNameStr);
int GetStoryboardCustomScreenNode(char* page);
void titleslua_main_stage2_preloop(void)
{
	char pRealNameOfScreen[256];
	strcpy(pRealNameOfScreen, g_pTitleCurrentPage);

	// call init function to set up resources
	char pLUAInit[256];
	int CustomScreenNode = 0;
	if (strncmp(g_pTitleCurrentPage, ":node:", 6) == NULL)
	{
		int realnodeid = atoi(t.game.pSwitchToPage + 6);
		GetStoryboardCustomScreenNodeName(realnodeid, pRealNameOfScreen);
		CustomScreenNode = GetStoryboardCustomScreenNode(pRealNameOfScreen);
	}
	else
		CustomScreenNode = GetStoryboardCustomScreenNode(g_pTitleCurrentPage);

	if (CustomScreenNode >= 0)
	{
		strcpy(pLUAInit,"custom_init");
		strcpy(g_strErrorClue, pLUAInit);
		LuaSetFunction(pLUAInit, 1, 0);
		LuaPushString(pRealNameOfScreen);
		LuaCall();
	}
	else
	{
		strcpy(pLUAInit, cstr(cstr(pRealNameOfScreen) + "_init").Get());
		strcpy(g_strErrorClue, pLUAInit);
		LuaSetFunction(pLUAInit, 0, 0);
		LuaCall();
	}

	// stay in main until page is quit
	strcpy(g_pTitleLUAMain, cstr(cstr(pRealNameOfScreen) + "_main").Get());
	strcpy(g_strErrorClue, g_pTitleLUAMain);
	t.game.titleloop = 1;
}

bool bForceRender = false;
void titleslua_main_stage3_inloop(void)
{
	// title loop
	char pRealNameOfScreen[256];
	strcpy(pRealNameOfScreen, g_pTitleCurrentPage);


	//PE: Make sure to show everything on first frame.
	extern int iBlockRenderingForFrames;
	extern bool g_bNoSwapchainPresent;
	extern bool bBlockImGuiUntilNewFrame;
	if (iBlockRenderingForFrames > 0 || g_bNoSwapchainPresent || bBlockImGuiUntilNewFrame)
	{
		iBlockRenderingForFrames = 0;
		g_bNoSwapchainPresent = false;
		bBlockImGuiUntilNewFrame = false;
	}

	// Machine independent speed update (makes g_TimeElapsed available)
	game_timeelapsed();

	// run LUA logic
	lua_loop_begin();

	//int CustomScreenNode = GetStoryboardCustomScreenNode(g_pTitleCurrentPage);
	int CustomScreenNode = 0;
	if (strncmp(g_pTitleCurrentPage, ":node:", 6) == NULL)
	{
		int realnodeid = atoi(t.game.pSwitchToPage + 6);
		GetStoryboardCustomScreenNodeName(realnodeid, pRealNameOfScreen);
		CustomScreenNode = GetStoryboardCustomScreenNode(pRealNameOfScreen);
	}
	else
		CustomScreenNode = GetStoryboardCustomScreenNode(g_pTitleCurrentPage);

	if (CustomScreenNode >= 0)
	{
		LuaSetFunction("custom_main", 0, 0);
		LuaCall();
	}
	else
	{
		LuaSetFunction(g_pTitleLUAMain, 0, 0);
		LuaCall();
	}
	lua_loop_finish();

	// if in game, extra update refreshes
	if ( t.game.gameloop == 1 ) 
	{
		music_loop ( );
		t.gdynamicterrainshadowcameratrigger=1;
		t.tmastersyncmask=0;
		SyncMask (  t.tmastersyncmask+(1<<3)+(1) );
	}

	// draw all sprites required
	int fhide = g.tabmodehidehuds;
	if (bForceRender)
		g.tabmodehidehuds = 1;
	sliders_draw ( );

	g.tabmodehidehuds = fhide;

	extern bool bJustRederedScreenEditor;
	extern int g_iInGameMenuState;
	if (!bJustRederedScreenEditor && g_iInGameMenuState != 1 )
	{
		//PE: We need to render everything. This is like the old Sync in Wicked.
	}
	if (bForceRender)
	{
		void StartForceRender(void);
		StartForceRender();
	}
}
void titleslua_main_stage4_afterloop(void)
{
	// title afterloop
	char pRealNameOfScreen[256];
	strcpy(pRealNameOfScreen, g_pTitleCurrentPage);

	// wait for mouse click release
	while ( MouseClick()!=0 ) { Sleep(1); }

	int CustomScreenNode = 0;
	if (strncmp(g_pTitleCurrentPage, ":node:", 6) == NULL)
	{
		int realnodeid = atoi(t.game.pSwitchToPage + 6);
		GetStoryboardCustomScreenNodeName(realnodeid, pRealNameOfScreen);
		CustomScreenNode = GetStoryboardCustomScreenNode(pRealNameOfScreen);
	}
	else
		CustomScreenNode = GetStoryboardCustomScreenNode(g_pTitleCurrentPage);

	// call to allow local resources to be freed
	char pLUAFree[256];
	strcpy (pLUAFree, cstr(cstr(pRealNameOfScreen) + "_free").Get());

	if (CustomScreenNode >= 0)
	{
		LuaSetFunction("custom_free", 0, 0);
		LuaCall();
	}
	else
	{
		LuaSetFunction(pLUAFree, 0, 0);
		LuaCall();
	}
	// switch to new page name
	if (strcmp (t.game.pSwitchToPage, "-1") == NULL)
	{
		strcpy (g_pTitleCurrentPage, t.game.pSwitchToLastPage);
	}
	else
	{
		strcpy (g_pTitleCurrentPage, t.game.pSwitchToPage);
	}

	// ensure IMGUI does not attempt to render to wicked during resource shifting (between page unload/load)
	extern bool bBlockImGuiUntilNewFrame;
	bBlockImGuiUntilNewFrame = true;
}
void titleslua_main_stage5(void)
{
	// currentpage afterloop
	// need to ensure low FPS warning not triggered by game sync absense
	t.conkit.cooldown = 100;
}
bool titleslua_main_loopcode(void)
{
	if (g_iTitleMainState == 0)
	{
		// current page loop
		if (strcmp(g_pTitleCurrentPage, "") != NULL) 
			g_iTitleMainState = 1;
		else
			g_iTitleMainState = 4;
	}
	if (g_iTitleMainState == 1)
	{
		// title pre
		titleslua_main_stage2_preloop();
		g_iTitleMainState = 2;
	}
	if (g_iTitleMainState == 2)
	{
		// title loop
		if (t.game.titleloop == 1) 
			titleslua_main_stage3_inloop();
		else
			g_iTitleMainState = 3;
	}
	if (g_iTitleMainState == 3)
	{
		// title post
		titleslua_main_stage4_afterloop();
		g_iTitleMainState = 0;
	}
	if (g_iTitleMainState == 4)
	{
		// currentpage post
		titleslua_main_stage5();
		g_iTitleMainState = 0;
		return true;
	}
	return false;
}


void titleslua_blocking_run(void)
{
	bool bRunLoop = true;
	MSG msg = { 0 };
	while (bRunLoop == true)
	{
		//PE: Make sure to empty messages , or win will think we are down.
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			bForceRender = true;
			if (titleslua_main_loopcode() == true) bRunLoop = false;
			bForceRender = false;
		}
	}
}

void titleslua_main(LPSTR pPageName)
{
	// now used to trigger title system
	titleslua_main_stage1_init(pPageName);
	g_iTitleMainState = 0;
}

void titleslua_free ( void )
{
	// frees all loaded scripts
	t.bThemeScriptsLoaded = false;
}
