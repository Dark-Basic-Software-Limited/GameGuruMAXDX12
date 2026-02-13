bool bPopModalOpenEntity = false;
void process_entity_library_v2(void)
{
	static int iStopVideoInNextFrame = 0;
	static int iStartVideoInNextFrame = 0;
	static int iVideoGetFirstFrame = 0;
	static int iVideoThumbID = 0;
	static cstr sVideoLoadName = "";

	if (iStopVideoInNextFrame > 0 && iVideoGetFirstFrame == 0)
	{
		if (AnimationExist(iStopVideoInNextFrame))
		{
			StopAnimation(iStopVideoInNextFrame);
			DeleteAnimation(iStopVideoInNextFrame);
		}
		SetVideoVolume(100.0);
		iStopVideoInNextFrame = 0;
	}

	if (iStartVideoInNextFrame && iVideoGetFirstFrame == 0)
	{
		iVideoThumbID = 0;
		for (int itl = 1; itl <= 32; itl++)
		{
			if (AnimationExist(itl) == 0) { iVideoThumbID = itl; break; }
		}
		if (LoadAnimation((char *)sVideoLoadName.Get(), iVideoThumbID, g.videoprecacheframes, 0, 1) == false)
		{
			iVideoThumbID = -999;
		}
		if (iVideoThumbID > 0)
		{
			PlaceAnimation(iVideoThumbID, -1, -1, -1, -1);
			//Try to get first frame.
			StopAnimation(iVideoThumbID);
			SetVideoVolume(100.0);
			PlayAnimation(iVideoThumbID);
			SetRenderAnimToImage(iVideoThumbID, true);

			UpdateAllAnimation();
			Sleep(50); //Sleep so we get a video texture in the next call.
			UpdateAllAnimation();
			iStartVideoInNextFrame = 0;
			PlayAnimation(iVideoThumbID);
			SetVideoVolume(100.0);
		}
	}

	static bool bLargePreview = false;
	bool bCheckGotoPreview = false;

	if (sGotoPreviewWithFile != "")
	{
		if (!bLargePreview)
		{
			bExternal_Entities_Window = true;
			iDisplayLibraryType = 0;
			iDisplayLibrarySubType = 0;
			bCheckGotoPreview = true;
		}
	}


	if (bExternal_Entities_Window)
	{
		//PE:As we can switch type on the fly, make sure to free object images when we switch.
		static int iOldDisplayLibraryType = -1;
		static int iOldDisplayLibrarySubType = -1;
		if (iDisplayLibraryType != iOldDisplayLibraryType || iDisplayLibrarySubType != iOldDisplayLibrarySubType)
		{
			iOldDisplayLibraryType = iDisplayLibraryType;
			iOldDisplayLibrarySubType = iDisplayLibrarySubType;
			FreeTempImageList();
			iRestoreEntidMaster = g.entidmaster; //Start over.
			bUpdateSearchSorting = true;
		}
		if (g_TempimageList.size() == 0)
		{
			g_TempimageList = g_imageList;
			iRestoreEntidMaster = g.entidmaster; //PE: Mark where we are.
		}
		//PE: Make sure we dont use all memory.
		//PE: 03-18-2021: Decreased from 50 to 20 as some users reported it used all there memory.
		if (!bLoopBackBuffer && g_imageList.size() > g_TempimageList.size() + 20)
		{
			//PE: After we cached 50 textures free all so we dont run out of mem. We might auto generate 1000 thumbs :)
			//PE: This is like allowing around 15 fpe with objects textures to be cached.
			FreeTempImageList();
			iRestoreEntidMaster = g.entidmaster; //Start over.
		}


		ImGuiWindowFlags ex_window_flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking;
		ImGuiIO& io = ImGui::GetIO();

		static bool bDoneOneThumbPerSync;
		static int iDeleteInNextUpdate = 0;
		static int uniqueId = 4000; //PE: Also used for imageID for previews.
		static int loaded_images = 0;
		static bool multi_selections = false;
		int multi_selections_count = 0;
		static int lf_multi_selections_count = 0;
		int olduniqueId = uniqueId;
		bool bReleaseIconsDynamic = false;

		//uniqueId (imageid) range goes from 4000-43000 MAX.
		uniqueId = 4000;
		if (iDisplayLibraryType == 2)
		{
			uniqueId += 6000;
		}
		if (iDisplayLibraryType == 1)
		{
			uniqueId += 12000;
		}
		if (iDisplayLibraryType == 3)
		{
			uniqueId += 18000;
		}
		if (iDisplayLibraryType == 4)
		{
			uniqueId += 24000;
		}
		if (iDisplayLibraryType == 5)
		{
			uniqueId += 30000;
		}

#ifdef DYNAMICLOADUNLOAD
		static int max_load_persync = 200; //First time only , changed later to 15
		bReleaseIconsDynamic = true;
#else
		int max_load_persync = 2000;
#endif

		if (iDisplayLibraryType == 3)
		{
			//Generating of video preview, could take some time so one per frame.
			max_load_persync = 1;
		}
		else if (iDisplayLibraryType == 2)
		{
			//Loading images takes to long, max one per sync. perhaps use preloader here ?
			max_load_persync = 1;
		}
		else if (iDisplayLibraryType == 4)
		{
			//Only Loading thumbs.
			max_load_persync = 10;
		}
		else if (iDisplayLibraryType == 5)
		{
			//Only Loading thumbs.
			max_load_persync = 10;
		}
		else
		{
			if (max_load_persync != 200)
				max_load_persync = 10;
		}

		int preview_count = 0;
		int media_icon_size = 64;
		int media_icon_size_y = 32;
		int thumb_x = 512;
		int thumb_y = 288;
		int iColumnsWidth = 110;

		bDoneOneThumbPerSync = false;

		if (iDeleteInNextUpdate > 0)
		{
			image_setlegacyimageloading(true);
			DeleteImage(iDeleteInNextUpdate);
			image_setlegacyimageloading(false);
			iDeleteInNextUpdate = 0;
		}

		time_t tCurrentTimeSec;
		time(&tCurrentTimeSec);

		static int iLargePreviewImageID = false;
		static cFolderItem::sFolderFiles * pPreviewFile = NULL;
		static bool bStartAnimation = false;
		static int iAnimationSet = 0;
		static bool bAnimationAll = true;
		static bool bLoopAnim = true;
		static int iGetTriangles = 0;
		static int iGetVertex = 0;
		static int iGetLodLevels = 0;
		static cstr cGetTextureSize;
		static int iLowestAnimFrame = 0, iHigestAnimFrame = 0;
		static cstr cCurrentBackDrop = "None";
		bool bImagesStillInImGuiQueue = false;

		//bUpdateSearchSorting = false; // cleared later so other funcs can set this to trigger
		static int current_sortby = 1;

		static cFolderItem::sFolderFiles * firstvisiblefile = NULL;
		static cFolderItem::sFolderFiles * scrolltofile = NULL;
		static cFolderItem::sFolderFiles * secondscrolltofile = NULL;
		static bool bScrollInNextFrame = false;
		firstvisiblefile = NULL;

		bool bAdvancedFPEFeatures = true;
		static bool bUpdateSearchAfterFPEScan = false;
		if (fpe_thread_in_progress())
		{
			bAdvancedFPEFeatures = false;
			bUpdateSearchAfterFPEScan = true;
		}
		else
		{
			if (bUpdateSearchAfterFPEScan)
			{
				bUpdateSearchAfterFPEScan = false;
				bUpdateSearchSorting = true;
			}
		}

		static char cCheckboxFilters[5][MAX_PATH];
		static bool bCheckboxFilters[5] = { true,true,true,true,true };

		//When changing media type , always refresh search results.

		if (iLastDisplayLibraryType != iDisplayLibraryType)
		{
			//Reset search
			iLastDisplayLibraryType = iDisplayLibraryType;
			bUpdateSearchSorting = true;
			bUpdateSearchScrollbar = true;
			strcpy(cSearchAllEntities[0], "");
			strcpy(cSearchAllEntities[1], "");
			strcpy(cSearchAllEntities[2], "");

			static bool bRestoreSearchOnLaunch = true;

			//PE: sStartLibrarySearchString can overwrite cRememberLastSearchObjects.
			if (iDisplayLibraryType == 0)
			{
				if (bRestoreSearchOnLaunch)
				{
					if(strlen(pref.cRememberLastSearchObjects) >  0)
						strcpy(cSearchAllEntities[0], pref.cRememberLastSearchObjects);
					for (int i = 0; i < 5; i++)
						bCheckboxFilters[i] = pref.iCheckboxFilters[i];

					bRestoreSearchOnLaunch = false;
				}
			}

			if (sStartLibrarySearchString != "")
			{
				strcpy(cSearchAllEntities[0], sStartLibrarySearchString.Get());
				sTriggerCategorySelect = sStartLibrarySearchString;
				seleted_tree_item = -1;
				sStartLibrarySearchString = "";
				bDisplayFavorite = false; //Disable favorite. so we see real search list.
			}
		}
		else
		{
			if (iDisplayLibraryType == 2)
			{
				//if image always use sStartLibrarySearchString.
				if (sStartLibrarySearchString != "")
				{
					strcpy(cSearchAllEntities[0], sStartLibrarySearchString.Get());
					sTriggerCategorySelect = sStartLibrarySearchString;
					seleted_tree_item = -1;
					sStartLibrarySearchString = "";
					bDisplayFavorite = false; //Disable favorite. so we see real search list.
				}
			}
		}

		//#########################################
		//#### Large preview window of objects ####
		//#########################################

		if (bLargePreview)
		{
			bImGuiGotFocus = true;
			bool bIsCCPObject = false;

			if (!bPopModalOpenEntity)
			{
				ImGui::OpenPopup("##Preview##DisModalMode");
				ImGui::SetNextWindowSize(ImVec2(1320.0f, 0), ImGuiCond_Always);
				bStartAnimation = false;
				bAnimationAll = true;
				iAnimationSet = 0;
				iGetTriangles = 0;
				iGetVertex = 0;
				iGetLodLevels = 0;
				sObject* pObject = g_ObjectList[BackBufferObjectID];
				if (ObjectExist(BackBufferObjectID))
				{
					iGetTriangles = GetObjectPolygonCount(BackBufferObjectID);
					iGetVertex = GetObjectTotalVertexCount(BackBufferObjectID);
					iGetLodLevels = GetLodLevels(BackBufferObjectID);

					//Get texture plate size from wicked.
					cGetTextureSize = "Texture Plate Size: ";

					if (pObject)
					{
						// Ensure that the characters created are rendered with double siced hair:
						if (sGotoPreviewWithFile != "")
						{
							if (strstr(sGotoPreviewWithFile.Get(), "entitybank\\user"))
							{
								if (FileOpen(1) == 1) CloseFile(1);

								OpenToRead(1, sGotoPreviewWithFile.Get());
								while (FileEnd(1) == 0)
								{
									t.tline_s = ReadString(1);
									t.tcciStat_s = Lower(FirstToken(t.tline_s.Get(), " "));
									if (t.tcciStat_s == "ccpassembly")
									{
										bIsCCPObject = true;

										t.entityprofile[BackBufferEntityID].thumbnailbackdrop = "Blue spotlight.dds";
									
										// this is a character creator creation!!
										for (int i = 0; i < pObject->iFrameCount; i++)
										{
											if (strstr(pObject->ppFrameList[i]->szName, "hair"))
											{
												wiScene::MeshComponent* mesh = wiScene::GetScene().meshes.GetComponent(pObject->ppFrameList[i]->pMesh->wickedmeshindex);
												if (mesh)
												{
													// Set the meshes double sided so that the hair renders correctly.
													mesh->SetDoubleSided(true);
												}
											}
										}
										break;
									}
								}
								CloseFile(1);
							}
						}
						
						sMesh* pMesh = NULL;
						for (int i = 0; i < pObject->iFrameCount; i++)
						{
							if (pObject->ppFrameList[i]->pMesh && pObject->ppFrameList[i]->pMesh->wickedmeshindex > 0)
							{
								pMesh = pObject->ppFrameList[i]->pMesh;
								break;
							}
						}
						if (pMesh)
						{
							wiScene::MeshComponent* mesh = wiScene::GetScene().meshes.GetComponent(pMesh->wickedmeshindex);
							if (mesh)
							{
								uint64_t materialEntity = mesh->subsets[0].materialID;
								wiScene::MaterialComponent* pObjectMaterial = wiScene::GetScene().materials.GetComponent(materialEntity);
								if (pObjectMaterial)
								{
									if (pObjectMaterial->textures[0].resource.IsValid()) // MaterialComponent::BASECOLORMAP].resource)
									{
										wiGraphics::Texture* texture;
										//texture = (wiGraphics::Texture*) &pObjectMaterial->textures[0].resource;// pObjectMaterial->GetBaseColorMap();
										texture = (wiGraphics::Texture*) pObjectMaterial->textures[0].GetGPUResource();
										if (texture)
										{
											int width, height;
											width = (int)texture->GetDesc().width;
											height = (int)texture->GetDesc().height;
											cGetTextureSize = cGetTextureSize + cstr(width) + "x" + cstr(height);
										}
									}
								}
							}
						}
					}
				}
				if (ObjectExist(BackBufferObjectID) && GetNumberOfFrames(BackBufferObjectID) > 0 && t.entityprofile[BackBufferEntityID].animmax > 0)
				{
					//PE: Stop any running animations.
					SetObjectFrame(BackBufferObjectID, 0);
					StopObject(BackBufferObjectID);
					iHigestAnimFrame = 0;
					for (int n = 0; n < t.entityprofile[BackBufferEntityID].startofaianim; n++)
					{
						if (t.entityanim[BackBufferEntityID][n].finish > iHigestAnimFrame)
							iHigestAnimFrame = t.entityanim[BackBufferEntityID][n].finish;
					}
					//PE: Lowest in wicked is always 0 , some fpe have -1, always assume 0 as start for ALLAnimations.
					iLowestAnimFrame = 0;
				}

				fpe_current_loaded_script = -1; //Make sure dlua is loaded in next call to DisplayFPEBehavior.
				if (pObject)
				{
					Wicked_Copy_Material_To_Grideleprof((void*)pObject, 0);
					Wicked_Set_Material_From_grideleprof((void*)pObject, 0); //Set default selected mesh , for material.
				}

				cCurrentBackDrop = "None";
				
				t.addentityfile_s = t.entitybank_s[BackBufferEntityID];
				cFolderItem *pNewFolder = &MainEntityList;
				pNewFolder = pNewFolder->m_pNext;
				pNewFolder = pNewFolder->m_pNext;
				bool bGetOut = false;
				while (pNewFolder)
				{
					if (pNewFolder->m_pFirstFile)
					{
						cFolderItem::sFolderFiles * myfiles = NULL;
						myfiles = pNewFolder->m_pFirstFile->m_pNext;
						while (myfiles)
						{
							if (myfiles->iPreview > 0)
							{
								cstr check = myfiles->m_sPath + cstr("\\") + myfiles->m_sName;
								if (pestrcasestr(check.Get(), t.addentityfile_s.Get()))
								{
									//Set default backdrop.
									if (myfiles->m_Backdrop.Len() > 0 && myfiles->m_Backdrop != "None")
									{
										CreateBackdropObject(false, cstr("texturebank\\backdrops\\") + myfiles->m_Backdrop, t.addentityfile_s); //PE: We need a extra frame, as we set the material dirty in this call.
										cCurrentBackDrop = myfiles->m_Backdrop;
										cstr importer_getfilenameonly(LPSTR pFileAndPossiblePath);
										cCurrentBackDrop = importer_getfilenameonly(cCurrentBackDrop.Get());
									}
									else
									{
										CreateBackdropObject(false, "None", t.addentityfile_s);
										cCurrentBackDrop = "None";
									}
									bGetOut = true;
								}
							}
							if (bGetOut) break;
							myfiles = myfiles->m_pNext;
						}
					}
					if (bGetOut) break;
					pNewFolder = pNewFolder->m_pNext;
				}

				if (!(t.entityprofile[BackBufferEntityID].BackBufferZoom == -1.0f && t.entityprofile[BackBufferEntityID].BackBufferCamLeft == -1.0f && t.entityprofile[BackBufferEntityID].BackBufferRotateX == -1.0f))
				{
					//Found settings, set restore.
					RestoreBackBufferZoom = t.entityprofile[BackBufferEntityID].BackBufferZoom;
					RestoreBackBufferCamLeft = t.entityprofile[BackBufferEntityID].BackBufferCamLeft;
					RestoreBackBufferCamUp = t.entityprofile[BackBufferEntityID].BackBufferCamUp;
					RestoreBackBufferRotateX = t.entityprofile[BackBufferEntityID].BackBufferRotateX;
					RestoreBackBufferRotateY = t.entityprofile[BackBufferEntityID].BackBufferRotateY;
					bBackBufferRestoreCamera = true; //Restore from fpe settings in next call.
				}

				//Check if we can default to animset 1 and set pose.
				if (ObjectExist(BackBufferObjectID) && GetNumberOfFrames(BackBufferObjectID) > 0 && t.entityprofile[BackBufferEntityID].animmax > 0)
				{
					iAnimationSet = 0;
					bAnimationAll = false;
					//Set pose dont start anim.
					int iFrameStart = t.entityanim[BackBufferEntityID][iAnimationSet].start;
					SetObjectFrame(BackBufferObjectID, iFrameStart);
				}
				if (ObjectExist(BackBufferObjectID) && GetNumberOfFrames(BackBufferObjectID) > 0 && t.entityprofile[BackBufferEntityID].animmax > 0)
				{
					if (t.entityprofile[BackBufferEntityID].iThumbnailAnimset >= 0 && t.entityprofile[BackBufferEntityID].iThumbnailAnimset < t.entityprofile[BackBufferEntityID].startofaianim)
					{
						// new method uses name instead of fixed values
						if (t.entityprofile[BackBufferEntityID].playanimineditor == -1)
						{
							// uses name instead of index, the negative is the ordinal into the animset
							extern void entity_loop_using_negative_playanimineditor(int e, int obj, cstr animname);
							entity_loop_using_negative_playanimineditor(0, BackBufferObjectID, t.entityprofile[BackBufferEntityID].playanimineditor_name);
						}
						else
						{
							iAnimationSet = t.entityprofile[BackBufferEntityID].iThumbnailAnimset;
							bAnimationAll = false;
							int iFrameStart = t.entityanim[BackBufferEntityID][iAnimationSet].start;
							int iFrameEnd = t.entityanim[BackBufferEntityID][iAnimationSet].finish;
							SetObjectFrame(BackBufferObjectID, iFrameStart);
							//Start selected animation
							LoopObject(BackBufferObjectID, iFrameStart, iFrameEnd);
						}
						bStartAnimation = true;
					}
					if (bStartAnimation == true)
					{
						if (t.entityprofile[BackBufferEntityID].animspeed > 0)
						{
							SetObjectSpeed(BackBufferObjectID, t.entityprofile[BackBufferEntityID].animspeed);
						}
					}
				}

				if (cCurrentBackDrop == "None" && t.entityprofile[BackBufferEntityID].thumbnailbackdrop.Len() > 0 && t.entityprofile[BackBufferEntityID].thumbnailbackdrop != "None")
				{
					cCurrentBackDrop = t.entityprofile[BackBufferEntityID].thumbnailbackdrop;
					CreateBackdropObject(false, cstr("texturebank\\backdrops\\") + cCurrentBackDrop, t.addentityfile_s);
				}

			}

			static bool bTriggerResize = false;
			if (bTriggerResize)
			{
				ImGui::SetNextWindowSize(ImVec2(1320.0f, 612.0f), ImGuiCond_Always);
				bTriggerResize = false;
			}
			//ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse |
			int addflags = 0;
			if (sGotoPreviewWithFile != "")
			{
				addflags = ImGuiWindowFlags_NoTitleBar;
			}
			bPopModalOpenEntity = ImGui::BeginPopupModal("##Preview##DisModalMode", &bLargePreview, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoResize | addflags);
			if (bPopModalOpenEntity)
			{
				if (addflags == ImGuiWindowFlags_NoTitleBar)
				{
					ImGui::Text("");
				}
				ImVec2 cur_size = ImGui::GetWindowSize();
				if (cur_size.y < 200.0f)
				{
					//Trigger resize.
					bTriggerResize = true;
				}
				grideleprof_uniqui_id = 45000; //Always use unique IDs.
				float fPreviewImgSize = 1024.0f;
		
				ImGui::Columns(2, "PreviewColumns", false);  //false no border
				ImGui::SetColumnOffset(0, 0.0f);
				ImGui::SetColumnOffset(1, fPreviewImgSize);

				if (ImageExist(iLargePreviewImageID))
				{
					bLoopFullFPS = false;
					float fMoveSpeed = 512.0f;
					float ImgX = ImageWidth(iLargePreviewImageID);
					float ImgY = ImageHeight(iLargePreviewImageID);
					float Ratio = fPreviewImgSize / ImgX;
					ImgY *= Ratio;
					ImGui::ImgBtn(iLargePreviewImageID, ImVec2(fPreviewImgSize, ImgY), ImVec4(0.0, 0.0, 0.0, 0.0), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(1.0, 1.0, 1.0, 1.0), 0, 0, 0, 0, false);
					if (ImGui::IsItemActive() && ImGui::IsMouseDown(0) && ImGui::IsMouseDragging(0))
					{
						//PE: Some object rotate inverted Pivot ?
						BackBufferRotateY -= ImGui::GetIO().MouseDelta.x / fMoveSpeed * (360.0f*g.timeelapsed_f);
						if (BackBufferRotateY < 0) BackBufferRotateY += 360.0;
						if (BackBufferRotateY > 360.0) BackBufferRotateY -= 360.0;

						if (t.entityprofile[BackBufferEntityID].ischaracter == 0) {
							BackBufferRotateX -= ImGui::GetIO().MouseDelta.y / fMoveSpeed * (360.0f*g.timeelapsed_f);
							if (BackBufferRotateX < 0) BackBufferRotateX += 360.0;
							if (BackBufferRotateX > 360.0) BackBufferRotateX -= 360.0;
						}
						bLoopFullFPS = true;
					}

					static float fMoveBackbufferToLeft = 0.0f, fMoveBackbufferToUp = 0.0f;
					static float BackBufferCamLeftOld = 0.0f, BackBufferCamUpOld = 0.0f;
					float fCamDistance = BackBufferCamMove * 0.5 - BackBufferZoom;
					//fCamDistance *= 0.475; //
					fCamDistance *= (0.70*g.timeelapsed_f);
					if (fCamDistance < 50.0f) fCamDistance = 50.0f;

					if (ImGui::IsMouseDown(1) && ImGui::IsItemHovered() && ImGui::IsMouseDragging(1))
					{
						//PE: Use inertia slerp
						fMoveBackbufferToLeft = fMoveBackbufferToLeft + ImGui::GetIO().MouseDelta.x / fMoveSpeed * fCamDistance;
						BackBufferCamLeft = ImLerp(fMoveBackbufferToLeft, BackBufferCamLeftOld, 0.85);
						BackBufferCamLeftOld = BackBufferCamLeft;

						fMoveBackbufferToUp = fMoveBackbufferToUp + ImGui::GetIO().MouseDelta.y / fMoveSpeed * fCamDistance;
						BackBufferCamUp = ImLerp(fMoveBackbufferToUp, BackBufferCamUpOld, 0.85);
						BackBufferCamUpOld = BackBufferCamUp;

						bLoopFullFPS = true;
						ImGui::SetMouseCursor(ImGuiMouseCursor_Pan);
					}
					else {
						fMoveBackbufferToLeft = BackBufferCamLeftOld = BackBufferCamLeft;
						fMoveBackbufferToUp = BackBufferCamUpOld = BackBufferCamUp;
					}

					if (ImGui::IsItemHovered() && ImGui::GetIO().MouseWheel != 0)
					{
						float speed = BackBufferCamMove / 30.0f; //Depent on object size. Now faster (/ 50.0f)
						if (io.KeyShift)
							speed *= 2.0; //Faster when using shift.
						speed *= g.timeelapsed_f;
						BackBufferZoom += ImGui::GetIO().MouseWheel*speed;
						bLoopFullFPS = true;
					}
				}

				ImGui::Spacing();

				ImGui::NextColumn();

				ImGui::BeginChild("##ChildNewFPEPropertiesPanel", ImVec2(0, 0), false, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNavInputs); //ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar
				
				std::string sString;
				
				if (ImGui::StyleCollapsingHeader("Backdrop", ImGuiTreeNodeFlags_DefaultOpen))
				{
					GetFilesListForLibrary("texturebank\\backdrops\\", true);
					ImGui::Indent(10);
					ImGui::TextCenter("Static Image");
					ImGui::PushItemWidth(-10);
					cstr cRet = ComboFilesListForLibrary(cCurrentBackDrop.Get(), 6, 120, true);
					if (cRet != cCurrentBackDrop)
					{
						t.addentityfile_s = t.entitybank_s[BackBufferEntityID];
						cCurrentBackDrop = cRet;
						CreateBackdropObject(true, cstr("texturebank\\backdrops\\") + cCurrentBackDrop, t.addentityfile_s);
						if (!ImageExist(BACKDROPMAGE))
						{
							cCurrentBackDrop = "None";
						}
					}
					if (ImGui::IsItemHovered()) ImGui::SetTooltip("Select static image for object backdrop");

					if (bBackbufferCubemapActive)
					{
						t.addentityfile_s = t.entitybank_s[BackBufferEntityID];
						sString = t.addentityfile_s.Get();
						replaceAll(sString, ".fpe", "_fpe_cube.dds");
					}
					else
						sString = "";

					//#### Keywords ####
					cstr cTmp = t.entityprofile[BackBufferEntityID].keywords_s;
					cTmp = imgui_setpropertystring2_v2(0, cTmp.Get(), "Keywords", "Enter additional keywords, separated by commas.", false);
					if (cTmp != t.entityprofile[BackBufferEntityID].keywords_s)
					{
						t.entityprofile[BackBufferEntityID].keywords_s = cTmp;
					}

					ImGui::PopItemWidth();
					ImGui::Indent(-10);
				}

				ImGui::Indent(10);
				if (ImGui::StyleButton("LOD Generator Lite", ImVec2(ImGui::GetContentRegionAvail().x - 10.0f, 0.0f)))
				{
					//PE: Startup Lod Generator Lite.
					std::string LODFile = "entitybank\\";
					LODFile = LODFile + t.entitybank_s[BackBufferEntityID].Get();
					char pDestinationFile[10240];
					strcpy(pDestinationFile, LODFile.c_str());
					GG_GetRealPath(pDestinationFile, 0);
					char pOldDir[MAX_PATH];
					strcpy(pOldDir, GetDir());
					SetDir("..");
					SetDir("Tools\\");
					SetDir("Lod Generator Lite");
					std::string params = "\"";
					params = params + pDestinationFile;
					params = params + "\"";
					HINSTANCE hinstance = ShellExecuteA(NULL, "open", "LODGeneratorLite.exe", params.c_str(), "", SW_SHOWDEFAULT);
					SetDir(pOldDir);
				}
				ImGui::Indent(-10);

				if (ObjectExist(BackBufferObjectID) && GetNumberOfFrames(BackBufferObjectID) > 0 && t.entityprofile[BackBufferEntityID].animmax > 0)
				{
					//PE: Make sure we set anim speed. we only use parent object in this system.
					t.tanimspeed_f = t.entityprofile[BackBufferEntityID].animspeed;
					if (ObjectExist(BackBufferObjectID) == 1)
					{
						SetObjectSpeed(BackBufferObjectID, t.tanimspeed_f);
					}

					if (strstr(sGotoPreviewWithFile.Get(), "charactercreatorplus"))
						bIsCCPObject = true;
				
					if (!bIsCCPObject)
					{
						if (t.entityprofile[BackBufferEntityID].playanimineditor != -1)
						{
							if (ImGui::StyleCollapsingHeader("Animations", ImGuiTreeNodeFlags_DefaultOpen))
							{
								ImGui::Indent(10);

								static int iFrameCurrent = 0;
								float fFontSize = ImGui::GetFontSize();
								int iFrameStart = t.entityanim[BackBufferEntityID][iAnimationSet].start;
								int iFrameEnd = t.entityanim[BackBufferEntityID][iAnimationSet].finish;
								if (bAnimationAll)
								{
									iFrameStart = iLowestAnimFrame;
									iFrameEnd = iHigestAnimFrame;
								}

								cstr sComboName = cstr("Animation ") + cstr(iAnimationSet) + " (" + cstr(iFrameStart) + "," + cstr(iFrameEnd) + ")";
								cstr sComboNameAll = cstr("All Animations ") + " (" + cstr(iLowestAnimFrame) + "," + cstr(iHigestAnimFrame) + ")";
								if (bAnimationAll)
									sComboName = sComboNameAll;

								ImGui::PushItemWidth(-10 - fFontSize - 8.0f);
								if (ImGui::BeginCombo("##animcomboselection", sComboName.Get())) // The second parameter is the label previewed before opening the combo.
								{
									if (ImGui::Selectable(sComboNameAll.Get(), bAnimationAll))
									{
										iFrameStart = iLowestAnimFrame;
										iFrameEnd = iHigestAnimFrame;
										bAnimationAll = true;
										SetObjectFrame(BackBufferObjectID, iLowestAnimFrame);
										if (bStartAnimation)
										{
											LoopObject(BackBufferObjectID, iLowestAnimFrame, iHigestAnimFrame);
										}
									}
									for (int n = 0; n < t.entityprofile[BackBufferEntityID].startofaianim; n++)
									{
										cstr sComboName = cstr("Animation ") + cstr(n) + " (" + cstr(t.entityanim[BackBufferEntityID][n].start) + "," + cstr(t.entityanim[BackBufferEntityID][n].finish) + ")";
										bool is_selected = (!bAnimationAll && iAnimationSet == n);
										if (ImGui::Selectable(sComboName.Get(), is_selected))
										{
											iAnimationSet = n;
											bAnimationAll = false;
											iFrameStart = t.entityanim[BackBufferEntityID][iAnimationSet].start;
											iFrameEnd = t.entityanim[BackBufferEntityID][iAnimationSet].finish;
											SetObjectFrame(BackBufferObjectID, iFrameStart);
											if (bStartAnimation)
											{
												//Start any animation
												LoopObject(BackBufferObjectID, iFrameStart, iFrameEnd);
											}
										}
										if (is_selected)
											ImGui::SetItemDefaultFocus();   // You may set the initial focus when opening the combo (scrolling + for keyboard navigation support)
									}
									ImGui::EndCombo();
								}
								if (ImGui::IsItemHovered()) ImGui::SetTooltip("Select Animation");
								ImGui::PopItemWidth();

								ImGui::SameLine();

								if (!bStartAnimation)
								{
									if (ImGui::ImgBtn(MEDIA_PLAY, ImVec2(fFontSize, fFontSize), ImColor(255, 255, 255, 0), ImColor(255, 255, 255, 255), ImColor(255, 255, 255, 128), drawCol_Down, -1, 0, 0, 0, false, false, false, false, false, bBoostIconColors))
									{
										SetObjectFrame(BackBufferObjectID, iFrameCurrent);
										LoopObject(BackBufferObjectID, iFrameStart, iFrameEnd);

										bStartAnimation = true;
									}
									if (ImGui::IsItemHovered()) ImGui::SetTooltip("Play Animation");

								}
								else
								{
									if (ImGui::ImgBtn(MEDIA_PAUSE, ImVec2(fFontSize, fFontSize), ImColor(255, 255, 255, 0), ImColor(255, 255, 255, 255), ImColor(255, 255, 255, 128), drawCol_Down, -1, 0, 0, 0, false, false, false, false, false, bBoostIconColors))
									{
										StopObject(BackBufferObjectID);
										SetObjectFrame(BackBufferObjectID, iFrameCurrent);
										bStartAnimation = false;
									}
									if (ImGui::IsItemHovered()) ImGui::SetTooltip("Pause Animation");
								}


								//PE: Wicked dont stop animation by itself so.
								sObject* pObject = g_ObjectList[BackBufferObjectID];
								WickedCall_CheckAnimationDone(pObject);

								iFrameCurrent = WickedCall_GetObjectFrame(pObject);
								ImGui::TextCenter("Current Animation Frame");
								if (bStartAnimation)
								{
									ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
									ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
								}
								ImGui::PushItemWidth(-10);

								if (ImGui::MaxSliderInputInt("##AnimFrame", &iFrameCurrent, iFrameStart, iFrameEnd, "Display Current Animation Frame"))
								{
									//If changed stop anim and display directly.
									StopObject(BackBufferObjectID);
									SetObjectFrame(BackBufferObjectID, iFrameCurrent);
									bStartAnimation = false;
								}

								ImGui::PopItemWidth();
								if (bStartAnimation)
								{
									ImGui::PopItemFlag();
									ImGui::PopStyleVar();
								}

								ImGui::Indent(-10);
								ImGui::Text("");
							}
						}
					}
				}

				entityeleproftype backup_grideleprof = t.grideleprof;
				int backup_entid = t.entid;
				int backup_gridentity = t.gridentity;
				t.entid = BackBufferEntityID;
				t.gridentity = BackBufferEntityID;

				entity_fillgrideleproffromprofile();
				imgui_set_openproperty_flags(t.gridentity);
				
				//PE: Only if active.
				if (t.grideleprof.soundset_s.Len() > 0 || t.grideleprof.soundset1_s.Len() > 0 || t.grideleprof.soundset2_s.Len() > 0 || t.grideleprof.soundset3_s.Len() > 0)
				{
					if (!bIsCCPObject)
					{
						if (ImGui::StyleCollapsingHeader("Object's Media", ImGuiTreeNodeFlags_DefaultOpen))
						{
							ImGui::Indent(10);
							DisplayFPEMedia(true, BackBufferEntityID);
							ImGui::Indent(-10);
						}
					}
				}
				t.grideleprof = backup_grideleprof;
				t.gridentity = backup_gridentity;
				t.entid = backup_entid;

				if (!bIsCCPObject)
				{
					ControlAdvancedSetting(pref.iFullscreenPreviewAdvanced, "advanced object library preview details", &bLargePreview);
					if (pref.iFullscreenPreviewAdvanced)
					{
						cstr cTmp, cOrg = (char *)sString.c_str();
						cTmp = imgui_setpropertyfile2_v2(t.group, cOrg.Get(), "Cubemap Image", "Select a cube map texture file for use as the object backdrop", "\\", false);
						if (cTmp != cOrg)
						{
							// change cube map
							t.addentityfile_s = cstr("entitybank\\") + t.entitybank_s[BackBufferEntityID];
							std::string sDestination = t.addentityfile_s.Get();
							replaceAll(sDestination, ".fpe", "_fpe_cube.dds");
							if (FileExist(cTmp.Get()))
							{
								char pDestinationFile[10240];
								strcpy(pDestinationFile, sDestination.c_str());
								GG_GetRealPath(pDestinationFile, 1);
								extern char szWriteDir[MAX_PATH];
								if (!pestrcasestr(pDestinationFile, szWriteDir))
								{
									// this is not the DocWrite folder ?
								}
								else
								{
									// copy chosen cube map to the new entity texture _cube.dds 
									bool bret = CopyFileA(cTmp.Get(), &pDestinationFile[0], false);

									// reload new textures
									cCurrentBackDrop = "None";
									t.addentityfile_s = t.entitybank_s[BackBufferEntityID];
									CreateBackdropObject(true, "None", t.addentityfile_s);
								}
							}
						}

						if (ImGui::StyleCollapsingHeader("Attributes", ImGuiTreeNodeFlags_DefaultOpen))
						{
							ImGui::Indent(10);
							{
								ImGui::Text("Triangles: %d", iGetTriangles);
								ImGui::Text("Vertices: %d", iGetVertex);
								ImGui::Text("LODs: %d", iGetLodLevels + 1);
								ImGui::Text("%s", cGetTextureSize.Get());
							}
							ImGui::Indent(-10);
						}

						sObject* pObject = g_ObjectList[BackBufferObjectID];
						if (pObject)
						{
							if (ImGui::StyleCollapsingHeader("Textures", ImGuiTreeNodeFlags_DefaultOpen))
							{
								ImGui::Indent(10);
								{
									Wicked_Copy_Material_To_Grideleprof((void*)pObject, 0);
									t.grideleprof.WEMaterial.MaterialActive = true;
									Wicked_Change_Object_Material((void*)pObject, 5);
								}
								ImGui::Indent(-10);
							}
						}
					}
				}

				// Update/Add To Library Button
				cstr sButLabel = "Update Thumbnail";
				if (sGotoPreviewWithFile != "")
				{
					sButLabel = "Add to Object Library";
				}
				sButLabel += "##ObjectLibPreview";
				static int iUpdateFPESettings = 0;
				if (ImGui::StyleButton(sButLabel.Get(), ImVec2(ImGui::GetContentRegionAvail().x - 10.0f, 0.0f)))
				{
					iUpdateFPESettings = 1;
				}

				if (iUpdateFPESettings > 0)
				{
					if (iUpdateFPESettings == 1)
					{
						iUpdateFPESettings++;
					}
					if (iUpdateFPESettings == 2)
					{
						iUpdateFPESettings++;
					}
					if (iUpdateFPESettings == 3)
					{
						//Update the actual FPE file.
						iUpdateFPESettings = 0;
						if (BackBufferIsGroup == true)
						{
							t.addentityfile_s = g_LastGroupSaved_s;
						}
						else
						{
							t.addentityfile_s = t.entitybank_s[BackBufferEntityID];
						}

						CreateBackBufferCacheNameEx(t.addentityfile_s.Get(), thumb_x, thumb_y, true);
						bool bGetOut = false;

						// delete old thumb image and give chance for new one to be saved (g_bThumbBankCopyMode)
						GG_SetWritablesToRoot(true);
						if (FileExist(BackBufferCacheName.Get()))
						{
							DeleteAFile(BackBufferCacheName.Get());
							g_bThumbBankCopyMode = false;
						}
						GG_SetWritablesToRoot(false);

						cFolderItem *pDontRefreshFolder = NULL;
						cFolderItem::sFolderFiles * updatefiles = NULL;
						cFolderItem *pNewFolder = &MainEntityList;
						if (pNewFolder) pNewFolder = pNewFolder->m_pNext;
						if (pNewFolder) pNewFolder = pNewFolder->m_pNext;
						while (pNewFolder)
						{
							if (pNewFolder->m_pFirstFile)
							{
								cFolderItem::sFolderFiles * myfiles = NULL;
								myfiles = pNewFolder->m_pFirstFile->m_pNext;
								while (myfiles)
								{
									cstr check = myfiles->m_sPath + cstr("\\") + myfiles->m_sName;
									if (pestrcasestr(check.Get(), t.addentityfile_s.Get()))
									{
										if (myfiles->iPreview > 0 && GetImageExistEx(myfiles->iPreview) && myfiles->iPreview >= 4000 && myfiles->iPreview < UIV3IMAGES) { //PE: Need to protect system images after tool img range has changed. (myfiles->iPreview can be a system icon)
											iDeleteInNextUpdate = myfiles->iPreview;
										}
										myfiles->iPreview = 0;
										myfiles->iBigPreview = 0;
										myfiles->m_Backdrop = cCurrentBackDrop;
										t.entityprofile[BackBufferEntityID].thumbnailbackdrop = cCurrentBackDrop;
										myfiles->m_sFPEKeywords = t.entityprofile[BackBufferEntityID].keywords_s;
										updatefiles = myfiles;
										bGetOut = true;
									}
									//}
									if (bGetOut) break;
									myfiles = myfiles->m_pNext;
								}
							}
							if (bGetOut) {
								pDontRefreshFolder = pNewFolder;
								break;
							}
							pNewFolder = pNewFolder->m_pNext;
						}

						//Update FPE file.
						std::vector<std::string> fpe_file;
						cstr c_fpefile = cstr("entitybank\\") + t.addentityfile_s;
						getVectorFileContent(c_fpefile.Get(), fpe_file, true);
						if (fpe_file.size() > 0)
						{
							RemoveStrStrFromVectorFile("thumbnailbackdrop=", fpe_file, true);
							RemoveStrStrFromVectorFile("thumbnailzoom=", fpe_file, true);
							RemoveStrStrFromVectorFile("thumbnailcamleft=", fpe_file, true);
							RemoveStrStrFromVectorFile("thumbnailcamup=", fpe_file, true);
							RemoveStrStrFromVectorFile("thumbnailrotatex=", fpe_file, true);
							RemoveStrStrFromVectorFile("thumbnailrotatey=", fpe_file, true);
							RemoveStrStrFromVectorFile("thumbnailanimset=", fpe_file, true);
							RemoveStrStrFromVectorFile("keywords=", fpe_file, true);
							RemoveStrStrFromVectorFile(";thumbnail", fpe_file, true);

							std::string add = ";thumbnail";
							fpe_file.push_back(add);
							add = std::string("thumbnailbackdrop = ") + std::string(cCurrentBackDrop.Get());
							fpe_file.push_back(add);
							add = std::string("thumbnailzoom = ") + std::to_string(BackBufferZoom);
							fpe_file.push_back(add);
							add = std::string("thumbnailcamleft = ") + std::to_string(BackBufferCamLeft);
							fpe_file.push_back(add);
							add = std::string("thumbnailcamup = ") + std::to_string(BackBufferCamUp);
							fpe_file.push_back(add);
							add = std::string("thumbnailrotatex = ") + std::to_string(BackBufferRotateX);
							fpe_file.push_back(add);
							add = std::string("thumbnailrotatey = ") + std::to_string(BackBufferRotateY);
							fpe_file.push_back(add);

							if (ObjectExist(BackBufferObjectID) && GetNumberOfFrames(BackBufferObjectID) > 0 && t.entityprofile[BackBufferEntityID].animmax > 0)
							{
								if (!bAnimationAll)
								{
									//We have animations.
									add = std::string("thumbnailanimset = ") + std::to_string(iAnimationSet);
									fpe_file.push_back(add);

								}
							}
							if (t.entityprofile[BackBufferEntityID].keywords_s.Len() > 0)
							{
								add = std::string("keywords = ") + t.entityprofile[BackBufferEntityID].keywords_s.Get();
								fpe_file.push_back(add);
							}

							saveVectorFileContent(c_fpefile.Get(), fpe_file);
						}

						if (sGotoPreviewWithFile != "")
						{
							bLargePreview = false;
							sGotoPreviewWithFile = "";
						}
						cCurrentBackDropImageFile = ""; //make sure to reload when doing new thumb.

						//Also update current loaded entityprofile.
						t.entityprofile[BackBufferEntityID].BackBufferZoom = BackBufferZoom;
						t.entityprofile[BackBufferEntityID].BackBufferCamLeft = BackBufferCamLeft;
						t.entityprofile[BackBufferEntityID].BackBufferCamUp = BackBufferCamUp;
						t.entityprofile[BackBufferEntityID].BackBufferRotateX = BackBufferRotateX;
						t.entityprofile[BackBufferEntityID].BackBufferRotateY = BackBufferRotateY;
						if (!bAnimationAll)
							t.entityprofile[BackBufferEntityID].iThumbnailAnimset = iAnimationSet;
						else
							t.entityprofile[BackBufferEntityID].iThumbnailAnimset = -1;

						if (pDontRefreshFolder)
						{
							cstr path = pDontRefreshFolder->m_sFolderFullPath.Lower();
							char *findpath = (char *)pestrcasestr(path.Get(), "entitybank\\");
							if (findpath) path = findpath;

							cFolderItem *pNewFolder = &MainEntityList;
							if (pNewFolder) pNewFolder = pNewFolder->m_pNext;
							if (pNewFolder) pNewFolder = pNewFolder->m_pNext;
							while (pNewFolder)
							{
								cstr pathcheck = pNewFolder->m_sFolderFullPath.Lower();
								char *findpath = (char *)pestrcasestr(pathcheck.Get(), "entitybank\\");
								if (findpath) pathcheck = findpath;
								if (pathcheck == path)
								{
									//PE: as we save a new fpe in a new folder DocWrite, dont refresh folder. we already have the old fpe that are updated.
									struct stat sb;
									if (stat(pNewFolder->m_sFolderFullPath.Get(), &sb) == 0)
									{
										if (sb.st_mtime != pNewFolder->m_tFolderModify)
										{
											pNewFolder->m_tFolderModify = sb.st_mtime;

											if (updatefiles && updatefiles->bFavorite)
											{
												//PE: This is the new folder, if old was fav., add to favorite in new folder.
												cstr file = pNewFolder->m_sFolderFullPath;
												file = file + "\\" + updatefiles->m_sName.Get();
												extern std::vector<std::string> files_favorite;
												files_favorite.push_back(file.Get());
												saveVectorFileContent("favoritelist.ini", files_favorite);
											}
										}
									}
								}
								pNewFolder = pNewFolder->m_pNext;
							}
						}

						bDisplayProjectMedia = false;
						bDisplayFavorite = false;
						bViewAllFolders = false;
						bViewShowcase = false;
						bUpdateSearchSorting = true;
						bUpdateSearchScrollbar = true;

						// after make preview thumb and updated FPE, once again refresh to latest entity 
						g_iCheckExistingFilesModifiedDelayed = 50;
					}
				}
				char* tooltip = "Finish Setting the Object Library Preview";
				if (ImGui::IsItemHovered()) ImGui::SetTooltip(tooltip);

				// Hitting exactly at the botton could cause flicker, so add some additional lines when scrollbar on.
				if (ImGui::GetCurrentWindow()->ScrollbarSizes.x > 0)
				{
					ImGui::Text("");
					ImGui::Text("");
				}

				ImGui::EndChild();

				//When popup open , make sure we update the backbuffer all the time.
				bLoopBackBuffer = true;
				WickedCall_EnableThumbLight(true);
				ImGui::Columns(1);

				//Render titlebar centered.
				cstr title = " Object Library Preview";
				if (pPreviewFile)
				{
					title = pPreviewFile->m_sNameFinal + title;
				}
				if (bIsCCPObject)
					title = "Choose your character's library thumbnail";

				float fTextSize = ImGui::CalcTextSize(title.Get()).x;
				float xcenter = (ImGui::GetWindowSize().x*0.5) - (fTextSize*0.5);
				ImVec2 titlebar_pos = ImGui::GetWindowPos() + ImVec2(xcenter, 4);
				ImGuiWindow* window = ImGui::GetCurrentWindow();

				ImGui::EndPopup();

				//Render title bar after End. end fill titlebar.
				ImGuiContext& g = *GImGui;
				window->DrawList->AddText(g.Font, g.FontSize, titlebar_pos, ImGui::GetColorU32(ImGuiCol_Text), title.Get());
			}

			if (!bLargePreview)
			{
				//Close down modal popup.
				BackBufferSaveCacheName = "";
				BackBufferObjectID = 0;
				BackBufferImageID = 0;
				bLoopBackBuffer = false;
				RevertBackbufferCubemap();
				WickedCall_SetSunDirection(t.visuals.SunAngleX, t.visuals.SunAngleY, t.visuals.SunAngleZ);
				master_renderer->setBloomEnabled(t.visuals.bBloomEnabled);
				WickedCall_MoveReflectionProbe(GGORIGIN_X, GGORIGIN_Y + 5000, GGORIGIN_Z, "editorProbe", 500);
				WickedCall_EnableThumbLight(false);
				if (pPreviewFile)
					pPreviewFile->iPreview = 0;

				bImagesStillInImGuiQueue = true;
			}

		}
		else
		{
			bPopModalOpenEntity = false;
		}

		//############################
		//#### END Large preview. ####
		//############################

		//MIN 600 width
		static float fLastContentWidth = 0;
		static ImVec2 vLastWindowSize = ImVec2(0, 0);
		if (refresh_gui_docking != 0 && !bResetObjectLibrarySize)
		{
			if (fLastContentWidth > 0 && fLastContentWidth < 600.0f && vLastWindowSize.y > 0)
			{
				ImGui::SetNextWindowSize(ImVec2(600.0f, vLastWindowSize.y), ImGuiCond_Always); //full screen.
			}
		}

		//PE: Moved here , so modal window above dont rely on this window for center position.
		if (refresh_gui_docking != 0 && !bResetObjectLibrarySize)
		{
			imgui_AddMinMaxButton(0, true); //check min max state.

			if (imgui_GetMinMaxButtonState(0))
			{
				//Fullscreen
				ex_window_flags |= ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;
			}

			static int lastminstate = -1;
			if (lastminstate != imgui_GetMinMaxButtonState(0))
			{
				init_Left_Categories_Column_Width = 3;
				lastminstate = imgui_GetMinMaxButtonState(0);
			}
		}

		static cFolderItem::sFolderFiles * playingiles = NULL;
		static cFolderItem::sFolderFiles * selectedmediafile = NULL;

		if (pref.iDisableObjectLibraryViewport)
		{
			ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowViewport(viewport->ID);
		}

		ImGui::Begin("##Object Library ExternalWindow", &bExternal_Entities_Window, ex_window_flags | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

		static float fScaleIcons = 1.0;

		//PE: If user exit in fullscreen (it get saved) make sure to restore to default size.
		static bool bCheckFullScreenOnStartup = true;
		bResetObjectLibrarySize = false;

		if (bCheckFullScreenOnStartup)
		{
			if (imgui_CheckMinMaxStartupState(0))
			{
				//PE: Was set fullscreen, adjust columns width
				init_Left_Categories_Column_Width = 3;
			}
			bCheckFullScreenOnStartup = false;
		}

		vLastWindowSize = ImGui::GetWindowSize();

		float cwidth = ImGui::GetContentRegionAvailWidth();
		fLastContentWidth = cwidth;

		static bool bAddNewSelectionToGame = false;
		static int iAddSelectionStep = 0;
		bool bIsWeDocked = ImGui::IsWindowDocked();
		static int current_tab = -1;

		int i = 0;

		static char cAllFilters[10][MAX_PATH];
		char cHeader[MAX_PATH];

		int control_wrap_width = 90;
		strcpy(cHeader, "");

		ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX()-2.0f, ImGui::GetCursorPosY() + 6.0));
		ImGui::Text(""); //"Filter: ");
		ImGui::SameLine();

		ID3D11ShaderResourceView* lpTexture = NULL;

		bool rb_change = false;
		static bool bDLUAOnly = true;

		strcpy(cAllFilters[0], "HUD Assets");
		strcpy(cAllFilters[1], "Character");
		strcpy(cAllFilters[2], "Objects");
		strcpy(cAllFilters[3], "Weapons");
		strcpy(cAllFilters[4], "User");
		if (iDisplayLibraryType == 1) strcpy(cAllFilters[0], "OGG"); //Not active
		if (iDisplayLibraryType == 2) strcpy(cAllFilters[0], "DDS"); //Not active
		if (iDisplayLibraryType == 3) strcpy(cAllFilters[0], "MP4"); //Not active
		if (iDisplayLibraryType == 4) strcpy(cAllFilters[0], "LUA"); //Not active
		if (iDisplayLibraryType == 5) strcpy(cAllFilters[0], "ARX"); //Not active

		int sortby_combo_width = 130;
		bool bLastEntityGotFocus = bEntityGotFocus;
		if (1) //Searchbar
		{
			ImGui::SameLine();

			if (ImGui::GetCursorPosX() + control_wrap_width > ImGui::GetWindowSize().x)
			{
				ImGui::Text(""); //NewLine
			}

			float fXWidth = ImGui::GetFontSize()*1.5;

			fXWidth += sortby_combo_width;

			ImGui::PushItemWidth(-10 - fXWidth);

			if (g_iDevToolsOpen != 0)
			{
				ImGui::PushItemWidth(30);
				if (ImGui::StyleButton("Refresh##+", ImVec2(0, 0)))
				{
					// developer mode rescans every time ADD is pressed
					extern int g_iRefreshLibraryFolders;
					g_iRefreshLibraryFolders = 1;
				}
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("Click to refresh all library folders (developer tools mode)");
				ImGui::PopItemWidth();
				ImGui::SameLine();
			}
			ImGui::Text(" ");

			ImGui::SameLine();

			ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX() - 8.0, ImGui::GetCursorPosY()));
			
			bEntityGotFocus = false;
			ImGuiStyle& style = ImGui::GetStyle();
			float fOldSpacing = style.FramePadding.x;
			style.FramePadding.x = 22.0; //Make room for search icon.
			ImVec2 vSearchPos = ImGui::GetCursorPos();
			static cstr sOldSearch = &cSearchAllEntities[i][0];
			static std::string phonetic_find = "";

			// Force the keyboard focus to the input text field when the user presses the close button (below).
			static bool bStealKeyboardFocus = false;
			if (bStealKeyboardFocus)
			{
				ImGui::SetKeyboardFocusHere(0);
				bStealKeyboardFocus = false;
			}

			if (ImGui::InputText("##cSearchAllEntities", &cSearchAllEntities[i][0], MAX_PATH, ImGuiInputTextFlags_EnterReturnsTrue))
			{
				bUpdateSearchSorting = true;
				phonetic_find = soundexall(cSearchAllEntities[i]);

				if (strlen(cSearchAllEntities[i]) > 1)
				{
					bool already_there = false;
					for (int l = 0; l < MAXSEARCHHISTORY; l++)
					{
						if (strcmp(cSearchAllEntities[i], pref.search_history[l]) == 0)
						{
							already_there = true;
							break;
						}
					}
					if (!already_there)
					{
						bool foundspot = false;
						for (int l = 0; l < MAXSEARCHHISTORY; l++)
						{
							if (strlen(pref.search_history[l]) <= 0)
							{
								strcpy(pref.search_history[l], cSearchAllEntities[i]);
								foundspot = true;
								break;
							}
						}
						if (!foundspot)
						{
							//Move entry list.
							for (int l = 0; l < MAXSEARCHHISTORY; l++)
							{
								strcpy(pref.search_history[l], pref.search_history[l + 1]);
							}
							strcpy(pref.search_history[MAXSEARCHHISTORY - 1], cSearchAllEntities[i]);
						}
					}
				}
			}
			if (ImGui::MaxIsItemFocused()) bImGuiGotFocus = true;
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Type here to search for an object in your library");

			cstr sNewSearch = &cSearchAllEntities[i][0];
			if (sOldSearch != sNewSearch)
			{
				sOldSearch = sNewSearch;
				phonetic_find = soundexall(cSearchAllEntities[i]);
				bUpdateSearchSorting = true;
				bUpdateSearchScrollbar = true;
			}
			style.FramePadding.x = fOldSpacing;
			lpTexture = GetImagePointerView(TOOL_ENT_SEARCH);
			if (lpTexture)
			{
				ImGuiWindow* window = ImGui::GetCurrentWindow();
				ImVec2 search_icon_pos = ImGui::GetWindowPos() + vSearchPos + ImVec2(3.0, 3.0);
				window->DrawList->AddImage((ImTextureID)lpTexture, search_icon_pos, search_icon_pos + ImVec2(16, 16), ImVec2(0, 0), ImVec2(1, 1), ImGui::GetColorU32(ImVec4(1.0, 1.0, 1.0, 1.0)));
			}
			ImGui::SameLine();
			ImGui::SetItemAllowOverlap();
			if (ImGui::CloseButton(ImGui::GetCurrentWindow()->GetID("#ClearSearch"), ImGui::GetWindowPos() + ImGui::GetCursorPos() + ImVec2(-30, 0)))
			{
				bUpdateSearchSorting = true;
				bUpdateSearchScrollbar = true;
				strcpy(cSearchAllEntities[i], "");
				bStealKeyboardFocus = true;
			}
			if (ImGui::IsItemHovered())
			{
				//mouse
				ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
				ImGui::SetTooltip("Clear search");
			}
			ImGui::PopItemWidth();
		}
					
		if (1) //Combo dropdowns. Use folder names as seach.
		{
			ImGui::SameLine();
			static char * current_combo_entry = "\0";
			int comboflags = ImGuiComboFlags_NoPreview | ImGuiComboFlags_PopupAlignLeft | ImGuiComboFlags_HeightLarge;

			ImGui::PushItemWidth(-sortby_combo_width - 14); //- 24

			if (ImGui::BeginCombo("##combolastsearch", current_combo_entry, comboflags))
			{
				//Do we have a search history.
				bool display_history = false;
				for (int l = 0; l < MAXSEARCHHISTORY; l++) 
				{
					if (strlen(pref.search_history[l]) > 0) 
					{
						display_history = true;
						break;
					}
				}
				if (display_history) {
					ImGui::Text("Search History:");
					ImGui::Indent(10);
					for (int l = 0; l < MAXSEARCHHISTORY; l++) 
					{
						if (strlen(pref.search_history[l]) > 0) 
						{
							bool is_selected = (current_combo_entry == pref.search_history[l]);
							cstr sSelName = pref.search_history[l];
							sSelName = sSelName + "##Hist";
							if (ImGui::Selectable(sSelName.Get(), is_selected)) 
							{
								bUpdateSearchSorting = true;
								bUpdateSearchScrollbar = true;
								current_combo_entry = (char *)pref.search_history[l];
								strcpy(cSearchAllEntities[i], pref.search_history[l]);
							}
							if (is_selected)
								ImGui::SetItemDefaultFocus();
						}
					}
					ImGui::Indent(-10);
				}
				ImGui::EndCombo();
			}
			ImGui::PopItemWidth();

			ImGui::SameLine();

			ImGui::PushItemWidth(-6);

			const char* sortby_modes[] = { "Category Order", "Name A-Z", "Name Z-A", "Created Old-New", "Created New-Old", "Number of Polygons Low-", "Number of Polygons High-" };
			int iComboItems = IM_ARRAYSIZE(sortby_modes);
			if (iDisplayLibraryType == 0)
			{
				iComboItems -= 2;
			}
			else
			{
				iComboItems -= 2;
			}

			if (ImGui::Combo("##combostaticIssuesFilter", &current_sortby, sortby_modes, iComboItems))
			{
				bUpdateSearchSorting = true;
				bUpdateSearchScrollbar = true;
				// set sortby mode = current_sortby
				// 0 = showcase
				// 1 = no sorting, category order and files A-Z inside each category. 0
				// 2 = A-Z 1
				// 3 = Z-A 2
				// 4 = Date Old-New 5
				// 5 = Date New-Old 6
				// 6 = Poly low 7
				// 7 = Poly high 8
			}
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Set Sorting");
			ImGui::PopItemWidth();

			ImGui::Separator();
		}

		static int total_files_displayed_in_library = 0;
		if (1) //Display what is searched for.
		{

			ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX() + 10.0, ImGui::GetCursorPosY() + 3));

			cstr cViewing = "";
			bool bSearchAll = false;
			if (strlen(cSearchAllEntities[i]) > 0)
			{
				if (iDisplayLibraryType == 0)
				{
					if(iDisplayLibrarySubType==1)
						cViewing = cstr("Viewing ") + cstr(Str(total_files_displayed_in_library)) + cstr(" Animations from \"All\" categories using search term \"") + cstr(cSearchAllEntities[i]) + cstr("\"");
					else
						cViewing = cstr("Viewing ") + cstr(Str(total_files_displayed_in_library)) + cstr(" Objects from \"All\" categories using search term \"") + cstr(cSearchAllEntities[i]) + cstr("\"");
				}
				else
				{
					cViewing = cstr("Viewing ") + cstr(Str(total_files_displayed_in_library)) + cstr(" Items from \"All\" categories using search term \"") + cstr(cSearchAllEntities[i]) + cstr("\"");
				}
				bSearchAll = true;
			}
			if (bDisplayFavorite)
			{
				cViewing = cstr("Viewing ") + cstr(Str(total_files_displayed_in_library)) + cstr(" Objects from \"Favourite\" using \"All\" categories");
				bSearchAll = true;
			}

			if (!bSearchAll)
			{
				cViewing = "Viewing ";
				if (iDisplayLibraryType == 0)
				{
					if (iDisplayLibrarySubType == 1)
						cViewing = cstr("Viewing ") + cstr(Str(total_files_displayed_in_library)) + cstr(" Animations from ");
					else
						cViewing = cstr("Viewing ") + cstr(Str(total_files_displayed_in_library)) + cstr(" Objects from ");
				}
				else
				{
					cViewing = cstr("Viewing ") + cstr(Str(total_files_displayed_in_library)) + cstr(" Items from ");
				}

				int iCategories = 0;
				if (iDisplayLibraryType == 2 && bCheckboxFilters[1] && bCheckboxFilters[2] && bCheckboxFilters[3] && bCheckboxFilters[4])
				{
					cViewing = cViewing + cstr("\"All\" Categories");
				}
				else if (iDisplayLibraryType == 1 && bCheckboxFilters[1] && bCheckboxFilters[2] && bCheckboxFilters[3] && bCheckboxFilters[4])
				{
					cViewing = cViewing + cstr("\"All\" Categories");
				}
				else if (iDisplayLibraryType == 3 && bCheckboxFilters[1] && bCheckboxFilters[2] && bCheckboxFilters[3] && bCheckboxFilters[4])
				{
					cViewing = cViewing + cstr("\"All\" Categories");
				}
				else if (iDisplayLibraryType == 4 && bCheckboxFilters[1] && bCheckboxFilters[2] && bCheckboxFilters[3] && bCheckboxFilters[4])
				{
					cViewing = cViewing + cstr("\"All\" Categories");
				}
				else if (iDisplayLibraryType == 5 && bCheckboxFilters[1] && bCheckboxFilters[2] && bCheckboxFilters[3] && bCheckboxFilters[4])
				{
					cViewing = cViewing + cstr("\"All\" Categories");
				}
				else if (bCheckboxFilters[0] && bCheckboxFilters[1] && bCheckboxFilters[2] && bCheckboxFilters[3] && bCheckboxFilters[4])
				{
					cViewing = cViewing + cstr("\"All\" Categories");
				}
				else
				{
					if (bCheckboxFilters[1])
					{
						if (iCategories == 0)
							cViewing = cViewing + cstr("\"");
						if (iCategories++ > 0)
							cViewing = cViewing + cstr(", ");
						cViewing = cViewing + cstr("Characters");
					}
					if (bCheckboxFilters[2])
					{
						if (iCategories == 0)
							cViewing = cViewing + cstr("\"");
						if (iCategories++ > 0)
							cViewing = cViewing + cstr(", ");
						cViewing = cViewing + cstr("Scenery");
					}
					if (bCheckboxFilters[3])
					{
						if (iCategories == 0)
							cViewing = cViewing + cstr("\"");
						if (iCategories++ > 0)
							cViewing = cViewing + cstr(", ");
						cViewing = cViewing + cstr("Elements");
					}

					if (iDisplayLibraryType == 0 && bCheckboxFilters[0])
					{
						if (iCategories == 0)
							cViewing = cViewing + cstr("\"");
						if (iCategories++ > 0)
							cViewing = cViewing + cstr(", ");
						cViewing = cViewing + cstr("HUD Assets");
					}
					if (iDisplayLibraryType == 1 && bCheckboxFilters[0])
					{
						if (iCategories == 0)
							cViewing = cViewing + cstr("\"");
						if (iCategories++ > 0)
							cViewing = cViewing + cstr(", ");
						cViewing = cViewing + cstr("OGG");
					}

					if (bCheckboxFilters[4])
					{
						if (iCategories == 0)
							cViewing = cViewing + cstr("\"");
						if (iCategories++ > 0)
							cViewing = cViewing + cstr(", ");
						cViewing = cViewing + cstr("User Generated");
					}

					if (iCategories == 0)
						cViewing = "";
					else if (iCategories == 1)
					{
						cViewing = cViewing + cstr("\" Category");
					}
					else if (iCategories > 1)
					{
						cViewing = cViewing + cstr("\" Categories");
					}
				}
			}
			if (cViewing == "" && current_sortby == 1 && total_files_displayed_in_library > 0 )
			{
				if (iDisplayLibraryType == 0)
				{
					if (iDisplayLibrarySubType == 1)
						cViewing = cstr("Viewing ") + cstr(Str(total_files_displayed_in_library)) + cstr(" Animations from \"Showcase\"");
					else
						cViewing = cstr("Viewing ") + cstr(Str(total_files_displayed_in_library)) + cstr(" Objects from \"Showcase\"");
				}
				else
				{
					cViewing = cstr("Viewing ") + cstr(Str(total_files_displayed_in_library)) + cstr(" Items from \"Showcase\"");
				}
			}
			//order
			if (cViewing != "")
			{
				if (current_sortby == 1)
					cViewing = cViewing + " in A-Z order";
				else if (current_sortby == 0)
					cViewing = cViewing + " in category order";
				else if (current_sortby == 2)
					cViewing = cViewing + " in Z-A order";
				else if (current_sortby == 3)
					cViewing = cViewing + " in Old-New order";
				else if (current_sortby == 4)
					cViewing = cViewing + " in New-Old order";
			}
			ImGui::SetWindowFontScale(1.25);
			ImGui::TextCenter(cViewing.Get());
			ImGui::SetWindowFontScale(1.0);

			//----------------
			ImGui::Separator();
		}
		total_files_displayed_in_library = 0;

		//PE: Generate folder view.
		static int updates = 0;
		static bool bTreeViewInit = false;
		if (!bTreeViewInit || bTreeViewInitInNextFrame )
		{
			bTreeViewInitInNextFrame = false;
			//UNPIN
			extern std::vector<std::string> files_pinned_categories;
			files_pinned_categories.clear();
			getVectorFileContent("pinnedlist.ini", files_pinned_categories);

			static folder_info * mem_list[5000];
			static int mem_list_count = 0;

			cFolderItem *pNewFolder = &MainEntityList;
			pNewFolder = pNewFolder->m_pNext;

			//Free old structures.
			for (int i = 0; i < mem_list_count; i++)
			{
				if (mem_list[i])
				{
					delete mem_list[i];
					mem_list[i] = 0;
				}
			}
			mem_list_count = 0;
			root_folders.clear();
			updates++;

			if (pNewFolder)
			{
				cStr path_remove = pNewFolder->m_sFolderFullPath.Get();
				int ipath_remove_len = path_remove.Len();
				int count = 0;
				while (pNewFolder)
				{

					cStr path = pNewFolder->m_sFolderFullPath.Get();
					int itype = pNewFolder->iType;
					bool bDoubleEntityBank = false;
					cstr DoubleSearchName = "\\entitybank";
					if (itype == 1)
						DoubleSearchName = "\\audiobank";
					if (itype == 2)
						DoubleSearchName = "\\imagebank";
					if (itype == 3)
						DoubleSearchName = "\\videobank";
					if (itype == 4)
						DoubleSearchName = "\\scriptbank";
					if (itype == 5)
						DoubleSearchName = "\\particlesbank";
					if (itype == 6)
						DoubleSearchName = "\\charactercreatorplus\\animations";
					int DoubleSearchNameLen = DoubleSearchName.Len();

					char *finde = (char *)pestrcasestr(path.Get(), DoubleSearchName.Get()); //Support entitybank inside entitybank.
					if (finde)
					{
						finde += DoubleSearchNameLen;
						finde = (char *)pestrcasestr(finde, DoubleSearchName.Get());
						if (finde) bDoubleEntityBank = true;
					}

					if (!bDoubleEntityBank && path.Right(DoubleSearchNameLen) == DoubleSearchName)
					{
						ipath_remove_len = path.Len();
					}
					else
					{
						//char *final_name = path.Get();
						char *final_name = (char *)pestrcasestr(path.Get(), DoubleSearchName.Get()); //Support entitybank inside entitybank.

						//final_name += ipath_remove_len;
						final_name += DoubleSearchNameLen;
						if (*final_name == '\\')
							final_name++;

						bool bVisible = true;
						std::string dir_name = final_name;
						replaceAll(dir_name, "/", "\\");
						replaceAll(dir_name, ";", ""); //We will use ; for tokens later.
						int level = std::count(dir_name.begin(), dir_name.end(), '\\');
						replaceAll(dir_name, "\\", " - ");

						//PE: Hide showcase.
						if (!pestrcasestr(dir_name.c_str(), " - "))
						{
							if (pestrcasestr(dir_name.c_str(), "showcase"))
								bVisible = false;

							if (pestrcasestr(dir_name.c_str(), "_markers"))
								dir_name = "Game Elements";
						}

						if (dir_name.length() == 0)
						{
							static uint32_t maxerrors = 5;
							void timestampactivity(int i, char* desc_s);
							if (maxerrors > 0)
							{
								maxerrors--;
								timestampactivity(0, "DEBUG: dir_name.length() == 0");
							}
							//__debugbreak();
						}

						int is = root_folders.size();
						if (mem_list_count < 5000 && dir_name.length() < 256 && dir_name.length() > 0 )
						{
							folder_info *fi = new folder_info;
							if (fi)
							{
								mem_list[mem_list_count++] = fi;
								fi->level = level;
								fi->type = pNewFolder->iType;
								strcpy(fi->real_name, dir_name.c_str());
								strcpy(fi->show_name, dir_name.c_str());
								
								fi->show_name[0] = toupper(fi->show_name[0]);
								fi->id = ++count;// count++;
								fi->parentid = 0;
								fi->folders = 0;
								fi->pFolder = pNewFolder;
								fi->bPinned = false;
								fi->bUsed = false;
								std::transform(dir_name.begin(), dir_name.end(), dir_name.begin(), [](unsigned char c) { return tolower(c); });
								char tmp[2];
								tmp[0] = fi->type + '0';
								tmp[1] = 0;
								std::string realname = fi->real_name;
								realname = realname + tmp;
								for (std::vector<std::string>::iterator itf = files_pinned_categories.begin(); itf != files_pinned_categories.end(); ++itf)
								{
									if (itf->size() > 0)
									{
										if (strcmp(itf->c_str(), realname.c_str() ) == 0)
										{
											fi->bPinned = true;
											//Place it at the top.
											dir_name = "!" + dir_name;
											break;
										}
									}
								}

								dir_name = dir_name + tmp; //Only uniq for each folder type.
								replaceAll(dir_name, " - ", ";"); //Make it more easy to parse later.
								root_folders.insert(std::make_pair(dir_name, fi));
								if (is == root_folders.size())
									bVisible = false;
								if (!bVisible)
								{
									//
								}
							}
						}
					}
					pNewFolder = pNewFolder->m_pNext;
				}
			}

			//PE: Find all parentID's
			for (auto it = root_folders.begin(); it != root_folders.end(); ++it) {
				std::string folder = it->first;
				if (it->second->bPinned)
				{
					//Remove '!' infront.
					folder = folder.substr(1);
				}
				if (it->second->level > 0)
				{
					int itype = it->second->type;
					folder = folder.substr(0, folder.length() - 1);
					char * pch;
					char cTmp[512];
					strcpy(cTmp, folder.c_str());
					pch = strtok(cTmp, ";");
					int current_level = 1;
					std::string parent = cTmp;
					std::string search = parent;
					while (pch != NULL)
					{
						pch = strtok(NULL, ";"); //Here as we skip the first.

						if (pch)
						{
							char tmp[2];
							tmp[0] = itype + '0';
							tmp[1] = 0;
							std::string tmpsearch = search + tmp; //Only uniq for each folder type.

							//Find token in level-1.
							for (auto it2 = root_folders.begin(); it2 != root_folders.end(); ++it2)
							{
								if (it2->second->level == current_level - 1)
								{
									if (it2->second->type == itype)
									{
										std::string check = it2->first;
										if (it2->second->bPinned)
										{
											check = check.substr(1);
										}

										if (tmpsearch == check)
										{
											it->second->parentid = it2->second->id;
											it2->second->folders++;

											strcpy(it->second->show_name, pch);
											it->second->show_name[0] = toupper(it->second->show_name[0]);
										}
									}
								}
							}
						}

						search = search + ';';
						if (pch)
							search = search + pch;

						current_level++;
					}
				}
			}
			bTreeViewInit = true;
		}

		///ImGui::Columns(2);
		ImGui::BeginColumns("##myleftcategoripanel",2, ImGuiColumnsFlags_None);
		if (init_Left_Categories_Column_Width > 0)
		{
			//PE: Must match 230 for defaults to match the exact icons on screen.
			//PE: Perhaps add the percent adjustment later.
			ImGui::SetColumnWidth(0, 230.0f);
			init_Left_Categories_Column_Width--;
		}
		ImVec2 vWindowSize = ImGui::GetContentRegionAvail();


		//Display view all and favorites.
		if (seleted_tree_item >= 0)
		{
			bViewAllFolders = false;
			bDisplayFavorite = false;
			bDisplayProjectMedia = false;
		}
		if (bViewAllFolders)
		{
			bDisplayFavorite = false;
			bDisplayProjectMedia = false;
		}

		int selectable_items = 0;

		extern char szBeforeChangeWriteDir[MAX_PATH];
		char projectfolder[MAX_PATH];
		strcpy(projectfolder, "");
		if (strlen(szBeforeChangeWriteDir) > 0 && strlen(Storyboard.customprojectfolder) > 0)
		{
			strcpy(projectfolder, Storyboard.customprojectfolder);
			strcat(projectfolder, Storyboard.gamename);
			if (ImGui::Checkbox("Include Document Folder", &bIncludeDocumentFolderInRemoteProject))
			{
				//PE: Regular project update the library.
				extern int g_iRefreshLibraryFoldersAfterDelay;
				g_iRefreshLibraryFoldersAfterDelay = 10;
			}
			selectable_items++;
		}

		if (ImGui::Selectable("View All", bViewAllFolders) || bSelectLibraryViewAll )
		{
			bSelectLibraryViewAll = false;
			seleted_tree_item = -1;
			strcpy(cSearchAllEntities[0], "");
			//Enable all filters.
			bCheckboxFilters[0] = true;
			bCheckboxFilters[1] = true;
			bCheckboxFilters[2] = true;
			bCheckboxFilters[3] = true;
			bCheckboxFilters[4] = true;
			bDisplayProjectMedia = false;
			bDisplayFavorite = false;
			bViewShowcase = false;
			bViewPurchased = false;
			bViewAllFolders = true;
			bUpdateSearchSorting = true;
			bUpdateSearchScrollbar = true;
		}
		selectable_items++;

		// only show SHOWCASE and PURCHASED as relating to objects (for now)
		if (iDisplayLibraryType == 0)
		{
			if (iDisplayLibrarySubType == 1)
			{
				// no showcase or purchased for animations for now
			}
			else
			{
				if (stricmp(cSearchAllEntities[0], "Purchased") != 0) bViewPurchased = false;
				if (ImGui::Selectable("Purchased", &bViewPurchased, 0))
				{
					// force library to purchased view, and refresh too
					process_gotopurchaedandrefreshtopurchases(true);
				}
				selectable_items++;
			}
		}
		else
		{
			// behavior has purchased option (from asset store)
			if (iDisplayLibraryType == 4)
			{
				if (stricmp(cSearchAllEntities[0], "Purchased") != 0) bViewPurchased = false;
				if (ImGui::Selectable("Purchased", &bViewPurchased, 0))
				{
					// force library to purchased view, and refresh too
					process_gotopurchaedandrefreshtopurchases(true);
				}
				selectable_items++;
			}
		}

		// only show project media if in a remote project now
		extern StoryboardStruct Storyboard;
		if ( strlen(Storyboard.customprojectfolder) > 0 )
		{
			bool bProjectMediaSelected = strstr(cSearchAllEntities[0], "project");
			if (ImGui::Selectable("Current Project##projectmedia", &bProjectMediaSelected, 0))
			{
				seleted_tree_item = -1;
				strcpy(cSearchAllEntities[0], "");
				bDisplayProjectMedia = true;
				bDisplayFavorite = false;
				bViewAllFolders = false;
				bViewShowcase = false;
				bUpdateSearchSorting = true;
				bUpdateSearchScrollbar = true;
			}
			selectable_items++;
		}

		if (ImGui::Selectable("Favorites##favourites", &bDisplayFavorite, 0))
		{
			seleted_tree_item = -1;
			strcpy(cSearchAllEntities[0], "");
			bDisplayProjectMedia = false;
			bDisplayFavorite = true;
			bViewAllFolders = false;
			bViewShowcase = false;
			bUpdateSearchSorting = true;
			bUpdateSearchScrollbar = true;
		}
		selectable_items++;
		static float setadder = 0;
		if(selectable_items <= 3)
			ImGui::BeginChild("##LeftPanelCategories", ImVec2(0, vWindowSize.y - (67.0f + setadder)), false, iGenralWindowsFlags);
		else if (selectable_items <= 4)
			ImGui::BeginChild("##LeftPanelCategories", ImVec2(0, vWindowSize.y - (95.0f)), false, iGenralWindowsFlags);
		else if (selectable_items <= 5)
			ImGui::BeginChild("##LeftPanelCategories", ImVec2(0, vWindowSize.y - (113.0f)), false, iGenralWindowsFlags);
		else
			ImGui::BeginChild("##LeftPanelCategories", ImVec2(0, vWindowSize.y - (133.0f)), false, iGenralWindowsFlags);

		if (iDisplayLibraryType == 0 && iDisplayLibrarySubType == 1)
		{
			// animations
			DoTreeNode(0, "Noncharacter", "Purchased", sTriggerCategorySelect.Get());
		}
		else
		{
			if (iDisplayLibraryType == 4)
			{
				// behaviors
				DoTreeNode(0, "AllNoneBehaviorFolders", "Community", sTriggerCategorySelect.Get());
			}
			else
			{
				// the rest
				DoTreeNode(0, "Community", "Purchased", sTriggerCategorySelect.Get());
			}
		}
		sTriggerCategorySelect = "";

		// list community folder contents separately
		DoTreeNode(0, "", "", "", "Community");

		ImGui::EndChild();
		ImGui::NextColumn();

		//PE: Begin child window.
		ImVec2 vWinSize = ImGui::GetContentRegionAvail();
		ImGui::BeginChild("##cSearchAllEntitiesBegin", ImVec2(0, vWinSize.y - 84.0f), false, iGenralWindowsFlags); //- 68.0f

		ImVec2 oldCursor = ImGui::GetCursorPos();

		int iIconVisiblePosY = ImGui::GetWindowSize().y + ImGui::GetScrollY() + media_icon_size;

		bool bFirstShiftHasBeenSeen = false;
		bool bAnySelectedItemsAvailable = false;
		static cFolderItem::sFolderFiles * firstShiftFile = NULL;
		static cFolderItem::sFolderFiles * lastShiftFile = NULL;
		static bool bInContextThumb;
		bInContextThumb = false;

		if (1)
		{
			static std::vector< std::pair<std::string, cFolderItem::sFolderFiles *>> sorted_files;
			static std::vector< std::pair<std::string, cFolderItem::sFolderFiles*>> remoteproject_files;
			if (sorted_files.size() == 0)
				bUpdateSearchSorting = true;

			cFolderItem *pNewFolder = &MainEntityList;

			static std::vector<cFolderItem *> all_folders;
			//Check if we got new files.
			if (all_folders.size() > 0)
			{
				for (int l = 0; l < all_folders.size(); l++)
				{
					pNewFolder = all_folders[l];
					if (pNewFolder)
					{
						//PE: Check here if we need to reload the folder, for new files.
						if (pNewFolder->m_fLastTimeUpdate < MAXTimer())
						{
							pNewFolder->m_fLastTimeUpdate = MAXTimer() + 4000; //Check every 4-6 sec.
							pNewFolder->m_fLastTimeUpdate += rand() % 2000; //Make sure we dont check folders in same cycle.
							struct stat sb;
							if (PathExist(pNewFolder->m_sFolderFullPath.Get()))
							{
								if (stat(pNewFolder->m_sFolderFullPath.Get(), &sb) == 0)
								{
									if (sb.st_mtime != pNewFolder->m_tFolderModify)
									{
										pNewFolder->m_tFolderModify = sb.st_mtime;
										RefreshEntityFolder(pNewFolder->m_sFolderFullPath.Get(), pNewFolder);
										bUpdateSearchSorting = true;
									}
								}
							}
						}

					}
				}
			}


			bool bDoBackbufferUpdate = false;

			//####################################################
			//#### Check if we need to update search results. ####
			//####################################################

			pNewFolder = &MainEntityList;
			pNewFolder = pNewFolder->m_pNext;
			if ((bCheckGotoPreview || bUpdateSearchSorting || bUpdateSearchSortingNextFrame) && pNewFolder)
			{
				sorted_files.clear();
				remoteproject_files.clear();

				pNewFolder = pNewFolder->m_pNext;

				cStr path_remove = pNewFolder->m_sFolderFullPath.Get();
				int ipath_remove_len = path_remove.Len();

				if (iDisplayLibraryType == 0) //Object only remember last search.
				{
					if (strlen(cSearchAllEntities[i]) < 256)
					{
						strcpy(pref.cRememberLastSearchObjects, cSearchAllEntities[0]);
					}
					for (int i = 0; i < 5; i++)
						pref.iCheckboxFilters[i] = bCheckboxFilters[i];

					//bCheckboxFilters[0] - 5 and 9=fav.
				}

				while (pNewFolder)
				{
					int iCompareType = iDisplayLibraryType;
					if (iDisplayLibraryType == 0 && iDisplayLibrarySubType == 1) iCompareType = 6;
					if (pNewFolder->iType == iCompareType)
					{
						//PE: Full path can now change in the middle of the list , so:
						cStr path = pNewFolder->m_sFolderFullPath.Get();

						if (pNewFolder->m_iEntityOffset > 0)
							ipath_remove_len = pNewFolder->m_iEntityOffset;


						bool isMarkers = false;
						bool bDisplayEverythingHere = false;
						bool bHideEverythingHere = false;
						bool bSentToTopOfList = false;
						if (i == 0 && strlen(cSearchAllEntities[i]) > 0)
						{
							//When search disable fixed tags search.
							bDisplayEverythingHere = false;
							bHideEverythingHere = false;
						}
						if (i == 1 && strlen(cSearchAllEntities[i]) > 0)
						{
							//When search disable fixed tags search.
							bDisplayEverythingHere = false;
							bHideEverythingHere = false;
						}

						//PE: Search only inside selected category did not work so good, if you type "box" you want all boxes.
						//PE: So here a searchterm overwrite the category checkbox selections.
						if (strlen(cSearchAllEntities[i]) == 0)
						{
							cstr cIgnoreFrom = "entitybank\\";
							if (iDisplayLibraryType == 1)
								cIgnoreFrom = "audiobank\\";
							if (iDisplayLibraryType == 2)
								cIgnoreFrom = "imagebank\\";
							if (iDisplayLibraryType == 3)
								cIgnoreFrom = "videobank\\";
							if (iDisplayLibraryType == 4)
								cIgnoreFrom = "scriptbank\\";
							if (iDisplayLibraryType == 5)
								cIgnoreFrom = "particlesbank\\";
							if (iDisplayLibraryType == 0 && iDisplayLibrarySubType==1)
								cIgnoreFrom = "charactercreatorplus\\animations\\";
							int iIgnoreLen = cIgnoreFrom.Len();

							char *find = (char *)pestrcasestr(pNewFolder->m_sFolderFullPath.Get(), cIgnoreFrom.Get());
							if (find) find += iIgnoreLen;
							else find = pNewFolder->m_sFolderFullPath.Get();

							if (find)
							{
								if (i == 0 && bCheckboxFilters[0]) //"showcase"
								{
									if (pestrcasestr(find, "hud assets"))
										bDisplayEverythingHere = true;
								}
								if (i == 0 && bCheckboxFilters[1]) //"Character"
								{
									if (pestrcasestr(find, "Character"))
										bDisplayEverythingHere = true;
									if (iDisplayLibraryType == 1)
									{
										if (pestrcasestr(find, "voices"))
											bDisplayEverythingHere = true;
									}
									if (iDisplayLibraryType == 4)
									{
										if (pestrcasestr(find, "people"))
										{
											if (!pestrcasestr(find, "people\\ai"))
												bDisplayEverythingHere = true;
										}
									}
								}
								if (i == 0 && bCheckboxFilters[2]) //"Scene"
								{
									if (pestrcasestr(find, "Objects"))
										bDisplayEverythingHere = true;
									if (pestrcasestr(find, "Weapons"))
										bDisplayEverythingHere = true;
									if (iDisplayLibraryType == 1)
									{
										//For now.
										if (pestrcasestr(find, "cellar"))
											bDisplayEverythingHere = true;
										if (pestrcasestr(find, "misc"))
											bDisplayEverythingHere = true;
									}
								}
								if (i == 0 && bCheckboxFilters[4]) //"User Generated"
								{
									if (pestrcasestr(find, "User"))
										bDisplayEverythingHere = true;
									if (iDisplayLibraryType == 1)
									{
										if (pestrcasestr(find, "recordings"))
											bDisplayEverythingHere = true;
									}
								}

								if (i == 0 && bCheckboxFilters[3]) //"elements"
								{
									bHideEverythingHere = false;
									if (!bDisplayEverythingHere)
									{
										bDisplayEverythingHere = true;
										if (pestrcasestr(find, cAllFilters[0]))
											bHideEverythingHere = true;
										if (pestrcasestr(find, cAllFilters[1]))
											bHideEverythingHere = true;
										if (pestrcasestr(find, cAllFilters[2]))
											bHideEverythingHere = true;
										if (pestrcasestr(find, cAllFilters[3]))
											bHideEverythingHere = true;
										if (pestrcasestr(find, cAllFilters[4]))
											bHideEverythingHere = true;
										if (iDisplayLibraryType == 1)
										{
											if (pestrcasestr(find, "voices"))
												bHideEverythingHere = true;
											if (pestrcasestr(find, "recordings")) //"Scene"
												bHideEverythingHere = true;
											//For now.
											if (pestrcasestr(find, "cellar"))
												bHideEverythingHere = true;
											if (pestrcasestr(find, "misc"))
												bHideEverythingHere = true;
										}
									}
								}

								if (current_sortby == 1) //0 a-z now also showcase showcase
								{
									if (pestrcasestr(find, "showcase"))
									{
										bDisplayEverythingHere = true;
										bSentToTopOfList = true;
									}
								}
								else
								{
									if (!bDisplayFavorite)
									{
										if (pestrcasestr(find, "showcase"))
										{
											bHideEverythingHere = true;
										}
									}
								}
							}
						}

						if (bDisplayFavorite)
						{
							//Favorite display from all folders.
							bDisplayEverythingHere = true;
						}

						char *final_name = path.Get();
						final_name += ipath_remove_len;
						if (*final_name == '\\')
							final_name++;

						std::string path_for_filename = final_name;
						std::string dir_name = final_name;
						replaceAll(dir_name, "\\", " - ");

						bool bSearchGameElements = false;
						if (pestrcasestr(dir_name.c_str(), "_markers"))
							bSearchGameElements = true;

						// behavior view with search term
						if (iDisplayLibraryType == 4)
						{
							if (strlen(cSearchAllEntities[i]) > 0)
							{
								// if viewing purchased, always show them
								if (pestrcasestr("Purchased", cSearchAllEntities[i]))
								{
									bDisplayEverythingHere = true;
									if (strlen(dir_name.c_str()) == 0)										bDisplayEverythingHere = false;
									if (strnicmp(dir_name.c_str(), "ai", strlen("ai")) == NULL )			bDisplayEverythingHere = false;
									if (strnicmp(dir_name.c_str(), "animals", strlen("animals")) == NULL)	bDisplayEverythingHere = false;
									if (strnicmp(dir_name.c_str(), "effects", strlen("effects")) == NULL)	bDisplayEverythingHere = false;
									if (strnicmp(dir_name.c_str(), "gfx", strlen("gfx")) == NULL)			bDisplayEverythingHere = false;
									if (strnicmp(dir_name.c_str(), "horror", strlen("horror")) == NULL)		bDisplayEverythingHere = false;
									if (strnicmp(dir_name.c_str(), "images", strlen("images")) == NULL)		bDisplayEverythingHere = false;
									if (strnicmp(dir_name.c_str(), "markers", strlen("markers")) == NULL)	bDisplayEverythingHere = false;
									if (strnicmp(dir_name.c_str(), "objects", strlen("objects")) == NULL)	bDisplayEverythingHere = false;
									if (strnicmp(dir_name.c_str(), "people", strlen("people")) == NULL)		bDisplayEverythingHere = false;
									if (strnicmp(dir_name.c_str(), "puzzle", strlen("puzzle")) == NULL)		bDisplayEverythingHere = false;
									if (strnicmp(dir_name.c_str(), "rpg", strlen("rpg")) == NULL)			bDisplayEverythingHere = false;
									if (strnicmp(dir_name.c_str(), "global", strlen("global")) == NULL)		bDisplayEverythingHere = false;
									if (strnicmp(dir_name.c_str(), "user", strlen("user")) == NULL)			bDisplayEverythingHere = false;
									if (strnicmp(dir_name.c_str(), "weather", strlen("weather")) == NULL)	bDisplayEverythingHere = false;
								}
							}
						}

						if (!isMarkers && i == 0 && !bDisplayEverythingHere && !bHideEverythingHere && strlen(cSearchAllEntities[i]) > 0) 
						{
							if (iDisplayLibraryType > 0) //Other media types only search sub folder names.
							{
								if (pestrcasestr(dir_name.c_str(), cSearchAllEntities[i]))
									bDisplayEverythingHere = true;
							}
							else
							{
								//LB011125-could pick up matches outside of MAX relative folders (i.e. C:\\Users\)
								//if (pestrcasestr(pNewFolder->m_sFolderFullPath.Get(), cSearchAllEntities[i]))
								//	bDisplayEverythingHere = true;
								//else if (pestrcasestr(dir_name.c_str(), cSearchAllEntities[i]))
								//	bDisplayEverythingHere = true;

								// exact matches with dir_name fine
								if (pestrcasestr(dir_name.c_str(), cSearchAllEntities[i])) 
									bDisplayEverythingHere = true;

								// also matches with m_sFolderFullPath, but only after MAX root folder
								LPSTR pRelevantPathString = pNewFolder->m_sFolderFullPath.Get();
								char cMaxPathString[MAX_PATH];
								strcpy(cMaxPathString, g.fpscrootdir_s.Get());
								LPSTR pch = (LPSTR)pestrcasestr(pRelevantPathString, cMaxPathString);
								if (pch)
								{
									pch += strlen(cMaxPathString);
									if (pestrcasestr(pch, cSearchAllEntities[i]))
										bDisplayEverythingHere = true;
								}

								// and of course the writables area
								strcpy(cMaxPathString, pref.cCustomWriteFolder);
								pch = (LPSTR)pestrcasestr(pRelevantPathString, cMaxPathString);
								if (pch)
								{
									pch += strlen(cMaxPathString);
									if (pestrcasestr(pch, cSearchAllEntities[i]))
										bDisplayEverythingHere = true;
								}
							}
						}

						//PE: Check here if we need to reload the folder, for new files.
						if (pNewFolder->m_fLastTimeUpdate < MAXTimer())
						{
							pNewFolder->m_fLastTimeUpdate = MAXTimer() + 4000; //Check every 4-6 sec.
							pNewFolder->m_fLastTimeUpdate += rand() % 2000; //Make sure we dont check folders in same cycle.
							struct stat sb;
							if (stat(pNewFolder->m_sFolderFullPath.Get(), &sb) == 0) 
							{
								if (sb.st_mtime != pNewFolder->m_tFolderModify) 
								{
									pNewFolder->m_tFolderModify = sb.st_mtime;
									RefreshEntityFolder(pNewFolder->m_sFolderFullPath.Get(), pNewFolder);
								}
							}
						}

						if (pNewFolder->m_pFirstFile)
						{
							bool bDisplayText = true;

							cFolderItem::sFolderFiles * myfiles = NULL;

							myfiles = pNewFolder->m_pFirstFile->m_pNext;

							while (myfiles)
							{
								bool bIsVisible = true;
								bool bForceVisible = false;
								bool bIsPhonetic = false;
								int iPhoneticDistance = 999;
								if (i == 0 && isMarkers) bIsVisible = false;
								if (i == 1 && !isMarkers) bIsVisible = false;
								if (i == 0) bIsVisible = false; //Default to not visible.

								//PE: Clear any selection when updating list.
								myfiles->iFlags = 0;

								if (!bLargePreview && bCheckGotoPreview && sGotoPreviewWithFile != "")
								{
									cstr test = pNewFolder->m_sFolderFullPath + "\\" + myfiles->m_sName;
									if (pestrcasestr(test.Get(), sGotoPreviewWithFile.Get()))
									{
										sStartLibrarySearchString = "user";
										iLastDisplayLibraryType = -1;
										//Refresh with new search.
										//Load dbo and display large preview.
										pPreviewFile = myfiles;
										bLargePreview = true;
										bDoBackbufferUpdate = true;
										bIsVisible = true;
										bForceVisible = true;
									}
								}

								if (i == 0 && strlen(cSearchAllEntities[i]) > 0) {

									if (iDisplayLibraryType == 4 && stricmp(cSearchAllEntities[i], "global") == 0 )
									{
										bDisplayEverythingHere = false;
										if (pestrcasestr(dir_name.c_str(), "global"))
										{
											bIsVisible = true;
										}
									}
									else
									{
										if (pestrcasestr(myfiles->m_sBetterSearch.Get(), cSearchAllEntities[i]))
											bIsVisible = true;
										if (!bIsVisible) // else current_sortby == 4)
										{
											if (bAdvancedFPEFeatures && myfiles->m_sFPEKeywords.Len() > 0)
											{
												if (pestrcasestr(myfiles->m_sFPEKeywords.Get(), cSearchAllEntities[i]))
													bIsVisible = true;
											}
										}
									}
									if (!bIsVisible && bSearchGameElements)
									{
										if (pestrcasestr(cSearchAllEntities[i], "Game Elements"))
											bIsVisible = true;
									}

								}

								if (bDisplayEverythingHere)
									bIsVisible = true;
								if (bHideEverythingHere)
									bIsVisible = false;

								if (bDisplayFavorite && !myfiles->bFavorite)
								{
									bIsVisible = false;
								}

								if (bDisplayProjectMedia == true)
								{
									// if not in project folder, hide
									char pFindProjectEntityFolder[MAX_PATH];
									extern StoryboardStruct Storyboard;
									strcpy(pFindProjectEntityFolder, Storyboard.customprojectfolder);
									strcat(pFindProjectEntityFolder, Storyboard.gamename);
									LPSTR pFileFolderToCheck = pNewFolder->m_sFolderFullPath.Get();
									if (strnicmp(pFileFolderToCheck, pFindProjectEntityFolder, strlen(pFindProjectEntityFolder)) != NULL)
									{
										bIsVisible = false;
									}
								}

								uniqueId++;

								if (bAdvancedFPEFeatures && current_sortby == 5 || current_sortby == 6)
								{
									//Hide files that do not "yet" have a DBO and then no poly count.
									if (myfiles->m_iFPEDBOFileSize <= 0)
										bIsVisible = false;
								}

								// always hide files ending with "_smartchild", they are only useful to the smart objects construction
								// and no good on their own for the vast majority of cases
								if (strnicmp(myfiles->m_sNameFinal.Get() + strlen(myfiles->m_sNameFinal.Get()) - 11, "_smartchild", 11) == NULL)
								{
									bIsVisible = false;
									bForceVisible = false;
								}

								if (bIsVisible || bForceVisible)
								{
									// set sortby mode = current_sortby
									// 0 = showcase
									// 1 = no sorting, category order and files A-Z inside each category. 0
									// 2 = A-Z 1
									// 3 = Z-A 2
									// 4 = Date Old-New 5
									// 5 = Date New-Old 6
									// 6 = Poly low 7
									// 7 = Poly high 8

									//Push files entry into a sort list.
									std::string SortBy = Lower(myfiles->m_sName.Get());

									if (current_sortby == 0) //|| current_sortby == 1
									{
										//Showcase and category display.
										SortBy = dir_name + ":" + SortBy;
									}
									if (seleted_tree_item >= 0 && pestrcasestr(cSearchAllEntities[i], dir_name.c_str()))
									{
										//PE: This search is from a click on a category, sort normally.
										//SortBy = dir_name + ":" + SortBy;
									}
									else if (strlen(cSearchAllEntities[i]) > 0) // && current_sortby == 3 || current_sortby == 4
									{
										std::string AddToSort = "";
										std::string SortSearch = Lower(cSearchAllEntities[i]);
									}

									if (current_sortby == 3 || current_sortby == 4)
									{
										char buffer[80];
										struct tm * timeinfo = localtime(&myfiles->m_tFileModify);
										strftime(buffer, 80, "%F%H%M", timeinfo);
										SortBy = buffer;
									}

#ifdef PHONETICSEARCH
									if (bIsPhonetic) // && current_sortby == 4
									{
										//Put in bottom of results.
										std::string AddToSort = "";
										if (iPhoneticDistance < 10)
											AddToSort = "0";
										AddToSort = AddToSort + std::to_string(iPhoneticDistance);
										SortBy = "ZZZZ" + AddToSort + SortBy;
									}
#endif

									if (bSentToTopOfList)
									{
										SortBy = "0000" + SortBy;
									}


									if (bAdvancedFPEFeatures && current_sortby == 5 || current_sortby == 6)
									{
										//Sort by poly count.
										int iSize = myfiles->m_iFPEDBOFileSize;
										if (iSize > 0)
										{
											std::string AddToSort = "";
											if (iSize < 10)
												AddToSort = "0000000";
											else if (iSize < 100)
												AddToSort = "000000";
											else if (iSize < 1000)
												AddToSort = "00000";
											else if (iSize < 10000)
												AddToSort = "0000";
											else if (iSize < 100000)
												AddToSort = "000";
											else if (iSize < 1000000)
												AddToSort = "00";
											else if (iSize < 10000000)
												AddToSort = "0";
											AddToSort = AddToSort + std::to_string(iSize);
											SortBy = AddToSort + SortBy;
										}
									}

									//PE: Remove dublicates here, if using 
									bool bDuplicate = false;
									bool bRemoteProject = false;
									extern char szBeforeChangeWriteDir[MAX_PATH];
 									if (strlen(szBeforeChangeWriteDir) > 0 && strlen(projectfolder) > 0 )
									{
										LPSTR pFileFolderToCheck = pNewFolder->m_sFolderFullPath.Get();
										if (myfiles && strnicmp(pFileFolderToCheck, projectfolder, strlen(projectfolder)) == NULL)
										{
											bRemoteProject = true;
										}
										else
										{
											//PE: Check if we already added this to remoteproject_files.
											for (int loop = 0; loop < remoteproject_files.size(); loop++)
											{
												if (remoteproject_files[loop].second->m_sName == myfiles->m_sName)
												{
													//PE: Check full folder.
													char check[MAX_PATH];
													strcpy(check, remoteproject_files[loop].second->m_sPath.Get());
													const char* find = pestrcasestr(check, "\\files\\");
													if (find)
													{
														if (pestrcasestr(myfiles->m_sPath.Get(),find))
														{
															//PE: Same path , mark as duplicate. and prefer remoteproject file.
															bDuplicate = true;
															break;
														}
													}
												}
											}
										}
									}
									if (bRemoteProject)
									{
										remoteproject_files.push_back(std::make_pair(SortBy, myfiles));
									}
									if(!bDuplicate)
										sorted_files.push_back(std::make_pair(SortBy, myfiles));

									//Map pNewFolder to files entry.
									myfiles->pNewFolder = pNewFolder;
									myfiles->uniqueId = uniqueId;
								}

								myfiles = myfiles->m_pNext;

							}
						}
					}
					pNewFolder = pNewFolder->m_pNext;

				} // while folders.

			} //bUpdateSearchSorting

			if ((bUpdateSearchSorting || bUpdateSearchSortingNextFrame) && sorted_files.size() > 0)
			{
				// set sortby mode = current_sortby
				// 0 = showcase
				// 1 = no sorting, category order and files A-Z inside each category. 0
				// 2 = A-Z 1
				// 3 = Z-A 2
				// 4 = Date Old-New 5
				// 5 = Date New-Old 6
				// 6 = Poly low 7
				// 7 = Poly high 8

				if (current_sortby >= 0)
				{
					std::sort(sorted_files.begin(), sorted_files.end());
					if (current_sortby == 4 || current_sortby == 2 || current_sortby == 6)
						std::reverse(sorted_files.begin(), sorted_files.end());
				}
			}

			bUpdateSearchSortingNextFrame = false;
			bUpdateSearchSorting = false;

			//#####################################################
			//#### Start the actual display of the object list ####
			//#####################################################

			cwidth = ImGui::GetContentRegionAvailWidth();
			iColumnsWidth = cwidth / pref.iSetColumnsEntityLib;
			iColumnsWidth -= 8.0; //padding

			if (iColumnsWidth < 100)
				iColumnsWidth = 100;
			media_icon_size = iColumnsWidth - 6.0;
			media_icon_size_y = media_icon_size * 0.5625f; //PE: 1920x1080 ratio.


			//float fWinWidth = ImGui::GetWindowSize().x - 10.0; // Flicker - ImGui::GetCurrentWindow()->ScrollbarSizes.x;
			float fWinWidth = ImGui::GetContentRegionAvailWidth() - 10.0; // Flicker - ImGui::GetCurrentWindow()->ScrollbarSizes.x;
			if (iColumnsWidth >= fWinWidth && fWinWidth > media_icon_size)
			{
				iColumnsWidth = fWinWidth;
			}
			//int iColumns = (int)(ImGui::GetWindowSize().x / (iColumnsWidth));
			int iColumns = (int)(ImGui::GetContentRegionAvailWidth() / (iColumnsWidth));
			if (iColumns <= 1)
				iColumns = 1;

			ImGui::BeginColumns("##filescolumns4entities", iColumns, ImGuiColumnsFlags_NoBorder);

			if (bUpdateSearchScrollbar)
			{
				ImGui::SetScrollY(0);
				bUpdateSearchScrollbar = false;
			}

			//PE: Control scrollbar here.
			static bool bTriggerAnotherEndKey = false;
			if (bTriggerAnotherEndKey)
			{
				ImGui::SetScrollY(ImGui::GetScrollMaxY() * 2.0);
				bTriggerAnotherEndKey = false;
			}
			if (bLastEntityGotFocus)
			{
				//PE: https://github.com/TheGameCreators/GameGuruRepo/issues/1239

				if (!io.KeyShift)
				{
					if (!io.KeyCtrl) // CTRL+A
					{
						bool bChanged = false;
						float sy = ImGui::GetScrollY();

						if(	ImGui::IsKeyPressed(38, true) ) //UP
						{
							if (sy > 0) sy = sy - ImGui::GetFontSize();
							bChanged = true;
						}
						if (ImGui::IsKeyPressed(40, true)) //Down
						{
							if (sy < ImGui::GetScrollMaxY()) sy = sy + ImGui::GetFontSize();
							bChanged = true;
						}
						if (ImGui::IsKeyPressed(33, true)) //PGUP
						{
							if (sy > 0) sy = sy - (ImGui::GetContentRegionAvail().y + 9.0);
							bChanged = true;
						}
						if (ImGui::IsKeyPressed(34, true)) //PGDOWN
						{
							if (sy < ImGui::GetScrollMaxY()) sy = sy + (ImGui::GetContentRegionAvail().y + 9.0);
							bChanged = true;
						}
						if (ImGui::IsKeyPressed(36, true)) //HOME
						{
							sy = 0;
							bChanged = true;
						}
						if (ImGui::IsKeyPressed(35, true)) //END
						{
							sy = ImGui::GetScrollMaxY() * 2.0;
							bChanged = true;
							bTriggerAnotherEndKey = true; //PE: Needed.
						}

						if (bChanged)
						{
							if (sy < 0) sy = 0;
							ImGui::SetScrollY(sy);
						}
					}
				}
			}


			//PE: Create a list of all selected objects.
			selected_library_fpe.clear();

			//#####################################
			//#### Created selected files list ####
			//#####################################

			static int all_folders_check = 20;
			if (all_folders_check-- < 0) all_folders_check = 20;
			all_folders.clear();
			for (int iLoop = 0; iLoop < sorted_files.size(); iLoop++)
			{
				//if (myfiles->iFlags == 1)
				if (sorted_files[iLoop].second)
				{
					cFolderItem::sFolderFiles * myfiles = NULL;
					myfiles = sorted_files[iLoop].second;
					if (all_folders_check == 0 && myfiles && sorted_files[iLoop].second->pNewFolder)
					{
						if (std::find(all_folders.begin(), all_folders.end(), sorted_files[iLoop].second->pNewFolder) == all_folders.end()) {
							all_folders.push_back(sorted_files[iLoop].second->pNewFolder);
						}
					}

					if (myfiles && myfiles->iFlags == 1)
					{

						pNewFolder = sorted_files[iLoop].second->pNewFolder;
						int ipath_remove_len = pNewFolder->m_sFolderFullPath.Len();

						cStr path = pNewFolder->m_sFolderFullPath.Get();

						if (pNewFolder->m_iEntityOffset > 0)
							ipath_remove_len = pNewFolder->m_iEntityOffset;

						//
						char *final_name = path.Get();
						final_name += ipath_remove_len;
						if (*final_name == '\\')
							final_name++;

						std::string path_for_filename = final_name;

						std::string sFpeName = path_for_filename.c_str();
						sFpeName = sFpeName + "\\" + myfiles->m_sName.Get();
						t.addentityfile_s = sFpeName.c_str();
						selected_library_fpe.insert(std::make_pair(t.addentityfile_s.Get(), 0));
					}
				}
			}


			static bool bAddAllVisibleTriggerMessage = false;
			if (bAddAllVisibleTriggerMessage && selected_library_fpe.size() >= 100)
			{
				//Check if we need to triggerr a warning.
				if (MessageBoxA(NULL, "You have selected more then 100 object, are you sure you want to continue ?", "Warning", MB_YESNO | MB_TOPMOST) != IDYES)
				{
					//Clear all flags again.
					for (int iLoop = 0; iLoop < sorted_files.size(); iLoop++)
					{
						if (sorted_files[iLoop].second)
						{
							cFolderItem::sFolderFiles * myfiles = NULL;
							myfiles = sorted_files[iLoop].second;
							if (myfiles && myfiles->iFlags == 1)
								myfiles->iFlags = 0;
						}
					}
				}
			}

			bAddAllVisibleTriggerMessage = false;

			int ipath_remove_len = 0;
			pNewFolder = NULL;
			static bool bAddAllVisible = false;
			bAddAllVisible = false;

			static int iVideoPreviewThumbID = 0;
			static int iVideoGenerateImageID = 0;
			static cFolderItem::sFolderFiles * pVideoGeneratingFile = NULL;
			static cstr sVideoSaveName = "";

			//Check if we are generating a video preview.
			if (iDisplayLibraryType == 3 && iVideoGetFirstFrame > 0)
			{

				if (iVideoGetFirstFrame > 0) {

					UpdateAllAnimation();
					SetVideoVolume(0.1);

					if (iVideoGetFirstFrame == 1) {

						PauseAnim(iVideoPreviewThumbID);
						SetVideoVolume(100.0); //Turn back volume.
						//Capture frame.
						ID3D11ShaderResourceView* lpVideoTextureView = GetAnimPointerView(iVideoPreviewThumbID);
						LPGGSURFACE lpVideoTexture = GetAnimPointerTexture(iVideoPreviewThumbID);

						float fVideoW = GetAnimWidth(iVideoPreviewThumbID);
						float fVideoH = GetAnimHeight(iVideoPreviewThumbID);

						if (iVideoPreviewThumbID > 0 && lpVideoTexture && iVideoGenerateImageID > 0) {
							float fRatio = 1.0f / (fVideoW / fVideoH);

							LPGGSURFACE pOldBackBuffer = g_pGlob->pCurrentBitmapSurface;
							 g_pGlob->pCurrentBitmapSurface = lpVideoTexture;
							SetGrabImageMode(1);
							// delete previous thumbnail
							if (GetImageExistEx(iVideoGenerateImageID))
							{
								DeleteImage(iVideoGenerateImageID);
							}

							GrabImage(iVideoGenerateImageID, 0, 0, fVideoW, fVideoH, 0);
							SetGrabImageMode(0);
							g_pGlob->pCurrentBitmapSurface = pOldBackBuffer;

							if (GetImageExistEx(iVideoGenerateImageID))
							{
								//The thumb name will include 512x288 but we can scale it to the actual size when we know its a video.
								SaveImage(sVideoSaveName.Get(), iVideoGenerateImageID);
								pVideoGeneratingFile->iPreview = iVideoGenerateImageID;
								pVideoGeneratingFile->iBigPreview = iVideoGenerateImageID;
							}
						}

						if (AnimationExist(iVideoPreviewThumbID)) {
							if (AnimationPlaying(iVideoPreviewThumbID))
								StopAnimation(iVideoPreviewThumbID);
							DeleteAnimation(iVideoPreviewThumbID);
						}
						iVideoPreviewThumbID = 0;
						iVideoGenerateImageID = 0;
					}
					iVideoGetFirstFrame--;
				}

			}

			static int iWaitFramesBeforeProceed = 0;
			if (iDisplayLibraryType == 5 && iWaitFramesBeforeProceed > 0)
			{
				iWaitFramesBeforeProceed--;
			}

			//###################
			//#### MAIN LOOP ####
			//###################

			static bool bSecondScrollActive = false;
			bSecondScrollActive = true;

			total_files_displayed_in_library = 0;
			for (int iLoop = 0; iLoop < sorted_files.size(); iLoop++)
			{
				bool bValid = false;

				if (sorted_files.size() > 0)
				{
					if (sorted_files[iLoop].second)
					{
						pNewFolder = sorted_files[iLoop].second->pNewFolder;
						ipath_remove_len = pNewFolder->m_sFolderFullPath.Len();
						bValid = true;
					}
				}

				if (bValid)
				{

					//PE: Full path can now change in the middle of the list , so:
					cStr path = pNewFolder->m_sFolderFullPath.Get();

					if (pNewFolder->m_iEntityOffset > 0)
						ipath_remove_len = pNewFolder->m_iEntityOffset;

					bool isMarkers = false;


					char *final_name = path.Get();
					final_name += ipath_remove_len;
					if (*final_name == '\\')
						final_name++;

					std::string path_for_filename = final_name;
					std::string dir_name = final_name;
					replaceAll(dir_name, "\\", " - ");

					bool bDisplayText = true;

					cFolderItem::sFolderFiles * myfiles = NULL;

					myfiles = sorted_files[iLoop].second;
					if (myfiles)
					{

						if (myfiles->iFlags == 1)
							multi_selections_count++;

						bool bIsVisible = true;

						uniqueId = myfiles->uniqueId;
						ImGui::PushID(uniqueId); //Already unique: +preview_count);

						bool bLoadedInNewFormat = true;
						int iDefaultTexture = TOOL_ENTITY;
						int textureId = 0;

						{
							if (iDisplayLibraryType > 0)
							{
								if (iDisplayLibraryType == 2)
								{
									//PE: Need default image if we cant load the image.
									iDefaultTexture = ABOUT_TGC;
								}
								if (iDisplayLibraryType == 1)
								{
									iDefaultTexture = FILETYPE_MP3;
									//TODO: Support loading custom thumbs for music ?
									if (pestrcasestr(myfiles->m_sName.Get(), ".ogg"))
									{
										iDefaultTexture = FILETYPE_OGG;
									}
									else if (pestrcasestr(myfiles->m_sName.Get(), ".wav"))
									{
										iDefaultTexture = FILETYPE_WAV;
									}
								}
								if (iDisplayLibraryType == 3) //Video
								{
									//PE: Need default image before we generate a video preview.
									iDefaultTexture = FILETYPE_VIDEO;
								}
								if (iDisplayLibraryType == 4) //Script
								{
									//PE: Need default image before we generate a video preview.
									iDefaultTexture = FILETYPE_SCRIPT;
								}
								if (iDisplayLibraryType == 5) //Particles
								{
									//PE: Need default image before we generate a video preview.
									iDefaultTexture = FILETYPE_PARTICLE;

								}
							}
							//FPE Files.
							bool bCheckForNewPreviewImage = true;

							//Dont check any new ones before the first is done. just use default in this case.
							if (iDisplayLibraryType == 3 )
							{
								if (iVideoGenerateImageID > 0)
								{
									bCheckForNewPreviewImage = false;
									textureId = iDefaultTexture;
								}
							}

							if (iDisplayLibraryType == 5 && BackBufferParticlesMode)
							{
								bCheckForNewPreviewImage = false;
								textureId = iDefaultTexture;
								iWaitFramesBeforeProceed = 3;
							}
							
							//PE: Make sure to init all dlua descriptions.
							if (iDisplayLibraryType == 4 && myfiles->m_sDLuaDescription == "##na##")
							{
								//Load in DLUA description.
								std::string sScriptName = myfiles->m_sPath.Get();
								sScriptName = sScriptName + "\\" + myfiles->m_sName.Get();
								entityeleproftype dluaload;
								ParseLuaScript(&dluaload, (char *)sScriptName.c_str());
								myfiles->m_sDLuaDescription = "";
								if (dluaload.PropertiesVariableActive == 1)
								{
									//Build string
									char pCaptureAnyScriptDesc[8192 + 4096];
									strcpy(pCaptureAnyScriptDesc, "");
									for (int i = 0; i < dluaload.PropertiesVariable.iVariables; i++)
									{
										if (strlen(pCaptureAnyScriptDesc) < 8192)
										{
											strcat(pCaptureAnyScriptDesc, dluaload.PropertiesVariable.VariableSectionDescription[i].Get());
											if (dluaload.PropertiesVariable.Variable[i] && strlen(dluaload.PropertiesVariable.Variable[i]) > 0)
											{
												//Split into segments.
												strcat(pCaptureAnyScriptDesc, "[");
												strcat(pCaptureAnyScriptDesc, dluaload.PropertiesVariable.Variable[i]);
												strcat(pCaptureAnyScriptDesc, "]");
											}
											if (dluaload.PropertiesVariable.VariableSectionEndDescription[i].Len() > 0)
											{
												strcat(pCaptureAnyScriptDesc, dluaload.PropertiesVariable.VariableSectionEndDescription[i].Get());
											}
										}
									}
									myfiles->m_sDLuaDescription = pCaptureAnyScriptDesc;
								}
								else
								{
									if (dluaload.PropertiesVariable.VariableDescription.Len() > 0)
									{
										myfiles->m_sDLuaDescription = dluaload.PropertiesVariable.VariableDescription;
									}
								}
							}

							if (myfiles->iPreview <= 0 && bCheckForNewPreviewImage)
							{
								bool bDontTouchThisID = false;
								//Only Visible.
								int gcpy = ImGui::GetCursorPosY();
								if (!bReleaseIconsDynamic || ((bIsVisible || isMarkers) && (gcpy < iIconVisiblePosY && gcpy >= ImGui::GetScrollY() - media_icon_size || isMarkers)))
								{
									myfiles->last_used = (long)tCurrentTimeSec;
									if (!bImagesStillInImGuiQueue && !bLargePreview && max_load_persync-- > 0)
									{
										//Load preview.
										std::string sImgName = myfiles->m_sPath.Get();
										if (iDisplayLibraryType == 2)
										{
											//PE: Use image directly.
											sImgName = sImgName + "\\" + myfiles->m_sName.Get();
											//__debugbreak(); //Test file.
										}
										else if (iDisplayLibraryType > 0)
										{
											//Use .jpg as thumbs for other media. 512x288 format.
											sImgName = sImgName + "\\" + Left(myfiles->m_sName.Get(), Len(myfiles->m_sName.Get()) - 4);
											if (iDisplayLibraryType == 4) //Script
											{
												if (pref.current_style == 25 || pref.current_style == 3)
													sImgName += ".png";
												else
													sImgName += "2.png";
											}
											else
												sImgName += ".jpg";
										}
										else
										{
											sImgName = sImgName + "\\" + Left(myfiles->m_sName.Get(), Len(myfiles->m_sName.Get()) - 4);
											sImgName += ".bmp";
										}
										myfiles->iPreview = uniqueId;
										SetMipmapNum(1); //PE: mipmaps not needed.

										//PE: Check if we got a cached thumb in correct format.
										std::string sFpeName = path_for_filename.c_str();
										sFpeName = sFpeName + "\\" + myfiles->m_sName.Get();
										t.addentityfile_s = sFpeName.c_str();
										if (iDisplayLibraryType == 5)
										{
											//PE: Changed to support subfolders.
											sFpeName = "";
											sFpeName = "particlesbank\\";
											sFpeName = sFpeName + path_for_filename.c_str();
											sFpeName = sFpeName + "\\" + myfiles->m_sName.Get();
											replaceAll(sFpeName, "\\\\", "\\");
											t.addentityfile_s = sFpeName.c_str();
										}
										// perhaps make this a common define!
										CreateBackBufferCacheName(t.addentityfile_s.Get(), thumb_x, thumb_y);
										GG_SetWritablesToRoot(true);
										image_setlegacyimageloading(true);
										if (FileExist(BackBufferCacheName.Get()))
										{
											LoadImage((char *)BackBufferCacheName.Get(), myfiles->iPreview);
											if (!ImageExist(myfiles->iPreview))
											{
												LoadImage((char *)sImgName.c_str(), myfiles->iPreview);
											}
											else
											{
												//PE: Mark as ok in new format.
												myfiles->iBigPreview = iDefaultTexture;
											}
											GG_SetWritablesToRoot(false);
										}
										else 
										{
											GG_SetWritablesToRoot(false);
											//PE: Try to make a thumb here.
											if (iDisplayLibraryType == 3)
											{
												//Try custom image.
												LoadImage((char *)sImgName.c_str(), myfiles->iPreview);
												if (!ImageExist(myfiles->iPreview))
												{
													if (iVideoThumbID > 0 && AnimationExist(iVideoThumbID) && AnimationPlaying(iVideoThumbID))
													{
														//We cant do this if a video is already playing. perhaps stop in next run ?
														iStopVideoInNextFrame = iVideoThumbID;
														bDontTouchThisID = true;
													}
													else
													{
														//Only if we are not already generating a preview of video.

														std::string sVideoName = myfiles->m_sPath.Get();
														sVideoName = sVideoName + "\\" + myfiles->m_sName.Get();
														if (iVideoGetFirstFrame == 0)
														{
															if (iVideoPreviewThumbID > 0) 
															{
																if (AnimationExist(iVideoPreviewThumbID)) 
																{
																	if (AnimationPlaying(iVideoPreviewThumbID))
																		StopAnimation(iVideoPreviewThumbID);
																	DeleteAnimation(iVideoPreviewThumbID);
																	iVideoPreviewThumbID = 0;
																}
															}

															iVideoPreviewThumbID = 0;

															for (int itl = 1; itl <= 32; itl++)
															{
																if (AnimationExist(itl) == 0) { iVideoPreviewThumbID = itl; break; }
															}
															if (LoadAnimation((char *)sVideoName.c_str(), iVideoPreviewThumbID, g.videoprecacheframes, 0, 1) == false)
															{
																//Failed to load mark as ok using default thumb.
																myfiles->iPreview = iDefaultTexture;
																myfiles->iBigPreview = iDefaultTexture;
																textureId = iDefaultTexture;
																iVideoPreviewThumbID = -999;
															}

															if (iVideoPreviewThumbID > 0) {
																PlaceAnimation(iVideoPreviewThumbID, -1, -1, -1, -1);
																SetRenderAnimToImage(iVideoPreviewThumbID, true);
																//Try to get first frame.
																StopAnimation(iVideoPreviewThumbID);
																PlayAnimation(iVideoPreviewThumbID);
																void SetVideoPosition(float seconds);
																SetVideoPosition(1.0f);
																PlayAnimation(iVideoPreviewThumbID);
																SetRenderAnimToImage(iVideoPreviewThumbID, true);

																UpdateAllAnimation();
																Sleep(50); //Sleep so we get a video texture in the next call.
																UpdateAllAnimation();
																SetVideoVolume(0.1);
																iVideoGetFirstFrame = 8;
																iVideoGenerateImageID = myfiles->iPreview;
																pVideoGeneratingFile = myfiles;
																sVideoSaveName = BackBufferCacheName;
															}

														}
													}
												}
											}
											else if (iDisplayLibraryType == 5)
											{
												//Create particle thumb here.
												BackBufferIsGroup = false;
												BackBufferEntityID = 0;
												BackBufferObjectID = 0;
												BackBufferImageID = myfiles->iPreview;
												BackBufferSizeX = 512;
												BackBufferSizeY = 288;
												BackBufferCamLeft = 0.0f;
												bRotateBackBuffer = false;
												bBackBufferAnimated = false;
												bLoopBackBuffer = false;
												BackBufferSaveCacheName = ""; //No saving for now
												//Dont fit snapshot to rubber band.
												fLastRubberBandX1 = fLastRubberBandX2 = fLastRubberBandY1 = fLastRubberBandY2 = 0.0f;
												bSnapShotModeUseCamera = false;
												BackBufferParticlesMode = true;
												bDontTouchThisID = true;
												CreateBackdropObject(false, cstr("texturebank\\backdrops\\Black backdrop.dds"), t.addentityfile_s); //PE: We need a extra frame, as we set the material dirty in this call.

												BackBufferZoom = 800.0f;
												BackBufferCamUp = 400.0f;

												//Test
												float centerx = -1000, centery = 39000, centerz = -1000;

												std::string sParticleName = myfiles->m_sPath.Get();
												char cTmp[MAX_PATH];
												cstr savename = myfiles->m_sPath + "\\" + myfiles->m_sName;
												strcpy(cTmp, savename.Get());
												char *find = (char *) pestrcasestr(cTmp, "\\particlesbank");
												if (find)
												{
													strcpy(cTmp, find + 1);
												}
												else
												{
													strcpy(cTmp, "particlesbank\\");
													strcat(cTmp, myfiles->m_sName.Get());
												}
												CreateBackBufferCacheName(cTmp, thumb_x, thumb_y);
												BackBufferSaveCacheName = BackBufferCacheName;

												sParticleName = sParticleName + "\\" + Left(myfiles->m_sName.Get(), Len(myfiles->m_sName.Get()) - 4);

												if (BackBufferParticleEmitter != -1)
												{
													gpup_deleteEffect(BackBufferParticleEmitter);
													BackBufferParticleEmitter = -1;
												}

												if (BackBufferParticleEmitter == -1)
												{
													BackBufferParticleEmitter = gpup_loadEffect(sParticleName.c_str(), 0, 0, 0, 1.0);
													gpup_setGlobalScale(BackBufferParticleEmitter, 100.0f);
													gpup_emitterActive(BackBufferParticleEmitter, 0);
												}
												float fLive = 10.0f;
												if (BackBufferParticleEmitter != -1)
												{
													gpup_setGlobalPosition(BackBufferParticleEmitter, centerx, centery - 30.0f, centerz);
													gpup_resetLocalPosition(BackBufferParticleEmitter);
													gpup_setGlobalScale(BackBufferParticleEmitter, 100.0f);
													gpup_emitterActive(BackBufferParticleEmitter, 1);
													gpup_setEffectAnimationSpeed(BackBufferParticleEmitter, 1.0f);
													gpup_setEffectOpacity(BackBufferParticleEmitter, 1.0f);

													fLive = gpup_getEffectLifespan(BackBufferParticleEmitter);
												}
												//We need to delay the thumb as we cant fast forward the particles.
												if (fLive < 100.0f)
													iBackBufferParticlesTrigger = 25;
												else if (fLive < 200.0f)
													iBackBufferParticlesTrigger = 110;
												else if (fLive < 300.0f)
													iBackBufferParticlesTrigger = 150;
												else
													iBackBufferParticlesTrigger = 160; //Max

												iWaitFramesBeforeProceed = 3;
											}
											else
											{
												bLoadedInNewFormat = false;
												if (FileExist((char*)sImgName.c_str()) == 1)
												{
													LoadImage((char*)sImgName.c_str(), myfiles->iPreview);
												}
												else
												{
													// when the preview image file does not exist, use default
													if (iDisplayLibraryType == 0)
													{
														// buit only for object previews, leave other media types to their default icons (i.e WAV/OFF/MP3)
														LoadImage("texturebank\\backdrops\\Black backdrop.dds", myfiles->iPreview);
													}
												}
											}
										}
										image_setlegacyimageloading(false);
										SetMipmapNum(-1);

										//##na##
										if (iDisplayLibraryType == 4)
										{
											//Load in DLUA description.
											std::string sScriptName = myfiles->m_sPath.Get();
											sScriptName = sScriptName + "\\" + myfiles->m_sName.Get();
											entityeleproftype dluaload;
											ParseLuaScript(&dluaload, (char *) sScriptName.c_str());
											myfiles->m_sDLuaDescription = "";
											if (dluaload.PropertiesVariableActive == 1)
											{
												//Build string
												char pCaptureAnyScriptDesc[8192+4096];
												strcpy(pCaptureAnyScriptDesc, "");
												for (int i = 0; i < dluaload.PropertiesVariable.iVariables; i++)
												{
													if (strlen(pCaptureAnyScriptDesc) < 8192)
													{
														strcat(pCaptureAnyScriptDesc, dluaload.PropertiesVariable.VariableSectionDescription[i].Get());
														if (dluaload.PropertiesVariable.Variable[i] && strlen(dluaload.PropertiesVariable.Variable[i]) > 0)
														{
															//Split into segments.
															strcat(pCaptureAnyScriptDesc, "[");
															strcat(pCaptureAnyScriptDesc, dluaload.PropertiesVariable.Variable[i]);
															strcat(pCaptureAnyScriptDesc, "]");
														}
														if (dluaload.PropertiesVariable.VariableSectionEndDescription[i].Len() > 0)
														{
															strcat(pCaptureAnyScriptDesc, dluaload.PropertiesVariable.VariableSectionEndDescription[i].Get());
														}
													}
												}
												myfiles->m_sDLuaDescription = pCaptureAnyScriptDesc;
											}
											else
											{
												if (dluaload.PropertiesVariable.VariableDescription.Len() > 0)
												{
													myfiles->m_sDLuaDescription = dluaload.PropertiesVariable.VariableDescription;
												}
											}
										}
										if (!GetImageExistEx(myfiles->iPreview))
										{
											if (!bDontTouchThisID)
											{
												myfiles->iPreview = iDefaultTexture;
												textureId = iDefaultTexture;
											}
											else
											{
												textureId = iDefaultTexture;
											}
										}
										else 
										{
											loaded_images++;
											textureId = myfiles->iPreview;
										}
									}
									else
									{
										textureId = iDefaultTexture;
									}
								}
								else
								{
									textureId = iDefaultTexture;
								}
							}
							else
							{
								//PE: Only delete in first run. so we dont delete a image that has already been sent to rendering.
								if (bReleaseIconsDynamic && myfiles->iPreview > 0 ) 
								{
									//Only NOT Visible with a preview image..
									int gcpy = ImGui::GetCursorPosY();
									if (!isMarkers && (!(gcpy < iIconVisiblePosY && gcpy >= ImGui::GetScrollY() - media_icon_size) || !bIsVisible)) 
									{
										if ((long)tCurrentTimeSec - myfiles->last_used > 20) 
										{
											//Delete Image not visible for 20 sec.
											if (GetImageExistEx(myfiles->iPreview) && myfiles->iPreview >= 4000 && myfiles->iPreview < UIV3IMAGES) 
											{ 
												//PE: Need to protect system images after tool img range has changed. (myfiles->iPreview can be a system icon)
												image_setlegacyimageloading(true);
												DeleteImage(myfiles->iPreview);
												image_setlegacyimageloading(false);
												myfiles->iPreview = 0;
												loaded_images--;
											}
											textureId = iDefaultTexture;
										}
										else
											textureId = myfiles->iPreview;
									}
									else 
									{
										//Still visible update time.
										if (bIsVisible || isMarkers) myfiles->last_used = (long)tCurrentTimeSec;
										textureId = myfiles->iPreview;
									}
								}
								else
								{
									textureId = myfiles->iPreview;
								}
							}
						}
						if (textureId == 0)
						{
							textureId = iDefaultTexture;
						}

						if (iDisplayLibraryType == 4 && bDLUAOnly)
						{
							//PE: We now preload all dlua descriptions, so just process everything.
							if(1)
							{
								if (myfiles->m_sDLuaDescription.Len() <= 0)
								{
									bIsVisible = false;
								}
								else
								{
									if( pestrcasestr(myfiles->m_sDLuaDescription.Get(),"do not assign to an entity") )
										bIsVisible = false;
									else if (pestrcasestr(myfiles->m_sDLuaDescription.Get(), "do not use this script"))
										bIsVisible = false;

								}
							}
						}

						//Is object visible
						// or bCheckGotoPreview
						if (bIsVisible || bCheckGotoPreview)
						{

							if (!firstvisiblefile)
							{
								if (!scrolltofile)
								{
									int gcpy = ImGui::GetCursorPosY();
									float fSPos = ImGui::GetScrollY() - (media_icon_size);
									if (fSPos < 0.0f) fSPos = 0.0f;
									if (gcpy >= fSPos)
									{
										firstvisiblefile = myfiles;
									}
								}
							}
							if (myfiles->iPreview > 0 && !GetImageExistEx(myfiles->iPreview))
							{
								myfiles->iPreview = 0;
								textureId = TOOL_ENTITY;
							}

							if (!isMarkers && i == 0)
							{
								LPSTR pFinalHeaderTitle = (LPSTR)dir_name.c_str();
								if (stricmp(pFinalHeaderTitle, "user") == NULL) pFinalHeaderTitle = "Custom Assets";
								if (stricmp(pFinalHeaderTitle, "user - charactercreatorplus") == NULL) pFinalHeaderTitle = "Custom Characters";
								if (stricmp(pFinalHeaderTitle, "user - ebestructures") == NULL) pFinalHeaderTitle = "Custom Structures";

								if (pestrcasestr(dir_name.c_str(), "_markers"))
									pFinalHeaderTitle = "Game Elements";

								strcpy(cHeader, pFinalHeaderTitle);
							}

							if (pref.iSetColumnsEntityLib <= 4)
								ImGui::SetWindowFontScale(1.0);
							else
								ImGui::SetWindowFontScale(SMALLFONTSIZE);

							float fFramePadding = (iColumnsWidth - media_icon_size)*0.5;
							float fCenterX = iColumnsWidth * 0.5;

							ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(fFramePadding, 2.0f));

							if (iDisplayLibraryType == 0) //Only FPE for now.
							{
								if (iDisplayLibrarySubType == 0)
								{
									if (!io.KeyShift)
									{
										if (io.KeyCtrl) // CTRL+A
										{
											if (ImGui::IsKeyReleased(65)) //A
											{
												bAddAllVisible = true;
												bAddAllVisibleTriggerMessage = true;
											}
										}
									}
									if (bAddAllVisible)
									{
										//PE: Include All.
										if (bIsVisible)
										{
											extern bool g_bFreeTrialVersion;
											if (g_bFreeTrialVersion == false || (g_bFreeTrialVersion == true && myfiles->bAvailableInFreeTrial == true))
												myfiles->iFlags = 1;
										}
									}
								}
							}
							ImRect selection_bb;
							bool bVisibleOnScreen = false;
							//sMakeDefaultSelecting
							if (sMakeDefaultSelecting != "")
							{
								std::string sMediaName = "";
								if (iDisplayLibraryType == 1) sMediaName = "audiobank\\";
								if (iDisplayLibraryType == 2) sMediaName = "imagebank\\";
								if (iDisplayLibraryType == 3) sMediaName = "videobank\\";
								if (iDisplayLibraryType == 5) sMediaName = "particlesbank\\";
								if (iDisplayLibraryType == 0 && iDisplayLibrarySubType == 1) sMediaName = "charactercreatorplus\\animations\\";
								sMediaName = sMediaName + path_for_filename.c_str();
								if (path_for_filename.length() == 0)
									sMediaName = sMediaName + myfiles->m_sName.Get();
								else
									sMediaName = sMediaName + "\\" + myfiles->m_sName.Get();
								if (stricmp(sMakeDefaultSelecting.Get(),sMediaName.c_str()) == 0)
								{
									selectedmediafile = myfiles;
									scrolltofile = myfiles;
									bScrollInNextFrame = true;
									sMakeDefaultSelecting = "";
								}
							}
							if (myfiles->iFlags == 1 || selectedmediafile == myfiles )
							{
								int gcpy = ImGui::GetCursorPosY();
								if (bIsVisible && gcpy < (iIconVisiblePosY ) && gcpy >= (ImGui::GetScrollY() - (media_icon_size*0.5)))
								{
									ImVec2 padding = { 0.0, 0.0 };
									ImGuiWindow* window = ImGui::GetCurrentWindow();
									float cw = ImGui::GetContentRegionAvailWidth();
									if (iDisplayLibraryType == 0)
									{
										selection_bb.Min = (window->DC.CursorPos - padding) + ImVec2(fFramePadding, 2.0f);
										if (pref.iSetColumnsEntityLib == 1)
											selection_bb.Max = (window->DC.CursorPos + padding) + ImVec2(cw - 1.0f, media_icon_size_y + 2.0f);
										else
											selection_bb.Max = (window->DC.CursorPos + padding) + ImVec2(cw + 6.0f, media_icon_size_y + 2.0f);
										if(bDisplayText)
											selection_bb.Max.y += ImGui::GetFontSize() + 8.0f;
									}
									else
									{
										selection_bb.Min = (window->DC.CursorPos - padding) + ImVec2(fFramePadding, 2.0f);
										if (pref.iSetColumnsEntityLib == 1)
											selection_bb.Max = (window->DC.CursorPos + padding) + ImVec2(cw - 1.0f, media_icon_size_y + 2.0f);
										else
											selection_bb.Max = (window->DC.CursorPos + padding) + ImVec2(cw + 6.0f, media_icon_size_y + 2.0f);
									}
									bAnySelectedItemsAvailable = true;
									bVisibleOnScreen = true;
								}
							}

							std::string sFinal = myfiles->m_sNameFinal.Get(); //PE: For speed.

							CheckTutorialAction(sFinal.c_str(), 13.0f); //Tutorial: check if we are waiting for this action

							//PE: Support Shift for selecting many items.
							if (iDisplayLibraryType == 0) //Currently only for fpe.
							{
								if (!io.KeyShift)
								{
									//firstShiftFile = NULL;
									lastShiftFile = NULL;
								}

								if (firstShiftFile && lastShiftFile) 
								{
									if (myfiles == firstShiftFile) bFirstShiftHasBeenSeen = true;
									if (!bFirstShiftHasBeenSeen && myfiles == lastShiftFile)
									{
										//Swap around, last is first.
										cFolderItem::sFolderFiles * tmpShiftFile = lastShiftFile;
										lastShiftFile = firstShiftFile;
										firstShiftFile = tmpShiftFile;
										bFirstShiftHasBeenSeen = true;
									}

									extern bool g_bFreeTrialVersion;
									static bool bStartShiftActive = false;

									myfiles->iFlags = 0;
									if (myfiles == firstShiftFile) 
									{
										bStartShiftActive = true;
										if (g_bFreeTrialVersion == false || (g_bFreeTrialVersion == true && myfiles->bAvailableInFreeTrial == true))
											myfiles->iFlags = 1;
									}
									if (myfiles == lastShiftFile) 
									{
										bStartShiftActive = false;
										if (g_bFreeTrialVersion == false || (g_bFreeTrialVersion == true && myfiles->bAvailableInFreeTrial == true))
											myfiles->iFlags = 1;
									}
									if (bStartShiftActive)
									{
										if (g_bFreeTrialVersion == false || (g_bFreeTrialVersion == true && myfiles->bAvailableInFreeTrial == true))
											myfiles->iFlags = 1;
									}
								}
							}

							bool bBlockBackBufferUpdating = false;
							if (bLargePreview || bImagesStillInImGuiQueue) bBlockBackBufferUpdating = true;
							if (iDisplayLibraryType != 0) bBlockBackBufferUpdating = true;

							ImGui::SetItemAllowOverlap();
							static void* iDelayedClickFile = NULL;
							static int iDelayedImgClick = 0;
							if (iDelayedClickFile == myfiles && iDelayedImgClick > 0) iDelayedImgClick--;

							ImVec2 imgsize = ImVec2(media_icon_size, media_icon_size_y);
							ImVec2 DirectPosition = ImGui::GetCursorPos();

							if (iDisplayLibraryType == 2)
							{
								float imgw = ImageWidth(textureId);
								float imgh = ImageHeight(textureId);
								if (imgh > imgw)
								{
									float fRatio = media_icon_size_y / imgh;
									imgsize = ImVec2(imgw*fRatio, imgh*fRatio);
									if (imgsize.x > media_icon_size)
									{
										//Reverse
										float fRatio = media_icon_size / imgw;
										imgsize = ImVec2(imgw*fRatio, imgh*fRatio);
									}
								} 
								else
								{
									float fRatio = media_icon_size / imgw;
									imgsize = ImVec2(imgw*fRatio, imgh*fRatio);
									if (imgsize.y > media_icon_size_y)
									{
										//Reverse.
										float fRatio = media_icon_size_y / imgh;
										imgsize = ImVec2(imgw*fRatio, imgh*fRatio);
									}
								}
								//Clip.
								if (imgsize.y > media_icon_size_y)
									imgsize.y = media_icon_size_y;
								if (imgsize.x > media_icon_size)
									imgsize.x = media_icon_size;
								//Make room for background borders.
								imgsize.x -= 8.0f;
								imgsize.y -= 8.0f;
								//Center and use direct position.
								float fOffsetX = (media_icon_size*0.5) - (imgsize.x*0.5);
								float fOffsetY = (media_icon_size_y*0.5) - (imgsize.y*0.5);
								fOffsetX -= 2.5f; //Fit borders.
								fOffsetY -= 2.0f; //Fit borders.
								//Add filler.
								ImGuiWindow* window = ImGui::GetCurrentWindow();
								//AddRectFilled faster ?
								ImVec2 vDrawPos = { ImGui::GetCursorScreenPos().x ,ImGui::GetCursorScreenPos().y };
								window->DrawList->AddRectFilled(vDrawPos, vDrawPos + ImVec2(media_icon_size, media_icon_size_y), ImGui::GetColorU32(ImVec4(0, 0, 0, 0.3)), 6.0f, 15);

								ImGui::SetCursorPos(DirectPosition + ImVec2(fOffsetX, fOffsetY));
							}
							
							bool bOverLayVideo = false;
							ImVec2 vVideoPos = ImGui::GetCursorPos();
							if( iDisplayLibraryType == 3 && playingiles == myfiles && iVideoThumbID > 0)
							{
								//Display video thumb.
								bOverLayVideo = true;
							}

							total_files_displayed_in_library++;
							if ((iDelayedImgClick == 1 && iDelayedClickFile == myfiles) || ImGui::ImgBtn(textureId, imgsize, drawCol_black, ImVec4(1, 1, 1, 1), ImVec4(1, 1, 1, 1), drawCol_Down, -1, 0, 0, 0, false))
							{
								iDelayedClickFile = myfiles;

								if (iDelayedImgClick == 1)
								{
									iDelayedImgClick = 0;

									if (iDisplayLibraryType == 1 || iDisplayLibraryType == 2 || iDisplayLibraryType == 3 || iDisplayLibraryType == 4 || iDisplayLibraryType == 5) //Music. , reuse selectedmediafile for images
									{
										selectedmediafile = myfiles;
									}
									else
									{
										bool bAllowSelection = true;
										extern bool g_bFreeTrialVersion;
										if (g_bFreeTrialVersion == true)
										{
											if (myfiles->bAvailableInFreeTrial == false)
											{
												bAllowSelection = false;
											}
										}
										if(bAllowSelection==true)
										{
											if (bTutorialCheckAction) TutorialNextAction(); //Clicked get next tutorial action.

											if (iDisplayLibrarySubType == 1)
											{
												// Animation Library selection simply returns choice as file path
												selectedmediafile = myfiles;
											}
											else
											{
												//If ctrl , just mark them.
												if (io.KeyShift)
												{
													if (firstShiftFile)
													{
														lastShiftFile = myfiles;
													}
													else
													{
														firstShiftFile = myfiles;
														multi_selections = true;
														if (g_bFreeTrialVersion == false || (g_bFreeTrialVersion == true && myfiles->bAvailableInFreeTrial == true))
															myfiles->iFlags = 1;
													}
												}
												else if (io.KeyCtrl)
												{
													//Mark object.
													multi_selections = true;
													if (myfiles->iFlags == 0)
													{
														if (g_bFreeTrialVersion == false || (g_bFreeTrialVersion == true && myfiles->bAvailableInFreeTrial == true))
															myfiles->iFlags = 1;
														firstShiftFile = myfiles;
													}
													else
														myfiles->iFlags = 0;
												}
												else
												{
													if (bWaypointDrawmode || bWaypoint_Window) { bWaypointDrawmode = false; bWaypoint_Window = false; }
													if (bImporter_Window) { importer_quit(); bImporter_Window = false; }
													if (g_bCharacterCreatorPlusActivated) g_bCharacterCreatorPlusActivated = false;

													//Make sure we are in entity mode.
													bForceKey = true;
													csForceKey = "o";

													bBlockBackBufferUpdating = true;
													DeleteWaypointsAddedToCurrentCursor();
													CloseDownEditorProperties();
													FreeTempImageList(); //PE: Make sure we free all not used textures before adding new objects.
													iLastEntityOnCursor = 0;

													std::string sFpeName = path_for_filename.c_str();
													sFpeName = sFpeName + "\\" + myfiles->m_sName.Get();
													t.addentityfile_s = sFpeName.c_str();
													if (t.addentityfile_s != "")
													{
														// Special group detection system
														entity_adduniqueentity(false);
														t.tasset = t.entid;
														if (t.talreadyloaded == 0)
														{
															editor_filllibrary();
														}
													}

													iExtractMode = 0; //PE: Always start in find floor mode.

													t.inputsys.constructselection = t.tasset;

													t.gridentity = t.entid;
													t.inputsys.constructselection = t.entid;
													t.inputsys.domodeentity = 1;
													t.grideditselect = 5;

													//Make sure we use a fresh t.grideleprof
													entity_fillgrideleproffromprofile();

													editor_refresheditmarkers();
													//PE: Close window for now.
													bCheckForClosing = true;
													bImGuiRenderTargetFocus = true; //PE: needed for window overlap check.
													bDraggingActive = false;

													//PE: we loose status somewhere, so force it off after adding a entity to map.
													extern bool bCubesVisible;
													if (bCubesVisible == false) bCubesVisible = true; //Force.
												}
											}
										}
									}
								}
								else
								{
									iDelayedImgClick = 14;
								}
							}
							bool bThumbHovered = ImGui::IsItemHovered();
							
							// if free trial and object not on list
							extern bool g_bFreeTrialVersion;
							if (g_bFreeTrialVersion == true)
							{
								if (myfiles->bAvailableInFreeTrial == false)
								{
									// place grey layer on thumb to show object not available
									ImVec2 vOldPos = ImGui::GetCursorPos();
									ImGui::SetCursorPos(DirectPosition);
									ImGui::SetItemAllowOverlap();
									ImGui::ImgBtn(FREETRIAL_NOTAVAILABLE, imgsize, ImColor(0, 0, 0, 0), ImColor(255, 255, 255, 180), ImColor(255, 255, 255, 180), ImColor(255, 255, 255, 180), 0, 0, 0, 0, false, false, false, false, true, bBoostIconColors);
									ImGui::SetCursorPos(vOldPos);
								}
							}

							if (iDisplayLibraryType == 2)
							{
								//PE: Add padding ?
								float paddingy = 10.0f;
								ImGui::SetCursorPos(DirectPosition + ImVec2(0, media_icon_size_y + paddingy));
							}

							if (bWaitOnMouseRelease)
							{
								if (!ImGui::IsMouseDown(0))
									bWaitOnMouseRelease = false;
							}

							if (iDisplayLibraryType > 0 || (iDisplayLibraryType==0 && iDisplayLibrarySubType == 1))
							{
								// Drag drop from other media types - no
							}
							else
							{
								bool bAllowSelection = true;
								extern bool g_bFreeTrialVersion;
								if (g_bFreeTrialVersion == true)
								{
									if (myfiles->bAvailableInFreeTrial == false)
									{
										bAllowSelection = false;
									}
								}
								if (bAllowSelection == true)
								{
									if (!bImagesStillInImGuiQueue && !bWaitOnMouseRelease && !bLargePreview && t.gridentity == 0 && t.gridentityobj == 0 && ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
									{
										CloseDownEditorProperties();
										t.inputsys.constructselection = 0;

										myfiles->m_dropptr = myfiles;

										std::string sFpeName = path_for_filename.c_str();
										sFpeName = sFpeName + "\\" + myfiles->m_sName.Get();

										myfiles->m_sFolder = sFpeName.c_str();

										//PE: Special add all selected. for drag drop.
										myfiles->iAnimationFrom = 0;
										if (selected_library_fpe.size() > 0)
										{
											myfiles->iAnimationFrom = 200000;
										}

										ImGui::SetDragDropPayload("DND_MODEL_DROP_TARGET", myfiles, sizeof(void *));
										ImGui::ImgBtn(textureId, ImVec2(media_icon_size, media_icon_size_y), drawCol_black, drawCol_normal, drawCol_hover, drawCol_Down, 0, 0, 0, 0, false);
										ImGui::SetCursorPos(oldCursor);
										pDragDropFile = myfiles;
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
							}
							ImGui::PopID();
							ImGui::PushID(uniqueId + 20000);

							//---
							if (iDisplayLibraryType == 0 && iDisplayLibrarySubType == 1)
							{
								if (bThumbHovered && ImGui::IsMouseClicked(0))
								{
									std::string sMediaName = "charactercreatorplus\\animations\\";
									sMediaName = sMediaName + path_for_filename.c_str();
									if (path_for_filename.length() == 0)
										sMediaName = sMediaName + myfiles->m_sName.Get();
									else
										sMediaName = sMediaName + "\\" + myfiles->m_sName.Get();
									sSelectedLibrarySting = sMediaName.c_str();
									iSelectedLibraryStingReturnID = iLibraryStingReturnToID;
									bBlockBackBufferUpdating = true;
									bCheckForClosing = true;
									bImGuiRenderTargetFocus = true;
								}
							}

							//---
							if (iDisplayLibraryType > 0)
							{
								if (bThumbHovered && ImGui::IsMouseDoubleClicked(0))
								{
									//Sent selection to imgui ID that have last requested a media file.
									selectedmediafile = NULL;
									playingiles = NULL;
									if(iDisplayLibraryType == 3) iStopVideoInNextFrame = iVideoThumbID;
									//Stop anything playing.
									if (SoundExist(g.temppreviewsoundoffset) == 1)
									{
										StopSound(g.temppreviewsoundoffset);
										DeleteSound(g.temppreviewsoundoffset);
									}

									std::string sMediaName = "";
									//if (strnicmp(path_for_filename.c_str(), "projectbank", 11) != NULL)
									//{
										if (iDisplayLibraryType == 1) sMediaName = "audiobank\\";
										if (iDisplayLibraryType == 2) sMediaName = "imagebank\\";
										if (iDisplayLibraryType == 3) sMediaName = "videobank\\";
										if (iDisplayLibraryType == 5) sMediaName = "particlesbank\\";
										if (iDisplayLibraryType == 0 && iDisplayLibrarySubType == 1) sMediaName = "charactercreatorplus\\animations\\";
									//}
									sMediaName = sMediaName + path_for_filename.c_str();
									if(path_for_filename.length() == 0)
										sMediaName = sMediaName + myfiles->m_sName.Get();
									else
										sMediaName = sMediaName + "\\" + myfiles->m_sName.Get();
									if (sMediaName != "")
									{
										sSelectedLibrarySting = sMediaName.c_str();
										iSelectedLibraryStingReturnID = iLibraryStingReturnToID;
									}
									bBlockBackBufferUpdating = true;
									bCheckForClosing = true;
									bImGuiRenderTargetFocus = true; //PE: needed for window overlap check.
								}

								if (bOverLayVideo && iStopVideoInNextFrame == 0)
								{
									ImVec2 vOldPos = ImGui::GetCursorPos();
									ImGui::SetCursorPos(vVideoPos);

									if (AnimationExist(iVideoThumbID) && AnimationPlaying(iVideoThumbID))
									{
										//imgsize
										ID3D11ShaderResourceView* lpVideoTexture = GetAnimPointerView(iVideoThumbID);
										float fVideoW = GetAnimWidth(iVideoThumbID);
										float fVideoH = GetAnimHeight(iVideoThumbID);
										if (lpVideoTexture)
										{
											ImGuiWindow* window = ImGui::GetCurrentWindow();
											ImRect image_bb(window->DC.CursorPos + ImVec2(3, 2), window->DC.CursorPos + ImVec2(3, 2) + imgsize);
											float animU = GetAnimU(iVideoThumbID);
											float animV = GetAnimV(iVideoThumbID);
											ImVec2 uv0 = ImVec2(0, 0);
											ImVec2 uv1 = ImVec2(animU, animV);
											window->DrawList->AddImage((ImTextureID)lpVideoTexture, image_bb.Min, image_bb.Max, uv0, uv1, ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 1.0f)));
										}
									}
									ImGui::SetCursorPos(vOldPos);
								}

								//Not while generating thumbs.
								if (iDisplayLibraryType == 3 && iVideoGenerateImageID == 0 )
								{
									//Play Video.
									int iImageSize = 24;
									ImVec2 opos = ImGui::GetCursorPos();
									ImGui::SetCursorPos(ImVec2((opos.x + media_icon_size) - iImageSize - 2.0f, opos.y - 14.0f - iImageSize));
									ImGui::SetItemAllowOverlap();

									if (playingiles == myfiles)
									{
										if (iVideoThumbID > 0 && AnimationExist(iVideoThumbID) && AnimationPlaying(iVideoThumbID))
										{
											float progress = GetAnimPercentDone(iVideoThumbID) / 100.0f;
											if (progress == 0.0f)
											{
												iStopVideoInNextFrame = iVideoThumbID;
												playingiles = NULL;
											}
										}

										if (ImGui::ImgBtn(MEDIA_PAUSE, ImVec2(iImageSize, iImageSize), ImColor(0, 0, 0, 0), ImColor(255, 255, 255, 220), ImColor(255, 255, 255, 220), ImColor(255, 255, 255, 220), 0, 0, 0, 0, false, false, false, false, true, bBoostIconColors))
										{
											selectedmediafile = myfiles;
											playingiles = NULL;
											//PLAY music file.
											iStopVideoInNextFrame = iVideoThumbID;
										}
										if (!bThumbHovered)
											bThumbHovered = ImGui::IsItemHovered();
										if (ImGui::IsItemHovered()) ImGui::SetTooltip("Stop Playing Video");

										if (iVideoThumbID > 0 && AnimationExist(iVideoThumbID) && AnimationPlaying(iVideoThumbID))
										{
											float progress = GetAnimPercentDone(iVideoThumbID) / 100.0f;
											if (progress < 0.0f) progress = 0.0f;
											if (progress > 1.0f) progress = 1.0f;
											ImVec2 opos = ImGui::GetCursorPos();
											ImVec4 backImGuiCol_PlotHistogram = ImGui::GetStyle().Colors[ImGuiCol_PlotHistogram];
											ImVec4 backImGuiCol_Border = ImGui::GetStyle().Colors[ImGuiCol_Border];
											ImVec4 backImGuiCol_BorderShadow = ImGui::GetStyle().Colors[ImGuiCol_BorderShadow];
											ImVec4 backImGuiCol_FrameBg = ImGui::GetStyle().Colors[ImGuiCol_FrameBg];
											//ImVec4 newBlue = ImVec4(56.0 / 255.0, 110.0 / 255.0, 145.0 / 255.0, 1);
											ImVec4 newBlue = ImVec4(0.8f, 0.8f, 0.8f, 0.6f);
											ImGui::GetStyle().Colors[ImGuiCol_PlotHistogram] = newBlue;
											ImGui::GetStyle().Colors[ImGuiCol_Border] = ImVec4(0, 0, 0, 0);
											ImGui::GetStyle().Colors[ImGuiCol_BorderShadow] = ImVec4(0, 0, 0, 0);
											ImGui::GetStyle().Colors[ImGuiCol_FrameBg] = ImVec4(0, 0, 0, 0);
											float padding = 30.0f;
											ImGui::SetCursorPos(ImVec2(opos.x + 4.0f + padding, opos.y - 16.0f - 10.0f));
											ImGui::ProgressBar(progress, ImVec2(media_icon_size - padding - padding, 16.0f), " ");
											ImGui::SetCursorPos(opos);
											ImGui::GetStyle().Colors[ImGuiCol_PlotHistogram] = backImGuiCol_PlotHistogram;
											ImGui::GetStyle().Colors[ImGuiCol_Border] = backImGuiCol_Border;
											ImGui::GetStyle().Colors[ImGuiCol_BorderShadow] = backImGuiCol_BorderShadow;
											ImGui::GetStyle().Colors[ImGuiCol_FrameBg] = backImGuiCol_FrameBg;
										}
									}
									else if (ImGui::ImgBtn(MEDIA_PLAY, ImVec2(iImageSize, iImageSize), ImColor(0, 0, 0, 0), ImColor(255, 255, 255, 220), ImColor(255, 255, 255, 220), ImColor(255, 255, 255, 220), 0, 0, 0, 0, false, false, false, false, true, bBoostIconColors))
									{
										//PLAY video file.
										selectedmediafile = myfiles;
										playingiles = NULL;

										if (iVideoThumbID > 0 && AnimationExist(iVideoThumbID))
										{
											iStopVideoInNextFrame = iVideoThumbID;
										}

										std::string sVideoName = myfiles->m_sPath.Get();
										sVideoName = sVideoName + "\\" + myfiles->m_sName.Get();

										sVideoLoadName = sVideoName.c_str();
										iStartVideoInNextFrame = 1;
										playingiles = myfiles;

									}
									ImGui::SetCursorPos(opos);
									if (!bThumbHovered)
										bThumbHovered = ImGui::IsItemHovered();
									if (ImGui::IsItemHovered()) ImGui::SetTooltip("Play Video");

								}

								if (iDisplayLibraryType == 1) //Music
								{
									//Play music.
									int iImageSize = 24;
									ImVec2 opos = ImGui::GetCursorPos();
									ImGui::SetCursorPos(ImVec2((opos.x + media_icon_size) - iImageSize - 2.0f, opos.y - 14.0f - iImageSize));
									ImGui::SetItemAllowOverlap();
									static int framesbeforestoppingsound = 0;
									if (playingiles == myfiles)
									{

										//isoundplaying
										float pos = GetSoundPosition(g.temppreviewsoundoffset);
										if (framesbeforestoppingsound > 0) framesbeforestoppingsound--;
										if (SoundExist(g.temppreviewsoundoffset) == 1 && SoundPlaying(g.temppreviewsoundoffset) == 0)
										{
											if (framesbeforestoppingsound == 0)
											{
												//Not playing.
												StopSound(g.temppreviewsoundoffset);
												playingiles = NULL;
											}
										}
										if (ImGui::ImgBtn(MEDIA_PAUSE, ImVec2(iImageSize, iImageSize), ImColor(0, 0, 0, 0), ImColor(255, 255, 255, 220), ImColor(255, 255, 255, 220), ImColor(255, 255, 255, 220), 0, 0, 0, 0, false, false, false, false, true, bBoostIconColors))
										{
											selectedmediafile = myfiles;
											playingiles = NULL;
											//PLAY music file.
											if (SoundExist(g.temppreviewsoundoffset) == 1)
											{
												StopSound(g.temppreviewsoundoffset);
												DeleteSound(g.temppreviewsoundoffset);
											}
										}
										if (!bThumbHovered)
											bThumbHovered = ImGui::IsItemHovered();
										if (ImGui::IsItemHovered()) ImGui::SetTooltip("Stop Playing Music");

										//PE: Draw progress bar end
										float progress = pos / 100.0f;
										if (progress < 0.0) progress = 0.0;
										if (progress > 1.0) progress = 1.0;
										ImVec2 opos = ImGui::GetCursorPos();
										ImVec4 backImGuiCol_PlotHistogram = ImGui::GetStyle().Colors[ImGuiCol_PlotHistogram];
										ImVec4 backImGuiCol_Border = ImGui::GetStyle().Colors[ImGuiCol_Border];
										ImVec4 backImGuiCol_BorderShadow = ImGui::GetStyle().Colors[ImGuiCol_BorderShadow];
										ImVec4 backImGuiCol_FrameBg = ImGui::GetStyle().Colors[ImGuiCol_FrameBg];

										//ImVec4 newBlue = ImVec4(56.0 / 255.0, 110.0 / 255.0, 145.0 / 255.0, 1);
										ImVec4 newBlue = ImVec4(0.8f, 0.8f, 0.8f, 0.6f );
										ImGui::GetStyle().Colors[ImGuiCol_PlotHistogram] = newBlue;
										ImGui::GetStyle().Colors[ImGuiCol_Border] = ImVec4(0, 0, 0, 0);
										ImGui::GetStyle().Colors[ImGuiCol_BorderShadow] = ImVec4(0, 0, 0, 0);
										ImGui::GetStyle().Colors[ImGuiCol_FrameBg] = ImVec4(0, 0, 0, 0);
										float padding = 30.0f;
										ImGui::SetCursorPos(ImVec2(opos.x + 4.0f + padding, opos.y - 16.0f - 10.0f));
										ImGui::ProgressBar(progress, ImVec2(media_icon_size - padding - padding, 16.0f), " ");
										ImGui::SetCursorPos(opos);

										ImGui::GetStyle().Colors[ImGuiCol_PlotHistogram] = backImGuiCol_PlotHistogram;
										ImGui::GetStyle().Colors[ImGuiCol_Border] = backImGuiCol_Border;
										ImGui::GetStyle().Colors[ImGuiCol_BorderShadow] = backImGuiCol_BorderShadow;
										ImGui::GetStyle().Colors[ImGuiCol_FrameBg] = backImGuiCol_FrameBg;

									}
									else if (ImGui::ImgBtn(MEDIA_PLAY, ImVec2(iImageSize, iImageSize), ImColor(0, 0, 0, 0), ImColor(255, 255, 255, 220), ImColor(255, 255, 255, 220), ImColor(255, 255, 255, 220), 0, 0, 0, 0, false, false, false, false, true, bBoostIconColors))
									{
										//PLAY music file.
										selectedmediafile = myfiles;
										playingiles = NULL;
										if (SoundExist(g.temppreviewsoundoffset) == 1)
										{
											StopSound(g.temppreviewsoundoffset);
											DeleteSound(g.temppreviewsoundoffset);
										}
										std::string sSoundName = "";
										//if (strnicmp(path_for_filename.c_str(), "projectbank", 11) != NULL) 
										sSoundName = "audiobank\\";
										sSoundName = sSoundName + path_for_filename.c_str();
										sSoundName = sSoundName + "\\" + myfiles->m_sName.Get();
										framesbeforestoppingsound = 5;
										LoadSound((char *) sSoundName.c_str(), g.temppreviewsoundoffset);
										if (SoundExist(g.temppreviewsoundoffset) == 1)
										{
											playingiles = myfiles;
											PlaySound(g.temppreviewsoundoffset);
											SetSoundVolume(g.temppreviewsoundoffset, 100.0);
										}
									}
									ImGui::SetCursorPos(opos);
									if (!bThumbHovered)
										bThumbHovered = ImGui::IsItemHovered();
									if (ImGui::IsItemHovered()) ImGui::SetTooltip("Play Music");
								}
							}
							else
							{
								bool bShowPreviewButton = true;
								if (myfiles->m_bIsGroupObject == true)
								{
									// do not show preview button for smart objects, no easy way to construct the preview - too many moving parts
									bShowPreviewButton = false;
								}
								if (bShowPreviewButton)
								{
									int iImageSize = 24;
									ImVec2 opos = ImGui::GetCursorPos();
									ImGui::SetCursorPos(ImVec2((opos.x + media_icon_size) - iImageSize - 2.0f, opos.y - 14.0f - iImageSize));
									ImGui::SetItemAllowOverlap();

									if (ImGui::ImgBtn(KEY_MAXIMIZE, ImVec2(iImageSize, iImageSize), ImColor(0, 0, 0, 0), ImColor(255, 255, 255, 180), ImColor(255, 255, 255, 180), ImColor(255, 255, 255, 180), 0, 0, 0, 0, false, false, false, false, true, bBoostIconColors))
									{
										pPreviewFile = myfiles;
										bLargePreview = true;
										bDoBackbufferUpdate = true;
									}
									ImGui::SetCursorPos(opos);
									if (!bThumbHovered)
										bThumbHovered = ImGui::IsItemHovered();
									if (ImGui::IsItemHovered()) ImGui::SetTooltip("Fullscreen Preview");

									if (bThumbHovered && ImGui::IsMouseDoubleClicked(0))
									{
										pPreviewFile = myfiles;
										bLargePreview = true;
										bDoBackbufferUpdate = true;
										iDelayedImgClick = 0; //Cancel last btn click.
									}
								}
							}
							//---

							// hovering over an object library item
							static void* additionalcheck = NULL;
							static cstr preload_fpe_file = "";
							static bool preload_fpe_file_started = false;
							if (iDisplayLibraryType > 0)
							{
								//Hover other media.
								static bool bTriggerTimer = false;
								if (iWaitFramesBeforeProceed == 0) //No hover while generating thumbs.
								{
									if (iDisplayLibraryType == 5 && bThumbHovered)
									{
										//Enable rotation
										iTooltipHoveredTimer = MAXTimer();
										if (!bTriggerTimer)
										{
											iTooltipTimer = iTooltipHoveredTimer;
											bTriggerTimer = true;

										}
										//Need a little timer so old images can reload before we start another.
										if (bTriggerTimer && iTooltipHoveredTimer - iTooltipTimer > 200)
										{
											//End any old.
											if (additionalcheck && additionalcheck != myfiles)
											{
												//Must reload old icon.
												cFolderItem::sFolderFiles * tmp = (cFolderItem::sFolderFiles *) additionalcheck;
												tmp->iPreview = 0;
												additionalcheck = 0;
												BackBufferSaveCacheName = "";
												BackBufferObjectID = 0;
												BackBufferImageID = 0;
												BackBufferZoom = 0.0f;
												BackBufferCamUp = 0.0f;
												bSnapShotModeUseCamera = false;
												BackBufferParticlesMode = false;
												bLoopBackBuffer = false;
											}
										}
										if (bTriggerTimer && iTooltipHoveredTimer - iTooltipTimer > 500)
										{
											if (additionalcheck != myfiles)
											{
												additionalcheck = myfiles;
												iTooltipTimer = iTooltipHoveredTimer;

												//Create particle thumb here.
												BackBufferIsGroup = false;
												BackBufferEntityID = 0;
												BackBufferObjectID = 0;
												BackBufferImageID = myfiles->iPreview;
												BackBufferSizeX = 512;
												BackBufferSizeY = 288;
												BackBufferCamLeft = 0.0f;
												bRotateBackBuffer = false;
												bBackBufferAnimated = false;
												BackBufferSaveCacheName = ""; //No saving
												//Dont fit snapshot to rubber band.
												fLastRubberBandX1 = fLastRubberBandX2 = fLastRubberBandY1 = fLastRubberBandY2 = 0.0f;
												CreateBackdropObject(false, cstr("texturebank\\backdrops\\Black backdrop.dds"), t.addentityfile_s); //PE: We need a extra frame, as we set the material dirty in this call.

												BackBufferZoom = 800.0f;
												BackBufferCamUp = 400.0f;
												bSnapShotModeUseCamera = false;
												BackBufferParticlesMode = true;
												bLoopBackBuffer = true;

												float centerx = -1000, centery = 39000, centerz = -1000;
												std::string sParticleName = myfiles->m_sPath.Get();
												sParticleName = sParticleName + "\\" + Left(myfiles->m_sName.Get(), Len(myfiles->m_sName.Get()) - 4);

												if (BackBufferParticleEmitter != -1)
												{
													gpup_deleteEffect(BackBufferParticleEmitter);
													BackBufferParticleEmitter = -1;
												}

												if (BackBufferParticleEmitter == -1)
												{
													BackBufferParticleEmitter = gpup_loadEffect(sParticleName.c_str(), 0, 0, 0, 1.0);
													gpup_setGlobalScale(BackBufferParticleEmitter, 100.0f);
													gpup_emitterActive(BackBufferParticleEmitter, 0);
												}
												float fLive = 10.0f;
												if (BackBufferParticleEmitter != -1)
												{
													gpup_setGlobalPosition(BackBufferParticleEmitter, centerx, centery - 30.0f, centerz);
													gpup_resetLocalPosition(BackBufferParticleEmitter);
													gpup_setGlobalScale(BackBufferParticleEmitter, 100.0f);
													gpup_emitterActive(BackBufferParticleEmitter, 1);

													gpup_setEffectAnimationSpeed(BackBufferParticleEmitter, 1.0f);
													gpup_setEffectOpacity(BackBufferParticleEmitter, 1.0f);

												}

											}
											else
											{
												//We are running
												BackBufferZoom = 700.0f;
												BackBufferCamUp = 250.0f;
												bSnapShotModeUseCamera = false;
												BackBufferParticlesMode = true;
												bLoopBackBuffer = true;
											}
										}
									}
									else if (iDisplayLibraryType == 5 && !ImGui::IsItemHovered())
									{
										if (additionalcheck == myfiles)
										{
											bTriggerTimer = false;
											if (bLoopBackBuffer)
											{
												//Disable rotation
												BackBufferSaveCacheName = "";
												BackBufferObjectID = 0;
												BackBufferImageID = 0;
												BackBufferZoom = 0.0f;
												BackBufferCamUp = 0.0f;
												bSnapShotModeUseCamera = false;
												BackBufferParticlesMode = false;
												bLoopBackBuffer = false;
												myfiles->iPreview = 0; //Reload old thumb.
												additionalcheck = NULL;
												if (BackBufferParticleEmitter != -1)
												{
													gpup_deleteEffect(BackBufferParticleEmitter);
													BackBufferParticleEmitter = -1;
												}
											}
										}
									}
								}

								if (bThumbHovered)
								{
									if (iDisplayLibraryType == 1)
									{
										//ImGui::SetTooltip("Music Media");
									}
									if (iDisplayLibraryType == 2)
									{
										//PE: Show large image preview.
										float imgw = ImageWidth(textureId);
										float imgh = ImageHeight(textureId);
										float fWidth = 500.0f;
										float fRatio = fWidth / imgw;
										if (imgw < fWidth) fRatio = 1.0f;
										ImVec2 imgsize = ImVec2(imgw*fRatio, imgh*fRatio);

										ImGui::BeginTooltip();
										float icon_ratio;
										ImGui::ImgBtn(textureId, imgsize, ImVec4(0.0, 0.0, 0.0, 1.0), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(0.8, 0.8, 0.8, 0.8), ImVec4(0.8, 0.8, 0.8, 0.8), 0, 0, 0, 0, false);
										ImGui::EndTooltip();
									}
								}
							}
							else
							{
								bool bHoverGroupActive = true;
								if ( myfiles->m_bIsGroupObject) bHoverGroupActive = false; // do not allow smarts to load/generate thumbs - performance hit!
								if (bHoverGroupActive && !bImagesStillInImGuiQueue && !bLargePreview && !bBlockBackBufferUpdating && !bEntity_Properties_Window && !g_bCharacterCreatorPlusActivated && !bImporter_Window && i == 0 && bThumbHovered)
								{
									iTooltipHoveredTimer = MAXTimer();
									if (iLastTooltipSelection != textureId || (additionalcheck != myfiles))
									{
										// new thumbnail to create
										additionalcheck = myfiles;
										iTooltipTimer = iTooltipHoveredTimer;
										iLastTooltipSelection = textureId;
										iTooltipObjectReady = false;

										// object FPE name
										std::string sFpeName = path_for_filename.c_str();
										sFpeName = sFpeName + "\\" + myfiles->m_sName.Get();
										t.addentityfile_s = sFpeName.c_str();

										// if we already have it, can show right away!
										t.talreadyloaded = 0;
										for (t.t = 1; t.t <= g.entidmaster; t.t++)
										{
											if (t.entitybank_s[t.t] == t.addentityfile_s) { t.talreadyloaded = 1; t.entid = t.t; }
										}
										if (t.talreadyloaded)
										{
											//Start rotate instant.
											CreateBackBufferCacheName(t.addentityfile_s.Get(), thumb_x, thumb_y);
											GG_SetWritablesToRoot(true);
											if (FileExist(BackBufferCacheName.Get()) == 0)
											{
												// only save new thumb if not exist in root
												BackBufferSaveCacheName = BackBufferCacheName;
											}
											GG_SetWritablesToRoot(false);
											bDoBackbufferUpdate = true;
											iTooltipTimer = iTooltipHoveredTimer - 750;
											iTooltipObjectReady = true;
											preload_fpe_file = "";
										}
										else
										{
											// Preload DBO and textures shortly
											preload_fpe_file = t.addentityfile_s;
											preload_fpe_file_started = false;
										}
									}
									else
									{
										if (strlen(preload_fpe_file.Get()) == 0)
										{
											if (iTooltipObjectReady)
											{
												// showing it
											}
											else
											{
												// Generate Thumbnail of object.
												std::string sFpeName = path_for_filename.c_str();
												sFpeName = sFpeName + "\\" + myfiles->m_sName.Get();
												t.addentityfile_s = sFpeName.c_str();
												CreateBackBufferCacheName(t.addentityfile_s.Get(), thumb_x, thumb_y);
												GG_SetWritablesToRoot(true);
												if (FileExist(BackBufferCacheName.Get()) == 0)
												{
													// only save new thumb if not exist in root
													BackBufferSaveCacheName = BackBufferCacheName;
												}
												GG_SetWritablesToRoot(false);
												bDoBackbufferUpdate = true;
											}
										}
										else
										{
											#ifdef PRELOAD_OBJECTS_ON_HOVER
											// wait long enough to ignore incidental scrolling past the object
											if (iTooltipHoveredTimer - iTooltipTimer > 100)
											{
												if (preload_fpe_file.Len() > 0)
												{
													// trigger this object to preload its resources, ready for the actual DBO and DDS loading
													if (preload_fpe_file_started == false)
													{
														if (object_preload_files_in_progress() == false && image_preload_files_in_progress() == false)
														{
															if (entity_load_thread_prepare(preload_fpe_file.Get()) == false)
															{
																// this file do not have a dbo version, do not preload
															}
														}
														preload_fpe_file_started = true;
													}

													// only when waited full span of progress bar do we release preload_fpe_file to start thumbn generation (small delay due to Wicked)
													if (iTooltipHoveredTimer - iTooltipTimer > 750)
													{
														preload_fpe_file = "";
													}
												}
											}
											#else
											preload_fpe_file = "";
											#endif
											if (!bInContextThumb)
											{
												//PE: Draw progress bar end
												int iVal = iTooltipHoveredTimer - iTooltipTimer;
												float progress = (float)iVal / 750;// 300;//quicker 2000.0f;
												if (progress < 0.0) progress = 0.0;
												if (progress > 1.0) progress = 1.0;
												ImVec2 opos = ImGui::GetCursorPos();
												ImVec4 backImGuiCol_PlotHistogram = ImGui::GetStyle().Colors[ImGuiCol_PlotHistogram];
												ImVec4 backImGuiCol_Border = ImGui::GetStyle().Colors[ImGuiCol_Border];
												ImVec4 backImGuiCol_BorderShadow = ImGui::GetStyle().Colors[ImGuiCol_BorderShadow];
												ImVec4 backImGuiCol_FrameBg = ImGui::GetStyle().Colors[ImGuiCol_FrameBg];

												ImVec4 newBlue = ImVec4(56.0 / 255.0, 110.0 / 255.0, 145.0 / 255.0, 1);
												ImGui::GetStyle().Colors[ImGuiCol_PlotHistogram] = newBlue;
												ImGui::GetStyle().Colors[ImGuiCol_Border] = ImVec4(0, 0, 0, 0);
												ImGui::GetStyle().Colors[ImGuiCol_BorderShadow] = ImVec4(0, 0, 0, 0);
												ImGui::GetStyle().Colors[ImGuiCol_FrameBg] = ImVec4(0, 0, 0, 0);

												ImGui::SetCursorPos(ImVec2(opos.x + 4.0f, opos.y - 16.0f));
												ImGui::ProgressBar(progress, ImVec2(media_icon_size - 4.0f, 6.0f), " ");
												ImGui::SetCursorPos(opos);

												ImGui::GetStyle().Colors[ImGuiCol_PlotHistogram] = backImGuiCol_PlotHistogram;
												ImGui::GetStyle().Colors[ImGuiCol_Border] = backImGuiCol_Border;
												ImGui::GetStyle().Colors[ImGuiCol_BorderShadow] = backImGuiCol_BorderShadow;
												ImGui::GetStyle().Colors[ImGuiCol_FrameBg] = backImGuiCol_FrameBg;
											}
										}
									}
								}
								else
								{
									if (!ImGui::IsItemHovered() && !bLargePreview && !bImagesStillInImGuiQueue)
									{
										if (textureId >= 4000 && textureId < UIV3IMAGES && BackBufferImageID == textureId)
										{
											//Disable rotation
											BackBufferSaveCacheName = "";
											BackBufferObjectID = 0;
											BackBufferImageID = 0;
											bLoopBackBuffer = false;
											RevertBackbufferCubemap();
										
											WickedCall_SetSunDirection(t.visuals.SunAngleX, t.visuals.SunAngleY, t.visuals.SunAngleZ);
											master_renderer->setBloomEnabled(t.visuals.bBloomEnabled);
											WickedCall_MoveReflectionProbe(GGORIGIN_X, GGORIGIN_Y+5000, GGORIGIN_Z, "editorProbe", 500);
											WickedCall_EnableThumbLight(false);
											myfiles->iPreview = 0;
											additionalcheck = NULL;
										}
									}
								}
							}

							//PE: Check if we need to generate a thumb or rotate a object in the thumb view.
							bool bForceUpdate = false;
							if (bCheckGotoPreview && pPreviewFile == myfiles)
								bForceUpdate = true;

							if (!bBlockBackBufferUpdating || bForceUpdate)
							{
								if (!bDoneOneThumbPerSync && bLoadedInNewFormat)
								{
									//PE: Should we try to update old thumb to new format ?
									if (myfiles->iBigPreview == 0)
									{
										//PE: Only if it is side visible area.
										int gcpy = ImGui::GetCursorPosY();
										if (gcpy < iIconVisiblePosY && gcpy >= ImGui::GetScrollY() - media_icon_size)
										{
											std::string sFpeName = path_for_filename.c_str();
											sFpeName = sFpeName + "\\" + myfiles->m_sName.Get();
											t.addentityfile_s = sFpeName.c_str();
											CreateBackBufferCacheName(t.addentityfile_s.Get(), thumb_x, thumb_y);
											GG_SetWritablesToRoot(true);
											if (!FileExist(BackBufferCacheName.Get()))
											{
												bLoadedInNewFormat = false; //Try it.
											}
											GG_SetWritablesToRoot(false);
										}
									}
								}
								if ((bDoBackbufferUpdate || !bLoadedInNewFormat) && !bDoneOneThumbPerSync)
								{
									std::string sFpeName = path_for_filename.c_str();
									sFpeName = sFpeName + "\\" + myfiles->m_sName.Get();
									t.addentityfile_s = sFpeName.c_str();

									// LB: delay actual entity_load (takes >1s due to Wicked obj setup in scene!)
									bool update = true;
									if (preload_fpe_file.Len() > 0)
									{
										//PE: This ruins thumb updating we must always have update so...
										if (!bDoBackbufferUpdate && bLoadedInNewFormat)
										{
											update = false;
											if (iTooltipHoveredTimer - iTooltipTimer > 750)
											{
												// this extension allows user to hover and click without getting the freeze
												update = true;
											}
										}
									}

									if (!bDoBackbufferUpdate && !bLoadedInNewFormat)
									{
										CreateBackBufferCacheName(t.addentityfile_s.Get(), thumb_x, thumb_y);
										GG_SetWritablesToRoot(true);
										if (FileExist(BackBufferCacheName.Get()) == 0)
										{
											// only save new thumb if not exist in root
											BackBufferSaveCacheName = BackBufferCacheName;
										}
										GG_SetWritablesToRoot(false);

										//PE: Triggered auto.
										if (myfiles->iBigPreview == 0)
										{
											myfiles->iBigPreview = TOOL_ENTITY;
											//PE: Update image in next run.
											if (myfiles->iPreview > 0)
											{
												if (GetImageExistEx(myfiles->iPreview) && myfiles->iPreview >= 4000 && myfiles->iPreview < UIV3IMAGES) { //PE: Need to protect system images after tool img range has changed. (myfiles->iPreview can be a system icon)
													iDeleteInNextUpdate = myfiles->iPreview;
												}
											}
											myfiles->iPreview = 0;
										}
										else
										{
											update = false;
										}
									}
									if (update)
									{
										int iEntIDWas = 0;
										bool bIsGroup = false;
										cstr EntWas_s = t.addentityfile_s;
										if (iDisplayLibrarySubType == 1)
										{
											// Animations
											t.entdir_s = "charactercreatorplus\\animations\\";
										}
										else
										{
											// Objects
											t.entdir_s = "entitybank\\";
											if (cstr(Lower(Left(t.addentityfile_s.Get(), 11))) == "entitybank\\")
											{
												t.addentityfile_s = Right(t.addentityfile_s.Get(), Len(t.addentityfile_s.Get()) - 11);
											}
											if (cstr(Lower(Left(t.addentityfile_s.Get(), 8))) == "ebebank\\")
											{
												t.entdir_s = "";
											}
										}

										t.talreadyloaded = 0;
										for (t.t = 1; t.t <= g.entidmaster; t.t++)
										{
											if (t.entitybank_s[t.t] == t.addentityfile_s) { t.talreadyloaded = 1; t.entid = t.t; }
										}
										if (t.talreadyloaded == 1)
										{
											//PE: Check ,if group.
											if (t.entityprofile[t.entid].model_s == "group" && t.entityprofile[t.entid].groupreference == 1)
											{
												cstr tmp = cstr("entitybank\\") + t.addentityfile_s;
												extern int GetGroupIndexFromName(cstr sLookFor);
												int groupID = GetGroupIndexFromName(tmp);
												if (groupID >= 0 && groupID < MAXGROUPSLISTS)
												{
													bIsGroup = true;
													g_LastGroupSaved_s = t.entitybank_s[t.entid];
													iEntIDWas = t.entid;


													for (int i = 0; i < vEntityGroupList[groupID].size(); i++)
													{
														// first update object from final entity element data
														int e = vEntityGroupList[groupID][i].e;
														int entid = t.entityelement[e].bankindex;
														if (t.entityprofile[entid].ismarker == 0)
														{
															// use this one!
															t.entid = entid;
															break;
														}
													}
													if (!bLargePreview)
													{
														//PE: Drop rotate if we already got the original group thumb.
														CreateBackBufferCacheName(t.addentityfile_s.Get(), thumb_x, thumb_y);
														GG_SetWritablesToRoot(true);
														if (FileExist(BackBufferCacheName.Get()))
														{
															update = false;
														}
														GG_SetWritablesToRoot(false);
													}
												}
											}
										}
											
										if (t.talreadyloaded == 0)
										{
											// Allocate one more entity item in array
											if (g.entidmaster > g.entitybankmax - 4)
											{
												Dim(t.tempentitybank_s, g.entitybankmax);
												for (t.t = 0; t.t <= g.entitybankmax; t.t++) t.tempentitybank_s[t.t] = t.entitybank_s[t.t];
												++g.entitybankmax;
												UnDim(t.entitybank_s);
												Dim(t.entitybank_s, g.entitybankmax);
												for (t.t = 0; t.t <= g.entitybankmax - 1; t.t++) t.entitybank_s[t.t] = t.tempentitybank_s[t.t];
											}

											bool bAutoGenerateThumb = false;
											if (!bDoBackbufferUpdate)
											{
												bAutoGenerateThumb = true;
											}

											//  Add entity to bank
											if (!bAutoGenerateThumb)
												++g.entidmaster;

											entity_validatearraysize();

											//  Load extra entity
											t.entid = g.entidmaster;

											//PE: Try to use index 0 on all auto generated thumbs.
											if (bAutoGenerateThumb)
												t.entid = 0;

											t.entitybank_s[t.entid] = t.addentityfile_s;
											iEntIDWas = t.entid;

											if (ObjectExist(g.entitybankoffset + t.entid)) 
											{
												//PE: We use a before/after list to free all used textures later.
												DeleteObject(g.entitybankoffset + t.entid);
											}

											t.ent_s = t.entitybank_s[t.entid];
											t.entpath_s = getpath(t.ent_s.Get());

											extern bool g_bGracefulWarningAboutOldXFiles;
											extern bool g_bDisplayWarnings;
											g_bGracefulWarningAboutOldXFiles = true;
											g_bDisplayWarnings = false;
											extern cstr g_sTempGroupForThumbnail;
											if (g_sTempGroupForThumbnail.Len() > 0)
											{
												extern int GetGroupIndexFromName (cstr sLookFor);
												current_selected_group = GetGroupIndexFromName(g_sTempGroupForThumbnail);
												UnGroupSelected(true);
												gridedit_deleteentityrubberbandfrommap();
												gridedit_clearentityrubberbandlist();
												g_sTempGroupForThumbnail = "";
											}
											extern int g_iAbortedAsEntityIsGroupFileMode;
											g_iAbortedAsEntityIsGroupFileMode = 1;
											extern int thumb_selected_group;
											thumb_selected_group = -1;
											g.thumbentityrubberbandlist.clear();
											
											bool bcalledfromlibrary = true; 
											if(bLargePreview) bcalledfromlibrary = false;

											if (entity_load(bcalledfromlibrary) == false)
											{
												// entity was a group object (smart object)
												if (thumb_selected_group >= 0)
													g_sTempGroupForThumbnail = sEntityGroupListName[thumb_selected_group];
												else
													g_sTempGroupForThumbnail = "";
												bIsGroup = true;

												g_LastGroupSaved_s = t.entitybank_s[iEntIDWas];

												// undo entry, it is NOT an entity of its own!
												t.entitybank_s[iEntIDWas] = "";

												// so scan group loaded and find first object that is not a marker (to display in preview)
												for (int i = 0; i < g.thumbentityrubberbandlist.size(); i++)
												{
													// first update object from final entity element data
													int e = g.thumbentityrubberbandlist[i].e;
													int entid = t.entityelement[e].bankindex;
													if (t.entityprofile[entid].ismarker == 0)
													{
														// use this one!
														t.entid = entid;
														break;
													}
												}

												if (!bLargePreview)
												{
													//PE: Drop rotate if we already got the original group thumb.
													CreateBackBufferCacheName(t.addentityfile_s.Get(), thumb_x, thumb_y);
													GG_SetWritablesToRoot(true);
													if (FileExist(BackBufferCacheName.Get()))
													{
														update = false;
													}
													GG_SetWritablesToRoot(false);
												}
											}
											g_bDisplayWarnings = true;
											g_bGracefulWarningAboutOldXFiles = false;

											HideObject(g.entitybankoffset + t.entid);
											if (t.entityprofile[t.entid].ischaracter == 1) 
											{
												RotateObject(g.entitybankoffset + t.entid, 0, 180, 0);
											}

											//entity_load can change folder by creating a dbo , so update timestamp without refresh.
											struct stat sb;
											if (stat(pNewFolder->m_sFolderFullPath.Get(), &sb) == 0) 
											{
												if (sb.st_mtime != pNewFolder->m_tFolderModify) 
												{
													pNewFolder->m_tFolderModify = sb.st_mtime;
												}
											}
										}

										if (update)
										{
											iTooltipLastObjectId = t.entid;
											iTooltipAlreadyLoaded = t.talreadyloaded;
											iTooltipObjectReady = true;

											BackBufferIsGroup = bIsGroup;
											BackBufferEntityID = t.entid;
											BackBufferObjectID = g.entitybankoffset + t.entid;
											BackBufferImageID = g.importermenuimageoffset + 50;

											BackBufferRotateZ = ObjectAngleZ(BackBufferObjectID);
											BackBufferRotateY = ObjectAngleY(BackBufferObjectID) + 15;
											BackBufferRotateX = ObjectAngleX(BackBufferObjectID);
											BackBufferZoom = 1.0f;
											BackBufferCamLeft = 0.0f;
											BackBufferCamUp = 0.0f;

											if (bLargePreview)
											{
												iLargePreviewImageID = BackBufferImageID;
												BackBufferSizeX = thumb_x * 2.0f;
												BackBufferSizeY = thumb_y * 2.0f;
											}
											else
											{
												BackBufferSizeX = thumb_x;
												BackBufferSizeY = thumb_y;
											}
											if (ObjectExist(BackBufferObjectID) && GetNumberOfFrames(BackBufferObjectID) > 0 && t.entityprofile[BackBufferEntityID].animmax > 0)
											{
												//PE: Stop any running animations. set default pose.
												int iFrameStart = t.entityanim[BackBufferEntityID][0].start;
												SetObjectFrame(BackBufferObjectID, iFrameStart);
											}
											BackBufferZoom = 1.0f;
											BackBufferCamLeft = 0.0f;
											BackBufferCamUp = 0.0f;
											bRotateBackBuffer = false;
											bBackBufferAnimated = false;
											bLoopBackBuffer = false;
											RevertBackbufferCubemap();

											//PE: We must enable editor light here, so its ready for the next frame where we grab the backbuffer.
											if (!bDoBackbufferUpdate)
											{
												WickedCall_EnableThumbLight(true);
											}

											if (bDoBackbufferUpdate)
											{
												if (textureId >= 4000 && textureId < UIV3IMAGES)
												{
													if (bLargePreview)
														bRotateBackBuffer = false;
													else
														bRotateBackBuffer = true;
												}
											}

											//PE: Prefer myfiles->m_Backdrop it has the latest changes.
											if (!(t.entityprofile[t.entid].BackBufferZoom == -1.0f && t.entityprofile[t.entid].BackBufferCamLeft == -1.0f && t.entityprofile[t.entid].BackBufferRotateX == -1.0f))
											{
												//Found settings, set restore.
												RestoreBackBufferZoom = t.entityprofile[t.entid].BackBufferZoom;
												RestoreBackBufferCamLeft = t.entityprofile[t.entid].BackBufferCamLeft;
												RestoreBackBufferCamUp = t.entityprofile[t.entid].BackBufferCamUp;
												RestoreBackBufferRotateX = t.entityprofile[t.entid].BackBufferRotateX;
												RestoreBackBufferRotateY = t.entityprofile[t.entid].BackBufferRotateY;
												bBackBufferRestoreCamera = true; //Restore from fpe settings in next call.

												if (ObjectExist(BackBufferObjectID) && GetNumberOfFrames(BackBufferObjectID) > 0 && t.entityprofile[BackBufferEntityID].animmax > 0)
												{
													if (t.entityprofile[BackBufferEntityID].iThumbnailAnimset >= 0 && t.entityprofile[BackBufferEntityID].iThumbnailAnimset < t.entityprofile[BackBufferEntityID].startofaianim)
													{
														// new method uses name instead of fixed values
														if (t.entityprofile[BackBufferEntityID].playanimineditor == -1)
														{
															// uses name instead of index, the negative is the ordinal into the animset
															extern void entity_loop_using_negative_playanimineditor(int e, int obj, cstr animname);
															entity_loop_using_negative_playanimineditor(0, BackBufferObjectID, t.entityprofile[BackBufferEntityID].playanimineditor_name);
														}
														else
														{
															int iAnimationSet = t.entityprofile[BackBufferEntityID].iThumbnailAnimset;
															int iFrameStart = t.entityanim[BackBufferEntityID][iAnimationSet].start;
															int iFrameEnd = t.entityanim[BackBufferEntityID][iAnimationSet].finish;
															SetObjectFrame(BackBufferObjectID, iFrameStart);
															LoopObject(BackBufferObjectID, iFrameStart, iFrameEnd);
														}
														bBackBufferAnimated = true;
													}
													if (bBackBufferAnimated == true)
													{
														if (t.entityprofile[BackBufferEntityID].animspeed > 0)
														{
															t.tanimspeed_f = t.entityprofile[BackBufferEntityID].animspeed;
															SetObjectSpeed(BackBufferObjectID, t.tanimspeed_f);
														}
													}
												}
											}

											if (bIsGroup == true)
											{
												CreateBackdropObject(false, cstr("texturebank\\backdrops\\") + t.entityprofile[iEntIDWas].thumbnailbackdrop, EntWas_s);
											}
											else
											{
												if (myfiles->m_Backdrop.Len() > 0)
												{
													CreateBackdropObject(false, cstr("texturebank\\backdrops\\") + myfiles->m_Backdrop, t.addentityfile_s); //PE: We need a extra frame, as we set the material dirty in this call.
												}
												else
												{
													if (t.entityprofile[t.entid].thumbnailbackdrop.Len() > 0)
														CreateBackdropObject(false, cstr("texturebank\\backdrops\\") + t.entityprofile[t.entid].thumbnailbackdrop, t.addentityfile_s);
													else
														CreateBackdropObject(false, "None", t.addentityfile_s);
												}
											}

											if (bDoBackbufferUpdate)
											{
												BackBufferSaveCacheName = ""; //No saving on tooltip images

												if (textureId >= 4000 && textureId < UIV3IMAGES)
												{
													BackBufferRotateZ = ObjectAngleZ(BackBufferObjectID);
													BackBufferRotateY = ObjectAngleY(BackBufferObjectID) + 15;
													BackBufferRotateX = ObjectAngleX(BackBufferObjectID);
													BackBufferZoom = 1.0f;
													BackBufferCamLeft = 0.0f;
													BackBufferCamUp = 0.0f;
													if (bLargePreview)
														bRotateBackBuffer = false;
													else
														bRotateBackBuffer = true;
													BackBufferImageID = textureId;
													iLargePreviewImageID = BackBufferImageID;
													bLoopBackBuffer = true;
												}
											}

											if (bForceUpdate)
											{
												BackBufferRotateZ = ObjectAngleZ(BackBufferObjectID);
												BackBufferRotateY = ObjectAngleY(BackBufferObjectID) + 15;
												BackBufferRotateX = ObjectAngleX(BackBufferObjectID);
												BackBufferZoom = 1.0f;
												BackBufferCamLeft = 0.0f;
												BackBufferCamUp = 0.0f;
												bRotateBackBuffer = false;
												bLoopBackBuffer = true;
											}

											bDoneOneThumbPerSync = true;
										}
										else
										{
											bRotateBackBuffer = false;
											bLoopBackBuffer = false;
										}
									}
								}
							}

							ImGui::PopStyleVar();

							ImGui::PopID();
							ImGui::PushID(uniqueId + 30000);
							ImVec2 opos = ImGui::GetCursorPos();

							if (myfiles->bFavorite)
							{
								int iImageSize = 20;
								ImGui::SetCursorPos(ImVec2(opos.x + 12.0f, opos.y - 16.0f - iImageSize));
								ImGui::SetItemAllowOverlap();
								if (ImGui::ImgBtn(MEDIA_FAVORITE, ImVec2(iImageSize, iImageSize), ImColor(0, 0, 0, 0), ImColor(255, 255, 255, 180), ImColor(255, 255, 255, 180), ImColor(255, 255, 255, 180), 0, 0, 0, 0, false, false, false, false, true, bBoostIconColors))
								{
									//Remove from favorite.
									cstr file = myfiles->m_sPath;
									file = file + "\\" + myfiles->m_sName.Get();
									file = file.Lower();
									extern std::vector<std::string> files_favorite;
									myfiles->bFavorite = false;
									for (int i = 0; i < files_favorite.size(); i++)
									{
										cstr check = cstr((char *)files_favorite[i].c_str()).Lower();
										if (file == check)
										{
											//Delete.
											files_favorite.erase(files_favorite.begin() + i);
										}
									}
									saveVectorFileContent("favoritelist.ini", files_favorite);
								}
								ImGui::SetCursorPos(opos);
								if (!bThumbHovered)
									bThumbHovered = ImGui::IsItemHovered();
								if (ImGui::IsItemHovered()) ImGui::SetTooltip("Remove From Favourite");
							}
							else
							{
								int iImageSize = 20;
								ImVec2 opos = ImGui::GetCursorPos();
								ImGui::SetCursorPos(ImVec2(opos.x + 12.0f, opos.y - 16.0f - iImageSize));
								ImGui::SetItemAllowOverlap();
								if (ImGui::ImgBtn(MEDIA_FAVORITE_DIS, ImVec2(iImageSize, iImageSize), ImColor(0, 0, 0, 0), ImColor(255, 255, 255, 180), ImColor(255, 255, 255, 180), ImColor(255, 255, 255, 180), 0, 0, 0, 0, false, false, false, false, false, bBoostIconColors))
								{
									//Add to favorite.
									cstr file = myfiles->m_sPath;
									file = file + "\\" + myfiles->m_sName.Get();
									extern std::vector<std::string> files_favorite;
									myfiles->bFavorite = true;
									files_favorite.push_back(file.Get());
									saveVectorFileContent("favoritelist.ini", files_favorite);
								}
								ImGui::SetCursorPos(opos);
								if (!bThumbHovered)
									bThumbHovered = ImGui::IsItemHovered();
								if (ImGui::IsItemHovered()) ImGui::SetTooltip("Add to Favourite");
							}

							if (bDisplayText)
							{
								if (iDisplayLibraryType == 4)
								{
									char cDisplayName[MAX_PATH];
									strcpy (cDisplayName, sFinal.c_str());
									FormatLUAFilenameToTitle(cDisplayName);
									sFinal = cstr(cstr(cDisplayName) + myfiles->m_sNameFinalCredit.Get()).Get();
									ImGui::Text("%s", sFinal.c_str());
									if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", sFinal.c_str());

									if (myfiles && myfiles->m_sDLuaDescription.Get())
									{
										if (myfiles->m_sDLuaDescription != "")
										{
											//Always use same size box on all DLUA boxes. looks better.
											ImGui::BeginChild("##DLUADescriptionbox", ImVec2(media_icon_size, (ImGui::GetTextLineHeight() * 5) - 7.0f), true, ImGuiWindowFlags_NoMove); //| ImGuiWindowFlags_AlwaysUseWindowPadding
											ImGui::TextWrapped("%s", myfiles->m_sDLuaDescription.Get()); 
											ImGui::EndChild();
											ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPos().x, ImGui::GetCursorPos().y + 3.0f));
										}
									}
								}
								else
								{
									std::string sFinalForDisplay = sFinal + myfiles->m_sNameFinalCredit.Get();
									if (iDisplayLibraryType == 0)
									{
										ImGui::Text("  %s", sFinalForDisplay.c_str());
									}
									else
									{
										ImGui::Text("%s", sFinalForDisplay.c_str());
									}
									if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", sFinalForDisplay.c_str());
								}
							}

							if (bSecondScrollActive && secondscrolltofile && secondscrolltofile == myfiles)
							{
								secondscrolltofile = NULL;
								//PE: Test second scrool if list is correct.
								if (imgui_GetMinMaxButtonState(0))
								{
									//Full screen.
									if (ImGui::GetCursorPosY() > (media_icon_size*0.75))
										ImGui::SetScrollY(ImGui::GetCursorPosY()); //+(media_icon_size*0.5));
									else
										ImGui::SetScrollY(0.0f);
								}
								else
								{
									float cpy = ImGui::GetCursorPosY();
									float scy = ImGui::GetScrollY();
									if (cpy > (media_icon_size*0.75))
									{
										//PE: Try DirectPosition.y;
										if (0)
										{
											ImGui::SetScrollY(DirectPosition.y);
										}
										else
										{
											if (cpy < ((media_icon_size*3.0)*0.75))
											{
												ImGui::SetScrollY(0.0);
											}
											else if (scy > 0 && cpy > (media_icon_size*2.0))
											{
												ImGui::SetScrollY(cpy - (media_icon_size*2.0));
											}
											else if (scy > 0 && cpy > (media_icon_size))
											{
												ImGui::SetScrollY(cpy - (media_icon_size));
											}
											else
											{
												ImGui::SetScrollY(DirectPosition.y + (media_icon_size*0.75));
											}
										}
									}
									else
										ImGui::SetScrollY(0.0f);
								}

							}
							//PE: Check if we need to adjust scroll after a window resize.
							if (!firstvisiblefile && scrolltofile && scrolltofile == myfiles)
							{
								if (bScrollInNextFrame)
								{
									firstvisiblefile = scrolltofile;
									bScrollInNextFrame = false;
								}
								else
								{
									firstvisiblefile = scrolltofile;
									secondscrolltofile = scrolltofile;
									bSecondScrollActive = false;
									scrolltofile = NULL;
									if (imgui_GetMinMaxButtonState(0))
									{
										//Full screen.
										if (ImGui::GetCursorPosY() > (media_icon_size*0.75))
											ImGui::SetScrollY(ImGui::GetCursorPosY()); //+(media_icon_size*0.5));
										else
											ImGui::SetScrollY(0.0f);
									}
									else
									{
										float cpy = ImGui::GetCursorPosY();
										float scy = ImGui::GetScrollY();
										if (cpy > (media_icon_size*0.75))
										{
											//PE: Try DirectPosition.y;
											if (0)
											{
												ImGui::SetScrollY(DirectPosition.y);
											}
											else
											{
												if (cpy < ((media_icon_size*3.0)*0.75))
												{
													ImGui::SetScrollY(0.0);
												}
												else if (scy > 0 && cpy > (media_icon_size*2.0))
												{
													ImGui::SetScrollY(cpy - (media_icon_size*2.0));
												}
												else if (scy > 0 && cpy > (media_icon_size))
												{
													ImGui::SetScrollY(cpy - (media_icon_size));
												}
												else
												{
													ImGui::SetScrollY(DirectPosition.y + (media_icon_size*0.75) );
												}
											}
										}
										else
											ImGui::SetScrollY(0.0f);
									}
								}
							}

							//PE: Draw selection last so we can overlap spacing.
							if( (myfiles->iFlags == 1 || selectedmediafile == myfiles) && bVisibleOnScreen)
							{
								ImGuiWindow* window = ImGui::GetCurrentWindow();
								ImVec4 bg_col = ImGui::GetStyle().Colors[ImGuiCol_PlotHistogram]; // { 0.0, 0.0, 0.0, 1.0 };
								window->DrawList->AddRect(selection_bb.Min, selection_bb.Max, ImGui::GetColorU32(bg_col), 0.0f, 0, 3.0f);
							}

							ImGui::NextColumn();
						}
						ImGui::PopID();
						preview_count++;
					}
					ImGui::SetWindowFontScale(1.0);

				} //bValid

			} //#### MAIN LOOP iLoop < sorted_files.size() ####

			ImGui::EndColumns();

			if (ImGui::GetCurrentWindow()->ScrollbarSizes.x > 0) 
			{
				//Hitting exactly at the botton could cause flicker, so add some additional lines when scrollbar on.
				ImGui::Text("");
				ImGui::Text("");
			}
		}

		if (!bAnySelectedItemsAvailable) 
		{
			//PE: We got no selections , we can reset first shift seen.
			if (!io.KeyShift) 
			{
				firstShiftFile = NULL;
			}
		}
		ImGui::SetWindowFontScale(1.0);

		ImRect bbwinclient(ImGui::GetWindowPos(), ImGui::GetWindowPos() + ImGui::GetWindowSize());
		if (ImGui::IsMouseHoveringRect(bbwinclient.Min, bbwinclient.Max))
		{
			bImGuiGotFocus = true;
			bEntityGotFocus = true;
		}
		if (ImGui::IsAnyItemFocused()) 
		{
			bImGuiGotFocus = true;
			bEntityGotFocus = true;
		}
		
		ImGui::EndChild();

		ImRect bbwin(ImGui::GetWindowPos(), ImGui::GetWindowPos() + ImGui::GetWindowSize());
		if (ImGui::IsMouseHoveringRect(bbwin.Min - ImVec2(1, 1), bbwin.Max , false))
		{
			bImGuiGotFocus = true;
			bEntityGotFocus = true;
		}
		if (ImGui::IsAnyItemFocused()) 
		{
			bImGuiGotFocus = true;
			bEntityGotFocus = true;
		}

		ImGui::Indent(10);
		ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPos().x, ImGui::GetCursorPos().y+1.0f));

		ImGui::PushItemWidth(150.f);
		float fSliderPos = ImGui::GetContentRegionAvailWidth() - 150.0f - 16.0f;
		ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPos().x + fSliderPos, ImGui::GetCursorPos().y + 7.0f));
		ImGui::SetWindowFontScale(0.6);
		int iMaxSlider = 8;
		if (fSliderPos > 820) iMaxSlider++;
		if (fSliderPos > 1000) iMaxSlider++;
		if (pref.iSetColumnsEntityLib > iMaxSlider) pref.iSetColumnsEntityLib = iMaxSlider;
		if (pref.iSetColumnsEntityLib < 1) pref.iSetColumnsEntityLib = 1;

		ImGui::SliderInt("##fScaleIcons", &pref.iSetColumnsEntityLib, 1, iMaxSlider, " ");
		ImGui::SetWindowFontScale(1.0);
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("Set Columns %d" , pref.iSetColumnsEntityLib);
		ImGui::PopItemWidth();
		ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPos().x, ImGui::GetCursorPos().y +7.0f));

		//PE: Display new buttons.
		int iButtons = 3;
		if (lf_multi_selections_count > 0)
			iButtons++;

		ImVec2 vContentSize = ImGui::GetContentRegionAvail();
		float fButWidth = vContentSize.x / iButtons;
		fButWidth -= 10.0f; //But spacing.
		if (fButWidth < 30.0f)
			fButWidth = 30.0f;

		float fFontSize = ImGui::GetFontSize();
		ImGui::SetWindowFontScale(1.4);

		if (bTriggerCloseEntityWindow)
		{
			bCheckForClosing = true;
			bTriggerCloseEntityWindow = false;
		}

		if (iDisplayLibraryType > 0)
		{
			//###############
			//#### Audio ####
			//###############

			if (iDisplayLibraryType == 1) //Music
			{
				int buts = 2;
				if (selectedmediafile != NULL) buts = 3;
				fButWidth = vContentSize.x / buts;
				fButWidth -= 10.0f;
				// Marketplace
				#ifndef GGMAXEDU
				if (ImGui::StyleButton("Get More Music and Sound", ImVec2(fButWidth, fFontSize*2.0)))
				{
					DeleteWaypointsAddedToCurrentCursor();
					CloseDownEditorProperties();
					bMarketplace_Window = true;
				}
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("Click to get more Music and Sound");
				#endif
				// Direct import here
				if (1)
				{
					static int import_process = 0;
					static cstr import_filename = "";
					static char import_name[MAX_PATH];
					if (import_process == 1)
					{
						static char import_to[MAX_PATH];
						strcpy(import_to, "audiobank\\users"); //currently fixed.

						bool bValid = true;
						if (strlen(import_name) < 1 || import_filename.Len() < 1)
						{
							bValid = false;
							import_process = 0; //Cancel.
						}
						if (bValid)
						{
							extern char g_pAbsPathToConverter[MAX_PATH];
							std::string process_name = g_pAbsPathToConverter;
							replaceAll(process_name, "\\Guru-Converter.exe", "\\ffmpeg.exe");
							HANDLE g_hConvertImportTOOggProcess = NULL;
							DARKSDK BOOL DB_ExecuteFile(HANDLE* phExecuteFileProcess, char* Operation, char* Filename, char* String, char* Path, bool bWaitForTermination);
							char parameters[MAX_PATH];
							char destination[MAX_PATH];
							strcpy(destination, "audiobank\\user\\");
							strcat(destination, import_name);

							//PE: Simple import , no options, so always to .wav
							//PE: use -y -i to always overwrite.
							if (0) //ogg
							{
								strcat(destination, ".ogg");
								GG_GetRealPath(destination, 1);
								strcpy(parameters, "-y -i \"");
								strcat(parameters, import_filename.Get());
								strcat(parameters, "\" -c:a libvorbis -q:a 4 \"");
								strcat(parameters, destination);
								strcat(parameters, "\"");
							}
							else
							{
								strcat(destination, ".wav");
								GG_GetRealPath(destination, 1);
								strcpy(parameters, "-y -i \"");
								strcat(parameters, import_filename.Get());
								strcat(parameters, "\" \"");
								strcat(parameters, destination);
								strcat(parameters, "\"");
							}

							::SetCursor(::LoadCursor(NULL, IDC_WAIT));
							DB_ExecuteFile(&g_hConvertImportTOOggProcess, "hide", (char *)process_name.c_str(), parameters, "", true);
							int iTimeout = 4000;
							int iRunning = 1;
							while (iRunning == 1)
							{
								iRunning = 0;
								DWORD dwStatus;
								if (GetExitCodeProcess(g_hConvertImportTOOggProcess, &dwStatus) == TRUE)
									if (dwStatus == STILL_ACTIVE)
										iRunning = 1;
								Sleep(1);
								if (iTimeout-- <= 0) iRunning = 0; //Timeout 4 sec.
							}
							CloseHandle(g_hConvertImportTOOggProcess);
							::SetCursor(::LoadCursor(NULL, IDC_ARROW));

							//Done close down.
							import_process = 0;
							iLastDisplayLibraryType = -1; //Update search and refresh if any new files found.
							sStartLibrarySearchString = "user";
						}
					}
					if (import_process == 2)
					{
						import_process = 1; //Need a frame for triggermessage
					}
					if (import_process == 0)
					{
						ImGui::SameLine();
						if (ImGui::StyleButton("Import Music And Sound", ImVec2(fButWidth, fFontSize*2.0)))
						{
							//Select File.
							//Select name.
							//Convert and copy to 'audiobank/users'.
							//ffmpeg can be used.
							cStr tOldDir = GetDir();
							char * cFileSelected;
							cFileSelected = (char *)noc_file_dialog_open(NOC_FILE_DIALOG_OPEN, "All\0*.mp3;*.wav;*.ogg\0mp3\0*.mp3\0wav\0*.wav\0ogg\0*.ogg\0", g.mysystem.mapbankAbs_s.Get(), NULL);
							SetDir(tOldDir.Get());
							if (cFileSelected && strlen(cFileSelected) > 0)
							{
								import_filename = cFileSelected;

								//import_name
								cstr importer_getfilenameonly(LPSTR pFileAndPossiblePath);
								cstr tmp = importer_getfilenameonly(import_filename.Get());
								strcpy(import_name, tmp.Get());
								if (tmp.Len() > 4)
								{
									import_name[strlen(import_name) - 4] = 0;
								}
								import_process = 1;
								sprintf(cTriggerMessage, "Importing");
								bTriggerMessage = true;
							}
						}
						if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", "Click to import Music or Sound");
					}
				}
				if (selectedmediafile != NULL)
				{
					ImGui::SameLine();
					if (ImGui::StyleButton("Add Selected", ImVec2(fButWidth, fFontSize*2.0)))
					{
						//Sent selection to imgui ID that have last requested a media file.
						playingiles = NULL;

						//Stop anything playing.
						if (SoundExist(MEDIA_PLAY) == 1)
						{
							StopSound(MEDIA_PLAY);
							DeleteSound(MEDIA_PLAY);
						}
						if (SoundExist(g.temppreviewsoundoffset) == 1)
						{
							StopSound(g.temppreviewsoundoffset);
							DeleteSound(g.temppreviewsoundoffset);
						}

						cFolderItem *pNewFolder = selectedmediafile->pNewFolder;
						cStr path = pNewFolder->m_sFolderFullPath.Get();
						int ipath_remove_len = path.Len();
						if (pNewFolder->m_iEntityOffset > 0)
							ipath_remove_len = pNewFolder->m_iEntityOffset;

						char *final_name = path.Get();
						final_name += ipath_remove_len;
						if (*final_name == '\\')
							final_name++;


						std::string path_for_filename = final_name;
						std::string sSoundName = "";
						if (iDisplayLibraryType == 1)
						{
							//if (strnicmp(path_for_filename.c_str(), "projectbank", 11) != NULL) 
							sSoundName = "audiobank\\";
						}
						sSoundName = sSoundName + path_for_filename.c_str();
						if (path_for_filename.length() == 0)
							sSoundName = sSoundName + selectedmediafile->m_sName.Get();
						else
							sSoundName = sSoundName + "\\" + selectedmediafile->m_sName.Get();
						if (sSoundName != "")
						{
							sSelectedLibrarySting = sSoundName.c_str();
							iSelectedLibraryStingReturnID = iLibraryStingReturnToID;

							// if remote project, copy over to that
							extern bool entity_copytoremoteifnotthere(LPSTR);
							entity_copytoremoteifnotthere((LPSTR)sSoundName.c_str());
						}
						bCheckForClosing = true;
						bImGuiRenderTargetFocus = true; //PE: needed for window overlap check.

						selectedmediafile = NULL;
					}
					if (ImGui::IsItemHovered()) ImGui::SetTooltip("Add Selected");
				}
			}

			//################
			//#### Images ####
			//################

			if (iDisplayLibraryType == 2) //Images
			{
				int buts = 1;
				if (selectedmediafile != NULL) buts = 2;
				fButWidth = vContentSize.x / buts;
				fButWidth -= 10.0f;

				if (ImGui::StyleButton("Import Image", ImVec2(fButWidth, fFontSize*2.0)))
				{
					static char picture_default_folder[MAX_PATH] = "\0";
					if (strlen(picture_default_folder) <= 0)
					{
						//Init. 
						if ((SHGetFolderPathA(NULL, CSIDL_COMMON_PICTURES, NULL, 0, &picture_default_folder[0])) != S_OK)
						{
							//Failed try another.
							if ((SHGetFolderPathA(NULL, CSIDL_MYPICTURES, NULL, 0, &picture_default_folder[0])) != S_OK)
							{
								//Failed try another.
								if ((SHGetFolderPathA(NULL, CSIDL_COMMON_DOCUMENTS, NULL, 0, &picture_default_folder[0])) != S_OK)
								{
									strcpy(picture_default_folder, "c:\\");
								}
							}
						}
					}
					//
					cStr tOldDir = GetDir();
					char * cFileSelected;
					cFileSelected = (char *)noc_file_dialog_open(NOC_FILE_DIALOG_OPEN, "all\0*.*\0png\0*.png\0jpg\0*.jpg\0jpeg\0*.jpeg\0dds\0*.dds\0bmp\0*.bmp\0jpeg\0*.jpeg\0", picture_default_folder, NULL ,true);
					SetDir(tOldDir.Get());
					if (cFileSelected && strlen(cFileSelected) > 0)
					{
						cstr import_filename = cFileSelected;
						strcpy(picture_default_folder, cFileSelected);
						//import_name
						cstr importer_getfilenameonly(LPSTR pFileAndPossiblePath);
						cstr tmp = importer_getfilenameonly(import_filename.Get());

						char destination[MAX_PATH];
						strcpy(destination, "imagebank\\user\\");
						strcat(destination, tmp.Get());
						GG_GetRealPath(destination, 1);
						CopyFileA(import_filename.Get(), destination, false);

						iLastDisplayLibraryType = -1; //Update search and refresh if any new files found.
						sSelectedLibrarySting = "";
						sStartLibrarySearchString = "user";
					}
				}
				if (selectedmediafile != NULL)
				{
					ImGui::SameLine();
					if (ImGui::StyleButton("Add Selected Image", ImVec2(fButWidth, fFontSize*2.0)))
					{
						//Sent selection to imgui ID that have last requested a media file.
						playingiles = NULL;

						cFolderItem *pNewFolder = selectedmediafile->pNewFolder;
						cStr path = pNewFolder->m_sFolderFullPath.Get();
						int ipath_remove_len = path.Len();
						if (pNewFolder->m_iEntityOffset > 0)
							ipath_remove_len = pNewFolder->m_iEntityOffset;

						char *final_name = path.Get();
						final_name += ipath_remove_len;
						if (*final_name == '\\')
							final_name++;

						std::string path_for_filename = final_name;
						std::string sImageName = "";

						sImageName = "imagebank\\";
						sImageName = sImageName + path_for_filename.c_str();
						if (path_for_filename.length() == 0)
							sImageName = sImageName + selectedmediafile->m_sName.Get();
						else
							sImageName = sImageName + "\\" + selectedmediafile->m_sName.Get();
						if (sImageName != "")
						{
							sSelectedLibrarySting = sImageName.c_str();
							iSelectedLibraryStingReturnID = iLibraryStingReturnToID;

							// if remote project, copy over to that
							extern bool entity_copytoremoteifnotthere(LPSTR);
							entity_copytoremoteifnotthere((LPSTR)sImageName.c_str());
						}
						bCheckForClosing = true;
						bImGuiRenderTargetFocus = true; //PE: needed for window overlap check.

						selectedmediafile = NULL;
					}
					if (ImGui::IsItemHovered()) ImGui::SetTooltip("Add Selected Image");
				}
			}

			//###############
			//#### Video ####
			//###############

			if (iDisplayLibraryType == 3) //Video
			{
				int buts = 1;
				if (selectedmediafile != NULL) buts = 2;
				fButWidth = vContentSize.x / buts;
				fButWidth -= 10.0f;


				if (ImGui::StyleButton("Import Video", ImVec2(fButWidth, fFontSize * 2.0)))
				{
					static char video_default_folder[MAX_PATH] = "\0";
					if (strlen(video_default_folder) <= 0)
					{
						//Init. 
						if ((SHGetFolderPathA(NULL, CSIDL_COMMON_VIDEO, NULL, 0, &video_default_folder[0])) != S_OK)
						{
							//Failed try another.
							if ((SHGetFolderPathA(NULL, CSIDL_MYVIDEO, NULL, 0, &video_default_folder[0])) != S_OK)
							{
								//Failed try another.
								if ((SHGetFolderPathA(NULL, CSIDL_COMMON_DOCUMENTS, NULL, 0, &video_default_folder[0])) != S_OK)
								{
									strcpy(video_default_folder, "c:\\");
								}
							}
						}
					}
					//
					cStr tOldDir = GetDir();
					char* cFileSelected;
					//WMV,MP4
					cFileSelected = (char*)noc_file_dialog_open(NOC_FILE_DIALOG_OPEN, "all\0*.*\0MP4\0*.MP4\0WMV\0*.WMV\0", video_default_folder, NULL, true);
					SetDir(tOldDir.Get());
					if (cFileSelected && strlen(cFileSelected) > 0)
					{
						cstr import_filename = cFileSelected;
						strcpy(video_default_folder, cFileSelected);
						//import_name
						cstr importer_getfilenameonly(LPSTR pFileAndPossiblePath);
						cstr tmp = importer_getfilenameonly(import_filename.Get());

						char destination[MAX_PATH];
						strcpy(destination, "videobank\\user\\");
						strcat(destination, tmp.Get());
						GG_GetRealPath(destination, 1);
						CopyFileA(import_filename.Get(), destination, false);
						iLastDisplayLibraryType = -1; //Update search and refresh if any new files found.
						sSelectedLibrarySting = "";
						sStartLibrarySearchString = "user";
					}
				}
				if (selectedmediafile != NULL)
				{
					ImGui::SameLine();
					if (ImGui::StyleButton("Add Selected Video", ImVec2(fButWidth, fFontSize*2.0)))
					{
						//Sent selection to imgui ID that have last requested a media file.
						playingiles = NULL;

						cFolderItem *pNewFolder = selectedmediafile->pNewFolder;
						cStr path = pNewFolder->m_sFolderFullPath.Get();
						int ipath_remove_len = path.Len();
						if (pNewFolder->m_iEntityOffset > 0)
							ipath_remove_len = pNewFolder->m_iEntityOffset;

						char *final_name = path.Get();
						final_name += ipath_remove_len;
						if (*final_name == '\\')
							final_name++;

						std::string path_for_filename = final_name;
						std::string sVideoName = "";
						if (iDisplayLibraryType == 3)
						{
							//if (strnicmp(path_for_filename.c_str(), "projectbank", 11) != NULL) 
							sVideoName = "videobank\\";
						}
						sVideoName = sVideoName + path_for_filename.c_str();
						if (path_for_filename.length() == 0)
							sVideoName = sVideoName + selectedmediafile->m_sName.Get();
						else
							sVideoName = sVideoName + "\\" + selectedmediafile->m_sName.Get();
						if (sVideoName != "")
						{
							sSelectedLibrarySting = sVideoName.c_str();
							iSelectedLibraryStingReturnID = iLibraryStingReturnToID;

							// if remote project, copy over to that
							extern bool entity_copytoremoteifnotthere(LPSTR);
							entity_copytoremoteifnotthere((LPSTR)sVideoName.c_str());
						}
						bCheckForClosing = true;
						bImGuiRenderTargetFocus = true; //PE: needed for window overlap check.

						selectedmediafile = NULL;
					}
					if (ImGui::IsItemHovered()) ImGui::SetTooltip("Add Selected Video");
				}
			}

			//################
			//#### Script ####
			//################
			char pChosenSelectedBehaviorFile[MAX_PATH];
			strcpy (pChosenSelectedBehaviorFile, "");
			if (iDisplayLibraryType == 4) //Script
			{
				// get more
				int buts = 1;
				if (pref.iEnableDeveloperProperties)
				{
					if (selectedmediafile != NULL) buts = 2;
				}
				fButWidth = vContentSize.x / buts;
				fButWidth -= 10.0f;

				// create new
				if (pref.iEnableDeveloperProperties)
				{
					//ImGui::SameLine();
					if (ImGui::StyleButton("Create New Behavior", ImVec2(fButWidth, fFontSize*2.0)))
					{
						// trigger entry of unique behavior name
						library_createbehavior = true;
						strcpy (library_newbehaviorname, "");
					}
					if (ImGui::IsItemHovered()) ImGui::SetTooltip("Create a new blank behavior script, ready for editing with the behavior editor");
					ImGui::SameLine();
				}

				// create a new state with unique name
				if (library_createbehavior == true)
				{
					// Can create new behvaior
					ImGui::SetNextWindowSize(ImVec2(26 * ImGui::GetFontSize(), 8 * ImGui::GetFontSize()), ImGuiCond_Once);
					ImGui::SetNextWindowPosCenter(ImGuiCond_Once);
					cstr sUniqueWinName = cstr("Enter A Name for your New Behavior##BehaviorEditorNew");
					ImGui::Begin(sUniqueWinName.Get(), &library_createbehavior, 0);
					ImGui::Indent(10);
					cstr sUniqueInputName = cstr("##Behavior Name") + cstr(1);
					ImGui::PushItemWidth(-10);
					ImGui::Text("");
					ImGui::Text("Type a name for your new Behavior and press ENTER:");
					if (ImGui::IsRootWindowOrAnyChildFocused() && !ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0)) ImGui::SetKeyboardFocusHere(0);
					if (ImGui::InputText(sUniqueInputName.Get(), library_newbehaviorname, 250, ImGuiInputTextFlags_EnterReturnsTrue))
					{
						// cannot contain spaces
						bool bContainsASpace = false;
						for (int n = 0; n < strlen(library_newbehaviorname); n++)
						{
							if (library_newbehaviorname[n] == ' ')
								bContainsASpace = true;
						}
						if (bContainsASpace == true)
						{
							sprintf(cTriggerMessage, "The behavior name cannot contain spaces");
							bTriggerMessage = true;
						}
						else
						{
							// create a behavior script in the user folder
							char pNewScriptPath[MAX_PATH];
							sprintf(pNewScriptPath, "%s\\Files\\scriptbank\\user\\%s.lua", g.fpscrootdir_s.Get(),library_newbehaviorname);
							GG_GetRealPath(pNewScriptPath, 1);
							if (FileExist(pNewScriptPath) == 1)
							{
								sprintf(cTriggerMessage, "This behavior name already exists");
								bTriggerMessage = true;
							}
							else
							{
								// temp
								char pLine[2048];

								// generate a blank LUA script
								OpenToWrite(1, pNewScriptPath);
								WriteString(1, "-- DESCRIPTION: This is a custom behavior, and can be configured with [customvalue1=0(0,100)], [customvalue2=0(0,100)] and [customvalue3=0(0,100)].");
								sprintf(pLine, "master_interpreter_core = require \"scriptbank\\\\masterinterpreter\""); WriteString(1, pLine);
								
								WriteString(1, "");
								sprintf(pLine, "g_%s = {}", library_newbehaviorname); WriteString(1, pLine);
								sprintf(pLine, "g_%s_behavior = {}", library_newbehaviorname); WriteString(1, pLine);
								sprintf(pLine, "g_%s_behavior_count = 0", library_newbehaviorname); WriteString(1, pLine);

								WriteString(1, "");
								sprintf(pLine, "function %s_init(e)", library_newbehaviorname); WriteString(1, pLine);
								sprintf(pLine, " g_%s[e] = {}", library_newbehaviorname); WriteString(1, pLine);

								sprintf(pLine, " g_%s[e][\"bycfilename\"] = \"scriptbank\\\\user\\\\%s.byc\"", library_newbehaviorname, library_newbehaviorname); WriteString(1, pLine);
								sprintf(pLine, " g_%s_behavior_count = master_interpreter_core.masterinterpreter_load (g_%s[e], g_%s_behavior )", library_newbehaviorname, library_newbehaviorname, library_newbehaviorname); WriteString(1, pLine);
								sprintf(pLine, " %s_properties(e,0,0,0)", library_newbehaviorname); WriteString(1, pLine);
								sprintf(pLine, "end"); WriteString(1, pLine);

								WriteString(1, "");
								sprintf(pLine, "function %s_properties(e, customvalue1, customvalue2, customvalue3)", library_newbehaviorname); WriteString(1, pLine);
								sprintf(pLine, " g_%s[e]['customvalue1'] = customvalue1", library_newbehaviorname); WriteString(1, pLine);
								sprintf(pLine, " g_%s[e]['customvalue2'] = customvalue2", library_newbehaviorname); WriteString(1, pLine);
								sprintf(pLine, " g_%s[e]['customvalue3'] = customvalue3", library_newbehaviorname); WriteString(1, pLine);
								sprintf(pLine, " master_interpreter_core.masterinterpreter_restart (g_%s[e], g_Entity[e])", library_newbehaviorname); WriteString(1, pLine);
								sprintf(pLine, "end"); WriteString(1, pLine);

								WriteString(1, "");
								sprintf(pLine, "function %s_main(e)", library_newbehaviorname); WriteString(1, pLine);
								sprintf(pLine, " if g_%s[e] ~= nil and g_%s_behavior_count > 0 then", library_newbehaviorname, library_newbehaviorname); WriteString(1, pLine);
								sprintf(pLine, "  g_%s_behavior_count = master_interpreter_core.masterinterpreter (g_%s_behavior, g_%s_behavior_count, e, g_%s[e], g_Entity[e])", library_newbehaviorname, library_newbehaviorname, library_newbehaviorname, library_newbehaviorname); WriteString(1, pLine);
								sprintf(pLine, " end"); WriteString(1, pLine);
								sprintf(pLine, "end"); WriteString(1, pLine);
								WriteString(1, "");
								CloseFile(1);

								// create a blank thumbnail for script
								char pBlankFile[MAX_PATH];
								sprintf(pBlankFile, "scriptbank\\user\\blank_icon.jpg");
								char pThumbFile[MAX_PATH];
								sprintf(pThumbFile, "%s\\Files\\scriptbank\\user\\%s.jpg", g.fpscrootdir_s.Get(), library_newbehaviorname);
								GG_GetRealPath(pThumbFile, 1);
								CopyFileA(pBlankFile, pThumbFile, TRUE);

								// when created, auto select this for the object
								char pRelativePathToScript[MAX_PATH];
								sprintf(pRelativePathToScript, "user\\%s.lua", library_newbehaviorname);
								strcpy (pChosenSelectedBehaviorFile, pRelativePathToScript);
							}
						}

						// finished here
						library_createbehavior = false;
						strcpy(library_newbehaviorname, "");
					}
					ImGui::PopItemWidth();
					ImGui::Indent(-10);
					bImGuiGotFocus = true;
					ImGui::End();
				}

				// add selected
				if (selectedmediafile != NULL)
				{
					if (ImGui::StyleButton("Add Selected Behavior", ImVec2(fButWidth, fFontSize*2.0)))
					{
						cFolderItem *pNewFolder = selectedmediafile->pNewFolder;
						cStr path = pNewFolder->m_sFolderFullPath.Get();
						int ipath_remove_len = path.Len();
						if (pNewFolder->m_iEntityOffset > 0)
							ipath_remove_len = pNewFolder->m_iEntityOffset;

						char *final_name = path.Get();
						final_name += ipath_remove_len;
						if (*final_name == '\\')
							final_name++;

						std::string path_for_filename = final_name;

						std::string sScriptName = "";
						sScriptName = sScriptName + path_for_filename.c_str();
						if (path_for_filename.length() == 0)
							sScriptName = sScriptName + selectedmediafile->m_sName.Get();
						else
							sScriptName = sScriptName + "\\" + selectedmediafile->m_sName.Get();

						// trigger selected
						strcpy (pChosenSelectedBehaviorFile, sScriptName.c_str());

						// if remote project, copy over to that
						if (sScriptName != "")
						{
							extern bool entity_copytoremoteifnotthere(LPSTR);
							std::string sScriptBankScriptName = "scriptbank\\" + sScriptName;
							entity_copytoremoteifnotthere((LPSTR)sScriptBankScriptName.c_str());
						}
					}
					if (ImGui::IsItemHovered()) ImGui::SetTooltip("Add Selected Behavior");
				}
			}

			// if cxeated/selected behavior
			if (strlen(pChosenSelectedBehaviorFile)>0)
			{
				// created or added
				//Sent selection to imgui ID that have last requested a media file.
				playingiles = NULL;
				sSelectedLibrarySting = pChosenSelectedBehaviorFile;
				iSelectedLibraryStingReturnID = iLibraryStingReturnToID;
				bCheckForClosing = true;
				bImGuiRenderTargetFocus = true; //PE: needed for window overlap check.
				selectedmediafile = NULL;
			}

			//###################
			//#### Particles ####
			//###################

			if (iDisplayLibraryType == 5)
			{
				int buts = 1;
				if (selectedmediafile != NULL) buts = 2;
				fButWidth = vContentSize.x / buts;
				fButWidth -= 10.0f;
				LPSTR pParticleEditorTitle = "Particle Editor";
				LPSTR pParticleEditorTooltip = "Update GameGuru MAX to the latest version to get the Particle Editor Tool";
				extern bool g_bParticleEditorPresent;
				if (g_bParticleEditorPresent == true)
				{
					pParticleEditorTitle = "Create New Particles";
					pParticleEditorTooltip = "Create More Particles using the Particle Editor";
				}
				if (ImGui::StyleButton(pParticleEditorTitle, ImVec2(fButWidth, fFontSize*2.0)))
				{
					if (g_bParticleEditorPresent == true)
					{
						extern void launchOrShowParticleEditor(void);
						launchOrShowParticleEditor();
					}
				}
				if (ImGui::IsItemHovered()) ImGui::SetTooltip(pParticleEditorTooltip);

				if (selectedmediafile != NULL)
				{
					ImGui::SameLine();
					if (ImGui::StyleButton("Add Selected Particle", ImVec2(fButWidth, fFontSize*2.0)))
					{
						//Sent selection to imgui ID that have last requested a media file.
						playingiles = NULL;

						cFolderItem *pNewFolder = selectedmediafile->pNewFolder;
						cStr path = pNewFolder->m_sFolderFullPath.Get();
						int ipath_remove_len = path.Len();
						if (pNewFolder->m_iEntityOffset > 0)
							ipath_remove_len = pNewFolder->m_iEntityOffset;

						char *final_name = path.Get();
						final_name += ipath_remove_len;
						if (*final_name == '\\')
							final_name++;

						std::string path_for_filename = final_name;
						std::string sParticlesName = "";

						sParticlesName = "particlesbank\\";
						sParticlesName = sParticlesName + path_for_filename.c_str();
						if (path_for_filename.length() == 0)
							sParticlesName = sParticlesName + selectedmediafile->m_sName.Get();
						else
							sParticlesName = sParticlesName + "\\" + selectedmediafile->m_sName.Get();
						if (sParticlesName != "")
						{
							sSelectedLibrarySting = sParticlesName.c_str();
							iSelectedLibraryStingReturnID = iLibraryStingReturnToID;

							// if remote project, copy over to that
							extern bool entity_copytoremoteifnotthere(LPSTR);
							entity_copytoremoteifnotthere((LPSTR)sParticlesName.c_str());
						}
						bCheckForClosing = true;
						bImGuiRenderTargetFocus = true; //PE: needed for window overlap check.

						selectedmediafile = NULL;
					}
					if (ImGui::IsItemHovered()) ImGui::SetTooltip("Add Selected Particles");
				}	
			}
		}
		else
		{
			if (iDisplayLibrarySubType == 0)
			{
				#ifdef GGMAXEDU
				if (ImGui::StyleButton("Building Editor", ImVec2(fButWidth, fFontSize * 2.0)))
				{
					DeleteWaypointsAddedToCurrentCursor();
					CloseDownEditorProperties();
					extern void launchOrShowBuildingEditor(void);
					launchOrShowBuildingEditor();
				}
				#else
				if (ImGui::StyleButton("Get More Objects", ImVec2(fButWidth, fFontSize * 2.0)))
				{
					DeleteWaypointsAddedToCurrentCursor();
					CloseDownEditorProperties();
					bMarketplace_Window = true;
				}
				#endif
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("Get More Objects from the Marketplace");

				ImGui::SameLine();
				if (ImGui::StyleButton("Import 3D Model", ImVec2(fButWidth, fFontSize*2.0)))
				{
					// copied from marketplace import
					DeleteWaypointsAddedToCurrentCursor();
					CloseDownEditorProperties();
					CloseAllOpenTools();
					iLaunchAfterSync = 8; //Import model
					iSkibFramesBeforeLaunch = 5;
					bMarketplace_Window = false;
					bTriggerCloseEntityWindow = true;
					bCheckForClosingForce = true; //Force window to close.
				}
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("Import 3D Model in the OBJ, FBX, GLTF or DBO format");

				ImGui::SameLine();
				if (ImGui::StyleButton("Character Creator", ImVec2(fButWidth, fFontSize*2.0)))
				{
					DeleteWaypointsAddedToCurrentCursor();
					//CheckTooltipObjectDelete();
					CloseDownEditorProperties();
					CloseAllOpenTools();

					//g_bCharacterCreatorPlusActivated = true;
					iLaunchAfterSync = 82; //Start Character Creator
					iSkibFramesBeforeLaunch = 2;
					strcpy(cTriggerMessage, "Loading Character Creator");
					bTriggerMessage = true;

					bCheckForClosing = true;
					bEnableWeather = false;
				}

				if (lf_multi_selections_count > 0)
				{
					ImGui::SameLine();
					std::string insert_text;
					//Display insert button.
					insert_text = " Add ";
					insert_text += std::to_string(lf_multi_selections_count);
					insert_text += " Objects to Level ";
					if (ImGui::StyleButton(insert_text.c_str(), ImVec2(fButWidth, fFontSize*2.0)))
					{
						//Insert all selected items.
						bAddNewSelectionToGame = true;
						iAddSelectionStep = 0;
					}
				}
			}
		}
		ImGui::SetWindowFontScale(1.0);

		ImGui::Indent(-10);
