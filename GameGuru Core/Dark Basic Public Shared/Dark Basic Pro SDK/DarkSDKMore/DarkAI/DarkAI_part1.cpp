int AIGetGridSpace( int container, float x, float z )
{
	if ( CheckAIInit ( )==0 ) return 0;

	Container *pContainer = cWorld.GetContainer ( container );
	
	if ( !pContainer )
	{
		if ( bShowErrors )
		{
			sprintf_s ( errAIStr, "AI Container (%d) Does Not Exist", container );
			MessageBox ( NULL, errAIStr, "AI Error", 0 );
			//exit ( 1 );
		}

		return 0;
	}

	return pContainer->pPathFinder->GridGetPosition( x,z );
}

int AIGetUndesirableGridSpace( int container, float x, float z )
{
	if ( CheckAIInit ( )==0 ) return 0;

	Container *pContainer = cWorld.GetContainer ( container );
	
	if ( !pContainer )
	{
		if ( bShowErrors )
		{
			sprintf_s ( errAIStr, "AI Container (%d) Does Not Exist", container );
			MessageBox ( NULL, errAIStr, "AI Error", 0 );
			//exit ( 1 );
		}

		return 0;
	}

	return pContainer->pPathFinder->GridGetUndesirablePosition( x,z );
}

void AISetBlockingThreshold( int iValue )
{
	if ( CheckAIInit ( )==0 ) return;

	if ( iValue < 1 ) iValue = 1;

	PathFinderAdvanced::iUndesirableThreshold = iValue;
}

void AIAddDynamicBlocker( int id, int container, float x, float z, float radius )
{
	if ( CheckAIInit ( )==0 ) return;

	Container *pContainer = CheckContainer( container );
	if ( !pContainer ) return;

	pContainer->pPathFinder->AddDynamicBlocker( id, x, z, radius );
}

void AIRemoveDynamicBlocker( int id )
{
	if ( CheckAIInit ( )==0 ) return;

	Container *pContainer = cWorld.pContainerList;
	while ( pContainer )
	{
		pContainer->pPathFinder->RemoveDynamicBlocker( id );
		pContainer = pContainer->pNextContainer;
	}
}

void AISaveObstacleData( int iContainer, char* szFilename )
{
	if ( CheckAIInit ( )==0 ) return;

	Container *pContainer = cWorld.GetContainer ( iContainer );
	
	if ( !pContainer )
	{
		if ( bShowErrors )
		{
			sprintf_s ( errAIStr, "AI Container (%d) Does Not Exist", iContainer );
			MessageBox ( NULL, errAIStr, "AI Error", 0 );
			//exit ( 1 );
		}

		return;
	}

#pragma warning (disable:4312)
	pContainer->pPathFinder->SaveObstacleData( szFilename );
#pragma warning (default:4312)
}

void AILoadObstacleData( int iContainer, char* szFilename )
{
	if ( CheckAIInit ( )==0 ) return;
	Container *pContainer = cWorld.GetContainer ( iContainer );
	if ( !pContainer )
	{
		if ( bShowErrors )
		{
			sprintf_s ( errAIStr, "AI Container (%d) Does Not Exist", iContainer );
			MessageBox ( NULL, errAIStr, "AI Error", 0 );
			//exit ( 1 );
		}
		return;
	}

	#pragma warning (disable:4312)
	pContainer->pPathFinder->LoadObstacleData( szFilename );
	#pragma warning (default:4312)
}

int AICountContainerObstacles( int iContainer )
{
	if ( CheckAIInit ( )==0 ) return 0;

	Container *pContainer = cWorld.GetContainer ( iContainer );
	
	if ( !pContainer )
	{
		if ( bShowErrors )
		{
			sprintf_s ( errAIStr, "AI Container (%d) Does Not Exist", iContainer );
			MessageBox ( NULL, errAIStr, "AI Error", 0 );
			//exit ( 1 );
		}

		return -1;
	}

	return pContainer->pPathFinder->CountObstacles( );
}

int AICountContainerEdges( int iContainer )
{
	if ( CheckAIInit ( )==0 ) return 0;

	Container *pContainer = cWorld.GetContainer ( iContainer );
	
	if ( !pContainer )
	{
		if ( bShowErrors )
		{
			sprintf_s ( errAIStr, "AI Container (%d) Does Not Exist", iContainer );
			MessageBox ( NULL, errAIStr, "AI Error", 0 );
			//exit ( 1 );
		}

		return -1;
	}

	return pContainer->pPathFinder->CountEdges( );
}

void AIUpdate ( )
{ 
	if ( CheckAIInit ( )==0 ) return;
	
	cWorld.Update ( );
}

void AIUpdate ( float fUserTime )
{ 
	if ( CheckAIInit ( )==0 ) return;

	cWorld.Update ( fUserTime );
}

int AIGetEntityAvoiding ( int iObjID )
{
	if ( CheckAIInit ( )==0 ) return 0;

	Entity *pEntity = cWorld.GetEntityCopy ( iObjID );
	
	if ( !pEntity ) return 0;

	int iAvoid = (int) pEntity->IsAvoiding ( );
	
	return iAvoid;
}

//DarkSDK function return FLOATS
#ifdef DARKSDK_COMPILE

float AIGetPathPointX ( int iPathID, int iIndex ) 
{ 
	if ( CheckAIInit ( )==0 ) return;

	return cWorld.GetPathPointX ( iPathID, iIndex ); 
}

float AIGetPathPointZ ( int iPathID, int iIndex ) 
{ 
	if ( CheckAIInit ( )==0 ) return;

	return cWorld.GetPathPointZ ( iPathID, iIndex ); 
}

float AIGetEntityX ( int iObjID ) 
{
	if ( CheckAIInit ( )==0 ) return;

	Entity *pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return 0;

	return pEntity->GetX( );
}

float AIGetEntityZ ( int iObjID ) 
{
	if ( CheckAIInit ( )==0 ) return;

	Entity *pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return 0;

	return pEntity->GetZ( );
}

