#include ".\..\..\Shared\DBOFormat\DBOData.h"
#include "DBPro Functions.h"

#include "Hero.h"
#include "Entity.h"
#include "StateMachine\StateSet.h"
#include "Path.h"
#include "PathFinderAdvanced.h"
#include "World.h"
#include "Beacon.h"
#include "Zone.h"
#include "Team.h"
#include "TeamController.h"
#include "Container.h"
#include "DynamicPathFinder.h"
#include <math.h>
#include <algorithm>

#include "LeeThread.h"

#define PI 3.14159265f
#define RADTODEG 57.295779513f
#define DEGTORAD 0.01745329252f

// Externals
extern World cWorld;
extern LeeThread g_LeeThread;
extern int g_GUIShaderEffectID;

// Globals
World* Entity::pWorld = NULL;
bool Entity::bShowPaths = false;
bool Entity::bShowViewArcs = false;
bool Entity::bShowAvoidanceAngles = false;
float Entity::fDebugViewObjHeight = 0.0f;
float Entity::fDebugObstacleAnglesObjHeight = 0.0f;
int Entity::iAvoidMode = 5;

// So only 1 path update is called every frame
float CurrentAIWorkedPathTimer = 0.0f;
vector <int> ListOfEntitiesToUpdateMovement;

Entity::Entity ( )
{
	SetDefaultValues ( );
}

Entity::Entity ( int id, sObject *pObj, sObject *pObjRef, Container* pCurrContainer )
{
	SetDefaultValues ( );
	
	bActive = true;
	iID = id;
	pObject = pObj;
	dwObjectNumberRef = pObjRef->dwObjectNumber;
	pContainer = pCurrContainer;
	
	if ( pObject ) 
	{
		vecFinalDest = pObject->position.vecPosition;
		fPosX = vecFinalDest.x;
		fPosY = vecFinalDest.y;
		fPosZ = vecFinalDest.z;
		fAngY = pObject->position.vecRotate.y;
	}
	else
	{
		vecFinalDest.x = 0.0f;
		vecFinalDest.y = 0.0f;
		vecFinalDest.z = 0.0f;
		fPosX = 0;
		fPosY = 0;
		fPosZ = 0;
		fAngY = 0;
	}

	vecOrigPos = vecFinalDest;
	vecDefendPos = vecOrigPos;

	iOriginalContainer = pContainer->GetID( );
	iDefendContainer = iOriginalContainer;

	vecLookAt = vecFinalDest;

	//Dave - manual only
	//this->ChangeState ( pWorld->pEntityStates->pStateIdle );

	//Dave
	this->SetManualControl( true );
	this->ChangeState ( pWorld->pEntityStates->pStateManual );
}

Entity::~Entity ( ) 
{ 
	ClearCoverPoint();

	sHitFrom_list.clear ( );
	sTarget_list.clear ( );

	DebugHideDestination();
	DebugHideObstacleAngles();
	DebugHideViewArcs();

	if ( pDynamicPathFinder ) delete pDynamicPathFinder;
}

void Entity::SetDefaultValues ( )
{
	//stagger = 0;
	pre_vecFinalDest.x = -999;
	pre_vecLastDest.x = -999;

	pDynamicPathFinder = 0;

	bMeleeMode = false;

	bActive = true;
	iID = 0;
	pCurrentState = 0;
	pLastState = 0;
	pObject = 0;
	pContainer = 0;
	iAggressiveness = 0;
	iDestContainer = -1;
	iEntityAvoidMode = -1;
	bAlwaysActive = false;

	iCurrentMovePoint = 1;
	iOriginalContainer = -1;
	iDefendContainer = -1;
	bChangingContainers = false;

	fPosX = 0; fPosY = 0; fPosZ = 0;
	fAngY = 0;
	fHeight = 0;

	bReservedSpace = false;

	fStateTimer = 0.0f;
	fLookAroundTimer = 0.0f;
	fCoverTimer = 0.0f;
	fMoveFeedbackTimer = 0.0f;
	fInvestigateTimer = 0.0f;
	fAvoidTimer = 0.0f;
	fPathTimer = 0.0f;
	fForceUpdatePathTimer = 0.0f;
	fResetTimer = 0.0f;
	fWaitTimer = 0.0f;
	fExpensiveUpdateTimer = ( 1.0f * rand() ) / RAND_MAX + 1.0f;
	fAdjustDestinationTimer = 0;
	fAdjustDirectionTimer = 0;
	fSearchTimer = 0;
	fObstacleTimer = 0;

	fAvgPatrolTime = 3.5f;

	fAlertLevel = 0.0f;

	bHit = false;
	bFireWeapon = false;
	bIsDucking = false;
	bHeardSound = false;
	fClosestSound = 0;
	iSoundUrgency = 0;
	iLastSoundUrgency = 0;
	iInvestigateUrgency = 0;

	bAdjustingDirection = false;

	bCanDuck = true;
	bCanStrafe = true;
	bCanAttack = true;
	bCanHear = true;
	bCanRoam = false;
	bCanSearch = true;
	bCanSelectTargets = true;

	cSearchPath.Clear ( );
	cSearchPath.CalculateSqrLength ( );

	fSpeed = 5.0f;
	fCurrSpeed = fSpeed;
	fTurnSpeed = fSpeed*7.5f;//500.0f;//240.0f; 140217 - turn twice as fast for fast movers (can adjust in LUA too)

	fRadius = 2.5f;
	fMinimumDist = fRadius * ( ( 4.0f * rand( ) ) / RAND_MAX + 6.5f );
	fAvoidDist = fRadius * 6.0f;
	fPathStartCostLimit = -1.0f;

	fDefendDist = 15.0f;

	fDistanceMoved = 0.0f;
	bStuck = false;
	bAvoiding = false;
	bMakingProgress = true;
	bMoving = false;
	bAvoidLeft = true;
	bRedoPath = false;
	bLegacyForceMove = false;
	
	bFollowingLeader = false;
	bDefending = false;
	bManualControl = false;
	bLookAtPointSet = false;	

	bHitSomething = false;
	bHitBySomething = false;
	pHitEntity = 0;
	pHitByEntity = 0;

	vecOrigPos.x = 0; vecOrigPos.y = 0; vecOrigPos.z = 0;
	vecDefendPos.x = 0; vecDefendPos.y = 0; vecDefendPos.z = 0;
	vecHidePos.x = 0; vecHidePos.y = 0; vecHidePos.z = 0;
	vecOldPos.x = 0; vecOldPos.y = 0; vecOldPos.z = 0;
	vecAvoidPos.x = 0; vecAvoidPos.y = 0; vecAvoidPos.z = 0;
	vecLastPos.x = 0; vecLastPos.y = 0; vecLastPos.z = 0;

	vecSoundPos.x = 0; vecSoundPos.y = 0; vecSoundPos.z = 0;
	vecInterestPos.x = 0; vecInterestPos.y = 0; vecInterestPos.z = 0;

	for (int i=0; i<100; i++ ) { pvecDir[i].x = 0; pvecDir[i].y = 0; pvecDir[i].z = 0; }
	vecLastDest.x = 0; vecLastDest.y = 0; vecLastDest.z = 0;
	vecFinalDest.x = 0; vecFinalDest.y = 0; vecFinalDest.z = 0;
	vecCurrDest.x = 0; vecCurrDest.y = 0; vecCurrDest.z = 0;
	vecLookAt.x = 0; vecLookAt.y = 0; vecLookAt.z = 0;
	vecCurrLookAt.x = 0; vecCurrLookAt.y = 0; vecCurrLookAt.z = 0;

	fFireArc = 20.0f;
	fInnerViewArc = 180.0f;
	fOuterViewArc = 180.0f;
	fViewRange = 60.0f;
	fHearingRange = 50.0f;
	iHearingThreshold = 0;

	iInnerViewObject = 0;
	iOuterViewObject = 0;
	iHearingRangeObject = 0;
	iCurrDestObject = 0;
	iObstacleAnglesObject = 0;

	pTeam = 0;

	sTarget_list.clear ( );
	sHitFrom_list.clear ( );
	sObstacleAngle_list.clear ( );
	sCombinedObstacleAngle_list.clear( );

	pPatrolPath = 0;
	iPatrolPathID = 0;
	iCurrentPatrolPoint = 0;

	pNextEntity = 0;

	bIsBehindCorner = false;
	bCanHideBehindCorner = false;

	fDiveRange = 0;
	bIsDiving = false;
	fLeapRange = 0;
	bIsLeaping = false;
	bCanDive = false;
	bCanLeap = false;
	fHideDirX = 0;
	fHideDirY = 0;
	fPeekX = 0;
	fPeekY = 0;
	bForceOutOfCover = false;
	fDiveMultiplier = 1.0f;
	fLeapMultiplier = 1.0f;
	
	pCoverPoint = 0;
}

