#ifdef STORYBOARD // continued from part3

int InitScreen(lua_State* L)
{
	// write to active LUA, i.e. g_UserGlobal[yourscript.user_variable_name]
	char pScreenName[512];
	strcpy(pScreenName, lua_tostring(L, 1));
	//int nodeid = FindLuaScreenNode(pScreenName);
	for (int nodeid = 0; nodeid < STORYBOARD_MAXNODES; nodeid++)
	{
		if (strlen(Storyboard.Nodes[nodeid].lua_name) > 0 && strnicmp(Storyboard.Nodes[nodeid].lua_name, "hud", 3) == NULL)
		{
			for (int i = 0; i < STORYBOARD_MAXWIDGETS; i++)
			{
				if (Storyboard.Nodes[nodeid].widget_type[i] == STORYBOARD_WIDGET_TEXT)
				{
					std::string readout = Storyboard.widget_readout[nodeid][i];
					if (stricmp(readout.c_str(), "User Defined Global") == NULL)
					{
						char pUserDefinedGlobal[256];
						sprintf(pUserDefinedGlobal, "g_UserGlobal['%s']", Storyboard.Nodes[nodeid].widget_label[i]);
						LuaSetInt(pUserDefinedGlobal, Storyboard.Nodes[nodeid].widget_initial_value[i]);
					}
					if (stricmp(readout.c_str(), "User Defined Global Text") == NULL)
					{
						char pUserDefinedGlobal[256];
						sprintf(pUserDefinedGlobal, "g_UserGlobal['%s']", Storyboard.Nodes[nodeid].widget_label[i]);
						//PE: We now reuse widget_click_sound for initial_value.
						LuaSetString(pUserDefinedGlobal, Storyboard.Nodes[nodeid].widget_click_sound[i]); // can we get from initial_value in screen editor?
					}
				}
			}
		}
	}
	return 0;
}
int DisplayScreen(lua_State* L)
{
	char pScreenName[512];
	strcpy(pScreenName, lua_tostring(L, 1));
	int screen_editor(int nodeid, bool standalone = false, char* screen = NULL);
	screen_editor(-1, true, pScreenName);
	// GGMAX 2.70: integer subtype — savegame.lua concats this into gameslot<N>.dat
	lua_pushinteger(L, iSpecialLuaReturn);
	return 1;
}
int DisplayCurrentScreen(lua_State* L)
{
	int screen_editor(int nodeid, bool standalone = false, char* screen = NULL);
	if (t.game.activeStoryboardScreen >= 0)
	{
		screen_editor(t.game.activeStoryboardScreen, true);
	}
	lua_pushinteger(L, iSpecialLuaReturn);
	return 1;
}
bool g_bEnableGunFireInHUD = false;
int DisableGunFireInHUD(lua_State* L)
{
	g_bEnableGunFireInHUD = false;
	return 0;
}
int EnableGunFireInHUD(lua_State* L)
{
	g_bEnableGunFireInHUD = true;
	return 0;
}
bool bDisableKeyToggles = false;
int DisableBoundHudKeys(lua_State* L)
{
	bDisableKeyToggles = true;
	return 0;
}
int EnableBoundHudKeys(lua_State* L)
{
	bDisableKeyToggles = false;
	return 0;
}
int CheckScreenToggles(lua_State* L)
{
	if (bDisableKeyToggles) return 0;
	extern void TriggerScreenFromKeyPress();
	TriggerScreenFromKeyPress();
	return 1;
}
int ScreenToggle(lua_State* L)
{
	lua2 = L;
	int n = lua_gettop(L); if (n < 1) return 0;
	char pScreenTitle[512];
	strcpy(pScreenTitle, lua_tostring(L, 1));
	t.game.activeStoryboardScreen = -1;
	int nodeid = FindLuaScreenTitleNode(pScreenTitle);
	if (nodeid >= 0)
	{
		t.game.activeStoryboardScreen = nodeid;
	}
	return 0;
}
int ScreenToggleByKey(lua_State* L)
{
	lua2 = L;
	int n = lua_gettop(L); if (n < 1) return 0;
	char pKeyToSearchFor[512];
	strcpy(pKeyToSearchFor, lua_tostring(L, 1));
	t.game.activeStoryboardScreen = -1;
	int nodeid = FindLuaScreenTitleNodeByKey(pKeyToSearchFor);
	if (nodeid >= 0)
	{
		t.game.activeStoryboardScreen = nodeid;
	}
	return 0;
}

int GetIfUsingTABScreen(lua_State* L)
{
	int iReturnValue = 0;
	if (g.tabmode > 0) iReturnValue = 1;
	lua_pushnumber(L, iReturnValue);
	return 1;
}

int GetCurrentScreen(lua_State* L)
{
	lua_pushnumber(L, t.game.activeStoryboardScreen);
	return 1;
}
int GetCurrentScreenName(lua_State* L)
{
	std::string check_lua_title = "";
	if (t.game.activeStoryboardScreen >= 0)
	{
		check_lua_title = Storyboard.Nodes[t.game.activeStoryboardScreen].title;
	}
	lua_pushstring(L, check_lua_title.c_str());
	return 1;
}

int GetScreenWidgetValue(lua_State *L)
{
	float fRet = -99999.0;
	char pScreenName[512];
	strcpy(pScreenName, lua_tostring(L, 1));
	int iButton = lua_tonumber(L, 2);
	if (iButton >= 0 && iButton < STORYBOARD_MAXOUTPUTS)
	{
		int nodeid = FindLuaScreenNode(pScreenName);
		if (nodeid >= 0 && nodeid < STORYBOARD_MAXNODES)
		{
			fRet = Storyboard.NodeSliderValues[nodeid][iButton];
		}
	}
	lua_pushnumber(L, fRet);
	return 1;
	//return 0;
}
int SetScreenWidgetValue(lua_State *L)
{
	float fRet = -99999.0;
	char pScreenName[512];
	strcpy(pScreenName, lua_tostring(L, 1));
	int iButton = lua_tonumber(L, 2);
	float fNewValue = lua_tonumber(L, 3);
	if (iButton >= 0 && iButton < STORYBOARD_MAXOUTPUTS)
	{
		int nodeid = FindLuaScreenNode(pScreenName);
		if (nodeid >= 0 && nodeid < STORYBOARD_MAXNODES)
		{
			fRet = Storyboard.NodeSliderValues[nodeid][iButton] = fNewValue;
		}
	}
	lua_pushnumber(L, fRet);
	return 1;
	//return 0;
}
int SetScreenWidgetSelection(lua_State *L)
{
	int iRet = -99999;
	char pScreenName[512];
	strcpy(pScreenName, lua_tostring(L, 1));
	int iButton = lua_tonumber(L, 2);
	if (iButton >= 0 && iButton < STORYBOARD_MAXOUTPUTS)
	{
		int nodeid = FindLuaScreenNode(pScreenName);
		if (nodeid >= 0 && nodeid < STORYBOARD_MAXNODES)
		{
			iRet = Storyboard.NodeRadioButtonSelected[nodeid] = iButton; // used by GRAPHICS SETTINGS (1,2,3)
		}
	}
	lua_pushnumber(L, iRet);
	return 1;
}
int GetScreenElementsType(lua_State* L)
{
	int iQty = 0;
	int nodeid = t.game.activeStoryboardScreen;
	if (nodeid == -1) nodeid = t.game.ingameHUDScreen;
	if (nodeid >= 0 && nodeid < STORYBOARD_MAXNODES)
	{
		char pReadoutName[512];
		strcpy(pReadoutName, lua_tostring(L, 1));
		if (strlen(pReadoutName) > 0)
		{
			int iPatternLength = strlen(pReadoutName) - 1;
			if (pReadoutName[iPatternLength] == '*')
			{
				// find pattern match with string passed in
				for (int n = 0; n < STORYBOARD_MAXWIDGETS; n++)
				{
					if (strnicmp(Storyboard.widget_readout[nodeid][n], pReadoutName, iPatternLength) == NULL)
					{
						iQty++;
					}
				}
			}
			else
			{
				// exact match
				for (int n = 0; n < STORYBOARD_MAXWIDGETS; n++)
				{
					if (stricmp(Storyboard.widget_readout[nodeid][n], pReadoutName) == NULL)
					{
						iQty++;
					}
				}
			}
		}
	}
	lua_pushnumber(L, iQty);
	return 1;
}
int GetScreenElementTypeID(lua_State* L)
{
	int iCount = 0;
	int iElementID = 0;
	int nodeid = t.game.activeStoryboardScreen;
	if (nodeid == -1) nodeid = t.game.ingameHUDScreen;
	if (nodeid >= 0 && nodeid < STORYBOARD_MAXNODES)
	{
		char pReadoutName[512];
		strcpy(pReadoutName, lua_tostring(L, 1));
		int iIndex = lua_tonumber(L, 2);
		for (int n = 0; n < STORYBOARD_MAXWIDGETS; n++)
		{
			int iPatternLength = strlen(pReadoutName) - 1;
			if (pReadoutName[iPatternLength] == '*')
			{
				// find pattern match with string passed in
				if (strnicmp(Storyboard.widget_readout[nodeid][n], pReadoutName, iPatternLength) == NULL)
				{
					iCount++;
					if (iCount == iIndex)
					{
						// found valid element from readout type match
						iElementID = 1 + n;

						// done
						break;
					}
				}
			}
			else
			{
				// exact match
				if (stricmp(Storyboard.widget_readout[nodeid][n], pReadoutName) == NULL)
				{
					iCount++;
					if (iCount == iIndex)
					{
						// found valid element from readout type match
						iElementID = 1 + n;

						// done
						break;
					}
				}
			}
		}
	}
	lua_pushnumber(L, iElementID);
	return 1;
}
int GetScreenElements(lua_State* L)
{
	int iQty = 0;
	int nodeid = t.game.activeStoryboardScreen;
	if (nodeid == -1) nodeid = t.game.ingameHUDScreen;
	if (nodeid >= 0 && nodeid < STORYBOARD_MAXNODES)
	{
		char pElementName[512];
		strcpy(pElementName, lua_tostring(L, 1));
		if (strlen(pElementName) > 0)
		{
			int iPatternLength = strlen(pElementName) - 1;
			if (pElementName[iPatternLength] == '*')
			{
				// find pattern match
				for (int n = 0; n < STORYBOARD_MAXWIDGETS; n++)
				{
					if (strnicmp(Storyboard.Nodes[nodeid].widget_label[n], pElementName, iPatternLength) == NULL)
					{
						iQty++;
					}
				}
			}
			else
			{
				// exact match
				for (int n = 0; n < STORYBOARD_MAXWIDGETS; n++)
				{
					if (stricmp(Storyboard.Nodes[nodeid].widget_label[n], pElementName) == NULL)
					{
						iQty++;
					}
				}
			}
		}
	}
	lua_pushnumber(L, iQty);
	return 1;
}
int GetScreenElementID(lua_State* L)
{
	int iCount = 0;
	int iElementID = 0;
	int nodeid = t.game.activeStoryboardScreen;
	if (nodeid == -1) nodeid = t.game.ingameHUDScreen;
	if (nodeid >= 0 && nodeid < STORYBOARD_MAXNODES)
	{
		char pElementName[512];
		strcpy(pElementName, lua_tostring(L, 1));
		int iIndex = lua_tonumber(L, 2);
		for (int n = 0; n < STORYBOARD_MAXWIDGETS; n++)
		{
			if (pElementName[0] == 's')
			{
				strcpy(pElementName, pElementName);
			}

			int iPatternLength = strlen(pElementName) - 1;
			if (pElementName[iPatternLength] == '*')
			{
				// find pattern match
				for (int n = 0; n < STORYBOARD_MAXWIDGETS; n++)
				{
					if (strnicmp(Storyboard.Nodes[nodeid].widget_label[n], pElementName, iPatternLength) == NULL)
					{
						iCount++;
						if (iCount == iIndex)
						{
							// found valid element
							iElementID = 1 + n;

							// done
							break;
						}
					}
				}
			}
			else
			{
				// exact match
				if (stricmp(Storyboard.Nodes[nodeid].widget_label[n], pElementName) == NULL)
				{
					iCount++;
					if (iCount == iIndex)
					{
						// found valid element
						iElementID = 1 + n;

						// done
						break;
					}
				}
			}
		}
	}
	lua_pushnumber(L, iElementID);
	return 1;
}
int GetScreenElementImage(lua_State* L)
{
	int iImgID = 0;
	int nodeid = t.game.activeStoryboardScreen;
	if (nodeid == -1) nodeid = t.game.ingameHUDScreen;
	if (nodeid >= 0 && nodeid < STORYBOARD_MAXNODES)
	{
		int iElementID = lua_tonumber(L, 1) - 1;
		if (iElementID >= 0 && iElementID < STORYBOARD_MAXWIDGETS)
		{
			iImgID = Storyboard.Nodes[nodeid].widget_normal_thumb_id[iElementID];
		}
	}
	lua_pushnumber(L, iImgID);
	return 1;
}
int GetScreenElementArea(lua_State* L)
{
	float fAreaX = 0;
	float fAreaY = 0;
	float fAreaWidth = 0;
	float fAreaHeight = 0;
	int nodeid = t.game.activeStoryboardScreen;
	if (nodeid == -1) nodeid = t.game.ingameHUDScreen;
	if (nodeid >= 0 && nodeid < STORYBOARD_MAXNODES)
	{
		int iElementID = lua_tonumber(L, 1) - 1;
		if (iElementID >= 0 && iElementID < STORYBOARD_MAXWIDGETS)
		{
			extern float screen_editor_scalemod (float);
			float fGlobalScale = screen_editor_scalemod((float)g_dwScreenWidth / 1920.0f);
			fAreaX = fabs(Storyboard.Nodes[nodeid].widget_pos[iElementID].x); // FABS to eliminate negative pos which is used to HIDE the widget
			fAreaY = fabs(Storyboard.Nodes[nodeid].widget_pos[iElementID].y); // FABS to eliminate negative pos which is used to HIDE the widget
			float widgetsizex = ImageWidth(Storyboard.Nodes[nodeid].widget_normal_thumb_id[iElementID]);
			float widgetsizey = ImageHeight(Storyboard.Nodes[nodeid].widget_normal_thumb_id[iElementID]);
			fAreaWidth = widgetsizex * fGlobalScale * Storyboard.Nodes[nodeid].widget_size[iElementID].x;
			fAreaHeight = widgetsizey * fGlobalScale * Storyboard.Nodes[nodeid].widget_size[iElementID].y;
			// convert to percentage system
			fAreaWidth = fabs(fAreaWidth / (float)g_dwScreenWidth) * 100.0f;
			fAreaHeight = fabs(fAreaHeight / (float)g_dwScreenHeight) * 100.0f;
			// position anchor is in middle of image!
			fAreaX -= fAreaWidth / 2.0f;
		}
	}
	lua_pushnumber(L, fAreaX);
	lua_pushnumber(L, fAreaY);
	lua_pushnumber(L, fAreaWidth);
	lua_pushnumber(L, fAreaHeight);
	return 4;
}
int GetScreenElementDetails(lua_State* L)
{
	float fRows = 0;
	float fColumns = 0;
	int nodeid = t.game.activeStoryboardScreen;
	if (nodeid == -1) nodeid = t.game.ingameHUDScreen;
	if (nodeid >= 0 && nodeid < STORYBOARD_MAXNODES)
	{
		int iElementID = lua_tonumber(L, 1) - 1;
		if (iElementID >= 0 && iElementID < STORYBOARD_MAXWIDGETS)
		{
			fRows = Storyboard.widget_textoffset[nodeid][iElementID].x;
			fColumns = Storyboard.widget_textoffset[nodeid][iElementID].y;
		}
	}
	lua_pushnumber(L, fRows);
	lua_pushnumber(L, fColumns);
	return 2;
}

