void About_Screen(void)
{
	ImGuiWindowFlags ex_window_flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking;
	if (refresh_gui_docking == 0)
	{
		if (iAboutLogoType == 2)
			ImGui::SetNextWindowSize(ImVec2(52 * ImGui::GetFontSize(), 39 * ImGui::GetFontSize()), ImGuiCond_Always); //ImGuiCond_FirstUseEver
		else
			ImGui::SetNextWindowSize(ImVec2(52 * ImGui::GetFontSize(), 30 * ImGui::GetFontSize()), ImGuiCond_Always); //ImGuiCond_FirstUseEver
		ImGui::SetNextWindowPosCenter(ImGuiCond_Always);
		bool bAlwaysTrue = true;
		ImGui::Begin("##About##AboutWindow", &bAlwaysTrue, ex_window_flags);
		ImGui::End();
	}
	else if (bAbout_Window)
	{
		if (bAbout_Window_First_Run)
		{
			if (iAboutLogoType == 2)
				ImGui::SetNextWindowSize(ImVec2(52 * ImGui::GetFontSize(), 39 * ImGui::GetFontSize()), ImGuiCond_Always); //ImGuiCond_FirstUseEver
			else
				ImGui::SetNextWindowSize(ImVec2(52 * ImGui::GetFontSize(), 30 * ImGui::GetFontSize()), ImGuiCond_Always); //ImGuiCond_FirstUseEver
			ImGui::SetNextWindowPosCenter(ImGuiCond_Always);
			bAbout_Window_First_Run = false;
		}

		ImGui::Begin("##About##AboutWindow", &bAbout_Window, ex_window_flags);

		ImGui::Text("");

		float fRegionWidth = ImGui::GetWindowContentRegionWidth();
		float img_w = ImageWidth(ABOUT_LOGO);
		float img_h = ImageHeight(ABOUT_LOGO);

		ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((fRegionWidth*0.5) - (img_w*0.5), 0.0f));

		ImGui::ImgBtn(ABOUT_LOGO, ImVec2(img_w, img_h), ImVec4(0.0, 0.0, 0.0, 0.0), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(1.0, 1.0, 1.0, 1.0), 0, 0, 0, 0, false);

		ImGui::TextCenter("");
		char pBuildText[1024];
		sprintf(pBuildText, "Version: %s", g.version_s.Get());
		ImGui::TextCenter(pBuildText);
		ImGui::TextCenter("");

		ImGui::SetWindowFontScale(1.25);
		ImGui::TextCenter("(c) Copyright 2020-2025 Dark Basic Software Limited. All Rights Reserved");
		ImGui::SetWindowFontScale(1.0);
		ImGui::TextCenter("GameGuru MAX and the respective logos are trademarks or registred trademarks of Dark Basic Software Limited.");
		
		ImGui::Text("");
		ImGui::Text("");
		ImVec2 cp = ImGui::GetCursorPos() + ImVec2(fRegionWidth*0.5 , 0.0f);
		ImGui::SetCursorPos(cp + ImVec2(-200.0f, 0.0f));
		if (ImGui::StyleButton("View Credits", ImVec2(100.0f, 0.0f))) 
		{
			bCredits_Window = true;
			bAbout_Window = false;
			bCredits_Window_First_Run = true;
		}
		ImGui::SameLine();
		ImGui::SetCursorPos(cp + ImVec2(100.0f, 0.0f));
		if (ImGui::StyleButton("OK", ImVec2(100.0f, 0.0f))) 
		{
			bAbout_Window = false;
		}
		ImGui::Text("");

		bImGuiGotFocus = true;

		cstr title = "About";
		float fTextSize = ImGui::CalcTextSize(title.Get()).x;
		float xcenter = (ImGui::GetWindowSize().x*0.5) - (fTextSize*0.5);
		ImVec2 titlebar_pos = ImGui::GetWindowPos() + ImVec2(xcenter, 4);
		ImGuiWindow* window = ImGui::GetCurrentWindow();

		ImGui::End();

		//Render title bar after End. end fill titlebar.
		ImGuiContext& g = *GImGui;
		window->DrawList->AddText(g.Font, g.FontSize, titlebar_pos, ImGui::GetColorU32(ImGuiCol_Text), title.Get());

	}

	//Credits window.
	if (refresh_gui_docking == 0)
	{
		ImGui::SetNextWindowSize(ImVec2(32 * ImGui::GetFontSize(), 52 * ImGui::GetFontSize()), ImGuiCond_Always); //ImGuiCond_FirstUseEver
		ImGui::SetNextWindowPosCenter(ImGuiCond_Always);
		bool bAlwaysTrue = true;
		ImGui::Begin("##Credits##AboutWindow", &bAlwaysTrue, ex_window_flags);
		ImGui::End();
	}
	else if (bCredits_Window)
	{

		if (bCredits_Window_First_Run)
		{
			ImGui::SetNextWindowSize(ImVec2(32 * ImGui::GetFontSize(), 52 * ImGui::GetFontSize()), ImGuiCond_Always); //ImGuiCond_FirstUseEver
			ImGui::SetNextWindowPosCenter(ImGuiCond_Always);
			bCredits_Window_First_Run = false;
		}

		ImGui::Begin("##Credits##AboutWindow", &bCredits_Window, ex_window_flags);

		ImGui::Text("");
		ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(0.0f, 8.0f));

		float fRegionWidth = ImGui::GetWindowContentRegionWidth();

		ImGui::SetWindowFontScale(1.5);
		ImGui::TextCenter("Programming Team");
		ImGui::SetWindowFontScale(1.0);
		ImGui::Text("");
		ImGui::TextCenter("Lee Bamber & Preben Eriksen");
		ImGui::TextCenter("Paul Johnston & Zak Judges");
		ImGui::TextCenter("Mike Johnson & Maciej Dowbor");
		ImGui::Text("");

		ImGui::SetWindowFontScale(1.5);
		ImGui::TextCenter("Community Contributors");
		ImGui::SetWindowFontScale(1.0);
		ImGui::Text("");
		ImGui::TextCenter("Synchromesh & Dave Hawkins for Support");
		ImGui::TextCenter("Necrym59 for Behaviors and Design");
		ImGui::TextCenter("Tom from Blood Moon Interactive for User Manual");
		ImGui::TextCenter("GraphiX for Art Support");
		ImGui::Text("");

		ImGui::SetWindowFontScale(1.5);
		ImGui::TextCenter("Art & Media Team");
		ImGui::SetWindowFontScale(1.0);
		ImGui::Text("");
		ImGui::TextCenter("Mark Blosser aka BOND1 - 3D and Animation");
		ImGui::TextCenter("Peter Jovanovic & Ugur Gokus");
		ImGui::TextCenter("Martin Oliver & Glynn Taylor");
		ImGui::TextCenter("Ispas Gabriela Cristina & Volkov Studio");
		ImGui::Text("");

		ImGui::SetWindowFontScale(1.5);
		ImGui::TextCenter("Design Team");
		ImGui::SetWindowFontScale(1.0);
		ImGui::Text("");
		ImGui::TextCenter("Lee Bamber & Richard Vanner");
		ImGui::TextCenter("Meash Meakin & Stuart Scott");

		ImGui::Text("");
		ImGui::SetWindowFontScale(1.5);
		ImGui::TextCenter("Wicked Engine");
		ImGui::SetWindowFontScale(1.0);
		ImGui::Text("");
		ImGui::TextCenter("Janos Turanszki");
		ImGui::TextCenter("www.wickedengine.net");

		ImGui::Text("");
		ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((fRegionWidth*0.5) - (100.0f*0.5), 0.0f));
		if (ImGui::StyleButton("OK", ImVec2(100.0f, 0.0f))) {
			bCredits_Window = false;
			bAbout_Window = false;
		}

		bImGuiGotFocus = true;

		cstr title = "GameGuru MAX Credits";
		float fTextSize = ImGui::CalcTextSize(title.Get()).x;
		float xcenter = (ImGui::GetWindowSize().x*0.5) - (fTextSize*0.5);
		ImVec2 titlebar_pos = ImGui::GetWindowPos() + ImVec2(xcenter, 4);
		ImGuiWindow* window = ImGui::GetCurrentWindow();

		ImGui::End();

		//Render title bar after End. end fill titlebar.
		ImGuiContext& g = *GImGui;
		window->DrawList->AddText(g.Font, g.FontSize, titlebar_pos, ImGui::GetColorU32(ImGuiCol_Text), title.Get());

	}

}



void FixEulerZInverted(float &ax, float &ay, float &az)
{
	if (ax < 0.0f) ax += 360.0f;
	if (ay < 0.0f) ay += 360.0f;
	if (az < 0.0f) az += 360.0f;
	if (ax > 360.0f) ax -= 360.0f;
	if (ay > 360.0f) ay -= 360.0f;
	if (az > 360.0f) az -= 360.0f;
	bool bZFlipped = false, bXFlipped = false, bXDeadPos = false , bZDeadPos = false;
	if (az >= 179.5f && az <= 180.5f) bZFlipped = true; // 180
	if (ax >= 179.5f && ax <= 180.5f) bXFlipped = true; // 180
	if (ax >= 89.5f && ax <= 90.5f) bXDeadPos = true; // 90
	if (az >= 299.5f && az <= 300.5f) bZDeadPos = true; // 300 Got a 299.7 , so lowered to 299.5.
	if (bZFlipped && bXFlipped)
	{
		//PE: When both z and x flipped it count backward, so 180-ay = real Y without x,z.
		ax = 0.0f;
		az = 0.0f;
		ay = (180.0 - ay);
	}
	else if (!bXDeadPos && bZFlipped)
	{
		ax -= 180.0f;
		ay = (180.0 - ay);
		az = 0.0f;
	}
	else if (bXDeadPos && bZDeadPos)
	{
		//Y dont change. z is just moved to x
		ax = az + 90;
		az = 0.0f;
	}
	if (ax < 0.0f) ax += 360.0f;
	if (ay < 0.0f) ay += 360.0f;
	if (az < 0.0f) az += 360.0f;
	if (ax > 360.0f) ax -= 360.0f;
	if (ay > 360.0f) ay -= 360.0f;
	if (az > 360.0f) az -= 360.0f;
	return;
}

