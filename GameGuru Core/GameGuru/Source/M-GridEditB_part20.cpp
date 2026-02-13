extern bool bTriggerTerrainSaveAsWindow;
int iTerrainSaveAsProcess = 0;

int save_level_as( void )
{
	int iRet = 0;
	static bool bGotAThumb = false;
	static int iSetKeyboardFocusHere = 10;

	if (bTriggerTerrainSaveAsWindow)
	{
		static char NewLevelName[256] = "\0";
		static char NewLevelError[256] = "\0";

		if (iTerrainSaveAsProcess == 0)
		{
			bGotAThumb = false;
			//Grab any thumbs created.
			if (ImageExist(STORYBOARD_THUMBS + 402)) DeleteImage(STORYBOARD_THUMBS + 402);

			GG_SetWritablesToRoot(true);
			if (FileExist("thumbbank\\lastnewlevel.jpg"))
			{
				image_setlegacyimageloading(true);
				LoadImageSize("thumbbank\\lastnewlevel.jpg", STORYBOARD_THUMBS + 402, 512, 288);
				image_setlegacyimageloading(false);
				if (ImageExist(STORYBOARD_THUMBS + 402))
					bGotAThumb = true;
			}
			GG_SetWritablesToRoot(false);
			strcpy(NewLevelName, "");
			strcpy(NewLevelError, "");
			iSetKeyboardFocusHere = 10;
			iTerrainSaveAsProcess++;
		}
		
		if (iTerrainSaveAsProcess == 1)
		{
			static int popwinheight = 0;
			if (popwinheight > 800 || iSkibFramesBeforeLaunch > 0)
			{
				ImGui::SetNextWindowSize(ImVec2(0, 532), ImGuiCond_Always);
			}

			//Ask to save new level.
			ImGui::OpenPopup("Save New Level##Storyboard");

			ImGui::SetNextWindowPosCenter(ImGuiCond_Always);
			bool bSaveNewLevelWindow = true;

			ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings;
			if (bDigAHoleToHWND) window_flags |= ImGuiWindowFlags_ForceRender;

			if (ImGui::BeginPopupModal("Save New Level##Storyboard", &bSaveNewLevelWindow, window_flags))
			{
				ImGuiWindow* bwindow = ImGui::GetCurrentWindow(); // ImGui::FindWindowByName("Save New Level##Storyboard");
				if (bDigAHoleToHWND)
				{
					if (bwindow)
					{
						bwindow->DrawList->AddCallback((ImDrawCallback)10, NULL); //force render.
						ImVec4 style_winback = ImGui::GetStyle().Colors[ImGuiCol_WindowBg];
						style_winback.w = 1.0f;
						ImGui::GetCurrentWindow()->DrawList->AddRectFilled(ImGui::GetWindowPos(), ImGui::GetWindowPos() + ImGui::GetWindowSize(), ImGui::GetColorU32(style_winback), 0.0f, ImDrawCornerFlags_None);
					}
				}
				popwinheight = ImGui::GetWindowSize().y;
				ImGui::Indent(10);
				ImGui::Text("");
				ImGui::SetWindowFontScale(1.4);
				ImGui::TextCenter("Save New Level As");
				ImGui::Separator();
				if (bGotAThumb && ImageExist(STORYBOARD_THUMBS + 402))
				{
					ImGui::ImgBtn(STORYBOARD_THUMBS + 402, ImVec2(512, 288), ImColor(0, 0, 0, 0), ImColor(255, 255, 255, 255), ImColor(255, 255, 255, 255), ImColor(255, 255, 255, 255), 0, 0, 0, 0, false, false, false);
					ImGui::Separator();
				}
				else
				{
					ImGui::Dummy(ImVec2(512, 10));
				}
				ImGui::SetWindowFontScale(1.0);
				ImGui::TextWrapped("You have just created a new level, to save this level please give it a name and click 'save'");
				ImGui::Text("");
				if (strlen(NewLevelError) > 0)
				{
					ImGui::Text(NewLevelError);
					ImGui::Text("");
				}
				ImGui::Text("Level Name");
				ImGui::PushItemWidth(-10);
				if (iSetKeyboardFocusHere > 0)
				{
					iSetKeyboardFocusHere--;
					ImGui::SetKeyboardFocusHere();
				}
				bool bTriggerReturnSave = false;
				if (ImGui::InputText("##NewLevelNameStoryboard", NewLevelName, 250, ImGuiInputTextFlags_EnterReturnsTrue)) //ImGuiInputTextFlags_None ImGuiInputTextFlags_ReadOnly
				{
					bTriggerReturnSave = true;
				}
				ImGui::PopItemWidth();

				ImGui::Text("");

				ImGui::SetWindowFontScale(1.4);
				if (ImGui::StyleButton("Save Level", ImVec2(ImGui::GetContentRegionAvail().x*0.5 - 20.0f, 0.0f)) || bTriggerReturnSave )
				{
					if (strlen(NewLevelName) > 0)
					{
						char tmp[MAX_PATH];
						strcpy(tmp, g.mysystem.mapbankAbs_s.Get());
						//Relative.
						char *find = (char *)pestrcasestr(tmp, "mapbank\\");
						if (find && find != &tmp[0]) strcpy(&tmp[0], find);
						strcat(tmp, NewLevelName);

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

							iLaunchAfterSync = 503; //Do the actualy save here.
							iSkibFramesBeforeLaunch = 3;
							iLaunchAfterSyncAction = 11;
							strcpy(cTriggerMessage, "Saving Level ...");
							bTriggerMessage = true;

							if (iNewLevelNode >= 0)
							{
								strcpy(Storyboard.Nodes[iNewLevelNode].title, NewLevelName);
								strcpy(Storyboard.Nodes[iNewLevelNode].level_name, g.projectfilename_s.Get());
								if (bGotAThumb)
								{
									CreateBackBufferCacheNameEx(Storyboard.Nodes[iNewLevelNode].level_name, 512, 288, true);
									SaveImage(BackBufferCacheName.Get(), STORYBOARD_THUMBS + 402);
									if (FileExist(BackBufferCacheName.Get()))
									{
										if (CopyToProjectFolder(BackBufferCacheName.Get()))
										{
											//PE: Use relative projectbank filename.
											if (FileExist(ProjectCacheName.Get()))
												BackBufferCacheName = ProjectCacheName;
										}

										//Load to correct id.
										SetMipmapNum(1);
										image_setlegacyimageloading(true);
										LoadImageSize(BackBufferCacheName.Get(), Storyboard.Nodes[iNewLevelNode].thumb_id, 512, 288);
										image_setlegacyimageloading(false);
										SetMipmapNum(-1);
										strcpy(Storyboard.Nodes[iNewLevelNode].thumb, BackBufferCacheName.Get());
									}
								}
							}

							bTriggerTerrainSaveAsWindow = false; //Close down window.
							iRet = 2;
						}
						else
						{
							//Cancel just ignore.
						}
					}
					else
					{
						strcpy(NewLevelError, "Error: Please give your level a name before save.");
					}
				}
				ImGui::SameLine();
				if (ImGui::StyleButton("Cancel", ImVec2(ImGui::GetContentRegionAvail().x - 10.0f, 0.0f)))
				{
					//Cancel.
					bTriggerTerrainSaveAsWindow = false; //Close down window.
					iRet = 1;
				}

				ImGui::SetWindowFontScale(1.0);
				ImGui::Text("");

				bImGuiGotFocus = true;
				ImGui::Indent(-10);
				ImGui::EndPopup();

				if (bDigAHoleToHWND && bwindow)
				{
					bwindow->DrawList->AddCallback((ImDrawCallback)11, NULL); //disable force render.
				}
			}
		}
	}
	else
	{
		iTerrainSaveAsProcess = 0;
	}
	return(iRet);
}

