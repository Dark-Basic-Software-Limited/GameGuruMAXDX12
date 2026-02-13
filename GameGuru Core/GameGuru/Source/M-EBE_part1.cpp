void ebe_loop(void)
{
	// if no tools object, cannot proceed
	if (ebebuild.iToolObj == 0) return;
	if (ObjectExist(ebebuild.iToolObj) == 0) return;
	if (GetObjectData(ebebuild.iToolObj)->bVisible == false) return;

	//Dynamic place tools
	#if defined(ENABLEIMGUI) && !defined(USEOLDIDE) 
	ebebuild.iTexturePanelX = GetChildWindowWidth() - 210;
	ebebuild.iTexturePanelY = GetChildWindowHeight() - 200;
	ebebuild.iTexturePanelWidth = 200;
	ebebuild.iTexturePanelHeight = 200;
	for (int iTex = 0; iTex < EBETEXPANELSPRMAX; iTex++)
	{
		LPSTR pTexImg = "";
		int iX = ebebuild.iTexturePanelX;
		int iY = ebebuild.iTexturePanelY;
		int iWidth = ebebuild.iTexturePanelWidth;
		int iHeight = ebebuild.iTexturePanelHeight;
		if (iTex == 0) { iX -= 10; iY -= 10; iWidth += 20; iHeight += 20; }
		if (iTex == 1) { iX -= 10; iY -= 10; iWidth += 20; iHeight = 1; }
		if (iTex == 2) { iX -= 10; iY += 209; iWidth += 20; iHeight = 1; }
		if (iTex == 3) { iX -= 10; iY -= 10; iWidth = 1; iHeight += 20; }
		if (iTex == 4) { iX += 209; iY -= 10; iWidth = 1; iHeight += 20; }
		if (!bDisableAllSprites) MAXSprite(ebebuild.iTexturePanelSprite[iTex], iX, iY, ebebuild.iTexturePanelImg[iTex]);
	}
	if (!bDisableAllSprites) MAXSprite(ebebuild.iTexturePanelHighSprite, ebebuild.iTexturePanelX, ebebuild.iTexturePanelY, ebebuild.iTexturePanelHighImg);
	if (!bDisableAllSprites) MAXSprite(ebebuild.iEBEHelpSpr, ebebuild.iTexturePanelX - ImageWidth(ebebuild.iEBEHelpImg) - 10, ebebuild.iTexturePanelY + 210 - ImageHeight(ebebuild.iEBEHelpImg), ebebuild.iEBEHelpImg);
	if (!bDisableAllSprites) MAXSprite(ebebuild.iEBETexHelpSpr, ebebuild.iTexturePanelX - 10, ebebuild.iTexturePanelY - 10 - ImageHeight(ebebuild.iEBETexHelpImg), ebebuild.iEBETexHelpImg);
	int n = 0;
	for (int y = 0; y < 4; y++)
	{
		for (int x = 0; x < 4; x++)
		{
			if (!bDisableAllSprites) MAXSprite(ebebuild.iMatSpr[n], ebebuild.iTexturePanelX + 36 + (x * 50), ebebuild.iTexturePanelY + 36 + (y * 50), ebebuild.iMatImg[ebebuild.TXP.iMaterialRef[n]]);
			n++;
		}
	}
	ebe_settexturehighlight();
	#endif

	// if UNDO or somehow delete this building obj, revert entity mode
	if (ObjectExist(ebebuild.iBuildObj) == 0)
	{
		ebe_hide();
		t.inputsys.domodeentity = 1;
		return;
	}

	// go through model slowly to refresh meshes (and avoid a vertbuffer lock)
	if (ebebuild.iRefreshBuild > 0)
	{
		ebebuild.iRefreshBuild--;
		int iLayerTotarget = 19 - ebebuild.iRefreshBuild;
		int iYBottom = (iLayerTotarget * 10);
		int iYTop = (iLayerTotarget * 10) + 10;
		if (iYTop > CUBEAREASIZE - 11) iYTop = CUBEAREASIZE - 11;
		ebe_refreshmesh(ebebuild.iBuildObj, 0, iYBottom, 0, CUBEAREASIZE - 10, iYTop, CUBEAREASIZE - 10);
		return;
	}

	// keep orientation with the main build object
	RotateObject(ebebuild.iToolObj, ObjectAngleX(ebebuild.iBuildObj), ObjectAngleY(ebebuild.iBuildObj), ObjectAngleZ(ebebuild.iBuildObj));

	// Keep tool object with EBE entity object (build object)
	int iAtX = t.entityelement[t.ebe.entityelementindex].x;
	int iAtY = t.entityelement[t.ebe.entityelementindex].y;
	int iAtZ = t.entityelement[t.ebe.entityelementindex].z;
	PositionObject(ebebuild.iToolObj, iAtX, iAtY + (CameraPositionY(0) / 1000.0f), iAtZ);
	PositionObject(ebebuild.iBuildObj, iAtX, iAtY, iAtZ);

	// Only when release mouse continue
	if (t.ebe.bReleaseMouseFirst == true && t.inputsys.mclick != 0) return;
	t.ebe.bReleaseMouseFirst = false;

	// One press key logic
	if (t.inputsys.kscancode == 0) ebebuild.iLocalKeyPressed = 0;

	// Reason this is above action for selecting customise is to allow texture highlight to show as selected
	if (ebebuild.bCustomiseTexture == true && t.inputsys.mclick == 0)
	{
		ebebuild.bCustomiseTexture = false;
		int iEntityProfileIndex = t.entityelement[t.ebe.entityelementindex].bankindex;
		if (ebe_loadcustomtexture(iEntityProfileIndex, ebebuild.iCurrentTexture) == 1)
		{
			// successfully pasted new texture into plate
		}
	}

	// use - and + keys to scroll through material index for current texture selected
	// handled in the IMGUI hover window, changing the one being hovered over, not the one selected (quicker)

	#if defined(ENABLEIMGUI) && !defined(USEOLDIDE)
	//This has been moved to imgui.
	#else
	// Select texture if in Texture Panel or Customise one
	int iRealSprMouseX = (GetChildWindowWidth() / 800.0f) * t.inputsys.xmouse;
	int iRealSprMouseY = (GetChildWindowHeight() / 600.0f) * t.inputsys.ymouse;
	if (t.inputsys.mclick > 0)
	{
		if (iRealSprMouseX > ebebuild.iTexturePanelX && iRealSprMouseX < ebebuild.iTexturePanelX + ebebuild.iTexturePanelWidth)
		{
			if (iRealSprMouseY > ebebuild.iTexturePanelY && iRealSprMouseY < ebebuild.iTexturePanelY + ebebuild.iTexturePanelHeight)
			{
				// while tile
				float fWhichCol = (float)(iRealSprMouseX - ebebuild.iTexturePanelX) / (float)ebebuild.iTexturePanelWidth;
				float fWhichRow = (float)(iRealSprMouseY - ebebuild.iTexturePanelY) / (float)ebebuild.iTexturePanelHeight;
				int iWhichTextureOver = (((int)(fWhichRow * 4)) * 4) + (int)(fWhichCol * 4);

				// select texture choice
				ebebuild.iCurrentTexture = iWhichTextureOver;
				ebe_settexturehighlight();

				// and if it was right mouse, customise this texture too
				if (t.inputsys.mclick == 2)
				{
					// replace texture within texture atlas
					ebebuild.bCustomiseTexture = true;
				}

				// ensure we do not write into builder if selecting texture
				set_inputsys_mclick(0);//t.inputsys.mclick = 0;
			}
		}
	}
	#endif

	// Control EBE edit grid layer
	float fCurrentGridLayerAbsHeight = ebebuild.iCurrentGridLayer*5.0f;
	float fMouseWheel = t.inputsys.wheelmousemove;
	if (t.inputsys.kscancode == 201 || t.inputsys.kscancode == 209 || (fMouseWheel != 0 && t.inputsys.keycontrol == 0))
	{
		if (ebebuild.iLocalKeyPressed == 0)
		{
			// work out snap grid size
			int iGridSize = ebebuild.Pattern.iWidth;
			if (ebebuild.Pattern.iHeight > iGridSize) iGridSize = ebebuild.Pattern.iHeight;
			if (ebebuild.Pattern.iDepth > iGridSize) iGridSize = ebebuild.Pattern.iDepth;

			// special case for ROW shape (one high, and one dimension longer on X or Z)
			if (ebebuild.Pattern.iHeight == 1 && ebebuild.Pattern.iWidth != ebebuild.Pattern.iDepth) iGridSize = 1;

			// move current grid up or down
			float fYShift = 0.0f;
			ebebuild.iLocalKeyPressed = 1;
			if ((t.inputsys.kscancode == 201 || fMouseWheel > 0.0f) && ebebuild.iCurrentGridLayer < 200 - iGridSize) { ebebuild.iCurrentGridLayer += iGridSize; fYShift = iGridSize * 5.0f; }
			if ((t.inputsys.kscancode == 209 || fMouseWheel < 0.0f))
			{
				ebebuild.iCurrentGridLayer -= iGridSize; fYShift = iGridSize * -5.0f;
				if (ebebuild.iCurrentGridLayer < 0) ebebuild.iCurrentGridLayer = 0;
			}
			fCurrentGridLayerAbsHeight = ebebuild.iCurrentGridLayer*5.0f;
			OffsetLimb(ebebuild.iToolObj, 1, 500, fCurrentGridLayerAbsHeight + 0.05f, 500);

			// also move camera for easier more intuitive editing 
			if (fMouseWheel == 0.0f)
			{
				// only with mouse wheel for convenience
				t.editorfreeflight.c.y_f += fYShift;
				PositionCamera(0, t.editorfreeflight.c.x_f, t.editorfreeflight.c.y_f, t.editorfreeflight.c.z_f);
			}
		}
	}

	// Rotate cursor and pattern
	if (t.inputsys.k_s == "r")
	{
		if (ebebuild.iLocalKeyPressed == 0)
		{
			ebebuild.iLocalKeyPressed = 1;
			ebebuild.iCursorRotation += 1;
			if (ebebuild.iCursorRotation > 3)
			{
				ebebuild.iCursorRotation = 0;
			}
			ebe_updatepatternwithrotation();
		}
	}

	// Position cursor on current grid layer
	bool bOffGrid = false;
	float fGridAbsHeight = LimbPositionY(ebebuild.iToolObj, 1);
	float fFloorWorldPosX, fFloorWorldPosY, fFloorWorldPosZ;
	if (WickedCall_GetPick(&fFloorWorldPosX, &fFloorWorldPosY, &fFloorWorldPosZ, NULL, NULL, NULL, NULL, GGRENDERLAYERS_NORMAL) == true)
	{
		// found surface to locate the world position of the cursor
	}
	if ( bOffGrid == false )
	{
		t.tx_f = fFloorWorldPosX - ObjectPositionX(ebebuild.iToolObj); 
		t.tz_f = fFloorWorldPosZ - ObjectPositionZ(ebebuild.iToolObj);
		GGVECTOR3 vec3 = GGVECTOR3(t.tx_f, 0, t.tz_f);
		GGMATRIX matRot;
		GGMatrixRotationY(&matRot, GGToRadian(-ObjectAngleY(ebebuild.iToolObj)));
		GGVec3TransformCoord(&vec3, &vec3, &matRot);
		t.tx_f = vec3.x;
		t.tz_f = vec3.z;
		t.tx_f = (int)(t.tx_f / 5.0f);
		t.tz_f = (int)(t.tz_f / 5.0f);
		if (t.tx_f < 0) { t.tx_f = 0; bOffGrid = true; }
		if (t.tz_f < 0) { t.tz_f = 0; bOffGrid = true; }
		if (t.tx_f > 199) { t.tx_f = 199; bOffGrid = true; }
		if (t.tz_f > 199) { t.tz_f = 199; bOffGrid = true; }
		int iGridSnapX = ebebuild.Pattern.iWidth;
		int iGridSnapZ = ebebuild.Pattern.iDepth;
		if (iGridSnapX > iGridSnapZ) iGridSnapZ = iGridSnapX;
		if (iGridSnapZ > iGridSnapX) iGridSnapX = iGridSnapZ;
		t.tx_f = (int)(t.tx_f / iGridSnapX);
		t.tz_f = (int)(t.tz_f / iGridSnapZ);
		t.tx_f = (t.tx_f*iGridSnapX) + (ebebuild.Pattern.iWidth / 2);
		t.tz_f = (t.tz_f*iGridSnapZ) + (ebebuild.Pattern.iDepth / 2);
		t.tx_f += ebebuild.Pattern.iWidthOffset;
		t.tz_f += ebebuild.Pattern.iDepthOffset;
		ebebuild.iCursorGridPosX = t.tx_f;
		ebebuild.iCursorGridPosZ = t.tz_f;
		t.tx_f *= 5.0f;
		t.tz_f *= 5.0f;
		int iHalfX = ebebuild.Pattern.iWidth / 2;
		int iHalfZ = ebebuild.Pattern.iDepth / 2;
		float fFractionX = ebebuild.Pattern.iWidth - (iHalfX * 2);
		float fFractionZ = ebebuild.Pattern.iDepth - (iHalfZ * 2);
		float fCubeSizeX = fFractionX * 2.5f;
		float fCubeSizeY = ebebuild.Pattern.iHeight * 2.5f;
		float fCubeSizeZ = fFractionZ * 2.5f;
		OffsetLimb(ebebuild.iToolObj, 2, t.tx_f + fCubeSizeX, fCurrentGridLayerAbsHeight + fCubeSizeY, t.tz_f + fCubeSizeZ);
	}

	// Hide cursor if not in grid
	if (bOffGrid == false)
	{
		ShowLimb(ebebuild.iToolObj, 2);
		ScaleLimb(ebebuild.iToolObj, 2, ebebuild.Pattern.iWidth*100.0f, ebebuild.Pattern.iHeight*100.0f, ebebuild.Pattern.iDepth*100.0f);
		if (ebebuild.Pattern.iWidth == ebebuild.Pattern.iDepth)
			RotateLimb(ebebuild.iToolObj, 2, 0, ebebuild.iCursorRotation * 90.0f, 0);
		else
			RotateLimb(ebebuild.iToolObj, 2, 0, 0.0f, 0);
	}
	else
	{
		HideLimb(ebebuild.iToolObj, 2);
	}

	// Before start to mark cube data, snapshot for undo buffer
	if (t.ebe.iWrittenCubeData == 0 && t.inputsys.mclick == 1)
	{
		ebe_snapshottobuffer();
		t.ebe.iWrittenCubeData = 1;
	}
	else
	{
		// and wait for user to click and write 'something'
		if (t.inputsys.mclick == 0 && t.ebe.iWrittenCubeData > 1)
		{
			// only then when release mouse, do a new snapshot
			t.ebe.iWrittenCubeData = 0;
		}
	}

	// If use axis-line-system, can correct cursor position
	if (t.inputsys.keycontrol != 0)
	{
		if (ebebuild.iDeterminedAxisDir == 0)
		{
			if (ebebuild.iCursorGridPosLastGoodX != ebebuild.iCursorGridPosX) ebebuild.iDeterminedAxisDir = 1;
			if (ebebuild.iCursorGridPosLastGoodZ != ebebuild.iCursorGridPosZ) ebebuild.iDeterminedAxisDir = 2;
			ebebuild.iCursorGridPosLastGoodX = ebebuild.iCursorGridPosX;
			ebebuild.iCursorGridPosLastGoodZ = ebebuild.iCursorGridPosZ;
		}
		else
		{
			if (ebebuild.iDeterminedAxisDir == 1) ebebuild.iCursorGridPosZ = ebebuild.iCursorGridPosLastGoodZ;
			if (ebebuild.iDeterminedAxisDir == 2) ebebuild.iCursorGridPosX = ebebuild.iCursorGridPosLastGoodX;
		}
	}
	else
	{
		ebebuild.iDeterminedAxisDir = 0;
		ebebuild.iCursorGridPosLastGoodX = ebebuild.iCursorGridPosX;
		ebebuild.iCursorGridPosLastGoodZ = ebebuild.iCursorGridPosZ;
	}

	// Detect if we add/delete/texture inside the grid
	if (bOffGrid == false)
	{
		bool bReverseOperation = false;
		if (t.inputsys.keyshift == 1) bReverseOperation = true;
		if (t.inputsys.mclick == 1)
		{
			// holding down left mouse button - add/delete/texture
			bool bRecordChangeToCubeData = false;
			int iRecordX1 = 999999, iRecordY1 = 999999, iRecordZ1 = 999999;
			int iRecordX2 = -999999, iRecordY2 = -999999, iRecordZ2 = -999999;
			for (int iPatternX = 0; iPatternX < ebebuild.Pattern.iWidth; iPatternX++)
			{
				for (int iPatternY = 0; iPatternY < ebebuild.Pattern.iHeight; iPatternY++)
				{
					for (int iPatternZ = 0; iPatternZ < ebebuild.Pattern.iDepth; iPatternZ++)
					{
						// get cube tyle and texture assignment
						cStr pPattern = ebebuild.Pattern.pPRow[iPatternY][iPatternZ];
						char* p;
						unsigned char cCubeType = strtol(Mid(pPattern.Get(), 1 + iPatternX), &p, 16);
						unsigned char cTexIndex = 0;
						if (cCubeType > 0) cTexIndex = ebebuild.iCurrentTexture;

						// intended new block type
						unsigned char cBitCube = (cTexIndex << 4) + cCubeType;

						// work out exact place in construction grid
						int iThisX = ebebuild.iCursorGridPosX + iPatternX;
						int iThisY = ebebuild.iCurrentGridLayer + iPatternY;
						int iThisZ = ebebuild.iCursorGridPosZ + iPatternZ;

						// cursor holds shape at center, so backward offset
						iThisX = iThisX - ebebuild.Pattern.iWidth / 2;
						iThisZ = iThisZ - ebebuild.Pattern.iDepth / 2;

						// only if within field of construction area
						if (iThisX < CUBEAREASIZE && iThisY < CUBEAREASIZE && iThisZ < CUBEAREASIZE)
						{
							// add or delete pattern
							bool bPreserveCube = false;
							if (bReverseOperation == true || iPaintMode != 1)
							{
								// delete mode
								// only wipe out if same texture
								unsigned char cTexIndexA = (cBitCube & (15 << 4)) >> 4;
								unsigned char cTexIndexB = (pCubes[iThisX][iThisY][iThisZ] & (15 << 4)) >> 4;
								if (cTexIndexA == cTexIndexB)
									cBitCube = 0;
								else
									cBitCube = pCubes[iThisX][iThisY][iThisZ];

								// if in delete mode, cancel a delete if the preserve mode protects it
								if (cBitCube == 0 && ebebuild.Pattern.iPreserveMode == 1)
								{
									// check if 'wall for example' is next to a cube at Z+1/X+1 which is filled
									int iThisRotation = ebebuild.iCursorRotation;
									if (iThisRotation == 0)
										if (iThisZ + 1 < CUBEAREASIZE)
											if (pCubes[iThisX][iThisY][iThisZ + 1] != 0)
												bPreserveCube = true;
									if (iThisRotation == 1)
										if (iThisX + 1 < CUBEAREASIZE)
											if (pCubes[iThisX + 1][iThisY][iThisZ] != 0)
												bPreserveCube = true;
									if (iThisRotation == 2)
										if (iThisZ - 1 >= 0)
											if (pCubes[iThisX][iThisY][iThisZ - 1] != 0)
												bPreserveCube = true;
									if (iThisRotation == 3)
										if (iThisX - 1 >= 0)
											if (pCubes[iThisX - 1][iThisY][iThisZ] != 0)
												bPreserveCube = true;
								}
							}
							else
							{
								// add mode
								// if in add mode, cancel an addition if the preserve mode protects it
								// and the target is already filled in (an existing cube)
								if (pCubes[iThisX][iThisY][iThisZ] != 0)
								{
									if (ebebuild.Pattern.iPreserveMode == 1)
									{
										// check if 'wall for example' is next to a cube at Z+1/X+1 which is filled
										int iThisRotation = ebebuild.iCursorRotation;
										if (iThisRotation == 0)
											if (iThisZ + 1 < CUBEAREASIZE)
												if (pCubes[iThisX][iThisY][iThisZ + 1] != 0)
													bPreserveCube = true;
										if (iThisRotation == 1)
											if (iThisX + 1 < CUBEAREASIZE)
												if (pCubes[iThisX + 1][iThisY][iThisZ] != 0)
													bPreserveCube = true;
										if (iThisRotation == 2)
											if (iThisZ - 1 >= 0)
												if (pCubes[iThisX][iThisY][iThisZ - 1] != 0)
													bPreserveCube = true;
										if (iThisRotation == 3)
											if (iThisX - 1 >= 0)
												if (pCubes[iThisX - 1][iThisY][iThisZ] != 0)
													bPreserveCube = true;
									}
									if (ebebuild.Pattern.iPreserveMode == 2)
									{
										// check if 'stairs for example' should not add to anything already present
										if (pCubes[iThisX][iThisY][iThisZ] != 0)
											bPreserveCube = true;
									}
								}
							}

							// replace blank with current cube already in there
							if (bPreserveCube == true)
							{
								cBitCube = pCubes[iThisX][iThisY][iThisZ];
							}

							// proceed to replace cube if different in any way
							if (pCubes[iThisX][iThisY][iThisZ] != cBitCube)
							{
								pCubes[iThisX][iThisY][iThisZ] = cBitCube;
								if (iThisX < iRecordX1) iRecordX1 = iThisX;
								if (iThisY < iRecordY1) iRecordY1 = iThisY;
								if (iThisZ < iRecordZ1) iRecordZ1 = iThisZ;
								if (iThisX > iRecordX2) iRecordX2 = iThisX;
								if (iThisY > iRecordY2) iRecordY2 = iThisY;
								if (iThisZ > iRecordZ2) iRecordZ2 = iThisZ;
								bRecordChangeToCubeData = true;
							}

							// mark that we have written something to the cube data, just just clicked
							if (t.ebe.iWrittenCubeData == 1) t.ebe.iWrittenCubeData = 2;
						}
					}
				}
			}

			// only update mesh(es) if cube data actually changed
			if (bRecordChangeToCubeData == true)
			{
				// update within range of pattern stamp
				iRecordX1 = ((int)iRecordX1 / 10) * 10;
				iRecordY1 = ((int)iRecordY1 / 10) * 10;
				iRecordZ1 = ((int)iRecordZ1 / 10) * 10;
				iRecordX2 = ((int)iRecordX2 / 10) * 10;
				iRecordY2 = ((int)iRecordY2 / 10) * 10;
				iRecordZ2 = ((int)iRecordZ2 / 10) * 10;
				ebe_refreshmesh(ebebuild.iBuildObj, iRecordX1, iRecordY1, iRecordZ1, iRecordX2, iRecordY2, iRecordZ2);
			}
		}
	}
	else
	{
		// if off grid, and left click another EBE entity, switch to that one
		if (t.inputsys.mclick == 1)
		{
			int iFoundE = findentitycursorobj(-1);
			if (iFoundE > 0 && iFoundE != t.ebe.entityelementindex)
			{
				if (t.entityprofile[t.entityelement[iFoundE].bankindex].isebe != 0)
				{
					// change to new site
					ebe_newsite(iFoundE);
					return;
				}
			}
		}
	}

	sObject* pToolObject = GetObjectData(ebebuild.iToolObj);
	if (pToolObject) WickedCall_UpdateLimbsOfObject(pToolObject);
	sObject* pBuildObject = GetObjectData(ebebuild.iBuildObj);
	if (pToolObject) WickedCall_UpdateLimbsOfObject(pBuildObject);
}

