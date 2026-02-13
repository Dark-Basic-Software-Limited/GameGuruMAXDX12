DARKSDK_DLL void ComputeBoneDataInsideVertex ( sMesh* pMesh, LPGGMESH pVertexShaderMesh, DWORD dwWeightOffsetInBytes, DWORD dwIndicesOffsetInBytes, DWORD dwExtraWeightOffsetInBytes, DWORD dwExtraIndicesOffsetInBytes, DWORD dwVertSizeInBytes )
{
	// do not do if no bones (catched earlier when making bonedata in declaration)
	if ( pMesh->dwBoneCount==0 )
		return;

	#ifdef DX11
	// put animation bone data ( weights and indices ) into the vertex data (for VS skinning)
	DWORD dwNumVertices = pMesh->dwVertexCount;
	DWORD dwNumBones = pMesh->dwBoneCount;

	// create maximum of bones-per-vertex for collection purposes
	struct sBoneForVertInfo
	{
		float fWorkWeight;
		float fFinalWeight;
	};
	sBoneForVertInfo* pBFVI = new sBoneForVertInfo [ dwNumVertices * dwNumBones ];
	memset ( pBFVI, 0, sizeof( sBoneForVertInfo ) * dwNumVertices * dwNumBones );

	// collect bonesdata for all verts
	for ( int iBone = 0; iBone < ( int ) dwNumBones; iBone++ )
	{
		// go through all influences in bone
		for ( int iVert = 0; iVert < ( int ) pMesh->pBones [ iBone ].dwNumInfluences; iVert++ )
		{
			// get the vertex and weight
			int iIndexToVertex = pMesh->pBones [ iBone ].pVertices [ iVert ];
			float fWeight = pMesh->pBones [ iBone ].pWeights [ iVert ];

			// add this influence of weight to the pool under the correct bone index
			pBFVI [ (iIndexToVertex*dwNumBones)+iBone ].fWorkWeight = fWeight;
		}
	}

	// prepare an array to hold the best four
	int* piUseBone = new int [ dwNumVertices * 4 ];
	memset ( piUseBone, 0, sizeof( int ) * dwNumVertices * 4 );

	// 121018 - also prepare an array for next best four (for eight bone-per-vertex models)
	int* piUseBoneExtra = new int [ dwNumVertices * 4 ];
	memset ( piUseBoneExtra, 0, sizeof( int ) * dwNumVertices * 4 );

	// work out the best four per vertex
	int maxBonesAttachedToVertex = 0;
	for ( DWORD iIndexToVertex = 0; iIndexToVertex < dwNumVertices; iIndexToVertex++ )
	{
		// fill with minus ones to indicate no bone at all
		for ( int iFillFlag = 0; iFillFlag<4; iFillFlag++ )
			piUseBone [ (iIndexToVertex*4)+iFillFlag ] = -1;
		for ( int iFillFlag = 0; iFillFlag<4; iFillFlag++ )
			piUseBoneExtra [ (iIndexToVertex*4)+iFillFlag ] = -1;

		// mark out best four
		int iBestFour = 0;
		for (; iBestFour<4; iBestFour++ )
		{
			// find best
			int iBestBone = -1;
			float fBest = 0.0f;
			for ( int iBone = 0; iBone < ( int ) dwNumBones; iBone++ )
			{
				// get weight and compare for best one
				float fWeight = pBFVI [ (iIndexToVertex*dwNumBones)+iBone ].fWorkWeight;
				if ( fWeight>fBest ) { fBest=fWeight; iBestBone=iBone; }
			}
			if ( iBestBone!=-1)
			{
				// confirm bone as best (in order)
				pBFVI [ (iIndexToVertex*dwNumBones)+iBestBone ].fFinalWeight=fBest;

				// record its position so we can refer to the correct bone
				piUseBone [ (iIndexToVertex*4)+iBestFour ] = iBestBone;

				// clear so not included in next iteration
				pBFVI [ (iIndexToVertex*dwNumBones)+iBestBone ].fWorkWeight=0.0f;
			}
			else
			{
				// no more weights, can break out
				break;
			}
		}
		if ( iBestFour == 4 )
		{
			// and add extra four one influences if flagged
			int iNextFour = 0;
			for (; iNextFour<4; iNextFour++ )
			{
				int iExtraBestBone = -1;
				float fExtraBest = 0.0f;
				for ( int iBone = 0; iBone < ( int ) dwNumBones; iBone++ )
				{
					// get weight and compare for best one
					float fWeight = pBFVI [ (iIndexToVertex*dwNumBones)+iBone ].fWorkWeight;
					if ( fWeight>fExtraBest ) { fExtraBest=fWeight; iExtraBestBone=iBone; }
				}
				if ( iExtraBestBone!=-1)
				{
					pBFVI [ (iIndexToVertex*dwNumBones)+iExtraBestBone ].fFinalWeight=fExtraBest;
					piUseBoneExtra [ (iIndexToVertex*4)+iNextFour ] = iExtraBestBone;
					pBFVI [ (iIndexToVertex*dwNumBones)+iExtraBestBone ].fWorkWeight=0.0f;
				}
				else
				{
					// no more extra weights, can break out
					break;
				}
			}
		}
	}

	// create new vertex data to include bone data
	DWORD dwNewFVFSize = pMesh->dwFVFSize+16+16;
	if ( dwExtraWeightOffsetInBytes > 0 ) dwNewFVFSize = dwNewFVFSize+16+16;
	DWORD dwNewSize = dwNumVertices * dwNewFVFSize;
	BYTE* pNewVertexData = new BYTE[dwNewSize];
	memset ( pNewVertexData, 0, sizeof(pNewVertexData) );
	BYTE* pWritePtr = pNewVertexData;
	BYTE* pReadPtr = pMesh->pVertexData;
	for ( DWORD v = 0; v < dwNumVertices; v++ )
	{
		memcpy ( pWritePtr, pReadPtr, pMesh->dwFVFSize );
		pWritePtr += pMesh->dwFVFSize + 16 + 16;
		if ( dwExtraWeightOffsetInBytes > 0 ) pWritePtr += 16 + 16;
		pReadPtr += pMesh->dwFVFSize;
	}

	// vertex data - populate with best four
	if ( pNewVertexData )
	{
		// get actual DWORD offsets
		DWORD dwWeightOffset = dwWeightOffsetInBytes/4;
		DWORD dwIndicesOffset = dwIndicesOffsetInBytes/4;
		DWORD dwExtraWeightOffset = dwExtraWeightOffsetInBytes/4;
		DWORD dwExtraIndicesOffset = dwExtraIndicesOffsetInBytes/4;

		// go through vertex data
		for ( DWORD v = 0; v < dwNumVertices; v++ )
		{
			// base ptr for vertex
			DWORD* pVertexPtr = (DWORD*)(pNewVertexData+(v*dwVertSizeInBytes));

			// clear weight and indices memarea
			memset ( pVertexPtr+dwWeightOffset, 0, sizeof ( float ) * 8 );

			// indexes to the bones we will use
			int iBoneA = piUseBone [ (v*4)+0 ];
			int iBoneB = piUseBone [ (v*4)+1 ];
			int iBoneC = piUseBone [ (v*4)+2 ];
			int iBoneD = piUseBone [ (v*4)+3 ];

			// indices for this vertex (references to bone index)
			if ( iBoneA>=0 ) *(float*)(pVertexPtr+dwIndicesOffset+0) = (float)iBoneA;
			if ( iBoneB>=0 ) *(float*)(pVertexPtr+dwIndicesOffset+1) = (float)iBoneB;
			if ( iBoneC>=0 ) *(float*)(pVertexPtr+dwIndicesOffset+2) = (float)iBoneC;
			if ( iBoneD>=0 ) *(float*)(pVertexPtr+dwIndicesOffset+3) = (float)iBoneD;

			// weights for this vertex (the weight to use against the bone)
			if ( iBoneA>=0 ) *(float*)(pVertexPtr+dwWeightOffset+0) = pBFVI [ (v*dwNumBones)+iBoneA ].fFinalWeight;
			if ( iBoneB>=0 ) *(float*)(pVertexPtr+dwWeightOffset+1) = pBFVI [ (v*dwNumBones)+iBoneB ].fFinalWeight;
			if ( iBoneC>=0 ) *(float*)(pVertexPtr+dwWeightOffset+2) = pBFVI [ (v*dwNumBones)+iBoneC ].fFinalWeight;
			if ( iBoneD>=0 ) *(float*)(pVertexPtr+dwWeightOffset+3) = pBFVI [ (v*dwNumBones)+iBoneD ].fFinalWeight;

			// 121018 - and also add extra indices and weights if present
			if ( dwExtraWeightOffset > 0 )
			{
				memset ( pVertexPtr+dwExtraWeightOffset, 0, sizeof ( float ) * 8 );
				int iBoneE = piUseBoneExtra [ (v*4)+0 ];
				int iBoneF = piUseBoneExtra [ (v*4)+1 ];
				int iBoneG = piUseBoneExtra [ (v*4)+2 ];
				int iBoneH = piUseBoneExtra [ (v*4)+3 ];
				if ( iBoneE>=0 ) *(float*)(pVertexPtr+dwExtraIndicesOffset+0) = (float)iBoneE;
				if ( iBoneF>=0 ) *(float*)(pVertexPtr+dwExtraIndicesOffset+1) = (float)iBoneF;
				if ( iBoneG>=0 ) *(float*)(pVertexPtr+dwExtraIndicesOffset+2) = (float)iBoneG;
				if ( iBoneH>=0 ) *(float*)(pVertexPtr+dwExtraIndicesOffset+3) = (float)iBoneH;
				if ( iBoneE>=0 ) *(float*)(pVertexPtr+dwExtraWeightOffset+0) = pBFVI [ (v*dwNumBones)+iBoneE ].fFinalWeight;
				if ( iBoneF>=0 ) *(float*)(pVertexPtr+dwExtraWeightOffset+1) = pBFVI [ (v*dwNumBones)+iBoneF ].fFinalWeight;
				if ( iBoneG>=0 ) *(float*)(pVertexPtr+dwExtraWeightOffset+2) = pBFVI [ (v*dwNumBones)+iBoneG ].fFinalWeight;
				if ( iBoneH>=0 ) *(float*)(pVertexPtr+dwExtraWeightOffset+3) = pBFVI [ (v*dwNumBones)+iBoneH ].fFinalWeight;
			}
		}
	}

	// now copy new vertex data to mesh
	SAFE_DELETE(pMesh->pVertexData);
	pMesh->dwFVFSize = dwNewFVFSize;
	pMesh->pVertexData = pNewVertexData;

	// free usages
	SAFE_DELETE ( pBFVI );
	SAFE_DELETE ( piUseBone );
	SAFE_DELETE ( piUseBoneExtra );
	#else
	// leeadd - 200204 - put animation bone data ( weights and indices ) into the vertex data (for VS skinning)
	DWORD dwNumVertices = pVertexShaderMesh->GetNumVertices();
	DWORD dwNumBones = pMesh->dwBoneCount;

	// create maximum of bones-per-vertex for collection purposes
	struct sBoneForVertInfo
	{
		float fWorkWeight;
		float fFinalWeight;
	};
	sBoneForVertInfo* pBFVI = new sBoneForVertInfo [ dwNumVertices * dwNumBones ];
	memset ( pBFVI, 0, sizeof( sBoneForVertInfo ) * dwNumVertices * dwNumBones );

	// collect bonesdata for all verts
	for ( int iBone = 0; iBone < ( int ) dwNumBones; iBone++ )
	{
		// go through all influenced bones
		for ( int iVert = 0; iVert < ( int ) pMesh->pBones [ iBone ].dwNumInfluences; iVert++ )
		{
			// get the vertex and weight
			int iIndexToVertex = pMesh->pBones [ iBone ].pVertices [ iVert ];
			float fWeight = pMesh->pBones [ iBone ].pWeights [ iVert ];

			// add this influence of weight to the pool under the correct bone index
			pBFVI [ (iIndexToVertex*dwNumBones)+iBone ].fWorkWeight = fWeight;
		}
	}

	// prepare an array to hold the best four
	int* piUseBone = new int [ dwNumVertices * 4 ];
	memset ( piUseBone, 0, sizeof( int ) * dwNumVertices * 4 );

	// work out the best four per vertex
	for ( DWORD iIndexToVertex = 0; iIndexToVertex < dwNumVertices; iIndexToVertex++ )
	{
		// fill with minus ones to indicate no bone at all
		for ( int iFillFlag = 0; iFillFlag<4; iFillFlag++ )
			piUseBone [ (iIndexToVertex*4)+iFillFlag ] = -1;

		// mark out best four
		for ( int iBestFour = 0; iBestFour<4; iBestFour++ )
		{
			// find best
			int iBestBone = -1;
			float fBest = 0.0f;
			for ( int iBone = 0; iBone < ( int ) dwNumBones; iBone++ )
			{
				// get weight and compare for best one
				float fWeight = pBFVI [ (iIndexToVertex*dwNumBones)+iBone ].fWorkWeight;
				if ( fWeight>fBest ) { fBest=fWeight; iBestBone=iBone; }
			}
			if ( iBestBone!=-1)
			{
				// confirm bone as best (in order)
				pBFVI [ (iIndexToVertex*dwNumBones)+iBestBone ].fFinalWeight=fBest;

				// record its position so we can refer to the correct bone
				piUseBone [ (iIndexToVertex*4)+iBestFour ] = iBestBone;

				// clear so not included in next iteration
				pBFVI [ (iIndexToVertex*dwNumBones)+iBestBone ].fWorkWeight=0.0f;
			}
			else
			{
				// no more weights, can break out
				break;
			}
		}
	}

	// lock vertex data and populate with best four
	BYTE* pVertexDataPtr = NULL;
	pVertexShaderMesh->LockVertexBuffer(D3DLOCK_READONLY, (VOID**)&pVertexDataPtr);
	if ( pVertexDataPtr )
	{
		// get actual DWORD offsets
		DWORD dwWeightOffset = dwWeightOffsetInBytes/4;
		DWORD dwIndicesOffset = dwIndicesOffsetInBytes/4;

		// go through vertex data
		for ( DWORD v = 0; v < dwNumVertices; v++ )
		{
			// base ptr for vertex
			DWORD* pVertexPtr = (DWORD*)(pVertexDataPtr+(v*dwVertSizeInBytes));

			// clear weight and indices memarea
			memset ( pVertexPtr+dwWeightOffset, 0, sizeof ( float ) * 8 );

			// indexes to the bones we will use
			int iBoneA = piUseBone [ (v*4)+0 ];
			int iBoneB = piUseBone [ (v*4)+1 ];
			int iBoneC = piUseBone [ (v*4)+2 ];
			int iBoneD = piUseBone [ (v*4)+3 ];

			// indices for this vertex (references to bone index)
			if ( iBoneA>=0 ) *(float*)(pVertexPtr+dwIndicesOffset+0) = (float)iBoneA;
			if ( iBoneB>=0 ) *(float*)(pVertexPtr+dwIndicesOffset+1) = (float)iBoneB;
			if ( iBoneC>=0 ) *(float*)(pVertexPtr+dwIndicesOffset+2) = (float)iBoneC;
			if ( iBoneD>=0 ) *(float*)(pVertexPtr+dwIndicesOffset+3) = (float)iBoneD;

			// weights for this vertex (the weight to use against the bone)
			if ( iBoneA>=0 ) *(float*)(pVertexPtr+dwWeightOffset+0) = pBFVI [ (v*dwNumBones)+iBoneA ].fFinalWeight;
			if ( iBoneB>=0 ) *(float*)(pVertexPtr+dwWeightOffset+1) = pBFVI [ (v*dwNumBones)+iBoneB ].fFinalWeight;
			if ( iBoneC>=0 ) *(float*)(pVertexPtr+dwWeightOffset+2) = pBFVI [ (v*dwNumBones)+iBoneC ].fFinalWeight;
			if ( iBoneD>=0 ) *(float*)(pVertexPtr+dwWeightOffset+3) = pBFVI [ (v*dwNumBones)+iBoneD ].fFinalWeight;
		}

		// unlock vertex data
		pVertexShaderMesh->UnlockVertexBuffer();
	}

	// free usages
	SAFE_DELETE ( pBFVI );
	SAFE_DELETE ( piUseBone );
	#endif

	// complete
	return;
}

