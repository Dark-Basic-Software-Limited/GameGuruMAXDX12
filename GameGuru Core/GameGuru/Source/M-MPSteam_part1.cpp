void mp_updatePlayerAnimations ( void )
{

	//  Update animations
	for ( t.c = 0 ; t.c<=  MP_MAX_NUMBER_OF_PLAYERS-1; t.c++ )
	{

			t.tobj = t.entityelement[t.mp_playerEntityID[t.c]].obj;

			t.thasNade = 0;
			t.tgunid=t.entityelement[t.mp_playerEntityID[t.c]].eleprof.hasweapon;
			if ( t.gun[t.tgunid].projectileframe != 0 ) t.thasNade = 1;

			t.mp_playerShooting[t.c] = SteamGetShoot(t.c);

			//  if the player is reloading we will try and show it (only works if idle or ducking at present)
			if (  SteamGetPlayerAppearance(t.c)  ==  201  )  t.mp_reload[t.c]  =  1;

			//  update animations
			g.mp.isAnimating = 0;
			if (  SteamGetPlayerAlive(t.c)  ==  1 ) 
			{
					t.spinelimbofcharacter=t.entityprofile[t.entityelement[t.mp_playerEntityID[t.c]].bankindex].spine;
					RotateLimb (  t.tobj,t.spinelimbofcharacter,LimbAngleX( t.tobj,t.spinelimbofcharacter),0,LimbAngleZ( t.tobj,t.spinelimbofcharacter) );

				if (  (SteamGetPlayerAppearance(t.c) < 102 || SteamGetPlayerAppearance(t.c) > 200) ) 
				{
					//  Melee
					if (  SteamGetKeyState(t.c,16)  ==  1 || t.mp_meleePlaying[t.c]  ==  1 ) 
					{
						g.mp.isAnimating = 1;
						if (  t.mp_meleePlaying[t.c]  ==  0 ) 
						{
							t.mp_meleePlaying[t.c] = 1;
						}
						else
						{
							if (  GetPlaying(t.tobj)  ==  0  )  t.mp_meleePlaying[t.c]  =  0;
							if (  GetLooping(t.tobj)  ==  1  )  t.mp_meleePlaying[t.c]  =  0;
						}
					}
					//  Forwards
					if (  SteamGetKeyState(t.c,17)  ==  1 ) 
					{
						g.mp.isAnimating = 1;
						//  are they moving left also
						if (  SteamGetKeyState(t.c,30)  ==  1 ) 
						{
							YRotateObject (  t.tobj, ObjectAngleY(t.tobj) - 45 );
							RotateLimb (  t.tobj,t.spinelimbofcharacter,LimbAngleX(t.tobj,t.spinelimbofcharacter),45,LimbAngleZ(t.tobj,t.spinelimbofcharacter) );
						}
						//  or perhaps they are moving right also
						if (  SteamGetKeyState(t.c,32)  ==  1 ) 
						{
							YRotateObject (  t.tobj, ObjectAngleY(t.tobj) + 45 );
							RotateLimb (  t.tobj,t.spinelimbofcharacter,LimbAngleX(t.tobj,t.spinelimbofcharacter),-45,LimbAngleZ(t.tobj,t.spinelimbofcharacter) );
						}
						if (  SteamGetKeyState(t.c,42)  ==  0 || SteamGetKeyState(t.c,46)  ==  1 ) 
						{
							if (  SteamGetPlayerAppearance(t.c)  ==  101 ) 
							{
								t.entityelement[t.mp_playerEntityID[t.c]].eleprof.animspeed=300;
							}
							else
							{
								t.entityelement[t.mp_playerEntityID[t.c]].eleprof.animspeed=100;
							}
						}
						else
						{
							if (  SteamGetPlayerAppearance(t.c)  ==  101 ) 
							{
								t.entityelement[t.mp_playerEntityID[t.c]].eleprof.animspeed=600;
							}
							else
							{
								t.entityelement[t.mp_playerEntityID[t.c]].eleprof.animspeed=200;
							}
						}
						if (  SteamGetKeyState(t.c,46)  ==  0 ) 
						{
							if (  t.mp_playingAnimation[t.c]  !=  MP_ANIMATION_WALKING ) 
							{

								t.tgunid=t.entityelement[t.mp_playerEntityID[t.c]].eleprof.hasweapon;
								t.tweapstyle=t.gun[t.tgunid].weapontype;
								if (  t.tweapstyle > 5  )  t.tweapstyle  =  1;
								if (  t.entityelement[t.mp_playerEntityID[t.c]].attachmentobj > 0 && t.thasNade  ==  0 ) 
								{
									t.tplaycsioranimindex=t.csi_stoodmoverunANIM[t.tweapstyle];
								}
								else
								{
									t.tplaycsioranimindex=g.csi_unarmedmoverunANIM;
								}
								mp_switchAnim ( );

								if (  SteamGetPlayerAppearance(t.c)  ==  101 ) 
								{
									t.tplaycsioranimindex=g.csi_unarmedANIM0;
									mp_switchAnim ( );
								}
								entity_lua_setanimationframes ( );
								t.e = t.mp_playerEntityID[t.c];
								entity_lua_loopanimation ( );
								g.mp.isAnimating = 1;
								t.mp_playingAnimation[t.c] = MP_ANIMATION_WALKING;
							}
						}
						else
						{
							if (  t.mp_playingAnimation[t.c]  !=  MP_ANIMATION_DUCKINGWALKING ) 
							{

								t.tgunid=t.entityelement[t.mp_playerEntityID[t.c]].eleprof.hasweapon;
								t.tweapstyle=t.gun[t.tgunid].weapontype;
								if (  t.tweapstyle > 5  )  t.tweapstyle  =  1;
								if (  t.tweapstyle  ==  0  )  t.tweapstyle  =  1;
								t.tplaycsioranimindex=t.csi_crouchmoverunANIM[t.tweapstyle];
								mp_switchAnim ( );

								entity_lua_setanimationframes ( );
								t.e = t.mp_playerEntityID[t.c];
								entity_lua_loopanimation ( );
								g.mp.isAnimating = 1;
								t.mp_playingAnimation[t.c] = MP_ANIMATION_DUCKINGWALKING;
							}
						}
					}
					//  Backwards
					if (  SteamGetKeyState(t.c,31)  ==  1 ) 
					{
						g.mp.isAnimating = 1;
						//  are they moving left also
						if (  SteamGetKeyState(t.c,30)  ==  1 ) 
						{
							YRotateObject (  t.tobj, ObjectAngleY(t.tobj) + 45 );
							RotateLimb (  t.tobj,t.spinelimbofcharacter,LimbAngleX(t.tobj,t.spinelimbofcharacter),-45,LimbAngleZ(t.tobj,t.spinelimbofcharacter) );
						}
						//  or perhaps they are moving right also
						if (  SteamGetKeyState(t.c,32)  ==  1 ) 
						{
							YRotateObject (  t.tobj, ObjectAngleY(t.tobj) - 45 );
							RotateLimb (  t.tobj,t.spinelimbofcharacter,LimbAngleX(t.tobj,t.spinelimbofcharacter),45,LimbAngleZ(t.tobj,t.spinelimbofcharacter) );
						}
						if (  SteamGetPlayerAppearance(t.c)  ==  101 ) 
						{
							t.entityelement[t.mp_playerEntityID[t.c]].eleprof.animspeed=-300;
						}
						else
						{
							t.entityelement[t.mp_playerEntityID[t.c]].eleprof.animspeed=-100;
						}
						if (  SteamGetKeyState(t.c,46)  ==  0 ) 
						{
							if (  t.mp_playingAnimation[t.c]  !=  MP_ANIMATION_WALKINGBACKWARDS ) 
							{
								t.tgunid=t.entityelement[t.mp_playerEntityID[t.c]].eleprof.hasweapon;
								t.tweapstyle=t.gun[t.tgunid].weapontype;
								if (  t.tweapstyle > 5  )  t.tweapstyle  =  1;
								if (  t.entityelement[t.mp_playerEntityID[t.c]].attachmentobj > 0 && t.thasNade  ==  0 ) 
								{
									t.tplaycsioranimindex=t.csi_stoodmoverunANIM[t.tweapstyle];
								}
								else
								{
									t.tplaycsioranimindex=g.csi_unarmedmoverunANIM;
								}
								mp_switchAnim ( );

								if (  SteamGetPlayerAppearance(t.c)  ==  101 ) 
								{
									t.tgunid=t.entityelement[t.mp_playerEntityID[t.c]].eleprof.hasweapon;
									t.tweapstyle=t.gun[t.tgunid].weapontype;
									if (  t.tweapstyle > 5  )  t.tweapstyle  =  1;
									t.tplaycsioranimindex=t.csi_stoodnormalANIM[t.tweapstyle];
									mp_switchAnim ( );
								}
								entity_lua_setanimationframes ( );
								t.e = t.mp_playerEntityID[t.c];
								entity_lua_loopanimation ( );
								g.mp.isAnimating = 1;
								t.mp_playingAnimation[t.c] = MP_ANIMATION_WALKINGBACKWARDS;
							}
						}
						else
						{
							if (  t.mp_playingAnimation[t.c]  !=  MP_ANIMATION_DUCKINGWALKINGBACKWARDS ) 
							{

								t.tgunid=t.entityelement[t.mp_playerEntityID[t.c]].eleprof.hasweapon;
								t.tweapstyle=t.gun[t.tgunid].weapontype;
								if (  t.tweapstyle > 5  )  t.tweapstyle  =  1;
								if (  t.tweapstyle  ==  0  )  t.tweapstyle  =  1;
								t.tplaycsioranimindex=t.csi_crouchmoverunANIM[t.tweapstyle];
								mp_switchAnim ( );

								entity_lua_setanimationframes ( );
								t.e = t.mp_playerEntityID[t.c];
								entity_lua_loopanimation ( );
								g.mp.isAnimating = 1;
								t.mp_playingAnimation[t.c] = MP_ANIMATION_DUCKINGWALKINGBACKWARDS;
							}
						}
					}

					//  strafe left
					if (  SteamGetKeyState(t.c,30)  ==  1 ) 
					{
						if (  g.mp.isAnimating  ==  0 ) 
						{
							g.mp.isAnimating = 1;
							if (  SteamGetKeyState(t.c,42)  ==  0 ) 
							{
								t.entityelement[t.mp_playerEntityID[t.c]].eleprof.animspeed=100;
							}
							else
							{
								t.entityelement[t.mp_playerEntityID[t.c]].eleprof.animspeed=150;
							}
							if (  SteamGetKeyState(t.c,46)  ==  1 ) 
							{
								if (  t.mp_playingAnimation[t.c]  !=  MP_ANIMATION_DUCKINGWALKING ) 
								{

								t.tgunid=t.entityelement[t.mp_playerEntityID[t.c]].eleprof.hasweapon;
								t.tweapstyle=t.gun[t.tgunid].weapontype;
								if (  t.tweapstyle > 5  )  t.tweapstyle  =  1;
								if (  t.tweapstyle  ==  0  )  t.tweapstyle  =  1;
								t.tplaycsioranimindex=t.csi_crouchmoveleftANIM[t.tweapstyle];
								mp_switchAnim ( );

									entity_lua_setanimationframes ( );
									t.e = t.mp_playerEntityID[t.c];
									entity_lua_loopanimation ( );
									g.mp.isAnimating = 1;
									t.mp_playingAnimation[t.c] = MP_ANIMATION_DUCKINGWALKING;
								}
							}
							else
							{
								if (  t.mp_playingAnimation[t.c]  !=  MP_ANIMATION_STRAFELEFT ) 
								{
									t.tgunid=t.entityelement[t.mp_playerEntityID[t.c]].eleprof.hasweapon;
									t.tweapstyle=t.gun[t.tgunid].weapontype;
									if (  t.tweapstyle > 5  )  t.tweapstyle  =  1;
									if (  t.entityelement[t.mp_playerEntityID[t.c]].attachmentobj > 0 && t.thasNade  ==  0 ) 
									{
										t.tplaycsioranimindex=t.csi_stoodmoverunleftANIM[t.tweapstyle];
									}
									else
									{
										RotateLimb (  t.tobj,t.spinelimbofcharacter,LimbAngleX(t.tobj,t.spinelimbofcharacter),-45,LimbAngleZ(t.tobj,t.spinelimbofcharacter) );
										t.tplaycsioranimindex=g.csi_unarmedmoverunANIM;
									}
									mp_switchAnim ( );

									entity_lua_setanimationframes ( );
									t.e = t.mp_playerEntityID[t.c];
									entity_lua_loopanimation ( );
									t.mp_playingAnimation[t.c] = MP_ANIMATION_STRAFELEFT;
								}
							}
						}
					}

					//  strafe right
					if (  SteamGetKeyState(t.c,32)  ==  1 ) 
					{
						if (  g.mp.isAnimating  ==  0 ) 
						{
							g.mp.isAnimating = 1;
							if (  SteamGetKeyState(t.c,42)  ==  0 ) 
							{
								t.entityelement[t.mp_playerEntityID[t.c]].eleprof.animspeed=100;
							}
							else
							{
								t.entityelement[t.mp_playerEntityID[t.c]].eleprof.animspeed=150;
							}
							if (  SteamGetKeyState(t.c,46)  ==  1 ) 
							{
								if (  t.mp_playingAnimation[t.c]  !=  MP_ANIMATION_DUCKINGWALKING ) 
								{

									t.tgunid=t.entityelement[t.mp_playerEntityID[t.c]].eleprof.hasweapon;
									t.tweapstyle=t.gun[t.tgunid].weapontype;
									if (  t.tweapstyle > 5  )  t.tweapstyle  =  1;
									if (  t.tweapstyle  ==  0  )  t.tweapstyle  =  1;
									t.tplaycsioranimindex=t.csi_crouchmoverightANIM[t.tweapstyle];
									mp_switchAnim ( );

									entity_lua_setanimationframes ( );
									t.e = t.mp_playerEntityID[t.c];
									entity_lua_loopanimation ( );
									g.mp.isAnimating = 1;
									t.mp_playingAnimation[t.c] = MP_ANIMATION_DUCKINGWALKING;
								}
							}
							else
							{
								if (  t.mp_playingAnimation[t.c]  !=  MP_ANIMATION_STRAFERIGHT ) 
								{

									t.tgunid=t.entityelement[t.mp_playerEntityID[t.c]].eleprof.hasweapon;
									t.tweapstyle=t.gun[t.tgunid].weapontype;
									if (  t.tweapstyle > 5  )  t.tweapstyle  =  1;
									if (  t.entityelement[t.mp_playerEntityID[t.c]].attachmentobj > 0 && t.thasNade  ==  0 ) 
									{
										t.tplaycsioranimindex=t.csi_stoodmoverunrightANIM[t.tweapstyle];
									}
									else
									{
										t.tplaycsioranimindex=g.csi_unarmedmoverunANIM;
									}
									mp_switchAnim ( );

									entity_lua_setanimationframes ( );
									t.e = t.mp_playerEntityID[t.c];
									entity_lua_loopanimation ( );
								
									t.mp_playingAnimation[t.c] = MP_ANIMATION_STRAFERIGHT;
								}
							}
						}
					}

					if (  t.mp_playingAnimation[t.c]  ==  MP_ANIMATION_STRAFELEFT ) 
					{
						if (  t.entityelement[t.mp_playerEntityID[t.c]].attachmentobj  ==  0 ) 
						{
							RotateLimb (  t.tobj,t.spinelimbofcharacter,LimbAngleX(t.tobj,t.spinelimbofcharacter),45,LimbAngleZ(t.tobj,t.spinelimbofcharacter) );
							YRotateObject (  t.tobj, ObjectAngleY(t.tobj) - 45 );
						}
					}

					if (  t.mp_playingAnimation[t.c]  ==  MP_ANIMATION_STRAFERIGHT ) 
					{
						if (  t.entityelement[t.mp_playerEntityID[t.c]].attachmentobj  ==  0 ) 
						{
							RotateLimb (  t.tobj,t.spinelimbofcharacter,LimbAngleX(t.tobj,t.spinelimbofcharacter),-45,LimbAngleZ(t.tobj,t.spinelimbofcharacter) );
							YRotateObject (  t.tobj, ObjectAngleY(t.tobj) + 45 );
						}
					}


					//  Ducking
					if (  SteamGetKeyState(t.c,46)  ==  1 && t.mp_jetpackOn[t.c]  ==  0 ) 
					{
						if (  g.mp.isAnimating  ==  0 && t.mp_reload[t.c]  ==  0 ) 
						{
							g.mp.isAnimating = 1;
							if (  t.mp_playingAnimation[t.c]  !=  MP_ANIMATION_DUCKING ) 
							{

								t.tgunid=t.entityelement[t.mp_playerEntityID[t.c]].eleprof.hasweapon;
								t.tweapstyle=t.gun[t.tgunid].weapontype;
								if (  t.tweapstyle > 5  )  t.tweapstyle  =  1;
								t.tplaycsioranimindex=t.csi_crouchidlenormalANIM1[t.tweapstyle];
								mp_switchAnim ( );

								entity_lua_setanimationframes ( );
								t.e = t.mp_playerEntityID[t.c];
								t.entityelement[t.e].eleprof.animspeed=100;
								entity_lua_playanimation ( );
								g.mp.isAnimating = 1;

								t.mp_playingAnimation[t.c] = MP_ANIMATION_DUCKING;

							}
						}
					}

				}

				if (  t.thasNade  ==  1 ) 
				{
					if (  t.mp_reload[t.c]  ==  1  )  t.mp_reload[t.c]  =  0;
				}

				if (  t.mp_reload[t.c]  ==  1 ) 
				{
						if (  g.mp.isAnimating  ==  0 || t.mp_playingAnimation[t.c]  ==  MP_ANIMATION_DUCKING ) 
						{
							if (  t.mp_playingAnimation[t.c]  !=  MP_ANIMATION_RELOAD ) 
							{

								t.tgunid=t.entityelement[t.mp_playerEntityID[t.c]].eleprof.hasweapon;
								t.tweapstyle=t.gun[t.tgunid].weapontype;
								if (  t.tweapstyle > 5  )  t.tweapstyle  =  1;
								t.tplaycsioranimindex=t.csi_stoodreloadANIM[t.tweapstyle];
								mp_switchAnim ( );

								if (  t.mp_playingAnimation[t.c]  ==  MP_ANIMATION_DUCKING ) 
								{

								t.tgunid=t.entityelement[t.mp_playerEntityID[t.c]].eleprof.hasweapon;
								t.tweapstyle=t.gun[t.tgunid].weapontype;
								if (  t.tweapstyle > 5  )  t.tweapstyle  =  1;
								t.tplaycsioranimindex=t.csi_crouchreloadANIM[t.tweapstyle];
								mp_switchAnim ( );

								}
								entity_lua_setanimationframes ( );
								t.e = t.mp_playerEntityID[t.c];
								t.entityelement[t.e].eleprof.animspeed=200;
								entity_lua_playanimation ( );
								g.mp.isAnimating = 1;

								t.mp_playingAnimation[t.c] = MP_ANIMATION_RELOAD;
							}
						}
						g.mp.isAnimating = 1;
						//  if the reload anim has finished or the player starts shooting, turn reloading off
						if (  GetFrame(t.tobj)  ==  605 || GetFrame(t.tobj)  ==  2010 || t.mp_playerShooting[t.c]  ==  1 ) 
						{
							t.mp_reload[t.c] = 0;
							if (  GetFrame(t.tobj)  ==  2010 ) 
							{
								t.mp_playingAnimation[t.c] = MP_ANIMATION_DUCKING;
							}
							else
							{
								t.mp_playingAnimation[t.c] = MP_ANIMATION_NONE;
								g.mp.isAnimating = 0;
							}
//        `print "DUCKING SPOT 2 <----------------------------------------------"

						}
				}

				t.tjetpacktempanim = 0;
				if (  SteamGetPlayerAppearance(t.c)  ==  102 ) 
				{
					t.tjetpacktempanim = 1;
					if (  t.mp_playingAnimation[t.c]  !=  MP_ANIMATION_IDLE ) 
					{
						t.mp_playingAnimation[t.c] = MP_ANIMATION_NONE;
						g.mp.isAnimating = 0;
					}
				}

				if (  t.thasNade  ==  1 ) 
				{
					if (  t.mp_playingAnimation[t.c]  ==  MP_ANIMATION_IDLE ) 
					{
						if (  t.mp_playerShooting[t.c]  ==  1 ) 
						{
							if (   GetFrame(t.tobj) < 2390 || GetFrame(t.tobj) > 2444 ) 
							{
								t.mp_playingAnimation[t.c] = MP_ANIMATION_NONE;
								g.mp.isAnimating = 0;
							}
						}

						if (  t.mp_playerShooting[t.c]  ==  0 ) 
						{
							if (  GetFrame(t.tobj)  ==  2444 ) 
							{
								SetObjectFrame(t.tobj,2443);
								StopObject (  t.tobj );
								g.mp.isAnimating = 0;
								t.mp_playingAnimation[t.c] = MP_ANIMATION_NONE;
							}
						}

					}
				}

				if (  g.mp.isAnimating  ==  0 ) 
				{
					mp_update_waist_rotation ( );
//      `print "IDLING"

					if (  t.mp_playingAnimation[t.c]  !=  MP_ANIMATION_IDLE ) 
					{
						if (  abs(t.mp_oldplayerx[t.c] - t.entityelement[t.mp_playerEntityID[t.c]].x) < 1.0 || t.tjetpacktempanim  ==  1 ) 
						{
							if (  abs(t.mp_oldplayery[t.c] - t.entityelement[t.mp_playerEntityID[t.c]].y) < 1.0 || t.tjetpacktempanim  ==  1 ) 
							{
								if (  abs(t.mp_oldplayerz[t.c] - t.entityelement[t.mp_playerEntityID[t.c]].z) < 1.0 || t.tjetpacktempanim  ==  1 ) 
								{
									t.tIsThrowingNade = 0;
									t.tgunid=t.entityelement[t.mp_playerEntityID[t.c]].eleprof.hasweapon;
									t.tweapstyle=t.gun[t.tgunid].weapontype;
									if (  t.tweapstyle > 5  )  t.tweapstyle  =  1;
									if (  t.entityelement[t.mp_playerEntityID[t.c]].attachmentobj > 0 && t.thasNade  ==  0 ) 
									{

										t.tplaycsioranimindex=t.csi_stoodnormalANIM[t.tweapstyle];
										mp_switchAnim ( );

									}
									else
									{

										if (  t.thasNade  ==  1 && t.mp_playerShooting[t.c]  ==  1 ) 
										{
											t.ttentid=t.entityelement[t.mp_playerEntityID[t.c]].bankindex;
											t.e=2390;
											t.v=2444;
											entity_lua_setanimationframes ( );
											t.e = t.mp_playerEntityID[t.c];
											t.entityelement[t.e].eleprof.animspeed=200;
											t.tLuaDontSendLua = 1;
											t.q=-1;
											entity_lua_playanimation ( );
											t.tLuaDontSendLua = 0;
											t.tIsThrowingNade = 1;
										}
										else
										{
											t.tgunid=t.entityelement[t.mp_playerEntityID[t.c]].eleprof.hasweapon;
											t.tweapstyle=t.gun[t.tgunid].weapontype;
											if (  t.tweapstyle > 5  )  t.tweapstyle  =  1;
											t.tplaycsioranimindex=g.csi_unarmedANIM0;
											mp_switchAnim ( );
										}

									}
									if (  t.tIsThrowingNade  ==  0 ) 
									{
										entity_lua_setanimationframes ( );
										t.e = t.mp_playerEntityID[t.c];
										t.entityelement[t.e].eleprof.animspeed=100;
										entity_lua_loopanimation ( );
									}
									t.mp_playingAnimation[t.c] = MP_ANIMATION_IDLE;
								}
							}
						}
					}
				}
				else
				{
					//  reset the idle turn if animating
					t.mp_lastIdleReset[t.c] = 1;
//      `print "last idle reset = 1"

				}

			}

			//  

			if (  SteamGetPlayerAlive(t.c)  ==  0 && g.mp.gameAlreadySpawnedBefore  !=  0 ) 
			{
				t.mp_playingAnimation[t.c] = MP_ANIMATION_NONE;
				t.mp_lastIdleReset[t.c] = 1;
				t.mp_forcePosition[t.c] = 1;

				if (  t.mp_jetpackparticles[t.c]  !=  -1 ) 
				{
					t.tRaveyParticlesEmitterID=t.mp_jetpackparticles[t.c];
					ravey_particles_delete_emitter ( );
					t.mp_jetpackparticles[t.c]=-1;
				}

				if (  t.mp_isDying[t.c]  ==  0 && t.mp_playerHasSpawned[t.c]  ==  1 ) 
				{

					t.mp_isDying[t.c] = 1;
					t.mp_playingAnimation[t.c] = MP_ANIMATION_NONE;
					t.spinelimbofcharacter=t.entityprofile[t.entityelement[t.mp_playerEntityID[t.c]].bankindex].spine;
					RotateLimb (  t.tobj,t.spinelimbofcharacter,LimbAngleX( t.tobj,t.spinelimbofcharacter),0,LimbAngleZ( t.tobj,t.spinelimbofcharacter) );
					t.e = t.mp_playerEntityID[t.c];
					if (  ObjectExist(g.steamplayermodelsoffset+t.c+121)  ==  1 ) 
					{
						t.tweight=t.entityelement[t.e].eleprof.phyweight;
						t.tfriction=t.entityelement[t.e].eleprof.phyfriction;
						ODECreateDynamicBox (  g.steamplayermodelsoffset+t.c+121,-1,0,t.tweight,t.tfriction,-1 );
					}

					//  NON-CHARACTER, but can still have ragdoll flagged (like Zombies)
					t.ttentid=t.entityelement[t.e].bankindex;
					t.ttte = t.e;
					t.mp_playingRagdoll[t.c] = 1;
					if (  t.entityelement[t.mp_playerEntityID[t.c]].attachmentobj > 0 ) 
					{
						if (  ObjectExist(t.entityelement[t.mp_playerEntityID[t.c]].attachmentobj)  )  DeleteObject (  t.entityelement[t.mp_playerEntityID[t.c]].attachmentobj );
						t.entityelement[t.mp_playerEntityID[t.c]].attachmentobj = 0;
					}
					t.entityprofile[t.ttentid].ragdoll=1;
					if (  t.entityprofile[t.ttentid].ragdoll == 1 ) 
					{

						//  can only ragdoll clones not instances
						t.tte=t.ttte;
						entity_converttoclone ( );

						//  create ragdoll and stop any further manipulation of the object
						t.tphye=t.ttte;
						t.tphyobj=t.entityelement[t.ttte].obj;
						t.oldc = t.c;
						ragdoll_setcollisionmask ( t.entityelement[t.ttte].eleprof.colondeath );
						ragdoll_create ( );
						t.c = t.oldc;

						/*      
						MoveObjectLeft (  t.entityelement[t.ttte].obj,60 );
						t.x_f = ObjectPositionX(t.entityelement[t.ttte].obj);
						t.y_f = ObjectPositionY(t.entityelement[t.ttte].obj)+50;
						t.z_f = ObjectPositionZ(t.entityelement[t.ttte].obj);
						MoveObjectRight (  t.entityelement[t.ttte].obj,60 );
						t.tforce_f = 1000;
						tlimb = Rnd(6);
						switch (  tlimb ) 
						{
						case 0; tlimb  =  8 ; break 
:
						case 1; tlimb  =  10 ; break 
:
						case 2; tlimb  =  12 ; break 
:
						case 3; tlimb  =  16 ; break 
:
						case 4; tlimb  =  17 ; break 
:
						case 5; tlimb  =  24 ; break 
:
						case 6; tlimb  =  25 ; break 
:
						}						//~       remend
						*/    

						//  use the real raycast if we shot them
//       `if Steam Get Player Killed Source(c) = mp.me

//        `ttx# = brayx2#-brayx1#

//        `tty# = brayy2#-brayy1#

//        `ttz# = brayz2#-brayz1#

//        `ttforce# = tforce#

//        `ttlimb = bulletraylimbhit

//       `else

							//  grab the details from the server if someone else shot them
							t.ttx_f = SteamGetPlayerKilledX(t.c);
							t.tty_f = SteamGetPlayerKilledY(t.c);
							t.ttz_f = SteamGetPlayerKilledZ(t.c);
							t.ttforce_f = SteamGetPlayerKilledForce(t.c);
							t.ttlimb = SteamGetPlayerKilledLimb(t.c);
//       `endif

	
						//  and apply bullet directional force (tforce#=from gun settings)
						t.entityelement[t.ttte].ragdollified=1;
//       `entityelement(ttte).ragdollifiedforcex#=(x#)*0.8

//       `entityelement(ttte).ragdollifiedforcey#=(y#)*1.2

//       `entityelement(ttte).ragdollifiedforcez#=(z#)*0.8

						t.entityelement[t.ttte].ragdollifiedforcex_f=(t.ttx_f)*0.8;
						t.entityelement[t.ttte].ragdollifiedforcey_f=(t.tty_f)*1.2;
						t.entityelement[t.ttte].ragdollifiedforcez_f=(t.ttz_f)*0.8;
						t.entityelement[t.ttte].ragdollifiedforcevalue_f=t.ttforce_f*8000.0;
						t.entityelement[t.ttte].ragdollifiedforcelimb=t.ttlimb;
					}
				}
			}
			else
			{
				if (  t.mp_forcePosition[t.c]  ==  0 ) 
				{
					if (  t.mp_isDying[t.c]  ==  1 ) 
					{
						if (  ObjectExist(g.steamplayermodelsoffset+t.c+121)  ==  1 ) 
						{
							ODEDestroyObject (  g.steamplayermodelsoffset+t.c+121 );
							RotateObject (  g.steamplayermodelsoffset+t.c+121,0,0,0 );
							PositionObject (  g.steamplayermodelsoffset+t.c+121,0,-99999,0 );
							HideObject (  g.steamplayermodelsoffset+t.c+121 );
							t.mp_playingAnimation[t.c] = MP_ANIMATION_NONE;
						}
						t.mp_isDying[t.c] = 0;
					}
				}
			}

			t.mp_oldplayerx[t.c] = t.entityelement[t.mp_playerEntityID[t.c]].x;
			t.mp_oldplayery[t.c] = t.entityelement[t.mp_playerEntityID[t.c]].y;
			t.mp_oldplayerz[t.c] = t.entityelement[t.mp_playerEntityID[t.c]].z;

}

return;

}

