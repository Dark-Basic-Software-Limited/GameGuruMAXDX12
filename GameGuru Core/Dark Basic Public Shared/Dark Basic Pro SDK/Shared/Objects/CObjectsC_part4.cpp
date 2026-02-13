//
// LIMB Commands
//

DARKSDK_DLL void PerformCheckListForLimbs ( int iID )
{
	// check the object limb exists
	if ( !ConfirmObject ( iID ) )
		return;

	// Generate Checklist
	DWORD dwMaxStringSizeInEnum=0;
	bool bCreateChecklistNow=false;
	g_pGlob->checklisthasvalues=true;
	g_pGlob->checklisthasstrings=true;

	for(int pass=0; pass<2; pass++)
	{
		if(pass==1)
		{
			// Ensure checklist is large enough
			bCreateChecklistNow=true;
			for(int c=0; c<g_pGlob->checklistqty; c++)
			{
				GlobExpandChecklist(c, dwMaxStringSizeInEnum);
			}
		}

		// Run through total list
		g_pGlob->checklistqty=0;
		DWORD dwLimbMax = g_ObjectList [ iID ]->iFrameCount;
		for(DWORD iLimbID=0; iLimbID<dwLimbMax; iLimbID++)
		{
			// Get limb name
			LPSTR pName = g_ObjectList [ iID ]->ppFrameList [ iLimbID ]->szName;

			// Add to checklist
			DWORD dwSize=0;
			if(pName) dwSize=strlen(pName);
			if(dwSize>dwMaxStringSizeInEnum) dwMaxStringSizeInEnum=dwSize;
			if(bCreateChecklistNow)
			{
				// New checklist item
				g_pGlob->checklist[g_pGlob->checklistqty].valuea=iLimbID;
				if(pName==NULL)
					strcpy(g_pGlob->checklist[g_pGlob->checklistqty].string, "");
				else
					strcpy(g_pGlob->checklist[g_pGlob->checklistqty].string, pName);

				// calculate parent and child id
				sFrame* pCurrent = g_ObjectList [ iID ]->ppFrameList [ iLimbID ];
				sFrame* pParent = pCurrent->pParent;
				sFrame* pSibling = pCurrent->pSibling;
				sFrame* pChild = pCurrent->pChild;
				int childid=-1, siblingid=-1, parentid=-1;
				if ( pParent ) parentid = pParent->iID;
				if ( pSibling ) siblingid = pSibling->iID;
				if ( pChild ) childid = pChild->iID;

				// record id of frame index
				g_pGlob->checklist[g_pGlob->checklistqty].valuea = iLimbID;

				// record id of parent
				g_pGlob->checklist[g_pGlob->checklistqty].valueb = parentid;

				// record if of sibling
				g_pGlob->checklist[g_pGlob->checklistqty].valuec = siblingid;

				// record if of child
				g_pGlob->checklist[g_pGlob->checklistqty].valued = childid;
			}
			g_pGlob->checklistqty++;
		}
	}
 
	// Determine if checklist has any contents
	if(g_pGlob->checklistqty>0)
		g_pGlob->checklistexists=true;
	else
		g_pGlob->checklistexists=false;
}

DARKSDK_DLL void PerformCheckListForOnscreenObjects ( int iMode )
{
	// 301007 - new command
	bool bCreateChecklistNow=false;
	g_pGlob->checklisthasvalues=true;
	g_pGlob->checklisthasstrings=false;
	for(int pass=0; pass<2; pass++)
	{
		// Ensure checklist is large enough
		if(pass==1)
		{
			bCreateChecklistNow=true;
			for(int c=0; c<g_pGlob->checklistqty; c++)
				GlobExpandChecklist(c, 256);
		}

		// Run through total list
		g_pGlob->checklistqty=0;
		int iVisibleObjectCount = m_ObjectManager.GetVisibleObjectCount();
		sObject** ppSortedObjectVisibleList = m_ObjectManager.GetSortedObjectVisibleList();
		for ( int iObject = 0; iObject < iVisibleObjectCount; iObject++ )
		{
			// the object to draw
			sObject* pObject = ppSortedObjectVisibleList [ iObject ];
			if ( pObject==NULL )
				continue;

			// Add to checklist
			if(bCreateChecklistNow)
			{
				// record id of frame index
				g_pGlob->checklist[g_pGlob->checklistqty].valuea = (int)pObject->dwObjectNumber;

				// reserved
				g_pGlob->checklist[g_pGlob->checklistqty].valueb = 0;
				g_pGlob->checklist[g_pGlob->checklistqty].valuec = 0;
				g_pGlob->checklist[g_pGlob->checklistqty].valued = 0;
			}
			g_pGlob->checklistqty++;
		}
	}
 
	// Determine if checklist has any contents
	if(g_pGlob->checklistqty>0)
		g_pGlob->checklistexists=true;
	else
		g_pGlob->checklistexists=false;
}

DARKSDK_DLL void HideLimb ( int iID, int iLimbID )
{
	// check the object limb exists
	if ( !ConfirmObjectAndLimbInstance ( iID, iLimbID ) )
		return;

	// actual object or instance of object
	sObject* pObject = g_ObjectList [ iID ];
	if ( pObject->pInstanceMeshVisible )
	{
		// record hide state in instance-mesh-visibility-array
		sObject* pActualObject = pObject->pInstanceOfObject;
		if ( iLimbID>=0 && iLimbID<pActualObject->iFrameCount)
			pObject->pInstanceMeshVisible [ iLimbID ] = false;
	}
	else
	{
		// ensure limb has mesh
		sMesh* pMesh = pObject->ppFrameList [ iLimbID ]->pMesh;
		if ( !pMesh )
			return;

		// apply to specific mesh
		Hide ( pMesh );

		#ifdef WICKEDENGINE
		WickedCall_SetLimbVisible(pObject->ppFrameList [ iLimbID ], false);
		#endif
	}
}

DARKSDK_DLL void ShowLimb ( int iID, int iLimbID )
{
	// check the object limb exists
	if ( !ConfirmObjectAndLimbInstance ( iID, iLimbID ) )
		return;

	// actual object or instance of object
	sObject* pObject = g_ObjectList [ iID ];
	if ( pObject->pInstanceMeshVisible )
	{
		// record hide state in instance-mesh-visibility-array
		sObject* pActualObject = pObject->pInstanceOfObject;
		if ( iLimbID>=0 && iLimbID<pActualObject->iFrameCount)
			pObject->pInstanceMeshVisible [ iLimbID ] = true;
	}
	else
	{
		// ensure limb has mesh
		sMesh* pMesh = pObject->ppFrameList [ iLimbID ]->pMesh;
		if ( !pMesh )
			return;

		// apply to specific mesh
		Show ( pMesh );

		#ifdef WICKEDENGINE
		WickedCall_SetLimbVisible(pObject->ppFrameList [ iLimbID ], true);
		#endif
	}
}

DARKSDK_DLL void TextureLimb ( int iID, int iLimbID, int iImageID )
{
	// check the object limb exists
	if ( !ConfirmObjectAndLimb ( iID, iLimbID ) )
		return;

	// ensure limb has mesh
	sMesh* pMesh = g_ObjectList [ iID ]->ppFrameList [ iLimbID ]->pMesh;
	if ( !pMesh )
		return;

	// apply to specific mesh
	SetBaseTexture ( pMesh, -1, iImageID );
}

DARKSDK_DLL void TextureLimbStage ( int iID, int iLimbID, int iStage, int iImageID )
{
	// check the object limb exists
	if ( !ConfirmObjectAndLimb ( iID, iLimbID ) )
		return;

	// ensure limb has mesh
	sMesh* pMesh = g_ObjectList [ iID ]->ppFrameList [ iLimbID ]->pMesh;
	if ( !pMesh )
		return;

	// apply to specific mesh
	SetBaseTextureStage ( pMesh, iStage, iImageID );
}

DARKSDK_DLL void ColorLimb ( int iID, int iLimbID, DWORD dwColor )
{
	// check the object limb exists
	if ( !ConfirmObjectAndLimb ( iID, iLimbID ) )
		return;

	// ensure limb has mesh
	sMesh* pMesh = g_ObjectList [ iID ]->ppFrameList [ iLimbID ]->pMesh;
	if ( !pMesh )
		return;

	// apply to specific mesh
	SetBaseColor ( pMesh, dwColor );
}

DARKSDK_DLL void ScrollLimbTexture ( int iID, int iLimbID, int iStage, float fU, float fV )
{
	// check the object limb exists
	if ( !ConfirmObjectAndLimb ( iID, iLimbID ) )
		return;

	// ensure limb has mesh
	sMesh* pMesh = g_ObjectList [ iID ]->ppFrameList [ iLimbID ]->pMesh;
	if ( !pMesh )
		return;

	// apply to specific mesh
	ScrollTexture ( pMesh, iStage, fU, fV );
}

DARKSDK_DLL void ScrollLimbTexture ( int iID, int iLimbID, float fU, float fV )
{
	// refers to core function above
	ScrollLimbTexture ( iID, iLimbID, 0, fU, fV );
}

