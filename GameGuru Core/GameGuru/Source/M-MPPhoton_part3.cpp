void mp_checkIfLobbiesAvailable ( void )
{
	if (  t.thowlongbetweenlobbychecks  <=  0  )  t.thowlongbetweenlobbychecks  =  15*1000;
	if (  MAXTimer() - t.tlasttimecheckedforlobbiestimer > t.thowlongbetweenlobbychecks ) 
	{
		SteamLoop (  );
		t.tlasttimecheckedforlobbiestimer = MAXTimer();
		if (  g.mp.checkiflobbiesavailablemode  ==  0 ) 
		{
			SteamGetLobbyList (  );
			g.mp.checkiflobbiesavailablemode = 1;
			return;
		}
		if (  g.mp.checkiflobbiesavailablemode  ==  1 ) 
		{
			g.mp.checkiflobbiesavailablemode = 0;

			if (  SteamIsLobbyListCreated()  ==  0 ) 
			{
				return;
			}


			t.thowmanylobbiesavailable = SteamGetLobbyListSize();
			if (  t.thowmanylobbiesavailable > 0 ) 
			{
				if (  t.thowmanylobbiesavailable > 10 ) 
				{
					t.thowlongbetweenlobbychecks = 60*1000;
				}
				if (  t.thowmanylobbiesavailable > 20 ) 
				{
					t.thowlongbetweenlobbychecks = 150*1000;
				}
				if (  t.thowmanylobbiesavailable > 40 ) 
				{
					t.thowlongbetweenlobbychecks = 300*1000;
				}
				t.steamStatusBar_s = "        |        Multiplayer Lobbies available to join";
			}
			else
			{
				t.thowlongbetweenlobbychecks = 15*1000;
				t.steamStatusBar_s = "";
			}
		}
	}
}

void mp_flashLightOff ( void )
{
	t.playerlight.flashlightcontrol_f=0.0;
}

void mp_setupCoopTeam ( void )
{
	for ( t.tc = 0 ; t.tc<=  MP_MAX_NUMBER_OF_PLAYERS-1; t.tc++ )
	{
		t.mp_team[t.tc] = 0;
	}
return;

//  requires; e, tSteamX# and tSteamZ#
}

void mp_COOP_aiMoveTo ( void )
{

	if (  g.mp.endplay  ==  1  )  return;

	if (  t.game.runasmultiplayer  ==  1 && g.mp.coop  ==  1 && t.tLuaDontSendLua  ==  0 ) 
	{

		SteamSendLua (  MP_LUA_AiGoToX,t.e,t.tSteamX_f );
		SteamSendLua (  MP_LUA_AiGoToZ,t.e,t.tSteamZ_f );
	}
	else
	{
		//  lets check if the distance is worth the effort of moving
		t.tdx_f=t.tSteamX_f-ObjectPositionX(t.e);
		t.tdz_f=t.tSteamZ_f-ObjectPositionZ(t.e);
		t.tdist_f=Sqrt((t.tdx_f*t.tdx_f)+(t.tdz_f*t.tdz_f));

		//  is it isn't very far, lets just stop the ai so it doesnt jerk about
		if (  t.tdist_f < 75.0 ) 
		{
			AISetEntityPosition (  t.e,t.tSteamX_f,BT_GetGroundHeight(t.terrain.TerrainID,t.tSteamX_f,t.tSteamZ_f),t.tSteamZ_f );
			AIEntityStop (  t.e );
		}
		else
		{
			AIEntityGoToPosition (  t.e, t.tSteamX_f, t.tSteamZ_f );
		}

	}
}

