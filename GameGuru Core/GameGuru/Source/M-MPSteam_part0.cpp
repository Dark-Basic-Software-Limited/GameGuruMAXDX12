//----------------------------------------------------
//--- GAMEGURU - M-Multiplayer
//----------------------------------------------------

#include "gameguru.h"

#ifdef FREETRIALVERSION

// No multiplayer in free trial version
void mp_init ( void ) {}
void mp_loop ( void ) {}
void mp_free ( void ) {}
void mp_checkVoiceChat ( void ) {}
void mp_spawn_objects ( void ) {}
void mp_lua ( void ) {}
void mp_delete_entities ( void ) {}
void mp_pre_game_file_sync ( void ) {}
void mp_pre_game_file_sync_server ( void ) {}
void mp_pre_game_file_sync_client ( void ) {}
void mp_sendAvatarInfo ( void ) {}
void mp_animation ( void ) {}
void mp_update_player ( void ) {}
void mp_updatePlayerPositions ( void ) {}
void mp_server_message ( void ) {}
void mp_updatePlayerNamePlates ( void ) {}
void mp_updatePlayerAnimations ( void ) {}
void mp_switchAnim ( void ) {}
void mp_update_waist_rotation ( void ) {}
void mp_showdeath ( void ) {}
void mp_respawn ( void ) {}
void mp_getPlaceToSpawn ( void ) {}
void mp_getInitialPlayerCount ( void ) {}
void mp_nukeTestmap ( void ) {}
void mp_respawnEntities ( void ) {}
void mp_addDestroyedObject ( void ) {}
void mp_add_respawn_timed ( void ) {}
void mp_setLuaPlayerNames ( void ) {}
void mp_setLuaResetStats ( void ) {}
void mp_updatePlayerInput ( void ) {}
void mp_load_guns ( void ) {}
void mp_check_for_attachments ( void ) {}
void mp_addJetpackParticles ( void ) {}
void mp_NearOtherPlayers ( void ) {}
void mp_check_respawn_objects ( void ) {}
void mp_checkForEveryoneLeft ( void ) {}
void mp_lostConnection ( void ) {}
void mp_gameLoop ( void ) {}
void mp_dontShowOtherPlayers ( void ) {}
void mp_ending_game ( void ) {}
void mp_free_game ( void ) {}
void mp_subbedToItem ( void ) {}
void mp_checkItemSubbed ( void ) {}
void mp_resetGameStats ( void ) {}
void mp_update_all_projectiles ( void ) {}
void mp_destroyentity ( void ) {}
void mp_refresh ( void ) {}
void mp_setMessage ( void ) {}
void mp_setMessageDots ( void ) {}
void mp_message ( void ) {}
void mp_messageDots ( void ) {}
void mp_update_projectile ( void ) {}
void mp_serverSetLuaGameMode ( void ) {}
void mp_setServerTimer ( void ) {}
void mp_serverRespawnAll ( void ) {}
void mp_restoreEntities ( void ) {}
void mp_serverEndPlay ( void ) {}
void mp_setServerKillsToWin ( void ) {}
void mp_networkkill ( void ) {}
void mp_lobbyListBox ( void ) {}
void mp_createLobby ( void ) {}
void mp_searchForLobbies ( void ) {}
void mp_searchForFpms ( void ) {}
void mp_launchGame ( void ) {}
void mp_backToStart ( void ) {}
void mp_selectedALevel ( void ) {}
void mp_checkIfLevelHasCustomContent ( void ) {}
void mp_buildWorkShopItem ( void ) {}
void mp_buildingWorkshopItemFailed ( void ) {}
void mp_joinALobby ( void ) {}
void mp_canIJoinThisLobby ( void ) {}
void mp_leaveALobby ( void ) {}
void mp_SubscribeToWorkShopItem ( void ) {}
void mp_save_workshop_files_needed ( void ) {}
void mp_grabWorkshopChangedFlagAndVersion ( void ) {}
int mp_check_if_entity_is_from_install ( char* name_s ) { return 0; }
void mp_resetSteam ( void ) {}
void mp_shoot ( void ) {}
void mp_chat ( void ) {}
void mp_chatNew ( void ) {}

void mp_quitGame ( void ) 
{
	t.tstartoffade = Timer();
	t.tfadestealpha_f = 0.0;
	t.tspritetouse = 0;
	for ( t.tloop = 2000 ; t.tloop<=  3000; t.tloop++ )
	{
		if (  SpriteExist(t.tloop)  ==  0 ) { t.tspritetouse  =  t.tloop  ; break; }
	}
	while (  Timer() - t.tstartoffade < 500 ) 
	{
		t.tfadestealpha_f = (Timer() - t.tstartoffade)*2;
		if (  t.tfadestealpha_f < 0  )  t.tfadestealpha_f  =  0.0;
		if (  t.tfadestealpha_f > 255.0  )  t.tfadestealpha_f  =  255.0;
		if (  t.tspritetouse > 0 && ImageExist(g.panelimageoffset+1)  ==  1 ) 
		{
			Sprite (  t.tspritetouse,0,0,g.panelimageoffset+1 );
			SizeSprite (  t.tspritetouse,GetDisplayWidth()*10, GetDisplayHeight()*10 );
			SetSpriteDiffuse (  t.tspritetouse,0,0,0 );
			SetSpriteAlpha (  t.tspritetouse,t.tfadestealpha_f );
		}
		//SteamLoop (  );
		Sync (  );
	}
	if (  g.mp.isGameHost  ==  1 ) 
	{
		//SteamEndGame (  );
	}
	t.game.gameloop=0;
	t.game.levelloop=0;
	t.game.titleloop=0;
	t.game.quitflag=1;
}

void mp_freefadesprite ( void ) {}
void mp_backToEditor ( void ) {}
void mp_cleanupGame ( void ) {}
void mp_sendSteamIDToEditor ( void ) {}
void mp_checkIfLobbiesAvailable ( void ) {}
void mp_flashLightOff ( void ) {}
void mp_setupCoopTeam ( void ) {}
void mp_COOP_aiMoveTo ( void ) {}
void mp_entity_lua_lookatplayer ( void ) {}
void mp_entity_lua_fireweaponEffectOnly ( void ) {}
void mp_updateAIForCOOP ( void ) {}
void mp_coop_rotatetoplayer ( void ) {}
void mp_storeOldEntityPositions ( void ) {}
void mp_howManyEnemiesLeftToKill ( void ) {}
void mp_IKilledAnAI ( void ) {}
void mp_text ( int x, int y, int size, char* txt_s ) {}
void mp_textDots ( int x, int y, int size, char* txt_s ) {}
void mp_textColor ( int x, int y, int size, char* txt_s, int r, int g, int b ) {}
void mp_panel ( int x, int y, int x2, int y2 ) {}
void mp_livelog ( char* t_s ) {}
void mp_deleteFile ( char* tempFileToDelete_s ) {}
int mp_check_if_lua_entity_exists ( int tentitytocheck ) { return 0; }
void mp_sendlua ( int code, int e, int v ) {}

void SteamApplyPlayerDamage ( int index, int damage, int x, int y, int z, int force, int limb ) {}
int SteamGetPlayerAlive ( int a ) { return 0; }
char* SteamGetWorkshopItemPath ( void ) { return ""; }
void SteamDestroy ( int a ) { }
void SteamShowAgreement ( void ) { }
int SteamInit ( void ) { return 0; }
void SteamGetWorkshopItemPathDLL ( char* ) {}
int SteamIsWorkshopLoadingOnDLL ( void ) { return 0; }
void SteamSendLua ( int a, int b, int c ) {}



#else

// flag to switch workshop handling from workshop to game managed, by default set to false, set to true for multiplayer mode
extern bool OnlineMultiplayerModeForSharingFiles;

//  Startup Steam
void mp_init ( void )
{
	timestampactivity(0,"_mp_init:");
	t.mp_build = 1121;
	g.mp.isRunning = SteamInit();
	g.mp.mode = MP_MODE_NONE;
	g.mp.dontDrawTitles = 0;
	g.mp.message = "";
	g.mp.messageTime = 0;

	// If a custom character head is used but the image no longer exists, we need to get rid of the avatar file
	characterkit_checkAvatarExists();
}

bool OccluderCheckingForMultiplayer ( void )
{
	if ( t.game.runasmultiplayer == 0 ) return false;

	return true;
}

