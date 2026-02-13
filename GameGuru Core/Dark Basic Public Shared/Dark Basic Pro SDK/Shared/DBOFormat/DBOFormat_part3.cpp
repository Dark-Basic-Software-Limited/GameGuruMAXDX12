DARKSDK_DLL void UpdateObjectAnimation ( sObject* pObject )
{
	// handle vertex level animation (even if not animating)
	if ( pObject->pAnimationSet )
	{
		// only need to go for first frame for MDL models
		if ( !pObject->pAnimationSet->pAnimation->bBoneType )
		{
			// just animate the first mesh MDL
			AnimateBoneMesh ( pObject, pObject->ppFrameList [ 0 ] );
			pObject->fAnimLastFrame = pObject->fAnimFrame;
			pObject->fAnimSlerpLastTime = pObject->fAnimSlerpTime;
		}
	}

	// U74 - 120409 - not NECESSARILY animation data uses below (remove if ( pObject->pAnimationSet ))
	if ( 1 )
	{		
		// all meshes in object
		for ( int iFrame = 0; iFrame < pObject->iFrameCount; iFrame++ )
		{
			// get a pointer to the frame
			sFrame* pFrame = pObject->ppFrameList [ iFrame ];
			if ( pFrame==NULL ) continue;

			// determine if we need to animate
			bool bProceedToAnimate = false;
			if ( pObject->pAnimationSet && pObject->fAnimTotalFrames > 0.0f ) bProceedToAnimate = true;
			if ( pObject->bAnimUpdateOnce ) bProceedToAnimate = true;

			// 130513 - no need to animate limbs we have hidden (useful when hiding LOD meshes)
			if ( pFrame->pMesh )
				if ( pFrame->pMesh->bVisible==false )
					bProceedToAnimate = false;

			// if object is parent of instance, must animate ALL (main mesh and LOD versions)
			if ( pObject->position.bParentOfInstance )
			{
				if ( bProceedToAnimate )
				{
					// animate all meshes
					sMesh* pMesh = NULL;
					pMesh = pFrame->pMesh;
					if ( pMesh && pMesh->dwBoneCount > 0 ) AnimateBoneMesh ( pObject, pFrame, pMesh );
					pMesh = pFrame->pLOD[0];
					if ( pMesh && pMesh->dwBoneCount > 0 ) AnimateBoneMesh ( pObject, pFrame, pMesh );
					pMesh = pFrame->pLOD[1];
					if ( pMesh && pMesh->dwBoneCount > 0 ) AnimateBoneMesh ( pObject, pFrame, pMesh );
					pMesh = pFrame->pLODForQUAD;
					if ( pMesh && pMesh->dwBoneCount > 0 ) AnimateBoneMesh ( pObject, pFrame, pMesh );

					// Update regular bounds
					UpdateBoundBoxMesh ( pFrame );
					UpdateBoundSphereMesh ( pFrame );
				}
			}
			else
			{
				// choose mesh based on any LOD distance
				sMesh* pMesh = NULL;
				if ( pObject->iUsingWhichLOD==-1000 )
				{
					// uses built-in distance based LOD mesh selector (only animate LOD being used/visible)
					if ( pFrame->pMesh )
					{
						if ( pFrame->pMesh->bVisible==true )
						{
							pMesh = pFrame->pMesh;
						}
					}
				}
				else
				{
					if ( pObject->iUsingWhichLOD==0 )
					{
						// normal object-mesh rendering
						pMesh = pFrame->pMesh;
					}
					else
					{
						if ( pObject->iUsingWhichLOD==1 )
						{
							// LOD2
							pMesh = pFrame->pLOD[0];
						}
						else
						{
							if ( pObject->iUsingWhichLOD==2 )
							{
								// LOD3
								pMesh = pFrame->pLOD[1];
							}
							else
							{
								// LODQUAD
								pMesh = pFrame->pLODForQUAD;
							}
						}
					}
				}

				// for each mesh
				if ( pMesh )
				{
					// use mesh bone animation
					if ( bProceedToAnimate )
					{
						// anim can have matrix data and NO bones
						if ( pMesh->dwBoneCount > 0 )
						{
							// animate bones with matrix animation data
							AnimateBoneMesh ( pObject, pFrame, pMesh );

							// Update regular bounds
							UpdateBoundBoxMesh ( pFrame );
							UpdateBoundSphereMesh ( pFrame );
						}
					}
				}

				// also animate any old LOD mesh in transition
				sMesh* pOldLODMesh = NULL;
				if ( pObject->iUsingOldLOD==0 )
					pOldLODMesh = pFrame->pMesh;
				else
				{
					if ( pObject->iUsingOldLOD==1 )
						pOldLODMesh = pFrame->pLOD[0];
					else
					{
						if ( pObject->iUsingOldLOD==2 )
							pOldLODMesh = pFrame->pLOD[1];
						else
							if ( pObject->iUsingOldLOD==3 )
								pOldLODMesh = pFrame->pLODForQUAD;
					}
				}
				if ( pOldLODMesh )
					if ( pObject->fAnimTotalFrames > 0.0f || pObject->bAnimUpdateOnce )
						if ( pOldLODMesh->dwBoneCount > 0 )
							AnimateBoneMesh ( pObject, pFrame, pOldLODMesh );
			}
		}
		
		// reset value to here
		// pObject->bAnimUpdateOnce = false;	// U75 - 051209 - caused DarkPHYSICS ragdoll rope demo to fail!

		// Store frame for next quick reject check
		pObject->fAnimLastFrame = pObject->fAnimFrame;
		pObject->fAnimSlerpLastTime = pObject->fAnimSlerpTime;
	}
}

DARKSDK_DLL bool SetupShortVertex ( DWORD dwFVF, BYTE* pVertex, int iOffset, float x, float y, float z, float tu, float tv )
{
	// check the memory pointer is valid
	SAFE_MEMORY ( pVertex );

	// get the offset map
	sOffsetMap	offsetMap;
	GetFVFValueOffsetMap ( dwFVF, &offsetMap );

	// we can only work with any valid formats
	if ( dwFVF & GGFVF_XYZ )
	{
		*( ( float* ) pVertex + offsetMap.dwX       + ( offsetMap.dwSize * iOffset ) ) = x;
		*( ( float* ) pVertex + offsetMap.dwY       + ( offsetMap.dwSize * iOffset ) ) = y;
		*( ( float* ) pVertex + offsetMap.dwZ       + ( offsetMap.dwSize * iOffset ) ) = z;
	}
	if ( dwFVF & GGFVF_TEX1 )
	{
		*( ( float* ) pVertex + offsetMap.dwTU[0]      + ( offsetMap.dwSize * iOffset ) ) = tu;
		*( ( float* ) pVertex + offsetMap.dwTV[0]      + ( offsetMap.dwSize * iOffset ) ) = tv;
	}

	// okay
	return true;
}

DARKSDK_DLL bool SetupStandardVertex ( DWORD dwFVF, BYTE* pVertex, int iOffset, float x, float y, float z, float nx, float ny, float nz, DWORD dwDiffuseColour, float tu, float tv )
{
	// setup a standard lit vertex

	// check the memory pointer is valid
	SAFE_MEMORY ( pVertex );

	// get the offset map
	sOffsetMap	offsetMap;
	GetFVFValueOffsetMap ( dwFVF, &offsetMap );

	// we can only work with any valid formats
	if ( dwFVF & GGFVF_XYZ )
	{
		*( ( float* ) pVertex + offsetMap.dwX       + ( offsetMap.dwSize * iOffset ) ) = x;
		*( ( float* ) pVertex + offsetMap.dwY       + ( offsetMap.dwSize * iOffset ) ) = y;
		*( ( float* ) pVertex + offsetMap.dwZ       + ( offsetMap.dwSize * iOffset ) ) = z;
	}
	if ( dwFVF & GGFVF_NORMAL )
	{
		*( ( float* ) pVertex + offsetMap.dwNX      + ( offsetMap.dwSize * iOffset ) ) = nx;
		*( ( float* ) pVertex + offsetMap.dwNY      + ( offsetMap.dwSize * iOffset ) ) = ny;
		*( ( float* ) pVertex + offsetMap.dwNZ      + ( offsetMap.dwSize * iOffset ) ) = nz;
	}
	if ( dwFVF & GGFVF_DIFFUSE )
	{
		*( ( DWORD* ) pVertex + offsetMap.dwDiffuse + ( offsetMap.dwSize * iOffset ) ) = dwDiffuseColour;
	}
	if ( dwFVF & GGFVF_TEX1 )
	{
		*( ( float* ) pVertex + offsetMap.dwTU[0]      + ( offsetMap.dwSize * iOffset ) ) = tu;
		*( ( float* ) pVertex + offsetMap.dwTV[0]      + ( offsetMap.dwSize * iOffset ) ) = tv;
	}

	// mike - 160903 - point size support, set to 1.0f by default
	if ( dwFVF & GGFVF_PSIZE )
	{
		*( ( float* ) pVertex + offsetMap.dwPointSize + ( offsetMap.dwSize * iOffset ) ) = 1.0f;
	}

	// okay
	return true;
}

DARKSDK_DLL bool SetupStandardVertexDec ( sMesh* pMesh, BYTE* pVertex, int iOffset, float x, float y, float z, float nx, float ny, float nz, DWORD dwDiffuseColour, float tu, float tv )
{
	// setup a standard lit vertex

	// check the memory pointer is valid
	SAFE_MEMORY ( pVertex );

	// get the offset map
	sOffsetMap	offsetMap;
	GetFVFOffsetMap ( pMesh, &offsetMap );

	// we can only work with any valid formats
	if ( offsetMap.dwZ>0 )
	{
		*( ( float* ) pVertex + offsetMap.dwX       + ( offsetMap.dwSize * iOffset ) ) = x;
		*( ( float* ) pVertex + offsetMap.dwY       + ( offsetMap.dwSize * iOffset ) ) = y;
		*( ( float* ) pVertex + offsetMap.dwZ       + ( offsetMap.dwSize * iOffset ) ) = z;
	}
	if ( offsetMap.dwNZ>0 )
	{
		*( ( float* ) pVertex + offsetMap.dwNX      + ( offsetMap.dwSize * iOffset ) ) = nx;
		*( ( float* ) pVertex + offsetMap.dwNY      + ( offsetMap.dwSize * iOffset ) ) = ny;
		*( ( float* ) pVertex + offsetMap.dwNZ      + ( offsetMap.dwSize * iOffset ) ) = nz;
	}
	if ( offsetMap.dwDiffuse>0 )
	{
		*( ( DWORD* ) pVertex + offsetMap.dwDiffuse + ( offsetMap.dwSize * iOffset ) ) = dwDiffuseColour;
	}
	if ( offsetMap.dwTV[0]>0 )
	{
		*( ( float* ) pVertex + offsetMap.dwTU[0]      + ( offsetMap.dwSize * iOffset ) ) = tu;
		*( ( float* ) pVertex + offsetMap.dwTV[0]      + ( offsetMap.dwSize * iOffset ) ) = tv;
	}

	// mike - 160903 - point size support, set to 1.0f by default
	if ( offsetMap.dwPointSize>0 )
	{
		*( ( float* ) pVertex + offsetMap.dwPointSize + ( offsetMap.dwSize * iOffset ) ) = 1.0f;
	}

	// okay
	return true;
}

