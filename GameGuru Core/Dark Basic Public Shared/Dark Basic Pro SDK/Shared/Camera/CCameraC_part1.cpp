DARKSDK void CameraFollow ( int iID, float targetx, float targety, float targetz, float targetangle, float camerarange, float cameraheight, float cameraspeed, int usestaticcollision )
{
	// update internal pointer
	if ( iID < 0 || iID > MAXIMUMVALUE )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANUMBERILLEGAL );
		return;
	}
	if ( !UpdateCameraPtr ( iID ) )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANOTEXIST );
		return;
	}

	// Angle to radians
	targetangle = GGToRadian( targetangle );

	// Create move target (where camera actually goes)
	bool bUntouched=true;
	float movetargetx = targetx - (float)(sin(targetangle)*camerarange);
	float movetargety = targety + cameraheight; // leefix - 200703 - added targety
	float movetargetz = targetz - (float)(cos(targetangle)*camerarange);

	// Calculate distance between camera and move position
	float dx = movetargetx - m_ptr->vecPosition.x;
	float dy = movetargety - m_ptr->vecPosition.y;
	float dz = movetargetz - m_ptr->vecPosition.z;

	// Have camera follow target at speed
	float moveangle = (float)atan2( (double)dx, (double)dz );
	float movedist = (float)sqrt(fabs(dx*dx)+fabs(dz*dz));
	float speed = movedist/cameraspeed;

	float newx, newy, newz;
	newx = m_ptr->vecPosition.x + (float)(sin(moveangle)*speed);
	newy = m_ptr->vecPosition.y + (dy/cameraspeed);
	newz = m_ptr->vecPosition.z + (float)(cos(moveangle)*speed);

	// Calculate look angle
	dx = targetx - newx;
	dz = targetz - newz;
	float lookangle = (float)atan2( (double)dx, (double)dz );

	// Calculate smoothing of camera look angle
	float a = GGToDegree ( lookangle );
	float da = m_ptr->fYRotate;
	float diff = a-da;
	if(diff<-180.0f) diff=(a+360.0f)-da;
	if(diff>180.0f) diff=a-(da+360.0f);
	da=da+(diff/1.5f);

	m_ptr->fYRotate = wrapangleoffset(da);
	m_ptr->vecPosition.x = newx;
	m_ptr->vecPosition.y = newy;
	m_ptr->vecPosition.z = newz;

	// apply changes
	CameraInternalUpdate ( iID );

	if(bUntouched==true)
	{
		lastmovetargetx=movetargetx;
		lastmovetargety=movetargety;
		lastmovetargetz=movetargetz;
	}
}

DARKSDK void CheckRotationConversion ( tagCameraData* m_ptr, bool bUseFreeFlightMode )
{
	// has there been a change?
	if ( bUseFreeFlightMode != m_ptr->bUseFreeFlightRotation )
	{
		// Caluclates equivilant rotation data if switch rotation-modes
		if( bUseFreeFlightMode==true )
		{
			// Euler to Freeflight
			m_ptr->matFreeFlightRotate = m_ptr->matView;
			m_ptr->matFreeFlightRotate._41=0.0f;
			m_ptr->matFreeFlightRotate._42=0.0f;
			m_ptr->matFreeFlightRotate._43=0.0f;
		}
	}

	// always calculate freeflight to euler (for angle feedback)
	if( bUseFreeFlightMode==true )
	{
		// Freeflight to Euler
		GGVECTOR3 vecRotate;
		CameraAnglesFromMatrix ( &m_ptr->matFreeFlightRotate, &vecRotate );
		m_ptr->fXRotate = vecRotate.x;
		m_ptr->fYRotate = vecRotate.y;
		m_ptr->fZRotate = vecRotate.z;
	}

	// new rotation mode
	m_ptr->bUseFreeFlightRotation = bUseFreeFlightMode;
}

