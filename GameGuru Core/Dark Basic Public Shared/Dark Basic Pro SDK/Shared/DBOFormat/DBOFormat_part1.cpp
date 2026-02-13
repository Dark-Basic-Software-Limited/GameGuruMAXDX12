DARKSDK_DLL bool CreateSingleMeshFromObjectCore ( sMesh** ppMesh, sObject* pObject, int iLimbNumberOptional, int iIgnoreMode )
{
	// get untranslated world matrix of object (includes rotation and scale)
	CalculateObjectWorld ( pObject, NULL );

	// create a new mesh
	*ppMesh = new sMesh;

	// count total number of vertices and indexes in object
	DWORD dwTotalVertices=0;
	DWORD dwTotalIndices=0;

	// leefix - 210703 - any FVF
	bool bGotFVFFromFirstMesh=false;
	bool bVertexOnlyBuffer=false;
	DWORD dwNewMeshFVF=0;

	// leeadd - 151008 - u70 - meshes should all be trilist from this process
	int iNewPrimitiveType=GGPT_TRIANGLELIST;

	// leefix - 081208 - U71 - ensure if ANY mesh has no indices, we go for vertex only	
	for ( int iCurrentFrame = 0; iCurrentFrame < pObject->iFrameCount; iCurrentFrame++ )
	{
		sFrame* pFrame = pObject->ppFrameList [ iCurrentFrame ];
		if ( pFrame && (iLimbNumberOptional==-1 || iLimbNumberOptional==iCurrentFrame) )
		{
			sMesh* pFrameMesh = pFrame->pMesh;
			if ( pFrameMesh )
			{
				if ( pFrameMesh->dwIndexCount==0 )
					bVertexOnlyBuffer=true;

				// U75 - 010410 - moved here from belo as both pFrameMesh->iPrimitiveType and iNewPrimitiveType known here
				if ( pFrameMesh->iPrimitiveType != iNewPrimitiveType ) 
				{
					// leeadd - 151008 - u70 - use verts if not trilist
					bVertexOnlyBuffer=true;
				}
			}
		}
	}

	// run through all of the frames within the object
	for ( int iPass = 1; iPass <= 2; iPass++ )
	{
		for ( int iCurrentFrame = 0; iCurrentFrame < pObject->iFrameCount; iCurrentFrame++ )
		{
			// find frame within object
			sFrame* pFrame = pObject->ppFrameList [ iCurrentFrame ];
			if ( pFrame && (iLimbNumberOptional==-1 || iLimbNumberOptional==iCurrentFrame) )
			{
				// mesh within frame
				sMesh* pFrameMesh = pFrame->pMesh;
				if ( pFrameMesh )
				{
					// ignore modes
					if ( iIgnoreMode==1 )
					{
						// 1 - ignore all meshes that have a NO-CULL status (typically leaves, grass, etc)
						if ( pFrameMesh->bCull==false )
							continue;
					}
					if ( iIgnoreMode==2 )
					{
						// 2 - ignore all meshes that have been HIDDEN
						if ( pFrameMesh->bVisible==false )
							continue;
					}

					// get new mesh fvf
					if ( bGotFVFFromFirstMesh==false )
					{
						dwNewMeshFVF = pFrameMesh->dwFVF;
						if ( dwNewMeshFVF==0 )
						{
							dwNewMeshFVF = GGFVF_XYZ | GGFVF_NORMAL | GGFVF_TEX1;
						}
						bGotFVFFromFirstMesh = true;
					}

					// calculate world matrix of frame
					GGMATRIX matWorld = pFrame->matCombined * pObject->position.matObjectNoTran;
					if (iIgnoreMode == 11)
					{
						// 11 - account for the world transform pos/rot for when we want to shift the mesh position too
						matWorld = pFrame->matCombined * pObject->position.matWorld;
					}

					// convert mesh to standard temp mesh-format
					sMesh* pStandardMesh = new sMesh;
					MakeLocalMeshFromOtherLocalMesh ( pStandardMesh, pFrameMesh );
					//ConvertLocalMeshToFVF ( pStandardMesh, dwNewMeshFVF );
					//PE: Make sure to follow wicked special FVF.
					DARKSDK_DLL void ConvertLocalMeshToFVFBones(sMesh * pMesh, DWORD dwFVF, bool bForceRepack);
					ConvertLocalMeshToFVFBones(pStandardMesh, dwNewMeshFVF,false);
					// U75 - 010410 - this is redundant as the check is performed earlier in this function
					// leeadd - 081208 - U71 - if mesh has NO index data, all mesh must have NO index data, so switch to vertex only
					//if ( pFrameMesh->dwIndexCount==0 )
					//	bVertexOnlyBuffer=true;

					// just verts
					if ( bVertexOnlyBuffer==true ) ConvertLocalMeshToVertsOnly ( pStandardMesh, false ); 

					// pass one - count all verts/indexes
					if ( iPass==1 )
					{
						// total size of data in this mesh
						dwTotalVertices += pStandardMesh->dwVertexCount;
						dwTotalIndices += pStandardMesh->dwIndexCount;
					}

					// pass two - copy data to single mesh
					if ( iPass==2 )
					{
						// copy vertex data from mesh to single-mesh
						BYTE* pDestVertexData = (BYTE*)((*ppMesh)->pVertexData+(dwTotalVertices*pStandardMesh->dwFVFSize));
						BYTE* pDestNormalData = (BYTE*)((*ppMesh)->pVertexData+(dwTotalVertices*pStandardMesh->dwFVFSize)) + ( sizeof(float)*3 );
						if ( dwNewMeshFVF & GGFVF_XYZ )
							memcpy ( pDestVertexData, pStandardMesh->pVertexData, pStandardMesh->dwVertexCount * pStandardMesh->dwFVFSize );

						// copy index data from mesh to single-mesh
						WORD* pDestIndexData = NULL;
						if ( (*ppMesh)->pIndices )
						{
							pDestIndexData = (WORD*)((*ppMesh)->pIndices + dwTotalIndices);
							memcpy ( pDestIndexData, pStandardMesh->pIndices, pStandardMesh->dwIndexCount * sizeof(WORD) );
						}

						// transform vertex data by world matrix of frame
						for ( DWORD v=0; v<pStandardMesh->dwVertexCount; v++ )
						{
							// get pointer to vertex and normal
							GGVECTOR3* pVertex = (GGVECTOR3*)(pDestVertexData+(v*pStandardMesh->dwFVFSize));
							GGVECTOR3* pNormal = (GGVECTOR3*)(pDestNormalData+(v*pStandardMesh->dwFVFSize));

							// transform with current combined frame matrix
							if ( dwNewMeshFVF & GGFVF_XYZ )
							{
								GGVec3TransformCoord ( pVertex, pVertex, &matWorld );
							}
							if ( dwNewMeshFVF & GGFVF_NORMAL )
							{
								GGVec3TransformNormal ( pNormal, pNormal, &matWorld );
								GGVec3Normalize ( pNormal, pNormal );
							}
						}

						// increment index data to reference correct vertex area
						if ( pDestIndexData )
						{
							for ( DWORD i=0; i<pStandardMesh->dwIndexCount; i++ )
							{
								pDestIndexData[i] += (WORD)dwTotalVertices;
							}
						}

						// advance counters
						dwTotalVertices += pStandardMesh->dwVertexCount;
						dwTotalIndices += pStandardMesh->dwIndexCount;
					}

					// delete standard temp mesh
					SAFE_DELETE ( pStandardMesh );
				}
			}
		}

		// end of passes
		if ( iPass==1 )
		{
			// leefix - 280305 - if index list too big, switch to using pre vertex buffer
			if ( dwTotalIndices > 0x0000FFFF )
			{
				bVertexOnlyBuffer = true;
				dwTotalVertices = dwTotalIndices * 3;
				dwTotalIndices = 0;
				iPass--; //redo count!
			}

			// make vertex and index buffers for single mesh
			SetupMeshFVFData ( *ppMesh, dwNewMeshFVF, dwTotalVertices, dwTotalIndices, false );

			// reset counters to fill single mesh
			dwTotalVertices = 0;
			dwTotalIndices = 0;
		}
		if ( iPass==2 )
		{
			// setup mesh drawing properties when single mesh data transfered
			(*ppMesh)->iPrimitiveType   = iNewPrimitiveType;
			(*ppMesh)->iDrawVertexCount = dwTotalVertices;
			if ( dwTotalIndices==0 )
				(*ppMesh)->iDrawPrimitives  = dwTotalVertices/3;
			else
				(*ppMesh)->iDrawPrimitives  = dwTotalIndices/3;
		}
	}

	if ( iLimbNumberOptional!=-1 )
	{
		// for limb specific creations, also copy texture info over
		sFrame* pFrame = pObject->ppFrameList [ iLimbNumberOptional ];
		if ( pFrame )
		{
			sMesh* pMesh = pFrame->pMesh;
			if ( pMesh )
			{
				(*ppMesh)->dwTextureCount = pMesh->dwTextureCount; 
				(*ppMesh)->pTextures = new sTexture [ (*ppMesh)->dwTextureCount ]; 
				CloneInternalTextures ( (*ppMesh), pMesh );
			}
		}
	}
	else
	{
		// for whole object conversions, use first texture of master object
		if ( pObject->ppMeshList )
		{
			sMesh* pMesh = pObject->ppMeshList [ 0 ];
			if ( pMesh )
			{
				(*ppMesh)->dwTextureCount = pMesh->dwTextureCount; 
				(*ppMesh)->pTextures = new sTexture [ (*ppMesh)->dwTextureCount ]; 
				CloneInternalTextures ( (*ppMesh), pMesh );
			}
		}
	}

	// all went okay
	return true;
}

DARKSDK_DLL bool CreateSingleMeshFromObject ( sMesh** ppMesh, sObject* pObject, int iIgnoreMode )
{
	return CreateSingleMeshFromObjectCore ( ppMesh, pObject, -1, iIgnoreMode );
}

DARKSDK_DLL bool CreateSingleMeshFromLimb ( sMesh** ppMesh, sObject* pObject, int iLimbNumber, int iIgnoreMode )
{
	return CreateSingleMeshFromObjectCore ( ppMesh, pObject, iLimbNumber, iIgnoreMode );
}