DARKSDK_DLL void ScaleLimbTexture ( int iID, int iLimbID, int iStage, float fU, float fV )
{
	// check the object limb exists
	if ( !ConfirmObjectAndLimb ( iID, iLimbID ) )
		return;

	// ensure limb has mesh
	sMesh* pMesh = g_ObjectList [ iID ]->ppFrameList [ iLimbID ]->pMesh;
	if ( !pMesh )
		return;

	// apply to specific mesh
	ScaleTexture ( pMesh, iStage, fU, fV );
}

DARKSDK_DLL void ScaleLimbTexture ( int iID, int iLimbID, float fU, float fV )
{
	// refers to core function above
	ScaleLimbTexture ( iID, iLimbID, 0, fU, fV );
}

DARKSDK_DLL void SetLimbSmoothing ( int iID, int iLimbID, float fPercentage )
{
	// check the object limb exists
	if ( !ConfirmObjectAndLimb ( iID, iLimbID ) )
		return;

	// limit percentage range
	if ( fPercentage<0.0f ) fPercentage=0.0f;
	if ( fPercentage>100.0f ) fPercentage=100.0f;

	// ensure limb has mesh
	sMesh* pMesh = g_ObjectList [ iID ]->ppFrameList [ iLimbID ]->pMesh;
	if ( !pMesh )
		return;

	// apply to specific mesh
	SmoothNormals ( pMesh, fPercentage/100.0f );
}

DARKSDK_DLL void SetLimbNormals ( int iID, int iLimbID )
{
	// check the object limb exists
	if ( !ConfirmObjectAndLimb ( iID, iLimbID ) )
		return;

	// ensure limb has mesh
	sMesh* pMesh = g_ObjectList [ iID ]->ppFrameList [ iLimbID ]->pMesh;
	if ( !pMesh )
		return;

	// apply to specific mesh
	GenerateNormals ( pMesh, 0 );
}

DARKSDK_DLL void OffsetLimb ( int iID, int iLimbID, float fX, float fY, float fZ, int iBoundsFlag )
{
	//LB: this messes up grass!!!
	//#ifdef WICKEDENGINE
	//// usurping use of Limb Zero (matUserMatrix) to apply adjustment for imported objects (no need for ROTX, OFFSETX, etc)
	//#else
	// check the object limb exists
	if ( !ConfirmObjectAndLimb ( iID, iLimbID ) )
		return;

	// apply to specific frame
	Offset ( g_ObjectList [ iID ]->ppFrameList [ iLimbID ], fX, fY, fZ );
	g_ObjectList [ iID ]->bAnimUpdateOnce = true;

	// leefix - 230604 - u54 - copy user matrix to combined matrix
	g_ObjectList [ iID ]->ppFrameList [ iLimbID ]->matCombined = g_ObjectList [ iID ]->ppFrameList [ iLimbID ]->matUserMatrix;

	// u55 - 080704 - under flag for best of both worlds
	if ( iBoundsFlag==1 ) CalculateAllBounds ( g_ObjectList [ iID ], false );
	//#endif
}

DARKSDK_DLL void OffsetLimb ( int iID, int iLimbID, float fX, float fY, float fZ )
{
	OffsetLimb ( iID, iLimbID, fX, fY, fZ, 0 );
}

DARKSDK_DLL void RotateLimb ( int iID, int iLimbID, float fX, float fY, float fZ, int iBoundsFlag )
{
	// check the object limb exists
	if ( !ConfirmObjectAndLimb ( iID, iLimbID ) )
		return;

	// apply to specific frame
	Rotate ( g_ObjectList [ iID ]->ppFrameList [ iLimbID ], fX, fY, fZ );

	// leefix - 230604 - u54 - copy user matrix to combined matrix
    if ( g_ObjectList [ iID ]->position.bApplyPivot==false )
	{
		// 270614 - used applypivot to prevent matCombined getting wiped out during ragdoll creation
		g_ObjectList [ iID ]->ppFrameList [ iLimbID ]->matCombined = g_ObjectList [ iID ]->ppFrameList [ iLimbID ]->matUserMatrix;
	}
	g_ObjectList [ iID ]->bAnimUpdateOnce = true;

	// u55 - 080704 - under flag for best of both worlds
	if ( iBoundsFlag==1 ) CalculateAllBounds ( g_ObjectList [ iID ], false );
}

DARKSDK_DLL void RotateLimb ( int iID, int iLimbID, float fX, float fY, float fZ )
{
	RotateLimb ( iID, iLimbID, fX, fY, fZ, 0 );
}

DARKSDK_DLL void ScaleLimb ( int iID, int iLimbID, float fX, float fY, float fZ, int iBoundsFlag )
{
	// check the object limb exists
	if ( !ConfirmObjectAndLimb ( iID, iLimbID ) )
		return;

	// scale down
	fX /= 100.0f;
	fY /= 100.0f;
	fZ /= 100.0f;

	// apply to specific frame
	Scale ( g_ObjectList [ iID ]->ppFrameList [ iLimbID ], fX, fY, fZ );

	// leefix - 230604 - u54 - copy user matrix to combined matrix
	g_ObjectList [ iID ]->ppFrameList [ iLimbID ]->matCombined = g_ObjectList [ iID ]->ppFrameList [ iLimbID ]->matUserMatrix;
	g_ObjectList [ iID ]->bAnimUpdateOnce = true;

	// u55 - 080704 - under flag for best of both worlds
	if ( iBoundsFlag==1 ) CalculateAllBounds ( g_ObjectList [ iID ], false );
}

DARKSDK_DLL void ScaleLimb ( int iID, int iLimbID, float fX, float fY, float fZ )
{
	ScaleLimb ( iID, iLimbID, fX, fY, fZ, 0 );
}

DARKSDK_DLL void AddLimb ( int iID, int iLimbID, int iMeshID )
{
	// check the object exists
	if ( !ConfirmObject ( iID ) )
		return;

	// check the mesh exists
	if ( !ConfirmMesh ( iMeshID ) )
		return;

	// get object ptr
	sObject* pObject = g_ObjectList [ iID ];

	// next available frame is
	int iAvailableFrame = pObject->iFrameCount;

	// if frame specified, must be correct
	if ( iLimbID > -1 )
	{
		if ( iLimbID!=iAvailableFrame )
		{
			#ifdef WICKEDENGINE
			extern bool g_bDisplayWarnings;
			// must be new consecutive limb 
			if (!g_bDisplayWarnings)
			{
				return;
			}
			#endif
			RunTimeError ( RUNTIMEERROR_LIMBMUSTCHAININSEQUENCE );
		}
	}

	// and the mesh to create frame with
	sMesh* pNewMesh = new sMesh;
	if ( pNewMesh==NULL )
		return;

	// no transform of new limb
	GGMATRIX matWorld;
	GGMatrixIdentity ( &matWorld );

	// make a copy of the mesh
	MakeMeshFromOtherMesh ( true, pNewMesh, g_RawMeshList [ iMeshID ], &matWorld );

	// add new frame to end of 
	if ( !AddNewFrame ( pObject, pNewMesh, "new limb" ) )
	{
		// could not make limb
		RunTimeError ( RUNTIMEERROR_B3DMESHTOOLARGE );
		SAFE_DELETE ( pNewMesh );
	}

	// recreate all mesh and frame lists
	CreateFrameAndMeshList ( pObject );

	// ensure bounds are recalculated
	pObject->bUpdateOverallBounds=true;

	// update mesh(es) of object
	m_ObjectManager.RefreshObjectInBuffer ( pObject );
}

DARKSDK_DLL void RemoveLimb ( int iID, int iLimbID )
{
	// check the object limb exists
	if ( !ConfirmObjectAndLimb ( iID, iLimbID ) )
		return;

	// cannot remove root frame
	if ( iLimbID==0 )
	{
		// could not remove limb zero
		RunTimeError ( RUNTIMEERROR_LIMBNUMBERILLEGAL );
		return;
	}

	// get object ptr
	sObject* pObject = g_ObjectList [ iID ];

	// free mesh resources
	if ( pObject->ppFrameList [ iLimbID ] )
	{
		if ( pObject->ppFrameList [ iLimbID ]->pMesh )
		{
			// frees buffers used to render mesh to backbuffer
			m_ObjectManager.RemoveBuffersUsedByObjectMesh ( pObject->ppFrameList [ iLimbID ]->pMesh );
		}
	}

	// remove frame from object
	if ( !RemoveFrame ( pObject, pObject->ppFrameList [ iLimbID ] ) )
	{
		// could not remove limb
		RunTimeError ( RUNTIMEERROR_LIMBNOTEXIST );
		return;
	}

	// recreate all meshand frame lists
	CreateFrameAndMeshList ( pObject );

	// ensure bounds are recalculated
	pObject->bUpdateOverallBounds=true;

	// update mesh(es) of object
	m_ObjectManager.RefreshObjectInBuffer ( pObject );
}

