void charactercreatorplus_imgui_v3(void)
{
	extern bool bImGuiGotFocus;
	extern bool bForceKey;
	extern cstr csForceKey;
	extern bool bEntity_Properties_Window;
	bool once_camera_adjust = false;
	if (g_bCharacterCreatorPlusActivated)
	{
		if (!g_bPartIconsInit)
		{
			for (int b = 0; b < 32; b++)
			{
				for (int i = 0; i < 10; i++)
				{
					for (int a = 0; a < MAXPARTICONS; a++)
						g_iPartsIconsIDs[b][i][a] = -1;
				}
			}
			g_bPartIconsInit = true;
		}

		if (g_CharacterCreatorPlus.bInitialised)
		{
			// handle thread dependent triggers (smooth UI) 
			charactercreatorplus_waitforpreptofinish();

			charactercreatorplus_performcameratransition();

			// handle in-level visuals
			if (!bCharObjVisible)
			{
				oldtcameraviewmode = t.cameraviewmode;
				editoroldmode_f = t.editorfreeflight.mode;
				editoroldx_f = t.editorfreeflight.c.x_f;
				if (editoroldx_f == 0) editoroldx_f = 0.0001;
				editoroldy_f = t.editorfreeflight.c.y_f;
				editoroldz_f = t.editorfreeflight.c.z_f;
				editoroldangx_f = t.editorfreeflight.c.angx_f;
				editoroldangy_f = t.editorfreeflight.c.angy_f;

				t.editorfreeflight.c.angx_f = 16.0;
				t.editorfreeflight.c.angy_f = 315.0;
				RotateCamera(16.0, 315.0, 0.0); //To always get the same light.

				ShowObject(iCharObj);
				bCharObjVisible = true;

				//Hide entities.
				//Exit properties.
				bForceKey = true;
				csForceKey = "e";

				t.inputsys.dowaypointview = 1;
				t.inputsys.domodeentity = 1;

				widget_hide();
				ebe_hide();
				terrain_paintselector_hide();
				t.geditorhighlightingtentityobj = 0;
				t.geditorhighlightingtentityID = 0;
				editor_restoreentityhighlightobj();
				gridedit_clearentityrubberbandlist();
				waypoint_hideall();

				ccpTargetX = t.editorfreeflight.c.x_f;
				ccpTargetY = t.editorfreeflight.c.y_f;
				ccpTargetZ = t.editorfreeflight.c.z_f;
				ccpTargetAX = t.editorfreeflight.c.angx_f;
				ccpTargetAY = t.editorfreeflight.c.angy_f;

				fCharObjectY = 180000.0f;

				float oangx = ObjectAngleX(iCharObj);
				float oangz = ObjectAngleZ(iCharObj);

				//PE: a simple z,x mouse from center of screen.
				float placeatx_f, placeatz_f;
				extern ImVec2 OldrenderTargetSize;
				extern ImVec2 OldrenderTargetPos;
				extern ImVec2 renderTargetAreaSize;
				extern ImVec2 renderTargetAreaPos;
				extern bool bWaypointDrawmode;

				ImVec2 vCenterPos = { (OldrenderTargetSize.x*0.5f) + OldrenderTargetPos.x , (OldrenderTargetSize.y*0.45f) + OldrenderTargetPos.y };

				int omx = t.inputsys.xmouse, omy = t.inputsys.ymouse, oldgridentitysurfacesnap = t.gridentitysurfacesnap, oldonedrag = t.onedrag;;
				bool owdm = bWaypointDrawmode;

				//Always target terrain only.
				float RatioX = ((float)GetDisplayWidth() / (float)renderTargetAreaSize.x) * ((float)GetDisplayWidth() / (float)GetChildWindowWidth(-1));
				float RatioY = ((float)GetDisplayHeight() / (float)renderTargetAreaSize.y) * ((float)GetDisplayHeight() / (float)GetChildWindowHeight(-1));
				t.inputsys.xmouse = (vCenterPos.x - renderTargetAreaPos.x) * RatioX;
				t.inputsys.ymouse = (vCenterPos.y - renderTargetAreaPos.y) * RatioY;

				t.gridentitysurfacesnap = 0; t.onedrag = 0; bWaypointDrawmode = false;

				input_calculatelocalcursor();

				if (!(t.inputsys.picksystemused == 1 || t.inputsys.localcurrentterrainheight_f < 100.0f))
				{
					ccpTargetX = t.inputsys.localx_f;
					ccpTargetZ = t.inputsys.localy_f;
				}

				t.onedrag = oldonedrag;
				bWaypointDrawmode = owdm;
				t.gridentitysurfacesnap = oldgridentitysurfacesnap;
				t.inputsys.xmouse = omx;
				t.inputsys.ymouse = omy;
				//Restore real input.
				input_calculatelocalcursor();

				fCharObjectY = 180000.0f;

				t.editorfreeflight.c.x_f = ccpTargetX;
				t.editorfreeflight.c.z_f = ccpTargetZ;

				SetObjectToCameraOrientation(iCharObj);
				PositionObject(iCharObj, ccpTargetX, fCharObjectY, ccpTargetZ);
				RotateObject(iCharObj, oangx, ObjectAngleY(iCharObj), oangz);
				MoveObject(iCharObj, 130);

				// Set default visuals for the Character Creator, and restore them when leaving.
				visualsdatastoragetype desiredVisuals;
				set_temp_visuals(t.visuals, t.visualsStorage, desiredVisuals);
				visuals_loop();

				fCharObjectY = 180000.0f;
				PositionObject(iCharObj, ObjectPositionX(iCharObj), fCharObjectY, ObjectPositionZ(iCharObj));

				ccpObjTargetX = ObjectPositionX(iCharObj);
				ccpObjTargetY = ObjectPositionY(iCharObj);
				ccpObjTargetZ = ObjectPositionZ(iCharObj);
				ccpObjTargetAX = ObjectAngleX(iCharObj);
				ccpObjTargetAY = ObjectAngleY(iCharObj);
				ccpObjTargetAZ = ObjectAngleZ(iCharObj);

				t.editorfreeflight.mode = 1;
				t.editorfreeflight.c.y_f = fCharObjectY + 60;
				t.editorfreeflight.c.angx_f = 0;
				t.editorfreeflight.s = t.editorfreeflight.c;

				once_camera_adjust = true;

				//  "hide" all entities in map by moving them out the way
				for (t.tcce = 1; t.tcce <= g.entityelementlist; t.tcce++)
				{
					t.tccentid = t.entityelement[t.tcce].bankindex;
					if (t.tccentid > 0)
					{
						t.tccsourceobj = t.entityelement[t.tcce].obj;
						if (ObjectExist(t.tccsourceobj) == 1)
						{
							PositionObject(t.tccsourceobj, 0, 0, 0);
						}
					}
				}

				fCCPRotateY = ccpObjTargetAY = ObjectAngleY(iCharObj);
				fCCPRotateY -= 15.0f; //Turn it a bit.
				//PE: Make sure we are in slider range.
				if (fCCPRotateY < 0.0) fCCPRotateY += 360.0;
				if (fCCPRotateY > 360.0) fCCPRotateY -= 360.0;
				// refresh thumbnail

				ccpObjTargetAY = fCCPRotateY;

				charactercreatorplus_initcameratransitions();

				// Prevent user camera input.
				t.cameraviewmode = 9; //No mode.
				t.editorfreeflight.mode = 1;

				iDressRoom = g.characterkitobjectoffset + 16;
				int iDressRoomImage = g.charactercreatorEditorImageoffset + 121;

				cstr check = "";
				int charactertypeindex = 0;
				for (int i = 0; i < g_CharacterType.size(); i++)
				{
					if (i == 0)
						check = "adult male";
					else if (i == 1)
						check = "adult female";
					else if (i == 2)
						check = "zombie male";
					else if (i == 3)
						check = "zombie female";
					else
					{
						if (i < g_CharacterType.size())
						{
							check = g_CharacterType[i].pPartsFolder;
						}
					}
					if (stricmp(CCP_Type, check.Get()) == 0)
					{
						charactertypeindex = i;
						break;
					}
				}
				change_dress_room(charactertypeindex);
				SetObjectToCameraOrientation(iDressRoom);
				PositionObject(iDressRoom, ccpTargetX, fCharObjectY - g_fLockerRoomOffset, ccpTargetZ);
				dressroomTargetAY = ObjectAngleY(iCharObj);
				RotateObject(iDressRoom, oangx, dressroomTargetAY, oangz);
				MoveObject(iDressRoom, 150);
				ShowObject(iDressRoom);
				PositionObject(iDressRoom, ObjectPositionX(iDressRoom), fCharObjectY - g_fLockerRoomOffset, ObjectPositionZ(iDressRoom));		
				RotateObject(iCharObj, ObjectAngleX(iCharObj), fCCPRotateY, ObjectAngleZ(iCharObj));
			}
		
			//Display sky for better look.
			if (ObjectExist(t.terrain.objectstartindex + 4) == 1)
			{
				PositionObject(t.terrain.objectstartindex + 4, CameraPositionX(0), CameraPositionY(0), CameraPositionZ(0));
				SetAlphaMappingOn(t.terrain.objectstartindex + 4, 100.0*t.sky.alpha1_f);
				ShowObject(t.terrain.objectstartindex + 4);
			}

			if (iDelayExecute == 1)
			{
				//PE: Change type.
				image_preload_files_wait();
				object_preload_files_wait();
				image_preload_files_reset(); //PE: Free all not used images from prev. type.

				charactercreatorplus_refreshtype();
				iDelayExecute = 0;
				ShowObject(iCharObj);
			}
			
			// handle preparing of animation data
			if (g_bCharacterCreatorPrepAnims == true)
			{
				int iUseDefaultNonCombatAnimations = 1;  // Adult Male/Female by default
				if (stricmp(CCP_Type, "zombie male") == NULL) iUseDefaultNonCombatAnimations = 2;
				extern void animsystem_prepareobjectforanimtool(int objectnumber, int iUseDefaultNonCombatAnimations);
				animsystem_prepareobjectforanimtool(iCharObj, iUseDefaultNonCombatAnimations);
				g_bCharacterCreatorPrepAnims = false;
			}

			//Enable this to disable all movement ... when g_bCharacterCreatorPlusActivated
			extern int iGenralWindowsFlags;
			ImGui::Begin("Character Creator##PropertiesWindow", &g_bCharacterCreatorPlusActivated, iGenralWindowsFlags);

			if (once_camera_adjust)
			{
				extern ImVec2 OldrenderTargetSize;
				extern ImVec2 OldrenderTargetPos;
				extern ImVec2 renderTargetAreaSize;
				PositionCamera(t.editorfreeflight.c.x_f, t.editorfreeflight.c.y_f, t.editorfreeflight.c.z_f);
				RotateCamera(t.editorfreeflight.c.angx_f, t.editorfreeflight.c.angy_f, 0);

				float camxadjust = renderTargetAreaSize.x - (ImGui::GetWindowPos().x - OldrenderTargetPos.x);

				if (camxadjust > 100.0f && camxadjust < GetDisplayWidth()) {
					camxadjust -= 100.0;
					camxadjust *= 0.068;
					MoveCameraLeft(g_pGlob->dwCurrentSetCameraID, -camxadjust);
					t.editorfreeflight.c.x_f = CameraPositionX();
					t.editorfreeflight.c.z_f = CameraPositionZ();;
				}
				once_camera_adjust = false;
			}
			int media_icon_size = 64; //96
			float col_start = 80.0f;
			ImGui::PushItemWidth(ImGui::GetFontSize()*10.0);
			
			// ZJ: Made global so when user has auto close enabled, the selection is still known.
			static int ccp_part_selection = 5;
			static int item_current_room_selection = 0;

			extern int iLastOpenHeader;
  																			//if no other headers are open this should be.                                                                 
			if (pref.bAutoClosePropertySections && iLastOpenHeader != 60 && iLastOpenHeader >= 60 && iLastOpenHeader <= 71)
				ImGui::SetNextItemOpen(false, ImGuiCond_Always);

			if (ImGui::StyleCollapsingHeader("Name And Type", ImGuiTreeNodeFlags_DefaultOpen)) {

				iLastOpenHeader = 60;
				float w = ImGui::GetWindowContentRegionWidth();

				ImGui::Indent(10);
				ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 13)); //3
				ImGui::Text("Name");
				ImGui::SameLine();
				ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() - 3));
				ImGui::SetCursorPos(ImVec2(col_start, ImGui::GetCursorPosY()));

				ImGui::PushItemWidth(-10);
				ImGui::InputText("##NameCCP", &CCP_Name[0], 250);
				if (ImGui::MaxIsItemFocused()) bImGuiGotFocus = true;
				if (!pref.iTurnOffEditboxTooltip && ImGui::IsItemHovered()) ImGui::SetTooltip("Set Character Name");

				ImGui::PopItemWidth();

				// Character Type
				ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));
				ImGui::Text("Type");
				ImGui::SameLine();
				ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() - 3));
				ImGui::SetCursorPos(ImVec2(col_start, ImGui::GetCursorPosY()));
				static int g_chartypes_item_count = 0;
				static char** g_chartypes_items = NULL;
				if (g_chartypes_item_count != g_CharacterType.size())
				{
					if (g_chartypes_items)
					{
						for (int i = 0; i < g_chartypes_item_count; i++) SAFE_DELETE(g_chartypes_items[i]);
						SAFE_DELETE(g_chartypes_items);
					}
					g_chartypes_item_count = g_CharacterType.size();
					g_chartypes_items = new char* [g_chartypes_item_count];
					for (int i = 0; i < g_chartypes_item_count; i++)
					{
						g_chartypes_items[i] = new char[256];
						strcpy(g_chartypes_items[i], pCharacterTypeDropDownList[i]);
					}
				}
				for (int i = 0; i < g_chartypes_item_count; i++)
				{
					if (pestrcasestr(CCP_Type, g_chartypes_items[i]))
					{
						item_current_type_selection = i;
						break;
					}
				}
				ImGui::PushItemWidth(-10);
				if (ImGui::Combo("##TypeCCP", &item_current_type_selection, g_chartypes_items, g_chartypes_item_count))
				{
					strcpy(CCP_Type, g_chartypes_items[item_current_type_selection]);
					iThumbsOffsetY = 0;
					if (item_current_type_selection == 2 || item_current_type_selection == 3) iThumbsOffsetY = 50;
					iDelayExecute = 1;
					DisplaySmallImGuiMessage("Loading ...");
					change_dress_room(item_current_type_selection);
					charactercreatorplus_changecameratransition(5);
					ccp_part_selection = 5;
					SetObjectToCameraOrientation(iDressRoom);
					PositionObject(iDressRoom, ccpTargetX, fCharObjectY - g_fLockerRoomOffset, ccpTargetZ);
					RotateObject(iDressRoom, ObjectAngleX(iCharObj), dressroomTargetAY, ObjectAngleZ(iCharObj));
					MoveObject(iDressRoom, 150);
					ShowObject(iDressRoom);
					fCharObjectY = 180000.0f;
					PositionObject(iDressRoom, ObjectPositionX(iDressRoom), fCharObjectY - g_fLockerRoomOffset, ObjectPositionZ(iDressRoom));
					HideObject(iCharObj);
				}
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("Select Character Type");

				// Dressing Room
				ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));
				ImGui::Text("Room");
				ImGui::SameLine();
				ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() - 3));
				ImGui::SetCursorPos(ImVec2(col_start, ImGui::GetCursorPosY()));
				static int g_roomtypes_item_count = 0;
				static char** g_roomtypes_items = NULL;
				if (g_roomtypes_item_count != g_RoomType.size())
				{
					if (g_roomtypes_items)
					{
						for (int i = 0; i < g_roomtypes_item_count; i++) SAFE_DELETE(g_roomtypes_items[i]);
						SAFE_DELETE(g_roomtypes_items);
					}
					g_roomtypes_item_count = g_RoomType.size();
					g_roomtypes_items = new char* [g_roomtypes_item_count];
					for (int i = 0; i < g_roomtypes_item_count; i++)
					{
						g_roomtypes_items[i] = new char[256];
						strcpy(g_roomtypes_items[i], pRoomTypeDropDownList[i]);
					}
				}
				for (int i = 0; i < g_roomtypes_item_count; i++)
				{
					if (stricmp(CCP_Room, g_roomtypes_items[i])==NULL)
					{
						item_current_room_selection = i;
						break;
					}
				}
				ImGui::PushItemWidth(-10);
				if (ImGui::Combo("##RoomCCP", &item_current_room_selection, g_roomtypes_items, g_roomtypes_item_count))
				{
					strcpy(CCP_Room, g_roomtypes_items[item_current_room_selection]);
					iThumbsOffsetY = 0;
					iDelayExecute = 1;
					DisplaySmallImGuiMessage("Loading ...");
					change_dress_room(item_current_room_selection * -1);

					// change preferred room for this character type and save locally 
					g_CharacterTypeRoomPref[item_current_type_selection] = item_current_room_selection;
					char pRoomPrefFile[MAX_PATH];
					sprintf(pRoomPrefFile, "charactercreatorplus\\parts\\%s\\roompref.txt", CCP_Type);
					GG_SetWritablesToRoot(1);
					GG_GetRealPath(pRoomPrefFile, 1);
					if (FileExist(pRoomPrefFile)) DeleteFileA(pRoomPrefFile);
					OpenToWrite(1, pRoomPrefFile);
					WriteString(1, CCP_Room);
					CloseFile(1);
					GG_SetWritablesToRoot(0);

					charactercreatorplus_changecameratransition(5);
					SetObjectToCameraOrientation(iDressRoom);
					PositionObject(iDressRoom, ccpTargetX, fCharObjectY - g_fLockerRoomOffset, ccpTargetZ);
					RotateObject(iDressRoom, ObjectAngleX(iCharObj), dressroomTargetAY, ObjectAngleZ(iCharObj));
					MoveObject(iDressRoom, 150);
					ShowObject(iDressRoom);
					fCharObjectY = 180000.0f;
					PositionObject(iDressRoom, ObjectPositionX(iDressRoom), fCharObjectY - g_fLockerRoomOffset, ObjectPositionZ(iDressRoom));
					HideObject(iCharObj);
				}
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("Select Dressing Room");

				ImGui::PopItemWidth();
				ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 8)); //3
				ImGui::Indent(-10);
			}

			bool bZoom = false;

			if (pref.bAutoClosePropertySections && iLastOpenHeader != 61)
				ImGui::SetNextItemOpen(false, ImGuiCond_Always);

			if (ImGui::StyleCollapsingHeader("Customize", ImGuiTreeNodeFlags_DefaultOpen))
			{
				iLastOpenHeader = 61;

				static std::map<std::string, std::string> CharacterCreatorCurrent_s;
				static std::map<std::string, std::string> CharacterCreatorCurrentAnnotated_s;
				static std::map<std::string, std::string> CharacterCreatorCurrentAnnotatedTag_s;
				cstr field_name;
				char* combo_buffer = NULL;
				char* combo_annotated_buffer = NULL;
				int part_number = 0;

				int ccp_part_icons = 10;
				int ccp_part_icons_columns = 5;
				float entity_w = ImGui::GetContentRegionAvailWidth() - 10.0f;
				float fSpacer = 0.0f;
				//New icons.
				float ccp_part_image_size = entity_w / (float)ccp_part_icons_columns;
				ccp_part_image_size -= ((2.0f) * ccp_part_icons_columns) - 2.0f;
				
				//PE: CHECK CCP_ACCESSORY1
				int ccp_part_images[] = { CCP_HEAD, CCP_HAIR, CCP_BEARD, CCP_HAT, CCP_GLASSES, CCP_BODY, CCP_LEGS, CCP_FEET,CCP_ACCESSORY1,CCP_ACCESSORY2 };
				int ccp_part_order[] =  { 2       ,1        ,4         ,0       ,3           ,5        ,6        ,7		,8		,9 };
				cstr ccp_part_tooltip[] = {
					"Head",
					"Hair",
					"Facial Hair",
					"Head Gear",
					"Wearing",
					"Body",
					"Legs",
					"Feet",
					"Accessory1",
					"Accessory2"
				};

				if (ccp_part_selection < 0 || ccp_part_selection >= ccp_part_icons) ccp_part_selection = 0;

				if (strstr(CCP_Type, "Adult Female"))
				{
					ccp_part_tooltip[2] = "Tattoo";
					ccp_part_images[2] = CCP_TATTOO;
				}

				ImVec4 IconColor = ImVec4(1.0, 1.0, 1.0, 1.0);
				ImGui::Indent(4);
				for (int i = 0; i < ccp_part_icons; i++)
				{
					ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(fSpacer, 0.0f));
					if (ccp_part_selection == i)
					{
						ImVec4 tool_selected_col = ImGui::GetStyle().Colors[ImGuiCol_PlotHistogram];
						ImGuiWindow* window = ImGui::GetCurrentWindow();
						ImVec2 padding = { 3.0, 3.0 };
						const ImRect image_bb((window->DC.CursorPos - padding), window->DC.CursorPos + padding + ImVec2(ccp_part_image_size, ccp_part_image_size));
						window->DrawList->AddRect(image_bb.Min, image_bb.Max, ImGui::GetColorU32(tool_selected_col), 0.0f, 15, 2.0f);
					}
					if (ImGui::ImgBtn(ccp_part_images[i], ImVec2(ccp_part_image_size, ccp_part_image_size), ImVec4(0.0, 0.0, 0.0, 0.0), IconColor, ImVec4(0.8, 0.8, 0.8, 0.8), ImVec4(0.8, 0.8, 0.8, 0.8), 0, 0, 0, 0, false, false, false, false, false, false)) //no bBoostIconColors
					{
						ccp_part_selection = i;

						charactercreatorplus_changecameratransition(ccp_part_selection);

						switch (ccp_part_selection)
						{
						case 0: charactercreatorplus_restrictpart(2);
							break;
						case 1: charactercreatorplus_restrictpart(1);
							break;
						case 2: charactercreatorplus_restrictpart(4);
							break;
						case 3: charactercreatorplus_restrictpart(0);
							break;
						case 4: charactercreatorplus_restrictpart(3);
							break;
						}
					}
					if (ImGui::IsItemHovered() ) ImGui::SetTooltip(ccp_part_tooltip[i].Get());

					ImVec2 restore_cursorpos = ImGui::GetCursorPos();
					if ((i + 1) % ccp_part_icons_columns != 0 && i != ccp_part_icons - 1)
						ImGui::SameLine();
				}
				ImGui::Indent(-4);

				//PE: We dont have the same order as the old, so convert orders.
				int part_loop = ccp_part_order[ccp_part_selection];
				part_number = part_loop;

				if (part_loop == 0)
				{
					CharacterCreatorCurrent_s = CharacterCreatorHeadGear_s;
					CharacterCreatorCurrentAnnotated_s = CharacterCreatorAnnotatedHeadGear_s;
					CharacterCreatorCurrentAnnotatedTag_s = CharacterCreatorAnnotatedTagHeadGear_s;
					field_name = "Head Gear";
					LPSTR pAnnotatedLabel = "None";
					if (strnicmp(cSelectedHeadGear, "None", 4) != NULL) pAnnotatedLabel = charactercreatorplus_findannotation(cSelectedHeadGear);
					combo_buffer = cSelectedHeadGear;
					combo_annotated_buffer = pAnnotatedLabel;
					part_number = part_loop;
				}
				if (part_loop == 1)
				{
					CharacterCreatorCurrent_s = CharacterCreatorHair_s;
					CharacterCreatorCurrentAnnotated_s = CharacterCreatorAnnotatedHair_s;
					CharacterCreatorCurrentAnnotatedTag_s = CharacterCreatorAnnotatedTagHair_s;
					field_name = "Hair";
					LPSTR pAnnotatedLabel = "None";
					if (strnicmp(cSelectedHair, "None", 4) != NULL) pAnnotatedLabel = charactercreatorplus_findannotation(cSelectedHair);
					combo_buffer = cSelectedHair;
					combo_annotated_buffer = pAnnotatedLabel;
					part_number = part_loop;
				}
				if (part_loop == 2)
				{
					CharacterCreatorCurrent_s = CharacterCreatorHead_s;
					CharacterCreatorCurrentAnnotated_s = CharacterCreatorAnnotatedHead_s;
					CharacterCreatorCurrentAnnotatedTag_s = CharacterCreatorAnnotatedTagHead_s;
					field_name = "Head";
					LPSTR pAnnotatedLabel = charactercreatorplus_findannotation(cSelectedHead);
					combo_buffer = cSelectedHead;
					combo_annotated_buffer = pAnnotatedLabel;
					part_number = part_loop;
				}
				if (part_loop == 3)
				{
					CharacterCreatorCurrent_s = CharacterCreatorEyeglasses_s;
					CharacterCreatorCurrentAnnotated_s = CharacterCreatorAnnotatedEyeglasses_s;
					CharacterCreatorCurrentAnnotatedTag_s = CharacterCreatorAnnotatedTagEyeglasses_s;
					field_name = "Wearing";
					LPSTR pAnnotatedLabel = "None";
					if (strnicmp(cSelectedEyeglasses, "None", 4) != NULL) pAnnotatedLabel = charactercreatorplus_findannotation(cSelectedEyeglasses);
					combo_buffer = cSelectedEyeglasses;
					combo_annotated_buffer = pAnnotatedLabel;
					part_number = part_loop;
				}

				if (part_loop == 8)
				{
					CharacterCreatorCurrent_s = CharacterCreatorAccessory1_s;
					CharacterCreatorCurrentAnnotated_s = CharacterCreatorAnnotatedAccessory1_s;
					CharacterCreatorCurrentAnnotatedTag_s = CharacterCreatorAnnotatedTagAccessory1_s;
					field_name = "Accessory";
					LPSTR pAnnotatedLabel = "None";
					if (strnicmp(cSelectedAccessory1, "None", 4) != NULL) pAnnotatedLabel = charactercreatorplus_findannotation(cSelectedAccessory1);
					combo_buffer = cSelectedAccessory1;
					combo_annotated_buffer = pAnnotatedLabel;
					part_number = part_loop;
				}
				if (part_loop == 9)
				{
					CharacterCreatorCurrent_s = CharacterCreatorAccessory2_s;
					CharacterCreatorCurrentAnnotated_s = CharacterCreatorAnnotatedAccessory2_s;
					CharacterCreatorCurrentAnnotatedTag_s = CharacterCreatorAnnotatedTagAccessory2_s;
					field_name = "Accessory";
					LPSTR pAnnotatedLabel = "None";
					if (strnicmp(cSelectedAccessory2, "None", 4) != NULL) pAnnotatedLabel = charactercreatorplus_findannotation(cSelectedAccessory2);
					combo_buffer = cSelectedAccessory2;
					combo_annotated_buffer = pAnnotatedLabel;
					part_number = part_loop;
				}

				if (part_loop == 4)
				{
					CharacterCreatorCurrent_s = CharacterCreatorFacialHair_s;
					CharacterCreatorCurrentAnnotated_s = CharacterCreatorAnnotatedFacialHair_s;
					CharacterCreatorCurrentAnnotatedTag_s = CharacterCreatorAnnotatedTagFacialHair_s;
					if (strstr(CCP_Type, "Adult Female"))
						field_name = "Tattoo";
					else
					field_name = "Facial Hair";
					LPSTR pAnnotatedLabel = "None";
					if (strnicmp(cSelectedFacialHair, "None", 4) != NULL) pAnnotatedLabel = charactercreatorplus_findannotation(cSelectedFacialHair);
					combo_buffer = cSelectedFacialHair;
					combo_annotated_buffer = pAnnotatedLabel;
					part_number = part_loop;
				}
				if (part_loop == 5)
				{
					CharacterCreatorCurrent_s = CharacterCreatorBody_s;
					CharacterCreatorCurrentAnnotated_s = CharacterCreatorAnnotatedBody_s;
					CharacterCreatorCurrentAnnotatedTag_s = CharacterCreatorAnnotatedTagBody_s;
					field_name = "Body";
					LPSTR pAnnotatedLabel = charactercreatorplus_findannotation(cSelectedBody);
					combo_buffer = cSelectedBody;
					combo_annotated_buffer = pAnnotatedLabel;
					part_number = part_loop;
				}
				if (part_loop == 6)
				{
					CharacterCreatorCurrent_s = CharacterCreatorLegs_s;
					CharacterCreatorCurrentAnnotated_s = CharacterCreatorAnnotatedLegs_s;
					CharacterCreatorCurrentAnnotatedTag_s = CharacterCreatorAnnotatedTagLegs_s;
					field_name = "Legs";
					LPSTR pAnnotatedLabel = charactercreatorplus_findannotation(cSelectedLegs);
					bool bAllow = false;
					// before allowng selected legs through, check they comply with our cSelectedLegsFilter filter
					if (strlen(cSelectedLegsFilter) == 0)
					{
						if (item_current_type_selection <= 3 && strnicmp(cSelectedLegs + strlen(cSelectedLegs) - 2, "01", 2) != NULL) bAllow = true;
						if (item_current_type_selection > 3) bAllow = true;
					}
					if (strlen(cSelectedLegsFilter) > 0 && pAnnotatedLabel && strstr(pAnnotatedLabel, cSelectedLegsFilter) != NULL) bAllow = true;
					if (bAllow == true)
					{
						// no filter so allow, or filter matches, so also allow
					}
					else
					{
						// this current legs selection no longer matches filter, so change to one that does
						// starting with the top-most item and working down
						std::map<std::string, std::string>::iterator annotated = CharacterCreatorCurrentAnnotated_s.begin();
						for (std::map<std::string, std::string>::iterator it = CharacterCreatorCurrent_s.begin(); it != CharacterCreatorCurrent_s.end(); ++it)
						{
							std::string thisname = it->first;
							std::string thistag = annotated->second;
							bool bThisAllow = false;
							LPSTR pThisName = (char*)thisname.c_str();
							if (strlen(cSelectedLegsFilter) == 0)
							{
								if (item_current_type_selection <= 3 && strnicmp(pThisName + strlen(pThisName) - 2, "01", 2) != NULL) bThisAllow = true;
								if (item_current_type_selection > 3) bAllow = true;
							}
							if (strlen(cSelectedLegsFilter) > 0 && strstr(thistag.c_str(), cSelectedLegsFilter) != NULL) bThisAllow = true;
							if (bThisAllow == true)
							{
								// found first (or one matching the filter)
								strcpy(cSelectedLegs, thisname.c_str());
								strcpy(cSelectedFeetFilter, "");
								g_bLegsChangeCascade = true;
								g_bFeetChangeCascade = true;
								break;
							}
							annotated++;
						}
						pAnnotatedLabel = charactercreatorplus_findannotation(cSelectedLegs);
					}
					// continue with selected legs as normal now
					combo_buffer = cSelectedLegs;
					combo_annotated_buffer = pAnnotatedLabel;
					part_number = part_loop;
				}
				if (part_loop == 7)
				{
					CharacterCreatorCurrent_s = CharacterCreatorFeet_s;
					CharacterCreatorCurrentAnnotated_s = CharacterCreatorAnnotatedFeet_s;
					CharacterCreatorCurrentAnnotatedTag_s = CharacterCreatorAnnotatedTagFeet_s;
					field_name = "Feet";
					// before allowng selected feet through, check they comply with our filter
					bool bAllow = false;
					LPSTR pAnnotatedLabel = charactercreatorplus_findannotation(cSelectedFeet);
					LPSTR pAnnotatedLabelTag = charactercreatorplus_findannotationtag(cSelectedFeet);
					if (strlen(cSelectedFeetFilter) == 0 && (pAnnotatedLabelTag == NULL || strlen(pAnnotatedLabelTag) == 0)) bAllow = true;
					if (strlen(cSelectedFeetFilter) > 0 && pAnnotatedLabelTag && strstr(pAnnotatedLabelTag, cSelectedFeetFilter) != NULL) bAllow = true;
					if (bAllow == true)
					{
						// no filter so allow, or filter matches, so also allow
					}
					else
					{
						// this current feet selection no longer matches filter, so change to one that does
						// starting with the top-most item and working down
						std::map<std::string, std::string>::iterator annotatedtag = CharacterCreatorCurrentAnnotatedTag_s.begin();
						for (std::map<std::string, std::string>::iterator it = CharacterCreatorCurrent_s.begin(); it != CharacterCreatorCurrent_s.end(); ++it)
						{
							std::string thisname = it->first;
							std::string thistag = annotatedtag->second;
							bool bThisAllow = false;
							LPSTR pThisName = (char*)thisname.c_str();
							if (strlen(cSelectedFeetFilter) == 0 && strlen(thistag.c_str()) == 0) bThisAllow = true;
							if (strlen(cSelectedFeetFilter) > 0 && strstr(thistag.c_str(), cSelectedFeetFilter) != NULL) bThisAllow = true;
							if (bThisAllow == true)
							{
								// found first (or one matching the filter)
								strcpy(cSelectedFeet, thisname.c_str());
								g_bFeetChangeCascade = true;
								break;
							}
							annotatedtag++;
						}
						pAnnotatedLabel = charactercreatorplus_findannotation(cSelectedFeet);
					}
					combo_buffer = cSelectedFeet;
					combo_annotated_buffer = pAnnotatedLabel;
					part_number = part_loop;
				}
				if (!CharacterCreatorCurrent_s.empty() && CharacterCreatorCurrent_s.size() > 1)
				{

					ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));
					ImGui::TextCenter(field_name.Get());
					ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() - 3));

					float Color_gadget_size = ImGui::GetFontSize()*2.0;


					cstr unique_label = "##CCP";
					unique_label += field_name;

					int iUniqueId = 444444;

					ImGui::Indent(1);
					ImGui::PushItemWidth(-10);
					ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
					ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f, 6.5f));

					static float fCWidth = 0.0;
					float partsheight;

					int rows = CharacterCreatorCurrent_s.size() / 5;
					if (rows * 4 != CharacterCreatorCurrent_s.size())
						rows += 1;
					if (rows > 3) rows = 3;

					partsheight = 60.0f * rows;
					if(rows == 1)
						partsheight = ImGui::GetContentRegionAvail().y / 9.5f * rows;
					else
						partsheight = ImGui::GetContentRegionAvail().y / 10.5f * rows;

					ImGui::BeginChild("##CCP-Parts-Child", ImVec2(ImGui::GetContentRegionAvail().x, partsheight), false, iGenralWindowsFlags | ImGuiWindowFlags_AlwaysVerticalScrollbar);
					ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));

					ImGui::Columns(4, "ccp-parts-columns4", false);  //false no border

					static int iCountUniqueImageIds = CCP_THUMBS;
					int iCountIcons = 0;

					std::map<std::string, std::string>::iterator annotated = CharacterCreatorCurrentAnnotated_s.begin();
					std::map<std::string, std::string>::iterator annotatedtag = CharacterCreatorCurrentAnnotatedTag_s.begin();
					for (std::map<std::string, std::string>::iterator it = CharacterCreatorCurrent_s.begin(); it != CharacterCreatorCurrent_s.end(); ++it)
					{
						std::string full_path = it->second;
						std::string name = it->first;

						// only allow if part has no filter or filter within name
						bool bThisAllow = false;
						if ( part_number == 6 || part_number == 7 )
						{
							LPSTR pThisName = (char*)name.c_str();
							if (part_number == 6)
							{
								// only allow specific legs
								LPSTR pThisAnnotatedName = (char*)annotated->second.c_str();
								if (strlen(cSelectedLegsFilter) == 0)
								{
									if (item_current_type_selection <= 3 && strnicmp(pThisName + strlen(pThisName) - 2, "01", 2) != NULL) bThisAllow = true;
									if (item_current_type_selection > 3) bThisAllow = true;
								}
								if (strlen(cSelectedLegsFilter) > 0 && strstr(pThisAnnotatedName, cSelectedLegsFilter) != NULL) bThisAllow = true;
							}
							if (part_number == 7)
							{
								// only allow specific feet
								LPSTR pThisAnnotatedTagName = (char*)annotatedtag->second.c_str();
								if (strlen(cSelectedFeetFilter) == 0 && (strlen(pThisAnnotatedTagName) == 0)) bThisAllow = true;
								if (strlen(cSelectedFeetFilter) > 0 && strstr(pThisAnnotatedTagName, cSelectedFeetFilter) != NULL) bThisAllow = true;
							}
						}
						else
						{
							// all other parts have a free pass!
							bThisAllow = true;
						}
						if (bThisAllow == true)
						{
							int iTextureID = CCP_EMPTY;
							if (iCountIcons < MAXPARTICONS)
							{
								if (g_iPartsIconsIDs[item_current_type_selection][part_number][iCountIcons] > 0)
								{
									iTextureID = g_iPartsIconsIDs[item_current_type_selection][part_number][iCountIcons];
								}
								else if (g_iPartsIconsIDs[item_current_type_selection][part_number][iCountIcons] == -1)
								{
									bool bValid = false;
									if (item_current_type_selection == 0 && pestrcasestr(full_path.c_str(), "adult male")) bValid = true;
									if (item_current_type_selection == 1 && pestrcasestr(full_path.c_str(), "adult female")) bValid = true;
									if (item_current_type_selection == 2 && pestrcasestr(full_path.c_str(), "zombie male")) bValid = true;
									if (item_current_type_selection == 3 && pestrcasestr(full_path.c_str(), "zombie female")) bValid = true;
									if (item_current_type_selection > 3 && pestrcasestr(full_path.c_str(), g_CharacterType[item_current_type_selection].pPartsFolder)) bValid = true;
									if (bValid)
									{
										//Try loading icon.
										std::string icon_path = full_path + name + ".png";
										image_setlegacyimageloading(true);
										LoadImage((char *)icon_path.c_str(), iCountUniqueImageIds);
										image_setlegacyimageloading(false);
										if (ImageExist(iCountUniqueImageIds))
										{
											g_iPartsIconsIDs[item_current_type_selection][part_number][iCountIcons] = iCountUniqueImageIds++;
										}
										else
										{
											g_iPartsIconsIDs[item_current_type_selection][part_number][iCountIcons] = 0;
										}
									}
								}
							}
							if (strnicmp(name.c_str(), "None", 4) == NULL)
								iTextureID = CCP_NONE;

							// mark the one selected
							bool is_selected = false;
							if (strcmp(name.c_str(), combo_buffer) == 0)
								is_selected = true;

							// the label we see
							std::string annotated_label = annotated->second;
							std::string annotatedtag_label = annotatedtag->second;

							bool bIconRestricted = false;

							// hide the icon for any restricted part (part that doesn't work with another part).
							for (int i = 0; i < g_restrictedParts.size(); i++)
							{
								if (strstr(annotated->second.c_str(), g_restrictedParts[i]))
								{
									bIconRestricted = true;
									break;
								}
							}
							// also hide any parts that are missing their thumbnail 
							if (iTextureID == CCP_EMPTY)
							{
								bIconRestricted = true;
							}

							if (!bIconRestricted)
							{
								ImGui::PushID(iUniqueId++);

								fCWidth = ImGui::GetContentRegionAvailWidth();
								ImGuiWindow* window = ImGui::GetCurrentWindow();
								//Add background rect.
								ImVec4 background_col = ImGui::GetStyle().Colors[ImGuiCol_Button]; //ImVec4(0.5, 0.5, 0.5, 0.6);
								background_col.w = 0.60;
								const ImRect image_bb((window->DC.CursorPos), window->DC.CursorPos + ImVec2(fCWidth, fCWidth));
								window->DrawList->AddRectFilled(image_bb.Min, image_bb.Max, ImGui::GetColorU32(background_col), 8.0f, 15);

								if (is_selected)
								{
									ImVec4 tool_selected_col = ImGui::GetStyle().Colors[ImGuiCol_PlotHistogram];
									ImVec2 padding = { 3.0, 3.0 };
									const ImRect image_bb((window->DC.CursorPos - padding), window->DC.CursorPos + padding + ImVec2(fCWidth, fCWidth));
									window->DrawList->AddRect(image_bb.Min, image_bb.Max, ImGui::GetColorU32(tool_selected_col), 0.0f, 15, 2.0f);
								}

								IconColor = ImVec4(1.0, 1.0, 1.0, 1.0);
								ImVec4 HoverColor = ImVec4(0.8f, 0.8f, 0.8f, 0.8f);

								if (iTextureID == CCP_NONE || iTextureID == CCP_EMPTY)
									IconColor.w = 0.5;

								if (iTextureID > 0 && ImGui::ImgBtn(iTextureID, ImVec2(fCWidth, fCWidth), ImVec4(0.0, 0.0, 0.0, 0.0), IconColor, HoverColor, HoverColor, 0, 0, 0, 0, false, false, false, false, true, false)) //no bBoostIconColors
								{
									// we need to wait for any previously requested preloads to exist before we can go on to make the character
									object_preload_files_wait(); // dont need to wait for image preload, image preload is thread safe and can overlap normal DX operations
									// Change Character. full_path.c_str()
									strcpy(combo_buffer, name.c_str());
									// instead of instant change, record change we want and fire off some preloads for smooth UI
									charactercreatorplus_preparechange((char *)full_path.c_str(), part_number, (char *)annotatedtag_label.c_str());
									if (part_number == 2)
									{
										// and set the IC for reference when its time to save the character assembly info
										strcpy(cSelectedICCode, (char *)annotatedtag_label.c_str());
									}
									if (part_number == 5)
									{
										// if body requires NO LEGS (or something else), set this condition for other dropdowns
										// and force any current legs to conform
										strcpy(cSelectedLegsFilter, (char *)annotatedtag_label.c_str());
									}
									if (part_number == 6)
									{
										// if legs requires NO FEET (or something else), set this condition for other dropdowns
										// and force any current feet to conform
										strcpy(cSelectedFeetFilter, (char *)annotatedtag_label.c_str());
									}
								}
								if (ImGui::IsItemHovered())
								{
									// Display an enlarged image of the icon, if the user is hovering over it.
									ImGui::BeginTooltip();
									ImVec4 background_col = ImGui::GetStyle().Colors[ImGuiCol_Button]; //ImVec4(0.5, 0.5, 0.5, 0.6);
									background_col.w = 0.6f;
									//ImGui::ImgBtn(iTextureID, ImVec2(fCWidth * 2.5f, fCWidth * 2.5f), background_col, IconColor, HoverColor, HoverColor, 0, 0, 0, 0, false, false, false, false, true, false);
									ImGui::ImgBtn(iTextureID, ImVec2(fCWidth * 3.0f, fCWidth * 3.0f), background_col, IconColor, HoverColor, HoverColor, 0, 0, 0, 0, false, false, false, false, true, false);
									ImGui::TextCenter(annotated_label.c_str());
									ImGui::EndTooltip();
								}

								ImGui::PopID();

								ImGui::NextColumn();
							}
						}

						// advance annotated list with real item list
						annotated++;
						annotatedtag++;
						iCountIcons++;
					}

					ImGui::Columns(1);
					if (ImGui::GetCurrentWindow()->ScrollbarSizes.x > 0) {
						// ZJ: Following feedback, the two line gap was removed.
						ImGui::Spacing();

					}
					ImGui::EndChild();

					ImGui::PopStyleVar();
					ImGui::PopStyleVar();
					ImGui::PopItemWidth();
					ImGui::Indent(-1);

					// also update character during a cascade (body changes legs, which changes feet)
					if ((g_bLegsChangeCascade == true || g_bFeetChangeCascade == true) && g_charactercreatorplus_preloading == false)
					{
						// and ensure all thread activity ends before we push this (or it may delay long enough to change base type again!)
						image_preload_files_wait();
						object_preload_files_wait();

						// a faster single pass option
						std::map<std::string, std::string>::iterator it = CharacterCreatorCurrent_s.begin();
						std::string full_path = it->second;
						charactercreatorplus_preparechange((char*)full_path.c_str(), 67, "");
						g_bLegsChangeCascade = false;
						g_bFeetChangeCascade = false;
					}
					CharacterCreatorCurrent_s.clear();
				}


				//	Rotate the character.
				ImGui::TextCenter("Rotate");
				ImGui::Indent(10.0f);
				if (ImGui::MaxSliderInputFloat("##CharacterRotation", &fCCPRotateY, 0.0f, 360.0f, "Rotate Character", 0.0f, 360.0f))
				{
					RotateObject(iCharObj, ObjectAngleX(iCharObj), fCCPRotateY, ObjectAngleZ(iCharObj));
					ccpObjTargetAY = fCCPRotateY;
				}
				ImGui::Indent(-10.0f);

				//	Zoom in/out to character.
				ImGui::TextCenter("Zoom");
				ImGui::Indent(10.0f);
				if (ImGui::MaxSliderInputFloat("##CCPZoom", &g_fCCPZoom, 0.0f, 100.0f, "Adjust Zoom"))
				{
					charactercreatorplus_dozoom();

					bZoom = true;
				}
				ImGui::Indent(-10.0f);

			}

			extern bool bImGuiRenderTargetFocus;
			if (bImGuiRenderTargetFocus && ImGui::IsMouseDown(0) && ImGui::IsMouseDragging(0) && !bZoom)
			{
				//PE: Some object rotate inverted Pivot ?
				fCCPRotateY -= ImGui::GetIO().MouseDelta.x * g.timeelapsed_f;
				if (fCCPRotateY < 0) fCCPRotateY += 360.0;
				if (fCCPRotateY > 360.0) fCCPRotateY -= 360.0;
				RotateObject(iCharObj, ObjectAngleX(iCharObj), fCCPRotateY, ObjectAngleZ(iCharObj));
				ccpObjTargetAY = fCCPRotateY;
			}

			if (!pref.iEnableAdvancedCharacterCreator)
			{
				extern void animsystem_animationtoolsimpleui(int objectnumber);
				animsystem_animationtoolsimpleui(iCharObj);
			}
			else
			{
				extern void animsystem_animationtoolui(int objectnumber);
				animsystem_animationtoolui(iCharObj);
			}

			// All users can specify weapon for character
			if (pref.bAutoClosePropertySections && iLastOpenHeader != 62)
				ImGui::SetNextItemOpen(false, ImGuiCond_Always);

			if (ImGui::StyleCollapsingHeader("Character Details", ImGuiTreeNodeFlags_DefaultOpen))
			{
				iLastOpenHeader = 62;
				ImGui::Indent(10);

				// Weapon Choice
				extern void animsystem_weaponproperty (int, bool, entityeleproftype*, bool, bool);
				int characterbasetype = -1;
				if (stricmp(CCP_Type, "adult male") == NULL) characterbasetype = 0;
				if (stricmp(CCP_Type, "adult female") == NULL) characterbasetype = 1;
				if (stricmp(CCP_Type, "zombie male") == NULL) characterbasetype = 2;
				if (stricmp(CCP_Type, "zombie female") == NULL) characterbasetype = 3;
				if (characterbasetype == -1)
				{
					int iBaseValue = GetBaseValueFromCCPType(CCP_Type);
					if (iBaseValue > 1) characterbasetype = iBaseValue-1;
				}
				if (characterbasetype >= 0 && characterbasetype <= 1)
				{
					animsystem_weaponproperty(characterbasetype, false, &g_grideleprof_holdchoices, true, true);

					// Choose behavior.
					ImGui::TextCenter("Behavior");
					static std::vector<std::string> characterBehaviors;
					static std::vector<std::string> characterBehaviorsDisplay;
					static bool bCollectedScriptNames = false;
					if (!bCollectedScriptNames)
					{
						cstr oldDir = GetDir();
						char newDir[260];
						strcpy(newDir, "scriptbank\\people\\");
						SetDir(newDir);
						ChecklistForFiles();
						for (int c = 1; c <= ChecklistQuantity(); c++)
						{
							cStr tfile_s = Lower(ChecklistString(c));
							if (tfile_s != "." && tfile_s != "..")
							{
								if (strcmp(Right(tfile_s.Get(), 4), ".lua") == 0)
								{
									std::string file(tfile_s.Get());
									characterBehaviors.push_back(file);
									std::string name = file;
									replaceAll(name, "_", " ");
									*(name.data() + name.length() - 4) = 0;
									name[0] = toupper(name[0]);
									for (int i = 1; i < name.length() - 1; i++)
									{
										if (name[i] == ' ')
										{
											name[i + 1] = toupper(name[i + 1]);
										}
									}
									characterBehaviorsDisplay.push_back(name);
								}
							}
						}
						bCollectedScriptNames = true;
						SetDir(oldDir.Get());
					}

					ImGui::PushItemWidth(-10);
					std::string displayName = CCP_Script;
					replaceAll(displayName, "_", " ");
					*(displayName.data() + displayName.length() - 4) = 0;
					displayName[0] = toupper(displayName[0]);
					for (int i = 1; i < displayName.length() - 1; i++)
					{
						if (displayName[i] == ' ')
						{
							displayName[i + 1] = toupper(displayName[i + 1]);
						}
					}
					if (ImGui::BeginCombo("##characterbehaviorcombo", displayName.c_str()))
					{
						for (int i = 0; i < characterBehaviorsDisplay.size(); i++)
						{
							bool bIsSelected = strcmp(CCP_Script, characterBehaviors[i].c_str()) == 0;
							if (ImGui::Selectable(characterBehaviorsDisplay.at(i).c_str(), &bIsSelected))
							{
								strcpy(CCP_Script, characterBehaviors[i].c_str());
							}
						}
						ImGui::EndCombo();
					}
					ImGui::PopItemWidth();
				}

				// CharacterDetails VOICE only in Advanced
				if (pref.iEnableAdvancedCharacterCreator)
				{
					// Voice field removed from CCP.
				}

				//unindent before center.
				ImGui::Indent(-10); 
			}

			if (pref.bAutoClosePropertySections && iLastOpenHeader != 63)
				ImGui::SetNextItemOpen(false, ImGuiCond_Always);

			if (ImGui::StyleCollapsingHeader("Save Character", ImGuiTreeNodeFlags_DefaultOpen)) 
			{
				iLastOpenHeader = 63;
				ImGui::Indent(10);

				ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));
				ImGui::Text("Path");
				ImGui::SameLine();
				ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() - 3));

				ImGui::SetCursorPos(ImVec2(col_start, ImGui::GetCursorPosY()));

				float path_gadget_size = ImGui::GetFontSize()*2.0;

				ImGui::PushItemWidth(-10 - path_gadget_size);

				ImGui::InputText("##InputPathCCP", &CCP_Path[0], 250);
				if (ImGui::MaxIsItemFocused()) bImGuiGotFocus = true;
				if (!pref.iTurnOffEditboxTooltip && ImGui::IsItemHovered()) ImGui::SetTooltip("Set Where to Save Your Character");

				ImGui::PopItemWidth();
				//	Let the user know they set an invalid save file path.
				if (ImGui::BeginPopup("##CCPInvalidSavePath"))
				{
					ImGui::Text("Path must be within 'Max\\Files\\entitybank\\user\\'");
					ImGui::EndPopup();
				}

				ImGui::SameLine();
				ImGui::PushItemWidth(path_gadget_size);
				if (ImGui::StyleButton("...##ccppath")) {
					//PE: filedialogs change dir so.
					cStr tOldDir = GetDir();
					char * cFileSelected;
					cstr fulldir = tOldDir + "\\entitybank\\user\\"; //"\\entitybank\\user\\charactercreatorplus\\";
					cFileSelected = (char *)noc_file_dialog_open(NOC_FILE_DIALOG_DIR, "All\0*.*\0", fulldir.Get(), "", true, NULL);

					SetDir(tOldDir.Get());

					if (cFileSelected && strlen(cFileSelected) > 0) {

						//	Check that the new path still contains the entitybank folder.
						char* cCropped = strstr(cFileSelected, "\\entitybank\\user");
						if (cCropped)
						{
							//	New location contains entitybank folder, so change the import path.
							strcpy(CCP_Path, cFileSelected);
						}
						else
						{
							ImGui::OpenPopup("##CCPInvalidSavePath");
						}
					}
				}
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("Select Where to Save Your Character");

				ImGui::PopItemWidth();

				ImGui::Indent(-10); //unindent before center.
				float save_gadget_size = ImGui::GetFontSize()*10.0;
				float w = ImGui::GetWindowContentRegionWidth();
				ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((w*0.5) - (save_gadget_size*0.5), 0.0f));

				if (ImGui::StyleButton("Save Character##butsave", ImVec2(save_gadget_size, 0)))
				{
					// if free trial, no import
					extern bool g_bFreeTrialVersion;
					if (g_bFreeTrialVersion == true)
					{
						extern bool bFreeTrial_Window;
						bFreeTrial_Window = true;
					}
					else
					{
						if (strlen(CCP_Name) > 0)
						{
							if (strlen(CCP_Path) > 0)
							{
								// choose behavior here
								char script[260];
								strcpy(script, "people\\");
								strcat(script, CCP_Script);

								// save character FPE
								g_CharacterCreatorPlus.obj.settings.script_s = script;
								g_CharacterCreatorPlus.obj.settings.voice_s = pCCPVoiceSet;
								g_CharacterCreatorPlus.obj.settings.iSpeakRate = CCP_Speak_Rate;
								int iCharObj = g.characterkitobjectoffset + 1;

								// Character names saved starting with a space don't save correctly. Wipe out spaces before characters.
								int iSpaceBeforeCharCount = 0;
								for (int i = 0; i < strlen(CCP_Name); i++)
								{
									if (CCP_Name[i] == ' ')
										iSpaceBeforeCharCount++;
									else
										break;
								}
								if (iSpaceBeforeCharCount > 0)
								{
									char temp[MAX_PATH];
									strcpy(temp, CCP_Name + iSpaceBeforeCharCount);
									CCP_Name[0] = 0;
									strcpy(CCP_Name, temp);
								}

								cstr pFillFilename = cstr(CCP_Path) + CCP_Name + ".dbo";
								if (charactercreatorplus_savecharacterentity(iCharObj, pFillFilename.Get(), g.importermenuimageoffset + 50) == true)
								{
									strcpy(cTriggerMessage, "Character Saved");
									bTriggerMessage = true;

									extern cstr sGotoPreviewWithFile;
									extern int iGotoPreviewType;
									sGotoPreviewWithFile = cstr(CCP_Path) + CCP_Name + ".fpe";
									char sTmp[MAX_PATH];
									strcpy(sTmp, sGotoPreviewWithFile.Get());
									char *find = (char *)pestrcasestr(sTmp, "entitybank\\");
									if (find && find != &sTmp[0]) strcpy(sTmp, find);
									sGotoPreviewWithFile = sTmp;
									//Only trigger if destination contain entitybank.
									if (find)
									{
										//Exit ccp. and open preview.
										g_bCharacterCreatorPlusActivated = false;
										iGotoPreviewType = 1;
										CCP_Name[0] = 0;
									}
									else
										sGotoPreviewWithFile = "";
								}
							}
							else
							{
								strcpy(cTriggerMessage, "Please select a path where you like the character saved.");
								bTriggerMessage = true;
							}
						}
						else
						{
							strcpy(cTriggerMessage, "You must give your character a name before you can save it.");
							bTriggerMessage = true;
						}
					}
				}
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("Save Your Character");
			}

			if (!pref.bHideTutorials)
			{
				#ifndef REMOVED_EARLYACCESS
				if (ImGui::StyleCollapsingHeader("Tutorial (this feature is incomplete)", ImGuiTreeNodeFlags_DefaultOpen))
				{
					ImGui::Indent(10);
					void SmallTutorialVideo(char *tutorial, char* combo_items[] = NULL, int combo_entries = 0, int iVideoSection = 0, bool bAutoStart = false);
					cstr cShowTutorial = "03 - Add character and set a path";
					char* tutorial_combo_items[] = { "01 - Getting started", "02 - Creating terrain", "03 - Add character and set a path" };
					SmallTutorialVideo(cShowTutorial.Get(), tutorial_combo_items, ARRAYSIZE(tutorial_combo_items), SECTION_CHARACTER_CREATOR);
					float but_gadget_size = ImGui::GetFontSize()*12.0;
					float w = ImGui::GetWindowContentRegionWidth() - 10.0;
					ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((w*0.5) - (but_gadget_size*0.5), 0.0f));
					#ifdef INCLUDESTEPBYSTEP
					if (ImGui::StyleButton("View Step by Step Tutorial", ImVec2(but_gadget_size, 0)))
					{
						// pre-select tutorial 03
						extern bool bHelpVideo_Window;
						extern bool bHelp_Window;
						extern char cForceTutorialName[1024];
						bHelp_Window = true;
						bHelpVideo_Window = true;
						extern bool bSetTutorialSectionLeft;
						bSetTutorialSectionLeft = false;
						strcpy(cForceTutorialName, cShowTutorial.Get());
					}
					if (ImGui::IsItemHovered()) ImGui::SetTooltip("Start Step by Step Tutorial");
					#endif

					ImGui::Indent(-10);
				}
				#endif
			}

			ImGui::PopItemWidth();

			void CheckMinimumDockSpaceSize(float minsize);
			CheckMinimumDockSpaceSize(250.0f);

			if (ImGui::GetCurrentWindow()->ScrollbarSizes.x > 0) 
			{
				//Hitting exactly at the botton could cause flicker, so add some additional lines when scrollbar on.
				ImGui::Text("");
				ImGui::Text("");
			}

			ImGui::End();
		}
	}
	else
	{
		if (g_CharacterCreatorPlus.bInitialised)
		{
			//Make sure we hide ccp
			if (bCharObjVisible && ObjectExist(iCharObj))
			{
				extern bool g_bShowBones;
				g_bShowBones = false;
				wiRenderer::SetToDrawDebugBoneLines(g_bShowBones);
				//ccp is only hidden so you still se bones, if going to the importer and enable bones, so move it out of the way.
				PositionObject(iCharObj, 500000, 500000, 500000);

				// first, erase preloaded files we dont need any more (and load in basics for when return to CCP)

				image_preload_files_wait();
				image_preload_files_reset();
				object_preload_files_wait(); // If it is still working on loading, it will crash when data is reset.
				object_preload_files_reset();

				// hide character creator model
				HideObject(iCharObj);

				if (ObjectExist(iDressRoom)) HideObject(iDressRoom);

				bCharObjVisible = false;

				t.inputsys.dowaypointview = 0;

				//Restore.
				waypoint_restore();

				t.gridentityhidemarkers = 0;
				editor_updatemarkervisibility();
				editor_refresheditmarkers();

				//  put all entities back where they were
				for (t.tcce = 1; t.tcce <= g.entityelementlist; t.tcce++)
				{
					t.tccentid = t.entityelement[t.tcce].bankindex;
					if (t.tccentid > 0)
					{
						t.tccsourceobj = t.entityelement[t.tcce].obj;
						if (ObjectExist(t.tccsourceobj) == 1)
						{
							PositionObject(t.tccsourceobj, t.entityelement[t.tcce].x, t.entityelement[t.tcce].y, t.entityelement[t.tcce].z);
						}
					}
				}

				//Restore editor camera.
				if (editoroldx_f != 0) //PE: Somehow editoroldx_f was 0 ?
				{
					t.cameraviewmode = oldtcameraviewmode;
					t.editorfreeflight.mode = editoroldmode_f;
					t.editorfreeflight.c.x_f = editoroldx_f;
					t.editorfreeflight.c.y_f = editoroldy_f;
					t.editorfreeflight.c.z_f = editoroldz_f;
					t.editorfreeflight.c.angx_f = editoroldangx_f;
					t.editorfreeflight.c.angy_f = editoroldangy_f;
					PositionCamera(t.editorfreeflight.c.x_f, t.editorfreeflight.c.y_f, t.editorfreeflight.c.z_f);
					RotateCamera(t.editorfreeflight.c.angx_f, t.editorfreeflight.c.angy_f, 0);
				}

				restore_visuals(t.visuals, t.visualsStorage);
				visuals_loop();
			}
		}
	}
}

