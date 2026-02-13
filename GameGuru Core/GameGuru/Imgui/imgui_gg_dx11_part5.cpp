float fDescriptionBoxSize = 0;
cstr sDescriptionBoxSize = "";
int DisplayLuaDescription(entityeleproftype *tmpeleprof)
{
	char tmp[2048];
	bool bUpdateMainString = false;
	int speech_entries = 0;
	float fPropertiesColoumWidth = ImGui::GetCursorPosX() + 110.0f;
	
	ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;

	if (1) 
	{
		ImGui::PushStyleColor(ImGuiCol_ChildBg, style.Colors[ImGuiCol_FrameBg]);
		ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 4.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, style.FrameBorderSize);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, style.FramePadding);

		bool ret;

		if (fDescriptionBoxSize > ImGui::GetFontSize() * 7) 
		{
			ret = ImGui::BeginChild("##DLUADescriptionbox", ImVec2(ImGui::GetContentRegionAvailWidth() - 10, (ImGui::GetFontSize()*7)), true, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysUseWindowPadding );
		}
		else 
		{
			if (fDescriptionBoxSize == 0)
				ret = ImGui::BeginChild("##DLUADescriptionbox", ImVec2(ImGui::GetContentRegionAvailWidth() - 10, (ImGui::GetFontSize()*5)), true, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysUseWindowPadding | ImGuiWindowFlags_NoScrollbar);
			else
				ret = ImGui::BeginChild("##DLUADescriptionbox", ImVec2(ImGui::GetContentRegionAvailWidth() - 10, fDescriptionBoxSize), true, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysUseWindowPadding | ImGuiWindowFlags_NoScrollbar);
		}
		void fDisplayDescriptionBox(entityeleproftype *tmpeleprof, bool textonly = false);
		fDisplayDescriptionBox(tmpeleprof);
		ImGui::EndChild();

		ImGui::PopStyleVar(3);
		ImGui::PopStyleColor();
	}
	ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));

	extern int speech_ids[5];

	int imageindexi = 0; // can have eight images indexed this way
	bool bwpefile = false;
	bool bwpeyoffet = false;

	for (int i = 0; i < tmpeleprof->PropertiesVariable.iVariables; i++) 
	{
		bool speech = false;

		cstr tmpvar = tmpeleprof->PropertiesVariable.Variable[i];
		tmpvar = tmpvar.Lower();

		if (speech_entries <= 3) {
			if (tmpvar == "speech1" || tmpvar == "speech 1") {
				speech_ids[speech_entries++] = i; speech = true;
			}
			else if (tmpvar == "speech2" || tmpvar == "speech 2") {
				speech_ids[speech_entries++] = i; speech = true;
			}
			else if (tmpvar == "speech3" || tmpvar == "speech 3") {
				speech_ids[speech_entries++] = i; speech = true;
			}
			else if (tmpvar == "speech4" || tmpvar == "speech 4") {
				speech_ids[speech_entries++] = i; speech = true;
			}
			else if (tmpvar == "speech0" || tmpvar == "speech 0") {
				speech_ids[speech_entries++] = i; speech = true;
			}
		}
		if (!speech)
		{
			if (tmpeleprof->PropertiesVariable.VariableType[i] == 3)
			{
				// BOOL needs to be on one line
				ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));
				ImGui::Text(DLUAFormatLabel(tmpeleprof->PropertiesVariable.Variable[i]).Get());
				ImGui::SameLine();
				ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() - 3));
				ImGui::SetCursorPos(ImVec2(fPropertiesColoumWidth, ImGui::GetCursorPosY()));
			}
			else
			{
				// REST can use new gadget style
				ImGui::TextCenter(DLUAFormatLabel(tmpeleprof->PropertiesVariable.Variable[i]).Get());
			}

			ImGui::PushID(54321 + i); //unique id.
			ImGui::PushItemWidth(-10);
			cstr val = tmpeleprof->PropertiesVariable.VariableValue[i];
			if (tmpeleprof->PropertiesVariable.VariableType[i] == 1)
			{
				//Float
				float tmpfloat = atof(val.Get());
				if (tmpeleprof->PropertiesVariable.VariableValueTo[i] > 0 && tmpeleprof->PropertiesVariable.VariableValueTo[i] > tmpeleprof->PropertiesVariable.VariableValueFrom[i])
				{
					ImGui::PushItemWidth(-50);
					if (ImGui::SliderFloat("##floatslider", &tmpfloat, tmpeleprof->PropertiesVariable.VariableValueFrom[i], tmpeleprof->PropertiesVariable.VariableValueTo[i], " "))
					{
						sprintf(tmp, "%f", tmpfloat);
						strcpy(tmpeleprof->PropertiesVariable.VariableValue[i], tmp);
						bUpdateMainString = true;
					}
					ImGui::SameLine();
					ImGui::PopItemWidth();
					ImGui::PushItemWidth(30);
					if (ImGui::InputFloat("##floatinput", &tmpfloat, 0, 0, "%.2f"))
					{
						// optionally cap within any specified range
						if (tmpfloat < tmpeleprof->PropertiesVariable.VariableValueFrom[i]) tmpfloat = tmpeleprof->PropertiesVariable.VariableValueFrom[i];
						if (tmpfloat > tmpeleprof->PropertiesVariable.VariableValueTo[i]) tmpfloat = tmpeleprof->PropertiesVariable.VariableValueTo[i];
					
						sprintf(tmp, "%f", tmpfloat);
						strcpy(tmpeleprof->PropertiesVariable.VariableValue[i], tmp);
						bUpdateMainString = true;
					}
					ImGui::PopItemWidth();
				}
				else
				{
					cstr title = cstr("##float") + cstr(tmpeleprof->PropertiesVariable.Variable[i]);
					if (ImGui::InputFloat(title.Get(), &tmpfloat))
					{
						// ZJ: Range based floats are above.
						//// optionally cap within any specified range
						//if (tmpfloat < tmpeleprof->PropertiesVariable.VariableValueFrom[i]) tmpfloat = tmpeleprof->PropertiesVariable.VariableValueFrom[i];
						//if (tmpfloat > tmpeleprof->PropertiesVariable.VariableValueTo[i]) tmpfloat = tmpeleprof->PropertiesVariable.VariableValueTo[i];
					}
					sprintf(tmp, "%f", tmpfloat);
					strcpy(tmpeleprof->PropertiesVariable.VariableValue[i], tmp);
					bUpdateMainString = true;
				}

			}
			else if (tmpeleprof->PropertiesVariable.VariableType[i] == 2) 
			{
				//String
				char * imgui_setpropertyfile2(int group, char* data_s, char* field_s, char* desc_s, char* within_s);
				char * imgui_setpropertyfile2_v2(int group, char* data_s, char* field_s, char* desc_s, char* within_s, bool readonly, char *startsearch = NULL);;
				//Special setups.
				//VIDEO1, FILE-IMAGE for file selector.
				if (pestrcasestr(tmpvar.Get(), "file")) 
				{
					if (pestrcasestr(tmpvar.Get(), "image")) 
					{
						cstr tmpvalue = tmpeleprof->PropertiesVariable.VariableValue[i];
						bool readonly = false;

						//Allow up to 8 images to be previewed in the properties area
						//#define IMGFILEID (PROPERTIES_CACHE_ICONS+998)
						int iImgFileIndex = imageindexi;
						if (iImgFileIndex > 8) iImgFileIndex = 8;
						int iImgFileID = PROPERTIES_CACHE_ICONS + 900 + iImgFileIndex;

						static cstr imgfile[10] = { "", "", "", "", "", "", "", "", "", "" };
						static int imgfile_preview_id[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
						if (tmpvalue != imgfile[iImgFileIndex] )
						{
							//Load new image preview.
							imgfile_preview_id[iImgFileIndex] = 0;
							if (tmpvalue != "")
							{
								image_setlegacyimageloading(true);
								LoadImage((char *)tmpvalue.Get(), iImgFileID);
								image_setlegacyimageloading(false);
								imgfile_preview_id[iImgFileIndex] = iImgFileID;
								if (!GetImageExistEx(iImgFileID))
								{
									imgfile_preview_id[iImgFileIndex] = 0;
								}
							}
							imgfile[iImgFileIndex] = tmpvalue;
						}

						tmpvalue = imgui_setpropertyfile2_v2(1, tmpvalue.Get(), "", "Select image to appear in-level", "imagebank\\", readonly);

						if (imgfile_preview_id[iImgFileIndex] > 0 && GetImageExistEx(imgfile_preview_id[iImgFileIndex]))
						{
							extern ImVec4 drawCol_back;
							extern ImVec4 drawCol_normal;
							extern ImVec4 drawCol_hover;
							extern ImVec4 drawCol_Down;

							float w = ImGui::GetContentRegionAvailWidth();
							float iwidth = w;
							float ImgW = ImageWidth(imgfile_preview_id[iImgFileIndex]);
							float ImgH = ImageHeight(imgfile_preview_id[iImgFileIndex]);
							float fHighRatio = ImgH / ImgW;
							if (ImgW < (iwidth - 18.0f))
							{
								//PE: Fit to width.
								iwidth = ImgW + 18.0f;
							}
							ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((w*0.5) - (iwidth*0.5), 0.0f));
							ImGui::ImgBtn(imgfile_preview_id[iImgFileIndex], ImVec2( iwidth-18.0f, (iwidth - 18.0f) * fHighRatio), drawCol_back, drawCol_normal, drawCol_normal, drawCol_normal, -1, 0, 0, 0, true);
						}
						if (tmpvalue != tmpeleprof->PropertiesVariable.VariableValue[i]) {
							strcpy(tmpeleprof->PropertiesVariable.VariableValue[i], tmpvalue.Get());
							bUpdateMainString = true;
						}
						imageindexi++;
					}
					else 
					{
						if (pestrcasestr(tmpeleprof->PropertiesVariable.Variable[i], "wpefile"))
						{
							if (FileExist(tmpeleprof->PropertiesVariable.VariableValue[i]))
							{
								//PE: Possible to preview.
								bwpefile = true;
							}
						}
						cstr tmpvalue = tmpeleprof->PropertiesVariable.VariableValue[i];
						tmpvalue = imgui_setpropertyfile2(1, tmpvalue.Get(), "", "Select File", "..\\files\\");
						if (tmpvalue != tmpeleprof->PropertiesVariable.VariableValue[i]) 
						{
							LPSTR pThisString = tmpvalue.Get();
							if (pThisString[1] == ':')
							{
								// replace absolute paths with relative ones
								char pRelativePathAndFile[MAX_PATH];
								strcpy(pRelativePathAndFile, tmpvalue.Get());
								GG_GetRealPath(pRelativePathAndFile, 0);
								extern char szWriteDir[MAX_PATH];
								char pRemoveAbsPart[MAX_PATH];
								strcpy(pRemoveAbsPart, szWriteDir);
								strcat(pRemoveAbsPart, "Files\\");
								if (strnicmp(pRelativePathAndFile, pRemoveAbsPart, strlen(pRemoveAbsPart)) == NULL)
								{
									strcpy(pRelativePathAndFile, pThisString + strlen(pRemoveAbsPart));
								}
								strcpy(tmpeleprof->PropertiesVariable.VariableValue[i], pRelativePathAndFile);
							}
							else
							{
								strcpy(tmpeleprof->PropertiesVariable.VariableValue[i], tmpvalue.Get());
							}
							bUpdateMainString = true;
							if (PreviewWPERoot != 0 && bPreviewWPE)
							{
								//PE: Delete effect and reload.
								WickedCall_PerformEmitterAction(6, PreviewWPERoot);
								void DeleteEmitterEffects(uint32_t root);
								DeleteEmitterEffects(PreviewWPERoot);
								PreviewWPERoot = 0;
								PreviewWPERoot = WickedCall_LoadWPE(tmpeleprof->PropertiesVariable.VariableValue[i]);
								void WickedCall_PerformEmitterAction(int iAction, uint32_t emitter_root);
								WickedCall_PerformEmitterAction(1, PreviewWPERoot);
								WickedCall_PerformEmitterAction(4, PreviewWPERoot);
								WickedCall_PerformEmitterAction(5, PreviewWPERoot);
							}
						}
						if (bwpefile)
						{
							ImGui::Separator();
							if (ImGui::Checkbox("Preview", &bPreviewWPE))
							{
								if (PreviewWPERoot == 0 && bPreviewWPE)
								{
									//PE: Load effect.
									//PE: Delete this preview before test game. and set bPreviewWPE = false
									PreviewWPERoot = WickedCall_LoadWPE(tmpeleprof->PropertiesVariable.VariableValue[i]);
									//iAction = 1 Burst all. 2 = Pause. - 3 = Resume. - 4 = Restart - 5 - visible - 6 = not visible. - 7 = pause emit - 8 = resume emit
									void WickedCall_PerformEmitterAction(int iAction, uint32_t emitter_root);
									WickedCall_PerformEmitterAction(1, PreviewWPERoot);
									WickedCall_PerformEmitterAction(4, PreviewWPERoot);
									WickedCall_PerformEmitterAction(5, PreviewWPERoot);
								}
								if (PreviewWPERoot != 0 && !bPreviewWPE)
								{
									//PE: Delete effects.
									WickedCall_PerformEmitterAction(6, PreviewWPERoot);
									void DeleteEmitterEffects(uint32_t root);
									DeleteEmitterEffects(PreviewWPERoot);
									PreviewWPERoot = 0;
								}
							}
							ImGui::SameLine();
							if (ImGui::Button("Burst##Burst", ImVec2(60, 0)))
							{
								WickedCall_PerformEmitterAction(1, PreviewWPERoot);
							}
							ImGui::Separator();
						}
					}
				}
				else if (tmpvar == "video1" || tmpvar == "video 1") 
				{
					cstr tmpvalue = tmpeleprof->soundset1_s;

					#define VIDEOFILEID (PROPERTIES_CACHE_ICONS+997)
					static cstr videofile = "";
					static int videofile_preview_id = 0;
					if (tmpvalue != videofile)
					{
						//Load new image preview.
						videofile_preview_id = 0;
						if (tmpvalue != "")
						{
							videofile_preview_id = VIDEOFILEID;

							std::string stmp = tmpvalue.Get();
							replaceAll(stmp, "videobank", ""); //Video thumbs stored without videobank.
							replaceAll(stmp, "\\\\", "\\"); //Remove double backslash.

							bool CreateBackBufferCacheName(char *file, int width, int height);
							extern cstr BackBufferCacheName;
							CreateBackBufferCacheName( (char *) stmp.c_str(), 512, 288);
							GG_SetWritablesToRoot(true);
							SetMipmapNum(1); //PE: mipmaps not needed.
							image_setlegacyimageloading(true);
							if (FileExist(BackBufferCacheName.Get()))
							{
								LoadImage((char *)BackBufferCacheName.Get(), videofile_preview_id);
							}
							image_setlegacyimageloading(false);
							SetMipmapNum(-1); //PE: mipmaps not needed.
							GG_SetWritablesToRoot(false);
							if (!GetImageExistEx(VIDEOFILEID))
							{
								videofile_preview_id = 0;
							}
						}
						videofile = tmpvalue;
					}


					tmpvalue = imgui_setpropertyfile2(1, tmpeleprof->soundset1_s.Get(), "", "Specify movie file you would like to play when the player enters the zone", "videobank\\");
					if (tmpvalue != tmpeleprof->soundset1_s) 
					{
						tmpeleprof->soundset1_s = tmpvalue;
						bUpdateMainString = true;
					}

					if (videofile_preview_id > 0 && GetImageExistEx(videofile_preview_id))
					{
						extern ImVec4 drawCol_back;
						extern ImVec4 drawCol_normal;
						extern ImVec4 drawCol_hover;
						extern ImVec4 drawCol_Down;

						float w = ImGui::GetContentRegionAvailWidth();
						float iwidth = w;
						float ImgW = ImageWidth(videofile_preview_id);
						float ImgH = ImageHeight(videofile_preview_id);
						float fHighRatio = ImgH / ImgW;
						ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((w*0.5) - (iwidth*0.5), 0.0f));
						ImGui::ImgBtn(videofile_preview_id, ImVec2(iwidth - 18.0f, (iwidth - 18.0f) * fHighRatio), drawCol_back, drawCol_normal, drawCol_normal, drawCol_normal, -1, 0, 0, 0, true);
					}

				}
				else 
				{
					if (ImGui::InputText("##string", &tmpeleprof->PropertiesVariable.VariableValue[i][0], MAXVARIABLESIZE)) 
					{
						bUpdateMainString = true;
					}
					if (ImGui::MaxIsItemFocused()) bImGuiGotFocus = true;

				}
			}
			else if (tmpeleprof->PropertiesVariable.VariableType[i] == 3) 
			{
				//Bool
				bool tmpbool = atoi(val.Get());
				if( ImGui::Checkbox("##bool",&tmpbool) ) 
				{
					if(tmpbool)
						strcpy(tmpeleprof->PropertiesVariable.VariableValue[i], "1");
					else
						strcpy(tmpeleprof->PropertiesVariable.VariableValue[i], "0");
					bUpdateMainString = true;
				}
				// hack in tooltip for now, better idea is adding verbose to LUA itself!
				LPSTR pTooltipForTickbox = NULL;
				if (stricmp (tmpeleprof->PropertiesVariable.Variable[i], "FollowAPath") == NULL) pTooltipForTickbox = "Tick to all character to find and follow a nearby flag path";
				if (stricmp (tmpeleprof->PropertiesVariable.Variable[i], "CanRetreat") == NULL) pTooltipForTickbox = "Tick to allow the character to retreat when hurt";
				if (stricmp (tmpeleprof->PropertiesVariable.Variable[i], "StandGround") == NULL) pTooltipForTickbox = "Tick to ensure the character stays near their starting position";
				if (stricmp (tmpeleprof->PropertiesVariable.Variable[i], "FlankTarget") == NULL) pTooltipForTickbox = "Tick to allow the character to wide flank their target";
				if (stricmp (tmpeleprof->PropertiesVariable.Variable[i], "Alerted") == NULL) pTooltipForTickbox = "Tick to skip the initial suspicion and go direct to full alert";
				if (stricmp (tmpeleprof->PropertiesVariable.Variable[i], "Feeding") == NULL) pTooltipForTickbox = "Tick to start the zombie off in a ground feeding state";
				if (pTooltipForTickbox && ImGui::IsItemHovered()) ImGui::SetTooltip(pTooltipForTickbox);
			}
			else if (tmpeleprof->PropertiesVariable.VariableType[i] == 4 || tmpeleprof->PropertiesVariable.VariableType[i] == 7)
			{
				// Dropdown of labelled integers.
				std::vector<std::string> labels;
				bool bGotLabels = false;
				bool bAsString = false;
				if (tmpeleprof->PropertiesVariable.VariableType[i] == 7)
					bAsString = true;

				// Retrieve the labels for this dropdown.
				for (int l = 0; l < luadropdownlabels.size(); l++)
				{
					labels = luadropdownlabels[l];

					// The first element is the variable number.
					if (atol(labels[0].c_str()) == i)
					{
						bGotLabels = true;
						break;
					}
				}
				if (bGotLabels)
				{
					// type of dropdown
					bool bIsAQuestList = false;
					int iQuestIndex = 0;

					// Determine the label for the currently selected value.
					int iSelectedIndex = 0;
					const char* preview = "";
					if (bAsString)
					{
						preview = tmpeleprof->PropertiesVariable.VariableValue[i];
					}
					else
					{
						iSelectedIndex = atol(tmpeleprof->PropertiesVariable.VariableValue[i]) - tmpeleprof->PropertiesVariable.VariableValueFrom[i];
						// Since the first element of the labels is the variable number, add 1.
						iSelectedIndex++;
						// Ensure preview for combo is always valid
						if (iSelectedIndex < labels.size()) preview = labels[iSelectedIndex].c_str();
					}

					// No combi if no choices
					if (tmpeleprof->PropertiesVariable.VariableValueTo[i] == 0 && tmpeleprof->PropertiesVariable.VariableValueFrom[i] == 0)
					{
						// Indicate no choices (no drop down list or animations)
						ImGui::TextCenter("No choices available");
					}
					else
					{
						// Create combo now
						if (ImGui::BeginCombo("##DLUACOMBO", preview))
						{
							for(int j = 1; j < labels.size(); j++)
							{
								bool bSelected = false;
								char labelID[128];
								int iValue = tmpeleprof->PropertiesVariable.VariableValueFrom[i] + j;
								sprintf(labelID, "%d", iValue-1);

								char label[128];
								strcpy_s(label, 127,  labels[j].c_str());
								label[127] = 0;

								if (bAsString)
								{
									if (strcmp(label, tmpeleprof->PropertiesVariable.VariableValue[i]) == NULL)
										bSelected = true;
								}
								else
								{
									if (strcmp(labelID, tmpeleprof->PropertiesVariable.VariableValue[i]) == NULL)
										bSelected = true;
								}
							
								if (ImGui::Selectable(label, bSelected))
								{
									if (bAsString)
									{
										strcpy(tmpeleprof->PropertiesVariable.VariableValue[i], label);
										if (strcmp(tmpeleprof->PropertiesVariable.VariableValue[i], "None") == 0)
											strcpy(tmpeleprof->PropertiesVariable.VariableValue[i], "");									
									}
									else
										strcpy(tmpeleprof->PropertiesVariable.VariableValue[i], labelID);

									bUpdateMainString = true;
								}
								if (bSelected) ImGui::SetItemDefaultFocus();
							}
							ImGui::EndCombo();
						}
					}

					// additional option to create a new quest under Quest Choice
					bIsAQuestList = false;
					for (int v = 0; v < tmpeleprof->PropertiesVariable.iVariables; v++)
					{
						if (strstr(tmpeleprof->PropertiesVariable.Variable[v], "QuestChoice") != NULL)
						{
							bIsAQuestList = true;
							break;
						}
					}
					if (bIsAQuestList == true)
					{
						if (bAsString)
						{
							//PE: Locate index.
							for (int v = 0; v < g_collectionQuestList.size(); v++)
							{
								if (stricmp(g_collectionQuestList[v].collectionFields[0].Get(), tmpeleprof->PropertiesVariable.VariableValue[i]) == 0)
								{
									iQuestIndex = v + 2;
									break;
								}
							}
							if (iQuestIndex == 0)
							{
								//PE: Default to "none"
								if (pestrcasestr(tmpeleprof->aimain_s.Get(), "quest_poster") ||
									pestrcasestr(tmpeleprof->aimain_s.Get(), "quest_giver"))
								{
									iQuestIndex = 1;
								}
							}
						}
						else
						{
							iQuestIndex = atoi(tmpeleprof->PropertiesVariable.VariableValue[i]);
						}
						if (bUpdateMainString == true)
						{
							if (iQuestIndex == 1)
							{
								tmpeleprof->name_s = "Quest Giver";
							}
							if (iQuestIndex >= 2 && iQuestIndex <= 1 + g_collectionQuestList.size())
							{
								// if quest list selection, change the object name to identify the quest chosen
								tmpeleprof->name_s = g_collectionQuestList[iQuestIndex - 2].collectionFields[0];
							}
						}
					}
					bool bBelowQuestCombo = false;
					if (strstr(tmpeleprof->PropertiesVariable.Variable[i], "QuestChoice") != NULL)
					{
						bBelowQuestCombo = true;
					}
					if (bBelowQuestCombo && bIsAQuestList == true && iQuestIndex > 0)
					{
						bool bDoARefresh = false;
						float but_gadget_size = ImGui::GetFontSize() * 12.0;
						float w = ImGui::GetWindowContentRegionWidth() - 10.0;
						ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((w * 0.5) - (but_gadget_size * 0.5), 0.0f));
						if (iQuestIndex == 1)
						{
							// prescan to ensure name unique
							bool bFoundMatch = false;
							for (int q = 0; q < g_collectionQuestList.size(); q++)
							{
								if (stricmp(g_collectionQuestList[q].collectionFields[0].Get(), tmpeleprof->name_s.Get()) == NULL)
								{
									bFoundMatch = true;
								}
							}

							if (ImGui::StyleButton("Quest Editor", ImVec2(but_gadget_size, 0)))
							{
								void CloseAllOpenTools(bool bTerrainTools = true);
								CloseAllOpenTools();
								extern std::vector<collectionQuestType> g_collectionQuestList_backup;
								g_collectionQuestList_backup.clear();
								extern bool bQuestEditor_Window;
								bQuestEditor_Window = true;
							}
							if (ImGui::IsItemHovered())
							{
								ImGui::SetTooltip("Open Quest Editor");
							}
							//PE: Now use "Quest Editor"
							/*
							LPSTR pCreateButtonLabel = "Create New Quest";
							if (ImGui::StyleButton(pCreateButtonLabel, ImVec2(but_gadget_size, 0)))
							{
								collectionQuestType item;
								fill_rpg_quest_defaults(&item, tmpeleprof->name_s.Get());

								//PE: Just to stop crash if not using storyboard.
								if (g_collectionQuestLabels.size() == 0)
								{
									bFoundMatch = false;
								}
								// only add unique quest titles
								if (bFoundMatch == false)
								{
									// add unique to quest list
									g_collectionQuestList.push_back(item);
									int iQuestIndexAdded = 1 + g_collectionQuestList.size();

									// inject into behaviour choice so can reflect as existing
									for (int n = 0; n < tmpeleprof->PropertiesVariable.iVariables; n++)
									{
										if (pestrcasestr(tmpeleprof->PropertiesVariable.Variable[n], "QuestChoice"))
										{
											sprintf(tmpeleprof->PropertiesVariable.VariableValue[n], "%d", iQuestIndexAdded);
											break;
										}
									}

									// refresh behaviour and the quest dropdown
									bDoARefresh = true;
								}
							}
							if (ImGui::IsItemHovered())
							{
								if(bFoundMatch==true)
									ImGui::SetTooltip("This quest name already exists in the main quest list!");
								else
									ImGui::SetTooltip("Use the name of this object to add a new quest to the main quest list");
							}
							*/
						}
						else
						{
							if (ImGui::StyleButton("Quest Editor", ImVec2(but_gadget_size, 0)))
							{
								extern int current_quest_selection; //iQuestIndex - 1
								int iIndex = iQuestIndex - 2;
								if (iIndex >= 0 && iIndex < g_collectionQuestList.size())
									current_quest_selection = iIndex;

								void CloseAllOpenTools(bool bTerrainTools = true);
								CloseAllOpenTools();
								extern std::vector<collectionQuestType> g_collectionQuestList_backup;
								g_collectionQuestList_backup.clear();
								extern bool bQuestEditor_Window;
								bQuestEditor_Window = true;
							}
							if (ImGui::IsItemHovered())
							{
								ImGui::SetTooltip("Open Quest Editor");
							}

							ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((w * 0.5) - (but_gadget_size * 0.5), 0.0f));

							LPSTR pCreateButtonLabel = "Delete Quest";
							if (ImGui::StyleButton(pCreateButtonLabel, ImVec2(but_gadget_size, 0)))
							{
								int iAction = askBoxCancel("This will delete the current selected quest. Are you sure ?", "Quest Confirmation"); //1==Yes 2=Cancel 0=No
								if (iAction == 1)
								{
									// create new quest list without the one deleted
									std::vector<collectionQuestType> newCollectionQuestList;
									for (int q = 0; q < g_collectionQuestList.size(); q++)
									{
										if (q != iQuestIndex - 2)
										{
											newCollectionQuestList.push_back(g_collectionQuestList[q]);
										}
									}
									g_collectionQuestList = newCollectionQuestList;
									sprintf(tmpeleprof->PropertiesVariable.VariableValue[i], "%d", 1);
									bDoARefresh = true;
								}
							}
							if (ImGui::IsItemHovered()) ImGui::SetTooltip("Delete this quest from the main quest list of the game project");
						}
						if (bDoARefresh == true)
						{
							// refresh behaviour and the quest dropdown
							extern int fpe_current_loaded_script;
							extern bool g_bChangedGameCollectionList;
							fpe_current_loaded_script = -1;
							g_bChangedGameCollectionList = true;
							bUpdateMainString = true;
						}
					}

					// User needs to select a non-default next level.
					if (preview && strcmp(preview, "Go to Specific Level") == 0)
					{
						extern StoryboardStruct Storyboard;
						char title[MAX_PATH];
						if (strlen(tmpeleprof->ifused_s.Get()) > 0)
						{
							// Remove mapbank\ and .fpm from the level name.
							int offset = 0;
							if (strstr(tmpeleprof->ifused_s.Get(), "mapbank"))
								offset = 8;
							strcpy(title, tmpeleprof->ifused_s.Get() + offset);
							title[strlen(title) - 4] = 0;
						}
						else
							strcpy(title, "Select a level from your game project...");

						if (ImGui::BeginCombo("##LuaLevelCombo", title))
						{
							for (int i = 0; i < STORYBOARD_MAXNODES; i++)
							{
								if (Storyboard.Nodes[i].used && strlen(Storyboard.Nodes[i].level_name) > 0)
								{
									title[0] = 0;
									int offset = 0;
									if (strstr(Storyboard.Nodes[i].level_name, "mapbank"))
										offset = 8;
									strcpy(title, Storyboard.Nodes[i].level_name + offset);
									title[strlen(title) - 4] = 0;
									ImGui::PushID(91679 + i);
									if (ImGui::Selectable(title))
									{
										strcpy(tmpeleprof->ifused_s.Get(), Storyboard.Nodes[i].level_name);
									}
									ImGui::PopID();
								}
							}
							ImGui::EndCombo();
						}
					}
					else if (preview && stricmp(preview, "Use Storyboard Logic") == 0)
					{
						// Ensure that ifused_s gets wiped so that the next level in storyboard logic will be used, instead of specific level
						strcpy(tmpeleprof->ifused_s.Get(), "");
					}
				}
			}
			else if (tmpeleprof->PropertiesVariable.VariableType[i] == 5)
			{
				// integer - but converts seconds (from user) to milliseconds in actual stored value
				int tmpint = atoi(val.Get()) / 1000;
				LPSTR pTooltipForIntegerSlider = "Use slider to set the desired value.";
				if (stricmp (tmpeleprof->PropertiesVariable.Variable[i], "CombatTime") == NULL) pTooltipForIntegerSlider = "Set the time in seconds for how long the character stays in combat mode before returning to base";
				cstr id = cstr("##") + tmpeleprof->PropertiesVariable.VariableScript/*tmpeleprof->name_s*/ + cstr(tmpeleprof->PropertiesVariable.Variable[i]);
				if (tmpeleprof->PropertiesVariable.VariableValueFrom[i] > 0 && tmpeleprof->PropertiesVariable.VariableValueTo[i] > 0 && tmpeleprof->PropertiesVariable.VariableValueTo[i] > tmpeleprof->PropertiesVariable.VariableValueFrom[i])
				{
					if (ImGui::MaxSliderInputInt(id.Get(), &tmpint, (int)tmpeleprof->PropertiesVariable.VariableValueFrom[i], (int)tmpeleprof->PropertiesVariable.VariableValueTo[i], pTooltipForIntegerSlider))
					{
						sprintf(tmp, "%d", tmpint * 1000);
						strcpy(tmpeleprof->PropertiesVariable.VariableValue[i], tmp);
						bUpdateMainString = true;
					}
				}
				else
				{
					if (ImGui::MaxSliderInputInt(id.Get(), &tmpint, 0.0F, 100.0f, pTooltipForIntegerSlider))
					{
						sprintf(tmp, "%d", tmpint * 1000);
						strcpy(tmpeleprof->PropertiesVariable.VariableValue[i], tmp);
						bUpdateMainString = true;
					}
				}
			}
			else if (tmpeleprof->PropertiesVariable.VariableType[i] == 6)
			{
				// Should alter the matching eleprof variable value e.g. in script "&QUANTITY" is eleprof->quantity
				
				// Determine the index into g_DLuaVariableNames for this properties variable.
				int variableNameIndex = -1;
				for (int j = 0; j < MAXPROPERTIESVARIABLES; j++)
				{
					if (tmpeleprof->PropertiesVariable.VariableType[j] == 6)
					{
						variableNameIndex++;
						if (j == i)
							break;
					}
				}

				if (variableNameIndex >= 0 && variableNameIndex < g_DLuaVariableNames.size())
				{
					// Find the matching eleprof variable that we want to edit.
					std::string variableName = g_DLuaVariableNames[variableNameIndex];
					void* pVariable = nullptr;

					enum VariableType
					{
						NONE,
						INT,
						FLOAT,
						STRING,
						BOOL
					};
					VariableType type = NONE;
					
					if (stricmp(variableName.c_str(), "quantity") == 0)
					{
						type = INT;
						pVariable = &tmpeleprof->quantity;
					}
					else if (stricmp(variableName.c_str(), "damage") == 0)
					{
						type = INT;
						pVariable = &tmpeleprof->damage;
					}
					else if (stricmp(variableName.c_str(), "accuracy") == 0)
					{
						type = INT;
						pVariable = &tmpeleprof->accuracy;
					}
					else if (stricmp(variableName.c_str(), "reloadqty") == 0)
					{
						type = INT;
						pVariable = &tmpeleprof->reloadqty;
					}
					else if (stricmp(variableName.c_str(), "fireiterations") == 0)
					{
						type = INT;
						pVariable = &tmpeleprof->fireiterations;
					}
					else if (stricmp(variableName.c_str(), "range") == 0)
					{
						type = INT;
						pVariable = &tmpeleprof->range;
					}
					else if (stricmp(variableName.c_str(), "dropoff") == 0)
					{
						type = INT;
						pVariable = &tmpeleprof->dropoff;
					}
					else if (stricmp(variableName.c_str(), "clipcapacity") == 0)
					{
						type = INT;
						pVariable = &tmpeleprof->clipcapacity;
					}
					else if (stricmp(variableName.c_str(), "weaponpropres1") == 0)
					{
						type = INT;
						pVariable = &tmpeleprof->weaponpropres1;
					}
					else if (stricmp(variableName.c_str(), "weaponpropres2") == 0)
					{
						type = INT;
						pVariable = &tmpeleprof->weaponpropres2;
					}

					// Display the correct ImGui gadget based on the variable type.
					switch (type)
					{
					case INT:
					{
						int* pEditVariable = (int*)pVariable;
						if (pEditVariable)
						{
							// Choose a range for the slider (1,100 if not specified in script).
							int from = 1;
							int to = 100;
							int scriptFrom = tmpeleprof->PropertiesVariable.VariableValueFrom[i];
							int scriptTo = tmpeleprof->PropertiesVariable.VariableValueTo[i];
							if (scriptFrom != scriptTo)
							{
								from = scriptFrom;
								to = scriptTo;
							}

							// Edit the variable via ImGui
							cstr sliderLabel = cstr("##") + cstr((char*)variableName.c_str());
							ImGui::MaxSliderInputInt(sliderLabel.Get(), pEditVariable, from, to, 0);
						}
						break;
					}
					case FLOAT:
						break;
					case STRING:
						break;
					case BOOL:
						break;
					}
				}
			}
			else 
			{
				//Integer
				int tmpint = atoi(val.Get());
				// hack in tooltip for now, better idea is adding verbose to LUA itself!
				LPSTR pTooltipForIntegerSlider = "Use slider to set the desired value.";
				if (stricmp (tmpeleprof->PropertiesVariable.Variable[i], "RetreatRange") == NULL) pTooltipForIntegerSlider = "Set the distance the character will retreat to before stopping";
				if (stricmp (tmpeleprof->PropertiesVariable.Variable[i], "ChaseModes") == NULL) pTooltipForIntegerSlider = "The first three modes are slow walkers and the last two are fast walkers";
				cstr id = cstr("##") + tmpeleprof->PropertiesVariable.VariableScript/*tmpeleprof->name_s*/ + cstr(tmpeleprof->PropertiesVariable.Variable[i]);

				// strange condition to enable correct integer slider - from can be zero just fine
				//if (tmpeleprof->PropertiesVariable.VariableValueFrom[i] != 0 && tmpeleprof->PropertiesVariable.VariableValueTo[i] != 0 && tmpeleprof->PropertiesVariable.VariableValueTo[i] > tmpeleprof->PropertiesVariable.VariableValueFrom[i])
				if (tmpeleprof->PropertiesVariable.VariableValueTo[i] != 0 && tmpeleprof->PropertiesVariable.VariableValueTo[i] > tmpeleprof->PropertiesVariable.VariableValueFrom[i])
				{
					if (ImGui::MaxSliderInputInt(id.Get(), &tmpint, (int)tmpeleprof->PropertiesVariable.VariableValueFrom[i], (int)tmpeleprof->PropertiesVariable.VariableValueTo[i], pTooltipForIntegerSlider))
					{
						sprintf(tmp, "%d", tmpint);
						strcpy(tmpeleprof->PropertiesVariable.VariableValue[i], tmp);
						bUpdateMainString = true;
					}
				}
				else
				{
					if (ImGui::MaxSliderInputInt(id.Get(), &tmpint, 0.0F, 100.0f, pTooltipForIntegerSlider))
					{
						sprintf(tmp, "%d", tmpint);
						strcpy(tmpeleprof->PropertiesVariable.VariableValue[i], tmp);
						bUpdateMainString = true;
					}
				}
				if (bwpefile)
				{
					if (pestrcasestr(tmpeleprof->PropertiesVariable.Variable[i], "offsety"))
					{
						bwpeyoffet = true;
						fPreviewYOffset = tmpint;
					}
				}

			}
			ImGui::PopItemWidth();
			ImGui::PopID();
		}
	}
	
	if(!bwpeyoffet)
		fPreviewYOffset = 0;

	//Update soundset4_s when we have changes.
	if (bUpdateMainString) 
	{
		cstr sLuaScriptName = tmpeleprof->PropertiesVariable.VariableScript;
		sLuaScriptName += "_properties(";
		//Check if we need to update with new default values.
		if (tmpeleprof->PropertiesVariable.iVariables > 0) 
		{
			tmpeleprof->soundset4_s = sLuaScriptName;
			//Add variables.
			for (int i = 0; i < tmpeleprof->PropertiesVariable.iVariables; i++) 
			{
				char val[3];
				val[0] = tmpeleprof->PropertiesVariable.VariableType[i] + '0';
				val[1] = 0;

				tmpeleprof->soundset4_s += val;
				tmpeleprof->soundset4_s += "\"";
				std::string clean_string = tmpeleprof->PropertiesVariable.VariableValue[i];
				replaceAll(clean_string, "\"", ""); //cant use "
				tmpeleprof->soundset4_s += (char *)clean_string.c_str();
				//tmpeleprof->soundset4_s += tmpeleprof->PropertiesVariable.VariableValue[i];
				tmpeleprof->soundset4_s += "\"";
				if (i < tmpeleprof->PropertiesVariable.iVariables - 1)
					tmpeleprof->soundset4_s += ",";
			}
			tmpeleprof->soundset4_s += ")";
		}
	}

	return speech_entries;
}