void switch_to_remote_project(LPSTR ProjectAsName)
{
	extern char szWriteDir[MAX_PATH];
	extern char szBeforeChangeWriteDir[MAX_PATH];
	extern char szAddWriteDirAdditional[MAX_PATH];

	strcpy(szBeforeChangeWriteDir, szWriteDir);

	// store writables folder
	char pStoreWriteable[MAX_PATH];
	strcpy(pStoreWriteable, pref.cCustomWriteFolder);

	// before move to new project folder, create projectbank marker so can load this remove located project
	char pRemoteProject[MAX_PATH];
	strcpy(pRemoteProject, "projectbank\\");
	strcat(pRemoteProject, ProjectAsName);
	strcat(pRemoteProject, "\\remoteproject.txt");
	GG_GetRealPath(pRemoteProject, 1);

	// create new project folder area
	strcpy(pref.cCustomWriteFolder, Storyboard.customprojectfolder);
	if (pref.cCustomWriteFolder[strlen(pref.cCustomWriteFolder) - 1] != '\\') strcat(pref.cCustomWriteFolder, "\\");
	strcat(pref.cCustomWriteFolder, ProjectAsName);
	if (pref.cCustomWriteFolder[strlen(pref.cCustomWriteFolder) - 1] != '\\') strcat(pref.cCustomWriteFolder, "\\");
	SetUpdaterWritePathFile(pref.cCustomWriteFolder);
	FileRedirectChangeWritableArea("");

	//PE: Fix szAddWriteDirAdditional
	if (!pestrcasestr(szAddWriteDirAdditional, "GameGuruApps"))
	{
		strcat(szAddWriteDirAdditional, "\\GameGuruApps\\GameGuruMAX\\");
	}

	//PE: We are using szWriteDir for remote project path, so set szAddWriteDirAdditional to normal document folder.
	if (strlen(pStoreWriteable) > 0)
	{
		// override writables with known custom writables folder
		strcpy_s(szAddWriteDirAdditional, MAX_PATH, pStoreWriteable);
	}

	// create remote project marker
	OpenToWrite(1, pRemoteProject);
	WriteString(1, Storyboard.customprojectfolder);
	CloseFile(1);

	// restore writables folder
	strcpy(pref.cCustomWriteFolder, pStoreWriteable);
}

void switch_to_regular_projects(void)
{
	extern char szBeforeChangeWriteDir[MAX_PATH];
	extern char szWriteDir[MAX_PATH];

	//PE: Check if we change from a remote project to none.
	if (strlen(szBeforeChangeWriteDir) > 0)
	{
		//PE: Regular project update the library.
		extern int g_iRefreshLibraryFoldersAfterDelay;
		g_iRefreshLibraryFoldersAfterDelay = 10;
	}

	strcpy(szBeforeChangeWriteDir, "");

	// generate app folder using exe name
	HMODULE hModule = GetModuleHandle(NULL);
	char szModule[MAX_PATH] = "";
	char szDrive[10] = "";
	char szDir[MAX_PATH] = "";
	char szEXE[MAX_PATH] = "";
	GetModuleFileNameA(hModule, szModule, MAX_PATH);
	_splitpath_s(szModule, szDrive, 10, szDir, MAX_PATH, szEXE, MAX_PATH, NULL, 0);
	FileRedirectRestoreWritableArea(szEXE);
	FileRedirectChangeWritableArea(szEXE);
}