DARKSDK void TurnCameraLeft ( int iID, float fAngle )
{
	// turns the camera left

	// update internal pointer
	if ( iID < 0 || iID > MAXIMUMVALUE )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANUMBERILLEGAL );
		return;
	}
	if ( !UpdateCameraPtr ( iID ) )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANOTEXIST );
		return;
	}

	// execute possible conversion
	CheckRotationConversion ( m_ptr, true );

	// rotation adjustment (leefix-210703-big change to freeflightcameracommands)
	GGMATRIX matRotation;
	GGMatrixRotationY ( &matRotation, GGToRadian ( fAngle ) );
	m_ptr->matFreeFlightRotate = m_ptr->matFreeFlightRotate * matRotation;
	m_ptr->bUseFreeFlightRotation=true;

	// apply changes
	CameraInternalUpdate ( iID );
}

DARKSDK void TurnCameraRight ( int iID, float fAngle )
{
	// turns the camera right

	// update internal pointer
	if ( iID < 0 || iID > MAXIMUMVALUE )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANUMBERILLEGAL );
		return;
	}
	if ( !UpdateCameraPtr ( iID ) )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANOTEXIST );
		return;
	}

	// execute possible conversion
	CheckRotationConversion ( m_ptr, true );

	// rotation adjustment
	GGMATRIX matRotation;
	GGMatrixRotationY ( &matRotation, GGToRadian ( -fAngle ) );
	m_ptr->matFreeFlightRotate = m_ptr->matFreeFlightRotate * matRotation;
	m_ptr->bUseFreeFlightRotation=true;

	// apply changes
	CameraInternalUpdate ( iID );
}

DARKSDK void PitchCameraUp ( int iID, float fAngle )
{
	// pitch the camera up

	// update internal pointer
	if ( iID < 0 || iID > MAXIMUMVALUE )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANUMBERILLEGAL );
		return;
	}
	if ( !UpdateCameraPtr ( iID ) )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANOTEXIST );
		return;
	}

	// execute possible conversion
	CheckRotationConversion ( m_ptr, true );

	// rotation adjustment
	GGMATRIX matRotation;
	GGMatrixRotationX ( &matRotation, GGToRadian ( fAngle ) );
	m_ptr->matFreeFlightRotate = m_ptr->matFreeFlightRotate * matRotation;
	m_ptr->bUseFreeFlightRotation=true;

	// apply changes
	CameraInternalUpdate ( iID );
}

DARKSDK void PitchCameraDown ( int iID, float fAngle )
{
	// pitch the camera down

	// update internal pointer
	if ( iID < 0 || iID > MAXIMUMVALUE )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANUMBERILLEGAL );
		return;
	}
	if ( !UpdateCameraPtr ( iID ) )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANOTEXIST );
		return;
	}

	// execute possible conversion
	CheckRotationConversion ( m_ptr, true );

	// rotation adjustment
	GGMATRIX matRotation;
	GGMatrixRotationX ( &matRotation, GGToRadian ( -fAngle ) );
	m_ptr->matFreeFlightRotate = m_ptr->matFreeFlightRotate * matRotation;
	m_ptr->bUseFreeFlightRotation=true;

	// apply changes
	CameraInternalUpdate ( iID );
}

DARKSDK void RollCameraLeft ( int iID, float fAngle )
{
	// roll the camera left

	// update internal pointer
	if ( iID < 0 || iID > MAXIMUMVALUE )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANUMBERILLEGAL );
		return;
	}
	if ( !UpdateCameraPtr ( iID ) )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANOTEXIST );
		return;
	}

	// execute possible conversion
	CheckRotationConversion ( m_ptr, true );

	// rotation adjustment
	GGMATRIX matRotation;
	GGMatrixRotationZ ( &matRotation, GGToRadian ( -fAngle ) );
	m_ptr->matFreeFlightRotate = m_ptr->matFreeFlightRotate * matRotation;
	m_ptr->bUseFreeFlightRotation=true;

	// apply changes
	CameraInternalUpdate ( iID );
}

