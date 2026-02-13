//
// SCORCH CODE
//

void cUniverse::SetScorchTexture ( int iImageID, int iWidth, int iHeight )
{
	// create scorch mesh if not exist
	if ( m_pScorchMesh==NULL )
	{
		// create large mesh to hold all scorch geometry for scene
		sMesh* pMesh = new sMesh;

		// scorch mesh description
		DWORD dwFVF = D3DFVF_XYZ | D3DFVF_TEX1;
		DWORD dwIndexCount  = 0xFFFF;
		DWORD dwScorchCount = dwIndexCount / 6;
		DWORD dwVertexCount = dwScorchCount*4;

		// create the single scorch mesh
		if ( !SetupMeshFVFData ( pMesh, dwFVF, dwVertexCount, dwIndexCount ) )
		{
			SAFE_DELETE(pMesh);
			return;
		}

		// create layer of scorch polys for visual reference
		DWORD v=0, i=0;
		for ( DWORD p=0; p<dwScorchCount; p++ )
		{
			// poly detail
			float fX = 10.0f;
			float fY = p*0.01f;
			float fZ = 10.0f;

			// create vertex data
			SetupStandardVertex ( dwFVF, pMesh->pVertexData, v+0, 0.0f, fY, 0.0f,  0.0f, 1.0f, 0.0f,  0xFFFFFFFF,  0.0f, 0.0f );
			SetupStandardVertex ( dwFVF, pMesh->pVertexData, v+1, 0.0f, fY, fZ,    0.0f, 1.0f, 0.0f,  0xFFFFFFFF,  0.0f, 1.0f );
			SetupStandardVertex ( dwFVF, pMesh->pVertexData, v+2, fX,   fY, 0.0f,  0.0f, 1.0f, 0.0f,  0xFFFFFFFF,  1.0f, 0.0f );
			SetupStandardVertex ( dwFVF, pMesh->pVertexData, v+3, fX,   fY, fZ,    0.0f, 1.0f, 0.0f,  0xFFFFFFFF,  1.0f, 1.0f );

			// create index data
			pMesh->pIndices [ i++ ] = v+0;
			pMesh->pIndices [ i++ ] = v+1;
			pMesh->pIndices [ i++ ] = v+2;
			pMesh->pIndices [ i++ ] = v+2;
			pMesh->pIndices [ i++ ] = v+1;
			pMesh->pIndices [ i++ ] = v+3;

			// next
			v+=4;
		}

		// reset draw limit
		m_dwPolyDrawLimit = 0;
		
		// setup mesh drawing properties
		pMesh->iPrimitiveType   = D3DPT_TRIANGLELIST;
		pMesh->iDrawVertexCount = dwVertexCount;
		pMesh->iDrawPrimitives  = m_dwPolyDrawLimit;

		// set some properties common to scorch
		pMesh->bTransparency=true;
		pMesh->bZWrite=false;

		// add this mesh to the draw buffers
		m_ObjectManager.AddObjectMeshToBuffers ( pMesh, true );

		// assign new mesh to ptr
		m_pScorchMesh = pMesh;
		m_iScorchTypeMax = (iWidth*iHeight)-1;
		m_iScorchUVWidth = iWidth;
		m_fScorchUTile = 1.0f/iWidth;
		m_fScorchVTile = 1.0f/iHeight;
	}

	// apply texture image to scorch mesh
	SetBaseTexture ( m_pScorchMesh, 0, iImageID );

	// set texture address mode
	SetTextureMode ( m_pScorchMesh, 3, 0 );

	// scorch texture must account for FOG (FPSCV104RC5)
	SetGhost ( m_pScorchMesh, true, 11 ); //1 );

	// set scorch texture to use texture only ( no diffuse )
	m_pScorchMesh->pTextures [ 0 ].dwBlendMode = D3DTOP_MODULATE;
	m_pScorchMesh->pTextures [ 0 ].dwBlendArg1 = D3DTA_TEXTURE;
	m_pScorchMesh->pTextures [ 0 ].dwBlendArg2 = D3DTA_DIFFUSE;
	m_pScorchMesh->bFog = true;
}

