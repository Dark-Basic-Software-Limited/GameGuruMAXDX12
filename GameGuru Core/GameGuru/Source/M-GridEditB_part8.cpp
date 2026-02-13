void process_entity_library(void)
{
	if (bExternal_Entities_Window)
	{
		ImGuiWindowFlags ex_window_flags = 0;
		ImGuiIO& io = ImGui::GetIO();

		static int uniqueId = 4000; //PE: Also used for imageID for previews.
		static int loaded_images = 0;
		static bool multi_selections = false;
		int multi_selections_count = 0;
		static int lf_multi_selections_count = 0;
		int olduniqueId = uniqueId;
		bool bReleaseIconsDynamic = false;
		uniqueId = 4000;
#ifdef DYNAMICLOADUNLOAD
		static int max_load_persync = 200; //First time only , changed later to 15
		bReleaseIconsDynamic = true;
#else
		int max_load_persync = 2000;
#endif

		int preview_count = 0;
		int media_icon_size = 64;
		int iColumnsWidth = 110;

		time_t tCurrentTimeSec;
		time(&tCurrentTimeSec);

		ImGui::Begin("Entity Library##ExternalWindow", &bExternal_Entities_Window, ex_window_flags);

		bool bAddSelectionToGame = false;
		bool bIsWeDocked = ImGui::IsWindowDocked();
		static int current_tab = -1;


		CheckTutorialAction("TABMARKERS", 54.0f); //Tutorial: check if we are waiting for this action
		if (current_tab == 1 && bTutorialCheckAction)
			TutorialNextAction(); //Clicked - selected the tab markers.
		CheckTutorialAction("TABENTITIES", -10.0f); //Tutorial: check if we are waiting for this action
		if (current_tab == 0 && bTutorialCheckAction)
			TutorialNextAction(); //Clicked - selected the tab Entities.

		ImGui::SetItemAllowOverlap();
		if (ImGui::BeginTabBar("entlibtabbar"))
		{
			static int iCurrentFilter = 0;
			static char cSearchAllEntities[3][MAX_PATH] = { "\0","\0","\0" };

			for (int i = 0; i < 2; i++) {

				cStr sTabHeader;
				if (i == 0) sTabHeader = " Entities ";
				if (i == 1) sTabHeader = " Markers! ";


				if (ImGui::BeginTabItem(sTabHeader.Get()))
				{
					if (current_tab != i) {
						//Tab changed.
						current_tab = i;
						iCurrentFilter = 0;
					}

					static char cAllFilters[10][MAX_PATH];
					char cFilter[MAX_PATH], cHeader[MAX_PATH];
					int splitsections = 1;

					if (i == 1) {
						splitsections = 5;
					}

					int control_wrap_width = 70;
					strcpy(cFilter, "");
					strcpy(cHeader, "");
					//PE: Debug dynamic icon load unload.
					ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3.0));
					ImGui::Text("Filter: ");
					ImGui::SameLine();
					ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() - 3.0));
					if (ImGui::GetCursorPosX() + control_wrap_width > ImGui::GetWindowSize().x)
						ImGui::Text(""); //NewLine
					bool rb_change = false;
					ImGui::RadioButton("All ", &iCurrentFilter, 0);
					ImGui::SameLine();
					if (ImGui::GetCursorPosX() + control_wrap_width > ImGui::GetWindowSize().x)
						ImGui::Text(""); //NewLine

					if (i == 0) {

						CheckTutorialAction("BUTCHARACTER", -28.0f); //Tutorial: check if we are waiting for this action
						if (ImGui::RadioButton("Characters ", &iCurrentFilter, 1)) {
							//On click remove search.
							strcpy(cSearchAllEntities[i], "");
							if (bTutorialCheckAction)
								TutorialNextAction(); //Clicked - on "Characters"
						}
						ImGui::SameLine();
						if (ImGui::GetCursorPosX() + control_wrap_width > ImGui::GetWindowSize().x)
							ImGui::Text(""); //NewLine

						CheckTutorialAction("BUTBUILDINGS", -28.0f); //Tutorial: check if we are waiting for this action
						if (ImGui::RadioButton("Buildings", &iCurrentFilter, 2)) {
							//On click remove search.
							strcpy(cSearchAllEntities[i], "");
							if (bTutorialCheckAction)
								TutorialNextAction(); //Clicked - on "Buildings"
						}
						ImGui::SameLine();

						if (ImGui::GetCursorPosX() + control_wrap_width > ImGui::GetWindowSize().x)
							ImGui::Text(""); //NewLine


						CheckTutorialAction("BUTOBJECTS", -28.0f); //Tutorial: check if we are waiting for this action
						if (ImGui::RadioButton("Objects", &iCurrentFilter, 6)) {
							//On click remove search.
							strcpy(cSearchAllEntities[i], "");
							if (bTutorialCheckAction)
								TutorialNextAction(); //Clicked - on "objects"
						}

						if (iCurrentFilter == 1) strcpy(cFilter, "Character");
						if (iCurrentFilter == 2) strcpy(cFilter, "Building");
						if (iCurrentFilter == 6) strcpy(cFilter, "*");

						strcpy(cAllFilters[0], "Character");
						strcpy(cAllFilters[1], "Building");

					}
					if (i == 1) {
						ImGui::RadioButton("Players ", &iCurrentFilter, 1);
						ImGui::SameLine();
						if (ImGui::GetCursorPosX() + control_wrap_width > ImGui::GetWindowSize().x)
							ImGui::Text(""); //NewLine
						ImGui::RadioButton("Zones", &iCurrentFilter, 2);
						ImGui::SameLine();
						if (ImGui::GetCursorPosX() + control_wrap_width > ImGui::GetWindowSize().x)
							ImGui::Text(""); //NewLine
						ImGui::RadioButton("Lights", &iCurrentFilter, 3);
						ImGui::SameLine();
						if (ImGui::GetCursorPosX() + control_wrap_width > ImGui::GetWindowSize().x)
							ImGui::Text(""); //NewLine
						ImGui::RadioButton("Spot Lights", &iCurrentFilter, 4);


						if (iCurrentFilter == 1) { strcpy(cFilter, "player"); strcpy(cHeader, "Player Positions"); }
						if (iCurrentFilter == 2) { strcpy(cFilter, "zone.fpe"); strcpy(cHeader, "Zones"); }
						if (iCurrentFilter == 3) { strcpy(cFilter, " light.fpe"); strcpy(cHeader, "Lights"); }
						if (iCurrentFilter == 4) { strcpy(cFilter, "spot.fpe"); strcpy(cHeader, "Spot Lights"); }
						if (iCurrentFilter > 0) splitsections = 1;
					}

					ImGui::SameLine();

					if (ImGui::GetCursorPosX() + control_wrap_width > ImGui::GetWindowSize().x)
						ImGui::Text(""); //NewLine

					float fXWidth = ImGui::GetFontSize()*1.5;
					if (i == 0)
						ImGui::PushItemWidth(-38 - fXWidth);
					else
						ImGui::PushItemWidth(-1 - fXWidth - 4.0);

					ImGui::Text(" Search: ");
					ImGui::SameLine();

					bEntityGotFocus = false;
					if (ImGui::InputText("##cSearchAllEntities", &cSearchAllEntities[i][0], MAX_PATH, ImGuiInputTextFlags_EnterReturnsTrue))
					{
						if (strlen(cSearchAllEntities[i]) > 1) {
							bool already_there = false;
							for (int l = 0; l < MAXSEARCHHISTORY; l++) {
								if (strcmp(cSearchAllEntities[i], pref.search_history[l]) == 0) {
									already_there = true;
									break;
								}
							}
							if (!already_there) {
								bool foundspot = false;
								for (int l = 0; l < MAXSEARCHHISTORY; l++) {
									if (strlen(pref.search_history[l]) <= 0) {
										strcpy(pref.search_history[l], cSearchAllEntities[i]);
										foundspot = true;
										break;
									}
								}
								if (!foundspot) {
									//Move entry list.
									for (int l = 0; l < MAXSEARCHHISTORY; l++) {
										strcpy(pref.search_history[l], pref.search_history[l + 1]);
									}
									strcpy(pref.search_history[MAXSEARCHHISTORY - 1], cSearchAllEntities[i]);
								}
							}
						}
					}
					ImGui::SameLine();
					if (ImGui::StyleButton("X##deletesearch"))
					{
						strcpy(cSearchAllEntities[i], "");
					}
					//}
					ImGui::PopItemWidth();


					//Combo dropdown. Use folder names as seach.
					if (i == 0) {
						ImGui::SameLine();
						static char * current_combo_entry = "\0";
						int comboflags = ImGuiComboFlags_NoPreview | ImGuiComboFlags_PopupAlignLeft | ImGuiComboFlags_HeightLarge;
						ImGui::PushItemWidth(-24);
						if (ImGui::BeginCombo("##combolastsearch", current_combo_entry, comboflags))
						{

							//Do we have a search history.
							bool display_history = false;
							for (int l = 0; l < MAXSEARCHHISTORY; l++) {
								if (strlen(pref.search_history[l]) > 0) {
									display_history = true;
									break;
								}
							}
							if (display_history) {
								ImGui::Text("Search History:");
								ImGui::Indent(10);
								for (int l = 0; l < MAXSEARCHHISTORY; l++) {
									if (strlen(pref.search_history[l]) > 0) {
										bool is_selected = (current_combo_entry == pref.search_history[l]);
										if (ImGui::Selectable(pref.search_history[l], is_selected)) {
											current_combo_entry = (char *)pref.search_history[l];
											strcpy(cSearchAllEntities[i], pref.search_history[l]);
										}
										if (is_selected)
											ImGui::SetItemDefaultFocus();
									}
								}
								ImGui::Indent(-10);
							}
							cFolderItem *pNewFolder = &MainEntityList;
							pNewFolder = pNewFolder->m_pNext;
							if (pNewFolder) {

								ImGui::Text("Folders:");
								ImGui::Indent(10);

								cStr path_remove = pNewFolder->m_sFolderFullPath.Get();
								int ipath_remove_len = path_remove.Len();

								ImVec4* style_colors = ImGui::GetStyle().Colors;
								ImVec2 wsize = ImGui::GetWindowSize();

								int line_count = 0;
								while (pNewFolder)
								{
									cStr path = pNewFolder->m_sFolderFullPath.Get();
									bool bDoubleEntityBank = false;
									char *finde = (char *)pestrcasestr(path.Get(), "\\entitybank"); //Support entitybank inside entitybank.
									if (finde)
									{
										finde += 11;
										finde = (char *)pestrcasestr(finde, "\\entitybank");
										if (finde) bDoubleEntityBank = true;
									}
									if (!bDoubleEntityBank && path.Right(11) == "\\entitybank")
									{
										ipath_remove_len = path.Len();
									}
									else
									{
										char *final_name = path.Get();
										final_name += ipath_remove_len;
										if (*final_name == '\\')
											final_name++;

										std::string dir_name = final_name;
										replaceAll(dir_name, "\\", " - ");

										if (dir_name.length() > 0 && pNewFolder->m_pFirstFile && !pestrcasestr(dir_name.c_str(), "_markers"))
										{
											bool is_selected = (current_combo_entry == pNewFolder->m_sFolderFullPath.Get());
											if (ImGui::Selectable(dir_name.c_str(), is_selected))
											{
												current_combo_entry = (char *)pNewFolder->m_sFolderFullPath.Get();
												//Make a search entry.
												strcpy(cSearchAllEntities[i], dir_name.c_str());

											}
											if (is_selected)
												ImGui::SetItemDefaultFocus();
											line_count++;
										}
									}
									pNewFolder = pNewFolder->m_pNext;
								}
								ImGui::Indent(-10);
							}
							ImGui::EndCombo();
						}

						ImGui::PopItemWidth();
					}
					ImGui::Separator();

					if (strlen(cSearchAllEntities[i]) > 0 && i == 1) {
						iCurrentFilter = 0;
						splitsections = 1;
					}

					ImGui::BeginChild("##cSearchAllEntitiesBegin", ImVec2(0, 0),false, iGenralWindowsFlags);

					ImVec2 oldCursor = ImGui::GetCursorPos();
					std::string insert_text;
					ImVec2 insert_text_width;

					if (lf_multi_selections_count > 0) {
						//Display insert button.
						insert_text = " Add ";
						insert_text += std::to_string(lf_multi_selections_count);
						insert_text += " Objects to Level ";
						insert_text_width = ImGui::CalcTextSize(insert_text.c_str());
						ImGui::SetCursorPos(ImVec2(0, (ImGui::GetWindowSize().y + ImGui::GetScrollY()) - insert_text_width.y - 10.0f));

						ImGui::Spacing();
						ImGui::SameLine(ImGui::GetWindowContentRegionWidth() - (insert_text_width.x + 18.0f));

						ImGui::SetItemAllowOverlap();
						if (ImGui::StyleButton(insert_text.c_str())) {
							//Inset all selected items.
							bAddSelectionToGame = true;
						}
					}

					ImGui::SetCursorPos(oldCursor);
					if (i == 0)
						ImGui::TextCenter("Click to add an object to your level. Control+Click to select multiple items.");

					int iIconVisiblePosY = ImGui::GetWindowSize().y + ImGui::GetScrollY() + media_icon_size;

					bool bFirstShiftHasBeenSeen = false;
					bool bAnySelectedItemsAvailable = false;
					static cFolderItem::sFolderFiles * firstShiftFile = NULL;
					static cFolderItem::sFolderFiles * lastShiftFile = NULL;

					for (int splitloop = 0; splitloop < splitsections; splitloop++) {

						if (iCurrentFilter == 0) {
							if (i == 1) strcpy(cFilter, "");
							if (i == 1) strcpy(cHeader, "");
							if (i == 1 && splitloop == 0) { strcpy(cFilter, "player"); strcpy(cHeader, "Player Positions"); }
							if (i == 1 && splitloop == 1) { strcpy(cFilter, "zone.fpe"); strcpy(cHeader, "Zones"); }
							if (i == 1 && splitloop == 2) { strcpy(cFilter, " light.fpe"); strcpy(cHeader, "Lights"); }
							if (i == 1 && splitloop == 3) { strcpy(cFilter, "spot.fpe"); strcpy(cHeader, "Spot Lights"); }
							if (i == 1 && splitloop == 4) { strcpy(cFilter, "*"); strcpy(cHeader, "Others"); }
							if (i == 1) strcpy(cAllFilters[splitloop], cFilter);
							if (strlen(cSearchAllEntities[i]) > 0 && i == 1) {
								strcpy(cHeader, "");
							}
						}
						cFolderItem *pNewFolder = &MainEntityList;
						pNewFolder = pNewFolder->m_pNext;
						if (pNewFolder)
						{
							//We start at the "entitybank" entry that we use to parce the others.
							cStr path_remove = pNewFolder->m_sFolderFullPath.Get();
							int ipath_remove_len = path_remove.Len();
							pNewFolder = pNewFolder->m_pNext;
							while (pNewFolder)
							{
								//PE: Full path can now change in the middle of the list , so:
								cStr path = pNewFolder->m_sFolderFullPath.Get();
								LPSTR pPathSearch = path.Get();
								LPSTR pFind = "\\entitybank";
								for (int n = 0; n < strlen(pPathSearch); n++)
								{
									if (strnicmp(pPathSearch + n, pFind, strlen(pFind)) == NULL)
									{
										ipath_remove_len = n + strlen(pFind);
										break;
									}
								}

								if (1)
								{
									bool isMarkers = false;
									bool bDisplayEverythingHere = false;
									bool bHideEverythingHere = false;
									if (i == 0 && cFilter[0] == '*')
									{
										if (pestrcasestr(pNewFolder->m_sFolderFullPath.Get(), cAllFilters[0]))
											bHideEverythingHere = true;
										if (pestrcasestr(pNewFolder->m_sFolderFullPath.Get(), cAllFilters[1]))
											bHideEverythingHere = true;
									}
									else if (strlen(cFilter) > 0)
									{
										if (pestrcasestr(pNewFolder->m_sFolderFullPath.Get(), cFilter))
											bDisplayEverythingHere = true;
									}
									if (strlen(cSearchAllEntities[i]) > 0)
									{
										//When search disable fixed tags search.
										bDisplayEverythingHere = false;
										bHideEverythingHere = false;
									}
									char *final_name = path.Get();
									final_name += ipath_remove_len;
									if (*final_name == '\\')
										final_name++;

									std::string path_for_filename = final_name;
									std::string dir_name = final_name;
									replaceAll(dir_name, "\\", " - ");

									if (pestrcasestr(dir_name.c_str(), "_markers"))
										isMarkers = true;

									if (!isMarkers && i == 0 && !bDisplayEverythingHere && !bHideEverythingHere && strlen(cSearchAllEntities[i]) > 0) {
										if (pestrcasestr(pNewFolder->m_sFolderFullPath.Get(), cSearchAllEntities[i]))
											bDisplayEverythingHere = true;
										else if (pestrcasestr(dir_name.c_str(), cSearchAllEntities[i]))
											bDisplayEverythingHere = true;

									}

									//PE: Check here if we need to reload the folder, for new files.
									if (pNewFolder->m_fLastTimeUpdate < MAXTimer())
									{
										pNewFolder->m_fLastTimeUpdate = MAXTimer() + 4000; //Check every 4-6 sec.
										pNewFolder->m_fLastTimeUpdate += rand() % 2000; //Make sure we dont check folders in same cycle.
										struct stat sb;
										if (stat(pNewFolder->m_sFolderFullPath.Get(), &sb) == 0) 
										{
											if (sb.st_mtime != pNewFolder->m_tFolderModify) 
											{
												pNewFolder->m_tFolderModify = sb.st_mtime;
												RefreshEntityFolder(pNewFolder->m_sFolderFullPath.Get(), pNewFolder);
											}
										}
									}
									if (pNewFolder->m_pFirstFile)
									{
										bool bHeaderDisplayed = false;
										bool bDisplayText = true;
										float fWinWidth = ImGui::GetWindowSize().x - 10.0; // Flicker - ImGui::GetCurrentWindow()->ScrollbarSizes.x;
										if (iColumnsWidth >= fWinWidth && fWinWidth > media_icon_size)
										{
											iColumnsWidth = fWinWidth;
										}
										int iColumns = (int)(ImGui::GetWindowSize().x / (iColumnsWidth));
										if (iColumns <= 1)
											iColumns = 1;

										cFolderItem::sFolderFiles * myfiles = pNewFolder->m_pFirstFile->m_pNext;
										while (myfiles)
										{
											std::string sFinal = Left(myfiles->m_sName.Get(), Len(myfiles->m_sName.Get()) - 4);

											if (splitloop == 0 && myfiles->iFlags == 1)
												multi_selections_count++;

											bool bIsVisible = true;
											if (i == 0 && isMarkers) bIsVisible = false;
											if (i == 1 && !isMarkers) bIsVisible = false;

											if (bIsVisible && strlen(cSearchAllEntities[i]) > 0) {
												if (!pestrcasestr(myfiles->m_sName.Get(), cSearchAllEntities[i]))
													bIsVisible = false;
											}
											else if (i == 1 && cFilter[0] == '*') {
												//Others not already displayed.
												for (int fl = 0; fl < splitloop; fl++) {
													if (pestrcasestr(myfiles->m_sName.Get(), cAllFilters[fl]))
														bIsVisible = false;
												}
											}
											else if (strlen(cFilter) > 0 && cFilter[0] != '*') {
												if (!pestrcasestr(myfiles->m_sName.Get(), cFilter))
													bIsVisible = false;
											}


											if (bDisplayEverythingHere)
												bIsVisible = true;
											if (bHideEverythingHere)
												bIsVisible = false;

											ImGui::PushID(uniqueId + preview_count);
											if (splitloop == 0)
												uniqueId++;

											int textureId = 0;
											if (myfiles->iPreview <= 0)
											{
												//Only Visible.
												int gcpy = ImGui::GetCursorPosY();
												if (!bReleaseIconsDynamic || (splitloop == 0 && (bIsVisible || isMarkers) && (gcpy < iIconVisiblePosY && gcpy >= ImGui::GetScrollY() - media_icon_size || isMarkers)))
												{
													myfiles->last_used = (long)tCurrentTimeSec;
													if (max_load_persync-- >= 0)
													{
														//Load preview.
														std::string sImgName = myfiles->m_sPath.Get();
														sImgName = sImgName + "\\" + Left(myfiles->m_sName.Get(), Len(myfiles->m_sName.Get()) - 4);
														sImgName += ".bmp";
														myfiles->iPreview = uniqueId; //TOOL_ENTITY; //Just for testing.
														SetMipmapNum(1); //PE: mipmaps not needed.
														image_setlegacyimageloading(true);
														LoadImage((char *)sImgName.c_str(), myfiles->iPreview);
														image_setlegacyimageloading(false);
														SetMipmapNum(-1);
														if (!GetImageExistEx(myfiles->iPreview))
														{
															myfiles->iPreview = TOOL_ENTITY;
															textureId = TOOL_ENTITY;
														}
														else {
															loaded_images++;
															textureId = myfiles->iPreview;
														}
													}
													else
														textureId = TOOL_ENTITY;
												}
												else {
													textureId = TOOL_ENTITY;
												}
											}
											else
											{
												//PE: Only delete in first run. so we dont delete a image that has already been sent to rendering.
												if (splitloop == 0 && bReleaseIconsDynamic) {
													//Only NOT Visible with a preview image..
													int gcpy = ImGui::GetCursorPosY();
													if (!isMarkers && (!(gcpy < iIconVisiblePosY && gcpy >= ImGui::GetScrollY() - media_icon_size) || !bIsVisible)) {

														if ((long)tCurrentTimeSec - myfiles->last_used > 20) {
															//Delete Image not visible for 20 sec.
															if (GetImageExistEx(myfiles->iPreview) && myfiles->iPreview >= 4000 && myfiles->iPreview < UIV3IMAGES) { //PE: Need to protect system images after tool img range has changed. (myfiles->iPreview can be a system icon)
																image_setlegacyimageloading(true);
																DeleteImage(myfiles->iPreview);
																image_setlegacyimageloading(false);
																myfiles->iPreview = 0;
																loaded_images--;
															}
															textureId = TOOL_ENTITY;
														}
														else
															textureId = myfiles->iPreview;
													}
													else {
														//Still visible update time.
														if (bIsVisible || isMarkers)
															myfiles->last_used = (long)tCurrentTimeSec;
														textureId = myfiles->iPreview;
													}
												}
												else
													textureId = myfiles->iPreview;
											}

											//Is object visible
											if (bIsVisible) 
											{
												if (myfiles->iPreview > 0 && !GetImageExistEx(myfiles->iPreview)) 
												{
													myfiles->iPreview = 0;
													textureId = TOOL_ENTITY;
												}

												if (!bHeaderDisplayed) {

													if (!isMarkers && i == 0)
													{
														ImGui::SetWindowFontScale(1.25);
														LPSTR pFinalHeaderTitle = (LPSTR)dir_name.c_str();
														if (stricmp(pFinalHeaderTitle, "user") == NULL) pFinalHeaderTitle = "Custom Assets";
														if (stricmp(pFinalHeaderTitle, "user - charactercreatorplus") == NULL) pFinalHeaderTitle = "Custom Characters";
														if (stricmp(pFinalHeaderTitle, "user - ebestructures") == NULL) pFinalHeaderTitle = "Custom Structures";
														ImGui::Text("%s", pFinalHeaderTitle);
														ImGui::Spacing();
													}
													else if (strlen(cHeader) > 0)
													{
														ImGui::SetWindowFontScale(1.25);
														ImGui::Text("%s", cHeader);
														ImGui::Spacing();
													}

													ImGui::Columns(iColumns, "filescolumns4entities", false);  //false no border
													bHeaderDisplayed = true;
												}
												ImGui::SetWindowFontScale(SMALLFONTSIZE);

												float fFramePadding = (iColumnsWidth - media_icon_size)*0.5;
												float fCenterX = iColumnsWidth * 0.5;

												ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(fFramePadding, 2.0f));

												if (myfiles->iFlags == 1) 
												{
													ImVec4 bg_col = ImGui::GetStyle().Colors[ImGuiCol_PlotHistogram]; // { 0.0, 0.0, 0.0, 1.0 };
													ImVec2 padding = { 6.0, 6.0 };
													ImGuiWindow* window = ImGui::GetCurrentWindow();
													const ImRect image_bb((window->DC.CursorPos - padding) + ImVec2(fFramePadding, 2.0f), window->DC.CursorPos + padding + ImVec2(fFramePadding, 2.0f) + ImVec2(media_icon_size, media_icon_size));
													window->DrawList->AddRect(image_bb.Min, image_bb.Max, ImGui::GetColorU32(bg_col), 0.0f, 15, 3.0f);
													bAnySelectedItemsAvailable = true;
												}

												CheckTutorialAction(sFinal.c_str(), 13.0f); //Tutorial: check if we are waiting for this action

												//PE: Support Shift fo selecting may items.
												if (!io.KeyShift) 
												{
													//firstShiftFile = NULL;
													lastShiftFile = NULL;
												}

												extern bool g_bFreeTrialVersion;
												if (firstShiftFile && lastShiftFile)
												{
													if (myfiles == firstShiftFile)
														bFirstShiftHasBeenSeen = true;
													if (!bFirstShiftHasBeenSeen && myfiles == lastShiftFile)
													{
														//Swap around, last is first.
														cFolderItem::sFolderFiles * tmpShiftFile = lastShiftFile;
														lastShiftFile = firstShiftFile;
														firstShiftFile = tmpShiftFile;
														bFirstShiftHasBeenSeen = true;
													}

													static bool bStartShiftActive = false;
													myfiles->iFlags = 0;
													if (myfiles == firstShiftFile) 
													{
														bStartShiftActive = true;
														if(g_bFreeTrialVersion == false || (g_bFreeTrialVersion==true && myfiles->bAvailableInFreeTrial==true) )
															myfiles->iFlags = 1;
													}
													if (myfiles == lastShiftFile) 
													{
														bStartShiftActive = false;
														if (g_bFreeTrialVersion == false || (g_bFreeTrialVersion == true && myfiles->bAvailableInFreeTrial == true))
															myfiles->iFlags = 1;
													}
													if (bStartShiftActive)
													{
														if (g_bFreeTrialVersion == false || (g_bFreeTrialVersion == true && myfiles->bAvailableInFreeTrial == true))
															myfiles->iFlags = 1;
													}
												}

												if (ImGui::ImgBtn(textureId, ImVec2(media_icon_size, media_icon_size), drawCol_back, drawCol_normal, drawCol_hover, drawCol_Down, -1, 0, 0, 0, true))
												{

													if (bTutorialCheckAction) TutorialNextAction(); //Clicked get next tutorial action.

													//If ctrl , just mark them.

													if (io.KeyShift) 
													{
														if (firstShiftFile)
														{
															lastShiftFile = myfiles;
														}
														else
														{
															firstShiftFile = myfiles;
															multi_selections = true;
															if (g_bFreeTrialVersion == false || (g_bFreeTrialVersion == true && myfiles->bAvailableInFreeTrial == true))
																myfiles->iFlags = 1;
														}
													}
													else if (io.KeyCtrl) 
													{
														//Mark object.
														multi_selections = true;
														if (myfiles->iFlags == 0) 
														{
															if (g_bFreeTrialVersion == false || (g_bFreeTrialVersion == true && myfiles->bAvailableInFreeTrial == true))
																myfiles->iFlags = 1;
															firstShiftFile = myfiles;
														}
														else
														{
															myfiles->iFlags = 0;
														}
													}
													else
													{
														if (bWaypointDrawmode || bWaypoint_Window) { bWaypointDrawmode = false; bWaypoint_Window = false; }
														if (bImporter_Window) { importer_quit(); bImporter_Window = false; }
														if (g_bCharacterCreatorPlusActivated) g_bCharacterCreatorPlusActivated = false;

														//Make sure we are in entity mode.
														bForceKey = true;
														csForceKey = "e";

														DeleteWaypointsAddedToCurrentCursor();
														CheckTooltipObjectDelete();
														CloseDownEditorProperties();
														iLastEntityOnCursor = 0;

														std::string sFpeName = path_for_filename.c_str();
														sFpeName = sFpeName + "\\" + myfiles->m_sName.Get();
														t.addentityfile_s = sFpeName.c_str();
														if (t.addentityfile_s != "")
														{
															entity_adduniqueentity(false);
															t.tasset = t.entid;
															if (t.talreadyloaded == 0)
															{
																editor_filllibrary();
															}
														}
														iExtractMode = 0; //PE: Always start in find floor mode.
														t.inputsys.constructselection = t.tasset;

														t.gridentity = t.entid;
														t.inputsys.constructselection = t.entid;
														t.inputsys.domodeentity = 1;
														t.grideditselect = 5;
														//Make sure we use a fresh t.grideleprof
														entity_fillgrideleproffromprofile();

														editor_refresheditmarkers();
														//PE: Close window for now.
														bCheckForClosing = true;
													}
												}

												if (!bEntity_Properties_Window && !g_bCharacterCreatorPlusActivated && !bImporter_Window && i == 0 && ImGui::IsItemHovered()) {

													iTooltipHoveredTimer = MAXTimer();
													static void* additionalcheck = NULL;

													if (iLastTooltipSelection != textureId || (additionalcheck != myfiles)) {

														//Check if we need to delete a old tooltip object.
														if (iTooltipLastObjectId > 0 && iTooltipLastObjectId != textureId) {
															CheckTooltipObjectDelete();
														}
														additionalcheck = myfiles;
														iTooltipTimer = iTooltipHoveredTimer;
														iLastTooltipSelection = textureId;
														iTooltipObjectReady = false;

													}
													else 
													{
														if (iTooltipHoveredTimer - iTooltipTimer > 2000) { // 2 sec before starting.
															if (iTooltipObjectReady) 
															{
																if (iTooltipLastObjectId > 0) 
																{
																	if (GetImageExistEx(g.importermenuimageoffset + 50)) 
																	{
																		float TooltipImageSize = 320.0f;
																		float ImgX = ImageWidth(g.importermenuimageoffset + 50);
																		float ImgY = ImageHeight(g.importermenuimageoffset + 50);
																		float Ratio = TooltipImageSize / ImgX;
																		ImgY *= Ratio;
																		ImVec2 cursor_pos = ImGui::GetIO().MousePos;
																		ImVec2 tooltip_offset(10.0f, ImGui::GetFontSize()*1.5);
																		ImVec2 tooltip_position = cursor_pos;
																		if (tooltip_position.x + TooltipImageSize > GetDesktopWidth())
																			tooltip_position.x -= TooltipImageSize;
																		if (tooltip_position.y + TooltipImageSize > GetDesktopHeight())
																			tooltip_position.y -= (TooltipImageSize + ImGui::GetFontSize()*3.0);
																		tooltip_position.x += tooltip_offset.x;
																		tooltip_position.y += tooltip_offset.y;
																		ImGui::SetNextWindowPos(tooltip_position);
																		ImGui::SetNextWindowContentWidth(TooltipImageSize);
																		ImGui::BeginTooltip();
																		float icon_ratio;
																		ImGui::ImgBtn(g.importermenuimageoffset + 50, ImVec2(TooltipImageSize, ImgY), ImVec4(0.0, 0.0, 0.0, 1.0), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(0.8, 0.8, 0.8, 0.8), ImVec4(0.8, 0.8, 0.8, 0.8), 0, 0, 0, 0, false);
																		ImGui::EndTooltip();
																	}
																	else
																		ImGui::SetTooltip("%s", sFinal.c_str());
																}
															}
															else 
															{
																//Generate Thumbnail of object.
																std::string sFpeName = path_for_filename.c_str();
																sFpeName = sFpeName + "\\" + myfiles->m_sName.Get();
																t.addentityfile_s = sFpeName.c_str();

																CreateBackBufferCacheName(t.addentityfile_s.Get(), 512, 512);
																BackBufferSaveCacheName = BackBufferCacheName;
																GG_SetWritablesToRoot(true);
																if (FileExist(BackBufferCacheName.Get()))
																{
																	SetMipmapNum(1); //PE: mipmaps not needed.
																	image_setlegacyimageloading(true);
																	if (ImageExist(g.importermenuimageoffset + 50))
																		DeleteImage(g.importermenuimageoffset + 50);
																	LoadImage((char *)BackBufferCacheName.Get(), g.importermenuimageoffset + 50);
																	image_setlegacyimageloading(false);
																	SetMipmapNum(-1);
																	GG_SetWritablesToRoot(false);
																	iTooltipObjectReady = true;
																	iTooltipLastObjectId = t.entid;
																}
																else
																{
																	GG_SetWritablesToRoot(false);
																	t.entdir_s = "entitybank\\";
																	if (cstr(Lower(Left(t.addentityfile_s.Get(), 11))) == "entitybank\\")
																	{
																		t.addentityfile_s = Right(t.addentityfile_s.Get(), Len(t.addentityfile_s.Get()) - 11);
																	}
																	if (cstr(Lower(Left(t.addentityfile_s.Get(), 8))) == "ebebank\\")
																	{
																		t.entdir_s = "";
																	}

																	t.talreadyloaded = 0;
																	for (t.t = 1; t.t <= g.entidmaster; t.t++)
																	{
																		if (t.entitybank_s[t.t] == t.addentityfile_s) { t.talreadyloaded = 1; t.entid = t.t; }
																	}
																	if (t.talreadyloaded == 0)
																	{
																		//  Allocate one more entity item in array
																		if (g.entidmaster > g.entitybankmax - 4)
																		{
																			Dim(t.tempentitybank_s, g.entitybankmax);
																			for (t.t = 0; t.t <= g.entitybankmax; t.t++) t.tempentitybank_s[t.t] = t.entitybank_s[t.t];
																			++g.entitybankmax;
																			UnDim(t.entitybank_s);
																			Dim(t.entitybank_s, g.entitybankmax);
																			for (t.t = 0; t.t <= g.entitybankmax - 1; t.t++) t.entitybank_s[t.t] = t.tempentitybank_s[t.t];
																		}

																		//  Add entity to bank
																		++g.entidmaster; entity_validatearraysize();
																		t.entitybank_s[g.entidmaster] = t.addentityfile_s;

																		if (ObjectExist(g.entitybankoffset + g.entidmaster)) 
																		{
																			DeleteObject(g.entitybankoffset + g.entidmaster);
																		}

																		//  Load extra entity
																		t.entid = g.entidmaster;
																		t.ent_s = t.entitybank_s[t.entid];
																		t.entpath_s = getpath(t.ent_s.Get());

																		extern bool g_bGracefulWarningAboutOldXFiles;
																		g_bGracefulWarningAboutOldXFiles = true;
																		entity_load();
																		g_bGracefulWarningAboutOldXFiles = false;
																		HideObject(g.entitybankoffset + g.entidmaster);
																		if (t.entityprofile[g.entidmaster].ischaracter == 1) {
																			RotateObject(g.entitybankoffset + g.entidmaster, 0, 180, 0);
																		}
																		g.entidmaster--; //Dont actual add it.

																		//entity_load can change folder by creating a dbo , so update timestamp without refresh.
																		struct stat sb;
																		if (stat(pNewFolder->m_sFolderFullPath.Get(), &sb) == 0) 
																		{
																			if (sb.st_mtime != pNewFolder->m_tFolderModify) 
																			{
																				pNewFolder->m_tFolderModify = sb.st_mtime;
																			}
																		}
																		//Create a new thumbnail.
																	}

																	iTooltipLastObjectId = t.entid;
																	iTooltipAlreadyLoaded = t.talreadyloaded;
																	iTooltipObjectReady = true;
																	BackBufferObjectID = g.entitybankoffset + t.entid;
																	BackBufferImageID = g.importermenuimageoffset + 50;
																	BackBufferSizeX = 512;
																	BackBufferSizeY = 512;
																	BackBufferSaveCacheName = ""; //Dont save for now.
																}
															}
														}
													}
												}

												if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
												{

													CheckTooltipObjectDelete();
													CloseDownEditorProperties();
													t.inputsys.constructselection = 0;

													myfiles->m_dropptr = myfiles;

													std::string sFpeName = path_for_filename.c_str();
													sFpeName = sFpeName + "\\" + myfiles->m_sName.Get();

													myfiles->m_sFolder = sFpeName.c_str();
													ImGui::SetDragDropPayload("DND_MODEL_DROP_TARGET", myfiles, sizeof(void *));
													ImGui::ImgBtn(textureId, ImVec2(media_icon_size, media_icon_size), drawCol_back, drawCol_normal, drawCol_hover, drawCol_Down, 0, 0, 0, 0, true);
													//ImGui::Text("%s", myfiles->m_sName.Get());
													ImGui::SetCursorPos(oldCursor);
													pDragDropFile = myfiles;
													ImGui::EndDragDropSource();
												}

												ImGui::PopStyleVar();

												if (bDisplayText) {
													int iTextWidth = ImGui::CalcTextSize(sFinal.c_str()).x;
													if (iTextWidth < iColumnsWidth)
														ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX() + (fCenterX - (iTextWidth*0.5)), ImGui::GetCursorPosY()));
													ImGui::TextWrapped("%s", sFinal.c_str());
												}

												ImGui::NextColumn();
											}
											ImGui::PopID();
											preview_count++;

											myfiles = myfiles->m_pNext;
										}

										ImGui::Columns(1);

										ImGui::SetWindowFontScale(1.0);
									}
								}
								pNewFolder = pNewFolder->m_pNext;
							}

						}
					}

					if (!bAnySelectedItemsAvailable) {
						//PE: We got no selections , we can reset first shift seen.
						if (!io.KeyShift) {
							firstShiftFile = NULL;
						}
					}
					ImGui::SetWindowFontScale(1.0);

					ImRect bbwin(ImGui::GetWindowPos(), ImGui::GetWindowPos() + ImGui::GetWindowSize());
					if (ImGui::IsMouseHoveringRect(bbwin.Min, bbwin.Max))
					{
						bImGuiGotFocus = true;
						bEntityGotFocus = true;
					}
					if (ImGui::IsAnyItemFocused()) {
						bImGuiGotFocus = true;
						bEntityGotFocus = true;
					}


					if (lf_multi_selections_count > 0) {
						ImGui::SetCursorPos(ImVec2(0, (ImGui::GetWindowSize().y + ImGui::GetScrollY()) - insert_text_width.y - 10.0f));
						ImGui::Spacing();
						ImGui::SameLine(ImGui::GetWindowContentRegionWidth() - (insert_text_width.x + 18.0f));
						ImGui::SetItemAllowOverlap();
						if (ImGui::StyleButton(insert_text.c_str())) {
							bAddSelectionToGame = true;
						}

					}

					ImGui::EndChild();

					ImGui::EndTabItem();
				}

				ImRect bbwin(ImGui::GetWindowPos(), ImGui::GetWindowPos() + ImGui::GetWindowSize());
				if (ImGui::IsMouseHoveringRect(bbwin.Min, bbwin.Max))
				{
					bImGuiGotFocus = true;
					bEntityGotFocus = true;
				}
				if (ImGui::IsAnyItemFocused()) {
					bImGuiGotFocus = true;
					bEntityGotFocus = true;
				}

			}
		}