DARKSDK void RollCameraRight ( int iID, float fAngle )
{
	// roll the camera right

	// update internal pointer
	if ( iID < 0 || iID > MAXIMUMVALUE )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANUMBERILLEGAL );
		return;
	}
	if ( !UpdateCameraPtr ( iID ) )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANOTEXIST );
		return;
	}

	// execute possible conversion
	CheckRotationConversion ( m_ptr, true );

	// rotation adjustment
	GGMATRIX matRotation;
	GGMatrixRotationZ ( &matRotation, GGToRadian ( fAngle ) );
	m_ptr->matFreeFlightRotate = m_ptr->matFreeFlightRotate * matRotation;
	m_ptr->bUseFreeFlightRotation=true;

	// apply changes
	CameraInternalUpdate ( iID );
}

DARKSDK void SetCameraToObjectOrientation ( int iID, int iObjectID )
{
	// Get Camera Ptr
	if ( !UpdateCameraPtr ( iID ) )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANOTEXIST );
		return;
	}

	// Get Object Ptr
	m_Object_Ptr = GetObjectData ( iObjectID );
	if ( m_Object_Ptr==NULL)
		return;

	// Assign Rotation Details to Camera
	m_ptr->vecLook					= m_Object_Ptr->position.vecLook;
	m_ptr->vecUp					= m_Object_Ptr->position.vecUp;
	m_ptr->vecRight					= m_Object_Ptr->position.vecRight;
	m_ptr->fXRotate					= m_Object_Ptr->position.vecRotate.x;
	m_ptr->fYRotate					= m_Object_Ptr->position.vecRotate.y;
	m_ptr->fZRotate					= m_Object_Ptr->position.vecRotate.z;
	m_ptr->bUseFreeFlightRotation	= m_Object_Ptr->position.bFreeFlightRotation;

	FLOAT fDeterminant;
	GGMatrixInverse ( &m_ptr->matFreeFlightRotate, &fDeterminant, &m_Object_Ptr->position.matFreeFlightRotate );

	if ( m_Object_Ptr->position.dwRotationOrder == ROTORDER_XYZ )
		m_ptr->bRotate					= true;
	else
		m_ptr->bRotate					= false;

	// update camera
	CameraInternalUpdate ( iID );
}

//
// New Expression Functions
//

DARKSDK float CameraPositionXEx ( int iID )
{
	if ( iID < 0 || iID > MAXIMUMVALUE )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANUMBERILLEGAL );
		return 0;
	}
	if ( !UpdateCameraPtr ( iID ) )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANOTEXIST );
		return 0;
	}
	return m_ptr->vecPosition.x;
}

DARKSDK float CameraPositionYEx ( int iID )
{
	if ( iID < 0 || iID > MAXIMUMVALUE )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANUMBERILLEGAL );
		return 0;
	}
	if ( !UpdateCameraPtr ( iID ) )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANOTEXIST );
		return 0;
	}
	return m_ptr->vecPosition.y;
}

DARKSDK float CameraPositionZEx ( int iID )
{
	if ( iID < 0 || iID > MAXIMUMVALUE )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANUMBERILLEGAL );
		return 0;
	}
	if ( !UpdateCameraPtr ( iID ) )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANOTEXIST );
		return 0;
	}
	return m_ptr->vecPosition.z;
}

DARKSDK float CameraAngleX ( int iID )
{
	if ( iID < 0 || iID > MAXIMUMVALUE )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANUMBERILLEGAL );
		return 0;
	}
	if ( !UpdateCameraPtr ( iID ) )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANOTEXIST );
		return 0;
	}
	return m_ptr->fXRotate;
}

DARKSDK float CameraAngleY ( int iID )
{
	if ( iID < 0 || iID > MAXIMUMVALUE )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANUMBERILLEGAL );
		return 0;
	}
	if ( !UpdateCameraPtr ( iID ) )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANOTEXIST );
		return 0;
	}
	return m_ptr->fYRotate;
}