void mp_switchAnim ( void )
{
	t.ttentid=t.entityelement[t.mp_playerEntityID[t.c]].bankindex;
	t.q=t.entityprofile[t.ttentid].startofaianim;
	t.e=t.entityanim[t.ttentid][t.q+t.tplaycsioranimindex].start;
	t.v=t.entityanim[t.ttentid][t.q+t.tplaycsioranimindex].finish;
}

void mp_update_waist_rotation ( void )
{
	return;
	t.tobj = t.entityelement[t.mp_playerEntityID[t.c]].obj;

	if (  t.mp_lastIdleReset[t.c]  ==  1 ) 
	{
		t.mp_lastIdleY[t.c] = t.entityelement[t.mp_playerEntityID[t.c]].ry;
		t.mp_lastIdleReset[t.c] = 0;
	}

	t.tDifference_f = t.entityelement[t.mp_playerEntityID[t.c]].ry - t.mp_lastIdleY[t.c];
	t.tAmountToRotateSpine_f = t.tDifference_f;
	t.tAmountToRotateObject_f = 0.0;

	if (  t.tAmountToRotateSpine_f > 60.0 ) 
	{
		t.tAmountToRotateObject_f = t.tAmountToRotateSpine_f - 60.0;
		t.tAmountToRotateSpine_f = 60.0;
	}

	if (  t.tAmountToRotateSpine_f < -60.0 ) 
	{
		t.tAmountToRotateObject_f = t.tAmountToRotateSpine_f + 60.0;
		t.tAmountToRotateSpine_f = -60.0;
	}

	YRotateObject (  t.tobj, t.mp_lastIdleY[t.c]+t.tAmountToRotateObject_f );
	t.mp_lastIdleY[t.c] = t.mp_lastIdleY[t.c]+t.tAmountToRotateObject_f;

	t.spinelimbofcharacter=t.entityprofile[t.entityelement[t.mp_playerEntityID[t.c]].bankindex].spine;
	RotateLimb (  t.tobj,t.spinelimbofcharacter,LimbAngleX( t.tobj,t.spinelimbofcharacter),t.tAmountToRotateSpine_f,LimbAngleZ( t.tobj,t.spinelimbofcharacter) );

	return;

}

