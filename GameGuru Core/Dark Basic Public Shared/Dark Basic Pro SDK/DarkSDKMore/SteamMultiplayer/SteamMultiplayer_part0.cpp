// SteamMultiplayer.cpp : Defines the exported functions for the DLL application.
//
/*
Steam Get Messages Begin

while Steam Messages Exist()
 Steam Get Next Message
 type = Steam Get Message Type
 if type = 1
  x# = Steam Get Message Float()
  y# =  Steam Get Message Float()
  health = Steam Get Message Int()
  s$ = Steam Get Message String$
 endif
endwhile
*/

#include "stdafx.h"
#include ".\globstruct.h"

#include "steam/steam_api.h"
#include "StatsAndAchievements.h"
#include "CClient.h"
#include "CSteamServer.h"
#include "MPAudio.h"
#include "voicechat.h"
#include <ShellAPI.h>

#include "CTextC.h"

//#define MAKE_MULTIPLAYER_LOG

// flag to switch workshop handling from workshop to game managed, by default set to false, set to true for multiplayer mode
bool OnlineMultiplayerModeForSharingFiles = false;

#ifdef MAKE_MULTIPLAYER_LOG
void debugLog(LPSTR str );
void debugLog(LPSTR str, int code , int e , int v );
#endif

#ifdef _DEBUG_LOG_
FILE* logFile = NULL;
#endif

#define KEYDOWN(vk_code) ((GetAsyncKeyState(vk_code) & 0x8000) ? 1 : 0)
#define KEYUP(vk_code)   ((GetAsyncKeyState(vk_code) & 0x8000) ? 0 : 1)

uint64 server_timeout_milliseconds = SERVER_TIMEOUT_MILLISECONDS_LONG;

CSteamID lobbyIAmInID;
int scores[MAX_PLAYERS_PER_SERVER];
int keystate[MAX_PLAYERS_PER_SERVER][256];
int alive[MAX_PLAYERS_PER_SERVER];
tbullet bullets[180];
bool bulletSeen[180][MAX_PLAYERS_PER_SERVER];
int playerDamage = 0;
int damageSource = 0;
int damageX = 0;
int damageY = 0;
int damageZ = 0;
int damageForce = 0;
int damageLimb = 0;
int killedSource[MAX_PLAYERS_PER_SERVER];
int killedX[MAX_PLAYERS_PER_SERVER];
int killedY[MAX_PLAYERS_PER_SERVER];
int killedZ[MAX_PLAYERS_PER_SERVER];
int killedForce[MAX_PLAYERS_PER_SERVER];
int killedLimb[MAX_PLAYERS_PER_SERVER];
CSteamID playerSteamIDs[MAX_PLAYERS_PER_SERVER];
int playerAppearance[MAX_PLAYERS_PER_SERVER];
int playerShoot[MAX_PLAYERS_PER_SERVER];
int tweening[MAX_PLAYERS_PER_SERVER];
char workshopItemName[256];
char steamRootPath[MAX_PATH];
bool needToSendMyName = true;
int ClientDeathNumber = 1;
int ServerClientLastDeathNumber[MAX_PLAYERS_PER_SERVER];
int SteamOverlayActive = 0;
int ServerIsShuttingDown = 0;

PublishedFileId_t WorkShopItemID = NULL;
UGCUpdateHandle_t WorkShopItemUpdateHandle = NULL;
uint64 WorkshopItemToDownloadID = NULL;
char WorkshopItemPath[MAX_PATH] = "";
int IsWorkshopLoadingOn = 0;
char hostsLobbyName[256];
bool serverActive = false;

// set when we have actually joined a game
bool isPlayingOnAServer = false;

extern GlobStruct* g_pGlob;

bool g_SteamRunning = false;

extern CClient *g_pClient;

int packetSendLogClientID = 0;
int packetSendLogServerID = 0;

//Guarenteed UDP
std::vector <packetSendLogClient_t> PacketSend_Log_Client;
std::vector <packetSendLogServer_t> PacketSend_Log_Server;
std::vector <packetSendReceiptLogClient_t> PacketSendReceipt_Log_Client;
std::vector <packetSendReceiptLogServer_t> PacketSendReceipt_Log_Server;
//===========

std::vector <tSpawn> spawnList;
std::vector <tLua> luaList;
std::vector <int> deleteList;
std::vector <int> deleteListSource;
std::vector <int> destroyList;
std::vector <tMessage> messageList;
std::vector <tCollision> collisionList;
std::vector <tAnimation> animationList;
std::vector <tChat> chatList;
std::vector <uint32> lobbyChatIDs;

int ServerHowManyToStart = 0;
int ServerHowManyJoined = 0;
int ServerSaysItIsOkayToStart = 0;
int ServerHaveIToldClientsToStart = 0;
int ServerFilesToReceive = 0;
int ServerFilesReceived = 0;
int IamSyncedWithServerFiles = 0;
int IamLoadedAndReady = 0;
int isEveryoneLoadedAndReady = 0;
int HowManyPlayersDoWeHave = 0;
int IamReadyToPlay = 0;
int isEveryoneReadyToPlay = 0;
int serverHowManyFileChunks = 0;
int serverChunkToSendCount = 0;
int serverFileFileSize = 0;
uint64 ServerCreationTime = 0;
int fileProgress = 0;
int voiceChatOn = 0;

int syncedAvatarTextureMode = SYNC_AVATAR_TEX_BEGIN;
int syncedAvatarTextureModeServer = SYNC_AVATAR_TEX_BEGIN;
char myAvatarTextureName[MAX_PATH];
int syncedAvatarHowManyTextures = 0;
int syncedAvatarHowManyTexturesReceived = 0;
FILE* avatarFile[MAX_PLAYERS_PER_SERVER];
int avatarHowManyFileChunks[MAX_PLAYERS_PER_SERVER];
int avatarFileFileSize[MAX_PLAYERS_PER_SERVER] ;

int serverClientsFileSynced[MAX_PLAYERS_PER_SERVER];
int serverClientsLoadedAndReady[MAX_PLAYERS_PER_SERVER];
int serverClientsReadyToPlay[MAX_PLAYERS_PER_SERVER];
int serverClientsHaveAvatarTexture[MAX_PLAYERS_PER_SERVER];

FILE* serverFile = NULL;

tSpawn currentSpawnObject;
tLua currentLua;
int currentDeleteObject;
int currentDeleteObjectSource;
int currentDestroyObject;
tCollision currentCollisionObject;
tAnimation currentAnimationObject;

//======================================================

#define SAFE_DELETE( p )  { if ( p ) { delete ( p );       ( p ) = NULL; } }
#define SAFE_RELEASE( p )  { if ( p ) { ( p )->Release ( ); ( p ) = NULL; } }

//======================================================================================================//

/*void Print (char* c)
{
	g_pGlob->PrintStringFunction ( c , true );
}*/

extern "C" void __cdecl SteamAPIDebugTextHook( int nSeverity, const char *pchDebugText )
{
	// if you're running in the debugger, only warnings (nSeverity >= 1) will be sent
	// if you add -debug_steamapi to the command-line, a lot of extra informational messages will also be sent
	::OutputDebugString( pchDebugText );

	if ( nSeverity >= 1 )
	{
		// place to set a breakpoint for catching API errors
		int x = 3;
		x = x;
	}
}

#define PI 3.14159

float CosineInterpolate( float y1,float y2, float mu)
{
   float mu2;

   mu2 = (float)((1-cos(mu*PI))/2);
   return(y1*(1-mu2)+y2*mu2);
}

float CosineInterpolateAngle( float y1,float y2, float mu)
{

	while (y1 > 360 ) y1-=360;
	while (y2 > 360 ) y2-=360;
	while (y1 < 0 ) y1+=360;
	while (y2 < 0 ) y2+=360;

	float difference = abs(y2 - y1);
	if (difference > 180)
	{
		// We need to add on to one of the values.
		if (y2 > y1)
		{
			// We'll add it on to start...
			y1 += 360;
		}
		else
		{
			// Add it on to end.
			y2 += 360;
		}
	}

	float mu2;

	mu2 = (float)((1-cos(mu*PI))/2);
	return(y1*(1-mu2)+y2*mu2);
}

