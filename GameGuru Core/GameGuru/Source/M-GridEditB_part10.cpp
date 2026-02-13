#define MINMAX_WINDOWS 10
ImRect rRetoreWindowSize[MINMAX_WINDOWS];
bool rMinMaxState[MINMAX_WINDOWS];
bool rMinMaxTrigger[MINMAX_WINDOWS];
bool rMinMaxInit[MINMAX_WINDOWS] = { false,false,false,false,false,false,false,false,false,false };
bool bMinMaxGlobalInit = false;
bool imgui_GetMinMaxButtonState(int win) { return rMinMaxState[win]; }
bool imgui_CheckMinMaxStartupState(int win)
{
	ImVec2 viewPortPos = ImGui::GetMainViewport()->Pos;
	ImVec2 viewPortSize = ImGui::GetMainViewport()->Size * 0.95;
	ImVec2 vCurSize = ImGui::GetWindowSize();
	if (vCurSize.x >= viewPortSize.x && vCurSize.y >= viewPortSize.y)
	{
		//bResetObjectLibrarySize = true;
		//PE: Instead of resetting window to default , set it as maximized.
		rMinMaxState[win] = true;
		//ImGui::SetNextWindowSize(ImVec2(66 * ImGui::GetFontSize(), (43 * ImGui::GetFontSize()) + 19.0), ImGuiCond_Always); //ImGuiCond_FirstUseEver
		ImVec2 vSize = ImVec2(66 * ImGui::GetFontSize(), (43 * ImGui::GetFontSize()) + 19.0);
		ImVec2 vCenter = viewPortPos + (ImGui::GetMainViewport()->Size*0.5);
		vCenter -= vSize * 0.5;
		rRetoreWindowSize[win].Min = vCenter;
		rRetoreWindowSize[win].Max = vSize;
		return true;
	}
	return false;
}

bool imgui_AddMinMaxButton(int win, bool bRestore)
{
	bool bRet = false;
	if (win >= MINMAX_WINDOWS || win < 0) return bRet;

	if (!bMinMaxGlobalInit)
	{
		for (int i = 0;i < MINMAX_WINDOWS; i++)
		{
			rMinMaxInit[i] = false;
		}
		bMinMaxGlobalInit = true;
	}
	if (!rMinMaxInit[win])
	{
		rRetoreWindowSize[win].Min = ImGui::GetWindowPos();
		rRetoreWindowSize[win].Max = ImGui::GetWindowSize();
		rMinMaxState[win] = false;
		rMinMaxTrigger[win] = false;
		rMinMaxInit[win] = true;
	}

	if (!bRestore)
	{
		//Display buttons.
		ImRect avail_window_rect;
		avail_window_rect.Min = ImGui::GetWindowPos();
		avail_window_rect.Max = ImGui::GetWindowPos() + ImGui::GetWindowSize();
		#define USEARROWBUTTON
		ImGui::PushClipRect(avail_window_rect.Min, avail_window_rect.Max, false);
		#ifdef USEARROWBUTTON
			ImVec2 vMinMaxButton = ImVec2(ImGui::GetWindowSize().x - 38.0f, 4.0);
		#else
			ImVec2 vMinMaxButton = ImVec2(ImGui::GetWindowSize().x - 44.0f, -2.0);
		#endif
		ImGui::SetCursorPos(vMinMaxButton);
		ImGui::SetItemAllowOverlap();
		int iIcon = MEDIA_MINIMIZE;
		int iDirection = ImGuiDir_Down;
		if (rMinMaxState[win] == false) {
			iIcon = MEDIA_MAXIMIZE;
			iDirection = ImGuiDir_Up;
		}

		ImVec4 col = ImGui::GetStyleColorVec4(ImGuiCol_Text);
		ImU32 text_col = ImGui::GetColorU32(ImGuiCol_Text);

		#ifdef USEARROWBUTTON
		auto *style = &ImGui::GetStyle();
		float ioldframe = style->FrameBorderSize;
		ImVec2 oldpadding = style->FramePadding;
		style->FrameBorderSize = 0.0f;
		style->FramePadding.x = 0;
		style->FramePadding.y = 0;
		if (ImGui::MinMaxButtonEx("##minmaxSelection", iDirection))
		#else
		if (ImGui::ImgBtn(iIcon, ImVec2(20, 20), ImColor(255, 255, 255, 0), ImColor(255, 255, 255, 255), drawCol_hover, drawCol_Down, -1, 0, 0, 0, true, false, false, false, false, bBoostIconColors))
		#endif
		{
			if (rMinMaxState[win] == false)
				rMinMaxState[win] = true;
			else
				rMinMaxState[win] = false;
			rMinMaxTrigger[win] = true;
			if (rMinMaxState[win])
			{
				//Update size pos with current windows settings.
				rRetoreWindowSize[win].Min = ImGui::GetWindowPos();
				rRetoreWindowSize[win].Max = ImGui::GetWindowSize();
			}
			bRet = true;
		}
		#ifdef USEARROWBUTTON
		style->FramePadding = oldpadding;
		style->FrameBorderSize = ioldframe;
		#endif
		ImGui::PopClipRect();
	}
	else
	{
		if (rMinMaxTrigger[win])
		{
			if (rMinMaxState[win])
			{
				//Set max size, but keep old size.
				ImVec2 viewPortPos = ImGui::GetMainViewport()->Pos;
				ImVec2 viewPortSize = ImGui::GetMainViewport()->Size;
				ImGui::SetNextWindowPos(viewPortPos, ImGuiCond_Always);
				ImGui::SetNextWindowSize(viewPortSize, ImGuiCond_Always); //full screen.
			}
			else
			{
				//Restore old settings.
				ImGui::SetNextWindowPos(rRetoreWindowSize[win].Min, ImGuiCond_Always);
				ImGui::SetNextWindowSize(rRetoreWindowSize[win].Max, ImGuiCond_Always);
			}
			rMinMaxTrigger[win] = false;
		}
	}
	return bRet;
}

void FreeTempImageList(void)
{
	if (g_TempimageList.size() > 0)
	{
		//t.entid = 0 ; is used for automated generating of thumbs, so delete this.
		if (ObjectExist(g.entitybankoffset)) 
		{
			DeleteObject(g.entitybankoffset);
		}
		if (iRestoreEntidMaster >= 0 && g.entidmaster > iRestoreEntidMaster)
		{
			//PE: Free loaded objects
			for (int i = iRestoreEntidMaster;i < g.entidmaster; i++)
			{
				int iEntId = i + 1;
				cstr sFree = t.entitybank_s[iEntId];
				if (ObjectExist(g.entitybankoffset + iEntId)) 
				{
					//PE: We use a before/after list to free all used textures later.
					DeleteObject(g.entitybankoffset + iEntId);
				}
			}
		}

		// restore g.entidmaster
		if (iRestoreEntidMaster >= 0)
		{
			g.entidmaster = iRestoreEntidMaster;
		}
		iRestoreEntidMaster = -1;

		//PE: TODO check legacy loaded images is part of g_imageList, should not be a problem as they are not actually inside wicked, check anyway.

		//PE: Compare the lists and check if we need to delete any textures.
		for (int i = 0; i < g_imageList.size(); i++)
		{
			sImageList* pImage = NULL;
			if (i >= g_TempimageList.size())
			{
				pImage = &g_imageList[i];
			}
			else
			{
				if (g_imageList[i].image != g_TempimageList[i].image)
				{
					pImage = &g_imageList[i];
				}
			}
			if (pImage)
			{
				// free texture
				WickedCall_FreeImage(pImage);
			}
		}

		// clear temp image list
		g_TempimageList.clear();
	}
}

bool DeleteEntityFromLists(int e)
{
	bool bFound = false;
	for (int l = 0; l < MAXGROUPSLISTS; l++)
	{
		if (vEntityGroupList[l].size() > 0)
		{

			for (int i = 0; i < vEntityGroupList[l].size(); i++)
			{
				if (e == vEntityGroupList[l][i].e)
				{
					vEntityGroupList[l].erase(vEntityGroupList[l].begin() + i);
					bFound = true;
					break;
				}
			}
		}
		if(bFound)
			break;
	}
	if (!bFound)
	{
		for (int i = 0; i < vEntityLockedList.size(); i++)
		{
			if (e == vEntityLockedList[i].e)
			{
				vEntityLockedList.erase(vEntityLockedList.begin() + i);
				bFound = true;
				break;
			}
		}
	}
	//Recursive as list's is changing.
	if (bFound)
		DeleteEntityFromLists(e);
	return(bFound);
}


void AddGroupListToRubberBand(int l)
{
	bool bFound = false;
	if (g.entityrubberbandlist.size() <= 0) {
		g.entityrubberbandlist = vEntityGroupList[l];
		return;
	}

	if (vEntityGroupList[l].size() > 0)
	{
		for (int i = 0; i < vEntityGroupList[l].size(); i++)
		{
			int e = vEntityGroupList[l][i].e;
			bool found = false;
			for (int l = 0; l < g.entityrubberbandlist.size(); l++)
			{
				if (g.entityrubberbandlist[l].e == e) {
					found = true;
				}
			}
			if (!found)
			{
				sRubberBandType rubberbandItem;
				rubberbandItem.e = e;
				rubberbandItem.x = t.entityelement[e].x;
				rubberbandItem.y = t.entityelement[e].y;
				rubberbandItem.z = t.entityelement[e].z;
				rubberbandItem.px = t.entityelement[e].x;
				rubberbandItem.py = t.entityelement[e].y;
				rubberbandItem.pz = t.entityelement[e].z;
				rubberbandItem.rx = t.entityelement[e].rx;
				rubberbandItem.ry = t.entityelement[e].ry;
				rubberbandItem.rz = t.entityelement[e].rz;			
				rubberbandItem.quatmode = t.entityelement[e].quatmode;
				rubberbandItem.quatx = t.entityelement[e].quatx;
				rubberbandItem.quaty = t.entityelement[e].quaty;
				rubberbandItem.quatz = t.entityelement[e].quatz;
				rubberbandItem.quatw = t.entityelement[e].quatw;
				rubberbandItem.scalex = t.entityelement[e].scalex;
				rubberbandItem.scaley = t.entityelement[e].scaley;
				rubberbandItem.scalez = t.entityelement[e].scalez;
				g.entityrubberbandlist.push_back(rubberbandItem);
			}
		}
	}
	return;
}

