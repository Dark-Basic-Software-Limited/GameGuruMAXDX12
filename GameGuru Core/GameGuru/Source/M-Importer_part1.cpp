void animsystem_loadanimtextfile (sObject* pObject, cstr pAbsPathToAnim, char* cFileSelected)
{
	// anim file or anim list file
	struct sAnimFileToAppend
	{
		cstr file;
		int iStep1;
		int iStep2;
		int iStep3;
	};
	char pTheFile[MAX_PATH];
	std::vector<sAnimFileToAppend> filestoappend;
	filestoappend.clear();
	if (strnicmp(cFileSelected + strlen(cFileSelected) - 4, ".dbo", 4) == NULL)
	{
		// animation file chosen
		sAnimFileToAppend item;
		item.file = cFileSelected;
		item.iStep1 = 0;
		item.iStep2 = 0;
		item.iStep3 = 0;
		filestoappend.push_back(item);
	}
	else if (strnicmp(cFileSelected + strlen(cFileSelected) - 4, ".txt", 4) == NULL || strnicmp(cFileSelected + strlen(cFileSelected) - 4, ".dat", 4) == NULL)
	{
		// special case, if this file chosen, we want to replace ALL models with mannequin and store in writables for testing (and eventual use)
		LPSTR pProcessFileTrigger = "processallmannequins.txt";
		if (strnicmp(cFileSelected + strlen(cFileSelected) - strlen(pProcessFileTrigger), pProcessFileTrigger, strlen(pProcessFileTrigger)) == NULL )
		{
			// replace all DBOs in "charactercreatorplus\animations" with mannequin model (exclude non-character animations)
			animsystem_processallmannequins(); // GG_SetWritablesToRoot(true); handled inside

			return;
		}

		// anim list file (text file containing many references to animation files)
		OpenToRead(1, cFileSelected);
		while (FileEnd(1) == 0)
		{
			int iThisStep1 = 0;
			int iThisStep2 = 0;
			int iThisStep3 = 0;
			int iStage = 0;
			LPSTR pLine = ReadString (1);
			char pLineChomp[MAX_PATH];
			memset(pLineChomp, 0, sizeof(pLineChomp));
			strcpy (pLineChomp, pLine);
			if (strlen(pLineChomp) > 0)
			{
				while (strlen(pLineChomp) > 0)
				{
					char pThisBit[MAX_PATH];
					memset(pThisBit, 0, sizeof(pThisBit));
					strcpy (pThisBit, pLineChomp);
					LPSTR pStepValue = strstr (pThisBit, ";");
					if (pStepValue)
					{
						*pStepValue = 0;
						strcpy (pLineChomp, pStepValue + 1);
					}
					else
					{
						strcpy (pLineChomp, "");
					}
					if (iStage == 0) sprintf(pTheFile, "%s%s.dbo", pAbsPathToAnim.Get(), pThisBit);
					if (iStage == 1) iThisStep1 = atoi(pThisBit);
					if (iStage == 2) iThisStep2 = atoi(pThisBit);
					if (iStage == 3) iThisStep3 = atoi(pThisBit);
					iStage++;
				}
				sAnimFileToAppend item;
				item.file = pTheFile;
				item.iStep1 = iThisStep1;
				item.iStep2 = iThisStep2;
				item.iStep3 = iThisStep3;
				filestoappend.push_back(item);
			}
		}
		CloseFile(1);
	}
	else
	{
		// single animation file chosen (not DBO or TXT/DAT list)
		sAnimFileToAppend item;
		item.file = cFileSelected;
		item.iStep1 = 0;
		item.iStep2 = 0;
		item.iStep3 = 0;
		filestoappend.push_back(item);
	}

	// append all animation files specified in list
	for (int l = 0; l < filestoappend.size(); l++)
	{
		// this anim
		strcpy (pTheFile, filestoappend[l].file.Get());
		pAbsPathToAnim = pTheFile;

		// extract name from file
		char pAppendThisAnimFile[MAX_PATH];
		strcpy(pAppendThisAnimFile, "");
		for (int n = strlen(pTheFile); n > 0; n--)
		{
			if (pTheFile[n] == '\\' || pTheFile[n] == '/')
			{
				strcpy(pAppendThisAnimFile, pTheFile + n + 1);
				if (strnicmp(pAppendThisAnimFile + strlen(pAppendThisAnimFile) - 4, ".dbo", 4) == NULL)
				{
					pAppendThisAnimFile[strlen(pAppendThisAnimFile) - 4] = 0;
				}
				break;
			}
		}

		// keep non-combat animations from base model
		int iFrameToAppendFrom = pObject->fAnimTotalFrames;
		if (iFrameToAppendFrom > 0)
		{
			// after initial animation, should not overwrite last frame!
			iFrameToAppendFrom++;
		}

		// append animations to character
		if (AppendAnimationFromFile(pObject, pAbsPathToAnim.Get(), iFrameToAppendFrom) == false)
			break;

		// Create animations (from animation data stored in DBO)
		WickedCall_RefreshObjectAnimations(pObject, pObject->wickedloaderstateptr);

		// if successful, add to end of list
		sAnimSlotStruct animslotitem;
		animslotitem.fStep1 = 0;
		animslotitem.fStep2 = 0;
		animslotitem.fStep3 = 0;
		if (strlen(pAppendThisAnimFile) >= 32)
		{
			memcpy(animslotitem.pName, pAppendThisAnimFile, 31);
			animslotitem.pName[31] = 0;
		}
		else
		{
			strcpy(animslotitem.pName, pAppendThisAnimFile);
		}
		animslotitem.fStart = iFrameToAppendFrom;//LB: this caused a world of pain +1;
		animslotitem.fFinish = pObject->fAnimTotalFrames;
		if (filestoappend[l].iStep1 > 0) animslotitem.fStep1 = iFrameToAppendFrom + filestoappend[l].iStep1;
		if (filestoappend[l].iStep2 > 0) animslotitem.fStep2 = iFrameToAppendFrom + filestoappend[l].iStep2;
		if (filestoappend[l].iStep3 > 0) animslotitem.fStep3 = iFrameToAppendFrom + filestoappend[l].iStep3;
		animslotitem.bLooped = true;
		g_pAnimSlotList.push_back(animslotitem);

		// also update main anim count (first slot always shows all frames)
		g_pAnimSlotList[0].fFinish = pObject->fAnimTotalFrames;
	}
}

void animsysten_clearset(sObject* pObject)
{
	// stop object anim, remove wicked anim components and clear all animation from object
	animsystem_clearoldanimationfromobject(pObject);

	// clear slot list
	g_iCurrentAnimationSlotIndex = 0;
	g_pAnimSlotList.clear();

	// and create new blank All slot
	// if successful, add to end of list
	sAnimSlotStruct animslotitem;
	animslotitem.fStep1 = 0;
	animslotitem.fStep2 = 0;
	animslotitem.fStep3 = 0;
	strcpy(animslotitem.pName, "All");
	animslotitem.fStart = 0;
	animslotitem.fFinish = 0;
	animslotitem.bLooped = true;
	g_pAnimSlotList.push_back(animslotitem);
}