float CosineInterpolateAngle2( float y1,float y2, float mu)
{
	while (y1 > 360 ) y1-=360;
	while (y2 > 360 ) y2-=360;
	while (y1 < 0 ) y1+=360;
	while (y2 < 0 ) y2+=360;

	float difference = abs(y2 - y1);
	if (difference > 180)
	{
		// We need to add on to one of the values.
		if (y2 > y1)
		{
			// We'll add it on to start...
			y1 += 360;
		}
		else
		{
			// Add it on to end.
			y2 += 360;
		}
	}

    // Interpolate it.
    float value = (y1 + ((y2 - y1) * mu));

    // Wrap it..
    float rangeZero = 360;

    //if (value >= 0 && value <= 360)
        return value;

   // return fmod (value, rangeZero);
}


//-----------------------------------------------------------------------------
// Purpose: Extracts some feature from the command line
//-----------------------------------------------------------------------------
void ParseCommandLine( const char *pchCmdLine, const char **ppchServerAddress, const char **ppchLobbyID, bool *pbUseVR );

//======================================================

#define ACH_PLAYED_A_GAME( id, name ) { id, #id, name, "", 0, 0 }

//======================================================

// Achievement array which will hold data about the achievements and their state
/*Achievement_t g_Achievements[] = 
{
	ACH_PLAYED_A_GAME( ACH_PLAYED_GAME, "ACH_PLAYED_A_GAME_2" ),
};*/

//class CSteamAchievements;

// Global access to Achievements object
//CSteamAchievements*	g_SteamAchievements = NULL;

//=========================================================================================================//

class CSteamUserGeneratedWorkshopItem
{
public:

	CSteamUserGeneratedWorkshopItem();
	~CSteamUserGeneratedWorkshopItem();

	void CreateWorkshopItem();
	void CreateWorkshopItem( LPSTR pString );
	void DownloadWorkshopitem( uint64 id );

	bool isItemCreated;
	int isItemDownloaded;
	int isItemUploaded;
	int isItemSubscribed;

	void OnWorkshopItemCreated( CreateItemResult_t *pCallback, bool bIOFailure );
	void OnWorkshopItemUpdated( SubmitItemUpdateResult_t *pCallback, bool bIOFailure );
	void OnWorkshopItemSubscribed( RemoteStorageSubscribePublishedFileResult_t  *pCallback, bool bIOFailure );
	void OnWorkshopItemDownloaded( ItemInstalled_t  *pCallback, bool bIOFailure );
	void OnWorkshopItemDownloadDone( DownloadItemResult_t  *pCallback );
	void onWorkshopItemQueried( SteamUGCQueryCompleted_t *pCallback, bool bIOFailure );

	CCallResult<CSteamUserGeneratedWorkshopItem, CreateItemResult_t> m_SteamCallResultWorkshopItemCreated;
	CCallResult<CSteamUserGeneratedWorkshopItem, SubmitItemUpdateResult_t> m_SteamCallResultWorkshopItemUpdated;
	CCallResult<CSteamUserGeneratedWorkshopItem, RemoteStorageSubscribePublishedFileResult_t> m_SteamCallResultWorkshopItemSubscribed;
	CCallResult<CSteamUserGeneratedWorkshopItem, ItemInstalled_t> m_SteamCallResultWorkshopItemDownloaded;
	CCallResult<CSteamUserGeneratedWorkshopItem, SteamUGCQueryCompleted_t> m_SteamCallResultWorkshopItemQueried;
	// new sdk
	CCallback<CSteamUserGeneratedWorkshopItem, DownloadItemResult_t > m_SteamCallResultWorkshopItemDownloadDone; // from the new sdk

};

void CSteamUserGeneratedWorkshopItem::onWorkshopItemQueried( SteamUGCQueryCompleted_t *pCallback, bool bIOFailure )
{
#ifdef MAKE_MULTIPLAYER_LOG
	debugLog ( "CSteamUserGeneratedWorkshopItem::onWorkshopItemQueried" );
#endif
	// NEED TO HANDLE FAILURE???

	// if the item does not exist, we make a new workshop item
	if ( pCallback->m_unNumResultsReturned == 0 )
	{
		SteamAPICall_t ret;

		if ( !OnlineMultiplayerModeForSharingFiles )
			ret = SteamUGC()->CreateItem ( SteamUtils()->GetAppID() , k_EWorkshopFileTypeCommunity );	
		else
			ret = SteamUGC()->CreateItem ( SteamUtils()->GetAppID() , k_EWorkshopFileTypeGameManagedItem );

		m_SteamCallResultWorkshopItemCreated.Set( ret, this, &CSteamUserGeneratedWorkshopItem::OnWorkshopItemCreated );
	}
	// if the item does exist, we update the existing instead
	else if ( pCallback->m_unNumResultsReturned > 0 )
	{
		SteamUGCDetails_t details;
		bool found = false;
		for ( unsigned int c = 0; c < pCallback->m_unNumResultsReturned ; c++ )
		{
			bool res = SteamUGC()->GetQueryUGCResult( pCallback->m_handle, c , &details );
			
			if ( strcmp ( details.m_rgchTitle , workshopItemName ) == 0 )
			{
				if ( details.m_eVisibility == k_ERemoteStoragePublishedFileVisibilityPublic && !details.m_bBanned )
				{
					WorkShopItemID = details.m_nPublishedFileId;
					isItemCreated = true;
					found = true;
					break;
				}
			}
		}

		if ( !found )
		{
			SteamAPICall_t ret;

			if ( !OnlineMultiplayerModeForSharingFiles )
				ret = SteamUGC()->CreateItem ( SteamUtils()->GetAppID() , k_EWorkshopFileTypeCommunity );	
			else
				ret = SteamUGC()->CreateItem ( SteamUtils()->GetAppID() , k_EWorkshopFileTypeGameManagedItem );

			m_SteamCallResultWorkshopItemCreated.Set( ret, this, &CSteamUserGeneratedWorkshopItem::OnWorkshopItemCreated );
			return;
		}

		UGCUpdateHandle_t WorkShopItemUpdateHandle = NULL;
		WorkShopItemUpdateHandle = SteamUGC()->StartItemUpdate( SteamUtils()->GetAppID() , WorkShopItemID );

		bool result;
		result = SteamUGC()->SetItemTitle( WorkShopItemUpdateHandle, workshopItemName );
		result = SteamUGC()->SetItemDescription( WorkShopItemUpdateHandle, "Game Guru Level" );
		result = SteamUGC()->SetItemVisibility( WorkShopItemUpdateHandle, k_ERemoteStoragePublishedFileVisibilityPublic );

		char** pMyTags = new char* [ 3 ];

		for ( int c = 0 ; c < 3 ; c++ )
			pMyTags [ c ] = new char [ 128 ];

		strcpy ( pMyTags[0], "Game Guru" );
		strcpy ( pMyTags[1], "FPS" );
		strcpy ( pMyTags[2], "Level" );

		SteamParamStringArray_t tags;

		tags.m_nNumStrings = 3;
		tags.m_ppStrings = (const char**) pMyTags;

		result = SteamUGC()->SetItemTags( WorkShopItemUpdateHandle, &tags );

		for ( int c = 0 ; c < 3 ; c++ )
			delete [] pMyTags [ c ];

		delete [] pMyTags;

		char tempString[MAX_PATH];
		sprintf ( tempString, "%seditors\\workshopItem" , steamRootPath );
		result = SteamUGC()->SetItemContent( WorkShopItemUpdateHandle , tempString );
		sprintf ( tempString, "%seditors\\gfx\\workshopItem.png" , steamRootPath );
		result = SteamUGC()->SetItemPreview( WorkShopItemUpdateHandle, tempString );

		SteamAPICall_t ret = SteamUGC()->SubmitItemUpdate( WorkShopItemUpdateHandle , "Updated Version" );
		m_SteamCallResultWorkshopItemUpdated.Set( ret, this, &CSteamUserGeneratedWorkshopItem::OnWorkshopItemUpdated );
	}

}

void CSteamUserGeneratedWorkshopItem::OnWorkshopItemUpdated( SubmitItemUpdateResult_t *pCallback, bool bIOFailure )
{
	// NEED TO CHECK IF IT WAS SUCESS OR FAIL <============================
	if ( pCallback->m_eResult == k_EResultOK ) //0x1951f420 {m_eResult=k_EResultOK (1) m_bUserNeedsToAcceptWorkshopLegalAgreement=false }
		isItemUploaded = 1;
	else
		isItemUploaded = -1;
}