int DisplayLuaDescriptionOnly(entityeleproftype *tmpeleprof)
{
	char tmp[2048];
	bool bUpdateMainString = false;
	int speech_entries = 0;
	float fPropertiesColoumWidth = ImGui::GetCursorPosX() + 110.0f;

	ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;

	if (1) {
		ImGui::PushStyleColor(ImGuiCol_ChildBg, style.Colors[ImGuiCol_FrameBg]);
		ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 4.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, style.FrameBorderSize);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, style.FramePadding);

		bool ret;
		if (fDescriptionBoxSize > ImGui::GetFontSize() * 7) {
			ret = ImGui::BeginChild("##DLUADescriptionbox", ImVec2(ImGui::GetContentRegionAvailWidth() - 10, ImGui::GetFontSize() * 7), true, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysUseWindowPadding);
		}
		else {
			if (fDescriptionBoxSize == 0)
				ret = ImGui::BeginChild("##DLUADescriptionbox", ImVec2(ImGui::GetContentRegionAvailWidth() - 10, ImGui::GetFontSize() * 5), true, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysUseWindowPadding | ImGuiWindowFlags_NoScrollbar);
			else
				ret = ImGui::BeginChild("##DLUADescriptionbox", ImVec2(ImGui::GetContentRegionAvailWidth() - 10, fDescriptionBoxSize), true, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysUseWindowPadding | ImGuiWindowFlags_NoScrollbar);
		}
		void fDisplayDescriptionBox(entityeleproftype *tmpeleprof, bool textonly = false);
		fDisplayDescriptionBox(tmpeleprof,true);
		ImGui::EndChild();

		ImGui::PopStyleVar(3);
		ImGui::PopStyleColor();
	}
	ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));

	return 0;
}