float AIGetEntityAngleY ( int iObjID )
{
	if ( CheckAIInit ( )==0 ) return;

	Entity* pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return 0;

	return pEntity->GetAngleY ( );
}

float AIGetEntityTargetX ( int iObjID )
{
	if ( CheckAIInit ( )==0 ) return;

	Entity *pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return 0;

	return pEntity->GetTargetX( );
}

float AIGetEntityTargetZ ( int iObjID )
{
	if ( CheckAIInit ( )==0 ) return;

	Entity *pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return 0;

	return pEntity->GetTargetZ( );
}

float AIGetEntityMoveX ( int iObjID )
{
	if ( CheckAIInit ( )==0 ) return;

	Entity *pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return 0;

	return pEntity->GetX( ) - pEntity->vecLastPos.x;
}

float AIGetEntityMoveZ ( int iObjID )
{
	if ( CheckAIInit ( )==0 ) return;

	Entity *pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return 0;

	return pEntity->GetZ( ) - pEntity->vecLastPos.z;
}

float AIGetEntityDestinationX ( int iObjID )
{
	if ( CheckAIInit ( )==0 ) return;

	Entity *pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return 0;

	return pEntity->GetDestX( );
}

float AIGetEntityDestinationZ ( int iObjID )
{
	if ( CheckAIInit ( )==0 ) return;

	Entity *pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return 0;

	return pEntity->GetDestZ( );
}


float AIGetPlayerX ( )
{
	if ( CheckAIInit ( )==0 ) return;

	Hero *pHero = cWorld.pTeamController->GetPlayer ( );

	if ( !pHero )
	{
		if ( bShowErrors )
		{
			sprintf_s ( errAIStr, 255, "Player Has Not Been Added To The AI System" );
			MessageBox ( NULL, errAIStr, "AI Error", 0 );
			exit ( 0 );
		}

		return 0;
	}

	return pHero->GetX( );
}

float AIGetPlayerZ ( )
{
	if ( CheckAIInit ( )==0 ) return;

	Hero *pHero = cWorld.pTeamController->GetPlayer ( );

	if ( !pHero )
	{
		if ( bShowErrors )
		{
			sprintf_s ( errAIStr, 255, "Player Has Not Been Added To The AI System" );
			MessageBox ( NULL, errAIStr, "AI Error", 0 );
			exit ( 0 );
		}

		return 0;
	}

	return pHero->GetZ( );
}

char* AIGetEntityState ( int iObjID )
{
	if ( CheckAIInit ( )==0 ) return;

	Entity *pEntity = cWorld.GetEntity ( iObjID );
	
	if ( !pEntity )	return "Error! Entity Does Not Exist";

	return pEntity->GetStateName( );
}

char* AIGetEntityAction ( int iObjID )
{
	if ( CheckAIInit ( )==0 ) return;

	Entity *pEntity = cWorld.GetEntity ( iObjID );
	
	if ( !pEntity ) return "Error! Entity Does Not Exist";

	bool bAttack = false;
	
	char* szNewString = new char[256];
	strcpy_s( szNewString, 255, pEntity->GetStateName ( ) );

	if ( strcmp( szNewString, "Attack" )
	  || strcmp( szNewString, "Run and Attack" )
	  || strcmp( szNewString, "Strafe and Attack" ) )
	{
		bAttack = true;
	}

	float fMoveX = pEntity->GetX( ) - pEntity->vecLastPos.x;
	float fMoveZ = pEntity->GetZ( ) - pEntity->vecLastPos.z;
	float fMoveAng;

	if ( fabs ( fMoveX ) + fabs ( fMoveZ ) < 0.00001f ) 
	{
		strcpy_s( szNewString, 255, "Stopped" );
	}
	else
	{
		fMoveAng = fMoveZ / sqrt ( fMoveX*fMoveX + fMoveZ*fMoveZ );
		fMoveAng = acos ( fMoveAng ) * RADTODEG;
		if ( fMoveX < 0.0f ) fMoveAng = 360.0f - fMoveAng;

		float fAngY = pEntity->GetAngleY( );
		float fDiffAng = fMoveAng - fAngY;
		if ( fDiffAng > 180.0f ) fDiffAng = fDiffAng - 360.0f;
		if ( fDiffAng < -180.0f ) fDiffAng = fDiffAng + 360.0f;
		
		if ( fDiffAng < 45 && fDiffAng > -45 ) strcpy_s( szNewString, 255, "Moving Forward" );
		else if ( fDiffAng < 135 && fDiffAng > 0 ) strcpy_s( szNewString, 255, "Strafing Left" );
		else if ( fDiffAng > -135 && fDiffAng < 0 ) strcpy_s( szNewString, 255, "Strafing Right" );
		else strcpy_s( szNewString, 255, "Moving Backwards" );
	}

	if ( bAttack )
	{
		strcat_s( szNewString, 255, " And Attacking" );
	}
	
	return szNewString;
}

#else

//DarkBasic functions return DWORDS
float AIGetPathPointX ( int iPathID, int iIndex )
{
	if ( CheckAIInit ( )==0 ) return 0;

	float fX = cWorld.GetPathPointX ( iPathID, iIndex );

	return fX;
}

float AIGetPathPointZ ( int iPathID, int iIndex )
{
	if ( CheckAIInit ( )==0 ) return 0;

	float fZ = cWorld.GetPathPointZ ( iPathID, iIndex );

	return fZ;
}

float AIGetEntityX ( int iObjID )
{
	if ( CheckAIInit ( )==0 ) return 0;

	Entity *pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return 0;

	float fValue = pEntity->GetX( );
	return fValue;
}

float AIGetEntityY ( int iObjID )
{
	if ( CheckAIInit ( )==0 ) return 0;

	Entity *pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return 0;

	float fValue = pEntity->GetY( );
	return fValue;
}

