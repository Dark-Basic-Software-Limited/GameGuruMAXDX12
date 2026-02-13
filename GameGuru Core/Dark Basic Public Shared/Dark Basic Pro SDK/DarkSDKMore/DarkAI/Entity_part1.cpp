bool Entity::GetIsDefending ( )
{
	return bDefending;
}

void Entity::SetInvestigatePosToHit ( )
{
	if ( CountHitPoints( ) == 0 ) 
	{
		SetNoInvestigatePos( );
		return;
	}
	
	GGVECTOR3 vecDir = GetHitDir ( 0 ) * fRadius*4.0;
	float fResult = pContainer->pPathFinder->FindClosestPolygon ( GetX ( ), GetZ ( ), GetX ( ) + vecDir.x, GetZ ( ) + vecDir.z );

	if ( fResult >= 0.0f )
		vecDir *= fResult;
	
	vecInterestPos.x = GetX ( ) + vecDir.x;
	vecInterestPos.y = GetY ( );
	vecInterestPos.z = GetZ ( ) + vecDir.z;
}

void Entity::SetInvestigatePosToSound ( )
{
	vecInterestPos = vecSoundPos;
}

void Entity::SetNoInvestigatePos ( )
{
	vecInterestPos.x = GetX( );
	vecInterestPos.y = GetY( );
	vecInterestPos.z = GetZ( );
}

void Entity::ZoneEnterEvent ( Entity *pEntityIn )
{
	if ( pEntityIn->pTeam != pTeam && !pEntityIn->IsNeutral( ) ) 
	{
		AddTarget ( pEntityIn, true );
	}
}

void Entity::ZoneEnterEvent ( Hero *pHeroIn )
{
	if ( pHeroIn->pTeam != pTeam ) 
	{
		AddTarget ( pHeroIn, true );
	}
}

void Entity::ZoneLeaveEvent ( Entity *pEntityOut )
{
	RemoveTarget ( pEntityOut );
}

void Entity::ZoneLeaveEvent ( Hero *pHeroOut )
{
	RemoveTarget ( pHeroOut );
}

void Entity::SetFollowing ( bool bFollowing )
{
	bFollowingLeader = bFollowing;
	bDefending = bFollowing;
}

bool Entity::GetFollowing ( )
{
	return bFollowingLeader;
}

float Entity::GetMaxSpeed ( )
{
	return fSpeed;
}

float Entity::GetViewRange ( )
{
	return fViewRange;
}

void Entity::SetStateTimer ( float fNewTime )
{
	fStateTimer = fNewTime;
}

void Entity::SetInvestigateTimer ( float fNewTime )
{
	fInvestigateTimer = fNewTime;
}

void Entity::SetCoverTimer ( float fNewTime )
{
	fCoverTimer = fNewTime;
}

void Entity::SetSearchTimer ( float fNewTime )
{
	fSearchTimer = fNewTime;
}

float Entity::GetStateTimer ( )
{
	return fStateTimer;
}

float Entity::GetInvestigateTimer ( )
{
	return fInvestigateTimer;
}

float Entity::GetCoverTimer ( )
{
	return fCoverTimer;
}

float Entity::GetLookTimer ( )
{
	return fLookAroundTimer;
}

float Entity::GetResetTimer ( )
{
	return fResetTimer;
}

float Entity::GetSearchTimer ( )
{
	return fSearchTimer;
}

void Entity::SetAvgPatrolTime ( float fNewTime )
{
	if ( fNewTime < 0 ) fNewTime = 0;
	fAvgPatrolTime = fNewTime;
}

float Entity::GetAvgPatrolTime ( )
{
	return fAvgPatrolTime;
}

bool Entity::HeardSound ( )
{
	return bHeardSound;
}

int Entity::GetLastSoundUrgency ( )
{
	return iLastSoundUrgency;
}

//returns 0 - can't see, 1 - in peripheral vision, 2 - can see
//bGround defines if the point is low and therefore obstructed by half height objects
int Entity::CanSee ( float fX, float fY, float fZ, bool bGround )
{
	float fThisX = GetX ( );
	float fThisY = GetY ( );
	float fThisZ = GetZ ( );

	if ( bIsBehindCorner )
	{
		fThisX = fPeekX;
		fThisZ = fPeekY;
	}

	int iHeight = 1;
	if ( bIsDucking || bGround ) iHeight = 2;

	bool bHit = false;

	//choose old or new visibility checks
	//if ( pWorld->UsingGlobalVisibility( ) ) bHit = pWorld->GlobalVisibilityCheck( fThisX, fThisY + GetHeight( ), fThisZ, fX, fY, fZ, NULL );
	//else bHit = pContainer->pPathFinder->QuickPolygonsCheckVisible ( fThisX, fThisZ, fX, fZ, iHeight );
	// 090417 - always use new visibility (even if nothing in AI obstacle world - i.e. new level with simple terrain)
	bHit = pWorld->GlobalVisibilityCheck( fThisX, fThisY + GetHeight( ), fThisZ, fX, fY, fZ, NULL );

	int iViewResult = 0;

	if ( !bHit )
	{
		float fDirX = fX - fThisX;
		float fDirY = pWorld->UsingGlobalVisibility( ) ? fY - ( fThisY + GetHeight( ) ) : 0;
		float fDirZ = fZ - fThisZ;
		float fDist = sqrt ( fDirX*fDirX + fDirY*fDirY + fDirZ*fDirZ );

		if ( fDist > fViewRange )
		{
			return 0;
		}
		if ( fDist < fRadius*3 && iViewResult < 1 ) iViewResult = 1;

		fDirX /= fDist;
		fDirY /= fDist;
		fDirZ /= fDist;

		/*if ( pWorld->UsingGlobalVisibility( ) )
		{
			//check vertical view arc (fixed)
			float angx = asin ( fDirY ) * RADTODEG;

			if ( ( -45 < angx && angx < 35 ) && iViewResult < 2 ) iViewResult = 2;
			if ( ( -85 < angx && angx < 85 ) && iViewResult < 1 ) iViewResult = 1;
		}*/

		float angy = acos ( fDirZ ) * RADTODEG;
		if ( fDirX < 0.0f ) angy = 360.0f - angy;
		
		float fDiff = fabs ( GetAngleY( ) - angy );
		if ( fDiff > 180.0f ) fDiff = 360.0f - fDiff;

		if ( bIsBehindCorner ) iViewResult = 1;
		else
		{
			if ( fDiff < fInnerViewArc && iViewResult < 2 ) iViewResult = 2;
			if ( fDiff < fOuterViewArc && iViewResult < 1 ) iViewResult = 1;
		}
	}

		return iViewResult;
}

int Entity::CouldSee( float fX, float fY, float fZ, bool bGround )
{
	float fThisX = GetX ( );
	float fThisZ = GetZ ( );
	int iHeight = 1;
	if ( bIsDucking || bGround ) iHeight = 2;

	//choose old or new visibility checks
	//if ( pWorld->UsingGlobalVisibility( ) ) 
	//{
//		return pWorld->GlobalVisibilityCheck( fThisX, GetY( ) + GetHeight( ), fThisZ, fX, fY, fZ, NULL ) ? 0 : 1;
//	}
//	else 
	{
		return pContainer->pPathFinder->QuickPolygonsCheckVisible ( fThisX, fThisZ, fX, fZ, iHeight ) ? 0 : 1;
	}
}

int Entity::CanSee ( Entity* pOtherEntity )
{
	if ( !pOtherEntity ) return 0;
	if ( !pWorld->UsingGlobalVisibility() && pOtherEntity->GetContainer( ) != pContainer ) return 0;
	
	float fX = pOtherEntity->GetX( );
	float fY = pOtherEntity->GetY( ) + pOtherEntity->GetHeight( );
	float fZ = pOtherEntity->GetZ( );

	// allow entities to see entities hiding behind corners
	if ( pOtherEntity->GetIsBehindCorner() )
	{
		// get the average position
		fX = (pOtherEntity->GetPeekX()+pOtherEntity->GetX()) / 2;
		fY = (pOtherEntity->GetPeekY()+pOtherEntity->GetY()) / 2;
	}

	return this->CanSee ( fX, fY, fZ, pOtherEntity->GetIsDucking( ) );
}