void fDisplayDescriptionBox(entityeleproftype *tmpeleprof, bool textonly = false)
{
	struct Segment
	{
		Segment(const char* text, ImU32 col = 0, bool underline = false)
			: textStart(text)
			, textEnd(text + strlen(text))
			, colour(col)
			, underline(underline)
		{}

		const char* textStart;
		const char* textEnd;
		ImU32		colour;
		bool		underline;

		Segment() { colour = 0; underline = false; }

	};
	
	Segment segs[MAXPROPERTIESVARIABLES*3] = {};
	int curseg = 0;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	ImVec4 col = style.Colors[ImGuiCol_Text] * ImVec4(255.0f, 255.0f, 255.0f, 255.0f);

	if (textonly) 
	{
		segs[curseg++] = Segment(tmpeleprof->PropertiesVariable.VariableDescription.Get());
	}
	else 
	{
		for (int i = 0; i < tmpeleprof->PropertiesVariable.iVariables; i++)
		{
			//segs[curseg++] = Segment(tmpeleprof->PropertiesVariable.VariableSectionDescription[i]);
			segs[curseg++] = Segment(tmpeleprof->PropertiesVariable.VariableSectionDescription[i].Get());
			segs[curseg++] = Segment(tmpeleprof->PropertiesVariable.Variable[i], IM_COL32(col.x, col.y, col.z, col.w), true);
			//if (strlen(tmpeleprof->PropertiesVariable.VariableSectionEndDescription[i]) > 0) 
			//{
			//	segs[curseg++] = Segment(tmpeleprof->PropertiesVariable.VariableSectionEndDescription[i]);
			//}
			if (tmpeleprof->PropertiesVariable.VariableSectionEndDescription[i].Len() > 0)
			{
				segs[curseg++] = Segment(tmpeleprof->PropertiesVariable.VariableSectionEndDescription[i].Get());
			}
			if (curseg >= (MAXPROPERTIESVARIABLES * 3) - 1)
				break;
		}
	}
	float startpos = ImGui::GetCursorPosY();

	const float wrapWidth = ImGui::GetWindowContentRegionWidth();
	for (int i = 0; i < curseg; ++i)
	{
		const char* textStart = segs[i].textStart;
		const char* textEnd = segs[i].textEnd ? segs[i].textEnd : textStart + strlen(textStart);

		ImFont* Font = ImGui::GetFont();

		do
		{
			float widthRemaining = ImGui::CalcWrapWidthForPos(ImGui::GetCursorScreenPos(), 0.0f);
			widthRemaining *= 1.65;
			const char* drawEnd = ImGui::CalcWordWrapPositionB(1.0f, textStart, textEnd, wrapWidth, wrapWidth - widthRemaining); //, wrapWidth - widthRemaining);
			if (textStart == drawEnd)
			{
				ImGui::NewLine();
				drawEnd = ImGui::CalcWordWrapPositionB(1.0f, textStart, textEnd, wrapWidth, wrapWidth - widthRemaining); //, wrapWidth - widthRemaining);
			}

			if (segs[i].colour)
				ImGui::PushStyleColor(ImGuiCol_Text, segs[i].colour);
			ImGui::TextUnformatted(textStart, textStart == drawEnd ? nullptr : drawEnd);
			if (segs[i].colour)
				ImGui::PopStyleColor();
			if (segs[i].underline)
			{
				ImVec2 lineEnd = ImGui::GetItemRectMax();
				ImVec2 lineStart = lineEnd;
				lineStart.x = ImGui::GetItemRectMin().x;
				ImGui::GetWindowDrawList()->AddLine(lineStart, lineEnd, segs[i].colour);

				//if (ImGui::IsItemHovered(ImGuiHoveredFlags_RectOnly))
				//	ImGui::SetMouseCursor(ImGuiMouseCursor_TextInput);
			}

			if (textStart == drawEnd || drawEnd == textEnd)
			{
				ImGui::SameLine(0.0f, 0.0f);
				break;
			}

			textStart = drawEnd;

			while (textStart < textEnd)
			{
				const char c = *textStart;
				if (ImCharIsBlankA(c)) { textStart++; }
				else if (c == '\n') { textStart++; break; }
				else { break; }
			}
		} while (true);
	}

	fDescriptionBoxSize = (ImGui::GetCursorPosY() - startpos) + (ImGui::GetFontSize()*2.0);

}