void CheckGroupListForRubberbandSelections(int entityindex)
{
	int grouplist = isEntityInGroupList(entityindex);
	if (grouplist >= 0)
	{
		//Check all objects in the current rubberband if they belong to a group.
		for (int i = 0; i < g.entityrubberbandlist.size(); i++)
		{
			int e = g.entityrubberbandlist[i].e;
			if (e > 0)
			{
				int group = isEntityInGroupList(e, grouplist);
				if (group >= 0)
					AddGroupListToRubberBand(group);
			}
		}

		//PE: We must add to current rubberband.
		AddGroupListToRubberBand(grouplist);
	}
	
	//Scan entityindex in all groups.
	for (int l = 0; l < MAXGROUPSLISTS; l++)
	{
		if (l != grouplist)
		{
			int group = isEntityInGroupListDirect(entityindex, l);
			if (group >= 0) {
				AddGroupListToRubberBand(group);
			}
		}
	}

	//Recheck new rubberband if any new entity is in other lists.
	for (int i = 0; i < g.entityrubberbandlist.size(); i++)
	{
		int e = g.entityrubberbandlist[i].e;
		if (e > 0)
		{
			//Check all groups.
			for (int l = 0; l < MAXGROUPSLISTS; l++)
			{
				int group = isEntityInGroupListDirect(e, l);
				if (group >= 0) {
					AddGroupListToRubberBand(group);
				}
			}
		}
	}
}

void ClearAllGroupLists(void)
{
	for (int l = 0; l < MAXGROUPSLISTS; l++)
	{
		sEntityGroupListName[l] = "";
		vEntityGroupList[l].clear();
		iEntityGroupListImage[l] = 0;
	}
	for (int i = 0; i < vEntityLockedList.size(); i++)
	{
		int e = vEntityLockedList[i].e;
		if (e > 0 && e < t.entityelement.size())
		{
			t.entityelement[e].editorlock = 0;
			sObject* pObject;
			int obj = t.entityelement[e].obj;
			if (obj > 0 && obj < g_iObjectListCount)
			{
				pObject = g_ObjectList[obj];
				if (pObject) 
				{
					WickedCall_SetObjectRenderLayer(pObject, GGRENDERLAYERS_NORMAL);
				}
			}
		}
	}
	vEntityLockedList.clear();
	g.entityrubberbandlist.clear();
}


void ReplaceEntityInGroupList(int e, int eto)
{
	for (int l = 0; l < MAXGROUPSLISTS; l++)
	{
		if (vEntityGroupList[l].size() > 0)
		{

			for (int i = 0; i < vEntityGroupList[l].size(); i++)
			{
				if (e == vEntityGroupList[l][i].e)
				{
					vEntityGroupList[l][i].e = eto;
				}
			}
		}
	}
}

int isEntityInGroupList(int e,int ignoregroup)
{
	bool bFound = false;
	for (int l = 0; l < MAXGROUPSLISTS; l++)
	{
		if (!(ignoregroup >= 0 && ignoregroup == l))
		{
			if (vEntityGroupList[l].size() > 0)
			{
				for (int i = 0; i < vEntityGroupList[l].size(); i++)
				{
					if (e == vEntityGroupList[l][i].e)
					{
						//found return list.
						return(l);
					}
				}
			}
		}
	}
	return(-1);
}

int isEntityInGroupListDirect(int e, int group)
{
	if (vEntityGroupList[group].size() > 0)
	{
		for (int i = 0; i < vEntityGroupList[group].size(); i++)
		{
			if (e == vEntityGroupList[group][i].e)
			{
				//found return list.
				return(group);
			}
		}
	}

	return(-1);
}

int g_iCopiedLogicConnectionsCount = 0;
struct sCopiedLogicConnections
{
	int iObjectLinkID[999];
	int iObjectRelationships[999][10];
	int iObjectRelationshipsType[999][10];
	int iObjectRelationshipsData[999][10];
};
std::vector<sCopiedLogicConnections> g_copiedLogicConnectionList;

void DuplicateLogicConnectionsCopyOriginal (std::vector<sRubberBandType> vEntityDuplicateList, int iOriginalGroupIndexSpecified)
{
	if (iOriginalGroupIndexSpecified > -1)
	{
		// the size of the list 
		g_iCopiedLogicConnectionsCount = (int)vEntityDuplicateList.size();

		// expand storage to include iOriginalGroupIndexSpecified index
		while (g_copiedLogicConnectionList.size() <= iOriginalGroupIndexSpecified)
		{
			sCopiedLogicConnections item;
			g_copiedLogicConnectionList.push_back(item);
		}

		// store data for group list
		sCopiedLogicConnections item;
		for (int listindex = 0; listindex < g_iCopiedLogicConnectionsCount; listindex++)
		{
			// original connection data
			int iOriginalE = vEntityDuplicateList[listindex].e;
			item.iObjectLinkID[listindex] = t.entityelement[iOriginalE].eleprof.iObjectLinkID;;
			for (int i = 0; i < 10; i++)
			{
				item.iObjectRelationships[listindex][i] = t.entityelement[iOriginalE].eleprof.iObjectRelationships[i];
				item.iObjectRelationshipsType[listindex][i] = t.entityelement[iOriginalE].eleprof.iObjectRelationshipsType[i];
				item.iObjectRelationshipsData[listindex][i] = t.entityelement[iOriginalE].eleprof.iObjectRelationshipsData[i];
			}
		}
		g_copiedLogicConnectionList[iOriginalGroupIndexSpecified] = item;
	}
}

void DuplicateLogicConnections (std::vector<sRubberBandType> vEntityDuplicateList, int iOriginalGroupIndexSpecified)
{
	if (g_iCopiedLogicConnectionsCount > 0)
	{
		// have 'vEntityDuplicateList' which is the original and g.entityrubberbandlist which is the duplicated copy
		sCopiedLogicConnections item = g_copiedLogicConnectionList[iOriginalGroupIndexSpecified];
		
		if (g_iCopiedLogicConnectionsCount != g.entityrubberbandlist.size())
		{
			//PE: Somehow g_iCopiedLogicConnectionsCount is larger then g.entityrubberbandlist.size() and you end up with a crash.
			//@Lee: is g_iCopiedLogicConnectionsCount used any more.
			//PE: Anyway protect it for now. Happens if you have one large group add another small group and save.
			g_iCopiedLogicConnectionsCount = g.entityrubberbandlist.size();
		}
		// scan old data, find old LinkIDs and replace with new ones
		for (int listindex = 0; listindex < g_iCopiedLogicConnectionsCount; listindex++)
		{
			// for each entity relationships, assign new LinkIDs
			int iDuplicateE = g.entityrubberbandlist[listindex].e;
			t.entityelement[iDuplicateE].eleprof.iObjectLinkID = item.iObjectLinkID[listindex];
			for (int i = 0; i < 10; i++)
			{
				t.entityelement[iDuplicateE].eleprof.iObjectRelationships[i] = item.iObjectRelationships[listindex][i];
				t.entityelement[iDuplicateE].eleprof.iObjectRelationshipsType[i] = item.iObjectRelationshipsType[listindex][i];
				t.entityelement[iDuplicateE].eleprof.iObjectRelationshipsData[i] = item.iObjectRelationshipsData[listindex][i];
			}
		}

		// completed logic duplication
		g_iCopiedLogicConnectionsCount = 0;
	}
}

