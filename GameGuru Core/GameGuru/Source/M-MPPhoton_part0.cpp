//----------------------------------------------------
//--- GAMEGURU - M-Multiplayer
//----------------------------------------------------

// Includes 
#include "stdafx.h"
#include "gameguru.h"
#include <wininet.h>
#include "Common-Keys.h"

// Defines
#define ENABLEMPAVATAR

// flag to switch workshop handling from workshop to game managed, by default set to false, set to true for multiplayer mode

// Prototypes
void lua_promptlocalcore ( int iTrueLocalOrForVR , int addtime = 1000);

//  Startup Steam
void mp_init ( void )
{
	timestampactivity(0,"_mp_init:");
	 t.mp_build = 2001;
	 g.mp.isRunning = 1; // PhotonInit() done later when actually need Photon
	g.mp.mode = MP_MODE_NONE;
	g.mp.dontDrawTitles = 0;
	g.mp.message = "";
	g.mp.messageTime = 0;

	// If a custom character head is used but the image no longer exists, we need to get rid of the avatar file
	// later!
}

void mp_fullinit ( void )
{
	// first get any avatar the player is identified as
	if ( FileOpen(1) == 1 ) CloseFile ( 1 );
	if ( FileExist( cstr(g.fpscrootdir_s + "\\multiplayeravatar.dat").Get() ) == 1 ) 
	{
		OpenToRead ( 1, cstr (g.fpscrootdir_s + "\\multiplayeravatar.dat").Get() );
		g.mp.myAvatar_s = ReadString ( 1 );
		g.mp.myAvatarHeadTexture_s = ReadString ( 1 );
		g.mp.myAvatarName_s = g.mp.myAvatarHeadTexture_s;
		CloseFile ( 1 );

		// and delete any old thumb of it
		if (ImageExist(g.charactercreatorEditorImageoffset) == 1) DeleteImage ( g.charactercreatorEditorImageoffset );
	}

	// second, get obfuscated site name from sitekey
	char pSiteName[1024];
	strcpy ( pSiteName, "site name" );
	cstr SiteName_s;
	char pSitename[1024];
	strcpy ( pSitename, "12345-12345-12345-12345" );
	if ( FileExist( cstr(g.fpscrootdir_s + "\\vrqcontrolmode.ini").Get() ) == 1 ) 
	{
		OpenToRead ( 1, cstr (g.fpscrootdir_s + "\\vrqcontrolmode.ini").Get() );
		SiteName_s = ReadString ( 1 );
		strcpy ( pSitename, SiteName_s.Get() );
		for ( int n = 0; n < strlen(pSitename); n++ )
		{
			if ( pSitename[n] == '-' ) 
				pSitename[n] = 'Z';
			else
				pSitename[n] = pSitename[n] + 1;
		}
		CloseFile ( 1 );
	}	

	// third, check if teacher view all mode enabled
	bool bViewAllMode = false;
	if ( FileExist( cstr(g.fpscrootdir_s + "\\teacherviewallmode.dat").Get() ) == 1 ) 
	{
		bViewAllMode = true;
		CloseFile ( 1 );
	}

	// Initialise multiplayer system
		cstr optionalPhotonAppID_s = "";
		if ( FileExist( cstr(g.fpscrootdir_s + "\\photonappid.ini").Get() ) == 1 ) 
		{
			OpenToRead ( 1, cstr (g.fpscrootdir_s + "\\photonappid.ini").Get() );
			cstr optionalPhotonAppID_s = ReadString ( 1 );
			CloseFile ( 1 );
		}
		LPSTR pUseAppID = NULL;
		if ( optionalPhotonAppID_s.Len() > 0 ) pUseAppID = optionalPhotonAppID_s.Get();
		PhotonInit(g.fpscrootdir_s.Get(),pSitename,g.mp.myAvatarName_s.Get(),bViewAllMode,pUseAppID);

	// check fpm master list at start (for good server cleanup while leaving files on server while MP screens in use)
	mp_checkToCleanUpMasterHostList();
}

void mp_fullclose ( void )
{
	// this is called when leaving multiplayer screen and back to IDE
	mp_checkToCleanUpMasterHostList();
}

bool OccluderCheckingForMultiplayer ( void )
{
	if ( t.game.runasmultiplayer == 0 ) return false;

	return true;
}

