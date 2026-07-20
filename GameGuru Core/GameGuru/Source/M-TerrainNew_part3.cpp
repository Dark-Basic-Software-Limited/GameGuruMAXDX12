#ifdef BUSHUI

int iDeleteSingleBushTextures = 0;
int iDeleteAllBushTextures = 0;
void imgui_Customize_Bush_v3(int mode)
{

	static cstr sBushTextures[32];
	static cstr sBushTexturesName[32];
	static int sBushTexturesID[32];
	static bool bBushSelected[32];
	static bool bBushInit = true;


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


		if (iDeleteSingleBushTextures > 0)
		{
			if (ImageExist(iDeleteSingleBushTextures))
				DeleteImage(iDeleteSingleBushTextures);
			iDeleteSingleBushTextures = 0;
		}
		if (iDeleteAllBushTextures)
		{
			iDeleteAllBushTextures = false;
			for (int iL = 0; iL < 32; iL++)
			{
				sBushTexturesID[iL] = t.terrain.imagestartindex + 280 + iL;
				if (ImageExist(sBushTexturesID[iL]))
					DeleteImage(sBushTexturesID[iL]);
			}
		}

		if (bBushInit)
		{
			bBushInit = false;

			for (int iL = 0; iL < 32; iL++) {
				sBushTexturesID[iL] = t.terrain.imagestartindex + 280 + iL;
				sBushTexturesName[iL] = "";
				sBushTextures[0] = "";
				bBushSelected[iL] = false;
			}
			bBushSelected[0] = true;

			sBushTextures[0] = "treebank/billboards/pine.dds";
			sBushTextures[1] = "treebank/billboards/birch.dds";
			sBushTextures[2] = "treebank/billboards/cactus var1.dds";
			sBushTextures[3] = "treebank/billboards/italian pine.dds";
			sBushTextures[4] = "treebank/billboards/kentia palm.dds";
			sBushTextures[5] = "treebank/billboards/palm.dds";
			sBushTextures[6] = "treebank/billboards/vine tree large.dds";
			sBushTextures[7] = "treebank/billboards/vine tree small.dds";
			sBushTextures[8] = "treebank/billboards/white pine.dds";
			sBushTextures[9] = "treebank/billboards/snow pine.dds";
			sBushTextures[10] = "treebank/billboards/snow fir1.dds";
			sBushTextures[11] = "treebank/billboards/snow fir2.dds";
			sBushTextures[12] = "treebank/billboards/snow fir3.dds";

			sBushTexturesName[0] = "pine";
			sBushTexturesName[1] = "birch";
			sBushTexturesName[2] = "cactus var1";
			sBushTexturesName[3] = "italian pine";
			sBushTexturesName[4] = "kentia palm";
			sBushTexturesName[5] = "palm";
			sBushTexturesName[6] = "vine tree large";
			sBushTexturesName[7] = "vine tree small";
			sBushTexturesName[8] = "white pine";
			sBushTexturesName[9] = "Snow pine";
			sBushTexturesName[10] = "Snow fir 1";
			sBushTexturesName[11] = "Snow fir 2";
			sBushTexturesName[12] = "Snow fir 3";

		}

		int iUsedImages = 0;
		float col_start = 100.0f;
		int iSelectTexture = -1;

		ImGui::Indent(-10);

		static float fLast4x4Height = 228.0;
		static float fLastMaxY = 0.0;
		if (fLastMaxY > 100.0 && fLastMaxY < 400.0)
		{
			fLast4x4Height = fLastMaxY + 1.0;
		}

		ImVec2 oldstylemain = ImGui::GetStyle().FramePadding;
		ImVec2 oldwinstylemain = ImGui::GetStyle().WindowPadding;
		ImGui::GetStyle().WindowPadding = { 0,0 };
		ImGui::GetStyle().FramePadding = { 0,0 };

		ImGui::BeginChild("##Bush4x4scrollbar", ImVec2(0, fLast4x4Height), false, 0); //ImGuiWindowFlags_AlwaysVerticalScrollbar

		w = ImGui::GetWindowContentRegionWidth(); //PE: Minus scrollbar.
		ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(0.0, 2.0));

		ImGui::Columns(4, "##Bush4x4columns", false);  //false no border
		bool bGotSelection = false;
		ImRect image_selected_bb;
		uint32_t values = ggtrees_global_params.paint_tree_bitfield;
		for (int iL = 0; iL < 32; iL++)
		{

			if (sBushTextures[iL] != "")
			{
				if (!ImageExist(sBushTexturesID[iL]))
				{
					//Load in image.
					image_setlegacyimageloading(true);
					SetMipmapNum(1); //PE: mipmaps not needed.
					if (ImageExist(sBushTexturesID[iL]) == 1) DeleteImage(sBushTexturesID[iL]);
					LoadImage(sBushTextures[iL].Get(), sBushTexturesID[iL], 0, g.gdividetexturesize);
					//LoadImageSize(pCompressedVersionIfAny, sBushTexturesID[iL], 512,512); //Takes to long.
					if (!ImageExist(sBushTexturesID[iL]) == 1)
					{
						//Load failed, clear texture slot.
						sBushTextures[iL] = "";
					}
					SetMipmapNum(-1);
					image_setlegacyimageloading(false);
				}

				if (ImageExist(sBushTexturesID[iL]))
				{

					iUsedImages++;

					float path_gadget_size = ImGui::GetFontSize()*3.0;
					int preview_icon_size = ImGui::GetFontSize();

					bool bDeleteImage = true;
					if (iL == 0)
						bDeleteImage = false;

					//TEXTURE.

					cStr sLabel = cStr("Texture##InputBushTexture") + cStr(iL);
					if (!bDeleteImage)
						sLabel = cStr("Default##InputBushTexture") + cStr(iL);

					//PE: New do 4x4 coloums here.
					int iLargerPreviewIconSize = 28;//PE: 54 , now lowest possible icon
					float control_width = (iLargerPreviewIconSize + 3.0) * 4.0f + 6.0;

					if (w > control_width) {
						//PE: fit perfectly with window width.
						iLargerPreviewIconSize = (w - 20.0) / 4.0;
						//iLargerPreviewIconSize -= 6.0; //Padding.
						iLargerPreviewIconSize -= 10.0; //Padding.
						if (iLargerPreviewIconSize > 70) iLargerPreviewIconSize = 70;
					}

					ImVec2 vSelectionDraw = ImGui::GetCurrentWindow()->DC.CursorPos;
					if (sBushTexturesID[iL] > 0)
					{

						cStr sLabelChild = cStr("##Bush4x4") + cStr(iL);

						ImVec2 content_avail = { iLargerPreviewIconSize + 1.0f ,iLargerPreviewIconSize + 1.0f };

						//style.WindowPadding
						ImVec2 oldstyle = ImGui::GetStyle().FramePadding;
						ImGui::GetStyle().FramePadding = { 1,1 };
						ImGui::BeginChild(sLabelChild.Get(), content_avail, false, ImGuiWindowFlags_NoScrollbar);

						iLargerPreviewIconSize &= 0xfffe;

						ImGui::SetBlurMode(true);
						//9999 halv uv x
						if (ImGui::ImgBtn(sBushTexturesID[iL], ImVec2(iLargerPreviewIconSize, iLargerPreviewIconSize), ImColor(0, 0, 0, 255), ImColor(255, 255, 255, 255), ImColor(220, 220, 220, 220), ImColor(220, 220, 220, 220), -1, 0, 0, 9999))
						{
							//ggtrees_global_params.paint_tree_type = iL;
							//ggtrees_global_params.paint_tree_bitfield = 1 << iL; // multiple bits can be set
							//PE: Toggle
							if (ggtrees_global_params.paint_tree_bitfield & (1 << iL))
								ggtrees_global_params.paint_tree_bitfield &= ~(1 << iL);
							else
								ggtrees_global_params.paint_tree_bitfield |= (1 << iL);
						}
						ImGui::SetBlurMode(false);

						if (ImGui::IsItemHovered())
						{
							ImGui::BeginTooltip();
							ImGui::ImgBtn(sBushTexturesID[iL], ImVec2(350, 350), ImColor(0, 0, 0, 255), ImColor(255, 255, 255, 255), ImColor(220, 220, 220, 220), ImColor(220, 220, 220, 220), -1, 0, 0, 9999);
							ImGui::TextCenter(sBushTexturesName[iL].Get());
							ImGui::Separator();
							ImGui::EndTooltip();
						}

						ImGui::EndChild();
						ImGui::GetStyle().FramePadding = oldstyle;
					}

					if (ggtrees_global_params.paint_tree_bitfield & (1 << iL)) //sTerrainTexturesID[iL] - t.terrain.imagestartindex - 80)
					{
						ImVec2 padding = { 2.0, 2.0 };
						image_selected_bb = ImRect((vSelectionDraw - padding), vSelectionDraw + padding + ImVec2(iLargerPreviewIconSize, iLargerPreviewIconSize));
						ImGui::GetCurrentWindow()->DrawList->AddRect(image_selected_bb.Min, image_selected_bb.Max, ImGui::GetColorU32(tool_selected_col), 0.0f, 15, 2.0f);
						bGotSelection = true;
						values |= (1 << iL);
					}
					else
					{
						values &= ~(1 << iL);
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
		//		if (bGotSelection)
		//		{
		//			//tool_selected_col
		//			ImVec4 test = ImVec4(1.0,0.0,0.0,1.0);
		//			//ImGui::GetCurrentWindow()->DrawList->AddRect(image_selected_bb.Min, image_selected_bb.Max, ImGui::GetColorU32(tool_selected_col), 0.0f, 15, 2.0f);
		//			ImGui::PushClipRect(image_selected_bb.Min, image_selected_bb.Max, false);
		//			ImGui::GetCurrentWindow()->DrawList->AddRect(image_selected_bb.Min, image_selected_bb.Max, ImGui::GetColorU32(test), 0.0f, 15, 4.0f);
		//			ImGui::PopClipRect();
		//		}

		ImGui::GetStyle().WindowPadding = oldwinstylemain;
		ImGui::GetStyle().FramePadding = oldstylemain;

		ImGui::Columns(1);


		float but_gadget_size = ImGui::GetFontSize()*10.0;
		ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((w*0.5) - (but_gadget_size*0.5), 0.0f));

		if (ImGui::StyleButton("Randomize All Bushes##TerrainBushes", ImVec2(but_gadget_size, 0)))
		{
			//ggtrees_global_params.paint_density //PE: Perhaps make this random ?
			GGTrees::GGTrees_RepopulateInstances(); //PE: Needed to get new random positions.
		}
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("All bushes in the level will have their position and rotation randomized");

		ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((w*0.5) - (but_gadget_size*0.5), 0.0f));
		if (ImGui::StyleButton("Clear All Bushes##TerrainBushes", ImVec2(but_gadget_size, 0)))
		{
			int iAction = askBoxCancel("This will delete all your bushes, are you sure?", "Confirmation"); //1==Yes 2=Cancel 0=No
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

#endif

std::vector<cstr> grass_selections;
std::vector<cstr> grass_selections_text;
void init_grass_selections()
{
	grass_selections.clear();
	grass_selections.push_back("grassbank\\grass short dead_color.dds");
	grass_selections.push_back("grassbank\\grass short_color.dds");
	grass_selections.push_back("grassbank\\grass tall dead_color.dds");
	grass_selections.push_back("grassbank\\grass tall_color.dds");
	grass_selections.push_back("grassbank\\short grass 2 dead_color.dds");
	grass_selections.push_back("grassbank\\short grass 2_color.dds");
	grass_selections.push_back("grassbank\\short grass 3 dead_color.dds");
	grass_selections.push_back("grassbank\\short grass 3_color.dds");
	grass_selections.push_back("grassbank\\short grass 4 dead_color.dds");
	grass_selections.push_back("grassbank\\short grass 4_color.dds");
	grass_selections.push_back("grassbank\\short grass 5 dead_color.dds");
	grass_selections.push_back("grassbank\\short grass 5_color.dds");
	grass_selections.push_back("grassbank\\tall grass 2 dead_color.dds");
	grass_selections.push_back("grassbank\\tall grass 2_color.dds");
	grass_selections.push_back("grassbank\\tall grass 3 dead_color.dds");
	grass_selections.push_back("grassbank\\tall grass 3_color.dds");
	for (int i = 0; i < grass_selections.size(); i++)
	{
		char pFileOnly[MAX_PATH];
		strcpy(pFileOnly, grass_selections[i].Get());
		for (int n = strlen(pFileOnly) - 1; n > 0; n--)
		{
			if (pFileOnly[n] == '\\' || pFileOnly[n] == '/')
			{
				strcpy(pFileOnly, pFileOnly + n + 1);
				break;
			}
		}
		char *remove_ext = (char *) pestrcasestr( pFileOnly , "_color.dds");
		if (remove_ext)
			*remove_ext = 0;
		grass_selections_text.push_back(pFileOnly);
	}
}


void imgui_Customize_Vegetation(int mode)
{
	int wflags = ImGuiTreeNodeFlags_DefaultOpen;
	if (mode == 1) wflags = ImGuiTreeNodeFlags_None;
	if (pref.bAutoClosePropertySections && mode == 1 && iLastOpenHeader != 3)
		ImGui::SetNextItemOpen(false, ImGuiCond_Always);

	float w = ImGui::GetWindowContentRegionWidth();

	//Customize Vegetation
	if (ImGui::StyleCollapsingHeader("Palette", wflags)) {

		if (mode == 1) iLastOpenHeader = 3;

		ImGui::Indent(10);
		ImGui::PushItemWidth(-10);

		if (iDeleteSingleGrassTextures > 0) 
		{
			if (ImageExist(iDeleteSingleGrassTextures))
				DeleteImage(iDeleteSingleGrassTextures);
			iDeleteSingleGrassTextures = 0;
		}
		if (iDeleteAllGrassTextures) 
		{
			iDeleteAllGrassTextures = false;
			for (int iL = 0; iL < 16; iL++) {
				if (ImageExist(t.terrain.imagestartindex + 180 + iL))
					DeleteImage(t.terrain.imagestartindex + 180 + iL);
			}
			bUpdateGrassMaterials = true;
		}
		if (t.visuals.sGrassTextures[0] == "")
		{
			//PE: @Lee-Grass Define default palette here. Also need to update t.gamevisuals :)

			init_grass_selections(); //PE: Set defaults for selection window.

			t.visuals.sGrassTextures[0] = "grassbank\\grass short dead_color.dds";
			t.visuals.sGrassTexturesName[0] = "Grass short dead";
			t.gamevisuals.sGrassTextures[0] = t.visuals.sGrassTextures[0];
			t.gamevisuals.sGrassTexturesName[0] = t.visuals.sGrassTexturesName[0];
			t.visuals.sGrassTextures[1] = "grassbank\\grass short_color.dds";
			t.visuals.sGrassTexturesName[1] = "Grass short";
			t.gamevisuals.sGrassTextures[1] = t.visuals.sGrassTextures[1];
			t.gamevisuals.sGrassTexturesName[1] = t.visuals.sGrassTexturesName[1];
			t.visuals.sGrassTextures[2] = "grassbank\\grass tall dead_color.dds";
			t.visuals.sGrassTexturesName[2] = "Grass tall dead";
			t.gamevisuals.sGrassTextures[2] = t.visuals.sGrassTextures[2];
			t.gamevisuals.sGrassTexturesName[2] = t.visuals.sGrassTexturesName[2];
			t.visuals.sGrassTextures[3] = "grassbank\\grass tall_color.dds";
			t.visuals.sGrassTexturesName[3] = "Grass tall";
			t.gamevisuals.sGrassTextures[3] = t.visuals.sGrassTextures[3];
			t.gamevisuals.sGrassTexturesName[3] = t.visuals.sGrassTexturesName[3];
			t.visuals.sGrassTextures[4] = "grassbank\\short grass 2 dead_color.dds";
			t.visuals.sGrassTexturesName[4] = "Grass 2 dead";
			t.gamevisuals.sGrassTextures[4] = t.visuals.sGrassTextures[4];
			t.gamevisuals.sGrassTexturesName[4] = t.visuals.sGrassTexturesName[4];
			t.visuals.sGrassTextures[5] = "grassbank\\short grass 2_color.dds";
			t.visuals.sGrassTexturesName[5] = "Short grass 2";
			t.gamevisuals.sGrassTextures[5] = t.visuals.sGrassTextures[5];
			t.gamevisuals.sGrassTexturesName[5] = t.visuals.sGrassTexturesName[5];
			t.visuals.sGrassTextures[6] = "grassbank\\short grass 3 dead_color.dds";
			t.visuals.sGrassTexturesName[6] = "Grass 3 dead";
			t.gamevisuals.sGrassTextures[6] = t.visuals.sGrassTextures[6];
			t.gamevisuals.sGrassTexturesName[6] = t.visuals.sGrassTexturesName[6];
			t.visuals.sGrassTextures[7] = "grassbank\\short grass 3_color.dds";
			t.visuals.sGrassTexturesName[7] = "Short grass 3";
			t.gamevisuals.sGrassTextures[7] = t.visuals.sGrassTextures[7];
			t.gamevisuals.sGrassTexturesName[7] = t.visuals.sGrassTexturesName[7];

			// blank rest to allow more room for UI stuff, and user can always add more!
			for (int iBlankRest = 8; iBlankRest < 16; iBlankRest++)
			{
				t.visuals.sGrassTextures[iBlankRest] = "";
				t.visuals.sGrassTexturesName[iBlankRest] = "";
				t.gamevisuals.sGrassTextures[iBlankRest] = t.visuals.sGrassTextures[iBlankRest];
				t.gamevisuals.sGrassTexturesName[iBlankRest] = t.visuals.sGrassTextures[iBlankRest];
			}

			// save out reset grass plate choices
			visuals_save ( );
		}

		// find last slot used
		int iLastSlotUsed = 0;
		for (int iL = 0; iL < 16; iL++)
			if (t.visuals.sGrassTextures[iL] != "")
				if (ImageExist(t.terrain.imagestartindex + 180 + iL))
					iLastSlotUsed = iL;

		int iUsedImages = 0;
		float col_start = 90.0f;
		int iSelectGrassTexture = -1;
		cStr sGrassTextureFolder = g.rootdir_s + "grassbank\\";


#ifdef DISPLAY4x4
		ImGui::Indent(-10);
		ImGui::Indent(4);
		ImGui::Columns(4, "##veg4x4columns", false);  //false no border
#endif

		for (int iL = 0; iL < 16; iL++) 
		{
			if (t.visuals.sGrassTextures[iL] != "")
			{
				if (!ImageExist(t.terrain.imagestartindex + 180 + iL))
				{
					//Load in image.
					image_setlegacyimageloading(true);
					SetMipmapNum(1); //PE: mipmaps not needed.
					if (ImageExist(t.terrain.imagestartindex + 180 + iL) == 1) DeleteImage(t.terrain.imagestartindex + 180 + iL);
					LoadImage(t.visuals.sGrassTextures[iL].Get(), t.terrain.imagestartindex + 180 + iL, 0, g.gdividetexturesize);
					if (ImageExist(t.terrain.imagestartindex + 180 + iL) == 1)
					{
						sGrassTexturesID[iL] = t.terrain.imagestartindex + 180 + iL;
					}
					else
					{
						//Load failed, clear texture slot.
						t.visuals.sGrassTextures[iL] = "";
						t.gamevisuals.sGrassTextures[iL] = t.visuals.sGrassTextures[iL];
						g.projectmodified = 1;
						bUpdateGrassMaterials = true;
					}
					SetMipmapNum(-1);
					image_setlegacyimageloading(false);
				}
				else
				{
					sGrassTexturesID[iL] = t.terrain.imagestartindex + 180 + iL;
				}
				if (ImageExist(sGrassTexturesID[iL]))
				{
					iUsedImages++;



					if (bGrassNameWindow[iL]) {
						//Ask for a proper name of veg.
						ImGui::SetNextWindowSize(ImVec2(26 * ImGui::GetFontSize(), 32 * ImGui::GetFontSize()), ImGuiCond_Once);
						ImGui::SetNextWindowPosCenter(ImGuiCond_Once);
						cstr sUniqueWinName = cstr("Vegetation Name##ttn") + cstr(iL);
						ImGui::Begin(sUniqueWinName.Get(), &bGrassNameWindow[iL], 0);
						ImGui::Indent(10);
						static char NewTextureName[256];
						cstr sUniqueInputName = cstr("##InputVegetationName") + cstr(iL);
						float content_width = ImGui::GetContentRegionAvailWidth() - 10.0;
						ImGui::ImgBtn(sGrassTexturesID[iL], ImVec2(content_width, content_width), ImColor(0, 0, 0, 255));
						ImGui::PushItemWidth(-10);
						ImGui::Text("Enter a name for your vegetation:");
						
						if (ImGui::IsRootWindowOrAnyChildFocused() && !ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0))
							ImGui::SetKeyboardFocusHere(0);

						if (ImGui::InputText(sUniqueInputName.Get(), t.visuals.sGrassTexturesName[iL].Get(), 250, ImGuiInputTextFlags_EnterReturnsTrue)) {
							t.gamevisuals.sGrassTexturesName[iL] = t.visuals.sGrassTexturesName[iL];
							bGrassNameWindow[iL] = false;
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


#ifndef DISPLAY4x4
					if (iUsedImages < 16 && iL == 0)
					{
						float but_gadget_size = ImGui::GetFontSize()*10.0;
						ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((w*0.5) - (but_gadget_size*0.5), 0.0f));

						if (ImGui::StyleButton("Add Texture##Grass", ImVec2(but_gadget_size, 0)))
						{
							iSelectGrassTexture = 99;
						}
						ImGui::Separator();
					}
#endif
					float path_gadget_size = ImGui::GetFontSize()*3.0;
					int preview_icon_size = ImGui::GetFontSize();

					//PE: You can only change image slot 0 , not delete it, we need at least one texture.
					bool bDeleteImage = false;
					if (iL != 0 && iL == iLastSlotUsed) bDeleteImage = true;

					// chop grassbank from material name 
					cstr materialname = t.visuals.sGrassTexturesName[iL];

					cStr sLabel = cStr("##InputGrassTexture") + cStr(iL);

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

					if (sGrassTexturesID[iL] > 0)
					{
						cStr sLabelChild = cStr("##grass4x4") + cStr(iL);

						ImVec2 content_avail = { iLargerPreviewIconSize + 1.0f ,iLargerPreviewIconSize + 1.0f };

						//style.WindowPadding
						ImVec2 oldstyle = ImGui::GetStyle().FramePadding;
						ImGui::GetStyle().FramePadding = { 0,0 };
						ImGui::BeginChild(sLabelChild.Get(), content_avail,false, ImGuiWindowFlags_NoScrollbar);

						if (ImGui::ImgBtn(sGrassTexturesID[iL], ImVec2(iLargerPreviewIconSize, iLargerPreviewIconSize), ImColor(0, 0, 0, 255)))
						{
							//iSelectGrassTexture = iL;
							//Now do toggle selection.
							if (bCurrentGrassTextureForPaint[iL])
								bCurrentGrassTextureForPaint[iL] = false;
							else
								bCurrentGrassTextureForPaint[iL] = true;

						}

						bool bInContext = false;
						static int iCurrentContext = -1;
						if ( iCurrentContext == -1 || iCurrentContext == iL )
						{
							if (ImGui::BeginPopupContextWindow())
							{
								iCurrentContext = iL;
								bInContext = true;
								if (ImGui::MenuItem("Change Vegetation"))
								{
									iSelectGrassTexture = iL;
									iCurrentContext = -1;
								}
								if (ImGui::MenuItem("Change Vegetation Name"))
								{
									bGrassNameWindow[iL] = true;
									iCurrentContext = -1;
								}
								if (bCurrentGrassTextureForPaint[iL])
								{
									if (ImGui::MenuItem("Unselect Vegetation"))
									{
										bCurrentGrassTextureForPaint[iL] = false;
									}
								}
								else {
									if (ImGui::MenuItem("Select Vegetation"))
									{
										bCurrentGrassTextureForPaint[iL] = true;
									}
								}
								if (iL != 0)
								{
									if (ImGui::MenuItem("Delete Vegetation"))
									{
										t.visuals.sGrassTextures[iL] = ""; //delete image in next run.
										t.gamevisuals.sGrassTextures[iL] = t.visuals.sGrassTextures[iL];
										g.projectmodified = 1;
										bUpdateGrassMaterials = true;
										iCurrentContext = -1;
									}
								}
								ImGui::EndPopup();
							}
							else {
								iCurrentContext = -1;
							}
						}
						if (!bInContext && ImGui::IsItemHovered())
						{
							ImGui::BeginTooltip();
							ImGui::ImgBtn(sGrassTexturesID[iL], ImVec2(350, 350), ImColor(0, 0, 0, 255), ImColor(220, 220, 220, 220), ImColor(255, 255, 255, 255), ImColor(180, 180, 160, 255), -1, 0, 0, 0, false,false,true);
							ImGui::TextCenter(materialname.Get());
							ImGui::Separator();
							ImGui::EndTooltip();
						}

						ImGui::EndChild();
						ImGui::GetStyle().FramePadding = oldstyle;

					}
					sLabel = cStr("##veg") +  cStr(iL);

					float checkwidth = ImGui::GetFontSize()*1.5;
					ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((ImGui::GetContentRegionAvail().x*0.5) - (checkwidth*0.5), 0.0f));

					if (bCurrentGrassTextureForPaint[iL]) //sTerrainTexturesID[iL] - t.terrain.imagestartindex - 80)
					{
						ImVec2 padding = { 2.0, 2.0 };
						const ImRect image_bb((vSelectionDraw - padding), vSelectionDraw + padding + ImVec2(iLargerPreviewIconSize, iLargerPreviewIconSize));
						ImGui::GetCurrentWindow()->DrawList->AddRect(image_bb.Min, image_bb.Max, ImGui::GetColorU32(tool_selected_col), 0.0f, 15, 2.0f);
					}


					ImGui::NextColumn();

#else
					ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));

					//PE: @Lee-Grass bCurrentGrassTextureForPaint[iL] will be true if you need to use it when painting.
					ImGui::Checkbox(sLabel.Get(), &bCurrentGrassTextureForPaint[iL]);

					ImGui::SameLine();
					//ImGui::SetCursorPos(ImVec2(col_start, ImGui::GetCursorPosY() - 3));
					//ImGui::SetCursorPos(ImVec2(col_start-40, ImGui::GetCursorPosY()));
					int iSmallPreviewYMargin = preview_icon_size / 2;
					int iLargerPreviewIconSize = preview_icon_size * 2;
					ImGui::SetCursorPos(ImVec2(col_start-40, ImGui::GetCursorPosY()-iSmallPreviewYMargin));

					if (sGrassTexturesID[iL] > 0)
					{
						if (ImGui::ImgBtn(sGrassTexturesID[iL], ImVec2(iLargerPreviewIconSize, iLargerPreviewIconSize), ImColor(0, 0, 0, 255)))
						{
							iSelectGrassTexture = iL;
						}
						if (ImGui::IsItemHovered())
						{
							ImGui::BeginTooltip();
							ImGui::ImgBtn(sGrassTexturesID[iL], ImVec2(180, 180), ImColor(0, 0, 0, 255));
							ImGui::EndTooltip();
						}
						ImGui::SameLine();
					}

					int iInputFlags = ImGuiInputTextFlags_EnterReturnsTrue;
					if (!bDeleteImage)
						iInputFlags = ImGuiInputTextFlags_ReadOnly;

					ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY()+iSmallPreviewYMargin));
					sLabel = cStr(" ##InputGrassTexture") + cStr(iL);
					ImGui::PushItemWidth(-10 - path_gadget_size);
					if (ImGui::InputText(sLabel.Get(), &materialname[0], 250, iInputFlags))
					{
						if (strlen(materialname) == 0) t.visuals.sGrassTextures[iL] = ""; //delete image in next run.
						t.gamevisuals.sGrassTextures[iL] = t.visuals.sGrassTextures[iL];
						g.projectmodified = 1;
						bUpdateGrassMaterials = true;
					}
					ImGui::PopItemWidth();

					ImGui::SameLine();
					ImGui::PushItemWidth(path_gadget_size);
					sLabel = cStr("...##InputGrassTexture") + cStr(iL);

					if (ImGui::StyleButton(sLabel.Get()))
						iSelectGrassTexture = iL;

					ImGui::PopItemWidth();

					if (bDeleteImage)
					{
						ImGui::SameLine();
						sLabel = cStr("X##InputGrassTexture") + cStr(iL);
						if (ImGui::StyleButton(sLabel.Get()))
						{
							t.visuals.sGrassTextures[iL] = ""; //delete image in next run.
							t.gamevisuals.sGrassTextures[iL] = t.visuals.sGrassTextures[iL];
							bCurrentGrassTextureForPaint[iL] = false;
							g.projectmodified = 1;
							bUpdateGrassMaterials = true;
							visuals_save();
						}
					}

					// squish together as it looks better - can see more grass choices
					ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY()-iSmallPreviewYMargin*2));
#endif
				}
			}
			else 
			{
				//Delete old image.
				if (ImageExist(t.terrain.imagestartindex + 180 + iL) == 1) DeleteImage(t.terrain.imagestartindex + 180 + iL);
				sGrassTexturesID[iL] = 0;
				bCurrentGrassTextureForPaint[iL] = false;
			}
		}

