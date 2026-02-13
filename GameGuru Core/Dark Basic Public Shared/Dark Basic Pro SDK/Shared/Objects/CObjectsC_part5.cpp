// ScreenData Expressions
DARKSDK_DLL void DB_ObjectScreenData( sObject* pObject, int* x, int* y )
{
	// object 3D position 
	GGVECTOR3 vecPos = pObject->position.vecPosition;

	// camera ptr
	tagCameraData* m_Camera_Ptr = (tagCameraData*)GetCameraInternalData ( g_pGlob->dwCurrentSetCameraID );

	// get current camera transformation matrices
	GGMATRIX matTransform = m_Camera_Ptr->matView * m_Camera_Ptr->matProjection;

	// Transform object position from world-space to screen-space
	GGVec3TransformCoord(&vecPos, &vecPos, &matTransform);

	// Screen data
	*x=(int)((vecPos.x+1.0f)*(g_pGlob->iScreenWidth/2.0f));
	*y=(int)((1.0f-vecPos.y)*(g_pGlob->iScreenHeight/2.0f));

	#ifdef WICKEDENGINE
	// leefix - 280305 - adjust coordinates using viewport of the target camera
	float fVPWidth = m_Camera_Ptr->viewPort3D.Width;
	float fVPHeight = m_Camera_Ptr->viewPort3D.Height;
	// ZJ: Getting strange values for viewPort3D in standalone. Only adjust if they are valid.
	if (fVPWidth > 0 && fVPHeight > 0)
	{
		float fRealX = (fVPWidth / g_pGlob->iScreenWidth) * (*x);
		float fRealY = (fVPHeight / g_pGlob->iScreenHeight) * (*y);
		*x = fRealX + m_Camera_Ptr->viewPort3D.X;
		*y = fRealY + m_Camera_Ptr->viewPort3D.Y;
	}
	#else
	// leefix - 280305 - adjust coordinates using viewport of the target camera
	float fVPWidth = m_Camera_Ptr->viewPort3D.Width;
	float fVPHeight = m_Camera_Ptr->viewPort3D.Height;
	float fRealX = (fVPWidth / g_pGlob->iScreenWidth) * (*x);
	float fRealY = (fVPHeight / g_pGlob->iScreenHeight) * (*y);
	*x = fRealX + m_Camera_Ptr->viewPort3D.X;
	*y = fRealY + m_Camera_Ptr->viewPort3D.Y;
	#endif
}

DARKSDK_DLL int GetScreenX ( int iID )
{
	// check the object exists
	if ( !ConfirmObjectInstance ( iID ) )
		return 0;

	// calculate screendata
	int x, y;
	sObject* pObject = g_ObjectList [ iID ];
	DB_ObjectScreenData ( pObject, &x, &y );
	return x;
}

DARKSDK_DLL int GetScreenY ( int iID )
{
	// check the object exists
	if ( !ConfirmObjectInstance ( iID ) )
		return 0;

	// calculate screendata
	int x, y;
	sObject* pObject = g_ObjectList [ iID ];
	DB_ObjectScreenData ( pObject, &x, &y );
	return y;
}

DARKSDK_DLL int GetInScreen ( int iID )
{
	// check the object exists
	if ( !ConfirmObjectInstance ( iID ) )
		return 0;

	sObject* pObject = g_ObjectList [ iID ];

    // u74b7 - Object centre needs to be adjusted by the mesh centre to give correct results
	GGVECTOR3 vBob = pObject->position.vecPosition + pObject->collision.vecCentre;

	// camera ptr
	tagCameraData* pCamera = (tagCameraData*)GetCameraInternalData ( g_pGlob->dwCurrentSetCameraID );

	// get current camera transformation matrices
	GGMATRIX matCamera = pCamera->matView * pCamera->matProjection;

    // u74b7 - Test whether the object 'sphere' in viewing frustum
    GGPLANE p_Planes[6];
    ExtractFrustumPlanes(p_Planes, matCamera);
    if (ContainsSphere(p_Planes, vBob, pObject->collision.fScaledRadius))
        return 1;
    else
        return 0;
}

// Collision Expressions

DARKSDK_DLL int GetCollision ( int iObjectA, int iObjectB )
{
	return CheckCol ( iObjectA, iObjectB );
}

DARKSDK_DLL int GetHit ( int iObjectA, int iObjectB )
{
	return CheckHit ( iObjectA, iObjectB );
}

DARKSDK_DLL int GetLimbCollision ( int iObjectA, int iLimbA, int iObjectB, int iLimbB )
{
	return CheckLimbCol ( iObjectA, iLimbA, iObjectB, iLimbB );
}

DARKSDK_DLL int GetLimbHit ( int iObjectA, int iLimbA, int iObjectB, int iLimbB )
{
	return CheckLimbHit ( iObjectA, iLimbA, iObjectB, iLimbB );
}

DARKSDK_DLL float GetObjectCollisionRadius ( int iID )
{
	// check the object exists
	if ( !ConfirmObject ( iID ) )
		return 0;

	// return object information
	sObject* pObject = g_ObjectList [ iID ];
	float fValue = GetColRadius ( pObject );
	return fValue;
}

DARKSDK_DLL float GetObjectCollisionCenterX ( int iID )
{
	// check the object exists
	//if ( !ConfirmObject ( iID ) )
	//	return 0;

	// mike - 120307 - allow instanced objects
	if ( !ConfirmObjectInstance ( iID ) )
		return 0;

	// return object information
	sObject* pObject = g_ObjectList [ iID ];
	float fValue = GetColCenterX ( pObject );
	return fValue;
}

DARKSDK_DLL float GetObjectCollisionCenterY ( int iID )
{
	// check the object exists
	//if ( !ConfirmObject ( iID ) )
	//	return 0;

	// mike - 120307 - allow instanced objects
	if ( !ConfirmObjectInstance ( iID ) )
		return 0;

	// return object information
	sObject* pObject = g_ObjectList [ iID ];
	float fValue = GetColCenterY ( pObject );
	return fValue;
}