void animsystem_animationtoolui(int objectnumber)
{
	extern int iLastOpenHeader;
	if (pref.bAutoClosePropertySections && iLastOpenHeader != 70)
		ImGui::SetNextItemOpen(false, ImGuiCond_Always);

	// If the last open header was the animation tool simple ui(71) then the user toggled advanced settings so this header should be opened.
	if (pref.bAutoClosePropertySections && iLastOpenHeader == 71)
		ImGui::SetNextItemOpen(true, ImGuiCond_Always);

	if (ImGui::StyleCollapsingHeader("Animation Tool", ImGuiTreeNodeFlags_DefaultOpen))
	{
		iLastOpenHeader = 70;

		// standard numeric box to hold 99999 value
		static int iNumericWidth = 30;
		iNumericWidth = 30;
		// object being edited with animation tool
		sObject* pObject = GetObjectData(objectnumber);

		// if have animations
		ImGui::Indent(10);
		if (bFoundanimSet == NULL)
		{
			ImGui::TextCenter("No Animations Found");
		}
		else
		{
			// prep of animation tool component
			ImGui::PushItemWidth(-10);

			// animation preview
			if (ImGui::Checkbox("Animate Preview", &g_bAnimatingObjectPreview))
			{
				g_bUpdateAnimationPreview = true;
			}
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Animate Object Preview");

			// animation preview
			if (ImGui::Checkbox("Show Bones", &g_bShowBones))
			{
				wiRenderer::SetToDrawDebugBoneLines(g_bShowBones);
			}
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Show debug bones associated with object");
			if (g_bShowBones == true)
			{
				if (ImGui::Checkbox("Show Bones Info", &g_bShowBonesExtraInfo))
				{
					wiRenderer::SetToDrawDebugBoneLines(g_bShowBonesExtraInfo);
				}
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("List below all bones names representing this object");
			}

			extern void ControlAdvancedSetting(int&, const char*, bool* = 0);
			if (t.importer.importerActive == 0)
				ControlAdvancedSetting(pref.iEnableAdvancedCharacterCreator, "Advanced Character Creator Settings");

			// animation frame scrubber
			ImGui::TextCenter("Animation Frame");
			static float fAnimFrameStart;
			fAnimFrameStart = WickedCall_GetObjectFrame(pObject);
			if (ImGui::MaxSliderInputFloat("##iAnimFrame:", &fAnimFrameStart, 0, pObject->fAnimTotalFrames, "Shows the current animation frame", 0, pObject->fAnimTotalFrames, iNumericWidth))
			{
				SetObjectFrame(objectnumber, fAnimFrameStart);
				g_bAnimatingObjectPreview = false;
				g_bUpdateAnimationPreview = true;
			}

			// animation speed
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Set Animation Speed");
			ImGui::TextCenter("Animation Speed");
			static float fAnimSpeed;
			fAnimSpeed = pObject->fAnimSpeed * 100;
			if (ImGui::MaxSliderInputFloat("##iAnimSpeed:", &fAnimSpeed, 0.0f, 500.0f, "Set the speed of the overall animations", 0, 500.0f, iNumericWidth))
			{
				SetObjectSpeed(objectnumber, fAnimSpeed);
			}

			iNumericWidth = 45;

			// layout of slot list
			static int iColumnLeft = ImGui::GetCursorPos().x;
			static int iColumnLabelXName = iColumnLeft + 30;
			static int iColumnLabelXStart = iColumnLeft + 110;
			static int iColumnLabelXFinish = iColumnLeft + 165;
			static int iColumnLabelXStep1 = iColumnLeft + 220;
			static int iColumnLabelXStep2 = iColumnLeft + 275;
			static int iColumnLabelXStep3 = iColumnLeft + 330;
			static int iColumnLabelXDelete = iColumnLeft + 365;
			static int iNameEntryWidth = 70.0f;

			// animation slot titles
			ImGui::TextCenter("Animations");
			ImGui::SetCursorPos(ImVec2(iColumnLabelXName, ImGui::GetCursorPos().y));
			ImGui::Text("Name");
			ImGui::SameLine();
			ImGui::SetCursorPos(ImVec2(iColumnLabelXStart, ImGui::GetCursorPos().y));
			ImGui::Text("Start");
			ImGui::SameLine();
			ImGui::SetCursorPos(ImVec2(iColumnLabelXFinish, ImGui::GetCursorPos().y));
			ImGui::Text("Finish");
			ImGui::SameLine();
			ImGui::SetCursorPos(ImVec2(iColumnLabelXStep1, ImGui::GetCursorPos().y));
			ImGui::Text("Left");
			ImGui::SameLine();
			ImGui::SetCursorPos(ImVec2(iColumnLabelXStep2, ImGui::GetCursorPos().y));
			ImGui::Text("Right");
			ImGui::SameLine();
			ImGui::SetCursorPos(ImVec2(iColumnLabelXStep3, ImGui::GetCursorPos().y));
			ImGui::Text("Any");

			// list of animation slots
			int iDeleteAnimSlot = -1;
			for (int slot = 0; slot < (int)g_pAnimSlotList.size(); slot++)
			{
				// animation slot details
				sAnimSlotStruct* pAnimSlotItem = &(g_pAnimSlotList[slot]);

				// scrollable list of columns (radio, name, start, finish, delete)
				ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(1, 0));
				char pImGuiID[32];
				sprintf(pImGuiID, "##AnimSlotRadio%d", slot);
				if (ImGui::RadioButton(pImGuiID, &g_iCurrentAnimationSlotIndex, slot))
				{
					// change slot to preview
					g_bUpdateAnimationPreview = true;
				}
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("Select this animation slot to preview the animation");
				if (ImGui::MaxIsItemFocused()) bImGuiGotFocus = true;

				ImGui::SameLine();
				ImGui::PushItemWidth(iNameEntryWidth);
				sprintf(pImGuiID, "##AnimSlotName%d", slot);
				if (ImGui::InputText(pImGuiID, pAnimSlotItem->pName, 32))
				{
					// animation item name changed
				}
				if (ImGui::MaxIsItemFocused()) bImGuiGotFocus = true;
				if (!pref.iTurnOffEditboxTooltip && ImGui::IsItemHovered()) ImGui::SetTooltip(pAnimSlotItem->pName);
				ImGui::PopItemWidth();
				ImGui::SameLine();
				ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(1, 0));
				ImGui::PushItemWidth(iNumericWidth);
				int iAnimSlotItemStart = (int)pAnimSlotItem->fStart;
				sprintf(pImGuiID, "##AnimSlotStart%d", slot);
				if (ImGui::InputInt(pImGuiID, &iAnimSlotItemStart, 0, 9999)) g_bUpdateAnimationPreview = true;
				pAnimSlotItem->fStart = (float)iAnimSlotItemStart;
				if (!pref.iTurnOffEditboxTooltip && ImGui::IsItemHovered()) ImGui::SetTooltip("Change animation starting frame");
				ImGui::PopItemWidth();
				ImGui::SameLine();
				ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(1, 0));
				ImGui::PushItemWidth(iNumericWidth);
				int iAnimSlotItemFinish = (int)pAnimSlotItem->fFinish;
				sprintf(pImGuiID, "##AnimSlotFinish%d", slot);
				if (ImGui::InputInt(pImGuiID, &iAnimSlotItemFinish, 0, 9999)) g_bUpdateAnimationPreview = true;
				if (!pref.iTurnOffEditboxTooltip && ImGui::IsItemHovered()) ImGui::SetTooltip("Change animation finishing frame");
				pAnimSlotItem->fFinish = (float)iAnimSlotItemFinish;
				ImGui::PopItemWidth();
				// additional info for footfall frames
				ImGui::SameLine();
				ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(1, 0));
				ImGui::PushItemWidth(iNumericWidth);
				int iAnimSlotItemStep1 = (int)pAnimSlotItem->fStep1;
				sprintf(pImGuiID, "##AnimSlotStep1%d", slot);
				if (ImGui::InputInt(pImGuiID, &iAnimSlotItemStep1, 0, 9999)) g_bUpdateAnimationPreview = true;
				if (!pref.iTurnOffEditboxTooltip && ImGui::IsItemHovered()) ImGui::SetTooltip("XXX");
				pAnimSlotItem->fStep1 = (float)iAnimSlotItemStep1;
				ImGui::PopItemWidth();
				ImGui::SameLine();
				ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(1, 0));
				ImGui::PushItemWidth(iNumericWidth);
				int iAnimSlotItemStep2 = (int)pAnimSlotItem->fStep2;
				sprintf(pImGuiID, "##AnimSlotStep2%d", slot);
				if (ImGui::InputInt(pImGuiID, &iAnimSlotItemStep2, 0, 9999)) g_bUpdateAnimationPreview = true;
				if (!pref.iTurnOffEditboxTooltip && ImGui::IsItemHovered()) ImGui::SetTooltip("XXX");
				pAnimSlotItem->fStep2 = (float)iAnimSlotItemStep2;
				ImGui::PopItemWidth();
				ImGui::SameLine();
				ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(1, 0));
				ImGui::PushItemWidth(iNumericWidth);
				int iAnimSlotItemStep3 = (int)pAnimSlotItem->fStep3;
				sprintf(pImGuiID, "##AnimSlotStep3%d", slot);
				if (ImGui::InputInt(pImGuiID, &iAnimSlotItemStep3, 0, 9999)) g_bUpdateAnimationPreview = true;
				if (!pref.iTurnOffEditboxTooltip && ImGui::IsItemHovered()) ImGui::SetTooltip("XXX");
				pAnimSlotItem->fStep3 = (float)iAnimSlotItemStep3;
				ImGui::PopItemWidth();

				if (slot > 0)
				{
					ImGui::SameLine();
					ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(1, 0));
					ImGui::PushItemWidth(10);
					sprintf(pImGuiID, "X##AnimaionToolDeleteSlot%d", slot);
					if (ImGui::StyleButton(pImGuiID, ImVec2(0, 0)))
					{
						// flag to delete from g_pAnimSlotList after loop!
						iDeleteAnimSlot = slot;
						break;
					}
					if (ImGui::IsItemHovered()) ImGui::SetTooltip("Click to delete this animation slot");
					ImGui::PopItemWidth();
				}
			}

			// create and load buttons
			float w = ImGui::GetWindowContentRegionWidth();
			float but_gadget_size_animset = ImGui::GetFontSize()*15.0;
			ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((w*0.5) - (but_gadget_size_animset *0.5), 0.0f));
			if (ImGui::StyleButton("Create Animation Slot##AnimaionToolCreateNewSlot", ImVec2(but_gadget_size_animset, 0)))
			{
				sAnimSlotStruct animslotitem;
				animslotitem.fStep1 = 0;
				animslotitem.fStep2 = 0;
				animslotitem.fStep3 = 0;
				strcpy(animslotitem.pName, "new slot");
				animslotitem.fStart = 0.0f;
				animslotitem.fFinish = pObject->fAnimTotalFrames;
				animslotitem.bLooped = true;
				g_pAnimSlotList.push_back(animslotitem);
			}
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Click to create a new animation slot");

			ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((w*0.5) - (but_gadget_size_animset *0.5), 0.0f));
			if (ImGui::StyleButton("Load Animation File##AnimaionToolLoadAnimationFile", ImVec2(but_gadget_size_animset, 0)))
			{
				// file requester to select animation file
				cStr tOldDir = GetDir();
				cstr pAbsPathToAnim = g.fpscrootdir_s + "\\Files\\charactercreatorplus\\animations\\";
				char pTheFolder[MAX_PATH];
				strcpy (pTheFolder, pAbsPathToAnim.Get());
				char* cFileSelected = (char*)noc_file_dialog_open(NOC_FILE_DIALOG_OPEN, "All\0*.*\0", pTheFolder, NULL);
				SetDir(tOldDir.Get());
				if (cFileSelected && strlen(cFileSelected) > 0)
				{
					// handle DBO or TXT/DAT and append animations to imported model
					animsystem_loadanimtextfile (pObject, pAbsPathToAnim, cFileSelected);

					// switch to newly appended animation
					g_iCurrentAnimationSlotIndex = g_pAnimSlotList.size() - 1;
					g_bUpdateAnimationPreview = true;
				}
			}
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Click to append an animation from a file");

			// Need a way to wipe out any old animation data
			ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((w*0.5) - (but_gadget_size_animset *0.5), 0.0f));
			if (ImGui::StyleButton("Clear All Animation##AnimaionToolClearAll", ImVec2(but_gadget_size_animset, 0)))
			{
				// clears all animation data from object
				animsysten_clearset(pObject);
			}
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Click to clear all animation data");

			// developer mode can save template of animation slots created
			extern int g_iDevToolsOpen;
			if (g_iDevToolsOpen != 0 )
			{
				ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((w*0.5) - (but_gadget_size_animset *0.5), 0.0f));
				if (ImGui::StyleButton("Save Animation Template##AnimaionToolSaveTemplate", ImVec2(but_gadget_size_animset, 0)))
				{
					cstr sTempFileMakeLifeEasier = g.fpscrootdir_s + "\\Files\\charactercreatorplus\\animations\\exportedtemplate.txt";
					char pTempFileMakeLifeEasier[MAX_PATH];
					strcpy (pTempFileMakeLifeEasier, sTempFileMakeLifeEasier.Get());
					GG_GetRealPath(pTempFileMakeLifeEasier, 1);
					OpenToWrite(1, pTempFileMakeLifeEasier);
					for (int i = 0; i < g_pAnimSlotList.size(); i++)
					{
						char pMyLine[MAX_PATH];
						if (g_pAnimSlotList[i].fStep1 == 0)
						{
							sprintf(pMyLine, "\\%s", g_pAnimSlotList[i].pName);
						}
						else
						{
							if (g_pAnimSlotList[i].fStep2 == 0)
							{
								sprintf(pMyLine, "\\%s;%d", g_pAnimSlotList[i].pName, (int)(g_pAnimSlotList[i].fStep1 - g_pAnimSlotList[i].fStart));
							}
							else
							{
								if (g_pAnimSlotList[i].fStep3 == 0)
								{
									sprintf(pMyLine, "\\%s;%d;%d", g_pAnimSlotList[i].pName, (int)(g_pAnimSlotList[i].fStep1 - g_pAnimSlotList[i].fStart), (int)(g_pAnimSlotList[i].fStep2 - g_pAnimSlotList[i].fStart));
								}
								else
								{
									sprintf(pMyLine, "\\%s;%d;%d;%d", g_pAnimSlotList[i].pName, (int)(g_pAnimSlotList[i].fStep1 - g_pAnimSlotList[i].fStart), (int)(g_pAnimSlotList[i].fStep2 - g_pAnimSlotList[i].fStart), (int)(g_pAnimSlotList[i].fStep3 - g_pAnimSlotList[i].fStart));
								}
							}
						}
						WriteString (1, pMyLine);
					}
					CloseFile(1);
				}
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("Click to save template of animation slots to writable 'charactercreatorplus\\animations'.");

				// process ALL animation TXT files after a change (creates new charactercreatorplus\animations\sets)
				ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((w * 0.5) - (but_gadget_size_animset * 0.5), 0.0f));
				if (ImGui::StyleButton("Regenerate Animation Sets##AnimaionToolSave", ImVec2(but_gadget_size_animset, 0)))
				{
					std::vector<cstr> DefaultAnimSetList;
					DefaultAnimSetList.clear();
					cstr AllAnimSetsToRegenerate_s = g.fpscrootdir_s + "\\Files\\charactercreatorplus\\animations\\sets.txt";
					char pAllAnimSetsToRegenerate[MAX_PATH];
					strcpy (pAllAnimSetsToRegenerate, AllAnimSetsToRegenerate_s.Get());
					GG_GetRealPath(pAllAnimSetsToRegenerate, 0);
					OpenToRead(1, pAllAnimSetsToRegenerate);
					while (FileEnd(1) == 0)
					{
						LPSTR pLine = ReadString(1);
						DefaultAnimSetList.push_back(pLine);
					}
					CloseFile(1);
					if (DefaultAnimSetList.size() > 0)
					{
						for ( int iSetID = 0; iSetID < DefaultAnimSetList.size(); iSetID++)
						{
							// get default filename 
							char pAnimSetFile[MAX_PATH];
							strcpy(pAnimSetFile, DefaultAnimSetList[iSetID].Get());
							LPSTR pDefaultFilename = strstr(pAnimSetFile, "=");
							if (pDefaultFilename)
							{
								// and TXT file to create animations from
								*pDefaultFilename = 0;
								char pTXTAnimFile[MAX_PATH];
								cstr TXTAnimFile_s = g.fpscrootdir_s + "\\Files\\charactercreatorplus\\animations\\";
								strcpy (pTXTAnimFile, TXTAnimFile_s.Get());
								strcat(pTXTAnimFile, pDefaultFilename+1);
								strcat(pTXTAnimFile, ".txt");

								// clears all animation data from object
								animsysten_clearset(pObject);

								// handle DBO or TXT/DAT and append animations to imported model
								cstr AbsPathToAnim_s = g.fpscrootdir_s + "\\Files\\charactercreatorplus\\animations\\";
								char pAbsPathToAnim[MAX_PATH];
								strcpy (pAbsPathToAnim, AbsPathToAnim_s.Get());
								//GG_GetRealPath(pAbsPathToAnim, 0);
								animsystem_loadanimtextfile (pObject, pAbsPathToAnim, pTXTAnimFile);

								// and ensure animset is detailed inside object!
								extern void UpdateObjectWithAnimSlotList(sObject* pObject);
								UpdateObjectWithAnimSlotList(pObject);

								// and finally save the object as a default DBO
								cstr SaveAnimSet_s = g.fpscrootdir_s + "\\Files\\charactercreatorplus\\animations\\sets\\" + pAnimSetFile + ".dbo";
								char pSaveAnimSetDBO[MAX_PATH];
								strcpy (pSaveAnimSetDBO, SaveAnimSet_s.Get());
								if (FileExist(pSaveAnimSetDBO) == 1) DeleteFileA(pSaveAnimSetDBO);
								GG_SetWritablesToRoot(true);
								SaveObject(pSaveAnimSetDBO, objectnumber);
								GG_SetWritablesToRoot(false);
							}
						}
					}

					// last one gets to preview appended animation
					g_iCurrentAnimationSlotIndex = g_pAnimSlotList.size() - 1;
					g_bUpdateAnimationPreview = true;

				}
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("Click to regenerate all animation sets. Advanced Feature!");
			}

			// if flagged, show extra bone info as list of bones
			if (g_bShowBonesExtraInfo == true)
			{
				PerformCheckListForLimbs(objectnumber);
				ImGui::TextCenter("");
				char pBoneListTitle[MAX_PATH];
				sprintf(pBoneListTitle, "Bone List (Total Bones:%d)", ChecklistQuantity());
				ImGui::TextCenter(pBoneListTitle);
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("Currently, standard character creator rigs should have 67 bones");
				for (t.tc = 1; t.tc <= ChecklistQuantity(); t.tc++)
				{
					char pBoneItem[MAX_PATH];
					sprintf(pBoneItem, "%d : %s", t.tc, ChecklistString(t.tc));
					ImGui::Text(pBoneItem);
				}
			}

			// end of animation tool component
			ImGui::PopItemWidth();

			// delete slot if required
			if (iDeleteAnimSlot != -1 && g_pAnimSlotList.size() > 1)
			{
				// delete specified slot
				g_pAnimSlotList.erase(g_pAnimSlotList.begin() + iDeleteAnimSlot);
				if (g_iCurrentAnimationSlotIndex == iDeleteAnimSlot)
				{
					g_iCurrentAnimationSlotIndex = iDeleteAnimSlot - 1;
				}

				// ensure current animation is a valid slot
				if (g_iCurrentAnimationSlotIndex < 0) g_iCurrentAnimationSlotIndex = 0;
				if (g_iCurrentAnimationSlotIndex >= g_pAnimSlotList.size()) g_iCurrentAnimationSlotIndex = g_pAnimSlotList.size() - 1;

				// ensure model shows current animation index
				g_bUpdateAnimationPreview = true;
				iDeleteAnimSlot = -1;
			}

			// and refresh animation preview if flagged
			if (g_bUpdateAnimationPreview == true)
			{
				// animation slot details
				sAnimSlotStruct* pAnimSlotItem = &(g_pAnimSlotList[g_iCurrentAnimationSlotIndex]);
				if (pAnimSlotItem->fStart == pAnimSlotItem->fFinish)
				{
					fAnimFrameStart = pAnimSlotItem->fStart;
					g_bAnimatingObjectPreview = false;
				}
				if (g_bAnimatingObjectPreview == true)
				{
					if (pAnimSlotItem->bLooped == true)
					{
						SetObjectFrame(objectnumber, pAnimSlotItem->fStart);
						LoopObject(objectnumber, pAnimSlotItem->fStart, pAnimSlotItem->fFinish);
						SetObjectSpeed(objectnumber, fAnimSpeed);
					}
					else
					{
						SetObjectFrame(objectnumber, pAnimSlotItem->fStart);
						PlayObject(objectnumber, pAnimSlotItem->fStart, pAnimSlotItem->fFinish);
						SetObjectSpeed(objectnumber, fAnimSpeed);
					}
				}
				else
				{
					StopObject(objectnumber);
					SetObjectFrame(objectnumber, fAnimFrameStart);
				}
				g_bUpdateAnimationPreview = false;
			}
		}
		ImGui::Indent(-10);
	}
	else
	{
		// ZJ: Moved this here so the animations will play when the header is closed.
		sObject* pObject = GetObjectData(objectnumber);
		static float fAnimSpeed;
		fAnimSpeed = pObject->fAnimSpeed * 50;

		// and refresh animation preview if flagged
		if (g_bUpdateAnimationPreview == true)
		{
			// animation slot details
			sAnimSlotStruct* pAnimSlotItem = &(g_pAnimSlotList[g_iCurrentAnimationSlotIndex]);
			if (g_bAnimatingObjectPreview == true)
			{
				if (pAnimSlotItem->bLooped == true)
				{
					SetObjectFrame(objectnumber, pAnimSlotItem->fStart);
					LoopObject(objectnumber, pAnimSlotItem->fStart, pAnimSlotItem->fFinish);
					SetObjectSpeed(objectnumber, fAnimSpeed);
				}
				else
					PlayObject(objectnumber, pAnimSlotItem->fStart);
			}
			else
			{
				StopObject(objectnumber);
				SetObjectFrame(objectnumber, pAnimSlotItem->fStart);
			}
			g_bUpdateAnimationPreview = false;
		}
	}
}