/* new version but messes with FPSC assumtpions of vertex/index layout

  bool CreateSingleMeshFromObject ( sMesh** ppMesh, sObject* pObject )
{
	// get untranslated world matrix of object (includes rotation and scale)
	CalculateObjectWorld ( pObject, NULL );

	// create a new mesh
	*ppMesh = new sMesh;

	// count total number of vertices and indexes in object
	DWORD dwTotalVertices=0;

	// leefix - 210703 - any FVF
	bool bGotFVFFromFirstMesh=false;
	DWORD dwNewMeshFVF=0;

	// run through all of the frames within the object
	for ( int iPass = 1; iPass <= 2; iPass++ )
	{
		for ( int iCurrentFrame = 0; iCurrentFrame < pObject->iFrameCount; iCurrentFrame++ )
		{
			// find frame within object
			sFrame* pFrame = pObject->ppFrameList [ iCurrentFrame ];
			if ( pFrame )
			{
				// mesh within frame
				sMesh* pFrameMesh = pFrame->pMesh;
				if ( pFrameMesh )
				{
					// get new mesh fvf
					if ( bGotFVFFromFirstMesh==false )
					{
						dwNewMeshFVF = pFrameMesh->dwFVF;
						if ( dwNewMeshFVF==0 )
						{
							// cannot create nonFVF mesh at the moment
							SAFE_DELETE(*ppMesh);
							return false;
						}
						bGotFVFFromFirstMesh = true;
					}

					// calculate world matrix of frame
					GGMATRIX matWorld = pFrame->matCombined * pObject->position.matObjectNoTran;

					// convert mesh to standard temp mesh-format
					sMesh* pStandardMesh = new sMesh;
					MakeLocalMeshFromOtherLocalMesh ( pStandardMesh, pFrameMesh );
					ConvertLocalMeshToFVF ( pStandardMesh, dwNewMeshFVF );
					ConvertLocalMeshToVertsOnly ( pStandardMesh );

					// pass one - count all verts/indexes
					if ( iPass==1 )
					{
						// total size of data in this mesh
						dwTotalVertices += pStandardMesh->dwVertexCount;
					}

					// pass two - copy data to single mesh
					if ( iPass==2 )
					{
						// copy vertex data from mesh to single-mesh
						BYTE* pDestVertexData = (BYTE*)((*ppMesh)->pVertexData+(dwTotalVertices*pStandardMesh->dwFVFSize));
						BYTE* pDestNormalData = (BYTE*)((*ppMesh)->pVertexData+(dwTotalVertices*pStandardMesh->dwFVFSize)) + ( sizeof(float)*3 );
						if ( dwNewMeshFVF & GGFVF_XYZ )
							memcpy ( pDestVertexData, pStandardMesh->pVertexData, pStandardMesh->dwVertexCount * pStandardMesh->dwFVFSize );

						// transform vertex data by world matrix of frame
						for ( DWORD v=0; v<pStandardMesh->dwVertexCount; v++ )
						{
							// get pointer to vertex and normal
							GGVECTOR3* pVertex = (GGVECTOR3*)(pDestVertexData+(v*pStandardMesh->dwFVFSize));
							GGVECTOR3* pNormal = (GGVECTOR3*)(pDestNormalData+(v*pStandardMesh->dwFVFSize));

							// transform with current combined frame matrix
							if ( dwNewMeshFVF & GGFVF_XYZ )
							{
								GGVec3TransformCoord ( pVertex, pVertex, &matWorld );
							}
							if ( dwNewMeshFVF & GGFVF_NORMAL )
							{
								GGVec3TransformNormal ( pNormal, pNormal, &matWorld );
								GGVec3Normalize ( pNormal, pNormal );
							}
						}

						// advance counters
						dwTotalVertices += pStandardMesh->dwVertexCount;
					}

					// delete standard temp mesh
					SAFE_DELETE ( pStandardMesh );
				}
			}
		}

		// end of passes
		if ( iPass==1 )
		{
			// 010804 - verts only due to size
			SetupMeshFVFData ( *ppMesh, dwNewMeshFVF, dwTotalVertices, 0, false );

			// reset counters to fill single mesh
			dwTotalVertices = 0;
		}
		if ( iPass==2 )
		{
			// setup mesh drawing properties when single mesh data transfered
			(*ppMesh)->iPrimitiveType   = GGPT_TRIANGLELIST;
			(*ppMesh)->iDrawVertexCount = dwTotalVertices;
			(*ppMesh)->iDrawPrimitives  = dwTotalVertices/3;
		}
	}

	// all went okay
	return true;
}
*/

// DXMesh implementations

DARKSDK_DLL LPGGMESH LocalMeshToDXMesh ( sMesh* pMesh, CONST LPGGVERTEXELEMENT pDeclarationOverride, DWORD dwFVFOverride )
{
	// result var
	LPGGMESH pNewMesh = NULL;

	#ifdef DX11
	// No mesh functions in DX11!
	#else
	// create a mesh
	DWORD dwFaces = pMesh->iDrawPrimitives;
	DWORD dwVertices = pMesh->dwVertexCount;
	DWORD dwFVF = pMesh->dwFVF;
	LPGGMESH pOriginalMesh = NULL;
	IGGIndexBuffer* pIB = NULL;
	IGGVertexBuffer* pVB = NULL;

	// if too big, create a 32bit index buffer XMesh
	if ( dwFaces*3 > 0x0000FFFF )
	{
		// create dx mesh
		if ( dwFVF==0 )
			D3DXCreateMesh ( dwFaces, dwVertices, D3DXMESH_MANAGED | D3DXMESH_32BIT, pMesh->pVertexDeclaration, m_pD3D, &pOriginalMesh );
		else
			D3DXCreateMeshFVF ( dwFaces, dwVertices, D3DXMESH_MANAGED | D3DXMESH_32BIT, dwFVF, m_pD3D, &pOriginalMesh );

		// if failed to create dx mesh
		if ( pOriginalMesh==NULL)
			return NULL;

		// must prepare mesh index data as basic triangle list indices, not strips
		ConvertLocalMeshToTriList ( pMesh );

		// if original input mesh has no index data, create some for process
		if ( pMesh->dwIndexCount==0 )
		{
			pMesh->dwIndexCount = dwFaces * 3;
			DWORD* pDWORDPtr = new DWORD [ pMesh->dwIndexCount ];
			pMesh->pIndices = (WORD*)pDWORDPtr;
			for ( DWORD dwI=0; dwI<pMesh->dwIndexCount; dwI++ )
				pDWORDPtr [ dwI ] = dwI;
		}
		else
		{
			// if it does have data, it's in WORDs so need to re-create them as DWORDs
			pMesh->dwIndexCount = dwFaces * 3;
			DWORD* pDWORDPtr = new DWORD [ pMesh->dwIndexCount ];
			memset(pDWORDPtr,0,pMesh->dwIndexCount*sizeof(DWORD));
			for ( DWORD dwI=0; dwI<pMesh->dwIndexCount; dwI++ )
				pDWORDPtr [ dwI ] = pMesh->pIndices[dwI];
			// 281114 - changed to SAFE_DELETE_ARRAY
			SAFE_DELETE_ARRAY(pMesh->pIndices);
			pMesh->pIndices = (WORD*)pDWORDPtr;
		}

		// fill mesh with input mesh data
		pOriginalMesh->GetIndexBuffer(&pIB);
		pOriginalMesh->GetVertexBuffer(&pVB);
		if(pIB) CopyDWORDMeshDataToDWORDIndexBuffer ( pMesh, pIB, 0 );
		if(pVB) CopyMeshDataToVertexBufferSameFVF ( pMesh, pVB, 0 );
	}
	else
	{
		// support for 32bit meshes
		if ( dwVertices > 65535 )
		{
			// create LARGE dx mesh
			if ( dwFVF==0 )
				D3DXCreateMesh ( dwFaces, dwVertices, D3DXMESH_MANAGED | D3DXMESH_32BIT, pMesh->pVertexDeclaration, m_pD3D, &pOriginalMesh );
			else
				D3DXCreateMeshFVF ( dwFaces, dwVertices, D3DXMESH_MANAGED | D3DXMESH_32BIT, dwFVF, m_pD3D, &pOriginalMesh );
		}
		else
		{
			// create dx mesh
			if ( dwFVF==0 )
				D3DXCreateMesh ( dwFaces, dwVertices, D3DXMESH_MANAGED, pMesh->pVertexDeclaration, m_pD3D, &pOriginalMesh );
			else
				D3DXCreateMeshFVF ( dwFaces, dwVertices, D3DXMESH_MANAGED, dwFVF, m_pD3D, &pOriginalMesh );
		}

		// if failed to create dx mesh
		if ( pOriginalMesh==NULL)
			return NULL;

		// must prepare mesh index data as basic triangle list indices, not strips
		ConvertLocalMeshToTriList ( pMesh );

		// if original input mesh has no index data, create some for process
		if ( pMesh->dwIndexCount==0 )
		{
			pMesh->dwIndexCount = dwFaces * 3;
			pMesh->pIndices = new WORD [ pMesh->dwIndexCount ];
			for ( WORD dwI=0; dwI<(WORD)pMesh->dwIndexCount; dwI++ )
				pMesh->pIndices [ dwI ] = dwI;
		}

		// fill mesh with input mesh data
		pOriginalMesh->GetIndexBuffer(&pIB);
		pOriginalMesh->GetVertexBuffer(&pVB);
		if(pIB) CopyMeshDataToIndexBuffer ( pMesh, pIB, 0 );
		if(pVB) CopyMeshDataToVertexBufferSameFVF ( pMesh, pVB, 0 );
	}

	// modify mesh if vertex format overwrite in place
	if ( dwFVFOverride > 0 )
	{
		// When converting mesh to FVF standard
		pOriginalMesh->CloneMeshFVF ( 0, dwFVFOverride, m_pD3D, &pNewMesh );
		SAFE_RELEASE ( pOriginalMesh );
	}
	else if ( pDeclarationOverride!=NULL )
	{
		// When converting mesh to shader Declaration
		pOriginalMesh->CloneMesh ( 0, pDeclarationOverride, m_pD3D, &pNewMesh );
		SAFE_RELEASE ( pOriginalMesh );
	}
	else
	{
		// keep mesh - it is okay
		pNewMesh = pOriginalMesh;
	}

	// release buffers
	SAFE_RELEASE(pIB);
	SAFE_RELEASE(pVB);
	#endif

	// return DX mesh
	return pNewMesh;
}