int DuplicateFromListToCursor(std::vector<sRubberBandType> vEntityDuplicateList, bool bRandomShiftXZ, int iOriginalGroupIndexForChild, bool bAttachToCursor)
{
	// anchor to a single object for dragging
	int iAnchorEntityIndex = -1;

	// Dublicate all from a list.
	if (vEntityDuplicateList.size() > 0)
	{
		// a small offset so user can see new pasted entity
		float fShiftOffsetForPasteX = 0.0f;
		float fShiftOffsetForPasteZ = 0.0f;
		if (bRandomShiftXZ == true)
		{
			fShiftOffsetForPasteX = 50.0f + rand() % 50;
			fShiftOffsetForPasteZ = 50.0f + rand() % 50;
			if (rand() % 2 == 0) fShiftOffsetForPasteX = -fShiftOffsetForPasteX;
			if (rand() % 2 == 0) fShiftOffsetForPasteZ = -fShiftOffsetForPasteZ;
		}

		// we are also going to move rubber band selection to new pasted group
		g.entityrubberbandlist.clear();

		float higesty = -999999.0f, lowesty = 999999.0;
		// for each entity, create a duplicate and offset slightly so we can see it
		for (int i = 0; i < (int)vEntityDuplicateList.size(); i++)
		{
			// duplicate new entity as clone of relevant original clipboard entity
			int e = vEntityDuplicateList[i].e;
			bool bLowestFound = false;
			t.gridentity = t.entityelement[e].bankindex;
			//PE: all t.gridentity... need to be set for this to work correctly.
			t.entid = t.gridentity;
			entity_fillgrideleproffromprofile();  // t.entid
			t.gridentityposx_f = t.entityelement[e].x;
			t.gridentityposy_f = t.entityelement[e].y;
			if (higesty < t.gridentityposy_f) higesty = t.gridentityposy_f;
			if (lowesty > t.gridentityposy_f)
			{
				lowesty = t.gridentityposy_f;
				bLowestFound = true;
			}
			t.gridentityposz_f = t.entityelement[e].z;
			t.gridentityrotatex_f = t.entityelement[e].rx;
			t.gridentityrotatey_f = t.entityelement[e].ry;
			t.gridentityrotatez_f = t.entityelement[e].rz;
			t.gridentityrotatequatmode = t.entityelement[e].quatmode;
			t.gridentityrotatequatx_f = t.entityelement[e].quatx;
			t.gridentityrotatequaty_f = t.entityelement[e].quaty;
			t.gridentityrotatequatz_f = t.entityelement[e].quatz;
			t.gridentityrotatequatw_f = t.entityelement[e].quatw;
			if (t.entityprofile[t.gridentity].ismarker == 10)
			{
				t.gridentityscalex_f = 100.0f + t.entityelement[e].scalex;
				t.gridentityscaley_f = 100.0f + t.entityelement[e].scaley;
				t.gridentityscalez_f = 100.0f + t.entityelement[e].scalez;
			}
			else
			{
				t.gridentityscalex_f = ObjectScaleX(t.entityelement[e].obj);
				t.gridentityscaley_f = ObjectScaleY(t.entityelement[e].obj);
				t.gridentityscalez_f = ObjectScaleZ(t.entityelement[e].obj);
			}
			// this seems to wipe out what "entity_fillgrideleproffromprofile" did!
			t.grideleprof = t.entityelement[e].eleprof;
			entity_cleargrideleprofrelationshipdata();

			// add object to the level
			//PE: InstanceObject - Cursor,Object Tools - objects must always be real clones.
			extern bool bNextObjectMustBeClone;
			bNextObjectMustBeClone = true;

			gridedit_addentitytomap();

			bNextObjectMustBeClone = false;

			//PE: Always use the lowest Y object in list.
			if (iAnchorEntityIndex == -1 || bLowestFound ) iAnchorEntityIndex = t.e;
			t.entityelement[t.e].x = t.entityelement[e].x + fShiftOffsetForPasteX;
			t.entityelement[t.e].y = t.entityelement[e].y;
			t.entityelement[t.e].z = t.entityelement[e].z + fShiftOffsetForPasteZ;
			t.entityelement[t.e].rx = t.entityelement[e].rx;
			t.entityelement[t.e].ry = t.entityelement[e].ry;
			t.entityelement[t.e].rz = t.entityelement[e].rz;
			t.entityelement[t.e].quatmode = t.entityelement[e].quatmode;
			t.entityelement[t.e].quatx = t.entityelement[e].quatx;
			t.entityelement[t.e].quaty = t.entityelement[e].quaty;
			t.entityelement[t.e].quatz = t.entityelement[e].quatz;
			t.entityelement[t.e].quatw = t.entityelement[e].quatw;
			t.entityelement[t.e].editorfixed = t.entityelement[e].editorfixed;
			t.entityelement[t.e].staticflag = t.entityelement[e].staticflag;
			t.entityelement[t.e].scalex = t.entityelement[e].scalex;
			t.entityelement[t.e].scaley = t.entityelement[e].scaley;
			t.entityelement[t.e].scalez = t.entityelement[e].scalez;
			t.entityelement[t.e].soundset = t.entityelement[e].soundset;
			t.entityelement[t.e].soundset1 = t.entityelement[e].soundset1;
			t.entityelement[t.e].soundset2 = t.entityelement[e].soundset2;
			t.entityelement[t.e].soundset3 = t.entityelement[e].soundset3;
			t.entityelement[t.e].soundset4 = t.entityelement[e].soundset4;
			t.entityelement[t.e].soundset5 = t.entityelement[e].soundset5;
			t.entityelement[t.e].soundset6 = t.entityelement[e].soundset6;

			//LB: destructive for auto flatten IDs
			//t.entityelement[t.e].eleprof = t.entityelement[e].eleprof;
			int iStoreFlattenID = t.entityelement[t.e].eleprof.iFlattenID;
			t.entityelement[t.e].eleprof = t.entityelement[e].eleprof;
			t.entityelement[t.e].eleprof.iFlattenID = iStoreFlattenID;

			PositionObject(t.entityelement[t.e].obj, t.entityelement[t.e].x, t.entityelement[t.e].y, t.entityelement[t.e].z);
			RotateObject(t.entityelement[t.e].obj, t.entityelement[t.e].rx, t.entityelement[t.e].ry, t.entityelement[t.e].rz);

			// need to generate new particles
			if (t.entityprofile[t.gridentity].ismarker == 0)
			{
				entity_updateautoflatten(t.e);
			}
			if (t.entityprofile[t.gridentity].ismarker == 10)
			{
				t.entityelement[t.e].eleprof.newparticle.emitterid = -1;
				entity_updateparticleemitter(t.e);
			}

			//LB: in order to determine when last of smart objects deleted from level, so the smart object parent can be removed from entitybank, mark
			// the entity elements with the group ID as they are created here
			int iUniqueGroupID = vEntityDuplicateList[i].iGroupID;
			t.entityelement[t.e].creationOfGroupID = iUniqueGroupID;

			// and add to new rubber band group
			sRubberBandType rubberbandItem;
			rubberbandItem.e = t.e;
			rubberbandItem.x = t.entityelement[t.e].x;
			rubberbandItem.y = t.entityelement[t.e].y;
			rubberbandItem.z = t.entityelement[t.e].z;
			rubberbandItem.px = t.entityelement[t.e].x;
			rubberbandItem.py = t.entityelement[t.e].y;
			rubberbandItem.pz = t.entityelement[t.e].z;
			rubberbandItem.rx = t.entityelement[t.e].rx;
			rubberbandItem.ry = t.entityelement[t.e].ry;
			rubberbandItem.rz = t.entityelement[t.e].rz;
			rubberbandItem.quatmode = t.entityelement[t.e].quatmode;
			rubberbandItem.quatx = t.entityelement[t.e].quatx;
			rubberbandItem.quaty = t.entityelement[t.e].quaty;
			rubberbandItem.quatz = t.entityelement[t.e].quatz;
			rubberbandItem.quatw = t.entityelement[t.e].quatw;
			rubberbandItem.scalex = t.entityelement[t.e].scalex;
			rubberbandItem.scaley = t.entityelement[t.e].scaley;
			rubberbandItem.scalez = t.entityelement[t.e].scalez;
			g.entityrubberbandlist.push_back(rubberbandItem);
		}

		// clone all the logic connections
		DuplicateLogicConnections(vEntityDuplicateList, iOriginalGroupIndexForChild);

		// Select and add first entity to cursor, along with the rubberband.
		if (iAnchorEntityIndex != -1 && bAttachToCursor == true)
		{
			//PE: Need to set scale from group settings. t.entityelement[iAnchorEntityIndex].scalex
			AddEntityToCursor(iAnchorEntityIndex, false);

			//Change to just place under cursor.
			t.inputsys.dragoffsetx_f = 0;
			t.inputsys.dragoffsety_f = 0;
			fHitPointX = 0;
			fHitPointY = HITPOINTYSTARTPOS;
			fHitPointZ = 0;
			fHitOffsetX = 0;
			fHitOffsetY = 0;
			fHitOffsetZ = 0;

			g_bHoldGridEntityPosWhenManaged = true;
			g_fHoldGridEntityPosX = t.gridentityposx_f;
			g_fHoldGridEntityPosY = t.gridentityposy_f;
			g_fHoldGridEntityPosZ = t.gridentityposz_f;

			//LB: should not change modes without users permission
			//PE: Always start in horizontal mode.
			//LB: When dragging in groups, need to be in smart mode to handle object placement on terrain/surface
			iObjectMoveMode = 2; // not happy to change users preference, need better solution for release
		}

		//LB: when duplicate, we create a child group spawned from the parent, use unique "iParentGroupID" to keep track of parent and children
		CreateNewGroup(vEntityDuplicateList[0].iGroupID, false, "", false, iOriginalGroupIndexForChild);

		//LB: when duplicate, clear last selected so do not double highlight and mess up correct group selection list!
		iLastSelectedEntityGroup = -1;
		iLastSelectedEntity = -1;
	}
	if (bAttachToCursor == false)
	{
		// reset any attachments
		current_selected_group = -1;
		t.gridentity = 0;
	}
	return iAnchorEntityIndex;
}

void ListGroupContextMenu(bool bPickedOnly, int iEntityId)
{
	int iEntid = t.widget.pickedEntityIndex;
	if (iEntityId > 0)
		iEntid = iEntityId;
	for (int l = 0; l < MAXGROUPSLISTS; l++)
	{
		if (vEntityGroupList[l].size() > 0 )
		{
			cstr sContextMenuString = cstr("Add to Group") + cstr(l + 1);
			if (ImGui::MenuItem(sContextMenuString.Get()))
			{
				if ( (g.entityrubberbandlist.size() > 0 && !bPickedOnly) || iEntid > 0)
				{
					int iLoop = 1;
					if (g.entityrubberbandlist.size() > 0 && !bPickedOnly)
						iLoop = g.entityrubberbandlist.size();
					for (int i = 0; i < (int)iLoop; i++)
					{
						int e = iEntid;
						if (g.entityrubberbandlist.size() > 0 && !bPickedOnly)
							e = g.entityrubberbandlist[i].e;
						bool bAlreadyThere = false;
						for (int il = 0; il < (int)vEntityGroupList[l].size(); il++)
						{
							if (e == vEntityGroupList[l][il].e)
							{
								bAlreadyThere = true;
								break;
							}
						}
						if (!bAlreadyThere)
						{
							sRubberBandType newItem;
							newItem.e = e;
							newItem.x = t.entityelement[e].x;
							newItem.y = t.entityelement[e].y;
							newItem.z = t.entityelement[e].z;
							vEntityGroupList[l].push_back(newItem);
						}
					}
					if (!bPickedOnly)
					{
						g.entityrubberbandlist.clear();
						widget_switchoff();
					}
				}
			}
		}
	}
}

int GetGroupIndexFromName (cstr sLookFor)
{
	int iParentGroupID = -1;
	for (int l = 0; l < MAXGROUPSLISTS; l++)
	{
		if (sEntityGroupListName[l].Len() > 0)
		{
			if (stricmp (sEntityGroupListName[l].Get(), sLookFor.Get()) == NULL)
			{
				iParentGroupID = l;
				break;
			}
		}
	}
	return iParentGroupID;
}

void UnGroupSelected(bool bRetainRubberBandList)
{
	//Ungroup to cursor and delete current group.
	if (current_selected_group >= 0)
	{
		entity_createundoaction(eUndoSys_Object_UnGroup, current_selected_group);
		g.entityrubberbandlist = vEntityGroupList[current_selected_group];
		//Set widget on first item in list.
		int e = g.entityrubberbandlist[0].e;
		if (e > 0)
		{
			if (t.entityelement[e].editorlock == 0)
			{
				t.widget.pickedEntityIndex = e;
				t.widget.pickedObject = t.entityelement[e].obj;
			}
		}
		vEntityGroupList[current_selected_group].clear();
		if (ImageExist(iEntityGroupListImage[current_selected_group]))
		{
			//Image is already on screen. so dont delete before its reused.
			//It will get the same id next time so we can do this.
			iEntityGroupListImage[current_selected_group] = 0;
		}
		sEntityGroupListName[current_selected_group] = "";
		current_selected_group = -1;

		// Also ensure objects not still rubberband highlighted as the user may think they are still grouped when the 
		// primary object in the old group is dragged about
		if (bRetainRubberBandList == false )
			g.entityrubberbandlist.clear();
	}
}