DARKSDK_DLL void LinkLimbEx ( int iID, int iParentID, int iLimbID, bool bDoNotReconstructMeshFrameList )
{
	// check the object limb exists
	if ( !ConfirmObjectAndLimb ( iID, iLimbID ) )
		return;

	// check the object limb exists
	if ( !ConfirmObjectAndLimb ( iID, iParentID ) )
		return;

	// cannot move root frame
	if ( iLimbID==0 )
	{
		// could not remove limb zero
		RunTimeError ( RUNTIMEERROR_LIMBNUMBERILLEGAL );
		return;
	}

	// cannot to ones self
	if ( iLimbID==iParentID )
	{
		// could not remove limb zero
		RunTimeError ( RUNTIMEERROR_LIMBNUMBERILLEGAL );
		return;
	}

	// get object ptr
	sObject* pObject = g_ObjectList [ iID ];

	// link frame from object
	if ( !LinkFrame ( pObject, pObject->ppFrameList [ iLimbID ], pObject->ppFrameList [ iParentID ] ) )
	{
		// could not link limb
		RunTimeError ( RUNTIMEERROR_LIMBNOTEXIST );
		return;
	}

	// recreate all meshand frame lists
	if (bDoNotReconstructMeshFrameList == false)
	{
		CreateFrameAndMeshList(pObject);
		pObject->bUpdateOverallBounds = true;
	}
}

DARKSDK_DLL void LinkLimb(int iID, int iParentID, int iLimbID)
{
	LinkLimbEx(iID, iParentID, iLimbID, false);
}

DARKSDK_DLL void ChangeLimbName	( int iID, int iLimbID, LPSTR pNewName )
{
	// check the object limb exists
	if ( !ConfirmObjectAndLimb ( iID, iLimbID ) )
		return;

	// cannot move root frame
	if ( iLimbID==0 )
	{
		// could not remove limb zero
		RunTimeError ( RUNTIMEERROR_LIMBNUMBERILLEGAL );
		return;
	}

	// change limb name
	sObject* pObject = g_ObjectList [ iID ];
	if (pObject)
	{
		strcpy(pObject->ppFrameList[iLimbID]->szName, pNewName);
	}
}

//
// MESH Commands
//

DARKSDK_DLL void LoadMeshCore ( SDK_LPSTR szFilename, int iMeshID )
{
	/*
	// ensure the mesh is okay to use
	ConfirmNewMesh ( iMeshID );

	// check memory allocation
	ID_MESH_ALLOCATION ( iMeshID );

	// load the mesh from an x file
	if ( !LoadRawMesh ( (LPSTR)szFilename, &g_RawMeshList [ iMeshID ] ) )
	{
		// pass mesh filename
		char pExtraErr[512];
		wsprintf ( pExtraErr, "LOAD MESH '%s',%d", szFilename, iMeshID );
		RunTimeError ( RUNTIMEERROR_B3DMESHNOTEXIST, pExtraErr );
		return;
	}
	*/
}

DARKSDK_DLL void DeleteMesh ( int iMeshID )
{
	// check the mesh exists
	if ( !ConfirmMesh ( iMeshID ) )
		return;

	// delete the mesh
	if ( !DeleteRawMesh ( g_RawMeshList [ iMeshID ] ) )
		return;

	// free the mesh from the list
	g_RawMeshList [ iMeshID ] = NULL;
}

/*
DARKSDK_DLL void LoadMesh ( LPSTR szFilename, int iMeshID )
{
	// Uses actual or virtual file..
	char VirtualFilename[_MAX_PATH];
	strcpy(VirtualFilename, szFilename);
	//g_pGlob->UpdateFilenameFromVirtualTable( VirtualFilename);

	CheckForWorkshopFile (VirtualFilename);

	// Decrypt and use media, re-encrypt
	g_pGlob->Decrypt( VirtualFilename );
	LoadMeshCore ( (SDK_LPSTR)VirtualFilename, iMeshID );
	g_pGlob->Encrypt( VirtualFilename );
}

DARKSDK_DLL void SaveMesh ( LPSTR pFilename, int iMeshID )
{
	// check the mesh exists
	if ( !ConfirmMesh ( iMeshID ) )
		return;

	// save mesh as an x file
	if ( !SaveRawMesh ( pFilename, g_RawMeshList [ iMeshID ] ) )
		return;
}
*/

DARKSDK_DLL void ChangeMesh ( int iObjectID, int iLimbID, int iMeshID )
{
	// check the limb exists
	if ( !ConfirmObjectAndLimb ( iObjectID, iLimbID ) )
		return;

	// specific limb/frame information
	sObject* pObject = g_ObjectList [ iObjectID ];
	sFrame* pFrameOfMeshToReplace = pObject->ppFrameList [ iLimbID ];
	sMesh* pOldMesh = pFrameOfMeshToReplace->pMesh;

	// 310817 - new mesh eater mode
	if ( iMeshID == 0 )
	{
		// keep existing mesh, just reset to zero
		for ( int iVertIndex = 0; iVertIndex < pOldMesh->dwVertexCount; iVertIndex++ )
		{
			*((float*)pOldMesh->pVertexData+(iVertIndex*(pOldMesh->dwFVFSize/4))+0) = 0.0f;
			*((float*)pOldMesh->pVertexData+(iVertIndex*(pOldMesh->dwFVFSize/4))+1) = 0.0f;
			*((float*)pOldMesh->pVertexData+(iVertIndex*(pOldMesh->dwFVFSize/4))+2) = 0.0f;
		}
		pOldMesh->bVBRefreshRequired = true;
	}
	else
	{
		// create new mesh
		sMesh* pNewMesh = new sMesh;
		if ( pNewMesh==NULL )
			return;

		// no transform of new limb
		GGMATRIX matWorld;
		GGMatrixIdentity ( &matWorld );

		// make a copy of the mesh
		MakeMeshFromOtherMesh ( true, pNewMesh, g_RawMeshList [ iMeshID ], &matWorld );

		// lee - 280306 - u6rc2 - if specify limb with no mesh, exit now
		if ( pOldMesh==NULL )
		{
			// no way to detect if mesh in limb, so silent fail this
			RunTimeWarning ( RUNTIMEERROR_B3DLIMBBUTNOMESH );
			SAFE_DELETE ( pNewMesh );
			return;
		}

		// create a texture-set for new mesh
		DWORD dwTextureCount = pOldMesh->dwTextureCount;
		pNewMesh->pTextures = new sTexture [ dwTextureCount ];
		pNewMesh->dwTextureCount = dwTextureCount;

		// lee - 200206 - u60 - extract all material and texture information from old mesh
		CloneInternalTextures ( pNewMesh, pOldMesh );
		CopyMeshSettings ( pNewMesh, pFrameOfMeshToReplace->pMesh );

		// 200603 - remove drawbuffer from mesh to be replaced
		m_ObjectManager.RemoveBuffersUsedByObjectMesh ( pFrameOfMeshToReplace->pMesh );

		// replace mesh
		if ( !ReplaceFrameMesh ( pFrameOfMeshToReplace, pNewMesh ) )
		{
			// failed to change mesh
			SAFE_DELETE ( pNewMesh );
		}

		// recreate all meshand frame lists
		CreateFrameAndMeshList ( pObject );

		// calculate bounding areas of object
		CalculateAllBounds ( pObject, false );
	}

	// update mesh(es) of object
	m_ObjectManager.RenewReplacedMeshes ( pObject );
}

DARKSDK_DLL void ConvertMeshToVertexData ( int iMeshID )
{
	// leeadd - 140405 - check the mesh not exists
	if ( !ConfirmMesh ( iMeshID ) )
		return;

	// do the conversion
	if ( g_RawMeshList ) ConvertLocalMeshToVertsOnly ( g_RawMeshList [ iMeshID ], false ); 
}

DARKSDK_DLL void MakeMeshFromObject ( int iMeshID, int iObjectID, int iIgnoreMode )
{
	// check the object exists
	if ( !ConfirmObjectInstance ( iObjectID ) )
		return;

	// check the mesh not exists
	if ( !ConfirmNewMesh ( iMeshID ) )
		return;

    // Get a pointer to the object (or if an instance, that instances object)
    sObject* pObject = g_ObjectList [ iObjectID ];
    if (pObject->pInstanceOfObject)
        pObject = pObject->pInstanceOfObject;

	// create new mesh
	sMesh* pNewMesh = NULL;
	if ( !CreateSingleMeshFromObject ( &pNewMesh, pObject, iIgnoreMode ) )
		return;

	// leeadd - 080405 - convert final mesh to vert only (for ODE trimesh support)
	// lee, interferes with fpsc-mapeditor blueprint, needs index data (and should keep it)
	// so move this to later in the trimesh making, as only ODE needs this!
	// maybe a new ObjectDLL command CONVERT OBJECT TO VERTEXDATA Object Number
	// ConvertLocalMeshToVertsOnly ( pNewMesh ); 
	// leeadd - 141008 - u70 - also make mesh from sphere and cylinder object need this - so put back in this case!
	if ( pNewMesh->iPrimitiveType!=GGPT_TRIANGLELIST )
		ConvertLocalMeshToVertsOnly ( pNewMesh, false ); 

	// check memory allocation
	ID_MESH_ALLOCATION ( iMeshID );

	// if full, delete contents
	if ( g_RawMeshList [ iMeshID ] )
	{
		SAFE_DELETE ( g_RawMeshList [ iMeshID ] );
	}

	// assign new mesh to rawmeshlist
	g_RawMeshList[iMeshID] = pNewMesh;
}