void mp_loop ( void )
{
	 PhotonLoop();

	// OnlineMultiplayerModeForSharingFiles handling
	 // find out later if this needed for Photon

	 // General handling
	if ( g.mp.mode != MP_IN_GAME_CLIENT && g.mp.mode != MP_IN_GAME_SERVER ) 
	{
		// reset flag
		g.mp.finishedLoadingMap = 0;

		// 200315 - 021 - flashlight of when starting a game
		mp_flashLightOff ( );
		g.mp.originalEntitycount = 0;
		if ( SpriteExist(g.steamchatpanelsprite)  )  DeleteSprite (  g.steamchatpanelsprite );
		g.mp.dontDrawTitles = 0;

		// If not connected to steam, retry
		 g.mp.backtoeditorforyou = 0;

		// Debug Info
		t.steamDoDropShadow = 1;
		 if ( PhotonGetSiteName() )
		 {
			 if ( PhotonGetViewAllMode() == 1 )
				 t.ttstring_s = cstr("Multiplayer Build ") + Str(t.mp_build) + "  (" + PhotonGetSiteName() + " in All Sites Mode)";
			 else
				 t.ttstring_s = cstr("Multiplayer Build ") + Str(t.mp_build) + "  (" + PhotonGetSiteName() + " in Site Only Mode)";
		 }
		 else
			 t.ttstring_s = cstr("Multiplayer Build ") + Str(t.mp_build) + "  (Site Unknown)";
		mp_text(-1,98,2,t.ttstring_s.Get());
	}
	else
	{
		// 030315 - 013 - Lobby chat
	}

	// Handle main menu
	if ( g.mp.mode == MP_MODE_MAIN_MENU ) 
	{
		// show avatar name if there is one
		#ifdef ENABLEMPAVATAR
		if (g.mp.myAvatarName_s != "")
		{
			if (ImageExist(g.charactercreatorEditorImageoffset) == 0)
			{
				t.tShowAvatarSprite = 1;
				characterkitplus_loadMyAvatarInfo();
			}
			t.tYPos_f = 95;
			if (GetDisplayHeight() > 900) t.tYPos_f = 92;
			mp_text(-1, t.tYPos_f, 2, g.mp.myAvatarName_s.Get());
			if (g.charactercreatorEditorImageoffset > 0)
			{
				if (ImageExist(g.charactercreatorEditorImageoffset) == 1)
				{
					t.tYPos_f = GetChildWindowHeight();
					t.tYPos_f = t.tYPos_f * 0.85;
					PasteImage(g.charactercreatorEditorImageoffset, (GetChildWindowWidth() / 2) - 32, t.tYPos_f, g.charactercreatorEditorImageoffset);
				}
			}
		}
		#endif
		g.mp.dontDrawTitles = 0;
		if ( g.mp.originalpath == "" ) 
		{
			g.mp.originalpath = GetDir();
			 PhotonSetRoot(cstr(g.fpscrootdir_s+"\\Files\\").Get());
		}
		// 110315 - 019 - remove fadeoutsprite if it exists
		if ( t.tspritetouse > 0 ) 
		{
			if ( SpriteExist(t.tspritetouse) == 1 )  DeleteSprite (  t.tspritetouse );
			t.tspritetouse = 0;
		}
		g.mp.lobbyscrollbarOn = 0;
		g.mp.selectedLobby = 0;
		t.tjoinedLobby = 0;
		g.mp.lobbyoffset = 0;
		g.mp.lobbycount = 0;
		mp_resetGameStats ( );
		t.game.jumplevel_s="__multiplayerlevel__";
	}
	else
	{
		if ( g.charactercreatorEditorImageoffset > 0 ) 
		{
			if ( ImageExist(g.charactercreatorEditorImageoffset)  )  DeleteImage (  g.charactercreatorEditorImageoffset );
		}
	}

	// Handle lobby creation
	if ( g.mp.mode == MP_WAITING_FOR_LOBBY_CREATION ) 
	{
		 g.mp.isLobbyCreated = 1;
		 PhotonGetLobbyList();
		 g.mp.mode = MP_MODE_LOBBY;
	}

	// Workshop related states
	 // No workshop in Photon

	if ( g.mp.mode == MP_SERVER_CHOOSING_FPM_TO_USE ) 
	{
		mp_text(-1,5,3,"LIST OF LEVELS");
		mp_lobbyListBox ( );
	}

	// Handle lobby page
	if ( g.mp.mode == MP_MODE_LOBBY ) 
	{
		if ( g.mp.isGameHost == 0 ) 
		{
			// if lose connection
			if ( g.mp.isRunning == 0 ) 
			{
				t.tsteamlostconnectioncustommessage_s = "Lost Connection";
				g.mp.backtoeditorforyou = 2;
				mp_lostConnection ( );
				return;
			}

			// if lose lobby list
			 mp_text(-1,5,3,"LIST OF LEVELS");
			 if ( MAXTimer() - g.mp.oldtime > 3000 ) 
			 {
				PhotonGetLobbyList();
				g.mp.oldtime = MAXTimer();
			 }
			mp_lobbyListBox ( );
		}
		else
		{
			// Chat handling
			 // No chat in Photon Lobby(game room)

			 // Determine number of players in lobby/room
			 t.tUserCount = PhotonGetLobbyUserCount();
			if ( t.tUserCount == 1 ) 
			{
				t.tstring_s = "There is 1 user (you!) here";
				g.mp.usersInServersLobbyAtServerCreation = 1;
			}
			else
			{
				t.tstring_s = cstr("There are ") + Str(t.tUserCount) + " users here";
			}
			if ( t.tUserCount != g.mp.usersInServersLobbyAtServerCreation ) 
			{
				g.mp.haveSentMyAvatar = 0;
			}
			if ( t.tUserCount > g.mp.usersInServersLobbyAtServerCreation ) 
			{
				g.mp.usersInServersLobbyAtServerCreation = t.tUserCount;
			}
			mp_text(-1,15,1,t.tstring_s.Get());
			t.tsteamy_f = 50.0 - (t.tUserCount * 2.5);
			t.tsteamy = t.tsteamy_f;
			for ( t.tn = 1 ; t.tn <= t.tUserCount; t.tn++ )
			{
				 LPSTR pDisplayName = PhotonGetLobbyUserDisplayName(t.tn-1);
				 t.tstring_s = cstr("Player ") + Str(t.tn) + ": " + pDisplayName;
				 if ( PhotonGetPlayerName() != PhotonGetLobbyUserName(t.tn-1) ) t.mp_joined[t.tn-1] = PhotonGetLobbyUserName(t.tn-1);
				mp_text(-1,t.tsteamy,1,t.tstring_s.Get());
				t.tsteamy += 5;
			}
			for ( t.tn = t.tUserCount ; t.tn<=  MP_MAX_NUMBER_OF_PLAYERS-1; t.tn++ )
			{
				t.mp_joined[t.tn] = "";
			}
			if ( g.mp.haveToldAboutSolo == 1 && t.tUserCount  <=  1 ) 
			{
				mp_textColor(-1,70,1,"No-one has joined yet. If you start now you will be playing alone.",255,100,100);
				mp_textColor(-1,75,1,"Press Start again to start anyway.",255,100,100);
			}

			// Handle server launch (start MP game)
			if ( g.mp.launchServer == 1 && t.tUserCount > 0 ) 
			{
				if ( g.mp.haveToldAboutSolo == 0 && t.tUserCount == 1 ) 
				{
					g.mp.haveToldAboutSolo = 1;
					g.mp.launchServer = 0;
					return;
				}
				 PhotonStartServer ( );
				 // at moment of starting server, register all present players so they dont get re-added when game starts
				 PhotonRegisterEveryonePresentAsHere();
				g.mp.mode = MP_WAITING_FOR_SERVER_CREATION;
				g.mp.oldtime = MAXTimer();
			}
		}
	}

	// Handle joining the lobby/room
	if ( g.mp.mode == MP_JOINING_LOBBY ) 
	{
		if ( MAXTimer() - g.mp.oldtime > 1000 && PhotonGetLobbyUserCount() > 1 ) //iIsGameRunning  == 1 ) just go direct to getting file and starting
		{
			g.mp.mode = MP_IN_GAME_CLIENT;
			g.mp.needToResetOnStartup = 1;
			t.toldsteamfolder_s=GetDir();
			t.tsteamtimeoutongamerunning = MAXTimer();

			// Reset player var
			 int tPlayerIndex = PhotonGetMyPlayerIndex();
			if ( tPlayerIndex >= 0 && tPlayerIndex < MP_MAX_NUMBER_OF_PLAYERS ) 
			{
				t.mp_health[tPlayerIndex] = 0;
				t.ta = MouseMoveX() + MouseMoveY();
			}
		}
		 // reduced all code below to a simple display of users in this game room (Photon can migrate host so not important if hosts leaves)
		 int iHasJoinedLobby = PhotonHasJoinedLobby();
		 if ( iHasJoinedLobby == 1 )
		 {
			t.tjoinedLobby = 1;
			int iLobbyUserCount = PhotonGetLobbyUserCount();
			if ( t.tUserCount != iLobbyUserCount ) 
			{
				g.mp.haveSentMyAvatar = 0;
			}
			t.tUserCount = iLobbyUserCount;
			t.tsteamy_f = 50.0 - (t.tUserCount * 2.5);
			t.tsteamy = t.tsteamy_f;
			for ( t.tn = 1 ; t.tn <= t.tUserCount; t.tn++ )
			{
				LPSTR pDisplayName = PhotonGetLobbyUserDisplayName(t.tn-1);
				LPSTR pLobbyUserName = pDisplayName;
				t.tstring_s = cstr("Player ") + Str(t.tn) + ": " + pLobbyUserName;
				mp_text(-1,t.tsteamy,1,t.tstring_s.Get());
				t.tsteamy += 5;
			}
		 }
		 else
		 {
			mp_textDots(-1,20,3,"Connecting to level...");
			int iClientServerConnectionStatus = PhotonGetClientServerConnectionStatus();
			if ( iClientServerConnectionStatus == 0 ) 
			{
				t.tsteamlostconnectioncustommessage_s = "Lost connection";
				g.mp.mode = MP_MODE_MAIN_MENU;
				mp_lostConnection ( );
				return;
			}
		 }
	}

	// LEE NOTE: This is the next stage, starting the host and joined games and exchanging in-game data

	// Server creation handling
	if ( g.mp.mode == MP_WAITING_FOR_SERVER_CREATION ) 
	{
		g.mp.dontDrawTitles = 1;
		 int iIsServerRunning = PhotonIsServerRunning();
		if ( iIsServerRunning == 1 ) 
		{
			mp_textDots(-1,10,3,"Server Started");
			 int iIsGameRunning = PhotonIsGameRunning();
			if ( iIsGameRunning == 1 ) 
			{
				if ( MAXTimer() - g.mp.oldtime > 150 ) 
				{
					g.mp.mode = MP_IN_GAME_SERVER;
					g.mp.needToResetOnStartup = 1;
				}
			}
			else
			{
				if ( MAXTimer() - g.mp.oldtime > 150 ) 
				{
					g.mp.oldtime = MAXTimer();
					t.tStartingServerCount_s = t.tStartingServerCount_s + ".";
					if ( Len(t.tStartingServerCount_s.Get()) > 5 )  t.tStartingServerCount_s = ".";
				}
				t.tstring_s = t.tStartingServerCount_s + "Waiting for level to start" + t.tStartingServerCount_s;
				mp_text(-1,25,3,t.tstring_s.Get());
				t.tstring_s = "";
			}
		}
		else
		{
			if ( MAXTimer() - g.mp.oldtime > 150 ) 
			{
				g.mp.oldtime = MAXTimer();
				t.tStartingServerCount_s = t.tStartingServerCount_s + ".";
				if ( Len(t.tStartingServerCount_s.Get()) > 5 ) t.tStartingServerCount_s = ".";
			}
			t.tstring_s = t.tStartingServerCount_s + "Starting server" + t.tStartingServerCount_s;
			mp_text(-1,15,3,t.tstring_s.Get());
			t.tstring_s = "";
		}
	}

	// In Game Server handling
	if ( g.mp.mode == MP_IN_GAME_SERVER ) 
	{
		g.mp.dontDrawTitles = 1;
		if ( g.mp.iHaveSaidIAmAlmostReady == 0 ) 
		{
			 PhotonSetThisPlayerAsCurrentServer ( );
			 PhotonSendIAmLoadedAndReady ( );
			g.mp.iHaveSaidIAmAlmostReady = 1;
			t.tempsteamingameinitialwaitingdelay = MAXTimer();
			while ( MAXTimer() - t.tempsteamingameinitialwaitingdelay < 2000 ) // not needed any more, server can serve up clients any time now.. was 20000 ) 
			{
				g.mp.syncedWithServerMode = 0;
				g.mp.onlySendMapToSpecificPlayer = -1;
				g.mp.okayToLoadLevel = 0;
				t.fLastProgress = 0;
				 PhotonLoop(); // dangerous - risk of recursion!
				mp_textDots(-1,20,3,"Waiting for other players");
				if ( MAXTimer() - t.tsteamiseveryoneloadedandreadytime > 1000 ) 
				{
					t.tsteamiseveryoneloadedandreadytime = MAXTimer();
					if ( PhotonIsEveryoneLoadedAndReady() == 1 )
					{
						t.tempsteamingameinitialwaitingdelay = -30000;
					}
				}
			}
			t.tskipLevelSync = MAXTimer();
		}

		// wait for everyone before starting to load, at this GetPoint (  they have all the files they need, they just have not loaded them )
		if ( g.mp.okayToLoadLevel == 0 && g.mp.syncedWithServerMode == 99 ) 
		{
			t.game.titleloop=0;
			t.game.levelloop=1;
			t.game.runasmultiplayer=1;
			t.game.levelloadprogress=0;
			t.game.cancelmultiplayer=0;
			t.game.quitflag=0;
			t.tescapepress=0 ; t.ttitlesbuttonhighlight=0;
			g.mp.playGame = 1;
			g.mp.okayToLoadLevel = 1;
			PhotonResetFile ( );
			t.tskipLevelSync = MAXTimer();
		}
		else
		{
			if ( g.mp.playGame == 1 ) 
			{
				if ( t.game.titleloop == 1 ) 
				{
					t.game.titleloop=0;
					t.game.levelloop=1;
					t.game.runasmultiplayer=1;
					t.game.levelloadprogress=0;
					t.game.cancelmultiplayer=0;
					t.game.quitflag=0;
					t.tescapepress=0 ; t.ttitlesbuttonhighlight=0;
				}
			}
			if ( g.mp.okayToLoadLevel == 0 ) 
			{
				mp_pre_game_file_sync_server ( -1 );
			}
		}
	}

	// In Game Client Handling
	if ( g.mp.mode == MP_IN_GAME_CLIENT ) 
	{
		if ( t.titlespage == 11 ) 
		{
			g.mp.dontDrawTitles = 0;
		}
		else
		{
			g.mp.dontDrawTitles = 1;
		}
		g.mp.dontDrawTitles = 1;
		if ( g.mp.iHaveSaidIAmAlmostReady == 0 ) 
		{
			 //this is wrong, it is sending the loaded and ready flag even before the file was received! (moved later in sequence)
			 //PhotonSendIAmLoadedAndReady (  );
			t.tskipLevelSync = MAXTimer();
			t.tempsteamingameinitialwaitingdelay = MAXTimer();
			g.mp.iKeepCheckingForGameRunning = MAXTimer();
			g.mp.iHaveSaidIAmAlmostReady = 1;
		}
		if ( g.mp.iHaveSaidIAmAlmostReady == 1 ) 
		{
			DWORD dwReasonableTimeOutIfWaitingForGameToStart = 3 * 60 * 1000; // 3 minutes (could simply be waiting for more players, not real time out here)
			if ( MAXTimer() - t.tempsteamingameinitialwaitingdelay < dwReasonableTimeOutIfWaitingForGameToStart ) 
			{
				g.mp.syncedWithServerMode = 0;
				g.mp.onlySendMapToSpecificPlayer = -1;
				g.mp.okayToLoadLevel = 0;
				g.mp.oldtime = MAXTimer();
				t.fLastProgress = 0;
				mp_textDots(-1,50,3,"Waiting for other players");
				 PhotonLoop(); // dangerous - risk of recursion!
				// real time-out if no connection after 16 seconds of coming in here
				if ( MAXTimer() - t.tsteamtimeoutongamerunning > 16000 ) 
				{
					if ( PhotonGetClientServerConnectionStatus() == 0 ) 
					{
						t.tsteamlostconnectioncustommessage_s = "Lost connection to host (Error MP009)";
						mp_lostConnection ( );
						return;
					}
				}
				 int iIsEveryoneLoadedAndReady = PhotonIsEveryoneLoadedAndReady();
				if ( iIsEveryoneLoadedAndReady == 1 ) t.tempsteamingameinitialwaitingdelay = -3000000; // was just = not ==

				// can also skip this wait if game is already running (or was started after joining)
				if ( MAXTimer() - g.mp.iKeepCheckingForGameRunning > 1000 ) 
				{
					g.mp.iKeepCheckingForGameRunning = MAXTimer();
					PhotonCheckIfGameRunning();
				}
				int iGameRunning = PhotonIsGameRunning();
				if ( iGameRunning == 1 ) 
				{
					t.tempsteamingameinitialwaitingdelay = -3000000;
				}

				// if user presses SPACE, force a disconnect and leave
				mp_text(-1,95,3,"(press SPACE KEY to return to main menu)");
				bool bEscapeEarly = false;
				if ( ScanCode() == 57 ) bEscapeEarly = true;
				if ( bEscapeEarly == true )
				{
					// forcing a quit
					t.tsteamconnectionlostmessage_s = "User terminated transfer and returning to main menu";
					g.mp.mode = MP_MODE_MAIN_MENU;
					mp_lostConnection ( );
					g.mp.iHaveSaidIAmAlmostReady = 0;
				}
			}   
			else
			{
				// okay, the game has been started and we can move on
				g.mp.iHaveSaidIAmAlmostReady = 2;
			}
		}
		if ( g.mp.iHaveSaidIAmAlmostReady == 2 ) 
		{
			// wait for everyone before starting to load, at this point
			// they have all the files they need, they just have not loaded them
			if ( g.mp.okayToLoadLevel == 0 && g.mp.syncedWithServerMode == 99 ) 
			{
				t.game.titleloop=0;
				t.game.levelloop=1;
				t.game.runasmultiplayer=1;
				t.game.levelloadprogress=0;
				t.game.cancelmultiplayer=0;
				t.game.quitflag=0;
				t.tescapepress=0 ; t.ttitlesbuttonhighlight=0;
				g.mp.playGame = 1;
				g.mp.okayToLoadLevel = 1;
				PhotonResetFile ( );
				t.tskipLevelSync = MAXTimer();
			}
			else
			{
				if ( g.mp.playGame == 1 ) 
				{
					if ( t.game.titleloop == 1 ) 
					{
						t.game.titleloop=0;
						t.game.levelloop=1;
						t.game.runasmultiplayer=1;
						t.game.levelloadprogress=0;
						t.game.cancelmultiplayer=0;
						t.game.quitflag=0;
						t.tescapepress=0 ; t.ttitlesbuttonhighlight=0;
					}
				}
				if ( g.mp.okayToLoadLevel == 0 ) 
				{
					mp_pre_game_file_sync_client ( );
				}
			}
		}
	}

	mp_message ( );
	mp_messageDots ( );
}