void charactercreatorplus_getautoswapdata(char* filename)
{
	// prepare destination file
	cstr pPath = GetDir();
	pPath += cstr("\\");
	cstr pFile = filename;
	pFile = pPath + pFile;
	LPSTR pFilename = pFile.Get();
	AutoSwapData* pAutoSwapData = nullptr;
	
	if (FileExist(filename) == 1)
	{
		OpenToRead(1, pFilename);
		while (FileEnd(1) == 0)
		{
			// get line by line
			cstr line_s = ReadString(1);
			LPSTR pLine = line_s.Get();

			// get field name
			char pLeft[128];
			char pRight[128];
			bool bIsNewAutoSwap = false;
			char* pEquals = strstr(line_s.Get(), "=");

			if (pEquals)
				bIsNewAutoSwap = true;
			else if (!pEquals)
				pEquals = strstr(line_s.Get(), ":");

			if (!pEquals)
			{
				CloseFile(1);
				return;
			}

			// Separate left and right into category and part.
			strcpy(pRight, pEquals + 2);
			strcpy(pLeft, line_s.Get());
			pLeft[strlen(pLeft) - strlen(pEquals) - 1] = 0;
			
			if (bIsNewAutoSwap)
			{
				// Hit a new auto swap section.
				pAutoSwapData = new AutoSwapData;
				int category = charactercreatorplus_getcategoryindex(pLeft);
				if (category == -1)
				{
					// Received an invalid category.
					delete pAutoSwapData;
					pAutoSwapData = nullptr;
					CloseFile(1);
					return;
				}
				pAutoSwapData->iPartType = category;
				pAutoSwapData->sPartName = pRight;
				charactercreatorplus_storeautoswapdata(pAutoSwapData);
			}
			else
			{
				// Get the items that need to be swapped out in order to fit the equipped clothing.
				int iPartType = charactercreatorplus_getcategoryindex(pLeft);
				std::string name(pRight);
				pAutoSwapData->requiredSwapCategories.push_back(iPartType);
				pAutoSwapData->requiredSwapNames.push_back(name);
			}
		}

		// close file handling
		CloseFile(1);
	}
}

