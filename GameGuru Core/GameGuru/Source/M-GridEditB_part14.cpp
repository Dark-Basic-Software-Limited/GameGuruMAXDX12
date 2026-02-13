int Get_t_gridentityobj(void)
{
	return t.gridentityobj;
}
int Get_t_gridentity(void)
{
	return t.gridentity;
}

void StartDragDropFromEntityID(int iEntID,int iGroup,int iCustomImage)
{
	if (iEntID < 0) return;
	int masterid = t.entityelement[iEntID].bankindex;
	if (masterid <= 0) return;
	if (masterid > t.entitybank_s.size()) return;
	if (t.entitybank_s[masterid].Len() == 0) return;

	cstr find = t.entitybank_s[masterid];
	cFolderItem *pSearchFolder = &MainEntityList;
	cFolderItem::sFolderFiles * foundfiles = NULL;
	pSearchFolder = pSearchFolder->m_pNext;
	cStr path = "";
	cStr path_remove = pSearchFolder->m_sFolderFullPath.Get();
	std::string sFpeName;
	int ipath_remove_len = path_remove.Len();
	bool bFound = false;
	while (pSearchFolder)
	{
		if (pSearchFolder->iType == 0)
		{
			cStr path = pSearchFolder->m_sFolderFullPath.Get();
			bool bDoubleEntityBank = false;
			char *finde = (char *)pestrcasestr(path.Get(), "\\entitybank"); //Support entitybank inside entitybank.
			if (finde)
			{
				finde += 11;
				finde = (char *)pestrcasestr(finde, "\\entitybank");
				if (finde) bDoubleEntityBank = true;
			}
			if (!bDoubleEntityBank && path.Right(11) == "\\entitybank")
			{
				ipath_remove_len = path.Len();
			}
			else
			{
				if (pSearchFolder->m_pFirstFile) 
				{
					cFolderItem::sFolderFiles * searchfiles = pSearchFolder->m_pFirstFile->m_pNext;
					while (searchfiles) {
						foundfiles = searchfiles;
						path = pSearchFolder->m_sFolderFullPath.Get();
						char *final_name = path.Get();
						final_name += ipath_remove_len;
						if (*final_name == '\\')
							final_name++;

						std::string path_for_filename = final_name;
						sFpeName = path_for_filename.c_str();
						sFpeName = sFpeName + "\\" + foundfiles->m_sName.Get();
						if (stricmp(find.Get(), sFpeName.c_str()) == 0)
							bFound = true;
						if (bFound)
							break;
						searchfiles = searchfiles->m_pNext;
					}
				}
			}
			if (bFound)
				break;
		}
		pSearchFolder = pSearchFolder->m_pNext;
	}

	if (foundfiles)
	{
		//CheckTooltipObjectDelete();
		CloseDownEditorProperties();
		t.inputsys.constructselection = 0;

		foundfiles->m_dropptr = foundfiles;
		if(iGroup >= 0)
			foundfiles->iAnimationFrom = 100000+ iGroup; //Reuse var for selecting a special entity for drag drop.
		else
			foundfiles->iAnimationFrom = iEntID; //Reuse var for selecting a special entity for drag drop.
		foundfiles->m_sFolder = sFpeName.c_str();
		ImGui::SetDragDropPayload("DND_MODEL_DROP_TARGET", foundfiles, sizeof(void *));
		if (iCustomImage > 0)
		{
			float fRatio = (float) ImageHeight(iCustomImage)  / (float) ImageWidth(iCustomImage);
			float imagew = 200.0f;
			float imageh = imagew * fRatio;
			ImGui::ImgBtn(iCustomImage, ImVec2(imagew, imageh), drawCol_back, drawCol_normal, drawCol_hover, drawCol_Down, 0, 0, 0, 0, false);
		}
		else
		{
			ImGui::ImgBtn(t.entityprofile[masterid].iThumbnailSmall, ImVec2(media_icon_size_leftpanel, media_icon_size_leftpanel), drawCol_back, drawCol_normal, drawCol_hover, drawCol_Down, 0, 0, 0, 0, false);
		}
		//ImGui::Text("%s", myfiles->m_sName.Get());
		//ImGui::SetCursorPos(oldCursor);
		pDragDropFile = foundfiles;
		ImGui::EndDragDropSource();
		bReadyToDropEntity = false;
		iDragDropActive = 50;
		bDraggingActive = true;
		bDraggingActiveInitial = true;
		// LB: these can be uninitialised, but we need these filled so the plane can be under the cursor initially
		t.gridentityposx_f = t.inputsys.localx_f;
		t.gridentityposy_f = t.inputsys.localcurrentterrainheight_f;
		t.gridentityposz_f = t.inputsys.localy_f;
	}
}

ImVec4 GetRealSizeToGridEntity(int direction)
{
	//Object size from center, include rotation.
	ImVec4 size = { 0.0f,0.0f,0.0f,0.0f };
	if (t.gridentityobj > 0)
	{
		//PE: Only use pivot on non markers and not on ebe.
		if (t.entityprofile[t.gridentity].isebe == 0 && t.entityprofile[t.gridentity].ismarker == 0)
		{
			//Find lowest point in bounding box, and use this to adjust Y
			sObject* pObject = GetObjectData(t.gridentityobj);
			if (pObject)
			{
				GGMATRIX matARotation;
				GGVECTOR3 box1;
				GGMATRIX matRotateX, matRotateY, matRotateZ;
				if (pObject->position.bFreeFlightRotation)
				{
					matARotation = pObject->position.matFreeFlightRotate;
				}
				else
				{
					//ZXY
					GGMatrixRotationX(&matRotateX, GGToRadian(pObject->position.vecRotate.x));	// x rotation
					GGMatrixRotationY(&matRotateY, GGToRadian(pObject->position.vecRotate.y));	// y rotation
					GGMatrixRotationZ(&matRotateZ, GGToRadian(pObject->position.vecRotate.z));	// z rotation
					matARotation = matRotateX * matRotateY * matRotateZ;
				}
				if (pObject->position.bApplyPivot)
				{
					matARotation *= pObject->position.matPivot;
				}

				AABB aabb;
				if (direction == 0)
				{
					aabb._min.x = (pObject->collision.vecMin.x - pObject->collision.vecMax.x) * 0.5;  //offset from center.
					aabb._min.y = pObject->collision.vecMin.y;
					aabb._min.z = 0.0f;
				}
				else if (direction == 1)
				{
					aabb._min.x = 0.0f;
					aabb._min.y = pObject->collision.vecMin.y;
					aabb._min.z = (pObject->collision.vecMin.z - pObject->collision.vecMax.z) * 0.5; //offset from center.
				}
				else if (direction == 2)
				{
					aabb._min.x = fabs((pObject->collision.vecMin.x - pObject->collision.vecMax.x) * 0.5);  //offset from center.
					aabb._min.y = pObject->collision.vecMin.y;
					aabb._min.z = 0.0f;
				}
				else if (direction == 3)
				{
					aabb._min.x = 0.0f;
					aabb._min.y = pObject->collision.vecMin.y;
					aabb._min.z = fabs((pObject->collision.vecMin.z - pObject->collision.vecMax.z) * 0.5); //offset from center.
				}

				box1.x = aabb._min.x; box1.y = aabb._min.y; box1.z = aabb._min.z;
				GGVec3TransformCoord(&box1, &box1, &matARotation);
				aabb._min.x = box1.x; aabb._min.y = box1.y; aabb._min.z = box1.z;
				
				aabb._min.x = (aabb._min.x * pObject->position.vecScale.x);
				aabb._min.y = (aabb._min.y * pObject->position.vecScale.y);
				aabb._min.z = (aabb._min.z * pObject->position.vecScale.z);

				size.x = aabb._min.x;
				size.y = aabb._min.y;
				size.z = aabb._min.z;
				size.w = 1.0;
			}
		}

	}
	return(size);
}

ImVec4 GetRealCenterToObject(int obj)
{
	ImVec4 realcenter = { 0.0f,0.0f,0.0f,0.0f };
	if (obj > 0)
	{
		//Find lowest point in bounding box, and use this to adjust Y
		sObject* pObject = GetObjectData(obj);
		if (pObject)
		{

			GGMATRIX matARotation;
			GGVECTOR3 box1;
			GGMATRIX matRotateX, matRotateY, matRotateZ;
			if (pObject->position.bFreeFlightRotation)
			{
				matARotation = pObject->position.matFreeFlightRotate;
			}
			else
			{
				//ZXY
				GGMatrixRotationX(&matRotateX, GGToRadian(pObject->position.vecRotate.x));	// x rotation
				GGMatrixRotationY(&matRotateY, GGToRadian(pObject->position.vecRotate.y));	// y rotation
				GGMatrixRotationZ(&matRotateZ, GGToRadian(pObject->position.vecRotate.z));	// z rotation
				matARotation = matRotateX * matRotateY * matRotateZ;
			}
			if (pObject->position.bApplyPivot)
			{
				matARotation *= pObject->position.matPivot;
			}

			AABB aabb;
			aabb._min.x = pObject->collision.vecMin.x;
			aabb._min.y = pObject->collision.vecMin.y;
			aabb._min.z = pObject->collision.vecMin.z;
			aabb._max.x = pObject->collision.vecMax.x;
			aabb._max.y = pObject->collision.vecMax.y;
			aabb._max.z = pObject->collision.vecMax.z;

			box1.x = aabb._min.x; box1.y = aabb._min.y; box1.z = aabb._min.z;
			GGVec3TransformCoord(&box1, &box1, &matARotation);
			aabb._min.x = box1.x; aabb._min.y = box1.y; aabb._min.z = box1.z;
			box1.x = aabb._max.x; box1.y = aabb._max.y; box1.z = aabb._max.z;
			GGVec3TransformCoord(&box1, &box1, &matARotation);
			aabb._max.x = box1.x; aabb._max.y = box1.y; aabb._max.z = box1.z;

			aabb._min.x = (aabb._min.x * pObject->position.vecScale.x);
			aabb._min.y = (aabb._min.y * pObject->position.vecScale.y);
			aabb._min.z = (aabb._min.z * pObject->position.vecScale.z);
			aabb._max.x = (aabb._max.x * pObject->position.vecScale.x);
			aabb._max.y = (aabb._max.y * pObject->position.vecScale.y);
			aabb._max.z = (aabb._max.z * pObject->position.vecScale.z);

			realcenter.x = (aabb._min.x + aabb._max.x)*0.5;
			realcenter.y = (aabb._min.y + aabb._max.y)*0.5;
			realcenter.z = (aabb._min.z + aabb._max.z)*0.5;
			realcenter.w = 1.0;
		}
	}
	return(realcenter);
}

