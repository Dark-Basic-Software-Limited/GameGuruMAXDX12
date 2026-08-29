void load_storyboard(char *name)
{
	extern void ResetStoryboardListenForKeys();
	ResetStoryboardListenForKeys();

	if (!name) return;
	if (strlen(name) <= 0) return;

	bool bProjectLoaded = false;
	char project[MAX_PATH];

	sprintf(project, "projectbank\\%s\\project%d.dat", name, STORYBOARDVERSION);
	FILE* projectfile = GG_fopen(project, "rb");
	if (projectfile == NULL)
	{
		strcpy(project, "projectbank\\");
		strcat(project, name);
		strcat(project, "\\project.dat");
		projectfile = GG_fopen(project, "rb");
	}
	if (projectfile==NULL)
	{
		// switch to remote project
		strcpy(project, "projectbank\\");
		strcat(project, name);
		strcat(project, "\\remoteproject.txt");
		if (GG_FileExists(project))
		{
			// get remote folder
			OpenToRead(1, project);
			LPSTR pRemoteProject = ReadString(1);
			CloseFile(1);

			// switch to it now
			strcpy(Storyboard.customprojectfolder, pRemoteProject);
			switch_to_remote_project(name);

			sprintf(project, "projectbank\\%s\\project%d.dat", name, STORYBOARDVERSION);
			projectfile = GG_fopen(project, "rb");

			if (projectfile == NULL)
			{
				// proceed as normal, loading now from the remote folder
				strcpy(project, "projectbank\\");
				strcat(project, name);
				strcat(project, "\\project.dat");
				projectfile = GG_fopen(project, "rb");
			}
			// remote project has own media, so update the library to include them!
			extern int g_iRefreshLibraryFoldersAfterDelay;
			g_iRefreshLibraryFoldersAfterDelay = 10;
		}
	}
	if (projectfile)
	{
		fclose(projectfile);

		//PE: Use this so we can upgrade from 202 to 203+
		//PE: Must make a backup and save out project203.dat directly. when converting from here.
		bUpgradeAndBackupOldProject = true;

		bool load__storyboard_into_struct(const char*, StoryboardStruct&);
		load__storyboard_into_struct(project, checkproject);

		bUpgradeAndBackupOldProject = false;

		char sig[12] = "Storyboard\0";
		if (checkproject.sig[0] == 'S' && checkproject.sig[8] == 'r')
		{
			//Valid Sig - Cleanup old project.
			for (int i = 0; i < STORYBOARD_MAXNODES; i++)
			{
				if (Storyboard.Nodes[i].used)
				{
					if (ImageExist(Storyboard.Nodes[i].thumb_id)) DeleteImage(Storyboard.Nodes[i].thumb_id);
				}
			}

			//PE: set defaults.
			iLoadGameNodeID = 3;
			iTitleScreenNodeID = 1;
			iGamePausedNodeID = 8;
			iHUDScreenNodeID = 13;

			// LB: can have a scenario that moves a remote project but project holds the OLD path
			// so prefer the current one than any held in the old project file
			char pCurrentPathIsBest[MAX_PATH];
			strcpy(pCurrentPathIsBest, Storyboard.customprojectfolder);
			Storyboard = checkproject;
			strcpy(Storyboard.customprojectfolder, pCurrentPathIsBest);

			bStoryboardFirstRunSetInitPos = false; //Load new thumbs, and reposition new nodes.
			Storyboard.iChanged = false;
			strcpy(pref.cLastUsedStoryboardProject, name);

			//PE: Check if this is a readonly project.
			char fullPath[MAX_PATH];
			strcpy(fullPath, project);
			GG_GetRealPath(fullPath, 0);
			extern char szRootDir[MAX_PATH];
			int rootLen = strlen(szRootDir);
			if (strnicmp(fullPath, szRootDir, rootLen) == 0)
			{
				//PE: Read only folder.
				Storyboard.project_readonly = 1;
				//PE: If game has been renamed.
				if (strcmp(name, Storyboard.gamename) != 0)
				{
					strcpy(Storyboard.gamename, name);
				}
			}
			else
			{
				//Read from write folder.
				Storyboard.project_readonly = 0;
			}

			// reset/repair any newly added fields
			storyboard_reset_ingamehidden(&Storyboard);

			// project loaded successfully
			bProjectLoaded = true;
		}
		else
		{
			strcpy(cTriggerMessage, "Could not load project, bad signature.");
			bTriggerMessage = true;
			fclose(projectfile);
		}
	}
	else
	{
		strcpy(cTriggerMessage, "Could not find project.");
		bTriggerMessage = true;
	}

	// Reset all node and image IDs to default - they don't need their state retained by saving, and under some circumstances when loading them, they can be duplicated, causing problems with selecting nodes and images.
	int iUniqueIds = STORYBOARD_THUMBS;
	int iUniqueIdsAdd = 1000;
	for (int i = 0; i < STORYBOARD_MAXNODES; i++)
	{
		if (i == 100 || i == 200)
			iUniqueIdsAdd += 100000;

		Storyboard.Nodes[i].id = iUniqueIds;
		Storyboard.Nodes[i].thumb_id = iUniqueIds;
		for (int l = 0; l < STORYBOARD_MAXWIDGETS; l++)
		{
			Storyboard.Nodes[i].widget_normal_thumb_id[l] = iUniqueIds + iUniqueIdsAdd + (1000 * l) + 600;
			Storyboard.Nodes[i].widget_highlight_thumb_id[l] = iUniqueIds + iUniqueIdsAdd + (1000 * l) + 700;
			Storyboard.Nodes[i].widget_selected_thumb_id[l] = iUniqueIds + iUniqueIdsAdd + (1000 * l) + 800;
		}
		Storyboard.Nodes[i].screen_backdrop_id = iUniqueIds + 500;

		iUniqueIds++;
	}

	//PE: Fix all output input unique id problems.
	storeboard_fix_uniqueids();

	// and load game project rpg databases, including item collection data
	init_rpg_system();
	load_rpg_system(name);

	// as we load a storyboard project, ensure correct grass and lensflare assets are being used for editor mode
	extern void ReloadLensFlareImages();
	if (strlen(Storyboard.customprojectfolder) > 0)
	{
		// load grass and lensflare from remote project area
		char pRemotePathToGrass[MAX_PATH];
		strcpy(pRemotePathToGrass, Storyboard.customprojectfolder);
		strcat(pRemotePathToGrass, "\\");
		strcat(pRemotePathToGrass, Storyboard.gamename);
		strcat(pRemotePathToGrass, "\\Files\\");
		GGGrass_Init_Textures(pRemotePathToGrass);
		ReloadLensFlareImages();

		//PE: Add custom fonts from remote project.
		iLaunchAfterSync = 699;
	}
	else
	{
		// load grass and lensflare from stock assets
		GGGrass_Init_Textures("");
		ReloadLensFlareImages();
	}

	// project can bounce between stock and remote resources, so always reload on new storyboard project load
	common_mustreload_foreachnewproject();

	// complete
	iLastNode = -1;
}

bool load_checkproject_storyboard(char *name)
{
	if (!name) return false;
	if (strlen(name) <= 0) return false;

	char project[MAX_PATH];
	sprintf(project, "projectbank\\%s\\project%d.dat", name, STORYBOARDVERSION);
	FILE* projectfile = GG_fopen(project, "rb");
	if (!projectfile)
	{
		strcpy(project, "projectbank\\");
		strcat(project, name);
		strcat(project, "\\project.dat");
		projectfile = GG_fopen(project, "rb");
	}

	bool bReadProjectDetails = false;
	if (projectfile)
	{
		bReadProjectDetails = true;
	}
	else
	{
		// may be a remote project
		strcpy(project, "projectbank\\");
		strcat(project, name);
		strcat(project, "\\remoteproject.txt");
		if (FileExist(project) == 1)
		{
			OpenToRead(1, project);
			strcpy(project, ReadString(1));
			strcat(project, name);
			strcat(project, "\\Files\\projectbank\\");
			strcat(project, name);
			strcat(project, "\\project.dat");
			CloseFile(1);
		}

		char newversion[MAX_PATH];
		sprintf(newversion, "project%d.dat", STORYBOARDVERSION);

		std::string newname = project;
		replaceAll(newname, "project.dat", newversion);
		projectfile = GG_fopen(newname.c_str(), "rb");
		if (!projectfile)
			projectfile = GG_fopen(project, "rb");
		else
			strcpy(project, newname.c_str());

		if (projectfile)
		{
			bReadProjectDetails = true;
		}
	}
	if (bReadProjectDetails==true)
	{
		fclose(projectfile);

		//PE: Use this so we can upgrade from 202 to 203+
		bool load__storyboard_into_struct(const char*, StoryboardStruct&);
		load__storyboard_into_struct(project, checkproject);

		char sig[12] = "Storyboard\0";
		if (checkproject.sig[0] == 'S' && checkproject.sig[8] == 'r')
		{
			storyboard_reset_ingamehidden(&checkproject);
			return true;
		}
	}
	return false;
}