void SetStartPositionsForRubberBand(int iActiveObj)
{
	// for multiple objects
	if (g.entityrubberbandlist.size() > 0)
	{
		// for each object in the selection
		for (int i = 0; i < (int)g.entityrubberbandlist.size(); i++)
		{
			int e = g.entityrubberbandlist[i].e;
			int tobj = t.entityelement[e].obj;
			if (tobj > 0)
			{
				if (ObjectExist(tobj) == 1)
				{
					// store starting position and orientation of objects
					g.entityrubberbandlist[i].x = ObjectPositionX(tobj) - ObjectPositionX(iActiveObj);
					g.entityrubberbandlist[i].y = ObjectPositionY(tobj) - ObjectPositionY(iActiveObj);
					g.entityrubberbandlist[i].z = ObjectPositionZ(tobj) - ObjectPositionZ(iActiveObj);
					if (t.entityelement[e].quatmode == 1)
					{
						g.entityrubberbandlist[i].quatAngle = GGQUATERNION(t.entityelement[e].quatx, t.entityelement[e].quaty, t.entityelement[e].quatz, t.entityelement[e].quatw);
					}
					else
					{
						GGQUATERNION QuatAroundX, QuatAroundY, QuatAroundZ;
						GGQuaternionRotationAxis(&QuatAroundX, &GGVECTOR3(1, 0, 0), GGToRadian(ObjectAngleX(tobj)));
						GGQuaternionRotationAxis(&QuatAroundY, &GGVECTOR3(0, 1, 0), GGToRadian(ObjectAngleY(tobj)));
						GGQuaternionRotationAxis(&QuatAroundZ, &GGVECTOR3(0, 0, 1), GGToRadian(ObjectAngleZ(tobj)));
						g.entityrubberbandlist[i].quatAngle = QuatAroundX * QuatAroundY * QuatAroundZ;
					}
				}
			}
		}
	}
}

void RotateAndMoveRubberBand(int iActiveObj, float fMovedActiveObjectX, float fMovedActiveObjectY, float fMovedActiveObjectZ, GGQUATERNION quatRotationEvent )//float fMovedActiveObjectRX, float fMovedActiveObjectRY, float fMovedActiveObjectRZ)
{
	// for multiple objects
	if (g.entityrubberbandlist.size() > 0)
	{
		// for each object in the selection
		for (int i = 0; i < (int)g.entityrubberbandlist.size(); i++)
		{
			int e = g.entityrubberbandlist[i].e;
			int tobj = t.entityelement[e].obj;
			if (tobj > 0 && t.entityelement[e].editorlock == 0)
			{
				if (ObjectExist(tobj) == 1)
				{
					if (tobj != iActiveObj)
					{
						// this object rotyation quat
						GGQUATERNION quatThisOrientation = g.entityrubberbandlist[i].quatAngle;

						// apply the rotation event to the object angle
						GGQUATERNION quatNewOrientation;
						GGQuaternionMultiply(&quatNewOrientation, &quatThisOrientation, &quatRotationEvent);

						// rotate this object with final quat and get new entity rotation eulers
						RotateObjectQuat(tobj, quatNewOrientation.x, quatNewOrientation.y, quatNewOrientation.z, quatNewOrientation.w);
						
						// final angles
						t.entityelement[e].rx = WrapValue(ObjectAngleX(tobj));
						t.entityelement[e].ry = WrapValue(ObjectAngleY(tobj));
						t.entityelement[e].rz = WrapValue(ObjectAngleZ(tobj));

						// and store the quat
						entity_updatequat(e, quatNewOrientation.x, quatNewOrientation.y, quatNewOrientation.z, quatNewOrientation.w);

						// apply the rotation event to the position
						GGVECTOR3 positionalOffset;
						positionalOffset.x = g.entityrubberbandlist[i].x;
						positionalOffset.y = g.entityrubberbandlist[i].y;
						positionalOffset.z = g.entityrubberbandlist[i].z;
						GGMATRIX matRotatePositions;
						GGMatrixRotationQuaternion(&matRotatePositions, &quatRotationEvent);
						GGVec3TransformCoord(&positionalOffset, &positionalOffset, &matRotatePositions);

						// if object was static, flag that static object moved
						if (t.entityelement[e].staticflag == 1) g.projectmodifiedstatic = 1;

						// put new adjusted positions back
						t.entityelement[e].x = ObjectPositionX(iActiveObj) + positionalOffset.x;
						t.entityelement[e].y = ObjectPositionY(iActiveObj) + positionalOffset.y;
						t.entityelement[e].z = ObjectPositionZ(iActiveObj) + positionalOffset.z;

						// update entity to new offset position if any movement
						t.entityelement[e].x = t.entityelement[e].x + fMovedActiveObjectX;
						t.entityelement[e].y = t.entityelement[e].y + fMovedActiveObjectY;
						t.entityelement[e].z = t.entityelement[e].z + fMovedActiveObjectZ;

						// finally update latest object position
						PositionObject(tobj, t.entityelement[e].x, t.entityelement[e].y, t.entityelement[e].z);

						// update light data for spot
						if (t.entityelement[e].eleprof.usespotlighting)	lighting_refresh();

						// move zones and lights if in group
						widget_movezonesandlights(e);
					}
				}
			}
		}
	}
}

void SetLightShaftState(bool bState)
{
	if (master_renderer) master_renderer->setLightShaftsEnabled(bState);
}
bool GetLightShaftState(void)
{
	if (master_renderer) return master_renderer->getLightShaftsEnabled();
	return false;
}

bool GetLensFlareState()
{
	if (master_renderer) return master_renderer->getLensFlareEnabled();
	return false;
}
void SetLensFlareState(bool bState)
{
	if (master_renderer) master_renderer->setLensFlareEnabled(bState);
}

void editor_toggle_element_vis(bool bIsVisible)
{
	if (t.game.gameisexe == 1) return; //PE: This trigger a 7018 error in standalone. from DrawLogicNodes();

	// hide all markers from view: win zones, player start, image zones etc.
	for (t.e = 1; t.e <= g.entityelementlist; t.e++)
	{
		t.entid = t.entityelement[t.e].bankindex;
		t.obj = t.entityelement[t.e].obj;
		if (t.obj > 0)
		{
			if (ObjectExist(t.obj) == 1)
			{
				if (t.entityprofile[t.entid].ismarker)
				{
					if (bIsVisible)
					{
						ShowObject(t.obj);
					}
					else
					{
						HideObject(t.obj);
					}
				}
			}
		}
	}

	// hide show waypoints
	if (bIsVisible)
	{
		for (t.waypointindex = 1; t.waypointindex <= g.waypointmax; t.waypointindex++)
		{
			t.obj = g.editorwaypointoffset + t.waypointindex;
			if (ObjectExist(t.obj) == 1)
			{
				ShowObject(t.obj);
				t.waypoint[t.waypointindex].active = 1;
			}
		}
	}
	else
	{
		for (t.waypointindex = 1; t.waypointindex <= g.waypointmax; t.waypointindex++)
		{
			t.obj = g.editorwaypointoffset + t.waypointindex;
			if (ObjectExist(t.obj) == 1)
			{
				HideObject(t.obj);
			}
		}

		if (ObjectExist(g.editorwaypointoffset + 0) == 1) 
		{
			HideObject(g.editorwaypointoffset + 0);
		}
	}

	// hide show relational lines
	extern bool g_bDotsAreVisible;
	if (t.showeditorelements)
	{
		if (g_bDotsAreVisible==false)
		{
			DrawLogicNodes(true);
			g_bDotsAreVisible = true;
		}
	}
	else
	{
		if (g_bDotsAreVisible==true)
		{
			DrawLogicNodes(false);
			g_bDotsAreVisible = false;
		}
	}

	//  Deactivate widget if still in effect
	if (!bIsVisible)
	{
		widget_switchoff();

		//  Deactivate floating selection of entity
		if (t.grideditselect != 5 && t.grideditselect != 4)
		{
			if (t.grideditselect != 5) HideObject(t.editor.objectstartindex + 5);
			t.gridentity = 0; t.gridentityposoffground = 0;
			t.gridentityusingsoftauto = 0;
			t.gridentitysurfacesnap = 1 - g.gdisablesurfacesnap;
			// MAX handles its own positioning system
			t.gridentityautofind = 0;
			t.inputsys.dragoffsetx_f = 0;
			t.inputsys.dragoffsety_f = 0;
		}
	}
	//  Update entity cursor? (delete many of these as it WAS old shroud updater!)
	t.refreshgrideditcursor = 1;

	//  Waypoint visibility
	if (t.grideditselect != t.lastgrideditselect)
	{
		t.lastgrideditselect = t.grideditselect;
		if (t.grideditselect == 6)
		{
			waypoint_showallpaths();
		}
		else
		{
			if (t.inputsys.dowaypointview == 0)
			{
				waypoint_showallpaths();
			}
			else
			{
				waypoint_hideallpaths();
			}
		}
	}

	// clear any gridentity light if gridentity no longer used
	if (t.gridentity == 0)
	{
		if (t.gridentitywickedlightindex > 0)
		{
			WickedCall_DeleteLight(t.gridentitywickedlightindex);
			t.gridentitywickedlightindex = 0;
		}
	}
}