void mp_showdeath ( void )
{

	t.characterkitcontrol.showmyhead = 1;
	if (  g.mp.haveshowndeath  ==  0 ) 
	{
		g.mp.haveshowndeath = 1;
		t.tolddeathcamx_f = 0;
		t.tolddeathcamy_f = 0;
		t.tolddeathcamz_f = 0;
		t.tamountToMoveIn_f = 0.0;
		t.tamountToMoveUp_f = 0.0;
		t.tspawninyoffset_f = 0.0;
		t.tshowdeathlockcam = -1;
		g.mp.spectatorfollowdistance = 200;
		t.tdeathamounttotakeoffdistance = 0;
	}

	//  19032015 - 021 - prevent water affect being triggered when in 3rd person
	visuals_underwater_off ( );

		t.tobjtosee = t.entityelement[t.mp_playerEntityID[g.mp.me]].obj;
//   `playercontrol.jetobjtouse=hudlayersbankoffset+1

		t.playercontrol.jetpackhidden=0;
		t.playercontrol.jetpackmode=0;
//   `plrzoomin# = 0

//   `plrzoominchange = 1

//   `gunzoommode = 0

		//  new subroutine so steam can reset zoom in
		physics_no_gun_zoom ( );
		if (  g.mp.endplay  ==  1 && g.mp.respawnLeft > 3 ) 
		{
				g.mp.respawnLeft = 100;
				return;
		}
		//  if dead switch to 3rd person view to see the action
			t.e = t.mp_playerEntityID[g.mp.me];
			t.tobj = t.entityelement[t.mp_playerEntityID[g.mp.me]].obj;
			if (  t.tobj > 0 ) 
			{
				if ( ObjectExist (t.tobj) ) 
				{
//     `if health(mp.me) <= 0

					ShowObject (  t.tobj );
					t.tpe = t.mp_playerEntityID[g.mp.me];
					if (  g.mp.ragdollon  ==  0 ) 
					{
						g.mp.ragdollon = 1;

					if (  g.mp.gameAlreadySpawnedBefore  ==  1 ) 
					{

						//  turn off jetpack sound, turn off particles and reset thrust
						if (  SoundExist(t.playercontrol.soundstartindex+18) == 1  )  StopSound (  t.playercontrol.soundstartindex+18 );
						t.playercontrol.jetpackthrust_f=0.0;
						//  stop particle emitter
						if (  t.playercontrol.jetpackparticleemitterindex>0 ) 
						{
							t.tRaveyParticlesEmitterID=t.playercontrol.jetpackparticleemitterindex;
							ravey_particles_delete_emitter ( );
							t.playercontrol.jetpackparticleemitterindex=0;
						}

						t.spinelimbofcharacter=t.entityprofile[t.entityelement[t.mp_playerEntityID[g.mp.me]].bankindex].spine;
						RotateLimb (  t.tobj,t.spinelimbofcharacter,LimbAngleX( t.tobj,t.spinelimbofcharacter),0,LimbAngleZ( t.tobj,t.spinelimbofcharacter) );
						if (  ObjectExist(g.steamplayermodelsoffset+g.mp.me+121)  ==  1 ) 
						{
							t.tweight=t.entityelement[t.e].eleprof.phyweight;
							t.tfriction=t.entityelement[t.e].eleprof.phyfriction;
							ODECreateDynamicBox (  g.steamplayermodelsoffset+g.mp.me+121,-1,0,t.tweight,t.tfriction,-1 );
						}
						t.tme = g.mp.me;

						//  NON-CHARACTER, but can still have ragdoll flagged (like Zombies)
						t.ttentid=t.entityelement[t.e].bankindex;
						t.ttte = t.e;
						t.mp_playingRagdoll[t.tme] = 1;
						if (  t.entityelement[t.mp_playerEntityID[t.tme]].attachmentobj > 0 ) 
						{
							if (  ObjectExist(t.entityelement[t.mp_playerEntityID[t.tme]].attachmentobj)  )  DeleteObject (  t.entityelement[t.mp_playerEntityID[t.tme]].attachmentobj );
							t.entityelement[t.mp_playerEntityID[t.tme]].attachmentobj = 0;
						}
						t.entityprofile[t.ttentid].ragdoll=1;
						if (  t.entityprofile[t.ttentid].ragdoll == 1 ) 
						{

							//  can only ragdoll clones not instances
							t.tte=t.ttte;
							entity_converttoclone ( );

							//  create ragdoll and stop any further manipulation of the object
							t.tphye=t.ttte;
							t.tphyobj=t.entityelement[t.ttte].obj;
							t.oldc = t.c;
							ragdoll_setcollisionmask ( t.entityelement[t.ttte].eleprof.colondeath );
							ragdoll_create ( );
							t.c = t.oldc;

							//  use the real raycast if we shot them
							if (  SteamGetPlayerDamageSource()  ==  g.mp.me ) 
							{
								t.ttx_f = t.brayx2_f-t.brayx1_f;
								t.tty_f = t.brayy2_f-t.brayy1_f;
								t.ttz_f = t.brayz2_f-t.brayz1_f;
								t.ttforce_f = t.tforce_f;
								t.ttlimb = t.bulletraylimbhit;
							}
							else
							{
								//  grab the details from the server if someone else shot them
								t.ttx_f = SteamGetPlayerDamageX();
								t.tty_f = SteamGetPlayerDamageY();
								t.ttz_f = SteamGetPlayerDamageZ();
								t.ttforce_f = SteamGetPlayerDamageForce();
								t.ttlimb = SteamGetPlayerDamageLimb();
							}
	
							//  and apply bullet directional force (tforce#=from gun settings)
							t.entityelement[t.ttte].ragdollified=1;
//        `entityelement(ttte).ragdollifiedforcex#=(x#)*0.8

//        `entityelement(ttte).ragdollifiedforcey#=(y#)*1.2

//        `entityelement(ttte).ragdollifiedforcez#=(z#)*0.8

							t.entityelement[t.ttte].ragdollifiedforcex_f=(t.ttx_f)*0.8;
							t.entityelement[t.ttte].ragdollifiedforcey_f=(t.tty_f)*1.2;
							t.entityelement[t.ttte].ragdollifiedforcez_f=(t.ttz_f)*0.8;
							t.entityelement[t.ttte].ragdollifiedforcevalue_f=t.ttforce_f*8000.0;
//        `entityelement(ttte).ragdollifiedforcelimb=tlimb

							t.entityelement[t.ttte].ragdollifiedforcelimb=t.ttlimb;
//        `bulletraylimbhit=-1


						}
					}
					}
//entity_updatepos ( );
//entity_lua_rotateupdate ( );
					if (  1 ) 
					{
						t.tsteamlimb=t.entityprofile[t.entityelement[t.tpe].bankindex].spine2;
						if (  g.mp.gameAlreadySpawnedBefore  ==  0 ) 
						{
							if (  g.mp.initialSpawnmoveDownCharacterFlag  ==  1 ) 
							{
								PositionObject (  t.entityelement[t.tpe].obj, ObjectPositionX(t.entityelement[t.tpe].obj), ObjectPositionY(t.entityelement[t.tpe].obj)-50, ObjectPositionZ(t.entityelement[t.tpe].obj) );
								if (  ObjectPositionY(t.entityelement[t.tpe].obj) < BT_GetGroundHeight(t.terrain.TerrainID,ObjectPositionX(t.entityelement[t.tpe].obj),ObjectPositionZ(t.entityelement[t.tpe].obj)) ) 
								{
									PositionObject (  t.entityelement[t.tpe].obj, ObjectPositionX(t.entityelement[t.tpe].obj), BT_GetGroundHeight(t.terrain.TerrainID,ObjectPositionX(t.entityelement[t.tpe].obj),ObjectPositionZ(t.entityelement[t.tpe].obj)) , ObjectPositionZ(t.entityelement[t.tpe].obj) );
								}
								g.mp.initialSpawnmoveDownCharacterFlag = 0;
							}
						}
						t.x_f = LimbPositionX(t.tobjtosee,t.tsteamlimb);
						t.y_f = LimbPositionY(t.tobjtosee,t.tsteamlimb);
						t.z_f = LimbPositionZ(t.tobjtosee,t.tsteamlimb);
						PositionCamera (  t.x_f,t.y_f+100,t.z_f );
						RotateCamera (  0,g.mp.camrotate,0 );
						g.mp.camrotate = g.mp.camrotate + (0.5*g.timeelapsed_f);


						t.x_f = LimbPositionX(t.tobjtosee,t.tsteamlimb);
						t.y_f = LimbPositionY(t.tobjtosee,t.tsteamlimb)+10;
						t.z_f = LimbPositionZ(t.tobjtosee,t.tsteamlimb);

						MoveCamera (  -g.mp.spectatorfollowdistance );

						t.tXOldPos_f = CameraPositionX();
						t.tYOldPos_f = CameraPositionY();
						t.tZOldPos_f = CameraPositionZ();

						MoveCamera (  g.mp.spectatorfollowdistance );
						if (g.lightmappedobjectoffset >= g.lightmappedobjectoffsetfinish)
							t.ttt = IntersectAll(87000, 87000 + g.merged_new_objects - 1, 0, 0, 0, 0, 0, 0, -123);
						else
							t.ttt=IntersectAll(g.lightmappedobjectoffset,g.lightmappedobjectoffsetfinish,0,0,0,0,0,0,-123);
//       `tEndEntity = entityviewstartobj+entityelementlist

						t.tHitObj=IntersectAll(g.entityviewstartobj,g.entityviewendobj,t.x_f,t.y_f,t.z_f,t.tXOldPos_f,t.tYOldPos_f,t.tZOldPos_f,t.tobjtosee);
						t.tdistancewecanmovecam_f = g.mp.spectatorfollowdistance;
						if (  t.tHitObj > 0 ) 
						{
							t.tHitX_f = ChecklistFValueA(6);
							t.tHitY_f = ChecklistFValueB(6);
							t.tHitZ_f = ChecklistFValueC(6);
							t.dx_f = t.x_f - t.tHitX_f;
							t.dy_f = t.y_f - t.tHitY_f;
							t.dz_f = t.z_f - t.tHitZ_f;
							t.tdistancewecanmovecam_f = Sqrt((t.dx_f*t.dx_f)+(t.dy_f*t.dy_f)+(t.dz_f*t.dz_f)) - 30;
						}
						MoveCamera (  -t.tdistancewecanmovecam_f );

//       `move camera -mp.spectatorfollowdistance

						PointCamera (  t.x_f,t.y_f,t.z_f );

//       `tEndEntity = entityviewstartobj+entityelementlist


						t.tXOldPos_f = CameraPositionX();
						t.tYOldPos_f = CameraPositionY();
						t.tZOldPos_f = CameraPositionZ();

						t.tXNewPos_f = t.x_f;
						t.tYNewPos_f = t.y_f;
						t.tZNewPos_f = t.z_f;

						//tobjtosee = entityelement(tpe).obj;
						/*      
						t.tHitObj=IntersectAll(g.entityviewstartobj,t.tEndEntity,t.tXOldPos_f,t.tYOldPos_f,t.tZOldPos_f,t.tXNewPos_f,t.tYNewPos_f,t.tZNewPos_f,t.entityelement[t.tpe].obj);
						if (  t.tHitObj>0 ) 
						{
							if (  g.mp.spectatorfollowdistance > 10.0 ) 
							{
								g.mp.spectatorfollowdistance = g.mp.spectatorfollowdistance - 10.0;
								g.mp.spectatorfollowdistancedelay = Timer();
							}
						}
						else
						{
							if (  g.mp.spectatorfollowdistance < 200.0 && Timer() - g.mp.spectatorfollowdistancedelay > 1000 ) 
							{
								g.mp.spectatorfollowdistance = g.mp.spectatorfollowdistance + 10.0;
							}
						}
						*/    
					}
					else
					{
						t.twhokilledme = SteamGetPlayerDamageSource();
						if (  t.twhokilledme  !=  g.mp.me ) 
						{
							t.tsteamlimb=t.entityprofile[t.entityelement[t.tpe].bankindex].spine2;
							t.x_f = LimbPositionX(t.entityelement[t.tpe].obj,t.tsteamlimb);
							t.y_f = LimbPositionY(t.entityelement[t.tpe].obj,t.tsteamlimb);
							t.z_f = LimbPositionZ(t.entityelement[t.tpe].obj,t.tsteamlimb);
							PositionCamera (  t.x_f,t.y_f+100,t.z_f );
							PointCamera (  SteamGetPlayerPositionX(t.twhokilledme),SteamGetPlayerPositionY(t.twhokilledme)+50,SteamGetPlayerPositionZ(t.twhokilledme) );
							/*      
							tcamheight = (200 - g.mp.spectatorfollowdistance) / 2;
							PositionCamera (  SteamGetPlayerPositionX(t.twhokilledme), SteamGetPlayerPositionY(t.twhokilledme)+tcamheight+50, SteamGetPlayerPositionZ(t.twhokilledme) );
							SteamSetPlayerPositionX (  SteamGetPlayerPositionX(t.twhokilledme) );
							SteamSetPlayerPositionY (  SteamGetPlayerPositionY(t.twhokilledme) );
							SteamSetPlayerPositionZ (  SteamGetPlayerPositionZ(t.twhokilledme) );
							RotateCamera (  0,SteamGetPlayerAngle(t.twhokilledme),0 );
							MoveCamera (  -g.mp.spectatorfollowdistance );
							PointCamera (  SteamGetPlayerPositionX(t.twhokilledme),SteamGetPlayerPositionY(t.twhokilledme)+50,SteamGetPlayerPositionZ(t.twhokilledme) );

							t.tEndEntity = g.entityviewstartobj+g.entityelementlist;

							t.tXOldPos_f = CameraPositionX();
							t.tYOldPos_f = CameraPositionY();
							t.tZOldPos_f = CameraPositionZ();

							t.tXNewPos_f = SteamGetPlayerPositionX(t.twhokilledme);
							t.tYNewPos_f = SteamGetPlayerPositionY(t.twhokilledme);
							t.tZNewPos_f = SteamGetPlayerPositionZ(t.twhokilledme);

							t.tHitObj=IntersectAll(g.entityviewstartobj,t.tEndEntity,t.tXOldPos_f,t.tYOldPos_f,t.tZOldPos_f,t.tXNewPos_f,t.tYNewPos_f,t.tZNewPos_f,t.entityelement[t.tpe].obj);
							if (  t.tHitObj>0 ) 
							{
								if (  g.mp.spectatorfollowdistance > 10.0 ) 
								{
									g.mp.spectatorfollowdistance = g.mp.spectatorfollowdistance - 10.0;
									g.mp.spectatorfollowdistancedelay = Timer();
								}
							}
							else
							{
								if (  g.mp.spectatorfollowdistance < 200.0 && Timer() - g.mp.spectatorfollowdistancedelay > 1000 ) 
								{
									g.mp.spectatorfollowdistance = g.mp.spectatorfollowdistance + 10.0;
								}
							}
						*/    
						}
//       `if mp.respawnLeft  ==  5 && toldrespawnleft  ==  6 then mp.spectatorfollowdistance  ==  200
						
						t.toldrespawnleft = g.mp.respawnLeft;
					}
				}
				if (  g.mp.gameAlreadySpawnedBefore  ==  1 ) 
				{
					if (  CameraPositionY()  <=  BT_GetGroundHeight(t.terrain.TerrainID,CameraPositionX(),CameraPositionZ()) ) 
					{
						PositionCamera (  CameraPositionX(), BT_GetGroundHeight(t.terrain.TerrainID,CameraPositionX(),CameraPositionZ()) + 50, CameraPositionZ() );
					}
					if (  CameraPositionY() < t.terrain.waterliney_f ) 
					{
						t.tshowdeathlockcam = 0;
					}
					if (  t.tshowdeathlockcam > -1 ) 
					{
						if (  t.tshowdeathlockcam  ==  0 ) 
						{
							PositionCamera (  CameraPositionX(), CameraPositionY() + t.tspawninyoffset_f , CameraPositionZ() );
							if (  CameraPositionY() < t.terrain.waterliney_f ) 
							{
								PositionCamera (  CameraPositionX(), t.terrain.waterliney_f+100 , CameraPositionZ() );
								t.tshowdeathlockcam = 1;
								t.tshowdeathlockcamx_f = CameraPositionX();
								t.tshowdeathlockcamy_f = CameraPositionY();
								t.tshowdeathlockcamz_f = CameraPositionZ();
								t.tshowdeathlockcamrotx_f = CameraAngleX();
								t.tshowdeathlockcamroty_f = CameraAngleY();
								t.tshowdeathlockcamrotz_f = CameraAngleZ();
							}
						}
						else
						{
							PositionCamera (  t.tshowdeathlockcamx_f,t.tshowdeathlockcamy_f,t.tshowdeathlockcamz_f );
							RotateCamera (  t.tshowdeathlockcamrotx_f,t.tshowdeathlockcamroty_f, t.tshowdeathlockcamrotz_f );
						}
					}
				}
				else
				{
					if (  CameraPositionY()  <=  BT_GetGroundHeight(t.terrain.TerrainID,CameraPositionX(),CameraPositionZ()) ) 
					{
						MoveCamera (  1.0 );
						t.tdeathamounttotakeoffdistance = t.tdeathamounttotakeoffdistance + 20;
					}
					if (  t.tdeathamounttotakeoffdistance > 0 && g.mp.spectatorfollowdistance > 40 ) 
					{
						g.mp.spectatorfollowdistance = g.mp.spectatorfollowdistance - 1.0;
						t.tdeathamounttotakeoffdistance = t.tdeathamounttotakeoffdistance - 1;
					}
				}

			}

			//  update any character creator people
			t.entityelement[t.mp_playerEntityID[g.mp.me]].x = ObjectPositionX(t.entityelement[t.mp_playerEntityID[g.mp.me]].obj);
			t.entityelement[t.mp_playerEntityID[g.mp.me]].y = ObjectPositionY(t.entityelement[t.mp_playerEntityID[g.mp.me]].obj);
			t.entityelement[t.mp_playerEntityID[g.mp.me]].z = ObjectPositionZ(t.entityelement[t.mp_playerEntityID[g.mp.me]].obj);
			//  for initial spawn in
			if (  g.mp.gameAlreadySpawnedBefore  ==  0 && ( g.mp.realfirsttimespawn  ==  1 || g.mp.coop  ==  1 ) ) 
			{
				characterkit_checkForCharacters ( );
				characterkit_updateAllCharacterCreatorEntitiesInMapFirstSpawn ( );
			}

return;

}