DARKSDK float CameraAngleZ ( int iID )
{
	if ( iID < 0 || iID > MAXIMUMVALUE )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANUMBERILLEGAL );
		return 0;
	}
	if ( !UpdateCameraPtr ( iID ) )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANOTEXIST );
		return 0;
	}
	return m_ptr->fZRotate;
}

DARKSDK float GetCameraLookX ( int iID )
{
	if ( iID < 0 || iID > MAXIMUMVALUE )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANUMBERILLEGAL );
		return 0;
	}
	if ( !UpdateCameraPtr ( iID ) )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANOTEXIST );
		return 0;
	}

	// return the value
	return m_ptr->vecLook.x;
}

DARKSDK float GetCameraLookY ( int iID )
{
	if ( iID < 0 || iID > MAXIMUMVALUE )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANUMBERILLEGAL );
		return 0;
	}
	if ( !UpdateCameraPtr ( iID ) )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANOTEXIST );
		return 0;
	}

	// return the value
	return m_ptr->vecLook.y;
}

DARKSDK float GetCameraLookZ ( int iID )
{
	if ( iID < 0 || iID > MAXIMUMVALUE )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANUMBERILLEGAL );
		return 0;
	}
	if ( !UpdateCameraPtr ( iID ) )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANOTEXIST );
		return 0;
	}

	// return the value
	return m_ptr->vecLook.z;
}

DARKSDK void BackdropOn ( int iID ) 
{
	// update the pointer
	if ( iID < 0 || iID > MAXIMUMVALUE )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANUMBERILLEGAL );
		return;
	}
	if ( !UpdateCameraPtr ( iID ) )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANOTEXIST );
		return;
	}

	// set state
	m_ptr->iBackdropState = 1;
}

DARKSDK void BackdropOff ( int iID ) 
{
	// update the pointer
	if ( iID < 0 || iID > MAXIMUMVALUE )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANUMBERILLEGAL );
		return;
	}
	if ( !UpdateCameraPtr ( iID ) )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANOTEXIST );
		return;
	}

	// deactivate backdrop activation
	m_bActivateBackdrop=false;

	// set state
	m_ptr->iBackdropState = 0;

}

DARKSDK void BackdropColor ( int iID, DWORD dwColor, DWORD dwForeColor ) 
{
	// update the pointer
	if ( iID < 0 || iID > MAXIMUMVALUE )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANUMBERILLEGAL );
		return;
	}
	if ( !UpdateCameraPtr ( iID ) )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANOTEXIST );
		return;
	}

	// set color
	m_ptr->dwBackdropColor = dwColor;
	m_ptr->dwForegroundColor = dwForeColor;

	// lee - 310306 - u6rc4 - if camera zero, community colour for sprite backdrop
	if ( g_pGlob && iID==0 ) g_pGlob->dw3DBackColor = dwColor;
}

DARKSDK void BackdropColor ( int iID, DWORD dwColor ) 
{
	// passed to core function
	BackdropColor ( iID, dwColor, 0 );
}

DARKSDK void BackdropTexture ( int iID, int iImage ) 
{
	// obsolete function - prior to 060409
	if ( iID < 0 || iID > MAXIMUMVALUE )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANUMBERILLEGAL );
		return;
	}
	if ( !UpdateCameraPtr ( iID ) )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANOTEXIST );
		return;
	}
	if ( iImage==0 )
	{
		// stop backdrop texture
		m_ptr->iBackdropTextureMode = 0;
		m_ptr->pBackdropTexture = NULL;
	}
	else
	{
		// use backdrop texture
		if(iImage<1 || iImage>MAXIMUMVALUE)
		{
			RunTimeError(RUNTIMEERROR_IMAGEILLEGALNUMBER);
			return;
		}
		if ( !ImageExist ( iImage ))
		{
			RunTimeError(RUNTIMEERROR_IMAGENOTEXIST);
			return;
		}

		// set backdrop texture
		m_ptr->iBackdropTextureMode = 1;
		m_ptr->pBackdropTexture = GetImagePointer ( iImage );
	}
}