bool DoTreeNodeEntity(int masterid,bool bMoveCameraToObjectPosition)
{
	for (int i = 1; i < t.entityelement.size(); i++)
	{
		bool bValid = true;
		if (t.entityelement[i].iIsSmarkobjectDummyObj == 1) bValid = false;
		if (bValid)
		{
			if (masterid > 0 && t.entityelement[i].bankindex == masterid || (t.widget.pickedEntityIndex == i && t.gridentity == masterid))
			{
				char cName[512];
				strcpy(cName, t.entityprofileheader[masterid].desc_s.Get());
				if(t.entityelement[i].eleprof.name_s.Len()  > 0 )
					strcpy(cName, t.entityelement[i].eleprof.name_s.Get());

				ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_OpenOnArrow; //Got sub selections.

				node_flags = ImGuiTreeNodeFlags_Leaf; //No sub selections.

				bool bSelected = false;
				//Find selection here.
				if (bSelected)
					node_flags |= ImGuiTreeNodeFlags_Selected;
				else
					node_flags &= ~ImGuiTreeNodeFlags_Selected;

				ImGui::PushItemWidth(-20.0); //PE: Room for a icon.

				std::string treename = "#" + std::to_string(i);
				if (t.widget.pickedEntityIndex == i && t.gridentity == masterid)
					treename = treename + " (Cursor) " + cName;
				else
					treename = treename + " " + cName;

				bool bAutoGenObject = false;
				if (t.entityelement[i].x == -99999 && t.entityelement[i].y == -99999 && t.entityelement[i].z == -99999)
				{
					treename = treename + " (Auto-Gen) ";
					bAutoGenObject = true;
				}
				if (t.entityelement[i].y == -999999)
				{
					treename = treename + " (Hidden) ";
					bAutoGenObject = true;
				}

				bool TreeNodeOpen = ImGui::TreeNodeEx((void*)(intptr_t)(i + 90000), node_flags, treename.c_str());
				ImGui::PopItemWidth();

				//PE: Select on mouse release.
				if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(0))
				{
					//PE: Find object in scene. and move camera. and select to cursor.
					if (t.entityelement[i].obj > 0)
					{
						t.widget.pickedEntityIndex = i;
						t.widget.pickedObject = t.entityelement[t.widget.pickedEntityIndex].obj;
						g.entityrubberbandlist.clear();

						bEditorInFreeFlightMode = true; //PE: Must be in freeflight mode.
						t.editorfreeflight.mode = 1;

						int group = isEntityInGroupList(t.widget.pickedEntityIndex);
						if (group >= 0)
						{
							//PE: Add all groups with entity to rubberband.
							CheckGroupListForRubberbandSelections(t.widget.pickedEntityIndex);
						}

						if (bMoveCameraToObjectPosition == true && bAutoGenObject == false)
						{
							float zoom = ObjectSize(t.entityelement[i].obj, 1) * 2.0;
							if (zoom < 30.0f) zoom = 30.0f;
							float realcamy = ObjectSizeY(t.entityelement[i].obj, 1) * 0.75;
							float camy = realcamy;
							if (camy < 30.0f) camy = 30.0f;

							if (t.entityprofile[masterid].ismarker > 0)
							{
								zoom = 100.0;
								camy = 50.0;
							}

							//PE: Move camera keep camera Y.
							PositionCamera(t.entityelement[i].x, t.entityelement[i].y, t.entityelement[i].z);
							PointCamera(t.entityelement[i].x, t.entityelement[i].y, t.entityelement[i].z);
							MoveCamera(0, -zoom);
							PositionCamera(CameraPositionX(0), t.entityelement[i].y + camy, CameraPositionZ(0));
							PointCamera(t.entityelement[i].x, t.entityelement[i].y + (realcamy * 0.5), t.entityelement[i].z);
							t.editorfreeflight.c.x_f = CameraPositionX();
							t.editorfreeflight.c.y_f = CameraPositionY();
							t.editorfreeflight.c.z_f = CameraPositionZ();
							t.editorfreeflight.c.angx_f = CameraAngleX();
							t.editorfreeflight.c.angy_f = CameraAngleY();
							t.cx_f = t.editorfreeflight.c.x_f;
							t.cy_f = t.editorfreeflight.c.z_f;
						}
					}
				}

				if (TreeNodeOpen) 
				{
					ImGui::TreePop();
				}
			}
		}
	}
	return(0);
}

bool DoTreeNodeGroup(int groupindex, bool bMoveCameraToObjectPosition)
{
	for (int i = 1; i < t.entityelement.size(); i++)
	{
		bool bValid = true;
		if (t.entityelement[i].iIsSmarkobjectDummyObj == 1) bValid = false;
		if (bValid)
		{
			if ( groupindex > 0 )
			{
				int iGroupID = isEntityInGroupList(i);
				if (groupindex == iGroupID)
				{
					char cName[512];
					int masterid = t.entityelement[i].bankindex;
					strcpy(cName, t.entityprofileheader[masterid].desc_s.Get());
					if (t.entityelement[i].eleprof.name_s.Len() > 0)
						strcpy(cName, t.entityelement[i].eleprof.name_s.Get());

					ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_OpenOnArrow;
					node_flags = ImGuiTreeNodeFlags_Leaf;

					//Find selection here.
					bool bSelected = false;
					if (bSelected)
						node_flags |= ImGuiTreeNodeFlags_Selected;
					else
						node_flags &= ~ImGuiTreeNodeFlags_Selected;

					ImGui::PushItemWidth(-20.0);

					std::string treename = "#" + std::to_string(i);
					treename = treename + " " + cName;

					bool TreeNodeOpen = ImGui::TreeNodeEx((void*)(intptr_t)(i + 90000), node_flags, treename.c_str());
					ImGui::PopItemWidth();

					if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(0))
					{
						//PE: Find object in scene. and move camera. and select to cursor.
						if (t.entityelement[i].obj > 0)
						{
							t.widget.pickedEntityIndex = i;
							t.widget.pickedObject = t.entityelement[t.widget.pickedEntityIndex].obj;
							g.entityrubberbandlist.clear();
							bEditorInFreeFlightMode = true;
							t.editorfreeflight.mode = 1;
							int group = isEntityInGroupList(t.widget.pickedEntityIndex);
							if (group >= 0)
							{
								//PE: Add all groups with entity to rubberband.
								CheckGroupListForRubberbandSelections(t.widget.pickedEntityIndex);
							}
							if (bMoveCameraToObjectPosition == true)
							{
								PositionCamera(t.entityelement[i].x, t.entityelement[i].y+500, t.entityelement[i].z-500);
								PointCamera(t.entityelement[i].x, t.entityelement[i].y, t.entityelement[i].z);
								t.editorfreeflight.c.x_f = CameraPositionX();
								t.editorfreeflight.c.y_f = CameraPositionY();
								t.editorfreeflight.c.z_f = CameraPositionZ();
								t.editorfreeflight.c.angx_f = CameraAngleX();
								t.editorfreeflight.c.angy_f = CameraAngleY();
								t.cx_f = t.editorfreeflight.c.x_f;
								t.cy_f = t.editorfreeflight.c.z_f;
							}
						}
					}
					if (TreeNodeOpen)
					{
						ImGui::TreePop();
					}
				}
			}
		}
	}
	return(0);
}

bool DoTreeNodeBehavior(LPSTR behaviorscriptname, bool bMoveCameraToObjectPosition)
{
	for (int i = 1; i < t.entityelement.size(); i++)
	{
		bool bValid = true;
		if (t.entityelement[i].iIsSmarkobjectDummyObj == 1) bValid = false;
		if (bValid)
		{
			int masterid = t.entityelement[i].bankindex;
			if (masterid  > 0)
			{
				char cName[512];
				strcpy(cName, t.entityprofileheader[masterid].desc_s.Get());
				if (t.entityelement[i].eleprof.name_s.Len() > 0)
					strcpy(cName, t.entityelement[i].eleprof.name_s.Get());

				// reject any that do not match the required behavior script
				bool bBehaviorMatch = false;
				if (t.entityelement[i].eleprof.aimain_s.Len() > 0)
				{
					if (stricmp(t.entityelement[i].eleprof.aimain_s.Get(), behaviorscriptname) == NULL)
					{
						bBehaviorMatch = true;
					}
				}
				if (bBehaviorMatch == false) continue;

				ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_Leaf;

				ImGui::PushItemWidth(-20.0); //PE: Room for a icon.

				std::string treename = "#" + std::to_string(i);
				treename = treename + " " + cName;

				bool bAutoGenObject = false;
				if (t.entityelement[i].x == -99999 && t.entityelement[i].y == -99999 && t.entityelement[i].z == -99999)
				{
					treename = treename + " (Auto-Gen) ";
					bAutoGenObject = true;
				}
				if (t.entityelement[i].y == -999999)
				{
					treename = treename + " (Hidden) ";
					bAutoGenObject = true;
				}

				bool TreeNodeOpen = ImGui::TreeNodeEx((void*)(intptr_t)(i + 90000), node_flags, treename.c_str());
				ImGui::PopItemWidth();

				//PE: Select on mouse release.
				if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(0))
				{
					//PE: Find object in scene. and move camera. and select to cursor.
					if (t.entityelement[i].obj > 0)
					{
						t.widget.pickedEntityIndex = i;
						t.widget.pickedObject = t.entityelement[t.widget.pickedEntityIndex].obj;
						g.entityrubberbandlist.clear();
						bEditorInFreeFlightMode = true; //PE: Must be in freeflight mode.
						t.editorfreeflight.mode = 1;
						int group = isEntityInGroupList(t.widget.pickedEntityIndex);
						if (group >= 0)
						{
							//PE: Add all groups with entity to rubberband.
							CheckGroupListForRubberbandSelections(t.widget.pickedEntityIndex);
						}
						if (bMoveCameraToObjectPosition == true && bAutoGenObject == false)
						{
							float zoom = ObjectSize(t.entityelement[i].obj, 1) * 2.0;
							if (zoom < 30.0f) zoom = 30.0f;
							float realcamy = ObjectSizeY(t.entityelement[i].obj, 1) * 0.75;
							float camy = realcamy;
							if (camy < 30.0f) camy = 30.0f;
							if (t.entityprofile[masterid].ismarker > 0)
							{
								zoom = 100.0;
								camy = 50.0;
							}
							//PE: Move camera keep camera Y.
							PositionCamera(t.entityelement[i].x, t.entityelement[i].y, t.entityelement[i].z);
							PointCamera(t.entityelement[i].x, t.entityelement[i].y, t.entityelement[i].z);
							MoveCamera(0, -zoom);
							PositionCamera(CameraPositionX(0), t.entityelement[i].y + camy, CameraPositionZ(0));
							PointCamera(t.entityelement[i].x, t.entityelement[i].y + (realcamy * 0.5), t.entityelement[i].z);
							t.editorfreeflight.c.x_f = CameraPositionX();
							t.editorfreeflight.c.y_f = CameraPositionY();
							t.editorfreeflight.c.z_f = CameraPositionZ();
							t.editorfreeflight.c.angx_f = CameraAngleX();
							t.editorfreeflight.c.angy_f = CameraAngleY();
							t.cx_f = t.editorfreeflight.c.x_f;
							t.cy_f = t.editorfreeflight.c.z_f;
						}
					}
				}
				if (TreeNodeOpen)
				{
					ImGui::TreePop();
				}
			}
		}
	}
	return(0);
}

