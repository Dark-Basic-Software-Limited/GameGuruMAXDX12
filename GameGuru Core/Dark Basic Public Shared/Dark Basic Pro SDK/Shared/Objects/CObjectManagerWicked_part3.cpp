/*
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
			if ( bCameraRangeAndProjectionChanged == false )
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
	end_mesh_light();

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
*/