DARKSDK_DLL bool CreateFrameAndMeshList ( sObject* pObject )
{
	// 130213 - used to traverse 10,000 nests (no more) (avoids recursive stack overflow)
	int iNestCountMax = 9999;
	sFrame* pFrameBeforeNest[9999];
	sFrame* pThisFrame = NULL;
	int iNestCount = 0;

	// leeadd - 221105 - can make count negative efore obj call to protect mesh list
	if ( pObject->iMeshCount>=0 )
	{
		// find the number of meshes
		pObject->iMeshCount = 0;
		if ( 1 ) 
		{
			if ( !GetMeshCount ( pObject->pFrame, &pObject->iMeshCount ) )
				return false;
		}
		else
		{
			// does NOT produce same lists!!
			pThisFrame = pObject->pFrame;
			iNestCount = 0;
			while ( pThisFrame && iNestCount==0 )
			{	
				// act on frames
				if ( pThisFrame )
				{
					// count mesh
					if ( pThisFrame->pMesh ) pObject->iMeshCount++;

					// next item
					if ( pThisFrame->pChild )
					{
						pFrameBeforeNest[iNestCount] = pThisFrame;
						pThisFrame = pThisFrame->pChild;
						iNestCount++;
					}
					else
					{
						pThisFrame = pThisFrame->pSibling;
					}
				}
				if ( pThisFrame==NULL && iNestCount>0 )
				{
					iNestCount--;
					pThisFrame = pFrameBeforeNest[iNestCount];
					pThisFrame = pThisFrame->pSibling;
				}
			}
		}

		// allocate a list of frame which matches the number of meshes
		SAFE_DELETE_ARRAY ( pObject->ppMeshList );
		pObject->ppMeshList = new sMesh* [ pObject->iMeshCount ];
		SAFE_MEMORY ( pObject->ppMeshList );
	
		// get a list of frames which have meshes
		pObject->iMeshCount = 0;
		if ( 1 ) 
		{
			if ( !BuildMeshList ( pObject->ppMeshList, pObject->pFrame, &pObject->iMeshCount ) )
				return false;
		}
		else
		{
			// does NOT produce same lists!!
			pThisFrame = pObject->pFrame;
			iNestCount = 0;
			while ( pThisFrame && iNestCount==0 )
			{	
				// act on frames
				if ( pThisFrame )
				{
					// create mesh list entry
					if ( pThisFrame->pMesh )
					{
						pObject->ppMeshList [ pObject->iMeshCount ] = pThisFrame->pMesh;
						pThisFrame->iID = pObject->iMeshCount;
						pObject->iMeshCount++;
					}

					// next item
					if ( pThisFrame->pChild )
					{
						pFrameBeforeNest[iNestCount] = pThisFrame;
						pThisFrame = pThisFrame->pChild;
						iNestCount++;
					}
					else
					{
						pThisFrame = pThisFrame->pSibling;
					}
				}
				if ( pThisFrame==NULL && iNestCount>0 )
				{
					iNestCount--;
					pThisFrame = pFrameBeforeNest[iNestCount];
					pThisFrame = pThisFrame->pSibling;
				}
			}
		}
	}
	else
	{
		// negate count back - shows we have skipped this (kept old)
		pObject->iMeshCount *= -1;
	}

	// leeadd - 221105 - can make count negative efore obj call to protect frame list
	if ( pObject->iFrameCount>=0 )
	{
		pObject->iFrameCount = 0;
		if ( 1 ) 
		{
			if ( !GetFrameCount ( pObject->pFrame, &pObject->iFrameCount ) )
				return false;
		}
		else
		{
			// does NOT produce same lists!!
			pThisFrame = pObject->pFrame;
			iNestCount = 0;
			while ( pThisFrame && iNestCount==0 )
			{	
				// act on frames
				if ( pThisFrame )
				{
					// count frame
					pObject->iFrameCount++;

					// next item
					if ( pThisFrame->pChild )
					{
						pFrameBeforeNest[iNestCount] = pThisFrame;
						pThisFrame = pThisFrame->pChild;
						iNestCount++;
					}
					else
					{
						pThisFrame = pThisFrame->pSibling;
					}
				}
				if ( pThisFrame==NULL && iNestCount>0 )
				{
					iNestCount--;
					pThisFrame = pFrameBeforeNest[iNestCount];
					pThisFrame = pThisFrame->pSibling;
				}
			}
		}

		// allocate a list of frame which matches the number of frames
		SAFE_DELETE_ARRAY ( pObject->ppFrameList );
		pObject->ppFrameList = new sFrame* [ pObject->iFrameCount ];
		SAFE_MEMORY ( pObject->ppFrameList );

		// build up a list of frames
		pObject->iFrameCount = 0;
		if ( 1 )
		{
			if ( !BuildFrameList ( pObject->ppFrameList, pObject->pFrame, &pObject->iFrameCount ) )
				return false;
		}
		else
		{
			// does NOT produce same lists!!
			pThisFrame = pObject->pFrame;
			iNestCount = 0;
			while ( pThisFrame && iNestCount==0 )
			{	
				// act on frames
				if ( pThisFrame )
				{
					// create mesh list entry
					pObject->ppFrameList [ pObject->iFrameCount ] = pThisFrame;
					pThisFrame->iID = pObject->iFrameCount;
					pObject->iFrameCount++;

					// next item
					if ( pThisFrame->pChild )
					{
						pFrameBeforeNest[iNestCount] = pThisFrame;
						pThisFrame = pThisFrame->pChild;
						iNestCount++;
					}
					else
					{
						pThisFrame = pThisFrame->pSibling;
					}
				}
				if ( pThisFrame==NULL && iNestCount>0 )
				{
					iNestCount--;
					pThisFrame = pFrameBeforeNest[iNestCount];
					pThisFrame = pThisFrame->pSibling;
				}
			}
		}
	}
	else
	{
		// negate count back - shows we have skipped this (kept old)
		pObject->iFrameCount *= -1;
	}

	// if object has no meshes
	if ( pObject->iMeshCount==0 )
	{
		// do not attempt to draw it
		SAFE_DELETE ( pObject->ppMeshList );
		pObject->bNoMeshesInObject=true;
	}
	else
	{
		// default object does have meshes
		pObject->bNoMeshesInObject=false;
	}

	// okay
	return true;
}

DARKSDK_DLL bool SetupObjectsGenericProperties ( sObject* pObject )
{
	// check the object is valid
	SAFE_MEMORY ( pObject );
	SAFE_MEMORY ( pObject->pFrame );

	// create frame and mesh lists for object
	CreateFrameAndMeshList ( pObject );

	// calculate any user matrices from limb offset/rotate/scale data
	for ( int iFrame = 0; iFrame < pObject->iFrameCount; iFrame++ )
	{
		sFrame* pFrame = pObject->ppFrameList [ iFrame ];
		if ( pFrame ) UpdateUserMatrix(pFrame);
	}

	// reset hierarchy and calculate combined frame matrix data
	GGMATRIX matrix;
	GGMatrixIdentity ( &matrix );
	ResetFrameMatrices ( pObject->pFrame );
	UpdateFrame ( pObject->pFrame, &matrix );

	// success
	return true;
}

DARKSDK_DLL void ComputeBoundValues ( int iPass, GGVECTOR3 vecXYZ, GGVECTOR3* pvecMin, GGVECTOR3* pvecMax, GGVECTOR3* pvecCenter, float* pfRadius )
{
	// passes
	// leeadd - 080305 - added pass one as the animated boundbox needs bounds calc
	if ( iPass==0 || iPass==1 )
	{
		// check to see if the vertices are within the bounds and set the appriopriate values
		if ( vecXYZ.x < pvecMin->x ) pvecMin->x = vecXYZ.x;
		if ( vecXYZ.y < pvecMin->y ) pvecMin->y = vecXYZ.y;
		if ( vecXYZ.z < pvecMin->z ) pvecMin->z = vecXYZ.z;
		if ( vecXYZ.x > pvecMax->x ) pvecMax->x = vecXYZ.x;
		if ( vecXYZ.y > pvecMax->y ) pvecMax->y = vecXYZ.y;
		if ( vecXYZ.z > pvecMax->z ) pvecMax->z = vecXYZ.z;
	}
	if ( iPass==0 )
	{
		// add to the centre vector
		*pvecCenter += vecXYZ;
	}
	if ( iPass!=0 )
	{
		// relative to center of sphere
		vecXYZ = vecXYZ - *pvecCenter;

		// get the square length of the vector
		float fDistSq = GGVec3LengthSq ( &vecXYZ );

		// see if it's larger than the current radius
		if ( fDistSq > *pfRadius ) *pfRadius = fDistSq;
	}
}

DARKSDK_DLL bool CalculateMeshBounds ( sMesh* pMesh )
{
	// ensure that the pMesh is valid
	SAFE_MEMORY ( pMesh );

	// set the initial min and max vectors to defaults
	pMesh->Collision.vecMin = GGVECTOR3 (  1000000.0f,  1000000.0f,  1000000.0f );
	pMesh->Collision.vecMax = GGVECTOR3 ( -1000000.0f, -1000000.0f, -1000000.0f );

	// set the initial values of the bounding sphere
	pMesh->Collision.vecCentre = GGVECTOR3 ( 0.0f, 0.0f, 0.0f );
	pMesh->Collision.fRadius   = 0.0f;

	// get the offset map for the FVF
	sOffsetMap offsetMap;
	GetFVFOffsetMap ( pMesh, &offsetMap );

	// for each mesh go through all of the vertices
	for ( int iPass=0; iPass < 2; iPass++ )
	{
		for ( int iCurrentVertex = 0; iCurrentVertex < ( int ) pMesh->dwVertexCount; iCurrentVertex++ )
		{
			// make sure we have position data in the vertices
			if ( offsetMap.dwZ>0 )
			{
				// get the x, y and z components
				GGVECTOR3 vecXYZ;
				vecXYZ.x = *( ( float* ) pMesh->pVertexData + offsetMap.dwX + ( offsetMap.dwSize * iCurrentVertex ) );
				vecXYZ.y = *( ( float* ) pMesh->pVertexData + offsetMap.dwY + ( offsetMap.dwSize * iCurrentVertex ) );
				vecXYZ.z = *( ( float* ) pMesh->pVertexData + offsetMap.dwZ + ( offsetMap.dwSize * iCurrentVertex ) );

				// compute bound box, center and radius
				ComputeBoundValues ( iPass, vecXYZ, &pMesh->Collision.vecMin,
													&pMesh->Collision.vecMax,
													&pMesh->Collision.vecCentre,
													&pMesh->Collision.fRadius		);
			}
		}
		if ( iPass==0 )
		{
			// divide the centre radius by the number of vertices and we'll get our centre position
			pMesh->Collision.vecCentre = pMesh->Collision.vecMin + ((pMesh->Collision.vecMax - pMesh->Collision.vecMin)/2.0f); 
		}
		else
		{
			// set the final radius value which is the square root of the current radius
			pMesh->Collision.fRadius = ( float ) sqrt ( pMesh->Collision.fRadius );
		}
	}

	// okay
	return true;
}