int Entity::CanSee ( Hero* pHero )
{
	if ( !pHero ) return 0;
	if ( !pWorld->UsingGlobalVisibility() && pHero->GetContainer( ) != pContainer ) return 0;

	float fX = pHero->GetX ( );
	float fY = pHero->GetY ( ) + pHero->GetHeight( );
	float fZ = pHero->GetZ ( );

	return this->CanSee ( fX, fY, fZ, pHero->GetIsDucking( ) );
}

bool Entity::CanSeeTarget ( )
{
	if ( CountTargets( ) == 0 ) return false;

	switch ( sTarget_list [ 0 ].iTargetType )
	{
		case 0: return ( CanSee ( sTarget_list [ 0 ].pTargetHero ) > 0 ); break;
		case 1: return ( CanSee ( sTarget_list [ 0 ].pTargetEntity ) > 0 ); break;

		default: return false;
	}
}

void Entity::SetAttackDist( float fNewDist )
{
	if ( fNewDist < 0 ) fNewDist = 0;
	fMinimumDist = fNewDist;
}

void Entity::SetAvoidDist( float fNewDist )
{
	if ( fNewDist < 0 ) fNewDist = 0;
	fAvoidDist = fNewDist;
}

int Entity::GetAvoidMode( )
{
	return iEntityAvoidMode;
}

void Entity::SetAvoidMode( int mode )
{
	iEntityAvoidMode = mode;
}

void Entity::SetAlwaysActive( bool alwaysactiveflag )
{
	bAlwaysActive = alwaysactiveflag;
}

void Entity::SetPathStartCostLimit( float fNewDist )
{
	if ( fNewDist < 0 ) fNewDist = -1.0f;
	fPathStartCostLimit = fNewDist;
}

float Entity::GetPathStartCostLimit ( )
{
	return fPathStartCostLimit;
}

int Entity::IsClose ( float fDist )
{
	if ( fDist < fMinimumDist )
		return 1;

	return 0;
}

int Entity::IsCloseSqr ( float fDist )
{
	if ( fDist < fMinimumDist*fMinimumDist )
		return 1;

	return 0;
}

bool Entity::IsTooClose ( float fDist )
{
	if ( fDist < fAvoidDist )
		return 1;

	return 0;
}

bool Entity::IsTooCloseSqr ( float fDist )
{
	if ( fDist < fAvoidDist*fAvoidDist )
		return 1;

	return 0;
}

bool Entity::IsLookingAtObstacle ( float fDist )
{
	if ( pWorld->UsingGlobalVisibility( ) )
	{
		float fDirX = sin( GetAngleY( )*0.017453f ) * fRadius*10.0f;
		float fDirZ = cos( GetAngleY( )*0.017453f ) * fRadius*10.0f;

		float fX = GetX( );
		float fY = GetY( ) + GetHeight( );
		float fZ = GetZ( );

		float dist = -1;

		bool bHit = pWorld->GlobalVisibilityCheck( fX, fY, fZ, fX + fDirX, fY, fZ + fDirZ, &dist );

		if ( bHit )
		{
			dist = dist * ( fDirX*fDirX + fDirZ*fDirZ );
			if ( dist < fRadius*4 ) return true;	
		}
	}
	else
	{
		float fDirX = sin( GetAngleY( )*0.017453f ) * fRadius*3.0f;
		float fDirZ = cos( GetAngleY( )*0.017453f ) * fRadius*3.0f;

		if ( pContainer->pPathFinder->QuickPolygonsCheckVisible( GetX( ), GetZ( ), GetX( ) + fDirX, GetZ( ) + fDirZ, 1 ) ) return true;
	}

	return false;
}

void Entity::PredictTargetPosition ( float fPredDist )
{
	if ( CountTargets ( ) == 0 ) return;

	float pX = sTarget_list [ 0 ].vecLastPos.x;
	float pY = sTarget_list [ 0 ].vecLastPos.z;
	pContainer->pPathFinder->FindClosestOutsidePoint( &pX, &pY );

	sTarget_list [ 0 ].vecLastPos.x = pX;
	sTarget_list [ 0 ].vecLastPos.z = pY;
	
	GGVECTOR3 vecDir = sTarget_list [ 0 ].vecLastDir;
	//GGVECTOR3 vecDir ( GetTargetX ( ) - vecLastKnownPos.x,
	//					 GetTargetY ( ) - vecLastKnownPos.y, 
	//					 GetTargetZ ( ) - vecLastKnownPos.z );

	float fDist = sqrt ( vecDir.x*vecDir.x + vecDir.z*vecDir.z );

	if ( fDist > 0.0001f )
		vecDir /= fDist;

	sTarget_list [ 0 ].vecGuessPos = sTarget_list [ 0 ].vecLastPos + ( vecDir * fPredDist );

	bool bBlocked = pContainer->pPathFinder->QuickPolygonsCheck ( sTarget_list [ 0 ].vecLastPos.x, sTarget_list [ 0 ].vecLastPos.z, sTarget_list [ 0 ].vecGuessPos.x, sTarget_list [ 0 ].vecGuessPos.z, 2 );

	if ( bBlocked ) sTarget_list [ 0 ].vecGuessPos = sTarget_list [ 0 ].vecLastPos;

	//pEntity->vecLastKnownPos = pEntity->vecFinalDest;
	//pEntity->vecGuessNewPos = pEntity->vecLastKnownPos;
	sTarget_list [ 0 ].vecLastDir = vecDir;
}

void Entity::Stop ( )
{
	SetDestinationForced ( GetX( ), GetY( ), GetZ( ), -1, true );
}

void Entity::StopNoMoveAddition ( )
{
	SetDestinationForced ( GetX( ), GetY( ), GetZ( ), -1, false );
}

void Entity::LookAtDest ( )
{
	vecLookAt = vecFinalDest;
}

void Entity::LookAtTarget ( )
{
	vecLookAt.x = GetTargetX ( );
	vecLookAt.y = GetTargetY ( );
	vecLookAt.z = GetTargetZ ( );

	bLookAtPointSet = true;
}

void Entity::LookAtHideDir ( )
{
	vecLookAt.x = GetX() + fHideDirX;
	vecLookAt.y = GetY ( );
	vecLookAt.z = GetZ() + fHideDirY;

	bLookAtPointSet = true;
}

void Entity::LookAt ( float x, float z )
{
	vecLookAt.x = x;
	vecLookAt.z = z;

	bLookAtPointSet = true;
}

void Entity::MoveToTarget ( )
{
	SetDestination ( GetTargetX( ), GetTargetY( ), GetTargetZ( ), GetTargetContainer( ) );
}

void Entity::MoveToInterest ( )
{
	SetDestination ( vecInterestPos.x, vecInterestPos.y, vecInterestPos.z, iInterestContainer ); 
}

void Entity::MoveToGuessPos ( )
{
	if ( CountTargets( ) == 0 ) return;

	SetDestination ( sTarget_list [ 0 ].vecGuessPos.x, sTarget_list [ 0 ].vecGuessPos.y, sTarget_list [ 0 ].vecGuessPos.z, sTarget_list [ 0 ].iContainer );
}

void Entity::MoveToOriginalPos ( )
{
	SetDestination ( vecOrigPos.x, vecOrigPos.y, vecOrigPos.z, iOriginalContainer ); 
}

void Entity::MoveToDefendPos ( )
{
	SetDestination ( vecDefendPos.x, vecDefendPos.y, vecDefendPos.z, iDefendContainer ); 
}