DARKSDK_DLL void MakeMeshFromLimb ( int iMeshID, int iObjectID, int iLimbNumber )
{
	// check the object exists
	if ( !ConfirmObjectAndLimbInstance ( iObjectID, iLimbNumber ) )
		return;

	// check the mesh not exists
	if ( !ConfirmNewMesh ( iMeshID ) )
		return;

    // Get a pointer to the object (or if an instance, that instances object)
    sObject* pObject = g_ObjectList [ iObjectID ];
    if (pObject->pInstanceOfObject)
        pObject = pObject->pInstanceOfObject;

	// early out if no mesh associated with the limb specified
	if ( pObject->ppFrameList )
		if ( pObject->ppFrameList[iLimbNumber] )
			if ( pObject->ppFrameList[iLimbNumber]->pMesh==NULL )
				return;

	// create new mesh
	sMesh* pNewMesh = NULL;
	if ( !CreateSingleMeshFromLimb ( &pNewMesh, pObject, iLimbNumber, 0 ) )
		return;

	// leeadd - 080405 - convert final mesh to vert only (for ODE trimesh support)
	// lee, interferes with fpsc-mapeditor blueprint, needs index data (and should keep it)
	// so move this to later in the trimesh making, as only ODE needs this!
	// maybe a new ObjectDLL command CONVERT OBJECT TO VERTEXDATA Object Number
	// ConvertLocalMeshToVertsOnly ( pNewMesh ); 
	// leeadd - 141008 - u70 - also make mesh from sphere and cylinder object need this - so put back in this case!
	if ( pNewMesh->iPrimitiveType!=GGPT_TRIANGLELIST )
		ConvertLocalMeshToVertsOnly ( pNewMesh, false ); 

	// check memory allocation
	ID_MESH_ALLOCATION ( iMeshID );

	// if full, delete contents
	if ( g_RawMeshList [ iMeshID ] )
	{
		SAFE_DELETE ( g_RawMeshList [ iMeshID ] );
	}

	// assign new mesh to rawmeshlist
	g_RawMeshList[iMeshID] = pNewMesh;
}

DARKSDK_DLL void MakeMeshFromObject ( int iMeshID, int iObjectID )
{
	// ignore nothing
	MakeMeshFromObject ( iMeshID, iObjectID, 0 );
}

DARKSDK_DLL void StealMeshesFromObject ( int iMasterObjectID, int iDonerObjectID )
{
	// this will transfer the meshes from one object to another, and erase references to those meshes
	// so the doner object can be safely deleted (ideal for merging two objects that share frames/anims into one)
	if ( !ConfirmObject ( iMasterObjectID ) && !ConfirmObject ( iDonerObjectID ) )
		return;

	// master object ptr
	sObject* pMasterObject = GetObjectData ( iMasterObjectID );

	// find last sibling to connect to
	sFrame* pFrameLinkage = pMasterObject->pFrame;
	while ( pFrameLinkage->pSibling ) pFrameLinkage = pFrameLinkage->pSibling;

	// go through all doner meshes
	sObject* pDonerObject = GetObjectData ( iDonerObjectID );
	for ( int iDonerFrameIndex = 0; iDonerFrameIndex < pDonerObject->iFrameCount; iDonerFrameIndex++ )
	{
		sFrame* pDonerFrame = pDonerObject->ppFrameList[iDonerFrameIndex];
		if ( pDonerFrame )
		{
			sMesh* pDonerMesh = pDonerFrame->pMesh;
			if ( pDonerMesh )
			{
				// add a new frame to root as sybling
				sFrame* pNewMasterFrame = new sFrame;
				pFrameLinkage->pSibling = pNewMasterFrame;
				strcpy ( pNewMasterFrame->szName, pDonerFrame->szName );
				pNewMasterFrame->pMesh = pDonerMesh;

				// erase reference from parent doner object
				pDonerFrame->pMesh = NULL;

				// and from list
				for ( int iMesh = 0; iMesh < pDonerObject->iMeshCount; iMesh++ )
				{
					sMesh* pMesh = pDonerObject->ppMeshList[iMesh];
					if ( pMesh == pDonerMesh ) pDonerObject->ppMeshList[iMesh] = NULL;
				}
			}
		}
	}

	// create frames and mesh lists from frame hierarchy modified above
	CreateFrameAndMeshList ( pMasterObject );

	// go through new meshes and associate with the frames of the master object
	if ( pMasterObject->ppMeshList )
	{
		InitFramesToBones ( pMasterObject->ppMeshList, pMasterObject->iMeshCount );
		MapFramesToBones ( pMasterObject->ppMeshList, pMasterObject->pFrame, pMasterObject->iMeshCount );
	}
}

DARKSDK_DLL void ReduceMesh ( int iMeshID, int iBlockMode, int iNearMode, int iGX, int iGY, int iGZ )
{
	//	reduce mesh 1,blockmode,nearmode,gx,gy,gz

	// check the mesh exists
	if ( !ConfirmMesh ( iMeshID ) )
		return;

	// reduce mesh as instructed
	ReduceMeshPolygons ( g_RawMeshList [ iMeshID ], iBlockMode, iNearMode, iGX, iGY, iGZ );
}

// Mesh/Limb Manipulation Commands

DBPRO_GLOBAL sObject*	g_pCurrentVertexDataObject		= NULL;
DBPRO_GLOBAL sMesh*		g_pCurrentVertexDataMesh		= NULL;
DBPRO_GLOBAL int		g_iCurrentVertexDataUpdateMode	= 0;

DARKSDK_DLL void LockVertexDataForLimbCore ( int iID, int iLimbID, int iReplaceOrUpdate )
{
	// check the object limb exists
	if ( !ConfirmObjectAndLimb ( iID, iLimbID ) )
		return;

	// ensure limb has mesh
	sMesh* pMesh = g_ObjectList [ iID ]->ppFrameList [ iLimbID ]->pMesh;
	if ( !pMesh )
		return;

	// assign mesh of limb for editing
	g_pCurrentVertexDataMesh = pMesh;
	g_pCurrentVertexDataObject = g_ObjectList [ iID ];
	g_iCurrentVertexDataUpdateMode = iReplaceOrUpdate;
}

DARKSDK_DLL void LockVertexDataForLimb ( int iID, int iLimbID )
{
	// lock vertex in basic replace mode
	LockVertexDataForLimbCore ( iID, iLimbID, 1 );
}

DARKSDK_DLL void LockVertexDataForMesh ( int iMeshID )
{
	// check the mesh exists
	if ( !ConfirmMesh ( iMeshID ) )
		return;

	// assign mesh of rawmesh for editing
	g_pCurrentVertexDataMesh = g_RawMeshList [ iMeshID ];
	g_pCurrentVertexDataObject = NULL;
	g_iCurrentVertexDataUpdateMode = 0;
}

DARKSDK_DLL void UnlockVertexData ( void )
{
	// mike - 010903 - need to update bounds or object may not be visible
//	CalculateAllBounds ( g_pCurrentVertexDataObject, true ); //slow slow slow

	// MIKE - 040204 - temporary fix for physics
	// LEE - 190204 - allow manual control over '1update' or '0replace' (until find physics issue)
	if ( g_pCurrentVertexDataObject )
	{
		if ( g_iCurrentVertexDataUpdateMode==2 )
		{
			// leeadd - 010306 - u60 - update original copy if flagged
			SAFE_DELETE ( g_pCurrentVertexDataMesh->pOriginalVertexData );
			CollectOriginalVertexData ( g_pCurrentVertexDataMesh );

			// not quite as fast as mode 1, but good for saving final results
			g_pCurrentVertexDataMesh->bVBRefreshRequired = true;
#ifndef WICKEDENGINE
			g_vRefreshMeshList.push_back ( g_pCurrentVertexDataMesh );
#endif
		}
		else
		{
			if ( g_iCurrentVertexDataUpdateMode==1 )
			{
				// mesh VB update - same size, just adjusted data - fast
				g_pCurrentVertexDataMesh->bVBRefreshRequired = true;
#ifndef WICKEDENGINE
				g_vRefreshMeshList.push_back ( g_pCurrentVertexDataMesh );
#endif
			}
			else
			{
				// complete mesh replace - slow
				g_pCurrentVertexDataObject->bReplaceObjectFromBuffers = true;
				m_ObjectManager.g_bObjectReplacedUpdateBuffers = true;
			}
		}
	}

	// end edit session
	g_pCurrentVertexDataMesh = NULL;
	g_pCurrentVertexDataObject = NULL;
	g_iCurrentVertexDataUpdateMode = 0;
}

DARKSDK_DLL void SetVertexDataPosition ( int iVertex, float fX, float fY, float fZ )
{
	// write directly to mesh
	if ( !g_pCurrentVertexDataMesh ) return;
	if ( iVertex<0 || iVertex>=(int)g_pCurrentVertexDataMesh->dwVertexCount ) return;

	SetPositionData ( g_pCurrentVertexDataMesh, iVertex, fX, fY, fZ );
}

DARKSDK_DLL void SetVertexDataNormals ( int iVertex, float fNX, float fNY, float fNZ )
{
	// write directly to mesh
	if ( !g_pCurrentVertexDataMesh ) return;
	if ( iVertex<0 || iVertex>=(int)g_pCurrentVertexDataMesh->dwVertexCount ) return;
	SetNormalsData ( g_pCurrentVertexDataMesh, iVertex, fNX, fNY, fNZ );
}

DARKSDK_DLL void SetVertexDataDiffuse	( int iVertex, DWORD dwDiffuse )
{
	// write directly to mesh
	if ( !g_pCurrentVertexDataMesh ) return;
	if ( iVertex<0 || iVertex>=(int)g_pCurrentVertexDataMesh->dwVertexCount ) return;
	SetDiffuseData ( g_pCurrentVertexDataMesh, iVertex, dwDiffuse );
}