DARKSDK_DLL void FlipNormals ( sMesh* pMesh, int iFlipMode )
{
	// get the offset map for the FVF
	sOffsetMap offsetMap;
	GetFVFOffsetMap ( pMesh, &offsetMap );

	// make sure we have normals in the vertices
	if ( offsetMap.dwNZ>0 )
	{
		// go through all of the vertices
		DWORD dwNumberOfVertices=pMesh->dwVertexCount;
		for ( int iCurrentVertex = 0; iCurrentVertex < (int)dwNumberOfVertices; iCurrentVertex++ )
		{
			GGVECTOR3 vecNorm = *(GGVECTOR3*)( ( float* ) pMesh->pVertexData + offsetMap.dwNX + ( offsetMap.dwSize * iCurrentVertex ) );
			if ( iFlipMode == 0 )
			{
				vecNorm.x *= -1;
				vecNorm.y *= -1;
				vecNorm.z *= -1;
			}
			*(GGVECTOR3*)( ( float* ) pMesh->pVertexData + offsetMap.dwNX + ( offsetMap.dwSize * iCurrentVertex ) ) = vecNorm;
		}
	}

	// flag mesh for a VB update
	pMesh->bVBRefreshRequired = true;
#ifndef WICKEDENGINE
	g_vRefreshMeshList.push_back ( pMesh );
#endif
}

DARKSDK_DLL void GenerateNewNormalsForMesh	( sMesh* pMesh, int iMode )
{
	#ifdef DX11
	// get the offset map for the FVF
	sOffsetMap offsetMap;
	GetFVFOffsetMap ( pMesh, &offsetMap );

	// make sure we have normals in the vertices
	if ( offsetMap.dwNZ>0 )
	{
		// go through index buffer or raw vertice list
		bool bUsingIndices = true;
		DWORD iCount = pMesh->dwIndexCount;
		if ( iCount == 0 ) { iCount = pMesh->dwVertexCount; bUsingIndices = false; }

		// go through all polys, work out normal, then apply to normal vectors
		for ( DWORD i=0; i<iCount; i+=3 )
		{
			// read face
			DWORD dwFace0, dwFace1, dwFace2;
			if ( bUsingIndices == true )
			{
				dwFace0 = pMesh->pIndices[i+0];
				dwFace1 = pMesh->pIndices[i+1];
				dwFace2 = pMesh->pIndices[i+2];
			}
			else
			{
				dwFace0 = i+0;
				dwFace1 = i+1;
				dwFace2 = i+2;
			}

			// get vertex
			GGVECTOR3 vecVert0 = *(GGVECTOR3*)( ( float* ) pMesh->pVertexData + offsetMap.dwX + ( offsetMap.dwSize * dwFace0 ) );
			GGVECTOR3 vecVert1 = *(GGVECTOR3*)( ( float* ) pMesh->pVertexData + offsetMap.dwX + ( offsetMap.dwSize * dwFace1 ) );
			GGVECTOR3 vecVert2 = *(GGVECTOR3*)( ( float* ) pMesh->pVertexData + offsetMap.dwX + ( offsetMap.dwSize * dwFace2 ) );

			// get normal
			GGVECTOR3 vecNorm0 = *(GGVECTOR3*)( ( float* ) pMesh->pVertexData + offsetMap.dwNX + ( offsetMap.dwSize * dwFace0 ) );
			GGVECTOR3 vecNorm1 = *(GGVECTOR3*)( ( float* ) pMesh->pVertexData + offsetMap.dwNX + ( offsetMap.dwSize * dwFace1 ) );
			GGVECTOR3 vecNorm2 = *(GGVECTOR3*)( ( float* ) pMesh->pVertexData + offsetMap.dwNX + ( offsetMap.dwSize * dwFace2 ) );

			// calculate normal from vertices
			GGVECTOR3 vNormal;
			GGVec3Cross ( &vNormal, &( vecVert2 - vecVert1 ), &( vecVert0 - vecVert1 ) );
			GGVec3Normalize ( &vNormal, &vNormal );

			// apply new normal to geometry for all normals associated with the poly
			*(GGVECTOR3*)( ( float* ) pMesh->pVertexData + offsetMap.dwNX + ( offsetMap.dwSize * dwFace0 ) ) = vNormal;
			*(GGVECTOR3*)( ( float* ) pMesh->pVertexData + offsetMap.dwNX + ( offsetMap.dwSize * dwFace1 ) ) = vNormal;
			*(GGVECTOR3*)( ( float* ) pMesh->pVertexData + offsetMap.dwNX + ( offsetMap.dwSize * dwFace2 ) ) = vNormal;
		}
	}

	// flag mesh for a VB update
	pMesh->bVBRefreshRequired = true;
#ifndef WICKEDENGINE
	g_vRefreshMeshList.push_back ( pMesh );
#endif

	#else
	if ( pMesh )
	{
		// use DX to generate new normals for this mesh
		if ( iMode == 0 )
		{
			LPGGMESH pDXMesh = LocalMeshToDXMesh ( pMesh, NULL, 0 );
			if ( pDXMesh )
			{
				D3DXComputeNormals ( pDXMesh, NULL );
				UpdateLocalMeshWithDXMesh ( pMesh, pDXMesh );
				SAFE_RELEASE(pDXMesh);
			}
		}

		// special mode can flip all normals in mesh (corrects bad exports)
		if ( iMode == 1 )
		{
			FlipNormals ( pMesh, 0 );
		}
	}
	#endif
}

