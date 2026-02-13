void ChangeToNewDeclaration ( int iID, DWORD* pdwDeclaration, DWORD dwOrFVF )
{
	// see if the model exists
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return;

	// get a pointer to the start mesh
	sMesh* pMesh = m_pModelData->m_Object.m_Meshes;
	for ( int iTemp = 0; iTemp < m_pModelData->m_Object.m_NumMeshes; iTemp++ )
	{
		// modify the mesh data
		LPD3DXMESH   pOriginalMesh = pMesh->m_Mesh;		// pointer to first mesh
		LPD3DXMESH   pNewMesh      = NULL;				// new mesh

		// clone the mesh
		if(dwOrFVF>0)
			pOriginalMesh->CloneMeshFVF ( 0, dwOrFVF, m_pD3D, &pNewMesh );
		else
			pOriginalMesh->CloneMesh ( 0, pdwDeclaration, m_pD3D, &pNewMesh );

		// only if successful
		if ( pNewMesh )
		{
			// free up the original mesh
			SAFE_RELEASE ( pOriginalMesh );

			// get rid of the original attribute table
			SAFE_DELETE_ARRAY ( pMesh->m_pAttributeTable );

			// store pointer to new mesh
			pMesh->m_Mesh = pNewMesh;

			// get new attribute table
			pMesh->m_Mesh->Optimize ( D3DXMESHOPT_ATTRSORT, NULL, NULL, NULL, NULL, &pNewMesh );
			SAFE_RELEASE ( pMesh->m_Mesh );
			pMesh->m_Mesh = pNewMesh;
			
			// get the size
			pMesh->m_Mesh->GetAttributeTable ( NULL, &pMesh->m_dwAttributeTableSize );

			// create new table
			pMesh->m_pAttributeTable = new D3DXATTRIBUTERANGE [ pMesh->m_dwAttributeTableSize ];

			// get table data
			pMesh->m_Mesh->GetAttributeTable ( pMesh->m_pAttributeTable, NULL );

			// update fvf information
			m_pModelData->m_Object.m_dwFVF = pMesh->m_Mesh->GetFVF ( );
			m_pModelData->m_Object.m_dwFVFSize = D3DXGetFVFVertexSize( m_pModelData->m_Object.m_dwFVF );
		}

		// next mesh
		pMesh = pMesh->m_Next;
	}
}

void HideLimb ( int iID, int iLimbID )
{
	// hides a limb

	// pointer to frame data
	sFrame* pFrame = NULL;

	// see if the model exists
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return;

	// now see if the limb exists
	if ( ! ( pFrame = m_pModelData->m_Object.m_Frames->FindFrame ( iLimbID ) ) )
		return;

	// we can now set the property of a limb, in this case we're
	// setting the visible property to false
	pFrame->m_Limb.bVisible = false;
}

void ShowLimb ( int iID, int iLimbID )
{
	// show a limb

	// pointer to frame data
	sFrame* pFrame = NULL;

	// see if the model exists
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return;

	// now see if the limb exists
	if ( ! ( pFrame = m_pModelData->m_Object.m_Frames->FindFrame ( iLimbID ) ) )
		return;

	// make the limb visible
	pFrame->m_Limb.bVisible = true;
}

void OffsetLimb ( int iID, int iLimbID, float fX, float fY, float fZ )
{
	// offset a limbs position

	// pointer to frame data
	sFrame* pFrame = NULL;

	// see if the model exists
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return;

	// now see if the limb exists
	if ( ! ( pFrame = m_pModelData->m_Object.m_Frames->FindFrame ( iLimbID ) ) )
		return;

	// setup the offset position
	pFrame->m_Limb.vecOffset = D3DXVECTOR3 ( fX, fY, fZ );
}

void RotateLimb ( int iID, int iLimbID, float fX, float fY, float fZ )
{
	// rotates a limb around it's 3 axes

	// pointer to frame data
	sFrame* pFrame = NULL;

	// see if the model exists
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return;

	// now see if the limb exists
	if ( ! ( pFrame = m_pModelData->m_Object.m_Frames->FindFrame ( iLimbID ) ) )
		return;

	// setup the rotation angles
	pFrame->m_Limb.vecRotate = D3DXVECTOR3 ( fX, fY, fZ );
}