void ebe_snapshottobuffer ( void )
{
	// pack cube data into undo buffer
	ebe_packsite ( &g_dwUndoBufferCount, &g_pUndoBufferPtr );

	// wipe redo as this snapshot is latest
	SAFE_DELETE ( g_pRedoBufferPtr );
	g_dwRedoBufferCount = 0;
}

void ebe_undo ( void )
{
	// ensure we are building a structure
	int iBuildObj = ebebuild.iBuildObj;
	if ( iBuildObj == 0 ) return;

	// extract undo buffer into cube data
	if ( g_pUndoBufferPtr != NULL )
	{
		// first snapshot cube data to redo buffer
		SAFE_DELETE ( g_pRedoBufferPtr );
		ebe_packsite ( &g_dwRedoBufferCount, &g_pRedoBufferPtr );

		// now unpack undo buffer to cube data
		ebe_unpacksite ( g_dwUndoBufferCount, g_pUndoBufferPtr );
		SAFE_DELETE ( g_pUndoBufferPtr );
		g_dwUndoBufferCount = 0;

		// refresh cube data to model
		ebebuild.iRefreshBuild = 20;
	}
}

void ebe_redo ( void )
{
	// ensure we are building a structure
	int iBuildObj = ebebuild.iBuildObj;
	if ( iBuildObj == 0 ) return;

	// revert back to last cube data, the undo was a mistake
	if ( g_pRedoBufferPtr != NULL )
	{
		// first pack cube data to undo buffer (if want to alternative UNDO/REDO)
		SAFE_DELETE ( g_pUndoBufferPtr );
		ebe_packsite ( &g_dwUndoBufferCount, &g_pUndoBufferPtr );

		// now put back the cube data from the redo buffer
		ebe_unpacksite ( g_dwRedoBufferCount, g_pRedoBufferPtr );
		SAFE_DELETE ( g_pRedoBufferPtr );
		g_dwRedoBufferCount = 0;

		// refresh cube data to model
		ebebuild.iRefreshBuild = 20;
	}
}