int GetScreenElementName(lua_State* L)
{
	// prep return string
	char pReturnData[512];
	strcpy(pReturnData, "");
	int nodeid = t.game.activeStoryboardScreen;
	if (nodeid == -1) nodeid = t.game.ingameHUDScreen;
	if (nodeid >= 0 && nodeid < STORYBOARD_MAXNODES)
	{
		int iElementID = lua_tonumber(L, 1) - 1;
		if (iElementID >= 0 && iElementID < STORYBOARD_MAXWIDGETS)
		{
			strcpy(pReturnData, Storyboard.Nodes[nodeid].widget_label[iElementID]);
		}
	}
	lua_pushstring(L, pReturnData);
	return 1;
}
int SetScreenElementVisibility(lua_State* L)
{
	int nodeid = t.game.activeStoryboardScreen;
	if (nodeid == -1) nodeid = t.game.ingameHUDScreen;
	if (nodeid >= 0 && nodeid < STORYBOARD_MAXNODES)
	{
		int iElementID = lua_tonumber(L, 1) - 1;
		if (iElementID >= 0 && iElementID < STORYBOARD_MAXWIDGETS)
		{
			int iVisibility = lua_tonumber(L, 2);
			if ( iVisibility == 1 )
				Storyboard.widget_ingamehidden[nodeid][iElementID] = 0;
			else
				Storyboard.widget_ingamehidden[nodeid][iElementID] = 1;
		}
	}
	return 0;
}
int SetScreenElementPosition(lua_State* L)
{
	int nodeid = t.game.activeStoryboardScreen;
	if (nodeid == -1) nodeid = t.game.ingameHUDScreen;
	if (nodeid >= 0 && nodeid < STORYBOARD_MAXNODES)
	{
		int iElementID = lua_tonumber(L, 1) - 1;
		if (iElementID >= 0 && iElementID < STORYBOARD_MAXWIDGETS)
		{
			float fX = lua_tonumber(L, 2);
			float fY = lua_tonumber(L, 3);
			// and add back half of image width!
			float widgetsizex = ImageWidth(Storyboard.Nodes[nodeid].widget_normal_thumb_id[iElementID]);
			float fAreaWidth = widgetsizex * Storyboard.Nodes[nodeid].widget_size[iElementID].x;
			fAreaWidth = fabs(fAreaWidth / (float)g_dwScreenWidth) * 100.0f;
			fX += fAreaWidth / 2.0f;

			Storyboard.Nodes[nodeid].widget_pos[iElementID].x = fX;
			Storyboard.Nodes[nodeid].widget_pos[iElementID].y = fY;
		}
	}
	return 0;
}
int SetScreenElementText(lua_State* L)
{
	int nodeid = t.game.activeStoryboardScreen;
	if (nodeid == -1) nodeid = t.game.ingameHUDScreen;
	if (nodeid >= 0 && nodeid < STORYBOARD_MAXNODES)
	{
		int iElementID = lua_tonumber(L, 1) - 1;
		if (iElementID >= 0 && iElementID < STORYBOARD_MAXWIDGETS)
		{
			char* pLabel = (char*)lua_tostring(L, 2);
			char pUserDefinedGlobal[MAX_PATH];
			sprintf(pUserDefinedGlobal, "g_UserGlobal['%s']", Storyboard.Nodes[nodeid].widget_label[iElementID]);
			LuaSetString(pUserDefinedGlobal, pLabel);
		}
	}
	return 0;
}
int SetScreenElementColor(lua_State* L)
{
	int nodeid = t.game.activeStoryboardScreen;
	if (nodeid == -1) nodeid = t.game.ingameHUDScreen;
	if (nodeid >= 0 && nodeid < STORYBOARD_MAXNODES)
	{
		int iElementID = lua_tonumber(L, 1) - 1;
		if (iElementID >= 0 && iElementID < STORYBOARD_MAXWIDGETS)
		{
			float fX = lua_tonumber(L, 2);
			float fY = lua_tonumber(L, 3);
			float fZ = lua_tonumber(L, 4);
			float fW = lua_tonumber(L, 5);
			Storyboard.Nodes[nodeid].widget_font_color[iElementID].x = fX;
			Storyboard.Nodes[nodeid].widget_font_color[iElementID].y = fY;
			Storyboard.Nodes[nodeid].widget_font_color[iElementID].z = fZ;
			Storyboard.Nodes[nodeid].widget_font_color[iElementID].w = fW;
		}
	}
	return 0;
}

// Collection Items

int GetCollectionAttributeQuantity(lua_State* L)
{
	int iQty = g_collectionLabels.size();
	lua_pushnumber(L, iQty);
	return 1;
}
int GetCollectionAttributeLabel(lua_State* L)
{
	// prep return string
	char pReturnData[512];
	strcpy(pReturnData, "");

	// collection label
	int iCollectionLabelIndex = lua_tonumber(L, 1);
	if (iCollectionLabelIndex > 0 && iCollectionLabelIndex <= g_collectionLabels.size())
	{
		strcpy(pReturnData, g_collectionLabels[iCollectionLabelIndex-1].Get());
	}
	lua_pushstring(L, pReturnData);
	return 1;
}
int GetCollectionItemQuantity(lua_State* L)
{
	int iQty = g_collectionList.size();
	lua_pushnumber(L, iQty);
	return 1;
}
int GetCollectionItemAttribute(lua_State* L)
{
	// prep return string
	char pReturnData[512];
	strcpy(pReturnData, "");

	// which collection item
	int iCollectionListIndex = lua_tonumber(L, 1);
	if (iCollectionListIndex > 0 && iCollectionListIndex <= g_collectionList.size())
	{
		// find attribute label index
		int iLabelIndex = 0;
		char pAttributeLabel[512];
		strcpy(pAttributeLabel, lua_tostring(L, 2));
		for (iLabelIndex = 0; iLabelIndex < g_collectionLabels.size(); iLabelIndex++)
			if (stricmp(g_collectionLabels[iLabelIndex].Get(), pAttributeLabel) == NULL)
				break;

		// can pull field data from collection list item
		if (iLabelIndex < g_collectionList[iCollectionListIndex - 1].collectionFields.size())
		{
			strcpy(pReturnData, g_collectionList[iCollectionListIndex - 1].collectionFields[iLabelIndex].Get());
		}
	}
	lua_pushstring(L, pReturnData);
	return 1;
}