DARKSDK_DLL void GenerateNormalsForMesh ( sMesh* pMesh, int iMode )
{
	#ifdef DX11
	#else
	// work vars
	GGVERTEXELEMENT Declaration[MAX_FVF_DECL_SIZE];
	GGVERTEXELEMENT End = GDECL_END();
	int iElem;

	// 16 or 32 bit mesh size
	DWORD dwSysteMemFlag = D3DXMESH_SYSTEMMEM;
	if ( pMesh->dwVertexCount > 65535 ) dwSysteMemFlag = D3DXMESH_SYSTEMMEM | D3DXMESH_32BIT;

	// get DX mesh from sMesh
	LPGGMESH pDXMesh = LocalMeshToDXMesh ( pMesh, NULL, 0 );
	if ( pDXMesh==NULL )
		return;

	// extract declaration from mesh
	pDXMesh->GetDeclaration( Declaration );

	// check if mesh already has a normal component
	BOOL bHasNormals = FALSE;
	for( iElem=0; Declaration[iElem].Stream != End.Stream; iElem++ )
	{   
		if( Declaration[iElem].Usage == GGDECLUSAGE_NORMAL )
		{
			bHasNormals = TRUE;
			break;
		}
	}

	// Update Mesh Semantics if does not have normals
	if( !bHasNormals ) 
	{
		Declaration[iElem].Stream = 0;
		Declaration[iElem].Offset = (WORD)pDXMesh->GetNumBytesPerVertex();
		Declaration[iElem].Type = GGDECLTYPE_FLOAT3;
		Declaration[iElem].Method = GGDECLMETHOD_DEFAULT;
		Declaration[iElem].Usage = GGDECLUSAGE_NORMAL;
		Declaration[iElem].UsageIndex = 0;
		Declaration[iElem+1] = End;
		LPGGMESH pTempMesh;
		HRESULT hr = pDXMesh->CloneMesh( dwSysteMemFlag, Declaration, m_pD3D, &pTempMesh );
		if( SUCCEEDED( hr ) )
		{
			SAFE_RELEASE( pDXMesh );
			pDXMesh = pTempMesh;
			D3DXComputeNormals ( pDXMesh, NULL );
		}
	}

	// update sMesh with new DX mesh
	UpdateLocalMeshWithDXMesh ( pMesh, pDXMesh );

	// free usages
	SAFE_RELEASE(pDXMesh);
	#endif
}