void SetupDecalObject(int obj, int elementID)
{
	bool bUseFPE = false;
	if (elementID > 0 && t.entityelement[elementID].eleprof.bUseFPESettings)
		bUseFPE = true;

	if (bUseFPE)
	{
		SetObjectTransparency(obj, 6);
		SetObjectLight(obj, 0);
	}
	sObject* pObject = g_ObjectList[obj];
	if (pObject)
	{
		//PE: SetObjectCull(t.tobj, 1); Dont work.
		//PE: iCullMode need to be zero in wicked ?

		if (bUseFPE)
		{
			for (int iMesh = 0; iMesh < pObject->iMeshCount; iMesh++)
			{
				if (pObject->ppMeshList[iMesh]) pObject->ppMeshList[iMesh]->iCullMode = 0;
			}
			WickedCall_SetObjectCullmode(pObject);
			WickedCall_SetObjectCastShadows(pObject, false); //PE: No shadows on particles for now.
		}
		

		if(bUseFPE) //!t.entityelement[elementID].eleprof.bCustomWickedMaterialActive) // ZJ: Only reset this if not using custom materials for this decal.
		{
			//PE: Use unlit shader.
			for (int iMesh = 0; iMesh < pObject->iMeshCount; iMesh++)
			{
				sMesh* pMesh = pObject->ppMeshList[iMesh];
				if (pMesh)
				{
					if (bUseFPE)
					{
						pMesh->mMaterial.Diffuse.r = 1.5;
						pMesh->mMaterial.Diffuse.g = 1.5;
						pMesh->mMaterial.Diffuse.b = 1.5;
						pMesh->mMaterial.Diffuse.a = 1.0;

						pMesh->mMaterial.Emissive.r = 1.0;
						pMesh->mMaterial.Emissive.g = 1.0;
						pMesh->mMaterial.Emissive.b = 1.0;
						pMesh->mMaterial.Emissive.a = 1.0;
					}
					wiScene::MeshComponent* mesh = wiScene::GetScene().meshes.GetComponent(pMesh->wickedmeshindex);
					if (mesh)
					{
						uint64_t materialEntity = mesh->subsets[0].materialID;
						wiScene::MaterialComponent* pObjectMaterial = wiScene::GetScene().materials.GetComponent(materialEntity);
						if (pObjectMaterial)
						{
							if (bUseFPE)
								pObjectMaterial->SetReflectance(0.0f);
							pObjectMaterial->shaderType = wiScene::MaterialComponent::SHADERTYPE_UNLIT; //PE: Yes 1:1 mapping and no light,env...
							pObjectMaterial->SetDirty(true);
						}
					}
					WickedCall_SetMeshMaterial(pMesh,true);
					if (elementID > 0)
					{
						WickedCall_SetMeshAlpha(pMesh, t.entityelement[elementID].fDecalOpacity * 100.0);
					}
				}
			}
		}
	}
}

// Sets the contents of a file in local app data with the path to the writable area.
// Used by the Updater to backup the writable area.
void SetUpdaterWritePathFile(char* sContents)
{
	cstr sUpdaterWritePath = defaultWriteFolder;
	sUpdaterWritePath += "writepath.ini";
	if (FileOpen(1)) CloseFile(1);
	if (FileExist(sUpdaterWritePath.Get())) DeleteAFile(sUpdaterWritePath.Get());
	OpenToWrite(1, sUpdaterWritePath.Get());
	WriteString(1, sContents);
	CloseFile(1);
}

void loadMarketplaceData(int* ggMaxDlc, cstr* ggMaxLink, int* sketchfabDlc, cstr* sketchfabLink, int* shockwaveDlc, cstr* shockwaveLink, int* communityDlc, cstr* communityLink, int* gcStoreDlc, cstr* gcStoreImageURL, cstr* gcStoreLink)
{
	std::ifstream fileRead;
	nlohmann::json jsonFile;
	int numOfPromoItems = 8;

	fileRead.open("editors\\marketplace\\MarketplaceData.json");
	if (fileRead.is_open())
	{
		fileRead >> jsonFile;
		numOfPromoItems = jsonFile["numberOfPromotionalItems"];
		std::string mainDir = jsonFile["mainMarketplaceDirectory"];
		std::string imageName = "";
		std::string link = "";
		std::string fullImageDir = "";

		//Have to convert strings to chars for LoadImage() to take it in
		char converter[1024];

		for (int i = 0; i < numOfPromoItems; i++)
		{
			//GG MAX DLC
			//Thumb
			ggMaxDlc[i] = MARKETPLACE_ICONS + i;
			imageName = jsonFile["ggMaxDLC"][i]["imageName"];
			fullImageDir = mainDir + imageName;
			strcpy(converter, fullImageDir.c_str());
			LoadImage(converter, ggMaxDlc[i]);

			link = jsonFile["ggMaxDLC"][i]["steamLink"]; 
			strcpy(converter, link.c_str());
			ggMaxLink[i] = converter;

			//Sketchfab DLC
			//Thumb
			sketchfabDlc[i] = MARKETPLACE_ICONS + i + numOfPromoItems;
			imageName = jsonFile["sketchfabDLC"][i]["imageName"];
			fullImageDir = mainDir + imageName;
			strcpy(converter, fullImageDir.c_str());
			LoadImage(converter, sketchfabDlc[i]);

			//Link
			link = jsonFile["sketchfabDLC"][i]["websiteLink"];
			strcpy(converter, link.c_str());
			sketchfabLink[i] = converter;

			//Shockwave DLC
			//Thumb
			shockwaveDlc[i] = MARKETPLACE_ICONS + i + (numOfPromoItems * 2);
			imageName = jsonFile["shockwaveDLC"][i]["imageName"];
			fullImageDir = mainDir + imageName;
			strcpy(converter, fullImageDir.c_str());
			LoadImage(converter, shockwaveDlc[i]);

			//Link
			link = jsonFile["shockwaveDLC"][i]["websiteLink"];
			strcpy(converter, link.c_str());
			shockwaveLink[i] = converter;

			//Community DLC
			//Thumb
			communityDlc[i] = MARKETPLACE_ICONS + i + (numOfPromoItems * 3);
			imageName = jsonFile["communityDLC"][i]["imageName"];
			fullImageDir = mainDir + imageName;
			strcpy(converter, fullImageDir.c_str());
			LoadImage(converter, communityDlc[i]);

			//Link
			link = jsonFile["communityDLC"][i]["websiteLink"];
			strcpy(converter, link.c_str());
			communityLink[i] = converter;

			//Game Creator Store DLC
			gcStoreDlc[i] = MARKETPLACE_ICONS + i + (numOfPromoItems * 4);
		}
	}
	fileRead.close();

	// additional system can replace the fixed 'Game Creator Store DLC' if a live connection can be had
	bool bUpdateGameCreatorStoreLive = true;
	if (bUpdateGameCreatorStoreLive)
	{
		// needed data reserve
		char pDataReturned[132000];
		char pDatatmp[132000];
		char cUrl[10240];

		// list we need
		std::vector<cstr> ItemName;
		std::vector<cstr> ItemURL;
		std::vector<cstr> ItemImage;

		// call API to get featured items in list
		memset(pDataReturned, 0, sizeof(pDataReturned));
		memset(pDatatmp, 0, sizeof(pDatatmp));
		DWORD dwDataReturnedSize = 0;
		sprintf(cUrl, "/api/max/featured/products");

		// access features list from store server
		UINT iError = StoreOpenURLForDataOrFile(NULL, pDataReturned, &dwDataReturnedSize, "", "GET", cUrl, NULL);
		if (iError <= 0 && *pDataReturned != 0 && strchr(pDataReturned, '{') != 0)
		{
			char* pChop = NULL;
			if ((pChop = (char *)pestrcasestr(pDataReturned, "{\"status\":\"success\"")) != NULL)
			{
				// data we need from each entry
				char pItemName[1024];
				char pStoreURL[1024];
				char pThumbURL[10240];

				// go through all items until no more names
				while (ItemName.size() < 8)
				{
					int iResultCount = 0;
					char pSearchForToken[256];
					strcpy(pSearchForToken, "\"name\"\0");
					if (pChop)
					{
						pChop = strstr(pChop, pSearchForToken);
						if (pChop)
						{
							pChop += strlen(pSearchForToken) + 2;
							strcpy(pDatatmp, pChop);
							char* pFindEnd = strstr(pDatatmp, "\"\0");
							if (pFindEnd)
							{
								pDatatmp[pFindEnd - pDatatmp] = 0;
								if (strlen(pDatatmp) < 256)
								{
									strcpy(pItemName, pDatatmp);
									iResultCount++;
								}
								pChop = pFindEnd + 1;
							}
							strcpy(pSearchForToken, "\"thumbnail_url\"\0");
							pChop = strstr(pChop, pSearchForToken);
							if (pChop)
							{
								pChop += strlen(pSearchForToken) + 2;
								strcpy(pDatatmp, pChop);
								char* pFindEnd = strstr(pDatatmp, "\"\0");
								pDatatmp[pFindEnd - pDatatmp] = 0;
								if (strlen(pDatatmp) < 10240)
								{
									strcpy(pThumbURL, pDatatmp);
									iResultCount++;
								}
								pChop = pFindEnd + 1;
							}
							if (pChop)
							{
								strcpy(pSearchForToken, "\"store_url\"\0");
								pChop = strstr(pChop, pSearchForToken);
								if (pChop)
								{
									pChop += strlen(pSearchForToken) + 2;
									strcpy(pDatatmp, pChop);
									char* pFindEnd = strstr(pDatatmp, "\"\0");
									pDatatmp[pFindEnd - pDatatmp] = 0;
									if (strlen(pDatatmp) < 256)
									{
										strcpy(pStoreURL, pDatatmp);
										iResultCount++;
									}
									pChop = pFindEnd + 1;
								}
							}
							if (iResultCount == 3)
							{
								// add results if valid
								ItemName.push_back(pItemName);
								ItemURL.push_back(pStoreURL);
								ItemImage.push_back(pThumbURL);
							}
						}
						else
						{
							// no more item names, leave now
							break;
						}
					}
					else
					{
						// less than eight
						break;
					}
				}
			}
		}

		// go through ItemImage list and fill gcStoreImageURL
		int iMaximumOfEight = ItemImage.size();
		if (iMaximumOfEight > 8) iMaximumOfEight = 8;
		for (int i = 0; i < iMaximumOfEight; i++)
		{
			// ItemName not carried back
			gcStoreLink[i] = ItemURL[i] + "?r=tgc";
			gcStoreImageURL[i] = ItemImage[i];
		}
	}

}


int current_icon_set = -1;
bool bTriggerIconSetChange = false;