// Collection Quests

int GetCollectionQuestAttributeQuantity(lua_State* L)
{
	int iQty = g_collectionQuestLabels.size();
	lua_pushnumber(L, iQty);
	return 1;
}
int GetCollectionQuestAttributeLabel(lua_State* L)
{
	// collection label
	char pReturnData[512];
	strcpy(pReturnData, "");
	int iCollectionLabelIndex = lua_tonumber(L, 1);
	if (iCollectionLabelIndex > 0 && iCollectionLabelIndex <= g_collectionQuestLabels.size())
	{
		strcpy(pReturnData, g_collectionQuestLabels[iCollectionLabelIndex - 1].Get());
	}
	lua_pushstring(L, pReturnData);
	return 1;
}
int GetCollectionQuestQuantity(lua_State* L)
{
	int iQty = g_collectionQuestList.size();
	lua_pushnumber(L, iQty);
	return 1;
}
int GetCollectionQuestAttribute(lua_State* L)
{
	// which collection quest
	char pReturnData[512];
	strcpy(pReturnData, "");
	int iCollectionListIndex = lua_tonumber(L, 1);
	if (iCollectionListIndex > 0 && iCollectionListIndex <= g_collectionQuestList.size())
	{
		// find attribute label index
		int iLabelIndex = 0;
		char pAttributeLabel[512];
		strcpy(pAttributeLabel, lua_tostring(L, 2));
		for (iLabelIndex = 0; iLabelIndex < g_collectionQuestLabels.size(); iLabelIndex++)
			if (stricmp(g_collectionQuestLabels[iLabelIndex].Get(), pAttributeLabel) == NULL)
				break;

		// can pull field data from collection list quest
		if (iLabelIndex < g_collectionQuestList[iCollectionListIndex - 1].collectionFields.size())
		{
			strcpy(pReturnData, g_collectionQuestList[iCollectionListIndex - 1].collectionFields[iLabelIndex].Get());
		}
	}
	lua_pushstring(L, pReturnData);
	return 1;
}

// Inventory containers

int FindInventoryIndex (LPSTR pNameOfInventory)
{
	int bothplayercontainers = -1;
	for (int n = 0; n < t.inventoryContainers.size(); n++)
	{
		if (stricmp(t.inventoryContainers[n].Get(), pNameOfInventory) == NULL)
		{
			bothplayercontainers = n;
			break;
		}
	}
	return bothplayercontainers;
}
int MakeInventoryContainer (lua_State* L)
{
	int containerindex = -1;
	char pNameOfInventory[512];
	strcpy(pNameOfInventory, lua_tostring(L, 1));
	for (int n = 0; n < t.inventoryContainers.size(); n++)
	{
		if (stricmp(t.inventoryContainers[n].Get(), pNameOfInventory) == NULL)
		{
			containerindex = n;
			break;
		}
	}
	if (containerindex == -1)
	{
		// create new container
		t.inventoryContainers.push_back(pNameOfInventory);
		containerindex = t.inventoryContainers.size() - 1;
		t.inventoryContainer[containerindex].clear();
	}
	lua_pushnumber(L, containerindex);
	return 1;
}
int GetInventoryTotal(lua_State* L)
{
	int iQty = t.inventoryContainers.size();
	lua_pushnumber(L, iQty);
	return 1;
}
int GetInventoryName(lua_State* L)
{
	int iInventoryIndex = lua_tonumber(L, 1);
	char pNameOfInventory[512];
	strcpy(pNameOfInventory, "");
	if (iInventoryIndex >= 0 && iInventoryIndex < t.inventoryContainers.size())
		strcpy(pNameOfInventory, t.inventoryContainers[iInventoryIndex].Get());
	lua_pushstring(L, pNameOfInventory);
	return 1;
}
int GetInventoryExist(lua_State* L)
{
	int iExist = 0;
	t.game.activeStoryboardScreen = -1;
	LPSTR pScreenTitle = "HUD Screen 2"; // traditionally the template RPG INVENTORY SCREEN!
	int nodeid = FindLuaScreenTitleNode(pScreenTitle);
	if (nodeid >= 0)
	{
		char pNameOfInventory[512];
		strcpy(pNameOfInventory, lua_tostring(L, 1));
		int bothplayercontainers = FindInventoryIndex(pNameOfInventory);
		if (bothplayercontainers >= 0) iExist = 1;
	}
	lua_pushnumber(L, iExist);
	return 1;
}
int GetInventoryQuantity(lua_State* L)
{
	int iQty = 0;
	char pNameOfInventory[512];
	strcpy(pNameOfInventory, lua_tostring(L, 1));
	int bothplayercontainers = FindInventoryIndex(pNameOfInventory);
	if (bothplayercontainers >= 0)
	{
		iQty = t.inventoryContainer[bothplayercontainers].size();
	}
	lua_pushnumber(L, iQty);
	return 1;
}
int GetInventoryItem(lua_State* L)
{
	int iCollectionItemID = 0;
	char pNameOfInventory[512];
	strcpy(pNameOfInventory, lua_tostring(L, 1));
	int bothplayercontainers = FindInventoryIndex(pNameOfInventory);
	if (bothplayercontainers >= 0)
	{
		int iInventoryIndex = lua_tonumber(L, 2);
		if (iInventoryIndex > 0 && iInventoryIndex <= t.inventoryContainer[bothplayercontainers].size())
		{
			iCollectionItemID = t.inventoryContainer[bothplayercontainers][iInventoryIndex - 1].collectionID;
		}
	}
	lua_pushnumber(L, iCollectionItemID);
	return 1;
}
int GetInventoryItemID(lua_State* L)
{
	int iItemEntityID = 0;
	char pNameOfInventory[512];
	strcpy(pNameOfInventory, lua_tostring(L, 1));
	int bothplayercontainers = FindInventoryIndex(pNameOfInventory);
	if (bothplayercontainers >= 0)
	{
		int iInventoryIndex = lua_tonumber(L, 2);
		if (iInventoryIndex > 0 && iInventoryIndex <= t.inventoryContainer[bothplayercontainers].size())
		{
			iItemEntityID = t.inventoryContainer[bothplayercontainers][iInventoryIndex - 1].e;
		}
	}
	lua_pushnumber(L, iItemEntityID);
	return 1;
}
int GetInventoryItemSlot(lua_State* L)
{
	int iItemSlot = 0;
	char pNameOfInventory[512];
	strcpy(pNameOfInventory, lua_tostring(L, 1));
	int bothplayercontainers = FindInventoryIndex(pNameOfInventory);
	if (bothplayercontainers >= 0)
	{
		int iInventoryIndex = lua_tonumber(L, 2);
		if (iInventoryIndex > 0 && iInventoryIndex <= t.inventoryContainer[bothplayercontainers].size())
		{
			iItemSlot = t.inventoryContainer[bothplayercontainers][iInventoryIndex - 1].slot;
		}
	}
	lua_pushnumber(L, iItemSlot);
	return 1;
}
int SetInventoryItemSlot(lua_State* L)
{
	char pNameOfInventory[512];
	strcpy(pNameOfInventory, lua_tostring(L, 1));
	int bothplayercontainers = FindInventoryIndex(pNameOfInventory);
	if (bothplayercontainers >= 0)
	{
		int iInventoryIndex = lua_tonumber(L, 2);
		if (iInventoryIndex > 0 && iInventoryIndex <= t.inventoryContainer[bothplayercontainers].size())
		{
			int iNewSlotIndex = lua_tonumber(L, 3);
			t.inventoryContainer[bothplayercontainers][iInventoryIndex - 1].slot = iNewSlotIndex;
		}
	}
	return 0;
}
int MoveInventoryItem (lua_State* L)
{
	lua2 = L;
	int n = lua_gettop(L);
	if (n < 5) return 0;
	char pNameOfInventoryFrom[512];
	strcpy(pNameOfInventoryFrom, lua_tostring(L, 1));
	int collectionindex = lua_tonumber(L, 3);
	int entityindex = lua_tonumber(L, 4);
	int bothplayercontainersfrom = FindInventoryIndex(pNameOfInventoryFrom);
	if (bothplayercontainersfrom >= 0)
	{
		bool bNeedToRecreateContainerFrom = false;
		char pNameOfInventoryTo[512];
		strcpy(pNameOfInventoryTo, lua_tostring(L, 2));
		int bothplayercontainersto = FindInventoryIndex(pNameOfInventoryTo);
		if (bothplayercontainersto >= 0)
		{
			int slotindex = lua_tonumber(L, 5);
			if (bothplayercontainersfrom == bothplayercontainersto)
			{
				// moved within same container
				int iListSize = t.inventoryContainer[bothplayercontainersfrom].size();
				for (int n = 0; n < iListSize; n++)
				{
					if (slotindex != -1)
					{
						if (collectionindex == -1)
						{
							if (t.inventoryContainer[bothplayercontainersfrom][n].e == entityindex)
							{
								t.inventoryContainer[bothplayercontainersfrom][n].slot = slotindex;
								break;
							}
						}
						else
						{
							if (t.inventoryContainer[bothplayercontainersfrom][n].collectionID == collectionindex)
							{
								t.inventoryContainer[bothplayercontainersfrom][n].slot = slotindex;
								break;
							}
						}
					}
				}
			}
			else
			{
				// different containers
				int iListSize = t.inventoryContainer[bothplayercontainersfrom].size();
				for (int n = 0; n < iListSize; n++)
				{
					bool bMatch = false;
					if (collectionindex == -1)
					{
						if (t.inventoryContainer[bothplayercontainersfrom][n].e == entityindex)
							bMatch = true;
					}
					else
					{
						if (t.inventoryContainer[bothplayercontainersfrom][n].collectionID == collectionindex)
							bMatch = true;
					}
					if (bMatch==true)
					{
						// create item
						inventoryContainerType item;
						item.e = t.inventoryContainer[bothplayercontainersfrom][n].e;
						item.collectionID = t.inventoryContainer[bothplayercontainersfrom][n].collectionID;
						item.slot = slotindex;
						if (item.slot == -1)
						{
							item.slot = 0;
							while (1)
							{
								bool bCanIUseThisSlot = true;
								for (int nn = 0; nn < t.inventoryContainer[bothplayercontainersto].size(); nn++)
								{
									if (t.inventoryContainer[bothplayercontainersto][nn].slot == item.slot)
									{
										bCanIUseThisSlot = false;
										break;
									}
								}
								if (bCanIUseThisSlot == false)
									item.slot++;
								else
									break;
							}
						}
						// remove from old (below)
						t.inventoryContainer[bothplayercontainersfrom][n].e = -2;
						bNeedToRecreateContainerFrom = true;
						// add new item or increase resource quantity
						bool bWeAddedToQuantityOfAnother = false;
						if ( item.e > 0 && t.entityelement[item.e].eleprof.iscollectable == 2)
						{
							int iListToSize = t.inventoryContainer[bothplayercontainersto].size();
							for (int n = 0; n < iListToSize; n++)
							{
								if (t.inventoryContainer[bothplayercontainersto][n].collectionID == item.collectionID)
								{
									// can add to resource, no need to create new item in dest
									int ee = t.inventoryContainer[bothplayercontainersto][n].e;
									if (ee > 0)
									{
										int iQtyToAdd = t.entityelement[item.e].eleprof.quantity;
										if (iQtyToAdd < 1) iQtyToAdd = 1;
										t.entityelement[ee].eleprof.quantity += iQtyToAdd;
										t.entityelement[item.e].eleprof.quantity = 0;
										bWeAddedToQuantityOfAnother = true;
										break;
									}
								}
							}
						}
						if (bWeAddedToQuantityOfAnother == false )
						{
							// add new item to dest
							t.inventoryContainer[bothplayercontainersto].push_back(item);
						}
						// ensure activate and deactivate entity as it passes from player/hotkeys to shop/chest/etc
						if (item.e > 0)
						{
							if (bothplayercontainersto == 0 || bothplayercontainersto == 1)
								t.entityelement[item.e].active = 1;
							else
								t.entityelement[item.e].active = 0;
						}
						break;
					}
				}
			}
		}
		else
		{
			// moving item to limbo (for resources that hit zero quantity)
			int iListSize = t.inventoryContainer[bothplayercontainersfrom].size();
			for (int n = 0; n < iListSize; n++)
			{
				bool bMatch = false;
				if (collectionindex == -1)
				{
					if (t.inventoryContainer[bothplayercontainersfrom][n].e == entityindex)
						bMatch = true;
				}
				else
				{
					if (t.inventoryContainer[bothplayercontainersfrom][n].collectionID == collectionindex)
						bMatch = true;
				}
				if (bMatch == true)
				{
					// remove (below)
					t.inventoryContainer[bothplayercontainersfrom][n].e = -2;
					bNeedToRecreateContainerFrom = true;
					break;
				}
			}
		}
		if ( bNeedToRecreateContainerFrom == true)
		{
			std::vector <inventoryContainerType> inventoryContainerTemp;
			inventoryContainerTemp.clear();
			int iListSize = t.inventoryContainer[bothplayercontainersfrom].size();
			for (int n = 0; n < iListSize; n++)
			{
				if (t.inventoryContainer[bothplayercontainersfrom][n].e != -2)
				{
					inventoryContainerTemp.push_back(t.inventoryContainer[bothplayercontainersfrom][n]);
				}
			}
			t.inventoryContainer[bothplayercontainersfrom] = inventoryContainerTemp;
		}
	}
	return 0;
}
int DeleteAllInventoryContainers (lua_State* L)
{
	for (int c = 0; c < t.inventoryContainers.size(); c++)
	{
		t.inventoryContainer[c].clear();
	}
	t.inventoryContainers.clear();
	return 0;
}
int AddInventoryItem (lua_State* L)
{
	lua2 = L;
	int n = lua_gettop(L);
	if (n < 4) return 0;
	char pNameOfInventoryTo[512];
	strcpy(pNameOfInventoryTo, lua_tostring(L, 1));
	int bothplayercontainersto = FindInventoryIndex(pNameOfInventoryTo);
	if (bothplayercontainersto >= 0)
	{
		int collectionindex = lua_tonumber(L, 2);
		int newe = lua_tonumber(L, 3);
		int slotindex = lua_tonumber(L, 4);
		inventoryContainerType item;
		item.e = newe;
		item.collectionID = collectionindex;
		item.slot = slotindex;
		t.inventoryContainer[bothplayercontainersto].push_back(item);
		// ensure activate and deactivate entity as it passes from player/hotkeys to shop/chest/etc
		if (bothplayercontainersto == 0 || bothplayercontainersto == 1)
			t.entityelement[item.e].active = 1;
		else
			t.entityelement[item.e].active = 0;
	}
	return 0;
}