void Entity::ResetState ( )
{
	if ( !bManualControl )
	{
		fResetTimer = 4.0f;
		ChangeState ( pWorld->pEntityStates->pStateIdle );
	}
}

bool Entity::GetMeleeMode( )
{
	return bMeleeMode;
}

void Entity::SetMeleeMode( bool mode )
{
	bMeleeMode = mode;
}

int Entity::GetInterestContainer( )
{
	return iInterestContainer;
}

bool Entity::GetActive( )
{
	return bActive;
}

int Entity::GetAggressiveness( )
{
	return iAggressiveness;
}

int Entity::GetSearchPoints( )
{
	return iSearchPoints;
}

int Entity::GetCurrentTargetIndex( )
{
	return iCurrentTargetIndex;
}

int Entity::GetInvestigateUrgency( )
{
	return iInvestigateUrgency;
}

int Entity::GetDefendContainer( )
{
	return iDefendContainer;
}

bool Entity::GetCanDuck( )
{
	return bCanDuck;
}

bool Entity::GetCanAttack( )
{
	return bCanAttack;
}

bool Entity::GetCanStrafe( )
{
	return bCanStrafe;
}

bool Entity::GetCanHear( )
{
	return bCanHear;
}

bool Entity::GetCanRoam( )
{
	return bCanRoam;
}

bool Entity::GetCanSearch( )
{
	return bCanSearch;
}

bool Entity::GetCanSelectTargets( )
{
	return bCanSelectTargets;
}

bool Entity::GetCanHideBehindCorner( )
{
	return bCanHideBehindCorner;
}

bool Entity::GetInVerticalCover( )
{
	return bInVerticalCover;
}

bool Entity::GetInDuckingCover( )
{
	return bInDuckingCover;
}

bool Entity::GetManualControl( )
{
	return bManualControl;
}

bool Entity::GetIsBehindCorner( )
{
	return bIsBehindCorner;
}

void Entity::SetIsBehindCorner( bool bValue )
{
	bIsBehindCorner = bValue;
}

void Entity::SetHideDirection( float dirx, float diry )
{
	fHideDirX = dirx;
	fHideDirY = diry;
}

void Entity::SetPeekingPos( float x, float y )
{
	fPeekX = x;
	fPeekY = y;
}

float Entity::GetHideX()
{
	return fHideDirX;
}

float Entity::GetHideY()
{
	return fHideDirY;
}

bool Entity::GetCanLeap( )
{
	return bCanLeap;
}

bool Entity::GetIsLeaping( )
{
	return bIsLeaping;
}

bool Entity::GetCanDive( )
{
	return bCanDive;
}

bool Entity::GetIsDiving()
{
	return bIsDiving;
}

void Entity::SetDiving( bool value )
{
	bIsDiving = value;
}

void Entity::SetLeaping( bool value )
{
	bIsLeaping = value;
}

float Entity::GetPeekX()
{
	return fPeekX;
}

float Entity::GetPeekY()
{
	return fPeekY;
}

void Entity::ForceOutOfCover( bool mode )
{
	bForceOutOfCover = mode;
}

bool Entity::IsForcedOutOfCover()
{
	return bForceOutOfCover;
}

float Entity::GetOrigPos( DWORD flag )
{
	if ( flag & ENT_X ) return vecOrigPos.x;
	if ( flag & ENT_Y ) return vecOrigPos.y;
	if ( flag & ENT_Z ) return vecOrigPos.z;
	return 0;
}

float Entity::GetDefendPos( DWORD flag )
{
	if ( flag & ENT_X ) return vecDefendPos.x;
	if ( flag & ENT_Y ) return vecDefendPos.y;
	if ( flag & ENT_Z ) return vecDefendPos.z;
	return 0;
}

float Entity::GetHidePos( DWORD flag )
{
	if ( flag & ENT_X ) return vecHidePos.x;
	if ( flag & ENT_Y ) return vecHidePos.y;
	if ( flag & ENT_Z ) return vecHidePos.z;
	return 0;
}

float Entity::GetOldPos( DWORD flag )
{
	if ( flag & ENT_X ) return vecOldPos.x;
	if ( flag & ENT_Y ) return vecOldPos.y;
	if ( flag & ENT_Z ) return vecOldPos.z;
	return 0;
}

float Entity::GetAvoidPos( DWORD flag )
{
	if ( flag & ENT_X ) return vecAvoidPos.x;
	if ( flag & ENT_Y ) return vecAvoidPos.y;
	if ( flag & ENT_Z ) return vecAvoidPos.z;
	return 0;
}

float Entity::GetLastPos( DWORD flag )
{
	if ( flag & ENT_X ) return vecLastPos.x;
	if ( flag & ENT_Y ) return vecLastPos.y;
	if ( flag & ENT_Z ) return vecLastPos.z;
	return 0;
}

float Entity::GetSoundPos( DWORD flag )
{
	if ( flag & ENT_X ) return vecSoundPos.x;
	if ( flag & ENT_Y ) return vecSoundPos.y;
	if ( flag & ENT_Z ) return vecSoundPos.z;
	return 0;
}

float Entity::GetInterestPos( DWORD flag )
{
	if ( flag & ENT_X ) return vecInterestPos.x;
	if ( flag & ENT_Y ) return vecInterestPos.y;
	if ( flag & ENT_Z ) return vecInterestPos.z;
	return 0;
}

float Entity::GetLastDest( DWORD flag )
{
	if ( flag & ENT_X ) return vecLastDest.x;
	if ( flag & ENT_Y ) return vecLastDest.y;
	if ( flag & ENT_Z ) return vecLastDest.z;
	return 0;
}

float Entity::GetFinalDest( DWORD flag )
{
	if ( flag & ENT_X ) return vecFinalDest.x;
	if ( flag & ENT_Y ) return vecFinalDest.y;
	if ( flag & ENT_Z ) return vecFinalDest.z;
	return 0;
}

float Entity::GetCurrDest( DWORD flag )
{
	if ( flag & ENT_X ) return vecCurrDest.x;
	if ( flag & ENT_Y ) return vecCurrDest.y;
	if ( flag & ENT_Z ) return vecCurrDest.z;
	return 0;
}

float Entity::GetLookAt( DWORD flag )
{
	if ( flag & ENT_X ) return vecLookAt.x;
	if ( flag & ENT_Y ) return vecLookAt.y;
	if ( flag & ENT_Z ) return vecLookAt.z;
	return 0;
}

