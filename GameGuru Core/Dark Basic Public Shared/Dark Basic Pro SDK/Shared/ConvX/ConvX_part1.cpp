DARKSDK bool XFILE_NEW_SetupTextures ( sMesh* pMesh )
{
	// mesh texture data
	if ( !pMesh )
		return false;

	// LEELEE - I can see g_MaterialList and g_pMaterialList - very confusing to know which is which!

	// number of loaddata materials
	DWORD dwCount = g_NewMaterialList.size ( ); 

	// at least one texture must be created
	if ( dwCount < 1 ) dwCount=1;

	// individual materials to each face (optional-see origin)
	if ( g_pMaterialList )
	{
		// create array to store material indexes within mesh
		DWORD dwNumberOfFaces = pMesh->dwIndexCount / 3;
		pMesh->pAttributeWorkData = new DWORD [ dwNumberOfFaces ];

		// copy actual index data from filedata
		memcpy ( pMesh->pAttributeWorkData, g_pMaterialList, sizeof ( DWORD ) * g_dwMaterialListIndexCount );

		// LEELEE - notice the g_dwMaterialListIndexCount is misleading, as it should be in FaceCount (ie DWORD nFaceIndexes)
		// LEEFIX - 171003 - Material List must be padded with last material index (for cases of 1,1,0)
		if ( g_dwMaterialListIndexCount < dwNumberOfFaces )
		{
			DWORD dwPadIndex = g_pMaterialList [ g_dwMaterialListIndexCount-1 ];
			for ( DWORD dwIndex=g_dwMaterialListIndexCount; dwIndex<dwNumberOfFaces; dwIndex++ )
				pMesh->pAttributeWorkData [ dwIndex ] = dwPadIndex;
		}
	}

	// create texture array for mesh
	pMesh->dwTextureCount = dwCount;
	pMesh->pTextures      = new sTexture [ dwCount ];

	// if loaddata has material properties, store them in mesh
	if ( g_NewMaterialList.size ( ) > 0 )
	{
		// material bank holds all materials
		pMesh->pMaterialBank = new GGBASEMATERIAL [ dwCount ];
		memset ( pMesh->pMaterialBank, 0, sizeof(GGBASEMATERIAL) * dwCount );

		// go through each material in bank
		for ( int iTexture = 0; iTexture < ( int ) dwCount; iTexture++ )
		{
			// copy material data from loaddata
			pMesh->pMaterialBank [ iTexture ].MatD3D.Diffuse.r  = g_NewMaterialList [ iTexture ].vecColour.x;
			pMesh->pMaterialBank [ iTexture ].MatD3D.Diffuse.g  = g_NewMaterialList [ iTexture ].vecColour.y;
			pMesh->pMaterialBank [ iTexture ].MatD3D.Diffuse.b  = g_NewMaterialList [ iTexture ].vecColour.z;
			pMesh->pMaterialBank [ iTexture ].MatD3D.Diffuse.a  = g_NewMaterialList [ iTexture ].vecColour.w;
			pMesh->pMaterialBank [ iTexture ].MatD3D.Ambient.r  = g_NewMaterialList [ iTexture ].vecColour.x;
			pMesh->pMaterialBank [ iTexture ].MatD3D.Ambient.g  = g_NewMaterialList [ iTexture ].vecColour.y;
			pMesh->pMaterialBank [ iTexture ].MatD3D.Ambient.b  = g_NewMaterialList [ iTexture ].vecColour.z;
			pMesh->pMaterialBank [ iTexture ].MatD3D.Ambient.a  = g_NewMaterialList [ iTexture ].vecColour.w;
			pMesh->pMaterialBank [ iTexture ].MatD3D.Specular.r = g_NewMaterialList [ iTexture ].vecSpecular.x;
			pMesh->pMaterialBank [ iTexture ].MatD3D.Specular.g = g_NewMaterialList [ iTexture ].vecSpecular.y;
			pMesh->pMaterialBank [ iTexture ].MatD3D.Specular.b = g_NewMaterialList [ iTexture ].vecSpecular.z;
			pMesh->pMaterialBank [ iTexture ].MatD3D.Specular.a = 1.0f;
			pMesh->pMaterialBank [ iTexture ].MatD3D.Emissive.r = g_NewMaterialList [ iTexture ].vecEmissive.x;
			pMesh->pMaterialBank [ iTexture ].MatD3D.Emissive.g = g_NewMaterialList [ iTexture ].vecEmissive.y;
			pMesh->pMaterialBank [ iTexture ].MatD3D.Emissive.b = g_NewMaterialList [ iTexture ].vecEmissive.z;
			pMesh->pMaterialBank [ iTexture ].MatD3D.Emissive.a = 1.0f;
			pMesh->pMaterialBank [ iTexture ].MatD3D.Power = g_NewMaterialList [ iTexture ].fPower;

			// copy material name from loaddata
			if ( g_NewMaterialList [ iTexture ].szFilename )
				strcpy ( pMesh->pTextures [ iTexture ].pName, g_NewMaterialList [ iTexture ].szFilename  );
			else
				strcpy ( pMesh->pTextures [ iTexture ].pName, "" );
		}

		// default material assignments
		memset ( &pMesh->mMaterial, 0, sizeof ( pMesh->mMaterial ) );
		pMesh->mMaterial.Diffuse  = pMesh->pMaterialBank [ 0 ].MatD3D.Diffuse;
		pMesh->mMaterial.Ambient  = pMesh->pMaterialBank [ 0 ].MatD3D.Ambient;
		pMesh->mMaterial.Specular = pMesh->pMaterialBank [ 0 ].MatD3D.Specular;
		pMesh->mMaterial.Emissive = pMesh->pMaterialBank [ 0 ].MatD3D.Emissive;
		pMesh->mMaterial.Power = pMesh->pMaterialBank [ 0 ].MatD3D.Power;
		pMesh->bUsesMaterial = true;
	}
	else
	{
		D3DMATERIAL9PRETEND material;
		material.Diffuse.r = 0.0f;
		material.Diffuse.g = 0.0f;
		material.Diffuse.b = 0.0f;
		material.Diffuse.a = 0.0f;
		material.Ambient	= material.Diffuse;
		pMesh->mMaterial = material;
		pMesh->pMaterialBank = new GGBASEMATERIAL [ 1 ];
		pMesh->pMaterialBank [ 0 ].MatD3D           = material;
		pMesh->pMaterialBank [ 0 ].pTextureFilename = NULL;
		pMesh->bUsesMaterial = true;
	}

	return true;
}