#ifdef DYNAMICLOADUNLOAD
		max_load_persync = 10; // 15 to slow try 10
#endif
		lf_multi_selections_count = multi_selections_count; //Use last frames count.

		ImGui::EndTabBar();


		if (bAddSelectionToGame) {

			DeleteWaypointsAddedToCurrentCursor();
			CheckTooltipObjectDelete();
			CloseDownEditorProperties();

			//Remove any selections.
			t.inputsys.constructselection = 0;
			if (t.gridentityobj > 0)
			{
				DeleteObject(t.gridentityobj);
				t.gridentityobj = 0;
			}
			t.refreshgrideditcursor = 1;
			t.gridentity = 0;
			t.gridentityposoffground = 0;
			t.gridentityusingsoftauto = 0;
			editor_refresheditmarkers();

			cFolderItem *pSearchFolder = &MainEntityList;
			pSearchFolder = pSearchFolder->m_pNext;
			cStr path_remove;
			int ipath_remove_len;
			if (pSearchFolder) {
				path_remove = pSearchFolder->m_sFolderFullPath.Get();
				ipath_remove_len = path_remove.Len();
			}
			while (pSearchFolder) {

				cStr path = pSearchFolder->m_sFolderFullPath.Get();
				bool bDoubleEntityBank = false;
				char *finde = (char *)pestrcasestr(path.Get(), "\\entitybank"); //Support entitybank inside entitybank.
				if (finde)
				{
					finde += 11;
					finde = (char *)pestrcasestr(finde, "\\entitybank");
					if (finde) bDoubleEntityBank = true;
				}

				if (!bDoubleEntityBank && path.Right(11) == "\\entitybank") 
				{
					ipath_remove_len = path.Len();
				}
				else
				{
					if (pSearchFolder->m_pFirstFile) 
					{
						cFolderItem::sFolderFiles * searchfiles = pSearchFolder->m_pFirstFile->m_pNext;
						while (searchfiles) 
						{
							if (searchfiles->iFlags == 1) 
							{
								if (bWaypointDrawmode || bWaypoint_Window) { bWaypointDrawmode = false; bWaypoint_Window = false; }
								if (bImporter_Window) { importer_quit(); bImporter_Window = false; }
								if (g_bCharacterCreatorPlusActivated) g_bCharacterCreatorPlusActivated = false;

								//Insert.
								cStr path = pSearchFolder->m_sFolderFullPath.Get();
								char *final_name = path.Get();
								final_name += ipath_remove_len;
								if (*final_name == '\\')
									final_name++;
								std::string path_for_filename = final_name;

								std::string sFpeName = path_for_filename.c_str();
								sFpeName = sFpeName + "\\" + searchfiles->m_sName.Get();
								iLastEntityOnCursor = 0;

								t.addentityfile_s = sFpeName.c_str();
								if (t.addentityfile_s != "")
								{
									entity_adduniqueentity(false);
									t.tasset = t.entid;
									if (t.talreadyloaded == 0)
									{
										editor_filllibrary();
									}
								}

								searchfiles->iFlags = 0;
							}
							searchfiles = searchfiles->m_pNext;
						}
					}
				}
				pSearchFolder = pSearchFolder->m_pNext;
			}
			bCheckForClosing = true;
		}

		bool bAreWeOverLapping = false;
		if (!bIsWeDocked) {
			//If we are over the rendertarget hide window.
			float itmpmousex = ImGui::GetWindowPos().x;
			float itmpmousey = ImGui::GetWindowPos().y;
			int iSecureZone = 4;
			if (bImGuiRenderTargetFocus && itmpmousex >= (renderTargetAreaPos.x + iSecureZone) && itmpmousey >= (renderTargetAreaPos.y + iSecureZone) &&
				itmpmousex <= renderTargetAreaPos.x + (renderTargetAreaSize.x - iSecureZone) && itmpmousey <= renderTargetAreaPos.y + (renderTargetAreaSize.y - ImGuiStatusBar_Size - iSecureZone))
			{
				bAreWeOverLapping = true;
			}
			itmpmousex = ImGui::GetWindowPos().x + ImGui::GetWindowSize().x;
			itmpmousey = ImGui::GetWindowPos().y + ImGui::GetWindowSize().y;
			if (bExternal_Entities_Window && bImGuiRenderTargetFocus && itmpmousex >= (renderTargetAreaPos.x + iSecureZone) && itmpmousey >= (renderTargetAreaPos.y + iSecureZone) &&
				itmpmousex <= renderTargetAreaPos.x + (renderTargetAreaSize.x - iSecureZone) && itmpmousey <= renderTargetAreaPos.y + (renderTargetAreaSize.y - ImGuiStatusBar_Size - iSecureZone))
			{
				bAreWeOverLapping = true;
			}
			itmpmousex = ImGui::GetWindowPos().x + ImGui::GetWindowSize().x;
			itmpmousey = ImGui::GetWindowPos().y;
			if (bExternal_Entities_Window && bImGuiRenderTargetFocus && itmpmousex >= (renderTargetAreaPos.x + iSecureZone) && itmpmousey >= (renderTargetAreaPos.y + iSecureZone) &&
				itmpmousex <= renderTargetAreaPos.x + (renderTargetAreaSize.x - iSecureZone) && itmpmousey <= renderTargetAreaPos.y + (renderTargetAreaSize.y - ImGuiStatusBar_Size - iSecureZone))
			{
				bAreWeOverLapping = true;
			}
			itmpmousex = ImGui::GetWindowPos().x;
			itmpmousey = ImGui::GetWindowPos().y + ImGui::GetWindowSize().y;
			if (bExternal_Entities_Window && bImGuiRenderTargetFocus && itmpmousex >= (renderTargetAreaPos.x + iSecureZone) && itmpmousey >= (renderTargetAreaPos.y + iSecureZone) &&
				itmpmousex <= renderTargetAreaPos.x + (renderTargetAreaSize.x - iSecureZone) && itmpmousey <= renderTargetAreaPos.y + (renderTargetAreaSize.y - ImGuiStatusBar_Size - iSecureZone))
			{
				bAreWeOverLapping = true;
			}
		}
		if (!bIsWeDocked && bCheckForClosing) {
			if (bAreWeOverLapping)
				bExternal_Entities_Window = false;
		}

		//Remove window on right click if we are overlapping.
		if (bAreWeOverLapping && t.inputsys.mclick == 2)
			bExternal_Entities_Window = false;


		bCheckForClosing = false;

		CheckMinimumDockSpaceSize(250.0f);

		ImGui::End();

	}

}