ImVec4 GetRealCenterToGridEntity(void)
{
	ImVec4 realcenter = { 0.0f,0.0f,0.0f,0.0f };
	if (t.gridentityobj > 0)
	{

		//PE: Only use pivot on non markers and not on ebe.
		if (t.entityprofile[t.gridentity].isebe == 0 && t.entityprofile[t.gridentity].ismarker == 0)
		{
			//Find lowest point in bounding box, and use this to adjust Y
			sObject* pObject = GetObjectData(t.gridentityobj);
			if (pObject)
			{

				GGMATRIX matARotation;
				GGVECTOR3 box1;
				GGMATRIX matRotateX, matRotateY, matRotateZ;
				if (pObject->position.bFreeFlightRotation)
				{
					matARotation = pObject->position.matFreeFlightRotate;
				}
				else
				{
					//ZXY
					GGMatrixRotationX(&matRotateX, GGToRadian(pObject->position.vecRotate.x));	// x rotation
					GGMatrixRotationY(&matRotateY, GGToRadian(pObject->position.vecRotate.y));	// y rotation
					GGMatrixRotationZ(&matRotateZ, GGToRadian(pObject->position.vecRotate.z));	// z rotation
					matARotation = matRotateX * matRotateY * matRotateZ;
				}
				if (pObject->position.bApplyPivot)
				{
					matARotation *= pObject->position.matPivot;
				}

				AABB aabb;
				aabb._min.x = pObject->collision.vecMin.x;
				aabb._min.y = pObject->collision.vecMin.y;
				aabb._min.z = pObject->collision.vecMin.z;
				aabb._max.x = pObject->collision.vecMax.x;
				aabb._max.y = pObject->collision.vecMax.y;
				aabb._max.z = pObject->collision.vecMax.z;

				box1.x = aabb._min.x; box1.y = aabb._min.y; box1.z = aabb._min.z;
				GGVec3TransformCoord(&box1, &box1, &matARotation);
				aabb._min.x = box1.x; aabb._min.y = box1.y; aabb._min.z = box1.z;
				box1.x = aabb._max.x; box1.y = aabb._max.y; box1.z = aabb._max.z;
				GGVec3TransformCoord(&box1, &box1, &matARotation);
				aabb._max.x = box1.x; aabb._max.y = box1.y; aabb._max.z = box1.z;

				aabb._min.x = (aabb._min.x * pObject->position.vecScale.x);
				aabb._min.y = (aabb._min.y * pObject->position.vecScale.y);
				aabb._min.z = (aabb._min.z * pObject->position.vecScale.z);
				aabb._max.x = (aabb._max.x * pObject->position.vecScale.x);
				aabb._max.y = (aabb._max.y * pObject->position.vecScale.y);
				aabb._max.z = (aabb._max.z * pObject->position.vecScale.z);

				realcenter.x = (aabb._min.x + aabb._max.x)*0.5;
				realcenter.y = (aabb._min.y + aabb._max.y)*0.5;
				realcenter.z = (aabb._min.z + aabb._max.z)*0.5;
				realcenter.w = 1.0;
			}
		}

	}
	return(realcenter);
}

float GetLowestY(int obj)
{
	if (obj <= 0) return(0.0f);

	if (t.entityprofile[t.gridentity].isebe == 0 && t.entityprofile[t.gridentity].ismarker == 0)
	{
		//Find lowest point in bounding box, and use this to adjust Y
		sObject* pObject = GetObjectData(obj);
		if (pObject)
		{

			GGMATRIX matARotation;
			GGVECTOR3 box1;
			GGMATRIX matRotateX, matRotateY, matRotateZ;
			if (pObject->position.bFreeFlightRotation)
			{
				matARotation = pObject->position.matFreeFlightRotate;
			}
			else
			{
				//ZXY
				GGMatrixRotationX(&matRotateX, GGToRadian(pObject->position.vecRotate.x));	// x rotation
				GGMatrixRotationY(&matRotateY, GGToRadian(pObject->position.vecRotate.y));	// y rotation
				GGMatrixRotationZ(&matRotateZ, GGToRadian(pObject->position.vecRotate.z));	// z rotation
				matARotation = matRotateX * matRotateY * matRotateZ;
			}
			if (pObject->position.bApplyPivot)
			{
				matARotation *= pObject->position.matPivot;
			}

			AABB aabb;
			aabb._min.x = pObject->collision.vecMin.x;
			aabb._min.y = pObject->collision.vecMin.y;
			aabb._min.z = pObject->collision.vecMin.z;
			aabb._max.x = pObject->collision.vecMax.x;
			aabb._max.y = pObject->collision.vecMax.y;
			aabb._max.z = pObject->collision.vecMax.z;

			box1.x = aabb._min.x; box1.y = aabb._min.y; box1.z = aabb._min.z;
			GGVec3TransformCoord(&box1, &box1, &matARotation);
			aabb._min.x = box1.x; aabb._min.y = box1.y; aabb._min.z = box1.z;
			box1.x = aabb._max.x; box1.y = aabb._max.y; box1.z = aabb._max.z;
			GGVec3TransformCoord(&box1, &box1, &matARotation);
			aabb._max.x = box1.x; aabb._max.y = box1.y; aabb._max.z = box1.z;

			aabb._min.x = (aabb._min.x * pObject->position.vecScale.x);
			aabb._min.y = (aabb._min.y * pObject->position.vecScale.y);
			aabb._min.z = (aabb._min.z * pObject->position.vecScale.z);
			aabb._max.x = (aabb._max.x * pObject->position.vecScale.x);
			aabb._max.y = (aabb._max.y * pObject->position.vecScale.y);
			aabb._max.z = (aabb._max.z * pObject->position.vecScale.z);

			float fLowestPoint = aabb._min.y;
			if (aabb._max.y < aabb._min.y)
				fLowestPoint = aabb._max.y;

			return(fLowestPoint);
		}
	}
	return(0.0f);
}

void ApplyPivotToGridEntity(void)
{
}

bool bUseEditorOutlineSelection(void) { return pref.iEnableEditorOutlineSelection; }
float fGetHighlightThickness(void) { return pref.fHighLightThickness; }
int iGetMouseClickState(void) { return t.inputsys.mclick; }
int iGetgrideditselect(void) { return t.grideditselect; }

//#########################################
//#### Template for new docking window ####
//#########################################

void ProcessTemplateWindow(void)
{
	//Make sure to setup the docking settings search: DockBuilderDockWindow
	//To open window set bTemplate_Window = true;
	//Remember: MAXVERSION should be increased when you add a new window.

	bool bTemplate_Window = false; //This should be moved to the top of code.

	if (refresh_gui_docking == 0 && !bTemplate_Window)
	{
		//Make sure window is setup in docking space.
		ImGui::Begin("Template Window##MustBeUnique", &bTemplate_Window, iGenralWindowsFlags);
		ImGui::End();
	}

	if (!bTemplate_Window)
		return;

	int wflags = ImGuiTreeNodeFlags_DefaultOpen;

	ImGui::Begin("Template Window##MustBeUnique", &bTemplate_Window, iGenralWindowsFlags);

	if (ImGui::StyleCollapsingHeader("Help With Template", wflags)) {

		ImGui::Indent(10);
		ImGui::PushItemWidth(-10);
		ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));

		//All you gui stuff goes here.
		ImGui::TextCenter("Heading use TextCenter");

		//Float Slider Template.
		static float fFloatSliderTest = 1000.0f; //1000.0f would be center (50) in this sample.
		if (ImGui::MaxSliderInputFloat("##templatefloatinput", &fFloatSliderTest, 0.0, 2000.0, "All Sliders should use this code."))
		{
		}

		ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));
		ImGui::PopItemWidth();
		ImGui::Indent(-10);
	}


	if (ImGui::GetCurrentWindow()->ScrollbarSizes.x > 0) {
		//Hitting exactly at the botton could cause flicker, so add some additional lines when scrollbar on.
		ImGui::Text("");
		ImGui::Text("");
	}

	ImRect bbwin(ImGui::GetWindowPos(), ImGui::GetWindowPos() + ImGui::GetWindowSize());
	if (ImGui::IsMouseHoveringRect(bbwin.Min, bbwin.Max))
	{
		bImGuiGotFocus = true; //Must set bImGuiGotFocus so if you type "w" in a imgui widget , editor dont move forward.
	}
	if (ImGui::IsAnyItemFocused()) {
		bImGuiGotFocus = true;
	}

	ImGui::End();
}

//#####