void ScaleLimb ( int iID, int iLimbID, float fX, float fY, float fZ )
{
	// scale a limb

	// pointer to frame data
	sFrame* pFrame = NULL;

	// see if the model exists
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return;

	// now see if the limb exists
	if ( ! ( pFrame = m_pModelData->m_Object.m_Frames->FindFrame ( iLimbID ) ) )
		return;

	// setup the scale values
	pFrame->m_Limb.vecScale = D3DXVECTOR3 ( fX/100.0f, fY/100.0f, fZ/100.0f );
}

void AddLimb ( int iID, int iLimbID, int iMeshID )
{
	// variable declarations
	tagModelData* pMeshObj		= NULL;	// mesh object
	sMesh* pActualMesh			= NULL;	// mesh data

	// see if the model exists
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return;

	// fail if already exists
	if ( ( m_pModelData->m_Object.m_Frames->FindFrame ( iLimbID ) ) )
		return;

	// get mesh object
	if ( ! ( pMeshObj = ( tagModelData* ) m_MeshList.Get ( iMeshID ) ) )
		return;

	// get first mesh from mesh object
	if ( ! ( pActualMesh = pMeshObj->m_Object.m_Meshes ) )
		return;
	
	// use mesh FVF to control objectmesh creation
	DWORD dwInFVF = m_pModelData->m_Object.m_dwFVF;
	DWORD dwInFVFSize = m_pModelData->m_Object.m_dwFVFSize;

	// determine info on sourcemesh
	DWORD dwInNumPoly=pActualMesh->m_pAttributeTable [ 0 ].FaceCount;
	DWORD dwInNumVert=pActualMesh->m_pAttributeTable [ 0 ].VertexCount;

	// convert mesh first
	ID3DXMesh* pNewMesh;
	pActualMesh->m_Mesh->CloneMeshFVF ( 0, dwInFVF, m_pD3D, &pNewMesh );

	// create copy of meshdata
	float* pDataFromNewMesh;
	float* pInMesh = (float*)new char[dwInNumVert*dwInFVFSize];
	if ( SUCCEEDED ( pNewMesh->LockVertexBuffer ( D3DLOCK_NOSYSLOCK, ( BYTE** ) &pDataFromNewMesh ) ) )
	{
		memcpy(pInMesh, pDataFromNewMesh, dwInNumVert*dwInFVFSize);
		pNewMesh->UnlockVertexBuffer ( );
	}

	// create new frame from mesh
	sMesh* Mesh = MakeMeshFromData ( dwInFVF, dwInFVFSize, pInMesh, dwInNumVert, D3DPT_TRIANGLELIST );

	// free meshdata
	SAFE_RELEASE(pNewMesh);
	SAFE_DELETE(pInMesh);

	// link in mesh
	Mesh->m_Next    = m_pModelData->m_Object.m_Meshes;
	m_pModelData->m_Object.m_Meshes = Mesh;
	m_pModelData->m_Object.m_NumMeshes++;

	// create a new frame
	sFrame* pFrame = new sFrame ( );

	// frame details
	pFrame->m_iID = iLimbID;
	pFrame->m_Name = new char [ 7 ];
	strcpy ( pFrame->m_Name, "%NEW%" );
	pFrame->AddMesh ( Mesh );
	pFrame->m_bFree=true;

	// Add to end of framelist
	sFrame* pCurrent = m_pModelData->m_Object.m_Frames;
	while(pCurrent)
	{
		if(pCurrent->m_Sibling==NULL)
		{
			pCurrent->m_Sibling = pFrame;
			break;
		}
		else
			pCurrent=pCurrent->m_Sibling;
	}

	// increase frame count if applicable
	if ( iLimbID+1 > m_pModelData->m_Object.m_NumFrames )
		m_pModelData->m_Object.m_NumFrames = iLimbID+1;
}