void FormatLUAFilenameToTitle(LPSTR cDisplayName)
{
	char cRemoveUnderscores[MAX_PATH];
	strcpy (cRemoveUnderscores, "");
	LPSTR pRemoveUnderscores = cRemoveUnderscores;
	bool bFirstLetterOfWord = true;
	for (int n = 0; n < strlen(cDisplayName); n++)
	{
		if (cDisplayName[n] != '_')
		{
			if (bFirstLetterOfWord == true)
			{
				if (cDisplayName[n] >= 'a' && cDisplayName[n] <= 'z')
				{
					char pUpper[2];
					pUpper[0] = cDisplayName[n];
					pUpper[1] = 0;
					strupr(pUpper);
					*(pRemoveUnderscores++) = pUpper[0];
				}
				else
					*(pRemoveUnderscores++) = cDisplayName[n];

				bFirstLetterOfWord = false;
			}
			else
			{
				*(pRemoveUnderscores++) = cDisplayName[n];
			}
		}
		else
		{
			bFirstLetterOfWord = true;
		}
	}
	*(pRemoveUnderscores++) = 0;

	// Now prepare spaces
	char cNewDisplayName[MAX_PATH];
	strcpy (cNewDisplayName, "");
	LPSTR pNewDisPtr = cNewDisplayName;
	for (int n = 0; n < strlen(cRemoveUnderscores); n++)
	{
		// no .LUA extension in title
		if (cRemoveUnderscores[n] == '.')
		{
			break;
		}

		// construct new title, adding spaces between capitalised words
		if (n > 0 && cRemoveUnderscores[n] >= 'A' && cRemoveUnderscores[n] <= 'Z')
		{
			*(pNewDisPtr++) = ' ';
		}
		*(pNewDisPtr++) = cRemoveUnderscores[n];
	}
	*(pNewDisPtr++) = 0;
	strcpy (cDisplayName, cNewDisplayName);
}


