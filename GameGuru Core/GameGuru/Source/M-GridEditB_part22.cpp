// 2026-08-05: automation TITLE_CLICK support — pending widget action queued by the
// harness, consumed by the widget hit-test loop below as if hovered+released
int g_iAutoTriggerScreenAction = 0;
extern "C" int GGAuto_MapScreenActionName(const char* name)
{
	if (stricmp(name, "start") == 0) return (int)STORYBOARD_ACTIONS_STARTGAME;
	if (stricmp(name, "exit") == 0) return (int)STORYBOARD_ACTIONS_EXITGAME;
	if (stricmp(name, "continue") == 0) return (int)STORYBOARD_ACTIONS_CONTINUE;
	if (stricmp(name, "back") == 0) return (int)STORYBOARD_ACTIONS_BACK;
	if (stricmp(name, "resume") == 0) return (int)STORYBOARD_ACTIONS_RESUMEGAME;
	if (stricmp(name, "leave") == 0) return (int)STORYBOARD_ACTIONS_LEAVEGAME;
	return 0;
}

int screen_editor(int nodeid, bool standalone, char *screen)
{
	extern bool g_bNoGGUntilGameGuruMainCalled;
	extern int iSpecialLuaReturn;
	iSpecialLuaReturn = -1;
	if (!g_bNoGGUntilGameGuruMainCalled)
		return -1;
	{
		ImGuiContext& gi = *GImGui;
		if (!gi.Font->IsLoaded())
			return -1;
	}

	int iRet = -1;
	int nodeidStore = nodeid;
	static bool bScreenToggleKeyWindow = false;

	ImGuiWindow* window = NULL; //ImGui::GetCurrentWindow();
	if (standalone)
	{
		bJustRederedScreenEditor = true;
		// new frame if about to use imgui to render this sprite draw list
		if ((bImGuiInTestGame) && !bRenderTabTab && !bImGuiFrameState)
		{
			//We need a new frame.
			ImGui_ImplDX11_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();
			bRenderTabTab = true;
			bBlockImGuiUntilNewFrame = false;
			extern bool bSpriteWinVisible;
			bSpriteWinVisible = false;
			bImGuiRenderWithNoCustomTextures = false;
		}
		if (bImGuiInTestGame)
		{
			//From lua, we need to update imgui mousepos ...
			int mclick = 0;
			ImGuiIO& io = ImGui::GetIO();
			io.MouseDown[0] = 0; //PE: Mouse (release) is also lost inside blocking dialogs. Reset!
			io.MouseDown[1] = 0;
			io.MouseDown[2] = 0;
			io.MouseDown[3] = 0;
			if (g.luaactivatemouse > 0)
			{
				//PE: Use fFinalPercX,Y so VR is also supported.
				float mouseposx = LuaMousePosPercentX * ( (float) GetDisplayWidth() / 100.0f );
				float mouseposy = LuaMousePosPercentY * ( (float) GetDisplayHeight() / 100.0f);

				mclick = LuaMouseClick;
				io.MousePos.x = mouseposx; //LuaMousePosX; //MouseX();
				io.MousePos.y = mouseposy; //LuaMousePosY; //MouseY();
			}
			else
			{
				mclick = MouseClick();
			}
			if (mclick == 1) io.MouseDown[0] = 1;
			if (mclick == 2) io.MouseDown[1] = 1;
			if (mclick == 4) io.MouseDown[2] = 1;
		}
	}

	if (standalone && screen != NULL)
	{
		//Find node to display in standalone.
		nodeid = FindLuaScreenNode(screen);
		if (nodeid < 0 && g.tabmodehidehuds <= 0 && stricmp(screen, "hud0") == 0) //&& stricmp(screen, "hud0.lua") == 0)
		{
			// Special case to handle the In-Game HUD screen missing from storyboard project (from playing an old project before it was added)
			// This would not work in standalone since the default project.dat is missing 
			// Don't want to always add that file to all standalones (extra bloat) and its only for old projects that haven't had their storyboard loaded...
			// ... so this should cover 99% of cases where it is missing
			int preview_size_x = ImGui::GetMainViewport()->Size.x - 270;
			float fNodeWidth = 180.0f;
			float fNodeHeight = 130.0f;
			nodeid = storyboard_add_missing_nodex(13, preview_size_x, fNodeWidth, fNodeHeight + 20, false);
			{
				// Some users report runtime error 501, when the above code is called, regenerate unique IDs for the images just in-case
				int iUniqueIds = STORYBOARD_THUMBS;
				int iUniqueIdsAdd = 1000;
				if (nodeid >= 200)
					iUniqueIdsAdd = 200000;
				else if (nodeid >= 100)
					iUniqueIdsAdd = 100000;

				Storyboard.Nodes[nodeid].id = iUniqueIds;
				Storyboard.Nodes[nodeid].thumb_id = iUniqueIds;

				for (int l = 0; l < STORYBOARD_MAXOUTPUTS; l++)
				{
					Storyboard.Nodes[nodeid].input_id[l] = iUniqueIds + iUniqueIdsAdd + (1000 * l);
					Storyboard.Nodes[nodeid].output_id[l] = iUniqueIds + iUniqueIdsAdd + (1000 * l) + 500;
				}

				for (int l = 0; l < STORYBOARD_MAXWIDGETS; l++)
				{
					Storyboard.Nodes[nodeid].widget_normal_thumb_id[l] = iUniqueIds + iUniqueIdsAdd + (1000 * l) + 600;
					Storyboard.Nodes[nodeid].widget_highlight_thumb_id[l] = iUniqueIds + iUniqueIdsAdd + (1000 * l) + 700;
					Storyboard.Nodes[nodeid].widget_selected_thumb_id[l] = iUniqueIds + iUniqueIdsAdd + (1000 * l) + 800;
				}
				Storyboard.Nodes[nodeid].screen_backdrop_id = iUniqueIds + 500;

				iUniqueIds++;
			}
		}
	}
	
	if (standalone) bPreviewScreen = true;

	if (nodeid >= 0 && ( iLastNode != nodeid || bLastStandalone != standalone ) )
	{
		//Load in lua ...
		//editors\\templates\\lua\\loading.lua
		//PE: Get loadgame data.
		//savegames\\gameslot1.dat
		bool bLoadSlots = false;
		if (stricmp(Storyboard.Nodes[nodeid].lua_name, "loadgame.lua") == 0 || stricmp(Storyboard.Nodes[nodeid].lua_name, "savegame.lua") == 0)
		{
			bLoadSlots = true;
		}

		for (int i = 1; i <= 8;i++)
		{
			char slotname[256];
			sprintf(LoadGameTitle[i], "%d: EMPTY PROGRESS SLOT", i);
			if (!standalone)
			{
				// Storyboard (i.e. NOT STANDALONE) should only show placeholder slot text
				strcpy(LoadGameTitle[i], "EMPTY PROGRESS SLOT");
			}
			else
			{
				sprintf(slotname, "savegames\\gameslot%d.dat", i);
				FILE* fFile = GG_fopen(slotname, "r");
				if (fFile)
				{
					char ctmp[256];
					bool bStart = false;
					fgets(ctmp, 256 - 1, fFile);
					fgets(ctmp, 256 - 1, fFile);
					fgets(ctmp, 256 - 1, fFile);
					if (strlen(ctmp) > 0 && ctmp[strlen(ctmp) - 1] == '\n')
						ctmp[strlen(ctmp) - 1] = 0;
					if (strlen(ctmp) > 0)
					{
						strcpy(LoadGameTitle[i], ctmp);
					}
					fclose(fFile);
				}
			}
		}

		if (standalone)
		{
			int iFreeSoundID = g.temppreviewsoundoffset + 2;
			bool bKeepPlaying = false;
			if (SoundExist(iFreeSoundID) == 1 && SoundPlaying(iFreeSoundID) == 1)
			{
				if (iLastNode >= 0 && strlen(Storyboard.Nodes[iLastNode].screen_music) <= 0)
				{
					//Last node did not have any music, if still playing keep playing.
					bKeepPlaying = true;
				}
			}
			if (!bKeepPlaying)
			{
				if (strlen(Storyboard.Nodes[nodeid].screen_music) > 0)
				{
					// play music
					if (SoundExist(iFreeSoundID) == 1) DeleteSound(iFreeSoundID);
					if (FileExist(Storyboard.Nodes[nodeid].screen_music) == 1)
					{
						LoadSound(Storyboard.Nodes[nodeid].screen_music, iFreeSoundID, 0, 1);
						if (SoundExist(iFreeSoundID) == 1)
						{
							if(Storyboard.Nodes[nodeid].loop_music)
								LoopSound(iFreeSoundID);
							else
								PlaySound(iFreeSoundID);
						}
					}
				}
			}
		}
		else
		{
			bPreviewScreen = false;
		}
		Storyboard_ActiveWidgets.clear();

		for (int i = 0; i < STORYBOARD_MAXWIDGETS; i++)
		{
			bool bWidgetUsed = false;
			if (Storyboard.Nodes[nodeid].widget_used[i] == 1)
			{
				//Load in any images.
				image_setlegacyimageloading(true);
				if (strlen(Storyboard.Nodes[nodeid].widget_normal_thumb[i]) > 0)
				{
					LoadImage(Storyboard.Nodes[nodeid].widget_normal_thumb[i], Storyboard.Nodes[nodeid].widget_normal_thumb_id[i]);
				}
				if (strlen(Storyboard.Nodes[nodeid].widget_highlight_thumb[i]) > 0)
				{
					LoadImage(Storyboard.Nodes[nodeid].widget_highlight_thumb[i], Storyboard.Nodes[nodeid].widget_highlight_thumb_id[i]);
				}
				if (strlen(Storyboard.Nodes[nodeid].widget_selected_thumb[i]) > 0)
				{
					LoadImage(Storyboard.Nodes[nodeid].widget_selected_thumb[i], Storyboard.Nodes[nodeid].widget_selected_thumb_id[i]);
				}
				image_setlegacyimageloading(false);
				bWidgetUsed = true;
			}
			bool bHardcodedID = IsHardcodedID(nodeid, i);
			if(bWidgetUsed==true || bHardcodedID==true)
			Storyboard_ActiveWidgets.push_back(i);
		}

		iUpdateBackDropNode = nodeid;
		iCurrentSelectedWidget = -1;
		iLastNode = nodeid;
		bLastStandalone = standalone;
	}
	if (iUpdateWidgetThumbNode >= 0)
	{
		image_setlegacyimageloading(true);
		if (iUpdateWidgetThumbButton >= 0)
		{
			if (strlen(Storyboard.Nodes[nodeid].widget_normal_thumb[iUpdateWidgetThumbButton]) > 0)
			{
				LoadImage(Storyboard.Nodes[nodeid].widget_normal_thumb[iUpdateWidgetThumbButton], Storyboard.Nodes[nodeid].widget_normal_thumb_id[iUpdateWidgetThumbButton]);
			}
			if (strlen(Storyboard.Nodes[nodeid].widget_highlight_thumb[iUpdateWidgetThumbButton]) > 0)
			{
				LoadImage(Storyboard.Nodes[nodeid].widget_highlight_thumb[iUpdateWidgetThumbButton], Storyboard.Nodes[nodeid].widget_highlight_thumb_id[iUpdateWidgetThumbButton]);
			}
			if (strlen(Storyboard.Nodes[nodeid].widget_selected_thumb[iUpdateWidgetThumbButton]) > 0)
			{
				LoadImage(Storyboard.Nodes[nodeid].widget_selected_thumb[iUpdateWidgetThumbButton], Storyboard.Nodes[nodeid].widget_selected_thumb_id[iUpdateWidgetThumbButton]);
			}
		}
		image_setlegacyimageloading(false);
		iUpdateWidgetThumbNode = -1;
	}
	if (iUpdateBackDropNode >= 0)
	{
		if (iUpdateBackDropNode == 99999) //All
		{
			for (int i = 0; i < STORYBOARD_MAXWIDGETS; i++)
			{
				iUpdateBackDropNode = i;
				if (Storyboard.Nodes[iUpdateBackDropNode].type == STORYBOARD_TYPE_SCREEN)
				{
					///PE: Not transparent screens.
					{
						if (ImageExist(Storyboard.Nodes[iUpdateBackDropNode].screen_backdrop_id)) DeleteImage(Storyboard.Nodes[iUpdateBackDropNode].screen_backdrop_id);

						//PE: Also need a thumb ?
						strcpy(Storyboard.Nodes[iUpdateBackDropNode].screen_backdrop, cCopyToAllScreens);
						
						if (strlen(Storyboard.Nodes[iUpdateBackDropNode].screen_backdrop) > 0)
						{
							//Backdrop.
							image_setlegacyimageloading(true);
							LoadImage(Storyboard.Nodes[iUpdateBackDropNode].screen_backdrop, Storyboard.Nodes[iUpdateBackDropNode].screen_backdrop_id);
							image_setlegacyimageloading(false);
						}
					}
				}
			}
		}
		else
		{
			if (ImageExist(Storyboard.Nodes[iUpdateBackDropNode].screen_backdrop_id)) DeleteImage(Storyboard.Nodes[iUpdateBackDropNode].screen_backdrop_id);
			if (strlen(Storyboard.Nodes[iUpdateBackDropNode].screen_backdrop) > 0)
			{
				//Backdrop.
				image_setlegacyimageloading(true);
				LoadImage(Storyboard.Nodes[iUpdateBackDropNode].screen_backdrop, Storyboard.Nodes[iUpdateBackDropNode].screen_backdrop_id);
				image_setlegacyimageloading(false);
			}
		}
		iUpdateBackDropNode = -1;
	}

	static int iQuitWindowLoop = 0;
	// Automation harness trigger for Exit to Storyboard
	extern int g_iAutoExitScreenEditor;
	if (g_iAutoExitScreenEditor && iQuitWindowLoop <= 0 && !standalone)
	{
		g_iAutoExitScreenEditor = 0;
		iQuitWindowLoop = 4;
	}
	if ((bScreen_Editor_Window || standalone) && nodeid >= 0)
	{
		if (standalone)
		{
			ImGuiViewport* viewport = ImGui::GetMainViewport();

			ImVec2 viewPortPos = ImGui::GetMainViewport()->Pos;
			ImVec2 viewPortSize = ImGui::GetMainViewport()->Size;
			ImGui::SetNextWindowPos(viewPortPos, ImGuiCond_Always);
			ImGui::SetNextWindowSize(viewPortSize, ImGuiCond_Always);
			ImGui::SetNextWindowViewport(viewport->ID);

			ImVec4 style_winback = ImGui::GetStyle().Colors[ImGuiCol_WindowBg];
			style_winback.w = 1.0f;
			ImGui::PushStyleColor(ImGuiCol_WindowBg, style_winback);


			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);


			int flags = ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
			bool bopen = ImGui::Begin("##StoryboardStandaloneWindow", &bStoryboardWindow, flags); // ImGuiWindowFlags_NoScrollbar
		}

#ifdef EMULATERESOLUTION

		static bool bInitResolutionCheck = true;
		static char CurrentResolution[256] = "\0";

		if (bInitResolutionCheck)
		{
			GetActiveMonitorResolution();
			if (CurrentMonitorResolutionX == 0 || CurrentMonitorResolutionY == 0)
			{
				CurrentMonitorResolutionX = master.masterrenderer.GetLogicalWidth();
				CurrentMonitorResolutionY = master.masterrenderer.GetLogicalHeight();
			}
			bInitResolutionCheck = false;
			sprintf(CurrentResolution, "Current Resolution %dx%d", CurrentMonitorResolutionX, CurrentMonitorResolutionY);
		}
		static int monitor_size_x = CurrentMonitorResolutionX;
		static int monitor_size_y = CurrentMonitorResolutionY;

		int preview_size_x = ImGui::GetMainViewport()->Size.x - 270;
		int preview_size_y = ImGui::GetMainViewport()->Size.y - 30.0;
		if (standalone)
		{
			preview_size_x = ImGui::GetMainViewport()->Size.x;
			preview_size_y = ImGui::GetMainViewport()->Size.y;
		}
		else if (bPreviewScreen)
		{
			preview_size_x = ImGui::GetMainViewport()->Size.x - 100;
			preview_size_y = ImGui::GetMainViewport()->Size.y;
		}

#else
		int preview_size_x = ImGui::GetMainViewport()->Size.x -270;
		int preview_size_y = ImGui::GetMainViewport()->Size.y -30.0;
		if (bPreviewScreen)
		{
			preview_size_x = ImGui::GetMainViewport()->Size.x;
			preview_size_y = ImGui::GetMainViewport()->Size.y;
		}
#endif

		float fStartWinPosY = ImGui::GetCursorPosY();

		if (!bPreviewScreen)
		{
			ImGui::Columns(2, "StoryboardEditorWindowColumns", false);  //false no border
			ImGui::SetColumnOffset(0, 0.0f);
			ImGui::SetColumnOffset(1, preview_size_x+10);
		}

		//if (!standalone) window = ImGui::GetCurrentWindow();
		window = ImGui::GetCurrentWindow();

		ImVec2 vCurPos = ImGui::GetCursorPos();
		ImVec2 vIconSize = { (float)ImGui::GetFontSize()*3.5f, (float)ImGui::GetFontSize()*3.5f };
		if (!standalone)
		{

			ImGui::SetCursorPos(ImVec2(3.0f, fStartWinPosY + 1.0f));
			ImGui::SetItemAllowOverlap();
			if (ImGui::ImgBtn(TOOL_GOBACK, vIconSize, ImVec4(0, 0, 0, 0), drawCol_normal, drawCol_hover, drawCol_Down, 0, 0, 0, 0, false, false, false, false, false, bBoostIconColors))
			{
				//Everything already saved.
				if (bPreviewScreen)
				{
					//Stop music ...
					int iFreeSoundID = g.temppreviewsoundoffset + 2;
					if (SoundExist(iFreeSoundID) == 1 && SoundPlaying(iFreeSoundID) == 1)
					{
						// stop currently playing preview
						StopSound(iFreeSoundID);
					}
					bPreviewScreen = false;
				}
				else
				{
					int iFreeSoundID = g.temppreviewsoundoffset + 2;
					if (SoundExist(iFreeSoundID) == 1 && SoundPlaying(iFreeSoundID) == 1)
					{
						// stop currently playing preview
						StopSound(iFreeSoundID);
					}

					iQuitWindowLoop = 4;
				}
			}
			if (!bPreviewScreen)
			{
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", "Exit to Storyboard");
			}
			else
			{
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", "Exit Preview");
			}
		}

		//Toolbar right aligned.
		cstr UniqueBackdropSelect = "##StoryboardSelectBackdrop";
		cstr UniqueMusicSelect = "##StoryboardSelectMusic";
		if (iSelectedLibraryStingReturnID == window->GetID(UniqueBackdropSelect.Get()))
		{
			strcpy(Storyboard.Nodes[nodeid].screen_backdrop, sSelectedLibrarySting.Get());
			iUpdateBackDropNode = nodeid; //Update thumb.
			sSelectedLibrarySting = "";
			iSelectedLibraryStingReturnID = -1; //disable.
		}
		if (iSelectedLibraryStingReturnID == window->GetID(UniqueMusicSelect.Get()))
		{
			strcpy(Storyboard.Nodes[nodeid].screen_music, sSelectedLibrarySting.Get());
			sSelectedLibrarySting = "";
			iSelectedLibraryStingReturnID = -1; //disable.
		}

		if (!bPreviewScreen)
		{
			//Display title in center
			ImGui::SetCursorPos(vCurPos);
			ImGui::Text("");
			ImGui::SetWindowFontScale(1.4);
			cstr luatitle = cstr("Editing: ") + cstr(Storyboard.Nodes[nodeid].title);
			ImGui::TextCenter(luatitle.Get()); // lua_name
			ImGui::SetWindowFontScale(1.0);
			ImGui::Text("");
		}

		if ( bPreviewScreen ) ImGui::SetCursorPos(vCurPos+ImVec2(0,13));
		if ( standalone ) ImGui::SetCursorPos(vCurPos);

		ImVec2 vHeaderEnd = ImGui::GetCursorPos();

		//ImGui::Separator(); //Ruin Columns ?
		ImVec2 vMonitorPos = ImVec2(0, -8.0);
		ImVec2 vMonitorBorder = ImVec2(15, 15);/*ImVec2(20, 20);*/
		ImVec2 vViewportSize = ImGui::GetMainViewport()->Size;

		int panelWidth = 250;


#ifdef EMULATERESOLUTION
		float fRatio = (float) monitor_size_x / (float) monitor_size_y; // Default ratio 1920x1080
		float fRatioInv = (float) monitor_size_y / (float) monitor_size_x;
		if (standalone)
		{
			fRatio = vViewportSize.x / vViewportSize.y;
			fRatioInv = vViewportSize.y / vViewportSize.x;
		}
#else
		float fRatio = vViewportSize.x / vViewportSize.y; // Default ratio 1920x1080
		/*float fRatioInv = 0.5625;*/
		float fRatioInv = vViewportSize.y / vViewportSize.x;
#endif
		float fMaxMonitorY = preview_size_y - vHeaderEnd.y; // -ImGui::GetFontSize();
		if (bPreviewScreen) fMaxMonitorY -= 10.0f;
		ImVec2 vMonitorSize;
		float vMonitorCenterX;
		if (standalone)
		{
			vMonitorPos = ImVec2(0, 0);
			fMaxMonitorY = preview_size_y;
			vMonitorSize = ImVec2(preview_size_x , preview_size_y);
			vMonitorSize.y = vMonitorSize.x * fRatioInv;
			if (vMonitorSize.y > fMaxMonitorY )
			{
				vMonitorSize.y = fMaxMonitorY;
				vMonitorSize.x = vMonitorSize.y * fRatio;
			}
			vMonitorCenterX = preview_size_x - vMonitorSize.x;
		}
		else
		{
#ifndef EMULATERESOLUTION
			vMonitorSize = ImVec2(preview_size_x - 10.0 - vMonitorPos.x - (vMonitorBorder.x*2.0), fMaxMonitorY - vMonitorPos.y - (vMonitorBorder.y*2.0));
			vMonitorSize.y = vMonitorSize.x * fRatioInv;
			if (vMonitorSize.y > fMaxMonitorY - vMonitorPos.x - (vMonitorBorder.y*2.0))
			{
				vMonitorSize.y = fMaxMonitorY - vMonitorPos.y - (vMonitorBorder.y*2.0);
				vMonitorSize.x = (vMonitorSize.y * fRatio);
			}
#endif

#ifdef EMULATERESOLUTION
			vMonitorSize = ImVec2(preview_size_x - 10.0 - vMonitorPos.x - (vMonitorBorder.x * 2.0), fMaxMonitorY - vMonitorPos.y - (vMonitorBorder.y * 2.0));
			if (!standalone && !bPreviewScreen)
			{
				vMonitorSize.x -= panelWidth;
			}
			vMonitorSize.y = vMonitorSize.x * fRatioInv;
			if (vMonitorSize.y > fMaxMonitorY - vMonitorPos.y - (vMonitorBorder.y * 2.0))
			{
				vMonitorSize.y = fMaxMonitorY - vMonitorPos.y - (vMonitorBorder.y * 2.0);
				vMonitorSize.x = (vMonitorSize.y * fRatio);
			}
			vMonitorCenterX = preview_size_x - 10.0 - (vMonitorSize.x + vMonitorPos.x + (vMonitorBorder.x * 2.0));
			if (!standalone && !bPreviewScreen)
			{
				vMonitorCenterX += panelWidth;
			}
			else if (!standalone && bPreviewScreen)
			{
				vMonitorCenterX += 120;
			}
#else
			vMonitorCenterX = preview_size_x - 10.0 - (vMonitorSize.x + vMonitorPos.x + (vMonitorBorder.x*2.0));
#endif

		}

		vMonitorCenterX *= 0.5;
		if (vMonitorCenterX < 0.0) vMonitorCenterX = 0.0f;

		ImVec4 tool_selected_col = ImVec4(0.5, 0.5, 0.5, 0.5); //ImGui::GetStyle().Colors[ImGuiCol_PlotHistogram];
		ImVec4 monitor_col = ImVec4(0, 0, 0, 1.0); //Black for now.
		ImVec4 monitor_border = ImVec4(0.30, 0.30, 0.30, 0.75); //gray
		ImVec2 padding = { 1.0, 1.0 };
		
		ImVec2 DCCursorPos = window->DC.CursorPos;
		ImVec2 spaceAvail;// = ImGui::GetContentRegionAvail();
		ImVec2 vScreenEditorPanelSize;// = ImVec2(spaceAvail.x - vMonitorSize.x, spaceAvail.y);
		ImRect leftPanelAABB;
		ImRect currentWidgetAABB;
		leftPanelAABB.Min = ImGui::GetCursorPos();
#ifdef EMULATERESOLUTION
		if (!standalone && bPreviewScreen)
		{
			vMonitorPos += vMonitorBorder + ImVec2(10, 0);
		}
#endif
		if (!standalone && !bPreviewScreen)
		{
			if (ImGui::IsMouseReleased(0) && bPlacingNewWidget)
			{
				bPlacingNewWidget = false;
			}

			vMonitorPos += vMonitorBorder + ImVec2(10,0);
			// Make room for panel to add/remove gadgets
#ifndef EMULATERESOLUTION
			vMonitorSize.x -= panelWidth;
			vMonitorCenterX += panelWidth;
#endif

			ImVec2 spaceAvail = ImGui::GetContentRegionAvail();
			ImVec2 vScreenEditorPanelSize = ImVec2(panelWidth, spaceAvail.y);
			leftPanelAABB.Max = leftPanelAABB.Min + vScreenEditorPanelSize;

			// can make space easily as more icons are added by shrinking the size of each icon
			int iIconsOnRow = 5;
			int iSizeOfEachIcon = ((180+48)-((iIconsOnRow-1)*16)) / iIconsOnRow;

			// also, old storyboards had old allowances, so if IMAGE allowed, can also have VIDEO
			const StoryboardNodesStruct& node = Storyboard.Nodes[nodeid];
			if (node.widgets_available & ALLOW_IMAGE)
			{
				// except for HUD Screens (which are live in game and video would hit performance)
				char pHUDScreenName[256];
				sprintf(pHUDScreenName, "HUD Screen");
				if (strnicmp (Storyboard.Nodes[nodeid].title, pHUDScreenName, strlen(pHUDScreenName)) != NULL)
				{
					Storyboard.Nodes[nodeid].widgets_available |= ALLOW_VIDEO;
				}
			}

			ImGui::BeginChild("##Screeneditorleftpanel", vScreenEditorPanelSize);
			if (ImGui::StyleCollapsingHeader("Visuals##screeneditorvisual", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::Indent(10);
				vIconSize = ImVec2(iSizeOfEachIcon, iSizeOfEachIcon);
				int widgetsOnDisplay = 0;
				if (node.widgets_available & ALLOW_IMAGE)
				{
					if (ImGui::ImgBtn(SCREENEDITOR_IMAGE, vIconSize))
					{
						AddWidgetToScreen(nodeid, STORYBOARD_WIDGET_IMAGE);
					}
					if (ImGui::IsItemHovered())
					{
						ImGui::SetTooltip("Add an image");
					}
					widgetsOnDisplay++;
				}
				if (node.widgets_available & ALLOW_VIDEO)
				{
					if ((widgetsOnDisplay % iIconsOnRow) != 0) ImGui::SameLine();
					if (ImGui::ImgBtn(SCREENEDITOR_VIDEO, vIconSize))
					{
						AddWidgetToScreen(nodeid, STORYBOARD_WIDGET_VIDEO);
					}
					if (ImGui::IsItemHovered())
					{
						ImGui::SetTooltip("Add a video");
					}
					widgetsOnDisplay++;
				}
				if (node.widgets_available & ALLOW_TEXT)
				{
					if ((widgetsOnDisplay % iIconsOnRow) != 0) ImGui::SameLine();
					if (ImGui::ImgBtn(SCREENEDITOR_TEXT, vIconSize)) AddWidgetToScreen(nodeid, STORYBOARD_WIDGET_TEXT);
					if (ImGui::IsItemHovered()) ImGui::SetTooltip("Add text line");
					widgetsOnDisplay++;
					if ((widgetsOnDisplay % iIconsOnRow) != 0) ImGui::SameLine();
					if (ImGui::ImgBtn(SCREENEDITOR_TEXTAREA, vIconSize))
					{
						AddWidgetToScreen(nodeid, STORYBOARD_WIDGET_TEXTAREA);
					}
					if (ImGui::IsItemHovered()) ImGui::SetTooltip("Add text area");
					widgetsOnDisplay++;
				}
				if (node.widgets_available & ALLOW_BUTTON)
				{
					if ((widgetsOnDisplay % iIconsOnRow) != 0)
					{
						ImGui::SameLine();
					}
					if (ImGui::ImgBtn(SCREENEDITOR_BUTTON, vIconSize))
					{
						AddWidgetToScreen(nodeid, STORYBOARD_WIDGET_BUTTON);
					}
					if (ImGui::IsItemHovered())
					{
						ImGui::SetTooltip("Add a button");
					}
					widgetsOnDisplay++;
				}
				ImGui::Indent(-10);
			}
			if (ImGui::StyleCollapsingHeader("Readouts##screeneditorreadout", ImGuiTreeNodeFlags_DefaultOpen))
			{
				// Display readouts
				ImGui::Indent(10);
				ImGui::Spacing();
				ImVec2 offset = ImVec2(0.0f,-9.0f);
				ImVec2 readoutIconSize = ImVec2(30, 30);
				StoryboardNodesStruct& node = Storyboard.Nodes[nodeid];
				for (int i = 0; i < readoutTitles.size(); i++)
				{
					if ((readoutLayers[i] & node.readouts_available) == 0)
					{
						// This readout is not available for the current screen
						continue;
					}

					ImGui::SetCursorPos(ImGui::GetCursorPos() + offset);
					int imgID = SCREENEDITOR_TEXT;
					if (readoutWidgetTypes[i] == STORYBOARD_WIDGET_TEXTAREA)
					{
						imgID = SCREENEDITOR_TEXTAREA;
					}
					else if (readoutWidgetTypes[i] == STORYBOARD_WIDGET_IMAGE)
					{
						imgID = SCREENEDITOR_IMAGE;
					}
					else if (readoutWidgetTypes[i] == STORYBOARD_WIDGET_VIDEO)
					{
						imgID = SCREENEDITOR_VIDEO;
					}
					else if (readoutWidgetTypes[i] == STORYBOARD_WIDGET_TICKBOX)
					{
						imgID = SCREENEDITOR_TICKBOX;
					}
					else if (readoutWidgetTypes[i] == STORYBOARD_WIDGET_BUTTON)
					{
						imgID = SCREENEDITOR_BUTTON;
					}
					else if (readoutWidgetTypes[i] == STORYBOARD_WIDGET_RADIOTYPE)
					{
						imgID = SCREENEDITOR_RADIOBUTTON;
					}		
					else if (readoutWidgetTypes[i] == STORYBOARD_WIDGET_SLIDER)
					{
						imgID = SCREENEDITOR_SLIDER;
					}
					else if (readoutWidgetTypes[i] == STORYBOARD_WIDGET_BAR)
					{
						imgID = SCREENEDITOR_SLIDER;
					}
					ImGui::PushID(78959 + i);
					if (ImGui::ImgBtn(imgID, readoutIconSize))
					{
						AddWidgetToScreen(nodeid, readoutWidgetTypes[i], readoutTitles[i]);
					}
					ImGui::PopID();
					ImGui::SameLine();
					ImGui::SetCursorPos(ImGui::GetCursorPos() - offset);
					ImGui::Text(readoutTitles[i].c_str());
				}	
				ImGui::Indent(-10);
			}
			ImGui::EndChild();
		}

		const ImRect image_bb((DCCursorPos + ImVec2(vMonitorCenterX,0) + vMonitorPos - padding), DCCursorPos + ImVec2(vMonitorCenterX, 0) + vMonitorPos + padding + vMonitorSize);
		ImRect rMonitorArea;
		rMonitorArea.Min = image_bb.Min + padding;
		rMonitorArea.Max = image_bb.Max - padding;
		g_rStealMonitorArea = rMonitorArea;
		if (standalone)
		{
			if (ImageExist(Storyboard.Nodes[nodeid].screen_backdrop_id) && !Storyboard.Nodes[nodeid].screen_backdrop_transparent ) //PE: Support transparent if no backdrop.
			{
				//Standalone Always fill hole screen.
				window->DrawList->AddRectFilled(DCCursorPos + ImVec2(-1, -1), DCCursorPos + ImVec2(preview_size_x, preview_size_y) + ImVec2(1, 1), ImGui::GetColorU32(monitor_col));
				window->DrawList->AddRectFilled(image_bb.Min + padding, image_bb.Max - padding, ImGui::GetColorU32(monitor_col));
			}
			else
			{
				extern bool bMainLoopRunning;
				extern int g_iInGameMenuState;
				if ( (!bMainLoopRunning || bTestStandalone) && g_iInGameMenuState != 1 && Storyboard.Nodes[nodeid].screen_backdrop_transparent)
				{
					//PE: Hide game screen if not inside game yet ( custom button -> settings screens).
					ImVec4 monitor_col = ImVec4(0.0, 0, 0, 1.0); //Black for now.
					window->DrawList->AddRectFilled(ImVec2(-1, -1), ImGui::GetMainViewport()->Size + ImVec2(1, 1), ImGui::GetColorU32(monitor_col));
				}
			}
		}
		else
		{
			window->DrawList->AddRectFilled(image_bb.Min - vMonitorBorder, image_bb.Max + vMonitorBorder, ImGui::GetColorU32(monitor_border), 12.0, 15);
			window->DrawList->AddRectFilled(image_bb.Min + padding, image_bb.Max - padding, ImGui::GetColorU32(monitor_col));
			window->DrawList->AddRect(image_bb.Min, image_bb.Max, ImGui::GetColorU32(tool_selected_col), 0.0f, 15, 2.0f);
			if ( Storyboard.Nodes[nodeid].screen_backdrop_transparent && !bPreviewScreen) //PE: Support transparent if no backdrop.
			{
				ID3D11ShaderResourceView* lpTexture = GetImagePointerView(STORYBOARD_TRANSPARET);
				if (lpTexture)
				{
					//Display transparent backdrop. stretch.
					window->DrawList->AddImage((ImTextureID)lpTexture, image_bb.Min + padding, image_bb.Max - padding, ImVec2(0, 0), ImVec2(1, 1), ImGui::GetColorU32(ImVec4(1.0, 1.0, 1.0, 0.25)));
				}
			}
		}
		if (ImageExist(Storyboard.Nodes[nodeid].screen_backdrop_id))
		{
			//Draw backdrop.
			ID3D11ShaderResourceView* lpTexture = GetImagePointerView(Storyboard.Nodes[nodeid].screen_backdrop_id);
			if (lpTexture)
			{
				//Support center,stretch,zoom
				if (Storyboard.Nodes[nodeid].screen_backdrop_placement == 0)
				{
					//Center;

					float img_w = ImageWidth(Storyboard.Nodes[nodeid].screen_backdrop_id);
					float img_h = ImageHeight(Storyboard.Nodes[nodeid].screen_backdrop_id);

					ImVec2 vSize = ImVec2(preview_size_x, preview_size_y);
					if (!standalone) vSize = vMonitorSize;

					if (img_w > vSize.x || img_h > vSize.y) 
					{
						float fRatio = 1.0f / (img_w / img_h);
						img_w = vSize.x;
						img_h = vSize.x * fRatio;
						if (img_h > vSize.y) 
						{
							float fRatio = 1.0f / (img_h / img_w);
							img_h = vSize.y;
							img_w = vSize.y * fRatio;
						}
					}

					ImVec2 img_pos = rMonitorArea.Min;
					if (standalone) img_pos = DCCursorPos;

					img_pos.x += (vSize.x - img_w) * 0.5;
					img_pos.y += (vSize.y - img_h) * 0.5;
					window->DrawList->AddImage((ImTextureID)lpTexture, img_pos, img_pos + ImVec2(img_w, img_h), ImVec2(0, 0), ImVec2(1, 1), ImGui::GetColorU32(ImVec4(1.0, 1.0, 1.0, 1.0)));
				}
				else if (Storyboard.Nodes[nodeid].screen_backdrop_placement == 2)
				{
					//Zoom
					float img_w = ImageWidth(Storyboard.Nodes[nodeid].screen_backdrop_id);
					float img_h = ImageHeight(Storyboard.Nodes[nodeid].screen_backdrop_id);

					ImVec2 vSize = ImVec2(preview_size_x, preview_size_y);
					if (!standalone) vSize = vMonitorSize;
					float fRatio = 1.0f / (img_h / img_w);
					img_h = vSize.y;
					img_w = vSize.y * fRatio;
					if (img_w < vSize.x)
					{
						float adjustx = vSize.x - img_w;
						img_w += adjustx;
						img_h += adjustx * (1.0f / (img_w / img_h)); //PE: Ups the other way around.
					}

					ImVec2 img_pos = rMonitorArea.Min;
					if (standalone) img_pos = DCCursorPos;

					img_pos.x += (vSize.x - img_w) * 0.5;
					img_pos.y += (vSize.y - img_h) * 0.5;

					if (!standalone) window->DrawList->PushClipRect(rMonitorArea.Min, rMonitorArea.Max, true);
					window->DrawList->AddImage((ImTextureID)lpTexture, img_pos, img_pos + ImVec2(img_w, img_h), ImVec2(0, 0), ImVec2(1, 1), ImGui::GetColorU32(ImVec4(1.0, 1.0, 1.0, 1.0)));
					if (!standalone) window->DrawList->PopClipRect();

				}
				else //if (Storyboard.Nodes[nodeid].screen_backdrop_placement == 1)
				{
					//Stretch
					if (standalone)
					{
						const ImRect image_bb((DCCursorPos + vMonitorPos - padding), DCCursorPos + vMonitorPos + padding + vMonitorSize);
						window->DrawList->AddImage((ImTextureID)lpTexture, DCCursorPos, DCCursorPos + ImVec2(preview_size_x, preview_size_y), ImVec2(0, 0), ImVec2(1, 1), ImGui::GetColorU32(ImVec4(1.0, 1.0, 1.0, 1.0)));
					}
					else
					{
						window->DrawList->AddImage((ImTextureID)lpTexture, image_bb.Min + padding, image_bb.Max - padding, ImVec2(0, 0), ImVec2(1, 1), ImGui::GetColorU32(ImVec4(1.0, 1.0, 1.0, 1.0)));
					}
				}
			}
		}
		if (!standalone)
		{
			//Moved grid to after backdrop.
			if (bDisplayGrid && !bPreviewScreen && iQuitWindowLoop <= 0)
			{
				if (Storyboard.Nodes[nodeid].screen_grid_size > 0)
				{
					//ImVec2 vScale = vMonitorSize / ImVec2(1980.0, 1080);
					ImVec2 vScale = vMonitorSize / vViewportSize;
					ImVec4 tool_selected_col = ImVec4(0.75, 0.75, 0.75, 0.25); //ImGui::GetStyle().Colors[ImGuiCol_PlotHistogram];
					float grid_size = Storyboard.Nodes[nodeid].screen_grid_size;
					ImVec2 fOnePercent = ImVec2(vMonitorSize.x / 100.0, vMonitorSize.y / 100.0); //PE: This can be changed in the future to support different screen ratio settings.
					float grid_step = (grid_size * fOnePercent.x); // *vScale.x;
					for (float fx = (image_bb.Min.x + padding.x); fx < (image_bb.Max.x - padding.x); fx += grid_step)
					{
						ImVec2 linefrom = ImVec2(fx, image_bb.Min.y + padding.y);
						ImVec2 lineto = ImVec2(fx, image_bb.Max.y - padding.y);
						window->DrawList->AddLine(linefrom, lineto, ImGui::GetColorU32(tool_selected_col));
					}
					grid_step = (grid_size * fOnePercent.y); // *vScale.y;
					for (float fy = (image_bb.Min.y + padding.y); fy < (image_bb.Max.y - padding.y); fy += grid_step)
					{
						ImVec2 linefrom = ImVec2(image_bb.Min.x + padding.x, fy);
						ImVec2 lineto = ImVec2(image_bb.Max.x - padding.x, fy);
						window->DrawList->AddLine(linefrom, lineto, ImGui::GetColorU32(tool_selected_col));
					}
				}
			}
		}

		extern float screen_editor_scalemod (float);
#ifdef EMULATERESOLUTION

		ImVec2 fGlobalScale = ImVec2(screen_editor_scalemod(vViewportSize.x / 1920.0f), screen_editor_scalemod(vViewportSize.x / 1920.0f));
		ImVec2 vScale = vMonitorSize / vViewportSize;
		float fFontScale = screen_editor_scalemod(1080.0f / monitor_size_y);

		ImVec2 vUniversalScale = ImVec2(vMonitorSize.x / monitor_size_x, vMonitorSize.x / monitor_size_x);
		ImVec2 fUniversalGlobalScale = ImVec2(screen_editor_scalemod(monitor_size_y / 1080.0f), screen_editor_scalemod(monitor_size_y / 1080.0f)); //Fit by y resolution.

#else
		ImVec2 fGlobalScale = ImVec2(screen_editor_scalemod(vViewportSize.x / 1920.0f), screen_editor_scalemod(vViewportSize.x / 1920.0f));
		ImVec2 vScale = vMonitorSize / vViewportSize;
		float fFontScale = fGlobalScale.x;
#endif
		ImVec2 vMonitorStart = ImVec2(vMonitorCenterX, 0) + vHeaderEnd + vMonitorPos;
		ImVec2 vMonitorEnd = ImVec2(vMonitorCenterX, 0) + vHeaderEnd + vMonitorPos + vMonitorSize;

		ImGui::SetWindowFontScale(vScale.y);

		ImGui::SetCursorPos(vMonitorStart);

		ImGui::SetWindowFontScale(vScale.y);
		ImGui::SetCursorPos(vMonitorEnd);

		if (iCurrentSelectedWidget < 0) iCurrentSelectedWidget = 0;
		int iSkipWidgetSelectionForFrames = 0;

		if (standalone == false)
		{
			// Determine which widget is selected
			ImVec2 cursorStore = ImGui::GetCursorPos();
			int iWidgetSelectedThisFrame = -1;

			for (int early = 1; early >= -1; early--)
			{
				for (int i = STORYBOARD_MAXWIDGETS; i >= 0; i--)
				{
					bool bSelectThis = false;
					if (Storyboard.widget_drawordergroup[nodeid][i] == early) bSelectThis = true;
					if (bSelectThis == false)
						continue;

					ImVec2 fOnePercent = ImVec2(vMonitorSize.x / 100.0, vMonitorSize.y / 100.0);
					ImVec2 widget_pos = Storyboard.Nodes[nodeid].widget_pos[i] * fOnePercent; //Real screen pos.
					ImVec2 widget_size = ImVec2(500, 74); //Default widget size.

#ifdef EMULATERESOLUTION
					if (Storyboard.Nodes[nodeid].universal_resolution[i])
					{
						widget_size = widget_size * vUniversalScale;
						widget_size = widget_size * fUniversalGlobalScale;
					}
					else
#endif
					{
						widget_size = widget_size * vScale;
						widget_size = widget_size * fGlobalScale;
					}
					float font_scale = WidgetSelectUsedFont(nodeid, i);
					ImGui::SetWindowFontScale(font_scale * vScale.x * fFontScale * fabs(Storyboard.Nodes[nodeid].widget_font_size[i]));
					if (ImageExist(Storyboard.Nodes[nodeid].widget_normal_thumb_id[i]))
					{
						widget_size.x = ImageWidth(Storyboard.Nodes[nodeid].widget_normal_thumb_id[i]);
						widget_size.y = ImageHeight(Storyboard.Nodes[nodeid].widget_normal_thumb_id[i]);
#ifdef EMULATERESOLUTION
						if (Storyboard.Nodes[nodeid].universal_resolution[i])
						{
							widget_size = widget_size * vUniversalScale;
							widget_size = widget_size * fUniversalGlobalScale;
							widget_size = widget_size * Storyboard.Nodes[nodeid].widget_size[i];
						}
						else
#endif
						{
							widget_size = widget_size * vScale; //Scale to visible screen size.
							widget_size = widget_size * fGlobalScale;
							widget_size = widget_size * Storyboard.Nodes[nodeid].widget_size[i];
						}
					}
					else
					{
						cstr text = Storyboard.Nodes[nodeid].widget_label[i];
						if (text.Len() <= 0) text = "Empty Text";
						widget_size = ImGui::CalcTextSize(text.Get());
					}
					widget_pos = (widget_pos - ImVec2((widget_size.x * 0.5), 0.0)); //Scale to visible screen size.
					ImGui::SetCursorPos(vMonitorStart + widget_pos);
					ImGui::Dummy(widget_size);
					if (ImGui::IsItemHovered())
					{
						if (!ImGui::IsMouseDragging(0) && ImGui::IsMouseDown(0))
						{
							if (iSkipWidgetSelectionForFrames == 0 && iWidgetSelectedThisFrame < 0)
							{
								iCurrentSelectedWidget = i;
								iWidgetSelectedThisFrame = i;
								ImGui::PopFont();
								break;
							}
						}
					}
					ImGui::PopFont();
				}
			}
			ImGui::SetCursorPos(cursorStore);
		}
		
		// Draw all widgets (early and regular)
		int iMinDraw = -1;
		int iMaxDraw =  1;
		for(int early = iMinDraw; early <= iMaxDraw; early++ )
		{
			for (int i = 0; i < Storyboard_ActiveWidgets.size(); i++)
			{
				bool bDrawThis = false;
				if (Storyboard.widget_drawordergroup[nodeid][i] < iMinDraw || Storyboard.widget_drawordergroup[nodeid][i] > iMaxDraw)
				{
					// ensure ALWAYS within the draw order slots!
					Storyboard.widget_drawordergroup[nodeid][i] = 0;
				}
				if (Storyboard.widget_drawordergroup[nodeid][i] == early) bDrawThis = true;
				if (bDrawThis == false)
					continue;
				
				int index = Storyboard_ActiveWidgets[i];
				bool bUsed = Storyboard.Nodes[nodeid].widget_used[index];
				if (bUsed == false)
					continue;

				bool bReadOnly = Storyboard.Nodes[nodeid].widget_read_only[index];
				bool bSpecialLuaReturnValue = false;
				if (bReadOnly)
				{
					if (index >= 1 && index <= 8 && (stricmp(Storyboard.Nodes[nodeid].lua_name, "loadgame.lua") == 0 || stricmp(Storyboard.Nodes[nodeid].lua_name, "savegame.lua") == 0 ) )
					{
						strcpy(Storyboard.Nodes[nodeid].widget_label[index], LoadGameTitle[index]);
						bReadOnly = false;
					}
				}

				// can hide any widget if flagged as so
				bool bIsWidgetHidden = bImGuiInTestGame && Storyboard.widget_ingamehidden[nodeid][index];
				if (bIsWidgetHidden == true)
					continue;

				//ImVec2 fOnePercent = ImVec2(1920.0 / 100.0, 1080.0 / 100.0); //PE: This can be changed in the future to support different screen ratio settings.
				ImVec2 fOnePercent = ImVec2(vMonitorSize.x / 100.0, vMonitorSize.y / 100.0);
				bool bUsePivotXCenter = true;
				int iTextAdjustment = 1; // 0=left, 1=center , 2=right

				ImVec2 widget_pos = Storyboard.Nodes[nodeid].widget_pos[index] * fOnePercent; //Real screen pos.
				ImVec2 widget_size = ImVec2(500, 74); //Default widget size.
#ifdef EMULATERESOLUTION
				if (Storyboard.Nodes[nodeid].universal_resolution[index])
				{
					widget_size = widget_size * vUniversalScale;
					widget_size = widget_size * fUniversalGlobalScale;
				}
				else
#endif
				{
					widget_size = widget_size * vScale;
					widget_size = widget_size * fGlobalScale;
				}
				//One widget can only use one font, so select it now and use for all functions.
				float font_scale = WidgetSelectUsedFont(nodeid, index);
				ImGui::SetWindowFontScale(font_scale*vScale.x* fFontScale * fabs(Storyboard.Nodes[nodeid].widget_font_size[index]));

				//Is a kind of progress bar?
				bool bProgressbar = false;
				if (   Storyboard.Nodes[nodeid].widget_type[index] == STORYBOARD_WIDGET_PROGRESS
					|| Storyboard.Nodes[nodeid].widget_type[index] == STORYBOARD_WIDGET_SLIDER
					|| Storyboard.Nodes[nodeid].widget_type[index] == STORYBOARD_WIDGET_BAR)
					bProgressbar = true;

				//Widget Button
				if (Storyboard.Nodes[nodeid].widget_type[index] == STORYBOARD_WIDGET_BUTTON 
				|| bProgressbar 
				|| Storyboard.Nodes[nodeid].widget_type[index] == STORYBOARD_WIDGET_RADIOTYPE 
				|| Storyboard.Nodes[nodeid].widget_type[index] == STORYBOARD_WIDGET_TICKBOX)
				{
					if (ImageExist(Storyboard.Nodes[nodeid].widget_normal_thumb_id[index]))
					{
						widget_size.x = ImageWidth(Storyboard.Nodes[nodeid].widget_normal_thumb_id[index]);
						widget_size.y = ImageHeight(Storyboard.Nodes[nodeid].widget_normal_thumb_id[index]);
#ifdef EMULATERESOLUTION
						if (Storyboard.Nodes[nodeid].universal_resolution[index])
						{
							widget_size = widget_size * vUniversalScale;
							widget_size = widget_size * fUniversalGlobalScale;
							widget_size = widget_size * Storyboard.Nodes[nodeid].widget_size[index];
						}
						else
#endif
						{
							widget_size = widget_size * vScale; //Scale to visible screen size.
							widget_size = widget_size * fGlobalScale;
							widget_size = widget_size * Storyboard.Nodes[nodeid].widget_size[index];
						}
					}

					if (bUsePivotXCenter)
						widget_pos = (widget_pos - ImVec2((widget_size.x*0.5), 0.0)); //Scale to visible screen size.

					ImGui::SetCursorPos(vMonitorStart + widget_pos);
					ImGui::Dummy(widget_size);
					bool bHovered = false;
					if (ImGui::IsItemHovered())
					{
						bHovered = true;
					}
					//Button Image
					ID3D11ShaderResourceView* lpTexture = GetImagePointerView(Storyboard.Nodes[nodeid].widget_normal_thumb_id[index]);
					if(!bProgressbar && bHovered) lpTexture = GetImagePointerView(Storyboard.Nodes[nodeid].widget_highlight_thumb_id[index]);
					if (Storyboard.Nodes[nodeid].widget_type[index] == STORYBOARD_WIDGET_RADIOTYPE)
					{
						// used by GRAPHICS SETTINGS (1,2,3)
						int iMatchToSettingValue = -1;
						if (stricmp(Storyboard.Nodes[nodeid].widget_label[index], "LOW") == NULL) iMatchToSettingValue = 1;
						if (stricmp(Storyboard.Nodes[nodeid].widget_label[index], "MEDIUM") == NULL) iMatchToSettingValue = 2;
						if (stricmp(Storyboard.Nodes[nodeid].widget_label[index], "HIGHEST") == NULL) iMatchToSettingValue = 3;
						if (Storyboard.NodeRadioButtonSelected[nodeid] < 0.0) Storyboard.NodeRadioButtonSelected[nodeid] = 3;// iMatchToSettingValue; HIGHEST always default
						if (Storyboard.NodeRadioButtonSelected[nodeid] == iMatchToSettingValue)
						{
							lpTexture = GetImagePointerView(Storyboard.Nodes[nodeid].widget_selected_thumb_id[index]);
							if(!lpTexture) lpTexture = GetImagePointerView(Storyboard.Nodes[nodeid].widget_highlight_thumb_id[index]);
						}
					}
					else if (Storyboard.Nodes[nodeid].widget_type[index] == STORYBOARD_WIDGET_TICKBOX)
					{
						bool bEnabled = false;
						if (strlen(Storyboard.widget_readout[nodeid][index]) > 0)
						{
							// Tickboxes - on/off
							bEnabled = GetReadoutValueInt(Storyboard.widget_readout[nodeid][index]);
						}
						else
						{
							// Using NodeSliderValues to prevent having to add to StoryboardStruct
							if (Storyboard.NodeSliderValues[nodeid][index] > 0.0f)
							{
								bEnabled = true;
							}
						}

						if (bEnabled)
						{
							lpTexture = GetImagePointerView(Storyboard.Nodes[nodeid].widget_selected_thumb_id[index]);
							if (!lpTexture) lpTexture = GetImagePointerView(Storyboard.Nodes[nodeid].widget_highlight_thumb_id[index]);
						}
					}
					if (lpTexture)
					{
						ImGui::SetCursorPos(vMonitorStart + widget_pos);
						ImVec2 img_pos = ImGui::GetWindowPos() + vMonitorStart + widget_pos;
						img_pos.y -= ImGui::GetScrollY();
						window->DrawList->AddImage((ImTextureID)lpTexture, img_pos, img_pos + widget_size, ImVec2(0, 0), ImVec2(1, 1), ImGui::GetColorU32(ImVec4(1.0, 1.0, 1.0, 1.0)));
					}
					if (bProgressbar)
					{
						lpTexture = GetImagePointerView(Storyboard.Nodes[nodeid].widget_highlight_thumb_id[index]);
						if (lpTexture)
						{
							static float fProgress = 0.0;
							if (Storyboard.Nodes[nodeid].widget_type[index] == STORYBOARD_WIDGET_BAR)
							{
								if (stricmp(Storyboard.widget_readout[nodeid][index], "User Defined Global Statusbar") == NULL)
								{
									// split label (first;second) so can read globals separately
									char storeFirstEntry[MAX_PATH];
									strcpy(storeFirstEntry, "");
									strcpy(storeFirstEntry, Storyboard.Nodes[nodeid].widget_label[index]);
									char storeSecondEntry[MAX_PATH];
									strcpy(storeSecondEntry, "");
									char* pDelimit = strstr(storeFirstEntry, ";");
									if (pDelimit)
									{
										strcpy(storeSecondEntry, pDelimit + 1);
										*pDelimit = 0;
									}

									// placeholder for all user defined global values
									if (bImGuiInTestGame == false)
									{
										// placeholder shown in screen editor
										fProgress = Storyboard.Nodes[nodeid].widget_initial_value[index];
									}
									else
									{
										// read from active LUA, i.e. g_UserGlobal[yourscript.user_variable_name]
										char pUserDefinedGlobal[256];
										int readoutValueFromLUA1 = 0;
										int readoutValueFromLUA2 = 0;
										if (stricmp(storeFirstEntry, "Health Remaining") == NULL)
										{
											readoutValueFromLUA1 = t.player[t.plrid].health;
										}
										else
										{
											if (stricmp(storeFirstEntry, "Lives Remaining") == NULL)
											{
												readoutValueFromLUA1 = t.player[t.plrid].lives;
											}
											else
											{
												if (stricmp(storeFirstEntry, "Ammo Remaining") == NULL)
												{
													readoutValueFromLUA1 = t.slidersmenuvalue[1][1].value;
												}
												else
												{
													if (stricmp(storeFirstEntry, "Maximum Ammo") == NULL)
													{
														readoutValueFromLUA1 = t.slidersmenuvalue[1][2].value;
													}
													else
													{
														sprintf(pUserDefinedGlobal, "g_UserGlobal['%s']", storeFirstEntry);
														readoutValueFromLUA1 = LuaGetInt(pUserDefinedGlobal);
													}
												}
											}
										}
										if (stricmp(storeSecondEntry, "Maximum Health") == NULL)
										{
											readoutValueFromLUA2 = t.playercontrol.startstrength;
										}
										else
										{
											if (stricmp(storeSecondEntry, "Weapon Reload Quantity") == NULL)
											{
												readoutValueFromLUA2 = g.firemodes[t.gunid][g.firemode].settings.reloadqty;
											}
											else
											{
												if (stricmp(storeSecondEntry, "Maximum Clipped Ammo") == NULL)
												{ 
													int iClipCapacity = g.firemodes[t.gunid][g.firemode].settings.clipcapacity;
													if (iClipCapacity == 0) iClipCapacity = 50; // if no clip size specified, default to 50
													readoutValueFromLUA2 = g.firemodes[t.gunid][g.firemode].settings.reloadqty * iClipCapacity;
												}
												else
												{
													sprintf(pUserDefinedGlobal, "g_UserGlobal['%s']", storeSecondEntry);
													readoutValueFromLUA2 = LuaGetInt(pUserDefinedGlobal);
												}
											}
										}
										fProgress = ((float)readoutValueFromLUA1/(float)readoutValueFromLUA2)*100.0f;
									}
								}
								else
								{
									// no value if not from user global
									fProgress = 0.0f;
								}
							}
							else if (Storyboard.Nodes[nodeid].widget_type[index] == STORYBOARD_WIDGET_SLIDER)
							{
								//Must be changeable.
								if (strlen(Storyboard.widget_readout[nodeid][index]) > 0)
								{
									fProgress = GetReadoutValueInt(Storyboard.widget_readout[nodeid][index]);
								}
								else
								{
									fProgress = Storyboard.NodeSliderValues[nodeid][index];
								}
							}
							else
							{
								if (standalone)
								{
									if (bStartLoadingGame && iFakeLoadGameTest > 0)
										fProgress = 100 - iFakeLoadGameTest;
									else
										fProgress = t.game.levelloadprogress;
								}
								else
								{
									fProgress += 0.25;
								}
								if (fProgress >= 100.0) fProgress = 0.0;
							}
							ImVec2 vSize = widget_size;
							vSize.x = (widget_size.x / 100.0) * fProgress;
							ImGui::SetCursorPos(vMonitorStart + widget_pos);
							ImVec2 img_pos = ImGui::GetWindowPos() + vMonitorStart + widget_pos;
							img_pos.y -= ImGui::GetScrollY();
							window->DrawList->AddImage((ImTextureID)lpTexture, img_pos, img_pos + vSize, ImVec2(0, 0), ImVec2(1, 1), ImGui::GetColorU32(ImVec4(1.0, 1.0, 1.0, 1.0)));
						}
					}

					//Text Label
					if (Storyboard.Nodes[nodeid].widget_type[index] != STORYBOARD_WIDGET_BAR)
					{
						// is this a special button
						char pDestStr[MAX_PATH];
						strcpy(pDestStr, "");
						LPSTR pWidgetLabel = Storyboard.Nodes[nodeid].widget_label[index];
						if (strnicmp(pWidgetLabel, "quest:", 6) == NULL && bImGuiInTestGame)
						{
							char pUserDefinedGlobal[256];
							sprintf(pUserDefinedGlobal, "g_UserGlobal['%s']", pWidgetLabel);
							LuaGetString(pUserDefinedGlobal, pDestStr);
							pWidgetLabel = pDestStr;
						}

						// render text for button and other things
						ImVec2 fTextAdjust = ImVec2(0.0f, 0.0f);
						ImVec2 fTextSize = ImGui::CalcTextSize(pWidgetLabel); //Already scaled.
						if (iTextAdjustment == 0)
							fTextAdjust.y = (widget_size.y * 0.5) - (fTextSize.y * 0.5); //y always center
						else if (iTextAdjustment == 1)
							fTextAdjust = (widget_size * 0.5) - (fTextSize * 0.5);
						else if (iTextAdjustment == 2)
						{
							fTextAdjust.x = widget_size.x - fTextSize.x - 4.0; //4.0 = padding.
							fTextAdjust.y = widget_size.y * 0.5 - fTextSize.y * 0.5; //y always center
						}
						fTextAdjust += Storyboard.widget_textoffset[nodeid][index];
						if (Storyboard.Nodes[nodeid].widget_font_size[index] > 0)
						{
							ImGui::SetCursorPos(vMonitorStart + widget_pos + fTextAdjust);
							ImGui::TextColored(Storyboard.Nodes[nodeid].widget_font_color[index], pWidgetLabel);
						}
					}
				}
			
				if (Storyboard.Nodes[nodeid].widget_type[index] == STORYBOARD_WIDGET_TEXT
				|| Storyboard.Nodes[nodeid].widget_type[index] == STORYBOARD_WIDGET_TEXTAREA)
				{
					cstr text = Storyboard.Nodes[nodeid].widget_label[index];
					if (stricmp(Storyboard.widget_readout[nodeid][index], "User Defined Global") == NULL)
					{
						// placeholder for all user defined global values
						if (bImGuiInTestGame==false)
						{
							// placeholder shown in screen editor
							char valueStr[64];
							sprintf(valueStr, "%d", Storyboard.Nodes[nodeid].widget_initial_value[index]);
							text = valueStr;
						}
						else
						{
							// read from active LUA, i.e. g_UserGlobal[yourscript.user_variable_name]
							char pUserDefinedGlobal[256];
							sprintf(pUserDefinedGlobal, "g_UserGlobal['%s']", Storyboard.Nodes[nodeid].widget_label[index]);
							int readoutValueFromLUA = LuaGetInt(pUserDefinedGlobal);
							char valueStr[64];
							sprintf(valueStr, "%d", readoutValueFromLUA);
							text = valueStr;
						}
					}
					else if (stricmp(Storyboard.widget_readout[nodeid][index], "User Defined Global Text") == NULL)
					{
						// placeholder for all user defined global values
						if (bImGuiInTestGame == false)
						{
							// placeholder shown in screen editor
							char valueStr[64];
							sprintf(valueStr, "%s", "[text]");// Storyboard.Nodes[nodeid].widget_initial_value[index]);
							text = valueStr;
						}
						else
						{
							// read from active LUA
							char pUserDefinedGlobal[256];
							sprintf(pUserDefinedGlobal, "g_UserGlobal['%s']", Storyboard.Nodes[nodeid].widget_label[index]);
							char pDestStr[MAX_PATH];
							strcpy(pDestStr, "");
							LuaGetString(pUserDefinedGlobal, pDestStr);
							text = pDestStr;
						}
					}
					else
					{
						if ((bImGuiInTestGame || standalone) && strlen(Storyboard.widget_readout[nodeid][index]) > 0)
						{
							// special code to hide certain HUDs
							bool bHideThis = false;
							if (t.player[1].health == 99999 || t.huddamage.immunity > 0)
							{
								if (stricmp (Storyboard.widget_readout[nodeid][index], "Health Remaining") == NULL) bHideThis = true;
								if (stricmp (Storyboard.widget_readout[nodeid][index], "Maximum Health") == NULL) bHideThis = true;
								if (stricmp (Storyboard.widget_readout[nodeid][index], "Lives Remaining") == NULL) bHideThis = true;
							}
						
							// Display the variable value that this readout represents
							if (bHideThis == false)
							{
								int readoutValue = GetReadoutValueInt(Storyboard.widget_readout[nodeid][index]);
								if (readoutValue != -INT_MAX) // -INT_MAX indicates failure to get the readout value
								{
									char valueStr[64];
									sprintf(valueStr, "%d", readoutValue);
									text = valueStr;
								}
								else
								{
									text = "";
								}
							}
							else
							{
								text = "";
							}
						}
					}

					//PE: Cant have empty text, we cant see it, where to select it.
					if (!bPreviewScreen && text.Len() <= 0) text = "Empty Text";

					widget_size = ImGui::CalcTextSize(text.Get());
					if (bUsePivotXCenter)
						widget_pos = (widget_pos - ImVec2((widget_size.x * 0.5), 0.0));

					ImGui::SetCursorPos(vMonitorStart + widget_pos);
					ImGui::Dummy(widget_size);
					bool bHovered = false;
					if (ImGui::IsItemHovered())
					{
						bHovered = true;
					}

					ImGui::SetCursorPos(vMonitorStart + widget_pos);
					if (Storyboard.Nodes[nodeid].widget_type[index] == STORYBOARD_WIDGET_TEXTAREA)
						ImGui::PushTextWrapPos(0.0f);

					// show text if in edit mode, do not show text in game and hidden
					LPSTR pTextToShow = text.Get();
					if (bImGuiInTestGame == false)
					{
						if (Storyboard.Nodes[nodeid].widget_font_size[index] < 0) pTextToShow = "(H)";
					}
					else
					{
						if (Storyboard.Nodes[nodeid].widget_font_size[index] < 0) pTextToShow = NULL;
					}
					if(pTextToShow) ImGui::TextColored(Storyboard.Nodes[nodeid].widget_font_color[index], pTextToShow);

					if (Storyboard.Nodes[nodeid].widget_type[index] == STORYBOARD_WIDGET_TEXTAREA)
						ImGui::PopTextWrapPos();
				}

				// monitor for screen changes to remove any lingering video
				static int g_iLastScreenNodeID = -1;
				if (g_iLastScreenNodeID != nodeid)
				{
					g_iLastScreenNodeID = nodeid;
					if (g_iStoryboardScreenVideoID > 0)
					{
						if (AnimationExist(g_iStoryboardScreenVideoID) == 1) DeleteAnimation(g_iStoryboardScreenVideoID);
						g_iStoryboardScreenVideoID = 0;
					}
				}

				if (Storyboard.Nodes[nodeid].widget_type[index] == STORYBOARD_WIDGET_IMAGE
				|| Storyboard.Nodes[nodeid].widget_type[index] == STORYBOARD_WIDGET_VIDEO)
				{
					bool bWidgetIsVideo = false;
					if (Storyboard.Nodes[nodeid].widget_type[index] == STORYBOARD_WIDGET_VIDEO ) 
						bWidgetIsVideo = true;

					int imgID = Storyboard.Nodes[nodeid].widget_normal_thumb_id[index];
					if (nodeidStore == -1 && strlen(Storyboard.widget_readout[nodeid][index]) > 0)
					{
						// Get the image ID to display the readout
						t.iTmpImgID = Storyboard.Nodes[nodeid].widget_normal_thumb_id[index];
						imgID = GetReadoutValueInt(Storyboard.widget_readout[nodeid][index]);
						if (imgID == -INT_MAX)
						{
							imgID = 0;
						}
					}
					if (bWidgetIsVideo==false && bImGuiInTestGame == true)
					{
						// special code to hide certain HUDs
						bool bHideThis = false;
						if (t.player[1].health == 99999 || t.huddamage.immunity > 0)
						{
							if (stricmp (Storyboard.widget_readout[nodeid][index], "Health Panel") == NULL) bHideThis = true;
						}
						if (bHideThis == true)
						{
							imgID = 0;
						}
					}
					else
					{
						// only in HUD editor mode
						if(!(bWidgetIsVideo == true && t.game.gameisexe == 1))
						{
							if (ImageExist(imgID) == 0)
							{
								// if no image, cannot grab widget or delete it, so use a placeholder
								image_setlegacyimageloading(true);
								LoadImage("imagebank\\HUD Library\\MAX\\missing.png", imgID);
								image_setlegacyimageloading(false);

								// also possible it was placed outside of screen if the missing image was large!
								Storyboard.Nodes[nodeid].widget_pos[index].x = 50.0f;
								Storyboard.Nodes[nodeid].widget_pos[index].y = 50.0f;
							}
						}
					}
					if (ImageExist(imgID))
					{
						widget_size.x = ImageWidth(imgID);
						widget_size.y = ImageHeight(imgID);
#ifdef EMULATERESOLUTION
						if (Storyboard.Nodes[nodeid].universal_resolution[index])
						{
							widget_size = widget_size * vUniversalScale;
							widget_size = widget_size * fUniversalGlobalScale;
							widget_size = widget_size * Storyboard.Nodes[nodeid].widget_size[index];
						}
						else
#endif
						{
							widget_size = widget_size * vScale; //Scale to visible screen size.
							widget_size = widget_size * fGlobalScale;
							widget_size = widget_size * Storyboard.Nodes[nodeid].widget_size[index];
						}
					}

					if (bUsePivotXCenter)
						widget_pos = (widget_pos - ImVec2((widget_size.x*0.5), 0.0)); //Scale to visible screen size.

					ImGui::SetCursorPos(vMonitorStart + widget_pos);
					ImGui::Dummy(widget_size);
					bool bHovered = false;
					if (ImGui::IsItemHovered())
					{
						bHovered = true;
					}
					bool bIsWidgetHidden = bImGuiInTestGame && Storyboard.widget_ingamehidden[nodeid][Storyboard_ActiveWidgets[i]];
					if (bIsWidgetHidden)
					{
						// Hide images
					}
					else
					{
						// Handle Video
						if (bWidgetIsVideo == true && t.game.gameisexe == 1)
						{
							// Display and Handle Video if in Standalone
							if (g_iStoryboardScreenVideoID == 0)
							{
								for (int itl = 1; itl <= 32; itl++)
								{
									if (AnimationExist(itl) == 0) { g_iStoryboardScreenVideoID = itl; break; }
								}
								char pFinalVideoFilePath[MAX_PATH];
								strcpy(pFinalVideoFilePath, Storyboard.Nodes[nodeid].widget_highlight_thumb[index]);
								GG_GetRealPath(pFinalVideoFilePath, 0);
								if (LoadAnimation(pFinalVideoFilePath, g_iStoryboardScreenVideoID, g.videoprecacheframes, 0, 1) == false)
								{
									g_iStoryboardScreenVideoID = -999;
								}
								if (g_iStoryboardScreenVideoID > 0)
								{
									SetVideoVolume(100.0);
									if (Storyboard.Nodes[nodeid].widget_font_size[index] != 0)
									{
										LoopAnimation(g_iStoryboardScreenVideoID);
									}
									else
									{
										PlayAnimation(g_iStoryboardScreenVideoID);
									}
									SetRenderAnimToImage(g_iStoryboardScreenVideoID, true);
									UpdateAllAnimation();
								}
							}
							else
							{
								UpdateAllAnimation();
								if (g_iStoryboardScreenVideoID > 0)
								{
									if (AnimationExist(g_iStoryboardScreenVideoID) && AnimationPlaying(g_iStoryboardScreenVideoID))
									{
										ID3D11ShaderResourceView* lpVideoTexture = GetAnimPointerView(g_iStoryboardScreenVideoID);
										float fVideoW = GetAnimWidth(g_iStoryboardScreenVideoID);
										float fVideoH = GetAnimHeight(g_iStoryboardScreenVideoID);

										//PE: imgID do not always exists in standalone.
										if (!ImageExist(imgID))
										{
											widget_size.x = fVideoW;
											widget_size.y = fVideoH;
#ifdef EMULATERESOLUTION
											if (Storyboard.Nodes[nodeid].universal_resolution[index])
											{
												widget_size = widget_size * vUniversalScale;
												widget_size = widget_size * fUniversalGlobalScale;
												widget_size = widget_size * Storyboard.Nodes[nodeid].widget_size[index];
											}
											else
#endif
											{
												widget_size = widget_size * vScale; //Scale to visible screen size.
												widget_size = widget_size * fGlobalScale;
												widget_size = widget_size * Storyboard.Nodes[nodeid].widget_size[index];
											}
										}
										if (lpVideoTexture)
										{
											ImGuiWindow* window = ImGui::GetCurrentWindow();
											ImGui::SetCursorPos(vMonitorStart + widget_pos);
											ImRect image_bb(window->DC.CursorPos, window->DC.CursorPos + widget_size);
											float animU = GetAnimU(g_iStoryboardScreenVideoID);
											float animV = GetAnimV(g_iStoryboardScreenVideoID);
											ImVec2 uv0 = ImVec2(0, 0);
											ImVec2 uv1 = ImVec2(animU, animV);
											window->DrawList->AddImage((ImTextureID)lpVideoTexture, image_bb.Min, image_bb.Max, uv0, uv1, ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 1.0f)));
										}
									}
								}
								else
								{
									// video was not loaded, so -999 in effect!
								}
							}
						}
						else
						{
							// Display Image
							// GGMAX 2.37: GetImagePointer is a DX11-ONLY accessor — it returns
							// m_imgptr->lpTexture, the D3D11 texture object, which is ALWAYS NULL
							// on DX12 (CImageC_part1.cpp:236). Only its sibling
							// GetImagePointerView carries the DX12 bridge that lazily creates the
							// texture via ImGui_DX12_GetOrLoadTexture (CImageC_part1.cpp:253-268).
							// So this guard could never pass and no screen-editor image has ever
							// drawn on DX12 — while the very next line, ImgBtn(imgID, ...), takes
							// the ID and resolves it through that same working bridge, which is
							// why the toolbar icons render fine. The blit was never broken; only
							// the null-check in front of it was. Using the View accessor both
							// tests correctly and warms the texture the blit is about to need.
							void* lpTexture = (void*)GetImagePointerView(imgID);
							if (lpTexture)
							{
								ImGui::SetCursorPos(vMonitorStart + widget_pos);
								ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
								ImVec4 imageColor = Storyboard.widget_colors[nodeid][Storyboard_ActiveWidgets[i]];
								ImGui::ImgBtn(imgID, widget_size, ImColor(255, 255, 255, 0), imageColor, imageColor, imageColor, 0);
								ImGui::PopItemFlag();
							}
						}
					}

					if (bWidgetIsVideo == false)
					{
						// no text if a global panel
						bool bShowText = true;
						cstr text = Storyboard.Nodes[nodeid].widget_label[index];
						if (stricmp(Storyboard.widget_readout[nodeid][index], "User Defined Global Image") == NULL) bShowText = false;
						if (stricmp(Storyboard.widget_readout[nodeid][index], "User Defined Global Panel") == NULL) bShowText = false;

						//Text Label
						if (bShowText == true)
						{
							LPSTR pTextToShow = Storyboard.Nodes[nodeid].widget_label[index];
							if (Storyboard.Nodes[nodeid].widget_font_size[index] < 0) pTextToShow = "(H)";
							ImVec2 fTextAdjust = ImVec2(0.0, 0.0);
							ImVec2 fTextSize = ImGui::CalcTextSize(Storyboard.Nodes[nodeid].widget_label[index]); //Already scaled.
							if (iTextAdjustment == 0)
								fTextAdjust.y = (widget_size.y * 0.5) - (fTextSize.y * 0.5); //y always center
							else if (iTextAdjustment == 1)
								fTextAdjust = (widget_size * 0.5) - (fTextSize * 0.5);
							else if (iTextAdjustment == 2)
							{
								fTextAdjust.x = widget_size.x - fTextSize.x - 4.0; //4.0 = padding.
								fTextAdjust.y = widget_size.y * 0.5 - fTextSize.y * 0.5; //y always center
							}
							fTextAdjust += Storyboard.widget_textoffset[nodeid][index];
							ImGui::SetCursorPos(vMonitorStart + widget_pos + fTextAdjust);
							ImGui::TextColored(Storyboard.Nodes[nodeid].widget_font_color[index], pTextToShow);
						}
					}
				}

				bool bLuaPageClosing = false;

				//PE: TODO - Need to add sound on button click somewhere here!.
				cstr cTriggerButtonClickSound = "";
				if (standalone)
				{
					if (Storyboard.Nodes[nodeid].widget_type[index] == STORYBOARD_WIDGET_BUTTON)
					{
						ImVec2 vLargerGrabArea = ImVec2(10.0, 10.0);
						bool bIsPointerHoveringOver = false;
						bool bIsPointerReleased = false;

						//if (g.vrglobals.GGVREnabled > 0 && g.vrglobals.GGVRUsingVRSystem == 1)
						extern int g_iActivelyUsingVRNow;
						if (g.vrglobals.GGVREnabled > 0 && g_iActivelyUsingVRNow == 1)
						{
							// VR support
							int iObjToHit = 5997;
							float fX = 0, fY = 0, fZ = 0;
							int iHitIt = GGVR_GetLaserGuidedHit (iObjToHit, &fX, &fY, &fZ);
							float fptrrealX = ((fX + 19.0f) / 38.0f) * (rMonitorArea.Max.x - rMonitorArea.Min.x);
							float fptrrealY = ((11.0f - fY) / 22.0f) * (rMonitorArea.Max.y - rMonitorArea.Min.y);
							if (GGVR_RightController_Trigger() > 0.5f)
							{
								bIsPointerReleased = true;
								ImVec2 topLeft = rMonitorArea.Min + widget_pos - vLargerGrabArea;
								ImVec2 bottomRight = rMonitorArea.Min + widget_pos + widget_size + vLargerGrabArea;
								if (fptrrealX > topLeft.x && fptrrealX < bottomRight.x)
								{
									if (fptrrealY > topLeft.y && fptrrealY < bottomRight.y)
									{
										bIsPointerHoveringOver = true;
									}
								}
							}
						}
						else
						{
							// non VR
							if (ImGui::IsMouseHoveringRect(rMonitorArea.Min + widget_pos - vLargerGrabArea, rMonitorArea.Min + widget_pos + widget_size + vLargerGrabArea)) bIsPointerHoveringOver = true;
							if (ImGui::IsMouseReleased(0)) bIsPointerReleased = true;
						}
						// 2026-08-05: automation TITLE_CLICK — fire this widget as if hovered+
						// released when its action matches the queued auto trigger, so click
						// sound + action dispatch run the user's exact code path
						if (g_iAutoTriggerScreenAction != 0 &&
							Storyboard.Nodes[nodeid].widget_action[index] == g_iAutoTriggerScreenAction)
						{
							bIsPointerHoveringOver = true;
							bIsPointerReleased = true;
							g_iAutoTriggerScreenAction = 0;
						}
						if (bIsPointerHoveringOver)
						{
							//if mouse release.
							if (bIsPointerReleased)
							{
								if (strlen(Storyboard.Nodes[nodeid].widget_click_sound[index]) > 0)
								{
									cTriggerButtonClickSound = Storyboard.Nodes[nodeid].widget_click_sound[index];
								}

								if (Storyboard.Nodes[nodeid].widget_action[index] == STORYBOARD_ACTIONS_NONE)
								{
									// depends on name of this button for the action
									int iActionTypeInternalByName = 0;
									if (stricmp(Storyboard.Nodes[nodeid].widget_label[index], "HIGHEST") == 0) iActionTypeInternalByName = 1;
									if (stricmp(Storyboard.Nodes[nodeid].widget_label[index], "MEDIUM") == 0) iActionTypeInternalByName = 3;
									if (stricmp(Storyboard.Nodes[nodeid].widget_label[index], "LOW") == 0) iActionTypeInternalByName = 4;
									if (iActionTypeInternalByName >= 1 && iActionTypeInternalByName <= 4)
									{
										extern void visuals_shaderlevels_setlevel (int, bool);
										visuals_shaderlevels_setlevel(iActionTypeInternalByName,true);
									}
								}

								if (Storyboard.Nodes[nodeid].widget_action[index] == STORYBOARD_ACTIONS_RETURNVALUETOLUA)
								{
									iSpecialLuaReturn = index;
									iRet = STORYBOARD_ACTIONS_RETURNVALUETOLUA;
									if (stricmp(Storyboard.Nodes[nodeid].lua_name, "savegame.lua") == 0)
									{
										if (strlen(Storyboard.Nodes[nodeid].screen_music) > 0) //PE: Only stop music if we have our own.
											bLuaPageClosing = true;
									}
									if (stricmp(Storyboard.Nodes[nodeid].lua_name, "loadgame.lua") == 0)
									{
										if (index >= 1 && index <= 8)
										{
											if (!pestrcasestr(LoadGameTitle[index], "EMPTY PROGRESS SLOT"))
											{
												if (strlen(Storyboard.Nodes[nodeid].screen_music) > 0) //PE: Only stop music if we have our own.
													bLuaPageClosing = true;
											}
										}
									}
								}
								//Find new node to execute. SwitchPage("")
								if (Storyboard.Nodes[nodeid].widget_action[index] == STORYBOARD_ACTIONS_BACK)
								{
									lua_switchpageback();
									if (strlen(Storyboard.Nodes[nodeid].screen_music) > 0) //PE: Only stop music if we have our own.
										bLuaPageClosing = true;
									iRet = STORYBOARD_ACTIONS_BACK;
								}
								if (Storyboard.Nodes[nodeid].widget_action[index] == STORYBOARD_ACTIONS_CONTINUE)
								{
									t.s_s = "";
									lua_switchpage();
									if (strlen(Storyboard.Nodes[nodeid].screen_music) > 0) //PE: Only stop music if we have our own.
										bLuaPageClosing = true;
									iRet = STORYBOARD_ACTIONS_CONTINUE;
								}
								if (Storyboard.Nodes[nodeid].widget_action[index] == STORYBOARD_ACTIONS_GOTOLEVEL)
								{
									//Support go to level directly ?
									iRet = STORYBOARD_ACTIONS_GOTOLEVEL;
								
									int iNewNode = FindOutputScreenNode(nodeid, index);
									if (iNewNode >= 0)
									{
										t.s_s = "";
										lua_switchpage();
										bLuaPageClosing = true;

										// may have linked to loading screen
										if (strlen(Storyboard.Nodes[iNewNode].level_name) == 0)
										{
											// will use last 'specified' loading screen
											extern cstr g_Storyboard_LoaderScreen_Name;
											g_Storyboard_LoaderScreen_Name = Storyboard.Nodes[iNewNode].lua_name;

											// if so, find out which level it goes to
											int input_id_of_level = Storyboard.Nodes[iNewNode].output_linkto[0];
											for (int findnode = 0; findnode < STORYBOARD_MAXNODES; findnode++)
											{
												if (Storyboard.Nodes[findnode].input_id[0] == input_id_of_level)
												{
													// change from loading node to level node
													iNewNode = findnode;
													break;
												}
											}
										}
									
										// must ultimately link to a level node!
										if (strlen(Storyboard.Nodes[iNewNode].level_name) > 0)
										{
											g_Storyboard_Current_Level = iNewNode;
											strcpy(g_Storyboard_Current_fpm, Storyboard.Nodes[iNewNode].level_name);

											//Clean name.
											std::string sLevelTitle = g_Storyboard_Current_fpm;
											replaceAll(sLevelTitle, ".fpm", "");
											replaceAll(sLevelTitle, "mapbank\\", "");
											t.game.jumplevel_s = sLevelTitle.c_str();
											extern bool g_Storyboard_Starting_New_Level;
											g_Storyboard_Starting_New_Level = true; //PE: Always start fresh when linking directly to a level.
										}
									}
									else
									{
										//PE: Not linked , start first level.
										t.s_s = "";
										lua_switchpage();
										bLuaPageClosing = true;
										iRet = STORYBOARD_ACTIONS_STARTGAME;

										//PE: Always use first level.
										FindFirstLevel(g_Storyboard_First_Level_Node, g_Storyboard_First_fpm);
										g_Storyboard_Current_Level = g_Storyboard_First_Level_Node;
										strcpy(g_Storyboard_Current_fpm, g_Storyboard_First_fpm);
										//Clean name.
										std::string sLevelTitle = g_Storyboard_First_fpm;
										replaceAll(sLevelTitle, ".fpm", "");
										replaceAll(sLevelTitle, "mapbank\\", "");
										t.game.jumplevel_s = sLevelTitle.c_str();
										extern bool g_Storyboard_Starting_New_Level;
										g_Storyboard_Starting_New_Level = true; //PE: Start a fresh game.
										// reset 'specified' loading screen
										extern cstr g_Storyboard_LoaderScreen_Name;
										g_Storyboard_LoaderScreen_Name = "loading";
									}
								}
								if (Storyboard.Nodes[nodeid].widget_action[index] == STORYBOARD_ACTIONS_STARTGAME)
								{
									t.s_s = "";
									lua_switchpage();
									bLuaPageClosing = true;
									iRet = STORYBOARD_ACTIONS_STARTGAME;

									//PE: Always use first level.
									FindFirstLevel(g_Storyboard_First_Level_Node, g_Storyboard_First_fpm);
									g_Storyboard_Current_Level = g_Storyboard_First_Level_Node;
									strcpy(g_Storyboard_Current_fpm, g_Storyboard_First_fpm);
									//Clean name.
									std::string sLevelTitle = g_Storyboard_First_fpm;
									replaceAll(sLevelTitle, ".fpm", "");
									replaceAll(sLevelTitle, "mapbank\\", "");
									t.game.jumplevel_s = sLevelTitle.c_str();
									extern bool g_Storyboard_Starting_New_Level;
									g_Storyboard_Starting_New_Level = true; //PE: Start a fresh game.
									// reset 'specified' loading screen
									extern cstr g_Storyboard_LoaderScreen_Name;
									g_Storyboard_LoaderScreen_Name = "loading";
								}
								if (Storyboard.Nodes[nodeid].widget_action[index] == STORYBOARD_ACTIONS_LEAVEGAME)
								{
									lua_leavegame();
									bLuaPageClosing = true;
									iRet = STORYBOARD_ACTIONS_LEAVEGAME;
									bLastStandalone = false;
								}
								if (Storyboard.Nodes[nodeid].widget_action[index] == STORYBOARD_ACTIONS_RESUMEGAME)
								{
									lua_resumegame();
									bLuaPageClosing = true;
									iRet = STORYBOARD_ACTIONS_RESUMEGAME;
								}

								if (Storyboard.Nodes[nodeid].widget_action[index] == STORYBOARD_ACTIONS_EXITGAME)
								{
									lua_quitgame();
									bLuaPageClosing = true;
									iRet = STORYBOARD_ACTIONS_EXITGAME;
									bLastStandalone = false;
									extern bool bSpecialStandalone;
									if (bSpecialStandalone)
									{
										extern bool g_bCascadeQuitFlag;
										g_bCascadeQuitFlag = true;
										PostQuitMessage(0);
										//PE: Launch editor again.
										//editorfromstandalone=
										SetCurrentDirectoryA("..\\");
										char par[MAX_PATH];
										extern bool bReturnToWelcome;
										if(bReturnToWelcome)
											sprintf(par, "editorfromstandalone2=%s", Storyboard.gamename);
										else
											sprintf(par, "editorfromstandalone=%s", Storyboard.gamename);
										ExecuteFile("GameGuruMAX.exe", par, "", 0);
										Sleep(500);
										ExitProcess(0);
									}

								}
								if (Storyboard.Nodes[nodeid].widget_action[index] == STORYBOARD_ACTIONS_GOTOSCREEN)
								{
									if ( _stricmp(Storyboard.Nodes[nodeid].lua_name,"gamemenu.lua") == 0 ) // output_* CANNOT be trusted!
									{
										//PE: Special mode where we need to follow output_action.
										//PE: This is a mess, and need to be changed when we change the system to ALL buttons have outlinks.
										//LB: Agreed, seems changing the in-game menu screen messed 'output_action' list, and the code below is a narsty hack!
										//The good news is that output_title stores the correct linkages as they are correct in Storyboard :)
										//i.e. strcpy(chr, Storyboard.Nodes[node].widget_label[ll]); strcat(chr, " -> Connect to Level"); strcpy(Storyboard.Nodes[node].output_title[outlinknum], chr);
										int iNodeToLinkTo = 0;
										LPSTR pWidgetLabelName = Storyboard.Nodes[nodeid].widget_label[index];
										std::string lua_name = "";
										for (int outlinkageindex = 0; outlinkageindex < STORYBOARD_MAXOUTPUTS; outlinkageindex++)
										{
											char pOutLinkTitleName[256];
											strcpy(pOutLinkTitleName, Storyboard.Nodes[nodeid].output_title[outlinkageindex]);
											LPSTR pConnectionTag = " -> Connect to Level";
											if (strlen(pOutLinkTitleName) > strlen(pConnectionTag))
											{
												pOutLinkTitleName[strlen(pOutLinkTitleName) - strlen(pConnectionTag) - 1] = 0;
												if (strstr(pOutLinkTitleName, pWidgetLabelName) != NULL)
												{
													// found the actual outlinkageindex, find where we link to
													int iLinkTo = Storyboard.Nodes[nodeid].output_linkto[outlinkageindex];
													if (iLinkTo > 0)
													{
														for (int i = 0; i < STORYBOARD_MAXNODES; i++)
														{
															if (Storyboard.Nodes[i].used)
															{
																for (int l = 0; l < STORYBOARD_MAXOUTPUTS; l++)
																{
																	if (iLinkTo == Storyboard.Nodes[i].input_id[l])
																	{
																		iNodeToLinkTo = i;
																		break;
																	}
																}
															}
															if (iNodeToLinkTo > 0) break;
														}
													}
												}
											}
											if (iNodeToLinkTo > 0)
												break;
										}
										if (iNodeToLinkTo > 0)
										{
											// screens can have same name (old corruption issue), so new method to identify screen by node
											std::string node_ident_name = ":node:";
											node_ident_name += std::to_string(iNodeToLinkTo);
											t.s_s = node_ident_name.c_str();
											lua_switchpage();

											bLuaPageClosing = true; //always stop music.
											iRet = STORYBOARD_ACTIONS_GOTOSCREEN;
										}
									}
									else
									{
										int iNewNode = FindOutputScreenNode(nodeid, index);
										if (iNewNode >= 0)
										{
											//Connected.
											if (Storyboard.Nodes[iNewNode].type == STORYBOARD_TYPE_SCREEN)
											{
												if (strlen(Storyboard.Nodes[iNewNode].lua_name) > 0)
												{
													// screens can have same name (old corruption issue), so new method to identify screen by node
													std::string node_ident_name = ":node:"; 
													node_ident_name += std::to_string(iNewNode);
													t.s_s = node_ident_name.c_str();
													lua_switchpage();
													if (strlen(Storyboard.Nodes[iNewNode].screen_music) > 0) //PE: Only stop music if new swcreen have its own.
														bLuaPageClosing = true;
													iRet = STORYBOARD_ACTIONS_GOTOSCREEN;
												}
											}
										}
										else
										{
											//PE: Not linked, check if we have a direct link to screen without a pin connection.
											if (index < STORYBOARD_MAXOUTPUTS)
											{
												if (strlen(Storyboard.Nodes[nodeid].output_title[index]) <= 0) //Empty no output pin.
												{
													if (Storyboard.Nodes[nodeid].output_can_link_to_type[index] == STORYBOARD_TYPE_SCREEN)
													{
														if (strlen(Storyboard.Nodes[nodeid].output_action[index]) > 0)
														{
															if (Storyboard.Nodes[nodeid].output_linkto[index] == 0)
															{
																// screens can have same name (old corruption issue), so new method to identify screen by node
																std::string node_ident_name = ":node:";
																node_ident_name += std::to_string(nodeid);
																t.s_s = node_ident_name.c_str();
																lua_switchpage();

																bLuaPageClosing = true; //always stop music.
																iRet = STORYBOARD_ACTIONS_GOTOSCREEN;
															}
														}
													}
												}
											}

										}
									}
								}
								int iActionID = Storyboard.Nodes[nodeid].widget_action[index];
								if (iActionID >= STORYBOARD_ACTIONS_GOTOSCREENHUD2
								&&  iActionID <= STORYBOARD_ACTIONS_GOTOSCREENHUD32)
								{
									// Toggle to new HUD screen ( can be improved this 'ard use of widget_action )
									for (int i = 0; i < STORYBOARD_MAXNODES; i++)
									{
										StoryboardNodesStruct& node = Storyboard.Nodes[i];
										if (node.used && strlen(node.level_name) == 0) // only HUDs
										{
											bool bFoundHUDScreen = false;
											int iHUDNumber = 2 + (iActionID - STORYBOARD_ACTIONS_GOTOSCREENHUD2);
											char pHUDScreenName[256];
											sprintf(pHUDScreenName, "HUD Screen %d", iHUDNumber);
											if (stricmp (node.title, pHUDScreenName) == NULL) bFoundHUDScreen = true;
											if (bFoundHUDScreen == true )
											{
												t.game.activeStoryboardScreen = i;
												break;
											}
										}
									}
								}
							}
						}
					}
				}
				else
				{
					if (bPreviewScreen)
					{
						//Trigger any sound from buttons.
						if (Storyboard.Nodes[nodeid].widget_type[index] == STORYBOARD_WIDGET_BUTTON)
						{
							ImVec2 vLargerGrabArea = ImVec2(10.0, 10.0);
							if (ImGui::IsMouseHoveringRect(rMonitorArea.Min + widget_pos - vLargerGrabArea, rMonitorArea.Min + widget_pos + widget_size + vLargerGrabArea))
							{
								//if mouse release.
								if (ImGui::IsMouseReleased(0))
								{
									if (strlen(Storyboard.Nodes[nodeid].widget_click_sound[index]) > 0)
									{
										cTriggerButtonClickSound = Storyboard.Nodes[nodeid].widget_click_sound[index];
									}
								}
							}
						}
					}
				}
				if (cTriggerButtonClickSound != "")
				{
					//Play sound.
					int iFreeSoundID = g.temppreviewsoundoffset + 4; //Button Sound.
					// play music
					if (SoundExist(iFreeSoundID) == 1) DeleteSound(iFreeSoundID);
					if (FileExist(cTriggerButtonClickSound.Get()) == 1)
					{
						LoadSound(cTriggerButtonClickSound.Get(), iFreeSoundID, 0, 1);
						if (SoundExist(iFreeSoundID) == 1)
							PlaySound(iFreeSoundID);
					}
					cTriggerButtonClickSound = "";
				}
				if (bLuaPageClosing)
				{
					//Stop music ...
					int iFreeSoundID = g.temppreviewsoundoffset + 2;
					if (SoundExist(iFreeSoundID) == 1 && SoundPlaying(iFreeSoundID) == 1)
					{
						// stop currently playing preview
						StopSound(iFreeSoundID);
					}
				}

				//Set Slider Values.
				if (!bReadOnly && iQuitWindowLoop <= 0 && (bPreviewScreen || standalone) )
				{
					if (Storyboard.Nodes[nodeid].widget_type[index] == STORYBOARD_WIDGET_SLIDER)
					{
						ImVec2 vLargerGrabArea = ImVec2(10.0, 10.0);
						if (ImGui::IsMouseHoveringRect(rMonitorArea.Min + widget_pos - vLargerGrabArea, rMonitorArea.Min + widget_pos + widget_size + vLargerGrabArea))
						{
							if (ImGui::IsMouseDown(0))
							{
								//Percent.
								ImVec2 mousecords = ImGui::GetMousePos();
								ImVec2 xy = mousecords - (rMonitorArea.Min + widget_pos);
								float percent = xy.x / (widget_size.x / 100.0);

								if (mousecords.x > (rMonitorArea.Min.x + widget_pos.x + widget_size.x))
									percent = 100.0;
								else if (percent < 0.0)
									percent = 0.0;
								else if (percent > 100.0)
									percent = 100.0;

								iSpecialLuaReturn = index;

								// If connected to readout, update the readout value
								if (strlen(Storyboard.widget_readout[nodeid][index]) > 0)
								{
									SetReadoutValueInt(Storyboard.widget_readout[nodeid][index], percent);
								}
								Storyboard.NodeSliderValues[nodeid][index] = percent;
							}
						}
					}
					if (Storyboard.Nodes[nodeid].widget_type[index] == STORYBOARD_WIDGET_RADIOTYPE)
					{
						if (ImGui::IsMouseHoveringRect(rMonitorArea.Min + widget_pos , rMonitorArea.Min + widget_pos + widget_size ))
						{
							if (ImGui::IsMouseDown(0))
							{
								// used by GRAPHICS SETTINGS (1,2,3)
								int iMatchToSettingValue = -1;
								if (stricmp(Storyboard.Nodes[nodeid].widget_label[index], "LOW") == NULL) iMatchToSettingValue = 1;
								if (stricmp(Storyboard.Nodes[nodeid].widget_label[index], "MEDIUM") == NULL) iMatchToSettingValue = 2;
								if (stricmp(Storyboard.Nodes[nodeid].widget_label[index], "HIGHEST") == NULL) iMatchToSettingValue = 3;
								Storyboard.NodeRadioButtonSelected[nodeid] = iMatchToSettingValue;
								iSpecialLuaReturn = iMatchToSettingValue;
							}
						}
					}
					if (Storyboard.Nodes[nodeid].widget_type[index] == STORYBOARD_WIDGET_TICKBOX)
					{
						if (ImGui::IsMouseHoveringRect(rMonitorArea.Min + widget_pos, rMonitorArea.Min + widget_pos + widget_size))
						{
							if (ImGui::IsMouseClicked(0))
							{
								// If connected to readout, update the readout value
								if (strlen(Storyboard.widget_readout[nodeid][index]) > 0)
								{
									bool bEnabled = GetReadoutValueInt(Storyboard.widget_readout[nodeid][index]);
									bEnabled = !bEnabled;
									SetReadoutValueInt(Storyboard.widget_readout[nodeid][index], bEnabled);
								}
								else
								{
									Storyboard.NodeSliderValues[nodeid][index] = 1.0f - Storyboard.NodeSliderValues[nodeid][index];
								}
								iSpecialLuaReturn = index;
							}
						}
					}
				}
				//Control Widget.
				if (iCurrentSelectedWidget == index && !standalone)
				{
					if (!bReadOnly && iQuitWindowLoop <= 0 && !bPreviewScreen)
					{
						storyboard_control_widget(nodeid, index, widget_pos, widget_size, rMonitorArea, vMonitorStart, vScale);
					}
				}

				if (iSkipWidgetSelectionForFrames > 0) //PE: Make sure we dont select anything after a window on top.
					iSkipWidgetSelectionForFrames--;

				ImGui::SetWindowFontScale(1.0*vScale.y * fFontScale);
				ImGui::PopFont();
			}
		}

		ImVec4 leftPanelBoxCol = ImGui::GetStyle().Colors[ImGuiCol_ChildBg];
		leftPanelBoxCol.w = 1.0f;
		window->DrawList->AddRectFilled(leftPanelAABB.Min, leftPanelAABB.Max, ImGui::GetColorU32(leftPanelBoxCol));

		if (!bPreviewScreen && !standalone)
		{	
			int index = iCurrentSelectedWidget;
			// Check for widget being dragged to left panel (for deletion)
			ImVec2 widgetSize = ImVec2(1.0f, 1.0f);
			if (ImageExist(Storyboard.Nodes[nodeid].widget_normal_thumb_id[index]))
			{
				widgetSize.x = ImageWidth(Storyboard.Nodes[nodeid].widget_normal_thumb_id[index]);
				widgetSize.y = ImageHeight(Storyboard.Nodes[nodeid].widget_normal_thumb_id[index]);
			}
			else
			{
				widgetSize = ImVec2(500, 74); //Default widget size for text
			}
#ifdef EMULATERESOLUTION
			if (Storyboard.Nodes[nodeid].universal_resolution[index])
			{
				widgetSize = widgetSize * vUniversalScale;
				widgetSize = widgetSize * fUniversalGlobalScale;
				widgetSize = widgetSize * Storyboard.Nodes[nodeid].widget_size[index];
			}
			else
#endif
			{
				widgetSize = widgetSize * vScale; //Scale to visible screen size.
				widgetSize = widgetSize * fGlobalScale;
				widgetSize = widgetSize * Storyboard.Nodes[nodeid].widget_size[index];
			}
			ImVec2 fOnePercentScreen = ImVec2(vMonitorSize.x / 100.0, vMonitorSize.y / 100.0);
			ImVec2 widgetPos = Storyboard.Nodes[nodeid].widget_pos[index] * fOnePercentScreen; //Real screen pos.
			ImVec2 centerOffset = ImVec2(widgetSize.x / 2.0f, 0.0f);
			ImVec2 widgetMin = rMonitorArea.Min + widgetPos - centerOffset;
			ImVec2 widgetMax = rMonitorArea.Min + widgetPos + widgetSize - centerOffset;
			currentWidgetAABB = ImRect(widgetMin, widgetMax);
		}

		ImGui::SetWindowFontScale(1.0);
		ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX() + vMonitorCenterX + vMonitorPos.x, vHeaderEnd.y + vMonitorSize.y + vMonitorPos.y + 3.0));
		ImGui::SetCursorPos( ImVec2(ImGui::GetCursorPosX(), vHeaderEnd.y + fMaxMonitorY + 6.0));

		bool bPrevPreviewScreen = bPreviewScreen;

		if (!bPreviewScreen)
			ImGui::NextColumn();

		if (!bPreviewScreen && iCurrentSelectedWidget >= 0)
		{
			float w = ImGui::GetContentRegionAvailWidth(); 
			bool bReadOnly = Storyboard.Nodes[nodeid].widget_read_only[iCurrentSelectedWidget];
			bool bSpecialNoText = false;
			if (bReadOnly)
			{
				bSpecialNoText = true;
				if (iCurrentSelectedWidget >= 1 && iCurrentSelectedWidget <= 8 && ( stricmp(Storyboard.Nodes[nodeid].lua_name, "loadgame.lua") == 0 || stricmp(Storyboard.Nodes[nodeid].lua_name, "savegame.lua") == 0 ) )
				{
					strcpy(Storyboard.Nodes[nodeid].widget_label[iCurrentSelectedWidget], LoadGameTitle[iCurrentSelectedWidget]);
					bReadOnly = false;
				}
			}

			if (bReadOnly)
			{
				//PE: Disable ALL gadgets and moving/rotation/scaling.
				ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
				ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
			}

			if (!standalone && !bPreviewScreen)
			{
				ImGui::BeginChild("##SERightPanel", ImVec2(0,0), false, 0);
			}

			float buttonwide = 200.0f;
			if (Storyboard.Nodes[nodeid].type != STORYBOARD_TYPE_HUD)
			{
				if (ImGui::StyleCollapsingHeader("Screen Media", ImGuiTreeNodeFlags_DefaultOpen))
				{
					ImGui::Indent(10);

					cstr cBackDrop = Storyboard.Nodes[nodeid].screen_backdrop;
					if (pestrcasestr(Storyboard.Nodes[nodeid].screen_backdrop, "editors\\"))
					{
						cBackDrop = "default";
					}
					cstr cNewBackDrop = imgui_setpropertyfile2_v2(0, cBackDrop.Get(), "Backdrop Image", "Select Backdrop", "imagebank\\", false, "backdrop");
					if (cNewBackDrop != cBackDrop)
					{
						strcpy(Storyboard.Nodes[nodeid].screen_backdrop, cNewBackDrop.Get());
						iUpdateBackDropNode = nodeid; //Update thumb.
					}

					if (strlen(Storyboard.Nodes[nodeid].screen_backdrop) > 0)
					{
						ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(((ImGui::GetContentRegionAvail().x - 14.f)*0.5) - (buttonwide*0.5), 0.0f));
						if (ImGui::StyleButton("Remove Backdrop", ImVec2(buttonwide, 0.0f)))
						{
							strcpy(Storyboard.Nodes[nodeid].screen_backdrop, "");
							iUpdateBackDropNode = nodeid; //Update thumb.
						}
						if (!Storyboard.Nodes[nodeid].screen_backdrop_transparent)
						{
							//PE: thump problem ?
							ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(((ImGui::GetContentRegionAvail().x - 14.f)*0.5) - (buttonwide*0.5), 0.0f));
							if (ImGui::StyleButton("Copy Backdrop to All Screens##1", ImVec2(buttonwide, 0.0f)))
							{
								int iAction = askBoxCancel("This will copy your backdrop to all screens, are you sure ?\nNOTE: You need to edit your screens after this change, to update the thumbnails.", "Confirmation"); //1==Yes 2=Cancel 0=No
								if (iAction == 1)
								{
									strcpy(cCopyToAllScreens, Storyboard.Nodes[nodeid].screen_backdrop);
									iUpdateBackDropNode = 99999; //Update ALL thumbs. ???
								}
							}

						}
					}

					if (pref.iStoryboardAdvanced)
					{
						ImGui::TextCenter("Backdrop Position");

						const char* backdrop_placement[] = { "Backdrop Center" , "Backdrop Stretch", "Backdrop Zoom" };
						ImGui::PushItemWidth(-10);
						if (ImGui::Combo("##screen_backdrop_placement", &Storyboard.Nodes[nodeid].screen_backdrop_placement, backdrop_placement, IM_ARRAYSIZE(backdrop_placement)))
						{
							//
						}
						ImGui::PopItemWidth();


						bool bTmp = Storyboard.Nodes[nodeid].screen_backdrop_transparent;
						ImGui::Checkbox("Transparent Backdrop", &bTmp);
						Storyboard.Nodes[nodeid].screen_backdrop_transparent = bTmp;
					}

					//ImGui::TextCenter("Music Track");
					cstr cMusicTrack = Storyboard.Nodes[nodeid].screen_music;
					cstr cNewMusicTrack = imgui_setpropertyfile2_v2(0, cMusicTrack.Get(), "Music Track", "Select Music Track", "audiobank\\", false);
					if (cNewMusicTrack != cMusicTrack)
					{
						strcpy(Storyboard.Nodes[nodeid].screen_music, cNewMusicTrack.Get());
					}

					if (strlen(Storyboard.Nodes[nodeid].screen_music) > 0)
					{
						bool bTmp = Storyboard.Nodes[nodeid].loop_music;
						if (ImGui::Checkbox("Loop Music", &bTmp))
						{
							Storyboard.Nodes[nodeid].loop_music = bTmp;
						}

						ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(((ImGui::GetContentRegionAvail().x - 14.0f)*0.5) - (buttonwide*0.5), 0.0f));
						if (ImGui::StyleButton("Remove Music", ImVec2(buttonwide, 0.0f)))
						{
							strcpy(Storyboard.Nodes[nodeid].screen_music, "");
						}

						ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(((ImGui::GetContentRegionAvail().x - 14.f)*0.5) - (buttonwide*0.5), 0.0f));
						if (ImGui::StyleButton("Copy Music to All Screens", ImVec2(buttonwide, 0.0f)))
						{
							int iAction = askBoxCancel("This will copy your music to all screens, are you sure?", "Confirmation"); //1==Yes 2=Cancel 0=No
							if (iAction == 1)
							{
								for (int i = 0; i < STORYBOARD_MAXWIDGETS; i++)
								{
									if (Storyboard.Nodes[i].used && Storyboard.Nodes[i].type == STORYBOARD_TYPE_SCREEN)
									{
										//PE: Only title,about,won,lost,loading
										bool bValid = false;
										if (i == iTitleScreenNodeID) bValid = true;
										if (i == iLoadingScreenNodeID) bValid = true;
										if (i == iAboutScreenNodeID) bValid = true;
										if (i == iGameWonScreenNodeID) bValid = true;
										if (i == iGameLostScreenNodeID) bValid = true;
										if (bValid && i != nodeid)
										{
											//PE: Copy music to scene.
											strcpy(Storyboard.Nodes[i].screen_music, Storyboard.Nodes[nodeid].screen_music);
										}
									}
								}
							}
						}

					}

					ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(((ImGui::GetContentRegionAvail().x - 14.0)*0.5) - (buttonwide*0.5), 0.0f));
					if (ImGui::StyleButton("Preview Screen", ImVec2(buttonwide, 0.0f)))
					{
						bPreviewScreen = true;
						int iFreeSoundID = g.temppreviewsoundoffset + 2;
						if (strlen(Storyboard.Nodes[nodeid].screen_music) > 0)
						{
							// play music
							if (SoundExist(iFreeSoundID) == 1) DeleteSound(iFreeSoundID);
							if (FileExist(Storyboard.Nodes[nodeid].screen_music) == 1)
							{
								LoadSound(Storyboard.Nodes[nodeid].screen_music, iFreeSoundID, 0, 1);
								if (SoundExist(iFreeSoundID) == 1)
								{
									if(Storyboard.Nodes[nodeid].loop_music)
										LoopSound(iFreeSoundID);
									else
										PlaySound(iFreeSoundID);
								}
							}
						}
					}

