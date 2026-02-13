void mp_updatePlayerInput ( void )
{

//  `print "RELOADING: " + Str(plrreloading)

	
	if (  t.playercontrol.plrhitfloormaterial  ==  0 ) 
	{
		if (  g.mp.oldfootfloortime  ==  0  )  g.mp.oldfootfloortime  =  Timer();
		if (  Timer()-g.mp.oldfootfloortime > 100  )  g.mp.footfloor  =  0;
	}
	else
	{
		g.mp.oldfootfloortime = 0;
		g.mp.footfloor = 1;
	}
	
	if (  g.plrreloading  ==  0 ) 
	{
		g.mp.reloadingCount = 0;
	}

	t.tTime = Timer();
	
	if (  t.tTime - g.mp.lastSendTimeAppearance > MP_APPEARANCE_UPDATE_DELAY ) 
	{
	
		if (  g.plrreloading  !=  0 ) 
		{
			++g.mp.reloadingCount;
			if (  g.mp.reloadingCount < 4 ) 
			{
				g.mp.reloading = 1;
			}
			else
			{
				g.mp.reloading = 0;
			}
		}
	
		g.mp.lastSendTimeAppearance = t.tTime;
		if (  t.playercontrol.jetpackmode  !=  2 && g.mp.reloading  ==  0 ) 
		{
			SteamSetPlayerAppearance (  g.mp.appearance );
		}
		else
		{
			if (  t.playercontrol.jetpackmode  !=  0 ) 
			{
				if (  g.mp.footfloor  ==  1 ) 
				{
					SteamSetPlayerAppearance (  101 );
				}
				else
				{
					SteamSetPlayerAppearance (  102 );
				}
				g.mp.reloading = 0;
			}
			else
			{
				if (  g.mp.reloading  ==  1 ) 
				{
						SteamSetPlayerAppearance (  201 );
				}
				if (  g.plrreloading  ==  0 ) 
				{
					g.mp.reloading = 0;
				}
			}
		}
//   `print "sending appearance update"

	}
	
	if (  t.tTime - g.mp.lastSendTime < MP_INPUT_UPDATE_DELAY  )  return;
	
	g.mp.lastSendTime = t.tTime;
	
//  `print "sending input update"

	
	if (  g.mp.meleeOn  ==  0 ) 
	{
		if (  KeyState(g.keymap[17])  ==  1 || KeyState(g.keymap[200])  ==  1 ) 
		{
			SteamSetKeyState (  17,1 );
		}
		else
		{
			SteamSetKeyState (  17,0 );
		}
		if (  KeyState(g.keymap[31])  ==  1 || KeyState(g.keymap[208])  ==  1 ) 
		{
			SteamSetKeyState (  31,1 );
		}
		else
		{
			SteamSetKeyState (  31,0 );
		}
		if (  KeyState(g.keymap[30])  ==  1 || KeyState(g.keymap[203])  ==  1 ) 
		{
			SteamSetKeyState (  30,1 );
		}
		else
		{
			SteamSetKeyState (  30,0 );
		}
		if (  KeyState(g.keymap[32])  ==  1 || KeyState(g.keymap[205])  ==  1 ) 
		{
			SteamSetKeyState (  32,1 );
		}
		else
		{
			SteamSetKeyState (  32,0 );
		}
	}
if (  KeyState(g.keymap[46])  ==  1 || KeyState(g.keymap[29])  ==  1 || KeyState(g.keymap[157])  ==  1 ) 
{
	SteamSetKeyState (  46,1 );
	g.mp.crouchOn = 1;
}
else
{
	SteamSetKeyState (  46,0 );
	g.mp.crouchOn = 0;
}
//  shift keys
if (  KeyState(g.keymap[42])  ==  1 || KeyState(g.keymap[54])  ==  1 ) 
{
	SteamSetKeyState (  42,1 );
}
else
{
	SteamSetKeyState (  42,0 );
}

return;

}

void mp_load_guns ( void )
{

/*       Debug info
for ( t.tgindex = 1 ; t.tgindex<=  g.gunmax; t.tgindex++ )
	if (  t.gun[t.tgindex].activeingame == 1 ) 
	{
		t.tweaponname_s=t.gun[t.tgindex].t.name_s;
		tactiveGun = t.tgindex;
		Print (  t.tweaponname_s );
	}
}
*/    

g.mp.gunCount = 0;

	//  all vweaps (that are active)
	for ( t.tgindex = 1 ; t.tgindex<=  g.gunmax; t.tgindex++ )
	{
		if (  t.gun[t.tgindex].activeingame == 1 ) 
		{
			t.tweaponname_s=t.gun[t.tgindex].name_s;
			if (  t.tweaponname_s != "" ) 
			{

				//  go and load this gun (attached to calling entity instance)
				t.ttobj=g.mp.gunCount+g.steamplayermodelsoffset;
				t.mp_gunobj[g.mp.gunCount] = t.ttobj;
				t.mp_gunname[g.mp.gunCount] = Lower(t.tweaponname_s.Get());
				++g.mp.gunCount;
				if (  ObjectExist(t.ttobj) == 1  )  DeleteObject (  t.ttobj );

				//  replaced X file load with optional DBO convert/load
				t.tfile_s=cstr("gamecore\\guns\\")+t.tweaponname_s+"\\vweap.x";
				deleteOutOfDateDBO(t.tfile_s.Get());
				if (  cstr(Lower(Right(t.tfile_s.Get(),2))) == ".x"  )  t.tdbofile_s = cstr(Left(t.tfile_s.Get(),Len(t.tfile_s.Get())-2))+cstr(".dbo"); else t.tdbofile_s = "";
				if (  FileExist(t.tfile_s.Get()) == 1 || FileExist(t.tdbofile_s.Get()) == 1 ) 
				{
					if (  FileExist(t.tdbofile_s.Get()) == 1 ) 
					{
						t.tfile_s=t.tdbofile_s;
						t.tdbofile_s="";
					}
					LoadObject (  t.tfile_s.Get(),t.ttobj );
					SetObjectFilter (  t.ttobj,2 );
					if ( Len(t.tdbofile_s.Get())>1 ) 
					{
						if ( FileExist( t.tdbofile_s.Get()) == 0 ) 
						{
							// unnecessary now as LoadObject auto creates DBO file!
							SaveObject ( t.tdbofile_s.Get(), t.ttobj );
						}
						if (  FileExist(t.tdbofile_s.Get()) == 1 ) 
						{
							DeleteObject (  t.ttobj );
							LoadObject (  t.tdbofile_s.Get(),t.ttobj );
							SetObjectFilter (  t.ttobj,2 );
							t.tfile_s=t.tdbofile_s;
						}
					}
				}
				else
				{
					MakeObjectTriangle (  t.ttobj,0,0,0,0,0,0,0,0,0 );
				}

				//  Apply object settings
				SetObjectTransparency (  t.ttobj,1 );
				SetObjectCollisionOff (  t.ttobj );
				HideObject (  t.ttobj );

				//  VWEAP is NOT part of collision universe (prevents rocket hitting launcher)
				SetObjectCollisionProperty (  t.ttobj,1 );

				//  apply texture to vweap
				if (  g.gdividetexturesize == 0 ) 
				{
					t.texuseid=loadinternaltexture("effectbank\\reloaded\\media\\white_D.dds");
				}
				else
				{
					t.texuseid=loadinternaltexture( cstr(cstr("gamecore\\guns\\")+t.tweaponname_s+"\\gun_D.dds").Get() );
				}
				TextureObject (  t.ttobj,0,t.texuseid );
				t.texuseid=loadinternaltexture( cstr(cstr("gamecore\\guns\\")+t.tweaponname_s+"\\gun_N.dds").Get() );
				TextureObject (  t.ttobj,1,loadinternaltexture( "effectbank\\reloaded\\media\\blank_O.dds" ));
				TextureObject (  t.ttobj,2,t.texuseid );
				t.texuseid=loadinternaltexture( cstr(cstr("gamecore\\guns\\")+t.tweaponname_s+"\\gun_S.dds").Get() );
				TextureObject (  t.ttobj,3,t.texuseid );
				TextureObject (  t.ttobj,4,t.terrain.imagestartindex );
				TextureObject (  t.ttobj,5,g.postprocessimageoffset+5 );
				TextureObject (  t.ttobj,6,loadinternaltexture( "effectbank\\reloaded\\media\\blank_I.dds") );

				//  Apply entity shader to vweap model
				t.teffectid=loadinternaleffect("effectbank\\reloaded\\entity_basic.fx");
				SetObjectEffect (  t.ttobj,t.teffectid );

				//  07032015 - 016 - ensure the gun orders are the same on all machines
				for ( t.i = 0 ; t.i<=  g.mp.gunCount-2; t.i++ )
				{
					for ( t.j = t.i ; t.j<=  g.mp.gunCount-1; t.j++ )
					{
						if (  SteamStrCmp(t.mp_gunname[t.i].Get(),t.mp_gunname[t.j].Get()) > 0 ) 
						{
							t.ttemp_s = t.mp_gunname[t.i];
							t.mp_gunname[t.i] = t.mp_gunname[t.j];
							t.mp_gunname[t.j]=t.ttemp_s;

							t.ttemp = t.mp_gunobj[t.i];
							t.mp_gunobj[t.i] = t.mp_gunobj[t.j];
							t.mp_gunobj[t.j]=t.ttemp;
						}
					}
				} 
			}

		}
		SteamLoop (  );
	}
}