void CSteamUserGeneratedWorkshopItem::DownloadWorkshopitem ( uint64 id )
{
	// first we subscribe
	isItemDownloaded = 0;
	isItemSubscribed = 0;
	WorkshopItemToDownloadID = id;
	if ( !OnlineMultiplayerModeForSharingFiles )
	{
		SteamAPICall_t ret = SteamUGC()->SubscribeItem ( id );
		m_SteamCallResultWorkshopItemSubscribed.Set( ret, this, &CSteamUserGeneratedWorkshopItem::OnWorkshopItemSubscribed );
		m_SteamCallResultWorkshopItemDownloaded.Set( ret, this, &CSteamUserGeneratedWorkshopItem::OnWorkshopItemDownloaded );
	}
	else
	{
		uint32 ret = SteamUGC()->GetItemState ( id );		
		if ( !(ret & k_EItemStateInstalled) || ret & k_EItemStateNeedsUpdate )
		{
			ret = SteamUGC()->DownloadItem ( id , true );			
			m_SteamCallResultWorkshopItemDownloadDone.Register( this, &CSteamUserGeneratedWorkshopItem::OnWorkshopItemDownloadDone );
		}
		else
		{
			isItemDownloaded = 1;
			char from[MAX_PATH];
			char to[MAX_PATH];
			sprintf ( from , "%s\\editors_gridedit___multiplayerlevel__.fpm" , WorkshopItemPath );
			sprintf ( to , "%seditors\\gridedit\\__multiplayerlevel__.fpm" , steamRootPath );
			DeleteFile ( to );
			CopyFile ( from , to , false );
			IsWorkshopLoadingOn = 1;
		}
	}
}

void CSteamUserGeneratedWorkshopItem::OnWorkshopItemDownloadDone( DownloadItemResult_t  *pCallback )
{

	if ( pCallback->m_nPublishedFileId == WorkshopItemToDownloadID )
	{
		uint64 unSizeOnDisk = 0;
		strcpy ( WorkshopItemPath , "" );

		if ( pCallback->m_eResult != k_EResultOK )
		{
			isItemDownloaded = -1;			
			return;
		}

		char from[MAX_PATH];
		char to[MAX_PATH];
		sprintf ( from , "%s\\editors_gridedit___multiplayerlevel__.fpm" , WorkshopItemPath );
		sprintf ( to , "%seditors\\gridedit\\__multiplayerlevel__.fpm" , steamRootPath );
		DeleteFile ( to );
		CopyFile ( from , to , false );
		IsWorkshopLoadingOn = 1;

		isItemDownloaded = 1;

	}

}

void CSteamUserGeneratedWorkshopItem::OnWorkshopItemDownloaded( ItemInstalled_t *pCallback, bool bIOFailure )
{
	int a = 0;
	if ( pCallback->m_nPublishedFileId == WorkshopItemToDownloadID )
	{
		uint64 unSizeOnDisk = 0;
		strcpy ( WorkshopItemPath , "" );

		//bool legacySupport = false;
		//if ( !SteamUGC()->GetItemInstallInfo( WorkshopItemToDownloadID, &unSizeOnDisk, WorkshopItemPath, sizeof(WorkshopItemPath) , &legacySupport ) )
		uint32 timestamp;
		if ( !SteamUGC()->GetItemInstallInfo( WorkshopItemToDownloadID, &unSizeOnDisk, WorkshopItemPath, sizeof(WorkshopItemPath) , &timestamp ) )
		{
			isItemDownloaded = -1;			
			return;
		}

		char from[MAX_PATH];
		char to[MAX_PATH];
		sprintf ( from , "%s\\editors_gridedit___multiplayerlevel__.fpm" , WorkshopItemPath );
		sprintf ( to , "%seditors\\gridedit\\__multiplayerlevel__.fpm" , steamRootPath );
		DeleteFile ( to );
		CopyFile ( from , to , false );
		IsWorkshopLoadingOn = 1;

		isItemDownloaded = 1;

	}
}

void CSteamUserGeneratedWorkshopItem::OnWorkshopItemSubscribed( RemoteStorageSubscribePublishedFileResult_t *pCallback, bool bIOFailure )
{
	if ( pCallback->m_nPublishedFileId == WorkshopItemToDownloadID )
	{	
		// NEED TO CHECK IF IT WAS SUCESS OR FAIL
		if ( pCallback->m_eResult == k_EResultOK && bIOFailure == false )
			isItemSubscribed = 1;
		else
			isItemSubscribed = -1;
	}
}

void CSteamUserGeneratedWorkshopItem::OnWorkshopItemCreated( CreateItemResult_t *pCallback, bool bIOFailure )
{
	WorkShopItemID = 0;

	if ( pCallback->m_bUserNeedsToAcceptWorkshopLegalAgreement )
		ShellExecuteW( NULL, L"open", L"http://steamcommunity.com/sharedfiles/workshoplegalagreement" , NULL, NULL, SW_SHOWMAXIMIZED );

	if ( !bIOFailure )
	{
		WorkShopItemID = pCallback->m_nPublishedFileId;
		isItemCreated = true;

		UGCUpdateHandle_t WorkShopItemUpdateHandle = NULL;
		WorkShopItemUpdateHandle = SteamUGC()->StartItemUpdate( SteamUtils()->GetAppID() , WorkShopItemID );

		bool result;
		result = SteamUGC()->SetItemTitle( WorkShopItemUpdateHandle, workshopItemName );
		result = SteamUGC()->SetItemDescription( WorkShopItemUpdateHandle, "Game Guru Level" );
		result = SteamUGC()->SetItemVisibility( WorkShopItemUpdateHandle, k_ERemoteStoragePublishedFileVisibilityPublic );

		char** pMyTags = new char* [ 3 ];

		for ( int c = 0 ; c < 3 ; c++ )
			pMyTags [ c ] = new char [ 128 ];

		strcpy ( pMyTags[0], "Game Guru" );
		strcpy ( pMyTags[1], "FPS" );
		strcpy ( pMyTags[2], "Level" );

		SteamParamStringArray_t tags;

		tags.m_nNumStrings = 3;
		tags.m_ppStrings = (const char**) pMyTags;

		result = SteamUGC()->SetItemTags( WorkShopItemUpdateHandle, &tags );

		for ( int c = 0 ; c < 3 ; c++ )
			delete [] pMyTags [ c ];

		delete [] pMyTags;

		char tempString[MAX_PATH];
		sprintf ( tempString, "%seditors\\workshopItem" , steamRootPath );
		result = SteamUGC()->SetItemContent( WorkShopItemUpdateHandle , tempString );
		sprintf ( tempString, "%seditors\\gfx\\workshopItem.png" , steamRootPath );
		result = SteamUGC()->SetItemPreview( WorkShopItemUpdateHandle, tempString );

		SteamAPICall_t ret = SteamUGC()->SubmitItemUpdate( WorkShopItemUpdateHandle , "First Version" );
		m_SteamCallResultWorkshopItemUpdated.Set( ret, this, &CSteamUserGeneratedWorkshopItem::OnWorkshopItemUpdated );


	}
}

CSteamUserGeneratedWorkshopItem::CSteamUserGeneratedWorkshopItem():m_SteamCallResultWorkshopItemDownloadDone(NULL,NULL)
{
}

CSteamUserGeneratedWorkshopItem::~CSteamUserGeneratedWorkshopItem() 
{
}


void CSteamUserGeneratedWorkshopItem::CreateWorkshopItem()
{
	isItemCreated = false;
	isItemUploaded = 0;
	//m_CallbackWorkshopItemCreatedReceived.Register(this,&CSteamUserGeneratedWorkshopItem::OnWorkshopItemCreated);
	SteamAPICall_t ret;

	if ( !OnlineMultiplayerModeForSharingFiles )
		ret = SteamUGC()->CreateItem ( SteamUtils()->GetAppID() , k_EWorkshopFileTypeCommunity );	
	else
		ret = SteamUGC()->CreateItem ( SteamUtils()->GetAppID() , k_EWorkshopFileTypeGameManagedItem );

	m_SteamCallResultWorkshopItemCreated.Set( ret, this, &CSteamUserGeneratedWorkshopItem::OnWorkshopItemCreated );
}

