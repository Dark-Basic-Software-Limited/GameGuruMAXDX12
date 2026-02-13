bool cUniverse::CollisionVeryQuickRayCast ( sMesh* pMesh, D3DXVECTOR3* pvecLineStart, D3DXVECTOR3* pvecLineEnd )
{
	// calculate direction
	D3DXVECTOR3 vecDirection;
	D3DXVECTOR3 vecDifference = *pvecLineEnd - *pvecLineStart;
	D3DXVec3Normalize ( &vecDirection, &vecDifference );

	// get a pointer to the vertex data
	BYTE* pVertex = pMesh->pVertexData;
	int   iIndex  = 0;

	// get the offset map
	sOffsetMap	 offsetMap;
	GetFVFOffsetMap ( pMesh, &offsetMap );

	// indices or vertonly
	int iIndexMax = 0;
	if ( pMesh->dwIndexCount>0 )
		iIndexMax = (int)pMesh->dwIndexCount/3;
	else
		iIndexMax = (int)pMesh->dwVertexCount/3;

	// now go through each triangle
	for ( int iTriangle = 0; iTriangle < iIndexMax; iTriangle++ )
	{
		// get each vector
		D3DXVECTOR3 vecA, vecB, vecC;
		if ( pMesh->dwIndexCount>0 )
		{
			// indices
			vecA = D3DXVECTOR3 ( 
												*( ( float* ) pVertex + offsetMap.dwX + ( offsetMap.dwSize * pMesh->pIndices [ iIndex   ] ) ),
												*( ( float* ) pVertex + offsetMap.dwY + ( offsetMap.dwSize * pMesh->pIndices [ iIndex   ] ) ),
												*( ( float* ) pVertex + offsetMap.dwZ + ( offsetMap.dwSize * pMesh->pIndices [ iIndex++ ] ) )
										   );

			vecB = D3DXVECTOR3 ( 
												*( ( float* ) pVertex + offsetMap.dwX + ( offsetMap.dwSize * pMesh->pIndices [ iIndex   ] ) ),
												*( ( float* ) pVertex + offsetMap.dwY + ( offsetMap.dwSize * pMesh->pIndices [ iIndex   ] ) ),
												*( ( float* ) pVertex + offsetMap.dwZ + ( offsetMap.dwSize * pMesh->pIndices [ iIndex++ ] ) )
										   );

			vecC = D3DXVECTOR3 ( 
												*( ( float* ) pVertex + offsetMap.dwX + ( offsetMap.dwSize * pMesh->pIndices [ iIndex   ] ) ),
												*( ( float* ) pVertex + offsetMap.dwY + ( offsetMap.dwSize * pMesh->pIndices [ iIndex   ] ) ),
												*( ( float* ) pVertex + offsetMap.dwZ + ( offsetMap.dwSize * pMesh->pIndices [ iIndex++ ] ) )
										   );
		}
		else
		{
			// vertonly
			vecA = D3DXVECTOR3 ( 
												*( ( float* ) pVertex + offsetMap.dwX + ( offsetMap.dwSize * iIndex ) ),
												*( ( float* ) pVertex + offsetMap.dwY + ( offsetMap.dwSize * iIndex ) ),
												*( ( float* ) pVertex + offsetMap.dwZ + ( offsetMap.dwSize * iIndex ) )
										   );
			iIndex++;

			vecB = D3DXVECTOR3 ( 
												*( ( float* ) pVertex + offsetMap.dwX + ( offsetMap.dwSize * iIndex ) ),
												*( ( float* ) pVertex + offsetMap.dwY + ( offsetMap.dwSize * iIndex ) ),
												*( ( float* ) pVertex + offsetMap.dwZ + ( offsetMap.dwSize * iIndex ) )
										   );
			iIndex++;

			vecC = D3DXVECTOR3 ( 
												*( ( float* ) pVertex + offsetMap.dwX + ( offsetMap.dwSize * iIndex ) ),
												*( ( float* ) pVertex + offsetMap.dwY + ( offsetMap.dwSize * iIndex ) ),
												*( ( float* ) pVertex + offsetMap.dwZ + ( offsetMap.dwSize * iIndex ) )
										   );
			iIndex++;
		}

		// tri test against ray
		float fU, fV, fDistance;
		if ( D3DXIntersectTri ( &vecA, &vecB, &vecC, pvecLineStart, &vecDirection, &fU, &fV, &fDistance ) )
		{
			float fLengthOfRay = D3DXVec3Length ( &vecDifference );
			if ( fDistance < fLengthOfRay-0.01f )
				return true;
		}
	}

	// no collision with this mesh
	return false;
}

bool cUniverse::CollisionBoundBoxTest ( D3DXVECTOR3* pvecMinA, D3DXVECTOR3* pvecMaxA, D3DXVECTOR3* pvecMinB, D3DXVECTOR3* pvecMaxB )
{
	// check box intersection
	if (pvecMaxA->x >= pvecMinB->x && pvecMinA->x <= pvecMaxB->x &&
		pvecMaxA->y >= pvecMinB->y && pvecMinA->y <= pvecMaxB->y &&
		pvecMaxA->z >= pvecMinB->z && pvecMinA->z <= pvecMaxB->z )
		return true;
	else
		return false;
}

int cUniverse::CollisionRayCast ( float fX, float fY, float fZ, float fNewX, float fNewY, float fNewZ )
{
	// U75 - 150610 - reset so this value remains fresh and reflective of current material after each collision call
	// U76 - 130710 - moved from within ray cast loop (as it got wiped if complex mesh/andor many iterations)
	g_DBPROCollisionResult.dwArbitaryValue = 0;

	// for best result from cast
	float fBestDistance = 99999.0f;
	int iBestReturnDistance = 0;
	float fBestU, fBestV;
	sMesh* pBestMesh;
	int iBestTriangle;

	// calculate boundbox
	D3DXVECTOR3 vecMin, vecMax;
	vecMin.x=fX; vecMin.y=fY; vecMin.z=fZ;
	vecMax.x=fX; vecMax.y=fY; vecMax.z=fZ;
	if ( fNewX<vecMin.x ) vecMin.x=fNewX;
	if ( fNewY<vecMin.y ) vecMin.y=fNewY;
	if ( fNewZ<vecMin.z ) vecMin.z=fNewZ;
	if ( fNewX>vecMax.x ) vecMax.x=fNewX;
	if ( fNewY>vecMax.y ) vecMax.y=fNewY;
	if ( fNewZ>vecMax.z ) vecMax.z=fNewZ;

	// V119 - 081211 - bug meaning we ignore meshes in meshref (large meshes that intersect an areabox, but the
	// areabox containing them is not intersected by the ray bound box!
	// so we clear some flags and tranverse the meshref meshes too, only once
	if ( m_pMasterMeshList.size()>0 )
	{
		for ( int iMeshIndex = 0; iMeshIndex < (int)m_pMasterMeshList.size ( ); iMeshIndex++ )
		{
			sMesh* pMesh = m_pMasterMeshList [ iMeshIndex ];
			pMesh->dwTempFlagUsedDuringUniverseRayCast = 0;
		}
	}

	// go through all areaboxes that touch boundbox
	for ( int iAreaBox = 0; iAreaBox < (int)m_pAreaList.size ( ); iAreaBox++ )
	{
		// get current area box ptr
		sArea* pArea = m_pAreaList [ iAreaBox ];

		// is mesh inside areabox
		if ( vecMax.x > pArea->vecMin.x
		&&   vecMin.x < pArea->vecMax.x
		&&   vecMax.y > pArea->vecMin.y
		&&   vecMin.y < pArea->vecMax.y
		&&   vecMax.z > pArea->vecMin.z
		&&   vecMin.z < pArea->vecMax.z )
		{
			if ( m_pMasterMeshList.size()>0 )
			{
				// new collision against meshgroups
				// each area box has within and ahred meshgroups
				//for ( int iBothMeshGroups=0; iBothMeshGroups<2; iBothMeshGroups++ ) // V119 also checking meshref now
				for ( int iBothMeshGroups=0; iBothMeshGroups<3; iBothMeshGroups++ )
				{
					int iIndexMax = 0;
					if ( iBothMeshGroups==0 ) iIndexMax = pArea->meshgroups.size ( );
					if ( iBothMeshGroups==1 ) iIndexMax = pArea->sharedmeshgroups.size ( );
					if ( iBothMeshGroups==2 ) iIndexMax = pArea->meshgroupref.size();
					for ( int iIndex = 0; iIndex < iIndexMax; iIndex++ )
					{
						sMesh* pMesh = NULL;
						if ( iBothMeshGroups==0 ) pMesh = pArea->meshgroups [ iIndex ]->pMesh;
						if ( iBothMeshGroups==1 ) pMesh = pArea->sharedmeshgroups [ iIndex ]->pMesh;
						if ( iBothMeshGroups==2 ) pMesh = pArea->meshgroupref [ iIndex ]->pMesh;
						if ( pMesh )
						{
							// V119 optmise - only check each mesh once for this ray check!
							if ( pMesh->dwTempFlagUsedDuringUniverseRayCast==0 )
							{
								// we check this mesh ONCE to see if the ray intersects it
								pMesh->dwTempFlagUsedDuringUniverseRayCast = 1;
								if ( CollisionBoundBoxTest ( &vecMin, &vecMax, &pMesh->Collision.vecMin, &pMesh->Collision.vecMax ) )
								{
									// raycast against mesh
									if ( CollisionSingleRayCast ( pMesh, fX, fY, fZ, fNewX, fNewY, fNewZ,
																  &fBestDistance, &fBestU, &fBestV, &pBestMesh, &iBestTriangle, true ) )
									{
										// store node of successful collision thus far
										D3DXVECTOR3 vecRayDist = vecMax - vecMin;
										float fRayDist = D3DXVec3Length ( &vecRayDist );
										if ( fBestDistance<fRayDist )
										{
											iBestReturnDistance = (int)fBestDistance;;
											if ( iBestReturnDistance<1 ) iBestReturnDistance=1;
										}
									}
								}
							}
						}
					}
				}
			}
			else
			{
				// old collision with meshes of node
				for ( int iNode = 0; iNode < (int)pArea->nodes.size ( ); iNode++ )
				{
					// get a pointer to the node
					sNode* pNode = pArea->nodes [ iNode ];
					for ( int iMesh = 0; iMesh < pNode->iMeshCount; iMesh++ )
					{
						sMesh* pMesh = pNode->ppMeshList [ iMesh ];
						if ( pMesh )
						{
							if ( CollisionBoundBoxTest ( &vecMin, &vecMax, &pMesh->Collision.vecMin, &pMesh->Collision.vecMax ) )
							{
								// raycast against mesh
								if ( CollisionSingleRayCast ( pMesh, fX, fY, fZ, fNewX, fNewY, fNewZ,
															  &fBestDistance, &fBestU, &fBestV, &pBestMesh, &iBestTriangle, true ) )
								{
									// store node of successful collision thus far
									D3DXVECTOR3 vecRayDist = vecMax - vecMin;
									float fRayDist = D3DXVec3Length ( &vecRayDist );
									if ( fBestDistance<fRayDist )
									{
										iBestReturnDistance = (int)fBestDistance;;
										if ( iBestReturnDistance<1 ) iBestReturnDistance=1;
									}
								}
							}
						}
					}
				}
			}
		}
	}

	// collision with ray
	return iBestReturnDistance;
}