#ifdef EMULATERESOLUTION
					if (!standalone && !bPreviewScreen)
					{
						ImGui::TextCenter("Emulate Resolution");

						const char* items[] = { &CurrentResolution[0] , "1920x1080 (16:9)",  "3440x1440 (21:9) Wide" , "5120x1440 (32x9) Ultrawide" }; //, "1600x1200 (4:3)","1080x2400 (FHD)" , "3840x1080 (FAKE)"
						static int monitor_resolution_current_selection = 0; //Default Custom.
						ImGui::PushItemWidth(-1);
						if (ImGui::Combo("##CustomResolutionSelecting", &monitor_resolution_current_selection, items, IM_ARRAYSIZE(items)))
						{
						}
						if (ImGui::IsItemHovered()) ImGui::SetTooltip("Select Resolution");
						ImGui::PopItemWidth();

						if (iQuitWindowLoop > 0 || monitor_resolution_current_selection == 0)
						{
							monitor_size_x = CurrentMonitorResolutionX;
							monitor_size_y = CurrentMonitorResolutionY;
						}
						else if (monitor_resolution_current_selection == 1)
						{
							monitor_size_x = 1920.0f;
							monitor_size_y = 1080.0f;
						}
						else if (monitor_resolution_current_selection == 3)
						{
							monitor_size_x = 5120.0f;
							monitor_size_y = 1440.0f;
						}
						else if (monitor_resolution_current_selection == 2)
						{
							monitor_size_x = 3440.0f;
							monitor_size_y = 1440.0f;
						}
						else if (monitor_resolution_current_selection == 4)
						{
							monitor_size_x = 3840.0f;
							monitor_size_y = 1080.0f;
						}
						else if (monitor_resolution_current_selection == 5)
						{
							monitor_size_x = 1080.0f;
							monitor_size_y = 2400.0f;
						}
						else
						{
							monitor_size_x = CurrentMonitorResolutionX;
							monitor_size_y = CurrentMonitorResolutionY;
						}
					}