void CSteamUserGeneratedWorkshopItem::CreateWorkshopItem( LPSTR pString )
{
	isItemCreated = false;
	isItemUploaded = 0;

	strcpy ( workshopItemName , pString );
	
	UGCQueryHandle_t handle;

	if ( !OnlineMultiplayerModeForSharingFiles )
	{
		handle = SteamUGC()->CreateQueryUserUGCRequest( SteamUser()->GetSteamID().GetAccountID(), k_EUserUGCList_Published, k_EUGCMatchingUGCType_Items, 
			k_EUserUGCListSortOrder_LastUpdatedDesc, SteamUtils()->GetAppID(), SteamUtils()->GetAppID(), 1 );
	}
	else
	{
		handle = SteamUGC()->CreateQueryUserUGCRequest( SteamUser()->GetSteamID().GetAccountID(), k_EUserUGCList_Published, k_EUGCMatchingUGCType_GameManagedItems, 
			k_EUserUGCListSortOrder_LastUpdatedDesc, SteamUtils()->GetAppID(), SteamUtils()->GetAppID(), 1 );
	}

	SteamUGC()->SetSearchText( handle, pString );

	SteamAPICall_t ret = SteamUGC()->SendQueryUGCRequest ( handle );

	m_SteamCallResultWorkshopItemQueried.Set( ret, this, &CSteamUserGeneratedWorkshopItem::onWorkshopItemQueried );
}

CSteamUserGeneratedWorkshopItem g_UserWorkShopItem;

//=========================================================================================================//
/*
class CSteamAchievements
{
private:
	int64 m_iAppID; // Our current AppID
	Achievement_t *m_pAchievements; // Achievements data
	int m_iNumAchievements; // The number of Achievements
	bool m_bInitialized; // Have we called Request stats and received the callback?

public:
	CSteamAchievements(Achievement_t *Achievements, int NumAchievements);
	~CSteamAchievements();
	
	bool RequestStats();
	bool SetAchievement(const char* ID);

	STEAM_CALLBACK( CSteamAchievements, OnUserStatsReceived, UserStatsReceived_t, 
		m_CallbackUserStatsReceived );
	STEAM_CALLBACK( CSteamAchievements, OnUserStatsStored, UserStatsStored_t, 
		m_CallbackUserStatsStored );
	STEAM_CALLBACK( CSteamAchievements, OnAchievementStored, 
		UserAchievementStored_t, m_CallbackAchievementStored );
};

//=========================================================================================================//

CSteamAchievements::CSteamAchievements(Achievement_t *Achievements, int NumAchievements): 				
 m_iAppID( 0 ),
 m_bInitialized( false ),
 m_CallbackUserStatsReceived( this, &CSteamAchievements::OnUserStatsReceived ),
 m_CallbackUserStatsStored( this, &CSteamAchievements::OnUserStatsStored ),
 m_CallbackAchievementStored( this, &CSteamAchievements::OnAchievementStored )
{
     m_iAppID = SteamUtils()->GetAppID();
     m_pAchievements = Achievements;
     m_iNumAchievements = NumAchievements;
     RequestStats();
}

//=========================================================================================================//

bool CSteamAchievements::RequestStats()
{
	// Is Steam loaded? If not we can't get stats.
	if ( NULL == SteamUserStats() || NULL == SteamUser() )
	{
		return false;
	}
	// Is the user logged on?  If not we can't get stats.
	if ( !SteamUser()->BLoggedOn() )
	{
		return false;
	}
	// Request user stats.
	return SteamUserStats()->RequestCurrentStats();
}

//=========================================================================================================//

bool CSteamAchievements::SetAchievement(const char* ID)
{
	// Have we received a call back from Steam yet?
	if (m_bInitialized)
	{
		SteamUserStats()->SetAchievement(ID);
		return SteamUserStats()->StoreStats();
	}
	// If not then we can't set achievements yet
	return false;
}

//=========================================================================================================//

void CSteamAchievements::OnUserStatsReceived( UserStatsReceived_t *pCallback )
{
 // we may get callbacks for other games' stats arriving, ignore them
 if ( m_iAppID == pCallback->m_nGameID )
 {
   if ( k_EResultOK == pCallback->m_eResult )
   {
     OutputDebugString("Received stats and achievements from Steam\n");
     m_bInitialized = true;

     // load achievements
     for ( int iAch = 0; iAch < m_iNumAchievements; ++iAch )
     {
       Achievement_t &ach = m_pAchievements[iAch];

       SteamUserStats()->GetAchievement(ach.m_pchAchievementID, &ach.m_bAchieved);
       _snprintf( ach.m_rgchName, sizeof(ach.m_rgchName), "%s", 
          SteamUserStats()->GetAchievementDisplayAttribute(ach.m_pchAchievementID, 
          "name"));
       _snprintf( ach.m_rgchDescription, sizeof(ach.m_rgchDescription), "%s", 
          SteamUserStats()->GetAchievementDisplayAttribute(ach.m_pchAchievementID, 
          "desc"));			
     }
   }
   else
   {
     char buffer[128];
     _snprintf( buffer, 128, "RequestStats - failed, %d\n", pCallback->m_eResult );
     OutputDebugString( buffer );
   }
 }
}

//=========================================================================================================//

void CSteamAchievements::OnUserStatsStored( UserStatsStored_t *pCallback )
{
 // we may get callbacks for other games' stats arriving, ignore them
 if ( m_iAppID == pCallback->m_nGameID )	
 {
   if ( k_EResultOK == pCallback->m_eResult )
   {
     OutputDebugString( "Stored stats for Steam\n" );
   }
   else
   {
     char buffer[128];
     _snprintf( buffer, 128, "StatsStored - failed, %d\n", pCallback->m_eResult );
     OutputDebugString( buffer );
   }
 }
}

//=========================================================================================================//

void CSteamAchievements::OnAchievementStored( UserAchievementStored_t *pCallback )
{
     // we may get callbacks for other games' stats arriving, ignore them
     if ( m_iAppID == pCallback->m_nGameID )	
     {
          OutputDebugString( "Stored Achievement for Steam\n" );
     }
}
*/
//=========================================================================================================//



//=========================================================================================================//
//=========================================================================================================//