void mp_free ( void )
{
	 PhotonFree();
}

void mp_checkVoiceChat ( void )
{
}

void mp_spawn_objects ( void )
{
	//  Grab the list of spawned objects from the server
	//  TO DO - find out how entities are spawned in FPSC and call those routines
	//  LEE - AGREED, no need to repeat code but we can do that during clean-up ;)
	while (  SteamGetSpawnList() ) 
	{
		t.obj = SteamGetSpawnObjectNumber();
		t.sourceobj = SteamGetSpawnObjectSource();
		t.x_f = SteamGetSpawnX();
		t.y_f = SteamGetSpawnY();
		t.z_f = SteamGetSpawnZ();
		InstanceObject (  t.obj, t.sourceobj );
		//  restore any radius settings the original object might have had
		SetSphereRadius (  t.obj,-1 );
		ShowObject (  t.obj );
		PositionObject (  t.obj, t.x_f, t.y_f, t.z_f );
		SteamGetNextSpawn (  );
	}
}

void mp_lua ( void )
{
	while ( PhotonGetLuaList() ) 
	{
		t.steamLuaCode = PhotonGetLuaCommand();
		t.e = PhotonGetLuaE();
		t.v = PhotonGetLuaV();	
		t.tLuaDontSendLua = 1;
	
		switch ( t.steamLuaCode ) 
		{
			case MP_LUA_SetActivated:
				if ( mp_check_if_lua_entity_exists(t.e) == 1 ) 
					entity_lua_setactivated();
			break;
			case MP_LUA_ActivateIfUsed:
				if ( mp_check_if_lua_entity_exists(t.e) == 1 ) 
					entity_lua_activateifused();
			break;
			case MP_LUA_SendAvatar:
			{
				int iSlotIndex = t.e;
				t.tsteams_s = PhotonGetLuaS();
				t.mp_playerAvatars_s[iSlotIndex] = t.tsteams_s;
				t.mp_playerAvatarLoaded[iSlotIndex] = false;
				t.bTriggerAvatarRescanAndLoad = true;
			}
			break;
			case MP_LUA_SendAvatarName:
			{
				int iSlotIndex = t.e;
				t.tsteams_s = PhotonGetLuaS();
				t.mp_playerAvatarOwners_s[iSlotIndex] = t.tsteams_s;
			}
			break;
		}
	
		t.tLuaDontSendLua = 0;	
		PhotonGetNextLua ( );
	}
}