void cUniverse::AddScorch ( float fSize, int iScorchType )
{
	// only if scorch setup
	if ( m_pScorchMesh )
	{
		// ensure type does not exceed max
		if ( iScorchType < 0 ) iScorchType=0;
		if ( iScorchType > m_iScorchTypeMax ) iScorchType=m_iScorchTypeMax;

		// UV data for correct bullet hole
		int iDown = (iScorchType/m_iScorchUVWidth);
		int iAcross = iScorchType - (iDown * m_iScorchUVWidth);
		float fBaseU = iAcross*m_fScorchUTile;
		float fBaseV = iDown*m_fScorchVTile;
		float fU0 = fBaseU+m_fScorchUTile;
		float fV0 = fBaseV;
		float fU1 = fBaseU+m_fScorchUTile;
		float fV1 = fBaseV+m_fScorchVTile;
		float fU2 = fBaseU;
		float fV2 = fBaseV;
		float fU3 = fBaseU;
		float fV3 = fBaseV+m_fScorchVTile;
		D3DXVECTOR3 vec0 = MegaCollisionFeedback.vecHitPoint;
		D3DXVECTOR3 vec1 = MegaCollisionFeedback.vecHitPoint;
		D3DXVECTOR3 vec2 = MegaCollisionFeedback.vecHitPoint;
		D3DXVECTOR3 vec3 = MegaCollisionFeedback.vecHitPoint;
		D3DXVECTOR3 vecN = MegaCollisionFeedback.vecNormal;
		float ax, ay, az;
		GetAngleFromPoint ( 0.0f, 0.0f, 0.0f, vecN.x, vecN.y, vecN.z, &ax, &ay, &az );
		D3DXMATRIX matOut;
		D3DXMatrixRotationYawPitchRoll ( &matOut, D3DXToRadian(ay), D3DXToRadian(ax), D3DXToRadian(az) );
		D3DXVECTOR3 vecA = D3DXVECTOR3( -0.5f,  0.5f, 0.0f);
		D3DXVECTOR3 vecB = D3DXVECTOR3( -0.5f, -0.5f, 0.0f);
		D3DXVECTOR3 vecC = D3DXVECTOR3(  0.5f,  0.5f, 0.0f);
		D3DXVECTOR3 vecD = D3DXVECTOR3(  0.5f, -0.5f, 0.0f);
		vecA*=fSize;
		vecB*=fSize;
		vecC*=fSize;
		vecD*=fSize;
		D3DXVec3TransformCoord ( &vecA, &vecA, &matOut );
		D3DXVec3TransformCoord ( &vecB, &vecB, &matOut );
		D3DXVec3TransformCoord ( &vecC, &vecC, &matOut );
		D3DXVec3TransformCoord ( &vecD, &vecD, &matOut );
		D3DXVECTOR3 vecAt = vecN;
		vecAt=vecAt/5.0f;
		vec0+=vecA+vecAt;
		vec1+=vecB+vecAt;
		vec2+=vecC+vecAt;
		vec3+=vecD+vecAt;

		// if corners of scorch touch mesh, add it (not leaking off edge)
		if ( MegaCollisionFeedback.pHitMesh )
		{
			if(	CollisionQuickRayCast ( MegaCollisionFeedback.pHitMesh, vec0.x+vecN.x, vec0.y+vecN.y, vec0.z+vecN.z, vec0.x-vecN.x, vec0.y-vecN.y, vec0.z-vecN.z )
			&&	CollisionQuickRayCast ( MegaCollisionFeedback.pHitMesh, vec1.x+vecN.x, vec1.y+vecN.y, vec1.z+vecN.z, vec1.x-vecN.x, vec1.y-vecN.y, vec1.z-vecN.z )
			&&	CollisionQuickRayCast ( MegaCollisionFeedback.pHitMesh, vec2.x+vecN.x, vec2.y+vecN.y, vec2.z+vecN.z, vec2.x-vecN.x, vec2.y-vecN.y, vec2.z-vecN.z )
			&&	CollisionQuickRayCast ( MegaCollisionFeedback.pHitMesh, vec3.x+vecN.x, vec3.y+vecN.y, vec3.z+vecN.z, vec3.x-vecN.x, vec3.y-vecN.y, vec3.z-vecN.z ) )
			{
				// add detected scorch polygons to scorch mesh
				DWORD dwVertPos=0;
				sMesh* pMesh = m_pScorchMesh;
				SetupShortVertex ( pMesh->dwFVF, pMesh->pVertexData, m_dwScorchVPos+0, vec0.x, vec0.y, vec0.z, fU0, fV0 );
				SetupShortVertex ( pMesh->dwFVF, pMesh->pVertexData, m_dwScorchVPos+1, vec1.x, vec1.y, vec1.z, fU1, fV1 );
				SetupShortVertex ( pMesh->dwFVF, pMesh->pVertexData, m_dwScorchVPos+2, vec2.x, vec2.y, vec2.z, fU2, fV2 );
				SetupShortVertex ( pMesh->dwFVF, pMesh->pVertexData, m_dwScorchVPos+3, vec3.x, vec3.y, vec3.z, fU3, fV3 );

				// update mesh at this point (record for later)
				DWORD m_dwScorchVPosAdded = m_dwScorchVPos;

				// wrap around when get to end of scorch-polys
				m_dwScorchVPos+=4;
				if ( m_dwScorchVPos>pMesh->dwVertexCount-4 )
					m_dwScorchVPos=0;

				// wrap around when INDEX BUFFER max reached
				if ( (m_dwScorchVPos/4) >= (0xFFFF/6) )
					m_dwScorchVPos=0;

				// increment draw limit (only need to draw what has been created)
				if ( m_dwPolyDrawLimit<m_dwScorchVPos/2 )
				{
					m_dwPolyDrawLimit=m_dwScorchVPos/2;
					pMesh->iDrawPrimitives  = m_dwPolyDrawLimit;
				}

				// update mesh (quickly)
				m_ObjectManager.QuicklyUpdateObjectMeshInBuffer ( pMesh, m_dwScorchVPosAdded, m_dwScorchVPosAdded+4 );
			}
		}
	}
}

