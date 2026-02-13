FPSCR int SteamGetLuaV()
{
#ifdef MAKE_MULTIPLAYER_LOG
	debugLog ( "SteamGetLuaV" );
#endif
	return currentLua.v;
}

FPSCR LPSTR SteamGetLuaS(void)
{
#ifdef MAKE_MULTIPLAYER_LOG
	debugLog ( "SteamGetLuaV" );
#endif

	// Delete old string
	//if(pOldString) g_pGlob->CreateDeleteString ( (DWORD*)&pOldString, 0 );

	// Create a new string and copy input string to it
	/*LPSTR pReturnString=NULL;
	DWORD dwSize=strlen( (const char*)currentLua.s );
	g_pGlob->CreateDeleteString ( (DWORD*)&pReturnString, dwSize+1 );
	strcpy(pReturnString, (const char*)currentLua.s);

	return pReturnString;*/
	return GetReturnStringFromTEXTWorkString( currentLua.s );
}


FPSCR int SteamGetSpawnList()
{
#ifdef MAKE_MULTIPLAYER_LOG
	debugLog ( "SteamGetSpawnList" );
#endif
	if ( spawnList.size() > 0 )
	{
		currentSpawnObject.object = spawnList.back().object;
		currentSpawnObject.source = spawnList.back().source;
		currentSpawnObject.x = spawnList.back().x;
		currentSpawnObject.y = spawnList.back().y;
		currentSpawnObject.z = spawnList.back().z;
		return 1;
	}
	return 0;
}

FPSCR void SteamGetNextSpawn()
{
#ifdef MAKE_MULTIPLAYER_LOG
	debugLog ( "SteamGetNextSpawn" );
#endif
	if ( spawnList.size() > 0 )
		spawnList.pop_back();
}

FPSCR int SteamGetSpawnObjectNumber()
{
#ifdef MAKE_MULTIPLAYER_LOG
	debugLog ( "SteamGetSpawnObjectNumber" );
#endif
	return currentSpawnObject.object;
}

FPSCR int SteamGetSpawnObjectSource()
{
	return currentSpawnObject.source;
}

FPSCR float SteamGetSpawnX()
{
	float fValue = currentSpawnObject.x;
	return fValue;
}

FPSCR float SteamGetSpawnY()
{
	float fValue = currentSpawnObject.y;
	return fValue;
}

FPSCR float SteamGetSpawnZ()
{
	float fValue = currentSpawnObject.z;
	return fValue;
}

FPSCR int SteamGetDeleteList()
{
#ifdef MAKE_MULTIPLAYER_LOG
	debugLog ( "SteamGetDeleteList" );
#endif
	if ( deleteList.size() > 0 )
	{
		currentDeleteObject = deleteList.back();
		currentDeleteObjectSource = deleteListSource.back();
		return 1;
	}
	return 0;
}

FPSCR void SteamGetNextDelete()
{
#ifdef MAKE_MULTIPLAYER_LOG
	debugLog ( "SteamGetNextDelete" );
#endif
	if ( deleteList.size() > 0 )
		deleteList.pop_back();

	if ( deleteListSource.size() > 0 )
		deleteListSource.pop_back();
}

FPSCR int SteamGetDeleteObjectNumber()
{
#ifdef MAKE_MULTIPLAYER_LOG
	debugLog ( "SteamGetDeleteObjectNumber" );
#endif
	return currentDeleteObject;
}

FPSCR int SteamGetDeleteSource()
{
	return currentDeleteObjectSource;
}

FPSCR int SteamGetDestroyList()
{
#ifdef MAKE_MULTIPLAYER_LOG
	debugLog ( "SteamGetDestroyList" );
#endif
	if ( destroyList.size() > 0 )
	{
		currentDestroyObject = destroyList.back();
		return 1;
	}
	return 0;
}