void animsystem_animationtoolsimpleui(int objectnumber)
{
	extern int iLastOpenHeader;
	if (pref.bAutoClosePropertySections && iLastOpenHeader != 71)
		ImGui::SetNextItemOpen(false, ImGuiCond_Always);

	// If the last open header was the animation tool ui(70) then the user toggled advanced settings so this header should be opened.
	// As they are two different headers, but in the UI they appear to be the same one.
	if (pref.bAutoClosePropertySections && iLastOpenHeader == 70)
		ImGui::SetNextItemOpen(true, ImGuiCond_Always);

	// object being edited with animation tool
	sObject* pObject = GetObjectData(objectnumber);

	if (ImGui::StyleCollapsingHeader("Animation", ImGuiTreeNodeFlags_DefaultOpen))
	{
		iLastOpenHeader = 71;

		// standard numeric box to hold 99999 value
		static int iNumericWidth = 45;

		// if have animations
		ImGui::Indent(10);
		if (bFoundanimSet == NULL)
		{
			ImGui::TextCenter("No Animations Found");
		}
		else
		{
			// prep of animation tool component
			ImGui::PushItemWidth(-10);

			// animation preview
			if (ImGui::Checkbox("Animate Preview", &g_bAnimatingObjectPreview))
			{
				g_bUpdateAnimationPreview = true;
			}
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Animate Object Preview");
		}

		extern void ControlAdvancedSetting(int&, const char*, bool* = 0);
		if (t.importer.importerActive == 0)
			ControlAdvancedSetting(pref.iEnableAdvancedCharacterCreator, "Advanced Character Creator Settings");

		ImGui::Indent(-10);
	}
	
	// ZJ: Moved this here so the animations will play when the header is closed.
	static float fAnimSpeed;
	fAnimSpeed = pObject->fAnimSpeed * 50;

	// and refresh animation preview if flagged
	if (g_bUpdateAnimationPreview == true)
	{
		// animation slot details
		sAnimSlotStruct* pAnimSlotItem = &(g_pAnimSlotList[g_iCurrentAnimationSlotIndex]);
		if (g_bAnimatingObjectPreview == true)
		{
			if (pAnimSlotItem->bLooped == true)
			{
				SetObjectFrame(objectnumber, pAnimSlotItem->fStart);
				LoopObject(objectnumber, pAnimSlotItem->fStart, pAnimSlotItem->fFinish);
				SetObjectSpeed(objectnumber, fAnimSpeed);
			}
			else
				PlayObject(objectnumber, pAnimSlotItem->fStart);
		}
		else
		{
			StopObject(objectnumber);
			SetObjectFrame(objectnumber, pAnimSlotItem->fStart);
		}
		g_bUpdateAnimationPreview = false;
	}
}

