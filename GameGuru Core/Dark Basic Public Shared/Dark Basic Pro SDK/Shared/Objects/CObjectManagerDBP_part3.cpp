bool CObjectManager::UpdateLayerInner ( int iLayer )
{
	// work vars
	int iObject = 0;
	static int iOnlyOneSortPerSync = 0;
	bool bUseStencilWrite=false;
	GGVECTOR3 vecShadowPos;

	// if sync mask override active, reject any drawing activity
	if ( g_dwSyncMaskOverride == 0 ) return true;

    // Get camera information for LOD and distance calculation
	// ensure rendercamera of 31-34 selects mask for camera 31 (shadow camera)
    DWORD dwCurrentCameraBit;
	if ( g_pGlob->dwRenderCameraID<31 )
		dwCurrentCameraBit = 1 << g_pGlob->dwRenderCameraID;
	else
		dwCurrentCameraBit = 1 << 31;

	// run through all visible objects and draw them (unrolled for performance)
	switch ( iLayer )
	{
	case -1 : // Very Early Objects (rendered even before StencilStart)
		{
			iOnlyOneSortPerSync = 0;
			// choose camera to render sky (and other early objects) to (used by cube map generator)
			int iPreferredCamera = 0;
			if ( g_pGlob->dwRenderCameraID == 30 ) iPreferredCamera = 30;

			// reset to default camera range for noz and locked objects
			float fCurrentNearRange = 0.0f;
			float fCurrentFarRange = 0.0f;
			bool bCameraRangeAndProjectionChanged = false;
			if ( g_pGlob->dwRenderCameraID != 6 && g_pGlob->dwRenderCameraID != 7 )
			{
				// except for cameras 6 and 7 which are VR eye cameras and have their own projection matrix (which should not be overwritten by SetCameraRange)
				tagCameraData* m_Camera_Ptr = (tagCameraData*)GetCameraInternalData( iPreferredCamera );
				fCurrentNearRange = m_Camera_Ptr->fZNear;
				fCurrentFarRange = m_Camera_Ptr->fZFar;
				SetCameraRange ( iPreferredCamera, 1, 70000 );
				bCameraRangeAndProjectionChanged = true;
			}
			if ( ! m_vVisibleObjectEarly.empty() )
			{
				for ( DWORD iIndex = 0; iIndex < m_vVisibleObjectEarly.size(); ++iIndex )
				{
					sObject* pObject = m_vVisibleObjectEarly [ iIndex ];

					// leeadd - 211006 - u63 - ignore objects whose masks reject the current camera
					if ( (pObject->dwCameraMaskBits & dwCurrentCameraBit)==0 )
						continue;

					// leeadd - 240106 - if any LOD activity
					// u74b8 - avoid recalculation of distance if already sorted by distance
					if ( pObject->bHadLODNeedCamDistance && g_eGlobalSortOrder != E_SORT_BY_DEPTH)
						pObject->position.fCamDistance = CalculateObjectDistanceFromCamera ( pObject );

					// call the draw function
					DrawObject ( pObject, true );
				}
			}
			// restore camera range
			if ( bCameraRangeAndProjectionChanged == true )
			{
				// except for cameras 6 and 7 which are VR eye cameras and have their own projection matrix (which should not be overwritten by SetCameraRange)
				SetCameraRange ( iPreferredCamera, fCurrentNearRange, fCurrentFarRange );
			}
		}
		break;

	case 0 : // Main Layer
		iOnlyOneSortPerSync = 0; //-1 not always called.
        if ( ! m_vVisibleObjectStandard.empty() )
        {
            for ( DWORD iIndex = 0; iIndex < m_vVisibleObjectStandard.size(); ++iIndex )
            {
                sObject* pObject = m_vVisibleObjectStandard [ iIndex ];

				// ignore objects whose masks reject the current camera
				if ( (pObject->dwCameraMaskBits & dwCurrentCameraBit)==0 )
					continue;

				// do not render static objects
				if ( pObject->bStatic )
					continue;

				// or stencil objects
				if ( pObject->bReflectiveObject )
					continue;

				// leeadd - 240106 - if any LOD activity
                // u74b8 - avoid recalculation of distance if already sorted by distance
                if ( pObject->bHadLODNeedCamDistance && g_eGlobalSortOrder != E_SORT_BY_DEPTH)
					pObject->position.fCamDistance = CalculateObjectDistanceFromCamera ( pObject );

				// call the draw function
				//if ( !DrawObject ( pObject, false ) )
				//	return false;
				DrawObject ( pObject, false );
			}
        }
        break;

	case 3 : // Overlay Ghost Layer (in stages)

        if ( ! m_vVisibleObjectTransparent.empty() )
        {
			if (iOnlyOneSortPerSync++ == 0) {

				// leeadd - 021205 - new feature which can divide transparent depth-sorted objects by a water
				// line so everything below is rendered, then the water, then everything at normal surface order
				bool bWaterPlaneDivision = false;
				float fWaterPlaneDivisionY = 99999.99f;

				// get list of ghosted objects for depth sort
				for (DWORD iIndex = 0; iIndex < m_vVisibleObjectTransparent.size(); ++iIndex)
				{
					sObject* pObject = m_vVisibleObjectTransparent[iIndex];
					if (!pObject) continue;

					// leeadd - 211006 - u63 - ignore objects whose masks reject the current camera
					if ((pObject->dwCameraMaskBits & dwCurrentCameraBit) == 0)
						continue;

					// calculate distance from object to camera (fills fCamDistance)
					if (pObject->bTransparencyWaterLine == true)
					{
						/*
													// leeadd - 021205 - transparent object water line, using HEIGHY (Y) as ordering (great for water planes)
													if ( pObject->position.vecPosition.y < fWaterPlaneDivisionY )
														fWaterPlaneDivisionY = pObject->position.vecPosition.y;

													// set as furthest surface distance object (and first to be drawn after underwater objs)
													// u74b8 - use the current camera
													if (g_eGlobalSortOrder != E_SORT_BY_DEPTH)
														pObject->position.fCamDistance = CalculateObjectDistanceFromCamera ( pObject );
													else
														pObject->position.fCamDistance = 0.0f;

													pObject->position.fCamDistance += m_pCamera->fZFar;
						*/

						//PE: Another try :)
						//PE: Distance to water object (0,600,0) can be huge (we have default camera in center), so below waterline objects dont trigger.
						//PE: If we just set it to m_pCamera->fZFar , they will trigger as they use (+= m_pCamera->fZFar)
						//PE: This fix some of the problems and allow pObject->bRenderBeforeWater "SetObjectTransparency(Obj,8)".

						if (pObject->position.vecPosition.y < fWaterPlaneDivisionY)
							fWaterPlaneDivisionY = pObject->position.vecPosition.y;

						pObject->position.fCamDistance = m_pCamera->fZFar;

						bWaterPlaneDivision = true;
					}
					else
					{
						// regular object vs camera distance
						// u74b8 - If already sorted by distance, then we've also already
						//         calculated the camera distance and there's no need to do it again.
						if (g_eGlobalSortOrder != E_SORT_BY_DEPTH)
						{
							//Perhaps drop particles. pObject->dwObjectNumber < 180000
							pObject->position.fCamDistance = CalculateObjectCenterDistanceFromCamera(pObject);
							//pObject->position.fCamDistance = CalculateObjectDistanceFromCamera(pObject);
						}
					}
				}

				// if some objs underwater division, increase their cam distances so they ALL are drawn first (in same order)
				// OR some objects have a distance offset to affect draw order
				for (DWORD iIndex = 0; iIndex < m_vVisibleObjectTransparent.size(); ++iIndex)
				{
					// get obj ptr
					sObject* pObject = m_vVisibleObjectTransparent[iIndex];

					// record original cam distance value
					pObject->position.fStoreLastCamDistance = pObject->position.fCamDistance;
					if (pObject->dwReservedR4 == 15)
					{
						//Smoke decal need this or it render before water for some reason ?
						pObject->position.fCamDistance -= m_pCamera->fZFar; //force after water
					}

					// for waterline object itself
					if (bWaterPlaneDivision == true)
					{
						//if(  t.terrain.vegetationshaderindex)
						if (pObject->bTransparencyWaterLine == false)
						{
							// for LARGE explosion decals, above water bangs are forced to render FIRST
							float fBaseOfObj = pObject->position.vecPosition.y;
							if (fBaseOfObj < fWaterPlaneDivisionY)
							{
								// u74b8 - use the current camera
								pObject->position.fCamDistance += m_pCamera->fZFar;
							}
							else if (pObject->bRenderBeforeWater) {
								pObject->position.fCamDistance += m_pCamera->fZFar;
							}
						}
					}

					// also apply any artificial distance to object to affect draw order
					pObject->position.fCamDistance += pObject->fArtificialDistanceOffset;
				}

				// u74b7 - sort objects by distance, replaced bubblesort with STL sort
				std::sort(m_vVisibleObjectTransparent.begin(), m_vVisibleObjectTransparent.end(), OrderByReverseCameraDistance());
			}

            // draw in correct back to front order
            for ( DWORD iIndex = 0; iIndex < m_vVisibleObjectTransparent.size(); ++iIndex )
            {
                sObject* pObject = m_vVisibleObjectTransparent [ iIndex ];
				if ( !pObject ) 
					continue;

				// restore original cam distance value (changed for depth reordering)
				pObject->position.fCamDistance = pObject->position.fStoreLastCamDistance;

				// u75b9 - fixes Transparency and Camera Mask problem
                if (( pObject->dwCameraMaskBits & dwCurrentCameraBit ) == 0)
                    continue;

                //if ( !DrawObject ( pObject, false ) )
                //    return false;
                DrawObject ( pObject, false );
            }

	    }
		// end ghost layer
		break;

	case 4 : // Overlay Locked/NoZ Layer
	    
        if ( ! m_vVisibleObjectNoZDepth.empty() )
        {
			// reset to default camera range for noz and locked objects
			tagCameraData* m_Camera_Ptr = (tagCameraData*)GetCameraInternalData( 0 );
			float fCurrentNearRange = m_Camera_Ptr->fZNear;
			float fCurrentFarRange = m_Camera_Ptr->fZFar;
			bool bCameraRangeAndProjectionChanged = false;
			if ( g_pGlob->dwRenderCameraID != 6 && g_pGlob->dwRenderCameraID != 7 )
			{
				// except for cameras 6 and 7 which are VR eye cameras and have their own projection matrix (which should not be overwritten by SetCameraRange)
				SetCameraRange ( 0, 2.5f, 70000.0f ); // forces HUD weapons not to blur/DOF/MOTION/etc
				bCameraRangeAndProjectionChanged = true;
			}

			// record weapon/jetpack techniques (so can restore after cutout technique)
			DWORD dwOldWeaponBasicShaderPtr, dwOldWeaponBoneShaderPtr, dwOldJetpackBoneShaderPtr;
			sEffectItem* pWeaponBasic = NULL;
			sEffectItem* pWeaponBone = NULL;
			sEffectItem* pJetpackBone = NULL;
			#ifndef NOSTEAMORVIDEO
			if ( g_weaponbasicshadereffectindex > 0 ) 
			{
				if ( GetEffectExist(g_weaponbasicshadereffectindex) ) 
				{
					dwOldWeaponBasicShaderPtr = GetEffectTechniqueEx ( g_weaponbasicshadereffectindex );
					pWeaponBasic = m_EffectList [ g_weaponbasicshadereffectindex ];
				}
			}
			if ( g_weaponboneshadereffectindex > 0 ) 
			{
				if ( GetEffectExist(g_weaponboneshadereffectindex) ) 
				{
					dwOldWeaponBoneShaderPtr = GetEffectTechniqueEx ( g_weaponboneshadereffectindex );
					pWeaponBone = m_EffectList [ g_weaponboneshadereffectindex ];
				}
			}
			if ( g_jetpackboneshadereffectindex > 0 )
			{
				if ( GetEffectExist(g_jetpackboneshadereffectindex) ) 
				{
					dwOldJetpackBoneShaderPtr = GetEffectTechniqueEx ( g_jetpackboneshadereffectindex );
					pJetpackBone = m_EffectList [ g_jetpackboneshadereffectindex ];
				}
			}
			#endif

			// prefer to render objects that are marked as 'not' transparent, not locked and bNewZLayerObject as true
			// this will allow muzzle flashes to render 'before' the weapon (and smoke to render AFTER as smoke transparency set to 6)
			for ( DWORD iIndex = 0; iIndex < m_vVisibleObjectNoZDepth.size(); ++iIndex )
			{
				sObject* pObject = m_vVisibleObjectNoZDepth [ iIndex ];
				if ( !pObject ) continue;

				// ignore objects whose masks reject the current camera
				if ( (pObject->dwCameraMaskBits & dwCurrentCameraBit)==0 )
					continue;

				// only render not-transparent, not locked and bNewZLayerObject true objects
				bool bRenderObject = false;
				if ( pObject->bTransparentObject==false && pObject->bLockedObject==false && pObject->bNewZLayerObject==true )
					bRenderObject=true;

				// only if object should be rendered
				if ( !bRenderObject )
					continue;

				// skip if IS weapon/jetpack
				bool bIsWeaponOrJetPack = false;
				sObject* pActualObject = pObject;
				if ( pObject->pInstanceOfObject ) pActualObject = pObject->pInstanceOfObject;
				if ( pActualObject->ppMeshList )
				{
					if ( pWeaponBasic && pWeaponBasic->pEffectObj > 0 && pActualObject->ppMeshList[0]->pVertexShaderEffect == pWeaponBasic->pEffectObj ) bIsWeaponOrJetPack = true;
					if ( pWeaponBone && pWeaponBone->pEffectObj > 0 && pActualObject->ppMeshList[0]->pVertexShaderEffect == pWeaponBone->pEffectObj ) bIsWeaponOrJetPack = true;
					if ( pJetpackBone && pJetpackBone->pEffectObj > 0 && pActualObject->ppMeshList[0]->pVertexShaderEffect == pJetpackBone->pEffectObj ) bIsWeaponOrJetPack = true;
				}
				if ( bIsWeaponOrJetPack == true )
					continue;

				// draw
				DrawObject ( pObject, false );
			}

			// WEAPON RENDERING
			// for NoZDepth pass, two cycles one for depthcutout and regular
			// and hard find weapon shaders that have cutoutdepth techniques
			#ifndef NOSTEAMORVIDEO
			for ( int iCutOutPassIndex = 0; iCutOutPassIndex < 2; iCutOutPassIndex++ )
			{
				if ( g_weaponbasicshadereffectindex+g_weaponboneshadereffectindex+g_jetpackboneshadereffectindex > 0 )
				{
					if ( iCutOutPassIndex == 0 )
					{
						if ( g_weaponbasicshadereffectindex > 0 ) if ( GetEffectExist(g_weaponbasicshadereffectindex) ) SetEffectTechnique ( g_weaponbasicshadereffectindex, "CutOutDepth" );
						if ( g_weaponboneshadereffectindex > 0 ) if ( GetEffectExist(g_weaponboneshadereffectindex) ) SetEffectTechnique ( g_weaponboneshadereffectindex, "CutOutDepth" );
						if ( g_jetpackboneshadereffectindex > 0 ) if ( GetEffectExist(g_jetpackboneshadereffectindex) ) SetEffectTechnique ( g_jetpackboneshadereffectindex, "CutOutDepth" );
					}
					if ( iCutOutPassIndex == 1 )
					{
						if ( g_weaponbasicshadereffectindex > 0 ) if ( GetEffectExist(g_weaponbasicshadereffectindex) ) SetEffectTechniqueEx ( g_weaponbasicshadereffectindex, dwOldWeaponBasicShaderPtr );
						if ( g_weaponboneshadereffectindex > 0 ) if ( GetEffectExist(g_weaponboneshadereffectindex) ) SetEffectTechniqueEx ( g_weaponboneshadereffectindex, dwOldWeaponBoneShaderPtr );
						if ( g_jetpackboneshadereffectindex > 0 ) if ( GetEffectExist(g_jetpackboneshadereffectindex) ) SetEffectTechniqueEx ( g_jetpackboneshadereffectindex, dwOldJetpackBoneShaderPtr );
					}
				}
				for ( DWORD iIndex = 0; iIndex < m_vVisibleObjectNoZDepth.size(); ++iIndex )
				{
					sObject* pObject = m_vVisibleObjectNoZDepth [ iIndex ];
					if ( !pObject ) continue;

					// ignore objects whose masks reject the current camera
					if ( (pObject->dwCameraMaskBits & dwCurrentCameraBit)==0 )
						continue;

					// skip if not weapon/jetpack
					bool bIsWeaponOrJetPack = false;
					if ( pObject->ppMeshList )
					{
						if ( pWeaponBasic && pWeaponBasic->pEffectObj > 0 && pObject->ppMeshList[0]->pVertexShaderEffect == pWeaponBasic->pEffectObj ) bIsWeaponOrJetPack = true;
						if ( pWeaponBone && pWeaponBone->pEffectObj > 0 && pObject->ppMeshList[0]->pVertexShaderEffect == pWeaponBone->pEffectObj ) bIsWeaponOrJetPack = true;
						if ( pJetpackBone && pJetpackBone->pEffectObj > 0 && pObject->ppMeshList[0]->pVertexShaderEffect == pJetpackBone->pEffectObj ) bIsWeaponOrJetPack = true;
					}
					if ( bIsWeaponOrJetPack == false )
						continue;

					// draw weapon/jetpack
					DrawObject ( pObject, false );
				}
			}
			#endif

			// NOZDEPTH LOOP (locked and nozdepth)
			// ( Pass A-ZDepth : Pass B-NoZDepth )
			bool bClearZBuffer = false;
			for ( int iPass = 0; iPass < 2; iPass++ )
			{
				// LOCKED STAGE
				float fCurrentFOV = 0.0f;
				bool bResetCamera = false;
				GGMATRIX matCurrentCameraView;
				for ( DWORD iIndex = 0; iIndex < m_vVisibleObjectNoZDepth.size(); ++iIndex )
				{
					sObject* pObject = m_vVisibleObjectNoZDepth [ iIndex ];
					if ( !pObject ) continue;

					// ignore objects whose masks reject the current camera
					if ( (pObject->dwCameraMaskBits & dwCurrentCameraBit)==0 )
						continue;

					// only render nozdepth objects on second pass
					bool bRenderObject = false;
					if ( iPass==0 && pObject->bNewZLayerObject==false )
					{
						// object has zdepth pass 1
						bRenderObject=true;
					}
					if ( iPass==1 && pObject->bNewZLayerObject==true )
					{
						// object has no zdepth pass 2
						bRenderObject=true;
						if ( bClearZBuffer==false )
						{
							// clear zbuffer
							#ifdef DX11
							//interferes with SAO m_pImmediateContext->ClearpthStencilView(g_pGlob->pCurrentDepthView, D3D11_CLEAR_DEPTH, 1.0f, 0);
							#else
							m_pD3D->Clear ( 0, NULL, D3DCLEAR_ZBUFFER, 0, 1.0f, 0 );
							#endif
							bClearZBuffer=true;
						}
					}

					// only if object should be rendered
					if ( !bRenderObject )
						continue;

					// skip if IS weapon/jetpack
					bool bIsWeaponOrJetPack = false;
					sObject* pActualObject = pObject;
					if ( pObject->pInstanceOfObject ) pActualObject = pObject->pInstanceOfObject;
					if ( pActualObject->ppMeshList )
					{
						if ( pWeaponBasic && pWeaponBasic->pEffectObj > 0 && pActualObject->ppMeshList[0]->pVertexShaderEffect == pWeaponBasic->pEffectObj ) bIsWeaponOrJetPack = true;
						if ( pWeaponBone && pWeaponBone->pEffectObj > 0 && pActualObject->ppMeshList[0]->pVertexShaderEffect == pWeaponBone->pEffectObj ) bIsWeaponOrJetPack = true;
						if ( pJetpackBone && pJetpackBone->pEffectObj > 0 && pActualObject->ppMeshList[0]->pVertexShaderEffect == pJetpackBone->pEffectObj ) bIsWeaponOrJetPack = true;
					}
					if ( bIsWeaponOrJetPack == true )
						continue;

					// do not render not-transparent, not locked and bNewZLayerObject true objects (did this earlier before weapon renders)
					if ( pObject->bTransparentObject==false && pObject->bLockedObject==false && pObject->bNewZLayerObject==true )
						continue;

					// locked objects
					if ( pObject->bLockedObject )
					{
						// reset camera
						if ( bResetCamera==false )
						{
							// Store current camera
							GGGetTransform ( GGTS_VIEW, &matCurrentCameraView );

							// record current FOV, and set default FOV
							tagCameraData* m_Camera_Ptr = (tagCameraData*)GetCameraInternalData( 0 );
							fCurrentFOV = m_Camera_Ptr->fFOV;
							if ( (m_Camera_Ptr->dwCameraSwitchBank & 1) == 0 )
								SetCameraFOV ( GGToDegree(3.14159265358979323846f/2.905f) );

							// Use Default unmodified camera
							GGMATRIX matDefaultCameraView;
							GGMatrixIdentity ( &matDefaultCameraView );
							matDefaultCameraView._11 = 1; 
							matDefaultCameraView._12 = 0; 
							matDefaultCameraView._13 = 0;
							matDefaultCameraView._21 = 0; 
							matDefaultCameraView._22 = 1; 
							matDefaultCameraView._23 = 0;
							matDefaultCameraView._31 = 0;
							matDefaultCameraView._32 = 0; 
							matDefaultCameraView._33 = 1;

							// Assign new default camera
							GGSetTransform ( GGTS_VIEW, &matDefaultCameraView );

							// clear zbuffer
							#ifdef DX11
							//interferes with SAO m_pImmediateContext->ClearpthStencilView(g_pGlob->pCurrentDepthView, D3D11_CLEAR_DEPTH, 1.0f, 0);
							#else
							m_pD3D->Clear ( 0, NULL, D3DCLEAR_ZBUFFER, 0, 1.0f, 0 );
							#endif

							// New camera established
							bResetCamera=true;
						}
					}
					else
					{
						if ( bResetCamera==true )
						{
							// Restore camera view
							GGSetTransform ( GGTS_VIEW, &matCurrentCameraView );
							SetCameraFOV ( fCurrentFOV );
							bResetCamera=false;
						}
					}

					// draw
					DrawObject ( pObject, false );
				}
				if ( bResetCamera )
				{
					// Restore camera view
					GGSetTransform ( GGTS_VIEW, &matCurrentCameraView );
					bResetCamera=false;

					// restore FOV if Locked Object set it (replaced in DBO system wioth better method)
					SetCameraFOV ( fCurrentFOV );
				}
			}

			// restore saved camera range
			if ( bCameraRangeAndProjectionChanged ) //PE: restore cam.
			{
				SetCameraRange ( 0, fCurrentNearRange, fCurrentFarRange );
			}
		}
		break;
	}

	// okay
	return true;
}