#ifdef DISPLAY4x4
		ImGui::Indent(-4);
		ImGui::Columns(1);
		ImGui::Indent(10);

		if (iUsedImages < 16 )
		{
			//ImGui::Separator();
			float but_gadget_size = ImGui::GetFontSize()*10.0;
			ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((w*0.5) - (but_gadget_size*0.5), 0.0f));

			if (ImGui::StyleButton("Add New Vegetation##Grass", ImVec2(but_gadget_size, 0)))
			{
				iSelectGrassTexture = 99;
			}
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Add New Vegetation");

			//ImGui::Separator();
		}
#endif

		static bool grass_selection_window = false;
		static int iSelectedGrassTexture = -1;
		if (iSelectGrassTexture >= 0)
		{
			iSelectedGrassTexture = iSelectGrassTexture;
			grass_selection_window = true;
			iSelectGrassTexture = 0;
		}
		iSelectGrassTexture = imgui_get_selections(grass_selections, grass_selections_text, 1, 280, &grass_selection_window);

		if (iSelectGrassTexture >= 0 && iSelectedGrassTexture >= 0)
		{
			bool bNeedFileSelector = false;
			if (iSelectGrassTexture == 999)
				bNeedFileSelector = true;

			grass_selection_window = false;

			int iNewGrassIndex = iSelectGrassTexture;
			iSelectGrassTexture = iSelectedGrassTexture;
			if (iSelectGrassTexture == 99)
			{
				//Find free slot.
				for (int iL2 = 0; iL2 < 32; iL2++) {
					if (t.visuals.sGrassTextures[iL2] == "")
					{
						iSelectGrassTexture = iL2;
						break;
					}
				}
			}

			if (!bNeedFileSelector) {
				t.visuals.sGrassTextures[iSelectGrassTexture] = grass_selections[iNewGrassIndex];
				if (ImageExist(t.terrain.imagestartindex + 180 + iSelectGrassTexture))
					iDeleteSingleGrassTextures = t.terrain.imagestartindex + 180 + iSelectGrassTexture;
				bUpdateGrassMaterials = true;
				t.visuals.sGrassTexturesName[iSelectGrassTexture] = grass_selections_text[iNewGrassIndex];
				bGrassNameWindow[iSelectGrassTexture] = true;
				bCurrentGrassTextureForPaint[iSelectGrassTexture] = true; //PE: Select new added texture.
			}

			if (bNeedFileSelector)
			{
				cStr tOldDir = GetDir();
				char * cFileSelected;
				cFileSelected = (char *)noc_file_dialog_open(NOC_FILE_DIALOG_OPEN, "DDS\0*_color.dds\0\0\0", sGrassTextureFolder.Get(), NULL);
				SetDir(tOldDir.Get());

				if (cFileSelected && strlen(cFileSelected) > 0)
				{
					char *relonly = (char *)pestrcasestr(cFileSelected, g.rootdir_s.Get());
					t.visuals.sGrassTextures[iSelectGrassTexture] = cFileSelected;
					t.visuals.sGrassTexturesName[iSelectGrassTexture] = "";
					if (relonly)
					{
						t.visuals.sGrassTextures[iSelectGrassTexture] = cFileSelected + g.rootdir_s.Len();
					}
					g.projectmodified = 1;
					t.gamevisuals.sGrassTextures[iSelectGrassTexture] = t.visuals.sGrassTextures[iSelectGrassTexture];
					bUpdateGrassMaterials = true;

					bGrassNameWindow[iSelectGrassTexture] = true;

					//PE: Reload image.
					if (ImageExist(t.terrain.imagestartindex + 180 + iSelectGrassTexture))
						iDeleteSingleGrassTextures = t.terrain.imagestartindex + 180 + iSelectGrassTexture;

					bCurrentGrassTextureForPaint[iSelectGrassTexture] = true; //PE: Select new added texture.

				}
			}
			iSelectGrassTexture = -1;
		}

		if (bUpdateGrassMaterials)
		{
			//@Lee-Grass update super palette.
			for (int iL = 0; iL < 16; iL++) 
			{
				if (t.visuals.sGrassTextures[iL] != "") 
				{
					if (sGrassChangedTextures[iL] != t.visuals.sGrassTextures[iL]) 
					{
						//Add t.visuals.sGrassTextures[iL] to palette in slot iL
						sGrassChangedTextures[iL] = t.visuals.sGrassTextures[iL];

						// the chosen grass file
						char pGrassSrcWorkFile[MAX_PATH];
						strcpy(pGrassSrcWorkFile, sGrassChangedTextures[iL].Get());
						pGrassSrcWorkFile[strlen(pGrassSrcWorkFile) - strlen("_color.dds")] = 0;
						char pGrassSrcFile[MAX_PATH];
						strcpy(pGrassSrcFile, pGrassSrcWorkFile + strlen("grassbank\\"));

						char pChosenGrassSrcFile[MAX_PATH];
						strcpy(pChosenGrassSrcFile, pGrassSrcFile);

						// determine grass texture filenames to insert (i.e. take ' dead' from 'grass short dead' and put in pChosenGrassSrcFile as 'grass short')
						char pFullGrassSrcFile[MAX_PATH];
						strcpy(pFullGrassSrcFile, pChosenGrassSrcFile);
						for (int n = strlen(pFullGrassSrcFile) - 1; n > 0; n--)
						{
							if (pFullGrassSrcFile[n] == ' ')
							{
								pChosenGrassSrcFile[n] = 0;
								break;
							}
						}

						// replace grass image in grass plate
						bool bGrassPlateChanged = false;
						for (int iGrassTexSet = 0; iGrassTexSet < 1; iGrassTexSet++)
						{
							// determine destination grass plate
							char pDestTerrainTextureFile[MAX_PATH];
							strcpy(pDestTerrainTextureFile, g.fpscrootdir_s.Get());
							strcat(pDestTerrainTextureFile, "\\Files\\levelbank\\testmap\\grass");
							if (iGrassTexSet == 0) strcat(pDestTerrainTextureFile, "_coloronly.dds");

							// construct grass texture filenames to insert
							char pTexFileToLoad[MAX_PATH];
							strcpy(pTexFileToLoad, g.fpscrootdir_s.Get());
							strcat(pTexFileToLoad, "\\Files\\grassbank\\");
							if (iGrassTexSet == 0)
							{
								// allows an additional word before _color so can use same normal and surface texture maps
								strcat(pTexFileToLoad, pFullGrassSrcFile);
							}
							else
							{
								// possibly truncated for normal and surface loading
								strcat(pTexFileToLoad, pChosenGrassSrcFile);
							}
							if (iGrassTexSet == 0) strcat(pTexFileToLoad, "_color.dds");

							// do the insert
							int iGrassType = iL;
							if (ImageCreateTexturePlate(pDestTerrainTextureFile, iGrassType, pTexFileToLoad, 1, 1) == 1)
							{
								// success
								bGrassPlateChanged = true;
							}
						}
						if (bGrassPlateChanged == true)
						{
							// must remove pre-stored reference to any previous grass_color texture set
							WickedCall_DeleteImage("levelbank\\testmap\\grass_coloronly.dds");
						}
					}
				}
			}
			bUpdateGrassMaterials = false;
		}

		ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));
		// no need to toggle veg, we ALWAYS 'may' need it!
		bEnableVeg = true;

		ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));
		ImGui::Indent(-10);
	}
}


