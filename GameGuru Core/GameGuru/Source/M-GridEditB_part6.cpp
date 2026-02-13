int iLastEmmisiveObject = 0;
bool bReadyToClickDot = false;

void MoveSelectedDotObject(void)
{
	//	extern uint64_t g_hovered_dot_entity;
	//	extern sObject* g_hovered_dot_pobject;

	bool bMiddleDotActive = false;
	if (!bDotObjectDragging && g_hovered_dot_pobject && g_hovered_dot_pobject->dwObjectNumber >= DOTMIDDLEOBJECTID)
	{
		bMiddleDotActive = true;
	}
	if(bDotMiddleWindow && !pref.iEnableRelationPopupWindow)
	{
		if (t.inputsys.mclick == 0)
			bReadyToClickDot = true;
		if (bReadyToClickDot && t.inputsys.mclick == 1)
			bDotMiddleWindow = false;
	}

	if (t.onedragmode == 1 && t.onedrag > 0)
		return;
	if (t.widget.pickedObject > 0)
		return;
	if (g.entityrubberbandlist.size() > 0)
		return;
	if (t.inputsys.rubberbandmode != 0)
		return;

	if (bMiddleDotActive) 
	{
		if (t.inputsys.mclick == 1 && !bDotMiddleWindow)
		{
			if (g_selected_middle_dot_pobject != g_hovered_dot_pobject)
			{
				ImVec2 wpos = ImGui::GetWindowPos();
				ImVec2 mpos = ImGui::GetMousePos();
				vDotMiddleWindowPos = mpos + ImVec2(10, 10);
			}
			g_selected_middle_dot_pobject = g_hovered_dot_pobject;
			int iId = (g_selected_middle_dot_pobject->dwObjectNumber - DOTMIDDLEOBJECTID);
			if (iId >= 0 && iId < MAXDOTMIDDLE)
			{
				bDotMiddleWindow = true;
				bReadyToClickDot = false;
				if (pref.iEnableRelationPopupWindow)
				{
					ImGui::OpenPopup("Relation##DotMiddleWindowRelation");
				}
			}
		}
	}
	if (pref.iEnableRelationPopupWindow)
	{
		if (bDotMiddleWindow)
		{
			if (g_selected_middle_dot_pobject)
			{
				int iSelectedDotObj = g_selected_middle_dot_pobject->dwObjectNumber;
				ImVec2 v2DPos = Convert3DTo2D(ObjectPositionX(iSelectedDotObj), ObjectPositionY(iSelectedDotObj), ObjectPositionZ(iSelectedDotObj));
				//PE: Window is relative to viewport position.
				v2DPos += ImGui::GetMainViewport()->Pos;
				ImGui::SetNextWindowPos(ImVec2(v2DPos.x, v2DPos.y));
			}
			else
			{
				ImGui::SetNextWindowPos(vDotMiddleWindowPos);
			}
			ImGui::SetNextWindowSize(ImVec2(ImGui::GetFontSize()*16.0, 0));
		}
		if (ImGui::BeginPopup("Relation##DotMiddleWindowRelation", ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings)) //ImGuiWindowFlags_AlwaysAutoResize
		{
			bDotMiddleWindow = true;
			int iDotMiddleIndex = g_selected_middle_dot_pobject->dwObjectNumber - DOTMIDDLEOBJECTID;
			if (iDotMiddleIndex >= 0 && iDotMiddleIndex < MAXDOTMIDDLE)
			{
				DisplayRelationshipMenu(iDotMiddleIndex, 1);
				bImGuiGotFocus = true;
				ImGui::EndPopup();
			}
		}
		else 
		{
			bDotMiddleWindow = false;
		}
	}

	if (!bMiddleDotActive && (g_hovered_dot_pobject || bDotObjectDragging))
	{
		if (t.inputsys.mclick == 1 && !bDotObjectDragging) 
		{
			g_source_dot_pobject = g_hovered_dot_pobject;
			ShowObject(DOTCURSOROBJECTID);
		}
		if (t.inputsys.mclick == 1) 
		{
			bDotObjectDragging = true;
			if (g_hovered_dot_pobject)
			{
				int dobj = g_hovered_dot_pobject->dwObjectNumber;
				int sobj = g_source_dot_pobject->dwObjectNumber;
				if (dobj > 70000)
				{
					if (iLastEmmisiveObject != dobj) 
					{
						if (iLastEmmisiveObject > 0) 
						{
							SetObjectEmissive(iLastEmmisiveObject, Rgb(0, 0, 0));
							if (sobj > 0)
							{
								SetObjectEmissive(sobj, Rgb(0, 0, 0));
							}
						}

						// Turn the dots green when hovered over each other (unless dragging the object that the dot is attached to).
						if (!bDraggingActive)
						{
							SetObjectEmissive(dobj, Rgb(56, 110, 146));// 255, 0));
							if (sobj > 0)
							{
								SetObjectEmissive(sobj, Rgb(56, 110, 146));//255, 0));
							}
						}
					}
					iLastEmmisiveObject = dobj;
				}
			}
			else
			{
				if (iLastEmmisiveObject > 0)
				{
					SetObjectEmissive(iLastEmmisiveObject, Rgb(0, 0, 0));
					iLastEmmisiveObject = 0;
				}
			}
		}
		else 
		{
			//PE: Release
			if (bDotObjectDragging && g_hovered_dot_pobject)
			{
				if (g_source_dot_pobject != g_hovered_dot_pobject)
				{
					//PE: Connect to this.
					g_destination_dot_pobject = g_hovered_dot_pobject;
					AddDotObjectRelation(g_source_dot_pobject->dwObjectNumber, g_destination_dot_pobject->dwObjectNumber);
				}
			}
			if (iLastEmmisiveObject > 0)
			{
				SetObjectEmissive(iLastEmmisiveObject, Rgb(0, 0, 0));
				int sobj = g_source_dot_pobject->dwObjectNumber;
				if (sobj > 0)
				{
					SetObjectEmissive(sobj, Rgb(0, 0, 0));
				}
			}
			iLastEmmisiveObject = 0;

			if (bDotObjectDragging)
				HideObject(DOTCURSOROBJECTID);
			bDotObjectDragging = false;

			//Make sure action did not select anything.
			t.widget.pickedObject = 0;
			t.tentitytoselect = 0;
			gridedit_clearentityrubberbandlist();
		}
		PositionObject(DOTCURSOROBJECTID, fLastHitPosition[0], fLastHitPosition[1], fLastHitPosition[2]);
		PointObject(DOTCURSOROBJECTID, CameraPositionX(), CameraPositionY(), CameraPositionZ());
	}
	else
	{
		if(bDotObjectDragging)
			HideObject(DOTCURSOROBJECTID);
		if (iLastEmmisiveObject > 0)
		{
			SetObjectEmissive(iLastEmmisiveObject, Rgb(0, 0, 0));
		}
		iLastEmmisiveObject = 0;
		g_destination_dot_pobject = NULL;
		bDotObjectDragging = false;
	}
}