DARKSDK bool XFILE_GetMeshData ( IDirectXFileData* pDataObj, sFrame* pParentFrame )
{
	// gets the mesh data from the X file and places it into the frame structure

	// first off check the parameters are valid
	SAFE_MEMORY ( pDataObj     );		// X file interface
	SAFE_MEMORY ( pParentFrame );		// frame to place data into
	SAFE_MEMORY ( m_pD3D       );		// D3D device

	// delete global variables
	SAFE_DELETE_ARRAY ( g_pVertexList   );
	SAFE_DELETE_ARRAY ( g_pIndexList    );
	SAFE_DELETE_ARRAY ( g_pFaceIndexOldRef );
	SAFE_DELETE_ARRAY ( g_pMaterialList );

	// reset values
	g_dwVertexCount            = 0;
	g_dwFaceCount              = 0;
	g_dwMaterialListIndexCount = 0;
	g_dwMaterialCount          = 0;
	g_dMaxSkinWeightsPerVertex = 0;
	g_dMaxSkinWeightsPerFace   = 0;
	g_dwBones                  = 0;
	g_iNumberOfExtraUVDataStagesAvailable = 0;

	// free resources in bone list
	for ( int i = 0; i < (int)g_BoneList.size(); i++ )
	{
		SAFE_DELETE ( g_BoneList [ i ].pfWeights );
		SAFE_DELETE ( g_BoneList [ i ].pIndices );
		SAFE_DELETE ( g_BoneList [ i ].szName );
	}
	// free resources in material list
	for ( int i = 0; i < (int)g_NewMaterialList.size(); i++ )
		SAFE_DELETE ( g_NewMaterialList [ i ].szFilename );

	// clear lists
	g_BoneList.clear        ( );
	g_NewMaterialList.clear ( );

	XFILE_NEW_GetVerticesAndIndices ( pDataObj, pParentFrame );

	bool				bSearch     = true;			// search flag
	IDirectXFileData*	pDataObject = pDataObj;		// data
	IDirectXFileObject*	pSubObject  = NULL;			// interface to object
	IDirectXFileData*	pSubData    = NULL;			// data pointer
	const GUID*			guidType    = NULL;			// type of data
	char*				szName	    = NULL;			// name

	while ( bSearch )
	{
		// see if a sub object exists
		if ( SUCCEEDED ( pDataObject->GetNextObject ( &pSubObject ) ) )
		{
			// attempt to access the data
			if ( SUCCEEDED ( pSubObject->QueryInterface ( DBIID_IDirectXFileData, ( void** ) &pSubData ) ) )
			{
				// find out the template we're dealing with
				if ( !XFILE_GetTemplateInfo ( pSubData, &guidType, &szName ) )
					return false;

				// normals
				if ( *guidType == TID_D3DRMMeshNormals )
					XFILE_NEW_GetNormals ( pSubData, pParentFrame );
				
				// texture coordinates
				if ( *guidType == TID_D3DRMMeshTextureCoords )
					XFILE_NEW_GetTextureCoordinates ( pSubData, pParentFrame );

				// texture coordinates SECOND UV LAYER (FVFDATA)
				if ( *guidType == DXFILEOBJ_FVFData )
					XFILE_NEW_GetFVFData ( pSubData, pParentFrame );
				
				// vertex colours
				if ( *guidType == TID_D3DRMMeshVertexColors )
					XFILE_NEW_GetVertexColors ( pSubData, pParentFrame );

				// material list
				if ( *guidType == TID_D3DRMMeshMaterialList )
					XFILE_NEW_GetMeshMaterialList ( pSubData, pParentFrame );

				// skin mesh header
				if ( *guidType == DXFILEOBJ_XSkinMeshHeader )
					XFILE_NEW_GetSkinMeshHeader ( pSubData, pParentFrame );

				// bones and weights
				if ( *guidType == DXFILEOBJ_SkinWeights )
					XFILE_NEW_GetSkinWeights ( pSubData, pParentFrame );

				// new U70 - 040908
				// DeclData - vertex declaration and data (replaces FVF mode)
				// can contain any of the above vertex information 
				if ( *guidType == DXFILEOBJ_DeclData  )
					XFILE_NEW_GetDeclData ( pSubData, pParentFrame );

				// MIKE - 311003
				SAFE_DELETE_ARRAY ( szName );
				SAFE_RELEASE ( pSubData );
			}
			else
				bSearch = false;

			SAFE_RELEASE ( pSubObject );
		}
		else
			bSearch = false;
	}

	// when all data gathered, copy normals to vertices ( can be more normals! )
	XFILE_NEW_CopyNormalsToVertices ();
	
	// call set up functions
	XFILE_NEW_CreateMeshInFrame ( pParentFrame, g_dwVertexCount, g_dwFaceCount );
	XFILE_NEW_SetupTextures     ( pParentFrame->pMesh );
	XFILE_NEW_SetupVertexData	( pParentFrame );

	// setup bones for mesh
	XFILE_NEW_SetupBones ( pParentFrame );

	// LEEFIX - 161003 - Simply cannot have more than 16bit's worth of vertices (max 0xFFFF)
	// and use index at same time! So if too many verts, convert to no index buffer version
	if ( g_dwVertexCount > 0xFFFF )
	{
		// unwraps index data and adds it to make larger vertex data
		ConvertLocalMeshToVertsOnly ( pParentFrame->pMesh, false );

		// must also unwrap the bone data which now points to incorrect vertex data
		// and can use the g_pConversionMap generated from the above conversion function
		// handle any mesh associated with this frame
		sMesh* pMesh = pParentFrame->pMesh;
		if ( pMesh )
		{
			for ( DWORD dwBone=0; dwBone<pMesh->dwBoneCount; dwBone++ )
			{
				for ( DWORD dwI=0; dwI<pMesh->pBones[dwBone].dwNumInfluences; dwI++ )
				{
					DWORD dwVIndex = pMesh->pBones[dwBone].pVertices[dwI];
					DWORD dwRelocatedVertexIndex = g_pConversionMap[dwVIndex];
					pMesh->pBones[dwBone].pVertices[dwI] = dwRelocatedVertexIndex;
				}
			}
		}

		// best to free conversion array now
		SAFE_DELETE(g_pConversionMap);
	}
	
	// clean up material list strings
	for ( int i = 0; i < (int)g_NewMaterialList.size ( ); i++ )
		SAFE_DELETE_ARRAY ( g_NewMaterialList [ i ].szFilename );
	g_NewMaterialList.clear();

	return true;
}

