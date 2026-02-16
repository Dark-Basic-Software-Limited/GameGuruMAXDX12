void GrabBackBufferCopy(void)
{
#ifdef OPTICK_ENABLE
	OPTICK_EVENT();
#endif
	// reject backbuffer copy under certain conditions
	if (iFogChangedFramesBeforeRestore > 0)
	{
		if(iFogChangedFramesBeforeRestore == 1) Wicked_Update_Visuals((void *)&t.visuals);
		iFogChangedFramesBeforeRestore--;
	}
	if (!bImGuiInitDone)
		return;

	// In DX12 mode, skip entirely — this function renders into a DX11 bitmap 99
	// render target and grabs pixels, none of which works in DX12. Calling
	// ForceRender(NULL) would start a second render pass on the swapChain
	// within the same frame, causing "Device Lost on Present".
	extern bool ImGui_DX12_IsInitialized();
	if (ImGui_DX12_IsInitialized())
		return;

	if (BackBufferObjectID == 0 && BackBufferGrabGameScreen)
	{
		// allow no object if grabbing whole game scene
	}
	else
	{
		if ((BackBufferObjectID <= 0 && !BackBufferSnapShotMode && !BackBufferParticlesMode) || BackBufferImageID <= 0)
			return;
	}

	if (BackBufferParticlesMode && iBackBufferParticlesTrigger > 0)
	{
		// wait some frames before capture.
		iBackBufferParticlesTrigger--;
		return;
	}

	if (iFogChangedFramesBeforeRestore > 0) iFogChangedFramesBeforeRestore = 5;

	if (!BackBufferParticlesMode && BackBufferParticleEmitter != -1)
	{
		gpup_deleteEffect(BackBufferParticleEmitter);
		BackBufferParticleEmitter = -1;
	}

	// can detect backbuffer size changes
	extern int iLastResolutionWidth;
	extern int iLastResolutionHeight;

	// make sure new rendertarget is the same as the backbuffer size.
	if (!BitmapExist(99) || (iLastResolutionWidth != current_backbuffer_width || iLastResolutionHeight != current_backbuffer_height))
	{
		current_backbuffer_height = iLastResolutionHeight;
		current_backbuffer_width = iLastResolutionWidth;
		if (wiGraphics::GetDevice())
		{
			//ID3D11Texture2D *pBackBuffer = (ID3D11Texture2D *)wiGraphics::GetDevice()->GetBackBufferForGG( &master.swapChain ); // REMOVED
			//if (pBackBuffer)
			{
				//GGSURFACE_DESC ddsd;
				//pBackBuffer->GetDesc(&ddsd);
				//PE: This is the resolution all current thumbs has used.
				if (bProceduralLevel || bFullScreenBackbuffer || BackBufferGrabGameScreen)
				{
					//Use current backbuffer size on this.
					MakeBitmap(99, iLastResolutionWidth, iLastResolutionHeight);
				}
				else
				{
					MakeBitmap(99, 1920, 1017);
				}
			}
		}
	}
	if (BitmapExist(99))
	{
		SetCurrentBitmap(99);
		SETUPClearEx(64, 64, 64, 64);
	}

	// using a render target in g_DefaultGGFORMAT is way faster.
	LPGGRENDERTARGETVIEW rendertarget;
	rendertarget = (LPGGRENDERTARGETVIEW)GetBitmapRenderTarget(99);

	// store current camera pos and angle
	if (!bBackdropSettingsSet || bSnapShotModeUseCamera || BackBufferParticlesMode)
	{
		composx = CameraPositionX(0);
		composy = CameraPositionY(0);
		composz = CameraPositionZ(0);
		comangx = CameraAngleX(0);
		comangy = CameraAngleY(0);
		comangz = CameraAngleZ(0);
	}

	// get water state
	bool bOldWater = t.visuals.bWaterEnable;

	// get backbuffer size as grab size
	float grabx = BackBufferSizeX;
	float graby = BackBufferSizeY;
	if (grabx <= 0 || graby <= 0)
	{
		grabx = 512;
		graby = 288;
	}

	// vars for below
	int displayobj, entid;
	float fOldObjPosX, fOldObjAngX, fOldObjPosY, fOldObjAngY, fOldObjPosZ, fOldObjAngZ;
	bool bDisplayObjVisible = false;
	bool bWaterVisible = false;

	// move camera for the grab, but not too far given camera resolution
	float centerx = -1000, centery = 39000, centerz = -1000;

	// if backbuffer ready to grab
	int backdropobj = BACKDROPMAGE;
	if (!BackBufferSnapShotMode && !BackBufferParticlesMode)
	{
		if (bUseBackDropImage && !ObjectExist(backdropobj))
		{
			CreateBackdropObject(false,"","");
		}
		if (bUseBackDropImage && ObjectExist(backdropobj))
		{
			if (BackBufferSizeX >= 1024)
				ScaleObject(backdropobj, 100, 100, 100);
			else if (BackBufferSizeX <= 512)
				ScaleObject(backdropobj, 50, 50, 50);
			ShowObject(backdropobj);
			if (cUseBackbufferCubemap.Len() > 0)
			{
				HideObject(backdropobj);
			}
		}

		// if object used
		sObject* pObject = NULL;
		bool bNeedZRotation = false;
		float fLargestY = 111;
		float fLargestX = 111;
		float fLargestZ = 111;
		float fLargest = 111;
		if (BackBufferObjectID > 0)
		{
			// place Object to be the subject of the grab
			displayobj = BackBufferObjectID;
			entid = displayobj - g.entitybankoffset;
			fOldObjPosX = ObjectPositionX(displayobj); fOldObjPosY = ObjectPositionY(displayobj); fOldObjPosZ = ObjectPositionZ(displayobj);
			fOldObjAngX = ObjectAngleX(displayobj); fOldObjAngY = ObjectAngleY(displayobj); fOldObjAngZ = ObjectAngleZ(displayobj);
			if (g_ObjectList[displayobj] && g_ObjectList[displayobj]->bVisible)	bDisplayObjVisible = true;
			float fOffsetX = 0.0f, fOffsetY = 0.0f, fOffsetZ = 0.0f;
			pObject = g_ObjectList[displayobj];
			if (pObject)
			{
				float fAdjustScaleX = 1.0, fAdjustScaleY = 1.0, fAdjustScaleZ = 1.0;
				if (pObject->pInstanceOfObject)
				{
					fAdjustScaleX = pObject->position.vecScale[0];
					fAdjustScaleY = pObject->position.vecScale[1];
					fAdjustScaleZ = pObject->position.vecScale[2];
					pObject = pObject->pInstanceOfObject;
				}
				float fValue;
				if (t.entityprofile[entid].ischaracter != 1)
				{
					fValue = (pObject->collision.vecMax[0] + pObject->collision.vecMin[0]);
					fValue = ApplyPivot(pObject, 0, GGVECTOR3(pObject->collision.vecMax - pObject->collision.vecMin), fValue);
					fValue = fValue * pObject->position.vecScale[0] * fAdjustScaleX;
					fOffsetX = fValue * 0.5f;
					fValue = (pObject->collision.vecMax[2] + pObject->collision.vecMin[2]);
					fValue = ApplyPivot(pObject, 2, GGVECTOR3(pObject->collision.vecMax - pObject->collision.vecMin), fValue);
					fValue = fValue * pObject->position.vecScale[2] * fAdjustScaleZ;
					fOffsetZ = fValue * 0.5f;
				}
				fValue = (pObject->collision.vecMax[1] + pObject->collision.vecMin[1]);
				fValue = ApplyPivot(pObject, 1, GGVECTOR3(pObject->collision.vecMax - pObject->collision.vecMin), fValue);
				fValue = fValue * pObject->position.vecScale[1] * fAdjustScaleY;
				fOffsetY = fValue * 0.5f;
				if (pObject->pFrame)
				{
					fOffsetX += -(pObject->pFrame->vecOffset.x * fAdjustScaleX);
					fOffsetY += -(pObject->pFrame->vecOffset.y * fAdjustScaleY);
					fOffsetZ += -(pObject->pFrame->vecOffset.z * fAdjustScaleZ);
				}
			}

			// find largest dimension
			fLargestY = ObjectSizeY(displayobj, 1);
			fLargestX = ObjectSizeX(displayobj, 1);
			fLargestZ = ObjectSizeZ(displayobj, 1);
			fLargest = fLargestX;
			if (fLargestZ > fLargest) fLargest = fLargestZ;
			if (fLargestY > fLargest) fLargest = fLargestY;

			// handle small objects.
			if (fLargest < 15.0) fLargest = 15.0;
			if (fLargest >= 2500.0) fLargest = 2500.0;

			// set object up
			PositionObject(displayobj, centerx, centery, centerz);
			if (t.entityprofile[entid].bIsDecal)
			{
				PositionObject(displayobj, centerx + (fLargestX * 0.5), centery + (fLargestY * 0.5), centerz);
			}
			if (t.entityprofile[entid].ismarker != 0 || t.entityprofile[entid].zdepth == 0)
			{
				SetObjectMask(displayobj, 1);
			}
			else
			{
				SetObjectMask(displayobj, 1 + (1 << 31));
			}
			ShowObject(displayobj);
			RotateObject(displayobj, fOldObjAngX, fOldObjAngY + 15, fOldObjAngZ);

			// set camera up
			RotateCamera(0, 0, 0);
			PositionCamera(centerx, centery, centerz);
			PointCamera(centerx, centery, centerz);

			// adjustments needed
			float fAdjustRange = 5.0;
			fAdjustRange -= (grabx + graby) / 512.0;
			if (fAdjustRange < 0.5)
				fAdjustRange = 0.5;
			if ((grabx + graby) <= 256)
				fAdjustRange += 2.0;
			if ((grabx + graby) >= 2048)
				fAdjustRange += 0.5;

			// 512x288=3.5 - This really depend on the image size we capture to.
			float fCamMove = fLargest * (fAdjustRange + 0.1);
			MoveCamera(-(fCamMove));
			BackBufferCamMove = fCamMove * 2.0;

			// adjust camera based on object largest calc and adjustment
			if (fLargest != 15.0 && fLargestY < 4.0) fLargestY += 200.0;
			if (fLargest != 15.0 && fLargestY < 7.0) fLargestY += 100.0;
			if (fLargest != 15.0 && fLargestY < 40.0) fLargestY = 40.0;
			if (fLargestY < 10.0) fLargestY = 10.0;
			float fAdjustY = fAdjustRange * 0.5;
			if (fLargestY >= 2500.0) fLargestY = 2500.0;
			if (t.entityprofile[entid].isebe == 1)
			{
				fLargestY += 140.0;
			}
			PositionCamera(CameraPositionX(0), CameraPositionY(0) + (fLargestY * fAdjustY), CameraPositionZ(0));
			PointCamera(centerx + fOffsetX, centery + fOffsetY, centerz + fOffsetZ);

			// work out if need rotation on the Z
			float cangx = CameraAngleX();
			float cangy = CameraAngleY();
			float cangz = CameraAngleZ();
			if (cangx > 39 && cangx < 300)
			{
				bNeedZRotation = true;
			}

			// restore backbuffer camera
			if (bBackBufferRestoreCamera)
			{
				//Restore camera settings from FPE. Only trigger this one time.
				BackBufferRotateY = RestoreBackBufferRotateY;
				if (t.entityprofile[entid].ischaracter == 0)
				{
					BackBufferRotateX = RestoreBackBufferRotateX;
				}
				else
				{
					BackBufferRotateX = 0.0;
				}
				BackBufferCamUp = RestoreBackBufferCamUp;
				BackBufferCamLeft = RestoreBackBufferCamLeft;
				BackBufferZoom = RestoreBackBufferZoom;
				if (BackBufferSizeX < 1024)
				{
					BackBufferZoom *= 1.7f;
					BackBufferCamUp *= 0.95f;
				}
			}

			// apply camera zoom and shift
			MoveCamera(BackBufferZoom);
			MoveCameraLeft(0, BackBufferCamLeft);
			MoveCameraUp(0, BackBufferCamUp);
		}

		// if backbuffer in loop or restore mode
		if (bLoopBackBuffer || bBackBufferRestoreCamera)
		{
			if (BackBufferObjectID > 0)
			{
				// object animation speed
				if (bBackBufferAnimated)
				{
					t.tanimspeed_f = t.entityprofile[BackBufferEntityID].animspeed;
					if (ObjectExist(BackBufferObjectID) == 1)
					{
						SetObjectSpeed(BackBufferObjectID, t.tanimspeed_f);
					}
				}

				// rotation mode
				if (bRotateBackBuffer && !bBackBufferAnimated)
				{
					if (bNeedZRotation)
					{
						BackBufferRotateZ += 3.0 * g.timeelapsed_f;
						if (BackBufferRotateZ > 360.0)
							BackBufferRotateZ -= 360.0;
					}
					else
					{
						BackBufferRotateY += 3.0 * g.timeelapsed_f;
						if (BackBufferRotateY > 360.0)
							BackBufferRotateY -= 360.0;
					}
				}

				// object and camera stats
				float ox = ObjectPositionX(displayobj);
				float oy = ObjectPositionY(displayobj);
				float oz = ObjectPositionZ(displayobj);
				float cx = CameraPositionX();
				float cy = CameraPositionY();
				float cz = CameraPositionZ();
				float dist = GetDistance(cx, cy, cz, ox, oy, oz);

				// apply object handling
				RotateObject(displayobj, 0, 0, 0);
				if (pObject && pObject->position.bApplyPivot)
				{
					PitchObjectUpWorld(displayobj, BackBufferRotateX);
					TurnObjectRightWorld(displayobj, BackBufferRotateY);
					if (bNeedZRotation) RollObjectLeftWorld(displayobj, BackBufferRotateZ);
				}
				else
				{
					PitchObjectDownWorld(displayobj, BackBufferRotateX);
					TurnObjectRightWorld(displayobj, BackBufferRotateY);
					if (bNeedZRotation)	RollObjectLeftWorld(displayobj, BackBufferRotateZ);
				}
			}
		}
		bBackBufferRestoreCamera = false;

		// using backdrop object
		if (bUseBackDropImage && ObjectExist(backdropobj))
		{
			MoveCamera(2.0f);
			PositionObject(backdropobj, CameraPositionX(), CameraPositionY(), CameraPositionZ());
			MoveCamera(-2.0f);
			PointObject(backdropobj, CameraPositionX(), CameraPositionY(), CameraPositionZ());
			MoveObject(backdropobj, -21200.0f);
			PointObject(backdropobj, CameraPositionX(), CameraPositionY(), CameraPositionZ());
		}

		// wwitch away from editor light , so we dont interfere with the current light on the scene.
		if (BackBufferObjectID > 0)
		{
			extern wiECS::Entity g_entityThumbLight, g_entityThumbLight2;
			if (g_entityThumbLight)
			{
				wiScene::TransformComponent* transformLightCamera = wiScene::GetScene().transforms.GetComponent(g_entityThumbLight);
				transformLightCamera->ClearTransform();
				float fCamX = CameraPositionX(0);
				float fCamY = CameraPositionY(0);
				float fCamZ = CameraPositionZ(0);
				transformLightCamera->Translate(XMFLOAT3(fCamX, fCamY + 20.0f, fCamZ));
				transformLightCamera->SetDirty();
			}
			if (g_entityThumbLight2)
			{
				wiScene::TransformComponent* transformLightCamera = wiScene::GetScene().transforms.GetComponent(g_entityThumbLight2);
				transformLightCamera->ClearTransform();
				float fCamX = ObjectPositionX(displayobj) - (fLargestX * 2.0);
				float fCamY = ObjectPositionY(displayobj);
				float fCamZ = ObjectPositionZ(displayobj) + (fLargestZ * 2.0); // move behind object
				transformLightCamera->Translate(XMFLOAT3(fCamX, fCamY + 20.0f, fCamZ));
				transformLightCamera->SetDirty();
			}
		}
	}
	else
	{
		// snap shop mode.
		if (BackBufferParticlesMode)
		{
			RotateCamera(0, 0, 0);
			PositionCamera(centerx, centery, centerz);
			PointCamera(centerx, centery, centerz);
			MoveCamera(-BackBufferZoom);
			MoveCameraUp(0, BackBufferCamUp);
			PointCamera(centerx, centery, centerz);
			if (bUseBackDropImage && ObjectExist(backdropobj))
			{
				if (BackBufferSizeX >= 1024) ScaleObject(backdropobj, 100, 100, 100);
				else if (BackBufferSizeX <= 512) ScaleObject(backdropobj, 50, 50, 50);
				ShowObject(backdropobj);
				MoveCamera(2.0f);
				PositionObject(backdropobj, CameraPositionX(), CameraPositionY(), CameraPositionZ());
				MoveCamera(-2.0f);
				PointObject(backdropobj, CameraPositionX(), CameraPositionY(), CameraPositionZ());
				MoveObject(backdropobj, -21200.0f); //Large objects get clipped, move further away.
				PointObject(backdropobj, CameraPositionX(), CameraPositionY(), CameraPositionZ());
			}
		}
		if (bSnapShotModeUseCamera )
		{
			PositionCamera(fSnapShotModeCameraX, fSnapShotModeCameraY, fSnapShotModeCameraZ);
			RotateCamera(fSnapShotModeCameraAngX, fSnapShotModeCameraAngY, fSnapShotModeCameraAngZ);
		}
	}

	// render settings
	extern bool g_bNoSwapchainPresent;
	bool ioldstate = g_bNoSwapchainPresent;
	g_bNoSwapchainPresent = true;

	extern bool g_bNo2DRender;
	g_bNo2DRender = true;
	if(bSnapShotModeUse2D || BackBufferGrabGameScreen)
		g_bNo2DRender = false;

	extern bool g_bNoTerrainRender;
	bool bOldg_bNoTerrainRender = g_bNoTerrainRender;
	if (bProceduralLevel)
		g_bNoTerrainRender = false;
	else
		g_bNoTerrainRender = true;

	// handlw wide screen scenario
	bool bIsWideScreen = false;
	float gpw = master.masterrenderer.GetPhysicalWidth();
	float gph = master.masterrenderer.GetPhysicalHeight();
	if (( (float)gpw / (float) gph) > 2.1 && gpw > 1920)
		bIsWideScreen = true;

	// fit fixed backbuffer and thumb resolution
	if (bIsWideScreen && !bFullScreenBackbuffer)
	{
		float fCameraFov = XM_PI / ((45.0) / 15.0f); 
		wiScene::GetCamera().CreatePerspective( 1920.0f, 1017.0f, t.visuals.CameraNEAR_f, t.visuals.CameraFAR_f, fCameraFov);
		wiScene::GetCamera().SetDirty(true);
	}

	// force a render of the backbuffer to do the grab
	bool renderstate = master.ForceRender(rendertarget);

	// restore render settings after the forced render
	if (bIsWideScreen && !bFullScreenBackbuffer)
	{
		float fCameraFov = XM_PI / ((t.visuals.CameraFOV_f) / 15.0f); //Fit GG settings.
		wiScene::GetCamera().CreatePerspective((float)master.masterrenderer.GetLogicalWidth(), (float)master.masterrenderer.GetLogicalHeight(), t.visuals.CameraNEAR_f, t.visuals.CameraFAR_f, fCameraFov);
		wiScene::GetCamera().SetDirty(true);
	}
	if (bProceduralLevel)
	{
		extern bool g_bNoTerrainRender;
		g_bNoTerrainRender = true;
	}
	else
	{
		g_bNoTerrainRender = bOldg_bNoTerrainRender;
	}
	g_bNo2DRender = false;
	g_bNoSwapchainPresent = ioldstate;
	bBackdropSettingsSet = false;

	// restore after grab process
	if (!BackBufferSnapShotMode && !BackBufferParticlesMode)
	{
		if (bUseBackDropImage && ObjectExist(backdropobj))
		{
			HideObject(backdropobj);
			if (!bLoopBackBuffer)
			{
				RevertBackbufferCubemap();
				WickedCall_UpdateProbes();
			}
		}
		if (!bLoopBackBuffer)
		{
			WickedCall_SetSunDirection(t.visuals.SunAngleX, t.visuals.SunAngleY, t.visuals.SunAngleZ);
			master_renderer->setBloomEnabled(t.visuals.bBloomEnabled);
			WickedCall_MoveReflectionProbe(GGORIGIN_X, GGORIGIN_Y + 5000, GGORIGIN_Z, "editorProbe", 500);
			WickedCall_EnableThumbLight(false);
		}
		PositionCamera(composx, composy, composz);
		RotateCamera(comangx, comangy, comangz);
		if (BackBufferObjectID > 0)
		{
			PositionObject(displayobj, fOldObjPosX, fOldObjPosY, fOldObjPosZ);
			RotateObject(displayobj, fOldObjAngX, fOldObjAngY, fOldObjAngZ);
			if (bDisplayObjVisible)
				ShowObject(displayobj);
			else
				HideObject(displayobj);
		}
	}
	else
	{
		if (bUseBackDropImage && ObjectExist(backdropobj))
		{
			HideObject(backdropobj);
		}
		if (bSnapShotModeUseCamera || BackBufferParticlesMode)
		{
			PositionCamera(composx, composy, composz);
			RotateCamera(comangx, comangy, comangz);
		}
		if (t.widget.pickedEntityIndex > 0 && t.widget.activeObject > 0)
		{
			widget_show_widget();
		}
	}
	if (!renderstate)
	{
		BackBufferSnapShotMode = false;
		BackBufferParticlesMode = false;
		return;
	}

	// if not snaposhot mode - save a second file to act as our ICON image (RPG inventory usage mainly)
	if (!BackBufferSnapShotMode && !BackBufferGrabGameScreen)
	{
		if (BackBufferSaveCacheName != "")
		{
			// ensure the grab results in a square icon
			int iIconImageID = BackBufferImageID;
			extern GlobStruct* g_pGlob;
			LPGGSURFACE	pTmpSurface = g_pGlob->pCurrentBitmapSurface;
			ID3D11Texture2D* pBackBuffer = NULL;
			if (rendertarget)
			{
				pBackBuffer = (ID3D11Texture2D*)GetBitmapTexture2D(99);
				//if (!pBackBuffer) pBackBuffer = (ID3D11Texture2D*)wiGraphics::GetDevice()->GetBackBufferForGG(&master.swapChain); // REMOVED
			}
			else
			{
				//pBackBuffer = (ID3D11Texture2D*)wiGraphics::GetDevice()->GetBackBufferForGG(&master.swapChain); // REMOVED
			}
			g_pGlob->pCurrentBitmapSurface = pBackBuffer;
			SetGrabImageMode(1);

			// get surface size to ensure grab not larger
			if (pBackBuffer)
			{
				int grabiconx = 288;
				int grabicony = 288;
				GGSURFACE_DESC ddsd;
				pBackBuffer->GetDesc(&ddsd);
				if (grabiconx > ddsd.Width) grabiconx = ddsd.Width;
				if (grabicony > ddsd.Height) grabicony = ddsd.Height;
				float imgcx = (ddsd.Width * 0.5) - (grabiconx * 0.5);
				float imgcy = (ddsd.Height * 0.5) - (grabicony * 0.5);
				if (imgcy < 0) imgcy = 0;
				if (imgcx < 0) imgcx = 0;
				if (imgcx + grabiconx > ddsd.Width)	grabiconx = (ddsd.Width - imgcx) - 1.0f;
				if (imgcy + grabicony > ddsd.Height) grabicony = (ddsd.Height - imgcy) - 1.0f;
				if (grabiconx > 0 && grabicony > 0)
				{
					GrabImage(iIconImageID, imgcx, imgcy, imgcx + grabiconx, imgcy + grabicony, 3);
				}
			}

			// restore bitmap pointer
			SetGrabImageMode(0);
			g_pGlob->pCurrentBitmapSurface = pTmpSurface;
				
			// ensure we save to writables area only
			if (ImageExist(iIconImageID))
			{
				char pRealICONFile[MAX_PATH];
				strcpy(pRealICONFile, BackBufferSaveCacheName.Get());
				pRealICONFile[strlen(pRealICONFile) - 4] = 0;
				strcat(pRealICONFile, ".png");
				GG_SetWritablesToRoot(true);
				GG_GetRealPath(pRealICONFile, 1);
				if (FileExist(pRealICONFile) == 1) DeleteAFile(pRealICONFile);
				SaveImage(pRealICONFile, iIconImageID);
				GG_SetWritablesToRoot(false);
			}
		}
	}

	// handle loop grab mode
	static int loop = 0;
	if(loop++ % 2 == 0 || !bLoopBackBuffer || bLoopFullFPS || BackBufferGrabGameScreen || BackBufferSnapShotMode || BackBufferParticlesMode)
	{
		// get backbuffer pointer
		int iPerEntityImageID = BackBufferImageID;
		extern GlobStruct* g_pGlob;
		LPGGSURFACE	pTmpSurface = g_pGlob->pCurrentBitmapSurface;
		ID3D11Texture2D *pBackBuffer = NULL;
		if (rendertarget)
		{
			pBackBuffer = (ID3D11Texture2D *) GetBitmapTexture2D(99);
			//if (!pBackBuffer) pBackBuffer = (ID3D11Texture2D *)wiGraphics::GetDevice()->GetBackBufferForGG( &master.swapChain ); // REMOVED
		}
		else
		{
			//pBackBuffer = (ID3D11Texture2D*)wiGraphics::GetDevice()->GetBackBufferForGG(&master.swapChain); // REMOVED
		}
		g_pGlob->pCurrentBitmapSurface = pBackBuffer;

		// get surface size to ensure grab not larger
		if (pBackBuffer)
		{
			GGSURFACE_DESC ddsd;
			pBackBuffer->GetDesc(&ddsd);
			SetGrabImageMode(1);
			if (graby > ddsd.Height)
				graby = ddsd.Height;
			if (grabx > ddsd.Width)
				grabx = ddsd.Width;

			// handle image size
			float imgcx = (ddsd.Width*0.5) - (grabx*0.5);
			float imgcy = (ddsd.Height*0.5) - (graby*0.5);
			if (imgcy < 0) imgcy = 0;
			if (imgcx < 0) imgcx = 0;

			// all screen for BackBufferGrabGameScreen
			if (BackBufferObjectID == 0 && BackBufferGrabGameScreen)
			{
				imgcy = 0;
				imgcx = 0;
				grabx = ddsd.Width;
				graby = ddsd.Height;
			}

			// if snapshot mode
			if (BackBufferSnapShotMode)
			{
				if (fLastRubberBandX2 > fLastRubberBandX1 && fLastRubberBandY2 > fLastRubberBandY1)
				{
					imgcx = fLastRubberBandX1;
					imgcy = fLastRubberBandY1;
					imgcy -= 10.0f;
					if (imgcx < 1.0f) imgcx = 1.0f;
					if (imgcy < 1.0f) imgcy = 1.0f;
					float fRatio = graby / grabx;
					if ((fLastRubberBandX2 - fLastRubberBandX1) > ((fLastRubberBandY2 - fLastRubberBandY1)*1.3))
					{
						grabx = fLastRubberBandX2 - fLastRubberBandX1;
						graby = grabx * fRatio;
						float fObjectsHeight = fLastRubberBandY2 - fLastRubberBandY1;
						if (graby > fObjectsHeight)
							imgcy -= ((graby - fObjectsHeight)*0.5);
						if (imgcy < 0) imgcy = 0;
					}
					else
					{
						fRatio = grabx / graby;
						graby = fLastRubberBandY2 - fLastRubberBandY1;
						float fObjectsWidth = fLastRubberBandX2 - fLastRubberBandX1;
						grabx = graby * fRatio;
						if(grabx > fObjectsWidth)
							imgcx -= ((grabx-fObjectsWidth)*0.5);
						if (imgcx < 0) imgcx = 0;
					}
				}
			}
			// make sure we are not going outside image.
			if (imgcy + graby > ddsd.Height)
				graby = (ddsd.Height - imgcy) - 1.0f;
			if (imgcx + grabx > ddsd.Width)
				grabx = (ddsd.Width - imgcx) - 1.0f;

			if (graby > 0 && grabx > 0)
			{
				GrabImage(iPerEntityImageID, imgcx, imgcy, imgcx + grabx, imgcy + graby, 3);
			}
			SetGrabImageMode(0);

			// restore bitmap pointer
			g_pGlob->pCurrentBitmapSurface = pTmpSurface;
		}
		else
		{
			// no backbuffer in DX12 mode, restore state
			g_pGlob->pCurrentBitmapSurface = pTmpSurface;
		}
	}

	// if not snaposhot mode and we want to save the grab
	if (!BackBufferSnapShotMode)
	{
		if (BackBufferSaveCacheName != "")
		{
			if (ImageExist(BackBufferImageID))
			{
				// ensure we save to writables area only
				char pRealThumbFile[MAX_PATH];
				strcpy(pRealThumbFile, BackBufferSaveCacheName.Get());
				GG_SetWritablesToRoot(true);
				GG_GetRealPath(pRealThumbFile, 1);
				if (FileExist(pRealThumbFile) == 1) DeleteAFile(pRealThumbFile);
				SaveImage(pRealThumbFile, BackBufferImageID);
				GG_SetWritablesToRoot(false);
			}
			BackBufferSaveCacheName = "";
		}
	}

	// restore other settings after grab
	if (!bLoopBackBuffer)
	{
		BackBufferImageID = 0;
		if (BackBufferParticlesMode && BackBufferParticleEmitter != -1)
		{
			gpup_deleteEffect(BackBufferParticleEmitter);
			BackBufferParticleEmitter = -1;
		}
		if (BackBufferParticlesMode)
		{
			BackBufferZoom = 0.0f;
			BackBufferCamUp = 0.0f;
		}
	}
	BackBufferSnapShotMode = false;
	BackBufferParticlesMode = false;
	BackBufferGrabGameScreen = false;
}