float AIGetEntityZ ( int iObjID )
{
	if ( CheckAIInit ( )==0 ) return 0;

	Entity *pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return 0;

	float fValue = pEntity->GetZ( );
	return fValue;
}

float AIGetEntityAngleY ( int iObjID )
{
	if ( CheckAIInit ( )==0 ) return 0;

	Entity* pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return 0;

	float fValue = pEntity->GetAngleY ( );
	return fValue;
}

float AIGetEntityTargetX ( int iObjID )
{
	if ( CheckAIInit ( )==0 ) return 0;

	Entity *pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return 0;

	float fValue = pEntity->GetTargetX( );
	return fValue;
}

float AIGetEntityTargetZ ( int iObjID )
{
	if ( CheckAIInit ( )==0 ) return 0;

	Entity *pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return 0;

	float fValue = pEntity->GetTargetZ( );
	return fValue;
}

float AIGetEntityMoveX ( int iObjID )
{
	if ( CheckAIInit ( )==0 ) return 0;

	Entity *pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return 0;

	float fValue = pEntity->GetX( ) - pEntity->GetLastPos( ENT_X );
	return fValue;
}

float AIGetEntityMoveZ ( int iObjID )
{
	if ( CheckAIInit ( )==0 ) return 0;

	Entity *pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return 0;

	float fValue = pEntity->GetZ( ) - pEntity->GetLastPos( ENT_Z );
	return fValue;
}

float AIGetEntityDestinationX ( int iObjID )
{
	if ( CheckAIInit ( )==0 ) return 0;

	Entity *pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return 0;

	float fValue = pEntity->GetDestX( );

	return fValue;
}

float AIGetEntityDestinationZ ( int iObjID )
{
	if ( CheckAIInit ( )==0 ) return 0;

	Entity *pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return 0;

	float fValue = pEntity->GetDestZ( );

	return fValue;
}

float AIGetPlayerX ( )
{
	if ( CheckAIInit ( )==0 ) return 0;

	Hero *pHero = cWorld.pTeamController->GetPlayer ( );

	if ( !pHero )
	{
		if ( bShowErrors )
		{
			sprintf_s ( errAIStr, 255, "Player Has Not Been Added To The AI System" );
			MessageBox ( NULL, errAIStr, "AI Error", 0 );
			exit ( 0 );
		}

		return 0;
	}

	float fX = pHero->GetX( );
	return fX;
}

float AIGetPlayerZ ( )
{
	if ( CheckAIInit ( )==0 ) return 0;

	Hero *pHero = cWorld.pTeamController->GetPlayer ( );

	if ( !pHero )
	{
		if ( bShowErrors )
		{
			sprintf_s ( errAIStr, 255, "Player Has Not Been Added To The AI System" );
			MessageBox ( NULL, errAIStr, "AI Error", 0 );
			exit ( 0 );
		}

		return 0;
	}

	float fZ = pHero->GetZ( );
	return fZ;
}

LPSTR AIGetEntityState ( int iObjID )
{
	if ( CheckAIInit ( )==0 ) return 0;

#pragma warning ( disable : 4312 )	//convert DWORD to 'char *'
		
	Entity *pEntity = cWorld.GetEntityCopy ( iObjID );
	
	if ( !pEntity )
	{
		LPSTR szReturnString = NULL;
		g_pGlob->CreateDeleteString ( (char**) &szReturnString, 30 );
		strcpy_s ( (char*) szReturnString, 30, "Error! Entity Does Not Exist" );
		return szReturnString;
	}
	
	char* szNewString = pEntity->GetStateName ( );
	
	DWORD dwSize = (DWORD) strlen ( (char*) szNewString );

	LPSTR szReturnString = NULL;
	g_pGlob->CreateDeleteString ( (char**) &szReturnString, dwSize+1 );
	
	strcpy_s ( (char*) szReturnString, dwSize+1, szNewString);

	//delete [] szNewString;

#pragma warning ( default : 4312 )
	
	return szReturnString;
}

LPSTR AIGetEntityAction ( int iObjID )
{
	if ( CheckAIInit ( )==0 ) return 0;

#pragma warning ( disable : 4312 )	//convert DWORD to 'char *'
		
	Entity *pEntity = cWorld.GetEntityCopy ( iObjID );
	
	if ( !pEntity )
	{
		LPSTR szReturnString = NULL;
		strcpy_s ( (char*) szReturnString, 30, "Error! Entity Does Not Exist" );
		return szReturnString;
	}

	bool bAttack = false;
	
	char* szNewString = new char[256];
	strcpy_s( szNewString, 255, pEntity->GetStateName ( ) );

	if ( !strcmp( szNewString, "Attack" )
	  || !strcmp( szNewString, "Run and Attack" )
	  || !strcmp( szNewString, "Strafe and Attack" ) )
	{
		bAttack = true;
	}

	float fMoveX = pEntity->GetX( ) - pEntity->GetLastPos( ENT_X );
	float fMoveZ = pEntity->GetZ( ) - pEntity->GetLastPos( ENT_Z );
	float fMoveAng;

	if ( fabs ( fMoveX ) + fabs ( fMoveZ ) < 0.00001f ) 
	{
		strcpy_s( szNewString, 255, "Stopped" );
	}
	else
	{
		fMoveAng = fMoveZ / sqrt ( fMoveX*fMoveX + fMoveZ*fMoveZ );
		fMoveAng = acos ( fMoveAng ) * RADTODEG;
		if ( fMoveX < 0.0f ) fMoveAng = 360.0f - fMoveAng;

		float fAngY = pEntity->GetAngleY( );
		float fDiffAng = fMoveAng - fAngY;
		if ( fDiffAng > 180.0f ) fDiffAng = fDiffAng - 360.0f;
		if ( fDiffAng < -180.0f ) fDiffAng = fDiffAng + 360.0f;
		
		if ( fDiffAng < 45 && fDiffAng > -45 ) strcpy_s( szNewString, 255, "Moving Forwards" );
		else if ( fDiffAng < 135 && fDiffAng > 0 ) strcpy_s( szNewString, 255, "Strafing Right" );
		else if ( fDiffAng > -135 && fDiffAng < 0 ) strcpy_s( szNewString, 255, "Strafing Left" );
		else strcpy_s( szNewString, 255, "Moving Backwards" );
	}

	if ( bAttack )
	{
		strcat_s( szNewString, 255, " And Attacking" );
	}
	
	DWORD dwSize = (DWORD) strlen ( (char*) szNewString );

	LPSTR szReturnString = NULL;
	g_pGlob->CreateDeleteString ( (char**) &szReturnString, dwSize+1 );
	
	strcpy_s ( (char*) szReturnString, dwSize+1, szNewString);
	
#pragma warning ( default : 4312 )
	
	return szReturnString;
}