int save_create_storyboard_project(void)
{
	int iRet = 0;
	static int iSetKeyboardFocusHere = 10;
	if (bTriggerSaveAs)
	{
		//Ask to save as.

		ImGui::OpenPopup("Save As#Storyboard");
		ImGui::SetNextWindowSize(ImVec2(0, 532), ImGuiCond_Once);
		static int popwinheight = 0;
		if (popwinheight > 800 || iSkibFramesBeforeLaunch > 0)
		{
			ImGui::SetNextWindowSize(ImVec2(0, 532), ImGuiCond_Always);
		}
		ImGui::SetNextWindowPosCenter(ImGuiCond_Always);
		bool bSaveAsWindow = true;
		if (ImGui::BeginPopupModal("Save As#Storyboard", &bSaveAsWindow, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings))
		{
			popwinheight = ImGui::GetWindowSize().y;

			if (bTriggerSaveAsAfterNewLevel)
			{
				ImVec2 vCurPos = ImGui::GetCursorPos();
				ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(3.0f, 8.0f));
				ImGui::SetItemAllowOverlap();
				ImVec2 vIconSize = { (float)ImGui::GetFontSize()*3.5f, (float)ImGui::GetFontSize()*3.5f };
				if (ImGui::ImgBtn(TOOL_GOBACK, vIconSize, ImVec4(0, 0, 0, 0), drawCol_normal, drawCol_hover, drawCol_Down, 0, 0, 0, 0, false, false, false, false, false, bBoostIconColors))
				{
					bTriggerSaveAs = false;
					bTriggerSaveAsAfterNewLevel = false;
					iRet = 2;

				}
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", "Close");
				ImGui::SetCursorPos(vCurPos);
			}

			ImGui::Indent(10);
			ImGui::Text("");

			ImGui::SetWindowFontScale(1.6);
			if (bTriggerSaveAsAfterNewLevel)
			{
				ImGui::TextCenter("Create a New Game Project");
				ImGui::SetWindowFontScale(1.0);
				ImGui::Text("");
			}
			else
			{
				ImGui::TextCenter("Save Game Project As");
			}
			ImGui::Separator();

			if (bTriggerSaveAsAfterNewLevel)
				ImGui::Dummy(ImVec2(580, 1));
			else
				ImGui::Dummy(ImVec2(510, 1));
			ImGui::SetWindowFontScale(1.2);
			ImGui::Text("");
			if (bTriggerSaveAsAfterNewLevel)
			{
				ImGui::TextCenter("All new games need a name. This is so they can be saved to the");
				ImGui::TextCenter("hard drive and identified when edited in the future.");
				ImGui::Text("");
				ImGui::TextCenter("If you have a name in mind that's great! Just enter it below.");
				ImGui::Text("");
				ImGui::TextCenter("Make sure you choose a good name for your project, you will");
				ImGui::TextCenter("not be able to change it later on.");
			}
			else
			{
				ImGui::Text("To save your game project, please give your project a name and click 'save'");
				ImGui::Text("");
				ImGui::TextWrapped("NOTE: This name will also be used as your standalone game name.");
			}
			ImGui::Text("");


			if (strlen(SaveProjectAsError) > 0)
			{
				ImGui::Text(SaveProjectAsError);
				ImGui::Text("");
			}
			if (bTriggerSaveAsAfterNewLevel)
			{
				ImGui::SetWindowFontScale(1.4);
				ImGui::TextCenter("Enter Your Game Name Here");
				ImGui::SetWindowFontScale(1.2);
			}
			else
			{
				ImGui::Text("Game Project Name");
			}
			ImGui::PushItemWidth(-10);

			bool bClicked = false;
			bool bForceClicked = false;

			if (iSetKeyboardFocusHere > 0)
			{
				iSetKeyboardFocusHere--;
				ImGui::SetKeyboardFocusHere();
			}
			if (ImGui::InputText("##SaveAsNameStoryboard", SaveProjectAsName, 250, ImGuiInputTextFlags_None | ImGuiInputTextFlags_EnterReturnsTrue))//ImGuiInputTextFlags_None ImGuiInputTextFlags_ReadOnly
			{
				//Clean name.
				std::string sCleanName = SaveProjectAsName;
				replaceAll(sCleanName, """", "");
				replaceAll(sCleanName, "\\", "");
				replaceAll(sCleanName, "/", "");
				replaceAll(sCleanName, "^", "");
				replaceAll(sCleanName, "?", "");
				replaceAll(sCleanName, "@", "");
				replaceAll(sCleanName, "\t", "");
				replaceAll(sCleanName, ":", "");
				strcpy(SaveProjectAsName, sCleanName.c_str());
				if(bTriggerSaveAsAfterNewLevel) bForceClicked = true; // Enter now just create the game.
			}
			ImGui::PopItemWidth();

			ImGui::Text("");
			ImGui::SetWindowFontScale(1.4);

			// New project Systemn Setting
			ImGui::TextCenter("Optional Project Folder");
			ImGui::SetWindowFontScale(1.2);
			ImGui::TextCenter("By default all projects are stored in the projectbank area.");
			ImGui::TextCenter("You have the option to create your game project folder at any location");
			ImGui::TextCenter("and ensure that all media used in your game is copied to this separate");
			ImGui::TextCenter("location, allowing your project to keep all necessary files in one place");
			float path_gadget_size = ImGui::GetFontSize() * 2.0;
			ImGui::PushItemWidth(-10 - path_gadget_size);
			static bool bSeparateProjectFolder = false;
			if (strlen(Storyboard.customprojectfolder) == 0) bSeparateProjectFolder = false;
			if (strlen(Storyboard.customprojectfolder) > 0) bSeparateProjectFolder = true;
			if (ImGui::Checkbox("Separate project folder", &bSeparateProjectFolder))
			{
				if(bSeparateProjectFolder==false)
				{
					strcpy(Storyboard.customprojectfolder, "");
				}
				else
				{
					if (strlen(Storyboard.customprojectfolder) == 0)
					{
						strcpy(Storyboard.customprojectfolder, "C:\\Dropbox\\");
					}
				}
			}
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Choose whether the game project should be a completely separate project area");
			if (bSeparateProjectFolder)
			{
				ImGui::InputText("##InputCustomNewProjectFolder", &Storyboard.customprojectfolder[0], 250, ImGuiInputTextFlags_ReadOnly);
				if (!pref.iTurnOffEditboxTooltip && ImGui::IsItemHovered()) ImGui::SetTooltip("%s", "Location of separate project folder");
				if (ImGui::MaxIsItemFocused()) bImGuiGotFocus = true;
				ImGui::SameLine();
				ImGui::PushItemWidth(path_gadget_size);
				if (ImGui::StyleButton("...##pathCustomNewProjectFolder"))
				{
					cStr tOldDir = GetDir();
					char* cFileSelected;
					cstr fulldir = pref.cCustomWriteFolder;
					cFileSelected = (char*)noc_file_dialog_open(NOC_FILE_DIALOG_DIR, "All\0*.*\0", fulldir.Get(), NULL);
					SetDir(tOldDir.Get());
					if (cFileSelected && strlen(cFileSelected) > 0)
					{
						strcpy(Storyboard.customprojectfolder, cFileSelected);
						if (Storyboard.customprojectfolder[strlen(Storyboard.customprojectfolder) - 1] != '\\') strcat(Storyboard.customprojectfolder, "\\");
					}
				}
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("Specify location for separate project folder");
				ImGui::PopItemWidth();
			}
			ImGui::PopItemWidth();
			ImGui::Text("");
#
			// Create Project Button
			if (bTriggerSaveAsAfterNewLevel)
			{
				bClicked = ImGui::StyleButton("Create Game Project", ImVec2(ImGui::GetContentRegionAvail().x - 10.0f, 0.0f));
			}
			else
				bClicked = ImGui::StyleButton("Save As", ImVec2(ImGui::GetContentRegionAvail().x*0.5 - 20.0f, 0.0f));

			if (bClicked || bForceClicked)
			{
				// and in case this was a remote project, restore to writables regular
				extern void switch_to_regular_projects(void);
				switch_to_regular_projects();

				// if custom project folder, new wriables
				if (strlen(Storyboard.customprojectfolder) > 0)
				{
					GG_SetWritablesToRoot(true);
					switch_to_remote_project(SaveProjectAsName);
					GG_SetWritablesToRoot(false);
				}

				//PE: Trim
				std::string sCleanName = SaveProjectAsName;
				sCleanName.erase(sCleanName.find_last_not_of(" \t") + 1); //PE: No spaces tab at end.
				strcpy(SaveProjectAsName, sCleanName.c_str());

				if (strlen(SaveProjectAsName) > 0)
				{
					char destination[MAX_PATH];
					strcpy(destination, "projectbank\\");
					strcat(destination, SaveProjectAsName);
					GG_GetRealPath(destination, 1);
					int bOkSave = true;
					if (PathExist(destination) == 1)
					{
						int iAction = askBoxCancel("Project already exists, do you want to overwrite ?", "Confirmation"); //1==Yes 2=Cancel 0=No
						if (iAction == 1)
						{
							//backup old folder.
							int i_loop = 1;
							char backup[MAX_PATH];
							sprintf(backup, "%s_backup_%d", destination, i_loop++);
							while (MoveFileA(destination, backup) == 0)
								sprintf(backup, "%s_backup_%d", destination, i_loop++);
						}
						else
						{
							//Cancel.
							bOkSave = false;
						}
					}
					if (bOkSave)
					{
						// Create the new project folder, and place project.dat inside it
						MakeDirectory(destination);
						strcat(destination, "\\project.dat");
						strcpy(Storyboard.gamename, SaveProjectAsName);
						Storyboard.project_readonly = 0;
						save_storyboard(Storyboard.gamename, false);
						bTriggerSaveAs = false;
						bTriggerSaveAsAfterNewLevel = false;
						iRet = 1;

						// to allow completely isolated files for per-project customization
						// copy files that ALL standalone games will load by default
						if (strlen(Storyboard.customprojectfolder) > 0)
						{
							extern bool g_bMakingAStandaloneUsingFileCollectionArray;
							if (g_bMakingAStandaloneUsingFileCollectionArray == false)
							{
								// clear file collection
								g.filecollectionmax = 0;
								Undim (t.filecollection_s);
								Dim (t.filecollection_s, 500);

								// all the files/folders that are needed by the standalone regardless
								addfoldertocollection ("audiobank\\materials");
								addfoldertocollection ("audiobank\\materials\\dirt");
								addfoldertocollection ("audiobank\\materials\\grass");
								addfoldertocollection ("audiobank\\materials\\gravel");
								addfoldertocollection ("audiobank\\materials\\metal");
								addfoldertocollection ("audiobank\\materials\\puddle");
								addfoldertocollection ("audiobank\\materials\\sand");
								addfoldertocollection ("audiobank\\materials\\snow");
								addfoldertocollection ("audiobank\\materials\\underwater");
								addfoldertocollection ("audiobank\\materials\\wood");
								addtocollection ("audiobank\\misc\\ammo.wav");
								addtocollection ("audiobank\\misc\\explode.wav");
								addtocollection ("audiobank\\misc\\silence.wav");
								addtocollection ("audiobank\\misc\\bullet_flyby_01.wav");
								addtocollection ("audiobank\\misc\\bullet_flyby_02.wav");
								addtocollection ("audiobank\\misc\\bullet_flyby_03.wav");
								addtocollection ("audiobank\\misc\\bullet_flyby_04.wav");
								addtocollection ("audiobank\\misc\\melee.wav");
								addtocollection ("audiobank\\misc\\melee1.wav");
								addtocollection ("audiobank\\misc\\melee2.wav");
								addtocollection ("audiobank\\misc\\melee3.wav");
								addtocollection ("audiobank\\misc\\melee4.wav");
								addtocollection ("audiobank\\misc\\melee5.wav");
								addtocollection ("audiobank\\misc\\melee6.wav");
								addfoldertocollection ("charactercreatorplus\\skins");
								addfoldertocollection ("databank");
								addtocollection ("editors\\keymap\\default.ini");
								addfoldertocollection ("editors\\lut");
								addfoldertocollection ("editors\\templates\\backdrops");
								addfoldertocollection ("editors\\templates\\buttons");
								addfoldertocollection ("editors\\templates\\fonts");
								addfoldertocollection ("editors\\templates\\screeneditor");
								addtocollection ("editors\\uiv3\\loadingsplash.jpg");
								addtocollection ("editors\\uiv3\\roboto-medium.ttf");
								addfoldertocollection ("effectbank\\common");
								addfoldertocollection ("effectbank\\explosion");
								addfoldertocollection ("effectbank\\particles");
								addfoldertocollection ("effectbank\\particles\\weather");
								addfoldertocollection ("languagebank\\english\\artwork\\watermark");
								addfoldertocollection ("languagebank\\english\\textfiles");
								addfoldertocollection ("languagebank\\neutral\\gamecore\\huds\\ammohealth");
								//addfoldertocollection ("languagebank\\neutral\\gamecore\\huds\\sliders");
								addfoldertocollection ("languagebank\\neutral\\gamecore\\huds\\panels"); // used in LUA command
								addfoldertocollection ("fontbank");
								addfoldertocollection ("gamecore\\bulletholes");
								addfoldertocollection ("gamecore\\decals\\splat");
								addfoldertocollection ("gamecore\\decals\\bloodsplat");
								addfoldertocollection ("gamecore\\decals\\dust");
								addfoldertocollection ("gamecore\\decals\\impact");
								addfoldertocollection ("gamecore\\decals\\gunsmoke");
								addfoldertocollection ("gamecore\\decals\\smoke1");
								addfoldertocollection ("gamecore\\decals\\sparks");
								addfoldertocollection ("gamecore\\decals\\muzzleflash4");
								addfoldertocollection ("gamecore\\decals\\splash_droplets");
								addfoldertocollection ("gamecore\\decals\\splash_foam");
								addfoldertocollection ("gamecore\\decals\\splash_large");
								addfoldertocollection ("gamecore\\decals\\splash_misty");
								addfoldertocollection ("gamecore\\decals\\splash_ripple");
								addfoldertocollection ("gamecore\\decals\\splash_small");
								addfoldertocollection ("gamecore\\decals\\splinters");
								addfoldertocollection ("gamecore\\vrcontroller");
								addfoldertocollection ("gamecore\\vrcontroller\\oculus");
								addfoldertocollection ("gamecore\\muzzleflash");
								addfoldertocollection ("gamecore\\projectiletypes");
								addfoldertocollection ("gamecore\\projectiletypes\\common\\explode");
								addfoldertocollection ("gamecore\\projectiletypes\\enhanced\\m67");
								addfoldertocollection ("gamecore\\vrcontroller");
								addfoldertocollection ("grassbank");
								addfoldertocollection ("imagebank\\hud");
								addfoldertocollection ("imagebank\\hud library\\max");
								addfoldertocollection ("lensflares");
								addallinfoldertocollection("particlesbank", "particlesbank"); // all particles so do not miss any for standalone (only 4MB for defaults)
								addfoldertocollection ("terraintextures");
								addfoldertocollection ("terraintextures\\mat1");
								addfoldertocollection ("terraintextures\\mat2");
								addfoldertocollection ("terraintextures\\mat3");
								addfoldertocollection ("terraintextures\\mat4");
								addfoldertocollection ("terraintextures\\mat5");
								addfoldertocollection ("terraintextures\\mat6");
								addfoldertocollection ("terraintextures\\mat7");
								addfoldertocollection ("terraintextures\\mat8");
								addfoldertocollection ("terraintextures\\mat9");
								addfoldertocollection ("terraintextures\\mat10");
								addfoldertocollection ("terraintextures\\mat11");
								addfoldertocollection ("terraintextures\\mat12");
								addfoldertocollection ("terraintextures\\mat13");
								addfoldertocollection ("terraintextures\\mat14");
								addfoldertocollection ("terraintextures\\mat15");
								addfoldertocollection ("terraintextures\\mat16");
								addfoldertocollection ("terraintextures\\mat17");
								addfoldertocollection ("terraintextures\\mat18");
								addfoldertocollection ("terraintextures\\mat19");
								addfoldertocollection ("terraintextures\\mat20");
								addfoldertocollection ("terraintextures\\mat21");
								addfoldertocollection ("terraintextures\\mat22");
								addfoldertocollection ("terraintextures\\mat23");
								addfoldertocollection ("terraintextures\\mat24");
								addfoldertocollection ("terraintextures\\mat25");
								addfoldertocollection ("terraintextures\\mat26");
								addfoldertocollection ("terraintextures\\mat27");
								addfoldertocollection ("terraintextures\\mat28");
								addfoldertocollection ("terraintextures\\mat29");
								addfoldertocollection ("terraintextures\\mat30");
								addfoldertocollection ("terraintextures\\mat31");
								addfoldertocollection ("terraintextures\\mat32");
								addtocollection ("titlesbank\\default\\cursor.png");
								addtocollection ("titlesbank\\default\\cursor-33.png");
								addtocollection ("titlesbank\\default\\cursor-big.png");
								addfoldertocollection ("treebank");
								addfoldertocollection ("treebank\\billboards");
								addfoldertocollection ("treebank\\textures");

								// finally copy all indicated files to remote project area for initial file set
								extern void mapfile_copyallfilecollectiontopreferredprojectfolder (void);
								mapfile_copyallfilecollectiontopreferredprojectfolder();
							}
						}
					}
				}
				else
				{
					strcpy(SaveProjectAsError, "Error: Please give your game project a name before saving.");
				}
			}
			if (!bTriggerSaveAsAfterNewLevel)
			{
				ImGui::SameLine();
				if (ImGui::StyleButton("Cancel", ImVec2(ImGui::GetContentRegionAvail().x - 10.0f, 0.0f)))
				{
					//Cancel.
					bTriggerSaveAs = false;
					bTriggerSaveAsAfterNewLevel = false;
					iRet = 2;
				}
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
		iSetKeyboardFocusHere = 10;
	}
	return iRet;
}

int FindOutputScreenNode(int iNode, int index)
{
	//Storyboard.Nodes[iNextNode].lua_name
	if (iNode >= 0)
	{
		int iOutPutLinkTo = index;
		if (Storyboard.Nodes[iNode].widget_type[index] == STORYBOARD_WIDGET_BUTTON)
			iOutPutLinkTo = get_output_linkindex(iNode, index);

		//Find connected to:
		int iLinkTo = Storyboard.Nodes[iNode].output_linkto[iOutPutLinkTo];
			//Find input.
		for (int i = 0; i < STORYBOARD_MAXNODES; i++)
		{
			if (Storyboard.Nodes[i].used)
			{
				for (int l = 0; l < STORYBOARD_MAXOUTPUTS; l++)
				{
					if (iLinkTo > 0 && iLinkTo == Storyboard.Nodes[i].input_id[l])
					{
						return(i);
					}
				}
			}
		}
	}
	return(-1);
}

int FindNextLevel(int &iNextLevelNode, char *level_name, int action)
{
	// action: 0-No more levels found, go to won.lua  1-Goto a new level.  2-/Goto a new lua screen.

	int iNextNode = -1;
	int iNextWinScreenNode = -1;
	int iNextLostScreenNode = -1;

	if (iNextLevelNode >= 0)
	{
		//Find connected to:
		int iLinkToWin = Storyboard.Nodes[iNextLevelNode].output_linkto[0]; //2=Next Won screen.
		int iLinkToLost = Storyboard.Nodes[iNextLevelNode].output_linkto[1]; //2=Next Lost screen.
		int iLinkTo = Storyboard.Nodes[iNextLevelNode].output_linkto[2]; //2=Next level.

		//PE: If Storyboard.Nodes[iNextLevelNode] is a loading screen , take output from there.
		for (int i = 0; i < STORYBOARD_MAXNODES; i++)
		{
			if (Storyboard.Nodes[i].used)
			{
				for (int l = 0; l < STORYBOARD_MAXOUTPUTS; l++)
				{
					if (iLinkTo > 0 && iLinkTo == Storyboard.Nodes[i].input_id[l])
					{
						if (pestrcasestr(Storyboard.Nodes[i].lua_name, "loading"))
						{
							iLinkTo = Storyboard.Nodes[i].output_linkto[0];
						}
					}
				}
			}
		}

		//Find input.
		for (int i = 0; i < STORYBOARD_MAXNODES; i++)
		{
			if (Storyboard.Nodes[i].used)
			{
				for (int l = 0; l < STORYBOARD_MAXOUTPUTS; l++)
				{
					if (iLinkTo > 0 && iLinkTo == Storyboard.Nodes[i].input_id[l])
					{
						iNextNode = i;
					}
					if (iLinkToWin > 0 && iLinkToWin == Storyboard.Nodes[i].input_id[l])
					{
						iNextWinScreenNode = i;
					}
					if (iLinkToLost > 0 && iLinkToLost == Storyboard.Nodes[i].input_id[l])
					{
						iNextLostScreenNode = i;
					}
				}
			}
		}
	}
	if (action == 1)
	{
		//Get lost screen.
		if (iNextLostScreenNode > 0)
		{
			//Found it.
			iNextLevelNode = iNextLostScreenNode;
			strcpy(level_name, Storyboard.Nodes[iNextLostScreenNode].lua_name);
			return(2); //Goto next lua script.
		}
		//Not connected for lose.lua
		strcpy(level_name, "lose.lua");
		return(2); //Goto next lua script.
	}

	if (iNextNode > 0)
	{
		//Found next level.
		if (strlen(Storyboard.Nodes[iNextNode].level_name) > 0)
		{
			iNextLevelNode = iNextNode;
			strcpy(level_name, Storyboard.Nodes[iNextNode].level_name);
			return(1); //Goto next level.
		}
	}

	//Not linking to a level, check if we have a linking won screen.
	if (iNextWinScreenNode > 0)
	{
		if (strlen(Storyboard.Nodes[iNextWinScreenNode].lua_name) > 0)
		{
			iNextLevelNode = iNextWinScreenNode;
			strcpy(level_name, Storyboard.Nodes[iNextWinScreenNode].lua_name);
			if (stricmp(level_name, "win.lua") == NULL)
			{
				return(3); //Goto final game won lua script.
			}
			else
			{
				return(2); //Goto next won lua script.
			}
		}
	}

	//Not linking to anything, force a win.lua
	strcpy(level_name, "win.lua");
	return(2); //Goto next lua script.
}

void FindFirstSplash(char *splash_name)
{
	for (int i = 0; i < STORYBOARD_MAXNODES; i++)
	{
		if (Storyboard.Nodes[i].used)
		{
			if (Storyboard.Nodes[i].type == STORYBOARD_TYPE_SPLASH)
			{
				if (strlen(Storyboard.Nodes[i].thumb) > 0)
				{
					// replace stock splash with custom one specified by storybaord game project
					strcpy(splash_name, Storyboard.Nodes[i].thumb);
				}
			}
		}
	}
}

int FindLuaScreenNode(char *name)
{
	if (g.tabmodehidehuds > 0 && strnicmp(name, "hud", 3) == 0) //stricmp(name, "hud.lua") == 0)
	{
		// Do not render in-game HUD when HideHuds() has been called
		return -1;
	}
	if (strlen(Storyboard.gamename) <= 0) return(-1);

	// node id or name
	if (strncmp (name, ":node:", 6) == NULL)
	{
		//Find by node id passed in via string
		int iNode = atoi(&name[6]);
		if (iNode >= 0 && iNode < STORYBOARD_MAXNODES)
		{
			if (Storyboard.Nodes[iNode].used)
			{
				return iNode;
			}
		}
	}
	else
	{
		// regular name search (prone to finding a duplicate if old corrupt project nodes)
		std::string lua_name = name;
		replaceAll(lua_name, ".lua", "");
		for (int i = 0; i < STORYBOARD_MAXNODES; i++)
		{
			if (Storyboard.Nodes[i].used)
			{
				std::string check_lua_name = Storyboard.Nodes[i].lua_name;
				replaceAll(check_lua_name, ".lua", "");
				if (stricmp(check_lua_name.c_str(), lua_name.c_str()) == NULL)
				{
					return i;
				}
			}
		}
	}
	return -1;
}

int FindLuaScreenTitleNode(char* name)
{
	if (strlen(Storyboard.gamename) <= 0) return(-1);
	for (int i = 0; i < STORYBOARD_MAXNODES; i++)
	{
		if (Storyboard.Nodes[i].used)
		{
			std::string check_lua_title = Storyboard.Nodes[i].title;
			if (stricmp(check_lua_title.c_str(), name) == NULL)
			{
				return i;
			}
		}
	}
	return -1;
}

int FindLuaScreenTitleNodeByKey(char* key)
{
	if (strlen(Storyboard.gamename) <= 0) return(-1);
	for (int i = 0; i < STORYBOARD_MAXNODES; i++)
	{
		if (Storyboard.Nodes[i].used)
		{
			char scanCodeName[128];
			extern unsigned int GetScancodeName(unsigned int scancode, char* buffer, unsigned int bufferLength);
			int result = GetScancodeName(Storyboard.Nodes[i].toggleKey, scanCodeName, 128);
			if (key[0] == scanCodeName[0])
			{
				return i;
			}
		}
	}
	return -1;
}

void FindLoadingScreen( void )
{
	strcpy(g_Storyboard_Current_Loading_Page, "loading.lua");

	if (g_Storyboard_Current_Level <= 0) return;
	int i = g_Storyboard_Current_Level;

	if (Storyboard.Nodes[i].used)
	{
		int iLinkTo = Storyboard.Nodes[i].input_id[0];

		//Find output.
		for (int i = 0; i < STORYBOARD_MAXNODES; i++)
		{
			if (Storyboard.Nodes[i].used)
			{
				for (int l = 0; l < STORYBOARD_MAXOUTPUTS; l++)
				{
					if (iLinkTo == Storyboard.Nodes[i].output_linkto[l])
					{
						strcpy(g_Storyboard_Current_Loading_Page, Storyboard.Nodes[i].lua_name);
						return;
					}
				}
			}
		}

	}
	return;
}

void FindFirstLevel(int &iFirstLevelNode, char *level_name, bool bFailIfNoLink)
{
	//Get loading screen connected to:
	iFirstLevelNode = -1;
	int iLoadingScreenNode = -1;
	int iLevelNode = -1;
	for (int i = 0; i < STORYBOARD_MAXNODES; i++)
	{
		if (Storyboard.Nodes[i].used)
		{
			if (stricmp(Storyboard.Nodes[i].lua_name, "loading.lua") == 0)
			{
				iLoadingScreenNode = i;
			}
		}
	}
	if (iLoadingScreenNode >= 0)
	{
		//Find connected to:
		int iLinkTo = Storyboard.Nodes[iLoadingScreenNode].output_linkto[0];
		if (iLinkTo > 0)
		{
			//Find input.
			for (int i = 0; i < STORYBOARD_MAXNODES; i++)
			{
				if (Storyboard.Nodes[i].used)
				{
					for (int l = 0; l < STORYBOARD_MAXOUTPUTS; l++)
					{
						if (iLinkTo == Storyboard.Nodes[i].input_id[l])
						{
							iLevelNode = i;
							break;
						}
					}
				}
				if (iLevelNode > 0) break;
			}
		}
	}

	if (iLevelNode > 0)
	{
		//Found first level.
		if (strlen(Storyboard.Nodes[iLevelNode].level_name) > 0)
		{
			iFirstLevelNode = iLevelNode;
			strcpy(level_name, Storyboard.Nodes[iFirstLevelNode].level_name);
			return;
		}
		else
		{
			// may have connected loading screen to another screen, that in turn connected to a level, and so on, so find that
			int iNextLinkTo = Storyboard.Nodes[iLevelNode].output_linkto[0];
			iLevelNode = -1;
			for (int i = 0; i < STORYBOARD_MAXNODES; i++)
			{
				if (Storyboard.Nodes[i].used)
				{
					for (int l = 0; l < STORYBOARD_MAXOUTPUTS; l++)
					{
						if (iNextLinkTo == Storyboard.Nodes[i].input_id[l])
						{
							iLevelNode = i;
							break;
						}
					}
				}
				if (iLevelNode > 0) break;
			}
			if (iLevelNode > 0)
			{
				if (strlen(Storyboard.Nodes[iLevelNode].level_name) > 0)
				{
					iFirstLevelNode = iLevelNode;
					strcpy(level_name, Storyboard.Nodes[iFirstLevelNode].level_name);
					return;
				}
			}
		}
	}

	if (bFailIfNoLink)
	{
		return;
	}

	//No level connected to the loading.lua page.
	//Just use the level with the internal name "Level 1"

	for (int i = 0; i < STORYBOARD_MAXNODES; i++)
	{
		if (Storyboard.Nodes[i].used)
		{
			if (Storyboard.Nodes[i].type == STORYBOARD_TYPE_LEVEL && !pestrcasestr(Storyboard.Nodes[i].lua_name, "loading"))
			{
				if (strcmp("Level 1", Storyboard.Nodes[i].levelnumber) == 0)
				{
					if (strlen(Storyboard.Nodes[i].level_name) > 0)
					{
						iFirstLevelNode = i;
						strcpy(level_name, Storyboard.Nodes[i].level_name);
						return;
					}
				}
			}
		}
	}
	return;
}

bool FindFreeLevelNode( int &iNextLevel,int &levelname ,int &iFirstNodeFree)
{
	//PE: Find next level from nodes.
	iNextLevel = 0;
	for (int i = 0; i < STORYBOARD_MAXNODES; i++)
	{
		if (Storyboard.Nodes[i].used)
		{
			if (Storyboard.Nodes[i].type == STORYBOARD_TYPE_LEVEL && !pestrcasestr(Storyboard.Nodes[i].lua_name, "loading"))
				iNextLevel++;
		}
	}
	//Free level name.
	levelname = -1;
	for (int i = 0; i < STORYBOARD_MAXNODES; i++)
	{
		char tmp[255];
		sprintf(tmp, "Level %d", i + 1);
		bool bFound = false;
		for (int a = 0; a < STORYBOARD_MAXNODES; a++)
		{
			if (Storyboard.Nodes[a].used)
			{
				if (strcmp(tmp, Storyboard.Nodes[a].levelnumber) == 0)
				{
					bFound = true;
				}
			}
		}
		if (!bFound)
		{
			levelname = i + 1;
			break;
		}
	}

	iFirstNodeFree = -1;
	for (int i = 0; i < STORYBOARD_MAXNODES; i++)
	{
		if (!Storyboard.Nodes[i].used)
		{
			iFirstNodeFree = i;
			break;
		}
	}

	return true;
}

void mapNodeStyle(void)
{
	//PE: We might need to tweek this for other styes.
	//PE: These grey with transparent to get some style colors looks better.

	ImVec4* colors = ImGui::GetStyle().Colors;
	ImVec4 fade = colors[ImGuiCol_Button] * ImVec4(0.8, 0.8, 0.8, 0.75);

	GImNodes->Style.Colors[ImNodesCol_NodeBackground] = IM_COL32(128, 128, 128, 130); //ImGui::ColorConvertFloat4ToU32(colors[ImGuiCol_WindowBg]);
	GImNodes->Style.Colors[ImNodesCol_NodeBackgroundHovered] = IM_COL32(128, 128, 128,200); //ImGui::ColorConvertFloat4ToU32(colors[ImGuiCol_Button]);
	GImNodes->Style.Colors[ImNodesCol_NodeBackgroundSelected] = IM_COL32(128, 128, 128, 200); //ImGui::ColorConvertFloat4ToU32(colors[ImGuiCol_Button]);
	GImNodes->Style.Colors[ImNodesCol_NodeOutline] = IM_COL32(128, 128, 128, 200); //ImGui::ColorConvertFloat4ToU32(fade);

	// title bar colors match ImGui's titlebg colors
	GImNodes->Style.Colors[ImNodesCol_TitleBar] = ImGui::ColorConvertFloat4ToU32(fade);
	GImNodes->Style.Colors[ImNodesCol_TitleBarHovered] = ImGui::ColorConvertFloat4ToU32(colors[ImGuiCol_Button]);
	GImNodes->Style.Colors[ImNodesCol_TitleBarSelected] = ImGui::ColorConvertFloat4ToU32(colors[ImGuiCol_Button]);

	GImNodes->Style.Colors[ImNodesCol_Link] = IM_COL32(180, 180, 180, 200);
	GImNodes->Style.Colors[ImNodesCol_LinkHovered] = IM_COL32(180, 180, 180, 255);
	GImNodes->Style.Colors[ImNodesCol_LinkSelected] = IM_COL32(180, 180, 180, 255);
	// pin colors match ImGui's button colors
	GImNodes->Style.Colors[ImNodesCol_Pin] = IM_COL32(180, 180, 180, 200);
	GImNodes->Style.Colors[ImNodesCol_PinHovered] = IM_COL32(180, 180, 180, 255);

	GImNodes->Style.Colors[ImNodesCol_BoxSelector] = IM_COL32(128, 128, 128, 30);
	GImNodes->Style.Colors[ImNodesCol_BoxSelectorOutline] = ImGui::ColorConvertFloat4ToU32(colors[ImGuiCol_PlotLines]);

	GImNodes->Style.Colors[ImNodesCol_GridBackground] = IM_COL32(0, 0, 0, 0);
	GImNodes->Style.Colors[ImNodesCol_GridLine] = IM_COL32(200, 200, 200, 25);

	// minimap colors
	GImNodes->Style.Colors[ImNodesCol_MiniMapBackground] = IM_COL32(25, 25, 25, 150);
	GImNodes->Style.Colors[ImNodesCol_MiniMapBackgroundHovered] = IM_COL32(25, 25, 25, 200);
	GImNodes->Style.Colors[ImNodesCol_MiniMapOutline] = IM_COL32(150, 150, 150, 100);
	GImNodes->Style.Colors[ImNodesCol_MiniMapOutlineHovered] = IM_COL32(150, 150, 150, 200);
	GImNodes->Style.Colors[ImNodesCol_MiniMapNodeBackground] = IM_COL32(200, 200, 200, 100);
	GImNodes->Style.Colors[ImNodesCol_MiniMapNodeBackgroundHovered] = IM_COL32(200, 200, 200, 255);
	GImNodes->Style.Colors[ImNodesCol_MiniMapNodeBackgroundSelected] = GImNodes->Style.Colors[ImNodesCol_MiniMapNodeBackgroundHovered];
	GImNodes->Style.Colors[ImNodesCol_MiniMapNodeOutline] = IM_COL32(200, 200, 200, 100);
	GImNodes->Style.Colors[ImNodesCol_MiniMapLink] = GImNodes->Style.Colors[ImNodesCol_Link];
	GImNodes->Style.Colors[ImNodesCol_MiniMapLinkSelected] = GImNodes->Style.Colors[ImNodesCol_LinkSelected];
	GImNodes->Style.Colors[ImNodesCol_MiniMapCanvas] = IM_COL32(200, 200, 200, 25);
	GImNodes->Style.Colors[ImNodesCol_MiniMapCanvasOutline] = IM_COL32(200, 200, 200, 200);
}

void hub_menubar(void)
{
	if (ImGui::BeginMenuBar())
	{
		ImVec2 CursorMenuStart = ImGui::GetCursorPos();
		bool bIsMenuHovered = false;
		if (ImGui::BeginMenu("File##Hub"))
		{
			//---------------------------------------------------------------------------

			if (bPreferences_Window == false)
			{
				if (ImGui::MenuItem("New Game Project", ""))
				{
					CloseAllOpenTools();
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

						strcpy(pref.cLastUsedStoryboardProject, "");
						bStoryboardInitNodes = false; //Just init again.
						bStoryboardFirstRunSetInitPos = false;
						process_storeboard(true); //Init a new project.
						//PE: Bug - When creating a new project, it would contain g_collectionList from prev. loaded project.
						init_rpg_system();

						bTriggerSaveAsAfterNewLevel = true;
						bTriggerSaveAs = true;
						strcpy(SaveProjectAsName, "");
						strcpy(SaveProjectAsError, "");

					}
				}
				if (!bIsMenuHovered) bIsMenuHovered = ImGui::IsItemHovered();
				if (ImGui::MenuItem("Open Game Project", ""))
				{
					CloseAllOpenToolsThatNeedSave();
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
						//Open Game Project
						bTriggerOpenProject = true;
					}
				}
				if (!bIsMenuHovered) bIsMenuHovered = ImGui::IsItemHovered();
			}

			//---------------------------------------------------------------------------
			if (ImGui::MenuItem("Exit to Desktop"))
			{
				int iAction = askBoxCancel("Are you sure you would like to exit to desktop?", "Confirmation"); //1==Yes 2=Cancel 0=No
				if (iAction == 1)
				{
					bStoryboardWindow = false;
					g_bCascadeQuitFlag = true;
				}
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Edit##Storyboard"))
		{
			if (ImGui::MenuItem("Settings", ""))
			{
				strcpy(cPreferencesMessage, "");
				bPreferences_Window = true;
			}
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}

	int preview_size_x = ImGui::GetMainViewport()->Size.x - 300.0;
	int preview_size_y = ImGui::GetMainViewport()->Size.y - 60.0;
	float fNodeWidth = 180.0f;
	float fNodeHeight = 130.0f;

	void storyboard_openproject(float preview_size_x, float fNodeWidth, float fNodeHeight,int mode);
	storyboard_openproject(preview_size_x, fNodeWidth, fNodeHeight,1);

}

void storyboard_menubar(float area_width, float node_width, float node_height)
{
	static int iCloseDownCount = 100;
	static int iCloseDownCount2 = 100;
	static int iCloseDownCount3 = 100;

	if (ImGui::BeginMenuBar())
	{
		ImVec2 CursorMenuStart = ImGui::GetCursorPos();
		if (ImGui::BeginMenu("File##Storyboard"))
		{
			bool bIsMenuHovered = false;
			if (bPreferences_Window == false)
			{
				if (ImGui::MenuItem("New Game Project", ""))
				{
					CloseAllOpenTools();
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
						bStoryboardInitNodes = false; //Just init again.
						bStoryboardFirstRunSetInitPos = false;
						strcpy(pref.cLastUsedStoryboardProject, "");
						bTriggerSaveAsAfterNewLevel = true;
						bTriggerSaveAs = true;
						strcpy(SaveProjectAsName, "");
						strcpy(SaveProjectAsError, "");

						//
					}
				}
				if (!bIsMenuHovered) bIsMenuHovered = ImGui::IsItemHovered();
				if (ImGui::MenuItem("Open Game Project", ""))
				{
					CloseAllOpenToolsThatNeedSave();
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
						//Open Game Project
						bTriggerOpenProject = true;
					}
				}
				if (!bIsMenuHovered) bIsMenuHovered = ImGui::IsItemHovered();

				//PE: Only use save if we already have a Project Name.
				if (strlen(Storyboard.gamename) > 0)
				{
					if (Storyboard.project_readonly != 1)
					{
						if (ImGui::MenuItem("Save Game Project", ""))
						{
							CloseAllOpenToolsThatNeedSave();
							//
							save_storyboard(Storyboard.gamename, false);
						}
						if (!bIsMenuHovered) bIsMenuHovered = ImGui::IsItemHovered();
					}
				}

				if (ImGui::MenuItem("Save Game Project As...", ""))//CTRL+R" ) )//F12") )
				{
					CloseAllOpenToolsThatNeedSave();
					//
					save_storyboard(Storyboard.gamename, true);
				}
				if (!bIsMenuHovered) bIsMenuHovered = ImGui::IsItemHovered();


				ImGui::Separator();
				//for (int ii = 0; ii < REMEMBERLASTFILES; ii++) { //reverse
				for (int ii = REMEMBERLASTFILES - 1; ii >= 0; ii--) {
					if (strlen(pref.last_project_files[ii]) > 0) {

						//std::string s_tmp = std::to_string(1+ii); //Reverse
						std::string s_tmp = std::to_string(REMEMBERLASTFILES - ii);
						s_tmp += ": ";
						s_tmp += pref.last_project_files[ii];

						if (ImGui::MenuItem(s_tmp.c_str())) {
							if (bWaypointDrawmode || bWaypoint_Window) { bWaypointDrawmode = false; bWaypoint_Window = false; }
							if (bImporter_Window) { importer_quit(); bImporter_Window = false; }
							if (g_bCharacterCreatorPlusActivated) g_bCharacterCreatorPlusActivated = false;
							if (bEntity_Properties_Window) bEntity_Properties_Window = false;
							if (t.ebe.on == 1) ebe_hide();
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
								load_storyboard(pref.last_project_files[ii]);
								iGamePausedNodeID = storyboard_add_missing_nodex(8, area_width, node_width, node_height + 20.0, false);
								iLoadGameNodeID = storyboard_add_missing_nodex(3, area_width, node_width, node_height + 20.0, false);
								iSaveGameNodeID = storyboard_add_missing_nodex(9, area_width, node_width, node_height + 20.0, false);
								iGraphicsNodeID = storyboard_add_missing_nodex(10, area_width, node_width, node_height + 20.0, false);
								iSoundsNodeID = storyboard_add_missing_nodex(11, area_width, node_width, node_height + 20.0, false);
								iControlNodeID = storyboard_add_missing_nodex(12, area_width, node_width, node_height + 20.0, false);
								iLoadingScreenNodeID = storyboard_add_missing_nodex(2, area_width, node_width, node_height + 20.0, false);
								iHUDScreenNodeID = storyboard_add_missing_nodex(13, area_width, node_width, node_height + 20.0, false);
							}
						}
						if (!bIsMenuHovered) bIsMenuHovered = ImGui::IsItemHovered();
					}
				}

				ImGui::Separator();
				if (!bIsMenuHovered) bIsMenuHovered = ImGui::IsItemHovered();

				if (ImGui::MenuItem("Exit to Desktop"))
				{
					if (bWaypointDrawmode || bWaypoint_Window) { bWaypointDrawmode = false; bWaypoint_Window = false; }
					if (bImporter_Window) { importer_quit(); bImporter_Window = false; }
					if (g_bCharacterCreatorPlusActivated) g_bCharacterCreatorPlusActivated = false;
					if (bEntity_Properties_Window) bEntity_Properties_Window = false;
					if (t.ebe.on == 1) ebe_hide();
					//
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
						bStoryboardWindow = false;
						g_bCascadeQuitFlag = true;
					}
				}
				if (!bIsMenuHovered) bIsMenuHovered = ImGui::IsItemHovered();
			}
			ImGui::EndMenu();
			if (!bIsMenuHovered) bIsMenuHovered = ImGui::IsItemHovered();

			if (pref.bAutoOpenMenuItems)
			{
				if (!bIsMenuHovered)
				{
					if (iCloseDownCount-- <= 0)
					{
						ImGui::OpenPopup("##DummyClose##Storyboard");
						if (ImGui::BeginMenu("##DummyClose##Storyboard"))
						{
							ImGui::EndMenu();
						}
					}
				}
				else
				{
					iCloseDownCount = 100;
				}
			}
		}
		else
		{
			if (pref.bAutoOpenMenuItems)
			{
				if (ImGui::IsItemHovered())
				{
					ImGui::OpenPopup("File##Storyboard");
					iCloseDownCount = 100;
				}
			}
		}

		//###################
		//#### Edit menu ####
		//###################

		if (ImGui::BeginMenu("Edit##Storyboard"))
		{
			bool bIsMenuHovered = false;
			if (bPreferences_Window == false)
			{
				if (ImGui::MenuItem("Add New Level", "CTRL+N"))
				{
					iStoryboardExecuteKey = 'N';
				}
				if (!bIsMenuHovered) bIsMenuHovered = ImGui::IsItemHovered();

				if (ImGui::MenuItem("Add Existing Level", "CTRL+L"))
				{
					iStoryboardExecuteKey = 'L';
				}
				if (!bIsMenuHovered) bIsMenuHovered = ImGui::IsItemHovered();

				ImGui::Separator();
				if (!bIsMenuHovered) bIsMenuHovered = ImGui::IsItemHovered();

				if (ImGui::MenuItem("Play Game", "CTRL+SPACE"))
				{
					iStoryboardExecuteKey = ' ';
				}
				if (!bIsMenuHovered) bIsMenuHovered = ImGui::IsItemHovered();

				if (ImGui::MenuItem("Play Game With Invulnerability", ""))
				{
					iStoryboardExecuteKey = '!';
				}
				if (!bIsMenuHovered) bIsMenuHovered = ImGui::IsItemHovered();

				ImGui::Separator();
				if (!bIsMenuHovered) bIsMenuHovered = ImGui::IsItemHovered();

				if (ImGui::MenuItem("Export Standalone Game", "CTRL+E"))
				{
					iStoryboardExecuteKey = 'E';
				}
				if (!bIsMenuHovered) bIsMenuHovered = ImGui::IsItemHovered();

				ImGui::Separator();
				if (!bIsMenuHovered) bIsMenuHovered = ImGui::IsItemHovered();

				if (ImGui::MenuItem("Settings", ""))
				{
					strcpy(cPreferencesMessage, "");
					bPreferences_Window = true;
				}
				if (!bIsMenuHovered) bIsMenuHovered = ImGui::IsItemHovered();
			}
			ImGui::EndMenu();
			if (!bIsMenuHovered) bIsMenuHovered = ImGui::IsItemHovered();

			if (pref.bAutoOpenMenuItems)
			{
				if (!bIsMenuHovered)
				{
					if (iCloseDownCount2-- <= 0)
					{
						ImGui::OpenPopup("##DummyClose##Storyboard");
						if (ImGui::BeginMenu("##DummyClose##Storyboard"))
						{
							ImGui::EndMenu();
						}
					}
				}
				else
				{
					iCloseDownCount2 = 100;
				}
			}
		}
		else
		{
			if (pref.bAutoOpenMenuItems)
			{
				if (ImGui::IsItemHovered())
				{
					ImGui::OpenPopup("Edit##Storyboard");
					iCloseDownCount2 = 100;
				}
			}
		}


		//###################
		//#### Help menu ####
		//###################

		if (ImGui::BeginMenu("Help##Storyboard"))
		{
			bool bIsMenuHovered = false;
			if (bPreferences_Window == false)
			{
				if (!bIsMenuHovered) bIsMenuHovered = ImGui::IsItemHovered();
				if (ImGui::MenuItem("Read User Manual"))
				{
					ExecuteFile("..\\Guides\\User Manual\\GameGuru MAX - User Guide.pdf", "", "", 0);
				}
				if (!bIsMenuHovered) bIsMenuHovered = ImGui::IsItemHovered();

				if (ImGui::MenuItem("Guides Folder"))
				{
					ExecuteFile("..\\Guides\\", "", "", 0);
				}
				if (!bIsMenuHovered) bIsMenuHovered = ImGui::IsItemHovered();

				if (ImGui::MenuItem("Report an Issue (GitHub)"))
				{
					ExecuteFile("https://github.com/TheGameCreators/GameGuruRepo/issues/new", "", "", 0);
				}
				if (!bIsMenuHovered) bIsMenuHovered = ImGui::IsItemHovered();


				if (ImGui::MenuItem("GameGuru MAX Hub")) //"Welcome Screen"
				{
					bWelcomeScreen_Window = true;
					bStoryboardWindow = false;
					cLastProjectList = ""; //Trigger a reload of projects, if anything changed.
					bWelcomeNoBackButton = true;
				}
				if (!bIsMenuHovered) bIsMenuHovered = ImGui::IsItemHovered();

				image_setlegacyimageloading(true);

				if (bAddWhatNewToMenu)
				{
					if (ImGui::MenuItem("What's New?")) //Change Log
					{
						//welcome_show(WELCOME_ANNOUNCEMENTS);
						if (gbWelcomeSystemActive == false)
						{
							welcome_init(1);
							welcome_init(2);
						}
						welcome_init(0);
					}
				}

				if (ImGui::MenuItem("About")) {
					bAbout_Window = true;
					bAbout_Window_First_Run = true;
				}
				if (!bIsMenuHovered) bIsMenuHovered = ImGui::IsItemHovered();
				image_setlegacyimageloading(false);
			}
			ImGui::EndMenu();
			if (!bIsMenuHovered) bIsMenuHovered = ImGui::IsItemHovered();

			if (pref.bAutoOpenMenuItems)
			{
				if (!bIsMenuHovered)
				{
					if (iCloseDownCount3-- <= 0)
					{
						ImGui::OpenPopup("##DummyClose##Storyboard");
						if (ImGui::BeginMenu("##DummyClose##Storyboard"))
						{
							ImGui::EndMenu();
						}
					}
				}
				else
				{
					iCloseDownCount3 = 100;
				}
			}
		}
		else
		{
			if (pref.bAutoOpenMenuItems)
			{
				if (ImGui::IsItemHovered())
				{
					ImGui::OpenPopup("Help##Storyboard");
					iCloseDownCount3 = 100;
				}
			}
		}

		ImVec2 CursorMenuEnd = ImGui::GetCursorPos();
		if (ImGui::IsMouseHoveringRect(ImGui::GetWindowPos()+CursorMenuStart+ImVec2(-4,0), ImGui::GetWindowPos() + CursorMenuEnd+ImVec2(0,24)))
		{
			iCloseDownCount = 100;
			iCloseDownCount2 = 100;
			iCloseDownCount3= 100;
		}
		ImGui::EndMenuBar();
	}
}


void save_storyboard(char *name,bool bSaveAs)
{
	cstr savename;
	if (!name) return;
	if (Storyboard.project_readonly == 1 && !bSaveAs) return;
	savename = name;
	if (bSaveAs)
	{
		//Select name.
		bTriggerSaveAs = true;
		return;
	}
	if (savename.Len() <= 0)
	{
		//Use save as.
		strcpy(cTriggerMessage, "Missing NAME of Game Project.");
		bTriggerMessage = true;
		return;
	}

	cLastProjectList = ""; //PE: Update project files.

	char project[MAX_PATH];
	if (STORYBOARDVERSION > 202)
	{
		sprintf(project, "projectbank\\%s\\project%d.dat", savename.Get(), STORYBOARDVERSION);
	}
	else
	{
		strcpy(project, "projectbank\\");
		strcat(project, savename.Get());
		strcat(project, "\\project.dat");
	}
	FILE* projectfile = GG_fopen(project, "wb+");
	if (projectfile) 
	{
		fwrite(&Storyboard, 1, sizeof(Storyboard), projectfile);
		fclose(projectfile);
		strcpy(pref.cLastUsedStoryboardProject, savename.Get());

		// save all RPG data (MAX can amend collection lists, and labels, etc)
		save_rpg_system(savename.Get(),false);
	}
	else
	{
		//Failed ?
		return;
	}
	if (1)
	{
		//Add newly saved project to recent list.
		int firstempty = -1;
		int i = 0;
		for (; i < REMEMBERLASTFILES; i++) {
			if (firstempty == -1 && strlen(pref.last_project_files[i]) <= 0)
				firstempty = i;
			if (strlen(pref.last_project_files[i]) > 0 && stricmp(savename.Get(), pref.last_project_files[i]) == 0) { //already there
				break;
			}
		}
		if (i >= REMEMBERLASTFILES) {
			if (firstempty == -1) {
				//No empty slots , rotate.
				for (int ii = 0; ii < REMEMBERLASTFILES - 1; ii++) {
					strcpy(pref.last_project_files[ii], pref.last_project_files[ii + 1]);
				}
				strcpy(pref.last_project_files[REMEMBERLASTFILES - 1], savename.Get());
			}
			else
				strcpy(pref.last_project_files[firstempty], savename.Get());
		}
	}

	Storyboard.iChanged = false;
}

static int iLastNode = -1;

// When storyboards were loaded, they were not zeroed out first, so any new arrays that were added would be filled with garbage data after load...
// ...The storyboards are now zeroed before load, but still need to ensure there is no garbage data in old projects...
void storyboard_reset_ingamehidden(StoryboardStruct* pStoryboard)
{
	for (int i = 0; i < STORYBOARD_MAXNODES; i++)
	{
		for (int j = 0; j < STORYBOARD_MAXWIDGETS; j++)
		{
			// Any garbage data can be reset here:
			// widget_ingamehidden is 1 if set by user, if it's not 0 or 1, its garbage data
			if (pStoryboard->widget_ingamehidden[i][j] != 1)
			{
				pStoryboard->widget_ingamehidden[i][j] = 0;
			}
		}
	}
}