void ebe_physics_setupebestructure ( int tphyobj, int entityelementindex )
{
	// unpack data into cube data
	int entid = t.entityelement[entityelementindex].bankindex;
	DWORD dwRLESize = t.entityprofile[entid].ebe.dwRLESize;
	if ( dwRLESize > 0 )
	{
		DWORD* pRLEData = t.entityprofile[entid].ebe.pRLEData;
		ebe_unpacksite ( dwRLESize, pRLEData );
	}

	// Create material ref table
	DWORD dwMatRefCount = t.entityprofile[entid].ebe.dwMatRefCount;
	if ( dwMatRefCount < 16 ) dwMatRefCount = 16;
	int* pMatRefTable = new int[dwMatRefCount];
	memset ( pMatRefTable, 0, sizeof ( pMatRefTable ) );
	if ( t.entityprofile[entid].ebe.dwMatRefCount > 0 )
		for ( int n = 0; n < dwMatRefCount; n++ )
			pMatRefTable[n] = t.entityprofile[entid].ebe.iMatRef[n];

	// 3D flood fill to make box list
	for ( int iThisX = 0; iThisX < CUBEAREASIZE; iThisX++ )
	{
		for ( int iThisY = 0; iThisY < CUBEAREASIZE; iThisY++ )
		{
			for ( int iThisZ = 0; iThisZ < CUBEAREASIZE; iThisZ++ )
			{
				pTemp[iThisX][iThisY][iThisZ] = pCubes[iThisX][iThisY][iThisZ];
			}
		}
	}
	int iMyColBoxIndex = 0;
	for ( int iThisX = 0; iThisX < CUBEAREASIZE; iThisX++ )
	{
		for ( int iThisY = 0; iThisY < CUBEAREASIZE; iThisY++ )
		{
			for ( int iThisZ = 0; iThisZ < CUBEAREASIZE; iThisZ++ )
			{
				unsigned char cBitCube = pTemp[iThisX][iThisY][iThisZ];
				if ( cBitCube != 0 )
				{
					// only wipe out if same texture
					unsigned char cTexIndexA = (cBitCube & (15<<4)) >> 4;
					int iMatRefIndexA = pMatRefTable[cTexIndexA];
					pMyColBox[iMyColBoxIndex].iMaterialIndex = iMatRefIndexA;
					pMyColBox[iMyColBoxIndex].x1 = iThisX;
					pMyColBox[iMyColBoxIndex].y1 = iThisY;
					pMyColBox[iMyColBoxIndex].z1 = iThisZ;
					pMyColBox[iMyColBoxIndex].x2 = iThisX;
					pMyColBox[iMyColBoxIndex].y2 = iThisY;
					pMyColBox[iMyColBoxIndex].z2 = iThisZ;
					int iStopped = 0;
					int iXOkay = 1;
					int iYOkay = 1;
					int iZOkay = 1;
					while ( iXOkay == 1 || iYOkay == 1 || iZOkay == 1 )
					{
						for ( int iTryPass = 0; iTryPass < 3; iTryPass++ )
						{
							bool bDoAxis = false;
							if ( iTryPass == 0 && iXOkay == 1 ) bDoAxis = true;
							if ( iTryPass == 1 && iYOkay == 1 ) bDoAxis = true;
							if ( iTryPass == 2 && iZOkay == 1 ) bDoAxis = true;
							if ( bDoAxis == true )
							{
								if ( iTryPass==0 ) 
								{
									pMyColBox[iMyColBoxIndex].x2 = pMyColBox[iMyColBoxIndex].x2 + 1;
									if ( pMyColBox[iMyColBoxIndex].x2 >= 200 )
									{
										pMyColBox[iMyColBoxIndex].x2 = 199;
										bDoAxis = false;
										iXOkay = 0;
									}
								}
								if ( iTryPass==1 ) 
								{
									pMyColBox[iMyColBoxIndex].y2 = pMyColBox[iMyColBoxIndex].y2 + 1;
									if ( pMyColBox[iMyColBoxIndex].y2 >= 200 )
									{
										pMyColBox[iMyColBoxIndex].y2 = 199;
										bDoAxis = false;
										iYOkay = 0;
									}
								}
								if ( iTryPass==2 ) 
								{
									pMyColBox[iMyColBoxIndex].z2 = pMyColBox[iMyColBoxIndex].z2 + 1;
									if ( pMyColBox[iMyColBoxIndex].z2 >= 200 )
									{
										pMyColBox[iMyColBoxIndex].z2 = 199;
										bDoAxis = false;
										iZOkay = 0;
									}
								}
								if ( bDoAxis == true )
								{
									int iAllSolid = 1;
									for ( int iTX = pMyColBox[iMyColBoxIndex].x1; iTX <= pMyColBox[iMyColBoxIndex].x2; iTX++ )
									{
										for ( int iTY = pMyColBox[iMyColBoxIndex].y1; iTY <= pMyColBox[iMyColBoxIndex].y2; iTY++ )
										{
											for ( int iTZ = pMyColBox[iMyColBoxIndex].z1; iTZ <= pMyColBox[iMyColBoxIndex].z2; iTZ++ )
											{
												unsigned char cTexIndexB = (pTemp[iTX][iTY][iTZ] & (15<<4)) >> 4;
												int iMatRefIndexB = pMatRefTable[cTexIndexB];
												if ( pTemp[iTX][iTY][iTZ] == 0 || iMatRefIndexA != iMatRefIndexB )
												{
													// end this axis
													iAllSolid = 0;
													iTX = pMyColBox[iMyColBoxIndex].x2;
													iTY = pMyColBox[iMyColBoxIndex].y2;
													iTZ = pMyColBox[iMyColBoxIndex].z2;
													break;
												}
											}
										}
									}
									if ( iAllSolid == 0 )
									{
										// failed, step back and flag axis as no more
										if ( iTryPass==0 ) 
										{
											pMyColBox[iMyColBoxIndex].x2 = pMyColBox[iMyColBoxIndex].x2 - 1;
											if ( pMyColBox[iMyColBoxIndex].x2 < pMyColBox[iMyColBoxIndex].x1 )
											{
												pMyColBox[iMyColBoxIndex].x2 = pMyColBox[iMyColBoxIndex].x1;
											}
											iXOkay = 0;
										}
										if ( iTryPass==1 ) 
										{
											pMyColBox[iMyColBoxIndex].y2 = pMyColBox[iMyColBoxIndex].y2 - 1;
											if ( pMyColBox[iMyColBoxIndex].y2 < pMyColBox[iMyColBoxIndex].y1 )
											{
												pMyColBox[iMyColBoxIndex].y2 = pMyColBox[iMyColBoxIndex].y1;
											}
											iYOkay = 0;
										}
										if ( iTryPass==2 ) 
										{
											pMyColBox[iMyColBoxIndex].z2 = pMyColBox[iMyColBoxIndex].z2 - 1;
											if ( pMyColBox[iMyColBoxIndex].z2 < pMyColBox[iMyColBoxIndex].z1 )
											{
												pMyColBox[iMyColBoxIndex].z2 = pMyColBox[iMyColBoxIndex].z1;
											}
											iZOkay = 0;
										}
									}
								}
							}
						}
					}
					// now erase all within this box area
					for ( int iDX = pMyColBox[iMyColBoxIndex].x1; iDX <= pMyColBox[iMyColBoxIndex].x2; iDX++ )
					{
						for ( int iDY = pMyColBox[iMyColBoxIndex].y1; iDY <= pMyColBox[iMyColBoxIndex].y2; iDY++ )
						{
							for ( int iDZ = pMyColBox[iMyColBoxIndex].z1; iDZ <= pMyColBox[iMyColBoxIndex].z2; iDZ++ )
							{
								pTemp[iDX][iDY][iDZ] = 0;
							}
						}
					}
					// move to next box list index
					iMyColBoxIndex++; if ( iMyColBoxIndex >= CUBECOLBOXMAX ) iMyColBoxIndex = CUBECOLBOXMAX-1;
				}
			}
		}
	}

	// Create optimized collision boxes from above 3d flood fill temp array
	ODEStartStaticObject ( tphyobj );
	for ( int iBoxIndex = 0; iBoxIndex < iMyColBoxIndex; iBoxIndex++ )
	{
		int iX1 = pMyColBox[iBoxIndex].x1;
		int iY1 = pMyColBox[iBoxIndex].y1;
		int iZ1 = pMyColBox[iBoxIndex].z1;
		int iW = 1+(pMyColBox[iBoxIndex].x2 - pMyColBox[iBoxIndex].x1);
		int iH = 1+(pMyColBox[iBoxIndex].y2 - pMyColBox[iBoxIndex].y1);
		int iD = 1+(pMyColBox[iBoxIndex].z2 - pMyColBox[iBoxIndex].z1);
		if ( ObjectExist(g.tempimporterlistobject) ) DeleteObject ( g.tempimporterlistobject );
		int iObjectToUse = g.tempimporterlistobject;
		int iSizeX = iW*5;
		int iSizeY = iH*5;
		int iSizeZ = iD*5;
		MakeObjectBox ( iObjectToUse, iSizeX, iSizeY, iSizeZ );
		PositionObject ( iObjectToUse, (iX1*5)+(iSizeX/2), (iY1*5)+(iSizeY/2), (iZ1*5)+(iSizeZ/2) );
		RotateObject ( iObjectToUse, 0, 0, 0 );
		ODEAddStaticObjectBox ( tphyobj, iObjectToUse, pMyColBox[iBoxIndex].iMaterialIndex );
	}
	if ( ObjectExist(g.tempimporterlistobject) ) DeleteObject ( g.tempimporterlistobject );
	ODEEndStaticObject ( tphyobj, 0 );

	// free resources
	SAFE_DELETE ( pMatRefTable );
}