DARKSDK_DLL void SetIndexData ( int iIndex, int iValue )
{
	// write directly to mesh
	if ( !g_pCurrentVertexDataMesh ) return;
	if ( iIndex<0 || iIndex>=(int)g_pCurrentVertexDataMesh->dwIndexCount ) return;
	g_pCurrentVertexDataMesh->pIndices [ iIndex ] = iValue;
}

DARKSDK_DLL void SetVertexDataUV ( int iVertex, float fU, float fV )
{
	// write directly to mesh
	if ( !g_pCurrentVertexDataMesh ) return;
	if ( iVertex<0 || iVertex>=(int)g_pCurrentVertexDataMesh->dwVertexCount ) return;

	SetUVData ( g_pCurrentVertexDataMesh, iVertex, fU, fV );
}

DARKSDK_DLL void SetVertexDataUV ( int iVertex, int iIndex, float fU, float fV )
{
	// write directly to mesh
	if ( !g_pCurrentVertexDataMesh ) return;
	if ( iVertex<0 || iVertex>=(int)g_pCurrentVertexDataMesh->dwVertexCount ) return;

	// only stages 0 to 7
	if ( iIndex >= 8 )
		return;

	// convert mesh if not supporting the stage
	bool bOkay=false;
	for ( int iI=0; iI<=iIndex; iI++ )
	{
		if ( iI==0 && iIndex<=iI && g_pCurrentVertexDataMesh->dwFVF & GGFVF_TEX1 )  bOkay=true;
		if ( iI==1 && iIndex<=iI && g_pCurrentVertexDataMesh->dwFVF & GGFVF_TEX2 )  bOkay=true;
		if ( iI==2 && iIndex<=iI && g_pCurrentVertexDataMesh->dwFVF & GGFVF_TEX3 )  bOkay=true;
		#ifdef DX11
		#else
		if ( iI==3 && iIndex<=iI && g_pCurrentVertexDataMesh->dwFVF & D3DFVF_TEX4 )  bOkay=true;
		if ( iI==4 && iIndex<=iI && g_pCurrentVertexDataMesh->dwFVF & D3DFVF_TEX5 )  bOkay=true;
		if ( iI==5 && iIndex<=iI && g_pCurrentVertexDataMesh->dwFVF & D3DFVF_TEX6 )  bOkay=true;
		if ( iI==6 && iIndex<=iI && g_pCurrentVertexDataMesh->dwFVF & D3DFVF_TEX7 )  bOkay=true;
		if ( iI==7 && iIndex<=iI && g_pCurrentVertexDataMesh->dwFVF & D3DFVF_TEX8 )  bOkay=true;
		#endif
	}
	if ( bOkay==false )
	{
		// convert to correct format
		DWORD dwFVF = g_pCurrentVertexDataMesh->dwFVF;
		if ( iIndex==0 ) dwFVF = dwFVF | GGFVF_TEX1;
		if ( iIndex==1 ) dwFVF = dwFVF | GGFVF_TEX2;
		if ( iIndex==2 ) dwFVF = dwFVF | GGFVF_TEX3;
		#ifdef DX11
		#else
		if ( iIndex==3 ) dwFVF = dwFVF | D3DFVF_TEX4;
		if ( iIndex==4 ) dwFVF = dwFVF | D3DFVF_TEX5;
		if ( iIndex==5 ) dwFVF = dwFVF | D3DFVF_TEX6;
		if ( iIndex==6 ) dwFVF = dwFVF | D3DFVF_TEX7;
		if ( iIndex==7 ) dwFVF = dwFVF | D3DFVF_TEX8;
		#endif
		ConvertLocalMeshToFVF ( g_pCurrentVertexDataMesh, dwFVF );
	}

	// get the offset map for the FVF
	sOffsetMap offsetMap;
	GetFVFOffsetMap ( g_pCurrentVertexDataMesh, &offsetMap );

	// make sure we have an offset to write UVs
	if ( offsetMap.dwTU[iIndex]>0 )
	{
		// set single UV vertex component
		*( ( float* ) g_pCurrentVertexDataMesh->pVertexData + offsetMap.dwTU[iIndex] + ( offsetMap.dwSize * iVertex ) ) = fU;
		*( ( float* ) g_pCurrentVertexDataMesh->pVertexData + offsetMap.dwTV[iIndex] + ( offsetMap.dwSize * iVertex ) ) = fV;
	}

	// flag mesh for a VB update
	g_pCurrentVertexDataMesh->bVBRefreshRequired=true;
#ifndef WICKEDENGINE
	g_vRefreshMeshList.push_back ( g_pCurrentVertexDataMesh );
#endif
}

DARKSDK_DLL void SetVertexDataSize ( int iVertex, float fSize )
{
	// mike - 160903 - set size for point sprites

	if ( !g_pCurrentVertexDataMesh ) return;

	sOffsetMap offsetMap;
	GetFVFOffsetMap ( g_pCurrentVertexDataMesh, &offsetMap );

	// make sure we have data in the vertices
	if ( g_pCurrentVertexDataMesh->dwFVF & GGFVF_PSIZE )
	{
		if ( iVertex < (int)g_pCurrentVertexDataMesh->dwVertexCount )
			*( ( float* ) g_pCurrentVertexDataMesh->pVertexData + offsetMap.dwPointSize + ( offsetMap.dwSize * iVertex ) ) = fSize;
	}

	// flag mesh for a VB update
	g_pCurrentVertexDataMesh->bVBRefreshRequired=true;
#ifndef WICKEDENGINE
	g_vRefreshMeshList.push_back ( g_pCurrentVertexDataMesh );
#endif
}

DARKSDK_DLL void AddMeshToVertexData ( int iMeshID )
{
	// check the mesh exists
	if ( !ConfirmMesh ( iMeshID ) )
		return;

	// write directly to mesh
	if ( g_pCurrentVertexDataMesh )
		if ( !AddMeshToData ( g_pCurrentVertexDataMesh, g_RawMeshList [ iMeshID ] ) )
			return;

	// must renew mesh
	if ( g_pCurrentVertexDataObject )
		m_ObjectManager.RenewReplacedMeshes ( g_pCurrentVertexDataObject );
}

DARKSDK_DLL void DeleteMeshFromVertexData ( int iVertex1, int iVertex2, int iIndex1, int iIndex2 )
{
	// write directly to mesh
	if ( g_pCurrentVertexDataMesh )
		if ( !DeleteMeshFromData ( g_pCurrentVertexDataMesh, iVertex1, iVertex2, iIndex1, iIndex2 ) )
			return;

	// must renew mesh
	if ( g_pCurrentVertexDataObject )
		m_ObjectManager.RenewReplacedMeshes ( g_pCurrentVertexDataObject );
}

DARKSDK_DLL int ObjectBlocking	( int iID, float X1, float Y1, float Z1, float X2, float Y2, float Z2 )
{
	// default is not blocking
	bool bResult = false;

	// check the object exists
	if ( !ConfirmObject ( iID ) )
		return false;

	// check ALL meshes
	sObject* pObject = g_ObjectList [ iID ];
	for ( int iMesh = 0; iMesh < pObject->iMeshCount; iMesh++ )
		if ( CheckIfMeshBlocking ( pObject->ppMeshList [ iMesh ], X1, Y1, Z1, X2, Y2, Z2 )==true )
			bResult=true;

	// result
	return bResult;
}

DARKSDK_DLL void AddMemblockToObject ( int iMemblock, int iID )
{
	// check the object exists
	if ( !ConfirmObject ( iID ) )
		return;

	// object ptr
	sObject* pObject = g_ObjectList [ iID ];

	// check memblock DLL exists
	/*if ( !MemblockExist )
		return;*/

	// check memblock X exists
	if ( !MemblockExist ( iMemblock ) )
		return;

	// release any previous custom data
	SAFE_DELETE ( pObject->pCustomData );

	// create custom data from memblock
	DWORD dwMemBlockSize = GetMemblockSize ( iMemblock );
	pObject->dwCustomSize = dwMemBlockSize + 8;
	pObject->pCustomData = new char [ pObject->dwCustomSize ];
	LPSTR pMemData = (LPSTR)GetMemblockPtr ( iMemblock );
	BYTE* pBytePtr = (BYTE*)pObject->pCustomData;
	*((DWORD*)pBytePtr+0 ) = DBOBLOCK_OBJECT_CUSTOMDATA; // custom token
	memcpy ( pBytePtr+4, pMemData, dwMemBlockSize ); // memblock data
	*((DWORD*)(pBytePtr+4+dwMemBlockSize) ) = 0; // terminator
}