void DisplayRelationshipMenu(int iDotMiddleIndex, int mode)
{
	//PE: We can use both source and dest entity to find type.
	//iObjectRelationshipsType
	//1 = Character + Character
	//2 = Character + Flag
	//3 = Character + Zone
	//4 = Character + Object
	//5 = Flag + Flag
	//6 = Flag + Zone
	//7 = Flag + Object
	//8 = Zone + Zone
	//9 = Zone + Object
	//10= Object + Object: Show When Object Activated dropdown: Activate other object, Destroy other object

	// Display a brain icon next to the dropdown.
	ImVec2 cursorPos = ImGui::GetCursorPos();
	float fFontSize = ImGui::GetFontSize();
	ImGui::SetCursorPos(cursorPos + ImVec2(-fFontSize + 10, fFontSize));
	ImGui::ImgBtn(BRAIN_ICON, ImVec2(fFontSize * 2, fFontSize * 2), ImVec4(255, 255, 255, 0), ImVec4(255,255,255,255), ImVec4(255,255,255,255), ImVec4(255,255,255,255));
	ImGui::SetCursorPos(cursorPos + ImVec2(fFontSize + 10, 0));

	int sobj = iDotMiddleInfoSource[iDotMiddleIndex] - DOTOBJECTIDADD;
	int dobj = iDotMiddleInfoDestination[iDotMiddleIndex] - DOTOBJECTIDADD;
	int iEntID = 0;
	int iRelationID = 0;
	int iNodeConnectionID = 0;
	GetMiddleEntityIdAndRelationshipId(sobj, dobj, iEntID, iRelationID, iNodeConnectionID);
	if (iEntID > 0)
	{
		int iRelationType = t.entityelement[iEntID].eleprof.iObjectRelationshipsType[iRelationID];
		ImGui::Indent(10);
		ImGui::PushItemWidth(-1.25f * fFontSize);
		
		// character v character
		if (iRelationType == 1)
		{
			ImGui::TextCenter("Character and Character");
			ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(fFontSize *2 , 0));
			const char* items_combo[] = { "Alert other character", "Stand-down other character", "Toggle alert of other character" };
			if (ImGui::Combo("##iCharRelationshipsDataAllies", &t.entityelement[iEntID].eleprof.iObjectRelationshipsData[iRelationID], items_combo, IM_ARRAYSIZE(items_combo)))
			{
				//Reverse data.
				int iEntDID = 0, iDRelationID = 0, iNodeID = 0;
				GetMiddleEntityIdAndRelationshipId(dobj, sobj, iEntDID, iDRelationID, iNodeID);
				if (iEntDID > 0)
				{
					t.entityelement[iEntDID].eleprof.iObjectRelationshipsData[iDRelationID] = t.entityelement[iEntID].eleprof.iObjectRelationshipsData[iRelationID];
				}
				if (mode == 1)
					ImGui::CloseCurrentPopup();
			}
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", "One character can activate activity in another character when alerted");
		}

		// character v flag
		if (iRelationType == 2)
		{
			ImGui::TextCenter("Character and Flag");
			ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(fFontSize * 2, 0));
			const char* items_combo[] = { "Reverse at End", "Loop Around At End", "Follow One Way", "Choose Random Flag" };
			if (ImGui::Combo("##iCharRelationshipsDataPatrol", &t.entityelement[iEntID].eleprof.iObjectRelationshipsData[iRelationID], items_combo, IM_ARRAYSIZE(items_combo)))
			{
				//PE: Also change chars Patrol settings.
				t.entityelement[iEntID].eleprof.iCharPatrolMode = t.entityelement[iEntID].eleprof.iObjectRelationshipsData[iRelationID];
				int iEntDID = 0, iDRelationID = 0, iNodeID = 0;
				GetMiddleEntityIdAndRelationshipId(dobj, sobj, iEntDID, iDRelationID, iNodeID);
				if (iEntDID > 0)
				{
					t.entityelement[iEntDID].eleprof.iObjectRelationshipsData[iDRelationID] = t.entityelement[iEntID].eleprof.iObjectRelationshipsData[iRelationID];
					t.entityelement[iEntDID].eleprof.iCharPatrolMode = t.entityelement[iEntID].eleprof.iObjectRelationshipsData[iRelationID];
				}
				if (mode == 1 && t.entityelement[iEntID].eleprof.iObjectRelationshipsData[iRelationID] != 3)
					ImGui::CloseCurrentPopup();
			}
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", "Set the characters patrol style when connected to a flag element");
		}

		// character v zone
		if (iRelationType == 3)
		{
			ImGui::TextCenter("Character and Zone");
			ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(fFontSize * 2, 0));
			const char* items_combo[] = { "Alert character", "Stand-down character", "Toggle alert the character" };
			if (ImGui::Combo("##iCharRelationshipsDataPatrol", &t.entityelement[iEntID].eleprof.iObjectRelationshipsData[iRelationID], items_combo, IM_ARRAYSIZE(items_combo)))
			{
				int iEntDID = 0, iDRelationID = 0, iNodeID = 0;
				GetMiddleEntityIdAndRelationshipId(dobj, sobj, iEntDID, iDRelationID, iNodeID);
				if (iEntDID > 0)
				{
					t.entityelement[iEntDID].eleprof.iObjectRelationshipsData[iDRelationID] = t.entityelement[iEntID].eleprof.iObjectRelationshipsData[iRelationID];
				}
				if (mode == 1)
					ImGui::CloseCurrentPopup();
			}
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", "Can alert character when zone activated, and activate zone when character alerted");
		}

		// character v object
		if (iRelationType == 4)
		{
			ImGui::TextCenter("Character and Object");
			ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(fFontSize * 2, 0));
			const char* items_combo[] = { "Alert character", "Stand-down character", "Toggle alert the character" };
			if (ImGui::Combo("##iCharRelationshipsDataPatrol", &t.entityelement[iEntID].eleprof.iObjectRelationshipsData[iRelationID], items_combo, IM_ARRAYSIZE(items_combo)))
			{
				int iEntDID = 0, iDRelationID = 0, iNodeID = 0;
				GetMiddleEntityIdAndRelationshipId(dobj, sobj, iEntDID, iDRelationID, iNodeID);
				if (iEntDID > 0)
				{
					t.entityelement[iEntDID].eleprof.iObjectRelationshipsData[iDRelationID] = t.entityelement[iEntID].eleprof.iObjectRelationshipsData[iRelationID];
				}
				if (mode == 1)
					ImGui::CloseCurrentPopup();
			}
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", "Can activate object when character alerted, and alert character when object activated");
		}

		// flag v flag
		if (iRelationType == 5)
		{
			ImGui::TextCenter("Flag and Flag");
			ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(fFontSize * 2, 0));
			const char* items_combo[] = { "Do Nothing" };
			if (ImGui::Combo("##iCharRelationshipsDataPatrol", &t.entityelement[iEntID].eleprof.iObjectRelationshipsData[iRelationID], items_combo, IM_ARRAYSIZE(items_combo)))
			{
				int iEntDID = 0, iDRelationID = 0, iNodeID = 0;
				GetMiddleEntityIdAndRelationshipId(dobj, sobj, iEntDID, iDRelationID, iNodeID);
				if (iEntDID > 0)
				{
					t.entityelement[iEntDID].eleprof.iObjectRelationshipsData[iDRelationID] = t.entityelement[iEntID].eleprof.iObjectRelationshipsData[iRelationID];
				}
				if (mode == 1)
					ImGui::CloseCurrentPopup();
			}
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", "Can set a suggested movement style for between these flags");
		}

		// flag v zone
		if (iRelationType == 6 || iRelationType == 7)
		{
			ImGui::TextCenter("Flag and Zone");
			ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(fFontSize * 2, 0));
			const char* items_combo[] = { "Do Nothing" };// Activate flag", "Deactivate flag", "Toggle flag" ;
			if (ImGui::Combo("##iCharRelationshipsDataPatrol", &t.entityelement[iEntID].eleprof.iObjectRelationshipsData[iRelationID], items_combo, IM_ARRAYSIZE(items_combo)))
			{
				int iEntDID = 0, iDRelationID = 0, iNodeID = 0;
				GetMiddleEntityIdAndRelationshipId(dobj, sobj, iEntDID, iDRelationID, iNodeID);
				if (iEntDID > 0)
				{
					t.entityelement[iEntDID].eleprof.iObjectRelationshipsData[iDRelationID] = t.entityelement[iEntID].eleprof.iObjectRelationshipsData[iRelationID];
				}
				if (mode == 1)
					ImGui::CloseCurrentPopup();
			}
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", "Can set zone to enable or disable any flag element");
		}

		// zone v zone
		if (iRelationType == 8)
		{
			ImGui::TextCenter("Zone and Zone");
			ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(fFontSize * 2, 0));
			const char* items_combo[] = { "Activate other zone", "Deactivate other zone", "Toggle other zone" };
			if (ImGui::Combo("##iCharRelationshipsDataPatrol", &t.entityelement[iEntID].eleprof.iObjectRelationshipsData[iRelationID], items_combo, IM_ARRAYSIZE(items_combo)))
			{
				int iEntDID = 0, iDRelationID = 0, iNodeID = 0;
				GetMiddleEntityIdAndRelationshipId(dobj, sobj, iEntDID, iDRelationID, iNodeID);
				if (iEntDID > 0)
				{
					t.entityelement[iEntDID].eleprof.iObjectRelationshipsData[iDRelationID] = t.entityelement[iEntID].eleprof.iObjectRelationshipsData[iRelationID];
				}
				if (mode == 1)
					ImGui::CloseCurrentPopup();
			}
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", "Can activate or deactivate a zone from another zone");
		}

		// zone v object
		if (iRelationType == 9)
		{
			ImGui::TextCenter("Zone and Object");
			ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(fFontSize * 2, 0));
			const char* items_combo[] = { "Activate object", "Deactivate object", "Toggle object" }; // "Activate zone", "Deactivate zone", "Toggle zone", "Activate object", "Deactivate object", "Toggle object" ;
			if (ImGui::Combo("##iCharRelationshipsDataPatrol", &t.entityelement[iEntID].eleprof.iObjectRelationshipsData[iRelationID], items_combo, IM_ARRAYSIZE(items_combo)))
			{
				int iEntDID = 0, iDRelationID = 0, iNodeID = 0;
				GetMiddleEntityIdAndRelationshipId(dobj, sobj, iEntDID, iDRelationID, iNodeID);
				if (iEntDID > 0)
				{
					t.entityelement[iEntDID].eleprof.iObjectRelationshipsData[iDRelationID] = t.entityelement[iEntID].eleprof.iObjectRelationshipsData[iRelationID];
				}
				if (mode == 1)
					ImGui::CloseCurrentPopup();
			}
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", "Can activate object from zone, or activate zone from object activation");
		}

		// object v object
		if (iRelationType == 10)
		{
			ImGui::TextCenter("Object and Object");
			ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(fFontSize * 2, 0));
			const char* items_combo[] = { "Activate other object", "Deactivate other object", "Toggle other object" };
			if (ImGui::Combo("##iCharRelationshipsDataPatrol", &t.entityelement[iEntID].eleprof.iObjectRelationshipsData[iRelationID], items_combo, IM_ARRAYSIZE(items_combo)))
			{
				int iEntDID = 0, iDRelationID = 0, iNodeID = 0;
				GetMiddleEntityIdAndRelationshipId(dobj, sobj, iEntDID, iDRelationID, iNodeID);
				if (iEntDID > 0)
				{
					t.entityelement[iEntDID].eleprof.iObjectRelationshipsData[iDRelationID] = t.entityelement[iEntID].eleprof.iObjectRelationshipsData[iRelationID];
				}
				if (mode == 1)
					ImGui::CloseCurrentPopup();
			}
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", "Can activate the other object when the first object is activated");
		}

		// delete relationship button
		float but_gadget_size = ImGui::GetFontSize()*12.0;
		float w = ImGui::GetWindowContentRegionWidth() - 10.0;
		ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((w*0.5) - (but_gadget_size*0.5), 0.0f));
		if (ImGui::StyleButton("Delete Relationship", ImVec2(but_gadget_size, 0)))
		{
			t.entityelement[iEntID].eleprof.iObjectRelationships[iRelationID] = 0;
			t.entityelement[iEntID].eleprof.iObjectRelationshipsType[iRelationID] = 0;
			t.entityelement[iEntID].eleprof.iObjectRelationshipsData[iRelationID] = 0;
			int iEntIDd = 0;
			int iRelationIDd = 0;
			int iNodeConnectionID = 0;
			int iNodeConnectionIndex = -1;
			GetMiddleEntityIdAndRelationshipId(dobj, sobj, iEntIDd, iRelationIDd, iNodeConnectionID, &iNodeConnectionIndex);
			if (iEntIDd > 0)
			{
				t.entityelement[iEntIDd].eleprof.iObjectRelationships[iRelationIDd] = 0;
				t.entityelement[iEntIDd].eleprof.iObjectRelationshipsType[iRelationIDd] = 0;
				t.entityelement[iEntIDd].eleprof.iObjectRelationshipsData[iRelationIDd] = 0;

				// Delete the node connection object and remove from storage.
				if (ObjectExist(iNodeConnectionID))
				{
					DeleteObject(iNodeConnectionID);
					iTotalRelationObjects--;
				}

				if (iNodeConnectionIndex > -1)
					nodeconnections.erase(nodeconnections.begin() + iNodeConnectionIndex);
			}

			if (mode == 1)
				ImGui::CloseCurrentPopup();
		}
		if (mode == 1)
		{
			ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((w*0.5) - (but_gadget_size*0.5), 0.0f));
			if (ImGui::StyleButton("Close", ImVec2(but_gadget_size, 0)))
			{
				ImGui::CloseCurrentPopup();
			}
		}
		ImGui::PopItemWidth();
		ImGui::Indent(-10);
	}
}