DARKSDK bool XFILE_CreateAnimationSet ( sAnimationSet** ppAnimSet, sAnimationSet** ppSubAnimSet, char* szName )
{
	// create an animation set for an object

	// create a new animation set
	*ppAnimSet = new sAnimationSet;

	// check to see the memory was allocated and make sure the name is valid
	SAFE_MEMORY ( *ppAnimSet );
	SAFE_MEMORY ( szName     );

	// store the animation name
	strcpy ( ( *ppAnimSet )->szName, szName );

	// link into the animation set list
	( *ppAnimSet )->pNext  = g_pObjectX->pAnimationSet;
	g_pObjectX->pAnimationSet = *ppAnimSet;
	
	// store a pointer to the anim set in sub anim set
	*ppSubAnimSet = *ppAnimSet;

	return true;
}

DARKSDK bool XFILE_CreateAnimation ( sAnimation** ppAnim, sAnimation** ppSubAnim, sAnimationSet* pParentAnim, char* szName )
{
	// create an animation for an object

	// create a new animation
	*ppAnim = new sAnimation;

	// make sure memory is okay
	SAFE_MEMORY ( *ppAnim     );
	SAFE_MEMORY ( szName      );
	SAFE_MEMORY ( pParentAnim );

	// store animation name
	strcpy ( ( *ppAnim )->szName, szName );

	// link into the animation list
	( *ppAnim )->pNext      = pParentAnim->pAnimation;
	pParentAnim->pAnimation = *ppAnim;

	// store the sub animation
	*ppSubAnim = *ppAnim;

	return true;
}