void SetIconSet(bool bInstant)
{
	//PE: This need to be done as the first, or we could change a image that is already on screen (crash).
	bTriggerIconSetChange = true;
	if (bInstant) SetIconSetCheck(bInstant);
}
void SetIconSetCheck(bool bInstant)
{
	if (bInstant == false && !bTriggerIconSetChange) return;
	
	bTriggerIconSetChange = false;

	//PE: These should be change when changing style.
	if (current_icon_set != pref.current_style)
	{
		SetMipmapNum(1); //PE: mipmaps not needed.
		image_setlegacyimageloading(true);
		if (pref.current_style == 25 || pref.current_style == 3)
		{
			LoadImage("editors\\uiv3\\entity_particle.png", ENTITY_PARTICLE);
			LoadImage("editors\\uiv3\\entity_light.png", ENTITY_LIGHT);
			LoadImage("editors\\uiv3\\entity_probe.png", ENTITY_PROBE);
			LoadImage("editors\\uiv3\\entity_cover.png", ENTITY_COVER);
			LoadImage("editors\\uiv3\\entity_win.png", ENTITY_WIN);
			LoadImage("editors\\uiv3\\entity_image.png", ENTITY_IMAGE);
			LoadImage("editors\\uiv3\\entity_music.png", ENTITY_MUSIC);
			LoadImage("editors\\uiv3\\entity_sound.png", ENTITY_SOUND);
			LoadImage("editors\\uiv3\\entity_text.png", ENTITY_TEXT);
			LoadImage("editors\\uiv3\\entity_video.png", ENTITY_VIDEO);
			LoadImage("editors\\uiv3\\entity_start.png", ENTITY_START);
			LoadImage("editors\\uiv3\\entity_checkpoint.png", ENTITY_CHECKPOINT); //
			LoadImage("editors\\uiv3\\shooter_flag.png", ENTITY_FLAG); //
			LoadImage("editors\\uiv3\\shooter_guns.png", ENTITY_GUNS); // Not used anymore?
			LoadImage("editors\\uiv3\\shooter_ammo.png", ENTITY_AMMO); // Not used anymore?
			LoadImage("editors\\uiv3\\shooter_enemies.png", ENTITY_ENEMIES); // Not used anymore?
			LoadImage("editors\\uiv3\\shooter_allies.png", ENTITY_ALLIES); // Not used anymore?
			LoadImage("editors\\uiv3\\entity_triggerzone.png", ENTITY_TRIGGERZONE);
			LoadImage("editors\\uiv3\\entity_behavior.png", ENTITY_BEHAVIOR);
			
			if (FileExist("editors\\uiv3\\ccp2-hat.png"))
				LoadImage("editors\\uiv3\\ccp2-hat.png", CCP_HAT);
			else
				LoadImage("editors\\uiv3\\ccp-hat.png", CCP_HAT);
			if (FileExist("editors\\uiv3\\ccp2-feet.png"))
				LoadImage("editors\\uiv3\\ccp2-feet.png", CCP_FEET);
			else
				LoadImage("editors\\uiv3\\ccp-feet.png", CCP_FEET);
			if (FileExist("editors\\uiv3\\ccp2-legs.png"))
				LoadImage("editors\\uiv3\\ccp2-legs.png", CCP_LEGS);
			else
				LoadImage("editors\\uiv3\\ccp-legs.png", CCP_LEGS);
			if (FileExist("editors\\uiv3\\ccp2-body.png"))
				LoadImage("editors\\uiv3\\ccp2-body.png", CCP_BODY);
			else
				LoadImage("editors\\uiv3\\ccp-body.png", CCP_BODY);
			if (FileExist("editors\\uiv3\\ccp2-glasses.png"))
				LoadImage("editors\\uiv3\\ccp2-glasses.png", CCP_GLASSES);
			else
				LoadImage("editors\\uiv3\\ccp-glasses.png", CCP_GLASSES);
			if (FileExist("editors\\uiv3\\ccp2-beard.png"))
				LoadImage("editors\\uiv3\\ccp2-beard.png", CCP_BEARD);
			else
				LoadImage("editors\\uiv3\\ccp-beard.png", CCP_BEARD);
			if (FileExist("editors\\uiv3\\ccp2-hair.png"))
				LoadImage("editors\\uiv3\\ccp2-hair.png", CCP_HAIR);
			else
				LoadImage("editors\\uiv3\\ccp-hair.png", CCP_HAIR);
			if (FileExist("editors\\uiv3\\ccp2-head.png"))
				LoadImage("editors\\uiv3\\ccp2-head.png", CCP_HEAD);
			else
				LoadImage("editors\\uiv3\\ccp-head.png", CCP_HEAD);
			if (FileExist("editors\\uiv3\\ccp2-tattoo.png"))
				LoadImage("editors\\uiv3\\ccp2-tattoo.png", CCP_TATTOO);
			else
				LoadImage("editors\\uiv3\\ccp-tattoo.png", CCP_TATTOO);
			LoadImage("editors\\uiv3\\ccp2-accessory_one.png", CCP_ACCESSORY1);
			LoadImage("editors\\uiv3\\ccp2-accessory_two.png", CCP_ACCESSORY2);

			LoadImage("editors\\uiv3\\filetype-script.png", FILETYPE_SCRIPT);

			LoadImage("editors\\uiv3\\blue-eye-on.png", ENTITY_EYE_ON);
			LoadImage("editors\\uiv3\\blue-eye-off.png", ENTITY_EYE_OFF);
		}
		else
		{
			LoadImage("editors\\uiv3\\entity_particle2.png", ENTITY_PARTICLE);
			LoadImage("editors\\uiv3\\entity_light2.png", ENTITY_LIGHT);
			LoadImage("editors\\uiv3\\entity_probe2.png", ENTITY_PROBE);
			LoadImage("editors\\uiv3\\entity_cover2.png", ENTITY_COVER);
			LoadImage("editors\\uiv3\\entity_win2.png", ENTITY_WIN);
			LoadImage("editors\\uiv3\\entity_image2.png", ENTITY_IMAGE);
			LoadImage("editors\\uiv3\\entity_music2.png", ENTITY_MUSIC);
			LoadImage("editors\\uiv3\\entity_sound2.png", ENTITY_SOUND);
			LoadImage("editors\\uiv3\\entity_text2.png", ENTITY_TEXT);
			LoadImage("editors\\uiv3\\entity_video2.png", ENTITY_VIDEO);
			LoadImage("editors\\uiv3\\entity_start2.png", ENTITY_START);
			LoadImage("editors\\uiv3\\entity_checkpoint2.png", ENTITY_CHECKPOINT); //
			LoadImage("editors\\uiv3\\shooter_flag2.png", ENTITY_FLAG); //
			LoadImage("editors\\uiv3\\shooter_guns2.png", ENTITY_GUNS);
			LoadImage("editors\\uiv3\\shooter_ammo2.png", ENTITY_AMMO);
			LoadImage("editors\\uiv3\\shooter_enemies2.png", ENTITY_ENEMIES);
			LoadImage("editors\\uiv3\\shooter_allies2.png", ENTITY_ALLIES);
			LoadImage("editors\\uiv3\\entity_triggerzone2.png", ENTITY_TRIGGERZONE);
			LoadImage("editors\\uiv3\\entity_behavior2.png", ENTITY_BEHAVIOR);

			if (FileExist("editors\\uiv3\\ccp2-hat2.png"))
				LoadImage("editors\\uiv3\\ccp2-hat2.png", CCP_HAT);
			else
				LoadImage("editors\\uiv3\\ccp-hat2.png", CCP_HAT);
			if (FileExist("editors\\uiv3\\ccp2-feet2.png"))
				LoadImage("editors\\uiv3\\ccp2-feet2.png", CCP_FEET);
			else
				LoadImage("editors\\uiv3\\ccp-feet2.png", CCP_FEET);
			if (FileExist("editors\\uiv3\\ccp2-legs2.png"))
				LoadImage("editors\\uiv3\\ccp2-legs2.png", CCP_LEGS);
			else
				LoadImage("editors\\uiv3\\ccp-legs2.png", CCP_LEGS);
			if (FileExist("editors\\uiv3\\ccp2-body2.png"))
				LoadImage("editors\\uiv3\\ccp2-body2.png", CCP_BODY);
			else
				LoadImage("editors\\uiv3\\ccp-body2.png", CCP_BODY);
			if (FileExist("editors\\uiv3\\ccp2-glasses2.png"))
				LoadImage("editors\\uiv3\\ccp2-glasses2.png", CCP_GLASSES);
			else
				LoadImage("editors\\uiv3\\ccp-glasses2.png", CCP_GLASSES);
			if (FileExist("editors\\uiv3\\ccp2-beard2.png"))
				LoadImage("editors\\uiv3\\ccp2-beard2.png", CCP_BEARD);
			else
				LoadImage("editors\\uiv3\\ccp-beard2.png", CCP_BEARD);
			if (FileExist("editors\\uiv3\\ccp2-hair2.png"))
				LoadImage("editors\\uiv3\\ccp2-hair2.png", CCP_HAIR);
			else
				LoadImage("editors\\uiv3\\ccp-hair2.png", CCP_HAIR);
			if (FileExist("editors\\uiv3\\ccp2-head2.png"))
				LoadImage("editors\\uiv3\\ccp2-head2.png", CCP_HEAD);
			else
				LoadImage("editors\\uiv3\\ccp-head2.png", CCP_HEAD);
			if (FileExist("editors\\uiv3\\ccp2-tattoo2.png"))
				LoadImage("editors\\uiv3\\ccp2-tattoo2.png", CCP_TATTOO);
			else
				LoadImage("editors\\uiv3\\ccp-tattoo2.png", CCP_TATTOO);
			LoadImage("editors\\uiv3\\ccp2-accessory_one2.png", CCP_ACCESSORY1);
			LoadImage("editors\\uiv3\\ccp2-accessory_two2.png", CCP_ACCESSORY2);

			LoadImage("editors\\uiv3\\filetype-script2.png", FILETYPE_SCRIPT);

			LoadImage("editors\\uiv3\\gray-eye-on.png", ENTITY_EYE_ON);
			LoadImage("editors\\uiv3\\gray-eye-off.png", ENTITY_EYE_OFF);

		}
		current_icon_set = pref.current_style;
		image_setlegacyimageloading(false);
		SetMipmapNum(-1);

		//PE: Mark all global Behaviors to update.
		for (t.e = 1; t.e <= g.entityelementlist; t.e++)
		{
			t.entid = t.entityelement[t.e].bankindex;
			if (t.entid > 0 && t.entityprofile[t.entid].ismarker == 12)
			{
				t.entityelement[t.e].eleprof.thumb_id = -1;
			}
		}
	}
	//----
}

int get_gameisexe(void)
{
	return(t.game.gameisexe);
}

int get_hidehudstate()
{
	return g.tabmodehidehuds;
}


#define STORYBOARD_INCLUDE_LOADGAME //PE: Not ready yet, also missing "in between game menu" graphics/music setup ...