void mp_check_for_attachments ( void )
{

	for ( t.c = 0 ; t.c<=  MP_MAX_NUMBER_OF_PLAYERS-1; t.c++ )
	{
		if (  t.c  !=  g.mp.me ) 
		{
		
			//  Jetpack
			if (  SteamGetPlayerAppearance(t.c)  !=  t.mp_oldAppearance[t.c] ) 
			{
				t.mp_playingAnimation[t.c] = MP_ANIMATION_NONE;
				if (  SteamGetPlayerAppearance(t.c)  ==  101 || SteamGetPlayerAppearance(t.c)  ==  102 ) 
				{
					t.mp_playingAnimation[t.c] = MP_ANIMATION_NONE;
					t.e = t.mp_playerEntityID[t.c];
					entity_freeattachment ( );

					if (  t.mp_jetpackparticles[t.c]  ==  -1 && SteamGetPlayerAppearance(t.c)  ==  102 ) 
					{
						mp_addJetpackParticles ( );
					}
					if (  SteamGetPlayerAppearance(t.c)  ==  101 && t.mp_jetpackparticles[t.c]  !=  -1 ) 
					{
							t.tRaveyParticlesEmitterID=t.mp_jetpackparticles[t.c];
							ravey_particles_delete_emitter ( );
							t.mp_jetpackparticles[t.c]=-1;
					}

					if (  t.entityelement[t.mp_playerEntityID[t.c]].attachmentobj > 0 ) 
					{
						DeleteObject (  t.entityelement[t.mp_playerEntityID[t.c]].attachmentobj );
						t.mp_playingAnimation[t.c] = MP_ANIMATION_NONE;
						t.entityelement[t.mp_playerEntityID[t.c]].attachmentobj = 0;
					}

					if (  ObjectExist(g.steamplayermodelsoffset+t.c+121)  ==  0 ) 
					{
						if (  t.playercontrol.jetobjtouse > 0 ) 
						{
							if (  ObjectExist (g.steamplayermodelsoffset+120)  ) 
							{
								CloneObject (  g.steamplayermodelsoffset+t.c+121,g.steamplayermodelsoffset+120 );
							}
						}
					}
				}
			}
			if (  SteamGetPlayerAppearance(t.c)  !=  101 && SteamGetPlayerAppearance(t.c)  !=  102 ) 
			{
				if (  t.mp_jetpackparticles[t.c]  !=  -1 ) 
				{
					t.tRaveyParticlesEmitterID=t.mp_jetpackparticles[t.c];
					ravey_particles_delete_emitter ( );
					t.mp_jetpackparticles[t.c] = -1;
				}
			}
			//  Gun
			if (  SteamGetPlayerAppearance(t.c)  !=  t.mp_oldAppearance[t.c] && SteamGetPlayerAppearance(t.c) < 101 ) 
			{
				t.mp_playingAnimation[t.c] = MP_ANIMATION_NONE;
				if (  ObjectExist(g.steamplayermodelsoffset+t.c+121)  ==  1 ) 
				{
					HideObject (  g.steamplayermodelsoffset+t.c+121 );
				}
				t.e = t.mp_playerEntityID[t.c];
				entity_freeattachment ( );
				if (  t.entityelement[t.mp_playerEntityID[t.c]].attachmentobj > 0 ) 
				{
					DeleteObject (  t.entityelement[t.mp_playerEntityID[t.c]].attachmentobj );
					t.entityelement[t.mp_playerEntityID[t.c]].attachmentobj = 0;
				}
				if (  ObjectExist(g.steamplayermodelsoffset+t.c+100)  ==  0 ) 
				{

					t.tobj = 0;
					if (  SteamGetPlayerAppearance(t.c) > 0 ) 
					{
						t.tobj = t.mp_gunobj[SteamGetPlayerAppearance(t.c)-1];
					}

					if (  t.tobj > 0 ) 
					{
						if (  ObjectExist(t.tobj)  ==  1 ) 
						{
							CloneObject (  g.steamplayermodelsoffset+t.c+100,t.tobj );
							ShowObject (  g.steamplayermodelsoffset+t.c+100 );
							SetObjectMask (  g.steamplayermodelsoffset+t.c+100,1 );
							t.entityelement[t.mp_playerEntityID[t.c]].attachmentobj = g.steamplayermodelsoffset+t.c+100;

							t.tfound = 0;
							for ( t.tgindex = 1 ; t.tgindex<=  g.gunmax; t.tgindex++ )
							{
								if (  t.gun[t.tgindex].activeingame == 1 ) 
								{
									if (  t.mp_gunname[SteamGetPlayerAppearance(t.c)-1] == Lower(t.gun[t.tgindex].name_s.Get()) ) 
									{
										t.tfound = t.tgindex;
									}
								}
							}

							t.entityelement[t.mp_playerEntityID[t.c]].eleprof.hasweapon = t.tfound;

						//  Find firespot for this vweap
						t.entityelement[t.e].attachmentobjfirespotlimb=0;
						PerformCheckListForLimbs (  t.tobj );
						for ( t.tc = 1 ; t.tc<=  ChecklistQuantity(); t.tc++ )
						{
							if (  cstr(Lower(ChecklistString(t.tc))) == "firespot" ) 
							{
								t.entityelement[t.e].attachmentobjfirespotlimb=t.tc-1;
								t.tc=ChecklistQuantity()+1;
							}
						}

						}
					}
					else
					{
						if (  t.entityelement[t.mp_playerEntityID[t.c]].attachmentobj > 0 ) 
						{
							if (  ObjectExist(t.entityelement[t.mp_playerEntityID[t.c]].attachmentobj) ) 
							{
								DeleteObject (  t.entityelement[t.mp_playerEntityID[t.c]].attachmentobj );
								t.entityelement[t.mp_playerEntityID[t.c]].eleprof.hasweapon = 0;
							}
						}
					}

				}
			}

			//  update jetpack appearance
			if (  SteamGetPlayerAppearance(t.c)  ==  101 || SteamGetPlayerAppearance(t.c)  ==  102 ) 
			{
				if (  ObjectExist(g.steamplayermodelsoffset+t.c+121)  ==  1 ) 
				{
					ShowObject (  g.steamplayermodelsoffset+t.c+121 );
					t.tobj = t.entityelement[t.mp_playerEntityID[t.c]].obj;
					if (  SteamGetKeyState(t.c,46)  ==  1 ) 
					{
						PositionObject (  g.steamplayermodelsoffset+t.c+121, ObjectPositionX(t.tobj), ObjectPositionY(t.tobj)+20, ObjectPositionZ(t.tobj) );
					}
					else
					{
						PositionObject (  g.steamplayermodelsoffset+t.c+121, ObjectPositionX(t.tobj), ObjectPositionY(t.tobj)+40, ObjectPositionZ(t.tobj) );
					}
					YRotateObject (  g.steamplayermodelsoffset+t.c+121,ObjectAngleY(t.tobj) );
				}
			}

			//  update gun appearance
			if (  t.entityelement[t.mp_playerEntityID[t.c]].attachmentobj > 0 ) 
			{
				t.e = t.mp_playerEntityID[t.c];
				entity_controlattachments ( );

				if (  t.mp_playerShooting[t.c]  ==  1 ) 
				{
					t.tgunid=t.entityelement[t.mp_playerEntityID[t.c]].eleprof.hasweapon;
					t.tattachedobj=t.entityelement[t.mp_playerEntityID[t.c]].attachmentobj;
					t.te = t.mp_playerEntityID[t.c];

					t.tgunid = t.entityelement[t.mp_playerEntityID[t.c]].eleprof.hasweapon;
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
									t.charanimstate.firesoundexpiry=Timer()+200;
								}
							}
						}
					}

					if (  t.charanimstate.firesoundindex>0 ) 
					{
						darkai_shooteffect ( );
					}
				}

			}
		}

		if (  t.mp_oldAppearance[t.c]  !=  SteamGetPlayerAppearance(t.c)  )  t.mp_playingAnimation[t.c]  =  MP_ANIMATION_NONE;
		t.mp_oldAppearance[t.c] = SteamGetPlayerAppearance(t.c);

	}
	return;
}

void mp_addJetpackParticles ( void )
{

	t.tpartObj = t.entityelement[t.mp_playerEntityID[t.c]].obj;

	ravey_particles_get_free_emitter ( );
	if (  t.tResult>0 ) 
	{
		t.mp_jetpackparticles[t.c]=t.tResult;
		g.tEmitter.id = t.tResult;
		g.tEmitter.emitterLife = 0;
		g.tEmitter.parentObject = t.tpartObj;
		g.tEmitter.parentLimb = 0;
		g.tEmitter.isAnObjectEmitter = 0;
		g.tEmitter.imageNumber = RAVEY_PARTICLES_IMAGETYPE_LIGHTSMOKE + g.particlesimageoffset;
		g.tEmitter.isAnimated = 1;
		g.tEmitter.animationSpeed = 1/64.0;
		g.tEmitter.isLooping = 1;
		g.tEmitter.frameCount = 64;
		g.tEmitter.startFrame = 0;
		g.tEmitter.endFrame = 63;
		g.tEmitter.startsOffRandomAngle = 1;
		g.tEmitter.offsetMinX = -20;
		g.tEmitter.offsetMinY = 50;
		g.tEmitter.offsetMinZ = -20;
		g.tEmitter.offsetMaxX = 20;
		g.tEmitter.offsetMaxY = 50;
		g.tEmitter.offsetMaxZ = 20;
		g.tEmitter.scaleStartMin = 5;
		g.tEmitter.scaleStartMax = 10;
		g.tEmitter.scaleEndMin = 90;
		g.tEmitter.scaleEndMax = 100;
		g.tEmitter.movementSpeedMinX = -0.1f;
		g.tEmitter.movementSpeedMinY = -0.9f;
		g.tEmitter.movementSpeedMinZ = -0.1f;
		g.tEmitter.movementSpeedMaxX = 0.1f;
		g.tEmitter.movementSpeedMaxY = -0.1f;
		g.tEmitter.movementSpeedMaxZ = 0.1f;
		g.tEmitter.rotateSpeedMinZ = -0.1f;
		g.tEmitter.rotateSpeedMaxZ = 0.1f;
		g.tEmitter.startGravity = 0;
		g.tEmitter.endGravity = 0;
		g.tEmitter.lifeMin = 1000;
		g.tEmitter.lifeMax = 2000;
		g.tEmitter.alphaStartMin = 40;
		g.tEmitter.alphaStartMax = 75;
		g.tEmitter.alphaEndMin = 0;
		g.tEmitter.alphaEndMax = 0;
		g.tEmitter.frequency = 25;
		ravey_particles_add_emitter ( );
	}

return;

}