void RemoveFrameFromHierarchy ( tagModelData* m_pModelData, sFrame* pFrame )
{
	// get parent of limb
	sFrame* pParent = pFrame->m_Parent;
	sFrame* pStartOfSybChain = NULL;
	if(pParent==NULL)
		pStartOfSybChain = m_pModelData->m_Object.m_Frames;
	else
		pStartOfSybChain = pParent->m_Child;

	// remove limb from symbling chain
	sFrame* pLastSyb = NULL;
	sFrame* pSybChain = pStartOfSybChain;
	while(pSybChain)
	{
		sFrame* pNextSyb = pSybChain->m_Sibling;
		if(pSybChain==pFrame)
		{
			if(pSybChain==pStartOfSybChain)
			{
				if(pParent==NULL)
					m_pModelData->m_Object.m_Frames = pNextSyb;
				else
					pParent->m_Child = pNextSyb;
			}
			else
				pLastSyb->m_Sibling = pFrame->m_Sibling;

			break;
		}
		pLastSyb = pSybChain;
		pSybChain = pNextSyb;
	}
	
	// remove all family links to model
	pFrame->m_Parent=NULL;
	pFrame->m_Sibling=NULL;
}

void RemoveLimb ( int iID, int iLimbID )
{
	// pointer to frame data
	sFrame* pFrame = NULL;

	// see if the model exists
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return;

	// now see if the limb exists
	if ( ! ( pFrame = m_pModelData->m_Object.m_Frames->FindFrame ( iLimbID ) ) )
		return;

	// remove limb from symbling chain
	RemoveFrameFromHierarchy ( m_pModelData, pFrame );

	// delete frame allocations (this and children of limb)
	SAFE_DELETE(pFrame);
}

void LinkLimb ( int iID, int iLimbID, int iParentID )
{
	// Rule : can only add new limb (so cannot create a recursive-infinite-nest)

	// find obj responsible
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return;

	// find target limb
	sFrame* pTargetFrame = NULL;
	if ( ! ( pTargetFrame = m_pModelData->m_Object.m_Frames->FindFrame ( iParentID ) ) )
		return;

	// find free limb
	sFrame* pFreeFrame = NULL;
	if ( ! ( pFreeFrame = m_pModelData->m_Object.m_Frames->FindFrame ( iLimbID ) ) )
		return;

	// check free limb state
	if ( pFreeFrame->m_bFree==false )
		return;

	// free limb cannot be root limb
	if(m_pModelData->m_Object.m_Frames==pFreeFrame)
		return;

	// remove limb from symbling chain
	RemoveFrameFromHierarchy ( m_pModelData, pFreeFrame );

	// attach limb to parent limb
	pFreeFrame->m_Parent = pTargetFrame;
	pFreeFrame->m_Sibling = NULL;

	// make parent aware of new child
	if(pTargetFrame->m_Child)
	{
		// child part of sybling chain
		sFrame* pCurrent = pTargetFrame->m_Child;
		while(pCurrent)
		{
			if(pCurrent->m_Sibling==NULL)
			{
				pCurrent->m_Sibling = pFreeFrame;
			}
			pCurrent = pCurrent->m_Sibling;
		}
	}
	else
	{
		// first child of parent
		pTargetFrame->m_Child = pFreeFrame;
	}

	// limb no longer free
	pFreeFrame->m_bFree=false;
}

