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
*/