void mp_respawn ( void )
{

	t.characterkitcontrol.showmyhead = 1;
	if ( g.autoloadgun != 0 ) { g.autoloadgun=0 ; gun_change ( ); }
	if (  t.player[t.plrid].health < 100  )  t.player[t.plrid].health  =  100;
	if (  g.mp.myOriginalSpawnPoint  !=  -1 ) 
	{
		t.tindex = g.mp.me+1;
	}
	else
	{
		t.tindex = g.mp.myOriginalSpawnPoint;
	}

	g.mp.invincibleTimer = Timer();
	t.huddamage.immunity=1000;

	g.mp.damageWasFromAI = 0;

	if (  g.mp.coop  ==  1 ) 
	{
//  `remstart

		if (  g.mp.originalEntitycount  ==  0 ) 
		{
			//  Store the count here incase other elements get added later (like guns)
			g.mp.originalEntitycount = g.entityelementlist;
			Dim (  t.steamStoreentityelement,g.entityelementlist );
			for ( t.te = 1 ; t.te<=  g.entityelementlist; t.te++ )
			{
				t.steamStoreentityelement[t.te]=t.entityelement[t.te];
			}
		}
//   `remend

	}

	t.playercontrol.deadtime = Timer() + 2000;
	t.playercontrol.redDeathFog_f = 0;
//physics_disableplayer ( );
	t.aisystem.processplayerlogic=0;
	g.mp.noplayermovement = 1;
	
	if (  g.mp.syncedWithServer  ==  0 ) 
	{
		SteamSendIAmReadyToPlay (  );
		g.mp.syncedWithServer = 1;
		g.mp.sentreadytime = Timer();
//   `print "SENDING I AM READY"

		//  are we the server? if so, let lua know
		if (  g.mp.isGameHost  ==  1 ) 
		{
			LuaSetInt (  "mp_isServer",1 );
		}
		else
		{
			LuaSetInt (  "mp_isServer",0 );
		}
		LuaSetInt (  "mp_coop", g.mp.coop );
		mp_howManyEnemiesLeftToKill ( );
		LuaSetInt (  "mp_me",g.mp.me+1 );
		mp_setLuaResetStats ( );
	}
	
	ravey_particles_delete_all_emitters ( );
	
	if (  g.mp.maxHealth  ==  0  )  g.mp.maxHealth  =  t.player[t.plrid].health;
	
	if (  g.mp.gameAlreadySpawnedBefore  ==  0 || Timer() - g.mp.dyingTime > 1500 ) 
	{
			if (  g.mp.gameAlreadySpawnedBefore  ==  0 ) 
			{

				//  13032015 0XX - Team Multiplayer
				if (  g.mp.team  ==  1 ) 
				{
					for ( t.tteam = 1 ; t.tteam<=  MP_MAX_NUMBER_OF_PLAYERS; t.tteam++ )
					{
						t.tnothing = LuaExecute( cstr(cstr("mp_playerTeam[") + Str(t.tteam) + "] = " + Str(t.mp_team[t.tteam-1])).Get() );
					}
						t.tnothing = LuaExecute( cstr(cstr("mp_teambased = ") + Str(g.mp.team)).Get() );
				}

				t.tindex = g.mp.me+1;
				g.mp.myOriginalSpawnPoint = t.tindex;

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
				else
				{
					t.tfound = 0;
					t.ttempindex = t.tindex/2;
					if (  t.ttempindex > 0 ) 
					{
						if (  t.mpmultiplayerstart[t.ttempindex].active == 1 ) 
						{
							g.mp.myOriginalSpawnPoint = t.ttempindex;
							t.tfound = 1;
							t.terrain.playerx_f=t.mpmultiplayerstart[t.ttempindex].x;
							t.terrain.playery_f=t.mpmultiplayerstart[t.ttempindex].y+20;
							t.terrain.playerz_f=t.mpmultiplayerstart[t.ttempindex].z;
							t.terrain.playerax_f=0;
							t.terrain.playeray_f=t.mpmultiplayerstart[t.ttempindex].angle;
							t.terrain.playeraz_f=0;

							g.mp.lastx=t.terrain.playerx_f;
							g.mp.lasty=t.terrain.playery_f;
							g.mp.lastz=t.terrain.playerz_f;
							g.mp.lastangley=t.terrain.playeray_f;
						}
					}
					if (  t.tfound  ==  0 ) 
					{
						if (  t.mpmultiplayerstart[1].active == 1 ) 
						{
							g.mp.myOriginalSpawnPoint = 1;
							t.tfound = 1;
							t.terrain.playerx_f=t.mpmultiplayerstart[1].x;
							t.terrain.playery_f=t.mpmultiplayerstart[1].y+20;
							t.terrain.playerz_f=t.mpmultiplayerstart[1].z;
							t.terrain.playerax_f=0;
							t.terrain.playeray_f=t.mpmultiplayerstart[1].angle;
							t.terrain.playeraz_f=0;
	
							g.mp.lastx=t.terrain.playerx_f;
							g.mp.lasty=t.terrain.playery_f;
							g.mp.lastz=t.terrain.playerz_f;
							g.mp.lastangley=t.terrain.playeray_f;
						}
					}
					if (  t.tfound  ==  0 ) 
					{
						physics_resetplayer_core ( );
					}
				}
			}

			SteamSetPlayerPositionX (  t.terrain.playerx_f );
			SteamSetPlayerPositionY (  t.terrain.playery_f );
			SteamSetPlayerPositionZ (  t.terrain.playerz_f );
			SteamSetPlayerAngle (  t.terrain.playeray_f );
	}

	if (  SteamIsEveryoneReadyToPlay()  ==  0 || g.mp.syncedWithServer  ==  0 ) 
	{
		mp_textDots(-1,30,3,"Waiting for other players to join");
		if (  Timer() - g.mp.sentreadytime > 30*1000 ) 
		{
				g.mp.syncedWithServer = 0;
		}
		t.typos = 40;
		for ( t.tn = 0 ; t.tn<=  MP_MAX_NUMBER_OF_PLAYERS-1; t.tn++ )
		{
			if (  t.mp_joined[t.tn]  !=  "" ) 
			{
				if (  cstr(Right(t.mp_joined[t.tn].Get(),6 ))  ==  "Joined" ) 
				{
					mp_textColor(-1,t.typos,1,t.mp_joined[t.tn].Get(),100,255,100);
				}
				else
				{
					mp_textColor(-1,t.typos,1, cstr(t.mp_joined[t.tn] + " - Waiting").Get(),255,200,100);
				}
				t.typos += 5;
			}
		}
		return;
	}

	for ( t.c = 0 ; t.c<=  MP_MAX_NUMBER_OF_PLAYERS-1; t.c++ )
	{
			t.x_f = SteamGetPlayerPositionX(t.c);
			t.y_f = SteamGetPlayerPositionY(t.c);
			t.z_f = SteamGetPlayerPositionZ(t.c);
			t.angle_f = SteamGetPlayerAngle(t.c);
	}

	t.tobj = t.entityelement[t.mp_playerEntityID[g.mp.me]].obj;

	if (  g.mp.gameAlreadySpawnedBefore  ==  1 ) 
	{
		//  if we have not died to another player, we take one off our kills instead since we killed ourself
		if (  g.mp.checkedWhoKilledMe  ==  0 ) 
		{
			g.mp.checkedWhoKilledMe = 1;
			if (  g.mp.killedByPlayerFlag  ==  0 ) 
			{
				if (  g.mp.coop  ==  0 ) 
				{
					SteamSendLua (  MP_LUA_ServerSetPlayerRemoveKill,0,g.mp.me+1 );
					SteamKilledSelf (  );
				}
			}
			else
			{
				if (  g.mp.coop  ==  0 ) 
				{
					SteamSendLua (  MP_LUA_ServerSetPlayerAddKill,0,g.mp.playerThatKilledMe+1 );
				}
			}
			SteamSendLua (  MP_LUA_ServerSetPlayerAddDeath,0,g.mp.me+1 );
		}
	}
	
	if (  SteamReadyToSpawn()  ==  0 ) 
	{
		mp_text(-1,20,3,"WAITING FOR PLAYERS");
		return;
	}
	if (  g.mp.syncedWithServer  ==  0 ) 
	{
		mp_pre_game_file_sync ( );
		if (  SteamGetClientServerConnectionStatus()  ==  0 ) 
		{
			t.tsteamlostconnectioncustommessage_s = "Lost connection with server (Error MP013)";
			mp_lostConnection ( );
			return;
		}
		return;
	}

	if (  g.mp.endplay  ==  0 && g.mp.showscoresdelay  ==  -2000 ) 
	{
		mp_panel(40,45,60,65);
		mp_text(-1,52,3,"SPAWNING IN");

		t.s_s = Str(5-g.mp.respawnLeft);
		mp_text(-1,58,3,t.s_s.Get());

		if (  g.mp.coop  ==  0 ) 
		{
			if (  g.mp.killedByPlayerFlag  ==  1 ) 
			{
				t.s_s = cstr("YOU WERE KILLED BY ") + Upper(SteamGetOtherPlayerName(g.mp.playerThatKilledMe));
				mp_text(-1,30,3,t.s_s.Get());
			}
			else
			{
				if (  g.mp.gameAlreadySpawnedBefore  ==  1 ) 
				{
					t.s_s = "YOU KILLED YOURSELF!";
					mp_text(-1,30,3,t.s_s.Get());
				}
			}
		}
		else
		{
			if (  g.mp.gameAlreadySpawnedBefore  ==  1 ) 
			{
				t.s_s = "YOU DIED!";
				mp_text(-1,30,3,t.s_s.Get());
			}
		}
	}
	if (  g.mp.oldSpawnTimeLeft  ==  0  )  g.mp.oldSpawnTimeLeft  =  Timer();

	if (  Timer() - g.mp.oldSpawnTimeLeft  >=  1000 ) 
	{
		++g.mp.respawnLeft;
		g.mp.oldSpawnTimeLeft = 0;
		if (  g.mp.respawnLeft  >=  5 ) 
		{

			g.mp.haveshowndeath = 0;
			weapon_mp_projectile_reset ( );
			ravey_particles_delete_all_emitters ( );
			lua_removeplayerweapons ( );
			t.tsteamwasnetworkdamage = 0;
			g.mp.checkedWhoKilledMe = 0;
			g.mp.killedByPlayerFlag = 0;
			g.plrreloading = 0;
			t.playercontrol.pushforce_f = 0.0;
			t.playercontrol.camerashake_f = 0.0;
			g.mp.lastSendTime = 0;
			g.mp.spectatorfollowdistance = 200.0;
			t.tme = g.mp.me;
			if (  t.mp_playingRagdoll[t.tme]  ==  1 ) 
			{

				t.tphyobj=t.entityelement[t.mp_playerEntityID[t.tme]].obj;
				ragdoll_destroy ( );
				RotateObject (  t.entityelement[t.mp_playerEntityID[t.tme]].obj,0,180,0 );
				FixObjectPivot (  t.entityelement[t.mp_playerEntityID[t.tme]].obj );
				t.mp_playingRagdoll[t.tme] = 0;

			}

			if (  ObjectExist(g.steamplayermodelsoffset+t.tme+121)  ==  1 ) 
			{
				ODEDestroyObject (  g.steamplayermodelsoffset+t.tme+121 );
				RotateObject (  g.steamplayermodelsoffset+t.tme+121,0,0,0 );
				PositionObject (  g.steamplayermodelsoffset+t.tme+121,0,-99999,0 );
				HideObject (  g.steamplayermodelsoffset+t.tme+121 );
			}

			g.mp.ragdollon = 0;
			if (  g.mp.endplay  ==  0 ) 
			{
				t.aisystem.processplayerlogic=1;
			}
			t.playercontrol.deadtime = 0;
			t.playercontrol.redDeathFog_f = 0;

			if (  g.mp.maxHealth  ==  0  )  g.mp.maxHealth  =  100;
			g.mp.reloading = 0;

			t.mp_health[g.mp.me] = g.mp.maxHealth;
			t.entityelement[t.mp_playerEntityID[g.mp.me]].health = g.mp.maxHealth;
			t.player[t.plrid].health = g.mp.maxHealth;
			g.mp.killedByPlayer = 0;
			g.mp.playedMyDeathAnim = 0;
	
			// courtesy of Ravey
			t.playercontrol.regenrate = 2;
			t.playercontrol.regenspeed = 100;
			t.playercontrol.regendelay = 3000;
			t.playercontrol.regentime = 3000;

			//  16032015 - 020 - MP Team code
			if (  g.mp.gameAlreadySpawnedBefore  ==  0 && g.mp.team  ==  0 ) 
			{
				t.tindex = g.mp.me+1;
	
				if (  t.mpmultiplayerstart[t.tindex].active == 1 ) 
				{
					t.terrain.playerx_f=t.mpmultiplayerstart[t.tindex].x;
					t.terrain.playery_f=t.mpmultiplayerstart[t.tindex].y+20;
					t.terrain.playerz_f=t.mpmultiplayerstart[t.tindex].z;
					t.terrain.playerax_f=0;
					t.terrain.playeray_f=t.mpmultiplayerstart[t.tindex].angle;
					t.terrain.playeraz_f=0;

					physics_resetplayer_core ( );
				}
				else
				{
					t.tfound = 0;
					t.ttempindex = t.tindex/2;
					if (  t.ttempindex > 0 ) 
					{
						if (  t.mpmultiplayerstart[t.ttempindex].active == 1 ) 
						{
							t.tfound = 1;
							t.terrain.playerx_f=t.mpmultiplayerstart[t.ttempindex].x;
							t.terrain.playery_f=t.mpmultiplayerstart[t.ttempindex].y+20;
							t.terrain.playerz_f=t.mpmultiplayerstart[t.ttempindex].z;
							t.terrain.playerax_f=0;
							t.terrain.playeray_f=t.mpmultiplayerstart[t.ttempindex].angle;
							t.terrain.playeraz_f=0;
	
							g.mp.lastx=t.terrain.playerx_f;
							g.mp.lasty=t.terrain.playery_f;
							g.mp.lastz=t.terrain.playerz_f;
							g.mp.lastangley=t.terrain.playeray_f;
						}
					}
					if (  t.tfound  ==  0 ) 
					{
						if (  t.mpmultiplayerstart[1].active == 1 ) 
						{
							t.tfound = 1;
							t.terrain.playerx_f=t.mpmultiplayerstart[1].x;
							t.terrain.playery_f=t.mpmultiplayerstart[1].y+20;
							t.terrain.playerz_f=t.mpmultiplayerstart[1].z;
							t.terrain.playerax_f=0;
							t.terrain.playeray_f=t.mpmultiplayerstart[1].angle;
							t.terrain.playeraz_f=0;

							g.mp.lastx=t.terrain.playerx_f;
							g.mp.lasty=t.terrain.playery_f;
							g.mp.lastz=t.terrain.playerz_f;
							g.mp.lastangley=t.terrain.playeray_f;
						}
					}
					if (  t.tfound  !=  0 ) 
					{
						physics_resetplayer_core ( );
					}
	
//      `endif

				}
			}
			else
			{
				t.tsteamnumberofmarkers = 0;
				for ( t.tc = 1 ; t.tc<=  MP_MAX_NUMBER_OF_PLAYERS; t.tc++ )
				{
					if (  t.mpmultiplayerstart[t.tc].active == 1 ) 
					{
						++t.tsteamnumberofmarkers;
					}
				}
				if (  g.mp.spawnrnd  ==  -1 && t.tsteamnumberofmarkers > 0  )  g.mp.spawnrnd  =  Rnd(t.tsteamnumberofmarkers-1)+1;
				//  13032015 0XX - Team Multiplayer
				if (  g.mp.team  ==  1 && g.mp.coop  ==  0 ) 
				{
					if (  t.tsteamnumberofmarkers  >=  8 ) 
					{
						if (  t.mp_team[g.mp.me]  ==  0 ) 
						{
							g.mp.spawnrnd = Rnd(4-1)+1;
						}
						else
						{
							g.mp.spawnrnd = Rnd(4-1)+1+4;
						}
					}
					if (  t.tsteamnumberofmarkers  ==  4 ) 
					{
						if (  t.mp_team[g.mp.me]  ==  0 ) 
						{
							g.mp.spawnrnd = Rnd(2-1)+1;
						}
						else
						{
							g.mp.spawnrnd = Rnd(2-1)+1+2;
						}
					}
					if (  t.tsteamnumberofmarkers  ==  2 ) 
					{
						if (  t.mp_team[g.mp.me]  ==  0 ) 
						{
							g.mp.spawnrnd = 0;
						}
						else
						{
							g.mp.spawnrnd = 1;
						}
					}
				}
				if (  t.tsteamnumberofmarkers  ==  1  )  g.mp.spawnrnd  =  0;
				mp_getPlaceToSpawn ( );
				if (  t.mpmultiplayerstart[g.mp.spawnrnd].active == 1 ) 
				{
					t.terrain.playerx_f=t.mpmultiplayerstart[g.mp.spawnrnd].x;
					t.terrain.playery_f=t.mpmultiplayerstart[g.mp.spawnrnd].y+20;
					t.terrain.playerz_f=t.mpmultiplayerstart[g.mp.spawnrnd].z;
					t.terrain.playerax_f=0;
					t.terrain.playeray_f=t.mpmultiplayerstart[g.mp.spawnrnd].angle;
					t.terrain.playeraz_f=0;
				}
				else
				{
					if (  t.mpmultiplayerstart[1].active == 1 ) 
					{
						t.terrain.playerx_f=t.mpmultiplayerstart[1].x;
						t.terrain.playery_f=t.mpmultiplayerstart[1].y+20;
						t.terrain.playerz_f=t.mpmultiplayerstart[1].z;
						t.terrain.playerax_f=0;
						t.terrain.playeray_f=t.mpmultiplayerstart[1].angle;
						t.terrain.playeraz_f=0;
					}
				}

				physics_resetplayer_core ( );

				SteamSetPlayerPositionX (  t.terrain.playerx_f );
				SteamSetPlayerPositionY (  t.terrain.playery_f );
				SteamSetPlayerPositionZ (  t.terrain.playerz_f );
				SteamSetPlayerAngle (  t.terrain.playeray_f );
			}
	
			g.mp.spawnrnd = -1;
	
			mp_getInitialPlayerCount ( );
	
			if (  g.mp.gameAlreadySpawnedBefore  ==  0 ) 
			{
				//  send our name on first respawn to ensure everyone gets it
				//  as this is the moment everyone is definately there
				//  steam can sometimes fail to get the name for a while
				//  so we will send it a few times
//     `if Timer() - mp.lastsendmynametime > 1000

//      `mp.lastsendmynametime = Timer()

					SteamSendMyName (  );
					g.mp.sentmyname = 1;
//     `endif


				if (  t.game.runasmultiplayer == 1 && g.mp.coop  ==  1 ) 
				{
					for ( t.e = 1 ; t.e<=  g.entityelementlist; t.e++ )
					{
						t.entid=t.entityelement[t.e].bankindex;
						if (  t.entid>0 ) 
						{
							if (  t.entityprofile[t.entid].ischaracter  ==  1 || t.entityelement[t.e].mp_isLuaChar  ==  1 ) 
							{
								t.entityelement[t.e].mp_coopControlledByPlayer = -1;
								t.entityelement[t.e].mp_coopLastTimeSwitchedTarget = 0;
//         `if entityelement(e).speedmodulator# < 1.0 then entityelement(e).speedmodulator#  ==  1.0

							}
						}
					}
				}

			}

			g.mp.realfirsttimespawn = 0;
			g.mp.gameAlreadySpawnedBefore = 1;
			for ( t.c = 0 ; t.c<=  MP_MAX_NUMBER_OF_PLAYERS-1; t.c++ )
			{
					t.mp_forcePosition[t.c] = 1;
			}
			g.mp.respawnLeft = 0;

			for ( t.tc = 0 ; t.tc<=  MP_MAX_NUMBER_OF_PLAYERS-1; t.tc++ )
			{
				t.mp_playingAnimation[t.tc] = MP_ANIMATION_NONE;
			}

		}
		//Much like mouse move x, calling get player damage amount will wipe it out after
		t.a=SteamGetPlayerDamageAmount();
		t.entityelement[t.mp_playerEntityID[g.mp.me]].eleprof.hasweapon = 0;
		g.mp.noplayermovement = 0;
		g.mp.invincibleTimer = Timer();
		g.mp.lastSpawnedTime = g.mp.invincibleTimer;


	}

return;

}