struct BugReport
{
	//time_t CreateTime;
	char cCreatedAt[32];
	char cTitle[256];
	char cDescription[4096];
	char cReply[4096];
	int iStatus;
};
static std::vector<BugReport *> sBugList;
bool g_bBugTrackerConnected = false;
int g_iUserID = 0;
cstr g_sHashToken = "";

int CLB(ImGuiTextEditCallbackData* data)
{
	float textWidth = ImGui::CalcTextSize(data->Buf).x;
	float controlWidth = *(float*)data->UserData;
	if (textWidth > controlWidth)
	{
		int iCurrPos = data->CursorPos;
		while (data->Buf[iCurrPos] != ' ' && iCurrPos > 0) iCurrPos--;
		if (iCurrPos > 0 && data->Buf[iCurrPos] == ' ' && data->Buf[iCurrPos+1] != '\n') data->InsertChars(iCurrPos+1, "\n");
		data->BufDirty = true;
	}
	return data->BufTextLen;
}

void ProcessBugReporting(void)
{
	static char cBugTitle[256];
	static char cBugDescription[4096];
	static char cBugDescriptionCopy[4096];
	static bool bIncludeScreenShot = true;
	static bool bIncludesystemSpecs = true;
	static int iReportBugProcessing = 0;

	if (refresh_gui_docking == 0 && !bBug_Reporting_Window)
	{
		//Make sure window is setup in docking space.
		ImGui::Begin("Bug Reporting System##BugReportingWindow", &bBug_Reporting_Window, iGenralWindowsFlags);
		ImGui::End();
	}

	static bool bLastBugStatus = false;
	if (bLastBugStatus != bBug_Reporting_Window || bBug_RefreshBugList ==true)
	{
		//Window just open , reset variables.
		strcpy(cBugTitle, "");
		strcpy(cBugDescription, "");
		bLastBugStatus = bBug_Reporting_Window;
		if (sBugList.size() == 0 || bBug_RefreshBugList==true)
		{
			GetBugReport();
		}
		iReportBugProcessing = 0;
		bBug_RefreshBugList = false;
	}

	if (!bBug_Reporting_Window)
		return;

	int wflags = ImGuiTreeNodeFlags_DefaultOpen;

	if (ImGui::Begin("Bug Reporting System##BugReportingWindow", &bBug_Reporting_Window, iGenralWindowsFlags) == true)
	{
		if (g_bBugTrackerConnected == false)
		{
			if (ImGui::StyleCollapsingHeader("Help With Bugs", wflags))
			{
				ImGui::Indent(10);
				ImGui::PushItemWidth(-10);
				ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));
				ImGui::TextWrapped("This Bug Tracker System enables you to submit any bugs you find, creates a record of the bugs you have raised and shows you updates when the bugs you have identified are dealt with.");
				ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));
				ImGui::TextWrapped("Click the button below to link your account at the TheGameCreators with your GitHub account.");
				ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));
				if (ImGui::StyleButton("Link Your GitHub Account", ImVec2(ImGui::GetContentRegionAvail().x - 10.0f, 0.0f)))
				{
					ExecuteFile("https://www.thegamecreators.com/github/link", "", "", 0);
				}
				ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));
				ImGui::TextWrapped("If you do not have a GitHub account you can register for free by clicking the button below.");
				ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));
				if (ImGui::StyleButton("Create GitHub Account", ImVec2(ImGui::GetContentRegionAvail().x - 10.0f, 0.0f)))
				{
					ExecuteFile("http://github.com/", "", "", 0);
				}
				ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));
				ImGui::TextWrapped("If you do not have an account with TheGameCreators you can register for free by clicking the button below.");
				ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));
				if (ImGui::StyleButton("Create Account", ImVec2(ImGui::GetContentRegionAvail().x - 10.0f, 0.0f)))
				{
					ExecuteFile("https://www.thegamecreators.com/", "", "", 0);
				}
			}
		}
		else
		{
			if (ImGui::StyleCollapsingHeader("Help With Bugs", wflags))
			{
				ImGui::Indent(10);
				ImGui::PushItemWidth(-10);

				ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));

				ImGui::TextWrapped("If you have found a problem with any functionailty found in the software, you can report it to the development team.");
				ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));
				ImGui::TextWrapped("First, ensure this is not a feature request, which you can submit by clicking this button.");
				ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));
				if (ImGui::StyleButton("Feature Requests", ImVec2(ImGui::GetContentRegionAvail().x - 10.0f, 0.0f)))
				{
					ExecuteFile("https://github.com/TheGameCreators/GameGuruRepo/issues/new", "", "", 0);
				}
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("Click here to visit our issues board to make a feature request");
				ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));
				ImGui::TextWrapped("Also, ensure it is not a general request for help and advice, which you can get by visiting the forum here:");
				if (ImGui::StyleButton("GameGuru Forums", ImVec2(ImGui::GetContentRegionAvail().x - 10.0f, 0.0f)))
				{
					ExecuteFile("https://forum.game-guru.com/board/1", "", "", 0);
				}
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("Click here to visit the GameGuru forums");
				ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));
				ImGui::PopItemWidth();
				ImGui::Indent(-10);
			}

			if (ImGui::StyleCollapsingHeader("Report A Bug", wflags))
			{
				ImGui::Indent(10);
				ImGui::PushItemWidth(-10);
				ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));

				ImGui::TextCenter("Your Bug Title");
				ImGui::InputText("##NameBugReport", &cBugTitle[0], 256);
				if (!pref.iTurnOffEditboxTooltip && ImGui::IsItemHovered()) ImGui::SetTooltip("Please give your bug a name");
				if (ImGui::MaxIsItemFocused()) bImGuiGotFocus = true;

				//Add pencil
				ImGuiWindow* window = ImGui::GetCurrentWindow(); //PE: Add a pencil to all color gadgets.
				ID3D11ShaderResourceView* lpTexture = GetImagePointerView(TOOL_PENCIL);
				ImVec2 vDrawPos = { ImGui::GetCursorScreenPos().x + (ImGui::GetContentRegionAvail().x - 30.0f) ,ImGui::GetCursorScreenPos().y - (ImGui::GetFontSize()*1.5f) - 3.0f };
				if (lpTexture)
					window->DrawList->AddImage((ImTextureID)lpTexture, vDrawPos, vDrawPos + ImVec2(16, 16), ImVec2(0, 0), ImVec2(1, 1), ImGui::GetColorU32(ImVec4(1, 1, 1, 1)));

				ImGui::TextCenter("Your Bug Description");

				float TextHeight = ImGui::GetFontSize()*7.5f;
				float w = ImGui::CalcItemWidth();
				ImGui::InputTextMultiline("##cBugDescription", &cBugDescription[0], 4096, ImVec2(0, TextHeight), ImGuiInputTextFlags_CallbackAlways, CLB, &(w));
				if (!pref.iTurnOffEditboxTooltip && ImGui::IsItemHovered()) ImGui::SetTooltip("Please enter a detailed step by step description of how to reproduce this bug");
				if (ImGui::MaxIsItemFocused()) bImGuiGotFocus = true;

				//Add pencil
				window = ImGui::GetCurrentWindow(); //PE: Add a pencil to all color gadgets.
				lpTexture = GetImagePointerView(TOOL_PENCIL);
				vDrawPos = { ImGui::GetCursorScreenPos().x + (ImGui::GetContentRegionAvail().x - 30.0f) ,ImGui::GetCursorScreenPos().y - (TextHeight)-3.0f };
				if (lpTexture)
					window->DrawList->AddImage((ImTextureID)lpTexture, vDrawPos, vDrawPos + ImVec2(16, 16), ImVec2(0, 0), ImVec2(1, 1), ImGui::GetColorU32(ImVec4(1, 1, 1, 1)));

				//Need additional Server Side Code - GitHub will shortly allow attachments it is said
				//ImGui::Checkbox("Include current screenshot",&bIncludeScreenShot);
				//if (ImGui::IsItemHovered()) ImGui::SetTooltip("Tick if you wish to include a screenshot with your bug");

				ImGui::Checkbox("Include my system specs", &bIncludesystemSpecs);
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("Tick to include my own system specifications with the bug report");

				if (ImGui::StyleButton("Submit Bug Report", ImVec2(ImGui::GetContentRegionAvail().x - 10.0f, 0.0f)))
				{
					//Start processing bug report.
					iReportBugProcessing = 1;
				}
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("When you click submit, your bug will be reported to TGC");

				ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));
				ImGui::PopItemWidth();
				ImGui::Indent(-10);
			}
			if (ImGui::StyleCollapsingHeader("Track My Bugs", wflags))
			{
				ImGui::Indent(10);
				ImGui::PushItemWidth(-10);
				ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));

				// drop down filter modes
				const char* filtermodes[] = { "Your Outstanding Issues", "Your Completed Issues", "All Issues" };
				static int filter_current_type_selection = 0;
				if (ImGui::Combo("##combostaticIssuesFilter", &filter_current_type_selection, filtermodes, IM_ARRAYSIZE(filtermodes)))
				{
					// set filter mode = filter_current_type_selection
				}
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("Use filter dropdown to choose which issues to see");

				float ListBoxHeight = ImGui::GetFontSize()*16.0f;
				float fAvailableX = ImGui::GetContentRegionAvailWidth();

				if (ImGui::ListBoxHeader("##BugListBox", ImVec2(0, ListBoxHeight)) == true)
				{
					ImGui::Columns(3, "buglistcolumns3", false);  //false no border
					ImGui::SetColumnOffset(0, 0.0f);
					ImGui::SetColumnOffset(1, ImGui::GetFontSize()*4.0f);
					ImGui::SetColumnOffset(2, fAvailableX - (20.0f + 18.0f + ImGui::GetCurrentWindow()->ScrollbarSizes.x));
					ImGui::Indent(-2);
					for (auto item : sBugList)
					{
						if (item)
						{
							int iIcon = MEDIA_REFRESH; //Unknown
							if (item->iStatus == 1)
								iIcon = MEDIA_RECORD;
							if (item->iStatus == 2)
								iIcon = MEDIA_RECORDPROCESSING;
							if (item->iStatus == 3)
								iIcon = MEDIA_RECORDING;

							// operate filter
							if (filter_current_type_selection == 0 && item->iStatus == 3) continue;
							if (filter_current_type_selection == 1 && item->iStatus != 3) continue;

							char buffer[80];
							strcpy(buffer, item->cCreatedAt);

							ImGui::Text(buffer);

							ImGui::NextColumn();

							bool bIsSelected = false;
							if (ImGui::Selectable(item->cTitle, bIsSelected))
							{
								// get issue number
								char pThisIssueNumber[6];
								memcpy(pThisIssueNumber, item->cTitle + 1, 6);
								pThisIssueNumber[5] = 0;
								for (int n = 0; n < strlen(pThisIssueNumber); n++)
									if (pThisIssueNumber[n] == ' ') pThisIssueNumber[n] = 0;

								// assemble new URL to exact issue
								char pURLToIssue[1024];
								strcpy(pURLToIssue, "https://github.com/TheGameCreators/GameGuruRepo/issues/");
								strcat(pURLToIssue, pThisIssueNumber);

								// handle selection
								ExecuteFile(pURLToIssue, "", "");
							}
							if (ImGui::IsItemHovered())
							{
								ImGui::BeginTooltip();
								ImGui::Indent(10);
								ImGui::PushItemWidth(-10);
								ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));

								ImGui::Text("%s", item->cTitle);
								ImGui::Separator();
								ImGui::TextWrapped(item->cDescription);
								ImGui::Separator();

								ImGui::ImgBtn(iIcon, ImVec2(16, 16), ImVec4(0, 0, 0, 0), drawCol_normal, drawCol_hover, drawCol_Down, 0, 0, 0, 0, false, false);
								ImGui::SameLine();
								ImGui::Text(" Status: ");
								ImGui::SameLine();
								if (item->iStatus == 1)
									ImGui::Text("The bug has been submitted. ");
								if (item->iStatus == 2)
									ImGui::Text("We have been able to reproduce this. ");
								if (item->iStatus == 3)
									ImGui::Text("The bug has been fixed. ");

								ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));

								ImGui::PopItemWidth();
								ImGui::Indent(-10);
								ImGui::EndTooltip();
							}

							ImGui::NextColumn();

							ImGui::ImgBtn(iIcon, ImVec2(16, 16), ImVec4(0, 0, 0, 0), drawCol_normal, drawCol_hover, drawCol_Down, 0, 0, 0, 0, false, false);
							if (ImGui::IsItemHovered())
							{
								LPSTR pTooltip = "";
								if (item->iStatus == 1) pTooltip = "The bug has been submitted.";
								if (item->iStatus == 2)	pTooltip = "We have been able to reproduce this. ";
								if (item->iStatus == 3)	pTooltip = "The bug has been fixed. ";
								ImGui::SetTooltip("%s", pTooltip);
							}

							ImGui::NextColumn();
						}
					}
					ImGui::Columns(1);
					ImGui::Indent(2);
					ImGui::ListBoxFooter();
				}

				ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 3));
				ImGui::PopItemWidth();
				ImGui::Indent(-10);
			}

			if (ImGui::GetCurrentWindow()->ScrollbarSizes.x > 0)
			{
				//Hitting exactly at the botton could cause flicker, so add some additional lines when scrollbar on.
				ImGui::Text("");
				ImGui::Text("");
			}

			// bug submission process
			if (iReportBugProcessing > 0)
			{
				if (iReportBugProcessing == 1)
				{
					// Validate data before processing
					LPSTR pErrorMsg = "";
					if (strlen(cBugTitle) == 0) pErrorMsg = "Please enter your bug title";
					if (strlen(cBugDescription) == 0) pErrorMsg = "Please enter your bug description";
					if (strlen(pErrorMsg) == 0)
					{
						// remove any /n insertions from cBugDescription
						LPSTR pDst = cBugDescription;
						for (LPSTR pSrc = cBugDescription; pSrc <= cBugDescription + sizeof(cBugDescription); pSrc++)
						{
							if (pSrc > cBugDescription && *(pSrc - 1) == ' ' && *pSrc == '\n')
							{
								// we inserted this to get IMGUI to multi-line wrap! So now remove it
							}
							else
							{
								*pDst = *pSrc;
								pDst++;
							}
						}

						// first make a copy of desc
						strcpy(cBugDescriptionCopy, cBugDescription);

						// if ticked sys specs, add this now
						if (bIncludesystemSpecs == true)
						{
							// if no specs found
							char pSystemSpecs[1024];
							strcpy(pSystemSpecs, "DXDIAG required!");

							// delete any old report
							cstr pDXDiagReport = g.mydocumentsdir_s + "\\GameGuruApps\\GameGuruMAX\\dxdiagreport.txt";
							if (FileExist(pDXDiagReport.Get())) DeleteFileA(pDXDiagReport.Get());

							// copy dxdiag bat to mydocs
							cstr pDXDiagSrc = g.fpscrootdir_s + "\\dxdiagsystemspecs.bat";
							cstr pDXDiagSystemScan = g.mydocumentsdir_s + "\\GameGuruApps\\GameGuruMAX\\dxdiagsystemspecs.bat";
							CopyFileA(pDXDiagSrc.Get(), pDXDiagSystemScan.Get(), FALSE);

							// get system specs from users machine
							cstr pOldDir = GetDir();
							SetCurrentDirectoryA(g.mydocumentsdir_s.Get());
							SetCurrentDirectoryA("GameGuruApps");
							SetCurrentDirectoryA("GameGuruMAX");
							int iStatusValue = ExecuteFile("dxdiagsystemspecs.bat", "", "", 1, 1);
							SetDir(pOldDir.Get());

							// load in new report
							if (FileExist(pDXDiagReport.Get()))
							{
								// parse out the lines we are interested in
								strcpy(pSystemSpecs, "");
								std::vector<cstr> dxdiagreport;
								LoadArray(pDXDiagReport.Get(), dxdiagreport);
								for (int line = 0; line < dxdiagreport.size(); line++)
								{
									LPSTR pThisLine = dxdiagreport[line].Get();
									for (int iFields = 0; iFields < 10; iFields++)
									{
										LPSTR pLabel = NULL;
										if (iFields == 0) pLabel = "Operating System: ";
										if (iFields == 1) pLabel = "Language: ";
										if (iFields == 2) pLabel = "System Model: ";
										if (iFields == 3) pLabel = "Processor: ";
										if (iFields == 4) pLabel = "Memory: ";
										if (iFields == 5) pLabel = "Windows Dir: ";
										if (iFields == 6) pLabel = "DirectX Version: ";
										if (iFields == 7) pLabel = "Card name: ";
										if (iFields == 8) pLabel = "Dedicated Memory: ";
										if (iFields == 9) pLabel = "Current Mode: ";
										LPSTR pFound = strstr(pThisLine, pLabel);
										if (pFound)
										{
											// exclude if characters found preceding the label
											bool bFoundChars = false;
											for (LPSTR pPtr = pFound - 1; pPtr > pThisLine; pPtr--)
												if (pPtr >= pThisLine && *pPtr != ' ')
													bFoundChars = true;
											if (bFoundChars == true) continue;

											// get to field data
											pFound += strlen(pLabel);

											// add to system specs
											strcat(pSystemSpecs, pLabel);
											strcat(pSystemSpecs, pFound);
											strcat(pSystemSpecs, "\n");
										}
									}
								}
								dxdiagreport.clear();
							}

							strcat(cBugDescription, "\n");
							strcat(cBugDescription, "\n");
							strcat(cBugDescription, "System Specs:\n");
							strcat(cBugDescription, pSystemSpecs);
						}

						// submit bug report to server
						// Create Issue
						// URL : https://www.thegamecreators.com/api/github/issues/create
						// Method: POST
						// Parameters :
						// title - This is the title of the issue and is required
						// body - This is the body text of the issue and is required, it does not support HTML but does support the special markup that GitHub uses for issues if you want to dead up on that here is the documentation https ://guides.github.com/features/mastering-markdown/ it will allow you to do things like embed images and make text bold so worth adding
						char m_szPostData[1024];
						memset(m_szPostData, 0, sizeof(m_szPostData));
						strcpy(m_szPostData, "title=");
						strcat(m_szPostData, cBugTitle);
						strcat(m_szPostData, "&body=");
						strcat(m_szPostData, cBugDescription);
						strcat(m_szPostData, "&labels[0]=max");
						strcat(m_szPostData, "&labels[1]=bug");
						DWORD dwDataReturnedSize = 0;
						LPSTR pDataReturned = NULL;
						char szAuthHeader[1024];
						sprintf(szAuthHeader, "public-api-auth-token: %d:%s", g_iUserID, g_sHashToken.Get());
						UINT iError = OpenURLForGETPOST("www.thegamecreators.com", &pDataReturned, &dwDataReturnedSize, szAuthHeader, m_szPostData, "POST", "/api/github/issues/create");
						if (iError <= 0 && *pDataReturned != 0 && strchr(pDataReturned, '{') != 0)
						{
							// to know when the data ends
							LPSTR pEndOfReturnedData = pDataReturned + dwDataReturnedSize;

							// error or success
							char pSuccessMarker[1024];
							strcpy(pSuccessMarker, Chr(34));
							strcat(pSuccessMarker, "success");
							strcat(pSuccessMarker, Chr(34));
							if (strstr(pDataReturned, pSuccessMarker) != NULL)
							{
								// success - all okay
								iReportBugProcessing++;
							}
							else
							{
								// failed to submit
								pErrorMsg = pDataReturned;
							}
						}

						// restore desc as we dont need system spec in local bug list item
						strcpy(cBugDescription, cBugDescriptionCopy);
					}
					// report error if found
					if (strlen(pErrorMsg) > 0)
					{
						MessageBoxA(NULL, pErrorMsg, "Bug Tracker Error", MB_OK);
						iReportBugProcessing = 0;
					}
				}
				if (iReportBugProcessing == 2)
				{
					// Do screenshot here (if supported)
					// next stage
					iReportBugProcessing++;
				}
				if (iReportBugProcessing == 3)
				{
					// Display "Bug has successful been added, Thanks"
					MessageBoxA(NULL, "Bug has successful been added", "Bug Tracking System", MB_OK);

					// just refresh bug list
					bBug_RefreshBugList = true;

					// End processing.
					iReportBugProcessing = 0;
				}
			}
		}

		// ensure IMGUI focus maintained
		ImRect bbwin(ImGui::GetWindowPos(), ImGui::GetWindowPos() + ImGui::GetWindowSize());
		if (ImGui::IsMouseHoveringRect(bbwin.Min, bbwin.Max))
		{
			bImGuiGotFocus = true;
		}
		if (ImGui::IsAnyItemFocused())
		{
			bImGuiGotFocus = true;
		}

		ImGui::End();
	}
}

