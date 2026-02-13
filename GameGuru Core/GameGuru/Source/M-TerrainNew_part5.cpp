#define NOMINIMAP
#define RANDOMSKYBOX
extern float fWickedMaxCenterTest;
bool bTriggerTerrainSaveAsWindow = false;
bool bFirstTimeVeg = true;
#define USEFULLVIEWPORT
#define DIGAHOLE

static inline ImVec2 ImGuiRotation(const ImVec2& v, float cos_a, float sin_a) { return ImVec2(v.x * cos_a - v.y * sin_a, v.x * sin_a + v.y * cos_a); }

void procedural_new_level(void)
{
	bool bUseFullScreen = true;
	bool bUseNoTitleBar = true;
	bool bUseModal = false;
	float fMaxCameraY = 210000.0f;// 800000.0f PE: To hide the big ugly shadow max here.

	static int iLargePreviewImageID = 0;
	static float oldFogNear, oldFogFar, oldCameraFAR_f, oldfShadowFarPlane;
	static int oldSkyIndex;
	static bool oldbFXAAEnabled , oldbDisableSkybox;
	static bool bIs2DViewHovered = false;
	static float fOldCamx = 0, fOldCamy = 0, fOldCamz = 0, fOldCamAx = 0, fOldCamAy = 0, fOldCamAz = 0, oldSunAngleX, oldSunAngleY, oldSunAngleZ;
	static float oldFogR_f, oldFogG_f, oldFogB_f, oldFogA_f, oldZenithRed_f, oldZenithGreen_f, oldZenithBlue_f, oldSkyCloudHeight;
	static int iCamMode,oldflags2;
	static int movecameratotarget = 0;


	//For squere look (non fullscreen) , 830x800 so we are same size as terrain rect.
	int preview_size_x = 830;
	int preview_size_y = 800;

	if (bUseFullScreen)
	{
		//PE: Use full available area.
		preview_size_x = ImGui::GetMainViewport()->Size.x - 300.0;

		//PE: without the lower buttons.
		if (!bUseNoTitleBar)
			preview_size_y = ImGui::GetMainViewport()->Size.y - 20.0;
		else
			preview_size_y = ImGui::GetMainViewport()->Size.y - 10.0;
	}

	//###################################
	//#### Procedural Level Preview. ####
	//###################################

	extern bool bDigAHoleToHWND;
	extern D3D11_RECT rD3D11DigAHole;
	static bool bNeedReloadTextures = true;

	if (bProceduralLevel)
	{
		if (bNeedReloadTextures)
		{
			g_iDeferTextureUpdateToNow = 1;
			t.visuals.customTexturesFolder = "";
			bNeedReloadTextures = false;
			for (int i = 0; i < 32; i++)
			{
				// Default to all generic material sounds
				g_iCustomTerrainMatSounds[i] = 10;
			}
			// Trigger update of material sounds
			extern bool g_bMapMatIDToMatIndexAvailable;
			g_bMapMatIDToMatIndexAvailable = false;
		}
		ImGuiIO& io = ImGui::GetIO();

		bImGuiGotFocus = true;

		static int TriggerCloseAllAfterSaveLevel = 0;

		if (TriggerCloseAllAfterSaveLevel > 0)
		{
			TriggerCloseAllAfterSaveLevel--;
			if (TriggerCloseAllAfterSaveLevel == 0)
			{
				bProceduralLevel = false;
				bNeedReloadTextures = true;
				extern bool bStoryboardWindow;
				bStoryboardWindow = false;
			}
		}
		
		int save_level_as(void);
		int iRet = save_level_as();
		if (iRet == 2)
		{
			//New level saved, close down and go to editor.
			TriggerCloseAllAfterSaveLevel = 5;
		}
		if (iRet == 1)
		{
			//Cancel save as. Restore settings and continue.
			ggterrain_global_render_params2.flags2 |= GGTERRAIN_SHADER_FLAG2_SHOW_MAP_SIZE;
			ggterrain_global_render_params2.flags2 |= GGTERRAIN_SHADER_FLAG2_SHOW_MINI_MAP;
			ggterrain_global_render_params2.flags2 &= ~GGTERRAIN_SHADER_FLAG2_SHOW_MAP_SIZE_3D;
			ggterrain_global_render_params2.flags2 |= GGTERRAIN_SHADER_FLAG2_USE_FOG;
			#ifdef NOMINIMAP
			ggterrain_global_render_params2.flags2 &= ~GGTERRAIN_SHADER_FLAG2_SHOW_MINI_MAP;
			#endif
		}

		int i3DViewHeight = 1181; //30 meters. was(800)
		static bool bTriggerStableY = false;
		static bool bUseSphere = false;
		//PE: Need to check if backbuffer size changed.
		if (!bPopModalOpenProcedural)
		{
			//Open window and setup defaults.
			if(bUseModal) ImGui::OpenPopup("##ProceduralPreview");

			if (!bUseFullScreen)
			{
				ImGui::SetNextWindowSize(ImVec2(1196.0f, 888));
			}

			//Delete backbuffer and create a new.
			if (BitmapExist(99))
			{
				DeleteBitmapEx(99);
			}

			extern bool BackBufferIsGroup;
			BackBufferIsGroup = false;
			BackBufferEntityID = 0;
			BackBufferObjectID = 0;
			BackBufferImageID = g.importermenuimageoffset + 50;
			iLargePreviewImageID = BackBufferImageID;
			#ifdef USEFULLVIEWPORT
			BackBufferSizeX = ImGui::GetMainViewport()->Size.x; //preview_size_x;
			BackBufferSizeY = ImGui::GetMainViewport()->Size.y; //preview_size_y;
			#else
			BackBufferSizeX = preview_size_x;
			BackBufferSizeY = preview_size_y;
			#endif
			BackBufferZoom = 1.0f;
			BackBufferCamLeft = 0.0f;
			BackBufferCamUp = 0.0f;
			bRotateBackBuffer = false;
			bBackBufferAnimated = false;
			bLoopBackBuffer = false;
			BackBufferSaveCacheName = ""; //No saving on tooltip images
			//Dont fit snapshot to rubber band.
			fLastRubberBandX1 = fLastRubberBandX2 = fLastRubberBandY1 = fLastRubberBandY2 = 0.0f;

			//Control camera snap shot.
			#ifdef DIGAHOLE
			bSnapShotModeUseCamera = false;
			#else
			bSnapShotModeUseCamera = true;
			#endif

			if (!bPopModalOpenProceduralCameraMode)
			{
				//PE: Now start up high looking at the editable area.
				fSnapShotModeCameraX = 73800; //New distance.
				fSnapShotModeCameraZ = -74000;
				fSnapShotModeCameraY = 51600;
				fSnapShotModeCameraAngZ = 0.0f;
				fSnapShotModeCameraAngY = -37;
				fSnapShotModeCameraAngX = 25; //PE: New 3D angle, was 34;
				bTriggerStableY = true; //PE: Make sure after terrain generate that the Y is correct.
			}
			else
			{
				//Take cam directly. controlled by load level.
				fSnapShotModeCameraX = CameraPositionX();
				fSnapShotModeCameraY = CameraPositionY();
				fSnapShotModeCameraZ = CameraPositionZ();
				fSnapShotModeCameraAngX = CameraAngleX();
				fSnapShotModeCameraAngY = CameraAngleY();
				fSnapShotModeCameraAngZ = CameraAngleZ();
			}
			oldFogNear = t.visuals.FogNearest_f;
			oldFogFar = t.visuals.FogDistance_f;
			oldSkyIndex = t.visuals.skyindex;
			oldbDisableSkybox = t.visuals.bDisableSkybox;
			oldCameraFAR_f = t.visuals.CameraFAR_f;
			oldfShadowFarPlane = t.visuals.fShadowFarPlane;
			oldbFXAAEnabled = t.visuals.bFXAAEnabled;

			oldSunAngleX = t.visuals.SunAngleX;
			oldSunAngleY = t.visuals.SunAngleY;
			oldSunAngleZ = t.visuals.SunAngleZ;

			oldZenithRed_f = t.visuals.ZenithRed_f;
			oldZenithGreen_f = t.visuals.ZenithGreen_f;
			oldZenithBlue_f = t.visuals.ZenithBlue_f;

			oldFogR_f = t.visuals.FogR_f;
			oldFogG_f = t.visuals.FogG_f;
			oldFogB_f = t.visuals.FogB_f;
			oldFogA_f = t.visuals.FogA_f;

			oldSkyCloudHeight = t.visuals.SkyCloudHeight;

			//PE: Dont change visuals in bPopModalOpenProceduralCameraMode
			if (!bPopModalOpenProceduralCameraMode)
			{
				t.visuals.FogA_f = 0.30; //0.4; //PE: Just a little fog. so you still can see terrain when way up (2D view).
				t.visuals.FogNearest_f = 120000; //50000;
				t.visuals.FogDistance_f = 500000; //700000;
				t.visuals.skyindex = 0; //Disable clouds. just use static skybox settings (new sky not really activated).
				#ifdef DIGAHOLE
				t.visuals.bDisableSkybox = false; //PE: Test dynamic sky.
				#else
				t.visuals.bDisableSkybox = true;
				t.visuals.bFXAAEnabled = false; //FXAA ruin custom backbuffer ?
				#endif
				t.visuals.CameraFAR_f = fMaxCameraY + 100000.0f;
				fWickedMaxCenterTest = 320000.0f; //PE: Hide ugly shadow.
				Wicked_Update_Visuals((void *)&t.visuals);
			}
			else
			{
				#ifdef DIGAHOLE
				//PE: No changes when we have a hole to hwnd :)
				#else
				//PE: Clouds look strange in take screenshot mode ? must disable for now!
				t.visuals.skyindex = 0; //Disable clouds. just use static skybox settings (new sky not really activated).
				t.visuals.bDisableSkybox = true;
				t.visuals.bFXAAEnabled = false; //FXAA ruin custom backbuffer ?
				Wicked_Update_Visuals((void *)&t.visuals);
				#endif
			}
			WickedCall_DisplayCubes(false); //Hide cubes.
			editor_toggle_element_vis(false);

			fOldCamAx = CameraAngleX();
			fOldCamAy = CameraAngleY();
			fOldCamAz = CameraAngleZ();
			fOldCamx = GetCameraPosition().x;
			fOldCamy = GetCameraPosition().y;
			fOldCamz = GetCameraPosition().z;
			iCamMode = t.editorfreeflight.mode;

			t.editorfreeflight.mode = 1;

			oldflags2 = ggterrain_global_render_params2.flags2;

			if (!bPopModalOpenProceduralCameraMode)
			{
				ggterrain_global_render_params2.flags2 |= GGTERRAIN_SHADER_FLAG2_SHOW_MAP_SIZE;
				ggterrain_global_render_params2.flags2 |= GGTERRAIN_SHADER_FLAG2_SHOW_MINI_MAP;
				ggterrain_global_render_params2.flags2 &= ~GGTERRAIN_SHADER_FLAG2_SHOW_MAP_SIZE_3D;
				ggterrain_global_render_params2.flags2 |= GGTERRAIN_SHADER_FLAG2_USE_FOG;
				#ifdef NOMINIMAP
				ggterrain_global_render_params2.flags2 &= ~GGTERRAIN_SHADER_FLAG2_SHOW_MINI_MAP;
				#endif
			}
			else
			{
				//PE: No minimap ... in cam mode.
				ggterrain_global_render_params2.flags2 &= ~GGTERRAIN_SHADER_FLAG2_SHOW_MAP_SIZE;
				ggterrain_global_render_params2.flags2 &= ~GGTERRAIN_SHADER_FLAG2_SHOW_MAP_SIZE_3D;
				ggterrain_global_render_params2.flags2 &= ~GGTERRAIN_SHADER_FLAG2_SHOW_MINI_MAP;
			}

			//if (ImageExist(TERRAINGENERATOR_IMAGE)) DeleteImage(TERRAINGENERATOR_IMAGE);
			if (ObjectExist(TERRAINGENERATOR_OBJECT)) DeleteObject(TERRAINGENERATOR_OBJECT);
			if (!bPopModalOpenProceduralCameraMode)
			{
				WickedCall_PresetObjectRenderLayer(GGRENDERLAYERS_CURSOROBJECT);

				//UI3D_TERRAINMOVER
				if (!ObjectExist(TERRAINGENERATOR_OBJECT))
					LoadObject("editors\\uiv3\\terrain mover solid.dbo", TERRAINGENERATOR_OBJECT);

				if (!ObjectExist(TERRAINGENERATOR_OBJECT))
				{
					bUseSphere = true;
					MakeObjectSphere(TERRAINGENERATOR_OBJECT, 200.0f, 30, 30);
				}
				WickedCall_PresetObjectRenderLayer(GGRENDERLAYERS_NORMAL);
				float fCenterHeight = BT_GetGroundHeight(t.terrain.TerrainID, fSnapShotModeCameraX, fSnapShotModeCameraZ);
				PositionObject(TERRAINGENERATOR_OBJECT, GGORIGIN_X, fCenterHeight, GGORIGIN_Z);

				if (!bUseSphere && ImageExist(UI3D_TERRAINMOVER))
				{
					//PE: Disable transparent, or we cant see it when below water.
					SetObjectMask(TERRAINGENERATOR_OBJECT, 1);
					TextureObject(TERRAINGENERATOR_OBJECT, UI3D_TERRAINMOVER);
					SetObjectDiffuse(TERRAINGENERATOR_OBJECT, Rgb(200, 200, 200));
					SetObjectEmissive(TERRAINGENERATOR_OBJECT, Rgb(0, 0, 0));
					ShowObject(TERRAINGENERATOR_OBJECT);
				}
				else
				{
					SetObjectEmissive(TERRAINGENERATOR_OBJECT, Rgb(200, 200, 0));
					SetObjectTransparency(TERRAINGENERATOR_OBJECT, 6);
					SetAlphaMappingOn(TERRAINGENERATOR_OBJECT, 50);
					SetObjectDiffuse(TERRAINGENERATOR_OBJECT, Rgb(0, 0, 0));
				}
				sObject* pObject = GetObjectData(TERRAINGENERATOR_OBJECT);
				if (pObject)
				{
					WickedCall_SetObjectCastShadows(pObject, false);
					WickedCall_SetObjectLightToUnlit(pObject, (int)wiScene::MaterialComponent::SHADERTYPE_UNLIT);
				}
				DisableObjectZDepth(TERRAINGENERATOR_OBJECT); //PE: Thanks Lee :)
			}

			bFirstTimeVeg = true;
			#ifndef DIGAHOLE
			extern bool g_bNoTerrainRender;
			g_bNoTerrainRender = true;
			#endif
		}
		else
		{
			static int last_preview_size_x = -1;
			static int last_preview_size_y = -1;

			#ifdef USEFULLVIEWPORT
			if (ImGui::GetMainViewport()->Size.x != last_preview_size_x || ImGui::GetMainViewport()->Size.y != last_preview_size_y)
			{
				//Resize. Delete backbuffer and create a new.
				if (BitmapExist(99))
				{
					DeleteBitmapEx(99);
				}
				last_preview_size_x = ImGui::GetMainViewport()->Size.x;
				last_preview_size_y = ImGui::GetMainViewport()->Size.y;

				BackBufferSizeX = ImGui::GetMainViewport()->Size.x;
				BackBufferSizeY = ImGui::GetMainViewport()->Size.y;

			}
			#else
			if (preview_size_x != last_preview_size_x || preview_size_y != last_preview_size_y)
			{
				//Resize. Delete backbuffer and create a new.
				if (BitmapExist(99))
				{
					DeleteBitmapEx(99);
				}
				last_preview_size_x = preview_size_x;
				last_preview_size_y = preview_size_y;
				BackBufferSizeX = preview_size_x;
				BackBufferSizeY = preview_size_y;
			}
			#endif
		}

		static int iCountToUpdate = 0;
		static bool bDraggingActive = false;
		static bool bObjHoverActive = false;
		static ImRect rClipRect;
		int iMoveTerrainObjectHeight = 900;
		
		extern bool bPreferences_Window;

		//Not if video playing.
		extern bool bLastSmallVideoPlayerMaximized;
		static float last_fx, last_fy, last_fz;
		if (ObjectExist(TERRAINGENERATOR_OBJECT) && !bTriggerTerrainSaveAsWindow && !bPreferences_Window && iQuitProceduralLevel == 0 && !bLastSmallVideoPlayerMaximized )
		{
			bImGuiGotFocus = false;
			ShowObject(TERRAINGENERATOR_OBJECT);
			static float fCursorPosX = 0.0f, fCursorPosY = 0.0f, fCursorPosZ = 0.0f;
			float fCenterHeight = BT_GetGroundHeight(t.terrain.TerrainID, GGORIGIN_X, GGORIGIN_Z);
			if (!bDraggingActive && iCountToUpdate++ >= 10)
			{
				PositionObject(TERRAINGENERATOR_OBJECT, GGORIGIN_X, fCenterHeight + iMoveTerrainObjectHeight, GGORIGIN_Z);
				iCountToUpdate = 0;
				//PE: check hover.
				int layer = GGRENDERLAYERS_CURSOROBJECT;
				float fHitX, fHitY, fHitZ;
				bool bHit = WickedCall_GetPick(&fHitX, &fHitY, &fHitZ, NULL, NULL, NULL, NULL, layer);
				if (bHit)
				{
					bObjHoverActive = true;
				}
				else
				{
					bObjHoverActive = false;
				}
			}
			if(bDraggingActive) bObjHoverActive = false;

			//PE: Only scale when moving done.
			if(movecameratotarget == 0)
			{
				float fTDX = GGORIGIN_X - CameraPositionX();
				float fTDY = fCenterHeight - CameraPositionY();
				float fTDZ = GGORIGIN_Z - CameraPositionZ();
				float fTerrDist = sqrt(fabs(fTDX*fTDX) + fabs(fTDY*fTDY) + fabs(fTDZ*fTDZ));
				float newscale = fTerrDist / 5000.0f;
				if (newscale < 0.2) newscale = 0.2;
				if (newscale > 100.0)newscale = 100.0f;
				if (!bUseSphere)
				{
					ScaleObject(TERRAINGENERATOR_OBJECT, newscale*300.0, newscale*300.0, newscale*300.0);
				}
				else
					ScaleObject(TERRAINGENERATOR_OBJECT, newscale*100.0, newscale*100.0, newscale*100.0);
			}

			static float fObjOffsetX, fObjOffsetY, fObjOffsetZ;
			static float fHitOffsetX, fHitOffsetY, fHitOffsetZ;
			static float fTerrainHitX, fTerrainHitY, fTerrainHitZ;
			static float fTerrainStartHitX, fTerrainStartHitY, fTerrainStartHitZ;
			bool bAreaAlreadyDisplayed = false;
			static ImVec2 newTargetOffset, newTargetCamera;
			int iMoveCameraSteps = 28;

			if (ImGui::IsMouseDown(0))
			{
				if (!bDraggingActive)
				{
					//Setup start values.
					int layer = GGRENDERLAYERS_CURSOROBJECT;
					bool bHit = WickedCall_GetPick(&fHitOffsetX, &fHitOffsetY, &fHitOffsetZ, NULL, NULL, NULL, NULL, layer);
					if (bHit)
					{
						layer = GGRENDERLAYERS_TERRAIN;
						bool bHitTerrain = WickedCall_GetPick(&fTerrainStartHitX, &fTerrainStartHitY, &fTerrainStartHitZ, NULL, NULL, NULL, NULL, layer);
						if (bHitTerrain)
						{
							fObjOffsetX = fHitOffsetX - fTerrainStartHitX;
							fObjOffsetY = fHitOffsetY - fTerrainStartHitY;
							fObjOffsetZ = fHitOffsetZ - fTerrainStartHitZ;
							bDraggingActive = true;
						}
					}
				}
				else
				{
					int layer = GGRENDERLAYERS_TERRAIN;
					bool bHitTerrain = WickedCall_GetPick(&fTerrainHitX, &fTerrainHitY, &fTerrainHitZ, NULL, NULL, NULL, NULL, layer);
					if (bHitTerrain)
					{
						float fX, fY, fZ;
						fX = fTerrainHitX;
						fY = fTerrainHitY;
						fZ = fTerrainHitZ;
						//PE: Dragging.
						float fCenterHeight = BT_GetGroundHeight(t.terrain.TerrainID, fX, fZ);
						PositionObject(TERRAINGENERATOR_OBJECT, fX, fCenterHeight + iMoveTerrainObjectHeight, fZ);

						//PE: Draw the editable area in 2D.
						ImGuiViewport* mainviewport = ImGui::GetMainViewport();
						if (mainviewport)
						{
							ImDrawList* dl = ImGui::GetForegroundDrawList(mainviewport);
							if (dl)
							{
								ImRect bb,dd;
								ImVec2 Convert3DTo2D(float x, float y, float z);
								float fAreaSize = ggterrain_global_render_params2.editable_size;
								fAreaSize *= 0.985; //PE: Make it a bit smaller as we use center for height on all corners.
								ImVec4 col = ImVec4(1.0, 1.0, 0.0, 0.6);

								dl->PushClipRect(rClipRect.Min, rClipRect.Max);

								dl->AddCallback((ImDrawCallback)10, NULL); //force render.

								last_fx = fX;
								last_fy = fCenterHeight;
								last_fz = fZ;

								ImVec2 vecmin = Convert3DTo2D(fX - fAreaSize, fCenterHeight, fZ - fAreaSize);
								ImVec2 vecmax = Convert3DTo2D(fX + fAreaSize, fCenterHeight, fZ - fAreaSize);
								vecmin += ImGui::GetMainViewport()->Pos;
								vecmax += ImGui::GetMainViewport()->Pos;
								if ( !(vecmin.y <= -1300.0 || vecmax.y <= -1300.0 || vecmin.x <= -5000.0 || vecmax.x > 2500.0 ))
									dl->AddLine(vecmin, vecmax, ImGui::GetColorU32(col), 2.0f);

								vecmin = vecmax;
								vecmax = Convert3DTo2D(fX + fAreaSize, fCenterHeight, fZ + fAreaSize);
								vecmax += ImGui::GetMainViewport()->Pos;
								if (!(vecmin.y <= -1300.0 || vecmax.y <= -1300.0 || vecmin.x <= -5000.0 || vecmax.x > 2500.0))
									dl->AddLine(vecmin, vecmax, ImGui::GetColorU32(col), 2.0f);

								vecmin = vecmax;
								vecmax = Convert3DTo2D(fX - fAreaSize, fCenterHeight, fZ + fAreaSize);
								vecmax += ImGui::GetMainViewport()->Pos;
								if (!(vecmin.y <= -1300.0 || vecmax.y <= -1300.0 || vecmin.x <= -5000.0 || vecmax.x > 2500.0))
									dl->AddLine(vecmin, vecmax, ImGui::GetColorU32(col), 2.0f);

								vecmin = vecmax;
								vecmax = Convert3DTo2D(fX - fAreaSize, fCenterHeight, fZ - fAreaSize);
								vecmax += ImGui::GetMainViewport()->Pos;
								if (!(vecmin.y <= -1300.0 || vecmax.y <= -1300.0 || vecmin.x <= -5000.0 || vecmax.x > 2500.0))
									dl->AddLine(vecmin, vecmax, ImGui::GetColorU32(col), 2.0f);

								dl->AddCallback((ImDrawCallback)11, NULL); //disable force render.

								dl->PopClipRect();

								bAreaAlreadyDisplayed = true;
							}
						}
					}
				}
			}
			else
			{
				if (bDraggingActive)
				{
					//Move to new position.
					float offsetX = fTerrainStartHitX - fTerrainHitX;
					float offsetZ = fTerrainStartHitZ - fTerrainHitZ;
					movecameratotarget = iMoveCameraSteps;

					newTargetCamera.x = -offsetX;
					newTargetCamera.y = -offsetZ;

					newTargetOffset.x = ggterrain_global_params.offset_x + GGTerrain_MetersToOffset(GGTerrain_UnitsToMeters(offsetX));
					newTargetOffset.y = ggterrain_global_params.offset_z + GGTerrain_MetersToOffset(GGTerrain_UnitsToMeters(offsetZ));
				}
				bDraggingActive = false;
			}
			bImGuiGotFocus = true;

			if (movecameratotarget > 0)
			{
				iCountToUpdate = 0;
				extern bool g_bNoSwapchainPresent;
				static ImVec2 orgCamera, orgObjPosition;
				movecameratotarget--;
				if (movecameratotarget > 13)
				{
					if (movecameratotarget == iMoveCameraSteps - 1)
					{
						orgCamera.x = fSnapShotModeCameraX;
						orgCamera.y = fSnapShotModeCameraZ;
					}
					fSnapShotModeCameraX += (newTargetCamera.x / (float)(iMoveCameraSteps - 12.0));
					fSnapShotModeCameraZ += (newTargetCamera.y / (float)(iMoveCameraSteps - 12.0));

					if (movecameratotarget == 14)
					{
						fSnapShotModeCameraX = newTargetCamera.x + orgCamera.x;
						fSnapShotModeCameraZ = newTargetCamera.y + orgCamera.y;
					}
				}
				if (movecameratotarget == 13)
				{
					ggterrain_global_params.offset_x = newTargetOffset.x;
					ggterrain_global_params.offset_z = newTargetOffset.y;
				}
				//PE: Freeze while terrain is generating.
				if (movecameratotarget <= 13)
				{
					g_bNoSwapchainPresent = true;
					//PE: No vsync so sleep a bit.
					Sleep(30); //30: looks like terrain is done with this delay.
				}
				if (movecameratotarget == 0)
				{
					fSnapShotModeCameraX = orgCamera.x;
					fSnapShotModeCameraZ = orgCamera.y;
					g_bNoSwapchainPresent = false;
					iCountToUpdate = 10; //9
				}
			}

			//PE: Always display 3d area using lines if possible.
			bool b2DPossible = false;
			//PE: Disable 2D box when using camera offsetting , 2D calc is a bit behind looks strange.
			if (bShowEditArea && !bAreaAlreadyDisplayed && movecameratotarget == 0)
			{
				//PE: Convert3DTo2D only works in topdown as it need the projection matrix so return wrong values if outside projection.
				if (fSnapShotModeCameraY > 36000.0 && fSnapShotModeCameraAngX > 8.0 ) // fSnapShotModeCameraAngX > 85 && fSnapShotModeCameraAngX < 95 && fSnapShotModeCameraY > 40000.0 )
				{
					ImGuiViewport* mainviewport = ImGui::GetMainViewport();
					if (mainviewport)
					{
						ImDrawList* dl = ImGui::GetForegroundDrawList(mainviewport);
						if (dl)
						{
							b2DPossible = true;

							float fX, fY, fZ;
							fX = ObjectPositionX(TERRAINGENERATOR_OBJECT);
							fY = ObjectPositionY(TERRAINGENERATOR_OBJECT);
							fZ = ObjectPositionZ(TERRAINGENERATOR_OBJECT);

							ImVec2 Convert3DTo2D(float x, float y, float z);
							float fAreaSize = ggterrain_global_render_params2.editable_size;
							fAreaSize *= 0.985; //PE: Make it a bit smaller as we use center for height on all corners.
							ImVec4 col = ImVec4(1.0, 1.0, 0.0, 0.6);

							ImVec2 vecmin[4], vecmax[4];

							vecmin[0] = Convert3DTo2D(fX - fAreaSize, fY, fZ - fAreaSize);
							vecmax[0] = Convert3DTo2D(fX + fAreaSize, fY, fZ - fAreaSize);
							vecmin[0] += ImGui::GetMainViewport()->Pos;
							vecmax[0] += ImGui::GetMainViewport()->Pos;

							vecmin[1] = vecmax[0];
							vecmax[1] = Convert3DTo2D(fX + fAreaSize, fY, fZ + fAreaSize);
							vecmax[1] += ImGui::GetMainViewport()->Pos;

							vecmin[2] = vecmax[1];
							vecmax[2] = Convert3DTo2D(fX - fAreaSize, fY, fZ + fAreaSize);
							vecmax[2] += ImGui::GetMainViewport()->Pos;

							vecmin[3] = vecmax[2];
							vecmax[3] = Convert3DTo2D(fX - fAreaSize, fY, fZ - fAreaSize);
							vecmax[3] += ImGui::GetMainViewport()->Pos;

							for(int i = 0; i < 4; i++)
							{
								if(vecmin[i].y <= -1300.0 || vecmax[i].y <= -1300.0 || vecmin[i].x <= -8000.0 || vecmin[i].x > 2500.0 || vecmax[i].x > 2500.0 || vecmax[i].y > 5000.0)
								{
									b2DPossible = false;
									break;
								}
							}
							if (b2DPossible)
							{
								dl->PushClipRect(rClipRect.Min, rClipRect.Max);

								dl->AddCallback((ImDrawCallback)10, NULL); //force render.

								dl->AddLine(vecmin[0], vecmax[0], ImGui::GetColorU32(col), 2.0f); //1
								dl->AddLine(vecmin[1], vecmax[1], ImGui::GetColorU32(col), 2.0f);
								dl->AddLine(vecmin[2], vecmax[2], ImGui::GetColorU32(col), 2.0f);
								dl->AddLine(vecmin[3], vecmax[3], ImGui::GetColorU32(col), 2.0f);

								char text[80];
								float fTmp = GGTerrain_UnitsToMeters(ggterrain_global_render_params2.editable_size * 2.0) / 1000.0f;
								sprintf(text, "Editable Area: %.2f Km / %.2f Miles" , fTmp, fTmp * 0.62137);
								ImVec2 textsize = ImGui::CalcTextSize(text);
								
								ImVec2 vCenterText = vecmin[0] - (((vecmin[0] - vecmax[0]) * 0.45));
								ImGuiContext& g = *GImGui;

								vCenterText -= textsize * 0.5;

								//PE: Special rotation as we use the GetForegroundDrawList that have its own vertex buffer.
								int rotation_start_index = dl->VtxBuffer.Size;
								auto& buf = dl->VtxBuffer;

								//PE: Offset a little from line.
								dl->AddText(g.Font, g.FontSize*1.2, vCenterText + ImVec2(0,-14.0), ImGui::GetColorU32(col), text); //Below line: ImVec2(0,20.0)
								
								//PE Get center.
								ImVec2 l(FLT_MAX, FLT_MAX), u(-FLT_MAX, -FLT_MAX); // bounds
								for (int i = rotation_start_index; i < buf.Size; i++)
									l = ImMin(l, buf[i].pos), u = ImMax(u, buf[i].pos);
								ImVec2 centerdraw = ImVec2((l.x + u.x) / 2, (l.y + u.y) / 2);

								float rad = atan2(vecmin[0].x - vecmax[0].x, vecmin[0].y - vecmax[0].y) + PI;

								//PE: Invert text so we never read upside down.
								if (rad > 3.20) rad += PI;

								float s = sin(rad), c = cos(rad);
								ImVec2 center = ImGuiRotation(centerdraw, s, c) - centerdraw;

								auto& buf2 = dl->VtxBuffer;
								for (int i = rotation_start_index; i < buf2.Size; i++)
								{
									buf2[i].pos = ImGuiRotation(buf2[i].pos, s, c) - center;
								}

								dl->AddCallback((ImDrawCallback)11, NULL); //disable force render.

								dl->PopClipRect();
							}
						}
					}
				}
			}
			if (b2DPossible || bDraggingActive || movecameratotarget > 0)
			{
				ggterrain_global_render_params2.flags2 &= ~GGTERRAIN_SHADER_FLAG2_SHOW_MAP_SIZE;
			}
			else
			{
				if (bShowEditArea) ggterrain_global_render_params2.flags2 |= GGTERRAIN_SHADER_FLAG2_SHOW_MAP_SIZE;
				else ggterrain_global_render_params2.flags2 &= ~GGTERRAIN_SHADER_FLAG2_SHOW_MAP_SIZE;
			}
		}
		else
		{
			bObjHoverActive = false;
		}

		//PE: Edit mode keep change when changing settings inside here, so keep disable paint "circles".
		ggterrain_global_render_params2.flags2 &= ~GGTERRAIN_SHADER_FLAG2_SHOW_BRUSH_SIZE;
		//PE: Make sure we dont have any paint "circles" here.
		set_terrain_sculpt_mode(0); // GGTERRAIN_SCULPT_NONE; Disable terrain sculpt circle.
		set_terrain_edit_mode(0); // GGTERRAIN_EDIT_NONE; Disable terrain paint circle.

		#ifdef DIGAHOLE
		BackBufferSnapShotMode = false;
		#else
		BackBufferSnapShotMode = true;
		#endif
		t.editorfreeflight.mode = 1; //PE: keep freeflight here.

		//PE: New full screen.
		if (bUseFullScreen)
		{
			ImVec2 viewPortPos = ImGui::GetMainViewport()->Pos;
			ImVec2 viewPortSize = ImGui::GetMainViewport()->Size;
			ImGui::SetNextWindowPos(viewPortPos);
			ImGui::SetNextWindowSize(viewPortSize);
		}

		//PE: Need windows background to be non transparent.
		ImVec4 style_winback = ImGui::GetStyle().Colors[ImGuiCol_WindowBg];
		style_winback.w = 1.0f;
		ImGui::PushStyleColor(ImGuiCol_WindowBg, style_winback);
		
		if (bUseModal)
		{
			if (!bUseNoTitleBar)
				bPopModalOpenProcedural = ImGui::BeginPopupModal("##ProceduralPreview", &bProceduralLevel, ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoResize);
			else
				bPopModalOpenProcedural = ImGui::BeginPopupModal("##ProceduralPreview", &bProceduralLevel, ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoResize);
		}
		else
		{
			if (!bUseNoTitleBar)
				bPopModalOpenProcedural = ImGui::Begin("##ProceduralPreview", &bProceduralLevel, ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoResize);
			else
				bPopModalOpenProcedural = ImGui::Begin("##ProceduralPreview", &bProceduralLevel, ImGuiWindowFlags_NoScrollWithMouse |ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoResize);
		}

		// terrain will automatically detect if a change has happened and update ggterrain_extra_params.bTerrainChanged
		if (bPopModalOpenProcedural)
		{
			//PE: Make sure "settings" is always on top of this window.
			ImGuiWindow* storyboard_window = ImGui::GetCurrentWindow();
			void CheckWindowsOnTop(ImGuiWindow* storyboard_window);
			CheckWindowsOnTop(storyboard_window);
			float fPreviewImgSize = preview_size_x;
			float fPreviewImgHeight = preview_size_x *0.5625; //Default.

			rClipRect.Min = ImGui::GetWindowPos()+ImVec2(0.0,67.0); //PE: ,67 = add header.
			rClipRect.Max = ImGui::GetWindowPos() + ImVec2(preview_size_x, preview_size_y);

			#ifdef DIGAHOLE
			bDigAHoleToHWND = true;
			rD3D11DigAHole.left = 2.0;
			rD3D11DigAHole.top = 67.0;
			rD3D11DigAHole.right = fPreviewImgSize;
			rD3D11DigAHole.bottom = ImGui::GetWindowSize().y - 24.0;
			#endif

			ImGui::Columns(2, "ProceduralPreviewColumns", false);  //false no border
			ImGui::SetColumnOffset(0, 0.0f);
			ImGui::SetColumnOffset(1, fPreviewImgSize);

			//PE: Move camera.
			static float fMoveX = 0.0f, fMoveZ = 0.0f;
			static float camposxold = 0.0f, camposzold = 0.0f;

			//Panning speed depend on camera height
			float fCamDistance = ((fSnapShotModeCameraY - 600.0f)*0.75f) * g.timeelapsed_f;
			if (fCamDistance < 50.0f) fCamDistance = 50.0f;

			static float fLastY = -1;
			if (fLastY != fSnapShotModeCameraY)
			{
				if (!bPopModalOpenProceduralCameraMode)
				{
					float fFog = 0.60; //0.35; //0.5
					float ratio = 1.0 - (fSnapShotModeCameraY / fMaxCameraY);
					if (ratio < 0.15) ratio = 0.15;
					if (ratio > 1.0) ratio = 1.0;
					wiScene::WeatherComponent* weather = wiScene::GetScene().weathers.GetComponent(g_weatherEntityID);
					if (t.visuals.skyindex == 0 && t.visuals.bDisableSkybox == false)
						weather->fogColorAndOpacity.w = 0.0; //PE: No fog Opacity in dynamic skybox mode.
					else
					{
						weather->fogColorAndOpacity.w = fFog * ratio;
						t.gamevisuals.FogA_f = t.visuals.FogA_f = oldFogA_f = fFog * ratio;
					}

					float fCover = 0.8;
					weather->volumetricCloudParameters.CoverageAmount = fCover * ratio;
					if (fSnapShotModeCameraY + 4000 > t.visuals.SkyCloudHeight)
					{
						weather->volumetricCloudParameters.CloudStartHeight = GGTerrain_UnitsToMeters(fSnapShotModeCameraY + 4000);
					}
					else
					{
						weather->volumetricCloudParameters.CloudStartHeight = GGTerrain_UnitsToMeters(t.visuals.SkyCloudHeight);
					}

				}
				fLastY = fSnapShotModeCameraY;
			}
			#ifndef DIGAHOLE
			if (ImageExist(iLargePreviewImageID))
			#else
			if(1)
			#endif
			{
				float fMoveSpeed = 512.0f;
				#ifndef DIGAHOLE
				bLoopFullFPS = false;
				float ImgX = ImageWidth(iLargePreviewImageID);
				float ImgY = ImageHeight(iLargePreviewImageID);
				float Ratio = fPreviewImgSize / ImgX;
				//PE: Always fit to Y preview_size_y
				ImgY *= Ratio;
				if (ImgY < preview_size_y)
				{
					ImgY = ImageHeight(iLargePreviewImageID);
					Ratio = preview_size_y / ImgY;
					fPreviewImgSize *= Ratio;
					ImgY = preview_size_y;
					//Revert ratio.
				}
				fPreviewImgHeight = ImgY;
				#endif

				#ifdef USEFULLVIEWPORT
				ImRect avail_window_rect;
				avail_window_rect.Min = ImGui::GetWindowPos();
				avail_window_rect.Max = ImGui::GetWindowPos() + ImVec2(preview_size_x, preview_size_y); //preview_size_x
				ImGui::PushClipRect(avail_window_rect.Min + ImVec2(2, 2), avail_window_rect.Max, false);
				ImGui::SetCursorPos(ImVec2(0.0f, 0.0f));
				#ifndef DIGAHOLE
				ImGui::ImgBtn(iLargePreviewImageID, ImVec2(ImGui::GetMainViewport()->Size.x, ImGui::GetMainViewport()->Size.y), ImVec4(0.0, 0.0, 0.0, 0.0), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(1.0, 1.0, 1.0, 1.0), 0, 0, 0, 0, false);
				#else
				ImGui::Dummy(ImVec2(ImGui::GetMainViewport()->Size.x, ImGui::GetMainViewport()->Size.y));
				#endif
				ImGui::PopClipRect();
				#else
				#ifndef DIGAHOLE
				ImGui::ImgBtn(iLargePreviewImageID, ImVec2(fPreviewImgSize, ImgY), ImVec4(0.0, 0.0, 0.0, 0.0), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(1.0, 1.0, 1.0, 1.0), 0, 0, 0, 0, false);
				#else
				ImGui::Dummy(ImVec2(fPreviewImgSize, ImgY));
				#endif			
				#endif
				bool bIsItemHovered = ImGui::IsItemHovered();
				if (!bTriggerTerrainSaveAsWindow && !bLastSmallVideoPlayerMaximized)
				{
					#ifndef DIGAHOLE
					if (ImGui::IsMouseDown(1) && bIsItemHovered && ImGui::IsMouseDragging(1))
					#else
					//PE: We need imgui positions here.
					D3D11_RECT rD3D11DigAHole_IMGUI_POS;
					rD3D11DigAHole_IMGUI_POS.left = ImGui::GetWindowPos().x + 2.0;
					rD3D11DigAHole_IMGUI_POS.top = ImGui::GetWindowPos().y + 44.0; //Header.
					rD3D11DigAHole_IMGUI_POS.right = ImGui::GetWindowPos().x + fPreviewImgSize;
					rD3D11DigAHole_IMGUI_POS.bottom = ImGui::GetWindowPos().y + ImGui::GetWindowSize().y - 24.0;

					ImVec2 min = { (float)rD3D11DigAHole_IMGUI_POS.left, (float)rD3D11DigAHole_IMGUI_POS.top };
					ImVec2 max = { (float)rD3D11DigAHole_IMGUI_POS.right, (float)rD3D11DigAHole_IMGUI_POS.bottom };
					bIsItemHovered = ImGui::IsMouseHoveringRect(min, max);
					if (ImGui::IsMouseDown(1) && bIsItemHovered && ImGui::IsMouseDragging(1))
					#endif
					{
						float speed = 6.0f;
						float xdiff = ImGui::GetIO().MouseDelta.x / speed;
						float ydiff = ImGui::GetIO().MouseDelta.y / speed;
						t.editorfreeflight.c.angx_f = fSnapShotModeCameraAngX + ydiff;
						t.editorfreeflight.c.angy_f = fSnapShotModeCameraAngY + xdiff;
						if (t.editorfreeflight.c.angx_f > 180.0f)  t.editorfreeflight.c.angx_f = t.editorfreeflight.c.angx_f - 360.0f;
						if (t.editorfreeflight.c.angx_f < -89.999f)  t.editorfreeflight.c.angx_f = -89.999f;
						if (t.editorfreeflight.c.angx_f > 89.999f)  t.editorfreeflight.c.angx_f = 89.999f;
						fSnapShotModeCameraAngX = t.editorfreeflight.c.angx_f;
						fSnapShotModeCameraAngY = t.editorfreeflight.c.angy_f;
						RotateCamera(fSnapShotModeCameraAngX, fSnapShotModeCameraAngY, 0);
					}
					if (!bDraggingActive && !bIs2DViewHovered && ImGui::IsMouseDown(0) && bIsItemHovered && ImGui::IsMouseDragging(0))
					{
						float xdiff = ImGui::GetIO().MouseDelta.x / fMoveSpeed * fCamDistance;
						MoveCameraLeft(0, xdiff);
						float ydiff = ImGui::GetIO().MouseDelta.y / fMoveSpeed * fCamDistance;
						MoveCameraUp(0, ydiff);
						fSnapShotModeCameraX = GetCameraPosition().x;
						fSnapShotModeCameraZ = GetCameraPosition().z;
						#ifndef DIGAHOLE
						bLoopFullFPS = true;
						#endif
						ImGui::SetMouseCursor(ImGuiMouseCursor_Pan);
					}
					else 
					{
						fMoveX = camposxold = fSnapShotModeCameraX;
						fMoveZ = camposzold = fSnapShotModeCameraZ;
					}

					float keyspeed = 15.0f;
					if (fCamDistance < 900.0f) fCamDistance = 900.0f; //Keys faster minimum.
					if (io.KeyShift)
					{
						keyspeed *= 3.5; //Large distances so really speed it up.
					}
					else if (io.KeyCtrl)
					{
						keyspeed *= 0.35; //Large distances so really speed it up.
					}

					if (bIsItemHovered && (ImGui::IsKeyDown(87)|| ImGui::IsKeyDown(38))) //W UP
					{
						MoveCamera(0, (keyspeed / fMoveSpeed * fCamDistance));
						fSnapShotModeCameraX = GetCameraPosition().x;
						fSnapShotModeCameraY = GetCameraPosition().y;
						fSnapShotModeCameraZ = GetCameraPosition().z;
					}
					if (bIsItemHovered && (ImGui::IsKeyDown(83)|| ImGui::IsKeyDown(40))) //S Down
					{
						MoveCamera(0, -(keyspeed / fMoveSpeed * fCamDistance));
						fSnapShotModeCameraX = GetCameraPosition().x;
						fSnapShotModeCameraY = GetCameraPosition().y;
						fSnapShotModeCameraZ = GetCameraPosition().z;
					}
					if (bIsItemHovered && (ImGui::IsKeyDown(68)|| ImGui::IsKeyDown(39))) //D 
					{
						MoveCameraLeft(0, -(keyspeed / fMoveSpeed * fCamDistance));
						fSnapShotModeCameraX = GetCameraPosition().x;
						fSnapShotModeCameraY = GetCameraPosition().y;
						fSnapShotModeCameraZ = GetCameraPosition().z;
					}
					if (bIsItemHovered && (ImGui::IsKeyDown(65)|| ImGui::IsKeyDown(37))) //A
					{
						MoveCameraLeft(0, (keyspeed / fMoveSpeed * fCamDistance));
						fSnapShotModeCameraX = GetCameraPosition().x;
						fSnapShotModeCameraY = GetCameraPosition().y;
						fSnapShotModeCameraZ = GetCameraPosition().z;
					}

					if (bIsItemHovered && ImGui::GetIO().MouseWheel != 0)
					{
						float cammove = fSnapShotModeCameraY;
						if (cammove < 1200) cammove = 1200;
						float speed = cammove / 40.0f; //Speed depent on camera height
						if (io.KeyShift) speed *= 2.0; //Faster when using shift.
						speed *= g.timeelapsed_f;
						MoveCamera(0, ImGui::GetIO().MouseWheel*speed);
						fSnapShotModeCameraX = GetCameraPosition().x;
						fSnapShotModeCameraY = GetCameraPosition().y;
						fSnapShotModeCameraZ = GetCameraPosition().z;
						#ifndef DIGAHOLE
						bLoopFullFPS = true;
						#endif
					}
				}
			}

			//PE: Toolbar look.
			float fToolbarHeight = 90.0f;
			ImGui::PushClipRect(ImGui::GetWindowPos()+ImVec2(2,2), ImGui::GetWindowPos() + ImVec2(preview_size_x+1.0, fToolbarHeight), false);
			ImGui::GetCurrentWindow()->DrawList->AddRectFilled(ImVec2(-1, -1), ImVec2(preview_size_x+1.0, fToolbarHeight), ImGui::GetColorU32(style_winback), 0.0f, ImDrawCornerFlags_None);
			ImGui::PopClipRect();

			ImVec2 vCurPos = ImGui::GetCursorPos();
			ImVec2 vIconSize = { (float) ImGui::GetFontSize()*4.0f, (float) ImGui::GetFontSize()*4.0f };
			if (bUseNoTitleBar)
				ImGui::SetCursorPos(ImVec2(3.0f, 3.0f));
			else
				ImGui::SetCursorPos(ImVec2(2.0f,22.0f));

			ImGui::SetItemAllowOverlap();
			if (ImGui::ImgBtn(TOOL_GOBACK, vIconSize, ImVec4(0, 0, 0, 0), drawCol_normal, drawCol_hover, drawCol_Down, 0, 0, 0, 0, false, false, false, false, false, bBoostIconColors))
			{
				if (!bPopModalOpenProceduralCameraMode)
				{
					if (bProceduralLevelFromStoryboard)
					{
						//PE: Now just goes back to storyboard with no save as.
						int iAction = askBoxCancel("Your new terrain is not saved, are you sure ?", "Confirmation"); //1==Yes 2=Cancel 0=No
						if (iAction == 1)
						{
							bProceduralLevel = false;
						}
					}
					else
					{
						//From level editor , just go back.
						bProceduralLevel = false;
					}
				}
				else
				{
					iQuitProceduralLevel = 5; // Quit with screenshot.
				}
			}
			if (ImGui::IsItemHovered() && iSkibFramesBeforeLaunch == 0) ImGui::SetTooltip("%s", "Exit");

			// only allow view toggle if not in map snapshot mode
			if (bPopModalTakeMapSnapshot == true)
			{
				// When in map snapshot mode, always position camera to point at center of edit area.
				int iEditableSize = ggterrain_global_render_params2.editable_size;
				if (movecameratotarget == 0)
				{
					float fTmp = GGTerrain_UnitsToMeters(iEditableSize * 2.0) / 1000.0f;
					fSnapShotModeCameraY = fTmp * 41000.0f;
					if (fSnapShotModeCameraY > 344000) fSnapShotModeCameraY = 344000; //Hide ugly shadow for now.
					fSnapShotModeCameraX = GGORIGIN_X; // +ggterrain_global_params.offset_x; It dont actual move from center.
					fSnapShotModeCameraZ = GGORIGIN_Z; // +ggterrain_global_params.offset_z;
					fSnapShotModeCameraAngZ = fSnapShotModeCameraAngY = 0.0f;
					fSnapShotModeCameraAngX = 90.0f; //Look down.
				}

				// show edge of game area
				bool bShow = true;
				if (bShow) ggterrain_global_render_params2.flags2 |= GGTERRAIN_SHADER_FLAG2_SHOW_MAP_SIZE;
				else ggterrain_global_render_params2.flags2 &= ~GGTERRAIN_SHADER_FLAG2_SHOW_MAP_SIZE;

				// ensure clouds are off
				extern wiECS::Entity g_weatherEntityID;
				wiScene::WeatherComponent* weather = wiScene::GetScene().weathers.GetComponent(g_weatherEntityID);
				weather->SetRealisticSky(false);
				weather->SetVolumetricClouds(false);

				// and override camera projection to create an ortho view
				wiScene::GetCamera().SetCustomProjectionEnabled(true);
				float fTmp = GGTerrain_UnitsToMeters(iEditableSize * 2.0) / 1000.0f;
				float fScaleProj = 50000.0f * fTmp;
				float fRatioProj = (float)master.masterrenderer.GetLogicalWidth() / (float)master.masterrenderer.GetLogicalHeight();
				XMMATRIX P = XMMatrixOrthographicLH(fScaleProj * fRatioProj, fScaleProj, wiScene::GetCamera().zFarP, wiScene::GetCamera().zNearP);
				XMStoreFloat4x4(&wiScene::GetCamera().Projection, P);
				wiScene::GetCamera().UpdateCamera();
			}
			else
			{
				//PE: Camera Tool Icon.
				ImGui::SetCursorPos(ImVec2(ImGui::GetContentRegionAvailWidth() - vIconSize.x, 3.0f));
				ImGui::SetItemAllowOverlap();
				if (fSnapShotModeCameraAngX > 85 && fSnapShotModeCameraAngX < 95)
				{
					if (ImGui::ImgBtn(TOOL_CAMERA, vIconSize, ImVec4(0, 0, 0, 0), drawCol_normal, drawCol_hover, drawCol_Down, 0, 0, 0, 0, false, false, false, false, false, bBoostIconColors))
					{
						if (movecameratotarget == 0)
						{
							if (fSnapShotModeCameraY > 344000) fSnapShotModeCameraY = 344000; //Hide ugly shadow for now.
							//PE: Now start up high looking at the editable area.
							fSnapShotModeCameraX = 73800;
							fSnapShotModeCameraZ = -74000;
							fSnapShotModeCameraY = 51600;
							fSnapShotModeCameraAngZ = 0.0f;
							fSnapShotModeCameraAngY = -37;
							fSnapShotModeCameraAngX = 25; //34;
						}
					}
					if (ImGui::IsItemHovered() && iSkibFramesBeforeLaunch == 0) ImGui::SetTooltip("%s", "Change to 3D View");
				}
				else
				{
					if (ImGui::ImgBtn(TOOL_CAMERA, vIconSize, ImVec4(0, 0, 0, 0), drawCol_normal, drawCol_hover, drawCol_Down, 0, 0, 0, 0, false, false, false, false, false, bBoostIconColors))
					{
						if (movecameratotarget == 0)
						{
							float fTmp = GGTerrain_UnitsToMeters(ggterrain_global_render_params2.editable_size * 2.0) / 1000.0f;
							//Reset camera to point at center of edit area.
							fSnapShotModeCameraY = fTmp * 41000.0f;
							if (fSnapShotModeCameraY > 344000) fSnapShotModeCameraY = 344000; //Hide ugly shadow for now.
							fSnapShotModeCameraX = GGORIGIN_X; // +ggterrain_global_params.offset_x; It dont actual move from center.
							fSnapShotModeCameraZ = GGORIGIN_Z; // +ggterrain_global_params.offset_z;
							fSnapShotModeCameraAngZ = fSnapShotModeCameraAngY = 0.0f;
							fSnapShotModeCameraAngX = 90.0f; //Look down.
						}
					}
					if (ImGui::IsItemHovered() && iSkibFramesBeforeLaunch == 0) ImGui::SetTooltip("%s", "Change to Top Down View");
				}
			}

			if (bUseNoTitleBar)
				ImGui::SetCursorPos(ImVec2(3.0f, 4.0));
			else
				ImGui::SetCursorPos(ImVec2(2.0f, 22.0f + 10.0));

			extern ImFont* customfont;
			extern ImFont* customfontlarge;

			if(customfontlarge) ImGui::PushFont(customfontlarge);
			ImGui::SetWindowFontScale(2.0);
			if (!bPopModalOpenProceduralCameraMode)
			{
				ImGui::TextCenter("Terrain Generator");
			}
			else
			{
				ImGui::TextCenter("Snapshot Mode");
			}
			ImGui::SetWindowFontScale(1.0);
			ImGui::PushFont(customfont);

			#ifdef NOMINIMAP
			if (!bPopModalOpenProceduralCameraMode && bPopModalTakeMapSnapshot == false)
			{
				if (bObjHoverActive) ImGui::SetTooltip("%s", "Grab and move around the terrain to choose the actual playable area you want for your game.");
			}
			if(0)
			#endif
			{
				if (!bTriggerTerrainSaveAsWindow)
				{
					ImVec2 vOverlayPos;

					//New 2D Overlay square.
					float fRatio = 0.18; //Larger
					float fOverlayOffsetX = 144.0f;
					float fOverlayOffsetY = 50.0f; //PE: Without lower buttons.
					float imgw = fPreviewImgSize * fRatio;
					float imgh = fPreviewImgSize * fRatio;
					bIs2DViewHovered = false;

					float fSquareRatio = (ggterrain_global_render_params2.editable_size / 39.3701 / 1000.0f) * 80.0f;

					float fSize = fSquareRatio; //44.0f;
					float fCenter = fSize * 0.5;
					static float move_areax = 0, move_areay = 0;
					static float bIsAreaMoving = false;

					if (!bPopModalOpenProceduralCameraMode)
					{
						vOverlayPos = ImVec2(ImVec2(ImGui::GetFontSize() + 10.0f + fOverlayOffsetX + (move_areax - fCenter), fPreviewImgHeight + (move_areay - fCenter)) - ImVec2(0.0f, imgh + ImGui::GetFontSize() + fOverlayOffsetY));
						ImGui::SetCursorPos(vOverlayPos);
						ImGui::SetItemAllowOverlap();
						ImGui::ImgBtn(SHAPE_SQUARE, ImVec2(fSize, fSize), ImVec4(0.0, 0.0, 0.0, 0.0), ImVec4(1.0, 1.0, 0.0, 1.0), ImVec4(1.0, 1.0, 0.0, 1.0), ImVec4(1.0, 1.0, 0.0, 1.0), 0, 0, 0, 0, false);
						if (ImGui::IsItemHovered())
						{
							bIs2DViewHovered = true;
							ImGui::SetTooltip("%s", "Click to drag editable area");
						}
						static bool bGotNewArea = false;
						static float fNewOffsetX, fNewOffsetZ;
						if (ImGui::IsMouseDown(0) && ImGui::IsItemHovered())
						{
							bIsAreaMoving = true;
							if (ImGui::IsMouseDragging(0))
							{
								bGotNewArea = true;
								//PE: is ggterrain_global_params.offset_x now in meters ?
								float fMoveSpeed = 30.0; //Looks like this is the scale from mouse coords to mapsize.
								float fBorder = 20.0;
								float xdiff = ImGui::GetIO().MouseDelta.x;
								if ((move_areax + xdiff) < imgw - fBorder && (move_areax + xdiff) > fBorder)
								{
									move_areax += xdiff;
									fNewOffsetX -= (xdiff / fMoveSpeed);
								}

								float ydiff = ImGui::GetIO().MouseDelta.y;
								if ((move_areay + ydiff) < imgh - fBorder && (move_areay + ydiff) > fBorder)
								{
									move_areay += ydiff;
									fNewOffsetZ += (ydiff / fMoveSpeed);
								}
								#ifndef DIGAHOLE
								bLoopFullFPS = true;
								#endif

							}
							ImGui::SetMouseCursor(ImGuiMouseCursor_Pan);
						}
						else {
							static bool bAreaInit = false;
							if (bGotNewArea)
							{
								//Reposition and center.
								ggterrain_global_params.offset_x = fNewOffsetX;
								ggterrain_global_params.offset_z = fNewOffsetZ;
								bGotNewArea = false;
								bAreaInit = false;
							}

							fNewOffsetX = ggterrain_global_params.offset_x;
							fNewOffsetZ = ggterrain_global_params.offset_z;
							//Place in center.
							if (!bAreaInit)
							{
								move_areax = imgw * 0.5; //+2.0 to match Paul area location.
								move_areay = imgh * 0.5;
								bAreaInit = true;
							}
							bIsAreaMoving = false;
						}
					}
				}
			}

			ImGui::SetCursorPos(vCurPos);
			ImGui::Spacing();
			ImGui::NextColumn();

			if (!bPopModalOpenProceduralCameraMode)
			{
				//PE: After large "generate..." button.
				ImGui::BeginChild("##ChildProceduralPreview", ImVec2(0, preview_size_y - 78), false, ImGuiWindowFlags_ForceRender | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNavInputs);

				//PE: Find a stable terrain height Y. To adjusty 3D view height if terrain change.
				static int stable_height_y = 0;
				static int stable_height_y_count = 0;
				static int iLastHeightY = -1;
				static int last_stable_height_y = -1;

				int iHeightY = -999999;
				float height;
				if (bTriggerStableY)
				{
					if (GGTerrain_GetHeight(fSnapShotModeCameraX, fSnapShotModeCameraZ, &height))
					{
						//PE: Only use valid Y.
						iHeightY = height;
						if (iHeightY != iLastHeightY)
						{
							stable_height_y_count = 15;
							iLastHeightY = iHeightY;
						}
						if (stable_height_y_count > 0)
						{
							if (--stable_height_y_count == 0)
							{
								stable_height_y = iHeightY;
								if (last_stable_height_y != stable_height_y)
								{
									//PE: Only if terrain changed from last update (dont break free flight).
									last_stable_height_y = stable_height_y;

									if ((stable_height_y + i3DViewHeight) > fSnapShotModeCameraY)
									{
										//PE: Only update if terrain got higher to not break free flight.
										fSnapShotModeCameraY = stable_height_y;
										fSnapShotModeCameraY += i3DViewHeight; //PE: A bit above terrain.
									}
								}
								bTriggerStableY = false;
							}
						}
					}
				}

				static bool bRandomizeTimeOfDay = true;
				static int iLastUserSelectedTimeOfDay = -1;
				static bool bSelectRandomSkybox = false;
				static bool bSelectNightSkybox = false;
				static bool bSelectDayRandomSkybox = false;
				static int iSelectedThemeChoice = 0;
				static bool bFirtTimeInTheme = false;
				static bool bFirstBiomes[9] = { true,true,true,true,true,true,true,true,true };

				if(bSelectRandomSkybox || bSelectNightSkybox)
				{
					bSelectRandomSkybox = false;

					#ifdef RANDOMSKYBOX
					int iRandom = (rand() % (t.skybank_s.size() ));
					int skyindex = iRandom;
					if (bSelectNightSkybox)
					{
						skyindex = iRandom = 5; //Night skybox.
					}
					bool bTriggerSetTimeOfDay = false;

					wiScene::WeatherComponent* weather = wiScene::GetScene().weathers.GetComponent(g_weatherEntityID);

					bool bForceDay = false;
					if (iSelectedThemeChoice < 10)
					{
						if (bFirstBiomes[iSelectedThemeChoice] || bSelectDayRandomSkybox)
						{
							bFirstBiomes[iSelectedThemeChoice] = false;
							bSelectDayRandomSkybox = false;
							bForceDay = true;
						}
					}

					if (((rand() % 3) == 0 || bForceDay) && bSelectNightSkybox == false)
					{
						//PE: Dynamic every 3 times randomly.
						t.gamevisuals.skyindex = t.visuals.skyindex = 0;
						t.gamevisuals.bDisableSkybox = t.visuals.bDisableSkybox = false;
						if (weather)
						{
							if (weather->skyMap != nullptr && weather->skyMapName.length() > 0)
							{
								//PE: Make sure to free any old resources.
								WickedCall_DeleteImage(weather->skyMapName);
							}
							weather->skyMap = nullptr;
							weather->skyMapName = "";
							weather->cloudSpeed = t.visuals.SkyCloudSpeed;
							weather->cloudiness = t.visuals.SkyCloudiness;
						}
						Wicked_Update_Visuals((void *)&t.visuals); //PE: Switch to dynamic skybox.
						bTriggerSetTimeOfDay = true;
					}
					else if (iRandom == 0 || iRandom  >= t.skybank_s.size() )
					{
						//Delete if we already have one loaded.
						if (weather)
						{
							if (weather->skyMap != nullptr && weather->skyMapName.length() > 0)
							{
								//PE: Make sure to free any old resources.
								WickedCall_DeleteImage(weather->skyMapName);
							}
							weather->skyMap = nullptr;
							weather->skyMapName = "";
							weather->cloudSpeed = t.visuals.SkyCloudSpeed;
							weather->cloudiness = t.visuals.SkyCloudiness;
						}
						t.visuals.skyindex = 0;
						t.gamevisuals.skyindex = t.visuals.skyindex;
						t.gamevisuals.bDisableSkybox = t.visuals.bDisableSkybox = true;
						bTriggerSetTimeOfDay = true;
					}
					else
					{
						t.gamevisuals.bDisableSkybox = t.visuals.bDisableSkybox = false;
						t.visuals.skyindex = skyindex;
						t.gamevisuals.skyindex = t.visuals.skyindex;

						g.skyindex = t.visuals.skyindex;
						t.visuals.sky_s = t.skybank_s[g.skyindex];
						t.gamevisuals.sky_s = t.skybank_s[g.skyindex];
						t.terrainskyspecinitmode = 0;
						sky_skyspec_init();
						t.sky.currenthour_f = 8.0;
						t.sky.daynightprogress = 0;
						WickedCall_UpdateProbes();

						t.gamevisuals.SunAngleX = t.visuals.SunAngleX;
						t.gamevisuals.SunAngleY = t.visuals.SunAngleY;
						t.gamevisuals.SunAngleZ = t.visuals.SunAngleZ;

						oldSunAngleX = t.visuals.SunAngleX; //PE: Make sure to update after save.
						oldSunAngleY = t.visuals.SunAngleY;
						oldSunAngleZ = t.visuals.SunAngleZ;

					}
					fLastY = -1; //PE: Trigger a update to fog and dynamic sky.
					bSelectNightSkybox = false;

					//PE: Check if we need to update time of day if skybox changed and we have no random time of day.
					if (bTriggerSetTimeOfDay)
					{
						if (!bRandomizeTimeOfDay && iLastUserSelectedTimeOfDay >= 0)
						{
							t.gamevisuals.iTimeOfday = t.visuals.iTimeOfday = iLastUserSelectedTimeOfDay;
							visuals_calcsunanglefromtimeofday(t.gamevisuals.iTimeOfday, &t.gamevisuals.SunAngleX, &t.gamevisuals.SunAngleY, &t.gamevisuals.SunAngleZ);
							t.visuals.SunAngleX = t.gamevisuals.SunAngleX;
							t.visuals.SunAngleY = t.gamevisuals.SunAngleY;
							t.visuals.SunAngleZ = t.gamevisuals.SunAngleZ;
							oldSunAngleX = t.visuals.SunAngleX; //PE: Make sure to update after save.
							oldSunAngleY = t.visuals.SunAngleY;
							oldSunAngleZ = t.visuals.SunAngleZ;
							Wicked_Update_Visuals((void *)&t.visuals);
						}
					}

					#endif
				}

				int iRandomTimeOfDayChoice = -1;

				static bool iLastTreeGrassSettings = -1;
				ImGui::Text("");
				if (ImGui::StyleCollapsingHeader("Terrain Biome", ImGuiTreeNodeFlags_DefaultOpen))
				{
					float feditable_size = ggterrain_global_render_params2.editable_size;

					ImGui::PushItemWidth(-10);
					float fButSizeX = ImGui::GetContentRegionAvailWidth() / 4.0;
					fButSizeX -= 6.0f; //Padding.
					float fButSizeY = fButSizeX * 0.60f;

					// when FIRST enter procedural terrain generator, randomly select a theme
					int iRandomThemeChoice = 0;
					extern bool bProceduralLevelStartup;
					if (bProceduralLevelStartup == true)
					{
						timestampactivity(0, "GGTerrain_RemoveAllFlatAreas:3");
						GGTerrain_RemoveAllFlatAreas(); //PE: Remove all flat areas.
						iRandomThemeChoice = 7; //PE: new design always rainforest.
						bProceduralLevelStartup = false;
						bTriggerStableY = true;
						iRandomTimeOfDayChoice = 3; // (1) PE: Default now afternoon 3
						fLastY = -1; //Trigger fog update.
						iLastTreeGrassSettings = -1;
					}

					ImGui::TextCenter("Default Choices");

					ImVec4 outline_color = ImGui::GetStyle().Colors[ImGuiCol_PlotHistogram];
					ImVec2 vSelectionDraw = ImGui::GetCurrentWindow()->DC.CursorPos;
					if (ImGui::StyleButton("Plains", ImVec2(fButSizeX, fButSizeY)) || iRandomThemeChoice == 1)
					{

						if(bRandomizeTimeOfDay && iRandomThemeChoice == 0 ) iRandomTimeOfDayChoice = (rand() % 7);
						if (iRandomThemeChoice == 0) bSelectRandomSkybox = true;
						iSelectedThemeChoice = 1;

						//editors\biomes\plains.dat
						GGTerrainFile_LoadTerrainData("editors\\biomes\\plains.dat", false);
						ggterrain_global_params.seed = Random2();

						// ggtrees_global_params.draw_enabled = 0; //PE: Trees is now only controlled by visual.ini t.visuals.bEndableTreeDrawing
						t.showeditortrees = t.gamevisuals.bEndableTreeDrawing = t.visuals.bEndableTreeDrawing = 0;
						t.showeditorveg = t.gamevisuals.bEndableGrassDrawing = t.visuals.bEndableGrassDrawing = 0;
						t.showeditorterrain = t.gamevisuals.bEndableTerrainDrawing = t.visuals.bEndableTerrainDrawing = 1;
						
						//PE: Default tree and grass setup.
						if (iLastTreeGrassSettings != 0)
						{
							iLastTreeGrassSettings = 0;
							ggtrees_global_params.paint_tree_bitfield = 1; //Plains
							ggtrees_global_params.paint_scale_random_low = 40.0; //Default Scale Min.
							ggtrees_global_params.paint_scale_random_high = 200.0; //Default Scale Max.
							GGTrees::GGTrees_ChangeDensity(65); //Default density.
							GGTrees::ggtrees_global_params.hide_until_update = 1;
							GGTrees::ggtrees_global_params.draw_enabled = 0;
							gggrass_global_params.paint_type = 1; //Default grass
							gggrass_global_params.paint_density = 100; //Default Density
							gggrass_global_params.paint_material = 0; //Auto
							GGGrass::GGGrass_AddAll();
						}

						t.terrain.waterliney_f = g.gdefaultwaterheight = 13.1234331; //-23; // = 75; -500
						t.gamevisuals.WaterSpeed1 = t.visuals.WaterSpeed1 = 0.03; // = 3 0.06
						t.gamevisuals.WaterRed_f = t.visuals.WaterRed_f = 11; // 9
						t.gamevisuals.WaterGreen_f = t.visuals.WaterGreen_f = 17; //21
						t.gamevisuals.WaterBlue_f = t.visuals.WaterBlue_f = 25; //43
						t.gamevisuals.WaterAlpha_f = t.visuals.WaterAlpha_f = 255; //0
						t.gamevisuals.fWaterWaveAmplitude = t.visuals.fWaterWaveAmplitude = 20; //20
						t.gamevisuals.fWaterWindDependency = t.visuals.fWaterWindDependency = 0;// 0
						t.gamevisuals.fWaterPatchLength = t.visuals.fWaterPatchLength = 17; // 40
						t.gamevisuals.fWaterChoppyScale = t.visuals.fWaterChoppyScale = 0; // 0
						t.gamevisuals.WaterFogMinDist = t.visuals.WaterFogMinDist = 0; //start 0
						t.gamevisuals.WaterFogMaxDist = t.visuals.WaterFogMaxDist = 3000.0; // 11500
						t.gamevisuals.WaterFogMinAmount = t.visuals.WaterFogMinAmount = 0.57; // = 58 0.25

						//new

						t.visuals.bWaterEnable = true;
						t.gamevisuals.bWaterEnable = t.visuals.bWaterEnable;
						Wicked_Update_Visuals((void *)&t.visuals);

						//Create Level
						ggterrain_extra_params.iProceduralTerrainType = 1;
						ggterrain_global_params.fractal_initial_amplitude = 1.0f;

						t.gamevisuals.bEndableAmbientMusicTrack = t.visuals.bEndableAmbientMusicTrack = true;
						t.visuals.sAmbientMusicTrack = "audiobank\\ambient\\Plains.wav";
						t.gamevisuals.sAmbientMusicTrack = t.visuals.sAmbientMusicTrack;
						t.gamevisuals.bEnableCombatMusicTrack = t.visuals.bEnableCombatMusicTrack = false;
						t.visuals.sCombatMusicTrack = "";
						t.gamevisuals.sCombatMusicTrack = t.visuals.sCombatMusicTrack;

						ggtrees_global_params.water_dist = 400.0;
						bTriggerStableY = true;
					}
					if (iSelectedThemeChoice == 1)
					{
						ImVec2 padding = { 0.0, 0.0 };
						const ImRect image_bb((vSelectionDraw - padding), vSelectionDraw + padding + ImVec2(fButSizeX, fButSizeY));
						ImGui::GetCurrentWindow()->DrawList->AddRect(image_bb.Min, image_bb.Max, ImGui::GetColorU32(outline_color), 0.0f, 15, 2.0f);
					}
					if (ImGui::IsItemHovered()) ImGui::SetTooltip("Select Plains Theme.");
					ImGui::SameLine();

					vSelectionDraw = ImGui::GetCurrentWindow()->DC.CursorPos;
					if (ImGui::StyleButton("Desert", ImVec2(fButSizeX, fButSizeY)) || iRandomThemeChoice == 2)
					{
						if (bRandomizeTimeOfDay && iRandomThemeChoice == 0) iRandomTimeOfDayChoice = (rand() % 7);
						if (iRandomThemeChoice == 0) bSelectRandomSkybox = true;
						iSelectedThemeChoice = 2;

						GGTerrainFile_LoadTerrainData("editors\\biomes\\desert.dat", false);
						ggterrain_global_params.seed = Random2();
						ggterrain_global_render_params.slopeMatIndex[0] = 0x100 | 25;
						ggterrain_global_render_params.slopeMatIndex[1] = 0x100 | 25;

						t.showeditortrees = t.gamevisuals.bEndableTreeDrawing = t.visuals.bEndableTreeDrawing = 1;
						t.showeditorveg = t.gamevisuals.bEndableGrassDrawing = t.visuals.bEndableGrassDrawing = 1;
						t.showeditorterrain = t.gamevisuals.bEndableTerrainDrawing = t.visuals.bEndableTerrainDrawing = 1;

						if (iLastTreeGrassSettings != 2)
						{
							iLastTreeGrassSettings = 2;

							//Dessert cactus + grass dessert.
							ggtrees_global_params.paint_tree_bitfield = 30; //PE: New trees 30 cactus , old 4;
							ggtrees_global_params.paint_scale_random_low = 40.0; //Scale Min.
							ggtrees_global_params.paint_scale_random_high = 200.0; //Scale Max.
							GGTrees::GGTrees_ChangeDensity(3); //3=Low density, Also set new type and scale.
							GGTrees::ggtrees_global_params.hide_until_update = 1;
							GGTrees::ggtrees_global_params.draw_enabled = 0;

							gggrass_global_params.paint_type = 528; //PE: New palette was 458752; //dessert types.
							gggrass_global_params.paint_density = 3; //Low Density
							gggrass_global_params.paint_material = 0; //Auto
							GGGrass::GGGrass_AddAll();
						}

						//PE: Default water settings.
						t.terrain.waterliney_f = g.gdefaultwaterheight = -500;
						t.gamevisuals.WaterSpeed1 = t.visuals.WaterSpeed1 = 0.06;
						t.gamevisuals.WaterRed_f = t.visuals.WaterRed_f = 9;
						t.gamevisuals.WaterGreen_f = t.visuals.WaterGreen_f = 21;
						t.gamevisuals.WaterBlue_f = t.visuals.WaterBlue_f = 43;
						t.gamevisuals.WaterAlpha_f = t.visuals.WaterAlpha_f = 0;
						t.gamevisuals.fWaterWaveAmplitude = t.visuals.fWaterWaveAmplitude = 20;
						t.gamevisuals.fWaterWindDependency = t.visuals.fWaterWindDependency = 0;
						t.gamevisuals.fWaterPatchLength = t.visuals.fWaterPatchLength = 40;
						t.gamevisuals.fWaterChoppyScale = t.visuals.fWaterChoppyScale = 0;
						t.gamevisuals.WaterFogMinDist = t.visuals.WaterFogMinDist = 0;
						t.gamevisuals.WaterFogMaxDist = t.visuals.WaterFogMaxDist = 11500;
						t.gamevisuals.WaterFogMinAmount = t.visuals.WaterFogMinAmount = 0.25;

						t.visuals.bWaterEnable = true;
						t.gamevisuals.bWaterEnable = t.visuals.bWaterEnable;
						Wicked_Update_Visuals((void *)&t.visuals);

						//Create Level
						ggterrain_extra_params.iProceduralTerrainType = 2;
						ggterrain_global_params.fractal_initial_amplitude = 1.0f;

						t.gamevisuals.bEndableAmbientMusicTrack = t.visuals.bEndableAmbientMusicTrack = true;
						t.visuals.sAmbientMusicTrack = "audiobank\\ambient\\Desert.wav";
						t.gamevisuals.sAmbientMusicTrack = t.visuals.sAmbientMusicTrack;
						t.gamevisuals.bEnableCombatMusicTrack = t.visuals.bEnableCombatMusicTrack = false;
						t.visuals.sCombatMusicTrack = "";
						t.gamevisuals.sCombatMusicTrack = t.visuals.sCombatMusicTrack;

						ggtrees_global_params.water_dist = 400.0;
						bTriggerStableY = true;

					}
					if (iSelectedThemeChoice == 2)
					{
						ImVec2 padding = { 0.0, 0.0 };
						const ImRect image_bb((vSelectionDraw - padding), vSelectionDraw + padding + ImVec2(fButSizeX, fButSizeY));
						ImGui::GetCurrentWindow()->DrawList->AddRect(image_bb.Min, image_bb.Max, ImGui::GetColorU32(outline_color), 0.0f, 15, 2.0f);
					}
					if (ImGui::IsItemHovered()) ImGui::SetTooltip("Select Desert Theme");
					ImGui::SameLine();

					vSelectionDraw = ImGui::GetCurrentWindow()->DC.CursorPos;
					if (ImGui::StyleButton("Forest", ImVec2(fButSizeX, fButSizeY)) || iRandomThemeChoice == 3)
					{
						if (bRandomizeTimeOfDay && iRandomThemeChoice == 0) iRandomTimeOfDayChoice = (rand() % 7);
						if (iRandomThemeChoice == 0) bSelectRandomSkybox = true;
						iSelectedThemeChoice = 3;

						GGTerrainFile_LoadTerrainData("editors\\biomes\\forest.dat", false);
						ggterrain_global_params.seed = Random2();

						// ggtrees_global_params.draw_enabled = 1; //PE: Trees is now only controlled by visual.ini t.visuals.bEndableTreeDrawing
						t.showeditortrees = t.gamevisuals.bEndableTreeDrawing = t.visuals.bEndableTreeDrawing = 1;
						t.showeditorveg = t.gamevisuals.bEndableGrassDrawing = t.visuals.bEndableGrassDrawing = 1;
						t.showeditorterrain = t.gamevisuals.bEndableTerrainDrawing = t.visuals.bEndableTerrainDrawing = 1;

						if (iLastTreeGrassSettings != 3)
						{
							iLastTreeGrassSettings = 3;
							//PE: Default tree and grass setup.
							ggtrees_global_params.paint_tree_bitfield = 7340033; //Forest
							ggtrees_global_params.paint_scale_random_low = 40.0; //Default Scale Min.
							ggtrees_global_params.paint_scale_random_high = 200.0; //Default Scale Max.
							GGTrees::GGTrees_ChangeDensity(90); //Default density.
							GGTrees::ggtrees_global_params.hide_until_update = 1;
							GGTrees::ggtrees_global_params.draw_enabled = 0;
							gggrass_global_params.paint_type = 1167; //Default grass
							gggrass_global_params.paint_density = 100; //Default Density
							gggrass_global_params.paint_material = 0; //Auto
							GGGrass::GGGrass_AddAll();
						}

						//PE: Default water settings.
						t.terrain.waterliney_f = g.gdefaultwaterheight = 0;
						t.gamevisuals.WaterSpeed1 = t.visuals.WaterSpeed1 = 0.1;
						t.gamevisuals.WaterRed_f = t.visuals.WaterRed_f = 4;
						t.gamevisuals.WaterGreen_f = t.visuals.WaterGreen_f = 19;
						t.gamevisuals.WaterBlue_f = t.visuals.WaterBlue_f = 51;
						t.gamevisuals.WaterAlpha_f = t.visuals.WaterAlpha_f = 0;
						t.gamevisuals.fWaterWaveAmplitude = t.visuals.fWaterWaveAmplitude = 20;
						t.gamevisuals.fWaterWindDependency = t.visuals.fWaterWindDependency = 0;
						t.gamevisuals.fWaterPatchLength = t.visuals.fWaterPatchLength = 27;
						t.gamevisuals.fWaterChoppyScale = t.visuals.fWaterChoppyScale = 0;
						t.gamevisuals.WaterFogMinDist = t.visuals.WaterFogMinDist = 0;
						t.gamevisuals.WaterFogMaxDist = t.visuals.WaterFogMaxDist = 1000;
						t.gamevisuals.WaterFogMinAmount = t.visuals.WaterFogMinAmount = 0.25;

						t.visuals.bWaterEnable = true;
						t.gamevisuals.bWaterEnable = t.visuals.bWaterEnable;
						Wicked_Update_Visuals((void *)&t.visuals);

						//Create Level
						ggterrain_extra_params.iProceduralTerrainType = 3;
						ggterrain_global_params.fractal_initial_amplitude = 1.0f;

						t.gamevisuals.bEndableAmbientMusicTrack = t.visuals.bEndableAmbientMusicTrack = true;
						t.visuals.sAmbientMusicTrack = "audiobank\\ambient\\Forest.wav";
						t.gamevisuals.sAmbientMusicTrack = t.visuals.sAmbientMusicTrack;
						t.gamevisuals.bEnableCombatMusicTrack = t.visuals.bEnableCombatMusicTrack = false;
						t.visuals.sCombatMusicTrack = "";
						t.gamevisuals.sCombatMusicTrack = t.visuals.sCombatMusicTrack;

						ggtrees_global_params.water_dist = 41.0;
						bTriggerStableY = true;

					}
					if (iSelectedThemeChoice == 3)
					{
						ImVec2 padding = { 0.0, 0.0 };
						const ImRect image_bb((vSelectionDraw - padding), vSelectionDraw + padding + ImVec2(fButSizeX, fButSizeY));
						ImGui::GetCurrentWindow()->DrawList->AddRect(image_bb.Min, image_bb.Max, ImGui::GetColorU32(outline_color), 0.0f, 15, 2.0f);
					}
					if (ImGui::IsItemHovered()) ImGui::SetTooltip("Select Forest Theme");
					ImGui::SameLine();

					vSelectionDraw = ImGui::GetCurrentWindow()->DC.CursorPos;
					if (ImGui::StyleButton("Snow", ImVec2(fButSizeX, fButSizeY)) || iRandomThemeChoice == 4) //Arctic
					{
						if (bRandomizeTimeOfDay && iRandomThemeChoice == 0) iRandomTimeOfDayChoice = (rand() % 7);
						if (iRandomThemeChoice == 0) bSelectRandomSkybox = true;
						iSelectedThemeChoice = 4;

						GGTerrainFile_LoadTerrainData("editors\\biomes\\snow.dat", false);
						ggterrain_global_params.seed = Random2();
						t.showeditortrees = t.gamevisuals.bEndableTreeDrawing = t.visuals.bEndableTreeDrawing = 1;
						t.showeditorveg = t.gamevisuals.bEndableGrassDrawing = t.visuals.bEndableGrassDrawing = 1;
						t.showeditorterrain = t.gamevisuals.bEndableTerrainDrawing = t.visuals.bEndableTerrainDrawing = 1;

						if (iLastTreeGrassSettings != 4)
						{
							iLastTreeGrassSettings = 4;
							//PE: Default tree and grass setup.
							ggtrees_global_params.paint_tree_bitfield = 520093696; //Snow
							ggtrees_global_params.paint_scale_random_low = 40.0; //Default Scale Min.
							ggtrees_global_params.paint_scale_random_high = 200.0; //Default Scale Max.
							GGTrees::GGTrees_ChangeDensity(10); //Default density.
							GGTrees::ggtrees_global_params.hide_until_update = 1;
							GGTrees::ggtrees_global_params.draw_enabled = 0;

							gggrass_global_params.paint_type = 1167; //Default grass
							gggrass_global_params.paint_density = 4; //Default Density
							gggrass_global_params.paint_material = 0; //Auto
							GGGrass::GGGrass_AddAll();
						}

						//PE: Default water settings.
						t.terrain.waterliney_f = g.gdefaultwaterheight = -500;
						t.gamevisuals.WaterSpeed1 = t.visuals.WaterSpeed1 = 0.06;
						t.gamevisuals.WaterRed_f = t.visuals.WaterRed_f = 6;
						t.gamevisuals.WaterGreen_f = t.visuals.WaterGreen_f = 32;
						t.gamevisuals.WaterBlue_f = t.visuals.WaterBlue_f = 80;
						t.gamevisuals.WaterAlpha_f = t.visuals.WaterAlpha_f = 0;
						t.gamevisuals.fWaterWaveAmplitude = t.visuals.fWaterWaveAmplitude = 20;
						t.gamevisuals.fWaterWindDependency = t.visuals.fWaterWindDependency = 0;
						t.gamevisuals.fWaterPatchLength = t.visuals.fWaterPatchLength = 40;
						t.gamevisuals.fWaterChoppyScale = t.visuals.fWaterChoppyScale = 0;
						t.gamevisuals.WaterFogMinDist = t.visuals.WaterFogMinDist = 0;
						t.gamevisuals.WaterFogMaxDist = t.visuals.WaterFogMaxDist = 1000;
						t.gamevisuals.WaterFogMinAmount = t.visuals.WaterFogMinAmount = 0.2;

						t.visuals.bWaterEnable = true;
						t.gamevisuals.bWaterEnable = t.visuals.bWaterEnable;
						Wicked_Update_Visuals((void *)&t.visuals);

						//Create Level
						ggterrain_extra_params.iProceduralTerrainType = 4;
						ggterrain_global_params.fractal_initial_amplitude = 1.0f;

						t.gamevisuals.bEndableAmbientMusicTrack = t.visuals.bEndableAmbientMusicTrack = true;
						t.visuals.sAmbientMusicTrack = "audiobank\\ambient\\Arctic.wav";
						t.gamevisuals.sAmbientMusicTrack = t.visuals.sAmbientMusicTrack;
						t.gamevisuals.bEnableCombatMusicTrack = t.visuals.bEnableCombatMusicTrack = false;
						t.visuals.sCombatMusicTrack = "";
						t.gamevisuals.sCombatMusicTrack = t.visuals.sCombatMusicTrack;

						ggtrees_global_params.water_dist = 1251.0;
						bTriggerStableY = true;

					}
					if (iSelectedThemeChoice == 4)
					{
						ImVec2 padding = { 0.0, 0.0 };
						const ImRect image_bb((vSelectionDraw - padding), vSelectionDraw + padding + ImVec2(fButSizeX, fButSizeY));
						ImGui::GetCurrentWindow()->DrawList->AddRect(image_bb.Min, image_bb.Max, ImGui::GetColorU32(outline_color), 0.0f, 15, 2.0f);
					}
					if (ImGui::IsItemHovered()) ImGui::SetTooltip("Select Arctic Theme");

					//Next line.
					vSelectionDraw = ImGui::GetCurrentWindow()->DC.CursorPos;
					if (ImGui::StyleButton("Canyon", ImVec2(fButSizeX, fButSizeY)) || iRandomThemeChoice == 5)
					{
						if (bRandomizeTimeOfDay && iRandomThemeChoice == 0) iRandomTimeOfDayChoice = (rand() % 7);
						if (iRandomThemeChoice == 0) bSelectRandomSkybox = true;
						iSelectedThemeChoice = 5;

						GGTerrainFile_LoadTerrainData("editors\\biomes\\canyon.dat", false);
						ggterrain_global_params.seed = Random2();
						t.showeditortrees = t.gamevisuals.bEndableTreeDrawing = t.visuals.bEndableTreeDrawing = 1;
						t.showeditorveg = t.gamevisuals.bEndableGrassDrawing = t.visuals.bEndableGrassDrawing = 1;
						t.showeditorterrain = t.gamevisuals.bEndableTerrainDrawing = t.visuals.bEndableTerrainDrawing = 1;

						if (iLastTreeGrassSettings != 5)
						{
							iLastTreeGrassSettings = 5;

							//PE: Default tree and grass setup.
							ggtrees_global_params.paint_tree_bitfield = 15728641; //Canyon
							ggtrees_global_params.paint_scale_random_low = 40.0; //Default Scale Min.
							ggtrees_global_params.paint_scale_random_high = 200.0; //Default Scale Max.
							GGTrees::ggtrees_global_params.hide_until_update = 1;
							GGTrees::ggtrees_global_params.draw_enabled = 0;
							GGTrees::GGTrees_ChangeDensity(66); //Default density.
							gggrass_global_params.paint_type = 15; //Default grass
							gggrass_global_params.paint_density = 100; //Default Density
							gggrass_global_params.paint_material = 0; //Auto
							GGGrass::GGGrass_AddAll();
						}

						//PE: Default water settings.
						t.terrain.waterliney_f = g.gdefaultwaterheight = -104;
						t.gamevisuals.WaterSpeed1 = t.visuals.WaterSpeed1 = 0.06;
						t.gamevisuals.WaterRed_f = t.visuals.WaterRed_f = 1;
						t.gamevisuals.WaterGreen_f = t.visuals.WaterGreen_f = 12;
						t.gamevisuals.WaterBlue_f = t.visuals.WaterBlue_f = 8;
						t.gamevisuals.WaterAlpha_f = t.visuals.WaterAlpha_f = 0;
						t.gamevisuals.fWaterWaveAmplitude = t.visuals.fWaterWaveAmplitude = 20;
						t.gamevisuals.fWaterWindDependency = t.visuals.fWaterWindDependency = 0;
						t.gamevisuals.fWaterPatchLength = t.visuals.fWaterPatchLength = 40;
						t.gamevisuals.fWaterChoppyScale = t.visuals.fWaterChoppyScale = 0;
						t.gamevisuals.WaterFogMinDist = t.visuals.WaterFogMinDist = 0;
						t.gamevisuals.WaterFogMaxDist = t.visuals.WaterFogMaxDist = 2000;
						t.gamevisuals.WaterFogMinAmount = t.visuals.WaterFogMinAmount = 0.089;

						t.visuals.bWaterEnable = true;
						t.gamevisuals.bWaterEnable = t.visuals.bWaterEnable;
						Wicked_Update_Visuals((void *)&t.visuals);

						//Create Level
						ggterrain_extra_params.iProceduralTerrainType = 5;
						ggterrain_global_params.fractal_initial_amplitude = 1.0f;

						t.gamevisuals.bEndableAmbientMusicTrack = t.visuals.bEndableAmbientMusicTrack = true;
						t.visuals.sAmbientMusicTrack = "audiobank\\ambient\\Canyon.wav";
						t.gamevisuals.sAmbientMusicTrack = t.visuals.sAmbientMusicTrack;
						t.gamevisuals.bEnableCombatMusicTrack = t.visuals.bEnableCombatMusicTrack = false;
						t.visuals.sCombatMusicTrack = "";
						t.gamevisuals.sCombatMusicTrack = t.visuals.sCombatMusicTrack;

						ggtrees_global_params.water_dist = 3155.0; //551.0;
						bTriggerStableY = true;

					}
					if (iSelectedThemeChoice == 5)
					{
						ImVec2 padding = { 0.0, 0.0 };
						const ImRect image_bb((vSelectionDraw - padding), vSelectionDraw + padding + ImVec2(fButSizeX, fButSizeY));
						ImGui::GetCurrentWindow()->DrawList->AddRect(image_bb.Min, image_bb.Max, ImGui::GetColorU32(outline_color), 0.0f, 15, 2.0f);
					}
					if (ImGui::IsItemHovered()) ImGui::SetTooltip("Select Canyon Theme");
					ImGui::SameLine();

					vSelectionDraw = ImGui::GetCurrentWindow()->DC.CursorPos;
					if (ImGui::StyleButton("Mountain", ImVec2(fButSizeX, fButSizeY)) || iRandomThemeChoice == 6)
					{
						if (bRandomizeTimeOfDay && iRandomThemeChoice == 0) iRandomTimeOfDayChoice = (rand() % 7);
						if (iRandomThemeChoice == 0) bSelectRandomSkybox = true;
						iSelectedThemeChoice = 6;

						GGTerrainFile_LoadTerrainData("editors\\biomes\\mountain.dat", false);
						ggterrain_global_params.seed = Random2();
						t.showeditortrees = t.gamevisuals.bEndableTreeDrawing = t.visuals.bEndableTreeDrawing = 0;
						t.showeditorveg = t.gamevisuals.bEndableGrassDrawing = t.visuals.bEndableGrassDrawing = 0;
						t.showeditorterrain = t.gamevisuals.bEndableTerrainDrawing = t.visuals.bEndableTerrainDrawing = 1;

						if (iLastTreeGrassSettings != 6)
						{
							iLastTreeGrassSettings = 6;

							//PE: Default tree and grass setup.
							ggtrees_global_params.paint_tree_bitfield = 9673113696; //Mountain
							ggtrees_global_params.paint_scale_random_low = 40.0; //Default Scale Min.
							ggtrees_global_params.paint_scale_random_high = 200.0; //Default Scale Max.
							GGTrees::GGTrees_ChangeDensity(40); //Default density.
							GGTrees::ggtrees_global_params.hide_until_update = 1;
							GGTrees::ggtrees_global_params.draw_enabled = 0;
							gggrass_global_params.paint_type = 1; //Default grass
							gggrass_global_params.paint_density = 100; //Default Density
							gggrass_global_params.paint_material = 0; //Auto
							GGGrass::GGGrass_AddAll();
						}

						//PE: Default water settings.
						t.terrain.waterliney_f = g.gdefaultwaterheight = -500;
						t.gamevisuals.WaterSpeed1 = t.visuals.WaterSpeed1 = 0.06;
						t.gamevisuals.WaterRed_f = t.visuals.WaterRed_f = 9;
						t.gamevisuals.WaterGreen_f = t.visuals.WaterGreen_f = 21;
						t.gamevisuals.WaterBlue_f = t.visuals.WaterBlue_f = 43;
						t.gamevisuals.WaterAlpha_f = t.visuals.WaterAlpha_f = 0;
						t.gamevisuals.fWaterWaveAmplitude = t.visuals.fWaterWaveAmplitude = 20;
						t.gamevisuals.fWaterWindDependency = t.visuals.fWaterWindDependency = 0;
						t.gamevisuals.fWaterPatchLength = t.visuals.fWaterPatchLength = 40;
						t.gamevisuals.fWaterChoppyScale = t.visuals.fWaterChoppyScale = 0;
						t.gamevisuals.WaterFogMinDist = t.visuals.WaterFogMinDist = 0;
						t.gamevisuals.WaterFogMaxDist = t.visuals.WaterFogMaxDist = 11500;
						t.gamevisuals.WaterFogMinAmount = t.visuals.WaterFogMinAmount = 0.25;

						t.visuals.bWaterEnable = true;
						t.gamevisuals.bWaterEnable = t.visuals.bWaterEnable;
						Wicked_Update_Visuals((void *)&t.visuals);

						ggterrain_extra_params.iProceduralTerrainType = 6;
						ggterrain_global_params.fractal_initial_amplitude = 1.0f;

						t.gamevisuals.bEndableAmbientMusicTrack = t.visuals.bEndableAmbientMusicTrack = true;
						t.visuals.sAmbientMusicTrack = "audiobank\\ambient\\Mountain.wav";
						t.gamevisuals.sAmbientMusicTrack = t.visuals.sAmbientMusicTrack;
						t.gamevisuals.bEnableCombatMusicTrack = t.visuals.bEnableCombatMusicTrack = false;
						t.visuals.sCombatMusicTrack = "";
						t.gamevisuals.sCombatMusicTrack = t.visuals.sCombatMusicTrack;

						ggtrees_global_params.water_dist = 400.0;
						bTriggerStableY = true;

					}
					if (iSelectedThemeChoice == 6)
					{
						ImVec2 padding = { 0.0, 0.0 };
						const ImRect image_bb((vSelectionDraw - padding), vSelectionDraw + padding + ImVec2(fButSizeX, fButSizeY));
						ImGui::GetCurrentWindow()->DrawList->AddRect(image_bb.Min, image_bb.Max, ImGui::GetColorU32(outline_color), 0.0f, 15, 2.0f);
					}
					if (ImGui::IsItemHovered()) ImGui::SetTooltip("Select Mountain Theme");
					ImGui::SameLine();

					vSelectionDraw = ImGui::GetCurrentWindow()->DC.CursorPos;
					if (ImGui::StyleButton("Rainforest", ImVec2(fButSizeX, fButSizeY)) || iRandomThemeChoice == 7)
					{
						if (bRandomizeTimeOfDay && iRandomThemeChoice == 0) iRandomTimeOfDayChoice = (rand() % 7);
						if (iRandomThemeChoice == 0) bSelectRandomSkybox = true;
						iSelectedThemeChoice = 7;

						GGTerrainFile_LoadTerrainData("editors\\biomes\\rainforest.dat", false);
						ggterrain_global_params.seed = Random2();
						t.showeditortrees = t.gamevisuals.bEndableTreeDrawing = t.visuals.bEndableTreeDrawing = 1;
						t.showeditorveg = t.gamevisuals.bEndableGrassDrawing = t.visuals.bEndableGrassDrawing = 1;
						t.showeditorterrain = t.gamevisuals.bEndableTerrainDrawing = t.visuals.bEndableTerrainDrawing = 1;

						if (iLastTreeGrassSettings != 7)
						{
							iLastTreeGrassSettings = 7;
							//PE: Default tree and grass setup.
							ggtrees_global_params.paint_tree_bitfield = 6443498496; //rain forest.
							ggtrees_global_params.paint_scale_random_low = 40.0; //Default Scale Min.
							ggtrees_global_params.paint_scale_random_high = 200.0; //Default Scale Max.
							GGTrees::GGTrees_ChangeDensity(90); //Default density.
							GGTrees::ggtrees_global_params.hide_until_update = 1;
							GGTrees::ggtrees_global_params.draw_enabled = 0;
							gggrass_global_params.paint_type = 15; //Default grass
							gggrass_global_params.paint_density = 100; //Default Density
							gggrass_global_params.paint_material = 0; //Auto
							GGGrass::GGGrass_AddAll();
						}

						//PE: Default water settings.
						t.terrain.waterliney_f = g.gdefaultwaterheight = 104;
						t.gamevisuals.WaterSpeed1 = t.visuals.WaterSpeed1 = 0.06;
						t.gamevisuals.WaterRed_f = t.visuals.WaterRed_f = 12;
						t.gamevisuals.WaterGreen_f = t.visuals.WaterGreen_f = 8;
						t.gamevisuals.WaterBlue_f = t.visuals.WaterBlue_f = 0;
						t.gamevisuals.WaterAlpha_f = t.visuals.WaterAlpha_f = 0;
						t.gamevisuals.fWaterWaveAmplitude = t.visuals.fWaterWaveAmplitude = 20;
						t.gamevisuals.fWaterWindDependency = t.visuals.fWaterWindDependency = 0;
						t.gamevisuals.fWaterPatchLength = t.visuals.fWaterPatchLength = 40;
						t.gamevisuals.fWaterChoppyScale = t.visuals.fWaterChoppyScale = 0;
						t.gamevisuals.WaterFogMinDist = t.visuals.WaterFogMinDist = 0;
						t.gamevisuals.WaterFogMaxDist = t.visuals.WaterFogMaxDist = 1000;
						t.gamevisuals.WaterFogMinAmount = t.visuals.WaterFogMinAmount = 0.25;

						t.visuals.bWaterEnable = true;
						t.gamevisuals.bWaterEnable = t.visuals.bWaterEnable;
						Wicked_Update_Visuals((void *)&t.visuals);

						//Create Level
						ggterrain_extra_params.iProceduralTerrainType = 7;
						ggterrain_global_params.fractal_initial_amplitude = 1.0f;

						t.gamevisuals.bEndableAmbientMusicTrack = t.visuals.bEndableAmbientMusicTrack = true;
						t.visuals.sAmbientMusicTrack = "audiobank\\ambient\\Rainforest.wav";
						t.gamevisuals.sAmbientMusicTrack = t.visuals.sAmbientMusicTrack;
						t.gamevisuals.bEnableCombatMusicTrack = t.visuals.bEnableCombatMusicTrack = false;
						t.visuals.sCombatMusicTrack = "";
						t.gamevisuals.sCombatMusicTrack = t.visuals.sCombatMusicTrack;

						ggtrees_global_params.water_dist = 212.0; //22.0;
						bTriggerStableY = true;

					}
					if (iSelectedThemeChoice == 7)
					{
						ImVec2 padding = { 0.0, 0.0 };
						const ImRect image_bb((vSelectionDraw - padding), vSelectionDraw + padding + ImVec2(fButSizeX, fButSizeY));
						ImGui::GetCurrentWindow()->DrawList->AddRect(image_bb.Min, image_bb.Max, ImGui::GetColorU32(outline_color), 0.0f, 15, 2.0f);
					}
					if (ImGui::IsItemHovered()) ImGui::SetTooltip("Select Rainforest Theme");
					ImGui::SameLine();

					vSelectionDraw = ImGui::GetCurrentWindow()->DC.CursorPos;
					if (ImGui::StyleButton("Empty", ImVec2(fButSizeX, fButSizeY)))
					{
						if (bRandomizeTimeOfDay && iRandomThemeChoice == 0) iRandomTimeOfDayChoice = (rand() % 7);
						bSelectNightSkybox = true;
						iSelectedThemeChoice = 8;

						t.showeditortrees = t.gamevisuals.bEndableTreeDrawing = t.visuals.bEndableTreeDrawing = 0;
						t.showeditorveg = t.gamevisuals.bEndableGrassDrawing = t.visuals.bEndableGrassDrawing = 0;
						t.showeditorterrain = t.gamevisuals.bEndableTerrainDrawing = t.visuals.bEndableTerrainDrawing = 1;
						t.gamevisuals.bEnableEmptyLevelMode = t.visuals.bEnableEmptyLevelMode = false;
						t.gamevisuals.bEnableZeroNavMeshMode = t.visuals.bEnableZeroNavMeshMode = false;

						//PE: Default tree and grass setup.
						if (iLastTreeGrassSettings != 0)
						{
							iLastTreeGrassSettings = 0;
							ggtrees_global_params.paint_tree_bitfield = 1; //Default
							ggtrees_global_params.paint_scale_random_low = 40.0; //Default Scale Min.
							ggtrees_global_params.paint_scale_random_high = 200.0; //Default Scale Max.
							GGTrees::ggtrees_global_params.hide_until_update = 1;
							GGTrees::ggtrees_global_params.draw_enabled = 0;
							GGTrees::GGTrees_ChangeDensity(65); //Default density.
							gggrass_global_params.paint_type = 1; //Default grass
							gggrass_global_params.paint_material = 0; //Auto
							gggrass_global_params.paint_density = 100; //Default Density
							GGGrass::GGGrass_AddAll();
						}

						//PE: Default water settings.
						t.gamevisuals.bWaterEnable = t.visuals.bWaterEnable = false;
						t.terrain.waterliney_f = g.gdefaultwaterheight = -500;
						t.gamevisuals.WaterSpeed1 = t.visuals.WaterSpeed1 = 0.06;
						t.gamevisuals.WaterRed_f = t.visuals.WaterRed_f = 9;
						t.gamevisuals.WaterGreen_f = t.visuals.WaterGreen_f = 21;
						t.gamevisuals.WaterBlue_f = t.visuals.WaterBlue_f = 43;
						t.gamevisuals.WaterAlpha_f = t.visuals.WaterAlpha_f = 0;
						t.gamevisuals.fWaterWaveAmplitude = t.visuals.fWaterWaveAmplitude = 20;
						t.gamevisuals.fWaterWindDependency = t.visuals.fWaterWindDependency = 0;
						t.gamevisuals.fWaterPatchLength = t.visuals.fWaterPatchLength = 40;
						t.gamevisuals.fWaterChoppyScale = t.visuals.fWaterChoppyScale = 0;
						t.gamevisuals.WaterFogMinDist = t.visuals.WaterFogMinDist = 0;
						t.gamevisuals.WaterFogMaxDist = t.visuals.WaterFogMaxDist = 11500;
						t.gamevisuals.WaterFogMinAmount = t.visuals.WaterFogMinAmount = 0.25;

						procedural_set_empty_level(true);

						t.gamevisuals.bEndableAmbientMusicTrack = t.visuals.bEndableAmbientMusicTrack = false;
						t.visuals.sAmbientMusicTrack = "";
						t.gamevisuals.sAmbientMusicTrack = t.visuals.sAmbientMusicTrack;
						t.gamevisuals.bEnableCombatMusicTrack = t.visuals.bEnableCombatMusicTrack = false;
						t.visuals.sCombatMusicTrack = "";
						t.gamevisuals.sCombatMusicTrack = t.visuals.sCombatMusicTrack;

						ggtrees_global_params.water_dist = 400.0;
						bTriggerStableY = true;
					}
					if (iSelectedThemeChoice == 8)
					{
						ImVec2 padding = { 0.0, 0.0 };
						const ImRect image_bb((vSelectionDraw - padding), vSelectionDraw + padding + ImVec2(fButSizeX, fButSizeY));
						ImGui::GetCurrentWindow()->DrawList->AddRect(image_bb.Min, image_bb.Max, ImGui::GetColorU32(outline_color), 0.0f, 15, 2.0f);
					}
					if (ImGui::IsItemHovered()) ImGui::SetTooltip("Select a Empty Level consisting of a blank grey grid");

					// for all stock default biomes, all use stock terrain textures, so remove any custom ones if selected
					if (iSelectedThemeChoice < 10)
					{
						if (t.visuals.customTexturesFolder.Len() > 0)
						{
							t.visuals.customTexturesFolder = "";
							ChooseTerrainTextureFolder("");
						}
					}

					// optional custom biomes can go here
					static int g_custombiome_item_count = 0;
					static char** g_custombiome_items = NULL;
					static char g_pCustomBiomeName[256];
					if (g_custombiome_item_count != g_sCustomBiomes.size())
					{
						if (g_custombiome_items)
						{
							for (int i = 0; i < g_custombiome_item_count; i++) SAFE_DELETE(g_custombiome_items[i]);
							SAFE_DELETE(g_custombiome_items);
						}
						g_custombiome_item_count = g_sCustomBiomes.size();
						g_custombiome_items = new char* [g_custombiome_item_count];
						for (int i = 0; i < g_custombiome_item_count; i++)
						{
							g_custombiome_items[i] = new char[256];
							strcpy(g_custombiome_items[i], g_sCustomBiomes[i].pName);
						}
					}
					int g_custombiome_selection = 0;
					if (iSelectedThemeChoice < 10) strcpy(g_pCustomBiomeName, "");
					for (int i = 0; i < g_custombiome_item_count; i++)
					{
						if (pestrcasestr(g_pCustomBiomeName, g_custombiome_items[i]))
						{
							g_custombiome_selection = i;
							break;
						}
					}
					if (g_custombiome_item_count>0)
					{
						ImGui::TextCenter("Custom Choices");

						int iCustomThemeIDStartsAt = 10;
						vSelectionDraw = ImGui::GetCurrentWindow()->DC.CursorPos;
						if (ImGui::Combo("##ComboCustomBiomes", &g_custombiome_selection, g_custombiome_items, g_custombiome_item_count))
						{
							// set this selection
							iSelectedThemeChoice = iCustomThemeIDStartsAt + g_custombiome_selection;
							strcpy(g_pCustomBiomeName, g_custombiome_items[g_custombiome_selection]);
							sCustomBiomeType item = g_sCustomBiomes[g_custombiome_selection];
							
							// do we randomise on selection
							if (item.randomizetimeofday==1 && iRandomThemeChoice == 0) iRandomTimeOfDayChoice = (rand() % 7);
							if (iRandomThemeChoice == 0) bSelectRandomSkybox = true;
							iSelectedThemeChoice = iCustomThemeIDStartsAt;

							// load in stock settings for this biome
							char pSettingsFile[MAX_PATH];
							sprintf(pSettingsFile, "editors\\biomes\\custom\\%s\\settings.ter", g_pCustomBiomeName);
							GGTerrainFile_LoadTerrainData(pSettingsFile, false);

							// starting seed
							//ggterrain_global_params.seed = Random2();

							// toggle trees, grass and terrain drawing
							t.showeditortrees = t.gamevisuals.bEndableTreeDrawing = t.visuals.bEndableTreeDrawing = item.showtrees;
							t.showeditorveg = t.gamevisuals.bEndableGrassDrawing = t.visuals.bEndableGrassDrawing = item.showgrass;
							t.showeditorterrain = t.gamevisuals.bEndableTerrainDrawing = t.visuals.bEndableTerrainDrawing = item.showterrain;

							// if switch biome, update trees and grass
							if (iLastTreeGrassSettings != iCustomThemeIDStartsAt)
							{
								iLastTreeGrassSettings = iCustomThemeIDStartsAt;
								ggtrees_global_params.paint_tree_bitfield = item.treesbitfield;
								ggtrees_global_params.paint_scale_random_low = item.treesscalerandomlow;
								ggtrees_global_params.paint_scale_random_high = item.treesscalerandomhigh;
								GGTrees::GGTrees_ChangeDensity(item.treeschangedensity);
								GGTrees::ggtrees_global_params.hide_until_update = 1;
								GGTrees::ggtrees_global_params.draw_enabled = 0;
								gggrass_global_params.paint_type = item.grasspainttype;
								gggrass_global_params.paint_density = item.grasspaintdensity;
								gggrass_global_params.paint_material = item.grasspaintmaterial;
								GGGrass::GGGrass_AddAll();
							}

							// Water settings
							t.terrain.waterliney_f = g.gdefaultwaterheight = item.waterline;
							t.gamevisuals.WaterSpeed1 = t.visuals.WaterSpeed1 = item.waterspeed / 100.0f;
							t.gamevisuals.WaterRed_f = t.visuals.WaterRed_f = item.waterred;
							t.gamevisuals.WaterGreen_f = t.visuals.WaterGreen_f = item.watergreen;
							t.gamevisuals.WaterBlue_f = t.visuals.WaterBlue_f = item.waterblue;
							t.gamevisuals.WaterAlpha_f = t.visuals.WaterAlpha_f = item.wateralpha;
							t.gamevisuals.fWaterWaveAmplitude = t.visuals.fWaterWaveAmplitude = item.waterwaveamplitude;
							t.gamevisuals.fWaterWindDependency = t.visuals.fWaterWindDependency = item.waterwinddependency;
							t.gamevisuals.fWaterPatchLength = t.visuals.fWaterPatchLength = item.waterpatchlength;
							t.gamevisuals.fWaterChoppyScale = t.visuals.fWaterChoppyScale = item.waterchoppyscale;
							t.gamevisuals.WaterFogMinDist = t.visuals.WaterFogMinDist = item.waterfogmindist;
							t.gamevisuals.WaterFogMaxDist = t.visuals.WaterFogMaxDist = item.waterfogmaxdist;
							t.gamevisuals.WaterFogMinAmount = t.visuals.WaterFogMinAmount = item.waterfogminamount / 100.0f;
							t.visuals.bWaterEnable = item.waterenable = item.waterenable;
							t.gamevisuals.bWaterEnable = t.visuals.bWaterEnable;

							// update visuals once settings made
							Wicked_Update_Visuals((void*)&t.visuals);

							// more terrain settings
							ggterrain_extra_params.iProceduralTerrainType = item.proceduralterraintype;

							// set atmospheric loop
							char pAtmosLoopFile[MAX_PATH];
							sprintf(pAtmosLoopFile, "editors\\biomes\\custom\\%s\\atmosloop.wav", g_pCustomBiomeName);
							t.visuals.sAmbientMusicTrack = pAtmosLoopFile;
							t.gamevisuals.bEndableAmbientMusicTrack = t.visuals.bEndableAmbientMusicTrack = true;
							t.gamevisuals.sAmbientMusicTrack = t.visuals.sAmbientMusicTrack;
							t.gamevisuals.bEnableCombatMusicTrack = t.visuals.bEnableCombatMusicTrack = false;
							t.visuals.sCombatMusicTrack = "";
							t.gamevisuals.sCombatMusicTrack = t.visuals.sCombatMusicTrack;

							// set water distance
							ggtrees_global_params.water_dist = item.waterdistance;

							// and also change terrain texture folder to custom one
							char pCustomTerrainTextureFolder[MAX_PATH];
							sprintf(pCustomTerrainTextureFolder, "editors\\biomes\\custom\\%s\\textures\\", g_pCustomBiomeName);
							t.visuals.customTexturesFolder = pCustomTerrainTextureFolder;
							char pCustomTerrainMaterialSoundFile[MAX_PATH];
							sprintf(pCustomTerrainMaterialSoundFile, "editors\\biomes\\custom\\%s\\matsounds.txt", g_pCustomBiomeName);
							extern void physics_loadmaterialsoundsintomapmat(LPSTR);
							physics_loadmaterialsoundsintomapmat (pCustomTerrainMaterialSoundFile);
							extern void physics_copymatmaptocustommat(void);
							physics_copymatmaptocustommat();

							// then save custom material settings in FPM of terrain generation so both terrain textures and material sounds are respected
							cstr terrainMaterialFile = g.mysystem.levelBankTestMap_s + "custommaterials.dat";
							SaveTerrainTextureFolder(terrainMaterialFile.Get());

							// and then trigger the texture to be loaded right now in the Terrain Generator
							ChooseTerrainTextureFolder(t.visuals.customTexturesFolder.Get());

							// biome settings complete
							bTriggerStableY = true;
						}
						if (iSelectedThemeChoice == iCustomThemeIDStartsAt)
						{
							ImVec2 padding = { 0.0, 0.0 };
							const ImRect image_bb((vSelectionDraw - padding), vSelectionDraw + padding + ImVec2(230, 22));
							ImGui::GetCurrentWindow()->DrawList->AddRect(image_bb.Min, image_bb.Max, ImGui::GetColorU32(outline_color), 0.0f, 15, 2.0f);
						}
						if (ImGui::IsItemHovered()) ImGui::SetTooltip("Select Custom Biome Theme");
					}
					ggterrain_global_render_params2.editable_size = feditable_size;
					ImGui::PopItemWidth();
				}
				if (iSelectedThemeChoice == 8)
				{
					if (ImGui::StyleCollapsingHeader("Disable Level Aspects", ImGuiTreeNodeFlags_DefaultOpen))
					{
						bool bCompletelyEmpty = t.gamevisuals.bEnableEmptyLevelMode;
						if (ImGui::Checkbox("Completely Empty Level", &bCompletelyEmpty))
						{
							if (bCompletelyEmpty == true)
							{
								// everything off
								t.gamevisuals.bEndableTreeDrawing = t.visuals.bEndableTreeDrawing = 0;
								t.gamevisuals.bEndableGrassDrawing = t.visuals.bEndableGrassDrawing = 0;
								t.showeditorterrain = t.gamevisuals.bEndableTerrainDrawing = t.visuals.bEndableTerrainDrawing = 0;
								t.showeditorwater = t.gamevisuals.bWaterEnable = t.visuals.bWaterEnable = false;
								t.showeditortrees = ggtrees_global_params.draw_enabled = false;
								t.showeditorveg = gggrass_global_params.draw_enabled = false;

								// make universe massive and hide all markings
								float fFiftyKilometersForSpaceGames = 50.0f;
								static float fNewEditasbleSize = GGTerrain_MetersToUnits(fFiftyKilometersForSpaceGames / 2.0);
								ggterrain_global_render_params2.editable_size = fNewEditasbleSize * 1000.0f;
								bShowEditArea = false; ggterrain_global_render_params2.flags2 &= ~GGTERRAIN_SHADER_FLAG2_SHOW_MAP_SIZE;

								// master flag to activate completely empty mode
								t.gamevisuals.bEnableEmptyLevelMode = t.visuals.bEnableEmptyLevelMode = true;
							}
							else
							{
								t.showeditorterrain = t.gamevisuals.bEndableTerrainDrawing = t.visuals.bEndableTerrainDrawing = 1;
								t.gamevisuals.bEnableEmptyLevelMode = t.visuals.bEnableEmptyLevelMode = false;
							}
							Wicked_Update_Visuals((void*)&t.visuals);
						}
						if (ImGui::IsItemHovered()) ImGui::SetTooltip("Additionally removes terrain, water and related defaults");

						bool bZeroNavMeshMode = t.gamevisuals.bEnableZeroNavMeshMode;
						if (ImGui::Checkbox("Do Not Generate Navmesh", &bZeroNavMeshMode))
						{
							if (bZeroNavMeshMode == true)
								t.gamevisuals.bEnableZeroNavMeshMode = t.visuals.bEnableZeroNavMeshMode = true;
							else
								t.gamevisuals.bEnableZeroNavMeshMode = t.visuals.bEnableZeroNavMeshMode = false;
						}
						if (ImGui::IsItemHovered()) ImGui::SetTooltip("Additionally removes navmesh generation (system used to detect walkable areas)");
					}
				}
				if(t.visuals.bEnableEmptyLevelMode==false)
				{
					if (ImGui::StyleCollapsingHeader("Terrain Size", ImGuiTreeNodeFlags_DefaultOpen))
					{
						ImGui::Indent(10);

						ImGui::TextCenter("Editable Area Size");
						float numericboxwidth = 60.0f;

						ImGui::PushItemWidth(-10 - 10 - numericboxwidth);

						float fTmp = GGTerrain_UnitsToMeters(ggterrain_global_render_params2.editable_size * 2.0) / 1000.0f;
						if (ImGui::SliderFloat("##UI2TerrainEditableSizeKilometers", &fTmp, 0.5, 5.0f, " "))
						{
							//Cam zoom. 420000 = 5.0 , 50000 = 0.5 , perhaps 48000 per 0.5
							ggterrain_global_render_params2.editable_size = GGTerrain_MetersToUnits(fTmp / 2.0) * 1000.0f;

							//Reset camera to point at center of edit area.
							if (fSnapShotModeCameraY < (fTmp * 41000.0f))
							{
								fSnapShotModeCameraY = fTmp * 41000.0f;
								if (fSnapShotModeCameraY > 344000) fSnapShotModeCameraY = 344000; //Hide ugly shadow for now.
								fSnapShotModeCameraX = GGORIGIN_X; // +ggterrain_global_params.offset_x; It dont actual move from center.
								fSnapShotModeCameraZ = GGORIGIN_Z; // +ggterrain_global_params.offset_z;
								fSnapShotModeCameraAngZ = fSnapShotModeCameraAngY = 0.0f;
								fSnapShotModeCameraAngX = 90.0f; //Look down.
							}
						}
						if (ImGui::IsItemHovered()) ImGui::SetTooltip("Set Editable Area Size in Kilometers.");
						ImGui::PopItemWidth();
						ImGui::SameLine();
						ImGui::PushItemWidth(numericboxwidth);
						if (ImGui::InputFloat("##UI2TerrainEditableSizeKilometersText", &fTmp, 0, 0, "%.1f Km"))
						{
							ggterrain_global_render_params2.editable_size = GGTerrain_MetersToUnits(fTmp / 2.0) * 1000.0f;
							fSnapShotModeCameraY = fTmp * 48000.0f;
						}
						if (!pref.iTurnOffEditboxTooltip && ImGui::IsItemHovered()) ImGui::SetTooltip("Set Editable Area Size in Kilometers.");

						ImGui::PopItemWidth();

						if (pref.iTerrainAdvanced)
						{
							if (ImGui::Checkbox("Show Editable Area", &bShowEditArea))
							{
							}
							ImGui::SameLine();

							bShow3DBoundary = (ggterrain_global_render_params2.flags2 & GGTERRAIN_SHADER_FLAG2_SHOW_MAP_SIZE_3D) != 0;
							if (ImGui::Checkbox("3D Boundary##bShow3DBoundary", &bShow3DBoundary))
							{
								if (bShow3DBoundary) ggterrain_global_render_params2.flags2 |= GGTERRAIN_SHADER_FLAG2_SHOW_MAP_SIZE_3D;
								else ggterrain_global_render_params2.flags2 &= ~GGTERRAIN_SHADER_FLAG2_SHOW_MAP_SIZE_3D;
							}
						}
						#ifndef NOMINIMAP
						bShowMiniMap = (ggterrain_global_render_params2.flags2 & GGTERRAIN_SHADER_FLAG2_SHOW_MINI_MAP) != 0;
						if (ImGui::Checkbox("Show Mini Map##bShowMiniMap", &bShowMiniMap))
						{
							if (bShowMiniMap) ggterrain_global_render_params2.flags2 |= GGTERRAIN_SHADER_FLAG2_SHOW_MINI_MAP;
							else ggterrain_global_render_params2.flags2 &= ~GGTERRAIN_SHADER_FLAG2_SHOW_MINI_MAP;
						}
						#endif
						ImGui::Indent(-10);
					}

					// allow terrain to be changed except for EMPTY mode
					if (ggterrain_extra_params.iProceduralTerrainType != 0)
					{
						if (ImGui::StyleCollapsingHeader("Terrain Attributes", ImGuiTreeNodeFlags_DefaultOpen))
						{
							ImGui::Indent(10);

							ImGui::TextCenter("Height Range");

							//Height Range
							//FYI: fTerrainHeightStart is missing in ggterrain_global_params.
							ImGui::PushItemWidth(-10 - 10 - 60);
							ImGui::TextCenter("Max Height (meters)");
							float meterValue = GGTerrain_UnitsToMeters(ggterrain_global_params.height);
							if(ImGui::MaxSliderInputFloatPower("##HeightRange", &meterValue, 0.0f, 1000.0f, 0, 0, 1000, 60, 2.0f, 2 ))
							{
								ggterrain_global_params.height = GGTerrain_MetersToUnits(meterValue);
								bTriggerStableY = true;
							}

							if (pref.iTerrainAdvanced)
							{
								ImGui::TextCenter("Valley Depth (meters)");
								meterValue = GGTerrain_UnitsToMeters(ggterrain_global_params.minHeight);
								if(ImGui::MaxSliderInputFloatPower("##MinHeightRange", &meterValue, 0.0f, 1000.0f, 0, 0, 1000, 60, 2.0f, 2 ))
								ggterrain_global_params.minHeight = GGTerrain_MetersToUnits(meterValue);
								bTriggerStableY = true;
							}
							ImGui::PopItemWidth();

							//Water Height
							//One meter = 39.3701 inch.
							ImGui::TextCenter("Water Height (meters)");
							float numericboxwidth = 60.0f;
							ImGui::PushItemWidth(-10 - 10 - numericboxwidth);
							float waterHeight = GGTerrain_UnitsToMeters(g.gdefaultwaterheight);
							if (ImGui::SliderFloat("##UI2fWaterHeightMeters", &waterHeight, -500.0, 1500.0f, "%.1f", 2.0f))
							{
								g.gdefaultwaterheight = (int)GGTerrain_MetersToUnits(waterHeight);
								t.terrain.waterliney_f = (float)g.gdefaultwaterheight;
								Wicked_Update_Visuals((void *)&t.visuals);
								ggterrain_extra_params.iUpdateTrees = 1;
								bTriggerStableY = true;
								fLastY = -1; //Trigger fog update.
							}
							if (ImGui::IsItemHovered()) ImGui::SetTooltip("Set Water Height in Meters.");
							ImGui::PopItemWidth();
							ImGui::SameLine();
							ImGui::PushItemWidth(numericboxwidth);
							if (ImGui::InputFloat("##UI2fWaterHeightMetersText", &waterHeight, 0, 0, "%.1f M"))
							{
								g.gdefaultwaterheight = (int)GGTerrain_MetersToUnits(waterHeight);
								t.terrain.waterliney_f = (float)g.gdefaultwaterheight;
								Wicked_Update_Visuals((void *)&t.visuals);
								ggterrain_extra_params.iUpdateTrees = 1;
								bTriggerStableY = true;
								fLastY = -1; //Trigger fog update.

							}
							if (!pref.iTurnOffEditboxTooltip && ImGui::IsItemHovered()) ImGui::SetTooltip("Set Water Height in Meters.");
							ImGui::PopItemWidth();

							ImGui::TextCenter("Time of Day");

							const char* time_combo[] = { "Dawn", "Morning", "Midday","Afternoon", "Evening", "Dusk","Night" ,"Skybox: Fixed Time of Day" };
							ImGui::PushItemWidth(-10);

							int iChoises = IM_ARRAYSIZE(time_combo) - 1;
							int iSelection = t.visuals.iTimeOfday;
							bool bReadOnlyMode = !t.visuals.bDisableSkybox;
							if (t.visuals.skyindex == 0 && t.visuals.bDisableSkybox == false) bReadOnlyMode = false; //PE: Dynamic.
							if (bReadOnlyMode) iChoises++;
							if (bReadOnlyMode) iSelection = 7;

							if (!bReadOnlyMode)
							{
								ImGui::Checkbox("Random Time of Day", &bRandomizeTimeOfDay);
							}

							if (bReadOnlyMode)
							{
								ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
								ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
							}

							if (ImGui::Combo("##ComboTimeOfDay2", &iSelection, time_combo, iChoises) || iRandomTimeOfDayChoice >= 0)
							{
								t.visuals.iTimeOfday = iSelection;
								static int iLastRandomTimeOfDayChoice = -1;
								if (iRandomTimeOfDayChoice >= 0)
								{
									if (iLastRandomTimeOfDayChoice == iRandomTimeOfDayChoice)
									{
										iRandomTimeOfDayChoice++;
										if (iLastRandomTimeOfDayChoice == 6) iRandomTimeOfDayChoice = 0;
									}
								}
								else
								{
									//PE: Disable randomize when user make a selection.
									iLastUserSelectedTimeOfDay = iSelection;
									bRandomizeTimeOfDay = false;
								}
								if (iRandomTimeOfDayChoice > 6) iRandomTimeOfDayChoice = 6;

								if (bFirstBiomes[iSelectedThemeChoice])
								{
									bFirstBiomes[iSelectedThemeChoice] = false;
									bSelectDayRandomSkybox = true;
									//Select a day time.
									if ((rand() % 3) == 1)
										iRandomTimeOfDayChoice = 1;
									else if ((rand() % 3) == 1)
										iRandomTimeOfDayChoice = 4;
									else
										iRandomTimeOfDayChoice = 3;
								}

								if(iRandomTimeOfDayChoice >=0 )
									t.gamevisuals.iTimeOfday = t.visuals.iTimeOfday = iRandomTimeOfDayChoice;
								else
									t.gamevisuals.iTimeOfday = t.visuals.iTimeOfday;
								iLastRandomTimeOfDayChoice = iRandomTimeOfDayChoice;
								visuals_calcsunanglefromtimeofday(t.gamevisuals.iTimeOfday, &t.gamevisuals.SunAngleX, &t.gamevisuals.SunAngleY, &t.gamevisuals.SunAngleZ);
								t.visuals.SunAngleX = t.gamevisuals.SunAngleX;
								t.visuals.SunAngleY = t.gamevisuals.SunAngleY;
								t.visuals.SunAngleZ = t.gamevisuals.SunAngleZ;

								oldSunAngleX = t.visuals.SunAngleX; //PE: Make sure to update after save.
								oldSunAngleY = t.visuals.SunAngleY;
								oldSunAngleZ = t.visuals.SunAngleZ;

								Wicked_Update_Visuals((void *)&t.visuals);
								bTriggerStableY = true;
								fLastY = -1; //Trigger fog update.
							}

							if (bReadOnlyMode)
							{
								ImGui::PopItemFlag();
								ImGui::PopStyleVar();
							}

							if (ImGui::IsItemHovered()) ImGui::SetTooltip("This sets the sun at the correct position for the time of day");
							ImGui::PopItemWidth();


							//Advanced start here!

							extern void ControlAdvancedSetting(int&, const char*, bool* = nullptr);
							bool bStateUnchanged = true;
							ControlAdvancedSetting(pref.iTerrainAdvanced, "advanced terrain tools", &bStateUnchanged);

							if (pref.iTerrainAdvanced)
							{
								cstr cSpecialTooltip = "";
								numericboxwidth = 60.0f;

								//ggterrain_global_params.noise_power
								ImGui::ImgBtn(ICON_INFO, ImVec2(ImGui::GetFontSize(), ImGui::GetFontSize()), ImColor(0, 0, 0, 0), ImColor(220, 220, 220, 220), ImColor(255, 255, 255, 255), ImColor(180, 180, 160, 255), -1, 0, 0, 0, false, false, false, false, false);
								if (ImGui::IsItemHovered())
								{
									cSpecialTooltip = "Values above 1 make lower areas flatter, \nvalues less than 1 make higher areas flatter, \na value of 1 does not modify the noise value";
								}
								ImGui::SameLine(); ImGui::SetCursorPosX(10);
								ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 3.0);
								ImGui::TextCenter("Noise Curve");
								ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 6.0);
								ImGui::PushItemWidth(-10 - 10 - numericboxwidth);
								if (ImGui::SliderFloat("##UI2fNoisePower", &ggterrain_global_params.noise_power, 0.0f, 10.0f, " "))
								{
									bTriggerStableY = true;
								}
								if (ImGui::IsItemHovered()) ImGui::SetTooltip("Set Noise Curve.");
								ImGui::PopItemWidth();
								ImGui::SameLine();
								ImGui::PushItemWidth(numericboxwidth);
								if (ImGui::InputFloat("##UI2fNoisePowerText", &ggterrain_global_params.noise_power, 0, 0, "%.1f"))
								{
									bTriggerStableY = true;
								}
								if (!pref.iTurnOffEditboxTooltip && ImGui::IsItemHovered()) ImGui::SetTooltip("Set Noise Curve.");
								ImGui::PopItemWidth();

								ImGui::ImgBtn(ICON_INFO, ImVec2(ImGui::GetFontSize(), ImGui::GetFontSize()), ImColor(0, 0, 0, 0), ImColor(220, 220, 220, 220), ImColor(255, 255, 255, 255), ImColor(180, 180, 160, 255), -1, 0, 0, 0, false, false, false, false, false);
								if (ImGui::IsItemHovered())
								{
									cSpecialTooltip = "A value of 0 does not modify the noise value, \nvalues greater than 0 make lower areas smoother";
								}
								ImGui::SameLine(); ImGui::SetCursorPosX(10);
								ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 3.0);
								ImGui::TextCenter("Noise Falloff");
								ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 6.0);
								ImGui::PushItemWidth(-10 - 10 - numericboxwidth);
								if (ImGui::SliderFloat("##UI2fNoiseFalloffPower", &ggterrain_global_params.noise_fallof_power, 0.0f, 10.0f, " "))
								{
									bTriggerStableY = true;
								}
								if (ImGui::IsItemHovered()) ImGui::SetTooltip("Set Noise Falloff.");
								ImGui::PopItemWidth();
								ImGui::SameLine();
								ImGui::PushItemWidth(numericboxwidth);
								if (ImGui::InputFloat("##UI2fNoiseFalloffPowerText", &ggterrain_global_params.noise_fallof_power, 0, 0, "%.1f"))
								{
									bTriggerStableY = true;
								}
								if (!pref.iTurnOffEditboxTooltip && ImGui::IsItemHovered()) ImGui::SetTooltip("Set Noise Falloff.");
								ImGui::PopItemWidth();

								ImGui::ImgBtn(ICON_INFO, ImVec2(ImGui::GetFontSize(), ImGui::GetFontSize()), ImColor(0, 0, 0, 0), ImColor(220, 220, 220, 220), ImColor(255, 255, 255, 255), ImColor(180, 180, 160, 255), -1, 0, 0, 0, false, false, false, false, false);
								if (ImGui::IsItemHovered())
								{
									cSpecialTooltip = "The number of iterations of noise to use which get \nlayered on top of each other, the \nhigher the value the more bumpy the \nterrain will be, good \nvalues are around 10";
								}
								ImGui::SameLine(); ImGui::SetCursorPosX(10);
								ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 3.0);
								ImGui::TextCenter("Noise Iterations");
								ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 6.0);
								ImGui::PushItemWidth(-10 - 10 - numericboxwidth);
								float fTmp = ggterrain_global_params.fractal_levels;
								if (ImGui::SliderFloat("##UI2fFractalIterations", &fTmp, 1.0f, 14.0f, " "))
								{
									ggterrain_global_params.fractal_levels = int(fTmp);
									bTriggerStableY = true;
								}
								if (ImGui::IsItemHovered()) ImGui::SetTooltip("Set Noise Iterations.");
								ImGui::PopItemWidth();
								ImGui::SameLine();
								ImGui::PushItemWidth(numericboxwidth);
								if (ImGui::InputFloat("##UI2fFractalIterationsText", &fTmp, 0, 0, "%.0f"))
								{
									ggterrain_global_params.fractal_levels = int(fTmp);
									bTriggerStableY = true;
								}
								if (!pref.iTurnOffEditboxTooltip && ImGui::IsItemHovered()) ImGui::SetTooltip("Set Noise Iterations.");
								ImGui::PopItemWidth();

								ImGui::ImgBtn(ICON_INFO, ImVec2(ImGui::GetFontSize(), ImGui::GetFontSize()), ImColor(0, 0, 0, 0), ImColor(220, 220, 220, 220), ImColor(255, 255, 255, 255), ImColor(180, 180, 160, 255), -1, 0, 0, 0, false, false, false, false, false);
								if (ImGui::IsItemHovered())
								{
									cSpecialTooltip = "The frequency of the first layer of noise, \nlarger values will have smaller terrain features, \nsmaller values will have larger terrain features. \nSmaller values often require a larger \nheight value to get a good effect";
								}
								ImGui::SameLine(); ImGui::SetCursorPosX(10);
								ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 3.0);
								ImGui::TextCenter("Noise Initial Frequency");
								ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 6.0);
								ImGui::PushItemWidth(-10 - 10 - numericboxwidth);
								if (ImGui::SliderFloat("##UI2fFractalInitialFrequence", &ggterrain_global_params.fractal_initial_freq, 0.01f, 8.0f, " "))
								{
									bTriggerStableY = true;

								}
								if (ImGui::IsItemHovered()) ImGui::SetTooltip("Set Noise Initial Frequency.");
								ImGui::PopItemWidth();
								ImGui::SameLine();
								ImGui::PushItemWidth(numericboxwidth);
								if (ImGui::InputFloat("##UI2fFractalInitialFrequenceText", &ggterrain_global_params.fractal_initial_freq, 0, 0, "%.1f"))
								{
									bTriggerStableY = true;

								}
								if (!pref.iTurnOffEditboxTooltip && ImGui::IsItemHovered()) ImGui::SetTooltip("Set Noise Initial Frequency.");
								ImGui::PopItemWidth();

								//PE: Noise Initial Amplitude
								ImGui::ImgBtn(ICON_INFO, ImVec2(ImGui::GetFontSize(), ImGui::GetFontSize()), ImColor(0, 0, 0, 0), ImColor(220, 220, 220, 220), ImColor(255, 255, 255, 255), ImColor(180, 180, 160, 255), -1, 0, 0, 0, false, false, false, false, false);
								if (ImGui::IsItemHovered())
								{
									cSpecialTooltip = "Noise Initial Amplitude: The multiplier used on the first noise layer.\nSetting this to 0 will make everything flat, whereas a value of 1\nwill utilise the full height range set by the Max Height parameter";
								}
								ImGui::SameLine(); ImGui::SetCursorPosX(10);
								ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 3.0);
								ImGui::TextCenter("Noise Initial Amplitude");
								ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 6.0);
								ImGui::PushItemWidth(-10 - 10 - numericboxwidth);
								if (ImGui::SliderFloat("##UI2fNoise Initial Amplitude", &ggterrain_global_params.fractal_initial_amplitude, 0.0f, 1.0f, " "))
								{
									bTriggerStableY = true;
								}
								if (ImGui::IsItemHovered()) ImGui::SetTooltip("Set Noise Initial Amplitude.");
								ImGui::PopItemWidth();
								ImGui::SameLine();
								ImGui::PushItemWidth(numericboxwidth);
								if (ImGui::InputFloat("##UI2fNoise Initial AmplitudeText", &ggterrain_global_params.fractal_initial_amplitude, 0, 0, "%.1f"))
								{
									bTriggerStableY = true;
								}
								if (!pref.iTurnOffEditboxTooltip && ImGui::IsItemHovered()) ImGui::SetTooltip("Set Noise Initial Amplitude.");
								ImGui::PopItemWidth();

								ImGui::ImgBtn(ICON_INFO, ImVec2(ImGui::GetFontSize(), ImGui::GetFontSize()), ImColor(0, 0, 0, 0), ImColor(220, 220, 220, 220), ImColor(255, 255, 255, 255), ImColor(180, 180, 160, 255), -1, 0, 0, 0, false, false, false, false, false);
								if (ImGui::IsItemHovered())
								{
									cSpecialTooltip = "How the frequency of the noise changes with \neach additional iteration, typically the frequency \nwill change by a multiple of 2.4 for each iteration. \nFor best results this value should be changed \nwith the \"Noise Amplitude Change\" value so that \n(Noise Amplitude Change)*(Noise Frequency Change) is close to 1";
								}
								ImGui::SameLine(); ImGui::SetCursorPosX(10);
								ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 3.0);
								ImGui::TextCenter("Noise Frequency Change");
								ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 6.0);
								ImGui::PushItemWidth(-10 - 10 - numericboxwidth);
								if (ImGui::SliderFloat("##UI2fFractalFrequenceIncrease", &ggterrain_global_params.fractal_freq_increase, 0.01f, 8.0f, " "))
								{
									bTriggerStableY = true;
								}
								if (ImGui::IsItemHovered()) ImGui::SetTooltip("Set Noise Frequency Change.");
								ImGui::PopItemWidth();
								ImGui::SameLine();
								ImGui::PushItemWidth(numericboxwidth);
								if (ImGui::InputFloat("##UI2fFractalFrequenceIncreaseText", &ggterrain_global_params.fractal_freq_increase, 0, 0, "%.1f"))
								{
									bTriggerStableY = true;
								}
								if (!pref.iTurnOffEditboxTooltip && ImGui::IsItemHovered()) ImGui::SetTooltip("Set Noise Frequency Change.");
								ImGui::PopItemWidth();

								ImGui::ImgBtn(ICON_INFO, ImVec2(ImGui::GetFontSize(), ImGui::GetFontSize()), ImColor(0, 0, 0, 0), ImColor(220, 220, 220, 220), ImColor(255, 255, 255, 255), ImColor(180, 180, 160, 255), -1, 0, 0, 0, false, false, false, false, false);
								if (ImGui::IsItemHovered())
								{
									cSpecialTooltip = "How the amplitude of the noise changes with \neach additional iteration, typically the amplitude \nwill change by a multiple of 0.4 for each iteration. \nFor best results this value should be changed \nwith the \"Noise Frequency Change\" value so that \n(Noise Amplitude Change)*(Noise Frequency Change) is close to 1";
								}
								ImGui::SameLine(); ImGui::SetCursorPosX(10);
								ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 3.0);
								ImGui::TextCenter("Noise Amplitude Change");
								ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 6.0);
								ImGui::PushItemWidth(-10 - 10 - numericboxwidth);
								if (ImGui::SliderFloat("##UI2fFractalFrequenceWeight", &ggterrain_global_params.fractal_freq_weight, 0.0f, 2.0f, " "))
								{
									bTriggerStableY = true;
								}
								if (ImGui::IsItemHovered()) ImGui::SetTooltip("Set Noise Amplitude Change.");
								ImGui::PopItemWidth();
								ImGui::SameLine();
								ImGui::PushItemWidth(numericboxwidth);
								if (ImGui::InputFloat("##UI2fFractalFrequenceWeightText", &ggterrain_global_params.fractal_freq_weight, 0, 0, "%.1f"))
								{
									bTriggerStableY = true;
								}
								if (!pref.iTurnOffEditboxTooltip && ImGui::IsItemHovered()) ImGui::SetTooltip("Set Noise Amplitude Change.");
								ImGui::PopItemWidth();

								//Seed value.
								ImGui::ImgBtn(ICON_INFO, ImVec2(ImGui::GetFontSize(), ImGui::GetFontSize()), ImColor(0, 0, 0, 0), ImColor(220, 220, 220, 220), ImColor(255, 255, 255, 255), ImColor(180, 180, 160, 255), -1, 0, 0, 0, false, false, false, false, false);
								if (ImGui::IsItemHovered())
								{
									cSpecialTooltip = "A random seed that is used as \nan input to the noise generator to \ncreate different results";
								}
								ImGui::SameLine(); ImGui::SetCursorPosX(10);
								ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 3.0);
								ImGui::TextCenter("Noise Seed Value");
								ImGui::PushItemWidth(-10);
								char ctmp[80];
								sprintf(ctmp, "%u", ggterrain_global_params.seed);
								if (ImGui::InputText("##seedText", ctmp, 78, ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_CharsDecimal))
								{
									if (strlen(ctmp) > 0)
									{
										ggterrain_global_params.seed = atol(ctmp);
										bTriggerStableY = true;
								}
								}
								if (!pref.iTurnOffEditboxTooltip && ImGui::IsItemHovered()) ImGui::SetTooltip("%s", "Set Noise Seed Value");
								if (ImGui::MaxIsItemFocused()) bImGuiGotFocus = true;
								ImGui::PopItemWidth();

								if (cSpecialTooltip != "")
								{
									//We are modal so special description popup.
									ImGui::SetNextWindowSize(ImVec2(500, 0), ImGuiCond_Always);
									ImGui::BeginTooltip();
									ImGui::Indent(12);
									ImGui::Text("");
									ImGui::PushItemWidth(-12);
									ImGui::TextWrapped(cSpecialTooltip.Get());
									ImGui::PopItemWidth();
									ImGui::Text("");
									ImGui::Indent(-12);
									ImGui::EndTooltip();
								}

							}

							ImGui::Indent(-10);
						}

						extern bool bProfilerEnable;
						if (bProfilerEnable)
						{
							ImGui::Separator();
							wiScene::Scene* pScene = &wiScene::GetScene();
							int iMeshes = pScene->meshes.GetCount();
							int iMaterials = pScene->materials.GetCount();

							int dc = wiProfiler::GetDrawCalls();
							int dcs = wiProfiler::GetDrawCallsShadows();
							int dct = wiProfiler::GetDrawCallsTransparent();

							int tris = wiProfiler::GetPolygons();
							int trisShadow = wiProfiler::GetPolygonsShadows();
							int trisTransparent = wiProfiler::GetPolygonsTransparent();

							ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
							ImGui::Text("DrawCalls: %d", dc);
							ImGui::Text("DrawCallsShadows: %d", dcs);
							ImGui::Text("DrawCallsTransparent: %d", dct);
							ImGui::Text("Triangles: %d", tris);
							ImGui::Text("TrianglesShadows: %d", trisShadow);
							ImGui::Text("TrianglesTransparent: %d", trisTransparent);
							ImGui::Text("Scene Meshes: %d", iMeshes);
							ImGui::Text("Scene Materials: %d", iMaterials);
							ImGui::Text("Scene Transforms: %d", (int)pScene->transforms.GetCount());
							ImGui::Text("Scene Hierarchy: %d", (int)pScene->hierarchy.GetCount());
							ImGui::Separator();
							std::string profiler_data = wiProfiler::GetProfilerData();
							ImGui::Text(profiler_data.c_str());
						}
					}
					else
					{
						ImGui::TextCenter("Time of Day");
				
						const char* time_combo[] = { "Dawn", "Morning", "Midday","Afternoon", "Evening", "Dusk","Night" , "Skybox: Fixed Time of Day" };

						int iChoises = IM_ARRAYSIZE(time_combo) - 1;
						int iSelection = t.visuals.iTimeOfday;
						bool bReadOnlyMode = !t.visuals.bDisableSkybox;
						if (t.visuals.skyindex == 0 && t.visuals.bDisableSkybox == false) bReadOnlyMode = false; //PE: Dynamic.
						if (bReadOnlyMode) iChoises++;
						if (bReadOnlyMode) iSelection = 7;

						if (!bReadOnlyMode)
						{
							ImGui::Checkbox("Random Time of Day", &bRandomizeTimeOfDay);
						}

						if (bReadOnlyMode)
						{
							ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
							ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
						}

						ImGui::PushItemWidth(-10);
						if (ImGui::Combo("##ComboTimeOfDay3", &iSelection, time_combo, iChoises) || iRandomTimeOfDayChoice >= 0)
						{
							t.visuals.iTimeOfday = iSelection;
							static int iLastRandomTimeOfDayChoice = -1;
							if (iRandomTimeOfDayChoice >= 0)
							{
								if (iLastRandomTimeOfDayChoice == iRandomTimeOfDayChoice)
								{
									iRandomTimeOfDayChoice++;
									if (iLastRandomTimeOfDayChoice == 6) iRandomTimeOfDayChoice = 0;
								}
							}
							else
							{
								//PE: Disable random when user make a selection.
								iLastUserSelectedTimeOfDay = iSelection;
								bRandomizeTimeOfDay = false;
							}
							if (iRandomTimeOfDayChoice > 6) iRandomTimeOfDayChoice = 6;

							if (bFirstBiomes[iSelectedThemeChoice])
							{
								bFirstBiomes[iSelectedThemeChoice] = false;
								bSelectDayRandomSkybox = true;
								//Select a day time.
								if ((rand() % 3) == 1)
									iRandomTimeOfDayChoice = 1;
								else if ((rand() % 3) == 1)
									iRandomTimeOfDayChoice = 4;
								else
									iRandomTimeOfDayChoice = 3;
							}

							if (iRandomTimeOfDayChoice >= 0)
								t.gamevisuals.iTimeOfday = t.visuals.iTimeOfday = iRandomTimeOfDayChoice;
							else
								t.gamevisuals.iTimeOfday = t.visuals.iTimeOfday;
							iLastRandomTimeOfDayChoice = iRandomTimeOfDayChoice;

							visuals_calcsunanglefromtimeofday(t.gamevisuals.iTimeOfday, &t.gamevisuals.SunAngleX, &t.gamevisuals.SunAngleY, &t.gamevisuals.SunAngleZ);
							t.visuals.SunAngleX = t.gamevisuals.SunAngleX;
							t.visuals.SunAngleY = t.gamevisuals.SunAngleY;
							t.visuals.SunAngleZ = t.gamevisuals.SunAngleZ;

							oldSunAngleX = t.visuals.SunAngleX; //PE: Make sure to update after save.
							oldSunAngleY = t.visuals.SunAngleY;
							oldSunAngleZ = t.visuals.SunAngleZ;

							Wicked_Update_Visuals((void *)&t.visuals);
							bTriggerStableY = true;
							fLastY = -1; //Trigger fog update.
						}

						if (bReadOnlyMode)
						{
							ImGui::PopItemFlag();
							ImGui::PopStyleVar();
						}

						if (ImGui::IsItemHovered()) ImGui::SetTooltip("This sets the sun at the correct position for the time of day");
						ImGui::PopItemWidth();
					}

					if (ImGui::StyleCollapsingHeader("Auto Populate Terrain", ImGuiTreeNodeFlags_DefaultOpen))
					{
						ImGui::Indent(10);
						float fButtonSizeX = (ImGui::GetContentRegionAvailWidth() - 10.0f);

						if (ImGui::Checkbox("Trees", &t.visuals.bEndableTreeDrawing))
						{
							t.showeditortrees = t.gamevisuals.bEndableTreeDrawing = t.visuals.bEndableTreeDrawing;
							if (t.visuals.bEndableTreeDrawing)
							{
								ggtrees_global_params.draw_enabled = 1;
							}
							else
							{
								ggtrees_global_params.draw_enabled = 0;
							}
						}
					
						ImGui::SameLine();
						ImGui::SetCursorPos(ImVec2(fButtonSizeX*0.5, ImGui::GetCursorPosY()));
						if (ImGui::Checkbox("Vegetation", &t.visuals.bEndableGrassDrawing))
						{
							t.showeditorveg = t.gamevisuals.bEndableGrassDrawing = t.visuals.bEndableGrassDrawing;
						}
						if (t.visuals.bEndableGrassDrawing)
						{
							if (bFirstTimeVeg)
							{
								//PE: We need grass everywhere for this to work.
								if (gggrass_global_params.paint_type == 0)
									gggrass_global_params.paint_type = 1;
								GGGrass::GGGrass_AddAll();
								bFirstTimeVeg = false;
							}
							gggrass_global_params.draw_enabled = 1;
						}
						else
						{
							gggrass_global_params.draw_enabled = 0;
						}
						ImGui::Indent(-10);
					}

					if (ImGui::StyleCollapsingHeader("Import Heightmap", ImGuiTreeNodeFlags_DefaultOpen))
					{
						float fButtonSizeX = (ImGui::GetContentRegionAvailWidth() - 20.0f);

							ImGui::PushID(11116);
							extern void ControlAdvancedSetting(int&, const char*, bool* = nullptr);
							ControlAdvancedSetting(pref.iEnableTerrainHeightmaps, "Heightmap Settings");
							ImGui::PopID();

							//PE: Heightmaps.
							if (pref.iEnableTerrainHeightmaps)
							{
								ImGui::Indent(10);
								ImGui::TextCenter("Heightmap Scale");
								float numericboxwidth = 60.0f;
								ImGui::PushItemWidth(-10 - 10 - numericboxwidth);
								if (ImGui::SliderFloat("##UI2fheightmap_scaleslider", &ggterrain_global_params.heightmap_scale, 0.01, 10.0f, "%.2f", 2.0f))
								{
								bTriggerStableY = true;
								}
								if (!pref.iTurnOffEditboxTooltip && ImGui::IsItemHovered()) ImGui::SetTooltip("Set Heightmap Scale");
								ImGui::PopItemWidth();
								ImGui::SameLine();
								ImGui::PushItemWidth(numericboxwidth);
								if (ImGui::InputFloat("##UI2fheightmap_scaleText", &ggterrain_global_params.heightmap_scale, 0, 0, "%.2f"))
								{
								bTriggerStableY = true;
								}
								if (!pref.iTurnOffEditboxTooltip && ImGui::IsItemHovered()) ImGui::SetTooltip("Set Heightmap Scale");
								ImGui::PopItemWidth();


								ImGui::TextCenter("Heightmap File Format");

								static int iRawFormat = 0;

								//PE: Graphics program will normally export to small endian, so activate again.
								ImGui::RadioButton("Big Endian", &iRawFormat, 0);
								if (ImGui::IsItemHovered()) ImGui::SetTooltip("Format Big Endian");
								ImGui::SameLine();
								ImGui::RadioButton("Little Endian", &iRawFormat, 1);
								if (ImGui::IsItemHovered()) ImGui::SetTooltip("Format Little Endian");

								static bool bGenerateOutsideHeightmap = false;
								if (ImGui::Checkbox("Generate Terrain Outside Heightmap", &bGenerateOutsideHeightmap))
								{
									GGTerrain_SetGenerateTerrainOutsideHeightMap(bGenerateOutsideHeightmap);
									bTriggerStableY = true;
								}

								if (!bGenerateOutsideHeightmap)
								{
									ImGui::TextCenter("Height Value Outside Heightmap");
									ImGui::PushItemWidth(-10 - 10 - numericboxwidth);
									if (ImGui::SliderFloat("##UI2fheight_outside_heightmapslider", &ggterrain_global_params.height_outside_heightmap, -2000.0, 10000.0f, "%.2f", 2.0f))
									{
									}
									if (!pref.iTurnOffEditboxTooltip && ImGui::IsItemHovered()) ImGui::SetTooltip("Set Height Value Outside Heightmap");
									ImGui::PopItemWidth();
									ImGui::SameLine();
									ImGui::PushItemWidth(numericboxwidth);
									if (ImGui::InputFloat("##UI2fheight_outside_heightmapText", &ggterrain_global_params.height_outside_heightmap, 0, 0, "%.2f"))
									{
									}
									if (!pref.iTurnOffEditboxTooltip && ImGui::IsItemHovered()) ImGui::SetTooltip("Set Height Value Outside Heightmap");
									ImGui::PopItemWidth();
								}

								ImGui::TextCenter("Height Map Fade Distance");
								ImGui::PushItemWidth(-10 - 10 - numericboxwidth);
								if (ImGui::SliderFloat("##UI2fheight_outside_fadeslider", &ggterrain_global_params.fade_outside_heightmap, 0, 1000.0f, "%.2f", 2.0f))
								{
								}
								if (!pref.iTurnOffEditboxTooltip && ImGui::IsItemHovered()) ImGui::SetTooltip("Set Height Map Fade Distance");
								ImGui::PopItemWidth();
								ImGui::SameLine();
								ImGui::PushItemWidth(numericboxwidth);
								if (ImGui::InputFloat("##UI2fheight_outside_fadeText", &ggterrain_global_params.fade_outside_heightmap, 0, 0, "%.2f"))
								{
								}
								if (!pref.iTurnOffEditboxTooltip && ImGui::IsItemHovered()) ImGui::SetTooltip("Set Height Map Fade Distance");
								ImGui::PopItemWidth();

								//PE: Note - Should be able to import 16bit and 32bit ? and or also 32bit floats ?
								static int mapwidth = 4096, mapheight = 4096;
								const char* items_mapsize[] = { "1024x1024", "2048x2048", "4096x4096" ,"Custom" }; //PE: ,"Custom" later.
								static int item_current_mapsize = 2;

								ImGui::TextCenter("Heightmap Raw Import Size");

								ImGui::PushItemWidth(-10);
								if (ImGui::Combo("##HeightmapRawImportSize", &item_current_mapsize, items_mapsize, IM_ARRAYSIZE(items_mapsize)))
								{
									if (item_current_mapsize == 0)
									{
										mapwidth = mapheight = 1024;
									}
									else if (item_current_mapsize == 1)
									{
										mapwidth = mapheight = 2048;
									}
									else if (item_current_mapsize == 2)
									{
										mapwidth = mapheight = 4096;
									}
								}
								ImGui::PopItemWidth();
								float fContentWidth = ImGui::GetContentRegionAvailWidth();
								if (item_current_mapsize == 3)
								{
									ImGui::PushItemWidth(fContentWidth * 0.25 - 10.0);
									ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(0.0f, 3.0f));
									ImGui::Text("Width ");
									ImGui::SameLine();
									ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(0.0f, -3.0f));
									if (ImGui::InputInt("##UI2imapwidthText", &mapwidth, 0, 0, 0))
									{

									}
									ImGui::PopItemWidth();
									ImGui::SameLine();
									ImGui::SetCursorPos(ImVec2(fContentWidth*0.5 - 10.0, ImGui::GetCursorPosY()));
									ImGui::Text("Height ");
									ImGui::SameLine();
									ImGui::PushItemWidth(fContentWidth * 0.25 - 10.0);
									if (ImGui::InputInt("##UI2imapheightText", &mapheight, 0, 0, 0))
									{

									}
									ImGui::PopItemWidth();
								}
								if (!pref.iTurnOffEditboxTooltip && ImGui::IsItemHovered()) ImGui::SetTooltip("Set Heightmap Raw Import Size");

								if (ImGui::StyleButton("Choose Heightmap Raw File", ImVec2(fButtonSizeX, 0.0f)))
								{
									cStr tOldDir = GetDir();
									char * cFileSelected;
									cFileSelected = (char *)noc_file_dialog_open(NOC_FILE_DIALOG_OPEN, "All\0*.*\0Raw\0*.raw\0Dat\0*.dat\0", g.mysystem.mapbankAbs_s.Get(), NULL);
									SetDir(tOldDir.Get());
									if (cFileSelected && strlen(cFileSelected) > 0)
									{
										bTriggerStableY = true;
										cstr import_filename = cFileSelected;
										bool bImage = false;
										if (import_filename.Len() > 4)
										{
											if (cstr(Lower(Right(import_filename.Get(), 4))) == ".jpg") bImage = true;
											if (cstr(Lower(Right(import_filename.Get(), 4))) == ".png") bImage = true;
											if (cstr(Lower(Right(import_filename.Get(), 4))) == "jpeg") bImage = true;
										}

										if (bImage)
										{
											int heightmapSizeX = 0, heightmapSizeY = 0, heightmapChannels = 0;
											uint8_t* heightmapImageData = stbi_load(import_filename.Get(), &heightmapSizeX, &heightmapSizeY, &heightmapChannels, 4);

											int iDescIndex = 0;
											if (heightmapImageData && heightmapSizeX >= 64 && heightmapSizeY >= 64 && heightmapSizeX <= 4096 && heightmapSizeY <= 4096)
											{
												uint16_t *tmpdata = new uint16_t[heightmapSizeX * heightmapSizeY];
												if (tmpdata)
												{
													for (int y = 0; y < heightmapSizeY; y++)
													{
														for (int x = 0; x < heightmapSizeX; x++)
														{
															int index = y * heightmapSizeX + x;
															int iValue = heightmapImageData[4 * index + 0] + heightmapImageData[4 * index + 1] + heightmapImageData[4 * index + 2];
															if (x > 0 && y > 0 && x < heightmapSizeX - 1 && y < heightmapSizeY - 1)
															{
																//Some blur needed.
																int index1 = (y + 1) * heightmapSizeX + (x + 1);
																iValue += heightmapImageData[4 * index + 0] + heightmapImageData[4 * index + 1] + heightmapImageData[4 * index + 2];
																index1 = y * heightmapSizeX + (x + 1);
																iValue += heightmapImageData[4 * index + 0] + heightmapImageData[4 * index + 1] + heightmapImageData[4 * index + 2];
																index1 = (y + 1) * heightmapSizeX + x;
																iValue += heightmapImageData[4 * index + 0] + heightmapImageData[4 * index + 1] + heightmapImageData[4 * index + 2];

																index1 = (y - 1) * heightmapSizeX + (x - 1);
																iValue += heightmapImageData[4 * index + 0] + heightmapImageData[4 * index + 1] + heightmapImageData[4 * index + 2];
																index1 = y * heightmapSizeX + (x - 1);
																iValue += heightmapImageData[4 * index + 0] + heightmapImageData[4 * index + 1] + heightmapImageData[4 * index + 2];
																index1 = (y - 1) * heightmapSizeX - x;
																iValue += heightmapImageData[4 * index + 0] + heightmapImageData[4 * index + 1] + heightmapImageData[4 * index + 2];

																iValue *= 0.2;
															}
															tmpdata[index] = iValue;

														}
													}
													GGTerrain_SetHeightMap(tmpdata, heightmapSizeX, heightmapSizeY, false);
												}
												if (tmpdata) delete[] tmpdata;

											}

											if (heightmapImageData) delete[] heightmapImageData;
										}
										else
										{
											GGTerrain_SetGenerateTerrainOutsideHeightMap(bGenerateOutsideHeightmap);
											int iRet = 0;
											if (iRawFormat == 0)
												iRet = GGTerrain_LoadHeightMap(import_filename.Get(), mapwidth, mapheight, true);
											else
												iRet = GGTerrain_LoadHeightMap(import_filename.Get(), mapwidth, mapheight, false);
											bool bValid = false;
											if (iRet == 0)
											{
												//PE: If fail try finding the dimensions.
												//The width and height can be calculated from the file size by taking the size in bytes, dividing by 2 and then square rooting

												DWORD filesize = 0;
												HANDLE hfile = GG_CreateFile(import_filename.Get(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
												if (hfile != INVALID_HANDLE_VALUE)
												{
													filesize = GetFileSize(hfile, NULL);
													CloseHandle(hfile);
												}

												if (filesize > 0)
												{
													int iSize = sqrt(filesize*0.5);
													if (iRawFormat == 0)
														iRet = GGTerrain_LoadHeightMap(import_filename.Get(), iSize, iSize, true);
													else
														iRet = GGTerrain_LoadHeightMap(import_filename.Get(), iSize, iSize, false);

													if (iRet != 0)
													{
														item_current_mapsize = 3;
														mapwidth = mapheight = iSize;
													}
												}
												if (iRet == 0)
												{
													//PE: The heightmap selected was not in the correct format.
													MessageBoxA(NULL, "The heightmap selected was not in the correct format.", "Error", 0);
													//PE: Switch to custom raw date size if thats the problem.
													item_current_mapsize = 3;
												}
												else
												{
													bValid = true;
												}
											}
											else
											{
												bValid = true;
											}
											if (bValid)
											{
												if (pestrcasestr(import_filename.Get(), "grandcanyon.raw"))
												{
													ggterrain_global_params.noise_power = 1.0f;
													ggterrain_global_params.noise_fallof_power = 0.0f;
													ggterrain_global_params.fractal_initial_freq = 0.362f;
													ggterrain_global_params.fractal_levels = 7;
													ggterrain_global_params.fractal_freq_increase = 2.5f;
													ggterrain_global_params.fractal_freq_weight = 0.4f;
													ggterrain_global_params.heightmap_roughness = 0.5f;
													ggterrain_global_params.height = GGTerrain_MetersToUnits(2290.0f);
													ggterrain_global_params.offset_y = GGTerrain_MetersToUnits(-55.0f);
													ggterrain_global_params.heightmap_scale = 0.125f;
													ggterrain_global_render_params.baseLayerMaterial = 0x100 | 30;
													ggterrain_global_render_params.layerMatIndex[0] = 0x100 | 30;
													ggterrain_global_render_params.layerMatIndex[1] = 0x100 | 14;
													ggterrain_global_render_params.layerMatIndex[2] = 0x100 | 15;
													ggterrain_global_render_params.slopeMatIndex[0] = 0x100 | 16;
													ggterrain_global_render_params.layerStartHeight[0] = GGTerrain_MetersToUnits(0);
													ggterrain_global_render_params.layerStartHeight[1] = GGTerrain_MetersToUnits(7.6f);
													ggterrain_global_render_params.layerStartHeight[2] = GGTerrain_MetersToUnits(1500);
													ggterrain_global_render_params.layerStartHeight[3] = GGTerrain_MetersToUnits(10000);
													ggterrain_global_render_params.layerStartHeight[4] = GGTerrain_MetersToUnits(10000);
													ggterrain_global_render_params.layerEndHeight[0] = GGTerrain_MetersToUnits(2.5f);
													ggterrain_global_render_params.layerEndHeight[1] = GGTerrain_MetersToUnits(68.24f);
													ggterrain_global_render_params.layerEndHeight[2] = GGTerrain_MetersToUnits(1800);
													ggterrain_global_render_params.layerEndHeight[3] = GGTerrain_MetersToUnits(10000);
													ggterrain_global_render_params.layerEndHeight[4] = GGTerrain_MetersToUnits(10000);
													ggterrain_global_render_params.slopeStart[0] = 0.2f;
													ggterrain_global_render_params.slopeStart[1] = 1.0f;
													ggterrain_global_render_params.slopeEnd[0] = 0.4f;
													ggterrain_global_render_params.slopeEnd[1] = 1.0f;
												}
												if (pestrcasestr(import_filename.Get(), "snowden.raw"))
												{
													ggterrain_global_params.noise_power = 1.0f;
													ggterrain_global_params.noise_fallof_power = 0.0f;
													ggterrain_global_params.fractal_initial_freq = 0.274f;
													ggterrain_global_params.fractal_levels = 7;
													ggterrain_global_params.fractal_freq_increase = 2.5f;
													ggterrain_global_params.fractal_freq_weight = 0.4f;
													ggterrain_global_params.heightmap_roughness = 1.0f;
													ggterrain_global_params.heightmap_scale = 0.06f;
													ggterrain_global_params.height = GGTerrain_MetersToUnits(1000.0f);
													ggterrain_global_params.offset_y = GGTerrain_MetersToUnits(-60.0f);
													ggterrain_global_render_params.baseLayerMaterial = 0x100 | 17;
													ggterrain_global_render_params.layerMatIndex[0] = 0x100 | 2;
													ggterrain_global_render_params.layerMatIndex[1] = 0x100 | 19;
													ggterrain_global_render_params.layerMatIndex[2] = 0x100 | 0;
													ggterrain_global_render_params.slopeMatIndex[0] = 0x100 | 4;
													ggterrain_global_render_params.layerStartHeight[0] = GGTerrain_MetersToUnits(0);
													ggterrain_global_render_params.layerStartHeight[1] = GGTerrain_MetersToUnits(28.5f);
													ggterrain_global_render_params.layerStartHeight[2] = GGTerrain_MetersToUnits(10000);
													ggterrain_global_render_params.layerStartHeight[3] = GGTerrain_MetersToUnits(10000);
													ggterrain_global_render_params.layerStartHeight[4] = GGTerrain_MetersToUnits(10000);
													ggterrain_global_render_params.layerEndHeight[0] = GGTerrain_MetersToUnits(2.5f);
													ggterrain_global_render_params.layerEndHeight[1] = GGTerrain_MetersToUnits(89.9f);
													ggterrain_global_render_params.layerEndHeight[2] = GGTerrain_MetersToUnits(10000);
													ggterrain_global_render_params.layerEndHeight[3] = GGTerrain_MetersToUnits(10000);
													ggterrain_global_render_params.layerEndHeight[4] = GGTerrain_MetersToUnits(10000);
													ggterrain_global_render_params.slopeStart[0] = 0.2f;
													ggterrain_global_render_params.slopeStart[1] = 1.0f;
													ggterrain_global_render_params.slopeEnd[0] = 0.4f;
													ggterrain_global_render_params.slopeEnd[1] = 1.0f;
												}
											}

										}
									}
								}
								if (!pref.iTurnOffEditboxTooltip && ImGui::IsItemHovered()) ImGui::SetTooltip("%s", "Import Heightmap Raw File");

								if (ImGui::StyleButton("Remove Heightmap Data", ImVec2(fButtonSizeX, 0.0f)))
								{
									GGTerrain_RemoveHeightMap();
									bTriggerStableY = true;
								}
								if (!pref.iTurnOffEditboxTooltip && ImGui::IsItemHovered()) ImGui::SetTooltip("%s", "Remove Heightmap Data");
								ImGui::Indent(-10);
							}
						}


					//## Terrain Buttons ##
					ImGui::BeginChild("##ChildProceduralButtons", ImVec2(0, ImGui::GetFontSize() * 5.0 + 3.0), true, ImGuiWindowFlags_ForceRender | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoSavedSettings);
					ImGui::Indent(10);
					float fButtonSizeX = (ImGui::GetContentRegionAvailWidth() - 10.0f);

					ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(0.0f, 10.0f));
					if (ImGui::StyleButton("Save Terrain Settings", ImVec2(fButtonSizeX, 0.0f)))
					{
						//PE: Make sure folder exists.
						char destination[MAX_PATH];
						strcpy(destination, "databank\\terrainsettings\\");
						GG_GetRealPath(destination, 1);
						MakeDirectory(destination);
						cstr terrainfile = "";

						t.returnstring_s = "";
						cStr tOldDir = GetDir();
						char * cFileSelected = (char *)noc_file_dialog_open(NOC_FILE_DIALOG_SAVE, "ter\0*.ter\0", destination, NULL, true);
						SetDir(tOldDir.Get());
						if (cFileSelected && strlen(cFileSelected) > 0) 
						{
							t.returnstring_s = cFileSelected;
						}
						if (t.returnstring_s != "")
						{
							if (cstr(Lower(Right(t.returnstring_s.Get(), 4))) != ".ter")  t.returnstring_s = t.returnstring_s + ".ter";
							terrainfile = t.returnstring_s;

							bool oksave = true;
							if (FileExist(terrainfile.Get())) {
								oksave = overWriteFileBox(terrainfile.Get());
							}
							if (oksave)
							{
								//Save here.
								GGTerrainFile_SaveTerrainData(terrainfile.Get(), g.gdefaultwaterheight);
							}
						}

					}
					if (ImGui::IsItemHovered()) ImGui::SetTooltip("Save Terrain Settings");

					if (ImGui::StyleButton("Load Terrain Settings", ImVec2(fButtonSizeX, 0.0f)))
					{
						//PE: Make sure folder exists.
						char destination[MAX_PATH];
						strcpy(destination, "databank\\terrainsettings\\");
						GG_GetRealPath(destination, 1);
						MakeDirectory(destination);

						cStr tOldDir = GetDir();
						char * cFileSelected;
						cFileSelected = (char *)noc_file_dialog_open(NOC_FILE_DIALOG_OPEN, "ter\0*.ter\0", destination, NULL, true);
						SetDir(tOldDir.Get());
						if (cFileSelected && strlen(cFileSelected) > 0)
						{
							t.returnstring_s = cFileSelected;
							if (t.returnstring_s != "")
							{
								if (cstr(Lower(Right(t.returnstring_s.Get(), 4))) == ".ter")
								{
									//Load settings.
									GGTerrainFile_LoadTerrainData(t.returnstring_s.Get(), true);
									bTreeGlobalInit = false;
									bTriggerStableY = true;
								}
							}
						}
					}
					if (ImGui::IsItemHovered()) ImGui::SetTooltip("Load Terrain Settings");

					ImGui::SetWindowFontScale(1.0);
					ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(0.0f, 3.0f));
					ImGui::Indent(-10);
					ImGui::EndChild();
				}

				//## Terrain Buttons END ##
				if (!pref.bHideTutorials)
				{
					if (ImGui::StyleCollapsingHeader("Tutorial", ImGuiTreeNodeFlags_DefaultOpen))
					{
						ImGui::Indent(10);
						char* my_combo_itemsp[] = { NULL,NULL,NULL };
						int my_combo_items = 0;
						int iVideoSection = 0;
						cstr cShowTutorial = "0501 - Terrain Generator";
						my_combo_itemsp[0] = "0501 - Terrain Generator";
						my_combo_itemsp[1] = "0502 - Terrain Height Maps";
						my_combo_items = 2;
						iVideoSection = SECTION_TERRAIN_GENERATOR;
						SmallTutorialVideo(cShowTutorial.Get(), my_combo_itemsp, my_combo_items, iVideoSection);
						float but_gadget_size = ImGui::GetFontSize()*12.0;
						float w = ImGui::GetWindowContentRegionWidth() - 10.0;
						ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((w*0.5) - (but_gadget_size*0.5), 0.0f));
						ImGui::Indent(-10);
					}
				}

				if (fSnapShotModeCameraY > fMaxCameraY) fSnapShotModeCameraY = fMaxCameraY;

				// insert a keyboard shortcut component into panel
				UniversalKeyboardShortcut(eKST_TerrainGenerator);
			}
			else
			{
				if (ImGui::StyleCollapsingHeader("Screenshot Mode", ImGuiTreeNodeFlags_DefaultOpen))
				{
					ImGui::Indent(10);
					extern bool bEditorLight;
					if (ImGui::Checkbox("Editor Light", &bEditorLight))
					{
						WickedCall_EnableCameraLight(bEditorLight);
					}
					LPSTR pSnapshotButtonTitle = "Take Screenshot";
					if(bPopModalTakeMapSnapshot==true) pSnapshotButtonTitle = "Take Map Snapshot";
					if (ImGui::StyleButton(pSnapshotButtonTitle, ImVec2(ImGui::GetContentRegionAvailWidth() - 10.0f, 0.0f)))
					{
						//PE: Just exit, we will have the lastest screenshot available in thumbbank.
						iQuitProceduralLevel = 5;
					}
					ImGui::Indent(-10);
				}

				if (fSnapShotModeCameraY > fMaxCameraY) fSnapShotModeCameraY = fMaxCameraY;

				if (bPopModalTakeMapSnapshot == false)
				{
					// insert a keyboard shortcut component into panel
					UniversalKeyboardShortcut(eKST_ObjectLibrary);
				}
			}

			if (ImGui::GetCurrentWindow()->ScrollbarSizes.x > 0) 
			{
				//Hitting exactly at the botton could cause flicker, so add some additional lines when scrollbar on.
				ImGui::Text("");
				ImGui::Text("");
			}

			//When popup open , make sure we update the backbuffer all the time.
			#ifdef DIGAHOLE
			bLoopBackBuffer = false;
			bSnapShotModeUseCamera = false;
			#else
			bLoopBackBuffer = true;
			bSnapShotModeUseCamera = true;
			#endif

			if (!bPopModalOpenProceduralCameraMode)
			{
				ImGui::EndChild();
			}

			if (!bPopModalOpenProceduralCameraMode)
			{
				ImGui::SetWindowFontScale(1.4);
				ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(0.0, 3.0));
				float fButtonSizeX = (ImGui::GetContentRegionAvailWidth() - 1.0f);
				ImVec2 texthere = ImGui::GetCursorPos();
				if (ImGui::StyleButton("##Generate Terrain for Level", ImVec2(fButtonSizeX, ImGui::GetFontSize()*3.0)))
				{
					t.addentityfile_s = "_markers\\Player Start.fpe";
					if (t.addentityfile_s != "")
					{
						entity_adduniqueentity(false);
						t.tasset = t.entid;
						if (t.talreadyloaded == 0)
						{
							editor_filllibrary();
						}
					}

					int masterobj = g.entitybankoffset + t.entid;

					// duplicate new entity as clone of relevant original clipboard entity
					bool bLowestFound = false;
					t.gridentity = t.entid;

					//PE: all t.gridentity... need to be set for this to work correctly.
					entity_fillgrideleproffromprofile();  // t.entid
					t.gridentityposx_f = 0;
					t.gridentityposy_f = BT_GetGroundHeight(t.terrain.TerrainID, 0, 0);
					t.gridentityposz_f = 0;

					if (t.gridentityposy_f < g.gdefaultwaterheight)
					{
						// Position the start marker above water, in case the following search for land is unsuccessful.
						t.gridentityposy_f = g.gdefaultwaterheight;
						t.editorfreeflight.c.y_f = t.gridentityposy_f;
						t.editorfreeflight.s = t.editorfreeflight.c;
						PositionCamera(0, t.gridentityposy_f, 0);

						float fNewYPos = t.gridentityposy_f;

						const int iPointsOnCircle = 12;
						const float fSearchRadius = 2000; // inches
						const int iSearchesToAttempt = GGTerrain::GGTerrain_GetEditableSize() / fSearchRadius;

						// Get angles for points on a circle on the terrain.
						float fAngleIncrement = 2.0f * 3.14159265359f / iPointsOnCircle; // radians

						bool bLandFound = false;

						// Go around the circle with fSearchRadius, gradually make the circle bigger until land is found, or we run out of room.
						for (int j = 0; j < iSearchesToAttempt; j++)
						{
							for (int i = 0; i < iPointsOnCircle; i++)
							{
								float x = fSearchRadius * (j + 1) * cosf(fAngleIncrement * i);
								float z = fSearchRadius * (j + 1) * sinf(fAngleIncrement * i);

								fNewYPos = BT_GetGroundHeight(t.terrain.TerrainID, x, z);

								// Found some land, reposition the start marker.
								if (fNewYPos > g.gdefaultwaterheight)
								{
									// Do an accurate but slow height check to make sure the marker won't be placed inside the terrain.
									GGTerrain_GetHeight(x, z, &fNewYPos, 1); 
									t.gridentityposy_f = fNewYPos + 10.0f;
									t.gridentityposx_f = x;
									t.gridentityposz_f = z;
									t.editorfreeflight.c.y_f = t.gridentityposy_f;
									t.editorfreeflight.c.x_f = x;
									t.editorfreeflight.c.z_f = z;
									t.editorfreeflight.s = t.editorfreeflight.c;
									bLandFound = true;
									newLevelCamera.set = 1;
									newLevelCamera.x = x; newLevelCamera.y = fNewYPos; newLevelCamera.z = z;
									break;
								}
							}

							if (bLandFound)
								break;
						}
					}

					t.gridentityrotatex_f = ObjectAngleX(masterobj);
					t.gridentityrotatey_f = ObjectAngleY(masterobj);
					t.gridentityrotatez_f = ObjectAngleZ(masterobj);
					t.gridentityrotatequatmode = 0;
					t.gridentityrotatequatx_f = 0;
					t.gridentityrotatequaty_f = 0;
					t.gridentityrotatequatz_f = 0;
					t.gridentityrotatequatw_f = 1;
					t.gridentityscalex_f = ObjectScaleX(masterobj);
					t.gridentityscaley_f = ObjectScaleY(masterobj);
					t.gridentityscalez_f = ObjectScaleZ(masterobj);

					gridedit_addentitytomap(); //Add it to map set t.e

					t.refreshgrideditcursor = 1;
					t.gridentity = 0;
					t.gridentityposoffground = 0;
					t.gridentityusingsoftauto = 0;
					t.gridentityautofind = 0;
					t.gridentityobj = 0;
					editor_refreshentitycursor();
					t.widget.pickedObject = 0;
					t.gridentityextractedindex = 0;

					// when choose a biome, ensure any ambient and combat music copied over to remote project
					extern bool entity_copytoremoteifnotthere(LPSTR);
					entity_copytoremoteifnotthere(t.visuals.sAmbientMusicTrack.Get());
					entity_copytoremoteifnotthere(t.visuals.sCombatMusicTrack.Get());

					//PE: Add any systemwidelua.ele to end of current elements
					extern StoryboardStruct Storyboard;
					if (strlen(Storyboard.gamename) > 0)
					{
						timestampactivity(0, "loading in systemwidelua.ele");
						cstr storeoldELEfile = t.elementsfilename_s;
						char collectionELEfilename[MAX_PATH];
						strcpy(collectionELEfilename, "projectbank\\");
						strcat(collectionELEfilename, Storyboard.gamename);
						strcat(collectionELEfilename, "\\systemwidelua.ele");
						t.elementsfilename_s = collectionELEfilename;
						extern int g_iAddEntityElementsMode;
						g_iAddEntityElementsMode = 2;
						void c_entity_loadelementsdata(void);
						c_entity_loadelementsdata();
						t.elementsfilename_s = storeoldELEfile;
						g_iAddEntityElementsMode = 0;

						//PE: Need to load masterobject if not there.
						for (int i = 1; i <= g.entityelementlist; i++)
						{
							if (t.entityelement[i].eleprof.systemwide_lua)
							{
								int tentid = t.entityelement[i].bankindex;
								if (tentid == 0 || tentid > t.entityprofile.size() || t.entityprofile[tentid].ismarker != 12)
								{
									//PE: Need to reload and remap.
									extern int g_iAddEntitiesModeFrom;
									g_iAddEntitiesModeFrom = g.entidmaster + 1;
									cstr entProfileToAdd_s = "_markers\\BehaviorHidden.fpe";

									int iFoundMatchEntID = 0;
									for (int entid = 1; entid <= g.entidmaster; entid++)
									{
										if (stricmp(t.entitybank_s[entid].Get(), entProfileToAdd_s.Get()) == NULL)
										{
											iFoundMatchEntID = entid;
											break;
										}
									}
									if (iFoundMatchEntID == 0)
									{
										g.entidmaster++;
										entity_validatearraysize();
										t.entitybank_s[g.entidmaster] = entProfileToAdd_s;
										iFoundMatchEntID = g.entidmaster;
										extern int g_iAddEntitiesMode;
										g_iAddEntitiesMode = 1;
										entity_loadentitiesnow();
										g_iAddEntitiesMode = 0;
									}
									t.entityelement[i].bankindex = iFoundMatchEntID;
								}
							}
						}

					}

					//Code
					iQuitProceduralLevel = 5;
					bTreeGlobalInit = false;
				}
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("Generates the terrain for your level and takes you to the Level Editor");

				ImVec2 restorehere = ImGui::GetCursorPos();
				ImGui::SetCursorPos(texthere+ImVec2(0.0,8.0));
				ImGui::TextCenter("Generate Terrain and");
				ImGui::TextCenter("Open the Level Editor");
				ImGui::SetWindowFontScale(1.0);
			}
			ImGui::Columns(1);

			//Render titlebar centered.
			ImVec2 titlebar_pos;
			cstr title;
			if (!bUseNoTitleBar)
			{
				if (bPopModalOpenProceduralCameraMode==false)
				{
					title = "Procedural Level Generator";
				}
				else
				{
					title = "Screenshot mode";
				}
				float fTextSize = ImGui::CalcTextSize(title.Get()).x;
				float xcenter = (ImGui::GetWindowSize().x*0.5) - (fTextSize*0.5);
				titlebar_pos = ImGui::GetWindowPos() + ImVec2(xcenter, 4);
			}

			//PE:Add help window here, corner.
			if (bPopModalOpenProceduralCameraMode==false && bPopModalTakeMapSnapshot == false)
			{
				if (pref.iDisplayTerrainGeneratorWelcome)
				{
					//PE: Render only window, so dont loose focus and go behind other windows.
					float zoomwindow = 1.2;
					float winheight = 250 * zoomwindow;
					float winwidth = 214 * zoomwindow;
					float margin = 5.0 * zoomwindow;
					ImGuiWindow* window = ImGui::GetCurrentWindow();
					extern ImFont* customfont;
					if (window->DrawList && customfont)
					{
						window->DrawList->AddCallback((ImDrawCallback)10, NULL); //force render.

						ImVec4 style_winback = ImGui::GetStyle().Colors[ImGuiCol_WindowBg];
						style_winback.w = 0.5f;

						float yspacer = 21.0 * zoomwindow;
						ImVec2 window_pos = ImVec2((ImGui::GetWindowPos().x + 10.0), (ImGui::GetWindowPos().y+ImGui::GetWindowSize().y) - winheight);
						window->DrawList->AddRectFilled(window_pos, window_pos + ImVec2(winwidth, winheight-10.0), ImGui::GetColorU32(style_winback), 6.0f, 15);
						window->DrawList->AddRectFilled(window_pos, window_pos + ImVec2(winwidth, 22 * zoomwindow), ImGui::GetColorU32(style_winback), 6.0f, 15);
						window->DrawList->AddText(customfont, 15 * zoomwindow, ImVec2(window_pos.x + margin, window_pos.y + 3.0), IM_COL32(255, 255, 255, 255), "About this editor");
						window_pos += ImVec2(0, 3.0 * zoomwindow);
						window_pos += ImVec2(0, yspacer);
						window->DrawList->AddText(customfont, 15 * zoomwindow, ImVec2(window_pos.x + margin, window_pos.y + 3.0), IM_COL32(255, 255, 255, 255), "With this editor you can create");
						window_pos += ImVec2(0, yspacer);
						window->DrawList->AddText(customfont, 15 * zoomwindow, ImVec2(window_pos.x + margin, window_pos.y + 3.0), IM_COL32(255, 255, 255, 255), "the terrain type for your new level.");
						window_pos += ImVec2(0, yspacer);
						window->DrawList->AddText(customfont, 15 * zoomwindow, ImVec2(window_pos.x + margin, window_pos.y + 3.0), IM_COL32(255, 255, 255, 255), "The choices you can make with the");
						window_pos += ImVec2(0, yspacer);
						window->DrawList->AddText(customfont, 15 * zoomwindow, ImVec2(window_pos.x + margin, window_pos.y + 3.0), IM_COL32(255, 255, 255, 255), "right panel options include:");
						window_pos += ImVec2(0, yspacer + yspacer);
						window->DrawList->AddText(customfont, 15 * zoomwindow, ImVec2(window_pos.x + margin, window_pos.y + 3.0), IM_COL32(255, 255, 255, 255), "- The Biome style");
						window_pos += ImVec2(0, yspacer);
						window->DrawList->AddText(customfont, 15 * zoomwindow, ImVec2(window_pos.x + margin, window_pos.y + 3.0), IM_COL32(255, 255, 255, 255), "- Size of level");
						window_pos += ImVec2(0, yspacer);
						window->DrawList->AddText(customfont, 15 * zoomwindow, ImVec2(window_pos.x + margin, window_pos.y + 3.0), IM_COL32(255, 255, 255, 255), "- Trees and vegetation");
						window_pos += ImVec2(0, yspacer + yspacer);
						window->DrawList->AddText(customfont, 15 * zoomwindow, ImVec2(window_pos.x + margin, window_pos.y + 3.0), IM_COL32(255, 255, 255, 255), "Watch the tutorial for more help.");
						ImGui::SetItemAllowOverlap();
						if (ImGui::CloseButton(ImGui::GetCurrentWindow()->GetID("#ClearSearch"), ImGui::GetWindowPos() + ImVec2(winwidth - 13.0, ImGui::GetWindowSize().y - winheight - 3.0)))
						{
							pref.iDisplayTerrainGeneratorWelcome = false;
						}

						window->DrawList->AddCallback((ImDrawCallback)11, NULL); //disableforce render.
					}
				}
			}

			if (customfontlarge) ImGui::PopFont();
			ImGui::PopFont();

			if (bUseModal)
			{
				ImGui::EndPopup();
			}
			else
			{
				ImGui::End();
			}
			ImGui::PopStyleColor();

			//Render title bar after End. end fill titlebar.
			if (!bUseNoTitleBar)
			{
				ImGuiWindow* window = ImGui::GetCurrentWindow();
				ImGuiContext& g = *GImGui;
				window->DrawList->AddText(g.Font, g.FontSize, titlebar_pos, ImGui::GetColorU32(ImGuiCol_Text), title.Get());
			}
		}

		if (iQuitProceduralLevel > 0)
		{
			iQuitProceduralLevel--;
			//Make a final screenshot. without minimap ...
			ggterrain_global_render_params2.flags2 &= ~GGTERRAIN_SHADER_FLAG2_SHOW_MAP_SIZE;
			ggterrain_global_render_params2.flags2 &= ~GGTERRAIN_SHADER_FLAG2_SHOW_MAP_SIZE_3D;
			ggterrain_global_render_params2.flags2 &= ~GGTERRAIN_SHADER_FLAG2_SHOW_MINI_MAP;
			if (ObjectExist(TERRAINGENERATOR_OBJECT)) HideObject(TERRAINGENERATOR_OBJECT);
			if (iQuitProceduralLevel == 0)
			{
				//Create Screenshot.
				bool bForce = false;
				#ifdef DIGAHOLE
				bForce = true;
				#endif
				if (BitmapExist(99) || bForce)
				{
					SetGrabImageMode(1);
					LPGGSURFACE	pTmpSurface = g_pGlob->pCurrentBitmapSurface;
					ID3D11Texture2D *pBackBuffer = NULL;
					#ifdef DIGAHOLE
					//PE: We need 5 frames (iQuitProceduralLevel). before backbuffer is updated with latest without 2d. ?
					//pBackBuffer = (ID3D11Texture2D *)wiRenderer::GetDevice()->GetBackBufferForGG(&master.swapChain);
					pBackBuffer = NULL;
					#else
					pBackBuffer = (ID3D11Texture2D *)GetBitmapTexture2D(99);
					#endif

					g_pGlob->pCurrentBitmapSurface = pBackBuffer;
					GGSURFACE_DESC ddsd;
					pBackBuffer->GetDesc(&ddsd);
					float grabx = 1280, graby = 720;
					if (bPopModalTakeMapSnapshot == true)
					{
						// square that matches the prepared view of the top down map
						grabx = 800;
						graby = 800;
					}
					if (graby > ddsd.Height)
						graby = ddsd.Height;
					if (grabx > ddsd.Width)
						grabx = ddsd.Width;

					float imgcx = (ddsd.Width*0.5) - (grabx*0.5);
					float imgcy = (ddsd.Height*0.5) - (graby*0.5);
					if (imgcx + grabx > ddsd.Width) imgcx = ddsd.Width - grabx;
					if (imgcy < 0) imgcy = 0;
					if (imgcx < 0) imgcx = 0;
					//PE: We need a unique id for this STORYBOARD_THUMBS+400
					GrabImage(STORYBOARD_THUMBS + 400, imgcx, imgcy, imgcx + grabx, imgcy + graby, 3);
					SetGrabImageMode(0);
					g_pGlob->pCurrentBitmapSurface = pTmpSurface;
					if (ImageExist(STORYBOARD_THUMBS + 400))
					{
						char destination[MAX_PATH];
						strcpy(destination, "thumbbank\\lastnewlevel.jpg");
						GG_SetWritablesToRoot(true);
						GG_GetRealPath(destination, 1);
						GG_SetWritablesToRoot(false);
						extern bool g_bDontUseImageAlpha;
						g_bDontUseImageAlpha = true;
						SaveImage(destination, STORYBOARD_THUMBS + 400);
						g_bDontUseImageAlpha = false;
						DeleteImage(STORYBOARD_THUMBS + 400);
					}
				}
				#ifndef DIGAHOLE
				if (ObjectExist(TERRAINGENERATOR_OBJECT)) ShowObject(TERRAINGENERATOR_OBJECT);
				#endif

				if (!bPopModalOpenProceduralCameraMode)
					bTriggerTerrainSaveAsWindow = true; //Open save as window.

				//PE: Now default to object mode
				bForceKey = true;
				csForceKey = "o";
				bTerrain_Tools_Window = false;
				extern bool Entity_Tools_Window;
				Entity_Tools_Window = true;

			}
			//PE: Need "save as" here so dont actually close it down yet.
			if (bPopModalOpenProceduralCameraMode)
			{
				if (iQuitProceduralLevel == 0)
					bProceduralLevel = false;
			}
		}
		if (!bProceduralLevel)
		{
			#ifndef DIGAHOLE
			extern bool g_bNoTerrainRender;
			g_bNoTerrainRender = false;
			#endif

			#ifdef DIGAHOLE
			bDigAHoleToHWND = false;
			#endif

			//Close down modal popup.
			BackBufferSaveCacheName = "";
			BackBufferObjectID = 0;
			BackBufferImageID = 0;
			bLoopBackBuffer = false;

			//Delete backbuffer and create a new.
			if (BitmapExist(99))
			{
				DeleteBitmapEx(99);
			}

			if (ObjectExist(TERRAINGENERATOR_OBJECT)) DeleteObject(TERRAINGENERATOR_OBJECT);

			//Restore fog.
			t.visuals.FogNearest_f = oldFogNear;
			t.visuals.FogDistance_f = oldFogFar;
			#ifndef RANDOMSKYBOX
			t.visuals.skyindex = oldSkyIndex;
			t.visuals.bDisableSkybox = oldbDisableSkybox;
			#endif
			t.visuals.CameraFAR_f = oldCameraFAR_f;
			t.visuals.fShadowFarPlane = oldfShadowFarPlane;
			t.visuals.bFXAAEnabled = oldbFXAAEnabled;

			fWickedMaxCenterTest = 0.0f; //Normal shadow matrix.
			t.visuals.SunAngleX = oldSunAngleX;
			t.visuals.SunAngleY = oldSunAngleY;
			t.visuals.SunAngleZ = oldSunAngleZ;

			t.visuals.ZenithRed_f = oldZenithRed_f;
			t.visuals.ZenithGreen_f = oldZenithGreen_f;
			t.visuals.ZenithBlue_f = oldZenithBlue_f;

			t.visuals.FogR_f = oldFogR_f;
			t.visuals.FogG_f = oldFogG_f;
			t.visuals.FogB_f = oldFogB_f;
			t.visuals.FogA_f = oldFogA_f;

			t.visuals.SkyCloudHeight = oldSkyCloudHeight;

			Wicked_Update_Visuals((void *)&t.visuals);

			//Display cubes again if needed.
			if (t.grideditselect == 0)
			{
				WickedCall_DisplayCubes(true);
			}

			editor_toggle_element_vis(t.showeditorelements);

			iLaunchAfterSync = 80; //Update all probes visuals ... after new level is generated.
			iSkibFramesBeforeLaunch = 5;

			//Restore camera.
			if (fOldCamx != 0 || fOldCamy != 0 || fOldCamz != 0)
			{
				t.editorfreeflight.mode = iCamMode;
				t.cx_f = t.editorfreeflight.c.x_f = fOldCamx;
				t.editorfreeflight.c.y_f = fOldCamy;
				t.cy_f = t.editorfreeflight.c.z_f = fOldCamz;
				PositionCamera(t.editorfreeflight.c.x_f, t.editorfreeflight.c.y_f, t.editorfreeflight.c.z_f);
				t.editorfreeflight.c.angx_f = fOldCamAx;
				t.editorfreeflight.c.angy_f = fOldCamAy;
				RotateCamera(t.editorfreeflight.c.angx_f, t.editorfreeflight.c.angy_f, 0);

				//Place in center.
				float camx = 350.0, camz = -360.0;
				float theight = BT_GetGroundHeight(t.terrain.TerrainID, camx, camz);
				if (theight < g.gdefaultwaterheight)
					theight = g.gdefaultwaterheight;
				t.editorfreeflight.mode = 3; //move camera to.
				t.editorfreeflight.c.y_f = theight + 1050.0; //From
				t.editorfreeflight.s.y_f = theight + 350.0; //To
				t.editorfreeflight.s.x_f = camx; //To
				t.editorfreeflight.s.z_f = camz; //To
				t.editorfreeflight.c.x_f = camx;
				t.editorfreeflight.c.z_f = camz;

				t.editorfreeflight.s.angx_f = 17.0;
				t.editorfreeflight.s.angy_f = 316.0;

				PositionCamera(t.editorfreeflight.c.x_f, t.editorfreeflight.c.y_f, t.editorfreeflight.c.z_f);

				if (newLevelCamera.set)
				{
					PositionCameraForNewLevel();
				}
			}

			ggterrain_global_render_params2.flags2 = oldflags2;
			set_terrain_sculpt_mode(GGTERRAIN_SCULPT_RAISE);
		}
		else
		{
			float theight = BT_GetGroundHeight(t.terrain.TerrainID, fSnapShotModeCameraX, fSnapShotModeCameraZ);
			if (fSnapShotModeCameraY < theight + 30.0f)  fSnapShotModeCameraY = theight + 30.0f;
			//When open always set main camera to the same as the new backbuffer.
			t.cx_f = t.editorfreeflight.c.x_f = fSnapShotModeCameraX;
			t.editorfreeflight.c.y_f = fSnapShotModeCameraY;
			t.cy_f = t.editorfreeflight.c.z_f = fSnapShotModeCameraZ;
			PositionCamera(t.editorfreeflight.c.x_f, t.editorfreeflight.c.y_f, t.editorfreeflight.c.z_f);

			//int oldcammode = t.cameraviewmode;
			t.editorfreeflight.c.angx_f = fSnapShotModeCameraAngX;
			t.editorfreeflight.c.angy_f = fSnapShotModeCameraAngY;
			RotateCamera(t.editorfreeflight.c.angx_f, t.editorfreeflight.c.angy_f, 0);
		}

	}
	else
	{
		bPopModalOpenProcedural = false;
	}

	//#################################
	//#### END Procedural Preview. ####
	//#################################
}