#define STORYBOARD_SAVE_MESSAGE "Do you wish to save your game project first ?"
#define STORYBOARD_YSTART 30

//PE: Not needed in save struct.
int StoryboardiActiveLinksId[STORYBOARD_MAXNODES];
int StoryboardiActiveLinksIdFrom[STORYBOARD_MAXNODES];
int iLoadGameNodeID = 3;
int iTitleScreenNodeID = 1;
int iGamePausedNodeID = 8;
int iSaveGameNodeID = 9;
int iGraphicsNodeID = 10;
int iSoundsNodeID = 11;

int iControlNodeID = 12;

int iLoadingScreenNodeID = 2;
int iAboutScreenNodeID = 4;
int iGameWonScreenNodeID = 5;
int iGameLostScreenNodeID = 6;
int iHUDScreenNodeID = 13;

int get_output_linkindex(int node, int index)
{
	if (node < 0 || node > STORYBOARD_MAXNODES) return index;
	int i = node;
	int outlinknum = 0;
	if (i == iGamePausedNodeID) return index;
	if (i == iGraphicsNodeID) return index;
	if (i == iSoundsNodeID) return index;
	if (i == iControlNodeID) return index;
	if (i == iSaveGameNodeID) return index;
	if (i == iLoadGameNodeID) return index;
	if (i == iLoadingScreenNodeID) return index; //PE: Special got no button for linking to output.

	if (Storyboard.Nodes[i].type == STORYBOARD_TYPE_SPLASH) return index;
	if (Storyboard.Nodes[i].type == STORYBOARD_TYPE_HUD) return index;

	for (int ll = 0; ll < STORYBOARD_MAXWIDGETS; ll++)
	{
		if (ll == index)
			return outlinknum;
		if (Storyboard.Nodes[i].widget_used[ll])
		{
			if (Storyboard.Nodes[node].widget_type[ll] == STORYBOARD_WIDGET_BUTTON)
			{
				if (Storyboard.Nodes[node].widget_action[ll] == STORYBOARD_ACTIONS_STARTGAME || Storyboard.Nodes[node].widget_action[ll] == STORYBOARD_ACTIONS_GOTOLEVEL)
				{
					outlinknum++;
				}
				else if (Storyboard.Nodes[node].widget_action[ll] == STORYBOARD_ACTIONS_GOTOSCREEN)
				{
					outlinknum++;
				}
			}
		}
	}

	return(index);
}

void setup_output_links(int node)
{
	if (node < 0 || node > STORYBOARD_MAXNODES) return;
	int i = node;
	int outlinknum = 0;
	char chr[MAX_PATH];
	if (i == iGamePausedNodeID) return;
	if (i == iGraphicsNodeID) return;
	if (i == iSoundsNodeID) return;
	if (i == iControlNodeID) return;
	if (i == iSaveGameNodeID) return;
	if (i == iLoadGameNodeID) return;
	if (i == iLoadingScreenNodeID) return; //PE: Special got no button for linking to output.

	if (Storyboard.Nodes[i].type == STORYBOARD_TYPE_SPLASH) return;
	if (Storyboard.Nodes[i].type == STORYBOARD_TYPE_HUD) return;


	//PE: reset outlinks.
	for (int l = 0; l < STORYBOARD_MAXOUTPUTS; l++)
	{
		strcpy(Storyboard.Nodes[i].output_action[l], "");
		strcpy(Storyboard.Nodes[i].output_title[l], "");
		Storyboard.Nodes[i].output_can_link_to_type[l] = 0;
	}

	for (int ll = 0; ll < STORYBOARD_MAXWIDGETS; ll++)
	{
		if (Storyboard.Nodes[node].widget_used[ll])
		{
			if (Storyboard.Nodes[node].widget_type[ll] == STORYBOARD_WIDGET_BUTTON)
			{
				if (Storyboard.Nodes[node].widget_action[ll] == STORYBOARD_ACTIONS_STARTGAME || Storyboard.Nodes[node].widget_action[ll] == STORYBOARD_ACTIONS_GOTOLEVEL)
				{
					strcpy(chr, Storyboard.Nodes[node].widget_label[ll]);
					strcat(chr, " -> Connect to Level");
					strcpy(Storyboard.Nodes[node].output_title[outlinknum], chr);
					strcpy(Storyboard.Nodes[node].output_action[outlinknum], "loadlevel"); //Not defined this yet.
					Storyboard.Nodes[node].output_can_link_to_type[outlinknum] = STORYBOARD_TYPE_LEVEL;
					outlinknum++;
				}
				else if (Storyboard.Nodes[node].widget_action[ll] == STORYBOARD_ACTIONS_GOTOSCREEN)
				{
					strcpy(chr, Storyboard.Nodes[node].widget_label[ll]);
					strcat(chr, " ->  Connect to Scene ");
					strcpy(Storyboard.Nodes[node].output_title[outlinknum], chr);
					strcpy(Storyboard.Nodes[node].output_action[outlinknum], "loadscene"); //Not defined this yet.
					Storyboard.Nodes[node].output_can_link_to_type[outlinknum] = STORYBOARD_TYPE_SCREEN;
					outlinknum++;
				}
			}
			if (pestrcasestr(Storyboard.Nodes[node].lua_name, "loading"))
			{
				strcpy(Storyboard.Nodes[node].output_title[0], " LOAD LEVEL -> Connect to Level ");
				strcpy(Storyboard.Nodes[node].output_action[0], "loadlevel"); //Not defined this yet.
				Storyboard.Nodes[node].output_can_link_to_type[0] = STORYBOARD_TYPE_LEVEL;
				outlinknum++;
			}
		}
	}
}

void reset_single_node_interscreen(int node)
{
	// this is called by restore to reset all 'screen elements' without affecting wider relationships (i.e. connections to nodes)
	int i = node;

	//Screen
	strcpy(Storyboard.Nodes[i].screen_title, "");
	strcpy(Storyboard.Nodes[i].screen_music, "");
	Storyboard.Nodes[i].screen_grid_size = 0;
	strcpy(Storyboard.Nodes[i].screen_backdrop, "");
	Storyboard.Nodes[i].screen_back_color = ImVec4(0.0, 0.0, 0.0, 1.0);
	Storyboard.Nodes[i].screen_backdrop_placement = 2; // center,stretch,zoom. PE: Default to zoom.
	strcpy(Storyboard.Nodes[i].screen_thumb, "");
	for (int ll = 0; ll < 10; ll++)
		Storyboard.Nodes[i].screen_backdrop_ratio_placement[ll] = 0; // 0=1920x1080 center,stretch,zoom. 1=1366x768 center,stretch,zoom ...

	//Editor.
	for (int ll = 0; ll < STORYBOARD_MAXWIDGETS; ll++)
	{
		Storyboard.Nodes[i].widget_used[ll] = 0;
		strcpy(Storyboard.Nodes[i].widget_label[ll], "");
		Storyboard.Nodes[i].widget_size[ll] = ImVec2(1.0, 1.0); // size zoom.
		Storyboard.Nodes[i].widget_pos[ll] = ImVec2(0, 0);
		strcpy(Storyboard.Nodes[i].widget_normal_thumb[ll], "");
		strcpy(Storyboard.Nodes[i].widget_highlight_thumb[ll], "");
		strcpy(Storyboard.Nodes[i].widget_selected_thumb[ll], ""); //only for state change button, checkbox ...
		strcpy(Storyboard.Nodes[i].widget_click_sound[ll], "");
		Storyboard.Nodes[i].widget_action[ll] = 0; // 0=none ...
		strcpy(Storyboard.Nodes[i].widget_font[ll], "Default Font");
		Storyboard.Nodes[i].widget_font_color[ll] = ImVec4(1.0, 1.0, 1.0, 1.0);
		Storyboard.Nodes[i].widget_font_size[ll] = 1.0;
		Storyboard.Nodes[i].widget_type[ll] = 0; // 0=none,but,text,image,video...
		Storyboard.Nodes[i].widget_layer[ll] = 0;
		Storyboard.Nodes[i].widget_initial_value[ll] = 0;
		strcpy(Storyboard.Nodes[i].widget_name[ll], "");
		Storyboard.Nodes[i].widget_read_only[ll] = 0; //off,level,screen.
		Storyboard.NodeSliderValues[i][ll] = 0.0;
	}

	// used by GRAPHICS SETTINGS (1,2,3)
	Storyboard.NodeRadioButtonSelected[i] = -1;
}

void reset_single_node(int node)
{
	int i = node;
	//PE: Dont touch id's they are reused.
	Storyboard.Nodes[i].used = false;
	Storyboard.Nodes[i].type = 0;
	if (ImageExist(Storyboard.Nodes[i].thumb_id)) DeleteImage(Storyboard.Nodes[i].thumb_id);
	Storyboard.Nodes[i].restore_position = ImVec2(0, 0);
	Storyboard.Nodes[i].iEditEnable = true;
	strcpy(Storyboard.Nodes[i].title, "");
	strcpy(Storyboard.Nodes[i].levelnumber, "");
	strcpy(Storyboard.Nodes[i].thumb, "");
	strcpy(Storyboard.Nodes[i].lua_name, "");
	strcpy(Storyboard.Nodes[i].level_name, "");

	//Each filler have its own, so can count it down later.
	Storyboard.Nodes[i].screen_backdrop_transparent = false;
	Storyboard.Nodes[i].loop_music = 0;
	for (int l = 0; l < 19; l++) Storyboard.Nodes[i].iFiller20[l] = 0;
	for (int l = 0; l < 20; l++) Storyboard.Nodes[i].fFiller20[l] = 0.0;
	for (int l = 0; l < STORYBOARD_MAXOUTPUTS; l++)
	{
		for (int ll = 0; ll < 19; ll++) Storyboard.Nodes[i].iFillerMaxOutputs20[ll][l] = 0.0;
		for (int ll = 0; ll < 20; ll++) strcpy(Storyboard.Nodes[i].FillerCharMaxOutput20[ll][l],"");

#ifdef EMULATERESOLUTION
		Storyboard.Nodes[i].universal_resolution[l] = 0;
#endif

		strcpy(Storyboard.Nodes[i].output_action[l], "");
		strcpy(Storyboard.Nodes[i].output_title[l], "");
		Storyboard.Nodes[i].output_linkto[l] = 0;
		Storyboard.Nodes[i].output_can_link_to_type[l] = 0;
		strcpy(Storyboard.Nodes[i].input_title[l], "");
		strcpy(Storyboard.Nodes[i].input_action[l], "");
	}

	//Screen and Editor
	reset_single_node_interscreen(i);
}