void AddDotObjectRelation(int sobj,int dobj)
{
	sobj -= DOTOBJECTIDADD;
	dobj -= DOTOBJECTIDADD;
	if (sobj < 70000 || dobj < 70000 || sobj > 90000 || dobj > 90000)
		return;

	//PE: Find real entity.
	bool bSourceSet = false;
	bool bDestSet = false;
	int iEntityIDSource = 0;
	int iEntityIDDest = 0;
	int iTypeSource = 0;
	int iTypeDest = 0;
	int iEntTypeSource = 0;
	int iEntTypeDest = 0;
	int iRelationIndexSource = -1;
	int iRelationIndexDest = -1;
	int iRelationshipLinkIDSource = 0;
	int iRelationshipLinkIDDest = 0;

	for (int iEntityID = 1; iEntityID <= g.entityelementlist; iEntityID++)
	{
		int iBankIndex = t.entityelement[iEntityID].bankindex;
		int iMasterObject = g.entitybankoffset + t.entityelement[iEntityID].bankindex;
		int iEntityObject = t.entityelement[iEntityID].obj;

		//PE: We make 2 ways relations , so no need to scan everything to find a relation.
		//PE: source get destobject.

		int iEntType = -1;
		if (t.entityprofile[iBankIndex].ischaracter == 1)
		{
			iEntType = 1;
		}
		else if (t.entityprofile[iBankIndex].ismarker == 11) //Flags
		{
			iEntType = 2;
		}
		else if (t.entityprofile[iBankIndex].ismarker == 1 || t.entityprofile[iBankIndex].ismarker == 3 || t.entityprofile[iBankIndex].ismarker == 6) //Player start, checkpoint or Trigger zone
		{
			iEntType = 3;
		}
		else 
		{
			//PE: Everything else is a object type.
			//PE: Adding dots to these could be a problem , like stacked boxes ...
			iEntType = 4;
		}

		if (sobj == iEntityObject)
		{
			int iFirstFree = -1;
			bool bAlreadyThere = false;
			iEntityIDSource = iEntityID;
			iEntTypeSource = iEntType;
			int iRelationShipEntityID = 0;
			for (int i = 0; i < 10; i++)
			{
				if (iFirstFree < 0 && t.entityelement[iEntityID].eleprof.iObjectRelationships[i] == 0)
				{
					iFirstFree = i;
				}
				int iRelationShipObject = dobj; // special extra search condition to bypass corruption
				GetRelationshipObject(t.entityelement[iEntityID].eleprof.iObjectRelationships[i], &iRelationShipEntityID, &iRelationShipObject);
				if (iRelationShipObject == dobj)
				{
					iRelationIndexSource = i;
					iRelationshipLinkIDSource = t.entityelement[iEntityID].eleprof.iObjectLinkID;
					bAlreadyThere = true;
					break;
				}
			}
			if (!bAlreadyThere && iFirstFree >= 0)
			{
				iRelationIndexSource = iFirstFree;
			}
			bSourceSet = true;
		}

		if (dobj == iEntityObject)
		{
			int iFirstFree = -1;
			bool bAlreadyThere = false;
			iEntityIDDest = iEntityID;
			iEntTypeDest = iEntType;
			int iRelationShipEntityID = 0;
			for (int i = 0; i < 10; i++)
			{
				if (iFirstFree < 0 && t.entityelement[iEntityID].eleprof.iObjectRelationships[i] == 0)
				{
					iFirstFree = i;
				}
				int iRelationShipObject = sobj; // special extra search condition to bypass corruption
				GetRelationshipObject(t.entityelement[iEntityID].eleprof.iObjectRelationships[i], &iRelationShipEntityID, &iRelationShipObject);
				if (iRelationShipObject == sobj)
				{
					iRelationIndexDest = i;
					iRelationshipLinkIDDest = t.entityelement[iEntityID].eleprof.iObjectLinkID;
					bAlreadyThere = true;
					break;
				}
			}
			if (!bAlreadyThere && iFirstFree >= 0)
			{
				iRelationIndexDest = iFirstFree;
			}
			bDestSet = true;
		}
	}

	// create unique links (if needed), then connect them
	if (iRelationshipLinkIDSource == 0)
	{
		if (t.entityelement[iEntityIDSource].eleprof.iObjectLinkID == 0)
		{
			iRelationshipLinkIDSource = GenerateRelationshipUniqueLinkID();
			t.entityelement[iEntityIDSource].eleprof.iObjectLinkID = iRelationshipLinkIDSource;
		}
		else
			iRelationshipLinkIDSource = t.entityelement[iEntityIDSource].eleprof.iObjectLinkID;
	}
	if (iRelationshipLinkIDDest == 0)
	{
		if (t.entityelement[iEntityIDDest].eleprof.iObjectLinkID == 0)
		{
			iRelationshipLinkIDDest = GenerateRelationshipUniqueLinkID();
			t.entityelement[iEntityIDDest].eleprof.iObjectLinkID = iRelationshipLinkIDDest;
		}
		else
			iRelationshipLinkIDDest = t.entityelement[iEntityIDDest].eleprof.iObjectLinkID;
	}
	if (bDestSet && bSourceSet && iRelationIndexDest >= 0 && iRelationIndexSource >= 0)
	{
		t.entityelement[iEntityIDSource].eleprof.iObjectRelationships[iRelationIndexSource] = iRelationshipLinkIDDest;
		t.entityelement[iEntityIDDest].eleprof.iObjectRelationships[iRelationIndexDest] = iRelationshipLinkIDSource;
	}

	if (bDestSet && bSourceSet && iRelationIndexDest >= 0 && iRelationIndexSource >= 0)
	{
		//Type: 1=Char, 2=Flag, 3=Zone , 4=Object.
		if (iEntTypeSource == 1 && iEntTypeDest == 1) 
		{ //Char,Char
			t.entityelement[iEntityIDSource].eleprof.iObjectRelationshipsType[iRelationIndexSource] = 1;
			t.entityelement[iEntityIDDest].eleprof.iObjectRelationshipsType[iRelationIndexDest] = 1;
		}
		else if ((iEntTypeSource == 1 && iEntTypeDest == 2) || (iEntTypeSource == 2 && iEntTypeDest == 1)) 
		{ //Char,Flag
			t.entityelement[iEntityIDSource].eleprof.iObjectRelationshipsType[iRelationIndexSource] = 2;
			t.entityelement[iEntityIDDest].eleprof.iObjectRelationshipsType[iRelationIndexDest] = 2;
		}
		else if ((iEntTypeSource == 1 && iEntTypeDest == 3) || (iEntTypeSource == 3 && iEntTypeDest == 1)) 
		{ //Char,zone
			t.entityelement[iEntityIDSource].eleprof.iObjectRelationshipsType[iRelationIndexSource] = 3;
			t.entityelement[iEntityIDDest].eleprof.iObjectRelationshipsType[iRelationIndexDest] = 3;
		}
		else if ((iEntTypeSource == 1 && iEntTypeDest == 4) || (iEntTypeSource == 4 && iEntTypeDest == 1))
		{ //Char,Object
			t.entityelement[iEntityIDSource].eleprof.iObjectRelationshipsType[iRelationIndexSource] = 4;
			t.entityelement[iEntityIDDest].eleprof.iObjectRelationshipsType[iRelationIndexDest] = 4;
		}
		else if (iEntTypeSource == 2 && iEntTypeDest == 2) { //Flag,Flag
			t.entityelement[iEntityIDSource].eleprof.iObjectRelationshipsType[iRelationIndexSource] = 5;
			t.entityelement[iEntityIDDest].eleprof.iObjectRelationshipsType[iRelationIndexDest] = 5;
		}
		else if ((iEntTypeSource == 2 && iEntTypeDest == 3) || (iEntTypeSource == 3 && iEntTypeDest == 2)) 
		{ //flag,zone
			t.entityelement[iEntityIDSource].eleprof.iObjectRelationshipsType[iRelationIndexSource] = 6;
			t.entityelement[iEntityIDDest].eleprof.iObjectRelationshipsType[iRelationIndexDest] = 6;
		}
		else if ((iEntTypeSource == 2 && iEntTypeDest == 4) || (iEntTypeSource == 4 && iEntTypeDest == 2)) 
		{ //flag,object
			t.entityelement[iEntityIDSource].eleprof.iObjectRelationshipsType[iRelationIndexSource] = 7;
			t.entityelement[iEntityIDDest].eleprof.iObjectRelationshipsType[iRelationIndexDest] = 7;
		}
		else if (iEntTypeSource == 3 && iEntTypeDest == 3) 
		{ //zone,zone
			t.entityelement[iEntityIDSource].eleprof.iObjectRelationshipsType[iRelationIndexSource] = 8;
			t.entityelement[iEntityIDDest].eleprof.iObjectRelationshipsType[iRelationIndexDest] = 8;
		}
		else if ((iEntTypeSource == 3 && iEntTypeDest == 4) || (iEntTypeSource == 4 && iEntTypeDest == 3)) 
		{ //zone,object
			t.entityelement[iEntityIDSource].eleprof.iObjectRelationshipsType[iRelationIndexSource] = 9;
			t.entityelement[iEntityIDDest].eleprof.iObjectRelationshipsType[iRelationIndexDest] = 9;
		}
		else if (iEntTypeSource == 4 && iEntTypeDest == 4) 
		{ 
			//object,object
			t.entityelement[iEntityIDSource].eleprof.iObjectRelationshipsType[iRelationIndexSource] = 10;
			t.entityelement[iEntityIDDest].eleprof.iObjectRelationshipsType[iRelationIndexDest] = 10;

			// objects toggle by default
			t.entityelement[iEntityIDSource].eleprof.iObjectRelationshipsData[iRelationIndexSource] = 2;
			t.entityelement[iEntityIDDest].eleprof.iObjectRelationshipsData[iRelationIndexDest] = 2;
		}
	}
}