float Entity::GetCurrLookAt( DWORD flag )
{
	if ( flag & ENT_X ) return vecCurrLookAt.x;
	if ( flag & ENT_Y ) return vecCurrLookAt.y;
	if ( flag & ENT_Z ) return vecCurrLookAt.z;
	return 0;
}

Path* Entity::GetSearchPath( )
{
	return &cSearchPath;
}

float Entity::GetSearchPointX( DWORD index )
{
	return cSearchPath.GetPoint( index ).x;
}

float Entity::GetSearchPointY( DWORD index )
{
	return cSearchPath.GetPoint( index ).y;
}

int Entity::GetSearchPointContainer( DWORD index )
{
	return cSearchPath.GetPoint( index ).container;
}

void Entity::RemoveFromMovementList()
{
	for ( int lindex = 0; lindex < ListOfEntitiesToUpdateMovement.size(); lindex++ )
	{
		if ( ListOfEntitiesToUpdateMovement[lindex] == GetID() )
		{
			ListOfEntitiesToUpdateMovement.erase(ListOfEntitiesToUpdateMovement.begin()+lindex);
			bRedoPath = false;
			break;
		}
	}
}

void Entity::SetActive( bool bValue )
{
	// set state for entity active status
	bActive = bValue;

	// if deactivating, ensure it is removed from movement list
	if ( bValue == false ) 
	{
		RemoveFromMovementList();
	}
}

void Entity::SetAggressiveness( int iValue )
{
	//char str [ 256 ];
	//sprintf_s( str, "STOP (SA) = %d", iValue );
	//MessageBox( NULL, str, "Info", 0 );

	iAggressiveness = iValue;
}

void Entity::SetSearchPoints( int iValue )
{
	iSearchPoints = iValue;
}

void Entity::IncSearchPoints( int iValue )
{
	iSearchPoints += iValue;
}

void Entity::SetCurrentTargetIndex( int iValue )
{
	iCurrentTargetIndex = iValue;
}

void Entity::SetInvestigateUrgency( int iValue )
{
	iInvestigateUrgency = iValue;
}

void Entity::SetDefendContainer( int iValue )
{
	iDefendContainer = iValue;
}

void Entity::SetCanDuck( bool bValue )
{
	bCanDuck = bValue;
}

void Entity::SetCanHideBehindCorner( bool bValue )
{
	bCanHideBehindCorner = bValue;
}

void Entity::SetCanDive( bool bValue )
{
	bCanDive = bValue;
}

void Entity::SetCanLeap( bool bValue )
{
	bCanLeap = bValue;
}

void Entity::SetDiveRange( float fValue )
{
	fDiveRange = fValue;
}

void Entity::SetLeapRange( float fValue )
{
	fLeapRange = fValue;
}

void Entity::SetCanAttack( bool bValue )
{
	bCanAttack = bValue;
}

void Entity::SetCanStrafe( bool bValue )
{
	bCanStrafe = bValue;
}

void Entity::SetCanHear( bool bValue )
{
	bCanHear = bValue;
}

void Entity::SetCanRoam( bool bValue )
{
	bCanRoam = bValue;
}

void Entity::SetCanSearch( bool bValue )
{
	bCanSearch = bValue;
}

void Entity::SetCanSelectTargets( bool bValue )
{
	bCanSelectTargets = bValue;
}

void Entity::SetInVerticalCover( bool bValue )
{
	bInVerticalCover = bValue;
}

void Entity::SetInDuckingCover( bool bValue )
{
	bInDuckingCover = bValue;
}

void Entity::SetManualControl( bool bValue )
{
	bManualControl = bValue;
}

void Entity::SetHitSomething( bool bValue )
{
	bHitSomething = bValue;
}

void Entity::SetHitBySomething( bool bValue )
{
	bHitBySomething = bValue;
}

void Entity::SetHitEntity( Entity *pValue )
{
	pHitEntity = pValue;
}

void Entity::SetHitByEntity( Entity *pValue )
{
	pHitByEntity = pValue;
}

void Entity::SetOrigPos( float x, float y, float z, DWORD flag )
{
	if ( flag & ENT_X ) vecOrigPos.x = x;
	if ( flag & ENT_Y ) vecOrigPos.y = y;
	if ( flag & ENT_Z ) vecOrigPos.z = z;
}

void Entity::SetDefendPos( float x, float y, float z, DWORD flag )
{
	if ( flag & ENT_X ) vecDefendPos.x = x;
	if ( flag & ENT_Y ) vecDefendPos.y = y;
	if ( flag & ENT_Z ) vecDefendPos.z = z;
}

void Entity::SetHidePos( float x, float y, float z, DWORD flag )
{
	if ( flag & ENT_X ) vecHidePos.x = x;
	if ( flag & ENT_Y ) vecHidePos.y = y;
	if ( flag & ENT_Z ) vecHidePos.z = z;
}

void Entity::SetOldPos( float x, float y, float z, DWORD flag )
{
	if ( flag & ENT_X ) vecOldPos.x = x;
	if ( flag & ENT_Y ) vecOldPos.y = y;
	if ( flag & ENT_Z ) vecOldPos.z = z;
}

void Entity::SetAvoidPos( float x, float y, float z, DWORD flag )
{
	if ( flag & ENT_X ) vecAvoidPos.x = x;
	if ( flag & ENT_Y ) vecAvoidPos.y = y;
	if ( flag & ENT_Z ) vecAvoidPos.z = z;
}

void Entity::SetLastPos( float x, float y, float z, DWORD flag )
{
	if ( flag & ENT_X ) vecLastPos.x = x;
	if ( flag & ENT_Y ) vecLastPos.y = y;
	if ( flag & ENT_Z ) vecLastPos.z = z;
}

void Entity::SetSoundPos( float x, float y, float z, DWORD flag )
{
	if ( flag & ENT_X ) vecSoundPos.x = x;
	if ( flag & ENT_Y ) vecSoundPos.y = y;
	if ( flag & ENT_Z ) vecSoundPos.z = z;
}

void Entity::SetInterestPos( float x, float y, float z, DWORD flag )
{
	if ( flag & ENT_X ) vecInterestPos.x = x;
	if ( flag & ENT_Y ) vecInterestPos.y = y;
	if ( flag & ENT_Z ) vecInterestPos.z = z;
}

void Entity::SetLastDest( float x, float y, float z, DWORD flag )
{
	if ( flag & ENT_X ) vecLastDest.x = x;
	if ( flag & ENT_Y ) vecLastDest.y = y;
	if ( flag & ENT_Z ) vecLastDest.z = z;
}

void Entity::SetFinalDest( float x, float y, float z, DWORD flag )
{
	if ( flag & ENT_X ) vecFinalDest.x = x;
	if ( flag & ENT_Y ) vecFinalDest.y = y;
	if ( flag & ENT_Z ) vecFinalDest.z = z;
}

void Entity::SetCurrDest( float x, float y, float z, DWORD flag )
{
	if ( flag & ENT_X ) vecCurrDest.x = x;
	if ( flag & ENT_Y ) vecCurrDest.y = y;
	if ( flag & ENT_Z ) vecCurrDest.z = z;
}

void Entity::SetLookAt( float x, float y, float z, DWORD flag )
{
	if ( flag & ENT_X ) vecLookAt.x = x;
	if ( flag & ENT_Y ) vecLookAt.y = y;
	if ( flag & ENT_Z ) vecLookAt.z = z;
}

void Entity::SetCurrLookAt( float x, float y, float z, DWORD flag )
{
	if ( flag & ENT_X ) vecCurrLookAt.x = x;
	if ( flag & ENT_Y ) vecCurrLookAt.y = y;
	if ( flag & ENT_Z ) vecCurrLookAt.z = z;
}