//=========================================================================================================//
//=========================================================================================================//

 /*FPSCR void ReceiveCoreDataPtr ( LPVOID pCore )
 {
#ifdef MAKE_MULTIPLAYER_LOG
	debugLog ( "ReceiveCoreDataPtr" );
#endif
	// Get Core Data Pointer here
	g_pGlob = (GlobStruct*)pCore;
 }*/

 //=========================================================================================================//

 FPSCR int SteamInit()
 {
#ifdef MAKE_MULTIPLAYER_LOG
	debugLog ( "SteamInit" );
#endif

#ifdef _DEBUG_LOG_
	logStart();
	log("SteamInit()" );
#endif

	SteamCleanupClient();

	ResetGameStats();

	 g_pClient = NULL;

	// Init Steam CEG
	/*if ( !Steamworks_InitCEGLibrary() )
	{
		return 0;
	}*/

	if ( SteamAPI_Init() )
	{
		// must be logged into steam, which will be the case if run from within steam.
		if ( !SteamUser()->BLoggedOn() )
		{
#ifdef _DEBUG_LOG_
	log("SteamInit() Failed - SteamUser()->BLoggedOn(), not logged on" );
#endif
			return 0;
		}

#ifdef _DEBUG_LOG_
	log("SteamInit() Pass - SteamUser()->BLoggedOn(), is logged on" );
#endif

		g_SteamRunning = true;
		//g_SteamAchievements = new CSteamAchievements(g_Achievements, 1);
		// Start up a steam client
#ifdef _DEBUG_LOG_
	log("SteamInitClient()" );
#endif
		SteamInitClient();

		//StartVoiceChat();

		return 1;
	}
	else
	{
#ifdef _DEBUG_LOG_
	log("SteamInit() Failed" );
#endif
		return 0;
	}
 }

 FPSCR void SteamFree()
 {
#ifdef MAKE_MULTIPLAYER_LOG
	debugLog ( "SteamFree - early exit" );
#endif
	 if ( !g_SteamRunning ) return;

#ifdef MAKE_MULTIPLAYER_LOG
	debugLog ( "SteamFree - past early exit" );
#endif
	//EndVoiceChat();

	//SAFE_DELETE ( g_SteamAchievements );
#ifdef MAKE_MULTIPLAYER_LOG
	debugLog ( "SteamFree - Deleting Client Start" );
#endif
	SAFE_DELETE ( g_pClient );
#ifdef MAKE_MULTIPLAYER_LOG
	debugLog ( "SteamFree - Deleting Client End" );
#endif

#ifdef MAKE_MULTIPLAYER_LOG
	debugLog ( "SteamFree - API Shutdown start" );
#endif
	 SteamAPI_Shutdown();
	 g_SteamRunning = false;
#ifdef MAKE_MULTIPLAYER_LOG
	debugLog ( "SteamFree - API SHutdown end" );
#endif

	 IsWorkshopLoadingOn = 0;
 }

 FPSCR bool SteamOwned ( void )
 {
	 bool bGameGuruOwned = SteamApps()->BIsSubscribed();
	 bool bGameGuruOwnedFromFreeWeekend = SteamApps()->BIsSubscribedFromFreeWeekend();
	 if ( bGameGuruOwned == true || bGameGuruOwnedFromFreeWeekend == true )
		 return true;
	 else
		 return false;
 }

 //=========================================================================================================//

 FPSCR LPSTR SteamGetPlayerName(void)
 {
#ifdef MAKE_MULTIPLAYER_LOG
	debugLog ( "SteamGetPlayerName" );
#endif
	// Delete old string
	//if(pOldString) g_pGlob->CreateDeleteString ( (DWORD*)&pOldString, 0 );

	 if ( g_SteamRunning )
	 {
		const char *playerName = SteamFriends()->GetPersonaName();

		if (playerName) 
		{
			// Create a new string and copy input string to it
			/*LPSTR pReturnString=NULL;
			DWORD dwSize=strlen( (const char*)playerName );
			g_pGlob->CreateDeleteString ( (DWORD*)&pReturnString, dwSize+1 );
			strcpy(pReturnString, (const char*)playerName);

			return pReturnString;*/

			return GetReturnStringFromTEXTWorkString((char*)playerName);
		}
	 }
	return "";
 }

 //=========================================================================================================//

  FPSCR LPSTR SteamGetOtherPlayerName( int index )
 {
#ifdef MAKE_MULTIPLAYER_LOG
	debugLog ( "SteamGetOtherPlayerName" );
#endif
	// Delete old string
	//if(pOldString) g_pGlob->CreateDeleteString ( (DWORD*)&pOldString, 0 );

	 if ( g_SteamRunning && index < MAX_PLAYERS_PER_SERVER )
	 {
		// Create a new string and copy input string to it
		/*LPSTR pReturnString=NULL;
		DWORD dwSize=strlen( (const char*)Client()->m_rgpPlayerName[index] );
		g_pGlob->CreateDeleteString ( (DWORD*)&pReturnString, dwSize+1 );
		strcpy(pReturnString, (const char*)Client()->m_rgpPlayerName[index]);

		return pReturnString;*/
		return GetReturnStringFromTEXTWorkString ( Client()->m_rgpPlayerName[index] );
	 }
	return "";
 } 

 //=========================================================================================================//

 FPSCR LPSTR SteamGetPlayerID(void)
 {
#ifdef _DEBUG_LOG_
	log("======== SteamGetPlayerID() ========");
#endif

#ifdef MAKE_MULTIPLAYER_LOG
	debugLog ( "SteamGetPlayerID - entered" );
#endif
	// Delete old string
	//if(pOldString) g_pGlob->CreateDeleteString ( (DWORD*)&pOldString, 0 );

	CSteamID steamID;
	UINT64 SteamID64 = 0;

#ifdef _DEBUG_LOG_
	log("Check if steam is running");
#endif

	if ( !g_SteamRunning )
	{
#ifdef _DEBUG_LOG_
	log("Steam NOT running, performing free and init");
#endif
		SteamFree();
		SteamInit();
	}

#ifdef _DEBUG_LOG_
	log("g_SteamRunning = " , int(g_SteamRunning));
#endif
	 if ( g_SteamRunning )
	 {
#ifdef _DEBUG_LOG_
		log("Steam is running");
#endif
		 steamID = SteamUser()->GetSteamID();
		 SteamID64 = steamID.GetAccountID() + 76561197960265728;

#ifdef _DEBUG_LOG_
		log("** steamID.GetAccountID() = ", steamID.GetAccountID() );
#endif
	 }
#ifdef _DEBUG_LOG_
	 else
	 {
		log("Steam NOT running");
	 }
#endif

#ifdef MAKE_MULTIPLAYER_LOG
	debugLog ( "SteamGetPlayerID - make string" );
#endif
	// Create a new string and copy input string to it
	char steamIDString[128];
	sprintf ( steamIDString , "%llu" , SteamID64 );
#ifdef _DEBUG_LOG_
	log("** SteamID64:" );
	log(steamIDString);
#endif
	/*LPSTR pReturnString=NULL;
	DWORD dwSize=strlen( (const char*)steamIDString );
	g_pGlob->CreateDeleteString ( (DWORD*)&pReturnString, dwSize+1 );
	strcpy(pReturnString, (const char*)steamIDString);
#ifdef _DEBUG_LOG_
	log("** pReturnString:" );
	log(pReturnString);
#endif

	return pReturnString;*/

	return GetReturnStringFromTEXTWorkString ( steamIDString );

 }

 //=========================================================================================================//

 FPSCR void SteamLoop(void)
 {
	if ( g_SteamRunning )
	{

		if ( !SteamUser()->BLoggedOn() )
		{
			g_SteamRunning = false;
			return;
		}

		SteamAPI_RunCallbacks(); 
	}
	else
	{
		//g_pGlob->PrintStringFunction ( "Steam Not Running is it" , true );
		return;
	}

	//g_pGlob->PrintStringFunction ( "Steam Loop" , true );


	Client()->timeElapsed = GetCounterPassed();

	// Run a game frame
	Client()->RunFrame();

	// Keep running the network on the client at a faster rate than the FPS limit
	Client()->ReceiveNetworkData();

 }

 //=========================================================================================================//

 FPSCR int SteamIsOnline(void)
 {
#ifdef MAKE_MULTIPLAYER_LOG
	debugLog ( "SteamIsOnline" );
#endif
	 if ( SteamUser()->BLoggedOn() )
		 return 1;
	 else
		 return 0;
 }

 //=========================================================================================================//

 FPSCR void SteamAddAch(void)
 {
	 /*if (g_SteamAchievements)
		g_SteamAchievements->SetAchievement("ACH_PLAYED_A_GAME_2");*/
 }

 //=========================================================================================================//

 //-----------------------------------------------------------------------------
// Purpose: Extracts some feature from the command line
//-----------------------------------------------------------------------------
void ParseCommandLine( const char *pchCmdLine, const char **ppchServerAddress, const char **ppchLobbyID, bool *pbUseVR )
{
#ifdef MAKE_MULTIPLAYER_LOG
	debugLog ( "ParseCommandLine" );
#endif
	// Look for the +connect ipaddress:port parameter in the command line,
	// Steam will pass this when a user has used the Steam Server browser to find
	// a server for our game and is trying to join it. 
	const char *pchConnectParam = "+connect";
	const char *pchConnect = strstr( pchCmdLine, pchConnectParam );
	*ppchServerAddress = NULL;
	if ( pchConnect && strlen( pchCmdLine ) > (pchConnect - pchCmdLine) + strlen( pchConnectParam ) + 1 )
	{
		// Address should be right after the +connect, +1 on the end to skip the space
		*ppchServerAddress = pchCmdLine + ( pchConnect - pchCmdLine ) + strlen( pchConnectParam ) + 1;
	}

	// look for +connect_lobby lobbyid paramter on the command line
	// Steam will pass this in if a user taken up an invite to a lobby
	const char *pchConnectLobbyParam = "+connect_lobby";
	const char *pchConnectLobby = strstr( pchCmdLine, pchConnectParam );
	*ppchLobbyID = NULL;
	if ( pchConnectLobby && strlen( pchCmdLine ) > (pchConnectLobby - pchCmdLine) + strlen( pchConnectLobbyParam ) + 1 )
	{
		// Address should be right after the +connect, +1 on the end to skip the space
		*ppchLobbyID = pchCmdLine + ( pchConnectLobby - pchCmdLine ) + strlen( pchConnectLobbyParam ) + 1;
	}

	// look for -vr on command line. Switch to VR mode if it's thre
	const char *pchVRParam = "-vr";
	const char *pchVR = strstr( pchCmdLine, pchVRParam );
	if ( pchVR )
		*pbUseVR = true;
}