bool load__storyboard_into_struct(const char *filepath, StoryboardStruct& storyboard)
{
	if (!filepath) return false;
	if (strlen(filepath) <= 0) return false;
	
	char project[MAX_PATH];
	sprintf(project, "project%d.dat", STORYBOARDVERSION);
	std::string newname = filepath;
	replaceAll(newname, "project.dat", project);
	FILE* projectfile = GG_fopen(newname.c_str(), "rb");
	if (!projectfile)
	{
		projectfile = GG_fopen(filepath, "rb");
	}
	if (projectfile)
	{
		memset(&storyboard, 0, sizeof(StoryboardStruct));
		//PE: Features added to STORYBOARDVERSION 203 that need default values other then 0.
		//PE: None yet. but like: storyboard.project_active = 1

		size_t size = fread(&storyboard, 1, sizeof(storyboard), projectfile);
		//Valid pref:
		fclose(projectfile);
		char sig[12] = "Storyboard\0";
		if (storyboard.sig[0] == 'S' && storyboard.sig[8] == 'r')
		{
			//PE: Need convert 202 -> 202+
			if (storyboard.iStoryboardVersion != STORYBOARDVERSION)
			{
				if (storyboard.iStoryboardVersion <= 202)
				{
					memset(&updateproject202, 0, sizeof(StoryboardStruct202));

					FILE* projectfile = GG_fopen(filepath, "rb");
					if (projectfile)
					{
						memset(&storyboard, 0, sizeof(StoryboardStruct));
						size_t size = fread(&updateproject202, 1, sizeof(updateproject202), projectfile);
						fclose(projectfile);

						//PE: Copy data.
						strcpy(storyboard.sig, updateproject202.sig);
						strcpy(storyboard.gamename, updateproject202.gamename);
						storyboard.iStoryboardVersion = STORYBOARDVERSION; //PE: New version.
						storyboard.iChanged = updateproject202.iChanged;
						storyboard.vEditorPanning = updateproject202.vEditorPanning;
						for (int i = 0; i < STORYBOARD_MAXNODES202; i++)
						{
							//PE: Copy old nodes.

							{
								storyboard.Nodes[i].type = updateproject202.Nodes[i].type;
								storyboard.Nodes[i].id = updateproject202.Nodes[i].id;
								storyboard.Nodes[i].restore_position = updateproject202.Nodes[i].restore_position;
								storyboard.Nodes[i].iEditEnable = updateproject202.Nodes[i].iEditEnable;
								storyboard.Nodes[i].used = updateproject202.Nodes[i].used;
								storyboard.Nodes[i].thumb_id = updateproject202.Nodes[i].thumb_id;

								strcpy(storyboard.Nodes[i].title, updateproject202.Nodes[i].title);
								strcpy(storyboard.Nodes[i].levelnumber, updateproject202.Nodes[i].levelnumber);
								strcpy(storyboard.Nodes[i].thumb, updateproject202.Nodes[i].thumb);
								strcpy(storyboard.Nodes[i].level_name, updateproject202.Nodes[i].level_name);
								strcpy(storyboard.Nodes[i].lua_name, updateproject202.Nodes[i].lua_name);
								strcpy(storyboard.Nodes[i].scene_name, updateproject202.Nodes[i].scene_name);

								for (int b = 0; b < STORYBOARD_MAXOUTPUTS202; b++)
								{
									storyboard.Nodes[i].input_id[b] = updateproject202.Nodes[i].input_id[b];
									storyboard.Nodes[i].output_id[b] = updateproject202.Nodes[i].output_id[b];
									storyboard.Nodes[i].output_linkto[b] = updateproject202.Nodes[i].output_linkto[b];
									storyboard.Nodes[i].output_can_link_to_type[b] = updateproject202.Nodes[i].output_can_link_to_type[b];
									strcpy(storyboard.Nodes[i].output_action[b], updateproject202.Nodes[i].output_action[b]);
									strcpy(storyboard.Nodes[i].output_title[b], updateproject202.Nodes[i].output_title[b]);
									strcpy(storyboard.Nodes[i].input_title[b], updateproject202.Nodes[i].input_title[b]);
									strcpy(storyboard.Nodes[i].input_action[b], updateproject202.Nodes[i].input_action[b]);
								}

								strcpy(storyboard.Nodes[i].screen_title, updateproject202.Nodes[i].screen_title);
								strcpy(storyboard.Nodes[i].screen_music, updateproject202.Nodes[i].screen_music);
								strcpy(storyboard.Nodes[i].screen_backdrop, updateproject202.Nodes[i].screen_backdrop);

								storyboard.Nodes[i].screen_backdrop_id = updateproject202.Nodes[i].screen_backdrop_id;

								storyboard.Nodes[i].screen_back_color = updateproject202.Nodes[i].screen_back_color;
								storyboard.Nodes[i].screen_backdrop_placement = updateproject202.Nodes[i].screen_backdrop_placement;
								storyboard.Nodes[i].screen_grid_size = updateproject202.Nodes[i].screen_grid_size;
								for(int c = 0; c < 10; c++)
									storyboard.Nodes[i].screen_backdrop_ratio_placement[c] = updateproject202.Nodes[i].screen_backdrop_ratio_placement[c];
								
								strcpy(storyboard.Nodes[i].screen_thumb, updateproject202.Nodes[i].screen_thumb);


								for (int b = 0; b < STORYBOARD_MAXWIDGETS202; b++)
								{
									strcpy(storyboard.Nodes[i].widget_label[b], updateproject202.Nodes[i].widget_label[b]);
									strcpy(storyboard.Nodes[i].widget_normal_thumb[b], updateproject202.Nodes[i].widget_normal_thumb[b]);

									strcpy(storyboard.Nodes[i].widget_highlight_thumb[b], updateproject202.Nodes[i].widget_highlight_thumb[b]);
									strcpy(storyboard.Nodes[i].widget_selected_thumb[b], updateproject202.Nodes[i].widget_selected_thumb[b]);
									strcpy(storyboard.Nodes[i].widget_font[b], updateproject202.Nodes[i].widget_font[b]);
									strcpy(storyboard.Nodes[i].widget_name[b], updateproject202.Nodes[i].widget_name[b]);
									strcpy(storyboard.Nodes[i].widget_click_sound[b], updateproject202.Nodes[i].widget_click_sound[b]);

									storyboard.Nodes[i].widget_used[b] = updateproject202.Nodes[i].widget_used[b];
									storyboard.Nodes[i].widget_size[b] = updateproject202.Nodes[i].widget_size[b];

									storyboard.Nodes[i].widget_pos[b] = updateproject202.Nodes[i].widget_pos[b];
									storyboard.Nodes[i].widget_normal_thumb_id[b] = updateproject202.Nodes[i].widget_normal_thumb_id[b];
									storyboard.Nodes[i].widget_highlight_thumb_id[b] = updateproject202.Nodes[i].widget_highlight_thumb_id[b];
									storyboard.Nodes[i].widget_selected_thumb_id[b] = updateproject202.Nodes[i].widget_selected_thumb_id[b];
									storyboard.Nodes[i].widget_action[b] = updateproject202.Nodes[i].widget_action[b];
									storyboard.Nodes[i].widget_font_color[b] = updateproject202.Nodes[i].widget_font_color[b];

									storyboard.Nodes[i].widget_font_size[b] = updateproject202.Nodes[i].widget_font_size[b];
									storyboard.Nodes[i].widget_type[b] = updateproject202.Nodes[i].widget_type[b];
									storyboard.Nodes[i].widget_read_only[b] = updateproject202.Nodes[i].widget_read_only[b];
									storyboard.Nodes[i].widget_layer[b] = updateproject202.Nodes[i].widget_layer[b];
									storyboard.Nodes[i].widget_initial_value[b] = updateproject202.Nodes[i].widget_initial_value[b];
#ifdef EMULATERESOLUTION
									storyboard.Nodes[i].universal_resolution[b] = updateproject202.Nodes[i].universal_resolution[b];
#endif

								}

								storyboard.Nodes[i].screen_backdrop_transparent = updateproject202.Nodes[i].screen_backdrop_transparent;

								storyboard.Nodes[i].readouts_available = updateproject202.Nodes[i].readouts_available;
								storyboard.Nodes[i].widgets_available = updateproject202.Nodes[i].widgets_available;
								storyboard.Nodes[i].toggleKey = updateproject202.Nodes[i].toggleKey;
								storyboard.Nodes[i].showAtStart = updateproject202.Nodes[i].showAtStart;
								storyboard.Nodes[i].loop_music = updateproject202.Nodes[i].loop_music;
							}

							storyboard.NodeRadioButtonSelected[i] = updateproject202.NodeRadioButtonSelected[i];
							for (int b = 0; b < STORYBOARD_MAXWIDGETS202; b++)
							{
								storyboard.NodeSliderValues[i][b] = updateproject202.NodeSliderValues[i][b];
								storyboard.widget_colors[i][b] = updateproject202.widget_colors[i][b];
								strcpy(storyboard.widget_readout[i][b], updateproject202.widget_readout[i][b]);
								storyboard.widget_textoffset[i][b] = updateproject202.widget_textoffset[i][b];
								storyboard.widget_ingamehidden[i][b] = updateproject202.widget_ingamehidden[i][b];
								storyboard.widget_drawordergroup[i][b] = updateproject202.widget_drawordergroup[i][b];
							}
						}
						strcpy(storyboard.game_icon, updateproject202.game_icon);
						strcpy(storyboard.game_thumb, updateproject202.game_thumb);
						strcpy(storyboard.game_description, updateproject202.game_description);
						strcpy(storyboard.game_world_edge_text, updateproject202.game_world_edge_text);
						strcpy(storyboard.game_developer_desc, updateproject202.game_developer_desc);
						strcpy(storyboard.customprojectfolder, updateproject202.customprojectfolder);
						storyboard.project_readonly = updateproject202.project_readonly;
						storyboard.game_thumb_id = updateproject202.game_thumb_id;
						storyboard.game_icon_id = updateproject202.game_icon_id;

						if (bUpgradeAndBackupOldProject)
						{
							char CopyFrom[MAX_PATH];
							strcpy(CopyFrom, filepath);
							GG_GetRealPath(CopyFrom, 0); //Resolve name. need full path.
							extern char szRootDir[MAX_PATH];
							int rootLen = strlen(szRootDir);
							if (strnicmp(CopyFrom, szRootDir, rootLen) == 0)
							{
								//PE: Read only folder.
								storyboard.project_readonly = 1;
							}

							if (storyboard.project_readonly != 1)
							{
								//PE: Make backup and save project203.dat.
								char CopyTo[MAX_PATH];
								strcpy(CopyTo, filepath);
								strcat(CopyTo, ".bck");
								GG_GetRealPath(CopyTo, 1); //Resolve name. need full path.
								bool bRet = CopyFileA(CopyFrom, CopyTo, FALSE);

								cstr savename = storyboard.gamename;
								sprintf(project, "projectbank\\%s\\project%d.dat", savename.Get(), STORYBOARDVERSION);
								FILE* projectfile = GG_fopen(project, "wb+");
								if (projectfile)
								{
									fwrite(&storyboard, 1, sizeof(Storyboard), projectfile);
									fclose(projectfile);
								}
							}
						}

					}

				}
			}
			return true;
		}
	}
	return false;
}