void duplicate_single_node (int sourceid)
{
	// check if source valid
	if (sourceid < 0 || sourceid > STORYBOARD_MAXNODES) return;

	// create a new blank HUD screen correctly formatted
	extern int process_createanewhudscreen(int iStartAt);
	int iNewNode = process_createanewhudscreen(10);
	if (iNewNode < 0) return;

	// position next to source so know where duplicate is
	Storyboard.Nodes[iNewNode].restore_position = Storyboard.Nodes[sourceid].restore_position + ImVec2(20, 20);
	ImNodes::SetNodeGridSpacePos(Storyboard.Nodes[iNewNode].id, Storyboard.Nodes[iNewNode].restore_position);
	strcpy(Storyboard.Nodes[iNewNode].lua_name, Storyboard.Nodes[sourceid].lua_name);

	// data chunk one
	memcpy(Storyboard.Nodes[iNewNode].screen_title, Storyboard.Nodes[sourceid].screen_title, sizeof(Storyboard.Nodes[sourceid].screen_title));
	memcpy(Storyboard.Nodes[iNewNode].screen_music, Storyboard.Nodes[sourceid].screen_music, sizeof(Storyboard.Nodes[sourceid].screen_music));
	memcpy(Storyboard.Nodes[iNewNode].screen_backdrop, Storyboard.Nodes[sourceid].screen_backdrop, sizeof(Storyboard.Nodes[sourceid].screen_backdrop));
	Storyboard.Nodes[iNewNode].screen_back_color = Storyboard.Nodes[sourceid].screen_back_color;
	Storyboard.Nodes[iNewNode].screen_backdrop_placement = Storyboard.Nodes[sourceid].screen_backdrop_placement;
	memcpy(Storyboard.Nodes[iNewNode].screen_thumb, Storyboard.Nodes[sourceid].screen_thumb, sizeof(Storyboard.Nodes[sourceid].screen_thumb));
	memcpy(Storyboard.Nodes[iNewNode].screen_backdrop_ratio_placement, Storyboard.Nodes[sourceid].screen_backdrop_ratio_placement, sizeof(Storyboard.Nodes[sourceid].screen_backdrop_ratio_placement));
	Storyboard.Nodes[iNewNode].screen_grid_size = Storyboard.Nodes[sourceid].screen_grid_size;
	memcpy(Storyboard.Nodes[iNewNode].widget_used, Storyboard.Nodes[sourceid].widget_used, sizeof(Storyboard.Nodes[sourceid].widget_used));
	memcpy(Storyboard.Nodes[iNewNode].widget_label, Storyboard.Nodes[sourceid].widget_label, sizeof(Storyboard.Nodes[sourceid].widget_label));
	memcpy(Storyboard.Nodes[iNewNode].widget_size, Storyboard.Nodes[sourceid].widget_size, sizeof(Storyboard.Nodes[sourceid].widget_size));
	memcpy(Storyboard.Nodes[iNewNode].widget_pos, Storyboard.Nodes[sourceid].widget_pos, sizeof(Storyboard.Nodes[sourceid].widget_pos));
	memcpy(Storyboard.Nodes[iNewNode].widget_normal_thumb, Storyboard.Nodes[sourceid].widget_normal_thumb, sizeof(Storyboard.Nodes[sourceid].widget_normal_thumb));
	memcpy(Storyboard.Nodes[iNewNode].widget_highlight_thumb, Storyboard.Nodes[sourceid].widget_highlight_thumb, sizeof(Storyboard.Nodes[sourceid].widget_highlight_thumb));
	memcpy(Storyboard.Nodes[iNewNode].widget_selected_thumb, Storyboard.Nodes[sourceid].widget_selected_thumb, sizeof(Storyboard.Nodes[sourceid].widget_selected_thumb));
	memcpy(Storyboard.Nodes[iNewNode].widget_click_sound, Storyboard.Nodes[sourceid].widget_click_sound, sizeof(Storyboard.Nodes[sourceid].widget_click_sound));
	memcpy(Storyboard.Nodes[iNewNode].widget_action, Storyboard.Nodes[sourceid].widget_action, sizeof(Storyboard.Nodes[sourceid].widget_action));
	memcpy(Storyboard.Nodes[iNewNode].widget_font, Storyboard.Nodes[sourceid].widget_font, sizeof(Storyboard.Nodes[sourceid].widget_font));
	memcpy(Storyboard.Nodes[iNewNode].widget_font_color, Storyboard.Nodes[sourceid].widget_font_color, sizeof(Storyboard.Nodes[sourceid].widget_font_color));
	memcpy(Storyboard.Nodes[iNewNode].widget_font_size, Storyboard.Nodes[sourceid].widget_font_size, sizeof(Storyboard.Nodes[sourceid].widget_font_size));
	memcpy(Storyboard.Nodes[iNewNode].widget_type, Storyboard.Nodes[sourceid].widget_type, sizeof(Storyboard.Nodes[sourceid].widget_type));
	memcpy(Storyboard.Nodes[iNewNode].widget_read_only, Storyboard.Nodes[sourceid].widget_read_only, sizeof(Storyboard.Nodes[sourceid].widget_read_only));
	memcpy(Storyboard.Nodes[iNewNode].widget_layer, Storyboard.Nodes[sourceid].widget_layer, sizeof(Storyboard.Nodes[sourceid].widget_layer));
	memcpy(Storyboard.Nodes[iNewNode].widget_initial_value, Storyboard.Nodes[sourceid].widget_initial_value, sizeof(Storyboard.Nodes[sourceid].widget_initial_value));
	memcpy(Storyboard.Nodes[iNewNode].widget_name, Storyboard.Nodes[sourceid].widget_name, sizeof(Storyboard.Nodes[sourceid].widget_name));
	Storyboard.Nodes[iNewNode].screen_backdrop_transparent = Storyboard.Nodes[sourceid].screen_backdrop_transparent;
	Storyboard.Nodes[iNewNode].readouts_available = Storyboard.Nodes[sourceid].readouts_available;
	Storyboard.Nodes[iNewNode].widgets_available = Storyboard.Nodes[sourceid].widgets_available;
	Storyboard.Nodes[iNewNode].toggleKey = Storyboard.Nodes[sourceid].toggleKey;
	Storyboard.Nodes[iNewNode].showAtStart = Storyboard.Nodes[sourceid].showAtStart;
	Storyboard.Nodes[iNewNode].loop_music = Storyboard.Nodes[sourceid].loop_music;
	memcpy(Storyboard.Nodes[iNewNode].iFiller20, Storyboard.Nodes[sourceid].iFiller20, sizeof(Storyboard.Nodes[sourceid].iFiller20));
	memcpy(Storyboard.Nodes[iNewNode].fFiller20, Storyboard.Nodes[sourceid].fFiller20, sizeof(Storyboard.Nodes[sourceid].fFiller20));
	memcpy(Storyboard.Nodes[iNewNode].iFillerMaxOutputs20, Storyboard.Nodes[sourceid].iFillerMaxOutputs20, sizeof(Storyboard.Nodes[sourceid].iFillerMaxOutputs20));
	memcpy(Storyboard.Nodes[iNewNode].FillerCharMaxOutput20, Storyboard.Nodes[sourceid].FillerCharMaxOutput20, sizeof(Storyboard.Nodes[sourceid].FillerCharMaxOutput20));

#ifdef EMULATERESOLUTION
	memcpy(Storyboard.Nodes[iNewNode].universal_resolution, Storyboard.Nodes[sourceid].universal_resolution, sizeof(Storyboard.Nodes[sourceid].universal_resolution));
#endif

	// data chunk two
	memcpy(Storyboard.widget_colors[iNewNode], Storyboard.widget_colors[sourceid], sizeof(Storyboard.widget_colors[sourceid]));
	memcpy(Storyboard.widget_readout[iNewNode], Storyboard.widget_readout[sourceid], sizeof(Storyboard.widget_readout[sourceid]));
	memcpy(Storyboard.widget_textoffset[iNewNode], Storyboard.widget_textoffset[sourceid], sizeof(Storyboard.widget_textoffset[sourceid]));
	memcpy(Storyboard.widget_ingamehidden[iNewNode], Storyboard.widget_ingamehidden[sourceid], sizeof(Storyboard.widget_ingamehidden[sourceid]));
	memcpy(Storyboard.widget_drawordergroup[iNewNode], Storyboard.widget_drawordergroup[sourceid], sizeof(Storyboard.widget_drawordergroup[sourceid]));
}

void rename_single_node(int sourceid)
{
	// check if source valid
	if (sourceid < 0 || sourceid > STORYBOARD_MAXNODES) return;

	// trigger renaming of HUD Screen
	g_iRenameHUDScreenID = sourceid;
	strcpy (g_pRenameHUDName, Storyboard.Nodes[sourceid].title);
	strcpy (g_pRenameHUDScreenError, "");
}

void storeboard_fix_uniqueids( void )
{
	int iUniqueId = STORYBOARD_THUMBS;
	int iUniqueIdsAdd = 1000;
	for (int i = 0; i < STORYBOARD_MAXNODES; i++)
	{
		if (i == 100 || i == 200)
			iUniqueIdsAdd += 100000;

		int node = i;

		for (int l = 0; l < STORYBOARD_MAXOUTPUTS; l++)
		{
			//PE: input_id,output_id ID's broken in checkproject.
			if (Storyboard.Nodes[node].input_id[l] != iUniqueId + iUniqueIdsAdd + (1000 * l) ||
				Storyboard.Nodes[node].output_id[l] != iUniqueId + iUniqueIdsAdd + (1000 * l) + 500)
			{
				//PE: Something wrong reset.
				bool bInputChanged = false;
				if (Storyboard.Nodes[node].input_id[l] != iUniqueId + iUniqueIdsAdd + (1000 * l)) bInputChanged = true;
				int oldinput = Storyboard.Nodes[node].input_id[l];
				Storyboard.Nodes[node].input_id[l] = iUniqueId + iUniqueIdsAdd + (1000 * l);
				Storyboard.Nodes[node].output_id[l] = iUniqueId + iUniqueIdsAdd + (1000 * l) + 500;

				if (bInputChanged)
				{
					//PE: As the unique ids has changed we need to remove all linking to us, need to be done so corrupt projects can be fixed by reconnecting again.
					for (int ii = 0; ii < STORYBOARD_MAXNODES; ii++)
					{
						for (int a = 0; a < STORYBOARD_MAXOUTPUTS; a++)
						{
							//PE: Check all output_linkto and remove link.
							if (Storyboard.Nodes[ii].output_linkto[a] == oldinput)
								Storyboard.Nodes[ii].output_linkto[a] = 0;
						}
					}
				}
			}
		}
		if (pestrcasestr(Storyboard.Nodes[node].title, "Level "))
		{
			if (Storyboard.Nodes[node].type != STORYBOARD_TYPE_LEVEL)
				Storyboard.Nodes[node].type = STORYBOARD_TYPE_LEVEL;
		}
		if (pestrcasestr(Storyboard.Nodes[node].lua_name, "loading"))
		{
			if (Storyboard.Nodes[node].type != STORYBOARD_TYPE_LEVEL)
				Storyboard.Nodes[node].type = STORYBOARD_TYPE_LEVEL;
		}

		iUniqueId++;
	}
}