DARKSDK_DLL void GenerateExtraDataForMeshEx ( sMesh* pMesh, BOOL bNormals, BOOL bTangents, BOOL bBinormals, BOOL bDiffuse, BOOL bBones, DWORD dwGenerateMode )
{
	#ifdef DX11

	// get FVF details
	sOffsetMap offsetMap;
	GetFVFValueOffsetMap ( pMesh->dwFVF, &offsetMap );

	// deactivate bone flag if no bones in source mesh
	if ( pMesh->dwBoneCount==0 ) bBones=FALSE;

	// valid mesh (no longer using DXMESH)
	if ( pMesh->dwFVF > 0 )
	{
		// extract vertex size from mesh
		WORD wNumBytesPerVertex = (WORD)pMesh->dwFVFSize;

		// Starting declaration
		int iDeclarationIndex = 0;
		D3D11_INPUT_ELEMENT_DESC pDeclaration[12];

		// check if mesh already has a component (and build declaration)
		BOOL bHasNormals = FALSE;
		BOOL bHasDiffuse = FALSE;
		BOOL bHasTangents = FALSE;
		BOOL bHasBinormals = FALSE;
		BOOL bHasBlendWeights = FALSE;
		BOOL bHasBlendIndices = FALSE;
		BOOL bHasSecondaryUVs = FALSE;
		if ( pMesh->dwFVF & GGFVF_XYZ )
		{
			pDeclaration[iDeclarationIndex].SemanticName = "POSITION";
			pDeclaration[iDeclarationIndex].SemanticIndex = 0;
			pDeclaration[iDeclarationIndex].Format = DXGI_FORMAT_R32G32B32_FLOAT;
			pDeclaration[iDeclarationIndex].InputSlot = 0;
			pDeclaration[iDeclarationIndex].AlignedByteOffset = 0;
			pDeclaration[iDeclarationIndex].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
			pDeclaration[iDeclarationIndex].InstanceDataStepRate = 0;
			iDeclarationIndex++;
		}
		if ( pMesh->dwFVF & GGFVF_NORMAL ) 
		{
			pDeclaration[iDeclarationIndex].SemanticName = "NORMAL";
			pDeclaration[iDeclarationIndex].SemanticIndex = 0;
			pDeclaration[iDeclarationIndex].Format = DXGI_FORMAT_R32G32B32_FLOAT;
			pDeclaration[iDeclarationIndex].InputSlot = 0;
			pDeclaration[iDeclarationIndex].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
			pDeclaration[iDeclarationIndex].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
			pDeclaration[iDeclarationIndex].InstanceDataStepRate = 0;
			iDeclarationIndex++;
			bHasNormals = TRUE;
		}
		if ( pMesh->dwFVF & GGFVF_TEX1 || offsetMap.dwTU[0] > 0 ) // lightmapped objects failed this test!
		{
			pDeclaration[iDeclarationIndex].SemanticName = "TEXCOORD";
			pDeclaration[iDeclarationIndex].SemanticIndex = 0;
			pDeclaration[iDeclarationIndex].Format = DXGI_FORMAT_R32G32_FLOAT;
			pDeclaration[iDeclarationIndex].InputSlot = 0;
			pDeclaration[iDeclarationIndex].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
			pDeclaration[iDeclarationIndex].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
			pDeclaration[iDeclarationIndex].InstanceDataStepRate = 0;
			iDeclarationIndex++;
		}
		if ( pMesh->dwFVF & GGFVF_DIFFUSE ) 
		{
			pDeclaration[iDeclarationIndex].SemanticName = "COLOR";
			pDeclaration[iDeclarationIndex].SemanticIndex = 0;
			pDeclaration[iDeclarationIndex].Format = DXGI_FORMAT_R32_FLOAT;
			pDeclaration[iDeclarationIndex].InputSlot = 0;
			pDeclaration[iDeclarationIndex].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
			pDeclaration[iDeclarationIndex].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
			pDeclaration[iDeclarationIndex].InstanceDataStepRate = 0;
			iDeclarationIndex++;
			bHasDiffuse = TRUE;
		}
		//if ( pMesh->dwFVF & dsadadsa ) bHasTangents = TRUE; // No FVF specifies TANGENT/BINORMALS
		//if ( pMesh->dwFVF & dsadadsa ) bHasBinormals = TRUE;
		//if ( pMesh->dwFVF & offsetMap.dwTU[1] > 0 ) 
		if ( offsetMap.dwTU[1] > 0 ) // 290618 - small fix removed the & operation?!
		{
			pDeclaration[iDeclarationIndex].SemanticName = "TEXCOORD";
			pDeclaration[iDeclarationIndex].SemanticIndex = 1;
			pDeclaration[iDeclarationIndex].Format = DXGI_FORMAT_R32G32_FLOAT;
			pDeclaration[iDeclarationIndex].InputSlot = 0;
			pDeclaration[iDeclarationIndex].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
			pDeclaration[iDeclarationIndex].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
			pDeclaration[iDeclarationIndex].InstanceDataStepRate = 0;
			iDeclarationIndex++;
			bHasSecondaryUVs = TRUE;
		}

		// 290618 - allow secondary UV objects to have tangent and binormals calculated (for lightmapped PBR objects)
		// objects that have TWO UV channels cause declaration problems when generating data
		// so we strip out any secondary UV channels from declaration (if generate bit two is one)
		//if ( bHasSecondaryUVs==TRUE )
		//{
		//	MessageBox ( NULL, "Secondary UV data not accepted", "GenerateExtraData", MB_OK );
		//	return;
		//}

		// Update Mesh Semantics if does not have components
		bool bGiveMeNormals = false;
		bool bGiveMeDiffuse = false;
		bool bGiveMeTangents = false;
		bool bGiveMeBinormals = false;
		bool bGiveMeBlendWeights = false;
		bool bGiveMeBlendIndices = false;
		if( !bHasNormals && bNormals ) 
		{
			bGiveMeNormals = true;
			pDeclaration[iDeclarationIndex].SemanticName = "NORMAL";
			pDeclaration[iDeclarationIndex].SemanticIndex = 0;
			pDeclaration[iDeclarationIndex].Format = DXGI_FORMAT_R32G32B32_FLOAT;
			pDeclaration[iDeclarationIndex].InputSlot = 0;
			pDeclaration[iDeclarationIndex].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
			pDeclaration[iDeclarationIndex].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
			pDeclaration[iDeclarationIndex].InstanceDataStepRate = 0;
			iDeclarationIndex++;
			wNumBytesPerVertex+=12;
		}
		if( !bHasDiffuse && bDiffuse ) 
		{
			bGiveMeDiffuse = true;
			pDeclaration[iDeclarationIndex].SemanticName = "COLOR";
			pDeclaration[iDeclarationIndex].SemanticIndex = 0;
			pDeclaration[iDeclarationIndex].Format = DXGI_FORMAT_R32_FLOAT;
			pDeclaration[iDeclarationIndex].InputSlot = 0;
			pDeclaration[iDeclarationIndex].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
			pDeclaration[iDeclarationIndex].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
			pDeclaration[iDeclarationIndex].InstanceDataStepRate = 0;
			iDeclarationIndex++;
			wNumBytesPerVertex+=4;
		}
		if( !bHasTangents && bTangents ) 
		{
			bGiveMeTangents = true;
			pDeclaration[iDeclarationIndex].SemanticName = "TANGENT";
			pDeclaration[iDeclarationIndex].SemanticIndex = 0;
			pDeclaration[iDeclarationIndex].Format = DXGI_FORMAT_R32G32B32_FLOAT;
			pDeclaration[iDeclarationIndex].InputSlot = 0;
			pDeclaration[iDeclarationIndex].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
			pDeclaration[iDeclarationIndex].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
			pDeclaration[iDeclarationIndex].InstanceDataStepRate = 0;
			iDeclarationIndex++;
			wNumBytesPerVertex+=12;
		}
		if( !bHasBinormals && bBinormals ) 
		{
			bGiveMeBinormals = true;
			pDeclaration[iDeclarationIndex].SemanticName = "BINORMAL";
			pDeclaration[iDeclarationIndex].SemanticIndex = 0;
			pDeclaration[iDeclarationIndex].Format = DXGI_FORMAT_R32G32B32_FLOAT;
			pDeclaration[iDeclarationIndex].InputSlot = 0;
			pDeclaration[iDeclarationIndex].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
			pDeclaration[iDeclarationIndex].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
			pDeclaration[iDeclarationIndex].InstanceDataStepRate = 0;
			iDeclarationIndex++;
			wNumBytesPerVertex+=12;
		}
		DWORD dwOffsetToWeights = wNumBytesPerVertex;
		if( !bHasBlendWeights && bBones ) 
		{
			bGiveMeBlendWeights = true;
			pDeclaration[iDeclarationIndex].SemanticName = "TEXCOORD";
			pDeclaration[iDeclarationIndex].SemanticIndex = 1;
			pDeclaration[iDeclarationIndex].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
			pDeclaration[iDeclarationIndex].InputSlot = 0;
			pDeclaration[iDeclarationIndex].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
			pDeclaration[iDeclarationIndex].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
			pDeclaration[iDeclarationIndex].InstanceDataStepRate = 0;
			iDeclarationIndex++;
			wNumBytesPerVertex+=16;
		}
		DWORD dwOffsetToIndices = wNumBytesPerVertex;
		if( !bHasBlendIndices && bBones ) 
		{
			bGiveMeBlendIndices = true;
			pDeclaration[iDeclarationIndex].SemanticName = "TEXCOORD";
			pDeclaration[iDeclarationIndex].SemanticIndex = 2;
			pDeclaration[iDeclarationIndex].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
			pDeclaration[iDeclarationIndex].InputSlot = 0;
			pDeclaration[iDeclarationIndex].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
			pDeclaration[iDeclarationIndex].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
			pDeclaration[iDeclarationIndex].InstanceDataStepRate = 0;
			iDeclarationIndex++;
			wNumBytesPerVertex+=16;
		}

		// 121018 - add extra weights if required
		DWORD dwOffsetToExtraWeights = 0;
		DWORD dwOffsetToExtraIndices = 0;
		if ( pMesh->pVertexShaderEffect )
		{
			if ( pMesh->pVertexShaderEffect->m_bNeed8BonesPerVertex == true )
			{
				dwOffsetToExtraWeights = wNumBytesPerVertex;
				if( !bHasBlendWeights && bBones ) 
				{
					pDeclaration[iDeclarationIndex].SemanticName = "TEXCOORD";
					pDeclaration[iDeclarationIndex].SemanticIndex = 3;
					pDeclaration[iDeclarationIndex].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
					pDeclaration[iDeclarationIndex].InputSlot = 0;
					pDeclaration[iDeclarationIndex].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
					pDeclaration[iDeclarationIndex].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
					pDeclaration[iDeclarationIndex].InstanceDataStepRate = 0;
					iDeclarationIndex++;
					wNumBytesPerVertex+=16;
				}
				dwOffsetToExtraIndices = wNumBytesPerVertex;
				if( !bHasBlendIndices && bBones ) 
				{
					pDeclaration[iDeclarationIndex].SemanticName = "TEXCOORD";
					pDeclaration[iDeclarationIndex].SemanticIndex = 4;
					pDeclaration[iDeclarationIndex].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
					pDeclaration[iDeclarationIndex].InputSlot = 0;
					pDeclaration[iDeclarationIndex].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
					pDeclaration[iDeclarationIndex].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
					pDeclaration[iDeclarationIndex].InstanceDataStepRate = 0;
					iDeclarationIndex++;
					wNumBytesPerVertex+=16;
				}
			}
		}

		// Compute any tangent basis data (no not attempt to correct mesh too much)
		if ( bGiveMeNormals || bGiveMeTangents || bGiveMeBinormals )
		{
			// cannot use FixTangents on bone based model, it adds verts to mess up bone skin (and does not work well)
			//PE: Tangent is not calculated correct, this calculation will be moved.
			//PE: Tangent calculation added to vertex shaders.
			//PE: Lee: We can consider to remove tangent,binormal all together should give a nice boost as we do not need to sent it to the GPU.
			//PE: in the dx9 version i was also calculating it in the shaders , so this is an old bug.
			//PE: issue https://github.com/TheGameCreators/GameGuruRepo/issues/85
			ComputeTangentBasisEx ( pMesh, bGiveMeNormals, bGiveMeTangents, bGiveMeBinormals, false, false, true );
		}

		// Fill blend data (weight and indices) if required 
		if ( bGiveMeBlendWeights || bGiveMeBlendIndices )
		{
			// fills mesh with additional data
			ComputeBoneDataInsideVertex ( pMesh, pMesh, dwOffsetToWeights, dwOffsetToIndices, dwOffsetToExtraWeights, dwOffsetToExtraIndices, wNumBytesPerVertex );

			// flag mesh as now being vertex skinned by a shader
			pMesh->bShaderBoneSkinning = true;
		}

		// copy declaration into old D3D9 format (as DBO relies on this data in the binary!)
		int iDecIndex = 0;
		int iByteOffset = 0;
		for (; iDecIndex < iDeclarationIndex; iDecIndex++ )
		{
			int iEntryByteSize = 0;
			if ( stricmp ( pDeclaration[iDecIndex].SemanticName, "POSITION" ) == NULL )
			{
				pMesh->pVertexDeclaration[iDecIndex].Usage = GGDECLUSAGE_POSITION;
				pMesh->pVertexDeclaration[iDecIndex].Type = GGDECLTYPE_FLOAT3;
				iEntryByteSize = 12;
			}
			if ( stricmp ( pDeclaration[iDecIndex].SemanticName, "NORMAL" ) == NULL )
			{
				pMesh->pVertexDeclaration[iDecIndex].Usage = GGDECLUSAGE_NORMAL;
				pMesh->pVertexDeclaration[iDecIndex].Type = GGDECLTYPE_FLOAT3;
				iEntryByteSize = 12;
			}
			if ( stricmp ( pDeclaration[iDecIndex].SemanticName, "COLOR" ) == NULL )
			{
				pMesh->pVertexDeclaration[iDecIndex].Usage = GGDECLUSAGE_COLOR;
				pMesh->pVertexDeclaration[iDecIndex].Type = GGDECLTYPE_FLOAT2;
				iEntryByteSize = 4;
			}
			if ( stricmp ( pDeclaration[iDecIndex].SemanticName, "TANGENT" ) == NULL )
			{
				pMesh->pVertexDeclaration[iDecIndex].Usage = GGDECLUSAGE_TANGENT;
				pMesh->pVertexDeclaration[iDecIndex].Type = GGDECLTYPE_FLOAT3;
				iEntryByteSize = 12;
			}			
			if ( stricmp ( pDeclaration[iDecIndex].SemanticName, "BINORMAL" ) == NULL )
			{
				pMesh->pVertexDeclaration[iDecIndex].Usage = GGDECLUSAGE_BINORMAL;
				pMesh->pVertexDeclaration[iDecIndex].Type = GGDECLTYPE_FLOAT3;
				iEntryByteSize = 12;
			}			
			if ( stricmp ( pDeclaration[iDecIndex].SemanticName, "TEXCOORD" ) == NULL )
			{
				pMesh->pVertexDeclaration[iDecIndex].Usage = GGDECLUSAGE_TEXCOORD;
				if ( pDeclaration[iDecIndex].Format == DXGI_FORMAT_R32G32B32A32_FLOAT )
				{
					pMesh->pVertexDeclaration[iDecIndex].Type = GGDECLTYPE_FLOAT4;
					iEntryByteSize = 16;
				}
				else
				{
					pMesh->pVertexDeclaration[iDecIndex].Type = GGDECLTYPE_FLOAT2;
					iEntryByteSize = 8;
				}
			}			
			pMesh->pVertexDeclaration[iDecIndex].Stream = 0;
			pMesh->pVertexDeclaration[iDecIndex].Method = GGDECLMETHOD_DEFAULT;
			pMesh->pVertexDeclaration[iDecIndex].UsageIndex = pDeclaration[iDecIndex].SemanticIndex;
			pMesh->pVertexDeclaration[iDecIndex].Offset = iByteOffset;
			iByteOffset += iEntryByteSize;
		}
		pMesh->pVertexDeclaration[iDecIndex] = GGDECLEND;
		#ifdef DX11
		pMesh->pVertexDeclaration[iDecIndex].Stream = 255;
		#endif
	}
	#else
	// work vars
	GGVERTEXELEMENT Declaration[MAX_FVF_DECL_SIZE];
	GGVERTEXELEMENT End = GDECL_END();
	int iElem;

	// deactivate bone flag if no bones in source mesh
	if ( pMesh->dwBoneCount==0 ) bBones=FALSE;

	// get DX mesh from sMesh
	LPGGMESH pDXMesh = LocalMeshToDXMesh ( pMesh, NULL, 0 );
	if ( pDXMesh )
	{
		// extract declaration and vertex size from mesh
		pDXMesh->GetDeclaration( Declaration );
		WORD wNumBytesPerVertex = (WORD)pDXMesh->GetNumBytesPerVertex();

		// check if mesh already has a component
		BOOL bHasNormals = FALSE;
		BOOL bHasDiffuse = FALSE;
		BOOL bHasTangents = FALSE;
		BOOL bHasBinormals = FALSE;
		BOOL bHasBlendWeights = FALSE;
		BOOL bHasBlendIndices = FALSE;
		BOOL bHasSecondaryUVs = FALSE;
		for( iElem=0; Declaration[iElem].Stream != End.Stream; iElem++ )
		{   
			if( Declaration[iElem].Usage == GGDECLUSAGE_NORMAL ) bHasNormals = TRUE;
			if( Declaration[iElem].Usage == D3DDECLUSAGE_COLOR ) bHasDiffuse = TRUE;
			if( Declaration[iElem].Usage == D3DDECLUSAGE_TANGENT ) bHasTangents = TRUE;
			if( Declaration[iElem].Usage == D3DDECLUSAGE_BINORMAL ) bHasBinormals = TRUE;
			if( Declaration[iElem].Usage == GGDECLUSAGE_TEXCOORD && Declaration[iElem].UsageIndex>0 ) bHasSecondaryUVs = TRUE;
		}

		// leefix - 071208 - U71 - objects that have TWO UV channels cause declaration problems when generating data
		// so we strip out any secondary UV channels from declaration (if generate bit two is one)
		if ( bHasSecondaryUVs==TRUE && dwGenerateMode==2 )
		{
			for(; iElem>=0; iElem-- )
			{
				if( Declaration[iElem].Usage == GGDECLUSAGE_TEXCOORD && Declaration[iElem].UsageIndex>0 )
				{
					Declaration[iElem] = Declaration[iElem+1];
					wNumBytesPerVertex -= 8; // assumes 8 bytes = texture coord
				}
			}
			for( iElem=0; Declaration[iElem].Stream != End.Stream; iElem++ ) {}
		}

		// Update Mesh Semantics if does not have components
		bool bGiveMeNormals = false;
		bool bGiveMeDiffuse = false;
		bool bGiveMeTangents = false;
		bool bGiveMeBinormals = false;
		bool bGiveMeBlendWeights = false;
		bool bGiveMeBlendIndices = false;
		if( !bHasNormals && bNormals ) 
		{
			bGiveMeNormals = true;
			Declaration[iElem].Stream = 0;
			Declaration[iElem].Offset = wNumBytesPerVertex;
			Declaration[iElem].Type = GGDECLTYPE_FLOAT3;
			Declaration[iElem].Method = GGDECLMETHOD_DEFAULT;
			Declaration[iElem].Usage = GGDECLUSAGE_NORMAL;
			Declaration[iElem].UsageIndex = 0;
			Declaration[iElem+1] = End;
			wNumBytesPerVertex+=12;
			iElem++;
		}
		if( !bHasDiffuse && bDiffuse ) 
		{
			bGiveMeDiffuse = true;
			Declaration[iElem].Stream = 0;
			Declaration[iElem].Offset = wNumBytesPerVertex;
			Declaration[iElem].Type = D3DDECLTYPE_FLOAT1;
			Declaration[iElem].Method = GGDECLMETHOD_DEFAULT;
			Declaration[iElem].Usage = D3DDECLUSAGE_COLOR;
			Declaration[iElem].UsageIndex = 0;
			Declaration[iElem+1] = End;
			wNumBytesPerVertex+=4;
			iElem++;
		}
		if( !bHasTangents && bTangents ) 
		{
			bGiveMeTangents = true;
			Declaration[iElem].Stream = 0;
			Declaration[iElem].Offset = wNumBytesPerVertex;
			Declaration[iElem].Type = GGDECLTYPE_FLOAT3;
			Declaration[iElem].Method = GGDECLMETHOD_DEFAULT;
			Declaration[iElem].Usage = D3DDECLUSAGE_TANGENT;
			Declaration[iElem].UsageIndex = 0;
			Declaration[iElem+1] = End;
			wNumBytesPerVertex+=12;
			iElem++;
		}
		//if( !bHasBinormals && bTangents ) // leefix - 050906 - now more specific for darkshader corrections
		if( !bHasBinormals && bBinormals ) 
		{
			bGiveMeBinormals = true;
			Declaration[iElem].Stream = 0;
			Declaration[iElem].Offset = wNumBytesPerVertex;
			Declaration[iElem].Type = GGDECLTYPE_FLOAT3;
			Declaration[iElem].Method = GGDECLMETHOD_DEFAULT;
			Declaration[iElem].Usage = D3DDECLUSAGE_BINORMAL;
			Declaration[iElem].UsageIndex = 0;
			Declaration[iElem+1] = End;
			wNumBytesPerVertex+=12;
			iElem++;
		}
		DWORD dwOffsetToWeights = wNumBytesPerVertex;
		if( !bHasBlendWeights && bBones ) 
		{
			bGiveMeBlendWeights = true;
			Declaration[iElem].Stream = 0;
			Declaration[iElem].Offset = wNumBytesPerVertex;
			Declaration[iElem].Type = D3DDECLTYPE_FLOAT4;
			Declaration[iElem].Method = GGDECLMETHOD_DEFAULT;
			Declaration[iElem].Usage = GGDECLUSAGE_TEXCOORD;
			Declaration[iElem].UsageIndex = 1;
			Declaration[iElem+1] = End;
			wNumBytesPerVertex+=16;
			iElem++;
		}
		DWORD dwOffsetToIndices = wNumBytesPerVertex;
		if( !bHasBlendIndices && bBones ) 
		{
			bGiveMeBlendIndices = true;
			Declaration[iElem].Stream = 0;
			Declaration[iElem].Offset = wNumBytesPerVertex;
			Declaration[iElem].Type = D3DDECLTYPE_FLOAT4;
			Declaration[iElem].Method = GGDECLMETHOD_DEFAULT;
			Declaration[iElem].Usage = GGDECLUSAGE_TEXCOORD;
			Declaration[iElem].UsageIndex = 2;
			Declaration[iElem+1] = End;
			wNumBytesPerVertex+=16;
			iElem++;
		}

		// create mesh from new declaration
		LPGGMESH pTempMesh = NULL;
		HRESULT hr;
		if ( pMesh->dwVertexCount>65535 )
			hr = pDXMesh->CloneMesh( D3DXMESH_SYSTEMMEM | D3DXMESH_32BIT, Declaration, m_pD3D, &pTempMesh );
		else
			hr = pDXMesh->CloneMesh( D3DXMESH_SYSTEMMEM, Declaration, m_pD3D, &pTempMesh );
		if( SUCCEEDED( hr ) )
		{
			// free old mesh and switch to new cloned mesh
			SAFE_RELEASE( pDXMesh );
			pDXMesh = pTempMesh;

			// Compute any tangent basis data (no not attempt to correct mesh too much)
			if ( bGiveMeNormals || bGiveMeTangents || bGiveMeBinormals )
			{
				// leefix - 050906 - this 'used' to EAT UV1 & other data, giving back a basic POS+NORM+TEX+TANGENT mesh
				// leenote - 090217 - cannot use FixTangents on bone based model, it adds verts to mess up bone skin (and does not work well)
				//pDXMesh = ComputeTangentBasisEx ( pTempMesh, bGiveMeNormals, bGiveMeTangents, bGiveMeBinormals, true, false, true );
				pDXMesh = ComputeTangentBasisEx ( pTempMesh, bGiveMeNormals, bGiveMeTangents, bGiveMeBinormals, false, false, true );
			}

			// Fill blend data (weight and indices) if required 
			if ( bGiveMeBlendWeights || bGiveMeBlendIndices )
			{
				// fills mesh with additional data
				ComputeBoneDataInsideVertex ( pMesh, pDXMesh, dwOffsetToWeights, dwOffsetToIndices, wNumBytesPerVertex );

				// flag mesh as now being vertex skinned by a shader
				pMesh->bShaderBoneSkinning = true;
			}
		}

		// update sMesh with new DX mesh
		UpdateLocalMeshWithDXMesh ( pMesh, pDXMesh );

		// free usages
		SAFE_RELEASE(pDXMesh);
	}
	#endif
}