DARKSDK_DLL float GetObjectCollisionCenterZ ( int iID )
{
	// check the object exists
	//if ( !ConfirmObject ( iID ) )
	//	return 0;

	// mike - 120307 - allow instanced objects
	if ( !ConfirmObjectInstance ( iID ) )
		return 0;

	// return object information
	sObject* pObject = g_ObjectList [ iID ];
	float fValue = GetColCenterZ ( pObject );
	return fValue;
}

DARKSDK_DLL int GetStaticHit (	float fOldX1, float fOldY1, float fOldZ1, float fOldX2, float fOldY2, float fOldZ2,
					float fNX1,   float fNY1,   float fNZ1,   float fNX2,   float fNY2,   float fNZ2    )
{
	return GetStaticHitEx (	fOldX1, fOldY1, fOldZ1, fOldX2, fOldY2, fOldZ2,
							fNX1,   fNY1,   fNZ1,   fNX2,   fNY2,   fNZ2    );
}

DARKSDK_DLL int GetStaticLineOfSight ( float fSx, float fSy, float fSz, float fDx, float fDy, float fDz, float fWidth, float fAccuracy )
{
	return GetStaticLineOfSightEx( fSx, fSy, fSz, fDx, fDy, fDz, fWidth, fAccuracy );
}

DARKSDK_DLL int GetStaticRayCast ( float fSx, float fSy, float fSz, float fDx, float fDy, float fDz )
{
	// returns a one if collision, details in checklist
	return 0;
}

DARKSDK_DLL int GetStaticVolumeCast ( float fX, float fY, float fZ, float fNewX, float fNewY, float fNewZ, float fSize )
{
	return 0;
}

DARKSDK_DLL SDK_FLOAT GetStaticX ( void )
{
	float result = GetStaticColX();
	return result;
}

DARKSDK_DLL SDK_FLOAT GetStaticY ( void )
{
	float result = GetStaticColY();
	return result;
}

DARKSDK_DLL SDK_FLOAT GetStaticZ ( void )
{
	float result = GetStaticColZ();
	return result;
}

DARKSDK_DLL int GetStaticFloor ( void )
{
	return GetCollidedAgainstFloor();
}

DARKSDK_DLL int GetStaticColCount ( void )
{
	return GetStaticColPolysChecked();
}

DARKSDK_DLL int GetStaticColValue ( void )
{
	return (int)GetStaticColArbitaryValue();
}

DARKSDK_DLL SDK_FLOAT GetStaticLineOfSightX ( void )
{
	float result = GetStaticLineOfSightExX();
	return result;
}

DARKSDK_DLL SDK_FLOAT GetStaticLineOfSightY ( void )
{
	float result = GetStaticLineOfSightExY();
	return result;
}

DARKSDK_DLL SDK_FLOAT GetStaticLineOfSightZ ( void )
{
	float result = GetStaticLineOfSightExZ();
	return result;
}

DARKSDK_DLL SDK_FLOAT GetCollisionX ( void )
{
	float result = GetColX();
	return result;
}

DARKSDK_DLL SDK_FLOAT GetCollisionY ( void )
{
	float result = GetColY();
	return result;
}

DARKSDK_DLL SDK_FLOAT GetCollisionZ ( void )
{
	float result = GetColZ();
	return result;
}

// Limb Expressions

DARKSDK_DLL int LimbExist ( int iID, int iLimbID )
{
	// check the object exists
	if ( !ConfirmObjectInstance ( iID ) )
		return 0;

	// check limb range
	if ( iLimbID < 0 || iLimbID > MAXIMUMVALUE )
	{ 
		RunTimeError ( RUNTIMEERROR_LIMBNUMBERILLEGAL );
		return 0;
	}

	// actual object or instance of object
	sObject* pObject = g_ObjectList [ iID ];
	if ( pObject->pInstanceOfObject )
	{
		// record hide state in instance-mesh-visibility-array
		sObject* pActualObject = pObject->pInstanceOfObject;
		if ( iLimbID>=0 && iLimbID<pActualObject->iFrameCount)
			return 1;
		else
			return 0;
	}
	else
	{
		if ( iLimbID < g_ObjectList [ iID ]->iFrameCount )
			return 1;
		else
			return 0;
	}
}

DARKSDK_DLL float LimbOffsetX ( int iID, int iLimbID )
{
	// check the object exists
	if ( !ConfirmObjectAndLimb ( iID, iLimbID ) )
		return 0;

	// specific limb/frame information
	sObject* pObject = g_ObjectList [ iID ];
	sFrame* pFrame = pObject->ppFrameList [ iLimbID ];
	UpdateRealtimeFrameVectors ( pObject, pFrame );
	return pFrame->vecOffset.x;
}

DARKSDK_DLL float LimbOffsetY ( int iID, int iLimbID )
{
	// check the object exists
	if ( !ConfirmObjectAndLimb ( iID, iLimbID ) )
		return 0;

	// specific limb/frame information
	sObject* pObject = g_ObjectList [ iID ];
	sFrame* pFrame = pObject->ppFrameList [ iLimbID ];
	UpdateRealtimeFrameVectors ( pObject, pFrame );
	return pFrame->vecOffset.y;
}

DARKSDK_DLL float LimbOffsetZ ( int iID, int iLimbID )
{
	// check the object exists
	if ( !ConfirmObjectAndLimb ( iID, iLimbID ) )
		return 0;

	// specific limb/frame information
	sObject* pObject = g_ObjectList [ iID ];
	sFrame* pFrame = pObject->ppFrameList [ iLimbID ];
	UpdateRealtimeFrameVectors ( pObject, pFrame );
	return pFrame->vecOffset.z;
}