void UnGroupUndoSys(int index)
{
	//Ungroup to cursor and delete current group.
	if (index >= 0)
	{
		g.entityrubberbandlist = vEntityGroupList[index];
		//Set widget on first item in list.
		int e = g.entityrubberbandlist[0].e;
		if (e > 0)
		{
			if (t.entityelement[e].editorlock == 0)
			{
				t.widget.pickedEntityIndex = e;
				t.widget.pickedObject = t.entityelement[e].obj;
			}
		}
		vEntityGroupList[index].clear();
		if (ImageExist(iEntityGroupListImage[index]))
		{
			//Image is already on screen. so dont delete before its reused.
			//It will get the same id next time so we can do this.
			iEntityGroupListImage[index] = 0;
		}
		sEntityGroupListName[index] = "";
		current_selected_group = -1;

		// Also ensure objects not still rubberband highlighted 
		g.entityrubberbandlist.clear();
	}
}
void GroupUndoSys(int index, std::vector<sRubberBandType> groupData)
{
	// Add the entities from the undo event to the rubberband list and then create a group from the rubberband
	g.entityrubberbandlist.clear();
	for (int i = 0; i < groupData.size(); i++)
	{
		g.entityrubberbandlist.push_back(groupData[i]);
	}

	CreateNewGroup(-1, true, "", false);
}

void CreateNewGroup(int iParentGroupID, bool bSnapshotGroupThumb, cstr GroupName_s, bool bGenerateUndo, int iForceIntoIndex)
{
	if (g.entityrubberbandlist.size() > 0)
	{
		GetRubberbandLowHighValues();
		if (iForceIntoIndex == -1)
		{
			//Check for duplicate.
			for (int i = 0; i < MAXGROUPSLISTS; i++)
			{
				if (vEntityGroupList[i].size() > 0)
				{
					if (vEntityGroupList[i].size() == g.entityrubberbandlist.size())
					{
						bool bAllFound = true;
						for (int ir = 0; ir < g.entityrubberbandlist.size(); ir++)
						{
							int e = g.entityrubberbandlist[ir].e;
							bool bfound = false;

							for (int il = 0; il < vEntityGroupList[i].size(); il++)
							{
								if (vEntityGroupList[i][il].e == e)
								{
									bfound = true;
									break;
								}
							}
							if (!bfound)
								bAllFound = false;
						}
						if (bAllFound) //All found in group.
							return;
					}

				}
			}

			//Remove empty.
			int dest = 0;
			for (int i = 0; i < MAXGROUPSLISTS; i++)
			{
				if (vEntityGroupList[i].size() > 0)
				{
					if (i != dest)
					{
						//move
						vEntityGroupList[dest] = vEntityGroupList[i];
						if (current_selected_group == dest)
							current_selected_group = dest;
						iEntityGroupListImage[dest] = iEntityGroupListImage[i];
						iEntityGroupListImage[i] = 0;
						sEntityGroupListName[dest] = sEntityGroupListName[i];
						sEntityGroupListName[i] = "";
						vEntityGroupList[i].clear();
					}
					dest++;
				}
			}

			//Move list down.
			for (int l = MAXGROUPSLISTS - 1; l > 0; l--)
			{
				if (vEntityGroupList[l - 1].size() > 0)
				{
					vEntityGroupList[l] = vEntityGroupList[l - 1];
					if (current_selected_group == l - 1)
						current_selected_group = l;
					iEntityGroupListImage[l] = iEntityGroupListImage[l - 1];
					sEntityGroupListName[l] = sEntityGroupListName[l - 1];
				}
			}
			vEntityGroupList[0].clear();
			sEntityGroupListName[0] = "";
		}

		// only create a thumbnail for parent groups, child groups are never shown
		int iImageID = 0;
		if (iParentGroupID == -1)
		{
			//Find free image id.
			for (int i = 0; i < MAXGROUPSLISTS; i++)
			{
				bool bAlreadyUsed = false;
				int iNewImageID = BACKBUFFERIMAGE + i;
				for (int l = MAXGROUPSLISTS; l > 0; l--)
				{
					if (iEntityGroupListImage[l] == iNewImageID)
					{
						bAlreadyUsed = true;
						break;
					}
				}
				if (!bAlreadyUsed) 
				{
					iImageID = iNewImageID;
					break;
				}
			}
			if (iImageID == 0) return; //No free imageID.
		}

		//Find free list or force a specific index
		int iChosenGroupIndex = iForceIntoIndex;
		if (iChosenGroupIndex == -1)
		{
			for (int l = 0; l < MAXGROUPSLISTS; l++)
			{
				if (vEntityGroupList[l].size() <= 0)
				{
					iChosenGroupIndex = l;
					break;
				}
			}
		}
		else
		{
			vEntityGroupList[iChosenGroupIndex].clear();
		}
		if ( iChosenGroupIndex != -1 )
		{
			int l = iChosenGroupIndex;
			if (vEntityGroupList[l].size() <= 0)
			{
				//Free, copy rubberband.
				vEntityGroupList[l] = g.entityrubberbandlist;
				widget_switchoff();

				//PE: Generate tumbnail of group.
				iEntityGroupListImage[l] = iImageID;
				if (bSnapshotGroupThumb == true)
				{
					// creating a group with NEW GROUP
					BackBufferIsGroup = false;
					BackBufferEntityID = 0;
					BackBufferObjectID = 0;
					BackBufferImageID = iEntityGroupListImage[l];
					BackBufferSizeX = 512 * 2.0f;
					BackBufferSizeY = 288 * 2.0f;
					BackBufferZoom = 1.0f;
					BackBufferCamLeft = 0.0f;
					BackBufferCamUp = 0.0f;
					bRotateBackBuffer = false;
					bBackBufferAnimated = false;
					bLoopBackBuffer = false;
					BackBufferSnapShotMode = true;
					if (BitmapExist(99))
					{
						DeleteBitmapEx(99);
					}
					bFullScreenBackbuffer = true;
					extern int bStopBackbufferGrab;
					bStopBackbufferGrab = 1;
				}
				else
				{
					// creating a group when we ADDED from Object Library
					int entid = 0;
					for (int i = 0; i < g.entityrubberbandlist.size(); i++)
					{
						// first update object from final entity element data
						int e = g.entityrubberbandlist[i].e;
						entid = t.entityelement[e].bankindex;
						if (t.entityprofile[entid].ismarker == 0)
						{
							// use this one!
							break;
						}
					}
					if (entid > 0)
					{
						BackBufferIsGroup = true;
						BackBufferEntityID = entid;
						BackBufferObjectID = g.entitybankoffset + entid;
						BackBufferImageID = iEntityGroupListImage[l];
						BackBufferSizeX = 512 * 2.0f;
						BackBufferSizeY = 288 * 2.0f;
						BackBufferZoom = 1.0f;
						BackBufferCamLeft = 0.0f;
						BackBufferCamUp = 0.0f;
						bRotateBackBuffer = false;
						bBackBufferAnimated = false;
						bLoopBackBuffer = false;
						if (BitmapExist(99))
						{
							DeleteBitmapEx(99);
						}
						bFullScreenBackbuffer = true;
						extern int bStopBackbufferGrab;
						bStopBackbufferGrab = 1;
					}
				}

				// assign name to group
				sEntityGroupListName[l] = GroupName_s;

				//Set selection on last created group.
				current_selected_group = l;
			}
		}

		//LB: if creating a child group, assign the parentID to it
		if (iParentGroupID == -1)
		{
			// creating a parent group
			if (current_selected_group >= 0)
			{
				g_iUniqueGroupID++;
				for (int n = 0; n < vEntityGroupList[current_selected_group].size(); n++)
				{
					vEntityGroupList[current_selected_group][n].iGroupID = g_iUniqueGroupID;
					vEntityGroupList[current_selected_group][n].iParentGroupID = -1;
				}

				// only "switch" to group tab if a parent group created, user will never see child groups being created here!
				if (sEntityGroupListName[current_selected_group].Len() == 0)
				{
					// Display group tab only for non smart objects
					i_switch_group_tab = 2;
				}
			}
		}
		else
		{
			// creating a child group of passed in iParentGroupID
			if (current_selected_group >= 0)
			{
				for (int n = 0; n < vEntityGroupList[current_selected_group].size(); n++)
				{
					vEntityGroupList[current_selected_group][n].iGroupID = -1;
					vEntityGroupList[current_selected_group][n].iParentGroupID = iParentGroupID;
				}
			}
		}

		if(bGenerateUndo)
			entity_createundoaction(eUndoSys_Object_Group, current_selected_group);
	}
}

void MakeFPELine (LPSTR pLine, LPSTR pFieldName, int iOptionalIndex, cstr str)
{
	// we want to have aligned spaced fields
	memset(pLine, 0, MAX_PATH);
	strcpy(pLine, "                 = ");

	// final field name
	char pFinalFieldName[MAX_PATH];
	strcpy (pFinalFieldName, pFieldName);
	if (iOptionalIndex > 0)
	{
		cstr pOptionalNum = cstr(iOptionalIndex);
		strcat(pFinalFieldName, pOptionalNum.Get());
	}

	// fill with field name
	if (strlen(pFinalFieldName) <= 17)//pFieldName) <= 17)
	{
		memcpy(pLine, pFinalFieldName, strlen(pFinalFieldName));
	}
	else
	{
		strcpy (pLine, pFinalFieldName);
		strcat (pLine, " = ");
	}

	// add value for field
	strcat (pLine, str.Get());
}