int charactercreatorplus_getcategoryindex(char* category)
{
	if (strcmp(category, "Head Gear") == NULL)
		return 0;
	else if (strcmp(category, "Hair") == NULL)
		return 1;
	else if (strcmp(category, "Head") == NULL)
		return 2;
	else if (strcmp(category, "Wearing") == NULL)
		return 3;
	else if (strcmp(category, "Facial Hair") == NULL)
		return 4;
	else if (strcmp(category, "Body") == NULL)
		return 5;
	else if (strcmp(category, "Legs") == NULL)
		return 6;
	else if (strcmp(category, "Feet") == NULL)
		return 7;
	else if (strcmp(category, "Accessory1") == NULL)
		return 8;
	else if (strcmp(category, "Accessory2") == NULL)
		return 9;
	else return -1;
}

// Add the auto-swap data to the relevant container.
void charactercreatorplus_storeautoswapdata(AutoSwapData* pData)
{
	// 0: Head Gear
	// 1: Hair
	// 2: Head
	// 3: Eye Glasses
	// 4: Facial Hair
	// 5: Body
	// 6: Legs
	// 7: Feet

	if (pData)
	{
		switch (pData->iPartType)
		{
		case 0: g_headGearMandatorySwaps.push_back(pData); break;
		case 1: break; // None needed (yet).
		case 2: break; // None needed (yet).
		case 3: break; // None needed (yet).
		case 4: break; // None needed (yet).
		case 5: break; // None needed (yet).
		case 6: break; // None needed (yet).
		case 7: break; // None needed (yet).
		case 8: break; // None needed (yet).
		case 9: break; // None needed (yet).
		default:break; 
		}
	}
}