float fDisplaySegmentText(char *text)
{
	if (!text) return(0);
	struct Segment
	{
		Segment(const char* text, ImU32 col = 0, bool underline = false)
			: textStart(text)
			, textEnd(text + strlen(text))
			, colour(col)
			, underline(underline)
		{}

		const char* textStart;
		const char* textEnd;
		ImU32		colour;
		bool		underline;

		Segment() { colour = 0; underline = false; }

	};

	Segment segs[MAXPROPERTIESVARIABLES * 3] = {};
	int curseg = 0;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	ImVec4 col = style.Colors[ImGuiCol_Text] * ImVec4(255.0f, 255.0f, 255.0f, 255.0f);

	segs[curseg++] = Segment(text);
	
	float startpos = ImGui::GetCursorPosY();

	const float wrapWidth = ImGui::GetWindowContentRegionWidth();
	for (int i = 0; i < curseg; ++i)
	{
		const char* textStart = segs[i].textStart;
		const char* textEnd = segs[i].textEnd ? segs[i].textEnd : textStart + strlen(textStart);

		ImFont* Font = ImGui::GetFont();

		do
		{
			float widthRemaining = ImGui::CalcWrapWidthForPos(ImGui::GetCursorScreenPos(), 0.0f);
			widthRemaining *= 1.65;
			const char* drawEnd = ImGui::CalcWordWrapPositionB(1.0f, textStart, textEnd, wrapWidth, wrapWidth - widthRemaining); //, wrapWidth - widthRemaining);
			if (textStart == drawEnd)
			{
				ImGui::NewLine();
				drawEnd = ImGui::CalcWordWrapPositionB(1.0f, textStart, textEnd, wrapWidth, wrapWidth - widthRemaining); //, wrapWidth - widthRemaining);
			}

			if (segs[i].colour)
				ImGui::PushStyleColor(ImGuiCol_Text, segs[i].colour);
			ImGui::TextUnformatted(textStart, textStart == drawEnd ? nullptr : drawEnd);
			if (segs[i].colour)
				ImGui::PopStyleColor();
			if (segs[i].underline)
			{
				ImVec2 lineEnd = ImGui::GetItemRectMax();
				ImVec2 lineStart = lineEnd;
				lineStart.x = ImGui::GetItemRectMin().x;
				ImGui::GetWindowDrawList()->AddLine(lineStart, lineEnd, segs[i].colour);
			}

			if (textStart == drawEnd || drawEnd == textEnd)
			{
				ImGui::SameLine(0.0f, 0.0f);
				break;
			}

			textStart = drawEnd;

			while (textStart < textEnd)
			{
				const char c = *textStart;
				if (ImCharIsBlankA(c)) { textStart++; }
				else if (c == '\n') { textStart++; break; }
				else { break; }
			}
		} while (true);
	}
	return( (ImGui::GetCursorPosY() - startpos) + (ImGui::GetFontSize()*2.0) );
}