FPSCR void SteamGetNextDestroy()
{
#ifdef MAKE_MULTIPLAYER_LOG
	debugLog ( "SteamGetNextDestroy" );
#endif
	if ( destroyList.size() > 0 )
		destroyList.pop_back();
}

FPSCR int SteamGetDestroyObjectNumber()
{
	return currentDestroyObject;
}

FPSCR int SteamReadyToSpawn()
{
	return ServerSaysItIsOkayToStart;
}

FPSCR void SteamSetSendFileCount(int count)
{
	if ( g_SteamRunning )
		Client()->SteamSetSendFileCount(count);
}

FPSCR void SteamSendFileBegin ( int index , LPSTR pString )
{
	if ( g_SteamRunning )
		Client()->SteamSendFileBegin( index, pString);	
}

FPSCR int SteamSendFileDone()
{
	if ( g_SteamRunning )
		return Client()->SteamSendFileDone();

	return 0;
}

FPSCR int SteamAmIFileSynced()
{
	if ( g_SteamRunning )
		return IamSyncedWithServerFiles;

	return 0;
}

FPSCR int SteamIsEveryoneFileSynced()
{
	if ( g_SteamRunning )
		return Client()->SteamIsEveryoneFileSynced();

	return 0;
}

FPSCR void SteamSendIAmLoadedAndReady()
{
	if ( g_SteamRunning )
	{
		Client()->SteamSendIAmLoadedAndReady();
		serverActive = true;
	}
}

FPSCR int SteamIsEveryoneLoadedAndReady()
{
	if ( g_SteamRunning )
	{
		Client()->ServerCheckEveryoneIsLoadedAndReady();
		return isEveryoneLoadedAndReady;
	}

	return 0;
}

FPSCR void SteamSendIAmReadyToPlay()
{
	lobbyChatIDs.clear();

	if ( g_SteamRunning )
	{
		Client()->SteamSendIAmReadyToPlay();
		char msg[80];
		sprintf ( msg , "s%s has joined the game" , SteamFriends()->GetPersonaName() );
		Client()->SteamSendChat(msg);
	}
}

FPSCR int SteamIsEveryoneReadyToPlay()
{
	lobbyChatIDs.clear();

	if ( g_SteamRunning )
	{
		Client()->ServerCheckEveryoneIsReadyToPlay();
		return isEveryoneReadyToPlay;
	}

	return 0;
}

FPSCR int SteamGetFileProgress()
{
	if ( g_SteamRunning )
		return Client()->SteamGetFileProgress();

	return 0;
}

FPSCR void SteamSetVoiceChat( int on )
{
	/*if ( voiceChatOn && on == 0 )
	{
		Client()->m_pVoiceChat->StopVoiceChat();
	}
	else if ( voiceChatOn == 0 && on == 1 )
	{
		Client()->m_pVoiceChat->StartVoiceChat();
	}

	voiceChatOn	 = on;*/
}

FPSCR void SteamSetPlayerAppearance( int a )
{
	if ( g_SteamRunning )
		Client()->SteamSetPlayerAppearance(a);
}

FPSCR int SteamGetPlayerAppearance( int index )
{
	if ( g_SteamRunning )
		return playerAppearance[index];

	return 0;
}

FPSCR void SteamSetCollision ( int index, int state )
{
	if ( g_SteamRunning )
		Client()->SteamSetCollision( index , state );
}

FPSCR int SteamGetCollisionList()
{
	if ( collisionList.size() > 0 )
	{
		currentCollisionObject.index = collisionList.back().index;
		currentCollisionObject.state = collisionList.back().state;
		return 1;
	}
	return 0;
}

FPSCR void SteamGetNextCollision()
{
	if ( collisionList.size() > 0 )
		collisionList.pop_back();
}

FPSCR int SteamGetCollisionIndex()
{
	return currentCollisionObject.index;
}

FPSCR int SteamGetCollisionState()
{
	return currentCollisionObject.state;
}

FPSCR void SteamShoot ()
{
	if ( g_SteamRunning )
		Client()->SteamShoot();
}

