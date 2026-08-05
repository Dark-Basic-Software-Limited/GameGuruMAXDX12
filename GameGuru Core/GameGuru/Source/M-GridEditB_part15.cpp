bool bRemoveVideoInNextFrame = false;
int iWelcomeVideoID = -1;
bool bWelcomeVideoMaximized = false;
bool bTriggerEditDemoGame = false;
bool bTriggerPlayDemoGame = false; // 2026-08-05: harness CLICK play_game on the hub

cstr ListboxFilesListForWelcomeScreen(char *currentselection, int columns, int iFixedWidth, bool bRemoveExtension, bool bIncludeNone, char *filter, float fbox_height, bool bDisplayDesc,bool bUseFullScreen)
{
	cstr sReturnComboName = currentselection;
	if (columns <= 0) columns = 2; //Default to 2.
	if (bDisplayDesc) columns++;

	float fImageWidth = (ImGui::GetContentRegionAvailWidth() - 10.0f - 18.0f) / columns; //ImGui::GetCurrentWindow()->ScrollbarSizes.x
	if (iFixedWidth > 0)
		fImageWidth = iFixedWidth;
	fImageWidth -= 5.0f;
	fImageWidth -= 6.0f; //Due to now using Columns add spacing.

	float fImageRatio = fImageWidth / 512.0f;

	float box_height = (((float)288.0*fImageRatio) * 5.0) + 10.0f;
	box_height += ImGui::GetFontSize() * 8.0;
	if (fbox_height > 0) box_height = fbox_height;

	ImGui::BeginChild("##ListboxFilesListForWelcome", ImVec2(ImGui::GetContentRegionAvail().x - 2.0, box_height), false, iGenralWindowsFlags | ImGuiWindowFlags_NoSavedSettings);
	ImGui::Indent(2);
	if (1)
	{
		ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(0, 3));
		bool is_selected = false;
		float total_width = ImGui::GetContentRegionAvailWidth();
		ImGui::Columns(columns, "ListboxFilesListForWelcomecolumns", false);  //false no border

		if (bDisplayDesc)
		{
			ImGui::SetColumnOffset(0, 0.0f);
			ImGui::SetColumnOffset(1, total_width * 0.34); //Right 60% size.
			fImageRatio = (ImGui::GetContentRegionAvailWidth()-2.0f) / 512.0f;
		}

		int columns_count = 0;
		for (int n = 0; n < 10; n++) //Display max 12 games.
		{
			int TextureID = WELCOME_FILLERROUNDED;
			cStr sName = "";
			ImVec4 alpha = { 1.0f,1.0f,1.0f,0.15f };

			if (n < g_LibraryFileList.size())
			{
				if (g_LibraryFileList[n].iImage > 0 && ImageExist(g_LibraryFileList[n].iImage))
				{
					TextureID = g_LibraryFileList[n].iImage;
				}
				sName = g_LibraryFileList[n].cName;
				alpha = { 1.0f,1.0f,1.0f,1.0f };
			}


			is_selected = (sReturnComboName == sName);

			cstr sDisplayName = sName;
			if (bRemoveExtension && n < g_LibraryFileList.size())
			{
				char *find = (char *)pestrcasestr(sDisplayName.Get(), ".");
				if (find)
				{
					int iPos = find - sDisplayName.Get();
					if (iPos > 0 && iPos < 1024)
						sDisplayName = Left(sDisplayName.Get(), iPos);
				}
			}
			bool bVisible = true;
			if (filter && n < g_LibraryFileList.size())
			{
				bVisible = false;
				cstr sfilter = filter;
				char *p = strtok(sfilter.Get(), ",");
				while (p)
				{
					if (p[0] == '*')
						bVisible = true;
					else if (pestrcasestr(g_LibraryFileList[n].cName.Get(), p))
						bVisible = true;
					p = strtok(NULL, ",");
				}
			}
			if (bVisible)
			{
				ImVec2 iThumbSize = { (float)512.0*fImageRatio, (float)288.0*fImageRatio };
				if (is_selected)
				{
					ImGuiWindow* window = ImGui::GetCurrentWindow();
					ImVec4 tool_selected_col = ImGui::GetStyle().Colors[ImGuiCol_PlotHistogram];
					ImVec2 padding = { 1.0, 1.0 };
					const ImRect image_bb((window->DC.CursorPos - padding), window->DC.CursorPos + padding + iThumbSize);
					window->DrawList->AddRect(image_bb.Min, image_bb.Max, ImGui::GetColorU32(tool_selected_col), 0.0f, 15, 2.0f);
				}

				ImGuiWindow* window = ImGui::GetCurrentWindow();
				ImRect image_bb(window->DC.CursorPos, window->DC.CursorPos + iThumbSize);

				if (ImGui::ImgBtn(TextureID, iThumbSize, ImColor(0, 0, 0, 0), ImColor(255, 255, 255, 255)*alpha, ImColor(255, 255, 255, 200)*alpha, ImColor(255, 255, 255, 200)*alpha, 0, 0, 0, 0, false, false, false))
				{
					if(n < g_LibraryFileList.size())
						sReturnComboName = g_LibraryFileList[n].cName;
				}

				if (ImGui::IsItemHovered())
				{
					if (ImGui::IsMouseDoubleClicked(0))
					{
						if (n < g_LibraryFileList.size())
						{
							sReturnComboName = g_LibraryFileList[n].cName;
							bTriggerEditDemoGame = true;
						}
					}
				}
				static int triggerEndVideo = 0;
				if (n < g_LibraryFileList.size() && ImGui::IsItemHovered() && g_LibraryFileList[n].timer++ > 50)
				{
					if (iWelcomeVideoID > 0 && AnimationExist(iWelcomeVideoID))
					{
						float fVideoW = 0;
						float fVideoH = 0;
						float animU = 1.0;
						float animV = 1.0;
						ID3D11ShaderResourceView* lpVideoTexture = NULL;
						lpVideoTexture = GetAnimPointerView(iWelcomeVideoID);
						if (lpVideoTexture != NULL)
						{
							fVideoW = GetAnimWidth(iWelcomeVideoID);
							fVideoH = GetAnimHeight(iWelcomeVideoID);
							SetRenderAnimToImage(iWelcomeVideoID, true);
							animU = GetAnimU(iWelcomeVideoID);
							animV = GetAnimV(iWelcomeVideoID);
							ImVec2 uv0 = ImVec2(0, 0);
							ImVec2 uv1 = ImVec2(animU, animV);
							window->DrawList->AddImage((ImTextureID)lpVideoTexture, image_bb.Min, image_bb.Max, uv0, uv1, ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 1.0f)));
						}

						float fdone = GetAnimPercentDone(iWelcomeVideoID) / 100.0f;
						if (fdone > 0.5)
							triggerEndVideo = 1;
						if (triggerEndVideo > 0 && (fdone == 0.0f || fdone >= 1.0f))
						{
							g_LibraryFileList[n].timer = 0;
							bRemoveVideoInNextFrame = true;
							triggerEndVideo = 0;
						}
					}
					else
					{
						//Load video.
						std::string sVideoFile = g_LibraryFileList[n].cFile.Get();
						replaceAll(sVideoFile, ".png", ".mp4");
						if (FileExist((LPSTR) sVideoFile.c_str()))
						{
							iWelcomeVideoID = 0;
							for (int i = 1; i <= 32; i++)
							{
								if (AnimationExist(i) == 0) { iWelcomeVideoID = i; break; }
							}

							if (iWelcomeVideoID > 0)
							{
								if (LoadAnimation((LPSTR) sVideoFile.c_str(), iWelcomeVideoID, g.videoprecacheframes, g.videodelayedload, 1) == false)
								{
									iWelcomeVideoID = -999;
								}
								if (iWelcomeVideoID > 0)
								{
									PlaceAnimation(iWelcomeVideoID, -1, -1, -1, -1);
									SetRenderAnimToImage(iWelcomeVideoID, true);
									PlayAnimation(iWelcomeVideoID);
									SetVideoVolume(100);
								}
							}
						}
						else
						{
							g_LibraryFileList[n].timer = 0; //Failed.
							triggerEndVideo = 0;
						}
					}
				}
				else
				{
					if (n < g_LibraryFileList.size() && !ImGui::IsItemHovered())
					{
						if (g_LibraryFileList[n].timer > 0 && iWelcomeVideoID > 0 && AnimationExist(iWelcomeVideoID)) {
							bRemoveVideoInNextFrame = true;
							triggerEndVideo = 0;
						}
						g_LibraryFileList[n].timer = 0;
					}
				}
				//zombie cellar demo - level1.mp4
				if (!bDisplayDesc)
					ImGui::Text(sDisplayName.Get());
				if (bDisplayDesc)
				{
					if (n < g_LibraryFileList.size())
					{
						ImGui::SetWindowFontScale(1.4);
						char UniqueString[256];
						ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.00f));
						if (g_LibraryFileList[n].iType == 2)
						{
							ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.0f, 0.0f, 1.00f));
							ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.6f, 0.0f, 0.0f, 0.75f));
							ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.0f, 0.0f, 1.00f));
							sprintf(UniqueString, "Design Complexity: Developer##%d", n);
						}
						else if (g_LibraryFileList[n].iType == 1)
						{
							ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.7f, 0.0f, 1.00f));
							ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.7f, 0.0f, 0.75f));
							ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.8f, 0.7f, 0.0f, 1.00f));
							sprintf(UniqueString, "Design Complexity: Advanced##%d", n);
						}
						else
						{
							ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.6f, 0.0f, 1.00f));
							ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 0.6f, 0.0f, 0.75f));
							ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.6f, 0.0f, 1.00f));
							sprintf(UniqueString, "Design Complexity: Standard##%d", n);
						}

						if (ImGui::StyleButton(UniqueString, ImVec2(iThumbSize.x, 0.0f)))
						{
							//Select Game
							if (n < g_LibraryFileList.size())
								sReturnComboName = g_LibraryFileList[n].cName;
						}

						ImGui::PopStyleColor(4);

						ImGui::SetWindowFontScale(1.0);
					}
				}

				ImGui::NextColumn();
				if (is_selected) ImGui::SetItemDefaultFocus();

				if (bDisplayDesc)
				{
					if (n < g_LibraryFileList.size())
					{
						//Overlay rating.
						char UniqueString[256];
						sprintf(UniqueString, "##ListboxFilesDescriptionBox%d", n);
						ImGui::BeginChild(UniqueString, ImVec2(ImGui::GetContentRegionAvailWidth() - 2.0f, iThumbSize.y+38.0f), false, iGenralWindowsFlags | ImGuiWindowFlags_NoSavedSettings);
						ImGui::SetWindowFontScale(1.8);
						ImVec4 textcol = ImGui::GetStyleColorVec4(ImGuiCol_Button);
						textcol.w = 1.0;
						ImGui::PushStyleColor(ImGuiCol_Text, textcol);
						ImGui::Text(sDisplayName.Get());
						ImGui::PopStyleColor();
						//ImGui::Separator();
						ImGui::SetWindowFontScale(1.2);
						if (bUseFullScreen) ImGui::SetWindowFontScale(1.5);
						ImGui::TextWrapped(g_LibraryFileList[n].cDescription.Get());
						ImGui::SetWindowFontScale(1.0);
						ImGui::EndChild();
					}
					ImGui::NextColumn();
				}
			}
		}
		ImGui::Columns(1);
	}
	ImGui::Indent(-2);
	ImGui::EndChild();
	return sReturnComboName;
}