void ebe_optimize_e ( void )
{
	// early out
	if ( t.ebe.entityelementindex == 0 ) return;

	// recreate entity using optimized polygons
	int e = t.ebe.entityelementindex;
	int iObj = t.entityelement[e].obj;

	// optimize the entity element object
	ebe_optimize_object ( iObj, t.entityelement[e].bankindex );

	// the object to optimize
	sObject* pObject = g_ObjectList[iObj];

	//Update master.
	int iEntityBankID = t.entityelement[e].bankindex;
	t.sourceobj = g.entitybankoffset + iEntityBankID;
	if (t.grideleprof.bCustomWickedMaterialActive) 
	{
		// in the case of EBE, we want to point to unique texture name for this structure
		// not the default textures provided during editing
		cstr pUniqieTexName = ebe_constructlongTXPname("_").Get();
		for (int iM = 0; iM < pObject->iMeshCount; iM++)
		{
			strcpy(pObject->ppMeshList[iM]->pTextures[0].pName, "ebebank\\default\\");
			strcat(pObject->ppMeshList[iM]->pTextures[0].pName, cstr(pUniqieTexName + "color.dds").Get());
		}
		WickedCall_TextureObject(pObject, NULL);

		// copies structure object details being edited to WEmaterial
		Wicked_Copy_JustTextureNames_To_Grideleprof((void*)pObject, 3);
		t.grideleprof.WEMaterial.MaterialActive = true;

		// copy object parent details to entity element instance with correct WEmaterials settings
		t.entityelement[e].eleprof.WEMaterial = t.grideleprof.WEMaterial;
		t.entityelement[e].eleprof.bCustomWickedMaterialActive = t.grideleprof.bCustomWickedMaterialActive;
		Wicked_Set_Material_From_grideleprof((void*)pObject, 3);
		//PE: Finally copy material to all meshed in new structure.
		Wicked_Update_All_Materials((void*)pObject, 3);
		//And master.
		
		pObject = g_ObjectList[t.sourceobj];
		if (pObject)
		{
			Wicked_Set_Material_From_grideleprof((void*)pObject, 3);
			Wicked_Update_All_Materials((void*)pObject, 3);
		}
	}

	// ensure parent is updated (for things like extracting, lightmapping, etc)
	ebe_updateparent ( e );

	// Set collision of working object active so can be ray cast detected
	pObject = g_ObjectList[iObj];
	SetColOn ( pObject );

}