#endif
//End of DarkSDK/DarkBasic differences.

int AIGetEntityTargetID ( int iObjID, int iIndex )
{
	if ( CheckAIInit ( )==0 ) return 0;

	Entity *pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return 0;

	return pEntity->GetTargetID ( iIndex );
}

int AIGetEntityTargetContainer ( int iObjID, int iIndex )
{
	if ( CheckAIInit ( )==0 ) return 0;

	Entity *pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return -1;

	int iTarget = pEntity->GetTargetID ( iIndex );

	Entity *pTarget = cWorld.GetEntityCopy ( iTarget );
	if ( !pTarget ) 
	{
		Hero *pTargetHero = cWorld.GetHero( iTarget );
		if ( !pTargetHero ) return -1;
		else
		{
			Container *pTargetContainer = pTargetHero->GetContainer( );
			if ( !pTargetContainer ) return -1;

			return pTargetContainer->GetID( );
		}
	}
	else
	{	
		Container *pTargetContainer = pTarget->GetContainer( );
		if ( !pTargetContainer ) return -1;

		return pTargetContainer->GetID( );
	}
}

int AIGetEntityTeam ( int iObjID )
{
	if ( CheckAIInit ( )==0 ) return 0;

	Entity *pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return 0;

	if ( pEntity->IsNeutral( ) ) return 0;
	return pEntity->IsFriendly( ) ? 1 : 2;
}

int AIGetEntityHeardSound ( int iObjID )
{
	if ( CheckAIInit ( )==0 ) return 0;
	Entity *pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return 0;
	return pEntity->HeardSound( ) ? 1 : 0;
}

int AIGetEntityCanSee ( int iObjID, float x, float z, int ground )
{
	/*
	if ( CheckAIInit ( )==0 ) return;

	Entity *pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return 0;

	return pEntity->CanSee ( x, pEntity->GetY( ), z, ground == 0 );
	*/
	MessageBox( NULL, "The command \"AI Get Entity Can See <id>,<x>,<z>,<ground>\" has been removed, use \"AI Get Entity Can See <id>,<x>,<y>,<z>,<ground>\" instead", "AI Error", 0 );
	exit(-1);
}

int AIGetEntityCanSee ( int iObjID, float x, float y, float z, int ground )
{
	if ( CheckAIInit ( )==0 ) return 0;

	Entity *pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return 0;

	return pEntity->CanSee ( x, y, z, ground == 0 );
}

float AIRayCast ( float x, float y, float z, float x2, float y2, float z2 )
{
	if ( CheckAIInit ( )==0 ) return 0;

	float dist = 0;
	bool bHit = cWorld.GlobalVisibilityCheck( x,y,z, x2,y2,z2, &dist );

	return dist;
}

int AIGetEntityStance ( int iObjID )
{
	if ( CheckAIInit ( )==0 ) return 0;

	Entity *pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return 0;

	return pEntity->GetAggressiveness();
}

void AISetEntityNoLookAtPoint ( int iObjID )
{
	if ( CheckAIInit ( )==0 ) return;

	Entity *pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return;

	pEntity->SetNoLookPoint ( );
}

void AIEntityMoveToClosestSound ( int iObjID )
{
	if ( CheckAIInit ( )==0 ) return;

	Entity *pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return;

	if ( pEntity->HeardSound( ) ) pEntity->MoveToInterest ( );
}

void AIEntityMoveAwayFromSound ( int iObjID )
{
	if ( CheckAIInit ( )==0 ) return;

	Entity *pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return;

	if ( pEntity->HeardSound( ) ) pEntity->MoveAwayFromSound ( );
}

int AIGetEntityIsMoving ( int iObjID )
{
	if ( CheckAIInit ( )==0 ) return 0;

	Entity *pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return 0;

	///if ( pEntity->bRedoPath ) return 1; // chganged beahiour of the moving flag

	return pEntity->IsMoving ( ) ? 1 : 0;
}

int AIGetEntityIsTurning ( int iObjID )
{
	if ( CheckAIInit ( )==0 ) return 0;

	Entity *pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return 0;

	return pEntity->IsTurning ( ) ? 1 : 0;
}

void AISetEntityIdlePosition ( int iObjID, float x, float z, int container )
{
	if ( CheckAIInit ( )==0 ) return;

	Entity *pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return;

	pEntity->SetIdlePos ( x, pEntity->GetY( ), z, container );
}

void AISetEntityIdlePosition ( int iObjID, float x, float z )
{
	//AISetEntityIdlePosition( iObjID, x, z, -1 );
	MessageBox( NULL, "\"AI Set Entity Idle Position <entity>,<x>,<z>\" Now requires a container ID", "AI Error", 0 );
	exit( 1 );
}

void AIEntityMoveToIdlePosition ( int iObjID )
{
	if ( CheckAIInit ( )==0 ) return;

	Entity *pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return;

	pEntity->MoveToIdlePos ( );
}

