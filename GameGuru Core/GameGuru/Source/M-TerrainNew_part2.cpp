void imgui_Customize_Terrain(int mode)
{
	int wflags = ImGuiTreeNodeFlags_DefaultOpen;
	if (mode == 1) wflags = ImGuiTreeNodeFlags_None;
	if(pref.bAutoClosePropertySections && mode == 1 && iLastOpenHeader != 2)
		ImGui::SetNextItemOpen(false, ImGuiCond_Always);

	float media_icon_size = 40.0f;
	float w = ImGui::GetWindowContentRegionWidth();
	float plate_width = (media_icon_size + 6.0) * 4.0f;
	if (ImGui::StyleCollapsingHeader("Palette", wflags))
	{
		if (mode == 1) iLastOpenHeader = 2;
		//Drpo down.
		ImGui::Indent(10);
		ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));



		//Use "texturebank" as default folder.

		if (iDeleteSingleTerrainTextures > 0) {
			if (ImageExist(iDeleteSingleTerrainTextures))
				DeleteImage(iDeleteSingleTerrainTextures);
			iDeleteSingleTerrainTextures = 0;
		}
		if (iDeleteAllTerrainTextures) {
			iDeleteAllTerrainTextures = false;
			for (int iL = 0; iL < 32; iL++) {
				sTerrainTexturesID[iL] = t.terrain.imagestartindex + 80 + iL;
				sTerrainSelectionID[iL] = iL;
				bTextureNameWindow[iL] = false;
				if (ImageExist(t.terrain.imagestartindex + 80 + iL))
					DeleteImage(t.terrain.imagestartindex + 80 + iL);
			}
			bUpdateTerrainMaterials = true;
			iOldMaterial = -1; //PE: Reset selection.
		}
		if (t.visuals.sTerrainTextures[0] == "")
		{
			//Add a few default textures.
			//PE: We always need a default texture in [0].

			for( int i = 0; i < 32; i++ )
			{
				char temp[ 1024 ];
				sprintf( temp, "terraintextures\\mat%d\\Color.dds", i+1 );
				t.visuals.sTerrainTextures[i] = temp;
				sprintf( temp, "Material %d", i+1 );
				t.visuals.sTerrainTexturesName[i] = temp;
				t.gamevisuals.sTerrainTextures[i] = t.visuals.sTerrainTextures[i];
				t.gamevisuals.sTerrainTexturesName[i] = t.visuals.sTerrainTexturesName[i];
			}

			for (int iL = 0; iL < 32; iL++) {
				sTerrainSelectionID[iL] = iL;
				sTerrainTexturesID[iL] = t.terrain.imagestartindex + 80 + iL;
				bTextureNameWindow[iL] = false;
			}

			init_terrain_selections();
			//PE: We now activate if not exists, when selecting, so no need to reset all textures on a fresh palette.
			bUpdateTerrainMaterials = false;
		}
		int iUsedImages = 0;
		cStr sTextureFolder = g.rootdir_s + "texturebank\\";
		float col_start = 100.0f;
		int iSelectTexture = -1;

#ifdef DISPLAY4x4
		ImGui::Indent(-10);
		ImGui::Indent(4);
		ImGui::Columns(4, "##terrain4x4columns", false);  //false no border
#endif

		for (int iL = 0; iL < 32; iL++)
		{

			if (t.visuals.sTerrainTextures[iL] != "")
			{
				if (!ImageExist(sTerrainTexturesID[iL]))
				{
					//Load in image.
					image_setlegacyimageloading(true);
					SetMipmapNum(1); //PE: mipmaps not needed.
					if (ImageExist(sTerrainTexturesID[iL]) == 1) DeleteImage(sTerrainTexturesID[iL]);

					// sTerrainTextures stores as (uncompressed) to speed up material loading as raw data, but 
					// here they can be compressed for quicker loading, so..
					char pCompressedVersionIfAny[MAX_PATH];
					strcpy(pCompressedVersionIfAny, t.visuals.sTerrainTextures[iL].Get());
					int iLen = strlen(pCompressedVersionIfAny) - strlen(" (uncompressed).dds");
					//PE: Make sure we dont crash if using small filenames like "a.dds".
					if (iLen > 0)
					{
						pCompressedVersionIfAny[iLen] = 0;
						strcat(pCompressedVersionIfAny, ".dds");
						if (FileExist(pCompressedVersionIfAny) == 0)
						{
							strcpy(pCompressedVersionIfAny, t.visuals.sTerrainTextures[iL].Get());
						}
					}
					LoadImage(pCompressedVersionIfAny, sTerrainTexturesID[iL], 0, g.gdividetexturesize);
					if (ImageExist(sTerrainTexturesID[iL]) == 1) 
					{
					}
					else 
					{
						//Load failed, clear texture slot.
						t.visuals.sTerrainTextures[iL] = "";
						t.gamevisuals.sTerrainTextures[iL] = t.visuals.sTerrainTextures[iL];
						g.projectmodified = 1;
						bUpdateTerrainMaterials = true;
					}
					SetMipmapNum(-1);
					image_setlegacyimageloading(false);
				}

				if (ImageExist(sTerrainTexturesID[iL]))
				{

					if (bTextureNameWindow[iL]) {
						//Ask for a proper name of texture.

						ImGui::SetNextWindowSize(ImVec2(26 * ImGui::GetFontSize(), 32 * ImGui::GetFontSize()), ImGuiCond_Once);
						ImGui::SetNextWindowPosCenter(ImGuiCond_Once);
						cstr sUniqueWinName = cstr("Terrain Texture Name##ttn") + cstr(iL);
						ImGui::Begin(sUniqueWinName.Get(), &bTextureNameWindow[iL], 0);
						ImGui::Indent(10);
						static char NewTextureName[256];
						cstr sUniqueInputName = cstr("##InoutTerrainName") + cstr(iL);
						float content_width = ImGui::GetContentRegionAvailWidth() - 10.0;
						ImGui::ImgBtn(sTerrainTexturesID[iL], ImVec2(content_width, content_width), ImColor(0, 0, 0, 255));
						ImGui::PushItemWidth(-10);
						ImGui::Text("Enter a name for your terrain texture:");
						if (ImGui::IsRootWindowOrAnyChildFocused() && !ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0))
							ImGui::SetKeyboardFocusHere(0);
						if (ImGui::InputText(sUniqueInputName.Get(), t.visuals.sTerrainTexturesName[iL].Get(), 250, ImGuiInputTextFlags_EnterReturnsTrue)) {
							t.gamevisuals.sTerrainTexturesName[iL] = t.visuals.sTerrainTexturesName[iL];
							bTextureNameWindow[iL] = false;
						}
						if (ImGui::MaxIsItemFocused()) bImGuiGotFocus = true;

						ImGui::PopItemWidth();
						ImGui::Indent(-10);
						if (ImGui::GetCurrentWindow()->ScrollbarSizes.x > 0) {
							//Hitting exactly at the botton could cause flicker, so add some additional lines when scrollbar on.
							ImGui::Text("");
							ImGui::Text("");
						}
						bImGuiGotFocus = true;
						ImGui::End();
					}



					iUsedImages++;

#ifndef DISPLAY4x4
					if (iUsedImages < 32 && iL == 0)
					{
						float but_gadget_size = ImGui::GetFontSize()*10.0;
						ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((w*0.5) - (but_gadget_size*0.5), 0.0f));

						if (ImGui::StyleButton("Add Texture", ImVec2(but_gadget_size, 0))) {
							iSelectTexture = 99;
						}
						ImGui::Separator();
					}

					char materialname[256];
					strcpy(materialname, t.visuals.sTerrainTextures[iL].Get());
#endif
					float path_gadget_size = ImGui::GetFontSize()*3.0;
					int preview_icon_size = ImGui::GetFontSize();

					bool bDeleteImage = true;
					if (iL == 0)
						bDeleteImage = false;

					//TEXTURE.

					cStr sLabel = cStr("Texture##InputTerrainTexture") + cStr(iL);
					if (!bDeleteImage)
						sLabel = cStr("Default##InputTerrainTexture") + cStr(iL);

					//PE: New do 4x4 coloums here.
#ifdef DISPLAY4x4
					int iLargerPreviewIconSize = 28;//PE: 54 , now lowest possible icon
					float control_width = (iLargerPreviewIconSize + 3.0) * 4.0f + 6.0;

					if (w > control_width) {
						//PE: fit perfectly with window width.
						iLargerPreviewIconSize = (w - 20.0) / 4.0;
						iLargerPreviewIconSize -= 6.0; //Padding.
						if (iLargerPreviewIconSize > 70) iLargerPreviewIconSize = 70;
					}

					ImVec2 vSelectionDraw = ImGui::GetCurrentWindow()->DC.CursorPos;
					if (sTerrainTexturesID[iL] > 0)
					{

						if (iL == 0) {
							CheckTutorialAction("TOOL_TERRAIN_SAND", -20.0f); //Tutorial: check if we are waiting for this action
						}
						if (iL == 2) {
							CheckTutorialAction("TOOL_TERRAIN_ROCK", -20.0f); //Tutorial: check if we are waiting for this action
						}

						cStr sLabelChild = cStr("##terrain4x4") + cStr(iL);

						ImVec2 content_avail = { iLargerPreviewIconSize + 1.0f ,iLargerPreviewIconSize + 1.0f };

						//style.WindowPadding
						ImVec2 oldstyle = ImGui::GetStyle().FramePadding;
						ImGui::GetStyle().FramePadding = { 0,0 };
						ImGui::BeginChild(sLabelChild.Get(), content_avail, false, ImGuiWindowFlags_NoScrollbar);

						iLargerPreviewIconSize &= 0xfffe;

						ImGui::SetBlurMode(true);
						if (ImGui::ImgBtn(sTerrainTexturesID[iL], ImVec2(iLargerPreviewIconSize, iLargerPreviewIconSize), ImColor(0, 0, 0, 255), ImColor(220,220,220,220), ImColor(255,255,255,255), ImColor(180,180,160,255), -1,
										   0, 0, 0, false, false, false, false, false, true))
						{
							if (iL == 0 && bTutorialCheckAction) TutorialNextAction();
							if (iL == 2 && bTutorialCheckAction) TutorialNextAction();
							iCurrentTextureForPaint = sTerrainSelectionID[iL];

						}
						ImGui::SetBlurMode(false);

						// Our buttons are both drag sources and drag targets here!
						if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
						{
							ImGui::SetDragDropPayload("DND_TERRAIN_TEXTURES", &iL, sizeof(int));
							ImGui::Text("Swap Texture");
							ImGui::EndDragDropSource();
						}
						if (ImGui::BeginDragDropTarget())
						{
							if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_TERRAIN_TEXTURES"))
							{
								IM_ASSERT(payload->DataSize == sizeof(int));
								int payload_n = *(const int*)payload->Data;

								if (payload_n >= 0 && payload_n < 32)
								{
									//PE: Swap iL and payload_n
									cStr sTmp = t.visuals.sTerrainTextures[iL];
									cStr sTmp2 = t.visuals.sTerrainTexturesName[iL];
									int iTmp = sTerrainTexturesID[iL];
									int iTmp2 = sTerrainSelectionID[iL];

									t.visuals.sTerrainTextures[iL] = t.visuals.sTerrainTextures[payload_n];
									t.visuals.sTerrainTexturesName[iL] = t.visuals.sTerrainTexturesName[payload_n];
									sTerrainTexturesID[iL] = sTerrainTexturesID[payload_n];
									sTerrainSelectionID[iL] = sTerrainSelectionID[payload_n];

									t.visuals.sTerrainTextures[payload_n] = sTmp;
									t.visuals.sTerrainTexturesName[payload_n] = sTmp2;
									sTerrainTexturesID[payload_n] = iTmp;
									sTerrainSelectionID[payload_n] = iTmp2;

									t.gamevisuals.sTerrainTextures[iL] = t.visuals.sTerrainTextures[iL];
									t.gamevisuals.sTerrainTexturesName[iL] = t.visuals.sTerrainTexturesName[iL];
									t.gamevisuals.sTerrainTextures[payload_n] = t.visuals.sTerrainTextures[payload_n];
									t.gamevisuals.sTerrainTexturesName[payload_n] = t.visuals.sTerrainTexturesName[payload_n];
								}
							}
							ImGui::EndDragDropTarget();
						}

						bool bInContext = false;
						static int iCurrentTerrainContext = -1;
						if (!bInContext && ImGui::IsItemHovered())
						{
							ImGui::BeginTooltip();
							ImGui::ImgBtn(sTerrainTexturesID[iL], ImVec2(350, 350), ImColor(0, 0, 0, 255));
							ImGui::TextCenter(t.visuals.sTerrainTexturesName[iL].Get());
							ImGui::Separator();
							ImGui::EndTooltip();
						}

						ImGui::EndChild();
						ImGui::GetStyle().FramePadding = oldstyle;
					}

					if (iCurrentTextureForPaint == sTerrainSelectionID[iL]) //sTerrainTexturesID[iL] - t.terrain.imagestartindex - 80)
					{
						ImVec2 padding = { 2.0, 2.0 };
						const ImRect image_bb((vSelectionDraw - padding), vSelectionDraw + padding + ImVec2(iLargerPreviewIconSize, iLargerPreviewIconSize));
						ImGui::GetCurrentWindow()->DrawList->AddRect(image_bb.Min, image_bb.Max, ImGui::GetColorU32(tool_selected_col), 0.0f, 15, 2.0f);
					}

					ImGui::NextColumn();

#else
					ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));

					if (iL == 0) {
						CheckTutorialAction("TOOL_TERRAIN_SAND", -20.0f); //Tutorial: check if we are waiting for this action
					}

					if (ImGui::RadioButton(sLabel.Get(), &iCurrentTextureForPaint, iL)) {
						if (iL == 0 && bTutorialCheckAction) TutorialNextAction();
					}
					if (iOldMaterial != iCurrentTextureForPaint)
					{
						//@Michael + 1 should be removed when your ready, this is just to follow your current test code :)
						if (g_pTerrain) 
						{
							if (g_pTerrain->FindMaterial(cStr(iCurrentTextureForPaint).Get()) != nullptr)
							{
								g_pTerrain->SetMaterial(cStr(iCurrentTextureForPaint).Get());
							}

							iOldMaterial = iCurrentTextureForPaint;
						}
					}

					ImGui::SameLine();

					int iSmallPreviewYMargin = preview_icon_size / 2;
					int iLargerPreviewIconSize = preview_icon_size * 2;
					ImGui::SetCursorPos(ImVec2(col_start-10.0f , ImGui::GetCursorPosY() - iSmallPreviewYMargin));