FPSCR int SteamGetShoot ( int index )
{
	int result = playerShoot[index];
	playerShoot[index] = 0;
	return result;
}


//========================================================================================
//========================================================================================

FPSCR void SteamPlayAnimation ( int index, int start, int end, int speed )
{
	if ( g_SteamRunning )
		Client()->SteamPlayAnimation ( index, start, end, speed );
}

FPSCR int SteamGetAnimationList()
{
	if ( animationList.size() > 0 )
	{
		currentAnimationObject.index = animationList.back().index;
		currentAnimationObject.start = animationList.back().start;
		currentAnimationObject.end = animationList.back().end;
		currentAnimationObject.speed = animationList.back().speed;
		return 1;
	}
	return 0;
}

FPSCR void SteamGetNextAnimation()
{
	if ( animationList.size() > 0 )
		animationList.pop_back();
}

FPSCR int SteamGetAnimationIndex()
{
	return currentAnimationObject.index;
}

FPSCR int SteamGetAnimationStart()
{
	return currentAnimationObject.start;
}

FPSCR int SteamGetAnimationEnd()
{
	return currentAnimationObject.end;
}

FPSCR int SteamGetAnimationSpeed()
{
	return currentAnimationObject.speed;
}

FPSCR void SteamSetTweening(int index , int flag)
{
	tweening[index] = flag;
}

FPSCR LPSTR SteamGetLobbyUserName( int index)
{
	// Delete old string
	//if(pOldString) g_pGlob->CreateDeleteString ( (DWORD*)&pOldString, 0 );

	if ( g_SteamRunning )
	{
		if ( lobbyIAmInID.IsValid() && lobbyIAmInID.IsLobby() )
		{
			CSteamID id = SteamMatchmaking()->GetLobbyMemberByIndex(lobbyIAmInID,index);
			CSteamID steamIDLobbyMember = SteamFriends()->GetFriendFromSourceByIndex( lobbyIAmInID, index );
			const char *pchName = SteamFriends()->GetFriendPersonaName( steamIDLobbyMember );

			// Create a new string and copy input string to it
			/*LPSTR pReturnString=NULL;
			DWORD dwSize=strlen( (const char*)pchName );
			g_pGlob->CreateDeleteString ( (DWORD*)&pReturnString, dwSize+1 );
			strcpy(pReturnString, (const char*)pchName);

			return pReturnString;*/
			return GetReturnStringFromTEXTWorkString( (char*)pchName );
		}
	}

	return "";
}

FPSCR void SteamLeaveLobby()
{
	if ( g_SteamRunning )
	{
		if ( lobbyIAmInID.IsValid() && lobbyIAmInID.IsLobby() )
		{
			Client()->SteamLeaveLobby();
		}
	}
}

FPSCR void SteamCreateWorkshopItem()
{
	if ( g_SteamRunning )
	{
		g_UserWorkShopItem.CreateWorkshopItem();
	}
}

FPSCR void SteamCreateWorkshopItem( LPSTR pString )
{
	if ( g_SteamRunning )
	{
		g_UserWorkShopItem.CreateWorkshopItem( pString );
	}
}

FPSCR void SteamDownloadWorkshopItem( LPSTR pString )
{

	if ( g_SteamRunning )
	{
		uint64 id = _atoi64(pString);
		g_UserWorkShopItem.DownloadWorkshopitem(id);
	}

}

FPSCR int SteamIsWorkshopItemDownloaded()
{
	if ( g_SteamRunning )
	{
		// TEMP!!!!!!!!!!
		//IsWorkshopLoadingOn = 1;

		return g_UserWorkShopItem.isItemDownloaded;
	}

	return 0;
}

FPSCR int SteamIsWorkshopItemUploaded()
{
	if ( g_SteamRunning )
	{
		return g_UserWorkShopItem.isItemUploaded;
	}

	return 0;
}