void imgui_Customize_Vegetation_v3(int mode)
{
	int wflags = ImGuiTreeNodeFlags_DefaultOpen;
	if (mode == 1) wflags = ImGuiTreeNodeFlags_None;
	if (pref.bAutoClosePropertySections && mode == 1 && iLastOpenHeader != 3)
		ImGui::SetNextItemOpen(false, ImGuiCond_Always);

	float w = ImGui::GetWindowContentRegionWidth();

	//Customize Vegetation
	if (ImGui::StyleCollapsingHeader("Palette", wflags)) {

		if (mode == 1) iLastOpenHeader = 3;

		ImGui::Indent(10);
		ImGui::PushItemWidth(-10);


		if (iDeleteSingleGrassTextures > 0)
		{
			if (ImageExist(iDeleteSingleGrassTextures))
				DeleteImage(iDeleteSingleGrassTextures);
			iDeleteSingleGrassTextures = 0;
		}
		if (iDeleteAllGrassTextures)
		{
			iDeleteAllGrassTextures = false;
			// Stage B.9: iterate all palette slots (stock + custom) so custom images get cleared too.
			for (int iL = 0; iL < GGGRASS_MAX_PALETTE_SLOTS; iL++) {
				if (ImageExist(t.terrain.imagestartindex + 180 + iL))
					DeleteImage(t.terrain.imagestartindex + 180 + iL);
			}
			bUpdateGrassMaterials = true;
		}
		bool bInitNewGrassSystem = true;
		//PE: @Paul We need this check to be able to update grass available in saved .fpm files (loaded .fpm after init) , as we save all information to be able to support "custom" grass from users.
		//PE: So we need someting unique like grassbank/course grass_mat1_SF_1.15.dds to go into the first slot :)
		if (bInitNewGrassSystem || t.visuals.sGrassTextures[0] == "" || t.visuals.sGrassTextures[0] != "grassbank/course grass_mat1_SF_1.15.dds")
		{
			char grassFilename[ 256 ];
			//PE: Use Auto mode to get all available textures in when init.
			uint32_t currMat = 0; // gggrass_global_params.paint_material;
			for (uint32_t i = 0; i < GGGRASS_NUM_SELECTABLE_TYPES; i++)
			{
				strcpy_s( grassFilename, "grassbank/" );
				strcat_s( grassFilename, GGGrass_GetTextureFilename(currMat, i) );

				t.visuals.sGrassTextures[i] = grassFilename;
				t.visuals.sGrassTexturesName[i] = GGGrass_GetTextureShortName(currMat, i);
				t.gamevisuals.sGrassTextures[i] = t.visuals.sGrassTextures[i];
				t.gamevisuals.sGrassTexturesName[i] = t.visuals.sGrassTexturesName[i];
			}
			bInitNewGrassSystem = false;
		}

		// Stage B.9: sync sGrassTextures[custom slots] -> Wicked's custom-slot registry every frame.
		// Cheap (compares std::string against stored value; only flags dirty on actual change). Handles:
		//   - Level load with custom entries populated from .fpm — Wicked wouldn't otherwise know
		//     about them until the user re-added.
		//   - Edge case where sGrassTextures got cleared by save/load without going through the
		//     Delete button — Wicked syncs the empty state too.
		for (int iL = GGGRASS_CUSTOM_SLOT_BASE; iL < GGGRASS_MAX_PALETTE_SLOTS; iL++)
		{
			const char* cur = t.visuals.sGrassTextures[iL].Get();
			GGGrass_SetCustomSlotFilename(iL, (cur && cur[0]) ? cur : nullptr);
		}

		// find last slot used (iterates the full palette so custom slots contribute to the last-used index)
		int iLastSlotUsed = 0;
		for (int iL = 0; iL < GGGRASS_MAX_PALETTE_SLOTS; iL++)
			if (t.visuals.sGrassTextures[iL] != "")
				if (ImageExist(t.terrain.imagestartindex + 180 + iL))
					iLastSlotUsed = iL;

		int iUsedImages = 0;
		float col_start = 90.0f;
		int iSelectGrassTexture = -1;

		ImGui::Indent(-10);


		struct icon_selections
		{
			bool bActive;
			ImRect image_bb;
		} draw_selections[128];
		for (int iL = 0; iL < GGGRASS_MAX_PALETTE_SLOTS; iL++) draw_selections[iL].bActive = false;

		//Child
		static float fContentHeight = 0;
		static ImVec2 vLastRunHeight = { 0,0 };
		if (fContentHeight <= 85) {
			fContentHeight = 85; //One line default. and prevent flicker.
		}
		vLastRunHeight = { 0 ,fContentHeight };
		int iActiveGrass = 0;
		ImVec2 oldstylemain = ImGui::GetStyle().FramePadding;
		ImVec2 oldwinstylemain = ImGui::GetStyle().WindowPadding;
		ImGui::GetStyle().WindowPadding = { 0,0 };
		ImGui::GetStyle().FramePadding = { 0,0 };
		ImVec2 child_begin = ImGui::GetCursorPos();
		ImVec2 vWindowPos = ImGui::GetWindowPos();
		ImVec2 vWindowSize = ImGui::GetWindowSize();
		ImGui::BeginChild("##grass4x4forscrollbar", vLastRunHeight, false, ImGuiWindowFlags_AlwaysVerticalScrollbar);

		ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(0, 3));

		//ImGui::Indent(4);
		ImGui::Columns(4, "##veg4x4columns", false);  //false no border

		float curposy = ImGui::GetCursorPosY();
		uint64_t values = gggrass_global_params.paint_type;
		// Stage B.9: iterate the full palette (stock 0..21 + custom 22..GGGRASS_MAX_PALETTE_SLOTS-1).
		// Empty slots (sGrassTextures[iL] == "") are skipped by the outer sanity check inside the loop,
		// so custom slots that haven't been populated yet don't draw.
		for (int iL = 0; iL < GGGRASS_MAX_PALETTE_SLOTS; iL++)
		{
			uint64_t mask = 1ULL << iL;
			if (t.visuals.sGrassTextures[iL] != "")
			{
				if (iActiveGrass++ <= 20) //record buttom
					fContentHeight = ImGui::GetCursorPosY() - curposy;
				
				if (!ImageExist(t.terrain.imagestartindex + 180 + iL))
				{
					//Load in image.
					//PE: Was hitting each frame with /NONE
					const char *bNoneGrass = pestrcasestr(t.visuals.sGrassTextures[iL].Get(), "/none");
					image_setlegacyimageloading(true);
					SetMipmapNum(1); //PE: mipmaps not needed.
					if (!bNoneGrass)
					{
						if (ImageExist(t.terrain.imagestartindex + 180 + iL) == 1) DeleteImage(t.terrain.imagestartindex + 180 + iL);
						LoadImage(t.visuals.sGrassTextures[iL].Get(), t.terrain.imagestartindex + 180 + iL, 0, g.gdividetexturesize);
					}
					if (!bNoneGrass && ImageExist(t.terrain.imagestartindex + 180 + iL) == 1)
					{
						sGrassTexturesID[iL] = t.terrain.imagestartindex + 180 + iL;
					}
					else
					{
						//Load failed, clear texture slot.
						t.visuals.sGrassTextures[iL] = "";
						t.gamevisuals.sGrassTextures[iL] = t.visuals.sGrassTextures[iL];
						g.projectmodified = 1;
						bUpdateGrassMaterials = true;
					}
					SetMipmapNum(-1);
					image_setlegacyimageloading(false);
				}
				else
				{
					sGrassTexturesID[iL] = t.terrain.imagestartindex + 180 + iL;
				}
				if (ImageExist(sGrassTexturesID[iL]))
				{
					iUsedImages++;

					float path_gadget_size = ImGui::GetFontSize()*3.0;
					int preview_icon_size = ImGui::GetFontSize();

					//PE: You can only change image slot 0 , not delete it, we need at least one texture.
					bool bDeleteImage = false;
					if (iL != 0 && iL == iLastSlotUsed) bDeleteImage = true;

					// chop grassbank from material name 
					cstr materialname = t.visuals.sGrassTexturesName[iL];

					cStr sLabel = cStr("##InputGrassTexture") + cStr(iL);

					//PE: New do 4x4 coloums here.

					int iLargerPreviewIconSize = 28;//PE: 54 , now lowest possible icon
					float control_width = (iLargerPreviewIconSize + 3.0) * 4.0f + 6.0;

					if (w > control_width) {
						//PE: fit perfectly with window width.
						iLargerPreviewIconSize = (w - 20.0) / 4.0;
						iLargerPreviewIconSize -= 8.0; //Padding.
						if (iLargerPreviewIconSize > 70) iLargerPreviewIconSize = 70;
					}

					ImVec2 vSelectionDraw = ImGui::GetCurrentWindow()->DC.CursorPos;

					if (sGrassTexturesID[iL] > 0)
					{
						cStr sLabelChild = cStr("##grass4x4") + cStr(iL);

						ImVec2 content_avail = { iLargerPreviewIconSize + 1.0f ,iLargerPreviewIconSize + 1.0f };

						//style.WindowPadding
						ImVec2 oldstyle = ImGui::GetStyle().FramePadding;
						ImGui::GetStyle().FramePadding = { 0,0 };
						ImGui::BeginChild(sLabelChild.Get(), content_avail, false, ImGuiWindowFlags_NoScrollbar);

						if (ImGui::ImgBtn(sGrassTexturesID[iL], ImVec2(iLargerPreviewIconSize, iLargerPreviewIconSize), ImColor(0, 0, 0, 255)))
						{
							//Now do toggle selection.
							if (bCurrentGrassTextureForPaint[iL])
								bCurrentGrassTextureForPaint[iL] = false;
							else
								bCurrentGrassTextureForPaint[iL] = true;

						}

						if ( ImGui::IsItemHovered())
						{
							int tooltip_height = 350;
							int tooltip_width = 350;
							ImVec2 tooltip_position = ImVec2(ImGui::GetWindowPos());
							tooltip_position.x = vWindowPos.x - (tooltip_width + 10.0);
							if ((tooltip_position.y + tooltip_height) > (vWindowPos.y+vWindowSize.y))
								tooltip_position.y = (vWindowPos.y+vWindowSize.y) - tooltip_height - 10.0;
							ImGui::SetNextWindowPos(tooltip_position);
							ImGui::BeginTooltip();
							ImGui::ImgBtn(sGrassTexturesID[iL], ImVec2(tooltip_width, tooltip_height), ImColor(0, 0, 0, 255), ImColor(220, 220, 220, 220), ImColor(255, 255, 255, 255), ImColor(180, 180, 160, 255), -1, 0, 0, 0, false, false, true);
							ImGui::TextCenter(materialname.Get());
							ImGui::Separator();
							ImGui::EndTooltip();
						}

						ImGui::EndChild();
						ImGui::GetStyle().FramePadding = oldstyle;

					}
					sLabel = cStr("##veg") + cStr(iL);

					float checkwidth = ImGui::GetFontSize()*1.5;
					ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((ImGui::GetContentRegionAvail().x*0.5) - (checkwidth*0.5), 0.0f));

					if (bCurrentGrassTextureForPaint[iL])
					{
						ImVec2 padding = { 2, 2 };
						const ImRect image_bb((vSelectionDraw - padding), vSelectionDraw + padding + ImVec2(iLargerPreviewIconSize, iLargerPreviewIconSize));
						draw_selections[iL].bActive = true;
						draw_selections[iL].image_bb = image_bb;
						values |= mask;
					}
					else
					{
						values &= ~mask;
					}

					ImGui::NextColumn();

				}
				else
				{
					values &= ~mask;
				}
			}
			else
			{
				//Delete old image.
				if (ImageExist(t.terrain.imagestartindex + 180 + iL) == 1) DeleteImage(t.terrain.imagestartindex + 180 + iL);
				sGrassTexturesID[iL] = 0;
				bCurrentGrassTextureForPaint[iL] = false;
				values &= ~mask;
			}
		}

		if (iActiveGrass <= 20)
			fContentHeight = 2.0 + (ImGui::GetCursorPosY() - curposy);

		ImGui::GetStyle().WindowPadding = oldwinstylemain;
		ImGui::GetStyle().FramePadding = oldstylemain;

		ImGui::EndChild();
		
		child_begin -= ImVec2(ImGui::GetScrollX(), ImGui::GetScrollY());
		ImRect cliprect = ImRect(ImGui::GetWindowPos() + child_begin, ImGui::GetWindowPos() + child_begin + ImVec2(ImGui::GetWindowContentRegionWidth(), fContentHeight));
		for (int iL = 0; iL < GGGRASS_MAX_PALETTE_SLOTS; iL++)
		{
			if (draw_selections[iL].bActive)
			{
				//curposy
				ImGui::GetCurrentWindow()->DrawList->PushClipRect(cliprect.Min, cliprect.Max, true);
				ImGui::GetCurrentWindow()->DrawList->AddRect(draw_selections[iL].image_bb.Min, draw_selections[iL].image_bb.Max, ImGui::GetColorU32(tool_selected_col), 0.0f, 15, 2.0f);
				ImGui::GetCurrentWindow()->DrawList->PopClipRect();
			}
		}
		gggrass_global_params.paint_type = values;


		//ImGui::Indent(-4);
		ImGui::Columns(1);
		ImGui::Indent(10);

		// Stage B.9: Add New Grass / Delete Grass buttons — expose custom palette slots to end users.
		// Stock slots (0..21) are protected from deletion; custom slots (22..GGGRASS_MAX_PALETTE_SLOTS-1)
		// can be populated via file dialog and cleared via Delete. Selected-for-paint slots highlight
		// through bCurrentGrassTextureForPaint[].
		{
			int iSelectedSlot = -1;
			int iSelectedCount = 0;
			for (int iL = 0; iL < GGGRASS_MAX_PALETTE_SLOTS; iL++)
			{
				if (bCurrentGrassTextureForPaint[iL])
				{
					iSelectedSlot = iL;
					iSelectedCount++;
				}
			}
			int iCustomCount = 0;
			int iFirstEmptyCustom = -1;
			for (int iL = GGGRASS_CUSTOM_SLOT_BASE; iL < GGGRASS_MAX_PALETTE_SLOTS; iL++)
			{
				if (t.visuals.sGrassTextures[iL] != "")
					iCustomCount++;
				else if (iFirstEmptyCustom < 0)
					iFirstEmptyCustom = iL;
			}
			bool canAdd = (iFirstEmptyCustom >= 0);
			bool canDelete = (iSelectedCount == 1 && iSelectedSlot >= GGGRASS_CUSTOM_SLOT_BASE);

			float w = ImGui::GetContentRegionAvail().x;
			float but_gadget_size = ImGui::GetFontSize() * 8.5f;
			float total_w = but_gadget_size * 2.0f + 6.0f;
			ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((w * 0.5f) - (total_w * 0.5f), 0.0f));

			// Buttons don't gray out (older ImGui in this project has no BeginDisabled/EndDisabled);
			// instead they gate on canAdd/canDelete inside their handler and the tooltip explains
			// why the click was a no-op.
			if (ImGui::StyleButton("Add New Grass##AddCustomGrass", ImVec2(but_gadget_size, 0)))
			{
				if (canAdd)
				{
					// Open a file dialog and let the user pick a DDS from Files/grassbank/ (or elsewhere
					// under Files/). We store the path RELATIVE to the game's root dir so save/load into
					// .fpm survives; GGGrass_SetCustomSlotFilename holds it for the Wicked renderer.
					cStr sGrassTextureFolder = g.rootdir_s + "grassbank\\";
					cStr tOldDir = GetDir();
					char* cFileSelected = (char*)noc_file_dialog_open(NOC_FILE_DIALOG_OPEN, "DDS\0*.dds\0\0\0", sGrassTextureFolder.Get(), NULL);
					SetDir(tOldDir.Get());
					if (cFileSelected && strlen(cFileSelected) > 0 && iFirstEmptyCustom >= 0)
					{
						char* relonly = (char*)pestrcasestr(cFileSelected, g.rootdir_s.Get());
						cStr relPath;
						if (relonly) relPath = cFileSelected + g.rootdir_s.Len();
						else         relPath = cFileSelected;
						// Strip "Files/" prefix (case-insensitive) if present — sGrassTextures stores paths
						// relative to Files/ (matches stock convention "grassbank/foo.dds").
						const char* p = relPath.Get();
						if (p && strlen(p) > 6)
						{
							if ((p[0]=='F'||p[0]=='f') && (p[1]=='i'||p[1]=='I') && (p[2]=='l'||p[2]=='L') &&
							    (p[3]=='e'||p[3]=='E') && (p[4]=='s'||p[4]=='S') && (p[5]=='/' || p[5]=='\\'))
							{
								relPath = (char*)(p + 6);
							}
						}

						t.visuals.sGrassTextures[iFirstEmptyCustom] = relPath;
						t.gamevisuals.sGrassTextures[iFirstEmptyCustom] = relPath;

						// Derive a short display name from the filename stem.
						cStr shortName = relPath;
						const char* sp = shortName.Get();
						const char* lastSlash = nullptr;
						for (const char* q = sp; *q; ++q) { if (*q == '/' || *q == '\\') lastSlash = q + 1; }
						if (lastSlash) shortName = (char*)lastSlash;
						t.visuals.sGrassTexturesName[iFirstEmptyCustom] = shortName;
						t.gamevisuals.sGrassTexturesName[iFirstEmptyCustom] = shortName;

						GGGrass_SetCustomSlotFilename(iFirstEmptyCustom, relPath.Get());
						bCurrentGrassTextureForPaint[iFirstEmptyCustom] = true; // auto-select the newly added
						bUpdateGrassMaterials = true;
						g.projectmodified = 1;
					}
				}
			}
			if (ImGui::IsItemHovered())
			{
				if (canAdd) ImGui::SetTooltip("Pick a DDS file to add as a new custom grass texture");
				else        ImGui::SetTooltip("All %d custom grass slots are full", GGGRASS_MAX_PALETTE_SLOTS - GGGRASS_CUSTOM_SLOT_BASE);
			}

			ImGui::SameLine();

			if (ImGui::StyleButton("Delete Grass##DeleteCustomGrass", ImVec2(but_gadget_size, 0)))
			{
				if (canDelete && iSelectedSlot >= GGGRASS_CUSTOM_SLOT_BASE)
				{
					// Queue image slot for deletion via existing plumbing.
					if (ImageExist(t.terrain.imagestartindex + 180 + iSelectedSlot))
						iDeleteSingleGrassTextures = t.terrain.imagestartindex + 180 + iSelectedSlot;

					t.visuals.sGrassTextures[iSelectedSlot] = "";
					t.visuals.sGrassTexturesName[iSelectedSlot] = "";
					t.gamevisuals.sGrassTextures[iSelectedSlot] = "";
					t.gamevisuals.sGrassTexturesName[iSelectedSlot] = "";
					sGrassTexturesID[iSelectedSlot] = 0;
					bCurrentGrassTextureForPaint[iSelectedSlot] = false;
					// Clear the corresponding bit in paint_type so the paint code won't try to encode it.
					gggrass_global_params.paint_type &= ~(1ULL << iSelectedSlot);

					GGGrass_SetCustomSlotFilename(iSelectedSlot, nullptr);
					bUpdateGrassMaterials = true;
					g.projectmodified = 1;
				}
			}
			if (ImGui::IsItemHovered())
			{
				if (canDelete)                         ImGui::SetTooltip("Delete the selected custom grass texture");
				else if (iSelectedCount == 0)          ImGui::SetTooltip("Select exactly one custom grass texture to delete");
				else if (iSelectedCount > 1)           ImGui::SetTooltip("Select exactly ONE custom grass texture to delete (currently %d selected)", iSelectedCount);
				else                                   ImGui::SetTooltip("Stock grass textures (first %d slots) cannot be deleted", GGGRASS_CUSTOM_SLOT_BASE);
			}

			ImGui::Text("Custom: %d / %d", iCustomCount, GGGRASS_MAX_PALETTE_SLOTS - GGGRASS_CUSTOM_SLOT_BASE);
		}

		if (bUpdateGrassMaterials)
		{
			bUpdateGrassMaterials = false;
		}
		ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));

		// no need to toggle veg, we ALWAYS 'may' need it!
		bEnableVeg = true;

		ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));
		ImGui::Indent(-10);
	}
}