DARKSDK_DLL void UpdateLocalMeshWithDXMesh ( sMesh* pMesh, LPGGMESH pDXMesh )
{
	// do not update if dxptr invalid
	if ( pDXMesh==NULL )
		return;

	#ifdef DX11
	// this mesh vx dxmesh - any way to avoid for DX11
	#else
	// If index count exceeds 16bit..
	bool bConvertIndexedDataToVertexOnly=false;
	DWORD dwVertexCount = pDXMesh->GetNumVertices();
	DWORD dwIndexCount = pDXMesh->GetNumFaces ( ) * 3;
	if ( dwIndexCount > 0 )
	{
		if ( dwIndexCount > 0x0000FFFF )
		{
			// create a vertex only mesh
			bConvertIndexedDataToVertexOnly=true;
			dwVertexCount = dwIndexCount;
			dwIndexCount = 0;
		}
	}

	// mesh can hold regular FVF and custom declarations
	if ( pDXMesh->GetFVF()==0 )
	{
		// now create new mesh data from new Declaration
		GGVERTEXELEMENT Declaration[MAX_FVF_DECL_SIZE];
		pDXMesh->GetDeclaration( Declaration );
		if ( SetupMeshDeclarationData ( pMesh, Declaration, pDXMesh->GetNumBytesPerVertex(), dwVertexCount, dwIndexCount )==false )
		{
			// need to fully restore the object mid-conversion
			return;
		}
	}
	else
	{
		// now create new mesh data from new FVF
		SetupMeshFVFData ( pMesh, pDXMesh->GetFVF(), dwVertexCount, dwIndexCount, false );
	}

	// get vertex and index buffer
	LPDIRECT3DVERTEXBUFFER9 m_pMeshVertexBuffer;
	LPDIRECT3DINDEXBUFFER9  m_pMeshIndexBuffer;
	pDXMesh->GetVertexBuffer ( &m_pMeshVertexBuffer );
	pDXMesh->GetIndexBuffer  ( &m_pMeshIndexBuffer );

	// copy dx mesh to dbpro mesh
	if ( bConvertIndexedDataToVertexOnly==true )
	{	
		// destination data
		BYTE* pDestVertPtr = (BYTE*)pMesh->pVertexData;

		// source data
		WORD* pIndices = NULL;
		if ( SUCCEEDED ( m_pMeshIndexBuffer->Lock ( 0, 0, ( VOID** ) &pIndices, 0 ) ) )
		{
			BYTE* pSrcVertexData = NULL;
			if ( SUCCEEDED ( m_pMeshVertexBuffer->Lock ( 0, 0, ( VOID** ) &pSrcVertexData, 0 ) ) )
			{
				// go through all faces data of dx mesh
				DWORD dwIndex = 0;
				for ( DWORD f=0; f<pDXMesh->GetNumFaces ( ); f++ )
				{
					// three vertx per face
					for ( int n=0; n<3; n++ )
					{
						// source data - index data gives us src vertex position
						DWORD dwBufferOffset = pIndices[ dwIndex++ ];

						// copy across to the dest buffer and advance to next vertex
						memcpy ( pDestVertPtr, pSrcVertexData+(dwBufferOffset*pMesh->dwFVFSize), pMesh->dwFVFSize );
						pDestVertPtr+=pMesh->dwFVFSize;
					}
				}

				// unlock the vertex buffer
				m_pMeshVertexBuffer->Unlock ( );
			}

			// unlock the index buffer
			m_pMeshIndexBuffer->Unlock ( );
		}
	}
	else
	{
		// fill mesh data from vertex and index buffers
		if(m_pMeshIndexBuffer) CopyIndexBufferToMeshData ( pMesh, m_pMeshIndexBuffer, 0 );
		if(m_pMeshVertexBuffer) CopyVertexBufferToMeshDataSameFVF ( pMesh, m_pMeshVertexBuffer, 0 );
	}

	// release buffers
	SAFE_RELEASE(m_pMeshVertexBuffer);
	SAFE_RELEASE(m_pMeshIndexBuffer);

	// when mesh changes, must flag it
	pMesh->bMeshHasBeenReplaced = true;
	#endif
}

DARKSDK_DLL void RestoreLocalMesh ( sMesh* pMesh )
{
	#ifdef DX11
	#else
	LPGGMESH pDXMesh = LocalMeshToDXMesh ( pMesh, NULL, pMesh->dwFVFOriginal );
	UpdateLocalMeshWithDXMesh ( pMesh, pDXMesh );
	// lee - 240306 - u6b5 - must not hold onto this if restored mesh
	SAFE_DELETE ( pMesh->pOriginalVertexData );
	SAFE_RELEASE(pDXMesh);
	#endif
}

DARKSDK_DLL void ConvertLocalMeshToFVF ( sMesh* pMesh, DWORD dwFVF )
{
	#ifdef DX11
	if ( pMesh->dwFVF != dwFVF )
	{
		// convert current mesh VB data to new dwFVF
		sOffsetMap offsetMap, offsetMapNew;
		GetFVFOffsetMap ( pMesh, &offsetMap );
		pMesh->dwFVF = dwFVF;
		GetFVFOffsetMap ( pMesh, &offsetMapNew );
		DWORD dwNewFVFSize = offsetMapNew.dwByteSize;
		DWORD dwNumberOfVertices = pMesh->dwVertexCount;
		DWORD dwNewVertexDataSize = dwNumberOfVertices * dwNewFVFSize;
		LPSTR pNewVertexData = new char[dwNewVertexDataSize];
		for ( int iCurrentVertex = 0; iCurrentVertex < (int)dwNumberOfVertices; iCurrentVertex++ )
		{
			if ( dwFVF & GGFVF_XYZ )
			{
				GGVECTOR3 vecPos = *(GGVECTOR3*)( ( float* ) pMesh->pVertexData + offsetMap.dwX + ( offsetMap.dwSize * iCurrentVertex ) );
				*(GGVECTOR3*)( ( float* ) pNewVertexData + offsetMapNew.dwX + ( offsetMapNew.dwSize * iCurrentVertex ) ) = vecPos;
			}
			if ( dwFVF & GGFVF_NORMAL )
			{
				GGVECTOR3 vecNorm = *(GGVECTOR3*)( ( float* ) pMesh->pVertexData + offsetMap.dwNX + ( offsetMap.dwSize * iCurrentVertex ) );
				*(GGVECTOR3*)( ( float* ) pNewVertexData + offsetMapNew.dwNX + ( offsetMapNew.dwSize * iCurrentVertex ) ) = vecNorm;
			}
			if ( dwFVF & GGFVF_DIFFUSE )
			{
				DWORD dwDiffuseColour = *(DWORD*)( ( float* ) pMesh->pVertexData + offsetMap.dwDiffuse + ( offsetMap.dwSize * iCurrentVertex ) );
				*(DWORD*)( ( float* ) pNewVertexData + offsetMapNew.dwDiffuse + ( offsetMapNew.dwSize * iCurrentVertex ) ) = dwDiffuseColour;
			}
			/* oops! GGFVF_TEX1 actually means ONE texture set, GGFVF_TEX2 means two texture sets (i.e. lightmapping)
			if ( dwFVF & GGFVF_TEX1 )
			{
				float fTex = *(float*)( ( float* ) pMesh->pVertexData + offsetMap.dwTU[0] + ( offsetMap.dwSize * iCurrentVertex ) );
				*(float*)( ( float* ) pNewVertexData + offsetMapNew.dwTU[0] + ( offsetMapNew.dwSize * iCurrentVertex ) ) = fTex;
			}
			if ( dwFVF & GGFVF_TEX2 )
			*/
			if ( dwFVF & GGFVF_TEX1 || dwFVF & GGFVF_TEX2 )
			{
				GGVECTOR2 vecTex = *(GGVECTOR2*)( ( float* ) pMesh->pVertexData + offsetMap.dwTU[0] + ( offsetMap.dwSize * iCurrentVertex ) );
				*(GGVECTOR2*)( ( float* ) pNewVertexData + offsetMapNew.dwTU[0] + ( offsetMapNew.dwSize * iCurrentVertex ) ) = vecTex;
			}
			if ( dwFVF & GGFVF_TEX2 )
			{
				GGVECTOR2 vecTex = *(GGVECTOR2*)( ( float* ) pMesh->pVertexData + offsetMap.dwTU[1] + ( offsetMap.dwSize * iCurrentVertex ) );
				*(GGVECTOR2*)( ( float* ) pNewVertexData + offsetMapNew.dwTU[1] + ( offsetMapNew.dwSize * iCurrentVertex ) ) = vecTex;
			}
		}
		SAFE_DELETE ( pMesh->pVertexData );
		pMesh->pVertexData = (BYTE*)pNewVertexData;
		pMesh->dwFVF = dwFVF;
		pMesh->dwFVFSize = dwNewFVFSize;
	}
	#else
	if ( pMesh->dwFVF != dwFVF )
	{
		LPGGMESH pDXMesh = LocalMeshToDXMesh ( pMesh, NULL, dwFVF );
		UpdateLocalMeshWithDXMesh ( pMesh, pDXMesh );
		SAFE_RELEASE(pDXMesh);
	}
	#endif
}
//#pragma optimize("", off)