void charactercreatorplus_performautoswap(int part)
{
	std::string sPartToSwap = "";
	std::map<std::string, std::string>* pAnnotationData = nullptr;
	std::vector<AutoSwapData*>* pAutoSwapData = nullptr;
	char pHairColour[128];

	// Get the correct part type to swap.
	switch (part)
	{
	case 0: sPartToSwap = cSelectedHeadGear;
		pAnnotationData = &CharacterCreatorAnnotatedHeadGear_s;
		pAutoSwapData = &g_headGearMandatorySwaps;
		break;
	case 1: sPartToSwap = cSelectedHair;
		pAnnotationData = &CharacterCreatorAnnotatedHair_s;
		break;
	case 2: sPartToSwap = cSelectedHead;
		pAnnotationData = &CharacterCreatorAnnotatedHead_s;
		break;
	case 3: sPartToSwap = cSelectedEyeglasses;
		pAnnotationData = &CharacterCreatorAnnotatedEyeglasses_s;
		break;
	case 4: sPartToSwap = cSelectedFacialHair;
		pAnnotationData = &CharacterCreatorAnnotatedFacialHair_s;
		break;
	case 5: sPartToSwap = cSelectedBody;
		pAnnotationData = &CharacterCreatorAnnotatedBody_s;
		break;
	case 6: sPartToSwap = cSelectedLegs;
		pAnnotationData = &CharacterCreatorAnnotatedFeet_s;
		break;
	case 7: sPartToSwap = cSelectedFeet;
		pAnnotationData = &CharacterCreatorAnnotatedLegs_s;
		break;
	case 8: sPartToSwap = cSelectedAccessory1;
		pAnnotationData = &CharacterCreatorAnnotatedAccessory1_s;
		break;
	case 9: sPartToSwap = cSelectedAccessory2;
		pAnnotationData = &CharacterCreatorAnnotatedAccessory2_s;
		break;
	}

	if (!pAutoSwapData || !pAnnotationData || sPartToSwap.length() == 0 || pAutoSwapData->size() == 0)
	{
		return;
	}

	std::string sNewPartUIName = "";

	// Get the name of the part that the user selected (as seen on-screen).
	for (const auto & part : *pAnnotationData)
	{
		if (part.first == sPartToSwap)
		{
			sNewPartUIName = part.second;
			break;
		}
	}

	// Couldn't find a matching name for the part.
	if (sNewPartUIName.length() == 0)
		return;

	// Get the swap data for this part (if any).
	AutoSwapData* pSwapToPerform = nullptr;
	for (const auto & pSwapData : *pAutoSwapData)
	{
		if (!pSwapData)
			continue;

		const char* pNameSegment = strstr(sNewPartUIName.c_str(), pSwapData->sPartName.c_str());
		if (pNameSegment)
		{
			// There are swaps that must take place with the chosen part.
			pSwapToPerform = pSwapData;
			break;
		}
	}

	// No swap needed for the chosen part.
	if (!pSwapToPerform)
		return;

	// Undo the last swap, so that the users original choice is maintained for this new swap.
	charactercreatorplus_restoreswappedparts();

	// Find the required mesh names to swap.
	for(int i = 0 ; i < pSwapToPerform->requiredSwapCategories.size(); i++)
	{
		char* pPartToSwap = nullptr;
		g_iPartsThatNeedReloaded[pSwapToPerform->requiredSwapCategories[i]] = 1;

		// Get the correct part type to swap.
		switch (pSwapToPerform->requiredSwapCategories[i])
		{
		case 0: pPartToSwap = cSelectedHeadGear;
			pAnnotationData = &CharacterCreatorAnnotatedHeadGear_s;
			break;
		case 1: pPartToSwap = cSelectedHair;
			pAnnotationData = &CharacterCreatorAnnotatedHair_s;
			break;
		case 2: pPartToSwap = cSelectedHead;
			pAnnotationData = &CharacterCreatorAnnotatedHead_s;
			break;
		case 3: pPartToSwap = cSelectedEyeglasses;
			pAnnotationData = &CharacterCreatorAnnotatedEyeglasses_s;
			break;
		case 4: pPartToSwap = cSelectedFacialHair;
			pAnnotationData = &CharacterCreatorAnnotatedFacialHair_s;
			break;
		case 5: pPartToSwap = cSelectedBody;
			pAnnotationData = &CharacterCreatorAnnotatedBody_s;
			break;
		case 6: pPartToSwap = cSelectedLegs;
			pAnnotationData = &CharacterCreatorAnnotatedFeet_s;
			break;
		case 7: pPartToSwap = cSelectedFeet;
			pAnnotationData = &CharacterCreatorAnnotatedLegs_s;
			break;
		case 8: pPartToSwap = cSelectedAccessory1;
			pAnnotationData = &CharacterCreatorAnnotatedAccessory1_s;
			break;
		case 9: pPartToSwap = cSelectedAccessory2;
			pAnnotationData = &CharacterCreatorAnnotatedAccessory2_s;
			break;
		}

		if (pPartToSwap == nullptr)
			continue;

		pSwapToPerform->swappedPartNames.push_back(pPartToSwap);

		// Exclude the following heads from being swapped (textures do not share a common structure with the others - so cannot be swapped).
		if (pSwapToPerform->requiredSwapCategories[i] == 2)
		{
			if (strstr(pPartToSwap, "adult male head 09")) continue;
			else if (strstr(pPartToSwap, "adult male head 10")) continue;
			else if (strstr(pPartToSwap, "adult male head 11")) continue;
			else if (strstr(pPartToSwap, "adult male head 12")) continue;
			else if (strstr(pPartToSwap, "adult male head 14")) continue;
		}

		// Get the name of the hair as seen in the UI, to then extract the hair colour.
		if (pSwapToPerform->requiredSwapCategories[i] == 1)
		{
			for (const auto & part : *pAnnotationData)
			{
				if (stricmp(part.first.c_str(), pPartToSwap) == NULL)
				{
					charactercreatorplus_extracthaircolour(part.second.c_str(), &pHairColour[0]);
					break;
				}
			}
		}
	
		char pPartBackup[MAX_PATH];
		strcpy(pPartBackup, pPartToSwap);

		strcpy(pPartToSwap, pSwapToPerform->requiredSwapNames[i].c_str());

		// Store the swap data so the swaps can be undone if the user changes the characters parts.
		g_previousAutoSwap = pSwapToPerform;

		// Store the last headgear autoswap, so that the original ui choice can be maintained.
		if (pSwapToPerform->requiredSwapCategories[i] == 2)
			g_pLastHeadgearAutoSwap = pSwapToPerform;
		
		// There is no mesh for "None", so do not alter pPartToSwap.
		if (strstr(pPartToSwap, "None"))
		{
			continue;
		}

		// Add the previous hair colour so we know which hair to swap to.
		if (pSwapToPerform->requiredSwapCategories[i] == 1)
		{
			if (strlen(pHairColour) > 0)
				strcpy(pPartToSwap + strlen(pPartToSwap), pHairColour);
		}

		int iSearchCounter = 1;
		for (const auto & part : *pAnnotationData)
		{
			// Now that we have the name of the replacement part, we need its mesh name so it can be loaded.
			if (strstr(part.second.c_str(), pPartToSwap))
			{
				strcpy(pPartToSwap, part.first.c_str());
				break;
			}

			iSearchCounter++;
		}

		// Couldn't find a replacement part, so undo changes.
		if (iSearchCounter > pAnnotationData->size())
			strcpy(pPartToSwap, pPartBackup);
	}
}