void mp_NearOtherPlayers ( void )
{

	for ( t.c = 0 ; t.c<=  MP_MAX_NUMBER_OF_PLAYERS-1; t.c++ )
	{
		if (  t.c  !=  g.mp.me ) 
		{
				t.tobj = t.entityelement[t.mp_playerEntityID[t.c]].obj;
				if (  t.tobj > 0 ) 
				{
					if (  ObjectExist(t.tobj) ) 
					{
						if (  SteamGetPlayerAlive(t.c)  ==  1 ) 
						{
							t.tplrproxx_f=CameraPositionX()-ObjectPositionX(t.tobj);
							if (  g.mp.crouchOn  ==  0 ) 
							{
								t.tplrproyy_f=(CameraPositionY()-64)-ObjectPositionY(t.tobj);
							}
							else
							{
								t.tplrproyy_f=(CameraPositionY()-64+30)-ObjectPositionY(t.tobj);
							}
							t.tplrproxz_f=CameraPositionZ()-ObjectPositionZ(t.tobj);
							t.tplrproxd_f=Sqrt(abs(t.tplrproxx_f*t.tplrproxx_f)+abs(t.tplrproyy_f*t.tplrproyy_f)+abs(t.tplrproxz_f*t.tplrproxz_f));
							t.tplrproxa_f=atan2deg(t.tplrproxx_f,t.tplrproxz_f);
							if (  t.tplrproxd_f<50.0 ) 
							{
								t.playercontrol.pushforce_f=0.5;
								t.playercontrol.pushangle_f=t.tplrproxa_f;
							}
						}
					}
				}
		}
	}

return;

}

void mp_check_respawn_objects ( void )
{
	t.tTime = Timer();
	for ( t.i = 0 ; t.i<=  MP_RESPAWN_TIME_OBJECT_LIST_SIZE; t.i++ )
	{
			if (  t.mp_respawn_timed[t.i].inuse  ==  1 ) 
			{
				if (  t.tTime - t.mp_respawn_timed[t.i].time > MP_RESPAWN_TIME_DELAY ) 
				{
					t.mp_respawn_timed[t.i].inuse = 0;

					t.e = t.mp_respawn_timed[t.i].e;
					t.entityelement[t.e].active = 1;
					entity_lua_spawn ( );
					entity_lua_collisionon ( );
					t.entityelement[t.e].activated = 0;
					t.entityelement[t.e].collected = 0;
					StopObject (  t.entityelement[t.e].obj );
					SetObjectFrame (  t.entityelement[t.e].obj,0 );
					ShowObject (  t.entityelement[t.e].obj );

				}
			}
	}
return;

}

void mp_checkForEveryoneLeft ( void )
{
		if (  g.mp.howmanyjoinedatstart > 1 ) 
		{
			t.tsteamhowmanynow = 0;
			for ( t.tcount = 0 ; t.tcount<=  MP_MAX_NUMBER_OF_PLAYERS-1; t.tcount++ )
			{
				t.tname_s = SteamGetOtherPlayerName(t.tcount);
				if (  t.tname_s != "Player"  )  ++t.tsteamhowmanynow;
			}

			if (  t.tsteamhowmanynow  <= 1 ) 
			{
				t.tsteamlostconnectioncustommessage_s = "Everyone else left the game! (Code MP014)";
				g.mp.backtoeditorforyou = 1;
				mp_lostConnection ( );
				return;
			}
		}
return;

}

void mp_lostConnection ( void )
{
		t.tTime = Timer();
		editor_hideall3d ( );
		SetDir (  cstr(g.fpscrootdir_s + "\\Files").Get() );
		if (  t.tsteamconnectionlostmessage_s  ==  "GAMEOVER"  )  g.mp.backtoeditorforyou  =  1;
		t.tsteamconnectionlostmessage_s = "Lost connection to server";
		if (  t.tsteamlostconnectioncustommessage_s != ""  )  t.tsteamconnectionlostmessage_s  =  t.tsteamlostconnectioncustommessage_s;
		while (  Timer() - t.tTime < 5000 ) 
		{
			Cls (  );
			mp_text(-1,30,3,t.tsteamconnectionlostmessage_s.Get());
			if (  t.tsteamconnectionlostmessage_s  ==  "Could not build workshop item (Error MP015)" ) 
			{
				mp_text(-1,40,3,"The workshop item did not upload to Steam");
				mp_text(-1,45,3,"Please t.try again in t.a few moments.");
				mp_text(-1,50,3,"If the problem persists t.try closing");
				mp_text(-1,55,3,"Game Guru and restarting Steam.");
			}
			SteamLoop (  );
			Sync (  );
		}
		t.tsteamlostconnectioncustommessage_s = "";
//mp_free_game ( );
		mp_setMessage ( );
		if (  g.mp.mode  ==  MP_IN_GAME_CLIENT || g.mp.mode  ==  MP_IN_GAME_SERVER || g.mp.backtoeditorforyou > 0 ) 
		{
			mp_resetGameStats ( );
			if (  g.mp.backtoeditorforyou  !=  2 ) 
			{
				mp_setLuaResetStats ( );
			}
			else
			{
				g.mp.goBackToEditor = 1;
			}
		}
		g.mp.backtoeditorforyou = 0;
		mp_resetGameStats ( );
		mp_quitGame ( );
}

void mp_gameLoop ( void )
{

//  check we have finished loading, if not exit out
if (  g.mp.finishedLoadingMap  ==  0  )  return;

//  HideMouse (  when menu finished )
if (  t.thaveShownMouse >0 ) 
{
	game_hidemouse ( );
	--t.thaveShownMouse;
}

mp_updateAIForCOOP ( );
mp_howManyEnemiesLeftToKill ( );

//  some debug stuff
// `print "destroycount = " + Str(tempsteamdestroycount)

// `if UpKey() then tempsteamdestroycount  ==  0

// `if guntimercount  ==  0 then guntimercount  ==  6

//  `print guntimercount

//  `print ttemppotato

//  `print gunmode


	mp_NearOtherPlayers ( );

	//  if we have lost connection, head back to main menu
	t.tconnectionStatus = SteamGetClientServerConnectionStatus();
	if (  t.tconnectionStatus  ==  0 ) 
	{
		t.tsteamconnectionlostmessage_s = "GAMEOVER";
		mp_lostConnection ( );
		return;
	}
	if (  t.tconnectionStatus  ==  2 ) 
	{
		t.tsteamconnectionlostmessage_s = "GAMEOVER";
		t.tsteamlostconnectioncustommessage_s = "Game Over. The host closed the server.";
		mp_lostConnection ( );
		return;
	}

	mp_lua ( );
	mp_setLuaPlayerNames ( );
	mp_check_respawn_objects ( );

//mp_checkVoiceChat ( );

	if (  Timer() - g.mp.showscoresdelay > 2000 ) 
	{
		if (  KeyState(g.keymap[15])  ==  1 && g.mp.chaton  ==  0 ) 
		{
			t.tnothing = LuaExecute("mp_showscores = 1");
			g.mp.showscoresdelay = Timer();
		}
		else
		{
			t.tnothing = LuaExecute("mp_showscores = 0");
			g.mp.showscoresdelay = -2000;
		}
	}

	//  Find out which index we are (server will always be 0)
	g.mp.me = SteamGetMyPlayerIndex();
	//  Hide our own player model but show everyone elses
	//  TO DO; if a player has d/c or never joined, need to hide their model rather than show a zombie standing there doing nothing
	for ( t.a = 0 ; t.a<=  MP_MAX_NUMBER_OF_PLAYERS-1; t.a++ )
	{
			if (  t.mp_playingRagdoll[t.a]  ==  1 && SteamGetPlayerAlive(t.a)  ==  1 ) 
			{
				t.mp_playingRagdoll[t.a] = 0;
				t.tphyobj=t.entityelement[t.mp_playerEntityID[t.a]].obj;
				ragdoll_destroy ( );
				RotateObject (  t.entityelement[t.mp_playerEntityID[t.a]].obj,0,180,0 );
				FixObjectPivot (  t.entityelement[t.mp_playerEntityID[t.a]].obj );
				t.e = t.mp_playerEntityID[t.a];
				t.entityelement[t.e].health=g.mp.maxHealth;
				//  set appearance back to default so they repick the gun up they had before
				t.mp_oldAppearance[t.a] = 0;
				t.mp_playingAnimation[t.a] = MP_ANIMATION_NONE;
			}
			if (  t.a  ==  g.mp.me ) 
			{
				if (  t.entityelement[t.mp_playerEntityID[g.mp.me]].obj > 0 ) 
				{
					if (  ObjectExist(t.entityelement[t.mp_playerEntityID[g.mp.me]].obj) ) 
					{
						HideObject (  t.entityelement[t.mp_playerEntityID[g.mp.me]].obj );
					}
				}
			}
			else
			{
				if (  t.entityelement[t.mp_playerEntityID[t.a]].obj > 0 ) 
				{
					if (  ObjectExist(t.entityelement[t.mp_playerEntityID[t.a]].obj) ) 
					{
						if (  t.mp_forcePosition[t.a] > 0 && SteamGetPlayerAlive(t.a)  ==  1 ) 
						{
							t.mp_playerHasSpawned[t.a]=1;
							HideObject (  t.entityelement[t.mp_playerEntityID[t.a]].obj );
							if (  t.entityelement[t.mp_playerEntityID[t.a]].attachmentobj > 0 ) 
							{
								if (  ObjectExist(t.entityelement[t.mp_playerEntityID[t.a]].attachmentobj)  ==  1  )  HideObject (  t.entityelement[t.mp_playerEntityID[t.a]].attachmentobj );
							}
						}
						else
						{
							if (  t.mp_playerHasSpawned[t.a]  ==  1 ) 
							{
								ShowObject (  t.entityelement[t.mp_playerEntityID[t.a]].obj );
								if (  t.entityelement[t.mp_playerEntityID[t.a]].attachmentobj > 0 ) 
								{
									if (  ObjectExist(t.entityelement[t.mp_playerEntityID[t.a]].attachmentobj)  ==  1  )  ShowObject (  t.entityelement[t.mp_playerEntityID[t.a]].attachmentobj );
								}
							}
							else
							{
								HideObject (  t.entityelement[t.mp_playerEntityID[t.a]].obj );
								if (  t.entityelement[t.mp_playerEntityID[t.a]].attachmentobj > 0 ) 
								{
									if (  ObjectExist(t.entityelement[t.mp_playerEntityID[t.a]].attachmentobj)  ==  1  )  HideObject (  t.entityelement[t.mp_playerEntityID[t.a]].attachmentobj );
								}
							}
						}
					}
				}
			}
	}

	//  Player is respawning or dead
	t.characterkitcontrol.showmyhead = 0;
	if (  t.mp_health[g.mp.me]  <=  0 ) 
	{
		t.tTime = Timer();
		if (  t.tTime - g.mp.lastSendAliveTime > MP_ALIVE_UPDATE_DELAY ) 
		{
			g.mp.lastSendAliveTime = t.tTime;
			SteamSetPlayerAlive (  0 );
		}
		mp_showdeath ( );
		mp_respawn ( );

		mp_updatePlayerPositions ( );
		mp_updatePlayerNamePlates ( );
		mp_updatePlayerAnimations ( );
		mp_delete_entities ( );
		mp_loop ( );
		mp_check_for_attachments ( );
		mp_update_all_projectiles ( );
		if (  g.mp.gameAlreadySpawnedBefore  ==  0 ) 
		{
			mp_dontShowOtherPlayers ( );
		}

		if (  t.mp_health[g.mp.me] > 0  )  g.mp.lastSendAliveTime  =  0;
		return;
	}

	//  Player is alive
	t.tTime = Timer();
	if (  t.tTime - g.mp.lastSendAliveTime > MP_ALIVE_UPDATE_DELAY ) 
	{
		g.mp.lastSendAliveTime = t.tTime;
		SteamSetPlayerAlive (  1 );
	}
	mp_update_player ( );
	mp_updatePlayerPositions ( );
	mp_updatePlayerInput ( );
	mp_updatePlayerNamePlates ( );
	mp_updatePlayerAnimations ( );
	mp_delete_entities ( );
	mp_loop ( );
	mp_server_message ( );
	mp_check_for_attachments ( );
	mp_update_all_projectiles ( );

	if ( g.mp.endplay == 1 ) mp_ending_game ( );

	if (  t.mp_health[g.mp.me]  <=  0  )  g.mp.lastSendAliveTime  =  0;

	t.tTime = Timer();

if ( g.mp.isGameHost == 1 ) mp_checkForEveryoneLeft ( );

return;

}