//					ImGui::SetCursorPos(ImVec2(col_start, ImGui::GetCursorPosY() - 3));

					if (sTerrainTexturesID[iL] > 0)
					{
						if (ImGui::ImgBtn(sTerrainTexturesID[iL], ImVec2(iLargerPreviewIconSize, iLargerPreviewIconSize), ImColor(0, 0, 0, 255)))
						{
							iSelectTexture = iL;
						}

						if (ImGui::IsItemHovered())
						{
							ImGui::BeginTooltip();
							ImGui::ImgBtn(sTerrainTexturesID[iL], ImVec2(180, 180), ImColor(0, 0, 0, 255));
							ImGui::EndTooltip();
						}
						ImGui::SameLine();
					}

					ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + iSmallPreviewYMargin));

					int iInputFlags = ImGuiInputTextFlags_EnterReturnsTrue;
					if (!bDeleteImage)
						iInputFlags = ImGuiInputTextFlags_ReadOnly;

					sLabel = cStr("##InputTerrainTexture") + cStr(iL);
					ImGui::PushItemWidth(-10 - path_gadget_size);
					if (ImGui::InputText(sLabel.Get(), &materialname[0], 250, iInputFlags))
					{
						if (strlen(materialname) == 0)
							t.visuals.sTerrainTextures[iL] = ""; //delete image in next run.
						t.gamevisuals.sTerrainTextures[iL] = t.visuals.sTerrainTextures[iL];
						g.projectmodified = 1;
						bUpdateTerrainMaterials = true;
						if (iCurrentTextureForPaint == iL)
							iCurrentTextureForPaint--;

					}
					ImGui::PopItemWidth();

					ImGui::SameLine();
					ImGui::PushItemWidth(path_gadget_size);
					sLabel = cStr("...##InputTerrainTexture") + cStr(iL);

					if (ImGui::StyleButton(sLabel.Get()))
						iSelectTexture = iL;

					if (bDeleteImage) {
						ImGui::SameLine();
						sLabel = cStr("X##InputTerrainTexture") + cStr(iL);
						if (ImGui::StyleButton(sLabel.Get())) {
							t.visuals.sTerrainTextures[iL] = ""; //delete image in next run.
							t.gamevisuals.sTerrainTextures[iL] = t.visuals.sTerrainTextures[iL];
							g.projectmodified = 1;
							bUpdateTerrainMaterials = true;
							if (iCurrentTextureForPaint == iL)
								iCurrentTextureForPaint--;
						}
					}

					ImGui::PopItemWidth();

					ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() - (iSmallPreviewYMargin*2.0)));