void Entity::ClearSearchPath( )
{
	cSearchPath.Clear( );
}



void Entity::SetRadius ( float fNewRadius )
{
	if ( fNewRadius < 0.0001f ) fNewRadius = 0.0001f;	//division by zero counter
	
	fMinimumDist /= fRadius;
	fAvoidDist /= fRadius;
	fRadius = fNewRadius;
	fMinimumDist *= fRadius;
	fAvoidDist *= fRadius;
}

float Entity::GetRadius ( )
{
	return fRadius;
}

void Entity::SetContainer ( Container *pNewContainer )
{
	int l_iAvoidMode = iEntityAvoidMode;
	if ( l_iAvoidMode < 0 ) l_iAvoidMode = iAvoidMode;

	if ( l_iAvoidMode == 5 ) pContainer->pPathFinder->GridClearEntityPosition( GetX(), GetZ( ) );

	pContainer = pNewContainer;
	if ( pDynamicPathFinder && pContainer ) pDynamicPathFinder->SetMainPathFinder( pContainer->pPathFinder );
}

Container * Entity::GetContainer ( )
{
	return pContainer;
}

bool Entity::IsChangingContainers( )
{
	return bChangingContainers;
}

bool Entity::Intersect ( float fSX, float fSZ, float fEX, float fEZ, int *iSide )
{
	float fVX = fEX - fSX;
	float fVZ = fEZ - fSZ;
	float fSqrDist = fVX*fVX + fVZ*fVZ;
	if ( fSqrDist < 0.00001f ) return false;

	float fV2X = GetX( ) - fSX;
	float fV2Z = GetZ( ) - fSZ;
	float fTValue = ( fVX*fV2X + fVZ*fV2Z ) / fSqrDist;
	float fNewSide = fVZ*fV2X - fVX*fV2Z;

	if ( fTValue < 0.0f ) fTValue = 0.0f;
	if ( fTValue > 1.0f ) fTValue = 1.0f;

	float fClosestX = fVX*fTValue + fSX;
	float fClosestZ = fVZ*fTValue + fSZ;

	fVX = fClosestX - GetX( );
	fVZ = fClosestZ - GetZ( );
	fSqrDist = fVX*fVX + fVZ*fVZ;

	if ( fSqrDist < fRadius*fRadius ) 
	{
		*iSide = fNewSide > 0 ? 1 : -1;
		return true;
	}

	return false;
}

void Entity::SetPosition ( float x, float y, float z )
{
	//if ( pContainer ) pContainer->pPathFinder->GridMoveEntity( GetX(), GetZ(), x, z );
	if ( pObject )
	{
		pObject->position.vecPosition.x = x;
		pObject->position.vecPosition.y = y;
		pObject->position.vecPosition.z = z;
	}
	else
	{
		fPosX = x;
		fPosY = y;
		fPosZ = z;
	}
	vecLastRecPos.x = x;
	vecLastRecPos.y = y;
	vecLastRecPos.z = z;
}

void Entity::StayWithinContainer ( float fLastX, float fLastZ, float* pX, float* pZ )
{
	// always keep AI inside container (or outside obstacles)
	// note: gets called just after entity pos is moved by 'MoveWithAnimation'
	if ( pContainer )
	{
		if ( pContainer->pPathFinder->InPolygons ( *pX, *pZ ) > 0 )
		{
			// find closest polygon
			float fResult = pContainer->pPathFinder->FindClosestPolygon ( fLastX, fLastZ, *pX, *pZ );
			if ( fResult > 0.0001f ) // fResult >= 0.0f ) // prevents stopping AI when traversing edge
			{
				*pX = fLastX + ( *pX - fLastX ) * fResult;
				*pZ = fLastZ + ( *pZ - fLastZ ) * fResult;
			}
			else
			{
				// 080317 - absolutely prevent AI from leaving the zone, revert to last good position
				*pX = fLastX;
				*pZ = fLastZ;
			}
		}
	}
}

void Entity::SetYPosition ( float y )
{
	if ( pObject ) pObject->position.vecPosition.y = y;
	else fPosY = y;

	vecLastRecPos.y = y;
}

void Entity::SetDestination ( float x, float y, float z, int container )
{
	if ( container < 0 ) container = GetContainer( )->GetID( );

	float fDiffX = vecFinalDest.x - x;
	float fDiffY = vecFinalDest.y - y;
	float fDiffZ = vecFinalDest.z - z;
	
	float fDist = fDiffX*fDiffX + fDiffY*fDiffY + fDiffZ*fDiffZ;
	
	if ( fDist > 0.00001 || container != GetContainer( )->GetID( ) )
	{
		pre_vecFinalDest.x = x;
		pre_vecFinalDest.y = y;
		pre_vecFinalDest.z = z;

		pre_vecLastDest.x = x;
		pre_vecLastDest.y = y;
		pre_vecLastDest.z = z;

		iDestContainer = container;

		// calculate the path we need
		if ( bRedoPath == false )
		{
			// this entity can be updated
			int n = 0;
			int iMaxInList = (int)ListOfEntitiesToUpdateMovement.size()-1;
			for (; n <= iMaxInList; n++ )
				if ( ListOfEntitiesToUpdateMovement[n] == this->iID )
					break;
			if ( n > iMaxInList )
				ListOfEntitiesToUpdateMovement.push_back(this->iID);
			bRedoPath = true;
		}
	}
}

void Entity::SetDestinationForced ( float x, float y, float z, int container, bool bMoveAddition )
{
	if ( container < 0 ) container = GetContainer( )->GetID( );
	float fDiffX = vecFinalDest.x - x;
	float fDiffY = vecFinalDest.y - y;
	float fDiffZ = vecFinalDest.z - z;
	float fDist = fDiffX*fDiffX + fDiffY*fDiffY + fDiffZ*fDiffZ;
	if ( fDist > 0.00001 || container != GetContainer( )->GetID( ) )
	{
		vecFinalDest.x = x;
		vecFinalDest.y = y;
		vecFinalDest.z = z;
		vecLastDest.x = x;
		vecLastDest.y = y;
		vecLastDest.z = z;
		pre_vecFinalDest.x = -999;
		pre_vecLastDest.x = -999;
		iDestContainer = container;
		if ( bMoveAddition == true )
		{
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
		}
	}
}

void Entity::SetIdlePos ( float x, float y, float z, int container = -1 )
{
	vecOrigPos.x = x;
	vecOrigPos.y = y;
	vecOrigPos.z = z;

	iOriginalContainer = container > 0 ? container : pContainer->GetID( );
}

void Entity::SetAngleY( float angY )
{
	fAngY = angY;
	//if ( pObject ) YRotateObject( iID, fAngY ); // 210918 - decouple forcing angle to object (so script can smooth it)
}

void Entity::SetHeight( float height )
{
	fHeight = height;
}

void Entity::SetTeam ( Team *pNewTeam )
{
	pTeam = pNewTeam;
}

float Entity::GetHeight( )
{
	return bIsDucking ? fHeight/2 : fHeight;
}

float Entity::GetFullHeight( )
{
	return fHeight;
}

int Entity::GetID ( )
{
	return iID;
}

float Entity::GetX ( ) { if ( pObject ) return pObject->position.vecPosition.x; else return fPosX; }
float Entity::GetY ( ) { if ( pObject ) return pObject->position.vecPosition.y; else return fPosY; }
float Entity::GetZ ( ) { if ( pObject ) return pObject->position.vecPosition.z; else return fPosZ; }
float Entity::GetAngleY ( ) 
{ 
	float AngY = fAngY;
	if ( pObject ) AngY = pObject->position.vecRotate.y;
	
	while ( AngY > 360.0f ) AngY -= 360.0;
	while ( AngY < 0.0f )	AngY += 360.0;

	return AngY; 
}