void Entity::MoveToPatrolPos ( )
{
	CheckPath ( );
	if ( !pPatrolPath ) return;

	int iNumPoints = pPatrolPath->CountPoints( );
	if ( iNumPoints <= 0 ) return;

	if ( iCurrentPatrolPoint < 0 ) iCurrentPatrolPoint = 0;
	if ( iCurrentPatrolPoint >= iNumPoints ) iCurrentPatrolPoint = 0;

	SetDestination ( pPatrolPath->GetPoint ( iCurrentPatrolPoint ).x, GetY( ), pPatrolPath->GetPoint ( iCurrentPatrolPoint ).y, pPatrolPath->GetPoint ( iCurrentPatrolPoint ).container );

	if ( GetSqrDistToDest( ) < 0.1 || ( GetSqrDistToDest( ) < fRadius*fRadius*16.0f && ( !bMakingProgress && pWorld->RandInt( 2 ) == 0 ) ) )
	{
		iCurrentPatrolPoint++;
		//if ( iCurrentPatrolPoint >= iNumPoints ) iCurrentPatrolPoint = 0;
		SetStateTimer ( ( ( (rand()*1.0f) / RAND_MAX ) - 0.5f ) + fAvgPatrolTime );

		if ( iCurrentPatrolPoint >= iNumPoints ) iCurrentPatrolPoint = 0;
		SetDestination ( pPatrolPath->GetPoint ( iCurrentPatrolPoint ).x, GetY( ), pPatrolPath->GetPoint ( iCurrentPatrolPoint ).y, pPatrolPath->GetPoint ( iCurrentPatrolPoint ).container );

		//Dave
		if ( bRedoPath == false )
		{
			int n = 0;
			int iMaxInList = (int)ListOfEntitiesToUpdateMovement.size()-1;
			for (; n <= iMaxInList; n++ )
				if ( ListOfEntitiesToUpdateMovement[n] == this->iID )
					break;
			if ( n > iMaxInList )
				ListOfEntitiesToUpdateMovement.push_back(this->iID);
			bRedoPath = true;
		}
		//Dave fPathTimer = 0;
	}
}

void Entity::MoveAwayFromHit ( )
{
	GGVECTOR3 vecDir = GetHitDirAvg( );
	float fX = GetX( ) - vecDir.x * fRadius * 10.0f;
	float fZ = GetZ( ) - vecDir.z * fRadius * 10.0f;
	int iRandDir = ( rand( ) % 2 )*2 - 1;	//-1 or 1
	fX = fX + vecDir.x * fRadius * 3.0f * iRandDir;
	fZ = fZ - vecDir.z * fRadius * 3.0f * iRandDir;

	pContainer->pPathFinder->FindClosestOutsidePoint ( &fX, &fZ );

	SetDestination ( fX, GetY( ), fZ, -1 );
}

void Entity::MoveAwayFromSound ( )
{
	float fDirX = vecInterestPos.x - GetX( );
	float fDirZ = vecInterestPos.z - GetZ( );
	float fDist = sqrt ( fDirX*fDirX + fDirZ*fDirZ );

	fDirX /= fDist;
	fDirZ /= fDist;

	float fX = GetX( ) - fDirX * fRadius * 10.0f;
	float fZ = GetZ( ) - fDirZ * fRadius * 10.0f;
	int iRandDir = ( rand( ) % 2 )*2 - 1;	//-1 or 1
	fX = fX + fDirZ * fRadius * 3.0f * iRandDir;
	fZ = fZ - fDirX * fRadius * 3.0f * iRandDir;

	pContainer->pPathFinder->FindClosestOutsidePoint ( &fX, &fZ );

	SetDestination ( fX, GetY( ), fZ, -1 );
}

bool Entity::MoveToCover ( )
{
	if ( !pContainer ) return false;

	//bool bCover = pContainer->pPathFinder->QuickPolygonsCheckVisible ( GetX ( ), GetZ ( ), vecInterestPos.x, vecInterestPos.z, 2 );

	bool bCover = CouldSee( vecInterestPos.x, vecInterestPos.y, vecInterestPos.z, 0 ) == 0;

	float fDiffX = vecInterestPos.x - GetX( );
	float fDiffZ = vecInterestPos.z - GetZ( );
	float fDist = fDiffX*fDiffX + fDiffZ*fDiffZ;

	if ( !bCover || fDist < fRadius*fRadius*9 )
	{
		Path cPath;
		// find half height first, if not then full height
		pContainer->pPathFinder->GetClosestWaypoints ( GetX ( ), GetZ ( ), fViewRange / 4.0f, &cPath, true );

		int iNumPoints = cPath.CountPoints ( );
		if ( iNumPoints == 0 )
			pContainer->pPathFinder->GetClosestWaypoints ( GetX ( ), GetZ ( ), fViewRange / 4.0f, &cPath, false );

		iNumPoints = cPath.CountPoints ( );
		if ( iNumPoints == 0 )
		{
			//no where to go
			//MoveAwayFromSound( );
			return false;
		}

		float fClosest = -1;
		int iIndex = 0;
		
		for ( int i = 0; i < iNumPoints; i++ )
		{
			float fX = cPath.GetPoint(i).x;
			float fY = GetY();	//not accurate on non-level containers
			float fZ = cPath.GetPoint(i).y;
			bool bHasCover = false;

			if ( pWorld->UsingGlobalVisibility( ) ) bHasCover = pWorld->GlobalVisibilityCheck( fX, fY, fZ, vecInterestPos.x, vecInterestPos.y, vecInterestPos.z, NULL );
			else bHasCover = pContainer->pPathFinder->QuickPolygonsCheckVisible ( fX, fZ, vecInterestPos.x, vecInterestPos.z, 2 );
						
			float fDiffX2 = cPath.GetPoint(i).x - GetX( );
			float fDiffZ2 = cPath.GetPoint(i).y - GetZ( );
			
			float fRange = fDiffX2*fDiffX2 + fDiffZ2*fDiffZ2;
			float fDotP = fDiffX*fDiffX2 + fDiffZ*fDiffZ2;
			if ( fDotP > 0 ) fRange += fRadius*fRadius*9;
			
			if ( bHasCover && ( fClosest < 0 || fRange < fClosest ) )
			{
				fClosest = fRange;
				iIndex = i;
			}
		}

		SetDestination ( cPath.GetPoint(iIndex).x, GetY(), cPath.GetPoint(iIndex).y, -1 );
		
		return true;
	}
	else
	{
		//if ( !IsTurning() ) LookAt( vecInterestPos.x , vecInterestPos.z );
		return false;
	}
}

void Entity::MoveToIdlePos ( )
{
	SetDestination ( vecOrigPos.x, vecOrigPos.y, vecOrigPos.z, iOriginalContainer );
}

void Entity::MoveTowards ( float x, float z, float dist, int container )
{
	float fDiffX = x - GetX( );
	float fDiffZ = z - GetZ( );
	float fDist = dist / sqrt ( fDiffX*fDiffX + fDiffZ*fDiffZ );

	x = GetX( ) + fDiffX * fDist;
	z = GetZ( ) + fDiffZ * fDist;

	SetDestination ( x, GetY( ), z, container );
}

void Entity::SearchArea ( )
{
	if ( !bManualControl ) ChangeState ( pWorld->pEntityStates->pStateSearchArea );
}

void Entity::LookAround ( float fStart, float fEnd )
{
	if ( !IsTurning ( ) )
	{
		if ( fStart < 0.0f ) fStart = 0.0f;
		if ( fEnd > 180.0f ) fEnd = 180.0f;
		if ( fEnd < fStart ) fEnd = fStart;

		float fRange = fEnd - fStart;
		float fRandNum = ( fRange * rand( ) ) / RAND_MAX + fStart;
		if ( rand( ) % 2 == 0 ) fRandNum = -fRandNum;
		
		float fNewAngY = ( GetAngleY ( ) + fRandNum ) * DEGTORAD;
		
		vecLookAt.x = GetX ( ) + sin ( fNewAngY )*100.0f;
		vecLookAt.z = GetZ ( ) + cos ( fNewAngY )*100.0f;

		bLookAtPointSet = true;
	}
}