//  used when restarting a match so you don't see everyone dropping out of the sky
void mp_dontShowOtherPlayers ( void )
{

	for ( t.a = 0 ; t.a<=  MP_MAX_NUMBER_OF_PLAYERS-1; t.a++ )
	{
		if (  t.a  !=  g.mp.me ) 
		{
			if (  t.entityelement[t.mp_playerEntityID[t.a]].obj > 0 ) 
			{
				if (  ObjectExist(t.entityelement[t.mp_playerEntityID[t.a]].obj) ) 
				{
					PositionObject (  t.entityelement[t.mp_playerEntityID[t.a]].obj,-100000,-100000,-100000 );
				}
			}
		}
	}

return;
	
}

void mp_ending_game ( void )
{
	PositionCamera (  25500,2000,25500 );
	RotateCamera (  90,t.tendofgamerotate_f,0 );
	t.tendofgamerotate_f = t.tendofgamerotate_f + (0.25*g.timeelapsed_f);
	for ( t.a = 0 ; t.a<=  MP_MAX_NUMBER_OF_PLAYERS-1; t.a++ )
	{
		t.tobj=t.entityelement[t.mp_playerEntityID[t.a]].obj;
		if (  t.tobj > 0 ) 
		{
				if (  ObjectExist(t.tobj) ) 
				{
					PositionObject (  t.tobj, t.terrain.playerx_f, t.terrain.playery_f-20, t.terrain.playerz_f );
					HideObject (  t.tobj );
				}
		}
	}
return;

}

void mp_free_game ( void )
{
	if (  t.tspritetouse > 0 ) 
	{
		if (  SpriteExist(t.tspritetouse)  ==  1  )  DeleteSprite (  t.tspritetouse );
		t.tspritetouse = 0;
	}

	if (  g.mp.coop  ==  1 && g.mp.originalEntitycount > 0 ) 
	{
		UnDim (  t.steamStoreentityelement );
		g.mp.originalEntitycount = 0;
	}

	if (  g.mp.gunCount > 0 ) 
	{
		for ( t.i = 0 ; t.i<=  g.mp.gunCount-1; t.i++ )
		{
			if (  t.mp_gunobj[t.i] > 0 ) 
			{
				if (  ObjectExist(t.mp_gunobj[t.i])  )  DeleteObject (  t.mp_gunobj[t.i] );
			}
		}
	}

	g.mp.gunCount = 0;
	for ( t.i = 0 ; t.i<=  599; t.i++ )
	{
		if (  ObjectExist (g.steamplayermodelsoffset+t.i)  ==  1  )  DeleteObject (  g.steamplayermodelsoffset+t.i ) ;
	}

	for ( t.tbulletloop = 0 ; t.tbulletloop<=  159; t.tbulletloop++ )
	{

		t.tSteamSoundID = g.steamsoundoffset+200+t.tbulletloop;
		if (  SoundExist(t.tSteamSoundID)  ==  1 ) 
		{
			if (  SoundPlaying(t.tSteamSoundID)  ==  0  )  StopSound(t.tSteamSoundID);
			DeleteSound (  t.tSteamSoundID );
		}

		t.tSteamSoundID = g.steamsoundoffset+t.tbulletloop;
		if (  SoundExist(t.tSteamSoundID)  ==  1 ) 
		{
			if (  SoundPlaying(t.tSteamSoundID)  ==  0  )  StopSound(t.tSteamSoundID);
			if (  SoundLooping(t.tSteamSoundID)  ==  0  )  StopSound(t.tSteamSoundID);
			DeleteSound (  t.tSteamSoundID );
		}

	}

	for ( t.i = 0 ; t.i<=  MP_MAX_NUMBER_OF_PLAYERS-1; t.i++ )
	{
		if (  t.mp_jetpackparticles[t.i]  !=  -1 ) 
		{
			t.tRaveyParticlesEmitterID=t.mp_jetpackparticles[t.i];
			ravey_particles_delete_emitter ( );
			t.mp_jetpackparticles[t.i] = -1;
		}
	}

	mp_resetGameStats ( );

	SteamWorkshopModeOff (  );

	g.mp.message = "";
	g.mp.messageTime = 0;

	t.game.gameloop = 0;

	if (  ImageExist(g.panelimageoffset+10)  )  DeleteImage (  g.panelimageoffset+10 );
	if (  SpriteExist(g.steamchatpanelsprite)  ==  1  )  DeleteSprite (  g.steamchatpanelsprite );

return;

//  needs tlobbytring$ to be set to the lobby name
}

void mp_subbedToItem ( void )
{
	for ( t.tloop = 0 ; t.tloop<=  20; t.tloop++ )
	{
		if (  t.mp_subbedItems[t.tloop]  ==  "" ) 
		{
			t.mp_subbedItems[t.tloop] = t.tlobbytring_s;
			break;
		}
	}
return;

//  needs tlobbytring$ to be set to the lobby name
}

void mp_checkItemSubbed ( void )
{
	for ( t.tloop = 0 ; t.tloop<=  20; t.tloop++ )
	{
		if (  t.mp_subbedItems[t.tloop]  ==  t.tlobbytring_s && t.tlobbytring_s != "" ) 
		{
			if (  Timer() - t.tsteaminstallingdotstime > 150 ) 
			{
				t.tsteaminstallingdotstime = Timer();
				t.tsteamInstallingDots_s = t.tsteamInstallingDots_s + ".";
				if (  Len(t.tsteamInstallingDots_s.Get()) > 3  )  t.tsteamInstallingDots_s  =  "";
			}
			t.tlobbytring_s = t.tlobbytring_s + " - Installing." + t.tsteamInstallingDots_s;
			break;
		}
	}
return;

}