void cUniverse::AddNodeColDataToPolygonList ( sNode* pCurrentNode )
{
	for ( int iA = 0; iA < (int)pCurrentNode->collisionE.size ( ); iA++ )
	{
		// expand pool if required
		if ( m_pCollisionPool==NULL )
		{
			m_dwCollisionPoolMax = 1024*9;
			m_pCollisionPool = new float [ m_dwCollisionPoolMax ];
			m_pCollisionDiffuse = new DWORD [ m_dwCollisionPoolMax/9 ];
		}
		else
		{
			if ( m_dwCollisionPoolIndex+4 >= m_dwCollisionPoolMax )
			{
				// pool
				float* pNewLargerPool = new float [ m_dwCollisionPoolMax+(512*9) ];
				memcpy ( pNewLargerPool, m_pCollisionPool, m_dwCollisionPoolMax * sizeof(float) );
				SAFE_DELETE(m_pCollisionPool);
				m_pCollisionPool = pNewLargerPool;

				// diffuse
				DWORD* pNewLargerDiffuse = new DWORD [ (m_dwCollisionPoolMax/9)+512 ];
				memcpy ( pNewLargerDiffuse, m_pCollisionDiffuse, (m_dwCollisionPoolMax/9) * sizeof(DWORD) );
				SAFE_DELETE(m_pCollisionDiffuse);
				m_pCollisionDiffuse = pNewLargerDiffuse;

				// increment permimnantly
				m_dwCollisionPoolMax+=(512*9);
			}
		}

		// fill pool with a polygon
		float* pPoolPtr = &(m_pCollisionPool [ m_dwCollisionPoolIndex ]);
		*(pPoolPtr+0) = pCurrentNode->collisionE [ iA ].triangle[0].vecPosition.x;
		*(pPoolPtr+1) = pCurrentNode->collisionE [ iA ].triangle[0].vecPosition.y;
		*(pPoolPtr+2) = pCurrentNode->collisionE [ iA ].triangle[0].vecPosition.z;
		*(pPoolPtr+3) = pCurrentNode->collisionE [ iA ].triangle[1].vecPosition.x;
		*(pPoolPtr+4) = pCurrentNode->collisionE [ iA ].triangle[1].vecPosition.y;
		*(pPoolPtr+5) = pCurrentNode->collisionE [ iA ].triangle[1].vecPosition.z;
		*(pPoolPtr+6) = pCurrentNode->collisionE [ iA ].triangle[2].vecPosition.x;
		*(pPoolPtr+7) = pCurrentNode->collisionE [ iA ].triangle[2].vecPosition.y;
		*(pPoolPtr+8) = pCurrentNode->collisionE [ iA ].triangle[2].vecPosition.z;
		m_pCollisionDiffuse [ m_dwCollisionPoolIndex/9 ] = pCurrentNode->collisionE [ iA ].diffuseid;
		m_dwCollisionPoolIndex+=9;
	}
}

void cUniverse::AddNodeColDataIfWithinBound ( sNode* pCheckNode, D3DXVECTOR3* pvecMin, D3DXVECTOR3* pvecMax )
{
	if ( pCheckNode )
	{
//		leefix - 030205 - replaced with colE bound box check (for static entities overlapping node bounds)
		/*
		float fCentreX = pCheckNode->vecCentre.x;
		float fCentreY = pCheckNode->vecCentre.y;
		float fCentreZ = pCheckNode->vecCentre.z;
		float fDimX    = pCheckNode->vecDimension.x;
		float fDimY    = pCheckNode->vecDimension.y;
		float fDimZ    = pCheckNode->vecDimension.z;
		if ( pvecMin->x <= fCentreX+fDimX )
			if ( pvecMin->y <= fCentreY+fDimY )
				if ( pvecMin->z <= fCentreZ+fDimZ )
					if ( pvecMax->x >= fCentreX-fDimX )
						if ( pvecMax->y >= fCentreY-fDimY )
							if ( pvecMax->z >= fCentreZ-fDimZ )
								AddNodeColDataToPolygonList ( pCheckNode );
		*/
		if ( pvecMin->x <= pCheckNode->vecColMax.x )
			if ( pvecMin->y <= pCheckNode->vecColMax.y )
				if ( pvecMin->z <= pCheckNode->vecColMax.z )
					if ( pvecMax->x >= pCheckNode->vecColMin.x )
						if ( pvecMax->y >= pCheckNode->vecColMin.y )
							if ( pvecMax->z >= pCheckNode->vecColMin.z )
								AddNodeColDataToPolygonList ( pCheckNode );
	}
}

bool cUniverse::CollisionRayVolume ( float fOldX, float fOldY, float fOldZ, float fNewX, float fNewY, float fNewZ, float fScale )
{
	// result var
	bool bResult = false;

	// find pCentralNode of new pos
	sNode* pCentralNode = NULL;
	for ( int iObject = 0; iObject < m_iNodeListSize; iObject++ )
	{
		sNode* pNode = &(m_pNode [ iObject ]);
		if ( pNode )
		{
			float fCentreX = pNode->vecCentre.x;
			float fCentreY = pNode->vecCentre.y;
			float fCentreZ = pNode->vecCentre.z;
			float fDimX    = pNode->vecDimension.x;
			float fDimY    = pNode->vecDimension.y;
			float fDimZ    = pNode->vecDimension.z;
			if ( fNewX <= fCentreX+fDimX )
				if ( fNewY <= fCentreY+fDimY )
					if ( fNewZ <= fCentreZ+fDimZ )
						if ( fNewX >= fCentreX-fDimX )
							if ( fNewY >= fCentreY-fDimY )
								if ( fNewZ >= fCentreZ-fDimZ )
								{
									pCentralNode = pNode;
									break;
								}
		}
	}

	// collision data pre-warped to ellipse of 1,3,1 shape (for performance)
	D3DXVECTOR3 vecEllipse = D3DXVECTOR3 ( 10.0f, 30.0f, 10.0f );
	
	//  leeadd - 020205 - take into account not only current size, but size of maximum movement!!
	D3DXVECTOR3 vecMovingEllipse = D3DXVECTOR3 ( 20.0f, 40.0f, 20.0f );

	// calculate boundbox of volume
	D3DXVECTOR3 vecMin, vecMax;
	vecMin.x=fNewX-vecMovingEllipse.x; vecMin.y=fNewY-vecMovingEllipse.y; vecMin.z=fNewZ-vecMovingEllipse.z;
	vecMax.x=fNewX+vecMovingEllipse.x; vecMax.y=fNewY+vecMovingEllipse.y; vecMax.z=fNewZ+vecMovingEllipse.z;

	// clear polygon pool for new batch
	m_dwCollisionPoolIndex = 0;

	// include node and any neighbor nodes within range of volume
	if ( pCentralNode )
	{
		// Add any neighbors node colpolys that are within range
		for ( int iSide=0; iSide<=2; iSide++ )
		{
			// 0-X,1+X,2-center XCX
			sNode* pNode = pCentralNode->pNeighbours [ iSide ];
			if ( iSide==2 ) pNode = pCentralNode;
			AddNodeColDataIfWithinBound ( pNode, &vecMin, &vecMax );

			// for each X, do 4-Z 5+Z on this layer
			if ( pNode )
			{
				// For each node on this later, do 2-Y 3+Y
				for ( int iYSide=2; iYSide<=3; iYSide++ )
				{
					sNode* pCheckHeightNode = pNode->pNeighbours [ iYSide ];
					AddNodeColDataIfWithinBound ( pCheckHeightNode, &vecMin, &vecMax );
				}

				for ( int iZSide=4; iZSide<=5; iZSide++ )
				{
					// Both Z neighbors
					sNode* pCheckNode = pNode->pNeighbours [ iZSide ];
					AddNodeColDataIfWithinBound ( pCheckNode, &vecMin, &vecMax );

					// For each node on this later, do 2-Y 3+Y
					if ( pCheckNode )
					{
						for ( int iYSide=2; iYSide<=3; iYSide++ )
						{
							sNode* pCheckHeightNode = pCheckNode->pNeighbours [ iYSide ];
							AddNodeColDataIfWithinBound ( pCheckHeightNode, &vecMin, &vecMax );
						}
					}
				}
			}
			/* only checks a cross shape (forgets diagonals)
			// go through fore/back/down on each
			if ( pNode )
			{
				for ( int iZSide=2; iZSide<6; iZSide++ )
				{
					// choose node (9 nodes checked in all)
					sNode* pCheckNode = pNode->pNeighbours [ iZSide ];
					AddNodeColDataIfWithinBound ( pCheckNode, &vecMin, &vecMax );
				}
			}
			*/
		}
	}
	
	// Check against all sunmitted polygons
	if ( m_dwCollisionPoolIndex>0 )
	{
		// gather data for final collision check
		D3DXVECTOR3 vecOld     = D3DXVECTOR3 ( fOldX, fOldY, fOldZ );
		D3DXVECTOR3 vecNew     = D3DXVECTOR3 ( fNewX, fNewY, fNewZ );

		// do collision check
		Collision collision;
		collision.Init();
		if ( collision.World (	vecOld,	vecNew,	vecEllipse,	fScale ) )
			bResult = true;
		else
			bResult = false;

		// tally polys used
		m_dwPolygonsForCollision += (m_dwCollisionPoolIndex/9);
		m_dwVolumesTestedForCollision++;
	}
	else
	{
		// no polygons in area, no collision
		memset ( &g_DBPROCollisionResult, 0, sizeof(g_DBPROCollisionResult) );
		bResult = false;
	}

	// Using global store for final result
	g_DBPROCollisionResult.bUsed = true;

	// return result
	return bResult;
}

bool cUniverse::CollisionTest (	int iTestIndex, float fX1, float fY1, float fZ1, float fX2, float fY2, float fZ2,
								float fNewX1, float fNewY1, float fNewZ1, float fNewX2, float fNewY2, float fNewZ2 )
{
	// report best collision result
	return false;
}