void GetProjectList(char *path, bool bGetThumbs)
{
	if (!path) return;
	int uniqueId = 16000;

	if (cLastProjectList != path)
	{
		projectbank_list.clear();
		projectbank_imageid.clear();
		projectbank_image.clear();
		projectbank_active.clear();

		cLastProjectList = path;
		cstr pOldDir = GetDir();

		char destination[MAX_PATH];
		strcpy(destination, path);
		GG_GetRealPath(destination, 1); //We need to path to the write folder.

		SetDir(destination);
		ChecklistForFiles();
		SetDir(pOldDir.Get());
		for (int c = 1; c <= ChecklistQuantity(); c++)
		{
			if (ChecklistValueA(c) != 0)
			{
				// only folders
				cstr folder = ChecklistString(c);
				if (folder != "." && folder != "..")
				{
					bool bIgnore = true;

					bool bHaveAProject = false;
					char project[MAX_PATH];
					sprintf(project, "projectbank\\%s\\project%d.dat", folder.Get(), STORYBOARDVERSION);
					if (!GG_FileExists(project))
					{
						strcpy(project, destination);
						strcat(project, folder.Get());
						strcat(project, "\\project.dat");
					}
					if (GG_FileExists(project))
					{
						bHaveAProject = true;
					}
					else
					{
						strcpy(project, destination);
						strcat(project, folder.Get());
						strcat(project, "\\remoteproject.txt");
						if (GG_FileExists(project))
						{
							bHaveAProject = true;
						}
					}
					//PE: Must have a project.
					if (bHaveAProject == true)
					{
						bIgnore = false;
					}
					if (!bIgnore)
					{
						projectbank_list.push_back(folder.Get());
						projectbank_active.push_back(true);

						//PE: We are always sorting , so just set all to CLICK HERE.
						//if (!bGetThumbs)
						//{
							projectbank_image.push_back(""); //Just use CLICK HERE.
						//}
						projectbank_imageid.push_back(0);
					}
				}
			}
		}
		SetDir(pOldDir.Get());
	}
}