void mp_delete_entities ( void )
{

	g.mp.ignoreDamageToEntity = 1;
	while (  SteamGetDeleteList() ) 
	{
		t.ttte = SteamGetDeleteObjectNumber();
		if (  t.ttte  <=  g.entityelementlist ) 
		{
			t.tobj = t.entityelement[t.ttte].obj;
			if (  t.tobj > 0 ) 
			{
				if (  ObjectExist(t.tobj)  ==  1 ) 
				{
					t.tdamage = t.entityelement[t.ttte].health;
					t.tdamageforce = 0;
					t.tdamagesource = 0;
					t.brayx1_f = ObjectPositionX(t.tobj);
					t.brayy1_f = ObjectPositionY(t.tobj);
					t.brayz1_f = ObjectPositionZ(t.tobj);
					t.brayx2_f = ObjectPositionX(t.tobj);
					t.brayy2_f = ObjectPositionY(t.tobj);
					t.brayz2_f = ObjectPositionZ(t.tobj);
	
					t.entityelement[t.ttte].mp_networkkill = 1;
					t.entityelement[t.ttte].mp_killedby = SteamGetDeleteSource();
					t.entityelement[t.ttte].health = 0;
	
					entity_applydamage ( );
				}
			}
		}
	
		SteamGetNextDelete (  );
	}
	g.mp.ignoreDamageToEntity = 0;

	while (  SteamGetDestroyList() ) 
	{
		t.ttte = SteamGetDestroyObjectNumber();
		if (  t.ttte  <=  g.entityelementlist ) 
		{
			t.tobj = t.entityelement[t.ttte].obj;
			if (  t.tobj > 0 ) 
			{
				if (  ObjectExist(t.tobj)  ==  1 ) 
				{
					t.entityelement[t.ttte].destroyme=1;
				}
			}
		}
		SteamGetNextDestroy (  );
	}
}

// TEMPORARY LEVEL CLOUD SERVER (V2+V3) - These FPM files are uploaded long enough for SocialVR to find, download and play them

void mp_writeNewFPMMasterList ( std::vector<LPSTR> pLines )
{
	char pFPMMasterList[2048];
	strcpy ( pFPMMasterList, g.fpscrootdir_s.Get() );
	strcat ( pFPMMasterList, "\\FPMMasterList.dat" );
	if ( FileExist ( pFPMMasterList ) ) DeleteFileA ( pFPMMasterList );
	OpenToWrite ( 5, pFPMMasterList );
	for ( int iFileIndex = 0; iFileIndex < pLines.size(); iFileIndex++ )
	{
		WriteString ( 5, pLines[iFileIndex] );
	}
	CloseFile ( 5 );
}

void mp_addHostFPMFIleToMasterHostList ( LPSTR pFilenameToAdd )
{
	// clear the list
	std::vector<LPSTR> pLines;
	pLines.clear();

	// adds filename to list just successfully added to server
	char pFPMMasterList[2048];
	strcpy ( pFPMMasterList, g.fpscrootdir_s.Get() );
	strcat ( pFPMMasterList, "\\FPMMasterList.dat" );
	if ( FileExist ( pFPMMasterList ) )
	{
		OpenToRead ( 5, pFPMMasterList );
		while ( FileEnd ( 5 ) == 0 )
		{
			LPSTR pLineRef = ReadString ( 5 );
			LPSTR pNewLine = new char[strlen(pLineRef)+1];
			strcpy ( pNewLine, pLineRef );
			pLines.push_back ( pNewLine );
		}
		CloseFile ( 5 );
	}

	// add new file to list
	LPSTR pNewLine = new char[strlen(pFilenameToAdd)+1];
	strcpy ( pNewLine, pFilenameToAdd );
	pLines.push_back ( pNewLine );

	// and write the list out
	mp_writeNewFPMMasterList ( pLines );

	// and free resources
	for ( int iFileIndex = 0; iFileIndex < pLines.size(); iFileIndex++ )
		delete pLines[iFileIndex];
}

void mp_encode (LPSTR pURLEncoded)
{
	const std::string& value = pURLEncoded;
	static auto hex_digt = "0123456789ABCDEF";
	std::string result;
	result.reserve(value.size() << 1);
	for (auto ch : value)
	{
		if ((ch >= '0' && ch <= '9')
			|| (ch >= 'A' && ch <= 'Z')
			|| (ch >= 'a' && ch <= 'z')
			|| ch == '-' || ch == '_' || ch == '!'
			|| ch == '\'' || ch == '(' || ch == ')'
			|| ch == '*' || ch == '~' || ch == '.')  // !'()*-._~
		{
			result.push_back(ch);
		}
		else
		{
			result += std::string("%") +
				hex_digt[static_cast<unsigned char>(ch) >> 4]
				+ hex_digt[static_cast<unsigned char>(ch) & 15];
		}
	}
	// copy result back to calling code
	strcpy (pURLEncoded, result.c_str());
}