bool importer_apply_materialformesh(MaterialComponentTEXTURESLOT eMatSlot, int iGGMeshTexSlot)
{
	// ensure the DBO mesh texture is large enough to hold references
	if (pSelectedMesh && pSelectedMesh->dwTextureCount <= GG_MESH_TEXTURE_SURFACE)
	{
		// if texture storage too small, increase it to store our references!
		extern bool EnsureTextureStageValid (sMesh* pMesh, int iTextureStage);
		EnsureTextureStageValid (pSelectedMesh, GG_MESH_TEXTURE_SURFACE);
	}
	if (pSelectedMaterial && !pSelectedMesh)
	{
		pSelectedMaterial->textures[eMatSlot].resource = {};
		pSelectedMaterial->textures[eMatSlot].name = "";
		pSelectedMaterial->SetDirty();
		wiJobSystem::context ctx;
		wiJobSystem::Wait(ctx);
	}
	if (pSelectedMesh)
	{
		// LB: We need to update wicked texture path folder as it is used when creating a local
		// copy of the texture we choose to assciate with he object being retextured
		char pOriginalTextureFolder[MAX_PATH];
		strcpy(pOriginalTextureFolder, "");
		if (strlen(cPreSelectedFile) > 0)
		{
			strcpy(pOriginalTextureFolder, cPreSelectedFile);
		}
		else
		{
			strcpy(pOriginalTextureFolder, pSelectedMaterial->textures[iGGMeshTexSlot].name.c_str());
		}
		for (int n = strlen(pOriginalTextureFolder) - 1; n > 0; n--)
		{
			if (pOriginalTextureFolder[n] == '\\' || pOriginalTextureFolder[n] == '/')
			{
				pOriginalTextureFolder[n+1] = 0;
				break;
			}
		}
		WickedCall_SetTexturePath(pOriginalTextureFolder);

		cStr tOldDir = GetDir();
		char * cFileSelected;
		if (strlen(cPreSelectedFile) > 0)
		{
			cFileSelected = &cPreSelectedFile[0];
		}
		else
		{
			bool bOpenExplorerAtPrevLocation = true;
			if (bChooseSurfaceChannel)
			{
				bOpenExplorerAtPrevLocation = false;
				bChooseSurfaceChannel = false;
			}
			cFileSelected = importer_selectfile(iGGMeshTexSlot, pSelectedMaterial->textures[iGGMeshTexSlot].name, bOpenExplorerAtPrevLocation);
		}

		// When applying a single texture to all meshes, this function will be called multiple times...
		if (t.importer.bEditAllMesh)
		{
			// Early return if user cancels file dialog, to ensure they don't need to cancel repeatedly.
			if (!cFileSelected) return false;

			// ...copy the selected file, to prevent opening the file dialog multiple times.
			strcpy(cPreSelectedFile, cFileSelected);
		}
		
		SetDir(tOldDir.Get());
		if (cFileSelected && strlen(cFileSelected) > 0)
		{
			// check if texture in the current texture path (where object and its textures are located)
			LPSTR pSelectedFilenameOnly = cFileSelected;
			char pDetermineSelectedPath[MAX_PATH];
			strcpy(pDetermineSelectedPath, cFileSelected);
			for (int n = strlen(pDetermineSelectedPath); n > 0; n--)
			{
				if (pDetermineSelectedPath[n] == '\\' || pDetermineSelectedPath[n] == '/')
				{
					pSelectedFilenameOnly = cFileSelected + n + 1;
					pDetermineSelectedPath[n+1] = 0;
					break;
				}
			}
			extern std::string g_pWickedTexturePath;
			LPSTR pWickedTexturePath = (LPSTR)g_pWickedTexturePath.c_str();
			if (strnicmp (pDetermineSelectedPath + strlen(pDetermineSelectedPath) - strlen(pWickedTexturePath), pWickedTexturePath, strlen(pWickedTexturePath)) != NULL)
			{
				// if not, we need to copy this selected texture into that area (writable folder) so that the object
				// can find this texture locally in the future, but ensure we do not overwrite anything, so can rename here
				char pNewFileLocation[MAX_PATH];
				strcpy (pNewFileLocation, g.fpscrootdir_s.Get());	//g.fpscrootdir_s.Get() returns the full path to the texture when in the importer.
				strcat (pNewFileLocation, "\\Files\\");
				strcat (pNewFileLocation, pWickedTexturePath);
				strcat (pNewFileLocation, pSelectedFilenameOnly);
				GG_GetRealPath(pNewFileLocation, 1); //This appears to get the writable area folder, but the path ends up containing the path that the user selected.
				// ^ An incorrect path is only generated when in the model importer, when editing custom materials in the object properties...
				// ... the path is perfectly fine.
				int iRenamedFile = 2;
				while (FileExist(pNewFileLocation) == 1)
				{
					// if need to rename, find a unique name
					char pExt[5];
					char pNewFilenameNoExt[MAX_PATH];
					strcpy (pNewFilenameNoExt, pSelectedFilenameOnly);
					strcpy(pExt, pNewFilenameNoExt + strlen(pNewFilenameNoExt) - 4);
					pNewFilenameNoExt[strlen(pNewFilenameNoExt) - 4] = 0;
					char pNewFilenameOnly[MAX_PATH];
					sprintf (pNewFilenameOnly, "%s%d%s", pNewFilenameNoExt, iRenamedFile, pExt);
					strcpy (pNewFileLocation, g.fpscrootdir_s.Get());
					strcat (pNewFileLocation, "\\Files\\");
					strcat (pNewFileLocation, pWickedTexturePath);
					strcat (pNewFileLocation, pNewFilenameOnly);
					GG_GetRealPath(pNewFileLocation, 1);
					iRenamedFile++;
				}
				CopyFileA(cFileSelected, pNewFileLocation, TRUE);

				if(t.importer.importerActive == 0)
					cFileSelected = pNewFileLocation;	// Wrong file path in the importer only.
			}

			// apply texture file to material
			pSelectedMaterial->textures[eMatSlot].name = cFileSelected;
			pSelectedMaterial->textures[eMatSlot].resource = WickedCall_LoadImage(pSelectedMaterial->textures[eMatSlot].name);
			if (pSelectedMaterial->textures[eMatSlot].resource.IsValid())
			{
				//PE: TODO Check if we need to copy file to remote project.
				extern bool entity_copytoremoteifnotthere(LPSTR);
				entity_copytoremoteifnotthere(cFileSelected);

				// loaded okay, update material
				char* pTextureFilename = pSelectedMesh->pTextures[iGGMeshTexSlot].pName;
				strcpy(pTextureFilename, cFileSelected);
				switch (iGGMeshTexSlot)
				{
				case GG_MESH_TEXTURE_NORMAL : 
					pSelectedMaterial->SetNormalMapStrength(1.0f);
					t.importer.bInvertNormalMap = false;
					strcpy(t.importer.pOrigNormalMap, cFileSelected);
					break;
				case GG_MESH_TEXTURE_SURFACE : 
					pSelectedMaterial->SetRoughness(1.0f);
					pSelectedMaterial->SetMetalness(1.0f);
					pSelectedMaterial->SetOcclusionEnabled_Primary(true);
					pSelectedMaterial->SetOcclusionEnabled_Secondary(false);
					break;
				case GG_MESH_TEXTURE_EMISSIVE : 
					pSelectedMaterial->SetEmissiveStrength(1.0f);
					break;
				}
				pSelectedMaterial->SetDirty();
				wiJobSystem::context ctx;
				wiJobSystem::Wait(ctx);
			}
			else
			{
				// failed to load, reset slot
				pSelectedMaterial->textures[eMatSlot].resource = {};
				pSelectedMaterial->textures[eMatSlot].name = "";
				pSelectedMaterial->SetDirty();
				wiJobSystem::context ctx;
				wiJobSystem::Wait(ctx);
			}
		}
	}

	return true;
}