bool bWidgetMouseDraggin = false;
void storyboard_control_widget(int nodeid, int index, ImVec2 pos, ImVec2 size, ImRect rMonitorArea, ImVec2 vMonitorStart, ImVec2 vScale)
{

	ImGuiWindow* window = ImGui::GetCurrentWindow();
	ImVec2 ocpos = ImGui::GetCursorPos();
	ImGui::SetCursorPos(vMonitorStart);
	ImVec4 tool_selected_col = ImGui::GetStyle().Colors[ImGuiCol_PlotHistogram];
	ImVec2 padding = { 2.0, 2.0 };
	window->DrawList->AddRect(rMonitorArea.Min + pos - padding, rMonitorArea.Min + pos + size + padding, ImGui::GetColorU32(tool_selected_col));

	ImVec2 but_size = ImVec2(12.0, 12.0);
	ImVec2 fp = ImGui::GetStyle().FramePadding;
	float fpb = ImGui::GetStyle().FrameBorderSize;
	ImVec4 *style_colors = ImGui::GetStyle().Colors;
	ImVec4 oldbutA = style_colors[ImGuiCol_ButtonActive];
	ImVec4 oldbutH = style_colors[ImGuiCol_ButtonHovered];
	ImVec4 oldbutN = style_colors[ImGuiCol_Button];
	style_colors[ImGuiCol_Button] = tool_selected_col;
	style_colors[ImGuiCol_ButtonHovered] = tool_selected_col;
	style_colors[ImGuiCol_ButtonHovered].w *= 0.80f;
	style_colors[ImGuiCol_ButtonActive] = tool_selected_col;

	ImGui::GetStyle().FramePadding = ImVec2(0, 0);
	ImGui::GetStyle().FrameBorderSize = 0;

	ImGui::SetCursorPos(vMonitorStart + pos - (but_size*0.5) );
	ImGui::SetItemAllowOverlap();
	bool bSizeHover = false;
	bool bSizeHover2 = false;
	if (ImGui::ButtonEx("##TopLeftCorner", but_size , 0))
	{
		//Resize, todo if time.
	}
	if (ImGui::IsItemHovered()) bSizeHover2 = true;
	ImGui::SetCursorPos(vMonitorStart + pos + ImVec2(size.x,0) - (but_size*0.5));
	ImGui::SetItemAllowOverlap();
	if (ImGui::ButtonEx("##TopRightCorner", but_size, 0))
	{
		//Resize, todo if time.
	}
	if (ImGui::IsItemHovered()) bSizeHover = true;
	ImGui::SetCursorPos(vMonitorStart + pos + ImVec2(0, size.y) - (but_size*0.5));
	ImGui::SetItemAllowOverlap();
	if (ImGui::ButtonEx("##BotLeftCorner", but_size, 0))
	{
		//Resize, todo if time.
	}
	if (ImGui::IsItemHovered()) bSizeHover2 = true;
	ImGui::SetCursorPos(vMonitorStart + pos + size - (but_size*0.5));
	ImGui::SetItemAllowOverlap();
	if (ImGui::ButtonEx("##BotRightCorner", but_size, 0))
	{
		//Resize, todo if time.
	}
	if (ImGui::IsItemHovered()) bSizeHover = true;

	ImGui::GetStyle().FramePadding = fp;
	ImGui::GetStyle().FrameBorderSize = fpb;
	style_colors[ImGuiCol_Button] = oldbutN;
	style_colors[ImGuiCol_ButtonHovered] = oldbutH;
	style_colors[ImGuiCol_ButtonActive] = oldbutA;

	static ImVec2 vMovePos;
	static int iMoveType = 0;

	static bool bWaitForRelease = false;
	if (bWaitForRelease)
	{
		if (!ImGui::IsMouseDown(0))
		{
			bWaitForRelease = false;
		}
	}
	else if (ImGui::IsMouseDown(0))
	{
		iMoveType = 0;
		if (bSizeHover)
			iMoveType = 1;
		if (bSizeHover2)
			iMoveType = 2;
		bWaitForRelease = true;
	}
	
	if (!bExternal_Entities_Window && !bVideoPlayerMaximized && !bPreferences_Window && !bLastSmallVideoPlayerMaximized)
	{
		static int bShowCenterLines = 0;
		static bool bNoSnapping = false;
		float fAdjustX = 0.0, fAdjustY = 0.0;
		ImVec2 fMouseToPercent = ImVec2(100.0 / (1920.0*vScale.x), 100.0 / (1080.0*vScale.y));
		int grid = Storyboard.Nodes[nodeid].screen_grid_size;
		ImVec2 mPos = ImGui::GetMousePos();
		if (mPos.x > rMonitorArea.Min.x && mPos.x < rMonitorArea.Max.x && mPos.y > rMonitorArea.Min.y && mPos.y < rMonitorArea.Max.y)
		{
			// only if within monitor area
			if (ImGui::IsKeyPressed(39, true))
			{
				if (grid > 0)
					fAdjustX = grid;
				else
					fAdjustX = 1.0 * fMouseToPercent.x;
			}
			if (ImGui::IsKeyPressed(37, true))
			{
				if (grid > 0)
					fAdjustX = -grid;
				else
					fAdjustX = -1.0 * fMouseToPercent.x;
			}
			if (ImGui::IsKeyPressed(38, true))
			{
				if (grid > 0)
					fAdjustY = -grid;
				else
					fAdjustY = -1.0 * fMouseToPercent.y;
			}
			if (ImGui::IsKeyPressed(40, true))
			{
				if (grid > 0)
					fAdjustY = grid;
				else
					fAdjustY = 1.0 * fMouseToPercent.y;
			}
		}
		if (fAdjustX != 0.0 || fAdjustY != 0.0)
		{
			if (fAdjustX != 0.0) Storyboard.Nodes[nodeid].widget_pos[index].x += fAdjustX;
			if (fAdjustY != 0.0) Storyboard.Nodes[nodeid].widget_pos[index].y += fAdjustY;
			if (grid > 0)
			{
				if (fAdjustX != 0.0) Storyboard.Nodes[nodeid].widget_pos[index].x -= fmod(Storyboard.Nodes[nodeid].widget_pos[index].x, grid);
				if (fAdjustY != 0.0) Storyboard.Nodes[nodeid].widget_pos[index].y -= fmod(Storyboard.Nodes[nodeid].widget_pos[index].y, grid);
			}
			bShowCenterLines = 30; //PE: This adds snapping so we cant move after this.
			bNoSnapping = true;
		}

		ImVec2 vLargerGrabArea = ImVec2(10.0, 10.0);
		if (ImGui::IsMouseHoveringRect(rMonitorArea.Min + pos - vLargerGrabArea, rMonitorArea.Min + pos + size + vLargerGrabArea) || bWidgetMouseDraggin)
		{
			//Drag if mouse down.
			if (ImGui::IsMouseDown(0) && ImGui::IsMouseDragging(0))
			{
				if (!bWidgetMouseDraggin)
				{
					vMovePos = Storyboard.Nodes[nodeid].widget_pos[index];
				}
				if (iMoveType == 1)
				{
					float fAdjust = ImGui::GetIO().MouseDelta.x / 350.0;
					fAdjust += ImGui::GetIO().MouseDelta.y / 350.0;
					if (Storyboard.Nodes[nodeid].widget_type[index] == STORYBOARD_WIDGET_BUTTON 
					|| Storyboard.Nodes[nodeid].widget_type[index] == STORYBOARD_WIDGET_PROGRESS
					|| Storyboard.Nodes[nodeid].widget_type[index] == STORYBOARD_WIDGET_SLIDER
					|| Storyboard.Nodes[nodeid].widget_type[index] == STORYBOARD_WIDGET_BAR)
					{
						Storyboard.Nodes[nodeid].widget_size[index].x += fAdjust;
						if (Storyboard.Nodes[nodeid].widget_size[index].x > 4.0) Storyboard.Nodes[nodeid].widget_size[index].x = 4.0;
						Storyboard.Nodes[nodeid].widget_size[index].y = Storyboard.Nodes[nodeid].widget_size[index].x;
					}
					if (Storyboard.Nodes[nodeid].widget_type[index] == STORYBOARD_WIDGET_TEXT 
					|| Storyboard.Nodes[nodeid].widget_type[index] == STORYBOARD_WIDGET_TEXTAREA)
					{
						Storyboard.Nodes[nodeid].widget_font_size[index] += fAdjust;
						if (Storyboard.Nodes[nodeid].widget_font_size[index] > 3.0) Storyboard.Nodes[nodeid].widget_font_size[index] = 3.0;
						if (Storyboard.Nodes[nodeid].widget_font_size[index] < 0.05) Storyboard.Nodes[nodeid].widget_font_size[index] = 0.05;
					}
				}
				else if (iMoveType == 2)
				{			
					float fAdjust = ImGui::GetIO().MouseDelta.x / 350.0;
					fAdjust += ImGui::GetIO().MouseDelta.y / 350.0;
					if (Storyboard.Nodes[nodeid].widget_type[index] == STORYBOARD_WIDGET_BUTTON 
					|| Storyboard.Nodes[nodeid].widget_type[index] == STORYBOARD_WIDGET_PROGRESS
					|| Storyboard.Nodes[nodeid].widget_type[index] == STORYBOARD_WIDGET_SLIDER
					|| Storyboard.Nodes[nodeid].widget_type[index] == STORYBOARD_WIDGET_BAR)
					{
						Storyboard.Nodes[nodeid].widget_size[index].x -= fAdjust;
						if (Storyboard.Nodes[nodeid].widget_size[index].x > 4.0) Storyboard.Nodes[nodeid].widget_size[index].x = 4.0;
						Storyboard.Nodes[nodeid].widget_size[index].y = Storyboard.Nodes[nodeid].widget_size[index].x;
					}
					if (Storyboard.Nodes[nodeid].widget_type[index] == STORYBOARD_WIDGET_TEXT 
					|| Storyboard.Nodes[nodeid].widget_type[index] == STORYBOARD_WIDGET_TEXTAREA)
					{
						Storyboard.Nodes[nodeid].widget_font_size[index] -= fAdjust;
						if (Storyboard.Nodes[nodeid].widget_font_size[index] > 3.0) Storyboard.Nodes[nodeid].widget_font_size[index] = 3.0;
						if (Storyboard.Nodes[nodeid].widget_font_size[index] < 0.05) Storyboard.Nodes[nodeid].widget_font_size[index] = 0.05;
					}
				}
				else
				{
					vMovePos.x += ImGui::GetIO().MouseDelta.x * fMouseToPercent.x;
					vMovePos.y += ImGui::GetIO().MouseDelta.y * fMouseToPercent.y;

					if (Storyboard.Nodes[nodeid].screen_grid_size > 0)
					{
						int grid = Storyboard.Nodes[nodeid].screen_grid_size;
						float adjustx = fmod(vMovePos.x, grid);
						float adjusty = fmod(vMovePos.y, grid);
						Storyboard.Nodes[nodeid].widget_pos[index].x = vMovePos.x - adjustx;
						Storyboard.Nodes[nodeid].widget_pos[index].y = vMovePos.y - adjusty;
					}
					else
					{
						Storyboard.Nodes[nodeid].widget_pos[index] = vMovePos;
					}
					bShowCenterLines = 1;
					bNoSnapping = false;
				}
				bWidgetMouseDraggin = true;

			}
			if (!ImGui::IsMouseDown(0))
			{
				bWidgetMouseDraggin = false;
			}
		}

		if (bShowCenterLines > 0)
		{
			bShowCenterLines--;
			float minsnapping = 49.8;
			float maxsnapping = 50.2;
			if (bNoSnapping)
			{
				//PE: One pixel.
				minsnapping = 50.0 - ((1.0 * fMouseToPercent.x) * 0.5);
				maxsnapping = 50.0 + ((1.0 * fMouseToPercent.x) * 0.5);
			}
			//Show center lines.
			if (Storyboard.Nodes[nodeid].widget_pos[index].x >= minsnapping && Storyboard.Nodes[nodeid].widget_pos[index].x <= maxsnapping)
			{
				//if(!bNoSnapping) 
				Storyboard.Nodes[nodeid].widget_pos[index].x = 50.0; //Align to center
				//Draw center line.
				float centerx = (rMonitorArea.Max.x - rMonitorArea.Min.x) * 0.5;
				centerx += rMonitorArea.Min.x;
				window->DrawList->AddLine(ImVec2(centerx, rMonitorArea.Min.y), ImVec2(centerx, rMonitorArea.Max.y), ImGui::GetColorU32(tool_selected_col));
			}

			if (bNoSnapping)
			{
				//PE: One pixel.
				minsnapping = 50.0 - ((1.0 * fMouseToPercent.y) * 0.5);
				maxsnapping = 50.0 + ((1.0 * fMouseToPercent.y) * 0.5);
			}

			float fPercent = (rMonitorArea.Max.y - rMonitorArea.Min.y) / 100.0;
			float fSizePivotCenterY = (size.y*0.5) / fPercent;
			//Show center Y line
			if (Storyboard.Nodes[nodeid].widget_pos[index].y + fSizePivotCenterY >= minsnapping && Storyboard.Nodes[nodeid].widget_pos[index].y + fSizePivotCenterY <= maxsnapping)
			{
				Storyboard.Nodes[nodeid].widget_pos[index].y = 50.0 - fSizePivotCenterY; //Align to center Y
				//Draw center line.
				float centery = (rMonitorArea.Max.y - rMonitorArea.Min.y) * 0.5;
				centery += rMonitorArea.Min.y;
				window->DrawList->AddLine(ImVec2(rMonitorArea.Min.x, centery), ImVec2(rMonitorArea.Max.x, centery), ImGui::GetColorU32(tool_selected_col));
			}
		}
	}
	ImGui::SetCursorPos(ocpos);
}