FPSCR LPSTR SteamGetWorkshopID(void)
{
	// Delete old string
	//if(pOldString) g_pGlob->CreateDeleteString ( (DWORD*)&pOldString, 0 );

	if ( g_SteamRunning )
	{
		// Create a new string and copy input string to it
		//LPSTR pReturnString=NULL;
		char workshopIDAsString[256];
		sprintf ( workshopIDAsString , "%llu" , WorkShopItemID );
		/*DWORD dwSize=strlen( (const char*)workshopIDAsString );
		g_pGlob->CreateDeleteString ( (DWORD*)&pReturnString, dwSize+1 );
		strcpy(pReturnString, (const char*)workshopIDAsString);

		return pReturnString;*/
		return GetReturnStringFromTEXTWorkString( workshopIDAsString );
	}

	return "";
}

FPSCR LPSTR SteamGetWorkshopItemPath(void)
{
	// Delete old string
	//if(pOldString) g_pGlob->CreateDeleteString ( (DWORD*)&pOldString, 0 );

	if ( g_SteamRunning )
	{
		// Create a new string and copy input string to it
		/*LPSTR pReturnString=NULL;
		DWORD dwSize=strlen( (const char*)WorkshopItemPath );
		g_pGlob->CreateDeleteString ( (DWORD*)&pReturnString, dwSize+1 );
		strcpy(pReturnString, (const char*)WorkshopItemPath);

		return pReturnString;*/
		return GetReturnStringFromTEXTWorkString( WorkshopItemPath );
	}

	return "";
}

FPSCR void SteamGetWorkshopItemPathDLL ( LPSTR string )
{
	if ( g_SteamRunning )
		strcpy(string, WorkshopItemPath);
	else
		strcpy(string, "");
}

FPSCR int SteamIsWorkshopLoadingOnDLL()
{
	// TEMP - REMOVE THIS LINE
	//return 1;
	if ( g_SteamRunning )
		return IsWorkshopLoadingOn;
	else
		return 0;
}

FPSCR void SteamSetRoot(LPSTR string )
{
	strcpy ( steamRootPath , string );
}

FPSCR void SteamSetLobbyName(LPSTR name )
{
	strcpy ( hostsLobbyName , name );
}

FPSCR int SteamIsWorkshopItemInstalled(LPSTR idString )
{
	if ( g_SteamRunning )
	{
		if ( strcmp ( idString , "" ) == 0 ) return 0;

		uint64 unSizeOnDisk = 0;
		strcpy ( WorkshopItemPath , "" );
		uint64 id = _atoi64(idString);		
		//bool legacySupport = false;
		//if ( SteamUGC()->GetItemInstallInfo( id, &unSizeOnDisk, WorkshopItemPath, sizeof(WorkshopItemPath) , &legacySupport ) )
		uint32 timestamp;
		if ( SteamUGC()->GetItemInstallInfo( id, &unSizeOnDisk, WorkshopItemPath, sizeof(WorkshopItemPath) , &timestamp ) )
		{
			bool needupdate = false;
			bool isDownloading = false;
			uint64 bytesDownloaded;
			uint64 bytesTotal;

			// tomato - Commented this out as GetItemUpdateInfo no longer exists
			// using the new getitemdownloadinfo
			if ( SteamUGC()->GetItemDownloadInfo ( id , &bytesDownloaded , &bytesTotal ) )
			//if ( SteamUGC()->GetItemUpdateInfo( id , &needupdate, &isDownloading, &bytesDownloaded, &bytesTotal ) )
			{
				uint32 ret = SteamUGC()->GetItemState ( id );
				if ( (ret & k_EItemStateNeedsUpdate) == 0 )
					needupdate = false;
				else
					needupdate = true;

				if ( needupdate )
				{
					g_UserWorkShopItem.DownloadWorkshopitem(id);
					isDownloading = true;
				}

				if (needupdate == false && isDownloading == false && bytesDownloaded == bytesTotal )
				{
					return 1;
				}
				else
				{
					if  ( needupdate == false )
						return 0;
					else
						return 2;
				}
			}

		}
	}

	return 0;
}