void SteamInitClient()
{
#ifdef MAKE_MULTIPLAYER_LOG
	debugLog ( "SteamInitClient" );
#endif
	//g_pGlob->PrintStringFunction ( "Steam Init", true );

	// set our debug handler
#ifdef _DEBUG_LOG_
	log("SteamClient()->SetWarningMessageHook( &SteamAPIDebugTextHook );" );
#endif
	SteamClient()->SetWarningMessageHook( &SteamAPIDebugTextHook );

	// Tell Steam where it's overlay should show notification dialogs, this can be top right, top left,
	// bottom right, bottom left. The default position is the bottom left if you don't call this.  
	// Generally you should use the default and not call this as users will be most comfortable with 
	// the default position.  The API is provided in case the bottom right creates a serious conflict 
	// with important UI in your game.
#ifdef _DEBUG_LOG_
	log("SteamUtils()->SetOverlayNotificationPosition( k_EPositionTopLeft );" );
#endif
	SteamUtils()->SetOverlayNotificationPosition( k_EPositionTopLeft );

	// We are going to use the controller interface, initialize it, which is a seperate step as it 
	// create a new thread in the game proc and we don't want to force that on games that don't
	// have native Steam controller implementations

	bool bUseVR = SteamUtils()->IsSteamRunningInVR();
#ifdef _DEBUG_LOG_
	log("SteamUtils()->IsSteamRunningInVR() = ", int(bUseVR) );
#endif
	const char *pchServerAddress, *pchLobbyID;
	ParseCommandLine( "", &pchServerAddress, &pchLobbyID, &bUseVR );

	// do a DRM self check
	Steamworks_SelfCheck();

	//=====================================================

	// Initialize the game
	g_pClient = new CClient();
	StartCounter();

	// If +connect was used to specify a server address, connect now
	g_pClient->ExecCommandLineConnect( pchServerAddress, pchLobbyID );

	// test a user specific secret before entering main loop
	Steamworks_TestSecret();

	g_pClient->RetrieveEncryptedAppTicket();

	g_SteamRunning = true;

}

FPSCR void SteamCreateLobby()
{
#ifdef MAKE_MULTIPLAYER_LOG
	debugLog ( "SteamCreateLobby" );
#endif
	isPlayingOnAServer = false;
	if ( g_SteamRunning )
	{
		messageList.clear();
		chatList.clear();
		deleteList.clear();
		deleteListSource.clear();
		destroyList.clear();
		lobbyChatIDs.clear();

		syncedAvatarTextureMode = SYNC_AVATAR_TEX_BEGIN;
		syncedAvatarTextureModeServer = SYNC_AVATAR_TEX_BEGIN;

		PacketSend_Log_Client.clear();
		PacketSend_Log_Server.clear();
		PacketSendReceipt_Log_Client.clear();
		PacketSendReceipt_Log_Server.clear();

		ClientDeathNumber = 1;
		for ( int c = 0 ; c < MAX_PLAYERS_PER_SERVER ; c++ )
			ServerClientLastDeathNumber[c] = 0;

		server_timeout_milliseconds = SERVER_TIMEOUT_MILLISECONDS_LONG;
		Client()->SteamCreateLobby();
	}
}

FPSCR int SteamIsLobbyCreated()
{
#ifdef MAKE_MULTIPLAYER_LOG
	debugLog ( "SteamIsLobbyCreated" );
#endif
	if ( g_SteamRunning )
	{
		return Client()->SteamIsLobbyCreated();
	}
	return 0;
}

FPSCR void SteamGetLobbyList()
{
#ifdef MAKE_MULTIPLAYER_LOG
	debugLog ( "SteamGetLobbyList" );
#endif
	isPlayingOnAServer = false;
	if ( g_SteamRunning )
	{
		server_timeout_milliseconds = SERVER_TIMEOUT_MILLISECONDS_LONG;
		Client()->SteamGetLobbyList();
	}
}

FPSCR int SteamIsLobbyListCreated()
{
#ifdef MAKE_MULTIPLAYER_LOG
	debugLog ( "SteamIsLobbyListCreated" );
#endif
	if ( g_SteamRunning )
		return Client()->SteamIsLobbyListCreated();
	return 0;
}

FPSCR int SteamGetLobbyListSize()
{
#ifdef MAKE_MULTIPLAYER_LOG
	debugLog ( "SteamGetLobbyListSize" );
#endif
	if ( g_SteamRunning )
		return Client()->SteamGetLobbyListSize();
	return 0;
}

FPSCR LPSTR SteamGetLobbyListName( int index )
{
#ifdef MAKE_MULTIPLAYER_LOG
	debugLog ( "SteamGetLobbyListName" );
#endif
	if ( g_SteamRunning )
		return Client()->SteamGetLobbyListName( index );
	return NULL;
}

FPSCR void SteamJoinLobby( int index )
{
#ifdef MAKE_MULTIPLAYER_LOG
	debugLog ( "SteamJoinLobby" );
#endif
	if ( g_SteamRunning )
	{
		messageList.clear();
		chatList.clear();
		deleteList.clear();
		deleteListSource.clear();
		destroyList.clear();
		lobbyChatIDs.clear();

		syncedAvatarTextureMode = SYNC_AVATAR_TEX_BEGIN;
		syncedAvatarTextureModeServer = SYNC_AVATAR_TEX_BEGIN;

		PacketSend_Log_Client.clear();
		PacketSend_Log_Server.clear();
		PacketSendReceipt_Log_Client.clear();
		PacketSendReceipt_Log_Server.clear();

		ClientDeathNumber = 1;
		for ( int c = 0 ; c < MAX_PLAYERS_PER_SERVER ; c++ )
			ServerClientLastDeathNumber[c] = 0;

		server_timeout_milliseconds = SERVER_TIMEOUT_MILLISECONDS_LONG;
		return Client()->SteamJoinLobby( index );
	}
}

FPSCR int SteamGetLobbyUserCount()
{
#ifdef MAKE_MULTIPLAYER_LOG
	debugLog ( "SteamGetLobbyUserCount" );
#endif
	if ( Client() ) 
	{
		if ( Client()->m_pServer == NULL )
			HowManyPlayersDoWeHave = Client()->SteamGetUsersInLobbyCount();
		return Client()->SteamGetUsersInLobbyCount();
	}
	else
		return 0;
}

FPSCR int SteamHasJoinedLobby()
{
#ifdef MAKE_MULTIPLAYER_LOG
	debugLog ( "SteamHasJoinedLobby" );
#endif
	if ( g_SteamRunning )
		return Client()->SteamHasJoinedLobby();
	return 0;
}

FPSCR void SteamStartServer()
{
#ifdef MAKE_MULTIPLAYER_LOG
	debugLog ( "SteamStartServer" );
#endif
#ifdef _DEBUG_LOG_
	log("SteamStartServer()" );
#endif

	if ( g_SteamRunning )
	{
		server_timeout_milliseconds = SERVER_TIMEOUT_MILLISECONDS_LONG;
		Client()->SteamStartServer();
		serverActive = true;
	}
}

FPSCR int SteamIsServerRunning()
{
#ifdef MAKE_MULTIPLAYER_LOG
	debugLog ( "SteamIsServerRunning" );
#endif
#ifdef _DEBUG_LOG_
	//log("SteamIsServerRunning()" , Client()->SteamIsServerRunning() );
#endif

	if ( g_SteamRunning )
	{
		return Client()->SteamIsServerRunning();
	}
	return 0;

}

FPSCR int SteamIsGameRunning()
{
#ifdef MAKE_MULTIPLAYER_LOG
	debugLog ( "SteamIsGameRunning" );
#endif
#ifdef _DEBUG_LOG_
	//log("SteamIsGameRunning()" , Client()->SteamIsGameRunning() );
#endif

	if ( g_SteamRunning )
	{
		int ret = Client()->SteamIsGameRunning();
		serverActive = 0;
		if ( ret == 1 )
			serverActive = 1;
		return ret;
	}
	return 0;
}