void Entity::SetNoLookPoint ( )
{
	bLookAtPointSet = false;
}

void Entity::SweepViewForward ( float fMaxAng )
{	
	if ( !IsTurning ( ) )
	{
		if ( fMaxAng > 180.0f ) fMaxAng = 180.0f;
		if ( fMaxAng < 0.0f ) fMaxAng = 0.0f;

		float x = vecCurrDest.x - GetX ( );
		float z = vecCurrDest.z - GetZ ( );
		float fNewAngY;

		if ( fabs ( x ) + fabs ( z ) < 0.1f ) 
		{
			LookAround ( 0.0f, fMaxAng );
			return;
		}
		else
		{
			fNewAngY = z / sqrt ( x*x + z*z );
			fNewAngY = acos ( fNewAngY ) * RADTODEG;
			if ( x < 0.0f ) fNewAngY = 360.0f - fNewAngY;
		}

		float fRandNum = ( fMaxAng * rand( ) ) / RAND_MAX;
		if ( rand( ) % 2 == 0 ) fRandNum = -fRandNum;
		
		fNewAngY = ( fNewAngY + fRandNum ) * DEGTORAD;
		
		vecLookAt.x = GetX ( ) + sin ( fNewAngY )*100.0f;
		vecLookAt.z = GetZ ( ) + cos ( fNewAngY )*100.0f;

		bLookAtPointSet = true;
	}
}

void Entity::RandomMove ( float fMin, float fMax )
{
	if ( fMin < 0.0f ) fMin = 0.0f;
	//if ( fMax > 100.0f ) fMax = 100.0f;
	if ( fMax < fMin ) fMax = fMin;

	fMax = fMax - fMin;

	float fAng = (6.2831853f * rand ( )) / RAND_MAX;
	float fRange = (fMax * rand ( )) / RAND_MAX + fMin;

	GGVECTOR3 vecPos ( sin( fAng ) * fRange, 0.0f, cos( fAng ) * fRange );
	float fDist = pContainer->pPathFinder->FindClosestPolygon ( GetX( ), GetZ( ), GetX( ) + vecPos.x, GetZ( ) + vecPos.z );

	if ( fDist >= 0.0f ) vecPos *= fDist-0.05f;
	
	SetDestination ( GetX ( ) + vecPos.x, GetY ( ), GetZ ( ) + vecPos.z, -1 );
}

void Entity::MoveClose ( float fDestX, float fDestY, float fDestZ, float fRange, int container )
{
	if ( fRange < 0.0f ) fRange = 0.0f;
	float fMinRange = fRadius*2;
	if ( fMinRange > fRange ) fMinRange = fRange;
	fRange = fRange - fMinRange;
	
	int iIterations = 0;
	
	float fVX, fVZ, fDist, fAng;

	//do
	//{
		fDist = ( fRange * rand( ) ) / RAND_MAX + fMinRange;
		fAng = ( 360.0f * DEGTORAD * rand( ) ) / RAND_MAX;

		fVX = sin( fAng )*fDist;
		fVZ = cos( fAng )*fDist;

		Container *pDestContainer = pContainer;
		if ( container >= 0 ) pDestContainer = pWorld->GetContainer( container );

		float fResult = pDestContainer->pPathFinder->FindClosestPolygon ( fDestX, fDestZ, fDestX + fVX, fDestZ + fVZ );

		//pContainer->pPathFinder->FindClosestOutsidePoint ( &fVX, &fVZ );

	//	iIterations++;
	//} while ( iIterations < 10 && ( fResult >= 0.0f && fResult*fDist < 1.0f ) );
	
	if ( fResult > 0 )
	{
		fResult -= 0.01f;
		fVX *= fResult;
		fVZ *= fResult;
	}

	SetDestination ( fDestX + fVX, fDestY, fDestZ + fVZ, container );
	//SetDestination ( fVX, GetY( ), fVZ );
}

void Entity::Duck ( )
{
	if ( !bCanDuck ) return;
	bIsDucking = true;
}

void Entity::Stand ( )
{
	bIsDucking = false;
}

void Entity::StrafeTarget ( )
{
	if ( !bCanStrafe ) return;
	if ( CountTargets( ) <= 0 ) return;

	GGVECTOR3 vecDir ( GetTargetX( ) - GetX ( ), 
						 GetTargetY( ) - GetY ( ),
						 GetTargetZ( ) - GetZ ( ) );

	float fDist = sqrt ( vecDir.x*vecDir.x + vecDir.z*vecDir.z );
	if ( fDist > 0.00001 ) vecDir /= fDist;
	
	int iRandDir = ( ( rand( ) % 2 ) * 2 ) - 1;				//-1 or 1
	float fRandDist = fRadius * ( ( 4.0f*rand ( ) ) / RAND_MAX + 2.0f );

	float fDirX =  iRandDir * vecDir.z * fRandDist;
	float fDirZ = -iRandDir * vecDir.x * fRandDist;

	float fResult = pContainer->pPathFinder->FindClosestPolygon ( GetX ( ), GetZ ( ), GetX ( ) + fDirX, GetZ ( ) + fDirZ );

	if ( fResult >= 0.01f )
	{
		fResult -= 0.01f;
		fDirX *= fResult;
		fDirZ *= fResult;
	}

	SetDestination ( GetX ( ) + fDirX, GetY ( ), GetZ ( ) + fDirZ, -1 );
}

void Entity::StrafeAvoid ( )
{
	if ( !bCanStrafe ) return;
	
	GGVECTOR3 vecDir ( GetTargetX ( ) - GetX ( ), 
						 GetTargetY ( ) - GetY ( ),
						 GetTargetZ ( ) - GetZ ( ) );

	float fDist = sqrt ( vecDir.x*vecDir.x + vecDir.z*vecDir.z );

	if ( fDist > 0.00001 ) vecDir /= fDist;

	
	float fDirX = GetTargetDirX( );
	float fDirZ = GetTargetDirZ( );
	
	float fDirDist = sqrt ( fDirX*fDirX + fDirZ*fDirZ );
	if ( fDirDist > 0.00001 ) { fDirX /= fDirDist; fDirZ /= fDirDist; }

	float fRel = ( fDirX * vecDir.z ) - ( fDirZ * vecDir.x );

	if ( fRel > 0.0f )
	{
		fDirX = -vecDir.z;
		fDirZ = vecDir.x;
	}
	else
	{
		fDirX = vecDir.z;
		fDirZ = -vecDir.x;
	}
	
	if ( fDirDist > 0.00001 )
	{
		fDirX = fDirX*(fDist) - vecDir.x*(fAvoidDist - fDist + 1.0f);
		fDirZ = fDirZ*(fDist) - vecDir.z*(fAvoidDist - fDist + 1.0f);
	}
	else
	{
		fDirX = -vecDir.x*(fAvoidDist - fDist + 1.0f);
		fDirZ = -vecDir.z*(fAvoidDist - fDist + 1.0f);
	}

	float fResult = pContainer->pPathFinder->FindClosestPolygon ( GetX ( ), GetZ ( ), GetX ( ) + fDirX, GetZ ( ) + fDirZ );

	if ( fResult >= 0.0f )
	{
		fResult -= 0.01f;
		fDirX *= fResult;
		fDirZ *= fResult;
	}

	SetDestination ( GetX ( ) + fDirX, GetY( ), GetZ ( ) + fDirZ, -1 );
}