bool cUniverse::SetDebugOn ( void )
{
	m_bDebug = true;
	m_bGhostDebug = true;
	return true;
}

bool cUniverse::SetDebugOff ( void )
{
	m_bDebug = false;
	m_bGhostDebug = false;
	return true;
}

bool cUniverse::AddPortals ( sNode* pNode )
{
	if ( !pNode )
		return true;

	if ( pNode->iMeshCount != 0 )
	{
		cBSPTree	g_tree;
		cProcessPRT	g_portals;
		sBSP		OptionsBSP;
		sPRT		OptionsPRT;
		sBSPStats   StatsBSP;
		sPRTStats   StatsPRT;

		memset ( &StatsBSP, 0, sizeof ( sBSPStats ) );
		memset ( &StatsPRT, 0, sizeof ( sPRTStats ) );

		typedef std::vector < cMesh* > vectorMesh;

		cMesh*			pNewMesh = new cMesh ( );
		int				iOffset  = 0;
		sMesh*			pMesh    = pNode->ppMeshList [ 0 ];
		vectorMesh      vpMeshList;

		if ( pMesh->bTransparency == 0 )
		{
			int iVertexCount = 0;

			OptionsBSP.bEnabled           = true;
			OptionsBSP.iTreeType          = BSP_TYPE_NONSPLIT;
			OptionsBSP.fSplitHeuristic    = 3.0f;
			OptionsBSP.iSplitterSample    = 60;
			OptionsBSP.bRemoveBackLeaves  = true;
			OptionsBSP.bAddBoundingPolys  = false;
			OptionsBSP.bLinkDetailBrushes = false;
			OptionsPRT.bEnabled           = true;
			OptionsPRT.bFullCompile       = true;

			pNewMesh->m_dwFaceCount = pMesh->dwIndexCount / 3;
			pNewMesh->m_ppFaces     = new cFace* [ pMesh->dwIndexCount / 3 ];

			// get the offset map for the FVF
			sOffsetMap offsetMap;
			GetFVFOffsetMap ( pMesh, &offsetMap );

			// go through faces
			for ( int iFace = 0; iFace < (int)pNewMesh->m_dwFaceCount; iFace++ )
			{
				pNewMesh->m_ppFaces [ iFace ] = new cFace;
				pNewMesh->m_ppFaces [ iFace ]->AddVertices ( 3 );

				for ( int iVertex = 0; iVertex < 3; iVertex++ )
				{
					// get vertex position from original mesh
					// get the x, y and z components
					sPortalVertex pVertex;
					memset ( &pVertex, 0, sizeof(pVertex) );
					
					int iVertexOffset = pMesh->pIndices [ iOffset++ ];

					if ( offsetMap.dwZ>0 )
					{
						pVertex.vecPosition.x = *( ( float* ) pMesh->pVertexData + offsetMap.dwX + ( offsetMap.dwSize * iVertexOffset ) );
						pVertex.vecPosition.y = *( ( float* ) pMesh->pVertexData + offsetMap.dwY + ( offsetMap.dwSize * iVertexOffset ) );
						pVertex.vecPosition.z = *( ( float* ) pMesh->pVertexData + offsetMap.dwZ + ( offsetMap.dwSize * iVertexOffset ) );
					}

					if ( offsetMap.dwNZ>0 )
					{
						pVertex.vecNormal.x = *( ( float* ) pMesh->pVertexData + offsetMap.dwNX + ( offsetMap.dwSize * iVertexOffset ) );
						pVertex.vecNormal.y = *( ( float* ) pMesh->pVertexData + offsetMap.dwNY + ( offsetMap.dwSize * iVertexOffset ) );
						pVertex.vecNormal.z = *( ( float* ) pMesh->pVertexData + offsetMap.dwNZ + ( offsetMap.dwSize * iVertexOffset ) );
					}

					if ( offsetMap.dwTV[0]>0 )
					{
						pVertex.tu = *( ( float* ) pMesh->pVertexData + offsetMap.dwTU[0] + ( offsetMap.dwSize * iVertexOffset ) );
						pVertex.tv = *( ( float* ) pMesh->pVertexData + offsetMap.dwTV[0] + ( offsetMap.dwSize * iVertexOffset ) );
					}

					pNewMesh->m_ppFaces [ iFace ]->m_pVertices [ iVertex ].x        = pVertex.vecPosition.x;
					pNewMesh->m_ppFaces [ iFace ]->m_pVertices [ iVertex ].y        = pVertex.vecPosition.y;
					pNewMesh->m_ppFaces [ iFace ]->m_pVertices [ iVertex ].z        = pVertex.vecPosition.z;
					pNewMesh->m_ppFaces [ iFace ]->m_pVertices [ iVertex ].tu       = pVertex.tu;
					pNewMesh->m_ppFaces [ iFace ]->m_pVertices [ iVertex ].tv       = pVertex.tv;
					pNewMesh->m_ppFaces [ iFace ]->m_pVertices [ iVertex ].Normal.x = pVertex.vecNormal.x;
					pNewMesh->m_ppFaces [ iFace ]->m_pVertices [ iVertex ].Normal.y = pVertex.vecNormal.y;
					pNewMesh->m_ppFaces [ iFace ]->m_pVertices [ iVertex ].Normal.z = pVertex.vecNormal.z;

					iVertexCount++;
				}

				// calculate the polygon normal
				D3DXVECTOR3 vec0 = D3DXVECTOR3 ( pNewMesh->m_ppFaces [ iFace ]->m_pVertices [ 0 ].x, pNewMesh->m_ppFaces [ iFace ]->m_pVertices [ 0 ].y, pNewMesh->m_ppFaces [ iFace ]->m_pVertices [ 0 ].z );
				D3DXVECTOR3 vec1 = D3DXVECTOR3 ( pNewMesh->m_ppFaces [ iFace ]->m_pVertices [ 1 ].x, pNewMesh->m_ppFaces [ iFace ]->m_pVertices [ 1 ].y, pNewMesh->m_ppFaces [ iFace ]->m_pVertices [ 1 ].z );
				D3DXVECTOR3 vec2 = D3DXVECTOR3 ( pNewMesh->m_ppFaces [ iFace ]->m_pVertices [ 2 ].x, pNewMesh->m_ppFaces [ iFace ]->m_pVertices [ 2 ].y, pNewMesh->m_ppFaces [ iFace ]->m_pVertices [ 2 ].z );

				// get the edges
				D3DXVECTOR3 edge1 = ( vec1 ) - ( vec0 );
				D3DXVECTOR3 edge2 = ( vec2 ) - ( vec0 );
				D3DXVECTOR3 vecNormal;

				// get the cross product and normalize it
				D3DXVec3Cross     ( &vecNormal, &edge1, &edge2 );
				D3DXVec3Normalize ( &vecNormal, &vecNormal );

				pNewMesh->m_ppFaces [ iFace ]->m_vecNormal.x = vecNormal.x;
				pNewMesh->m_ppFaces [ iFace ]->m_vecNormal.y = vecNormal.y;
				pNewMesh->m_ppFaces [ iFace ]->m_vecNormal.z = vecNormal.z;
			}

			vpMeshList.push_back ( pNewMesh );

			g_tree.SetOptions ( OptionsBSP );
			g_tree.SetStats   ( &StatsBSP  );

			for ( int iMesh = 0; iMesh < (int)vpMeshList.size ( ); iMesh++ )
			{
				cMesh* pMesh = vpMeshList [ iMesh ];

				if ( !pMesh )
					continue;

				pMesh->m_Bounds.Max.x = 10000.0f;
				pMesh->m_Bounds.Max.y = 10000.0f;
				pMesh->m_Bounds.Max.z = 10000.0f;

				pMesh->m_Bounds.Min.x = -10000.0f;
				pMesh->m_Bounds.Min.y = -10000.0f;
				pMesh->m_Bounds.Min.z = -10000.0f;
				
				g_tree.AddFaces ( pMesh->m_ppFaces, pMesh->m_dwFaceCount, true );
			}

			g_tree.CompileTree ( );

			g_portals.SetOptions ( OptionsPRT );
			g_portals.SetStats   ( &StatsPRT  );

			g_portals.Process ( &g_tree );

			// get the number of portals from the mesh
			pNode->iMeshPortalCount = GetPortalCount ( &g_tree, pMesh );
		
			// if we have some portals then create lists
			if ( pNode->iMeshPortalCount )
			{
				if ( ! CopyPortalVertices ( &g_tree, pNode, pMesh ) )
					return false;
			}
		}

		SAFE_DELETE ( pNewMesh );
	}

	return true;
}

bool cUniverse::CopyPortalVertices ( cBSPTree* pTree, sNode* pNode, sMesh* pMesh )
{
	// allocate memory for portals
	pNode->pMeshPortalList = new sPortal [ pNode->iMeshPortalCount ];

	// ensure memory was allocated
	SAFE_MEMORY ( pNode->pMeshPortalList );

	// array position for mesh vertices
	int iPosition = 0;

	// go through each face
	for ( int iPortal = 0; iPortal < pTree->GetPortalCount ( ); iPortal++ )
	{
		// get the face
		cBSPPortal* pPortal = pTree->GetPortal ( iPortal );

		// see if the portal is valid
		if ( !IsPortalValid ( pPortal, pMesh ) )
			continue;

		if ( !pPortal )
			continue;

		// copy vertices from the portal into the mesh list

		// ignore portals which do not have 4 vertices
		if ( pPortal->m_dwVertexCount != 4 )
			continue;

		CopyVertices ( &pNode->pMeshPortalList [ iPosition ].vertices [ 0 ], &pPortal->m_pVertices [ 0 ] );
		CopyVertices ( &pNode->pMeshPortalList [ iPosition ].vertices [ 1 ], &pPortal->m_pVertices [ 1 ] );
		CopyVertices ( &pNode->pMeshPortalList [ iPosition ].vertices [ 2 ], &pPortal->m_pVertices [ 2 ] );
		CopyVertices ( &pNode->pMeshPortalList [ iPosition ].vertices [ 3 ], &pPortal->m_pVertices [ 3 ] );

		for ( int i = 0; i < 4; i++ )
		{
			pNode->pMeshPortalList [ iPosition ].vertices [ i ].dwDiffuse = D3DCOLOR_ARGB ( 255, 255, 0, 0 );
		}

		// increment array position
		iPosition++;
	}

	// everything went okay
	return true;
}