void AISetEntityCollide ( int iObjID, int iObjHit )
{
	if ( CheckAIInit ( )==0 ) return;

	Entity* pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return;

	pEntity->SetHitEntity( NULL );
	if ( iObjHit > 0 ) 
	{
		Entity *pOtherEntity = cWorld.GetEntityCopy ( iObjHit );
		if ( pOtherEntity ) 
		{
			pEntity->SetHitEntity( pOtherEntity );
			pOtherEntity->SetHitByEntity( pEntity );
			pOtherEntity->SetHitBySomething( true );
		}
	}

	pEntity->SetHitSomething( true );
}

void AICreateSound ( float x, float y, float z, int iType, float fSize, int iContainerID )
{
	if ( CheckAIInit ( )==0 ) return;

	Container *pContainer = 0;

	if ( iContainerID >= 0 )
	{
		pContainer = CheckContainer ( iContainerID );
		if ( !pContainer ) return;
	}

	//char str [ 256 ];
	//sprintf_s( str, 256, "High Priority Sound Created: %d, Container: %d", iType, iContainerID );
	//if ( iType >= 10 ) MessageBox( NULL, str, "Info", 0 );

	Beacon *pBeacon = new Beacon ( 0.4f );
	pBeacon->SetPosition ( x, y, z );
	pBeacon->SetSound ( iType, fSize, pContainer );

	cWorld.AddBeacon ( pBeacon );
}

void AICreateSound ( float x, float y, float z, int iType, float fSize )
{
	AICreateSound ( x, y, z, iType, fSize, -1 );
}

void AICreateSound ( float x, float z, int iType, float fSize, int iContainerID )
{
	AICreateSound ( x, iContainerID*100.0f + 20.0f, z, iType, fSize, iContainerID );
	//MessageBox( NULL, "'AI CREATE SOUND <x>,<z>,<type>,<size>,<container>' now requires a y value", "AI Error", 0 );
	//exit(-1);
}

void AICreateSound ( float x, float z, int iType, float fSize )
{
	MessageBox( NULL, "'AI CREATE SOUND <x>,<z>,<type>,<size>' now requires a y value", "AI Error", 0 );
	exit(-1);
}

void AISetEntityControl ( int iObjID, int iMode )
{
	if ( CheckAIInit ( )==0 ) return;

	Entity *pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return ;

	pEntity->SetManualControl( true );
	pEntity->ChangeState ( cWorld.pEntityStates->pStateManual );
}

void AISetEntityMoveBoostPriority ( int iObjID )
{
	if ( CheckAIInit ( )==0 ) return;
	Entity *pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return;
	// does nothing yet
}

void AIEntityGoToPosition ( int iObjID, float x, float z, int container )
{
	if ( CheckAIInit ( )==0 ) return;
	Entity *pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return;
	pEntity->SetDestination ( x, pEntity->GetY( ), z, container );
	if ( !pEntity->GetManualControl() )
	{
		pEntity->ChangeState ( cWorld.pEntityStates->pStateGoToDest );
	}
	pEntity->bLegacyForceMove = false;
}

void AIEntityGoToPosition ( int iObjID, float x, float z )
{
	if ( CheckAIInit ( )==0 ) return;
	Entity *pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return;
	AIEntityGoToPosition( iObjID, x, z, -1 );
	pEntity->bLegacyForceMove = true;
}

void AIEntitySearchArea ( int iObjID )
{
	if ( CheckAIInit ( )==0 ) return;

	Entity *pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return;

	pEntity->SearchArea ( );
}

int AIGetEntityCountTargets ( int iObjID )
{
	if ( CheckAIInit ( )==0 ) return 0;

	Entity *pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return 0;

	return pEntity->CountTargets ( );
}

void AIEntityDefendArea ( int iObjID, float x, float z, float dist, int container )
{
	MessageBox( NULL, "\"AI Entity Defend Area <id>,<x>,<z>,<dist>,<container>\" now requires a Y value as well", "AI Error", 0 );
}

void AIEntityDefendArea ( int iObjID, float x, float y, float z, float dist, int container )
{
	if ( CheckAIInit ( )==0 ) return;

	Entity *pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return;

	pEntity->SetDefendPos( x,y,z, ENT_X|ENT_Y|ENT_Z );
	pEntity->SetDefendArea ( dist, container );
}

void AIEntityDefendPoint ( int iObjID, float x, float z )
{
	if ( CheckAIInit ( )==0 ) return;

	Entity *pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return;

	pEntity->SetAggressiveness( 2 );
	if ( !pEntity->GetManualControl() ) pEntity->ChangeState ( cWorld.pEntityStates->pStateIdle );
	pEntity->SetOrigPos( x,0,z, ENT_X|ENT_Z );
	pEntity->SetFinalDest( x,pEntity->GetOrigPos( ENT_Y ),z, ENT_X|ENT_Y|ENT_Z );
}

void AIEntityStop ( int iObjID )
{
	if ( CheckAIInit ( )==0 ) return;

	Entity *pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return;

	pEntity->Stop ( );
}

void AIEntityStopNoMoveAddition ( int iObjID )
{
	if ( CheckAIInit ( )==0 ) return;

	Entity *pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return;

	pEntity->StopNoMoveAddition();
}

void AIEntityLookAtPosition ( int iObjID, float x, float z )
{
	if ( CheckAIInit ( )==0 ) return;

	Entity *pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return;

	pEntity->LookAt ( x, z );
}

void AIEntityLookAround ( int iObjID, float minAng, float maxAng )
{
	if ( CheckAIInit ( )==0 ) return;

	Entity *pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return;

	pEntity->LookAround ( minAng, maxAng );
}

void AIEntityRandomMove ( int iObjID, float min, float max )
{
	if ( CheckAIInit ( )==0 ) return;

	Entity *pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return;

	pEntity->RandomMove ( min, max );
}

void AIEntityMoveClose ( int iObjID, float x, float z, float maxDist )
{
	if ( CheckAIInit ( )==0 ) return;

	Entity *pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return;

	pEntity->MoveClose ( x, pEntity->GetY(), z, maxDist, -1 );
}