void ebe_optimize_object ( int iObj, int iEntID )
{
	// Create material ref table
	DWORD dwMatRefCount = t.entityprofile[iEntID].ebe.dwMatRefCount;
	if ( dwMatRefCount < 16 ) dwMatRefCount = 16;
	int* pMatRefTable = new int[dwMatRefCount];
	memset ( pMatRefTable, 0, sizeof ( pMatRefTable ) );
	if ( t.entityprofile[iEntID].ebe.dwMatRefCount > 0 )
		for ( int n = 0; n < dwMatRefCount; n++ )
			pMatRefTable[n] = t.entityprofile[iEntID].ebe.iMatRef[n];

	// copy cube data to temp array
	for ( int iThisX = 0; iThisX < CUBEAREASIZE; iThisX++ )
		for ( int iThisY = 0; iThisY < CUBEAREASIZE; iThisY++ )
			for ( int iThisZ = 0; iThisZ < CUBEAREASIZE; iThisZ++ )
				pTemp[iThisX][iThisY][iThisZ] = pCubes[iThisX][iThisY][iThisZ];

	// traverse cube data, find boxes of same texture
	int iMyColBoxIndex = 0;
	for ( int iThisX = 0; iThisX < CUBEAREASIZE; iThisX++ )
	{
		for ( int iThisY = 0; iThisY < CUBEAREASIZE; iThisY++ )
		{
			for ( int iThisZ = 0; iThisZ < CUBEAREASIZE; iThisZ++ )
			{
				unsigned char cBitCube = pTemp[iThisX][iThisY][iThisZ];
				if ( cBitCube != 0 )
				{
					pMyColBox[iMyColBoxIndex].x1 = iThisX;
					pMyColBox[iMyColBoxIndex].y1 = iThisY;
					pMyColBox[iMyColBoxIndex].z1 = iThisZ;
					pMyColBox[iMyColBoxIndex].x2 = iThisX;
					pMyColBox[iMyColBoxIndex].y2 = iThisY;
					pMyColBox[iMyColBoxIndex].z2 = iThisZ;
					unsigned char cTexIndex = (cBitCube & (15<<4)) >> 4;
					pMyColBox[iMyColBoxIndex].cCubeTexIndex = cTexIndex;
					int iMatRefIndexA = pMatRefTable[cTexIndex];
					pMyColBox[iMyColBoxIndex].iMaterialIndex = iMatRefIndexA;
					int iRegionX = iThisX / 20; // must split boxes into the 20x20x20 regions to preserve UV wrap (as using atlas)
					int iRegionY = iThisY / 20;
					int iRegionZ = iThisZ / 20;
					int iStopped = 0;
					int iXOkay = 1;
					int iYOkay = 1;
					int iZOkay = 1;
					while ( iXOkay == 1 || iYOkay == 1 || iZOkay == 1 )
					{
						for ( int iTryPass = 0; iTryPass < 3; iTryPass++ )
						{
							bool bDoAxis = false;
							if ( iTryPass == 0 && iXOkay == 1 ) bDoAxis = true;
							if ( iTryPass == 1 && iYOkay == 1 ) bDoAxis = true;
							if ( iTryPass == 2 && iZOkay == 1 ) bDoAxis = true;
							if ( bDoAxis == true )
							{
								if ( iTryPass==0 ) 
								{
									pMyColBox[iMyColBoxIndex].x2 = pMyColBox[iMyColBoxIndex].x2 + 1;
									if ( pMyColBox[iMyColBoxIndex].x2 >= 200 )
									{
										pMyColBox[iMyColBoxIndex].x2 = 199;
										bDoAxis = false;
										iXOkay = 0;
									}
									if ( pMyColBox[iMyColBoxIndex].x2 / 20 != iRegionX )
									{
										pMyColBox[iMyColBoxIndex].x2 = pMyColBox[iMyColBoxIndex].x2 - 1;
										bDoAxis = false;
										iXOkay = 0;
									}
								}
								if ( iTryPass==1 ) 
								{
									pMyColBox[iMyColBoxIndex].y2 = pMyColBox[iMyColBoxIndex].y2 + 1;
									if ( pMyColBox[iMyColBoxIndex].y2 >= 200 )
									{
										pMyColBox[iMyColBoxIndex].y2 = 199;
										bDoAxis = false;
										iYOkay = 0;
									}
									if ( pMyColBox[iMyColBoxIndex].y2 / 20 != iRegionY )
									{
										pMyColBox[iMyColBoxIndex].y2 = pMyColBox[iMyColBoxIndex].y2 - 1;
										bDoAxis = false;
										iYOkay = 0;
									}
								}
								if ( iTryPass==2 ) 
								{
									pMyColBox[iMyColBoxIndex].z2 = pMyColBox[iMyColBoxIndex].z2 + 1;
									if ( pMyColBox[iMyColBoxIndex].z2 >= 200 )
									{
										pMyColBox[iMyColBoxIndex].z2 = 199;
										bDoAxis = false;
										iZOkay = 0;
									}
									if ( pMyColBox[iMyColBoxIndex].z2 / 20 != iRegionZ )
									{
										pMyColBox[iMyColBoxIndex].z2 = pMyColBox[iMyColBoxIndex].z2 - 1;
										bDoAxis = false;
										iZOkay = 0;
									}
								}
								if ( bDoAxis == true )
								{
									int iAllSolid = 1;
									for ( int iTX = pMyColBox[iMyColBoxIndex].x1; iTX <= pMyColBox[iMyColBoxIndex].x2; iTX++ )
									{
										for ( int iTY = pMyColBox[iMyColBoxIndex].y1; iTY <= pMyColBox[iMyColBoxIndex].y2; iTY++ )
										{
											for ( int iTZ = pMyColBox[iMyColBoxIndex].z1; iTZ <= pMyColBox[iMyColBoxIndex].z2; iTZ++ )
											{
												bool bGapOrDifferentTexture = false;
												unsigned char cBitCube = pTemp[iTX][iTY][iTZ];
												unsigned char cThisTexIndex = (cBitCube & (15<<4)) >> 4;
												if ( cBitCube == 0 ) bGapOrDifferentTexture = true;
												if ( cTexIndex != cThisTexIndex ) bGapOrDifferentTexture = true; // separate by texture to get boxes of one tex type each
												int iMatRefIndexB = pMatRefTable[cThisTexIndex];
												if ( iMatRefIndexA != iMatRefIndexB ) bGapOrDifferentTexture = true; // also separate by material to get boxes of one material type each
												if ( bGapOrDifferentTexture == true )
												{
													// end this axis
													iAllSolid = 0;
													iTX = pMyColBox[iMyColBoxIndex].x2;
													iTY = pMyColBox[iMyColBoxIndex].y2;
													iTZ = pMyColBox[iMyColBoxIndex].z2;
													break;
												}
											}
										}
									}
									if ( iAllSolid == 0 )
									{
										// failed, step back and flag axis as no more
										if ( iTryPass==0 ) 
										{
											pMyColBox[iMyColBoxIndex].x2 = pMyColBox[iMyColBoxIndex].x2 - 1;
											if ( pMyColBox[iMyColBoxIndex].x2 < pMyColBox[iMyColBoxIndex].x1 )
											{
												pMyColBox[iMyColBoxIndex].x2 = pMyColBox[iMyColBoxIndex].x1;
											}
											iXOkay = 0;
										}
										if ( iTryPass==1 ) 
										{
											pMyColBox[iMyColBoxIndex].y2 = pMyColBox[iMyColBoxIndex].y2 - 1;
											if ( pMyColBox[iMyColBoxIndex].y2 < pMyColBox[iMyColBoxIndex].y1 )
											{
												pMyColBox[iMyColBoxIndex].y2 = pMyColBox[iMyColBoxIndex].y1;
											}
											iYOkay = 0;
										}
										if ( iTryPass==2 ) 
										{
											pMyColBox[iMyColBoxIndex].z2 = pMyColBox[iMyColBoxIndex].z2 - 1;
											if ( pMyColBox[iMyColBoxIndex].z2 < pMyColBox[iMyColBoxIndex].z1 )
											{
												pMyColBox[iMyColBoxIndex].z2 = pMyColBox[iMyColBoxIndex].z1;
											}
											iZOkay = 0;
										}
									}
								}
							}
						}
					}
					// now erase all within this box area
					for ( int iDX = pMyColBox[iMyColBoxIndex].x1; iDX <= pMyColBox[iMyColBoxIndex].x2; iDX++ )
					{
						for ( int iDY = pMyColBox[iMyColBoxIndex].y1; iDY <= pMyColBox[iMyColBoxIndex].y2; iDY++ )
						{
							for ( int iDZ = pMyColBox[iMyColBoxIndex].z1; iDZ <= pMyColBox[iMyColBoxIndex].z2; iDZ++ )
							{
								pTemp[iDX][iDY][iDZ] = 0;
							}
						}
					}
					// move to next box list index
					iMyColBoxIndex++; if ( iMyColBoxIndex >= CUBECOLBOXMAX ) iMyColBoxIndex = CUBECOLBOXMAX-1;
				}
			}
		}
	}

	// Create optimized poly boxes
	float fX = ObjectPositionX(iObj);
	float fY = ObjectPositionY(iObj);
	float fZ = ObjectPositionZ(iObj);
	float fRX = ObjectAngleX(iObj);
	float fRY = ObjectAngleY(iObj);
	float fRZ = ObjectAngleZ(iObj);
	DeleteObject ( iObj );
	MakeObject ( iObj, g.meshebemarker, 0 );
	sObject* pObjectRemove = GetObjectData(iObj);
	WickedCall_RemoveObject(pObjectRemove);
	// so we can handle custom material applied to this EBE
	if (t.ebe.entityelementindex > 0)
	{
		WickedSetElementId(t.ebe.entityelementindex);
		WickedSetEntityId(t.entityelement[t.ebe.entityelementindex].bankindex);
	}
	PositionObject ( iObj, fX, fY, fZ );
	RotateObject ( iObj, fRX, fRY, fRZ );

	// create meshes (one per material currently materials range from 0 to 18)
	int iMeshIndex = 0;
	for ( int iMatRefIndex = 0; iMatRefIndex <= 18; iMatRefIndex++ )
	{
		// this material in box collection?
		int iCountMaterialForThisMesh = 0;
		for ( int iBoxIndex = 0; iBoxIndex < iMyColBoxIndex; iBoxIndex++ )
			if ( pMyColBox[iBoxIndex].iMaterialIndex == iMatRefIndex )
				iCountMaterialForThisMesh++;
		if ( iCountMaterialForThisMesh == 0 )
			continue;

		// create this mesh (we found a match material)
		iMeshIndex++;
		AddLimb ( iObj, iMeshIndex, g.meshebemarker );
		sObject* pObject = GetObjectData ( iObj );
		sMesh* pMesh = pObject->ppMeshList[iMeshIndex];
		pMesh->bVBRefreshRequired = true;
		pMesh->bMeshHasBeenReplaced = true;
		DWORD dwVertPos = iCountMaterialForThisMesh * 24;
		int secure_vertex_data = 62400; //PE: must be divideable by 24
		if (dwVertPos > secure_vertex_data)
			dwVertPos = secure_vertex_data; //PE: Max vertex. per material.
		DWORD dwIndicePos = (dwVertPos/2)*3;

		SetupMeshFVFData ( pMesh, pMesh->dwFVF, dwVertPos, dwIndicePos, false );
		pMesh->iPrimitiveType = GGPT_TRIANGLELIST;
		pMesh->iDrawVertexCount = dwVertPos;
		pMesh->iDrawPrimitives  = dwIndicePos / 3;

		// assign material to mesh
		pMesh->Collision.dwArbitaryValue = iMatRefIndex;

		// populate with poly boxes
		DWORD dwColor = GGCOLOR(1,1,1,1);
		float fU = 1.0f/4.0f;
		float fV = 1.0f/4.0f;
		dwVertPos = 0;
		dwIndicePos = 0;
		for ( int iBoxIndex = 0; iBoxIndex < iMyColBoxIndex; iBoxIndex++ )
		{
			// process box if correct material
			if ( pMyColBox[iBoxIndex].iMaterialIndex != iMatRefIndex )
				continue;
			if (dwVertPos >= secure_vertex_data) //PE: Make sure we never crash.
				continue;
			// work out size and position of unified box
			unsigned char cCubeTexIndex = pMyColBox[iBoxIndex].cCubeTexIndex;
			int iX1 = pMyColBox[iBoxIndex].x1;
			int iY1 = pMyColBox[iBoxIndex].y1;
			int iZ1 = pMyColBox[iBoxIndex].z1;
			int iW = 1+(pMyColBox[iBoxIndex].x2 - pMyColBox[iBoxIndex].x1);
			int iH = 1+(pMyColBox[iBoxIndex].y2 - pMyColBox[iBoxIndex].y1);
			int iD = 1+(pMyColBox[iBoxIndex].z2 - pMyColBox[iBoxIndex].z1);
			int iAbsX = iX1;
			int iAbsY = iY1;
			int iAbsZ = iZ1;
			int iAbs2X = iX1+(iW-1);
			int iAbs2Y = iY1+(iH-1);
			int iAbs2Z = iZ1+(iD-1);
			if ( ObjectExist(g.tempimporterlistobject) ) DeleteObject ( g.tempimporterlistobject );
			int iObjectToUse = g.tempimporterlistobject;
			int iSizeX = iW*5;
			int iSizeY = iH*5;
			int iSizeZ = iD*5;

			// UV for texture choice
			int iRow = cCubeTexIndex / 4;
			int iCol = cCubeTexIndex - (iRow*4);
			float fBitU = fU / 20.0f;
			float fBitV = fV / 20.0f;
			float fCoverageU = fBitU * (float)(iAbsX%20);
			float fCoverageUR = fBitU * (float)(19-(iAbs2X%20));
			float fCoverageV = fBitV * (float)(19-(iAbs2Y%20));
			float fCoverageVR = fBitV * (float)(iAbsY%20);
			float fCoverageW = fBitU * (float)(iAbsZ%20);
			float fCoverageWR = fBitU * (float)(19-(iAbs2Z%20));
			float fU1 = (fU * iCol)+fCoverageU;
			float fU1R = (fU * iCol)+fCoverageUR;
			float fV1 = (fV * iRow)+fCoverageV;
			float fV1R = (fV * iRow)+fCoverageVR;
			float fW1 = (fU * iCol)+fCoverageW;
			float fZ1 = (fU * iRow)+fCoverageW;
			float fW1R = (fU * iCol)+fCoverageWR;
			float fZ1R = (fU * iRow)+fCoverageWR;
			float fU2 = fU1 + (fBitU*iW);
			float fU2R = fU1R + (fBitU*iW);
			float fV2 = fV1 + (fBitV*iH);
			float fW2 = fW1 + (fBitU*iD);
			float fZ2 = fZ1 + (fBitU*iD);
			float fW2R = fW1R + (fBitU*iD);
			float fZ2R = fZ1R + (fBitU*iD);

			// process UV for seamless texturing
			ebe_makeseamless ( iRow, iCol, &fU1, &fU1R, &fV1, &fV1R, &fW1, &fZ1, &fW1R, &fZ1R, &fU2, &fU2R, &fV2, &fW2, &fZ2, &fW2R, &fZ2R );

			// create one poly box
			float fWidth1 = (iX1*5);
			float fWidth2 = (iX1*5)+iSizeX;
			float fHeight1 = (iY1*5);
			float fHeight2 = (iY1*5)+iSizeY;
			float fDepth1 = (iZ1*5);
			float fDepth2 = (iZ1*5)+iSizeZ;
			SetupStandardVertex ( pMesh->dwFVF, pMesh->pVertexData, dwVertPos+0, fWidth1, fHeight2, fDepth1,  0.0f,  0.0f, -1.0f, dwColor, fU1, fV1 );	// front
			SetupStandardVertex ( pMesh->dwFVF, pMesh->pVertexData, dwVertPos+1, fWidth2, fHeight2, fDepth1,  0.0f,  0.0f, -1.0f, dwColor, fU2, fV1 );
			SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData, dwVertPos+2, fWidth2, fHeight1, fDepth1,  0.0f,  0.0f, -1.0f, dwColor, fU2, fV2 );
			SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData, dwVertPos+3, fWidth1, fHeight1, fDepth1,  0.0f,  0.0f, -1.0f, dwColor, fU1, fV2 );
			SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData, dwVertPos+4, fWidth1, fHeight2, fDepth2,  0.0f,  0.0f,  1.0f, dwColor, fU2R, fV1 );	// back
			SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData, dwVertPos+5, fWidth1, fHeight1, fDepth2,  0.0f,  0.0f,  1.0f, dwColor, fU2R, fV2 );
			SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData, dwVertPos+6, fWidth2, fHeight1, fDepth2,  0.0f,  0.0f,  1.0f, dwColor, fU1R, fV2 );
			SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData, dwVertPos+7, fWidth2, fHeight2, fDepth2,  0.0f,  0.0f,  1.0f, dwColor, fU1R, fV1 );
			SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData, dwVertPos+8, fWidth1, fHeight2, fDepth2,	 0.0f,  1.0f,  0.0f, dwColor, fU1, fZ1R );	// top
			SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData, dwVertPos+9, fWidth2, fHeight2, fDepth2,	 0.0f,  1.0f,  0.0f, dwColor, fU2, fZ1R );
			SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData, dwVertPos+10, fWidth2, fHeight2, fDepth1,	 0.0f,  1.0f,  0.0f, dwColor, fU2, fZ2R );
			SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData, dwVertPos+11, fWidth1, fHeight2, fDepth1,	 0.0f,  1.0f,  0.0f, dwColor, fU1, fZ2R );
			SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData, dwVertPos+12, fWidth1, fHeight1, fDepth2,  0.0f, -1.0f,  0.0f, dwColor, fU1, fZ2 );	// bottom
			SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData, dwVertPos+13, fWidth1, fHeight1, fDepth1,	 0.0f, -1.0f,  0.0f, dwColor, fU1, fZ1 );
			SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData, dwVertPos+14, fWidth2, fHeight1, fDepth1,	 0.0f, -1.0f,  0.0f, dwColor, fU2, fZ1 );
			SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData, dwVertPos+15, fWidth2, fHeight1, fDepth2,	 0.0f, -1.0f,  0.0f, dwColor, fU2, fZ2 );
			SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData, dwVertPos+16, fWidth2, fHeight2, fDepth1,	 1.0f,  0.0f,  0.0f, dwColor, fW1, fV1 );	// right
			SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData, dwVertPos+17, fWidth2, fHeight2, fDepth2,	 1.0f,  0.0f,  0.0f, dwColor, fW2, fV1 );
			SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData, dwVertPos+18, fWidth2, fHeight1, fDepth2,	 1.0f,  0.0f,  0.0f, dwColor, fW2, fV2 );
			SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData, dwVertPos+19, fWidth2, fHeight1, fDepth1,	 1.0f,  0.0f,  0.0f, dwColor, fW1, fV2 );
			SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData, dwVertPos+20, fWidth1, fHeight2, fDepth1,	-1.0f,  0.0f,  0.0f, dwColor, fW2R, fV1 );	// left
			SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData, dwVertPos+21, fWidth1, fHeight1, fDepth1,	-1.0f,  0.0f,  0.0f, dwColor, fW2R, fV2 );
			SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData, dwVertPos+22, fWidth1, fHeight1, fDepth2,	-1.0f,  0.0f,  0.0f, dwColor, fW1R, fV2 );
			SetupStandardVertex ( pMesh->dwFVF,	pMesh->pVertexData, dwVertPos+23, fWidth1, fHeight2, fDepth2,	-1.0f,  0.0f,  0.0f, dwColor, fW1R, fV1 );

			// and now fill in the index list
			pMesh->pIndices [ dwIndicePos+0 ] = dwVertPos+0;		pMesh->pIndices [ dwIndicePos+1 ] = dwVertPos+1;		pMesh->pIndices [ dwIndicePos+2 ] = dwVertPos+2;
			pMesh->pIndices [ dwIndicePos+3 ] = dwVertPos+2;		pMesh->pIndices [ dwIndicePos+4 ] = dwVertPos+3;		pMesh->pIndices [ dwIndicePos+5 ] = dwVertPos+0;
			pMesh->pIndices [ dwIndicePos+6 ] = dwVertPos+4;		pMesh->pIndices [ dwIndicePos+7 ] = dwVertPos+5;		pMesh->pIndices [ dwIndicePos+8 ] = dwVertPos+6;
			pMesh->pIndices [ dwIndicePos+9 ] = dwVertPos+6;		pMesh->pIndices [ dwIndicePos+10 ] = dwVertPos+7;		pMesh->pIndices [ dwIndicePos+11 ] = dwVertPos+4;
			pMesh->pIndices [ dwIndicePos+12 ] = dwVertPos+8;		pMesh->pIndices [ dwIndicePos+13 ] = dwVertPos+9;		pMesh->pIndices [ dwIndicePos+14 ] = dwVertPos+10;
			pMesh->pIndices [ dwIndicePos+15 ] = dwVertPos+10;		pMesh->pIndices [ dwIndicePos+16 ] = dwVertPos+11;		pMesh->pIndices [ dwIndicePos+17 ] = dwVertPos+8;
			pMesh->pIndices [ dwIndicePos+18 ] = dwVertPos+12;		pMesh->pIndices [ dwIndicePos+19 ] = dwVertPos+13;		pMesh->pIndices [ dwIndicePos+20 ] = dwVertPos+14;
			pMesh->pIndices [ dwIndicePos+21 ] = dwVertPos+14;		pMesh->pIndices [ dwIndicePos+22 ] = dwVertPos+15;		pMesh->pIndices [ dwIndicePos+23 ] = dwVertPos+12;
			pMesh->pIndices [ dwIndicePos+24 ] = dwVertPos+16;		pMesh->pIndices [ dwIndicePos+25 ] = dwVertPos+17;		pMesh->pIndices [ dwIndicePos+26 ] = dwVertPos+18;
			pMesh->pIndices [ dwIndicePos+27 ] = dwVertPos+18;		pMesh->pIndices [ dwIndicePos+28 ] = dwVertPos+19;		pMesh->pIndices [ dwIndicePos+29 ] = dwVertPos+16;
			pMesh->pIndices [ dwIndicePos+30 ] = dwVertPos+20;		pMesh->pIndices [ dwIndicePos+31 ] = dwVertPos+21;		pMesh->pIndices [ dwIndicePos+32 ] = dwVertPos+22;
			pMesh->pIndices [ dwIndicePos+33 ] = dwVertPos+22;		pMesh->pIndices [ dwIndicePos+34 ] = dwVertPos+23;		pMesh->pIndices [ dwIndicePos+35 ] = dwVertPos+20;

			// next cube
			dwVertPos+=24;
			dwIndicePos+=36;
		}

		// add the worked mesh to buffers (unique VB/IB buffer for this new mesh)
		m_ObjectManager.AddObjectMeshToBuffers ( pMesh, true );
	}

	// work out bounds/radius of entity
	CalculateObjectBounds ( iObj );

	// Apply textures
	 image_setlegacyimageloading(true);
	 int iTexD = loadinternaltexture(cstr(cstr("ebebank\\default\\") + ebe_constructlongTXPname("_color.dds")).Get());
	 int iTexN = loadinternaltexture(cstr(cstr("ebebank\\default\\") + ebe_constructlongTXPname("_normal.dds")).Get());
	 int iTexS = loadinternaltexture(cstr(cstr("ebebank\\default\\") + ebe_constructlongTXPname("_surface.dds")).Get());
	 image_setlegacyimageloading(false);
	 t.entityprofile[iEntID].texdid = iTexD;
	 t.entityprofile[iEntID].texnid = iTexN;
	 t.entityprofile[iEntID].texsid = iTexS;
	TextureObject ( iObj, 0, iTexD );
	SetObjectTransparency ( iObj, 0 );
	
	// now apply regular entity shader
	int iEffectIndex = loadinternaleffect("effectbank\\reloaded\\apbr_basic.fx");
	SetObjectEffect ( iObj, iEffectIndex );
	SetObjectMask ( iObj, 0x1+(1<<31) );

	// free resources
	SAFE_DELETE ( pMatRefTable );

	// finally submit optimized to wicked
	sObject* pObject = GetObjectData(iObj);
	WickedCall_AddObject(pObject);
	WickedCall_UpdateObject(pObject);
	WickedCall_UpdateLimbsOfObject(pObject);
	WickedCall_TextureObject(pObject,NULL);
	// so we can handle custom material applied to this EBE
	WickedSetElementId(0);
	WickedSetEntityId(-1);
}