#endif

				}
			}
			else {
				//Delete old image.
				if (ImageExist(sTerrainTexturesID[iL]) == 1) DeleteImage(sTerrainTexturesID[iL]);
				//sTerrainTexturesID[iL] = 0;
			}

		}

		#define REMOVETERRAINTEXTUREDIALOG
		static bool terrain_selection_window = false;
		static int iSelectedTerrainTexture = -1;
		if (iSelectTexture >= 0)
		{
			iSelectedTerrainTexture = iSelectTexture;
			#ifndef REMOVETERRAINTEXTUREDIALOG
			terrain_selection_window = true;
			iSelectTexture = 0;
			#else
			iSelectTexture = 999;
			#endif
		}

		#ifndef REMOVETERRAINTEXTUREDIALOG
		iSelectTexture = imgui_get_selections(terrain_selections, terrain_selections_text, 2, 380, &terrain_selection_window);
		#endif
		if (iSelectTexture >= 0 && iSelectedTerrainTexture >= 0)
		{
			bool bNeedFileSelector = false;
			if (iSelectTexture == 999)
				bNeedFileSelector = true;

			terrain_selection_window = false;

			int iNewTerrainIndex = iSelectTexture;
			iSelectTexture = iSelectedTerrainTexture;

			if (iSelectTexture == 99) {
				//Find free slot.
				for (int iL2 = 0; iL2 < 32; iL2++) {
					if (t.visuals.sTerrainTextures[iL2] == "") {
						iSelectTexture = iL2;
						break;
					}
				}
			}

			if (!bNeedFileSelector) {
				t.visuals.sTerrainTextures[iSelectTexture] = terrain_selections[iNewTerrainIndex];
				if (ImageExist(sTerrainTexturesID[iSelectTexture]))
					iDeleteSingleTerrainTextures = sTerrainTexturesID[iSelectTexture];
				iCurrentTextureForPaint = iSelectTexture;
				bUpdateTerrainMaterials = true;
			}

			if (bNeedFileSelector)
			{

				//iSelectTexture
				cStr tOldDir = GetDir();
				char * cFileSelected;
				cFileSelected = (char *)noc_file_dialog_open(NOC_FILE_DIALOG_OPEN, "All\0*.*\0DDS\0*.dds\0PNG\0*.png\0JPEG\0*.jpg\0TGA\0*.tga\0BMP\0*.bmp\0\0\0", sTextureFolder.Get(), NULL);
				SetDir(tOldDir.Get());

				if (cFileSelected && strlen(cFileSelected) > 0)
				{
					char *relonly = (char *)pestrcasestr(cFileSelected, g.rootdir_s.Get());

					t.visuals.sTerrainTextures[iSelectTexture] = cFileSelected;
					t.visuals.sTerrainTexturesName[iSelectTexture] = ""; //PE: User must enter a proper name.
					bTextureNameWindow[iSelectTexture] = true;
					if (relonly) {
						t.visuals.sTerrainTextures[iSelectTexture] = cFileSelected + g.rootdir_s.Len();
					}
					g.projectmodified = 1;
					t.gamevisuals.sTerrainTextures[iSelectTexture] = t.visuals.sTerrainTextures[iSelectTexture];
					t.gamevisuals.sTerrainTexturesName[iSelectTexture] = t.visuals.sTerrainTexturesName[iSelectTexture];
					bUpdateTerrainMaterials = true;
					iCurrentTextureForPaint = iSelectTexture;

					//PE: Reload image.
					if (ImageExist(sTerrainTexturesID[iSelectTexture]))
						iDeleteSingleTerrainTextures = sTerrainTexturesID[iSelectTexture];
				}
			}
			iSelectTexture = -1;
		}

		if (bUpdateTerrainMaterials)
		{
			bUpdateTerrainMaterials = false;
		}

		ImGui::Indent(-10);
		ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));

	}
}


void imgui_Customize_Terrain_v3(int mode)
{
	int wflags = ImGuiTreeNodeFlags_DefaultOpen;
	if (mode == 1) wflags = ImGuiTreeNodeFlags_None;

	float media_icon_size = 40.0f;
	float w = ImGui::GetWindowContentRegionWidth();
	float plate_width = (media_icon_size + 6.0) * 4.0f;
	if (ImGui::StyleCollapsingHeader("Palette", wflags))
	{
		//Drpo down.
		ImGui::Indent(10);
		ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));


		//Use "texturebank" as default folder.

		if (iDeleteSingleTerrainTextures > 0) {
			if (ImageExist(iDeleteSingleTerrainTextures))
				DeleteImage(iDeleteSingleTerrainTextures);
			iDeleteSingleTerrainTextures = 0;
		}
		if (iDeleteAllTerrainTextures) {
			iDeleteAllTerrainTextures = false;
			for (int iL = 0; iL < 32; iL++) {
				sTerrainTexturesID[iL] = t.terrain.imagestartindex + 80 + iL;
				sTerrainSelectionID[iL] = iL;
				bTextureNameWindow[iL] = false;
				if (ImageExist(t.terrain.imagestartindex + 80 + iL))
					DeleteImage(t.terrain.imagestartindex + 80 + iL);
			}
			bUpdateTerrainMaterials = true;
			iOldMaterial = -1; //PE: Reset selection.
		}

		//PE: Saved textures in visual.ini is not really possible at the monent, so reset every time. t.visuals.sTerrainTextures[0] == ""
		//terraintextures\mat1\Color.dds
		if (t.visuals.customTexturesFolder.Len() == 0 && (sTerrainTexturesID[0] == 0 || t.visuals.sTerrainTextures[0] != "terraintextures\\mat1\\Color.dds" || t.visuals.sTerrainTexturesName[0] != "Rainforest Overgrowth" ))
		{
			//Add a few default textures.
			//PE: We always need a default texture in [0].

			for( int i = 0; i < 32; i++ )
			{
				char temp[ 1024 ];
				sprintf( temp, "terraintextures\\mat%d\\Color.dds", i+1 );
				t.visuals.sTerrainTextures[i] = temp;
				sprintf( temp, "Material %d", i+1 );
				t.visuals.sTerrainTexturesName[i] = temp;
				t.gamevisuals.sTerrainTextures[i] = t.visuals.sTerrainTextures[i];
				t.gamevisuals.sTerrainTexturesName[i] = t.visuals.sTerrainTexturesName[i];
				
				//PE: Texture name List from Mark.
				if (i + 1 == 1) t.visuals.sTerrainTexturesName[i] = "Rainforest Overgrowth";
				if (i + 1 == 2) t.visuals.sTerrainTexturesName[i] = "Wet Shoreline Sand";
				if (i + 1 == 3) t.visuals.sTerrainTexturesName[i] = "Grassy Clover";
				if (i + 1 == 4) t.visuals.sTerrainTexturesName[i] = "Dry Beach Sand";
				if (i + 1 == 5) t.visuals.sTerrainTexturesName[i] = "Cliff Rock with Weeds";
				if (i + 1 == 6) t.visuals.sTerrainTexturesName[i] = "Desert Gravel Path";
				if (i + 1 == 7) t.visuals.sTerrainTexturesName[i] = "Desert Jagged Rock";
				if (i + 1 == 8) t.visuals.sTerrainTexturesName[i] = "Desert Sand Flat";
				if (i + 1 == 9) t.visuals.sTerrainTexturesName[i] = "Melting Snow";
				if (i + 1 == 10) t.visuals.sTerrainTexturesName[i] = "Shoreline Mud";
				if (i + 1 == 11) t.visuals.sTerrainTexturesName[i] = "Snowy Rock";
				if (i + 1 == 12) t.visuals.sTerrainTexturesName[i] = "Trampled Snow";
				if (i + 1 == 13) t.visuals.sTerrainTexturesName[i] = "Virgin Snow";
				if (i + 1 == 14) t.visuals.sTerrainTexturesName[i] = "Cracked Ground";
				if (i + 1 == 15) t.visuals.sTerrainTexturesName[i] = "Canyon Gravel Path";
				if (i + 1 == 16) t.visuals.sTerrainTexturesName[i] = "Patchy Grass";
				if (i + 1 == 17) t.visuals.sTerrainTexturesName[i] = "Canyon Cliff Wall";
				if (i + 1 == 18) t.visuals.sTerrainTexturesName[i] = "Chunky Rock";
				if (i + 1 == 19) t.visuals.sTerrainTexturesName[i] = "Cliff Rock";
				if (i + 1 == 20) t.visuals.sTerrainTexturesName[i] = "Moss";
				if (i + 1 == 21) t.visuals.sTerrainTexturesName[i] = "Mossy Rock";
				if (i + 1 == 22) t.visuals.sTerrainTexturesName[i] = "Riverbed Stones";
				if (i + 1 == 23) t.visuals.sTerrainTexturesName[i] = "Rocky Mountain Ground";
				if (i + 1 == 24) t.visuals.sTerrainTexturesName[i] = "Ferns";
				if (i + 1 == 25) t.visuals.sTerrainTexturesName[i] = "Grass Dense";
				if (i + 1 == 26) t.visuals.sTerrainTexturesName[i] = "Rocky Soil";
				if (i + 1 == 27) t.visuals.sTerrainTexturesName[i] = "Woodland Cliff Rock";
				if (i + 1 == 28) t.visuals.sTerrainTexturesName[i] = "Dirt Path";
				if (i + 1 == 29) t.visuals.sTerrainTexturesName[i] = "Field Grass";
				if (i + 1 == 30) t.visuals.sTerrainTexturesName[i] = "Forest Ground";
				if (i + 1 == 31) t.visuals.sTerrainTexturesName[i] = "Wet Riverbed Gravel";
				if (i + 1 == 32) t.visuals.sTerrainTexturesName[i] = "Square Pattern";
			}

			for (int iL = 0; iL < 32; iL++) {
				sTerrainSelectionID[iL] = iL;
				sTerrainTexturesID[iL] = t.terrain.imagestartindex + 80 + iL;
				bTextureNameWindow[iL] = false;
			}

			init_terrain_selections();
			//PE: We now activate if not exists, when selecting, so no need to reset all textures on a fresh palette.
			bUpdateTerrainMaterials = false;
		}
		//PE: Make sure we always have the correct ids, even if custom textures.
		if (sTerrainTexturesID[0] == 0)
		{
			for (int iL = 0; iL < 32; iL++) {
				sTerrainSelectionID[iL] = iL;
				sTerrainTexturesID[iL] = t.terrain.imagestartindex + 80 + iL;
				bTextureNameWindow[iL] = false;
			}
		}
		int iUsedImages = 0;
		cStr sTextureFolder = g.rootdir_s + "texturebank\\";
		float col_start = 100.0f;
		int iSelectTexture = -1;