void SetTexture ( int iID, LPDIRECT3DTEXTURE8 pTexture )
{
	// set the texture of an object, this is applied to all
	// limbs contained within the model

	// variable declarations
	sFrame* pFrame = NULL;		// pointer to frame data
	int		iTemp  = 0;			// used for loops
	
	// see if the model exists
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return;

	// run through all frames
	for ( iTemp = 0; iTemp < m_pModelData->m_Object.m_NumFrames; iTemp++ )
	{
		// see if the frame exists
		if ( ! ( pFrame = m_pModelData->m_Object.m_Frames->FindFrame ( iTemp ) ) )
			return;

		// check we have a valid mesh list
		if ( pFrame->m_MeshList )
		{
			pFrame->m_MeshList->m_Mesh->m_Textures [ 0 ] = pTexture;
		}
	}
	
	// sometimes a model may have no frames of animation and just be 
	// 1 segment, we deal with this case here
	if ( m_pModelData->m_Object.m_NumFrames == 0 )
	{
		// delete the original texture array, it may not exist so we do this just in case,
		// then we recreate the texture list
		SAFE_DELETE_ARRAY ( m_pModelData->m_Object.m_Meshes->m_Textures );

		// allocate memory for the textures
		m_pModelData->m_Object.m_Meshes->m_Textures = new LPDIRECT3DTEXTURE8 [ m_pModelData->m_Object.m_Meshes->m_NumMaterials ];

		// run through the materials and setup the texture
		for ( int iTemp = 0; iTemp <  (int)m_pModelData->m_Object.m_Meshes->m_NumMaterials; iTemp++ )
			m_pModelData->m_Object.m_Meshes->m_Textures [ iTemp ] = pTexture;
	}
}

int GetFrames ( int iID )
{
	// see if the model exists
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return -1;

	return m_pModelData->m_Object.m_NumFrames;
}

float GetXSize ( int iID )
{
	// see if the model exists
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return 0.0f;

	return m_pModelData->m_Object.m_Max.x;
}

float GetYSize ( int iID )
{
	// see if the model exists
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return 0.0f;

	return m_pModelData->m_Object.m_Max.y;
}

float GetZSize ( int iID )
{
	// see if the model exists
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return 0.0f;

	return m_pModelData->m_Object.m_Max.z;
}

void ChangeFrameMeshData ( sFrame* pFrame, int iTask, float fA, float fB, float fC )
{
	if ( !pFrame->m_MeshList )
		return;
	if ( !pFrame->m_MeshList->m_Mesh )
		return;
	if ( !pFrame->m_MeshList->m_Mesh->m_Mesh )
		return;

	// Get Vertex Info
	DWORD dwNumVertices = pFrame->m_MeshList->m_Mesh->m_Mesh->GetNumVertices ( );
	DWORD dwFVF         = pFrame->m_MeshList->m_Mesh->m_Mesh->GetFVF ( );

	// need diffuse for colour
	if ( iTask==1 && !(dwFVF & D3DFVF_DIFFUSE) )
		return;

	// need tex for UVscroll
	if ( iTask==2 && !(dwFVF & D3DFVF_TEX1) )
		return;

	// need tex for UVscale
	if ( iTask==3 )
	{
		if ( !(dwFVF & D3DFVF_TEX1) )
			return;

		// scale 100 -> 1.0f
		fA/=100.0f;
		fB/=100.0f;
	}

	// need normals for Fade
	if ( iTask==4 )
	{
		if( !(dwFVF & D3DFVF_NORMAL) )
			return;

		// fade 100 -> 1.0f
		fA/=100.0f;
		fB/=100.0f;
		fC/=100.0f;
	}

	// need diffuse for fade
	if ( iTask==5 )
	{
		if( !(dwFVF & D3DFVF_DIFFUSE) )
			return;

		// fade 100 -> 1.0f
		fA/=100.0f;
		fB/=100.0f;
		fC/=100.0f;
	}

	// Calculate Vertex Offsets
	DWORD dwFVFSize     = 0;
	DWORD dwXYZOffset = dwFVFSize;
	if ( dwFVF & D3DFVF_XYZ ) dwFVFSize+=12;
	DWORD dwNORMALOffset = dwFVFSize;
	if ( dwFVF & D3DFVF_NORMAL ) dwFVFSize+=12;
	DWORD dwDIFFUSEOffset = dwFVFSize;
	if ( dwFVF & D3DFVF_DIFFUSE ) dwFVFSize+=4;
	DWORD dwTEXOffset = dwFVFSize;
	if ( dwFVF & D3DFVF_TEX1 ) dwFVFSize+=8;

	// Change Diffuse Data
	LPSTR pVertices = NULL;
	if ( SUCCEEDED ( pFrame->m_MeshList->m_Mesh->m_Mesh->LockVertexBuffer ( D3DLOCK_NOSYSLOCK , ( BYTE** ) &pVertices ) ) )
	{
		// go through each triangle and set the diffuse component
		for ( int iTemp = 0; iTemp < (int)dwNumVertices; iTemp++ )
		{
			// do task to vertex
			switch(iTask)
			{
				case 1 : {	// new diffuse colour
							DWORD* pDiffuse = (DWORD*)(pVertices+dwDIFFUSEOffset);
							*pDiffuse = D3DCOLOR_ARGB ( 255, (DWORD)fA, (DWORD)fB, (DWORD)fC );
							break;
						 }

				case 2 : {	// scroll UV
							float* pU = (float*)(pVertices+dwTEXOffset);
							float* pV = (float*)(pVertices+dwTEXOffset+4);
							if ( *pU > 1.0e15f ) *pU = 0.0f; else *pU = *pU + fA;
							if ( *pV > 1.0e15f ) *pV = 0.0f; else *pV = *pV + fB;
							break;
						 }

				case 3 : {	// scale UV
							float* pU = (float*)(pVertices+dwTEXOffset);
							float* pV = (float*)(pVertices+dwTEXOffset+4);
							*pU = *pU * fA;
							*pV = *pV * fB;
							break;
						 }

				case 4 : {	// Fade using normals
							float* pX = (float*)(pVertices+dwNORMALOffset);
							float* pY = (float*)(pVertices+dwNORMALOffset+4);
							float* pZ = (float*)(pVertices+dwNORMALOffset+8);
							*pX = *pX * fA;
							*pY = *pY * fB;
							*pZ = *pZ * fC;
							break;
						 }

				case 5 : {	// Fade using diffuse
							DWORD* pDiffuse = (DWORD*)(pVertices+dwDIFFUSEOffset);
							*pDiffuse = D3DCOLOR_ARGB ( 255, (DWORD)(fA*255.0f), (DWORD)(fB*255.0f), (DWORD)(fC*255.0f) );
							break;
						 }
			}

			// next vertex
			pVertices+=dwFVFSize;
		}
		pFrame->m_MeshList->m_Mesh->m_Mesh->UnlockVertexBuffer ( );
	}
}