DARKSDK_DLL float LimbAngleX ( int iID, int iLimbID )
{
	// check the object exists
	if ( !ConfirmObjectAndLimb ( iID, iLimbID ) )
		return 0;

	// specific limb/frame information
	sObject* pObject = g_ObjectList [ iID ];
	sFrame* pFrame = pObject->ppFrameList [ iLimbID ];
	UpdateRealtimeFrameVectors ( pObject, pFrame );
	return pFrame->vecRotation.x;
}

DARKSDK_DLL float LimbAngleY ( int iID, int iLimbID )
{
	// check the object exists
	if ( !ConfirmObjectAndLimb ( iID, iLimbID ) )
		return 0;

	// specific limb/frame information
	sObject* pObject = g_ObjectList [ iID ];
	sFrame* pFrame = pObject->ppFrameList [ iLimbID ];
	UpdateRealtimeFrameVectors ( pObject, pFrame );
	return pFrame->vecRotation.y;
}

DARKSDK_DLL float LimbAngleZ ( int iID, int iLimbID )
{
	// check the object exists
	if ( !ConfirmObjectAndLimb ( iID, iLimbID ) )
		return 0;

	// specific limb/frame information
	sObject* pObject = g_ObjectList [ iID ];
	sFrame* pFrame = pObject->ppFrameList [ iLimbID ];
	UpdateRealtimeFrameVectors ( pObject, pFrame );
	return pFrame->vecRotation.z;
}

void LimbPositionCore (int iID, int iLimbID, GGVECTOR3* pvecPos)
{
	// actual or instanced
	sObject* pObject = g_ObjectList[iID];
	sObject* pActualObject = pObject;
	if (pObject->pInstanceOfObject)
		pActualObject = pObject->pInstanceOfObject;

	// if this object was glued, need to interrogate Wicked to get child position
	if (pObject->position.bGlued == true)
	{
		GGVECTOR3 vecPosWorld;
		WickedCall_GetGluedLimbWorldPos(pObject, iLimbID, &vecPosWorld.x, &vecPosWorld.y, &vecPosWorld.z);
		*pvecPos = vecPosWorld;
	}
	else
	{
		// get frame of object
		sFrame* pFrame = pActualObject->ppFrameList[iLimbID];
		if (pObject->pInstanceOfObject) pFrame->bVectorsCalculated = false;

		// specific limb/frame information
		WickedCall_GetFrameWorldPos(pFrame, &pFrame->vecPosition.x, &pFrame->vecPosition.y, &pFrame->vecPosition.z);
		*pvecPos = pFrame->vecPosition;
	}
}

DARKSDK_DLL float LimbPositionX ( int iID, int iLimbID )
{
	// check the object exists
	if ( !ConfirmObjectAndLimbInstance ( iID, iLimbID ) )
		return 0;

	// return correct position value
	GGVECTOR3 vecPos;
	LimbPositionCore (iID, iLimbID, &vecPos);
	float fValue = vecPos.x;
	return fValue;
}

DARKSDK_DLL float LimbPositionY ( int iID, int iLimbID )
{
	// check the object exists
	if ( !ConfirmObjectAndLimbInstance ( iID, iLimbID ) )
		return 0;

	// return correct position value
	GGVECTOR3 vecPos;
	LimbPositionCore (iID, iLimbID, &vecPos);
	float fValue = vecPos.y;
	return fValue;
}

DARKSDK_DLL float LimbPositionZ ( int iID, int iLimbID )
{
	// check the object exists
	if ( !ConfirmObjectAndLimbInstance ( iID, iLimbID ) )
		return 0;

	// return correct position value
	GGVECTOR3 vecPos;
	LimbPositionCore (iID, iLimbID, &vecPos);
	float fValue = vecPos.z;
	return fValue;

	/* old way
	// actual or instanced
	sObject* pObject = g_ObjectList [ iID ];
	sObject* pActualObject = pObject;
	if ( pObject->pInstanceOfObject )
		pActualObject = pObject->pInstanceOfObject;

	// get frame of object
	sFrame* pFrame = pActualObject->ppFrameList [ iLimbID ];
	if ( pObject->pInstanceOfObject ) pFrame->bVectorsCalculated = false;

	// specific limb/frame information
	#ifdef WICKEDENGINE
	WickedCall_GetFrameWorldPos(pFrame, &pFrame->vecPosition.x, & pFrame->vecPosition.y, & pFrame->vecPosition.z);
	#else
	UpdateRealtimeFrameVectors ( pObject, pFrame );
	#endif
	float fValue = pFrame->vecPosition.z;
	return fValue;
	*/
}

DARKSDK_DLL float LimbDirectionX ( int iID, int iLimbID )
{
	// check the object exists
	if ( !ConfirmObjectAndLimb ( iID, iLimbID ) )
		return 0;

	// specific limb/frame information
	sObject* pObject = g_ObjectList [ iID ];
	sFrame* pFrame = pObject->ppFrameList [ iLimbID ];
	UpdateRealtimeFrameVectors ( pObject, pFrame );
	float fValue = pFrame->vecDirection.x;
	return fValue;
}

DARKSDK_DLL float LimbDirectionY ( int iID, int iLimbID )
{
	// check the object exists
	if ( !ConfirmObjectAndLimb ( iID, iLimbID ) )
		return 0;

	// specific limb/frame information
	sObject* pObject = g_ObjectList [ iID ];
	sFrame* pFrame = pObject->ppFrameList [ iLimbID ];
	UpdateRealtimeFrameVectors ( pObject, pFrame );
	float fValue = pFrame->vecDirection.y;
	return fValue;
}

DARKSDK_DLL float LimbDirectionZ ( int iID, int iLimbID )
{
	// check the object exists
	if ( !ConfirmObjectAndLimb ( iID, iLimbID ) )
		return 0;

	// specific limb/frame information
	sObject* pObject = g_ObjectList [ iID ];
	sFrame* pFrame = pObject->ppFrameList [ iLimbID ];
	UpdateRealtimeFrameVectors ( pObject, pFrame );
	float fValue = pFrame->vecDirection.z;
	return fValue;
}