void UniversalKeyboardShortcutAddItem(int iIconID, int iIcon2ID, LPSTR pLabel)
{
	ImGui::Indent(10);
	// standard spacing settings
	int iRightColumn = 70;
	int iKeyIconSize = 26;
	float fShortcutTextSpacing = 5.0f;
	float fShortcutVerticalSpacing = -10.0f;

	if (iIconID > 0)
	{
		float fSize = 1.0f;
		ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPos().x - 4.0f, ImGui::GetCursorPos().y));
	

		ImGui::ImgBtn(iIconID, ImVec2(iKeyIconSize*fSize, iKeyIconSize*fSize), ImVec4(0.0, 0.0, 0.0, 0.0), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(1.0, 1.0, 1.0, 1.0), 0, 0, 0, 0, false, false, false, false);
		LPSTR pTooltip = "";
		if (iIcon2ID != 0)
		{
			if (iIconID == KEY_TAB) pTooltip = "Hold down the TAB key";
			if (iIconID == KEY_SHIFT) pTooltip = "Hold down the SHIFT key";
			if (iIconID == KEY_CONTROL) pTooltip = "Hold down the CONTROL key";
			if (iIconID == KEY_CONTROL_SHIFT) pTooltip = "Hold down the CONTROL and SHIFT keys";
			if (iIconID == KEY_ALT) pTooltip = "Hold down the ALT key";
		}
		else
		{
			if (iIconID == KEY_TAB) pTooltip = "Press the TAB key";
			if (iIconID == KEY_SHIFT) pTooltip = "Press the SHIFT key";
			if (iIconID == KEY_CONTROL) pTooltip = "Press the CONTROL key";
			if (iIconID == KEY_CONTROL_SHIFT) pTooltip = "Press the CONTROL and SHIFT keys";
			if (iIconID == KEY_ALT) pTooltip = "Press the ALT key";

		}
		if (iIconID == KEY_KEYBOARD) pTooltip = "Use the WASD or the arrow keys";
		if (iIconID == MOUSE_LMB) pTooltip = "Use the left mouse button";
		if (iIconID == MOUSE_MMB) pTooltip = "Use the mouse wheel";
		if (iIconID == MOUSE_RMB) pTooltip = "Use the right mouse button";
		if (iIconID == KEY_BACKSPACE) pTooltip = "Press the BACKSPACE key";
		if (iIconID == KEY_R) pTooltip = "Press the R key";
		if (iIconID == KEY_Y) pTooltip = "Press the Y key";
		if (iIconID == KEY_F) pTooltip = "Press the F key";
		if (iIconID == KEY_G) pTooltip = "Press the G key";
		if (iIconID == KEY_I) pTooltip = "Press the I key";
		if (iIconID == KEY_N) pTooltip = "Press the N key";
		if (iIconID == KEY_L) pTooltip = "Press the L key";
		if (iIconID == KEY_E) pTooltip = "Press the E key";
		if (iIconID == KEY_Q) pTooltip = "Press the Q key";
		if (iIconID == KEY_SPACE) pTooltip = "Press the SPACE key";
		if (iIconID == KEY_DELETE) pTooltip = "Press the DELETE key";
		if (iIconID == KEY_RETURN) pTooltip = "Press the ENTER key";
		if (iIconID == KEY_PGUP) pTooltip = "Press the PAGEUP key";
		if (iIconID == KEY_PGDN) pTooltip = "Press the PAGEDOWN key";
		if (iIconID == KEY_O) pTooltip = "Press the O key";
		if (iIconID == KEY_T) pTooltip = "Press the T key";
		if (iIconID == KEY_Z) pTooltip = "Press the Z key";
		LPSTR pTooltip2 = "";
		ImGui::SameLine();
		if (iIcon2ID != 0)
		{
			ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPos().x - 14.0f - ((fSize - 1.0f)*4.0f), ImGui::GetCursorPos().y));
			ImGui::ImgBtn(KEY_SEPARATOR_SMALL, ImVec2((float)iKeyIconSize*0.5*fSize, iKeyIconSize*fSize), ImVec4(0.0, 0.0, 0.0, 0.0), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(1.0, 1.0, 1.0, 1.0), 0, 0, 0, 0, false, false, false, false);
			ImGui::SameLine();
			ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPos().x - 14.0f - ((fSize - 1.0f)*4.0f), ImGui::GetCursorPos().y));
			ImGui::ImgBtn(iIcon2ID, ImVec2(iKeyIconSize*fSize, iKeyIconSize*fSize), ImVec4(0.0, 0.0, 0.0, 0.0), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(1.0, 1.0, 1.0, 1.0), 0, 0, 0, 0, false, false, false, false);
			if (iIcon2ID == KEY_PLUS) pTooltip2 = "Press the PLUS key";
			if (iIcon2ID == KEY_MINUS) pTooltip2 = "Press the MINUS key";
			if (iIcon2ID == KEY_SHIFT) pTooltip2 = "Press the SHIFT key";
			if (iIcon2ID == MOUSE_LMB) pTooltip2 = "Use the left mouse button";
			if (iIcon2ID == MOUSE_RMB) pTooltip2 = "Use the right mouse button";
			if (iIcon2ID == KEY_BACKSPACE) pTooltip2 = "Press the BACKSPACE key";
			if (iIcon2ID == KEY_R) pTooltip2 = "Press the R key";
			if (iIcon2ID == KEY_G) pTooltip2 = "Press the G key";
			if (iIcon2ID == KEY_I) pTooltip2 = "Press the I key";
			if (iIcon2ID == KEY_SPACE) pTooltip2 = "Press the SPACE key";
			if (iIcon2ID == KEY_N) pTooltip2 = "Press the N key";
			if (iIcon2ID == KEY_L) pTooltip2 = "Press the L key";
			if (iIcon2ID == KEY_E) pTooltip2 = "Press the E key";
			if (iIcon2ID == KEY_DELETE) pTooltip2 = "Press the DELETE key";
			if (iIcon2ID == KEY_Y) pTooltip2 = "Press the Y key";
			if (iIcon2ID == KEY_RETURN) pTooltip2 = "Press the ENTER key";
			if (iIconID == KEY_O) pTooltip = "Press the O key";
			if (iIconID == KEY_T) pTooltip = "Press the T key";
			if (iIconID == KEY_Z) pTooltip = "Press the Z key";

			ImGui::SameLine();
		}
		int iNewRightColumn = iRightColumn;
		if (fSize > 1.0 && iIcon2ID != 0) iNewRightColumn = iRightColumn + 42.0f;
		ImGui::SetCursorPos(ImVec2(iNewRightColumn, ImGui::GetCursorPos().y + fShortcutTextSpacing));
		//if (bEnlargingKeyIcon == true)
		//{
			cstr pFullLabel = pTooltip;
			if (strlen(pTooltip2) > 0)
			{
				pFullLabel += " and ";
				pFullLabel += pTooltip2;
				//fShortcutVerticalSpacing += 12.0f;
			}
			else
			{
				//fShortcutVerticalSpacing += 8.0f;
			}
			pFullLabel += " to ";
			pFullLabel += pLabel;
			//pFullLabel += ".";
			//ImGui::TextWrapped(pFullLabel.Get());
		//}
		//else
		//{
			ImGui::TextWrapped(pLabel);

			if (ImGui::IsItemHovered()) ImGui::SetTooltip(pFullLabel.Get());
		//}
	}
	else
	{
		// separator
		//ImGui::Text("");
	}
	ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(0, fShortcutVerticalSpacing));
	ImGui::Indent(-10);
}