DARKSDK void BackdropScroll ( int iID, int iU, int iV )
{
	// obsolete function
	RunTimeError ( RUNTIMEERROR_COMMANDNOWOBSOLETE );
}

DARKSDK void SetCameraMatrix ( int iID, GGMATRIX* pMatrix )
{
	if ( iID < 0 || iID > MAXIMUMVALUE )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANUMBERILLEGAL );
		return;
	}
	if ( !UpdateCameraPtr ( iID ) )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANOTEXIST );
		return;
	}

	m_ptr->bOverride = true;
	m_ptr->matViewOverride = *pMatrix;
	m_ptr->matView = m_ptr->matViewOverride;
	GGMATRIX matCurrentTrans;
	float f;
	GGMatrixInverse ( &matCurrentTrans, &f, &m_ptr->matView );
	m_ptr->vecPosition.x = matCurrentTrans._41;
	m_ptr->vecPosition.y = matCurrentTrans._42;
	m_ptr->vecPosition.z = matCurrentTrans._43;

	// get euler angles from view matrix
	GGVECTOR3 sCamright, sCamup, sCamlook;
	sCamright.x = m_ptr->matView._11;
	sCamright.y = m_ptr->matView._21;
	sCamright.z = m_ptr->matView._31;
	sCamup.x    = m_ptr->matView._12;
	sCamup.y    = m_ptr->matView._22;
	sCamup.z    = m_ptr->matView._32;
	sCamlook.x  = m_ptr->matView._13;
	sCamlook.y  = m_ptr->matView._23;
	sCamlook.z  = m_ptr->matView._33;

	// copy vector values into camera (for frustum culling)
	m_ptr->vecUp = sCamlook;
	m_ptr->vecLook = sCamup;
	m_ptr->vecRight = sCamright;

	// Calculate yaw and pitch and roll
	float lookLengthOnXZ = sqrtf( (sCamlook.z*sCamlook.z) + (sCamlook.x*sCamlook.x) );
	float fPitch = atan2f( sCamlook.y, lookLengthOnXZ );
	float fYaw   = atan2f( sCamlook.x, sCamlook.z );
	float fRoll  = atan2f( sCamup.y, sCamright.y ) - GG_PI/2;
	m_ptr->fXRotate = GGToDegree(-fPitch);
	m_ptr->fYRotate = GGToDegree(fYaw);
	m_ptr->fZRotate = GGToDegree(fRoll);
}

DARKSDK void ReleaseCameraMatrix ( int iID )
{
	if ( iID < 0 || iID > MAXIMUMVALUE )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANUMBERILLEGAL );
		return;
	}
	if ( !UpdateCameraPtr ( iID ) )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANOTEXIST );
		return;
	}

	m_ptr->bOverride = false;
}

GGMATRIX GetCameraViewMatrix ( int iID )
{
	GGMATRIX matReturn;
	GGMatrixIdentity ( &matReturn );
	if ( !UpdateCameraPtr ( iID ) )
		return matReturn;

	return m_ptr->matView;
}

GGMATRIX GetCameraProjectionMatrix ( int iID )
{
	GGMATRIX matReturn;
	GGMatrixIdentity ( &matReturn );
	if ( !UpdateCameraPtr ( iID ) )
		return matReturn;

	return m_ptr->matProjection;
}

void SetCameraProjectionMatrix ( int iID, GGMATRIX* pMatrix )
{
	if ( iID < 0 || iID > MAXIMUMVALUE )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANUMBERILLEGAL );
		return;
	}
	if ( !UpdateCameraPtr ( iID ) )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANOTEXIST );
		return;
	}

	m_ptr->bOverride = true;
	m_ptr->matProjection = *pMatrix;
}