void mp_resetGameStats ( void )
{

	mp_nukeTestmap ( );
	//if (  FileExist( cstr(g.fpscrootdir_s+"\\Files\\editors\\gridedit\\__multiplayerlevel__.fpm").Get())  )  DeleteAFile (  cstr(g.fpscrootdir_s+"\\Files\\editors\\gridedit\\__multiplayerlevel__.fpm").Get() );
	//if (  FileExist( cstr(g.fpscrootdir_s+"\\Files\\editors\\gridedit\\__multiplayerworkshopitemid__.dat").Get())  )  DeleteAFile (  cstr(g.fpscrootdir_s+"\\Files\\editors\\gridedit\\__multiplayerworkshopitemid__.dat").Get() );
	cstr mlevel_s = g.mysystem.editorsGrideditAbs_s + "__multiplayerlevel__.fpm";
	if ( FileExist( mlevel_s.Get())  )  DeleteAFile ( mlevel_s.Get() );
	cstr mlevelworkshop_s = g.mysystem.editorsGrideditAbs_s + "__multiplayerworkshopitemid__.dat";
	if ( FileExist( mlevelworkshop_s.Get())  )  DeleteAFile ( mlevelworkshop_s.Get() );

	//  empty messages
	for ( t.tloop = 0 ; t.tloop<=  MP_MAX_CHAT_LINES-1; t.tloop++ )
	{
		t.mp_chat[t.tloop] = "";
	}

	for ( t.e = 1 ; t.e<=  g.entityelementlist; t.e++ )
	{
			t.entityelement[t.e].mp_networkkill = 0;
	}

	for ( t.tloop = 0 ; t.tloop<=  20; t.tloop++ )
	{
		t.mp_subbedItems[t.tloop] = "";
	}

	if ( SteamGetPlayerName() != NULL )
	{
		g.mp.playerName = SteamGetPlayerName();
		g.mp.playerID = SteamGetPlayerID();
	}

	g.mp.mode = MP_MODE_MAIN_MENU;
	g.mp.launchServer = 0;
	g.mp.maxHealth = 0;
	g.mp.isLobbyCreated = 0;
	g.mp.isServerCreated = 0;
	g.mp.isGameHost= 0;
	g.mp.voiceChatOn = 0;
	g.mp.lobbycount = 0;
	g.mp.lobbyscrollbarOn = 0;
	g.mp.gameAlreadySpawnedBefore = 0;
	g.mp.killedByPlayer = 0;
	g.mp.previousMessage_s = "";
	g.mp.syncedWithServer = 0;
	g.mp.syncedWithServerMode = 0;
	g.mp.oldtime = 0;
	g.mp.me = 0;
	g.mp.playedMyDeathAnim = 0;
	g.mp.fileLoaded = 0;
	g.mp.playGame = 0;
	g.mp.oldSpawnTimeLeft = 0;
	g.mp.respawnLeft = 0;
	g.mp.crouchOn = 0;
	g.mp.meleeOn = 0;
	g.mp.isAnimating = 0;
	g.mp.okayToLoadLevel = 0;
	g.mp.iHaveSaidIAmReady = 0;
	g.mp.attachmentcount = 0;
	g.mp.gunCount = 0;
	g.mp.gunid = 0;
	g.mp.lastSendTime = 0;
	g.mp.lastSendTimeAppearance = 0;
	g.mp.appearance = 0;
	g.mp.dyingTime = 0;
	g.mp.spawnrnd = -1;
	g.mp.reloading = 0;
	g.mp.syncedWithServer = 0;
	g.mp.sentreadytime = 0;
	g.mp.AttemptedToJoinLobbyTime = 0;
	g.mp.lastSendProjectileTime = 0;
	g.mp.dontApplyDamage = 0;
	g.mp.ragdollon = 0;
	g.mp.spectatorfollowdistance = 200.0;
	g.mp.ignoreDamageToEntity = 0;
	g.mp.endplay = 0;
	g.mp.destroyedObjectCount = 0;
	g.mp.message = "";
	g.mp.messageTime = 0;
	g.mp.oldfootfloortime = 0;
	g.mp.footfloor = 0;
	g.mp.resetcore = 0;
	g.mp.levelContainsCustomContent = 0;
	g.mp.workshopid = "0";
	g.mp.initialSpawnmoveDownCharacterFlag=1;
	g.mp.usersInServersLobbyAtServerCreation = 0;
	g.mp.dontDrawTitles = 0;
	g.mp.haveshowndeath = 0;
	g.mp.checkiflobbiesavailablemode = 0;
	g.mp.noplayermovement = 0;
	g.mp.team = 0;
	g.mp.friendlyfireoff = 0;
	g.mp.nameplatesOff = 0;
	g.mp.damageWasFromAI = 0;
	g.mp.haveSentMyAvatar = 0;
	g.mp.myOriginalSpawnPoint = -1;
	g.mp.realfirsttimespawn = 1;

	for ( t.tc = 0 ; t.tc<=  MP_MAX_NUMBER_OF_PLAYERS-1; t.tc++ )
	{
		t.mp_kills[t.tc] = 0;
		t.mp_deaths[t.tc] = 0;
		t.mp_reload[t.tc] = 0;
		t.mp_playerShooting[t.tc] = 0;
		t.mp_playerAttachmentIndex[t.tc] = 0;
		t.mp_playerIsRagdoll[t.tc] = 0;
		t.mp_playerAttachmentObject[t.tc] = 0;
		t.mp_playerHasSpawned[t.tc] = 0;
		t.mp_oldAppearance[t.tc] = 0;
		t.mp_playingAnimation[t.tc] = 0;
		t.mp_playingRagdoll[t.tc] = 0;
		t.mp_oldplayerx[t.tc] = 0;
		t.mp_oldplayery[t.tc] = 0;
		t.mp_oldplayerz[t.tc] = 0;
		t.mp_meleePlaying[t.tc] = 0;

		t.mp_isDying[t.tc] = 0;
		t.mp_jetpackOn[t.tc] = 0;
		t.mp_lobbies_s[t.tc] = "";
		t.mp_playerEntityID[t.tc] = 0;
		t.mp_forcePosition[t.tc] = 0;
		t.mp_health[t.tc] = 0;
		t.mp_lastIdleReset[t.tc] = 1;
		t.mp_jetpackparticles[t.tc] = -1;
		t.mp_joined[t.tc] = "";
	}

	t.twhichteam = 1;
	for ( t.tc = 0 ; t.tc<=  MP_MAX_NUMBER_OF_PLAYERS-1; t.tc++ )
	{
		t.twhichteam = 1-t.twhichteam;
		t.mp_team[t.tc] = t.twhichteam;
	}

	for ( t.tc = 0 ; t.tc<=  99; t.tc++ )
	{
		t.mp_attachmentobjects[t.tc] = 0;
		t.mp_gunobj[t.tc] = 0;
		t.mp_gunname[t.tc] = "";
	}

	for ( t.tc = 0 ; t.tc<=  79; t.tc++ )
	{
		t.mp_bullets[t.tc].on = 0;
		t.mp_bullets[t.tc].particles = -1;
		t.mp_bullets[t.tc].sound = 0;
	}

	for ( t.i = 0 ; t.i<=  MP_RESPAWN_TIME_OBJECT_LIST_SIZE; t.i++ )
	{
		t.mp_respawn_timed[t.i].inuse = 0;
	}
	
	t.characterkitcontrol.showmyhead = 0;
	

return;

}

void mp_update_all_projectiles ( void )
{
t.debugHowManyInUse = 0;
	for ( t.tbulletloop = 0 ; t.tbulletloop<=  159; t.tbulletloop++ )
	{
			if (  SteamGetBulletOn(t.tbulletloop)  ==  0 ) 
			{
				//  clean up particles
				if (  t.mp_bullets[t.tbulletloop].particles  !=  -1 ) 
				{
						t.tRaveyParticlesEmitterID=t.mp_bullets[t.tbulletloop].particles;
						ravey_particles_delete_emitter ( );
						t.mp_bullets[t.tbulletloop].particles=-1;
				}
			}
			if (  t.tbulletloop < g.mp.me*20 || t.tbulletloop > (g.mp.me*20)+19 ) 
			{
				t.tSteamSoundID = g.steamsoundoffset+t.tbulletloop;
				if (  SoundExist(t.tSteamSoundID)  ==  1 ) 
				{
					if (  SoundLooping(t.tSteamSoundID)  ==  0 ) 
					{
						DeleteSound (  t.tSteamSoundID );
						t.mp_bullets[t.tbulletloop].sound = 0;
					}
				}
				t.tSteamSoundID = g.steamsoundoffset+200+t.tbulletloop;
				if (  SoundExist(t.tSteamSoundID)  ==  1 ) 
				{
					if (  SoundPlaying(t.tSteamSoundID)  ==  0  )  DeleteSound (  t.tSteamSoundID );
				}
				if (  SteamGetBulletOn(t.tbulletloop)  ==  1 ) 
				{
					++t.debugHowManyInUse;
					t.tsteamBObj = g.steamplayermodelsoffset+200+t.tbulletloop;
					t.mp_bullets[t.tbulletloop].btype = SteamGetBulletType(t.tbulletloop);
					if (  ObjectExist(t.tsteamBObj)  ==  0 ) 
					{
						t.tfindObj = t.WeaponProjectileBase[t.mp_bullets[t.tbulletloop].btype].baseObj;
						if (  t.tfindObj  !=  0 ) 
						{
							CloneObject (  t.tsteamBObj, t.tfindObj );
						}
						else
						{
							MakeObjectBox (  t.tsteamBObj,20,20,20 );
						}

						//  setup particle emitters for this projectile
						//  but only if entities not set to LOWEST as particle trails are expensive!
						t.mp_bullets[t.tbulletloop].particles = -1;
						t.tokay = 1 ; if (  t.visuals.shaderlevels.entities == 3  )  t.tokay = 0;
						if (  t.WeaponProjectileBase[t.mp_bullets[t.tbulletloop].btype].particleType>0 && t.tokay == 1 ) 
						{
							ravey_particles_get_free_emitter ( );
							if (  t.tResult>0 ) 
							{
								t.tobj = t.tsteamBObj;
								weapon_add_projectile_particles ( );
								t.mp_bullets[t.tbulletloop].particles = t.tResult;
							}
						}

						//  setup sound
						t.mp_bullets[t.tbulletloop].sound = g.steamsoundoffset+t.tbulletloop;
						if (  SoundExist(t.mp_bullets[t.tbulletloop].sound) == 1 ) 
						{
							if (  SoundPlaying(t.mp_bullets[t.tbulletloop].sound)  ==  1  )  StopSound (  t.mp_bullets[t.tbulletloop].sound );
							if (  SoundLooping(t.mp_bullets[t.tbulletloop].sound)  ==  1  )  StopSound (  t.mp_bullets[t.tbulletloop].sound );
							DeleteSound (  t.mp_bullets[t.tbulletloop].sound );
						}
						//  if this projectile has a sound that loops, start it now
						if (  t.WeaponProjectileBase[t.mp_bullets[t.tbulletloop].btype].sound  ==  0  )  t.mp_bullets[t.tbulletloop].sound  =  0;
						if (  t.mp_bullets[t.tbulletloop].sound > 0 ) 
						{
							CloneSound (  t.mp_bullets[t.tbulletloop].sound,t.WeaponProjectileBase[t.mp_bullets[t.tbulletloop].btype].sound );
							if (  t.WeaponProjectileBase[t.mp_bullets[t.tbulletloop].btype].soundLoopFlag  ==  1 ) 
							{
								PositionSound (  t.mp_bullets[t.tbulletloop].sound,SteamGetBulletX(t.tbulletloop), SteamGetBulletY(t.tbulletloop), SteamGetBulletZ(t.tbulletloop) );
								SetSoundSpeed (  t.mp_bullets[t.tbulletloop].sound, t.WeaponProjectileBase[t.mp_bullets[t.tbulletloop].btype].soundDopplerBaseSpeed );
								LoopSound (  t.mp_bullets[t.tbulletloop].sound );
								if (  t.WeaponProjectileBase[t.mp_bullets[t.tbulletloop].btype].soundDopplerFlag  ==  1 ) 
								{
									t.txDist_f = CameraPositionX(0) - SteamGetBulletX(t.tbulletloop);
									t.tyDist_f = CameraPositionY(0) - SteamGetBulletY(t.tbulletloop);
									t.tzDist_f = CameraPositionZ(0) - SteamGetBulletZ(t.tbulletloop);
									t.mp_bullets[t.tbulletloop].soundDistFromPlayer = Sqrt(t.txDist_f*t.txDist_f + t.tyDist_f*t.tyDist_f + t.tzDist_f*t.tzDist_f);
								}
							}
						}

					}
					if (  ObjectExist(t.tsteamBObj)  ==  1 ) 
					{
						PositionObject (  t.tsteamBObj, SteamGetBulletX(t.tbulletloop), SteamGetBulletY(t.tbulletloop), SteamGetBulletZ(t.tbulletloop) );
						RotateObject (  t.tsteamBObj, SteamGetBulletAngleX(t.tbulletloop), SteamGetBulletAngleY(t.tbulletloop), SteamGetBulletAngleZ(t.tbulletloop) );

						//  do we need to reposition the 3D sound?
						t.tSndID = t.mp_bullets[t.tbulletloop].sound;
						if (  t.tSndID > 0 ) 
						{
							if (  t.WeaponProjectileBase[t.mp_bullets[t.tbulletloop].btype].soundLoopFlag  ==  1 ) 
							{
								//  position the sound
								PositionSound (  t.tSndID,SteamGetBulletX(t.tbulletloop), SteamGetBulletY(t.tbulletloop), SteamGetBulletZ(t.tbulletloop) );
								//  calculate and set doppler pitch
								if (  t.WeaponProjectileBase[t.mp_bullets[t.tbulletloop].btype].soundDopplerFlag  ==  1 ) 
								{
									t.tOldDist_f = t.mp_bullets[t.tbulletloop].soundDistFromPlayer;
									t.txDist_f = CameraPositionX(0) - SteamGetBulletX(t.tbulletloop);
									t.tyDist_f = CameraPositionY(0) - SteamGetBulletY(t.tbulletloop);
									t.tzDist_f = CameraPositionZ(0) - SteamGetBulletZ(t.tbulletloop);
									t.mp_bullets[t.tbulletloop].soundDistFromPlayer = Sqrt(t.txDist_f*t.txDist_f + t.tyDist_f*t.tyDist_f + t.tzDist_f*t.tzDist_f);
									t.tDistDiff_f = t.tOldDist_f - t.mp_bullets[t.tbulletloop].soundDistFromPlayer;
									t.tSoundMultiplier_f = 1 + (t.tDistDiff_f/t.ElapsedTime_f)*0.00015;
									if (  t.tSoundMultiplier_f < 0.5  )  t.tSoundMultiplier_f  =  0.5;
									if (  t.tSoundMultiplier_f > 2  )  t.tSoundMultiplier_f  =  2;
									t.mp_bullets[t.tbulletloop].btype = SteamGetBulletType(t.tbulletloop);
									SetSoundSpeed (  t.tSndID, t.WeaponProjectileBase[t.mp_bullets[t.tbulletloop].btype].soundDopplerBaseSpeed * t.tSoundMultiplier_f );
								}
							}
						}

					}
				}
				else
				{
					//  projectile ended, show result

					//  clean up particles
					if (  t.mp_bullets[t.tbulletloop].particles  !=  -1 ) 
					{
							t.tRaveyParticlesEmitterID=t.mp_bullets[t.tbulletloop].particles;
							ravey_particles_delete_emitter ( );
							t.mp_bullets[t.tbulletloop].particles=-1;
					}

					if (  ObjectExist(g.steamplayermodelsoffset+200+t.tbulletloop)  ==  1 ) 
					{

						t.tSteamProjectileType = t.mp_bullets[t.tbulletloop].btype;

						//  stop any looping sound
						if (  t.mp_bullets[t.tbulletloop].sound > 0 ) 
						{
							StopSound (  t.mp_bullets[t.tbulletloop].sound );
							t.mp_bullets[t.tbulletloop].sound = 0;
						}

						t.tsteamBObj = g.steamplayermodelsoffset+200+t.tbulletloop;

						t.tDeathSoundSoundID = t.WeaponProjectileBase[t.tSteamProjectileType].soundDeath;
						if (  t.tDeathSoundSoundID > 0 ) 
						{
							t.tSteamSoundID = g.steamsoundoffset+200+t.tbulletloop;
							if (  SoundExist(t.tSteamSoundID)  ==  0 ) 
							{
								CloneSound (  t.tSteamSoundID,t.tDeathSoundSoundID );
							}
							if (SoundExist(t.tSteamSoundID) == 1) {
								PositionSound(t.tSteamSoundID, ObjectPositionX(t.tsteamBObj), ObjectPositionY(t.tsteamBObj), ObjectPositionZ(t.tsteamBObj));
								SetSoundSpeed(t.tSteamSoundID, 38000 + Rnd(8000));
								PlaySound(t.tSteamSoundID);
							}
						}

						t.texplodex_f=ObjectPositionX(t.tsteamBObj);
						t.texplodey_f=ObjectPositionY(t.tsteamBObj);
						t.texplodez_f=ObjectPositionZ(t.tsteamBObj);
//       `texploderadius#=300.0

//       `mp.dontApplyDamage = 1

						explosion_rocket(t.texplodex_f,t.texplodey_f,t.texplodez_f);
//physics_explodesphere ( );
//       `mp.dontApplyDamage = 0

						DeleteObject (  t.tsteamBObj );

					}
				}
			}
	}
	
//  `print "Multiplayer rockets in use: " + Str(debugHowManyInUse)

return;


//Direct Multiplayer Instructions


}

