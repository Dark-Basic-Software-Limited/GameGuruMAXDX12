void AutoCameraCol ( int iCameraID, float fRadius, int iResponse, int iStandGroundMode )
{
	// update internal data
	if ( iCameraID < 0 || iCameraID > MAXIMUMVALUE )
	{
		RunTimeError(RUNTIMEERROR_CAMERANUMBERILLEGAL);
		return;
	}
	if ( GetCameraInternalData ( iCameraID )==NULL )
	{
		RunTimeError(RUNTIMEERROR_CAMERANOTEXIST);
		return;
	}
	else
	{
		return;
	}

	// set global camera collision flag
	g_iCameraCollisionBehaviourMode = iStandGroundMode;

	// Used to hold camera index
	m_iCameraUsingForCollision = iCameraID;

	// Special camera collision object
	memset ( &CameraObject, sizeof(CameraObject), 0);

	// Prepare radius and boundbox size (camera has no native collision type)
	CameraObject.collision.fRadius = fRadius;
	AdjustBoundBoxToCollisionSize ( &CameraObject, fRadius );

	// Elongate camera bound box for human sized area
	CameraObject.collision.vecMin.y *= 3.3f;
	CameraObject.collision.vecMax.y *= 3.3f;

	// Set standard collision defaults
	CameraObject.collision.bActive=true;
	CameraObject.collision.eCollisionType = COLLISION_BOX;
// lee - 150306 - u60b3 - caused camera collision to stick on floor
//	CameraObject.collision.bFixedBoxCheck=false;
	CameraObject.collision.bFixedBoxCheck=true;
	CameraObject.collision.iResponseMode=1+iResponse;
	CameraObject.bVisible=false;

	// Set start position from current camera
	tagCameraData* m_Camera_Ptr = (tagCameraData*)GetCameraInternalData ( m_iCameraUsingForCollision );
	CameraObject.position.vecLastPosition = m_Camera_Ptr->vecPosition;
	CameraObject.position.vecPosition = m_Camera_Ptr->vecPosition;
	fLastCamX = m_Camera_Ptr->vecPosition.x;
	fLastCamY = m_Camera_Ptr->vecPosition.y;
	fLastCamZ = m_Camera_Ptr->vecPosition.z;
}

void AutoCameraCol ( int iCameraID, float fRadius, int iResponse )
{
	// see above
	AutoCameraCol ( iCameraID, fRadius, iResponse, 0 );
}
