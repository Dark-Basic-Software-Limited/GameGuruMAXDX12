#define WIN32_LEAN_AND_MEAN					// Exclude rarely-used stuff from Windows headers

#include <windows.h>
#include <stdio.h>
#include <vector>
#include ".\..\..\Shared\DBOFormat\DBOData.h"
#include ".\..\..\..\Include\DarkAI.h"

#include "globstruct.h"
extern GlobStruct *g_pGlob;

#include "Entity.h"
#include "World.h"
#include "Hero.h"
#include "Beacon.h"
#include "StateMachine\StateSet.h"
#include "TeamController.h"
#include "Zone.h"
#include "DBPro Functions.h"
#include <cctype>
#include <stdlib.h>
#include <time.h>

#include "CObjectsC.h"
#include "CMemblocks.h"

extern std::vector <int> ListOfEntitiesToUpdateMovement;
vector <tempCoverPoint> tempCoverPoints;
vector <float> coverInUseX;
vector <float> coverInUseZ;
vector <Entity*> coverInUseEntity;

int g_GUIShaderEffectID = 0;

//using namespace std;

bool bShowErrors = false;
bool bAIInitialised = false;
int iThreadCount = 4;
char errAIStr [ 256 ];
int iThreadMode = 0;
World cWorld;

// associated with AI Get Last Obstacle Center X()
float g_fShapeMidX = -1.0f;
float g_fShapeMidZ = -1.0f;
float g_fShapeRadius = -1.0f;

//DarkAI main funcitons
void AIStart ( int iGUIShaderEffectIndex )
{
	if ( bAIInitialised ) return;

	g_GUIShaderEffectID = iGUIShaderEffectIndex;
	
	srand ( (unsigned int) time (NULL) );
	AIReset ( 0 );

	bAIInitialised = true;
}

void AIReset ( int iStopTheadLoop ) 
{ 
	if ( iStopTheadLoop == 1 )
	{
		// signal to stop thead
		cWorld.StopWorld();
	}

	coverInUseX.clear();
	coverInUseZ.clear();
	coverInUseEntity.clear();
	tempCoverPoints.clear();
	cWorld.Reset ( );
	ListOfEntitiesToUpdateMovement.clear();
}

void AISetAvoidMode ( int iNewMode )
{
	if ( iNewMode < 0 ) iNewMode = 0;

	Entity::iAvoidMode = iNewMode;
}

inline Entity* CheckEntity ( int iObjID )
{
	Entity* pEntity = cWorld.GetEntityCopy ( iObjID );
	
	//Dave set bShowErrors to false and commented this out as everywhere that uses this function checks if it is null and returns out anyway
	/*if ( !pEntity && bShowErrors ) 
	{
		sprintf_s ( errAIStr, 255, "AI Entity (%d) Does Not Exist", iObjID );
		MessageBox ( NULL, errAIStr, "AI Error", 0 );
		//exit ( 1 );
	}*/

	return pEntity;
}

inline Container* CheckContainer ( int iContainerID )
{
	Container* pContainer = cWorld.GetContainer ( iContainerID );
	
	if ( !pContainer && bShowErrors ) 
	{
		sprintf_s ( errAIStr, 255, "AI Container (%d) Does Not Exist", iContainerID );
		MessageBox ( NULL, errAIStr, "AI Error", 0 );
		//exit ( 1 );
	}

	return pContainer;
}

inline bool CheckAIInit ( )
{
	if ( !bAIInitialised )
	{
		//MessageBox ( NULL, "AI System Not Initialised, Call 'AI Start' First", "AI Error", 0 );
		RunTimeError ( 0, "AI System Not Initialised, Call 'AI Start' First" );
		////exit ( 1 );
		return 0;
	}
	else
		return 1;
}

float AILastObstacleCenterX ( )
{
	float fReturn = g_fShapeMidX;
	g_fShapeMidX=-1;
	return fReturn;
}

float AILastObstacleCenterZ ( )
{
	float fReturn = g_fShapeMidZ;
	g_fShapeMidZ=-1;
	return fReturn;
}

float AILastObstacleRadius ( )
{
	float fReturn = g_fShapeRadius;
	g_fShapeRadius=-1;
	return fReturn;
}

void AIAddStaticObstacle ( int iObjID, int iHeight, int iContainerID ) 
{ 
	if ( CheckAIInit ( )==0 ) return;
	
	if ( ObjectExist( iObjID ) == 0 ) 
	{
		if ( bShowErrors )
		{
			sprintf_s ( errAIStr, 255, "Static Object (%d) Does Not Exist", iObjID );
			MessageBox ( NULL, errAIStr, "AI Error", 0 );
			//exit ( 1 );
		}

		return;
	}

	Container *pContainer = CheckContainer ( iContainerID );
	if ( !pContainer ) return;
	
	sObject *pObject = GetObjectData ( iObjID );
	
	cWorld.AddStatic ( iObjID, iContainerID, pObject, iHeight, 0 );
}

void AIAddStaticObstacle ( int iObjID, int iHeight ) 
{ 
	AIAddStaticObstacle ( iObjID, iHeight, 0 );
}

void AIAddStaticObstacle ( int iObjID ) 
{ 
	AIAddStaticObstacle ( iObjID, 1, 0 );
}

void AIAddViewBlockingObstacle ( int iObjID, int iHeight, int iContainerID ) 
{ 
	if ( CheckAIInit ( )==0 ) return;
	
	if ( ObjectExist( iObjID ) == 0 ) 
	{
		if ( bShowErrors )
		{
			sprintf_s ( errAIStr, 255, "Object (%d) Does Not Exist", iObjID );
			MessageBox ( NULL, errAIStr, "AI Error", 0 );
			//exit ( 1 );
		}

		return;
	}

	Container *pContainer = CheckContainer ( iContainerID );
	if ( !pContainer ) return;
	
	sObject *pObject = GetObjectData ( iObjID );
	
	cWorld.AddStatic ( iObjID, iContainerID, pObject, iHeight, 1 );
}

void AIAddViewBlockingObstacle ( int iObjID, int iHeight ) 
{ 
	AIAddViewBlockingObstacle ( iObjID, iHeight, 0 );
}

void AIAddViewBlockingObstacle ( int iObjID ) 
{ 
	AIAddViewBlockingObstacle ( iObjID, 1, 0 );
}

void AIRemoveObstacle ( int iObjID )
{
	if ( CheckAIInit ( )==0 ) return;

	cWorld.RemoveStatic ( iObjID );
}

void AIAddObstacleFromLevel ( int iObjID, int iContainerID, int iHeight, float fPlaneHeight, float fMinLength, float fMinHeight, int iOutputToFile )
{
	if ( CheckAIInit ( )==0 ) return;

	sObject *pObject = GetObjectData( iObjID );
	if ( !pObject ) return;

	cWorld.AddObstacleFromObject ( pObject, iContainerID, iHeight, fPlaneHeight, fMinLength, fMinHeight, ( iOutputToFile != 0 ) );
}