void Color ( int iID, int iR, int iG, int iB )
{
	// see if the model exists
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return;

	// run through all frames
	sFrame* pFrame        = NULL;
	for ( int iFrame = 0; iFrame < m_pModelData->m_Object.m_NumFrames; iFrame++ )
	{
		// see if the frame exists
		if ( ! ( pFrame = m_pModelData->m_Object.m_Frames->FindFrame ( iFrame ) ) )
			return;

		// modify mesh within frame
		ChangeFrameMeshData ( pFrame, 1, (float)iR, (float)iG, (float)iB );
	}
}

void Fade ( int iID, float fPercentage )
{
	// see if the model exists
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return;

	// run through all frames
	sFrame* pFrame        = NULL;
	for ( int iFrame = 0; iFrame < m_pModelData->m_Object.m_NumFrames; iFrame++ )
	{
		// see if the frame exists
		if ( ! ( pFrame = m_pModelData->m_Object.m_Frames->FindFrame ( iFrame ) ) )
			return;

		// modify mesh within frame
		ChangeFrameMeshData ( pFrame, 5, fPercentage, fPercentage, fPercentage );
	}
}

void ScrollTexture ( int iID, float fU, float fV )
{
	sFrame* pFrame = NULL;

	// see if the model exists
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return;

	if ( m_pModelData->m_Object.m_Meshes==NULL )
		return;

	// disable scrolling of non limb objects
	if ( m_pModelData->m_Object.m_Meshes->m_NumBones > 0 )
		return;

	// run through all frames
	for ( int iFrame = 0; iFrame < m_pModelData->m_Object.m_NumFrames; iFrame++ )
	{
		// see if the frame exists
		if ( ! ( pFrame = m_pModelData->m_Object.m_Frames->FindFrame ( iFrame ) ) )
			return;

		// modify mesh within frame
		ChangeFrameMeshData ( pFrame, 2, fU, fV, 0.0f );
	}
}