LPSTR GetCommentFromIssue( LPSTR pIssueNumber )
{
	// string to return
	LPSTR pCommentToReturn = NULL;

	// get comment from server
	char m_szGETURL[1024];
	strcpy(m_szGETURL, "/api/github/issue/");
	strcat(m_szGETURL, pIssueNumber);
	strcat(m_szGETURL, "/comments?");
	strcat(m_szGETURL, "page=1"); // page number starting with 1
	DWORD dwDataReturnedSize = 0;
	LPSTR pDataReturned = NULL;
	char szAuthHeader[1024];
	sprintf(szAuthHeader, "public-api-auth-token: %d:%s", g_iUserID, g_sHashToken.Get());
	UINT iError = OpenURLForGETPOST("www.thegamecreators.com", &pDataReturned, &dwDataReturnedSize, szAuthHeader, NULL, "GET", m_szGETURL);
	if (iError <= 0 && *pDataReturned != 0 && strchr(pDataReturned, '{') != 0)
	{
		// to know when the data ends
		LPSTR pEndOfReturnedData = pDataReturned + dwDataReturnedSize;

		// create string to return 
		pCommentToReturn = new char[dwDataReturnedSize+1];
		memset(pCommentToReturn, 0, dwDataReturnedSize);

		// now thin this out to only show comment bodies
		char pSpeechMark[2];
		strcpy(pSpeechMark, Chr(34));
		char pBodyMarker[1024];
		strcpy(pBodyMarker, pSpeechMark);
		strcat(pBodyMarker, "body");
		strcat(pBodyMarker, pSpeechMark);
		strcat(pBodyMarker, ":");
		strcat(pBodyMarker, pSpeechMark);
		LPSTR pBodyPtr = strstr(pDataReturned, pBodyMarker);
		if (pBodyPtr)
		{
			pBodyPtr += strlen(pBodyMarker);
			LPSTR pBodyEndPtr = strstr(pBodyPtr, pSpeechMark);
			if (pBodyEndPtr)
			{
				int iOneCommentSize = pBodyEndPtr - pBodyPtr;
				LPSTR pOneComment = new char[iOneCommentSize + 1];
				memcpy(pOneComment, pBodyPtr, iOneCommentSize);
				pOneComment[iOneCommentSize] = 0;
				strcat(pCommentToReturn, "Comment: ");
				strcat(pCommentToReturn, pOneComment);
				strcat(pCommentToReturn, "\n");
				delete pOneComment;
			}
		}
	}

	// free data returned from GET call
	if (pDataReturned)
	{
		delete pDataReturned;
		pDataReturned = NULL;
	}

	// success, return
	return pCommentToReturn;
}