DARKSDK_DLL void ConvertLocalMeshToFVFBones(sMesh* pMesh, DWORD dwFVF, bool bForceRepack)
{
#ifdef DX11
	if (pMesh->dwFVF != dwFVF || bForceRepack)
	{
		// convert current mesh VB data to new dwFVF
		sOffsetMap offsetMap, offsetMapNew;
		GetFVFOffsetMapFixedForBones(pMesh, &offsetMap);
		pMesh->dwFVF = dwFVF;
		GetFVFOffsetMapFixedForBones(pMesh, &offsetMapNew);
		DWORD dwNewFVFSize = offsetMapNew.dwByteSize;
		DWORD dwNumberOfVertices = pMesh->dwVertexCount;
		DWORD dwNewVertexDataSize = dwNumberOfVertices * dwNewFVFSize;
		LPSTR pNewVertexData = new char[dwNewVertexDataSize];
		for (int iCurrentVertex = 0; iCurrentVertex < (int)dwNumberOfVertices; iCurrentVertex++)
		{
			if (dwFVF & GGFVF_XYZ)
			{
				GGVECTOR3 vecPos = *(GGVECTOR3*)((float*)pMesh->pVertexData + offsetMap.dwX + (offsetMap.dwSize * iCurrentVertex));
				*(GGVECTOR3*)((float*)pNewVertexData + offsetMapNew.dwX + (offsetMapNew.dwSize * iCurrentVertex)) = vecPos;
			}
			if (dwFVF & GGFVF_NORMAL)
			{
				GGVECTOR3 vecNorm = *(GGVECTOR3*)((float*)pMesh->pVertexData + offsetMap.dwNX + (offsetMap.dwSize * iCurrentVertex));
				*(GGVECTOR3*)((float*)pNewVertexData + offsetMapNew.dwNX + (offsetMapNew.dwSize * iCurrentVertex)) = vecNorm;
			}
			if ((dwFVF & GGFVF_DIFFUSE) && offsetMap.dwDiffuse > 0 && offsetMapNew.dwDiffuse > 0 )
			{
				DWORD dwDiffuseColour = *(DWORD*)((float*)pMesh->pVertexData + offsetMap.dwDiffuse + (offsetMap.dwSize * iCurrentVertex));
				*(DWORD*)((float*)pNewVertexData + offsetMapNew.dwDiffuse + (offsetMapNew.dwSize * iCurrentVertex)) = dwDiffuseColour;
			}
			/* oops! GGFVF_TEX1 actually means ONE texture set, GGFVF_TEX2 means two texture sets (i.e. lightmapping)
			if ( dwFVF & GGFVF_TEX1 )
			{
				float fTex = *(float*)( ( float* ) pMesh->pVertexData + offsetMap.dwTU[0] + ( offsetMap.dwSize * iCurrentVertex ) );
				*(float*)( ( float* ) pNewVertexData + offsetMapNew.dwTU[0] + ( offsetMapNew.dwSize * iCurrentVertex ) ) = fTex;
			}
			if ( dwFVF & GGFVF_TEX2 )
			*/
			if ((dwFVF & GGFVF_TEX1 || dwFVF & GGFVF_TEX2))
			{
				GGVECTOR2 vecTex = *(GGVECTOR2*)((float*)pMesh->pVertexData + offsetMap.dwTU[0] + (offsetMap.dwSize * iCurrentVertex));
				*(GGVECTOR2*)((float*)pNewVertexData + offsetMapNew.dwTU[0] + (offsetMapNew.dwSize * iCurrentVertex)) = vecTex;

			}
			if ((dwFVF & GGFVF_TEX2) && offsetMap.dwTU[1] > 0 && offsetMapNew.dwTU[1] > 0)
			{
				GGVECTOR2 vecTex = *(GGVECTOR2*)((float*)pMesh->pVertexData + offsetMap.dwTU[1] + (offsetMap.dwSize * iCurrentVertex));
				*(GGVECTOR2*)((float*)pNewVertexData + offsetMapNew.dwTU[1] + (offsetMapNew.dwSize * iCurrentVertex)) = vecTex;
			}
		}
		SAFE_DELETE(pMesh->pVertexData);
		pMesh->pVertexData = (BYTE*)pNewVertexData;
		pMesh->dwFVF = dwFVF;
		pMesh->dwFVFSize = dwNewFVFSize;
	}
#else
	if (pMesh->dwFVF != dwFVF)
	{
		LPGGMESH pDXMesh = LocalMeshToDXMesh(pMesh, NULL, dwFVF);
		UpdateLocalMeshWithDXMesh(pMesh, pDXMesh);
		SAFE_RELEASE(pDXMesh);
	}
#endif
}

DARKSDK_DLL void ConvertLocalMeshToVertsOnly ( sMesh* pMesh, bool bIs32BitIndexData )
{
	// mesh 'can' store 32bit index data temporarily for later conversion to vertex only.
	// and should only use 32bit if going to do a vertex expanding (due to >16bit verts)
	bool b32BITIndexData=false;
	if ( pMesh->dwVertexCount > 0xFFFF || bIs32BitIndexData == true )
		b32BITIndexData=true;

	// ensure it is a trilist first
	if ( b32BITIndexData==false )
		ConvertLocalMeshToTriList ( pMesh );

	// convert from index to vertex only
	if ( pMesh->iPrimitiveType==GGPT_TRIANGLELIST)
	{
		if ( pMesh->pIndices )
		{
			// new vertex data
			DWORD dwVertSize = pMesh->dwFVFSize;
			DWORD dwNewVertexCount = pMesh->dwIndexCount;
			BYTE* pNewVertexData = (BYTE*)new char [ dwNewVertexCount * dwVertSize ];

			// recreate conversion map
			SAFE_DELETE_ARRAY ( g_pConversionMap );
			if ( g_pConversionMap==NULL )
			{
				// size of old vertex buffer (where old face data referenced)
				g_pConversionMap = new DWORD[pMesh->dwVertexCount];
			}

			// go through all indices (leefix-161003-fixed wFaceIndex to dwFaceIndex as this function used to expand vertex data so no index data is required (16bit+)
			DWORD dwFaceIndex = 0;
			BYTE* pBase = pMesh->pVertexData;
			DWORD* pDWORDIndexPtr = (DWORD*)pMesh->pIndices;
			for ( DWORD i=0; i<pMesh->dwIndexCount; i+=3 )
			{
				// read face
				DWORD dwFace0, dwFace1, dwFace2;
				if ( b32BITIndexData )
				{
					// special temporary DWORD index data
					dwFace0 = pDWORDIndexPtr[i+0];
					dwFace1 = pDWORDIndexPtr[i+1];
					dwFace2 = pDWORDIndexPtr[i+2];
				}
				else
				{
					// normal WORD index data
					dwFace0 = pMesh->pIndices[i+0];
					dwFace1 = pMesh->pIndices[i+1];
					dwFace2 = pMesh->pIndices[i+2];
				}

				// get vert data
				float* pFromVert1 = (float*)((BYTE*)pBase+(dwFace0*dwVertSize));
				float* pFromVert2 = (float*)((BYTE*)pBase+(dwFace1*dwVertSize));
				float* pFromVert3 = (float*)((BYTE*)pBase+(dwFace2*dwVertSize));
				float* pToVert1 = (float*)((BYTE*)pNewVertexData+((dwFaceIndex+0)*dwVertSize));
				float* pToVert2 = (float*)((BYTE*)pNewVertexData+((dwFaceIndex+1)*dwVertSize));
				float* pToVert3 = (float*)((BYTE*)pNewVertexData+((dwFaceIndex+2)*dwVertSize));

				// record destination indexes in conversion map
				g_pConversionMap[dwFace0]=dwFaceIndex+0;
				g_pConversionMap[dwFace1]=dwFaceIndex+1;
				g_pConversionMap[dwFace2]=dwFaceIndex+2;
				dwFaceIndex+=3;

				// write to new vert data
				memcpy ( pToVert1, pFromVert1, dwVertSize );
				memcpy ( pToVert2, pFromVert2, dwVertSize );
				memcpy ( pToVert3, pFromVert3, dwVertSize );
			}

			// delete index data and old vertex data
			// 281114 - changed to SAFE_DELETE_ARRAY
			SAFE_DELETE_ARRAY(pMesh->pIndices);
			SAFE_DELETE_ARRAY(pMesh->pVertexData);

			// replace mesh ptrs
			pMesh->dwIndexCount = 0;
			pMesh->dwVertexCount = dwNewVertexCount;
			pMesh->pVertexData = pNewVertexData;
			pMesh->iDrawVertexCount = dwNewVertexCount;
			pMesh->iDrawPrimitives  = dwNewVertexCount/3;
		}
	}
}

DARKSDK_DLL bool ConvertLocalMeshToTriList ( sMesh* pMesh )
{
	// was action taken
	bool bActionTaken=false;

	// convert from tristrip
	if ( pMesh->iPrimitiveType==GGPT_TRIANGLESTRIP )
	{
		WORD* pNewIndex = NULL;
		DWORD dwNewIndexCount = 0;
		if ( pMesh->pIndices==NULL )
		{
			// generate new mesh without indices
			WORD wIndexSeq = 0;
			dwNewIndexCount = (pMesh->dwVertexCount-2) * 3;
			pNewIndex = new WORD [ dwNewIndexCount ];
			WORD wFace0 = wIndexSeq; wIndexSeq++;
			WORD wFace1 = 0;
			WORD wFace2 = wIndexSeq; wIndexSeq++;
			int iToggle = 0;
			DWORD dwIndex = 0;
			for ( DWORD i = 2; i < pMesh->dwVertexCount; i++ )
			{
				// face assignments
				if ( iToggle==0 )
				{
					wFace0 = wFace0;
					wFace1 = wFace2;
					wFace2 = wIndexSeq; wIndexSeq++;
				}
				else
				{
					wFace0 = wFace2;
					wFace1 = wFace1;
					wFace2 = wIndexSeq; wIndexSeq++;
				}
				iToggle=1-iToggle;

				// get face vectors
				pNewIndex [ dwIndex++ ] = wFace0;
				pNewIndex [ dwIndex++ ] = wFace1;
				pNewIndex [ dwIndex++ ] = wFace2;
			}
		}
		else
		{
			// generate new mesh from indices
			dwNewIndexCount = (pMesh->dwIndexCount-2) * 3;
			pNewIndex = new WORD [ dwNewIndexCount ];
			WORD wFace0 = pMesh->pIndices [ 0 ];
			WORD wFace1 = 0;
			WORD wFace2 = pMesh->pIndices [ 1 ];
			int iToggle = 0;
			DWORD dwIndex = 0;
			for ( DWORD i = 2; i < pMesh->dwIndexCount; i++ )
			{
				// face assignments
				if ( iToggle==0 )
				{
					wFace0 = wFace0;
					wFace1 = wFace2;
					wFace2 = pMesh->pIndices [ i ];
				}
				else
				{
					wFace0 = wFace2;
					wFace1 = wFace1;
					wFace2 = pMesh->pIndices [ i ];
				}
				iToggle=1-iToggle;

				// get face vectors
				pNewIndex [ dwIndex++ ] = wFace0;
				pNewIndex [ dwIndex++ ] = wFace1;
				pNewIndex [ dwIndex++ ] = wFace2;
			}
		}

		// delete old index data
		// 281114 - changed to SAFE_DELETE_ARRAY
		SAFE_DELETE_ARRAY(pMesh->pIndices);

		// replace mesh ptrs
		pMesh->iPrimitiveType = 4;
		pMesh->dwIndexCount = dwNewIndexCount;
		pMesh->pIndices = pNewIndex;

		// complete mesh replace - slow
		pMesh->bMeshHasBeenReplaced = true;
		bActionTaken = true;
	}

	// convert from trifan
	if ( pMesh->iPrimitiveType==GGPT_TRIANGLEFAN )
	{
		WORD* pNewIndex = NULL;
		DWORD dwNewIndexCount = 0;
		if ( pMesh->pIndices==NULL )
		{
			// generate new indices
			WORD wIndexSeq = 0;
			dwNewIndexCount = (pMesh->dwVertexCount-2) * 3;
			pNewIndex = new WORD [ dwNewIndexCount ];
			WORD wFace0 = wIndexSeq; wIndexSeq++;
			WORD wFace1 = 0;
			WORD wFace2 = wIndexSeq; wIndexSeq++;
			DWORD dwIndex = 0;
			for ( DWORD i = 2; i < pMesh->dwVertexCount; i++ )
			{
				// face assignments
				wFace0 = wFace0;
				wFace1 = wFace2;
				wFace2 = wIndexSeq; wIndexSeq++;

				// get face vectors
				pNewIndex [ dwIndex++ ] = wFace0;
				pNewIndex [ dwIndex++ ] = wFace1;
				pNewIndex [ dwIndex++ ] = wFace2;
			}
		}

		// delete old index data
		// 281114 - changed to SAFE_DELETE_ARRAY
		SAFE_DELETE_ARRAY(pMesh->pIndices);

		// replace mesh ptrs
		pMesh->iPrimitiveType = 4;
		pMesh->dwIndexCount = dwNewIndexCount;
		pMesh->pIndices = pNewIndex;

		// complete mesh replace - slow
		pMesh->bMeshHasBeenReplaced = true;
		bActionTaken = true;
	}

	// return action state
	return bActionTaken;
}