void Entity::CalculateAvoidPosition ( float fDist )
{
	/*
	if ( cMovePath.CountPoints ( ) > 2 )
	{
		int iIndex = pContainer->pPathFinder->GetClosestWaypoint ( cMovePath.GetPoint ( 1 ).x, cMovePath.GetPoint ( 1 ).y );
		pContainer->pPathFinder->DeActivateWaypoint ( iIndex );

		pContainer->pPathFinder->CalculatePath ( GetX ( ), GetZ ( ), vecFinalDest.x, vecFinalDest.z, &cMovePath );
		
		if ( cMovePath.CountPoints ( ) >= 2 )
		{
			vecAvoidPos.x = cMovePath.GetPoint ( 1 ).x;
			vecAvoidPos.z = cMovePath.GetPoint ( 1 ).y;
			vecAvoidPos.y = GetY ( );
			return;
		}
	}*/
	
	if ( pWorld->RandInt( 4 ) == 0 )
	{
		if ( rand( ) % 2 == 0 ) bAvoidLeft = true;
		else bAvoidLeft = false;
	}

	GGVECTOR3 vecDir;
	vecDir.x = vecCurrDest.x - GetX ( );
	vecDir.y = vecCurrDest.y - GetY ( );
	vecDir.z = vecCurrDest.z - GetZ ( );

	float fLength = vecDir.x*vecDir.x + vecDir.z*vecDir.z;
	
	if ( fLength > 0.001f ) vecDir *= ( fDist / sqrt(fLength) );
	else 
	{
		vecDir.x = sin ( GetAngleY ( ) * DEGTORAD ) * fDist;
		vecDir.z = cos ( GetAngleY ( ) * DEGTORAD ) * fDist;
	}

	float fSwap = vecDir.x;

	if ( bAvoidLeft )
	{
		vecDir.x = -vecDir.z;
		vecDir.z = fSwap;
	}
	else
	{
		vecDir.x = vecDir.z;
		vecDir.z = -fSwap;
	}

	float fResult = pContainer->pPathFinder->FindClosestPolygon ( GetX( ), GetZ( ), GetX( ) + vecDir.x, GetZ( ) + vecDir.z );
	
	if ( fResult >= 0.0f )
	{
		if ( fResult < 0.1f && rand( ) % 3 == 0 ) bAvoidLeft = !bAvoidLeft;
		if ( fResult > 0.01f ) fResult -= 0.01f;
		vecDir *= fResult;
	}

	vecAvoidPos.x = GetX ( ) + vecDir.x;
	vecAvoidPos.z = GetZ ( ) + vecDir.z;
	vecAvoidPos.y = GetY ( );
}

void Entity::TurnToAngle ( float fTimeDelta, bool bMoving )
{
	// work out speed of turn and distance to look at destination
	float fCurrTurnSpeed = fTurnSpeed * fTimeDelta;
	float x = vecCurrLookAt.x - GetX ( );
	float z = vecCurrLookAt.z - GetZ ( );
	float fDist = sqrt ( fabs ( x*x ) + fabs ( z*z ) );
	if ( fDist < 1.0f )
	{
		// leave last angle in tact (would continue along last control path direction)
		return;
	}
	else
	{
		fDestAngY = z / sqrt ( x*x + z*z );
		fDestAngY = acos ( fDestAngY ) * RADTODEG;
		if ( x < 0.0f ) fDestAngY = 360.0f - fDestAngY;
	}
	
	// 210918 - decouple forcing angle to object (so script can smooth it)
	//if ( pObject ) YRotateObject ( iID, fDestAngY );
	//else fAngY = fDestAngY;
	fAngY = fDestAngY;
	/*
	float fDifference = fabs ( fDestAngY - GetAngleY( ) );
	if ( fDifference > 180.0f ) fDifference = 360.0f - fDifference;
	if ( fDifference <= fCurrTurnSpeed || bMoving == false )
	{
		// 080517 - when movement slowed/stopped, use actual object angle for 
		if ( GetVisible ( dwObjectNumberRef ) == 1 )
			fAngY = ObjectAngleY(dwObjectNumberRef);
	}
	else
	{
		// need to report exact angle as this function not called quickly enough for gradual de/increments of angle
		float fDir = sin ( ( fDestAngY - GetAngleY ( ) ) * DEGTORAD );
		if ( fDir > 0.0f )
		{
			if ( pObject ) YRotateObject ( iID, GetAngleY ( ) + fCurrTurnSpeed );
			else fAngY += fCurrTurnSpeed;
		}
		else
		{
			if ( pObject ) YRotateObject ( iID, GetAngleY ( ) - fCurrTurnSpeed );
			else fAngY -= fCurrTurnSpeed;
		}
	}
	*/
	fLookAroundTimer = ( (float) rand( ) ) / RAND_MAX + 1.0f;
}

void Entity::FireWeapon ( )
{
	bFireWeapon = true;
}

void Entity::ResetFeedbackPos ( )
{
	fMoveFeedbackTimer = 0.3f;
	vecLastRecPos.x = GetX ( );
	vecLastRecPos.y = GetY ( );
	vecLastRecPos.z = GetZ ( );
}

void Entity::UpdateFeedbackPos ( )
{
	GGVECTOR3 vecDir, vecDestDir;

	vecDir.x = GetX ( ) - vecLastRecPos.x;
	vecDir.y = GetY ( ) - vecLastRecPos.y;
	vecDir.z = GetZ ( ) - vecLastRecPos.z;

	fDistanceMoved = sqrt ( vecDir.x*vecDir.x + vecDir.z*vecDir.z );
	
	ResetFeedbackPos ( );
	
	if ( cMovePath.CountPoints( ) <= 2 )
	{
		vecDestDir.x = vecFinalDest.x - GetX ( );
		vecDestDir.y = vecFinalDest.y - GetY ( );
		vecDestDir.z = vecFinalDest.z - GetZ ( );
	}
	else
	{
		vecDestDir.x = vecCurrDest.x - GetX ( );
		vecDestDir.y = vecCurrDest.y - GetY ( );
		vecDestDir.z = vecCurrDest.z - GetZ ( );
	}

	float fLength = vecDestDir.x*vecDestDir.x + vecDestDir.z*vecDestDir.z;
	float fDotP = vecDestDir.x*vecDir.x + vecDestDir.z*vecDir.z;

	bStuck = fDistanceMoved < fCurrSpeed / 12.0;
	bMakingProgress = fDotP >= 0.0f && fLength > 0.01f && !bStuck;
}

bool Entity::DirectionBlocked ( float fDirX, float fDirZ )
{
	if ( sCombinedObstacleAngle_list.size( ) < 1 ) return false;
	if ( sCombinedObstacleAngle_list.size( ) == 1 ) 
	{
		if ( sCombinedObstacleAngle_list [ 0 ].fAngEnd - sCombinedObstacleAngle_list [ 0 ].fAngBegin >= 359.9999f ) return true;
	}

	float fDist = fDirX*fDirX + fDirZ*fDirZ;
	if ( fDist < 0.00001f ) return false;
	fDist = sqrt ( fDist );

	float fDirAngle = acos ( fDirZ / fDist ) * RADTODEG;
	if ( fDirX < 0 ) fDirAngle = 360.0f - fDirAngle;

	return DirectionBlocked ( fDirAngle );
}

bool Entity::DirectionBlocked ( float fDir )
{
	if ( sCombinedObstacleAngle_list.size( ) < 1 ) return false;
	if ( sCombinedObstacleAngle_list.size( ) == 1 ) 
	{
		if ( sCombinedObstacleAngle_list [ 0 ].fAngEnd - sCombinedObstacleAngle_list [ 0 ].fAngBegin >= 359.9999f ) return true;
	}
	
	for ( int i = 0; i < (int) sCombinedObstacleAngle_list.size( ); i++ )
	{
		if ( fDir > sCombinedObstacleAngle_list [ i ].fAngBegin && fDir < sCombinedObstacleAngle_list [ i ].fAngEnd ) return true;
		if ( fDir + 360.0f > sCombinedObstacleAngle_list [ i ].fAngBegin && fDir + 360.0f < sCombinedObstacleAngle_list [ i ].fAngEnd ) return true;
	}

	return false;
}

