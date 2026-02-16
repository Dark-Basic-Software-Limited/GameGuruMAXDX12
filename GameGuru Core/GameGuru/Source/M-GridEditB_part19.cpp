void process_storeboard(bool bInitOnly)
{
	bool bModal = false; //Use a modal window.

	//Emulate standalone.
	static int iFramesBeforeEmulate = 0;
	static char startpage[255], lastpage[255], playerrors[255] = "\0";

	if (!bInitOnly)
	{
		if (bStoryboardWindowLast != bStoryboardWindow || iLastPrefStyle != pref.current_style)
		{
			//PE: Map each time we enter.
			mapNodeStyle();
			bStoryboardWindowLast = bStoryboardWindow;
			iLastPrefStyle = pref.current_style;
		}
	}

	if (!bStoryboardWindow && !bInitOnly)
	{
		if (!bWelcomeScreen_Window && (pref.iDisplayWelcomeScreen == 0 || pref.iLastInStoryboard == 0 ) && bTriggerWhatsNewInStoryboard)
		{
			if (g.gshowannouncements == 1)
			{
				if (g_iWelcomeLoopPage == WELCOME_ANNOUNCEMENTS)
				{
					if (gbWelcomeSystemActive == false)
					{
						welcome_init(1);
						welcome_init(2);
					}

					welcome_init(0);
				}
				bTriggerWhatsNewInStoryboard = false;
			}
		}
	}

	if (bStoryboardWindow || bInitOnly)
	{

		//PE: setup fixed ID's
		Storyboard.game_thumb_id = STORYBOARD_THUMBS + 420;
		Storyboard.game_icon_id = STORYBOARD_THUMBS + 421;

		if (!bInitOnly)
		{
			//PE: You cant get here, without level has already asked you to save changes, so always disable project save flag here.
			g.projectmodified = 0; gridedit_changemodifiedflag();
			g.projectmodifiedstatic = 0;
		}

		if (!bInitOnly)
		{
			if (gbWelcomeSystemActive)
			{
				//PE: Hide everything in the background (wicked 3D stuff).
				ImGuiWindow* window = ImGui::FindWindowByName("Toolbar");//ImGui::GetCurrentWindow();
				ImVec4 monitor_col = ImVec4(0, 0, 0, 1.0); //Black for now.
				window->DrawList->AddRectFilled(ImVec2(-1, -1), ImGui::GetMainViewport()->Size + ImVec2(1, 1), ImGui::GetColorU32(monitor_col));
				return;
			}
			else
			{
				static int iHideWindowForFrames = 8;
				if (iHideWindowForFrames > 0)
				{
					iHideWindowForFrames--;
					//PE: Hide everything in the background (wicked 3D stuff).
					ImGuiWindow* window = ImGui::FindWindowByName("Toolbar");//ImGui::GetCurrentWindow();
					ImVec4 monitor_col = ImVec4(0, 0, 0, 1.0); //Black for now.
					window->DrawList->AddRectFilled(ImVec2(-1, -1), ImGui::GetMainViewport()->Size + ImVec2(1, 1), ImGui::GetColorU32(monitor_col));
					return;
				}
				if (bTriggerWhatsNewInStoryboard)
				{
					if (g.gshowannouncements == 1)
					{
						if (g_iWelcomeLoopPage == WELCOME_ANNOUNCEMENTS)
						{
							if (gbWelcomeSystemActive == false)
							{
								welcome_init(1);
								welcome_init(2);
							}

							welcome_init(0);
						}
						bTriggerWhatsNewInStoryboard = false;
					}
				}
			}
		}

		pref.iLastInStoryboard = true;
		int isize = sizeof(Storyboard); //Test.

		//PE: Use full available area.
		int preview_size_x = ImGui::GetMainViewport()->Size.x - 300.0;
		int preview_size_y = ImGui::GetMainViewport()->Size.y - 60.0;
		float fNodeWidth = 180.0f;
		float fNodeHeight = 130.0f;
		float fImgRatio = fNodeWidth/512.0;
		ImVec2 iThumbSize = ImVec2(fNodeWidth, 288.0 * fImgRatio);
		static int iLastHoveredId = -1;
		static int iLastHoveredNodeId = -1;

		//PE: Only execute this one time.
		static bool bInitStartupProject = true;
		if (bStoryboardInitNodes && bInitStartupProject)
		{
			static int iDelayLoadFrames = 2;
			if (iDelayLoadFrames == 0)
			{
				extern bool bSpecialEditorFromStandalone;
				if (bSpecialEditorFromStandalone)
				{
					extern char cSpecialStandaloneProject[MAX_PATH];
					load_storyboard(cSpecialStandaloneProject);
					iGamePausedNodeID = storyboard_add_missing_nodex(8, preview_size_x, fNodeWidth, fNodeHeight + 20.0, false);
					iLoadGameNodeID = storyboard_add_missing_nodex(3, preview_size_x, fNodeWidth, fNodeHeight + 20.0, false);
					iSaveGameNodeID = storyboard_add_missing_nodex(9, preview_size_x, fNodeWidth, fNodeHeight + 20.0, false);
					iGraphicsNodeID = storyboard_add_missing_nodex(10, preview_size_x, fNodeWidth, fNodeHeight + 20.0, false);
					iSoundsNodeID = storyboard_add_missing_nodex(11, preview_size_x, fNodeWidth, fNodeHeight + 20.0, false);
					iControlNodeID = storyboard_add_missing_nodex(12, preview_size_x, fNodeWidth, fNodeHeight + 20.0, false);
					iLoadingScreenNodeID = storyboard_add_missing_nodex(2, preview_size_x, fNodeWidth, fNodeHeight + 20.0, false);
					iHUDScreenNodeID = storyboard_add_missing_nodex(13, preview_size_x, fNodeWidth, fNodeHeight + 20.0, false);
					//Nothing could have changed we are going directly to storyboard.
					g.projectmodified = 0;
					g.projectmodifiedstatic = 0;
				}
				else if (!pref.iDisplayWelcomeScreen)
				{
					if (strlen(pref.cLastUsedStoryboardProject) > 0)
					{
						load_storyboard(pref.cLastUsedStoryboardProject);
						iGamePausedNodeID = storyboard_add_missing_nodex(8, preview_size_x, fNodeWidth, fNodeHeight + 20.0, false);
						iLoadGameNodeID = storyboard_add_missing_nodex(3, preview_size_x, fNodeWidth, fNodeHeight + 20.0, false);
						iSaveGameNodeID = storyboard_add_missing_nodex(9, preview_size_x, fNodeWidth, fNodeHeight + 20.0, false);
						iGraphicsNodeID = storyboard_add_missing_nodex(10, preview_size_x, fNodeWidth, fNodeHeight + 20.0, false);
						iSoundsNodeID = storyboard_add_missing_nodex(11, preview_size_x, fNodeWidth, fNodeHeight + 20.0, false);
						iControlNodeID = storyboard_add_missing_nodex(12, preview_size_x, fNodeWidth, fNodeHeight + 20.0, false);
						iLoadingScreenNodeID = storyboard_add_missing_nodex(2, preview_size_x, fNodeWidth, fNodeHeight + 20.0, false);
						iHUDScreenNodeID = storyboard_add_missing_nodex(13, preview_size_x, fNodeWidth, fNodeHeight + 20.0, false);

						//Nothing could have changed we are going directly to storyboard.
						g.projectmodified = 0;
						g.projectmodifiedstatic = 0;

					}
				}
				bInitStartupProject = false;
			}
			else iDelayLoadFrames--;
		}

		static int iStoryboardAdvancedChanged = -1;

		if (bInitOnly) iStoryboardAdvancedChanged = -1; //PE: Make sure to reload after new project on welcome screen.
		if (!bInitOnly)
		{
			if (!bStoryboardInitNodes)
			{
				iStoryboardAdvancedChanged = pref.iStoryboardAdvanced;
			}
			if (iStoryboardAdvancedChanged != pref.iStoryboardAdvanced)
			{
				//PE: If we get a change we must reload the project.
				if (strlen(Storyboard.gamename) > 0)
				{
					load_storyboard(Storyboard.gamename);
					iGamePausedNodeID = storyboard_add_missing_nodex(8, preview_size_x, fNodeWidth, fNodeHeight + 20.0, false);
					iLoadGameNodeID = storyboard_add_missing_nodex(3, preview_size_x, fNodeWidth, fNodeHeight + 20.0, false);
					iSaveGameNodeID = storyboard_add_missing_nodex(9, preview_size_x, fNodeWidth, fNodeHeight + 20.0, false);
					iGraphicsNodeID = storyboard_add_missing_nodex(10, preview_size_x, fNodeWidth, fNodeHeight + 20.0, false);
					iSoundsNodeID = storyboard_add_missing_nodex(11, preview_size_x, fNodeWidth, fNodeHeight + 20.0, false);
					iControlNodeID = storyboard_add_missing_nodex(12, preview_size_x, fNodeWidth, fNodeHeight + 20.0, false);
					iLoadingScreenNodeID = storyboard_add_missing_nodex(2, preview_size_x, fNodeWidth, fNodeHeight + 20.0, false);
					
				}
				else
				{
					//We got a empty level , setup everything again.
					bStoryboardInitNodes = false; //Just init again.
					bStoryboardFirstRunSetInitPos = false;
					strcpy(pref.cLastUsedStoryboardProject, "");
					bTriggerSaveAsAfterNewLevel = true;
					bTriggerSaveAs = true;
					strcpy(SaveProjectAsName, "");
					strcpy(SaveProjectAsError, "");
				}
				iStoryboardAdvancedChanged = pref.iStoryboardAdvanced;
			}
		}

		storeboard_init_nodes(preview_size_x, fNodeWidth, fNodeHeight+20.0);

		if (bInitOnly) return;

		if (TriggerLoadGameProject != "")
		{
			// and in case this was a remote project, restore to writables regular
			extern void switch_to_regular_projects(void);
			switch_to_regular_projects();

			load_storyboard((char *)TriggerLoadGameProject.Get());
			iGamePausedNodeID = storyboard_add_missing_nodex(8, preview_size_x, fNodeWidth, fNodeHeight + 20.0, false);
			iLoadGameNodeID = storyboard_add_missing_nodex(3, preview_size_x, fNodeWidth, fNodeHeight + 20.0, false);
			iSaveGameNodeID = storyboard_add_missing_nodex(9, preview_size_x, fNodeWidth, fNodeHeight + 20.0, false);
			iGraphicsNodeID = storyboard_add_missing_nodex(10, preview_size_x, fNodeWidth, fNodeHeight + 20.0, false);
			iSoundsNodeID = storyboard_add_missing_nodex(11, preview_size_x, fNodeWidth, fNodeHeight + 20.0, false);
			iControlNodeID = storyboard_add_missing_nodex(12, preview_size_x, fNodeWidth, fNodeHeight + 20.0, false);
			iLoadingScreenNodeID = storyboard_add_missing_nodex(2, preview_size_x, fNodeWidth, fNodeHeight + 20.0, false);
			iHUDScreenNodeID = storyboard_add_missing_nodex(13, preview_size_x, fNodeWidth, fNodeHeight + 20.0, false);

			TriggerLoadGameProject = "";
			bTriggerSaveAsAfterNewLevel = false;
			bTriggerSaveAs = false;
		}

		static bool bGotAThumb = false;
		
		if (iWaitFor2DEditor > 0 && iWaitFor2DEditorNode >= 0)
		{
			if (!bScreen_Editor_Window)
			{
				if (iWaitFor2DEditor == 1)
				{
					//Closed.
					GG_SetWritablesToRoot(true);
					if (FileExist("thumbbank\\lastnewlevel.jpg"))
					{
						image_setlegacyimageloading(true);
						//Use a tmp unique image id
						LoadImageSize("thumbbank\\lastnewlevel.jpg", STORYBOARD_THUMBS + 401, 512, 288);
						image_setlegacyimageloading(false);

						if (ImageExist(STORYBOARD_THUMBS + 401))
						{
							//Get it into the correct id.
							image_setlegacyimageloading(true);
							LoadImageSize("thumbbank\\lastnewlevel.jpg", Storyboard.Nodes[iWaitFor2DEditorNode].thumb_id, 512, 288);
							image_setlegacyimageloading(false);

							if (ImageExist(Storyboard.Nodes[iWaitFor2DEditorNode].thumb_id))
							{
								//Save into thumbbank , and save thumb filename in node.
								//PE: Needed to add Storyboard.gamename so we dont get duplicates.
								// name of thumb is now based on screen title, to prevent multiple screens with same thumb name
								cstr name = cstr("screen_") + cstr(Storyboard.gamename) + cstr("_") + cstr(Storyboard.Nodes[iWaitFor2DEditorNode].title);
								CreateBackBufferCacheNameEx(name.Get(), 512, 288, true);
								SaveImage(BackBufferCacheName.Get(), Storyboard.Nodes[iWaitFor2DEditorNode].thumb_id);
								if (FileExist(BackBufferCacheName.Get()))
								{
									if (CopyToProjectFolder(BackBufferCacheName.Get()))
									{
										//PE: Use relative projectbank filename.
										strcpy(Storyboard.Nodes[iWaitFor2DEditorNode].thumb, ProjectCacheName.Get());
									}
									else
									{
										strcpy(Storyboard.Nodes[iWaitFor2DEditorNode].thumb, BackBufferCacheName.Get());
									}
								}
							}
						}
					}
					GG_SetWritablesToRoot(false);

					//Close down.
					iWaitFor2DEditor = 0;
					iWaitFor2DEditorNode = -1;
				}
				else
					iWaitFor2DEditor--;
			}
		}
		if (iWaitForNewScreenshot > 0 && iScreenshotNode >= 0)
		{
			//PE: Start process after a load.
			if (iSkibFramesBeforeLaunch <= 0)
			{
				//PE: Let level load settle and terrain generation finish before starting.
				if (!bProceduralLevel && iWaitForNewScreenshot == 3)
				{
					//Startup take screenshot.
					extern bool bPopModalOpenProceduralCameraMode;
					bPopModalOpenProceduralCameraMode = true;
					bProceduralLevel = true;
					iWaitForNewScreenshot = 2;
					bProceduralLevelFromStoryboard = true; //So it dont quit.
				}
				else
				{
					if (iWaitForNewScreenshot == 2)
					{
						//Now wait for screenshot window to close.
						if (!bProceduralLevel)
						{
							//Closed, check created screenshot.
							bGotAThumb = false;
							GG_SetWritablesToRoot(true);
							if (FileExist("thumbbank\\lastnewlevel.jpg"))
							{
								image_setlegacyimageloading(true);
								//Use a tmp unique image id
								LoadImageSize("thumbbank\\lastnewlevel.jpg", STORYBOARD_THUMBS + 401, 512, 288);
								image_setlegacyimageloading(false);
								if (ImageExist(STORYBOARD_THUMBS + 401))
									bGotAThumb = true;
							}
							GG_SetWritablesToRoot(false);

							if(!bGotAThumb)
							{
								//No screenshot created just quit.
								iWaitForNewScreenshot = 0;
							}
							else
							{
								iWaitForNewScreenshot = 1;
							}
						}
					}
					else if (iWaitForNewScreenshot == 1)
					{
						//Last step. ask if they like to save new screenshot.
						ImGui::OpenPopup("Screenshot#Storyboard");
						ImGui::SetNextWindowSize(ImVec2(0, 532), ImGuiCond_Once);
						static int popwinheight = 0;
						if (popwinheight > 800 || iSkibFramesBeforeLaunch > 0)
						{
							ImGui::SetNextWindowSize(ImVec2(0, 532), ImGuiCond_Always);
						}
						ImGui::SetNextWindowPosCenter(ImGuiCond_Always);
						bool bScreenshotWindow = true;
						if (ImGui::BeginPopupModal("Screenshot#Storyboard", &bScreenshotWindow, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings))
						{
							popwinheight = ImGui::GetWindowSize().y;
							ImGui::Indent(10);
							ImGui::Text("");
							ImGui::SetWindowFontScale(1.4);
							extern bool bPopModalTakeMapSnapshot;
							if(bPopModalTakeMapSnapshot==true)
								ImGui::TextCenter("Use New Map Snapshot ?");
							else
								ImGui::TextCenter("Use New Screenshot ?");
							ImGui::Separator();
							if (ImageExist(STORYBOARD_THUMBS + 401))
							{
								ImGui::ImgBtn(STORYBOARD_THUMBS + 401, ImVec2(512, 288), ImColor(0, 0, 0, 0), ImColor(255, 255, 255, 255), ImColor(255, 255, 255, 255), ImColor(255, 255, 255, 255), 0, 0, 0, 0, false, false, false);
								ImGui::Separator();
							}
							ImGui::SetWindowFontScale(1.0);

							ImGui::SetWindowFontScale(1.4);
							LPSTR pUseNewSnapTitle = "Use New Screenshot";
							if (bPopModalTakeMapSnapshot == true) pUseNewSnapTitle = "Use New Map Snapshot";
							if (ImGui::StyleButton(pUseNewSnapTitle, ImVec2(ImGui::GetContentRegionAvail().x*0.5 - 20.0f, 0.0f)))
							{
								//screenshot or map snapshot
								bool bDoScreenshotForThumb = false;
								if (bPopModalTakeMapSnapshot == true)
								{
									// Load into map snapshot image then save alongside mapbank FPM level file
									GG_SetWritablesToRoot(true);
									int iMapImageID = Storyboard.Nodes[iScreenshotNode].thumb_id;
									image_setlegacyimageloading(true);
									LoadImageSize("thumbbank\\lastnewlevel.jpg", iMapImageID, 2048, 2048);
									image_setlegacyimageloading(false);
									GG_SetWritablesToRoot(false);
									if (ImageExist(iMapImageID)==1)
									{
										// filename to save map image alongside mapbank FPM file
										char pMapImageFile[MAX_PATH];
										strcpy(pMapImageFile, g.fpscrootdir_s.Get());
										strcat(pMapImageFile, "\\Files\\");
										strcat(pMapImageFile, Storyboard.Nodes[iScreenshotNode].level_name);
										if (strnicmp(pMapImageFile + strlen(pMapImageFile) - 4, ".fpm", 4) == NULL)
										{
											pMapImageFile[strlen(pMapImageFile) - 4] = 0;
										}
										strcat(pMapImageFile, ".png");
										GG_GetRealPath(pMapImageFile, 1);
										SaveImage(pMapImageFile, iMapImageID);
									}

									// and restore screenshot thumb if one exists
									if (FileExist(Storyboard.Nodes[iScreenshotNode].thumb))
									{
										image_setlegacyimageloading(true);
										LoadImageSize(Storyboard.Nodes[iScreenshotNode].thumb, Storyboard.Nodes[iScreenshotNode].thumb_id, 512, 288);
										image_setlegacyimageloading(false);
									}
									else
									{
										// no screenshot when did map snapshot, so do screenshot at same time to have something to show
										bDoScreenshotForThumb = true;
									}
								}
								else
								{
									// screenshot mode
									bDoScreenshotForThumb = true;
								}
								if(bDoScreenshotForThumb==true)
								{
									//Load into node slot.
									GG_SetWritablesToRoot(true);
									image_setlegacyimageloading(true);
									LoadImageSize("thumbbank\\lastnewlevel.jpg", Storyboard.Nodes[iScreenshotNode].thumb_id, 512, 288);
									image_setlegacyimageloading(false);
									GG_SetWritablesToRoot(false);
									if (ImageExist(Storyboard.Nodes[iScreenshotNode].thumb_id))
									{
										//Save into thumbbank , and save thumb filename in node.
										CreateBackBufferCacheNameEx(Storyboard.Nodes[iScreenshotNode].level_name, 512, 288, true);
										SaveImage(BackBufferCacheName.Get(), Storyboard.Nodes[iScreenshotNode].thumb_id);
										if (FileExist(BackBufferCacheName.Get()))
										{
											if (CopyToProjectFolder(BackBufferCacheName.Get()))
											{
												//PE: Use relative projectbank filename.
												strcpy(Storyboard.Nodes[iScreenshotNode].thumb, ProjectCacheName.Get());
											}
											else
											{
												strcpy(Storyboard.Nodes[iScreenshotNode].thumb, BackBufferCacheName.Get());
											}
										}
									}
								}
								iWaitForNewScreenshot = 0;
							}
							ImGui::SameLine();
							if (ImGui::StyleButton("Cancel", ImVec2(ImGui::GetContentRegionAvail().x - 10.0f, 0.0f)))
							{
								//Cancel.
								iWaitForNewScreenshot = 0;
							}

							ImGui::SetWindowFontScale(1.0);
							ImGui::Text("");

							bImGuiGotFocus = true;
							ImGui::Indent(-10);
							ImGui::EndPopup();
						}
					}
					else
					{
						iWaitForNewScreenshot--;
					}
				}
			}

			if (iWaitForNewScreenshot == 0)
			{
				//Quit.
				iScreenshotNode = -1;
				extern bool bPopModalOpenProceduralCameraMode;
				extern bool bPopModalTakeMapSnapshot;
				bPopModalOpenProceduralCameraMode = false;
				if (bPopModalTakeMapSnapshot == true)
				{
					// and restore camera projection override
					wiScene::GetCamera().SetCustomProjectionEnabled(false);
					wiScene::GetCamera().UpdateCamera();
					bPopModalTakeMapSnapshot = false;
				}
			}
		}

		//#########################
		//#### Duplicate Level ####
		//#########################

		static char DuplicateLevelName[256] = "\0";
		static char DuplicateLevelError[256] = "\0";
		if (bDuplicateLevel && iDuplicateNode >= 0)
		{
			//Ask to save duplicate level.
			ImGui::OpenPopup("Duplicate Level#Storyboard");
			ImGui::SetNextWindowSize(ImVec2(0, 532), ImGuiCond_Once);
			static int popwinheight = 0;
			if (popwinheight > 800 || iSkibFramesBeforeLaunch > 0)
			{
				ImGui::SetNextWindowSize(ImVec2(0, 532), ImGuiCond_Always);
			}
			ImGui::SetNextWindowPosCenter(ImGuiCond_Always);
			bool bDuplicateLevelWindow = true;
			if (ImGui::BeginPopupModal("Duplicate Level#Storyboard", &bDuplicateLevelWindow, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings))
			{
				popwinheight = ImGui::GetWindowSize().y;
				ImGui::Indent(10);
				ImGui::Text("");
				ImGui::SetWindowFontScale(1.4);
				ImGui::TextCenter("Duplicate Level As");
				ImGui::SetWindowFontScale(1.0);
				ImGui::Text("");
				ImGui::SetWindowFontScale(1.4);
				ImGui::Separator();
				if (ImageExist(Storyboard.Nodes[iDuplicateNode].thumb_id))
				{
					ImGui::ImgBtn(Storyboard.Nodes[iDuplicateNode].thumb_id, ImVec2(512, 288), ImColor(0, 0, 0, 0), ImColor(255, 255, 255, 255), ImColor(255, 255, 255, 255), ImColor(255, 255, 255, 255), 0, 0, 0, 0, false, false, false);
					ImGui::Separator();
				}
				else
				{
					ImGui::Dummy(ImVec2(512, 288));
					ImGui::Separator();
				}
				ImGui::SetWindowFontScale(1.0);

				ImGui::TextWrapped("To duplicate this level, please give it a new name and click 'Save'");
				ImGui::Text("");
				if (strlen(DuplicateLevelError) > 0)
				{
					ImGui::Text(DuplicateLevelError);
					ImGui::Text("");
				}
				ImGui::Text("Duplicate Level Name");
				ImGui::PushItemWidth(-10);
				ImGui::InputText("##DuplicateLevelNameStoryboard", DuplicateLevelName, 250, ImGuiInputTextFlags_None); //ImGuiInputTextFlags_None ImGuiInputTextFlags_ReadOnly
				ImGui::PopItemWidth();

				ImGui::Text("");

				ImGui::SetWindowFontScale(1.4);
				if (ImGui::StyleButton("Save Level", ImVec2(ImGui::GetContentRegionAvail().x*0.5 - 20.0f, 0.0f)))
				{
					if (strlen(DuplicateLevelName) > 0)
					{
						char tmp[MAX_PATH];
						strcpy(tmp, g.mysystem.mapbankAbs_s.Get());
						//Relative.
						char *find = (char *)pestrcasestr(tmp, "mapbank\\");
						if (find && find != &tmp[0]) strcpy(&tmp[0], find);
						strcat(tmp, DuplicateLevelName);

						t.returnstring_s = tmp;

						if (cstr(Lower(Right(t.returnstring_s.Get(), 4))) != ".fpm")  t.returnstring_s = t.returnstring_s + ".fpm";
						g.projectfilename_s = t.returnstring_s;

						bool oksave = true;
						if (FileExist(g.projectfilename_s.Get())) {
							oksave = overWriteFileBox(g.projectfilename_s.Get());
						}
						if (oksave)
						{
							//Add newly saved fpm level to recent list.
							int firstempty = -1;
							int i = 0;
							for (; i < REMEMBERLASTFILES; i++) {
								if (firstempty == -1 && strlen(pref.last_open_files[i]) <= 0)
									firstempty = i;
								if (strlen(pref.last_open_files[i]) > 0 && pestrcasestr(g.projectfilename_s.Get(), pref.last_open_files[i])) { //already there
									break;
								}
							}
							if (i >= REMEMBERLASTFILES) {
								if (firstempty == -1) {
									//No empty slots , rotate.
									for (int ii = 0; ii < REMEMBERLASTFILES - 1; ii++) {
										strcpy(pref.last_open_files[ii], pref.last_open_files[ii + 1]);
									}
									strcpy(pref.last_open_files[REMEMBERLASTFILES - 1], g.projectfilename_s.Get());
								}
								else
									strcpy(pref.last_open_files[firstempty], g.projectfilename_s.Get());
							}

							//PE: Find next level from nodes.
							int iNextLevel = 0, levelname = -1, iFirstNodeFree = -1;
							FindFreeLevelNode(iNextLevel, levelname, iFirstNodeFree);

							if (iFirstNodeFree >= 0)
							{
								//Create new level.
								char tmp[255];
								int node = iFirstNodeFree;
								int nodeposy = iNextLevel;
								if (levelname > 0)
								{
									sprintf(tmp, "Level %d", levelname);
									nodeposy = levelname - 1;
								}
								else
									sprintf(tmp, "Level %d", iNextLevel + 1);

								//PE: Make sure any old data is removed, also thumbs.
								reset_single_node(node);

								Storyboard.Nodes[node].used = true;
								Storyboard.Nodes[node].type = STORYBOARD_TYPE_LEVEL;
								Storyboard.Nodes[node].restore_position = ImVec2(preview_size_x*0.5 - (fNodeWidth*0.5) + ((fNodeWidth + NODE_WIDTH_PADDING)*2.0), STORYBOARD_YSTART + ((fNodeHeight + 20.0 + NODE_HEIGHT_PADDING) * (nodeposy)));
								Storyboard.Nodes[node].iEditEnable = true;
								strcpy(Storyboard.Nodes[node].title, tmp);
								strcpy(Storyboard.Nodes[node].levelnumber, tmp);

								strcpy(Storyboard.Nodes[node].thumb, "");
								//Input.
								strcpy(Storyboard.Nodes[node].input_title[0], " Input ");
								//Output.
								strcpy(Storyboard.Nodes[node].output_title[0], " WIN LEVEL -> Connect to Scene ");
								strcpy(Storyboard.Nodes[node].output_action[0], "loadlevel"); //Not defined this yet.
								Storyboard.Nodes[node].output_can_link_to_type[0] = STORYBOARD_TYPE_SCREEN;
								Storyboard.Nodes[node].output_linkto[0] = 0;

								strcpy(Storyboard.Nodes[node].output_title[1], " GAME OVER -> Connect to Scene ");
								strcpy(Storyboard.Nodes[node].output_action[1], "loadlevel"); //Not defined this yet.
								Storyboard.Nodes[node].output_can_link_to_type[1] = STORYBOARD_TYPE_SCREEN;
								Storyboard.Nodes[node].output_linkto[1] = 0;

								strcpy(Storyboard.Nodes[node].output_title[2], " NEXT LEVEL -> Connect to Level ");
								strcpy(Storyboard.Nodes[node].output_action[2], "loadlevel"); //Not defined this yet.
								Storyboard.Nodes[node].output_can_link_to_type[2] = STORYBOARD_TYPE_LEVEL;
								Storyboard.Nodes[node].output_linkto[2] = 0;
								ImNodes::SetNodeGridSpacePos(Storyboard.Nodes[node].id, Storyboard.Nodes[node].restore_position);


								iLaunchAfterSync = 503; //Do the actualy save here.
								iSkibFramesBeforeLaunch = 3;
								strcpy(cTriggerMessage, "Saving Level ...");
								bTriggerMessage = true;

								strcpy(Storyboard.Nodes[node].title, DuplicateLevelName);
								strcpy(Storyboard.Nodes[node].level_name, g.projectfilename_s.Get());

								if (ImageExist(Storyboard.Nodes[iDuplicateNode].thumb_id))
								{
									//Save old thumb to new thumb.
									CreateBackBufferCacheNameEx(Storyboard.Nodes[node].level_name, 512, 288, true);
									SaveImage(BackBufferCacheName.Get(), Storyboard.Nodes[iDuplicateNode].thumb_id);
									if (FileExist(BackBufferCacheName.Get()))
									{
										if (CopyToProjectFolder(BackBufferCacheName.Get()))
										{
											//PE: Use relative projectbank filename.
											strcpy(Storyboard.Nodes[node].thumb, ProjectCacheName.Get());
										}
										else
										{
											strcpy(Storyboard.Nodes[node].thumb, BackBufferCacheName.Get());
										}
										//PE: Load in new thumb to own id.
										SetMipmapNum(1); //PE: mipmaps not needed.
										image_setlegacyimageloading(true);
										LoadImageSize(Storyboard.Nodes[node].thumb, Storyboard.Nodes[node].thumb_id, 512, 288);
										image_setlegacyimageloading(false);
										SetMipmapNum(-1);
									}
								}
								bDuplicateLevel = false;
							}
							else
							{
								char pErrMess[256];
								sprintf(pErrMess, "Error: Number of allocated nodes reached. The maximum nodes is %d.", STORYBOARD_MAXNODES);
								strcpy(DuplicateLevelError, pErrMess);
							}
						}
						else
						{
							//Cancel just ignore.
						}
					}
					else
					{
						strcpy(DuplicateLevelError, "Error: Please give your level a name before save.");
					}
				}
				ImGui::SameLine();
				if (ImGui::StyleButton("Cancel", ImVec2(ImGui::GetContentRegionAvail().x - 10.0f, 0.0f)))
				{
					//Cancel.
					bDuplicateLevel = false;
				}

				ImGui::SetWindowFontScale(1.0);
				ImGui::Text("");

				bImGuiGotFocus = true;
				ImGui::Indent(-10);
				ImGui::EndPopup();
			}

		}


		//######################
		//#### Rename Level ####
		//######################

		static char RenameLevelName[256] = "\0";
		static char RenameOriginalLevelName[MAX_PATH] = "\0";
		static char RenameLevelError[256] = "\0";
		if (bRenameLevel && iRenameNode >= 0)
		{
			//Ask to Rename level.
			ImGui::OpenPopup("Rename Level##Storyboard");
			ImGui::SetNextWindowSize(ImVec2(0, 532), ImGuiCond_Once);
			static int popwinheight = 0;
			if (popwinheight > 800 || iSkibFramesBeforeLaunch > 0)
			{
				ImGui::SetNextWindowSize(ImVec2(0, 532), ImGuiCond_Always);
			}
			ImGui::SetNextWindowPosCenter(ImGuiCond_Always);
			bool bRenameLevelWindow = true;
			if (ImGui::BeginPopupModal("Rename Level##Storyboard", &bRenameLevelWindow, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings))
			{
				popwinheight = ImGui::GetWindowSize().y;
				ImGui::Indent(10);
				ImGui::Text("");
				ImGui::SetWindowFontScale(1.4);
				ImGui::TextCenter("Rename Level");
				ImGui::SetWindowFontScale(1.0);
				ImGui::Text("");
				ImGui::SetWindowFontScale(1.4);
				ImGui::Separator();
				if (ImageExist(Storyboard.Nodes[iRenameNode].thumb_id))
				{
					ImGui::ImgBtn(Storyboard.Nodes[iRenameNode].thumb_id, ImVec2(512, 288), ImColor(0, 0, 0, 0), ImColor(255, 255, 255, 255), ImColor(255, 255, 255, 255), ImColor(255, 255, 255, 255), 0, 0, 0, 0, false, false, false);
					ImGui::Separator();
				}
				else
				{
					ImGui::Dummy(ImVec2(512, 288));
					ImGui::Separator();
				}
				ImGui::SetWindowFontScale(1.0);

				ImGui::TextWrapped("To rename this level, please give it a new name and click 'Rename'");
				ImGui::Text("");
				if (strlen(RenameLevelError) > 0)
				{
					ImGui::Text(RenameLevelError);
					ImGui::Text("");
				}
				ImGui::Text("Rename Level To");
				ImGui::PushItemWidth(-10);
				ImGui::InputText("##RenameLevelNameStoryboard", RenameLevelName, 250, ImGuiInputTextFlags_None); //ImGuiInputTextFlags_None ImGuiInputTextFlags_ReadOnly
				ImGui::PopItemWidth();

				ImGui::Text("");

				ImGui::SetWindowFontScale(1.4);
				if (ImGui::StyleButton("Rename Level", ImVec2(ImGui::GetContentRegionAvail().x*0.5 - 20.0f, 0.0f)))
				{
					if (strlen(RenameLevelName) > 0)
					{
						char tmp[MAX_PATH];
						strcpy(tmp, g.mysystem.mapbankAbs_s.Get());
						//Relative.
						char *find = (char *)pestrcasestr(tmp, "mapbank\\");
						if (find && find != &tmp[0]) strcpy(&tmp[0], find);
						strcat(tmp, RenameLevelName);

						t.returnstring_s = tmp;

						if (cstr(Lower(Right(t.returnstring_s.Get(), 4))) != ".fpm")  t.returnstring_s = t.returnstring_s + ".fpm";
						g.projectfilename_s = t.returnstring_s;

						bool oksave = true;
						if (FileExist(g.projectfilename_s.Get())) {
							oksave = overWriteFileBox(g.projectfilename_s.Get());
						}
						if (oksave)
						{
							//PE: Just rename level.
							//PE: Also remember thumb ?

							//PE: If level is not from doc write do not delete the original, but only make a copy.
							bool bIsInstallFolder = false;
							extern char szRootDir[MAX_PATH];
							if (pestrcasestr(RenameOriginalLevelName, szRootDir)) bIsInstallFolder = true;

							char destination[MAX_PATH];
							strcpy(destination, g.projectfilename_s.Get());
							GG_GetRealPath(destination, 1); //Resolve name. need full path.

							if (bIsInstallFolder)
							{
								CopyFileA(RenameOriginalLevelName, destination,false);
							}
							else
							{
								rename(RenameOriginalLevelName, destination);
							}

							if (FileExist(destination))
							{

								//Now setup new level name include relative only.
								strcpy(tmp, destination);
								char *find = (char *)pestrcasestr(tmp, "mapbank\\");
								if (find && find != &tmp[0]) strcpy(&tmp[0], find);
								strcpy(Storyboard.Nodes[iRenameNode].level_name, tmp);

								//Setup new title.
								std::string sLevelTitle = Storyboard.Nodes[iRenameNode].level_name;
								replaceAll(sLevelTitle, ".fpm", "");
								replaceAll(sLevelTitle, "mapbank\\", "");
								strcpy(Storyboard.Nodes[iRenameNode].title, sLevelTitle.c_str());

								bRenameLevel = false;

								//PE: Save changes if possible.
								if (!pref.iDisableProjectAutoSave && strlen(Storyboard.gamename) > 0)
								{
									save_storyboard(Storyboard.gamename, false);
								}

							}
							else
							{
								strcpy(RenameLevelError, "Error: Rename failed no changes done.");
							}
						}
						else
						{
							//Cancel just ignore.
						}
					}
					else
					{
						strcpy(RenameLevelError, "Error: Please give your level a new name before renaming.");
					}
				}
				ImGui::SameLine();
				if (ImGui::StyleButton("Cancel", ImVec2(ImGui::GetContentRegionAvail().x - 10.0f, 0.0f)))
				{
					//Cancel.
					bRenameLevel = false;
				}

				ImGui::SetWindowFontScale(1.0);
				ImGui::Text("");

				bImGuiGotFocus = true;
				ImGui::Indent(-10);
				ImGui::EndPopup();
			}
		}

		void storyboard_openproject(float preview_size_x, float fNodeWidth, float fNodeHeight,int mode);
		storyboard_openproject(preview_size_x, fNodeWidth, fNodeHeight,0);

		if (bStoryboardWindowOpenLoad)
		{
			bTriggerOpenProject = true;
			bStoryboardWindowOpenLoad = false;
			bTriggerSaveAs = false;
			bTriggerSaveAsAfterNewLevel = false;
		}

		int iCreateRet = save_create_storyboard_project();

		if (!bPopModalStoryboard)
		{
			if (bModal)
				ImGui::OpenPopup("##StoryboardWindow");
		}
		static int UpdateThumbOnNode = -1;
		if (UpdateThumbOnNode >= 0)
		{
			image_setlegacyimageloading(true);
			if (Storyboard.Nodes[UpdateThumbOnNode].type == STORYBOARD_TYPE_SPLASH)
			{
				//PE: On splash load directly. Splash always get a center positons.
				bool bUseRealSplash = true;
				if ( strcmp(Storyboard.Nodes[UpdateThumbOnNode].thumb, "editors\\uiv3\\loadingsplash.jpg") == 0)
				{
					LoadImage("editors\\uiv3\\user-splash-screen.png", Storyboard.Nodes[UpdateThumbOnNode].thumb_id);
					if(ImageExist(Storyboard.Nodes[UpdateThumbOnNode].thumb_id)) bUseRealSplash = false;
				}

				if(bUseRealSplash)
					LoadImage(Storyboard.Nodes[UpdateThumbOnNode].thumb, Storyboard.Nodes[UpdateThumbOnNode].thumb_id);
			}
			else
			{
				LoadImageSize(Storyboard.Nodes[UpdateThumbOnNode].thumb, Storyboard.Nodes[UpdateThumbOnNode].thumb_id, 512, 288);
			}
			image_setlegacyimageloading(false);

			UpdateThumbOnNode = -1;
		}
		if (!bStoryboardFirstRunSetInitPos)
		{

			if (Storyboard.iStoryboardVersion != STORYBOARDVERSION)
			{
				//PE: Convert old storyboard setups if needed.
				if (Storyboard.iStoryboardVersion == 100 && Storyboard.project_readonly == 0)
				{
					Storyboard.iStoryboardVersion = STORYBOARDVERSION;
					Storyboard.iChanged = true;
					//PE: Convert old thumb path to new projectbank path, and copy thumbs.
					for (int i = 0; i < STORYBOARD_MAXNODES; i++)
					{
						bool bValid = true;
						if (bValid && Storyboard.Nodes[i].used)
						{
							if (strlen(Storyboard.Nodes[i].thumb) > 0)
							{
								char *find = (char *)pestrcasestr(Storyboard.Nodes[i].thumb, "thumbbank\\");
								if (find)
								{
									if (CopyToProjectFolder(Storyboard.Nodes[i].thumb))
									{
										//PE: Use relative projectbank filename.
										if (FileExist(ProjectCacheName.Get()))
										{
											strcpy(Storyboard.Nodes[i].thumb, ProjectCacheName.Get());
										}
									}
								}
							}
							if (Storyboard.Nodes[i].type == STORYBOARD_TYPE_LEVEL && !pestrcasestr(Storyboard.Nodes[i].lua_name, "loading"))
							{
								if (strlen(Storyboard.Nodes[i].level_name) > 0)
								{
									char *find = (char *)pestrcasestr(Storyboard.Nodes[i].thumb, "thumbbank\\");
									if (find)
									{
										if (CopyToProjectFolder(Storyboard.Nodes[i].thumb))
										{
											//PE: Use relative projectbank filename.
											if (FileExist(ProjectCacheName.Get()))
											{
												strcpy(Storyboard.Nodes[i].thumb, ProjectCacheName.Get());
											}
										}
									}
								}
							}
						}
					}
				}
			}

			//PE: Initial position. and initial nodes.
			if (ImageExist(Storyboard.game_icon_id)) DeleteImage(Storyboard.game_icon_id);
			if (ImageExist(Storyboard.game_thumb_id)) DeleteImage(Storyboard.game_thumb_id);
			if (strlen(Storyboard.game_thumb) > 0)
			{
				image_setlegacyimageloading(true);
				LoadImage(Storyboard.game_thumb, Storyboard.game_thumb_id);
				image_setlegacyimageloading(false);
			}
			if (strlen(Storyboard.game_icon) > 0)
			{
				image_setlegacyimageloading(true);
				LoadImageSize(Storyboard.game_icon, Storyboard.game_icon_id, 256, 256);
				image_setlegacyimageloading(false);
			}

			for (int i = 0; i < STORYBOARD_MAXNODES; i++)
			{
				bool bValid = true;
				if (bValid && Storyboard.Nodes[i].used)
				{

					if (ImageExist(Storyboard.Nodes[i].screen_backdrop_id)) DeleteImage(Storyboard.Nodes[i].screen_backdrop_id);
					if (strlen(Storyboard.Nodes[i].screen_backdrop) > 0)
					{
						//Backdrop.
						image_setlegacyimageloading(true);
						LoadImage(Storyboard.Nodes[i].screen_backdrop, Storyboard.Nodes[i].screen_backdrop_id);
						image_setlegacyimageloading(false);
					}

					ImNodes::SetNodeGridSpacePos(Storyboard.Nodes[i].id, Storyboard.Nodes[i].restore_position);

					if (ImageExist(Storyboard.Nodes[i].thumb_id)) DeleteImage(Storyboard.Nodes[i].thumb_id);
					if (!ImageExist(Storyboard.Nodes[i].thumb_id) && strlen(Storyboard.Nodes[i].thumb) > 0)
					{
						image_setlegacyimageloading(true);
						if (Storyboard.Nodes[i].type == STORYBOARD_TYPE_SPLASH)
						{
							//PE: On splash load directly. Splash always get a center positons.
							bool bUseRealSplash = true;
							if (strcmp(Storyboard.Nodes[i].thumb, "editors\\uiv3\\loadingsplash.jpg") == 0)
							{
								LoadImage("editors\\uiv3\\user-splash-screen.png", Storyboard.Nodes[i].thumb_id);
								if (ImageExist(Storyboard.Nodes[i].thumb_id)) bUseRealSplash = false;
							}

							if (bUseRealSplash)

							LoadImage(Storyboard.Nodes[i].thumb, Storyboard.Nodes[i].thumb_id);
						}
						else
						{
							LoadImageSize(Storyboard.Nodes[i].thumb, Storyboard.Nodes[i].thumb_id, 512, 288);
							// If .jpg failed, try .dds (legacy JPG thumbnails may have been renamed to DDS by DX12 cleanup)
							if (!ImageExist(Storyboard.Nodes[i].thumb_id))
							{
								int thumbLen = strlen(Storyboard.Nodes[i].thumb);
								if (thumbLen > 4 && _stricmp(Storyboard.Nodes[i].thumb + thumbLen - 4, ".jpg") == 0)
								{
									char ddsThumb[256];
									strcpy(ddsThumb, Storyboard.Nodes[i].thumb);
									strcpy(ddsThumb + thumbLen - 4, ".dds");
									LoadImageSize(ddsThumb, Storyboard.Nodes[i].thumb_id, 512, 288);
									if (ImageExist(Storyboard.Nodes[i].thumb_id))
									{
										strcpy(Storyboard.Nodes[i].thumb, ddsThumb);
									}
								}
							}
							if (!ImageExist(Storyboard.Nodes[i].thumb_id))
							{
								//PE: Try relative path.
								char tmp[MAX_PATH];
								strcpy(tmp, Storyboard.Nodes[i].thumb);
								char *find = (char *) pestrcasestr(tmp, "\\thumbbank\\");
								if (find)
								{
									find++;
									LoadImageSize(find, Storyboard.Nodes[i].thumb_id, 512, 288);
									if (ImageExist(Storyboard.Nodes[i].thumb_id))
									{
										//PE: Update thumb path.
										strcpy(Storyboard.Nodes[i].thumb,find);
									}
									else
									{
										//Relative also failed, use default.
										if (Storyboard.Nodes[i].type == STORYBOARD_TYPE_SCREEN && i == iTitleScreenNodeID)
										{
											LoadImageSize("editors\\templates\\thumbs\\screen_title.lua.png", Storyboard.Nodes[i].thumb_id, 512, 288);
										}
										else if (Storyboard.Nodes[i].type == STORYBOARD_TYPE_SCREEN && i == 2)
										{
											LoadImageSize("editors\\templates\\thumbs\\screen_loading.lua.png", Storyboard.Nodes[i].thumb_id, 512, 288);
										}
										else if (Storyboard.Nodes[i].type == STORYBOARD_TYPE_SCREEN && i == iLoadGameNodeID)
										{
											LoadImageSize("editors\\templates\\thumbs\\screen_loadgame.lua.png", Storyboard.Nodes[i].thumb_id, 512, 288);
										}
										else if (Storyboard.Nodes[i].type == STORYBOARD_TYPE_SCREEN && i == 4)
										{
											LoadImageSize("editors\\templates\\thumbs\\screen_about.lua.png", Storyboard.Nodes[i].thumb_id, 512, 288);
										}
										else if (Storyboard.Nodes[i].type == STORYBOARD_TYPE_SCREEN && strcmp(Storyboard.Nodes[i].title,"Game Won Screen") == 0)
										{
											LoadImageSize("editors\\templates\\thumbs\\screen_win.lua.png", Storyboard.Nodes[i].thumb_id, 512, 288);
										}
										else if (Storyboard.Nodes[i].type == STORYBOARD_TYPE_SCREEN && strcmp(Storyboard.Nodes[i].title, "Game Over Screen") == 0)
										{
											LoadImageSize("editors\\templates\\thumbs\\screen_lose.lua.png", Storyboard.Nodes[i].thumb_id, 512, 288);
										}
										else if (Storyboard.Nodes[i].type == STORYBOARD_TYPE_SCREEN && strcmp(Storyboard.Nodes[i].title, "Game Paused") == 0)
										{
											LoadImageSize("editors\\templates\\thumbs\\screen_gamemenu.lua.png", Storyboard.Nodes[i].thumb_id, 512, 288);
										}
										else if (Storyboard.Nodes[i].type == STORYBOARD_TYPE_SCREEN && strcmp(Storyboard.Nodes[i].title, "Load Game Screen") == 0)
										{
											LoadImageSize("editors\\templates\\thumbs\\screen_loadgame.lua.png", Storyboard.Nodes[i].thumb_id, 512, 288);
										}
										else if (Storyboard.Nodes[i].type == STORYBOARD_TYPE_SCREEN && strcmp(Storyboard.Nodes[i].title, "Save Game Screen") == 0)
										{
											LoadImageSize("editors\\templates\\thumbs\\screen_savegame.lua.png", Storyboard.Nodes[i].thumb_id, 512, 288);
										}
										else if (Storyboard.Nodes[i].type == STORYBOARD_TYPE_SCREEN && strcmp(Storyboard.Nodes[i].title, "Sound Settings Screen") == 0)
										{
											LoadImageSize("editors\\templates\\thumbs\\screen_sounds.lua.png", Storyboard.Nodes[i].thumb_id, 512, 288);
										}
										else if (Storyboard.Nodes[i].type == STORYBOARD_TYPE_SCREEN && strcmp(Storyboard.Nodes[i].title, "Graphics Settings Screen") == 0)
										{
											LoadImageSize("editors\\templates\\thumbs\\screen_graphics.lua.png", Storyboard.Nodes[i].thumb_id, 512, 288);
										}
										else if (Storyboard.Nodes[i].type == STORYBOARD_TYPE_SCREEN && strcmp(Storyboard.Nodes[i].title, "Controls Screen") == 0)
										{
											LoadImageSize("editors\\templates\\thumbs\\screen_controls.lua.png", Storyboard.Nodes[i].thumb_id, 512, 288);
										}
										// LB latest
										else if (Storyboard.Nodes[i].type == STORYBOARD_TYPE_HUD && strcmp(Storyboard.Nodes[i].title, "In-Game HUD") == 0)
										{
											LoadImageSize("editors\\templates\\thumbs\\hud.lua.png", Storyboard.Nodes[i].thumb_id, 512, 288);
										}
									}
								}

							}
						}
						image_setlegacyimageloading(false);
					}
				}
			}
			if (GImNodes)
			{
				//Reset grid.
				ImNodes::EditorContextResetPanning(Storyboard.vEditorPanning);
			}

			bStoryboardFirstRunSetInitPos = true;
		}

		//#### Init Done ####

		#ifdef INCLUDE_GAME_SETTINGS

		if (bEditGameSettings)
		{
			//PE: Cant use modal, as we use the objectlib.

			int iWindowWidth = 1100;
			int iWindowHeight = 600;
			float buttonwide = 200.0f;
			ImGui::SetNextWindowSize(ImVec2(iWindowWidth, iWindowHeight), ImGuiCond_Always);
			ImGui::SetNextWindowPosCenter(ImGuiCond_Once);
			bool bOpenWindow = true;

			if (ImGui::Begin("Edit Game Settings##Storyboard", &bEditGameSettings, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings))
			{
				//PE: Header look.
				ImGuiWindow* window = ImGui::GetCurrentWindow();
				float fToolbarHeight = 74.0f;
				ImVec4 style_headerback = ImGui::GetStyle().Colors[ImGuiCol_TitleBgActive];
				style_headerback.w = 1.0f;
				ImRect rHeader = { ImGui::GetWindowPos(), ImGui::GetWindowPos() + ImVec2(ImGui::GetWindowSize().x, fToolbarHeight) };
				if(window) window->DrawList->AddRectFilled(rHeader.Min, rHeader.Max, ImGui::GetColorU32(style_headerback), 0.0f, ImDrawCornerFlags_None);

				ImVec4 style_back = ImGui::GetStyle().Colors[ImGuiCol_WindowBg] * ImVec4(0.9, 0.9, 0.9, 0.9); //A Little darker.
				style_back.w = 1.0f;

				ImGui::Text("");
				ImGui::SetWindowFontScale(1.8);
				ImGui::TextCenter("Editing: Game Project Settings.");
				ImGui::SetWindowFontScale(1.0);
				ImGui::Text("");

				ImVec2 vCurPos = ImGui::GetCursorPos();
				ImVec2 vIconSize = { (float)ImGui::GetFontSize()*4.0f, (float)ImGui::GetFontSize()*4.0f };
				ImGui::SetCursorPos(ImVec2(3.0f, 7.0 + 1.0f));
				ImGui::SetItemAllowOverlap();
				if (ImGui::ImgBtn(TOOL_GOBACK, vIconSize, ImVec4(0, 0, 0, 0), drawCol_normal, drawCol_hover, drawCol_Down, 0, 0, 0, 0, false, false, false, false, false, bBoostIconColors))
				{
					bEditGameSettings = false;
				}
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", "Exit to Storyboard");
				ImGui::SetCursorPos(vCurPos);


				ImGui::Columns(2, "StoryboardWindowGameSettingsColumns", false);  //false no border
				ImGui::SetColumnOffset(0, 0.0f);
				ImGui::SetColumnOffset(1, (float)iWindowWidth * 0.75);


				//Storyboard.game_icon_id

				static int iChangeTab = 0;
				if (ImGui::BeginTabBar("GameSettingstabbar"))
				{
					int tabflags = 0;
					if (iChangeTab == 1)
					{
						iChangeTab = 0;
						tabflags = ImGuiTabItemFlags_SetSelected;
					}

					static bool bUpdateThumbImage = false;
					if (bUpdateThumbImage)
					{
						bUpdateThumbImage = false;
						if (ImageExist(Storyboard.game_thumb_id)) DeleteImage(Storyboard.game_thumb_id);
						if (strlen(Storyboard.game_thumb) > 0)
						{
							image_setlegacyimageloading(true);
							LoadImage(Storyboard.game_thumb, Storyboard.game_thumb_id);
							image_setlegacyimageloading(false);
						}
					}

					static bool bUpdateIconImage = false;
					if (bUpdateIconImage)
					{
						bUpdateIconImage = false;
						if (ImageExist(Storyboard.game_icon_id)) DeleteImage(Storyboard.game_icon_id);
						if (strlen(Storyboard.game_icon) > 0)
						{
							image_setlegacyimageloading(true);
							LoadImage(Storyboard.game_icon, Storyboard.game_icon_id);
							image_setlegacyimageloading(false);
						}
					}

					if (ImGui::BeginTabItem(" Thumbnail ", NULL, tabflags))
					{
						ImGui::SetWindowFontScale(1.4);
						ImGui::Text("");
						ImGui::TextCenter("Change Game Thumbnail");
						ImGui::Text("");
						ImGui::SetWindowFontScale(1.0);

						ImGui::TextCenter("For best result use a 16:9 ratio image like 1920Ã—1080.");

						ImVec2 cPos = ImVec2(ImGui::GetCursorPos() + ImVec2((ImGui::GetContentRegionAvail().x*0.5) - (512*0.5), 0.0f));
						ImGui::SetCursorPos(cPos);
						if (ImageExist(Storyboard.game_thumb_id))
						{
							//Need ratio here. fit to 288 height.
							ImGui::ImgBtn(Storyboard.game_thumb_id, ImVec2(512, 288), ImColor(0, 0, 0, 0), ImColor(255, 255, 255, 255), ImColor(255, 255, 255, 255), ImColor(255, 255, 255, 255), 0, 0, 0, 0, false, false, false);
						}
						else
						{
							if (window) window->DrawList->AddRectFilled(window->DC.CursorPos, window->DC.CursorPos + ImVec2(512, 288), ImGui::GetColorU32(style_back), 0.0f, ImDrawCornerFlags_None);
							ImGui::Dummy(ImVec2(512, 288));
						}

						cstr UniqueThumbnailSelect = "##StoryboardSelectThumbnail";
						if (iSelectedLibraryStingReturnID == window->GetID(UniqueThumbnailSelect.Get()))
						{
							strcpy(Storyboard.game_thumb, sSelectedLibrarySting.Get());
							bUpdateThumbImage = true; //Update thumb.
							sSelectedLibrarySting = "";
							iSelectedLibraryStingReturnID = -1; //disable.
							Storyboard.iChanged = true;
						}


						ImGui::SetWindowFontScale(1.4);
						ImGui::Text("");

						cPos = ImVec2(ImGui::GetCursorPos() + ImVec2((ImGui::GetContentRegionAvail().x*0.5) - (buttonwide*0.5), 0.0f));
						ImGui::SetCursorPos(cPos);

						if (ImGui::StyleButton("Select a New Image", ImVec2(buttonwide, 0.0f)))
						{
							bExternal_Entities_Window = true;
							iDisplayLibraryType = 2; //Image
							iLibraryStingReturnToID = window->GetID(UniqueThumbnailSelect.Get());
							if (strlen(Storyboard.game_thumb) > 0)
							{
								sMakeDefaultSelecting = Storyboard.game_thumb;
								bSelectLibraryViewAll = true;
							}
						}
						ImGui::SetWindowFontScale(1.0);

						ImGui::EndTabItem();
					}

					tabflags = 0;
					if (iChangeTab == 2)
					{
						iChangeTab = 0;
						tabflags = ImGuiTabItemFlags_SetSelected;
					}
					if (ImGui::BeginTabItem(" Description ", NULL, tabflags))
					{
						ImGui::SetWindowFontScale(1.4);
						ImGui::Text("");
						ImGui::TextCenter("Edit Game Description");
						ImGui::Text("");
						ImGui::SetWindowFontScale(1.0);

						ImVec2 cPos = ImVec2(ImGui::GetCursorPos() + ImVec2((ImGui::GetContentRegionAvail().x*0.5) - (512 * 0.5), 0.0f));
						ImGui::SetCursorPos(cPos);
						//PE: Need to be a little bit smaller then welcome screen that display description.

						ImGui::SetWindowFontScale(1.2);
						float width = 586.0; //480.0;
						if (ImGui::InputTextMultiline("##Game Description", &Storyboard.game_description[0], 2048, ImVec2(width, ImGui::GetFontSize()*19+12.0), 0, NULL, (void*)-1))
						{
							//PE: Simple Wrap input.

							char tmp[4096];
							char cmp[4096];
							strcpy(tmp, Storyboard.game_description);

							//PE: Get a avg. char wide.
							int wraparound = width / ((float) ImGui::CalcTextSize("abcdefghijkABCDEFGHIJK").x / 22.0);

							int len = strlen(tmp);
							int bBestMatch = -1;
							int count = 0;
							for (int i = 0; i < len; i++)
							{
								cmp[count] = tmp[i]; cmp[count + 1] = 0;

								if ( i > bBestMatch && tmp[i] == ' ') bBestMatch = i;
								if (tmp[i] == '\n') count = 0;

								if (count++ >= wraparound && (ImGui::CalcTextSize(cmp).x > (width - 18.0)) )
								{
									if (bBestMatch > 0)
									{
										tmp[bBestMatch] = '\n';
									}
									bBestMatch = -1;
									count = 0;
								}
							}
							strcpy(Storyboard.game_description, tmp);
							Storyboard.iChanged = true;
						}
						ImGui::SetWindowFontScale(1.0);
						ImGui::EndTabItem();
					}

					tabflags = 0;
					if (iChangeTab == 3)
					{
						iChangeTab = 0;
						tabflags = ImGuiTabItemFlags_SetSelected;
					}
					if (ImGui::BeginTabItem(" Edge of Game World ", NULL, tabflags))
					{
						ImGui::SetWindowFontScale(1.4);
						ImGui::Text("");
						ImGui::TextCenter("Edit Edge of Game World Message");
						ImGui::Text("");
						ImGui::SetWindowFontScale(1.0);

						// gameplayercontro.lua: Prompt ( "You cannot leave the area of play" )

						ImVec2 cPos = ImVec2(ImGui::GetCursorPos() + ImVec2((ImGui::GetContentRegionAvail().x*0.5) - (512 * 0.5), 0.0f));
						ImGui::SetCursorPos(cPos);

						ImGui::PushItemWidth(512);
						if (ImGui::InputText("##game_world_edge_textStoryboardInput", Storyboard.game_world_edge_text, 512, ImGuiInputTextFlags_None))
						{
							Storyboard.iChanged = true;
						}
						ImGui::PopItemWidth();

						ImGui::EndTabItem();
					}

					tabflags = 0;
					if (iChangeTab == 6)
					{
						iChangeTab = 0;
						tabflags = ImGuiTabItemFlags_SetSelected;
					}
					if (ImGui::BeginTabItem(" Key Bindings ", NULL, tabflags))
					{
						ImGui::SetWindowFontScale(1.4);
						ImGui::Text("");
						ImGui::TextCenter("Modify Global Key Bindings");
						ImGui::Text("");
						ImVec2 cPos = ImVec2(ImGui::GetCursorPos() + ImVec2((ImGui::GetContentRegionAvail().x * 0.5) - (200), 0.0f));

						ImGui::SetWindowFontScale(1.2);
						int buttonwide = 200;
						int iDefKey = 0;
						char pButtonName[256];
						char pBindingDesc[256];
						for (int keyi = 0; keyi < 8; keyi++)
						{
							if (keyi == 0)      { iDefKey = 17;  sprintf(pButtonName, "Move Forward Key"); sprintf(pBindingDesc, "[W] Mapped To Scancode %d", g.keymap[iDefKey]); }
							else if (keyi == 1) { iDefKey = 31;  sprintf(pButtonName, "Move Backward Key"); sprintf(pBindingDesc, "[S] Mapped To Scancode %d", g.keymap[iDefKey]); }
							else if (keyi == 2) { iDefKey = 30;  sprintf(pButtonName, "Move Left Key"); sprintf(pBindingDesc, "[A] Mapped To Scancode %d", g.keymap[iDefKey]); }
							else if (keyi == 3) { iDefKey = 32;  sprintf(pButtonName, "Move Right Key"); sprintf(pBindingDesc, "[D] Mapped To Scancode %d", g.keymap[iDefKey]); }
							else if (keyi == 4) { iDefKey = 18;  sprintf(pButtonName, "Action Key"); sprintf(pBindingDesc, "[E] Mapped To Scancode %d", g.keymap[iDefKey]); }
							else if (keyi == 5) { iDefKey = 46;  sprintf(pButtonName, "Crouch Key"); sprintf(pBindingDesc, "[C] Mapped To Scancode %d", g.keymap[iDefKey]); }
							else if (keyi == 6) { iDefKey = 42;  sprintf(pButtonName, "Run Key"); sprintf(pBindingDesc, "[SHIFT] Mapped To Scancode %d", g.keymap[iDefKey]); }
							else if (keyi == 7) { iDefKey = 57;  sprintf(pButtonName, "Jump Key"); sprintf(pBindingDesc, "[SPACE] Mapped To Scancode %d", g.keymap[iDefKey]); }
							ImGui::SetCursorPos(ImVec2(cPos.x, ImGui::GetCursorPos().y));
							if (ImGui::StyleButton(pButtonName, ImVec2(buttonwide, 0.0f))) 
							{ 
								g_iMappingKeyToChange = iDefKey;
								g_bMappingKeyWindow = true;
							}
							bool bModified = false;	if (g.keymap[iDefKey] != iDefKey) bModified = true;
							ImGui::SameLine();
							ImGui::SetCursorPos(ImVec2(cPos.x + buttonwide + 20.0f, ImGui::GetCursorPos().y));
							if(bModified) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.8f, 1.0f));
							ImGui::Text(pBindingDesc);
							if (bModified) ImGui::PopStyleColor();
						}
						ImGui::SetWindowFontScale(1.0);
						ImGui::EndTabItem();
					}

					tabflags = 0;
					if (iChangeTab == 4)
					{
						iChangeTab = 0;
						tabflags = ImGuiTabItemFlags_SetSelected;
					}
					if (strlen(Storyboard.gamename) > 0)
					{
						if (ImGui::BeginTabItem(" Icon ", NULL, tabflags))
						{
							cstr UniqueIconnailSelect = "##StoryboardSelectIconnail";
							if (iSelectedLibraryStingReturnID == window->GetID(UniqueIconnailSelect.Get()))
							{
								strcpy(Storyboard.game_icon, sSelectedLibrarySting.Get());
								sSelectedLibrarySting = "";
								iSelectedLibraryStingReturnID = -1; //disable.
								Storyboard.iChanged = true;

								if (ImageExist(Storyboard.game_icon_id)) DeleteImage(Storyboard.game_icon_id);
								if (strlen(Storyboard.game_icon) > 0)
								{
									image_setlegacyimageloading(true);
									LoadImageSize(Storyboard.game_icon, Storyboard.game_icon_id, 256, 256);
									image_setlegacyimageloading(false);
								}

								if (ImageExist(Storyboard.game_icon_id))
								{
									//Save and convert to ico.

									char projectpng[MAX_PATH];
									strcpy(projectpng, Storyboard.game_icon);
									GG_GetRealPath(projectpng, 0);

									//ffmpeg -i maxicon.png -vf scale=256:256 maxicon.ico

									extern char g_pAbsPathToConverter[MAX_PATH];
									std::string process_name = g_pAbsPathToConverter;
									replaceAll(process_name, "\\Guru-Converter.exe", "\\ffmpeg.exe");

									DARKSDK BOOL DB_ExecuteFile(HANDLE* phExecuteFileProcess, char* Operation, char* Filename, char* String, char* Path, bool bWaitForTermination);
									char parameters[MAX_PATH];

									::SetCursor(::LoadCursor(NULL, IDC_WAIT));

									for (int a = 0; a < 1; a++) //PE: (a < 6) for now only use 256x256
									{

										strcpy(parameters, "-i \"");
										strcat(parameters, projectpng);

										//Delete old ico.
										char projectico[MAX_PATH];
										strcpy(projectico, "projectbank\\");
										strcat(projectico, Storyboard.gamename);

										//PE: in MAX 256x256 is 256 colors only, so use 128x128 as main icon.
										if (a == 0)
										{
											strcat(parameters, "\" -vf scale=256:256 \"");
											strcat(projectico, "\\project256.ico");
										}
										else if (a == 1)
										{
											strcat(parameters, "\" -vf scale=128:128 \"");
											strcat(projectico, "\\project128.ico");
										}
										else if (a == 2)
										{
											strcat(parameters, "\" -vf scale=64:64 \"");
											strcat(projectico, "\\project64.ico");
										}
										else if (a == 3)
										{
											strcat(parameters, "\" -vf scale=48:48 \"");
											strcat(projectico, "\\project32.ico");
										}
										else if (a == 4)
										{
											strcat(parameters, "\" -vf scale=32:32 \"");
											strcat(projectico, "\\project32.ico");
										}
										else
										{
											strcat(parameters, "\" -vf scale=16:16 \"");
											strcat(projectico, "\\project16.ico");
										}

										GG_GetRealPath(projectico, 1);
										DeleteFileA(projectico);

										strcat(parameters, projectico);
										strcat(parameters, "\"");

										HANDLE g_hConvertPngToIcoProcess = NULL;

										DB_ExecuteFile(&g_hConvertPngToIcoProcess, "hide", (char *)process_name.c_str(), parameters, "", true);
										int iRunning = 1;
										int timeout = 0;
										while (iRunning == 1)
										{
											iRunning = 0;
											DWORD dwStatus;
											if (GetExitCodeProcess(g_hConvertPngToIcoProcess, &dwStatus) == TRUE)
											{
												if (dwStatus == STILL_ACTIVE)
												{
													iRunning = 1;
													Sleep(1);
													if (timeout++ > 4000) iRunning = 0; //Timeout
												}
											}
										}
										CloseHandle(g_hConvertPngToIcoProcess);
										Sleep(10);
									}
									::SetCursor(::LoadCursor(NULL, IDC_ARROW));
								}
							}

							ImGui::SetWindowFontScale(1.4);
							ImGui::Text("");
							ImGui::TextCenter("Edit Game Executable Icon");
							ImGui::Text("");
							ImGui::SetWindowFontScale(1.0);

							ImGui::TextCenter("For best result use a 256x256 png image.");

							ImVec2 cPos = ImVec2(ImGui::GetCursorPos() + ImVec2((ImGui::GetContentRegionAvail().x*0.5) - (256 * 0.5), 0.0f));
							ImGui::SetCursorPos(cPos);
							if (ImageExist(Storyboard.game_icon_id))
							{
								//Need ratio here. fit to 288 height.
								ImGui::ImgBtn(Storyboard.game_icon_id, ImVec2(256, 256), ImColor(0, 0, 0, 0), ImColor(255, 255, 255, 255), ImColor(255, 255, 255, 255), ImColor(255, 255, 255, 255), 0, 0, 0, 0, false, false, false);
							}
							else
							{
								if (window) window->DrawList->AddRectFilled(window->DC.CursorPos, window->DC.CursorPos + ImVec2(256, 256), ImGui::GetColorU32(style_back), 0.0f, ImDrawCornerFlags_None);
								ImGui::Dummy(ImVec2(256, 256));
							}

							ImGui::SetWindowFontScale(1.4);
							ImGui::Text("");

							cPos = ImVec2(ImGui::GetCursorPos() + ImVec2((ImGui::GetContentRegionAvail().x*0.5) - (buttonwide*0.5), 0.0f));
							ImGui::SetCursorPos(cPos);

							if (ImGui::StyleButton("Select a New Icon", ImVec2(buttonwide, 0.0f)))
							{
								bExternal_Entities_Window = true;
								iDisplayLibraryType = 2; //Image
								iLibraryStingReturnToID = window->GetID(UniqueIconnailSelect.Get());
								if (strlen(Storyboard.game_icon) > 0)
								{
									sMakeDefaultSelecting = Storyboard.game_icon;
									bSelectLibraryViewAll = true;
								}
							}
							ImGui::SetWindowFontScale(1.0);
							ImGui::EndTabItem();
						}
					}
					tabflags = 0;
					if (iChangeTab == 5)
					{
						iChangeTab = 0;
						tabflags = ImGuiTabItemFlags_SetSelected;
					}
					if (ImGui::BeginTabItem(" Game Developer ", NULL, tabflags))
					{
						ImGui::SetWindowFontScale(1.4);
						ImGui::Text("");
						ImGui::TextCenter("Edit Game Developer Description");
						ImGui::Text("");
						ImGui::SetWindowFontScale(1.0);

						ImVec2 cPos = ImVec2(ImGui::GetCursorPos() + ImVec2((ImGui::GetContentRegionAvail().x*0.5) - (512 * 0.5), 0.0f));
						ImGui::SetCursorPos(cPos);

						ImGui::SetWindowFontScale(1.2);
						float width = 586.0; //480.0;

						if (ImGui::InputTextMultiline("##Developer Description", &Storyboard.game_developer_desc[0], 1023, ImVec2(width, ImGui::GetFontSize() * 10 + 12.0), 0, NULL, (void*)-1))
						{
							char tmp[4096];
							char cmp[4096];
							strcpy(tmp, Storyboard.game_developer_desc);

							//PE: Get a avg. char wide.
							int wraparound = width / ((float)ImGui::CalcTextSize("abcdefghijkABCDEFGHIJK").x / 22.0);

							int len = strlen(tmp);
							int bBestMatch = -1;
							int count = 0;
							for (int i = 0; i < len; i++)
							{
								cmp[count] = tmp[i]; cmp[count + 1] = 0;

								if (i > bBestMatch && tmp[i] == ' ') bBestMatch = i;
								if (tmp[i] == '\n') count = 0;

								if (count++ >= wraparound && (ImGui::CalcTextSize(cmp).x > (width - 18.0)))
								{
									if (bBestMatch > 0)
									{
										tmp[bBestMatch] = '\n';
									}
									bBestMatch = -1;
									count = 0;
								}
							}
							strcpy(Storyboard.game_developer_desc, tmp);
							Storyboard.iChanged = true;
						}
						ImGui::SetWindowFontScale(1.0);
						ImGui::EndTabItem();
					}
					ImGui::EndTabBar();
				}
				ImGui::NextColumn();

				ImGui::Text("");

				if (ImGui::StyleCollapsingHeader("Game Description", ImGuiTreeNodeFlags_DefaultOpen) || iStoryboardExecuteKey != 0) //"Add New"
				{
					ImGui::Indent(10);
					ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((ImGui::GetContentRegionAvail().x*0.5) - (buttonwide*0.5), 0.0f));
					if (ImGui::StyleButton("Change Game Thumbnail", ImVec2(buttonwide, 0.0f)))
					{
						iChangeTab = 1;
					}
					ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((ImGui::GetContentRegionAvail().x*0.5) - (buttonwide*0.5), 0.0f));
					if (ImGui::StyleButton("Edit Text Description", ImVec2(buttonwide, 0.0f)))
					{
						iChangeTab = 2;
					}
					ImGui::Indent(-10);
				}
				if (ImGui::StyleCollapsingHeader("In-Game Settings", ImGuiTreeNodeFlags_DefaultOpen) || iStoryboardExecuteKey != 0) //"Add New"
				{
					ImGui::Indent(10);
					ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((ImGui::GetContentRegionAvail().x*0.5) - (buttonwide*0.5), 0.0f));
					if (ImGui::StyleButton("Edge of Game World Message", ImVec2(buttonwide, 0.0f)))
					{
						iChangeTab = 3;
					}
					ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((ImGui::GetContentRegionAvail().x * 0.5) - (buttonwide * 0.5), 0.0f));
					if (ImGui::StyleButton("Key Bindings", ImVec2(buttonwide, 0.0f)))
					{
						iChangeTab = 6;
					}
					ImGui::Indent(-10);
				}
				if (strlen(Storyboard.gamename) > 0)
				{
					if (ImGui::StyleCollapsingHeader("Export Settings", ImGuiTreeNodeFlags_DefaultOpen) || iStoryboardExecuteKey != 0) //"Add New"
					{
						ImGui::Indent(10);
						ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((ImGui::GetContentRegionAvail().x*0.5) - (buttonwide*0.5), 0.0f));
						if (ImGui::StyleButton("Game Executable Icon", ImVec2(buttonwide, 0.0f)))
						{
							iChangeTab = 4;
						}
						ImGui::Indent(-10);
					}
				}
				if (ImGui::StyleCollapsingHeader("Developer Description", ImGuiTreeNodeFlags_DefaultOpen) || iStoryboardExecuteKey != 0) //"Add New"
				{
					ImGui::Indent(10);
					ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((ImGui::GetContentRegionAvail().x * 0.5) - (buttonwide * 0.5), 0.0f));
					if (ImGui::StyleButton("Game Developer Description", ImVec2(buttonwide, 0.0f)))
					{
						iChangeTab = 5;
					}
					ImGui::Indent(-10);
				}

				ImGui::Text("");
				bool bTmp = 1 - Storyboard.project_inactive;
				if (ImGui::Checkbox("Active/InActive Project", &bTmp))
				{
					Storyboard.project_inactive = 1 - bTmp;
					Storyboard.iChanged = true;
					//PE: Check if we have a current list and update.
					if (projectbank_list.size() > 0)
					{
						for (int i = 0; i < projectbank_list.size(); i++)
						{
							if (stricmp(Storyboard.gamename, projectbank_list[i].c_str()) == NULL )
							{
								if(Storyboard.project_inactive)
									projectbank_active[i] = false;
								else
									projectbank_active[i] = true;
								break;
							}
						}
					}
				}

				ImGui::EndColumns();

				bImGuiGotFocus = true;
				ImGui::Indent(-10);
				bBlockNextMouseCheck = true;
			}
			ImGui::End();

			if (!bOpenWindow)
			{
				bEditGameSettings = false;
			}
		}
		#endif

		// handle capture of new key for key binding
		if (g_bMappingKeyWindow)
		{
			bImGuiGotFocus = true;
			ImGui::SetNextWindowPosCenter(ImGuiCond_Always);
			ImGui::SetNextWindowSize(ImVec2(512, 256));
			ImGui::OpenPopup("##mappingkeypopup");
			ImGui::BeginPopupModal("##mappingkeypopup", 0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse);
			ImGui::Text("");
			ImGui::Text("");
			ImGui::Text("");
			ImGui::Text("");
			ImGui::TextCenter("Press the new key that you would like to use for this action.");
			ImGui::Text("");
			ImGui::TextCenter("Press ESCAPE to reset selection back to default.");
			ImGuiStyle& style = ImGui::GetStyle();
			ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 1.0f, 1.0f, 0.54f));
			t.inputsys.kscancode = ScanCode();
			if (t.inputsys.kscancode > 0)
			{
				if (g_iMappingKeyToChange > 0)
				{
					// make change in keymap
					if (t.inputsys.kscancode != 57)
					{
						// use that new key
						g.keymap[g_iMappingKeyToChange] = t.inputsys.kscancode;
					}
					else
					{
						// reset key to default
						g.keymap[g_iMappingKeyToChange] = g_iMappingKeyToChange;
					}

					// save this change out to relevant file (Files\editors\keymap\custom.ini)
					cstr pOldDir = GetDir();
					char pWritableKeyMapFile[MAX_PATH];
					strcpy(pWritableKeyMapFile, "editors\\keymap\\custom.ini");
					GG_GetRealPath(pWritableKeyMapFile, 1);
					if (FileExist(pWritableKeyMapFile) == 1) DeleteFileA(pWritableKeyMapFile);
					OpenToWrite(1, pWritableKeyMapFile);
					WriteString(1, ";Key binding created in Storyboard Key Bindings section");
					WriteString(1, "");
					WriteString(1, "[KEYMAP]");
					char pKeyBindingLine[256];
					for (int i = 0; i < 256; i++)
					{
						if (g.keymap[i] != i)
						{
							sprintf(pKeyBindingLine, "key%d = %d", i, g.keymap[i]);
							WriteString(1, pKeyBindingLine);
						}
					}
					CloseFile(1);

					// finished binding
					g_iMappingKeyToChange = -1;
				}
				ImGui::CloseCurrentPopup();
				g_bMappingKeyWindow = false;
			}
			ImGui::PopStyleColor();
			ImGui::Text("");
			ImGui::EndPopup();
		}

		ImVec2 viewPortPos = ImGui::GetMainViewport()->Pos;
		ImVec2 viewPortSize = ImGui::GetMainViewport()->Size;

		static ImVec2 old_viewPortSize = { -1,-1 };
		if (old_viewPortSize.x != viewPortSize.x || old_viewPortSize.y != viewPortSize.y)
		{
			ImGui::SetNextWindowPos(viewPortPos, ImGuiCond_Always);
			ImGui::SetNextWindowSize(viewPortSize, ImGuiCond_Always);
			old_viewPortSize = viewPortSize;
		}
		else if (bModal)
		{
			ImGui::SetNextWindowPos(viewPortPos);
			ImGui::SetNextWindowSize(viewPortSize);
		}
		else
		{
			ImGui::SetNextWindowPos(viewPortPos, ImGuiCond_Once);
			ImGui::SetNextWindowSize(viewPortSize, ImGuiCond_Once);
		}

		//ImGuiWindowFlags_MenuBar , ImGuiWindowFlags_NoTitleBar, ImGuiWindowFlags_NoBringToFrontOnFocus
		if (bModal)
		{
			bPopModalStoryboard = ImGui::BeginPopupModal("##StoryboardWindow", &bStoryboardWindow, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoResize); // ImGuiWindowFlags_NoScrollbar
		}
		else
		{
			ImVec4 style_winback = ImGui::GetStyle().Colors[ImGuiCol_WindowBg];
			style_winback.w = 1.0f;
			ImGui::PushStyleColor(ImGuiCol_WindowBg, style_winback);
			int flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
			if (!bScreen_Editor_Window) flags |= ImGuiWindowFlags_MenuBar;
			bPopModalStoryboard = ImGui::Begin("##StoryboardWindow", &bStoryboardWindow, flags); // ImGuiWindowFlags_NoScrollbar
		}
		if (bPopModalStoryboard)
		{

			if (!bProceduralLevel)
			{
				ImGuiWindow* storyboard_window = ImGui::GetCurrentWindow();
				CheckWindowsOnTop(storyboard_window);
			}

			if (bScreen_Editor_Window)
			{
				screen_editor(iScreen_Editor_Node);
			}
			else
			{
				storyboard_menubar(preview_size_x, fNodeWidth, fNodeHeight);

				float fStartWinPosY = ImGui::GetCursorPosY();

				ImGui::Columns(2, "StoryboardWindowColumns", false);  //false no border
				ImGui::SetColumnOffset(0, 0.0f);
				ImGui::SetColumnOffset(1, preview_size_x);
				ImNodesContext* gImNodes = ImNodes::GetCurrentContext();
				if (gImNodes)
				{
					gImNodes->Style.PinOffset = 6.0f;
					gImNodes->Style.PinTriangleSideLength = ImGui::GetFontSize() - 2.0;
					gImNodes->Style.PinQuadSideLength = ImGui::GetFontSize() - 4.0;
					if (bStoryboardFirstRunSetInitPos)
						Storyboard.vEditorPanning = ImNodes::EditorContextGetPanning();
				}
				static ImVec2 vTooltipPos;
				static cstr sTooltip = "";

				/* old storyboard banner replaced with more modern header
				float fRatio = preview_size_x / 1200.0f;
				float fHeaderHeight = g_Storyboard_header_height * fRatio;
				ID3D11ShaderResourceView* lpTexture;
				lpTexture = GetImagePointerView(STORYBOARD_HEADER);
				ImVec2 vHeaderDim = { (float)preview_size_x, fHeaderHeight };
				static ImVec4 fade_heading = ImVec4(1.0, 1.0, 1.0, 1.0); //New header no fading.
				if (lpTexture)
				{
					ImGuiWindow* window = ImGui::GetCurrentWindow();
					ImVec2 header_pos = ImGui::GetWindowPos() + ImVec2(0.0, fStartWinPosY);
					window->DrawList->AddImage((ImTextureID)lpTexture, header_pos, header_pos + vHeaderDim, ImVec2(0, 0), ImVec2(1, 1), ImGui::GetColorU32(fade_heading));
				}	
				ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(0, fStartWinPosY + 6.0f));
				ImVec2 vCurPos = ImGui::GetCursorPos();
				*/
				ImGui::SetWindowFontScale(1.0);
				ImVec2 vIconSize = { (float)ImGui::GetFontSize() * 3.5f, (float)ImGui::GetFontSize() * 3.5f };
				ImVec2 vHeaderDim = { (float)preview_size_x, vIconSize.y };
				float fHeaderHeight = vHeaderDim.y;
				ID3D11ShaderResourceView* lpTexture;
				lpTexture = GetImagePointerView(STORYBOARD_HEADER);
				if (lpTexture)
				{
					ImGuiWindow* window = ImGui::GetCurrentWindow();
					ImVec2 header_pos = ImGui::GetWindowPos() + ImVec2(0.0, fStartWinPosY);
					window->DrawList->AddImage((ImTextureID)lpTexture, header_pos, header_pos + vHeaderDim, ImVec2(0, 0), ImVec2(1, 1), ImGui::GetColorU32(ImVec4(1.0, 1.0, 1.0, 1.0)));
				}
				ImGui::SetCursorPos(ImVec2(4.0f, fStartWinPosY - 1.0f));
				ImGui::SetItemAllowOverlap();
				if (pref.iDisplayWelcomeScreen != 0)
				{
					// now only allows backing from storyboard if have a HUB to go to
					if (ImGui::ImgBtn(TOOL_GOBACK, vIconSize, ImVec4(0, 0, 0, 0), drawCol_normal, drawCol_hover, drawCol_Down, 0, 0, 0, 0, false, false, false, false, false, bBoostIconColors))
					{
						bool bAbort = false;
						if (Storyboard.iChanged)
						{
							if (!pref.iDisableProjectAutoSave && strlen(Storyboard.gamename) > 0)
							{
								save_storyboard(Storyboard.gamename, false);
							}
							else
							{
								int iAction = askBoxCancel(STORYBOARD_SAVE_MESSAGE, "Confirmation"); //1==Yes 2=Cancel 0=No
								if (iAction == 1)
								{
									//Save.
									if (strlen(Storyboard.gamename) > 0)
										save_storyboard(Storyboard.gamename, false);
									else
									{
										bAbort = true;
										save_storyboard(Storyboard.gamename, true);
									}
								}
							}
						}
						if (!bAbort)
						{
							iLevelEditorFromStoryboardID = -1;
							//Back to welcome.
							bWelcomeScreen_Window = true;
							bStoryboardWindow = false;
							cLastProjectList = ""; //Trigger a reload of projects, if anything changed.
							bSortProjects = true;

							// and in case this was a remote project, restore to writables regular
							extern void switch_to_regular_projects(void);
							switch_to_regular_projects();
						}
					}
					if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", "Exit to Hub"); //Welcome Screen
				}

				ImGui::SetWindowFontScale(2.0); //1.4
				ImGui::TextCenter(""); //New header already have this text.

				//PE: changed inputtext to just displying the project name.
				ImGui::SetWindowFontScale(1.0);
				ImGui::Text("");
				ImGui::SetWindowFontScale(2.0);
				ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), fHeaderHeight * 0.70));
				ImGui::TextCenter(Storyboard.gamename);
				ImGui::SetWindowFontScale(1.0);

				ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPos().x, fStartWinPosY + fHeaderHeight + 6.0f));

				if (sTooltip != "")
				{
					ImVec2 cposback = ImGui::GetCursorPos();
					ImGui::SetCursorPos(vTooltipPos);
					ImGui::SetTooltip(sTooltip.Get());
					sTooltip = "";
					ImGui::SetCursorPos(cposback);
				}


				ImVec2 vNodeAreaStart = ImGui::GetCursorScreenPos();

				//PE: Make sure if we focus another window and go back , mouse dragging is reset.
				extern bool g_bAppActiveStat;
				const ImGuiIO& io = ImGui::GetIO();
				if (!g_bAppActiveStat)
				{
					ImGuiContext& g = *GImGui;
					GImGui->IO.MousePos = ImVec2(-10000, -10000);
					g.IO.MouseDelta = ImVec2(0, 0);
					g_bAppActiveStat = true;
				}

				// can duplicate screen, but only do it outside the node loop
				int iTriggerDuplicateOutsideOfLoop = -1;

				ImNodes::BeginNodeEditor();

				ImNodes::PushAttributeFlag(ImNodesAttributeFlags_EnableLinkDetachWithDragClick);

				for (int i = 0; i < STORYBOARD_MAXNODES; i++)
				{
					bool bValid = true;

					if (!pref.iStoryboardAdvanced && Storyboard.Nodes[i].used)
					{
						//PE: Hide nodes if not in advanced.
						if (i == iLoadGameNodeID ||
							i == iGamePausedNodeID ||
							i == iSaveGameNodeID ||
							i == iGraphicsNodeID ||
							i == iControlNodeID ||
							i == iSoundsNodeID)
							bValid = false;
					}

					if (bValid && Storyboard.Nodes[i].used)
					{
						//PE: Store current location for restore/save.
						if (bStoryboardFirstRunSetInitPos)
							Storyboard.Nodes[i].restore_position = ImNodes::GetNodeGridSpacePos(Storyboard.Nodes[i].id);

						//PE: Setup node.
						ImNodes::BeginNode(Storyboard.Nodes[i].id);

						//#### Title bar ####
						ImNodes::BeginNodeTitleBar();
						ImGui::Dummy(ImVec2(fNodeWidth, 0));
						ImVec2 cpos = ImGui::GetCursorPos();
						ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(0.0, -3.0));

						ImVec2 text_size = ImGui::CalcTextSize(Storyboard.Nodes[i].title);
						if (text_size.x > fNodeWidth - 50.0)
						{
							char tmp[255];
							strncpy(tmp, Storyboard.Nodes[i].title, 19);
							tmp[19] = 0;
							strcat(tmp, "...");
							ImGui::TextUnformatted(tmp);
							if (ImGui::IsItemHovered())
							{
								vTooltipPos = ImGui::GetCursorPos();
								sTooltip = Storyboard.Nodes[i].title;
							}
						}
						else
						{
							ImGui::TextUnformatted(Storyboard.Nodes[i].title);
						}

						if (Storyboard.Nodes[i].type == STORYBOARD_TYPE_LEVEL && !pestrcasestr(Storyboard.Nodes[i].lua_name, "loading"))
						{
							//Reverse push's from BeginNodeEditor
							ImGui::PopStyleColor(); // pop child window background color
							ImGui::PopStyleVar();   // pop window padding
							ImGui::PopStyleVar();   // pop frame padding

							//Combo.
							const char* items_storyboard_level[] = { "Duplicate Level", "Rename Level", "Delete Level", "Take Screenshot", "Take Map Snapshot" };
							ImGui::SetCursorPos(ImVec2(cpos.x + fNodeWidth - 48.0f, cpos.y - 8.0));
							int selection = 0;
							char iUniqueString[255];
							sprintf(iUniqueString, "##ComboStoryboardLevels%d", i);

							int iComboEntries = 3;
							if (strlen(Storyboard.Nodes[i].level_name) > 0)
							{
								// Only screenshot if we got a level.
								iComboEntries = 5;
							}

							int comboflags = ImGuiComboFlags_NoPreview | ImGuiComboFlags_PopupAlignLeft | ImGuiComboFlags_HeightLarge;
							ImGui::PushItemWidth(20);
							if (ImGui::BeginCombo(iUniqueString, "", comboflags))
							{
								for (int n = 0; n < iComboEntries; n++)
								{
									//PE: Only display rename and duplicate if we got a name.
									if (!(iComboEntries == 3 && n <= 1))
									{
										if (ImGui::Selectable(items_storyboard_level[n], false))
										{
											Storyboard.iChanged = true;
											bBlockNextMouseCheck = true;
											selection = n;
											if (selection == 0)
											{
												//Duplicate Level.
												iDuplicateNode = i;
												bDuplicateLevel = true;

												//Load in old level if any.
												if (strlen(Storyboard.Nodes[i].level_name) > 0)
												{
													//Load and edit.
													if (stricmp(Storyboard.Nodes[i].level_name, g.projectfilename_s.Get()) == NULL)
													{
														//Already open , just start dup screen.
													}
													else
													{
														//Load level and exit to level editor.
														strcpy(cDirectOpen, Storyboard.Nodes[i].level_name);
														iLaunchAfterSync = 7; //Direct load.
														iSkibFramesBeforeLaunch = 3;
													}
												}
												strcpy(DuplicateLevelName, Storyboard.Nodes[i].title);
												strcat(DuplicateLevelName, " (copy)");
												strcpy(DuplicateLevelError, "");

											}
											if (selection == 1)
											{
												//Rename Level
												if (strlen(Storyboard.Nodes[i].level_name) > 0)
												{
													bRenameLevel = true;
													iRenameNode = i;

													std::string sLevelTitle = Storyboard.Nodes[i].level_name;
													replaceAll(sLevelTitle, ".fpm", "");
													replaceAll(sLevelTitle, "mapbank\\", "");

													char destination[MAX_PATH];
													strcpy(destination, Storyboard.Nodes[i].level_name);
													GG_GetRealPath(destination, 0); //Resolve name. need full path.
													strcpy(RenameOriginalLevelName, destination);
													strcpy(RenameLevelName, sLevelTitle.c_str());
													strcpy(RenameLevelError, "");
												}

											}
											if (selection == 2)
											{
												//Delete Level
												int iAction = askBoxCancel("This will delete the level from your storyboard, are you sure?", "Confirmation"); //1==Yes 2=Cancel 0=No
												if (iAction == 1)
												{
													//Delete any links to this level.
													for (int il = 0; il < STORYBOARD_MAXNODES; il++)
													{
														if (Storyboard.Nodes[il].used)
														{
															for (int ll = 0; ll < STORYBOARD_MAXOUTPUTS; ll++)
															{
																if (Storyboard.Nodes[il].output_linkto[ll] > 0)
																{
																	for (int a = 0; a < STORYBOARD_MAXOUTPUTS; a++)
																	{
																		//Check all inputs.
																		if (Storyboard.Nodes[i].input_id[a] > 0)
																		{
																			if (Storyboard.Nodes[il].output_linkto[ll] == Storyboard.Nodes[i].input_id[a])
																				Storyboard.Nodes[il].output_linkto[ll] = 0;
																		}
																	}
																}
															}
														}
													}
													reset_single_node(i);
													Storyboard.Nodes[i].used = false;
													bBlockNextMouseCheck = true;
												}
											}
											if (selection == 3 || selection == 4)
											{
												//Take Screenshot.
												extern bool bPopModalOpenProceduralCameraMode;
												bPopModalOpenProceduralCameraMode = true;

												// option to snap a map snapshot
												extern bool bPopModalTakeMapSnapshot;
												if (selection == 3) bPopModalTakeMapSnapshot = false;
												if (selection == 4) bPopModalTakeMapSnapshot = true;

												//PE: Do we need to load this level ?
												if (strlen(Storyboard.Nodes[i].level_name) > 0)
												{
													//Load and edit.
													if (stricmp(Storyboard.Nodes[i].level_name, g.projectfilename_s.Get()) == NULL)
													{
														//Already open , just start screenshot
													}
													else
													{
														//Load level and start screenshot.
														strcpy(cDirectOpen, Storyboard.Nodes[i].level_name);
														iLaunchAfterSync = 7; //Direct load.
														iSkibFramesBeforeLaunch = 5;
													}
												}

												iWaitForNewScreenshot = 10;
												iScreenshotNode = i;

												//Make sure we have a fresh thumb.
												GG_SetWritablesToRoot(true);
												if (FileExist("thumbbank\\lastnewlevel.jpg")) DeleteAFile("thumbbank\\lastnewlevel.jpg");
												GG_SetWritablesToRoot(false);
											}
										}
									}
								}
								ImGui::EndCombo();
							}
							if (ImGui::IsItemHovered()) 
							{
								bBlockNextMouseCheck = true;
								vTooltipPos = ImGui::GetCursorPos();
								sTooltip = " Duplicate, delete or take a screen shot ";
							}
							ImGui::PopItemWidth();

							//Add push to BeginNodeEditor.
							ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1.f, 1.f));
							ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
							ImGui::PushStyleColor(ImGuiCol_ChildBg, GImNodes->Style.Colors[ImNodesCol_GridBackground]);
						}

						if (Storyboard.Nodes[i].type == STORYBOARD_TYPE_LEVEL && pestrcasestr(Storyboard.Nodes[i].lua_name, "loading") && !pestrcasestr(Storyboard.Nodes[i].lua_name, "loading.lua") )
						{
							ImGui::PopStyleColor();
							ImGui::PopStyleVar();
							ImGui::PopStyleVar();
							const char* items_storyboard_hud[] = { "Delete Loading Screen" };
							ImGui::SetCursorPos(ImVec2(cpos.x + fNodeWidth - 48.0f, cpos.y - 8.0));
							int selection = 0;
							char iUniqueString[255];
							sprintf(iUniqueString, "##ComboStoryboardCustom%d", i);
							int iComboEntries = 1;
							int comboflags = ImGuiComboFlags_NoPreview | ImGuiComboFlags_PopupAlignLeft | ImGuiComboFlags_HeightLarge;
							ImGui::PushItemWidth(20);
							if (ImGui::BeginCombo(iUniqueString, "", comboflags))
							{
								for (int n = 0; n < iComboEntries; n++)
								{
									if (ImGui::Selectable(items_storyboard_hud[n], false))
									{
										Storyboard.iChanged = true;
										bBlockNextMouseCheck = true;
										selection = n;
										if (selection == 0)
										{
											int iAction = askBoxCancel("This will delete the loading screen from your storyboard, are you sure?", "Confirmation"); //1==Yes 2=Cancel 0=No
											if (iAction == 1)
											{
												reset_single_node(i);
												Storyboard.Nodes[i].used = false;
												bBlockNextMouseCheck = true;
											}
										}
									}
								}
								ImGui::EndCombo();
							}
							if (ImGui::IsItemHovered())
							{
								bBlockNextMouseCheck = true;
								vTooltipPos = ImGui::GetCursorPos();
								sTooltip = " Delete Loading Screen ";
							}
							ImGui::PopItemWidth();
							ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1.f, 1.f));
							ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
							ImGui::PushStyleColor(ImGuiCol_ChildBg, GImNodes->Style.Colors[ImNodesCol_GridBackground]);

						}

						bool bIsCustomScreen = pestrcasestr(Storyboard.Nodes[i].title, "Custom Screen");
						if (bIsCustomScreen)
						{
							ImGui::PopStyleColor();
							ImGui::PopStyleVar();
							ImGui::PopStyleVar();
							const char* items_storyboard_hud[] = { "Delete Custom Screen" };
							ImGui::SetCursorPos(ImVec2(cpos.x + fNodeWidth - 48.0f, cpos.y - 8.0));
							int selection = 0;
							char iUniqueString[255];
							sprintf(iUniqueString, "##ComboStoryboardCustom%d", i);
							int iComboEntries = 1;
							int comboflags = ImGuiComboFlags_NoPreview | ImGuiComboFlags_PopupAlignLeft | ImGuiComboFlags_HeightLarge;
							ImGui::PushItemWidth(20);
							if (ImGui::BeginCombo(iUniqueString, "", comboflags))
							{
								for (int n = 0; n < iComboEntries; n++)
								{
									if (ImGui::Selectable(items_storyboard_hud[n], false))
									{
										Storyboard.iChanged = true;
										bBlockNextMouseCheck = true;
										selection = n;
										if (selection == 0)
										{
											int iAction = askBoxCancel("This will delete the custom screen from your storyboard, are you sure?", "Confirmation"); //1==Yes 2=Cancel 0=No
											if (iAction == 1)
											{
												reset_single_node(i);
												Storyboard.Nodes[i].used = false;
												bBlockNextMouseCheck = true;
											}
										}
									}
								}
								ImGui::EndCombo();
							}
							if (ImGui::IsItemHovered())
							{
								bBlockNextMouseCheck = true;
								vTooltipPos = ImGui::GetCursorPos();
								sTooltip = " Delete Custom screen ";
							}
							ImGui::PopItemWidth();
							ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1.f, 1.f));
							ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
							ImGui::PushStyleColor(ImGuiCol_ChildBg, GImNodes->Style.Colors[ImNodesCol_GridBackground]);
						}

						if (Storyboard.Nodes[i].type == STORYBOARD_TYPE_HUD)
						{
							ImGui::PopStyleColor();
							ImGui::PopStyleVar();
							ImGui::PopStyleVar();
							const char* items_storyboard_hud[] = { "Delete HUD Screen", "Duplicate HUD Screen", "Rename HUD Screen"};
							ImGui::SetCursorPos(ImVec2(cpos.x + fNodeWidth - 48.0f, cpos.y - 8.0));
							int selection = 0;
							char iUniqueString[255];
							sprintf(iUniqueString, "##ComboStoryboardHUD%d", i);
							int iComboEntries = 3;// 1;
							int comboflags = ImGuiComboFlags_NoPreview | ImGuiComboFlags_PopupAlignLeft | ImGuiComboFlags_HeightLarge;
							ImGui::PushItemWidth(20);
							if (ImGui::BeginCombo(iUniqueString, "", comboflags))
							{
								for (int n = 0; n < iComboEntries; n++)
								{
									if (ImGui::Selectable(items_storyboard_hud[n], false))
									{
										Storyboard.iChanged = true;
										bBlockNextMouseCheck = true;
										selection = n;
										if (selection == 0)
										{
											int iAction = askBoxCancel("This will delete the HUD screen from your storyboard, are you sure?", "Confirmation"); //1==Yes 2=Cancel 0=No
											if (iAction == 1)
											{
												//Delete HUD.
												reset_single_node(i);
												Storyboard.Nodes[i].used = false;
												bBlockNextMouseCheck = true;
											}
										}
										if (selection == 1)
										{
											//Duplicate HUD.
											iTriggerDuplicateOutsideOfLoop = i;
											bBlockNextMouseCheck = true;
										}
										if (selection == 2)
										{
											//Rename HUD.
											rename_single_node(i);
											bBlockNextMouseCheck = true;
										}
									}
								}
								ImGui::EndCombo();
							}
							if (ImGui::IsItemHovered()) 
							{
								bBlockNextMouseCheck = true;
								vTooltipPos = ImGui::GetCursorPos();
								sTooltip = " Manage HUD screen";
							}
							ImGui::PopItemWidth();
							ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1.f, 1.f));
							ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
							ImGui::PushStyleColor(ImGuiCol_ChildBg, GImNodes->Style.Colors[ImNodesCol_GridBackground]);
						}

						if (Storyboard.Nodes[i].type == STORYBOARD_TYPE_SCREEN && i>=0 && i<=12)
						{
							ImGui::PopStyleColor();
							ImGui::PopStyleVar();
							ImGui::PopStyleVar();
							const char* items_storyboard_screen_restore[] = { "Restore Screen" };
							ImGui::SetCursorPos(ImVec2(cpos.x + fNodeWidth - 48.0f, cpos.y - 8.0));
							int selection = 0;
							char iUniqueString[255];
							sprintf(iUniqueString, "##ComboStoryboardScreenRestore%d", i);
							int iComboEntries = 1;
							int comboflags = ImGuiComboFlags_NoPreview | ImGuiComboFlags_PopupAlignLeft | ImGuiComboFlags_HeightLarge;
							ImGui::PushItemWidth(20);
							if (ImGui::BeginCombo(iUniqueString, "", comboflags))
							{
								for (int n = 0; n < iComboEntries; n++)
								{
									if (ImGui::Selectable(items_storyboard_screen_restore[n], false))
									{
										Storyboard.iChanged = true;
										bBlockNextMouseCheck = true;
										selection = n;
										if (selection == 0)
										{
											int iAction = askBoxCancel("This will restore the screen to original, are you sure?", "Confirmation"); //1==Yes 2=Cancel 0=No
											if (iAction == 1)
											{
												reset_single_node_interscreen(i);
												storyboard_add_missing_nodex(i, 0, 0, 0, true, true);
												extern void RefreshThumbImagesForNode(int, int);
												RefreshThumbImagesForNode(i, 0);
												strcpy(Storyboard.Nodes[i].thumb, "editors\\uiv3\\click-here-box-screen.png");
												SetMipmapNum(1);
												image_setlegacyimageloading(true);
												LoadImageSize(Storyboard.Nodes[i].thumb, Storyboard.Nodes[i].thumb_id, 512, 288);
												image_setlegacyimageloading(false);
												SetMipmapNum(-1);
												bBlockNextMouseCheck = true;
											}
										}
									}
								}
								ImGui::EndCombo();
							}
							if (ImGui::IsItemHovered())
							{
								bBlockNextMouseCheck = true;
								vTooltipPos = ImGui::GetCursorPos();
								sTooltip = " Restore Screen ";
							}
							ImGui::PopItemWidth();
							ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1.f, 1.f));
							ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
							ImGui::PushStyleColor(ImGuiCol_ChildBg, GImNodes->Style.Colors[ImNodesCol_GridBackground]);
						}

						bool bExecutePencelEdit = false;
						char UniqueFileString[MAX_PATH];
						sprintf(UniqueFileString, "##GetFileStoryboard%d", i);
						ImGuiWindow* window = ImGui::GetCurrentWindow();
						int iSplashID = window->GetID(UniqueFileString);

						if (Storyboard.Nodes[i].iEditEnable)
						{
							ImGui::SetItemAllowOverlap();
							ImGui::SetCursorPos(ImVec2(cpos.x + fNodeWidth - 20.0f, cpos.y - 3.0));
							ImGui::PushID(Storyboard.Nodes[i].id + 500);

							if (Storyboard.Nodes[i].type == STORYBOARD_TYPE_SPLASH)
							{
								if (iSelectedLibraryStingReturnID == iSplashID)
								{
									//Update selected var.
									strcpy(Storyboard.Nodes[i].thumb, sSelectedLibrarySting.Get());
									sSelectedLibrarySting = "";
									iSelectedLibraryStingReturnID = -1; //disable.
									//Update thumb.
									UpdateThumbOnNode = i;
								}
							}

							if (ImGui::ImgBtn(TOOL_PENCIL, ImVec2(16, 16), ImColor(255, 255, 255, 0)))
							{
								bExecutePencelEdit = true;
							}
							if (ImGui::IsItemHovered())
							{
								bBlockNextMouseCheck = true;
								vTooltipPos = ImGui::GetCursorPos();
								if (Storyboard.Nodes[i].type == STORYBOARD_TYPE_SPLASH)
									sTooltip = " Edit Splash Screen ";
								else if (Storyboard.Nodes[i].type == STORYBOARD_TYPE_SCREEN)
									sTooltip = " Edit Screen ";
								else if (Storyboard.Nodes[i].type == STORYBOARD_TYPE_LEVEL && !pestrcasestr(Storyboard.Nodes[i].lua_name, "loading"))
									sTooltip = " Edit Level ";
								else if (Storyboard.Nodes[i].type == STORYBOARD_TYPE_LEVEL)
									sTooltip = " Edit Loading Screen ";
							}
							ImGui::PopID();
						}
						ImNodes::EndNodeTitleBar();

						//#### Input attibs ####
						cpos = ImGui::GetCursorPos();
						for (int l = 0; l < STORYBOARD_MAXOUTPUTS; l++)
						{
							if (strlen(Storyboard.Nodes[i].input_title[l]) > 0)
							{
								int iPinId = Storyboard.Nodes[i].input_id[l];
								ImNodes::BeginInputAttribute(iPinId, ImNodesPinShape_QuadFilled);
								ImGui::Text("");
								if (iLastHoveredId == iPinId) { vTooltipPos = ImGui::GetCursorPos(); sTooltip = Storyboard.Nodes[i].input_title[l]; }
								ImNodes::EndInputAttribute();
							}

						}
						ImGui::SetCursorPos(cpos);

						//#### Preview Thumb ####
						int TextureID = BOX_CLICK_HERE; //MARKETPLACE_FILLER;
						if (ImageExist(Storyboard.Nodes[i].thumb_id))
						{
							TextureID = Storyboard.Nodes[i].thumb_id;
						}
						cpos = ImGui::GetCursorPos();

						//#### Add button so can click to edit, pencil function ####
						//ImGui::Dummy(iThumbSize);
						char UniqueButName[128];
						sprintf(UniqueButName, "##Dummy%d", i);
						if (iLastHoveredNodeId > 0 && iLastHoveredNodeId == Storyboard.Nodes[i].id)
						{
							if (ImGui::ButtonEx(UniqueButName, iThumbSize, 0))
							{
								bExecutePencelEdit = true;
							}
						}
						else
						{
							ImGui::Dummy(iThumbSize);
						}
						//#### Execute Edit With Pencil ####
						if (bExecutePencelEdit)
						{
							// only for non game levels (ensures can open storyboard, open level, play, return without needing to save project on way back to hub)
							if (strlen(Storyboard.Nodes[i].levelnumber) == 0) Storyboard.iChanged = true;

							//ImGuiWindow* window = ImGui::GetCurrentWindow();
							if (Storyboard.Nodes[i].type == STORYBOARD_TYPE_SPLASH)
							{
								//Edit splash.
								bExternal_Entities_Window = true;
								iDisplayLibraryType = 2;
								iLibraryStingReturnToID = iSplashID;
								if (strlen(Storyboard.Nodes[i].thumb) > 0)
								{
									sMakeDefaultSelecting = Storyboard.Nodes[i].thumb;
									bSelectLibraryViewAll = true;
								}

								if (pref.iSplashStartMessage == 0)
								{
									strcpy(cTriggerMessage, "Select the backdrop image you want to appear when your game is first launched");
									bTriggerMessage = true;
									pref.iSplashStartMessage = 1;
									iTriggerMessageDelay = 2;
									iTriggerMessageY = 1;
								}

							}
							else if (Storyboard.Nodes[i].type == STORYBOARD_TYPE_SCREEN || pestrcasestr(Storyboard.Nodes[i].lua_name, "loading"))
							{

								if (i == iTitleScreenNodeID && pref.iTitleStartMessage == 0)
								{
									pref.iTitleStartMessage = 1;
									strcpy(cTriggerMessage, "Design your game start menus");
									bTriggerMessage = true;
									iTriggerMessageDelay = 2;
									iTriggerMessageY = 1;
								}
								if (i == 2 && pref.iLoadingStartMessage == 0)
								{
									pref.iLoadingStartMessage = 1;
									strcpy(cTriggerMessage, "Create the screen shown before each level loads");
									bTriggerMessage = true;
									iTriggerMessageDelay = 2;
									iTriggerMessageY = 1;
								}
								if (i == 5 && pref.iGameWonStartMessage == 0)
								{
									pref.iGameWonStartMessage = 1;
									strcpy(cTriggerMessage, "Design the screen shown when the game is won");
									bTriggerMessage = true;
									iTriggerMessageDelay = 2;
									iTriggerMessageY = 1;
								}
								if (i == 6 && pref.iGameOverStartMessage == 0)
								{
									pref.iGameOverStartMessage = 1;
									strcpy(cTriggerMessage, "Create the screen when the player loses the game");
									bTriggerMessage = true;
									iTriggerMessageDelay = 2;
									iTriggerMessageY = 1;
								}

								bScreen_Editor_Window = true;
								iScreen_Editor_Node = i;
							}
							else if (Storyboard.Nodes[i].type == STORYBOARD_TYPE_LEVEL && !pestrcasestr(Storyboard.Nodes[i].lua_name, "loading"))
							{
								//Edit level.
								CloseAllOpenTools();

								if (strlen(Storyboard.Nodes[i].level_name) > 0)
								{
									//Load and edit.
									if (stricmp(Storyboard.Nodes[i].level_name, g.projectfilename_s.Get()) == NULL)
									{
										//Already open , just exit to level editor.
										if (bWaypointDrawmode || bWaypoint_Window) { bWaypointDrawmode = false; bWaypoint_Window = false; }
										if (bImporter_Window) { importer_quit(); bImporter_Window = false; }
										if (g_bCharacterCreatorPlusActivated) g_bCharacterCreatorPlusActivated = false;
										if (bEntity_Properties_Window) bEntity_Properties_Window = false;
										if (t.ebe.on == 1) ebe_hide();
										//
										bool bAbort = false;
										if (Storyboard.iChanged)
										{
											bBlockNextMouseCheck = true;
											if (!pref.iDisableProjectAutoSave && strlen(Storyboard.gamename) > 0)
											{
												save_storyboard(Storyboard.gamename, false);
											}
											else
											{
												int iAction = askBoxCancel(STORYBOARD_SAVE_MESSAGE, "Confirmation"); //1==Yes 2=Cancel 0=No
												if (iAction == 1)
												{
													//Save.
													if (strlen(Storyboard.gamename) > 0)
														save_storyboard(Storyboard.gamename, false);
													else
													{
														bAbort = true;
														save_storyboard(Storyboard.gamename, true);
													}
												}
											}
										}
										if (!bAbort)
										{
											//Object Tools as default.
											bForceKey = true;
											csForceKey = "o";
											bTerrain_Tools_Window = false;
											Entity_Tools_Window = true;

											bStoryboardWindow = false;
											iLevelEditorFromStoryboardID = i;
										}

									}
									else
									{
										//Load level and exit to level editor.
										strcpy(cDirectOpen, Storyboard.Nodes[i].level_name);
										iLaunchAfterSync = 7; //Direct load.
										iSkibFramesBeforeLaunch = 5;
										bCloseStoryboardAfterLoad = true;
										iLevelEditorFromStoryboardID = i;
									}
								}
								else
								{
									//PE: If empty node level , start level editor.
									//PE: Default to terrain tools , like when we launch Max.

									//PE: New default to object mode.
									bForceKey = true;
									csForceKey = "o";
									bTerrain_Tools_Window = false;
									Entity_Tools_Window = true;

									bProceduralLevelFromStoryboard = true;
									iLaunchAfterSync = 5;
									iBlackoutForFrames = 5;
									iSkibFramesBeforeLaunch = 2;
									iWaitForNewLevel = 10;
									iNewLevelNode = i;

									//Make sure we have a fresh thumb. if generated by new level.
									GG_SetWritablesToRoot(true);
									if (FileExist("thumbbank\\lastnewlevel.jpg")) DeleteAFile("thumbbank\\lastnewlevel.jpg");
									GG_SetWritablesToRoot(false);

									//PE: Switch to normal message.
									strcpy(cTriggerMessage, "Preparing the Terrain Generator. Please wait...");
									bTriggerMessage = true;

									// refresh custom biomes before enter Terrain Generator
									extern void imgui_populatecustombiomes(void);
									imgui_populatecustombiomes();
								}
							}
							else if (Storyboard.Nodes[i].type == STORYBOARD_TYPE_HUD)
							{
								if (i == iHUDScreenNodeID && pref.iTitleStartMessage == 0)
								{
									pref.iTitleStartMessage = 1;
									strcpy(cTriggerMessage, "Design your in-game HUD");
									bTriggerMessage = true;
									iTriggerMessageDelay = 2;
									iTriggerMessageY = 1;
								}
								bScreen_Editor_Window = true;
								iScreen_Editor_Node = i;
							}
						}

						ImGui::SetCursorPos(cpos);
						lpTexture = GetImagePointerView(TextureID);
						if (lpTexture && Storyboard.Nodes[i].type == STORYBOARD_TYPE_SPLASH)
						{
							//Center image.
							float img_w = ImageWidth(TextureID);
							float img_h = ImageHeight(TextureID);

							if (img_w > iThumbSize.x || img_h > iThumbSize.y) {
								float fRatio = 1.0f / (img_w / img_h);
								img_w = iThumbSize.x;
								img_h = iThumbSize.x * fRatio;
								if (img_h > iThumbSize.y) {
									float fRatio = 1.0f / (img_h / img_w);
									img_h = iThumbSize.y;
									img_w = iThumbSize.y * fRatio;
								}
							}

							ImGuiWindow* window = ImGui::GetCurrentWindow();
							ImVec2 img_pos = ImGui::GetWindowPos() + cpos;

							img_pos.x += (iThumbSize.x - img_w) * 0.5;
							img_pos.y += (iThumbSize.y - img_h) * 0.5;
							window->DrawList->AddImage((ImTextureID)lpTexture, img_pos, img_pos + ImVec2(img_w, img_h), ImVec2(0, 0), ImVec2(1, 1), ImGui::GetColorU32(ImVec4(1.0, 1.0, 1.0, 0.8)));

						}
						else
						{
							if (lpTexture)
							{
								ImGuiWindow* window = ImGui::GetCurrentWindow();
								ImVec2 img_pos = ImGui::GetWindowPos() + cpos;
								window->DrawList->AddImage((ImTextureID)lpTexture, img_pos, img_pos + iThumbSize, ImVec2(0, 0), ImVec2(1, 1), ImGui::GetColorU32(ImVec4(1.0, 1.0, 1.0, 0.8)));
							}
						}

						//#### Output attibs ####
						for (int l = 0; l < STORYBOARD_MAXOUTPUTS; l++)
						{
							bool bValid = true;
							int iPinId = Storyboard.Nodes[i].output_id[l];
							int linkto = Storyboard.Nodes[i].output_linkto[l];
							if (!pref.iStoryboardAdvanced && strlen(Storyboard.Nodes[i].output_title[l]) > 0)
							{
								//PE: Hide nodes if not in advanced.
								if (linkto == Storyboard.Nodes[iLoadGameNodeID].input_id[0] ||
									linkto == Storyboard.Nodes[iGamePausedNodeID].input_id[0] ||
									linkto == Storyboard.Nodes[iSaveGameNodeID].input_id[0] ||
									linkto == Storyboard.Nodes[iGraphicsNodeID].input_id[0] ||
									linkto == Storyboard.Nodes[iSoundsNodeID].input_id[0] )
									bValid = false;
							}

							if (bValid && strlen(Storyboard.Nodes[i].output_title[l]) > 0)
							{
								ImNodes::BeginOutputAttribute(iPinId, ImNodesPinShape_TriangleFilled);
								ImGui::Text("");
								if (iLastHoveredId == iPinId) { vTooltipPos = ImGui::GetCursorPos(); sTooltip = Storyboard.Nodes[i].output_title[l]; }
								ImNodes::EndOutputAttribute();
							}
						}


						ImNodes::EndNode();
					}
				}

				ImNodes::PopAttributeFlag();

				std::map<int, int> no_duplicates;
				int linkid = 0;
				for (int i = 0; i < STORYBOARD_MAXNODES; i++)
				{
					bool bValid = true;

					if (!pref.iStoryboardAdvanced && Storyboard.Nodes[i].used)
					{
						//PE: Hide nodes if not in advanced.
						if (i == iLoadGameNodeID ||
							i == iGamePausedNodeID ||
							i == iSaveGameNodeID ||
							i == iGraphicsNodeID ||
							i == iSoundsNodeID)
							bValid = false;
					}

					if (bValid && Storyboard.Nodes[i].used)
					{
						for (int l = 0; l < STORYBOARD_MAXOUTPUTS; l++)
						{
							int linkfrom = Storyboard.Nodes[i].output_id[l];
							int linkto = Storyboard.Nodes[i].output_linkto[l];
							if (linkfrom > 0 && linkto > 0)
							{
								bool bLinkValid = true;
								if (!pref.iStoryboardAdvanced )
								{
									//PE: Hide nodes if not in advanced.
									if (linkto == Storyboard.Nodes[iLoadGameNodeID].input_id[0] ||
										linkto == Storyboard.Nodes[iGamePausedNodeID].input_id[0] ||
										linkto == Storyboard.Nodes[iSaveGameNodeID].input_id[0] ||
										linkto == Storyboard.Nodes[iGraphicsNodeID].input_id[0] ||
										linkto == Storyboard.Nodes[iSoundsNodeID].input_id[0])
										bLinkValid = false;
								}

								if (bLinkValid)
								{
									bool bVisible = true;
									int is = no_duplicates.size();
									no_duplicates.insert(std::make_pair(linkfrom, linkto));
									if (is == no_duplicates.size())
										bVisible = false;
									if (bVisible)
									{
										if (linkid < STORYBOARD_MAXNODES)
										{
											ImNodes::Link(linkid, linkfrom, linkto);
											StoryboardiActiveLinksId[linkid] = linkto;
											StoryboardiActiveLinksIdFrom[linkid] = linkfrom;
											linkid++;
										}
									}
								}
							}
						}
					}
				}
				no_duplicates.clear();

				ImNodes::MiniMap(0.15f, ImNodesMiniMapLocation_BottomRight); //PE: size,corner.
				ImNodes::EndNodeEditor();

				// do not create new node elements in side the node loop!
				if (iTriggerDuplicateOutsideOfLoop > 0)
				{
					duplicate_single_node(iTriggerDuplicateOutsideOfLoop);
					iTriggerDuplicateOutsideOfLoop = -1;
				}

				ImVec2 vNodeAreaEnd = ImGui::GetCursorScreenPos();
				vNodeAreaEnd.x += ImGui::GetContentRegionAvailWidth();
				if (ImGui::IsMouseHoveringRect(vNodeAreaStart, vNodeAreaEnd))
				{
					static bool bGetMouseDown = false;
					static bool bWaitOnRelease = false;
					if (!bTriggerSaveAs && !bTriggerOpenProject && !bProceduralLevel && !bBlockNextMouseCheck)
					{
						if (bWaitOnRelease && !ImGui::IsMouseDown(0))
						{
							bWaitOnRelease = false;
						}
						else
						{
							//Need a down and release to trigger.
							if (bGetMouseDown && ImGui::IsMouseReleased(0))
							{
								//As outside of any specific node, cannot know if modification was to screen or level
								//Storyboard.iChanged = true; //PE: Anything trigger a change in project.
								//if (strlen(Storyboard.Nodes[i].levelnumber) == 0) Storyboard.iChanged = true; // Not for game levels
								//LB: Agreed, it seems we cannot determine difference between a click to nowhere and a click that changed something
								// so we opt for the one that provides the best chance for users NOT to lose their work
								Storyboard.iChanged = 1;
								bGetMouseDown = false;
							}
							if (ImGui::IsMouseDown(0)) bGetMouseDown = true; //PE: Anything trigger a change in project.
						}
					}
					else
					{
						bWaitOnRelease = true;
						bBlockNextMouseCheck = false;
						bGetMouseDown = false;
					}
				}


				int start_attr, end_attr;
				if (ImNodes::IsLinkCreated(&start_attr, &end_attr))
				{
					bool valid_link = false; // type goes to correct type.
					int iInNode = -1, iOutNode = -1, iInAttr = -1, iOutAttr = -1;
					for (int i = 0; i < STORYBOARD_MAXNODES; i++)
					{
						if (Storyboard.Nodes[i].used)
						{
							for (int l = 0; l < STORYBOARD_MAXOUTPUTS; l++)
							{
								int linkfrom = Storyboard.Nodes[i].output_id[l];
								int linkto = Storyboard.Nodes[i].input_id[l];

								if (start_attr == linkfrom)
								{
									iInNode = i;
									iInAttr = l;
								}
								if (end_attr == linkto)
								{
									iOutNode = i;
									iOutAttr = l;
								}
							}
						}
					}
					if (iInNode >= 0 && iOutNode >= 0 && iInAttr >= 0 && iOutAttr >= 0)
					{
						//PE: Check if connect type match
						if (Storyboard.Nodes[iOutNode].type == Storyboard.Nodes[iInNode].output_can_link_to_type[iInAttr])
						{
							valid_link = true;
						}
					}

					if (valid_link)
					{
						Storyboard.Nodes[iInNode].output_linkto[iInAttr] = end_attr;
					}
				}

				int link_id;
				if (ImNodes::IsLinkDestroyed(&link_id))
				{
					if (link_id < STORYBOARD_MAXNODES && link_id >= 0)
					{
						int linkfrom = StoryboardiActiveLinksId[link_id];
						int linkto = StoryboardiActiveLinksIdFrom[link_id];

						for (int i = 0; i < STORYBOARD_MAXNODES; i++)
						{
							if (Storyboard.Nodes[i].used)
							{
								for (int l = 0; l < STORYBOARD_MAXOUTPUTS; l++)
								{

									if (linkfrom == Storyboard.Nodes[i].output_linkto[l] && linkto == Storyboard.Nodes[i].output_id[l])
										Storyboard.Nodes[i].output_linkto[l] = 0;
								}
							}
						}
					}

				}

				ImGui::NextColumn();

				const ImNodesEditorContext& editor = ImNodes::EditorContextGet();
				if (gImNodes->HoveredPinIdx.HasValue() && gImNodes->HoveredPinIdx.Value() >= 0)
					iLastHoveredId = editor.Pins.Pool[GImNodes->HoveredPinIdx.Value()].Id;
				else
					iLastHoveredId = 0;

				if (!ImGui::IsMouseDown(0))
				{
					iLastHoveredNodeId = 0;
					if (gImNodes->HoveredNodeIdx.HasValue() && gImNodes->HoveredNodeIdx.Value() >= 0)
					{
						if (gImNodes->HoveredNodeIdx.Value() < editor.Nodes.Pool.Size)
							iLastHoveredNodeId = editor.Nodes.Pool[gImNodes->HoveredNodeIdx.Value()].Id;
					}
				}

				float buttonwide = 200.0f;

				// extra help for users to know maximum limits of node creation
				bool bShowNoMoreScreensError = false;
				char pToolTipForAddingNewScreens[256];
				sprintf(pToolTipForAddingNewScreens, "The game project can contain up to %d screens or levels.", STORYBOARD_MAXNODES);

				const float groupspacer = 4.0f;
				static int ClassicConversion = 0;
				static char pReconstructGameGuruRootFiles[MAX_PATH];

				#ifdef INCLUDE_GAME_SETTINGS
				//if (pref.iStoryboardAdvanced) // Necrym59 always visible.
				{
					if (ImGui::StyleCollapsingHeader("Game Project Settings", ImGuiTreeNodeFlags_DefaultOpen) || iStoryboardExecuteKey != 0) //"Add New"
					{
						ImGui::Indent(10);
						ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((ImGui::GetContentRegionAvail().x * 0.5) - (buttonwide * 0.5), 0.0f));
						if (ImGui::StyleButton("Edit Game Settings", ImVec2(buttonwide, 0.0f)))
						{
							bEditGameSettings = true;
						}
						ImGui::Indent(-10);
					}
				}
				#endif

				if (ImGui::StyleCollapsingHeader("Add and Edit Storyboard", ImGuiTreeNodeFlags_DefaultOpen) || iStoryboardExecuteKey != 0) //"Add New"
				{
					int iAutoConnectNode = -1;
					ImGui::Indent(10);
					ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((ImGui::GetContentRegionAvail().x*0.5) - (buttonwide*0.5), 0.0f));
					if (ImGui::StyleButton("Add New Level", ImVec2(buttonwide, 0.0f)) || iStoryboardExecuteKey == 'N')
					{
						iStoryboardExecuteKey = 0;
						//PE: Find next level from nodes.
						int iNextLevel = 0, levelname = -1, iFirstNodeFree = -1;
						FindFreeLevelNode(iNextLevel, levelname, iFirstNodeFree);

						if (iFirstNodeFree >= 0)
						{
							//Create new level.
							char tmp[255];
							int node = iFirstNodeFree;
							int nodeposy = iNextLevel;
							if (levelname > 0)
							{
								sprintf(tmp, "Level %d", levelname);
								nodeposy = levelname - 1;
							}
							else
								sprintf(tmp, "Level %d", iNextLevel + 1);

							//PE: Make sure any old data is removed, also thumbs.
							reset_single_node(node);

							Storyboard.Nodes[node].used = true;
							Storyboard.Nodes[node].type = STORYBOARD_TYPE_LEVEL;
							Storyboard.Nodes[node].restore_position = ImVec2(preview_size_x*0.5 - (fNodeWidth*0.5) + ((fNodeWidth + NODE_WIDTH_PADDING)*2.0), STORYBOARD_YSTART + ((fNodeHeight + 20.0 + NODE_HEIGHT_PADDING) * (nodeposy)));
							Storyboard.Nodes[node].iEditEnable = true;
							strcpy(Storyboard.Nodes[node].title, tmp);
							strcpy(Storyboard.Nodes[node].levelnumber, tmp);

							strcpy(Storyboard.Nodes[node].thumb, "");
							//Input.
							strcpy(Storyboard.Nodes[node].input_title[0], " Input ");
							//Output.
							strcpy(Storyboard.Nodes[node].output_title[0], " WIN LEVEL -> Connect to Scene ");
							strcpy(Storyboard.Nodes[node].output_action[0], "loadlevel"); //Not defined this yet.
							Storyboard.Nodes[node].output_can_link_to_type[0] = STORYBOARD_TYPE_SCREEN;
							Storyboard.Nodes[node].output_linkto[0] = 0;

							strcpy(Storyboard.Nodes[node].output_title[1], " GAME OVER -> Connect to Scene ");
							strcpy(Storyboard.Nodes[node].output_action[1], "loadlevel"); //Not defined this yet.
							Storyboard.Nodes[node].output_can_link_to_type[1] = STORYBOARD_TYPE_SCREEN;
							Storyboard.Nodes[node].output_linkto[1] = 0;

							strcpy(Storyboard.Nodes[node].output_title[2], " NEXT LEVEL -> Connect to Level ");
							strcpy(Storyboard.Nodes[node].output_action[2], "loadlevel"); //Not defined this yet.
							Storyboard.Nodes[node].output_can_link_to_type[2] = STORYBOARD_TYPE_LEVEL;
							Storyboard.Nodes[node].output_linkto[2] = 0;
							ImNodes::SetNodeGridSpacePos(Storyboard.Nodes[node].id, Storyboard.Nodes[node].restore_position);
							iAutoConnectNode = node;
						}
						else
						{
							bShowNoMoreScreensError = true;
						}
					}
					if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", pToolTipForAddingNewScreens);


					ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((ImGui::GetContentRegionAvail().x * 0.5) - (buttonwide * 0.5), 0.0f));
					if (ImGui::StyleButton("Add Existing Level", ImVec2(buttonwide, 0.0f)) || iStoryboardExecuteKey == 'L')
					{
						iStoryboardExecuteKey = 0;
						cStr tOldDir = GetDir();

						// we know we need to focus on the mapbank associated with the current storyboard
						cstr correctFPMLocation_s = Storyboard.customprojectfolder;
						if (correctFPMLocation_s.Len() > 0)
						{
							correctFPMLocation_s += Storyboard.gamename;
							correctFPMLocation_s += "\\Files\\mapbank";
						}
						else
						{
							correctFPMLocation_s = g.mysystem.mapbankAbs_s.Get();
						}
						//cFileSelected = (char *)noc_file_dialog_open(NOC_FILE_DIALOG_OPEN, "fpm\0*.fpm\0", g.mysystem.mapbankAbs_s.Get(), NULL, true);
						char* cFileSelected = (char*)noc_file_dialog_open(NOC_FILE_DIALOG_OPEN, "fpm\0*.fpm\0", correctFPMLocation_s.Get(), NULL, true);

						SetDir(tOldDir.Get());
						if (cFileSelected && strlen(cFileSelected) > 0)
						{
							t.returnstring_s = cFileSelected;
							if (t.returnstring_s != "")
							{
								if (cstr(Lower(Right(t.returnstring_s.Get(), 4))) == ".fpm")
								{

									//Only relative.
									char tmp[MAX_PATH];
									strcpy(tmp, t.returnstring_s.Get());

									//PE: Vaidate path, must be inside the mapbank to work.
									extern char szRootDir[MAX_PATH];
									extern char szWriteDir[MAX_PATH];
									extern char szAddWriteDirAdditional[MAX_PATH];

									bool bValidPath = false;
									int rootLen = strlen(szWriteDir);
									if (strnicmp(tmp, szWriteDir, rootLen) == 0)
									{
										bValidPath = true;
									}
									rootLen = strlen(szAddWriteDirAdditional);
									if (!bValidPath && strnicmp(tmp, szAddWriteDirAdditional, rootLen) == 0)
									{
										bValidPath = true;
									}
									rootLen = strlen(szRootDir);
									if (!bValidPath && strnicmp(tmp, szRootDir, rootLen) == 0)
									{
										bValidPath = true;
									}


									char* find = (char*)pestrcasestr(tmp, "mapbank\\");
									if (find && find != &tmp[0]) strcpy(&tmp[0], find);

									if (bValidPath && !find)
									{
										//PE: Must be located inside mapbank.
										bValidPath = false;;
									}

									//PE: check if this is a Classic map we need to import.
									bool bImportClassicMap = false;
									{
										strcpy(pReconstructGameGuruRootFiles, "");
										char pReconstructGameGuruFolder[MAX_PATH];
										strcpy(pReconstructGameGuruFolder, "");
										char pReconstructGameGuruEXE[MAX_PATH];
										strcpy(pReconstructGameGuruEXE, cFileSelected);
										char* pFindClassicFolder = (char*)pestrcasestr(pReconstructGameGuruEXE, "Game Guru\\Files\\mapbank\\");
										if (pFindClassicFolder != NULL)
										{
											*pFindClassicFolder = 0;
											strcpy(pReconstructGameGuruRootFiles, pReconstructGameGuruEXE);
											strcat(pReconstructGameGuruRootFiles, "Game Guru\\Files\\");
											strcpy(pReconstructGameGuruFolder, pReconstructGameGuruEXE);
											strcat(pReconstructGameGuruFolder, "Game Guru\\Files\\entitybank\\");
											strcat(pReconstructGameGuruEXE, "Game Guru\\GameGuru.exe");
											if (FileExist(pReconstructGameGuruEXE) == 1)
											{
												bImportClassicMap = true;
												bValidPath = false;
											}
										}
									}

									if (!bValidPath)
									{
										if (bImportClassicMap)
										{
											int iAction = askBoxCancel("You have selected a classic map, do you want to import this level ?", "GameGuru Classic Map!"); //1==Yes 2=Cancel 0=No
											if (iAction == 1)
											{
												//Import map.
												ClassicConversion = 1;
												sNextLevelToLoad = t.returnstring_s;
											}
										}
										else
										{
											MessageBoxA(NULL, "All levels added to storyboard must be saved inside the default 'mapbank' folder.", "Error:", 0);
										}
									}
									if (bValidPath)
									{
										std::string sLevelPath = &tmp[0];

										//Dont actual load, just use filename.
										int iPos;
										for (iPos = strlen(tmp); iPos >= 0; iPos--)
											if (tmp[iPos] == '\\') break;
										if (iPos > 0) iPos++;
										std::string sLevelTitle = &tmp[iPos];
										replaceAll(sLevelTitle, ".fpm", "");

										//PE: Find next level from nodes.
										int iNextLevel = 0, levelname = -1, iFirstNodeFree = -1;
										FindFreeLevelNode(iNextLevel, levelname, iFirstNodeFree);

										if (iFirstNodeFree >= 0)
										{
											//Create new level.
											char tmp[255];
											int node = iFirstNodeFree;
											int nodeposy = iNextLevel;
											if (levelname > 0)
											{
												sprintf(tmp, "Level %d", levelname);
												nodeposy = levelname - 1;
											}
											else
												sprintf(tmp, "Level %d", iNextLevel + 1);

											//PE: Make sure any old data is removed, also thumbs.
											reset_single_node(node);

											Storyboard.Nodes[node].used = true;
											Storyboard.Nodes[node].type = STORYBOARD_TYPE_LEVEL;
											Storyboard.Nodes[node].restore_position = ImVec2(preview_size_x * 0.5 - (fNodeWidth * 0.5) + ((fNodeWidth + NODE_WIDTH_PADDING) * 2.0), STORYBOARD_YSTART + ((fNodeHeight + 20.0 + NODE_HEIGHT_PADDING) * (nodeposy)));
											Storyboard.Nodes[node].iEditEnable = true;
											strcpy(Storyboard.Nodes[node].title, sLevelTitle.c_str());
											strcpy(Storyboard.Nodes[node].level_name, sLevelPath.c_str());
											strcpy(Storyboard.Nodes[node].levelnumber, tmp);

											strcpy(Storyboard.Nodes[node].thumb, "");
											//Input.
											strcpy(Storyboard.Nodes[node].input_title[0], " Input ");
											//Output.
											strcpy(Storyboard.Nodes[node].output_title[0], " WIN LEVEL -> Connect to Scene ");
											strcpy(Storyboard.Nodes[node].output_action[0], "loadlevel"); //Not defined this yet.
											Storyboard.Nodes[node].output_can_link_to_type[0] = STORYBOARD_TYPE_SCREEN;
											Storyboard.Nodes[node].output_linkto[0] = 0;

											strcpy(Storyboard.Nodes[node].output_title[1], " GAME OVER -> Connect to Scene ");
											strcpy(Storyboard.Nodes[node].output_action[1], "loadlevel"); //Not defined this yet.
											Storyboard.Nodes[node].output_can_link_to_type[1] = STORYBOARD_TYPE_SCREEN;
											Storyboard.Nodes[node].output_linkto[1] = 0;

											strcpy(Storyboard.Nodes[node].output_title[2], " NEXT LEVEL -> Connect to Level ");
											strcpy(Storyboard.Nodes[node].output_action[2], "loadlevel"); //Not defined this yet.
											Storyboard.Nodes[node].output_can_link_to_type[2] = STORYBOARD_TYPE_LEVEL;
											Storyboard.Nodes[node].output_linkto[2] = 0;
											ImNodes::SetNodeGridSpacePos(Storyboard.Nodes[node].id, Storyboard.Nodes[node].restore_position);
											iAutoConnectNode = node;
											//Check if level already got a thumb.
											CreateBackBufferCacheNameEx(Storyboard.Nodes[node].level_name, 512, 288, true);
											if (FileExist(BackBufferCacheName.Get()))
											{
												if (CopyToProjectFolder(BackBufferCacheName.Get()))
												{
													//PE: Use relative projectbank filename.
													if (FileExist(ProjectCacheName.Get()))
														BackBufferCacheName = ProjectCacheName;
												}

												//PE: Load in old thumb.
												SetMipmapNum(1); //PE: mipmaps not needed.
												image_setlegacyimageloading(true);
												LoadImageSize(BackBufferCacheName.Get(), Storyboard.Nodes[node].thumb_id, 512, 288);
												image_setlegacyimageloading(false);
												SetMipmapNum(-1); //PE: mipmaps not needed.
												if (ImageExist(Storyboard.Nodes[node].thumb_id))
												{
													//PE: Success update thumb filename.
													strcpy(Storyboard.Nodes[node].thumb, BackBufferCacheName.Get());
												}
											}
										}
										else
										{
											bShowNoMoreScreensError = true;
										}
									}
								}
							}
						}
					}
					if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", pToolTipForAddingNewScreens);

					ImGui::SetCursorPosY(ImGui::GetCursorPosY() + groupspacer);
					//---- spacer ----


					ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((ImGui::GetContentRegionAvail().x * 0.5) - (buttonwide * 0.5), 0.0f));
					if (ImGui::StyleButton("Add New Screen", ImVec2(buttonwide, 0.0f)))
					{
						int iScreenCount = 1;
						for (int i = 0; i < STORYBOARD_MAXNODES; i++)
						{
							if (Storyboard.Nodes[i].used && Storyboard.Nodes[i].type == STORYBOARD_TYPE_SCREEN)
							{
								if(pestrcasestr(Storyboard.Nodes[i].title,"Custom Screen"))
									iScreenCount++;
							}
						}
						char cScreenCount[8];
						sprintf_s(cScreenCount, "%d", iScreenCount);
						// Find first free storyboard node that we can use for the new screen.
						int node = -1;

						int iUniqueIdsAdd = 1000;
						for (int i = 0; i < STORYBOARD_MAXNODES; i++)
						{
							if (i == 100 || i == 200)
								iUniqueIdsAdd += 100000;

							if (Storyboard.Nodes[i].used == 0)
							{
								// Reset node to default state, in case any old data remains.
								node = i;
								reset_single_node(node);

								//PE: Setup new unique id's
								int iUniqueId = STORYBOARD_THUMBS + node;
								Storyboard.Nodes[node].id = iUniqueId;
								Storyboard.Nodes[node].thumb_id = iUniqueId;
								for (int l = 0; l < STORYBOARD_MAXWIDGETS; l++)
								{
									//PE: input_id,output_id ID's broken in checkproject.
									Storyboard.Nodes[node].widget_normal_thumb_id[l] = iUniqueId + iUniqueIdsAdd + (1000 * l) + 600;
									Storyboard.Nodes[node].widget_highlight_thumb_id[l] = iUniqueId + iUniqueIdsAdd + (1000 * l) + 700;
									Storyboard.Nodes[node].widget_selected_thumb_id[l] = iUniqueId + iUniqueIdsAdd + (1000 * l) + 800;
								}
								for (int l = 0; l < STORYBOARD_MAXOUTPUTS; l++)
								{
									Storyboard.Nodes[node].input_id[l] = iUniqueId + iUniqueIdsAdd + (1000 * l);
									Storyboard.Nodes[node].output_id[l] = iUniqueId + iUniqueIdsAdd + (1000 * l) + 500;
								}

								Storyboard.Nodes[node].screen_backdrop_id = iUniqueId + 500;

								// New node defaults to a HUD screen
								Storyboard.Nodes[node].used = true;
								Storyboard.Nodes[node].type = STORYBOARD_TYPE_SCREEN;

								Storyboard.Nodes[node].restore_position = ImVec2(Storyboard.Nodes[iAboutScreenNodeID].restore_position.x + 200 * iScreenCount, Storyboard.Nodes[iAboutScreenNodeID].restore_position.y);

								ImNodes::SetNodeGridSpacePos(Storyboard.Nodes[node].id, Storyboard.Nodes[node].restore_position);
								Storyboard.Nodes[node].iEditEnable = true;
								strcpy(Storyboard.Nodes[node].title, "Custom Screen ");
								strcat(Storyboard.Nodes[node].title, cScreenCount);
								//strcpy(Storyboard.Nodes[node].thumb, "editors\\templates\\thumbs\\hud.lua.png");
								strcpy(Storyboard.Nodes[node].lua_name, "custom");
								strcat(Storyboard.Nodes[node].lua_name, cScreenCount);
								strcpy(Storyboard.Nodes[node].screen_backdrop, "");
								Storyboard.Nodes[node].screen_backdrop_transparent = true;
								Storyboard.Nodes[node].readouts_available = 0;
								Storyboard.Nodes[node].widgets_available = ALLOW_BUTTON | ALLOW_TEXT | ALLOW_IMAGE | ALLOW_RADIOTYPE | ALLOW_SLIDER | ALLOW_TICKBOX | ALLOW_VIDEO | ALLOW_PROGRESS | ALLOW_TEXTAREA;

								strcpy(Storyboard.Nodes[node].thumb, "editors\\templates\\thumbs\\screen_about.lua.png");
								strcpy(Storyboard.Nodes[node].screen_backdrop, "editors\\templates\\backdrops\\about.png");

								strcpy(Storyboard.Nodes[node].input_title[0], " Input ");
								int button = 0;
								strcpy(Storyboard.Nodes[node].widget_label[button], "BACK");
								Storyboard.Nodes[node].widget_used[button] = 1;
								Storyboard.Nodes[node].widget_type[button] = STORYBOARD_WIDGET_BUTTON;
								Storyboard.Nodes[node].widget_size[button] = ImVec2(1.0, 1.0); //Only for scaling. else but image size.
								Storyboard.Nodes[node].widget_pos[button] = ImVec2(50.0, 80); //Pos in percent. using pivot center on X only.
								Storyboard.Nodes[node].widget_action[button] = STORYBOARD_ACTIONS_BACK;
								Storyboard.Nodes[node].widget_layer[button] = 0;
								Storyboard.Nodes[node].widget_font_color[button] = ImVec4(1.0, 1.0, 1.0, 1.0);
								strcpy(Storyboard.Nodes[node].widget_font[button], "Default Font"); // ?
								strcpy(Storyboard.Nodes[node].widget_normal_thumb[button], "editors\\templates\\buttons\\default.png");
								strcpy(Storyboard.Nodes[node].widget_highlight_thumb[button], "editors\\templates\\buttons\\default-hover.png");
								strcpy(Storyboard.Nodes[node].widget_selected_thumb[button], "editors\\templates\\buttons\\default-selected.png");
								strcpy(Storyboard.Nodes[node].widget_name[button], "continue"); //NOTE: DUP (continue) - Also add "-hover.png" ...
								break;
							}
						}
						if (node < 0)
						{
							bShowNoMoreScreensError = true;
						}
						else
						{
							// Trigger creation of a new thumbnail for the newly created screen
							iWaitFor2DEditor = 5;
							iWaitFor2DEditorNode = node;
							if (BitmapExist(99))
							{
								DeleteBitmapEx(99);
							}
						}
					}
					if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", pToolTipForAddingNewScreens);
					
					ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((ImGui::GetContentRegionAvail().x * 0.5) - (buttonwide * 0.5), 0.0f));
					if (ImGui::StyleButton("Add New Loading Screen", ImVec2(buttonwide, 0.0f)))
					{
						int iLoadingScreenCount = 1;
						for (int i = 0; i < STORYBOARD_MAXNODES; i++)
						{
							if (Storyboard.Nodes[i].used && Storyboard.Nodes[i].type == STORYBOARD_TYPE_LEVEL)
							{
								if(pestrcasestr(Storyboard.Nodes[i].title,"Loading Screen"))
									iLoadingScreenCount++;
							}
						}
						char cLoadingScreenCount[8];
						sprintf_s(cLoadingScreenCount, "%d", iLoadingScreenCount);
						// Find first free storyboard node that we can use for the new screen.
						int node = -1;

						int iUniqueIdsAdd = 1000;
						for (int i = 0; i < STORYBOARD_MAXNODES; i++)
						{
							if (i == 100 || i == 200)
								iUniqueIdsAdd += 100000;
							if (Storyboard.Nodes[i].used == 0)
							{
								// Reset node to default state, in case any old data remains.
								node = i;
								reset_single_node(node);

								//PE: Setup new unique id's
								int iUniqueId = STORYBOARD_THUMBS + node;
								Storyboard.Nodes[node].id = iUniqueId;
								Storyboard.Nodes[node].thumb_id = iUniqueId;
								for (int l = 0; l < STORYBOARD_MAXWIDGETS; l++)
								{
									//PE: input_id,output_id ID's broken in checkproject.
									Storyboard.Nodes[node].widget_normal_thumb_id[l] = iUniqueId + iUniqueIdsAdd + (1000 * l) + 600;
									Storyboard.Nodes[node].widget_highlight_thumb_id[l] = iUniqueId + iUniqueIdsAdd + (1000 * l) + 700;
									Storyboard.Nodes[node].widget_selected_thumb_id[l] = iUniqueId + iUniqueIdsAdd + (1000 * l) + 800;
								}
								for (int l = 0; l < STORYBOARD_MAXOUTPUTS; l++)
								{
									Storyboard.Nodes[node].input_id[l] = iUniqueId + iUniqueIdsAdd + (1000 * l);
									Storyboard.Nodes[node].output_id[l] = iUniqueId + iUniqueIdsAdd + (1000 * l) + 500;
								}

								Storyboard.Nodes[node].screen_backdrop_id = iUniqueId + 500;

								//PE: New loading screen.
								Storyboard.Nodes[node].widgets_available = ALLOW_TEXT | ALLOW_IMAGE | ALLOW_VIDEO;
								Storyboard.Nodes[node].used = true;
								Storyboard.Nodes[node].type = STORYBOARD_TYPE_LEVEL;
								Storyboard.Nodes[node].restore_position = ImVec2(Storyboard.Nodes[iLoadingScreenNodeID].restore_position.x + 200 * iLoadingScreenCount, Storyboard.Nodes[iLoadingScreenNodeID].restore_position.y);
								ImNodes::SetNodeGridSpacePos(Storyboard.Nodes[node].id, Storyboard.Nodes[node].restore_position);

								Storyboard.Nodes[node].iEditEnable = true;
								strcpy(Storyboard.Nodes[node].title, "Loading Screen ");
								strcat(Storyboard.Nodes[node].title, cLoadingScreenCount);
								strcpy(Storyboard.Nodes[node].lua_name, "loading");
								strcat(Storyboard.Nodes[node].lua_name, cLoadingScreenCount);
								strcat(Storyboard.Nodes[node].lua_name, ".lua");

								strcpy(Storyboard.Nodes[node].thumb, "editors\\templates\\thumbs\\screen_loading.lua.png");
								strcpy(Storyboard.Nodes[node].screen_backdrop, "editors\\templates\\backdrops\\loading.png");

								//Input.
								strcpy(Storyboard.Nodes[node].input_title[0], " Input ");
								//Output.
								strcpy(Storyboard.Nodes[node].output_title[0], " LOAD LEVEL -> Connect to Level ");
								strcpy(Storyboard.Nodes[node].output_action[0], "loadlevel"); //Not defined this yet.
								Storyboard.Nodes[node].output_can_link_to_type[0] = STORYBOARD_TYPE_LEVEL;
								Storyboard.Nodes[node].output_linkto[0] = 0;

								int button = 0;
								strcpy(Storyboard.Nodes[node].widget_label[button], "LOADING LEVEL");
								Storyboard.Nodes[node].widget_used[button] = 1;
								Storyboard.Nodes[node].widget_type[button] = STORYBOARD_WIDGET_TEXT;
								Storyboard.Nodes[node].widget_size[button] = ImVec2(1.0, 1.0); //Only for scaling. else but image size.
								Storyboard.Nodes[node].widget_pos[button] = ImVec2(50.0, 80.0); //Pos in percent. using pivot center on X only.
								Storyboard.Nodes[node].widget_action[button] = STORYBOARD_ACTIONS_NONE;
								Storyboard.Nodes[node].widget_layer[button] = 0;
								Storyboard.Nodes[node].widget_font_color[button] = ImVec4(1.0, 1.0, 1.0, 1.0);
								strcpy(Storyboard.Nodes[node].widget_font[button], "Default Font"); // ?
								strcpy(Storyboard.Nodes[node].widget_name[button], "loading-text"); //Also add "-hover.png" ...

								button = 1;

								strcpy(Storyboard.Nodes[node].widget_label[button], ""); //Progressbar
								Storyboard.Nodes[node].widget_used[button] = 1;
								Storyboard.Nodes[node].widget_type[button] = STORYBOARD_WIDGET_PROGRESS;
								Storyboard.Nodes[node].widget_size[button] = ImVec2(1.0, 1.0); //Only for scaling. else but image size.
								Storyboard.Nodes[node].widget_pos[button] = ImVec2(50.0, 90.0); //Pos in percent. using pivot center on X only.
								Storyboard.Nodes[node].widget_action[button] = STORYBOARD_ACTIONS_NONE;
								Storyboard.Nodes[node].widget_layer[button] = 0;
								Storyboard.Nodes[node].widget_font_color[button] = ImVec4(1.0, 1.0, 1.0, 1.0);
								strcpy(Storyboard.Nodes[node].widget_font[button], "Default Font"); // ?
								strcpy(Storyboard.Nodes[node].widget_normal_thumb[button], "editors\\templates\\buttons\\slider-bar-empty.png");
								strcpy(Storyboard.Nodes[node].widget_highlight_thumb[button], "editors\\templates\\buttons\\slider-bar-full.png");


								button = 2;
								strcpy(Storyboard.Nodes[node].widget_label[button], "When in game, press the Escape key for controls and other settings.");
								Storyboard.Nodes[node].widget_used[button] = 1;
								Storyboard.Nodes[node].widget_type[button] = STORYBOARD_WIDGET_TEXT;
								Storyboard.Nodes[node].widget_size[button] = ImVec2(1.0, 1.0); //Only for scaling. else but image size.
								Storyboard.Nodes[node].widget_pos[button] = ImVec2(50.0, 95.0); //Pos in percent. using pivot center on X only.
								Storyboard.Nodes[node].widget_action[button] = STORYBOARD_ACTIONS_NONE;
								Storyboard.Nodes[node].widget_layer[button] = 0;
								Storyboard.Nodes[node].widget_font_color[button] = ImVec4(1.0, 1.0, 1.0, 1.0);
								Storyboard.Nodes[node].widget_font_size[button] = 0.5;
								strcpy(Storyboard.Nodes[node].widget_font[button], "Default Font"); // ?
								strcpy(Storyboard.Nodes[node].widget_name[button], "loading-text"); //Also add "-hover.png" ...

								break;
							}
						}
						if (node < 0)
						{
							bShowNoMoreScreensError = true;
						}
						else
						{
							// Trigger creation of a new thumbnail for the newly created screen
							iWaitFor2DEditor = 5;
							iWaitFor2DEditorNode = node;
							if (BitmapExist(99))
							{
								DeleteBitmapEx(99);
							}
						}
					}
					if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", pToolTipForAddingNewScreens);

					ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((ImGui::GetContentRegionAvail().x * 0.5) - (buttonwide * 0.5), 0.0f));
					if (ImGui::StyleButton("Add New HUD Screen", ImVec2(buttonwide, 0.0f)))
					{
						int node = process_createanewhudscreen(10);
						if (node < 0)
						{
							bShowNoMoreScreensError = true;
						}
					}
					if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", pToolTipForAddingNewScreens);

					ImGui::SetCursorPosY(ImGui::GetCursorPosY() + groupspacer);
					//---- spacer ----


					if (ClassicConversion > 0 && bShowNoMoreScreensError == false)
					{
						if (ClassicConversion <= 3)
						{
							strcpy(cTriggerMessage, "Importing Classic Level ...");
							bTriggerMessage = true;
							iTriggerMessageDelay = 0;
							iTriggerMessageY = 0;
							if(ClassicConversion == 1) iMessageTimer = 0;
							ClassicConversion++;
						}
						else if (ClassicConversion == 4)
						{
							//PE: Load fpm from original location.
							g.projectfilename_s = sNextLevelToLoad;

							extern bool g_bAllowBackwardCompatibleConversion;
							g_bAllowBackwardCompatibleConversion = true;
							extern bool g_bDisplayWarnings;
							g_bDisplayWarnings = false;
							gridedit_load_map();
							g_bDisplayWarnings = true;
							g_bAllowBackwardCompatibleConversion = false;
							strcpy(cTriggerMessage, "Converting Settings ...");
							bTriggerMessage = true;
							iTriggerMessageDelay = 0;
							iTriggerMessageY = 0;
							iMessageTimer = 0;

							ClassicConversion++;
						}
						else if (ClassicConversion == 5)
						{
							//PE: Converting settings.
							//PE: ALL ai_ lua scripts will not work in wicked.
							extern bool g_bMakingAStandaloneUsingFileCollectionArray;
							if (g_bMakingAStandaloneUsingFileCollectionArray == false)
							{
								Undim(t.filecollection_s);
								g.filecollectionmax = 0;
								Dim(t.filecollection_s, 500);

								for (int i = 1; i <= g.entityelementmax; i++)
								{
									t.e = i;
									int masterid = t.entityelement[t.e].bankindex;
									if (masterid > 0)
									{
										if (t.entityprofile[masterid].ischaractercreator == 1)
										{
											//Delete Old Character Creator
											t.tentitytoselect = t.e;
											g_UndoSysObjectIsBeingMoved = false;
											entity_deleteentityfrommap();
											//gridedit_deleteentityfrommap();
										}
										if (t.entityelement[t.e].eleprof.aimain_s.Len() > 0)
										{
											t.entityelement[t.e].eleprof.soundset5_s = t.entityelement[t.e].eleprof.soundset4_s;
											t.entityelement[t.e].eleprof.soundset4_s = "";

											//PE: Map scripts.
											if (t.entityelement[t.e].eleprof.aimain_s != "no_behavior_selected.lua")
											{
												char script[MAX_PATH];

												strcpy(script, t.entityelement[t.e].eleprof.aimain_s.Get());
												bool bIncludeMarker = pestrcasestr(script, "markers");
												bool bIncludeObjects = pestrcasestr(script, "objects");

												//PE: Convert old script to new DLUA versions.
												if (!bIncludeMarker && pestrcasestr(script, "winzone.lua")) t.entityelement[t.e].eleprof.aimain_s = "markers\\winzone.lua";
												else if (!bIncludeMarker && pestrcasestr(script, "teleport.lua")) t.entityelement[t.e].eleprof.aimain_s = "markers\\teleport.lua";
												else if (!bIncludeMarker && pestrcasestr(script, "plrinzone.lua")) t.entityelement[t.e].eleprof.aimain_s = "markers\\plrinzone.lua";
												else if (!bIncludeMarker && pestrcasestr(script, "FlameLight.lua")) t.entityelement[t.e].eleprof.aimain_s = "markers\\FlameLight.lua";
												else if (!bIncludeMarker && pestrcasestr(script, "soundinzone.lua")) t.entityelement[t.e].eleprof.aimain_s = "markers\\soundinzone.lua";
												else if (!bIncludeMarker && pestrcasestr(script, "ToggleLight.lua")) t.entityelement[t.e].eleprof.aimain_s = "markers\\ToggleLight.lua";
												else if (!bIncludeMarker && pestrcasestr(script, "StrobeLight.lua")) t.entityelement[t.e].eleprof.aimain_s = "markers\\StrobeLight.lua";
												else if (!bIncludeMarker && pestrcasestr(script, "FlickerLight.lua")) t.entityelement[t.e].eleprof.aimain_s = "markers\\FlickerLight.lua";
												else if (!bIncludeMarker && pestrcasestr(script, "RotateLight.lua")) t.entityelement[t.e].eleprof.aimain_s = "markers\\RotateLight.lua";
												else if (!bIncludeMarker && pestrcasestr(script, "ambienceinzone.lua")) t.entityelement[t.e].eleprof.aimain_s = "markers\\ambienceinzone.lua";
												else if (!bIncludeMarker && pestrcasestr(script, "imageinzone.lua")) t.entityelement[t.e].eleprof.aimain_s = "markers\\imageinzone.lua";
												else if (!bIncludeMarker && pestrcasestr(script, "fadezone.lua")) t.entityelement[t.e].eleprof.aimain_s = "markers\\fadezone.lua";
												else if (!bIncludeMarker && pestrcasestr(script, "electrocute.lua")) t.entityelement[t.e].eleprof.aimain_s = "markers\\electrocute.lua";
												else if (!bIncludeMarker && pestrcasestr(script, "textinzone.lua")) t.entityelement[t.e].eleprof.aimain_s = "markers\\textinzone.lua";
												else if (!bIncludeMarker && pestrcasestr(script, "envirozone.lua")) t.entityelement[t.e].eleprof.aimain_s = "markers\\envirozone.lua";
												else if (!bIncludeMarker && pestrcasestr(script, "hurt.lua")) t.entityelement[t.e].eleprof.aimain_s = "markers\\hurt.lua";
												else if (!bIncludeMarker && pestrcasestr(script, "npcinzone.lua")) t.entityelement[t.e].eleprof.aimain_s = "markers\\npcinzone.lua";
												else if (!bIncludeMarker && pestrcasestr(script, "heal.lua")) t.entityelement[t.e].eleprof.aimain_s = "markers\\heal.lua";
												else if (!bIncludeMarker && pestrcasestr(script, "stealthzone.lua")) t.entityelement[t.e].eleprof.aimain_s = "markers\\stealthzone.lua";
												else if (!bIncludeMarker && pestrcasestr(script, "watercontrol.lua")) t.entityelement[t.e].eleprof.aimain_s = "markers\\watercontrol.lua";
												else if (!bIncludeMarker && pestrcasestr(script, "slip.lua")) t.entityelement[t.e].eleprof.aimain_s = "markers\\slip.lua";
												else if (!bIncludeMarker && pestrcasestr(script, "bounce.lua")) t.entityelement[t.e].eleprof.aimain_s = "markers\\bounce.lua";
												else if (!bIncludeMarker && pestrcasestr(script, "FreezePlayer.lua")) t.entityelement[t.e].eleprof.aimain_s = "markers\\FreezePlayer.lua";
												else if (!bIncludeMarker && pestrcasestr(script, "videoinzone.lua")) t.entityelement[t.e].eleprof.aimain_s = "markers\\videoinzone.lua";
												else if (!bIncludeMarker && pestrcasestr(script, "checkpoint.lua")) t.entityelement[t.e].eleprof.aimain_s = "markers\\checkpoint.lua";
												else if (!bIncludeMarker && pestrcasestr(script, "ambienceonceinzone.lua")) t.entityelement[t.e].eleprof.aimain_s = "markers\\ambienceonceinzone.lua";
												else if (!bIncludeMarker && pestrcasestr(script, "soundrepeatinzone.lua")) t.entityelement[t.e].eleprof.aimain_s = "markers\\soundrepeatinzone.lua";
												else if (!bIncludeMarker && pestrcasestr(script, "module_lightcontrol.lua")) t.entityelement[t.e].eleprof.aimain_s = "markers\\module_lightcontrol.lua";
												else if (!bIncludeMarker && pestrcasestr(script, "ConstantLight.lua")) t.entityelement[t.e].eleprof.aimain_s = "markers\\ConstantLight.lua";
												else if (!bIncludeMarker && pestrcasestr(script, "sendpulse.lua")) t.entityelement[t.e].eleprof.aimain_s = "markers\\sendpulse.lua";
												else if (!bIncludeMarker && pestrcasestr(script, "particle.lua")) t.entityelement[t.e].eleprof.aimain_s = "markers\\particle.lua";
												//PE: new DLUA in Objects.
												else if (!bIncludeObjects && pestrcasestr(script, "proximity_mine.lua")) t.entityelement[t.e].eleprof.aimain_s = "objects\\proximity_mine.lua";
												else if (!bIncludeObjects && pestrcasestr(script, "ladder.lua")) t.entityelement[t.e].eleprof.aimain_s = "objects\\ladder.lua";
												else if (!bIncludeObjects && pestrcasestr(script, "healthbar.lua")) t.entityelement[t.e].eleprof.aimain_s = "objects\\healthbar.lua";
												else if (!bIncludeObjects && pestrcasestr(script, "change_texture.lua")) t.entityelement[t.e].eleprof.aimain_s = "objects\\change_texture.lua";
												else if (!bIncludeObjects && pestrcasestr(script, "door_rotate.lua")) t.entityelement[t.e].eleprof.aimain_s = "objects\\door_rotate.lua";
												else if (!bIncludeObjects && pestrcasestr(script, "secmon.lua")) t.entityelement[t.e].eleprof.aimain_s = "objects\\secmon.lua";
												else if (!bIncludeObjects && pestrcasestr(script, "winswitch.lua")) t.entityelement[t.e].eleprof.aimain_s = "objects\\winswitch.lua";
												else if (!bIncludeObjects && pestrcasestr(script, "spin.lua")) t.entityelement[t.e].eleprof.aimain_s = "objects\\spin.lua";
												else if (!bIncludeObjects && pestrcasestr(script, "dynamite.lua")) t.entityelement[t.e].eleprof.aimain_s = "objects\\dynamite.lua";
												else if (!bIncludeObjects && pestrcasestr(script, "seccam.lua")) t.entityelement[t.e].eleprof.aimain_s = "objects\\seccam.lua";
												else if (!bIncludeObjects && pestrcasestr(script, "hideshow.lua")) t.entityelement[t.e].eleprof.aimain_s = "objects\\hideshow.lua";
												else if (!bIncludeObjects && pestrcasestr(script, "boat.lua")) t.entityelement[t.e].eleprof.aimain_s = "objects\\boat.lua";
												else if (!bIncludeObjects && pestrcasestr(script, "door_sliding.lua")) t.entityelement[t.e].eleprof.aimain_s = "objects\\door_sliding.lua";
												else if (!bIncludeObjects && pestrcasestr(script, "carry_object.lua")) t.entityelement[t.e].eleprof.aimain_s = "objects\\carry_object.lua";
												else if (!bIncludeObjects && pestrcasestr(script, "door.lua"))
												{
													//door_properties(0\"70\",0\"1000\",3\"0\",2\"Door locked. Find key\",3\"0\",2\"Press E To Open\")
													//g_Entity[e]['haskey'] ?
													t.entityelement[t.e].eleprof.soundset4_s = "door_properties(0\"70\",0\"1000\",3\"1\",2\"Door locked. Find key\",3\"0\",2\"Press E To Open\")";
													t.entityelement[t.e].eleprof.aimain_s = "objects\\door.lua";
												}
												else if (!bIncludeObjects && pestrcasestr(script, "decalshow.lua")) t.entityelement[t.e].eleprof.aimain_s = "objects\\decalshow.lua";
												else if (!bIncludeObjects && pestrcasestr(script, "sentry.lua")) t.entityelement[t.e].eleprof.aimain_s = "objects\\sentry.lua";
												else if (!bIncludeObjects && pestrcasestr(script, "mines.lua")) t.entityelement[t.e].eleprof.aimain_s = "objects\\mines.lua";
												else if (!bIncludeObjects && pestrcasestr(script, "proximine.lua")) t.entityelement[t.e].eleprof.aimain_s = "objects\\mines.lua";
												else if (!bIncludeObjects && pestrcasestr(script, "face_object.lua")) t.entityelement[t.e].eleprof.aimain_s = "objects\\face_object.lua";
												else if (!bIncludeObjects && pestrcasestr(script, "document.lua")) t.entityelement[t.e].eleprof.aimain_s = "objects\\document.lua";
												else if (!bIncludeObjects && pestrcasestr(script, "switch.lua")) t.entityelement[t.e].eleprof.aimain_s = "objects\\switch.lua";
												else if (!bIncludeObjects && pestrcasestr(script, "move_near.lua")) t.entityelement[t.e].eleprof.aimain_s = "objects\\move_near.lua";
												else if (!bIncludeObjects && pestrcasestr(script, "invisible.lua")) t.entityelement[t.e].eleprof.aimain_s = "objects\\invisible.lua";
												else if (!bIncludeObjects && pestrcasestr(script, "move_away.lua")) t.entityelement[t.e].eleprof.aimain_s = "objects\\move_away.lua";
												else if (!bIncludeObjects && pestrcasestr(script, "loopwaypoint.lua")) t.entityelement[t.e].eleprof.aimain_s = "objects\\loopwaypoint.lua";
												else if (!bIncludeObjects && pestrcasestr(script, "invisibleprompt.lua")) t.entityelement[t.e].eleprof.aimain_s = "objects\\invisibleprompt.lua";
												else if (!bIncludeObjects && pestrcasestr(script, "hover.lua")) t.entityelement[t.e].eleprof.aimain_s = "objects\\hover.lua";

												else if (!bIncludeObjects && pestrcasestr(script, "health.lua")) t.entityelement[t.e].eleprof.aimain_s = "rpg\\health.lua";
												else if (!bIncludeObjects && pestrcasestr(script, "radar.lua")) t.entityelement[t.e].eleprof.aimain_s = "rpg\\radar.lua";

												else if (pestrcasestr(script, "invisible_wall.lua"))
													t.entityelement[t.e].eleprof.aimain_s = "objects\\invisible.lua";
												else if (pestrcasestr(script, "ai_") && t.entityprofile[masterid].ischaracter == 1)
												{
													//PE: No old AI_ is working in max.
													t.entityelement[t.e].eleprof.aimain_s = "";
												}
												else if (pestrcasestr(script, "stories\\T"))
												{
													//PE: TBE stories dont work., LoadImages("The Big Escape",0)
													t.entityelement[t.e].eleprof.aimain_s = "";
												}
												else
												{
													//PE: Missing. copy to docwrite script folder , old lua script might work.
													//PE: Check if its already in max scripts.
													char scriptfile[MAX_PATH];
													strcpy(scriptfile, "scriptbank\\");
													strcat(scriptfile, t.entityelement[t.e].eleprof.aimain_s.Get());
													if (!FileExist(scriptfile))
													{
														char WriteTo[MAX_PATH];
														char ReadFrom[MAX_PATH];
														strcpy(WriteTo, scriptfile);
														GG_GetRealPath(WriteTo, 1);
														strcpy(ReadFrom, pReconstructGameGuruRootFiles);
														strcat(ReadFrom, scriptfile);
														bool bRet = CopyFileA(ReadFrom, WriteTo, TRUE);
														scanscriptfileandaddtocollection(ReadFrom, &pReconstructGameGuruRootFiles[0]);
													}
												}
											}
										}

										if (t.entityprofile[masterid].model_s.Len() > 0)
										{
											//PE: 24 bits dds.
											if (pestrcasestr(t.entityprofile[masterid].model_s.Get(), "FireFlies.X"))
											{
												//Remove.
												t.entityelement[t.e].bankindex = 0;
											}
										}
										char pGameCoreAsset[MAX_PATH];
										strcpy(pGameCoreAsset, "");
										//#define INCLUDECLASSICWEAPONS
	#ifdef INCLUDECLASSICWEAPONS
										if (t.entityprofile[masterid].ischaracter == 1)
										{
											strcpy(pGameCoreAsset, t.entityprofile[masterid].hasweapon_s.Get());
										}
										if (t.entityprofile[masterid].ismarker == 1)
										{
											strcpy(pGameCoreAsset, t.entityelement[t.e].eleprof.hasweapon_s.Get());
										}
	#endif
										if (strlen(pGameCoreAsset) > 0)
										{
											char pGameCoreFolder[MAX_PATH];
											strcpy(pGameCoreFolder, pGameCoreAsset);
											cstr pOldDir = GetDir();
											char pSrcFolder[MAX_PATH];
											strcpy(pSrcFolder, pReconstructGameGuruRootFiles);
											strcat(pSrcFolder, "gamecore\\guns\\");
											strcat(pSrcFolder, pGameCoreFolder);
											if (PathExist(pSrcFolder))
											{
												SetDir(pSrcFolder);
												ChecklistForFiles();
												SetDir(pOldDir.Get());
												strcat(pGameCoreFolder, "\\");
												strcat(pSrcFolder, "\\");
												for (int c = 1; c <= ChecklistQuantity(); c++)
												{
													LPSTR pFileName = ChecklistString(c);
													if (strcmp(pFileName, ".") != NULL && strcmp(pFileName, "..") != NULL)
													{
														char pSrcFile[MAX_PATH];
														char pDestFile[MAX_PATH];
														strcpy(pSrcFile, pSrcFolder);
														strcat(pSrcFile, pFileName);
														strcpy(pDestFile, "gamecore\\guns\\");
														strcat(pDestFile, pGameCoreFolder);
														strcat(pDestFile, pFileName);
														if (!FileExist(pDestFile))
														{
															GG_GetRealPath(pDestFile, 1);
															CopyFileA(pSrcFile, pDestFile, TRUE);
														}
													}
												}
											}
										}

									}
								}
								if (g.filecollectionmax > 0)
								{
									for (int i = 0; i <= g.filecollectionmax; i++)
									{
										LPSTR pThisFile = t.filecollection_s[i].Get();
										int iThisSize = strlen(pThisFile);
										if (iThisSize > 0)
										{
											// must have a file specified
											if (pThisFile[iThisSize - 1] == '\\' || pThisFile[iThisSize - 1] == '/')
											{
												// ignore folders
											}
											else
											{
												char ReadFrom[MAX_PATH];
												strcpy(ReadFrom, pReconstructGameGuruRootFiles);
												strcat(ReadFrom, pThisFile);
												if (FileExist(ReadFrom) == 1)
												{
													char WriteTo[MAX_PATH];
													strcpy(WriteTo, pThisFile);
													if (!FileExist(pThisFile))
													{
														GG_GetRealPath(WriteTo, 1);
														bool bRet = CopyFileA(ReadFrom, WriteTo, TRUE);
													}
												}
											}
										}
									}
									Undim(t.filecollection_s);
									g.filecollectionmax = 0;
									Dim(t.filecollection_s, 500);
								}
							}

							t.tentitytoselect = 0;
							strcpy(cTriggerMessage, "Creating New Level ...");
							bTriggerMessage = true;
							iTriggerMessageDelay = 0;
							iTriggerMessageY = 0;
							iMessageTimer = 0;

							ClassicConversion++;
						}
						else if (ClassicConversion == 6)
						{
							//PE: Check for missing .dbo files.
							extern char szWriteDir[MAX_PATH];
							extern char g_pAbsPathToConverter[MAX_PATH];

							char pOldDir[MAX_PATH];
							GetCurrentDirectoryA(MAX_PATH, pOldDir);

							SetDir(szWriteDir);
							HINSTANCE hinstance = ShellExecuteA(NULL, "open", g_pAbsPathToConverter, "", "", SW_SHOWDEFAULT);
							Sleep(3000);
							SetDir(pOldDir);

							//PE: saving new level.
							char pNewGameGuruLevel[MAX_PATH];
							strcpy(pNewGameGuruLevel, g.projectfilename_s.Get());
							char* level = (char *) pestrcasestr(pNewGameGuruLevel, "mapbank\\");
							if (level > 0)
							{
								char tmp[MAX_PATH];
								strcpy(tmp, g.mysystem.mapbankAbs_s.Get());
								//Relative.
								char* find = (char*)pestrcasestr(tmp, "mapbank\\");
								if (find && find != &tmp[0]) strcpy(&tmp[0], find);
								strcat(tmp, level+8);

								t.returnstring_s = tmp;
								g.projectfilename_s = tmp;
								g.projectmodifiedstatic = 1;
								g.projectmodifiedstatic = 1;

								for (t.e = 1; t.e <= g.entityelementlist; t.e++)
								{
									t.entid = t.entityelement[t.e].bankindex;
									if (t.entityprofile[t.entid].ismarker == 1)
									{
										//PE: set camera to Player Start Marker Settings
										PositionCamera( t.entityelement[t.e].x, t.entityelement[t.e].y+50, t.entityelement[t.e].z);
										t.cx_f = t.editorfreeflight.c.x_f = GetCameraPosition().x;
										t.editorfreeflight.c.y_f = GetCameraPosition().y;
										t.cy_f = t.editorfreeflight.c.z_f = GetCameraPosition().z;
										break;
									}
								}

								gridedit_save_map();
								g.projectmodified = 0; gridedit_changemodifiedflag();
								g.projectmodifiedstatic = 0;
								ClassicConversion++;
								strcpy(cTriggerMessage, "Loading Converted Level ...");
								bTriggerMessage = true;
								iMessageTimer = 0;
								iTriggerMessageDelay = 0;
								iTriggerMessageY = 0;
							}
							else
							{
								ClassicConversion = 10;
							}
						}
						else if (ClassicConversion == 7)
						{
							strcpy(cTriggerMessage, "Loading Converted Level ...");
							bTriggerMessage = true;
							iMessageTimer = 0;
							iTriggerMessageDelay = 0;
							iTriggerMessageY = 0;

							ClassicConversion++;
						}
						else if (ClassicConversion == 8)
						{
							//PE: Loading new converted map.

							char tmp[MAX_PATH];
							strcpy(tmp, g.projectfilename_s.Get());

							char* find = (char*)pestrcasestr(tmp, "mapbank\\");
							if (find && find != &tmp[0]) strcpy(&tmp[0], find);

							int iPos;
							for (iPos = strlen(tmp); iPos >= 0; iPos--)
								if (tmp[iPos] == '\\') break;
							if (iPos > 0) iPos++;
							std::string sLevelTitle = &tmp[iPos];
							replaceAll(sLevelTitle, ".fpm", "");

							std::string sLevelPath = &tmp[0];

							//PE: Find next level from nodes.
							int iNextLevel = 0, levelname = -1, iFirstNodeFree = -1;
							FindFreeLevelNode(iNextLevel, levelname, iFirstNodeFree);

							if (iFirstNodeFree >= 0)
							{
								//Create new level.
								char tmp[255];
								int node = iFirstNodeFree;
								int nodeposy = iNextLevel;
								if (levelname > 0)
								{
									sprintf(tmp, "Level %d", levelname);
									nodeposy = levelname - 1;
								}
								else
									sprintf(tmp, "Level %d", iNextLevel + 1);

								//PE: Make sure any old data is removed, also thumbs.
								reset_single_node(node);

								Storyboard.Nodes[node].used = true;
								Storyboard.Nodes[node].type = STORYBOARD_TYPE_LEVEL;
								Storyboard.Nodes[node].restore_position = ImVec2(preview_size_x * 0.5 - (fNodeWidth * 0.5) + ((fNodeWidth + NODE_WIDTH_PADDING) * 2.0), STORYBOARD_YSTART + ((fNodeHeight + 20.0 + NODE_HEIGHT_PADDING) * (nodeposy)));
								Storyboard.Nodes[node].iEditEnable = true;
								strcpy(Storyboard.Nodes[node].title, sLevelTitle.c_str());
								strcpy(Storyboard.Nodes[node].level_name, sLevelPath.c_str());
								strcpy(Storyboard.Nodes[node].levelnumber, tmp);

								strcpy(Storyboard.Nodes[node].thumb, "");
								//Input.
								strcpy(Storyboard.Nodes[node].input_title[0], " Input ");
								//Output.
								strcpy(Storyboard.Nodes[node].output_title[0], " WIN LEVEL -> Connect to Scene ");
								strcpy(Storyboard.Nodes[node].output_action[0], "loadlevel"); //Not defined this yet.
								Storyboard.Nodes[node].output_can_link_to_type[0] = STORYBOARD_TYPE_SCREEN;
								Storyboard.Nodes[node].output_linkto[0] = 0;

								strcpy(Storyboard.Nodes[node].output_title[1], " GAME OVER -> Connect to Scene ");
								strcpy(Storyboard.Nodes[node].output_action[1], "loadlevel"); //Not defined this yet.
								Storyboard.Nodes[node].output_can_link_to_type[1] = STORYBOARD_TYPE_SCREEN;
								Storyboard.Nodes[node].output_linkto[1] = 0;

								strcpy(Storyboard.Nodes[node].output_title[2], " NEXT LEVEL -> Connect to Level ");
								strcpy(Storyboard.Nodes[node].output_action[2], "loadlevel"); //Not defined this yet.
								Storyboard.Nodes[node].output_can_link_to_type[2] = STORYBOARD_TYPE_LEVEL;
								Storyboard.Nodes[node].output_linkto[2] = 0;
								ImNodes::SetNodeGridSpacePos(Storyboard.Nodes[node].id, Storyboard.Nodes[node].restore_position);
								iAutoConnectNode = node;
								//Check if level already got a thumb.
								CreateBackBufferCacheNameEx(Storyboard.Nodes[node].level_name, 512, 288, true);
								if (FileExist(BackBufferCacheName.Get()))
								{
									if (CopyToProjectFolder(BackBufferCacheName.Get()))
									{
										//PE: Use relative projectbank filename.
										if (FileExist(ProjectCacheName.Get()))
											BackBufferCacheName = ProjectCacheName;
									}

									//PE: Load in old thumb.
									SetMipmapNum(1); //PE: mipmaps not needed.
									image_setlegacyimageloading(true);
									LoadImageSize(BackBufferCacheName.Get(), Storyboard.Nodes[node].thumb_id, 512, 288);
									image_setlegacyimageloading(false);
									SetMipmapNum(-1); //PE: mipmaps not needed.
									if (ImageExist(Storyboard.Nodes[node].thumb_id))
									{
										//PE: Success update thumb filename.
										strcpy(Storyboard.Nodes[node].thumb, BackBufferCacheName.Get());
									}
								}
							}

							gridedit_load_map();
							bUpdateVeg = true;
							ClassicConversion++;
						}
						else if (ClassicConversion == 9)
						{
							ClassicConversion = 0;
						}

						if (ClassicConversion == 10)
						{
							//Failed.
							strcpy(cTriggerMessage, "ERROR: Importing Classic Level Failed!");
							bTriggerMessage = true;
							iMessageTimer = 0;
							iTriggerMessageDelay = 0;
							iTriggerMessageY = 0;
							ClassicConversion = 0;
						}
					}
					#ifdef INCLUDE_GAME_SETTINGS
					if (pref.iStoryboardAdvanced)
					{
						ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((ImGui::GetContentRegionAvail().x * 0.5) - (buttonwide * 0.5), 0.0f));
						if (ImGui::StyleButton("Add RPG HUD Screens", ImVec2(buttonwide, 0.0f)))
						{
							g_bRefreshGlobalList = true;

							// check if RPG screens already exist
							int iCountNewHUDScreensNeeded = 0;
							bool bRPGHUDSMissing[10];
							memset(bRPGHUDSMissing, 0, sizeof(bRPGHUDSMissing));
							bool bAllRPGScreensAlreadyExist = true;
							for (int hudi = 1; hudi <= 9; hudi++) // include RPG Templates VR screen :)
							{
								// assume HUD screen missing
								bRPGHUDSMissing[hudi] = true;
								char pTitleLabel[256];
								if (hudi == 1)
								{
									sprintf(pTitleLabel, "In-Game HUD");
								}
								else
								{
									sprintf(pTitleLabel, "HUD Screen %d", hudi);
								}
								for (int i = 0; i < STORYBOARD_MAXNODES; i++)
								{
									if (Storyboard.Nodes[i].used)
									{
										if (Storyboard.Nodes[i].type == STORYBOARD_TYPE_HUD)
										{
											if (pestrcasestr(Storyboard.Nodes[i].title, pTitleLabel))
											{
												// found the HUD, not missing
												bRPGHUDSMissing[hudi] = false;
												break;
											}
										}
									}
								}
								if (bRPGHUDSMissing[hudi] == true)
								{
									iCountNewHUDScreensNeeded++;
									bAllRPGScreensAlreadyExist = false;
								}
							}
							if (bAllRPGScreensAlreadyExist == false && iCountNewHUDScreensNeeded > 0)
							{
								// need to find this many free nodes, or throw error
								int iCountRemainingFreeOnes = 0;
								for (int i = 14; i < STORYBOARD_MAXNODES; i++)
								{
									if (Storyboard.Nodes[i].used == 0)
									{
										iCountRemainingFreeOnes++;
									}
								}
								if (iCountRemainingFreeOnes < iCountNewHUDScreensNeeded)
								{
									bShowNoMoreScreensError = true;
								}
							}
							if (bAllRPGScreensAlreadyExist == false && bShowNoMoreScreensError == false)
							{
								// load in template screens from "RPG Template" project
								char project[MAX_PATH];

								sprintf(project, "projectbank\\RPG Template\\project%d.dat", STORYBOARDVERSION);
								FILE* projectfile = GG_fopen(project, "rb");
								if (!projectfile)
								{
									strcpy(project, "projectbank\\RPG Template\\project.dat");
									projectfile = GG_fopen(project, "rb");
								}
								if (projectfile)
								{
									StoryboardStruct* check_project = nullptr;
									memset(&tempProjectData, 0, sizeof(StoryboardStruct));
									fclose(projectfile);

									//PE: Use this so we can upgrade from 202 to 203+
									bool load__storyboard_into_struct(const char*, StoryboardStruct&);
									load__storyboard_into_struct(project, tempProjectData);
									check_project = &tempProjectData;


									//size_t size = fread(check_project, 1, sizeof(StoryboardStruct), projectfile);
									char sig[12] = "Storyboard\0";
									if (check_project->sig[0] == 'S' && check_project->sig[8] == 'r')
									{
										// go through and find all HUD Screens related to RPG
										for (int i = 0; i < STORYBOARD_MAXNODES; i++)
										{
											if (check_project->Nodes[i].used)
											{
												if (check_project->Nodes[i].type == STORYBOARD_TYPE_HUD)
												{
													//for (int hudi = 1; hudi <= 8; hudi++)
													for (int hudi = 1; hudi <= 9; hudi++)
													{
														// only add those that are missing
														if (bRPGHUDSMissing[hudi] == true)
														{
															char pTitleLabel[256];
															if (hudi == 1)
																sprintf(pTitleLabel, "In-Game HUD");
															else
																sprintf(pTitleLabel, "HUD Screen %d", hudi);
															if (pestrcasestr(check_project->Nodes[i].title, pTitleLabel))
															{
																// find spare node
																int newnodeid = 0;
																for (int i = 14; i < STORYBOARD_MAXNODES; i++)
																{
																	if (Storyboard.Nodes[i].used == 0)
																	{
																		newnodeid = i;
																		break;
																	}
																}
																if (newnodeid > 0)
																{
																	// must use unique NODEIDs for IMNODE
																	int iFoundNodeID = 0;
																	int iTryNodeID = STORYBOARD_THUMBS;
																	while (iFoundNodeID == 0)
																	{
																		int i = 0;
																		for (; i < STORYBOARD_MAXNODES; i++)
																		{
																			if (Storyboard.Nodes[i].used)
																			{
																				if (Storyboard.Nodes[i].id == iTryNodeID)
																				{
																					iTryNodeID++;
																					break;
																				}
																			}
																		}
																		if (i >= STORYBOARD_MAXNODES)
																		{
																			iFoundNodeID = iTryNodeID;
																		}
																	}
																	if (iFoundNodeID > 0)
																	{
																		// copy node from RPG Template project to current storyboard
																		Storyboard.Nodes[newnodeid] = check_project->Nodes[i];
																		Storyboard.Nodes[newnodeid].id = iFoundNodeID;
																		Storyboard.NodeRadioButtonSelected[newnodeid] = check_project->NodeRadioButtonSelected[i];
																		for (int iWidgetIndex = 0; iWidgetIndex < STORYBOARD_MAXWIDGETS; iWidgetIndex++)
																		{
																			Storyboard.NodeSliderValues[newnodeid][iWidgetIndex] = check_project->NodeSliderValues[i][iWidgetIndex];
																			Storyboard.widget_colors[newnodeid][iWidgetIndex] = check_project->widget_colors[i][iWidgetIndex];
																			for (int n = 0; n < 128; n++)
																			{
																				Storyboard.widget_readout[newnodeid][iWidgetIndex][n] = check_project->widget_readout[i][iWidgetIndex][n];
																			}
																			Storyboard.widget_textoffset[newnodeid][iWidgetIndex] = check_project->widget_textoffset[i][iWidgetIndex];
																			Storyboard.widget_ingamehidden[newnodeid][iWidgetIndex] = check_project->widget_ingamehidden[i][iWidgetIndex];
																			Storyboard.widget_drawordergroup[newnodeid][iWidgetIndex] = check_project->widget_drawordergroup[i][iWidgetIndex];
																		}

																		//PE: unique ids are wrong in checkproject so assign new here.
																		int iUniqueId = STORYBOARD_THUMBS + newnodeid;
																		int iUniqueIdsAdd = 1000;
																		if (newnodeid >= 200)
																			iUniqueIdsAdd = 200000;
																		else if (newnodeid >= 100)
																			iUniqueIdsAdd = 100000;

																		Storyboard.Nodes[newnodeid].id = iUniqueId;
																		Storyboard.Nodes[newnodeid].thumb_id = iUniqueId;
																		for (int l = 0; l < STORYBOARD_MAXWIDGETS; l++)
																		{
																			//PE: input_id,output_id ID's broken in checkproject.
																			Storyboard.Nodes[newnodeid].widget_normal_thumb_id[l] = iUniqueId + iUniqueIdsAdd + (1000 * l) + 600;
																			Storyboard.Nodes[newnodeid].widget_highlight_thumb_id[l] = iUniqueId + iUniqueIdsAdd + (1000 * l) + 700;
																			Storyboard.Nodes[newnodeid].widget_selected_thumb_id[l] = iUniqueId + iUniqueIdsAdd + (1000 * l) + 800;
																		}
																		for (int l = 0; l < STORYBOARD_MAXOUTPUTS; l++)
																		{
																			Storyboard.Nodes[newnodeid].input_id[l] = iUniqueId + iUniqueIdsAdd + (1000 * l);
																			Storyboard.Nodes[newnodeid].output_id[l] = iUniqueId + iUniqueIdsAdd + (1000 * l) + 500;
																		}
																		Storyboard.Nodes[newnodeid].screen_backdrop_id = iUniqueId + 500;

																		// update thumbs
																		SetMipmapNum(1);
																		image_setlegacyimageloading(true);
																		LoadImageSize(Storyboard.Nodes[newnodeid].thumb, Storyboard.Nodes[newnodeid].thumb_id, 512, 288);
																		image_setlegacyimageloading(false);
																		SetMipmapNum(-1);

																		// indicate a change
																		ImNodes::SetNodeGridSpacePos(Storyboard.Nodes[newnodeid].id, Storyboard.Nodes[newnodeid].restore_position);
																		Storyboard.iChanged = true;
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
							}
						}
						if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", pToolTipForAddingNewScreens);

						ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((ImGui::GetContentRegionAvail().x * 0.5) - (buttonwide * 0.5), 0.0f));
						if (ImGui::StyleButton("Reset All HUD Screens", ImVec2(buttonwide, 0.0f)))
						{
							int iAction = askBoxCancel("This will reset ALL HUD screens, are you sure?", "Confirmation"); //1==Yes 2=Cancel 0=No
							if (iAction == 1)
							{
								// Force a reset of the In-Game HUD screen, and reset node position
								for (int i = 0; i < STORYBOARD_MAXNODES; i++)
								{
									if (Storyboard.Nodes[i].used && Storyboard.Nodes[i].type == STORYBOARD_TYPE_HUD)
									{
										Storyboard.Nodes[i].used = false;
									}
								}
								int areaWidth = ImGui::GetMainViewport()->Size.x - 300;
								int nodeWidth = 180;
								int nodeHeight = 150;
								//PE: We cant force it to 13, it might overwrite another node.
								iHUDScreenNodeID = storyboard_add_missing_nodex(13, areaWidth, nodeWidth, nodeHeight, false);
								ImNodes::SetNodeGridSpacePos(Storyboard.Nodes[iHUDScreenNodeID].id, ImVec2(areaWidth * 0.5 - (nodeWidth * 0.5), STORYBOARD_YSTART + (nodeHeight + NODE_HEIGHT_PADDING) * 3));
								iCurrentSelectedWidget = -1;
								// Also ensure that any user defined globals are removed when resetting HUD screens
								for (int i = 0; i < STORYBOARD_MAXNODES; i++)
								{
									if (strnicmp(Storyboard.Nodes[i].lua_name, "hud", 3) == NULL)
									{
										for (int j = 0; j < STORYBOARD_MAXWIDGETS; j++)
										{
											if (strnicmp(Storyboard.widget_readout[i][j], "user defined", 12) == 0)
											{
												Storyboard.widget_readout[i][j][0] = 0;
											}
										}
									}
								}
								g_bRefreshGlobalList = true;
							}
						}
						ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((ImGui::GetContentRegionAvail().x * 0.5) - (buttonwide * 0.5), 0.0f));

					}
					#endif

					// generic warning of no more screens
					if (bShowNoMoreScreensError == true)
					{
						char pErrMess[256];
						sprintf(pErrMess, "Number of allocated screens/levels reached. The maximum is %d.", STORYBOARD_MAXNODES);
						strcpy(cTriggerMessage, pErrMess);
						bTriggerMessage = true;
					}

					//PE: Auto connect node.
					if (iAutoConnectNode >= 0)
					{
						for (int i = 0; i < STORYBOARD_MAXNODES; i++)
						{
							if (Storyboard.Nodes[i].used)
							{
								if (Storyboard.Nodes[i].type == STORYBOARD_TYPE_LEVEL && !pestrcasestr(Storyboard.Nodes[i].lua_name, "loading"))
								{
									if (Storyboard.Nodes[i].output_linkto[2] == 0 && i != iAutoConnectNode)
									{
										Storyboard.Nodes[i].output_linkto[2] = Storyboard.Nodes[iAutoConnectNode].input_id[0];
										break;
									}
								}
							}
						}
						iAutoConnectNode = -1;
					}

					ImGui::Indent(-10);
				}
				if (ImGui::StyleCollapsingHeader("Play Game", ImGuiTreeNodeFlags_DefaultOpen) || iStoryboardExecuteKey != 0 )
				{
					ImGui::Indent(10);
					if (strlen(playerrors) > 0) ImGui::TextWrapped(playerrors);

					static bool bStandalone = false;
					float tw = ImGui::CalcTextSize("Standalone Test All Levels").x;
					ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((ImGui::GetContentRegionAvailWidth()*0.5) - (tw*0.5) - (ImGui::GetFrameHeight()*0.5), 0.0f));
					ImGui::Checkbox("Standalone Test All Levels", &bStandalone);
					if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", "NOTE: This will exit the editor and start a full standalone version of your game.");

					cstr but1 = "Normal Single Level";
					cstr but2 = "Invulnerable Single Level";

					if (bStandalone)
					{
						but1 = "Normal Standalone";
						but2 = "Invulnerable Standalone";
					}

					ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((ImGui::GetContentRegionAvail().x*0.5) - (buttonwide*0.5), 0.0f));
					if (ImGui::StyleButton(but1.Get(), ImVec2(buttonwide, 0.0f)) || iStoryboardExecuteKey == ' ')
					{
						strcpy(playerrors, "");

						iStoryboardExecuteKey = 0;

						g.projectmodified = 0; gridedit_changemodifiedflag();
						g.projectmodifiedstatic = 0;

						//Try loading level first.
						if (strlen(Storyboard.gamename) <= 0)
						{
							BoxerInfo("You should save your game project first.", "Information");
						}
						else
						{
							FindFirstLevel(g_Storyboard_First_Level_Node, g_Storyboard_First_fpm, true);
							if (g_Storyboard_First_Level_Node == -1)
							{
								strcpy(playerrors, "You do not have any levels in your setup.");
							}
							else
							{
								bool bAbort = false;
								if (Storyboard.iChanged)
								{
									bBlockNextMouseCheck = true;
									if (!pref.iDisableProjectAutoSave && strlen(Storyboard.gamename) > 0)
									{
										save_storyboard(Storyboard.gamename, false);
									}
									else
									{
										int iAction = askBoxCancel(STORYBOARD_SAVE_MESSAGE, "Confirmation"); //1==Yes 2=Cancel 0=No
										if (iAction == 1)
										{
											//Save.
											if (strlen(Storyboard.gamename) > 0)
												save_storyboard(Storyboard.gamename, false);
											else
											{
												bAbort = true;
												save_storyboard(Storyboard.gamename, true);
											}
										}
									}
								}
								if (!bAbort)
								{
									if (bStandalone)
									{
										Sleep(100); //PE: Write done,
										extern bool g_bCascadeQuitFlag;
										g_bCascadeQuitFlag = true;
										PostQuitMessage(0);
										SetCurrentDirectoryA("..\\");
										char par[MAX_PATH];
										sprintf(par, "project=0%s", Storyboard.gamename);
										ExecuteFile("GameGuruMAX.exe", par, "", 0);
										Sleep(500);
										//ExitProcess(0); PostQuitMessage(0); above should take care of this.
									}
									else
									{
										g_Storyboard_Current_Level = g_Storyboard_First_Level_Node;
										strcpy(g_Storyboard_Current_fpm, g_Storyboard_First_fpm);

										if (g.projectfilename_s != g_Storyboard_First_fpm)
										{
											//Load level if not already loaded.
											strcpy(cDirectOpen, g_Storyboard_First_fpm);
											iLaunchAfterSync = 7; //Direct load.
											iSkibFramesBeforeLaunch = 2;
											iFramesBeforeEmulate = 10;
										}
										else
										{
											iFramesBeforeEmulate = 2;
										}
										g.projectfilename_s = g_Storyboard_First_fpm;
										bTestStandalone = true;
										strcpy(startpage, "title");
										strcpy(lastpage, "title");
										strcpy(playerrors, "");
									}
								}
							}
						}


					}
					ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((ImGui::GetContentRegionAvail().x*0.5) - (buttonwide*0.5), 0.0f));
					if (ImGui::StyleButton(but2.Get(), ImVec2(buttonwide, 0.0f)) || iStoryboardExecuteKey == '!' )
					{
						strcpy(playerrors, "");

						iStoryboardExecuteKey = 0;
						 //Try loading level first.
						if (strlen(Storyboard.gamename) <= 0)
						{
							BoxerInfo("You should save your game project first.", "Information");
						}
						else
						{
							FindFirstLevel(g_Storyboard_First_Level_Node, g_Storyboard_First_fpm, true);
							if (g_Storyboard_First_Level_Node == -1)
							{
								strcpy(playerrors, "You do not have any levels in your setup.");
							}
							else
							{
								bool bAbort = false;
								if (Storyboard.iChanged)
								{
									bBlockNextMouseCheck = true;
									if (!pref.iDisableProjectAutoSave && strlen(Storyboard.gamename) > 0)
									{
										save_storyboard(Storyboard.gamename, false);
									}
									else
									{
										int iAction = askBoxCancel(STORYBOARD_SAVE_MESSAGE, "Confirmation"); //1==Yes 2=Cancel 0=No
										if (iAction == 1)
										{
											//Save.
											if (strlen(Storyboard.gamename) > 0)
												save_storyboard(Storyboard.gamename, false);
											else
											{
												bAbort = true;
												save_storyboard(Storyboard.gamename, true);
											}
										}
									}
								}
								if (!bAbort)
								{
									if (bStandalone)
									{
										extern bool g_bCascadeQuitFlag;
										g_bCascadeQuitFlag = true;
										PostQuitMessage(0);
										SetCurrentDirectoryA("..\\");
										char par[MAX_PATH];
										sprintf(par, "project=1%s", Storyboard.gamename);
										ExecuteFile("GameGuruMAX.exe", par, "", 0);
										Sleep(500);
										ExitProcess(0);
									}
									else
									{
										g_Storyboard_Current_Level = g_Storyboard_First_Level_Node;
										strcpy(g_Storyboard_Current_fpm, g_Storyboard_First_fpm);

										if (g.projectfilename_s != g_Storyboard_First_fpm)
										{
											//Load level if not already loaded.
											strcpy(cDirectOpen, g_Storyboard_First_fpm);
											iLaunchAfterSync = 7; //Direct load.
											iSkibFramesBeforeLaunch = 2;
											iFramesBeforeEmulate = 10;
										}
										else
										{
											iFramesBeforeEmulate = 2;
										}
										g.projectfilename_s = g_Storyboard_First_fpm;
										bTestStandalone = true;
										strcpy(startpage, "title");
										strcpy(lastpage, "title");
										strcpy(playerrors, "");
										bStartInvulnerableMode = true;
									}
								}
							}
						}
					}
					ImGui::Indent(-10);
				}
				if (ImGui::StyleCollapsingHeader("Export Game", ImGuiTreeNodeFlags_DefaultOpen) || iStoryboardExecuteKey != 0)
				{
					ImGui::Indent(10);
					static char errors[256] = "\0";
					if (strlen(errors) > 0) ImGui::TextWrapped(errors);
					ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((ImGui::GetContentRegionAvail().x*0.5) - (buttonwide*0.5), 0.0f));
					if (ImGui::StyleButton("Save Standalone Game", ImVec2(buttonwide, 0.0f)) || iStoryboardExecuteKey == 'E')
					{
						strcpy(errors, "");

						//PE: Deselect any objects, if we load a level with less entityties then t.widget.pickedEntityIndex it will crash.
						t.widget.pickedEntityIndex = 0;
						t.gridentity = 0;

						iStoryboardExecuteKey = 0;
						//PE: We need to store a list of levels to generate,
						if (strlen(Storyboard.gamename) <= 0)
						{
							BoxerInfo("You should save your game project before making a standalone.", "Information");
						}
						else
						{
							CloseAllOpenTools();
							int iRet = AskSaveBeforeNewAction();
							if (iRet != 2)
							{
								g.bUseStoryBoardSetup = true;
								FindFirstLevel(g_Storyboard_First_Level_Node, g_Storyboard_First_fpm, true);
								if (g_Storyboard_First_Level_Node == -1)
								{
									strcpy(errors, "You do not have any levels in your setup.");
								}
								else
								{
									bool bAbort = false;
									if (Storyboard.iChanged)
									{
										bBlockNextMouseCheck = true;
										if (!pref.iDisableProjectAutoSave && strlen(Storyboard.gamename) > 0)
										{
											save_storyboard(Storyboard.gamename, false);
										}
										else
										{
											int iAction = askBoxCancel(STORYBOARD_SAVE_MESSAGE, "Confirmation"); //1==Yes 2=Cancel 0=No
											if (iAction == 1)
											{
												//Save.
												if (strlen(Storyboard.gamename) > 0)
													save_storyboard(Storyboard.gamename, false);
												else
												{
													bAbort = true;
													save_storyboard(Storyboard.gamename, true);
												}
											}
										}
									}
									if (!bAbort)
									{

										g_Storyboard_Current_Level = g_Storyboard_First_Level_Node;
										strcpy(g_Storyboard_Current_fpm, g_Storyboard_First_fpm);

										if (g.projectfilename_s != g_Storyboard_First_fpm)
										{
											//Load level if not already loaded.
											strcpy(cDirectOpen, g_Storyboard_First_fpm);
											iLaunchAfterSync = 7; //Direct load.
											iSkibFramesBeforeLaunch = 2;
											//PE: Delay open standalone.
											bLaunchSaveStandalonefterLoad = true;
										}
										else
										{									
											//PE: Already loaded directly to export.
											bExport_Standalone_Window = true;
										}
										g.projectfilename_s = g_Storyboard_First_fpm;

										//Use fullpath to levels.
										char destination[MAX_PATH];
										strcpy(destination, g.projectfilename_s.Get());
										GG_GetRealPath(destination, 1);
										g.projectfilename_s = destination;
									}
								}
							}
						}
					}
					ImGui::Indent(-10);
				}

				if(bModal)
					ControlAdvancedSetting(pref.iStoryboardAdvanced, "advanced storyboard features", &bStoryboardWindow);
				else
					ControlAdvancedSetting(pref.iStoryboardAdvanced, "advanced storyboard features", NULL);

				if (pref.iStoryboardAdvanced)
				{
					//Advanced options.
				}

				if (!pref.bHideTutorials)
				{
					if (ImGui::StyleCollapsingHeader("Tutorial", ImGuiTreeNodeFlags_DefaultOpen))
					{
						ImGui::Indent(10);
						char* my_combo_itemsp[] = { "0701 - Game Storyboard", "0501 - Terrain Generator" };
						int my_combo_items = 0;
						int iVideoSection = 0;
						cstr cShowTutorial = "0701 - Game Storyboard";
						iVideoSection = SECTION_STORYBOARD;//LB: SECTION_SCULPT_TERRAIN;
						SmallTutorialVideo(cShowTutorial.Get(), my_combo_itemsp, ARRAYSIZE(my_combo_itemsp), iVideoSection);
						float but_gadget_size = ImGui::GetFontSize()*12.0;
						float w = ImGui::GetWindowContentRegionWidth() - 10.0;
						ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((w*0.5) - (but_gadget_size*0.5), 0.0f));
						ImGui::Indent(-10);
					}
				}

				// insert a keyboard shortcut component into panel
				UniversalKeyboardShortcut(eKST_Storyboard);

				if (ImGui::GetCurrentWindow()->ScrollbarSizes.x > 0) {
					//Hitting exactly at the botton could cause flicker, so add some additional lines when scrollbar on.
					ImGui::Text("");
					ImGui::Text("");
				}

				ImGui::Columns(1);
			}

			if (bModal)
				ImGui::EndPopup();
			else
				ImGui::End();
		}

		if (!bModal) ImGui::PopStyleColor();

		if (!bPopModalStoryboard)
		{
			//Close down everything.
		}

		// handle renaming of HUD Screen
		if (g_iRenameHUDScreenID > 0)
		{
			ImGui::OpenPopup("Rename HUD Screen##Storyboard");
			ImGui::SetNextWindowSize(ImVec2(380, 320), ImGuiCond_Always);
			ImGui::SetNextWindowPosCenter(ImGuiCond_Always);
			bool bRenameHUDScreenWindow = true;
			if (ImGui::BeginPopupModal("Rename HUD Screen##Storyboard", &bRenameHUDScreenWindow, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings))
			{
				ImGui::Indent(10);
				ImGui::Text("");
				ImGui::SetWindowFontScale(1.4);
				ImGui::TextCenter("Rename HUD Screen");
				ImGui::SetWindowFontScale(1.0);
				ImGui::Text("");
				ImGui::SetWindowFontScale(1.4);
				ImGui::Separator();
				ImGui::SetWindowFontScale(1.0);

				ImGui::TextWrapped("To rename this HUD Screen, please give it a new unique name and click 'Rename HUD Screen'.");
				if (strlen(g_pRenameHUDScreenError) > 0)
				{
					ImGui::Text(g_pRenameHUDScreenError);
					ImGui::Text("");
				}
				ImGui::Text("Rename HUD Screen to:");
				ImGui::PushItemWidth(-10);
				ImGui::InputText("##RenameHUDScreenNameStoryboard", g_pRenameHUDName, 250, ImGuiInputTextFlags_None);
				ImGui::PopItemWidth();
				ImGui::Text("");
				ImGui::SetWindowFontScale(1.4);
				if (ImGui::StyleButton("Rename HUD Screen", ImVec2(ImGui::GetContentRegionAvail().x * 1.0 - 10.0f, 0.0f)))
				{
					if (strlen(g_pRenameHUDName) > 0)
					{
						strcpy(g_pRenameHUDScreenError, "");
						for (int i = 0; i < STORYBOARD_MAXNODES; i++)
						{
							if (Storyboard.Nodes[i].used && stricmp(Storyboard.Nodes[i].title, g_pRenameHUDName) == NULL)
							{
								// hmm, this name already existy
								strcpy(g_pRenameHUDScreenError, "This HUD Screen name already exists, choose another!");
								break;
							}
						}
						if (strcmp(g_pRenameHUDScreenError, "") == NULL)
						{
							strcpy (Storyboard.Nodes[g_iRenameHUDScreenID].title, g_pRenameHUDName);
							g_iRenameHUDScreenID = -1;
						}
					}
				}
				if (ImGui::StyleButton("Cancel", ImVec2(ImGui::GetContentRegionAvail().x * 1.0 - 10.0f, 0.0f)))
				{
					g_iRenameHUDScreenID = -1;
				}
			}
		}

		//Emulate standalone.
		bool bBlackOut = false;
		if (iFramesBeforeEmulate > 0)
		{
			iFramesBeforeEmulate--;
			//PE: Hide everything.
			if (iFramesBeforeEmulate < 4)
			{
				bBlackOut = true;
			}
		}
		else
		{
			if (bStartLoadingGame)
			{
				if (iFakeLoadGameTest-- > 0)
				{
					strcpy(lastpage, startpage);
					//strcpy(startpage, "loading");
					extern cstr g_Storyboard_LoaderScreen_Name;
					strcpy(startpage, g_Storyboard_LoaderScreen_Name.Get());
					int iret = screen_editor(-1, true, startpage);
				}
				else
				{
					bTestStandalone = false;
					bStartLoadingGame = false;
					bFakeStandaloneTest = true;
					//Start test game.
					iLaunchAfterSync = 1;
					bBlackOut = true;
				}
			}
			else if (bTestStandalone)
			{
				int iret = screen_editor(-1, true, startpage);

				if (iret == STORYBOARD_ACTIONS_GOTOLEVEL)
				{
					//PE: Need to load the level before emulating loading.
					if (g.projectfilename_s != g_Storyboard_Current_fpm)
					{
						t.returnstring_s = g_Storyboard_Current_fpm;
						if (t.returnstring_s != "")
						{
							if (cstr(Lower(Right(t.returnstring_s.Get(), 4))) == ".fpm")
							{
								GGTerrain_CancelRamp();
								t.gridentity = 0;
								t.inputsys.constructselection = 0;
								editor_refresheditmarkers();
								g.projectfilename_s = t.returnstring_s;

								extern bool g_bAllowBackwardCompatibleConversion;
								g_bAllowBackwardCompatibleConversion = true;
								gridedit_load_map();
								g_EntityClipboard.clear(); //PE: Clear any old copy/paste.
								g_bAllowBackwardCompatibleConversion = false;

								t.terrain.grassregionx1 = t.terrain.grassregionx2;

								iLastUpdateVeg = 0;
								bUpdateVeg = true;
								extern int g_iSuperTriggerFullGrassReveal; // hmm, shoved in to get the damn grass showing on initial load!
								g_iSuperTriggerFullGrassReveal = 10;
								iLaunchAfterSync = 80; //Update env
								iSkibFramesBeforeLaunch = 5;
							}
						}
					}
					bStartLoadingGame = true;
					iFakeLoadGameTest = 100;
					extern bool g_Storyboard_Starting_New_Level;
					g_Storyboard_Starting_New_Level = true; //PE: Always start fresh when linking directly to a level.

				}
				if (iret == STORYBOARD_ACTIONS_CONTINUE)
				{
					//Restart
					strcpy(startpage, "title");
					strcpy(lastpage, "title");
					strcpy(playerrors, "");
				}
				if (iret == STORYBOARD_ACTIONS_BACK)
				{
					strcpy(startpage, lastpage);
				}
				if (iret == STORYBOARD_ACTIONS_EXITGAME)
				{
					bTestStandalone = false;
				}
				if (iret == STORYBOARD_ACTIONS_STARTGAME)
				{
					bStartLoadingGame = true;
					iFakeLoadGameTest = 100;
					extern bool g_Storyboard_Starting_New_Level;
					g_Storyboard_Starting_New_Level = true; //PE: Start a fresh game.
				}
				if (iret == STORYBOARD_ACTIONS_GOTOSCREEN)
				{
					if (strlen(t.game.pSwitchToPage) == 0)
					{
						strcpy(lastpage, startpage);
						extern cstr g_Storyboard_LoaderScreen_Name;
						strcpy(startpage, g_Storyboard_LoaderScreen_Name.Get());
					}
					else
					{
						strcpy(lastpage, startpage);
						strcpy(startpage, t.game.pSwitchToPage);
					}
				}
			}
		}
		if (bBlackOut)
		{
			ImGuiViewport* mainviewport = ImGui::GetMainViewport();
			if (mainviewport)
			{
				ImDrawList* drawlist = ImGui::GetForegroundDrawList(mainviewport);
				if (drawlist)
				{
					ImVec4 monitor_col = ImVec4(0.0, 0.0, 0.0, 1.0 - t.postprocessings.fadeinvalue_f); //Fade in.
					drawlist->AddRectFilled(ImVec2(-1, -1), ImGui::GetMainViewport()->Size + ImVec2(40.0f, 40.0f), ImGui::GetColorU32(monitor_col));
				}
			}
		}

	}
	else
	{
		pref.iLastInStoryboard = false;
	}
}