LPSTR animsystem_getweapontype (LPSTR pSelectedWeapon, LPSTR pAnimSetOverride)
{
	LPSTR pWeaponType = "-melee";//""; no weapon MUST be melee!!
	// can override from gunspec file now
	if (strlen(pAnimSetOverride) > 0)
	{
		if (stricmp(pAnimSetOverride, "-pistol") == NULL) 
			pWeaponType = "";
		else
			pWeaponType = pAnimSetOverride;
	}

	// correct animset to use
	return pWeaponType;
}

void animsystem_weaponproperty (int characterbasetype, bool readonly, entityeleproftype* edit_grideleprof, bool bForShooting, bool bForMelee)
{
	// weapon selection for character properties (object editing and character creator use this)
	LPSTR pAttachmentTitle = "Weapon";
	cstr sCurrentWeapon = edit_grideleprof->hasweapon_s;
	extern char* imgui_setpropertylist2c_v2(int, int, char*, char*, char*, int, bool, bool, bool, bool, int);
	edit_grideleprof->hasweapon_s = imgui_setpropertylist2c_v2(t.group, t.controlindex, edit_grideleprof->hasweapon_s.Get(), pAttachmentTitle, t.strarr_s[209].Get(), 1, readonly, true, bForShooting, bForMelee, 0);
	LPSTR pSelectedWeapon = edit_grideleprof->hasweapon_s.Get();
	if (stricmp(pSelectedWeapon, sCurrentWeapon.Get()) != NULL)
	{
		// iof weapon changes, refresh any overrideanimset_s that might be set
		edit_grideleprof->overrideanimset_s = "";
		extern bool g_bNowPopulateWithCorrectAnimSet;
		g_bNowPopulateWithCorrectAnimSet = true;
	}

	// allow player to take weapon (and ammo)
	t.tfile_s = cstr("gamecore\\guns\\") + edit_grideleprof->hasweapon_s + cstr("\\HUD.dbo");
	if (FileExist(t.tfile_s.Get()) == 1)
	{
		// any weapon specified may be dropped, and so needs to exist in the level so it can be cloned later
		bool bThisWeaponIsInLevel = false;
		for (int n = 0; n < g_collectionList.size(); n++)
		{
			if (g_collectionList[n].collectionFields.size() > 8)
			{
				if (g_collectionList[n].iEntityID > 0)
				{
					LPSTR pCollectionItemWeapon = g_collectionList[n].collectionFields[8].Get();
					if (strlen(pCollectionItemWeapon) > 7)
					{
						LPSTR pJustWeaponPathAndName = pCollectionItemWeapon + 7; // weapon= skip
						if (stricmp(pJustWeaponPathAndName, pSelectedWeapon) == NULL)
						{
							// also check the entity ACTUALLY exists (user may delete it suddenly)
							int actualE = g_collectionList[n].iEntityElementE;
							if (actualE > 0 && actualE < t.entityelement.size())
							{
								// collection lists can sometimes have blank entity references (old level corruptions)
								if (t.entityelement[actualE].bankindex > 0 && t.entityelement[actualE].profileobj > 0)
								{
									// only drop if definately have an object to drop
									bThisWeaponIsInLevel = true;
								}
							}
							break;
						}
					}
				}
			}
		}
		bool bCanTakeWeapon = false;
		if (bThisWeaponIsInLevel == true)
		{
			bCanTakeWeapon = edit_grideleprof->cantakeweapon;
			if (ImGui::Checkbox("Player Can Take Weapon", &bCanTakeWeapon))
			{
				edit_grideleprof->cantakeweapon = bCanTakeWeapon;
			}
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Tick to allow the player to take the weapon when this character dies");
		}
		else
		{
			// weapon is not physically in the level so cannot be dropped
			edit_grideleprof->cantakeweapon = false;
			ImGui::Text("NOTE: Weapon object is not in this level so cannot be dropped");
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("This weapon can be dropped if you physically add the weapon object associated with this weapon to the level");
		}
		if (bCanTakeWeapon)
		{
			// only if weapon shoots, no sense for melee weapons
			t.findgun_s = Lower(edit_grideleprof->hasweapon_s.Get());
			gun_findweaponindexbyname ();
			int gunid = t.foundgunid;
			if (g.firemodes[gunid][0].settings.reloadqty != 0 || g.firemodes[gunid][1].settings.reloadqty != 0 )
			{
				ImGui::TextCenter("Taken Weapon Ammo");
				ImGui::MaxSliderInputInt("##ammo", &edit_grideleprof->quantity, 0, 1000, "The amount of ammo the weapon will have when picked up");
			}
		}
	}
}