void ScaleTexture ( int iID, float fU, float fV )
{
	sFrame* pFrame = NULL;

	// see if the model exists
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return;

	if ( m_pModelData->m_Object.m_Meshes==NULL )
		return;

	// disable scrolling of non limb objects
	if ( m_pModelData->m_Object.m_Meshes->m_NumBones > 0 )
		return;

	// run through all frames
	for ( int iFrame = 0; iFrame < m_pModelData->m_Object.m_NumFrames; iFrame++ )
	{
		// see if the frame exists
		if ( ! ( pFrame = m_pModelData->m_Object.m_Frames->FindFrame ( iFrame ) ) )
			return;

		// modify mesh within frame
		ChangeFrameMeshData ( pFrame, 3, fU, fV, 0.0f );
	}
}

void TextureLimb ( int iID, int iLimbID, int iImageID )
{
	// set the texture of a limb

	// pointer to frame data
	sFrame* pFrame = NULL;
	
	// see if the model exists
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return;

	// disable scrolling of non limb objects
	if ( m_pModelData->m_Object.m_Meshes->m_NumBones > 0 )
		return;

	// now see if the limb exists
	if ( ! ( pFrame = m_pModelData->m_Object.m_Frames->FindFrame ( iLimbID ) ) )
		return;

	// set the texture pointer
	//pFrame->m_MeshList->m_Mesh->m_Textures [ 0 ] = g_Image_GetPointer ( iImageID );
	pFrame->m_MeshList->m_Mesh->m_Textures [ 0 ] = NULL;
}

void ColorLimb ( int iID, int iLimbID, DWORD dwColor )
{
	// set the color of a limb

	// pointer to frame data
	sFrame* pFrame = NULL;
	
	// see if the model exists
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return;

	// disable scrolling of non limb objects
	if ( m_pModelData->m_Object.m_Meshes->m_NumBones > 0 )
		return;

	// now see if the limb exists
	if ( ! ( pFrame = m_pModelData->m_Object.m_Frames->FindFrame ( iLimbID ) ) )
		return;

	// break down the dwColor variable into individual components
	DWORD dwRed   = ( ( ( dwColor ) >> 16)       & 0xff );
	DWORD dwGreen = ( ( ( dwColor ) >>  8 ) & 0xff );
	DWORD dwBlue  = ( ( ( dwColor )  ) & 0xff );

	// modify mesh within frame
	ChangeFrameMeshData ( pFrame, 1, (float)dwRed, (float)dwGreen, (float)dwBlue );
}

void ScrollLimbTexture ( int iID, int iLimbID, float fU, float fV )
{
	// scrolls texture of a limb

	// pointer to frame data
	sFrame* pFrame        = NULL;
	BYTE**	Ptr           = NULL;
	DWORD	dwNumVertices = 0;
	DWORD   dwFVF         = 0;
	int     iCount        = 0;
	int		iTemp	      = 0;

	// see if the model exists
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return;
	
	// disable scrolling of non limb objects
	if ( m_pModelData->m_Object.m_Meshes->m_NumBones > 0 )
		return;

	// now see if the limb exists
	if ( ! ( pFrame = m_pModelData->m_Object.m_Frames->FindFrame ( iLimbID ) ) )
		return;

	// modify mesh within frame
	ChangeFrameMeshData ( pFrame, 2, fU, fV, 0.0f );
}

void ScaleLimbTexture ( int iID, int iLimbID, float fU, float fV )
{
	// scrolls texture of a limb

	// pointer to frame data
	sFrame* pFrame        = NULL;
	BYTE**	Ptr           = NULL;
	DWORD	dwNumVertices = 0;
	DWORD   dwFVF         = 0;
	int     iCount        = 0;
	int		iTemp	      = 0;

	// see if the model exists
	if ( ! ( m_pModelData = ( tagModelData* ) m_List.Get ( iID ) ) )
		return;

	// disable scrolling of non limb objects
	if ( m_pModelData->m_Object.m_Meshes->m_NumBones > 0 )
		return;

	// now see if the limb exists
	if ( ! ( pFrame = m_pModelData->m_Object.m_Frames->FindFrame ( iLimbID ) ) )
		return;

	// modify mesh within frame
	ChangeFrameMeshData ( pFrame, 3, fU, fV, 0.0f );
}

