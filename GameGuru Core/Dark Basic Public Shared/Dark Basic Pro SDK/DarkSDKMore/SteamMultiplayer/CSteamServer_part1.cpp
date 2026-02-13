int CSteamServer::AvatarCheck( int check )
{
	int result = 1;
	bool sendMsg = false;
	MsgServerAvatarChangeMode_t msg;

	switch ( check )
	{
		case SYNC_AVATAR_TEX_BEGIN:
		{
			int amount = 0;
			ServerAvatarfilesReceived = 0;

			for( uint32 i = 0; i < MAX_PLAYERS_PER_SERVER; i++ )
			{
				if ( m_rgpPlayer[i] && serverClientsHaveAvatarTexture[i] == 0 )
				{
					result = 0;
					break;
				}

				// check for avatar texture
				if ( m_rgpPlayer[i] && serverClientsHaveAvatarTexture[i] == 1 )
				{
					amount++;
				}
			}

			if ( result == 1 )
			{
				sendMsg = true;

				if ( amount == 0 )
					msg.mode = SYNC_AVATAR_TEX_MODE_DONE;
				else
					msg.mode = SYNC_AVATAR_TEX_MODE_SENDING;

				// work out how many textures to send

				msg.result = amount;
			}

		} break;
		case SYNC_AVATAR_TEX_MODE_SENDING:
		{
			int amount = 0;

			for( uint32 i = 0; i < MAX_PLAYERS_PER_SERVER; i++ )
			{
				// check for avatar texture
				if ( m_rgpPlayer[i] && serverClientsHaveAvatarTexture[i] == 1 )
				{
					amount++;
				}
			}

			if ( ServerAvatarfilesReceived < amount ) result = 0;

			if ( result == 1 )
			{
				sendMsg = true;

				msg.mode = SYNC_AVATAR_TEX_MODE_DONE;
			}

		} break;
	}

	// Check if we should send out a mode change
	if ( sendMsg == true )
	{
		for( uint32 i=0; i<MAX_PLAYERS_PER_SERVER; i++ )
		{
			if ( m_rgpPlayer[i] )
			{				
				SteamGameServerNetworking()->SendP2PPacket( m_rgClientData[i].m_SteamIDUser, &msg, sizeof(MsgServerAvatarChangeMode_t), k_EP2PSendReliable );
			}
		}
	}

	return result;
}

void CSteamServer::ServerEndGame( int index )
{

	MsgEndGame_t msg;

	for( uint32 i=0; i<MAX_PLAYERS_PER_SERVER; ++i )
	{
		if ( m_rgpPlayer[i] && i != index )
			SteamGameServerNetworking()->SendP2PPacket( m_rgClientData[i].m_SteamIDUser, &msg, sizeof(MsgEndGame_t), k_EP2PSendReliable );
	}
}

void CSteamServer::CheckReceipts()
{

	/*char s[256];
	sprintf ( s, "Server Outstanding:%d" , PacketSend_Log_Server.size() );
	Print ( s );*/

	double timeNow = GetCounterPassedTotal();

	for ( unsigned int c = 0; c < PacketSend_Log_Server.size() ; c++ )
	{
		if ( timeNow - PacketSend_Log_Server[c].timeStamp > 1000 )
		{
			PacketSend_Log_Server[c].timeStamp = timeNow;
			// send again
			int index = PacketSend_Log_Server[c].playerID;
			switch ( PacketSend_Log_Server[c].packetType )
			{
			case k_EMsgClientEveryoneReadyToPlay:
				if ( m_rgpPlayer[index] )
					SteamGameServerNetworking()->SendP2PPacket( m_rgClientData[index].m_SteamIDUser, PacketSend_Log_Server[c].pPacket, sizeof(MsgClientEveryoneReadyToPlay_t), k_EP2PSendUnreliable ); break;
			case k_EMsgClientEveryoneLoadedAndReady:
				if ( m_rgpPlayer[index] )
					SteamGameServerNetworking()->SendP2PPacket( m_rgClientData[index].m_SteamIDUser, PacketSend_Log_Server[c].pPacket, sizeof(MsgClientEveryoneLoadedAndReady_t), k_EP2PSendUnreliable ); break;
			case k_EMsgClientKilledBy:
				if ( m_rgpPlayer[index] )
					SteamGameServerNetworking()->SendP2PPacket( m_rgClientData[index].m_SteamIDUser, PacketSend_Log_Server[c].pPacket, sizeof(MsgClientKilledBy_t), k_EP2PSendUnreliable ); break;
			case k_EMsgClientPlayerApplyDamage:
				if ( m_rgpPlayer[index] )
					SteamGameServerNetworking()->SendP2PPacket( m_rgClientData[index].m_SteamIDUser, PacketSend_Log_Server[c].pPacket, sizeof(MsgClientPlayerApplyDamage_t), k_EP2PSendUnreliable ); break;
			case k_EMsgServerLua:
				if ( m_rgpPlayer[index] )
					SteamGameServerNetworking()->SendP2PPacket( m_rgClientData[index].m_SteamIDUser, PacketSend_Log_Server[c].pPacket, sizeof(MsgServerLua_t), k_EP2PSendUnreliable ); break;
			case k_EMsgServerLuaString:
				if ( m_rgpPlayer[index] )
					SteamGameServerNetworking()->SendP2PPacket( m_rgClientData[index].m_SteamIDUser, PacketSend_Log_Server[c].pPacket, sizeof(MsgServerLuaString_t), k_EP2PSendUnreliable ); break;
			}

		}
	}
}

void CSteamServer::GotReceipt( int c )
{
	int found = -1;
	for ( unsigned int i = 0; i < PacketSend_Log_Server.size() ; i++ )
	{
		if ( PacketSend_Log_Server[i].LogID == c ) 
		{
			found = i;
			break;
		}
	}

	if ( found > -1 )
	{
		switch ( PacketSend_Log_Server[found].packetType )
		{
		case k_EMsgClientEveryoneReadyToPlay:
			delete (MsgClientEveryoneReadyToPlay_t*)PacketSend_Log_Server[found].pPacket; break;
		case k_EMsgClientEveryoneLoadedAndReady:
			delete (MsgClientEveryoneLoadedAndReady_t*)PacketSend_Log_Server[found].pPacket; break;
		case k_EMsgClientKilledBy:
			delete (MsgClientKilledBy_t*)PacketSend_Log_Server[found].pPacket; break;
		case k_EMsgClientPlayerApplyDamage:
			delete (MsgClientPlayerApplyDamage_t*)PacketSend_Log_Server[found].pPacket; break;
		case k_EMsgServerLua:
			delete (MsgServerLua_t*)PacketSend_Log_Server[found].pPacket; break;
		case k_EMsgServerLuaString:
			delete (MsgServerLuaString_t*)PacketSend_Log_Server[found].pPacket; break;
		}

		PacketSend_Log_Server.erase(PacketSend_Log_Server.begin()+found);
	}
}