void AIAddObstacleFromLevel ( int iObjID, float fPlaneHeight, float fMinLength, float fMinHeight )
{
	AIAddObstacleFromLevel( iObjID, 0, 1, fPlaneHeight, fMinLength, fMinHeight, 0 );
}

void AIAddObstacleFromLevel ( int iObjID, int iContainerID, float fPlaneHeight, float fMinLength, float fMinHeight )
{
	AIAddObstacleFromLevel( iObjID, iContainerID, 1, fPlaneHeight, fMinLength, fMinHeight, 0 );
}

void AIAddObstacleFromLevel ( int iObjID, int iContainerID, int iHeight, float fPlaneHeight, float fMinLength, float fMinHeight )
{
	AIAddObstacleFromLevel( iObjID, iContainerID, iHeight, fPlaneHeight, fMinLength, fMinHeight, 0 );
}

//type: 0 = no obstruction, otherwise polygon
void AIAddAlternateVisibilityObject( int iObjID, int type )
{
	if ( CheckAIInit ( )==0 ) return;

	sObject *pObject = GetObjectData( iObjID );
	if ( !pObject ) return;

	cWorld.AddAlternateVisibilityObject( iObjID, pObject, type );
}

void AIRemoveAlternateVisibilityObject( int iObjID )
{
	if ( CheckAIInit ( )==0 ) return;

	cWorld.RemoveAlternateVisibilityObject( iObjID );
}

void AICompleteObstacles ( )
{
	if ( CheckAIInit ( )==0 ) return;

	cWorld.CompleteObstacles( -1 );
}

void AICompleteObstacles ( int iContainerID )
{
	if ( CheckAIInit ( )==0 ) return;

	cWorld.CompleteObstacles( iContainerID );
}

void AIAddPlayer ( int iObjID, int iUseObject ) 
{ 
	if ( CheckAIInit ( )==0 ) return;

	if ( cWorld.pTeamController->GetHero ( iObjID ) )
	{
		if ( bShowErrors )
		{
			sprintf_s ( errAIStr, 255, "AI Player (%d) Already Exists", iObjID );
			MessageBox ( NULL, errAIStr, "AI Error", 0 );
			//exit ( 1 );
		}

		return;
	}
	
	sObject *pObject = 0;
	
	if ( iUseObject > 0 )
	{
		if ( ObjectExist( iObjID ) == 0 ) 
		{
			if ( bShowErrors )
			{
				sprintf_s ( errAIStr, 255, "Player Object (%d) Does Not Exist", iObjID );
				MessageBox ( NULL, errAIStr, "AI Error", 0 );
				//exit ( 1 );
			}

			return;
		}

		pObject = GetObjectData ( iObjID );
	}

	cWorld.AddHero ( iObjID, 0, pObject, 1 );
}

void AIAddPlayer ( int iObjID ) 
{
	AIAddPlayer ( iObjID, 1 );
}

void AIAddFriendly ( int iObjID, int iUseObject, int iContainerID ) 
{ 
	if ( CheckAIInit ( )==0 ) return;

	if ( cWorld.pTeamController->GetEntity ( iObjID ) )
	{
		if ( bShowErrors )
		{
			sprintf_s ( errAIStr, 255, "Friendly (%d) Already Exists", iObjID );
			MessageBox ( NULL, errAIStr, "AI Error", 0 );
			//exit ( 1 );
		}

		return;
	}
	
	sObject *pObject = 0;
	
	if ( iUseObject > 0 )
	{
		if ( ObjectExist( iObjID ) == 0 )  
		{
			if ( bShowErrors )
			{
				sprintf_s ( errAIStr, 255, "Friendly Object (%d) Does Not Exist", iObjID );
				MessageBox ( NULL, errAIStr, "AI Error", 0 );
				//exit ( 1 );
			}

			return;
		}

		pObject = GetObjectData ( iObjID );
	}

	Container *pContainer = CheckContainer ( iContainerID );
	if ( !pContainer ) return;

	cWorld.AddEntity ( iObjID, iContainerID, pObject, GetObjectData ( iObjID ), 1 );
}

void AIAddFriendly ( int iObjID, int iUseObject ) 
{
	AIAddFriendly ( iObjID, iUseObject, 0 );
}

void AIAddFriendly ( int iObjID ) 
{
	AIAddFriendly ( iObjID, 1, 0 );
}

void AIAddNeutral ( int iObjID, int iUseObject, int iContainerID ) 
{ 
	if ( CheckAIInit ( )==0 ) return;

	if ( cWorld.pTeamController->GetEntity ( iObjID ) )
	{
		if ( bShowErrors )
		{
			sprintf_s ( errAIStr, 255, "Neutral (%d) Already Exists", iObjID );
			MessageBox ( NULL, errAIStr, "AI Error", 0 );
			//exit ( 1 );
		}

		return;
	}
	
	sObject *pObject = 0;
	
	if ( iUseObject > 0 )
	{
		if ( !ObjectExist( iObjID ) ) 
		{
			if ( bShowErrors )
			{
				sprintf_s ( errAIStr, 255, "Neutral Object (%d) Does Not Exist", iObjID );
				MessageBox ( NULL, errAIStr, "AI Error", 0 );
				//exit ( 1 );
			}

			return;
		}

		pObject = GetObjectData ( iObjID );
	}

	Container *pContainer = CheckContainer ( iContainerID );
	if ( !pContainer ) return;

	cWorld.AddEntity ( iObjID, iContainerID, pObject, GetObjectData ( iObjID ), 0 );
}

void AIAddNeutral ( int iObjID, int iUseObject ) 
{
	AIAddNeutral ( iObjID, iUseObject, 0 );
}

void AIAddNeutral ( int iObjID ) 
{
	AIAddNeutral ( iObjID, 1, 0 );
}

void AIAddEnemy ( int iObjID, int iUseObject, int iContainerID ) 
{ 
	if ( CheckAIInit ( )==0 ) return;

	if ( cWorld.pTeamController->GetEntity ( iObjID ) )
	{
		if ( bShowErrors )
		{
			sprintf_s ( errAIStr, 255, "Enemy (%d) Already Exists", iObjID );
			MessageBox ( NULL, errAIStr, "AI Error", 0 );
			//exit ( 1 );
		}

		return;
	}

	sObject *pObject = 0;

	if ( iUseObject > 0 )
	{
		if ( !ObjectExist( iObjID ) ) 
		{
			if ( bShowErrors )
			{
				sprintf_s ( errAIStr, 255, "Enemy Object (%d) Does Not Exist", iObjID );
				MessageBox ( NULL, errAIStr, "AI Error", 0 );
				//exit ( 1 );
			}

			return;
		}

		pObject = GetObjectData ( iObjID );
	}

	Container *pContainer = CheckContainer ( iContainerID );
	if ( !pContainer ) return;

	cWorld.AddEntity ( iObjID, iContainerID, pObject, GetObjectData ( iObjID ), 2 );
}