void imgui_Customize_Weather(int mode)
{
#ifdef ALLOW_WEATHER_IN_EDITOR
	//PE: Default to closed.
	int wflags = ImGuiTreeNodeFlags_None;

	if (pref.bAutoClosePropertySections && iLastOpenHeader != 4)
		ImGui::SetNextItemOpen(false, ImGuiCond_Always);


	if (ImGui::StyleCollapsingHeader("Weather", wflags)) {

		iLastOpenHeader = 4;

		ImGui::Indent(10);

		const char* items_align[] = { "None" , "Light Rain", "Heavy Rain","Light Snow" ,"Heavy Snow" }; //,"Test"
		int item_current_type_selection = 0;
		item_current_type_selection = t.visuals.iEnvironmentWeather;
		ImGui::PushItemWidth(-10);
		if (ImGui::Combo("##WeatherDropwDown", &item_current_type_selection, items_align, IM_ARRAYSIZE(items_align))) {
			t.visuals.iEnvironmentWeather = item_current_type_selection;
			t.gamevisuals.iEnvironmentWeather = t.visuals.iEnvironmentWeather;
		}
		ImGui::PopItemWidth();
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", "Weather");

		if (ImGui::Checkbox("Display Weather##DisplayWeather", &bEnableWeather))
		{
			reset_env_particles();
		}
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("Display Weather in Editor");

		ImGui::Indent(-10);
	}
#endif
}


