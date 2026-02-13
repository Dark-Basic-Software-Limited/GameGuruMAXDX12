//========= Copyright � 1996-2008, Valve LLC, All rights reserved. ============
//
// Purpose: Main class for the space war game client
//
// $NoKeywords: $
//=============================================================================

#include "stdafx.h"
#include "CClient.h"
#include "SteamMultiplayer.h"
#include "CSteamServer.h"
#include "stdlib.h"
#include "time.h"
#include "ServerBrowser.h"
#include "Leaderboards.h"
#include "Lobby.h"
#include "p2pauth.h"
#include "voicechat.h"
#include "steam/steamencryptedappticket.h"
#include "globstruct.h"
#ifdef WIN32
#include <direct.h>
#else
#define MAX_PATH PATH_MAX
#define _getcwd getcwd
#endif

#ifdef _DEBUG_LOG_
int DEBUG_FLAG_ON = 0;
#endif

//#define INTERPOLATE_SMOOTHING 0.15
//#define INTERPOLATE_SMOOTHING 0.25
//#define INTERPOLATE_SMOOTHING 0.1f // prevous setting, too smooth and skiddy
// This works great - DO NOT CHANGE!!!!!!!!!!!!!!!!
#define INTERPOLATE_SMOOTHING 0.2f
#define INTERPOLATE_SMOOTHING_MIN 0.25f
#define INTERPOLATE_SMOOTHING_TURN 0.1f

extern void ParseCommandLine( const char *pchCmdLine, const char **ppchServerAddress, const char **ppchLobbyID, bool *pbUseVR );

extern GlobStruct* g_pGlob;

CClient *g_pClient = NULL;
CClient* Client() { return g_pClient; }

#if defined(WIN32)
#define atoll _atoi64
#endif


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CClient::CClient( ) :
		m_CallbackP2PSessionConnectFail( this, &CClient::OnP2PSessionConnectFail ),
		m_LobbyGameCreated( this, &CClient::OnLobbyGameCreated ),
		m_LobbyChatMsg ( this, &CClient::OnLobbyChatMessage  ),
		m_AvatarImageLoadedCreated( this, &CClient::OnAvatarImageLoaded ),
		m_IPCFailureCallback( this, &CClient::OnIPCFailure ),
		m_SteamShutdownCallback( this, &CClient::OnSteamShutdown ),
		m_SteamServersConnected( this, &CClient::OnSteamServersConnected ),
		m_SteamServersDisconnected( this, &CClient::OnSteamServersDisconnected ),
		m_SteamServerConnectFailure( this, &CClient::OnSteamServerConnectFailure ),
		m_GameJoinRequested( this, &CClient::OnGameJoinRequested ),
		m_CallbackGameOverlayActivated( this, &CClient::OnGameOverlayActivated ),
		m_CallbackGameWebCallback( this, &CClient::OnGameWebCallback ),
		m_CallbackWorkshopItemInstalled( this, &CClient::OnWorkshopItemInstalled )
{
	Init( );
}



//-----------------------------------------------------------------------------
// Purpose: initialize our client for use
//-----------------------------------------------------------------------------
void CClient::Init()
{

#ifdef _DEBUG_LOG_
	log("CClient::Init()" , m_eGameState );
#endif

	if ( SteamUser()->BLoggedOn() )
	{
		m_SteamIDLocalUser = SteamUser()->GetSteamID();
		m_eGameState = k_EClientGameMenu;
	}

	spawnList.clear();
	luaList.clear();
	deleteList.clear();
	deleteListSource.clear();
	collisionList.clear();
	animationList.clear();

	deleteList.clear();
	deleteListSource.clear();
	destroyList.clear();

	m_gotPlayerInfoFromServer = false;
	m_bLastControllerStateInMenu = false;
	g_pClient = this;
	m_uPlayerWhoWonGame = 0;
	m_ulLastNetworkDataReceivedTime = 0;
	m_pServer = NULL;
	m_uPlayerIndex = 0;
	m_eConnectedStatus = k_EClientNotConnected;
	m_bTransitionedGameState = true;
	m_rgchErrorText[0] = 0;
	m_unServerIP = 0;
	m_usServerPort = 0;
	m_ulPingSentTime = 0;
	m_bSentWebOpen = false;
	m_bSentPlayerName = false;
	ServerSaysItIsOkayToStart = 0;
	ServerHaveIToldClientsToStart = 0;

	char tStr[256];
	for( uint32 i = 0; i < MAX_PLAYERS_PER_SERVER; ++i )
	{
		m_rguPlayerScores[i] = 0;
		m_rgpPlayer[i] = NULL;
		//sprintf ( tStr , "Player %d" , i+1 );
		strcpy ( tStr , "Player" );
		strcpy ( m_rgpPlayerName[i] , tStr );
	}

	// Seed random num generator
	srand( (uint32)time( NULL ) );


	/*m_nNumWorkshopItems = 0;
	for (uint32 i = 0; i < MAX_WORKSHOP_ITEMS; ++i)
	{
		m_rgpWorkshopItems[i] = NULL;
	}*/

	// initialize P2P auth engine
	m_pP2PAuthedGame = new CP2PAuthedGame( );

	// Create matchmaking menus
	m_pServerBrowser = new CServerBrowser(  );
	m_pLobbyBrowser = new CLobbyBrowser(  );
	m_pLobby = new CLobby(  );


	// Init stats
	m_pStatsAndAchievements = NULL;
	//m_pStatsAndAchievements = new CStatsAndAchievements(  );
	m_pLeaderboards = new CLeaderboards(  );

	// Remote Storage page
	m_pRemoteStorage = new CRemoteStorage(  );


	StartCounter();
	// P2P voice chat 
	m_pVoiceChat = NULL;
	//m_pVoiceChat = new CVoiceChat();

	//LoadWorkshopItems();
}


//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CClient::~CClient()
{

#ifdef _DEBUG_LOG_
	log("CClient::~CClient()" , m_eGameState );
#endif

	DisconnectFromServer();

	if ( m_pP2PAuthedGame )
	{
		m_pP2PAuthedGame->EndGame();
		delete m_pP2PAuthedGame;
		m_pP2PAuthedGame = NULL;
	}

	if ( m_pServer )
	{
		delete m_pServer;
		m_pServer = NULL; 
	}

	if ( m_pStatsAndAchievements )
		delete m_pStatsAndAchievements;

	if ( m_pServerBrowser )
		delete m_pServerBrowser; 

	if ( m_pVoiceChat )
		delete m_pVoiceChat;

	for( uint32 i = 0; i < MAX_PLAYERS_PER_SERVER; ++i )
	{
		if ( m_rgpPlayer[i] )
		{
			delete m_rgpPlayer[i];
			m_rgpPlayer[i] = NULL;
		}
	}
	
	/*for (uint32 i = 0; i < MAX_WORKSHOP_ITEMS; ++i)
	{
		if ( m_rgpWorkshopItems[i] )
		{
			delete m_rgpWorkshopItems[i];
			m_rgpWorkshopItems[i] = NULL;
		}
	}*/
}


//-----------------------------------------------------------------------------
// Purpose: Tell the connected server we are disconnecting (if we are connected)
//-----------------------------------------------------------------------------
void CClient::DisconnectFromServer()
{

#ifdef _DEBUG_LOG_
	log("CClient::DisconnectFromServer()" , m_eGameState );
#endif

	if ( m_eConnectedStatus != k_EClientNotConnected )
	{
#ifdef USE_GS_AUTH_API
		if ( m_hAuthTicket != k_HAuthTicketInvalid )
			SteamUser()->CancelAuthTicket( m_hAuthTicket );
		m_hAuthTicket = k_HAuthTicketInvalid;
#else
		SteamUser()->AdvertiseGame( k_steamIDNil, 0, 0 );
#endif

		MsgClientLeavingServer_t msg;
		BSendServerData( &msg, sizeof(msg) );
		m_eConnectedStatus = k_EClientNotConnected;		

	}
	if ( m_pP2PAuthedGame )
	{
		m_pP2PAuthedGame->EndGame();
	}

	if ( m_pVoiceChat )
	{
		m_pVoiceChat->StopVoiceChat();
	}

	// forget the game server ID
	if ( m_steamIDGameServer.IsValid() )
	{
		SteamNetworking()->CloseP2PSessionWithUser( m_steamIDGameServer );
		m_steamIDGameServer = CSteamID();
	}

	// if we were playing a game and the server has dropped, perhaps we could take over
	if ( isPlayingOnAServer )
		CheckIfIShouldTakeOverServer();
}