void GetMiddleEntityIdAndRelationshipId(int sobj, int dobj, int &Entid, int &ReleationshipId, int& NodeConnectionId, int* NodeConnectionIndex)
{
	if (sobj < 70000 || sobj > 90000 || dobj < 70000 || dobj > 90000)
		return;

	Entid = 0;
	ReleationshipId = 0;
	NodeConnectionId = 0;

	for (int iEntityID = 1; iEntityID <= g.entityelementlist; iEntityID++)
	{
		int iBankIndex = t.entityelement[iEntityID].bankindex;
		int iMasterObject = g.entitybankoffset + t.entityelement[iEntityID].bankindex;
		int iEntityObject = t.entityelement[iEntityID].obj;

		//PE: We make 2 ways relations , so no need to scan everything to find a relation.
		//PE: source get destobject.
		if (sobj == iEntityObject)
		{
			for (int i = 0; i < 10; i++)
			{
				int iRelationShipEntityID = 0;
				int iRelationShipObject = dobj; // special extra search condition to bypass corruption
				GetRelationshipObject(t.entityelement[iEntityID].eleprof.iObjectRelationships[i], &iRelationShipEntityID, &iRelationShipObject);
				if (iRelationShipObject == dobj)
				{
					Entid = iEntityID;
					ReleationshipId = i;

					for (int j = 0; j < nodeconnections.size(); j++)
					{
						if ((nodeconnections[j].from == sobj && nodeconnections[j].to == dobj) || (nodeconnections[j].from == dobj && nodeconnections[j].to == sobj))
						{
							NodeConnectionId = nodeconnections[j].nodeconnectionid;
							if (NodeConnectionIndex)
								*NodeConnectionIndex = j;
							break;
						}
					}
					return;
				}
			}
		}
	}
}

void FindDotObjectRelation(int sobj, int dobj,int middle_index)
{
	sobj -= DOTOBJECTIDADD;
	dobj -= DOTOBJECTIDADD;

	if (sobj < 70000 || dobj < 70000 || sobj > 90000 || dobj > 90000)
		return;

	//PE: Find real entity.
	for (int iEntityID = 1; iEntityID <= g.entityelementlist; iEntityID++)
	{
		int iBankIndex = t.entityelement[iEntityID].bankindex;
		int iMasterObject = g.entitybankoffset + t.entityelement[iEntityID].bankindex;
		int iEntityObject = t.entityelement[iEntityID].obj;
	}
}

void DrawDotArcsCircle(int from, int radius)
{
	if (radius < 1)
		return;

	float ffromx = ObjectPositionX(from);
	float ffromy = ObjectPositionY(from);
	float ffromz = ObjectPositionZ(from);
	int AddDots = radius / 4;//8 8.0;
	if (AddDots < 8)
		AddDots = 8;
	float fStep = (3.14159265*2.0) / ((float)AddDots);
	DWORD dwNewColor = Rgb(255, 128, 0);
	int iColor = 199;
	int iPointObject = iTotalArcs;
	float sincount = 0;
	for (int i = 0;i < AddDots;i++)
	{
		CreateDotArcObject(DOTARCSOBJECTID + iTotalArcs);
		ShowObject(DOTARCSOBJECTID + iTotalArcs);

		float fnfromx = ffromx + (sin(sincount)*(float)radius);
		float fnfromy = ffromy;
		float fnfromz = ffromz + (cos(sincount)*(float)radius);;
		PositionObject(DOTARCSOBJECTID + iTotalArcs, fnfromx, fnfromy, fnfromz);

		sincount += fStep;

		if (i > 0) 
		{
			int iOldObj = DOTARCSOBJECTID + (iTotalArcs - 1);
			PointObject(DOTARCSOBJECTID + iTotalArcs, ObjectPositionX(iOldObj), ObjectPositionY(iOldObj), ObjectPositionZ(iOldObj));
		}

		if (iDotArceColor[iTotalArcs] != iColor)
		{
			iDotArceColor[iTotalArcs] = iColor;
			SetObjectDiffuseEx(DOTARCSOBJECTID + iTotalArcs, dwNewColor, 0);
		}

		if (iTotalArcs < MAXDOTARCSOBJECTS)
		{
			iTotalArcs++;
			if (iTotalArcs > iLargestArcs)
				iLargestArcs = iTotalArcs;
		}
	}
	int iOldObj = DOTARCSOBJECTID + iPointObject;
	PointObject(iOldObj, ObjectPositionX(iOldObj + 1), ObjectPositionY(iOldObj + 1), ObjectPositionZ(iOldObj + 1));
}

void AddVertToObjectRelation(float x, float y, float z, float texU, float texV, int v, int memblock)
{
	//  Position of vertex in memblock
	int pos = 12 + (v * 32);

	//  Set vertex position
	WriteMemblockFloat(memblock, pos + 0, x);
	WriteMemblockFloat(memblock, pos + 4, y);
	WriteMemblockFloat(memblock, pos + 8, z);
	WriteMemblockFloat(memblock, pos + 12, 0);
	WriteMemblockFloat(memblock, pos + 16, 0);
	WriteMemblockFloat(memblock, pos + 20, 0);
	WriteMemblockFloat(memblock, pos + 24, texU);
	WriteMemblockFloat(memblock, pos + 28, texV);
}

void CreateObjectRelationMesh(float fromx, float fromy, float fromz, float tox, float toy, float toz, DWORD color)
{
	// this gets the diffuse into the emissive for a glowier line for logic lines!
	WickedCall_PresetObjectPutInEmissive(1);

	// Find a free memblock.
	int iFound = 0;
	for (int i = 1; i <= 257; i++)
	{
		if (MemblockExist(i) == 0)
		{
			iFound = i;
			break;
		}
	}
	if (iFound == 0) return;

	// Find a free object slot.
	int obj = t.activerelationobjectid;
	if (ObjectExist(obj) == 1)
		return;

	iTotalRelationObjects++;

	int vertsize = 32;
	int iSizeBytes = 0;
	int vertexCount = 36;
	iSizeBytes = vertsize * vertexCount;
	iSizeBytes += 12; // Add header bytes.
	MakeMemblock(iFound, iSizeBytes);

	// Write the memblock header.
	// FVF format.
	WriteMemblockDWord(iFound, 0, GGFVF_XYZ | GGFVF_NORMAL | GGFVF_TEX1);
	// Size of single vertex - 3 x float: position, 3 x float: normal, 2 x float: tex coords = 32 bytes.
	WriteMemblockDWord(iFound, 4, 32);
	// Number of vertices in the mesh.
	WriteMemblockDWord(iFound, 8, vertexCount);

	// Corners of the prism.
	float x0, x1, x2, x3, x4, x5;
	float y0, y1, y2, y3, y4, y5;
	float z0, z1, z2, z3, z4, z5;
	// Midpoints to split the prism in two.
	float mx03, mx14, mx25;
	float my03, my14, my25;
	float mz03, mz14, mz25;

	int v = 0;
	float p0[3];
	float p1[3];
	float points[18];
	
	p0[0] = fromx; p0[1] = fromy; p0[2] = fromz;
	p1[0] = tox; p1[1] = toy; p1[2] = toz;

	physics_debug_make_prism_between_points(p0, p1, points, 0.5f);// 3);

	// Corners of the prism.
	x0 = points[0]; y0 = points[1]; z0 = points[2];
	x1 = points[3]; y1 = points[4]; z1 = points[5];
	x2 = points[6]; y2 = points[7]; z2 = points[8];
	x3 = points[9]; y3 = points[10]; z3 = points[11];
	x4 = points[12]; y4 = points[13]; z4 = points[14];
	x5 = points[15]; y5 = points[16]; z5 = points[17];

	// Midpoints.
	mx03 = (x0 + x3) / 2.0f; my03 = (y0 + y3) / 2.0f; mz03 = (z0 + z3) / 2.0f;
	mx14 = (x1 + x4) / 2.0f; my14 = (y1 + y4) / 2.0f; mz14 = (z1 + z4) / 2.0f;
	mx25 = (x2 + x5) / 2.0f; my25 = (y2 + y5) / 2.0f; mz25 = (z2 + z5) / 2.0f;
	
	// Tex Coords.
	if (!bUVsAlreadySet)
	{
		fRelationUVs[0] = 0; fRelationUVs[1] = 0;
		fRelationUVs[2] = 0; fRelationUVs[3] = 1;
		fRelationUVs[4] = 1; fRelationUVs[5] = 0;
		fRelationUVs[6] = 1; fRelationUVs[7] = 0;
		fRelationUVs[8] = 0; fRelationUVs[9] = 1;
		fRelationUVs[10] = 1; fRelationUVs[11] = 1;
		const float off = 0.16666667f;
		fRelationUVs[12] = 0 + off; fRelationUVs[13] = 0 + off;
		fRelationUVs[14] = 0 + off; fRelationUVs[15] = 1 + off;
		fRelationUVs[16] = 1 + off; fRelationUVs[17] = 0 + off;
		fRelationUVs[18] = 1 + off; fRelationUVs[19] = 0 + off;
		fRelationUVs[20] = 0 + off; fRelationUVs[21] = 1 + off;
		fRelationUVs[22] = 1 + off; fRelationUVs[23] = 1 + off;

		for (int i = 0; i < 24; i++)
		{
			fRelationUVsStorage[i] = fRelationUVs[i];
		}

		bUVsAlreadySet = true;
	}
	
	// TODO: Just make one face and point at camera.

	// Bottom face.
	AddVertToObjectRelation(x0, y0, z0, fRelationUVs[0], fRelationUVs[1], v++, iFound);
	AddVertToObjectRelation(x2, y2, z2, fRelationUVs[2], fRelationUVs[3], v++, iFound);
	AddVertToObjectRelation(mx03, my03, mz03, fRelationUVs[4], fRelationUVs[5], v++, iFound);
	AddVertToObjectRelation(mx03, my03, mz03, fRelationUVs[6], fRelationUVs[7], v++, iFound);
	AddVertToObjectRelation(x2, y2, z2, fRelationUVs[8], fRelationUVs[9], v++, iFound);
	AddVertToObjectRelation(mx25, my25, mz25, fRelationUVs[10], fRelationUVs[11], v++, iFound);
	AddVertToObjectRelation(mx03, my03, mz03, fRelationUVs[0], fRelationUVs[1], v++, iFound);
	AddVertToObjectRelation(mx25, my25, mz25, fRelationUVs[2], fRelationUVs[3], v++, iFound);
	AddVertToObjectRelation(x3, y3, z3, fRelationUVs[4], fRelationUVs[5], v++, iFound);
	AddVertToObjectRelation(x3, y3, z3, fRelationUVs[6], fRelationUVs[7], v++, iFound);
	AddVertToObjectRelation(mx25, my25, mz25, fRelationUVs[8], fRelationUVs[9], v++, iFound);
	AddVertToObjectRelation(x5, y5, z5, fRelationUVs[10], fRelationUVs[11], v++, iFound);

	// Right face.
	AddVertToObjectRelation(x1, y1, z1, fRelationUVs[0], fRelationUVs[1], v++, iFound);
	AddVertToObjectRelation(x0, y0, z0, fRelationUVs[2], fRelationUVs[3], v++, iFound);
	AddVertToObjectRelation(mx14, my14, mz14, fRelationUVs[4], fRelationUVs[5], v++, iFound);
	AddVertToObjectRelation(mx14, my14, mz14, fRelationUVs[6], fRelationUVs[7], v++, iFound);
	AddVertToObjectRelation(x0, y0, z0, fRelationUVs[8], fRelationUVs[9], v++, iFound);
	AddVertToObjectRelation(mx03, my03, mz03, fRelationUVs[10], fRelationUVs[11], v++, iFound);
	AddVertToObjectRelation(mx14, my14, mz14, fRelationUVs[0], fRelationUVs[1], v++, iFound);
	AddVertToObjectRelation(mx03, my03, mz03, fRelationUVs[2], fRelationUVs[3], v++, iFound);
	AddVertToObjectRelation(x4, y4, z4, fRelationUVs[4], fRelationUVs[5], v++, iFound);
	AddVertToObjectRelation(x4, y4, z4, fRelationUVs[6], fRelationUVs[7], v++, iFound);
	AddVertToObjectRelation(mx03, my03, mz03, fRelationUVs[8], fRelationUVs[9], v++, iFound);
	AddVertToObjectRelation(x3, y3, z3, fRelationUVs[10], fRelationUVs[11], v++, iFound);

	// Left face.
	AddVertToObjectRelation(x2, y2, z2, fRelationUVs[0], fRelationUVs[1], v++, iFound);
	AddVertToObjectRelation(x1, y1, z1, fRelationUVs[2], fRelationUVs[3], v++, iFound);
	AddVertToObjectRelation(mx25, my25, mz25, fRelationUVs[4], fRelationUVs[5], v++, iFound);
	AddVertToObjectRelation(mx25, my25, mz25, fRelationUVs[6], fRelationUVs[7], v++, iFound);
	AddVertToObjectRelation(x1, y1, z1, fRelationUVs[8], fRelationUVs[9], v++, iFound);
	AddVertToObjectRelation(mx14, my14, mz14, fRelationUVs[10], fRelationUVs[11], v++, iFound);
	AddVertToObjectRelation(mx25, my25, mz25, fRelationUVs[0], fRelationUVs[1], v++, iFound);
	AddVertToObjectRelation(mx14, my14, mz14, fRelationUVs[2], fRelationUVs[3], v++, iFound);
	AddVertToObjectRelation(x5, y5, z5, fRelationUVs[4], fRelationUVs[5], v++, iFound);
	AddVertToObjectRelation(x5, y5, z5, fRelationUVs[6], fRelationUVs[7], v++, iFound);
	AddVertToObjectRelation(mx14, my14, mz14, fRelationUVs[8], fRelationUVs[9], v++, iFound);
	AddVertToObjectRelation(x4, y4, z4, fRelationUVs[10], fRelationUVs[11], v++, iFound);

	CreateMeshFromMemblock(obj, iFound);
	MakeObject(obj, obj, 0);

	sObject* pObject = GetObjectData(obj);

	WickedCall_PresetObjectRenderLayer(GGRENDERLAYERS_CURSOROBJECT);
	WickedCall_RemoveObject(pObject);
	WickedCall_AddObject(pObject);
	
	if (ImageExist(g.visuallogicimageoffset) == 0)
		LoadImage("editors\\uiv3\\nodeconnection.png", g.visuallogicimageoffset);
	image_setlegacyimageloading(false);

	TextureObject(obj, g.visuallogicimageoffset);
	//TextureObject(obj, ABOUT_LOGO);
	
	WickedCall_SetObjectCastShadows(pObject, false);
	WickedCall_SetObjectLightToUnlit(pObject, (int)wiScene::MaterialComponent::SHADERTYPE::SHADERTYPE_UNLIT);

	WickedCall_PresetObjectRenderLayer(GGRENDERLAYERS_NORMAL);

	//SetObjectDiffuse(obj, color);
	SetObjectCull(obj, 0);

	DeleteMemblock(iFound);
	ShowObject(obj);

	WickedCall_PresetObjectPutInEmissive(0);
}