//void UniversalKeyboardShortcutAddItem(int iIconID, int iIcon2ID, LPSTR pLabel)
//{
//	// standard spacing settings
//	int iRightColumn = 60;
//	int iKeyIconSize = 26;
//	float fShortcutTextSpacing = 5.0f;
//	float fShortcutVerticalSpacing = -10.0f;
//
//	if (iIconID > 0)
//	{
//
//		float fSize = 1.0f;
//		ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPos().x - 4.0f, ImGui::GetCursorPos().y));
//		ImRect bbwin(ImGui::GetWindowPos()+ImGui::GetCursorPos()+ImVec2(0,-ImGui::GetScrollY()), ImGui::GetWindowPos() + ImGui::GetCursorPos() + ImVec2(0, -ImGui::GetScrollY()) + ImVec2(ImGui::GetWindowSize().x,ImGui::GetFontSize()*2.0));
//		// enlarge key icon if hover over it
//		bool bEnlargingKeyIcon = false;
//		if (ImGui::IsMouseHoveringRect(bbwin.Min, bbwin.Max))
//		{
//			//fSize = 1.75f;
//			ImGuiWindow* window = ImGui::GetCurrentWindow();
//			ImVec4 back_col = ImGui::GetStyle().Colors[ImGuiCol_ChildBg];
//			ImVec2 offset = { -10.0f,8.0f };
//			window->DrawList->AddRectFilled(bbwin.Min + offset, bbwin.Max + offset, ImGui::GetColorU32(back_col), 0.0f, 0.0f);
//			//bEnlargingKeyIcon = true;
//		}
//
//		ImGui::ImgBtn(iIconID, ImVec2(iKeyIconSize*fSize, iKeyIconSize*fSize), ImVec4(0.0, 0.0, 0.0, 0.0), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(1.0, 1.0, 1.0, 1.0), 0, 0, 0, 0, false, false, false, false);
//		LPSTR pTooltip = "";
//		if (iIcon2ID != 0)
//		{
//			if (iIconID == KEY_TAB) pTooltip = "Hold down the TAB key";
//			if (iIconID == KEY_SHIFT) pTooltip = "Hold down the SHIFT key";
//			if (iIconID == KEY_CONTROL) pTooltip = "Hold down the CONTROL key";
//			if (iIconID == KEY_CONTROL_SHIFT) pTooltip = "Hold down the CONTROL and SHIFT keys";
//			if (iIconID == KEY_ALT) pTooltip = "Hold down the ALT key";
//		}
//		else
//		{
//			if (iIconID == KEY_TAB) pTooltip = "Press the TAB key";
//			if (iIconID == KEY_SHIFT) pTooltip = "Press the SHIFT key";
//			if (iIconID == KEY_CONTROL) pTooltip = "Press the CONTROL key";
//			if (iIconID == KEY_CONTROL_SHIFT) pTooltip = "Press the CONTROL and SHIFT keys";
//			if (iIconID == KEY_ALT) pTooltip = "Press the ALT key";
//		}
//		if (iIconID == KEY_KEYBOARD) pTooltip = "Use the W, A, S and D keys";
//		if (iIconID == MOUSE_LMB) pTooltip = "Use the left mouse button";
//		if (iIconID == MOUSE_MMB) pTooltip = "Use the mouse wheel";
//		if (iIconID == MOUSE_RMB) pTooltip = "Use the right mouse button";
//		if (iIconID == KEY_BACKSPACE) pTooltip = "Press the BACKSPACE key";
//		if (iIconID == KEY_R) pTooltip = "Press the R key";
//		if (iIconID == KEY_Y) pTooltip = "Press the Y key";
//		if (iIconID == KEY_F) pTooltip = "Press the F key";
//		if (iIconID == KEY_G) pTooltip = "Press the G key";
//		if (iIconID == KEY_N) pTooltip = "Press the N key";
//		if (iIconID == KEY_L) pTooltip = "Press the L key";
//		if (iIconID == KEY_E) pTooltip = "Press the E key";
//		if (iIconID == KEY_SPACE) pTooltip = "Press the SPACE key";
//		if (iIconID == KEY_DELETE) pTooltip = "Press the DELETE key";
//		if (iIconID == KEY_RETURN) pTooltip = "Press the ENTER key";
//		if (iIconID == KEY_PGUP) pTooltip = "Press the PAGEUP key";
//		if (iIconID == KEY_PGDN) pTooltip = "Press the PAGEDOWN key";
//		LPSTR pTooltip2 = "";
//		ImGui::SameLine();
//		if (iIcon2ID != 0)
//		{
//			ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPos().x - 14.0f - ((fSize-1.0f)*4.0f), ImGui::GetCursorPos().y));
//			ImGui::ImgBtn(KEY_SEPARATOR_SMALL, ImVec2((float)iKeyIconSize*0.5*fSize, iKeyIconSize*fSize), ImVec4(0.0, 0.0, 0.0, 0.0), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(1.0, 1.0, 1.0, 1.0), 0, 0, 0, 0, false, false, false, false);
//			ImGui::SameLine();
//			ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPos().x - 14.0f - ((fSize - 1.0f)*4.0f), ImGui::GetCursorPos().y));
//			ImGui::ImgBtn(iIcon2ID, ImVec2(iKeyIconSize*fSize, iKeyIconSize*fSize), ImVec4(0.0, 0.0, 0.0, 0.0), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(1.0, 1.0, 1.0, 1.0), 0, 0, 0, 0, false, false, false, false);
//			if (iIcon2ID == KEY_PLUS) pTooltip2 = "Press the PLUS key";
//			if (iIcon2ID == KEY_MINUS) pTooltip2 = "Press the MINUS key";
//			if (iIcon2ID == KEY_SHIFT) pTooltip2 = "Press the SHIFT key";
//			if (iIcon2ID == MOUSE_LMB) pTooltip2 = "Use the left mouse button";
//			if (iIcon2ID == MOUSE_RMB) pTooltip2 = "Use the right mouse button";
//			if (iIcon2ID == KEY_BACKSPACE) pTooltip2 = "Press the BACKSPACE key";
//			if (iIcon2ID == KEY_R) pTooltip2 = "Press the R key";
//			if (iIcon2ID == KEY_G) pTooltip2 = "Press the G key";
//			if (iIcon2ID == KEY_SPACE) pTooltip2 = "Press the SPACE key";
//			if (iIcon2ID == KEY_N) pTooltip2 = "Press the N key";
//			if (iIcon2ID == KEY_L) pTooltip2 = "Press the L key";
//			if (iIcon2ID == KEY_E) pTooltip2 = "Press the E key";
//			if (iIcon2ID == KEY_DELETE) pTooltip2 = "Press the DELETE key";
//			if (iIcon2ID == KEY_Y) pTooltip2 = "Press the Y key";
//			if (iIcon2ID == KEY_RETURN) pTooltip2 = "Press the ENTER key";
//			ImGui::SameLine();
//		}
//		int iNewRightColumn = iRightColumn;
//		if (fSize > 1.0 && iIcon2ID != 0) iNewRightColumn = iRightColumn + 42.0f;
//		ImGui::SetCursorPos(ImVec2(iNewRightColumn, ImGui::GetCursorPos().y + fShortcutTextSpacing));
//		if (bEnlargingKeyIcon == true)
//		{
//			cstr pFullLabel = pTooltip;
//			if (strlen(pTooltip2) > 0)
//			{
//				pFullLabel += " and ";
//				pFullLabel += pTooltip2;
//				fShortcutVerticalSpacing += 12.0f;
//			}
//			else
//			{
//				fShortcutVerticalSpacing += 8.0f;
//			}
//			pFullLabel += " to ";
//			pFullLabel += pLabel;
//			pFullLabel += ".";
//			ImGui::TextWrapped(pFullLabel.Get());
//		}
//		else
//		{
//			ImGui::TextWrapped(pLabel);
//		}
//	}
//	else
//	{
//		// separator
//		ImGui::Text("");
//	}
//	ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(0, fShortcutVerticalSpacing));
//}