DARKSDK_DLL void GenerateExtraDataForMeshEx ( sMesh* pMesh, BOOL bNormals, BOOL bTangents, BOOL bBinormals, BOOL bDiffuse, BOOL bBones )
{
	GenerateExtraDataForMeshEx ( pMesh, bNormals, bTangents, bBinormals, bDiffuse, bBones, 0 );
}

DARKSDK_DLL void GenerateExtraDataForMesh ( sMesh* pMesh, BOOL bNormals, BOOL bTangents, BOOL bDiffuse, BOOL bBones )
{
	// leeadd - 050906 - added binormal seperation for U63 (for new darkshader)
	GenerateExtraDataForMeshEx ( pMesh, bNormals, bTangents, FALSE, bDiffuse, bBones );
}

DARKSDK_DLL void CopyReferencesToShaderEffects ( sMesh* pNewMesh, sMesh* pMesh )
{
	pNewMesh->bUseVertexShader = pMesh->bUseVertexShader;
	pNewMesh->pVertexShader = pMesh->pVertexShader;
	pNewMesh->pVertexDec = pMesh->pVertexDec;
	pNewMesh->bOverridePixelShader = pMesh->bOverridePixelShader;
	pNewMesh->pPixelShader = pMesh->pPixelShader;
	pNewMesh->pVertexShaderEffect = pMesh->pVertexShaderEffect;
	pNewMesh->bVertexShaderEffectRefOnly = true;
	pNewMesh->bShaderBoneSkinning = pMesh->bShaderBoneSkinning;
	pNewMesh->dwForceCPUAnimationMode = pMesh->dwForceCPUAnimationMode;
	strcpy ( pNewMesh->pEffectName, pMesh->pEffectName );
}

DARKSDK_DLL void CloneShaderEffects ( sMesh* pNewMesh, sMesh* pMesh )
{
	// Copy references as normal
	CopyReferencesToShaderEffects ( pNewMesh, pMesh );

	// Also make source a reference so effect cannot be deleted 9as now ised twice..)
	pMesh->bVertexShaderEffectRefOnly = true;
}

// buffer functions

DARKSDK_DLL bool CopyMeshDataToIndexBuffer ( sMesh* pMesh, IGGIndexBuffer* pIndexBufferRef, DWORD dwBufferOffset )
{
	#ifdef DX11
	//if(0) //pObject->bDynamic )
	//{
	//	// if VB created with USAGE_DYNAMIC
	//	D3D11_MAPPED_SUBRESOURCE resource;
	//	memset ( &resource, 0, sizeof ( resource ) );
	//	if ( FAILED ( m_pImmediateContext->Map ( pIndexBufferRef, 0, D3D11_MAP_WRITE_NO_OVERWRITE, 0, &resource ) ) ) return false;
	//	unsigned char* pIndices = (unsigned char*)resource.pData;
	//	memcpy ( &pIndices[ dwBufferOffset ], pMesh->pIndices, sizeof ( WORD ) * pMesh->dwIndexCount );
	//	m_pImmediateContext->Unmap ( pIndexBufferRef, 0 );
	//}
	//else
	{
		// if VB created with USAGE_DEFAULT (fast GPU)
		D3D11_BOX box;
		box.left = dwBufferOffset;
		box.right = dwBufferOffset + ( sizeof ( WORD ) * pMesh->dwIndexCount );
		box.top = 0;
		box.bottom = 1;
		box.front = 0;
		box.back = 1;
		m_pImmediateContext->UpdateSubresource ( pIndexBufferRef, 0, &box, pMesh->pIndices, 0, 0 );
	}
	#else
	// copy index data from mesh to buffer
	WORD* pIndices = NULL;
	pIndexBufferRef->Lock ( 0, 0, ( VOID** ) &pIndices, 0 );

	// copy across to the buffer
	memcpy ( &pIndices[ dwBufferOffset ], pMesh->pIndices, sizeof ( WORD ) * pMesh->dwIndexCount );

	// unlock the index buffer
	pIndexBufferRef->Unlock ( );
	#endif

	// all went okay
	return true;
}

DARKSDK_DLL bool CopyMeshDataToDWORDIndexBuffer ( sMesh* pMesh, IGGIndexBuffer* pIndexBufferRef, DWORD dwBufferOffset )
{
	#ifdef DX11
	//if(0) //pObject->bDynamic )
	//{
	//	D3D11_MAPPED_SUBRESOURCE resource;
	//	memset ( &resource, 0, sizeof ( resource ) );
	//	if ( FAILED ( m_pImmediateContext->Map ( pIndexBufferRef, 0, D3D11_MAP_WRITE_NO_OVERWRITE, 0, &resource ) ) ) 
	//		return false;
	//
	//	// copy across WORD source data to the DWORD buffer
	//	unsigned char* pIndices = (unsigned char*)resource.pData;
	//	DWORD dwIter = dwBufferOffset;
	//	for ( DWORD i=0; i<pMesh->dwIndexCount; i++ )
	//		pIndices[ dwIter++ ] = pMesh->pIndices [ i ];
	//
	//	m_pImmediateContext->Unmap ( pIndexBufferRef, 0 );
	//}
	#else
	// copy index data from mesh to buffer
	DWORD* pIndices = NULL;
	pIndexBufferRef->Lock ( 0, 0, ( VOID** ) &pIndices, 0 );

	// copy across WORD source data to the DWORD buffer
	DWORD dwIter = dwBufferOffset;
	for ( DWORD i=0; i<pMesh->dwIndexCount; i++ )
		pIndices[ dwIter++ ] = pMesh->pIndices [ i ];

	// unlock the index buffer
	pIndexBufferRef->Unlock ( );
	#endif

	// all went okay
	return true;
}

