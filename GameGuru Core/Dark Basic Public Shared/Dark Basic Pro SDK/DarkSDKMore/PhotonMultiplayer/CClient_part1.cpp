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
			SteamAPICall_t hSteamAPICall = SteamMatchmaking()->CreateLobby( k_ELobbyTypePublic, MAX_PLAYERS_PER_SERVER );
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


//-----------------------------------------------------------------------------
// Purpose: Main frame function, updates the state of the world
//-----------------------------------------------------------------------------
void CClient::RunFrame()
{
	// Get any new data off the network to begin with
	ReceiveNetworkData();

	CheckReceipts();

	if ( m_eConnectedStatus != k_EClientNotConnected && GetCounterPassedTotal() - m_ulLastNetworkDataReceivedTime > MILLISECONDS_CONNECTION_TIMEOUT ) // dave
	{
		SetConnectionFailureText( "Game server connection failure." );
#ifdef _DEBUG_LOG_
	log("DisconnectFromServer() due to timeout" , m_eGameState );
#endif
		DisconnectFromServer(); // cleanup on our side, even though server won't get our disconnect msg
		SetGameState( k_EClientGameConnectionFailure );
	}

	// Run Steam client callbacks
	SteamAPI_RunCallbacks();

	// For now, run stats/achievements every frame
	//m_pStatsAndAchievements->RunFrame();

	// if we just transitioned state, perform on change handlers
	if ( m_bTransitionedGameState )
	{
		m_bTransitionedGameState = false;
		OnGameStateChanged( m_eGameState );
	}

	bool bInMenuNow = false;
	switch( m_eGameState )
	{
	case k_EClientGameMenu:
	case k_EClientGameQuitMenu:
		bInMenuNow = true;
		break;
	default:
		bInMenuNow = false;
		break;
	}

	// Update steam controller override mode appropriately
	if ( bInMenuNow && !m_bLastControllerStateInMenu )
	{
		m_bLastControllerStateInMenu = true;
		//SteamController()->SetOverrideMode( "menu" );
	}
	else if ( !bInMenuNow && m_bLastControllerStateInMenu )
	{
		m_bLastControllerStateInMenu = false;
		//SteamController()->SetOverrideMode( "" );
	}

	// Update state for everything
	switch ( m_eGameState )
	{
	case k_EClientConnectingToSteam:
		//m_pConnectingMenu->RunFrame();
		break;
	case k_EClientRetrySteamConnection:

		OutputDebugString( "Invalidate state k_EClientRetrySteamConnection hit on non-PS3 platform" );

		break;
	case k_EClientLinkSteamAccount:

		OutputDebugString( "Invalidate state k_EClientLinkSteamAccount hit on non-PS3 platform" );

		break;
	case k_EClientAutoCreateAccount:

		OutputDebugString( "Invalidate state k_EClientAutoCreateAccount hit on non-PS3 platform" );

		break;
	case k_EClientGameMenu:
		playerDamage = 0;
		//m_pMainMenu->RunFrame();
		break;
	case k_EClientFindInternetServers:
	case k_EClientFindLANServers:
		m_pServerBrowser->RunFrame();
		break;
	
	case k_EClientCreatingLobby:
		// draw some text about creating lobby (may take a second or two)
		break;

	case k_EClientInLobby:
		// display the lobby
		m_pLobby->RunFrame();
		
		// see if we have a game server ready to play on
		if ( m_pServer && m_pServer->IsConnectedToSteam() )
		{
			// server is up; tell everyone else to connect
			SteamMatchmaking()->SetLobbyGameServer( m_steamIDLobby, 0, 0, m_pServer->GetSteamID() );
			// start connecting ourself via localhost (this will automatically leave the lobby)
			InitiateServerConnection( m_pServer->GetSteamID() );
		}
		break;

	case k_EClientFindLobby:

		// display the list of lobbies
		m_pLobbyBrowser->RunFrame();
		break;

	case k_EClientJoiningLobby:
		
		// Check if we've waited too long and should time out the connection
		if ( GetCounterPassedTotal()- m_ulStateTransitionTime > MILLISECONDS_CONNECTION_TIMEOUT ) // dave
		{
			SetConnectionFailureText( "Timed out connecting to lobby." );
			SetGameState( k_EClientGameConnectionFailure );
		}
		break;

	case k_EClientGameConnectionFailure:
		DrawConnectionFailureText();
		break;
	case k_EClientGameConnecting:

		// Draw text telling the user a connection attempt is in progress

		m_bSentPlayerName = false;

		// Check if we've waited too long and should time out the connection
		if (  GetCounterPassedTotal()- m_ulStateTransitionTime > MILLISECONDS_CONNECTION_TIMEOUT ) // dave
		{
			if ( m_pP2PAuthedGame )
				m_pP2PAuthedGame->EndGame();
			if ( m_eConnectedStatus == k_EClientConnectedAndAuthenticated )
			{
				SteamUser()->TerminateGameConnection( m_unServerIP, m_usServerPort );
			}
			m_GameServerPing.CancelPing();
			SetConnectionFailureText( "Timed out connecting to game server" );
			SetGameState( k_EClientGameConnectionFailure );
		}

		break;
	case k_EClientGameQuitMenu:

		// Update all the entities (this is client side interpolation)...
		for( uint32 i=0; i<MAX_PLAYERS_PER_SERVER; ++i )
		{
			if ( m_rgpPlayer[i] )
			{
				//m_rgpPlayer[i]->RunFrame();
			}
		}

		// Now draw the menu
		//m_pQuitMenu->RunFrame();
		break;
	case k_EClientGameInstructions:
		//DrawInstructions();

		//if ( bEscapePressed )
		//	SetGameState( k_EClientGameMenu );
		break;
	case k_EClientWorkshop:
		//DrawWorkshopItems();

		//if (bEscapePressed)
		//	SetGameState(k_EClientGameMenu);
		break;

	case k_EClientStatsAchievements:
		//m_pStatsAndAchievements->Render();

		//if ( bEscapePressed )
			//SetGameState( k_EClientGameMenu );
		break;
	case k_EClientLeaderboards:
		m_pLeaderboards->RunFrame();		

		//if ( bEscapePressed )
			//SetGameState( k_EClientGameMenu );
		break;

	case k_EClientClanChatRoom:
		//m_pClanChatRoom->RunFrame();		

		//if ( bEscapePressed )
			//SetGameState( k_EClientGameMenu );
		break;

	case k_EClientRemoteStorage:
		//m_pRemoteStorage->Render();
		break;

	case k_EClientMinidump:
#ifdef _WIN32
		RaiseException( EXCEPTION_NONCONTINUABLE_EXCEPTION,
			EXCEPTION_NONCONTINUABLE,
			0, NULL );
#endif
		SetGameState( k_EClientGameMenu );
		break;

	case k_EClientGameStartServer:
		if ( !m_pServer )
		{
			m_pServer = new CSteamServer( );
		}

		if ( m_pServer && m_pServer->IsConnectedToSteam() )
		{
			// server is ready, connect to it
			InitiateServerConnection( m_pServer->GetSteamID() );
		}
		break;
	case k_EClientGameDraw:
	case k_EClientGameWinner:
	case k_EClientGameWaitingForPlayers:

		// Update all the entities (this is client side interpolation)...
		for( uint32 i=0; i<MAX_PLAYERS_PER_SERVER; ++i )
		{
			if ( m_rgpPlayer[i] )
			{
				//m_rgpPlayer[i]->RunFrame();
			}
		}

		//DrawHUDText();
		//DrawWinnerDrawOrWaitingText();

		if ( m_pVoiceChat )
			m_pVoiceChat->RunFrame();

		break;

	case k_EClientGameActive:

		// Update all the entities...
		for( uint32 i=0; i<MAX_PLAYERS_PER_SERVER; ++i )
		{
			if ( m_rgpPlayer[i] )
			{
				//m_rgpPlayer[i]->RunFrame();
			}
		}

		//Send name to server
		if ( (!m_bSentPlayerName && m_rgpPlayer[ m_uPlayerIndex ]) || needToSendMyName == true )
		{
			if ( ServerSaysItIsOkayToStart )
			{
				MsgClientPlayerName_t msg;
				msg.index = m_uPlayerIndex;
				strcpy ( msg.name , SteamFriends()->GetPersonaName() ); 
				strcpy ( m_rgpPlayerName[m_uPlayerIndex] , msg.name );
				if (!m_bSentPlayerName || needToSendMyName == true )
				{
					if ( SteamNetworking()->SendP2PPacket( m_steamIDGameServer, &msg, sizeof(MsgClientPlayerName_t), k_EP2PSendReliable ) )
					{
						m_bSentPlayerName = true;
						needToSendMyName = false;
					}
				}
			}
		}

		//DrawHUDText();

		//m_pStatsAndAchievements->RunFrame();

		// Voice Chat
		if ( m_pVoiceChat )
			m_pVoiceChat->RunFrame();

		break;
	case k_EClientGameExiting:
#ifdef _DEBUG_LOG_
	log("DisconnectFromServer() due to client game exiting" , m_eGameState );
#endif
		DisconnectFromServer();
		return;
	case k_EClientWebCallback:

		if ( !m_bSentWebOpen )
		{
			m_bSentWebOpen = true;
#ifndef _PS3
			char szCurDir[MAX_PATH];
			_getcwd( szCurDir, sizeof(szCurDir) );
			char szURL[MAX_PATH];
			sprintf_safe( szURL, "file:///%s/test.html", szCurDir );
			// load the test html page, it just has a steam://gamewebcallback link in it
			SteamFriends()->ActivateGameOverlayToWebPage( szURL );
			SetGameState( k_EClientGameMenu );
#endif
		}

		break;
	case k_EClientMusic:
		break;
	default:
		OutputDebugString( "Unhandled game state in CSpaceWar::RunFrame\n" );
	}


	// Send an update on our local ship to the server
	if ( m_eConnectedStatus == k_EClientConnectedAndAuthenticated &&  m_rgpPlayer[ m_uPlayerIndex ] )
	{
		MsgClientSendLocalUpdate_t msg;
		msg.SetShipPosition( m_uPlayerIndex );

		// If this fails, it probably just means its not time to send an update yet
		if ( m_rgpPlayer[ m_uPlayerIndex ]->BGetClientUpdateData( msg.AccessUpdateData() ) )
			BSendServerData( &msg, sizeof( msg ) );
	}

	if ( m_pP2PAuthedGame )
	{
		if ( m_pServer )
		{
			// Now if we are the owner of the game, lets make sure all of our players are legit.
			// if they are not, we tell the server to kick them off
			// Start at 1 to skip myself
			for ( int i = 1; i < MAX_PLAYERS_PER_SERVER; i++ )
			{
				if ( m_pP2PAuthedGame->m_rgpP2PAuthPlayer[i] && !m_pP2PAuthedGame->m_rgpP2PAuthPlayer[i]->BIsAuthOk() )
				{
					m_pServer->KickPlayerOffServer( m_pP2PAuthedGame->m_rgpP2PAuthPlayer[i]->m_steamID );
				}
			}
		}
		else
		{
			// If we are not the owner of the game, lets make sure the game owner is legit
			// if he is not, we leave the game
			if ( m_pP2PAuthedGame->m_rgpP2PAuthPlayer[0] )
			{
				if ( !m_pP2PAuthedGame->m_rgpP2PAuthPlayer[0]->BIsAuthOk() )
				{
					// leave the game
 					SetGameState( k_EClientGameMenu );
				}
			}
		}
	}

	// If we've started a local server run it
	if ( m_pServer )
	{
		m_pServer->RunFrame();
	}

	// Render everything that might have been updated by the server
	switch ( m_eGameState )
	{
	case k_EClientGameDraw:
	case k_EClientGameWinner:
	case k_EClientGameActive:
		// Now render all the objects

		for( uint32 i=0; i<MAX_PLAYERS_PER_SERVER; ++i )
		{
			if ( m_rgpPlayer[i] )
			{
			//	m_rgpPlayer[i]->Render();
			}
		}
		break;
	default:
		// Any needed drawing was already done above before server updates
		break;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Draws some HUD text indicating game status
//-----------------------------------------------------------------------------
void CClient::DrawHUDText()
{
}


//-----------------------------------------------------------------------------
// Purpose: Draws some instructions on how to play the game
//-----------------------------------------------------------------------------
void CClient::DrawInstructions()
{
}

//-----------------------------------------------------------------------------
// Purpose: Draws some text indicating a connection attempt is in progress
//-----------------------------------------------------------------------------
void CClient::DrawConnectionAttemptText()
{
}


//-----------------------------------------------------------------------------
// Purpose: Draws some text indicating a connection failure
//-----------------------------------------------------------------------------
void CClient::DrawConnectionFailureText()
{
}


//-----------------------------------------------------------------------------
// Purpose: Draws some text about who just won (or that there was a draw)
//-----------------------------------------------------------------------------
void CClient::DrawWinnerDrawOrWaitingText()
{
}


//-----------------------------------------------------------------------------
// Purpose: Did we win the last game?
//-----------------------------------------------------------------------------
bool CClient::BLocalPlayerWonLastGame()
{
	if ( m_eGameState == k_EClientGameWinner )
	{
		if ( m_uPlayerWhoWonGame >= MAX_PLAYERS_PER_SERVER )
		{
			// ur
			return false;
		}

		if ( m_rgpPlayer[m_uPlayerWhoWonGame] && m_rgpPlayer[m_uPlayerWhoWonGame]->BIsLocalPlayer() )
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		return false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Scale pixel sizes to "real" sizes
//-----------------------------------------------------------------------------
float CClient::PixelsToFeet( float flPixels )
{
	// This game is actual size! (at 72dpi) LOL
	// Those are very tiny ships, and an itty bitty neutron star

	float flReturn = ( flPixels / 72 ) / 12;

	return flReturn;
}


//-----------------------------------------------------------------------------
// Purpose: Request an encrypted app ticket
//-----------------------------------------------------------------------------
uint32 k_unSecretData = 0x5444;
void CClient::RetrieveEncryptedAppTicket()
{	
	SteamAPICall_t hSteamAPICall = SteamUser()->RequestEncryptedAppTicket( &k_unSecretData, sizeof( k_unSecretData ) );
	m_SteamCallResultEncryptedAppTicket.Set( hSteamAPICall, this, &CClient::OnRequestEncryptedAppTicket );
}


//-----------------------------------------------------------------------------
// Purpose: Called when requested app ticket asynchronously completes
//-----------------------------------------------------------------------------
void CClient::OnRequestEncryptedAppTicket( EncryptedAppTicketResponse_t *pEncryptedAppTicketResponse, bool bIOFailure )
{

#ifdef _DEBUG_LOG_
	log("CClient::OnRequestEncryptedAppTicket()" , m_eGameState , m_eConnectedStatus , bIOFailure );
#endif

	if ( bIOFailure )
		return;

	if ( pEncryptedAppTicketResponse->m_eResult == k_EResultOK )
	{

#ifdef _DEBUG_LOG_
	log("CClient::OnRequestEncryptedAppTicket() - OK" , m_eGameState , m_eConnectedStatus , pEncryptedAppTicketResponse->m_eResult );
#endif

		uint8 rgubTicket[1024];
		uint32 cubTicket;		
		SteamUser()->GetEncryptedAppTicket( rgubTicket, sizeof( rgubTicket), &cubTicket );


#ifdef _WIN32
		// normally at this point you transmit the encrypted ticket to the service that knows the decryption key
		// this code is just to demonstrate the ticket cracking library

		// included is the "secret" key. normally this is secret
		const uint8 rgubKey[k_nSteamEncryptedAppTicketSymmetricKeyLen] = { 0xed, 0x93, 0x86, 0x07, 0x36, 0x47, 0xce, 0xa5, 0x8b, 0x77, 0x21, 0x49, 0x0d, 0x59, 0xed, 0x44, 0x57, 0x23, 0xf0, 0xf6, 0x6e, 0x74, 0x14, 0xe1, 0x53, 0x3b, 0xa3, 0x3c, 0xd8, 0x03, 0xbd, 0xbd };		

		uint8 rgubDecrypted[1024];
		uint32 cubDecrypted = sizeof( rgubDecrypted );
		if ( !SteamEncryptedAppTicket_BDecryptTicket( rgubTicket, cubTicket, rgubDecrypted, &cubDecrypted, rgubKey, sizeof( rgubKey ) ) )
		{
			OutputDebugString( "Ticket failed to decrypt\n" );
			return;
		}

		if ( !SteamEncryptedAppTicket_BIsTicketForApp( rgubDecrypted, cubDecrypted, SteamUtils()->GetAppID() ) )
			OutputDebugString( "Ticket for wrong app id\n" );

		CSteamID steamIDFromTicket;
		SteamEncryptedAppTicket_GetTicketSteamID( rgubDecrypted, cubDecrypted, &steamIDFromTicket );
		if ( steamIDFromTicket != SteamUser()->GetSteamID() )
			OutputDebugString( "Ticket for wrong user\n" );

		uint32 cubData;
		uint32 *punSecretData = (uint32 *)SteamEncryptedAppTicket_GetUserVariableData( rgubDecrypted, cubDecrypted, &cubData );
		if ( cubData != sizeof( uint32 ) || *punSecretData != k_unSecretData )
			OutputDebugString( "Failed to retrieve secret data\n" );
#endif
	}
	else if ( pEncryptedAppTicketResponse->m_eResult == k_EResultLimitExceeded )
	{
		OutputDebugString( "Calling RequestEncryptedAppTicket more than once per minute returns this error\n" );
	}
	else if ( pEncryptedAppTicketResponse->m_eResult == k_EResultDuplicateRequest )
	{
		OutputDebugString( "Calling RequestEncryptedAppTicket while there is already a pending request results in this error\n" );
	}
	else if ( pEncryptedAppTicketResponse->m_eResult == k_EResultNoConnection )
	{
		OutputDebugString( "Calling RequestEncryptedAppTicket while not connected to steam results in this error\n" );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Updates what we show to friends about what we're doing and how to connect
//-----------------------------------------------------------------------------
void CClient::UpdateRichPresenceConnectionInfo()
{

#ifdef _DEBUG_LOG_
	log("CClient::UpdateRichPresenceConnectionInfo()" , m_eGameState , m_eConnectedStatus );
	if ( m_eGameState == 3 && m_eConnectedStatus == 2 )
	{
		int dave = 1;
		dave++;
	}
#endif

	// connect string that will come back to us on the command line	when a friend tries to join our game
	char rgchConnectString[128];
	rgchConnectString[0] = 0;

	if ( m_eConnectedStatus == k_EClientConnectedAndAuthenticated && m_unServerIP && m_usServerPort )
	{
		// game server connection method
		sprintf_safe( rgchConnectString, "+connect %d:%d", m_unServerIP, m_usServerPort );
	}
	else if ( m_steamIDLobby.IsValid() )
	{
		// lobby connection method
		sprintf_safe( rgchConnectString, "+connect_lobby %llu", m_steamIDLobby.ConvertToUint64() );
	}

	SteamFriends()->SetRichPresence( "connect", rgchConnectString );
}


//-----------------------------------------------------------------------------
// Purpose: applies a command-line connect
//-----------------------------------------------------------------------------
void CClient::ExecCommandLineConnect( const char *pchServerAddress, const char *pchLobbyID )
{

#ifdef _DEBUG_LOG_
	log("CClient::ExecCommandLineConnect()" , m_eGameState , m_eConnectedStatus );
#endif

	if ( pchServerAddress )
	{
		int32 octet0 = 0, octet1 = 0, octet2 = 0, octet3 = 0;
		int32 uPort = 0;
		int nConverted = sscanf( pchServerAddress, "%d.%d.%d.%d:%d", &octet0, &octet1, &octet2, &octet3, &uPort );
		if ( nConverted == 5 )
		{
			char rgchIPAddress[128];
			sprintf_safe( rgchIPAddress, "%d.%d.%d.%d", octet0, octet1, octet2, octet3 );
			uint32 unIPAddress = ( octet3 ) + ( octet2 << 8 ) + ( octet1 << 16 ) + ( octet0 << 24 );
			InitiateServerConnection( unIPAddress, uPort );
		}
	}

	// if +connect_lobby was used to specify a lobby to join, connect now
	if ( pchLobbyID )
	{
		CSteamID steamIDLobby( (uint64)atoll( pchLobbyID ) );
		if ( steamIDLobby.IsValid() )
		{
			// act just like we had selected it from the menu
			LobbyBrowserMenuItem_t menuItem = { steamIDLobby, k_EClientJoiningLobby };
			OnMenuSelection( menuItem );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: new Workshop was installed, load it instantly
//-----------------------------------------------------------------------------
void CClient::OnWorkshopItemInstalled( ItemInstalled_t *pParam )
{

#ifdef _DEBUG_LOG_
	log("CClient::OnWorkshopItemInstalled()" , m_eGameState , m_eConnectedStatus );
#endif
}

void CClient::SteamCreateLobby()
{

#ifdef _DEBUG_LOG_
	log("CClient::SteamCreateLobby()" , m_eGameState , m_eConnectedStatus );
#endif

		// start creating the lobby
		if ( !m_SteamCallResultLobbyCreated.IsActive() )
		{
			SetGameState(k_EClientCreatingLobby);
			// ask steam to create a lobby
			SteamAPICall_t hSteamAPICall = SteamMatchmaking()->CreateLobby( k_ELobbyTypePublic, MAX_PLAYERS_PER_SERVER );
			// set the function to call when this completes
			m_SteamCallResultLobbyCreated.Set( hSteamAPICall, this, &CClient::OnLobbyCreated );
		}
		SteamFriends()->SetRichPresence( "status", "Creating a lobby" );
}

int CClient::SteamIsLobbyCreated()
{
	m_gotPlayerInfoFromServer = false;
	if ( m_SteamCallResultLobbyCreated.IsActive() )
	{
		return 1;
	}
	else
		return 0;
}

void CClient::SteamGetLobbyList()
{
	m_gotPlayerInfoFromServer = false;
	m_pLobbyBrowser->Refresh();
	SteamFriends()->SetRichPresence( "status", "Game Guru: finding lobbies" );
}

int CClient::SteamIsLobbyListCreated()
{
	m_gotPlayerInfoFromServer = false;
	if ( m_pLobbyBrowser->m_bRequestingLobbies )
		return 0;
	else
		return 1;
}

int CClient::SteamGetLobbyListSize()
{
	return m_pLobbyBrowser->m_ListLobbies.size();
}

LPSTR CClient::SteamGetLobbyListName( int index )
{
	std::list<Lobby_t>::iterator iter;

  	// Return string pointer
	LPSTR pReturnString=NULL;

	int i = 0;
	for( iter = m_pLobbyBrowser->m_ListLobbies.begin(); iter != m_pLobbyBrowser->m_ListLobbies.end(); ++iter, i++ )
	{
		if ( i == index )
		{
			DWORD dwSize=strlen( (char*)iter->m_rgchName );
			g_pGlob->CreateDeleteString ( (char**)&pReturnString, dwSize+1 );
			strcpy(pReturnString, (char*)iter->m_rgchName);
			return pReturnString;
		}

	}

	return NULL;
}

void CClient::SteamJoinLobby( int index )
{

	std::list<Lobby_t>::iterator iter;

	int i = 0;
	CSteamID lobbyID;
	for( iter = m_pLobbyBrowser->m_ListLobbies.begin(); iter != m_pLobbyBrowser->m_ListLobbies.end(); ++iter, i++ )
	{
		if ( i == index )
		{
			lobbyID = iter->m_steamIDLobby;
			lobbyIAmInID = lobbyID;
		}

	}

	if ( lobbyID.IsValid() )
	{
		SteamAPICall_t hSteamAPICall = SteamMatchmaking()->JoinLobby( lobbyID );
		// set the function to call when this API completes
		m_SteamCallResultLobbyEntered.Set( hSteamAPICall, this, &CClient::OnLobbyEntered );
		SetGameState( k_EClientJoiningLobby );
	}
}

int CClient::SteamHasJoinedLobby()
{
	m_gotPlayerInfoFromServer = false;
	if ( m_eGameState == k_EClientInLobby ) return 1;
	return 0;
}

int CClient::SteamGetUsersInLobbyCount()
{
	ServerHowManyToStart = SteamMatchmaking()->GetNumLobbyMembers( m_steamIDLobby );
	return SteamMatchmaking()->GetNumLobbyMembers( m_steamIDLobby );
}

void CClient::SteamStartServer()
{
	// make sure we're not already starting a server
	if ( m_pServer )
		return;

#ifdef _DEBUG_LOG_
	log("SERVER->STARTING SERVER" , m_eGameState );
#endif

	// broadcast to everyone in the lobby that the game is starting
	SteamMatchmaking()->SetLobbyData( m_steamIDLobby, "game_starting", "1" );
		
	// start a local game server
	ServerHowManyJoined = 0;
	ServerCreationTime = (uint64)GetCounterPassedTotal();

	m_pServer = new CSteamServer();
	// we'll have to wait until the game server connects to the Steam server back-end 
	// before telling all the lobby members to join (so that the NAT traversal code has a path to contact the game server)
}

int CClient::SteamIsServerRunning()
{
	if ( !m_pServer ) return 0;
		return m_pServer->SteamIsServerRunning();
}

int CClient::SteamIsGameRunning()
{

#ifdef _DEBUG_LOG_
	log("CClient::SteamIsGameRunning() state: %i, compared to active: %i" , m_eGameState, k_EClientGameActive );
#endif

	m_gotPlayerInfoFromServer = false;
	if ( m_eGameState == k_EClientGameActive ) return 1;
	return 0;

}

void CClient::SteamSetPlayerPositionX( float _x )
{
	if ( m_rgpPlayer[m_uPlayerIndex] )
		m_rgpPlayer[m_uPlayerIndex]->x = _x;
}

void CClient::SteamSetPlayerPositionY( float _y )
{
	if ( m_rgpPlayer[m_uPlayerIndex] )
		m_rgpPlayer[m_uPlayerIndex]->y = _y;
}

void CClient::SteamSetPlayerPositionZ( float _z )
{
	if ( m_rgpPlayer[m_uPlayerIndex] )
		m_rgpPlayer[m_uPlayerIndex]->z = _z;
}

void CClient::SteamSetPlayerAngle( float _angle )
{
	if ( m_rgpPlayer[m_uPlayerIndex] )
		m_rgpPlayer[m_uPlayerIndex]->angle = _angle;
}

float CClient::SteamGetPlayerPositionX ( int index )
{
	if ( m_rgpPlayer[index] )
	{
		m_rgpPlayer[index]->x = CosineInterpolate ( m_rgpPlayer[index]->x , m_rgpPlayer[index]->newx , INTERPOLATE_SMOOTHING );
		return m_rgpPlayer[index]->x;
	}

	return 0;
}

float CClient::SteamGetPlayerPositionY ( int index )
{
	if ( m_rgpPlayer[index] )
	{
		m_rgpPlayer[index]->y = CosineInterpolate ( m_rgpPlayer[index]->y , m_rgpPlayer[index]->newy , INTERPOLATE_SMOOTHING );
		return m_rgpPlayer[index]->y;
	}

	return 0;
}

float CClient::SteamGetPlayerPositionZ ( int index )
{
	if ( m_rgpPlayer[index] )
	{
		m_rgpPlayer[index]->z = CosineInterpolate ( m_rgpPlayer[index]->z , m_rgpPlayer[index]->newz , INTERPOLATE_SMOOTHING );
		return m_rgpPlayer[index]->z;
	}

	return 0;
}

float CClient::SteamGetPlayerAngle ( int index )
{
	if ( m_rgpPlayer[index] )
	{
		//m_rgpPlayer[index]->angle = m_rgpPlayer[index]->newangle;
		m_rgpPlayer[index]->angle = CosineInterpolateAngle( m_rgpPlayer[index]->angle , m_rgpPlayer[index]->newangle , INTERPOLATE_SMOOTHING_TURN );
		return m_rgpPlayer[index]->angle;
	}

	return 0;
}

void CClient::SteamSetPlayerScore ( int index, int score )
{
	if ( score == -1 )
		scores[index] = 0;
	else
		scores[index] += score;
	

	MsgClientPlayerScore_t msg;
	msg.index = index;
	msg.score = scores[index];
	SteamNetworking()->SendP2PPacket( m_steamIDGameServer, &msg, sizeof(MsgClientPlayerScore_t), k_EP2PSendReliable );
}

int CClient::SteamGetPlayerScore ( int index )
{
	return scores[index];
}

void CClient::SteamSetBullet ( int index , float x , float y , float z, float anglex, float angley, float anglez, int type, int on )
{
	if  ( bullets[index].on == 0 && on == 1 )
	{
		bullets[index].x = x;
		bullets[index].y = y;
		bullets[index].z = z;
	}

	bullets[index].newx = x;
	bullets[index].newy = y;
	bullets[index].newz = z;
	bullets[index].anglex = anglex;
	bullets[index].angley = angley;
	bullets[index].anglez = anglez;
	bullets[index].type = type;
	bullets[index].on = on;

	MsgClientPlayerBullet_t msg;
	msg.index = index;
	msg.x = x;
	msg.y = y;
	msg.z = z;
	msg.anglex = anglex;
	msg.angley = angley;
	msg.anglez = anglez;
	msg.type = type;
	msg.on = on;

	SteamNetworking()->SendP2PPacket( m_steamIDGameServer, &msg, sizeof(MsgClientPlayerBullet_t), k_EP2PSendUnreliable );
	if ( on == 0) SteamNetworking()->SendP2PPacket( m_steamIDGameServer, &msg, sizeof(MsgClientPlayerBullet_t), k_EP2PSendReliable );

}

int CClient::SteamGetBulletOn ( int index )
{
	int result = bullets[index].on;
	if ( result == 1 )
	{
		if ( GetCounterPassedTotal() - bullets[index].onTime > 10000 )
		{
			bullets[index].on = 0;
			result = 0;
		}
	}
	//bullets[index].on = 0;
	return result;
}

int CClient::SteamGetBulletType ( int index )
{
	return bullets[index].type;
}

float CClient::SteamGetBulletX ( int index )
{
	float dx =  bullets[index].x - bullets[index].newx;
	float dy =  bullets[index].y - bullets[index].newy;
	float dz =  bullets[index].z - bullets[index].newz;
	float d = sqrt(dx*dx + dy*dy + dz*dz);

	if ( d < 100.0f )
		bullets[index].x = CosineInterpolate ( bullets[index].x , bullets[index].newx , INTERPOLATE_SMOOTHING_MIN );
	else
		bullets[index].x = CosineInterpolate ( bullets[index].x , bullets[index].newx , INTERPOLATE_SMOOTHING );

	return bullets[index].x;

}

float CClient::SteamGetBulletY ( int index )
{
	float dx =  bullets[index].x - bullets[index].newx;
	float dy =  bullets[index].y - bullets[index].newy;
	float dz =  bullets[index].z - bullets[index].newz;
	float d = sqrt(dx*dx + dy*dy + dz*dz);

	if ( d < 100.0f )
		bullets[index].y = CosineInterpolate ( bullets[index].y , bullets[index].newy , INTERPOLATE_SMOOTHING_MIN );
	else
		bullets[index].y = CosineInterpolate ( bullets[index].y , bullets[index].newy , INTERPOLATE_SMOOTHING );

	return bullets[index].y;

}

float CClient::SteamGetBulletZ ( int index )
{
	float dx =  bullets[index].x - bullets[index].newx;
	float dy =  bullets[index].y - bullets[index].newy;
	float dz =  bullets[index].z - bullets[index].newz;
	float d = sqrt(dx*dx + dy*dy + dz*dz);

	if ( d < 100.0f )
		bullets[index].z = CosineInterpolate ( bullets[index].z , bullets[index].newz , INTERPOLATE_SMOOTHING_MIN );
	else
		bullets[index].z = CosineInterpolate ( bullets[index].z , bullets[index].newz , INTERPOLATE_SMOOTHING );

	return bullets[index].z;

}

float CClient::SteamGetBulletAngleX ( int index )
{
	return bullets[index].anglex;
}

float CClient::SteamGetBulletAngleY ( int index )
{
	return bullets[index].angley;
}

float CClient::SteamGetBulletAngleZ ( int index )
{
	return bullets[index].anglez;
}

void CClient::SteamSetKeyState ( int key , int state )
{
	MsgClientPlayerKeyState_t msg;
	msg.index = m_uPlayerIndex;
	msg.key = key;
	msg.state = state;
	SteamNetworking()->SendP2PPacket( m_steamIDGameServer, &msg, sizeof(MsgClientPlayerKeyState_t), k_EP2PSendUnreliable );
}

void CClient::SteamApplyPlayerDamage ( int index, int damage, int x, int y, int z, int force, int limb )
{

	MsgClientPlayerApplyDamage_t* pmsg;
	pmsg = new MsgClientPlayerApplyDamage_t();
	pmsg->index = index;
	pmsg->source = m_uPlayerIndex;
	pmsg->amount = damage;
	pmsg->x = x;
	pmsg->y = y;
	pmsg->z = z;
	pmsg->force = force;
	pmsg->limb = limb;
	pmsg->logID = packetSendLogClientID;

	packetSendLogClient_t log;
	log.LogID = packetSendLogClientID++;
	log.packetType = k_EMsgClientPlayerApplyDamage;
	log.pPacket = pmsg;
	log.timeStamp = GetCounterPassedTotal();

	PacketSend_Log_Client.push_back(log);

	SteamNetworking()->SendP2PPacket( m_steamIDGameServer, pmsg, sizeof(MsgClientPlayerApplyDamage_t), k_EP2PSendUnreliable );
}

int CClient::SteamGetClientServerConnectionStatus()
{
	if ( ServerIsShuttingDown == 1 )
	{
		return 2;
	}

	if (m_eConnectedStatus == k_EClientNotConnected )
	{
		return 0;
	}

	return 1;
}

void CClient::SteamSetPlayerAlive ( int state )
{
	alive[m_uPlayerIndex] = state;
	MsgClientPlayerSetPlayerAlive_t msg;
	msg.index = m_uPlayerIndex;
	msg.state = state;
	SteamNetworking()->SendP2PPacket( m_steamIDGameServer, &msg, sizeof(MsgClientPlayerSetPlayerAlive_t), k_EP2PSendUnreliable );
}

int CClient::IsServer()
{
	if ( m_pServer ) return 1;
	return 0;
}

void CClient::CheckIfIShouldTakeOverServer()
{
	// if we are the next player in the list (1, as the server is player 0) then we will attempt to carry on the game
	if ( m_uPlayerIndex == 1 )
	{
		for( uint32 i=0; i < MAX_PLAYERS_PER_SERVER; ++i )
		{
			// Update steamid array with data from server
			AccountID_t act = playerSteamIDs[i].GetAccountID();
			int b;
			b = 1;
			//playerSteamIDs[i].SetFromUint64( pUpdateData->GetPlayerSteamID( i ) );
		}
	}
}

void CClient::SteamSpawnObject ( int obj, int source, float x, float y, float z )
{
	MsgClientSpawnObject_t msg;
	msg.index = m_uPlayerIndex;
	msg.object = obj;
	msg.source = source;
	msg.x = x;
	msg.y = y;
	msg.z = z;
	SteamNetworking()->SendP2PPacket( m_steamIDGameServer, &msg, sizeof(MsgClientSpawnObject_t), k_EP2PSendReliable );
}

void CClient::SteamSendLua ( int code, int e, int v )
{
	if ( code == 17 )
	{
#ifdef _DEBUG_LOG_
		if ( DEBUG_FLAG_ON == 0 )
		{
			log("################################################### " );
			log("=================== END PLAY ====================== " );
			log("=================== END PLAY ====================== " );
			log("=================== END PLAY ====================== " );
			log("=================== END PLAY ====================== " );
			log("################################################### " );
		}
		DEBUG_FLAG_ON = 1;
#endif
	}

	if ( m_steamIDGameServer.IsValid() )
	{
		if ( code < 5 )
		{
			MsgClientLua_t msg;
			msg.index = m_uPlayerIndex;
			msg.code = code;
			msg.e = e;
			msg.v = v;

			SteamNetworking()->SendP2PPacket( m_steamIDGameServer, &msg, sizeof(MsgClientLua_t), k_EP2PSendReliable );
		}
		else
		{
			// no ticket if goto position
			if ( code != 18 && code != 19 )
			{
				MsgClientLua_t* pmsg;
				pmsg = new MsgClientLua_t();
				pmsg->index = m_uPlayerIndex;
				pmsg->code = code;
				pmsg->e = e;
				pmsg->v = v;
				pmsg->logID = packetSendLogClientID;

				packetSendLogClient_t log;
				log.LogID = packetSendLogClientID++;
				log.packetType = k_EMsgClientLua;
				log.pPacket = pmsg;
				log.timeStamp = GetCounterPassedTotal();

				PacketSend_Log_Client.push_back(log);

				SteamNetworking()->SendP2PPacket( m_steamIDGameServer, pmsg, sizeof(MsgClientLua_t), k_EP2PSendUnreliable );
			}
			else
			{
				MsgClientLua_t msg;
				msg.index = m_uPlayerIndex;
				msg.code = code;
				msg.e = e;
				msg.v = v;

				SteamNetworking()->SendP2PPacket( m_steamIDGameServer, &msg, sizeof(MsgClientLua_t), k_EP2PSendUnreliable );
			}
		}
	}
}

void CClient::SteamSendLuaString ( int code, int e, LPSTR s )
{ 
	if ( m_steamIDGameServer.IsValid() )
	{
		MsgClientLuaString_t* pmsg;
		pmsg = new MsgClientLuaString_t();
		pmsg->index = m_uPlayerIndex;
		pmsg->code = code;
		pmsg->e = e;
		strcpy ( pmsg->s , s );
		pmsg->logID = packetSendLogClientID;

		packetSendLogClient_t log;
		log.LogID = packetSendLogClientID++;
		log.packetType = k_EMsgClientLuaString;
		log.pPacket = pmsg;
		log.timeStamp = GetCounterPassedTotal();

		PacketSend_Log_Client.push_back(log);

		SteamNetworking()->SendP2PPacket( m_steamIDGameServer, pmsg, sizeof(MsgClientLuaString_t), k_EP2PSendUnreliable );

	}
}

void CClient::SteamDeleteObject ( int obj )
{
	if ( m_steamIDGameServer.IsValid() )
	{
		MsgClientDeleteObject_t msg;
		msg.index = m_uPlayerIndex;
		msg.object = obj;
		SteamNetworking()->SendP2PPacket( m_steamIDGameServer, &msg, sizeof(MsgClientDeleteObject_t), k_EP2PSendReliable );
	}
}

void CClient::SteamDestroyObject ( int obj )
{
	if ( m_steamIDGameServer.IsValid() )
	{
		MsgClientDestroyObject_t msg;
		msg.index = m_uPlayerIndex;
		msg.object = obj;
		SteamNetworking()->SendP2PPacket( m_steamIDGameServer, &msg, sizeof(MsgClientDestroyObject_t), k_EP2PSendReliable );
	}
}

void CClient::SteamKilledBy ( int killedBy, int x, int y, int z, int force, int limb )
{
	MsgClientKilledBy_t* pmsg;
	pmsg = new MsgClientKilledBy_t();
	pmsg->index = m_uPlayerIndex;
	pmsg->logID = packetSendLogClientID;
	pmsg->killedBy = killedBy;
	pmsg->x = x;
	pmsg->y = y;
	pmsg->z = z;
	pmsg->force = force;
	pmsg->limb = limb;

	packetSendLogClient_t log;
	log.LogID = packetSendLogClientID++;
	log.packetType = k_EMsgClientKilledBy;
	log.pPacket = pmsg;
	log.timeStamp = GetCounterPassedTotal();

	PacketSend_Log_Client.push_back(log);

	SteamNetworking()->SendP2PPacket( m_steamIDGameServer, pmsg, sizeof(MsgClientKilledBy_t), k_EP2PSendUnreliable );

}

void CClient::SteamKilledSelf()
{
	MsgClientKilledSelf_t msg;
	msg.index = m_uPlayerIndex;
	SteamNetworking()->SendP2PPacket( m_steamIDGameServer, &msg, sizeof(MsgClientKilledSelf_t), k_EP2PSendUnreliable );
}

void CClient::SteamSetSendFileCount ( int count )
{
	if ( IsServer() )
	{

		MsgClientSetSendFileCount_t msg;
		msg.count = count;
		SteamNetworking()->SendP2PPacket( m_steamIDGameServer, &msg, sizeof(MsgClientSetSendFileCount_t), k_EP2PSendReliable );

		ServerFilesToReceive = count;
		ServerFilesReceived = count;
		IamSyncedWithServerFiles = 1;
		IamLoadedAndReady = 0;
		isEveryoneLoadedAndReady = 0;
		IamReadyToPlay = 0;
		isEveryoneReadyToPlay = 0;

		for( uint32 i = 0; i < MAX_PLAYERS_PER_SERVER; ++i )
		{
			serverClientsFileSynced[i] = 0;
			serverClientsLoadedAndReady[i] = 0;
			serverClientsReadyToPlay[i] = 0;
		}
	}
}

void CClient::SteamSendFileBegin ( int index , LPSTR pString )
{
	if ( IsServer() )
	{
		HowManyPlayersDoWeHave = 0;

		MsgClientSendFileBegin_t msg;
		msg.index = index;
		strcpy ( msg.fileName , pString );

		serverFile = GG_fopen ( pString, "rb" );
		if ( serverFile )
		{
			fseek ( serverFile, 0, SEEK_END );
			msg.fileSize = ftell ( serverFile );
			rewind ( serverFile );

			serverHowManyFileChunks = (int)ceil( (float)msg.fileSize / float(FILE_CHUNK_SIZE) );
			serverChunkToSendCount = 1;

			SteamNetworking()->SendP2PPacket( m_steamIDGameServer, &msg, sizeof(MsgClientSendFileBegin_t), k_EP2PSendReliable );
		}
	}
}

// send the next chunk or mark as done
int CClient::SteamSendFileDone()
{
	if ( IsServer() )
	{
		if ( serverFile )
		{
			MsgClientSendChunk_t msg;
			msg.index = serverChunkToSendCount;
			fread ( &msg.chunk, 1 , FILE_CHUNK_SIZE , serverFile );

			SteamNetworking()->SendP2PPacket( m_steamIDGameServer, &msg, sizeof(MsgClientSendChunk_t), k_EP2PSendReliable );

			serverChunkToSendCount++;
			
			if ( serverChunkToSendCount > serverHowManyFileChunks )
			{

				IamLoadedAndReady = 0;
				isEveryoneLoadedAndReady = 0;

				for( uint32 i = 1; i < MAX_PLAYERS_PER_SERVER; i++ )
				{
					serverClientsLoadedAndReady[i] = 0;
				}

				fclose ( serverFile );
				serverFile = NULL;
				return 1;
			}
			return 0;
		}
	}

	return 0;
}

void CClient::SteamSendAvatarFileClient ( int index , LPSTR pString )
{

	HowManyPlayersDoWeHave = 0;

	MsgClientSendAvatarFileBeginClient_t msg;
	msg.index = index;
	char dest[MAX_PATH];
	sprintf ( dest, "customAvatar_%i_cc.dds" ,  SteamGetMyPlayerIndex() );
	strcpy ( msg.fileName , dest );

	FILE* file = GG_fopen ( pString, "rb" );
	if ( file )
	{
		fseek ( file, 0, SEEK_END );
		msg.fileSize = ftell ( file );
		rewind ( file );

		serverHowManyFileChunks = (int)ceil( (float)msg.fileSize / float(FILE_CHUNK_SIZE) );
		serverChunkToSendCount = 1;

		SteamNetworking()->SendP2PPacket( m_steamIDGameServer, &msg, sizeof(MsgClientSendAvatarFileBeginClient_t), k_EP2PSendReliable );

		// send all the chunks
		bool done = false;
		while ( done == false )
		{
			MsgClientSendAvatarChunkClient_t msg;
			msg.index = index;
			msg.count = serverChunkToSendCount;
			fread ( &msg.chunk, 1 , FILE_CHUNK_SIZE , file );

			SteamNetworking()->SendP2PPacket( m_steamIDGameServer, &msg, sizeof(MsgClientSendAvatarChunkClient_t), k_EP2PSendReliable );

			serverChunkToSendCount++;
			
			if ( serverChunkToSendCount > serverHowManyFileChunks )
			{
				serverChunkToSendCount = 0;
				serverHowManyFileChunks = 0;
				fclose ( file );
				file = NULL;
				done = true;
			}

		}

		// let the server know we are done
		MsgClientSendAvatarDone_t msg;
		msg.index = index;
		SteamNetworking()->SendP2PPacket( m_steamIDGameServer, &msg, sizeof(MsgClientSendAvatarDone_t), k_EP2PSendReliable );		
	}
}

int CClient::SteamIsEveryoneFileSynced()
{
	if ( IsServer() )
	{

		IamLoadedAndReady = 0;
		isEveryoneLoadedAndReady = 0;

		for( uint32 i = 1; i < MAX_PLAYERS_PER_SERVER; i++ )
		{
			serverClientsLoadedAndReady[i] = 0;
		}

		// check from 1 since the client hosting the server is player 0 and always synced
		for( uint32 i = 1; i < MAX_PLAYERS_PER_SERVER; ++i )
		{
			if ( m_rgpPlayer[i] && serverClientsFileSynced[i] == 0 )
				return 0;
		}

		return 1;
	}

	return 0;
}

void CClient::ServerCheckEveryoneIsLoadedAndReady()
{
	if ( IsServer() )
	{
		m_pServer->ServerCheckEveryoneIsLoadedAndReady();
	}
}

void CClient::ServerCheckEveryoneIsReadyToPlay()
{
	if ( IsServer() )
	{
		m_pServer->ServerCheckEveryoneIsReadyToPlay();
	}
}

void CClient::SteamEndGame()
{
	if ( IsServer() )
	{
		m_pServer->ServerEndGame( m_uPlayerIndex ); // don't need to send it to ourselves since we are fully aware the server is shutting down
	}
}

void CClient::SteamSendIAmLoadedAndReady()
{

			isEveryoneLoadedAndReady = 0;

			MsgClientSendIAmLoadedAndReady_t msg;
			msg.index = m_uPlayerIndex;

			SteamNetworking()->SendP2PPacket( m_steamIDGameServer, &msg, sizeof(MsgClientSendIAmLoadedAndReady_t), k_EP2PSendReliable );
}

void CClient::SteamSendIAmReadyToPlay()
{
			MsgClientSendIAmReadyToPlay_t* pmsg;
			pmsg = new MsgClientSendIAmReadyToPlay_t();
			pmsg->index = m_uPlayerIndex;
			pmsg->logID = packetSendLogClientID;

			packetSendLogClient_t log;
			log.LogID = packetSendLogClientID++;
			log.packetType = k_EMsgClientPlayerSendIAmReadyToPlay;
			log.pPacket = pmsg;
			log.timeStamp = GetCounterPassedTotal();

			PacketSend_Log_Client.push_back(log);

			SteamNetworking()->SendP2PPacket( m_steamIDGameServer, pmsg, sizeof(MsgClientSendIAmReadyToPlay_t), k_EP2PSendUnreliable );
}

int CClient::SteamGetFileProgress()
{
	return fileProgress;
}

void CClient::SteamSetPlayerAppearance( int a)
{
	MsgClientPlayerAppearance_t msg;
	msg.index = m_uPlayerIndex;
	msg.appearance = a;
	SteamNetworking()->SendP2PPacket( m_steamIDGameServer, &msg, sizeof(MsgClientPlayerAppearance_t), k_EP2PSendUnreliable );
}

void CClient::SteamSetCollision ( int index, int state )
{
	MsgClientSetCollision_t msg;
	msg.playerIndex = m_uPlayerIndex;
	msg.index = index;
	msg.state = state;
	SteamNetworking()->SendP2PPacket( m_steamIDGameServer, &msg, sizeof(MsgClientSetCollision_t), k_EP2PSendReliable );
}

void CClient::SteamPlayAnimation ( int index, int start, int end, int speed )
{
	MsgClientPlayAnimation_t msg;
	msg.playerIndex = m_uPlayerIndex;
	msg.index = index;
	msg.start = start;
	msg.end = end;
	msg.speed = speed;
	SteamNetworking()->SendP2PPacket( m_steamIDGameServer, &msg, sizeof(MsgClientPlayAnimation_t), k_EP2PSendReliable );
}

void CClient::SteamShoot ()
{
	MsgClientShoot_t msg;
	msg.index = m_uPlayerIndex;
	SteamNetworking()->SendP2PPacket( m_steamIDGameServer, &msg, sizeof(MsgClientShoot_t), k_EP2PSendUnreliable );
}

void CClient::SteamSendChat(LPSTR chat )
{
	MsgChat_t msg;
	msg.index = m_uPlayerIndex;
	strcpy ( msg.msg , chat );
	SteamNetworking()->SendP2PPacket( m_steamIDGameServer, &msg, sizeof(MsgChat_t), k_EP2PSendReliable );
}

void CClient::SteamLeaveLobby()
{
	// leave the lobby
	SteamMatchmaking()->LeaveLobby( m_steamIDLobby );
	m_steamIDLobby = CSteamID();
	lobbyIAmInID = CSteamID();

	// return to main menu
	SetGameState( k_EClientGameMenu );
}

// send if we have an avatar texture to the server
void CClient::AvatarSendWeHaveHeadTextureToServer(int flag)
{
	MsgClientAvatarDoWeHaveHeadTex_t* pmsg;
	pmsg = new MsgClientAvatarDoWeHaveHeadTex_t();
	pmsg->index = m_uPlayerIndex;
	pmsg->flag = flag;
	pmsg->logID = packetSendLogClientID;

	packetSendLogClient_t log;
	log.LogID = packetSendLogClientID++;
	log.packetType = k_EMsgClientAvatarDoWeHaveHeadTex;
	log.pPacket = pmsg;
	log.timeStamp = GetCounterPassedTotal();

	PacketSend_Log_Client.push_back(log);

	SteamNetworking()->SendP2PPacket( m_steamIDGameServer, pmsg, sizeof(MsgClientAvatarDoWeHaveHeadTex_t), k_EP2PSendUnreliable );
}

void CClient::CheckReceipts()
{
	double timeNow = GetCounterPassedTotal();

	for ( unsigned int c = 0; c < PacketSend_Log_Client.size() ; c++ )
	{
		if ( timeNow - PacketSend_Log_Client[c].timeStamp > 1000 )
		{
			PacketSend_Log_Client[c].timeStamp = timeNow;
			// send again
			switch ( PacketSend_Log_Client[c].packetType )
			{
			case k_EMsgClientPlayerSendIAmReadyToPlay:
				SteamNetworking()->SendP2PPacket( m_steamIDGameServer, PacketSend_Log_Client[c].pPacket , sizeof(MsgClientSendIAmReadyToPlay_t), k_EP2PSendUnreliable ); break;
			case k_EMsgClientKilledBy:
				SteamNetworking()->SendP2PPacket( m_steamIDGameServer, PacketSend_Log_Client[c].pPacket , sizeof(MsgClientKilledBy_t), k_EP2PSendUnreliable ); break;
			case k_EMsgClientPlayerApplyDamage:
				SteamNetworking()->SendP2PPacket( m_steamIDGameServer, PacketSend_Log_Client[c].pPacket , sizeof(MsgClientPlayerApplyDamage_t), k_EP2PSendUnreliable ); break;
			case k_EMsgClientLua:
				SteamNetworking()->SendP2PPacket( m_steamIDGameServer, PacketSend_Log_Client[c].pPacket , sizeof(MsgClientLua_t), k_EP2PSendUnreliable ); break;
			case k_EMsgClientLuaString:
				SteamNetworking()->SendP2PPacket( m_steamIDGameServer, PacketSend_Log_Client[c].pPacket , sizeof(MsgClientLuaString_t), k_EP2PSendUnreliable ); break;
			case k_EMsgClientAvatarDoWeHaveHeadTex:
				SteamNetworking()->SendP2PPacket( m_steamIDGameServer, PacketSend_Log_Client[c].pPacket , sizeof(MsgClientAvatarDoWeHaveHeadTex_t), k_EP2PSendUnreliable ); break;
			}

		}
	}
}

void CClient::GotReceipt( int c )
{
	int found = -1;
	for ( unsigned int i = 0; i < PacketSend_Log_Client.size() ; i++ )
	{
		if ( PacketSend_Log_Client[i].LogID == c ) 
		{
			found = i;
			break;
		}
	}

	if ( found > -1 )
	{
		switch ( PacketSend_Log_Client[found].packetType )
		{
		case k_EMsgClientPlayerSendIAmReadyToPlay:
			delete (MsgClientSendIAmReadyToPlay_t*)PacketSend_Log_Client[found].pPacket; break;
		case k_EMsgClientKilledBy:
			delete (MsgClientKilledBy_t*)PacketSend_Log_Client[found].pPacket; break;
		case k_EMsgClientPlayerApplyDamage:
			delete (MsgClientPlayerApplyDamage_t*)PacketSend_Log_Client[found].pPacket; break;
		case k_EMsgClientLua:
			delete (MsgClientLua_t*)PacketSend_Log_Client[found].pPacket; break;
		case k_EMsgClientLuaString:
			delete (MsgClientLuaString_t*)PacketSend_Log_Client[found].pPacket; break;
		case k_EMsgClientAvatarDoWeHaveHeadTex:
			delete (MsgClientAvatarDoWeHaveHeadTex_t*)PacketSend_Log_Client[found].pPacket; break;
		}

		PacketSend_Log_Client.erase(PacketSend_Log_Client.begin()+found);
	}
}
*/