int iContentHeight[eKST_Last]; //PE: Support multiply different panels open at the same time.
float fLastContentHeight[eKST_Last];

void UniversalKeyboardShortcut(eKeyboardShortcutType KST)
{

	if (pref.iHideKeyboardShortcuts)
		return;

	extern int iLastOpenHeader;
	extern bool bStoryboardWindow;
	int iHeader = 30;
	if (KST == eKST_ObjectMode)
		iHeader = 20;

	if (!bStoryboardWindow && pref.bAutoClosePropertySections && iLastOpenHeader != iHeader)
		ImGui::SetNextItemOpen(false, ImGuiCond_Always);

	// called to create Keyboard Shortcut component for all software
	if (ImGui::StyleCollapsingHeader("Keyboard Shortcuts", ImGuiTreeNodeFlags_DefaultOpen))
	{
		iLastOpenHeader = iHeader;

		ImVec2 vChildSize = { 0,0 };
		if (fLastContentHeight[KST] > 20)
			vChildSize.y = fLastContentHeight[KST];

		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0));
		ImGui::BeginChild("##KeyboardShortcutsChild", vChildSize, false, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_None | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);
		ImGui::PopStyleColor();
		iContentHeight[KST] = 0;
		// indent to start list of shortcuts
		ImGui::Indent(10);
		ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(0.0f, 4.0f));

		// record start of keyboard shortcut content
		float fStartOfKeyboardShortcutContent = ImGui::GetCursorPos().y;

		// additional common controls for specific modes
		bool bShowCameraViewShortcuts = false;
		if (KST == eKST_Sculpt || KST == eKST_Paint || KST == eKST_AddVeg || KST == eKST_ObjectMode) bShowCameraViewShortcuts = true;
		if (bShowCameraViewShortcuts == true)
		{
			ImGui::Text("General Shortcuts"); iContentHeight[KST]++;
			//UniversalKeyboardShortcutAddItem(0, 0, ""); iContentHeight[KST]++; //PE: Removed for consistency with other sub sections.
			UniversalKeyboardShortcutAddItem(KEY_KEYBOARD, 0, "Move camera around the map"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_E, 0, "Move camera up"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_Q, 0, "Move camera down"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_SHIFT, 0, "Move camera around quickly"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_CONTROL, 0, "Move camera around slowly"); iContentHeight[KST]++;
			extern bool bEditorInFreeFlightMode;
			if (bEditorInFreeFlightMode == true)
			{
				UniversalKeyboardShortcutAddItem(MOUSE_RMB, 0, "Rotate camera view"); iContentHeight[KST]++;
			}
			UniversalKeyboardShortcutAddItem(MOUSE_MMB, 0, "Zoom in and out"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_O, 0, "Enable object tools"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_T, 0, "Enable terrain tools"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_F, 0, "Toggle camera view"); iContentHeight[KST]++;

		}

		float fSectionSpacer = 8.0f;

		// shortcuts for each type
		switch (KST)
		{
		case eKST_Sculpt:
			if( bShowCameraViewShortcuts ) ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(0.0f, fSectionSpacer));
			ImGui::Text("Sculpt Shortcuts"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(MOUSE_LMB, 0, "Raise and Lower the terrain"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_TAB, 0, "Toggle between raising and lowering"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_PLUS, 0, "Increase the brush size"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_MINUS, 0, "Decrease the brush size"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_CONTROL, MOUSE_MMB, "Increase/Decrease the brush size"); iContentHeight[KST]++;
			break;
		case eKST_Paint:
			if (bShowCameraViewShortcuts) ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(0.0f, fSectionSpacer));
			ImGui::Text("Paint Shortcuts"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(MOUSE_LMB, 0, "Paint and clear terrain texture"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_TAB, 0, "Toggle between painting modes"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_PLUS, 0, "Increase the brush size"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_MINUS, 0, "Decrease the brush size"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_CONTROL, MOUSE_MMB, "Increase/Decrease the brush size"); iContentHeight[KST]++;
			break;
		case eKST_AddVeg:
			if (bShowCameraViewShortcuts) ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(0.0f, fSectionSpacer));
			ImGui::Text("Vegetation Shortcuts"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(MOUSE_LMB, 0, "Add and remove terrain vegetation"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_TAB, 0, "Toggle between adding and removing"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_PLUS, 0, "Increase the brush size"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_MINUS, 0, "Decrease the brush size"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_CONTROL, MOUSE_MMB, "Increase/Decrease the brush size"); iContentHeight[KST]++;
			break;
		case eKST_ObjectMode:
			if (bShowCameraViewShortcuts) ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(0.0f, fSectionSpacer));
			ImGui::Text("Object Tools Shortcuts"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(MOUSE_LMB, 0, "Drag camera around"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_SHIFT, MOUSE_LMB, "Select multiple objects"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_R, 0, "Rotate selected object"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_SHIFT, KEY_R, "Rotate object quickly"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_CONTROL, KEY_R, "Rotate object slowly"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_TAB, 0, "Toggle between positioning modes"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_DELETE, 0, "Delete selected object(s)"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_CONTROL, KEY_Z, "Undo"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_CONTROL, KEY_Y, "Redo"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_Y, 0, "Toggle object static and dynamic"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_G, 0, "Toggle between grid modes"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_I, 0, "Toggle spray objects"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_RETURN, 0, "Place object on nearest surface"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_PGUP, 0, "Move object upward"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_PGDN, 0, "Move object downward"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_CONTROL, KEY_G, "Group selected objects"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_CONTROL_SHIFT, KEY_G, "UnGroup selected objects"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_CONTROL, KEY_L, "Lock/unlock selected object"); iContentHeight[KST]++;

			iContentHeight[KST]++; //Extra for wrapping.
			iContentHeight[KST]++; //Extra for wrapping.
			iContentHeight[KST]++; //Extra for wrapping.
			break;
		case eKST_CharacterCreator:
			if (bShowCameraViewShortcuts) ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(0.0f, fSectionSpacer));
			ImGui::Text("Character Creator Shortcuts"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_KEYBOARD, 0, "Move the camera around"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(MOUSE_RMB, 0, "Look around with the camera"); iContentHeight[KST]++;
			break;
		case eKST_ObjectLibrary:
		case eKST_TerrainGenerator:
			if (bShowCameraViewShortcuts) ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(0.0f, fSectionSpacer));
			if (KST == eKST_TerrainGenerator)
			{
				ImGui::Text("Terrain Generator Shortcuts"); iContentHeight[KST]++;
				UniversalKeyboardShortcutAddItem(KEY_KEYBOARD, 0, "Move camera around the map"); iContentHeight[KST]++;

			}
			else
			{
				ImGui::Text("Object Library Shortcuts"); iContentHeight[KST]++;
			}
			UniversalKeyboardShortcutAddItem(MOUSE_LMB, 0, "Move the terrain preview"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(MOUSE_RMB, 0, "Rotate the terrain preview"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(MOUSE_MMB, 0, "Zoom in and out of the terrain preview"); iContentHeight[KST]++;
			break;
		case eKST_Storyboard:
			if (bShowCameraViewShortcuts) ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(0.0f, fSectionSpacer));
			ImGui::Text("Storyboard Shortcuts"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(MOUSE_LMB, 0, "Drag screen around"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_SHIFT, MOUSE_LMB, "Select multiple nodes"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_CONTROL, KEY_N, "Add new level"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_CONTROL, KEY_L, "Add existing level"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_CONTROL, KEY_SPACE, "Play game"); iContentHeight[KST]++;
			UniversalKeyboardShortcutAddItem(KEY_CONTROL, KEY_E, "Save standalone"); iContentHeight[KST]++;

			break;
		}

		// restore indent
		ImGui::Indent(-10);

		float fShortcutTextSpacing = 6.6f;
		//PE: TODO - We need to calcutate wrapping lines also for this to be precise.
		//PE: We need some additional height when we are using wrapping.
		if(iContentHeight[KST] <= 5)
			fLastContentHeight[KST] = (float) (iContentHeight[KST] + 2) * (ImGui::GetFontSize()+ fShortcutTextSpacing);
		else
			fLastContentHeight[KST] = (float)(iContentHeight[KST] + 5) * (ImGui::GetFontSize() + fShortcutTextSpacing);

		if (bShowCameraViewShortcuts) fLastContentHeight[KST] += fSectionSpacer;

		// finished panel
		ImGui::EndChild();
	}
}