bool SaveGroup(int iGroupID, LPSTR pObjectSavedFilename)
{
	if (current_selected_group < 0) return false;
	// collect all objects in this group
	g.entityrubberbandlist = vEntityGroupList[current_selected_group];
	if (g.entityrubberbandlist.size() == 0)
	{
		strcpy(cTriggerMessage, "Group failed to save - no objects in group");
		bTriggerMessage = true;
		return false;
	}

	// save default location
	char pEntityBankFolder[MAX_PATH];
	strcpy (pEntityBankFolder, g.fpscrootdir_s.Get());
	strcat (pEntityBankFolder, "\\Files\\entitybank\\");
	GG_GetRealPath(pEntityBankFolder, true);
	char pSaveDefaultLocation[MAX_PATH];
	strcpy (pSaveDefaultLocation, pEntityBankFolder);
	strcat (pSaveDefaultLocation, "user\\");

	// select filename to save as or have one passed in
	char pFileSelectedEntered[MAX_PATH];
	char* cFileSelectedEntered = NULL;
	if (strlen(pObjectSavedFilename) > 0)
	{
		strcpy ( pFileSelectedEntered, pSaveDefaultLocation);
		strcat ( pFileSelectedEntered, pObjectSavedFilename);
		cFileSelectedEntered = pFileSelectedEntered;
	}
	else
	{
		cStr tOldDir = GetDir();
		cFileSelectedEntered = (char *)noc_file_dialog_open(NOC_FILE_DIALOG_SAVE, "fpe\0*.fpe\0", pSaveDefaultLocation, NULL);
		SetDir(tOldDir.Get());
	}

	if (cFileSelectedEntered == NULL)
	{
		strcpy(cTriggerMessage, "Group failed to save - you must specify a location inside entitybank");
		bTriggerMessage = true;
		return false;
	}

	// ensure entered filename has the .fpoe extension
	char cFileSelected[MAX_PATH];
	strcpy (cFileSelected, cFileSelectedEntered);
	if (strnicmp (cFileSelected + strlen(cFileSelected) - 4, ".fpe", 4) != NULL)
		strcat (cFileSelected, ".fpe");

	// can only save into entitybank
	if (strnicmp (cFileSelected, pEntityBankFolder, strlen(pEntityBankFolder)) != NULL)
	{
		strcpy(cTriggerMessage, "Group failed to save - you must specify a location inside entitybank");
		bTriggerMessage = true;
		return false;
	}

	// extract name from file chosen
	char pGroupObjectName[MAX_PATH];
	strcpy (pGroupObjectName, cFileSelected + strlen(pSaveDefaultLocation));
	pGroupObjectName[strlen(pGroupObjectName) - 4] = 0;

	// group object filename
	char pGroupObjectFilename[MAX_PATH];
	strcpy (pGroupObjectFilename, cFileSelected);
	GG_GetRealPath(pGroupObjectFilename, false);

	// gather grouip info
	int iGroupCount = g.entityrubberbandlist.size();

	// create FPE file (Group Object)
	char pLine[MAX_PATH];

	// check if one already exists of this name
	if (FileExist(pGroupObjectFilename) == 1)
	{
		strcpy(cTriggerMessage, "Group failed to save - an object of this name already exists!");
		bTriggerMessage = true;
		return false;
	}

	// write smart object
	if (FileOpen(1) == 1) CloseFile (1);
	OpenToWrite (1, pGroupObjectFilename);
	if (FileOpen(1) == 1)
	{
		WriteString (1, "; Group Object (Smart Object)");
		WriteString (1, "; header");
		MakeFPELine(pLine, "desc", 0, cstr(pGroupObjectName)); WriteString (1, pLine);
		WriteString (1, "");
		WriteString (1, "; group details");
		MakeFPELine(pLine, "isgroupobject", 0, cstr(1)); WriteString (1, pLine);
		MakeFPELine(pLine, "groupobjcount", 0, cstr(iGroupCount)); WriteString (1, pLine);
		WriteString (1, "");
		WriteString (1, "; group list");
		int e = g.entityrubberbandlist[0].e;
		float fBaseX = t.entityelement[e].x;
		float fBaseY = t.entityelement[e].y;
		float fBaseZ = t.entityelement[e].z;
		for (int i = 1; i <= iGroupCount; i++)
		{
			e = g.entityrubberbandlist[i - 1].e;
			int entid = t.entityelement[e].bankindex;
			LPSTR pEntityName = t.entitybank_s[entid].Get();

			MakeFPELine(pLine, "objname", i, cstr(pEntityName)); WriteString (1, pLine);
			MakeFPELine(pLine, "objoffx", i, cstr(t.entityelement[e].x - fBaseX)); WriteString (1, pLine);
			MakeFPELine(pLine, "objoffy", i, cstr(t.entityelement[e].y - fBaseY)); WriteString (1, pLine);
			MakeFPELine(pLine, "objoffz", i, cstr(t.entityelement[e].z - fBaseZ)); WriteString (1, pLine);
			MakeFPELine(pLine, "objrotx", i, cstr(t.entityelement[e].rx)); WriteString (1, pLine);
			MakeFPELine(pLine, "objroty", i, cstr(t.entityelement[e].ry)); WriteString (1, pLine);
			MakeFPELine(pLine, "objrotz", i, cstr(t.entityelement[e].rz)); WriteString (1, pLine);

			MakeFPELine(pLine, "objquatmode", i, cstr(t.entityelement[e].quatmode)); WriteString(1, pLine);
			MakeFPELine(pLine, "objquatx", i, cstr(t.entityelement[e].quatx)); WriteString(1, pLine);
			MakeFPELine(pLine, "objquaty", i, cstr(t.entityelement[e].quaty)); WriteString(1, pLine);
			MakeFPELine(pLine, "objquatz", i, cstr(t.entityelement[e].quatz)); WriteString(1, pLine);
			MakeFPELine(pLine, "objquatw", i, cstr(t.entityelement[e].quatw)); WriteString(1, pLine);

			MakeFPELine(pLine, "objscalex", i, cstr(t.entityelement[e].scalex)); WriteString (1, pLine);
			MakeFPELine(pLine, "objscaley", i, cstr(t.entityelement[e].scaley)); WriteString (1, pLine);
			MakeFPELine(pLine, "objscalez", i, cstr(t.entityelement[e].scalez)); WriteString (1, pLine);
			MakeFPELine(pLine, "objphysicsmode", i, cstr(t.entityelement[e].eleprof.physics)); WriteString (1, pLine);
			MakeFPELine(pLine, "objstaticmode", i, cstr(t.entityelement[e].staticflag)); WriteString (1, pLine);
			MakeFPELine(pLine, "objisimmobile", i, cstr(t.entityelement[e].eleprof.isimmobile)); WriteString (1, pLine);
			MakeFPELine(pLine, "objcollisionmode", i, cstr(t.entityelement[e].eleprof.iOverrideCollisionMode)); WriteString (1, pLine);

			if (t.entityprofile[entid].ismarker == 10)
			{
				MakeFPELine(pLine, "objshowstart", i, cstr(t.entityelement[e].eleprof.newparticle.bParticle_Show_At_Start)); WriteString (1, pLine);
			}
			else
			{
				MakeFPELine(pLine, "objshowstart", i, cstr(t.entityelement[e].eleprof.spawnatstart)); WriteString (1, pLine);
			}

			if (t.entityprofile[entid].ismarker == 10)
			{
				LPSTR pParticleName = t.entityelement[e].eleprof.newparticle.emittername.Get();
				MakeFPELine(pLine, "objpartname", i, cstr(pParticleName)); WriteString (1, pLine);
				MakeFPELine(pLine, "objpartloop", i, cstr(t.entityelement[e].eleprof.newparticle.bParticle_Looping_Animation)); WriteString (1, pLine);
				MakeFPELine(pLine, "objpartspeed", i, cstr(t.entityelement[e].eleprof.newparticle.fParticle_Speed)); WriteString (1, pLine);
				MakeFPELine(pLine, "objpartopacity", i, cstr(t.entityelement[e].eleprof.newparticle.fParticle_Opacity)); WriteString (1, pLine);
			}
			if (t.entityprofile[entid].ismarker == 2)
			{
				int lightindex = t.entityelement[e].eleprof.light.index;
				MakeFPELine(pLine, "objlighttype", i, cstr(t.entityelement[e].eleprof.light.index)); WriteString (1, pLine);
				MakeFPELine(pLine, "objlightcolor", i, cstr((int)t.entityelement[e].eleprof.light.color)); WriteString (1, pLine);
				MakeFPELine(pLine, "objlightdist", i, cstr(t.entityelement[e].eleprof.light.range)); WriteString (1, pLine);
				MakeFPELine(pLine, "objlightradius", i, cstr(t.entityelement[e].eleprof.light.offsetup)); WriteString (1, pLine);
				MakeFPELine(pLine, "objlightcast", i, cstr(t.entityelement[e].eleprof.castshadow)); WriteString (1, pLine);
				LPSTR pBehaviorName = t.entityelement[e].eleprof.aimain_s.Get();
				MakeFPELine(pLine, "objlightlogic", i, cstr(pBehaviorName)); WriteString (1, pLine);
				MakeFPELine(pLine, "objlightspot", i, cstr(t.entityelement[e].eleprof.usespotlighting)); WriteString(1, pLine);
			}
		}
		WriteString (1, "");
		WriteString (1, "; thumbnail");
		WriteString (1, "thumbnailbackdrop = Blue showroom.dds");
		CloseFile (1);

		//PE: If we got a generated thumb use this.
		if (ImageExist(iEntityGroupListImage[current_selected_group]))
		{
			char *find = (char *) pestrcasestr(cFileSelected, "entitybank\\");
			if (find)
			{
				cstr fname = (find + 11);
				CreateBackBufferCacheNameEx(fname.Get(), 512, 288, true);
				GG_SetWritablesToRoot(true);
				SaveImage(BackBufferCacheName.Get(), iEntityGroupListImage[current_selected_group]);
				GG_SetWritablesToRoot(false);
			}
		}
	}
	else
	{
		strcpy(cTriggerMessage, "Group failed to save - could not write the new object file");
		bTriggerMessage = true;
		return false;
	}

	// success
	strcpy (pObjectSavedFilename, pGroupObjectFilename + strlen(pEntityBankFolder));
	return true;
}

bool ReadFPELine (LPSTR pTryField, LPSTR pThisField, int* piOptionalIndex)
{
	if (strnicmp (pThisField, pTryField, strlen(pTryField)) == NULL)
	{
		char pNumberPart[32];
		strcpy (pNumberPart, pThisField + strlen(pTryField));
		if (pNumberPart[0] != ' ')
		{
			*piOptionalIndex = atoi(pNumberPart);
		}
		return true;
	}
	else
	{
		return false;
	}
}

struct sObjTable
{
	int entid;
	int e;
};