// Reverse any swaps that were applied on the users previous choice.
void charactercreatorplus_restoreswappedparts()
{
	if (g_previousAutoSwap == nullptr)
		return;

	for (int i = 0; i < g_previousAutoSwap->requiredSwapCategories.size(); i++)
	{
		char* pPartToSwap = nullptr;

		switch (g_previousAutoSwap->requiredSwapCategories[i])
		{
		case 0: pPartToSwap = cSelectedHeadGear;
			break;
		case 1: pPartToSwap = cSelectedHair;
			break;
		case 2: pPartToSwap = cSelectedHead;
			break;
		case 3: pPartToSwap = cSelectedEyeglasses;
			break;
		case 4: pPartToSwap = cSelectedFacialHair;
			break;
		case 5: pPartToSwap = cSelectedBody;
			break;
		case 6: pPartToSwap = cSelectedLegs;
			break;
		case 7: pPartToSwap = cSelectedFeet;
			break;
		case 8: pPartToSwap = cSelectedAccessory1;
			break;
		case 9: pPartToSwap = cSelectedAccessory2;
			break;
		}
		strcpy(pPartToSwap, g_previousAutoSwap->swappedPartNames[i].c_str());
		g_iPartsThatNeedReloaded[g_previousAutoSwap->requiredSwapCategories[i]] = 1;
	}

	g_previousAutoSwap = nullptr;
}