void mp_entity_lua_lookatplayer ( void )
{
	entity_lua_findcharanimstate ( );
	if (  t.tcharanimindex != -1 ) 
	{
		//  Simply look in direction of player
		t.ee = t.mp_playerEntityID[t.v];
		if ( t.mp_playerEntityID[t.v] > 0 )
		{
			t.tdx_f= ObjectPositionX (t.entityelement[t.ee].obj) - ObjectPositionX(t.entityelement[t.e].obj);
			t.tdz_f= ObjectPositionZ (t.entityelement[t.ee].obj) - ObjectPositionZ(t.entityelement[t.e].obj);
			AISetEntityAngleY (  t.charanimstate.obj,atan2deg(t.tdx_f,t.tdz_f) );
		
			//  If angle beyond 'look angle range', perform full rotation
			t.tangley_f=AIGetEntityAngleY(t.charanimstate.obj) ;
			t.headangley_f=t.tangley_f-ObjectAngleY(t.charanimstate.obj) ;
			if (  t.headangley_f<-180  )  t.headangley_f = t.headangley_f+360;
			if (  t.headangley_f>180  )  t.headangley_f = t.headangley_f-360;
			if (  t.headangley_f<-75 || t.headangley_f>75 ) 
			{
				t.charanimstate.currentangle_f=t.tangley_f;
				t.charanimstate.updatemoveangle=1;
				AISetEntityAngleY (  t.charanimstate.obj,t.charanimstate.currentangle_f );
				t.charanimstates[t.tcharanimindex] = t.charanimstate;
			}
		}
	}
}

void mp_entity_lua_fireweaponEffectOnly ( void )
{
	//  update gun appearance
	if (  t.entityelement[t.e].attachmentobj > 0 ) 
	{
		t.tgunid=t.entityelement[t.e].eleprof.hasweapon;
		t.tattachedobj=t.entityelement[t.e].attachmentobj;
		t.te = t.e;

		t.tgunid = t.entityelement[t.e].eleprof.hasweapon;
		t.ttrr=Rnd(1);
		for ( t.tt = t.ttrr+0 ; t.tt<=  t.ttrr+1; t.tt++ )
		{
			t.ttsnd=t.gunsoundcompanion[t.tgunid][1][t.tt].soundid;
			if (  t.ttsnd>0 ) 
			{
				if (  SoundExist(t.ttsnd) == 1 ) 
				{
					if (  SoundPlaying(t.ttsnd) == 0 || t.tt == t.ttrr+1 ) 
					{
						t.charanimstate.firesoundindex=t.ttsnd ; t.tt=3;
						t.charanimstate.firesoundexpiry= MAXTimer()+200;
					}
				}
			}
		}

		if (  t.charanimstate.firesoundindex>0 ) 
		{
			entity_lua_findcharanimstate ( );
			darkai_shooteffect ( );
		}

	}

	//  charanimstate is purely temporary, the firesoundindex will NOT be persistent!!
	t.charanimstate.firesoundindex=0;
}