void imgui_Customize_Water(int mode)
{
	void tab_tab_Column_text(char *text, float fColumn);

	//PE: Default to closed.
	int wflags = ImGuiTreeNodeFlags_None;
	if (pref.bAutoClosePropertySections && iLastOpenHeader != 32)
		ImGui::SetNextItemOpen(false, ImGuiCond_Always);


	if (ImGui::StyleCollapsingHeader("Water", wflags)) {

		iLastOpenHeader = 32;

		ImGui::Indent(10);
		float fTabColumnWidth = 120.0f;

		ImGui::PushItemWidth(-10);
		
		if (ImGui::Checkbox("Enable Water##v2bEnableWater", &t.visuals.bWaterEnable)) {
			t.gamevisuals.bWaterEnable = t.visuals.bWaterEnable;
			Wicked_Update_Visuals((void *)&t.visuals);
		}
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("Enable Water");
		

		if (t.visuals.bWaterEnable)
		{
			t.terrain.waterliney_f = g.gdefaultwaterheight;

			ImGui::PushItemWidth(-10);
			float waterHeight = GGTerrain_UnitsToMeters( g.gdefaultwaterheight );
			if (ImGui::SliderFloat("##igdefaultwaterheight:", &waterHeight, -500, 1500, "%.1f", 2.0f))
			{
				g.gdefaultwaterheight = waterHeight;
				Wicked_Update_Visuals((void *)&t.visuals);
			}
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Water Height");
			ImGui::PopItemWidth();


			ImGui::PushItemWidth(-10);
			if (ImGui::SliderFloat("##fWaterSpeed1:", &t.visuals.WaterSpeed1, -100.0f, 100.0f))
			{
				t.gamevisuals.WaterSpeed1 = t.visuals.WaterSpeed1;
				Wicked_Update_Visuals((void *)&t.visuals);
			}
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Water Speed");
			ImGui::PopItemWidth();


			float fWaterColor[4];

			fWaterColor[0] = t.visuals.WaterRed_f / 255.0;
			fWaterColor[1] = t.visuals.WaterGreen_f / 255.0;
			fWaterColor[2] = t.visuals.WaterBlue_f / 255.0;
			fWaterColor[3] = t.visuals.WaterAlpha_f / 255.0;
			ImGui::PushItemWidth(-10);
			if (ImGui::ColorEdit3("##V2WickedWaterColor", &fWaterColor[0], ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_DisplayRGB))
			{
				t.gamevisuals.WaterRed_f = t.visuals.WaterRed_f = fWaterColor[0] * 255.0;
				t.gamevisuals.WaterGreen_f = t.visuals.WaterGreen_f = fWaterColor[1] * 255.0;
				t.gamevisuals.WaterBlue_f = t.visuals.WaterBlue_f = fWaterColor[2] * 255.0;
				t.gamevisuals.WaterAlpha_f = t.visuals.WaterAlpha_f = fWaterColor[3] * 255.0;

				g.projectmodified = 1;
				Wicked_Update_Visuals((void *)&t.visuals);
			}
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Change the water base color applied as part of the overall effect");

			ImGui::PopItemWidth();

			ImGui::PushItemWidth(-10);
			if (ImGui::SliderFloat("##fWaterWaveAmplitude:", &t.visuals.fWaterWaveAmplitude, 0.0f, 1000.0f))
			{
				t.gamevisuals.fWaterWaveAmplitude = t.visuals.fWaterWaveAmplitude;
				Wicked_Update_Visuals((void *)&t.visuals);
			}
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Water Wave Size");
			ImGui::PopItemWidth();
			

			ImGui::PushItemWidth(-10);
			if (ImGui::SliderFloat("##fWaterWindDependency:", &t.visuals.fWaterWindDependency, 0.0f, 1.0f))
			{
				t.gamevisuals.fWaterWindDependency = t.visuals.fWaterWindDependency;
				Wicked_Update_Visuals((void *)&t.visuals);
			}
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Water Wind Contribution");
			ImGui::PopItemWidth();


			ImGui::PushItemWidth(-10);
			if (ImGui::SliderFloat("##fWaterPatchLength:", &t.visuals.fWaterPatchLength, 10.0f, 2000.0f))
			{
				t.gamevisuals.fWaterPatchLength = t.visuals.fWaterPatchLength;
				Wicked_Update_Visuals((void *)&t.visuals);
			}
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Water Tiling Patch Size");
			ImGui::PopItemWidth();


			ImGui::PushItemWidth(-10);
			if (ImGui::SliderFloat("##fWaterCausticSize:", &t.visuals.fWaterCausticSize, 0.5f, 12.0f))
			{
				t.gamevisuals.fWaterCausticSize = t.visuals.fWaterCausticSize;
				Wicked_Update_Visuals((void *)&t.visuals);
			}
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Underwater Caustic Size (bigger = larger light ripples on the seabed)");
			ImGui::PopItemWidth();


			ImGui::PushItemWidth(-10);
			if (ImGui::SliderFloat("##fWaterChoppyScale:", &t.visuals.fWaterChoppyScale, 0.0f, 10.0f))
			{
				t.gamevisuals.fWaterChoppyScale = t.visuals.fWaterChoppyScale;
				Wicked_Update_Visuals((void *)&t.visuals);
			}
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Water Wave Choppiness");
			ImGui::PopItemWidth();

			ImGui::PushItemWidth(-10);
			if (ImGui::SliderFloat("##fWaterFogMinAmount", &t.visuals.WaterFogMinAmount, 0.0f, 1.0f, "%.3f", 1.0f))
			{
				t.gamevisuals.WaterFogMinAmount = t.visuals.WaterFogMinAmount;
				Wicked_Update_Visuals((void *)&t.visuals);
			}
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Water Fog Min Amount");
			ImGui::PopItemWidth();

			ImGui::PushItemWidth(-10);
			if (ImGui::SliderFloat("##fWaterFogMinDist", &t.visuals.WaterFogMinDist, 0.0f, 100000.0f, "%.2f", 2.0f))
			{
				if ( t.visuals.WaterFogMinDist > t.visuals.WaterFogMaxDist ) t.visuals.WaterFogMaxDist = t.visuals.WaterFogMinDist+0.1;
				t.gamevisuals.WaterFogMinDist = t.visuals.WaterFogMinDist;
				Wicked_Update_Visuals((void *)&t.visuals);
			}
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Water Fog Min Distance");
			
			if (ImGui::SliderFloat("##fWaterFogMaxDist", &t.visuals.WaterFogMaxDist, 0.0f, 100000.0f, "%.2f", 2.0f))
			{
				if ( t.visuals.WaterFogMinDist > t.visuals.WaterFogMaxDist ) t.visuals.WaterFogMinDist = t.visuals.WaterFogMaxDist-0.1;
				t.gamevisuals.WaterFogMaxDist = t.visuals.WaterFogMaxDist;
				Wicked_Update_Visuals((void *)&t.visuals);
			}
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Water Fog Max Distance");
			ImGui::PopItemWidth();
		}
		ImGui::PopItemWidth();

		ImGui::Indent(-10);
	}
}