void coreResetIMGUIFunctionalityPrefs(void)
{
	// this is called when MAXVERSION incremented
	// and restores/resets all flags so that each NEW build
	// provides users with a view of the STANDARD USER MOD£
	// not advanced or developer tools modes
	pref.current_style = 25; // 0
	pref.bHideTutorials = false;
	pref.bMultiplyOpenHeaders = false;
	pref.bAutoClosePropertySections = true;
	pref.bDisableMultipleViewport = false;
	pref.bAutoOpenMenuItems = true;
	pref.iAllowUndocking = false;
	pref.iTurnOffUITransparent = false;
	pref.iEnableDragDropEntityMode = 1;
	pref.iEnableAdvancedSky = 0;
	pref.iEnableAdvancedWater = 0;
	pref.iEnableAdvancedPostProcessing = 0;
	pref.iEnableAdvancedShadows = 0;
	pref.iEnableArcRelationshipLines = 1;
	pref.iEnableRelationPopupWindow = 1;// 0; we want you back ;)
	pref.iEnableAxisRotationShortcuts = 0;
	pref.iObjectEnableAdvanced = 0;
	pref.iEnableDragDropWidgetSelect = 0;
	pref.iEnableEditorOutlineSelection = 1;
	pref.iEnableSingleRightPanelAdvanced = 0;
	pref.iDragCameraMovement = 1;
	pref.iGameCreaterStore = 0;
	pref.iFullscreenPreviewAdvanced = 0;
	pref.iDisplayWelcomeScreen = 1;
	pref.iDisplayIntroScreen = 1;
	pref.iEnableAdvancedEntityList = 0;
	pref.fHighLightThickness = 1.0;
	pref.iTurnOffEditboxTooltip = false;
	pref.iImporterDome = 1;
	pref.iEnableAutoExposureInEditor = 0;
	pref.iSetColumnsEntityLib = 3;
	pref.iTerrainAdvanced = 0;
	pref.iStoryboardAdvanced = 0;
	pref.iAdvancedGridModeSettings = 0;// iTerrainDebugMode = 0;
	pref.iEnableAdvancedCharacterCreator = 0;
	pref.iDisableProjectAutoSave = 0;
	pref.iDisableLevelAutoSave = 0;
	pref.iEnableFpsMemMonitor = 0;
	pref.iEnableAutoFlattenSystem = 1;

	for (int i = 0; i < 10; i++)
		pref.iCheckboxFilters[i] = 1;

	//pref.iDisableObjectLibraryViewport = false;
	pref.iLastInStoryboard = false;
	strcpy(pref.cLastUsedStoryboardProject, "");
	pref.changelog_ftime = 0;

	pref.iHideKeyboardShortcuts = 0;
	pref.iEnableDeveloperObjectTools = 0;
	pref.iEnableLevelEditorOpenAndNew = 0;
	pref.iDisplayTerrainGeneratorWelcome = 1;
	pref.iTestGameGraphicsQuality = 2;
	pref.iEnableAutoFlattenSystem = 1;
	pref.iEnableAdvancedGrass = 0;

	pref.iEnableDragDropStopSelectFromInside = 0;
	pref.iGridMode = 0;
	pref.iGridEnabled = 0;
	pref.fEditorGridOffsetX = 50.0f;
	pref.fEditorGridOffsetY = 0.0f;
	pref.fEditorGridOffsetZ = 50.0f;
	pref.fEditorGridSizeX = 100.0f;
	pref.fEditorGridSizeY = 10.0f;
	pref.fEditorGridSizeZ = 100.0f;
	pref.iDevToolsOpen = 0;
	extern int g_iDevToolsOpen;
	g_iDevToolsOpen = 0;
	pref.iCheckFilesModifiedOnFocus = 1;
	pref.status_bar_color = ImVec4((1.0f / 255.0f) * 14, (1.0f / 255.0f) * 99, (1.0f / 255.0f) * 156, 1.0);
	pref.highlight_color = pref.status_bar_color;
}

void DrawRubberBand(float fx, float fy, float fx2, float fy2 )
{
	ImGuiViewport* mainviewport = ImGui::GetMainViewport();
	if (mainviewport)
	{
		ImDrawList* dl = ImGui::GetForegroundDrawList(mainviewport);
		if (dl)
		{
			ImRect bb;
			bb.Min.x = fx;
			bb.Min.y = fy;
			bb.Max.x = fx2;
			bb.Max.y = fy2;
			dl->AddRect(bb.Min,bb.Max, ImGui::GetColorU32(ImVec4(1.0,1.0,1.0,0.8)), 0.0f, ImDrawCornerFlags_None, 3.0f);
		}
	}

}