bool Entity::IsTurning ( )
{
	if ( !bLookAtPointSet ) return bMoving;

	float x = vecLookAt.x - GetX ( );
	float z = vecLookAt.z - GetZ ( );
	float fLookAngY;

	if ( fabs ( x ) + fabs ( z ) < 0.1f ) 
	{
		return false;
	}
	else
	{
		fLookAngY = z / sqrt ( x*x + z*z );
		fLookAngY = acos ( fLookAngY ) * RADTODEG;
		if ( x < 0.0f ) fLookAngY = 360.0f - fLookAngY;
	}
	
	float fArc = fabs ( fLookAngY - GetAngleY( ) );
	if ( fArc > 180.0f ) fArc  = 360.0f - fArc;

	return fArc > 0.1f;
}

bool Entity::IsAvoiding ( )
{
	return bAvoiding;
}

bool Entity::IsMakingProgress ( )
{
	return bMakingProgress;
}

bool Entity::IsMoving ( )
{
	return bMoving;
}

float Entity::GetDestX ( ) { return vecFinalDest.x; }
float Entity::GetDestY ( ) { return vecFinalDest.y; }
float Entity::GetDestZ ( ) { return vecFinalDest.z; }

float Entity::GetTargetX ( ) 
{ 
	if ( CountTargets( ) == 0 ) return 0.0f;
	return sTarget_list [ 0 ].vecPos.x;
}

float Entity::GetTargetY ( ) 
{ 
	if ( CountTargets( ) == 0 ) return 0.0f;
	return sTarget_list [ 0 ].vecPos.y;
}

float Entity::GetTargetZ ( )  
{ 
	if ( CountTargets( ) == 0 ) return 0.0f;
	return sTarget_list [ 0 ].vecPos.z;
}

int Entity::GetTargetContainer( )  
{ 
	if ( CountTargets( ) == 0 ) return -1;
	return sTarget_list [ 0 ].iContainer;
}

float Entity::GetTargetGuessX ( ) 
{ 
	if ( CountTargets( ) == 0 ) return 0.0f;
	return sTarget_list [ 0 ].vecGuessPos.x;
}

float Entity::GetTargetGuessY ( ) 
{ 
	if ( CountTargets( ) == 0 ) return 0.0f;
	return sTarget_list [ 0 ].vecGuessPos.y;
}

float Entity::GetTargetGuessZ ( )  
{ 
	if ( CountTargets( ) == 0 ) return 0.0f;
	return sTarget_list [ 0 ].vecGuessPos.z;
}

float Entity::GetTargetDirX ( ) 
{ 
	if ( CountTargets( ) == 0 ) return 0.0f;
	return sTarget_list [ 0 ].vecLastDir.x;
}

float Entity::GetTargetDirY ( ) 
{ 
	if ( CountTargets( ) == 0 ) return 0.0f;
	return sTarget_list [ 0 ].vecLastDir.y;
}

float Entity::GetTargetDirZ ( )  
{ 
	if ( CountTargets( ) == 0 ) return 0.0f;
	return sTarget_list [ 0 ].vecLastDir.z;
}

int Entity::GetTargetType ( )
{
	if ( CountTargets( ) == 0 ) return -1;
	return sTarget_list [ 0 ].iTargetType;
}

bool Entity::GetIsZoneTarget( )
{
	if ( CountTargets( ) == 0 ) return false;
	return sTarget_list [ 0 ].bZoneTarget;
}

int Entity::GetTargetID ( int iIndex )
{
	if ( iIndex < 1 || iIndex > CountTargets( ) ) return 0;
	iIndex--;

	if ( sTarget_list [ iIndex ].iTargetType == 1 && sTarget_list [ iIndex ].pTargetEntity )	
	{
		return sTarget_list [ iIndex ].pTargetEntity->GetID( );
	}

	if ( sTarget_list [ iIndex ].iTargetType == 0 && sTarget_list [ iIndex ].pTargetHero )	
	{
		return sTarget_list [ iIndex ].pTargetHero->GetID( );
	}

	return 0;
}

bool Entity::GetIsFiring ( )
{
	return bFireWeapon;
}

bool Entity::GetIsDucking ( )
{
	return bIsDucking;
}

float Entity::GetDistToDest ( ) { return sqrt ( GetSqrDistToDest ( ) ); }
float Entity::GetDistToTarget ( ) { return sqrt ( GetSqrDistToTarget ( ) ); }
float Entity::GetDistTo ( GGVECTOR3 vecPoint ) { return sqrt ( GetSqrDistTo ( vecPoint ) ); }
float Entity::GetDistTo ( float x, float y, float z ) { return sqrt ( GetSqrDistTo ( x, y, z ) ); }

float Entity::GetSqrDistToDest ( )
{
	float fX = vecFinalDest.x - GetX ( );
	float fY = vecFinalDest.y - GetY ( );
	float fZ = vecFinalDest.z - GetZ ( );

	if ( pContainer->GetID( ) != iDestContainer ) return fX*fX + fZ*fZ + 10000;

	//return fX*fX + fY*fY + fZ*fZ;
	return fX*fX + fZ*fZ;
}

float Entity::GetSqrDistToTarget ( )
{
	float fX = GetTargetX ( ) - GetX ( );
	float fY = GetTargetY ( ) - GetY ( );
	float fZ = GetTargetZ ( ) - GetZ ( );

	return fX*fX + fZ*fZ;
}

float Entity::GetSqrDistTo ( GGVECTOR3 vecPoint )
{
	float fX = vecPoint.x - GetX ( );
	float fY = vecPoint.y - GetY ( );
	float fZ = vecPoint.z - GetZ ( );

	return fX*fX + fZ*fZ;
}

float Entity::GetSqrDistTo ( float x, float y, float z )
{
	float fX = x - GetX ( );
	float fY = y - GetY ( );
	float fZ = z - GetZ ( );

	return fX*fX + fZ*fZ;
}

char* Entity::GetStateName ( )
{
	return pCurrentState->GetName ( );
}

void Entity::SetPatrolPath ( Path* pPath, int iPathID )
{
	pPatrolPath = pPath;
	iPatrolPathID = iPathID;
	iCurrentPatrolPoint = 0;
}

void Entity::RemovePatrolPath ( )
{
	pPatrolPath = 0;
	iPatrolPathID = 0;
	iCurrentPatrolPoint = 0;
}

bool Entity::PatrolPathExist ( )
{
	CheckPath ( );
	return ( pPatrolPath != 0 );
}

int Entity::CountPatrolPoints ( )
{
	CheckPath ( );
	if ( !pPatrolPath ) return 0;
	
	return pPatrolPath->CountPoints( );
}

void Entity::CheckPath ( )
{
	if ( pPatrolPath )
	{
		if ( !pWorld->GetPath ( iPatrolPathID ) )
		{
			pPatrolPath = 0;
			iPatrolPathID = 0;
			iCurrentPatrolPoint = 0;
		}
	}
}

void Entity::CheckTargets ( )
{
	for ( int i = 0; i < (int) sTarget_list.size( ); i++ )
	{
		if ( sTarget_list[ i ].iTargetType == 0 )
		{
			if ( !pWorld->GetHero ( sTarget_list[ i ].iTargetID ) )
			{
				sTarget_list.erase ( sTarget_list.begin( ) + i );
				i--;
			}
		} else if ( sTarget_list[ i ].iTargetType == 1 )
		{
			if ( !pWorld->GetEntity ( sTarget_list[ i ].iTargetID ) )
			{
				sTarget_list.erase ( sTarget_list.begin( ) + i );
				i--;
			}
		}
	}
}