void mp_loop ( void )
{

	SteamLoop (  );

	//  store old positions of entities if in coop mode
	/*      
	if (  g.mp.coop  ==  1 ) 
	{
		if (  g.mp.madeArrays   ==  0 ) 
		{
			g.mp.madeArrays = 1;
			mp_storeOldEntityPositions ( );
		}
	}
	*/    

	//  debug stuff for loading in custom cc avatars
	/*      
	SetCursor (  0,0 );
	Print (  "My avatar string = " + g.mp.myAvatar_s );
	for ( t.c = 0 ; t.c<=  7; t.c++ )
	{
		Print (  t.mp_playerAvatarOwners_s[t.c] );
		Print (  t.mp_playerAvatars_s[t.c] );
	}
	Print (  "have sent mine " + Str(g.mp.haveSentMyAvatar) );
	*/    

	if (  g.mp.mode  ==  MP_MODE_NONE || t.game.runasmultiplayer  ==  0  )
	{
		// usual workshop mode
		OnlineMultiplayerModeForSharingFiles = false;
		return;
	}

	// game managed mode for sharing files
	OnlineMultiplayerModeForSharingFiles = true;

	if (  g.mp.mode  !=  MP_IN_GAME_CLIENT && g.mp.mode  !=  MP_IN_GAME_SERVER ) 
	{

		g.mp.finishedLoadingMap = 0;

		//  200315 - 021 - flashlight of when starting a game
		mp_flashLightOff ( );
		g.mp.originalEntitycount = 0;
		if (  SpriteExist(g.steamchatpanelsprite)  )  DeleteSprite (  g.steamchatpanelsprite );
		g.mp.dontDrawTitles = 0;
		//  If not connected to steam, retry
		if (  g.mp.isRunning  ==  0 || g.mp.needToResetOnStartup  ==  1 ) 
		{
			g.mp.goBackToEditor = 0;
			mp_resetSteam ( );
			if (  g.mp.isRunning  ==  0 ) 
			{
				t.tsteamlostconnectioncustommessage_s = "Cannot connect to Steam (Error MP001)";
				g.mp.backtoeditorforyou = 2;
				mp_lostConnection ( );
				return;
			}
		}
		else
		{
			g.mp.backtoeditorforyou = 0;
		}

		//  Debug Info
		t.steamDoDropShadow = 1;
		t.ttstring_s = cstr("Multiplayer Build ") + Str(t.mp_build);
		mp_text(-1,98,2,t.ttstring_s.Get());


	}
	else
	{
		//  030315 - 013 - Lobby chat
		t.tchatLobbyMode = 0;
		mp_chat ( );
	}
	if (  g.mp.mode  ==  MP_MODE_MAIN_MENU ) 
	{
		//  show avatar name if there is one
		if (  g.mp.myAvatarName_s != "" ) 
		{
		if (  ImageExist(g.charactercreatorEditorImageoffset)  ==  0 ) 
		{
			t.tShowAvatarSprite = 1;
			characterkit_loadMyAvatarInfo ( );
		}
			t.tYPos_f = 95;
			if (  GetDisplayHeight() > 900  )  t.tYPos_f  =  92;
			mp_text(-1,t.tYPos_f,2,g.mp.myAvatarName_s.Get());
			if (  g.charactercreatorEditorImageoffset > 0 ) 
			{
				if (  ImageExist(g.charactercreatorEditorImageoffset)  ==  1 ) 
				{
					t.tYPos_f = GetChildWindowHeight();
					t.tYPos_f = t.tYPos_f * 0.85;
					PasteImage (  g.charactercreatorEditorImageoffset, (GetChildWindowWidth()/2)-32, t.tYPos_f,g.charactercreatorEditorImageoffset );
				}
			}
		}
		g.mp.dontDrawTitles = 0;
		if (  g.mp.originalpath  ==  "" ) 
		{
			g.mp.originalpath = GetDir();
			SteamSetRoot(cstr(g.fpscrootdir_s+"\\Files\\").Get());
		}
		//  110315 - 019 - remove fadeoutsprite if it exists
		if (  t.tspritetouse > 0 ) 
		{
			if (  SpriteExist(t.tspritetouse)  ==  1  )  DeleteSprite (  t.tspritetouse );
			t.tspritetouse = 0;
		}
		/*      
		if (  SpaceKey() && oldspacekey  ==  0 ) 
		{
			SteamSetRoot(g.fpscrootdir_s+"\\Files\\");
			SteamCreateWorkshopItem (  "Awesome Custom Level" );
		}
		if (  LeftKey() && oldleftkey  ==  0 ) 
		{
//mp_save_workshop_files_needed ( );
			SteamDownloadWorkshopItem (  "378579107" );
			Print (  "HELLO" );
		}
		if (  SteamIsWorkshopItemDownloaded()  ==  1 ) 
		{
			mp_text(-1,5,3,"WORKSHOP ITEM DOWNLOADED");
//    `if ImageExist(999) = 0

//     `load image "F:\\TGCSHARED\\fpsc-reloaded\\FPS Creator Files\\Files\\entitybank\\ravey\\fizco\\fizzie.jpg",999

//     `sprite 999,0,0,999

//    `endif

		}
		else
		{
			mp_text(-1,5,3,"WORKSHOP ITEM NOT DOWNLOADED");
		}
		oldleftkey = LeftKey();
		oldspacekey = SpaceKey();
		*/    

//   `print "======================================"

//   `print "(C) Create Lobby"

//   `print "(S) Search for Lobbies"

//   `print "======================================"


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
		if (  g.charactercreatorEditorImageoffset > 0 ) 
		{
			if (  ImageExist(g.charactercreatorEditorImageoffset)  )  DeleteImage (  g.charactercreatorEditorImageoffset );
		}
	}

	if (  g.mp.mode  ==  MP_WAITING_FOR_LOBBY_CREATION ) 
	{
//   `print "======================================"

//   `print "Creating Lobby"

//   `print "======================================"

		if (  SteamIsLobbyCreated()  ==  1 ) 
		{
			g.mp.isLobbyCreated = 1;
			SteamGetLobbyList (  );
			g.mp.mode = MP_MODE_LOBBY;
		}
		else
		{
			g.mp.haveToldAboutSolo = 0;
			if (  Timer() - t.tempsteamlobbycreationtimeout > 5000 ) 
			{
				t.tsteamlostconnectioncustommessage_s = "Could not create lobby (Error MP002)";
				mp_lostConnection ( );
				return;
			}
			if (  g.mp.isRunning  ==  0 ) 
			{
				t.tsteamlostconnectioncustommessage_s = "Cannot connect to Steam (Error MP003)";
				g.mp.backtoeditorforyou = 2;
				mp_lostConnection ( );
				return;
			}
		}
	}

	if (  g.mp.mode  ==  MP_ASKING_IF_SUBSCRIBE_TO_WORKSHOP_ITEM ) 
	{
		mp_text(-1,45,3,"You do not currently have the workshop item required to");
		mp_text(-1,50,3,"join this game. Do you wish to subscribe to the workshop");
		mp_text(-1,55,3,"item so you can join a game with this level at a later time?");
		mp_text(-1,65,3,"Note: Once you have subscribed the Lobby will remain yellow until");
		mp_text(-1,70,3,"you have downloaded the whole workshop item.");
		t.tempsteamhaveaskedtosubscribeflag = 0;
	}

	if (  g.mp.mode  ==  MP_TELLING_THEY_NEED_TO_RESTART ) 
	{
		mp_text(-1,45,3,"Your version of this workshop item is outdated.");
		mp_text(-1,50,3,"To enable Steam to download the update you will need to:");
		mp_text(-1,55,3,"Exit multiplayer, then exit Game Guru completely.");
		mp_text(-1,60,3,"Then restart Game Guru and Steam will update all");
		mp_text(-1,65,3,"your subscriptions.");
	}

	if (  g.mp.mode  ==  MP_ASKING_IF_SUBSCRIBE_TO_WORKSHOP_ITEM_WAITING_FOR_RESULTS ) 
	{
		if (  t.tempsteamhaveaskedtosubscribeflag  ==  0 ) 
		{
			t.tempsteamhaveaskedtosubscribeflag = 1;
			SteamDownloadWorkshopItem (  g.mp.workshopidtojoin.Get() );
		}
		if (  SteamHasSubscriptionWorkshopItemFinished()  ==  0 ) 
		{
			if (  Timer() - g.mp.oldtime > 150 ) 
			{
				g.mp.oldtime = Timer();
				t.tSteamBuildingWorkshopItem_s = t.tSteamBuildingWorkshopItem_s + ".";
				if (  Len(t.tSteamBuildingWorkshopItem_s.Get()) > 5  )  t.tSteamBuildingWorkshopItem_s  =  ".";
			}
			mp_text(-1,50,3,cstr( cstr("Subscribing you") + t.tSteamBuildingWorkshopItem_s).Get() );
		}
		if (  SteamHasSubscriptionWorkshopItemFinished()  ==  1 ) 
		{
			mp_text(-1,50,3,"You are now subscribed to:");
			mp_text(-1,55,3,g.mp.levelnametojoin.Get());
			mp_text(-1,65,3,"Press back and wait for this level to install");
			mp_text(-1,70,3,"(the lobby will turn from yellow to white)");
			mp_text(-1,75,3,"then re-join the lobby.");
		}
		if (  SteamHasSubscriptionWorkshopItemFinished()  ==  -1 ) 
		{
			mp_text(-1,50,3,"Subscription failed");
			mp_text(-1,55,3,"Please t.try again in t.a few moments");
		}
	}

	if (  g.mp.mode  ==  MP_SERVER_CHOOSING_FPM_TO_USE ) 
	{
		mp_text(-1,5,3,"LIST OF LEVELS");
		mp_lobbyListBox ( );
	}

	if (  g.mp.mode  ==  MP_SERVER_CHOOSING_TO_MAKE_FPS_WORKSHOP ) 
	{
		mp_text(-1,30,3,"This level contains custom content.");
		mp_text(-1,35,3,"To share this level with others you will need to create a workshop item.");
		mp_text(-1,40,3,"(This is done automatically for you)");
		mp_text(-1,50,3,"Once your level is a workshop item other players can play your level.");
		mp_text(-1,60,3,"Do you wish to create (or update if you have share this level before)");
		mp_text(-1,65,3,"A workshop item?");
		mp_text(-1,75,3,"By submitting this item, you agree to the workshop terms of service");
	
//   `mp.oldtime = 0

	}

	if (  g.mp.mode  ==  MP_CREATING_WORKSHOP_ITEM ) 
	{
		if (  Timer() - g.mp.oldtime > 150 ) 
		{
			g.mp.oldtime = Timer();
			t.tSteamBuildingWorkshopItem_s = t.tSteamBuildingWorkshopItem_s + ".";
			if (  Len(t.tSteamBuildingWorkshopItem_s.Get()) > 5  )  t.tSteamBuildingWorkshopItem_s  =  ".";
		}
		t.tstring_s = t.tSteamBuildingWorkshopItem_s + "Building Workshop Item" + t.tSteamBuildingWorkshopItem_s;
		mp_text(-1,50,3,t.tstring_s.Get());
		t.tstring_s = "";
	
//  `mp_text(0,10,3, "mp.buildingWorkshopItemMode = " + Str(mp.buildingWorkshopItemMode) )

//  `mp_text(0,20,3, "mp.workshopid = " + mp.workshopid )

	
	}

	if (  g.mp.mode  ==  MP_MODE_LOBBY ) 
	{
		if (  g.mp.isGameHost  ==  0 ) 
		{

			if (  g.mp.isRunning  ==  0 ) 
			{
				t.tsteamlostconnectioncustommessage_s = "Cannot connect to Steam (Error MP004)";
				g.mp.backtoeditorforyou = 2;
				mp_lostConnection ( );
				return;
			}
//    `print "======================================"

//    `print "Lobby list"

//    `print "======================================"

			mp_text(-1,5,3,"LIST OF LOBBIES");
			if (  SteamIsLobbyListCreated()  ==  0 ) 
			{

				if (  g.mp.lobbycount  ==  0 ) 
				{
					t.tstring_s = "Building Lobby list";
					mp_text(-1,10,1,t.tstring_s.Get());
				}
				else
				{
//      `if mp.lobbycount = 1

//       `tstring$ = "1 lobby found"

//      `else

//       `tstring$ = Str(tsize) + " lobbies found"

//      `endif

//      `mp_text(-1,15,1,tstring$)


				}

				if (  Timer() - g.mp.oldtime > 3000 ) 
				{
					SteamGetLobbyList (  );
					g.mp.oldtime = Timer();
				}
			}
			else
			{
				if (  Timer() - g.mp.oldtime > 3000 ) 
				{
					SteamGetLobbyList (  );
					g.mp.oldtime = Timer();
				}
			}

			mp_lobbyListBox ( );

		}
		else
		{
			//  030315 - 013 - Lobby chat
			t.tchatLobbyMode = 1;
			mp_chat ( );
			mp_text(-1,85,3,"Press Enter to chat");

			t.tUserCount = SteamGetLobbyUserCount();
			if (  Timer() - t.tempsteamlobbycreationtimeout > 5000 && t.tUserCount  ==  0 ) 
			{
				t.tsteamlostconnectioncustommessage_s = "Could not create lobby (Error MP005)";
				mp_lostConnection ( );
				return;
			}
//    `print "======================================"

//    `print mp.playerName + "'s Lobby"

//    `print "======================================"

//    `print "(S) Start Server"

//    `print "======================================"

			if (  t.tUserCount  ==  1 ) 
			{
				t.tstring_s = "There is 1 user (you!) in this lobby";
				g.mp.usersInServersLobbyAtServerCreation = 1;
			}
			else
			{
				t.tstring_s = cstr("There are ") + Str(t.tUserCount) + " users in this lobby";
			}
			if (  t.tUserCount  !=  g.mp.usersInServersLobbyAtServerCreation ) 
			{
				g.mp.haveSentMyAvatar = 0;
			}
			if (  t.tUserCount > g.mp.usersInServersLobbyAtServerCreation ) 
			{
				g.mp.usersInServersLobbyAtServerCreation = t.tUserCount;
			}
			mp_text(-1,15,1,t.tstring_s.Get());
			t.tsteamy_f = 50.0 - (t.tUserCount * 2.5);
			t.tsteamy = t.tsteamy_f;
			for ( t.tn = 1 ; t.tn<=  t.tUserCount; t.tn++ )
			{
				t.tstring_s = cstr("Player ") + Str(t.tn) + ": " + SteamGetLobbyUserName(t.tn-1);
				if (  SteamGetPlayerName()  !=  SteamGetLobbyUserName(t.tn-1)  )  t.mp_joined[t.tn-1]  =  SteamGetLobbyUserName(t.tn-1);
				mp_text(-1,t.tsteamy,1,t.tstring_s.Get());
				t.tsteamy += 5;
			}
			for ( t.tn = t.tUserCount ; t.tn<=  MP_MAX_NUMBER_OF_PLAYERS-1; t.tn++ )
			{
				t.mp_joined[t.tn] = "";
			}
				if (  g.mp.haveToldAboutSolo  ==  1 && t.tUserCount  <=  1 ) 
				{
					mp_textColor(-1,70,1,"Noone has joined your lobby yet. If you start now you will be playing alone.",255,100,100);
					mp_textColor(-1,75,1,"Press Start Server again to start anyway.",255,100,100);
				}

		if (  g.mp.launchServer  ==  1 ) 
		{
					if (  g.mp.haveToldAboutSolo  ==  0 && t.tUserCount  <=  1 ) 
					{
						g.mp.haveToldAboutSolo = 1;
						g.mp.launchServer = 0;
						return;
					}
					SteamStartServer (  );
				g.mp.mode = MP_WAITING_FOR_SERVER_CREATION;
				g.mp.oldtime = Timer();
			}
		}
	}

	if (  g.mp.mode  ==  MP_JOINING_LOBBY ) 
	{
		if (  SteamIsGameRunning()  ==  1 ) 
		{
			g.mp.mode = MP_IN_GAME_CLIENT;
			g.mp.needToResetOnStartup = 1;
			t.toldsteamfolder_s=GetDir();
			SetDir (  cstr(g.fpscrootdir_s + "\\Files\\editors\\gridedit").Get() );
			t.tsteamtimeoutongamerunning = Timer();
			t.tPlayerIndex = SteamGetMyPlayerIndex();
			if (  t.tPlayerIndex  >=  0 && t.tPlayerIndex < MP_MAX_NUMBER_OF_PLAYERS ) 
			{
				t.mp_health[t.tPlayerIndex] = 0;
				t.ta = MouseMoveX() + MouseMoveY();
			}
		}
		if (  t.tjoinedLobby  ==  0 ) 
		{
			if (  Timer() - g.mp.AttemptedToJoinLobbyTime > MP_JOIN_LOBBY_TIMEOUT ) 
			{
				g.mp.mode = MP_MODE_MAIN_MENU;
				t.tmsg_s = "Could not join Lobby";
				mp_setMessage ( );
			}
		}
		if (  SteamHasJoinedLobby()  ==  1 ) 
		{
			t.tjoinedLobby = 1;
			if (  t.tjoinedLobby  ==  0 ) 
			{
					t.tsteamwaitedforlobbytimer = Timer();
			}
		}
		else
		{
			t.tsteamwaitedforlobbytimer = Timer();
			t.tsteamistheownerpresenttime = t.tsteamwaitedforlobbytimer;
		}
		if (  t.tjoinedLobby  ==  0 ) 
		{
				t.tsteamwaitedforlobbytimer = Timer();
				t.tsteamistheownerpresenttime = Timer();
		}
		if (  t.tjoinedLobby  ==  1 ) 
		{
			if (  SteamHasJoinedLobby()  ==  1 ) 
			{
					//  030315 - 013 - Lobby chat
					t.tchatLobbyMode = 1;
					mp_chat ( );
					mp_text(-1,85,3,"Press Enter to chat");
					t.tsteamlobbertimer = Timer();
//     `print "======================================"

//     `print "In lobby"

//     `print "======================================"

				if (  t.tUserCount  !=  SteamGetLobbyUserCount() ) 
				{
					g.mp.haveSentMyAvatar = 0;
				}
				t.tUserCount = SteamGetLobbyUserCount();
				if (  t.tUserCount  ==  1 && Timer() - t.tsteamwaitedforlobbytimer > 15000 ) 
				{
					if (  SteamIsGameRunning()  ==  0 ) 
					{
						SteamLeaveLobby (  );
						t.tsteamlostconnectioncustommessage_s = "Lost connection to lobby (Error MP006)";
						mp_lostConnection ( );
						return;
					}
				}
				else
				{
					t.tsteamwaitedforlobbytimer = Timer();
					mp_text(-1,15,1, cstr(cstr("There are ") + Str(t.tUserCount) + " users in this lobby").Get() );
					mp_text(-1,10,1, cstr(cstr("Game being hosted is '") + g.mp.levelnametojoin + "'").Get() );

				}
				t.tsteamistheownerpresent = 0;
				t.tsteamnamewearelookingfor_s = Left(g.mp.lobbyjoinedname.Get(),Len(g.mp.lobbyjoinedname.Get())-8);

				t.tsteamy_f = 50.0 - (t.tUserCount * 2.5);
				t.tsteamy = t.tsteamy_f;

				if (  t.tsteamnamewearelookingfor_s  ==  SteamGetLobbyUserName(0) ) 
				{
					for ( t.tn = t.tUserCount ; t.tn<=  MP_MAX_NUMBER_OF_PLAYERS-1; t.tn++ )
					{
						t.mp_joined[t.tn] = "";
					}
					for ( t.tn = 1 ; t.tn<=  t.tUserCount; t.tn++ )
					{
						if (  SteamGetPlayerName()  !=  SteamGetLobbyUserName(t.tn-1)  )  t.mp_joined[t.tn-1]  =  SteamGetLobbyUserName(t.tn-1);
					}
				}
				for ( t.tn = 1 ; t.tn<=  t.tUserCount; t.tn++ )
				{
					if (  t.tn  ==  1 ) 
					{
						t.tstring_s = cstr("Player ") + Str(t.tn) + ": " + SteamGetLobbyUserName(t.tn-1) + " (Host)";
					}
					else
					{
						t.tstring_s = cstr("Player ") + Str(t.tn) + ": " + SteamGetLobbyUserName(t.tn-1);
					}
					mp_text(-1,t.tsteamy,1,t.tstring_s.Get());
					t.tsteamy += 5;
					if (  t.tsteamnamewearelookingfor_s  ==  SteamGetLobbyUserName(t.tn-1) ) 
					{
						t.tsteamistheownerpresent = 1;
						t.tsteamistheownerpresenttime = Timer();
					}
				}
				if (  t.tsteamistheownerpresent  ==  0 && Timer() - t.tsteamistheownerpresenttime > 10000 ) 
				{
					SteamLeaveLobby (  );
					t.tsteamlostconnectioncustommessage_s = "The host left the lobby (Code MP007)";
					mp_lostConnection ( );
					return;
				}

				t.tsteamlobbertimer = Timer();
			}
			else
			{
				mp_textDots(-1,20,3,"Game Starting...Connecting");
				if (  Timer() - t.tsteamlobbertimer > 20000 ) 
				{
					if (  SteamGetClientServerConnectionStatus()  ==  0 ) 
					{
						t.tsteamlostconnectioncustommessage_s = "Lost connection to host (Error MP008)";
						mp_lostConnection ( );
						return;
					}
				}
			}
		}
		else
		{
//    `print "======================================"

//    `print "Joining lobby"

//    `print "======================================"

		}
	}
	if (  g.mp.mode  ==  MP_WAITING_FOR_SERVER_CREATION ) 
	{
		g.mp.dontDrawTitles = 1;
		if (  SteamIsServerRunning()  ==  1 ) 
		{
			mp_textDots(-1,10,3,"Server Started");
			if (  SteamIsGameRunning()  ==  1 ) 
			{
				g.mp.mode = MP_IN_GAME_SERVER;
				g.mp.needToResetOnStartup = 1;
				t.toldsteamfolder_s=GetDir();
				SetDir (  cstr(g.fpscrootdir_s + "\\Files\\editors\\gridedit").Get() );
				t.tPlayerIndex = SteamGetMyPlayerIndex();
				SteamSetSendFileCount (  1 );
			}
			else
			{
				if (  Timer() - g.mp.oldtime > 150 ) 
				{
					g.mp.oldtime = Timer();
					t.tStartingServerCount_s = t.tStartingServerCount_s + ".";
					if (  Len(t.tStartingServerCount_s.Get()) > 5  )  t.tStartingServerCount_s  =  ".";
				}
				t.tstring_s = t.tStartingServerCount_s + "Waiting for game to start" + t.tStartingServerCount_s;
				mp_text(-1,25,3,t.tstring_s.Get());
				t.tstring_s = "";
			}
		}
		else
		{
			if (  Timer() - g.mp.oldtime > 150 ) 
			{
				g.mp.oldtime = Timer();
				t.tStartingServerCount_s = t.tStartingServerCount_s + ".";
				if (  Len(t.tStartingServerCount_s.Get()) > 5  )  t.tStartingServerCount_s  =  ".";
			}
			t.tstring_s = t.tStartingServerCount_s + "Starting server" + t.tStartingServerCount_s;
			mp_text(-1,15,3,t.tstring_s.Get());
			t.tstring_s = "";
		}
	}

	if (  g.mp.mode  ==  MP_IN_GAME_SERVER ) 
	{
			g.mp.dontDrawTitles = 1;
			if (  g.mp.iHaveSaidIAmReady  ==  0 ) 
			{
				SteamSendIAmLoadedAndReady (  );
				g.mp.iHaveSaidIAmReady = 1;
				t.tempsteamingameinitialwaitingdelay = Timer();
				while (  Timer() - t.tempsteamingameinitialwaitingdelay < 20000 ) 
				{
					g.mp.syncedWithServerMode = 0;
					g.mp.okayToLoadLevel = 0;
					SteamLoop (  );
					mp_textDots(-1,20,3,"Waiting for other players");
					if (  Timer() - t.tsteamiseveryoneloadedandreadytime > 1000 ) 
					{
						t.tsteamiseveryoneloadedandreadytime = Timer();
						if (  SteamIsEveryoneLoadedAndReady()  ==  1  )  t.tempsteamingameinitialwaitingdelay  =  -30000;
					}
				}
				t.tskipLevelSync = Timer();
			}

			//wait for everyone before starting to load, at this GetPoint (  they have all the files they need, they just have not loaded them )
			if (  g.mp.okayToLoadLevel  ==  0 && g.mp.syncedWithServerMode  ==  99 ) 
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
				t.tskipLevelSync = Timer();
			}
			else
			{
				if (  g.mp.playGame  ==  1 ) 
				{
					if (  t.game.titleloop == 1 ) 
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
				if (  g.mp.okayToLoadLevel  ==  0 ) 
				{
					mp_pre_game_file_sync_server ( 0 );
				}
			}

	}
	if (  g.mp.mode  ==  MP_IN_GAME_CLIENT ) 
	{
			if (  t.titlespage  ==  11 ) 
			{
				g.mp.dontDrawTitles = 0;
			}
			else
			{
				g.mp.dontDrawTitles = 1;
			}
			g.mp.dontDrawTitles = 1;
			if (  g.mp.iHaveSaidIAmReady  ==  0 ) 
			{
				SteamSendIAmLoadedAndReady (  );
				g.mp.iHaveSaidIAmReady = 1;
				t.tempsteamingameinitialwaitingdelay = Timer();
				while (  Timer() - t.tempsteamingameinitialwaitingdelay < 20000 ) 
				{
					g.mp.syncedWithServerMode = 0;
					g.mp.okayToLoadLevel = 0;
					mp_textDots(-1,50,3,"Waiting for other players");
					SteamLoop (  );
					if (  Timer() - t.tsteamtimeoutongamerunning > 16000 ) 
					{
						if (  SteamGetClientServerConnectionStatus()  ==  0 ) 
						{
							t.tsteamlostconnectioncustommessage_s = "Lost connection to host (Error MP009)";
							mp_lostConnection ( );
							return;
						}
					}
					t.tskipLevelSync = Timer();
					if (  SteamIsEveryoneLoadedAndReady()  ==  1  )  t.tempsteamingameinitialwaitingdelay  =  -30000;
				}   
			}

			//wait for everyone before starting to load, at this GetPoint (  they have all the files they need, they just have not loaded them )
			if (  g.mp.okayToLoadLevel  ==  0 && g.mp.syncedWithServerMode  ==  99 ) 
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
					t.tskipLevelSync = Timer();
			}
			else
			{
				if (  g.mp.playGame  ==  1 ) 
				{
					if (  t.game.titleloop == 1 ) 
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
				if (  g.mp.okayToLoadLevel  ==  0 ) 
				{
					mp_pre_game_file_sync_client ( );
				}
			}

	}

	mp_message ( );
	mp_messageDots ( );
}