#ifdef DISPLAY4x4
		ImGui::Indent(-10);

		static float fLast4x4Height = 228.0;
		static float fLastMaxY = 0.0;
		if (fLastMaxY > 100.0 && fLastMaxY < 400.0)
		{
			fLast4x4Height = fLastMaxY;
		}

		ImVec2 oldstylemain = ImGui::GetStyle().FramePadding;
		ImVec2 oldwinstylemain = ImGui::GetStyle().WindowPadding;
		ImGui::GetStyle().WindowPadding = { 0,0 };
		ImGui::GetStyle().FramePadding = { 0,0 };
		ImVec2 vWindowPos = ImGui::GetWindowPos();
		ImVec2 vWindowSize = ImGui::GetWindowSize();

		ImGui::BeginChild("##terrain4x4scrollbar", ImVec2(0, fLast4x4Height), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);

		w = ImGui::GetWindowContentRegionWidth(); //PE: Minus scrollbar.
		ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(0.0, 2.0));

		ImGui::Columns(4, "##terrain4x4columns", false);  //false no border

#endif

		for (int iL = 0; iL < 32; iL++)
		{
			if (t.visuals.sTerrainTextures[iL] != "")
			{
				if (!ImageExist(sTerrainTexturesID[iL]))
				{
					//PE: This loop leaks, somewhere. even if all images has been deleted.
					//PE: We dont really need this , so now we dont reload on new level, load level. (iDeleteAllTerrainTextures=false).

					//Load in image.
					image_setlegacyimageloading(true);
					SetMipmapNum(1); //PE: mipmaps not needed.
					if (ImageExist(sTerrainTexturesID[iL]) == 1) DeleteImage(sTerrainTexturesID[iL]);

					// sTerrainTextures stores as (uncompressed) to speed up material loading as raw data, but 
					// here they can be compressed for quicker loading, so..
					char pCompressedVersionIfAny[MAX_PATH];
					strcpy(pCompressedVersionIfAny, t.visuals.sTerrainTextures[iL].Get());
					int iLen = strlen(pCompressedVersionIfAny) - strlen(" (uncompressed).dds");
					//PE: Make sure we dont crash if using small filenames like "a.dds".
					if (iLen > 0)
					{
						pCompressedVersionIfAny[iLen] = 0;
						strcat(pCompressedVersionIfAny, ".dds");
						if (FileExist(pCompressedVersionIfAny) == 0)
						{
							strcpy(pCompressedVersionIfAny, t.visuals.sTerrainTextures[iL].Get());
						}
					}
					LoadImage(pCompressedVersionIfAny, sTerrainTexturesID[iL], 0, g.gdividetexturesize);
					if (ImageExist(sTerrainTexturesID[iL]) == 1)
					{
					}
					else 
					{
						//Load failed, clear texture slot.
						t.visuals.sTerrainTextures[iL] = "";
						t.gamevisuals.sTerrainTextures[iL] = t.visuals.sTerrainTextures[iL];
						g.projectmodified = 1;
						bUpdateTerrainMaterials = true;
					}
					SetMipmapNum(-1);
					image_setlegacyimageloading(false);
				}

				if (ImageExist(sTerrainTexturesID[iL]))
				{

					if (bTextureNameWindow[iL]) {
						//Ask for a proper name of texture.

						ImGui::SetNextWindowSize(ImVec2(26 * ImGui::GetFontSize(), 32 * ImGui::GetFontSize()), ImGuiCond_Once);
						ImGui::SetNextWindowPosCenter(ImGuiCond_Once);
						cstr sUniqueWinName = cstr("Terrain Texture Name##ttn") + cstr(iL);
						ImGui::Begin(sUniqueWinName.Get(), &bTextureNameWindow[iL], 0);
						ImGui::Indent(10);
						static char NewTextureName[256];
						cstr sUniqueInputName = cstr("##InoutTerrainName") + cstr(iL);
						float content_width = ImGui::GetContentRegionAvailWidth() - 10.0;
						ImGui::ImgBtn(sTerrainTexturesID[iL], ImVec2(content_width, content_width), ImColor(0, 0, 0, 255));
						ImGui::PushItemWidth(-10);
						ImGui::Text("Enter a name for your terrain texture:");
						if (ImGui::IsRootWindowOrAnyChildFocused() && !ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0))
							ImGui::SetKeyboardFocusHere(0);
						if (ImGui::InputText(sUniqueInputName.Get(), t.visuals.sTerrainTexturesName[iL].Get(), 250, ImGuiInputTextFlags_EnterReturnsTrue)) {
							t.gamevisuals.sTerrainTexturesName[iL] = t.visuals.sTerrainTexturesName[iL];
							bTextureNameWindow[iL] = false;
						}

						if (ImGui::MaxIsItemFocused()) bImGuiGotFocus = true;

						ImGui::PopItemWidth();
						ImGui::Indent(-10);
						if (ImGui::GetCurrentWindow()->ScrollbarSizes.x > 0) {
							//Hitting exactly at the botton could cause flicker, so add some additional lines when scrollbar on.
							ImGui::Text("");
							ImGui::Text("");
						}
						bImGuiGotFocus = true;
						ImGui::End();
					}



					iUsedImages++;

#ifndef DISPLAY4x4
					if (iUsedImages < 32 && iL == 0)
					{
						float but_gadget_size = ImGui::GetFontSize()*10.0;
						ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((w*0.5) - (but_gadget_size*0.5), 0.0f));

						if (ImGui::StyleButton("Add Texture", ImVec2(but_gadget_size, 0))) {
							iSelectTexture = 99;
						}
						ImGui::Separator();
					}

					char materialname[256];
					strcpy(materialname, t.visuals.sTerrainTextures[iL].Get());
#endif
					float path_gadget_size = ImGui::GetFontSize()*3.0;
					int preview_icon_size = ImGui::GetFontSize();

					bool bDeleteImage = true;
					if (iL == 0)
						bDeleteImage = false;

					//TEXTURE.

					cStr sLabel = cStr("Texture##InputTerrainTexture") + cStr(iL);
					if (!bDeleteImage)
						sLabel = cStr("Default##InputTerrainTexture") + cStr(iL);

					//PE: New do 4x4 coloums here.
#ifdef DISPLAY4x4
					int iLargerPreviewIconSize = 28;//PE: 54 , now lowest possible icon
					float control_width = (iLargerPreviewIconSize + 3.0) * 4.0f + 6.0;

					if (w > control_width) {
						//PE: fit perfectly with window width.
						iLargerPreviewIconSize = (w - 20.0) / 4.0;
						iLargerPreviewIconSize -= 6.0; //Padding.
						if (iLargerPreviewIconSize > 70) iLargerPreviewIconSize = 70;
					}

					ImVec2 vSelectionDraw = ImGui::GetCurrentWindow()->DC.CursorPos;
					if (sTerrainTexturesID[iL] > 0)
					{

						if (iL == 0) {
							CheckTutorialAction("TOOL_TERRAIN_SAND", -20.0f); //Tutorial: check if we are waiting for this action
						}
						if (iL == 2) {
							CheckTutorialAction("TOOL_TERRAIN_ROCK", -20.0f); //Tutorial: check if we are waiting for this action
						}

						cStr sLabelChild = cStr("##terrain4x4") + cStr(iL);

						ImVec2 content_avail = { iLargerPreviewIconSize + 1.0f ,iLargerPreviewIconSize + 1.0f };

						//style.WindowPadding
						ImVec2 oldstyle = ImGui::GetStyle().FramePadding;
						ImGui::GetStyle().FramePadding = { 1,1 };
						ImGui::BeginChild(sLabelChild.Get(), content_avail, false, ImGuiWindowFlags_NoScrollbar);

						iLargerPreviewIconSize &= 0xfffe;

						ImGui::SetBlurMode(true);
						if (ImGui::ImgBtn(sTerrainTexturesID[iL], ImVec2(iLargerPreviewIconSize, iLargerPreviewIconSize), ImColor(0, 0, 0, 255)))
						{
							//iSelectTexture = iL; //PE: Just select image.
							if (iL == 0 && bTutorialCheckAction) TutorialNextAction();
							if (iL == 2 && bTutorialCheckAction) TutorialNextAction();
							iCurrentTextureForPaint = sTerrainSelectionID[iL];
							ggterrain_extra_params.paint_material = iCurrentTextureForPaint + 1; //iL + 1;
							iTerrainPaintMode = 1;
						}
						ImGui::SetBlurMode(false);

						// Our buttons are both drag sources and drag targets here!
						if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
						{
							ImGui::SetDragDropPayload("DND_TERRAIN_TEXTURES", &iL, sizeof(int));
							ImGui::Text("Swap Texture");
							ImGui::EndDragDropSource();
						}
						if (ImGui::BeginDragDropTarget())
						{
							if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_TERRAIN_TEXTURES"))
							{
								IM_ASSERT(payload->DataSize == sizeof(int));
								int payload_n = *(const int*)payload->Data;

								if (payload_n >= 0 && payload_n < 32)
								{
									//PE: Swap iL and payload_n
									cStr sTmp = t.visuals.sTerrainTextures[iL];
									cStr sTmp2 = t.visuals.sTerrainTexturesName[iL];
									int iTmp = sTerrainTexturesID[iL];
									int iTmp2 = sTerrainSelectionID[iL];

									t.visuals.sTerrainTextures[iL] = t.visuals.sTerrainTextures[payload_n];
									t.visuals.sTerrainTexturesName[iL] = t.visuals.sTerrainTexturesName[payload_n];
									sTerrainTexturesID[iL] = sTerrainTexturesID[payload_n];
									sTerrainSelectionID[iL] = sTerrainSelectionID[payload_n];

									t.visuals.sTerrainTextures[payload_n] = sTmp;
									t.visuals.sTerrainTexturesName[payload_n] = sTmp2;
									sTerrainTexturesID[payload_n] = iTmp;
									sTerrainSelectionID[payload_n] = iTmp2;

									t.gamevisuals.sTerrainTextures[iL] = t.visuals.sTerrainTextures[iL];
									t.gamevisuals.sTerrainTexturesName[iL] = t.visuals.sTerrainTexturesName[iL];
									t.gamevisuals.sTerrainTextures[payload_n] = t.visuals.sTerrainTextures[payload_n];
									t.gamevisuals.sTerrainTexturesName[payload_n] = t.visuals.sTerrainTexturesName[payload_n];
								}
							}
							ImGui::EndDragDropTarget();
						}


						bool bInContext = false;
						static int iCurrentTerrainContext = -1;

						if (!bInContext && ImGui::IsItemHovered())
						{
							int tooltip_height = 350;
							int tooltip_width = 350;
							ImVec2 tooltip_position = ImVec2(ImGui::GetWindowPos());
							tooltip_position.x = vWindowPos.x - (tooltip_width + 10.0);
							if ((tooltip_position.y + tooltip_height) > (vWindowPos.y + vWindowSize.y))
								tooltip_position.y = (vWindowPos.y + vWindowSize.y) - tooltip_height - 10.0;
							ImGui::SetNextWindowPos(tooltip_position);

							ImGui::BeginTooltip();
							ImGui::ImgBtn(sTerrainTexturesID[iL], ImVec2(350, 350), ImColor(0, 0, 0, 255));
							ImGui::TextCenter(t.visuals.sTerrainTexturesName[iL].Get());
							ImGui::Separator();
							ImGui::EndTooltip();
						}

						ImGui::EndChild();
						ImGui::GetStyle().FramePadding = oldstyle;
					}

					if (iCurrentTextureForPaint == sTerrainSelectionID[iL]) //sTerrainTexturesID[iL] - t.terrain.imagestartindex - 80)
					{
						ImVec2 padding = { 2.0, 2.0 };
						const ImRect image_bb((vSelectionDraw - padding), vSelectionDraw + padding + ImVec2(iLargerPreviewIconSize, iLargerPreviewIconSize));
						ImGui::GetCurrentWindow()->DrawList->AddRect(image_bb.Min, image_bb.Max, ImGui::GetColorU32(tool_selected_col), 0.0f, 15, 2.0f);
					}

					//					sLabel = cStr("##tradio") + cStr(iL);
					//					float checkwidth = ImGui::GetFontSize()*1.5;
					//					ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((ImGui::GetContentRegionAvail().x*0.5) - (checkwidth*0.5), 0.0f));

					ImGui::NextColumn();

					if (iUsedImages == 1)
					{
						fLastMaxY = ImGui::GetCursorPosY();
					}
					if (iUsedImages == 17)
					{
						fLastMaxY = ImGui::GetCursorPosY() - fLastMaxY;
					}

#else
					ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));

					if (iL == 0) {
						CheckTutorialAction("TOOL_TERRAIN_SAND", -20.0f); //Tutorial: check if we are waiting for this action
					}

					if (ImGui::RadioButton(sLabel.Get(), &iCurrentTextureForPaint, iL)) {
						if (iL == 0 && bTutorialCheckAction) TutorialNextAction();
					}
					if (iOldMaterial != iCurrentTextureForPaint)
					{
						//@Michael + 1 should be removed when your ready, this is just to follow your current test code :)
						if (g_pTerrain)
						{
							if (g_pTerrain->FindMaterial(cStr(iCurrentTextureForPaint).Get()) != nullptr)
							{
								g_pTerrain->SetMaterial(cStr(iCurrentTextureForPaint).Get());
							}

							iOldMaterial = iCurrentTextureForPaint;
						}
					}

					ImGui::SameLine();

					int iSmallPreviewYMargin = preview_icon_size / 2;
					int iLargerPreviewIconSize = preview_icon_size * 2;
					ImGui::SetCursorPos(ImVec2(col_start - 10.0f, ImGui::GetCursorPosY() - iSmallPreviewYMargin));
					//					ImGui::SetCursorPos(ImVec2(col_start, ImGui::GetCursorPosY() - 3));

					if (sTerrainTexturesID[iL] > 0)
					{
						if (ImGui::ImgBtn(sTerrainTexturesID[iL], ImVec2(iLargerPreviewIconSize, iLargerPreviewIconSize), ImColor(0, 0, 0, 255)))
						{
							iSelectTexture = iL;
						}

						if (ImGui::IsItemHovered())
						{
							ImGui::BeginTooltip();
							ImGui::ImgBtn(sTerrainTexturesID[iL], ImVec2(180, 180), ImColor(0, 0, 0, 255));
							ImGui::EndTooltip();
						}
						ImGui::SameLine();
					}

					ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + iSmallPreviewYMargin));

					int iInputFlags = ImGuiInputTextFlags_EnterReturnsTrue;
					if (!bDeleteImage)
						iInputFlags = ImGuiInputTextFlags_ReadOnly;

					sLabel = cStr("##InputTerrainTexture") + cStr(iL);
					ImGui::PushItemWidth(-10 - path_gadget_size);
					if (ImGui::InputText(sLabel.Get(), &materialname[0], 250, iInputFlags))
					{
						if (strlen(materialname) == 0)
							t.visuals.sTerrainTextures[iL] = ""; //delete image in next run.
						t.gamevisuals.sTerrainTextures[iL] = t.visuals.sTerrainTextures[iL];
						g.projectmodified = 1;
						bUpdateTerrainMaterials = true;
						if (iCurrentTextureForPaint == iL)
							iCurrentTextureForPaint--;

					}
					ImGui::PopItemWidth();

					ImGui::SameLine();
					ImGui::PushItemWidth(path_gadget_size);
					sLabel = cStr("...##InputTerrainTexture") + cStr(iL);

					if (ImGui::StyleButton(sLabel.Get()))
						iSelectTexture = iL;

					if (bDeleteImage) {
						ImGui::SameLine();
						sLabel = cStr("X##InputTerrainTexture") + cStr(iL);
						if (ImGui::StyleButton(sLabel.Get())) {
							t.visuals.sTerrainTextures[iL] = ""; //delete image in next run.
							t.gamevisuals.sTerrainTextures[iL] = t.visuals.sTerrainTextures[iL];
							g.projectmodified = 1;
							bUpdateTerrainMaterials = true;
							if (iCurrentTextureForPaint == iL)
								iCurrentTextureForPaint--;
						}
					}

					ImGui::PopItemWidth();

					ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() - (iSmallPreviewYMargin*2.0)));