DARKSDK_DLL bool CalculateObjectBounds ( int iPass, sMesh* pMesh, GGMATRIX* pMatrix, GGVECTOR3* pvecMin, GGVECTOR3* pvecMax, GGVECTOR3* pvecCenter, float* pfRadius )
{
	// ensure that the pMesh is valid
	SAFE_MEMORY ( pMesh );

	// get the offset map for the FVF
	sOffsetMap offsetMap;
	GetFVFOffsetMap ( pMesh, &offsetMap );

	// minimum data for quick mesh test - better system is calculate and store offsetmap in sMesh (fvf or declaration based)
	if ( pMesh->dwFVF==0 )
	{
		offsetMap.dwX = 0;
		offsetMap.dwY = 1;
		offsetMap.dwZ = 2;
		offsetMap.dwSize = pMesh->dwFVFSize/4;
	}

	// for each mesh go through all of the vertices
	for ( int iCurrentVertex = 0; iCurrentVertex < ( int ) pMesh->dwVertexCount; iCurrentVertex++ )
	{
		// make sure we have position data in the vertices
		if ( (offsetMap.dwZ>0) || pMesh->dwFVF==0 )
		{
			// get the x, y and z components
			GGVECTOR3 vecXYZ;
			vecXYZ.x = *( ( float* ) pMesh->pVertexData + offsetMap.dwX + ( offsetMap.dwSize * iCurrentVertex ) );
			vecXYZ.y = *( ( float* ) pMesh->pVertexData + offsetMap.dwY + ( offsetMap.dwSize * iCurrentVertex ) );
			vecXYZ.z = *( ( float* ) pMesh->pVertexData + offsetMap.dwZ + ( offsetMap.dwSize * iCurrentVertex ) );

			// transform the vector to world coords
			GGVec3TransformCoord( &vecXYZ, &vecXYZ, pMatrix );

			// compute bound box, center and radius
			ComputeBoundValues ( iPass, vecXYZ, pvecMin, pvecMax, pvecCenter, pfRadius );
		}
	}

	// okay
	return true;
}