#endif

// Game Player Control/State Set/Get commands

int SetGamePlayerControlData ( lua_State *L, int iDataMode )
{
	lua2 = L;
	int iSrc = 0;
	int iDest = 0;
	int n = lua_gettop(L);
	if ( iDataMode < 500 )
	{
		if ( n < 1 ) return 0;
	}
	else
	{
		if ( n < 2 ) return 0;
	}
	int gunId = t.gunid;
	int fireMode = g.firemode;
	int param = 1;
	if ( n > 1 && iDataMode > 200 && iDataMode < 500 ) 
	{
		gunId = lua_tonumber( L, 1 );
		if ( n == 2 )
		{
			fireMode = 0;
			param = 2;
		}
		else
		{
			fireMode = lua_tonumber( L, 2 );
			param = 3;
		}
	}
	switch ( iDataMode )
	{
		case 1 : t.playercontrol.jetpackmode = lua_tonumber(L, 1); break;
		case 2 : t.playercontrol.jetpackfuel_f = lua_tonumber(L, 1); break;
		case 3 : t.playercontrol.jetpackhidden = lua_tonumber(L, 1); break;
		case 4 : t.playercontrol.jetpackcollected = lua_tonumber(L, 1); break;
		case 5 : t.playercontrol.soundstartindex = lua_tonumber(L, 1); break;
		case 6 : t.playercontrol.jetpackparticleemitterindex = lua_tonumber(L, 1); break;
		case 7 : t.playercontrol.jetpackthrust_f = lua_tonumber(L, 1); break;
		case 8 : t.playercontrol.startstrength = lua_tonumber(L, 1); break;

		case 9 : t.playercontrol.isrunning = lua_tonumber(L, 1); break;
		case 10 : break;
		case 11 : t.playercontrol.cx_f = lua_tonumber(L, 1); break;
		case 12 : t.playercontrol.cy_f = lua_tonumber(L, 1); break;
		case 13 : t.playercontrol.cz_f = lua_tonumber(L, 1); break;
		case 14 : 
		{
#ifdef FASTBULLETPHYSICS
			t.playercontrol.basespeed_f *= 2.0; //FASTBULLETPHYSICS need to move faster.
#else
			t.playercontrol.basespeed_f = lua_tonumber(L, 1);
			extern bool bPhysicsRunningAt120FPS;
			if (!bPhysicsRunningAt120FPS)
				t.playercontrol.basespeed_f *= 2.0;
#endif
			break;
		}
		case 15 : t.playercontrol.canrun = lua_tonumber(L, 1); break;
		case 16 : t.playercontrol.maxspeed_f = lua_tonumber(L, 1); break;
		case 17 : t.playercontrol.topspeed_f = lua_tonumber(L, 1); break;
		case 18 : t.playercontrol.movement = lua_tonumber(L, 1); break;
		case 19 : t.playercontrol.movey_f = lua_tonumber(L, 1); break;
		case 20 : t.playercontrol.lastmovement = lua_tonumber(L, 1); break;
		case 21 : t.playercontrol.footfallcount = lua_tonumber(L, 1); break;
		case 22 : break;
		case 23 : t.playercontrol.gravityactive = lua_tonumber(L, 1); break;
		case 24 : t.playercontrol.plrhitfloormaterial = lua_tonumber(L, 1); break;
		case 25:  t.playercontrol.underwater = lua_tonumber(L, 1); break;
		case 26 : t.playercontrol.jumpmode = lua_tonumber(L, 1); break;
		case 27 : t.playercontrol.jumpmodecanaffectvelocitycountdown_f = lua_tonumber(L, 1); break;
		case 28 : t.playercontrol.speed_f = lua_tonumber(L, 1); break;
		case 29 : t.playercontrol.accel_f = lua_tonumber(L, 1); break;
		case 30 : t.playercontrol.speedratio_f = lua_tonumber(L, 1); break;
		case 31 : t.playercontrol.wobble_f = lua_tonumber(L, 1); break;
		case 32 : t.playercontrol.wobblespeed_f = lua_tonumber(L, 1); break;
		case 33 : t.playercontrol.wobbleheight_f = lua_tonumber(L, 1); break;
		case 34 : t.playercontrol.jumpmax_f = lua_tonumber(L, 1); break;
		case 35 : t.playercontrol.pushangle_f = lua_tonumber(L, 1); break;
		case 36 : t.playercontrol.pushforce_f = lua_tonumber(L, 1); break;
		case 37 : t.playercontrol.footfallpace_f = lua_tonumber(L, 1); break;
		case 38 : t.playercontrol.lockatheight = lua_tonumber(L, 1); break;
		case 39 : t.playercontrol.controlheight = lua_tonumber(L, 1); break;
		case 40 : t.playercontrol.controlheightcooldown = lua_tonumber(L, 1); break;
		case 41 : t.playercontrol.storemovey = lua_tonumber(L, 1); break;
		case 42 : break;
		case 43 : t.playercontrol.hurtfall = lua_tonumber(L, 1); break;
		case 44 : t.playercontrol.leanoverangle_f = lua_tonumber(L, 1); break;
		case 45 : t.playercontrol.leanover_f = lua_tonumber(L, 1); break;
		case 46 : 
			t.playercontrol.camerashake_f = lua_tonumber(L, 1); 
			//#ifdef WICKEDENGINE
			//if (t.playercontrol.camerashake_f > 25.0f)  t.playercontrol.camerashake_f = 25.0f;
			//#endif
			break;
		case 47 : t.playercontrol.finalcameraanglex_f = lua_tonumber(L, 1); break;
		case 48 : t.playercontrol.finalcameraangley_f = lua_tonumber(L, 1); break;
		case 49 : t.playercontrol.finalcameraanglez_f = lua_tonumber(L, 1); break;
		case 50 : t.playercontrol.camrightmousemode = lua_tonumber(L, 1); break;
		case 51 : t.playercontrol.camcollisionsmooth = lua_tonumber(L, 1); break;
		case 52 : t.playercontrol.camcurrentdistance = lua_tonumber(L, 1); break;
		case 53 : t.playercontrol.camdofullraycheck = lua_tonumber(L, 1); break;
		case 54 : t.playercontrol.lastgoodcx_f = lua_tonumber(L, 1); break;
		case 55 : t.playercontrol.lastgoodcy_f = lua_tonumber(L, 1); break;
		case 56 : t.playercontrol.lastgoodcz_f = lua_tonumber(L, 1); break;
		case 57 : break;
		case 58 : t.playercontrol.flinchx_f = lua_tonumber(L, 1); break;
		case 59 : t.playercontrol.flinchy_f = lua_tonumber(L, 1); break;
		case 60 : t.playercontrol.flinchz_f = lua_tonumber(L, 1); break;
		case 61 : t.playercontrol.flinchcurrentx_f = lua_tonumber(L, 1); break;
		case 62 : t.playercontrol.flinchcurrenty_f = lua_tonumber(L, 1); break;
		case 63 : t.playercontrol.flinchcurrentz_f = lua_tonumber(L, 1); break;
		case 64 : t.playercontrol.footfalltype = lua_tonumber(L, 1); break;
		case 65 : t.playercontrol.ripplecount_f = lua_tonumber(L, 1); break;
		case 66 : t.playercontrol.lastfootfallsound = lua_tonumber(L, 1); break;
		case 67 : t.playercontrol.inwaterstate = lua_tonumber(L, 1); break;
		case 68 : t.playercontrol.drowntimestamp = lua_tonumber(L, 1); break;
		case 69 : t.playercontrol.deadtime = lua_tonumber(L, 1); break;
		case 70 : t.playercontrol.swimtimestamp = lua_tonumber(L, 1); break;
		case 71 : t.playercontrol.redDeathFog_f = lua_tonumber(L, 1); break;
		case 72 : t.playercontrol.heartbeatTimeStamp = lua_tonumber(L, 1); break;
		case 81 : t.playercontrol.thirdperson.enabled = lua_tonumber(L, 1); break;
		case 82 : t.playercontrol.thirdperson.characterindex = lua_tonumber(L, 1); break;
		case 83 : t.playercontrol.thirdperson.camerafollow = lua_tonumber(L, 1); break;
		case 84 : t.playercontrol.thirdperson.camerafocus = lua_tonumber(L, 1); break;
		case 85 : t.playercontrol.thirdperson.charactere = lua_tonumber(L, 1); break;
		case 86 : break;
		case 87 : t.playercontrol.thirdperson.shotfired = lua_tonumber(L, 1); break;
		case 88 : t.playercontrol.thirdperson.cameradistance = lua_tonumber(L, 1); break;
		case 89 : t.playercontrol.thirdperson.cameraspeed = lua_tonumber(L, 1); break;
		case 90 : t.playercontrol.thirdperson.cameralocked = lua_tonumber(L, 1); break;
		case 91 : t.playercontrol.thirdperson.cameraheight = lua_tonumber(L, 1); break;
		case 92 : t.playercontrol.thirdperson.camerashoulder = lua_tonumber(L, 1); break;

		case 99 : break;
		case 101 : t.gunmode = lua_tonumber(L, 1); break;
		case 102 : t.player[t.plrid].state.firingmode = lua_tonumber(L, 1); break;
		case 103 : g.weaponammoindex = lua_tonumber(L, 1); break;
		case 104 : g.ammooffset = lua_tonumber(L, 1); break;
		case 105 : g.ggunmeleekey = lua_tonumber(L, 1); break;
		case 106 : t.player[t.plrid].state.blockingaction = lua_tonumber(L, 1); break;
		case 107 : t.gunshootnoammo = lua_tonumber(L, 1); break;
		case 108 : g.playerunderwater = lua_tonumber(L, 1); break;
		case 109 : g.gdisablerightmousehold = lua_tonumber(L, 1); break;
		case 110 : g.gxbox = lua_tonumber(L, 1); break;
		case 111 : break;
		case 112 : break;
		case 113 : break;
		case 114 : t.gunzoommode = lua_tonumber(L, 1); break;
		case 115 : t.gunzoommag_f = lua_tonumber(L, 1); break;
		case 116 : t.gunreloadnoammo = lua_tonumber(L, 1); break;
		case 117 : g.plrreloading = lua_tonumber(L, 1); break;
		case 118 : g.ggunaltswapkey1 = lua_tonumber(L, 1); break;
		case 119 : g.ggunaltswapkey2 = lua_tonumber(L, 1); break;
		case 120 : 
		{
			t.weaponkeyselection = lua_tonumber(L, 1);
			// Trigger zoom out so that the player doesn't stay zoomed in when picking up a new weapon
			if (t.gunzoommode == 9 || t.gunzoommode == 10)
			{
				t.gunzoommode = 11;
			}
		}
		break;

		case 121 : t.weaponindex = lua_tonumber(L, 1); break;
		case 122 : t.player[t.plrid].command.newweapon = lua_tonumber(L, 1); break;
		case 123 : t.gunid = lua_tonumber(L, 1); break;
		case 124 : t.gunselectionafterhide = lua_tonumber(L, 1); break;
		case 125 : t.gunburst = lua_tonumber(L, 1); break;
		case 126 : break;
		case 127 : break;
		case 128 : break;
		case 129 : t.plrkeyForceKeystate = lua_tonumber(L, 1); break;
		case 130 : t.plrzoominchange = lua_tonumber(L, 1); break;
		case 131 : t.plrzoomin_f = lua_tonumber(L, 1); break;
		case 132 : g.luaactivatemouse = lua_tonumber(L, 1); break;
		case 133 : g.realfov_f = lua_tonumber(L, 1); break;
		case 134 : g.gdisablepeeking = lua_tonumber(L, 1); break;
		case 135 : t.plrhasfocus = lua_tonumber(L, 1); break;
		case 136 : t.game.runasmultiplayer = lua_tonumber(L, 1); break;
		case 137 : g.mp.respawnLeft = lua_tonumber(L, 1); break;
		case 138 : g.tabmode = lua_tonumber(L, 1); break;
		case 139 : g.lowfpswarning = lua_tonumber(L, 1); break;
		case 140 : t.visuals.CameraFOV_f = lua_tonumber(L, 1); break;
		case 141 : t.visuals.CameraFOVZoomed_f = lua_tonumber(L, 1); break;
		case 142 : g.gminvert = lua_tonumber(L, 1); break;
		case 143 : t.plrkeySLOWMOTION = lua_tonumber(L, 1); break;
		case 144 : g.globals.smoothcamerakeys = lua_tonumber(L, 1); break;
		case 145 : t.cammousemovex_f = lua_tonumber(L, 1); break;
		case 146 : t.cammousemovey_f = lua_tonumber(L, 1); break;
		case 147 : g.gunRecoilX_f = lua_tonumber(L, 1); break;
		case 148 : g.gunRecoilY_f = lua_tonumber(L, 1); break;
		case 149 : g.gunRecoilAngleX_f = lua_tonumber(L, 1); break;
		case 150 : g.gunRecoilAngleY_f = lua_tonumber(L, 1); break;
		case 151 : t.gunRecoilCorrectY_f = lua_tonumber(L, 1); break;
		case 152 : g.gunRecoilCorrectX_f = lua_tonumber(L, 1); break;
		case 153 : g.gunRecoilCorrectAngleY_f = lua_tonumber(L, 1); break;
		case 154 : t.gunRecoilCorrectAngleX_f = lua_tonumber(L, 1); break;
		case 155 : t.camangx_f = lua_tonumber(L, 1); break;
		case 156 : t.camangy_f = lua_tonumber(L, 1); break;
		case 157 : t.aisystem.playerducking = lua_tonumber(L, 1); break;
		case 158 : t.conkit.editmodeactive = lua_tonumber(L, 1); break;
		case 159 : t.plrkeySHIFT = lua_tonumber(L, 1); break;
		case 160 : t.plrkeySHIFT2 = lua_tonumber(L, 1); break;
		case 161 : t.inputsys.keycontrol = lua_tonumber(L, 1); break;
		case 162 : t.hardwareinfoglobals.nowater = lua_tonumber(L, 1); break;
		case 163 : t.terrain.waterliney_f = lua_tonumber(L, 1); break;
		case 164 : g.flashLightKeyEnabled = lua_tonumber(L, 1); break;
		case 165 : t.playerlight.flashlightcontrol_f = lua_tonumber(L, 1); break;
		case 166 : t.player[t.plrid].state.moving = lua_tonumber(L, 1); break;
		case 167 : t.tplayerterrainheight_f = lua_tonumber(L, 1); break;
		case 168 : 
			t.tjetpackverticalmove_f = lua_tonumber(L, 1);
			break;
		case 169 : t.terrain.TerrainID = lua_tonumber(L, 1); break;
		case 170 : g.globals.enableplrspeedmods = lua_tonumber(L, 1); break;
		case 171 : g.globals.riftmode = lua_tonumber(L, 1); break;
		case 172 : t.tplayerx_f = lua_tonumber(L, 1); break;
		case 173 : t.tplayery_f = lua_tonumber(L, 1); break;
		case 174 : t.tplayerz_f = lua_tonumber(L, 1); break;
		case 175 : t.terrain.playerx_f = lua_tonumber(L, 1); break;
		case 176 : t.terrain.playery_f = lua_tonumber(L, 1); break;
		case 177 : t.terrain.playerz_f = lua_tonumber(L, 1); break;
		case 178 : t.terrain.playerax_f = lua_tonumber(L, 1); break;
		case 179 : t.terrain.playeray_f = lua_tonumber(L, 1); break;
		case 180 : t.terrain.playeraz_f = lua_tonumber(L, 1); break;
		case 181 : t.tadjustbasedonwobbley_f = lua_tonumber(L, 1); break;
		case 182 : t.tFinalCamX_f = lua_tonumber(L, 1); break;
		case 183 : t.tFinalCamY_f = lua_tonumber(L, 1); break;
		case 184 : t.tFinalCamZ_f = lua_tonumber(L, 1); break;
		case 185 : t.tshakex_f = lua_tonumber(L, 1); break;
		case 186 : t.tshakey_f = lua_tonumber(L, 1); break;
		case 187 : t.tshakez_f = lua_tonumber(L, 1); break;		
		case 188 : t.huddamage.immunity = lua_tonumber(L, 1); break;		
		case 189 : g.charanimindex = lua_tonumber(L, 1); break;	

		// 190-200 reserved for MOTION CONTROLLER actions
		case 201: t.gun[gunId].settings.ismelee = lua_tonumber(L, param); break;
		case 202 : t.gun[gunId].settings.alternate       = lua_tonumber( L, param ); break;
		case 203 : t.gun[gunId].settings.modessharemags  = lua_tonumber( L, param ); break;
		case 204 : t.gun[gunId].settings.alternateisflak = lua_tonumber( L, param ); break;
		case 205 : t.gun[gunId].settings.alternateisray  = lua_tonumber( L, param ); break;
		case 301 : g.firemodes[gunId][fireMode].settings.reloadqty         = lua_tonumber( L, param ); break;
		case 302 : g.firemodes[gunId][fireMode].settings.isempty           = lua_tonumber( L, param ); break;
		case 303 : g.firemodes[gunId][fireMode].settings.jammed            = lua_tonumber( L, param ); break;
		case 304 : g.firemodes[gunId][fireMode].settings.jamchance         = lua_tonumber( L, param ); break;
		case 305 : g.firemodes[gunId][fireMode].settings.mintimer          = lua_tonumber( L, param ); break;
		case 306 : g.firemodes[gunId][fireMode].settings.addtimer          = lua_tonumber( L, param ); break;
		case 307 : g.firemodes[gunId][fireMode].settings.shotsfired        = lua_tonumber( L, param ); break;
		case 308 : g.firemodes[gunId][fireMode].settings.cooltimer         = lua_tonumber( L, param ); break;
		case 309 : g.firemodes[gunId][fireMode].settings.overheatafter     = lua_tonumber( L, param ); break;
		case 310 : g.firemodes[gunId][fireMode].settings.jamchancetime     = lua_tonumber( L, param ); break;
		case 311 : g.firemodes[gunId][fireMode].settings.cooldown          = lua_tonumber( L, param ); break;
		case 312 : g.firemodes[gunId][fireMode].settings.nosubmergedfire   = lua_tonumber( L, param ); break;
		case 313 : g.firemodes[gunId][fireMode].settings.simplezoom        = lua_tonumber( L, param ); break;
		case 314 : g.firemodes[gunId][fireMode].settings.forcezoomout      = lua_tonumber( L, param ); break;
		case 315 : g.firemodes[gunId][fireMode].settings.zoommode          = lua_tonumber( L, param ); break;
		case 316 : g.firemodes[gunId][fireMode].settings.simplezoomanim    = lua_tonumber( L, param ); break;
		case 317 : g.firemodes[gunId][fireMode].settings.poolindex         = lua_tonumber( L, param ); break;
		case 318 : g.firemodes[gunId][fireMode].settings.plrturnspeedmod   = lua_tonumber( L, param ); break;
		case 319 : g.firemodes[gunId][fireMode].settings.zoomturnspeed     = lua_tonumber( L, param ); break;
		case 320 : g.firemodes[gunId][fireMode].settings.plrjumpspeedmod   = lua_tonumber( L, param ); break;
		case 321 : g.firemodes[gunId][fireMode].settings.plremptyspeedmod  = lua_tonumber( L, param ); break;
		case 322 : g.firemodes[gunId][fireMode].settings.plrmovespeedmod   = lua_tonumber( L, param ); break;
		case 323 : g.firemodes[gunId][fireMode].settings.zoomwalkspeed     = lua_tonumber( L, param ); break;
		case 324 : g.firemodes[gunId][fireMode].settings.plrreloadspeedmod = lua_tonumber( L, param ); break;
		case 325 : g.firemodes[gunId][fireMode].settings.hasempty          = lua_tonumber( L, param ); break;
		case 326 : g.firemodes[gunId][fireMode].action.block.s             = lua_tonumber( L, param ); break;

		// for extra commands as yet unimagined :)
		case 401: t.playerlight.flashlightcontrol_range_f = lua_tonumber(L, 1);		break;
		case 402: t.playerlight.flashlightcontrol_radius_f = lua_tonumber(L, 1);	break;
		case 403: t.playerlight.flashlightcontrol_colorR_f = lua_tonumber(L, 1);	break;
		case 404: t.playerlight.flashlightcontrol_colorG_f = lua_tonumber(L, 1);	break;
		case 405: t.playerlight.flashlightcontrol_colorB_f = lua_tonumber(L, 1);	break;
		case 406: t.playerlight.flashlightcontrol_cashshadow = lua_tonumber(L, 1);	break;

		case 501 : t.gunsound[t.gunid][lua_tonumber(L, 1)].soundid1 = lua_tonumber(L, 2); break;
		case 502 : t.gunsound[t.gunid][lua_tonumber(L, 1)].altsoundid = lua_tonumber(L, 2); break;
		case 503 : break;
		case 504 : break;

		case 601: t.player[t.plrid].state.counteredaction = lua_tonumber(L, 1); break;

		case 700 :  iSrc = lua_tonumber(L, 1);
					iDest = lua_tonumber(L, 2);
					if ( iDest == 0 ) 
						t.charanimstate = t.charanimstates[lua_tonumber(L, 1)]; 
					else
						if ( iSrc == 0 ) 
							t.charanimstates[iDest] = t.charanimstate;
						else
							t.charanimstates[iDest] = t.charanimstates[iSrc]; 
					break;
		case 701 :	iDest = lua_tonumber(L, 1);
					if ( iDest == 0 ) 
						t.charanimstate.playcsi = lua_tonumber(L, 2); 
					else
						t.charanimstates[iDest].playcsi = lua_tonumber(L, 2); 
					break;
		case 702 :	iDest = lua_tonumber(L, 1);
					if ( iDest == 0 ) 
						t.charanimstate.originale = lua_tonumber(L, 2); 
					else
						t.charanimstates[iDest].originale = lua_tonumber(L, 2); 
					break;
		case 703 :	iDest = lua_tonumber(L, 1);
					if ( iDest == 0 ) 
						t.charanimstate.obj = lua_tonumber(L, 2); 
					else
						t.charanimstates[iDest].obj = lua_tonumber(L, 2); 
					break;
		case 704 :	iDest = lua_tonumber(L, 1);
					if ( iDest == 0 ) 
						t.charanimstate.animationspeed_f = lua_tonumber(L, 2); 
					else
						t.charanimstates[iDest].animationspeed_f = lua_tonumber(L, 2); 
					break;
		case 705 :	iDest = lua_tonumber(L, 1);
					if ( iDest == 0 ) 
						t.charanimstate.e = lua_tonumber(L, 2); 
					else
						t.charanimstates[iDest].e = lua_tonumber(L, 2); 
					break;

		case 801 : t.charanimcontrols[lua_tonumber(L, 1)].leaping = lua_tonumber(L, 2); 
			break;
		case 802 : t.charanimcontrols[lua_tonumber(L, 1)].moving = lua_tonumber(L, 2); 
			break;

		case 1001 : break;
		case 1002 : break;
	}
	return 0;
}
int GetGamePlayerControlData ( lua_State *L, int iDataMode )
{
	lua2 = L;
	int iSrc = 0;
	int n = lua_gettop(L);
	if ( iDataMode >= 500 )
	{
		if ( iDataMode >= 1001 )
			if ( n < 2 ) return 0;
		else
			if ( n < 1 ) return 0;
	}
	int gunId = t.gunid;
	int fireMode = t.tfiremode;

	if ( n > 0 && iDataMode > 200 && iDataMode < 500 )
	{
		gunId = lua_tonumber(L, 1);
		if (n > 1)
		{
			fireMode = lua_tonumber(L, 2);
		}
		else
		{
			fireMode = 0;
		}
	}

	switch ( iDataMode )
	{
		case 1 : lua_pushnumber ( L, t.playercontrol.jetpackmode ); break;
		case 2 : lua_pushnumber ( L, t.playercontrol.jetpackfuel_f ); break;
		case 3 : lua_pushnumber ( L, t.playercontrol.jetpackhidden ); break;
		case 4 : lua_pushnumber ( L, t.playercontrol.jetpackcollected ); break;
		case 5 : lua_pushnumber ( L, t.playercontrol.soundstartindex ); break;
		case 6 : lua_pushnumber ( L, t.playercontrol.jetpackparticleemitterindex ); break;
		case 7 : lua_pushnumber ( L, t.playercontrol.jetpackthrust_f ); break;
		case 8 : lua_pushnumber ( L, t.playercontrol.startstrength ); break;
		case 9 : lua_pushnumber ( L, t.playercontrol.isrunning ); break;
		case 10 : break;
		case 11 : lua_pushnumber ( L, t.playercontrol.cx_f ); break;
		case 12 : lua_pushnumber ( L, t.playercontrol.cy_f ); break;
		case 13 : lua_pushnumber ( L, t.playercontrol.cz_f ); break;
		case 14 : lua_pushnumber ( L, t.playercontrol.basespeed_f ); break;
		case 15 : lua_pushnumber ( L, t.playercontrol.canrun ); break;
		case 16 : lua_pushnumber ( L, t.playercontrol.maxspeed_f ); break;
		case 17 : lua_pushnumber ( L, t.playercontrol.topspeed_f ); break;
		case 18 : lua_pushnumber ( L, t.playercontrol.movement ); break;
		case 19 : lua_pushnumber ( L, t.playercontrol.movey_f ); break;
		case 20 : lua_pushnumber ( L, t.playercontrol.lastmovement ); break;
		case 21 : lua_pushnumber ( L, t.playercontrol.footfallcount ); break;
		case 22 : break;
		case 23 : lua_pushnumber ( L, t.playercontrol.gravityactive ); break;
		case 24 : lua_pushnumber ( L, t.playercontrol.plrhitfloormaterial ); break;
		case 25:  lua_pushnumber(L, t.playercontrol.underwater); break;
		case 26 : lua_pushnumber ( L, t.playercontrol.jumpmode ); break;
		case 27 : lua_pushnumber ( L, t.playercontrol.jumpmodecanaffectvelocitycountdown_f ); break;
		case 28 : lua_pushnumber ( L, t.playercontrol.speed_f ); break;
		case 29 : lua_pushnumber ( L, t.playercontrol.accel_f ); break;
		case 30 : lua_pushnumber ( L, t.playercontrol.speedratio_f ); break;
		case 31 : lua_pushnumber ( L, t.playercontrol.wobble_f ); break;
		case 32 :
		{
#ifdef FASTBULLETPHYSICS
			lua_pushnumber(L, t.playercontrol.wobblespeed_f*0.5);
#else
			extern bool bPhysicsRunningAt120FPS;
			if (!bPhysicsRunningAt120FPS)
				lua_pushnumber(L, t.playercontrol.wobblespeed_f*0.5);
			else
				lua_pushnumber(L, t.playercontrol.wobblespeed_f);
#endif
			break;
		}
		case 33 : lua_pushnumber ( L, t.playercontrol.wobbleheight_f ); break;
		case 34 : lua_pushnumber ( L, t.playercontrol.jumpmax_f ); break;
		case 35 : lua_pushnumber ( L, t.playercontrol.pushangle_f ); break;
		case 36 : lua_pushnumber ( L, t.playercontrol.pushforce_f ); break;
		case 37 : lua_pushnumber ( L, t.playercontrol.footfallpace_f ); break;
		case 38 : lua_pushnumber ( L, t.playercontrol.lockatheight ); break;
		case 39 : lua_pushnumber ( L, t.playercontrol.controlheight ); break;
		case 40 : lua_pushnumber ( L, t.playercontrol.controlheightcooldown ); break;
		case 41 : lua_pushnumber ( L, t.playercontrol.storemovey ); break;
		case 42 : break;
		case 43 : lua_pushnumber ( L, t.playercontrol.hurtfall ); break;
		case 44 : lua_pushnumber ( L, t.playercontrol.leanoverangle_f ); break;
		case 45 : lua_pushnumber ( L, t.playercontrol.leanover_f ); break;
		case 46 : lua_pushnumber ( L, t.playercontrol.camerashake_f ); break;
		case 47 : lua_pushnumber ( L, t.playercontrol.finalcameraanglex_f ); break;
		case 48 : lua_pushnumber ( L, t.playercontrol.finalcameraangley_f ); break;
		case 49 : lua_pushnumber ( L, t.playercontrol.finalcameraanglez_f ); break;
		case 50 : lua_pushnumber ( L, t.playercontrol.camrightmousemode ); break;
		case 51 : lua_pushnumber ( L, t.playercontrol.camcollisionsmooth ); break;
		case 52 : lua_pushnumber ( L, t.playercontrol.camcurrentdistance ); break;
		case 53 : lua_pushnumber ( L, t.playercontrol.camdofullraycheck ); break;
		case 54 : lua_pushnumber ( L, t.playercontrol.lastgoodcx_f ); break;
		case 55 : lua_pushnumber ( L, t.playercontrol.lastgoodcy_f ); break;
		case 56 : lua_pushnumber ( L, t.playercontrol.lastgoodcz_f ); break;
		case 57 : break;
		case 58 : lua_pushnumber ( L, t.playercontrol.flinchx_f ); break;
		case 59 : lua_pushnumber ( L, t.playercontrol.flinchy_f ); break;
		case 60 : lua_pushnumber ( L, t.playercontrol.flinchz_f ); break;
		case 61 : lua_pushnumber ( L, t.playercontrol.flinchcurrentx_f ); break;
		case 62 : lua_pushnumber ( L, t.playercontrol.flinchcurrenty_f ); break;
		case 63 : lua_pushnumber ( L, t.playercontrol.flinchcurrentz_f ); break;
		case 64 : lua_pushnumber ( L, t.playercontrol.footfalltype ); break;
		case 65 : lua_pushnumber ( L, t.playercontrol.ripplecount_f ); break;
		case 66 : lua_pushnumber ( L, t.playercontrol.lastfootfallsound ); break;
		case 67 : lua_pushnumber ( L, t.playercontrol.inwaterstate ); break;
		case 68 : lua_pushnumber ( L, t.playercontrol.drowntimestamp ); break;
		case 69 : lua_pushnumber ( L, t.playercontrol.deadtime ); break;
		case 70 : lua_pushnumber ( L, t.playercontrol.swimtimestamp ); break;
		case 71 : lua_pushnumber ( L, t.playercontrol.redDeathFog_f ); break;
		case 72 : 
			#ifdef WICKEDENGINE
			if (t.playercontrol.iPlayHeartBeatSoundOff == 1)
				lua_pushnumber (L, -1);
			else
				lua_pushnumber (L, t.playercontrol.heartbeatTimeStamp);
			#else
			lua_pushnumber (L, t.playercontrol.heartbeatTimeStamp);
			#endif
			break;
		case 81 : lua_pushnumber ( L, t.playercontrol.thirdperson.enabled ); break;
		case 82 : lua_pushnumber ( L, t.playercontrol.thirdperson.characterindex ); break;
		case 83 : lua_pushnumber ( L, t.playercontrol.thirdperson.camerafollow ); break;
		case 84 : lua_pushnumber ( L, t.playercontrol.thirdperson.camerafocus ); break;
		case 85 : lua_pushnumber ( L, t.playercontrol.thirdperson.charactere ); break;
		case 86 : break;
		case 87 : lua_pushnumber ( L, t.playercontrol.thirdperson.shotfired ); break;
		case 88 : lua_pushnumber ( L, t.playercontrol.thirdperson.cameradistance ); break;
		case 89 : lua_pushnumber ( L, t.playercontrol.thirdperson.cameraspeed ); break;
		case 90 : lua_pushnumber ( L, t.playercontrol.thirdperson.cameralocked ); break;
		case 91 : lua_pushnumber ( L, t.playercontrol.thirdperson.cameraheight ); break;
		case 92 : lua_pushnumber ( L, t.playercontrol.thirdperson.camerashoulder ); break;
		
		case 95 : lua_pushnumber(L, t.playercontrol.fFallDamageMultiplier); break;
		case 96: lua_pushnumber(L, t.playercontrol.fSwimSpeed); break;

		case 99 : lua_pushnumber ( L, g.gxboxcontrollertype ); break;		
		case 101 : lua_pushnumber ( L, t.gunmode ); break;
		case 102 : lua_pushnumber ( L, t.player[t.plrid].state.firingmode ); break;
		case 103 : lua_pushnumber ( L, g.weaponammoindex ); break;
		case 104 : lua_pushnumber ( L, g.ammooffset ); break;
		case 105 : lua_pushnumber ( L, g.ggunmeleekey ); break;
		case 106 : lua_pushnumber ( L, t.player[t.plrid].state.blockingaction ); break;
		case 107 : lua_pushnumber ( L, t.gunshootnoammo ); break;		

		case 108 :
			//PE: Now controlled in lua. old (lua_pushnumber(L, g.playerunderwater); )
			if(t.playercontrol.inwaterstate >= 2)
				lua_pushnumber ( L, 1 );
			else
				lua_pushnumber(L, 0);
			break;

		case 109 : lua_pushnumber ( L, g.gdisablerightmousehold ); break;		
		case 110 : lua_pushnumber ( L, g.gxbox ); break;		
		case 111 : lua_pushnumber ( L, JoystickX() ); break;
		case 112 : lua_pushnumber ( L, JoystickY() ); break;
		case 113 : lua_pushnumber ( L, JoystickZ() ); break;
		case 114 : lua_pushnumber ( L, t.gunzoommode ); break;		
		case 115 : lua_pushnumber ( L, t.gunzoommag_f ); break;		
		case 116 : lua_pushnumber ( L, t.gunreloadnoammo ); break;	
		case 117 : lua_pushnumber ( L, g.plrreloading ); break;	
		case 118 : lua_pushnumber ( L, g.ggunaltswapkey1 ); break;	
		case 119 : lua_pushnumber ( L, g.ggunaltswapkey2 ); break;	
		case 120 : lua_pushnumber ( L, t.weaponkeyselection ); break;	
		case 121 : lua_pushnumber ( L, t.weaponindex ); break;	
		case 122 : lua_pushnumber ( L, t.player[t.plrid].command.newweapon ); break;	
		case 123 : lua_pushnumber ( L, t.gunid ); break;	
		case 124 : lua_pushnumber ( L, t.gunselectionafterhide ); break;	
		case 125 : lua_pushnumber ( L, t.gunburst ); break;	
		case 126 : break;
		case 127 : lua_pushnumber ( L, JoystickTwistX() ); break;
		case 128 : lua_pushnumber ( L, JoystickTwistY() ); break;
		case 129 : lua_pushnumber ( L, JoystickTwistZ() ); break;
		case 130 : lua_pushnumber ( L, t.plrzoominchange ); break;	
		case 131 : lua_pushnumber ( L, t.plrzoomin_f ); break;	
		case 132 : lua_pushnumber ( L, g.luaactivatemouse ); break;	
		case 133 : lua_pushnumber ( L, g.realfov_f ); break;	
		case 134 : lua_pushnumber ( L, g.gdisablepeeking ); break;	
		case 135 : lua_pushnumber ( L, t.plrhasfocus ); break;	
		case 136 : lua_pushnumber ( L, t.game.runasmultiplayer ); break;	
		case 137 : lua_pushnumber ( L, g.mp.respawnLeft ); break;	
		case 138 : lua_pushnumber ( L, g.tabmode ); break;	
		case 139 : lua_pushnumber ( L, g.lowfpswarning ); break;	
		case 140 : lua_pushnumber ( L, t.visuals.CameraFOV_f ); break;	
		case 141 : lua_pushnumber ( L, t.visuals.CameraFOVZoomed_f ); break;	
		case 142 : lua_pushnumber ( L, g.gminvert ); break;	
		case 143 : lua_pushnumber ( L, t.plrkeySLOWMOTION ); break;	
		case 144 : lua_pushnumber ( L, g.globals.smoothcamerakeys ); break;	
		case 145 : lua_pushnumber ( L, t.cammousemovex_f ); break;	
		case 146 : lua_pushnumber ( L, t.cammousemovey_f ); break;	
		case 147 : lua_pushnumber ( L, g.gunRecoilX_f ); break;	
		case 148 : lua_pushnumber ( L, g.gunRecoilY_f ); break;	
		case 149 : lua_pushnumber ( L, g.gunRecoilAngleX_f ); break;	
		case 150 : lua_pushnumber ( L, g.gunRecoilAngleY_f ); break;	
		case 151 : lua_pushnumber ( L, t.gunRecoilCorrectY_f ); break;	
		case 152 : lua_pushnumber ( L, g.gunRecoilCorrectX_f ); break;	
		case 153 : lua_pushnumber ( L, g.gunRecoilCorrectAngleY_f ); break;	
		case 154 : lua_pushnumber ( L, t.gunRecoilCorrectAngleX_f ); break;	
		case 155 : lua_pushnumber ( L, t.camangx_f ); break;	
		case 156 : lua_pushnumber ( L, t.camangy_f ); break;	
		case 157 : lua_pushnumber ( L, t.aisystem.playerducking ); break;	
		case 158 : lua_pushnumber ( L, t.conkit.editmodeactive ); break;	
		case 159 : lua_pushnumber ( L, t.plrkeySHIFT ); break;	
		case 160 : lua_pushnumber ( L, t.plrkeySHIFT2 ); break;	
		case 161 : lua_pushnumber ( L, t.inputsys.keycontrol ); break;	
		case 162 : lua_pushnumber ( L, t.hardwareinfoglobals.nowater ); break;	
		case 163 : lua_pushnumber ( L, t.terrain.waterliney_f ); break;	
		case 164 : lua_pushnumber ( L, g.flashLightKeyEnabled ); break;	
		case 165 : lua_pushnumber ( L, t.playerlight.flashlightcontrol_f ); break;	
		case 166 : lua_pushnumber ( L, t.player[t.plrid].state.moving ); break;	
		case 167 : lua_pushnumber ( L, t.tplayerterrainheight_f ); break;	
		case 168 : 
			lua_pushnumber ( L, t.tjetpackverticalmove_f );
			break;	
		case 169 : lua_pushnumber ( L, t.terrain.TerrainID ); break;	
		case 170 : lua_pushnumber ( L, g.globals.enableplrspeedmods ); break;	
		case 171 : lua_pushnumber ( L, g.globals.riftmode ); break;	
		case 172 : lua_pushnumber ( L, t.tplayerx_f ); break;	
		case 173 : lua_pushnumber ( L, t.tplayery_f ); break;	
		case 174 : lua_pushnumber ( L, t.tplayerz_f ); break;	
		case 175 : lua_pushnumber ( L, t.terrain.playerx_f ); break;	
		case 176 : lua_pushnumber ( L, t.terrain.playery_f ); break;	
		case 177 : lua_pushnumber ( L, t.terrain.playerz_f ); break;	
		case 178 : lua_pushnumber ( L, t.terrain.playerax_f ); break;	
		case 179 : lua_pushnumber ( L, t.terrain.playeray_f ); break;	
		case 180 : lua_pushnumber ( L, t.terrain.playeraz_f ); break;	
		case 181 : lua_pushnumber ( L, t.tadjustbasedonwobbley_f ); break;	
		case 182 : lua_pushnumber ( L, t.tFinalCamX_f ); break;	
		case 183 : lua_pushnumber ( L, t.tFinalCamY_f ); break;	
		case 184 : lua_pushnumber ( L, t.tFinalCamZ_f ); break;	
		case 185 : lua_pushnumber ( L, t.tshakex_f ); break;	
		case 186 : lua_pushnumber ( L, t.tshakey_f); break;	
		case 187 : lua_pushnumber ( L, t.tshakez_f ); break;	
		case 188 : lua_pushnumber ( L, t.huddamage.immunity ); break;	
		case 189 : lua_pushnumber ( L, g.charanimindex ); break;				

		#ifdef VRTECH
		case 190 : if ( GGVR_IsHmdPresent() > 0 ) { lua_pushnumber ( L, 1 ); } else { lua_pushnumber ( L, 0 ); } break;
		case 191 : lua_pushnumber ( L, GGVR_IsHmdPresent() ); break;				
		case 192 : lua_pushnumber ( L, GGVR_BestController_JoyX() ); break;
		case 193 : lua_pushnumber ( L, GGVR_BestController_JoyY() ); break;
		case 194 : lua_pushnumber ( L, GGVR_RightController_Trigger() ); break;
		case 195 : lua_pushnumber ( L, GGVR_RightController_Grip() ); break;
		case 196 : lua_pushnumber ( L, GGVR_RightController_JoyX() ); break;
		case 197: lua_pushnumber (L, GGVR_RightController_JoyY()); break;
		case 198: lua_pushnumber (L, GGVR_RightController_Button1()); break;
		case 199: lua_pushnumber (L, GGVR_RightController_Button2()); break;
		case 251 : lua_pushnumber ( L, GGVR_GetBestHandX() ); break;
		case 252 : lua_pushnumber ( L, GGVR_GetBestHandY() ); break;
		case 253 : lua_pushnumber ( L, GGVR_GetBestHandZ() ); break;
		case 254 : lua_pushnumber ( L, GGVR_GetBestHandAngleX() ); break;
		case 255 : lua_pushnumber ( L, GGVR_GetBestHandAngleY() ); break;
		case 256 : lua_pushnumber ( L, GGVR_GetBestHandAngleZ() ); break;
		case 257: lua_pushnumber (L, GGVR_GetLaserGuidedEntityObj(g.entityviewstartobj, g.entityviewendobj)); break;
		#else
		case 190 : 
		case 191 : 
		case 192 : 
		case 193 : 
		case 194 : 
		case 195 : 
		case 196 : 
		case 197 : 
		case 198 : 
		case 199 : 
		case 200 : 
		case 251 : 
		case 252 : 
		case 253 : 
		case 254 : 
		case 255 : 
		case 256 : 
		case 257 : 
			lua_pushnumber ( L, 0 ); 
			break;
		#endif

		case 201: lua_pushnumber (L, t.gun[gunId].settings.ismelee); break;
		case 202 : lua_pushnumber ( L, t.gun[gunId].settings.alternate       ); break;
		case 203 : lua_pushnumber ( L, t.gun[gunId].settings.modessharemags  ); break;
		case 204 : lua_pushnumber ( L, t.gun[gunId].settings.alternateisflak ); break;
		case 205 : lua_pushnumber ( L, t.gun[gunId].settings.alternateisray  ); break;
		
		// 251-260 used above

		case 301 : lua_pushnumber ( L, g.firemodes[gunId][fireMode].settings.reloadqty         ); break;
		case 302 : lua_pushnumber ( L, g.firemodes[gunId][fireMode].settings.isempty           ); break;
		case 303 : lua_pushnumber ( L, g.firemodes[gunId][fireMode].settings.jammed            ); break;
		case 304 : lua_pushnumber ( L, g.firemodes[gunId][fireMode].settings.jamchance         ); break;
		case 305 : lua_pushnumber ( L, g.firemodes[gunId][fireMode].settings.mintimer          ); break;
		case 306 : lua_pushnumber ( L, g.firemodes[gunId][fireMode].settings.addtimer          ); break;
		case 307 : lua_pushnumber ( L, g.firemodes[gunId][fireMode].settings.shotsfired        ); break;
		case 308 : lua_pushnumber ( L, g.firemodes[gunId][fireMode].settings.cooltimer         ); break;
		case 309 : lua_pushnumber ( L, g.firemodes[gunId][fireMode].settings.overheatafter     ); break;
		case 310 : lua_pushnumber ( L, g.firemodes[gunId][fireMode].settings.jamchancetime     ); break;
		case 311 : lua_pushnumber ( L, g.firemodes[gunId][fireMode].settings.cooldown          ); break;
		case 312 : lua_pushnumber ( L, g.firemodes[gunId][fireMode].settings.nosubmergedfire   ); break;
		case 313 : lua_pushnumber ( L, g.firemodes[gunId][fireMode].settings.simplezoom        ); break;
		case 314 : lua_pushnumber ( L, g.firemodes[gunId][fireMode].settings.forcezoomout      ); break;
		case 315 : lua_pushnumber ( L, g.firemodes[gunId][fireMode].settings.zoommode          ); break;
		case 316 : lua_pushnumber ( L, g.firemodes[gunId][fireMode].settings.simplezoomanim    ); break;
		case 317 : lua_pushnumber ( L, g.firemodes[gunId][fireMode].settings.poolindex         ); break;
		case 318 : lua_pushnumber ( L, g.firemodes[gunId][fireMode].settings.plrturnspeedmod   ); break;
		case 319 : lua_pushnumber ( L, g.firemodes[gunId][fireMode].settings.zoomturnspeed     ); break;
		case 320 : lua_pushnumber ( L, g.firemodes[gunId][fireMode].settings.plrjumpspeedmod   ); break;
		case 321 : lua_pushnumber ( L, g.firemodes[gunId][fireMode].settings.plremptyspeedmod  ); break;
		case 322 : lua_pushnumber ( L, g.firemodes[gunId][fireMode].settings.plrmovespeedmod   ); break;
		case 323 : lua_pushnumber ( L, g.firemodes[gunId][fireMode].settings.zoomwalkspeed     ); break;
		case 324 : lua_pushnumber ( L, g.firemodes[gunId][fireMode].settings.plrreloadspeedmod ); break;
		case 325 : lua_pushnumber ( L, g.firemodes[gunId][fireMode].settings.hasempty          ); break;
		case 326 : lua_pushnumber ( L, g.firemodes[gunId][fireMode].action.block.s			   ); break;
		case 327 : lua_pushnumber ( L, g.firemodes[gunId][fireMode].settings.meleewithrightclick ); break;
		case 328: lua_pushnumber (L, g.firemodes[gunId][fireMode].settings.blockwithrightclick); break;

		// for extra commands as yet unimagined :)
		case 401: lua_pushnumber (L, t.playerlight.flashlightcontrol_range_f); break;
		case 402: lua_pushnumber (L, t.playerlight.flashlightcontrol_radius_f); break;
		case 403: lua_pushnumber (L, t.playerlight.flashlightcontrol_colorR_f); break;
		case 404: lua_pushnumber (L, t.playerlight.flashlightcontrol_colorG_f); break;
		case 405: lua_pushnumber (L, t.playerlight.flashlightcontrol_colorB_f); break;
		case 406: lua_pushnumber (L, t.playerlight.flashlightcontrol_cashshadow); break;

		case 501 : lua_pushnumber ( L, t.gunsound[t.gunid][lua_tonumber(L, 1)].soundid1 ); break;
		case 502 : lua_pushnumber ( L, t.gunsound[t.gunid][lua_tonumber(L, 1)].altsoundid ); break;		
		case 503 : lua_pushnumber ( L, JoystickHatAngle(lua_tonumber(L, 1)) ); break;
		case 504 : lua_pushnumber ( L, JoystickFireXL(lua_tonumber(L, 1)) ); break;

		case 601: lua_pushnumber (L, t.player[t.plrid].state.counteredaction); break;
			
		case 701 :	iSrc = lua_tonumber(L, 1);
					if ( iSrc == 0 )
						lua_pushnumber ( L, t.charanimstate.playcsi ); 
					else
						lua_pushnumber ( L, t.charanimstates[iSrc].playcsi ); 
					break;
		case 702 :	iSrc = lua_tonumber(L, 1);
					if ( iSrc == 0 )
						lua_pushnumber ( L, t.charanimstate.originale ); 
					else
						lua_pushnumber ( L, t.charanimstates[iSrc].originale ); 
					break;
		case 703 :	iSrc = lua_tonumber(L, 1);
					if ( iSrc == 0 )
						lua_pushnumber ( L, t.charanimstate.obj ); 
					else
						lua_pushnumber ( L, t.charanimstates[iSrc].obj ); 
					break;
		case 704 :	iSrc = lua_tonumber(L, 1);
					if ( iSrc == 0 )
						lua_pushnumber ( L, t.charanimstate.animationspeed_f ); 
					else
						lua_pushnumber ( L, t.charanimstates[iSrc].animationspeed_f ); 
					break;
		case 705 :	iSrc = lua_tonumber(L, 1);
					if ( iSrc == 0 )
						lua_pushnumber ( L, t.charanimstate.e ); 
					else
						lua_pushnumber ( L, t.charanimstates[iSrc].e ); 
					break;

		case 741 : lua_pushnumber ( L, t.csi_stoodvault[lua_tonumber(L, 1)] ); break;
		case 751 : lua_pushnumber ( L, t.charseq[lua_tonumber(L, 1)].trigger ); break;
		case 761 : lua_pushnumber ( L, t.entityelement[lua_tonumber(L, 1)].bankindex ); break;
		case 762 : lua_pushnumber ( L, t.entityelement[lua_tonumber(L, 1)].obj ); break;
		case 763 : lua_pushnumber ( L, t.entityelement[lua_tonumber(L, 1)].ragdollified ); break;
		case 764 : lua_pushnumber ( L, t.entityelement[lua_tonumber(L, 1)].speedmodulator_f ); break;
		case 801 : lua_pushnumber ( L, t.charanimcontrols[lua_tonumber(L, 1)].leaping ); break;
		case 802 : lua_pushnumber ( L, t.charanimcontrols[lua_tonumber(L, 1)].moving ); break;
		case 851 : lua_pushnumber ( L, t.entityprofile[lua_tonumber(L, 1)].fJumpModifier ); break;
		case 852 : lua_pushnumber ( L, t.entityprofile[lua_tonumber(L, 1)].startofaianim ); break;			
		case 853 : lua_pushnumber ( L, t.entityprofile[lua_tonumber(L, 1)].jumphold ); break;			
		case 854 : lua_pushnumber ( L, t.entityprofile[lua_tonumber(L, 1)].jumpresume ); break;			

		case 1001 : lua_pushnumber ( L, t.entityanim[lua_tonumber(L, 1)][lua_tonumber(L, 2)].start ); break;
		case 1002 : lua_pushnumber ( L, t.entityanim[lua_tonumber(L, 1)][lua_tonumber(L, 2)].finish ); break;
	}
	return 1;
}
int GetPlayerFov (lua_State* L)
{
	// to match the SetPlayerFOV command - or you could have used 'g_PlayerFOV'
	int iPlayerFOVPerc = (((t.visuals.CameraFOV_f * t.visuals.CameraASPECT_f) - 20.0) / 180.0) * 114.0f;// 100.0;
	lua_pushnumber (L, iPlayerFOVPerc);
	return 1;
}
int GetPlayerAttacking (lua_State* L)
{
	int iPlayerIsAttackingNow = 0;
	if (t.gunmode >= 101 && t.gunmode < 110) iPlayerIsAttackingNow = 1;
	if(t.gunmode >= 1020 && t.gunmode < 1029 ) iPlayerIsAttackingNow = 1;
	lua_pushnumber (L, iPlayerIsAttackingNow);
	return 1;
}
int PushPlayer (lua_State* L)
{
	lua2 = L;
	int n = lua_gettop(L);
	if (n < 1) return 0;
	// trigger short or long player arms animation (jerked back; typically when counter attacked)
	t.gunmode = 1011;
	return 0;
}