#endif

				}
			}
			else {
				//Delete old image.
				if (ImageExist(sTerrainTexturesID[iL]) == 1) DeleteImage(sTerrainTexturesID[iL]);
			}

		}

#ifdef DISPLAY4x4

		ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(0.0, 3.0));

		ImGui::EndChild();

		ImGui::GetStyle().WindowPadding = oldwinstylemain;
		ImGui::GetStyle().FramePadding = oldstylemain;

		ImGui::Columns(1);
		ImGui::Indent(10);

		#ifdef CUSTOMTEXTURES
		float avail = ImGui::GetContentRegionAvailWidth();
		ImVec2 buttonSize = ImVec2(195, 23);
		float width = 0.0f;
		width += buttonSize.x;
		width += ImGui::GetStyle().ItemSpacing.x;
		float off = (avail - width) * 0.5f;
		if (off > 0.0f)
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + off);
		if (ImGui::Button("Change texture folder", buttonSize))
		{
			ChooseTerrainTextureFolder();
		}
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Use textures from a folder elsewhere on your system");
		}

		//LB: moved out of GGTerrain so can access global structures
		if (g_iDeferTextureUpdateToNow == 3)
		{
			// ensure invalid terrain textures are not allowed to be used
			for (auto& id : g_DeferTextureUpdateIncompatibleTextures)
			{
				char* name = t.visuals.sTerrainTextures[id].Get();
				int nameLength = t.visuals.sTerrainTextures[id].Len();
				if (strcmp(name + nameLength - strlen("mat32\\Color.dds"), "mat32\\Color.dds") != 0)
				{
					t.visuals.sTerrainTextures[id] = "";
				}
			}
			if (g_DeferTextureUpdateIncompatibleTextures.size() == g_DeferTextureUpdate.size())
			{
				// No textures in this folder are useable
				extern bool bTriggerMessage;
				extern char cTriggerMessage[MAX_PATH];
				bTriggerMessage = true;
				strcpy(cTriggerMessage, "No compatible textures");// , try PNG format");
				ResetTextureSettings();
			}

			// textures updated, we can finish this state machine :)
			g_iDeferTextureUpdateToNow = 0;
		}

		width = 0.0f;
		width += buttonSize.x;
		width += ImGui::GetStyle().ItemSpacing.x;
		off = (avail - width) * 0.5f;
		if (off > 0.0f)
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + off);
		if (ImGui::Button("Reset texture settings", buttonSize))
		{
			ResetTextureSettings();
		}
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Restore the texture folder and default texture set to their original states");
		}
		#endif