extern ImFont* customfont;
extern ImFont* customfontlarge;
float WidgetSelectUsedFont(int nodeid, int index)
{
	
	for (int i = 0; i < StoryboardFonts.size(); i++)
	{
		bool bIsSelected = false;
		if (stricmp(StoryboardFonts[i].second.c_str(), Storyboard.Nodes[nodeid].widget_font[index]) == NULL)
		{
			ImGui::PushFont(StoryboardFonts[i].first);  //storyboard special fonts.
			return 2.0; //2.0=60,2.5=48
		}
	}
	ImGui::PushFont(customfontlarge);  //defaultfont
	return 2.0;
}

static int iUpdateBackDropNode = -1;
static int iUpdateWidgetThumbNode = -1;
static int iUpdateWidgetThumbButton = -1;
static bool bPreviewScreen = false;
static bool bLastStandalone = false;
static bool bDisplayGrid = false;
static bool bPlacingNewWidget = false;
std::vector<int> Storyboard_ActiveWidgets;

bool IsHardcodedID (int nodeID, int widgetID)
{
	bool bHardcoded = false;
	if (nodeID == iTitleScreenNodeID && widgetID <= 4) bHardcoded = true;
	if (nodeID == iAboutScreenNodeID && widgetID <= 2) bHardcoded = true;
	if (nodeID == iGamePausedNodeID && widgetID <= 7) bHardcoded = true;
	if (nodeID == iGraphicsNodeID && widgetID <= 6) bHardcoded = true;
	if (nodeID == iSoundsNodeID && widgetID <= 5) bHardcoded = true;
	if (nodeID == iSaveGameNodeID && widgetID <= 9) bHardcoded = true;
	if (nodeID == iLoadGameNodeID && widgetID <= 9) bHardcoded = true;
	if (nodeID == iLoadingScreenNodeID && widgetID <= 2) bHardcoded = true;
	if (nodeID == iControlNodeID && widgetID <= 7) bHardcoded = true;
	return bHardcoded;
}

int AddWidgetToScreen(int nodeID, STORYBOARD_WIDGET_ type, std::string readoutTitle = "")
{
	StoryboardNodesStruct& node = Storyboard.Nodes[nodeID];

	// Find a free slot to add the widget to
	int widgetSlot = -1;
	for (int i = 0; i < STORYBOARD_MAXWIDGETS; i++)
	{
		bool bHardcodedIDForNow = IsHardcodedID(nodeID, i);
		if (bHardcodedIDForNow == false)
		{
			if (node.widget_used[i] == 0)
			{
				widgetSlot = i;
				node.widget_used[i] = 1;
				break;
			}
		}
	}
	if (widgetSlot < 0)
	{
		// No free space for widgets
		return widgetSlot;
	}

	// Set widget defaults
	node.widget_type[widgetSlot] = type;
	Storyboard.widget_colors[nodeID][widgetSlot] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
	Storyboard.Nodes[nodeID].widget_size[widgetSlot] = ImVec2(1.0f, 1.0f);
	node.widget_pos[widgetSlot].x = 200 / ImGui::GetMainViewport()->Size.x * 100;
	node.widget_pos[widgetSlot].y = 200 / ImGui::GetMainViewport()->Size.y * 100;
	if (readoutTitle.length() > 0)
	{
		strcpy(Storyboard.widget_readout[nodeID][widgetSlot], readoutTitle.c_str());
	}
	else
	{
		// Reset in case this widget was previously used as a readout but no longer is
		Storyboard.widget_readout[nodeID][widgetSlot][0] = 0;
	}
	Storyboard.widget_textoffset[nodeID][widgetSlot] = ImVec2(0.0f, 0.0f);
	Storyboard.widget_ingamehidden[nodeID][widgetSlot] = 0;
	Storyboard.widget_drawordergroup[nodeID][widgetSlot] = 0;

	// Settings specific to widget type:
	if (type == STORYBOARD_WIDGET_IMAGE)
	{
		if (readoutTitle.length() > 0)
		{
			if (strcmp(readoutTitle.c_str(),"Weapon Held") == 0)
			{
				strcpy(node.widget_normal_thumb[widgetSlot], "languagebank\\neutral\\gamecore\\huds\\ammohealth\\icon-colt.png");
			}
			else if (strcmp(readoutTitle.c_str(), "Weapon Firemode") == 0)
			{
				strcpy(node.widget_normal_thumb[widgetSlot], "languagebank\\neutral\\gamecore\\huds\\ammohealth\\ammo-icon-handgun.png");
			}
			else
			{
				strcpy(node.widget_normal_thumb[widgetSlot], "editors\\templates\\panels\\image-panel.png");
			}
		}
		else
		{
			strcpy(node.widget_normal_thumb[widgetSlot], "editors\\templates\\panels\\image-panel.png");
		}
		// used as GRID ROW and COLUMN default (1x1)
		Storyboard.widget_textoffset[nodeID][widgetSlot] = ImVec2(1, 1);
	}
	else if (type == STORYBOARD_WIDGET_VIDEO)
	{
		strcpy(node.widget_normal_thumb[widgetSlot], "editors\\templates\\panels\\video-panel.png");
		strcpy(node.widget_highlight_thumb[widgetSlot], ""); // will ne used to store MP4 file
	}
	else if (type == STORYBOARD_WIDGET_TEXT || type == STORYBOARD_WIDGET_TEXTAREA)
	{
		if (readoutTitle.length() > 0)
		{
			char valueStr[64];
			sprintf(valueStr, "%d", node.widget_initial_value[widgetSlot]);
			strcpy(node.widget_label[widgetSlot], valueStr);// "999");
			node.widget_font_size[widgetSlot] = 0.8f;
		}
	}
	else if (type == STORYBOARD_WIDGET_BUTTON)
	{
		strcpy(node.widget_normal_thumb[widgetSlot], "editors\\templates\\buttons\\default.png");
		strcpy(node.widget_highlight_thumb[widgetSlot], "editors\\templates\\buttons\\default-hover.png");
	}
	else if (type == STORYBOARD_WIDGET_RADIOTYPE)
	{
		strcpy(node.widget_normal_thumb[widgetSlot], "editors\\templates\\buttons\\default.png");
		strcpy(node.widget_highlight_thumb[widgetSlot], "editors\\templates\\buttons\\default-hover.png");
		strcpy(node.widget_selected_thumb[widgetSlot], "editors\\templates\\buttons\\default-selected.png");
	}
	else if (type == STORYBOARD_WIDGET_TICKBOX)
	{
		strcpy(node.widget_normal_thumb[widgetSlot], "editors\\templates\\buttons\\tick-box-off.png");
		strcpy(node.widget_highlight_thumb[widgetSlot], "editors\\templates\\buttons\\tick-box-off.png");		
		strcpy(node.widget_selected_thumb[widgetSlot], "editors\\templates\\buttons\\tick-box-on.png");
		Storyboard.widget_textoffset[nodeID][widgetSlot] = ImVec2(-60, 0);
		if (readoutTitle.length() > 0)
		{
			strcpy(node.widget_label[widgetSlot], readoutTitle.c_str());
		}
	}
	else if (type == STORYBOARD_WIDGET_SLIDER)
	{
		strcpy(node.widget_normal_thumb[widgetSlot], "editors\\templates\\buttons\\slider-bar-empty.png");
		strcpy(node.widget_highlight_thumb[widgetSlot], "editors\\templates\\buttons\\slider-bar-full.png");
		Storyboard.widget_textoffset[nodeID][widgetSlot] = ImVec2(0, -50);
		if (readoutTitle.length() > 0)
		{
			strcpy(node.widget_label[widgetSlot], readoutTitle.c_str());
		}
	}
	else if (type == STORYBOARD_WIDGET_PROGRESS)
	{
		strcpy(node.widget_normal_thumb[widgetSlot], "editors\\templates\\buttons\\slider-bar-empty.png");
		strcpy(node.widget_highlight_thumb[widgetSlot], "editors\\templates\\buttons\\slider-bar-full.png");
	}
	else if (type == STORYBOARD_WIDGET_BAR)
	{
		strcpy(node.widget_normal_thumb[widgetSlot], "editors\\templates\\bars\\bar-empty.png");
		strcpy(node.widget_highlight_thumb[widgetSlot], "editors\\templates\\bars\\bar-full.png");
	}

	iUpdateWidgetThumbNode = widgetSlot;
	iUpdateWidgetThumbButton = widgetSlot;
	iCurrentSelectedWidget = widgetSlot;
	Storyboard_ActiveWidgets.push_back(widgetSlot);
	bPlacingNewWidget = true;
	return widgetSlot;
}