void mp_getPlaceToSpawn ( void )
{
	t.found = -1;
	
	if (  g.mp.team  ==  1  )  t.tdisttocheck  =  100; else t.tdisttocheck  =  300;
	
	//  check if the spawnpoint picked is clear, if it is, just use that
	t.failed = 0;
	for ( t.c = 0 ; t.c<=  MP_MAX_NUMBER_OF_PLAYERS-1; t.c++ )
	{

		if (  t.c  !=  g.mp.me ) 
		{
			t.tpx_f = SteamGetPlayerPositionX(t.c);
			t.tpy_f = SteamGetPlayerPositionY(t.c);
			t.tpz_f = SteamGetPlayerPositionZ(t.c);

			t.tsx_f = t.mpmultiplayerstart[g.mp.spawnrnd].x;
			t.tsy_f = t.mpmultiplayerstart[g.mp.spawnrnd].y-50;
			t.tsz_f = t.mpmultiplayerstart[g.mp.spawnrnd].z;

			t.dx_f = t.tpx_f - t.tsx_f;
			t.dy_f = t.tpy_f - t.tsy_f;
			t.dz_f = t.tpz_f - t.tsz_f;

			t.dist_f = Sqrt((t.dx_f*t.dx_f)+(t.dy_f*t.dy_f)+(t.dz_f*t.dz_f));

			if (  t.dist_f < t.tdisttocheck  )  t.failed  =  1;
		}

	}

	//  is no one is here, lets use it
	if (  t.failed  ==  0  )  return;
	
	t.tstart = 1;
	t.tend = MP_MAX_NUMBER_OF_PLAYERS;

	if (  g.mp.team  ==  1 ) 
	{

		t.tsteamnumberofmarkers = 0;
		for ( t.tc = 1 ; t.tc<=  MP_MAX_NUMBER_OF_PLAYERS; t.tc++ )
		{
			if (  t.mpmultiplayerstart[t.tc].active == 1 ) 
			{
				++t.tsteamnumberofmarkers;
			}
		}

		if (  t.tsteamnumberofmarkers  >=  8 ) 
		{
			if (  t.mp_team[g.mp.me]  ==  0 ) 
			{
				t.tstart = 1;
				t.tend = 4;
			}
			else
			{
				t.tstart = 5;
				t.tend = 8;
			}
		}
		if (  t.tsteamnumberofmarkers  ==  4 ) 
		{
			if (  t.mp_team[g.mp.me]  ==  0 ) 
			{
				t.tstart = 1;
				t.tend = 2;
			}
			else
			{
				t.tstart = 3;
				t.tend = 4;
			}
		}
		if (  t.tsteamnumberofmarkers  <=  2 ) 
		{
			return;
		}
	}

	if (  g.mp.coop  ==  1 ) 
	{
		t.tstart = 1;
		t.tend = t.tsteamnumberofmarkers;
	}
	//  it failed so lets look for an alternative
	for ( t.tspawnpoints = t.tstart ; t.tspawnpoints<=  t.tend; t.tspawnpoints++ )
	{
		if (  t.tspawnpoints  !=  g.mp.spawnrnd ) 
		{
			t.failed = 0;
			if (  t.mpmultiplayerstart[t.tspawnpoints].active == 1 ) 
			{

				for ( t.c = 0 ; t.c<=  MP_MAX_NUMBER_OF_PLAYERS-1; t.c++ )
				{

					if (  t.c  !=  g.mp.me ) 
					{
						t.tpx_f = SteamGetPlayerPositionX(t.c);
						t.tpy_f = SteamGetPlayerPositionY(t.c);
						t.tpz_f = SteamGetPlayerPositionZ(t.c);

						t.tsx_f = t.mpmultiplayerstart[t.tspawnpoints].x;
						t.tsy_f = t.mpmultiplayerstart[t.tspawnpoints].y-50;
						t.tsz_f = t.mpmultiplayerstart[t.tspawnpoints].z;

						t.dx_f = t.tpx_f - t.tsx_f;
						t.dy_f = t.tpy_f - t.tsy_f;
						t.dz_f = t.tpz_f - t.tsz_f;

						t.dist_f = Sqrt((t.dx_f*t.dx_f)+(t.dy_f*t.dy_f)+(t.dz_f*t.dz_f));

						if (  t.dist_f < t.tdisttocheck  )  t.failed  =  1;
					}

				}

				//  if noone is here lets use this
				if (  t.failed  ==  0 ) 
				{
					g.mp.spawnrnd = t.tspawnpoints;
					return;
				}
			}
		}
	}
	
return;

}