DARKSDK_DLL bool CopyDWORDMeshDataToDWORDIndexBuffer ( sMesh* pMesh, IGGIndexBuffer* pIndexBufferRef, DWORD dwBufferOffset )
{
	#ifdef DX11
	#else
	// copy index data from mesh to buffer
	DWORD* pIndices = NULL;
	pIndexBufferRef->Lock ( 0, 0, ( VOID** ) &pIndices, 0 );

	// copy across DWORD source data to the DWORD buffer
	DWORD dwIter = dwBufferOffset;
	DWORD* pDWORDPtr = (DWORD*)pMesh->pIndices;
	for ( DWORD i=0; i<pMesh->dwIndexCount; i++ )
		pIndices[ dwIter++ ] = pDWORDPtr [ i ];

	// unlock the index buffer
	pIndexBufferRef->Unlock ( );
	#endif

	// all went okay
	return true;
}

DARKSDK_DLL bool CopyMeshDataToVertexBufferSameFVF ( sMesh* pMesh, IGGVertexBuffer* pVertexBufferRef, DWORD dwBufferOffset )
{
	#ifdef DX11
	// lock the vertex buffer
	//if(0) //pObject->bDynamic )
	//{
	//	// for VB with USAGE_DYNAMIC
	//	D3D11_MAPPED_SUBRESOURCE resource;
	//	memset ( &resource, 0, sizeof ( resource ) );
	//	if ( FAILED ( m_pImmediateContext->Map ( pVertexBufferRef, 0, D3D11_MAP_WRITE_NO_OVERWRITE, 0, &resource ) ) ) return false;
	//	unsigned char* pfData = (unsigned char*)resource.pData;
	//	// copy across to the buffer
	//	memcpy ( &pfData[ dwBufferOffset ], pMesh->pVertexData, pMesh->dwFVFSize * pMesh->dwVertexCount );
	//	m_pImmediateContext->Unmap ( pVertexBufferRef, 0 );
	//}
	//else
	{
		// for VB with USAGE_DEFAULT
		D3D11_BOX box;
		box.left = dwBufferOffset;
		box.right = dwBufferOffset + ( pMesh->dwFVFSize * pMesh->dwVertexCount );
		box.top = 0;
		box.bottom = 1;
		box.front = 0;
		box.back = 1;
		m_pImmediateContext->UpdateSubresource ( pVertexBufferRef, 0, &box, pMesh->pVertexData, 0, 0 );
	}
	#else
	// lock the vertex buffer
	float* pfData = NULL;
	if ( FAILED ( pVertexBufferRef->Lock ( 0, 0, ( VOID** ) &pfData, 0 ) ) )
		return false;

	// copy across to the buffer
	memcpy ( &pfData[ dwBufferOffset ], pMesh->pVertexData, pMesh->dwFVFSize * pMesh->dwVertexCount );

	// unlock the vertex buffer
	pVertexBufferRef->Unlock ( );
	#endif

	// all went okay
	return true;
}

DARKSDK_DLL bool CopyIndexBufferToMeshData ( sMesh* pMesh, IGGIndexBuffer* pIndexBufferRef, DWORD dwBufferOffset )
{
	#ifdef DX11
	#else
	// copy index data from mesh to buffer
	WORD* pIndices = NULL;
	pIndexBufferRef->Lock ( 0, 0, ( VOID** ) &pIndices, 0 );

	// copy across to the buffer
	memcpy ( pMesh->pIndices, &pIndices[ dwBufferOffset ], sizeof ( WORD ) * pMesh->dwIndexCount );

	// unlock the index buffer
	pIndexBufferRef->Unlock ( );
	#endif

	// all went okay
	return true;
}

DARKSDK_DLL bool CopyVertexBufferToMeshDataSameFVF ( sMesh* pMesh, IGGVertexBuffer* pVertexBufferRef, DWORD dwBufferOffset )
{
	#ifdef DX11
	#else
	// lock the vertex buffer
	float* pfData = NULL;
	if ( FAILED ( pVertexBufferRef->Lock ( 0, 0, ( VOID** ) &pfData, 0 ) ) )
		return false;

	// copy across to the buffer
	memcpy ( pMesh->pVertexData, &pfData[ dwBufferOffset ], pMesh->dwFVFSize * pMesh->dwVertexCount );

	// unlock the vertex buffer
	pVertexBufferRef->Unlock ( );
	#endif

	// all went okay
	return true;
}

DARKSDK_DLL bool CopyIndexMeshData ( sMesh* pDstMesh, sMesh* pSrcMesh, DWORD dwOffset, DWORD dwIndexCount )
{
	// copy across
	memcpy ( pDstMesh->pIndices + dwOffset, pSrcMesh->pIndices, sizeof(WORD) * dwIndexCount );
	return true;
}

DARKSDK_DLL bool IncrementIndexMeshData ( sMesh* pDstMesh, DWORD dwOffset, DWORD dwIndexCount, DWORD dwIncrement )
{
	// increment index data (used when adding a mesh to end of another mesh)
	WORD* pIndexData = pDstMesh->pIndices + dwOffset;
	for ( int iIndex=0; iIndex<(int)dwIndexCount; iIndex++ )
	{
		*pIndexData = (WORD)(*pIndexData + dwIncrement);
		pIndexData++;
	}
	return true;
}

DARKSDK_DLL bool CopyVertexMeshDataSameFVF ( sMesh* pDstMesh, sMesh* pSrcMesh, DWORD dwOffset, DWORD dwVertexCount )
{
	// copy across
	memcpy ( pDstMesh->pVertexData + ( dwOffset * pSrcMesh->dwFVFSize ), pSrcMesh->pVertexData, pSrcMesh->dwFVFSize * dwVertexCount );
	return true;
}

DARKSDK_DLL void SplitMeshSide ( int iSide, sMesh* pMesh, sMesh* pSplitMesh )
{
	// determine direction of side
	GGVECTOR3 vecDirection;
	switch ( iSide )
	{
		case 0 : vecDirection = GGVECTOR3 ( 0.0f, 0.0f, 1.0f );
		case 1 : vecDirection = GGVECTOR3 ( 0.0f, 0.0f,-1.0f );
		case 2 : vecDirection = GGVECTOR3 ( 0.0f,-1.0f, 0.0f );
		case 3 : vecDirection = GGVECTOR3 ( 0.0f, 1.0f, 0.0f );
		case 4 : vecDirection = GGVECTOR3 (-1.0f, 0.0f, 0.0f );
		case 5 : vecDirection = GGVECTOR3 ( 1.0f, 0.0f, 0.0f );
	}

	// copy index data from mesh to single-mesh
	BYTE* pVertexData = (BYTE*)pMesh->pVertexData;
	BYTE* pNormalData = pVertexData + ( sizeof(float)*3 );
	WORD* pIndexData = (WORD*)pMesh->pIndices;

	// transform vertex data by world matrix of frame
	for ( DWORD i=0; i<pMesh->dwIndexCount; i+=3 )
	{
		// get poly vertex normals
		DWORD v0 = pIndexData[i+0];
		DWORD v1 = pIndexData[i+1];
		DWORD v2 = pIndexData[i+2];
		GGVECTOR3* pNormal0 = (GGVECTOR3*)(pNormalData+(v0*pMesh->dwFVFSize));
		GGVECTOR3* pNormal1 = (GGVECTOR3*)(pNormalData+(v1*pMesh->dwFVFSize));
		GGVECTOR3* pNormal2 = (GGVECTOR3*)(pNormalData+(v2*pMesh->dwFVFSize));
		
		// work out poly normal
		GGVECTOR3 vecPolyNormal = *pNormal0 + *pNormal1 + *pNormal2;
		GGVec3Normalize ( &vecPolyNormal, &vecPolyNormal );

		// if facing node side, split it
		if ( GGVec3Dot ( &vecPolyNormal, &vecDirection ) > 0.0f )
		{
			// hide polygon fpr test
			*((GGVECTOR3*)(pVertexData+(v1*pMesh->dwFVFSize))) = *((GGVECTOR3*)(pVertexData+(v0*pMesh->dwFVFSize)));
			*((GGVECTOR3*)(pVertexData+(v2*pMesh->dwFVFSize))) = *((GGVECTOR3*)(pVertexData+(v0*pMesh->dwFVFSize)));
		}
	}
}

DARKSDK_DLL bool GetFrameCount ( sFrame* pFrame, int* piCount )
{
	// check if the frame and counter is okay is ok
	SAFE_MEMORY ( pFrame );
	SAFE_MEMORY ( piCount );

	// increment the count
	//*piCount += 1;
	// get frames for the sibling and child
	//GetFrameCount ( pFrame->pChild,   piCount );
	//GetFrameCount ( pFrame->pSibling, piCount );

	// avoids recirsion issue
	sFrame* pThis = pFrame;
	while ( pThis )
	{
		*piCount += 1;
		GetFrameCount ( pThis->pChild, piCount );
		pThis = pThis->pSibling;
	}

	return true;
}

DARKSDK_DLL bool BuildFrameList ( sFrame** pFrameList, sFrame* pFrame, int* iStart )
{
	// check if the pointers are valid
	SAFE_MEMORY ( pFrameList );
	SAFE_MEMORY ( pFrame );
	SAFE_MEMORY ( iStart );

	//pFrameList [ *iStart ] = pFrame;
	//pFrame->iID = *iStart;
	//*iStart += 1;
	// keep on calling build, we need to run through all items in the list
	//BuildFrameList ( pFrameList, pFrame->pChild, iStart );
	//BuildFrameList ( pFrameList, pFrame->pSibling, iStart );

	// avoids recirsion issue
	sFrame* pThis = pFrame;
	while ( pThis )
	{
		pFrameList [ *iStart ] = pThis;
		pThis->iID = *iStart;
		*iStart += 1;

		// and next down to tackle children (if any)
		BuildFrameList ( pFrameList, pThis->pChild, iStart );

		// next one
		pThis = pThis->pSibling;
	}

	return true;
}

DARKSDK_DLL bool GetMeshCount ( sFrame* pFrame, int* piCount )
{
	// check if the pointers are valids
	SAFE_MEMORY ( pFrame );
	SAFE_MEMORY ( piCount );

	// see if we have a mesh
	//if ( pFrame->pMesh ) *piCount += 1;
	//GetMeshCount ( pFrame->pChild,   piCount );
	//GetMeshCount ( pFrame->pSibling, piCount );

	// avoids recirsion issue
	sFrame* pThis = pFrame;
	while ( pThis )
	{
		if ( pThis->pMesh ) *piCount += 1;
		GetMeshCount ( pThis->pChild, piCount );
		pThis = pThis->pSibling;
	}

	return true;
}