void GetBugReport(void)
{
	// always assume link is not established (link confirmed below)
	g_bBugTrackerConnected = false;

	// clear old bug list
	if (sBugList.size() > 0)
	{
		//Free old list.
		for (int i = 0; i < sBugList.size(); i++)
			SAFE_DELETE(sBugList[i]);
	}
	sBugList.clear();

}


ImVec2 Convert3DTo2D(float x, float y, float z)
{
	// take wicked viewproj and convert 3D XYZ to screenspace XY
	XMFLOAT3 thisPos = XMFLOAT3(x, y, z);
	XMVECTOR vecPos = XMLoadFloat3(&thisPos);
	wiCanvas canvas = master.masterrenderer;
	canvas.dpi = 96.0f; //PE: Always use default dpi. 2D coords is not using dpi scaling.
	float screenW = canvas.GetLogicalWidth();
	float screenH = canvas.GetLogicalHeight();
	wiScene::CameraComponent &camera = wiScene::GetCamera();
	XMMATRIX V = camera.GetView();
	XMMATRIX P = camera.GetProjection();
	XMMATRIX W = XMMatrixIdentity();
	XMVECTOR screen2D = XMVector3Project(vecPos, 0, 0, screenW, screenH, 0.0f, 1.0f, P, V, W);
	if (screen2D.m128_f32[2] >= 0.0f )
		return(ImVec2(screen2D.m128_f32[0], screen2D.m128_f32[1]));
	else
		return(ImVec2(-999999,-999999));
}

void GetRubberbandLowHighValues(void)
{
	//PE: Find clipping rect.
	float fLowX = 999999.0, fLowY = 999999.0, fLowZ = 999999.0;
	float fHighX = -999999.0, fHighY = -999999.0, fHighZ = -999999.0;
	for (int i = 0; i < (int)g.entityrubberbandlist.size(); i++)
	{
		int e = g.entityrubberbandlist[i].e;
		if (e > 0)
		{
			//Think i need the bounding box here.
			AABB aabb;
			sObject* pObject = GetObjectData(t.entityelement[e].obj);
			if (pObject && ObjectExist(t.entityelement[e].obj))
			{
				GGMATRIX matARotation;
				GGVECTOR3 box1;
				GGMATRIX matRotateX, matRotateY, matRotateZ;
				if (pObject->position.bFreeFlightRotation)
				{
					matARotation = pObject->position.matFreeFlightRotate;
				}
				else
				{
					//ZXY
					GGMatrixRotationX(&matRotateX, GGToRadian(pObject->position.vecRotate.x));	// x rotation
					GGMatrixRotationY(&matRotateY, GGToRadian(pObject->position.vecRotate.y));	// y rotation
					GGMatrixRotationZ(&matRotateZ, GGToRadian(pObject->position.vecRotate.z));	// z rotation
					matARotation = matRotateX * matRotateY * matRotateZ;
				}
				if (pObject->position.bApplyPivot)
				{
					matARotation *= pObject->position.matPivot;
				}

				aabb._min.x = pObject->collision.vecMin.x;
				aabb._min.y = pObject->collision.vecMin.y;
				aabb._min.z = pObject->collision.vecMin.z;
				aabb._max.x = pObject->collision.vecMax.x;
				aabb._max.y = pObject->collision.vecMax.y;
				aabb._max.z = pObject->collision.vecMax.z;

				box1.x = aabb._min.x; box1.y = aabb._min.y; box1.z = aabb._min.z;
				GGVec3TransformCoord(&box1, &box1, &matARotation);
				aabb._min.x = box1.x; aabb._min.y = box1.y; aabb._min.z = box1.z;
				box1.x = aabb._max.x; box1.y = aabb._max.y; box1.z = aabb._max.z;
				GGVec3TransformCoord(&box1, &box1, &matARotation);
				aabb._max.x = box1.x; aabb._max.y = box1.y; aabb._max.z = box1.z;

				aabb._min.x = (aabb._min.x * pObject->position.vecScale.x);
				aabb._min.y = (aabb._min.y * pObject->position.vecScale.y);
				aabb._min.z = (aabb._min.z * pObject->position.vecScale.z);
				aabb._max.x = (aabb._max.x * pObject->position.vecScale.x);
				aabb._max.y = (aabb._max.y * pObject->position.vecScale.y);
				aabb._max.z = (aabb._max.z * pObject->position.vecScale.z);

				if (t.entityelement[e].x + aabb._min.x < fLowX) fLowX = t.entityelement[e].x + aabb._min.x;
				if (t.entityelement[e].x + aabb._min.x > fHighX) fHighX = t.entityelement[e].x + aabb._min.x;
				if (t.entityelement[e].y + aabb._min.y < fLowY) fLowY = t.entityelement[e].y + aabb._min.y;
				if (t.entityelement[e].y + aabb._min.y > fHighY) fHighY = t.entityelement[e].y + aabb._min.y;
				if (t.entityelement[e].z + aabb._min.z < fLowZ) fLowZ = t.entityelement[e].z + aabb._min.z;
				if (t.entityelement[e].z + aabb._min.z > fHighZ) fHighZ = t.entityelement[e].z + aabb._min.z;
				if (t.entityelement[e].x + aabb._max.x < fLowX) fLowX = t.entityelement[e].x + aabb._max.x;
				if (t.entityelement[e].x + aabb._max.x > fHighX) fHighX = t.entityelement[e].x + aabb._max.x;
				if (t.entityelement[e].y + aabb._max.y < fLowY) fLowY = t.entityelement[e].y + aabb._max.y;
				if (t.entityelement[e].y + aabb._max.y > fHighY) fHighY = t.entityelement[e].y + aabb._max.y;
				if (t.entityelement[e].z + aabb._max.z < fLowZ) fLowZ = t.entityelement[e].z + aabb._max.z;
				if (t.entityelement[e].z + aabb._max.z > fHighZ) fHighZ = t.entityelement[e].z + aabb._max.z;

			}

			if (t.entityelement[e].x < fLowX) fLowX = t.entityelement[e].x;
			if (t.entityelement[e].y < fLowY) fLowY = t.entityelement[e].y;
			if (t.entityelement[e].z < fLowZ) fLowZ = t.entityelement[e].z;
			if (t.entityelement[e].x > fHighX) fHighX = t.entityelement[e].x;
			if (t.entityelement[e].y > fHighY) fHighY = t.entityelement[e].y;
			if (t.entityelement[e].z > fHighZ) fHighZ = t.entityelement[e].z;
		}
	}
	//PE: Convert fLowX to screencords.
	if (fHighX > -999998.0 && fLowX < 999998.0)
	{
		ImVec2 vLowValue = Convert3DTo2D(fLowX, fLowY, fLowZ);
		ImVec2 vHighValue = Convert3DTo2D(fHighX, fHighY, fHighZ);
		if (vLowValue.x < vHighValue.x)
		{
			fLastRubberBandX1 = vLowValue.x;
			fLastRubberBandX2 = vHighValue.x;
		}
		else
		{
			fLastRubberBandX2 = vLowValue.x;
			fLastRubberBandX1 = vHighValue.x;
		}
		if (vLowValue.y < vHighValue.y)
		{
			fLastRubberBandY1 = vLowValue.y;
			fLastRubberBandY2 = vHighValue.y;
		}
		else
		{
			fLastRubberBandY2 = vLowValue.y;
			fLastRubberBandY1 = vHighValue.y;
		}

		//Expand Y.
		fLastRubberBandY1 *= 0.8;
		fLastRubberBandY2 *= 1.2;

		//Adjust to fit better.
		fLastRubberBandX1 -= 40.f;
		fLastRubberBandY1 -= 40.f;
		fLastRubberBandX2 += 40.f;
		fLastRubberBandY2 += 40.f;
	}
}