#else
					int preview_size_x = ImGui::GetMainViewport()->Size.x - 270;
					int preview_size_y = ImGui::GetMainViewport()->Size.y - 30.0;
#endif

					ImGui::Indent(-10);

				}
			}
			
			if (Storyboard.Nodes[nodeid].type == STORYBOARD_TYPE_HUD)
			{
				if (ImGui::StyleCollapsingHeader("Screen", ImGuiTreeNodeFlags_DefaultOpen))
				{
					ImGui::Indent(10);
					Storyboard.Nodes[nodeid].screen_backdrop_transparent = true;

					// "In-Game HUD" is the default HUD screen, and currently cannot have its visibility conditions changed
					// If we plan to make any HUD screen a possible default then hud0.lua will need changed to accomodate this
					if (strcmp(Storyboard.Nodes[nodeid].title, "In-Game HUD") != 0)
					{
						// Display key that will make this screen appear in-game
						Storyboard.Nodes[nodeid].toggleKey;
						char* toggleKey = "NONE";
						if (Storyboard.Nodes[nodeid].toggleKey > 0)
						{
							char scanCodeName[128];
							int result = GetScancodeName(Storyboard.Nodes[nodeid].toggleKey, scanCodeName, 128);
							toggleKey = scanCodeName;
						}

						ImGui::Text("Screen Toggle Key: %s", toggleKey);
						if (ImGui::Button("Change Toggle Key"))
						{
							bScreenToggleKeyWindow = true;
						}
						bool bShowAtStart = Storyboard.Nodes[nodeid].showAtStart;
						if (ImGui::Checkbox("Show Screen At Start", &bShowAtStart))
						{
							Storyboard.Nodes[nodeid].showAtStart = bShowAtStart;
						}
					}
					ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(((ImGui::GetContentRegionAvail().x - 14.0)*0.5) - (buttonwide*0.5), 0.0f));
					if (ImGui::StyleButton("Preview Screen", ImVec2(buttonwide, 0.0f)))
					{
						bPreviewScreen = true;
						int iFreeSoundID = g.temppreviewsoundoffset + 2;
						if (strlen(Storyboard.Nodes[nodeid].screen_music) > 0)
						{
							// play music
							if (SoundExist(iFreeSoundID) == 1) DeleteSound(iFreeSoundID);
							if (FileExist(Storyboard.Nodes[nodeid].screen_music) == 1)
							{
								LoadSound(Storyboard.Nodes[nodeid].screen_music, iFreeSoundID, 0, 1);
								if (SoundExist(iFreeSoundID) == 1)
								{
									if (Storyboard.Nodes[nodeid].loop_music)
										LoopSound(iFreeSoundID);
									else
										PlaySound(iFreeSoundID);
								}
							}
						}
					}

#ifdef EMULATERESOLUTION
					if (!standalone && !bPreviewScreen)
					{
						ImGui::TextCenter("Emulate Resolution");

						const char* items[] = { &CurrentResolution[0] , "1920x1080 (16:9)",  "3440x1440 (21:9) Wide" , "5120x1440 (32x9) Ultrawide" }; //, "1600x1200 (4:3)","1080x2400 (FHD)" , "3840x1080 (FAKE)"
						static int monitor_resolution_current_selection = 0; //Default Custom.
						ImGui::PushItemWidth(-1);
						if (ImGui::Combo("##CustomResolutionSelecting", &monitor_resolution_current_selection, items, IM_ARRAYSIZE(items)))
						{
						}
						if (ImGui::IsItemHovered()) ImGui::SetTooltip("Select Resolution");
						ImGui::PopItemWidth();

						if (iQuitWindowLoop > 0 || monitor_resolution_current_selection == 0)
						{
							monitor_size_x = CurrentMonitorResolutionX;
							monitor_size_y = CurrentMonitorResolutionY;
						}
						else if (monitor_resolution_current_selection == 1)
						{
							monitor_size_x = 1920.0f;
							monitor_size_y = 1080.0f;
						}
						else if (monitor_resolution_current_selection == 3)
						{
							monitor_size_x = 5120.0f;
							monitor_size_y = 1440.0f;
						}
						else if (monitor_resolution_current_selection == 2)
						{
							monitor_size_x = 3440.0f;
							monitor_size_y = 1440.0f;
						}
						else if (monitor_resolution_current_selection == 4)
						{
							monitor_size_x = 3840.0f;
							monitor_size_y = 1080.0f;
						}
						else if (monitor_resolution_current_selection == 5)
						{
							monitor_size_x = 1080.0f;
							monitor_size_y = 2400.0f;
						}
						else
						{
							monitor_size_x = CurrentMonitorResolutionX;
							monitor_size_y = CurrentMonitorResolutionY;
						}
					}
#endif
					ImGui::Indent(-10);
				}
			}
			if (ImGui::StyleCollapsingHeader("Grid Settings", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::Indent(10);

				ImGui::TextCenter("Grid Size");
				ImGui::MaxSliderInputInt("##StoryboardGridSize", &Storyboard.Nodes[nodeid].screen_grid_size, 0, 10, "Set Grid Size");

				if (Storyboard.Nodes[nodeid].screen_grid_size > 0)
				{
					ImGui::Checkbox("Show Grid", &bDisplayGrid);
				}
				else
				{
					bool bTmp = false;
					ImGui::Checkbox("Show Grid", &bTmp);
				}

				ImGui::Indent(-10);
			}

			cstr sLabel = "Button Settings";
			cstr sPositionText = "Current Position";
			cstr sTextText = "Text";
			cstr sButtonText = "Button";
			if (Storyboard.Nodes[nodeid].widget_type[iCurrentSelectedWidget] == STORYBOARD_WIDGET_TEXT) { sLabel = "Text Settings"; sPositionText = "Current Text Position"; }
			if (Storyboard.Nodes[nodeid].widget_type[iCurrentSelectedWidget] == STORYBOARD_WIDGET_IMAGE)  sLabel = "Image Settings";
			if (Storyboard.Nodes[nodeid].widget_type[iCurrentSelectedWidget] == STORYBOARD_WIDGET_VIDEO)  sLabel = "Video Settings";
			if (Storyboard.Nodes[nodeid].widget_type[iCurrentSelectedWidget] == STORYBOARD_WIDGET_PROGRESS)  sLabel = "Progress Bar Settings";
			if (Storyboard.Nodes[nodeid].widget_type[iCurrentSelectedWidget] == STORYBOARD_WIDGET_SLIDER)  sLabel = "Slider Settings";
			if (Storyboard.Nodes[nodeid].widget_type[iCurrentSelectedWidget] == STORYBOARD_WIDGET_BAR) { sLabel = "Status Bar Settings"; sTextText = ""; sButtonText = "Bar"; }
			if (Storyboard.Nodes[nodeid].widget_type[iCurrentSelectedWidget] == STORYBOARD_WIDGET_TEXTAREA) { sLabel = "Text Area Settings"; sPositionText = "Current Text Position"; }
			if (Storyboard.Nodes[nodeid].widget_type[iCurrentSelectedWidget] == STORYBOARD_WIDGET_BUTTON) { sPositionText = "Current Button Position"; sTextText = "Button Text"; }
			if (Storyboard.Nodes[nodeid].widget_type[iCurrentSelectedWidget] == STORYBOARD_WIDGET_RADIOTYPE) { sLabel = "Radio Button Settings"; sTextText = "Radio Button Text"; }
		
			if (ImGui::StyleCollapsingHeader(sLabel.Get(), ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::Indent(10);

				char pLayerDesc[256];
				sprintf(pLayerDesc, "Current Draw Order Layer : %d", 2+Storyboard.widget_drawordergroup[nodeid][iCurrentSelectedWidget]);
				ImGui::TextCenter(pLayerDesc);

				ImGui::TextCenter(sPositionText.Get());

				ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(0.0f, 3.0f));
				ImGui::Text("X"); //PE: No room for XYZ labels in default width ?
				ImGui::SameLine();
				ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(0.0f, -3.0f));
				float scrollSizeX = ImGui::GetCurrentWindow()->ScrollbarSizes.x / 2.0f;
				ImGui::PushItemWidth(w * 0.5 - (ImGui::GetFontSize() * 2.0) - scrollSizeX);
				if (ImGui::InputFloat("##StoryboarTextPositionX", &Storyboard.Nodes[nodeid].widget_pos[iCurrentSelectedWidget].x, 0.0f, 0.0f, "%.0f")) //"%.2f"
				{
					//
				}
				if (!pref.iTurnOffEditboxTooltip && ImGui::IsItemHovered()) ImGui::SetTooltip("Change Widget Position X");
				ImGui::PopItemWidth();
				ImGui::SameLine();
				ImGui::Text("Y");
				ImGui::SameLine();
				ImGui::PushItemWidth(w * 0.5 - (ImGui::GetFontSize() * 2.0) - scrollSizeX);
				if (ImGui::InputFloat("##StoryboarTextPositionY", &Storyboard.Nodes[nodeid].widget_pos[iCurrentSelectedWidget].y, 0.0f, 0.0f, "%.0f")) //"%.2f"
				{
					//
				}
				if (!pref.iTurnOffEditboxTooltip && ImGui::IsItemHovered()) ImGui::SetTooltip("Change Widget Position Y");
				ImGui::PopItemWidth();

				// Determine if the widget is 'a'User Defined Global' powered (if so we nick the name field for our global var name) 
				int iUserDefinedGlobal = 0;
				std::string readout = Storyboard.widget_readout[nodeid][iCurrentSelectedWidget];
				if (stricmp(readout.c_str(), "User Defined Global") == NULL) iUserDefinedGlobal = 1;
				if (stricmp(readout.c_str(), "User Defined Global Text") == NULL) iUserDefinedGlobal = 1;
				if (stricmp(readout.c_str(), "User Defined Global Statusbar") == NULL) iUserDefinedGlobal = 2;
				if (stricmp(readout.c_str(), "User Defined Global Image") == NULL) iUserDefinedGlobal = 3;
				if (stricmp(readout.c_str(), "User Defined Global Panel") == NULL) iUserDefinedGlobal = 4;

				// skip text config if no text!
				int widgetType = Storyboard.Nodes[nodeid].widget_type[iCurrentSelectedWidget];
				if ( strlen(sTextText.Get())>0 )
				{
					if (widgetType != STORYBOARD_WIDGET_PROGRESS && widgetType != STORYBOARD_WIDGET_IMAGE && widgetType != STORYBOARD_WIDGET_VIDEO)
					{
						// text size determines size and if visible in HUD
						float fTextSize = Storyboard.Nodes[nodeid].widget_font_size[iCurrentSelectedWidget];

						// Can hide text from HUD (using negative text size trick)
						bool bHidingText = false;
						if (fTextSize < 0) bHidingText = true;
						if (ImGui::Checkbox("Hide Text", &bHidingText))
						{
							fTextSize = fabs(fTextSize);
							if (bHidingText == true) fTextSize = -fTextSize;
							Storyboard.Nodes[nodeid].widget_font_size[iCurrentSelectedWidget] = fTextSize;
						}

						// Normal text handling if not hidden
						if ( fTextSize > 0.0f )
						{
							if (!bSpecialNoText && iUserDefinedGlobal == 0)
							{
								ImGui::TextCenter(sTextText.Get());
								cstr UniqueTextFiledName = cstr("##WidgetTextStoryboardInput") + cstr(iCurrentSelectedWidget);
								ImGui::PushItemWidth(-10);
								if (Storyboard.Nodes[nodeid].widget_type[iCurrentSelectedWidget] == STORYBOARD_WIDGET_TEXTAREA)
								{
									ImGui::InputTextMultiline(UniqueTextFiledName.Get(), Storyboard.Nodes[nodeid].widget_label[iCurrentSelectedWidget], 250, ImVec2(0, 6.0 * ImGui::GetFontSize()), ImGuiInputTextFlags_None); //ImGuiInputTextFlags_ReadOnly
								}
								else
								{
									ImGui::InputText(UniqueTextFiledName.Get(), Storyboard.Nodes[nodeid].widget_label[iCurrentSelectedWidget], 250, ImGuiInputTextFlags_None); //ImGuiInputTextFlags_ReadOnly
								}
								if (ImGui::MaxIsItemFocused())
								{
									bImGuiGotFocus = true;
								}
								ImGui::PopItemWidth();
							}

							ImGui::TextCenter("Text Font");
							//	extern std::vector< std::pair<ImFont*, std::string>> StoryboardFonts;
							char FontSelected[MAX_PATH];
							strcpy(FontSelected, Storyboard.Nodes[nodeid].widget_font[iCurrentSelectedWidget]);

							ImGui::PushItemWidth(-10);
							WidgetSelectUsedFont(nodeid, iCurrentSelectedWidget);
							ImGui::SetWindowFontScale(0.75);

							if (ImGui::BeginCombo("##TextFontStoryboard", FontSelected)) // The second parameter is the label previewed before opening the combo.
							{
								bool bIsSelected = false;
								if (stricmp(FontSelected, "Default Font") == NULL) bIsSelected = true;
								ImGui::PushFont(customfontlarge);  //defaultfont
								if (ImGui::Selectable("Default Font", bIsSelected))
								{
									strcpy(Storyboard.Nodes[nodeid].widget_font[iCurrentSelectedWidget], "Default Font");
								}
								ImGui::PopFont();
								for (int i = 0; i < StoryboardFonts.size(); i++)
								{
									bool bIsSelected = false;
									if (stricmp(StoryboardFonts[i].second.c_str(), FontSelected) == NULL) bIsSelected = true;

									ImGui::PushFont(StoryboardFonts[i].first);  //defaultfont
									if (ImGui::Selectable(StoryboardFonts[i].second.c_str(), bIsSelected))
									{
										strcpy(Storyboard.Nodes[nodeid].widget_font[iCurrentSelectedWidget], StoryboardFonts[i].second.c_str());
									}
									//ImGui::PushFont(customfont);  //defaultfont
									ImGui::PopFont();
								}
								ImGui::EndCombo();
								//ImGui::PopFont();
							}
							ImGui::PopItemWidth();

							ImGui::SetWindowFontScale(1.0);
							ImGui::PopFont();

							ImGui::TextCenter("Text Color");
							bool open_popup = ImGui::ColorButton("##StoryboardWidgetTextColor", Storyboard.Nodes[nodeid].widget_font_color[iCurrentSelectedWidget], 0, ImVec2(w - 20.0, 0));
							if (open_popup)
								ImGui::OpenPopup("##StoryboardWidgetTextColor");
							if (ImGui::BeginPopup("##StoryboardWidgetTextColor", ImGuiWindowFlags_NoMove))
							{
								if (ImGui::ColorPicker4("##StoryboardPickerTextColor", (float*)&Storyboard.Nodes[nodeid].widget_font_color[iCurrentSelectedWidget], ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoSmallPreview))
								{
									//
								}
								ImGui::EndPopup();
							}
							//Add pencil
							ID3D11ShaderResourceView* lpTexture = GetImagePointerView(TOOL_PENCIL);
							if (lpTexture)
							{
								ImVec2 vDrawPos = { ImGui::GetCursorScreenPos().x + (ImGui::GetContentRegionAvail().x - 30.0f) ,ImGui::GetCursorScreenPos().y - (ImGui::GetFontSize() * 1.5f) - 3.0f };
								window->DrawList->AddImage((ImTextureID)lpTexture, vDrawPos, vDrawPos + ImVec2(16, 16), ImVec2(0, 0), ImVec2(1, 1), ImGui::GetColorU32(ImVec4(1, 1, 1, 1)));
							}

							ImGui::TextCenter("Text Size");
							if (Storyboard.Nodes[nodeid].widget_font_size[iCurrentSelectedWidget] > 3.5) Storyboard.Nodes[nodeid].widget_font_size[iCurrentSelectedWidget] = 3.5;
							ImGui::MaxSliderInputFloat("##WidgetTextSize", &Storyboard.Nodes[nodeid].widget_font_size[iCurrentSelectedWidget], 0.00f, 3.50f, "Set Text Font Size", 1.0, 100);
							if (Storyboard.Nodes[nodeid].widget_font_size[iCurrentSelectedWidget] > 3.5) Storyboard.Nodes[nodeid].widget_font_size[iCurrentSelectedWidget] = 3.5;
							if (Storyboard.Nodes[nodeid].widget_font_size[iCurrentSelectedWidget] <= 0) Storyboard.Nodes[nodeid].widget_font_size[iCurrentSelectedWidget] = 0.0001;
						}
					}

					if (iUserDefinedGlobal == 4) // needed for panel grid row and column
					{
						if (Storyboard.Nodes[nodeid].widget_type[iCurrentSelectedWidget] != STORYBOARD_WIDGET_TEXT
							&& Storyboard.Nodes[nodeid].widget_type[iCurrentSelectedWidget] != STORYBOARD_WIDGET_TEXTAREA)
						{
							LPSTR pTitle = "Text Offset";
							LPSTR pLabelX = "Offset X";
							LPSTR pLabelTipX = "Change Widget Text Offset X";
							LPSTR pLabelY = "Offset Y";
							LPSTR pLabelTipY = "Change Widget Text Offset Y";
							if (iUserDefinedGlobal == 4)
							{
								pTitle = "Panel Grid Size";
								pLabelX = "Columns";
								pLabelTipX = "Change the total number of columns";
								pLabelY = "Rows";
								pLabelTipY = "Change the total number of rows";
							}
							ImGui::TextCenter(pTitle);

							ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(0.0f, 3.0f));
							ImGui::Text(pLabelX);
							ImGui::SameLine();
							ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(0.0f, -3.0f));
							ImGui::PushItemWidth(w * 0.5 - (ImGui::GetFontSize() * 2.0) - 40 - scrollSizeX);
							if (ImGui::InputFloat("##StoryboardTextOffsetX", &Storyboard.widget_textoffset[nodeid][iCurrentSelectedWidget].x, 0.0f, 0.0f, "%.0f")) //"%.2f"
							{
								//
							}
							if (!pref.iTurnOffEditboxTooltip && ImGui::IsItemHovered()) ImGui::SetTooltip(pLabelTipX);
							ImGui::PopItemWidth();
							ImGui::SameLine();
							ImGui::Text(pLabelY);
							ImGui::SameLine();
							ImGui::PushItemWidth(w * 0.5 - (ImGui::GetFontSize() * 2.0) - 40 - scrollSizeX);
							if (ImGui::InputFloat("##StoryboardTextOffsetY", &Storyboard.widget_textoffset[nodeid][iCurrentSelectedWidget].y, 0.0f, 0.0f, "%.0f")) //"%.2f"
							{
								//
							}
							if (!pref.iTurnOffEditboxTooltip && ImGui::IsItemHovered()) ImGui::SetTooltip(pLabelTipY);
							ImGui::PopItemWidth();
						}
					}
				}

				if (Storyboard.Nodes[nodeid].widget_type[iCurrentSelectedWidget] == STORYBOARD_WIDGET_BUTTON 
				|| Storyboard.Nodes[nodeid].widget_type[iCurrentSelectedWidget] == STORYBOARD_WIDGET_PROGRESS 
				|| Storyboard.Nodes[nodeid].widget_type[iCurrentSelectedWidget] == STORYBOARD_WIDGET_SLIDER
				|| Storyboard.Nodes[nodeid].widget_type[iCurrentSelectedWidget] == STORYBOARD_WIDGET_BAR
				|| Storyboard.Nodes[nodeid].widget_type[iCurrentSelectedWidget] == STORYBOARD_WIDGET_RADIOTYPE
				|| Storyboard.Nodes[nodeid].widget_type[iCurrentSelectedWidget] == STORYBOARD_WIDGET_TICKBOX)
				{
					ImGui::TextCenter(cstr(sButtonText+cstr(" Size")).Get());
					float fTmp = Storyboard.Nodes[nodeid].widget_size[iCurrentSelectedWidget].x * 100.0f;
					//if (fTmp > 4.0) fTmp = 4.0;
					//if (fTmp <= 0.10) fTmp = 0.1000;
					if (ImGui::MaxSliderInputFloat("##WidgetButtonSize", &fTmp, 0, 100, "Set the size of this widget as a percentage of original size", 0, 100))
					{
						if (fTmp < 1) fTmp = 1;
						Storyboard.Nodes[nodeid].widget_size[iCurrentSelectedWidget].x = fTmp / 100.0f;
						Storyboard.Nodes[nodeid].widget_size[iCurrentSelectedWidget].y = fTmp / 100.0f;
					}

					int iButtons = 3;
					if (Storyboard.Nodes[nodeid].widget_type[iCurrentSelectedWidget] == STORYBOARD_WIDGET_PROGRESS 
						|| Storyboard.Nodes[nodeid].widget_type[iCurrentSelectedWidget] == STORYBOARD_WIDGET_SLIDER
						|| Storyboard.Nodes[nodeid].widget_type[iCurrentSelectedWidget] == STORYBOARD_WIDGET_BAR)
						iButtons = 2;

					//Button Image Regular
					for (int i = 0; i < iButtons; i++) //PE: Selected not really used yet, to enable it just set < 3 :)
					{
						if (i == 0) ImGui::TextCenter(cstr(sButtonText + cstr(" Image Regular")).Get());
						if (i == 1) ImGui::TextCenter(cstr(sButtonText + cstr(" Image Highlighted")).Get());
						if (i == 2) ImGui::TextCenter(cstr(sButtonText + cstr(" Image Selected")).Get());

						cstr UniqueRegularButtonSelect = "##StoryboardUniqueRegularButtonSelect";
						if (i == 1) UniqueRegularButtonSelect = "##StoryboardUniqueHighligtedButtonSelect";
						if (i == 2) UniqueRegularButtonSelect = "##StoryboardUniqueSelectedButtonSelect";

						if (iSelectedLibraryStingReturnID == window->GetID(UniqueRegularButtonSelect.Get()))
						{
							if (i == 0) strcpy(Storyboard.Nodes[nodeid].widget_normal_thumb[iCurrentSelectedWidget], sSelectedLibrarySting.Get());
							if (i == 1) strcpy(Storyboard.Nodes[nodeid].widget_highlight_thumb[iCurrentSelectedWidget], sSelectedLibrarySting.Get());
							if (i == 2) strcpy(Storyboard.Nodes[nodeid].widget_selected_thumb[iCurrentSelectedWidget], sSelectedLibrarySting.Get());
							iUpdateWidgetThumbNode = nodeid;
							iUpdateWidgetThumbButton = iCurrentSelectedWidget;
							sSelectedLibrarySting = "";
							iSelectedLibraryStingReturnID = -1; //disable.
							iSkipWidgetSelectionForFrames = 10; //No widget selection for 10 frames.
						}
						int iTextureID = Storyboard.Nodes[nodeid].widget_normal_thumb_id[iCurrentSelectedWidget];
						if (i == 1) iTextureID = Storyboard.Nodes[nodeid].widget_highlight_thumb_id[iCurrentSelectedWidget];
						if (i == 2) iTextureID = Storyboard.Nodes[nodeid].widget_selected_thumb_id[iCurrentSelectedWidget];

						ImVec2 ImageSize = ImVec2(w - 20.0 - ImGui::GetCurrentWindow()->ScrollbarSizes.x, ImGui::GetFontSize());
						ID3D11ShaderResourceView* lpTexture = GetImagePointerView(iTextureID);
						if (lpTexture)
						{
							float img_w = ImageWidth(iTextureID);
							float img_h = ImageHeight(iTextureID);
							ImageSize.y = img_h * (ImageSize.x / img_w);
						}
						ImVec2 vImagePos = ImGui::GetCursorPos();

						ImGui::Dummy(ImageSize);
						ImVec4 color = ImVec4(1.0, 1.0, 1.0, 1.0);
						ImVec4 back_color = ImVec4(0.2, 0.2, 0.2, 0.75);
						if (ImGui::IsItemHovered())
						{
							color.w = 0.75;
							if (!bWidgetMouseDraggin && ImGui::IsMouseReleased(0))
							{
								//Choose Button image
								sStartLibrarySearchString = "Buttons";
								bExternal_Entities_Window = true;
								iDisplayLibraryType = 2; //Image
								iLibraryStingReturnToID = window->GetID(UniqueRegularButtonSelect.Get());
							}
						}
						ImVec2 img_pos = ImGui::GetWindowPos() + vImagePos;
						img_pos.y -= ImGui::GetScrollY();

						window->DrawList->AddRectFilled(img_pos, img_pos + ImageSize, ImGui::GetColorU32(back_color));
						if (lpTexture)
						{
							window->DrawList->AddImage((ImTextureID)lpTexture, img_pos, img_pos + ImageSize, ImVec2(0, 0), ImVec2(1, 1), ImGui::GetColorU32(color));
						}
						else
						{
							window->DrawList->AddRectFilled(img_pos, img_pos + ImageSize, ImGui::GetColorU32(color));
						}
						lpTexture = GetImagePointerView(TOOL_PENCIL); //Add pencil
						if (lpTexture)
						{
							ImVec2 vDrawPos = { ImGui::GetCursorScreenPos().x + (ImGui::GetContentRegionAvail().x - 30.0f) ,ImGui::GetCursorScreenPos().y - ImageSize.y - 3.0f };
							window->DrawList->AddImage((ImTextureID)lpTexture, vDrawPos, vDrawPos + ImVec2(16, 16), ImVec2(0, 0), ImVec2(1, 1), ImGui::GetColorU32(ImVec4(1, 1, 1, 1)));
						}
					}
		
					if (Storyboard.Nodes[nodeid].widget_type[iCurrentSelectedWidget] != STORYBOARD_WIDGET_PROGRESS 
					&& Storyboard.Nodes[nodeid].widget_type[iCurrentSelectedWidget] != STORYBOARD_WIDGET_SLIDER
					&& Storyboard.Nodes[nodeid].widget_type[iCurrentSelectedWidget] != STORYBOARD_WIDGET_BAR)
					{
						cstr cursound = Storyboard.Nodes[nodeid].widget_click_sound[iCurrentSelectedWidget];				
						cstr butsound = imgui_setpropertyfile2_v2(0, cursound.Get(), cstr(sButtonText + cstr(" Sound")).Get(), cstr(cstr("Select ")+sButtonText+cstr(" Image Regular")).Get(), "audiobank\\", false);
						if (butsound != cursound)
						{
							strcpy(Storyboard.Nodes[nodeid].widget_click_sound[iCurrentSelectedWidget], butsound.Get());
						}

						ImGui::TextCenter("Action");
						char ActionSelected[255];

						int iCurAction = Storyboard.Nodes[nodeid].widget_action[iCurrentSelectedWidget];
						if (iCurAction == STORYBOARD_ACTIONS_NONE) strcpy(ActionSelected, "None");
						if (iCurAction == STORYBOARD_ACTIONS_STARTGAME) strcpy(ActionSelected, "Start Game");
						if (iCurAction == STORYBOARD_ACTIONS_EXITGAME) strcpy(ActionSelected, "Exit Game");
						if (iCurAction == STORYBOARD_ACTIONS_GOTOSCREEN) strcpy(ActionSelected, "Go To Another Screen");
						if (iCurAction == STORYBOARD_ACTIONS_GOTOLEVEL) strcpy(ActionSelected, "Go To Another Level");
						if (iCurAction == STORYBOARD_ACTIONS_CONTINUE) strcpy(ActionSelected, "Continue Game");
						if (iCurAction == STORYBOARD_ACTIONS_BACK) strcpy(ActionSelected, "Close Screen");
						if (iCurAction == STORYBOARD_ACTIONS_LEAVEGAME) strcpy(ActionSelected, "Leave Game");
						if (iCurAction == STORYBOARD_ACTIONS_RESUMEGAME) strcpy(ActionSelected, "Resume Game");
						if (iCurAction == STORYBOARD_ACTIONS_RETURNVALUETOLUA) strcpy(ActionSelected, "Return Button ID to Lua");
						if (iCurAction == STORYBOARD_ACTIONS_GOTOSCREENHUD2) strcpy(ActionSelected, "Go To HUD Screen 2");
						if (iCurAction == STORYBOARD_ACTIONS_GOTOSCREENHUD3) strcpy(ActionSelected, "Go To HUD Screen 3");
						if (iCurAction == STORYBOARD_ACTIONS_GOTOSCREENHUD4) strcpy(ActionSelected, "Go To HUD Screen 4");
						if (iCurAction == STORYBOARD_ACTIONS_GOTOSCREENHUD5) strcpy(ActionSelected, "Go To HUD Screen 5");
						if (iCurAction == STORYBOARD_ACTIONS_GOTOSCREENHUD6) strcpy(ActionSelected, "Go To HUD Screen 6");
						if (iCurAction == STORYBOARD_ACTIONS_GOTOSCREENHUD7) strcpy(ActionSelected, "Go To HUD Screen 7");
						if (iCurAction == STORYBOARD_ACTIONS_GOTOSCREENHUD8) strcpy(ActionSelected, "Go To HUD Screen 8");
						if (iCurAction == STORYBOARD_ACTIONS_GOTOSCREENHUD9) strcpy(ActionSelected, "Go To HUD Screen 9");
						if (iCurAction == STORYBOARD_ACTIONS_GOTOSCREENHUD10) strcpy(ActionSelected, "Go To HUD Screen 10");
						if (iCurAction == STORYBOARD_ACTIONS_GOTOSCREENHUD11) strcpy(ActionSelected, "Go To HUD Screen 11");
						if (iCurAction == STORYBOARD_ACTIONS_GOTOSCREENHUD12) strcpy(ActionSelected, "Go To HUD Screen 12");
						if (iCurAction == STORYBOARD_ACTIONS_GOTOSCREENHUD13) strcpy(ActionSelected, "Go To HUD Screen 13");
						if (iCurAction == STORYBOARD_ACTIONS_GOTOSCREENHUD14) strcpy(ActionSelected, "Go To HUD Screen 14");
						if (iCurAction == STORYBOARD_ACTIONS_GOTOSCREENHUD15) strcpy(ActionSelected, "Go To HUD Screen 15");
						if (iCurAction == STORYBOARD_ACTIONS_GOTOSCREENHUD16) strcpy(ActionSelected, "Go To HUD Screen 16");
						if (iCurAction == STORYBOARD_ACTIONS_GOTOSCREENHUD17) strcpy(ActionSelected, "Go To HUD Screen 17");
						if (iCurAction == STORYBOARD_ACTIONS_GOTOSCREENHUD18) strcpy(ActionSelected, "Go To HUD Screen 18");
						if (iCurAction == STORYBOARD_ACTIONS_GOTOSCREENHUD19) strcpy(ActionSelected, "Go To HUD Screen 19");
						if (iCurAction == STORYBOARD_ACTIONS_GOTOSCREENHUD20) strcpy(ActionSelected, "Go To HUD Screen 20");
						if (iCurAction == STORYBOARD_ACTIONS_GOTOSCREENHUD21) strcpy(ActionSelected, "Go To HUD Screen 21");
						if (iCurAction == STORYBOARD_ACTIONS_GOTOSCREENHUD22) strcpy(ActionSelected, "Go To HUD Screen 22");
						if (iCurAction == STORYBOARD_ACTIONS_GOTOSCREENHUD23) strcpy(ActionSelected, "Go To HUD Screen 23");
						if (iCurAction == STORYBOARD_ACTIONS_GOTOSCREENHUD24) strcpy(ActionSelected, "Go To HUD Screen 24");
						if (iCurAction == STORYBOARD_ACTIONS_GOTOSCREENHUD25) strcpy(ActionSelected, "Go To HUD Screen 25");
						if (iCurAction == STORYBOARD_ACTIONS_GOTOSCREENHUD26) strcpy(ActionSelected, "Go To HUD Screen 26");
						if (iCurAction == STORYBOARD_ACTIONS_GOTOSCREENHUD27) strcpy(ActionSelected, "Go To HUD Screen 27");
						if (iCurAction == STORYBOARD_ACTIONS_GOTOSCREENHUD28) strcpy(ActionSelected, "Go To HUD Screen 28");
						if (iCurAction == STORYBOARD_ACTIONS_GOTOSCREENHUD29) strcpy(ActionSelected, "Go To HUD Screen 29");
						if (iCurAction == STORYBOARD_ACTIONS_GOTOSCREENHUD30) strcpy(ActionSelected, "Go To HUD Screen 30");
						if (iCurAction == STORYBOARD_ACTIONS_GOTOSCREENHUD31) strcpy(ActionSelected, "Go To HUD Screen 31");
						if (iCurAction == STORYBOARD_ACTIONS_GOTOSCREENHUD32) strcpy(ActionSelected, "Go To HUD Screen 32");

						const char* actions_names[] = { "None", "Start Game", "Exit Game", "Go To Another Screen", "Go To Another Level", "Continue Game", "Close Screen", "Leave Game","Resume Game",
							"Return Button ID to Lua", 
							"Go To HUD Screen 2", "Go To HUD Screen 3", "Go To HUD Screen 4", "Go To HUD Screen 5",
							"Go To HUD Screen 6", "Go To HUD Screen 7", "Go To HUD Screen 8", "Go To HUD Screen 9", "Go To HUD Screen 10",
							"Go To HUD Screen 11", "Go To HUD Screen 12", "Go To HUD Screen 13", "Go To HUD Screen 14", "Go To HUD Screen 15",
							"Go To HUD Screen 16", "Go To HUD Screen 17", "Go To HUD Screen 18", "Go To HUD Screen 19", "Go To HUD Screen 20",
							"Go To HUD Screen 21", "Go To HUD Screen 22", "Go To HUD Screen 23", "Go To HUD Screen 24", "Go To HUD Screen 25",
							"Go To HUD Screen 26", "Go To HUD Screen 27", "Go To HUD Screen 28", "Go To HUD Screen 29", "Go To HUD Screen 30",
							"Go To HUD Screen 31", "Go To HUD Screen 32"
						};

						ImGui::PushItemWidth(-10);
						if (ImGui::BeginCombo("##StoryboardAction", ActionSelected)) // The second parameter is the label previewed before opening the combo.
						{
							for (int i = 0; i < IM_ARRAYSIZE(actions_names); i++)
							{
								bool bIsSelected = false;
								if (strcmp(actions_names[i], ActionSelected) == NULL) bIsSelected = true;
								int flag = 0;// ImGuiSelectableFlags_Disabled;
								if (bIsSelected) flag = 0;
								if (ImGui::Selectable(actions_names[i], bIsSelected, flag))
								{
									if (Storyboard.Nodes[nodeid].type == STORYBOARD_TYPE_HUD && i >= STORYBOARD_ACTIONS_STARTGAME && i <= STORYBOARD_ACTIONS_RESUMEGAME)
									{
										// HUD screens cannot use the screen-control-actions inside a game level
										strcpy(cTriggerMessage, "You can only use this storyboard action in non-HUD screens!");
										bTriggerMessage = true;
									}
									else
									{
										if (Storyboard.Nodes[nodeid].type != STORYBOARD_TYPE_HUD && (i < STORYBOARD_ACTIONS_STARTGAME || i > STORYBOARD_ACTIONS_RESUMEGAME))
										{
											// non-HUD screens cannot use the HUD-control-actions inside a main storyboard screen
											strcpy(cTriggerMessage, "You can only use this storyboard action in HUD screens!");
											bTriggerMessage = true;
										}
										else
										{
											Storyboard.Nodes[nodeid].widget_action[iCurrentSelectedWidget] = i;
										}
									}
								}
							}
							ImGui::EndCombo();
						}
						ImGui::PopItemWidth();
					}
				}

				if (Storyboard.Nodes[nodeid].widget_type[iCurrentSelectedWidget] == STORYBOARD_WIDGET_IMAGE
				|| Storyboard.Nodes[nodeid].widget_type[iCurrentSelectedWidget] == STORYBOARD_WIDGET_VIDEO)
				{
					LPSTR pLabel = "Image";
					LPSTR pLabelSelect = "Select Image";
					LPSTR pLabelHide = "Hide Image In Game";
					LPSTR pLabelSize = "Image Size";
					LPSTR pWidgetUnique = "##imgsize";
					LPSTR pSizeTip = "Set the size of this image as a percentage of original size";
					LPSTR pWidgetPathToFile = "imagebank\\HUD\\";
					char name[MAX_PATH];
					strcpy (name, Storyboard.Nodes[nodeid].widget_normal_thumb[iCurrentSelectedWidget]);
					bool bWidgetIsVideo = false;
					if (Storyboard.Nodes[nodeid].widget_type[iCurrentSelectedWidget] == STORYBOARD_WIDGET_VIDEO)
					{
						pLabel = "Video";
						pLabelSelect = "Video File";
						pLabelHide = "Hide Video In Game";
						pLabelSize = "Video Size";
						pWidgetUnique = "##vidsize";
						pSizeTip = "Set the size of this video as a percentage of original size";
						pWidgetPathToFile = "videobank\\";
						strcpy (name, Storyboard.Nodes[nodeid].widget_highlight_thumb[iCurrentSelectedWidget]);
						bWidgetIsVideo = true;
					}
					if (g_bSelectedMapImageTypeSpecialHelp == false)
					{
						cstr cNewImageOrVideo = imgui_setpropertyfile2_v2(0, name, pLabel, pLabelSelect, pWidgetPathToFile, false, "");
						if (cNewImageOrVideo.Len() > 0 && stricmp(name, cNewImageOrVideo.Get()) != NULL)
						{
							// Delete old image or video thumb and trigger reload of the newly chosen one
							DeleteImage(Storyboard.Nodes[nodeid].widget_normal_thumb_id[iCurrentSelectedWidget]);
							if (bWidgetIsVideo == true)
							{
								// ensure original video stored for later use
								strcpy(Storyboard.Nodes[nodeid].widget_normal_thumb[iCurrentSelectedWidget], cNewImageOrVideo.Get());
								strcpy(Storyboard.Nodes[nodeid].widget_highlight_thumb[iCurrentSelectedWidget], cNewImageOrVideo.Get());

								// video needs to grab (if any) the video thumbnail (generated in the video library browser)
								char pVideoThumb[MAX_PATH];
								strcpy(pVideoThumb, cNewImageOrVideo.Get());
								if (strnicmp(pVideoThumb, "videobank\\", strlen("videobank\\")) == NULL)
								{
									strcpy(pVideoThumb, cNewImageOrVideo.Get() + strlen("videobank\\"));
									int thumb_x = 512; int thumb_y = 288;
									CreateBackBufferCacheName(pVideoThumb, thumb_x, thumb_y);
									char pThumbPath[MAX_PATH];
									strcpy (pThumbPath, BackBufferCacheName.Get());
									LPSTR pRelativePart = BackBufferCacheName.Get() + strlen(g.fpscrootdir_s.Get()) + strlen("\\Files\\");
									if (pThumbPath[1] == ':')
									{
										// if absolute, replace with relative path
										strcpy (pVideoThumb, pRelativePart);
									}
									if (FileExist(pVideoThumb)==0)
									{
										// if video in root, add underscore to find correct thumb (poss. bug)
										strcpy(pVideoThumb, "thumbbank\\_");
										strcat (pVideoThumb, pRelativePart + strlen("thumbbank\\"));
									}
									if (FileExist(pVideoThumb))
									{
										strcpy(Storyboard.Nodes[nodeid].widget_normal_thumb[iCurrentSelectedWidget], pVideoThumb);
									}
								}
							}
							else
							{
								// can use image file directly
								strcpy(Storyboard.Nodes[nodeid].widget_normal_thumb[iCurrentSelectedWidget], cNewImageOrVideo.Get());
							}
							iUpdateWidgetThumbNode = iCurrentSelectedWidget;
							iUpdateWidgetThumbButton = iCurrentSelectedWidget;
						}
					}
					bool bHidingImageInGame = false;
					if (bWidgetIsVideo == false)
					{
						if (Storyboard.widget_ingamehidden[nodeid][iCurrentSelectedWidget] == 1) bHidingImageInGame = true;
						if (ImGui::Checkbox(pLabelHide, &bHidingImageInGame))
						{
							if (bHidingImageInGame == true)
								Storyboard.widget_ingamehidden[nodeid][iCurrentSelectedWidget] = 1;
							else
								Storyboard.widget_ingamehidden[nodeid][iCurrentSelectedWidget] = 0;
						}
					}
					if (bWidgetIsVideo == false)
					{
						// image has color
						ImGui::TextCenter("Image Color");
						bool open_popup = ImGui::ColorButton("##StoryboardWidgetImageColor", Storyboard.widget_colors[nodeid][iCurrentSelectedWidget], 0, ImVec2(w - 20.0, 0));
						if (open_popup)
							ImGui::OpenPopup("##StoryboardWidgetImageColor");
						if (ImGui::BeginPopup("##StoryboardWidgetImageColor", ImGuiWindowFlags_NoMove))
						{
							if (ImGui::ColorPicker4("##StoryboardPickerImageColor", (float*)&Storyboard.widget_colors[nodeid][iCurrentSelectedWidget], ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoSmallPreview))
							{
							}
							ImGui::EndPopup();
						}
					}
					else
					{
						// video has playback controls
						bool g_bVideoLooping = false;
						if (Storyboard.Nodes[nodeid].widget_font_size[iCurrentSelectedWidget] != 0) g_bVideoLooping = true;
						if (ImGui::Checkbox("Loop Video Animation", &g_bVideoLooping))
						{
							Storyboard.Nodes[nodeid].widget_font_size[iCurrentSelectedWidget] = (int)g_bVideoLooping;
						}
					}

					ImGui::TextCenter(pLabelSize);
					float fTmp = Storyboard.Nodes[nodeid].widget_size[iCurrentSelectedWidget].x * 100.0f;
					if( ImGui::MaxSliderInputFloat(pWidgetUnique, &fTmp, 0, 100, pSizeTip, 0, 100))
					{
						if (fTmp < 1) fTmp = 1;
						Storyboard.Nodes[nodeid].widget_size[iCurrentSelectedWidget].y = fTmp / 100.0f;
						Storyboard.Nodes[nodeid].widget_size[iCurrentSelectedWidget].x = fTmp / 100.0f;
					}
#ifdef EMULATERESOLUTION
					bool bTmp = Storyboard.Nodes[nodeid].universal_resolution[iCurrentSelectedWidget];
					if (ImGui::Checkbox("Universal Scaling", &bTmp))
					{
						Storyboard.Nodes[nodeid].universal_resolution[iCurrentSelectedWidget] = bTmp;
					}
					if (ImGui::IsItemHovered()) ImGui::SetTooltip("Will always display using 1:1 pixel ratio, based on screen y resolution.");
#endif
				}

				// Display and allow editing of readouts
				if (strlen(Storyboard.widget_readout[nodeid][iCurrentSelectedWidget]) > 0)
				{
					ImGui::TextCenter("Readout");
					ImGui::PushItemWidth(-10);
					std::string readout = Storyboard.widget_readout[nodeid][iCurrentSelectedWidget];
					if (ImGui::BeginCombo("##readoutcombo", readout.c_str()))
					{
						ImGui::Selectable(readout.c_str(), true);
						ImGui::EndCombo();
					}
					ImGui::PopItemWidth();

					// moved enry of 'widget_label' down here to match readout selection
					if (iUserDefinedGlobal > 0)
					{
						if (widgetType != STORYBOARD_WIDGET_PROGRESS )
						{
							// label
							if( iUserDefinedGlobal==2 )
								ImGui::TextCenter("User Defined Global Names");
							else
								ImGui::TextCenter("User Defined Global Name");

							// one or two (single or pair handling)
							char storeFirstEntry[MAX_PATH];
							strcpy(storeFirstEntry, "");
							strcpy(storeFirstEntry, Storyboard.Nodes[nodeid].widget_label[iCurrentSelectedWidget]);
							char storeSecondEntry[MAX_PATH];
							strcpy(storeSecondEntry, "");
							char* pDelimit = strstr(storeFirstEntry, ";");
							if (pDelimit)
							{
								strcpy(storeSecondEntry, pDelimit + 1);
								*pDelimit = 0;
							}

							// field (use dropdown for common values)
							ImGui::PushItemWidth(-10);
							bool bShowCustomValueBox = false;
							std::string readout = Storyboard.widget_readout[nodeid][iCurrentSelectedWidget];
							if (stricmp(readout.c_str(), "User Defined Global") == NULL)
							{
								bool bKnownValue = false;
								const char* pSelectedComboItem = "";
								for (int n = 0; n <= g_gameGlobalListNodeId.size(); n++)
								{
									LPSTR pThisGlobItem = "";
									if (n < g_gameGlobalListNodeId.size())
									{
										int i = g_gameGlobalListIndex[n];
										int allhudscreensnodeid = g_gameGlobalListNodeId[n];
										pThisGlobItem = Storyboard.Nodes[allhudscreensnodeid].widget_label[i];
										if (stricmp(pThisGlobItem, storeFirstEntry) == NULL)
										{
											pSelectedComboItem = pThisGlobItem;
											bKnownValue = true;
											break;
										}
									}
								}
								if ( strlen(pSelectedComboItem) == 0 )
								{
									pSelectedComboItem = "Custom Value";
									bShowCustomValueBox = true;
								}
								if (ImGui::BeginCombo("##comboRPGGlobalKinds", pSelectedComboItem))
								{
									for (int n = 0; n <= g_gameGlobalListNodeId.size(); n++)
									{
										LPSTR pThisGlobItem = "";
										if (n < g_gameGlobalListNodeId.size())
										{
											int i = g_gameGlobalListIndex[n];
											int allhudscreensnodeid = g_gameGlobalListNodeId[n];
											pThisGlobItem = Storyboard.Nodes[allhudscreensnodeid].widget_label[i];
										}
										else
										{
											pThisGlobItem = "Custom Value";
										}
										bool bIsSelected = false;
										if (stricmp(pThisGlobItem, storeFirstEntry) == NULL) bIsSelected = true;
										if (ImGui::Selectable(pThisGlobItem, bIsSelected))
										{
											if (n < g_gameGlobalListNodeId.size())
											{
												strcpy(storeFirstEntry, pThisGlobItem);
											}
											else
											{
												if (bKnownValue == true) strcpy(storeFirstEntry, "");
											}
											g_bRefreshGlobalList = true;
											break;
										}
									}
									ImGui::EndCombo();
								}
								if (ImGui::IsItemHovered()) ImGui::SetTooltip("Specify a user defined global value");
							}
							if (strcmp(readout.c_str(), "User Defined Global Text") == NULL)
							{
								const char* rpginventorykinds[] = { "prompt:main", "selected:title", "selected:description", "selected:cost", "selected:value", "selected:ingredients", "craft:title", "craft:ingredients", "Custom Value" };
								int rpginventorykinds_selection = -1;
								for (int n = 0; n <= IM_ARRAYSIZE(rpginventorykinds) - 2; n++)
								{
									if (stricmp(rpginventorykinds[n], storeFirstEntry) == NULL)
									{
										rpginventorykinds_selection = n;
										break;
									}
								}
								int ishowselectionincombo = rpginventorykinds_selection;
								if (ishowselectionincombo == -1) ishowselectionincombo = IM_ARRAYSIZE(rpginventorykinds) - 1;
								if (ImGui::Combo("##comboRPGInventoryKinds", &ishowselectionincombo, rpginventorykinds, IM_ARRAYSIZE(rpginventorykinds)))
								{
									if (ishowselectionincombo <= IM_ARRAYSIZE(rpginventorykinds) - 2)
									{
										strcpy (storeFirstEntry, rpginventorykinds[ishowselectionincombo]);
									}
									else
									{
										for (int n = 0; n <= IM_ARRAYSIZE(rpginventorykinds) - 2; n++)
										{
											if (stricmp(rpginventorykinds[n], storeFirstEntry) == NULL)
											{
												strcpy(storeFirstEntry, "");
												break;
											}
										}
										rpginventorykinds_selection = -1;
									}
								}
								if (rpginventorykinds_selection == -1)
								{
									bShowCustomValueBox = true;
								}
								if (ImGui::IsItemHovered()) ImGui::SetTooltip("Specify the user global value used by this text");
							}
							if (strcmp(readout.c_str(), "User Defined Global Image") == NULL)
							{
								const char* rpginventorykinds[] = { "selected:image", "scrollbar:box", "scrollbar:handle", "map:window", "map:image", "map:player", "map:winzone", "map:character", "map:objective", "Custom Value" };
								int rpginventorykinds_selection = -1;
								for (int n = 0; n <= IM_ARRAYSIZE(rpginventorykinds) - 2; n++)
								{
									if (stricmp(rpginventorykinds[n], storeFirstEntry) == NULL)
									{
										rpginventorykinds_selection = n;
										break;
									}
								}
								int ishowselectionincombo = rpginventorykinds_selection;
								if (ishowselectionincombo == -1) ishowselectionincombo = IM_ARRAYSIZE(rpginventorykinds) - 1;
								if (ImGui::Combo("##comboRPGImageKinds", &ishowselectionincombo, rpginventorykinds, IM_ARRAYSIZE(rpginventorykinds)))
								{
									if (ishowselectionincombo <= IM_ARRAYSIZE(rpginventorykinds) - 2)
									{
										strcpy (storeFirstEntry, rpginventorykinds[ishowselectionincombo]);
									}
									else
									{
										for (int n = 0; n <= IM_ARRAYSIZE(rpginventorykinds) - 2; n++)
										{
											if (stricmp(rpginventorykinds[n], storeFirstEntry) == NULL)
											{
												strcpy(storeFirstEntry, "");
												break;
											}
										}
										rpginventorykinds_selection = -1;
									}
								}
								if (rpginventorykinds_selection == -1)
								{
									bShowCustomValueBox = true;
								}
								if (ImGui::IsItemHovered()) ImGui::SetTooltip("Specify a user defined global value used by this image");

								// Extra help in screen editor
								g_bSelectedMapImageTypeSpecialHelp = false;
								if ( stricmp(storeFirstEntry,"map:image")==NULL )
								{
									ImGui::TextWrapped("NOTE: Map Image will be replaced with the Map Snaphot associated with each level of your game project");
									if (ImGui::IsItemHovered()) ImGui::SetTooltip("You can generate a map snapshot by using the dropdown menu for the level in the Storyboard Editor");
									g_bSelectedMapImageTypeSpecialHelp = true;
								}
							}
							if (strcmp(readout.c_str(), "User Defined Global Panel") == NULL)
							{
								const char* rpginventorykinds[] = { "inventory:player", "inventory:hotkeys", "inventory:container", "inventory:craft", "Custom Value" };
								int rpginventorykinds_selection = -1;
								for (int n = 0; n <= IM_ARRAYSIZE(rpginventorykinds) - 2; n++)
								{
									if (stricmp(rpginventorykinds[n], storeFirstEntry) == NULL)
									{
										rpginventorykinds_selection = n;
										break;
									}
								}
								int ishowselectionincombo = rpginventorykinds_selection;
								if (ishowselectionincombo == -1) ishowselectionincombo = IM_ARRAYSIZE(rpginventorykinds) - 1;
								if (ImGui::Combo("##comboRPGInventoryKinds", &ishowselectionincombo, rpginventorykinds, IM_ARRAYSIZE(rpginventorykinds)))
								{
									if (ishowselectionincombo <= IM_ARRAYSIZE(rpginventorykinds) - 2)
									{
										strcpy (storeFirstEntry, rpginventorykinds[ishowselectionincombo]);
									}
									else
									{
										for (int n = 0; n <= IM_ARRAYSIZE(rpginventorykinds) - 2; n++)
										{
											if (stricmp(rpginventorykinds[n], storeFirstEntry) == NULL)
											{
												strcpy(storeFirstEntry, "");
												break;
											}
										}
										rpginventorykinds_selection = -1;
									}
								}
								if (rpginventorykinds_selection==-1)
								{
									bShowCustomValueBox = true;
								}
								if (ImGui::IsItemHovered()) ImGui::SetTooltip("Specify the user global value used by this panel");
							}
							ImGui::PopItemWidth();
							if (strcmp(readout.c_str(), "User Defined Global Statusbar") == NULL)
							{
								bShowCustomValueBox = true;
							}
							if (bShowCustomValueBox == true )
							{
								cstr UniqueTextFiledName = cstr("##WidgetTextStoryboardInput") + cstr(iCurrentSelectedWidget);
								ImGui::PushItemWidth(-10);
								static char tempinputFirstEntry[251];
								static int ilastSelectedWidget = -1;
								if (iCurrentSelectedWidget+(nodeid*1000) != ilastSelectedWidget)
								{
									ilastSelectedWidget = iCurrentSelectedWidget + (nodeid * 1000);
									strcpy(tempinputFirstEntry, storeFirstEntry);
								}
								if (ImGui::InputText(UniqueTextFiledName.Get(), tempinputFirstEntry, 250, ImGuiInputTextFlags_EnterReturnsTrue))
								{
									strcpy(storeFirstEntry, tempinputFirstEntry);
									g_bRefreshGlobalList = true;
								}
								if (ImGui::MaxIsItemFocused()) bImGuiGotFocus = true;
								ImGui::PopItemWidth();
								if (ImGui::IsItemHovered()) ImGui::SetTooltip("Specify a user defined global or one of the presets (Health Remaining, Ammo Remaining, Maximum Ammo, Lives Remaining)");
							}

							// globals pairs are used for things like status bars (current value var and max var)
							if (iUserDefinedGlobal == 2)
							{
								// ask for a second on its own
								cstr UniqueTextFiledNameSecond = cstr("##WidgetTextStoryboardInputSecond") + cstr(iCurrentSelectedWidget);
								ImGui::PushItemWidth(-10);
								if (ImGui::InputText(UniqueTextFiledNameSecond.Get(), storeSecondEntry, 250, ImGuiInputTextFlags_EnterReturnsTrue))
								{
									g_bRefreshGlobalList = true;
								}
								if (ImGui::IsItemHovered()) ImGui::SetTooltip("Specify a second user defined global or one of the presets (Maximum Health, Weapon Reload Quantity, Maximum Clipped Ammo)");
								if (ImGui::MaxIsItemFocused()) bImGuiGotFocus = true;
								ImGui::PopItemWidth();
							}

							// global pairs reconstruct first and second and store for later use
							strcpy(Storyboard.Nodes[nodeid].widget_label[iCurrentSelectedWidget], storeFirstEntry);
							if (strlen(storeSecondEntry) > 0)
							{
								strcat(Storyboard.Nodes[nodeid].widget_label[iCurrentSelectedWidget], ";");
								strcat(Storyboard.Nodes[nodeid].widget_label[iCurrentSelectedWidget], storeSecondEntry);
							}
						}
					}
					else
					{
						//PE: Re enable image file selecting.
						g_bSelectedMapImageTypeSpecialHelp = false;
					}
				}
				else
				{
					//PE: Re enable image file selecting.
					g_bSelectedMapImageTypeSpecialHelp = false;
				}
				ImGui::Indent(-10);
			}

			// from here, initial values can be set for all detected user global variables in this screen (nice place for them)
			cstr sUserGlobalLabel = "User Defined Global Settings";
			if (ImGui::StyleCollapsingHeader(sUserGlobalLabel.Get(), ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::Indent(10);
				// only for HUD screens
				if (g_bRefreshGlobalList == true)
				{
					g_gameGlobalListNodeId.clear();
					g_gameGlobalListIndex.clear();
					g_gameGlobalListValue.clear();
					g_gameGlobalListValueString.clear();

					for (int allhudscreensnodeid = 0; allhudscreensnodeid < STORYBOARD_MAXNODES; allhudscreensnodeid++)
					{
						if (strlen(Storyboard.Nodes[allhudscreensnodeid].lua_name) > 0 && strnicmp(Storyboard.Nodes[allhudscreensnodeid].lua_name, "hud", 3) == NULL)
						{
							for (int i = STORYBOARD_MAXWIDGETS; i >= 0; i--)
							{
								if (Storyboard.Nodes[allhudscreensnodeid].widget_type[i] == STORYBOARD_WIDGET_TEXT)
								{
									std::string readout = Storyboard.widget_readout[allhudscreensnodeid][i];
									if (stricmp(readout.c_str(), "User Defined Global") == NULL
										|| stricmp(readout.c_str(), "User Defined Global Text") == NULL)
									{
										// only add unique ones to game global list
										LPSTR pNewName = Storyboard.Nodes[allhudscreensnodeid].widget_label[i];
										if (!pestrcasestr(pNewName, ":")) //PE: Do not show : rpginventorykinds.
										{
											for (int n = 0; n < g_gameGlobalListNodeId.size(); n++)
											{
												int thisnodeid = g_gameGlobalListNodeId[n];
												int index = g_gameGlobalListIndex[n];
												LPSTR pThisName = Storyboard.Nodes[thisnodeid].widget_label[index];
												if (strcmp(pNewName, pThisName) == NULL)
												{
													// already exists
													pNewName = "";
													break;
												}
											}
											if (strlen(pNewName) > 0)
											{
												g_gameGlobalListNodeId.push_back(allhudscreensnodeid);
												g_gameGlobalListIndex.push_back(i);
												g_gameGlobalListValue.push_back(Storyboard.Nodes[allhudscreensnodeid].widget_initial_value[i]);
												if(stricmp(readout.c_str(), "User Defined Global Text") == NULL)
													g_gameGlobalListValueString.push_back(Storyboard.Nodes[allhudscreensnodeid].widget_click_sound[i]);
												else
													g_gameGlobalListValueString.push_back("");
											}
										}
									}
								}
							}
						}
					}
				}
				bool bChangedAGameGlobal = false;
				for (int n = 0; n < g_gameGlobalListNodeId.size(); n++)
				{
					int allhudscreensnodeid = g_gameGlobalListNodeId[n];
					int i = g_gameGlobalListIndex[n];
					ImGui::TextCenter(Storyboard.Nodes[allhudscreensnodeid].widget_label[i]);
					char pUDGVar[256];
					sprintf(pUDGVar, "##WidgetUDG%d-%d", allhudscreensnodeid, i);
					float fValue = g_gameGlobalListValue[n];
					if (stricmp(Storyboard.widget_readout[allhudscreensnodeid][i], "User Defined Global Text") == NULL)
					{
						char storeEntry[MAX_PATH];
						strcpy(storeEntry, g_gameGlobalListValueString[n].c_str());
						ImGui::PushItemWidth(-10);
						if (ImGui::InputText(pUDGVar, storeEntry, 250, ImGuiInputTextFlags_EnterReturnsTrue))
						{
							g_gameGlobalListValueString[n] = storeEntry;
							strcpy(Storyboard.Nodes[allhudscreensnodeid].widget_click_sound[i], storeEntry);
							bChangedAGameGlobal = true;
						}
						ImGui::PopItemWidth();
					}
					else
					{
						//PE: TEXT g_gameGlobalListValueString.push_back(Storyboard.Nodes[allhudscreensnodeid].widget_click_sound[i]);
						ImGui::MaxSliderInputFloat(pUDGVar, &fValue, 0, 100, "Set Initial Value for this User Defined Global", 0, 100);
						if (fValue != g_gameGlobalListValue[n])
						{
							g_gameGlobalListValue[n] = fValue;
							bChangedAGameGlobal = true;
						}
					}
				}
				if (g_bRefreshGlobalList == true)
				{
					if (bChangedAGameGlobal == true)
					{
						for (int allhudscreensnodeid = 0; allhudscreensnodeid < STORYBOARD_MAXNODES; allhudscreensnodeid++)
						{
							if (strlen(Storyboard.Nodes[allhudscreensnodeid].lua_name) > 0 && strnicmp(Storyboard.Nodes[allhudscreensnodeid].lua_name, "hud", 3) == NULL)
							{
								for (int i = STORYBOARD_MAXWIDGETS; i >= 0; i--)
								{
									if (Storyboard.Nodes[allhudscreensnodeid].widget_type[i] == STORYBOARD_WIDGET_TEXT)
									{
										std::string readout = Storyboard.widget_readout[allhudscreensnodeid][i];
										if (stricmp(readout.c_str(), "User Defined Global") == NULL
											|| stricmp(readout.c_str(), "User Defined Global Text") == NULL)
										{
											// only add unique ones to game global list
											LPSTR pNewName = Storyboard.Nodes[allhudscreensnodeid].widget_label[i];
											if (!pestrcasestr(pNewName, ":")) //PE: Do not show : rpginventorykinds.
											{
												for (int n = 0; n < g_gameGlobalListNodeId.size(); n++)
												{
													int thisnodeid = g_gameGlobalListNodeId[n];
													int index = g_gameGlobalListIndex[n];
													LPSTR pThisName = Storyboard.Nodes[thisnodeid].widget_label[index];
													if (strcmp(pNewName, pThisName) == NULL)
													{
														if(stricmp(readout.c_str(), "User Defined Global Text") == NULL)
														{
															//g_gameGlobalListValueString[n]
															strcpy(Storyboard.Nodes[allhudscreensnodeid].widget_click_sound[i], g_gameGlobalListValueString[n].c_str());
															Storyboard.Nodes[allhudscreensnodeid].widget_initial_value[i] = 0;
														}
														else
															Storyboard.Nodes[allhudscreensnodeid].widget_initial_value[i] = g_gameGlobalListValue[n];
														break;
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
				g_bRefreshGlobalList = false;
				if (bChangedAGameGlobal == true)
				{
					// if modify a global, allow another refresh to place in initial_value
					g_bRefreshGlobalList = true;
				}
				ImGui::Indent(-10);
			}

			if (bReadOnly)
			{
				ImGui::PopItemFlag();
				ImGui::PopStyleVar();
			}

			if (ImGui::GetCurrentWindow()->ScrollbarSizes.x > 0) {
				//Hitting exactly at the botton could cause flicker, so add some additional lines when scrollbar on.
				ImGui::Text("");
				ImGui::Text("");
			}
			//ImGui::EndChild();
		}

		if ((!standalone && !bPreviewScreen) || (bPreviewScreen && !bPrevPreviewScreen))
		{
			ImGui::EndChild(); //"##SERightPanel"
		}

		if (!bPreviewScreen)
			ImGui::Columns(1);

		if (iQuitWindowLoop > 0)
		{
			if (iQuitWindowLoop == 1)
			{
				BackBufferSaveCacheName = "";
				BackBufferObjectID = 0;
				BackBufferImageID = 0;
				bLoopBackBuffer = false;
				BackBufferSnapShotMode = false;
				bSnapShotModeUse2D = false;
				bFullScreenBackbuffer = false;
				//Use Grab to get center of screen.
				if (BitmapExist(99))
				{
					ImRect rGrabMonitorArea = rMonitorArea;

					//PE: Widescreen - now use full backbuffer so everything fits.
					SetGrabImageMode(1);
					LPGGSURFACE	pTmpSurface = g_pGlob->pCurrentBitmapSurface;
					ID3D11Texture2D *pBackBuffer = NULL;
					pBackBuffer = (ID3D11Texture2D *)GetBitmapTexture2D(99);
					g_pGlob->pCurrentBitmapSurface = pBackBuffer;

					if (pBackBuffer)
					{
						GGSURFACE_DESC ddsd;
						pBackBuffer->GetDesc(&ddsd);

						float grabx = rGrabMonitorArea.Max.x - rGrabMonitorArea.Min.x;
						float graby = rGrabMonitorArea.Max.y - rGrabMonitorArea.Min.y;
						//graby -= 10.0;

						//PE: Change to using vMonitorStart its already in pixels.
						float imgcx = vMonitorStart.x + 1.0; //Skip our padding line.
						float imgcy = vMonitorStart.y + 1.0; //Skip our padding line.

						if (graby > ddsd.Height)
							graby = ddsd.Height;
						if (grabx > ddsd.Width)
							grabx = ddsd.Width;

						if (imgcx + grabx > ddsd.Width) imgcx = ddsd.Width - grabx - 1.0; // check is >= so need -1.
						if (imgcy < 0) imgcy = 0;
						if (imgcx < 0) imgcx = 0;

						bool bValid = true;
						if (imgcx >= imgcx + grabx || imgcy >= imgcy + graby)
						{
							//PE: pBackBuffer->GetDesc failed ?
							if (ddsd.Width >= 1900 && ddsd.Height >= 1000)
							{
								imgcx = 30.0;
								imgcy = 98.0;
								grabx = 1567.0;
								graby = 872.0;
							}
							else
								bValid = false;
						}

						if (bValid)
						{
							//PE: We need a unique id for this STORYBOARD_THUMBS+400
							GrabImage(STORYBOARD_THUMBS + 400, imgcx, imgcy, imgcx + grabx, imgcy + graby, 3);
							SetGrabImageMode(0);
						}
						g_pGlob->pCurrentBitmapSurface = pTmpSurface;

						GG_SetWritablesToRoot(true);
						if (FileExist("thumbbank\\lastnewlevel.jpg")) DeleteAFile("thumbbank\\lastnewlevel.jpg");
						GG_SetWritablesToRoot(false);

						if (bValid)
						{
							if (ImageExist(STORYBOARD_THUMBS + 400))
							{
								char destination[MAX_PATH];
								strcpy(destination, "thumbbank\\lastnewlevel.jpg");
								GG_SetWritablesToRoot(true);
								GG_GetRealPath(destination, 1);
								GG_SetWritablesToRoot(false);
								//Need a no alpha save.
								extern bool g_bDontUseImageAlpha;
								g_bDontUseImageAlpha = true;
								SaveImage(destination, STORYBOARD_THUMBS + 400);
								g_bDontUseImageAlpha = false;
								DeleteImage(STORYBOARD_THUMBS + 400);
							}
						}
					}
					else
					{
						// no backbuffer in DX12 mode, restore state
						SetGrabImageMode(0);
						g_pGlob->pCurrentBitmapSurface = pTmpSurface;
					}
				}
				else
				{
					// DX12 mode: bitmap 99 doesn't exist, capture backbuffer directly
					extern ImRect g_rStealMonitorArea;
					int captureX = (int)g_rStealMonitorArea.Min.x;
					int captureY = (int)g_rStealMonitorArea.Min.y;
					int captureW = (int)(g_rStealMonitorArea.Max.x - g_rStealMonitorArea.Min.x);
					int captureH = (int)(g_rStealMonitorArea.Max.y - g_rStealMonitorArea.Min.y);

					GG_SetWritablesToRoot(true);
					if (FileExist("thumbbank\\lastnewlevel.jpg")) DeleteAFile("thumbbank\\lastnewlevel.jpg");
					GG_SetWritablesToRoot(false);

					if (captureW > 0 && captureH > 0)
					{
						char destination[MAX_PATH];
						strcpy(destination, "thumbbank\\lastnewlevel.jpg");
						GG_SetWritablesToRoot(true);
						GG_GetRealPath(destination, 1);
						GG_SetWritablesToRoot(false);
						WickedCall_CaptureBackbufferRegionToJPG(captureX, captureY, captureW, captureH, destination);
					}
				}

				//Quit and get new thumb to correct node id.
				iWaitFor2DEditor = 5;
				iWaitFor2DEditorNode = nodeid;
				bScreen_Editor_Window = false;
				if (BitmapExist(99))
				{
					DeleteBitmapEx(99);
				}

				//PE: Change output link based on changes.
				setup_output_links(nodeid);
			}
			else
			{
				//Grab a thumb. need some frames.
				BackBufferIsGroup = false;
				BackBufferEntityID = 0;
				BackBufferObjectID = 0;
				BackBufferImageID = g.importermenuimageoffset + 50;
				BackBufferSizeX = ImGui::GetMainViewport()->Size.x;
				BackBufferSizeY = ImGui::GetMainViewport()->Size.y;
				BackBufferZoom = 1.0f;
				BackBufferCamLeft = 0.0f;
				BackBufferCamUp = 0.0f;
				bRotateBackBuffer = false;
				bBackBufferAnimated = false;
				BackBufferSaveCacheName = ""; //No saving on tooltip images
				fLastRubberBandX1 = fLastRubberBandX2 = fLastRubberBandY1 = fLastRubberBandY2 = 0.0f;//Dont fit snapshot to rubber band.
				//Control camera snap shot.
				bLoopBackBuffer = true;
				bSnapShotModeUseCamera = true;
				BackBufferSnapShotMode = true;
				bSnapShotModeUse2D = true;
				bFullScreenBackbuffer = true;
			}

			if (iQuitWindowLoop == 4)
			{
				if (BitmapExist(99))
				{
					DeleteBitmapEx(99);
				}
			}

			iQuitWindowLoop--;
		}

		if (standalone)
		{
			ImGui::PopStyleColor();
			ImGui::PopStyleVar();   // pop window padding
			ImGui::PopStyleVar();   // pop frame padding
			ImGui::PopStyleVar();   // pop item padding
			ImGui::PopStyleVar();   // windows border
			ImGui::End();
		}

	}
	else
	{
		//Something wrong, close down window.
		bScreen_Editor_Window = false;
	}

	// Delete currently selected widget
	if (iCurrentSelectedWidget>=0 && bScreen_Editor_Window && !bPreviewScreen && !standalone)
	{
		extern ImRect g_rStealMonitorArea;
		ImVec2 mPos = ImGui::GetMousePos();
		if (mPos.x > g_rStealMonitorArea.Min.x && mPos.x < g_rStealMonitorArea.Max.x && mPos.y > g_rStealMonitorArea.Min.y && mPos.y < g_rStealMonitorArea.Max.y)
		{
			// only if within monitor area
			static bool bBlockUntilKeyReleased = false;
			constexpr int deleteKey = 46;
			if (ImGui::GetIO().KeysDown[deleteKey] && !bBlockUntilKeyReleased)
			{
				RemoveWidgetFromScreen(nodeid, iCurrentSelectedWidget);
				bBlockUntilKeyReleased = true;
			}
			else if (ImGui::GetIO().KeysDown[deleteKey] == false)
			{
				bBlockUntilKeyReleased = false;
			}
		}

		static bool bRMBMenu = false;
		if (ImGui::IsMouseReleased(0))
		{
			if (bRMBMenu)
			{
				ImGui::CloseCurrentPopup();
				bRMBMenu = false;
			}
		}
		if (ImGui::IsMouseClicked(1))
		{
			bRMBMenu = true;
			ImGui::OpenPopup("##rmbmenu");
		}

		static int iRightClickItem = -1;
		if (ImGui::BeginPopup("##rmbmenu", ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings))
		{
			//PE: When right click and popup open, widget is not hovered and iCurrentSelectedWidget is set to 0.
			if (iRightClickItem < 0)
				iRightClickItem = iCurrentSelectedWidget;
			if (ImGui::Selectable("Delete##rmbdelete"))
			{
				RemoveWidgetFromScreen(nodeid, iRightClickItem);
			}
			ImGui::Separator();
			if (ImGui::Selectable("Send to Back##rmbtoback"))
			{
				SendWidgetToBack(nodeid, iRightClickItem);
			}
			if (ImGui::Selectable("Send to Front##rmbtofront"))
			{
				SendWidgetToFront(nodeid, iRightClickItem);
			}
			ImGui::EndPopup();
		}
		else
		{
			iRightClickItem = -1;
		}
	}

	// Modal window, waits for user to press a key. The key that they press will be used to toggle the visibility of the current screen
	if (bScreenToggleKeyWindow)
	{
		bImGuiGotFocus = true;
		ImGui::SetNextWindowPosCenter(ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(512, 256));
		ImGui::OpenPopup("##screenkeytogglepopup");
		ImGui::BeginPopupModal("##screenkeytogglepopup", 0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse);
		ImGui::Text("");
		ImGui::Text("");
		ImGui::Text("");
		ImGui::Text("");
		ImGui::TextCenter("Press the key that you would like to make this screen appear in-game.");
		ImGui::Text("");
		ImGui::TextCenter("Press SPACEBAR to reset selection back to NONE.");
		ImGuiStyle& style = ImGui::GetStyle();
		ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 1.0f, 1.0f, 0.54f));
		t.inputsys.kscancode = ScanCode();
		if (t.inputsys.kscancode > 0)
		{
			if (t.inputsys.kscancode != 57)
			{
				// use that key code
				Storyboard.Nodes[nodeid].toggleKey = t.inputsys.kscancode;
			}
			else
			{
				// reset key toggle
				Storyboard.Nodes[nodeid].toggleKey = 0;
			}
			ImGui::CloseCurrentPopup();
			ResetStoryboardListenForKeys();
			bScreenToggleKeyWindow = false;
		}
		ImGui::PopStyleColor();
		ImGui::Text("");
		ImGui::EndPopup();
	}
	return iRet;
}