DARKSDK_DLL bool BuildMeshList ( sMesh** pMeshList, sFrame* pFrame, int* iStart )
{
	// check if the pointers are valid
	SAFE_MEMORY ( pMeshList );
	SAFE_MEMORY ( pFrame );
	SAFE_MEMORY ( iStart );

	// see if we have a mesh
	//if ( pFrame->pMesh )
	//{
	//	pMeshList [ *iStart ] = pFrame->pMesh;
	//	pFrame->iID = *iStart;
	//	*iStart += 1;
	//}
	//BuildMeshList ( pMeshList, pFrame->pChild, iStart );
	//BuildMeshList ( pMeshList, pFrame->pSibling, iStart );

	// recursive causes stack overflow on large linklists, do old fashioned way
	// delete all pointers which will in turn delete all sub frames
	// keep on calling build, we need to run through all items in the list
	//BuildMeshList ( pMeshList, pFrame->pChild, iStart );
	//BuildMeshList ( pMeshList, pFrame->pSibling, iStart );
	sFrame* pThis = pFrame;
	while ( pThis )
	{
		// see if we have a mesh
		if ( pThis->pMesh )
		{
			pMeshList [ *iStart ] = pThis->pMesh;
			pThis->iID = *iStart;
			*iStart += 1;
		}

		// and next down to tackle children (if any)
		BuildMeshList ( pMeshList, pThis->pChild, iStart );

		// next one
		pThis = pThis->pSibling;
	}

	return true;
}

DARKSDK_DLL sFrame* FindFrame ( char* szName, sFrame* pFrame )
{
	// finds a frame with a given name, this function will
	// search through any sub frames of the frame passed in

	// make sure the frame is valid
	if ( !pFrame )
		return NULL;

	// if no name is specified return this frame
	if ( !szName )
		return pFrame;

	// set up a sub frame for more searches
	sFrame* pSubFrame = NULL;

	// compare names and return if exact match
	if ( pFrame->szName != NULL && !strcmp ( szName, pFrame->szName ) )
		return pFrame;

	// see if there is a child frame we can check
	if ( pFrame->pChild )
	{
		// see if we have a match with the child frame
		if ( ( pSubFrame = FindFrame ( szName, pFrame->pChild ) ) != NULL )
			return pSubFrame;
	}

	// see if there is a sibling frame
	if ( pFrame->pSibling )
	{
		// see if we have a match with the sibling frame
		if ( ( pSubFrame = FindFrame ( szName, pFrame->pSibling ) ) != NULL )
			return pSubFrame;
	}

	// no match found in the hiearachy
	return NULL;
}

DARKSDK_DLL sAnimation* FindAnimation ( sObject* pObject, sFrame* pOriginalFrame )
{
	// use original frame name to find pAnim and duplicate for new frame
	if ( pObject->pAnimationSet )
	{
		sAnimation* pCurrentAnim = pObject->pAnimationSet->pAnimation;
		if ( pCurrentAnim )
		{
			// go through all anims
			while ( pCurrentAnim )
			{
				if ( _stricmp ( pCurrentAnim->szName, pOriginalFrame->szName )==NULL )
				{
					// Found animation
					return pCurrentAnim;
				}
				pCurrentAnim = pCurrentAnim->pNext;
			}
		}
	}

	// could not find
	return NULL;
}

DARKSDK_DLL sAnimation* CopyAnimation ( sAnimation* pCurrentAnim, LPSTR szNewName )
{
	// found original anim data - copy it
	sAnimation* pNewAnim = new sAnimation;

	// copy over values
	strcpy ( pNewAnim->szName,		  szNewName );
	pNewAnim->bLoop					= pCurrentAnim->bLoop;
	pNewAnim->bLinear				= pCurrentAnim->bLinear;
	pNewAnim->dwNumPositionKeys		= pCurrentAnim->dwNumPositionKeys;
	pNewAnim->dwNumRotateKeys		= pCurrentAnim->dwNumRotateKeys;
	pNewAnim->dwNumScaleKeys		= pCurrentAnim->dwNumScaleKeys;
	pNewAnim->dwNumMatrixKeys		= pCurrentAnim->dwNumMatrixKeys;

	// create arrays and copy data
	pNewAnim->pPositionKeys = new sPositionKey[pCurrentAnim->dwNumPositionKeys];
	memcpy ( pNewAnim->pPositionKeys, pCurrentAnim->pPositionKeys, sizeof(sPositionKey)*pCurrentAnim->dwNumPositionKeys );
	pNewAnim->pRotateKeys = new sRotateKey[pCurrentAnim->dwNumRotateKeys];
	memcpy ( pNewAnim->pRotateKeys, pCurrentAnim->pRotateKeys, sizeof(sRotateKey)*pCurrentAnim->dwNumRotateKeys );
	pNewAnim->pScaleKeys = new sScaleKey[pCurrentAnim->dwNumScaleKeys];
	memcpy ( pNewAnim->pScaleKeys, pCurrentAnim->pScaleKeys, sizeof(sScaleKey)*pCurrentAnim->dwNumScaleKeys );
	pNewAnim->pMatrixKeys = new sMatrixKey[pCurrentAnim->dwNumMatrixKeys];
	memcpy ( pNewAnim->pMatrixKeys, pCurrentAnim->pMatrixKeys, sizeof(sMatrixKey)*pCurrentAnim->dwNumMatrixKeys );

	// add new anim to existing list
	while ( pCurrentAnim->pNext ) pCurrentAnim = pCurrentAnim->pNext;
	pCurrentAnim->pNext = pNewAnim;

	return pNewAnim;
}

DARKSDK_DLL bool SortAnimationPositionByTime ( sAnimation* pAnim, bool bDoTheCostlySort )
{
	// leefix - 270203 - some animation data is not time sorted (required for keyframe finder)
	SAFE_MEMORY ( pAnim->pPositionKeys );

	// store the number of keys
	DWORD dwNumKeys = pAnim->dwNumPositionKeys;

	if ( bDoTheCostlySort == true )
	{
		// bubble sort into time ascending order (or key-frame select gets messed up)
		for ( int iKeyA = 0; iKeyA < ( int ) dwNumKeys; iKeyA++ )
		{
			for ( int iKeyB = iKeyA; iKeyB < ( int ) dwNumKeys; iKeyB++ )
			{
				if ( iKeyA!=iKeyB )
				{
					if ( pAnim->pPositionKeys [ iKeyB ].dwTime < pAnim->pPositionKeys [ iKeyA ].dwTime ) 
					{
						// swap A and B
						sPositionKey sStoreA = pAnim->pPositionKeys [ iKeyA ];
						pAnim->pPositionKeys [ iKeyA ] = pAnim->pPositionKeys [ iKeyB ];
						pAnim->pPositionKeys [ iKeyB ] = sStoreA;
					}
				}
			}
		}
	}

	// work out interpolation data after keyframes sorted
	if ( dwNumKeys > 1 )
	{
		for ( int iKey = 0; iKey < ( int ) dwNumKeys - 1; iKey++ )
		{
			DWORD dwTime;

			pAnim->pPositionKeys [ iKey ].vecPosInterpolation = pAnim->pPositionKeys [ iKey + 1 ].vecPos - pAnim->pPositionKeys [ iKey ].vecPos;
			dwTime                                            = pAnim->pPositionKeys [ iKey + 1 ].dwTime - pAnim->pPositionKeys [ iKey ].dwTime;
			
			if ( !dwTime )
				dwTime = 1;

			pAnim->pPositionKeys [ iKey ].vecPosInterpolation /= ( float ) dwTime;
		}
	}

	// success
	return true;
}

DARKSDK_DLL bool SortAnimationRotationByTime ( sAnimation* pAnim, bool bDoTheCostlySort )
{
	// leefix - 270203 - some animation data is not time sorted (required for keyframe finder)
	SAFE_MEMORY ( pAnim->pRotateKeys );

	// store the number of keys
	DWORD dwNumKeys = pAnim->dwNumRotateKeys;
	
	if ( bDoTheCostlySort == true )
	{
		// bubble sort into time ascending order (or key-frame select gets messed up)
		for ( int iKeyA = 0; iKeyA < ( int ) dwNumKeys; iKeyA++ )
		{
			for ( int iKeyB = iKeyA; iKeyB < ( int ) dwNumKeys; iKeyB++ )
			{
				if ( iKeyA!=iKeyB )
				{
					if ( pAnim->pRotateKeys [ iKeyB ].dwTime < pAnim->pRotateKeys [ iKeyA ].dwTime ) 
					{
						// swap A and B
						sRotateKey sStoreA = pAnim->pRotateKeys [ iKeyA ];
						pAnim->pRotateKeys [ iKeyA ] = pAnim->pRotateKeys [ iKeyB ];
						pAnim->pRotateKeys [ iKeyB ] = sStoreA;
					}
				}
			}
		}
	}

	// success
	return true;
}

DARKSDK_DLL bool SortAnimationScaleByTime ( sAnimation* pAnim, bool bDoTheCostlySort )
{
	// leefix - 270203 - some animation data is not time sorted (required for keyframe finder)
	SAFE_MEMORY ( pAnim->pScaleKeys );

	// store the number of keys
	DWORD dwNumKeys = pAnim->dwNumScaleKeys;
	
	if ( bDoTheCostlySort == true )
	{
		// bubble sort into time ascending order (or key-frame select gets messed up)
		for ( int iKeyA = 0; iKeyA < ( int ) dwNumKeys; iKeyA++ )
		{
			for ( int iKeyB = iKeyA; iKeyB < ( int ) dwNumKeys; iKeyB++ )
			{
				if ( iKeyA!=iKeyB )
				{
					if ( pAnim->pScaleKeys [ iKeyB ].dwTime < pAnim->pScaleKeys [ iKeyA ].dwTime ) 
					{
						// swap A and B
						sScaleKey sStoreA = pAnim->pScaleKeys [ iKeyA ];
						pAnim->pScaleKeys [ iKeyA ] = pAnim->pScaleKeys [ iKeyB ];
						pAnim->pScaleKeys [ iKeyB ] = sStoreA;
					}
				}
			}
		}
	}

	// work out interpolation data after keyframes sorted
	if ( dwNumKeys > 1 )
	{
		for ( int iKey = 0; iKey < ( int ) dwNumKeys - 1; iKey++ )
		{
			DWORD dwTime = 0;

			pAnim->pScaleKeys [ iKey ].vecScaleInterpolation = pAnim->pScaleKeys [ iKey + 1 ].vecScale - pAnim->pScaleKeys [ iKey ].vecScale;
			dwTime                                           = pAnim->pScaleKeys [ iKey + 1 ].dwTime   - pAnim->pScaleKeys [ iKey ].dwTime;
			
			if ( !dwTime )
				dwTime = 1;

			pAnim->pScaleKeys [ iKey ].vecScaleInterpolation /= ( float ) dwTime;
		}
	}

	// success
	return true;
}