void mp_destroyentity ( void )
{
	//  takes ttte
	SteamDeleteObject (  t.ttte );
return;

}

void mp_refresh ( void )
{
	SteamLoop (  );
return;

}

void mp_setMessage ( void )
{
	//  takes tmsg$
	g.mp.message = t.tmsg_s;
	g.mp.messageTime = Timer();
return;

}

void mp_setMessageDots ( void )
{
	//  takes tmsg$
	g.mp.messageDots = t.tmsg_s;
	g.mp.messageTimeDots = Timer();
return;

}

void mp_message ( void )
{
	if (  g.mp.message  !=  "" ) 
	{
//   `x = (GetDisplayWidth()/2) - (Text (  width(mp.message)/2) )

//   `text x,200,mp.message

		mp_text(-1,15,3,g.mp.message.Get());
		if (  Timer() - g.mp.messageTime > MP_MESSAGE_TIMOUT ) 
		{
			g.mp.message = "";
		}
	}
return;

}

void mp_messageDots ( void )
{
	if (  g.mp.messageDots  !=  "" ) 
	{
//   `x = (GetDisplayWidth()/2) - (Text (  width(mp.message)/2) )

//   `text x,200,mp.message

		mp_textDots(-1,15,3,g.mp.messageDots.Get());
		if (  Timer() - g.mp.messageTimeDots > MP_MESSAGE_TIMOUT ) 
		{
			g.mp.messageDots = "";
		}
	}
return;

}

void mp_update_projectile ( void )
{
	if (  t.tProj > 19  )  return;

	t.tSteamBullet = (g.mp.me*20) + t.tProj;

	t.tTime = Timer();
	if (  t.tTime - t.mp_bullets_send_time[t.tSteamBullet] < MP_PROJECTILE_UPDATE_DELAY && t.tSteamBulletOn  ==  1  )  return;

	t.mp_bullets_send_time[t.tSteamBullet] = t.tTime;
	
	t.mp_bullets[t.tSteamBullet].on = t.tSteamBulletOn;
	SteamSetBullet (  t.tSteamBullet , t.WeaponProjectile[t.tProj].xPos_f, t.WeaponProjectile[t.tProj].yPos_f, t.WeaponProjectile[t.tProj].zPos_f, t.WeaponProjectile[t.tProj].xAng_f, t.WeaponProjectile[t.tProj].yAng_f, t.WeaponProjectile[t.tProj].zAng_f, t.tProjType, t.tSteamBulletOn );

	return;
}

void mp_serverSetLuaGameMode ( void )
{
	SteamSendLua (  MP_LUA_ServerSetLuaGameMode,0,t.v );
return;

}

void mp_setServerTimer ( void )
{
	SteamSendLua (  MP_LUA_SetServerTimer,0,t.v );
return;

}

void mp_serverRespawnAll ( void )
{
	SteamSendLua (  MP_LUA_ServerRespawnAll,0,0 );
	mp_restoreEntities ( );
	mp_setLuaResetStats ( );
	mp_respawnEntities ( );
	t.playercontrol.jetpackhidden=0;
	t.playercontrol.jetpackmode=0;
	physics_no_gun_zoom ( );
	t.aisystem.processplayerlogic=1;
	g.mp.gameAlreadySpawnedBefore = 0;
	t.tindex = g.mp.me+1;

	//  Find start position for player
	t.tfoundone = 0;
	if (  t.mpmultiplayerstart[t.tindex].active == 1 ) 
	{
		t.tfoundone = 1;
	}
	else
	{
		t.tonetotry = t.tindex/2;
		if (  t.tonetotry > 0 ) 
		{
			if (  t.mpmultiplayerstart[t.tonetotry].active == 1 ) 
			{
				t.tfoundone = 1;
				t.tindex = t.tonetotry;
			}
		}
	}
	if (  t.tfoundone  ==  0 ) 
	{
		if (  t.mpmultiplayerstart[1].active == 1 ) 
		{
			t.tindex = 1;
			t.tfoundone = 1;
		}
	}

	if (  t.mpmultiplayerstart[t.tindex].active == 1 ) 
	{

		t.terrain.playerx_f=t.mpmultiplayerstart[t.tindex].x;
		t.terrain.playery_f=t.mpmultiplayerstart[t.tindex].y+20;
		t.terrain.playerz_f=t.mpmultiplayerstart[t.tindex].z;
		t.terrain.playerax_f=0;
		t.terrain.playeray_f=t.mpmultiplayerstart[t.tindex].angle;
		t.terrain.playeraz_f=0;

		g.mp.lastx=t.terrain.playerx_f;
		g.mp.lasty=t.terrain.playery_f;
		g.mp.lastz=t.terrain.playerz_f;
		g.mp.lastangley=t.terrain.playeray_f;
	}
	physics_resetplayer_core ( );
	t.tobj = t.entityelement[t.mp_playerEntityID[g.mp.me]].obj;
	if (  t.tobj > 0 ) 
	{
		PositionObject (  t.tobj, t.terrain.playerx_f, t.terrain.playery_f-70, t.terrain.playerz_f );
	}
	g.mp.gameAlreadySpawnedBefore = 0;
	t.player[t.plrid].health = 0;
	t.mp_health[g.mp.me] = 0;
	g.mp.endplay = 0;
	g.autoloadgun=0 ; gun_change ( );
return;

//  Put entities back to the original "first played" state
}

void mp_restoreEntities ( void )
{
//  `remstart

	for ( t.te = 1 ; t.te<=  g.mp.originalEntitycount; t.te++ )
	{
		t.tentid=t.entityelement[t.te].bankindex;
		if (  t.tentid>0 ) 
		{
			if (  t.entityprofile[t.tentid].ischaracter  ==  1 || t.entityelement[t.te].mp_isLuaChar  ==  1 ) 
			{
//     `entityelement(te) = steamStoreentityelement(te)

				t.entityelement[t.te].x = t.steamStoreentityelement[t.te].x;
				t.entityelement[t.te].y = t.steamStoreentityelement[t.te].y;
				t.entityelement[t.te].z = t.steamStoreentityelement[t.te].z;
				ScaleObject (  t.entityelement[t.te].obj,100,100,100 );
				t.entityelement[t.te].health = 100;
				PositionObject (  t.entityelement[t.te].obj,t.entityelement[t.te].x,t.entityelement[t.te].y,t.entityelement[t.te].z );
				t.entityelement[t.te].mp_coopControlledByPlayer = -1;
				t.entityelement[t.te].mp_updateOn = 0;
				t.entityelement[t.te].active = 1;
				AISetEntityActive (  t.entityelement[t.te].obj,1 );
			}
		}
	}
//  `remend

return;

}