//-----------------------------------------------------------------------------
// Purpose: Receive basic server info from the server after we initiate a connection
//-----------------------------------------------------------------------------
void CClient::OnReceiveServerInfo( CSteamID steamIDGameServer, bool bVACSecure, const char *pchServerName )
{

#ifdef _DEBUG_LOG_
	log("CClient::OnReceiveServerInfo()" , m_eGameState , m_eConnectedStatus );
#endif

	m_eConnectedStatus = k_EClientConnectedPendingAuthentication;
	m_steamIDGameServer = steamIDGameServer;

	// look up the servers IP and Port from the connection
	P2PSessionState_t p2pSessionState;
	SteamNetworking()->GetP2PSessionState( steamIDGameServer, &p2pSessionState );
	m_unServerIP = p2pSessionState.m_nRemoteIP;
	m_usServerPort = p2pSessionState.m_nRemotePort;

	// set how to connect to the game server, using the Rich Presence API
	// this lets our friends connect to this game via their friends list
	UpdateRichPresenceConnectionInfo();

	MsgClientBeginAuthentication_t msg;
#ifdef USE_GS_AUTH_API
	char rgchToken[1024];
	uint32 unTokenLen = 0;
	m_hAuthTicket = SteamUser()->GetAuthSessionTicket( rgchToken, sizeof( rgchToken ), &unTokenLen );
	msg.SetToken( rgchToken, unTokenLen );

#ifdef _DEBUG_LOG_
	log("CClient::OnReceiveServerInfo() - Getting Authorisation Ticket" , m_eGameState , m_eConnectedStatus );
#endif

#else
	// When you aren't using Steam auth you can still call AdvertiseGame() so you can communicate presence data to the friends
	// system. Make sure to pass k_steamIDNonSteamGS
	uint32 unTokenLen = SteamUser()->AdvertiseGame( k_steamIDNonSteamGS, m_unServerIP, m_usServerPort );
	msg.SetSteamID( SteamUser()->GetSteamID().ConvertToUint64() );
#endif

	Steamworks_TestSecret();

	if ( msg.GetTokenLen() < 1 )
	{
		OutputDebugString( "Warning: Looks like GetAuthSessionTicket didn't give us a good ticket\n" );

#ifdef _DEBUG_LOG_
	log("CClient::OnReceiveServerInfo() - Did not receive a good ticket" , m_eGameState , m_eConnectedStatus );
#endif

	}

	BSendServerData( &msg, sizeof(msg) );
}


//-----------------------------------------------------------------------------
// Purpose: Receive an authentication response from the server
//-----------------------------------------------------------------------------
void CClient::OnReceiveServerAuthenticationResponse( bool bSuccess, uint32 uPlayerPosition )
{

#ifdef _DEBUG_LOG_
	log("CClient::OnReceiveServerAuthenticationResponse()" , m_eGameState , m_eConnectedStatus, int(bSuccess) );
	log("CClient::OnReceiveServerAuthenticationResponse() - position" , uPlayerPosition );
#endif

	if ( !bSuccess )
	{
		SetConnectionFailureText( "Connection failure.\nMultiplayer authentication failed\n" );
		SetGameState( k_EClientGameConnectionFailure );
#ifdef _DEBUG_LOG_
	log("CClient::DisconnectFromServer() - disconnecting due to auth response" , m_eGameState );
#endif
		DisconnectFromServer();


#ifdef _DEBUG_LOG_
	log("CClient::OnReceiveServerAuthenticationResponse() - Failed" , m_eGameState , m_eConnectedStatus );
#endif

	}
	else
	{


#ifdef _DEBUG_LOG_
	log("CClient::OnReceiveServerAuthenticationResponse() - Done (poss dupe)" , m_eGameState , m_eConnectedStatus );
#endif

		// Is this a duplicate message? If so ignore it...
		if ( m_eConnectedStatus == k_EClientConnectedAndAuthenticated && m_uPlayerIndex == uPlayerPosition )
			return;

		m_uPlayerIndex = uPlayerPosition;
		m_eConnectedStatus = k_EClientConnectedAndAuthenticated;
		isPlayingOnAServer = true;

#ifdef _DEBUG_LOG_
	log("CClient::OnReceiveServerAuthenticationResponse() - Done for real" , m_eGameState , m_eConnectedStatus );
#endif

		// set information so our friends can join the lobby
		UpdateRichPresenceConnectionInfo();

		// send a ping, to measure round-trip time
		m_ulPingSentTime = (uint64)GetCounterPassedTotal();
		MsgClientPing_t msg;
		BSendServerData( &msg, sizeof( msg ) );
	}
}


#ifdef _DEBUG_LOG_ 
EClientGameState oldState = k_EClientGameStartServer;
#endif