#endif


#define REMOVETERRAINTEXTUREDIALOG
		static bool terrain_selection_window = false;
		static int iSelectedTerrainTexture = -1;
		if (iSelectTexture >= 0)
		{
			iSelectedTerrainTexture = iSelectTexture;
#ifndef REMOVETERRAINTEXTUREDIALOG
			terrain_selection_window = true;
			iSelectTexture = 0;
#else
			iSelectTexture = 999;
#endif
		}

#ifndef REMOVETERRAINTEXTUREDIALOG
		iSelectTexture = imgui_get_selections(terrain_selections, terrain_selections_text, 2, 380, &terrain_selection_window);
#endif
		if (iSelectTexture >= 0 && iSelectedTerrainTexture >= 0)
		{
			bool bNeedFileSelector = false;
			if (iSelectTexture == 999)
				bNeedFileSelector = true;

			terrain_selection_window = false;

			int iNewTerrainIndex = iSelectTexture;
			iSelectTexture = iSelectedTerrainTexture;

			if (iSelectTexture == 99) {
				//Find free slot.
				for (int iL2 = 0; iL2 < 32; iL2++) {
					if (t.visuals.sTerrainTextures[iL2] == "") {
						iSelectTexture = iL2;
						break;
					}
				}
			}

			if (!bNeedFileSelector) {
				t.visuals.sTerrainTextures[iSelectTexture] = terrain_selections[iNewTerrainIndex];
				if (ImageExist(sTerrainTexturesID[iSelectTexture]))
					iDeleteSingleTerrainTextures = sTerrainTexturesID[iSelectTexture];
				iCurrentTextureForPaint = iSelectTexture;
				bUpdateTerrainMaterials = true;
			}

			if (bNeedFileSelector)
			{

				//iSelectTexture
				cStr tOldDir = GetDir();
				char * cFileSelected;
				cFileSelected = (char *)noc_file_dialog_open(NOC_FILE_DIALOG_OPEN, "All\0*.*\0DDS\0*.dds\0PNG\0*.png\0JPEG\0*.jpg\0TGA\0*.tga\0BMP\0*.bmp\0\0\0", sTextureFolder.Get(), NULL);
				SetDir(tOldDir.Get());

				if (cFileSelected && strlen(cFileSelected) > 0)
				{
					char *relonly = (char *)pestrcasestr(cFileSelected, g.rootdir_s.Get());

					t.visuals.sTerrainTextures[iSelectTexture] = cFileSelected;
					t.visuals.sTerrainTexturesName[iSelectTexture] = ""; //PE: User must enter a proper name.
					bTextureNameWindow[iSelectTexture] = true;
					if (relonly) {
						t.visuals.sTerrainTextures[iSelectTexture] = cFileSelected + g.rootdir_s.Len();
					}
					g.projectmodified = 1;
					t.gamevisuals.sTerrainTextures[iSelectTexture] = t.visuals.sTerrainTextures[iSelectTexture];
					t.gamevisuals.sTerrainTexturesName[iSelectTexture] = t.visuals.sTerrainTexturesName[iSelectTexture];
					bUpdateTerrainMaterials = true;
					iCurrentTextureForPaint = iSelectTexture;

					//PE: Reload image.
					if (ImageExist(sTerrainTexturesID[iSelectTexture]))
						iDeleteSingleTerrainTextures = sTerrainTexturesID[iSelectTexture];
				}
			}
			iSelectTexture = -1;
		}

		if (bUpdateTerrainMaterials)
		{
			bUpdateTerrainMaterials = false;
		}

		ImGui::Indent(-10);
		ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));
	}
	if (t.visuals.customTexturesFolder.Len() > 0)
	{
		if (ImGui::StyleCollapsingHeader("Palette Properties", 0))
		{
			bool bOpenMaterialSelector = false;

			ImGui::Indent(10);
			ImGui::TextCenter("Terrain Layers");
			ImGui::Spacing();
			const char* materialNames[5] = {"Lowest Layer", "Low Layer", "Middle Layer", "High Layer", "Highest Layer"};
			int startNameIndexOffset = 0;
			static int layerMatIndex = -1;
			for (int i = 4; i >= 0; i--)
			{
				if (ggterrain_global_render_params.layerStartHeight[i] != ggterrain_global_render_params.layerEndHeight[i])
				{
					int index = ggterrain_global_render_params.layerMatIndex[i] & 0xFF;
					bool rotate = (ggterrain_global_render_params.layerMatIndex[i] >> 8) != 0;
					ImGui::PushID(234355 + i);
					ImGui::Text(materialNames[i + startNameIndexOffset]);
					int texID = sTerrainTexturesID[index];
					if (ImageExist(texID) == 0)
					{
						for (int ID = 31; ID >= 0; ID--)
						{
							if (t.visuals.sTerrainTextures[ID].Len() > 0)
							{
								texID = sTerrainTexturesID[ID];
								index = ID;
								break;
							}
						}
					}
					ImGui::SameLine();
					ImGui::SetCursorPos(ImVec2(ImGui::GetWindowWidth() / 2.0f - 25, ImGui::GetCursorPosY()-8));
					if (ImGui::ImgBtn(texID, ImVec2(50, 25)))
					{
						bOpenMaterialSelector = true;
						layerMatIndex = i;
					}
					ImGui::PopID();
				}
				else
				{
					if (startNameIndexOffset == 0)
					{
						// Display "Low Layer", "Middle Layer", "High Layer" instead of the five material names (lowest and highest not used)
						startNameIndexOffset++;
					}
				}
			}
			int index = ggterrain_global_render_params.baseLayerMaterial & 0xFF;
			bool rotate = (ggterrain_global_render_params.baseLayerMaterial >> 8) != 0;
			ImGui::PushID(255555);
			ImGui::Text("Underwater");
			static int underWaterLayer = -1;
			int texID = sTerrainTexturesID[index];
			if (ImageExist(texID) == 0)
			{
				for (int ID = 31; ID >= 0; ID--)
				{
					if (t.visuals.sTerrainTextures[ID].Len() > 0)
					{
						texID = sTerrainTexturesID[ID];
						index = ID;
						break;
					}
				}
			}
			ImGui::SameLine();
			ImGui::SetCursorPos(ImVec2(ImGui::GetWindowWidth() / 2.0f - 25, ImGui::GetCursorPosY() - 8));
			if (ImGui::ImgBtn(texID, ImVec2(50, 25)))
			{
				bOpenMaterialSelector = true;
				underWaterLayer = 1;
			}
			ImGui::PopID();
			ImGui::TextCenter("Slope Layers");
			ImGui::Spacing();
			const char* slopeNames[2] = { "Main Slope", "Secondary Slope" };
			static int slopeMatIndex = -1;
			for (int i = 0; i < 2; i++)
			{
				if (ggterrain_global_render_params.slopeStart[i] != ggterrain_global_render_params.slopeEnd[i])
				{
					int index = ggterrain_global_render_params.slopeMatIndex[i] & 0xFF;
					bool rotate = (ggterrain_global_render_params.slopeMatIndex[i] >> 8) != 0;
					ImGui::PushID(234000 + i);
					ImGui::Text(slopeNames[i]);
					int texID = sTerrainTexturesID[index];
					if (ImageExist(texID) == 0)
					{
						for (int ID = 31; ID >= 0; ID--)
						{
							if (t.visuals.sTerrainTextures[ID].Len() > 0)
							{
								texID = sTerrainTexturesID[ID];
								index = ID;
								break;
							}
						}
					}
					ImGui::SameLine();
					ImGui::SetCursorPos(ImVec2(ImGui::GetWindowWidth() / 2.0f - 25, ImGui::GetCursorPosY() - 8));
					if (ImGui::ImgBtn(texID, ImVec2(50, 25)))
					{
						bOpenMaterialSelector = true;
						slopeMatIndex = i;
					}
					ImGui::PopID();
				}
			}

			ImGui::Spacing();
			ImGui::Spacing();

			if (bOpenMaterialSelector)
			{
				ImGui::OpenPopup("##texturepalette");
			}
			if (ImGui::BeginPopup("##texturepalette"))
			{
				for (int i = 0; i < GGTERRAIN_MAX_SOURCE_TEXTURES; i++)
				{
					ImGui::PushID(88888+ i);
					if (i % 3 != 0)
						ImGui::SameLine();
					if (ImGui::ImgBtn(sTerrainTexturesID[i], ImVec2(50.0f, 50.0f)))
					{
						// Check if we are altering height material or slope material
						int* matIndexToChange = nullptr;
						if (layerMatIndex >= 0)
						{
							matIndexToChange = &ggterrain_global_render_params.layerMatIndex[layerMatIndex];
						}
						else if (slopeMatIndex >= 0)
						{
							matIndexToChange = &ggterrain_global_render_params.slopeMatIndex[slopeMatIndex];
						}
						else if (underWaterLayer >= 0)
						{
							matIndexToChange = &ggterrain_global_render_params.baseLayerMaterial;
						}

						// Update the layer material
						if (matIndexToChange)
						{
							*matIndexToChange = 0x100 | i;
						}

						layerMatIndex = -1;
						slopeMatIndex = -1;
						// Trigger update of material sounds
						extern bool g_bMapMatIDToMatIndexAvailable;
						g_bMapMatIDToMatIndexAvailable = false;
						ImGui::CloseCurrentPopup();
					}
					ImGui::PopID();
				}
				ImGui::EndPopup();
			}
			
			ImGui::TextCenter("Texture Material Type");
			ImGui::GetStyle().ItemSpacing.y -= 10;
			std::array<int, 8> matTypes = {
				0, // Grass
				1, // Stone
				2, // Metal
				3, // Wood
				6, // Snow
				10,// Tarmac (generic)
				11,// Dirt
				13 // Sand
			};
			std::array<const char*, 8> matTypeNames = {
				"Grass",
				"Stone",
				"Metal",
				"Wood",
				"Snow",
				"Generic",
				"Dirt",
				"Sand"
			};
			for (int i = 0; i < GGTERRAIN_MAX_SOURCE_TEXTURES; i++)
			{
				if (t.visuals.sTerrainTextures[i].Len() > 0)
				{
					ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
					ImGui::ImgBtn(sTerrainTexturesID[i], ImVec2(50, 25));
					ImGui::PopItemFlag();
					ImGui::SameLine();
					ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 4.0f));
					bool b = false;
					char comboName[64];
					sprintf_s(comboName, "##materialtype%d", i);
					char selectedMat[64];
					int val = g_iCustomTerrainMatSounds[i];
					for (int j= 0; j < matTypes.size();j++)
					{
						if (val == matTypes[j])
						{
							strcpy(selectedMat, matTypeNames[j]);
							break;
						}
					}
					if (strlen(selectedMat) == 0)
					{
						strcpy(selectedMat, "Silent");
					}
					ImGui::PushItemWidth(-10);
					if (ImGui::BeginCombo(comboName, selectedMat))
					{
						ImGui::GetStyle().ItemSpacing.y += 10;
						for (int j = 0; j < matTypes.size(); j++)
						{
							if (ImGui::Selectable(matTypeNames[j]))
							{
								g_iCustomTerrainMatSounds[i] = matTypes[j];
								// Trigger update of material sounds
								extern bool g_bMapMatIDToMatIndexAvailable;
								g_bMapMatIDToMatIndexAvailable = false;
							}
						}
						ImGui::GetStyle().ItemSpacing.y -= 10;
						ImGui::EndCombo();
					}
					ImGui::PopItemWidth();
				}
			}
			ImGui::GetStyle().ItemSpacing.y += 10;
			ImGui::Indent(-10);
			ImGui::Spacing();
		}
	}
}