int g_iSkipFrameBeforeResetAsLUADescChanges = 0;
bool g_bNowPopulateWithCorrectAnimSet = false;

void animsystem_animationsetproperty (int characterbasetype, bool readonly, entityeleproftype* edit_grideleprof, int iAnimationSetType, int elementID)
{
	// <Animation Set> included in LUA, so allow user to change the underlying animation set for this object
	// iAnimationSetType : 1-soldier, 2-melee, 3-zombie, 4-default
	bool bRefreshObjectAnimationSet = false;
	if (g_bNowPopulateWithCorrectAnimSet == true)
	{
		bool bWipeOutOverrideIfCustomType = false;
		LPSTR pCurrentWeapon = edit_grideleprof->hasweapon_s.Get();
		LPSTR pWeaponType = "";
		if (strlen(pCurrentWeapon) > 0)
		{
			t.findgun_s = Lower(pCurrentWeapon);
			gun_findweaponindexbyname ();
			int gunid = t.foundgunid;
			pWeaponType = animsystem_getweapontype(pCurrentWeapon, t.gun[gunid].animsetoverride.Get());
		}
		LPSTR pCharTypeName = "";
		if (iAnimationSetType == 1 || iAnimationSetType == 2)
		{
			// soldier or melee
			if (characterbasetype >= 0 && characterbasetype <= 3)
			{
				pCharTypeName = "adult male";
				if (characterbasetype == 1 || characterbasetype == 3) pCharTypeName = "adult female";
			}
			else
			{
				if(characterbasetype > 0)
				{
					pCharTypeName = g_CharacterType[characterbasetype].pPartsFolder;
				}
				else
				{
					bWipeOutOverrideIfCustomType = true;
				}
			}

			// melee will need melee animations if no specific weapon
			if (iAnimationSetType == 2)
			{
				if (strlen(pWeaponType) == 0) pWeaponType = "-melee";
			}
		}
		if (iAnimationSetType == 3)
		{
			// zombie
			if (characterbasetype >= 0 && characterbasetype <= 3)
			{
				pCharTypeName = "zombie male";
				if (characterbasetype == 1 || characterbasetype == 3) pCharTypeName = "zombie female";
			}
			else
			{
				if (characterbasetype > 0)
				{
					pCharTypeName = g_CharacterType[characterbasetype].pPartsFolder;
				}
				else
				{
					bWipeOutOverrideIfCustomType = true;
				}
			}
		}
		if (iAnimationSetType == 4)
		{
			// default animations keeps field blank so can specify any anim (even non characters)
		}
		else
		{
			char pPathToWeaponAnim[MAX_PATH];
			sprintf(pPathToWeaponAnim, "charactercreatorplus\\animations\\sets\\%s\\default animations%s.dbo", pCharTypeName, pWeaponType);
			if (FileExist(pPathToWeaponAnim) == 0)
			{
				// if not weapon specific animation set, use regular base default
				sprintf(pPathToWeaponAnim, "charactercreatorplus\\animations\\sets\\%s\\default animations.dbo", pCharTypeName);
			}
			if (FileExist(pPathToWeaponAnim))
			{
				// correct default for base type and weapon held
				edit_grideleprof->overrideanimset_s = pPathToWeaponAnim;
			}
		}

		// used when character type not determined (probably custom character type, ie low poly)
		if (bWipeOutOverrideIfCustomType == true)
		{
			edit_grideleprof->overrideanimset_s = "-";
		}

		// in any event, refresh object when change behavior (and associated anim)
		g_bNowPopulateWithCorrectAnimSet = false;
		bRefreshObjectAnimationSet = true;
	}

	// standard anim choices or custom animset file 
	cstr newAnimSetFile_s;
	extern int g_iDevToolsOpen;
	if (g_iDevToolsOpen == 0 && iAnimationSetType != 4)
	{
		int iSpecialValue = 0;
		if (characterbasetype >= 4)
		{
			// Specifies Gender-Neutral Animation
			iSpecialValue = 7;
		}
		else
		{
			if (iAnimationSetType == 1)
			{
				// Handles Soldier SetType
				if (characterbasetype == 0 || characterbasetype == 2)
				{
					iSpecialValue = 1;
				}
				else
				{
					iSpecialValue = 3;
				}
			}
			if (iAnimationSetType == 2)
			{
				// Handles Melee SetType
				if (characterbasetype == 0 || characterbasetype == 2)
				{
					iSpecialValue = 2;
				}
				else
				{
					iSpecialValue = 4;
				}
			}
			if (iAnimationSetType == 3)
			{
				// Handles Zombie SetType
				if (characterbasetype == 0 || characterbasetype == 2)
				{
					iSpecialValue = 5;
				}
				else
				{
					iSpecialValue = 6;
				}
			}
		}

		if (iSpecialValue > 0)
		{
			// standard users only see choices if a character base type
			extern char* imgui_setpropertylist2c_v2(int, int, char*, char*, char*, int, bool, bool, bool, bool, int);
			newAnimSetFile_s = imgui_setpropertylist2c_v2(t.group, t.controlindex, edit_grideleprof->overrideanimset_s.Get(), "Animation Choice", "Overrides the default animations used by default", 2, readonly, true, false, false, iSpecialValue);
		}
		else
		{
			// default animations keep their last selection (and can also be blank)
			newAnimSetFile_s = edit_grideleprof->overrideanimset_s;
		}
	}
	else
	{
		// advanced users always have option to change animset file or non character specific
		if (strcmp(edit_grideleprof->overrideanimset_s.Get(), "-") == NULL)
		{
			// will use animation stored in DBO
			bool bUseDefaultAnimation = true;
			if (ImGui::Checkbox("Use Default Animation", &bUseDefaultAnimation))
			{
				// allow user to specify override
				newAnimSetFile_s = "";
			}
			else
			{
				// continue using default anim
				newAnimSetFile_s = edit_grideleprof->overrideanimset_s;
			}
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Untick to disable default animation and specify your own animation file");
		}
		else
		{
			// can enter own animation file
			extern char* imgui_setpropertyfile2_v2(int group, char* data_s, char* field_s, char* desc_s, char* within_s, bool readonly, char* startsearch);
			newAnimSetFile_s = imgui_setpropertyfile2_v2(t.group, edit_grideleprof->overrideanimset_s.Get(), "Animation Choice", "Overrides the default animation set with a custom choice", "charactercreatorplus\\animations", readonly, 0);

			// user can opt to use default
			bool bUseDefaultAnimation = false;
			if (ImGui::Checkbox("Use Default Animation", &bUseDefaultAnimation))
			{
				// allow user to specify override
				newAnimSetFile_s = "-";
			}
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Tick to use the default animation stored in the object");
		}
	}
	if (stricmp(newAnimSetFile_s.Get(), edit_grideleprof->overrideanimset_s.Get()) != NULL)
	{
		// animation set changed, so refresh object so can get anim set names if needed (and see anim in level editor)
		edit_grideleprof->overrideanimset_s = newAnimSetFile_s;
		bRefreshObjectAnimationSet = true;

		// and trigger DLUA to refresh in case DisplayFPEBehavior needs to display latest animset names
		extern int fpe_current_loaded_script;
		fpe_current_loaded_script = -1;
	}
	if (bRefreshObjectAnimationSet == true && elementID > 0)
	{
		// replace actual object animations
		int iObjID = t.entityelement[elementID].obj;
		if (iObjID > 0)
		{
			LPSTR pWeaponAnimFile = edit_grideleprof->overrideanimset_s.Get();
			if (strlen(pWeaponAnimFile) > 1) // "" = default to weapon type, "-" = default to object anim
			{
				if (!FileExist(pWeaponAnimFile))
				{
					strcpy(pWeaponAnimFile, "");
				}
			}
			//LB: this must match what happens when the test level is run, that is, with ZERO override, use the default anim sets!!
			//if (strlen(pWeaponAnimFile) <= 1 && strlen(edit_grideleprof->overrideanimset_s.Get()) > 1) // "" = default to weapon type, "-" = default to object anim
			if (strlen(pWeaponAnimFile) == 0 && strlen(edit_grideleprof->overrideanimset_s.Get()) != 1) // "" = default to weapon type, "-" = default to object anim
			{
				LPSTR pCurrentWeapon = edit_grideleprof->hasweapon_s.Get();
				LPSTR pWeaponType = "";
				if (strlen(pCurrentWeapon) > 0)
				{
					t.findgun_s = Lower(pCurrentWeapon);
					gun_findweaponindexbyname ();
					int gunid = t.foundgunid;
					pWeaponType = animsystem_getweapontype(pCurrentWeapon, t.gun[gunid].animsetoverride.Get());
				}
				LPSTR pGender = NULL;
				if (characterbasetype == 0) pGender = "adult male";
				if (characterbasetype == 1) pGender = "adult female";
				if (characterbasetype == 2) pGender = "zombie male";
				if (characterbasetype == 3) pGender = "zombie female";
				if (characterbasetype > 3) pGender = g_CharacterType[characterbasetype].pPartsFolder;
				if (pGender != NULL)
				{
					if (t.entityprofile[t.entid].characterbasetype >= 0 && t.entityprofile[t.entid].characterbasetype <= 1)
						sprintf(pWeaponAnimFile, "charactercreatorplus\\animations\\sets\\%s\\default animations%s.dbo", pGender, pWeaponType);
					else
						sprintf(pWeaponAnimFile, "charactercreatorplus\\animations\\sets\\%s\\default animations.dbo", pGender);
				}
			}

			// when get the final anim file we need, append it
			if (strlen(pWeaponAnimFile) > 1) // "" = default to weapon type, "-" = default to object anim
			{
				if (FileExist(pWeaponAnimFile))
				{
					// appended animation
					sObject* pObject = GetObjectData(iObjID);
					AppendObject(pWeaponAnimFile, iObjID, 0);
					WickedCall_RefreshObjectAnimations(pObject, pObject->wickedloaderstateptr);
				}
			}
			else
			{
				// obtain the original animation from the object (in cases where user has created their own character an anims)
				int entid = t.entityelement[elementID].bankindex;
				if (entid > 0)
				{
					char pOrigModelPath[MAX_PATH];
					strcpy(pOrigModelPath, t.entitybank_s[entid].Get());
					for (int n = strlen(pOrigModelPath) - 1; n > 0; n--)
					{
						if (pOrigModelPath[n] == '\\' || pOrigModelPath[n] == '/')
						{
							pOrigModelPath[n] = 0;
							break;
						}
					}
					char pOrigModelFile[MAX_PATH];
					strcpy(pOrigModelFile, "entitybank\\");
					strcat(pOrigModelFile, pOrigModelPath);
					strcat(pOrigModelFile, "\\");
					strcat(pOrigModelFile, t.entityprofile[entid].model_s.Get());
					sObject* pObject = GetObjectData(iObjID);
					AppendObject(pOrigModelFile, iObjID, 0);
					WickedCall_RefreshObjectAnimations(pObject, pObject->wickedloaderstateptr);
				}
			}

			// and fire up first frame on the change
			SetObjectFrame (iObjID, 0); LoopObject (iObjID); StopObject (iObjID);

			// and apply correct starter animation as a preview
			extern void entity_loop_using_negative_playanimineditor(int e, int obj, cstr animname);
			entity_loop_using_negative_playanimineditor(elementID, iObjID, t.entityprofile[t.entityelement[elementID].bankindex].playanimineditor_name);
		}
	}
}