void DragDrop_DeleteEntityCursor(void)
{
	// prevent deleting them, and instead instruct user to ungroup the objects first
	bool bContinueWithDelete = true; //PE: Changed to true , we should be able to delete normal objects.
	if (current_selected_group >= 0 && vEntityGroupList[current_selected_group][0].iGroupID != -1)
	{
		// do not delete 'key' objects that are part of group
		strcpy(cTriggerMessage, "Cannot delete a parent group. First ungroup objects, then you can delete them all.");
		bTriggerMessage = true;
		bContinueWithDelete = false;
	}
	if (bContinueWithDelete == true)
	{
		//PE: Delete entity cursor.
		//Delete any associated waypoint/trigger zone
		t.waypointindex = t.grideleprof.trigger.waypointzoneindex;
		if (t.waypointindex > 0)
		{
			t.w = t.waypoint[t.waypointindex].start;
			waypoint_delete();
		}
		t.grideleprof.trigger.waypointzoneindex = 0;
		//  delete grid entity object and reset
		t.gridentitydelete = 0;
		if (t.gridentityobj > 0)
		{
			DeleteObject(t.gridentityobj);
			t.gridentityobj = 0;
		}
		t.refreshgrideditcursor = 1;
		t.gridentity = 0;
		t.gridentityposoffground = 0;
		t.gridentityusingsoftauto = 0;
		t.gridentitysurfacesnap = 1 - g.gdisablesurfacesnap;
		// MAX handles its own positioning system
		t.gridentityautofind = 0;
		t.inputsys.dragoffsetx_f = 0;
		t.inputsys.dragoffsety_f = 0;
		editor_refreshentitycursor();
		t.widget.pickedObject = 0;

		bool bDisableRubberBandMoving = false;
		if (current_selected_group >= 0 && group_editing_on)
		{
			bDisableRubberBandMoving = true;
		}
		bDraggingActive = false;
		if (!bDisableRubberBandMoving)
		{
			// if rubberband selection, delete all in selection
			gridedit_deleteentityrubberbandfrommap();
		}
		// flag also used to restore highlighting behavior
		t.gridentityextractedindex = 0;

		// when place down, ensure waypoint not affected until release mouse button
		t.mclickpressed = 1;
		t.selstage = 1;
	}
}


void DragDrop_CheckTrashcanDrop(ImRect bb)
{
	if (pref.iEnableDragDropEntityMode && !bDraggingActive)
	{
		//Allow Trash drag drop, even when not dragging.
		//Reverse action, so you need to release mouse to delete, to prevent accidental deleting.
		if (t.gridentity != 0 || t.gridentityobj != 0)
		{
			if (ImGui::IsMouseHoveringRect(bb.Min, bb.Max, false))
			{
				if (!ImGui::IsMouseDown(0))
				{
					ImGui::BeginTooltip();
					ImGui::ImgBtn(TOOL_TRASHCAN, ImVec2(48, 48), ImVec4(1.0, 1.0, 1.0, 0.0), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(0.8, 0.8, 0.8, 0.8), ImVec4(0.8, 0.8, 0.8, 0.8), 0, 0, 0, 0, false, false, false, false, false, bBoostIconColors);
					ImGui::EndTooltip();
					bTrashcanIconActive = true;
				}
				else
				{
					DragDrop_DeleteEntityCursor();
				}
			}
		}
	}
	if (pref.iEnableDragDropEntityMode && bDraggingActive)
	{
		if (t.gridentity != 0 || t.gridentityobj != 0)
		{
			if (ImGui::IsMouseHoveringRect(bb.Min, bb.Max, false))
			{
				if (ImGui::IsMouseDown(0))
				{
					//ImGui::SetT   ooltip("%s", "Remove");
					ImGui::BeginTooltip();
					ImGui::ImgBtn(TOOL_TRASHCAN, ImVec2(48, 48), ImVec4(1.0, 1.0, 1.0, 0.0), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(0.8, 0.8, 0.8, 0.8), ImVec4(0.8, 0.8, 0.8, 0.8), 0, 0, 0, 0, false, false, false, false,false, bBoostIconColors);
					ImGui::EndTooltip();
					bTrashcanIconActive = true;
				}
				else {
					DragDrop_DeleteEntityCursor();
				}
			}
		}
	}
}

void DragCameraMovement(void)
{
	#ifdef DIRECTIONALCAMERASCROLLING
	// this is a nice method, but probably not ideal for everyone (and you can easily overshoot)
	// so decided to go with the pan method which has more finer controls for camera management
	static int iActivateCount = 0;
	if (pref.iDragCameraMovement && t.ebe.on == 0 && t.inputsys.xmouse != 500000  && t.grideditselect == 5 && t.gridentity == 0 && t.widget.activeObject == 0 && t.inputsys.keyshift == 0)
	{
		static ImVec2 vStartPos = { 0,0 };
		if (ImGui::IsMouseDown(0))
		{
			if (iActivateCount < 5) //PE: Give priority to object selection.
			{
				iActivateCount++;
				return;
			}
			static float fOldX = 0.0f, fOldY = 0.0f;
			if (!bDragCameraActive)
			{
				vStartPos = ImGui::GetMousePos();
				bDragCameraActive = true;
				fOldX = 0.0f;
			}
			ImVec2 vPos = ImGui::GetMousePos();
			ImVec2 vDiff = (vStartPos - vPos) * 0.60;
			//Draw line.
			{
				ImGuiViewport* mainviewport = ImGui::GetMainViewport();
				if (mainviewport)
				{
					ImDrawList* dl = ImGui::GetForegroundDrawList(mainviewport);
					if (dl)
					{
						dl->AddLine(vStartPos, vPos, ImGui::GetColorU32(ImVec4(1.0, 1.0, 1.0, 0.8)), 3.0f);
					}
				}
			}

			GGVECTOR3 vCamPos = GetCameraPosition();
			float fMoveSpeed = 8192.0f;
			float fCamDistance = (vCamPos.y*2.0f);
			if (fCamDistance < 50.0f) fCamDistance = 50.0f;

			vDiff = vDiff / fMoveSpeed * fCamDistance;

			float fmove = ImLerp(vDiff.x, fOldX, 0.25f);
			fmove *= g.timeelapsed_f;
			MoveCameraLeft(0, fmove);

			t.cx_f = t.editorfreeflight.c.x_f = GetCameraPosition().x;
			t.editorfreeflight.c.y_f = GetCameraPosition().y;
			t.cy_f = t.editorfreeflight.c.z_f = GetCameraPosition().z;
			fOldX = fmove;

			fmove = ImLerp(vDiff.y, fOldY, 0.25f);
			fmove *= g.timeelapsed_f;
			MoveCameraUp(0, fmove);

			t.cx_f = t.editorfreeflight.c.x_f = GetCameraPosition().x;
			t.editorfreeflight.c.y_f = vCamPos.y;
			t.cy_f = t.editorfreeflight.c.z_f = GetCameraPosition().z;
			fOldY = fmove;


		}
		else
		{
			bDragCameraActive = false;
			iActivateCount = 0;
		}
	}
	else
	{
		iActivateCount = 0;
	}
	#endif
}