void charactercreatorplus_extracthaircolour(const char* source, char* destination)
{
	if (strstr(source, "Black"))
		strcpy(destination, " Black");
	else if (strstr(source, "Blonde"))
		strcpy(destination, " Blonde");
	else if (strstr(source, "Blond")) // Must be after Blonde.
		strcpy(destination, " Blond");
	else if (strstr(source, "Brown"))
		strcpy(destination, " Brown");
	else if (strstr(source, "Grey"))
		strcpy(destination, " Grey");
	else if (strstr(source, "Red"))
		strcpy(destination, " Red");
	else if (strstr(source, "Purple"))
		strcpy(destination, " Purple");
	else if (strstr(source, "Brunette"))
		strcpy(destination, " Brunette");
	else if (strstr(source, "White"))
		strcpy(destination, " White");
	else if (strstr(source, "Ginger"))
		strcpy(destination, " Red"); // There is only one instance of ginger hair, so caesar red is used when swapping it.
	else if (strstr(source, "Straw"))
		strcpy(destination, " Straw");
	else if (strstr(source, "Dark"))
		strcpy(destination, " Dark");
}

bool charactercreatorplus_checkforautoswapcategory(char* pSwappedPartName, int iSwappedPartCategory, int iCategoryToFind)
{
	std::vector<AutoSwapData*>* pSwapData = nullptr;

	switch (iSwappedPartCategory)
	{
	case 0: pSwapData = &g_headGearMandatorySwaps;
		break;
	}

	if (pSwapData == nullptr)
		return false;

	for (const auto& swap : *pSwapData)
	{
		// Check if the character has any parts equipped from the chosen auto swap category.
		if (strstr(pSwappedPartName, swap->sPartName.c_str()))
		{
			// Now see if there is any auto-swap data for the desired category (another type of part that gets swapped because of iSwappedPartCategory).
			for (int i = 0; i < swap->requiredSwapCategories.size(); i++)
			{
				if (swap->requiredSwapCategories[i] == iCategoryToFind)
				{
					// Found an auto swap with this category.
					return true;
				}
			}
		}
	}

	// Did not find an auto swap for the desired category.
	return false;
}