DARKSDK_DLL bool SortAnimationMatrixByTime ( sAnimation* pAnim, bool bDoTheCostlySort )
{
	// store the number of keys
	DWORD dwNumKeys = pAnim->dwNumMatrixKeys;
	
	// bubble sort into time ascending order (or key-frame select gets messed up)
	if ( bDoTheCostlySort == true )
	{
		for ( int iKeyA = 0; iKeyA < ( int ) dwNumKeys; iKeyA++ )
		{
			for ( int iKeyB = iKeyA; iKeyB < ( int ) dwNumKeys; iKeyB++ )
			{
				if ( iKeyA!=iKeyB )
				{
					if ( pAnim->pMatrixKeys [ iKeyB ].dwTime < pAnim->pMatrixKeys [ iKeyA ].dwTime ) 
					{
						// swap A and B
						sMatrixKey sStoreA = pAnim->pMatrixKeys [ iKeyA ];
						pAnim->pMatrixKeys [ iKeyA ] = pAnim->pMatrixKeys [ iKeyB ];
						pAnim->pMatrixKeys [ iKeyB ] = sStoreA;
					}
				}
			}
		}
	}

	// work out interpolation data after keyframes sorted
	if ( dwNumKeys > 1 )
	{
		for ( int iKey = 0; iKey < ( int ) dwNumKeys - 1; iKey++ )
		{
			DWORD dwTime = 0;
			pAnim->pMatrixKeys [ iKey ].matInterpolation = pAnim->pMatrixKeys [ iKey + 1 ].matMatrix - pAnim->pMatrixKeys [ iKey ].matMatrix;
			dwTime = pAnim->pMatrixKeys [ iKey + 1 ].dwTime - pAnim->pMatrixKeys [ iKey ].dwTime;
			if ( !dwTime ) dwTime = 1;
			pAnim->pMatrixKeys [ iKey ].matInterpolation /= ( float ) dwTime;
		}
	}

	// success
	return true;
}

DARKSDK_DLL bool SortAnimationDataByTime ( sAnimation* pAnim, bool bCostlySort )
{
	// sort position, rotation and scale data and calc interpolations
	SortAnimationPositionByTime	( pAnim, bCostlySort );
	SortAnimationRotationByTime	( pAnim, bCostlySort );
	SortAnimationScaleByTime	( pAnim, bCostlySort );

	// 060718 - need to organise matrix keyframes too
	SortAnimationMatrixByTime	( pAnim, bCostlySort );

	// success
	return true;
}

bool MapFramesToAnimations ( sObject* pObject, bool bCostlySort )
{
	// go through the animation and find the frames which are used
	// we then store a pointer to the frame in the anim structure

	// clear old frame anim references (can cause crash of old ptr exists)
	for (int iFrame = 0; iFrame < pObject->iFrameCount; iFrame++)
	{
		sFrame* pFrame = pObject->ppFrameList[iFrame];
		if (pFrame)
		{
			pFrame->pAnimRef = NULL;
		}
	}

	// leefix - 171203 - for MD*, we can first for sublist count
	pObject->fAnimTotalFrames = 0.0f;
	for ( int iMesh=0; iMesh < pObject->iMeshCount; iMesh++ )
	{
		sMesh* pMesh = pObject->ppMeshList [ iMesh ];
		if ( pMesh )
		{
			float fNewMax = ( float ) pMesh->dwSubMeshListCount;
			if ( fNewMax > pObject->fAnimTotalFrames )
			{
				pObject->fAnimTotalFrames = fNewMax;
				pObject->fAnimFrameEnd = fNewMax;
			}
		}
	}

	// check the memory we need to access
	SAFE_MEMORY ( pObject );
	SAFE_MEMORY ( pObject->pAnimationSet );
	SAFE_MEMORY ( pObject->pAnimationSet->pAnimation );

	// setup some local variables
	sAnimationSet* pAnimSet = NULL;
	sAnimation*    pAnim    = NULL;

	// LEEFIX - 171203 - this cannot work as some MD* do not have 'pAnimationSet'
	if ( !pObject->pAnimationSet->pAnimation->bBoneType )
	{
		pObject->fAnimTotalFrames = ( float ) pObject->pAnimationSet->pAnimation->iBoneFrameA - 1;
		pObject->fAnimFrameEnd    = pObject->fAnimTotalFrames;
		return true;
	}

	// get a pointer to the animation set
	pAnimSet = pObject->pAnimationSet;

	// get total length of animation
	DWORD dwTotalLength=0;

	// we need to run through all of the set
	LONG lStartPerf = timeGetTime();
	#ifdef WICKEDENGINE
	// Wicked stores ALL the needed animations for the object in the FIRST animset only.
	#else
	while ( pAnimSet != NULL )
	#endif
	{
		// get a pointer to the animation data
		pAnim = pAnimSet->pAnimation;

		// run through all animation sequences
		int iCountMe = 0;
		LONG lStartPerf2 = timeGetTime();
		while ( pAnim != NULL )
		{
			// scans all animation data and creates the interpolation vectors between all keyframes (vital)
			if ( pAnim ) SortAnimationDataByTime ( pAnim, bCostlySort );

			// find the frame which matches the animation
			pAnim->pFrame = FindFrame ( pAnim->szName, pObject->pFrame );	// find matching frame

			// and also associate frame with anim, so new wicked engine can find internal animation structure
			if ( pAnim->pFrame ) pAnim->pFrame->pAnimRef = pAnim;

			// next
			pAnim         = pAnim->pNext;									// move to next animation sequence
			iCountMe++;
		}
		lStartPerf2 = timeGetTime() - lStartPerf2;

		// update total length
		if ( pAnimSet->ulLength > dwTotalLength ) dwTotalLength = pAnimSet->ulLength;

		#ifdef WICKEDENGINE
		// Wicked stores ALL the needed animations for the object in the FIRST animset only.
		#else
		// move to next animation set
		pAnimSet = pAnimSet->pNext;
		#endif
	}
	lStartPerf = timeGetTime() - lStartPerf;

	// store total frames in object
	pObject->fAnimTotalFrames = (float)dwTotalLength;
	pObject->fAnimFrameEnd = pObject->fAnimTotalFrames;

	// return back to caller
	return true;
}

void InitOneMeshFramesToBones ( sMesh* pMesh )
{
	// create the frame mapping matrix array and clear out
	pMesh->pFrameRef = new sFrame* [ pMesh->dwBoneCount ];
	pMesh->pFrameMatrices = new GGMATRIX* [ pMesh->dwBoneCount ];

	// set all matrix pointers to null
	for ( int i = 0; i < ( int ) pMesh->dwBoneCount; i++ )
	{
		pMesh->pFrameRef [ i ] = NULL;
		pMesh->pFrameMatrices [ i ] = NULL;
	}
}

DARKSDK_DLL bool InitFramesToBones ( sMesh** pMeshList, int iMeshCount )
{
	// check mesh list
	SAFE_MEMORY ( *pMeshList );

	// get first mesh
	for ( int iMesh=0; iMesh < iMeshCount; iMesh++ )
	{
		sMesh* pMesh = pMeshList [ iMesh ];
		if ( pMesh )
		{
			// create the frame mapping matrix array and clear out
			InitOneMeshFramesToBones ( pMesh );
		}
	}

	// okay
	return true;
}

bool MapFramesToBones ( sMesh** pMesh, sFrame *pFrame, int iCount )
{
	// check mesh and frame
	SAFE_MEMORY ( *pMesh );
	SAFE_MEMORY ( pFrame );

	// current position
	int iPos = 0;

	// get first mesh
	sMesh* pMain = pMesh [ iPos ];

	// scan through meshes looking for bone matches
	if ( pMain != NULL )
	{
		// run through all meshes in list
		while ( 1 )
		{
			// get mesh and advance
			pMain = pMesh [ iPos++ ];

			// only update if we are dealing with bones
			if ( pMain->dwBoneCount )
			{
				// get list of bone names
				for ( DWORD i = 0; i < pMain->dwBoneCount; i++ )
				{
					// find the frame which matches the bone
					if ( strcmp ( pFrame->szName, pMain->pBones [ i ].szName ) == 0 )
					{
						// leeadd - 180204 - also store reference to frame (for bone-anim model limb based collision)
						pMain->pFrameRef [ i ] = pFrame;

						// lee - 021114 - record WHICH bone is referenced so UpdateFrame can use SkinOffset local transform matrix from bone
						pFrame->pmatBoneLocalTransform = &pMain->pBones [ i ].matTranslation;

						// store the matrix
						pMain->pFrameMatrices [ i ] = &pFrame->matCombined;

						// done
						break;
					}
				}
			}

			if ( iPos == iCount )
				break;
		}
	}

	// scan through child frames
	MapFramesToBones ( pMesh, pFrame->pChild, iCount );
	
	// scan through sibling frames
	MapFramesToBones ( pMesh, pFrame->pSibling, iCount );

	return true;
}

bool MapOneMeshFramesToBones ( sMesh* pMain, sFrame* pFrame )
{
	// only update if we are dealing with bones
	if ( pMain->dwBoneCount )
	{
		// get list of bone names
		for ( DWORD i = 0; i < pMain->dwBoneCount; i++ )
		{
			// find the frame which matches the bone
			if ( strcmp ( pFrame->szName, pMain->pBones [ i ].szName ) == 0 )
			{
				pMain->pFrameRef [ i ] = pFrame;
				pMain->pFrameMatrices [ i ] = &pFrame->matCombined;
				break;
			}
		}
	}

	// scan through child frames
	if ( pFrame->pChild ) MapOneMeshFramesToBones ( pMain, pFrame->pChild );
	
	// scan through sibling frames
	if ( pFrame->pSibling ) MapOneMeshFramesToBones ( pMain, pFrame->pSibling );

	return true;
}

DARKSDK_DLL void UpdateObjectCamDistance ( sObject* pObject )
{
	// using mesh LOD style ONLY
	if ( pObject->iUsingWhichLOD!=-1000 )
	{
		// store current LOD index
		int iStoreLOD = pObject->iUsingWhichLOD;

		// handle any LOD assigning (only if not in transition)
		if ( pObject->iUsingOldLOD==-1 )
		{
			if ( pObject->position.fCamDistance < pObject->fLODDistance [ 0 ] || pObject->fLODDistance [ 0 ]==0 )
				pObject->iUsingWhichLOD = 0;
			else
			{
				if ( pObject->position.fCamDistance < pObject->fLODDistance [ 1 ] || pObject->fLODDistance [ 1 ]==0 )
					pObject->iUsingWhichLOD = 1;
				else
				{
					if ( pObject->position.fCamDistance < pObject->fLODDistanceQUAD || pObject->fLODDistanceQUAD==0 )
						pObject->iUsingWhichLOD = 2;
					else
						pObject->iUsingWhichLOD = 3;
				}
			}

			// leeadd - U71 - add transitional code
			if ( iStoreLOD != pObject->iUsingWhichLOD )
			{
				// LOD level has changed due to distance, so initiate transition effect (handled in objectmanager)
				pObject->iUsingOldLOD = iStoreLOD;
				pObject->fLODTransition = 0.0f;
			}
		}
	}
}