// Update the vertex positions of half the node connection mesh, used for one way movement.
void UpdateObjectRelationMeshHalf(float fromx, float fromy, float fromz, float tox, float toy, float toz)
{
	float x0, x1, x2, x3, x4, x5;
	float y0, y1, y2, y3, y4, y5;
	float z0, z1, z2, z3, z4, z5;
	int v = 0;
	float p0[3];
	float p1[3];
	float points[18];
	int id = RELATIONOBJECTID;
	sObject* pObject = GetObjectData(id);
	if (!pObject)
	{
		return;
	}

	sMesh* pMesh = pObject->ppMeshList[0];
	if (!pMesh)
	{
		return;
	}

	LockVertexDataForLimbCore(id, 0, 1);
	
	// Every 6 elements of data contain two points on the physics object.
	p0[0] = fromx; p0[1] = fromy; p0[2] = fromz;
	p1[0] = tox; p1[1] = toy; p1[2] = toz;

	physics_debug_make_prism_between_points(p0, p1, points, 0.5f);// 3);

	// Corners of the prism.
	x0 = points[0]; y0 = points[1]; z0 = points[2];
	x1 = points[3]; y1 = points[4]; z1 = points[5];
	x2 = points[6]; y2 = points[7]; z2 = points[8];
	x3 = points[9]; y3 = points[10]; z3 = points[11];
	x4 = points[12]; y4 = points[13]; z4 = points[14];
	x5 = points[15]; y5 = points[16]; z5 = points[17];

	// Make each face of the prism (two triangles per face).
	SetVertexDataUV(v, fRelationUVs[12], fRelationUVs[13]);
	SetVertexDataPosition(v++, x0, y0, z0);
	SetVertexDataUV(v, fRelationUVs[14], fRelationUVs[15]);
	SetVertexDataPosition(v++, x2, y2, z2);
	SetVertexDataUV(v, fRelationUVs[16], fRelationUVs[17]);
	SetVertexDataPosition(v++, x3, y3, z3);
	SetVertexDataUV(v, fRelationUVs[18], fRelationUVs[19]);
	SetVertexDataPosition(v++, x3, y3, z3);
	SetVertexDataUV(v, fRelationUVs[20], fRelationUVs[21]);
	SetVertexDataPosition(v++, x2, y2, z2);
	SetVertexDataUV(v, fRelationUVs[22], fRelationUVs[23]);
	SetVertexDataPosition(v++, x5, y5, z5);

	SetVertexDataUV(v, fRelationUVs[12], fRelationUVs[13]);
	SetVertexDataPosition(v++, x1, y1, z1);
	SetVertexDataUV(v, fRelationUVs[14], fRelationUVs[15]);
	SetVertexDataPosition(v++, x0, y0, z0);
	SetVertexDataUV(v, fRelationUVs[16], fRelationUVs[17]);
	SetVertexDataPosition(v++, x4, y4, z4);
	SetVertexDataUV(v, fRelationUVs[18], fRelationUVs[19]);
	SetVertexDataPosition(v++, x4, y4, z4);
	SetVertexDataUV(v, fRelationUVs[20], fRelationUVs[21]);
	SetVertexDataPosition(v++, x0, y0, z0);
	SetVertexDataUV(v, fRelationUVs[22], fRelationUVs[23]);
	SetVertexDataPosition(v++, x3, y3, z3);

	SetVertexDataUV(v, fRelationUVs[12], fRelationUVs[13]);
	SetVertexDataPosition(v++, x2, y2, z2);
	SetVertexDataUV(v, fRelationUVs[14], fRelationUVs[15]);
	SetVertexDataPosition(v++, x1, y1, z1);
	SetVertexDataUV(v, fRelationUVs[16], fRelationUVs[17]);
	SetVertexDataPosition(v++, x5, y5, z5);
	SetVertexDataUV(v, fRelationUVs[18], fRelationUVs[19]);
	SetVertexDataPosition(v++, x5, y5, z5);
	SetVertexDataUV(v, fRelationUVs[20], fRelationUVs[21]);
	SetVertexDataPosition(v++, x1, y1, z1);
	SetVertexDataUV(v, fRelationUVs[22], fRelationUVs[23]);
	SetVertexDataPosition(v++, x4, y4, z4);

	for (int i = 0; i < 18; i++)
		SetVertexDataPosition(v++, 0, 0, 0);

	UnlockVertexData();

	WickedCall_UpdateMeshVertexData(pMesh);
}