void RefreshThumbImagesForNode(int nodeID, int indexToActiveWidget)
{
	StoryboardNodesStruct& node = Storyboard.Nodes[nodeID];
	image_setlegacyimageloading(true);
	for (int i = indexToActiveWidget; i < STORYBOARD_MAXWIDGETS; i++)
	{
		DeleteImage(node.widget_normal_thumb_id[i]);
		if (strlen(node.widget_normal_thumb[i]) > 0)
		{
			LoadImage(node.widget_normal_thumb[i], node.widget_normal_thumb_id[i]);
		}
		DeleteImage(node.widget_highlight_thumb_id[i]);
		if (strlen(node.widget_highlight_thumb[i]) > 0)
		{
			LoadImage(node.widget_highlight_thumb[i], node.widget_highlight_thumb_id[i]);
		}
		DeleteImage(node.widget_selected_thumb_id[i]);
		if (strlen(node.widget_selected_thumb[i]) > 0)
		{
			LoadImage(node.widget_selected_thumb[i], node.widget_selected_thumb_id[i]);
		}
	}
	image_setlegacyimageloading(false);
}

void RemoveWidgetFromScreen(int nodeID, int widgetID)
{
	bool bHardcodedID = IsHardcodedID(nodeID, widgetID);
	if (Storyboard.Nodes[nodeID].type != STORYBOARD_TYPE_HUD)
	{
		if ( bHardcodedID==true )
		{
			bool bHardWidgetsThatCanBeDeleted = false;
			if (nodeID == iTitleScreenNodeID && (widgetID == 1 || widgetID == 2)) bHardWidgetsThatCanBeDeleted = true;
			if (nodeID == iAboutScreenNodeID && (widgetID != 2)) bHardWidgetsThatCanBeDeleted = true;
			if (nodeID == iGamePausedNodeID && (widgetID == 2 || widgetID == 3)) bHardWidgetsThatCanBeDeleted = true;
			if (bHardWidgetsThatCanBeDeleted==true)
			{
				// allow user to delete certain hard buttons (LOAD and SAVE and ABOUT etc)
			}
			else
			{
				return;
			}
		}

		//Also not all pages got add sliders.
		if(Storyboard.Nodes[nodeID].widget_type[widgetID] == STORYBOARD_WIDGET_SLIDER) return;

		//Progress bar cant be added yet.
		if (Storyboard.Nodes[nodeID].widget_type[widgetID] == STORYBOARD_WIDGET_PROGRESS) return;

		//PE: Loading screens must have a progress bar.
		if (pestrcasestr(Storyboard.Nodes[nodeID].lua_name, "loading"))
		{
			if (Storyboard.Nodes[nodeID].widget_type[widgetID] == STORYBOARD_WIDGET_PROGRESS) return;
		}
	}

	// hard coded are just reset
	StoryboardNodesStruct& node = Storyboard.Nodes[nodeID];
	if (bHardcodedID == true)
	{
		// hard widget
		node.widget_used[widgetID] = 0;
	}
	else
	{
		// custom widget
		// First, check that the widget is actually on the screen and able to be removed.
		int indexToActiveWidget = -1;
		for (int i = 0; i < Storyboard_ActiveWidgets.size(); i++)
		{
			if (Storyboard_ActiveWidgets[i] == widgetID)
			{
				indexToActiveWidget = i;
				break;
			}
		}
		if (indexToActiveWidget < 0)
		{
			return;
		}

		// Remove the widget from the active widgets rendered on-screen
		for (int i = indexToActiveWidget; i < Storyboard_ActiveWidgets.size(); i++)
		{
			Storyboard_ActiveWidgets[i] -= 1;
		}
		Storyboard_ActiveWidgets.erase(Storyboard_ActiveWidgets.begin() + indexToActiveWidget);

		// Mark the widget as unused in the storyboard struct and move any widgets after the deleted slot down a slot, to keep the active widgets continguous
		for (int i = indexToActiveWidget; i < STORYBOARD_MAXWIDGETS - 1; i++)
		{
			node.widget_used[i] = node.widget_used[i + 1];
			strcpy(node.widget_label[i], node.widget_label[i + 1]);
			node.widget_size[i] = node.widget_size[i + 1];
			node.widget_pos[i] = node.widget_pos[i + 1];
			strcpy(node.widget_normal_thumb[i], node.widget_normal_thumb[i + 1]);
			strcpy(node.widget_highlight_thumb[i], node.widget_highlight_thumb[i + 1]);
			strcpy(node.widget_selected_thumb[i], node.widget_selected_thumb[i + 1]);
			strcpy(node.widget_click_sound[i], node.widget_click_sound[i + 1]);
			node.widget_action[i] = node.widget_action[i + 1];
			strcpy(node.widget_font[i], node.widget_font[i + 1]);
			node.widget_font_color[i] = node.widget_font_color[i + 1];
			node.widget_font_size[i] = node.widget_font_size[i + 1];
			node.widget_type[i] = node.widget_type[i + 1];
			node.widget_read_only[i] = node.widget_read_only[i + 1];
			node.widget_layer[i] = node.widget_layer[i + 1];
			node.widget_initial_value[i] = node.widget_initial_value[i + 1];
			strcpy(node.widget_name[i], node.widget_name[i + 1]);
			Storyboard.widget_colors[nodeID][i] = Storyboard.widget_colors[nodeID][i + 1];
			strcpy(Storyboard.widget_readout[nodeID][i], Storyboard.widget_readout[nodeID][i + 1]);
			Storyboard.widget_textoffset[nodeID][i] = Storyboard.widget_textoffset[nodeID][i + 1];
			Storyboard.widget_ingamehidden[nodeID][i] = Storyboard.widget_ingamehidden[nodeID][i + 1];
			Storyboard.widget_drawordergroup[nodeID][i] = Storyboard.widget_drawordergroup[nodeID][i + 1];
		}

		// Reload any images after the deleted widget, to ensure they remain in the correct slot
		RefreshThumbImagesForNode(nodeID, indexToActiveWidget);
	}
	
	iCurrentSelectedWidget = -1;
}

// TODO: this is very unsafe, would be better to have the desired type as a parameter to ensure we only return expected types
void* GetReadoutAddress(char* readoutTitle)
{
	if (strcmp(readoutTitle, "User Defined Global") == 0)
	{
		return nullptr;
	}
	if (strcmp(readoutTitle, "User Defined Global Text") == 0)
	{
		return nullptr;
	}
	else if (strcmp(readoutTitle, "User Defined Global Statusbar") == 0)
	{
		return nullptr;
	}
	else if (strcmp(readoutTitle, "User Defined Global Imagw") == 0)
	{
		return (void*)&t.iTmpImgID;
	}
	else if (strcmp(readoutTitle, "User Defined Global Panel") == 0)
	{
		return (void*)&t.iTmpImgID;
	}
	else if (strcmp(readoutTitle, "Health Remaining") == 0)
	{
		return (void*)&t.player[t.plrid].health;
	}
	else if(strcmp(readoutTitle, "Maximum Health") == 0)
	{
		return (void*)&t.playercontrol.startstrength;
	}
	else if (strcmp(readoutTitle, "Health Panel") == 0)
	{
		return (void*)&t.iTmpImgID;
	}
	else if (strcmp(readoutTitle, "Ammo Remaining") == 0 && t.gunid > 0 && t.gun[t.gunid].weapontype != 51)
	{
		return (void*)&t.slidersmenuvalue[1][1].value;
	}
	else if (strcmp(readoutTitle, "Maximum Ammo") == 0 && t.gunid > 0 && t.gun[t.gunid].weapontype != 51)
	{
		return (void*)&t.slidersmenuvalue[1][2].value;
	}
	else if (strcmp(readoutTitle, "Ammo Panel") == 0 && t.gunid > 0 )// && t.gunid > 0 && t.gun[t.gunid].weapontype != 51) may want to see weapon panel for melee
	{
		return (void*)&t.iTmpImgID;
	}
	else if (strcmp(readoutTitle, "Weapon Held") == 0 && t.gunid > 0 ) // may want to see weapon symbol panel for melee && t.gun[t.gunid].weapontype != 51)
	{
		return (void*)&g.firemodes[t.gunid][g.firemode].iconimg;
	}
	else if (strcmp(readoutTitle, "Weapon Firemode") == 0 && t.gun[t.gunid].weapontype != 51)
	{
		return (void*)&g.firemodes[t.gunid][g.firemode].ammoimg;
	}
	else if (strcmp(readoutTitle, "Lives Remaining") == 0)
	{
		return (void*)&t.player[t.plrid].lives;
	}
	else if (strcmp(readoutTitle, "VSync") == 0)
	{
		return (void*)&master.bVsyncEnabled;
	}
	else if (strcmp(readoutTitle, "Music Volume") == 0)
	{
		return (void*)&t.gamesounds.music;
	}
	else if (strcmp(readoutTitle, "Sound Effects Volume") == 0)
	{
		return (void*)&t.gamesounds.sounds;
	}
	
	return nullptr;
}