void mp_free ( void )
{
	//PhotonFree();
	SteamFree();
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

return;

}

void mp_lua ( void )
{

	while (  SteamGetLuaList() ) 
	{
		t.steamLuaCode = SteamGetLuaCommand();
		t.e = SteamGetLuaE();
		t.v = SteamGetLuaV();
	
		t.tLuaDontSendLua = 1;
	
		switch (  t.steamLuaCode ) 
		{
		case MP_LUA_SetActivated:
			if ( mp_check_if_lua_entity_exists(t.e) == 1 ) entity_lua_setactivated() ; ++t.activatedCount;
		break;
		case MP_LUA_SetAnimation:
			entity_lua_setanimation() ; ++t.animCount;
		break;
		case MP_LUA_PlayAnimation:
			if ( mp_check_if_lua_entity_exists(t.e) == 1 ) entity_lua_playanimation() ; ++t.playanimCount;
		break;
		case MP_LUA_ActivateIfUsed:
			if ( mp_check_if_lua_entity_exists(t.e) == 1 ) entity_lua_activateifused() ; ++t.activateCount;
		break;
		case MP_LUA_PlaySound:
			entity_lua_playsound ( );
		break;
		case MP_LUA_StartTimer:
			entity_lua_starttimer ( );
		break;
		case MP_LUA_CollisionOff:
			entity_lua_collisionoff ( );
		break;
		case MP_LUA_CollisionOn:
			entity_lua_collisionon ( );
		break;
		case MP_LUA_ServerSetLuaGameMode:
			LuaSetInt (  "mp_gameMode",t.v );
		break;
		case MP_LUA_ServerSetPlayerKills:
			t.tnothing = LuaExecute( cstr(cstr("mp_playerKills[") + Str(t.e) + "] = " + Str(t.v)).Get() );
		break;
		case MP_LUA_ServerSetPlayerDeaths:
			t.tnothing = LuaExecute( cstr(cstr("mp_playerDeaths[") + Str(t.e) + "] = " + Str(t.v)).Get() );
		break;
		case MP_LUA_ServerSetPlayerAddKill:
			t.mp_kills[t.v] = t.mp_kills[t.v] + 1;
			SteamSendLua (  MP_LUA_ServerSetPlayerKills,t.v,t.mp_kills[t.v] );
			t.tnothing = LuaExecute( cstr(cstr("mp_playerKills[") + Str(t.v) + "] = " + Str(t.mp_kills[t.v])).Get() );
		break;
		case MP_LUA_ServerSetPlayerRemoveKill:
			//  check if they already have the kills needed to win
			//  because they may kill someone else first, then themselves, which has already triggered a win
			//  so we only remove a kill if they havent yet won
			if (  g.mp.setserverkillstowin  <= 0  )  g.mp.setserverkillstowin  =  100;
			if (  t.mp_kills[t.v] < g.mp.setserverkillstowin ) 
			{
				t.mp_kills[t.v] = t.mp_kills[t.v] - 1;
				SteamSendLua (  MP_LUA_ServerSetPlayerKills,t.v,t.mp_kills[t.v] );
				t.tnothing = LuaExecute( cstr(cstr("mp_playerKills[") + Str(t.v) + "] = " + Str(t.mp_kills[t.v])).Get() );
			}
		break;
		case MP_LUA_ServerSetPlayerAddDeath:
			t.mp_deaths[t.v] = t.mp_deaths[t.v] + 1;
			SteamSendLua (  MP_LUA_ServerSetPlayerDeaths,t.v,t.mp_deaths[t.v] );
			t.tnothing = LuaExecute( cstr(cstr("mp_playerDeaths[") + Str(t.v) + "] = " + Str(t.mp_deaths[t.v])).Get() );
		break;
		case MP_LUA_SetServerTimer:
			t.tnothing = LuaExecute( cstr(cstr("mp_servertimer = ") + Str(t.v)).Get() );
		break;
		case MP_LUA_ServerRespawnAll:
			mp_restoreEntities ( );
			mp_setLuaResetStats ( );
			mp_respawnEntities ( );
			t.playercontrol.jetpackhidden=0;
			t.playercontrol.jetpackmode=0;
			physics_no_gun_zoom ( );
			t.aisystem.processplayerlogic=1;
			g.mp.gameAlreadySpawnedBefore = 0;
			t.mp_playerHasSpawned[g.mp.me] = 0;
			if (  g.mp.myOriginalSpawnPoint  !=  -1 ) 
			{
				t.tindex = g.mp.myOriginalSpawnPoint;
			}
			else
			{
				t.tindex = 1;
			}
			if (  t.mpmultiplayerstart[t.tindex].active == 1 ) 
			{
				t.terrain.playerx_f=t.mpmultiplayerstart[t.tindex].x;
				t.terrain.playery_f=t.mpmultiplayerstart[t.tindex].y;
				t.terrain.playerz_f=t.mpmultiplayerstart[t.tindex].z;
				t.terrain.playerax_f=0;
				t.terrain.playeray_f=t.mpmultiplayerstart[t.tindex].angle;
				t.terrain.playeraz_f=0;

				g.mp.lastx=t.terrain.playerx_f;
				g.mp.lasty=t.terrain.playery_f;
				g.mp.lastz=t.terrain.playerz_f;
				g.mp.lastangley=t.terrain.playeray_f;

				t.tobj = t.entityelement[t.mp_playerEntityID[g.mp.me]].obj;
				if (  t.tobj > 0 ) 
				{
					PositionObject (  t.tobj, t.terrain.playerx_f, t.terrain.playery_f-50, t.terrain.playerz_f );
					RotateObject (  t.tobj, t.terrain.playerax_f, t.terrain.playeray_f, t.terrain.playeraz_f );
				}
			}

			g.autoloadgun=0  ; gun_change ( );
			g.mp.endplay = 0;
			t.player[t.plrid].health = 0;
			t.mp_health[g.mp.me] = 0;
			physics_resetplayer_core ( );
		break;
		case MP_LUA_ServerEndPlay:
				t.playercontrol.jetpackhidden=0;
				t.playercontrol.jetpackmode=0;
				physics_no_gun_zoom ( );
				t.aisystem.processplayerlogic=0;
				g.mp.endplay = 1;
				g.autoloadgun=0 ; gun_change ( );
		break;
		case MP_LUA_AiGoToX:
			t.tSteamX_f = t.v;
		break;
		case MP_LUA_AiGoToZ:
			t.tSteamZ_f = t.v;
			if (  t.e > 0 ) 
			{
				if (  ObjectExist(t.e)  ==  1 ) 
				{
					AISetEntityActive (  t.e,1 );
					mp_COOP_aiMoveTo ( );
				}
			}
			for ( t.tee = 1 ; t.tee<=  g.entityelementlist; t.tee++ )
			{
				if (  t.entityelement[t.tee].obj  ==  t.e ) 
				{
					t.entityelement[t.tee].mp_updateOn = 1;
					t.entityelement[t.tee].active = 1;
//      `print "updating object " + Str(tee)

					break;
				}
			}
		break;
		case MP_LUA_setcharactertowalkrun:
			entity_lua_setcharactertowalkrun ( );
			if (  t.entityelement[t.e].obj > 0 ) 
			{
				if (  ObjectExist(t.entityelement[t.e].obj)  ==  1 ) 
				{
					t.entityelement[t.e].mp_updateOn = 1;
					t.entityelement[t.e].active = 1;
				}
			}
		break;
		case MP_LUA_CharacterControlManual:
			entity_lua_charactercontrolmanual ( );
			t.entityelement[t.e].mp_updateOn = 1;
			t.entityelement[t.e].active = 1;
		break;
		case MP_LUA_CharacterControlLimbo:
			entity_lua_charactercontrollimbo ( );
			t.entityelement[t.e].mp_updateOn = 1;
			t.entityelement[t.e].active = 1;
		break;
		case MP_LUA_CharacterControlArmed:
			entity_lua_charactercontrolarmed ( );
			t.entityelement[t.e].mp_updateOn = 1;
			t.entityelement[t.e].active = 1;
			AISetEntityActive (  t.entityelement[t.e].obj,1 );
		break;
		case MP_LUA_CharacterControlUnarmed:
			entity_lua_charactercontrolunarmed ( );
			t.entityelement[t.e].mp_updateOn = 1;
			t.entityelement[t.e].active = 1;
		break;
		case MP_LUA_LookAtPlayer:
			if (  t.entityelement[t.e].obj > 0 ) 
			{
				if (  ObjectExist(t.entityelement[t.e].obj)  ==  1 ) 
				{
					AISetEntityActive (  t.entityelement[t.e].obj,1 );
					mp_entity_lua_lookatplayer ( );
					t.entityelement[t.e].mp_updateOn = 1;
					t.entityelement[t.e].active = 1;
					t.entityelement[t.e].mp_rotateTimer = Timer();
					t.entityelement[t.e].mp_rotateType = 1;
				}
			}
		break;
		case MP_LUA_TakenAggro:
			if (  t.entityelement[t.e].obj > 0 ) 
			{
				AISetEntityActive (  t.entityelement[t.e].obj,1 );
				if (  ObjectExist(t.entityelement[t.e].obj)  ==  1 ) 
				{
//      `if mp.me  ==  0 then SteamSendLua (  MP_LUA_TakenAggro,e,v )

					t.entityelement[t.e].mp_coopControlledByPlayer = t.v;
					t.entityelement[t.e].mp_coopLastTimeSwitchedTarget = Timer();
//      `if v  ==  mp.me then inc entityelement(e).mp_coopLastTimeSwitchedTarget,5000
					
					t.entityelement[t.e].mp_updateOn = 1;
					t.entityelement[t.e].active = 1;
					//  set them to run - not totally ideal for zombies (some walk) but okay for now
					t.v = 1;
					entity_lua_setcharactertowalkrun ( );
//      `AI Entity Stop entityelement(e).obj

				}
			}
		break;
		case MP_LUA_HaveAggro:
			if (  t.entityelement[t.e].obj > 0 ) 
			{
				AISetEntityActive (  t.entityelement[t.e].obj,1 );
				if (  ObjectExist(t.entityelement[t.e].obj)  ==  1 ) 
				{
//      `if mp.me  ==  0 then SteamSendLua (  MP_LUA_TakenAggro,e,v )
					
					t.entityelement[t.e].mp_coopControlledByPlayer = t.v;
					t.entityelement[t.e].mp_updateOn = 1;
					t.entityelement[t.e].active = 1;
					//  set them to run - not totally ideal for zombies (some walk) but okay for now
					t.v = 1;
					entity_lua_setcharactertowalkrun ( );
//      `AI Entity Stop entityelement(e).obj

				}
			}
		break;
		case MP_LUA_FireWeaponEffectOnly:
			if (  t.entityelement[t.e].obj > 0 ) 
			{
				if (  ObjectExist(t.entityelement[t.e].obj)  ==  1 ) 
				{
					mp_entity_lua_fireweaponEffectOnly ( );
					t.entityelement[t.e].mp_updateOn = 1;
					t.entityelement[t.e].active = 1;
					AISetEntityActive (  t.entityelement[t.e].obj,1 );
				}
			}
		break;
		case MP_LUA_RotateToPlayer:
			if (  t.entityelement[t.e].obj > 0 ) 
			{
				if (  ObjectExist(t.entityelement[t.e].obj)  ==  1 ) 
				{
					mp_coop_rotatetoplayer ( );
					t.entityelement[t.e].mp_updateOn = 1;
					t.entityelement[t.e].active = 1;
					t.entityelement[t.e].mp_rotateTimer = Timer();
					t.entityelement[t.e].mp_rotateType = 2;
					AISetEntityActive (  t.entityelement[t.e].obj,1 );
				}
			}
		break;
		case MP_LUA_SetAnimationFrames:
			entity_lua_setanimationframes ( );
		break;
		case MP_LUA_AISetEntityControl:
			AISetEntityControl (  t.e,t.v );
		break;
		case MP_LUA_AIMoveX:
			t.tsteamPosX = t.v;
		break;
		case MP_LUA_AIMoveZ:
			AISetEntityPosition (  t.e, t.tsteamPosX, BT_GetGroundHeight(t.terrain.TerrainID,t.tsteamPosX,t.v),t.v );
		break;
		case MP_LUA_SendAvatar:
			t.tsteams_s = SteamGetLuaS();
			t.mp_playerAvatars_s[t.e] = t.tsteams_s;
		break;
		case MP_LUA_SendAvatarName:
			t.tsteams_s = SteamGetLuaS();
			t.mp_playerAvatarOwners_s[t.e] = t.tsteams_s;
		break;
		}	//~   
	
		t.tLuaDontSendLua = 0;
	
		SteamGetNextLua (  );
	}

return;

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

void mp_pre_game_file_sync ( void )
{
	if (  g.mp.isGameHost  ==  1 ) 
	{
		mp_pre_game_file_sync_server ( 0 );
	}
	else
	{
		mp_pre_game_file_sync_client ( );
	}
}

void mp_pre_game_file_sync_server ( int iOnlySendMapToSpecificPlayer )
{
//  if we have lost connection, head back to main menu
t.tconnectionStatus = SteamGetClientServerConnectionStatus();
if (  t.tconnectionStatus  ==  0 ) 
{
	t.tsteamconnectionlostmessage_s = "Lost Connection";
	mp_lostConnection ( );
	return;
}

mp_sendAvatarInfo ( );
//  check if we have finished sending and receiving textures with the server
//  (the actual process is handled by steam dll)
if (  g.mp.isGameHost  ==  0 || g.mp.me  !=  0  )  return;
if (  SteamCheckSyncedAvatarTexturesWithServer()  ==  0 ) 
{
	t.tstring_s = "Syncing Avatars";
	mp_textDots(-1,50,3,t.tstring_s.Get());
	return;
}

	switch (  g.mp.syncedWithServerMode ) 
	{

		case 0:
			//  for solo testing to prevent sending files
			if (  g.mp.usersInServersLobbyAtServerCreation  ==  1 ) 
			{
				g.mp.syncedWithServerMode = 3;
				return;
			}
	
			g.mp.serverusingworkshop = 0;
	
			SteamSetSendFileCount (  1 );
			if (  g.mp.levelContainsCustomContent  ==  0 ) 
			{
				SteamSendFileBegin (  1,"__multiplayerlevel__.fpm" );
				g.mp.serverusingworkshop = 1;
			}
			else
			{
				t.tempsteamfiletosend_s = g.mysystem.editorsGrideditAbs_s+"__multiplayerworkshopitemid__.dat";//g.fpscrootdir_s+"\\Files\\editors\\gridedit\\__multiplayerworkshopitemid__.dat";
				if (  FileExist (t.tempsteamfiletosend_s.Get())  ==  1  )  DeleteAFile (  t.tempsteamfiletosend_s.Get() );
				if (  FileOpen(1)  )  CloseFile (  1 );
				OpenToWrite (  1,t.tempsteamfiletosend_s.Get() );
				WriteString (  1,g.mp.workshopid.Get() );
				CloseFile (  1 );
				SteamSendFileBegin (  1,"__multiplayerworkshopitemid__.dat" );
			}
			g.mp.syncedWithServerMode = 1;
			mp_textDots(-1,30,3,"Setting up data for clients")  ;    

		break;

		case 1:
			if (  SteamSendFileDone()  ==  1 ) 
			{
					g.mp.syncedWithServerMode = 2;
			}
				mp_textDots(-1,50,3,"Sending data to clients");
		break;

		case 2:
			mp_textDots(-1,30,3,"Waiting for clients to receive data");

			if (  SteamIsEveryoneFileSynced()  ==  1 ) 
			{
			//the client hosting the server needs to have loaded everything in also
				SteamSendIAmLoadedAndReady (  );
				g.mp.syncedWithServerMode = 3;
				g.mp.oldtime = Timer();
			}
		break;

	case 3:
		if (  SteamIsEveryoneLoadedAndReady()  ==  1 ) 
		{
			if (  g.mp.serverusingworkshop  ==  1 ) 
			{
					mp_textDots(-1,30,3,"Waiting for clients to receive data");
					if (  Timer() - g.mp.oldtime > 3000 ) 
					{
						g.mp.oldtime = Timer();
						g.mp.syncedWithServer = 1;
						g.mp.syncedWithServerMode = 99;
						SetDir (  t.toldsteamfolder_s.Get() );
						SetDir (  g.mp.originalpath.Get() );
					}
				}
				else
				{
					g.mp.oldtime = Timer();
					g.mp.syncedWithServer = 1;
					g.mp.syncedWithServerMode = 99;
					SetDir (  t.toldsteamfolder_s.Get() );
					SetDir (  g.mp.originalpath.Get() );
				}
			}
			else
			{
				if (  Timer() - g.mp.oldtime > 150 ) 
				{
					g.mp.oldtime = Timer();
					t.tSteamBuildingWorkshopItem_s = t.tSteamBuildingWorkshopItem_s + ".";
					if (  Len(t.tSteamBuildingWorkshopItem_s.Get()) > 5  )  t.tSteamBuildingWorkshopItem_s  =  ".";
				}
				if (  Timer() - t.tempMPsendingready > 2000 ) 
				{
					SteamSendIAmLoadedAndReady (  );
					t.tempMPsendingready = Timer();
				}
				t.tstring_s = t.tSteamBuildingWorkshopItem_s + "Waiting for everyone to be ready" + t.tSteamBuildingWorkshopItem_s;
				mp_text(-1,50,3,t.tstring_s.Get());
				t.tstring_s = "";
			}
	break;
	}//~ ` 

return;

}

void mp_pre_game_file_sync_client ( void )
{
	//  if we have lost connection, head back to main menu
	t.tconnectionStatus = SteamGetClientServerConnectionStatus();
	if (  t.tconnectionStatus  ==  0 ) 
	{
		t.tsteamconnectionlostmessage_s = "Lost Connection";
		mp_lostConnection ( );
		return;
	}

	mp_sendAvatarInfo ( );
	//  check if we have finished sending and receiving textures with the server
	//  (the actual process is handled by steam dll)
	if (  g.mp.isGameHost  ==  1 || g.mp.me  ==  0  )  return;
	if (  SteamCheckSyncedAvatarTexturesWithServer()  ==  0 ) 
	{
		t.tstring_s = "Syncing Avatars";
		mp_textDots(-1,50,3,t.tstring_s.Get());
		return;
	}

	if ( SteamGetClientServerConnectionStatus()  ==  0 ) 
	{
		t.tsteamlostconnectioncustommessage_s = "Lost connect to server (Error MP010)";
		g.mp.backtoeditorforyou = 0;
		g.mp.mode = 0;
		mp_lostConnection ( );
		return;
	}

	switch (  g.mp.syncedWithServerMode ) 
	{
		case 0:
			if (  SteamAmIFileSynced()  ==  1 ) 
			{
				t.tempMPshopidfile_s = g.mysystem.editorsGrideditAbs_s+"__multiplayerworkshopitemid__.dat";//g.fpscrootdir_s+"\\Files\\editors\\gridedit\\__multiplayerworkshopitemid__.dat";
				if (  FileExist(t.tempMPshopidfile_s.Get()) ) 
				{
					if (  FileOpen(10)  ==  1  )  CloseFile (  10 );
					OpenToRead (  10,t.tempMPshopidfile_s.Get() );
					g.mp.workshopid = ReadString ( 10 );
					CloseFile (  10 );
					cstr mlevel_s = g.mysystem.editorsGrideditAbs_s + "__multiplayerlevel__.fpm";
					if (  FileExist( mlevel_s.Get() ) )  DeleteAFile ( mlevel_s.Get() );
					SteamDownloadWorkshopItem (  g.mp.workshopid.Get() );
					g.mp.syncedWithServerMode = 2;
				}
				else
				{
					g.mp.fileLoaded = 1;
					SteamSendIAmLoadedAndReady (  );
					g.mp.syncedWithServerMode = 1;
				}
			}
			else
			{
				t.tProgress = SteamGetFileProgress();
				t.tstring_s = cstr("Receiving '")+g.mp.levelnametojoin+"': " + Str(t.tProgress) + "%";
				mp_text(-1,85,3,t.tstring_s.Get());
			}
		break;

		case 1:
			if (  SteamIsEveryoneLoadedAndReady()  ==  1 ) 
			{
				g.mp.syncedWithServer = 1;
				g.mp.syncedWithServerMode = 99;
				SetDir (  t.toldsteamfolder_s.Get() );
				SetDir (  g.mp.originalpath.Get() );
			}
			else
			{
				if (  Timer() - g.mp.oldtime > 150 ) 
				{
					g.mp.oldtime = Timer();
					t.tSteamBuildingWorkshopItem_s = t.tSteamBuildingWorkshopItem_s + ".";
					if (  Len(t.tSteamBuildingWorkshopItem_s.Get()) > 5  )  t.tSteamBuildingWorkshopItem_s  =  ".";
				}
				if (  Timer() - t.tempMPsendingready > 2000 ) 
				{
					SteamSendIAmLoadedAndReady (  );
					t.tempMPsendingready = Timer();
				}
				t.tstring_s = t.tSteamBuildingWorkshopItem_s + "Waiting for everyone to be ready" + t.tSteamBuildingWorkshopItem_s;
				mp_text(-1,50,3,t.tstring_s.Get());
				t.tstring_s = "";
			}
		break;

		case 2:
			if (  SteamIsWorkshopItemDownloaded()  ==  -1 ) 
			{
				t.tsteamconnectionlostmessage_s = "Unable to join, Steam does not yet have all the files needed (Error MP011)";
				mp_lostConnection ( );
				return;
			}
			if (  SteamIsWorkshopItemDownloaded()  ==  1 ) 
			{
				cstr mlevel_s = g.mysystem.editorsGrideditAbs_s + "__multiplayerlevel__.fpm";
				if ( FileExist( mlevel_s.Get() ) ) 
				{
					g.mp.fileLoaded = 1;
					SteamSendIAmLoadedAndReady (  );
					g.mp.syncedWithServerMode = 1;
				}
				else
				{
					t.tsteamconnectionlostmessage_s = "Unable to join, Steam does not yet have all the files needed (Error MP012)";
					mp_lostConnection ( );
					return;
				}
			}
			else
			{
				if (  Timer() - g.mp.oldtime > 150 ) 
				{
					g.mp.oldtime = Timer();
					t.tSteamBuildingWorkshopItem_s = t.tSteamBuildingWorkshopItem_s + ".";
					if (  Len(t.tSteamBuildingWorkshopItem_s.Get()) > 5  )  t.tSteamBuildingWorkshopItem_s  =  ".";
				}
				t.tstring_s = t.tSteamBuildingWorkshopItem_s + "Downloading Workshop Item" + t.tSteamBuildingWorkshopItem_s;
				mp_text(-1,50,3,t.tstring_s.Get());
				t.tstring_s = "";
			}
		break;

	} 
	return;
}

void mp_sendAvatarInfo ( void )
{
		if (  g.mp.haveSentMyAvatar  ==  0 ) 
		{
			g.mp.me = SteamGetMyPlayerIndex();
			if (  g.mp.isGameHost  ==  1 || g.mp.me  !=  0 ) 
			{
				g.mp.haveSentMyAvatar = 1;
				SteamSendLuaString (  MP_LUA_SendAvatarName,g.mp.me,SteamGetPlayerName() );
				SteamSendLuaString (  MP_LUA_SendAvatar,g.mp.me,g.mp.myAvatar_s.Get() );

				//  store our own info for loading in our avatar
				t.mp_playerAvatarOwners_s[g.mp.me] = SteamGetPlayerName();
				t.mp_playerAvatars_s[g.mp.me] = g.mp.myAvatar_s;
				//  send out custom texture (mp.myAvatarHeadTexture$ will be "" if we don't have one)
				SteamSetMyAvatarHeadTextureName (  g.mp.myAvatarHeadTexture_s.Get() );
			}
		}
//  `endif

	mp_lua ( );
return;

}

void mp_animation ( void )
{

	while (  SteamGetAnimationList() ) 
	{
		t.tEnt = SteamGetAnimationIndex();
		t.astart = SteamGetAnimationStart();
		t.aend = SteamGetAnimationEnd();
		t.aspeed = SteamGetAnimationSpeed();
	
		SetObjectSpeed (  t.entityelement[t.tEnt].obj,t.aspeed );
		PlayObject (  t.entityelement[t.tEnt].obj,t.astart,t.aend );

		SteamGetNextAnimation (  );
	}

return;
//  Send our position and angle to steam
}

void mp_update_player ( void )
{
if (  g.mp.endplay  ==  1  )  return;

// once we are alive, no immunity
t.huddamage.immunity = 1000;
if (  Timer() - g.mp.invincibleTimer > 6000 ) 
{
	t.huddamage.immunity = 0;
}
else
{
	t.huddamage.immunity = 1000;
	t.tthrowawaythisdamage = SteamGetPlayerDamageAmount();
}
//  check if we have taken damage
t.tdamage = SteamGetPlayerDamageAmount();
// `if player(plrid).health > 100 then player(plrid).health  ==  100

if (  t.tdamage > 0 ) 
{

	t.tsteamlastdamageincounter = t.tsteamlastdamageincounter + 1;
	//  Receives; tdamage, te, tDrownDamageFlag
	t.te = t.mp_playerEntityID[SteamGetPlayerDamageSource()];
	t.tDrownDamageFlag = 0;
	physics_player_takedamage ( );

	if (  t.player[t.plrid].health  <=  0 ) 
	{
		g.mp.killedByPlayerFlag = 1;
		g.mp.playerThatKilledMe = SteamGetPlayerDamageSource();
		t.tsteamforce = SteamGetPlayerDamageForce();
		SteamKilledBy (  g.mp.playerThatKilledMe , SteamGetPlayerDamageX(), SteamGetPlayerDamageY(), SteamGetPlayerDamageZ(), t.tsteamforce, SteamGetPlayerDamageLimb() );


		g.mp.dyingTime = Timer();
	}
}

t.mp_health[g.mp.me] = t.player[t.plrid].health;

	//  check if we have changed guns
	if (  g.mp.gunid  !=  t.gunid ) 
	{
		//  send a server message saying we have a new gun
		t.tfound = 0;
		for ( t.ti = 0 ; t.ti<=  g.mp.gunCount; t.ti++ )
		{
			if (  t.mp_gunname[t.ti]  ==  Lower(t.gun[t.gunid].name_s.Get()) ) 
			{
				t.tfound = t.ti+1;
			}
		}
		if (  t.tfound>0 ) 
		{
			t.hasgunname_s=t.gun[t.gunid].name_s;
			t.steamhasgunname_s=t.mp_gunname[t.tfound-1];
			g.mp.appearance = t.tfound;
			t.toldappearancevariable = t.tfound;
//    `mp.gunid = gunid

		}
		else
		{
			g.mp.appearance = 0;
		}
		g.mp.gunid = t.gunid;
	}

	//  Send our positional data to the server
//  `t.tTime = Timer()

//  `if t.tTime - mp.lastSendPositionTime < MP_POSITION_UPDATE_DELAY then return
	
	
//  `mp.lastSendPositionTime = t.tTime

	
	SteamSetPlayerPositionX (  CameraPositionX() );
	g.mp.lastx = CameraPositionX();

		if (  g.mp.crouchOn  ==  0 ) 
		{
			SteamSetPlayerPositionY (  CameraPositionY()-64 );
			g.mp.lasty = CameraPositionY()-64;
		}
		else
		{
			SteamSetPlayerPositionY (  CameraPositionY()-64+30 );
			g.mp.lasty = CameraPositionY()-64+30;
		}

	SteamSetPlayerPositionZ (  CameraPositionZ() );
	g.mp.lastz = CameraPositionZ();

	SteamSetPlayerAngle (  CameraAngleY() );
	g.mp.lastangley = CameraAngleY();

	t.tpe = t.mp_playerEntityID[g.mp.me];
	t.entityelement[t.tpe].x=g.mp.lastx;
	t.entityelement[t.tpe].y=g.mp.lasty;
	t.entityelement[t.tpe].z=g.mp.lastz;
	if (  t.entityelement[t.mp_playerEntityID[g.mp.me]].obj > 0 ) 
	{
		if (  ObjectExist(t.entityelement[t.mp_playerEntityID[g.mp.me]].obj) ) 
		{
			PositionObject (  t.entityelement[t.mp_playerEntityID[g.mp.me]].obj, g.mp.lastx, g.mp.lasty+10, g.mp.lastz );
		}
	}
	t.te = t.tpe;
	t.tolde = t.e;
	t.e = t.tpe;
	entity_updatepos ( );
	entity_lua_rotateupdate ( );
	t.e = t.tolde;

return;

}

void mp_updatePlayerPositions ( void )
{

	if (  g.mp.endplay  ==  1  )  return;

	//  Get player data from the server
	for ( t.c = 0 ; t.c<=  MP_MAX_NUMBER_OF_PLAYERS-1; t.c++ )
	{

		if (  t.mp_forcePosition[t.c] > 0 && SteamGetPlayerAlive(t.c)  ==  1 ) 
		{
			if (  t.mp_forcePosition[t.c]  ==  1  )  t.mp_forcePosition[t.c]  =  Timer();
			if (  Timer() - t.mp_forcePosition[t.c] > 1000 ) 
			{
				t.mp_forcePosition[t.c] = 0;
				t.x_f = SteamGetPlayerPositionX(t.c);
				t.y_f = SteamGetPlayerPositionY(t.c);
				t.z_f = SteamGetPlayerPositionZ(t.c);
				SteamSetTweening (  t.c,1 );
			}
			else
			{
				SteamSetTweening (  t.c,0 );
			}

			t.x_f = SteamGetPlayerPositionX(t.c);
			t.y_f = SteamGetPlayerPositionY(t.c);
			t.z_f = SteamGetPlayerPositionZ(t.c);
			t.angle_f = SteamGetPlayerAngle(t.c);

		}
		//  Get other players tweened positional data
		t.x_f = SteamGetPlayerPositionX(t.c);
		t.y_f = SteamGetPlayerPositionY(t.c);
		t.z_f = SteamGetPlayerPositionZ(t.c);
		t.angle_f = SteamGetPlayerAngle(t.c);
		if (  t.c  !=  g.mp.me ) 
		{
			if (  SteamGetPlayerAlive(t.c)  ==  1 && t.mp_forcePosition[t.c]  ==  0 ) 
			{
				t.e = t.mp_playerEntityID[t.c];
				t.entityelement[t.e].x=t.x_f;
				t.entityelement[t.e].y=t.y_f;
				t.entityelement[t.e].z=t.z_f;
				t.entityelement[t.e].ry=t.angle_f;
				PositionObject (  t.entityelement[t.e].obj, t.entityelement[t.e].x, t.entityelement[t.e].y, t.entityelement[t.e].z );
				t.te = t.e;
				entity_updatepos ( );
				entity_lua_rotateupdate ( );
			}
		}
	}

return;

//  Display message from server
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
	t.tsteamdisplaymessagetimer = Timer();
	t.s_s = Upper(t.s_s.Get());
}
if (  t.s_s  ==  ""  )  t.s_s  =  g.mp.previousMessage_s;
g.mp.previousMessage_s = t.s_s;
if (  Timer() - t.tsteamdisplaymessagetimer < 2000  )  mp_text(-1,10,3,t.s_s.Get());
// `text GetDisplayWidth()/2 - Text (  width(s$)/2, 100, s$ )


return;

}