// Used when the user drags an object with an active relation, so need to update the node connection mesh.
void UpdateObjectRelationMesh(float fromx, float fromy, float fromz, float tox, float toy, float toz)
{
	float x0, x1, x2, x3, x4, x5;
	float y0, y1, y2, y3, y4, y5;
	float z0, z1, z2, z3, z4, z5;
	// Midpoints to split the prism in two.
	float mx03, mx14, mx25;
	float my03, my14, my25;
	float mz03, mz14, mz25;

	int v = 0;
	float p0[3];
	float p1[3];
	float points[18];
	int id = t.activerelationobjectid;
	sObject* pObject = GetObjectData(id);
	if (!pObject)
	{
		return;
	}

	sMesh* pMesh = pObject->ppMeshList[0];
	if (!pMesh)
	{
		return;
	}

	LockVertexDataForLimbCore(id, 0, 1);

	// Every 6 elements of data contain two points on the physics object.
	p0[0] = fromx; p0[1] = fromy; p0[2] = fromz;
	p1[0] = tox; p1[1] = toy; p1[2] = toz;

	physics_debug_make_prism_between_points(p0, p1, points, 0.5f);// 3);

	// Corners of the prism.
	x0 = points[0]; y0 = points[1]; z0 = points[2];
	x1 = points[3]; y1 = points[4]; z1 = points[5];
	x2 = points[6]; y2 = points[7]; z2 = points[8];
	x3 = points[9]; y3 = points[10]; z3 = points[11];
	x4 = points[12]; y4 = points[13]; z4 = points[14];
	x5 = points[15]; y5 = points[16]; z5 = points[17];
	// Midpoints.
	mx03 = (x0 + x3) / 2.0f; my03 = (y0 + y3) / 2.0f; mz03 = (z0 + z3) / 2.0f;
	mx14 = (x1 + x4) / 2.0f; my14 = (y1 + y4) / 2.0f; mz14 = (z1 + z4) / 2.0f;
	mx25 = (x2 + x5) / 2.0f; my25 = (y2 + y5) / 2.0f; mz25 = (z2 + z5) / 2.0f;

	// Make each face of the prism (two triangles per face).
	SetVertexDataUV(v, fRelationUVs[0], fRelationUVs[1]);
	SetVertexDataPosition(v++, x0, y0, z0);
	SetVertexDataUV(v, fRelationUVs[2], fRelationUVs[3]);
	SetVertexDataPosition(v++, x2, y2, z2);
	SetVertexDataUV(v, fRelationUVs[4], fRelationUVs[5]);
	SetVertexDataPosition(v++, mx03, my03, mz03);
	SetVertexDataUV(v, fRelationUVs[6], fRelationUVs[7]);
	SetVertexDataPosition(v++, mx03, my03, mz03);
	SetVertexDataUV(v, fRelationUVs[8], fRelationUVs[9]);
	SetVertexDataPosition(v++, x2, y2, z2);
	SetVertexDataUV(v, fRelationUVs[10], fRelationUVs[11]);
	SetVertexDataPosition(v++, mx25, my25, mz25);
	SetVertexDataUV(v, fRelationUVs[12], fRelationUVs[13]);
	SetVertexDataPosition(v++, mx03, my03, mz03);
	SetVertexDataUV(v, fRelationUVs[14], fRelationUVs[15]);
	SetVertexDataPosition(v++, mx25, my25, mz25);
	SetVertexDataUV(v, fRelationUVs[16], fRelationUVs[17]);
	SetVertexDataPosition(v++, x3, y3, z3);
	SetVertexDataUV(v, fRelationUVs[18], fRelationUVs[19]);
	SetVertexDataPosition(v++, x3, y3, z3);
	SetVertexDataUV(v, fRelationUVs[20], fRelationUVs[21]);
	SetVertexDataPosition(v++, mx25, my25, mz25);
	SetVertexDataUV(v, fRelationUVs[22], fRelationUVs[23]);
	SetVertexDataPosition(v++, x5, y5, z5);

	SetVertexDataUV(v, fRelationUVs[0], fRelationUVs[1]);
	SetVertexDataPosition(v++, x1, y1, z1);
	SetVertexDataUV(v, fRelationUVs[2], fRelationUVs[3]);
	SetVertexDataPosition(v++, x0, y0, z0);
	SetVertexDataUV(v, fRelationUVs[4], fRelationUVs[5]);
	SetVertexDataPosition(v++, mx14, my14, mz14);
	SetVertexDataUV(v, fRelationUVs[6], fRelationUVs[7]);
	SetVertexDataPosition(v++, mx14, my14, mz14);
	SetVertexDataUV(v, fRelationUVs[8], fRelationUVs[9]);
	SetVertexDataPosition(v++,x0 ,y0 ,z0 );
	SetVertexDataUV(v, fRelationUVs[10], fRelationUVs[11]);
	SetVertexDataPosition(v++, mx03, my03, mz03);
	SetVertexDataUV(v, fRelationUVs[12], fRelationUVs[13]);
	SetVertexDataPosition(v++, mx14, my14, mz14);
	SetVertexDataUV(v, fRelationUVs[14], fRelationUVs[15]);
	SetVertexDataPosition(v++, mx03, my03, mz03);
	SetVertexDataUV(v, fRelationUVs[16], fRelationUVs[17]);
	SetVertexDataPosition(v++, x4, y4, z4);
	SetVertexDataUV(v, fRelationUVs[18], fRelationUVs[19]);
	SetVertexDataPosition(v++, x4, y4, z4);
	SetVertexDataUV(v, fRelationUVs[20], fRelationUVs[21]);
	SetVertexDataPosition(v++, mx03, my03, mz03);
	SetVertexDataUV(v, fRelationUVs[22], fRelationUVs[23]);
	SetVertexDataPosition(v++, x3, y3, z3);

	SetVertexDataUV(v, fRelationUVs[0], fRelationUVs[1]);
	SetVertexDataPosition(v++, x2, y2, z2);
	SetVertexDataUV(v, fRelationUVs[2], fRelationUVs[3]);
	SetVertexDataPosition(v++, x1, y1, z1);
	SetVertexDataUV(v, fRelationUVs[4], fRelationUVs[5]);
	SetVertexDataPosition(v++, mx25, my25, mz25);
	SetVertexDataUV(v, fRelationUVs[6], fRelationUVs[7]);
	SetVertexDataPosition(v++, mx25, my25, mz25);
	SetVertexDataUV(v, fRelationUVs[8], fRelationUVs[9]);
	SetVertexDataPosition(v++, x1, y1, z1);
	SetVertexDataUV(v, fRelationUVs[10], fRelationUVs[11]);
	SetVertexDataPosition(v++, mx14, my14, mz14);
	SetVertexDataUV(v, fRelationUVs[12], fRelationUVs[13]);
	SetVertexDataPosition(v++, mx25, my25, mz25);
	SetVertexDataUV(v, fRelationUVs[14], fRelationUVs[15]);
	SetVertexDataPosition(v++, mx14, my14, mz14);
	SetVertexDataUV(v, fRelationUVs[16], fRelationUVs[17]);
	SetVertexDataPosition(v++, x5, y5, z5);
	SetVertexDataUV(v, fRelationUVs[18], fRelationUVs[19]);
	SetVertexDataPosition(v++, x5, y5, z5);
	SetVertexDataUV(v, fRelationUVs[20], fRelationUVs[21]);
	SetVertexDataPosition(v++, mx14, my14, mz14);
	SetVertexDataUV(v, fRelationUVs[22], fRelationUVs[23]);
	SetVertexDataPosition(v++, x4, y4, z4);

	UnlockVertexData();

	WickedCall_UpdateMeshVertexData(pMesh);
}

void UpdateObjectRelationUVs(int id)
{
	sObject* pObject = GetObjectData(id);
	if (!pObject)
	{
		return;
	}

	sMesh* pMesh = pObject->ppMeshList[0];
	if (!pMesh)
	{
		return;
	}

	int v = 0;

	LockVertexDataForLimbCore(id, 0, 1);

	for (int i = 0; i < 3; i++)
		for (int j = 0; j < 24; j+=2)
			SetVertexDataUV(v++, fRelationUVs[j], fRelationUVs[j+1]);

	UnlockVertexData();

	WickedCall_UpdateMeshVertexData(pMesh);
}

void DrawObjectRelation(int from, int to, bool bDrawMiddleBut, int forceVertUpdate)
{
	//Find relation colors.
	bool bHighlight = false;
	int iColor = -1;
	int sobj = from - DOTOBJECTIDADD;
	int dobj = to - DOTOBJECTIDADD;
	int iEntID = 0;
	int iRelationID = 0;
	int iNodeConnectionID = 0;
	//DWORD dwNewColor = Rgb(255, 255, 0);
	DWORD dwNewColor = Rgb(225, 225, 225);

	if (bDotMiddleWindow && g_selected_middle_dot_pobject && g_selected_middle_dot_pobject->dwObjectNumber == DOTMIDDLEOBJECTID + iTotalMiddle)
	{
		bHighlight = true;
	}

	if (bHighlight)
	{
		//dwNewColor = Rgb(255, 255, 32);
		dwNewColor = Rgb(255, 255, 255);
	}

	int iNodeConnectionIndex = -1;
	GetMiddleEntityIdAndRelationshipId(sobj, dobj, iEntID, iRelationID, iNodeConnectionID, &iNodeConnectionIndex);
	if (iEntID > 0)
	{
		int iRelationType = t.entityelement[iEntID].eleprof.iObjectRelationshipsType[iRelationID];
		if (iRelationType == 1) //Char,Char
		{
			//"Standard Alliances behavior", "Assist other when attacked", "Run away from other when attacked","Stay away from other","Meet other when in visual range"
			if (t.entityelement[iEntID].eleprof.iObjectRelationshipsData[iRelationID] == 0)
			{
				iColor = 1; //Green Standard Alliances behavior
			}
			else if (t.entityelement[iEntID].eleprof.iObjectRelationshipsData[iRelationID] == 1)
			{
				iColor = 2; //Red Assist other when attacked
			}
			else if (t.entityelement[iEntID].eleprof.iObjectRelationshipsData[iRelationID] == 2)
			{
				iColor = 3; //Blue Run away from other when attacked
			}
			else if (t.entityelement[iEntID].eleprof.iObjectRelationshipsData[iRelationID] == 3)
			{
				iColor = 3; //Blue Stay away from other
			}
			else if (t.entityelement[iEntID].eleprof.iObjectRelationshipsData[iRelationID] == 4)
			{
				iColor = 1; //Green Meet other when in visual range
			}
		}

		int iColorAdd = 0;
		if (bHighlight)
			iColorAdd = 45;
	}

	if (bHighlight)
	{
		iColor += 99;
	}

	// when established connection, use centers of object
	float ffromx = ObjectPositionX(sobj) + GetObjectCollisionCenterX(sobj);
	float ffromy = ObjectPositionY(sobj) + GetObjectCollisionCenterY(sobj);
	float ffromz = ObjectPositionZ(sobj) + GetObjectCollisionCenterZ(sobj);
	float ftox = ObjectPositionX(dobj) + GetObjectCollisionCenterX(dobj);
	float ftoy = ObjectPositionY(dobj) + GetObjectCollisionCenterY(dobj);
	float ftoz = ObjectPositionZ(dobj) + GetObjectCollisionCenterZ(dobj);

	// cursor connect at click drag release sites
	if (to == DOTCURSOROBJECTID)
	{
		ftox = ObjectPositionX(to);
		ftoy = ObjectPositionY(to);
		ftoz = ObjectPositionZ(to);
	}

	float fdx = ffromx - ftox;
	float fdy = ffromy - ftoy;
	float fdz = ffromz - ftoz;

	float fDistance = sqrt((fdx * fdx) + (fdy * fdy) + (fdz * fdz));

	if (to == DOTCURSOROBJECTID)
	{
		// Drawing a new relation.
		t.activerelationobjectid = RELATIONOBJECTID;
		if (!ObjectExist(RELATIONOBJECTID))
			CreateObjectRelationMesh(ffromx, ffromy, ffromz, ftox, ftoy, ftoz, dwNewColor);
		else
			UpdateObjectRelationMeshHalf(ffromx, ffromy, ffromz, ftox, ftoy, ftoz);

		ShowObject(RELATIONOBJECTID);
	}

	if (bDrawMiddleBut)
	{
		// Make the button go green when selected.
		if (bHighlight && iNodeConnectionIndex >= 0 && !bDraggingActive)
		{
			ScaleObject(nodeconnections[iNodeConnectionIndex].middle, 35, 35, 35);
		}
		else if (iNodeConnectionIndex >= 0)
		{
			ScaleObject(nodeconnections[iNodeConnectionIndex].middle, 25, 25, 25);
		}

		if (iTotalMiddle < MAXDOTMIDDLE)
		{
			//PE: Middle add a dropdown gadget.
			CreateDotMiddleObject(DOTMIDDLEOBJECTID + iTotalMiddle);

			// Create/Update the lines that travel between the objects.
			t.activerelationobjectid = RELATIONOBJECTID + iTotalMiddle + 1;
			if (ObjectExist(RELATIONOBJECTID + iTotalMiddle + 1) == 0)
			{
				CreateObjectRelationMesh(ffromx, ffromy, ffromz, ftox, ftoy, ftoz, dwNewColor);
				NodeConnection connection;
				connection.from = sobj; connection.to = dobj; connection.nodeconnectionid = RELATIONOBJECTID + iTotalMiddle + 1;
				connection.middle = DOTMIDDLEOBJECTID + iTotalMiddle;
				nodeconnections.push_back(connection);
			}
			else
			{
				SetObjectDiffuse(t.activerelationobjectid, dwNewColor);

				if (nodeconnections[iTotalMiddle].from == sobj && nodeconnections[iTotalMiddle].to == dobj && forceVertUpdate == 0)
				{
					// No vertex update necessary.
					UpdateObjectRelationUVs(t.activerelationobjectid);
				}
				else
				{
					// Update verts and UVs.
					nodeconnections[iTotalMiddle].from = sobj;
					nodeconnections[iTotalMiddle].to = dobj;
					UpdateObjectRelationMesh(ffromx, ffromy, ffromz, ftox, ftoy, ftoz);
				}
				ShowObject(RELATIONOBJECTID + iTotalMiddle + 1);
			}
			//CreateDotMiddleObject(iTotalMiddle);
			ShowObject(DOTMIDDLEOBJECTID + iTotalMiddle);
			PositionObject(DOTMIDDLEOBJECTID + iTotalMiddle, ffromx, ffromy, ffromz);
			PointObject(DOTMIDDLEOBJECTID + iTotalMiddle, ftox, ftoy, ftoz);
			MoveObject(DOTMIDDLEOBJECTID + iTotalMiddle, fDistance*0.5);

			iDotMiddleInfoSource[iTotalMiddle] = from;
			iDotMiddleInfoDestination[iTotalMiddle] = to;
	
			iTotalMiddle++;
			if (iTotalMiddle > iLargestMiddle)
				iLargestMiddle = iTotalMiddle;
		}
	}
}