// Find the index of the readout with the passed in title
int GetReadoutIndex(char* readoutTitle)
{
	for (int i = 0; i < readoutTitles.size(); i++)
	{
		if (strcmp(readoutTitle, readoutTitles[i].c_str()) == 0)
		{
			return i;
		}
	}

	return 0;
}

void ExecuteReadoutCallback(int indexToCallback)
{
	// Obtain function callback pointer and call
	auto& fn = readoutCallbacks[indexToCallback];
	if (fn != nullptr)
	{
		fn();
	}
}

int GetReadoutValueInt(char* readoutTitle)
{
	void* pReadout = GetReadoutAddress(readoutTitle);
	if (pReadout)
	{
		int readoutIndex = GetReadoutIndex(readoutTitle);
		ReadoutTypes type = readoutTypes[readoutIndex];
		int value = 0;
		if (type == READOUT_BOOL)
		{
			// 1 byte
			value = *(bool*)pReadout;
		}
		else
		{
			// 4 bytes
			value = *(int*)pReadout;
		}
		return value;
	}
	else
	{
		// For now, a special value to indicate failure getting readout
		return -INT_MAX;
	}
}

// Set the variable that the readout represents (also executes callback, if one exists)
void SetReadoutValueInt(char* readoutTitle, int value)
{
	void* pReadout = GetReadoutAddress(readoutTitle);
	if (pReadout)
	{
		int readoutIndex = GetReadoutIndex(readoutTitle);
		ReadoutTypes type = readoutTypes[readoutIndex];
		if (type == READOUT_BOOL)
		{
			// 1 byte
			*(bool*)pReadout = value;
		}
		else
		{
			// 4 bytes
			*(int*)pReadout = value;
		}
		ExecuteReadoutCallback(readoutIndex);
	}
}

// Swap the contents of two widgets (must be part of same node)
void SwapWidgets(int nodeID, int widgetA, int widgetB)
{
	StoryboardNodesStruct& node = Storyboard.Nodes[nodeID];
	std::swap(node.widget_used[widgetA], node.widget_used[widgetB]);
	std::swap(node.widget_label[widgetA], node.widget_label[widgetB]);
	std::swap(node.widget_size[widgetA], node.widget_size[widgetB]);
	std::swap(node.widget_pos[widgetA], node.widget_pos[widgetB]);
	std::swap(node.widget_normal_thumb[widgetA], node.widget_normal_thumb[widgetB]);
	std::swap(node.widget_normal_thumb_id[widgetA], node.widget_normal_thumb_id[widgetB]);
	std::swap(node.widget_highlight_thumb[widgetA], node.widget_highlight_thumb[widgetB]);
	std::swap(node.widget_highlight_thumb_id[widgetA], node.widget_highlight_thumb_id[widgetB]);
	std::swap(node.widget_selected_thumb[widgetA], node.widget_selected_thumb[widgetB]);
	std::swap(node.widget_selected_thumb_id[widgetA], node.widget_selected_thumb_id[widgetB]);
	std::swap(node.widget_click_sound[widgetA], node.widget_click_sound[widgetB]);
	std::swap(node.widget_action[widgetA], node.widget_action[widgetB]);
	std::swap(node.widget_font[widgetA], node.widget_font[widgetB]);
	std::swap(node.widget_font_color[widgetA], node.widget_font_color[widgetB]);
	std::swap(node.widget_font_size[widgetA], node.widget_font_size[widgetB]);
	std::swap(node.widget_type[widgetA], node.widget_type[widgetB]);
	std::swap(node.widget_read_only[widgetA], node.widget_read_only[widgetB]);
	std::swap(node.widget_layer[widgetA], node.widget_layer[widgetB]);
	std::swap(node.widget_initial_value[widgetA], node.widget_initial_value[widgetB]);	
	std::swap(node.widget_name[widgetA], node.widget_name[widgetB]);
	std::swap(Storyboard.widget_colors[nodeID][widgetA], Storyboard.widget_colors[nodeID][widgetB]);
	std::swap(Storyboard.widget_readout[nodeID][widgetA], Storyboard.widget_readout[nodeID][widgetB]);
	std::swap(Storyboard.widget_textoffset[nodeID][widgetA], Storyboard.widget_textoffset[nodeID][widgetB]);
	std::swap(Storyboard.widget_ingamehidden[nodeID][widgetA], Storyboard.widget_ingamehidden[nodeID][widgetB]);
	std::swap(Storyboard.widget_drawordergroup[nodeID][widgetA], Storyboard.widget_drawordergroup[nodeID][widgetB]);
}

void SendWidgetToFront(int nodeID, int widgetID)
{
	StoryboardNodesStruct& node = Storyboard.Nodes[nodeID];

	// Find the last widget in the array (the one that is drawn last so is at the 'front' of the screen)
	int lastIndex = -1;
	for (int i = STORYBOARD_MAXWIDGETS - 1; i >= 0; i--)
	{
		if (node.widget_used[i])
		{
			lastIndex = i;
			break;
		}
	}
	if (lastIndex < 1)
		return;

	// normal order group (draws in ID order)
	Storyboard.widget_drawordergroup[nodeID][widgetID]++;
	if (Storyboard.widget_drawordergroup[nodeID][widgetID] > 1) Storyboard.widget_drawordergroup[nodeID][widgetID] = 1;
	bool bHardcodedID = IsHardcodedID(nodeID, widgetID);
	if (bHardcodedID == true)
		return;

	// Keep swapping widgetID with its neighbour until it reaches the end of the array (the front of the screen)
	int iSwap = widgetID;
	while (iSwap < lastIndex)
	{
		bool bHardcodedID1 = IsHardcodedID(nodeID, iSwap);
		if (bHardcodedID1 == false)
		{
			bool bHardcodedID2 = IsHardcodedID(nodeID, iSwap + 1);
			if (bHardcodedID2 == false)
			{
				SwapWidgets(nodeID, iSwap, iSwap + 1);
				iCurrentSelectedWidget = iSwap + 1;
			}
		}
		iSwap++;
	}
}

void SendWidgetToBack(int nodeID, int widgetID)
{
	StoryboardNodesStruct& node = Storyboard.Nodes[nodeID];

	// draw early sort order group (and then draws in ID order)
	Storyboard.widget_drawordergroup[nodeID][widgetID]--;
	if (Storyboard.widget_drawordergroup[nodeID][widgetID] < -1) Storyboard.widget_drawordergroup[nodeID][widgetID] = -1;
	bool bHardcodedID = IsHardcodedID(nodeID, widgetID);
	if (bHardcodedID == true)
		return;

	// Keep swapping widgetID with its neighbour until it reaches the front of the array (the back of the screen)
	int iSwap = widgetID;
	while (iSwap > 0)//bRestrictShuffleWidgetsToBeyond)
	{
		bool bHardcodedID1 = IsHardcodedID(nodeID, iSwap);
		if (bHardcodedID1 == false)
		{
			bool bHardcodedID2 = IsHardcodedID(nodeID, iSwap - 1);
			if (bHardcodedID2 == false)
			{
				SwapWidgets(nodeID, iSwap, iSwap - 1);
				iCurrentSelectedWidget = iSwap - 1;
			}
		}
		iSwap--;
	}
}

unsigned int GetScancodeName(unsigned int scancode, char* buffer, unsigned int bufferLength) 
{

	// bit 16 - 23 contains the first byte of the scancode
	// bit 24 indicates that the scancode is 2 bytes(extended)
	unsigned int result = 0;
	unsigned int extended = scancode & 0xffff00;
	unsigned int lParam = 0;

	if (extended) 
	{
		if (extended == 0xE11D00) 
		{
			lParam = 0x45 << 16;
		}
		else 
		{
			lParam = (0x100 | (scancode & 0xff)) << 16;
		}
	}
	else 
	{
		lParam = scancode << 16;
		if (scancode == 0x45) 
		{
			lParam |= (0x1 << 24);
		}
	}

	result = GetKeyNameTextA(lParam, buffer, bufferLength);
	return result;
}