bool Entity::AdjustDirection ( float *pDir )
{
	// quit and ignore avoidance if nothing to avoid
	if ( sCombinedObstacleAngle_list.size( ) < 1 ) return false;

	// if only one obstacle to avoid
	if ( sCombinedObstacleAngle_list.size( ) == 1 ) 
	{
		// but the angles are way out, ignore avoidance 
		if ( sCombinedObstacleAngle_list [ 0 ].fAngEnd - sCombinedObstacleAngle_list [ 0 ].fAngBegin >= 359 ) return false;
	}

	// for bots with multiple obstacles to avoid
	for ( int i = 0; i < (int) sCombinedObstacleAngle_list.size( ); i++ )
	{
		// check if present direction enters an arc that leads to another AI bot
		if ( *pDir > sCombinedObstacleAngle_list [ i ].fAngBegin && *pDir < sCombinedObstacleAngle_list [ i ].fAngEnd ) 
		{
			// work out which side of the AI bot we should aim for to get out of its way
			float fDiff = ( sCombinedObstacleAngle_list [ i ].fAngEnd + sCombinedObstacleAngle_list [ i ].fAngBegin ) / 2.0f;
			if ( *pDir < fDiff ) *pDir = sCombinedObstacleAngle_list [ i ].fAngBegin;
			else *pDir = sCombinedObstacleAngle_list [ i ].fAngEnd;

			// mark as true so this avoidance is carried out
			return true;
		}

		// as above but in case direction angle is a negative (<0)
		if ( *pDir + 360.0f > sCombinedObstacleAngle_list [ i ].fAngBegin && *pDir + 360.0f < sCombinedObstacleAngle_list [ i ].fAngEnd ) 
		{
			// work out which side of the AI bot we should aim for to get out of its way
			float fDiff = ( sCombinedObstacleAngle_list [ i ].fAngEnd + sCombinedObstacleAngle_list [ i ].fAngBegin ) / 2.0f;
			if ( *pDir + 360.0f < fDiff ) *pDir = sCombinedObstacleAngle_list [ i ].fAngBegin;
			else *pDir = sCombinedObstacleAngle_list [ i ].fAngEnd;

			// mark as true so this avoidance is carried out
			return true;
		}
	}

	// no avoidance event today!
	return false;
}

void Entity::AdjustDestination ( )
{
	if ( fAdjustDestinationTimer > 0 ) return;

	pWorld->pTeamController->StartGlobalEntityIterator ( );
	Entity *pOtherEntity = pWorld->pTeamController->GetNextGlobalEntity ( );

	//vecLastDest = vecCurrDest;

	bool bAdjusted = false;

	while ( pOtherEntity )
	{
		if ( pOtherEntity == this || pOtherEntity->GetContainer( ) != pContainer )
		{
			pOtherEntity = pWorld->pTeamController->GetNextGlobalEntity ( );
			continue;
		}

		float fDiffX = vecCurrDest.x - pOtherEntity->GetX( );
		float fDiffZ = vecCurrDest.z - pOtherEntity->GetZ( );
		float fDist = fDiffX*fDiffX + fDiffZ*fDiffZ;
		float fTotalRadius = pOtherEntity->GetRadius( ) + fRadius*1.1f;

		if ( fDist < fTotalRadius*fTotalRadius )
		{
			float fNewDestX;
			float fNewDestZ;

			if ( fDist > 0.0001f )
			{	
				fDist = fTotalRadius / sqrt( fDist );
				fNewDestX = pOtherEntity->GetX( ) + (fDiffX * fDist);
				fNewDestZ = pOtherEntity->GetZ( ) + (fDiffZ * fDist);
			}
			else
			{
				fDiffX = vecCurrDest.x - GetX( );
				fDiffZ = vecCurrDest.z - GetZ( );
				fDist = 1.0f / sqrt ( fDiffX*fDiffX + fDiffZ*fDiffZ );
				fDiffX *= fDist;
				fDiffZ *= fDist;
				
				fNewDestX = pOtherEntity->GetX( ) - (fDiffX * fTotalRadius) - fDiffZ*0.1f;
				fNewDestZ = pOtherEntity->GetZ( ) - (fDiffZ * fTotalRadius) + fDiffX*0.1f;
			}

			vecCurrDest.x = fNewDestX;
			vecCurrDest.z = fNewDestZ;

			bAdjusted = true;
		}

		pOtherEntity = pWorld->pTeamController->GetNextGlobalEntity ( );
	}

	Hero *pHero = pWorld->pTeamController->GetPlayer( );

	if ( pHero && pHero->GetContainer( ) == pContainer )
	{
		float fDiffX = vecCurrDest.x - pHero->GetX( );
		float fDiffZ = vecCurrDest.z - pHero->GetZ( );
		float fDist = fDiffX*fDiffX + fDiffZ*fDiffZ;
		float fTotalRadius = pHero->GetRadius( ) + fRadius*1.1f;

		if ( fDist < fTotalRadius*fTotalRadius )
		{
			float fNewDestX;
			float fNewDestZ;

			if ( fDist > 0.0001f )
			{	
				fDist = fTotalRadius / sqrt( fDist );
				fNewDestX = pHero->GetX( ) + (fDiffX * fDist);
				fNewDestZ = pHero->GetZ( ) + (fDiffZ * fDist);
			}
			else
			{
				fDiffX = vecCurrDest.x - GetX( );
				fDiffZ = vecCurrDest.z - GetZ( );
				fDist = 1.0f / sqrt ( fDiffX*fDiffX + fDiffZ*fDiffZ );
				fDiffX *= fDist;
				fDiffZ *= fDist;
				
				fNewDestX = pHero->GetX( ) - (fDiffX * fTotalRadius) - fDiffZ*0.1f;
				fNewDestZ = pHero->GetZ( ) - (fDiffZ * fTotalRadius) + fDiffX*0.1f;
			}

			vecCurrDest.x = fNewDestX;
			vecCurrDest.z = fNewDestZ;

			bAdjusted = true;
		}
	}

	if ( bAdjusted )
	{
		fAdjustDestinationTimer = 1.5f;
	}
}

void Entity::ClearSounds ( )
{
	bHeardSound = false;
	fClosestSound = -1.0f;
	iSoundUrgency = 0;
}

void Entity::CheckBeacons ( Beacon *pBeacon )
{	
	while ( pBeacon )
	{
		//if ( !pBeacon->GetContainer( ) || pBeacon->GetContainer( ) == pContainer )
		{
			if ( pBeacon->IsSound ( ) && bCanHear )
			{
				if ( pBeacon->iExtraInfo >= iHearingThreshold )
				{
					GGVECTOR3 vecDir;
					vecDir.x = pBeacon->vecPos.x - GetX ( );
					vecDir.y = pBeacon->vecPos.y - GetY ( );
					vecDir.z = pBeacon->vecPos.z - GetZ ( );

					float fDist = vecDir.x*vecDir.x + vecDir.z*vecDir.z;

					float fRange = fHearingRange + pBeacon->fSoundSize;

					if ( fDist < fRange*fRange && fDist > fRadius*fRadius*9 )
					{
						if ( pBeacon->iExtraInfo > iSoundUrgency || ( ( fClosestSound < 0 || fDist < fClosestSound ) && pBeacon->iExtraInfo == iSoundUrgency ) ) 
						{
							fClosestSound = fDist;
							iSoundUrgency = pBeacon->iExtraInfo;
							iLastSoundUrgency = pBeacon->iExtraInfo;
							vecInterestPos = pBeacon->vecPos;
							iInterestContainer = pBeacon->pContainer ? pBeacon->pContainer->GetID() : -1;
							bHeardSound = true;
						}
					}
				}
			}
		}

		pBeacon = pBeacon->pNextBeacon;
	}
}

void Entity::CheckZones ( Zone *pZone )
{
	while ( pZone )
	{
		bool bInNow = pZone->InZone ( GetX( ), GetZ( ), GetContainer( ) );
		bool bInLast = pZone->InZone ( vecLastPos.x, vecLastPos.z, GetContainer( ) );

		if ( bInNow ) pZone->NotifyEnter ( this );
		if ( bInLast && !bInNow ) pZone->NotifyLeave ( this );

		pZone = pZone->pNextZone;
	}
}