DARKSDK bool XFILE_SetupRotationAnimation ( PBYTE* pDataPtr, sAnimation* pAnim, sAnimationSet* pParentAnim, DWORD dwNumKeys )
{
	// setup animation for the rotation

	// check the parameters
	SAFE_MEMORY ( pDataPtr    );
	SAFE_MEMORY ( pAnim       );
	SAFE_MEMORY ( pParentAnim );

	// rotation key
	sXFileRotateKey* pRotKey;

	// delete any current rotation keys
	SAFE_DELETE_ARRAY ( pAnim->pRotateKeys );

	// create a new set of rotation keys
	pAnim->pRotateKeys = new sRotateKey [ dwNumKeys ];

	// check the memory
	SAFE_MEMORY ( pAnim->pRotateKeys );

	// store the number of rotation keys
	pAnim->dwNumRotateKeys = dwNumKeys;
	pRotKey                = ( sXFileRotateKey* ) ( ( char* ) pDataPtr + ( sizeof ( DWORD ) * 2 ) );
	
	// run through all of the keys (this data is time sorted later in object DLL)
	for ( int iKey = 0; iKey < ( int ) dwNumKeys; iKey++ )
	{
		pAnim->pRotateKeys [ iKey ].dwTime       = pRotKey->dwTime;	// get the time
		//if(0)
		//{
		//	// new 2016 way based on imported from FBX > X (AssImp) ???
		//	pAnim->pRotateKeys [ iKey ].Quaternion.x = pRotKey->x;		// x rotation
		//	pAnim->pRotateKeys [ iKey ].Quaternion.y = pRotKey->y;		// y rotation
		//	pAnim->pRotateKeys [ iKey ].Quaternion.z = pRotKey->z;		// z rotation
		//}
		//else
		{
			// legacy method of bringing in quats from X files
			pAnim->pRotateKeys [ iKey ].Quaternion.x = -pRotKey->x;		// x rotation
			pAnim->pRotateKeys [ iKey ].Quaternion.y = -pRotKey->y;		// y rotation
			pAnim->pRotateKeys [ iKey ].Quaternion.z = -pRotKey->z;		// z rotation
		}
		pAnim->pRotateKeys [ iKey ].Quaternion.w =  pRotKey->w;		// w rotation

		// check the length
		if ( pRotKey->dwTime > pParentAnim->ulLength )
			pParentAnim->ulLength = pRotKey->dwTime;

		// move to the next rotation key
		pRotKey++;
	}

	// all went okay
	return true;
}

DARKSDK bool XFILE_SetupScalingAnimation ( PBYTE* pDataPtr, sAnimation* pAnim, sAnimationSet* pParentAnim, DWORD dwNumKeys )
{
	// sort out scaling keys

	// check the parameters
	SAFE_MEMORY ( pDataPtr    );
	SAFE_MEMORY ( pAnim       );
	SAFE_MEMORY ( pParentAnim );

	// scale key
	sXFileScaleKey* pScaleKey = NULL;

	// delete any current scale keys
	SAFE_DELETE_ARRAY ( pAnim->pScaleKeys );

	// create a new set of scale keys
	pAnim->pScaleKeys = new sScaleKey [ dwNumKeys ];

	// check the newly allocated memory
	SAFE_MEMORY ( pAnim->pScaleKeys );

	// store the number of scale keys
	pAnim->dwNumScaleKeys = dwNumKeys;
	pScaleKey             = ( sXFileScaleKey* ) ( ( char* ) pDataPtr + ( sizeof ( DWORD ) * 2 ) );
	
	// go through all keys (this data is time sorted later in object DLL)
	for ( int iKey = 0; iKey < ( int ) dwNumKeys; iKey++ )
	{
		pAnim->pScaleKeys [ iKey ].dwTime   = pScaleKey->dwTime;
		pAnim->pScaleKeys [ iKey ].vecScale = pScaleKey->vecScale;

		if ( pScaleKey->dwTime > pParentAnim->ulLength )
			pParentAnim->ulLength = pScaleKey->dwTime;

		pScaleKey++;
	}

	return true;
}