void AIAddEnemy ( int iObjID, int iUseObject ) 
{
	AIAddEnemy ( iObjID, iUseObject, 0 );
}

void AIAddEnemy ( int iObjID ) 
{
	AIAddEnemy ( iObjID, 1, 0 );
}

void AIStartNewObstacle ( int id )
{
	if ( CheckAIInit ( )==0 ) return;
	if ( cWorld.pManualPolygon )
	{
		if ( bShowErrors )
		{
			sprintf_s ( errAIStr, 255, "Cannot Start New Obstacle, Previous Has Not Been Finished" );
			MessageBox ( NULL, errAIStr, "AI Error", 0 );
		}
		return;
	}
	cWorld.StartNewPolygon ( id );
}

void AIStartNewObstacle ( )
{
	AIStartNewObstacle ( 0 );
}

void AIAddObstacleVertex ( float x, float z )
{
	if ( CheckAIInit ( )==0 ) return;

	if ( !cWorld.pManualPolygon )
	{
		if ( bShowErrors )
		{
			sprintf_s ( errAIStr, 255, "Cannot Add Vertex, New Obstacle Has Not Been Started" );
			MessageBox ( NULL, errAIStr, "AI Error", 0 );
			////exit ( 1 );
		}

		return;
	}

	cWorld.AddVertex ( x, z );
}

void AIEndNewObstacle ( int iContainerID, int iHeight, int iObstacleType )
{
	if ( CheckAIInit ( )==0 ) return;

	Container *pContainer = CheckContainer ( iContainerID );
	
	if ( !cWorld.pManualPolygon )
	{
		if ( bShowErrors )
		{
			sprintf_s ( errAIStr, 255, "Cannot End New Obstacle, New Obstacle Has Not Been Started" );
			MessageBox ( NULL, errAIStr, "AI Error", 0 );
			//exit ( 1 );
		}

		return;
	}

	cWorld.EndNewPolygon ( iContainerID, iHeight, iObstacleType );
}

void AIEndNewObstacle ( int iContainerID, int iHeight )
{
	AIEndNewObstacle ( iContainerID, iHeight, 0 );
}

void AIDiscardNewObstacle ( )
{
	if ( CheckAIInit ( )==0 ) return;

	if ( !cWorld.pManualPolygon )
	{
		if ( bShowErrors )
		{
			sprintf_s ( errAIStr, 255, "Cannot Discard New Obstacle, New Obstacle Has Not Been Started" );
			MessageBox ( NULL, errAIStr, "AI Error", 0 );
			//exit ( 1 );
		}

		return;
	}

	cWorld.DiscardNewPolygon ( );
}

void AIAddCoverPoint( float fx, float fy, float fz, float angle, LPSTR pStringPtr )
{
	tempCoverPoint cp;
	cp.fx = fx;
	cp.fy = fy;
	cp.fz = fz;
	cp.angle = angle;
	if ( pStringPtr )
	{
		if ( strlen ( pStringPtr ) < 64 )
			strcpy ( cp.pCoverName, pStringPtr );
		else
			strcpy ( cp.pCoverName, "<string too long>" );
	}
	else
		strcpy ( cp.pCoverName, "" );
		
	tempCoverPoints.push_back(cp);
}
tempCoverPoint* GetCoverPointPtr ( int iFindIndex )
{
	if ( iFindIndex >= 1 && iFindIndex <= (int)tempCoverPoints.size() )
		return &tempCoverPoints[iFindIndex-1];
	else
		return NULL;
}
int AIGetTotalCover()
{
	return (int)tempCoverPoints.size();
}
float AICoverGetPointX ( int iIndex )
{
	tempCoverPoint* pCoverPtr = GetCoverPointPtr ( iIndex );
	if ( pCoverPtr )
		return pCoverPtr->fx;
	else
		return 0.0f;
}
float AICoverGetPointY ( int iIndex )
{
	tempCoverPoint* pCoverPtr = GetCoverPointPtr ( iIndex );
	if ( pCoverPtr )
		return pCoverPtr->fy;
	else
		return 0.0f;
}
float AICoverGetPointZ ( int iIndex )
{
	tempCoverPoint* pCoverPtr = GetCoverPointPtr ( iIndex );
	if ( pCoverPtr )
		return pCoverPtr->fz;
	else
		return 0.0f;
}
float AICoverGetAngle ( int iIndex )
{
	tempCoverPoint* pCoverPtr = GetCoverPointPtr ( iIndex );
	if ( pCoverPtr )
		return pCoverPtr->angle;
	else
		return 0.0f;
}
LPSTR AICoverGetIfUsed ( int iIndex )
{
	tempCoverPoint* pCoverPtr = GetCoverPointPtr ( iIndex );
	if ( pCoverPtr )
		return pCoverPtr->pCoverName;
	else
		return "";
}

void AIAddContainer ( int iContainerID )
{
	if ( CheckAIInit ( )==0 ) return;
	Container *pContainer = cWorld.GetContainer ( iContainerID );
	if ( pContainer )
	{
		if ( bShowErrors )
		{
			sprintf_s ( errAIStr, "AI Container (%d) Already Exists", iContainerID );
			MessageBox ( NULL, errAIStr, "AI Error", 0 );
		}
		return;
	}
	cWorld.AddContainer ( iContainerID );
}

int AIConnectContainers( int iFromContainer, float x, float z, int iToContainer, float x2, float z2 )
{
	CheckAIInit( );

	Container *pFromContainer = cWorld.GetContainer ( iFromContainer );
	
	if ( !pFromContainer )
	{
		if ( bShowErrors )
		{
			sprintf_s ( errAIStr, "AI Container (%d) Does Not Exist", iFromContainer );
			MessageBox ( NULL, errAIStr, "AI Error", 0 );
			//exit ( 1 );
		}

		return 1;
	}

	Container *pContainer = cWorld.GetContainer ( iToContainer );
	
	if ( !pContainer )
	{
		if ( bShowErrors )
		{
			sprintf_s ( errAIStr, "AI Container (%d) Does Not Exist", iToContainer );
			MessageBox ( NULL, errAIStr, "AI Error", 0 );
			//exit ( 1 );
		}

		return 1;
	}

	return cWorld.ConnectContainers( iFromContainer, x,z, iToContainer, x2,z2 );
}