void mp_getInitialPlayerCount ( void )
{
	g.mp.howmanyjoinedatstart = 0;
	for ( t.c = 0 ; t.c<=  MP_MAX_NUMBER_OF_PLAYERS-1; t.c++ )
	{
		t.tname_s = SteamGetOtherPlayerName(t.c);
		if (  t.tname_s != "Player"  )  ++g.mp.howmanyjoinedatstart;
	}
return;

}

void mp_nukeTestmap ( void )
{
	mp_deleteFile ("levelbank\\cfg.cfg");
	mp_deleteFile ("levelbank\\conkit.dat");
	mp_deleteFile ("levelbank\\header.dat");
	mp_deleteFile ("levelbank\\m.dat");
	mp_deleteFile ("levelbank\\map.ele");
	mp_deleteFile ("levelbank\\map.ent");
	mp_deleteFile ("levelbank\\map.way");
	mp_deleteFile ("levelbank\\playerconfig.dat");
	mp_deleteFile ("levelbank\\temparea.txt");
	mp_deleteFile ("levelbank\\vegmaskgrass.dat");
	mp_deleteFile ("levelbank\\visuals.ini");
	mp_deleteFile ("levelbank\\watermask.dds");
	//mp_deleteFile ("editors\\gridedit\\__multiplayerlevel__.fpm");
	//mp_deleteFile ("editors\\gridedit\\__multiplayerworkshopitemid__.dat");
	mp_deleteFile (cstr(g.mysystem.editorsGridedit_s+"__multiplayerlevel__.fpm").Get());
	mp_deleteFile (cstr(g.mysystem.editorsGridedit_s+"__multiplayerworkshopitemid__.dat").Get());
}