bool Entity::FindTargets ( )
{
	if ( !pTeam ) return sTarget_list.size( ) > 0;
	if ( pTeam->IsNeutral ( ) ) return sTarget_list.size( ) > 0;
	if ( !bCanSelectTargets ) return sTarget_list.size( ) > 0;
	
	TeamController *pTeams = pWorld->pTeamController;
	
	pTeams->StartEntityIterator ( pTeam );
	Entity *pOtherEntity = pTeams->GetNextEntity ( );

	while ( pOtherEntity )
	{
		if ( this->CanSee ( pOtherEntity ) && !this->IsTarget ( pOtherEntity ) )
		{
			if ( pOtherEntity->pTeam != pTeam && !pOtherEntity->IsNeutral( ) ) 
			{
				this->AddTarget ( pOtherEntity, false );
			}
		}

		pOtherEntity = pTeams->GetNextEntity ( );
	}

	pTeams->StartHeroIterator ( pTeam );
	Hero *pHero = pTeams->GetNextHero ( );

	while ( pHero )
	{
		if ( this->CanSee ( pHero ) && !this->IsTarget ( pHero ) )
		{
			if ( pHero->pTeam != pTeam )
			{
				this->AddTarget ( pHero, false );
			}
		}

		pHero = pTeams->GetNextHero ( );
	}

	return ( sTarget_list.size( ) > 0 );
}

int Entity::CountTargets ( )
{
	return (int) sTarget_list.size ( );
}

bool Entity::ValidTarget ( )
{
	if ( CountTargets ( ) == 0 ) return false;

	return ( sTarget_list [ 0 ].bCanSee );
}

bool Entity::IsTarget ( Entity *pOtherEntity )
{
	int iNumTargets = (int) sTarget_list.size ( );

	for ( int i = 0; i < iNumTargets; i++ )
	{
		if ( sTarget_list [ i ].pTargetEntity == pOtherEntity ) return true;
	}

	return false;
}

bool Entity::IsTarget ( Hero *pHero )
{
	int iNumTargets = (int) sTarget_list.size ( );

	for ( int i = 0; i < iNumTargets; i++ )
	{
		if ( sTarget_list [ i ].pTargetHero == pHero ) return true;
	}

	return false;
}

void Entity::AddTarget ( Entity *pOtherEntity, bool bFromZone )
{
	if ( !pOtherEntity ) return;
	//if ( pOtherEntity->pTeam == pTeam ) return;
	//if ( pOtherEntity->IsNeutral( ) ) return;
	if ( IsTarget ( pOtherEntity ) ) return;

	sTarget sNewTarget;
	sNewTarget.bZoneTarget = bFromZone;
	if ( bFromZone ) 
	{
		sNewTarget.fTargetThreat = 3.0f;
		sNewTarget.fTargetTimer = 60.0f;
	}
	else 
	{
		sNewTarget.fTargetThreat = 1.0f;
		sNewTarget.fTargetTimer = 10.0f;
	}

	sNewTarget.iTargetID = pOtherEntity->iID;
	sNewTarget.iTargetType = 1;
	sNewTarget.pTargetEntity = pOtherEntity;
	sNewTarget.pTargetHero = 0;
	sNewTarget.pTargetObject = 0;
	sNewTarget.bCanSee = !bFromZone;
	sNewTarget.iContainer = pOtherEntity->GetContainer( )->GetID( );
	
	sNewTarget.vecPos.x = pOtherEntity->GetX( );
	sNewTarget.vecPos.y = pOtherEntity->GetY( ) + pOtherEntity->GetHeight( );
	sNewTarget.vecPos.z = pOtherEntity->GetZ( );
	
	sNewTarget.vecLastPos = sNewTarget.vecPos;
	sNewTarget.vecGuessPos = sNewTarget.vecPos;
	
	sNewTarget.vecLastDir.x = 0;
	sNewTarget.vecLastDir.y = 0;
	sNewTarget.vecLastDir.z = 0;

	sTarget_list.push_back ( sNewTarget );
	sort ( sTarget_list.begin( ), sTarget_list.end( ) );
}

void Entity::AddTarget ( Hero *pHero, bool bFromZone )
{
	if ( !pHero ) return;
	//if ( pHero->pTeam == pTeam ) return;
	if ( IsTarget ( pHero ) ) return;

	sTarget sNewTarget;
	sNewTarget.bZoneTarget = bFromZone;
	if ( bFromZone ) 
	{
		sNewTarget.fTargetThreat = 3.0f;
		sNewTarget.fTargetTimer = 60.0f;
	}
	else 
	{
		sNewTarget.fTargetThreat = 1.0f;
		sNewTarget.fTargetTimer = 10.0f;
	}

	sNewTarget.iTargetID = pHero->iID;
	sNewTarget.iTargetType = 0;
	sNewTarget.pTargetEntity = 0;
	sNewTarget.pTargetHero = pHero;
	sNewTarget.pTargetObject = 0;
	sNewTarget.bCanSee = !bFromZone;
	sNewTarget.iContainer = pHero->GetContainer( )->GetID( );
	
	sNewTarget.vecPos.x = pHero->GetX( );
	sNewTarget.vecPos.y = pHero->GetY( ) + pHero->GetHeight( );
	sNewTarget.vecPos.z = pHero->GetZ( );
	
	sNewTarget.vecLastPos = sNewTarget.vecPos;
	sNewTarget.vecGuessPos = sNewTarget.vecPos;
	
	sNewTarget.vecLastDir.x = 0;
	sNewTarget.vecLastDir.y = 0;
	sNewTarget.vecLastDir.z = 0;

	sTarget_list.push_back ( sNewTarget );
	sort ( sTarget_list.begin( ), sTarget_list.end( ) );
}

void Entity::RemoveTarget ( Entity *pOtherEntity )
{
	for ( int i = 0; i < (int) sTarget_list.size ( ); i++ )
	{
		if ( sTarget_list [ i ].pTargetEntity == pOtherEntity )
		{
			sTarget_list.erase ( sTarget_list.begin( ) + i );
			i--;
		}
	}
}

void Entity::RemoveTarget ( Hero *pHero )
{
	for ( int i = 0; i < (int) sTarget_list.size ( ); i++ )
	{
		if ( sTarget_list [ i ].pTargetHero == pHero )
		{
			sTarget_list.erase ( sTarget_list.begin( ) + i );
			i--;
		}
	}
}