bool mp_deleteFPMFileFromServer ( LPSTR pFilenameToDelete )
{
	char pDataReturned[10340];
	strcpy ( pDataReturned, "" );
	char urlWhere[2048];
	strcpy ( urlWhere, "/api/gameguru/multiplayer/storage/delete?" );
	strcat ( urlWhere, FPMHOSTUPLOADKEY );
	strcat ( urlWhere, "&file=" );
	char pURLEncoded[MAX_PATH];
	strcpy(pURLEncoded, pFilenameToDelete);
	mp_encode(pURLEncoded);
	strcat ( urlWhere, pURLEncoded);
	UINT iError = 0;
	unsigned int dwDataLength = 0;
	HINTERNET m_hInet = InternetOpenA( "InternetConnection", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0 );
	if ( m_hInet == NULL )
	{
		iError = GetLastError( );
	}
	else
	{
		unsigned short wHTTPType = INTERNET_DEFAULT_HTTPS_PORT;
		HINTERNET m_hInetConnect = InternetConnectA( m_hInet, "www.thegamecreators.com", wHTTPType, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0 );
		if ( m_hInetConnect == NULL )
		{
			iError = GetLastError( );
		}
		else
		{
			int m_iTimeout = 2000;
			InternetSetOption( m_hInetConnect, INTERNET_OPTION_CONNECT_TIMEOUT, (void*)&m_iTimeout, sizeof(m_iTimeout) );  
			HINTERNET hHttpRequest = HttpOpenRequestA( m_hInetConnect, "GET", urlWhere, "HTTP/1.1", NULL, NULL, INTERNET_FLAG_IGNORE_CERT_CN_INVALID | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_SECURE, 0 );
			if ( hHttpRequest == NULL )
			{
				iError = GetLastError( );
			}
			else
			{
				HttpAddRequestHeadersA( hHttpRequest, "Content-Type: application/x-www-form-urlencoded", -1, HTTP_ADDREQ_FLAG_ADD | HTTP_ADDREQ_FLAG_REPLACE );
				int bSendResult = 0;
				bSendResult = HttpSendRequest( hHttpRequest, NULL, -1, NULL, 0 );
				if ( bSendResult == 0 )
				{
					iError = GetLastError( );
				}
				else
				{
					int m_iStatusCode = 0;
					char m_szContentType[150];
					unsigned int dwBufferSize = sizeof(int);
					unsigned int dwHeaderIndex = 0;
					HttpQueryInfo( hHttpRequest, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, (void*)&m_iStatusCode, (LPDWORD)&dwBufferSize, (LPDWORD)&dwHeaderIndex );
					dwHeaderIndex = 0;
					unsigned int dwContentLength = 0;
					HttpQueryInfo( hHttpRequest, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, (void*)&dwContentLength, (LPDWORD)&dwBufferSize, (LPDWORD)&dwHeaderIndex );
					dwHeaderIndex = 0;
					unsigned int ContentTypeLength = 150;
					HttpQueryInfo( hHttpRequest, HTTP_QUERY_CONTENT_TYPE, (void*)m_szContentType, (LPDWORD)&ContentTypeLength, (LPDWORD)&dwHeaderIndex );
					char pBuffer[ 20000 ];
					for(;;)
					{
						unsigned int written = 0;
						if( !InternetReadFile( hHttpRequest, (void*) pBuffer, 2000, (LPDWORD)&written ) )
						{
							// error
						}
						if ( written == 0 ) break;
						if ( dwDataLength + written > 10240 ) written = 10240 - dwDataLength;
						memcpy( pDataReturned + dwDataLength, pBuffer, written );
						dwDataLength = dwDataLength + written;
						if ( dwDataLength >= 10240 ) break;
					}
					InternetCloseHandle( hHttpRequest );
				}
			}
			InternetCloseHandle( m_hInetConnect );
		}
		InternetCloseHandle( m_hInet );
	}
	if ( iError > 0 )
	{
		char *szError = 0;
		if ( iError > 12000 && iError < 12174 ) 
			FormatMessageA( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE, GetModuleHandleA("wininet.dll"), iError, 0, (char*)&szError, 0, 0 );
		else 
			FormatMessageA( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, iError, 0, (char*)&szError, 0, 0 );
		if ( szError )
		{
			LocalFree( szError );
		}
	}

	// check response from GET call
	if ( pDataReturned && strchr(pDataReturned, '{') != 0 && dwDataLength < 10240 )
	{
		// break up response string
		// {"success": true,"message": "deleted"} 
		char pfilenameText[10240];
		strcpy ( pfilenameText, "" );
		char pWorkStr[10240];
		strcpy ( pWorkStr, pDataReturned );
		if ( pWorkStr[0]=='{' ) strcpy ( pWorkStr, pWorkStr+1 );
		int n = 10200;
		for (; n>0; n-- ) if ( pWorkStr[n] == '}' ) { pWorkStr[n] = 0; break; }
		char* pChop = strstr ( pWorkStr, "," );
		char pStatusStr[10240];
		strcpy ( pStatusStr, pWorkStr );
		if ( pChop ) pStatusStr[pChop-pWorkStr] = 0;
		if ( pChop[0]==',' ) pChop += 1;
		if ( strstr ( pStatusStr, "success" ) != NULL )
		{
			// success
			if ( strstr ( pStatusStr, "true" ) != NULL )
			{
				// delete was successful
				return true;
			}
		}
	}

	// failed to delete for some reason
	return false;
}

void mp_checkToCleanUpMasterHostList ( void )
{
	// final list after cleanup
	std::vector<LPSTR> pLines;
	pLines.clear();

	// go through master list, instruct all in list to be deleted from server
	char pFPMMasterList[2048];
	strcpy ( pFPMMasterList, g.fpscrootdir_s.Get() );
	strcat ( pFPMMasterList, "\\FPMMasterList.dat" );
	if ( FileExist ( pFPMMasterList ) )
	{
		OpenToRead ( 5, pFPMMasterList );
		while ( FileEnd ( 5 ) == 0 )
		{
			LPSTR pFilenameToDelete = ReadString ( 5 );
			if ( strlen ( pFilenameToDelete ) > 0 )
			{
				// attempt to delete the file from the server
				if ( mp_deleteFPMFileFromServer ( pFilenameToDelete ) == false )
				{
					// if failed to delete file from server, keep it in list
					LPSTR pNewLine = new char[strlen(pFilenameToDelete)+1];
					strcpy ( pNewLine, pFilenameToDelete );
					pLines.push_back ( pNewLine );
				}
			}
		}
		CloseFile ( 5 );

		// write out new potentially empty or shorter list
		mp_writeNewFPMMasterList ( pLines );
	}

	// and free resources
	for ( int iFileIndex = 0; iFileIndex < pLines.size(); iFileIndex++ )
		delete pLines[iFileIndex];
}

// PERMANENT LEVEL CLOUD SERVER - These FPM game files remain on the list (so Oculus Quest Player can find, download and play them)

char g_gamecloud_error[2048];
int g_gamecloud_activity = 0;
int g_gamecloud_mode = 0;
std::vector<cstr> g_gamecloud_gamelist;
int SendFileInternal(LPSTR szServerFile, bool bSaveToFile, LPSTR szLocalFile, LPSTR szUploadFile, LPSTR szPostData);
bool SendFileInternalAsync(LPSTR* ppDataReturned, DWORD* pdwDataReturnedSize);
float SendFileInternalGetProgress(void);

void mp_gamecloud_getlist(void)
{
	if (g_gamecloud_activity == 0)
	{
		// clar level cloud list
		g_gamecloud_gamelist.clear();

		// connect to server
		LPSTR pServerHost = "www.thegamecreators.com";
		HTTPConnect(pServerHost);

		// access server
		char szGetData[1024];
		strcpy(szGetData, "/api/gameguru/multiplayer/storage/v2/listing?");
		strcat(szGetData, FPMHOSTUPLOADKEY);
		LPSTR pVerb = "GET";
		LPSTR pDataReturned = HTTPRequestData(pVerb, szGetData, NULL);

		// disconnect from server
		HTTPDisconnect();

		// if no internet connection
		if (strstr(pDataReturned, "Send Request failed") == NULL )
		{
			// ensure its a successful transmission
			if (strstr(pDataReturned, "success") != NULL)
			{
				// break up response string
				char* pFilenamePtr = strstr(pDataReturned, "files");
				if (pFilenamePtr)
				{
					pFilenamePtr += strlen("files") + 4;
					char pFPMFilename[MAX_PATH];
					char pEndOfChunk[2];
					pEndOfChunk[0] = '"';
					pEndOfChunk[1] = 0;
					char* pEndofFilenamePtr = strstr(pFilenamePtr,pEndOfChunk);
					while (pEndofFilenamePtr)
					{
						// put this filename in list
						int iSizeOfFilename = pEndofFilenamePtr - pFilenamePtr;
						memcpy(pFPMFilename, pFilenamePtr, iSizeOfFilename);
						pFPMFilename[iSizeOfFilename] = 0;
						g_gamecloud_gamelist.push_back(cstr(pFPMFilename));

						// find next filename (level.fpm","nextone.fpm","third.fpm")
						pFilenamePtr += iSizeOfFilename + 1; // ,"next
						pEndofFilenamePtr = strstr(pFilenamePtr, pEndOfChunk); // "next
						if (pEndofFilenamePtr)
						{
							pFilenamePtr = pEndofFilenamePtr + 1; // nextone.fpm","third
							pEndofFilenamePtr = strstr(pFilenamePtr, pEndOfChunk); // ","third
						}
					}
				}
			}
			else
			{
				// error prompt when server check fails
				strcpy(g_gamecloud_error, pDataReturned);
			}
		}
		else
		{
			// no internet connection available
			strcpy(g_gamecloud_error, "Error getting level cloud list");
		}
	}
	else
	{
		strcpy(g_gamecloud_error, "Level Cloud Server access busy");
	}
}