void animsystem_createlootlist(cstr ifused_s)
{
	// format is: "name;name;name" or "name;*99;name;*33" etc
	int iLootIndex = 0;
	char pLootStr[MAX_PATH];
	strcpy(pLootStr, ifused_s.Get());
	int iLootPercentage = 100;
	int n = 0;
	bool bOnlyShowOneChooseCollectible = false;
	int iOptionalPercString = 0;
	while (iLootIndex < 10 && n < strlen(pLootStr))
	{
		bool bTheEnd = false;
		if (n == strlen(pLootStr) - 1) bTheEnd = true;
		if (pLootStr[n] == ';' || bTheEnd == true)
		{
			bool bValid = true;
			char pThisOne[MAX_PATH];
			strcpy(pThisOne, pLootStr);
			if (bTheEnd==false)
			{
				strcpy(pLootStr, pLootStr + n + 1);
				pThisOne[n] = 0;
			}
			if (stricmp(pThisOne, "(Choose Collectible)") == NULL)
			{
				bValid = false;
				if (bOnlyShowOneChooseCollectible == false) bValid = true;
				bOnlyShowOneChooseCollectible = true;
			}
			if (bTheEnd==true)
			{
				if (bValid == true)
				{
					if (pThisOne[0] == '*')
					{
						iLootPercentage = atoi(pThisOne +1);
						g_lootListPercentage[iLootIndex-1] = iLootPercentage;
					}
					else
					{
						g_lootList_s[iLootIndex] = pThisOne;
						g_lootListPercentage[iLootIndex] = 100; // poss. changed above
						iLootIndex++;
					}
				}
				break; //! last one
			}
			else
			{
				if (bValid == true)
				{
					if (pThisOne[0] == '*')
					{
						iLootPercentage = atoi(pThisOne + 1);
						g_lootListPercentage[iLootIndex - 1] = iLootPercentage;
					}
					else
					{
						g_lootList_s[iLootIndex] = pThisOne;
						g_lootListPercentage[iLootIndex] = 100; // poss. changed above
						iLootIndex++;
					}
				}
				n = 0;
			}
		}
		else
		{
			n++;
		}
	}
	if (bOnlyShowOneChooseCollectible == false)
	{
		g_lootList_s[iLootIndex] = "(Choose Collectible)";
		g_lootListPercentage[iLootIndex] = 100;
		iLootIndex++;
	}
	g_iLootListCount = iLootIndex;
	if (g_iLootListCount > 10) g_iLootListCount = 10;
}