void Entity::UpdateObstacleListCountDown ( float fTimeDelta )
{
	// go through all AI bots recorded in OA list and removes old ones
	for ( int i = 0; i < (int) sObstacleAngle_list.size( ); i++ )
	{
		sObstacleAngle_list [ i ].fTimer -= fTimeDelta;
		if ( sObstacleAngle_list [ i ].fTimer <= 0.0f )
		{
			sObstacleAngle_list.erase ( sObstacleAngle_list.begin( ) + i );
			i--;
		}
	}
}

void Entity::UpdateObstacleList ( float fTimeDelta )
{
	// scans all AI bots and works out if they get too close
	// to each other or the player 

	// go through all AI bots
	pWorld->pTeamController->StartGlobalEntityIterator ( );
	Entity *pOtherEntity = pWorld->pTeamController->GetNextGlobalEntity ( );
	while ( pOtherEntity )
	{
		// ignore owner of search and bots in other containers
		if ( pOtherEntity == this || pOtherEntity->GetContainer( ) != pContainer )
		{
			pOtherEntity = pWorld->pTeamController->GetNextGlobalEntity ( );
			continue;
		}

		// work out distance of the two bots
		float fDiffX = pOtherEntity->GetX( ) - GetX( );
		float fDiffZ = pOtherEntity->GetZ( ) - GetZ( );
		float fDist = fDiffX*fDiffX + fDiffZ*fDiffZ;
		float radius = fRadius + pOtherEntity->GetRadius( );
		if ( fDist < (radius*radius) ) // (8*radius*radius) )
		{
			// if close enough, work out angle limits to the other bot
			fDist = sqrt ( fDist );
			float fEntAng = acos ( fDiffZ / fDist ) * RADTODEG;
			if ( fDiffX < 0 ) fEntAng = 360.0f - fEntAng;
			float fSegAng;
			if ( fDist <= fRadius + pOtherEntity->GetRadius( ) ) fSegAng = 70.0f;
			else fSegAng = asin ( ( fRadius + pOtherEntity->GetRadius( ) ) / fDist ) * RADTODEG;
			if ( fSegAng >= 179.99f ) fSegAng = 179.99f;
			float fBeginAngle = fEntAng - fSegAng;
			if ( fBeginAngle < 0 ) fBeginAngle += 360.0f;
			if ( fBeginAngle > 360 ) fBeginAngle -= 360.0f;
			float fEndAngle = fBeginAngle + fSegAng*2.0f;

			// search if the other bot is already in the OA list
			bool bFound = false;
			for ( int i = 0; i < (int) sObstacleAngle_list.size( ) - 1; i++ )
			{
				if ( sObstacleAngle_list [ i ].pEntity == pOtherEntity )
				{
					bFound = true;
					sObstacleAngle_list [ i ].fAngBegin = fBeginAngle;
					sObstacleAngle_list [ i ].fAngEnd = fEndAngle;
					sObstacleAngle_list [ i ].fTimer = 0.4f;
				}
			}
			if ( !bFound && fDist < (2*radius) )
			{
				// if not in OA list and close enough to the other bot
				sObstacleAngle sNewObstacleAngle;
				sNewObstacleAngle.fAngBegin = fBeginAngle;
				sNewObstacleAngle.fAngEnd = fEndAngle;
				sNewObstacleAngle.fTimer = 0.4f;
				sNewObstacleAngle.pEntity = pOtherEntity;
				sObstacleAngle_list.push_back ( sNewObstacleAngle );
			}
		}

		// next AI bot to check against
		pOtherEntity = pWorld->pTeamController->GetNextGlobalEntity ( );
	}

	// check if bot is too close to the main player
	/* Allow AI bots UP CLOSE to player (neded for close-combat melee and physics will push player)
	Hero *pHero = pWorld->pTeamController->GetPlayer( );
	if ( pHero && pHero->GetContainer( ) == pContainer )
	{
		float fDiffX = pHero->GetX( ) - GetX( );
		float fDiffZ = pHero->GetZ( ) - GetZ( );
		float fDist = fDiffX*fDiffX + fDiffZ*fDiffZ;
		float radius = fRadius + pHero->GetRadius( );
		if ( fDist > 0.000001f && fDist < (8*radius*radius) )
		{
			// if close enough, work out angle limits to the player
			fDist = sqrt ( fDist );
			float fEntAng = acos ( fDiffZ / fDist ) * RADTODEG;
			if ( fDiffX < 0 ) fEntAng = 360.0f - fEntAng;
			float fSegAng;
			if ( fDist <= fRadius + pHero->GetRadius( ) ) fSegAng = 70.0f;
			else fSegAng = asin ( ( fRadius + pHero->GetRadius( ) ) / fDist ) * RADTODEG;
			if ( fSegAng >= 179.99f ) fSegAng = 179.99f;
			float fBeginAngle = fEntAng - fSegAng;
			if ( fBeginAngle < 0 ) fBeginAngle += 360.0f;
			if ( fBeginAngle > 360 ) fBeginAngle -= 360.0f;
			float fEndAngle = fBeginAngle + fSegAng*2.0f;

			// if not in OA list and close enough to the other bot
			bool bFound = false;
			for ( int i = 0; i < (int) sObstacleAngle_list.size( ) - 1; i++ )
			{
				if ( sObstacleAngle_list [ i ].pEntity == NULL )
				{
					bFound = true;
					sObstacleAngle_list [ i ].fAngBegin = fBeginAngle;
					sObstacleAngle_list [ i ].fAngEnd = fEndAngle;
					sObstacleAngle_list [ i ].fTimer = 0.5f;
				}
			}
			if ( !bFound && fDist < (2*radius) )
			{
				sObstacleAngle sNewObstacleAngle;
				sNewObstacleAngle.fAngBegin = fBeginAngle;
				sNewObstacleAngle.fAngEnd = fEndAngle;
				sNewObstacleAngle.fTimer = 0.5f;
				sNewObstacleAngle.pEntity = NULL;
				sObstacleAngle_list.push_back ( sNewObstacleAngle );
			}
		}
	}
	*/

	// if we have a container to respect
	if ( pContainer )
	{
		// get any blockers stored for container
		const Blocker *pBlocker = pContainer->pPathFinder->GetBlockerList( );
		while ( pBlocker )
		{
			float fDiffX = pBlocker->x - GetX( );
			float fDiffZ = pBlocker->z - GetZ( );
			float fDist = fDiffX*fDiffX + fDiffZ*fDiffZ;
			float radius = fRadius + pBlocker->radius;
			if ( fDist > 0.000001f && fDist < (8*radius*radius) )
			{
				// if close enough, work out angle limits to the blocker
				fDist = sqrt ( fDist );
				float fEntAng = acos ( fDiffZ / fDist ) * RADTODEG;
				if ( fDiffX < 0 ) fEntAng = 360.0f - fEntAng;
				float fSegAng;
				if ( fDist <= radius ) fSegAng = 70.0f;
				else fSegAng = asin ( ( radius ) / fDist ) * RADTODEG;
				if ( fSegAng >= 179.99f ) fSegAng = 179.99f;
				float fBeginAngle = fEntAng - fSegAng;
				if ( fBeginAngle < 0 ) fBeginAngle += 360.0f;
				if ( fBeginAngle > 360 ) fBeginAngle -= 360.0f;
				float fEndAngle = fBeginAngle + fSegAng*2.0f;
				
				// if not in OA list and close enough to the blocker
				bool bFound = false;
				for ( int i = 0; i < (int) sObstacleAngle_list.size( ) - 1; i++ )
				{
					if ( sObstacleAngle_list [ i ].pEntity == (Entity*) pBlocker )
					{
						bFound = true;
						sObstacleAngle_list [ i ].fAngBegin = fBeginAngle;
						sObstacleAngle_list [ i ].fAngEnd = fEndAngle;
						sObstacleAngle_list [ i ].fTimer = 0.4f;
					}
				}
				if ( !bFound && fDist < (2*radius) )
				{
					sObstacleAngle sNewObstacleAngle;
					sNewObstacleAngle.fAngBegin = fBeginAngle;
					sNewObstacleAngle.fAngEnd = fEndAngle;
					sNewObstacleAngle.fTimer = 0.4f;
					sNewObstacleAngle.pEntity = (Entity*) pBlocker;
					sObstacleAngle_list.push_back ( sNewObstacleAngle );
				}
			}

			// next container blocker in list
			pBlocker = pBlocker->pNextBlocker;
		}
	}

	// now sort the OA list
	sort ( sObstacleAngle_list.begin( ), sObstacleAngle_list.end( ) );

	// create a new combined OA list from any surviving/added items above (but miss out last one)
	sCombinedObstacleAngle_list.clear ( );
	//for ( int i = 0; i < (int) sObstacleAngle_list.size( ) - 1; i++ )  / chopping off the item which ants to avoid is bad!
	for ( int i = 0; i < (int) sObstacleAngle_list.size( ); i++ ) 
	{
		sCombinedObstacleAngle_list.push_back( sObstacleAngle_list [ i ] );
	}

	// determine if we should remove items inside other arcs, or make it all one big 360 degree arc
	if ( sCombinedObstacleAngle_list.size( ) > 1 )
	{
		bool bComplete = false;

		// remove items that have their hit-arcs entirely within another
		int iNext = 1;
		for ( int i = 0; i < (int) sCombinedObstacleAngle_list.size( ) - 1; i++ )
		{
			iNext = i + 1;
			if ( sCombinedObstacleAngle_list [ i ].fAngEnd > sCombinedObstacleAngle_list [ iNext ].fAngBegin )
			{
				if ( sCombinedObstacleAngle_list [ i ].fAngEnd < sCombinedObstacleAngle_list [ iNext ].fAngEnd )
				{
					sCombinedObstacleAngle_list [ i ].fAngEnd = sCombinedObstacleAngle_list [ iNext ].fAngEnd;
					if ( sCombinedObstacleAngle_list [ i ].fAngEnd - sCombinedObstacleAngle_list [ i ].fAngBegin >= 360.0f ) 
						bComplete = true;
				}
				sCombinedObstacleAngle_list.erase ( sCombinedObstacleAngle_list.begin( ) + iNext );
				i--;
			}
		}

		// remove items that have their hit-arcs entirely within another (for angles which are negative (<0))
		int iLast = (int) sCombinedObstacleAngle_list.size( ) - 1;
		float fBeginAngle = sCombinedObstacleAngle_list [ 0 ].fAngBegin + 360.0f;
		float fEndAngle = sCombinedObstacleAngle_list [ 0 ].fAngEnd + 360.0f;
		if ( sCombinedObstacleAngle_list [ iLast ].fAngEnd > fBeginAngle )
		{
			if ( sCombinedObstacleAngle_list [ iLast ].fAngEnd < fEndAngle )
			{
				sCombinedObstacleAngle_list [ iLast ].fAngEnd = fEndAngle;
				if ( sCombinedObstacleAngle_list [ iLast ].fAngEnd - sCombinedObstacleAngle_list [ iLast ].fAngBegin >= 360.0f ) 
					bComplete = true;
			}
			sCombinedObstacleAngle_list.erase ( sCombinedObstacleAngle_list.begin( ) );
		}

		// if mark as complete, reduce while list down to one item which repells everything (arc 0-360)
		if ( bComplete )
		{
			sCombinedObstacleAngle_list.clear ( );
			sObstacleAngle sNewObstacleAngle;
			sNewObstacleAngle.fAngBegin = 0.0f;
			sNewObstacleAngle.fAngEnd = 360.0f;
			sCombinedObstacleAngle_list.push_back ( sNewObstacleAngle );
		}
	}
}