int iDeleteSingleTreeTextures = 0;
int iDeleteAllTreeTextures = 0;
void imgui_Customize_Tree_v3(int mode)
{
	const uint32_t numTreeTypes = 64; // max tree types

	static cstr sTreeTextures[numTreeTypes];
	static cstr sTreeTexturesName[numTreeTypes];
	static int sTreeTexturesID[numTreeTypes];
	static bool bTreeSelected[numTreeTypes];
	static bool bTreeInit = true;


	int wflags = ImGuiTreeNodeFlags_DefaultOpen;
	if (mode == 1) wflags = ImGuiTreeNodeFlags_None;
	if (pref.bAutoClosePropertySections && mode == 1 && iLastOpenHeader != 2)
		ImGui::SetNextItemOpen(false, ImGuiCond_Always);

	float media_icon_size = 40.0f;
	float w = ImGui::GetWindowContentRegionWidth();
	float plate_width = (media_icon_size + 6.0) * 4.0f;
	if (ImGui::StyleCollapsingHeader("Palette", wflags))
	{
		if (mode == 1) iLastOpenHeader = 2;
		//Drpo down.
		ImGui::Indent(10);
		ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));


		if (iDeleteSingleTreeTextures > 0)
		{
			if (ImageExist(iDeleteSingleTreeTextures))
				DeleteImage(iDeleteSingleTreeTextures);
			iDeleteSingleTreeTextures = 0;
		}
		if (iDeleteAllTreeTextures)
		{
			iDeleteAllTreeTextures = false;
			for (int iL = 0; iL < GGTrees_GetNumTypes(); iL++)
			{
				sTreeTexturesID[iL] = t.terrain.imagestartindex + 280 + iL;
				if (ImageExist(sTreeTexturesID[iL]))
					DeleteImage(sTreeTexturesID[iL]);
			}
		}

		if (bTreeInit)
		{
			bTreeInit = false;

			for (int iL = 0; iL < GGTrees_GetNumTypes(); iL++) {
				sTreeTexturesID[iL] = t.terrain.imagestartindex + 280 + iL;
				sTreeTexturesName[iL] = "";
				sTreeTextures[0] = "";
				bTreeSelected[iL] = false;
			}
			bTreeSelected[0] = true;

			char path[ 1024 ];
			for( uint32_t i = 0; i < GGTrees_GetNumTypes(); i++ )
			{
				const char* textureName = GGTrees_GetTextureName(i);
				if ( textureName && *textureName )
				{
					strcpy_s( path, "treebank/billboards/" );
					strcat_s( path, GGTrees_GetTextureName(i) );
					sTreeTextures[i] = path;
				}
			}

			sTreeTexturesName[0]  = "Birch";
			sTreeTexturesName[1]  = "Cactus Var 1";
			sTreeTexturesName[2]  = "Cactus Var 2";
			sTreeTexturesName[3]  = "Cactus Var 3";
			sTreeTexturesName[4]  = "Cactus Var 4";
			sTreeTexturesName[5]  = "Dead Pine Tree";
			sTreeTexturesName[6]  = "Dry Pine";
			sTreeTexturesName[7]  = "Italian Pine";
			sTreeTexturesName[8]  = "Jungle Tree 1";
			sTreeTexturesName[9]  = "Jungle Tree 2";
			sTreeTexturesName[10] = "Jungle Tree 3a";
			sTreeTexturesName[11] = "Jungle Tree 3b";
			sTreeTexturesName[12] = "Jungle Tree 4a";
			sTreeTexturesName[13] = "Jungle Tree 4b";
			sTreeTexturesName[14] = "Jungle Tree 5a";
			sTreeTexturesName[15] = "Jungle Tree 5b";
			sTreeTexturesName[16] = "Jungle Tree 6a";
			sTreeTexturesName[17] = "Jungle Tree 6b";
			sTreeTexturesName[18] = "Kentia Palm";
			sTreeTexturesName[19] = "Palm";
			sTreeTexturesName[20] = "Pine";
			sTreeTexturesName[21] = "Scots Pine 1";
			sTreeTexturesName[22] = "Scots Pine 2";
			sTreeTexturesName[23] = "Scots Pine Dead";
			sTreeTexturesName[24] = "Snow Fir 2";
			sTreeTexturesName[25] = "Snow Fir 3";
			sTreeTexturesName[26] = "Snow Fir";
			sTreeTexturesName[27] = "Snow Pine";
			sTreeTexturesName[28] = "Snow Pine Tall 2";
			sTreeTexturesName[29] = "Snow Pine Tall";
			sTreeTexturesName[30] = "Sparse Pine";
			sTreeTexturesName[31] = "Vine Tree Large";
			sTreeTexturesName[32] = "Vine Tree Small";
			sTreeTexturesName[33] = "Western Pine";
			sTreeTexturesName[34] = "White Pine";
			sTreeTexturesName[35] = "Autumn Birch 1";
			sTreeTexturesName[36] = "Autumn Birch 2";
			sTreeTexturesName[37] = "Autumn Birch 3";
		}

		int iUsedImages = 0;
		float col_start = 100.0f;
		int iSelectTexture = -1;

		ImGui::Indent(-10);

		static float fLast4x4Height = 228.0;
		static float fLastMaxY = 0.0;
		if (fLastMaxY > 100.0 && fLastMaxY < 400.0)
		{
			fLast4x4Height = fLastMaxY+1.0;
		}

		ImVec2 oldstylemain = ImGui::GetStyle().FramePadding;
		ImVec2 oldwinstylemain = ImGui::GetStyle().WindowPadding;
		ImGui::GetStyle().WindowPadding = { 0,0 };
		ImGui::GetStyle().FramePadding = { 0,0 };
		ImVec2 vWindowPos = ImGui::GetWindowPos();
		ImVec2 vWindowSize = ImGui::GetWindowSize();

		ImGui::BeginChild("##Tree4x4scrollbar", ImVec2(0, fLast4x4Height), false, 0); //ImGuiWindowFlags_AlwaysVerticalScrollbar

		w = ImGui::GetWindowContentRegionWidth(); //PE: Minus scrollbar.
		ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(0.0, 2.0));

		ImGui::Columns(4, "##Tree4x4columns", false);  //false no border
		bool bGotSelection = false;
		ImRect image_selected_bb;
		uint64_t values = ggtrees_global_params.paint_tree_bitfield;
		if (values > 0x3000000000) //Max 2fffffffff
		{
			values = 1; //PE: Somethis is wrong , select default tree.
			ggtrees_global_params.paint_tree_bitfield = 1;
		}
		for (int iL = 0; iL < GGTrees_GetNumTypes(); iL++)
		{
			float imageScaleX = GGTrees_GetImageScale( iL );

			if (sTreeTextures[iL] != "")
			{
				if (!ImageExist(sTreeTexturesID[iL]))
				{
					//Load in image.
					image_setlegacyimageloading(true);
					SetMipmapNum(1); //PE: mipmaps not needed.
					if (ImageExist(sTreeTexturesID[iL]) == 1) DeleteImage(sTreeTexturesID[iL]);
					LoadImage(sTreeTextures[iL].Get(), sTreeTexturesID[iL], 0, g.gdividetexturesize);
					if (!ImageExist(sTreeTexturesID[iL]) == 1)
					{
						//Load failed, clear texture slot.
						sTreeTextures[iL] = "";
					}
					SetMipmapNum(-1);
					image_setlegacyimageloading(false);
				}

				if (ImageExist(sTreeTexturesID[iL]))
				{

					iUsedImages++;

					float path_gadget_size = ImGui::GetFontSize()*3.0;
					int preview_icon_size = ImGui::GetFontSize();

					bool bDeleteImage = true;
					if (iL == 0)
						bDeleteImage = false;

					//TEXTURE.

					cStr sLabel = cStr("Texture##InputTreeTexture") + cStr(iL);
					if (!bDeleteImage)
						sLabel = cStr("Default##InputTreeTexture") + cStr(iL);

					//PE: New do 4x4 coloums here.
					int iLargerPreviewIconSize = 28;//PE: 54 , now lowest possible icon
					float control_width = (iLargerPreviewIconSize + 3.0) * 4.0f + 6.0;

					if (w > control_width) {
						//PE: fit perfectly with window width.
						iLargerPreviewIconSize = (w - 20.0) / 4.0;
						iLargerPreviewIconSize -= 10.0; //Padding.
						if (iLargerPreviewIconSize > 70) iLargerPreviewIconSize = 70;
					}

					ImVec2 vSelectionDraw = ImGui::GetCurrentWindow()->DC.CursorPos;
					if (sTreeTexturesID[iL] > 0)
					{

						cStr sLabelChild = cStr("##Tree4x4") + cStr(iL);

						ImVec2 content_avail = { iLargerPreviewIconSize + 1.0f ,iLargerPreviewIconSize + 1.0f };

						//style.WindowPadding
						ImVec2 oldstyle = ImGui::GetStyle().FramePadding;
						ImGui::GetStyle().FramePadding = { 1,1 };
						ImGui::BeginChild(sLabelChild.Get(), content_avail, false, ImGuiWindowFlags_NoScrollbar);

						iLargerPreviewIconSize &= 0xfffe;

						ImGui::SetBlurMode(true);
						//9999 halv uv x
						ImVec2 fill_rect = ImGui::GetWindowPos() + ImGui::GetCursorPos();
						ImGui::GetCurrentWindow()->DrawList->AddRectFilled(fill_rect, fill_rect + ImVec2(iLargerPreviewIconSize, iLargerPreviewIconSize), ImGui::GetColorU32(ImVec4(0, 0, 0, 1)), 0.0f, ImDrawCornerFlags_None);
						ImGui::SetCursorPosX(((float)iLargerPreviewIconSize - ((float)iLargerPreviewIconSize*imageScaleX)) * 0.5);

						if (ImGui::ImgBtn(sTreeTexturesID[iL], ImVec2(iLargerPreviewIconSize*imageScaleX, iLargerPreviewIconSize), ImColor(0, 0, 0, 255), ImColor(255, 255, 255, 255), ImColor(220, 220, 220, 220), ImColor(220, 220, 220, 220),-1,0,0,0))
						{
							//PE: Toggle
							if (ggtrees_global_params.paint_tree_bitfield & (1ULL << iL))
								ggtrees_global_params.paint_tree_bitfield &= ~(1ULL << iL);
							else
								ggtrees_global_params.paint_tree_bitfield |= (1ULL << iL);
						}
						ImGui::SetBlurMode(false);

						if (ImGui::IsItemHovered())
						{
							int tooltip_height = 350;
							int tooltip_width = 350 * imageScaleX;
							ImVec2 tooltip_position = ImVec2(ImGui::GetWindowPos());
							tooltip_position.x = vWindowPos.x - (tooltip_width + 10.0);
							if ((tooltip_position.y + tooltip_height) > (vWindowPos.y + vWindowSize.y))
								tooltip_position.y = (vWindowPos.y + vWindowSize.y) - tooltip_height - 10.0;
							ImGui::SetNextWindowPos(tooltip_position);

							ImGui::BeginTooltip();
							ImGui::ImgBtn(sTreeTexturesID[iL], ImVec2(350*imageScaleX, 350), ImColor(0, 0, 0, 255), ImColor(255, 255, 255, 255), ImColor(220, 220, 220, 220), ImColor(220, 220, 220, 220), -1, 0, 0, 0);
							ImGui::TextCenter(sTreeTexturesName[iL].Get());
							ImGui::Separator();
							ImGui::EndTooltip();
						}

						ImGui::EndChild();
						ImGui::GetStyle().FramePadding = oldstyle;
					}

					if (ggtrees_global_params.paint_tree_bitfield & (1ULL << iL)) //sTerrainTexturesID[iL] - t.terrain.imagestartindex - 80)
					{
						ImVec2 padding = { 2.0, 2.0 };
						image_selected_bb = ImRect((vSelectionDraw - padding), vSelectionDraw + padding + ImVec2(iLargerPreviewIconSize, iLargerPreviewIconSize));
						ImGui::GetCurrentWindow()->DrawList->AddRect(image_selected_bb.Min, image_selected_bb.Max, ImGui::GetColorU32(tool_selected_col), 0.0f, 15, 2.0f);
						bGotSelection = true;
						values |= (1ULL << iL);
					}
					else
					{
						values &= ~(1ULL << iL);
					}

					if (iUsedImages == 1 || iUsedImages == 5 || iUsedImages == 9 || iUsedImages == 13 || iUsedImages == 17)
					{
						fLastMaxY = ImGui::GetCursorPosY();
					}

					ImGui::NextColumn();

				}
			}
			else {
				//Delete old image.
				if (ImageExist(sTerrainTexturesID[iL]) == 1) DeleteImage(sTerrainTexturesID[iL]);
			}

		}
		if (values <= 0) values = 1; //Always have one tree selected.
		ggtrees_global_params.paint_tree_bitfield = values;

		ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(0.0, 3.0));

		if (ImGui::GetCurrentWindow()->ScrollbarSizes.x > 0) {
			//Hitting exactly at the botton could cause flicker, so add some additional lines when scrollbar on.
			ImGui::Text("");
			ImGui::Text("");
			fLastMaxY += 20.0;
		}

		ImGui::EndChild();

		ImGui::GetStyle().WindowPadding = oldwinstylemain;
		ImGui::GetStyle().FramePadding = oldstylemain;

		ImGui::Columns(1);


		float but_gadget_size = ImGui::GetFontSize()*10.0;
		ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((w*0.5) - (but_gadget_size*0.5), 0.0f));

		if (ImGui::StyleButton("Randomize All Trees##TerrainTrees", ImVec2(but_gadget_size, 0)))
		{
			int iAction = askBoxCancel("This will randomize all your trees, are you sure?", "Confirmation"); //1==Yes 2=Cancel 0=No
			if (iAction == 1)
			{
				GGTrees::GGTrees_RepopulateInstances(); //PE: Needed to get new random positions.
			}
		}
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("All trees in the level will have their position and rotation randomized");

		ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((w*0.5) - (but_gadget_size*0.5), 0.0f));
		if (ImGui::StyleButton("Clear All Trees##TerrainTrees", ImVec2(but_gadget_size, 0)))
		{
			int iAction = askBoxCancel("This will delete all your trees, are you sure?", "Confirmation"); //1==Yes 2=Cancel 0=No
			if (iAction == 1)
			{
				GGTrees::GGTrees_HideAll();
			}
		}

		ImRect bbwin(ImGui::GetWindowPos(), ImGui::GetWindowPos() + ImGui::GetWindowSize());
		if (ImGui::IsMouseHoveringRect(bbwin.Min, bbwin.Max))
		{
			bImGuiGotFocus = true;
		}

		ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));
	}
}