#ifdef DYNAMICLOADUNLOAD
		max_load_persync = 10; // 15 to slow try 10
#endif
		lf_multi_selections_count = multi_selections_count; //Use last frames count.

		if (!bImagesStillInImGuiQueue && !bLargePreview && bAddNewSelectionToGame)
		{
			extern cstr g_sTempGroupForThumbnail;
			if (g_sTempGroupForThumbnail.Len() > 0 )
			{
				//hmm
				//extern int GetGroupIndexFromName (cstr sLookFor);
				//current_selected_group = GetGroupIndexFromName(g_sTempGroupForThumbnail);
				g_sTempGroupForThumbnail = "";
			}

			DeleteWaypointsAddedToCurrentCursor();
			CloseDownEditorProperties();

			//Remove any selections.
			t.inputsys.constructselection = 0;
			if (t.gridentityobj > 0)
			{
				DeleteObject(t.gridentityobj);
				t.gridentityobj = 0;
			}

			//PE: Make sure we free all not used textures-objects before adding new objects.
			FreeTempImageList();

			t.refreshgrideditcursor = 1;
			t.gridentity = 0;
			t.gridentityposoffground = 0;
			t.gridentityusingsoftauto = 0;
			editor_refresheditmarkers();

			cFolderItem *pSearchFolder = &MainEntityList;
			pSearchFolder = pSearchFolder->m_pNext;
			cStr path_remove;
			int ipath_remove_len;
			if (pSearchFolder) {
				path_remove = pSearchFolder->m_sFolderFullPath.Get();
				ipath_remove_len = path_remove.Len();
			}

			bool bOneFound = false;

			iAddSelectionStep++;
			while(pSearchFolder)
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
				if (!bDoubleEntityBank && path.Right(11) == "\\entitybank") {
					ipath_remove_len = path.Len();
				}
				else
				{
					if (pSearchFolder->m_pFirstFile) {

						cFolderItem::sFolderFiles * searchfiles = pSearchFolder->m_pFirstFile->m_pNext;
						while (searchfiles)
						{
							if (searchfiles->iFlags == 1)
							{
								bOneFound = true;
								if (bWaypointDrawmode || bWaypoint_Window) { bWaypointDrawmode = false; bWaypoint_Window = false; }
								if (bImporter_Window) { importer_quit(); bImporter_Window = false; }
								if (g_bCharacterCreatorPlusActivated) g_bCharacterCreatorPlusActivated = false;

								//Insert.
								cStr path = pSearchFolder->m_sFolderFullPath.Get();
								char *final_name = path.Get();
								final_name += ipath_remove_len;
								if (*final_name == '\\')
									final_name++;
								std::string path_for_filename = final_name;

								std::string sFpeName = path_for_filename.c_str();
								sFpeName = sFpeName + "\\" + searchfiles->m_sName.Get();
								iLastEntityOnCursor = 0;

								char adding[256];
								strcpy(adding, searchfiles->m_sName.Get());
								if (strlen(adding) > 4)
									adding[strlen(adding) - 4] = 0;
								
								iMessageTimer = 0; //Restart fading.
								sprintf(cTriggerMessage, "Adding \"%s\" to Level", adding);
								bTriggerMessage = true;

								t.addentityfile_s = sFpeName.c_str();
								if (t.addentityfile_s != "")
								{
									entity_adduniqueentity(false);
									t.tasset = t.entid;
									if (t.talreadyloaded == 0)
									{
										editor_filllibrary();
									}
								}

								searchfiles->iFlags = 0;
							}
							if (bOneFound) break;
							searchfiles = searchfiles->m_pNext;
						}
					}
				}
				if (bOneFound) break;
				pSearchFolder = pSearchFolder->m_pNext;
			}
			if (iAddSelectionStep > 0 && !bOneFound)
			{
				bCheckForClosing = true;
				bAddNewSelectionToGame = false;
				iAddSelectionStep = 0;
			}
		}

		bool bAreWeOverLapping = false;
		if (!bIsWeDocked) 
		{
			//If we are over the rendertarget hide window.
			float itmptopmousex = ImGui::GetWindowPos().x;
			float itmptopmousey = ImGui::GetWindowPos().y;
			int iSecureZone = 4;

			if (bImGuiRenderTargetFocus && itmptopmousex >= (renderTargetAreaPos.x + iSecureZone) && itmptopmousey >= (renderTargetAreaPos.y + iSecureZone) &&
				itmptopmousex <= renderTargetAreaPos.x + (renderTargetAreaSize.x - iSecureZone) && itmptopmousey <= renderTargetAreaPos.y + (renderTargetAreaSize.y - ImGuiStatusBar_Size - iSecureZone))
			{
				bAreWeOverLapping = true;
			}
			float itmpmousex = ImGui::GetWindowPos().x + ImGui::GetWindowSize().x;
			float itmpmousey = ImGui::GetWindowPos().y + ImGui::GetWindowSize().y;
			if (bExternal_Entities_Window && bImGuiRenderTargetFocus && itmpmousex >= (renderTargetAreaPos.x + iSecureZone) && itmpmousey >= (renderTargetAreaPos.y + iSecureZone) &&
				itmptopmousex <= renderTargetAreaPos.x + (renderTargetAreaSize.x - iSecureZone) && itmptopmousey <= renderTargetAreaPos.y + (renderTargetAreaSize.y - ImGuiStatusBar_Size - iSecureZone))
			{
				bAreWeOverLapping = true;
			}
			itmpmousex = ImGui::GetWindowPos().x + ImGui::GetWindowSize().x;
			itmpmousey = ImGui::GetWindowPos().y;
			if (bExternal_Entities_Window && bImGuiRenderTargetFocus && itmpmousex >= (renderTargetAreaPos.x + iSecureZone) && itmpmousey >= (renderTargetAreaPos.y + iSecureZone) &&
				itmpmousex <= renderTargetAreaPos.x + (renderTargetAreaSize.x - iSecureZone) && itmpmousey <= renderTargetAreaPos.y + (renderTargetAreaSize.y - ImGuiStatusBar_Size - iSecureZone))
			{
				bAreWeOverLapping = true;
			}
			itmpmousex = ImGui::GetWindowPos().x;
			itmpmousey = ImGui::GetWindowPos().y + ImGui::GetWindowSize().y;
			if (bExternal_Entities_Window && bImGuiRenderTargetFocus && itmpmousex >= (renderTargetAreaPos.x + iSecureZone) && itmpmousey >= (renderTargetAreaPos.y + iSecureZone) &&
				itmpmousex <= renderTargetAreaPos.x + (renderTargetAreaSize.x - iSecureZone) && itmpmousey <= renderTargetAreaPos.y + (renderTargetAreaSize.y - ImGuiStatusBar_Size - iSecureZone))
			{
				bAreWeOverLapping = true;
			}
		}
		if (!bIsWeDocked && bCheckForClosing) {
			if (bAreWeOverLapping || bCheckForClosingForce)
			{
				bExternal_Entities_Window = false;
			}
		}

		//Remove window on right click if we are overlapping.
		if (bAreWeOverLapping && t.inputsys.mclick == 2)
			bExternal_Entities_Window = false;

		bCheckForClosingForce = false;
		bCheckForClosing = false;

		CheckMinimumDockSpaceSize(250.0f);

		//Render titlebar centered.
		cstr title = "Object Library - What do you want to add to your level?";
		cstr titlesmall = "Object Library";
		if (iDisplayLibraryType == 0 && iDisplayLibrarySubType == 1)
		{
			title = "Animation Library - What do you want to add to your object?";
			titlesmall = "Animation Library";
		}
		if (iDisplayLibraryType > 0)
		{
			if (iDisplayLibraryType == 1)
			{
				title = "Music and Sound Library - What do you want to add to your level?";
				titlesmall = "Music and Sound Library";
			}
			if (iDisplayLibraryType == 2)
			{
				title = "Image Library - What do you want to add to your level?";
				titlesmall = "Image Library";
			}
			if (iDisplayLibraryType == 3)
			{
				title = "Video Library - What do you want to add to your level?";
				titlesmall = "Video Library";
			}
			if (iDisplayLibraryType == 4)
			{
				title = "Behavior Library - What do you want to add to your level?";
				titlesmall = "Behavior Library";
			}
			if (iDisplayLibraryType == 5)
			{
				title = "Particle Library - What do you want to add to your level?";
				titlesmall = "Particle Library";

				// very special case to monitor the legacy writables area where
				// the particle editor tool exports its MAX particles, and if there is
				// a file count difference, force the new file to copy over to the
				// remote project area, and also force a refresh of the particle library
				if (strlen(Storyboard.customprojectfolder) > 0)
				{
					static int iParticleLastKnownFileCount = 0;
					static std::vector<cStr> vParticleLastKnownFiles;
					static unsigned long iParticleCheckerTimer = 0;
					if (iParticleCheckerTimer < timeGetTime())
					{
						// regular checking
						cstr pOldDir = GetDir();
						iParticleCheckerTimer = timeGetTime() + 1000;

						// check the particle editor writables area
						char pParticleWriteAreaPath[MAX_PATH];
						sprintf(pParticleWriteAreaPath, "%s\\Files\\particlesbank\\user", g.fpscrootdir_s.Get());
						GG_SetWritablesToRoot(true);
						GG_GetRealPath(pParticleWriteAreaPath, 0);
						GG_SetWritablesToRoot(false);
						SetDir(pParticleWriteAreaPath);

						// count latest particle files
						int iParticleEditorFileCount = 0;
						ChecklistForFiles();
						for (int f = 1; f <= ChecklistQuantity(); f++)
						{
							cstr tfile_s = ChecklistString(f);
							LPSTR pFilename = tfile_s.Get();
							if (tfile_s != "." && tfile_s != "..")
							{
								if (strnicmp(pFilename + strlen(pFilename) - 4, ".arx", 4) == NULL || strnicmp(pFilename + strlen(pFilename) - 4, ".png", 4) == NULL)
								{
									iParticleEditorFileCount++;
								}
							}
						}

						// if different, copy over and refresh
						if (iParticleEditorFileCount != iParticleLastKnownFileCount)
						{
							std::vector<cStr> vParticleThisKnownFiles;
							vParticleThisKnownFiles.clear();
							for (int f = 1; f <= ChecklistQuantity(); f++)
							{
								cstr tfile_s = ChecklistString(f);
								LPSTR pFilename = tfile_s.Get();
								if (tfile_s != "." && tfile_s != "..")
								{
									if (strnicmp(pFilename + strlen(pFilename) - 4, ".arx", 4) == NULL || strnicmp(pFilename + strlen(pFilename) - 4, ".png", 4) == NULL)
									{
										bool bDidItAlreadyExist = false;
										for (int i = 0; i < vParticleLastKnownFiles.size(); i++)
										{
											if (vParticleLastKnownFiles[i] == tfile_s)
											{
												bDidItAlreadyExist = true;
												break;
											}
										}
										if (bDidItAlreadyExist == false)
										{
											// copy over to remote project
											// only if additions since first checked
											//if (iParticleLastKnownFileCount > 0) may have created it when not in remote project, must copy them over!
											//{
											// ensure user folder exists
											char pUserFolder[MAX_PATH];
											sprintf(pUserFolder, "%s\\Files\\particlesbank\\user", g.fpscrootdir_s.Get());
											GG_GetRealPath(pUserFolder, 1);
												
											char pRemotePath[MAX_PATH];
											sprintf(pRemotePath, "%s\\Files\\particlesbank\\user\\%s", g.fpscrootdir_s.Get(), pFilename);
											GG_GetRealPath(pRemotePath, 1);
											if (FileExist(pRemotePath) == 0)
											{
												CopyFileA(pFilename, pRemotePath, TRUE);
											}
											//}

											// force a refresh so user can see their particle right away!
											extern int g_iRefreshLibraryFolders;
											g_iRefreshLibraryFolders = 1;
										}

										// record for next time when current list is last list next time
										vParticleThisKnownFiles.push_back(tfile_s);
									}
								}
							}

							// when complete, copy this list to last known
							vParticleLastKnownFiles.clear();
							vParticleLastKnownFiles = vParticleThisKnownFiles;
							iParticleLastKnownFileCount = iParticleEditorFileCount;
						}

						// restore current folder now file ops finished
						SetDir(pOldDir.Get());
					}
				}
			}
		}
		float fTextSize = ImGui::CalcTextSize(title.Get()).x;
		if (ImGui::GetWindowSize().x < fTextSize)
		{
			title = titlesmall;
			fTextSize = ImGui::CalcTextSize(title.Get()).x;
			if (ImGui::GetWindowSize().x < fTextSize)
			{
				title = "";
				fTextSize = 0.0f;
			}
		}
		float xcenter = (ImGui::GetWindowSize().x*0.5) - (fTextSize*0.5);
		ImVec2 titlebar_pos = ImGui::GetWindowPos() + ImVec2(xcenter, 4);
		ImGuiWindow* window = ImGui::GetCurrentWindow();

		if (imgui_AddMinMaxButton(0, false))
		{
			//Record where to scroll to after a window resize.
			if (firstvisiblefile)
			{
				scrolltofile = firstvisiblefile;
				bScrollInNextFrame = true;
			}
		}

		//ImGui::Columns(1);
		ImGui::EndColumns();

		ImGui::End();

		//Render title bar after End. end fill titlebar.
		ImGuiContext& g = *GImGui;
		window->DrawList->AddText(g.Font, g.FontSize, titlebar_pos, ImGui::GetColorU32(ImGuiCol_Text), title.Get());

	}
	else 
	{
		//Window closed.
		FreeTempImageList();
		if (iVideoThumbID > 0 && AnimationExist(iVideoThumbID))
		{
			iStopVideoInNextFrame = iVideoThumbID;
			iVideoThumbID = 0;
		}
		// free any temp groups created for Smart Object previews
		extern cstr g_sTempGroupForThumbnail;
		if(g_sTempGroupForThumbnail.Len()>0 )
		{
			int store_current_selected_group = current_selected_group;
			extern int GetGroupIndexFromName (cstr sLookFor);
			current_selected_group = GetGroupIndexFromName(g_sTempGroupForThumbnail);
			UnGroupSelected(true);
			gridedit_deleteentityrubberbandfrommap();
			gridedit_clearentityrubberbandlist();
			g_sTempGroupForThumbnail = "";
			if (store_current_selected_group != -1 )
				g.entityrubberbandlist = vEntityGroupList[store_current_selected_group];
		}
	}

	static bool bExternal_Entities_Window_Last = false;
	if (bExternal_Entities_Window != bExternal_Entities_Window_Last)
	{
		//PE: Rick was so fast he could click one object and start a rotate on another thumb in the same loop, this fix it :)
		bExternal_Entities_Window_Last = bExternal_Entities_Window;
		if (!bExternal_Entities_Window)
		{
			//PE: If just closed, disable any thumb grab.
			bLoopBackBuffer = false;
			BackBufferImageID = 0;
		}
	}

	if (sGotoPreviewWithFile != "")
	{
		if (!bLargePreview && bCheckGotoPreview)
		{
			//We failed to find the char, cancel request.
			sGotoPreviewWithFile = "";
		}
	}
}