void mp_serverEndPlay ( void )
{
	SteamSendLua (  MP_LUA_ServerEndPlay,0,0 );
	t.playercontrol.jetpackhidden=0;
	t.playercontrol.jetpackmode=0;
	physics_no_gun_zoom ( );
	g.mp.endplay = 1;
	t.aisystem.processplayerlogic = 0;
	g.autoloadgun=0 ; gun_change ( );
return;

}

void mp_setServerKillsToWin ( void )
{
	g.mp.setserverkillstowin = t.v;
return;

}

void mp_networkkill ( void )
{

	//  get damage amount to set it back to 0
	t.tdamage = SteamGetPlayerDamageAmount();

	t.tsteamlastdamageincounter = t.tsteamlastdamageincounter + 1;
	t.tsource = t.entityelement[t.texplodesourceEntity].mp_killedby;
	t.te = t.mp_playerEntityID[t.tsource];
	
	g.mp.killedByPlayerFlag = 1;
	g.mp.playerThatKilledMe = t.tsource;
	t.tsteamforce = 500;
	SteamKilledBy (  g.mp.playerThatKilledMe , CameraPositionX(), CameraPositionY(), CameraPositionZ(), t.tsteamforce, 0 );
	g.mp.dyingTime = Timer();

return;

}

void mp_lobbyListBox ( void )
{
	t.tluaTextCenterX = 0;
	if (  g.mp.listboxmode  ==  0 ) 
	{
		t.tsize = SteamGetLobbyListSize();

		t.tLeft = 5;
		t.tTop = 5;
		t.tRight = (26*10)+10;
		t.tBottom = 98+110+70;
		InkEx ( 20, 20, 20 );// Rgb(20,20,20),Rgb(20,20,20) );
		BoxEx (  t.tLeft,t.tTop,t.tRight,t.tTop );
		InkEx ( 255, 255, 255 );// Rgb (255,255,255),0 );
		LineEx (  t.tLeft,t.tTop,t.tRight,t.tTop );
		LineEx (  t.tLeft,t.tTop,t.tLeft,t.tBottom );
		LineEx (  t.tLeft,t.tBottom,t.tRight,t.tBottom );
		LineEx (  t.tRight,t.tTop,t.tRight,t.tBottom );

		InkEx (  255, 255, 255 );//Rgb ( 255,255,255),0 );
		BoxEx (  20,25,40,45 );
		InkEx (  255, 255, 50 );//Rgb (255,255,50),0 );
		BoxEx (  20,60,40,80 );
		InkEx (  255, 100, 100 );//Rgb (255,100,100),0 );
		BoxEx (  20,195,40,215 );

		t.tluarealcoords = 1;
		mp_textColor(50,20,1,"You can join this Lobby",255,255,255);
		t.tluarealcoords = 1;
		mp_textColor(50,55,1,"Join to subscribe and",255,255,50);
		t.tluarealcoords = 1;
		mp_textColor(50,80,1,"download the content",255,255,50);
		t.tluarealcoords = 1;
		mp_textColor(50,105,1,"for this game. The",255,255,50);
		t.tluarealcoords = 1;
		mp_textColor(50,130,1,"lobby will turn white",255,255,50);
		t.tluarealcoords = 1;
		mp_textColor(50,155,1,"when downloaded",255,255,50);
		t.tluarealcoords = 1;
		mp_textColor(50,190,1,"Please restart",255,100,100);
		t.tluarealcoords = 1;
		mp_textColor(50,215,1,"GameGuru to",255,100,100);
		t.tluarealcoords = 1;
		mp_textColor(50,240,1,"update this game",255,100,100);
	}
	if (  g.mp.listboxmode  ==  1 ) 
	{
		t.tsize = t.tempsteamhowmanyfpmsarethere;
	}
	

	t.tTop_f = 20.0 * (GetDisplayHeight() / 100.0);
	t.tleft_f = 30.0 * (GetDisplayWidth() / 100.0);
	t.tBottom_f = 75.0 * (GetDisplayHeight() / 100.0);
	t.tright_f = 70.0 * (GetDisplayWidth() / 100.0);

	t.tTop = t.tTop_f;
	t.tLeft = t.tleft_f;
	t.tBottom = t.tBottom_f;
	t.tRight = t.tright_f;

	t.tempsteamyminY_f = (GetDisplayHeight() * 0.25) - (GetDisplayHeight() * 0.025);
	t.tempsteamymaxY_f = GetDisplayHeight() * 0.75;
	t.tempsteamyminX_f = GetDisplayWidth() * 0.30;
	t.tempsteamymaxX_f = GetDisplayWidth() * 0.65;
	t.tempsteamselected_f = g.mp.selectedLobby - g.mp.lobbyoffset;
	t.tempmissthisone_f = t.tempsteamselected_f;
	if (  t.tempsteamselected_f  >=  0 && t.tempsteamselected_f  <= 9 ) 
	{
		t.tempsteamselectedY_f = t.tempsteamyminY_f + (t.tempsteamselected_f * (GetDisplayHeight() * 0.05));
		InkEx ( 128, 128, 128 );//  Rgb ( 128,128,128),0 );
		BoxEx (  t.tLeft,t.tempsteamselectedY_f,t.tRight,t.tempsteamselectedY_f+(GetDisplayHeight() * 0.05) );
		if (  t.mc  ==  1 && t.tempsteamoldmc  ==  0  )  g.mp.selectedLobby  =  t.tempsteamselected_f+g.mp.lobbyoffset;
	}

	t.tempsteamyminY_f = (GetDisplayHeight() * 0.25) - (GetDisplayHeight() * 0.025);
	t.tempsteamymaxY_f = GetDisplayHeight() * 0.75;
	t.tempsteamyminX_f = GetDisplayWidth() * 0.30;
	t.tempsteamymaxX_f = GetDisplayWidth() * 0.65;
	if (  t.mx  >=  t.tempsteamyminX_f && t.mx  <=  t.tempsteamymaxX_f ) 
	{
		if (  t.my  >=  t.tempsteamyminY_f && t.my  <=  t.tempsteamymaxY_f ) 
		{
			t.my_f = t.my;
			t.tempsteamselected_f = Floor((t.my_f - t.tempsteamyminY_f) / (GetDisplayHeight() * 0.05));
			if (  t.tempsteamselected_f  >=  0 && t.tempsteamselected_f < 10 && t.tempsteamselected_f  !=  t.tempmissthisone_f ) 
			{
				if (  t.tempsteamselected_f+g.mp.lobbyoffset < t.tsize ) 
				{
					t.tempsteamselectedY_f = t.tempsteamyminY_f + (t.tempsteamselected_f * (GetDisplayHeight() * 0.05));
					InkEx ( 64, 64, 64 );//  Rgb ( 64,64,64),0 );
					BoxEx (  t.tLeft,t.tempsteamselectedY_f,t.tRight,t.tempsteamselectedY_f+(GetDisplayHeight() * 0.05) );
					if (  t.mc  ==  1 && t.tempsteamoldmc  ==  0  )  g.mp.selectedLobby  =  t.tempsteamselected_f+g.mp.lobbyoffset;
				}
			}
		}
	}

	InkEx ( 255, 255, 255 );// Rgb (255,255,255),0 );
	LineEx (  t.tLeft,t.tTop,t.tRight,t.tTop );
	LineEx (  t.tLeft,t.tTop,t.tLeft,t.tBottom );
	LineEx (  t.tLeft,t.tBottom,t.tRight,t.tBottom );
	LineEx (  t.tRight,t.tTop,t.tRight,t.tBottom );

	t.toffx_f = (1.0 * GetDisplayWidth()) / 100.0;
	t.toffx = t.toffx_f;
	InkEx ( 30, 30, 30 );// Rgb ( 30,30,30),0 );
	BoxEx (  t.tRight-(t.toffx*2)-8,t.tTop+1,t.tRight-1,t.tBottom-1 );

	t.tTop += 4;
	t.tBottom -= 4;
	t.tRight -= 4;

	t.toffx_f = (1.0 * GetDisplayWidth()) / 100.0;
	t.toffx = t.toffx_f;
	t.toffy_f = (1.0 * GetDisplayHeight()) / 100.0;
	t.toffy = t.toffy_f;

	InkEx ( 255, 255, 255 );//  Rgb (255,255,255),0 );
	if (  t.mx > t.tLeft && t.mx < t.tRight ) 
	{
		if (  t.my > t.tTop && t.my < t.tTop+(t.toffy_f*2) ) 
		{
			InkEx ( 128, 128, 128 );//  Rgb (128,128,128),0 ) ;
			if (  t.mc  ==  1 ) 
			{
				if (  Timer() - t.tempsteamscrollclicktimer > 100 ) 
				{
					--g.mp.lobbyoffset;
					t.tempsteamscrollclicktimer = Timer();
				}
			}
		}
	}

	LineEx (  t.tRight-t.toffx,t.tTop,t.tRight-(t.toffx*2), t.tTop+(t.toffy*2) );
	LineEx (  t.tRight-t.toffx,t.tTop,t.tRight, t.tTop+(t.toffy*2) );
	LineEx (  t.tRight-(t.toffx*2),t.tTop+(t.toffy*2),t.tRight,t.tTop+(t.toffy*2) );

	InkEx ( 255, 255, 255 );// Rgb (255,255,255),0 ) ;
	if (  t.mx > t.tLeft && t.mx < t.tRight ) 
	{
		if (  t.my > t.tBottom-(t.toffy*2) && t.my < t.tBottom ) 
		{
			InkEx ( 128, 128, 128 );// Rgb (128,128,128),0 ) ;
			if (  t.mc  ==  1 ) 
			{
				if (  Timer() - t.tempsteamscrollclicktimer > 100 ) 
				{
					++g.mp.lobbyoffset;
					t.tempsteamscrollclicktimer = Timer();
				}
			}
		}
	}
	

	LineEx (  t.tRight-t.toffx,t.tBottom,t.tRight-(t.toffx*2), t.tBottom-(t.toffy*2) );
	LineEx (  t.tRight-t.toffx,t.tBottom,t.tRight, t.tBottom-(t.toffy*2) );
	LineEx (  t.tRight-(t.toffx*2),t.tBottom-(t.toffy*2),t.tRight,t.tBottom-(t.toffy*2) );

	if (  g.mp.lobbyscrollbarOn  ==  0 || t.mc  ==  0 ) 
	{
		g.mp.lobbyscrollbarOn = 0;
		t.tboxsize_f = (10.0 * GetDisplayHeight()) / 100.0;
		t.tboxsize = t.tboxsize_f;
		t.tloboffset_f = g.mp.lobbyoffset;
		t.tsize_f = t.tsize;
		t.tboxoffset_f = (t.tloboffset_f / t.tsize_f) * 100.0;
		if (  t.tboxoffset_f < 0.0  )  t.tboxoffset_f  =  0.0;
		if (  t.tboxoffset_f > 100.0  )  t.tboxoffset_f  =  100.0;
		t.tboxoffset_f = (t.tboxoffset_f * (GetDisplayHeight() * 0.42)) / 100.0;
		t.tboxoffset = t.tboxoffset_f;
		t.tboxtop = t.tTop+(t.toffy*2) + t.tboxoffset + 2;
		InkEx ( 255, 255, 255 );//  Rgb (255,255,255),0 ) ;
		if (  t.mx > t.tRight-(t.toffx*2) && t.mx < t.tRight ) 
		{
			if (  t.my > t.tboxtop && t.my < t.tboxtop+t.tboxsize ) 
			{
				InkEx ( 160, 160, 160 );// Rgb (160,160,160),0 ) ;
				if (  t.mc  ==  1 && t.tempsteamoldmc  ==  0 ) 
				{
					g.mp.lobbyscrollbarOn = 1;
					g.mp.lobbyscrollbarOffset = t.tboxtop-t.my;
					g.mp.lobbyscrollbarOldY = t.my;
				}
			}
		}

		BoxEx (  t.tRight-(t.toffx*2),t.tboxtop,t.tRight,t.tboxtop+t.tboxsize );

	}
	else
	{
		g.mp.lobbyscrollbarOldY = t.my;
		t.tboxtop = t.my+g.mp.lobbyscrollbarOffset;
		if (  t.tboxtop < t.tTop+(t.toffy*2)+2  )  t.tboxtop  =  t.tTop+(t.toffy*2)+2;
		if (  t.tboxtop > t.tTop+(t.toffy*2)+2 + ((100.0 * (GetDisplayHeight() * 0.40)) / 100.0)  )  t.tboxtop  =  t.tTop+(t.toffy*2)+2 + ((100.0 * (GetDisplayHeight() * 0.40)) / 100.0);
		t.tboxsize_f = (10.0 * GetDisplayHeight()) / 100.0;
		t.tboxsize = t.tboxsize_f;
		InkEx ( 160, 160, 160 );// Rgb (160,160,160),0 ) ;
		BoxEx (  t.tRight-(t.toffx*2),t.tboxtop,t.tRight,t.tboxtop+t.tboxsize );

		//  update the list to reflect where the bar is
		t.tboxtop_f = t.tboxtop - (t.tTop+(t.toffy*2)+2);
		t.tempsteamperc_f = (t.tboxtop_f / (GetDisplayHeight() * 0.42)) * 100.0;
		if (  t.tempsteamperc_f < 0.0  )  t.tempsteamperc_f  =  0.0;
		if (  t.tempsteamperc_f > 100.0  )  t.tempsteamperc_f  =  100.0;
		t.tsize_f = t.tsize;
		t.tempsteamnewoffset_f = (t.tempsteamperc_f * t.tsize_f) / 100.0;
		g.mp.lobbyoffset = t.tempsteamnewoffset_f;

	}

	if (  g.mp.lobbyoffset > t.tsize-10  )  g.mp.lobbyoffset  =  t.tsize-10;
	if (  g.mp.lobbyoffset < 0  )  g.mp.lobbyoffset  =  0;

	InkEx ( 255, 255, 255 );// Rgb(255,255,255),0 );

	t.tempsteamoldmc = t.mc;

	if (  g.mp.listboxmode  ==  0 ) 
	{
		if (  t.tsize  ==  1 ) 
		{
			t.tstring_s = "1 lobby found";
		}
		else
		{
			t.tstring_s = cstr(cstr(Str(t.tsize)) + " lobbies found");
		}
	}
	if (  g.mp.listboxmode  ==  1 ) 
	{
		t.tstring_s = cstr(cstr(Str(t.tsize)) + " levels found");
	}
	mp_text(-1,15,1,t.tstring_s.Get());

	if (  t.tsize > 0  )  g.mp.lobbycount  =  t.tsize;
	t.teampsteamy = 25;
	t.tlobbycount = 0;
	for ( t.c = 0 ; t.c<=  t.tsize-1; t.c++ )
	{
			if (  t.c  >=  g.mp.lobbyoffset && t.c < g.mp.lobbyoffset+10 ) 
			{
				if (  g.mp.listboxmode  ==  0 ) 
				{
					t.mp_lobbies_s[t.tlobbycount] = SteamGetLobbyListName(t.c);

					if (  cstr(Left(t.mp_lobbies_s[t.tlobbycount].Get(),5))  ==  "Lobby" || Len(t.mp_lobbies_s[t.tlobbycount].Get()) < 8 ) 
					{
						t.mp_lobbies_s[t.tlobbycount] = "Waiting for lobby details...";
					}
					if (  g.mp.selectedLobby  ==  t.c  )  g.mp.selectedLobbyName  =  t.mp_lobbies_s[t.tlobbycount];

					t.tempMPLobbyNameFromList_s = t.mp_lobbies_s[t.tlobbycount];
					mp_canIJoinThisLobby ( );
					t.tsteamstring_s = g.mp.lobbyjoinedname;
					if (  t.tsteamcanjoinlobby  ==  1 ) 
					{
						t.tr = 255;
						t.tg = 255;
						t.tb = 255;
					}
					else
					{
						if (  t.tsteamcanjoinlobby  ==  2 ) 
						{
							t.tr = 255;
							t.tg = 100;
							t.tb = 100;
						}
						else
						{
							t.tr = 255;
							t.tg = 255;
							t.tb = 50;
						}
					}
				}
				if (  g.mp.listboxmode  ==  1 ) 
				{
					t.mp_lobbies_s[t.tlobbycount] = t.tfpmfilelist_s[t.c];
					t.tsteamstring_s = t.mp_lobbies_s[t.tlobbycount];
					t.tr = 255;
					t.tg = 255;
					t.tb = 255;
				}

				t.tlobbytring_s = t.tsteamstring_s;
				if ( t.tsteamcanjoinlobby == 0  ) mp_checkItemSubbed ( );
				mp_textColor(32,t.teampsteamy,1,t.tlobbytring_s.Get(),t.tr,t.tg,t.tb);
				t.teampsteamy += 5;
				++t.tlobbycount;
			}
	}
}