DARKSDK_DLL void GetMemblockFromObject ( int iMemblock, int iID )
{
	// check the object exists
	if ( !ConfirmObject ( iID ) )
		return;

	// object ptr
	sObject* pObject = g_ObjectList [ iID ];

	// ensure we have custom data
	if ( !pObject->pCustomData )
		return;

	// check call ptr to memblock DLL exists
	/*if ( !MemblockExist )
		return;*/

	// 250413 - must be real membloc, not instance stamp custom data
	if ( pObject->dwCustomSize>4000000000 )
		return;

	// get custom data and get the data from it
	DWORD dwMemblockToken = *((DWORD*)(LPSTR)pObject->pCustomData+0 );
	LPSTR pMemblockData = (LPSTR)pObject->pCustomData+4;
	DWORD dwMemBlockSize = pObject->dwCustomSize-8;

	// token must be correct
	if ( dwMemblockToken!=DBOBLOCK_OBJECT_CUSTOMDATA )
		return;

	// check memblock X exists
	if ( MemblockExist ( iMemblock ) )
	{
		// delete it as we are creating a new one
		DeleteMemblock ( iMemblock );
	}	

	// make memblock
	MakeMemblock ( iMemblock, dwMemBlockSize );

	// read data into memblock from custom data in object
	LPSTR pMemData = (LPSTR)GetMemblockPtr ( iMemblock );
	BYTE* pBytePtr = (BYTE*)pObject->pCustomData;
	memcpy ( pMemData, pBytePtr+4, dwMemBlockSize );
}

DARKSDK_DLL void DeleteMemblockFromObject ( int iID )
{
	// check the object exists
	if ( !ConfirmObject ( iID ) )
		return;

	// object ptr
	sObject* pObject = g_ObjectList [ iID ];

	// ensure we have custom data
	if ( !pObject->pCustomData )
		return;

	// clear custom data
	SAFE_DELETE ( pObject->pCustomData );
	pObject->dwCustomSize = 0;
}

void SetObjectStatisticsInteger ( int iID, int iIndex, int dwValue )
{
	// check the object exists
	if ( !ConfirmObject ( iID ) )
		return;

	// object ptr
	sObject* pObject = g_ObjectList [ iID ];
	if ( pObject->dwCustomSize==0 )
	{
		// create custom slot
		DWORD dwStatisticsDataSize = 8;
		pObject->dwCustomSize = dwStatisticsDataSize*-1;
		pObject->pCustomData = (LPVOID)new DWORD[dwStatisticsDataSize];
		for ( DWORD i=0; i<dwStatisticsDataSize; i++ )
			*(((DWORD*)pObject->pCustomData)+i) = 0;
	}
	if ( pObject->dwCustomSize>4000000000 )
	{
		*(((DWORD*)pObject->pCustomData)+iIndex) = dwValue;
	}
}

DARKSDK_DLL int ObjectStatisticsInteger ( int iID, int iIndex )
{
	// check the object exists
	if ( !ConfirmObject ( iID ) )
		return 0;

	// object ptr
	sObject* pObject = g_ObjectList [ iID ];

	// ensure we have custom data
	int iResult = 0;
	if ( pObject->dwCustomSize>4000000000 )
	{
		if ( pObject->pCustomData )
		{
			iResult = *(((int*)pObject->pCustomData)+iIndex);
		}
	}
	return iResult;
}


// Mesh/Limb Manipulation Expressions

DARKSDK_DLL int GetVertexDataVertexCount ( void )
{
	// return vertexdata value
	if ( !g_pCurrentVertexDataMesh ) return 0;
	return GetVertexCount ( g_pCurrentVertexDataMesh );
}

DARKSDK_DLL int GetVertexDataIndexCount ( void )
{
	// return vertexdata value
	if ( !g_pCurrentVertexDataMesh ) return 0;
	return GetIndexCount ( g_pCurrentVertexDataMesh );
}

DARKSDK_DLL float GetVertexDataPositionX ( int iVertex )
{
	// return vertexdata value
	if ( !g_pCurrentVertexDataMesh ) return 0;
	float fValue = GetDataPositionX ( g_pCurrentVertexDataMesh, iVertex );
	return fValue;
}

DARKSDK_DLL float GetVertexDataPositionY ( int iVertex )
{
	// return vertexdata value
	if ( !g_pCurrentVertexDataMesh ) return 0;
	float fValue = GetDataPositionY ( g_pCurrentVertexDataMesh, iVertex );
	return fValue;
}

DARKSDK_DLL float GetVertexDataPositionZ ( int iVertex )
{
	// return vertexdata value
	if ( !g_pCurrentVertexDataMesh ) return 0;
	float fValue = GetDataPositionZ ( g_pCurrentVertexDataMesh, iVertex );
	return fValue;
}

DARKSDK_DLL float GetVertexDataNormalsX	( int iVertex )
{
	// return vertexdata value
	if ( !g_pCurrentVertexDataMesh ) return 0;
	float fValue = GetDataNormalsX ( g_pCurrentVertexDataMesh, iVertex );
	return fValue;
}

DARKSDK_DLL float GetVertexDataNormalsY	( int iVertex )
{
	// return vertexdata value
	if ( !g_pCurrentVertexDataMesh ) return 0;
	float fValue = GetDataNormalsY ( g_pCurrentVertexDataMesh, iVertex );
	return fValue;
}

DARKSDK_DLL float GetVertexDataNormalsZ	( int iVertex )
{
	// return vertexdata value
	if ( !g_pCurrentVertexDataMesh ) return 0;
	float fValue = GetDataNormalsZ ( g_pCurrentVertexDataMesh, iVertex );
	return fValue;
}

DARKSDK_DLL DWORD GetVertexDataDiffuse ( int iVertex )
{
	// return vertexdata value
	if ( !g_pCurrentVertexDataMesh ) return 0;
	return GetDataDiffuse ( g_pCurrentVertexDataMesh, iVertex );
}

DARKSDK_DLL float GetVertexDataU ( int iVertex )
{
	// return vertexdata value
	if ( !g_pCurrentVertexDataMesh ) return 0;
	float fValue = GetDataU ( g_pCurrentVertexDataMesh, iVertex );
	return fValue;
}

DARKSDK_DLL float GetVertexDataV ( int iVertex )
{
	// return vertexdata value
	if ( !g_pCurrentVertexDataMesh ) return 0;
	float fValue = GetDataV ( g_pCurrentVertexDataMesh, iVertex );
	return fValue;
}

DARKSDK_DLL float GetVertexDataU ( int iVertex, int iIndex )
{
	if ( !g_pCurrentVertexDataMesh ) return 0;
	float fValue = GetDataU ( g_pCurrentVertexDataMesh, iVertex, iIndex );
	return fValue;
}

DARKSDK_DLL float GetVertexDataV ( int iVertex, int iIndex )
{
	if ( !g_pCurrentVertexDataMesh ) return 0;
	float fValue = GetDataV ( g_pCurrentVertexDataMesh, iVertex, iIndex );
	return fValue;
}

DARKSDK_DLL int GetIndexData ( int iIndex )
{
	if ( !g_pCurrentVertexDataMesh ) return 0;
	if ( iIndex<0 || iIndex>=(int)g_pCurrentVertexDataMesh->dwIndexCount ) return 0;
	return g_pCurrentVertexDataMesh->pIndices [ iIndex ];
}

DARKSDK_DLL DWORD GetVertexDataPtr ( void )
{
	return (DWORD)g_pCurrentVertexDataMesh;
}

// Misc Commands

DARKSDK_DLL void SetFastBoundsCalculation ( int iMode )
{
	if ( iMode==1 )
		g_bFastBoundsCalculation = true;
	else
		g_bFastBoundsCalculation = false;
}

DARKSDK_DLL void SetMipmapMode ( int iMode )
{
	// OBSOLETE
}

DARKSDK_DLL void FlushVideoMemory ( void )
{
	// OBSOLETE
}

DARKSDK_DLL void DisableTNL ( void )
{
	// OBSOLETE
}

DARKSDK_DLL void EnableTNL ( void )
{
	// OBSOLETE
}

DARKSDK_DLL void Convert3DStoX ( DWORD pFilename1, DWORD pFilename2 )
{
	// OBSOLETE
}