DARKSDK_DLL bool CalculateAllBounds ( sObject* pObject, bool bNotUsed )
{
	// ensure that the object is valid
	SAFE_MEMORY ( pObject );

	// u74b7 - get the largest scale
	float fBiggestScale = std::max (
		pObject->position.vecScale.x, std::max (
			pObject->position.vecScale.y,
			pObject->position.vecScale.z)
		);

	// set the initial min and max vectors to defaults
	pObject->collision.vecMin = GGVECTOR3 (  1000000.0f,  1000000.0f,  1000000.0f );
	pObject->collision.vecMax = GGVECTOR3 ( -1000000.0f, -1000000.0f, -1000000.0f );

	// set the initial values of the bounding sphere
	GGVECTOR3 vecRealObjectCenter;
	pObject->collision.vecCentre = GGVECTOR3 ( 0.0f, 0.0f, 0.0f );
	pObject->collision.fRadius = 0.0f;

	// leeadd - 080305 - array to store bounboxes per frame
	if ( pObject->pAnimationSet )
	{
		// LB: error trap for crazy lengths 4294322343 (poss -1 as unsigned)
		// lee - 280306 - u6rc3 - can have animset tag but no frames data
		if (pObject->pAnimationSet->ulLength > 0 && pObject->pAnimationSet->ulLength < 4001110000)
		{
			// lee - 150206 - u60 - always have at least one slot for bounds vectors
			DWORD dwTotalFrames = pObject->pAnimationSet->ulLength;
			if ( dwTotalFrames==0 ) dwTotalFrames=1;

			// create dynamic boundbox arrays
			SAFE_DELETE(pObject->pAnimationSet->pvecBoundMin);
			pObject->pAnimationSet->pvecBoundMin = new GGVECTOR3 [ dwTotalFrames ];
			SAFE_DELETE(pObject->pAnimationSet->pvecBoundMax);
			pObject->pAnimationSet->pvecBoundMax = new GGVECTOR3 [ dwTotalFrames ];
			SAFE_DELETE(pObject->pAnimationSet->pvecBoundCenter);
			pObject->pAnimationSet->pvecBoundCenter = new GGVECTOR3 [ dwTotalFrames ];
			SAFE_DELETE(pObject->pAnimationSet->pfBoundRadius);
			pObject->pAnimationSet->pfBoundRadius = new float [ dwTotalFrames ];

			// reset boundboxes
			for ( int iKeyframe=0; iKeyframe<(int)dwTotalFrames; iKeyframe+=1 )
			{
				pObject->pAnimationSet->pvecBoundMin [ iKeyframe ] = GGVECTOR3 (  1000000.0f,  1000000.0f,  1000000.0f );
				pObject->pAnimationSet->pvecBoundMax [ iKeyframe ] = GGVECTOR3 ( -1000000.0f, -1000000.0f, -1000000.0f );
				pObject->pAnimationSet->pvecBoundCenter [ iKeyframe ] = GGVECTOR3 ( 0.0f, 0.0f, 0.0f );
				pObject->pAnimationSet->pfBoundRadius [ iKeyframe ] = 0.0f;
			}

			// U74 - 210409 - FPSCV115 - must animate before first pass or 'center' calc wrong (as animation can SCALE vertex data)
			if ( 1 )
			{
				// store anim flag
				bool bStoreAnimFlag = pObject->bAnimPlaying;
				float fStoreFrame = pObject->fAnimFrame;

				// perform animation processing of first keyframe (in case anim scales vertex data)
				for ( int iCurrentFrame = 0; iCurrentFrame < pObject->iFrameCount; iCurrentFrame++ )
				{
					// find frame within object
					sFrame* pFrame = pObject->ppFrameList [ iCurrentFrame ];
					if ( pFrame )
					{
						sMesh* pMesh = pFrame->pMesh;
						if ( pMesh )
						{
							pObject->bAnimPlaying=false;
							pObject->fAnimFrame=0.0f;
							GGMATRIX matrix;
							GGMatrixIdentity ( &matrix );
							UpdateAllFrameData ( pObject, pObject->fAnimFrame );
							UpdateFrame ( pObject->pFrame, &matrix );
							AnimateBoneMesh ( pObject, pFrame );
						}
					}
				}

				// restore animation flag
				pObject->bAnimPlaying = bStoreAnimFlag;
				pObject->fAnimFrame = fStoreFrame;
			}
		}
	}

	// bound vectir coutn var
	DWORD dwOverallObjBoundVectorCount = 0;

	// run through two passes (center calc, largest radius calc and radius calc)
	for ( int iMainPass = 0; iMainPass < 3; iMainPass++ )
	{
		// run through all of the frames within the object
		for ( int iCurrentFrame = 0; iCurrentFrame < pObject->iFrameCount; iCurrentFrame++ )
		{
			// find frame within object
			sFrame* pFrame = pObject->ppFrameList [ iCurrentFrame ];
			if ( pFrame )
			{
				sMesh* pMesh = pFrame->pMesh;
				if ( pMesh )
				{
					// calculate bounds for mesh (and animate on third pass)
					if ( iMainPass==0 ) CalculateMeshBounds ( pMesh );

					// world matrix (includes any scaling or orienting from frame data)
					// for example DarkMATTER models that have a 0.2 scaling at frame 1
					GGMATRIX WorldMatrix = pFrame->matCombined;

					// U74 - 210409 - FPSCV115 - animated vertex data earlier which applied scale so can use an identity
					// matrix here so the pFrameCombined does not interfere with vertex data MIN MAX boundbox
					// combined matrix required by many unanimated models so ODE physics can align box with
					// geometry so if animation is in effect, reset matrix, otherwise leave be, and ensure
					// BoneCount check so only vertexdata animated do not use frame combined but other animating
					// models such as chests which rotate/scale/translate can use combined for scale adjust
					if ( pObject->pAnimationSet )
					{
						if ( pObject->pAnimationSet->ulLength>0 )
							if ( pMesh->dwBoneCount>0 )
								GGMatrixIdentity ( &WorldMatrix );
					}
					else
					{
						// 241113 - discovered can have skinweight matrix on objects with no anim data but pBones data!
						// and will throw out proper position of object if not accounted for
						// 070917 - avoid corrupt data by also checking bone count
						if ( pMesh->pBones )
							if ( pMesh->dwBoneCount>0 )
								GGMatrixMultiply ( &WorldMatrix, &pMesh->pBones[0].matTranslation, &WorldMatrix );
					}

					// accumilate mesh data for object bounds calc
					if ( iMainPass==1 )
					{
						// object is animatable or static
						GGMATRIX matrix;
						if ( pObject->pAnimationSet )
						{
							// store anim flag
							bool bStoreAnimFlag = pObject->bAnimPlaying;
							float fStoreFrame = pObject->fAnimFrame;

							// calculate from all keyframes of animated mesh
							pObject->bAnimPlaying=true;
							int iLength = pObject->pAnimationSet->ulLength;
							sAnimation* pAnim = pObject->pAnimationSet->pAnimation;

							// keyframe ised to balance bounds of animation with speed code takes
							int iKeyStep = iLength/1000;

							// leefix - 170605 - this was added from a years old suggestion (animating all frames to get true bounds)
							// but it is slow (ie 13 seconds to load a character model in FPSC-V1), so do the quick version
							// BE AWARE that this will mean the keyframe-based bounbox data is not useful!
							if ( g_bFastBoundsCalculation==true )
								iKeyStep = iLength - 1;
							
							// go through all animations to get total bounds
							if ( iKeyStep<1 ) iKeyStep=1;
							for ( int iKeyframe=0; iKeyframe<iLength; iKeyframe+=iKeyStep )
							{
								// need combined for all non-bone-animating models
								matrix = pFrame->matCombined;
								if ( pObject->pAnimationSet->ulLength>0 )
									if ( pMesh->dwBoneCount>0 )
										GGMatrixIdentity ( &matrix );

								// set frame, animate and use mesh to get largest bounds
								pObject->fAnimFrame=(float)iKeyframe;
								UpdateAllFrameData ( pObject, pObject->fAnimFrame );
								UpdateFrame ( pObject->pFrame, &matrix );
								AnimateBoneMesh ( pObject, pFrame );

								// Calculate object bounds that animate
								if ( pObject->pAnimationSet )
								{
									// Calculate object bounds and store in bounds array
									CalculateObjectBounds ( iMainPass, pMesh, &matrix,
															&pObject->pAnimationSet->pvecBoundMin [ iKeyframe ], &pObject->pAnimationSet->pvecBoundMax [ iKeyframe ],
															&pObject->pAnimationSet->pvecBoundCenter [ iKeyframe ], &pObject->pAnimationSet->pfBoundRadius [ iKeyframe ] );

									// U74 - 210409 - FPSCV115 - store boundbox locally in case animationset bounds data not used
									if ( iKeyframe==0 )
									{
										pObject->collision.vecMin = pObject->pAnimationSet->pvecBoundMin [ iKeyframe ];
										pObject->collision.vecMax = pObject->pAnimationSet->pvecBoundMax [ iKeyframe ];
										pObject->collision.vecCentre = pObject->pAnimationSet->pvecBoundCenter [ iKeyframe ];
										pObject->collision.fRadius = pObject->pAnimationSet->pfBoundRadius [ iKeyframe ];
									}
								}
							}

							// restore mesh from animation parse
							pObject->bAnimPlaying=false;
							pObject->fAnimFrame=0.0f;
							GGMatrixIdentity ( &matrix );
							UpdateAllFrameData ( pObject, pObject->fAnimFrame );
							UpdateFrame ( pObject->pFrame, &matrix );
							AnimateBoneMesh ( pObject, pFrame );

							// restore animation flag
							pObject->bAnimPlaying = bStoreAnimFlag;
							pObject->fAnimFrame = fStoreFrame;
						}
						else
						{
							// update static model if has bones
							GGMatrixIdentity ( &matrix );
							UpdateAllFrameData ( pObject, pObject->fAnimFrame );
							UpdateFrame ( pObject->pFrame, &matrix );
							AnimateBoneMesh ( pObject, pFrame );

							// leefix - 280403 - calculate bound from non-animated mesh (ie cloned objects, etc)
							CalculateObjectBounds ( iMainPass, pMesh, &WorldMatrix,
													&pObject->collision.vecMin,	&pObject->collision.vecMax,
													&pObject->collision.vecCentre, &pObject->collision.fRadius	);
						}
					}
					else
					{
						// calculate from static mesh
						CalculateObjectBounds ( iMainPass, pMesh, &WorldMatrix,
												&pObject->collision.vecMin,	&pObject->collision.vecMax,
												&pObject->collision.vecCentre, &pObject->collision.fRadius	);
					}

					// increment bound vector count
					dwOverallObjBoundVectorCount+=pMesh->dwVertexCount;
				}
			}
		}
		if ( iMainPass==0 )
		{
			// finalise object center
			pObject->collision.vecCentre = pObject->collision.vecMin + ((pObject->collision.vecMax - pObject->collision.vecMin)/2.0f);
			vecRealObjectCenter = pObject->collision.vecCentre;
		}
		if ( iMainPass==1 )
		{
			// work out largest object radius from object bound box
			pObject->collision.fLargestRadius = ( float ) sqrt ( pObject->collision.fRadius );
			pObject->collision.vecCentre = vecRealObjectCenter;
			pObject->collision.fRadius = 0.0f;

			// lee - 140307 - I think this was added without a date, but it stops scaledradius from being set as it is zero by default
			// and this condition allows the computebounds to be called while keeping scaledradius at zero if it was zero (so the no-culling feature remains viable)
			// so to combat this, we set the fScaledLargestRadius field to one before we call the object create common functions
			// u74b7 - Use the scaled radius
			if ( pObject->collision.fScaledLargestRadius > 0.0f ) pObject->collision.fScaledLargestRadius = pObject->collision.fLargestRadius * fBiggestScale;

			// lee - 060406 - u6rc6 - shadow casters have a larger visual cull radius
			if ( pObject->bCastsAShadow==true )
			{
				// increase largest range to encompass possible shadow
				if ( pObject->collision.fScaledLargestRadius > 0.0f )
				{
					pObject->collision.fScaledLargestRadius = pObject->collision.fLargestRadius;
					pObject->collision.fScaledLargestRadius += 3000.0f;
				}
			}
		}
		if ( iMainPass==2 )
		{
			// work out final object radius from object bound box
			pObject->collision.fRadius = ( float ) sqrt ( pObject->collision.fRadius );
			// u74b7 - Use the scaled radius
			pObject->collision.fScaledRadius = pObject->collision.fRadius * fBiggestScale;
		}
	}

	// leefix - 140306 - u60b3 - of course by reducing the keyframe scan, the array is not filled
	if ( pObject->pAnimationSet )
	{
		// lee - 280306 - u6rc3 - can have animset tag but no frames data
		if ( pObject->pAnimationSet->ulLength>0 )
		{
			// fill the array elements that have not been filled
			for ( int iKeyframe=0; iKeyframe<(int)pObject->pAnimationSet->ulLength; iKeyframe++ )
			{
				if ( pObject->pAnimationSet->pfBoundRadius [ iKeyframe ]==0.0f )
				{
					// fill from master collision shape
					pObject->pAnimationSet->pvecBoundMin [ iKeyframe ] = pObject->collision.vecMin;
					pObject->pAnimationSet->pvecBoundMax [ iKeyframe ] = pObject->collision.vecMax;
					pObject->pAnimationSet->pfBoundRadius [ iKeyframe ] = pObject->collision.fRadius;
					pObject->pAnimationSet->pvecBoundCenter [ iKeyframe ] = pObject->collision.vecCentre;
				}
			}
		}
	}

	// leadd - 080305 - apply boundbox from current frame in animation
	if ( pObject->pAnimationSet )
	{
		// lee - 280306 - u6rc3 - can have animset tag but no frames data
		if ( pObject->pAnimationSet->ulLength>0 )
		{
			if ( pObject->bUpdateOnlyCurrentFrameBounds==false )
			{
				// work out center and radius
				for ( int iKeyframe=0; iKeyframe<(int)pObject->pAnimationSet->ulLength; iKeyframe+=1 )
				{
					pObject->pAnimationSet->pvecBoundCenter [ iKeyframe ] = pObject->pAnimationSet->pvecBoundMin [ iKeyframe ] + ((pObject->pAnimationSet->pvecBoundMax [ iKeyframe ] - pObject->pAnimationSet->pvecBoundMin [ iKeyframe ])/2.0f);
					GGVECTOR3 vecXYZ1 = pObject->pAnimationSet->pvecBoundMin [ iKeyframe ] - pObject->pAnimationSet->pvecBoundCenter [ iKeyframe ];
					GGVECTOR3 vecXYZ2 = pObject->pAnimationSet->pvecBoundMax [ iKeyframe ] - pObject->pAnimationSet->pvecBoundCenter [ iKeyframe ];
					float fDistSq1 = GGVec3Length ( &vecXYZ1 );
					float fDistSq2 = GGVec3Length ( &vecXYZ2 );
					if ( fDistSq2>fDistSq1 ) fDistSq1=fDistSq2;
					pObject->pAnimationSet->pfBoundRadius [ iKeyframe ] = fDistSq1;
				}

				// object bounds updated, and copied to first mesh collision bounds (for visual tweak)
				pObject->collision.vecMin = pObject->pAnimationSet->pvecBoundMin [ (int)pObject->fAnimFrame ];
				pObject->collision.vecMax = pObject->pAnimationSet->pvecBoundMax [ (int)pObject->fAnimFrame ];
				pObject->collision.vecCentre = pObject->pAnimationSet->pvecBoundCenter [ (int)pObject->fAnimFrame ];
				pObject->collision.fRadius = pObject->pAnimationSet->pfBoundRadius [ (int)pObject->fAnimFrame ];
				pObject->ppMeshList [ 0 ]->Collision.vecMin = pObject->collision.vecMin;
				pObject->ppMeshList [ 0 ]->Collision.vecMax = pObject->collision.vecMax;
				pObject->ppMeshList [ 0 ]->Collision.vecCentre = pObject->collision.vecCentre;
				pObject->ppMeshList [ 0 ]->Collision.fRadius = pObject->collision.fRadius;
			}
		}
	}

	// okay
	return true;
}

DARKSDK_DLL bool SetupMeshData ( sMesh* pMesh, DWORD dwVertexCount, DWORD dwIndexCount, bool bTempAllow32BitIndexBuffer )
{
	// if index size exceeds 16bit, cannot allow index buffer (except when temporarily allowing 32bit indices to copy in other mesh, then convert to vert only, done elsewhere)
	if ( bTempAllow32BitIndexBuffer == false )
		if ( dwIndexCount > 0 )
			if ( dwVertexCount > 0xFFFF ) //if ( dwIndexCount > 0x0000FFFF )
				return false;

	// ensure the mesh is valid
	SAFE_MEMORY ( pMesh );

	// ensure we free old data
	// 281114 - changed to SAFE_DELETE_ARRAY
	SAFE_DELETE_ARRAY(pMesh->pVertexData);
	SAFE_DELETE_ARRAY(pMesh->pIndices);

	// setup mesh properties
	pMesh->dwVertexCount	= dwVertexCount;									// vertex count assigned
	pMesh->pVertexData		= new BYTE [ pMesh->dwFVFSize * dwVertexCount ];	// allocate vertex memory

	// create new index mesh data
	if ( dwIndexCount>0 )
	{
		if ( bTempAllow32BitIndexBuffer == false )
		{
			pMesh->dwIndexCount		= dwIndexCount;
			pMesh->pIndices			= new WORD [ pMesh->dwIndexCount ];
		}
		else
		{
			pMesh->dwIndexCount		= dwIndexCount;
			pMesh->pIndices			= (WORD*)new DWORD [ pMesh->dwIndexCount ];
		}
	}
	else
	{
		pMesh->dwIndexCount		= 0;
		pMesh->pIndices			= NULL;
	}

	// check the memory was allocated correctly
	SAFE_MEMORY ( pMesh->pVertexData );

	// okay
	return true;
}