cstr ListboxFilesListForWelcomeScreen_v2(char *currentselection, int columns, int iFixedWidth, bool bRemoveExtension, bool bIncludeNone, char *filter, float fbox_height, bool bDisplayDesc, bool bUseFullScreen)
{
	cstr sReturnComboName = currentselection;
	if (columns <= 0) columns = 2; //Default to 2.
	if (bDisplayDesc) columns++;

	float fImageWidth = (ImGui::GetContentRegionAvailWidth() - 10.0f - 18.0f) / columns; //ImGui::GetCurrentWindow()->ScrollbarSizes.x
	if (iFixedWidth > 0)
		fImageWidth = iFixedWidth;
	fImageWidth -= 5.0f;
	fImageWidth -= 6.0f; //Due to now using Columns add spacing.

	float fImageRatio = fImageWidth / 512.0f;

	float box_height = (((float)288.0*fImageRatio) * 5.0) + 10.0f;
	box_height += ImGui::GetFontSize() * 8.0;
	if (fbox_height > 0) box_height = fbox_height;

	ImGui::BeginChild("##ListboxFilesListForWelcome", ImVec2(ImGui::GetContentRegionAvail().x - 2.0, box_height), false, iGenralWindowsFlags | ImGuiWindowFlags_NoSavedSettings);
	ImGui::Indent(2);
	if (1)
	{
		ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(0, 3));
		bool is_selected = false;
		float total_width = ImGui::GetContentRegionAvailWidth();
		ImGui::Columns(columns, "ListboxFilesListForWelcomecolumns", false);  //false no border

		if (bDisplayDesc)
		{
			ImGui::SetColumnOffset(0, 0.0f);
			ImGui::SetColumnOffset(1, total_width * 0.34); //Right 60% size.
			fImageRatio = (ImGui::GetContentRegionAvailWidth() - 2.0f) / 512.0f;
		}

		int columns_count = 0;
		for (int n = 0; n < g_LibraryFileList.size(); n++) // Display max 12 games.
		{
			int TextureID = WELCOME_FILLERROUNDED;
			cStr sName = "";
			ImVec4 alpha = { 1.0f,1.0f,1.0f,0.15f };

			if (n < g_LibraryFileList.size())
			{
				if (g_LibraryFileList[n].iImage > 0 && ImageExist(g_LibraryFileList[n].iImage))
				{
					TextureID = g_LibraryFileList[n].iImage;
				}
				sName = g_LibraryFileList[n].cName;
				alpha = { 1.0f,1.0f,1.0f,1.0f };
			}


			is_selected = (sReturnComboName == sName);

			cstr sDisplayName = sName;
			if (bRemoveExtension && n < g_LibraryFileList.size())
			{
				char *find = (char *)pestrcasestr(sDisplayName.Get(), ".");
				if (find)
				{
					int iPos = find - sDisplayName.Get();
					if (iPos > 0 && iPos < 1024)
						sDisplayName = Left(sDisplayName.Get(), iPos);
				}
			}
			bool bVisible = true;
			if (filter && n < g_LibraryFileList.size())
			{
				bVisible = false;
				cstr sfilter = filter;
				char *p = strtok(sfilter.Get(), ",");
				while (p)
				{
					if (p[0] == '*')
						bVisible = true;
					else if (pestrcasestr(g_LibraryFileList[n].cName.Get(), p))
						bVisible = true;
					p = strtok(NULL, ",");
				}
			}
			if (bVisible)
			{
				ImVec2 iThumbSize = { (float)512.0*fImageRatio, (float)288.0*fImageRatio };
				if (is_selected)
				{
					ImGuiWindow* window = ImGui::GetCurrentWindow();
					ImVec4 tool_selected_col = ImGui::GetStyle().Colors[ImGuiCol_PlotHistogram];
					ImVec2 padding = { 1.0, 1.0 };
					const ImRect image_bb((window->DC.CursorPos - padding), window->DC.CursorPos + padding + iThumbSize);
					window->DrawList->AddRect(image_bb.Min, image_bb.Max, ImGui::GetColorU32(tool_selected_col), 0.0f, 15, 2.0f);
				}

				ImGuiWindow* window = ImGui::GetCurrentWindow();
				ImRect image_bb(window->DC.CursorPos, window->DC.CursorPos + iThumbSize);
				ImVec2 vTopLeftCorner = ImGui::GetCursorPos();

				if (ImGui::ImgBtn(TextureID, iThumbSize, ImColor(0, 0, 0, 0), ImColor(255, 255, 255, 255)*alpha, ImColor(255, 255, 255, 200)*alpha, ImColor(255, 255, 255, 200)*alpha, 0, 0, 0, 0, false, false, false))
				{
					if (n < g_LibraryFileList.size())
						sReturnComboName = g_LibraryFileList[n].cName;
				}

				// add not available image over thumb if project not exist
				extern bool g_bFreeTrialVersion;
				if (g_bFreeTrialVersion == true)
				{
					if (g_LibraryFileList[n].bProjectExists == false)
					{
						// place grey layer on thumb to show object not available
						ImVec2 vOldPos = ImGui::GetCursorPos();
						ImGui::SetCursorPos(vTopLeftCorner);
						ImGui::SetItemAllowOverlap();
						ImGui::ImgBtn(FREETRIAL_NOTAVAILABLE, iThumbSize, ImColor(0, 0, 0, 0), ImColor(255, 255, 255, 180), ImColor(255, 255, 255, 180), ImColor(255, 255, 255, 180), 0, 0, 0, 0, false, false, false, false, true, bBoostIconColors);
						ImGui::SetCursorPos(vOldPos);
					}
				}

				if (ImGui::IsItemHovered())
				{
					if (ImGui::IsMouseDoubleClicked(0))
					{
						if (n < g_LibraryFileList.size())
						{
							sReturnComboName = g_LibraryFileList[n].cName;
							bTriggerEditDemoGame = true;

							if (iWelcomeVideoID > 0 && AnimationExist(iWelcomeVideoID)) {
								bRemoveVideoInNextFrame = true;
							}
						}
					}
				}
				static int triggerEndVideo = 0;
				if (n < g_LibraryFileList.size() && ImGui::IsItemHovered() && g_LibraryFileList[n].timer++ > 50)
				{
					if (iWelcomeVideoID > 0 && AnimationExist(iWelcomeVideoID))
					{
						float fVideoW = 0;
						float fVideoH = 0;
						float animU = 1.0;
						float animV = 1.0;
						ID3D11ShaderResourceView* lpVideoTexture = NULL;
						lpVideoTexture = GetAnimPointerView(iWelcomeVideoID);
						if (lpVideoTexture != NULL)
						{
							fVideoW = GetAnimWidth(iWelcomeVideoID);
							fVideoH = GetAnimHeight(iWelcomeVideoID);
							SetRenderAnimToImage(iWelcomeVideoID, true);
							animU = GetAnimU(iWelcomeVideoID);
							animV = GetAnimV(iWelcomeVideoID);
							ImVec2 uv0 = ImVec2(0, 0);
							ImVec2 uv1 = ImVec2(animU, animV);
							window->DrawList->AddImage((ImTextureID)lpVideoTexture, image_bb.Min, image_bb.Max, uv0, uv1, ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 1.0f)));
						}

						float fdone = GetAnimPercentDone(iWelcomeVideoID) / 100.0f;
						if (fdone > 0.5)
							triggerEndVideo = 1;
						if (triggerEndVideo > 0 && (fdone == 0.0f || fdone >= 1.0f))
						{
							g_LibraryFileList[n].timer = 0;
							bRemoveVideoInNextFrame = true;
							triggerEndVideo = 0;
						}
					}
					else
					{
						//Load video.
						std::string sVideoFile = g_LibraryFileList[n].cFile.Get();
						replaceAll(sVideoFile, ".png", ".mp4");
						if (FileExist((LPSTR)sVideoFile.c_str()))
						{
							iWelcomeVideoID = 0;
							for (int i = 1; i <= 32; i++)
							{
								if (AnimationExist(i) == 0) { iWelcomeVideoID = i; break; }
							}
							if (iWelcomeVideoID > 0)
							{
								for (int i = 1; i <= 32; i++)
								{
									if (i != iWelcomeVideoID && AnimationExist(i) == 1)
									{ 
										StopAnimation(i);
									}
								}
								if (LoadAnimation((LPSTR)sVideoFile.c_str(), iWelcomeVideoID, g.videoprecacheframes, g.videodelayedload, 1) == false)
								{
									iWelcomeVideoID = -999;
								}
								if (iWelcomeVideoID > 0)
								{
									PlaceAnimation(iWelcomeVideoID, -1, -1, -1, -1);
									SetRenderAnimToImage(iWelcomeVideoID, true);
									PlayAnimation(iWelcomeVideoID);
									SetVideoVolume(100);
								}
							}
						}
						else
						{
							g_LibraryFileList[n].timer = 0; //Failed.
							triggerEndVideo = 0;
						}
					}
				}
				else
				{
					if (n < g_LibraryFileList.size() && !ImGui::IsItemHovered())
					{
						if (g_LibraryFileList[n].timer > 0 && iWelcomeVideoID > 0 && AnimationExist(iWelcomeVideoID)) {
							bRemoveVideoInNextFrame = true;
							triggerEndVideo = 0;
						}
						g_LibraryFileList[n].timer = 0;
					}
				}

				if(1)
				{
					if (n < g_LibraryFileList.size())
					{
						ImGui::SetWindowFontScale(1.4);
						char UniqueString[256];
						ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.00f));
						if (g_LibraryFileList[n].iType == 2)
						{
							ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.0f, 0.0f, 1.00f));
							ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.6f, 0.0f, 0.0f, 0.75f));
							ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.0f, 0.0f, 1.00f));
							sprintf(UniqueString, "Design Complexity: Developer");
						}
						else if (g_LibraryFileList[n].iType == 1)
						{
							ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.7f, 0.0f, 1.00f));
							ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.7f, 0.0f, 0.75f));
							ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.8f, 0.7f, 0.0f, 1.00f));
							sprintf(UniqueString, "Design Complexity: Advanced");
						}
						else
						{
							ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.6f, 0.0f, 1.00f));
							ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 0.6f, 0.0f, 0.75f));
							ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.6f, 0.0f, 1.00f));
							sprintf(UniqueString, "Design Complexity: Standard");
						}

						if (ImGui::StyleButton(sDisplayName.Get(), ImVec2(iThumbSize.x, 0.0f)))
						{
							//Select Game
							if (n < g_LibraryFileList.size())
								sReturnComboName = g_LibraryFileList[n].cName;
						}
						if (ImGui::IsItemHovered() && iSkibFramesBeforeLaunch == 0) ImGui::SetTooltip("%s", UniqueString);

						ImGui::PopStyleColor(4);

						ImGui::SetWindowFontScale(1.0);
					}
				}

				ImGui::NextColumn();
				if (is_selected) ImGui::SetItemDefaultFocus();

				if (bDisplayDesc)
				{
					if (n < g_LibraryFileList.size())
					{
						//Overlay rating.
						char UniqueString[256];
						sprintf(UniqueString, "##ListboxFilesDescriptionBox%d", n);
						ImGui::BeginChild(UniqueString, ImVec2(ImGui::GetContentRegionAvailWidth() - 2.0f, iThumbSize.y + 38.0f), false, iGenralWindowsFlags | ImGuiWindowFlags_NoSavedSettings);
						ImGui::SetWindowFontScale(1.8);
						ImVec4 textcol = ImGui::GetStyleColorVec4(ImGuiCol_Button);
						textcol.w = 1.0;
						ImGui::PushStyleColor(ImGuiCol_Text, textcol);
						ImGui::Text(sDisplayName.Get());
						ImGui::PopStyleColor();
						//ImGui::Separator();
						ImGui::SetWindowFontScale(1.2);
						if (bUseFullScreen) ImGui::SetWindowFontScale(1.5);
						ImGui::TextWrapped(g_LibraryFileList[n].cDescription.Get());
						ImGui::SetWindowFontScale(1.0);
						ImGui::EndChild();
					}
					ImGui::NextColumn();
				}
			}
		}
		ImGui::Columns(1);
	}
	ImGui::Indent(-2);
	ImGui::EndChild();
	return sReturnComboName;
}
struct ProjectSortData
{
	std::string writeDate;
	std::string folderName;
};