DARKSDK bool XFILE_SetupPositionAnimation ( PBYTE* pDataPtr, sAnimation* pAnim, sAnimationSet* pParentAnim, DWORD dwNumKeys )
{
	// set up animation with position keys

	// first off check the memory we will access
	SAFE_MEMORY ( pDataPtr    );
	SAFE_MEMORY ( pAnim       );
	SAFE_MEMORY ( pParentAnim );

	// scale key
	sXFilePositionKey* pPosKey = NULL;

	// clear out any current position keys
	SAFE_DELETE_ARRAY ( pAnim->pPositionKeys );

	// allocate a new array of position keys
	pAnim->pPositionKeys = new sPositionKey [ dwNumKeys ];

	// check they were allocated correctly
	SAFE_MEMORY ( pAnim->pPositionKeys );

	// store the number of keys and get a pointer to the data
	pAnim->dwNumPositionKeys = dwNumKeys;
	pPosKey                  = ( sXFilePositionKey* ) ( ( char* ) pDataPtr + ( sizeof ( DWORD ) * 2 ) );
	
	//  all keyframes (this data is time sorted later in object DLL)
	for ( int iKey = 0; iKey < ( int ) dwNumKeys; iKey++ )
	{
		pAnim->pPositionKeys [ iKey ].dwTime = pPosKey->dwTime;
		pAnim->pPositionKeys [ iKey ].vecPos = pPosKey->vecPos;

		if ( pPosKey->dwTime > pParentAnim->ulLength )
			pParentAnim->ulLength = pPosKey->dwTime;

		pPosKey++;
	}

	return true;
}

DARKSDK bool XFILE_SetupMatrixAnimation ( PBYTE* pDataPtr, sAnimation* pAnim, sAnimationSet* pParentAnim, DWORD dwNumKeys )
{
	// set up animation that uses matrix keys

	// check the pointers
	SAFE_MEMORY ( pDataPtr    );
	SAFE_MEMORY ( pAnim       );
	SAFE_MEMORY ( pParentAnim );

	// delete any current matrix keys
	SAFE_DELETE_ARRAY ( pAnim->pMatrixKeys );
	
	// matrix key
	sXFileMatrixKey* pMatKey = NULL;

	// allocate a new set of matrix keys
	pAnim->pMatrixKeys = new sMatrixKey [ dwNumKeys ];

	// check the memory was allocated
	SAFE_MEMORY ( pAnim->pMatrixKeys );

	// store number of keys and get a pointer to the matrix data
	pAnim->dwNumMatrixKeys = dwNumKeys;
	pMatKey                = ( sXFileMatrixKey* ) ( ( char* ) pDataPtr + ( sizeof ( DWORD ) * 2 ) );

	for ( int iKey = 0; iKey < ( int ) dwNumKeys; iKey++ )
	{
		pAnim->pMatrixKeys [ iKey ].dwTime    = pMatKey->dwTime;
		pAnim->pMatrixKeys [ iKey ].matMatrix = pMatKey->matMatrix;

		if ( pMatKey->dwTime > pParentAnim->ulLength )
			pParentAnim->ulLength = pMatKey->dwTime;

		pMatKey++;
	}

	// calculate the interpolation matrices
	if ( dwNumKeys > 1 )
	{
		for ( int iKey = 0; iKey < ( int ) dwNumKeys - 1; iKey++ )
		{
			DWORD dwTime;

			pAnim->pMatrixKeys [ iKey ].matInterpolation = pAnim->pMatrixKeys [ iKey + 1 ].matMatrix - pAnim->pMatrixKeys [ iKey ].matMatrix;
			dwTime                                       = pAnim->pMatrixKeys [ iKey + 1 ].dwTime    - pAnim->pMatrixKeys [ iKey ].dwTime;

			if ( !dwTime )
				dwTime = 1;

			pAnim->pMatrixKeys [ iKey ].matInterpolation /= ( float ) dwTime;
		}
	}

	return true;
}