void ebe_reset ( void )
{
	if ( t.ebe.on == 0 )
	{
		// early out if not ready for this (i.e. no init performed)
		if ( t.ebe.entityelementindex == 0 ) return;

		// Set the tool object we will be using
		ebebuild.iToolObj = g.ebeobjectbankoffset + 0;

		// reset defaults when enter build mode
		if ( t.ebe.entityelementindex != t.ebe.lastentityelementindex )
		{
			// only if we switch to a new EBE site (so can edit/leave/edit and keep settings)
			t.ebe.lastentityelementindex = t.ebe.entityelementindex;
			ebebuild.iCurrentGridLayer = 0;
			ebebuild.iCursorRotation = 0;
			ebebuild.iCurrentTexture = 0;

			// reset pattern selection to [3] FLOOR SHAPE (most common and visual to start with)
			ebe_loadpattern ( t.ebebank_s[3].Get() );
		}

		// update edit grid to correct grid height
		float fCurrentGridLayerAbsHeight = ebebuild.iCurrentGridLayer*5.0f;
		OffsetLimb ( ebebuild.iToolObj, 1, 500, fCurrentGridLayerAbsHeight + 0.05f, 500 );

		// update cursor to start when new construction reset
		ebe_settexturehighlight();

		// show if previously hidden
		ebe_show ();

		// begin tool work
		t.ebe.on = 1;
		bBuilder_Properties_Window = true;
	}
}

void ebe_hardreset ( void )
{
	// full reset of editor state
	t.ebe.lastentityelementindex = -1;
	ebebuild.iCurrentGridLayer = 0;
	ebebuild.iCursorRotation = 0;
	ebebuild.iCurrentTexture = 0;

	// force reset when new site created/loaded
	t.ebe.on = 0;
}

void ebe_hide ( void )
{
	if ( t.ebe.on == 1 )
	{
		ebe_finishsite();
		if ( ebebuild.iTexturePanelSprite[0] > 0 && ebebuild.iToolObj > 0 )
		{
			if (!bDisableAllSprites) 
			{
				for (int n = 0; n < 16; n++) if (SpriteExist(ebebuild.iMatSpr[n]) == 1) HideSprite(ebebuild.iMatSpr[n]);
				if (SpriteExist(ebebuild.iEBETexHelpSpr) == 1) HideSprite(ebebuild.iEBETexHelpSpr);
				if (SpriteExist(ebebuild.iEBEHelpSpr) == 1) HideSprite(ebebuild.iEBEHelpSpr);
				if (SpriteExist(ebebuild.iTexturePanelHighSprite) == 1) HideSprite(ebebuild.iTexturePanelHighSprite);
				for (int iTex = 0; iTex < EBETEXPANELSPRMAX; iTex++)
				{
					if (SpriteExist(ebebuild.iTexturePanelSprite[iTex]) == 1) HideSprite(ebebuild.iTexturePanelSprite[iTex]);
				}
			}
			if ( ObjectExist ( ebebuild.iToolObj ) == 1 ) HideObject ( ebebuild.iToolObj );
		}
		t.ebe.on = 0;
		ImGui::SetWindowFocus(TABENTITYNAME);
		bBuilder_Properties_Window = false;
	}

	// always clear undo buffers
	SAFE_DELETE ( g_pUndoBufferPtr );
	g_dwUndoBufferCount = 0;
	SAFE_DELETE ( g_pRedoBufferPtr );
	g_dwRedoBufferCount = 0;
}

void ebe_show ( void )
{
	if ( ebebuild.iTexturePanelSprite[0] > 0 && ebebuild.iToolObj > 0 )
	{
		if (!bDisableAllSprites) {
			for (int n = 0; n < 16; n++) if (SpriteExist(ebebuild.iMatSpr[n]) == 1) ShowSprite(ebebuild.iMatSpr[n]);
			if (SpriteExist(ebebuild.iEBETexHelpSpr) == 1) ShowSprite(ebebuild.iEBETexHelpSpr);
			if (SpriteExist(ebebuild.iEBEHelpSpr) == 1) ShowSprite(ebebuild.iEBEHelpSpr);
			if (SpriteExist(ebebuild.iTexturePanelHighSprite) == 1) ShowSprite(ebebuild.iTexturePanelHighSprite);
			for (int iTex = 0; iTex < EBETEXPANELSPRMAX; iTex++)
			{
				if (SpriteExist(ebebuild.iTexturePanelSprite[iTex]) == 1) ShowSprite(ebebuild.iTexturePanelSprite[iTex]);
			}
		}
		if ( ObjectExist ( ebebuild.iToolObj ) == 1 ) ShowObject ( ebebuild.iToolObj );
	}
}

void ebe_newsite ( int iEntityIndex )
{
	// save any existing site
	if ( t.ebe.entityelementindex > 0 ) ebe_finishsite();

	// and assign newly added entity as site
	t.ebe.entityelementindex = iEntityIndex;

	// create new build object when triggered (t.ebe.entityelementindex)
	int iBuildObj = t.entityelement[t.ebe.entityelementindex].obj;
	int entid = t.entityelement[t.ebe.entityelementindex].bankindex;
	ebe_init_newbuild ( iBuildObj, entid );

	// reset for tool work
	ebe_reset();
}

int ebe_save ( int iEntityIndex )
{
	// local vars
	int iHaveCreatedNewItems = 0;

	// Needed locals
	cstr pOldDir = GetDir();
	HWND hThisWnd = GetForegroundWindow();

	// Check if EBE
	int iEntID = 0;
	bool bIsThisAnEBE = false;
	if ( iEntityIndex > 0 ) 
	{
		iEntID = t.entityelement[iEntityIndex].bankindex;
		if ( iEntID > 0 ) 
			if ( t.entityprofile[iEntID].isebe != 0 )
				bIsThisAnEBE = true;
	}
	if ( bIsThisAnEBE == false )
		return 0;

	// EBE Save Folder
	t.strwork = g.fpscrootdir_s + "\\Files\\entitybank\\user\\ebestructures";
	if ( PathExist( t.strwork.Get() ) == 0 ) 
	{
		MessageBoxA ( hThisWnd, "Cannot find 'entitybank\\user\\ebestructures' folder", "Error", MB_OK | MB_TOPMOST );
		return 0;
	}

	//  Ask for save filename
	cStr tSaveFile = "";
	cStr tSaveMessage = "Save EBE Structure";
	tSaveFile = openFileBox("EBE Structure (.ebe)|*.ebe|All Files|*.*|", t.strwork.Get(), tSaveMessage.Get(), ".ebe", IMPORTERSAVEFILE);
	if ( tSaveFile == "Error" )
	{
		SetDir(pOldDir.Get());
		return 0;
	}

	// Truncate file from absolute save filename
	LPSTR pFileNameOnly = NULL;
	LPSTR pThisSaveFile = tSaveFile.Get();
	for ( int n = strlen(tSaveFile.Get())-1; n > 0; n-- )
	{
		if ( pThisSaveFile[n] == '\\' || pThisSaveFile[n] == '/' )
		{
			pFileNameOnly = pThisSaveFile + n + 1;
			break;
		}
	}

	// Check if already exists, if so, ask if should be overwritten
	if ( FileExist ( tSaveFile.Get() ) == 1 )
	{
		// Already Exists
		char pDisplayErrorMsg[512];
		if ( strlen ( pFileNameOnly ) > 482 ) pFileNameOnly = pThisSaveFile + strlen(pThisSaveFile) - 480;
		strcpy ( pDisplayErrorMsg, pFileNameOnly );
		strcat ( pDisplayErrorMsg, " already exists! Overwrite?" );
		if ( MessageBoxA ( hThisWnd, pDisplayErrorMsg, "File Already Exists", MB_YESNO | MB_TOPMOST ) != IDYES )
		{
			// Abort save here
			SetDir(pOldDir.Get());
			return 0;
		}
	}
	else
	{
		// record this is a new EBE file
		iHaveCreatedNewItems = 1;

		// assign new name to entityID
		t.entityprofileheader[iEntID].desc_s = Left(pFileNameOnly,strlen(pFileNameOnly)-4);
	}

	// restore current folder
	SetDir(pOldDir.Get());

	// Use filename to save EBE Entity to location
	ebe_save_ebefile ( tSaveFile, iEntID );

	// return 1 if have created new EBE file (will trigger icon to be added to IDE)
	return iHaveCreatedNewItems;
}