void Entity::UpdateTimers ( float fTimeDelta )
{
	if ( fStateTimer > 0.0f )				fStateTimer -= fTimeDelta;
	if ( fLookAroundTimer > 0.0f )			fLookAroundTimer -= fTimeDelta;
	if ( fInvestigateTimer > 0.0f )			fInvestigateTimer -= fTimeDelta;
	if ( fCoverTimer > 0.0f )				fCoverTimer -= fTimeDelta;
	if ( fMoveFeedbackTimer > 0.0f )		fMoveFeedbackTimer -= fTimeDelta;
	if ( fAvoidTimer > 0.0f )				fAvoidTimer -= fTimeDelta;
	if ( fPathTimer > 0.0f )				fPathTimer -= fTimeDelta;
	if ( fForceUpdatePathTimer > 0.0f )		fForceUpdatePathTimer -= fTimeDelta;
	if ( fResetTimer > 0.0f )				fResetTimer -= fTimeDelta;
	if ( fWaitTimer > 0.0f )				fWaitTimer -= fTimeDelta;	
	if ( fExpensiveUpdateTimer > 0.0f )		fExpensiveUpdateTimer -= fTimeDelta;
	if ( fAdjustDestinationTimer > 0.0f )	fAdjustDestinationTimer -= fTimeDelta;
	if ( fAdjustDirectionTimer > 0.0f )		fAdjustDirectionTimer -= fTimeDelta;
	if ( fSearchTimer > 0.0f )				fSearchTimer -= fTimeDelta;
	if ( fObstacleTimer > 0.0f )			fObstacleTimer -= fTimeDelta;
}

void Entity::RebuildDynamicPathFinder( )
{
	//DynamicPathFinder *pDynamicPathFinder = new DynamicPathFinder( pContainer->pPathFinder );
	if ( !pDynamicPathFinder ) pDynamicPathFinder = new DynamicPathFinder( pContainer->pPathFinder );
	pDynamicPathFinder->ClearObjects( );
	pDynamicPathFinder->ClearWaypoints( );

	pWorld->pTeamController->StartGlobalEntityIterator ( );
	Entity *pOtherEntity = pWorld->pTeamController->GetNextGlobalEntity ( );

	float fTotalRadius = 0;

	while ( pOtherEntity )
	{
		if ( pOtherEntity == this || pOtherEntity->GetContainer( ) != pContainer )
		{
			pOtherEntity = pWorld->pTeamController->GetNextGlobalEntity ( );
			continue;
		}
		
		float fDiffX = pOtherEntity->GetX( ) - GetX( );
		float fDiffZ = pOtherEntity->GetZ( ) - GetZ( );
		float fDist = fDiffX*fDiffX + fDiffZ*fDiffZ;

		fTotalRadius = fRadius + pOtherEntity->GetRadius();

		if ( fDist < fTotalRadius*fTotalRadius*25 )
		{
			AvoidanceSphere *pColShape = new AvoidanceSphere( fTotalRadius, pOtherEntity->GetX( ), pOtherEntity->GetZ( ) );
			pDynamicPathFinder->AddObject( pColShape, false );
		}

		pOtherEntity = pWorld->pTeamController->GetNextGlobalEntity ( );
	}

	Hero *pHero = pWorld->pTeamController->GetPlayer( );
	
	if ( pHero && pHero->GetContainer( ) == pContainer )
	{
		float fDiffX = pHero->GetX( ) - GetX( );
		float fDiffZ = pHero->GetZ( ) - GetZ( );
		float fDist = fDiffX*fDiffX + fDiffZ*fDiffZ;

		fTotalRadius = pHero->GetRadius( )+fRadius;

		if ( fDist < fRadius*fRadius*25 )
		{
			AvoidanceSphere *pColShape = new AvoidanceSphere( fTotalRadius, pHero->GetX( ), pHero->GetZ( ) );
			pDynamicPathFinder->AddObject( pColShape, false );
		}
	}

	pDynamicPathFinder->CompleteObstacles( );
	pDynamicPathFinder->BuildWaypoints( );

	//delete pDynamicPathFinder;
}

void Entity::ForceMove( float x, float y )
{
	SetDestination( x, GetY(), y, pContainer->GetID() );
	cMovePath.Clear();
	cMovePath.AddPoint( GetX(), 0, GetY() );
	cMovePath.AddPoint( x, 0, y );
	iCurrentMovePoint = 1;
}