bool g_bCreatingHiddenGroupInstance = false;
std::vector<int> g_smartObjectDummyEntities;

bool LoadGroup(LPSTR pAbsFilename)
{
	// init vars and clear rubberband list
	cstr sGroupObjectName;
	int iGroupCount = 0;
	bool bGroupFileValid = false;
	g.entityrubberbandlist.clear();
	sObjTable* pObjTable = NULL;
	std::vector<int> entityIDsNewlyCreated;

	// parse group file to get all objects
	if (FileExist(pAbsFilename) == 1)
	{
		//LB: any instances created from group will be hidden and should not use autoflatten!
		g_bCreatingHiddenGroupInstance = true;

		// parse group file
		std::vector <cstr> groupdata_s;
		Dim (groupdata_s, 9999);
		LoadArray (pAbsFilename, groupdata_s);
		for (int groupline = 0; groupline < 9999; groupline++)
		{
			cstr line_s = groupdata_s[groupline];
			if (Len(line_s.Get()) > 0)
			{
				LPSTR pLine = line_s.Get();
				if (pLine[0] != ';')
				{
					// take fieldname and values
					for (t.c = 0; t.c < Len(pLine); t.c++)
					{
						if (pLine[t.c] == '=') { t.mid = t.c + 1; break; }
					}
					t.field_s = Lower(removeedgespaces(Left(pLine, t.mid - 1)));
					t.value_s = removeedgespaces(Right(pLine, Len(pLine) - t.mid));
					for (t.c = 0; t.c < Len(t.value_s.Get()); t.c++)
					{
						if (t.value_s.Get()[t.c] == ',') { t.mid = t.c + 1; break; }
					}
					t.value1 = ValF(removeedgespaces(Left(t.value_s.Get(), t.mid - 1)));
					t.value2_s = removeedgespaces(Right(t.value_s.Get(), Len(t.value_s.Get()) - t.mid));
					if (Len(t.value2_s.Get()) > 0)  t.value2 = ValF(t.value2_s.Get()); else t.value2 = -1;

					// populate with values found
					t.tryfield_s = "desc"; if (t.field_s == t.tryfield_s)  sGroupObjectName = t.value_s;
					t.tryfield_s = "isgroupobject"; if (t.field_s == t.tryfield_s)  bGroupFileValid = t.value1;
					t.tryfield_s = "groupobjcount";
					if (t.field_s == t.tryfield_s)
					{
						iGroupCount = t.value1;
						pObjTable = new sObjTable[iGroupCount + 1];
						memset(pObjTable, 0, sizeof(pObjTable));
					}
					if (iGroupCount > 0 && bGroupFileValid == true)
					{
						int iOptionalIndex = 0;
						if (ReadFPELine("objname", t.field_s.Get(), &iOptionalIndex))
						{
							// load this entity or find existing entid
							t.entid = 0;
							t.addentityfile_s = t.value_s.Get();
							if (t.addentityfile_s != "")
							{
								// allow filename only to be specified indicating file is local to main smart object FPE
								bool bNameOnlyNoPath = true;
								char pEntityFilePath[MAX_PATH];
								strcpy(pEntityFilePath, t.addentityfile_s.Get());
								for (int n = 0; n < strlen(pEntityFilePath); n++)
								{
									if (pEntityFilePath[n] == '\\' || pEntityFilePath[n] == '/')
									{
										bNameOnlyNoPath = false;
										break;
									}
								}
								if (bNameOnlyNoPath == true)
								{
									// get path of main smart object file
									char pMainSmartFilePath[MAX_PATH];
									strcpy(pMainSmartFilePath, pAbsFilename);
									for (int n = strlen(pMainSmartFilePath) - 1; n > 0; n--)
									{
										if (pMainSmartFilePath[n] == '\\' || pMainSmartFilePath[n] == '/')
										{
											pMainSmartFilePath[n] = 0;
											break;
										}
									}

									// add a path to this local smaert object to load child of smart object
									strcpy(pEntityFilePath, pMainSmartFilePath);
									strcat(pEntityFilePath, "\\");
									strcat(pEntityFilePath, t.addentityfile_s.Get());
									t.addentityfile_s = pEntityFilePath;
								}
								entity_adduniqueentity(false);

								// using this list to assign child states to them
								entityIDsNewlyCreated.push_back(t.entid);

								// we want to interrogate the group to get the entity profiles added,
								if (g_iAbortedAsEntityIsGroupFileModeStubOnly == 1)
								{
									// but not create entities at this stage
									continue;
								}
							}
							else
							{
								// could not load this object in the group!
								t.entid = 0;
							}
							pObjTable[iOptionalIndex].entid = t.entid;

							// entity details
							t.gridentity = t.entid;
							t.gridentityeditorfixed = 0;
							t.entitymaintype = 1;
							t.entitybankindex = t.entid;
							t.gridentitystaticmode = 0;
							t.gridentityhasparent = 0;
							t.gridentityposx_f = 0;
							t.gridentityposz_f = 0;
							t.gridentityposy_f = 0;
							t.gridentityrotatex_f = 0;
							t.gridentityrotatey_f = 0;
							t.gridentityrotatez_f = 0;
							t.gridentityrotatequatmode = 1;
							t.gridentityrotatequatx_f = 0;
							t.gridentityrotatequaty_f = 0;
							t.gridentityrotatequatz_f = 0;
							t.gridentityrotatequatw_f = 1;
							t.gridentityscalex_f = 100;
							t.gridentityscaley_f = 100;
							t.gridentityscalez_f = 100;
							entity_fillgrideleproffromprofile();

							// add new entity element (certain modes never reach here, see above code)
							t.e = 0; entity_addentitytomap();

							// add to rubberband list (e for now, rest populated later)
							int e = t.e;
							sRubberBandType rubberbandItem;
							rubberbandItem.e = e;
							g.entityrubberbandlist.push_back(rubberbandItem);

							// add reference to object table (for rest of objxxx parsing)
							pObjTable[iOptionalIndex].e = e;
							pObjTable[iOptionalIndex].entid = t.entid;

							// no leftover fields from entity loaddata (called above)
							t.field_s = "";
						}
						if (g_iAbortedAsEntityIsGroupFileModeStubOnly != 1)
						{
							if (ReadFPELine("objoffx", t.field_s.Get(), &iOptionalIndex)) t.entityelement[pObjTable[iOptionalIndex].e].x = t.value1;
							if (ReadFPELine("objoffy", t.field_s.Get(), &iOptionalIndex)) t.entityelement[pObjTable[iOptionalIndex].e].y = t.value1;
							if (ReadFPELine("objoffz", t.field_s.Get(), &iOptionalIndex)) t.entityelement[pObjTable[iOptionalIndex].e].z = t.value1;
							if (ReadFPELine("objrotx", t.field_s.Get(), &iOptionalIndex)) t.entityelement[pObjTable[iOptionalIndex].e].rx = t.value1;
							if (ReadFPELine("objroty", t.field_s.Get(), &iOptionalIndex)) t.entityelement[pObjTable[iOptionalIndex].e].ry = t.value1;
							if (ReadFPELine("objrotz", t.field_s.Get(), &iOptionalIndex)) t.entityelement[pObjTable[iOptionalIndex].e].rz = t.value1;
							if (ReadFPELine("objscalex", t.field_s.Get(), &iOptionalIndex)) t.entityelement[pObjTable[iOptionalIndex].e].scalex = t.value1;
							if (ReadFPELine("objscaley", t.field_s.Get(), &iOptionalIndex)) t.entityelement[pObjTable[iOptionalIndex].e].scaley = t.value1;
							if (ReadFPELine("objscalez", t.field_s.Get(), &iOptionalIndex)) t.entityelement[pObjTable[iOptionalIndex].e].scalez = t.value1;
							if (ReadFPELine("objphysicsmode", t.field_s.Get(), &iOptionalIndex)) t.entityelement[pObjTable[iOptionalIndex].e].eleprof.physics = t.value1;
							if (ReadFPELine("objstaticmode", t.field_s.Get(), &iOptionalIndex)) t.entityelement[pObjTable[iOptionalIndex].e].staticflag = t.value1;
							if (ReadFPELine("objisimmobile", t.field_s.Get(), &iOptionalIndex)) t.entityelement[pObjTable[iOptionalIndex].e].eleprof.isimmobile = t.value1;
							if (ReadFPELine("objshowstart", t.field_s.Get(), &iOptionalIndex)) t.entityelement[pObjTable[iOptionalIndex].e].eleprof.newparticle.bParticle_Show_At_Start = t.value1;
							if (ReadFPELine("objshowstart", t.field_s.Get(), &iOptionalIndex)) t.entityelement[pObjTable[iOptionalIndex].e].eleprof.spawnatstart = t.value1;
							if (ReadFPELine("objpartname", t.field_s.Get(), &iOptionalIndex)) t.entityelement[pObjTable[iOptionalIndex].e].eleprof.newparticle.emittername = t.value_s;
							if (ReadFPELine("objpartloop", t.field_s.Get(), &iOptionalIndex)) t.entityelement[pObjTable[iOptionalIndex].e].eleprof.newparticle.bParticle_Looping_Animation = t.value1;
							if (ReadFPELine("objpartspeed", t.field_s.Get(), &iOptionalIndex)) t.entityelement[pObjTable[iOptionalIndex].e].eleprof.newparticle.fParticle_Speed = t.value1;
							if (ReadFPELine("objpartopacity", t.field_s.Get(), &iOptionalIndex)) t.entityelement[pObjTable[iOptionalIndex].e].eleprof.newparticle.fParticle_Opacity = t.value1;
							if (ReadFPELine("objlighttype", t.field_s.Get(), &iOptionalIndex)) t.entityelement[pObjTable[iOptionalIndex].e].eleprof.light.index = t.value1;
							if (ReadFPELine("objlightcolor", t.field_s.Get(), &iOptionalIndex)) t.entityelement[pObjTable[iOptionalIndex].e].eleprof.light.color = t.value1;
							if (ReadFPELine("objlightdist", t.field_s.Get(), &iOptionalIndex)) t.entityelement[pObjTable[iOptionalIndex].e].eleprof.light.range = t.value1;
							if (ReadFPELine("objlightradius", t.field_s.Get(), &iOptionalIndex)) t.entityelement[pObjTable[iOptionalIndex].e].eleprof.light.offsetup = t.value1;
							if (ReadFPELine("objlightcast", t.field_s.Get(), &iOptionalIndex)) t.entityelement[pObjTable[iOptionalIndex].e].eleprof.castshadow = t.value1;
							if (ReadFPELine("objlightlogic", t.field_s.Get(), &iOptionalIndex)) t.entityelement[pObjTable[iOptionalIndex].e].eleprof.aimain_s = t.value_s;
							if (ReadFPELine("objlightspot", t.field_s.Get(), &iOptionalIndex)) t.entityelement[pObjTable[iOptionalIndex].e].eleprof.usespotlighting = t.value1;
							if (ReadFPELine("objquatmode", t.field_s.Get(), &iOptionalIndex)) t.entityelement[pObjTable[iOptionalIndex].e].quatmode = t.value1;
							if (ReadFPELine("objquatx", t.field_s.Get(), &iOptionalIndex)) t.entityelement[pObjTable[iOptionalIndex].e].quatx = t.value1;
							if (ReadFPELine("objquaty", t.field_s.Get(), &iOptionalIndex)) t.entityelement[pObjTable[iOptionalIndex].e].quaty = t.value1;
							if (ReadFPELine("objquatz", t.field_s.Get(), &iOptionalIndex)) t.entityelement[pObjTable[iOptionalIndex].e].quatz = t.value1;
							if (ReadFPELine("objquatw", t.field_s.Get(), &iOptionalIndex)) t.entityelement[pObjTable[iOptionalIndex].e].quatw = t.value1;
							if (ReadFPELine("objcollisionmode", t.field_s.Get(), &iOptionalIndex)) t.entityelement[pObjTable[iOptionalIndex].e].eleprof.iOverrideCollisionMode = t.value1;
						}
					}
				}
			}
		}
		UnDim(groupdata_s);

		// in this mode we just want the entity profiles from any groups, no need to instantiate entity elements yet
		if (g_iAbortedAsEntityIsGroupFileModeStubOnly == 1)
			return true;

		// location to place the entityelements for this group load
		float fBaseX = 0;
		float fBaseY = -500000; //PE: Hide master group.
		float fBaseZ = 0;

		// raise baseY so lowest object in the group is one floor
		float fLowest = 0.0f;
		for (int i = 0; i < g.entityrubberbandlist.size(); i++)
		{
			int e = g.entityrubberbandlist[i].e;
			if (t.entityelement[e].y < fLowest)
				fLowest = t.entityelement[e].y;
		}
		if (fLowest < 0.0f) fBaseY -= fLowest;

		// populate rubberband list items
		for (int i = 0; i < g.entityrubberbandlist.size(); i++)
		{
			// first update object from final entity element data
			int e = g.entityrubberbandlist[i].e;

			// place entity so can be seen
			t.entityelement[e].x += fBaseX;
			t.entityelement[e].y += fBaseY;
			t.entityelement[e].z += fBaseZ;

			// update entity object from its new settings
			t.tupdatee = e; entity_updateentityobj();

			t.entityelement[e].iIsSmarkobjectDummyObj = 1; //PE: We need a way so we dont display these, in the detailed object list.

			// update lights and particles too
			int entid = t.entityelement[e].bankindex;
			if (t.entityprofile[entid].ismarker == 0)
			{
				entity_updateautoflatten(t.tupdatee);
			}
			if (t.entityprofile[entid].ismarker == 2 || t.entityprofile[entid].ismarker == 5 || t.entityelement[t.e].eleprof.usespotlighting)
			{
				t.entityelement[e].eleprof.light.index = 0;
				lighting_refresh();
				entity_updatelightobj(e, t.entityelement[e].obj);
			}
			if (t.entityprofile[entid].ismarker == 10)
			{
				t.entityelement[e].eleprof.newparticle.emitterid = -1;
				entity_updateparticleemitter(t.tupdatee);
			}

			// and then add correct values to rubberband list
			g.entityrubberbandlist[i].x = t.entityelement[e].x;
			g.entityrubberbandlist[i].y = t.entityelement[e].y;
			g.entityrubberbandlist[i].z = t.entityelement[e].z;
			g.entityrubberbandlist[i].px = t.entityelement[e].x;
			g.entityrubberbandlist[i].py = t.entityelement[e].y;
			g.entityrubberbandlist[i].pz = t.entityelement[e].z;
			g.entityrubberbandlist[i].rx = t.entityelement[e].rx;
			g.entityrubberbandlist[i].ry = t.entityelement[e].ry;
			g.entityrubberbandlist[i].rz = t.entityelement[e].rz;

			// calculate quat from ROTXYZ in smart object child
			//if(t.entityelement[e].quatmode == 0) // seems quatmode set to 1 but has no quat in there for BE objects
			entity_updatequatfromeuler(e);

			g.entityrubberbandlist[i].quatmode = t.entityelement[e].quatmode;
			g.entityrubberbandlist[i].quatx = t.entityelement[e].quatx;
			g.entityrubberbandlist[i].quaty = t.entityelement[e].quaty;
			g.entityrubberbandlist[i].quatz = t.entityelement[e].quatz;
			g.entityrubberbandlist[i].quatw = t.entityelement[e].quatw;
			g.entityrubberbandlist[i].scalex = t.entityelement[e].scalex;
			g.entityrubberbandlist[i].scaley = t.entityelement[e].scaley;
			g.entityrubberbandlist[i].scalez = t.entityelement[e].scalez;

			g.entityrubberbandlist[i].iGroupID = -1;
			g.entityrubberbandlist[i].iParentGroupID = -1;
		}

		if (g_iAbortedAsEntityIsGroupFileModeStubOnly > 0)
		{
			// group data and entities already loaded, we can skip a new group creation here
			if (g_iAbortedAsEntityIsGroupFileModeStubOnly > 1)
			{
				int iParentGroupIndex = g_iAbortedAsEntityIsGroupFileModeStubOnly - 2;
				if (iParentGroupIndex != -1)
				{
					vEntityGroupList[iParentGroupIndex] = g.entityrubberbandlist;
				}
			}
		}
		else
		{
			// create group from list
			CreateNewGroup(-1, false, cstr(pAbsFilename));

			// populate newly created hidden elements of group with associated groupID
			int iUniqueGroupID = -1;
			int iGroupIndex = GetGroupIndexFromName(pAbsFilename);
			if (vEntityGroupList[iGroupIndex].size() > 0)
			{
				iUniqueGroupID = vEntityGroupList[iGroupIndex][0].iGroupID;
			}
			for (int i = 0; i < g.entityrubberbandlist.size(); i++)
			{
				// first update object from final entity element data
				int ee = g.entityrubberbandlist[i].e;
				t.entityelement[ee].creationOfGroupID = iUniqueGroupID;
			}
		}

		//LB: back to regular mode, instances can be autoflatten again!
		g_bCreatingHiddenGroupInstance = false;

		// any entities created here should be assigned current_selected_group
		for (auto& entityID : entityIDsNewlyCreated)
		{
			t.entityprofile[entityID].ischildofgroup = 1;
		}
		entityIDsNewlyCreated.clear();
	}
	else
	{
		strcpy(cTriggerMessage, "Group failed to load - no file found");
		return false;
	}

	// success
	return true;
}