// Data Access Functions

void GetMeshData( int iMeshID, DWORD* pdwFVF, DWORD* pdwFVFSize, DWORD* pdwVertMax, LPSTR* pData, DWORD* dwDataSize, bool bLockData )
{
	// Read Data
	if(bLockData==true)
	{
		// makes a mesh from a meshobject
		tagModelData* pMeshObj = NULL;
		if ( ! ( pMeshObj = ( tagModelData* ) m_MeshList.Get ( iMeshID ) ) )
			return;

		// get first mesh from mesh object
		sMesh* pActualMesh			= NULL;	// mesh data
		if ( ! ( pActualMesh = pMeshObj->m_Object.m_Meshes ) )
			return;

		// use mesh FVF to control objectmesh creation
		DWORD dwInFVF = pMeshObj->m_Object.m_dwFVF;
		DWORD dwInFVFSize = pMeshObj->m_Object.m_dwFVFSize;
		DWORD dwInNumPoly=pActualMesh->m_pAttributeTable [ 0 ].FaceCount;
		DWORD dwInNumVert=pActualMesh->m_pAttributeTable [ 0 ].VertexCount;

		// convert mesh first
		ID3DXMesh* pNewMesh;
		pActualMesh->m_Mesh->CloneMeshFVF ( 0, dwInFVF, m_pD3D, &pNewMesh );
		ID3DXMesh* pWorkMesh = pNewMesh;
	
		// create copy of meshdata
		float* pDataFromNewMesh;
		float* pInMesh = (float*)new char[dwInNumVert*dwInFVFSize];
		if ( SUCCEEDED ( pWorkMesh->LockVertexBuffer ( D3DLOCK_NOSYSLOCK, ( BYTE** ) &pDataFromNewMesh ) ) )
		{
			memcpy(pInMesh, pDataFromNewMesh, dwInNumVert*dwInFVFSize);
			pWorkMesh->UnlockVertexBuffer ( );
		}

		// create copy of meshdata
		WORD* pIndicesFromNewMesh;
		WORD* pInIndices = new WORD[dwInNumPoly*3];
		if ( SUCCEEDED ( pWorkMesh->LockIndexBuffer ( D3DLOCK_NOSYSLOCK, ( BYTE** ) &pIndicesFromNewMesh ) ) )
		{
			memcpy(pInIndices, pIndicesFromNewMesh, dwInNumPoly*3*sizeof(WORD));
			pWorkMesh->UnlockIndexBuffer ( );
		}

		// make mesh data not depend on indices
		DWORD dwNewVertCount=0;
		float* pNoIndexRequiredMesh = CreatePureTriangleMeshData( pInMesh, &dwNewVertCount, dwInNumVert, dwInFVFSize, pInIndices, dwInNumPoly );
		SAFE_DELETE(pInIndices);
		SAFE_DELETE(pInMesh);

		// mesh data
		*pdwFVF = dwInFVF;
		*pdwFVFSize = dwInFVFSize;
		*pdwVertMax = dwNewVertCount;

		// create memory
		DWORD dwSizeOfData = dwNewVertCount*dwInFVFSize;
		*pData = new char[dwSizeOfData];
		*dwDataSize = dwSizeOfData;

		// copy mesh to new memory
		memcpy( *pData, pNoIndexRequiredMesh, dwSizeOfData );

		// free usages
		SAFE_DELETE(pNoIndexRequiredMesh);

		// free work mesh
		SAFE_RELEASE(pWorkMesh);
	}
	else
	{
		// free memory
		delete *pData;
	}
}

void SetMeshData( int iMeshID, DWORD dwFVF, DWORD dwFVFSize, LPSTR pMeshData, DWORD dwVertMax )
{
	// zero mesh not perimitted
	if ( iMeshID<=0 )
		return;

	// create new mesh-object from single mesh
	MakeMeshFromMesh ( iMeshID, dwFVF, dwFVFSize, (float*)pMeshData, dwVertMax/3, dwVertMax, D3DPT_TRIANGLELIST );
}