DARKSDK_DLL int PickScreenObjectEx ( int iX, int iY, int iObjectStart, int iObjectEnd, int iIgnoreCamera, int iIgnoreAllButLastFrame )
{
	// result hit
	int iObjectHit=0;
    bool bPickingLocked = false;

	// leefix - 010306 - u60 - camera X (actually checked in U6 and it is; dwCurrentSetCameraID )
	tagCameraData* m_Camera_Ptr = (tagCameraData*)GetCameraInternalData ( g_pGlob->dwCurrentSetCameraID );

    // u74b7 - Save the FOV as may need to reset it later
    float fCurrentFov = m_Camera_Ptr->fFOV;

	// calculate line from camera to diatant point through projected XY
	GGVECTOR3 vecFrom = m_Camera_Ptr->vecPosition;

	// Calculate inverse to take 2D into 3D
	GGMATRIX matInvertedView = m_Camera_Ptr->matView;
	GGMATRIX matInvertedProjection = m_Camera_Ptr->matProjection;
	GGMatrixInverse ( &matInvertedView, NULL, &matInvertedView );
	GGMatrixInverse ( &matInvertedProjection, NULL, &matInvertedProjection );

	// leeadd - 130306 - u60 - great god lee adds camera independence
	if ( iIgnoreCamera == 1 )
	{
        // u74b7 - In this mode, we are looking for locked objects only using the default FOV
        bPickingLocked = true;
        if (fCurrentFov != GGToDegree(3.14159265358979323846f/2.905f) )
        {
    		SetCameraFOV ( GGToDegree(3.14159265358979323846f/2.905f) );
        }

		// null camera (facing forward)
		vecFrom = GGVECTOR3(0,0,0);
		GGMatrixIdentity ( &matInvertedView );
		matInvertedProjection = m_Camera_Ptr->matProjection;
		GGMatrixInverse ( &matInvertedView, NULL, &matInvertedView );
		GGMatrixInverse ( &matInvertedProjection, NULL, &matInvertedProjection );
	}

	// Transform destination vector into screen
	float fHWidth = m_Camera_Ptr->viewPort3D.Width/2;
	float fHHeight = m_Camera_Ptr->viewPort3D.Height/2;
	iX = iX - m_Camera_Ptr->viewPort3D.X;
	iY = iY - m_Camera_Ptr->viewPort3D.Y;
	iX = iX - fHWidth; iY = fHHeight - iY;
	GGVECTOR3 vecTo = GGVECTOR3 ( (float)iX/fHWidth, (float)iY/fHHeight, 1.0f );
	GGVec3TransformCoord ( &vecTo, &vecTo, &matInvertedProjection );
	GGVec3TransformCoord ( &vecTo, &vecTo, &matInvertedView );
	vecTo = vecTo-vecFrom;
	GGVec3Normalize ( &vecTo, &vecTo );
	vecTo *= 15000.0f;
	vecTo = vecFrom + vecTo;

	// do intersect test against all objects in shortlist
	float fBestDistance = 999999.9f;
	for ( int iShortList = 0; iShortList < g_iObjectListRefCount; iShortList++ )
	{
		// get index from shortlist
		int iObjectID = g_ObjectListRef [ iShortList ];

		// only those within range
		if ( iObjectID>=iObjectStart && iObjectID<=iObjectEnd )
		{
			// actual object or instance of object
			sObject* pObject = g_ObjectList [ iObjectID ];

			// leeadd - 160504 - u6 - only pick a visible object
            // u74b7 - Allow locked if looking for locked, and vice-versa
			if ( !pObject->bVisible || pObject->bLockedObject != bPickingLocked )
				continue;

			// lee - 170117 - only pick an object with default collision ACTIVE
			if ( pObject->collision.bActive == false )
				continue;

			// see if we have a valid list
			float fThisDistance = IntersectObjectCore ( pObject, vecFrom.x, vecFrom.y, vecFrom.z, vecTo.x, vecTo.y, vecTo.z, iIgnoreAllButLastFrame );
			if ( fThisDistance>0.0f && fThisDistance < fBestDistance )
			{
				fBestDistance = fThisDistance;
				iObjectHit = iObjectID;
			}
		}
	}

	// generate useful data from pick
	if ( iObjectHit>0 )
	{
		// store normal vector with distance
		g_DBPROPickResult.iObjectID = iObjectHit;
		g_DBPROPickResult.fPickDistance = fBestDistance;
		g_DBPROPickResult.vecPickVector = vecTo - vecFrom;
		g_DBPROPickResult.vecFromVector = vecFrom;
		GGVec3Normalize ( &g_DBPROPickResult.vecPickVector, &g_DBPROPickResult.vecPickVector );
		g_DBPROPickResult.vecPickVector *= fBestDistance;
	}

    // Reset the FOV if necessary
    if (fCurrentFov != m_Camera_Ptr->fFOV)
    {
		SetCameraFOV ( fCurrentFov );
    }

	/// return result
	return iObjectHit;
}

DARKSDK_DLL int PickScreenObjectFromHeight(int iX, int iY, int iObjectStart, int iObjectEnd, float starty)
{
	// result hit
	int iObjectHit = 0;
	bool bPickingLocked = false;

	// leefix - 010306 - u60 - camera X (actually checked in U6 and it is; dwCurrentSetCameraID )
	tagCameraData* m_Camera_Ptr = (tagCameraData*)GetCameraInternalData(g_pGlob->dwCurrentSetCameraID);

	// u74b7 - Save the FOV as may need to reset it later
	float fCurrentFov = m_Camera_Ptr->fFOV;

	// calculate line from camera to diatant point through projected XY
	GGVECTOR3 vecFrom = m_Camera_Ptr->vecPosition;

	// Calculate inverse to take 2D into 3D
	GGMATRIX matInvertedView = m_Camera_Ptr->matView;
	GGMATRIX matInvertedProjection = m_Camera_Ptr->matProjection;
	GGMatrixInverse(&matInvertedView, NULL, &matInvertedView);
	GGMatrixInverse(&matInvertedProjection, NULL, &matInvertedProjection);

	// Transform destination vector into screen
	float fHWidth = m_Camera_Ptr->viewPort3D.Width / 2;
	float fHHeight = m_Camera_Ptr->viewPort3D.Height / 2;
	iX = iX - m_Camera_Ptr->viewPort3D.X;
	iY = iY - m_Camera_Ptr->viewPort3D.Y;
	iX = iX - fHWidth; iY = fHHeight - iY;
	GGVECTOR3 vecTo = GGVECTOR3((float)iX / fHWidth, (float)iY / fHHeight, 1.0f);
	GGVec3TransformCoord(&vecTo, &vecTo, &matInvertedProjection);
	GGVec3TransformCoord(&vecTo, &vecTo, &matInvertedView);
	vecTo = vecTo - vecFrom;
	GGVec3Normalize(&vecTo, &vecTo);
	GGVECTOR3 vecFromInc = vecTo;
	vecTo *= 15000.0f;
	vecTo = vecFrom + vecTo;

	//PE: We need to walk vecFrom until we are below starty.
	vecFromInc *= 5.0; //walk quicker.
	int maxwalk = 5000;
	while (vecFrom.y > starty && vecFrom.y > 0 && vecFrom.y > vecTo.y && maxwalk-- > 0) {
		if ((vecFrom.y + vecFromInc.y) > starty) //Stop right before we hit the max.
			break;
		vecFrom += vecFromInc;
	}

	// do intersect test against all objects in shortlist
	float fBestDistance = 999999.9f;
	for (int iShortList = 0; iShortList < g_iObjectListRefCount; iShortList++)
	{
		// get index from shortlist
		int iObjectID = g_ObjectListRef[iShortList];

		// only those within range
		if (iObjectID >= iObjectStart && iObjectID <= iObjectEnd)
		{
			// actual object or instance of object
			sObject* pObject = g_ObjectList[iObjectID];

			// leeadd - 160504 - u6 - only pick a visible object
			// u74b7 - Allow locked if looking for locked, and vice-versa
			if (!pObject->bVisible || pObject->bLockedObject != bPickingLocked)
				continue;

			// lee - 170117 - only pick an object with default collision ACTIVE
			if (pObject->collision.bActive == false)
				continue;

			// see if we have a valid list
			float fThisDistance = IntersectObjectCore(pObject, vecFrom.x, vecFrom.y, vecFrom.z, vecTo.x, vecTo.y, vecTo.z, 0);
			if (fThisDistance > 0.0f && fThisDistance < fBestDistance)
			{
				fBestDistance = fThisDistance;
				iObjectHit = iObjectID;
			}
		}
	}

	// generate useful data from pick
	if (iObjectHit > 0)
	{
		// store normal vector with distance
		g_DBPROPickResult.iObjectID = iObjectHit;
		g_DBPROPickResult.fPickDistance = fBestDistance;
		g_DBPROPickResult.vecPickVector = vecTo - vecFrom;
		g_DBPROPickResult.vecFromVector = vecFrom;
		GGVec3Normalize(&g_DBPROPickResult.vecPickVector, &g_DBPROPickResult.vecPickVector);
		g_DBPROPickResult.vecPickVector *= fBestDistance;
	}

	// Reset the FOV if necessary
	if (fCurrentFov != m_Camera_Ptr->fFOV)
	{
		SetCameraFOV(fCurrentFov);
	}

	/// return result
	return iObjectHit;
}


DARKSDK_DLL int PickScreenObject ( int iX, int iY, int iObjectStart, int iObjectEnd )
{
	// see abive - this is the default behaviour
	return PickScreenObjectEx ( iX, iY, iObjectStart, iObjectEnd, 0, 0 );
}

DARKSDK_DLL void PickScreen2D23D ( int iX, int iY, float fDistance )
{
	// camera zero only
	tagCameraData* m_Camera_Ptr = (tagCameraData*)GetCameraInternalData ( g_pGlob->dwCurrentSetCameraID );

	// calculate line from camera to diatant point through projected XY
	GGVECTOR3 vecFrom = m_Camera_Ptr->vecPosition;

	// Calculate inverse to take 2D into 3D
	GGMATRIX matInvertedView = m_Camera_Ptr->matView;
	GGMATRIX matInvertedProjection = m_Camera_Ptr->matProjection;
	GGMatrixInverse ( &matInvertedView, NULL, &matInvertedView );
	GGMatrixInverse ( &matInvertedProjection, NULL, &matInvertedProjection );

	// Transform destination vector into screen
	float fHWidth = m_Camera_Ptr->viewPort3D.Width/2;
	float fHHeight = m_Camera_Ptr->viewPort3D.Height/2;
	iX = iX - m_Camera_Ptr->viewPort3D.X;
	iY = iY - m_Camera_Ptr->viewPort3D.Y;
	iX = iX - fHWidth; iY = fHHeight - iY;
	GGVECTOR3 vecTo = GGVECTOR3 ( (float)iX/fHWidth, (float)iY/fHHeight, 1.0f );
	GGVec3TransformCoord ( &vecTo, &vecTo, &matInvertedProjection );
	GGVec3TransformCoord ( &vecTo, &vecTo, &matInvertedView );
	vecTo = vecTo-vecFrom;
	vecTo *= 3000.0f;

	// store normal vector with distance
	g_DBPROPickResult.iObjectID = 0;
	g_DBPROPickResult.fPickDistance = fDistance;
	g_DBPROPickResult.vecPickVector = vecTo - vecFrom;
	g_DBPROPickResult.vecFromVector = vecFrom;
	GGVec3Normalize ( &g_DBPROPickResult.vecPickVector, &g_DBPROPickResult.vecPickVector );
	g_DBPROPickResult.vecPickVector *= fDistance;
}