void RevertBackbufferCubemap(void)
{
	if (bBackbufferCubemapActive && cUseBackbufferCubemap.Len() > 0)
	{
		sky_skyspec_init();
		WickedCall_DeleteImage(cUseBackbufferCubemap.Get());
		if (t.grideditselect == 0)
		{
			WickedCall_DisplayCubes(true);
		}
		if (ObjectExist(t.terrain.terrainobjectindex) == 1)
		{
			ShowObject(t.terrain.terrainobjectindex);
		}
		t.hardwareinfoglobals.noterrain = 0;
		WickedCall_SetSunDirection(t.visuals.SunAngleX, t.visuals.SunAngleY, t.visuals.SunAngleZ);
		master_renderer->setBloomEnabled(t.visuals.bBloomEnabled);
		WickedCall_MoveReflectionProbe(GGORIGIN_X, GGORIGIN_Y+5000, GGORIGIN_Z, "editorProbe", 500);
		WickedCall_EnableThumbLight(false);
		WickedCall_UpdateProbes();
		bBackbufferCubemapActive = false;
	}
}

bool CreateProjectCacheName(char *project_name,char *file)
{
	if (!project_name || !file) return false;
	if (strlen(project_name) <= 0) return false;

	char tmp[MAX_PATH];
	char project[MAX_PATH];
	strcpy(tmp, file);
	char *find = (char *)pestrcasestr(tmp, "thumbbank\\");
	if (find) find += 10;
	else find = &tmp[0];
	strcpy(project, "projectbank\\");
	strcat(project, project_name);
	strcat(project, "\\");
	strcat(project, find);
	ProjectCacheName = project; //PE: Should be relative.
	return(true);
}
bool CopyToProjectFolder(char *file)
{
	if (strlen(Storyboard.gamename) <= 0) return false;
	// D:\MAX-DocWrite\Files\thumbbank\mapbank_cool level 1 - copy.fpm512x288.jpg
	char tmp[MAX_PATH];
	char project[MAX_PATH];

	strcpy(tmp, file);
	char *find = (char *) pestrcasestr(tmp, "thumbbank\\");
	if (find) find += 10;
	else find = &tmp[0];

	strcpy(project, "projectbank\\");
	strcat(project, Storyboard.gamename);
	strcat(project, "\\");
	strcat(project, find);
	strcpy(tmp, project);
	GG_GetRealPath(project, 1); //Resolve name. need full path.
	bool bRet = CopyFileA( (LPSTR) file, project, FALSE);
	ProjectCacheName = tmp; //PE: Should be relative.
	return bRet;
}
bool CreateBackBufferCacheNameEx(char *file,int width,int height, bool bUsedForSaving)
{
	// returns true if have own thumb or a group that does not generate one
	bool bHasOwnLocalThumb = false;

	std::string cache_name = file;
	replaceAll(cache_name, ".fpe", "");
	replaceAll(cache_name, "entitybank\\", "");
	replaceAll(cache_name, "\\", "_");
	replaceAll(cache_name, "/", "_");
	replaceAll(cache_name, "\"", ""); //Got this when deleting a file in user, if fpe not found and already in list.
	std::string cache_final_name = cache_name;
	if (bUsedForSaving == false)
	{
		std::string src_thumbbank_path = g.fpscrootdir_s.Get();
		src_thumbbank_path = src_thumbbank_path + "\\Files\\thumbbank\\";
		std::string base_name = src_thumbbank_path + cache_name + std::to_string(width) + "x" + std::to_string(height);
		cache_final_name = base_name + ".jpg";
		// Some thumbbank assets only have .dds or .png (no .jpg), try alternatives
		if (GetFileAttributesA(cache_final_name.c_str()) == INVALID_FILE_ATTRIBUTES)
		{
			std::string alt = base_name + ".dds";
			if (GetFileAttributesA(alt.c_str()) != INVALID_FILE_ATTRIBUTES)
				cache_final_name = alt;
			else
			{
				alt = base_name + ".png";
				if (GetFileAttributesA(alt.c_str()) != INVALID_FILE_ATTRIBUTES)
					cache_final_name = alt;
			}
		}
	}
	else
	{
		cache_final_name = g.mysystem.thumbbank_s.Get() + cache_name + std::to_string(width) + "x" + std::to_string(height) + ".jpg";
	}
	BackBufferCacheName = cache_final_name.c_str();

	// LB: We have pre-generated all stock assets thumbs, so copy over if we have them to save time
	if (g_bThumbBankCopyMode == true)
	{
		if (FileExist(BackBufferCacheName.Get()) == 0)
		{
			// source from thumbnank in parent files
			std::string src_file = g.fpscrootdir_s.Get();
			src_file = src_file + "\\Files\\thumbbank\\";
			src_file = src_file + cache_name;
			src_file = src_file + std::to_string(width) + "x" + std::to_string(height) + ".jpg";
			LPSTR pSrcFile = (LPSTR)src_file.c_str();
			char pAssociatedThumb[MAX_PATH];
			GG_SetWritablesToRoot(true);
			if (FileExist(pSrcFile) == 0)
			{
				// any object that has a JPG of the same FPE name can force a custom thumb
				char pRelativePath[MAX_PATH];
				strcpy(pAssociatedThumb, file);
				strcpy(pRelativePath, pAssociatedThumb);
				GG_SetWritablesToRoot(true);
				GG_GetRealPath(pAssociatedThumb, 0);
				GG_SetWritablesToRoot(false);
				if (FileExist(pAssociatedThumb) == 0)
				{
					// sometimes the file path is passed in without the entitybank
					strcpy(pAssociatedThumb, "entitybank\\");
					strcat(pAssociatedThumb, file);
					strcpy(pRelativePath, pAssociatedThumb);
					GG_SetWritablesToRoot(true);
					GG_GetRealPath(pAssociatedThumb, 0);
					GG_SetWritablesToRoot(false);
				}
				GG_SetWritablesToRoot(true);
				if (FileExist(pAssociatedThumb) == 1)
				{
					GG_SetWritablesToRoot(false);
					// so as not to disrupt purchased and other thumbs generated, just tackle building editor for now
					if (strnicmp(pRelativePath, "entitybank\\user\\buildingeditor", 30) == NULL)
					{
						if (strnicmp(pAssociatedThumb + strlen(pAssociatedThumb) - 4, ".fpe", 4) == NULL)
						{
							pAssociatedThumb[strlen(pAssociatedThumb) - 4] = 0;
							strcat(pAssociatedThumb, ".jpg");
							GG_GetRealPath(pAssociatedThumb, 0);
							if (FileExist(pAssociatedThumb) == 0)
							{
								strcpy(pAssociatedThumb, g.fpscrootdir_s.Get());
								strcat(pAssociatedThumb, "\\Files\\editors\\uiv3\\filetype-object.jpg");
							}
							pSrcFile = pAssociatedThumb;
							bHasOwnLocalThumb = true;
						}
					}
				}
			}
			GG_SetWritablesToRoot(false);

			// file destination not exist, copy src to it to save time
			CopyFileA (pSrcFile, BackBufferCacheName.Get(), TRUE);
		}
	}

	// determined to have own thumb
	return bHasOwnLocalThumb;
}
bool CreateBackBufferCacheName(char* file, int width, int height)
{
	return CreateBackBufferCacheNameEx(file, width, height, false);
}