DARKSDK_DLL bool SetupMeshDeclarationData ( sMesh* pMesh, CONST GGVERTEXELEMENT* pDeclaration, DWORD dwVertexSize, DWORD dwVertexCount, DWORD dwIndexCount )
{
	LPGGVERTEXLAYOUT pNewVertexDec = NULL;	
	#ifdef DX11
	#else
	// create a new vertex declaration object
	if ( FAILED ( m_pD3D->CreateVertexDeclaration ( pDeclaration, &pNewVertexDec ) ) )
		return false;
	#endif

	// setup mesh properties
	pMesh->dwFVF			= 0;
	pMesh->dwFVFSize		= dwVertexSize;

	// store declaration for later reversal
	memcpy ( pMesh->pVertexDeclaration, pDeclaration, sizeof(pMesh->pVertexDeclaration) );

	// free any previous association with vertex dec handle
	/// 151015 - pVertexDec is created then copied as reference, then original released, leaving these (leak known from this!!)
	/// SAFE_RELEASE ( pMesh->pVertexDec );
	pMesh->pVertexDec = pNewVertexDec;

	// now setup the data
	if ( !SetupMeshData ( pMesh, dwVertexCount, dwIndexCount, false ) )
		return false;

	// complete
	return true;
}

DARKSDK_DLL bool SetupMeshFVFData ( sMesh* pMesh, DWORD dwFVF, DWORD dwVertexCount, DWORD dwIndexCount, bool bTempAllow32BitIndexBuffer )
{
	// set up mesh properties for the given FVF
	pMesh->dwFVF = dwFVF;
	sOffsetMap	offsetMap;
	GetFVFOffsetMap ( pMesh, &offsetMap );

	// setup mesh properties
	pMesh->dwFVFSize = offsetMap.dwByteSize;

	// now setup the data
	if ( !SetupMeshData ( pMesh, dwVertexCount, dwIndexCount, bTempAllow32BitIndexBuffer ) )
		return false;

	// complete
	return true;
}

DARKSDK_DLL bool SetupFrustum ( float fZDistance )
{
	// setup the planes for the viewing frustum
	// variable declarations
	GGMATRIX Matrix;
	float ZMin,	Q;
	GGMATRIX matProj;
	GGMATRIX matView;

	// check d3d is ok
	SAFE_MEMORY ( m_pD3D );
	
	// get the projection matrix
	GGGetTransform ( GGTS_PROJECTION, &matProj );
	
	// get the view matrix
	GGGetTransform ( GGTS_VIEW, &matView );

	if ( fZDistance != 0.0f )
	{
		// calculate new projection matrix based on distance provided
		ZMin        = -matProj._43 / matProj._33;
		Q           = fZDistance / ( fZDistance - ZMin );
		matProj._33 = Q;
		matProj._43 = -Q * ZMin;
	}

	// multiply with the projection matrix
	GGMatrixMultiply ( &Matrix, &matView, &matProj );

	// and now calculate the planes
	// near plane
	g_Planes [ 0 ][ 0 ].a = Matrix._14 + Matrix._13;
	g_Planes [ 0 ][ 0 ].b = Matrix._24 + Matrix._23;
	g_Planes [ 0 ][ 0 ].c = Matrix._34 + Matrix._33;
	g_Planes [ 0 ][ 0 ].d = Matrix._44 + Matrix._43;
	GGPlaneNormalize ( &g_Planes [ 0 ][ 0 ], &g_Planes [ 0 ][ 0 ] );

	// far plane
	g_Planes [ 0 ][ 1 ].a = Matrix._14 - Matrix._13;
	g_Planes [ 0 ][ 1 ].b = Matrix._24 - Matrix._23;
	g_Planes [ 0 ][ 1 ].c = Matrix._34 - Matrix._33;
	g_Planes [ 0 ][ 1 ].d = Matrix._44 - Matrix._43;
	GGPlaneNormalize ( &g_Planes [ 0 ][ 1 ], &g_Planes [ 0 ][ 1 ] );

	// left plane
	g_Planes [ 0 ][ 2 ].a = Matrix._14 + Matrix._11;
	g_Planes [ 0 ][ 2 ].b = Matrix._24 + Matrix._21;
	g_Planes [ 0 ][ 2 ].c = Matrix._34 + Matrix._31;
	g_Planes [ 0 ][ 2 ].d = Matrix._44 + Matrix._41;
	GGPlaneNormalize ( &g_Planes [ 0 ][ 2 ], &g_Planes [ 0 ][ 2 ] );

	// right plane
	g_Planes [ 0 ][ 3 ].a = Matrix._14 - Matrix._11;
	g_Planes [ 0 ][ 3 ].b = Matrix._24 - Matrix._21;
	g_Planes [ 0 ][ 3 ].c = Matrix._34 - Matrix._31;
	g_Planes [ 0 ][ 3 ].d = Matrix._44 - Matrix._41;
	GGPlaneNormalize ( &g_Planes [ 0 ][ 3 ], &g_Planes [ 0 ][ 3 ] );

	// top plane
	g_Planes [ 0 ][ 4 ].a = Matrix._14 - Matrix._12;
	g_Planes [ 0 ][ 4 ].b = Matrix._24 - Matrix._22;
	g_Planes [ 0 ][ 4 ].c = Matrix._34 - Matrix._32;
	g_Planes [ 0 ][ 4 ].d = Matrix._44 - Matrix._42;
	GGPlaneNormalize ( &g_Planes [ 0 ][ 4 ], &g_Planes [ 0 ][ 4 ] );

	// bottom plane
	g_Planes [ 0 ][ 5 ].a = Matrix._14 + Matrix._12;
	g_Planes [ 0 ][ 5 ].b = Matrix._24 + Matrix._22;
	g_Planes [ 0 ][ 5 ].c = Matrix._34 + Matrix._32;
	g_Planes [ 0 ][ 5 ].d = Matrix._44 + Matrix._42;
	GGPlaneNormalize ( &g_Planes [ 0 ][ 5 ], &g_Planes [ 0 ][ 5 ] );

	// complete
	return true;
}

DARKSDK_DLL bool SetupPortalFrustum ( DWORD dwFrustumCount, GGVECTOR3* pvecStart, GGVECTOR3* pvecA, GGVECTOR3* pvecB, GGVECTOR3* pvecC, GGVECTOR3* pvecD, bool bFrustrumZeroIsValid )
{
	// near plane
	GGPlaneFromPoints ( &g_Planes [ dwFrustumCount ][ 0 ], pvecA, pvecB, pvecC );
	g_PlaneVector [ dwFrustumCount ][ 0 ] = *pvecA;

	// far plane
	g_Planes [ dwFrustumCount ][ 1 ] = g_Planes [ 0 ][ 1 ];

	// left plane
	GGPlaneFromPoints ( &g_Planes [ dwFrustumCount ][ 2 ], pvecStart, pvecB, pvecC );
	g_PlaneVector [ dwFrustumCount ][ 2 ] = *pvecB;

	// right plane
	GGPlaneFromPoints ( &g_Planes [ dwFrustumCount ][ 3 ], pvecStart, pvecD, pvecA );
	g_PlaneVector [ dwFrustumCount ][ 3 ] = *pvecD;

	// top plane
	GGPlaneFromPoints ( &g_Planes [ dwFrustumCount ][ 4 ], pvecStart, pvecA, pvecB );
	g_PlaneVector [ dwFrustumCount ][ 4 ] = *pvecA;

	// bottom plane
	GGPlaneFromPoints ( &g_Planes [ dwFrustumCount ][ 5 ], pvecStart, pvecC, pvecD );
	g_PlaneVector [ dwFrustumCount ][ 5 ] = *pvecC;

	// make sure child is clipped by parent frustrum
	if ( (dwFrustumCount > 0 && bFrustrumZeroIsValid) || dwFrustumCount > 1 )
	{
		for ( int iP=0; iP<NUM_CULLPLANES; iP++ )
		{
			// not back plane
			if ( iP!=1 )
			{
				// if vector of child is outside parent plane, must clip child
				GGVECTOR3 vecChildVector = g_PlaneVector [ dwFrustumCount ][ iP ];
				float fChildOutside = GGPlaneDotCoord ( &g_Planes [ dwFrustumCount-1 ][ iP ], &vecChildVector );
				if ( (int)fChildOutside < 0.0f )
				{
					// child is outside parent frustrum at this plane, adjust child plane
					g_Planes [ dwFrustumCount ][ iP ] = g_Planes [ dwFrustumCount-1 ][ iP ];
				}
			}
		}
	}

	// complete
	return true;
}

DARKSDK_DLL bool SetupCastFrustum ( DWORD dwFrustumCount, GGVECTOR3* pvecStart, GGVECTOR3* pvecFinish )
{
	// useful to create a frustrum that immitates a ray cast for finding nodes from a line

	// complete
	return true;
}

DARKSDK_DLL bool CheckPoint ( float fX, float fY, float fZ )
{
	// make sure point is in frustum
	for ( int iPlaneIndex = 0; iPlaneIndex < NUM_CULLPLANES; iPlaneIndex++ ) 
	{
		if ( GGPlaneDotCoord ( &g_Planes [ 0 ][ iPlaneIndex ], &GGVECTOR3 ( fX, fY, fZ ) ) < 0.0f )
			return false;
	}
	return true;
}

DARKSDK_DLL bool CheckPoint ( GGPLANE* pPlanes, GGVECTOR3* pvecPoint )
{
	// is point in frustum?
//	for ( int iPlaneIndex = 0; iPlaneIndex < NUM_CULLPLANES; iPlaneIndex++ ) 
	for ( int iPlaneIndex = 2; iPlaneIndex < NUM_CULLPLANES; iPlaneIndex++ ) 
		if ( GGPlaneDotCoord ( &pPlanes [ iPlaneIndex ], pvecPoint ) < 0.0f )
			return false;

	// yes
	return true;
}