void mp_updateAIForCOOP ( void )
{

		if (  g.mp.endplay  ==  1  )  return;

		t.tsentone = 0;

		if (  t.game.runasmultiplayer == 1 && g.mp.coop  ==  1 ) 
		{
			t.thowManyDoIHave = 0;
			//  only send one update to other players max
			//  so everyone gets a chance to update we keep track of where we were up to in the list last time we sent
			if (  t.tcoopyentityupdatetostartat  ==  0 || t.tcoopyentityupdatetostartat > g.entityelementlist ) 
			{
				t.tcoopyentityupdatetostartat = 1;
				++t.tcoopSendPositionUpdate;
				if (  t.tcoopSendPositionUpdate > 3  )  t.tcoopSendPositionUpdate  =  0;
			}
			for ( t.e = 1 ; t.e<=  g.entityelementlist; t.e++ )
			{
				t.entid=t.entityelement[t.e].bankindex;
				if (  t.entid>0 ) 
				{
					if (  (t.entityprofile[t.entid].ischaracter  ==  1 || t.entityelement[t.e].mp_isLuaChar  ==  1) && t.entityprofile[t.entid].ismultiplayercharacter  ==  0 ) 
					{
						if (  t.entityelement[t.e].health  ==  0 ) 
						{
							t.distx_f = CameraPositionX() - ObjectPositionX(t.entityelement[t.e].obj);
							t.distz_f = CameraPositionZ() - ObjectPositionZ(t.entityelement[t.e].obj);
							t.tdist_f = Sqrt((t.distx_f*t.distx_f)+(t.distz_f*t.distz_f));
							if (  t.tdist_f > 3000  )  ScaleObject (  t.entityelement[t.e].obj,0,0,0 );
						}
						if (  t.entityelement[t.e].mp_coopControlledByPlayer  !=  g.mp.me && t.entityelement[t.e].active == 1 && t.entityelement[t.e].health > 0 ) 
						{
							// rotate or look at player for 1 second after receiving lua command, to cut down packets being sent
							if (  t.entityelement[t.e].mp_rotateType  ==  1 ) 
							{
								if (  MAXTimer() - t.entityelement[t.e].mp_rotateTimer > 1000  )  t.entityelement[t.e].mp_rotateType  =  0;
								if (  t.entityelement[t.e].obj > 0 ) 
								{
									if (  ObjectExist(t.entityelement[t.e].obj)  ==  1 ) 
									{
										if (  t.entityelement[t.e].mp_coopControlledByPlayer > -1 && t.entityelement[t.e].mp_coopControlledByPlayer < MP_MAX_NUMBER_OF_PLAYERS ) 
										{
											t.v = t.entityelement[t.e].mp_coopControlledByPlayer;
											mp_entity_lua_lookatplayer ( );
										}
									}
								}
							}
							if (  t.entityelement[t.e].mp_rotateType  ==  2 ) 
							{
								if (  MAXTimer() - t.entityelement[t.e].mp_rotateTimer > 1000  )  t.entityelement[t.e].mp_rotateType  =  0;
								if (  t.entityelement[t.e].obj > 0 ) 
								{
									if (  ObjectExist(t.entityelement[t.e].obj)  ==  1 ) 
									{
										if (  t.entityelement[t.e].mp_coopControlledByPlayer > -1 && t.entityelement[t.e].mp_coopControlledByPlayer < MP_MAX_NUMBER_OF_PLAYERS ) 
										{
											t.v = t.entityelement[t.e].mp_coopControlledByPlayer;
											mp_entity_lua_lookatplayer ( );
										}
									}
								}
							}

							t.distx_f = CameraPositionX() - ObjectPositionX(t.entityelement[t.e].obj);
							t.distz_f = CameraPositionZ() - ObjectPositionZ(t.entityelement[t.e].obj);
							t.tdist_f = Sqrt((t.distx_f*t.distx_f)+(t.distz_f*t.distz_f));
							if (  t.tdist_f < 1200 || ( AIGetEntityHeardSound(t.entityelement[t.e].obj)  ==  1 && t.entityelement[t.e].mp_coopControlledByPlayer  ==  -1 ) ) 
							{
								if (  t.entityelement[t.e].mp_coopControlledByPlayer  ==  -1 ) 
								{
									t.tsteamplayeralive = 0;
								}
								else
								{
									t.tsteamplayeralive = SteamGetPlayerAlive(t.entityelement[t.e].mp_coopControlledByPlayer);
								}
								if (  MAXTimer() - t.entityelement[t.e].mp_coopLastTimeSwitchedTarget > 5000 || t.tsteamplayeralive  ==  0 ) 
								{
									t.tthrowaway = Rnd(1);
									if (  t.tsteamplayeralive  ==  0 || t.entityelement[t.e].mp_coopControlledByPlayer  ==  -1  )  t.tthrowaway  =  1;
									if (  t.tthrowaway  ==  1 ) 
									{
										t.entityelement[t.e].mp_coopControlledByPlayer = g.mp.me;
										SteamSendLua (  MP_LUA_TakenAggro,t.e,g.mp.me );
										SteamSendLua (  MP_LUA_AiGoToX,t.entityelement[t.e].obj,ObjectPositionX(t.entityelement[t.e].obj) );
										SteamSendLua (  MP_LUA_AiGoToZ,t.entityelement[t.e].obj,ObjectPositionZ(t.entityelement[t.e].obj) );
										t.entityelement[t.e].mp_updateOn = 1;
										t.entityelement[t.e].mp_lastUpdateSent = 0;
									}
									t.entityelement[t.e].mp_coopLastTimeSwitchedTarget = MAXTimer()+5000;
								}
							}
						}
						else
						{
							if (  t.entityelement[t.e].mp_coopControlledByPlayer  ==  g.mp.me ) 
							{
								if (  t.entityelement[t.e].mp_updateOn  ==  1 && t.tsentone  ==  0 ) 
								{
									if (  ObjectExist(t.entityelement[t.e].obj)  ==  1 ) 
									{
										AISetEntityActive (  t.entityelement[t.e].obj,1 );
									}
									if (  t.tcoopyentityupdatetostartat  <=  t.e ) 
									{

										if (  t.entityelement[t.e].active == 1 && t.entityelement[t.e].health > 0 ) 
										{

											if (  MAXTimer() - t.tcoopLastUpdateSent > 500 || t.tcoopLastUpdateSent < 0 ) 
											{
													t.tsentone = 1;
													SteamSendLua (  MP_LUA_HaveAggro,t.e,g.mp.me );
													SteamSendLua (  MP_LUA_AiGoToX,t.entityelement[t.e].obj,ObjectPositionX(t.entityelement[t.e].obj) );
													SteamSendLua (  MP_LUA_AiGoToZ,t.entityelement[t.e].obj,ObjectPositionZ(t.entityelement[t.e].obj) );
													t.entityelement[t.e].mp_lastUpdateSent = MAXTimer();
													t.tcoopLastUpdateSent = MAXTimer();
													t.tcoopyentityupdatetostartat = t.e+1;


											}
											else
											{
												//  pretend way have sent one this time, since we need to wait a little while
												t.tsentone = 1;
											}

										}

									}
								}
							}

						}
					}
				}
			}
			//  if we havent sent anything, reset the list
			if (  t.tsentone  ==  0 ) 
			{
				t.tcoopyentityupdatetostartat = 1;
				++t.tcoopSendPositionUpdate;
				if (  t.tcoopSendPositionUpdate > 3  )  t.tcoopSendPositionUpdate  =  0;
			}
		}

return;

}