//
// Shadow functions
//

void cUniverse::SetShadow ( sMesh* pMeshCastingShadow, D3DXMATRIX* pMeshMatrix )
{
	// Activate stencil effect
	g_pGlob->dwStencilShadowCount++;
	g_pGlob->dwStencilMode=1;
	g_pGlob->dwRedrawCount=1;

	// add caster mesh and matrix to shadow caster master list
	sShadowCaster ShadowCaster;
	if ( pMeshMatrix )
		ShadowCaster.matPos = *pMeshMatrix;
	else
		D3DXMatrixIdentity ( &ShadowCaster.matPos );

	ShadowCaster.pMeshRef = pMeshCastingShadow;
	m_pShadowCasterMasterList.push_back ( ShadowCaster );
}

void cUniverse::SetLight ( int iLightIndex, float fX, float fY, float fZ, float fRange, bool bRecreateAllShadows )
{
	// check range
	if ( iLightIndex < 1 || iLightIndex > 256 )
		return;

	// expand list to include index
	if ( iLightIndex >= (int)m_pShadowLightList.size ( ) )
	{
		sShadowLight shadowlight;

		while ( iLightIndex >= (int)m_pShadowLightList.size ( ) )
			m_pShadowLightList.push_back ( shadowlight );
	}

	// set data for specified light
	sShadowLight* pCurrentLight = &m_pShadowLightList[iLightIndex];
	if ( pCurrentLight )
	{
		// set light position
		pCurrentLight->fX = fX;
		pCurrentLight->fY = fY;
		pCurrentLight->fZ = fZ;
		pCurrentLight->fRange = fRange;

		// find all casters within light range (range code later)
		if ( bRecreateAllShadows==true && pCurrentLight->pShadowMeshList.size()==0)
		{
			// free any old shadows
			for ( int iShadow=0; iShadow<(int)pCurrentLight->pShadowMeshList.size(); iShadow++ )
				SAFE_DELETE ( pCurrentLight->pShadowMeshList [ iShadow ] );

			// clears lists
			pCurrentLight->pCasterRefList.clear();
			pCurrentLight->pShadowMeshList.clear();

			// create new shadows
			for ( int iCaster=0; iCaster<(int)m_pShadowCasterMasterList.size(); iCaster++ )
			{
				// for each caster
				sShadowCaster* pShadowCaster = &m_pShadowCasterMasterList [ iCaster ];
				if ( pShadowCaster )
				{
					// create new shadow mesh just for this caster and light (ie caster can have ten shadows if there are ten lights)
					sMesh* pShadowMesh = NULL;
					if ( CreateShadowMesh ( pShadowCaster->pMeshRef, &pShadowMesh, 0 ) )
					{
						// shadow valid
						if ( pShadowMesh )
						{
							// shadow mesh is outside object update cycle, so do manually
							m_ObjectManager.AddObjectMeshToBuffers ( pShadowMesh, false );

							// add to list
							pCurrentLight->pCasterRefList.push_back ( pShadowCaster );
							pCurrentLight->pShadowMeshList.push_back ( pShadowMesh );
						}
					}
				}
			}
		}

		// for each shadow owned by light
		for ( int iShadow=0; iShadow<(int)pCurrentLight->pShadowMeshList.size(); iShadow++ )
		{
			// update shadow to new altered light position
			sMesh* pCasterMesh = pCurrentLight->pCasterRefList [ iShadow ]->pMeshRef;
			D3DXMATRIX* pCasterPos = &pCurrentLight->pCasterRefList [ iShadow ]->matPos;
			sMesh* pShadowMesh = pCurrentLight->pShadowMeshList [ iShadow ];
			// migrated shadows to shader - finish this when time is right
			// UpdateShadowMesh ( pCurrentLight->fX, pCurrentLight->fY, pCurrentLight->fZ, pCurrentLight->fRange, pCasterMesh, pShadowMesh, pCasterPos, NULL );
			// Put this back on 050908 as it stopped static shadows working..
			// but we need the shadows to cast 'away from light', so we need to fill the caster pos which seems
			// not to have any world data for the static objects in this calculation right now, so we add it:
			pCurrentLight->pCasterRefList [ iShadow ]->matPos._41 = pCasterMesh->Collision.vecCentre.x;
			pCurrentLight->pCasterRefList [ iShadow ]->matPos._42 = pCasterMesh->Collision.vecCentre.y;
			pCurrentLight->pCasterRefList [ iShadow ]->matPos._43 = pCasterMesh->Collision.vecCentre.z;
			UpdateShadowMesh ( pCurrentLight->fX, pCurrentLight->fY, pCurrentLight->fZ, pCurrentLight->fRange, pCasterMesh, pShadowMesh, pCasterPos, NULL );
			m_ObjectManager.UpdateObjectMeshInBuffer ( pShadowMesh );
		}

		// unlock all buffers after updates
		m_ObjectManager.CompleteUpdateInBuffers();
	}
}