void charactercreatorplus_restrictpart(int part)
{
	// 0: Head Gear
	// 1: Hair
	// 2: Head
	// 3: Eye Glasses
	// 4: Facial Hair
	// 5: Body
	// 6: Legs
	// 7: Feet

	g_restrictedParts.clear();

	std::map<std::string, std::string>* pAnnotated = nullptr;
	char* pSelectedGear = nullptr;
	int iCategoryToFind = part;
	int iSwappedPartCategory = -1;

	// Restricting headgear is treated specially.
	if (part > 0)
	{
		// Restricting hair, head, glasses or facial hair (all depend on the chosen headgear).
		if (part < 5)
		{
			pAnnotated = &CharacterCreatorAnnotatedHeadGear_s;
			pSelectedGear = cSelectedHeadGear;
			iSwappedPartCategory = 0;
		}

		if (!pSelectedGear || !pAnnotated)
			return;

		char gear[256];

		// Get the annotation for the currently selected gear so we can compare it to the autoswaps.
		for (const auto& annotation : *pAnnotated)
		{
			if (stricmp(annotation.first.c_str(), pSelectedGear) == NULL)
			{
				strcpy(gear, annotation.second.c_str());
				break;
			}
		}

		if (strlen(gear) == 0)
			return;

		bool bRestricted = charactercreatorplus_checkforautoswapcategory(gear, iSwappedPartCategory, iCategoryToFind);

		// No restrictions needed.
		if (!bRestricted)
			return;

		// In future if we add more parts, it would be better to extract them from annotates automatically.
		switch (part)
		{
		case 1:
			if (stricmp(CCP_Type, "Adult Male") == NULL)
			{
				g_restrictedParts.push_back("Parting");
				g_restrictedParts.push_back("Swept");
				g_restrictedParts.push_back("Cropped");
				g_restrictedParts.push_back("Viking");
				g_restrictedParts.push_back("Aztec Bowl");
				g_restrictedParts.push_back("Aztec Ponytail");
			}
			else if (stricmp(CCP_Type, "Adult Female") == NULL)
			{
				g_restrictedParts.push_back("Ponytail");
				g_restrictedParts.push_back("Braids");
				g_restrictedParts.push_back("Wavy");
				g_restrictedParts.push_back("Punk");
			}
			break;

		case 2:
			if (stricmp(CCP_Type, "Adult Male") == NULL)
			{
				g_restrictedParts.push_back("Dark Old");
				g_restrictedParts.push_back("Asian Old");
				g_restrictedParts.push_back("Caucasian Old");
				g_restrictedParts.push_back("Mediterranean Old");
				g_restrictedParts.push_back("Worn");
			}
			break;

		case 3:
			g_restrictedParts.push_back("Vintage Glasses");
			g_restrictedParts.push_back("Sport Sunglasses");
			g_restrictedParts.push_back("Aviator Sunglasses");
			g_restrictedParts.push_back("Round Glasses");

			if (stricmp(CCP_Type, "Adult Female") == NULL)
				g_restrictedParts.push_back("Round Glasses");

			break;

		case 4:
			if (stricmp(CCP_Type, "Adult Male") == NULL)
			{
				g_restrictedParts.push_back("Scruffy");
				g_restrictedParts.push_back("Beard");
				g_restrictedParts.push_back("Moustache");
				g_restrictedParts.push_back("Biker");
			}

			break;
		}
	}
	else if (part == 0 && stricmp(CCP_Type, "Adult Male") == NULL)
	{
		std::vector<char*> headsWithDifferentStructure;
		headsWithDifferentStructure.push_back("Dark Old");
		headsWithDifferentStructure.push_back("Asian Old");
		headsWithDifferentStructure.push_back("Caucasian Old");
		headsWithDifferentStructure.push_back("Mediterranean Old");
		headsWithDifferentStructure.push_back("Worn");

		// Find the mesh name of the currently equipped head.
		char head[256];
		for (const auto& annotation : CharacterCreatorAnnotatedHead_s)
		{
			if (stricmp(annotation.first.c_str(), cSelectedHead) == NULL)
			{
				strcpy(head, annotation.second.c_str());
				break;
			}
		}

		// Check if the equipped head is one of the above with different texture structures, and is incompatible with some headgears.
		bool bHeadHasRestrictions = false;
		for (int i = 0; i < headsWithDifferentStructure.size(); i++)
		{
			if (strstr(head, headsWithDifferentStructure[i]))
			{
				bHeadHasRestrictions = true;
				break;
			}
		}

		if (bHeadHasRestrictions)
		{
			// There is no auto-swap data available for heads.
			// ...So going to search for a head category within each headgear autoswap and if found, it will be restricted.
			for (const auto& swap : g_headGearMandatorySwaps)
			{
				for (int i = 0; i < swap->requiredSwapCategories.size(); i++)
				{
					if (swap->requiredSwapCategories[i] == 2)
					{
						// This headgear auto-swap requires the face to be changed.
						// ... Since the currently equipped face is one of the structurally different ones, it is incompatible with the head gear.
						
						g_restrictedParts.push_back(&swap->sPartName[0]);
						break;
					}
				}
			}
		}
	}
}