void cUniverse::CopyVertices ( sPortalVertex* pTo, cVertex* pFrom )
{
	// copy vertices from one structure to another

	pTo->vecPosition.x = pFrom->x;
	pTo->vecPosition.y = pFrom->y;
	pTo->vecPosition.z = pFrom->z;
	pTo->vecNormal.x   = pFrom->Normal.x;
	pTo->vecNormal.y   = pFrom->Normal.y;
	pTo->vecNormal.z   = pFrom->Normal.z;
	pTo->tu            = pFrom->tu;
	pTo->tv            = pFrom->tv;
	pTo->dwDiffuse     = D3DCOLOR_ARGB ( 255, 0, 0, 255 );
}

int	cUniverse::GetPortalCount ( cBSPTree* pTree, sMesh* pMesh )
{
	// get the number of valid portals within a tree

	// used to store number of portals
	int iCount = 0;

	// go through each face
	for ( int iPortal = 0; iPortal < pTree->GetPortalCount ( ); iPortal++ )
	{
		// get the face
		cBSPPortal* pPortal = pTree->GetPortal ( iPortal );

		// see if the portal is valid
		//if ( !IsPortalValid ( pPortal, pMesh ) )
		//	continue;

		if ( pPortal->m_dwVertexCount != 4 )
			continue;
	
		// if it is then increment the portal counter
		iCount++;
	}

	// return the number of valid portals
	return iCount;
}

bool cUniverse::IsPortalValid ( cBSPPortal* pPortal, sMesh* pMesh )
{
	// determine if a portal is valid

	// check the face and mesh
	if ( !pPortal || !pMesh )
		return false;

	// go through each vertex
	for ( int iVertex = 0; iVertex < (int)pPortal->m_dwVertexCount; iVertex++ )
	{
		if ( pPortal->m_pVertices [ iVertex ].x > pMesh->Collision.vecMax.x + 1.0f )
			return false;

		if ( pPortal->m_pVertices [ iVertex ].y > pMesh->Collision.vecMax.y + 1.0f )
			return false;

		if ( pPortal->m_pVertices [ iVertex ].z > pMesh->Collision.vecMax.z + 1.0f )
			return false;

		if ( pPortal->m_pVertices [ iVertex ].x < pMesh->Collision.vecMin.x - 1.0f )
			return false;

		if ( pPortal->m_pVertices [ iVertex ].y < pMesh->Collision.vecMin.y - 1.0f )
			return false;

		if ( pPortal->m_pVertices [ iVertex ].z < pMesh->Collision.vecMin.z - 1.0f )
			return false;
	}

	// the portal must be valid so return true
	return true;
}

bool cUniverse::IsBoxWithinNode ( sNode* pNode, D3DXVECTOR3* pvecCenter, D3DXVECTOR3* pvecMin, D3DXVECTOR3* pvecMax, int *piSide )
{
	// epsilon increases size of submitted mesh (so touching meshes are part of node!)
	float epsilon = 1.0f;

	// left or right
	if ( pvecMax->x+epsilon <= GetTopLeftFront ( pNode ).x || pvecMin->x-epsilon >= GetTopRightBack ( pNode ).x )
		return false;

	// above or below
	if ( pvecMax->y+epsilon <= GetBottomLeftFront ( pNode ).y || pvecMin->y-epsilon >= GetTopLeftFront ( pNode ).y )
		return false;

	// behind or front
	if ( pvecMax->z+epsilon <= GetTopLeftBack ( pNode ).z || pvecMin->z-epsilon >= GetTopLeftFront ( pNode ).z )
		return false;

	// box does touch node - work out which side-of-node box is on
	if ( piSide )
	{
		// calculate node bound
		D3DXVECTOR3 vecNodeMin = pNode->vecCentre - pNode->vecDimension;
		D3DXVECTOR3 vecNodeMax = pNode->vecCentre + pNode->vecDimension;

		// no side
		*piSide = -1;

		// left or right
		if ( pvecCenter->y>vecNodeMin.y && pvecCenter->z>vecNodeMin.z )
		{
			if ( pvecCenter->x < pNode->vecCentre.x )
				*piSide = eLeft;
			else
				*piSide = eRight;
		}

		// bottom or top
		if ( pvecCenter->x>vecNodeMin.x && pvecCenter->z>vecNodeMin.z )
		{
			if ( pvecCenter->y < pNode->vecCentre.y )
				*piSide = eBottom;
			else
				*piSide = eTop;
		}

		// front or back
		if ( pvecCenter->x>vecNodeMin.x && pvecCenter->y>vecNodeMin.y )
		{
			if ( pvecCenter->z < pNode->vecCentre.z )
				*piSide = eFront;
			else
				*piSide = eBack;
		}
	}

	// within
	return true;
}

//020205-bool cUniverse::IsBoxPurelyWithinNode ( sNode* pNode, D3DXVECTOR3* pvecMin, D3DXVECTOR3* pvecMax )
bool cUniverse::IsBoxPurelyWithinNode ( sNode* pNode, D3DXVECTOR3* pvecCenter )
{
	// left or right
	if ( pvecCenter->x <= GetTopLeftFront ( pNode ).x || pvecCenter->x > GetTopRightBack ( pNode ).x )
		return false;

	// above or below
	if ( pvecCenter->y <= GetBottomLeftFront ( pNode ).y || pvecCenter->y > GetTopLeftFront ( pNode ).y )
		return false;

	// behind or front
	if ( pvecCenter->z <= GetTopLeftBack ( pNode ).z || pvecCenter->z > GetTopLeftFront ( pNode ).z )
		return false;

	// within
	return true;
}

bool cUniverse::IsPointWithinNode ( sNode* pNode, float x, float y, float z )
{
	float epsilon = 0.01f;

	// left or right
	if ( x + epsilon < GetTopLeftFront ( pNode ).x || x + epsilon > GetTopRightBack ( pNode ).x )
		return false;

	// above or below
	if ( y + epsilon < GetBottomLeftFront ( pNode ).y || y + epsilon > GetTopLeftFront ( pNode ).y )
		return false;

	// behind or in front of boundary
	if ( z + epsilon < GetTopLeftBack ( pNode ).z || z + epsilon > GetTopLeftFront ( pNode ).z )
		return false;

	return true;
}

bool cUniverse::SetPortalsOn ( void )
{
	m_bPortalsActivated = true;

	return true;
}

bool cUniverse::SetPortalsOff ( void )
{
	m_bPortalsActivated = false;

	return true;
}

void cUniverse::CreateNodesForScene ( bool bMode )
{
	// this function will do one of 2 tasks -
	//
	//		* count the number of nodes we need to create
	//		* or build the node list
	//
	// when "bMode" is true we count the nodes, when it's
	// false we build the node list

	int	iNodeDimension		= 50;										// node dimension
	int	iNodeSize			= 100;										// size of node
	int	iUniverseDimensionX = m_pNodeList->vecDimension.x;
	int	iUniverseDimensionY = m_pNodeList->vecDimension.y;
	int	iUniverseDimensionZ = m_pNodeList->vecDimension.z;
	int	x					= iUniverseDimensionX - iNodeDimension;		// initial x position
	int	y					= iUniverseDimensionY - iNodeDimension;		// initial y position
	int	z					= iUniverseDimensionZ - iNodeDimension;		// and initial z position
	int	iLoopX				= iUniverseDimensionX / iNodeDimension;		// how many times we need to loop around
	int	iLoopY				= iUniverseDimensionY / iNodeDimension;		// how many times we need to loop around
	int	iLoopZ				= iUniverseDimensionZ / iNodeDimension;		// how many times we need to loop around
	int	iOffset				= 0;

	// set the centre to 0, 0, 0
	m_vecNodeCentre = D3DXVECTOR3 ( 0.0f, 0.0f, 0.0f );
	m_iUniverseSizeX = iLoopX;
	m_iUniverseSizeY = iLoopY;
	m_iUniverseSizeZ = iLoopZ;
	m_iHalfUniverseSizeX = m_iUniverseSizeX/2;
	m_iHalfUniverseSizeY = m_iUniverseSizeY/2;
	m_iHalfUniverseSizeZ = m_iUniverseSizeZ/2;

	// see what we need to do
	if ( bMode )
	{
		// we're counting the nodes so allocate one node only
		m_iNodeListSize = 0;
		m_pNode         = new sNode;
		m_pNodeRef      = NULL;
	}
	else
	{
		// we will be building the node list so allocate
		// an array which will be large enough
		m_pNode			= new sNode [ m_iNodeListSize + 1 ];
		DWORD dwNodeRefSize = m_iUniverseSizeX * m_iUniverseSizeY * m_iUniverseSizeZ;
		m_pNodeRef      = new sNode* [ dwNodeRefSize ];
		memset ( m_pNodeRef, 0, sizeof(sNode*) * dwNodeRefSize );

		// also build a nodeusefag array based on size
		m_dwNodeUseFlagMapSize = dwNodeRefSize;
		m_pNodeUseFlagMap = new bool [ m_dwNodeUseFlagMapSize ];
	}

	// go through y
	for ( int iY = 0; iY < iLoopY; iY++ )
	{
		// and x
		for ( int iX = 0; iX < iLoopX; iX++ )
		{
			// and z axis
			for ( int iZ = 0; iZ < iLoopZ; iZ++ )
			{
				// set centre point and dimension
				m_pNode [ iOffset ].vecCentre    = D3DXVECTOR3 ( x, y, z );
				m_pNode [ iOffset ].vecDimension = D3DXVECTOR3 ( iNodeDimension, iNodeDimension, iNodeDimension );

				// node is currently valid
				bool bOk = true;

				// if node is out of x bounds then it's invalid
				if ( m_pNode [ iOffset ].vecCentre.x < m_vecNodeMin.x || m_pNode [ iOffset ].vecCentre.x > m_vecNodeMax.x )
					bOk = false;
				
				// if node is out of z bounds then it's invalid
				if ( m_pNode [ iOffset ].vecCentre.z < m_vecNodeMin.z || m_pNode [ iOffset ].vecCentre.z > m_vecNodeMax.z )
					bOk = false;
				
				// if node is out of y bounds then it's invalid
				if ( m_pNode [ iOffset ].vecCentre.y < m_vecNodeMin.y || m_pNode [ iOffset ].vecCentre.y > m_vecNodeMax.y )
					bOk = false;

				// grid reference in node universe
				DWORD dwNodeRefIndex = 0;
				int iRefX = m_iHalfUniverseSizeX+(x/100);
				int iRefY = m_iHalfUniverseSizeY+(y/100);
				int iRefZ = m_iHalfUniverseSizeZ+(z/100);
				if ( iRefX>=0 && iRefY>=0 && iRefZ>=0 )
				{
					dwNodeRefIndex = iRefX + (iRefY*m_iUniverseSizeX) + (iRefZ*(m_iUniverseSizeX*m_iUniverseSizeY));
				}

				// record pure index to map reference
				m_pNode [ iOffset ].dwNodeRefIndex = dwNodeRefIndex;

				// if node is valid then send to back of node list
				if ( bOk )
				{
					if ( bMode == false )
					{
						// add node centre to universe centre
						m_vecNodeCentre += m_pNode [ iOffset ].vecCentre;

						// store reference to node
						if ( m_pNodeRef ) m_pNodeRef [ dwNodeRefIndex ] = m_pNode+iOffset;

						iOffset++;
					}
					else
					{
						m_iNodeListSize++;
					}
				}

				// move the z by the size of the node
				z -= iNodeSize;
			}

			// reset z and move x
			z  = iUniverseDimensionZ - iNodeDimension;
			x -= iNodeSize;
		}

		// reset x and z and move y down
		z  = iUniverseDimensionZ - iNodeDimension;
		x  = iUniverseDimensionX - iNodeDimension;
		y -= iNodeSize;
	}

	// divide the centre of the universe by the number of
	// created nodes, we now have our true centre point
	m_vecNodeCentre /= iOffset;

	if ( bMode )
		SAFE_DELETE ( m_pNode );
}