DARKSDK bool XFILE_GetAnimationData ( IDirectXFileData* pDataObj, sAnimation* pAnim, sAnimationSet* pParentAnim )
{
	// get the animation data from the file and move it into the DBO format
	
	// check the parameters
	SAFE_MEMORY ( pDataObj    );
	SAFE_MEMORY ( pAnim       );
	SAFE_MEMORY ( pParentAnim );

	// local variables
	PBYTE* pDataPtr  = NULL;	// data pointer from file
	DWORD  dwSize    = 0;		// size of data
	DWORD  dwKeyType = 0;		// type of animation
	DWORD  dwNumKeys = 0;		// number of animation keys

	// load in this animation's key data
	if ( FAILED ( pDataObj->GetData ( NULL, &dwSize, ( PVOID* ) &pDataPtr ) ) )
		return false;

	// get the key type and number of keys in the animation
	dwKeyType = ( ( DWORD* ) pDataPtr ) [ 0 ];
	dwNumKeys = ( ( DWORD* ) pDataPtr ) [ 1 ];

	// check the type of animation and call the appropriate function
	switch ( dwKeyType )
	{
		case 0:
		{
			// rotation
			if ( !XFILE_SetupRotationAnimation ( pDataPtr, pAnim, pParentAnim, dwNumKeys ) )
				return false;
		}
		break;

		case 1:
		{
			// scaling
			if ( !XFILE_SetupScalingAnimation ( pDataPtr, pAnim, pParentAnim, dwNumKeys ) )
				return false;
		}
		break;

		case 2:
		{
			// position
			if ( !XFILE_SetupPositionAnimation ( pDataPtr, pAnim, pParentAnim, dwNumKeys ) )
				return false;
		}
		break;

		case 4:
		{
			// matrix keys
			if ( !XFILE_SetupMatrixAnimation ( pDataPtr, pAnim, pParentAnim, dwNumKeys ) )
				return false;
		}
		break;

		default:
			break;
	}

	return true;
}