DARKSDK_DLL void ConvertToSharedVerts ( sMesh* pMesh, float fEpsilon )
{
	if ( pMesh->pIndices )
	{
		DWORD dwFaceIndex = 0;
		BYTE* pBase = pMesh->pVertexData;
		DWORD dwVertSize = pMesh->dwFVFSize;
		for ( DWORD i=0; i<pMesh->dwIndexCount; i+=3 )
		{
			// read face
			DWORD dwFace0, dwFace1, dwFace2;
			dwFace0 = pMesh->pIndices[i+0];
			dwFace1 = pMesh->pIndices[i+1];
			dwFace2 = pMesh->pIndices[i+2];

			// get vert data
			GGVECTOR3* pV0 = (GGVECTOR3*)((BYTE*)pBase+(dwFace0*dwVertSize));
			GGVECTOR3* pV1 = (GGVECTOR3*)((BYTE*)pBase+(dwFace1*dwVertSize));
			GGVECTOR3* pV2 = (GGVECTOR3*)((BYTE*)pBase+(dwFace2*dwVertSize));

			// find any previous instance of each vert position
			for ( DWORD facedone=0; facedone<i; facedone++ )
			{
				// read previous faces
				DWORD dwFaceD0 = pMesh->pIndices[facedone+0];
				DWORD dwFaceD1 = pMesh->pIndices[facedone+1];
				DWORD dwFaceD2 = pMesh->pIndices[facedone+2];
				GGVECTOR3* pVD0 = (GGVECTOR3*)((BYTE*)pBase+(dwFaceD0*dwVertSize));
				GGVECTOR3* pVD1 = (GGVECTOR3*)((BYTE*)pBase+(dwFaceD1*dwVertSize));
				GGVECTOR3* pVD2 = (GGVECTOR3*)((BYTE*)pBase+(dwFaceD2*dwVertSize));

				// check against current face, and re-use older face index if found
				GGVECTOR3 vec0 = *pV0 - *pVD0;
				GGVECTOR3 vec1 = *pV0 - *pVD1;
				GGVECTOR3 vec2 = *pV0 - *pVD2;
				if ( GGVec3Length ( &vec0 ) < fEpsilon ) pMesh->pIndices[i+0] = (WORD)dwFaceD0;
				if ( GGVec3Length ( &vec1 ) < fEpsilon ) pMesh->pIndices[i+0] = (WORD)dwFaceD1;
				if ( GGVec3Length ( &vec2 ) < fEpsilon ) pMesh->pIndices[i+0] = (WORD)dwFaceD2;
				vec0 = *pV1 - *pVD0;
				vec1 = *pV1 - *pVD1;
				vec2 = *pV1 - *pVD2;
				if ( GGVec3Length ( &vec0 ) < fEpsilon ) pMesh->pIndices[i+1] = (WORD)dwFaceD0;
				if ( GGVec3Length ( &vec1 ) < fEpsilon ) pMesh->pIndices[i+1] = (WORD)dwFaceD1;
				if ( GGVec3Length ( &vec2 ) < fEpsilon ) pMesh->pIndices[i+1] = (WORD)dwFaceD2;
				vec0 = *pV2 - *pVD0;
				vec1 = *pV2 - *pVD1;
				vec2 = *pV2 - *pVD2;
				if ( GGVec3Length ( &vec0 ) < fEpsilon ) pMesh->pIndices[i+2] = (WORD)dwFaceD0;
				if ( GGVec3Length ( &vec1 ) < fEpsilon ) pMesh->pIndices[i+2] = (WORD)dwFaceD1;
				if ( GGVec3Length ( &vec2 ) < fEpsilon ) pMesh->pIndices[i+2] = (WORD)dwFaceD2;
			}
		}
	}
}

DARKSDK_DLL bool MakeLocalMeshFromOtherLocalMesh ( sMesh* pMesh, sMesh* pOtherMesh, DWORD dwIndexCount, DWORD dwVertexCount )
{
	// get details from other mesh
	DWORD dwFVF				= pOtherMesh->dwFVF;
	DWORD dwFVFSize			= pOtherMesh->dwFVFSize;

	// mesh can hold regular FVF and custom declarations
	bool bTempAllow32BitIndexSoCanProduceVertOnlyMesh = false;
	if ( dwFVF==0 )
	{
		// now create new mesh data from declaration
		if ( !SetupMeshDeclarationData ( pMesh, pOtherMesh->pVertexDeclaration, dwFVFSize, dwVertexCount, dwIndexCount ) )
			return false;
	}
	else
	{
		// 310819 - if the indexcount is over 16bit, we know the next call will fail, so convert mesh to vertex only
		if ( dwIndexCount > 0 )
			if ( dwVertexCount > 0xFFFF )//if ( dwIndexCount > 0x0000FFFF )
				bTempAllow32BitIndexSoCanProduceVertOnlyMesh = true;

		// create new mesh from FVF
		if ( !SetupMeshFVFData ( pMesh, dwFVF, dwVertexCount, dwIndexCount, bTempAllow32BitIndexSoCanProduceVertOnlyMesh ) )
			return false;
	}

	// copy vertex data
	DWORD dwVertexDataSize = pOtherMesh->dwFVFSize * pOtherMesh->dwVertexCount;
	memcpy ( pMesh->pVertexData, pOtherMesh->pVertexData, dwVertexDataSize );

	// copy index data
	DWORD dwIndiceDataSize = sizeof(WORD) * pOtherMesh->dwIndexCount;
	if ( bTempAllow32BitIndexSoCanProduceVertOnlyMesh == true ) dwIndiceDataSize = sizeof(DWORD) * pOtherMesh->dwIndexCount;
	if ( pMesh->pIndices ) memcpy ( pMesh->pIndices, pOtherMesh->pIndices, dwIndiceDataSize );

	// setup mesh drawing properties
	pMesh->iPrimitiveType   = pOtherMesh->iPrimitiveType;
	pMesh->iDrawVertexCount = pOtherMesh->iDrawVertexCount;
	pMesh->iDrawPrimitives  = pOtherMesh->iDrawPrimitives;

	// leeadd - 030306 - u60 - if mesh from rawmesh, no prim to draw from
	if ( pMesh->iDrawPrimitives==0 )
	{
		// calculate a value that makes most sense
		pMesh->iDrawPrimitives=pMesh->iDrawVertexCount/3;
	}

	// we 'still' do not support 32bit indices (ouch), so convert this mesh to vert only so it works with everything else
	if ( bTempAllow32BitIndexSoCanProduceVertOnlyMesh == true )
		ConvertLocalMeshToVertsOnly ( pMesh, bTempAllow32BitIndexSoCanProduceVertOnlyMesh );

	// okay
	return true;
}

DARKSDK_DLL bool MakeLocalMeshFromOtherLocalMesh ( sMesh* pMesh, sMesh* pOtherMesh )
{
	DWORD dwIndexCount = pOtherMesh->dwIndexCount;
	DWORD dwVertexCount = pOtherMesh->dwVertexCount;
	return MakeLocalMeshFromOtherLocalMesh ( pMesh, pOtherMesh, dwIndexCount, dwVertexCount );
}

DARKSDK_DLL bool MakeLocalMeshFromPureMeshData ( sMesh* pMesh, DWORD dwFVF, DWORD dwFVFSize, float* pMeshData, DWORD dwVertexCount, DWORD dwPrimType )
{
	// create new mesh
	if ( !SetupMeshFVFData ( pMesh, dwFVF, dwVertexCount, 0, false ) )
		return false;

	// copy vertex data
	DWORD dwVertexDataSize = dwFVFSize * dwVertexCount;
	memcpy ( pMesh->pVertexData, pMeshData, dwVertexDataSize );

	// setup mesh drawing properties
	pMesh->iPrimitiveType   = dwPrimType;
	pMesh->iDrawVertexCount = dwVertexCount;
	pMesh->iDrawPrimitives  = dwVertexCount/3;

	#ifdef WICKEDENGINE
	// wicked engine needs indice data, so create some
	if (pMesh->pIndices == NULL)
	{
		if (dwVertexCount < 65535)
		{
			pMesh->dwIndexCount = dwVertexCount;
			pMesh->pIndices = new WORD[dwVertexCount];
			for (int i = 0; i < dwVertexCount; i++)
				pMesh->pIndices[i] = i;
		}
		else
		{
			// Cannot have index buffer greater than 2^16 
			pMesh->dwIndexCount = 0;
			pMesh->pIndices = 0;
		}
	}
	#endif

	// okay
	return true;
}