//-----------------------------------------------------------------------------
// Purpose: Handles receiving a state update from the game server
//-----------------------------------------------------------------------------
void CClient::OnReceiveServerUpdate( ServerSteamUpdateData_t *pUpdateData )
{


#ifdef _DEBUG_LOG_
	if ( oldState != m_eGameState )
	{
		log("CClient::OnReceiveServerUpdate() - state changed " , m_eGameState , m_eConnectedStatus );
		oldState = m_eGameState;
	}

#endif

	// Update our client state based on what the server tells us
	
	switch( pUpdateData->GetServerGameState() )
	{
	case k_EServerWaitingForPlayers:
		if ( m_eGameState == k_EClientGameQuitMenu )
			break;
		else if (m_eGameState == k_EClientGameMenu )
			break;
		else if ( m_eGameState == k_EClientGameExiting )
			break;

		SetGameState( k_EClientGameWaitingForPlayers );
		break;
	case k_EServerActive:
		if ( m_eGameState == k_EClientGameQuitMenu )
			break;
		else if (m_eGameState == k_EClientGameMenu )
			break;
		else if ( m_eGameState == k_EClientGameExiting )
			break;

		SetGameState( k_EClientGameActive );
		break;
	case k_EServerDraw:
		if ( m_eGameState == k_EClientGameQuitMenu )
			break;
		else if ( m_eGameState == k_EClientGameMenu )
			break;
		else if ( m_eGameState == k_EClientGameExiting )
			break;

		SetGameState( k_EClientGameDraw );
		break;
	case k_EServerWinner:
		if ( m_eGameState == k_EClientGameQuitMenu )
			break;
		else if ( m_eGameState == k_EClientGameMenu )
			break;
		else if ( m_eGameState == k_EClientGameExiting )
			break;

		SetGameState( k_EClientGameWinner );
		break;
	case k_EServerExiting:
		if ( m_eGameState == k_EClientGameExiting )
			break;

		SetGameState( k_EClientGameMenu );
		break;
	}

	// Update scores
	/*for( int i=0; i < MAX_PLAYERS_PER_SERVER; ++i )
	{
		m_rguPlayerScores[i] = pUpdateData->GetPlayerScore(i);
	}*/

	// Update who won last
	//m_uPlayerWhoWonGame = pUpdateData->GetPlayerWhoWon();

	if ( m_pP2PAuthedGame )
	{
		// has the player list changed?
		if ( m_pServer )
		{
			// if i am the server owner i need to auth everyone who wants to play
			// assume i am in slot 0, so start at slot 1
			for( uint32 i=1; i < MAX_PLAYERS_PER_SERVER; ++i )
			{
				CSteamID steamIDNew( pUpdateData->GetPlayerSteamID(i) );
				if ( steamIDNew == SteamUser()->GetSteamID() )
				{
					OutputDebugString( "Server player slot 0 is not server owner.\n" );
				}
				else if ( steamIDNew != m_rgSteamIDPlayers[i] )
				{
					if ( m_rgSteamIDPlayers[i].IsValid() )
					{
						m_pP2PAuthedGame->PlayerDisconnect( i );
					}
					if ( steamIDNew.IsValid() )
					{
						m_pP2PAuthedGame->RegisterPlayer( i, steamIDNew );
					}
				}
			}
		}
		else
		{
			// i am just a client, i need to auth the game owner ( slot 0 )
			CSteamID steamIDNew( pUpdateData->GetPlayerSteamID( 0 ) );
			if ( steamIDNew == SteamUser()->GetSteamID() )
			{
				OutputDebugString( "Server player slot 0 is not server owner.\n" );
			}
			else if ( steamIDNew != m_rgSteamIDPlayers[0] )
			{
				if ( m_rgSteamIDPlayers[0].IsValid() )
				{
					OutputDebugString( "Server player slot 0 has disconnected - but thats the server owner.\n" );
					m_pP2PAuthedGame->PlayerDisconnect( 0 );
				}
				if ( steamIDNew.IsValid() )
				{
					m_pP2PAuthedGame->StartAuthPlayer( 0, steamIDNew );
				}
			}
		}
	}

	// update all players that are active
	if ( m_pVoiceChat )
		m_pVoiceChat->MarkAllPlayersInactive();

	// Update the players
	for( uint32 i=0; i < MAX_PLAYERS_PER_SERVER; ++i )
	{
		// Update steamid array with data from server
		m_rgSteamIDPlayers[i].SetFromUint64( pUpdateData->GetPlayerSteamID( i ) );
		playerSteamIDs[i].SetFromUint64( pUpdateData->GetPlayerSteamID( i ) );

		if ( pUpdateData->GetPlayerActive( i ) )
		{
			// Check if we have a player created locally for this player slot, if not create it
			if ( !m_rgpPlayer[i] )
			{
				ServerPlayerUpdateData_t *pPlayerData = pUpdateData->AccessPlayerUpdateData( i );
				m_rgpPlayer[i] = new CPlayer();
				if ( i == m_uPlayerIndex )
				{
					// is the local player
				}
			}

			if ( i == m_uPlayerIndex )
				m_rgpPlayer[i]->SetIsLocalPlayer( 1 );
			else
				m_rgpPlayer[i]->SetIsLocalPlayer( 0 );

			m_rgpPlayer[i]->OnReceiveServerUpdate( pUpdateData->AccessPlayerUpdateData( i ) );			

			if ( m_pVoiceChat )
				m_pVoiceChat->MarkPlayerAsActive( m_rgSteamIDPlayers[i] );

		}
		else
		{
			// Make sure we don't have a player locally for this slot
			if ( m_rgpPlayer[i] )
			{
				delete m_rgpPlayer[i];
				m_rgpPlayer[i] = NULL;
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Used to transition game state
//-----------------------------------------------------------------------------
void CClient::SetGameState( EClientGameState eState )
{
	if ( m_eGameState == eState )
		return;

#ifdef _DEBUG_LOG_
		log("** CLIENT - NEW STATE **" , m_eGameState , m_eConnectedStatus );
#endif

	m_bTransitionedGameState = true;
	m_ulStateTransitionTime = (uint64)GetCounterPassedTotal();
	m_eGameState = eState;

	// Let the stats handler check the state (so it can detect wins, losses, etc...)
	//m_pStatsAndAchievements->OnGameStateChange( eState );

	// update any rich presence state
	UpdateRichPresenceConnectionInfo();
}


//-----------------------------------------------------------------------------
// Purpose: set the error string to display in the UI
//-----------------------------------------------------------------------------
void CClient::SetConnectionFailureText( const char *pchErrorText )
{

#ifdef _DEBUG_LOG_
		log("CClient::SetConnectionFailureText()" , m_eGameState , m_eConnectedStatus );
		log( (char*)pchErrorText );
#endif

	sprintf_safe( m_rgchErrorText, "%s", pchErrorText );
}


//-----------------------------------------------------------------------------
// Purpose: Send data to the current server
//-----------------------------------------------------------------------------
bool CClient::BSendServerData( const void *pData, uint32 nSizeOfData )
{

#ifdef _DEBUG_LOG_
		//log("CClient::BSendServerData()" , m_eGameState , m_eConnectedStatus );
#endif

	if ( !SteamNetworking()->SendP2PPacket( m_steamIDGameServer, pData, nSizeOfData, k_EP2PSendUnreliable ) )
	{
#ifdef _DEBUG_LOG_
		log("CClient::BSendServerData() - Fail" , m_eGameState , m_eConnectedStatus );
#endif

		OutputDebugString( "Failed sending data to server\n" );
		return false;
	}

#ifdef _DEBUG_LOG_
		//log("CClient::BSendServerData() - Done" , m_eGameState , m_eConnectedStatus );
#endif

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Initiates a connection to a server
//-----------------------------------------------------------------------------
void CClient::InitiateServerConnection( uint32 unServerAddress, const int32 nPort )
{

#ifdef _DEBUG_LOG_
		log("CClient::InitiateServerConnection()" , m_eGameState , m_eConnectedStatus );
#endif

	if ( m_eGameState == k_EClientInLobby && m_steamIDLobby.IsValid() )
	{
		SteamMatchmaking()->LeaveLobby( m_steamIDLobby );
	}

	SetGameState( k_EClientGameConnecting );

	// Update when we last retried the connection, as well as the last packet received time so we won't timeout too soon,
	// and so we will retry at appropriate intervals if packets drop
	m_ulLastNetworkDataReceivedTime = m_ulLastConnectionAttemptRetryTime = (uint64)GetCounterPassedTotal(); // dave

	// ping the server to find out what it's steamID is
	m_unServerIP = unServerAddress;
	m_usServerPort = (uint16)nPort;
	m_GameServerPing.RetrieveSteamIDFromGameServer( this, m_unServerIP, m_usServerPort );
}


//-----------------------------------------------------------------------------
// Purpose: Initiates a connection to a server via P2P (NAT-traversing) connection
//-----------------------------------------------------------------------------
void CClient::InitiateServerConnection( CSteamID steamIDGameServer )
{

#ifdef _DEBUG_LOG_
		log("CClient::InitiateServerConnection()" , m_eGameState , m_eConnectedStatus );
#endif

	if ( m_eGameState == k_EClientInLobby && m_steamIDLobby.IsValid() )
	{
		SteamMatchmaking()->LeaveLobby( m_steamIDLobby );
	}

	SetGameState( k_EClientGameConnecting );

	m_steamIDGameServer = steamIDGameServer;

	// Update when we last retried the connection, as well as the last packet received time so we won't timeout too soon,
	// and so we will retry at appropriate intervals if packets drop
	m_ulLastNetworkDataReceivedTime = m_ulLastConnectionAttemptRetryTime = (uint64)GetCounterPassedTotal(); // dave

	// send the packet to the server
	MsgClientInitiateConnection_t msg;
	BSendServerData( &msg, sizeof( msg ) );
}


//-----------------------------------------------------------------------------
// Purpose: steam callback, triggered when our connection to another client fails
//-----------------------------------------------------------------------------
void CClient::OnP2PSessionConnectFail( P2PSessionConnectFail_t *pCallback )
{

#ifdef _DEBUG_LOG_
		log("CClient::OnP2PSessionConnectFail()" , m_eGameState , m_eConnectedStatus );
#endif

	if ( pCallback->m_steamIDRemote == m_steamIDGameServer )
	{

#ifdef _DEBUG_LOG_
		log("CClient::OnP2PSessionConnectFail() - Failed to make connection" , m_eGameState , m_eConnectedStatus );
#endif

		// failed, error out
		OutputDebugString( "Failed to make P2P connection, quiting server\n" );
		OnReceiveServerExiting();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Receives incoming network data
//-----------------------------------------------------------------------------
void CClient::ReceiveNetworkData()
{
	char rgchRecvBuf[1024];
	char *pchRecvBuf = rgchRecvBuf;
	uint32 cubMsgSize;
	for (;;)
	{
		// reset the receive buffer
		if ( pchRecvBuf != rgchRecvBuf )
		{
			free( pchRecvBuf );
			pchRecvBuf = rgchRecvBuf;
		}

		// see if there is any data waiting on the socket
		if ( !SteamNetworking()->IsP2PPacketAvailable( &cubMsgSize ) )
			break;

		// not enough space in default buffer
		// alloc custom size and try again
		if ( cubMsgSize > sizeof(rgchRecvBuf) )
		{
			pchRecvBuf = (char *)malloc( cubMsgSize );
		}
		CSteamID steamIDRemote;
		if ( !SteamNetworking()->ReadP2PPacket( pchRecvBuf, cubMsgSize, &cubMsgSize, &steamIDRemote ) )
			break;

		// see if it's from the game server
		if ( steamIDRemote == m_steamIDGameServer )
		{
			m_ulLastNetworkDataReceivedTime = (uint64)GetCounterPassedTotal(); // dave;

			// make sure we're connected
			if ( m_eConnectedStatus == k_EClientNotConnected && m_eGameState != k_EClientGameConnecting )
			{
				continue;
			}

			if ( cubMsgSize < sizeof( DWORD ) )
			{
				OutputDebugString( "Got garbage on client socket, too short\n" );
#ifdef _DEBUG_LOG_
	log("Received garbage" , 0 );
#endif
			}

			EMessage eMsg = (EMessage)LittleDWord( *(DWORD*)pchRecvBuf );
#ifdef _DEBUG_LOG_
			if ( DEBUG_FLAG_ON )
				log("Client Message Received" , eMsg );
#endif
			switch ( eMsg )
			{
			case k_EMsgReceipt:
				{
					if ( cubMsgSize == sizeof( MsgReceipt_t ) )
					{
						MsgReceipt_t* pmsg = (MsgReceipt_t*)pchRecvBuf;
						int index = pmsg->logID;

						GotReceipt(index);
					}
				} break;
			case k_EMsgClientPlayerName:
				{
					if ( cubMsgSize == sizeof( MsgClientPlayerName_t ) )
					{
						MsgClientPlayerName_t* pmsg = (MsgClientPlayerName_t*)pchRecvBuf;
						int index = pmsg->index;

						if ( m_rgpPlayer[index] )
						{
							strcpy ( m_rgpPlayerName[index] , pmsg->name );
						}
					}
				}
				break;
			case k_EMsgClientPlayerScore:
				{
					if ( cubMsgSize == sizeof( MsgClientPlayerScore_t ) )
					{
						MsgClientPlayerScore_t* pmsg = (MsgClientPlayerScore_t*)pchRecvBuf;
						int index = pmsg->index;

						if ( m_rgpPlayer[index] )
						{
							scores[index] = pmsg->score;
						}
					}
				}
				break;
			case k_EMsgClientShoot:
				{
					if ( cubMsgSize == sizeof( MsgClientShoot_t ) )
					{
						MsgClientShoot_t* pmsg = (MsgClientShoot_t*)pchRecvBuf;
						int index = pmsg->index;

						if ( m_rgpPlayer[index] )
						{
							playerShoot[index] = 1;
						}
					}
				}
				break;
			case k_EMsgEndGame:
				{
					if ( cubMsgSize == sizeof( MsgEndGame_t ) )
					{
						MsgEndGame_t* pmsg = (MsgEndGame_t*)pchRecvBuf;
						
						ServerIsShuttingDown = 1;
					}
				}
				break;
			case k_EMsgClientSetCollision:
				{
					if ( cubMsgSize == sizeof( MsgClientSetCollision_t ) )
					{
						MsgClientSetCollision_t* pmsg = (MsgClientSetCollision_t*)pchRecvBuf;

						tCollision c;
						c.index = pmsg->index;
						c.state = pmsg->state;
						collisionList.push_back(c);
					}
				}
				break;
			case k_EMsgClientPlayAnimation:
				{
					if ( cubMsgSize == sizeof( MsgClientPlayAnimation_t ) )
					{
						MsgClientPlayAnimation_t* pmsg = (MsgClientPlayAnimation_t*)pchRecvBuf;

						tAnimation a;
						a.index = pmsg->index;
						a.start = pmsg->start;
						a.end = pmsg->end;
						a.speed = pmsg->speed;
						animationList.push_back(a);
					}
				}
				break;
			case k_EMsgClientPlayerBullet:
				{
					if ( cubMsgSize == sizeof( MsgClientPlayerBullet_t ) )
					{
						MsgClientPlayerBullet_t* pmsg = (MsgClientPlayerBullet_t*)pchRecvBuf;
						int index = pmsg->index;

						//if ( m_rgpPlayer[index] )
						{

							if  ( bullets[index].on == 0 && pmsg->on == 1 )
							{
								bullets[index].x = pmsg->x;
								bullets[index].y = pmsg->y;
								bullets[index].z = pmsg->z;
								bullets[index].onTime = GetCounterPassedTotal();
							}
							

							bullets[index].newx = pmsg->x;
							bullets[index].newy = pmsg->y;
							bullets[index].newz = pmsg->z;

							float dx = bullets[index].newx - bullets[index].x;
							float dy = bullets[index].newy - bullets[index].y;
							float dz = bullets[index].newz - bullets[index].z;

							if ( pmsg->on == 0 )
							{
								bullets[index].x = -1000000;
								bullets[index].y = -1000000;
								bullets[index].z = -1000000;
							}


							bullets[index].anglex = pmsg->anglex;
							bullets[index].angley = pmsg->angley;
							bullets[index].anglez = pmsg->anglez;
							bullets[index].type = pmsg->type;
							bullets[index].on = pmsg->on;
						}
					}
				}
				break;
			case k_EMsgClientPlayerKeyState:
				{
					if ( cubMsgSize == sizeof( MsgClientPlayerKeyState_t ) )
					{
						MsgClientPlayerKeyState_t* pmsg = (MsgClientPlayerKeyState_t*)pchRecvBuf;
						int index = pmsg->index;

						if ( m_rgpPlayer[index] )
						{
							keystate[index][pmsg->key] = pmsg->state;
						}
					}
				}
				break;
			case k_EMsgClientServerReadyForSpawn:
				{
					if ( cubMsgSize == sizeof( MsgClientServerReadyForSpawn_t ) )
					{
						MsgClientServerReadyForSpawn_t* pmsg = (MsgClientServerReadyForSpawn_t*)pchRecvBuf;
						//int index = pmsg->index;

						//if ( m_rgpPlayer[index] )
						//{
							ServerSaysItIsOkayToStart = 1;
						//}
					}
				}
				break;
			case k_EMsgClientPlayerApplyDamage:
				{
					if ( cubMsgSize == sizeof( MsgClientPlayerApplyDamage_t ) )
					{
						MsgClientPlayerApplyDamage_t* pmsg = (MsgClientPlayerApplyDamage_t*)pchRecvBuf;
						int index = pmsg->index;

						MsgReceipt_t msg;
						msg.logID = pmsg->logID;
						SteamNetworking()->SendP2PPacket( m_steamIDGameServer, &msg, sizeof(MsgReceipt_t), k_EP2PSendUnreliable );

						if ( m_rgpPlayer[index] )
						{
							playerDamage += pmsg->amount;
							damageSource = pmsg->source;
							damageX = pmsg->x;
							damageY = pmsg->y;
							damageZ = pmsg->z;
							damageForce = pmsg->force;
							damageLimb = pmsg->limb;
						}

					}
				}
				break;
			case k_EMsgClientPlayerSetPlayerAlive:
				{
					if ( cubMsgSize == sizeof( MsgClientPlayerSetPlayerAlive_t ) )
					{
						MsgClientPlayerSetPlayerAlive_t* pmsg = (MsgClientPlayerSetPlayerAlive_t*)pchRecvBuf;
						int index = pmsg->index;

						if ( m_rgpPlayer[index] )
						{
							alive[index] = pmsg->state;
						}
					}
				}
				break;
			case k_EMsgClientSpawnObject:
				{
					if ( cubMsgSize == sizeof( MsgClientSpawnObject_t ) )
					{
						MsgClientSpawnObject_t* pmsg = (MsgClientSpawnObject_t*)pchRecvBuf;
						int index = pmsg->index;

						if ( m_rgpPlayer[index] )
						{
							tSpawn s;
							s.object = pmsg->object;
							s.source = pmsg->source;
							s.x = pmsg->x;
							s.y = pmsg->y;
							s.z = pmsg->z;

							spawnList.push_back(s);
						}
					}
				}
				break;
			case k_EMsgServerLua:
				{
					if ( cubMsgSize == sizeof( MsgServerLua_t ) )
					{
						MsgServerLua_t* pmsg = (MsgServerLua_t*)pchRecvBuf;

						// no ticket if goto position
						if ( pmsg->code != 18 && pmsg->code != 19 && pmsg->code > 4 )
						{
							MsgReceipt_t msg;
							msg.logID = pmsg->logID;
							SteamNetworking()->SendP2PPacket( m_steamIDGameServer, &msg, sizeof(MsgReceipt_t), k_EP2PSendUnreliable );
						}

						tLua l;
						l.code = pmsg->code;
						l.e = pmsg->e;
						l.v = pmsg->v;
						strcpy ( l.s , "" );

						luaList.insert( luaList.begin(),l);
					}
				}
				break;
			case k_EMsgServerLuaString:
				{
					if ( cubMsgSize == sizeof( MsgServerLuaString_t ) )
					{
						MsgServerLuaString_t* pmsg = (MsgServerLuaString_t*)pchRecvBuf;

						MsgReceipt_t msg;
						msg.logID = pmsg->logID;
						SteamNetworking()->SendP2PPacket( m_steamIDGameServer, &msg, sizeof(MsgReceipt_t), k_EP2PSendUnreliable );

						tLua l;
						l.code = pmsg->code;
						l.e = pmsg->e;
						l.v = 0;
						strcpy ( l.s , pmsg->s );

						luaList.insert( luaList.begin(),l);
					}
				}
				break;
			case k_EMsgClientDeleteObject:
				{
					if ( cubMsgSize == sizeof( MsgClientDeleteObject_t ) )
					{
						MsgClientDeleteObject_t* pmsg = (MsgClientDeleteObject_t*)pchRecvBuf;
						int index = pmsg->index;

						if ( m_rgpPlayer[index] )
						{
							deleteList.push_back(pmsg->object);
							deleteListSource.push_back(pmsg->index);
						}
					}
				}
				break;
			case k_EMsgClientDestroyObject:
				{
					if ( cubMsgSize == sizeof( MsgClientDestroyObject_t ) )
					{
						MsgClientDestroyObject_t* pmsg = (MsgClientDestroyObject_t*)pchRecvBuf;
						int index = pmsg->index;

						if ( m_rgpPlayer[index] )
						{
							destroyList.push_back(pmsg->object);
						}
					}
				}
				break;
			case k_EMsgClientKilledBy:
				{
					if ( cubMsgSize == sizeof( MsgClientKilledBy_t ) )
					{
						MsgClientKilledBy_t* pmsg = (MsgClientKilledBy_t*)pchRecvBuf;
						int index = pmsg->index;
						int killedBy = pmsg->killedBy;

						killedSource[pmsg->index] = pmsg->killedBy;
						killedX[pmsg->index] = pmsg->x;
						killedY[pmsg->index] = pmsg->y;
						killedZ[pmsg->index] = pmsg->z;
						killedForce[pmsg->index] = pmsg->force;
						killedLimb[pmsg->index] = pmsg->limb;

						MsgReceipt_t msg;
						msg.logID = pmsg->logID;
						SteamNetworking()->SendP2PPacket( m_steamIDGameServer, &msg, sizeof(MsgReceipt_t), k_EP2PSendUnreliable );

						if ( m_rgpPlayer[index] )
						{
							tMessage s;
							sprintf ( s.message , "%s was killed by %s" , Client()->m_rgpPlayerName[index] , Client()->m_rgpPlayerName[killedBy] );
							messageList.push_back(s);
						}
					}
				}
				break;
			case k_EMsgChat:
				{
					if ( cubMsgSize == sizeof( MsgChat_t ) )
					{
						MsgChat_t* pmsg = (MsgChat_t*)pchRecvBuf;

						tChat c;
						sprintf ( c.msg , pmsg->msg );
						chatList.push_back(c);
					}
				}
				break;
			case k_EMsgClientKilledSelf:
				{
					if ( cubMsgSize == sizeof( MsgClientKilledSelf_t ) )
					{
						MsgClientKilledSelf_t* pmsg = (MsgClientKilledSelf_t*)pchRecvBuf;
						int index = pmsg->index;

						if ( m_rgpPlayer[index] )
						{
							tMessage s;
							sprintf ( s.message , "%s killed themself" , Client()->m_rgpPlayerName[index] );
							messageList.push_back(s);
						}
					}
				}
				break;
			case k_EMsgClientLeft:
				{
					if ( cubMsgSize == sizeof( MsgClientLeft_t ) )
					{
						MsgClientLeft_t* pmsg = (MsgClientLeft_t*)pchRecvBuf;
						int index = pmsg->index;

						tMessage s;
						sprintf ( s.message , "%s has left the game" , Client()->m_rgpPlayerName[index]);
						messageList.push_back(s);

						strcpy ( Client()->m_rgpPlayerName[index] , "Player" );

					}
				}
				break;
			case k_EMsgClientPlayerAppearance:
				{
					if ( cubMsgSize == sizeof( MsgClientPlayerAppearance_t ) )
					{
						MsgClientPlayerAppearance_t* pmsg = (MsgClientPlayerAppearance_t*)pchRecvBuf;
						int index = pmsg->index;
						int appearance = pmsg->appearance;

						if ( m_rgpPlayer[index] )
						{
							playerAppearance[index] = appearance;
						}
					}
				}
				break;
			case k_EMsgClientSetSendFileCount:
				{
					if ( cubMsgSize == sizeof( MsgClientSetSendFileCount_t ) )
					{
						MsgClientSetSendFileCount_t* pmsg = (MsgClientSetSendFileCount_t*)pchRecvBuf;

						ServerFilesToReceive = pmsg->count;
						ServerFilesReceived = 0;
						IamSyncedWithServerFiles = 0;
						IamLoadedAndReady = 0;
						isEveryoneLoadedAndReady = 0;
						IamReadyToPlay = 0;
						isEveryoneReadyToPlay = 0;
					}
				}
				break;				
			case k_EMsgServerAvatarChangeMode:
				{
					if ( cubMsgSize == sizeof( MsgServerAvatarChangeMode_t ) )
					{
						MsgServerAvatarChangeMode_t* pmsg = (MsgServerAvatarChangeMode_t*)pchRecvBuf;

						syncedAvatarTextureMode = pmsg->mode;
						int result = pmsg->result;
						int result2 = pmsg->result2;

						// apply any results sent
						switch (syncedAvatarTextureMode)
						{
							case SYNC_AVATAR_TEX_MODE_SENDING: // servers lets us know how many textures there are to get
							{
								syncedAvatarHowManyTextures = result;
								syncedAvatarHowManyTexturesReceived = 0;
							} break;
						}
					}
				}
				break;
			case k_EMsgClientSendFileBegin:
				{

					if ( cubMsgSize == sizeof( MsgClientSendFileBegin_t ) )
					{
						MsgClientSendFileBegin_t* pmsg = (MsgClientSendFileBegin_t*)pchRecvBuf;

						if ( serverFile ) fclose ( serverFile );
						
						serverFile = fopen ( pmsg->fileName, "wb" );

						serverHowManyFileChunks = (int)ceil( (float)pmsg->fileSize / float(FILE_CHUNK_SIZE) );
						serverFileFileSize = pmsg->fileSize;

						IsWorkshopLoadingOn = 1;

						fileProgress = 0;
					}
				}
				break;
			case k_EMsgClientSendChunk:
				{

					if ( cubMsgSize == sizeof( MsgClientSendChunk_t ) )
					{
						MsgClientSendChunk_t* pmsg = (MsgClientSendChunk_t*)pchRecvBuf;

						if ( serverFile ) 
						{
							int chunkSize = FILE_CHUNK_SIZE;

							if (  pmsg->index == serverHowManyFileChunks )
							{
								if ( serverHowManyFileChunks == 1 )
									chunkSize = serverFileFileSize;
								else
									chunkSize = serverFileFileSize - ((serverHowManyFileChunks-1) * FILE_CHUNK_SIZE	);				
							}

							fileProgress = (int)ceil(((float)(pmsg->index * FILE_CHUNK_SIZE) / (float)serverFileFileSize )  * 100.0f);

							fwrite( &pmsg->chunk[0] , 1 , chunkSize , serverFile );
							//fflush ( serverFile ); // <-- this causes the file to fail to save for some reason

							if (  pmsg->index == serverHowManyFileChunks )
							{
								fclose ( serverFile );
								serverFile = NULL;
								ServerFilesReceived++;
								if ( ServerFilesReceived == 0 ) ServerFilesReceived = 1;

								if ( ServerFilesReceived >= ServerFilesToReceive ) 
								{
									IamSyncedWithServerFiles = 1;

									MsgClientPlayerIamSyncedWithServerFiles_t msg;
									msg.index = m_uPlayerIndex;
									SteamNetworking()->SendP2PPacket( m_steamIDGameServer, &msg, sizeof(MsgClientPlayerIamSyncedWithServerFiles_t), k_EP2PSendReliable );

								}
								else
									IamSyncedWithServerFiles = 0;
							}
						}
					}
				}
				break;
			case k_EMsgClientSendAvatarFileBeginClient:
				{

					if ( cubMsgSize == sizeof( MsgClientSendAvatarFileBeginClient_t ) )
					{
						MsgClientSendAvatarFileBeginClient_t* pmsg = (MsgClientSendAvatarFileBeginClient_t*)pchRecvBuf;

						int index = pmsg->index;

						char dest[MAX_PATH];
						sprintf ( dest, "%sentitybank\\user\\charactercreator\\customAvatar_%i_cc.dds" , steamRootPath , index );
						DeleteFile ( dest );

						if ( avatarFile[index] ) fclose ( avatarFile[index] );
						
						avatarFile[index] = fopen ( dest, "wb" );

						avatarHowManyFileChunks[index] = (int)ceil( (float)pmsg->fileSize / float(FILE_CHUNK_SIZE) );
						avatarFileFileSize[index] = pmsg->fileSize;

						//fileProgress = 0;
					}
				}
				break;
			case k_EMsgClientSendAvatarChunkClient:
				{

					if ( cubMsgSize == sizeof( MsgClientSendAvatarChunkClient_t ) )
					{
						MsgClientSendAvatarChunkClient_t* pmsg = (MsgClientSendAvatarChunkClient_t*)pchRecvBuf;

						int index = pmsg->index;

						if ( avatarFile[index] ) 
						{
							int chunkSize = (FILE_CHUNK_SIZE);

							if (  pmsg->index == avatarHowManyFileChunks[index] )
							{
								if ( avatarHowManyFileChunks[index] == 1 )
									chunkSize = avatarFileFileSize[index];
								else
									chunkSize = avatarFileFileSize[index] - ((avatarHowManyFileChunks[index]-1) * (FILE_CHUNK_SIZE)	);				
							}

							//fileProgress = ceil(((float)(pmsg->index * FILE_CHUNK_SIZE) / (float)serverFileFileSize )  * 100.0f);

							fwrite( &pmsg->chunk[0] , 1 , chunkSize , avatarFile[index] );

							if (  pmsg->count == avatarHowManyFileChunks[index] )
							{
								fclose ( avatarFile[index] );
								avatarFile[index] = NULL;
							}
						}
					}
				}
				break;
			case k_EMsgClientEveryoneLoadedAndReady:
				{
					if ( cubMsgSize == sizeof( MsgClientEveryoneLoadedAndReady_t ) )
					{
						isEveryoneLoadedAndReady = 1;

						MsgClientEveryoneLoadedAndReady_t *pMsg = (MsgClientEveryoneLoadedAndReady_t*)pchRecvBuf;
						MsgReceipt_t msg;
						msg.logID = pMsg->logID;
						SteamNetworking()->SendP2PPacket( m_steamIDGameServer, &msg, sizeof(MsgReceipt_t), k_EP2PSendUnreliable );
					}
				}
				break;
			case k_EMsgClientEveryoneReadyToPlay:
				{
					if ( cubMsgSize == sizeof( MsgClientEveryoneReadyToPlay_t ) )
					{
						//server_change_timeout = 1;
						if ( m_pServer && m_pServer->IsConnectedToSteam() )
						{
							m_pServer->ResetTimeouts();
							server_timeout_milliseconds = SERVER_TIMEOUT_MILLISECONDS;
							m_bSentPlayerName = false;
						}
						isEveryoneReadyToPlay = 1;

						MsgClientEveryoneReadyToPlay_t *pMsg = (MsgClientEveryoneReadyToPlay_t*)pchRecvBuf;
						MsgReceipt_t msg;
						msg.logID = pMsg->logID;
						SteamNetworking()->SendP2PPacket( m_steamIDGameServer, &msg, sizeof(MsgReceipt_t), k_EP2PSendUnreliable );

					}
				}
				break;
			case k_EMsgServerSendInfo:
				{
					if ( cubMsgSize != sizeof( MsgServerSendInfo_t ) )
					{
						OutputDebugString ("Bad server info msg\n" );
						continue;
					}
					MsgServerSendInfo_t *pMsg = (MsgServerSendInfo_t*)pchRecvBuf;

					// pull the IP address of the user from the socket
					OnReceiveServerInfo( CSteamID( pMsg->GetSteamIDServer() ), pMsg->GetSecure(), pMsg->GetServerName() );
				}
				break;
			case k_EMsgServerPassAuthentication:
				{
					if ( cubMsgSize != sizeof( MsgServerPassAuthentication_t ) )
					{
						OutputDebugString( "Bad accept connection msg\n" );
						continue;
					}
					MsgServerPassAuthentication_t *pMsg = (MsgServerPassAuthentication_t*)pchRecvBuf;

					// Our game client doesn't really care about whether the server is secure, or what its 
					// steamID is, but if it did we would pass them in here as they are part of the accept message
					OnReceiveServerAuthenticationResponse( true, pMsg->GetPlayerPosition() );
				}
				break;
			case k_EMsgServerFailAuthentication:
				{
					OnReceiveServerAuthenticationResponse( false, 0 );
				}
				break;
			case k_EMsgServerUpdateWorld:
				{
					if ( cubMsgSize != sizeof( MsgServerUpdateWorld_t ) )
					{
						OutputDebugString( "Bad server world update msg\n" );
						continue;
					}

					MsgServerUpdateWorld_t *pMsg = (MsgServerUpdateWorld_t*)pchRecvBuf;
					OnReceiveServerUpdate( pMsg->AccessUpdateData() );
				}
				break;
			case k_EMsgServerExiting:
				{
					if ( cubMsgSize != sizeof( MsgServerExiting_t ) )
					{
						OutputDebugString( "Bad server exiting msg\n" );
					}
					OnReceiveServerExiting();
				}
				break;
			case k_EMsgServerPingResponse:
				{
					uint64 ulTimePassedMS = (uint64)(GetCounterPassedTotal() - m_ulPingSentTime); // dave
					char rgchT[256];
					sprintf_safe( rgchT, "Round-trip ping time to server %d ms\n", (int)ulTimePassedMS );
					rgchT[ sizeof(rgchT) - 1 ] = 0;
					OutputDebugString( rgchT );
					m_ulPingSentTime = 0;
				}
				break;
			default:
				OutputDebugString( "Unhandled message from server\n" );
				break;
			}
		}
		else 
		{
			// the message is from another player
			EMessage eMsg = (EMessage)LittleDWord( *(DWORD*)pchRecvBuf );

			if ( m_pP2PAuthedGame->HandleMessage( eMsg, pchRecvBuf ) )
				continue; // this was a P2P auth message

			if ( m_pVoiceChat )
			{
				if ( m_pVoiceChat->HandleMessage( steamIDRemote, eMsg, pchRecvBuf ) )
					continue;
			}

			// Unhandled message
			OutputDebugString( "Received unknown message on our listen socket\n" );
		}
	}

	// if we're running a server, do that as well
	if ( m_pServer )
	{
		m_pServer->ReceiveNetworkData();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Handle the server telling us it is exiting
//-----------------------------------------------------------------------------
void CClient::OnReceiveServerExiting()
{

#ifdef _DEBUG_LOG_
	log("CClient::OnReceiveServerExiting()" , m_eGameState );
#endif

	if ( m_pP2PAuthedGame )
		m_pP2PAuthedGame->EndGame();

#ifdef USE_GS_AUTH_API
	if ( m_hAuthTicket != k_HAuthTicketInvalid )
	{
		SteamUser()->CancelAuthTicket( m_hAuthTicket );
	}
	m_hAuthTicket = k_HAuthTicketInvalid;
#else
	SteamUser()->AdvertiseGame( k_steamIDNil, 0, 0 );
#endif

	if ( m_eGameState != k_EClientGameActive )
		return;
	m_eConnectedStatus = k_EClientNotConnected;

	SetConnectionFailureText( "Game server has exited." );
	SetGameState( k_EClientGameConnectionFailure );
}


//-----------------------------------------------------------------------------
// Purpose: Steam is asking us to join a game, based on the user selecting
//			'join game' on a friend in their friends list 
//			the string comes from the "connect" field set in the friends' rich presence
//-----------------------------------------------------------------------------
void CClient::OnGameJoinRequested( GameRichPresenceJoinRequested_t *pCallback )
{

#ifdef _DEBUG_LOG_
		log("CClient::OnGameJoinRequested()" , m_eGameState , m_eConnectedStatus );
#endif

	// parse out the connect 
	const char *pchServerAddress, *pchLobbyID;
	bool bUseVR = false;	
	ParseCommandLine( pCallback->m_rgchConnect, &pchServerAddress, &pchLobbyID, &bUseVR );

	// exec
	ExecCommandLineConnect( pchServerAddress, pchLobbyID );
}

//-----------------------------------------------------------------------------
// Purpose: Finishes up entering a lobby of our own creation
//-----------------------------------------------------------------------------
void CClient::OnLobbyCreated( LobbyCreated_t *pCallback, bool bIOFailure )
{

#ifdef _DEBUG_LOG_
	log("CClient::OnLobbyCreated()" , m_eGameState , m_eConnectedStatus );
#endif

	if ( m_eGameState != k_EClientCreatingLobby )
		return;

#ifdef _DEBUG_LOG_
	log("CClient::OnLobbyCreated() - created" , m_eGameState , m_eConnectedStatus );
#endif

	// record which lobby we're in
	if ( pCallback->m_eResult == k_EResultOK )
	{
		// success
		m_steamIDLobby = pCallback->m_ulSteamIDLobby;
		lobbyIAmInID = m_steamIDLobby;
		m_pLobby->SetLobbySteamID( m_steamIDLobby );

		// set the name of the lobby if it's ours
		char rgchLobbyName[256];
		strcpy( rgchLobbyName, hostsLobbyName ); //"%s's lobby", SteamFriends()->GetPersonaName() );
		SteamMatchmaking()->SetLobbyData( m_steamIDLobby, "name", rgchLobbyName );

		// mark that we're in the lobby
		SetGameState( k_EClientInLobby );

#ifdef _DEBUG_LOG_
	log("CClient::OnLobbyCreated() - in lobby" , m_eGameState , m_eConnectedStatus );
#endif

	}
	else
	{
		// failed, show error
		SetConnectionFailureText( "Failed to create lobby (lost connection to Steam back-end servers." );
		SetGameState( k_EClientGameConnectionFailure );

#ifdef _DEBUG_LOG_
	log("CClient::OnLobbyCreated() - lost connect to steam back end servers" , m_eGameState , m_eConnectedStatus );
#endif
	}
}

//-----------------------------------------------------------------------------
// Purpose: Finishes up entering a lobby
//-----------------------------------------------------------------------------
void CClient::OnLobbyEntered( LobbyEnter_t *pCallback, bool bIOFailure )
{
	if ( m_eGameState != k_EClientJoiningLobby )
		return;

#ifdef _DEBUG_LOG_
	log("CClient::OnLobbyEntered()" , m_eGameState , m_eConnectedStatus );
#endif

	if ( pCallback->m_EChatRoomEnterResponse != k_EChatRoomEnterResponseSuccess )
	{
		// failed, show error
		SetConnectionFailureText( "Failed to enter lobby" );
		SetGameState( k_EClientGameConnectionFailure );

#ifdef _DEBUG_LOG_
	log("CClient::OnLobbyEntered() - failed" , m_eGameState , m_eConnectedStatus );
#endif

		return;
	}

	// success

	// move forward the state
	m_steamIDLobby = pCallback->m_ulSteamIDLobby;
	m_pLobby->SetLobbySteamID( m_steamIDLobby );
	SetGameState( k_EClientInLobby );

#ifdef _DEBUG_LOG_
	log("CClient::OnLobbyEntered() - in the lobby" , m_eGameState , m_eConnectedStatus );
#endif

}


//-----------------------------------------------------------------------------
// Purpose: Joins a game from a lobby
//-----------------------------------------------------------------------------
void CClient::OnLobbyGameCreated( LobbyGameCreated_t *pCallback )
{

#ifdef _DEBUG_LOG_
		log("CClient::OnLobbyGameCreated()" , m_eGameState , m_eConnectedStatus );
#endif

	if ( m_eGameState != k_EClientInLobby )
		return;

	// join the game server specified, via whichever method we can
	if ( CSteamID( pCallback->m_ulSteamIDGameServer ).IsValid() )
	{
		InitiateServerConnection( CSteamID( pCallback->m_ulSteamIDGameServer ) );
	}
}

void CClient::OnLobbyChatMessage ( LobbyChatMsg_t *pCallback )
{

//	if ( m_eGameState != k_EClientInLobby )
	//	return;

	//if ( pCallback->m_ulSteamIDLobby == lobbyIAmInID.GetAccountID() )
	{

		CSteamID speaker;
		EChatEntryType entryType;
		char msg[82];
		int cubData=sizeof(msg);
		SteamMatchmaking()->GetLobbyChatEntry( lobbyIAmInID, pCallback->m_iChatID, &speaker, msg, cubData, &entryType);
		if (entryType==k_EChatEntryTypeChatMsg && speaker != SteamUser()->GetSteamID() )
		{
			bool found = false;
			uint32 chatID = pCallback->m_iChatID;
			for ( int c = 0 ; c < (int)lobbyChatIDs.size() ; c++ )
			{
				if ( chatID == lobbyChatIDs[c] )
				{
					found = true;
					break;
				}
			}

			if ( lobbyChatIDs.size() > 100 )
				lobbyChatIDs.erase( lobbyChatIDs.begin() );

			if ( !found )
			{
				tChat c;
				sprintf ( c.msg , msg );
				chatList.push_back(c);
				lobbyChatIDs.push_back(chatID);
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: a large avatar image has been loaded for us
//-----------------------------------------------------------------------------
void CClient::OnAvatarImageLoaded( AvatarImageLoaded_t *pCallback )
{
}


//-----------------------------------------------------------------------------
// Purpose: Handles menu actions in a lobby
//-----------------------------------------------------------------------------
void CClient::OnMenuSelection( LobbyMenuItem_t selection )
{

#ifdef _DEBUG_LOG_
		log("CClient::OnMenuSelection()" , m_eGameState , m_eConnectedStatus );
#endif

	if ( selection.m_eCommand == LobbyMenuItem_t::k_ELobbyMenuItemLeaveLobby )
	{
		// leave the lobby
		SteamMatchmaking()->LeaveLobby( m_steamIDLobby );
		m_steamIDLobby = CSteamID();

		// return to main menu
		SetGameState( k_EClientGameMenu );
	}
	else if ( selection.m_eCommand == LobbyMenuItem_t::k_ELobbyMenuItemToggleReadState )
	{
		// update our state
		bool bOldState = ( 1 == atoi( SteamMatchmaking()->GetLobbyMemberData( m_steamIDLobby, SteamUser()->GetSteamID(), "ready" ) ) );
		bool bNewState = !bOldState;
		// publish to everyone
		SteamMatchmaking()->SetLobbyMemberData( m_steamIDLobby, "ready", bNewState ? "1" : "0" );
	}
	else if ( selection.m_eCommand == LobbyMenuItem_t::k_ELobbyMenuItemStartGame )
	{
		// make sure we're not already starting a server
		if ( m_pServer )
			return;

		// broadcast to everyone in the lobby that the game is starting
		SteamMatchmaking()->SetLobbyData( m_steamIDLobby, "game_starting", "1" );
		
		// start a local game server
		m_pServer = new CSteamServer( );
		// we'll have to wait until the game server connects to the Steam server back-end 
		// before telling all the lobby members to join (so that the NAT traversal code has a path to contact the game server)
		OutputDebugString( "Game server being created; game will start soon.\n" );
	}
	else if ( selection.m_eCommand == LobbyMenuItem_t::k_ELobbyMenuItemInviteToLobby )
	{
		SteamFriends()->ActivateGameOverlayInviteDialog( selection.m_steamIDLobby );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Handles menu actions when viewing a leaderboard
//-----------------------------------------------------------------------------
void CClient::OnMenuSelection( LeaderboardMenuItem_t selection )
{

#ifdef _DEBUG_LOG_
		log("CClient::OnMenuSelection() - leaderboard" , m_eGameState , m_eConnectedStatus );
#endif

	m_pLeaderboards->OnMenuSelection( selection );
}


//-----------------------------------------------------------------------------
// Purpose: Handles menu actions when viewing the remote storage sync screen
//-----------------------------------------------------------------------------
void CClient::OnMenuSelection( ERemoteStorageSyncMenuCommand selection )
{

#ifdef _DEBUG_LOG_
		log("CClient::OnMenuSelection() - remote storage" , m_eGameState , m_eConnectedStatus );
#endif

	m_pRemoteStorage->OnMenuSelection( selection );
}


//-----------------------------------------------------------------------------
// Purpose: does work on transitioning from one game state to another
//-----------------------------------------------------------------------------
void CClient::OnGameStateChanged( EClientGameState eGameStateNew )
{

#ifdef _DEBUG_LOG_
		log("CClient::OnGameStateChanged()" , m_eGameState , m_eConnectedStatus );
#endif

	if ( m_eGameState == k_EClientFindInternetServers )
	{
		// If we are just opening the find servers screen, then start a refresh
		m_pServerBrowser->RefreshInternetServers();
		SteamFriends()->SetRichPresence( "status", "Finding an internet game" );
	}
	else if ( m_eGameState == k_EClientFindLANServers )
	{
		m_pServerBrowser->RefreshLANServers();
		SteamFriends()->SetRichPresence( "status", "Finding a LAN game" );
	}
	else if ( m_eGameState == k_EClientCreatingLobby )
	{
		// start creating the lobby
		if ( !m_SteamCallResultLobbyCreated.IsActive() )
		{
			// ask steam to create a lobby
			SteamAPICall_t hSteamAPICall = SteamMatchmaking()->CreateLobby( k_ELobbyTypePublic /* public lobby, anyone can find it */, MAX_PLAYERS_PER_SERVER );
			// set the function to call when this completes
			m_SteamCallResultLobbyCreated.Set( hSteamAPICall, this, &CClient::OnLobbyCreated );
		}
		SteamFriends()->SetRichPresence( "status", "Creating a lobby" );
	}
	else if ( m_eGameState == k_EClientFindLobby )
	{
		m_pLobbyBrowser->Refresh();
		SteamFriends()->SetRichPresence( "status", "Main menu: finding lobbies" );
	}
	else if ( m_eGameState == k_EClientGameMenu )
	{
		// we've switched out to the main menu

		// Tell the server we have left if we are connected
#ifdef _DEBUG_LOG_
	log("DisconnectFromServer() due to going back to main menu" , m_eGameState );
#endif
		DisconnectFromServer();

		// shut down any server we were running
		if ( m_pServer )
		{
			delete m_pServer;
			m_pServer = NULL;
		}

		SteamFriends()->SetRichPresence( "status", "Main menu" );
	}
	else if ( m_eGameState == k_EClientGameWinner || m_eGameState == k_EClientGameDraw )
	{
		// game over.. update the leaderboard
		//m_pLeaderboards->UpdateLeaderboards( m_pStatsAndAchievements );
	}
	else if ( m_eGameState == k_EClientLeaderboards )
	{
		// we've switched to the leaderboard menu
		m_pLeaderboards->Show();
		SteamFriends()->SetRichPresence( "status", "Viewing leaderboards" );
	}
	else if ( m_eGameState == k_EClientClanChatRoom )
	{
		// we've switched to the leaderboard menu
		//m_pClanChatRoom->Show();
		//SteamFriends()->SetRichPresence( "status", "Chatting" );
	}
	else if ( m_eGameState == k_EClientGameActive )
	{
		// start voice chat 
		//m_pVoiceChat->StartVoiceChat();
		SteamFriends()->SetRichPresence( "status", "In match" );
	}
	else if ( m_eGameState == k_EClientRemoteStorage )
	{
		// we've switched to the remote storage menu
		m_pRemoteStorage->Show();
		SteamFriends()->SetRichPresence( "status", "Viewing remote storage" );
	}
	else if ( m_eGameState == k_EClientMusic )
	{
	}
}

//-----------------------------------------------------------------------------
// Purpose: Handles notification of a steam ipc failure
// we may get multiple callbacks, one for each IPC operation we attempted
// since the actual failure, so protect ourselves from alerting more than once.
//-----------------------------------------------------------------------------
void CClient::OnIPCFailure( IPCFailure_t *failure )
{

#ifdef _DEBUG_LOG_
		log("CClient::OnIPCFailure()" , m_eGameState , m_eConnectedStatus );
#endif

	static bool bExiting = false;
	if ( !bExiting )
	{
		OutputDebugString( "Steam IPC Failure, shutting down\n" );
#if defined( _WIN32 )
		::MessageBoxA( NULL, "Connection to Steam Lost, Exiting", "Steam Connection Error", MB_OK );
#endif
		bExiting = true;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Handles notification of a Steam shutdown request since a Windows
// user in a second concurrent session requests to play this game. Shutdown
// this process immediately if possible.
//-----------------------------------------------------------------------------
void CClient::OnSteamShutdown( SteamShutdown_t *callback )
{

#ifdef _DEBUG_LOG_
		log("CClient::OnSteamShutdown()" , m_eGameState , m_eConnectedStatus );
#endif

	static bool bExiting = false;
	if ( !bExiting )
	{
		OutputDebugString( "Steam shutdown request, shutting down\n" );
		bExiting = true;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Handles notification that we are now connected to Steam
//-----------------------------------------------------------------------------
void CClient::OnSteamServersConnected( SteamServersConnected_t *callback )
{
#ifdef _DEBUG_LOG_
		log("CClient::OnSteamServersConnected()" , m_eGameState , m_eConnectedStatus );
#endif

	if ( SteamUser()->BLoggedOn() )
		m_eGameState = k_EClientGameMenu;
	else
	{

#ifdef _DEBUG_LOG_
		log("CClient::OnSteamServersConnected() - not logged on" , m_eGameState , m_eConnectedStatus );
#endif

		OutputDebugString( "Got SteamServersConnected_t, but not logged on?\n" );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Handles notification that we are now connected to Steam
//-----------------------------------------------------------------------------
void CClient::OnSteamServersDisconnected( SteamServersDisconnected_t *callback )
{

#ifdef _DEBUG_LOG_
		log("CClient::OnSteamServersDisconnected()" , m_eGameState , m_eConnectedStatus );
#endif

	SetGameState( k_EClientConnectingToSteam );
	//m_pConnectingMenu->OnConnectFailure();
	OutputDebugString( "Got SteamServersDisconnected_t\n" );
}


//-----------------------------------------------------------------------------
// Purpose: Handles notification that the Steam overlay is shown/hidden, note, this
// doesn't mean the overlay will or will not draw, it may still draw when not active.
// This does mean the time when the overlay takes over input focus from the game.
//-----------------------------------------------------------------------------
void CClient::OnGameOverlayActivated( GameOverlayActivated_t *callback )
{

#ifdef _DEBUG_LOG_
		log("CClient::OnGameOverlayActivated()" , m_eGameState , m_eConnectedStatus );
#endif

	if ( callback->m_bActive )
		SteamOverlayActive = 1;
	else
		SteamOverlayActive = 0;
}


//-----------------------------------------------------------------------------
// Purpose: Handle the callback from the user clicking a steam://gamewebcallback/ link in the overlay browser
//	You can use this to add support for external site signups where you want to pop back into the browser
//  after some web page signup sequence, and optionally get back some detail about that.
//-----------------------------------------------------------------------------
void CClient::OnGameWebCallback( GameWebCallback_t *callback )
{

#ifdef _DEBUG_LOG_
		log("CClient::OnGameWebCallback()" , m_eGameState , m_eConnectedStatus );
#endif

	m_bSentWebOpen = false;
	char rgchString[256];
	sprintf_safe( rgchString, "User submitted following url: %s\n", callback->m_szURL );
	OutputDebugString( rgchString );
}


//-----------------------------------------------------------------------------
// Purpose: Handles notification that we are failed to connected to Steam
//-----------------------------------------------------------------------------
void CClient::OnSteamServerConnectFailure( SteamServerConnectFailure_t *callback )
{

#ifdef _DEBUG_LOG_
		log("CClient::OnSteamServerConnectFailure()" , m_eGameState , m_eConnectedStatus );
#endif

	char rgchString[256];
	sprintf_safe( rgchString, "SteamServerConnectFailure_t: %d\n", callback->m_eResult );

	//m_pConnectingMenu->OnConnectFailure();
}