int mp_gamecloud_delete(LPSTR pFilenameToDelete)
{
	if (g_gamecloud_activity == 0)
	{
		char pDataReturned[10340];
		strcpy(pDataReturned, "");
		char urlWhere[2048];
		strcpy(urlWhere, "/api/gameguru/multiplayer/storage/v2/delete?");
		strcat(urlWhere, FPMHOSTUPLOADKEY);
		strcat(urlWhere, "&file=");
		char pURLEncoded[MAX_PATH];
		strcpy(pURLEncoded, pFilenameToDelete);
		mp_encode(pURLEncoded);
		strcat (urlWhere, pURLEncoded);
		UINT iError = 0;
		unsigned int dwDataLength = 0;
		HINTERNET m_hInet = InternetOpenA("InternetConnection", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
		if (m_hInet == NULL)
		{
			iError = GetLastError();
		}
		else
		{
			unsigned short wHTTPType = INTERNET_DEFAULT_HTTPS_PORT;
			HINTERNET m_hInetConnect = InternetConnectA(m_hInet, "www.thegamecreators.com", wHTTPType, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
			if (m_hInetConnect == NULL)
			{
				iError = GetLastError();
			}
			else
			{
				int m_iTimeout = 2000;
				InternetSetOption(m_hInetConnect, INTERNET_OPTION_CONNECT_TIMEOUT, (void*)&m_iTimeout, sizeof(m_iTimeout));
				HINTERNET hHttpRequest = HttpOpenRequestA(m_hInetConnect, "GET", urlWhere, "HTTP/1.1", NULL, NULL, INTERNET_FLAG_IGNORE_CERT_CN_INVALID | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_SECURE, 0);
				if (hHttpRequest == NULL)
				{
					iError = GetLastError();
				}
				else
				{
					HttpAddRequestHeadersA(hHttpRequest, "Content-Type: application/x-www-form-urlencoded", -1, HTTP_ADDREQ_FLAG_ADD | HTTP_ADDREQ_FLAG_REPLACE);
					int bSendResult = 0;
					bSendResult = HttpSendRequest(hHttpRequest, NULL, -1, NULL, 0);
					if (bSendResult == 0)
					{
						iError = GetLastError();
					}
					else
					{
						int m_iStatusCode = 0;
						char m_szContentType[150];
						unsigned int dwBufferSize = sizeof(int);
						unsigned int dwHeaderIndex = 0;
						HttpQueryInfo(hHttpRequest, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, (void*)&m_iStatusCode, (LPDWORD)&dwBufferSize, (LPDWORD)&dwHeaderIndex);
						dwHeaderIndex = 0;
						unsigned int dwContentLength = 0;
						HttpQueryInfo(hHttpRequest, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, (void*)&dwContentLength, (LPDWORD)&dwBufferSize, (LPDWORD)&dwHeaderIndex);
						dwHeaderIndex = 0;
						unsigned int ContentTypeLength = 150;
						HttpQueryInfo(hHttpRequest, HTTP_QUERY_CONTENT_TYPE, (void*)m_szContentType, (LPDWORD)&ContentTypeLength, (LPDWORD)&dwHeaderIndex);
						char pBuffer[20000];
						for (;;)
						{
							unsigned int written = 0;
							if (!InternetReadFile(hHttpRequest, (void*)pBuffer, 2000, (LPDWORD)&written))
							{
								// error
							}
							if (written == 0) break;
							if (dwDataLength + written > 10240) written = 10240 - dwDataLength;
							memcpy(pDataReturned + dwDataLength, pBuffer, written);
							dwDataLength = dwDataLength + written;
							if (dwDataLength >= 10240) break;
						}
						InternetCloseHandle(hHttpRequest);
					}
				}
				InternetCloseHandle(m_hInetConnect);
			}
			InternetCloseHandle(m_hInet);
		}
		if (iError > 0)
		{
			char *szError = 0;
			if (iError > 12000 && iError < 12174)
				FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE, GetModuleHandleA("wininet.dll"), iError, 0, (char*)&szError, 0, 0);
			else
				FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, iError, 0, (char*)&szError, 0, 0);
			if (szError)
			{
				LocalFree(szError);
			}
		}

		// check response from GET call
		if (pDataReturned && strchr(pDataReturned, '{') != 0 && dwDataLength < 10240)
		{
			// break up response string
			// {"success": true,"message": "deleted"} 
			char pfilenameText[10240];
			strcpy(pfilenameText, "");
			char pWorkStr[10240];
			strcpy(pWorkStr, pDataReturned);
			if (pWorkStr[0] == '{') strcpy(pWorkStr, pWorkStr + 1);
			int n = 10200;
			for (; n > 0; n--) if (pWorkStr[n] == '}') { pWorkStr[n] = 0; break; }
			char* pChop = strstr(pWorkStr, ",");
			char pStatusStr[10240];
			strcpy(pStatusStr, pWorkStr);
			if (pChop) pStatusStr[pChop - pWorkStr] = 0;
			if (pChop[0] == ',') pChop += 1;
			if (strstr(pStatusStr, "success") != NULL)
			{
				// success
				if (strstr(pStatusStr, "true") != NULL)
				{
					// delete was successful
					return 0;
				}
			}
		}
		else
		{
			// failed to delete for some reason
			strcpy(g_gamecloud_error, "Error deleting level on level cloud");
		}
	}
	else
	{
		strcpy(g_gamecloud_error, "Level Cloud Server access busy");
	}
	return -1;
}

int mp_gamecloud_overwriteexisting(LPSTR pFileOnly)
{
	// check if filename already in Level cloud, if so, delete it
	if (g_gamecloud_gamelist.size() > 0)
	{
		for (int n = 0; n < g_gamecloud_gamelist.size(); n++)
		{
			if (stricmp(g_gamecloud_gamelist[n].Get(), pFileOnly) == NULL)
			{
				mp_gamecloud_delete(pFileOnly);
				break;
			}
		}
	}
	return 0;
}

void mp_gamecloud_deleteALLgamefiles(LPSTR pSiteCode)
{
	// delete everything, or anything that matches pSiteCode
	if (g_gamecloud_gamelist.size() > 0)
	{
		for (int n = 0; n < g_gamecloud_gamelist.size(); n++)
		{
			if (pSiteCode == NULL || (pSiteCode !=NULL && strnicmp(g_gamecloud_gamelist[n].Get(), pSiteCode, strlen(pSiteCode)) == NULL))
			{
				mp_gamecloud_delete(g_gamecloud_gamelist[n].Get());
			}
		}
	}
}