void deleterelationobjects()
{
	for (int i = RELATIONOBJECTID; i < RELATIONOBJECTID + RELATIONOBJECTMAX; i++)
	{
		if (ObjectExist(i))
			DeleteObject(i);
	}
}

///

float ImGuiGetMouseX( void )
{
	RECT rect = { NULL };
	GetWindowRect(g_pGlob->hWnd, &rect);
	return(t.inputsys.xmouse - rect.left);
}

float ImGuiGetMouseY(void)
{
	RECT rect = { NULL };
	GetWindowRect(g_pGlob->hWnd, &rect);
	return(t.inputsys.ymouse - rect.top);
}


//PE:Turning 180 make artifacts in colors, and are hard to control. it get reflections/light from env.
//#define TURNBACKDROP180
//#define MOVEPROBETO_BLACK //Move probe to black area, for better control of colors. This make titanium objects black, without ambient control this will not work.

bool bBackdropSettingsSet = false;
bool old_g_bNoSwapchainPresent = false;
float composx;
float composy;
float composz;
float comangx;
float comangy;
float comangz;
int iFogChangedFramesBeforeRestore = 0;

void CreateBackdropObject(bool bForceRecreate,cstr newImageFile,cstr fpefile)
{
	if(cUseBackbufferCubemap.Len() > 0)
		RevertBackbufferCubemap();
	cUseBackbufferCubemap = "";
	
	if (newImageFile.Len() > 0)
	{
		if (cCurrentBackDropImageFile != newImageFile)
		{
			// this is the new current backdrop
			cCurrentBackDropImageFile = newImageFile;

			// delete if old one exists
			image_setlegacyimageloading(true);
			if (ImageExist(BACKDROPMAGE))
				DeleteImage(BACKDROPMAGE);
			image_setlegacyimageloading(false);

			// LB: Prefer DDS if available
			LPSTR pLoadThisBackdrop = cCurrentBackDropImageFile.Get();
			char pDDSVariantOfBackdropFile[MAX_PATH];
			strcpy ( pDDSVariantOfBackdropFile, cCurrentBackDropImageFile.Get());
			pDDSVariantOfBackdropFile[strlen(pDDSVariantOfBackdropFile) - 4] = 0;
			strcat (pDDSVariantOfBackdropFile, ".dds");
			if (FileExist(pDDSVariantOfBackdropFile) == 1)
				pLoadThisBackdrop = pDDSVariantOfBackdropFile;

			// load for IMGUI to display
			image_setlegacyimageloading(true);
			LoadImage(pLoadThisBackdrop, BACKDROPMAGE);
			image_setlegacyimageloading(false);
			bForceRecreate = true;
		}
	}

	//Simple Sky.
	if((!ImageExist(BACKDROPMAGE) || newImageFile == "None") && fpefile.Len() > 0)
	{
		if (pestrcasestr(fpefile.Get(), ".fpe"))
		{
			std::string sString = fpefile.Get();
			replaceAll(sString, ".fpe", "_fpe_cube.dds");
			cUseBackbufferCubemap = cstr("entitybank\\") + cstr( (char *) sString.c_str());
			// showcase\titanium_fpe_cube.dds
			if (FileExist(cUseBackbufferCubemap.Get()))
			{
				wiScene::WeatherComponent* weather = wiScene::GetScene().weathers.GetComponent(g_weatherEntityID);
				if (weather->skyMap != nullptr && weather->skyMapName.length() > 0)
				{
					//PE: Make sure to free any old resources.
					WickedCall_DeleteImage(weather->skyMapName);
				}
				weather->skyMapName = cUseBackbufferCubemap.Get();
				weather->skyMap = WickedCall_LoadImage(weather->skyMapName);
				weather->cloudiness = 0.0f;
				weather->cloudSpeed = 0.0f;

				// update cubes
				WickedCall_DisplayCubes(false);

				if (ObjectExist(t.terrain.terrainobjectindex) == 1)
				{
					HideObject(t.terrain.terrainobjectindex);
				}
				t.hardwareinfoglobals.noterrain = 1;

				float centerx = -1000, centery = 39000, centerz = -1000;
				WickedCall_MoveReflectionProbe(centerx, centery, centerz, "editorProbe", 500);
				WickedCall_UpdateProbes();
				bBackbufferCubemapActive = true;
			}
			else
			{
				cUseBackbufferCubemap = "";
			}
		}
	}

	int backdropobj = BACKDROPMAGE;
	if (bForceRecreate || (bUseBackDropImage && !ObjectExist(backdropobj)))
	{
		if (ObjectExist(backdropobj))
			DeleteObject(backdropobj);
		//float fDist = 6.5f;
		float fDist = 13.0f;
		MakeObjectPlane(backdropobj, 1920 * fDist, 1080 * fDist);
		//PE: No light.
		LockVertexDataForLimbCore(backdropobj, 0, 1);
		SetVertexDataNormals(0, 0, 1, 0);
		SetVertexDataNormals(1, 0, 1, 0);
		SetVertexDataNormals(2, 0, 1, 0);
		SetVertexDataNormals(3, 0, 1, 0);
		SetVertexDataNormals(4, 0, 1, 0);
		SetVertexDataNormals(5, 0, 1, 0);
		UnlockVertexData();
		#ifndef TURNBACKDROP180
		//Flip image.
		float U_f = 1.0f , V_f = 0.0f, D_f = -1.0f;
		LockVertexDataForLimb(backdropobj, 0);
		SetVertexDataUV(0, U_f, V_f);
		SetVertexDataUV(1, U_f + D_f, V_f);
		SetVertexDataUV(2, U_f + D_f, V_f + 1.0f);
		SetVertexDataUV(3, U_f + D_f, V_f + 1.0f);
		SetVertexDataUV(4, U_f, V_f + 1.0f);
		SetVertexDataUV(5, U_f, V_f);
		UnlockVertexData();
		#endif

		FixObjectPivot(backdropobj);
		SetObjectTransparency(backdropobj, 1);
		SetObjectCollisionOff(backdropobj);
		SetObjectTextureMode(backdropobj, 0, 0);
		SetObjectLight(backdropobj, 0);
		SetObjectMask(backdropobj, 1);
		if(cCurrentBackDropImageFile.Len() > 0 && ImageExist(BACKDROPMAGE))
			TextureObject(backdropobj, BACKDROPMAGE);
		SetObjectCull(backdropobj, 0);
		sObject* pBackObject = GetObjectData(backdropobj);
		if (pBackObject)
		{
			//Make sure we flip UV , image is inverted in the x dir.
			if (pBackObject->ppMeshList)
			{
				sMesh* pMesh = pBackObject->ppMeshList[0];
				if (pMesh) WickedCall_UpdateMeshVertexData(pMesh);
			}

			WickedCall_SetObjectCastShadows(pBackObject, false);

			float fColorR,fColorG,fColorB;
			if (cCurrentBackDropImageFile.Len() > 0 && ImageExist(BACKDROPMAGE))
			{
				fColorR = 1.0f;
				fColorG = 1.0f;
				fColorB = 1.0f;
			}
			else
			{
				//Default thumb color , changed for unlit shader.
				fColorR = 0.32f;
				fColorG = 0.32f;
				fColorB = 0.32f;

			}

			for (int iMesh = 0; iMesh < pBackObject->iMeshCount; iMesh++)
			{
				sMesh* pMesh = pBackObject->ppMeshList[iMesh];
				if (pMesh)
				{
					//Boost colors. we are way out there in 3D space.
					#ifdef TURNBACKDROP180
					pMesh->mMaterial.Diffuse.r = fColorR * 2.0f;
					pMesh->mMaterial.Diffuse.g = fColorG * 2.0f;
					pMesh->mMaterial.Diffuse.b = fColorB * 2.0f;
					#else

					if (cCurrentBackDropImageFile.Len() > 0 && ImageExist(BACKDROPMAGE))
					{
						//Place above water level.
						pMesh->mMaterial.Diffuse.r = fColorR; // *1.0f;
						pMesh->mMaterial.Diffuse.g = fColorG; // *1.0f;
						pMesh->mMaterial.Diffuse.b = fColorB; // *1.0f;
					}
					else
					{
						//ENV-Probe placed at play area.
						pMesh->mMaterial.Diffuse.r = fColorR * 2.5f;
						pMesh->mMaterial.Diffuse.g = fColorG * 2.5f;
						pMesh->mMaterial.Diffuse.b = fColorB * 2.5f;
					}

					#endif
					pMesh->mMaterial.Diffuse.a = 1.0f;
					wiScene::MeshComponent* mesh = wiScene::GetScene().meshes.GetComponent(pMesh->wickedmeshindex);
					if (mesh)
					{
						uint64_t materialEntity = mesh->subsets[0].materialID;
						wiScene::MaterialComponent* pObjectMaterial = wiScene::GetScene().materials.GetComponent(materialEntity);
						if (pObjectMaterial)
						{
							pObjectMaterial->SetReflectance(0.0f);
							pObjectMaterial->shaderType = wiScene::MaterialComponent::SHADERTYPE_UNLIT; //PE: Yes 1:1 mapping and no light,env... :)
							//PE: Also ignoes all other material settings , so its perfect.
							pObjectMaterial->SetDirty(true);
						}
					}
					WickedCall_SetMeshMaterial(pMesh,true);
				}
			}
			WickedCall_SetObjectMetalness(pBackObject, 0.0f);
			WickedCall_SetObjectRoughness(pBackObject, 0.0f);
		}
	}
	if (ObjectExist(backdropobj))
		HideObject(backdropobj);
	WickedCall_DisplayCubes(false);

	if (!bRotateBackBuffer) {
		//Dont disable bloom when hover over , so the scene behind dont flash with bloom on / off.
		//WickedCall_SetSunDirection(45.0f, 270.0f, 0.0f);
		master_renderer->setBloomEnabled(false);
	}
	//PE: Always move probe, as terrain is now not grass.
	float centerx = -1000, centery = 190000000, centerz = -1000;
	//PE: This pos is black.
	#ifdef MOVEPROBETO_BLACK
	WickedCall_MoveReflectionProbe(centerx, centery, centerz, "editorProbe", 500);
	#endif

	//PE: Disable fog.
	float oldFogNear = t.visuals.FogNearest_f;
	float oldFogFar = t.visuals.FogDistance_f;
	//Now always use thumb light.
	WickedCall_EnableThumbLight(true);

	extern bool g_bNoSwapchainPresent;
	old_g_bNoSwapchainPresent = g_bNoSwapchainPresent;

	composx = CameraPositionX(0);
	composy = CameraPositionY(0);
	composz = CameraPositionZ(0);
	comangx = CameraAngleX(0);
	comangy = CameraAngleY(0);
	comangz = CameraAngleZ(0);

	bBackdropSettingsSet = true;
}