void mp_coop_rotatetoplayer ( void )
{
	//  only rotate to player if enemy ai with proper rig
	if (  t.entityelement[t.e].mp_isLuaChar  ==  1  )  return;
	entity_lua_findcharanimstate ( );
	if (  t.tcharanimindex == -1 ) 
	{
		//  regular entity
		t.entityelement[t.e].ry=t.v;
		entity_lua_rotateupdate ( );
	}
	else
	{
		// character subsystem
		t.charanimstate.currentangle_f=t.v;
		t.charanimstate.updatemoveangle=1;
		AISetEntityAngleY ( t.charanimstate.obj,t.charanimstate.currentangle_f );
		t.charanimstates[t.tcharanimindex] = t.charanimstate;
		t.entityelement[t.e].ry=t.v;

		// 240217 - and update visually
		entity_lua_rotateupdate ( );
	}
}

void mp_storeOldEntityPositions ( void )
{

	Dim (  t.mp_oldEntityPositionsX,g.entityelementlist );
	Dim (  t.mp_oldEntityPositionsZ,g.entityelementlist );

	for ( t.e = 1 ; t.e<=  g.entityelementlist; t.e++ )
	{
		t.entid=t.entityelement[t.e].bankindex;
		if (  t.entid>0 ) 
		{
			if (  (t.entityprofile[t.entid].ischaracter  ==  1 || t.entityelement[t.e].mp_isLuaChar  ==  1) && t.entityprofile[t.entid].ismultiplayercharacter  ==  0 ) 
			{
				if (  t.entityelement[t.e].obj > 0 ) 
				{
					if (  ObjectExist(t.entityelement[t.e].obj)  ==  1 ) 
					{
						t.mp_oldEntityPositionsX[t.e] = ObjectPositionX(t.entityelement[t.e].obj);
						t.mp_oldEntityPositionsZ[t.e] = ObjectPositionZ(t.entityelement[t.e].obj);
					}
				}
			}
		}
	}

return;

}