void mp_createLobby ( void )
{
	g.mp.haveToldAboutSolo = 0;
	t.tempsteamhostlobbyname_s = cstr(SteamGetPlayerName()) + cstr("'s Lobby:");
	if (  g.mp.fpmpicked  ==  "Level I just worked on" ) 
	{
		t.tempsteamlevelname_s = cstr(SteamGetPlayerName()) + cstr("'s Level:");
	}
	else
	{
		t.tempsteamlevelname_s = cstr(Left(g.mp.fpmpicked.Get(),Len(g.mp.fpmpicked.Get())-4)) + cstr(":");
	}
	//  grab version number
	if (  g.mp.fpmpicked  ==  "Level I just worked on" ) 
	{
		//t.tempsteammaptocheck_s = g.fpscrootdir_s+"\\Files\\mapbank\\worklevel.dat";
		t.tempsteammaptocheck_s = g.mysystem.mapbankAbs_s + "worklevel.dat";		
	}
	else
	{
		//t.tempsteammaptocheck_s = g.fpscrootdir_s+"\\Files\\mapbank\\"+Left(g.mp.fpmpicked.Get(),Len(g.mp.fpmpicked.Get())-3)+"dat";
		t.tempsteammaptocheck_s = g.mysystem.mapbankAbs_s+Left(g.mp.fpmpicked.Get(),Len(g.mp.fpmpicked.Get())-3)+"dat";
	}

	t.tmphopitemtocheckifchangedandversion_s = t.tempsteammaptocheck_s;
	mp_grabWorkshopChangedFlagAndVersion ( );

	SteamSetLobbyName (  cstr(t.tempsteamhostlobbyname_s+t.tempsteamlevelname_s+g.mp.workshopid+":"+Str(t.tMPshopTheVersionNumber)).Get() );
	SteamCreateLobby (  );
	g.mp.isGameHost = 1;
	g.mp.mode = MP_WAITING_FOR_LOBBY_CREATION;
	t.tempsteamlobbycreationtimeout = Timer();
return;

}

void mp_searchForLobbies ( void )
{
	SteamGetLobbyList (  );
	g.mp.mode = MP_MODE_LOBBY;
	g.mp.isGameHost = 0;
return;

}

void mp_searchForFpms ( void )
{
	g.mp.mode = MP_SERVER_CHOOSING_FPM_TO_USE;
	t.told_s=GetDir();
	//SetDir (  cstr(g.fpscrootdir_s + "\\Files\\mapbank").Get() );
	SetDir ( g.mysystem.mapbankAbs_s.Get() );
	ChecklistForFiles (  );
	Dim (  t.tfpmfilelist_s,ChecklistQuantity( ) );
	t.tempsteamhowmanyfpmsarethere = 0;
	for ( t.c = 1 ; t.c<=  ChecklistQuantity(); t.c++ )
	{
		if (  ChecklistValueA(t.c) == 0 ) 
		{
			t.tfile_s=ChecklistString(t.c);
			if (  t.tfile_s != "." && t.tfile_s != ".." ) 
			{
				if (  cstr(Lower(Right(t.tfile_s.Get(),4)))  ==  ".fpm" ) 
				{
					t.tfpmfilelist_s[t.tempsteamhowmanyfpmsarethere] = t.tfile_s;
					++t.tempsteamhowmanyfpmsarethere;
				}
			}
		}
	}
	SetDir (  t.told_s.Get() );
}

void mp_launchGame ( void )
{
	g.mp.launchServer = 1;
}

void mp_backToStart ( void )
{
	mp_resetGameStats ( );
}