void imgui_Customize_Water_V2(int mode)
{
	void tab_tab_Column_text(char *text, float fColumn);

	// have no access to water settings in completely-empty mode
	if (t.visuals.bEnableEmptyLevelMode == true) return;

	//PE: NOTE tabtab mode < 3 normally used iLastOpenHeader = 5
	int wflags = ImGuiTreeNodeFlags_None;
	if (mode == 4)
	{
		// Displayed in terrain tools
		if (pref.bAutoClosePropertySections && iLastOpenHeader != 32)
			ImGui::SetNextItemOpen(false, ImGuiCond_Always);
	}
	else
	{
		// Displayed in tabtab visuals
		if (pref.bAutoClosePropertySections && iLastOpenHeader != 5)
			ImGui::SetNextItemOpen(false, ImGuiCond_Always);
	}
	ImVec2 vHoverRectStart = ImGui::GetCursorPos();
	if (ImGui::StyleCollapsingHeader("Water", wflags))
	{
		if (mode != 4)
			iLastOpenHeader = 5;
		else
			iLastOpenHeader = 32;

		ImGui::Indent(10);
		ImGui::PushItemWidth(-10);
		float w = ImGui::GetWindowContentRegionWidth();

		float fTabColumnWidth = 120.0f;

		if (pref.iEnableAdvancedWater)
		{
		}
		else 
		{
			t.gamevisuals.bWaterEnable = t.visuals.bWaterEnable = true;
		}

		if (t.visuals.bWaterEnable)
		{

			t.terrain.waterliney_f = g.gdefaultwaterheight;

			// water height calc is confusing if need it as a unit (as it has min/max and power applied to match terrain biome params)
			if (pref.iEnableAdvancedWater)
			{
				// simpler Y axis units for working with water control
				ImGui::TextCenter("Water Height (units)");
				float fTmp = g.gdefaultwaterheight - 3937;
				int iMinValue = -100 * 39.37f;
				int iMaxValue = 300 * 39.37f;
				if (ImGui::MaxSliderInputFloat("##igdefaultwaterheightinunits", &fTmp, iMinValue, iMaxValue, "Set Water Height based on simple Y-axis unit value", iMinValue, fabs(iMinValue)+fabs(iMaxValue)))
				{
					g.gdefaultwaterheight = fTmp + 3937;
					Wicked_Update_Visuals((void*)&t.visuals);
					g.projectmodified = 1;
					ggterrain_extra_params.iUpdateTrees = 1;
				}
			}
			else
			{
				ImGui::TextCenter("Water Height (meters)");
				float fTmp = GGTerrain_UnitsToMeters(g.gdefaultwaterheight);
				if (ImGui::MaxSliderInputFloatPower("##igdefaultwaterheight", &fTmp, -100.0, 300.0, "Set Water Height using a meter scale tied to the original biome settings", -100, 300, 30, 1.2f, 1))
				{
					g.gdefaultwaterheight = GGTerrain_MetersToUnits(fTmp);
					Wicked_Update_Visuals((void*)&t.visuals);
					g.projectmodified = 1;
					ggterrain_extra_params.iUpdateTrees = 1;
				}
			}

			ImGui::TextCenter("Water Speed");
			float fTmp = t.visuals.WaterSpeed1 * 100.0f;
			if (ImGui::MaxSliderInputFloat("##fWaterSpeed1:", &fTmp, 0.0f, 50.0f, "Set Water Speed", 0, 50.0f))
			{
				t.visuals.WaterSpeed1 = fTmp * 0.01f;
				t.gamevisuals.WaterSpeed1 = t.visuals.WaterSpeed1;
				Wicked_Update_Visuals((void *)&t.visuals);
				g.projectmodified = 1;
			}

			ImGui::TextCenter("Water Base Color");
			ImVec4 mycolor = ImVec4(t.visuals.WaterRed_f / 255.0, t.visuals.WaterGreen_f / 255.0, t.visuals.WaterBlue_f / 255.0, 1.0);

			bool open_popup = ImGui::ColorButton("##NewV2WickedWaterColor", mycolor, 0, ImVec2(w - 20.0, 0));
			if (open_popup)
				ImGui::OpenPopup("##pickV2WickedWaterColor");
			if (ImGui::BeginPopup("##pickV2WickedWaterColor", ImGuiWindowFlags_NoMove))
			{
				if (ImGui::ColorPicker4("##pickerV2WickedWaterColor", (float*)&mycolor, ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoSmallPreview))
				{
					t.gamevisuals.WaterRed_f = t.visuals.WaterRed_f = mycolor.x * 255.0;
					t.gamevisuals.WaterGreen_f = t.visuals.WaterGreen_f = mycolor.y * 255.0;
					t.gamevisuals.WaterBlue_f = t.visuals.WaterBlue_f = mycolor.z * 255.0;
					t.gamevisuals.WaterAlpha_f = t.visuals.WaterAlpha_f = mycolor.w * 255.0;

					g.projectmodified = 1;
					Wicked_Update_Visuals((void *)&t.visuals);
				}
				ImGui::EndPopup();
			}

			ImGuiWindow* window = ImGui::GetCurrentWindow(); //PE: Add a pencil to all color gadgets.
			ID3D11ShaderResourceView* lpTexture = GetImagePointerView(TOOL_PENCIL);
			ImVec2 vDrawPos = { ImGui::GetCursorScreenPos().x + (ImGui::GetContentRegionAvail().x - 30.0f) ,ImGui::GetCursorScreenPos().y - (ImGui::GetFontSize()*1.5f) - 3.0f };
			window->DrawList->AddImage((ImTextureID)lpTexture, vDrawPos, vDrawPos + ImVec2(16, 16), ImVec2(0, 0), ImVec2(1, 1), ImGui::GetColorU32(ImVec4(1, 1, 1, 1)));
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Set Water Color");

			extern void ControlAdvancedSetting(int&, const char*, bool* = nullptr);
			ControlAdvancedSetting(pref.iEnableAdvancedWater, "Advanced Water Settings");

			if (pref.iEnableAdvancedWater)
			{
				ImGui::TextCenter("Water Wave Size");
				if (ImGui::MaxSliderInputFloat("##fWaterWaveAmplitude:", &t.visuals.fWaterWaveAmplitude, 0.0f, 800.0f, "Set Water Wave Size", 0, 800))
				{
					t.gamevisuals.fWaterWaveAmplitude = t.visuals.fWaterWaveAmplitude;
					Wicked_Update_Visuals((void*)&t.visuals);
					g.projectmodified = 1;
				}
				ImGui::TextCenter("Water Wind Contribution");
				fTmp = t.visuals.fWaterWindDependency * 100.0f;
				if (ImGui::MaxSliderInputFloat("##fWaterWindDependency:", &fTmp, 0.0f, 100.0f, "Set Water Wind Contribution", 0, 100.0f))
				{
					t.visuals.fWaterWindDependency = fTmp * 0.01f;
					t.gamevisuals.fWaterWindDependency = t.visuals.fWaterWindDependency;
					Wicked_Update_Visuals((void*)&t.visuals);
					g.projectmodified = 1;
				}
				ImGui::TextCenter("Water Tiling Patch Size");
				if (ImGui::MaxSliderInputFloat("##fWaterPatchLength:", &t.visuals.fWaterPatchLength, 10.0f, 300.0f, "Set Water Tile Size", 10.0f, 300.0f))
				{
					t.gamevisuals.fWaterPatchLength = t.visuals.fWaterPatchLength;
					Wicked_Update_Visuals((void*)&t.visuals);
					g.projectmodified = 1;
				}
				// NOTE: MaxSliderInputFloat is INTEGER-backed (drives a SliderInt from startval..maxval
				// with ceil() quantisation), and arg6/arg7 are (int startval, float maxval) - NOT a
				// second min/max pair. It also permanently expands its own max if the value ever
				// exceeds v_max, caching that in g_SliderData for the session. So: work in tenths
				// (1..120 == size 0.1..12.0, giving usable 0.1 granularity) and clamp before the call,
				// otherwise one stray value poisons the slider range until restart.
				ImGui::TextCenter("Caustic Size");
				fTmp = t.visuals.fWaterCausticSize * 10.0f;
				if (fTmp < 10.0f || fTmp > 100.0f) fTmp = 30.0f; // 30 == default size 3.0
				if (ImGui::MaxSliderInputFloat("##fWaterCausticSize:", &fTmp, 10.0f, 100.0f, "Size of the caustic light ripples cast on the sea floor. Independent of Water Tiling Patch Size, so it does NOT change wave size. 10 = fine stock speckle, 30 = larger cells, 100 = broad soft blobs", 10, 100.0f))
				{
					t.visuals.fWaterCausticSize = fTmp * 0.1f;
					t.gamevisuals.fWaterCausticSize = t.visuals.fWaterCausticSize;
					Wicked_Update_Visuals((void*)&t.visuals);
					g.projectmodified = 1;
				}
				ImGui::TextCenter("Water Wave Choppiness");
				fTmp = t.visuals.fWaterChoppyScale * 10.0f;
				if (ImGui::MaxSliderInputFloat("##fWaterChoppyScale:", &fTmp, 0.0f, 100.0f, "Set Water Wave Choppiness"))
				{
					t.visuals.fWaterChoppyScale = fTmp * 0.1f;
					t.gamevisuals.fWaterChoppyScale = t.visuals.fWaterChoppyScale;
					Wicked_Update_Visuals((void*)&t.visuals);
					g.projectmodified = 1;
				}


				ImGui::TextCenter("Water Fog Start");
				if (ImGui::MaxSliderInputFloat("##fWaterFogMinDist", &t.visuals.WaterFogMinDist, 0.0f, 100000.0f, "Closer than this distance under water will have the minimum amount of fog"))
				{
					if (t.visuals.WaterFogMinDist > t.visuals.WaterFogMaxDist) t.visuals.WaterFogMaxDist = t.visuals.WaterFogMinDist;
					t.gamevisuals.WaterFogMinDist = t.visuals.WaterFogMinDist;
					Wicked_Update_Visuals((void*)&t.visuals);
					g.projectmodified = 1;
				}

				ImGui::TextCenter("Water Fog Maximum");
				if (ImGui::MaxSliderInputFloat("##fWaterFogMaxDist", &t.visuals.WaterFogMaxDist, 0.0f, 100000.0f, "After this distance under water it will be completely opaque"))
				{
					if (t.visuals.WaterFogMinDist > t.visuals.WaterFogMaxDist) t.visuals.WaterFogMinDist = t.visuals.WaterFogMaxDist;
					t.gamevisuals.WaterFogMaxDist = t.visuals.WaterFogMaxDist;
					Wicked_Update_Visuals((void*)&t.visuals);
					g.projectmodified = 1;
				}

				ImGui::TextCenter("Water Fog Minimum");
				if (ImGui::MaxSliderInputFloat("##fWaterFogMinAmount", &t.visuals.WaterFogMinAmount, 0.0f, 1.0f, "The minimum amount of under water fog that will always be present regardless of distance"))
				{
					t.gamevisuals.WaterFogMinAmount = t.visuals.WaterFogMinAmount;
					Wicked_Update_Visuals((void*)&t.visuals);
					g.projectmodified = 1;
				}
			}
		}
		ImGui::PopItemWidth();

		if (mode == 4)
		{
			//PE: To follow new auto close headers.
			if (pref.bAutoClosePropertySections)
			{
			}
			else
			{
				ImVec2 vHoverRectEnd = ImGui::GetCursorPos();
				ImRect bbwin(ImGui::GetWindowPos() + ImVec2(0, vHoverRectStart.y - ImGui::GetScrollY()), ImGui::GetWindowPos() + ImVec2(ImGui::GetWindowSize().x + 20.0, vHoverRectEnd.y - ImGui::GetScrollY()));
				if (ImGui::IsMouseHoveringRect(bbwin.Min, bbwin.Max))
				{
					ggterrain_extra_params.edit_mode = 0;
					ggterrain_global_render_params2.flags2 &= ~GGTERRAIN_SHADER_FLAG2_SHOW_BRUSH_SIZE;
					bImGuiGotFocus = true;
				}
			}
		}
		ImGui::Indent(-10);
	}
}


