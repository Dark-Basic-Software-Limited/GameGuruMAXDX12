void mp_updatePlayerAnimations ( void )
{
	// Update animations
	for ( t.c = 0 ; t.c <= MP_MAX_NUMBER_OF_PLAYERS-1; t.c++ )
	{
		if ( t.mp_playerEntityID[t.c] > 0 )
		{
			// get player info
			t.tobj = t.entityelement[t.mp_playerEntityID[t.c]].obj;
			t.thasNade = 0;
			t.tgunid=t.entityelement[t.mp_playerEntityID[t.c]].eleprof.hasweapon;
			if ( t.gun[t.tgunid].projectileframe != 0 ) t.thasNade = 1;

			// get multiplayer datas
 			 int iPlayerShoot = PhotonGetShoot(t.c);
			 int iPlayerAlive = PhotonGetPlayerAlive(t.c);
			 int iPlayerAppearance = PhotonGetPlayerAppearance(t.c);
			 int iPlayerKey16 = PhotonGetKeyState(t.c,16);
			 int iPlayerKey17 = PhotonGetKeyState(t.c,17);
			 int iPlayerKey30 = PhotonGetKeyState(t.c,30);
			 int iPlayerKey31 = PhotonGetKeyState(t.c,31);
			 int iPlayerKey32 = PhotonGetKeyState(t.c,32);
			 int iPlayerKey42 = PhotonGetKeyState(t.c,42);
			 int iPlayerKey46 = PhotonGetKeyState(t.c,46);
 			t.mp_playerShooting[t.c] = iPlayerShoot;

			// if the player is reloading we will try and show it (only works if idle or ducking at present)
			if ( iPlayerAppearance == 201 ) t.mp_reload[t.c] = 1;

			// update animations
			g.mp.isAnimating = 0;
			if ( iPlayerAlive == 1 ) 
			{
				if ( (iPlayerAppearance < 102 || iPlayerAppearance > 200) ) 
				{
					// Melee
					if ( iPlayerKey16 == 1 || t.mp_meleePlaying[t.c] == 1 ) 
					{
						g.mp.isAnimating = 1;
						if ( t.mp_meleePlaying[t.c]  ==  0 ) 
						{
							t.mp_meleePlaying[t.c] = 1;
						}
						else
						{
							if ( GetPlaying(t.tobj)  ==  0  )  t.mp_meleePlaying[t.c]  =  0;
							if ( GetLooping(t.tobj)  ==  1  )  t.mp_meleePlaying[t.c]  =  0;
						}
					}

					//  Forwards
					if ( iPlayerKey17  ==  1 ) 
					{
						g.mp.isAnimating = 1;
						if ( iPlayerKey30  ==  1 ) 
						{
							YRotateObject (  t.tobj, ObjectAngleY(t.tobj) - 45 );
						}
						if ( iPlayerKey32  ==  1 ) 
						{
							YRotateObject (  t.tobj, ObjectAngleY(t.tobj) + 45 );
						}
						if ( iPlayerKey42 ==  0 || iPlayerKey46 ==  1 ) 
						{
							if ( iPlayerAppearance ==  101 ) 
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
							if ( iPlayerAppearance ==  101 ) 
							{
								t.entityelement[t.mp_playerEntityID[t.c]].eleprof.animspeed=600;
							}
							else
							{
								t.entityelement[t.mp_playerEntityID[t.c]].eleprof.animspeed=200;
							}
						}
						if ( iPlayerKey46 ==  0 ) 
						{
							if ( t.mp_playingAnimation[t.c]  != MP_ANIMATION_WALKING ) 
							{
								t.tgunid=t.entityelement[t.mp_playerEntityID[t.c]].eleprof.hasweapon;
								t.tweapstyle=t.gun[t.tgunid].weapontype;
								if ( t.tweapstyle > 5  )  t.tweapstyle  =  1;
								if ( t.entityelement[t.mp_playerEntityID[t.c]].attachmentobj > 0 && t.thasNade  ==  0 ) 
								{
									t.tplaycsioranimindex = 1;// 10;//t.csi_stoodmoverunANIM[t.tweapstyle];
								}
								else
								{
									t.tplaycsioranimindex = 1;// 10;//g.csi_unarmedmoverunANIM;
								}
								mp_switchDirectAnim ( t.tplaycsioranimindex );
								if ( iPlayerAppearance == 101 ) 
								{
									t.tplaycsioranimindex = 0;// g.csi_unarmedANIM0;
									mp_switchDirectAnim ( t.tplaycsioranimindex );
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
							if ( t.mp_playingAnimation[t.c]  !=  MP_ANIMATION_DUCKINGWALKING ) 
							{
								t.tgunid=t.entityelement[t.mp_playerEntityID[t.c]].eleprof.hasweapon;
								t.tweapstyle=t.gun[t.tgunid].weapontype;
								if ( t.tweapstyle > 5  )  t.tweapstyle  =  1;
								if ( t.tweapstyle  ==  0  )  t.tweapstyle  =  1;
								t.tplaycsioranimindex = 1;// 12;//t.csi_crouchmoverunANIM[t.tweapstyle];
								mp_switchDirectAnim ( t.tplaycsioranimindex );
								entity_lua_setanimationframes ( );
								t.e = t.mp_playerEntityID[t.c];
								entity_lua_loopanimation ( );
								g.mp.isAnimating = 1;
								t.mp_playingAnimation[t.c] = MP_ANIMATION_DUCKINGWALKING;
							}
						}
					}

					// Backwards
					if ( iPlayerKey31 ==  1 ) 
					{
						g.mp.isAnimating = 1;
						if ( iPlayerKey30 ==  1 ) 
						{
							YRotateObject ( t.tobj, ObjectAngleY(t.tobj) + 45 );
						}
						if ( iPlayerKey32 == 1 ) 
						{
							YRotateObject ( t.tobj, ObjectAngleY(t.tobj) - 45 );
						}
						if ( iPlayerAppearance ==  101 ) 
						{
							t.entityelement[t.mp_playerEntityID[t.c]].eleprof.animspeed=-300;
						}
						else
						{
							t.entityelement[t.mp_playerEntityID[t.c]].eleprof.animspeed=-100;
						}
						if ( iPlayerKey46 ==  0 ) 
						{
							if ( t.mp_playingAnimation[t.c]  !=  MP_ANIMATION_WALKINGBACKWARDS ) 
							{
								t.tgunid=t.entityelement[t.mp_playerEntityID[t.c]].eleprof.hasweapon;
								t.tweapstyle=t.gun[t.tgunid].weapontype;
								if ( t.tweapstyle > 5  )  t.tweapstyle  =  1;
								if ( t.entityelement[t.mp_playerEntityID[t.c]].attachmentobj > 0 && t.thasNade  ==  0 ) 
								{
									t.tplaycsioranimindex = 1;// 10;//t.csi_stoodmoverunANIM[t.tweapstyle];
								}
								else
								{
									t.tplaycsioranimindex = 1;// 10;//g.csi_unarmedmoverunANIM;
								}
								mp_switchDirectAnim ( t.tplaycsioranimindex );
								if ( iPlayerAppearance ==  101 ) 
								{
									t.tgunid=t.entityelement[t.mp_playerEntityID[t.c]].eleprof.hasweapon;
									t.tweapstyle=t.gun[t.tgunid].weapontype;
									if (  t.tweapstyle > 5  )  t.tweapstyle  =  1;
									t.tplaycsioranimindex = 0;//t.csi_stoodnormalANIM[t.tweapstyle];
									mp_switchDirectAnim ( t.tplaycsioranimindex );
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
							if ( t.mp_playingAnimation[t.c]  !=  MP_ANIMATION_DUCKINGWALKINGBACKWARDS ) 
							{
								t.tgunid=t.entityelement[t.mp_playerEntityID[t.c]].eleprof.hasweapon;
								t.tweapstyle=t.gun[t.tgunid].weapontype;
								if ( t.tweapstyle > 5  )  t.tweapstyle  =  1;
								if ( t.tweapstyle  ==  0  )  t.tweapstyle  =  1;
								t.tplaycsioranimindex = 1;// 10;//t.csi_crouchmoverunANIM[t.tweapstyle];
								mp_switchDirectAnim ( t.tplaycsioranimindex );
								entity_lua_setanimationframes ( );
								t.e = t.mp_playerEntityID[t.c];
								entity_lua_loopanimation ( );
								g.mp.isAnimating = 1;
								t.mp_playingAnimation[t.c] = MP_ANIMATION_DUCKINGWALKINGBACKWARDS;
							}
						}
					}

					//  strafe left
					if ( iPlayerKey30 ==  1 ) 
					{
						if ( g.mp.isAnimating  ==  0 ) 
						{
							g.mp.isAnimating = 1;
							if ( iPlayerKey42 ==  0 ) 
							{
								t.entityelement[t.mp_playerEntityID[t.c]].eleprof.animspeed=100;
							}
							else
							{
								t.entityelement[t.mp_playerEntityID[t.c]].eleprof.animspeed=150;
							}
							if ( iPlayerKey46 == 1 ) 
							{
								if ( t.mp_playingAnimation[t.c]  !=  MP_ANIMATION_DUCKINGWALKING ) 
								{
									t.tgunid=t.entityelement[t.mp_playerEntityID[t.c]].eleprof.hasweapon;
									t.tweapstyle=t.gun[t.tgunid].weapontype;
									if (  t.tweapstyle > 5  )  t.tweapstyle  =  1;
									if (  t.tweapstyle  ==  0  )  t.tweapstyle  =  1;
									t.tplaycsioranimindex = 1;// 12;//t.csi_crouchmoveleftANIM[t.tweapstyle];
									mp_switchDirectAnim ( t.tplaycsioranimindex );
									entity_lua_setanimationframes ( );
									t.e = t.mp_playerEntityID[t.c];
									entity_lua_loopanimation ( );
									g.mp.isAnimating = 1;
									t.mp_playingAnimation[t.c] = MP_ANIMATION_DUCKINGWALKING;
								}
							}
							else
							{
								if ( t.mp_playingAnimation[t.c]  !=  MP_ANIMATION_STRAFELEFT ) 
								{
									t.tgunid=t.entityelement[t.mp_playerEntityID[t.c]].eleprof.hasweapon;
									t.tweapstyle=t.gun[t.tgunid].weapontype;
									if ( t.tweapstyle > 5  )  t.tweapstyle  =  1;
									if ( t.entityelement[t.mp_playerEntityID[t.c]].attachmentobj > 0 && t.thasNade  ==  0 ) 
									{
										t.tplaycsioranimindex = 1;// 15;//t.csi_stoodmoverunleftANIM[t.tweapstyle];
									}
									else
									{
										t.tplaycsioranimindex = 1;// 10;//g.csi_unarmedmoverunANIM;
									}
									mp_switchDirectAnim ( t.tplaycsioranimindex );
									entity_lua_setanimationframes ( );
									t.e = t.mp_playerEntityID[t.c];
									entity_lua_loopanimation ( );
									t.mp_playingAnimation[t.c] = MP_ANIMATION_STRAFELEFT;
								}
							}
						}
					}

					//  strafe right
					if ( iPlayerKey32 ==  1 ) 
					{
						if ( g.mp.isAnimating  ==  0 ) 
						{
							g.mp.isAnimating = 1;
							if ( iPlayerKey42 ==  0 ) 
							{
								t.entityelement[t.mp_playerEntityID[t.c]].eleprof.animspeed=100;
							}
							else
							{
								t.entityelement[t.mp_playerEntityID[t.c]].eleprof.animspeed=150;
							}
							if ( iPlayerKey46 ==  1 ) 
							{
								if ( t.mp_playingAnimation[t.c]  !=  MP_ANIMATION_DUCKINGWALKING ) 
								{
									t.tgunid=t.entityelement[t.mp_playerEntityID[t.c]].eleprof.hasweapon;
									t.tweapstyle=t.gun[t.tgunid].weapontype;
									if ( t.tweapstyle > 5  )  t.tweapstyle  =  1;
									if ( t.tweapstyle  ==  0  )  t.tweapstyle  =  1;
									t.tplaycsioranimindex = 1;// 12;//t.csi_crouchmoverightANIM[t.tweapstyle];
									mp_switchDirectAnim ( t.tplaycsioranimindex );
									entity_lua_setanimationframes ( );
									t.e = t.mp_playerEntityID[t.c];
									entity_lua_loopanimation ( );
									g.mp.isAnimating = 1;
									t.mp_playingAnimation[t.c] = MP_ANIMATION_DUCKINGWALKING;
								}
							}
							else
							{
								if ( t.mp_playingAnimation[t.c]  !=  MP_ANIMATION_STRAFERIGHT ) 
								{
									t.tgunid=t.entityelement[t.mp_playerEntityID[t.c]].eleprof.hasweapon;
									t.tweapstyle=t.gun[t.tgunid].weapontype;
									if ( t.tweapstyle > 5  )  t.tweapstyle  =  1;
									if ( t.entityelement[t.mp_playerEntityID[t.c]].attachmentobj > 0 && t.thasNade  ==  0 ) 
									{
										t.tplaycsioranimindex = 1;// 16;//t.csi_stoodmoverunrightANIM[t.tweapstyle];
									}
									else
									{
										t.tplaycsioranimindex = 1;// 10;//g.csi_unarmedmoverunANIM;
									}
									mp_switchDirectAnim ( t.tplaycsioranimindex );
									entity_lua_setanimationframes ( );
									t.e = t.mp_playerEntityID[t.c];
									entity_lua_loopanimation ( );
									t.mp_playingAnimation[t.c] = MP_ANIMATION_STRAFERIGHT;
								}
							}
						}
					}

					// Strafing
					if ( t.mp_playingAnimation[t.c]  ==  MP_ANIMATION_STRAFELEFT ) 
					{
						if ( t.entityelement[t.mp_playerEntityID[t.c]].attachmentobj  ==  0 ) 
						{
							///RotateLimb ( t.tobj,t.spinelimbofcharacter,LimbAngleX(t.tobj,t.spinelimbofcharacter),45,LimbAngleZ(t.tobj,t.spinelimbofcharacter) );
							YRotateObject ( t.tobj, ObjectAngleY(t.tobj) - 45 );
						}
					}
					if ( t.mp_playingAnimation[t.c]  ==  MP_ANIMATION_STRAFERIGHT ) 
					{
						if ( t.entityelement[t.mp_playerEntityID[t.c]].attachmentobj  ==  0 ) 
						{
							///RotateLimb (  t.tobj,t.spinelimbofcharacter,LimbAngleX(t.tobj,t.spinelimbofcharacter),-45,LimbAngleZ(t.tobj,t.spinelimbofcharacter) );
							YRotateObject (  t.tobj, ObjectAngleY(t.tobj) + 45 );
						}
					}

					// Ducking
					if ( iPlayerKey46 == 1 && t.mp_jetpackOn[t.c] == 0 ) 
					{
						if ( g.mp.isAnimating == 0 && t.mp_reload[t.c] == 0 ) 
						{
							g.mp.isAnimating = 1;
							if ( t.mp_playingAnimation[t.c] != MP_ANIMATION_DUCKING ) 
							{
								t.tgunid=t.entityelement[t.mp_playerEntityID[t.c]].eleprof.hasweapon;
								t.tweapstyle=t.gun[t.tgunid].weapontype;
								if (  t.tweapstyle > 5  )  t.tweapstyle  =  1;
								t.tplaycsioranimindex = 6;//point forwards hack when crouch 8;//t.csi_crouchidlenormalANIM1[t.tweapstyle];
								mp_switchDirectAnim ( t.tplaycsioranimindex );
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

				// Idle
				if ( g.mp.isAnimating  ==  0 ) 
				{
					mp_update_waist_rotation ( );
					if ( t.mp_playingAnimation[t.c]  !=  MP_ANIMATION_IDLE ) 
					{
						if ( abs(t.mp_oldplayerx[t.c] - t.entityelement[t.mp_playerEntityID[t.c]].x) < 1.0 || t.tjetpacktempanim  ==  1 ) 
						{
							if ( abs(t.mp_oldplayery[t.c] - t.entityelement[t.mp_playerEntityID[t.c]].y) < 1.0 || t.tjetpacktempanim  ==  1 ) 
							{
								if ( abs(t.mp_oldplayerz[t.c] - t.entityelement[t.mp_playerEntityID[t.c]].z) < 1.0 || t.tjetpacktempanim  ==  1 ) 
								{
									t.tIsThrowingNade = 0;
									t.tgunid=t.entityelement[t.mp_playerEntityID[t.c]].eleprof.hasweapon;
									t.tweapstyle=t.gun[t.tgunid].weapontype;
									if ( t.tweapstyle > 5  )  t.tweapstyle  =  1;
									if ( t.entityelement[t.mp_playerEntityID[t.c]].attachmentobj > 0 && t.thasNade  ==  0 ) 
									{
										t.tplaycsioranimindex = 0;//t.csi_stoodnormalANIM[t.tweapstyle];
										mp_switchDirectAnim ( t.tplaycsioranimindex );
									}
									else
									{
										if ( t.thasNade  ==  1 && t.mp_playerShooting[t.c]  ==  1 ) 
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
											t.tplaycsioranimindex = 0;//g.csi_unarmedANIM0;
											mp_switchDirectAnim ( t.tplaycsioranimindex );
										}
									}
									if ( t.tIsThrowingNade  ==  0 ) 
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
					// reset the idle turn if animating
					t.mp_lastIdleReset[t.c] = 1;
				}
			}
			t.mp_oldplayerx[t.c] = t.entityelement[t.mp_playerEntityID[t.c]].x;
			t.mp_oldplayery[t.c] = t.entityelement[t.mp_playerEntityID[t.c]].y;
			t.mp_oldplayerz[t.c] = t.entityelement[t.mp_playerEntityID[t.c]].z;
		}
	}
}

void mp_switchDirectAnim ( int iAnimIndex )
{
	if ( t.mp_playerEntityID[t.c] > 0 )
	{
		t.ttentid=t.entityelement[t.mp_playerEntityID[t.c]].bankindex;
		t.e=t.entityanim[t.ttentid][iAnimIndex].start;
		t.v=t.entityanim[t.ttentid][iAnimIndex].finish;
	}
}

void mp_switchAnim ( void )
{
	if ( t.mp_playerEntityID[t.c] > 0 )
	{
		t.ttentid=t.entityelement[t.mp_playerEntityID[t.c]].bankindex;
		t.q=t.entityprofile[t.ttentid].startofaianim;
		t.e=t.entityanim[t.ttentid][t.q+t.tplaycsioranimindex].start;
		t.v=t.entityanim[t.ttentid][t.q+t.tplaycsioranimindex].finish;
	}
}

void mp_update_waist_rotation ( void )
{
	// not used
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

	t.playercontrol.jetpackhidden=0;
	t.playercontrol.jetpackmode=0;

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

					if ( t.mp_playerEntityID[g.mp.me] > 0 )
					{
						t.spinelimbofcharacter=t.entityprofile[t.entityelement[t.mp_playerEntityID[g.mp.me]].bankindex].spine;
						RotateLimb (  t.tobj,t.spinelimbofcharacter,LimbAngleX( t.tobj,t.spinelimbofcharacter),0,LimbAngleZ( t.tobj,t.spinelimbofcharacter) );
						if (  ObjectExist(g.steamplayermodelsoffset+g.mp.me+121)  ==  1 ) 
						{
							t.tweight=t.entityelement[t.e].eleprof.phyweight;
							t.tfriction=t.entityelement[t.e].eleprof.phyfriction;
							ODECreateDynamicBox (  g.steamplayermodelsoffset+g.mp.me+121,-1,0,t.tweight,t.tfriction,-1 );
						}
					}
					t.tme = g.mp.me;

					//  NON-CHARACTER, but can still have ragdoll flagged (like Zombies)
					t.ttentid=t.entityelement[t.e].bankindex;
					t.ttte = t.e;
					t.mp_playingRagdoll[t.tme] = 1;
					if ( t.mp_playerEntityID[t.tme] > 0 )
					{
						if (  t.entityelement[t.mp_playerEntityID[t.tme]].attachmentobj > 0 ) 
						{
							if (  ObjectExist(t.entityelement[t.mp_playerEntityID[t.tme]].attachmentobj)  )  DeleteObject (  t.entityelement[t.mp_playerEntityID[t.tme]].attachmentobj );
							t.entityelement[t.mp_playerEntityID[t.tme]].attachmentobj = 0;
						}
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
						t.entityelement[t.ttte].ragdollifiedforcex_f=(t.ttx_f)*0.8;
						t.entityelement[t.ttte].ragdollifiedforcey_f=(t.tty_f)*1.2;
						t.entityelement[t.ttte].ragdollifiedforcez_f=(t.ttz_f)*0.8;
						t.entityelement[t.ttte].ragdollifiedforcevalue_f=t.ttforce_f*8000.0;
						t.entityelement[t.ttte].ragdollifiedforcelimb=t.ttlimb;
					}
				}
				}
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
					t.ttt=IntersectAll(g.lightmappedobjectoffset,g.lightmappedobjectoffsetfinish,0,0,0,0,0,0,-123);
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

					PointCamera (  t.x_f,t.y_f,t.z_f );


					t.tXOldPos_f = CameraPositionX();
					t.tYOldPos_f = CameraPositionY();
					t.tZOldPos_f = CameraPositionZ();

					t.tXNewPos_f = t.x_f;
					t.tYNewPos_f = t.y_f;
					t.tZNewPos_f = t.z_f;
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
					}
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
		if ( t.mp_playerEntityID[g.mp.me] > 0 )
		{
			t.entityelement[t.mp_playerEntityID[g.mp.me]].x = ObjectPositionX(t.entityelement[t.mp_playerEntityID[g.mp.me]].obj);
			t.entityelement[t.mp_playerEntityID[g.mp.me]].y = ObjectPositionY(t.entityelement[t.mp_playerEntityID[g.mp.me]].obj);
			t.entityelement[t.mp_playerEntityID[g.mp.me]].z = ObjectPositionZ(t.entityelement[t.mp_playerEntityID[g.mp.me]].obj);
		}
}

void mp_respawn ( void )
{
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
}

void mp_getInitialPlayerCount ( void )
{
	g.mp.howmanyjoinedatstart = 0;
	for ( t.c = 0 ; t.c<=  MP_MAX_NUMBER_OF_PLAYERS-1; t.c++ )
	{
		t.tname_s = SteamGetOtherPlayerName(t.c);
		if (  t.tname_s != "Player"  )  ++g.mp.howmanyjoinedatstart;
	}
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
	mp_deleteFile("levelbank\\vegmask.dds");
	mp_deleteFile("levelbank\\vegmask.png");
	mp_deleteFile("levelbank\\vegmaskgrass.dat");
	mp_deleteFile ("levelbank\\visuals.ini");
	mp_deleteFile ("levelbank\\watermask.dds");
	mp_deleteFile ("levelbank\\watermask.png");
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
			t.entityelement[t.e].whoactivated = 0;		
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
					t.entityelement[t.e].whoactivated = 0;			
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
}

void mp_add_respawn_timed ( void )
{
	for ( t.i = 0 ; t.i<=  MP_RESPAWN_TIME_OBJECT_LIST_SIZE; t.i++ )
	{
			if (  t.mp_respawn_timed[t.i].inuse  ==  0 ) 
			{
				t.mp_respawn_timed[t.i].inuse = 1;
				t.mp_respawn_timed[t.i].e = t.e;
				t.mp_respawn_timed[t.i].time = MAXTimer();
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
}

void mp_updatePlayerInput ( void )
{
	// handle floor contact and reloading counter
	if ( t.playercontrol.plrhitfloormaterial == 0 ) 
	{
		if ( g.mp.oldfootfloortime == 0 ) g.mp.oldfootfloortime = MAXTimer();
		if ( MAXTimer()-g.mp.oldfootfloortime > 100 )  g.mp.footfloor = 0;
	}
	else
	{
		g.mp.oldfootfloortime = 0;
		g.mp.footfloor = 1;
	}
	if ( g.plrreloading == 0 ) g.mp.reloadingCount = 0;
	t.tTime = MAXTimer();
	
	// send appearance info
	if ( t.tTime - g.mp.lastSendTimeAppearance > MP_APPEARANCE_UPDATE_DELAY ) 
	{
		if ( g.plrreloading != 0 ) 
		{
			++g.mp.reloadingCount;
			if ( g.mp.reloadingCount < 4 ) 
			{
				g.mp.reloading = 1;
			}
			else
			{
				g.mp.reloading = 0;
			}
		}
		g.mp.lastSendTimeAppearance = t.tTime;
		int iSetAPlayerAppearanceValue = -1;
		if ( t.playercontrol.jetpackmode != 2 && g.mp.reloading == 0 ) 
		{
			//SteamSetPlayerAppearance ( g.mp.appearance );
			iSetAPlayerAppearanceValue = g.mp.appearance;
		}
		else
		{
			if ( t.playercontrol.jetpackmode != 0 ) 
			{
				if ( g.mp.footfloor == 1 ) 
				{
					iSetAPlayerAppearanceValue = 101;
				}
				else
				{
					iSetAPlayerAppearanceValue = 102;
				}
				g.mp.reloading = 0;
			}
			else
			{
				if ( g.mp.reloading == 1 ) 
				{
					iSetAPlayerAppearanceValue = 201;
				}
				if ( g.plrreloading == 0 ) 
				{
					g.mp.reloading = 0;
				}
			}
		}
		if ( iSetAPlayerAppearanceValue != -1 )
		{
			 PhotonSetPlayerAppearance ( iSetAPlayerAppearanceValue );
		}
	}
	if (  t.tTime - g.mp.lastSendTime < MP_INPUT_UPDATE_DELAY  )  return;
	g.mp.lastSendTime = t.tTime;

	// send movement info
	if ( g.mp.meleeOn == 0 ) 
	{
		// forward
		int iSetAPlayerKeyStateKeyValue = -1;
		int iSetAPlayerKeyStateKeyState = 0;
		bool bForwardAnim = false;
		if ( KeyState(g.keymap[17]) == 1 || KeyState(g.keymap[200]) == 1 ) bForwardAnim = true;
		if ( g.vrglobals.GGVREnabled > 0 && g.vrglobals.GGVRUsingVRSystem == 1 )
			if ( GGVR_RightController_JoyY() > 0.5 ) 
				bForwardAnim = true;
		if ( bForwardAnim == true )
		{
			iSetAPlayerKeyStateKeyValue = 17;
			iSetAPlayerKeyStateKeyState = 1;
		}
		else
		{
			iSetAPlayerKeyStateKeyValue = 17;
			iSetAPlayerKeyStateKeyState = 0;
		}
			PhotonSetKeyState ( iSetAPlayerKeyStateKeyValue, iSetAPlayerKeyStateKeyState );
		// backward
		bool bBackwardAnim = false;
		if ( KeyState(g.keymap[31]) == 1 || KeyState(g.keymap[208]) == 1 ) bBackwardAnim = true;
		if ( g.vrglobals.GGVREnabled > 0 && g.vrglobals.GGVRUsingVRSystem == 1 )
			if ( GGVR_RightController_JoyY() < -0.5)  
				bBackwardAnim = true;
		if ( bBackwardAnim == true )
		{
			iSetAPlayerKeyStateKeyValue = 31;
			iSetAPlayerKeyStateKeyState = 1;
		}
		else
		{
			iSetAPlayerKeyStateKeyValue = 31;
			iSetAPlayerKeyStateKeyState = 0;
		}
			PhotonSetKeyState ( iSetAPlayerKeyStateKeyValue, iSetAPlayerKeyStateKeyState );
		// left
		bool bLeftwardAnim = false;
		if ( KeyState(g.keymap[30]) == 1 || KeyState(g.keymap[203]) == 1 ) bLeftwardAnim = true;
		if ( bLeftwardAnim == true )
		{
			iSetAPlayerKeyStateKeyValue = 30;
			iSetAPlayerKeyStateKeyState = 1;
		}
		else
		{
			iSetAPlayerKeyStateKeyValue = 30;
			iSetAPlayerKeyStateKeyState = 0;
		}
			PhotonSetKeyState ( iSetAPlayerKeyStateKeyValue, iSetAPlayerKeyStateKeyState );
		// right
		bool bRightwardAnim = false;
		if ( KeyState(g.keymap[32]) == 1 || KeyState(g.keymap[205]) == 1 ) bRightwardAnim = true;
		if ( bRightwardAnim == true )
		{
			iSetAPlayerKeyStateKeyValue = 32;
			iSetAPlayerKeyStateKeyState = 1;
		}
		else
		{
			iSetAPlayerKeyStateKeyValue = 32;
			iSetAPlayerKeyStateKeyState = 0;
		}
			PhotonSetKeyState ( iSetAPlayerKeyStateKeyValue, iSetAPlayerKeyStateKeyState );
	}

	// ducking
	int iSetAPlayerkey46Value = - 1;
	if ( KeyState(g.keymap[46]) == 1 || KeyState(g.keymap[29]) == 1 || KeyState(g.keymap[157]) == 1 ) 
	{
		iSetAPlayerkey46Value = 1;
		g.mp.crouchOn = 1;
	}
	else
	{
		iSetAPlayerkey46Value = 0;
		g.mp.crouchOn = 0;
	}
		PhotonSetKeyState ( 46, iSetAPlayerkey46Value );

	// shift keys for running
	bool bShiftRunningAnim = false;
	if ( KeyState(g.keymap[42]) == 1 || KeyState(g.keymap[54]) == 1 ) bShiftRunningAnim = true;
	if ( g.vrglobals.GGVREnabled > 0 && g.vrglobals.GGVRUsingVRSystem == 1 )
		if ( GGVR_RightController_Grip() == 1 ) 
			bShiftRunningAnim = true;
	int iSetAPlayerkey42Value = - 1;
	if ( bShiftRunningAnim == true )
	{
		iSetAPlayerkey42Value = 1;
	}
	else
	{
		iSetAPlayerkey42Value = 0;
	}
		PhotonSetKeyState ( 42, iSetAPlayerkey42Value );
}

void mp_load_guns ( void )
{
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
				if (FileExist(t.tfile_s.Get()) == 0 && FileExist(t.tdbofile_s.Get()) == 0)
				{
					// can use new designation for weapons held (for both player and character in new system)
					t.tfile_s = "";
					t.tdbofile_s = "gamecore\\guns\\";
					t.tdbofile_s += t.tweaponname_s + "\\weapon.dbo";
				}
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
		if ( t.c != g.mp.me && t.mp_playerEntityID[t.c] > 0 ) 
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
							t.entityelement[t.e].attachmentobjfirespotlimb = -1; // always use a limb, it in turn uses LimbPosition (which takes reading from glued Wicked object)
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
									t.charanimstate.firesoundexpiry= MAXTimer()+200;
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
	if ( t.mp_playerEntityID[t.c] > 0 )
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
	}
}

void mp_NearOtherPlayers ( void )
{
	for ( t.c = 0 ; t.c<=  MP_MAX_NUMBER_OF_PLAYERS-1; t.c++ )
	{
		if ( t.c != g.mp.me ) 
		{
			t.tobj = t.entityelement[t.mp_playerEntityID[t.c]].obj;
			if ( t.tobj > 0 ) 
			{
				if ( ObjectExist(t.tobj) ) 
				{
						int iAlive = PhotonGetPlayerAlive(t.c);
					if ( iAlive == 1 ) 
					{
						t.tplrproxx_f=CameraPositionX()-ObjectPositionX(t.tobj);
						if ( g.mp.crouchOn == 0 ) 
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
						if ( t.tplrproxd_f<50.0 ) 
						{
							t.playercontrol.pushforce_f=0.5;
							t.playercontrol.pushangle_f=t.tplrproxa_f;
						}
					}
				}
			}
		}
	}
}

void mp_check_respawn_objects ( void )
{
	t.tTime = MAXTimer();
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
				t.entityelement[t.e].whoactivated = 0;
				t.entityelement[t.e].collected = 0;
				StopObject (  t.entityelement[t.e].obj );
				SetObjectFrame (  t.entityelement[t.e].obj,0 );
				ShowObject (  t.entityelement[t.e].obj );
			}
		}
	}
}

void mp_checkForEveryoneLeft ( void )
{
	if ( g.mp.howmanyjoinedatstart > 1 ) 
	{
		t.tsteamhowmanynow = 0;
		for ( t.tcount = 0 ; t.tcount<=  MP_MAX_NUMBER_OF_PLAYERS-1; t.tcount++ )
		{
			t.tname_s = SteamGetOtherPlayerName(t.tcount);
			if ( t.tname_s != "Player"  )  ++t.tsteamhowmanynow;
		}
	}
}

void mp_lostConnection ( void )
{
	t.tTime = MAXTimer();
	editor_hideall3d ( );
	SetDir ( cstr(g.fpscrootdir_s + "\\Files").Get() );
	if ( t.tsteamconnectionlostmessage_s == "GAMEOVER" )  g.mp.backtoeditorforyou = 1;
	if ( t.tsteamconnectionlostmessage_s == "" ) t.tsteamconnectionlostmessage_s = "Lost connection to server";
	if ( t.tsteamlostconnectioncustommessage_s != "" )  t.tsteamconnectionlostmessage_s = t.tsteamlostconnectioncustommessage_s;
	while ( MAXTimer() - t.tTime < 5000 ) 
	{
		Cls ( );
		mp_text(-1,30,3,t.tsteamconnectionlostmessage_s.Get());
		 PhotonLoop ( );
		Sync (  );
	}
	t.tsteamlostconnectioncustommessage_s = "";
	mp_setMessage ( );
	if ( g.mp.mode == MP_IN_GAME_CLIENT || g.mp.mode == MP_IN_GAME_SERVER || g.mp.backtoeditorforyou > 0 ) 
	{
		mp_resetGameStats ( );
		g.mp.goBackToEditor = 1;
	}
	g.mp.backtoeditorforyou = 0;
	mp_resetGameStats ( );
	mp_quitGame ( );
}

void mp_resetslotvarsforplayerarrival ( int iSlotIndex )
{
	// causes player to hide and show properly when arriving
	t.mp_forcePosition[iSlotIndex] = 1;
}

void mp_hostalwaysreadytosendplayeramapfile()
{
	// host needs to send the map to the new arrival
	if ( g.mp.syncedWithServerMode == 99 )
	{
		// if new arrival
		int iNewPlayerArrived = PhotonPlayerArrived();
		if ( iNewPlayerArrived != -1 )
		{
			// reset some slot vars
			mp_resetslotvarsforplayerarrival(PhotonGetRemap(iNewPlayerArrived));

			// triggers server to send map file
			g.mp.syncedWithServerMode = 0;
			g.mp.onlySendMapToSpecificPlayer = iNewPlayerArrived;
			t.fLastProgress = 0;
			t.tUserCount = PhotonGetLobbyUserCount();
			g.mp.usersInServersLobbyAtServerCreation = t.tUserCount;

			// also send all entity activated values (+100) so scripts can set states and
			// update level to the current state of the game logic (requires special multiplayer capable scripts)
			for ( t.e = 1; t.e <= g.entityelementlist; t.e++ )
			{
				if ( t.entityelement[t.e].staticflag == 0 && t.entityelement[t.e].obj > 0 )
				{
					mp_sendluaToPlayer ( iNewPlayerArrived, MP_LUA_SetActivated, t.e, t.entityelement[t.e].activated+100 );
				}
			}

			// resent avatar of server to new player (and others)
			g.mp.haveSentMyAvatar = 0;
		}
	}
	else
	{
		// handles server job to send map file to newly arrived player
		if ( g.mp.onlySendMapToSpecificPlayer != -1 )
		{
			mp_pre_game_file_sync_server ( g.mp.onlySendMapToSpecificPlayer );
		}
	}
}

void mp_gameLoop ( void )
{
	// check we have finished loading, if not exit out
	if ( g.mp.finishedLoadingMap == 0 ) return;

	// Find out which index we are
	 g.mp.me = PhotonGetMyPlayerIndex();

	// and only if player not in process of leaving
	 // handle player leaving
	 if ( PhotonPlayerLeaving() == true ) 
	 {
		 // only handle code to process withdrawal
		 PhotonLoop();
		 return;
	 }
	// if game already started
	if ( PhotonIsGameRunning() == 1 )
	{
		 // handle new player arriving while game is running
		 if ( g.mp.isGameHost == 1 )
		 {
			 // this will handle host sending mapfile to joiner
			 mp_hostalwaysreadytosendplayeramapfile();
		 }
		 else
		 {
			// non-host players need to send their avatars to new player
			int iNewPlayerArrived = PhotonPlayerArrived();
			if ( iNewPlayerArrived != -1 )
			{
				// reset some slot vars
				mp_resetslotvarsforplayerarrival(PhotonGetRemap(iNewPlayerArrived));

				// resent avatar of server to new player (and others)
				g.mp.haveSentMyAvatar = 0;
			}
		 }
	 }
	 // handle sending of avatar info
	 mp_sendAvatarInfo ( );

	 // if player becomes host, ensure it is flagged
	 if ( PhotonIsPlayerTheServer() == 1 )
	 {
		 g.mp.isGameHost = 1;
	 }

	// HideMouse ( when menu finished )
	if ( t.thaveShownMouse > 0 ) 
	{
		game_hidemouse ( );
		--t.thaveShownMouse;
	}

	mp_NearOtherPlayers ( );

	// if we have lost connection, head back to main menu
	 t.tconnectionStatus = PhotonGetClientServerConnectionStatus();
	if ( t.tconnectionStatus == 0 ) 
	{
		t.tsteamconnectionlostmessage_s = "GAMEOVER";
		t.tsteamlostconnectioncustommessage_s = "Level Over. The server closed.";
		mp_lostConnection ( );
		return;
	}
	if ( t.tconnectionStatus == 2 ) 
	{
		t.tsteamconnectionlostmessage_s = "GAMEOVER";
		t.tsteamlostconnectioncustommessage_s = "Level Over. The server closed.";
		mp_lostConnection ( );
		return;
	}

	mp_lua ( );

	// Hide our own player model but show everyone elses
	for ( t.a = 0 ; t.a <= MP_MAX_NUMBER_OF_PLAYERS-1; t.a++ )
	{
		if ( t.a == g.mp.me ) 
		{
			if ( t.entityelement[t.mp_playerEntityID[g.mp.me]].obj > 0 ) 
			{
				if ( ObjectExist(t.entityelement[t.mp_playerEntityID[g.mp.me]].obj) ) 
				{
					HideObject ( t.entityelement[t.mp_playerEntityID[g.mp.me]].obj );
				}
			}
		}
		else
		{
			if ( t.entityelement[t.mp_playerEntityID[t.a]].obj > 0 ) 
			{
				if ( ObjectExist(t.entityelement[t.mp_playerEntityID[t.a]].obj) ) 
				{
					 int iAlive = PhotonGetPlayerAlive(t.a);
					if ( t.mp_forcePosition[t.a] > 0 && iAlive == 1 ) 
					{
						t.mp_playerHasSpawned[t.a] = 1;
						HideObject ( t.entityelement[t.mp_playerEntityID[t.a]].obj );
						if ( t.entityelement[t.mp_playerEntityID[t.a]].attachmentobj > 0 ) 
						{
							if ( ObjectExist(t.entityelement[t.mp_playerEntityID[t.a]].attachmentobj) ==  1 )  HideObject ( t.entityelement[t.mp_playerEntityID[t.a]].attachmentobj );
						}
					}
					else
					{
						if ( iAlive == 0 ) t.mp_playerHasSpawned[t.a] = 0;
						if ( t.mp_playerHasSpawned[t.a] == 1 ) 
						{
							ShowObject ( t.entityelement[t.mp_playerEntityID[t.a]].obj );
							if ( t.entityelement[t.mp_playerEntityID[t.a]].attachmentobj > 0 ) 
							{
								if ( ObjectExist(t.entityelement[t.mp_playerEntityID[t.a]].attachmentobj) == 1 )  ShowObject ( t.entityelement[t.mp_playerEntityID[t.a]].attachmentobj );
							}
						}
						else
						{
							HideObject ( t.entityelement[t.mp_playerEntityID[t.a]].obj );
							if ( t.entityelement[t.mp_playerEntityID[t.a]].attachmentobj > 0 ) 
							{
								if ( ObjectExist(t.entityelement[t.mp_playerEntityID[t.a]].attachmentobj) == 1 )  HideObject ( t.entityelement[t.mp_playerEntityID[t.a]].attachmentobj );
							}
						}
					}
				}
			}
		}
	}

	// Player is alive
	t.tTime = MAXTimer();
	if ( t.tTime - g.mp.lastSendAliveTime > MP_ALIVE_UPDATE_DELAY ) 
	{
		g.mp.lastSendAliveTime = t.tTime;
		 PhotonSetPlayerAlive ( 1 );
	}
	mp_update_player ( );
	mp_updatePlayerPositions ( );
	mp_updatePlayerInput ( );
	mp_updatePlayerNamePlates ( );
	mp_updatePlayerAnimations ( );
	mp_loop ( );
}

// used when restarting a match so you don't see everyone dropping out of the sky
void mp_dontShowOtherPlayers ( void )
{
	for ( t.a = 0 ; t.a<=  MP_MAX_NUMBER_OF_PLAYERS-1; t.a++ )
	{
		if ( t.a != g.mp.me ) 
		{
			if ( t.entityelement[t.mp_playerEntityID[t.a]].obj > 0 ) 
			{
				if ( ObjectExist(t.entityelement[t.mp_playerEntityID[t.a]].obj) ) 
				{
					PositionObject ( t.entityelement[t.mp_playerEntityID[t.a]].obj,-100000,-100000,-100000 );
				}
			}
		}
	}
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
}