void mp_howManyEnemiesLeftToKill ( void )
{
		if (  g.mp.coop  ==  1 ) 
		{
			t.thowmanyenemies = 0;
			for ( t.e = 1 ; t.e<=  g.entityelementlist; t.e++ )
			{
				t.entid=t.entityelement[t.e].bankindex;
				if (  t.entid>0 ) 
				{
					if (  t.entityprofile[t.entid].ischaracter  ==  1 || t.entityelement[t.e].mp_isLuaChar  ==  1 ) 
					{
						if (  t.entityelement[t.e].active  ==  1 && t.entityelement[t.e].health > 0 ) 
						{
							++t.thowmanyenemies;
						}
					}
				}
			}
			LuaSetInt (  "mp_enemiesLeftToKill", t.thowmanyenemies );
		}
return;

}

void mp_IKilledAnAI ( void )
{
	t.mp_kills[g.mp.me+1] = t.mp_kills[g.mp.me+1] + 1;
	SteamSendLua (  MP_LUA_ServerSetPlayerKills,g.mp.me+1,t.mp_kills[g.mp.me+1] );
	t.tnothing = LuaExecute( cstr(cstr("mp_playerKills[") + Str(g.mp.me+1) + "] = " + Str(t.mp_kills[g.mp.me+1])).Get());
}

void mp_text ( int x, int y, int size, char* txt_s )
{
	t.luaText.txt = txt_s;
	t.luaText.x = x;
	t.luaText.y = y;
	t.luaText.size = size;
	lua_text ( );
}

void mp_textDots ( int x, int y, int size, char* txt_s )
{

	if (  MAXTimer() - g.mp.steamdotsoldtime > 150 ) 
	{
		g.mp.steamdotsoldtime = MAXTimer();
		g.mp.buildingDots = g.mp.buildingDots + ".";
		if (  Len(g.mp.buildingDots.Get()) > 5  )  g.mp.buildingDots  =  ".";
	}

	t.luaText.txt = g.mp.buildingDots + txt_s + g.mp.buildingDots;
	t.luaText.x = x;
	t.luaText.y = y;
	t.luaText.size = size;
	lua_text ( );
}

void mp_textColor ( int x, int y, int size, char* txt_s, int r, int gg, int b )
{
	g.mp.steamDoColorText = 1;
	t.luaText.txt = txt_s;
	t.luaText.x = x;
	t.luaText.y = y;
	t.luaText.size = size;
	g.mp.steamColorRed = r;
	g.mp.steamColorGreen = gg;
	g.mp.steamColorBlue = b;
	lua_text ( );
}

void mp_panel ( int x, int y, int x2, int y2 )
{
	t.luaPanel.x = x;
	t.luaPanel.y = y;
	t.luaPanel.x2 = x2;
	t.luaPanel.y2 = y2;
	lua_panel ( );
//endfunction

}

void mp_deleteFile ( char* tempFileToDelete_s )
{
	cstr fileToDelete;
	fileToDelete =  cstr(g.fpscrootdir_s + "\\Files\\" + tempFileToDelete_s).Get();
	if (  FileExist(fileToDelete.Get() ) )  DeleteAFile (  fileToDelete.Get() );
}

int mp_check_if_lua_entity_exists ( int tentitytocheck )
{
	int tcheckobj = 0;
	int result = 0;
	if ( tentitytocheck <= g.entityelementlist ) 
	{
		tcheckobj = t.entityelement[tentitytocheck].obj;
		if ( tcheckobj > 0 ) 
		{
			if ( ObjectExist(tcheckobj) == 1 ) 
			{
				result = 1;
			}
		}
	}
	return result;
}

void mp_sendlua ( int code, int e, int v )
{
	 PhotonSendLua ( code, e, v );
}

void mp_sendluaToPlayer ( int iRealPhotonPlayerID, int code, int e, int v )
{
	 PhotonSendLuaToPlayer ( iRealPhotonPlayerID, code, e, v );
}