DARKSDK bool ParseXFileData ( IDirectXFileData* pDataObj, sFrame* pParentFrame, char* szTexturePath, sAnimationSet* pParentAnim, sAnimation* pCurrentAnim, bool bAnim )
{
	// parse the X file data, this is a fairly complex function that runs through all
	// of the data in an X file

	// check some of the parameters, we don't need to check the anim ones as they
	// don't need to be valid at the moment
	SAFE_MEMORY ( pDataObj );
	SAFE_MEMORY ( pParentFrame );
	SAFE_MEMORY ( szTexturePath );
	
	IDirectXFileObject*			pSubObj     = NULL;
	IDirectXFileData*			pSubData    = NULL;
	IDirectXFileDataReference*	pDataRef    = NULL;
	const GUID*					guidType    = NULL;
	char*						szName      = NULL;
	DWORD						dwSize      = 0;
	sFrame*						pSubFrame   = NULL;
	sFrame*						pFrame      = NULL;
	sAnimationSet*				pSubAnimSet = NULL;
	sAnimation*					pSubAnim    = NULL;
	sAnimation*					pAnim       = NULL;
	sAnimationSet*				pAnimSet    = NULL;

	// get the template info for the X file object
	if ( !XFILE_GetTemplateInfo ( pDataObj, &guidType, &szName ) )
		return false;

	// set sub frame
	pSubFrame = pParentFrame;

	// set sub frame parent
	pSubAnimSet = pParentAnim;
	pSubAnim    = pCurrentAnim;

	// process the templates

	// section for non animation
	if ( !bAnim )
	{
		if ( *guidType == TID_D3DRMFrame )
		{
			// create a frame
			if ( !XFILE_CreateFrame ( &pFrame, pParentFrame, &pSubFrame, szName ) )
				return false;
		}
		
		if ( *guidType == TID_D3DRMFrameTransformMatrix )
		{
			// get the frame transformation matrix
			XFILE_CreateTransformationMatrix ( pDataObj, pParentFrame );
		}

		// mesh
		if ( *guidType == TID_D3DRMMesh )
		{
//			if getting to the data is needed, this may help...
//			DWORD dwSize = 0;
//			LPSTR pDataPtr = NULL;
//			pDataObj->GetData ( NULL, &dwSize, (void**)&pDataPtr );
//			DWORD* pRawData = (DWORD*)pDataPtr;
//			DWORD dwRawVerts = *(pRawData+0);

			// leefix - 100303 - if mesh exists in current frame, must create new sibling frame to hold new mesh
			if ( pParentFrame->pMesh )
			{
				// create a new frame for extra meshes
				if ( !XFILE_CreateFrame ( &pFrame, pParentFrame, &pSubFrame, szName ) )
					return false;

				// create new mesh in new sibling frame
				if ( !XFILE_GetMeshData ( pDataObj, pFrame ) )
					return false;
			}
			else
			{
                // apply the mesh name to the frame if the frame hasn't yet got a name
                if (pParentFrame->szName[0] == 0 && szName[0] != 0)
                    strcpy ( pParentFrame->szName, szName );

                // create new mesh in frame
				if ( !XFILE_GetMeshData ( pDataObj, pParentFrame ) )
					return false;
			}
		}

		// skip animation sets and animations
		if ( *guidType == TID_D3DRMAnimationSet || *guidType == TID_D3DRMAnimation || *guidType == TID_D3DRMAnimationKey )
		{
			// take no further action if animation token
			SAFE_DELETE_ARRAY ( szName );
			return true;
		}
	}

	if ( bAnim )
	{
		// process an animation set
		if ( *guidType == TID_D3DRMAnimationSet )
		{
			if ( !XFILE_CreateAnimationSet ( &pAnimSet, &pSubAnimSet, szName ) )
				return false;
		}

		// create an animation
		if ( *guidType == TID_D3DRMAnimation && pParentAnim != NULL ) 
		{
			if ( !XFILE_CreateAnimation ( &pAnim, &pSubAnim, pParentAnim, szName ) )
				return false;
		}

		// process an animation key
		if ( *guidType == TID_D3DRMAnimationKey && pCurrentAnim != NULL )
		{
			// get the animation data from the key
			if ( !XFILE_GetAnimationData ( pDataObj, pCurrentAnim, pParentAnim ) )
				return false;
		}

		// process an animation options
		if ( *guidType == TID_D3DRMAnimationOptions && pCurrentAnim != NULL )
		{
			// get the animation options - open/closed and spline/linear
			DWORD dwSize = 0;
			PBYTE* pDataPtr  = NULL;	// data pointer from file
			if ( FAILED ( pDataObj->GetData ( NULL, &dwSize, ( PVOID* ) &pDataPtr ) ) )
				return false;

			// get animation options data
			DWORD dwOpenClosed			= ( ( DWORD* ) pDataPtr ) [ 0 ];
			DWORD dwPositionQuality		= ( ( DWORD* ) pDataPtr ) [ 1 ];

			// switch animation flags based on options
			if ( dwPositionQuality==1 )
				pCurrentAnim->bLinear = TRUE;
			else
				pCurrentAnim->bLinear = FALSE;
		}

		// process a frame reference
		if ( *guidType == TID_D3DRMFrame && pCurrentAnim != NULL )
		{
			strcpy ( pCurrentAnim->szName, szName );
			
			// take no further action if frame (ref) token
			SAFE_DELETE_ARRAY ( szName );
			return true;
		}
	}

	// MIKE - 311003
	SAFE_DELETE_ARRAY ( szName );

	// scan for embedded templates
	while ( SUCCEEDED ( pDataObj->GetNextObject ( &pSubObj ) ) )
	{
		// process embedded references
		if ( SUCCEEDED ( pSubObj->QueryInterface ( DBIID_IDirectXFileDataReference, ( void** ) &pDataRef ) ) )
		{
			if ( SUCCEEDED ( pDataRef->Resolve ( &pSubData ) ) )
			{
				if ( !ParseXFileData ( pSubData, pSubFrame, szTexturePath, pSubAnimSet, pSubAnim, bAnim ) )
					return false;

				SAFE_RELEASE ( pSubData );
			}

			SAFE_RELEASE ( pDataRef );
		}

		// process non - referenced embedded templates
		if ( SUCCEEDED ( pSubObj->QueryInterface ( DBIID_IDirectXFileData, ( void** ) &pSubData ) ) )
		{
			if ( !ParseXFileData ( pSubData, pSubFrame, szTexturePath, pSubAnimSet, pSubAnim, bAnim ) )
				return false;

			SAFE_RELEASE ( pSubData );
		}

		SAFE_RELEASE ( pSubObj );
	}

	return true;
}