DARKSDK_DLL float LimbScaleX ( int iID, int iLimbID )
{
	// check the object exists
	if ( !ConfirmObjectAndLimb ( iID, iLimbID ) )
		return 0;

	// specific limb/frame information
	sObject* pObject = g_ObjectList [ iID ];
	sFrame* pFrame = pObject->ppFrameList [ iLimbID ];
	UpdateRealtimeFrameVectors ( pObject, pFrame );
	float fValue = pFrame->vecScale.x*100.0f;
	return fValue;
}

DARKSDK_DLL float LimbScaleY ( int iID, int iLimbID )
{
	// check the object exists
	if ( !ConfirmObjectAndLimb ( iID, iLimbID ) )
		return 0;

	// specific limb/frame information
	sObject* pObject = g_ObjectList [ iID ];
	sFrame* pFrame = pObject->ppFrameList [ iLimbID ];
	UpdateRealtimeFrameVectors ( pObject, pFrame );
	float fValue = pFrame->vecScale.y*100.0f;
	return fValue;
}

DARKSDK_DLL float LimbScaleZ ( int iID, int iLimbID )
{
	// check the object exists
	if ( !ConfirmObjectAndLimb ( iID, iLimbID ) )
		return 0;

	// specific limb/frame information
	sObject* pObject = g_ObjectList [ iID ];
	sFrame* pFrame = pObject->ppFrameList [ iLimbID ];
	UpdateRealtimeFrameVectors ( pObject, pFrame );
	float fValue = pFrame->vecScale.z*100.0f;
	return fValue;
}

DARKSDK_DLL int LimbTexture ( int iID, int iLimbID, int iTextureStage )
{
	// check the object exists
	if ( !ConfirmObjectAndLimb ( iID, iLimbID ) )
		return 0;

	// ensure limb has mesh
	sMesh* pMesh = g_ObjectList [ iID ]->ppFrameList [ iLimbID ]->pMesh;
	if ( !pMesh )
		return 0;

	// Ensure iTextureStage is valid
	if ( iTextureStage < 0 && iTextureStage >= (int)pMesh->dwTextureCount )
		return NULL;

	// return stage zero texture image
	if ( pMesh->pTextures )
		return pMesh->pTextures [ iTextureStage ].iImageID;

	// no texture
	return 0;
}

DARKSDK_DLL int LimbTexture ( int iID, int iLimbID )
{
	return LimbTexture ( iID, iLimbID, 0 );
}

DARKSDK_DLL int GetLimbTexturePtr ( int iID, int iLimbID )
{
	// check the object exists
	if ( !ConfirmObjectAndLimb ( iID, iLimbID ) )
		return 0;

	// ensure limb has mesh
	sMesh* pMesh = g_ObjectList [ iID ]->ppFrameList [ iLimbID ]->pMesh;
	if ( !pMesh )
		return 0;

	// return ptr as int (for determining texture matching)
	int iPtrValue = 0;
	if ( pMesh->pTextures )
		iPtrValue = (int)pMesh->pTextures [ 0 ].pTexturesRef;
	return iPtrValue;
}

DARKSDK_DLL int LimbVisible ( int iID, int iLimbID )
{
	// check the object exists
	if ( !ConfirmObjectAndLimbInstance ( iID, iLimbID ) )
		return 0;

	// actual object or instance of object
	sObject* pObject = g_ObjectList [ iID ];
	if ( pObject->pInstanceMeshVisible )
	{
		// record hide state in instance-mesh-visibility-array
		sObject* pActualObject = pObject->pInstanceOfObject;
		if ( iLimbID>=0 && iLimbID<pActualObject->iFrameCount)
		{
			if ( pObject->pInstanceMeshVisible [ iLimbID ] )
				return 1;
			else
				return 0;
		}
		else
			return 0;
	}
	else
	{
		// ensure limb has mesh
		sMesh* pMesh = g_ObjectList [ iID ]->ppFrameList [ iLimbID ]->pMesh;
		if ( !pMesh )
			return 0;

		// specific limb/frame information
		if ( pMesh->bVisible )
			return 1;
		else
			return 0;
	}
}

DARKSDK_DLL int LimbLink ( int iID, int iLimbID )
{
	// check the object exists
	if ( !ConfirmObjectAndLimb ( iID, iLimbID ) )
		return 0;

	// can always modify links in limbs now
	return 1;
}

DARKSDK_DLL int GetLimbPolygonCount ( int iID, int iLimbID )
{
	// check the object exists
	if ( !ConfirmObjectAndLimbInstance ( iID, iLimbID ) )
		return 0;

	// actual object
	sObject* pObject = g_ObjectList [ iID ];
	if ( pObject==NULL ) return 0;

	// get frame from object
	sFrame** pFrameList = pObject->ppFrameList;
	if ( pFrameList==NULL ) return 0;
	if ( iLimbID>=pObject->iFrameCount ) return 0;

	// get mesh from object
	sMesh* pMesh = pFrameList[ iLimbID ]->pMesh;
	if ( pMesh==NULL ) return 0;

	// return polygon count for this limb
	return pMesh->iDrawPrimitives;
}

DARKSDK_DLL int GetMultiMaterialCount ( int iID )
{
	// check the object exists
	if ( !ConfirmObjectInstance ( iID ) )
		return 0;

	// actual object
	sObject* pObject = g_ObjectList [ iID ];
	if ( pObject==NULL ) return 0;

	// add all multiaterial counts
	if ( pObject->pInstanceOfObject ) pObject = pObject->pInstanceOfObject;
	DWORD dwTotalMaterialCount = 0;
	for ( int iMesh = 0; iMesh < pObject->iMeshCount; iMesh++ )
	{
		sMesh* pMesh = pObject->ppMeshList [ iMesh ];
		if ( pMesh )
			if ( pMesh->bUseMultiMaterial==true )
				dwTotalMaterialCount+=pMesh->dwMultiMaterialCount;
	}

	// return total
	return dwTotalMaterialCount;
}