void AIRemoveContainer ( int iContainerID )
{
	if ( CheckAIInit ( )==0 ) return;

	if ( iContainerID == 0 )
	{
		if ( bShowErrors )
		{
			sprintf_s ( errAIStr, "Cannot Remove AI Container %d", iContainerID );
			MessageBox ( NULL, errAIStr, "AI Error", 0 );
			//exit ( 1 );
		}

		return;
	}

	Container *pContainer = CheckContainer ( iContainerID );
	if ( !pContainer ) return;

	cWorld.RemoveContainer ( iContainerID );
}

int AIContainerExist ( int iContainerID )
{
	if ( CheckAIInit ( )==0 ) return 0;

	Container* pContainer = cWorld.GetContainer ( iContainerID );
	
	return pContainer ? 1 : 0;
}

void AISetContainerActive ( int iContainerID, int iActive )
{
	if ( CheckAIInit ( )==0 ) return;

	Container* pContainer = CheckContainer ( iContainerID );
	if ( !pContainer ) return;

	( iActive != 0 ) ? pContainer->Activate( ) : pContainer->DeActivate( );
}

void AIAddZone ( int iID, float minx, float minz, float maxx, float maxz, int iContainerID )
{
	if ( CheckAIInit ( )==0 ) return;

	Container *pContainer = 0;

	if ( iContainerID >= 0 )
	{
		pContainer = CheckContainer ( iContainerID );
		if ( !pContainer ) return;
	}

	cWorld.AddZone ( iID, minx, minz, maxx, maxz, pContainer );
}

void AIAddZone ( int iID, float minx, float minz, float maxx, float maxz )
{
	AIAddZone ( iID, minx, minz, maxx, maxz, -1 );
}

void AIRemoveZone ( int iID )
{
	if ( CheckAIInit ( )==0 ) return;

	cWorld.DeleteZone ( iID );
}

int AIZoneExist( int iID )
{
	if ( CheckAIInit ( )==0 ) return 0;

	Zone *pZone = cWorld.GetZone( iID );
	
	return pZone ? 1 : 0;
}

void AIEntityAssignZone ( int iEntityID, int iZoneID )
{
	if ( CheckAIInit ( )==0 ) return;

	Zone *pZone = cWorld.GetZone ( iZoneID );

	if ( !pZone ) 
	{
		if ( bShowErrors )
		{
			sprintf_s ( errAIStr, 255, "Zone (%d) Does Not Exist", iZoneID );
			MessageBox ( NULL, errAIStr, "AI Error", 0 );
			//exit ( 1 );
		}

		return;
	}

	Entity *pEntity = CheckEntity ( iEntityID );
	if ( !pEntity ) return;

	pZone->AddEntity ( pEntity );
}

void AIEntityRemoveZone ( int iEntityID, int iZoneID )
{
	if ( CheckAIInit ( )==0 ) return;

	Zone *pZone = cWorld.GetZone ( iZoneID );

	if ( !pZone ) 
	{
		sprintf_s ( errAIStr, 255, "Zone (%d) Does Not Exist", iZoneID );
		MessageBox ( NULL, errAIStr, "AI Error", 0 );
		//exit ( 1 );
		return;
	}

	Entity *pEntity = CheckEntity ( iEntityID );
	if ( !pEntity ) return;

	pZone->RemoveEntity ( pEntity );
}

void AIKillEntity ( int iEntityID )
{
	if ( CheckAIInit ( )==0 ) return;
	cWorld.DeleteEntity ( iEntityID );
}

void AIKillPlayer ( )
{
	CheckAIInit( );

	cWorld.DeletePlayer( );
}

void AIMakePath ( int iPathID )
{
	if ( CheckAIInit ( )==0 ) return;

	int iResult = cWorld.AddPath ( iPathID );

	if ( iResult == 1 )
	{
		if ( bShowErrors )
		{
			sprintf_s ( errAIStr, "Path ID (%d) Is Out Of Range (1-%d)", iPathID, cWorld.GetMaxPath( ) );
			MessageBox ( NULL, errAIStr, "AI Error", 0 );
			//exit ( 1 );
		}

		return;
	}

	if ( iResult == 2 )
	{
		if ( bShowErrors )
		{
			sprintf_s ( errAIStr, "Path (%d) Already Exists", iPathID );
			MessageBox ( NULL, errAIStr, "AI Error", 0 );
			//exit ( 1 );
		}

		return;
	}
}

void AIDeletePath ( int iPathID )
{
	if ( CheckAIInit ( )==0 ) return;

	int iResult = cWorld.DeletePath ( iPathID );

	if ( iResult == 1 )
	{
		if ( bShowErrors )
		{
			sprintf_s ( errAIStr, "Path ID (%d) Is Out Of Range (1-%d)", iPathID, cWorld.GetMaxPath( ) );
			MessageBox ( NULL, errAIStr, "AI Error", 0 );
			//exit ( 1 );
		}

		return;
	}

	if ( iResult == 2 )
	{
		if ( bShowErrors )
		{
			sprintf_s ( errAIStr, "Path (%d) Does Not Exist", iPathID );
			MessageBox ( NULL, errAIStr, "AI Error", 0 );
			//exit ( 1 );
		}

		return;
	}
}

void AIPathAddPoint ( int iPathID, float x, float y, float z )
{
	AIPathAddPoint ( iPathID, x, y, z, -1 );
}

void AIPathAddPoint ( int iPathID, float x, float y, float z, int container )
{
	if ( CheckAIInit ( )==0 ) return;
	int iResult = cWorld.AddPathPoint ( iPathID, x, y, z, container );
	if ( iResult == 1 )
	{
		if ( bShowErrors )
		{
			sprintf_s ( errAIStr, "Path ID (%d) Is Out Of Range (1-%d)", iPathID, cWorld.GetMaxPath( ) );
			MessageBox ( NULL, errAIStr, "AI Error", 0 );
		}
		return;
	}
	if ( iResult == 2 )
	{
		if ( bShowErrors )
		{
			sprintf_s ( errAIStr, "Path (%d) Does Not Exist", iPathID );
			MessageBox ( NULL, errAIStr, "AI Error", 0 );
		}
		return;
	}
}

int AIPathCountPoints ( int iPathID )
{
	if ( CheckAIInit ( )==0 ) return 0;

	return cWorld.CountPathPoints ( iPathID );
}

void AIMakeMemblockFromPath ( int iMemblockID, int iPathID )
{
	if ( CheckAIInit ( )==0 ) return;

	/*
	if ( !MemblockExist )
	{
		if ( bShowErrors )
		{
			MessageBox ( NULL, "Include At Least One (non-AI) Memblock Command In Your Code To Use The AI Memblock Commands", "AI Error", 0 );
			exit(1);
		}

		return;
	}*/
	
	int iResult = cWorld.MakeMemblockFromPath ( iMemblockID, iPathID );

	if ( iResult == 1 )
	{
		if ( bShowErrors )
		{
			sprintf_s ( errAIStr, "Path (%d) Does Not Exist", iPathID );
			MessageBox ( NULL, errAIStr, "AI Error", 0 );
			//exit ( 1 );
		}

		return;
	}
}