int AIEntityMoveToCover ( int iObjID, float x, float z )
{
	if ( CheckAIInit ( )==0 ) return 0;

	Entity *pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return 0;

	/*pEntity->SetInterestPos( x,pEntity->GetY( ),z, ENT_X|ENT_Y|ENT_Z );
	if ( pEntity->MoveToCover () )
		return 1;
	else
		return 0;*/

	// check if entity has already chosen a cover point
	if ( pEntity->GetCoverPoint() )
	{
		for ( int c = 0; c < (int)coverInUseEntity.size() ; c++ )
		{
			if ( coverInUseEntity[c] == pEntity )
			{
				coverInUseEntity.erase( coverInUseEntity.begin()+c );
				coverInUseX.erase( coverInUseX.begin()+c );
				coverInUseZ.erase( coverInUseZ.begin()+c );
				break;
			}
		}

		pEntity->ClearCoverPoint();
	}

	// look for a cover point
	Path cPath;
	pEntity->GetContainer( )->pPathFinder->SearchCoverPoints ( pEntity->GetX(), pEntity->GetZ(), x, z, &cPath );

	int iNumPoints = cPath.CountPoints ( );
	if ( iNumPoints == 0 )
	{
		//no where to go
		return 0;
	}

	pEntity->Stand ( );
	
	//get entity->target vector
	float fDirX = x - pEntity->GetX ( );
	float fDirZ = z - pEntity->GetZ ( );

	float fRange = ( fDirX*fDirX + fDirZ*fDirZ );
	float fTargetDist = sqrt( fRange );

	bool bFound = false;
	int iCount = 0;

	int iIndex = -1;
	float fClosest = 1000000.0f;

	float fDistX, fDistZ, fCoverPointDist;
	bool btFound;

	for ( int i = 0; i < iNumPoints; i++ )
	{

		fDistX = x - cPath.GetPoint ( i ).x;
		fDistZ = z - cPath.GetPoint ( i ).y;

		// dont pick a cover point that is further away
		fCoverPointDist = sqrt( fDistX*fDistX + fDistZ*fDistZ );
		if ( fCoverPointDist > fTargetDist ) continue;

		float fX = cPath.GetPoint ( i ).x - pEntity->GetX ( );
		float fZ = cPath.GetPoint ( i ).y - pEntity->GetZ ( );

		btFound = false;

		if (  (int)pEntity->prevCoverX.size() < iNumPoints )
		{
			for ( int c = 0 ; c < (int)pEntity->prevCoverX.size(); c++ )
			{
				if ( pEntity->prevCoverX[c] == cPath.GetPoint ( i ).x && pEntity->prevCoverZ[c] == cPath.GetPoint ( i ).y && iNumPoints > 1 )
				{
					btFound = true;
					break;
				}
				else
				{

					for ( int t = 0; t < (int)coverInUseEntity.size() ; t++ )
					{
						fDistX = pEntity->GetX ( ) - coverInUseX[t];
						fDistZ = pEntity->GetZ ( ) - coverInUseZ[t];
						fCoverPointDist = sqrt( fDistX*fDistX + fDistZ*fDistZ );

						if ( fCoverPointDist < 200 )
						{
							btFound = true;
							break;
						}

					}

					fDistX = pEntity->GetX ( ) - cPath.GetPoint ( i ).x;
					fDistZ = pEntity->GetZ ( ) - cPath.GetPoint ( i ).y;
					fCoverPointDist = sqrt( fDistX*fDistX + fDistZ*fDistZ );

					if ( fCoverPointDist < 100 )
					{
						btFound = true;
						break;
					}
				}
			}
		}
		else
		{
			pEntity->prevCoverX.clear();
			pEntity->prevCoverZ.clear();
		}

		if ( btFound ) continue;

		float fDist = fX*fX + fZ*fZ;

		// bias distance for entity stance

			// aggressive, favour forward positions
			float fDotP = (fX*fDirX + fZ*fDirZ)/fTargetDist;
			if ( fDotP < 0 ) fDist -= fDotP*2;


		/*if ( pEntity->GetAggressiveness() == 0 )
		{
			// cautious, favour backward positions
			float fDotP = (fX*fDirX + fZ*fDirZ)/fTargetDist;
			if ( fDotP > 0 ) fDist += fDotP*2;
		}*/

		//choose closest
		if ( fDist < fClosest )
		{
			fClosest = fDist;
			iIndex = i;
		}
	}

	if ( iIndex == -1 ) return 0;

	int coverID = cPath.GetPoint ( iIndex ).container;
	sCoverPoint *pCoverPoint = pEntity->GetContainer( )->pPathFinder->GetCoverPoint( coverID );
	pEntity->SetCoverPoint( pCoverPoint );
	coverInUseEntity.push_back(pEntity);
	coverInUseX.push_back(pCoverPoint->fX);
	coverInUseZ.push_back(pCoverPoint->fY);
	//pEntity->SetDestination ( cPath.GetPoint ( iIndex ).x, pEntity->GetY( ), cPath.GetPoint ( iIndex ).y, -1 );
	//pEntity->MoveToCoverPoint();

	pEntity->MoveToCoverPoint();	

	return 1;

}

void AIEntityDuck ( int iObjID )
{
	if ( CheckAIInit ( )==0 ) return;

	Entity *pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return;

	pEntity->Duck ( );
}

void AIEntityStand ( int iObjID )
{
	if ( CheckAIInit ( )==0 ) return;

	Entity *pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return;

	pEntity->Stand ( );
}

void AIEntityStrafeTarget ( int iObjID )
{
	if ( CheckAIInit ( )==0 ) return;

	Entity *pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return;

	pEntity->StrafeTarget ( );
}

void AIMakeMemblockFromWaypoints ( int iMemblockID, int iContainerID )
{
	if ( CheckAIInit ( )==0 ) return;

	Container *pContainer = CheckContainer ( iContainerID );
	if ( !pContainer ) return;
	
	if ( MemblockExist ( iMemblockID ) )
	{
		DeleteMemblock ( iMemblockID );
	}
	
	pContainer->pPathFinder->MakeMemblockFromWaypoints ( iMemblockID );
}