DARKSDK_DLL LPSTR LimbTextureName ( int iID, int iLimbID, int iTextureStage )
{
	// check the object exists
	if ( !ConfirmObjectAndLimb ( iID, iLimbID ) )
		return NULL;
	
	// texture name to return
	LPSTR pTextureLimbName = NULL;

	// get ptrs
	sObject* pObject = g_ObjectList [ iID ];
	sMesh* pMesh = pObject->ppFrameList [ iLimbID ]->pMesh;
	if ( pMesh )
	{
		// lee - 220306 - u6b4 - if multimaterial, texturestage becomes index to material
		if ( pMesh->dwMultiMaterialCount > 0 && pMesh->pMultiMaterial )
		{
			// ensure materiual inde xis valid
			int iMaterialIndex = iTextureStage ; // re-use
			//if ( iMaterialIndex < 0 && iMaterialIndex >= (int)pMesh->dwMultiMaterialCount ) // leefix - 210806 - fixed condition
			if ( iMaterialIndex < 0 || iMaterialIndex >= (int)pMesh->dwMultiMaterialCount )
			{
				// skip
				pTextureLimbName = "";
			}
			else
			{
				// get name of texturename of material in framemesh
				pTextureLimbName = pMesh->pMultiMaterial [ iMaterialIndex ].pName;
			}
		}
		else
		{
			// Ensure iTextureStage is valid
			//if ( iTextureStage < 0 && iTextureStage >= (int)pMesh->dwTextureCount ) // leefix - 210806 - fixed condition
			if ( iTextureStage < 0 || iTextureStage >= (int)pMesh->dwTextureCount )
			{
				// skip
				pTextureLimbName = "";
			}
			else
			{
				// get name of texture in framemesh
				pTextureLimbName = "";
				if ( pMesh->pTextures )
					pTextureLimbName = pMesh->pTextures [ iTextureStage ].pName;
			}
		}
	}
	else
		pTextureLimbName = "";

	// Allocate new size
	LPSTR pString = NULL;
	DWORD dwSize = strlen ( pTextureLimbName );
	g_pGlob->CreateDeleteString((char**)&pString, dwSize+1);
	ZeroMemory ( pString, dwSize+1 );
	memcpy ( pString, pTextureLimbName, dwSize );

	// Return String
	return pString;
}

DARKSDK_DLL LPSTR LimbTextureName ( int iID, int iLimbID )
{
	return LimbTextureName ( iID, iLimbID, 0 );
	//#ifdef SDK_RETSTR
	// return GetLimbTextureNameEx ( lpStr, iID, iLimbID, 0 );
	//#else
	// return GetLimbTextureNameEx ( iID, iLimbID, 0 );
	//#endif
}

//PE: Mem never freed.
char cLimbName[1024];

DARKSDK_DLL LPSTR LimbName(int iID, int iLimbID)
{
	// check the object exists
	if (!ConfirmObjectAndLimb(iID, iLimbID))
		return NULL;

	// get name of frame
	sObject* pObject = g_ObjectList[iID];
	LPSTR pLimbName = pObject->ppFrameList[iLimbID]->szName;

	// Allocate new size
	LPSTR pString = NULL;
	DWORD dwSize = strlen(pLimbName);
	if (dwSize < 1024)
	{
		strcpy(cLimbName, pLimbName);
		pString = &cLimbName[0];
	}
	else
	{
		g_pGlob->CreateDeleteString((char**)&pString, dwSize + 1);
		ZeroMemory(pString, dwSize + 1);
		memcpy(pString, pLimbName, dwSize);
	}

	// Return String
	return pString;
}

DARKSDK_DLL LPSTR LimbNameOLD ( int iID, int iLimbID )
{
	// check the object exists
	if ( !ConfirmObjectAndLimb ( iID, iLimbID ) )
		return NULL;
	
	// get name of frame
	sObject* pObject = g_ObjectList [ iID ];
	LPSTR pLimbName = pObject->ppFrameList [ iLimbID ]->szName;

	// Allocate new size
	LPSTR pString = NULL;
	DWORD dwSize = strlen ( pLimbName );
	g_pGlob->CreateDeleteString((char**)&pString, dwSize+1);
	ZeroMemory ( pString, dwSize+1 );
	memcpy ( pString, pLimbName, dwSize );

	// Return String
	return pString;
}

DARKSDK_DLL int GetLimbCount ( int iID )
{
	// check the object exists
	if ( !ConfirmObject ( iID ) )
		return 0;

	// get object ptr
	sObject* pObject = g_ObjectList [ iID ];

	// specific limb/frame information
	return pObject->iFrameCount;
}

// Mesh Expressions

DARKSDK_DLL int GetMeshExist ( int iID )
{
	if ( iID < g_iRawMeshListCount )
		if ( g_RawMeshList [ iID ] )
			return 1;

	return 0;
}

// Shader Expressions

DARKSDK_DLL SDK_BOOL PixelShaderExist ( int iShader )
{
	// if shader value valid
	if ( iShader < 0 || iShader > MAX_VERTEX_SHADERS )
	{
		RunTimeError(RUNTIMEERROR_B3DVSHADERNUMBERILLEGAL);
		return 0;
	}

	// if shader exists
	if ( m_PixelShaders [ iShader ].pPixelShader )
		return 1;
	else
		return 0;
}

DARKSDK_DLL SDK_BOOL VertexShaderExist ( int iShader )
{
	// if shader value valid
	if ( iShader < 0 || iShader > MAX_VERTEX_SHADERS )
	{
		RunTimeError(RUNTIMEERROR_B3DVSHADERNUMBERILLEGAL);
		return 0;
	}

	// if shader exists
	if ( m_VertexShaders [ iShader ].pVertexShader )
		return 1;
	else
		return 0;
}