void cUniverse::RenderShadows ( void )
{
	// see debug
//	bool bNoDebugShadows = false;
	bool bNoDebugShadows = true;

	// set world matrix
	D3DXMATRIX matDef;
	D3DXMatrixIdentity ( &matDef );
	m_pD3D->SetTransform ( D3DTS_WORLD, &matDef );

	// setup for stencil writing
	if ( bNoDebugShadows ) m_ObjectManager.StartStencilMeshWrite();

	// render all shadows owned by lights within range of current location
	for ( int iLightIndex=0; iLightIndex<(int)m_pShadowLightList.size(); iLightIndex++ )
	{
		// for all shadow lights in universe
		sShadowLight* pShadowLight = &m_pShadowLightList [ iLightIndex ];
		if ( pShadowLight )
		{
			// all shadows belonging to light
			for ( int iShadow=0; iShadow<(int)pShadowLight->pShadowMeshList.size(); iShadow++ )
			{
				// draw all universe shadow meshes
				if ( bNoDebugShadows ) 
					m_ObjectManager.DrawStencilMesh ( 0, NULL, pShadowLight->pShadowMeshList [ iShadow ] );
				else
				{
					pShadowLight->pShadowMeshList [ iShadow ]->bZRead = false;
					pShadowLight->pShadowMeshList [ iShadow ]->bLight = false;
					pShadowLight->pShadowMeshList [ iShadow ]->bWireframe = true;
					m_ObjectManager.DrawMesh ( pShadowLight->pShadowMeshList [ iShadow ] );
				}
			}
		}
	}

	// finish from stencil writing
	if ( bNoDebugShadows ) m_ObjectManager.FinishStencilMeshWrite();
}