void cUniverse::CalculateSolidityOfMeshes ( void )
{
	// go through all meshes in scene
	for ( int iFrame = 0; iFrame < (int)m_pMeshList.size ( ); iFrame++ )
	{
		// check each mesh for solidity
		sMesh* pMesh = m_pMeshList [ iFrame ];
		if ( pMesh->Collision.dwPortalBlocker==2 )
		{
			// definately fully solid according to input param
			pMesh->iSolidForVisibility = 2;
		}
		else
		{
			if ( pMesh->Collision.dwPortalBlocker==1 )
				pMesh->iSolidForVisibility = CheckIfMeshSolid ( pMesh, 10, 10, 10 );
			else
				pMesh->iSolidForVisibility = 0;
		}
	}
}

void cUniverse::GetBoundingBoxForAllMeshes ( void )
{
	// mike - 010903

	// initial bounds for universe
	m_vecNodeMax = D3DXVECTOR3 ( -1000000.0f, -1000000.0f, -1000000.0f );
	m_vecNodeMin = D3DXVECTOR3 (  1000000.0f,  1000000.0f,  1000000.0f );

	// go through all meshes in scene
	for ( int iFrame = 0; iFrame < (int)m_pMeshList.size ( ); iFrame++ )
	{
		// get mesh pointer
		sMesh* pMesh = m_pMeshList [ iFrame ];

		// update vec min if necessary
		if ( pMesh->Collision.vecMin.x < m_vecNodeMin.x ) m_vecNodeMin.x = pMesh->Collision.vecMin.x;
		if ( pMesh->Collision.vecMin.y < m_vecNodeMin.y ) m_vecNodeMin.y = pMesh->Collision.vecMin.y;
		if ( pMesh->Collision.vecMin.z < m_vecNodeMin.z ) m_vecNodeMin.z = pMesh->Collision.vecMin.z;

		// update vec max if necessary
		if ( pMesh->Collision.vecMax.x > m_vecNodeMax.x ) m_vecNodeMax.x = pMesh->Collision.vecMax.x;
		if ( pMesh->Collision.vecMax.y > m_vecNodeMax.y ) m_vecNodeMax.y = pMesh->Collision.vecMax.y;
		if ( pMesh->Collision.vecMax.z > m_vecNodeMax.z ) m_vecNodeMax.z = pMesh->Collision.vecMax.z;
	}

	// ensure minimum size for universe
	if ( m_vecNodeMin.x > - 100.0f ) m_vecNodeMin.x = -100.0f;
	if ( m_vecNodeMin.y >     0.0f ) m_vecNodeMin.y =    0.0f;
	if ( m_vecNodeMin.z > - 100.0f ) m_vecNodeMin.z = -100.0f;
	if ( m_vecNodeMax.x <   100.0f ) m_vecNodeMax.x =  100.0f;
	if ( m_vecNodeMax.y <   100.0f ) m_vecNodeMax.y =  100.0f;
	if ( m_vecNodeMax.z <   100.0f ) m_vecNodeMax.z =  100.0f;
}

void cUniverse::LinkMeshesToNodes ( void )
{
	// add meshes into created nodes
	for ( int iMesh = 0; iMesh < (int)m_pMeshList.size ( ); iMesh++ )
	{
		sMesh* pMesh = m_pMeshList [ iMesh ];
		for ( int iNode = 0; iNode < m_iNodeListSize; iNode++ )
		{
			// if mesh bound within any part of node
			if ( IsBoxWithinNode (	&m_pNode [ iNode ],
									&pMesh->Collision.vecCentre,
									&pMesh->Collision.vecMin,
									&pMesh->Collision.vecMax, NULL ) )
			{
				// add mesh to this node (mesh can be in several nodes)
				AddMeshToNode ( &m_pNode [ iNode ], pMesh );

				// only add mesh-col-data that are PURELY in the node (unlike addmeshtonode above)
//				020205 - collisionE being filled with neighboring meshes - only use own nodes meshes
//				if ( IsBoxPurelyWithinNode (	&m_pNode [ iNode ],
//												&pMesh->Collision.vecMin,
//												&pMesh->Collision.vecMax ) )
				if ( IsBoxPurelyWithinNode (	&m_pNode [ iNode ],
												&pMesh->Collision.vecCentre ) )
				{
					// check collision type of mesh
					if ( m_iMeshCollisionList [ iMesh ] == eBox )
					{
						// use reduced mesh - which stores the bound box created in BUILD
						CopyCollisionDataToNode ( &m_pNode [ iNode ], pMesh, m_pColMeshList [ iMesh ], pMesh->Collision.dwArbitaryValue );

						// leefix - 280104 - removed 1.01f as it was shifting the collision boxes slowly
						/* moved to BUILD function (reduced mesh carries it)
						sMesh* pBound = new sMesh;
						MakeMeshBox ( 	true,
										pBound,
										pMesh->Collision.vecMin.x * 1.0f,
										pMesh->Collision.vecMin.y * 1.0f,
										pMesh->Collision.vecMin.z * 1.0f,
										pMesh->Collision.vecMax.x * 1.0f,
										pMesh->Collision.vecMax.y * 1.0f,
										pMesh->Collision.vecMax.z * 1.0f,
										D3DFVF_XYZ,	0 );

						// if actual poly is less than box, use poly!
						if ( pMesh->dwIndexCount < pBound->dwIndexCount )
						{
							// use full polygon data for collision shape (HSR probably reduced it quite a lot)
							CopyCollisionDataToNode ( &m_pNode [ iNode ], pMesh, NULL, pMesh->Collision.dwArbitaryValue );
						}
						else
						{
							// use mesh box for collision shape
							CopyCollisionDataToNode ( &m_pNode [ iNode ], pBound, NULL, pMesh->Collision.dwArbitaryValue );
						}

						// free usages
						SAFE_DELETE ( pBound );
						*/
					}
					else if ( m_iMeshCollisionList [ iMesh ] == ePolygon )
					{
						// use full polygon data for collision shape
						CopyCollisionDataToNode ( &m_pNode [ iNode ], pMesh, NULL, pMesh->Collision.dwArbitaryValue );
					}
					else if ( m_iMeshCollisionList [ iMesh ] == eReducedPolygon )
					{
						// use reduced mesh from polygon data for collision shape
						CopyCollisionDataToNode ( &m_pNode [ iNode ], pMesh, m_pColMeshList [ iMesh ], pMesh->Collision.dwArbitaryValue );
					}
				}
			}
		}
	}
}

bool cUniverse::BuildPortals ( void )
{
	// set portal flag to true
	m_bPortalsCreated = true;

	// stage 1 - initial set up
	CalculateSolidityOfMeshes  ( );			// uses IsMeshSolid on all meshes in universe
	GetBoundingBoxForAllMeshes ( );			// find the bounding box of all meshes
	CreateNodesForScene        ( true  );	// find out how many nodes we need to create
	CreateNodesForScene        ( false );	// now create the nodes
	LinkMeshesToNodes          ( );			// link meshes to the newly created nodes
	FindNeighbouringNodes      ( );			// find neighbours for all nodes

	// stage 2 - building portals
	for ( int iNode = 0; iNode < m_iNodeListSize; iNode++ )
	{
		// create initial portals
		CreatePortalVertices ( &m_pNode [ iNode ] );
		
		// only need to work on portals if we have a mesh in node
		if ( m_pNode [ iNode ].iMeshCount )
			BuildVisibility	( &m_pNode [ iNode ] );
	}

	// stage 3 - build area boxes
	BuildAreaBoxes ( );

	// stage 4 - build area links
	BuildAreaLinks ( );

	// stage 5 - node meshes may exceed node itself
	CalculateNodeCollisionBoxes ();

	// return back to caller
	return true;
}

void cUniverse::FindNeighbouringNodes ( void )
{
	for ( int iA = 0; iA < m_iNodeListSize; iA++ )
	{
//		// exclude node that has no meshes
//		if ( !m_pNode [ iA ].ppMeshList )
//			continue;

		for ( int iB = 0; iB < m_iNodeListSize; iB++ )
		{
			if ( iA == iB )
				continue;

//			// exclude node that has no meshes
//			if ( !m_pNode [ iB ].ppMeshList )
//				continue;

			// right
			if ( m_pNode [ iA ].vecCentre.x + 100.0f == m_pNode [ iB ].vecCentre.x )
			{
				if ( m_pNode [ iA ].vecCentre.y == m_pNode [ iB ].vecCentre.y )
				{
					if ( m_pNode [ iA ].vecCentre.z == m_pNode [ iB ].vecCentre.z )
					{
						m_pNode [ iA ].pNeighbours [ 4 ] = &m_pNode [ iB ];
					}
				}
			}

			// left
			if ( m_pNode [ iA ].vecCentre.x - 100.0f == m_pNode [ iB ].vecCentre.x )
			{
				if ( m_pNode [ iA ].vecCentre.y == m_pNode [ iB ].vecCentre.y )
				{
					if ( m_pNode [ iA ].vecCentre.z == m_pNode [ iB ].vecCentre.z )
					{
						m_pNode [ iA ].pNeighbours [ 5 ] = &m_pNode [ iB ];
					}
				}
			}

			// back
			if ( m_pNode [ iA ].vecCentre.z - 100.0f == m_pNode [ iB ].vecCentre.z )
			{
				if ( m_pNode [ iA ].vecCentre.y == m_pNode [ iB ].vecCentre.y )
				{
					if ( m_pNode [ iA ].vecCentre.x == m_pNode [ iB ].vecCentre.x )
					{
						m_pNode [ iA ].pNeighbours [ 0 ] = &m_pNode [ iB ];
					}
				}
			}

			// front
			if ( m_pNode [ iA ].vecCentre.z + 100.0f == m_pNode [ iB ].vecCentre.z )
			{
				if ( m_pNode [ iA ].vecCentre.y == m_pNode [ iB ].vecCentre.y )
				{
					if ( m_pNode [ iA ].vecCentre.x == m_pNode [ iB ].vecCentre.x )
					{
						m_pNode [ iA ].pNeighbours [ 1 ] = &m_pNode [ iB ];
					}
				}
			}

			// bottom
			if ( m_pNode [ iA ].vecCentre.z == m_pNode [ iB ].vecCentre.z )
			{
				if ( m_pNode [ iA ].vecCentre.y - 100.0f == m_pNode [ iB ].vecCentre.y )
				{
					if ( m_pNode [ iA ].vecCentre.x == m_pNode [ iB ].vecCentre.x )
					{
						m_pNode [ iA ].pNeighbours [ 3 ] = &m_pNode [ iB ];
					}
				}
			}

			// top
			if ( m_pNode [ iA ].vecCentre.z == m_pNode [ iB ].vecCentre.z )
			{
				if ( m_pNode [ iA ].vecCentre.y + 100.0f == m_pNode [ iB ].vecCentre.y )
				{
					if ( m_pNode [ iA ].vecCentre.x == m_pNode [ iB ].vecCentre.x )
					{
						m_pNode [ iA ].pNeighbours [ 2 ] = &m_pNode [ iB ];
					}
				}
			}
		}
	}
}

