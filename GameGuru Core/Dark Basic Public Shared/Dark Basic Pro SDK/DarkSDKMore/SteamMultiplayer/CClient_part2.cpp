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

	/*char s[256];
	sprintf ( s, "Client Outstanding:%d" , PacketSend_Log_Client.size() );
	Print ( s );*/

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