void AIMakePathFromMemblock ( int iPathID, int iMemblockID )
{
	if ( CheckAIInit ( )==0 ) return;

	/*
	if ( !MemblockExist )
	{
		if ( bShowErrors )
		{
			MessageBox ( NULL, "Include At Least One (non-AI) Memblock Command In Your Code To Use The AI Memblock Commands", "AI Error", 0 );
			exit(1);
		}

		return;
	}
	*/
	
	int iResult = cWorld.MakePathFromMemblock ( iPathID, iMemblockID );

	if ( iResult == 1 )
	{
		if ( bShowErrors )
		{
			sprintf_s ( errAIStr, "Path ID (%d) Is Out Of Range (0-%d)", iPathID, cWorld.GetMaxPath( ) );
			MessageBox ( NULL, errAIStr, "AI Error", 0 );
			//exit ( 1 );
		}

		return;
	}

	if ( iResult == 2 )
	{
		if ( bShowErrors )
		{
			sprintf_s ( errAIStr, "Path (%d) Already Exists", iPathID );
			MessageBox ( NULL, errAIStr, "AI Error", 0 );
			//exit ( 1 );
		}

		return;
	}

	if ( iResult == 3 )
	{
		if ( bShowErrors )
		{
			sprintf_s ( errAIStr, "Memblock (%d) Does Not Exist", iMemblockID );
			MessageBox ( NULL, errAIStr, "AI Error", 0 );
			//exit ( 1 );
		}

		return;
	}
}

void AIMakePathBetweenPoints ( int iPathID, int iContainerID, float x, float z, float x2, float z2, float fMaxEdgeCost, int destContainer )
{
	if ( CheckAIInit ( )==0 ) return;

	int iResult = cWorld.AddObstaclePath ( iPathID, iContainerID, x, z, x2, z2, fMaxEdgeCost, destContainer );

	if ( iResult == 1 )
	{
		if ( bShowErrors )
		{
			sprintf_s ( errAIStr, "Path ID (%d) Is Out Of Range (0-%d)", iPathID, cWorld.GetMaxPath( ) );
			MessageBox ( NULL, errAIStr, "AI Error", 0 );
			//exit ( 1 );
		}

		return;
	}

	if ( iResult == 2 )
	{
		if ( bShowErrors )
		{
			sprintf_s ( errAIStr, "Path (%d) Already Exists", iPathID );
			MessageBox ( NULL, errAIStr, "AI Error", 0 );
			//exit ( 1 );
		}

		return;
	}
}

void AIMakePathBetweenPoints ( int iPathID, int iContainerID, float x, float z, float x2, float z2, float fMaxEdgeCost )
{
	AIMakePathBetweenPoints( iPathID, iContainerID, x, z, x2, z2, -1, iContainerID );
}

void AIMakePathBetweenPoints ( int iPathID, int iContainerID, float x, float z, float x2, float z2 )
{
	AIMakePathBetweenPoints( iPathID, iContainerID, x, z, x2, z2, -1, iContainerID );
}

void AIMakePathBetweenPoints ( int iPathID, float x, float z, float x2, float z2 )
{
	AIMakePathBetweenPoints ( iPathID, 0, x, z, x2, z2, -1, 0 );
}

void AIMakePathFromClosestWaypoints ( int iPathID, int iContainerID, float x, float z )
{
	if ( CheckAIInit ( )==0 ) return;

	Container *pContainer = CheckContainer ( iContainerID );
	if ( pContainer ) return;

	int iResult = cWorld.AddPath ( iPathID );
	
	if ( iResult == 1 )
	{
		if ( bShowErrors )
		{
			sprintf_s ( errAIStr, "Path (%d) Out Of Range (1-1024)", iPathID );
			MessageBox ( NULL, errAIStr, "AI Error", 0 );
			//exit ( 1 );
		}

		return;
	}

	if ( iResult == 2 )
	{
		if ( bShowErrors )
		{
			sprintf_s ( errAIStr, "Path (%d) Already Exists", iPathID );
			MessageBox ( NULL, errAIStr, "AI Error", 0 );
			//exit ( 1 );
		}

		return;
	}
	
	Path *pPath = cWorld.GetPath ( iPathID );
	pContainer->pPathFinder->SearchPoints ( x, z, pPath, 2 );
}

void AIEntityAssignPatrolPath ( int iEntityID, int iPathID )
{
	if ( CheckAIInit ( )==0 ) return;

	Entity *pEntity = CheckEntity ( iEntityID );
	if ( !pEntity ) return;

	int iResult = cWorld.AssignEntityToPatrolPath ( pEntity, iPathID );
	
	if ( iResult == 1 )
	{
		if ( bShowErrors )
		{
			sprintf_s ( errAIStr, "AI Path (%d) Does Not Exist", iPathID );
			MessageBox ( NULL, errAIStr, "AI Error", 0 );
			//exit ( 1 );
		}

		return;
	}

	if ( iResult == 2 )
	{
		if ( bShowErrors )
		{
			sprintf_s ( errAIStr, "AI Path (%d) Is Out Of Range (1-1024)", iPathID );
			MessageBox ( NULL, errAIStr, "AI Error", 0 );
			//exit ( 1 );
		}

		return;
	}

	pEntity->SetDefending ( false );
}

void AIEntityAddTarget( int iEntityID, int iTargetID )
{
	CheckAIInit( );

	Entity *pEntity = CheckEntity( iEntityID );
	if ( !pEntity ) return;

	Entity* pTargetEntity = cWorld.GetEntityCopy ( iTargetID );
	if ( pTargetEntity ) 
	{
		pEntity->AddTarget( pTargetEntity, false );
	}
	else
	{
		Hero* pTargetHero = cWorld.GetHeroCopy( iTargetID );
		if ( pTargetHero )
		{
			pEntity->AddTarget( pTargetHero, false );
		}
	}
}

void AIEntityRemoveTarget( int iEntityID, int iTargetID )
{
	CheckAIInit( );

	Entity *pEntity = CheckEntity( iEntityID );
	if ( !pEntity ) return;

	Entity* pTargetEntity = cWorld.GetEntityCopy ( iTargetID );
	if ( pTargetEntity ) 
	{
		pEntity->RemoveTarget( pTargetEntity );
	}
	else
	{
		Hero* pTargetHero = cWorld.GetHeroCopy( iTargetID );
		if ( pTargetHero )
		{
			pEntity->RemoveTarget( pTargetHero );
		}
	}
}

void AISetEntityCanSelectTargets( int iEntityID, int iCanSelect )
{
	CheckAIInit( );

	Entity *pEntity = CheckEntity( iEntityID );
	if ( !pEntity ) return;

	pEntity->SetCanSelectTargets( iCanSelect != 0 );
}