void MouseLeftDragXZPanning(void)
{
	static int iActivateCount = 0;
	static bool bPanningActive = false;
	if (1)
	{
		static ImVec2 vStartPos = { 0,0 };
		static bool bRestoreMouseAfterXZPan = false;
		bool bOkayToGo = false;
		static bool bOverLockedObject = false;
		if (bPanningActive == true) bOkayToGo = true;
		if (t.onedrag == 0 && bPanningActive == false && pref.iDragCameraMovement && t.ebe.on == 0 && t.inputsys.xmouse != 500000 && t.grideditselect == 5 && t.gridentity == 0 && t.widget.activeObject == 0 && t.inputsys.keyshift == 0 && t.inputsys.keycontrol == 0 && bDotObjectDragging==false ) bOkayToGo = true;
		if(bOverLockedObject) bOkayToGo = false;
		if (ImGui::IsMouseDown(0) && bOkayToGo == true)
		{
			//PE: Give priority to object selection
			if (iActivateCount < 5) 
			{
				iActivateCount++;
				return;
			}

			//PE: Must call it each frame.
			ImGui::SetMouseCursor(ImGuiMouseCursor_Pan);

			// New mouse drag camera system relies on actual ray casts
			// to terrain to get exact XZ coordinates for the drag
			static float fTerrainLastHitX = 0.0f;
			static float fTerrainLastHitZ = 0.0f;
			float fTerrainHitX, fTerrainHitY, fTerrainHitZ;
			//PE: Bug fix , if locked object is under cursor terrain pan is activated.
			bool bLockedObject = false;
			if (WickedCall_GetPick(&fTerrainHitX, &fTerrainHitY, &fTerrainHitZ, NULL, NULL, NULL, NULL, GGRENDERLAYERS_NORMAL | GGRENDERLAYERS_TERRAIN) == true)
			{
				if (g_hovered_pobject != NULL)
				{
					bLockedObject = true;
					bOverLockedObject = true;
					bRestoreMouseAfterXZPan = false;
					ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
				}
				if (!bLockedObject)
				{
					if (bPanningActive == false)
					{
						bPanningActive = true;
						bDragCameraActive = true;
						fTerrainLastHitX = fTerrainHitX;
						fTerrainLastHitZ = fTerrainHitZ;
						if (bRestoreMouseAfterXZPan == false && ImGui::GetMouseCursor() == ImGuiMouseCursor_Arrow)
						{
							bRestoreMouseAfterXZPan = true;
							ImGui::SetMouseCursor(ImGuiMouseCursor_Pan);
						}
					}
					float fDifferenceX = fTerrainHitX - fTerrainLastHitX;
					float fDifferenceZ = fTerrainHitZ - fTerrainLastHitZ;
					//LB: prevent moving TOO fast such as grabbing terrain in extreme distance and shifting
					if (fabs(fDifferenceX) + fabs(fDifferenceZ) > 100.0f)
					{
						float fDist = sqrt((fabs(fDifferenceX) * fabs(fDifferenceX)) + (fabs(fDifferenceZ) * fabs(fDifferenceZ)));
						fDifferenceX = (fDifferenceX / fabs(fDist)) * 100.0f;
						fDifferenceZ = (fDifferenceZ / fabs(fDist)) * 100.0f;
					}
					fDifferenceX *= 1.8; //Move a bit faster.
					fDifferenceZ *= 1.8; //Move a bit faster.

					//PE: If we move in-out of water we can get some huge differences.
					if (fabs(fDifferenceX) + fabs(fDifferenceZ) < 6000.0f)
					{
						if (fabs(fDifferenceX) + fabs(fDifferenceZ) != 0.0f)
						{
							bPanningActive = false;
							// LB: also need to close this down or we end up locked in constant drag
							bDragCameraActive = false;
							if (bRestoreMouseAfterXZPan == true)
							{
								bRestoreMouseAfterXZPan = false;
								ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
							}
						}
						t.cx_f = t.editorfreeflight.c.x_f = (t.editorfreeflight.c.x_f - fDifferenceX);
						t.cy_f = t.editorfreeflight.c.z_f = (t.editorfreeflight.c.z_f - fDifferenceZ);
					}
				}
			}
			fTerrainLastHitX = fTerrainHitX;
			fTerrainLastHitZ = fTerrainHitZ;
		}
		else
		{
			if (bPanningActive == true)
			{
				bPanningActive = false;
				bDragCameraActive = false;
				if (bRestoreMouseAfterXZPan == true)
				{
					bRestoreMouseAfterXZPan = false;
					ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
				}
			}
			iActivateCount = 0;
			if (!ImGui::IsMouseDown(0))
				bOverLockedObject = false;
		}
	}
	else
	{
		iActivateCount = 0;
	}
}

void MouseWheelYPanning(void)
{
	static bool bPanningActive = false;
	static ImVec2 vStartPos = { 0,0 };
	static bool bRestoreMouseAfterYPan = false;
	if (ImGui::IsMouseDown(2))
	{
		if (bRestoreMouseAfterYPan ==false && ImGui::GetMouseCursor() == ImGuiMouseCursor_Arrow)
		{
			bRestoreMouseAfterYPan = true;
			ImGui::SetMouseCursor(ImGuiMouseCursor_Pan);
		}

		// New system for Camera Panning (Y) is to pan across the camera lens plane
		// so left/right is left and right for the camera operator, and up/down lifts and lowers the camera operator
	}
	else
	{
		bPanningActive = false;
		if (bRestoreMouseAfterYPan == true)
		{
			bRestoreMouseAfterYPan = false;
			ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
		}
	}
}

cStr cLastLibraryList = "";

//PE: TODO - g_LibraryFileList[] should be an array so we can have many on the screen at the same times.
//PE: TODO - get image width height from actual images so we dont lock it to widethumb format only.
//PE: This will be used for many different image selection types, Behavior,Images,Sound? ...
void SortFilesListForLibraryType()
{
	std::vector<sLibraryList> SortFileList;
	SortFileList.clear();
	SortFileList.resize(g_LibraryFileList.size());
	int iSort = 0;
	for (int i = 0; i < 3 ; i++)
	{
		for (int a = 0; a < g_LibraryFileList.size() ; a++)
		{
			if (g_LibraryFileList[a].iType == i)
			{
				SortFileList[iSort++] = g_LibraryFileList[a];
			}
		}
	}
	g_LibraryFileList = SortFileList;
}
void GetFilesListForLibrary(char *path, bool bCreateThumbs, int win, int iThumbWidth, int iThumbHeight, int SortType)
{
	if (!path) return;
	int uniqueId = 16000;

	if (cLastLibraryList != path)
	{
		cLastLibraryList = path;
		//Free any old thumbs.
		for (int n = 0; n < g_LibraryFileList.size(); n++)
		{
			if (g_LibraryFileList[n].iImage > 0 && ImageExist(g_LibraryFileList[n].iImage))
			{
				image_setlegacyimageloading(true);
				DeleteImage(g_LibraryFileList[n].iImage);
				image_setlegacyimageloading(false);
				g_LibraryFileList[n].iImage = 0;
			}
		}
		g_LibraryFileList.clear();
		cstr pOldDir = GetDir();
		SetDir(path);
		ChecklistForFiles();
		SetDir(pOldDir.Get());
		for (int c = 0; c < ChecklistQuantity(); c++)
		{
			cStr cFile = cStr(ChecklistString(1 + c));
			bool bIsImage = false;
			if (cFile != "." && cFile != "..")
			{
				cstr sTmp = ChecklistString(1 + c);
				if (pestrcasestr(sTmp.Get(), ".png"))
					bIsImage = true;
				if (!bIsImage && pestrcasestr(sTmp.Get(), ".jpg"))
					bIsImage = true;
				if (!bIsImage && pestrcasestr(sTmp.Get(), ".jpeg"))
					bIsImage = true;
				if (!bIsImage && pestrcasestr(sTmp.Get(), ".dds"))
					bIsImage = true;
				if (!bIsImage && pestrcasestr(sTmp.Get(), ".bmp"))
					bIsImage = true;
				if (bIsImage)
				{
					sLibraryList vTmp;
					vTmp.cDescription = "";
					vTmp.cName = cStr(sTmp);
					vTmp.cPath = cStr(path);
					vTmp.cFile = cStr(path) + cFile;
					vTmp.iType = 0;
					vTmp.cProject = "";
					vTmp.bProjectExists = false;

					//Check if we got a description file.
					std::string sDescFile = vTmp.cFile.Get();
					
					replaceAll(sDescFile, ".bmp", "");
					replaceAll(sDescFile, ".dds", "");
					replaceAll(sDescFile, ".jpg", "");
					replaceAll(sDescFile, ".jpeg", "");
					replaceAll(sDescFile, ".png", "");
					sDescFile = sDescFile + ".txt";
					std::ifstream t(sDescFile);
					std::string str((std::istreambuf_iterator<char>(t)),std::istreambuf_iterator<char>());
					if (str.length() > 0)
					{
						if (pestrcasestr(str.c_str(), "(rating:0)"))
						{
							replaceAll(str, "(rating:0)", "");
							vTmp.iType = 0;
						}
						else if (pestrcasestr(str.c_str(), "(rating:1)"))
						{
							replaceAll(str, "(rating:1)", "");
							vTmp.iType = 1;
						}
						else if (pestrcasestr(str.c_str(), "(rating:2)"))
						{
							replaceAll(str, "(rating:2)", "");
							vTmp.iType = 2;
						}

						char *find = (char *) pestrcasestr(str.c_str(), "(project:");
						if (find)
						{
							std::string remove = "(project:";
							find += 9;
							char project[512];
							if (strlen(find) < 500)
							{
								strcpy(project, find);
								char *find2 = (char *)pestrcasestr(project, ")");
								if (find2)
								{
									*find2 = 0;

									remove = remove + project;
									remove = remove + ")";
									replaceAll(str, remove, "");
									vTmp.cProject = project;

									// quick check to see if the folder exists
									char pProjFolderFile[MAX_PATH];
									sprintf(pProjFolderFile, "projectbank\\%s\\project%d.dat", vTmp.cProject.Get(), STORYBOARDVERSION);
									GG_GetRealPath(pProjFolderFile, false);
									if (FileExist(pProjFolderFile) == 0)
									{
										strcpy(pProjFolderFile, "projectbank\\");
										strcat(pProjFolderFile, vTmp.cProject.Get());
										strcat(pProjFolderFile, "\\project.dat");
										GG_GetRealPath(pProjFolderFile, false);
									}
									if(FileExist(pProjFolderFile)==1)
										vTmp.bProjectExists = true;
									else
										vTmp.bProjectExists = false;
								}
							}
						}

						//PE: The rest is the pure description.
						vTmp.cDescription = str.c_str();

					}

					g_LibraryFileList.push_back(vTmp);
				}
			}
		}

		if (bCreateThumbs)
		{
			SetDir(pOldDir.Get());
			for (int n = 0; n < g_LibraryFileList.size(); n++)
			{
				//PE: Skip cached thumbs for now.
				CreateBackBufferCacheName(g_LibraryFileList[n].cFile.Get(), iThumbWidth, iThumbHeight);
				g_LibraryFileList[n].iImage = 0;
				GG_SetWritablesToRoot(true);
				if (1==2 && FileExist(BackBufferCacheName.Get()))
				{
					SetMipmapNum(1); //PE: mipmaps not needed.
					image_setlegacyimageloading(true);
					if (FileExist(BackBufferCacheName.Get()))
					{
						LoadImage((char *)BackBufferCacheName.Get(), uniqueId + n);
					}
					if (ImageExist(uniqueId + n))
						g_LibraryFileList[n].iImage = uniqueId + n;
					image_setlegacyimageloading(false);
					SetMipmapNum(-1);
				}
				else
				{
					SetMipmapNum(1); //PE: mipmaps not needed.
					image_setlegacyimageloading(true);
					//Create thumb.
					LoadImageSize((char *)g_LibraryFileList[n].cFile.Get(), uniqueId + n, iThumbWidth, iThumbHeight);
					if (1==2 && ImageExist(uniqueId + n))
					{
						//Image
						//Save thumb.
						SaveImage(BackBufferCacheName.Get(), uniqueId + n);
						DeleteImage(uniqueId + n);
						LoadImage((char *)BackBufferCacheName.Get(), uniqueId + n); //Reload new and delete old.

						// resume thumbbank copy mode
						extern bool g_bThumbBankCopyMode;
						g_bThumbBankCopyMode = true;
					}
					if (ImageExist(uniqueId + n))
						g_LibraryFileList[n].iImage = uniqueId + n;
					image_setlegacyimageloading(false);
					SetMipmapNum(-1);
				}
				GG_SetWritablesToRoot(false);
				BackBufferCacheName = "";
			}
		}
		SetDir(pOldDir.Get());
		if (SortType == 1) SortFilesListForLibraryType();
	}
}