//
// fast collision (for lightmapping)
//

void cUniverse::ShortlistMeshesWithinBox ( D3DXVECTOR3* pvecMin, D3DXVECTOR3* pvecMax )
{
	// calculate correct boundbox
	D3DXVECTOR3 vecMin = *pvecMin;
	D3DXVECTOR3 vecMax = *pvecMax;
	if ( pvecMax->x<vecMin.x) { vecMin.x=pvecMax->x; vecMax.x=pvecMin->x; }
	if ( pvecMax->y<vecMin.y) { vecMin.y=pvecMax->y; vecMax.y=pvecMin->y; }
	if ( pvecMax->z<vecMin.z) { vecMin.z=pvecMax->z; vecMax.z=pvecMin->z; }

	// scan nodes, and collect all meshes touching input box
	m_pMeshShortList.clear();

	// go through whole node universe
	for ( int iNode = 0; iNode < m_iNodeListSize; iNode++ )
	{
		// get node ptr
		sNode* pNode = &m_pNode [ iNode ];

		// reject quickly if nodebox completely outside input box
		D3DXVECTOR3 vecNodeMin = pNode->vecCentre - pNode->vecDimension;
		D3DXVECTOR3 vecNodeMax = pNode->vecCentre + pNode->vecDimension;
		if ( CollisionBoundBoxTest ( &vecMin, &vecMax, &vecNodeMin, &vecNodeMax ) )
		{
			// go through each mesh in the node
			for ( int iMesh = 0; iMesh < pNode->iMeshCount; iMesh++ )
			{
				// get pointer to mesh
				sMesh* pMesh = pNode->ppMeshList [ iMesh ];

				// if meshbox touches input box
				if ( CollisionBoundBoxTest ( &vecMin, &vecMax, &pMesh->Collision.vecMin, &pMesh->Collision.vecMax ) )
				{
					// add mesh to shortlist of meshes
					m_pMeshShortList.push_back ( pMesh );
				}
			}
		}
	}
}

// mike - 080305 - so we can get the master mesh list
void cUniverse::GetMasterMeshList ( vector < sMesh* > *pMeshList )
{
	// all meshgroups in universe
	for ( int iAreaBox = 0; iAreaBox < (int)m_pAreaList.size ( ); iAreaBox++ )
	{
	    int iIndex = 0;
	    
		sArea* pArea = m_pAreaList [ iAreaBox ];
		for ( iIndex = 0; iIndex < (int)pArea->meshgroups.size ( ); iIndex++ )
		{
			sMesh* pMesh = pArea->meshgroups [ iIndex ]->pMesh;
			pMeshList->push_back ( pMesh );
		}
		for ( iIndex = 0; iIndex < (int)pArea->sharedmeshgroups.size ( ); iIndex++ )
		{
			sMesh* pMesh = pArea->sharedmeshgroups [ iIndex ]->pMesh;
			pMeshList->push_back ( pMesh );
		}
	}
}