// Check all of the storyboard nodes for their key to toggle them. If it matches the keyboard input then make the screen appear
static std::vector<int> listenForKeys;
void TriggerScreenFromKeyPress()
{
	if (g.tabmode == 0 )
	{
		static std::vector<int> scans;
		if (listenForKeys.empty())
		{
			// Initialise listenForKeys - we only want to check for keys that will toggle a screen, if we let e.g. player movement keys through, then it will trigger the wait to release it before allowing us to toggle a screen.
			// This will allow us to trigger screens whilst moving  
			for (int i = 0; i < STORYBOARD_MAXNODES; i++)
			{
				int keyToListenFor = Storyboard.Nodes[i].toggleKey;
				if (keyToListenFor > 0)
				{
					listenForKeys.push_back(keyToListenFor);
				}
			}
		}

		// Early exit: this project has no screens that are toggled with keypresses.
		if (listenForKeys.empty())
		{
			return;
		}

		// Need to check all keys, in case we need to ignore any in m_KeyBuffer, that come before our toggle keys
		scans.clear();
		UpdateKeyboard();
		extern unsigned char m_KeyBuffer[256];
		for (int i = 0; i < 256; i++)
		{
			if (m_KeyBuffer[i] > 0)
			{
				scans.push_back(i);
			}
		}

		static bool bWaitForKeyRelease = false;
		if (scans.empty())
		{
			// No keys pressed
			bWaitForKeyRelease = false;
			return;
		}

		int scan = 0;
		// Check if the keys that are pressed are used for toggling screens
		for (int i = 0; i < scans.size(); i++)
		{
			if (std::find(listenForKeys.begin(), listenForKeys.end(), scans[i]) != listenForKeys.end())
			{
				scan = scans[i];
				break;
			}
			if (i == scans.size() - 1)
			{
				// No keys are pressed that are being 'listened' to
				bWaitForKeyRelease = false;
				return;
			}
		}

		if (bWaitForKeyRelease == false)
		{
			// If we reached here, then a key is pressed that should toggle a screen, find the screen to toggle.
			for (int i = 0; i < STORYBOARD_MAXNODES; i++)
			{
				StoryboardNodesStruct& node = Storyboard.Nodes[i];
				if (node.used && scan > 0 && node.toggleKey == scan && strlen(node.level_name) == 0) // only HUDs
				{
					if (node.type == STORYBOARD_TYPE_HUD)
					{
						// Only HUD types (not loading screens, etc)
						bWaitForKeyRelease = true;
						if (t.game.activeStoryboardScreen == i)
						{
							// Screen is already active, turn it off
							t.game.activeStoryboardScreen = -1;
						}
						else
						{
							t.game.activeStoryboardScreen = i;
						}
						return;
					}
				}
			}
		}
	}
}
void ResetStoryboardListenForKeys()
{
	listenForKeys.clear();
}

float LuaMousePosPercentX, LuaMousePosX, LuaMousePosPercentY, LuaMousePosY;
int LuaMouseClick = 0;
static char LoadGameTitle[9][256];
char cCopyToAllScreens[MAX_PATH];

void screen_editor_setscalemod (float fGlobalScaleMod)
{
	g.globalhudscale = fGlobalScaleMod;
}
float screen_editor_scalemod (float fGlobalScaleIn)
{
	return fGlobalScaleIn * g.globalhudscale;
}

// GGMAX 3.35i: debug output goes beside the EXE, not into whatever the CWD is now
// (a file dialog can move it). Defined in Guru-WickedMAX/master_part1.cpp.
extern FILE* GGDiagFopen(const char* name, const char* mode);

// ============================================================================================
// GGMAX 2.36: harness hooks for the SCREEN (HUD) editor.
//
// These live here because the screen editor's load-trigger statics (iUpdateWidgetThumbNode /
// iUpdateWidgetThumbButton, declared static at the top of this file) are not visible anywhere
// else, and setting the image path WITHOUT tripping them is not what the UI does — it would
// test a different code path from the one a user exercises.
// ============================================================================================

// Enter the screen editor on the node with this title (e.g. "In-Game HUD"). Returns nodeid, or -1.
int GGHudEditScreen(const char* title)
{
	extern bool bScreen_Editor_Window;
	extern int iScreen_Editor_Node;
	if (title == NULL || title[0] == 0) return -1;
	for (int i = 0; i < STORYBOARD_MAXNODES; i++)
	{
		if (Storyboard.Nodes[i].used != true) continue;
		if (_stricmp(Storyboard.Nodes[i].title, title) != 0) continue;
		bScreen_Editor_Window = true;
		iScreen_Editor_Node = i;
		return i;
	}
	return -1;
}

// Add an image widget to the screen currently being edited, centre it, and point it at `path`.
// Mirrors the UI exactly: AddWidgetToScreen (the "Add an image" button) then the path assignment
// plus the same reload trigger the Select Image control uses. Returns the widget slot, or -1.
int GGHudAddImage(const char* path, char* result, int resultSize)
{
	extern int iScreen_Editor_Node;
	const int nodeid = iScreen_Editor_Node;
	if (nodeid < 0 || nodeid >= STORYBOARD_MAXNODES || Storyboard.Nodes[nodeid].used != true)
	{
		_snprintf(result, resultSize, "ERROR: not in the screen editor (call HUD_EDIT first)");
		return -1;
	}
	const int slot = AddWidgetToScreen(nodeid, STORYBOARD_WIDGET_IMAGE);
	if (slot < 0)
	{
		_snprintf(result, resultSize, "ERROR: no free widget slot on node %d", nodeid);
		return -1;
	}
	// Centre of the screen editor canvas: widget_pos is a PERCENTAGE (AddWidgetToScreen seeds it
	// as 200/viewport*100), so 50,50 is the middle regardless of emulated resolution.
	Storyboard.Nodes[nodeid].widget_pos[slot].x = 50.0f;
	Storyboard.Nodes[nodeid].widget_pos[slot].y = 50.0f;
	if (path != NULL && path[0] != 0)
	{
		strcpy(Storyboard.Nodes[nodeid].widget_normal_thumb[slot], path);
		// The same pair the Select Image control trips, so the load takes the user's path.
		iUpdateWidgetThumbNode = nodeid;
		iUpdateWidgetThumbButton = slot;
	}
	_snprintf(result, resultSize, "OK: HUD_ADD_IMAGE node=%d slot=%d pos=(50,50) path=\"%s\"",
		nodeid, slot, path ? path : "");
	return slot;
}

// Report every widget on the edited screen, and — the point of the exercise — whether the image
// each one names is ACTUALLY LOADED. `ImageExist(thumb_id)` is precisely what the editor's own
// draw path tests before it can blit anything (M-GridEditB_part22.cpp:938), so exist=0 means the
// yellow selection box is empty on screen, with no need to eyeball a screenshot.
void GGHudDumpWidgets(char* result, int resultSize)
{
	extern int iScreen_Editor_Node;
	const int nodeid = iScreen_Editor_Node;
	if (nodeid < 0 || nodeid >= STORYBOARD_MAXNODES || Storyboard.Nodes[nodeid].used != true)
	{
		_snprintf(result, resultSize, "ERROR: not in the screen editor (call HUD_EDIT first)");
		return;
	}
	FILE* f = GGDiagFopen("hudwidgets.txt", "w");
	int used = 0, withPath = 0, loaded = 0;
	if (f != NULL)
		fprintf(f, "screen node %d \"%s\"\n%-4s %-6s %-8s %-9s %-6s %-7s %s\n",
			nodeid, Storyboard.Nodes[nodeid].title,
			"slot", "type", "pos", "thumb_id", "exist", "size", "path");
	for (int i = 0; i < STORYBOARD_MAXWIDGETS; i++)
	{
		if (Storyboard.Nodes[nodeid].widget_used[i] != 1) continue;
		used++;
		const char* p = Storyboard.Nodes[nodeid].widget_normal_thumb[i];
		const int id = Storyboard.Nodes[nodeid].widget_normal_thumb_id[i];
		const int exist = (p[0] != 0) ? (ImageExist(id) ? 1 : 0) : -1;
		if (p[0] != 0) withPath++;
		if (exist == 1) loaded++;
		if (f != NULL)
			fprintf(f, "%-4d %-6d (%3.0f,%3.0f) %-9d %-6s %dx%d  %s\n",
				i, (int)Storyboard.Nodes[nodeid].widget_type[i],
				Storyboard.Nodes[nodeid].widget_pos[i].x, Storyboard.Nodes[nodeid].widget_pos[i].y,
				id, (exist < 0) ? "-" : (exist ? "YES" : "NO"),
				(exist == 1) ? ImageWidth(id) : 0, (exist == 1) ? ImageHeight(id) : 0, p);
	}
	if (f != NULL) fclose(f);
	_snprintf(result, resultSize,
		"OK: HUD_DUMP node=%d \"%s\" widgets=%d withPath=%d imagesLOADED=%d -> Files/hudwidgets.txt%s",
		nodeid, Storyboard.Nodes[nodeid].title, used, withPath, loaded,
		(withPath > 0 && loaded == 0) ? "  *** every named image FAILED to load — nothing can draw ***" : "");
	result[resultSize - 1] = 0;
}