int AIGetEntityInZone ( int iEntityID, int iZoneID )
{
	if ( CheckAIInit ( )==0 ) return 0;

	Entity *pEntity = CheckEntity ( iEntityID );
	if ( !pEntity ) return 0;
	Zone *pZone = cWorld.GetZone ( iZoneID );

	if ( !pZone )
	{
		if ( bShowErrors )
		{
			sprintf_s ( errAIStr, 255, "Zone (%d) Does Not Exist", iZoneID );
			MessageBox ( NULL, errAIStr, "AI Error", 0 );
			//exit ( 1 );
		}

		return 0;
	}

	bool bInZone = pZone->InZone ( pEntity->GetX( ), pEntity->GetZ( ), pEntity->GetContainer( ) );
	return bInZone ? 1 : 0;
}

int AIGetPlayerInZone ( int iZoneID )
{
	if ( CheckAIInit ( )==0 ) return 0;

	Hero *pHero = cWorld.pTeamController->GetPlayer( );
	if ( !pHero ) 
	{
		if ( bShowErrors )
		{
			sprintf_s(errAIStr, 255, "Player Does Not Exist"); // , iZoneID );
			MessageBox ( NULL, errAIStr, "AI Error", 0 );
			//exit ( 1 );
		}

		return 0;
	}

	Zone *pZone = cWorld.GetZone ( iZoneID );

	if ( !pZone )
	{
		if ( bShowErrors )
		{
			sprintf_s ( errAIStr, 255, "Zone (%d) Does Not Exist", iZoneID );
			MessageBox ( NULL, errAIStr, "AI Error", 0 );
			//exit ( 1 );
		}

		return 0;
	}

	bool bInZone = pZone->InZone ( pHero->GetX( ), pHero->GetZ( ), pHero->GetContainer( ) );
	return bInZone ? 1 : 0;
}

void AISetEntitySpeed ( int iObjID, float fNewSpeed ) 
{ 
	if ( CheckAIInit ( )==0 ) return;

	Entity* pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return;

	pEntity->SetMaxSpeed ( fNewSpeed );
}

void AISetEntityMaxPathStartCost( int iObjID, float fNewDistance )
{
	if ( CheckAIInit ( )==0 ) return;

	Entity* pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return;

	pEntity->SetPathStartCostLimit( fNewDistance );
}

void AISetEntityTurnSpeed ( int iObjID, float fNewSpeed ) 
{ 
	if ( CheckAIInit ( )==0 ) return;

	Entity* pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return;

	pEntity->SetTurnSpeed ( fNewSpeed );
}

void AISetEntityPatrolTime ( int iObjID, float fNewTime )
{
	if ( CheckAIInit ( )==0 ) return;

	Entity* pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return;

	pEntity->SetAvgPatrolTime ( fNewTime );
}

void AISetEntityStance ( int iObjID, int iNewValue ) 
{ 
	if ( CheckAIInit ( )==0 ) return;

	Entity* pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return;

	pEntity->SetDefending ( false );
	pEntity->SetAggressiveness( iNewValue );
}

void AISetEntityAggressive ( int iObjID )
{
	if ( CheckAIInit ( )==0 ) return;

	Entity* pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return;

	pEntity->SetDefending ( false );
	pEntity->SetAggressiveness( 1 );
}

void AISetEntityMeleeMode( int iObjID, int mode )
{
	CheckAIInit( );

	Entity* pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return;

	pEntity->SetMeleeMode( mode > 0 );
}

void AISetEntityActive ( int iObjID, int iActive )
{
	if ( CheckAIInit ( )==0 ) return;

	Entity* pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return;

	pEntity->SetActive( iActive != 0 );
}

void AISetEntityRunAwayWhenHit ( int iObjID )
{
	if ( CheckAIInit ( )==0 ) return;

	Entity* pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return;

	pEntity->SetDefending ( false );
	pEntity->SetAggressiveness( 3 );
}

void AIEntityHoldPosition ( int iObjID )
{
	if ( CheckAIInit ( )==0 ) return;

	Entity* pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return;
	
	pEntity->Stop ( );
	pEntity->SetDefending ( false );
	pEntity->SetAggressiveness( 2 );
}

void AIStayWithinContainer ( int iObjID, float fLastX, float fLastZ, float* pX, float* pZ )
{
	if ( CheckAIInit ( )==0 ) return;
	Entity* pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return;
	pEntity->StayWithinContainer ( fLastX, fLastZ, pX, pZ );
}

void AISetEntityPosition ( int iObjID, float x, float z )
{
	/*
	if ( CheckAIInit ( )==0 ) return;

	Entity* pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return;

	pEntity->SetPosition ( x, 0, z );
	*/
	MessageBox( NULL, "The command \"AI Set Entity Position <id>,<x>,<z>\" has been removed, a y value must now be specified as well", "AI Error", 0 );
	exit(-1);
}

//the distance from the object's center to the entity's head, used for line of sight checks
void AISetEntityHeight( int iObjID, float height )
{
	if ( CheckAIInit ( )==0 ) return;

	Entity* pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return;

	pEntity->SetHeight( height );
}

void AISetEntityPosition ( int iObjID, float x, float y, float z )
{
	if ( CheckAIInit ( )==0 ) return;
	Entity* pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return;
	pEntity->SetPosition ( x, y, z );
}

void AISetEntityYPosition ( int iObjID, float y )
{
	if ( CheckAIInit ( )==0 ) return;

	Entity* pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return;

	pEntity->SetYPosition( y );
}

void AISetEntityAngleY( int iObjID, float angY )
{
	CheckAIInit( );

	Entity* pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return;

	pEntity->SetAngleY ( angY );
}

void AISetPlayerPosition ( float x, float z )
{
	MessageBox ( NULL, "\"AI Set Player Position <x>,<z>\" now requires a Y value", "AI Error", 0 );
	exit(-1);
}

void AISetPlayerPosition ( float x, float y, float z )
{
	if ( CheckAIInit ( )==0 ) return;

	Hero *pHero = cWorld.pTeamController->GetPlayer ( );

	if ( !pHero )
	{
		if ( bShowErrors )
		{
			MessageBox ( NULL, "Player has not been added to the AI system", "AI Error", 0 );
			//exit ( 1 );
		}

		return;
	}

	pHero->SetPosition ( x, y, z );
}

void AISetPlayerAngleY( float angY )
{
	CheckAIInit( );

	Hero* pHero = cWorld.pTeamController->GetPlayer( );
	if ( !pHero )
	{
		if ( bShowErrors )
		{
			MessageBox ( NULL, "Player has not been added to the AI system", "AI Error", 0 );
			//exit ( 1 );
		}

		return;
	}

	pHero->SetAngleY ( angY );
}