FPSCR int SteamGetMyPlayerIndex()
{
#ifdef MAKE_MULTIPLAYER_LOG
	debugLog ( "SteamGetMyPlayerIndex" );
#endif
	if ( g_SteamRunning )
		return Client()->SteamGetMyPlayerIndex();

	return 0;
}

FPSCR void SteamSetPlayerPositionX( float _x)
{
#ifdef MAKE_MULTIPLAYER_LOG
	debugLog ( "SteamGetMyPlayerIndex" );
#endif
	if ( g_SteamRunning )
		Client()->SteamSetPlayerPositionX( _x );
}

FPSCR void SteamSetPlayerPositionY( float _y )
{
#ifdef MAKE_MULTIPLAYER_LOG
	debugLog ( "SteamSetPlayerPositionY" );
#endif
	if ( g_SteamRunning )
		Client()->SteamSetPlayerPositionY( _y );
}

FPSCR void SteamSetPlayerPositionZ( float _z )
{
#ifdef MAKE_MULTIPLAYER_LOG
	debugLog ( "SteamSetPlayerPositionZ" );
#endif
	if ( g_SteamRunning )
		Client()->SteamSetPlayerPositionZ( _z );
}

FPSCR void SteamSetPlayerAngle( float _angle )
{
#ifdef MAKE_MULTIPLAYER_LOG
	debugLog ( "SteamSetPlayerAngle" );
#endif
	if ( g_SteamRunning )
		Client()->SteamSetPlayerAngle( _angle );
}

FPSCR void SteamSetPlayerScore( int index, int score )
{
#ifdef MAKE_MULTIPLAYER_LOG
	debugLog ( "SteamSetPlayerScore" );
#endif
	if ( g_SteamRunning )
		Client()->SteamSetPlayerScore( index, score );
}

FPSCR float SteamGetPlayerPositionX ( int index )
{
#ifdef MAKE_MULTIPLAYER_LOG
	debugLog ( "SteamGetPlayerPositionX" );
#endif
	if ( g_SteamRunning )
	{
		float fValue = Client()->SteamGetPlayerPositionX ( index );
		return fValue;
	}

	return 0;
}

FPSCR float SteamGetPlayerPositionY ( int index )
{
#ifdef MAKE_MULTIPLAYER_LOG
	debugLog ( "SteamGetPlayerPositionY" );
#endif
	if ( g_SteamRunning )
	{
		float fValue = Client()->SteamGetPlayerPositionY ( index );
		return fValue;
	}

	return 0;
}

FPSCR float SteamGetPlayerPositionZ ( int index )
{
#ifdef MAKE_MULTIPLAYER_LOG
	debugLog ( "SteamGetPlayerPositionZ" );
#endif
	if ( g_SteamRunning )
	{
		float fValue = Client()->SteamGetPlayerPositionZ ( index );
		return fValue;
	}

	return 0;
}

FPSCR float SteamGetPlayerAngle ( int index )
{
#ifdef MAKE_MULTIPLAYER_LOG
	debugLog ( "SteamGetPlayerAngle" );
#endif
	if ( g_SteamRunning )
	{
		float fValue = Client()->SteamGetPlayerAngle ( index );
		return fValue;
	}

	return 0;
}

FPSCR int SteamGetPlayerScore ( int index )
{
	if ( g_SteamRunning )
	{
		return Client()->SteamGetPlayerScore ( index );
	}

	return 0;
}

FPSCR void SteamSetBullet ( int index , float x , float y , float z, float anglex, float angley, float anglez, int type, int on )
{
	if ( g_SteamRunning )
	{
		Client()->SteamSetBullet ( index , x , y , z, anglex, angley, anglez, type, on );
	}
}

FPSCR int SteamGetBulletOn ( int index )
{
	if ( g_SteamRunning )
	{
		return Client()->SteamGetBulletOn ( index );
	}

	return 0;
}

FPSCR int SteamGetBulletType ( int index )
{
	if ( g_SteamRunning )
	{
		return Client()->SteamGetBulletType ( index );
	}

	return 0;
}

FPSCR float SteamGetBulletX ( int index )
{
	if ( g_SteamRunning )
	{
		float fValue = Client()->SteamGetBulletX ( index );
		return fValue;
	}
	return 0;
}

FPSCR float SteamGetBulletY ( int index )
{
	if ( g_SteamRunning )
	{
		float fValue = Client()->SteamGetBulletY ( index );
		return fValue;
	}
	return 0;
}

FPSCR float SteamGetBulletZ ( int index )
{
	if ( g_SteamRunning )
	{
		float fValue = Client()->SteamGetBulletZ ( index );
		return fValue;
	}
	return 0;
}

FPSCR float SteamGetBulletAngleX ( int index )
{
	if ( g_SteamRunning )
	{
		float fValue = Client()->SteamGetBulletAngleX ( index );
		return fValue;
	}
	return 0;
}

FPSCR float SteamGetBulletAngleY ( int index )
{
	if ( g_SteamRunning )
	{
		float fValue = Client()->SteamGetBulletAngleY ( index );
		return fValue;
	}
	return 0;
}

FPSCR float SteamGetBulletAngleZ ( int index )
{
	if ( g_SteamRunning )
	{
		float fValue = Client()->SteamGetBulletAngleZ ( index );
		return fValue;
	}
	return 0;
}

/*FPSCR void SteamBoom ( float x , float y , float z )
{
	if ( g_SteamRunning )
	{
		Client()->SteamBoom ( x , y , z );
	}
}*/

FPSCR void SteamSetKeyState ( int key , int state )
{
	if ( g_SteamRunning )
		Client()->SteamSetKeyState(key,state);
}

FPSCR int SteamGetKeyState ( int index, int key )
{
	if ( g_SteamRunning )
		return keystate[index][key];

	return 0;
}

FPSCR void SteamApplyPlayerDamage ( int index, int damage, int x, int y, int z, int force, int limb )
{
#ifdef MAKE_MULTIPLAYER_LOG
	debugLog ( "SteamApplyPlayerDamage" );
#endif
	if ( g_SteamRunning )
		Client()->SteamApplyPlayerDamage( index , damage, x, y, z, force, limb );
}

FPSCR int SteamGetPlayerDamageSource()
{
	return damageSource;
}

FPSCR int SteamGetPlayerDamageX()
{
	return damageX;
}

FPSCR int SteamGetPlayerDamageY()
{
	return damageY;
}

FPSCR int SteamGetPlayerDamageZ()
{
	return damageZ;
}

FPSCR int SteamGetPlayerDamageForce()
{
	return damageForce;
}

FPSCR int SteamGetPlayerDamageLimb()
{
	return damageLimb;
}

FPSCR int SteamGetPlayerKilledSource( int index )
{
	return killedSource[index];
}

FPSCR int SteamGetPlayerKilledX( int index )
{
	return killedX[index];
}

FPSCR int SteamGetPlayerKilledY( int index )
{
	return killedY[index];
}

FPSCR int SteamGetPlayerKilledZ( int index )
{
	return killedZ[index];
}

FPSCR int SteamGetPlayerKilledForce( int index )
{
	return killedForce[index];
}

FPSCR int SteamGetPlayerKilledLimb( int index )
{
	return killedLimb[index];
}

FPSCR void SteamKilledBy ( int killedBy, int x, int y, int z, int force, int limb )
{
#ifdef MAKE_MULTIPLAYER_LOG
	debugLog ( "SteamKilledBy" );
#endif
	if ( g_SteamRunning )
		Client()->SteamKilledBy( killedBy, x, y, z, force, limb );
}

FPSCR void SteamKilledSelf()
{
#ifdef MAKE_MULTIPLAYER_LOG
	debugLog ( "SteamKilledSelf" );
#endif
	if ( g_SteamRunning )
		Client()->SteamKilledSelf();
}

char ServerMessage[256];