void mp_updatePlayerNamePlates ( void )
{

//  `if mp.endplay  ==  1 then return
	

	if (  g.mp.nameplatesOff  ==  1 ) 
	{
		for ( t.c = 0 ; t.c<=  MP_MAX_NUMBER_OF_PLAYERS-1; t.c++ )
		{
			if (  ObjectExist(g.steamplayermodelsoffset+500+t.c) ) 
			{
				PositionObject (  g.steamplayermodelsoffset+500+t.c,500000,-500000,500000 );
			}
		}
		return;
	}
	//  Display players names and stats
	for ( t.c = 0 ; t.c<=  MP_MAX_NUMBER_OF_PLAYERS-1; t.c++ )
	{
			//  if it isnt me, display their details above their head
			if (  g.mp.sentmyname  ==  1 ) 
			{
				if (  ObjectExist(g.steamplayermodelsoffset+500+t.c)  ==  1  )  DeleteObject (  g.steamplayermodelsoffset+500+t.c );
			}
			if (  ObjectExist(g.steamplayermodelsoffset+500+t.c) ) 
			{
				PositionObject (  g.steamplayermodelsoffset+500+t.c,500000,-500000,500000 );
			}

			if (  t.entityelement[t.mp_playerEntityID[t.c]].obj > 0 ) 
			{
				if (  ObjectExist(t.entityelement[t.mp_playerEntityID[t.c]].obj)  ==  1 ) 
				{
					if (  t.c  !=  g.mp.me ) 
					{
						if (  t.mp_forcePosition[t.c]  ==  0 && SteamGetPlayerAlive(t.c)  ==  1 ) 
						{
							if (  GetInScreen(t.entityelement[t.mp_playerEntityID[t.c]].obj) ) 
							{
								t.tname_s = SteamGetOtherPlayerName(t.c);
								if (  t.tname_s != "Player" ) 
								{
									t.tobj = t.entityelement[t.mp_playerEntityID[t.c]].obj;
									if (  ObjectExist(g.steamplayermodelsoffset+500+t.c)  ==  0 ) 
									{
										t.tResult = MakeNewObjectPanel(g.steamplayermodelsoffset+500+t.c,Len(t.tname_s.Get()));
										if (  t.tResult ) 
										{
											t.index = 3;
											t.twidth=0;
											for ( t.n = 1 ; t.n<=  Len(t.tname_s.Get()); t.n++ )
											{
												t.charindex=Asc(Mid(t.tname_s.Get(),t.n));
												t.twidth += t.bitmapfont[t.index][t.charindex].w;
											}
											t.tx = -(t.twidth/2.0);

											t.timg = g.bitmapfontimagetart+t.index;
											for ( t.n = 1 ; t.n<=  Len(t.tname_s.Get()); t.n++ )
											{
												t.charindex=Asc(Mid(t.tname_s.Get(),t.n));
												t.u1_f=t.bitmapfont[t.index][t.charindex].x1;
												t.v1_f=t.bitmapfont[t.index][t.charindex].y1;
												t.u2_f=t.bitmapfont[t.index][t.charindex].x2;
												t.v2_f=t.bitmapfont[t.index][t.charindex].y2;
												t.r = 255;
												t.g = 50;
												t.b = 50;
												if (  g.mp.team  ==  1 ) 
												{
													if (  t.mp_team[t.c]  ==  t.mp_team[g.mp.me] ) 
													{
														t.r = 100;
														t.g = 255;
														t.b = 100;
													}
												}
												SetObjectPanelQuad (  g.steamplayermodelsoffset+500+t.c,t.n-1,t.tx,0,t.bitmapfont[t.index][t.charindex].w,t.bitmapfont[t.index][t.charindex].h,t.u1_f,t.v1_f,t.u2_f,t.v2_f,t.r,t.g,t.b );
												t.tx += t.bitmapfont[t.index][t.charindex].w;
											}
											FinishObjectPanel (  g.steamplayermodelsoffset+500+t.c,32,10 );

											SetCharacterCreatorTones (  g.steamplayermodelsoffset+500+t.c,0,t.r,t.g,t.b,1.0 );
											SetObjectLight (  g.steamplayermodelsoffset+500+t.c,0 );
											YRotateObject (  g.steamplayermodelsoffset+500+t.c,180 );
											FixObjectPivot (  g.steamplayermodelsoffset+500+t.c );
											SetObjectTransparency (  g.steamplayermodelsoffset+500+t.c, 6 );
											ScaleObject (  g.steamplayermodelsoffset+500+t.c,60,60,100 );
											SetSphereRadius (  g.steamplayermodelsoffset+500+t.c,0 );
											SetObjectMask (  g.steamplayermodelsoffset+500+t.c, 1 );
											//  apply special overlay_basic shader which also handles depth render for DOF avoidance
											t.teffectid=loadinternaleffect("effectbank\\reloaded\\overlay_basic.fx");
											TextureObject (  g.steamplayermodelsoffset+500+t.c,t.timg );
											SetObjectEffect (  g.steamplayermodelsoffset+500+t.c,t.teffectid );
										}
									}
									else
									{
//          `show it

										if (  SteamGetPlayerAlive(t.c)  ==  1 && g.mp.endplay  ==  0 ) 
										{
											t.tnameplatey_f = ObjectPositionY(t.tobj)+ ObjectSizeY(t.tobj,1);
											if (  t.mp_playerAvatars_s[t.c]  !=  ""  )  t.tnameplatey_f  =  t.tnameplatey_f + 15.0;
											ShowObject (  g.steamplayermodelsoffset+500+t.c );
											PositionObject((g.steamplayermodelsoffset+500+t.c), ObjectPositionX(t.tobj), t.tnameplatey_f , ObjectPositionZ(t.tobj));
											PointObject (  g.steamplayermodelsoffset+500+t.c,CameraPositionX(), CameraPositionY(), CameraPositionZ() );
										}
										else
										{
											HideObject (  g.steamplayermodelsoffset+500+t.c );
										}
									}
								}
							}
						}
						else
						{
							if (  ObjectExist(g.steamplayermodelsoffset+500+t.c) ) 
							{
								PositionObject (  g.steamplayermodelsoffset+500+t.c,500000,-500000,500000 );
							}
						}
					}
				}
			}
		}

		g.mp.sentmyname = 0;

return;

}

