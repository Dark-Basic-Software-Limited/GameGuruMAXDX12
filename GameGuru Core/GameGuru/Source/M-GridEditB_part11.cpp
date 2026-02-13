void BeginDragDropFPE(char *fpe, int textureid, bool bToolTipActive, ImVec2 vISize)
{
	if (bWaitOnMouseRelease)
	{
		if (!ImGui::IsMouseDown(0))
			bWaitOnMouseRelease = false;
	}

	if (bToolTipActive && pref.iEnableDragDropEntityMode && !bWaitOnMouseRelease && t.gridentity == 0 && t.gridentityobj == 0 && ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
	{

		cstr find = fpe;

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
					if (pSearchFolder->m_pFirstFile) {
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

		if (bFound && foundfiles)
		{
			CloseDownEditorProperties();
			t.inputsys.constructselection = 0;

			foundfiles->m_dropptr = foundfiles;
			foundfiles->iAnimationFrom = 0;

			foundfiles->m_sFolder = sFpeName.c_str();
			ImGui::SetDragDropPayload("DND_MODEL_DROP_TARGET", foundfiles, sizeof(void *));
			ImGui::ImgBtn(textureid, vISize, drawCol_back, drawCol_normal, drawCol_hover, drawCol_Down, 0, 0, 0, 0, false);
			pDragDropFile = foundfiles;
			bReadyToDropEntity = false;
			iDragDropActive = 50;
			bDraggingActive = true;
			bDraggingActiveInitial = true;
			// LB: these can be uninitialised, but we need these filled so the plane can be under the cursor initially
			t.gridentityposx_f = t.inputsys.localx_f;
			t.gridentityposy_f = t.inputsys.localcurrentterrainheight_f;
			t.gridentityposz_f = t.inputsys.localy_f;
		}
		// ZJ: Moved this here, to prevent the icons being dragged in release, and assertion error in debug, when bFound is false 
		ImGui::EndDragDropSource();
	}
}


void DisplayFPEMedia(bool readonly, int entid, entityeleproftype *edit_grideleprof)
{
	int tflagtext = 0, tflagimage = 0; //PE: These is not used in VRTECH ?
	bool mediaactive[6] = { true,true,true,true,true,true };
	int iActiveMedia = 0;

	if (!edit_grideleprof)
	{
		edit_grideleprof = &t.grideleprof;
	}

	if (readonly)
	{
		ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
	}

	if (readonly)
	{
		if (edit_grideleprof->soundset_s.Len() <= 0)
		{
			mediaactive[0] = false;
			iActiveMedia++;
		}
		if (edit_grideleprof->soundset1_s.Len() <= 0)
		{
			mediaactive[1] = false;
			iActiveMedia++;
		}
		if (edit_grideleprof->soundset2_s.Len() <= 0)
		{
			mediaactive[2] = false;
			iActiveMedia++;
		}
		if (edit_grideleprof->soundset3_s.Len() <= 0)
		{
			mediaactive[3] = false;
			iActiveMedia++;
		}
		if (edit_grideleprof->soundset5_s.Len() <= 0)
		{
			mediaactive[4] = false;
			iActiveMedia++;
		}
		if (edit_grideleprof->soundset6_s.Len() <= 0)
		{
			mediaactive[5] = false;
			iActiveMedia++;
		}
	}
	
	// Sound
	if (t.tflagsound == 1 || t.tflagsoundset == 1 || tflagtext == 1 || tflagimage == 1)
	{
		cstr group_text;
		if (tflagtext == 1 || tflagimage == 1)
		{
			if (tflagtext == 1) group_text = "Text";
			if (tflagimage == 1) group_text = "Image";
		}
		else
		{
			group_text = "Media";
		}

		if (g.fpgcgenre == 1)
		{
			if (t.entityprofile[entid].ischaracter > 0)
			{
				if(mediaactive[0])
					edit_grideleprof->soundset_s = imgui_setpropertyfile2_v2(t.group, edit_grideleprof->soundset_s.Get(), "Sound0", t.strarr_s[254].Get(), "audiobank\\",readonly);
			}
			else
			{
				if (g.vrqcontrolmode != 0)
				{
					if (t.tflagsound == 1 && t.tflagsoundset != 1)
					{
						//PE: changed from 469 to 467 , should be sound0
						if(mediaactive[0])
							edit_grideleprof->soundset_s = imgui_setpropertyfile2_v2(t.group, edit_grideleprof->soundset_s.Get(), t.strarr_s[467].Get(), t.strarr_s[253].Get(), "audiobank\\",readonly);
					}
				}
				else
				{
					if (t.tflagsound == 1 && t.tflagsoundset != 1)
					{
						if (mediaactive[0])
							edit_grideleprof->soundset_s = imgui_setpropertyfile2_v2(t.group, edit_grideleprof->soundset_s.Get(), t.strarr_s[467].Get(), t.strarr_s[253].Get(), "audiobank\\",readonly);
					}
				}
				if (t.tflagsoundset == 1)
				{
					if (mediaactive[0])
						edit_grideleprof->soundset_s = imgui_setpropertyfile2_v2(t.group, edit_grideleprof->soundset_s.Get(), t.strarr_s[469].Get(), t.strarr_s[255].Get(), "audiobank\\voices\\",readonly);
				}
				if (tflagtext == 1)
				{
					if (mediaactive[0])
						edit_grideleprof->soundset_s = imgui_setpropertystring2_v2(t.group, edit_grideleprof->soundset_s.Get(), "Text to Appear", "Enter text to appear in-game",readonly);
				}
				if (tflagimage == 1)
				{
					if (mediaactive[0])
					{
						#define IMGFILEID (PROPERTIES_CACHE_ICONS+998)
						static cstr imgfile = "";
						static int imgfile_preview_id = 0;
						if (edit_grideleprof->soundset_s != imgfile)
						{
							//Load new image preview.
							imgfile_preview_id = 0;
							if (edit_grideleprof->soundset_s != "")
							{
								image_setlegacyimageloading(true);
								LoadImage((char *)edit_grideleprof->soundset_s.Get(), IMGFILEID);
								image_setlegacyimageloading(false);
								imgfile_preview_id = IMGFILEID;
								if (!GetImageExistEx(IMGFILEID))
								{
									imgfile_preview_id = 0;
								}
							}
							imgfile = edit_grideleprof->soundset_s;
						}

						edit_grideleprof->soundset_s = imgui_setpropertyfile2_v2(t.group, edit_grideleprof->soundset_s.Get(), "Image File", "Select image to appear in-game", "imagebank\\", readonly);

						if (imgfile_preview_id > 0 && GetImageExistEx(imgfile_preview_id))
						{
							float w = ImGui::GetContentRegionAvailWidth();
							float iwidth = w;
							float ImgW = ImageWidth(imgfile_preview_id);
							float ImgH = ImageHeight(imgfile_preview_id);
							float fHighRatio = ImgH / ImgW;
							ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((w*0.5) - (iwidth*0.5), 0.0f));
							ImGui::ImgBtn(imgfile_preview_id, ImVec2(iwidth - 18.0f, (iwidth - 18.0f) * fHighRatio), drawCol_back, drawCol_normal, drawCol_normal, drawCol_normal, -1, 0, 0, 0, true);
						}
					}
				}
			}

			if (t.tflagnosecond == 0)
			{
				if (t.tflagsound == 1 || t.tflagsoundset == 1)
				{
					//We got some missing translations.
					if (t.strarr_s[468] == "") t.strarr_s[468] = "Sound1";
					if (t.strarr_s[480] == "") t.strarr_s[480] = "Sound2";
					if (t.strarr_s[481] == "") t.strarr_s[481] = "Sound3";
					if (t.strarr_s[482] == "") t.strarr_s[482] = "Sound4";
					if (mediaactive[1])
						edit_grideleprof->soundset1_s = imgui_setpropertyfile2_v2(t.group, edit_grideleprof->soundset1_s.Get(), t.strarr_s[468].Get(), t.strarr_s[254].Get(), "audiobank\\",readonly);
					if (mediaactive[2])
						edit_grideleprof->soundset2_s = imgui_setpropertyfile2_v2(t.group, edit_grideleprof->soundset2_s.Get(), t.strarr_s[480].Get(), t.strarr_s[254].Get(), "audiobank\\",readonly);
					if (mediaactive[3])
						edit_grideleprof->soundset3_s = imgui_setpropertyfile2_v2(t.group, edit_grideleprof->soundset3_s.Get(), t.strarr_s[481].Get(), t.strarr_s[254].Get(), "audiobank\\",readonly);
					if (mediaactive[4])
						edit_grideleprof->soundset6_s = imgui_setpropertyfile2_v2(t.group, edit_grideleprof->soundset5_s.Get(), "Sound5", t.strarr_s[254].Get(), "audiobank\\", readonly);
					if (mediaactive[5])
						edit_grideleprof->soundset6_s = imgui_setpropertyfile2_v2(t.group, edit_grideleprof->soundset6_s.Get(), "Sound6", t.strarr_s[254].Get(), "audiobank\\", readonly);
				}
			}
		}
		else
		{
			if (t.tflagsoundset == 1)
			{
				if (mediaactive[0])
					edit_grideleprof->soundset_s = imgui_setpropertyfile2_v2(t.group, edit_grideleprof->soundset_s.Get(), t.strarr_s[469].Get(), t.strarr_s[255].Get(), "audiobank\\voices\\",readonly);
			}
			else
			{
				if (mediaactive[0])
					edit_grideleprof->soundset_s = imgui_setpropertyfile2_v2(t.group, edit_grideleprof->soundset_s.Get(), t.strarr_s[467].Get(), t.strarr_s[253].Get(), "audiobank\\",readonly); ++t.controlindex;
			}
			if (mediaactive[1])
				edit_grideleprof->soundset1_s = imgui_setpropertyfile2_v2(t.group, edit_grideleprof->soundset1_s.Get(), t.strarr_s[468].Get(), t.strarr_s[254].Get(), "audiobank\\",readonly); ++t.controlindex;
		}

	}

	// Video
	if (t.tflagvideo == 1)
	{
		//t.strarr_s[597].Get()
		if (mediaactive[1])
			edit_grideleprof->soundset1_s = imgui_setpropertyfile2_v2(t.group, edit_grideleprof->soundset1_s.Get(), "Video Slot", t.strarr_s[601].Get(), "videobank\\",readonly);
	}

	if (readonly)
	{
		ImGui::PopItemFlag();
		ImGui::PopStyleVar();
	}
}

void DisplayFPEPhysics(bool readonly, int entid, entityeleproftype *edit_grideleprof)
{
	if (readonly)
	{
		ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
	}

	if (!edit_grideleprof)
	{
		edit_grideleprof = &t.grideleprof;
	}

	//Physics
	cstr desc ="";
	if (t.entityprofile[entid].ismarker == 0 && t.entityprofile[entid].islightmarker == 0)
	{
		ImGui::PushItemWidth(-10);

		if (edit_grideleprof->physics != 1)  edit_grideleprof->physics = 0;

		bool btmp = edit_grideleprof->physics;
		ImGui::Checkbox("Object Uses Physics?", &btmp);
		edit_grideleprof->physics = btmp;
		desc = t.strarr_s[581];
		if (ImGui::IsItemHovered() && desc.Len() > 0)
		{
			//When using checkbox, change text.
			std::string newtext = desc.Get();
			replaceAll(newtext, "Set to YES", "If set");
			replaceAll(newtext, " to YES", "");
			ImGui::SetTooltip("%s", newtext.c_str());
		}

		btmp = edit_grideleprof->phyalways;
		ImGui::Checkbox("Always Active?", &btmp);
		edit_grideleprof->phyalways = btmp;
		desc = t.strarr_s[583];
		if (ImGui::IsItemHovered() && desc.Len() > 0)
		{
			//When using checkbox, change text.
			std::string newtext = desc.Get();
			replaceAll(newtext, "Set to YES", "If set");
			replaceAll(newtext, " to YES", "");
			ImGui::SetTooltip("%s", newtext.c_str());
		}

		ImGui::TextCenter("Weight of Object");
		desc = t.strarr_s[585];
		ImGui::MaxSliderInputInt("##weightphysics", &edit_grideleprof->phyweight, 0, 1000, desc.Get());
		
		ImGui::TextCenter("Object's Friction");
		desc = t.strarr_s[587];
		ImGui::MaxSliderInputInt("##frictionphysics", &edit_grideleprof->phyfriction, 0, 1000, desc.Get());
	
		if (t.tflagsimpler == 0)
		{
			btmp = edit_grideleprof->explodable;
			ImGui::Checkbox("Explodable Object?", &btmp);
			edit_grideleprof->explodable = btmp;
			desc = t.strarr_s[593];
			if (ImGui::IsItemHovered() && desc.Len() > 0)
			{
				//When using checkbox, change text.
				std::string newtext = desc.Get();
				replaceAll(newtext, "Set to YES", "If set");
				replaceAll(newtext, " to YES", "");
				ImGui::SetTooltip("%s", newtext.c_str());
			}

			// Explosion Properties
			ImGui::TextCenter("Explosion Damage");
			ImGui::MaxSliderInputInt("##damagephysics", &edit_grideleprof->explodedamage, 0, 1000, t.strarr_s[595].Get());
			ImGui::TextCenter("Explosion Height");
			ImGui::MaxSliderInputInt("##damagephysicsheight", &edit_grideleprof->explodeheight, 0, 100, "Set the optional height at which the explosion occurs above default object center");
		}

		ImGui::PopItemWidth();
	}

	if (readonly)
	{
		ImGui::PopItemFlag();
		ImGui::PopStyleVar();
	}
}

void gridedit_makelighthybrid ( void )
{
	// create a single object that has multiple meshes inside it to represent different light types
	LPSTR pHybridLightModel = "entitybank\\_markers\\hybridlight.dbo";
	if (FileExist(pHybridLightModel) == 0)
	{
		// load in point light
		LoadObject ("entitybank\\_markers\\light.dbo", g.tempobjectoffset+0);
		RotateLimb(g.tempobjectoffset + 0, 0, 90, 0, 0);

		// load in spot light
		LoadObject ("entitybank\\_markers\\spotlight.dbo", g.tempobjectoffset+1);
		RotateObject(g.tempobjectoffset + 1, -90, 0, 0);
		MakeMeshFromObject(g.tempobjectoffset + 1, g.tempobjectoffset + 1);

		// add spot to point
		sObject* pObject = GetObjectData(g.tempobjectoffset+0);
		int iFrameCount = pObject->iFrameCount;
		AddLimb(g.tempobjectoffset+0, iFrameCount, g.tempobjectoffset + 1);

		// remove unneeded resources
		DeleteMesh(g.tempobjectoffset + 1);
		DeleteObject(g.tempobjectoffset + 1);

		//PE: saved object only have 3 frames ?
		// save new hybrid light for use by all new light markers
		SaveObject (pHybridLightModel, g.tempobjectoffset);
	}
}