void AISetPlayerContainer ( int iContainerID )
{
	if ( CheckAIInit ( )==0 ) return;

	Hero *pHero = cWorld.pTeamController->GetPlayer ( );

	if ( !pHero )
	{
		if ( bShowErrors )
		{
			MessageBox ( NULL, "Player has not been added to the AI system", "AI Error", 0 );
			//exit ( 1 );
		}

		return;
	}

	Container *pContainer = CheckContainer ( iContainerID );
	if ( !pContainer ) 
	{
		if ( bShowErrors )
		{
			MessageBox ( NULL, "Container does not exist", "AI Error", 0 );
			//exit ( 1 );
		}

		return;
	}

	pHero->SetContainer ( pContainer );
}

void AISetEntityContainer ( int iObjID, int iContainerID )
{
	if ( CheckAIInit ( )==0 ) return;

	Entity *pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return;

	Container *pContainer = CheckContainer ( iContainerID );
	if ( !pContainer ) return;

	pEntity->SetContainer ( pContainer );
}

int AIGetEntityContainer ( int iObjID )
{
	if ( CheckAIInit ( )==0 ) return 0;

	Entity *pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return -2;

	Container *pContainer = pEntity->GetContainer( );
	if ( !pContainer ) return -1;

	if ( pEntity->bChangingContainers == true )
		return -3;

	return pContainer->GetID( );
}

//specify an angle of 360 for all round
void AISetEntityViewArc ( int iObjID, float fNewValue, float fNewValue2 ) 
{ 
	if ( CheckAIInit ( )==0 ) return;

	Entity* pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return;
	
	fNewValue  /= 2.0f; //convert between the two descriptions
	fNewValue2 /= 2.0f;
	pEntity->SetViewArc ( fNewValue, fNewValue2 );	//specify and angle of 180 for all round
}

void AISetEntityViewRange ( int iObjID, float fNewValue ) 
{ 
	if ( CheckAIInit ( )==0 ) return;

	Entity* pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return;

	pEntity->SetViewRange ( fNewValue );
}

void AISetEntityHearingRange ( int iObjID, float fNewValue )
{
	if ( CheckAIInit ( )==0 ) return;

	Entity* pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return;

	pEntity->SetHearingRange ( fNewValue );
}

void AISetEntityHearingThreshold ( int iObjID, int iNewValue )
{
	if ( CheckAIInit ( )==0 ) return;

	Entity* pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return;

	pEntity->SetHearingThreshold ( iNewValue );
}

void AISetEntityFireArc ( int iObjID, float fNewValue ) 
{ 
	if ( CheckAIInit ( )==0 ) return;

	Entity* pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return;

	pEntity->SetFireArc ( fNewValue / 2.0f );
}

void AISetEntityAttackDistance( int iObjID, float fNewDist )
{
	CheckAIInit( );

	Entity* pEntity = CheckEntity ( iObjID );
	pEntity->SetAttackDist( fNewDist );
}

void AISetEntityAvoidDistance( int iObjID, float fNewDist )
{
	CheckAIInit( );

	Entity* pEntity = CheckEntity ( iObjID );
	if ( pEntity ) pEntity->SetAvoidDist( fNewDist );
}

void AISetEntityAvoidMode( int iObjID, int iNewMode )
{
	CheckAIInit( );

	Entity* pEntity = CheckEntity ( iObjID );
	if ( pEntity ) pEntity->SetAvoidMode( iNewMode );
}

void AISetEntityAlwaysActive( int iObjID, bool flag )
{
	CheckAIInit( );
	Entity* pEntity = CheckEntity ( iObjID );
	if ( pEntity ) pEntity->SetAlwaysActive( flag );
}

int AIGetEntityAvoidMode( int iObjID )
{
	CheckAIInit( );

	Entity* pEntity = CheckEntity ( iObjID );
	if ( pEntity ) 
		return pEntity->GetAvoidMode( );
	else
		return 0;
}

void AISetEntityCanDuck ( int iObjID, int iMode )
{
	if ( CheckAIInit ( )==0 ) return;

	Entity* pEntity = CheckEntity ( iObjID );
	if ( pEntity ) pEntity->SetCanDuck( iMode > 0 );
}

void AISetEntityCanAttack ( int iObjID, int iMode )
{
	if ( CheckAIInit ( )==0 ) return;

	Entity* pEntity = CheckEntity ( iObjID );
	if ( pEntity ) pEntity->SetCanAttack( iMode > 0 );
}

void AISetEntityCanStrafe ( int iObjID, int iMode )
{
	if ( CheckAIInit ( )==0 ) return;

	Entity* pEntity = CheckEntity ( iObjID );
	if ( pEntity ) pEntity->SetCanStrafe( iMode > 0 );
}

void AISetEntityCanHear ( int iObjID, int iMode )
{
	if ( CheckAIInit ( )==0 ) return;

	Entity* pEntity = CheckEntity ( iObjID );
	if ( pEntity ) pEntity->SetCanHear( iMode > 0 );
}

void AISetEntityCanRoam ( int iObjID, int iMode )
{
	if ( CheckAIInit ( )==0 ) return;

	Entity* pEntity = CheckEntity ( iObjID );
	if ( pEntity ) pEntity->SetCanRoam( iMode > 0 );
}

void AISetEntityCanSearch ( int iObjID, int iMode )
{
	if ( CheckAIInit ( )==0 ) return;

	Entity* pEntity = CheckEntity ( iObjID );
	if ( pEntity ) pEntity->SetCanSearch( iMode > 0 );
}

void AISetPlayerDucking ( int iDuck )
{
	if ( CheckAIInit ( )==0 ) return;

	Hero *pHero = cWorld.pTeamController->GetPlayer ( );

	if ( !pHero )
	{
		if ( bShowErrors )
		{
			MessageBox ( NULL, "Player has not been added to the AI system", "AI Error", 0 );
			//exit ( 1 );
		}

		return;
	}

	pHero->SetDucking ( (iDuck > 0) );
}

void AISetPlayerHeight ( float height )
{
	if ( CheckAIInit ( )==0 ) return;

	Hero *pHero = cWorld.pTeamController->GetPlayer ( );

	if ( !pHero )
	{
		if ( bShowErrors )
		{
			MessageBox ( NULL, "Player has not been added to the AI system", "AI Error", 0 );
			//exit ( 1 );
		}

		return;
	}

	pHero->SetHeight( height );
}

/*
void AISetRadius ( int iObjID, float fNewRadius )
{
	if ( CheckAIInit ( )==0 ) return;

	Entity *pEntity = cWorld.GetEntity ( iObjID );

	if ( pEntity )
	{
		pEntity->SetRadius ( fNewRadius );
	}
	else
	{
		Hero *pHero = cWorld.GetHero ( iObjID );

		if ( pHero ) pHero->SetRadius ( fNewRadius );
		else
		{
			sprintf_s ( errAIStr, 255, "AI Object (%d) Does Not Exist", iObjID );
			MessageBox ( NULL, errAIStr, "AI Error", 0 );
			//exit ( 1 );
			return;
		}
	}
}*/