DARKSDK_DLL LPGGMESH ComputeTangentBasisEx ( LPGGMESH gMasterMesh, bool bMakeNormals, bool bMakeTangents, bool bMakeBinormals, bool bFixTangents, bool bCylTexGen, bool bWeightNormalsByFace )
{
	#ifdef DX11
	// define raw input data type for this computation
	typedef struct 
	{
		GGVECTOR3 position;
		GGVECTOR3 normal;
		GGVECTOR2 texCoord;
	} MeshVertex;

	// input/output ptrs for conversion
	std::vector<float> position;
	std::vector<float> normal;
	std::vector<float> texCoord;
	std::vector<float> texCoord2;
	std::vector<float> binormal;
	std::vector<float> tangent;

	// Retrieve data from the temp mesh, put in input/output ptrs
	sOffsetMap offsetMap;
	bool bRetainSecondaryUVData = false;
	GetFVFValueOffsetMap ( gMasterMesh->dwFVF, &offsetMap );
	if ( offsetMap.dwTU[1] > 0 ) bRetainSecondaryUVData = true;
	DWORD numVertices = gMasterMesh->dwVertexCount;
	for (unsigned int i = 0; i < numVertices; ++i) 
	{
		float fX = *( ( float* ) gMasterMesh->pVertexData + offsetMap.dwX + ( offsetMap.dwSize * i ) );
		float fY = *( ( float* ) gMasterMesh->pVertexData + offsetMap.dwY + ( offsetMap.dwSize * i ) );
		float fZ = *( ( float* ) gMasterMesh->pVertexData + offsetMap.dwZ + ( offsetMap.dwSize * i ) );
		float fNX = *( ( float* ) gMasterMesh->pVertexData + offsetMap.dwNX + ( offsetMap.dwSize * i ) );
		float fNY = *( ( float* ) gMasterMesh->pVertexData + offsetMap.dwNY + ( offsetMap.dwSize * i ) );
		float fNZ = *( ( float* ) gMasterMesh->pVertexData + offsetMap.dwNZ + ( offsetMap.dwSize * i ) );
		float fU = *( ( float* ) gMasterMesh->pVertexData + offsetMap.dwTU[0] + ( offsetMap.dwSize * i ) );
		float fV = *( ( float* ) gMasterMesh->pVertexData + offsetMap.dwTV[0] + ( offsetMap.dwSize * i ) );
		float fU2 = 0.0f;
		float fV2 = 0.0f;
		if ( bRetainSecondaryUVData == true )
		{
			fU2 = *( ( float* ) gMasterMesh->pVertexData + offsetMap.dwTU[1] + ( offsetMap.dwSize * i ) );
			fV2 = *( ( float* ) gMasterMesh->pVertexData + offsetMap.dwTV[1] + ( offsetMap.dwSize * i ) );
		}
		position.push_back(fX);
		position.push_back(fY);
		position.push_back(fZ);
		normal.push_back(fNX);
		normal.push_back(fNY);
		normal.push_back(fNZ);
		texCoord.push_back(fU);
		texCoord.push_back(fV);
		texCoord.push_back(0);
		texCoord2.push_back(fU2);
		texCoord2.push_back(fV2);
		texCoord2.push_back(0);
	}

	// Retrieve triangle indices from the temp mesh, put in input/output ptr
	std::vector<int> index;
	if ( gMasterMesh->pIndices )
	{
		DWORD numTriangles = gMasterMesh->iDrawPrimitives;
		for (unsigned int i = 0; i < numTriangles; ++i) 
		{
			int i0 = *( ( WORD* ) gMasterMesh->pIndices + ( 3 * i ) + 0 );
			int i1 = *( ( WORD* ) gMasterMesh->pIndices + ( 3 * i ) + 1 );
			int i2 = *( ( WORD* ) gMasterMesh->pIndices + ( 3 * i ) + 2 );
			index.push_back(i0);
			index.push_back(i1);
			index.push_back(i2);
		}
	}
	else
	{
		// no indices, no conversion - sky ried to convert, but had no indices
		// return NULL;
		if ( gMasterMesh->dwFVF == 530 )
		{
			// 290618 - so can add tangents and binormals to lightmapped objects, add indices if not present
			int iPolygonCount = 0;
			DWORD numTriangles = gMasterMesh->iDrawPrimitives;
			for (unsigned int i = 0; i < numTriangles; ++i) 
			{
				int i0 = iPolygonCount + 0;
				int i1 = iPolygonCount + 1;
				int i2 = iPolygonCount + 2;
				index.push_back(i0);
				index.push_back(i1);
				index.push_back(i2);
				iPolygonCount+=3;
			}
		}
		else
		{
			// all other objects are not converted to retain backwards compatibility with engine elements (shadow floor)
			return NULL;
		}
	}

	// Specify conversion options from flags
	NVMeshMender::Option _FixTangents = NVMeshMender::FixTangents;
	NVMeshMender::Option _FixCylindricalTexGen = NVMeshMender::FixCylindricalTexGen;
	NVMeshMender::Option _WeightNormalsByFaceSize = NVMeshMender::WeightNormalsByFaceSize;
	if ( bFixTangents==false ) _FixTangents = NVMeshMender::DontFixTangents;
	if ( bCylTexGen==false ) _FixCylindricalTexGen = NVMeshMender::DontFixCylindricalTexGen;
	if ( bWeightNormalsByFace==false ) _WeightNormalsByFaceSize = NVMeshMender::DontWeightNormalsByFaceSize;

	// Setup INPUT Components
	NVMeshMender::VertexAttribute positionAtt;
	positionAtt.Name_ = "position";
	positionAtt.floatVector_ = position;
	NVMeshMender::VertexAttribute normalAtt;
	normalAtt.Name_ = "normal";
	normalAtt.floatVector_ = normal;
	NVMeshMender::VertexAttribute indexAtt;
	indexAtt.Name_ = "indices";
	indexAtt.intVector_ = index;
	NVMeshMender::VertexAttribute texCoordAtt;
	texCoordAtt.Name_ = "tex0";
	texCoordAtt.floatVector_ = texCoord;
	NVMeshMender::VertexAttribute texCoordAtt2;
	texCoordAtt2.Name_ = "tex1";
	texCoordAtt2.floatVector_ = texCoord2;

	// Create INPUT Attribute
	std::vector<NVMeshMender::VertexAttribute> inputAtts;
	inputAtts.push_back(positionAtt);
	inputAtts.push_back(indexAtt);
	inputAtts.push_back(texCoordAtt);
	if ( bRetainSecondaryUVData == true ) inputAtts.push_back(texCoordAtt2);
	inputAtts.push_back(normalAtt);
	
	// Add In OUTPUT Components
	NVMeshMender::VertexAttribute tangentAtt;
	tangentAtt.Name_ = "tangent";
	NVMeshMender::VertexAttribute binormalAtt;
	binormalAtt.Name_ = "binormal";

	// Create OUTPUT Attribute
	unsigned int n = 0;
	std::vector<NVMeshMender::VertexAttribute> outputAtts;
	outputAtts.push_back(positionAtt); ++n;
	outputAtts.push_back(indexAtt); ++n;
	outputAtts.push_back(texCoordAtt); ++n;
	if ( bRetainSecondaryUVData == true ) 
	{
		outputAtts.push_back(texCoordAtt2); ++n;
	}
	outputAtts.push_back(normalAtt); ++n;
	outputAtts.push_back(tangentAtt); ++n;
	outputAtts.push_back(binormalAtt); ++n;

	//PE: (note) tangent is sometimes calculated wrong ? perhaps for missing fixtangent, that we are not able to use.
	// Uses MeshMenderD3DX from NVIDIA
	NVMeshMender mender;
	if (!mender.MungeD3DX(
					inputAtts,								// input attributes
					outputAtts,								// outputs attributes
					3.141592654f / 3.0f,					// tangent space smooth angle
					0,										// no texture matrix applied to my texture coordinates
					_FixTangents,							// fix degenerate bases & texture mirroring
					_FixCylindricalTexGen,					// low poly and quad meshes
					_WeightNormalsByFaceSize				// weigh vertex normals by the triangle's size
					))
	{
		// Failed to convert mesh over
		return gMasterMesh;
	}

	// Get output ptrs after conversion
	--n; binormal = outputAtts[n].floatVector_; 
	--n; tangent = outputAtts[n].floatVector_; 
	--n; normal = outputAtts[n].floatVector_; 
	if ( bRetainSecondaryUVData == true ) 
	{
		--n; texCoord2 = outputAtts[n].floatVector_;
	}
	--n; texCoord = outputAtts[n].floatVector_;
	--n; index = outputAtts[n].intVector_;
	--n; position = outputAtts[n].floatVector_;

	// ensure vertex data and index data size is unchanged
	DWORD dwNewVertexCount = position.size()/3;
	DWORD dwNewFaceCount = index.size()/3;

	// create mesh from new declaration
	SAFE_DELETE(gMasterMesh->pVertexData);
	SAFE_DELETE(gMasterMesh->pIndices);
	gMasterMesh->dwFVFOriginal = gMasterMesh->dwFVF;
	gMasterMesh->dwFVF = 0;
	gMasterMesh->dwFVFSize = 12+12+8+12+12;
	if ( bRetainSecondaryUVData == true ) 
	{
		gMasterMesh->dwFVFSize += 8;
		gMasterMesh->dwFVF = 530;
	}
	DWORD dwVSize = gMasterMesh->dwFVFSize;
	gMasterMesh->pVertexData = new BYTE[dwNewVertexCount*dwVSize];
	gMasterMesh->pIndices = new WORD[dwNewFaceCount*3];
	gMasterMesh->dwIndexCount = dwNewFaceCount*3;

	// Copy data into new mesh
	int iPosOffset = -1;
	int iDiffuseOffset = -1;
	int iTexOffset = -1;
	int iTexOffset2 = -1;
	int iNormalOffset = -1;
	int iTangentOffset = -1;
	int iBinormalOffset = -1;
	GGEFFECT_DESC EffectDesc;
	ID3DX11EffectTechnique* hTechnique;
	D3DX11_TECHNIQUE_DESC TechniqueDesc;
	ID3DX11EffectPass* hPass;
	cSpecialEffect* pEffect = gMasterMesh->pVertexShaderEffect;
	pEffect->m_pEffect->GetDesc( &EffectDesc );
	for( UINT iTech = 0; iTech < EffectDesc.Techniques; iTech++ )
	{
		hTechnique = pEffect->m_pEffect->GetTechniqueByIndex( iTech );
		hTechnique->GetDesc ( &TechniqueDesc );
		for( UINT iPass = 0; iPass < TechniqueDesc.Passes; iPass++ )
		{
			hPass = hTechnique->GetPassByIndex ( iPass );
			D3DX11_PASS_SHADER_DESC vs_desc;
			hPass->GetVertexShaderDesc(&vs_desc);
			D3DX11_EFFECT_SHADER_DESC s_desc;
			vs_desc.pShaderVariable->GetShaderDesc(0, &s_desc);
            UINT NumVSSemanticsUsed = s_desc.NumInputSignatureEntries;
			int iByteOffset = 0;
			for( UINT iSem = 0; iSem < NumVSSemanticsUsed; iSem++ )
			{
				D3D11_SIGNATURE_PARAMETER_DESC pSigParDesc;
				vs_desc.pShaderVariable->GetInputSignatureElementDesc ( 0, iSem, &pSigParDesc );
				if( stricmp ( pSigParDesc.SemanticName, "POSITION" ) == NULL ) { iPosOffset = iByteOffset; iByteOffset += 12; }
				if( stricmp ( pSigParDesc.SemanticName, "NORMAL" ) == NULL ) { iNormalOffset = iByteOffset; iByteOffset += 12; }
				if( stricmp ( pSigParDesc.SemanticName, "COLOR" ) == NULL ) { iDiffuseOffset = iByteOffset; iByteOffset += 4; }
				if( stricmp ( pSigParDesc.SemanticName, "TEXCOORD" ) == NULL && pSigParDesc.SemanticIndex == 0 ) { iTexOffset = iByteOffset; iByteOffset += 8; }
				if( stricmp ( pSigParDesc.SemanticName, "TEXCOORD" ) == NULL && pSigParDesc.SemanticIndex == 1 ) { iTexOffset2 = iByteOffset; iByteOffset += 8; }
				if( stricmp ( pSigParDesc.SemanticName, "TANGENT" ) == NULL ) { iTangentOffset = iByteOffset; iByteOffset += 12; }
				if( stricmp ( pSigParDesc.SemanticName, "BINORMAL" ) == NULL ) { iBinormalOffset = iByteOffset; iByteOffset += 12; }
			}
		}
	}

	// Binormal makers
	BYTE* pPtr = gMasterMesh->pVertexData;
	for ( DWORD v=0; v<dwNewVertexCount; ++v)
	{
		// obtain component ptrs
		GGVECTOR3* vecPos = (GGVECTOR3*)(pPtr+iPosOffset);
		GGVECTOR3* vecNormal = (GGVECTOR3*)(pPtr+iNormalOffset);
		GGCOLOR*   colDiffuse = (GGCOLOR*)(pPtr+iDiffuseOffset);
		GGVECTOR2* vecTex = (GGVECTOR2*)(pPtr+iTexOffset);
		GGVECTOR3* vecTangent = (GGVECTOR3*)(pPtr+iTangentOffset);
		GGVECTOR3* vecBinormal = (GGVECTOR3*)(pPtr+iBinormalOffset);

		// fill data of components in output mesh
		if ( iPosOffset!=-1 )
		{
			vecPos->x = position[3 * v + 0];
			vecPos->y = position[3 * v + 1];
			vecPos->z = position[3 * v + 2];
		}
		if ( iNormalOffset!=-1 )
		{
			vecNormal->x = normal[3 * v + 0];
			vecNormal->y = normal[3 * v + 1];
			vecNormal->z = normal[3 * v + 2];
		}
		if ( iTexOffset!=-1 )
		{
			vecTex->x = texCoord[3 * v + 0];
			vecTex->y = texCoord[3 * v + 1];
		}
		if ( bRetainSecondaryUVData == true ) 
		{
			GGVECTOR2* vecTex2 = (GGVECTOR2*)(pPtr+iTexOffset2);
			if ( iTexOffset2!=-1 )
			{
				vecTex2->x = texCoord2[3 * v + 0];
				vecTex2->y = texCoord2[3 * v + 1];
			}
		}
		if ( iDiffuseOffset!=-1 )
		{
			*colDiffuse = GGCOLOR(255,255,255,255);
		}
		if ( iTangentOffset!=-1 )
		{
			vecTangent->x = tangent[3 * v + 0];
			vecTangent->y = tangent[3 * v + 1];
			vecTangent->z = tangent[3 * v + 2];
		}
		if ( iBinormalOffset!=-1 )
		{
			vecBinormal->x = binormal[3 * v + 0];
			vecBinormal->y = binormal[3 * v + 1];
			vecBinormal->z = binormal[3 * v + 2];
		}

		// next vertex
		pPtr+=dwVSize;
	}

	// index buffer data
	for (DWORD i = 0; i < dwNewFaceCount; ++i)
	{
		gMasterMesh->pIndices[(i*3)+0] = index[3 * i + 0];
		gMasterMesh->pIndices[(i*3)+1] = index[3 * i + 1];
		gMasterMesh->pIndices[(i*3)+2] = index[3 * i + 2];
	}
	return NULL;
	#else
	// define raw input data type for this computation
	LPGGMESH pOutputMesh = NULL;
	typedef struct 
	{
		GGVECTOR3 position;
		GGVECTOR3 normal;
		GGVECTOR2 texCoord;
	} MeshVertex;

	// input/output ptrs for conversion
	std::vector<float> position;
	std::vector<float> normal;
	std::vector<float> texCoord;
	std::vector<float> binormal;
	std::vector<float> tangent;

	// Get declaration of mesh to convert
	GGVERTEXELEMENT End = GDECL_END();
	GGVERTEXELEMENT Declaration[MAX_FVF_DECL_SIZE];
	gMasterMesh->GetDeclaration( Declaration );

	// Create temporary mesh to hold required input data XYZ,NORMAL,TEX
	// leefix - 050906 - changed 'gMesh' to 'gMasterMesh' and 'pNewMesh' to 'pInputDataMesh'
	LPGGMESH pInputDataMesh = NULL;
	DWORD dwSysteMemFlag = D3DXMESH_SYSTEMMEM;
	if ( gMasterMesh->GetNumVertices() > 65535 ) dwSysteMemFlag = D3DXMESH_SYSTEMMEM | D3DXMESH_32BIT;
	gMasterMesh->CloneMeshFVF ( dwSysteMemFlag, GGFVF_XYZ | GGFVF_NORMAL | GGFVF_TEX1, m_pD3D, &pInputDataMesh );
	if ( pInputDataMesh==NULL ) return gMasterMesh;

	// Retrieve data from the temp mesh, put in input/output ptrs
	MeshVertex* vertexBuffer = NULL;
	DWORD numVertices = pInputDataMesh->GetNumVertices();
	pInputDataMesh->LockVertexBuffer(D3DLOCK_READONLY, (VOID**)&vertexBuffer);
	if ( vertexBuffer==NULL ) return gMasterMesh;
	unsigned int i;
	for (i = 0; i < numVertices; ++i) {
		position.push_back(vertexBuffer[i].position.x);
		position.push_back(vertexBuffer[i].position.y);
		position.push_back(vertexBuffer[i].position.z);
		normal.push_back(vertexBuffer[i].normal.x);
		normal.push_back(vertexBuffer[i].normal.y);
		normal.push_back(vertexBuffer[i].normal.z);
		texCoord.push_back(vertexBuffer[i].texCoord.x);
		texCoord.push_back(vertexBuffer[i].texCoord.y);
		texCoord.push_back(0);
	}
	pInputDataMesh->UnlockVertexBuffer();

	// Retrieve triangle indices from the temp mesh, put in input/output ptr
	WORD (*indexBuffer)[3];
	std::vector<int> index;
	DWORD numTriangles = pInputDataMesh->GetNumFaces();
	pInputDataMesh->LockIndexBuffer(D3DLOCK_READONLY, (VOID**)&indexBuffer);
	if ( indexBuffer==NULL ) return gMasterMesh;
	for (i = 0; i < numTriangles; ++i) 
	{
		index.push_back(indexBuffer[i][0]);
		index.push_back(indexBuffer[i][1]);
		index.push_back(indexBuffer[i][2]);
	}
	pInputDataMesh->UnlockIndexBuffer();

	// finished with input mesh, release this temp mesh now
	SAFE_RELEASE ( pInputDataMesh );

	// Specify conversion options from flags
	NVMeshMender::Option _FixTangents = NVMeshMender::FixTangents;
	NVMeshMender::Option _FixCylindricalTexGen = NVMeshMender::FixCylindricalTexGen;
	NVMeshMender::Option _WeightNormalsByFaceSize = NVMeshMender::WeightNormalsByFaceSize;
	if ( bFixTangents==false ) _FixTangents = NVMeshMender::DontFixTangents;
	if ( bCylTexGen==false ) _FixCylindricalTexGen = NVMeshMender::DontFixCylindricalTexGen;
	if ( bWeightNormalsByFace==false ) _WeightNormalsByFaceSize = NVMeshMender::DontWeightNormalsByFaceSize;

	// Setup INPUT Components
	NVMeshMender::VertexAttribute positionAtt;
	positionAtt.Name_ = "position";
	positionAtt.floatVector_ = position;
	NVMeshMender::VertexAttribute normalAtt;
	normalAtt.Name_ = "normal";
	normalAtt.floatVector_ = normal;
	NVMeshMender::VertexAttribute indexAtt;
	indexAtt.Name_ = "indices";
	indexAtt.intVector_ = index;
	NVMeshMender::VertexAttribute texCoordAtt;
	texCoordAtt.Name_ = "tex0";
	texCoordAtt.floatVector_ = texCoord;

	// Create INPUT Attribute
	std::vector<NVMeshMender::VertexAttribute> inputAtts;
	inputAtts.push_back(positionAtt);
	inputAtts.push_back(indexAtt);
	inputAtts.push_back(texCoordAtt);
	inputAtts.push_back(normalAtt);
	
	// Add In OUTPUT Components
	NVMeshMender::VertexAttribute tangentAtt;
	tangentAtt.Name_ = "tangent";
	NVMeshMender::VertexAttribute binormalAtt;
	binormalAtt.Name_ = "binormal";

	// Create OUTPUT Attribute
	unsigned int n = 0;
	std::vector<NVMeshMender::VertexAttribute> outputAtts;
	outputAtts.push_back(positionAtt); ++n;
	outputAtts.push_back(indexAtt); ++n;
	outputAtts.push_back(texCoordAtt); ++n;
	outputAtts.push_back(normalAtt); ++n;
	outputAtts.push_back(tangentAtt); ++n;
	outputAtts.push_back(binormalAtt); ++n;

	// Uses MeshMenderD3DX from NVIDIA
	NVMeshMender mender;
	if (!mender.MungeD3DX(
					inputAtts,								// input attributes
					outputAtts,								// outputs attributes
					3.141592654f / 3.0f,					// tangent space smooth angle
					0,										// no texture matrix applied to my texture coordinates
					_FixTangents,							// fix degenerate bases & texture mirroring
					_FixCylindricalTexGen,					// low poly and quad meshes
					_WeightNormalsByFaceSize				// weigh vertex normals by the triangle's size
					))
	{
		// Failed to convert mesh over
		return gMasterMesh;
	}

	// Get output ptrs after conversion
	--n; binormal = outputAtts[n].floatVector_; 
	--n; tangent = outputAtts[n].floatVector_; 
	--n; normal = outputAtts[n].floatVector_; 
	--n; texCoord = outputAtts[n].floatVector_;
	--n; index = outputAtts[n].intVector_;
	--n; position = outputAtts[n].floatVector_;

	///* SOMEONE is corrupting the heap (near here, is it the code below?)

	// ensure vertex data and index data size is unchanged
	DWORD dwNewVertexCount = position.size()/3;
	DWORD dwNewFaceCount = index.size()/3;
	if ( dwNewVertexCount==numVertices && dwNewFaceCount==numTriangles )
	{
		// create mesh from new declaration
		gMasterMesh->CloneMesh( dwSysteMemFlag, Declaration, m_pD3D, &pOutputMesh );
	}
	else
	{
		// create new mesh resized for new output data
		D3DXCreateMesh ( dwNewFaceCount, dwNewVertexCount, dwSysteMemFlag, Declaration, m_pD3D, &pOutputMesh);
	}
	if ( pOutputMesh==NULL ) return gMasterMesh;

	// Lock the vertex buffer
	LPDIRECT3DVERTEXBUFFER9 pVB = NULL;
	DWORD dwVSize = pOutputMesh->GetNumBytesPerVertex();
	DWORD dwVertexCount = pOutputMesh->GetNumVertices();
	DWORD dwFaceCount = pOutputMesh->GetNumFaces();
	HRESULT hr = pOutputMesh->GetVertexBuffer( &pVB );
	if( SUCCEEDED(hr) )
	{
		BYTE* pVertices = NULL;
		hr = pVB->Lock( 0, 0, (VOID**)&pVertices, 0 );
		if( SUCCEEDED(hr) )
		{
			// Find Offsets
			int iPosOffset = -1;
			int iDiffuseOffset = -1;
			int iTexOffset = -1;
			int iNormalOffset = -1;
			int iTangentOffset = -1;
			int iBinormalOffset = -1;
			for( int iElem=0; Declaration[iElem].Stream != End.Stream; iElem++ )
			{   
				if( Declaration[iElem].Usage == GGDECLUSAGE_POSITION ) iPosOffset = Declaration[iElem].Offset;
				if( Declaration[iElem].Usage == GGDECLUSAGE_NORMAL ) iNormalOffset = Declaration[iElem].Offset;
				if( Declaration[iElem].Usage == D3DDECLUSAGE_COLOR ) iDiffuseOffset = Declaration[iElem].Offset;
				if( Declaration[iElem].Usage == GGDECLUSAGE_TEXCOORD && Declaration[iElem].UsageIndex==0 ) iTexOffset = Declaration[iElem].Offset; //leefix - 050906 - only take first stage TEX UV data
				if( Declaration[iElem].Usage == D3DDECLUSAGE_TANGENT ) iTangentOffset = Declaration[iElem].Offset;
				if( Declaration[iElem].Usage == D3DDECLUSAGE_BINORMAL ) iBinormalOffset = Declaration[iElem].Offset;
			}

			// Binormal makers
			BYTE* pPtr = pVertices;
			for ( DWORD v=0; v<dwVertexCount; ++v)
			{
				// obtain component ptrs
				GGVECTOR3* vecPos = (GGVECTOR3*)(pPtr+iPosOffset);
				GGVECTOR3* vecNormal = (GGVECTOR3*)(pPtr+iNormalOffset);
				GGCOLOR*   colDiffuse = (GGCOLOR*)(pPtr+iDiffuseOffset);
				GGVECTOR2* vecTex = (GGVECTOR2*)(pPtr+iTexOffset);
				GGVECTOR3* vecTangent = (GGVECTOR3*)(pPtr+iTangentOffset);
				GGVECTOR3* vecBinormal = (GGVECTOR3*)(pPtr+iBinormalOffset);

				// fill data of components in output mesh
				if ( iPosOffset!=-1 )
				{
					vecPos->x = position[3 * v + 0];
					vecPos->y = position[3 * v + 1];
					vecPos->z = position[3 * v + 2];
				}
				if ( iNormalOffset!=-1 )
				{
					vecNormal->x = normal[3 * v + 0];
					vecNormal->y = normal[3 * v + 1];
					vecNormal->z = normal[3 * v + 2];
				}
				if ( iTexOffset!=-1 )
				{
					vecTex->x = texCoord[3 * v + 0];
					vecTex->y = texCoord[3 * v + 1];
				}
				if ( iDiffuseOffset!=-1 )
				{
					*colDiffuse = GGCOLOR(255,255,255,255);
				}
				if ( iTangentOffset!=-1 )
				{
					vecTangent->x = tangent[3 * v + 0];
					vecTangent->y = tangent[3 * v + 1];
					vecTangent->z = tangent[3 * v + 2];
				}
				if ( iBinormalOffset!=-1 )
				{
					vecBinormal->x = binormal[3 * v + 0];
					vecBinormal->y = binormal[3 * v + 1];
					vecBinormal->z = binormal[3 * v + 2];
				}

				// next vertex
				pPtr+=dwVSize;
			}

			// unlock buffer
			pVB->Unlock();
		}

		// release buffer
		SAFE_RELEASE( pVB );
	}

	// Lock the index buffer
	LPDIRECT3DINDEXBUFFER9 pIB = NULL;
	hr = pOutputMesh->GetIndexBuffer( &pIB );
	if( SUCCEEDED(hr) )
	{
		hr = pIB->Lock( 0, 0, (VOID**)&indexBuffer, 0 );
		if( SUCCEEDED(hr) )
		{
			for (DWORD i = 0; i < dwFaceCount; ++i)
			{
				indexBuffer[i][0] = index[3 * i + 0];
				indexBuffer[i][1] = index[3 * i + 1];
				indexBuffer[i][2] = index[3 * i + 2];
			}

			// unlock buffer
			pIB->Unlock();
		}

		// release buffer
		SAFE_RELEASE( pIB );
	}
	
	// free old mesh
	SAFE_RELEASE ( gMasterMesh );

	// complete
	return pOutputMesh;
	#endif
}