void AIMakeWaypointsFromMemblock ( int iContainerID, int iMemblockID )
{
	if ( CheckAIInit ( )==0 ) return;

	Container *pContainer = cWorld.GetContainer ( iContainerID );

	if ( !pContainer )
	{
		cWorld.AddContainer ( iContainerID );
		pContainer = cWorld.GetContainer ( iContainerID );
	}
	
	if ( !MemblockExist ( iMemblockID ) )
	{
		if ( bShowErrors )
		{
			sprintf_s ( errAIStr, "Memblock (%d) Does Not Exist", iMemblockID );
			MessageBox ( NULL, errAIStr, "AI Error", 0 );
			//exit ( 1 );
		}

		return;
	}

	pContainer->pPathFinder->MakeWaypointsFromMemblock ( iMemblockID );
}

int AICountWaypoints ( int iContainerID )
{
	if ( CheckAIInit ( )==0 ) return 0;

	Container *pContainer = CheckContainer ( iContainerID );
	if ( !pContainer ) return 0;

	return pContainer->pPathFinder->CountWaypoints ( );
}

void AIAddWaypoint ( int iContainerID, float x, float z )
{
	if ( CheckAIInit ( )==0 ) return;

	Container *pContainer = CheckContainer ( iContainerID );
	if ( !pContainer ) return;

	pContainer->pPathFinder->AddWaypoint ( x, z, false );
}

void AIRemoveWaypoint ( int iContainerID, int iIndex )
{
	if ( CheckAIInit ( )==0 ) return;

	Container *pContainer = CheckContainer ( iContainerID );
	if ( !pContainer ) return;

	if ( iIndex < 1 || iIndex > pContainer->pPathFinder->CountWaypoints( ) )
	{
		if ( bShowErrors )
		{
			sprintf_s ( errAIStr, "Waypoint Index (%d) Out Of Range", iIndex );
			MessageBox ( NULL, errAIStr, "AI Error", 0 );
			//exit ( 1 );
		}

		return;
	}

	pContainer->pPathFinder->RemoveWaypoint ( iIndex );
}

void AIUpdateWaypointVisibility ( int iContainerID, float fLimit )
{
	if ( CheckAIInit ( )==0 ) return;

	Container *pContainer = CheckContainer ( iContainerID );
	if ( !pContainer ) return;

	pContainer->pPathFinder->UpdateVisibility ( fLimit );
}

void AIUpdateWaypointVisibility ( int iContainerID )
{
	AIUpdateWaypointVisibility ( iContainerID, -1 );
}

void AIClearWaypoints ( int iContainerID )
{
	if ( CheckAIInit ( )==0 ) return;

	Container *pContainer = CheckContainer ( iContainerID );
	if ( !pContainer ) return;

	pContainer->pPathFinder->ClearWaypoints ( );
}

void AISetWaypointCost ( int iContainerID, int iIndex, float fCost )
{
	if ( CheckAIInit ( )==0 ) return;

	Container *pContainer = CheckContainer ( iContainerID );
	if ( !pContainer ) return;

	pContainer->pPathFinder->SetWaypointCost ( iIndex, fCost );
}

void AIHideErrors ( )
{
	if ( CheckAIInit ( )==0 ) return;

	bShowErrors = false;
}

void AIShowErrors ( )
{
	if ( CheckAIInit ( )==0 ) return;

	bShowErrors = true;
}

void AIDebugShowWaypoints ( int iContainerID, float fHeight )
{
	if ( CheckAIInit ( )==0 ) return;

	Container *pContainer = CheckContainer ( iContainerID );
	if ( !pContainer ) return;

	pContainer->pPathFinder->DebugDrawWaypoints ( fHeight );
	pContainer->pPathFinder->DebugDrawCoverPoints ( fHeight );
}

void AIDebugHideWaypoints ( int iContainerID )
{
	if ( CheckAIInit ( )==0 ) return;

	Container *pContainer = CheckContainer ( iContainerID );
	if ( !pContainer ) return;

	pContainer->pPathFinder->DebugHideWaypoints ( );
	pContainer->pPathFinder->DebugHideCoverPoints ( );
}

void AIDebugShowWaypointEdges ( int iContainerID, float fHeight )
{
	if ( CheckAIInit ( )==0 ) return;

	Container *pContainer = CheckContainer ( iContainerID );
	if ( !pContainer ) return;

	pContainer->pPathFinder->DebugDrawWaypointEdges ( fHeight );
}

void AIDebugHideWaypointEdges ( int iContainerID )
{
	if ( CheckAIInit ( )==0 ) return;

	Container *pContainer = CheckContainer ( iContainerID );
	if ( !pContainer ) return;

	pContainer->pPathFinder->DebugHideWaypointEdges ( );
}

void AIDebugShowHideObstacleBounds ( int iShowMode, int iContainerID, float fHeight )
{
	if ( CheckAIInit ( )==0 ) return;
	if ( iContainerID == -1 )
	{
		// all containers
		Container *pContainer = cWorld.GetContainerList();
		while ( pContainer )
		{
			if ( iShowMode == 1 )
				pContainer->pPathFinder->DebugDrawPolygonBounds ( fHeight );
			else
				pContainer->pPathFinder->DebugHidePolygonBounds ( );
			pContainer = pContainer->pNextContainer;
		}
	}
	else
	{
		// one container
		Container *pContainer = CheckContainer ( iContainerID );
		if ( !pContainer ) return;
		if ( iShowMode == 1 )
			pContainer->pPathFinder->DebugDrawPolygonBounds ( fHeight );
		else
			pContainer->pPathFinder->DebugHidePolygonBounds ( );
	}
}
void AIDebugShowObstacleBounds ( int iContainerID, float fHeight ) { AIDebugShowHideObstacleBounds ( 1, iContainerID, fHeight ); }
void AIDebugHideObstacleBounds ( int iContainerID ) { AIDebugShowHideObstacleBounds ( 0, iContainerID, 0.0f ); }