void AISetRadius ( float fNewRadius )
{
	if ( CheckAIInit ( )==0 ) return;

	cWorld.SetRadius ( fNewRadius );
}

//0 is follow hero, 1+ is fend for themselves
//negative distance preserves current entity defend distance
void AITeamFollowPlayer ( float fDist )
{
	if ( CheckAIInit ( )==0 ) return;

	cWorld.pTeamController->SetTeamStance ( 1, 0, fDist );
}

void AITeamSeparate ( )
{
	if ( CheckAIInit ( )==0 ) return;

	cWorld.pTeamController->SetTeamStance ( 1, 1, -1 );
}

void AIEntityFollowPlayer ( int iObjID, float fDist )
{
	if ( CheckAIInit ( )==0 ) return;

	Entity *pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return;
	if ( !pEntity->IsFriendly( ) ) return;

	pEntity->SetDefendHero ( fDist );
}

void AIEntitySeparate ( int iObjID )
{
	if ( CheckAIInit ( )==0 ) return;

	Entity *pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return;
	
	pEntity->SetAggressiveness( 1 );
	pEntity->SetFollowing ( false );
}

int AIGetEntityFollowing( int iObjID )
{
	if ( CheckAIInit ( )==0 ) return 0;

	Entity *pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return 0;
	
	return pEntity->GetFollowing( ) ? 1 : 0;
}

void AIEntityReset ( int iObjID )
{
	if ( CheckAIInit ( )==0 ) return;

	Entity* pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return;

	pEntity->ResetState( );
}

void AISetEntityDefendDist ( int iObjID, float fDist )
{
	if ( CheckAIInit ( )==0 ) return;

	Entity* pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return;

	pEntity->SetDefendDist ( fDist );
}

void AISetEntityDefending ( int iObjID, int iMode )
{
	if ( CheckAIInit ( )==0 ) return;

	Entity* pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return;

	pEntity->SetDefending ( iMode != 1 );
}

int AIGetEntityCanFire ( int iObjID ) 
{ 
	if ( CheckAIInit ( )==0 ) return 0;

	Entity* pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return 0;

	return (int) pEntity->GetIsFiring ( );
}

int AIGetEntityIsDucking ( int iObjID ) 
{ 
	if ( CheckAIInit ( )==0 ) return 0;

	Entity* pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return 0;

	return (int) pEntity->GetIsDucking ( );
}

float AIGetEntityViewRange ( int iObjID ) 
{ 
	if ( CheckAIInit ( )==0 ) return 0;

	Entity* pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return 0;

	return (float) pEntity->GetViewRange ();
}
float AIGetEntitySpeed ( int iObjID ) 
{ 
	if ( CheckAIInit ( )==0 ) return 0;
	Entity* pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return 0;
	return (float) pEntity->GetMaxSpeed();
}

int AIGetTotalPaths()
{
	return cWorld.GetTotalPaths();
}

int AIGetPathCountPoints( int iID )
{
	return cWorld.CountPathPoints( iID );
}

int AICouldSee ( int iObjID , float fX , float fY , float fZ )
{
	if ( CheckAIInit ( )==0 ) return 0;

	Entity* pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return 0;

	return (int) pEntity->CouldSee ( fX , fY , fZ , true );
}

float AIPathGetPointX ( int iPath, int iPoint )
{
	return cWorld.GetPathPointX(iPath,iPoint);
}
float AIPathGetPointY ( int iPath, int iPoint )
{
	return cWorld.GetPathPointY(iPath,iPoint);
}
float AIPathGetPointZ ( int iPath, int iPoint )
{
	return cWorld.GetPathPointZ(iPath,iPoint);
}

int AIGetEntityChangingContainers( int iObjID )
{
	if ( CheckAIInit ( )==0 ) return 0;

	Entity* pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return 0;

	return (int) pEntity->IsChangingContainers( );
}

void AISetEntityRadius( int iObjID, float fRadius )
{
	if ( CheckAIInit ( )==0 ) return;

	Entity* pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return;

	pEntity->SetRadius( fRadius );
}

void AISetPlayerRadius( float fRadius )
{
	if ( CheckAIInit ( )==0 ) return;

	Hero *pHero = cWorld.pTeamController->GetPlayer ( );

	if ( !pHero )
	{
		if ( bShowErrors )
		{
			MessageBox ( NULL, "Player has not been added to the AI system", "AI Error", 0 );
			//exit ( 1 );
		}

		return;
	}

	pHero->SetRadius( fRadius );
}

void AISetEntityHit ( int iObjID, float x, float z )
{
	if ( CheckAIInit ( )==0 ) return;

	Entity* pEntity = CheckEntity ( iObjID );
	if ( !pEntity ) return;

	pEntity->SetGettingHit ( x, 0.0f, z );
}

int AIEntityExist ( int iObjID )
{
	if ( CheckAIInit ( )==0 ) return 0;

	Entity *pEntity = cWorld.GetEntityCopy ( iObjID );

	return pEntity ? 1 : 0;
}

void AISetThreadMode( int iMode )
{
	if ( CheckAIInit ( )==0 ) return;

	if ( iMode < 0 ) iMode = 0;
	if ( iMode > 2 ) iMode = 2;

	iThreadMode = iMode;
	cWorld.pTeamController->SetThreadNumber( iThreadCount );
}

void AISetThreadCount( int iCount )
{
	if ( CheckAIInit ( )==0 ) return;

	if ( iCount < 1 ) iCount = 1;
	
	iThreadCount = iCount;
	cWorld.pTeamController->SetThreadNumber( iCount );
}

void AISetUndesirableGridSpace( int container, float x, float z )
{
	if ( CheckAIInit ( )==0 ) return;

	Container *pContainer = cWorld.GetContainer ( container );
	
	if ( !pContainer )
	{
		if ( bShowErrors )
		{
			sprintf_s ( errAIStr, "AI Container (%d) Does Not Exist", container );
			MessageBox ( NULL, errAIStr, "AI Error", 0 );
			//exit ( 1 );
		}

		return;
	}

	pContainer->pPathFinder->GridSetUndesirablePosition( x,z );
}

void AIClearUndesirableGridSpace( int container, float x, float z )
{
	if ( CheckAIInit ( )==0 ) return;

	Container *pContainer = cWorld.GetContainer ( container );
	
	if ( !pContainer )
	{
		if ( bShowErrors )
		{
			sprintf_s ( errAIStr, "AI Container (%d) Does Not Exist", container );
			MessageBox ( NULL, errAIStr, "AI Error", 0 );
			//exit ( 1 );
		}

		return;
	}

	pContainer->pPathFinder->GridClearUndesirablePosition( x,z );
}