bool CObjectManager::Update ( void )
{
	// leeadd - U71 - can render even earlier in pipeline, so this can be flagged to happen earlier in UpdateOnce
	if ( g_bScenePrepared==false )
	{
		// Prepare main render
		UpdateInit();

		// prepare initial scene states
		if ( !PreSceneSettings ( ) )
			return false;

		// scene prepared
		g_bScenePrepared = true;
	}
	
	// Main layer render
	UpdateLayer ( 0 );

	// okay
	return true;
}

bool CObjectManager::UpdateGhostLayer ( void )
{
	// lee - 050406 - u6rc6 - overlay render layer (ghost used to be part of UpdateNoZLayer)
	Reset();
	UpdateLayer ( 3 );
	return true;
}

bool CObjectManager::UpdateNoZLayer ( void )
{
	// Must reset when return to manager
	Reset();

	// Overlay render layer (lock, nozdepth)
	UpdateLayer ( 4 );

	//PE: End mesh light system.
	#ifndef NOSTEAMORVIDEO
	end_mesh_light();
	#endif

	// okay
	return true;
}

void CObjectManager::SetGlobalShadowsOn ( void )
{
	m_bGlobalShadows = true;
}

void CObjectManager::SetGlobalShadowsOff ( void )
{
	m_bGlobalShadows = false;
}

int CObjectManager::GetVisibleObjectCount ( void )
{
	// 301007 - new function
	return m_iVisibleObjectCount;
}

sObject** CObjectManager::GetSortedObjectVisibleList ( void )
{
	// 301007 - new function
	return m_ppSortedObjectVisibleList;
}