DARKSDK_DLL bool CheckCube ( float fX, float fY, float fZ, float fSize )
{
	// this does not always work when using very long ranges or small plane data
	for ( int iPlaneIndex = 0; iPlaneIndex < NUM_CULLPLANES; iPlaneIndex++ )
	{
		if ( GGPlaneDotCoord ( &g_Planes [ 0 ][ iPlaneIndex ], &GGVECTOR3 ( fX - fSize, fY - fSize, fZ - fSize ) ) >= 0.0f )
			continue;

		if ( GGPlaneDotCoord ( &g_Planes [ 0 ][ iPlaneIndex ], &GGVECTOR3 ( fX + fSize, fY - fSize, fZ - fSize ) ) >= 0.0f )
			continue;

		if ( GGPlaneDotCoord ( &g_Planes [ 0 ][ iPlaneIndex ], &GGVECTOR3 ( fX - fSize, fY + fSize, fZ - fSize ) ) >= 0.0f )
			continue;

		if ( GGPlaneDotCoord ( &g_Planes [ 0 ][ iPlaneIndex ], &GGVECTOR3 ( fX + fSize, fY + fSize, fZ - fSize ) ) >= 0.0f )
			continue;

		if ( GGPlaneDotCoord ( &g_Planes [ 0 ][ iPlaneIndex ], &GGVECTOR3 ( fX - fSize, fY - fSize, fZ + fSize ) ) >= 0.0f )
			continue;

		if ( GGPlaneDotCoord ( &g_Planes [ 0 ][ iPlaneIndex ], &GGVECTOR3 ( fX + fSize, fY - fSize, fZ + fSize ) ) >= 0.0f )
			continue;

		if ( GGPlaneDotCoord ( &g_Planes [ 0 ][ iPlaneIndex ], &GGVECTOR3 ( fX - fSize, fY + fSize, fZ + fSize ) ) >= 0.0f )
			continue;

		if ( GGPlaneDotCoord ( &g_Planes [ 0 ][ iPlaneIndex ], &GGVECTOR3 ( fX + fSize, fY + fSize, fZ + fSize ) ) >= 0.0f )
			continue;

		return false;
	}
	return true;
}

DARKSDK_DLL bool CheckRectangleEx ( DWORD iFrustumIndex, float fX, float fY, float fZ, float fXSize, float fYSize, float fZSize )
{
	bool bBoxFoundInsideOneOfTheFrustrums=false;
	bool bBoxOutsideFrustrum=false;
	for ( int iPlaneIndex = 0; iPlaneIndex < NUM_CULLPLANES; iPlaneIndex++ )
	{
		// if ALL eight points are outside the plane, it is not inside
		int iCountPointsOutsidePlane=0;

		// FPGC - 180909 - disregard near plane clip on frustrum zero checks (player camera can get nose right upto a portal)
		// if ( iFrustumIndex!=0 ) if ( GGPlaneDotCoord ( &g_Planes [ iFrustumIndex ][ iPlaneIndex ], &GGVECTOR3 ( fX - fXSize, fY - fYSize, fZ - fZSize ) ) < -0.01f ) iCountPointsOutsidePlane++;
		// 18113 - the above line would NEVER reject a boundbox for camera zero frustum (not intended behaviour!)
		if ( GGPlaneDotCoord ( &g_Planes [ iFrustumIndex ][ iPlaneIndex ], &GGVECTOR3 ( fX - fXSize, fY - fYSize, fZ - fZSize ) ) < -0.01f ) iCountPointsOutsidePlane++;
		if ( GGPlaneDotCoord ( &g_Planes [ iFrustumIndex ][ iPlaneIndex ], &GGVECTOR3 ( fX + fXSize, fY - fYSize, fZ - fZSize ) ) < -0.01f ) iCountPointsOutsidePlane++;
		if ( GGPlaneDotCoord ( &g_Planes [ iFrustumIndex ][ iPlaneIndex ], &GGVECTOR3 ( fX - fXSize, fY + fYSize, fZ - fZSize ) ) < -0.01f ) iCountPointsOutsidePlane++;
		if ( GGPlaneDotCoord ( &g_Planes [ iFrustumIndex ][ iPlaneIndex ], &GGVECTOR3 ( fX + fXSize, fY + fYSize, fZ - fZSize ) ) < -0.01f ) iCountPointsOutsidePlane++;
		if ( GGPlaneDotCoord ( &g_Planes [ iFrustumIndex ][ iPlaneIndex ], &GGVECTOR3 ( fX - fXSize, fY - fYSize, fZ + fZSize ) ) < -0.01f ) iCountPointsOutsidePlane++;
		if ( GGPlaneDotCoord ( &g_Planes [ iFrustumIndex ][ iPlaneIndex ], &GGVECTOR3 ( fX + fXSize, fY - fYSize, fZ + fZSize ) ) < -0.01f ) iCountPointsOutsidePlane++;
		if ( GGPlaneDotCoord ( &g_Planes [ iFrustumIndex ][ iPlaneIndex ], &GGVECTOR3 ( fX - fXSize, fY + fYSize, fZ + fZSize ) ) < -0.01f ) iCountPointsOutsidePlane++;
		if ( GGPlaneDotCoord ( &g_Planes [ iFrustumIndex ][ iPlaneIndex ], &GGVECTOR3 ( fX + fXSize, fY + fYSize, fZ + fZSize ) ) < -0.01f ) iCountPointsOutsidePlane++;
		if ( iCountPointsOutsidePlane==8 ) 
		{
			bBoxOutsideFrustrum=true;
			break;
		}
	}
	if ( bBoxOutsideFrustrum==false )
	{
		// box was inside one of the frustrums
		bBoxFoundInsideOneOfTheFrustrums=true;
	}

	// compare if box was outside ALL frustrums
	if ( bBoxFoundInsideOneOfTheFrustrums )
	{
		// at least one corner striding one of the frustrums
		return true;
	}
	else
	{
		// all corners outside all frustrums
		return false;
	}
}

DARKSDK_DLL bool CheckRectangle ( float fX, float fY, float fZ, float fXSize, float fYSize, float fZSize )
{
	return CheckRectangleEx ( 0, fX, fY, fZ, fXSize, fYSize, fZSize );
}

DARKSDK_DLL bool CheckSphere ( float fX, float fY, float fZ, float fRadius )
{
	// make sure radius is in frustum
	fRadius *= 1.25f; // leefix - 190307 - added an epsilon so object does not disappear too early
	for ( int iPlaneIndex = 0; iPlaneIndex < NUM_CULLPLANES; iPlaneIndex++ )
	{
		//if ( GGPlaneDotCoord ( &g_Planes [ 0 ][ iPlaneIndex ], &GGVECTOR3 ( fX, fY, fZ ) ) < -fRadius )
		//Dave Performance
		if ( g_Planes [ 0 ] [ iPlaneIndex ].a * fX + g_Planes [ 0 ] [ iPlaneIndex ].b * fY + g_Planes [ 0 ] [ iPlaneIndex ].c * fZ + g_Planes [ 0 ] [ iPlaneIndex ].d < -fRadius )
			return false;
	}

	return true;
}

DARKSDK_DLL bool CheckSphere ( DWORD dwFrustumMax, float fX, float fY, float fZ, float fRadius )
{
	// make sure radius is in frustum
	//for ( int iFrustumIndex = 0; iFrustumIndex <= (int)dwFrustumMax; iFrustumIndex++ )
	{	
		for ( int iPlaneIndex = 0; iPlaneIndex < NUM_CULLPLANES; iPlaneIndex++ )
		{
			//if ( GGPlaneDotCoord ( &g_Planes [ iFrustumIndex ][ iPlaneIndex ], &GGVECTOR3 ( fX, fY, fZ ) ) < -fRadius )
			//Dave Performance
			if ( g_Planes [ 0 ] [ iPlaneIndex ].a * fX + g_Planes [ 0 ] [ iPlaneIndex ].b * fY + g_Planes [ 0 ] [ iPlaneIndex ].c * fZ + g_Planes [ 0 ] [ iPlaneIndex ].d < -fRadius )
				return false;
		}
	}
	return true;
}

DARKSDK_DLL bool CheckPolygon ( GGPLANE* pPlanes, GGVECTOR3* pvec0, GGVECTOR3* pvec1, GGVECTOR3* pvec2 )
{
	// check all six planes of frustrum
	bool bBoxOutsideFrustrum=false;
	for ( int iPlaneIndex = 0; iPlaneIndex < NUM_CULLPLANES; iPlaneIndex++ )
	{
		// if polygon completely outside one of the planes, absolutely not in frustrum!
		int iCountPointsOutsidePlane=0;
		if ( GGPlaneDotCoord ( &pPlanes [ iPlaneIndex ], pvec0 ) < -0.01f ) iCountPointsOutsidePlane++;
		if ( GGPlaneDotCoord ( &pPlanes [ iPlaneIndex ], pvec1 ) < -0.01f ) iCountPointsOutsidePlane++;
		if ( GGPlaneDotCoord ( &pPlanes [ iPlaneIndex ], pvec2 ) < -0.01f ) iCountPointsOutsidePlane++;
		if ( iCountPointsOutsidePlane==3 ) 
		{
			bBoxOutsideFrustrum=true;
			break;
		}
	}
	if ( bBoxOutsideFrustrum )
		return false;
	else
		return true;
}

DARKSDK_DLL bool QuickSortArray ( int* array, int low, int high )
{
	// this function takes an array of any size
	// and will resort it into a correct order
	// e.g. sending the function a list of 4,2,3,1
	// will result in a list of 1,2,3,4
	// the low and high values are used to specify
	// the start and end point for the search

	if ( !array )
		return false;

	// initialize pointers
	int top    = low;
	int	bottom = high - 1;
	int part_index;
	int part_value;

	// do nothing if low >= high
	if ( low < high )
	{
		// check if elements are sequential
		if ( high == ( low + 1 ) )
		{
			if ( array [ low ] > array [ high ] )
				SwapInts ( array, high, low );
		}
		else
		{
			// choose a partition element and swap 
			// it with the last value in the array
			part_index = ( int ) ( ( low + high ) / 2 );
			part_value = array [ part_index ];

			SwapInts ( array, high, part_index );
		
			do
			{
				// increment the top pointer
				while ( ( array [ top ] <= part_value ) && ( top <= bottom ) )
					top++;
				
				// decrement the bottom pointer
				while ( ( array [ bottom ] > part_value ) && ( top <= bottom ) )
					bottom--;
				
				// swap elements if pointers have not met
				if ( top < bottom )
					SwapInts ( array, top, bottom );

			} while ( top < bottom );

			// put the partition element back where it belongs
			SwapInts ( array, top, high );

			// recursive calls
			QuickSortArray ( array, low,     top - 1 );
			QuickSortArray ( array, top + 1, high    );
		}
	}

	return true;
}

// swaps array n1 with array n2
DARKSDK_DLL bool SwapInts ( int* array, int n1, int n2 )
{
	if ( !array )
		return false;

	int temp;

	temp         = array [ n1 ];
	array [ n1 ] = array [ n2 ];
	array [ n2 ] = temp;

	return true;
}