struct folder_info
{
	int level;
	int type;
	int id;
	int parentid;
	int folders;
	bool bPinned;
	bool bUsed;
	char real_name[260];
	char show_name[260];
	cFolderItem *pFolder;
};

std::map<std::string, folder_info *> root_folders;
int seleted_tree_item = -1;
bool bViewAllFolders = true;
bool bViewShowcase = false;
bool bViewPurchased = false;
char cSearchAllEntities[3][MAX_PATH] = { "\0","\0","\0" };
bool bUpdateSearchSorting = false;
bool bUpdateSearchScrollbar = false;
bool bUpdateSearchSortingNextFrame = false;
bool bTreeViewInitInNextFrame = false;

bool DoTreeNode(int parentid, char *ignore, char *ignore2, char *selectfolder = NULL, char* pstartfolder = NULL, bool bSwapInSteamName = false)
{
	bool bViewingCommunityFolder = false;
	auto it = root_folders.begin();
	if (pstartfolder)
	{
		// option to do tree in only one root folder (Community)
		for (; it != root_folders.end(); ++it)
		{
			if ( it->second->level==0 && it->second->type == iDisplayLibraryType && stricmp(pstartfolder, it->second->show_name) == NULL)
			{
				bViewingCommunityFolder = true;
				break;
			}
		}
	}
	for (; it != root_folders.end(); ++it)
	{
		bool bValid = true;
		int iCompareType = iDisplayLibraryType;
		if (iDisplayLibraryType == 0 && iDisplayLibrarySubType == 1) iCompareType = 6; // Select only animation files when subtype=1
		if (it->second->type == iCompareType && (it->second->parentid == parentid || (it->second->bPinned && parentid == 0)))
		{
			//Pinned level 0 display as is.
			ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_OpenOnArrow;
			if (it->second->folders <= 0)
				node_flags = ImGuiTreeNodeFlags_Leaf;
			if (seleted_tree_item == it->second->id)
				node_flags |= ImGuiTreeNodeFlags_Selected;
			else
				node_flags &= ~ImGuiTreeNodeFlags_Selected;

			ImGui::PushItemWidth(-20.0);

			std::string treename = it->second->show_name;

			if (parentid != 0) bValid = true;
			if (parentid == 0 && ignore && stricmp(treename.c_str(), ignore) == NULL) bValid = false;
			if (parentid == 0 && ignore2 && stricmp(treename.c_str(), ignore2) == NULL) bValid = false;
			if (parentid == 0)
			{
				// special extended filters for specific types
				if (ignore && stricmp(ignore,"AllNoneBehaviorFolders")==NULL)
				{
					if (stricmp(cSearchAllEntities[0], "purchased") == NULL)
					{
						// only show purchased behaviors
						if (treename == "Animals") bValid = false;
						if (treename == "Effects") bValid = false;
						if (treename == "Horror") bValid = false;
						if (treename == "Markers") bValid = false;
						if (treename == "Objects") bValid = false;
						if (treename == "People") bValid = false;
						if (treename == "Puzzle") bValid = false;
						if (treename == "Rpg") bValid = false;
						if (treename == "User") bValid = false;
						if (treename == "Ai") bValid = false;
						if (treename == "Gfx") bValid = false;
						if (treename == "Images") bValid = false;
						if (treename == "Weather") bValid = false;
					}
					else
					{
						// regular behavior view
						//bValid = true;
						if (treename == "Ai") bValid = false;
						if (treename == "Gfx") bValid = false;
						if (treename == "Images") bValid = false;
						if (treename == "Weather") bValid = false;
					}
				}
			}
			if (bValid)
			{
				if (it->second->bPinned)
				{
					treename = it->second->real_name;
					treename[0] = toupper(treename[0]);
				}

				// shows name in UI
				char pChangeUIName[256];
				strcpy(pChangeUIName, treename.c_str());
				if (bSwapInSteamName == true)
				{
					// all items subscribed to includes meta data for the steam user who created it, we will use the first instance of that
					#ifndef GGMAXEDU
					for (int i = 0; i < g_workshopSteamUserNames.size(); i++)
					{
						if (stricmp(g_workshopSteamUserNames[i].sSteamUserAccountID.Get(), pChangeUIName) == NULL)
						{
							strcpy(pChangeUIName, g_workshopSteamUserNames[i].sSteamUsersPersonaName.Get());
							break;
						}
					}
					#endif
				}
				bool TreeNodeOpen = ImGui::TreeNodeEx((void*)(intptr_t)it->second->id, node_flags, pChangeUIName);

				ImGui::PopItemWidth();
				//PE: Default category select.
				if (selectfolder != NULL && seleted_tree_item == -1 && _stricmp(selectfolder, it->second->real_name) == 0)
				{
					//Select category.
					seleted_tree_item = it->second->id;
				}
				//PE: Select on mouse release.
				if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(0))
				{
					seleted_tree_item = it->second->id;
					strcpy(cSearchAllEntities[0], it->second->real_name);
					bUpdateSearchSorting = true;
					bUpdateSearchScrollbar = true;
				}
				ImGui::SameLine();
				ImGui::SetItemAllowOverlap();
				ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(ImGui::GetContentRegionAvailWidth() - 18.0, -4.0));
				ImGui::PushID(it->second->id + 80000);
				ImVec4 back = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);

				if (it->second->bPinned)
				{
					if (ImGui::ImgBtn(MEDIA_UNPIN, ImVec2(15, 15), back, ImColor(255, 255, 255, 255), ImColor(255, 255, 255, 128), drawCol_Down, -1, 0, 0, 0, false, false, false, false, true, bBoostIconColors))
					{
						//PE: Delete pinned category.
						extern std::vector<std::string> files_pinned_categories;
						char tmp[2];
						tmp[0] = it->second->type + '0';
						tmp[1] = 0;
						cstr namelower = cstr(Lower(it->second->real_name)) + cstr(tmp);
						for (int i = 0; i < files_pinned_categories.size(); i++)
						{
							cstr check = cstr((char *)files_pinned_categories[i].c_str()).Lower();
							if (namelower == check)
							{
								files_pinned_categories.erase(files_pinned_categories.begin() + i);
							}
						}
						saveVectorFileContent("pinnedlist.ini", files_pinned_categories);
						it->second->bPinned = false;
						bTreeViewInitInNextFrame = true;
					}
					if (ImGui::IsItemHovered()) ImGui::SetTooltip("Unpin Category");
				}
				else
				{
					if (ImGui::ImgBtn(MEDIA_PIN, ImVec2(15, 15), back, ImColor(255, 255, 255, 255), ImColor(255, 255, 255, 128), drawCol_Down, -1, 0, 0, 0, false, false, false, false, true, bBoostIconColors))
					{
						//PE Save pinned category.
						extern std::vector<std::string> files_pinned_categories;
						char tmp[2];
						tmp[0] = it->second->type + '0';
						tmp[1] = 0;
						cstr name = cstr(it->second->real_name) + cstr(tmp);
						files_pinned_categories.push_back(name.Get());
						saveVectorFileContent("pinnedlist.ini", files_pinned_categories);
						it->second->bPinned = true;
						bTreeViewInitInNextFrame = true;
					}
					if (ImGui::IsItemHovered()) ImGui::SetTooltip("Pin Category");
				}
				ImGui::PopID();
				ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(0.0, -8.0));

				if (TreeNodeOpen) 
				{
					ImGui::Indent(-5);
					bool bSwapName = false;
					if (bViewingCommunityFolder == true && it->second->level == 0) bSwapName = true;
					if (parentid != it->second->id)	DoTreeNode(it->second->id, ignore, ignore2, NULL, NULL, bSwapName);
					ImGui::Indent(5);
					ImGui::TreePop();
				}
			}
		}
		if (pstartfolder)
		{
			// leave after processing this one folder when pstartfolder mode used
			return(0);
		}
	}
	return(0);
}