DARKSDK_DLL LPGGMESH ComputeTangentBasis( LPGGMESH gMesh, bool bFixTangents, bool bCylTexGen, bool bWeightNormalsByFace )
{
	// leeadd - 050906 - added extra boolean controls for normal, tangent and binormal generation (darkshader control)
	return ComputeTangentBasisEx ( gMesh, true, true, true, bFixTangents, bCylTexGen, bWeightNormalsByFace );
}

DARKSDK_DLL bool CheckIfNeedExtraBonesPerVertices ( sMesh* pMesh )
{
	if ( pMesh->dwBoneCount==0 ) return false;
	DWORD dwNumVertices = pMesh->dwVertexCount;
	DWORD dwNumBones = pMesh->dwBoneCount;
	float* pfWorkWeight = new float [ dwNumVertices * dwNumBones ];
	memset ( pfWorkWeight, 0, sizeof( float ) * dwNumVertices * dwNumBones );
	for ( int iBone = 0; iBone < ( int ) dwNumBones; iBone++ )
	{
		for ( int iVert = 0; iVert < ( int ) pMesh->pBones [ iBone ].dwNumInfluences; iVert++ )
		{
			int iIndexToVertex = pMesh->pBones [ iBone ].pVertices [ iVert ];
			float fWeight = pMesh->pBones [ iBone ].pWeights [ iVert ];
			pfWorkWeight [ (iIndexToVertex*dwNumBones)+iBone ] = fWeight;
		}
	}
	int maxBonesAttachedToVertexOfMesh = 0;
	for ( DWORD iIndexToVertex = 0; iIndexToVertex < dwNumVertices; iIndexToVertex++ )
	{
		int countBonesAttachedToVertex = 0;
		for (int iTryEightBones = 0; iTryEightBones < 8; iTryEightBones++ )
		{
			int iBestBone = -1;
			float fBest = 0.0f;
			for ( int iBone = 0; iBone < ( int ) dwNumBones; iBone++ )
			{
				float fWeight = pfWorkWeight [ (iIndexToVertex*dwNumBones)+iBone ];
				if ( fWeight>fBest ) { fBest=fWeight; iBestBone=iBone; }
			}
			if ( iBestBone!=-1)
			{
				pfWorkWeight [ (iIndexToVertex*dwNumBones)+iBestBone ]=0.0f;
				countBonesAttachedToVertex++;
			}
			else
				break;
		}
		if ( countBonesAttachedToVertex > maxBonesAttachedToVertexOfMesh ) 
			maxBonesAttachedToVertexOfMesh = countBonesAttachedToVertex;
	}
	SAFE_DELETE(pfWorkWeight);
	if ( maxBonesAttachedToVertexOfMesh > 4 )
		return true;
	else
		return false;
}