int mp_gamecloud_upload ( bool bStartUpload, LPSTR fileName, LPSTR pFinalFilenameToUse)
{
	if (bStartUpload == true && g_gamecloud_activity == 0) { g_gamecloud_activity = 1;  g_gamecloud_mode = 1; }
	if (g_gamecloud_activity == 1)
	{
		if (g_gamecloud_mode == 1)
		{
			// check file exists
			if (FileExist(fileName) == 1)
			{
				// start uploading the FPM to the level cloud
				LPSTR szServerFile = "/api/gameguru/multiplayer/storage/v2/upload";
				bool bSaveToFile = false;
				LPSTR szLocalFileOrAltNameForFileOnServer = pFinalFilenameToUse;
				LPSTR szUploadFile = fileName;
				LPSTR szPostData = FPMHOSTUPLOADKEY;
				if (SendFileInternal(szServerFile, bSaveToFile, szLocalFileOrAltNameForFileOnServer, szUploadFile, szPostData) == 1)
				{
					g_gamecloud_mode = 2;
				}
				else
				{
					strcpy(g_gamecloud_error, "Could not save the file to the level cloud");
					g_gamecloud_activity = 0;
					g_gamecloud_mode = 0;
					return -1;
				}
			}
			else
			{
				strcpy(g_gamecloud_error, "Please specify an FPM file you'd like to save to the level cloud");
				g_gamecloud_activity = 0;
				g_gamecloud_mode = 0;
				return -1;
			}
		}
		if (g_gamecloud_mode == 2)
		{
			DWORD dwDataReturnedSize = 0;
			LPSTR pDataReturned = NULL;
			if (SendFileInternalAsync(&pDataReturned, &dwDataReturnedSize) == true)
			{
				if (pDataReturned && strchr(pDataReturned, '{') != 0 && dwDataReturnedSize < 10240)
				{
					char pfilenameText[10240];
					strcpy(pfilenameText, "");
					char pWorkStr[10240];
					memset(pWorkStr, 0, sizeof(pWorkStr));
					strcpy(pWorkStr, pDataReturned);
					if (pWorkStr[0] == '{') strcpy(pWorkStr, pWorkStr + 1);
					int n = 10200;
					for (; n > 0; n--) if (pWorkStr[n] == '}') { pWorkStr[n - 1] = 0; break; }
					char* pChop = strstr(pWorkStr, ",");
					char pStatusStr[10240];
					strcpy(pStatusStr, pWorkStr);
					if (pChop) pStatusStr[pChop - pWorkStr] = 0;
					if (pChop[0] == ',') pChop += 1;
					if (strstr(pStatusStr, "success") != NULL)
					{
						// success
						if (strstr(pStatusStr, "true") != NULL)
						{
							// filename
							pChop = strstr(pChop, ":") + 2;
							strcpy(pfilenameText, pChop);

							// send has been completed
							g_gamecloud_activity = 0;
							g_gamecloud_mode = 0;
							return 1;
						}
					}
					else
					{
						// error
						char* pMessageValue = strstr(pChop, ":") + 1;
						strcpy(g_gamecloud_error, pMessageValue);
						g_gamecloud_activity = 0;
						g_gamecloud_mode = 0;
						return -1;
					}
				}
				else
				{
					// error uploading (likely permission denied due to invalid FPMHOSTUPLOADKEY)
					if (strstr(FPMHOSTUPLOADKEY, "[hostuploadkey]") != 0)
					{
						strcpy(g_gamecloud_error, "Ensure the build is using the proper server key code");
					}
					g_gamecloud_activity = 0;
					g_gamecloud_mode = 0;
					return -1;
				}
			}
		}
	}
	return 0;
}

float mp_gamecloud_getprogress(void)
{
	return SendFileInternalGetProgress();
}

LPSTR mp_gamecloud_geterror(void)
{
	return g_gamecloud_error;
}

// MP Server Client Syncing

void mp_pre_game_file_sync ( void )
{
	// handle file transfer activity
	if ( g.mp.isGameHost == 1 ) 
	{
		mp_pre_game_file_sync_server ( -1 );
	}
	else
	{
		mp_pre_game_file_sync_client ( );
	}
}

void mp_pre_game_file_sync_server ( int iOnlySendMapToSpecificPlayer )
{
	// vars
	static DWORD g_dwSendLastTime;
	cstr pFullPathAndFile = "";

	// if we have lost connection, head back to main menu
	t.tconnectionStatus = PhotonGetClientServerConnectionStatus();
	if ( t.tconnectionStatus  ==  0 ) 
	{
		t.tsteamconnectionlostmessage_s = "Lost Connection";
		g.mp.mode = MP_MODE_MAIN_MENU;
		mp_lostConnection ( );
		return;
	}

	// handle sending of avatar info
	//mp_sendAvatarInfo ( ); //done in game loop

	// check if we have finished sending and receiving textures with the server
	// (the actual process is handled by steam dll)
	 if ( g.mp.isGameHost == 0 ) return;

	// file send transfer sequence
	switch ( g.mp.syncedWithServerMode ) 
	{
		case 0:
			
			// for solo testing to prevent sending files
			if ( g.mp.usersInServersLobbyAtServerCreation <= 1 ) 
			{
				g.mp.syncedWithServerMode = 3;
				return;
			}

			// if host tries to send file to itself
			if ( iOnlySendMapToSpecificPlayer == PhotonGetMyRealPlayerNr() )
			{
				g.mp.syncedWithServerMode = 3;
				return;
			}

			// if try to send map to player who is already loaded and ready (gathered at start screen and loading done)
			if ( iOnlySendMapToSpecificPlayer != -1 )
			{
				if ( PhotonIsPlayerLoadedAndReady ( iOnlySendMapToSpecificPlayer ) == 1 )
				{
					g.mp.syncedWithServerMode = 3;
					return;
				}
			}

			// okay, we have a go to send the file to the specific player
			PhotonSetSendFileCount ( 1, iOnlySendMapToSpecificPlayer );
			pFullPathAndFile = "editors\\gridedit\\__multiplayerlevel__.fpm";
			PhotonSendFileBegin ( 1, pFullPathAndFile.Get(), g.fpscrootdir_s.Get() );
			g.mp.syncedWithServerMode = 1;
			mp_textDots(-1,30,3,"Setting up data for clients");
			g_dwSendLastTime = timeGetTime();
			break;

		case 1:
		{
			char pProgressFloat[1024];
			sprintf ( pProgressFloat, "%.1f", PhotonGetSendProgress() );
			cstr sShowSendingProgress = cstr("sharing files (") + pProgressFloat + "%) with incoming player";
			mp_textDots(-1,50,3,sShowSendingProgress.Get());
			// take precaution not to send too much too quickly (Photon Server will ise error 1040 and timeout!!)
			if ( timeGetTime() > g_dwSendLastTime )
			{
				g_dwSendLastTime = timeGetTime() + 1; // new FPM HOST Transfer to Server (much quicker and no drop out)
				int iSendFileStatus = PhotonSendFileDone();
				if ( iSendFileStatus == 1 )
				{
					g.mp.syncedWithServerMode = 2;
					g.mp.oldtime = MAXTimer();
				}
				else
				{
					if ( iSendFileStatus == -1 )
					{
						// error uploading (permissed denied)
						char pError[1024];
						PhotonGetSendError ( pError );
						t.tsteamconnectionlostmessage_s = cstr("Upload Error (") + pError + ")";
						g.mp.mode = MP_MODE_MAIN_MENU;
						mp_lostConnection ( );
					}
				}
			}
		}
		break;

		case 2:
			g.mp.syncedWithServerMode = 3;
			g.mp.oldtime = MAXTimer();
			break;

		case 3:
			g.mp.oldtime = MAXTimer();
			g.mp.syncedWithServer = 1;
			g.mp.syncedWithServerMode = 99;
			break;
	} 
}

void mp_pre_game_file_sync_client ( void )
{
	// if we have lost connection, head back to main menu
	t.tconnectionStatus = PhotonGetClientServerConnectionStatus();
	if ( t.tconnectionStatus == 0 ) 
	{
		t.tsteamconnectionlostmessage_s = "Lost Connection";
		g.mp.mode = MP_MODE_MAIN_MENU;
		mp_lostConnection ( );
		return;
	}

	switch ( g.mp.syncedWithServerMode ) 
	{
		case 0:

			if ( PhotonAmIFileSynced() == 1 ) 
			{
				// can NOW send that this joiner is ready (file received!)
				PhotonSendIAmLoadedAndReady (  );

				// start loading resources sequence
				g.mp.fileLoaded = 1;
				g.mp.syncedWithServerMode = 1;
			}
			else
			{
				// out progress downloading files from server
				float fProgress = PhotonGetFileProgress();

				// after 20 seconds, and no percentage change, produce timeout
				if ( MAXTimer() - g.mp.oldtime > 1000*20 ) 
				{
					g.mp.oldtime = MAXTimer();
					if ( fProgress == t.fLastProgress )
					{
						t.tsteamconnectionlostmessage_s = "Timed out waiting for transfer of file";
						g.mp.mode = MP_MODE_MAIN_MENU;
						mp_lostConnection ( );
					}
					t.fLastProgress = fProgress;
				}

				// if user presses ESCAPE, force a disconnect and leave
				bool bEscapeEarly = false;
				if ( ScanCode() == 57 ) bEscapeEarly = true; //EscapeKey() == 1 ) bEscapeEarly = true;
				if ( bEscapeEarly == true )
				{
					// forcing a quit
					t.tsteamconnectionlostmessage_s = "User terminated transfer and returning to main menu";
					g.mp.mode = MP_MODE_MAIN_MENU;
					mp_lostConnection ( );
				}

				// report progress of file download
				 char pProgressFloat[1024];
				 sprintf ( pProgressFloat, "%.1f", fProgress );
				 t.tstring_s = cstr("Receiving file: ") + pProgressFloat + "%";
				 mp_text(-1,95,3,"(press SPACE KEY to return to main menu)");
				mp_text(-1,85,3,t.tstring_s.Get());
			}
			break;

		case 1:
			g.mp.syncedWithServer = 1;
			SetDir ( t.toldsteamfolder_s.Get() );
			SetDir ( g.mp.originalpath.Get() );
			g.mp.syncedWithServerMode = 99;
			break;
	} 
	return;
}