void Entity::UpdateTargets ( float fTimeDelta )
{
	CheckTargets ( );
	FindTargets ( );
	
	for ( int i = 0; i < (int) sTarget_list.size ( ); i++ )
	{
		float x, y, z;
		bool bTargetDucking = false;
		int iTargetContainer = pContainer->GetID( );

		switch ( sTarget_list [ i ].iTargetType )
		{
			case 0:		if ( !sTarget_list [ i ].pTargetHero )
						{
							sTarget_list.erase ( sTarget_list.begin( ) + i );
							i--;
							continue;
						}
						else
						{
							x = sTarget_list [ i ].pTargetHero->GetX( );
							y = sTarget_list [ i ].pTargetHero->GetY( ) + sTarget_list [ i ].pTargetHero->GetHeight( );
							z = sTarget_list [ i ].pTargetHero->GetZ( );
							bTargetDucking = sTarget_list [ i ].pTargetHero->GetIsDucking( );
							//bSameContainer = pContainer == sTarget_list [ i ].pTargetHero->GetContainer( );
							iTargetContainer = sTarget_list [ i ].pTargetHero->GetContainer( )->GetID( );
						}
						break;

			case 1:		if ( !sTarget_list [ i ].pTargetEntity )
						{
							sTarget_list.erase ( sTarget_list.begin( ) + i );
							i--;
							continue;
						}
						else
						{
							x = sTarget_list [ i ].pTargetEntity->GetX( );
							y = sTarget_list [ i ].pTargetEntity->GetY( ) + sTarget_list [ i ].pTargetEntity->GetHeight( );
							z = sTarget_list [ i ].pTargetEntity->GetZ( );
							bTargetDucking = sTarget_list [ i ].pTargetEntity->GetIsDucking( );
							//bSameContainer = pContainer == sTarget_list [ i ].pTargetEntity->GetContainer( );
							iTargetContainer = sTarget_list [ i ].pTargetEntity->GetContainer( )->GetID( );
						}
						break;

			default:	sTarget_list.erase ( sTarget_list.begin( ) + i );
						i--;
						continue;
		}

		bool bSameContainer = ( iTargetContainer == pContainer->GetID( ) );
		bool bCanSee = false;
				
		if ( pWorld->UsingGlobalVisibility( ) ) bCanSee = CanSee( x, y, z, bTargetDucking ) > 0;
		else bCanSee = (sTarget_list [ i ].bZoneTarget || bSameContainer ) && ( CanSee ( x, y, z, bTargetDucking ) > 0 );

		sTarget_list [ i ].bCanSee = bCanSee;

		float fDiffX = x - GetX(); 
		float fDiffY = y - GetY();
		float fDiffZ = z - GetZ();
		float fDist = fDiffX*fDiffX + fDiffY*fDiffY + fDiffZ*fDiffZ;

		if ( sTarget_list [ i ].bZoneTarget )
		{
			if ( bCanSee ) 
			{
				sTarget_list [ i ].fTargetThreat = sqrt(fDist);
				if ( sTarget_list [ i ].fTargetThreat > fRadius*99.0f ) sTarget_list [ i ].fTargetThreat = fRadius*99.0f;
			}
			else
			{
				sTarget_list [ i ].fTargetThreat = fRadius*99.9f;
			}

			sTarget_list [ i ].fTargetTimer = 60.0f;
			sTarget_list [ i ].vecLastPos = sTarget_list [ i ].vecPos;
			sTarget_list [ i ].vecPos.x = x;
			sTarget_list [ i ].vecPos.y = y;
			sTarget_list [ i ].vecPos.z = z;
			sTarget_list [ i ].vecGuessPos = sTarget_list [ i ].vecPos;
			sTarget_list [ i ].vecLastDir = sTarget_list [ i ].vecPos - sTarget_list [ i ].vecLastPos;
			sTarget_list [ i ].iContainer = iTargetContainer;
		}
		else
		{
			if ( bCanSee ) 
			{
				sTarget_list [ i ].fTargetThreat = sqrt(fDist);
				
				if ( bCanSearch && iAggressiveness == 1 ) sTarget_list [ i ].fTargetTimer = 20.0f;
				else sTarget_list [ i ].fTargetTimer = 6.0f + (rand()*1.0f)/RAND_MAX;

				sTarget_list [ i ].vecLastPos = sTarget_list [ i ].vecPos;
				sTarget_list [ i ].vecPos.x = x;
				sTarget_list [ i ].vecPos.y = y;
				sTarget_list [ i ].vecPos.z = z;
				sTarget_list [ i ].vecGuessPos = sTarget_list [ i ].vecPos;
				sTarget_list [ i ].vecLastDir = sTarget_list [ i ].vecPos - sTarget_list [ i ].vecLastPos;
				sTarget_list [ i ].iContainer = iTargetContainer;

				if ( sTarget_list [ i ].fTargetThreat > fRadius*99.0f ) sTarget_list [ i ].fTargetThreat = fRadius*99.0f;
			}
			else
			{
				if ( sTarget_list [ i ].fTargetThreat < fRadius*100.0f ) sTarget_list [ i ].fTargetThreat = fRadius*100.0f;
				sTarget_list [ i ].fTargetThreat += 0.01f;
				if ( sTarget_list [ i ].fTargetThreat > fRadius*1000.0f ) sTarget_list [ i ].fTargetThreat = fRadius*1000.0f;
			}
		}
	}

	for ( int i = 0; i < (int) sTarget_list.size ( ); i++ )
	{
		sTarget_list [ i ].fTargetTimer -= fTimeDelta;
		if ( sTarget_list [ i ].fTargetTimer <= 0 ) 
		{
			sTarget_list.erase ( sTarget_list.begin( ) + i );
			i--;
		}
	}

	sort ( sTarget_list.begin( ), sTarget_list.end( ) );

	bFireWeapon = false;
	if ( CanFire( ) && bCanAttack ) FireWeapon ( );
}

bool Entity::CanFire ( )
{	
	if ( !ValidTarget( ) ) return false;

	if ( !GetIsBehindCorner() )
	{	
		float x = GetTargetX( ) - GetX( );
		float z = GetTargetZ( ) - GetZ( );
		float fTargetAngY;

		if ( fabs ( x ) + fabs ( z ) < 0.1f ) 
		{
			return false;
		}
		else
		{
			fTargetAngY = z / sqrt ( x*x + z*z );
			fTargetAngY = acos ( fTargetAngY ) * RADTODEG;
			if ( x < 0.0f ) fTargetAngY = 360.0f - fTargetAngY;
		}
		
		float fArc = fabs ( fTargetAngY - GetAngleY( ) );
		if ( fArc > 180.0f ) fArc  = 360.0f - fArc;

		if ( fArc > fFireArc ) return false;
	}

	int iHeight = 1;
	bool bTargetDucking = false;

	if ( sTarget_list [ 0 ].iTargetType == 0 && sTarget_list [ 0 ].pTargetHero->GetIsDucking( ) ) bTargetDucking = true;
	if ( sTarget_list [ 0 ].iTargetType == 1 && sTarget_list [ 0 ].pTargetEntity->GetIsDucking( ) ) bTargetDucking = true;
	if ( GetIsDucking( ) || bTargetDucking ) iHeight = 2;

	return CanSee( GetTargetX( ), GetTargetY( ), GetTargetZ( ), bTargetDucking ) > 0;
	//return !( pContainer->pPathFinder->QuickPolygonsCheckVisible ( GetX( ), GetZ( ), GetTargetX( ), GetTargetZ( ), iHeight ) );
}

void Entity::SetGettingHit ( float fDirX, float fDirY, float fDirZ )
{
	bHit = true;

	float fDist = fDirX*fDirX + fDirZ*fDirZ;
	
	if ( fDist > 0.00001 )
	{
		fDist = sqrt ( fDist );

		sHitPoint sNewHitPoint;
		sNewHitPoint.x = -( fDirX / fDist );
		sNewHitPoint.y = 0.0f;
		sNewHitPoint.z = -( fDirZ / fDist );
		sNewHitPoint.angY = acos ( -fDirZ / fDist );
		sNewHitPoint.fLifeTime = 1.0f;

		sHitFrom_list.push_back ( sNewHitPoint );
	}

	sort ( sHitFrom_list.begin ( ), sHitFrom_list.end ( ) );
}

bool Entity::IsHit ( )
{
	//return ( bHit || CountHitPoints( ) > 0 );
	return bHit;
}

int Entity::CountHitPoints ( )
{
	return (int) sHitFrom_list.size ( );
}

GGVECTOR3 Entity::GetHitDir ( int iIndex )
{
	GGVECTOR3 vecDir ( 0.0f, 0.0f, 0.0f );
	
	if ( iIndex >= 0 && iIndex < (int) sHitFrom_list.size( ) )
	{
		vecDir.x = sHitFrom_list [ iIndex ].x;
		vecDir.y = sHitFrom_list [ iIndex ].y;
		vecDir.z = sHitFrom_list [ iIndex ].z;
	}

	return vecDir;
}