bool DoTreeNodeSearch(int parentid, char *lookup)
{
	for (auto it = root_folders.begin(); it != root_folders.end(); ++it)
	{
		bool bValid = false;
		it->second->bUsed = false;

		int iCompareType = iDisplayLibraryType;
		if (iDisplayLibraryType == 0 && iDisplayLibrarySubType == 1) iCompareType = 6; // Select only animation files when subtype=1
		if (it->second->type == iCompareType && (it->second->parentid == parentid || (it->second->bPinned && parentid == 0)))
		{
			//Pinned level 0 display as is.

			ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_OpenOnArrow;
			if (it->second->folders <= 0)
				node_flags = ImGuiTreeNodeFlags_Leaf;
			if (seleted_tree_item == it->second->id)
				node_flags |= ImGuiTreeNodeFlags_Selected;
			else
				node_flags &= ~ImGuiTreeNodeFlags_Selected;

			ImGui::PushItemWidth(-20.0);

			std::string treename = it->second->show_name;

			if (parentid != 0) bValid = true;
			if (parentid == 0 && lookup && treename == lookup) bValid = true;

			if (bValid)
			{
				it->second->bUsed = true;

				if (it->second->bPinned)
				{
					treename = it->second->real_name;
					treename[0] = toupper(treename[0]);
				}
				bool TreeNodeOpen = ImGui::TreeNodeEx((void*)(intptr_t)it->second->id, node_flags, treename.c_str());
				ImGui::PopItemWidth();
				//PE: Select on mouse release.
				if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(0))
				{
					seleted_tree_item = it->second->id;
					strcpy(cSearchAllEntities[0], it->second->real_name);
					bUpdateSearchSorting = true;
					bUpdateSearchScrollbar = true;
				}

				if (TreeNodeOpen) 
				{
					ImGui::Indent(-5);
					if (parentid != it->second->id)
						DoTreeNodeSearch(it->second->id, lookup);
					ImGui::Indent(5);
					ImGui::TreePop();
				}
			}
		}
	}
	return(0);
}

bool bDisplayProjectMedia = false;
bool bDisplayFavorite = false;

void process_gotopurchaedandrefreshtopurchases ( bool bForceSearch )
{
	seleted_tree_item = -1;
	if(!bIncludeDocumentFolderInRemoteProject || bForceSearch)
		strcpy(cSearchAllEntities[0], "Purchased");
	bDisplayProjectMedia = false;
	bDisplayFavorite = false;
	bViewAllFolders = false;
	bViewShowcase = false;
	bViewPurchased = true;
	bUpdateSearchSorting = true;
	bUpdateSearchScrollbar = true;
}