// Caps Expressions

DARKSDK_DLL int Get3DBLITAvailable ( void )
{
	// OBSOLETE
	return 1;
}

DARKSDK_DLL int GetStatistic ( int iCode )
{
	// Active for hidden-values
	switch(iCode)
	{
		case 1 :	// Polycount monitored from scene geometry
					if ( g_pGlob )
						return g_pGlob->dwNumberOfPolygonsDrawn;
					else
						return 0;

		case 2 :	// Stencil buffer available in current mode
					return 0;

		case 3:		// Current Universe Area Box Of Camera Zero
					return 0;

		case 4:		// Total Number Of Universe Area Boxes
					return 0;

		case 5 :	// DrawPrimitive calls monitored from scene geometry
					if ( g_pGlob )
						return g_pGlob->dwNumberOfPrimCalls;
					else
						return 0;

		case 6 :	// Polygons from current areabox (universe)
					return 0;

		case 7:		// DrawPrimitive calls from current areabox (universe)
					return 0;

		case 8:		// Polygons tested for collision
					return 0;

		case 9:		// Volumes tested for collision
					return 0;	

		case 10:	// Occluded Objects
					return 0;	
	}
	return 0;
}

DARKSDK_DLL SDK_FLOAT GetPixelShaderVersion ( void )
{
	float fVersion = 0.0f;
	#ifdef DX11
	#else
	if(m_Caps.MaxStreams>0)
	{
		unsigned char sub = *((LPSTR)(&m_Caps.PixelShaderVersion));
		fVersion = *((LPSTR)(&m_Caps.PixelShaderVersion)+1);
		fVersion += ((float)sub/10.0f);
	}
	#endif
	return SDK_RETFLOAT ( fVersion );
}

DARKSDK_DLL SDK_FLOAT GetMaxPixelShaderValue ( void )
{
	#ifdef DX11
	return 0;
	#else
	return SDK_RETFLOAT ( m_Caps.PixelShader1xMaxValue );
	#endif
}

//
// Shadows
//

DARKSDK_DLL void SetGlobalShadowsOn ( void )
{
	m_ObjectManager.SetGlobalShadowsOn();
}