FPSCR LPSTR SteamGetServerMessage( void )
{
#ifdef MAKE_MULTIPLAYER_LOG
	debugLog ( "SteamGetServerMessage" );
#endif
	// Delete old string
	//if(pOldString) g_pGlob->CreateDeleteString ( (DWORD*)&pOldString, 0 );

	if ( messageList.size() > 0 )
	{
		// Create a new string and copy input string to it
		/*LPSTR pReturnString=NULL;
		DWORD dwSize=strlen( (const char*)messageList[0].message );
		g_pGlob->CreateDeleteString ( (DWORD*)&pReturnString, dwSize+1 );*/

		strcpy(ServerMessage , (const char*)messageList[0].message);
		messageList.erase( messageList.begin() );

		return ServerMessage;		
		//return GetReturnStringFromTEXTWorkString( messageList[0].message );

	}

	return "";
} 

FPSCR int SteamGetPlayerDamageAmount ()
{
#ifdef MAKE_MULTIPLAYER_LOG
	debugLog ( "SteamGetPlayerDamageAmount" );
#endif
	if ( g_SteamRunning )
	{	
		int result = playerDamage;
		playerDamage = 0;
		return result;
	}
	return 0;
}

FPSCR int SteamGetClientServerConnectionStatus()
{
#ifdef MAKE_MULTIPLAYER_LOG
	debugLog ( "SteamGetClientServerConnectionStatus" );
#endif
	if ( g_SteamRunning )
	{
		int result = Client()->SteamGetClientServerConnectionStatus();

		if ( result == 0 )
		{
			SteamResetClient();
			return 0;
		}
		// 020315 - 012 - server to send quit game message
		// server closing down status
		if ( result == 2 )
		{
			SteamResetClient();
			ServerIsShuttingDown = 0;
			return 2;
		}
		else
			return 1;
	}

	return 0;
}

void SteamResetClient()
{
#ifdef MAKE_MULTIPLAYER_LOG
	debugLog ( "SteamResetClient" );
#endif
	SteamCleanupClient();

	SAFE_DELETE ( g_pClient );

	 g_pClient = NULL;

	// must be logged into steam, which will be the case if run from within steam.
	if ( !SteamUser()->BLoggedOn() )
		return;

	// Initialize the game again
	g_pClient = new CClient();
	//SteamInitClient();
	//StartCounter();

	deleteList.clear();
	deleteListSource.clear();
	destroyList.clear();

	messageList.clear();
	chatList.clear();
	lobbyChatIDs.clear();

	g_pClient->RetrieveEncryptedAppTicket();

}

void SteamCleanupClient()
{
#ifdef MAKE_MULTIPLAYER_LOG
	debugLog ( "SteamCleanupClient" );
#endif
	ResetGameStats();
}

void ResetGameStats()
{
#ifdef MAKE_MULTIPLAYER_LOG
	debugLog ( "ResetGameStats" );
#endif
	serverActive = false;

	IsWorkshopLoadingOn = 0;
	ServerIsShuttingDown = 0;

	char fileToDelete[MAX_PATH];

	for ( int c = 0; c < MAX_PLAYERS_PER_SERVER ; c++ )
	{
		scores[c] = 0;
		for ( int k = 0 ; k < 256 ; k++ ) keystate[c][k];
		alive[c] = 1;
		playerAppearance[c] = 0;

		serverClientsFileSynced[c] = 0;
		serverClientsLoadedAndReady[c] = 0;
		serverClientsReadyToPlay[c] = 0;
		playerShoot[c] = 0;
		tweening[c] = 1;
		HowManyPlayersDoWeHave = 0;

		avatarFile[c] = NULL;
		avatarHowManyFileChunks[c] = 0;
		avatarFileFileSize[c] = 0;

		sprintf ( fileToDelete, "%sentitybank\\user\\charactercreator\\customAvatar_%i_cc.dds" , steamRootPath , c );
		DeleteFile ( fileToDelete );

		sprintf ( fileToDelete, "%sentitybank\\user\\charactercreator\\customAvatar_%i.fpe" , steamRootPath , c );
		DeleteFile ( fileToDelete );
	}

	for ( int i = 0 ; i < 179 ; i++ )
	{
		bullets[i].on = 0;
		for ( int a = 0 ; a < MAX_PLAYERS_PER_SERVER; a++ )
			bulletSeen[i][a] = false;
	}

	// wait a long time when inititally connecting to allow for long loading times
	server_timeout_milliseconds = SERVER_TIMEOUT_MILLISECONDS_LONG;

	ServerFilesToReceive = 0;
	ServerFilesReceived = 0;
	IamSyncedWithServerFiles = 0;
	IamLoadedAndReady = 0;
	IamReadyToPlay = 0;
	isEveryoneLoadedAndReady = 0;
	isEveryoneReadyToPlay = 0;
	serverChunkToSendCount = 0;
	fileProgress = 0;
	HowManyPlayersDoWeHave = 0;
	ServerHowManyToStart = 0;

	strcpy ( WorkshopItemPath , "" );

	if ( serverFile )
	{
		fclose (serverFile );
		serverFile = NULL;
	}

}

FPSCR void SteamSetPlayerAlive ( int state )
{
#ifdef MAKE_MULTIPLAYER_LOG
	debugLog ( "SteamSetPlayerAlive" );
#endif
	if ( g_SteamRunning )
		Client()->SteamSetPlayerAlive(state);
}

FPSCR int SteamGetPlayerAlive ( int index )
{
#ifdef MAKE_MULTIPLAYER_LOG
	debugLog ( "SteamGetPlayerAlive" );
#endif
	if ( g_SteamRunning )
		return alive[index];

	return 0;
}

FPSCR void SteamSpawnObject ( int obj, int source, float x, float y, float z )
{
#ifdef MAKE_MULTIPLAYER_LOG
	debugLog ( "SteamSpawnObject" );
#endif
	if ( g_SteamRunning )
		Client()->SteamSpawnObject ( obj, source, x, y, z );
}

FPSCR void SteamDeleteObject ( int obj )
{
#ifdef MAKE_MULTIPLAYER_LOG
	debugLog ( "SteamDeleteObject" );
#endif
	if ( g_SteamRunning )
		Client()->SteamDeleteObject ( obj );
}

FPSCR void SteamDestroy ( int obj )
{
#ifdef MAKE_MULTIPLAYER_LOG
	debugLog ( "SteamDestroy" );
#endif
	if ( g_SteamRunning )
		Client()->SteamDestroyObject ( obj );
}

FPSCR void SteamSendLua ( int code, int e, int v )
{
#ifdef MAKE_MULTIPLAYER_LOG
	debugLog ( "SteamSendLua", code, e, v );
#endif
	if ( g_SteamRunning )
	{
		if (!serverActive) return;

		if ( Client() )
		{
			Client()->SteamSendLua ( code, e, v);
		}
	}
}

FPSCR void SteamSendLuaString ( int code, int e, LPSTR s )
{
#ifdef MAKE_MULTIPLAYER_LOG
	debugLog ( "SteamSendLua", code, e, v );
#endif
	if ( g_SteamRunning )
	{
		if (!serverActive) return;

		if ( Client() )
		{
			Client()->SteamSendLuaString ( code, e, s);
		}
	}
}

FPSCR int SteamGetLuaList()
{
#ifdef MAKE_MULTIPLAYER_LOG
	debugLog ( "SteamGetLuaList" );
#endif
	if ( luaList.size() > 0 )
	{
		currentLua.code = luaList.back().code;
		currentLua.e = luaList.back().e;
		currentLua.v = luaList.back().v;
		//strcpy ( currentLua.s , luaList.back().s );
		if ( strlen(luaList.back().s) >= CURRENT_LUA_STRING_SIZE ) MessageBox ( NULL , "Lua String Exceeded Max", "SteamMultiplayer Error", MB_OK );
		strncpy ( currentLua.s , luaList.back().s , CURRENT_LUA_STRING_SIZE );
		currentLua.s[CURRENT_LUA_STRING_SIZE-1] = 0;

		return 1;
	}
	return 0;
}

FPSCR void SteamGetNextLua()
{
#ifdef MAKE_MULTIPLAYER_LOG
	debugLog ( "SteamGetNextLua" );
#endif
	if ( luaList.size() > 0 )
		luaList.pop_back();
}

FPSCR int SteamGetLuaCommand()
{
#ifdef MAKE_MULTIPLAYER_LOG
	debugLog ( "SteamGetLuaCommand" );
#endif
	return currentLua.code;
}

FPSCR int SteamGetLuaE()
{
#ifdef MAKE_MULTIPLAYER_LOG
	debugLog ( "SteamGetLuaE" );
#endif
	return currentLua.e;
}