// Compare two ProjectSortData by date.
bool CompareFileDates(const ProjectSortData& file0, const ProjectSortData& file1)
{
	if (file0.writeDate.compare(file1.writeDate) > 0) return true; // date2 comes before date1 (change required)

	return false; // date1 comes before date2 or they are the same (no change required)
}
// Compare two ProjectSortData by name.
bool CompareFileNames(const ProjectSortData& file0, const ProjectSortData& file1)
{
	cstr a ((char*)file0.folderName.c_str());
	cstr b ((char*)file1.folderName.c_str());

	if (strcmp(a.Lower().Get(), b.Lower().Get()) < 0) return true;

	return false;
}

// Fill in Project Sort Data so it can be sorted by write time later.
void GetProjectSortData (std::vector<ProjectSortData>& output)
{
	cstr pOldDir = GetDir();

	char destination[MAX_PATH];
	strcpy(destination, "projectbank\\");
	GG_GetRealPath(destination, 1); // Get actual path to write folder.

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
				char project[MAX_PATH];
				sprintf(project, "%s%s\\project%d.dat", destination,folder.Get(), STORYBOARDVERSION);
				ProjectSortData data;
				data.folderName = std::string(folder.Get());
				wchar_t filePath[MAX_PATH];
				MultiByteToWideChar(CP_UTF8, 0, &project[0], -1, filePath, MAX_PATH);

				HANDLE hFile = CreateFile(filePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
				if (hFile == INVALID_HANDLE_VALUE)
				{
					strcpy(project, destination);
					strcat(project, folder.Get());
					strcat(project, "\\project.dat");

					ProjectSortData data;
					data.folderName = std::string(folder.Get());
					wchar_t filePath[MAX_PATH];
					MultiByteToWideChar(CP_UTF8, 0, &project[0], -1, filePath, MAX_PATH);
					hFile = CreateFile(filePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
				}

				if (hFile == INVALID_HANDLE_VALUE)
				{
					strcpy(project, destination);
					strcat(project, folder.Get());
					strcat(project, "\\remoteproject.txt");
					if(GG_FileExists(project))
					{
						char pAbsTrueProjectPath[MAX_PATH];
						OpenToRead(1, project);
						strcpy(pAbsTrueProjectPath, ReadString(1));
						CloseFile(1);
						GG_GetRealPath(pAbsTrueProjectPath, 0);

						sprintf(project, "%s%s\\Files\\projectbank\\%s\\project%d.dat", pAbsTrueProjectPath,folder.Get(), folder.Get(), STORYBOARDVERSION);
						MultiByteToWideChar(CP_UTF8, 0, &project[0], -1, filePath, MAX_PATH);
						hFile = CreateFile(filePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
						if (hFile == INVALID_HANDLE_VALUE)
						{
							strcpy(project, pAbsTrueProjectPath);
							strcat(project, folder.Get());
							strcat(project, "\\Files\\projectbank\\");
							strcat(project, folder.Get());
							strcat(project, "\\project.dat");
							MultiByteToWideChar(CP_UTF8, 0, &project[0], -1, filePath, MAX_PATH);
							hFile = CreateFile(filePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
						}
					}
				}
				if (hFile != INVALID_HANDLE_VALUE)
				{
					// Get the last write time for this file.

					FILETIME ftCreate, ftAccess, ftWrite;
					SYSTEMTIME stUTC, stLocal;

					// Retrieve the file times for the file.
					if (GetFileTime(hFile, &ftCreate, &ftAccess, &ftWrite))
					{
						TCHAR wWriteTime[MAX_PATH];

						// Convert the last-write time to local time.
						FileTimeToSystemTime(&ftWrite, &stUTC);
						SystemTimeToTzSpecificLocalTime(NULL, &stUTC, &stLocal);

						// the sort system just uses string compare to work out which is the newest date, so make year/month more important
						StringCchPrintf(wWriteTime, MAX_PATH,
							TEXT("%04d/%02d/%02d  %02d:%02d:%02d"),
							stLocal.wYear, stLocal.wMonth, stLocal.wDay,
							stLocal.wHour, stLocal.wMinute, stLocal.wSecond);

						// Convert from wide char.
						char writeTime[MAX_PATH];
						WideCharToMultiByte(CP_UTF8, 0, &wWriteTime[0], MAX_PATH, &writeTime[0], MAX_PATH, NULL, NULL); 
						data.writeDate = std::string(writeTime);
						output.push_back(data);
					}
				}

				CloseHandle(hFile);					
			}
		}
	}
	SetDir(pOldDir.Get());
}

void SortProjects(int iProjectSortMode)
{
	if (projectbank_list.size() == 0)
		return;

	std::vector<ProjectSortData> sortData;
	GetProjectSortData(sortData);
	
	// Perform the sort.
	switch (iProjectSortMode)
	{
	case 0: // Newest
	{
		std::sort(sortData.begin(), sortData.end(), CompareFileDates);
		break;
	}
	case 1: // Oldest
	{
		std::sort(sortData.begin(), sortData.end(), CompareFileDates);
		std::reverse(sortData.begin(), sortData.end());
		break;
	}
	case 2: // A-Z
	{
		std::sort(sortData.begin(), sortData.end(), CompareFileNames);
		break;
	}
	case 3: // Z-A
	{
		std::sort(sortData.begin(), sortData.end(), CompareFileNames);
		std::reverse(sortData.begin(), sortData.end());
		break;
	}
	}

	// Set the new order of project names.
	projectbank_list.clear();
	for (int i = 0; i < sortData.size(); i++)
	{
		projectbank_list.push_back(sortData[i].folderName);
		projectbank_imageid[i] = 0;
	}
}

void GetProjectThumbnails()
{
	projectbank_image.clear();
	projectbank_imageid.resize(projectbank_list.size());
	projectbank_active.resize(projectbank_list.size());

	for (int i = 0; i < projectbank_list.size(); i++)
	{
		projectbank_active[i] = true;

		if (!pestrcasestr((char *)projectbank_list[i].c_str(), "_backup_"))
		{
			char project[MAX_PATH];
			strcpy(project, "projectbank\\");
			strcat(project, projectbank_list[i].c_str());
			strcat(project, "\\remoteproject.txt");
			FILE* projectfile = NULL;
			if (GG_FileExists(project))
			{
				// this project is a remote project
				char pAbsTrueProjectPath[MAX_PATH];
				OpenToRead(1, project);
				strcpy(pAbsTrueProjectPath, ReadString(1));
				CloseFile(1);
				GG_GetRealPath(pAbsTrueProjectPath, 0);
				strcpy(project, pAbsTrueProjectPath);
				strcat(project, projectbank_list[i].c_str());
				strcat(project, "\\Files\\projectbank\\");
			}
			else
			{
				// regular projectbank project
				strcpy(project, "projectbank\\");
			}
			strcat(project, projectbank_list[i].c_str());

			char project2[MAX_PATH];
			strcpy(project2, project);

			sprintf(project, "%s\\project%d.dat", project2, STORYBOARDVERSION);
			projectfile = GG_fopen(project, "rb");
			if (!projectfile)
			{
				strcpy(project, project2);
				strcat(project, "\\project.dat");
				projectfile = GG_fopen(project, "rb");
			}

			if (projectfile)
			{
				fclose(projectfile);

				//PE: Use this so we can upgrade from 202 to 203+
				bool load__storyboard_into_struct(const char*, StoryboardStruct&);
				load__storyboard_into_struct(project, checkproject);

				char sig[12] = "Storyboard\0";
				if (checkproject.sig[0] == 'S' && checkproject.sig[8] == 'r')
				{
					cstr bestfound = "";
					char pFindGameThumb[MAX_PATH];
					strcpy(pFindGameThumb, "");
					if (strlen(checkproject.customprojectfolder) > 0)
					{
						strcat(pFindGameThumb, checkproject.customprojectfolder);
						strcat(pFindGameThumb, checkproject.gamename);
						strcat(pFindGameThumb, "\\Files\\");
					}
					strcat(pFindGameThumb, checkproject.game_thumb);
					if (strlen(checkproject.game_thumb) > 0 && FileExist(pFindGameThumb))
					{
						bestfound = pFindGameThumb;
					}
					else
					{
						for (int i = 0; i < STORYBOARD_MAXNODES; i++) //SMALL_STORYBOARD_MAXNODES
						{
							if (checkproject.Nodes[i].used)
							{
								if (checkproject.Nodes[i].type == STORYBOARD_TYPE_SPLASH)
								{
									//Splash if no level found.
									if (bestfound == "")
									{
										if (!pestrcasestr(checkproject.Nodes[i].thumb, "editors\\uiv3\\"))
										{
											//custom use.
											bestfound = checkproject.Nodes[i].thumb;
										}
									}
								}
								if (checkproject.Nodes[i].type == STORYBOARD_TYPE_SCREEN)
								{
									if (pestrcasestr(checkproject.Nodes[i].title, "title screen"))
									{
										//Splash if no level found.
										if (!pestrcasestr(checkproject.Nodes[i].thumb, "editors\\templates\\"))
										{
											//custom use.
											bestfound = checkproject.Nodes[i].thumb;
										}
									}
								}
								//PE: Try finding level that loading.lua is pointing to ?
								if (checkproject.Nodes[i].type == STORYBOARD_TYPE_LEVEL && !pestrcasestr(Storyboard.Nodes[i].lua_name, "loading"))
								{
									if (strlen(checkproject.Nodes[i].level_name) > 0)
									{
										CreateBackBufferCacheNameEx(checkproject.Nodes[i].level_name, 512, 288, true);
										if (CreateProjectCacheName(checkproject.gamename, BackBufferCacheName.Get()) &&
											FileExist(ProjectCacheName.Get()))
										{
											bestfound = ProjectCacheName.Get();
											break;
										}
										else if (FileExist(BackBufferCacheName.Get()))
										{
											bestfound = BackBufferCacheName.Get();
											break;
										}
									}
								}

							}
						}
					}
					if(checkproject.project_inactive)
						projectbank_active[i] = false;

					projectbank_image.push_back(bestfound.Get());
				}
				else
				{
					projectbank_image.push_back(""); //Just use CLICK HERE.
				}
			}
			else
			{
				//PE: Was missing if not found. https://github.com/TheGameCreators/GameGuruRepo/issues/1722
				projectbank_image.push_back(""); //Just use CLICK HERE.
			}
		}
		else
		{
			projectbank_image.push_back(""); //Backup just use CLICK HERE.
		}
	}
}