FPSCR int SteamHasSubscriptionWorkshopItemFinished()
{
	if ( g_SteamRunning )
	{
		return g_UserWorkShopItem.isItemDownloaded;
		// tomato
		//return g_UserWorkShopItem.isItemSubscribed;
	}

	return -1;

}

FPSCR void SteamSendMyName()
{
	needToSendMyName = true;
}

FPSCR int SteamIsOverlayActive()
{
	if ( g_SteamRunning )
		return SteamOverlayActive;

	return 0;
}

FPSCR void SteamWorkshopModeOff()
{
	IsWorkshopLoadingOn = 0;
}

FPSCR void SteamShowAgreement()
{
	ShellExecuteW( NULL, L"open", L"http://steamcommunity.com/sharedfiles/workshoplegalagreement" , NULL, NULL, SW_SHOWMAXIMIZED );
}

FPSCR void SteamEndGame()
{
	if ( g_SteamRunning )
		Client()->SteamEndGame();
}

FPSCR void SteamSendChat( LPSTR msg )
{
#ifdef MAKE_MULTIPLAYER_LOG
	debugLog ( "SteamSendChat" );
#endif
	if ( g_SteamRunning )
		Client()->SteamSendChat(msg);
}

FPSCR void SteamSendLobbyChat( LPSTR msg )
{
#ifdef MAKE_MULTIPLAYER_LOG
	debugLog ( "SteamSendLobbyChat" );
#endif
	if ( g_SteamRunning )
	{
		SteamMatchmaking()->SendLobbyChatMsg ( lobbyIAmInID , msg , strlen(msg)+1 );
	}
}

char chatTextReturn[100];

FPSCR LPSTR SteamGetChat( void )
{
#ifdef MAKE_MULTIPLAYER_LOG
	debugLog ( "SteamGetChat" );
#endif
	// Delete old string
	//if(pOldString) g_pGlob->CreateDeleteString ( (DWORD*)&pOldString, 0 );

	if ( chatList.size() > 0 )
	{
		// Create a new string and copy input string to it
		/*LPSTR pReturnString=NULL;
		DWORD dwSize=strlen( (const char*)chatList[0].msg );
		g_pGlob->CreateDeleteString ( (DWORD*)&pReturnString, dwSize+1 );
		strcpy(pReturnString, (const char*)chatList[0].msg);*/

		strcpy ( chatTextReturn , chatList[0].msg );
		chatList.erase( chatList.begin() );

		//return pReturnString;
		return GetReturnStringFromTEXTWorkString( chatTextReturn );

	}

	return "";
} 

FPSCR int SteamInkey( int scancode )
{
	WORD theChar;

   static HKL layout=GetKeyboardLayout(0);
   static UCHAR State[256];

   if (GetKeyboardState(State)==FALSE)
      return 0;
   UINT vk=MapVirtualKeyEx(scancode,1,layout);
   if ( ToAsciiEx(vk,scancode,State,&theChar,0,layout) == 1 )
   {
	   if ( KEYDOWN ( VK_LSHIFT ) || KEYDOWN ( VK_RSHIFT ) )
	   {
		   if ( theChar >= 97 && theChar <=122 )
			   theChar -= 32;
		   if ( theChar >= 91 && theChar <=96 )
			   theChar += 32;
		   if ( theChar >= 49 && theChar <=57 )
			   theChar -= 16;
		   if ( theChar == 48 )
			   theChar = 41;
	   }

	   return int(theChar);
   }

	return 0;
}

FPSCR int SteamCheckSyncedAvatarTexturesWithServer()
{
	if ( Client() ) 
		if ( Client()->IsServer() )
			SteamAvatarServer();

	SteamAvatarClient();

	if ( syncedAvatarTextureMode == SYNC_AVATAR_TEX_MODE_DONE ) return 1;
	return 0;
}