void AIDebugShowAvoidanceGrid ( int iContainerID, float fHeight )
{
	if ( CheckAIInit ( )==0 ) return;

	Container *pContainer = CheckContainer ( iContainerID );
	if ( !pContainer ) return;

	pContainer->pPathFinder->DebugDrawAvoidanceGrid ( fHeight );
}

void AIDebugHideAvoidanceGrid ( int iContainerID )
{
	if ( CheckAIInit ( )==0 ) return;

	Container *pContainer = CheckContainer ( iContainerID );
	if ( !pContainer ) return;

	pContainer->pPathFinder->DebugHideAvoidanceGrid ( );
}

void AIDebugShowPaths ( float fHeight )
{
	if ( CheckAIInit ( )==0 ) return;

	Entity::bShowPaths = true;
	Path::fDebugObjHeight = fHeight;
}

void AIDebugHidePaths ( )
{
	if ( CheckAIInit ( )==0 ) return;

	Entity::bShowPaths = false;
}

void AIDebugShowAvoidanceAngles ( float fHeight )
{
	if ( CheckAIInit ( )==0 ) return;

	Entity::bShowAvoidanceAngles = true;
	Entity::fDebugObstacleAnglesObjHeight = fHeight;
}

void AIDebugHideAvoidanceAngles ( )
{
	if ( CheckAIInit ( )==0 ) return;

	Entity::bShowAvoidanceAngles = false;
}

void AIDebugShowViewArcs ( float fHeight )
{
	if ( CheckAIInit ( )==0 ) return;

	Entity::bShowViewArcs = true;
	Entity::fDebugViewObjHeight = fHeight;
}

void AIDebugHideViewArcs ( )
{
	if ( CheckAIInit ( )==0 ) return;

	Entity::bShowViewArcs = false;
}

void AIDebugShowSounds ( float fHeight )
{
	if ( CheckAIInit ( )==0 ) return;

	World::bShowBeacons = true;
	Beacon::fObjHeight = fHeight;
}

void AIDebugHideSounds ( )
{
	if ( CheckAIInit ( )==0 ) return;

	World::bShowBeacons = false;
	cWorld.DebugHideBeacons ( );
}

void AISetConsoleOutputOn ( int iObjID )
{
	if ( CheckAIInit ( )==0 ) return;

	cWorld.SetConsoleOn ( iObjID );
}

void AIChangeConsoleObject ( int iObjID )
{
	if ( CheckAIInit ( )==0 ) return;

	cWorld.ChangeConsoleObject ( iObjID );
}

void AISetConsoleOutputOff ( )
{
	if ( CheckAIInit ( )==0 ) return;

	cWorld.SetConsoleOff ( );
}

/*
#ifndef DARKSDK_COMPILE
BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
    return TRUE;
}
#endif
*/

// new for Reloaded

void AISetEntityDiveSpeedMultiplier ( int iObjID, float speed )
{
	if ( CheckAIInit ( )==0 ) return;

	Entity* pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return;

	pEntity->SetDiveMultiplier ( speed );
}

void AISetEntityLeapSpeedMultiplier ( int iObjID, float speed )
{
	if ( CheckAIInit ( )==0 ) return;

	Entity* pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return;

	pEntity->SetLeapMultiplier ( speed );
}

void AISetAvoidanceGridRadius ( float size )
{
	if ( CheckAIInit ( )==0 ) return;

	cWorld.SetAvoidanceRadius ( size );
}

int AIGetEntityIsBehindCorner ( int iObjID )
{
	if ( CheckAIInit ( )==0 ) return 0;

	Entity* pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return 0;

	return pEntity->GetIsBehindCorner();
}

void AISetEntityCanHideBehindCorner ( int iObjID, int mode )
{
	if ( CheckAIInit ( )==0 ) return;

	Entity* pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return;

	pEntity->SetCanHideBehindCorner( mode > 0 );
}

void AISetEntityDiveRange ( int iObjID, float range )
{
	if ( CheckAIInit ( )==0 ) return;

	Entity* pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return;

	pEntity->SetDiveRange( range );
}

int AIGetEntityIsDiving ( int iObjID )
{
	if ( CheckAIInit ( )==0 ) return 0;

	Entity* pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return 0;

	return pEntity->GetIsDiving();
}

void AISetEntityCanDive ( int iObjID, int mode )
{
	if ( CheckAIInit ( )==0 ) return;

	Entity* pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return;

	pEntity->SetCanDive( mode > 0 );
}

void AIEntityForceOutOfCover ( int iObjID )
{
	if ( CheckAIInit ( )==0 ) return;

	Entity* pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return;

	pEntity->ForceOutOfCover( true );
}

void AISetEntityLeapRange ( int iObjID, float range )
{
	if ( CheckAIInit ( )==0 ) return;

	Entity* pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return;

	pEntity->SetLeapRange( range );
}

int AIGetEntityIsLeaping ( int iObjID )
{
	if ( CheckAIInit ( )==0 ) return 0;

	Entity* pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return 0;

	return pEntity->GetIsLeaping( );
}

void AISetEntityCanLeap ( int iObjID, int mode )
{
	if ( CheckAIInit ( )==0 ) return;

	Entity* pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return;

	pEntity->SetCanLeap( mode > 0 );
}

void AIAddDoor ( int iObjID, int container, float x1, float y1, float x2, float y2 )
{
	if ( CheckAIInit ( )==0 ) return;

	Container *pContainer = CheckContainer ( container );
	if ( !pContainer ) return;

	cWorld.AddDoor( iObjID, container, x1,y1, x2,y2 );
}

void AIRemoveDoor ( int iObjID, int container )
{
	if ( CheckAIInit ( )==0 ) return;

	Container *pContainer = CheckContainer ( container );
	if ( !pContainer ) return;

	cWorld.RemoveDoor( iObjID, container );
}