GGVECTOR3 Entity::GetHitDirAvg ( )
{
	GGVECTOR3 vecDir ( 0.0f, 0.0f, 0.0f );
	if ( CountHitPoints( ) == 0 ) return vecDir;

	vector < sHitPoint >::iterator hIter = sHitFrom_list.begin ( );
	vector < sHitPoint >::iterator hPrevIter = sHitFrom_list.end ( ) - 1;
	vector < sHitPoint >::iterator hBiggestIter = hIter;
	float fBiggestAngle = -1.0f;

	while ( hIter < sHitFrom_list.end ( ) )
	{
		float fHitAngY = hIter->angY - hPrevIter->angY;
		if ( fHitAngY <= 0.0f ) fHitAngY += 360.0f;

		if ( fHitAngY > fBiggestAngle )
		{
			fBiggestAngle = fHitAngY;
			hBiggestIter = hIter;
		}
		
		hPrevIter = hIter;
		hIter++;
	}

	if ( fBiggestAngle > 0.0f )
	{
		vecDir.x += hBiggestIter->x;
		vecDir.y += hBiggestIter->y;
		vecDir.z += hBiggestIter->z;

		if ( hBiggestIter == sHitFrom_list.begin ( ) ) hBiggestIter = sHitFrom_list.end ( ) - 1;
		else hBiggestIter--;

		vecDir.x += hBiggestIter->x;
		vecDir.y += hBiggestIter->y;
		vecDir.z += hBiggestIter->z;

		vecDir /= 2.0f;
	}
	
	if ( fBiggestAngle < 3.141592f )
	{
		vecDir = -vecDir;
	}

	float fDist = sqrt ( vecDir.x*vecDir.x + vecDir.z*vecDir.z );

	if ( fDist > 0.00001 ) vecDir /= fDist;
	else 
	{
		vecDir.x = 0.0f;
		vecDir.y = 0.0f;
		vecDir.z = 0.0f;
	}

	return vecDir;
}

float Entity::GetHitSpread ( )
{
	if ( (int) sHitFrom_list.size( ) == 0 ) return 0.0f;

	vector < sHitPoint >::iterator hIter = sHitFrom_list.begin ( );
	vector < sHitPoint >::iterator hPrevIter = sHitFrom_list.end ( ) - 1;
	vector < sHitPoint >::iterator hBiggestIter = hIter;
	float fBiggestAngle = -1.0f;

	while ( hIter < sHitFrom_list.end ( ) )
	{
		float fAngY = hIter->angY - hPrevIter->angY;
		if ( fAngY < 0.0f ) fAngY += 360.0f;

		if ( fAngY > fBiggestAngle )
		{
			fBiggestAngle = fAngY;
			hBiggestIter = hIter;
		}
	}
	
	if ( fBiggestAngle <= 0.0f ) return 0.0f;

	return 360.0f - fBiggestAngle;
}

void Entity::MarkOldPosition ( )
{
	vecOldPos.x = GetX ( );
	vecOldPos.y = GetY ( );
	vecOldPos.z = GetZ ( );
}

bool Entity::IsAllied ( Entity* pAskingEntity )
{
	return ( pTeam == pAskingEntity->pTeam );
}

bool Entity::IsEnemy ( Entity* pAskingEntity )
{
	return ( pTeam != pAskingEntity->pTeam );
}

bool Entity::IsNeutral ( )
{
	return ( pTeam == 0 );
}

bool Entity::IsFriendly ( )
{
	if ( !pTeam ) return false;
	return ( pTeam->GetTeamNum( ) == 1 );
}

void Entity::SetMaxSpeed ( float fNewSpeed )
{
	fSpeed = fNewSpeed;
}

void Entity::SetSpeed ( float fNewSpeed )
{
	if ( fNewSpeed < 0 ) fNewSpeed = 0;
	if ( fNewSpeed > fSpeed ) fNewSpeed = fSpeed;

	fCurrSpeed = fNewSpeed;
}

void Entity::SetTurnSpeed ( float fNewTurnSpeed )
{
	fTurnSpeed = fNewTurnSpeed;
}

void Entity::SetFireArc ( float fNewFireArc )
{
	if ( fNewFireArc < 0.0f ) fNewFireArc = 0.0f;
	if ( fNewFireArc > 180.0f ) fNewFireArc = 180.0f;
	
	fFireArc = fNewFireArc; 
}

void Entity::SetViewArc ( float fNewInnerViewArc, float fNewOuterViewArc )
{
	if ( fNewInnerViewArc < 0.0f ) fNewInnerViewArc = 0.0f;
	if ( fNewInnerViewArc > 180.0f ) fNewInnerViewArc = 180.0f;
	if ( fNewOuterViewArc > 180.0f ) fNewOuterViewArc = 180.0f;

	if ( fNewOuterViewArc < fNewInnerViewArc ) fNewOuterViewArc = fNewInnerViewArc;

	fInnerViewArc = fNewInnerViewArc;
	fOuterViewArc = fNewOuterViewArc;

	DebugHideViewArcs ( );
}

void Entity::SetViewRange ( float fNewViewRange )
{
	if ( fNewViewRange < 1.0f ) fNewViewRange = 1.0f;
	//if ( fNewViewRange > 100.0f ) fNewViewRange = 100.0f;

	fViewRange = fNewViewRange;

	DebugHideViewArcs ( );
}

void Entity::SetHearingRange ( float fNewHearingRange )
{
	if ( fNewHearingRange < 1.0f ) fNewHearingRange = 1.0f;

	fHearingRange = fNewHearingRange;

	DebugHideViewArcs ( );
}

void Entity::SetHearingThreshold ( int iNewThreshold )
{
	iHearingThreshold = iNewThreshold;
}

void Entity::SetNeutral ( bool bIsNeutral )
{
	if ( bIsNeutral )
	{
		if ( pTeam )
		{
			pTeam->RemoveMember ( this );
			pTeam = 0;
		}

		iAggressiveness = 3;
		bCanRoam = true;
	}
	else
	{
		iAggressiveness = 0;
	}
}

void Entity::SetDefendArea ( float fDist, int container )
{
	if ( fDist >= 0.0f ) fDefendDist = fDist;
	iAggressiveness = 0;
	bFollowingLeader = false;
	bDefending = true;

	iDefendContainer = container;

	SetIdlePos( vecDefendPos.x, vecDefendPos.y, vecDefendPos.z, container );
}

void Entity::SetDefendHero ( float fDist )
{
	if ( fDist >= 0.0f ) fDefendDist = fDist;
	iAggressiveness = 0;
	bFollowingLeader = true;
	bDefending = true;
}

bool Entity::InDefendArea ( float x, float y, float z )
{
	if ( iAggressiveness != 0 ) return false;
	
	float fDiffX = x - vecDefendPos.x;
	float fDiffY = y - vecDefendPos.y;
	float fDiffZ = z - vecDefendPos.z;
	float fDist = fDiffX*fDiffX + fDiffY*fDiffY + fDiffZ*fDiffZ;

	return ( fDist < fDefendDist*fDefendDist );
}

void Entity::SetDefendDist ( float fDist )
{
	if ( fDist < 0.0f ) fDist = 0.0f;

	fDefendDist = fDist;
}

float Entity::GetDefendDist ( )
{
	return fDefendDist;
}

void Entity::SetDefending ( bool bIsDefending )
{
	bDefending = bIsDefending;
	if ( !bDefending && bFollowingLeader ) bFollowingLeader = false;

	if ( !bDefending && iAggressiveness == 2 ) iAggressiveness = 0;
}