FPSCR void SteamSetMyAvatarHeadTextureName(LPSTR sAvatarTexture)
{
	strcpy ( myAvatarTextureName , sAvatarTexture );
}

void SteamAvatarClient()
{
	switch(syncedAvatarTextureMode )
	{
		case SYNC_AVATAR_TEX_BEGIN: // send my texture name to the server
		{
			syncedAvatarTextureMode = SYNC_AVATAR_TEX_SENT_IF_I_HAVE_TEXTURE;
			if ( Client() ) 
			{
				if ( strcmp ( "" , myAvatarTextureName ) == 0 )
					Client()->AvatarSendWeHaveHeadTextureToServer(2);
				else
					Client()->AvatarSendWeHaveHeadTextureToServer(1);
			}

		} break;
		case SYNC_AVATAR_TEX_SENT_IF_I_HAVE_TEXTURE: {} break; // waiting for server to change mode
		case SYNC_AVATAR_TEX_MODE_SENDING: // send our texture
		{

			syncedAvatarTextureMode = SYNC_AVATAR_TEX_MODE_SENDING_WAITING;

			if ( strcmp ( "" , myAvatarTextureName ) == 0 ) return;

			// if we have a texture to send, we can take one off the count
			if ( strcmp ( "" , myAvatarTextureName ) != 0 ) --syncedAvatarHowManyTextures;

			// lets make our own texture first, then send it
			char dest[MAX_PATH];
			sprintf ( dest, "%sentitybank\\user\\charactercreator\\customAvatar_%i_cc.dds" , steamRootPath , Client()->SteamGetMyPlayerIndex() );
			DeleteFile(dest);
			CopyFile ( myAvatarTextureName , dest, false );

			if ( Client() ) 
			{
				Client()->SteamSendAvatarFileClient ( Client()->SteamGetMyPlayerIndex() , dest );
			}

		} break;
		case SYNC_AVATAR_TEX_MODE_SENDING_WAITING: {} break; // waiting for server to change mode
		case SYNC_AVATAR_TEX_MODE_DONE: {} break; // All done
	}
}

void SteamAvatarServer()
{
	switch(syncedAvatarTextureModeServer)
	{	
		case SYNC_AVATAR_TEX_BEGIN:
		{
			if ( Server()->AvatarCheck(SYNC_AVATAR_TEX_BEGIN) == 1 )
				syncedAvatarTextureModeServer = SYNC_AVATAR_TEX_MODE_SENDING;
		} break;
		case SYNC_AVATAR_TEX_MODE_SENDING:
		{
			if ( Server()->AvatarCheck(SYNC_AVATAR_TEX_MODE_SENDING) == 1 )
				syncedAvatarTextureModeServer = SYNC_AVATAR_TEX_MODE_DONE;
		} break;
		case SYNC_AVATAR_TEX_MODE_DONE: {} break; // All done
	}
}

FPSCR int SteamStrCmp( LPSTR s1, LPSTR s2 )
{
#ifdef MAKE_MULTIPLAYER_LOG
	debugLog ( "SteamStrCmp" );
#endif
	return strcmp ( s1 , s2 );
}

FILE* debugFile = NULL;

void debugLog(LPSTR str )
{
	if ( !debugFile )
	{
		debugFile = fopen ( "Steam Multiplayer Debug Log.txt" , "a" );
		char newstr[256];
		strcpy ( newstr , "\n" );
		fwrite ( newstr , strlen(newstr) , 1 , debugFile );
		strcpy ( newstr , "========================= NEW =========================\n" );
		fwrite ( newstr , strlen(newstr) , 1 , debugFile );
		strcpy ( newstr , "========================= NEW =========================\n" );
		fwrite ( newstr , strlen(newstr) , 1 , debugFile );
		strcpy ( newstr , "\n" );
		fwrite ( newstr , strlen(newstr) , 1 , debugFile );

	}

	if ( debugFile )
	{
		fwrite ( str , strlen(str) , 1 , debugFile );
		fflush ( debugFile );
		fwrite ( "\n" , 1 , 1 , debugFile );
		fflush ( debugFile );
	}

}