void mp_respawnEntities ( void )
{
	if (  g.mp.destroyedObjectCount > 0 ) 
	{
		for ( t.i = 0 ; t.i<=  g.mp.destroyedObjectCount-1; t.i++ )
		{
			t.e = t.mp_destroyedObjectList[t.i];
			t.entityelement[t.e].active = 1;
			entity_lua_spawn ( );
			entity_lua_collisionon ( );
			t.entityelement[t.e].lua.firsttime=0;
			t.entityelement[t.e].activated = 0;
			t.entityelement[t.e].collected = 0;
			t.entityelement[t.e].explodefusetime = 0;
			StopObject (  t.entityelement[t.e].obj );
			SetObjectFrame (  t.entityelement[t.e].obj,0 );
			ShowObject (  t.entityelement[t.e].obj );
		}
	}
	for ( t.e = 1 ; t.e<=  g.entityelementlist; t.e++ )
	{
		t.ttentid=t.entityelement[t.e].bankindex;
		if (  t.entityprofile[t.ttentid].strength > 0 || t.entityelement[t.e].activated  !=  0 || t.entityelement[t.e].collected  !=  0 || t.entityelement[t.e].eleprof.strength > 0 ) 
		{
			if (  t.entityelement[t.e].obj > 0 && t.entityelement[t.e].staticflag  ==  0 ) 
			{
				if (  ObjectExist(t.entityelement[t.e].obj)  ==  1 ) 
				{

					t.entityelement[t.e].lua.flagschanged=1;
					if (  g.mp.coop  ==  0 ) 
					{
						entity_lua_spawn ( );
					}
					else
					{
						entity_lua_spawn_core ( );
					}
					t.entityelement[t.e].lua.firsttime=0;
					//120916 - seems collisionON sets SetObjectCollisionProperty to 0 (needed for exploding barrel) - left as Dave coded it just in case
					if ( t.entityelement[t.e].eleprof.explodable  ==  0 ) 
					{
						entity_lua_collisionon ( );
					}
					else
					{
						t.tphyobj = t.entityelement[t.e].obj;
						t.entid = t.ttentid;
						physics_setupobject ( );
					}
					t.aisystem.cumilativepauses=0;
					t.entityelement[t.e].mp_updateOn = 0;
					t.entityelement[t.e].mp_coopControlledByPlayer = -1;
					t.entityelement[t.e].active = 1;
					t.entityelement[t.e].activated = 0;
					t.entityelement[t.e].collected = 0;
					t.entityelement[t.e].explodefusetime = 0;
					t.entityelement[t.e].health = t.entityelement[t.e].eleprof.strength;
					StopObject (  t.entityelement[t.e].obj );
					SetObjectFrame (  t.entityelement[t.e].obj,0 );
				}
			}
		}
	}
	g.mp.destroyedObjectCount = 0;
}