extern Master master;

void StartForceRender(void)
{
	extern bool g_bNoGGUntilGameGuruMainCalled;
	//PE: Cant use forcerender until init is done.
	if (!g_bNoGGUntilGameGuruMainCalled) return;
	extern bool bSkipAllGameLogic;
	bSkipAllGameLogic = true;
	//PE: Empty messages , so windows dont think we are dead. ( perhaps remember QUIT ? )
	MSG msg = { 0 };
	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	master.RunCustom();
	//Give some time for wicked to finish its jobs.
	Sleep(1);
	bSkipAllGameLogic = false;
	return;
}

int current_backbuffer_width = 0;
int current_backbuffer_height = 0;
int current_backbuffer_grabimg = 0;

void GrabBackBufferForAnImage(void)
{
	extern int g_iActivelyUsingVRNow;
	if (g.vrglobals.GGVREnabled > 0 && g_iActivelyUsingVRNow == 1 && t.game.activeStoryboardScreen > -1)
	{
		// instruct to grab screen 
		BackBufferSaveCacheName = "";
		current_backbuffer_grabimg = g.importermenuimageoffset + 50;
		BackBufferImageID = current_backbuffer_grabimg;
		BackBufferZoom = 1.0f;
		BackBufferCamLeft = 0.0f;
		BackBufferCamUp = 0.0f;
		bRotateBackBuffer = false;
		bLoopBackBuffer = true;
		BackBufferObjectID = 0;
		BackBufferGrabGameScreen = true;
		BackBufferSizeX = 1920;
		BackBufferSizeY = 1080;
		bFullScreenBackbuffer = true;

		// quad to view HUD screen
		bool bShowMe = true;
		if (bShowMe == true)
		{
			if (ObjectExist(g.hudscreen3dobjectoffset) == 0)
			{
				float fCorrectWidth = 192.0f / 5.0f;
				float fCorrectHeight = 108.0f / 5.0f;
				int backdropobj = g.hudscreen3dobjectoffset;
				MakeObjectPlane(backdropobj, fCorrectWidth, fCorrectHeight);
				LockVertexDataForLimbCore(backdropobj, 0, 1);
				SetVertexDataNormals(0, 0, 1, 0);
				SetVertexDataNormals(1, 0, 1, 0);
				SetVertexDataNormals(2, 0, 1, 0);
				SetVertexDataNormals(3, 0, 1, 0);
				SetVertexDataNormals(4, 0, 1, 0);
				SetVertexDataNormals(5, 0, 1, 0);
				UnlockVertexData();
				float U_f = 0.0f, V_f = 0.0f, D_f = 1.0f;
				LockVertexDataForLimb(backdropobj, 0);
				SetVertexDataUV(0, U_f, V_f);
				SetVertexDataUV(1, U_f + D_f, V_f);
				SetVertexDataUV(2, U_f + D_f, V_f + 1.0f);
				SetVertexDataUV(3, U_f + D_f, V_f + 1.0f);
				SetVertexDataUV(4, U_f, V_f + 1.0f);
				SetVertexDataUV(5, U_f, V_f);
				UnlockVertexData();
				FixObjectPivot(backdropobj);
				SetObjectTransparency(backdropobj, 0);// 1); transparency mode (preferred) is WAT TOO DIM!
				SetObjectCollisionOff(backdropobj);
				SetObjectTextureMode(backdropobj, 0, 0);
				SetObjectLight(backdropobj, 0);
				SetObjectCull(backdropobj, 0);
				sObject* pBackObject = GetObjectData(backdropobj);
				if (pBackObject)
				{
					if (pBackObject->ppMeshList)
					{
						sMesh* pMesh = pBackObject->ppMeshList[0];
						if (pMesh) WickedCall_UpdateMeshVertexData(pMesh);
					}
					WickedCall_SetObjectCastShadows(pBackObject, false);
					float fColorR, fColorG, fColorB;
					fColorR = 1.0f;
					fColorG = 1.0f;
					fColorB = 1.0f;
					for (int iMesh = 0; iMesh < pBackObject->iMeshCount; iMesh++)
					{
						sMesh* pMesh = pBackObject->ppMeshList[iMesh];
						if (pMesh)
						{
							pMesh->mMaterial.Diffuse.r = fColorR; // *1.0f;
							pMesh->mMaterial.Diffuse.g = fColorG; // *1.0f;
							pMesh->mMaterial.Diffuse.b = fColorB; // *1.0f;
							pMesh->mMaterial.Diffuse.a = 1.0f;
							wiScene::MeshComponent* mesh = wiScene::GetScene().meshes.GetComponent(pMesh->wickedmeshindex);
							if (mesh)
							{
								uint64_t materialEntity = mesh->subsets[0].materialID;
								wiScene::MaterialComponent* pObjectMaterial = wiScene::GetScene().materials.GetComponent(materialEntity);
								if (pObjectMaterial)
								{
									pObjectMaterial->SetReflectance(0.0f);
									pObjectMaterial->shaderType = wiScene::MaterialComponent::SHADERTYPE_UNLIT;
									pObjectMaterial->SetDirty(true);
								}
							}
						}
						WickedCall_SetMeshMaterial(pMesh,true);
					}
					WickedCall_SetObjectMetalness(pBackObject, 0.0f);
					WickedCall_SetObjectRoughness(pBackObject, 0.0f);
				}
				SetObjectMask (g.hudscreen3dobjectoffset, (1 << 6) + (1 << 7) + 1);
			}
			if (ObjectExist(g.hudscreen3dobjectoffset) == 1)
			{
				float fX = CameraPositionX(0);
				float fY = CameraPositionY(0);
				float fZ = CameraPositionZ(0);
				PositionObject (g.hudscreen3dobjectoffset, fX, fY, fZ);
				
				//not reliable in VR mode
				RotateObject(g.hudscreen3dobjectoffset, 0, t.playercontrol.cy_f, 0);
				MoveObject(g.hudscreen3dobjectoffset, 20.0f);

				sObject* pHUDScreenObject = GetObjectData(g.hudscreen3dobjectoffset);
				if (pHUDScreenObject)
				{
					TextureObject (g.hudscreen3dobjectoffset, current_backbuffer_grabimg);
					WickedCall_TextureObjectWithImagePtr(pHUDScreenObject, 0);
				}
				ShowObject(g.hudscreen3dobjectoffset);
				if (t.currentgunobj > 0 && ObjectExist(t.currentgunobj) == 1) HideObject(t.currentgunobj);
			}
		}
	}
	else
	{
		if (ObjectExist(g.hudscreen3dobjectoffset) == 1)
		{
			HideObject(g.hudscreen3dobjectoffset);
			if (t.currentgunobj > 0 && ObjectExist(t.currentgunobj) == 1) ShowObject(t.currentgunobj);
		}
	}
}