bool cUniverse::CopyMeshPropertiesToNewMesh ( sMesh* pOriginalMesh, sMesh* pNewMesh )
{
	// copy mesh properties from one to another

	// check the parameters
	SAFE_MEMORY ( pOriginalMesh );
	SAFE_MEMORY ( pNewMesh );

	// copy mesh properties
	pNewMesh->bWireframe           = pOriginalMesh->bWireframe;
	pNewMesh->bLight               = pOriginalMesh->bLight;
	pNewMesh->bCull                = pOriginalMesh->bCull;
	pNewMesh->bFog                 = pOriginalMesh->bFog;
	pNewMesh->bAmbient             = pOriginalMesh->bAmbient;
	pNewMesh->bGhost               = pOriginalMesh->bGhost;
	pNewMesh->bTransparency        = pOriginalMesh->bTransparency;
	pNewMesh->iGhostMode           = pOriginalMesh->iGhostMode;
	pNewMesh->dwAlphaTestValue     = pOriginalMesh->dwAlphaTestValue;
	pNewMesh->bVisible             = pOriginalMesh->bVisible;
	pNewMesh->bZRead			   = pOriginalMesh->bZRead;
	pNewMesh->bZWrite			   = pOriginalMesh->bZWrite;
	pNewMesh->bZBiasActive		   = pOriginalMesh->bZBiasActive;
	pNewMesh->fZBiasSlopeScale	   = pOriginalMesh->fZBiasSlopeScale;
	pNewMesh->fZBiasDepth		   = pOriginalMesh->fZBiasDepth;
	pNewMesh->bUsesMaterial        = pOriginalMesh->bUsesMaterial;
	pNewMesh->dwTextureCount       = pOriginalMesh->dwTextureCount;
	pNewMesh->dwMeshID			   = pOriginalMesh->dwMeshID;

	// more mesh properties
	pNewMesh->Collision.dwArbitaryValue = pOriginalMesh->Collision.dwArbitaryValue;
	pNewMesh->Collision.dwPortalBlocker = pOriginalMesh->Collision.dwPortalBlocker;

	// copy a reference to the shader\effect
	CopyReferencesToShaderEffects ( pNewMesh, pOriginalMesh );

	// copy collsion and material information
	memcpy ( &pNewMesh->Collision, &pOriginalMesh->Collision, sizeof ( sCollisionData ) );

	// allocat texture array
	pNewMesh->pTextures = new sTexture [ pNewMesh->dwTextureCount ];

	// copy textures
	memcpy ( pNewMesh->pTextures, pOriginalMesh->pTextures, sizeof ( sTexture ) * pNewMesh->dwTextureCount );

	// maybe not the best way to do this?
	if ( strlen ( pNewMesh->pTextures [ 0 ].pName ) < 1 )
	{
		if ( pOriginalMesh->pMultiMaterial )
		{
			strcpy ( pNewMesh->pTextures->pName, pOriginalMesh->pMultiMaterial->pName );
			
			pNewMesh->pTextures->pTexturesRef = pOriginalMesh->pMultiMaterial->pTexturesRef;
		}
	}

	return true;
}

// 
// FINAL UNIVERSE BUILDING CODE
//

void cUniverse::FindAreaBoxHoldingMesh ( sMesh* pMesh, sArea** ppArea )
{
	// for each areabox
	for ( int iAreaBox = 0; iAreaBox < (int)m_pAreaList.size ( ); iAreaBox++ )
	{
		// get current area box ptr
		sArea* pArea = m_pAreaList [ iAreaBox ];

		// is mesh inside areabox
//		if ( pMesh->Collision.vecCentre.x > pArea->vecMin.x
//		&&   pMesh->Collision.vecCentre.x < pArea->vecMax.x
//		&&   pMesh->Collision.vecCentre.y > pArea->vecMin.y
//		&&   pMesh->Collision.vecCentre.y < pArea->vecMax.y
//		&&   pMesh->Collision.vecCentre.z > pArea->vecMin.z
//		&&   pMesh->Collision.vecCentre.z < pArea->vecMax.z )
// FPSCV108 - 290208 - meshes on edge can slip through gaps and not ger assigned to an areabox!!
		if ( pMesh->Collision.vecCentre.x >= pArea->vecMin.x
		&&   pMesh->Collision.vecCentre.x < pArea->vecMax.x
		&&   pMesh->Collision.vecCentre.y >= pArea->vecMin.y
		&&   pMesh->Collision.vecCentre.y < pArea->vecMax.y
		&&   pMesh->Collision.vecCentre.z >= pArea->vecMin.z
		&&   pMesh->Collision.vecCentre.z < pArea->vecMax.z )
		{
			// mesh inside this areabox
			*ppArea = pArea;
			return;
		}
	}

	// mesh outside all areaboxes
	*ppArea = NULL;
	return;
}