void animsystem_dropcollectablesetproperty(bool readonly, entityeleproftype* edit_grideleprof)
{
	bool bCanDrop = false;
	if (edit_grideleprof->ifused_s.Len() > 0) bCanDrop = true;
	if (ImGui::Checkbox("Player Can Take Loot", &bCanDrop))
	{
		if (bCanDrop == true)
		{
			int listmax = fillgloballistwithcollectables();
			edit_grideleprof->ifused_s = t.list_s[0];
		}
		else
		{
			edit_grideleprof->ifused_s = "";
		}
	}
	if (ImGui::IsItemHovered()) ImGui::SetTooltip("Tick to enable the character to drop collectibles when killed");
	if (edit_grideleprof->ifused_s.Len() > 0)
	{
		animsystem_createlootlist(edit_grideleprof->ifused_s);
		ImGui::TextCenter("Chance Of Dropping Anything");
		int iLootPerc = edit_grideleprof->lootpercentage;
		ImGui::MaxSliderInputInt("##chancelootdropanything", &iLootPerc, 0, 100, "Set the percentage chance that any loot is dropped at all");
		edit_grideleprof->lootpercentage = iLootPerc;
		cstr finallootstr_s  = "";
		for (int l = 0; l < g_iLootListCount; l++)
		{
			char pLootDropTitle[256];
			sprintf(pLootDropTitle, "Loot Object %d", 1+l);
			extern char* imgui_setpropertylist2c_v2(int, int, char*, char*, char*, int, bool, bool, bool, bool, int);
			g_lootList_s[l] = imgui_setpropertylist2c_v2(t.group, t.controlindex, g_lootList_s[l].Get(), pLootDropTitle, t.strarr_s[209].Get(), 21, readonly, true, false, false, 0);
			if (finallootstr_s.Len() > 0) finallootstr_s = finallootstr_s + ";";
			finallootstr_s += g_lootList_s[l];
			if (l < g_iLootListCount-1)
			{
				// skip last one as that allows adding to the list
				sprintf(pLootDropTitle, "Chance Drop For Loot Object %d", 1 + l);
				ImGui::TextCenter(pLootDropTitle);
				int iLootPerc = g_lootListPercentage[l];
				sprintf(pLootDropTitle, "##chancelootdrop%d", 1 + l);
				ImGui::MaxSliderInputInt(pLootDropTitle, &iLootPerc, 0, 100, "Set the percentage chance that this loot item would be dropped");
				g_lootListPercentage[l] = iLootPerc;
				finallootstr_s = finallootstr_s + ";";
				finallootstr_s += cstr("*") + cstr(g_lootListPercentage[l]);
			}
		}
		edit_grideleprof->ifused_s = finallootstr_s;
	}
}

void importer_applyframezerooffsets (int objectnumber, float fX, float fY, float fZ, float fRX, float fRY, float fRZ)
{
	sObject* pObject = g_ObjectList[objectnumber];
	if (pObject)
	{
		sFrame* pFrame = pObject->pFrame;
		if (pFrame)
		{
			// using first frame orientation to control an object wide adjustment (from importer (or from DBO))
			pFrame->vecOffset = GGVECTOR3(fX, fY, fZ);
			pFrame->vecRotation = GGVECTOR3(fRX, fRY, fRZ);
		}
		WickedCall_UpdateObject(pObject);
	}
}

void UpdateObjectWithAnimSlotList ( sObject* pObject )
{
	sAnimationSet* pPrevAnimSet = NULL;
	sAnimationSet* pAnimSetPtr = pObject->pAnimationSet;
	for (int slot = 0; slot < (int)g_pAnimSlotList.size(); slot++)
	{
		// sync UI anim list with animset list in DBO
		if (pAnimSetPtr == NULL)
		{
			sAnimationSet* pNewAnimSetForRef = new sAnimationSet();
			memset(pNewAnimSetForRef, 0, sizeof(pNewAnimSetForRef));
			pNewAnimSetForRef->ulLength = g_pAnimSlotList[slot].fFinish - g_pAnimSlotList[slot].fStart;
			strcpy(pNewAnimSetForRef->szName, g_pAnimSlotList[slot].pName);
			if(pPrevAnimSet) //PE: Got a crash here pPrevAnimSet == NULL.
				pPrevAnimSet->pNext = pNewAnimSetForRef;
			pAnimSetPtr = pNewAnimSetForRef;
		}
		else
		{
			strcpy(pAnimSetPtr->szName, g_pAnimSlotList[slot].pName);
		}
		if (slot > 0)
		{
			// 124 = reference animset (not containing animation data, only references main animset zero core)
			pAnimSetPtr->dwAnimSetType = 124;
			pAnimSetPtr->fAnimSetStart = g_pAnimSlotList[slot].fStart;
			pAnimSetPtr->fAnimSetFinish = g_pAnimSlotList[slot].fFinish;
			pAnimSetPtr->fAnimSetStep1 = g_pAnimSlotList[slot].fStep1;
			pAnimSetPtr->fAnimSetStep2 = g_pAnimSlotList[slot].fStep2;
			pAnimSetPtr->fAnimSetStep3 = g_pAnimSlotList[slot].fStep3;
		}
		pPrevAnimSet = pAnimSetPtr;
		pAnimSetPtr = pAnimSetPtr->pNext;
	}
	if (pAnimSetPtr && pPrevAnimSet)
	{
		// still have old animsets in DBO, remove them
		pPrevAnimSet->pNext = NULL;
		while (pAnimSetPtr)
		{
			sAnimationSet* pNextOne = pAnimSetPtr->pNext;
			pAnimSetPtr->pNext = NULL;
			delete pAnimSetPtr;
			pAnimSetPtr = pNextOne;
		}
	}
}

bool g_bIgnoreDBOAsAlreadyConverted = false;

void imgui_importer_refreshbatchlist (void)
{
	// collect list of models to convert
	batchFileList.clear();
	cstr pOldDir = GetDir();
	SetDir(cImportPath);
	ChecklistForFiles();
	for (int c = 1; c <= ChecklistQuantity(); c++)
	{
		LPSTR pFileName = ChecklistString(c);
		if (strcmp(pFileName, ".") != NULL && strcmp(pFileName, "..") != NULL)
		{
			bool bPermittedFormat = false;
			const char* pExtension = strrchr(pFileName, '.');
			if (pExtension)
			{
				if (stricmp(pExtension, ".x") == NULL) bPermittedFormat = true;
				if (g_bIgnoreDBOAsAlreadyConverted == false)
				{
					// want to avoid converting the converted (most of the time) :)
					if (stricmp(pExtension, ".dbo") == NULL) bPermittedFormat = true;
				}
				if (stricmp(pExtension, ".obj") == NULL) bPermittedFormat = true;
				if (stricmp(pExtension, ".fbx") == NULL) bPermittedFormat = true;
				if (stricmp(pExtension, ".gltf") == NULL) bPermittedFormat = true;
				if (stricmp(pExtension, ".glb") == NULL) bPermittedFormat = true;
			}
			if (bPermittedFormat == true)
			{
				batchFileList.push_back(pFileName);
			}
		}
	}
	SetDir(pOldDir.Get());
}