DARKSDK_DLL bool GetFVFValueOffsetMap ( DWORD dwFVF, sOffsetMap* psOffsetMap )
{
	SAFE_MEMORY ( psOffsetMap );

	memset ( psOffsetMap, 0, sizeof ( sOffsetMap ) );

	int iOffset   = 0;
	int iPosition = 0;
	DWORD dwFVFSize = 0;

	if ( dwFVF & GGFVF_XYZ )
	{
		psOffsetMap->dwX         = iOffset + 0;
		psOffsetMap->dwY         = iOffset + 1;
		psOffsetMap->dwZ         = iOffset + 2;
		iOffset += 3;
	}

	if ( dwFVF & GGFVF_XYZRHW )
	{
		psOffsetMap->dwRWH = iOffset + 0;
		iOffset += 1;
	}

	if ( dwFVF & GGFVF_NORMAL )
	{
		psOffsetMap->dwNX        = iOffset + 0;
		psOffsetMap->dwNY        = iOffset + 1;
		psOffsetMap->dwNZ        = iOffset + 2;
		iOffset += 3;
	}

	if ( dwFVF & GGFVF_PSIZE )
	{
		psOffsetMap->dwPointSize = iOffset + 0;
		iOffset += 1;
	}

	if ( dwFVF & GGFVF_DIFFUSE )
	{
		psOffsetMap->dwDiffuse   = iOffset + 0;
		iOffset += 1;
	}

	if ( dwFVF & GGFVF_SPECULAR )
	{
		psOffsetMap->dwSpecular   = iOffset + 0;
		iOffset += 1;
	}

	DWORD dwTexCount = 0;
	if ( (dwFVF & GGFVF_TEXCOUNT_MASK) == GGFVF_TEX1 ) dwTexCount=1;
	if ( (dwFVF & GGFVF_TEXCOUNT_MASK) == GGFVF_TEX2 ) dwTexCount=2;
	if ( (dwFVF & GGFVF_TEXCOUNT_MASK) == GGFVF_TEX3 ) dwTexCount=3;
	if ( (dwFVF & GGFVF_TEXCOUNT_MASK) == GGFVF_TEX4 ) dwTexCount=4;
	if ( (dwFVF & GGFVF_TEXCOUNT_MASK) == GGFVF_TEX5 ) dwTexCount=5;
	if ( (dwFVF & GGFVF_TEXCOUNT_MASK) == GGFVF_TEX6 ) dwTexCount=6;
	if ( (dwFVF & GGFVF_TEXCOUNT_MASK) == GGFVF_TEX7 ) dwTexCount=7;
	if ( (dwFVF & GGFVF_TEXCOUNT_MASK) == GGFVF_TEX8 ) dwTexCount=8;
	for ( DWORD dwTexCoordSet=0; dwTexCoordSet<dwTexCount; dwTexCoordSet++ )
	{
		DWORD dwTexCoord = dwFVF & GGFVF_TEXCOORDSIZE1(dwTexCoordSet);
		if ( dwTexCoord==(DWORD)GGFVF_TEXCOORDSIZE1(dwTexCoordSet) )
		{
			psOffsetMap->dwTU[dwTexCoordSet] = iOffset + 0;
			iOffset += 1;
		}
		if ( dwTexCoord==(DWORD)GGFVF_TEXCOORDSIZE2(dwTexCoordSet) )
		{
			psOffsetMap->dwTU[dwTexCoordSet] = iOffset + 0;
			psOffsetMap->dwTV[dwTexCoordSet] = iOffset + 1;
			iOffset += 2;
		}
		if ( dwTexCoord==(DWORD)GGFVF_TEXCOORDSIZE3(dwTexCoordSet) )
		{
			psOffsetMap->dwTU[dwTexCoordSet] = iOffset + 0;
			psOffsetMap->dwTV[dwTexCoordSet] = iOffset + 1;
			psOffsetMap->dwTZ[dwTexCoordSet] = iOffset + 2;
			iOffset += 3;
		}
		if ( dwTexCoord==(DWORD)GGFVF_TEXCOORDSIZE4(dwTexCoordSet) )
		{
			psOffsetMap->dwTU[dwTexCoordSet] = iOffset + 0;
			psOffsetMap->dwTV[dwTexCoordSet] = iOffset + 1;
			psOffsetMap->dwTZ[dwTexCoordSet] = iOffset + 2;
			psOffsetMap->dwTW[dwTexCoordSet] = iOffset + 3;
			iOffset += 4;
		}
	}
	
	// calculate byte offset
	psOffsetMap->dwByteSize = sizeof ( DWORD ) * iOffset;

	// store number of offsets
	psOffsetMap->dwSize = iOffset;

	// check if matches byte size of actual FVF
	#ifdef DX11
	#else
	dwFVFSize = D3DXGetFVFVertexSize ( dwFVF );
	if ( dwFVFSize != psOffsetMap->dwByteSize )
	{
		// Offsets not being calculated correctly!
		return false;
	}
	#endif

	// complete
	return true;
}

DARKSDK_DLL bool GetFVFOffsetMap ( sMesh* pMesh, sOffsetMap* psOffsetMap )
{
	// clear to begin with
	memset ( psOffsetMap, 0, sizeof(sOffsetMap) );

	// FVF or declaration
	if ( pMesh->dwFVF==0 )
	{
		// Define end declaration token
		GGVERTEXELEMENT End = GGDECLEND;
		#ifdef DX11
		End.Stream = 255;
		#endif

		//PE: We get a exception here , pMesh->pVertexDeclaration[iElem].Stream has no 255 (end) entry.

		// Find Offsets
		for( int iElem=0; pMesh->pVertexDeclaration[iElem].Stream != End.Stream; iElem++ )
		{   
			if (iElem >= MAX_FVF_DECL_SIZE - 1) break; //PE: Make sure we dont crash.

			int iIndex = pMesh->pVertexDeclaration[iElem].UsageIndex;
			int iElementOffset = pMesh->pVertexDeclaration[iElem].Offset / sizeof(DWORD);
			if( pMesh->pVertexDeclaration[iElem].Usage == GGDECLUSAGE_POSITION )
			{
				psOffsetMap->dwX = iElementOffset + 0;
				psOffsetMap->dwY = iElementOffset + 1;
				psOffsetMap->dwZ = iElementOffset + 2;
			}
			if( pMesh->pVertexDeclaration[iElem].Usage == GGDECLUSAGE_POSITIONT )
			{
				psOffsetMap->dwX = iElementOffset + 0;
				psOffsetMap->dwY = iElementOffset + 1;
				psOffsetMap->dwZ = iElementOffset + 2;
				psOffsetMap->dwRWH = iElementOffset + 3;
			}
			if( pMesh->pVertexDeclaration[iElem].Usage == GGDECLUSAGE_PSIZE )
			{
				psOffsetMap->dwPointSize = iElementOffset + 0;
			}
			if( pMesh->pVertexDeclaration[iElem].Usage == GGDECLUSAGE_NORMAL )
			{
				psOffsetMap->dwNX = iElementOffset + 0;
				psOffsetMap->dwNY = iElementOffset + 1;
				psOffsetMap->dwNZ = iElementOffset + 2;
			}
			if( pMesh->pVertexDeclaration[iElem].Usage == GGDECLUSAGE_COLOR && iIndex==0 )
			{
				psOffsetMap->dwDiffuse = iElementOffset + 0;
			}
			if( pMesh->pVertexDeclaration[iElem].Usage == GGDECLUSAGE_COLOR && iIndex==1 )
			{
				psOffsetMap->dwSpecular = iElementOffset + 0;
			}
			if( pMesh->pVertexDeclaration[iElem].Usage == GGDECLUSAGE_TEXCOORD )
			{
				psOffsetMap->dwTU[iIndex] = iElementOffset + 0;
				if ( pMesh->pVertexDeclaration[iElem].Type==GGDECLTYPE_FLOAT2)
				{
					psOffsetMap->dwTV[iIndex] = iElementOffset + 1;
				}
				if ( pMesh->pVertexDeclaration[iElem].Type==GGDECLTYPE_FLOAT3)
				{
					psOffsetMap->dwTV[iIndex] = iElementOffset + 1;
					psOffsetMap->dwTZ[iIndex] = iElementOffset + 2;
				}
				if ( pMesh->pVertexDeclaration[iElem].Type==GGDECLTYPE_FLOAT4)
				{
					psOffsetMap->dwTV[iIndex] = iElementOffset + 1;
					psOffsetMap->dwTZ[iIndex] = iElementOffset + 2;
					psOffsetMap->dwTW[iIndex] = iElementOffset + 3;
				}
			}
			if( pMesh->pVertexDeclaration[iElem].Usage == GGDECLUSAGE_TANGENT )
			{
				psOffsetMap->dwTU[1] = iElementOffset + 0;
				psOffsetMap->dwTV[1] = iElementOffset + 1;
				psOffsetMap->dwTZ[1] = iElementOffset + 2;
				psOffsetMap->dwTW[1] = iElementOffset + 3;
			}
			if( pMesh->pVertexDeclaration[iElem].Usage == GGDECLUSAGE_BINORMAL )
			{
				psOffsetMap->dwTU[2] = iElementOffset + 0;
				psOffsetMap->dwTV[2] = iElementOffset + 1;
				psOffsetMap->dwTZ[2] = iElementOffset + 2;
				psOffsetMap->dwTW[2] = iElementOffset + 3;
			}
		}

		// calculate byte offset
		psOffsetMap->dwByteSize = pMesh->dwFVFSize;

		// store number of offsets
		psOffsetMap->dwSize = pMesh->dwFVFSize/sizeof(DWORD);

		// complete
		return true;
	}
	else
	{
		return GetFVFValueOffsetMap ( pMesh->dwFVF, psOffsetMap );
	}
}