void ebe_load_ebefile ( cStr pLoadFile, int iEntID )
{
	// clear previous data, and reset for new data
	SAFE_DELETE ( t.entityprofile[iEntID].ebe.pRLEData );
	t.entityprofile[iEntID].ebe.dwRLESize = 0;
	SAFE_DELETE ( t.entityprofile[iEntID].ebe.iMatRef );
	t.entityprofile[iEntID].ebe.dwMatRefCount = 0;
	for ( int n = 0; n < t.entityprofile[iEntID].ebe.dwTexRefCount; n++ )
		SAFE_DELETE ( t.entityprofile[iEntID].ebe.pTexRef[n] );
	SAFE_DELETE ( t.entityprofile[iEntID].ebe.pTexRef );
	t.entityprofile[iEntID].ebe.dwTexRefCount = 0;

	// Use filename to save EBE Entity to location
	if ( FileExist ( pLoadFile.Get() ) == 1 )
	{
		char debug[1024];
		sprintf(debug, "ebe_load_ebefile( %s )", pLoadFile.Get());
		timestampactivity(0, debug);

		OpenToRead ( 1, pLoadFile.Get() );
		int iVersionNumber = ReadLong ( 1 );
		if ( iVersionNumber >= 101 )
		{
			// Store X, Y, Z dimenions of grid volume
			int iCubeAreaSizeX = ReadLong ( 1 );// CUBEAREASIZE )
			int iCubeAreaSizeY = ReadLong ( 1 );// CUBEAREASIZE )
			int iCubeAreaSizeZ = ReadLong ( 1 );// CUBEAREASIZE )

			// Save cube raw data
			t.entityprofile[iEntID].ebe.dwRLESize = ReadLong ( 1 );
			t.entityprofile[iEntID].ebe.pRLEData = new DWORD[t.entityprofile[iEntID].ebe.dwRLESize];
			for ( DWORD dwPos = 0; dwPos < t.entityprofile[iEntID].ebe.dwRLESize; dwPos++ )
			{
				int iDataItem = ReadLong ( 1 );
				t.entityprofile[iEntID].ebe.pRLEData[dwPos] = *(DWORD*)&iDataItem;
			}
		}
		if ( iVersionNumber >= 102 )
		{
			// Load material references
			t.entityprofile[iEntID].ebe.dwMatRefCount = ReadLong ( 1 );
			SAFE_DELETE ( t.entityprofile[iEntID].ebe.iMatRef );
			t.entityprofile[iEntID].ebe.iMatRef = new int[t.entityprofile[iEntID].ebe.dwMatRefCount];
			for ( DWORD dwI = 0; dwI < t.entityprofile[iEntID].ebe.dwMatRefCount; dwI++ )
			{
				t.entityprofile[iEntID].ebe.iMatRef[dwI] = ReadLong ( 1 );
			}
		}
		if ( iVersionNumber >= 103 )
		{
			// Load texture references
			for ( DWORD dwI = 0; dwI < t.entityprofile[iEntID].ebe.dwTexRefCount; dwI++ ) 
				SAFE_DELETE ( t.entityprofile[iEntID].ebe.pTexRef[dwI] );
			SAFE_DELETE ( t.entityprofile[iEntID].ebe.pTexRef );
			t.entityprofile[iEntID].ebe.dwTexRefCount = ReadLong ( 1 );
			t.entityprofile[iEntID].ebe.pTexRef = new LPSTR[t.entityprofile[iEntID].ebe.dwTexRefCount];
			for ( DWORD dwI = 0; dwI < t.entityprofile[iEntID].ebe.dwTexRefCount; dwI++ )
			{
				LPSTR pString = ReadString ( 1 );
				t.entityprofile[iEntID].ebe.pTexRef[dwI] = new char[256];
				strcpy ( t.entityprofile[iEntID].ebe.pTexRef[dwI], pString );
			}
		}
		CloseFile ( 1 );
	}
}