void cUniverse::BuildAreaBoxMeshGroups ( void )
{
    int iAreaBox = 0;
    
	// clear geometry lists for all areaboxes
	for ( iAreaBox = 0; iAreaBox < (int)m_pAreaList.size ( ); iAreaBox++ )
	{
		// get current area box ptr
		sArea* pArea = m_pAreaList [ iAreaBox ];
		pArea->meshgroups.clear ( );
		pArea->sharedmeshgroups.clear ( );
		pArea->meshgroupref.clear ( );
	}

	// clear flags in meshes (should only use a mesh ONCE)
	// Signature Modes (0-not dealt with / 1-added)
	for ( iAreaBox = 0; iAreaBox < (int)m_pAreaList.size ( ); iAreaBox++ )
	{
		sArea* pArea = m_pAreaList [ iAreaBox ];
		for ( int iNode = 0; iNode < (int)pArea->nodes.size ( ); iNode++ )
		{
			sNode* pNode = pArea->nodes [ iNode ];
			for ( int iMesh = 0; iMesh < (int)pNode->iMeshCount; iMesh++ )
			{
				sMesh* pMesh = pNode->ppMeshList [ iMesh ];
				pMesh->dwDrawSignature = 0;
			}
		}
	}

	// 0 : first for 'shared'
	// 1 : then for meshes 'within'
	// 2 : then for meshes not used (outside universe)
	for ( int iFirstShared=0; iFirstShared<3; iFirstShared++ )
	{
		// for each areabox
		for ( int iAreaBox = 0; iAreaBox < (int)m_pAreaList.size ( ); iAreaBox++ )
		{
			// get current area box ptr
			sArea* pArea = m_pAreaList [ iAreaBox ];
			if ( !pArea ) continue;

			// go through all nodes and build list of meshgroups
			for ( int iNode = 0; iNode < (int)pArea->nodes.size ( ); iNode++ )
			{
				// get a pointer to the node
				sNode* pNode = pArea->nodes [ iNode ];
				if ( !pNode ) continue;

				// create min max for node
				D3DXVECTOR3 vecNodeMin = pNode->vecCentre - pNode->vecDimension;
				D3DXVECTOR3 vecNodeMax = pNode->vecCentre + pNode->vecDimension;

				// get meshes from node (can be outside node for walls that border on the split)
				for ( int iMesh = 0; iMesh < pNode->iMeshCount; iMesh++ )
				{
					// get mesh from node
					sMesh* pMesh = pNode->ppMeshList [ iMesh ];
					if ( !pMesh ) continue;

					// mesh either inside or portal blocker
//					if ( pMesh->Collision.vecCentre.x >= pArea->vecMin.x
//					&&   pMesh->Collision.vecCentre.x < pArea->vecMax.x
//					&&   pMesh->Collision.vecCentre.y >= pArea->vecMin.y
//					&&   pMesh->Collision.vecCentre.y < pArea->vecMax.y
//					&&   pMesh->Collision.vecCentre.z >= pArea->vecMin.z
//					&&   pMesh->Collision.vecCentre.z < pArea->vecMax.z )
// FPSCV108 - 290208 - thin meshes exactly on north/west edges did not get shared - use BOUNDBOX against areabox, not center!
					if ( pMesh->Collision.vecMin.x > pArea->vecMin.x
					&&   pMesh->Collision.vecMax.x < pArea->vecMax.x
					&&   pMesh->Collision.vecMin.y > pArea->vecMin.y
					&&   pMesh->Collision.vecMax.y < pArea->vecMax.y
					&&   pMesh->Collision.vecMin.z > pArea->vecMin.z
					&&   pMesh->Collision.vecMax.z < pArea->vecMax.z )
					{
						// do non-shared meshes second
						if ( iFirstShared==1 )
						{
							// mesh not added in yet
							if ( pMesh->dwDrawSignature==0 )
							{
								// mesh is inside correct node
// FPSCV108 - 290208 - no reason to check against node, we are adding to the areabox once, and that's it
//								if ( pMesh->Collision.vecCentre.x >= vecNodeMin.x
//								&&   pMesh->Collision.vecCentre.x < vecNodeMax.x
//								&&   pMesh->Collision.vecCentre.y >= vecNodeMin.y
//								&&   pMesh->Collision.vecCentre.y < vecNodeMax.y
//								&&   pMesh->Collision.vecCentre.z >= vecNodeMin.z
//								&&   pMesh->Collision.vecCentre.z < vecNodeMax.z )
								{
									// WITHIN - entirely property of this node (within the areabox)
									AddMeshToAreaBox ( pArea, pMesh, 0 );
									pMesh->dwDrawSignature = 1;
								}
							}
						}
					}
					else
					{
						/* see below, i think i am right
						// do all shared meshes first
						if ( iFirstShared==0 )
						{
							// on edge, so must be blocker
							int iSharedMeshSideIndex = -1;
							if ( pNode->portals[0].pBlocker==pMesh ) iSharedMeshSideIndex = 0;
							if ( pNode->portals[1].pBlocker==pMesh ) iSharedMeshSideIndex = 1;
							if ( pNode->portals[2].pBlocker==pMesh ) iSharedMeshSideIndex = 2;
							if ( pNode->portals[3].pBlocker==pMesh ) iSharedMeshSideIndex = 3;
							if ( pNode->portals[4].pBlocker==pMesh ) iSharedMeshSideIndex = 4;
							if ( pNode->portals[5].pBlocker==pMesh ) iSharedMeshSideIndex = 5;

							// disregard if blocker side does not match contact against areabox
							if ( iSharedMeshSideIndex>=0 )
							{
								// check side this mesh blocking, and ignore it if not on correct side of areabox
								if ( iSharedMeshSideIndex==3 && pMesh->Collision.vecCentre.y>=pArea->vecMax.y ) iSharedMeshSideIndex = -1;
								if ( iSharedMeshSideIndex==2 && pMesh->Collision.vecCentre.y<=pArea->vecMax.y ) iSharedMeshSideIndex = -1;
							}

							// my service corridor corners are not blockers, but need drawing!
// 280904 - floor added to shared, even though only wall required for rendering!
//							iSharedMeshSideIndex = 6;//all on edge, share with this AB

							// 061204 - absolute should add a mesh that is bounbox overlapping the node!
							if ( iSharedMeshSideIndex==-1 )
							{
								// mesh bound box overlapping node - not on border with it!
								float fEpsilon = 0.1f;
								if ( pMesh->Collision.vecMax.x > vecNodeMin.x + fEpsilon
								&&   pMesh->Collision.vecMin.x < vecNodeMax.x - fEpsilon
								&&   pMesh->Collision.vecMax.y > vecNodeMin.y + fEpsilon
								&&   pMesh->Collision.vecMin.y < vecNodeMax.y - fEpsilon
								&&   pMesh->Collision.vecMax.z > vecNodeMin.z + fEpsilon
								&&   pMesh->Collision.vecMin.z < vecNodeMax.z - fEpsilon )
									iSharedMeshSideIndex = 6;
							}
							
							if ( iSharedMeshSideIndex>=0 )
							{
								// Find neighbor areaboxes so we can add mesh to its shard list
								sArea* pNeighborArea = NULL;
								FindAreaBoxHoldingMesh ( pMesh, &pNeighborArea );

								// if neighbor, associate mesh with that areabox
								if ( pNeighborArea )
								{
									// mesh not added in yet
									if ( pMesh->dwDrawSignature==0 )
									{
										// SHARED - add this mesh to neighboring areabox as a shared meshgroup
										sMeshGroup* pMeshGroupRef = AddMeshToAreaBox ( pArea, pMesh, pNeighborArea );
										pMesh->dwDrawSignature = 1;

										// record meshgroup which now resides in neighboring areabox
										pArea->meshgroupref.push_back ( pMeshGroupRef );
									}
									else
									{
										// find meshgroup in neighbor that holds this mesh
										sMeshGroup* pMeshGroupRef = FindMeshInAreaBox ( pArea, pMesh, pNeighborArea );
										if ( pMeshGroupRef==NULL )
										{
											// odd, as mesh was 'used' and placed in neighbor?
											int lee=42;
										}
										else
										{
											// this areabox ALSO needs to draw mesh, so draw the meshgroup that holds it 
											int iRef = 0;
											
											int iRefMax = pArea->meshgroupref.size();
											for (  iRef = 0; iRef < iRefMax; iRef++ )
												if ( pArea->meshgroupref [ iRef ]==pMeshGroupRef )
													break;

											// only if it is unique to ref list
											if ( iRef>=iRefMax )
												pArea->meshgroupref.push_back ( pMeshGroupRef );
										}
									}
								}
								else
								{
									// no neighbor, must be on edge of universe
									AddMeshToAreaBox ( pArea, pMesh, 0 );
									pMesh->dwDrawSignature = 1;
								}
							}
						}

						// 110804 - finally handle meshes outside ALL areaboxes
						if ( iFirstShared==2 )
						{
							// mesh not added in yet
							if ( pMesh->dwDrawSignature==0 )
							{
								// we know mesh is PART of node, so simply add it
								AddMeshToAreaBox ( pArea, pMesh, 0 );
								pMesh->dwDrawSignature = 1;
							}
						}
						// 110804 - finally handle meshes outside ALL areaboxes
						if ( iFirstShared==2 )
						{
							// mesh not added in yet
							if ( pMesh->dwDrawSignature==0 )
							{
								// we know mesh is PART of node, so simply add it
								AddMeshToAreaBox ( pArea, pMesh, 0 );
								pMesh->dwDrawSignature = 1;
							}
						}
						*/

						// on edge, so must be blocker
						int iSharedMeshSideIndex = -1;
						if ( pNode->portals[0].pBlocker==pMesh ) iSharedMeshSideIndex = 0;
						if ( pNode->portals[1].pBlocker==pMesh ) iSharedMeshSideIndex = 1;
						if ( pNode->portals[2].pBlocker==pMesh ) iSharedMeshSideIndex = 2;
						if ( pNode->portals[3].pBlocker==pMesh ) iSharedMeshSideIndex = 3;
						if ( pNode->portals[4].pBlocker==pMesh ) iSharedMeshSideIndex = 4;
						if ( pNode->portals[5].pBlocker==pMesh ) iSharedMeshSideIndex = 5;

						// disregard if blocker side does not match contact against areabox
						if ( iSharedMeshSideIndex>=0 )
						{
							// check side this mesh blocking, and ignore it if not on correct side of areabox
							if ( iSharedMeshSideIndex==3 && pMesh->Collision.vecCentre.y>=pArea->vecMax.y ) iSharedMeshSideIndex = -1;
							if ( iSharedMeshSideIndex==2 && pMesh->Collision.vecCentre.y<=pArea->vecMax.y ) iSharedMeshSideIndex = -1;
						}

						// 061204 - absolute should add a mesh that is bounbox overlapping the node!
						if ( iSharedMeshSideIndex==-1 )
						{
							// mesh bound box overlapping node - not on border with it!
							float fEpsilon = 0.1f;
							if ( pMesh->Collision.vecMax.x > vecNodeMin.x + fEpsilon
							&&   pMesh->Collision.vecMin.x < vecNodeMax.x - fEpsilon
							&&   pMesh->Collision.vecMax.y > vecNodeMin.y + fEpsilon
							&&   pMesh->Collision.vecMin.y < vecNodeMax.y - fEpsilon
							&&   pMesh->Collision.vecMax.z > vecNodeMin.z + fEpsilon
							&&   pMesh->Collision.vecMin.z < vecNodeMax.z - fEpsilon )
								iSharedMeshSideIndex = 6;
						}
						
						if ( iSharedMeshSideIndex>=0 )
						{
							// do all shared meshes first
							if ( iFirstShared==0 )
							{
								// Find neighbor areaboxes so we can add mesh to its shard list
								sArea* pNeighborArea = NULL;
								FindAreaBoxHoldingMesh ( pMesh, &pNeighborArea );

								// if neighbor, associate mesh with that areabox
								// FPSCV108 - 290208 - changes allow more shared meshes, but need to make sure areabox does not find itself as the neighbor!
								if ( pNeighborArea && pNeighborArea!=pArea )
								{
									// mesh not added in yet
									if ( pMesh->dwDrawSignature==0 )
									{
										// SHARED - add this mesh to neighboring areabox as a shared meshgroup
										sMeshGroup* pMeshGroupRef = AddMeshToAreaBox ( pArea, pMesh, pNeighborArea );
										pMesh->dwDrawSignature = 1;

										// record meshgroup which now resides in neighboring areabox
										pArea->meshgroupref.push_back ( pMeshGroupRef );
									}
									else
									{
										// find meshgroup in neighbor that holds this mesh
										sMeshGroup* pMeshGroupRef = FindMeshInAreaBox ( pArea, pMesh, pNeighborArea );
										if ( pMeshGroupRef==NULL )
										{
											// odd, as mesh was 'used' and placed in neighbor?
											int lee=42;
										}
										else
										{
											// this areabox ALSO needs to draw mesh, so draw the meshgroup that holds it 
											int iRef = 0;
											
											int iRefMax = pArea->meshgroupref.size();
											for (  iRef = 0; iRef < iRefMax; iRef++ )
												if ( pArea->meshgroupref [ iRef ]==pMeshGroupRef )
													break;

											// only if it is unique to ref list
											if ( iRef>=iRefMax )
												pArea->meshgroupref.push_back ( pMeshGroupRef );
										}
									}
								}
							}
							else
							{
								// final pass means all interior and shared meshes are spoken for
								if ( iFirstShared==2 )
								{
									// leeadd - 170105 - only if not used
									if ( pMesh->dwDrawSignature==0 )
									{
										// no neighbor AT ALL, must be on edge of universe
										AddMeshToAreaBox ( pArea, pMesh, 0 );
										pMesh->dwDrawSignature = 1;
									}
								}
							}
						}
					}
				}
			}
		}
	}

	// update vertex and index buffers
	CalculateNodeCollisionBoxes ();
	UploadMeshgroupsToBuffers ();
}

void cUniverse::UploadMeshgroupsToBuffers ( void )
{
	// add final meshgroups to vertex and index buffers
	for ( int iAreaBox = 0; iAreaBox < (int)m_pAreaList.size ( ); iAreaBox++ )
	{
	    int iIndex = 0;
	    
		// for each meshgroup
		for ( iIndex = 0; iIndex < (int)m_pAreaList [ iAreaBox ]->meshgroups.size ( ); iIndex++ )
		{
			sMesh* pMesh = m_pAreaList [ iAreaBox ]->meshgroups [ iIndex ]->pMesh;
			if ( pMesh ) m_ObjectManager.AddObjectMeshToBuffers ( pMesh, true );
		}

		// for shared meshgroup
		for ( iIndex = 0; iIndex < (int)m_pAreaList [ iAreaBox ]->sharedmeshgroups.size ( ); iIndex++ )
		{
			sMesh* pMesh = m_pAreaList [ iAreaBox ]->sharedmeshgroups [ iIndex ]->pMesh;
			if ( pMesh ) m_ObjectManager.AddObjectMeshToBuffers ( pMesh, true );
		}
	}
}