cstr ComboFilesListForLibrary(char *currentselection, int columns, int iFixedWidth, bool bRemoveExtension)
{
	cstr sReturnComboName = currentselection;
	cstr sDisplayName = sReturnComboName;
	if (bRemoveExtension)
	{
		char *find = (char *) pestrcasestr(sDisplayName.Get(), ".");
		if (find)
		{
			int iPos = find - sDisplayName.Get();
			if(iPos > 0 && iPos < 1024)
				sDisplayName = Left(sDisplayName.Get(), iPos);
		}
	}
	if (ImGui::BeginCombo("##ComboFilesListForLibrary", sDisplayName.Get(), ImGuiComboFlags_HeightLargest))
	{
		bool is_selected = (sReturnComboName == "None");
		if (ImGui::Selectable("None", is_selected)) {
			sReturnComboName = "None";
		}
		if (is_selected) ImGui::SetItemDefaultFocus();

		bool bUseImage = false;
		float fImageWidth = (ImGui::GetContentRegionAvailWidth() - 10.0f) * 0.5;
		if(iFixedWidth > 0)
			fImageWidth = iFixedWidth;
		fImageWidth -= 5.0f;
		float fImageRatio = fImageWidth / 512.0f;
		if(g_LibraryFileList.size() > 0 && g_LibraryFileList[0].iImage > 0 && ImageExist(g_LibraryFileList[0].iImage) )
			bUseImage = true;

		int columns_count = 0;
		for (int n = 0; n < g_LibraryFileList.size(); n++)
		{
			is_selected = (sReturnComboName == g_LibraryFileList[n].cName);

			if (bUseImage && g_LibraryFileList[n].iImage > 0 && ImageExist(g_LibraryFileList[n].iImage))
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
				if (ImGui::ImgBtn(g_LibraryFileList[n].iImage, iThumbSize, ImColor(255, 255, 255, 255), ImColor(255, 255, 255, 255), ImColor(255, 255, 255, 200), ImColor(255, 255, 255, 200), 0, 0, 0, 0, false, false,false))
				{
					sReturnComboName = g_LibraryFileList[n].cName;
					ImGui::CloseCurrentPopup();
				}
				if (columns > 0)
				{
					if (++columns_count < columns)
						ImGui::SameLine();
					else
						columns_count = 0;
				}
				else
				{
					//Default to 2 columns
					if (n % 2 == 0)
						ImGui::SameLine();
				}
			}
			else if (ImGui::Selectable(g_LibraryFileList[n].cName.Get(), is_selected)) {
				sReturnComboName = g_LibraryFileList[n].cName;
			}
			if (is_selected) ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}
	return sReturnComboName;
}

int GetImageIDFilesListForLibrary(cstr & currentselection)
{
	for (int n = 0; n < g_LibraryFileList.size(); n++)
	{
		if (currentselection == g_LibraryFileList[n].cName)
		{
			if (ImageExist(g_LibraryFileList[n].iImage))
				return(g_LibraryFileList[n].iImage);
			return(0);
		}
	}
	return(0);
}

int GetEntryFilesListForLibrary(cstr & currentselection)
{
	for (int n = 0; n < g_LibraryFileList.size(); n++)
	{
		if (currentselection == g_LibraryFileList[n].cName)
		{
			return(n);
		}
	}
	return(-1);
}

cstr ListboxFilesListForLibrary(char *currentselection, int columns, int iFixedWidth, bool bRemoveExtension, bool bIncludeNone, char *filter)
{
	cstr sReturnComboName = currentselection;
	if (columns <= 0) columns = 2; //Default to 2.

	bool bUseImage = false;
	float fImageWidth = (ImGui::GetContentRegionAvailWidth() - 10.0f - 18.0f ) / columns; //ImGui::GetCurrentWindow()->ScrollbarSizes.x
	if (iFixedWidth > 0)
		fImageWidth = iFixedWidth;
	fImageWidth -= 5.0f;
	fImageWidth -= 6.0f; //Due to now using Columns add spacing.

	float fImageRatio = fImageWidth / 512.0f;
	if (g_LibraryFileList.size() > 0 && g_LibraryFileList[0].iImage > 0 && ImageExist(g_LibraryFileList[0].iImage))
		bUseImage = true;

	float box_height = (((float)288.0*fImageRatio) * 2.0) + 6.0f;
	box_height += ImGui::GetFontSize() * 5.0;

	ImGui::BeginChild("##ListboxFilesListForLibrary", ImVec2(ImGui::GetContentRegionAvail().x - 4.0, box_height), false, iGenralWindowsFlags | ImGuiWindowFlags_NoSavedSettings);
	ImGui::Indent(2);
	if(1)
	{
		ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(0, 3));
		bool is_selected = false;

		ImGui::Columns(columns, "ListboxFilesListForLibrarycolumns", false);  //false no border

		int columns_count = 0;
		for (int n = 0; n < g_LibraryFileList.size(); n++)
		{
			is_selected = (sReturnComboName == g_LibraryFileList[n].cName);

			cstr sDisplayName = g_LibraryFileList[n].cName;
			if (bRemoveExtension)
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
			if (filter)
			{
				bVisible = false;
				cstr sfilter = filter;
				char *p = strtok(sfilter.Get(), ",");
				while (p)
				{
					if(p[0] == '*')
						bVisible = true;
					else if (pestrcasestr(g_LibraryFileList[n].cName.Get(), p))
						bVisible = true;
					p = strtok(NULL, ",");
				}
			}
			if (bVisible)
			{
				if (bUseImage && g_LibraryFileList[n].iImage > 0 && ImageExist(g_LibraryFileList[n].iImage))
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
					if (ImGui::ImgBtn(g_LibraryFileList[n].iImage, iThumbSize, ImColor(0, 0, 0, 0), ImColor(255, 255, 255, 255), ImColor(255, 255, 255, 200), ImColor(255, 255, 255, 200), 0, 0, 0, 0, false, false, false))
					{
						sReturnComboName = g_LibraryFileList[n].cName;
					}

					ImGui::Text(sDisplayName.Get());

					ImGui::NextColumn();
				}
				else if (ImGui::Selectable(sDisplayName.Get(), is_selected))
				{
					sReturnComboName = g_LibraryFileList[n].cName;
				}
				if (is_selected) ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::Columns(1);
	}
	ImGui::Indent(-2);
	ImGui::EndChild();
	return sReturnComboName;
}