DARKSDK_DLL SDK_FLOAT GetPickDistance ( void )
{
	float fValue = g_DBPROPickResult.fPickDistance;
	return fValue;
}

DARKSDK_DLL SDK_FLOAT GetPickVectorX ( void )
{
	float fValue = g_DBPROPickResult.vecPickVector.x;
	return fValue;
}

DARKSDK_DLL SDK_FLOAT GetFromVectorX(void)
{
	float fValue = g_DBPROPickResult.vecFromVector.x;
	return fValue;
}

DARKSDK_DLL SDK_FLOAT GetPickVectorY ( void )
{
	float fValue = g_DBPROPickResult.vecPickVector.y;
	return fValue;
}

DARKSDK_DLL SDK_FLOAT GetFromVectorY(void)
{
	float fValue = g_DBPROPickResult.vecFromVector.y;
	return fValue;
}

DARKSDK_DLL SDK_FLOAT GetPickVectorZ ( void )
{
	float fValue = g_DBPROPickResult.vecPickVector.z;
	return fValue;
}
DARKSDK_DLL SDK_FLOAT GetFromVectorZ(void)
{
	float fValue = g_DBPROPickResult.vecFromVector.z;
	return fValue;
}

//////////////////////////////////////////////////////////////////////////////////
// EXPRESSIONS ///////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

DARKSDK_DLL int ObjectExist ( int iID )
{
	if ( iID < 1 || iID > MAXIMUMVALUE )
	{ 
		//PE: We use ObjectExist to check if a object can be used, so why this ? (LB: Just in case a dodgy index is passed in which can be caught and distinguished from an object that has not been created)
		//RunTimeError ( RUNTIMEERROR_B3DMODELNUMBERILLEGAL );
		return 0;
	}
	if ( iID < g_iObjectListCount )
	{ 
		if ( g_ObjectList [ iID ] )
		{
			if ( g_ObjectList [ iID ]->pFrame )
			{
				return 1;
			}
			if ( g_ObjectList [ iID ]->pInstanceOfObject )
			{
				return 1;
			}
		}
	}
	return 0;
}

DARKSDK_DLL int GetVisible ( int iID )
{
	// check the object exists
//	if ( !ConfirmObject ( iID ) ) // 190506 - u61 - instances have visibility
	if ( !ConfirmObjectInstance ( iID ) )
		return 0;

	// return object information
// lee - 101004 - U5.8 added universe state to visibility result
// V111 - 110608 - universe visibility messes up visibility checks in engine
//	if ( g_ObjectList [ iID ]->bUniverseVisible==true
//	&&   g_ObjectList [ iID ]->bVisible==true )
	if ( g_ObjectList [ iID ]->bVisible==true )
		return 1;
	else
		return 0;		
}


DARKSDK_DLL float ObjectSize ( int iID, int iActualSize )
{
    // check the object exists
	if ( !ConfirmObjectInstance ( iID ) )
		return 0;

	// return object information
	sObject* pObject = g_ObjectList [ iID ];
	float fValue = pObject->collision.fRadius;

	// if zero, still NEED a dimension, so calc now
	if ( fValue==0.0f )
	{
		float fDX = fabs(pObject->collision.vecMax.x - pObject->collision.vecMin.x);
		float fDY = fabs(pObject->collision.vecMax.y - pObject->collision.vecMin.y);
		float fDZ = fabs(pObject->collision.vecMax.z - pObject->collision.vecMin.z);
		fValue = fDX;
		if ( fDY>fValue ) fValue = fDY;
		if ( fDZ>fValue ) fValue = fDZ;
	}

	// scale optional
	if ( iActualSize==1 )
	{
		//050803 - adjusts to averaged scale now
		float fAvScale = (pObject->position.vecScale.x+pObject->position.vecScale.y+pObject->position.vecScale.z)/3.0f;
		fValue *= fAvScale;
	}
	return fValue;
}

DARKSDK_DLL float ObjectSizeX ( int iID, int iActualSize )
{
    return GetAxisSizeFromVectorOffset( iID, iActualSize, 0 /* 0 = x part of vector */);
}

DARKSDK_DLL float ObjectSizeY ( int iID, int iActualSize )
{
    return GetAxisSizeFromVectorOffset( iID, iActualSize, 1 /* 1 = y part of vector */ );
}

DARKSDK_DLL float ObjectSizeZ ( int iID, int iActualSize )
{
    return GetAxisSizeFromVectorOffset ( iID, iActualSize, 2 /* 2 = z part of vector */ );
}

DARKSDK_DLL float ObjectSize ( int iID )
{
	return ObjectSize ( iID, 0 );
}

DARKSDK_DLL float ObjectSizeX ( int iID )
{
	return ObjectSizeX ( iID, 0 );
}

DARKSDK_DLL float ObjectSizeY ( int iID )
{
	return ObjectSizeY ( iID, 0 );
}

DARKSDK_DLL float ObjectSizeZ ( int iID )
{
	return ObjectSizeZ ( iID, 0 );
}

DARKSDK_DLL float ObjectScaleX ( int iID )
{
    // check the object exists
	if ( !ConfirmObjectInstance ( iID ) ) return 0;
	sObject* pObject = g_ObjectList [ iID ];
	float fValue = pObject->position.vecScale[ 0 ] * 100;
	return fValue;
}

DARKSDK_DLL float ObjectScaleY ( int iID )
{
    // check the object exists
	if ( !ConfirmObjectInstance ( iID ) ) return 0;
	sObject* pObject = g_ObjectList [ iID ];
	float fValue = pObject->position.vecScale[ 1 ] * 100;
	return fValue;
}

DARKSDK_DLL float ObjectScaleZ ( int iID )
{
    // check the object exists
	if ( !ConfirmObjectInstance ( iID ) ) return 0;
	sObject* pObject = g_ObjectList [ iID ];
	float fValue = pObject->position.vecScale[ 2 ] * 100;
	return fValue;
}

// Animation Expressions

DARKSDK_DLL int GetNumberOfFrames ( int iID )
{
	// check the object exists
	if ( !ConfirmObject ( iID ) )
		return 0;

	// leefix - 150503 - use better value, was pObject->pAnimationSet->ulLength;
	// return object information
	sObject* pObject = g_ObjectList [ iID ];
	return (int)pObject->fAnimTotalFrames;
}

DARKSDK_DLL int GetPlaying ( int iID )
{
	// check the object exists
	if ( !ConfirmObject ( iID ) )
		return 0;

	// return object information
	sObject* pObject = g_ObjectList [ iID ];
	if ( pObject->bAnimPlaying )
		return 1;
	else
		return 0;
}

DARKSDK_DLL int GetLooping ( int iID )
{
	// check the object exists
	if ( !ConfirmObject ( iID ) )
		return 0;

	// return object information
	sObject* pObject = g_ObjectList [ iID ];
	if ( pObject->bAnimLooping )
		return 1;
	else
		return 0;
}

DARKSDK_DLL float GetFrame ( int iID )
{
	// check the object exists
	if ( !ConfirmObject ( iID ) )
		return 0;

	// return object information
	sObject* pObject = g_ObjectList [ iID ];

	#ifdef WICKEDENGINE
	float fValue = WickedCall_GetObjectFrame(pObject);
	#else
	float fValue = pObject->fAnimFrame;
	#endif
	return fValue;
}

DARKSDK_DLL float GetFrameEx ( int iID, int iLimbID )
{
	// check the object exists
	if ( !ConfirmObjectAndLimb ( iID, iLimbID ) )
		return 0;

	// get object ptr
	sObject* pObject = g_ObjectList [ iID ];
	float fValue = pObject->fAnimFrame;

	// can set an animation frame override per limb
	if ( pObject->pfAnimLimbFrame )
	{
		float fViewValue = pObject->pfAnimLimbFrame [ iLimbID ];
		if ( fViewValue >= 0.0f ) fValue = fViewValue;
	}

	// return object information
	return fValue;
}

DARKSDK_DLL float GetSpeed ( int iID )
{
	// check the object exists
	if ( !ConfirmObject ( iID ) )
		return 0;

	// return object information (leefix - 260604-u54-float conversion looses .000009)
	sObject* pObject = g_ObjectList [ iID ];
	float fValue = pObject->fAnimSpeed*100.00001f;
	return fValue;
}

DARKSDK_DLL float GetInterpolation ( int iID )
{
	// check the object exists
	if ( !ConfirmObject ( iID ) )
		return 0;

	// return object information (leefix - 260604-u54-float conversion looses .000009)
	sObject* pObject = g_ObjectList [ iID ];
	float fValue = pObject->fAnimInterp*100.00001f;
	return fValue;
}