DARKSDK_DLL void SetGlobalShadowsOff ( void )
{
	m_ObjectManager.SetGlobalShadowsOff();
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

// mike 090903 - now uses a pointer
DARKSDK_DLL void SetWorldMatrix ( int iID, GGMATRIX* pMatrix )
{
	if ( !ConfirmObject ( iID ) )
		return;

	// get object ptr
	sObject* pObject = g_ObjectList [ iID ];
	memcpy ( &pObject->position.matWorld, pMatrix, sizeof ( GGMATRIX ) );
	pObject->position.bCustomWorldMatrix = true;
}

// mike - 040903 - updates a structure
DARKSDK_DLL void UpdatePositionStructure ( sPositionData* pPosition )
{
	sObject object;

	object.position = *pPosition;

	CalculateObjectWorld ( &object, NULL );
}

DARKSDK_DLL void GetWorldMatrix ( int iID, int iLimb, GGMATRIX* pMatrix )
{
	if ( !ConfirmObject ( iID ) )
		return;

	sObject* pObject = g_ObjectList [ iID ];

	CalculateObjectWorld ( pObject, NULL );

	//CalculateAbsoluteWorldMatrix ( pObject, pObject->ppFrameList [ iLimb ], pObject->ppFrameList [ iLimb ]->pMesh );

	*pMatrix = pObject->ppFrameList [ iLimb ]->matCombined * pObject->position.matWorld;
}

DARKSDK_DLL GGVECTOR3 GetCameraLook ( void )
{
	tagCameraData* pCameraData = ( tagCameraData* ) GetCameraInternalData ( g_pGlob->dwCurrentSetCameraID );

	return pCameraData->vecLook;
}

DARKSDK_DLL GGVECTOR3 GetCameraPosition ( void )
{
	tagCameraData* pCameraData = ( tagCameraData* ) GetCameraInternalData ( g_pGlob->dwCurrentSetCameraID );

	return pCameraData->vecPosition;
}

DARKSDK_DLL GGVECTOR3 GetCameraUp ( void )
{
	tagCameraData* pCameraData = ( tagCameraData* ) GetCameraInternalData ( g_pGlob->dwCurrentSetCameraID );

	return pCameraData->vecUp;
}

DARKSDK_DLL GGVECTOR3 GetCameraRight ( void )
{
	tagCameraData* pCameraData = ( tagCameraData* ) GetCameraInternalData ( g_pGlob->dwCurrentSetCameraID );

	return pCameraData->vecRight;
}

DARKSDK_DLL GGMATRIX GetCameraMatrix ( void )
{
	tagCameraData* pCameraData = ( tagCameraData* ) GetCameraInternalData ( g_pGlob->dwCurrentSetCameraID );

	return pCameraData->matView;
}

DARKSDK_DLL void ExcludeOn ( int iID )
{
	if ( !CheckObjectExist ( iID ) )
		return;

	sObject* pObject = g_ObjectList [ iID ];
	pObject->bExcluded = true;
	if ( pObject->pInstanceOfObject ) pObject->pInstanceOfObject->bExcluded = true; // 131107 - added as seemd to be missing?

	m_ObjectManager.UpdateTextures ( );
}

DARKSDK_DLL void ExcludeOff ( int iID )
{
	if ( !CheckObjectExist ( iID ) )
		return;

	sObject* pObject = g_ObjectList [ iID ];
	pObject->bExcluded = false;
	if ( pObject->pInstanceOfObject ) pObject->pInstanceOfObject->bExcluded = false;

	m_ObjectManager.UpdateTextures ( );
}

DARKSDK_DLL void ExcludeLimbOn ( int iID, int iLimbID )
{
	// 301007 - new command
	if ( !CheckObjectExist ( iID ) )
		return;

	// exclude limb if exists
	sObject* pObject = g_ObjectList [ iID ];
	if ( pObject )
		if ( pObject->ppFrameList )
			if ( iLimbID < pObject->iFrameCount )
				if ( pObject->ppFrameList [ iLimbID ] )
					pObject->ppFrameList [ iLimbID ]->bExcluded = true;
}

DARKSDK_DLL void ExcludeLimbOff	( int iID, int iLimbID )
{
	// 301007 - new command
	if ( !CheckObjectExist ( iID ) )
		return;

	// exclude limb if exists
	sObject* pObject = g_ObjectList [ iID ];
	if ( pObject )
		if ( pObject->ppFrameList )
			if ( iLimbID < pObject->iFrameCount )
				if ( pObject->ppFrameList [ iLimbID ] )
					pObject->ppFrameList [ iLimbID ]->bExcluded = false;
}

DARKSDK_DLL void SetGlobalObjectCreationMode ( int iMode )
{
    // If bit 1 isn't set, then allow individual buffers
    g_bGlobalVBIBUsageFlag = ! (iMode & 1);

    // Set the rendering order
	switch ( iMode )
    {
    // By Texture
    case 0:
    case 1:
        g_eGlobalSortOrder = E_SORT_BY_TEXTURE;
		break;

	// No particular order
    case 2:
    case 3:
        g_eGlobalSortOrder = E_SORT_BY_NONE;
		break;

	// By Object number
    case 4:
    case 5:
        g_eGlobalSortOrder = E_SORT_BY_OBJECT;
		break;

	// By Reverse Distance
    case 6:
    case 7:
        g_eGlobalSortOrder = E_SORT_BY_DEPTH;
		break;

    // Ignore anything else
    default:
        break;
	}
	m_ObjectManager.UpdateTextures ( );
}

DARKSDK_DLL sObject* GetObjectData ( int iID )
{
	// MIKE - 050104 - function to access an object
//	if ( !ConfirmObject ( iID ) ) // 180506 - u61 - tpc can get instanced objects
//	if ( !ConfirmObjectInstance ( iID ) )
//		return NULL;

	// lee - 040914 - needs to be silent fail
	if ( iID < 1 || iID > MAXIMUMVALUE )
		return NULL;

	if ( iID < g_iObjectListCount )
		return g_ObjectList [ iID ];
	else
		return NULL;
}

// mike - 230505 - need to be able to set mip map LOD bias on a per mesh basis
void SetObjectMipMapLODBias	( int iID, int iLimb, float fBias )
{
	// ensure the object is present
	if ( !ConfirmObject ( iID ) )
		return;

	// get object ptr
	sObject* pObject = g_ObjectList [ iID ];

	if ( iLimb == -1 )
	{
		for ( int iMesh = 0; iMesh < pObject->iMeshCount; iMesh++ )
			pObject->ppMeshList [ iMesh ]->fMipMapLODBias = fBias;

		return;
	}

	if ( iLimb >= pObject->iMeshCount || iLimb < 0 )
		return;

	pObject->ppMeshList [ iLimb ]->fMipMapLODBias = fBias;
}

// mike - 230505 - need to be able to set mip map LOD bias on a per mesh basis
void SetObjectMipMapLODBias	( int iID, float fBias )
{
	SetObjectMipMapLODBias ( iID, -1, fBias );
}

// mike - 300905 - command to update object bounds
void CalculateObjectBounds ( int iID )
{
	// check the object limb exists
	if ( !ConfirmObject ( iID ) )
		return;

	CalculateAllBounds ( g_ObjectList [ iID ], false );
}

void CalculateObjectBoundsEx ( int iID, int iOnlyUpdateFrames )
{
	// check the object limb exists
	if ( !ConfirmObject ( iID ) )
		return;

	// only update frame matices OR update ALL BOUNDS (performance intense)
	if ( iOnlyUpdateFrames==1 )
	{
		// get object ptr
		sObject* pObject = g_ObjectList [ iID ];

		// store current animation states
		bool bStoreAnimPlaying = pObject->bAnimPlaying;
		float bStoreAnimFrame = pObject->fAnimFrame;
		bool bStoreAnimLooping = pObject->bAnimLooping;
		float bStoreAnimSlerpTime = pObject->fAnimSlerpTime;
		bool bStoreAnimManualSlerp = pObject->bAnimManualSlerp;

		// advance animation frame (only done at SYNC normally)
		// NOTE: This also updates the latest limb bounds for FAST ACCURATE limb detection :)
		if ( pObject->bExcluded==false && pObject->pAnimationSet )
		{
			// conditions moved outside of function to speed up (CPU 'call' is more expensive)
			m_ObjectManager.UpdateAnimationCyclePerObject ( pObject );
		}

		// simple update
		GGMATRIX matrix;
		GGMatrixIdentity ( &matrix );
		UpdateAllFrameData ( pObject, pObject->fAnimFrame );
		UpdateFrame ( pObject->pFrame, &matrix );

		// restore animation states
		pObject->bAnimPlaying = bStoreAnimPlaying;
		pObject->fAnimFrame = bStoreAnimFrame;
		pObject->bAnimLooping = bStoreAnimLooping;
		pObject->fAnimSlerpTime = bStoreAnimSlerpTime;
		pObject->bAnimManualSlerp = bStoreAnimManualSlerp;
	}
	else
	{
		// full bounds box/sphere calculation
		CalculateAllBounds ( g_ObjectList [ iID ], false );
	}
}

void CalculateObjectFrameBounds (int iID, float fFrame)
{
	// check the object limb exists
	if (!ConfirmObject (iID))
		return;

	// object ptr
	sObject* pObject = GetObjectData(iID);

	// first calculate frames from this animation
	float bStoreFrame = pObject->fAnimFrame;
	pObject->fAnimFrame = fFrame;
	CalculateObjectBoundsEx(iID, 1);
	pObject->fAnimFrame = bStoreFrame;

	// and finally write final bounds into object global bounds
	GGVECTOR3 vecMin = GGVECTOR3 (1000000.0f, 1000000.0f, 1000000.0f);
	GGVECTOR3 vecMax = GGVECTOR3 (-1000000.0f, -1000000.0f, -1000000.0f);

	// now calculate all frame bound based on bones transformed (close enough and much faster)
	for (int iFrame = 0; iFrame < pObject->iFrameCount; iFrame++)
	{
		sFrame* pFrame = pObject->ppFrameList[iFrame];
		if (pFrame)
		{
			GGVECTOR3 vecBonePos = GGVECTOR3(0, 0, 0);
			GGVec3TransformCoord(&vecBonePos, &vecBonePos, &pFrame->matCombined);
			if (vecBonePos.x < vecMin.x) vecMin.x = vecBonePos.x;
			if (vecBonePos.y < vecMin.y) vecMin.y = vecBonePos.y;
			if (vecBonePos.z < vecMin.z) vecMin.z = vecBonePos.z;
			if (vecBonePos.x > vecMax.x) vecMax.x = vecBonePos.x;
			if (vecBonePos.y > vecMax.y) vecMax.y = vecBonePos.y;
			if (vecBonePos.z > vecMax.z) vecMax.z = vecBonePos.z;
		}
	}

	// and finally write final bounds into object global bounds
	pObject->collision.vecMin = vecMin;
	pObject->collision.vecMax = vecMax;
	pObject->collision.vecCentre = vecMin + ((vecMax - vecMin) / 2.0f);
}

// lee - 140108 - x10 compat.
DARKSDK void SetObjectMask					( int iID, int iMASK, int iShadowMASK )
{
	// MessageBox ( NULL, "DX10", "", MB_OK );
}

DARKSDK void SetObjectMask					( int iID, int iMASK, int iShadowMASK, int iCubeMapMASK )
{
	// MessageBox ( NULL, "DX10", "", MB_OK );
}

DARKSDK void SetObjectMask ( int iID, int iMASK, int iShadowMASK, int iCubeMapMASK, int iForeColorWipe )
{
	// check the object exists
	// leefix - 211006 - allow instanced objects to be masked
	if ( !ConfirmObjectInstance ( iID ) )
		return;

	// apply setting to all meshes
	sObject* pObject = g_ObjectList [ iID ];

	// set regular mask, then the following
	SetObjectMask ( iID, iMASK );

	// set fore color wipe to all meshes in object
	for ( int iMesh = 0; iMesh < pObject->iMeshCount; iMesh++ )
	{
		sMesh* pMesh = pObject->ppMeshList [ iMesh ];
		if ( pMesh )
			if ( pMesh->pDrawBuffer )
				pMesh->pDrawBuffer->dwImmuneToForeColorWipe = (DWORD)iForeColorWipe;
	}

	// 210214 - the mask flag can remove object from sorted list (effectively removing it from all engine render considerations)
	m_ObjectManager.UpdateTextures();
}


DARKSDK void SetArrayMap				( int iID, int iStage, int i1, int i2, int i3, int i4, int i5, int i6, int i7, int i8, int i9, int i10 )
{
	// MessageBox ( NULL, "DX10", "", MB_OK );
}

DARKSDK void SetArrayMapEx				( int iID, int iStage, int iSrcObject, int iSrcStage )
{
	// MessageBox ( NULL, "DX10", "", MB_OK );
}

DARKSDK void Instance					( int iDestinationID, int iSourceID, int iInstanceValue )
{
	// MessageBox ( NULL, "DX10", "", MB_OK );
}

DARKSDK void SetNodeTreeEffect			( int iEffectID )
{
	// MessageBox ( NULL, "DX10", "", MB_OK );
}

DARKSDK void DrawSingle					( int iObjectID, int iCameraID )
{
	// MessageBox ( NULL, "DX10", "", MB_OK );
}

DARKSDK void ResetStaticLights			( void )
{
	// MessageBox ( NULL, "DX10", "", MB_OK );
}

DARKSDK void AddStaticLight				( int iIndex, float fX, float fY, float fZ, float fRange )
{
	// MessageBox ( NULL, "DX10", "", MB_OK );
}

DARKSDK void UpdateStaticLights			( void )
{
	// MessageBox ( NULL, "DX10", "", MB_OK );
}

DARKSDK void SetCharacterCreatorTones ( int iID, int index, float red, float green, float blue, float mix )
{
	// Character creator - Dave - 070515
	if ( index < 0 || index > 3 ) return;

	// check the object exists
	//if ( !ConfirmObjectInstance ( iID ) )
		//return;

	// apply setting to all meshes
	sObject* pObject = g_ObjectList [ iID ];
	if ( !pObject ) return;
	
	if ( !pObject->pCharacterCreator )
	{
		pObject->pCharacterCreator = new sObjectCharacterCreator;
		for ( int c = 0 ; c < 4 ; c++ )
		{
			pObject->pCharacterCreator->ColorTone[c][0] = -1;
			pObject->pCharacterCreator->ColorTone[c][1] = 0;
			pObject->pCharacterCreator->ColorTone[c][2] = 0;
			pObject->pCharacterCreator->ToneMix[c] = 0.0;
		}
	}

	pObject->pCharacterCreator->ColorTone[index][0] = red / 255.0f;
	pObject->pCharacterCreator->ColorTone[index][1] = green / 255.0f;
	pObject->pCharacterCreator->ColorTone[index][2] = blue / 255.0f;
	pObject->pCharacterCreator->ToneMix[index] = mix;
}

DARKSDK void SetLegacyMode ( int iUseLegacy )
{
	if ( iUseLegacy==0 )
		g_bSwitchLegacyOn = false;
	else
		g_bSwitchLegacyOn = true;
}