void imgui_Customize_Logic_Settings(int mode)
{
	bool bVisualUpdated = false;
	float fTabColumnWidth = 120.0f;
	bool Global_Behaviors_Settings(float fTabColumnWidth, bool bVisualUpdated);
	bVisualUpdated = Global_Behaviors_Settings(fTabColumnWidth, bVisualUpdated);

	// Control all in-game debugging options 
	bool AI_Management_Settings(float fTabColumnWidth, bool bVisualUpdated);
	bVisualUpdated = AI_Management_Settings(fTabColumnWidth, bVisualUpdated);

	if (bVisualUpdated)
	{
		// visuals have been updated, inform wicked engine and mark level has modified
		Wicked_Update_Visuals((void*)&t.visuals);
		g.projectmodified = 1;
	}

}


void imgui_Customize_Game_Settings(int mode)
{
	bool bVisualUpdated = false;
	float fTabColumnWidth = 120.0f;
	float w = ImGui::GetWindowContentRegionWidth();

	bool Graphics_Performance_Settings(float fTabColumnWidth, bool bVisualUpdated);
	bVisualUpdated = Graphics_Performance_Settings(fTabColumnWidth, bVisualUpdated);

	bool PostProcess_Settings(float fTabColumnWidth, bool bVisualUpdated);
	bVisualUpdated = PostProcess_Settings(fTabColumnWidth, bVisualUpdated);

	bool Shadows_Settings(float fTabColumnWidth, bool bVisualUpdated);
	bVisualUpdated = Shadows_Settings(fTabColumnWidth, bVisualUpdated);

	//Reset
	ImGui::Separator();
	float but_gadget_size = ImGui::GetFontSize() * 10.0;
	ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((w * 0.5) - (but_gadget_size * 0.5), 0.0f));
	if (ImGui::StyleButton("Reset Visuals##WickedResetVisualsUniqueId", ImVec2(but_gadget_size, 0)))
	{
		int iAction = askBoxCancel("This will delete all your visual changes, are you sure?", "Confirmation"); //1==Yes 2=Cancel 0=No
		if (iAction == 1)
		{
			//Reset
			visuals_resetvalues(false);
			t.gamevisuals = t.visuals;
			bVisualUpdated = true;
		}
	}
	if (ImGui::IsItemHovered()) ImGui::SetTooltip("Reset Visuals to Default Values");

	if (bVisualUpdated)
	{
		// visuals have been updated, inform wicked engine and mark level has modified
		Wicked_Update_Visuals((void*)&t.visuals);
		g.projectmodified = 1;
	}


}