void ebe_save_ebefile ( cStr tSaveFile, int iEntID )
{
	// Chop current directory from save file to shorten filenames (for long EBE texture name)
	cstr pOldDir = GetDir();
	if ( strnicmp ( tSaveFile.Get(), pOldDir.Get(), strlen(pOldDir.Get())) == NULL)
		tSaveFile = Right ( tSaveFile.Get(), strlen(tSaveFile.Get()) - (strlen(pOldDir.Get())+1) );
	
	// Use filename (tSaveFile, i.e. ebe1.ebe) to save EBE Entity to location
	if ( FileExist(tSaveFile.Get()) == 1 ) DeleteAFile ( tSaveFile.Get() );
	OpenToWrite ( 1, tSaveFile.Get() );
	int iVersionNumber = 103; 
	WriteLong ( 1, iVersionNumber );
	if ( iVersionNumber >= 101 )
	{
		// Store X, Y, Z dimenions of grid volume
		WriteLong ( 1, CUBEAREASIZE );
		WriteLong ( 1, CUBEAREASIZE );
		WriteLong ( 1, CUBEAREASIZE );

		// Save cube raw data
		WriteLong ( 1, t.entityprofile[iEntID].ebe.dwRLESize );
		for ( DWORD dwPos = 0; dwPos < t.entityprofile[iEntID].ebe.dwRLESize; dwPos++ )
		{
			DWORD dwDataItem = t.entityprofile[iEntID].ebe.pRLEData[dwPos];
			WriteLong ( 1, *(int*)&dwDataItem );
		}
	}
	if ( iVersionNumber >= 102 )
	{
		// Store material references
		WriteLong ( 1, t.entityprofile[iEntID].ebe.dwMatRefCount );
		for ( DWORD dwI = 0; dwI < t.entityprofile[iEntID].ebe.dwMatRefCount; dwI++ )
		{
			if ( t.entityprofile[iEntID].ebe.iMatRef != NULL )
			{
				WriteLong ( 1, t.entityprofile[iEntID].ebe.iMatRef[dwI] );
			}
			else
			{
				WriteLong ( 1, 0 );
			}
		}
	}
	if ( iVersionNumber >= 103 )
	{
		// Store texture references
		WriteLong ( 1, t.entityprofile[iEntID].ebe.dwTexRefCount );
		for ( DWORD dwI = 0; dwI < t.entityprofile[iEntID].ebe.dwTexRefCount; dwI++ )
		{
			if ( t.entityprofile[iEntID].ebe.pTexRef[dwI] != NULL )
			{
				WriteString ( 1, t.entityprofile[iEntID].ebe.pTexRef[dwI] );
			}
			else
			{
				WriteString ( 1, "" );
			}
		}
	}
	CloseFile ( 1 );

	// Determine name part
	cStr tNameOnly;
	LPSTR pFileNameOnly = NULL;
	LPSTR pThisSaveFile = tSaveFile.Get();
	for ( int n = strlen(pThisSaveFile)-1; n > 0; n-- )
	{
		if ( pThisSaveFile[n] == '\\' || pThisSaveFile[n] == '/' )
		{
			pFileNameOnly = pThisSaveFile + n + 1;
			break;
		}
	}
	tNameOnly = Left ( pFileNameOnly, strlen(pFileNameOnly) - 4 );
	cstr tRawPathAndFile = cstr ( Left ( tSaveFile.Get(), strlen(tSaveFile.Get()) - 4 ) );
	cstr tRawPath = cstr ( Left ( tSaveFile.Get(), strlen(tSaveFile.Get()) - 4 - strlen(tNameOnly.Get()) ) );

	// Now create a real entity from it (for quickest library and level loading)
	// FPE
	Dim(t.setuparr_s, 50);
	int iA = 0;
	t.setuparr_s[iA++] = ";EBE Entity";
	t.setuparr_s[iA++] = "";
	t.setuparr_s[iA++] = ";Header";
	t.setuparr_s[iA++] = cstr("desc          = ") + tNameOnly;
	t.setuparr_s[iA++] = "";
	t.setuparr_s[iA++] = ";AI";
	t.setuparr_s[iA++] = "aimain	      = default.lua";
	t.setuparr_s[iA++] = "";
	t.setuparr_s[iA++] = ";Orientation";

	t.setuparr_s[iA++] = cstr("model         = ") + tNameOnly + ".x";
	//If single mesh just store one entry. else make a entry per mesh.
	sObject* pObject = g_ObjectList[ebebuild.iBuildObj];
	sMesh * pMesh = NULL;
	bool bTextured = false;
	if (pObject && t.grideleprof.bCustomWickedMaterialActive) 
	{
		cstr importer_getfilenameonly(LPSTR pFileAndPossiblePath);

		//PE: Only support singe mesh wicked textured.
		int iFoundTextured = 0;
		for (int i = 0; i < pObject->iFrameCount; i++)
		{
			if (pObject->ppFrameList[i]->pMesh && pObject->ppFrameList[i]->pMesh->wickedmeshindex > 0)
			{
				pMesh = pObject->ppFrameList[i]->pMesh;
				iFoundTextured++;
				break; //PE: Only first.
			}
		}

		if (iFoundTextured == 1 && pMesh) 
		{
			wiScene::MeshComponent* mesh = wiScene::GetScene().meshes.GetComponent(pMesh->wickedmeshindex);
			if (mesh)
			{
				uint64_t materialEntity = mesh->subsets[0].materialID;
				wiScene::MaterialComponent* pObjectMaterial = wiScene::GetScene().materials.GetComponent(materialEntity);
				if (pObjectMaterial)
				{
					if (pObjectMaterial->textures[MaterialComponentTEXTURESLOT::BASECOLORMAP].resource)
					{
						cStr TextureFilename = importer_getfilenameonly((LPSTR)pObjectMaterial->textures[MaterialComponentTEXTURESLOT::BASECOLORMAP].name.c_str());
						t.tString = ""; t.tString = t.tString + importerPadString("textured") + "= " + TextureFilename;
						t.setuparr_s[iA++] = t.tString;
						t.tString = ""; t.tString = t.tString + importerPadString("baseColorMap") + "= " + TextureFilename;
						t.setuparr_s[iA++] = t.tString;
						t.tString = ""; t.tString = t.tString + importerPadString("alphaRef") + "= " + cStr(t.grideleprof.WEMaterial.fAlphaRef[0]);
						t.setuparr_s[iA++] = t.tString;
						bTextured = true;
					}
					if (pObjectMaterial->textures[MaterialComponentTEXTURESLOT::NORMALMAP].resource)
					{
						cStr TextureFilename = importer_getfilenameonly((LPSTR)pObjectMaterial->textures[MaterialComponentTEXTURESLOT::NORMALMAP].name.c_str());
						t.tString = ""; t.tString = t.tString + importerPadString("normalMap") + "= " + TextureFilename;
						t.setuparr_s[iA++] = t.tString;
						t.tString = ""; t.tString = t.tString + importerPadString("normalStrength") + "= " + cStr(t.grideleprof.WEMaterial.fNormal[0]);
						t.setuparr_s[iA++] = t.tString;
					}
					if (pObjectMaterial->textures[MaterialComponentTEXTURESLOT::SURFACEMAP].resource)
					{
						cStr TextureFilename = importer_getfilenameonly((LPSTR)pObjectMaterial->textures[MaterialComponentTEXTURESLOT::SURFACEMAP].name.c_str());
						t.tString = ""; t.tString = t.tString + importerPadString("surfaceMap") + "= " + TextureFilename;
						t.setuparr_s[iA++] = t.tString;
						t.tString = ""; t.tString = t.tString + importerPadString("roughnessStrength") + "= " + cStr(t.grideleprof.WEMaterial.fRoughness[0]);
						t.setuparr_s[iA++] = t.tString;
						t.tString = ""; t.tString = t.tString + importerPadString("metalnessStrength") + "= " + cStr(t.grideleprof.WEMaterial.fMetallness[0]);
						t.setuparr_s[iA++] = t.tString;
					}
					if (pObjectMaterial->textures[MaterialComponentTEXTURESLOT::DISPLACEMENTMAP].resource)
					{
						cStr TextureFilename = importer_getfilenameonly((LPSTR)pObjectMaterial->textures[MaterialComponentTEXTURESLOT::DISPLACEMENTMAP].name.c_str());
						t.tString = ""; t.tString = t.tString + importerPadString("displacementMap") + "= " + TextureFilename;
						t.setuparr_s[iA++] = t.tString;
					}

					if (pObjectMaterial->textures[MaterialComponentTEXTURESLOT::EMISSIVEMAP].resource)
					{
						cStr TextureFilename = importer_getfilenameonly((LPSTR)pObjectMaterial->textures[MaterialComponentTEXTURESLOT::EMISSIVEMAP].name.c_str());
						t.tString = ""; t.tString = t.tString + importerPadString("emissiveMap") + "= " + TextureFilename;
						t.setuparr_s[iA++] = t.tString;
						t.tString = ""; t.tString = t.tString + importerPadString("emissiveStrength") + "= " + cStr(t.grideleprof.WEMaterial.fEmissive[0]);
						t.setuparr_s[iA++] = t.tString;
					}
					if (pObjectMaterial->textures[MaterialComponentTEXTURESLOT::OCCLUSIONMAP].resource)
					{
						cStr TextureFilename = importer_getfilenameonly((LPSTR)pObjectMaterial->textures[MaterialComponentTEXTURESLOT::OCCLUSIONMAP].name.c_str());
						t.tString = ""; t.tString = t.tString + importerPadString("occlusionMap") + "= " + TextureFilename;
						t.setuparr_s[iA++] = t.tString;
					}
				}
			}
		}
	}

	if (!bTextured)
	{
		t.setuparr_s[iA++] = cstr("textured      = ") + t.entityprofile[iEntID].texd_s.Get();
	}

	t.setuparr_s[iA++] = "effect        = effectbank\\reloaded\\apbr_basic.fx";

	t.setuparr_s[iA++] = "scale         = 100";
	t.setuparr_s[iA++] = "defaultstatic = 1";
	t.setuparr_s[iA++] = "collisionmode = 1";
	t.setuparr_s[iA++] = "forcesimpleobstacle = 3";
	t.setuparr_s[iA++] = cstr("forceobstaclepolysize = ") + cstr(t.entityprofile[iEntID].forceobstaclepolysize);
	t.setuparr_s[iA++] = "";
	t.setuparr_s[iA++] = ";EBE Builder Extras";
	t.setuparr_s[iA++] = "isebe         = 1";

	extern float fReflectance;
	extern bool bTransparent;
	extern bool bDoubleSided;
	extern float fRenderOrderBias;
	extern bool bPlanerReflection;
	extern bool bCastShadows;
	extern DWORD dwBaseColor;
	extern DWORD dwEmmisiveColor;

	if (bTransparent) 
	{
		t.tString = ""; t.tString = t.tString + importerPadString("transparency") + "= 1";
		t.setuparr_s[iA++] = t.tString;
	}
	else 
	{
		t.tString = ""; t.tString = t.tString + importerPadString("transparency") + "= 0";
		t.setuparr_s[iA++] = t.tString;
	}
	if (!bCastShadows) 
	{
		t.tString = ""; t.tString = t.tString + importerPadString("castshadow") + "= -1";
		t.setuparr_s[iA++] = t.tString;
	}
	if (bDoubleSided) 
	{
		t.tString = ""; t.tString = t.tString + importerPadString("doublesided") + "= 1";
		t.setuparr_s[iA++] = t.tString;
	}
	t.tString = ""; t.tString = t.tString + importerPadString("renderorderbias") + "= " + cStr(fRenderOrderBias);
	t.setuparr_s[iA++] = t.tString;
	if (bPlanerReflection) 
	{
		t.tString = ""; t.tString = t.tString + importerPadString("planerreflection") + "= 1";
		t.setuparr_s[iA++] = t.tString;
	}
	if (fReflectance != 0.002f) 
	{
		t.tString = ""; t.tString = t.tString + importerPadString("reflectance") + "= " + cStr(fReflectance);
		t.setuparr_s[iA++] = t.tString;
	}

	//PE: Test t.grideleprof.WEMaterial.dwBaseColor
	if (t.grideleprof.WEMaterial.dwBaseColor[0] != -1)
	{
		char tmp[256];
		sprintf(tmp, "%lu", (unsigned long)t.grideleprof.WEMaterial.dwBaseColor[0]);
		t.tString = ""; t.tString = t.tString + importerPadString("basecolor") + "= " + cStr(tmp);
		t.setuparr_s[iA++] = t.tString;

	}
	else if (dwBaseColor != -1) 
	{
		char tmp[256];
		sprintf(tmp, "%lu", (unsigned long)dwBaseColor);
		t.tString = ""; t.tString = t.tString + importerPadString("basecolor") + "= " + cStr(tmp);
		t.setuparr_s[iA++] = t.tString;
	}
	if (t.grideleprof.WEMaterial.dwEmmisiveColor[0] != -1)
	{
		char tmp[256];
		sprintf(tmp, "%lu", (unsigned long)t.grideleprof.WEMaterial.dwEmmisiveColor[0]);
		t.tString = ""; t.tString = t.tString + importerPadString("emissivecolor") + "= " + cStr(tmp);
		t.setuparr_s[iA++] = t.tString;
	}
	else if (dwEmmisiveColor != -1) 
	{
		char tmp[256];
		sprintf(tmp, "%lu", (unsigned long)dwEmmisiveColor);
		t.tString = ""; t.tString = t.tString + importerPadString("emissivecolor") + "= " + cStr(tmp);
		t.setuparr_s[iA++] = t.tString;
	}

	cstr pFPEFile = tRawPathAndFile + cstr(".fpe");
	if (FileExist(pFPEFile.Get()) == 1) DeleteAFile(pFPEFile.Get());
	SaveArray(pFPEFile.Get(), t.setuparr_s);
	UnDim(t.setuparr_s);


	// BMP
	cstr tBMPFilename = "ebebank\\_builder\\EBE.bmp";
	if ( FileExist(tBMPFilename.Get()) == 1 ) 
	{
		cstr sBMPFile = tRawPathAndFile + cstr(".bmp");
		char pRealBMPFile[MAX_PATH];
		strcpy(pRealBMPFile, sBMPFile.Get());
		GG_GetRealPath(pRealBMPFile, 1);
		sBMPFile = pRealBMPFile;
		if ( FileExist(sBMPFile.Get()) == 1 ) DeleteAFile ( sBMPFile.Get() );
		CopyFileA ( tBMPFilename.Get(), sBMPFile.Get(), FALSE );
	}

	// DBO
	int iSourceObj = g.entitybankoffset + iEntID;
	cstr tDBOFile = tRawPathAndFile + cstr(".dbo");
	char pRealDBOFile[MAX_PATH];
	strcpy(pRealDBOFile, tDBOFile.Get());
	GG_GetRealPath(pRealDBOFile, 1);
	tDBOFile = pRealDBOFile;
	if ( FileExist(tDBOFile.Get()) == 1 ) DeleteAFile ( tDBOFile.Get() );
	SaveObject ( tDBOFile.Get(), iSourceObj );

	// and X
	cstr tXFile = tRawPathAndFile + cstr(".x");
	if ( FileExist(tXFile.Get()) == 1 ) DeleteAFile ( tXFile.Get() );
	dbo2xConvert(tDBOFile.Get(), tXFile.Get());
	// keep DBO for faster loading

	// DDS/JPG
	cstr tDDSSourceRaw = cstr("ebebank\\default\\") + Left(t.entityprofile[iEntID].texd_s.Get(),strlen(t.entityprofile[iEntID].texd_s.Get())-10);
	cstr tDDSSourceFilename = tDDSSourceRaw + "_color.dds";
	tRawPathAndFile = tRawPath + Left(t.entityprofile[iEntID].texd_s.Get(),strlen(t.entityprofile[iEntID].texd_s.Get())-10);
	if ( FileExist(tDDSSourceFilename.Get()) == 0 ) 
	{
		if ( strnicmp ( tRawPath.Get(), "levelbank", 9 ) != NULL )
		{
			tDDSSourceRaw = g.mysystem.levelBankTestMap_s + Left(t.entityprofile[iEntID].texd_s.Get(),strlen(t.entityprofile[iEntID].texd_s.Get())-10);
			tDDSSourceFilename = tDDSSourceRaw + "_color.dds";
		}
	}
	if ( FileExist(tDDSSourceFilename.Get()) == 1 ) 
	{
		cstr sDDSFile = tRawPathAndFile + cstr("_color.dds");
		char pRealDDSFile[MAX_PATH];
		strcpy(pRealDDSFile, sDDSFile.Get());
		GG_GetRealPath(pRealDDSFile, 1);
		sDDSFile = pRealDDSFile;
		// if source and dest are same, should not delete or copy, just leave EBE texture in place
		bool bProceedWithCopy = true;
		if (stricmp (tDDSSourceFilename.Get(), sDDSFile.Get()) == NULL)
		{
			// same texture, leave alone so level can contain original EBE texture
			bProceedWithCopy = false;
		}
		else
		{
			if (FileExist(sDDSFile.Get()) == 1) DeleteAFile (sDDSFile.Get());
		}
		char pRealDDSSourceFilename[MAX_PATH];
		if (bProceedWithCopy == true)
		{
			strcpy(pRealDDSSourceFilename, tDDSSourceFilename.Get());
			GG_GetRealPath(pRealDDSSourceFilename, 1);
			tDDSSourceFilename = pRealDDSSourceFilename;
			CopyFileA (tDDSSourceFilename.Get(), sDDSFile.Get(), FALSE);
		}
		 tDDSSourceFilename = tDDSSourceRaw + "_normal.dds";
		 sDDSFile = tRawPathAndFile + cstr("_normal.dds");
		 strcpy(pRealDDSFile, sDDSFile.Get());
		 GG_GetRealPath(pRealDDSFile, 1);
		 sDDSFile = pRealDDSFile;
		 if (bProceedWithCopy == true && FileExist(sDDSFile.Get()) == 1 ) DeleteAFile ( sDDSFile.Get() );
		 if (bProceedWithCopy == true)
		 {
			 strcpy(pRealDDSSourceFilename, tDDSSourceFilename.Get());
			 GG_GetRealPath(pRealDDSSourceFilename, 1);
			 tDDSSourceFilename = pRealDDSSourceFilename;
			 CopyFileA (tDDSSourceFilename.Get(), sDDSFile.Get(), FALSE);
		 }
		 tDDSSourceFilename = tDDSSourceRaw + "_surface.dds";
		 sDDSFile = tRawPathAndFile + cstr("_surface.dds");
		 strcpy(pRealDDSFile, sDDSFile.Get());
		 GG_GetRealPath(pRealDDSFile, 1);
		 sDDSFile = pRealDDSFile;
		 if (bProceedWithCopy == true && FileExist(sDDSFile.Get()) == 1 ) DeleteAFile ( sDDSFile.Get() );
		 if (bProceedWithCopy == true)
		 {
			 strcpy(pRealDDSSourceFilename, tDDSSourceFilename.Get());
			 GG_GetRealPath(pRealDDSSourceFilename, 1);
			 tDDSSourceFilename = pRealDDSSourceFilename;
			 CopyFileA (tDDSSourceFilename.Get(), sDDSFile.Get(), FALSE);
		 }
	}
}