void cUniverse::CalculateNodeCollisionBoxes ( void )
{
	for ( int iObject = 0; iObject < m_iNodeListSize; iObject++ )
	{
		sNode* pCurrentNode = &(m_pNode [ iObject ]);
		if ( pCurrentNode )
		{
			// find bound box of collision data
			D3DXVECTOR3 vecMin = D3DXVECTOR3(999999,999999,999999);
			D3DXVECTOR3 vecMax = D3DXVECTOR3(-999999,-999999,-999999);
			for ( int iA = 0; iA < (int)pCurrentNode->collisionE.size ( ); iA++ )
			{
				for ( int iT=0; iT<3; iT++ )
				{
					// get raw ellipse-crushed geometry of collisionE
					D3DXVECTOR3 vecPos = pCurrentNode->collisionE [ iA ].triangle[iT].vecPosition;

					// uncrush it to get original vectors
					vecPos.x*=10.0f;
					vecPos.y*=30.0f;
					vecPos.z*=10.0f;

					// expand bound box
					if ( vecPos.x < vecMin.x ) vecMin.x=vecPos.x;
					if ( vecPos.y < vecMin.y ) vecMin.y=vecPos.y;
					if ( vecPos.z < vecMin.z ) vecMin.z=vecPos.z;
					if ( vecPos.x > vecMax.x ) vecMax.x=vecPos.x;
					if ( vecPos.y > vecMax.y ) vecMax.y=vecPos.y;
					if ( vecPos.z > vecMax.z ) vecMax.z=vecPos.z;
				}
			}
			pCurrentNode->vecColMin = vecMin;
			pCurrentNode->vecColMax = vecMax;
		}
	}
}

bool cUniverse::DoMeshTexturesMatch ( sMesh* pMeshA, sMesh* pMeshB )
{
	bool bTexturesMatchPerfectly = false;
	if ( pMeshA->dwTextureCount==pMeshB->dwTextureCount )
	{
		if ( pMeshA->pVertexShaderEffect==pMeshB->pVertexShaderEffect )
		{
			// get both texture array ptrs
			sTexture* pCurrentTex = pMeshA->pTextures;
			sTexture* pInputTex = pMeshB->pTextures;
			if ( pCurrentTex && pInputTex )
			{
				// same texture count and effect ptr
				bTexturesMatchPerfectly = true;

				// check all texture stage data for mismatch
				for ( int iStage=0; iStage<(int)pMeshB->dwTextureCount; iStage++ )
				{
					sTexture* pCurrentTexStage = &(pCurrentTex [ iStage ]);
					sTexture* pInputTexStage = &(pInputTex [ iStage ]);
//					if ( pCurrentTexStage->pTexturesRef != pInputTexStage->pTexturesRef) - was NULL for re-builds of UNIVERSE textures
//					if ( pCurrentTexStage->iImageID != pInputTexStage->iImageID )
					if ( stricmp ( pCurrentTexStage->pName, pInputTexStage->pName )!=NULL )
					{
						// data mismatch - textures not the same
						bTexturesMatchPerfectly = false;
					}
				}
			}
		}
	}

	// do these mesh textures match
	return bTexturesMatchPerfectly;
}

sMeshGroup* cUniverse::FindMeshInAreaBox ( sArea* pArea, sMesh* pInputMesh, sArea* pNeighborArea )
{
	// find meshgroup that matches mesh (good bet that it is where similar meshes are)
	if ( pNeighborArea==NULL )
	{
		for ( int iList = 0; iList < (int)pArea->meshgroups.size ( ); iList++ )
		{
			sMeshGroup* pMeshGroup = pArea->meshgroups [ iList ];
			if ( DoMeshTexturesMatch ( pMeshGroup->pMesh, pInputMesh ) )
				return pMeshGroup;
		}
	}
	else
	{
		for ( int iList = 0; iList < (int)pNeighborArea->sharedmeshgroups.size ( ); iList++ )
		{
			sMeshGroup* pMeshGroup = pNeighborArea->sharedmeshgroups [ iList ];
			if ( DoMeshTexturesMatch ( pMeshGroup->pMesh, pInputMesh ) )
				return pMeshGroup;
		}
	}
	return NULL;
}

sMeshGroup* cUniverse::AddMeshToAreaBox ( sArea* pArea, sMesh* pInputMesh, sArea* pNeighborArea )
{
	// find if mesh texture matches any in meshgroup list
	sMeshGroup* pMeshGroup = NULL;
	bool bMeshIsUniqueToMeshGroupList = true;
	if ( pNeighborArea==NULL )
	{
		for ( int iList = 0; iList < (int)pArea->meshgroups.size ( ); iList++ )
		{
			// get meshgroup from list
			pMeshGroup = pArea->meshgroups [ iList ];

			// only consider meshgroups I can fill
			if ( ((pMeshGroup->pMesh->dwVertexCount + pInputMesh->dwVertexCount)/4) >= (0xFFFF/6) )
				continue;

			// leefix - 050408 - V109 BETA2 - ensure 16bit index buffer not exceeded either
			if ( (pMeshGroup->pMesh->dwIndexCount + pInputMesh->dwIndexCount) >= 0x0000FFFF )
				continue;

			// leefix - 050408 - V109 BETA2 - ensure polygons would fit in a 512 lightmap
			if ( (pMeshGroup->pMesh->dwIndexCount + pInputMesh->dwIndexCount)/3 >= 1000 )
				continue;

			// does this texture match new mesh passed in
			if ( DoMeshTexturesMatch ( pMeshGroup->pMesh, pInputMesh ) )
			{
				// yes, can add this mesh to existing meshgroup
				bMeshIsUniqueToMeshGroupList = false;
				break;
			}
		}
	}
	else
	{
		// See if mesh unique or matches an existing meshgroup in neighbor areabox
		if ( pNeighborArea )
		{
			for ( int iList = 0; iList < (int)pNeighborArea->sharedmeshgroups.size ( ); iList++ )
			{
				// get texture from meshgroup list
				pMeshGroup = pNeighborArea->sharedmeshgroups [ iList ];

				// only consider meshgroups I can fill
				if ( ((pMeshGroup->pMesh->dwVertexCount + pInputMesh->dwVertexCount)/4) >= (0xFFFF/6) )
					continue;

				// leefix - 050408 - V109 BETA2 - ensure 16bit index buffer not exceeded either
				if ( (pMeshGroup->pMesh->dwIndexCount + pInputMesh->dwIndexCount) >= 0x0000FFFF )
					continue;

				// leefix - 050408 - V109 BETA2 - ensure polygons would fit in a 512 lightmap
				if ( (pMeshGroup->pMesh->dwIndexCount + pInputMesh->dwIndexCount)/3 >= 1000 )
					continue;

				// does this texture match new mesh passed in
				if ( DoMeshTexturesMatch ( pMeshGroup->pMesh, pInputMesh ) )
				{
					bMeshIsUniqueToMeshGroupList = false;
					break;
				}
			}
		}
	}

	// is mesh texture unique
	if ( bMeshIsUniqueToMeshGroupList==true )
	{
		// create new texture for list (new meshgroup)
		pMeshGroup = new sMeshGroup;
		pMeshGroup->pMesh = new sMesh;

		// fill new meshgroup with data from pMesh
		MakeMeshFromOtherMesh ( true, pMeshGroup->pMesh, pInputMesh, NULL );
		CopyMeshPropertiesToNewMesh ( pInputMesh, pMeshGroup->pMesh );

		// create texture array for new mesh
		pMeshGroup->pMesh->dwTextureCount = pInputMesh->dwTextureCount;
		if ( pMeshGroup->pMesh->dwTextureCount > 0 )
			pMeshGroup->pMesh->pTextures = new sTexture[pInputMesh->dwTextureCount];
		CloneInternalTextures ( pMeshGroup->pMesh, pInputMesh );
		CloneShaderEffects ( pMeshGroup->pMesh, pInputMesh );

		// add new meshgroup to list
		if ( pNeighborArea )
			pNeighborArea->sharedmeshgroups.push_back ( pMeshGroup );
		else
			pArea->meshgroups.push_back ( pMeshGroup );
	}
	else
	{
		// add mesh data only to existing meshgroup
		DWORD dwNewIndexCount = pMeshGroup->pMesh->dwIndexCount;
		DWORD dwNewVertexCount = pMeshGroup->pMesh->dwVertexCount;
		dwNewIndexCount += pInputMesh->dwIndexCount;
		dwNewVertexCount += pInputMesh->dwVertexCount;

		// OMLY if fit
		if ( (dwNewVertexCount/4) < (0xFFFF/6) )
		{
			// create new larger mesh (copy existing mesh data to larger mesh)
			sMesh* pLargerMesh = new sMesh;
			MakeMeshFromOtherMesh ( true, pLargerMesh, pMeshGroup->pMesh, NULL, dwNewIndexCount, dwNewVertexCount );
			CopyMeshPropertiesToNewMesh ( pInputMesh, pLargerMesh );

			// create texture array for new mesh
			pLargerMesh->dwTextureCount = pInputMesh->dwTextureCount;
			if ( pLargerMesh->dwTextureCount > 0 )
				pLargerMesh->pTextures = new sTexture[pInputMesh->dwTextureCount];
			CloneInternalTextures ( pLargerMesh, pInputMesh );
			CloneShaderEffects ( pLargerMesh, pInputMesh );

			// copy additional mesh data passed in to larger mesh
			CopyIndexMeshData ( pLargerMesh, pInputMesh, pMeshGroup->pMesh->dwIndexCount, pInputMesh->dwIndexCount );
			IncrementIndexMeshData ( pLargerMesh, pMeshGroup->pMesh->dwIndexCount, pInputMesh->dwIndexCount, pMeshGroup->pMesh->dwVertexCount );
			CopyVertexMeshDataSameFVF ( pLargerMesh, pInputMesh, pMeshGroup->pMesh->dwVertexCount, pInputMesh->dwVertexCount );

			// set new primitive counts (assume trilist)
			pLargerMesh->iDrawVertexCount = dwNewVertexCount;
			pLargerMesh->iDrawPrimitives  = dwNewIndexCount/3;

			// delete old meshgroup mesh
			SAFE_DELETE ( pMeshGroup->pMesh );

			// larger mesh is now the new meshgroup
			pMeshGroup->pMesh = pLargerMesh;
		}
		else
		{
			// should not get here, if does, means a single area filling up mesh with too many polygons!
			int lee=42;
		}
	}

	// update mesh of meshgroup (if any)
	if ( pMeshGroup ) pMeshGroup->pMesh->bMeshHasBeenReplaced = true;

	// return meshgroup used
	return pMeshGroup;
}