DARKSDK_DLL bool GetFVFOffsetMapFixedForBones ( sMesh* pMesh, sOffsetMap* psOffsetMap )
{
	// The above "GetFVFOffsetMap" function overwrites the Binormals and Tangents offsets
	// when bone indices and weights are also in the DBO vertex declaration (ouch!)
	// this one is created for use by Wicked to read in all data properly (without messing with legacy stuff)
	// we deliberately place BONEINDICES in TUVZW[3] and BONEWEIGHTS in TUVZW[4]
	int iBoneDataOffset = 3;

	// clear to begin with
	memset ( psOffsetMap, 0, sizeof(sOffsetMap) );

	// FVF or declaration
	if ( pMesh->dwFVF==0 )
	{
		// Define end declaration token
		GGVERTEXELEMENT End = GGDECLEND;
		#ifdef DX11
		End.Stream = 255;
		#endif

		// Find Offsets
		for( int iElem=0; pMesh->pVertexDeclaration[iElem].Stream != End.Stream; iElem++ )
		{   
			if (iElem >= MAX_FVF_DECL_SIZE - 1) break;
			int iIndex = pMesh->pVertexDeclaration[iElem].UsageIndex;
			int iElementOffset = pMesh->pVertexDeclaration[iElem].Offset / sizeof(DWORD);
			if( pMesh->pVertexDeclaration[iElem].Usage == GGDECLUSAGE_POSITION )
			{
				psOffsetMap->dwX = iElementOffset + 0;
				psOffsetMap->dwY = iElementOffset + 1;
				psOffsetMap->dwZ = iElementOffset + 2;
			}
			if( pMesh->pVertexDeclaration[iElem].Usage == GGDECLUSAGE_POSITIONT )
			{
				psOffsetMap->dwX = iElementOffset + 0;
				psOffsetMap->dwY = iElementOffset + 1;
				psOffsetMap->dwZ = iElementOffset + 2;
				psOffsetMap->dwRWH = iElementOffset + 3;
			}
			if( pMesh->pVertexDeclaration[iElem].Usage == GGDECLUSAGE_PSIZE )
			{
				psOffsetMap->dwPointSize = iElementOffset + 0;
			}
			if( pMesh->pVertexDeclaration[iElem].Usage == GGDECLUSAGE_NORMAL )
			{
				psOffsetMap->dwNX = iElementOffset + 0;
				psOffsetMap->dwNY = iElementOffset + 1;
				psOffsetMap->dwNZ = iElementOffset + 2;
			}
			if( pMesh->pVertexDeclaration[iElem].Usage == GGDECLUSAGE_COLOR && iIndex==0 )
			{
				psOffsetMap->dwDiffuse = iElementOffset + 0;
			}
			if( pMesh->pVertexDeclaration[iElem].Usage == GGDECLUSAGE_COLOR && iIndex==1 )
			{
				psOffsetMap->dwSpecular = iElementOffset + 0;
			}
			if( pMesh->pVertexDeclaration[iElem].Usage == GGDECLUSAGE_TEXCOORD )
			{
				if (pMesh->pVertexDeclaration[iElem].Type == GGDECLTYPE_FLOAT4)
				{
					// bone data comes in from this one, so expect INDICES then WEIGHTS
					if (iBoneDataOffset == 3 || iBoneDataOffset == 4)
					{
						psOffsetMap->dwTU[iBoneDataOffset] = iElementOffset + 0;
						psOffsetMap->dwTV[iBoneDataOffset] = iElementOffset + 1;
						psOffsetMap->dwTZ[iBoneDataOffset] = iElementOffset + 2;
						psOffsetMap->dwTW[iBoneDataOffset] = iElementOffset + 3;
						iBoneDataOffset++;
					}
				}
				else
				{
					psOffsetMap->dwTU[iIndex] = iElementOffset + 0;
					if (pMesh->pVertexDeclaration[iElem].Type == GGDECLTYPE_FLOAT2)
					{
						psOffsetMap->dwTV[iIndex] = iElementOffset + 1;
					}
					if (pMesh->pVertexDeclaration[iElem].Type == GGDECLTYPE_FLOAT3)
					{
						psOffsetMap->dwTV[iIndex] = iElementOffset + 1;
						psOffsetMap->dwTZ[iIndex] = iElementOffset + 2;
					}
				}
			}
			if( pMesh->pVertexDeclaration[iElem].Usage == GGDECLUSAGE_TANGENT )
			{
				psOffsetMap->dwTU[1] = iElementOffset + 0;
				psOffsetMap->dwTV[1] = iElementOffset + 1;
				psOffsetMap->dwTZ[1] = iElementOffset + 2;
				psOffsetMap->dwTW[1] = iElementOffset + 3;
			}
			if( pMesh->pVertexDeclaration[iElem].Usage == GGDECLUSAGE_BINORMAL )
			{
				psOffsetMap->dwTU[2] = iElementOffset + 0;
				psOffsetMap->dwTV[2] = iElementOffset + 1;
				psOffsetMap->dwTZ[2] = iElementOffset + 2;
				psOffsetMap->dwTW[2] = iElementOffset + 3;
			}
		}

		// calculate byte offset
		psOffsetMap->dwByteSize = pMesh->dwFVFSize;

		// store number of offsets
		psOffsetMap->dwSize = pMesh->dwFVFSize/sizeof(DWORD);

		// complete
		return true;
	}
	else
	{
		return GetFVFValueOffsetMap ( pMesh->dwFVF, psOffsetMap );
	}
}

DARKSDK_DLL bool CreateVertexShaderFromFVF ( DWORD dwFVF, DWORD* pdwShader )
{
	// create the vertex shader handle

	// check all of the memory
	SAFE_MEMORY ( m_pD3D );

	// all okay
	return true;
}

DARKSDK_DLL void DBOCalculateLoaderTempFolder ( void )
{
	if ( g_pGlob )
	{
		// Add the DBPDATA folder to temp
		strcpy ( g_WindowsTempDirectory, g_pGlob->pEXEUnpackDirectory );
		if ( g_WindowsTempDirectory [ strlen(g_WindowsTempDirectory)-1 ]!='\\' )
 			strcat ( g_WindowsTempDirectory, "\\" );
	}
	else
	{
		// Current directory
		char CurrentDirectory[_MAX_PATH];
		_getcwd(CurrentDirectory, _MAX_PATH);

		// Find temporary directory (C:\WINDOWS\Temp)
		GetTempPath(_MAX_PATH, g_WindowsTempDirectory);
		if(_strcmpi(g_WindowsTempDirectory, CurrentDirectory)==NULL)
		{
			// Pre-XP Temp Folder
			GetWindowsDirectory(g_WindowsTempDirectory, _MAX_PATH);
			strcat(g_WindowsTempDirectory, "\\temp\\");
		}

		// Create DBPDATA folder in any event
		_chdir(g_WindowsTempDirectory);
		_mkdir("dbpdata");
		_chdir(CurrentDirectory);

		// Add the DBPDATA folder to temp
		strcat ( g_WindowsTempDirectory, "\\dbpdata\\" );
	}
}

DARKSDK_DLL bool DBOFileExist ( LPSTR pFilename )
{
	HANDLE hfile = GG_CreateFile ( pFilename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hfile != INVALID_HANDLE_VALUE )
	{
		CloseHandle ( hfile );
		return true;
	}
	return false;
}

DARKSDK_DLL bool ConvertToDBOBlock ( LPSTR pFilename, LPSTR pExtension, void** ppDBOBlock, DWORD* pdwBlockSize )
{
	// result var
	bool bResult=false;

	// disabled for Wicked Debugging (no D3DXOF 64 Bit DLL!!!)
	// LEELEE, there is, but its an updated loadde for X files, maybe look at it AFTER ASSIMP!
	#ifdef WICKEDENGINE
	MessageBox(NULL, "No D3DXOF 64 Bit DLL! Need to use 32 bit LIB in 64 bit compile, possible?", "So no X file loading for now", MB_OK);
	#else

	// clear return block
	*ppDBOBlock = NULL;
	*pdwBlockSize = 0;

	// Set LEGACY ON
	if ( g_bSwitchLegacyOn==true ) SetLegacyModeOn();

	// 061115 - Choose X or XYZ to load
	bool bSuccess = false;
	int iModelFormatMode = 0;
	void* pTempBlock=0;
	DWORD dwTempSize=0;
	if ( strnicmp ( pExtension, "xyz", 3 ) == 0 ) iModelFormatMode = 1;
	switch ( iModelFormatMode )
	{
		case 1 : bSuccess = false; break;
		default : bSuccess = ConvXConvert ( pFilename, &pTempBlock, &dwTempSize ); break;
	}

	// Call Convert Function
	if ( bSuccess==true )
	{
		// Create local memory for block
		*pdwBlockSize = dwTempSize;
		*ppDBOBlock = (void*) new char [ dwTempSize ];
		memcpy ( (LPSTR)*ppDBOBlock, (LPSTR)pTempBlock, dwTempSize );

		// Call Free Function			
		switch ( iModelFormatMode )
		{
			case 1 : break;
			default : ConvXFree( (LPSTR)pTempBlock ); break;
		}

		// success
		bResult=true;
	}
	else
	{
		// could not convert
		bResult=false;
	}
	#endif

	// okay
	return bResult;
}

// DBO Import/Export Functions

DARKSDK_DLL bool LoadDBODataBlock ( LPSTR pFilename, DWORD* pdwBlockSize, void** ppDBOBlock )
{
	// Obtain extension
	char pExtension[256];
	strcpy(pExtension, "");
	for ( int n=strlen(pFilename); n>0; n-- )
	{
		if ( pFilename[n]=='.' )
		{
			strcpy(pExtension, pFilename+n+1);
			break;
		}
	}

	/* never do MD3 at this level again
	// if file MD3 format - permit filecheck skip
	if ( _stricmp ( pExtension, "MD3" )!=NULL )
	{
		// does file exist
		if ( !DBOFileExist ( pFilename ) )
		{
			RunTimeError ( RUNTIMEERROR_FILENOTEXIST, pFilename );
			return false;
		}
	}
	*/

	// if file native DBO format
	if ( _stricmp ( pExtension, "DBO" )==NULL )
	{
		// load DBO object directly
		if ( !DBOLoadBlockFile ( pFilename, ppDBOBlock, pdwBlockSize ) )
		{
			char str[ 1024 ];
			sprintf_s( str, "Failed to load object: %s", pFilename );
			Message( 0, str, "Error" );
			RunTimeError ( RUNTIMEERROR_B3DOBJECTLOADFAILED );
			return false;
		}
	}
	else
	{
		// call converter DLL (ConvX.dll)
		if ( !ConvertToDBOBlock ( pFilename, pExtension, ppDBOBlock, pdwBlockSize ) )
		{
			char str[ 1024 ];
			sprintf_s( str, "Failed to load object: %s", pFilename );
			Message( 0, str, "Error" );
			RunTimeError ( RUNTIMEERROR_B3DOBJECTLOADFAILED );
			return false;
		}
	}

	// success
	return true;
}

enumScalingMode g_eLoadScalingMode = eScalingMode_Off;

DARKSDK_DLL void SetLoadScale ( enumScalingMode eScaleMode )
{
	// when loading (importing) an object, have the option of affecting the
	// scale as it loads to avoid creating huge transforms via the secondary scale
	// feature. Ideal if models come in as 1 unit = 1 meter vs GameGuru 1 unit = 1 inch
	g_eLoadScalingMode = eScaleMode;
}