void mp_sendAvatarInfo ( void )
{
	if ( g.mp.haveSentMyAvatar == 0 ) 
	{
		 g.mp.me = PhotonGetMyPlayerIndex();
		 //if ( g.mp.me <= 0 ) g.mp.me = 0;
		if ( 1 )
		{
			g.mp.haveSentMyAvatar = 1;
			 LPSTR pPlayerName = PhotonGetPlayerName();
			 int iRealPhotonPlayerNr = PhotonGetMyRealPlayerNr();
			 PhotonSendLuaPlayerSpecificString ( MP_LUA_SendAvatarName, iRealPhotonPlayerNr, pPlayerName );
			 PhotonSendLuaPlayerSpecificString ( MP_LUA_SendAvatar, iRealPhotonPlayerNr, g.mp.myAvatar_s.Get() );

			// store our own info for loading in our avatar
			t.mp_playerAvatarOwners_s[g.mp.me] = pPlayerName;
			t.mp_playerAvatars_s[g.mp.me] = g.mp.myAvatar_s;
			t.mp_playerAvatarLoaded[g.mp.me] = false;
			t.bTriggerAvatarRescanAndLoad = true;

			// send out custom texture (mp.myAvatarHeadTexture$ will be "" if we don't have one)
			 // No custom face image
		}
	}
	mp_lua ( );
}

void mp_animation ( void )
{
	 while ( PhotonGetAnimationList() ) 
	 {
		t.tEnt = PhotonGetAnimationIndex();
		t.astart = PhotonGetAnimationStart();
		t.aend = PhotonGetAnimationEnd();
		t.aspeed = PhotonGetAnimationSpeed();
		SetObjectSpeed ( t.entityelement[t.tEnt].obj,t.aspeed );
		PlayObject ( t.entityelement[t.tEnt].obj,t.astart,t.aend );
		PhotonGetNextAnimation (  );
	 }
}

void mp_update_player ( void )
{
	if ( g.mp.endplay == 1 ) return;

	 PhotonSetPlayerPositionX ( CameraPositionX() );
	 if ( g.mp.crouchOn == 0 ) 
	 {
		PhotonSetPlayerPositionY ( CameraPositionY()-64 );
	 }
	 else
	 {
		PhotonSetPlayerPositionY ( CameraPositionY()-64+30 );
 	 }
	 PhotonSetPlayerPositionZ ( CameraPositionZ() );
	 PhotonSetPlayerAngle ( t.camangy_f );
	g.mp.lastx = CameraPositionX();
	if ( g.mp.crouchOn == 0 ) 
	{
		g.mp.lasty = CameraPositionY()-64;
	}
	else
	{
		g.mp.lasty = CameraPositionY()-64+30;
	}
	g.mp.lastz = CameraPositionZ();
	g.mp.lastangley = t.camangy_f;

	t.tpe = t.mp_playerEntityID[g.mp.me];
	if ( t.tpe > 0 )
	{
		t.entityelement[t.tpe].x=g.mp.lastx;
		t.entityelement[t.tpe].y=g.mp.lasty;
		t.entityelement[t.tpe].z=g.mp.lastz;
		if ( t.entityelement[t.mp_playerEntityID[g.mp.me]].obj > 0 ) 
		{
			if ( ObjectExist(t.entityelement[t.mp_playerEntityID[g.mp.me]].obj) ) 
			{
				PositionObject ( t.entityelement[t.mp_playerEntityID[g.mp.me]].obj, g.mp.lastx, g.mp.lasty+10, g.mp.lastz );
			}
		}
		t.te = t.tpe;
		t.tolde = t.e;
		t.e = t.tpe;
		entity_updatepos ( );
		entity_lua_rotateupdate ( );
		t.e = t.tolde;
	}
}

void mp_updatePlayerPositions ( void )
{
	if ( g.mp.endplay == 1 ) return;

	// Get player data from the server
	for ( t.c = 0 ; t.c <= MP_MAX_NUMBER_OF_PLAYERS-1; t.c++ )
	{
		// get server data
		 int iAlive = PhotonGetPlayerAlive(t.c);
		 float fX = PhotonGetPlayerPositionX(t.c);
		 float fY = PhotonGetPlayerPositionY(t.c);
		 float fZ = PhotonGetPlayerPositionZ(t.c);
		 float fAngle = PhotonGetPlayerAngle(t.c);
		if ( t.mp_forcePosition[t.c] > 0 && iAlive == 1 ) 
		{
			if ( t.mp_forcePosition[t.c] == 1 ) t.mp_forcePosition[t.c] = MAXTimer();
			if ( MAXTimer() - t.mp_forcePosition[t.c] > 1000 ) 
			{
				t.mp_forcePosition[t.c] = 0;
				t.x_f = fX; // seem redundant 9and are if you look below!!)
				t.y_f = fY;
				t.z_f = fZ;
				 PhotonSetTweening ( t.c, 1 );
			}
			else
			{
				 PhotonSetTweening ( t.c, 0 );
			}
			t.x_f = fX;
			t.y_f = fY;
			t.z_f = fZ;
			t.angle_f = fAngle;
		}

		//  Get other players tweened positional data
		t.x_f = fX;
		t.y_f = fY;
		t.z_f = fZ;
		t.angle_f = fAngle;
		if ( t.c != g.mp.me ) 
		{
			if ( iAlive == 1 && t.mp_forcePosition[t.c] == 0 ) 
			{
				t.e = t.mp_playerEntityID[t.c];
				if ( t.e > 0 )
				{
					t.entityelement[t.e].x=t.x_f;
					t.entityelement[t.e].y=t.y_f;
					t.entityelement[t.e].z=t.z_f;
					t.entityelement[t.e].ry=t.angle_f;
					PositionObject ( t.entityelement[t.e].obj, t.entityelement[t.e].x, t.entityelement[t.e].y, t.entityelement[t.e].z );
					t.te = t.e;
					entity_updatepos ( );
					entity_lua_rotateupdate ( );
				}
			}
		}
	}
}

void mp_server_message ( void )
{

if (  g.mp.endplay  ==  1  )  return;

t.s_s = SteamGetServerMessage();
if (  g.mp.coop  ==  1 ) 
{
	t.tplayer_s = FirstToken(t.s_s.Get()," ");
	t.tcheckforkilled_s = NextToken(" ");
	if (  t.tcheckforkilled_s  ==  "was" || t.tcheckforkilled_s  ==  "killed"  )  t.s_s  =  t.tplayer_s + " died!";
}
if (  t.s_s  !=  "" ) 
{
	t.tsteamdisplaymessagetimer = MAXTimer();
	t.s_s = Upper(t.s_s.Get());
}
if (  t.s_s  ==  ""  )  t.s_s  =  g.mp.previousMessage_s;
g.mp.previousMessage_s = t.s_s;
if (  MAXTimer() - t.tsteamdisplaymessagetimer < 2000  )  mp_text(-1,10,3,t.s_s.Get());
// `text GetDisplayWidth()/2 - Text (  width(s$)/2, 100, s$ )
}

void mp_updatePlayerNamePlates ( void )
{
	// Display players names  
	for ( t.c = 0 ; t.c <= MP_MAX_NUMBER_OF_PLAYERS-1; t.c++ )
	{
		t.e = t.mp_playerEntityID[t.c];
		LPSTR pDisplayName = PhotonGetLobbyUserDisplayName ( t.c );
		if ( pDisplayName && t.entityelement[t.e].obj > 0 && ObjectExist(t.entityelement[t.e].obj) == 1 && GetVisible(t.entityelement[t.e].obj) == 1 )
		{
			char pFinalDisplayName[1024];
			strcpy ( pFinalDisplayName, pDisplayName );
			strupr ( pFinalDisplayName );
			strcpy ( pFinalDisplayName+1, pDisplayName+1 );
			t.s_s = pFinalDisplayName;
		}
		else
			t.s_s = "";
		lua_promptlocalcore ( 2 );
	}
}