void mp_addDestroyedObject ( void )
{
	//  if (  it has a quantity  )  we will respawn it after so much time has passed
	if (  t.entityelement[t.e].eleprof.quantity > 0 ) 
	{
		mp_add_respawn_timed ( );
	}
	if (  g.mp.destroyedObjectCount < MP_DESTROYED_OBJECT_LIST_SIZE ) 
	{
		t.mp_destroyedObjectList[g.mp.destroyedObjectCount] = t.e;
		++g.mp.destroyedObjectCount;
	}
return;

}

void mp_add_respawn_timed ( void )
{
	for ( t.i = 0 ; t.i<=  MP_RESPAWN_TIME_OBJECT_LIST_SIZE; t.i++ )
	{
			if (  t.mp_respawn_timed[t.i].inuse  ==  0 ) 
			{
				t.mp_respawn_timed[t.i].inuse = 1;
				t.mp_respawn_timed[t.i].e = t.e;
				t.mp_respawn_timed[t.i].time = Timer();
				break;
			}
	}
return;

}

void mp_setLuaPlayerNames ( void )
{
	for ( t.i = 0 ; t.i<=  MP_MAX_NUMBER_OF_PLAYERS-1; t.i++ )
	{
		if (  t.i  ==  g.mp.me ) 
		{
			t.tsteamname_s = g.mp.playerName;
		}
		else
		{
			t.tsteamname_s = SteamGetOtherPlayerName(t.i);
		}
		//  ensure the string isnt null before doing anything to it
		if (  t.tsteamname_s  !=  "" ) 
		{
			t.tsteamnameNoApos_s = "";
			for ( t.tloop = 1 ; t.tloop<=  Len(t.tsteamname_s.Get()); t.tloop++ )
			{
				if (  cstr(Mid(t.tsteamname_s.Get(),t.tloop))  !=  "'" && cstr(Mid(t.tsteamname_s.Get(),t.tloop))  !=  Chr(34) ) 
				{
					t.tsteamnameNoApos_s = t.tsteamnameNoApos_s + Mid(t.tsteamname_s.Get(),t.tloop);
				}
				else
				{
					t.tsteamnameNoApos_s = t.tsteamnameNoApos_s + "_";
				}
			}
			t.tnothing = LuaExecute( cstr(cstr("mp_playerNames[") + Str(t.i+1) + "] = '" + t.tsteamnameNoApos_s + "'").Get() );
			if (  t.tsteamname_s  ==  "Player" || t.tsteamname_s  ==  "" ) 
			{
				t.tnothing = LuaExecute( cstr(cstr("mp_playerConnected[") + Str(t.i+1) + "] = 0").Get() );
			}
			else
			{
				t.tnothing = LuaExecute( cstr(cstr("mp_playerConnected[") + Str(t.i+1) + "] = 1").Get() );
			}
		}
	}
return;

}

void mp_setLuaResetStats ( void )
{
	for ( t.i = 0 ; t.i<=  MP_MAX_NUMBER_OF_PLAYERS-1; t.i++ )
	{
		t.tnothing = LuaExecute( cstr(cstr("mp_playerKills[") + Str(t.i+1) + "] = 0").Get() );
		t.tnothing = LuaExecute( cstr(cstr("mp_playerDeaths[") + Str(t.i+1) + "] = 0").Get() );
		t.tnothing = LuaExecute( cstr(cstr("mp_playerNames[") + Str(t.i+1) + "] = ''").Get() );
		t.tnothing = LuaExecute( cstr(cstr("mp_playerConnected[") + Str(t.i+1) + "] = 0").Get() );
		t.mp_kills[t.i] = 0;
		t.mp_deaths[t.i] = 0;
	}
	for ( t.i = 0 ; t.i<=  MP_RESPAWN_TIME_OBJECT_LIST_SIZE; t.i++ )
	{
		t.mp_respawn_timed[t.i].inuse = 0;
	}

	for ( t.e = 1 ; t.e<=  g.entityelementlist; t.e++ )
	{
			t.entityelement[t.e].mp_networkkill = 0;
	}

	t.tsteamwasnetworkdamage = 0;

return;

}