void debugLog(LPSTR str, int code , int e , int v )
{
	if ( !debugFile )
	{
		debugFile = fopen ( "Steam Multiplayer Debug Log.txt" , "a" );
		char newstr[256];
		strcpy ( newstr , "\n" );
		fwrite ( newstr , strlen(newstr) , 1 , debugFile );
		strcpy ( newstr , "========================= NEW =========================\n" );
		fwrite ( newstr , strlen(newstr) , 1 , debugFile );
		strcpy ( newstr , "========================= NEW =========================\n" );
		fwrite ( newstr , strlen(newstr) , 1 , debugFile );
		strcpy ( newstr , "\n" );
		fwrite ( newstr , strlen(newstr) , 1 , debugFile );

	}

	if ( debugFile )
	{
		char newstr[1024];
		sprintf ( newstr , "%s ( %d , %d , %d )" , str, code, e, v );
		fwrite ( str , strlen(newstr) , 1 , debugFile );
		fflush ( debugFile );
		fwrite ( "\n" , 1 , 1 , debugFile );
		fwrite ( str , strlen(newstr) , 1 , debugFile );
	}

}

void debugLog(LPSTR str, int code )
{
	if ( !debugFile )
	{
		debugFile = fopen ( "Steam Multiplayer Debug Log.txt" , "a" );
		char newstr[256];
		strcpy ( newstr , "\n" );
		fwrite ( newstr , strlen(newstr) , 1 , debugFile );
		strcpy ( newstr , "========================= NEW =========================\n" );
		fwrite ( newstr , strlen(newstr) , 1 , debugFile );
		strcpy ( newstr , "========================= NEW =========================\n" );
		fwrite ( newstr , strlen(newstr) , 1 , debugFile );
		strcpy ( newstr , "\n" );
		fwrite ( newstr , strlen(newstr) , 1 , debugFile );

	}

	if ( debugFile )
	{
		char newstr[1024];
		sprintf ( newstr , "%s ( %d )" , str, code );
		fwrite ( str , strlen(newstr) , 1 , debugFile );
		fflush ( debugFile );
		fwrite ( "\n" , 1 , 1 , debugFile );
		fwrite ( str , strlen(newstr) , 1 , debugFile );
	}

}

//========================================================================================
//========================================================================================

#ifdef _DEBUG_LOG_

void logStart()
{
	logFile = fopen ( "multiplayer_logfile.txt" , "a" );
	log ( "=============================================================================" );
	log ( "================================ NEW SESSION ================================" );
	log ( "=============================================================================" );
}

void log( char* s)
{ 
	if ( !logFile ) logStart();

	if ( logFile )
	{
		fputs ( s , logFile );
		fputs ( "\n" , logFile );
	}
}

void log( char* s, int a)
{ 
	if ( !logFile ) logStart();

	if ( logFile )
	{
		char ts[512];
		sprintf ( ts , "%s, VAR: %d" , s , a );
		fputs ( ts , logFile );
		fputs ( "\n" , logFile );
	}
}

void log( char* s, int a, int b)
{ 

	if ( !logFile ) logStart();

	if ( logFile )
	{
		char ts[512];
		sprintf ( ts , "%s, VARS: %d, %d" , s , a , b );
		fputs ( ts , logFile );
		fputs ( "\n" , logFile );
	}
}

void log( char* s, int a, int b, int c)
{ 

	if ( !logFile ) logStart();

	if ( logFile )
	{
		char ts[512];
		sprintf ( ts , "%s, VARS: %d, %d, %d" , s , a , b , c );
		fputs ( ts , logFile );
		fputs ( "\n" , logFile );
	}
}

void logEnd()
{
	if ( logFile )
	{
		fclose ( logFile );
		logFile = NULL;
	}
}

#endif

//========================================================================================
//========================================================================================