void AddEntityToCursor(int e, bool bDuplicate)
{
	if (e <= 0) return;

	if (t.gridentityinzoomview > 0) return; //Return if we are in "properties".

	if (!bDuplicate && t.entityelement[e].editorlock) return; //Dont allow selection of locked entity.


	//PE: we loose status somewhere, so force it off after adding a entity to map.
	extern bool bCubesVisible;
	if (bCubesVisible == false) bCubesVisible = true; //Force.

	if (!bDuplicate)
		iLastEntityOnCursor = e;
	else
		iLastEntityOnCursor = 0;

	//PE: Check if we can do a valid offset.
	float clickx = 0.0f, clickz = 0.0f, fPickedYAxis = 0.0f;
	// PE: Mixed system

	bDetectTerrainOnly = false;

	if (!bDuplicate)
	{
		int iObj = t.entityelement[e].obj;
		bool bVisible = false;
		if (iObj > 0)
		{
			if (ObjectExist(iObj) == 1)
			{
				sObject* pObject = g_ObjectList[iObj];
				bVisible = pObject->bVisible;
				HideObject(iObj);
			}
		}
		//Sent ray with the object visible.
		WickedCall_GetPick(&clickx, &fPickedYAxis, &clickz, NULL, NULL, NULL, NULL, GGRENDERLAYERS_NORMAL);

		if (bVisible)
		{
			ShowObject(iObj);
		}
	}

	if (!bDuplicate)
	{
		if (g.entityrubberbandlist.size() > 0)
		{
			bool bEntityOk = false;
			//Only allow selection/move if within rubberband.
			for (int i = 0; i < (int)g.entityrubberbandlist.size(); i++)
			{
				int ent = g.entityrubberbandlist[i].e;
				if (ent == e)
				{
					bEntityOk = true;
					break;
				}
			}
			if (!bEntityOk) return;
		}
	}
	if (bWaypointDrawmode || bWaypoint_Window) { bWaypointDrawmode = false; bWaypoint_Window = false; }
	if (g_bCharacterCreatorPlusActivated) g_bCharacterCreatorPlusActivated = false;
	if (bImporter_Window) { importer_quit(); bImporter_Window = false; }

	DeleteWaypointsAddedToCurrentCursor();
	CloseDownEditorProperties();

	//LB: Only when not in shooter mode
	//LB: shooter now a filter mode if (!Shooter_Tools_Window)
	{
		//Make sure we are in entity mode.
		bForceKey = true;
		csForceKey = "o";
	}

	t.tentitytoselect = e;
	t.widget.duplicatebuttonselected = 0;
	t.gridentityautofind = 7;

	t.onetimeentitypickup = 1;

	//  extract entity from the map
	if (t.tentitytoselect > 0)
	{
		if (t.entityelement[t.tentitytoselect].editorfixed == 0)
		{
			fExtractYValue = t.entityelement[t.tentitytoselect].y;

			t.gridentityeditorfixed = t.entityelement[t.tentitytoselect].editorfixed;
			t.gridentity = t.entityelement[t.tentitytoselect].bankindex;
			t.ttrygridentitystaticmode = t.entityelement[t.tentitytoselect].staticflag;
			t.ttrygridentity = t.gridentity; editor_validatestaticmode();
			t.gridedit.autoflatten = t.entityprofile[t.gridentity].autoflatten;
			//t.gridedit.entityspraymode = 0; //PE: spray checkbox.
			if (t.gridentityautofind == 7)
			{
				//  widget extracts without forcing entity to Floor
				t.gridentityautofind = 0;
				t.gridentityposoffground = 1;
				t.gridentityusingsoftauto = 0;
			}
			else
			{
				t.gridentityposoffground = 0;
				t.gridentityusingsoftauto = 1;
				// MAX handles its own positioning system
				{
					t.gridentityautofind = 0;
				}
			}
			t.gridentitysurfacesnap = 0; // surfacesnap off as messes up extract offset for entity
			t.gridentityextractedindex = t.tentitytoselect;
			t.gridentityhasparent = 0;//t.entityelement[t.tentitytoselect].iHasParentIndex; 210317 - break association when extract so can place free of parent
			t.gridentityposx_f = t.entityelement[t.tentitytoselect].x;
			t.gridentityposy_f = t.entityelement[t.tentitytoselect].y;
			t.gridentityposz_f = t.entityelement[t.tentitytoselect].z;
			t.gridentityrotatex_f = t.entityelement[t.tentitytoselect].rx;
			t.gridentityrotatey_f = t.entityelement[t.tentitytoselect].ry;
			t.gridentityrotatez_f = t.entityelement[t.tentitytoselect].rz;
			t.gridentityrotatequatmode = t.entityelement[t.tentitytoselect].quatmode;
			t.gridentityrotatequatx_f = t.entityelement[t.tentitytoselect].quatx;
			t.gridentityrotatequaty_f = t.entityelement[t.tentitytoselect].quaty;
			t.gridentityrotatequatz_f = t.entityelement[t.tentitytoselect].quatz;
			t.gridentityrotatequatw_f = t.entityelement[t.tentitytoselect].quatw;
			if (t.entityprofile[t.gridentity].ismarker == 10)
			{
				t.gridentityscalex_f = 100.0f + t.entityelement[t.tentitytoselect].scalex;
				t.gridentityscaley_f = 100.0f + t.entityelement[t.tentitytoselect].scaley;
				t.gridentityscalez_f = 100.0f + t.entityelement[t.tentitytoselect].scalez;
			}
			else
			{
				t.gridentityscalex_f = ObjectScaleX(t.entityelement[t.tentitytoselect].obj);
				t.gridentityscaley_f = ObjectScaleY(t.entityelement[t.tentitytoselect].obj);
				t.gridentityscalez_f = ObjectScaleZ(t.entityelement[t.tentitytoselect].obj);
			}
			t.grideleprof = t.entityelement[t.tentitytoselect].eleprof;
			t.grideleproflastname_s = t.grideleprof.name_s;

			//  Transfer any waypoint association
			t.waypointindex = t.entityelement[t.tentitytoselect].eleprof.trigger.waypointzoneindex;
			t.grideleprof.trigger.waypointzoneindex = t.waypointindex;
			t.waypoint[t.waypointindex].linkedtoentityindex = 0;

			//Just place under cursor.
			t.inputsys.dragoffsetx_f = 0;
			t.inputsys.dragoffsety_f = 0;

			float lowy = 0.0f;

			if (bDuplicate)
			{
				//PE: New object need a unique particle id.
				t.grideleprof.newparticle.emitterid = -1;
			}

			// get object size before we delete it
			float fObjectRealSize = 0.0f;
			if (t.tentitytoselect > 0) 
				fObjectRealSize = ObjectSize(t.entityelement[t.tentitytoselect].obj, 1);

			if (!bDuplicate) 
			{
				lowy = GetLowestY(t.entityelement[t.tentitytoselect].obj);
				g_UndoSysObjectIsBeingMoved = true; // this is not a real delete, just a step in highlighting the entity
				t.gridentitypreferelementindex = t.tentitytoselect; // set a preference where we want object put back into element index
				gridedit_deleteentityfrommap();
				g_UndoSysObjectIsBeingMoved = false;
				t.onetimeentitypickup = 0;
				//PE: If this is a light ,
				if (t.entityprofile[t.gridentity].ismarker == 2 || t.entityprofile[t.gridentity].ismarker == 5)
				{
					//Add the light.
					if (t.gridentitywickedlightindex == 0)
					{
						int iLightType = 1;
						if (t.grideleprof.usespotlighting) iLightType = 2;
						t.gridentitywickedlightindex = WickedCall_AddLight(iLightType);
					}
					if (t.gridentitywickedlightindex > 0)
					{
						float lightx = t.gridentityposx_f;
						float lighty = t.gridentityposy_f;
						float lightz = t.gridentityposz_f;
						float lightax = t.gridentityrotatex_f;
						float lightay = t.gridentityrotatey_f;
						float lightaz = t.gridentityrotatez_f;
						float lightrange = t.grideleprof.light.range;
						float lightspotradius = t.grideleprof.light.offsetup;
						int colr = ((t.grideleprof.light.color & 0x00ff0000) >> 16);
						int colg = ((t.grideleprof.light.color & 0x0000ff00) >> 8);
						int colb = (t.grideleprof.light.color & 0x000000ff);
						bool bCastShadow = true;
						if (t.grideleprof.castshadow == 1) bCastShadow = false;
						WickedCall_UpdateLight(t.gridentitywickedlightindex, lightx, lighty, lightz, lightax, lightay, lightaz, lightrange, lightspotradius, colr, colg, colb, bCastShadow);
					}
				}
			}

			fHitPointX = 0;
			fHitPointY = HITPOINTYSTARTPOS;
			fHitPointZ = 0;
			fHitOffsetX = 0;
			fHitOffsetY = 0;
			fHitOffsetZ = 0;
			iStartMouseX = (int)ImGui::GetMousePos().x;
			iStartMouseY = (int)ImGui::GetMousePos().y;
			iLastHitObjectID = 0;

			g_bHoldGridEntityPosWhenManaged = true;
			g_fHoldGridEntityPosX = t.gridentityposx_f;
			g_fHoldGridEntityPosY = t.gridentityposy_f;
			g_fHoldGridEntityPosZ = t.gridentityposz_f;

			if (!bDuplicate)
			{
				// LB: get the first hit XYZ when click an object to move about
				fHitPointX = t.inputsys.localx_f;
				fHitPointY = t.inputsys.localcurrentterrainheight_f;
				fHitPointZ = t.inputsys.localy_f;
				fHitOffsetX = fHitPointX - t.gridentityposx_f;
				fHitOffsetY = fHitPointY - t.gridentityposy_f;
				fHitOffsetZ = fHitPointZ - t.gridentityposz_f;

				//PE: Need to start rubberbandmove with same values.
				t.fOldGridEntityX = t.gridentityposx_f;
				t.fOldGridEntityY = t.gridentityposy_f;
				t.fOldGridEntityZ = t.gridentityposz_f;
				t.fOldGridEntityRX = t.gridentityrotatex_f;
				t.fOldGridEntityRY = t.gridentityrotatey_f;
				t.fOldGridEntityRZ = t.gridentityrotatez_f;
				t.fOldGridEntityQuatMode = t.gridentityrotatequatmode;
				t.fOldGridEntityQuatX = t.gridentityrotatequatx_f;
				t.fOldGridEntityQuatY = t.gridentityrotatequaty_f;
				t.fOldGridEntityQuatZ = t.gridentityrotatequatz_f;
				t.fOldGridEntityQuatW = t.gridentityrotatequatw_f;
			}
			else
			{
				g.entityrubberbandlist.clear();
			}

			// get size of object selected, to determine if to use drop system (only used for larger objects)
			iObjectMoveModeDropSystemUsing = 0;
			int iForwardFacing = t.entityprofile[t.gridentity].forwardfacing;
			float fUpDownAngle = WrapValue(CameraAngleX(0));
			if (fUpDownAngle > 10.0f && fUpDownAngle < 91.0f)
			{
				// down is fine for ghost drop
				if (t.tentitytoselect > 0 && fObjectRealSize > 20.0f && iForwardFacing == 0 && g_iStackToSurfaceMode == 1)
				{
					bool bJustForInitialDragIn = false;
					if (bDraggingActive == false && fHitOffsetX == 0 && fHitOffsetY == 0 && fHitOffsetZ == 0) bJustForInitialDragIn = true;
					if (bDraggingActive == true && t.gridentityposx_f == 0 && t.gridentityposz_f == 0) bJustForInitialDragIn = true;
					if (bDraggingActiveInitial == true)	bJustForInitialDragIn = true;
					if (bJustForInitialDragIn == true || g.entityrubberbandlist.size() <= 1)
					{
						iObjectMoveModeDropSystemUsing = 1;
					}
				}
			}
			else
			{
				// looking up and to side, drop system not a good idea
			}

			if (!bDuplicate) 
			{
				// in smart positning mode, always find surface when drop into level
				if (iObjectMoveMode == 2 && iObjectMoveModeDropSystem == 0)
				{
					if (iObjectMoveModeDropSystemUsing == 1 && g_bHoldGridEntityPosWhenManaged == false)
					{
						iObjectMoveModeDropSystem = 1;
						t.inputsys.dragoffsetx_f = 0.0f;
						t.inputsys.dragoffsety_f = 0.0f;
					}
				}
				else
				{
					// positioning mode horiz and vert do not seek surface adjustment, so keep as is
					t.inputsys.dragoffsetx_f = t.entityelement[t.tentitytoselect].x - clickx;
					t.inputsys.dragoffsety_f = t.entityelement[t.tentitytoselect].z - clickz;
				}
			}
			else
			{
				// in smart positning mode, always find surface when drop into level
				if (iObjectMoveMode == 2 && iObjectMoveModeDropSystem == 0)
				{
					if (iObjectMoveModeDropSystemUsing == 1 && g_bHoldGridEntityPosWhenManaged == false)
					{
						iObjectMoveModeDropSystem = 1;
						t.inputsys.dragoffsetx_f = 0.0f;
						t.inputsys.dragoffsety_f = 0.0f;
					}
				}
			}

			t.widget.pickedObject = 0;
			widget_updatewidgetobject();
			t.refreshgrideditcursor = 1;
		}
	}
}