/* This seems redundant, if so, delete from exisrtance

  void SetupTextures ( sMesh* pMesh, GGBASEMATERIAL* pMaterials, DWORD dwCount, DWORD* pAttribute, DWORD dwIndexCount )
{
	// at least one texture must be created
	if ( dwCount < 1 ) dwCount=1;

	// Store the attribute data for later processing (if any)
	if ( pAttribute )
	{
		DWORD dwNumberOfFaces = dwIndexCount / 3;
		pMesh->pAttributeWorkData = new DWORD [ dwNumberOfFaces ];
		memcpy ( pMesh->pAttributeWorkData, pAttribute, sizeof(DWORD) * dwNumberOfFaces );
	}

	// get properties
	pMesh->dwTextureCount = dwCount;							// save the number of textures
	pMesh->pTextures      = new sTexture [ dwCount ];			// allocate texture memory

	// if have a material 
	if ( pMaterials )
	{
		// copy entire material buffer to temporary bank (in case mesh needs splitting)
		pMesh->pMaterialBank  = new GGBASEMATERIAL [ dwCount ];
		memcpy ( pMesh->pMaterialBank, pMaterials, sizeof ( GGBASEMATERIAL ) * dwCount );

		// copy single material (normally the default if no splitting)
		pMesh->bUsesMaterial = true;
		memcpy ( &pMesh->mMaterial, &pMaterials [ 0 ].MatD3D, sizeof ( GGMATERIAL ) );

		// run through all textures, get materials and store
		for ( int iTexture = 0; iTexture < ( int ) dwCount; iTexture++ )
		{
			if ( pMaterials [ iTexture ].pTextureFilename )
			{
				DWORD dwSize = strlen ( pMaterials [ iTexture ].pTextureFilename );
				strcpy ( pMesh->pTextures [ iTexture ].pName, pMaterials [ iTexture ].pTextureFilename );
			}
			else
				strcpy ( pMesh->pTextures [ iTexture ].pName, "" );
		}
	}
	else
	{
		// default material
		GGMATERIAL material;
		material.Diffuse.r = 0.0f;
		material.Diffuse.g = 0.0f;
		material.Diffuse.b = 0.0f;
		material.Diffuse.a = 0.0f;
		material.Ambient	= material.Diffuse;

		// Store as base material
		pMesh->mMaterial = material;

		// create a single material bank item
		pMesh->bUsesMaterial = true;
		pMesh->pMaterialBank = new GGBASEMATERIAL [ 1 ];
		pMesh->pMaterialBank[0].MatD3D = material;
		pMesh->pMaterialBank[0].pTextureFilename = NULL;
	}
}
*/





















































































//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

// MIKE - 081003 - we need to keep this code because some of the DBO files refer to it without
//               - keeping the code in the DBO files

DARKSDK float CONVXwrapangleoffset(float da)
{
	int breakout=100;
	while(da<0.0f || da>=360.0f)
	{
		if(da<0.0f) da=da+360.0f;
		if(da>=360.0f) da=da-360.0f;
		breakout--;
		if(breakout==0) break;
	}
	if(breakout==0) da=0.0f;
	return da;
}

DARKSDK void ConvXGetAngleFromPoint(float x1, float y1, float z1, float x2, float y2, float z2, float* ax, float* ay, float* az)
{
	GGVECTOR3 Vector;
	Vector.x = x2-x1;
	Vector.y = y2-y1;
	Vector.z = z2-z1;

	// Find Y and then X axis rotation
	double yangle=atan2(Vector.x, Vector.z);
	if(yangle<0.0) yangle+=GGToRadian(360.0);
	if(yangle>=GGToRadian(360.0)) yangle-=GGToRadian(360.0);

	GGMATRIX yrotate;
	GGMatrixRotationY ( &yrotate, (float)-yangle );
	GGVec3TransformCoord ( &Vector, &Vector, &yrotate );

	double xangle=-atan2(Vector.y, Vector.z);
	if(xangle<0.0) xangle+=GGToRadian(360.0);
	if(xangle>=GGToRadian(360.0)) xangle-=GGToRadian(360.0);

	*ax = CONVXwrapangleoffset(GGToDegree((float)xangle));
	*ay = CONVXwrapangleoffset(GGToDegree((float)yangle));
	*az = 0.0f;
}

DARKSDK void ConvXAnglesFromMatrix ( GGMATRIX* pmatMatrix, GGVECTOR3* pVecAngles )
{
	// Calculate angle vector based on matrix
	GGVECTOR3 pDirVec = GGVECTOR3 ( 0.0f, 0.0f, 1.0f );
	GGVec3TransformCoord ( &pDirVec, &pDirVec, pmatMatrix );
	ConvXGetAngleFromPoint ( 0.0f, 0.0f, 0.0f, pDirVec.x, pDirVec.y, pDirVec.z, &(pVecAngles->x), &(pVecAngles->y), &(pVecAngles->z));
}

//#endif