void storeboard_init_nodes(float area_width, float node_width, float node_height)
{
	if (bStoryboardInitNodes) return;
	bStoryboardInitNodes = true;
	iLoadGameNodeID = 3;
	iTitleScreenNodeID = 1;
	iGamePausedNodeID = 8;
	iSaveGameNodeID = 9;
	iGraphicsNodeID = 10;
	iSoundsNodeID = 11;
	iHUDScreenNodeID = 13;
	int iUniqueIds = STORYBOARD_THUMBS;
	strcpy(Storyboard.gamename,""); //Start with no name,
	Storyboard.iStoryboardVersion = STORYBOARDVERSION;
	Storyboard.iChanged = false;
	Storyboard.vEditorPanning = ImVec2(0.0f, 0.0f);
	strcpy(Storyboard.game_icon, "");
	strcpy(Storyboard.game_thumb, "");
	strcpy(Storyboard.game_description, "A game I made in GameGuru MAX");
	strcpy(Storyboard.game_world_edge_text, "You cannot leave the area of play");
	strcpy(Storyboard.game_developer_desc, "");
	Storyboard.project_readonly = 0;

	//PE: STORYBOARD_MAXNODES = 150. Largest unique id = 199749 , lowest = 49000.
	//PE: checkunique = { size=28500 }
	//PE: STORYBOARD_MAXNODES = 150. STORYBOARD_MAXOUTPUTS = 30. STORYBOARD_MAXWIDGETS = 100. Largest unique id = 249949 , lowest = 49000.
	//PE: checkunique = { size=54000 }
	//#define TESTUNIQUEIDS

	#ifdef TESTUNIQUEIDS
	std::vector<int> checkunique;
	#endif

	int iUniqueIdsAdd = 1000;
	for (int i = 0; i < STORYBOARD_MAXNODES; i++)
	{
		if (i == 100 || i == 200)
			iUniqueIdsAdd += 100000;
		reset_single_node(i);

		StoryboardiActiveLinksId[i] = 0;
		StoryboardiActiveLinksIdFrom[i] = 0;

		//PE: Setup Id's
		Storyboard.Nodes[i].id = iUniqueIds;
		Storyboard.Nodes[i].thumb_id = iUniqueIds;

		for (int l = 0; l < STORYBOARD_MAXOUTPUTS; l++)
		{
			Storyboard.Nodes[i].input_id[l] = iUniqueIds + iUniqueIdsAdd + (1000 * l);
			Storyboard.Nodes[i].output_id[l] = iUniqueIds + iUniqueIdsAdd + (1000 * l) + 500;
			#ifdef TESTUNIQUEIDS
			if (std::find(checkunique.begin(), checkunique.end(), Storyboard.Nodes[i].input_id[l]) != checkunique.end())
				printf("tmp");
			checkunique.push_back(Storyboard.Nodes[i].input_id[l]);
			if (std::find(checkunique.begin(), checkunique.end(), Storyboard.Nodes[i].output_id[l]) != checkunique.end())
				printf("tmp");
			checkunique.push_back(Storyboard.Nodes[i].output_id[l]);
			#endif
		}

		for (int l = 0; l < STORYBOARD_MAXWIDGETS; l++)
		{
			Storyboard.Nodes[i].widget_normal_thumb_id[l] = iUniqueIds + iUniqueIdsAdd + (1000 * l) + 600;
			Storyboard.Nodes[i].widget_highlight_thumb_id[l] = iUniqueIds + iUniqueIdsAdd + (1000 * l) + 700;
			Storyboard.Nodes[i].widget_selected_thumb_id[l] = iUniqueIds + iUniqueIdsAdd + (1000 * l) + 800;

			#ifdef TESTUNIQUEIDS
			if (std::find(checkunique.begin(), checkunique.end(), Storyboard.Nodes[i].widget_normal_thumb_id[l]) != checkunique.end())
				printf("tmp");
			checkunique.push_back(Storyboard.Nodes[i].widget_normal_thumb_id[l]);
			if (std::find(checkunique.begin(), checkunique.end(), Storyboard.Nodes[i].widget_highlight_thumb_id[l]) != checkunique.end())
				printf("tmp");
			checkunique.push_back(Storyboard.Nodes[i].widget_highlight_thumb_id[l]);
			if (std::find(checkunique.begin(), checkunique.end(), Storyboard.Nodes[i].widget_selected_thumb_id[l]) != checkunique.end())
				printf("tmp");
			checkunique.push_back(Storyboard.Nodes[i].widget_selected_thumb_id[l]);
			#endif
		}
		Storyboard.Nodes[i].screen_backdrop_id = iUniqueIds + 500;

		iUniqueIds++;
	}
	
	Storyboard.game_thumb_id = STORYBOARD_THUMBS + 420;
	Storyboard.game_icon_id = STORYBOARD_THUMBS + 421;

	// All default screens
	int node = 0;
	constexpr int allWidgets = ALLOW_BUTTON | ALLOW_TEXT | ALLOW_IMAGE | ALLOW_RADIOTYPE | ALLOW_SLIDER | ALLOW_TICKBOX | ALLOW_VIDEO | ALLOW_PROGRESS | ALLOW_TEXTAREA;

	//
	// 0 : Default splash screen.
	//
	storyboard_add_missing_nodex(node, area_width, node_width, node_height, true);
	node++;
	
	//
	// 1 : Default title screen
	//
	iTitleScreenNodeID = storyboard_add_missing_nodex(node, area_width, node_width, node_height, true);
	node++;

	//
	// 2 : iLoadingScreenNodeID
	//
	iLoadingScreenNodeID = storyboard_add_missing_nodex(node, area_width, node_width, node_height, true);
	node++;

	//
	// 3 : iLoadGameNodeID
	//
	iLoadGameNodeID = storyboard_add_missing_nodex(node, area_width, node_width, node_height, true);
	node++;

	//
	// 4 : Default About screen.
	//
	iAboutScreenNodeID = storyboard_add_missing_nodex(node, area_width, node_width, node_height, true);
	node++;

	//
	// 5 : Default Game Won screen.
	//
	iGameWonScreenNodeID = storyboard_add_missing_nodex(node, area_width, node_width, node_height, true);
	node++;

	//
	// 6 : Default Game Over screen.
	//
	iGameLostScreenNodeID = storyboard_add_missing_nodex(node, area_width, node_width, node_height, true);
	node++;

	//
	// 7 : Default Level1 screen.
	//
	int iLevelOne = storyboard_add_missing_nodex(node, area_width, node_width, node_height, true);
	Storyboard.Nodes[node].output_linkto[0] = Storyboard.Nodes[5].input_id[0];
	Storyboard.Nodes[node].output_linkto[1] = Storyboard.Nodes[6].input_id[0];
	Storyboard.Nodes[node].output_linkto[2] = 0;
	node++;

	//
	// 8 : iGamePausedNodeID
	//
	iGamePausedNodeID = storyboard_add_missing_nodex(node, area_width, node_width, node_height, true);
	node++;

	//
	// 9 : iSaveGameNodeID
	//
	iSaveGameNodeID = storyboard_add_missing_nodex(node, area_width, node_width, node_height, true);
	node++;

	//
	// 10 : iGraphicsNodeID
	//
	iGraphicsNodeID = storyboard_add_missing_nodex(node, area_width, node_width, node_height, true);
	node++;

	//
	// 11 : iSoundsNodeID
	//
	iSoundsNodeID = storyboard_add_missing_nodex(node, area_width, node_width, node_height, true);
	node++;

	//
	// 12 : iControlNodeID
	//
	iControlNodeID = storyboard_add_missing_nodex(node, area_width, node_width, node_height, true);
	node++;

	//
	// 13 : In-game HUD screen
	//
	iHUDScreenNodeID = storyboard_add_missing_nodex(node, area_width, node_width, node_height, true);
	node++;

	// Create node links now we know screen IDs
	Storyboard.Nodes[0].output_linkto[0] = Storyboard.Nodes[iTitleScreenNodeID].input_id[0];
	Storyboard.Nodes[iTitleScreenNodeID].output_linkto[0] = Storyboard.Nodes[iLoadingScreenNodeID].input_id[0];
	Storyboard.Nodes[iTitleScreenNodeID].output_linkto[1] = Storyboard.Nodes[iLoadGameNodeID].input_id[0];
	Storyboard.Nodes[iTitleScreenNodeID].output_linkto[2] = Storyboard.Nodes[iAboutScreenNodeID].input_id[0];
	Storyboard.Nodes[iGamePausedNodeID].output_linkto[0] = Storyboard.Nodes[iLoadGameNodeID].input_id[0];
	Storyboard.Nodes[iGamePausedNodeID].output_linkto[1] = Storyboard.Nodes[iSaveGameNodeID].input_id[0];
	Storyboard.Nodes[iGamePausedNodeID].output_linkto[2] = Storyboard.Nodes[iGraphicsNodeID].input_id[0];
	Storyboard.Nodes[iGamePausedNodeID].output_linkto[3] = Storyboard.Nodes[iSoundsNodeID].input_id[0];
	Storyboard.Nodes[iGamePausedNodeID].output_linkto[4] = 0;
	Storyboard.Nodes[iGamePausedNodeID].output_linkto[5] = 0;
	Storyboard.Nodes[iGamePausedNodeID].output_linkto[6] = 0;
	Storyboard.Nodes[iGamePausedNodeID].output_linkto[7] = Storyboard.Nodes[iControlNodeID].input_id[0];
	Storyboard.Nodes[iLoadingScreenNodeID].output_linkto[0] = Storyboard.Nodes[iLevelOne].input_id[0];

	//Make sure we have the needed folders
	char destination[MAX_PATH];
	strcpy(destination, "projectbank\\");
	GG_GetRealPath(destination, 1);
	MakeDirectory(destination);
}

