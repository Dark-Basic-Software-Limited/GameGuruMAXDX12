void darkai_character_loop ( void )
{
	// Multiplayer game seems to want to modify original freeze distance (added by dave)
	if (  t.game.runasmultiplayer != 0 ) 
	{
		t.toriginalmaximumnonefreezedistance = t.maximumnonefreezedistance;
	}

	//  Handles inerconnection between DarkA.I and Visual Object
	for ( g.charanimindex = 1 ; g.charanimindex<=  g.charanimindexmax; g.charanimindex++ )
	{
		//  This character
		t.charanimstate = t.charanimstates[g.charanimindex];

		//  Entity Element Index for this A.I character
		t.te=t.charanimstate.e;
		if (  t.entityelement[t.te].active == 1 && t.charanimstate.obj>0 ) 
		{
		if (  ObjectExist(t.charanimstate.obj) == 1 ) 
		{
		//  if the char is controlled by someone else, we increase the range to max
		if (  t.game.runasmultiplayer  ==  1 && g.mp.coop  ==  1 ) 
		{
			if (  t.entityelement[t.te].mp_updateOn  ==  1 && t.entityelement[t.te].mp_coopControlledByPlayer  !=  g.mp.me && t.entityelement[t.te].mp_coopControlledByPlayer  !=  -1 ) 
			{
				t.maximumnonefreezedistance = 999990;
			}
			else
			{
				t.maximumnonefreezedistance = t.toriginalmaximumnonefreezedistance;
			}
		}

		//  The 'Alive' behavior of the character
		if (  t.entityelement[t.charanimstate.e].health>0 && t.entityelement[t.charanimstate.e].plrdist<t.maximumnonefreezedistance ) 
		{
			// if in LIMBO MANUAL MODE, skip all AI interference from legacy system
			if ( t.charanimstate.playcsi == g.csi_limbo && t.charanimstate.limbomanualmode == 1 )
			{
				// AI decoupled allowing AI MANUAL MODE to work unmolested (and then data read directly by LUA)
				// but still feed machine indie speed into AI system
				//t.taispeed_f = 220.0 * t.entityelement[t.charanimstate.e].speedmodulator_f;
				//t.taispeed_f = 220.0 * 2.0f * t.entityelement[t.charanimstate.e].speedmodulator_f; // 270217 - all chars seemed slow compared to old so doubled it for NEW AI MANUAL MODE
				t.taispeed_f = 220.0 * t.entityelement[t.charanimstate.e].speedmodulator_f; // 070317 - now zombies sliding, restore this and correct anything else from FPEs
				
				t.taispeedtimecap_f = g.timeelapsed_f;
				if ( t.taispeedtimecap_f<0.23f ) t.taispeedtimecap_f = 0.23f;
				t.taispeed_f = t.taispeed_f * (t.entityelement[t.charanimstate.e].eleprof.speed/100.0);
				AISetEntitySpeed ( t.charanimstate.obj, t.taispeed_f*t.taispeedtimecap_f );
			}
			else
			{
				//  get raw A.I entity position and angle
				t.tairealposx_f=AIGetEntityX(t.charanimstate.obj);
				t.tairealposz_f=AIGetEntityZ(t.charanimstate.obj);
				t.tangley_f=AIGetEntityAngleY(t.charanimstate.obj);

				// proximity check to ensure they do not enter other character entities
				entity_lua_moveforward_core_nooverlap ( t.charanimstate.e, &t.tairealposx_f, &t.tairealposz_f );
				AISetEntityPosition ( t.charanimstate.obj, t.tairealposx_f, AIGetEntityY(t.charanimstate.obj), t.tairealposz_f );

				//  get terrain ground height for A.I entity
				if (  t.terrain.TerrainID>0 ) 
				{
					t.tusecurrentgroundheight_f=BT_GetGroundHeight(t.terrain.TerrainID,t.tairealposx_f,t.tairealposz_f);
				}
				else
				{
					t.tusecurrentgroundheight_f=g.gdefaultterrainheight;
				}

				//  work out distance and angle between A.I entity and visible character object
				t.dx_f=t.tairealposx_f-ObjectPositionX(t.charanimstate.obj);
				t.dz_f=t.tairealposz_f-ObjectPositionZ(t.charanimstate.obj);
				t.dd_f=Sqrt(abs(t.dx_f*t.dx_f)+abs(t.dz_f*t.dz_f));
				t.da_f=atan2deg(t.dx_f,t.dz_f);

				//  expert system to prevent small movements when in run-mode
				if (  t.charanimstate.runmode == 1 && t.dd_f<30.0  )  t.dd_f = 0;

				//  All warm-up code removed by Rick (too over-complicated?)
				t.charanimstate.warmupwalk_f=1.0;

				//  move character using charanimcontrols flags if A.I entity somewhere else
				t.charanimstate.distancetotarget_f=t.dd_f;
				if (  t.charanimstate.playcsi != g.csi_limbo ) 
				{
					//  if (  you are less than XX units from destination AND running,  ) 
					t.tdx_f=AIGetEntityDestinationX(t.charanimstate.obj)-ObjectPositionX(t.charanimstate.obj);
					t.tdz_f=AIGetEntityDestinationZ(t.charanimstate.obj)-ObjectPositionZ(t.charanimstate.obj);
					t.tdistancebetweenentityandfinaldestination_f=Sqrt(abs(t.tdx_f*t.tdx_f)+abs(t.tdz_f*t.tdz_f));
					t.tdothestop=0;
					if (  t.tdistancebetweenentityandfinaldestination_f <= 25.0 && t.charanimstate.runmode == 0  )  t.tdothestop = 1;
					if (  t.tdistancebetweenentityandfinaldestination_f <= 75.0 && t.charanimstate.runmode == 1  )  t.tdothestop = 1;

					//  don't stop them in coop mode, as the destinations are smaller increments (stopping is handled in the steam module)
					if (  t.game.runasmultiplayer  ==  1 && g.mp.coop  ==  1 ) 
					{
						if (  t.entityelement[t.te].mp_updateOn  ==  1 && t.entityelement[t.te].mp_coopControlledByPlayer  !=  g.mp.me ) 
						{
							t.tdothestop = 0;
							t.tminrundestsize_f = 0.0;
						}
						else
						{
							t.tminrundestsize_f = 75.0;
						}
					}
					else
					{
						t.tminrundestsize_f = 75.0;
					}

					if (  t.tdothestop == 1 ) 
					{
						//  change destination to current position (so subsystem will do the stopping)
						//  and also means the waypoint system can work again!
						AISetEntityPosition (  t.charanimstate.obj,ObjectPositionX(t.charanimstate.obj),ObjectPositionY(t.charanimstate.obj),ObjectPositionZ(t.charanimstate.obj) );
						AIEntityStop (  t.charanimstate.obj );

						// 050116 - ensure we stop any subsequent movement actions (stops char moving off when active from unfreeze event)
						t.dd_f = 0.0f;
					}

					//  can only move when not in limbo
					if (  t.dd_f >= 10.0 ) 
					{
						//  moving (1-walk,2-back,3-left,4-right,5-run,11-left,12-right)
						t.tneedtostrafe=t.charanimstate.strafemode;
						if (  t.tneedtostrafe>0 ) 
						{
							if (  t.tneedtostrafe == 1  )  t.charanimcontrols[g.charanimindex].moving = 11;
							if (  t.tneedtostrafe == 2  )  t.charanimcontrols[g.charanimindex].moving = 12;
						}
						else
						{
							//  if frozen, set moving to 0
							if (  t.charanimstate.freezeallmovement  ==  0 ) 
							{
								t.tneedtorun=t.charanimstate.runmode;
								if (  t.tneedtorun == 1 && t.tdistancebetweenentityandfinaldestination_f>t.tminrundestsize_f ) 
								{
									t.charanimcontrols[g.charanimindex].moving=5;
								}
								else
								{
									t.charanimcontrols[g.charanimindex].moving=1;
								}
								//  while moving, this is the object Y angle we use
								t.charanimstate.currentangle_f=t.da_f;
							}
							else
							{
								t.charanimcontrols[g.charanimindex].moving=0;
							}
						}
					}
					else
					{
						//  not moving
						t.charanimcontrols[g.charanimindex].moving=0;
					}

					// 060116 - refresh entity angle Y from current character Y angle (for if enter limbo)
					t.entityelement[t.charanimstate.e].ry = t.charanimstate.currentangle_f;
				}
				else
				{
					//  when in limbo state, angle controlled my entityelement RY
					if ( t.charanimstate.limbomanualmode == 1 )
					{
						// should never get here, see condition further up
					}
					else
					{
						// default limbo
						t.charanimstate.currentangle_f=t.entityelement[t.charanimstate.e].ry;
						AISetEntityAngleY (  t.charanimstate.obj,t.charanimstate.currentangle_f );
					}
				}

				//  work out direct Angle between OBJECT character and player
				if ( t.game.runasmultiplayer == 0 || g.mp.coop == 0 ) 
				{
					t.tdx_f=CameraPositionX(0)-ObjectPositionX(t.charanimstate.obj);
					t.tdz_f=CameraPositionZ(0)-ObjectPositionZ(t.charanimstate.obj);
				}
				else
				{
					if (  t.entityelement[t.charanimstate.e].mp_coopControlledByPlayer  ==  g.mp.me || t.entityelement[t.te].mp_coopControlledByPlayer  ==  -1 ) 
					{
						t.tdx_f=CameraPositionX(0)-ObjectPositionX(t.charanimstate.obj);
						t.tdz_f=CameraPositionZ(0)-ObjectPositionZ(t.charanimstate.obj);
					}
					else
					{
						t.tsteamotherplayer = t.entityelement[t.mp_playerEntityID[t.entityelement[t.te].mp_coopControlledByPlayer]].obj;
						if ( t.tsteamotherplayer > 0 )
						{
							t.tdx_f=ObjectPositionX(t.tsteamotherplayer)-ObjectPositionX(t.charanimstate.obj);
							t.tdz_f=ObjectPositionZ(t.tsteamotherplayer)-ObjectPositionZ(t.charanimstate.obj);
						}
					}
				}
				t.tdirectangley_f=atan2deg(t.tdx_f,t.tdz_f);

				//  All characters aim off to side, so correct now
				t.tdirectangley_f=t.tdirectangley_f-10.0;

				//  work out movement angle, and if stood still, only rotate if significant turn
				if (  t.charanimcontrols[g.charanimindex].moving == 0 ) 
				{
					// if stood, and no waste twist data, override rotation to use head direction (gun facing)
					if (  t.charanimstate.usingcharacterposedat == 0 ) 
					{
						// 050116 - this would force stood character to rotate to face player (irrespective of engagement)
						//t.charanimstate.moveangle_f = t.tdirectangley_f;
						// 110416 - completely broke RotateToPlayer functionality (forced rotation only happens because script commanded it!)
						t.charanimstate.moveangle_f = t.charanimstate.currentangle_f;
					}

					//  work out if 'turn on spot' should animate
					t.tdiff_f=abs(WrapValue(t.charanimstate.moveangle_f)-WrapValue(t.charanimstate.currentangle_f));
					if (  t.tdiff_f>180  )  t.tdiff_f = 360-t.tdiff_f;
					if (  t.tdiff_f>20 || (t.tdiff_f>1.0 && t.charanimstate.updatemoveangle == 1) ) 
					{
						if (  t.tdiff_f>20 ) 
						{
							t.charanimcontrols[g.charanimindex].moving=13;
							if (  t.charanimstate.usingcharacterposedat == 1 ) 
							{
								t.charanimstate.moveangle_f=t.charanimstate.currentangle_f;
							}
							t.charanimstate.updatemoveangle=0;
						}
					}
				}
				else
				{
					t.charanimstate.moveangle_f=t.charanimstate.currentangle_f;
				}

				//  ensure curve does not exceed intertia max
				t.tsmoothspeed_f=2.0/g.timeelapsed_f;
				t.tnowaty_f=WrapValue(ObjectAngleY(t.charanimstate.obj));
				t.twanttobey_f=CurveAngle(t.charanimstate.moveangle_f,ObjectAngleY(t.charanimstate.obj),t.tsmoothspeed_f);
				t.tdiffy_f=t.twanttobey_f-t.tnowaty_f;
				if (  t.tdiffy_f<-180  )  t.tdiffy_f = t.tdiffy_f+360.0;
				if (  t.tdiffy_f>180  )  t.tdiffy_f = t.tdiffy_f-360.0;
				if (  abs(t.tdiffy_f)>11.0 ) 
				{
					if (  t.tdiffy_f>0 ) 
					{
						t.tdiffy_f=11.0;
					}
					else
					{
						t.tdiffy_f=-11.0;
					}
					t.twanttobey_f=t.tnowaty_f+t.tdiffy_f;
				}

				//  curve angle of visible character object to move angle
				YRotateObject (  t.charanimstate.obj,t.twanttobey_f );
				//  calculate movement speed of A.I entity
				if (  t.dd_f>40.0 ) 
				{
					t.taispeed_f=0.0;
				}
				else
				{
					if (  t.charanimstate.runmode == 1 ) 
					{
						t.taispeed_f=t.charseq[t.charanimstates[g.charanimindex].playcsi].speed_f*300.0;
						if (  t.taispeed_f<300.0  )  t.taispeed_f = 300.0;
					}
					else
					{
						t.taispeed_f=t.charseq[t.charanimstates[g.charanimindex].playcsi].speed_f*220.0;
						if (  t.taispeed_f<220.0  )  t.taispeed_f = 220.0;
					}
				}
				t.taispeed_f=t.taispeed_f*t.entityelement[t.charanimstate.e].speedmodulator_f;

				//  speed up the character a bit if its not controlled by us
				t.taispeedtimecap_f=g.timeelapsed_f;
				if (  t.taispeedtimecap_f<0.23f  )  t.taispeedtimecap_f = 0.23f;
				t.taispeed_f=t.taispeed_f*(t.entityelement[t.charanimstate.e].eleprof.speed/100.0);
				AISetEntitySpeed (  t.charanimstate.obj,t.taispeed_f*t.taispeedtimecap_f );

				//  control directly of via physics (capsule)
				t.te=t.charanimstate.e ; t.tv_f=1.0f ; entity_updatepos ( );

				//  Angle between AI BOT character and player
				t.taiangley_f=AIGetEntityAngleY(t.charanimstate.obj);
				t.taiangley_f=t.taiangley_f-ObjectAngleY(t.charanimstate.obj);
				if (  t.taiangley_f<-180  )  t.taiangley_f = t.taiangley_f+360;
				if (  t.taiangley_f>180  )  t.taiangley_f = t.taiangley_f-360;

				//  work out relative head angle between angle we want and current model angle
				t.headangley_f=t.tdirectangley_f-ObjectAngleY(t.charanimstate.obj);
				if (  t.headangley_f<-180  )  t.headangley_f = t.headangley_f+360;
				if (  t.headangley_f>180  )  t.headangley_f = t.headangley_f-360;

				//  Choose more accurate object character to player angle if close enough
				if (  abs(t.taiangley_f-t.headangley_f)>45  )  t.headangley_f = t.taiangley_f;
				if (  t.headangley_f<-90 || t.headangley_f>90  )  t.tfacingawayfromplr = 1; else t.tfacingawayfromplr = 0;
				if (  t.headangley_f<-75  )  t.headangley_f = -75;
				if (  t.headangley_f>75  )  t.headangley_f = 75;

			}
		}

		// character Animation Speed
		if ( t.charanimstate.animationspeed_f >=0.0f )
		{
			// only use reverse polarity if animationspeed is NOT negative
			if (  GetSpeed(t.charanimstate.obj)<0  )  t.polarity = -1; else t.polarity = 1;
		}
		else
		{
			// 271115 - can set animatiospeed to negative, no need for polarity
			t.polarity = 1; 
		}
		t.tfinalspeed_f=t.entityelement[t.charanimstate.e].speedmodulator_f*t.charanimstate.animationspeed_f*t.polarity*2.5*g.timeelapsed_f;
		SetObjectSpeed (  t.charanimstate.obj,t.tfinalspeed_f );

		//  call character animation system
		if (  t.playercontrol.thirdperson.enabled == 0 ) 
		{
			if (  t.entityelement[t.charanimstate.e].plrdist<t.maximumnonefreezedistance || t.entityelement[t.charanimstate.e].health <= 0 ) 
			{
				char_loop ( );
			}
		}
		else
		{
			if (  g.charanimindex != t.playercontrol.thirdperson.characterindex ) 
			{
				if (  t.entityelement[t.charanimstate.e].plrdist<t.maximumnonefreezedistance || t.entityelement[t.charanimstate.e].health <= 0 ) 
				{
					char_loop ( );
				}
			}
		}

		//  Active branch
		}
		}

		//  Store any changes
		t.charanimstates[g.charanimindex] = t.charanimstate;

	}

return;

}

void darkai_finalsettingofcharacterobjects ( void )
{
	//  Ensure the LAST thing is to override frames to create smooth remder
	for ( g.charanimindex = 1 ; g.charanimindex<=  g.charanimindexmax; g.charanimindex++ )
	{
		t.charanimstate = t.charanimstates[g.charanimindex];
		t.te=t.charanimstate.e;
		//  Handle any override smoothing (waste twist over existing animation)
		if (  t.entityelement[t.te].active == 1 && t.charanimstate.obj>0 ) 
		{
			if (  ObjectExist(t.charanimstate.obj) == 1 ) 
			{
				if (  t.charanimstate.smoothoverridedest_f>0.0 ) 
				{
					//  if not being refreshed, move frame back to center
					t.qstart_f=t.charanimstate.smoothoverrideqstart_f;
					t.qmiddle_f=t.charanimstate.smoothoverrideqmiddle_f;
					t.qfinish_f=t.charanimstate.smoothoverrideqfinish_f;
					if (  t.charanimstate.smoothoverridedest_f<0.5 ) 
					{
						if (  t.charanimstate.smoothoverridedestframe_f>t.qstart_f && t.charanimstate.smoothoverridedestframe_f <= t.qmiddle_f-0.1 ) 
						{
							t.charanimstate.smoothoverridedestframe_f=t.charanimstate.smoothoverridedestframe_f-0.1;
							if (  t.charanimstate.smoothoverridedestframe_f<t.qstart_f ) 
							{
								t.charanimstate.smoothoverridedestframe_f=t.qstart_f;
							}
						}
						if (  t.charanimstate.smoothoverridedestframe_f>t.qmiddle_f && t.charanimstate.smoothoverridedestframe_f <= t.qfinish_f ) 
						{
							t.charanimstate.smoothoverridedestframe_f=t.charanimstate.smoothoverridedestframe_f-0.1;
							if (  t.charanimstate.smoothoverridedestframe_f<t.qmiddle_f ) 
							{
								t.charanimstate.smoothoverridedestframe_f=t.qmiddle_f;
							}
						}
					}
					//  reduce persistence of this override over time
					t.charanimstates[g.charanimindex].smoothoverridedest_f=t.charanimstates[g.charanimindex].smoothoverridedest_f-g.timeelapsed_f;
					if (  t.charanimstates[g.charanimindex].smoothoverridedest_f<0.001 ) 
					{
						//  switch off override frames
						for ( t.f = 10 ; t.f<=  26; t.f++ )
						{
							SetObjectFrameEx (  t.charanimstate.obj,t.f,-1.0,1 );
						}
						t.charanimstates[g.charanimindex].smoothoverridedest_f=0;
					}
					else
					{
						//  use override frames
						for ( t.f = 10 ; t.f<=  26; t.f++ )
						{
							SetObjectFrameEx (  t.charanimstate.obj,t.f,t.charanimstate.smoothoverridedestframe_f,1 );
						}
					}
				}
			}
		}
	}

return;

}

void darkai_character_freezeall ( void )
{

	//  Used when entering menu/edit modes and characters need to be still
	for ( g.charanimindex = 1 ; g.charanimindex<=  g.charanimindexmax; g.charanimindex++ )
	{
		t.charanimstate = t.charanimstates[g.charanimindex];
		if (  t.charanimstate.obj>0 ) 
		{
			if (  ObjectExist(t.charanimstate.obj) == 1 ) 
			{
				SetObjectSpeed (  t.charanimstate.obj,0.0 );
			}
		}
	}
}

void darkai_assignanimtofield ( void )
{
	//  291014 - AI system animation sets
	if (  t.tstartofaianim >= 0 ) 
	{
		t.tdidweuseweapstyleanim=0;
		for ( t.tcsiindex = 0 ; t.tcsiindex<=  g.csi_csimax; t.tcsiindex++ )
		{
			t.tryfield_s="";
			if (  t.tcsiindex == g.csi_relaxedANIM0  )  t.tryfield_s = "csi_relaxed1";
			if (  t.tcsiindex == g.csi_relaxedANIM1  )  t.tryfield_s = "csi_relaxed2";
			if (  t.tcsiindex == g.csi_relaxedmoveforeANIM  )  t.tryfield_s = "csi_relaxedmovefore";
			if (  t.tcsiindex == g.csi_cautiousANIM  )  t.tryfield_s = "csi_cautious";
			if (  t.tcsiindex == g.csi_cautiousmoveforeANIM  )  t.tryfield_s = "csi_cautiousmovefore";
			if (  t.tcsiindex == g.csi_unarmedANIM0  )  t.tryfield_s = "csi_unarmed1";
			if (  t.tcsiindex == g.csi_unarmedANIM1  )  t.tryfield_s = "csi_unarmed2";
			if (  t.tcsiindex == g.csi_unarmedconversationANIM  )  t.tryfield_s = "csi_unarmedconversation";
			if (  t.tcsiindex == g.csi_unarmedexplainANIM  )  t.tryfield_s = "csi_unarmedexplain";
			if (  t.tcsiindex == g.csi_unarmedpointforeANIM  )  t.tryfield_s = "csi_unarmedpointfore";
			if (  t.tcsiindex == g.csi_unarmedpointbackANIM  )  t.tryfield_s = "csi_unarmedpointback";
			if (  t.tcsiindex == g.csi_unarmedpointleftANIM  )  t.tryfield_s = "csi_unarmedpointleft";
			if (  t.tcsiindex == g.csi_unarmedpointrightANIM  )  t.tryfield_s = "csi_unarmedpointright";
			if (  t.tcsiindex == g.csi_unarmedmoveforeANIM  )  t.tryfield_s = "csi_unarmedmovefore";
			if (  t.tcsiindex == g.csi_unarmedmoverunANIM  )  t.tryfield_s = "csi_unarmedmoverun";
			if (  t.tcsiindex == g.csi_unarmedstairascendANIM  )  t.tryfield_s = "csi_unarmedstairascend";
			if (  t.tcsiindex == g.csi_unarmedstairdecendANIM  )  t.tryfield_s = "csi_unarmedstairdecend";
			if (  t.tcsiindex == g.csi_unarmedladderascendANIM0  )  t.tryfield_s = "csi_unarmedladderascend1";
			if (  t.tcsiindex == g.csi_unarmedladderascendANIM1  )  t.tryfield_s = "csi_unarmedladderascend2";
			if (  t.tcsiindex == g.csi_unarmedladderascendANIM2  )  t.tryfield_s = "csi_unarmedladderascend3";
			if (  t.tcsiindex == g.csi_unarmedladderdecendANIM0  )  t.tryfield_s = "csi_unarmedladderdecend1";
			if (  t.tcsiindex == g.csi_unarmedladderdecendANIM1  )  t.tryfield_s = "csi_unarmedladderdecend2";
			if (  t.tcsiindex == g.csi_unarmeddeathANIM  )  t.tryfield_s = "csi_unarmeddeath";
			if (  t.tcsiindex == g.csi_unarmedimpactforeANIM  )  t.tryfield_s = "csi_unarmedimpactfore";
			if (  t.tcsiindex == g.csi_unarmedimpactbackANIM  )  
			{
				t.tryfield_s = "csi_unarmedimpactback";
			}
			if (  t.tcsiindex == g.csi_unarmedimpactleftANIM  )  t.tryfield_s = "csi_unarmedimpactleft";
			if (  t.tcsiindex == g.csi_unarmedimpactrightANIM  )  t.tryfield_s = "csi_unarmedimpactright";
			if (  t.tcsiindex == g.csi_inchairANIM  )  t.tryfield_s = "csi_inchair";
			if (  t.tcsiindex == g.csi_inchairsitANIM  )  t.tryfield_s = "csi_inchairsit";
			if (  t.tcsiindex == g.csi_inchairgetupANIM  )  t.tryfield_s = "csi_inchairgetup";
			if (  t.tcsiindex == g.csi_swimANIM  )  t.tryfield_s = "csi_swim";
			if (  t.tcsiindex == g.csi_swimmoveforeANIM  )  t.tryfield_s = "csi_swimmovefore";
			t.strwork = "";
			bool bIsVaultAnim = false;
			for ( t.tweapsty = 1 ; t.tweapsty<=  5; t.tweapsty++ )
			{
				if (  t.tweapsty == 1  )  t.tweapsty_s = "";
				if (  t.tweapsty == 2  )  t.tweapsty_s = "_rocket";
				if (  t.tweapsty == 3  )  t.tweapsty_s = "_shotgun";
				if (  t.tweapsty == 4  )  t.tweapsty_s = "_uzi";
				if (  t.tweapsty == 5  )  t.tweapsty_s = "_assault";
				if (  t.tcsiindex == t.csi_stoodnormalANIM[t.tweapsty]  )  t.tryfield_s = t.strwork + "csi"+t.tweapsty_s+"_stoodnormal";
				if (  t.tcsiindex == t.csi_stoodrocketANIM[t.tweapsty]  )  t.tryfield_s = t.strwork + "csi"+t.tweapsty_s+"_stoodrocket";
				if (  t.tcsiindex == t.csi_stoodfidgetANIM0[t.tweapsty]  )  t.tryfield_s = t.strwork + "csi"+t.tweapsty_s+"_stoodfidget1";
				if (  t.tcsiindex == t.csi_stoodfidgetANIM1[t.tweapsty]  )  t.tryfield_s = t.strwork + "csi"+t.tweapsty_s+"_stoodfidget2";
				if (  t.tcsiindex == t.csi_stoodfidgetANIM2[t.tweapsty]  )  t.tryfield_s = t.strwork + "csi"+t.tweapsty_s+"_stoodfidget3";
				if (  t.tcsiindex == t.csi_stoodfidgetANIM3[t.tweapsty]  )  t.tryfield_s = t.strwork + "csi"+t.tweapsty_s+"_stoodfidget4";
				if (  t.tcsiindex == t.csi_stoodstartledANIM[t.tweapsty]  )  t.tryfield_s = t.strwork + "csi"+t.tweapsty_s+"_stoodstartled";
				if (  t.tcsiindex == t.csi_stoodpunchANIM[t.tweapsty]  )  t.tryfield_s = t.strwork + "csi"+t.tweapsty_s+"_stoodpunch";
				if (  t.tcsiindex == t.csi_stoodkickANIM[t.tweapsty]  )  t.tryfield_s = t.strwork + "csi"+t.tweapsty_s+"_stoodkick";
				if (  t.tcsiindex == t.csi_stoodmoveforeANIM[t.tweapsty]  )  
					t.tryfield_s = t.strwork + "csi"+t.tweapsty_s+"_stoodmovefore";
				if (  t.tcsiindex == t.csi_stoodmovebackANIM[t.tweapsty]  )  t.tryfield_s = t.strwork + "csi"+t.tweapsty_s+"_stoodmoveback";
				if (  t.tcsiindex == t.csi_stoodmoveleftANIM[t.tweapsty]  )  t.tryfield_s = t.strwork + "csi"+t.tweapsty_s+"_stoodmoveleft";
				if (  t.tcsiindex == t.csi_stoodmoverightANIM[t.tweapsty]  )  t.tryfield_s = t.strwork + "csi"+t.tweapsty_s+"_stoodmoveright";
				if (  t.tcsiindex == t.csi_stoodstepleftANIM[t.tweapsty]  )  t.tryfield_s = t.strwork + "csi"+t.tweapsty_s+"_stoodstepleft";
				if (  t.tcsiindex == t.csi_stoodsteprightANIM[t.tweapsty]  )  t.tryfield_s = t.strwork + "csi"+t.tweapsty_s+"_stoodstepright";
				if (  t.tcsiindex == t.csi_stoodstrafeleftANIM[t.tweapsty]  )  t.tryfield_s = t.strwork + "csi"+t.tweapsty_s+"_stoodstrafeleft";
				if (  t.tcsiindex == t.csi_stoodstraferightANIM[t.tweapsty]  )  t.tryfield_s = t.strwork + "csi"+t.tweapsty_s+"_stoodstraferight";
				if (  t.tcsiindex == t.csi_stoodvaultANIM[t.tweapsty]  )  { bIsVaultAnim = true; t.tryfield_s = t.strwork + "csi"+t.tweapsty_s+"_stoodvault"; }
				if (  t.tcsiindex == t.csi_stoodmoverunANIM[t.tweapsty]  )  t.tryfield_s = t.strwork + "csi"+t.tweapsty_s+"_stoodmoverun";
				if (  t.tcsiindex == t.csi_stoodmoverunleftANIM[t.tweapsty]  )  t.tryfield_s = t.strwork + "csi"+t.tweapsty_s+"_stoodmoverunleft";
				if (  t.tcsiindex == t.csi_stoodmoverunrightANIM[t.tweapsty]  )  t.tryfield_s = t.strwork + "csi"+t.tweapsty_s+"_stoodmoverunright";
				if (  t.tcsiindex == t.csi_stoodreloadANIM[t.tweapsty]  )  t.tryfield_s = t.strwork + "csi"+t.tweapsty_s+"_stoodreload";
				if (  t.tcsiindex == t.csi_stoodreloadrocketANIM[t.tweapsty]  )  t.tryfield_s = t.strwork + "csi"+t.tweapsty_s+"_stoodreloadrocket";
				if (  t.tcsiindex == t.csi_stoodwaveANIM[t.tweapsty]  )  t.tryfield_s = t.strwork + "csi"+t.tweapsty_s+"_stoodwave";
				if (  t.tcsiindex == t.csi_stoodtossANIM[t.tweapsty]  )  t.tryfield_s = t.strwork + "csi"+t.tweapsty_s+"_stoodtoss";
				if (  t.tcsiindex == t.csi_stoodfirerocketANIM[t.tweapsty]  )  t.tryfield_s = t.strwork + "csi"+t.tweapsty_s+"_stoodfirerocket";
				if (  t.tcsiindex == t.csi_stoodincoverleftANIM[t.tweapsty]  )  t.tryfield_s = t.strwork + "csi"+t.tweapsty_s+"_stoodincoverleft";
				if (  t.tcsiindex == t.csi_stoodincoverpeekleftANIM[t.tweapsty]  )  t.tryfield_s = t.strwork + "csi"+t.tweapsty_s+"_stoodincoverpeekleft";
				if (  t.tcsiindex == t.csi_stoodincoverthrowleftANIM[t.tweapsty]  )  t.tryfield_s = t.strwork + "csi"+t.tweapsty_s+"_stoodincoverthrowleft";
				if (  t.tcsiindex == t.csi_stoodincoverrightANIM[t.tweapsty]  )  t.tryfield_s = t.strwork + "csi"+t.tweapsty_s+"_stoodincoverright";
				if (  t.tcsiindex == t.csi_stoodincoverpeekrightANIM[t.tweapsty]  )  t.tryfield_s = t.strwork + "csi"+t.tweapsty_s+"_stoodincoverpeekright";
				if (  t.tcsiindex == t.csi_stoodincoverthrowrightANIM[t.tweapsty]  )  t.tryfield_s = t.strwork + "csi"+t.tweapsty_s+"_stoodincoverthrowright";
				if (  t.tcsiindex == t.csi_stoodandturnANIM[t.tweapsty]  )  t.tryfield_s = t.strwork + "csi"+t.tweapsty_s+"_stoodandturn";
				if (  t.tcsiindex == t.csi_crouchidlenormalANIM0[t.tweapsty]  )  t.tryfield_s = t.strwork + "csi"+t.tweapsty_s+"_crouchidlenormal1";
				if (  t.tcsiindex == t.csi_crouchidlenormalANIM1[t.tweapsty]  )  t.tryfield_s = t.strwork + "csi"+t.tweapsty_s+"_crouchidlenormal2";
				if (  t.tcsiindex == t.csi_crouchidlerocketANIM[t.tweapsty]  )  t.tryfield_s = t.strwork + "csi"+t.tweapsty_s+"_crouchidlerocket";
				if (  t.tcsiindex == t.csi_crouchdownANIM[t.tweapsty]  )  t.tryfield_s = t.strwork + "csi"+t.tweapsty_s+"_crouchdown";
				if (  t.tcsiindex == t.csi_crouchdownrocketANIM[t.tweapsty]  )  t.tryfield_s = t.strwork + "csi"+t.tweapsty_s+"_crouchdownrocket";
				if (  t.tcsiindex == t.csi_crouchrolldownANIM[t.tweapsty]  )  t.tryfield_s = t.strwork + "csi"+t.tweapsty_s+"_crouchrolldown";
				if (  t.tcsiindex == t.csi_crouchrollupANIM[t.tweapsty]  )  t.tryfield_s = t.strwork + "csi"+t.tweapsty_s+"_crouchrollup";
				if (  t.tcsiindex == t.csi_crouchmoveforeANIM[t.tweapsty]  )  t.tryfield_s = t.strwork + "csi"+t.tweapsty_s+"_crouchmovefore";
				if (  t.tcsiindex == t.csi_crouchmovebackANIM[t.tweapsty]  )  t.tryfield_s = t.strwork + "csi"+t.tweapsty_s+"_crouchmoveback";
				if (  t.tcsiindex == t.csi_crouchmoveleftANIM[t.tweapsty]  )  t.tryfield_s = t.strwork + "csi"+t.tweapsty_s+"_crouchmoveleft";
				if (  t.tcsiindex == t.csi_crouchmoverightANIM[t.tweapsty]  )  t.tryfield_s = t.strwork + "csi"+t.tweapsty_s+"_crouchmoveright";
				if (  t.tcsiindex == t.csi_crouchmoverunANIM[t.tweapsty]  )  t.tryfield_s = t.strwork + "csi"+t.tweapsty_s+"_crouchmoverun";
				if (  t.tcsiindex == t.csi_crouchreloadANIM[t.tweapsty]  )  t.tryfield_s = t.strwork + "csi"+t.tweapsty_s+"_crouchreload";
				if (  t.tcsiindex == t.csi_crouchreloadrocketANIM[t.tweapsty]  )  t.tryfield_s = t.strwork + "csi"+t.tweapsty_s+"_crouchreloadrocket";
				if (  t.tcsiindex == t.csi_crouchwaveANIM[t.tweapsty]  )  t.tryfield_s = t.strwork + "csi"+t.tweapsty_s+"_crouchwave";
				if (  t.tcsiindex == t.csi_crouchtossANIM[t.tweapsty]  )  t.tryfield_s = t.strwork + "csi"+t.tweapsty_s+"_crouchtoss";
				if (  t.tcsiindex == t.csi_crouchfirerocketANIM[t.tweapsty]  )  t.tryfield_s = t.strwork + "csi"+t.tweapsty_s+"_crouchfirerocket";
				if (  t.tcsiindex == t.csi_crouchimpactforeANIM[t.tweapsty]  )  t.tryfield_s = t.strwork + "csi"+t.tweapsty_s+"_crouchimpactfore";
				if (  t.tcsiindex == t.csi_crouchimpactbackANIM[t.tweapsty]  )  t.tryfield_s = t.strwork + "csi"+t.tweapsty_s+"_crouchimpactback";
				if (  t.tcsiindex == t.csi_crouchimpactleftANIM[t.tweapsty]  )  t.tryfield_s = t.strwork + "csi"+t.tweapsty_s+"_crouchimpactleft";
				if (  t.tcsiindex == t.csi_crouchimpactrightANIM[t.tweapsty]  )  t.tryfield_s = t.strwork + "csi"+t.tweapsty_s+"_crouchimpactright";
				if (  t.tcsiindex == t.csi_crouchgetupANIM[t.tweapsty]  )  t.tryfield_s = t.strwork + "csi"+t.tweapsty_s+"_crouchgetup";
				if (  t.tcsiindex == t.csi_crouchgetuprocketANIM[t.tweapsty]  )  t.tryfield_s = t.strwork + "csi"+t.tweapsty_s+"_crouchgetuprocket";
			}
			if (  t.field_s == t.tryfield_s && t.field_s != "" ) 
			{
				t.q=t.tstartofaianim+t.tcsiindex;
				if (  t.q <= g.animmax ) 
				{
					if (  t.entityanim[t.entid][t.q].start == 0 ) 
					{
						// ensure VAULT animation trims start and end
						if ( bIsVaultAnim == true && t.entityprofile[t.entid].jumpvaulttrim == 1 && t.value1 > 0 )
						{
							// trim ends of animation (vault needs to be quicker for jump usage)
							t.value1 += 10;
							t.value2 -= 10;
						}

						//  only use the first occurance (FPE entries first, Default Values second)
						t.entityanim[t.entid][t.q].start=t.value1 ; t.entityanim[t.entid][t.q].finish=t.value2;
						if (  t.q>t.tnewanimmax  )  t.tnewanimmax = t.q;
						//  if using extra anims from weapstyle, then flag this next gen entity for later
						if (  t.tcsiindex >= t.csi_stoodnormalANIM[2]  ) 
							t.entityprofile[t.entid].usesweapstyleanims = 1;
					}
				}
			}
		}
	}
}

void char_init ( void )
{
	//  Create array to hold transition information for per-object
	t.tmaxobjectnumber=90000; // allows 20,000 entities in each level
	Dim (  t.smoothanim,t.tmaxobjectnumber );

	//  Create arrays for object animation engine used only for characters
	t.tmaxcharacterstateengineentities=10;
	Dim (  t.charanimcontrols,t.tmaxcharacterstateengineentities );
	Dim (  t.charanimstates,t.tmaxcharacterstateengineentities );

	//  Create state engine database for character animations
	char_createseqdata ( );
}

void char_createseqdata ( void )
{

	//  Create character anim sequence database
	t.tcharseqmax=750;
	Dim (  t.charseq,t.tcharseqmax  );
	g.csi_choosealertstate = -1;

	//  Initial state is LIMBO (corpse characters)
	t.csi=0;
	g.csi_limbo = t.csi;
	t.charseq[t.csi].mode=0 ; t.charseq[t.csi].trigger=0 ; ++t.csi;

	g.csi_relaxed = t.csi;
	t.charseq[t.csi].mode=91 ; t.charseq[t.csi].a_f=2 ; ++t.csi;
	g.csi_relaxedANIM0 = t.csi;
	t.charseq[t.csi].mode=1 ; t.charseq[t.csi].trigger=3 ; t.charseq[t.csi].a_f=900 ; t.charseq[t.csi].b_f=999 ; t.charseq[t.csi].c_f=10 ; ++t.csi;
	t.charseq[t.csi].mode=92 ; t.charseq[t.csi].a_f=g.csi_relaxed ; ++t.csi;
	g.csi_relaxedANIM1 = t.csi;
	t.charseq[t.csi].mode=1 ; t.charseq[t.csi].trigger=3 ; t.charseq[t.csi].a_f=1000 ; t.charseq[t.csi].b_f=1282 ; t.charseq[t.csi].c_f=10 ; ++t.csi;
	t.charseq[t.csi].mode=92 ; t.charseq[t.csi].a_f=g.csi_relaxed ; ++t.csi;
	g.csi_relaxedmovefore = t.csi;
	g.csi_relaxedmoveforeANIM = t.csi;
	t.charseq[t.csi].mode=5 ; t.charseq[t.csi].trigger=3 ; t.charseq[t.csi].a_f=1290 ; t.charseq[t.csi].b_f=1320 ; t.charseq[t.csi].c_f=10 ; t.charseq[t.csi].angle_f=0.0 ; t.charseq[t.csi].speed_f=1.0 ; ++t.csi;

	g.csi_cautious = t.csi;
	g.csi_cautiousANIM = t.csi;
	t.charseq[t.csi].mode=1 ; t.charseq[t.csi].trigger=9 ; t.charseq[t.csi].a_f=900 ; t.charseq[t.csi].b_f=999 ; t.charseq[t.csi].c_f=10 ; ++t.csi;
	t.charseq[t.csi].mode=92 ; t.charseq[t.csi].a_f=g.csi_cautious ; ++t.csi;
	g.csi_cautiousmovefore = t.csi;
	g.csi_cautiousmoveforeANIM = t.csi;
	t.charseq[t.csi].mode=4 ; t.charseq[t.csi].trigger=9 ; t.charseq[t.csi].a_f=1325 ; t.charseq[t.csi].b_f=1419 ; t.charseq[t.csi].c_f=10 ; t.charseq[t.csi].angle_f=0.0 ; t.charseq[t.csi].speed_f=1.0 ; ++t.csi;
	t.charseq[t.csi].mode=92 ; t.charseq[t.csi].a_f=g.csi_cautiousmovefore ; ++t.csi;

	g.csi_unarmed = t.csi;
	t.charseq[t.csi].mode=91 ; t.charseq[t.csi].a_f=2 ; ++t.csi;
	g.csi_unarmedANIM0 = t.csi;
	t.charseq[t.csi].mode=1 ; t.charseq[t.csi].trigger=4 ; t.charseq[t.csi].a_f=3000 ; t.charseq[t.csi].b_f=3100 ; t.charseq[t.csi].c_f=10 ; t.charseq[t.csi].freeze=1 ; ++t.csi;
	t.charseq[t.csi].mode=92 ; t.charseq[t.csi].a_f=g.csi_unarmed ; ++t.csi;
	g.csi_unarmedANIM1 = t.csi;
	t.charseq[t.csi].mode=1 ; t.charseq[t.csi].trigger=4 ; t.charseq[t.csi].a_f=3430 ; t.charseq[t.csi].b_f=3697 ; t.charseq[t.csi].c_f=10 ; t.charseq[t.csi].freeze=1 ; ++t.csi;
	t.charseq[t.csi].mode=92 ; t.charseq[t.csi].a_f=g.csi_unarmed ; ++t.csi;
	g.csi_unarmedconversation = t.csi;
	g.csi_unarmedconversationANIM = t.csi;
	t.charseq[t.csi].mode=1 ; t.charseq[t.csi].trigger=4 ; t.charseq[t.csi].a_f=3110 ; t.charseq[t.csi].b_f=3420 ; t.charseq[t.csi].c_f=10 ; t.charseq[t.csi].freeze=1 ; ++t.csi;
	t.charseq[t.csi].mode=92 ; t.charseq[t.csi].a_f=g.csi_unarmed ; ++t.csi;
	g.csi_unarmedexplain = t.csi;
	g.csi_unarmedexplainANIM = t.csi;
	t.charseq[t.csi].mode=1 ; t.charseq[t.csi].trigger=4 ; t.charseq[t.csi].a_f=4260 ; t.charseq[t.csi].b_f=4464 ; t.charseq[t.csi].c_f=10 ; t.charseq[t.csi].freeze=1 ; ++t.csi;
	t.charseq[t.csi].mode=92 ; t.charseq[t.csi].a_f=g.csi_unarmed ; ++t.csi;
	g.csi_unarmedpointfore = t.csi;
	g.csi_unarmedpointforeANIM = t.csi;
	t.charseq[t.csi].mode=1 ; t.charseq[t.csi].trigger=4 ; t.charseq[t.csi].a_f=4470 ; t.charseq[t.csi].b_f=4535 ; t.charseq[t.csi].c_f=10 ; t.charseq[t.csi].freeze=1 ; ++t.csi;
	t.charseq[t.csi].mode=92 ; t.charseq[t.csi].a_f=g.csi_unarmed ; ++t.csi;
	g.csi_unarmedpointback = t.csi;
	g.csi_unarmedpointbackANIM = t.csi;
	t.charseq[t.csi].mode=1 ; t.charseq[t.csi].trigger=4 ; t.charseq[t.csi].a_f=4680 ; t.charseq[t.csi].b_f=4745 ; t.charseq[t.csi].c_f=10 ; t.charseq[t.csi].freeze=1 ; ++t.csi;
	t.charseq[t.csi].mode=92 ; t.charseq[t.csi].a_f=g.csi_unarmed ; ++t.csi;
	g.csi_unarmedpointleft = t.csi;
	g.csi_unarmedpointleftANIM = t.csi;
	t.charseq[t.csi].mode=1 ; t.charseq[t.csi].trigger=4 ; t.charseq[t.csi].a_f=4610 ; t.charseq[t.csi].b_f=4675 ; t.charseq[t.csi].c_f=10 ; t.charseq[t.csi].freeze=1 ; ++t.csi;
	t.charseq[t.csi].mode=92 ; t.charseq[t.csi].a_f=g.csi_unarmed ; ++t.csi;
	g.csi_unarmedpointright = t.csi;
	g.csi_unarmedpointrightANIM = t.csi;
	t.charseq[t.csi].mode=1 ; t.charseq[t.csi].trigger=4 ; t.charseq[t.csi].a_f=4540 ; t.charseq[t.csi].b_f=4605 ; t.charseq[t.csi].c_f=10 ; t.charseq[t.csi].freeze=1 ; ++t.csi;
	t.charseq[t.csi].mode=92 ; t.charseq[t.csi].a_f=g.csi_unarmed ; ++t.csi;
	g.csi_unarmedmovefore = t.csi;
	g.csi_unarmedmoveforeANIM = t.csi;
	t.charseq[t.csi].mode=5 ; t.charseq[t.csi].trigger=4 ; t.charseq[t.csi].a_f=3870 ; t.charseq[t.csi].b_f=3900 ; t.charseq[t.csi].c_f=20 ; t.charseq[t.csi].angle_f=0.0 ; t.charseq[t.csi].speed_f=1.0 ; ++t.csi;
	t.charseq[t.csi].mode=92 ; t.charseq[t.csi].a_f=g.csi_unarmedmovefore ; ++t.csi;
	g.csi_unarmedmoverun = t.csi;
	g.csi_unarmedmoverunANIM = t.csi;
	t.charseq[t.csi].mode=5 ; t.charseq[t.csi].trigger=4 ; t.charseq[t.csi].a_f=3905 ; t.charseq[t.csi].b_f=3925 ; t.charseq[t.csi].c_f=10 ; t.charseq[t.csi].angle_f=0.0 ; t.charseq[t.csi].speed_f=3.0 ; ++t.csi;
	t.charseq[t.csi].mode=92 ; t.charseq[t.csi].a_f=g.csi_unarmedmoverun ; ++t.csi;
	g.csi_unarmedstairascend = t.csi;
	g.csi_unarmedstairascendANIM = t.csi;
	t.charseq[t.csi].mode=1 ; t.charseq[t.csi].loopback=100 ; t.charseq[t.csi].a_f=5600 ; t.charseq[t.csi].b_f=5768 ; t.charseq[t.csi].c_f=10 ; t.charseq[t.csi].angle_f=0.0 ; t.charseq[t.csi].speed_f=3.0 ; t.charseq[t.csi].freeze=1 ; ++t.csi;
	t.charseq[t.csi].mode=3 ; t.charseq[t.csi].speed_f=110.0 ; t.charseq[t.csi].angle_f=0 ; t.charseq[t.csi].a_f=100.0 ; ++t.csi;
	t.charseq[t.csi].mode=54 ; ++t.csi;
	t.charseq[t.csi].mode=92 ; t.charseq[t.csi].a_f=g.csi_choosealertstate ; ++t.csi;
	g.csi_unarmedstairdecend = t.csi;
	g.csi_unarmedstairdecendANIM = t.csi;
	t.charseq[t.csi].mode=1 ; t.charseq[t.csi].loopback=100 ; t.charseq[t.csi].a_f=5800 ; t.charseq[t.csi].b_f=5965 ; t.charseq[t.csi].c_f=10 ; t.charseq[t.csi].angle_f=0.0 ; t.charseq[t.csi].speed_f=3.0 ; t.charseq[t.csi].freeze=1 ; ++t.csi;
	t.charseq[t.csi].mode=3 ; t.charseq[t.csi].speed_f=110.0 ; t.charseq[t.csi].angle_f=0 ; t.charseq[t.csi].a_f=-100.0 ; ++t.csi;
	t.charseq[t.csi].mode=54 ; ++t.csi;
	t.charseq[t.csi].mode=92 ; t.charseq[t.csi].a_f=g.csi_choosealertstate ; ++t.csi;
	g.csi_unarmedladderascend = t.csi;
	t.charseq[t.csi].mode=3 ; t.charseq[t.csi].speed_f=15.0 ; t.charseq[t.csi].angle_f=0 ; ++t.csi;
	g.csi_unarmedladderascendANIM0 = t.csi;
	t.charseq[t.csi].mode=4 ; t.charseq[t.csi].trigger=8 ; t.charseq[t.csi].loopback=4148 ; t.charseq[t.csi].a_f=4110 ; t.charseq[t.csi].b_f=4148 ; t.charseq[t.csi].c_f=1 ; t.charseq[t.csi].angle_f=0.0f ; t.charseq[t.csi].vertspeed_f=0.45f ; t.charseq[t.csi].freeze=1 ; ++t.csi;
	g.csi_unarmedladderascendANIM1 = t.csi;
	t.charseq[t.csi].mode=5 ; t.charseq[t.csi].trigger=8 ; t.charseq[t.csi].a_f=4148 ; t.charseq[t.csi].b_f=4225 ; t.charseq[t.csi].c_f=1 ; t.charseq[t.csi].angle_f=0.0 ; t.charseq[t.csi].vertspeed_f=0.5 ; t.charseq[t.csi].freeze=1 ; ++t.csi;
	t.charseq[t.csi].mode=53 ; t.charseq[t.csi].advancecap_f=10 ; t.charseq[t.csi].freeze=1 ; ++t.csi;
	g.csi_unarmedladderascendANIM2 = t.csi;
	t.charseq[t.csi].mode=4 ; t.charseq[t.csi].trigger=8 ; t.charseq[t.csi].loopback=0 ; t.charseq[t.csi].a_f=4225 ; t.charseq[t.csi].b_f=4255 ; t.charseq[t.csi].c_f=4 ; t.charseq[t.csi].angle_f=0.0f ; t.charseq[t.csi].speed_f=0.9f ; t.charseq[t.csi].vertspeed_f=0.9f ; t.charseq[t.csi].freeze=1 ; ++t.csi;
	t.charseq[t.csi].mode=3 ; t.charseq[t.csi].speed_f=10.0 ; t.charseq[t.csi].angle_f=0 ; ++t.csi;
	t.charseq[t.csi].mode=1 ; t.charseq[t.csi].loopback=0 ; t.charseq[t.csi].a_f=100 ; t.charseq[t.csi].b_f=101 ; t.charseq[t.csi].c_f=4 ; t.charseq[t.csi].angle_f=0.0f ; t.charseq[t.csi].speed_f=0.9f ; t.charseq[t.csi].freeze=1 ; ++t.csi;
	t.charseq[t.csi].mode=54 ; ++t.csi;
	t.charseq[t.csi].mode=92 ; t.charseq[t.csi].a_f=g.csi_choosealertstate ; ++t.csi;
	g.csi_unarmedladderdecend = t.csi;
	t.charseq[t.csi].mode=3 ; t.charseq[t.csi].speed_f=-10.0 ; t.charseq[t.csi].angle_f=0 ; ++t.csi;
	t.charseq[t.csi].mode=53 ; t.charseq[t.csi].advancecap_f=-10 ; t.charseq[t.csi].freeze=1 ; ++t.csi;
	g.csi_unarmedladderdecendANIM0 = t.csi;
	t.charseq[t.csi].mode=4 ; t.charseq[t.csi].trigger=8 ; t.charseq[t.csi].loopback=0 ; t.charseq[t.csi].a_f=4255 ; t.charseq[t.csi].b_f=4225 ; t.charseq[t.csi].c_f=4 ; t.charseq[t.csi].angle_f=0.0 ; t.charseq[t.csi].speed_f=0.9f ; t.charseq[t.csi].vertspeed_f=0.9f ; t.charseq[t.csi].freeze=1 ; ++t.csi;
	g.csi_unarmedladderdecendANIM1 = t.csi;
	t.charseq[t.csi].mode=5 ; t.charseq[t.csi].trigger=8 ; t.charseq[t.csi].a_f=4225 ; t.charseq[t.csi].b_f=4148 ; t.charseq[t.csi].c_f=1 ; t.charseq[t.csi].angle_f=0.0 ; t.charseq[t.csi].vertspeed_f=0.5 ; t.charseq[t.csi].freeze=1 ; ++t.csi;
	t.charseq[t.csi].mode=3 ; t.charseq[t.csi].speed_f=-15.0 ; t.charseq[t.csi].angle_f=0 ; ++t.csi;
	t.charseq[t.csi].mode=1 ; t.charseq[t.csi].loopback=0 ; t.charseq[t.csi].a_f=100 ; t.charseq[t.csi].b_f=101 ; t.charseq[t.csi].c_f=4 ; t.charseq[t.csi].angle_f=0.0f ; t.charseq[t.csi].speed_f=0.9f ; t.charseq[t.csi].freeze=1 ; ++t.csi;
	t.charseq[t.csi].mode=54 ; ++t.csi;
	t.charseq[t.csi].mode=92 ; t.charseq[t.csi].a_f=g.csi_choosealertstate ; ++t.csi;
	g.csi_unarmeddeath = t.csi;
	g.csi_unarmeddeathANIM = t.csi;
	t.charseq[t.csi].mode=1 ; t.charseq[t.csi].trigger=0 ; t.charseq[t.csi].a_f=4800 ; t.charseq[t.csi].b_f=4958 ; t.charseq[t.csi].c_f=10 ; t.charseq[t.csi].freeze=1 ; ++t.csi;
	t.charseq[t.csi].mode=92 ; t.charseq[t.csi].a_f=g.csi_limbo ; ++t.csi;
	g.csi_unarmedimpactfore = t.csi;
	g.csi_unarmedimpactforeANIM = t.csi;
	t.charseq[t.csi].mode=1 ; t.charseq[t.csi].loopback=0 ; t.charseq[t.csi].a_f=4971 ; t.charseq[t.csi].b_f=5021 ; t.charseq[t.csi].c_f=3 ; t.charseq[t.csi].freeze=1 ; ++t.csi;
	t.charseq[t.csi].mode=92 ; t.charseq[t.csi].a_f=g.csi_limbo ; ++t.csi;
	g.csi_unarmedimpactback = t.csi;
	g.csi_unarmedimpactbackANIM = t.csi;
	t.charseq[t.csi].mode=1 ; t.charseq[t.csi].loopback=0 ; t.charseq[t.csi].a_f=5031 ; t.charseq[t.csi].b_f=5090 ; t.charseq[t.csi].c_f=3 ; t.charseq[t.csi].freeze=1 ; ++t.csi;
	t.charseq[t.csi].mode=92 ; t.charseq[t.csi].a_f=g.csi_limbo ; ++t.csi;
	g.csi_unarmedimpactleft = t.csi;
	g.csi_unarmedimpactleftANIM = t.csi;
	t.charseq[t.csi].mode=1 ; t.charseq[t.csi].loopback=0 ; t.charseq[t.csi].a_f=5171 ; t.charseq[t.csi].b_f=5229 ; t.charseq[t.csi].c_f=3 ; t.charseq[t.csi].freeze=1 ; ++t.csi;
	t.charseq[t.csi].mode=92 ; t.charseq[t.csi].a_f=g.csi_limbo ; ++t.csi;
	g.csi_unarmedimpactright = t.csi;
	g.csi_unarmedimpactrightANIM = t.csi;
	t.charseq[t.csi].mode=1 ; t.charseq[t.csi].loopback=0 ; t.charseq[t.csi].a_f=5101 ; t.charseq[t.csi].b_f=5160 ; t.charseq[t.csi].c_f=3 ; t.charseq[t.csi].freeze=1 ; ++t.csi;
	t.charseq[t.csi].mode=92 ; t.charseq[t.csi].a_f=g.csi_limbo ; ++t.csi;

	g.csi_inchair = t.csi;
	g.csi_inchairANIM = t.csi;
	t.charseq[t.csi].mode=1 ; t.charseq[t.csi].trigger=5 ; t.charseq[t.csi].a_f=3744 ; t.charseq[t.csi].b_f=3828 ; t.charseq[t.csi].c_f=10 ; t.charseq[t.csi].freeze=1 ; ++t.csi;
	t.charseq[t.csi].mode=92 ; t.charseq[t.csi].a_f=g.csi_inchair ; ++t.csi;
	g.csi_inchairsit = t.csi;
	g.csi_inchairsitANIM = t.csi;
	t.charseq[t.csi].mode=1 ; t.charseq[t.csi].trigger=5 ; t.charseq[t.csi].a_f=3710 ; t.charseq[t.csi].b_f=3744 ; t.charseq[t.csi].c_f=10 ; t.charseq[t.csi].freeze=1 ; ++t.csi;
	t.charseq[t.csi].mode=92 ; t.charseq[t.csi].a_f=g.csi_inchair ; ++t.csi;
	g.csi_inchairgetup = t.csi;
	g.csi_inchairgetupANIM = t.csi;
	t.charseq[t.csi].mode=1 ; t.charseq[t.csi].trigger=5 ; t.charseq[t.csi].a_f=3828 ; t.charseq[t.csi].b_f=3862 ; t.charseq[t.csi].c_f=10 ; t.charseq[t.csi].freeze=1 ; ++t.csi;
	t.charseq[t.csi].mode=92 ; t.charseq[t.csi].a_f=g.csi_unarmed ; ++t.csi;

	g.csi_swim = t.csi;
	g.csi_swimANIM = t.csi;
	t.charseq[t.csi].mode=1 ; t.charseq[t.csi].trigger=6 ; t.charseq[t.csi].a_f=3930 ; t.charseq[t.csi].b_f=4015 ; t.charseq[t.csi].c_f=10 ; ++t.csi;
	t.charseq[t.csi].mode=92 ; t.charseq[t.csi].a_f=g.csi_swim ; ++t.csi;
	g.csi_swimmovefore = t.csi;
	g.csi_swimmoveforeANIM = t.csi;
	t.charseq[t.csi].mode=1 ; t.charseq[t.csi].trigger=6 ; t.charseq[t.csi].a_f=4030 ; t.charseq[t.csi].b_f=4072 ; t.charseq[t.csi].c_f=10 ; ++t.csi;
	t.charseq[t.csi].mode=92 ; t.charseq[t.csi].a_f=g.csi_swim ; ++t.csi;

	//  Create CSI instructions for all five weapon styles
	t.weapstylemax=5;
	for ( t.weapstyle = 1 ; t.weapstyle<=  t.weapstylemax; t.weapstyle++ )
	{
	Dim (  t.csi_stood,t.weapstylemax   ); t.csi_stood[t.weapstyle]=t.csi;
	t.charseq[t.csi].mode=61 ; t.charseq[t.csi].trigger=1 ; ++t.csi;
	Dim (  t.csi_stoodnormal,t.weapstylemax   ); t.csi_stoodnormal[t.weapstyle]=t.csi;
	Dim (  t.csi_stoodnormalANIM,t.weapstylemax   ); t.csi_stoodnormalANIM[t.weapstyle]=t.csi;
	t.charseq[t.csi].mode=1 ; t.charseq[t.csi].trigger=1 ; t.charseq[t.csi].a_f=100 ; t.charseq[t.csi].b_f=205 ; t.charseq[t.csi].c_f=3 ; ++t.csi;
	t.charseq[t.csi].mode=92 ; t.charseq[t.csi].a_f=t.csi_stood[t.weapstyle] ; ++t.csi;
	Dim (  t.csi_stoodrocket,t.weapstylemax   ); t.csi_stoodrocket[t.weapstyle]=t.csi;
	Dim (  t.csi_stoodrocketANIM,t.weapstylemax   ); t.csi_stoodrocketANIM[t.weapstyle]=t.csi;
	t.charseq[t.csi].mode=1 ; t.charseq[t.csi].trigger=1 ; t.charseq[t.csi].a_f=6133 ; t.charseq[t.csi].b_f=6206 ; t.charseq[t.csi].c_f=3 ; ++t.csi;
	t.charseq[t.csi].mode=92 ; t.charseq[t.csi].a_f=t.csi_stood[t.weapstyle] ; ++t.csi;
	Dim (  t.csi_stoodfidget,t.weapstylemax   ); t.csi_stoodfidget[t.weapstyle]=t.csi;
	t.charseq[t.csi].mode=91 ; t.charseq[t.csi].a_f=4 ; ++t.csi;
	Dim (  t.csi_stoodfidgetANIM0,t.weapstylemax   ); t.csi_stoodfidgetANIM0[t.weapstyle]=t.csi;
	t.charseq[t.csi].mode=1 ; t.charseq[t.csi].trigger=1 ; t.charseq[t.csi].a_f=100 ; t.charseq[t.csi].b_f=205 ; t.charseq[t.csi].c_f=3 ; ++t.csi;
	t.charseq[t.csi].mode=92 ; t.charseq[t.csi].a_f=t.csi_stood[t.weapstyle] ; ++t.csi;
	Dim (  t.csi_stoodfidgetANIM1,t.weapstylemax   ); t.csi_stoodfidgetANIM1[t.weapstyle]=t.csi;
	t.charseq[t.csi].mode=1 ; t.charseq[t.csi].trigger=1 ; t.charseq[t.csi].a_f=210 ; t.charseq[t.csi].b_f=318 ; t.charseq[t.csi].c_f=3 ; ++t.csi;
	t.charseq[t.csi].mode=92 ; t.charseq[t.csi].a_f=t.csi_stood[t.weapstyle] ; ++t.csi;
	Dim (  t.csi_stoodfidgetANIM2,t.weapstylemax   ); t.csi_stoodfidgetANIM2[t.weapstyle]=t.csi;
	t.charseq[t.csi].mode=1 ; t.charseq[t.csi].trigger=1 ; t.charseq[t.csi].a_f=325 ; t.charseq[t.csi].b_f=431 ; t.charseq[t.csi].c_f=3 ; ++t.csi;
	t.charseq[t.csi].mode=92 ; t.charseq[t.csi].a_f=t.csi_stood[t.weapstyle] ; ++t.csi;
	Dim (  t.csi_stoodfidgetANIM3,t.weapstylemax   ); t.csi_stoodfidgetANIM3[t.weapstyle]=t.csi;
	t.charseq[t.csi].mode=1 ; t.charseq[t.csi].trigger=1 ; t.charseq[t.csi].a_f=440 ; t.charseq[t.csi].b_f=511 ; t.charseq[t.csi].c_f=3 ; ++t.csi;
	t.charseq[t.csi].mode=92 ; t.charseq[t.csi].a_f=t.csi_stood[t.weapstyle] ; ++t.csi;
	Dim (  t.csi_stoodstartled,t.weapstylemax   ); t.csi_stoodstartled[t.weapstyle]=t.csi;
	t.charseq[t.csi].mode=91 ; t.charseq[t.csi].a_f=2 ; ++t.csi;
	t.charseq[t.csi].mode=92 ; t.charseq[t.csi].a_f=t.csi_stood[t.weapstyle] ; ++t.csi;
	t.charseq[t.csi].mode=92 ; t.charseq[t.csi].a_f=t.csi_stood[t.weapstyle] ; ++t.csi;
	Dim (  t.csi_stoodstartledANIM,t.weapstylemax   ); t.csi_stoodstartledANIM[t.weapstyle]=t.csi;
	t.charseq[t.csi].mode=1 ; t.charseq[t.csi].trigger=1 ; t.charseq[t.csi].a_f=1425 ; t.charseq[t.csi].b_f=1465 ; t.charseq[t.csi].c_f=3 ; ++t.csi;
	t.charseq[t.csi].mode=92 ; t.charseq[t.csi].a_f=t.csi_stood[t.weapstyle] ; ++t.csi;
	Dim (  t.csi_stoodpunch,t.weapstylemax   ); t.csi_stoodpunch[t.weapstyle]=t.csi;
	Dim (  t.csi_stoodpunchANIM,t.weapstylemax   ); t.csi_stoodpunchANIM[t.weapstyle]=t.csi;
	t.charseq[t.csi].mode=4 ; t.charseq[t.csi].loopback=100 ; t.charseq[t.csi].a_f=2340 ; t.charseq[t.csi].b_f=2382 ; t.charseq[t.csi].c_f=1 ; t.charseq[t.csi].freeze=1 ; ++t.csi;
	t.charseq[t.csi].mode=54 ; ++t.csi;
	t.charseq[t.csi].mode=92 ; t.charseq[t.csi].a_f=t.csi_stood[t.weapstyle] ; ++t.csi;
	Dim (  t.csi_stoodkick,t.weapstylemax   ); t.csi_stoodkick[t.weapstyle]=t.csi;
	Dim (  t.csi_stoodkickANIM,t.weapstylemax   ); t.csi_stoodkickANIM[t.weapstyle]=t.csi;
	t.charseq[t.csi].mode=4 ; t.charseq[t.csi].loopback=100 ; t.charseq[t.csi].a_f=5511 ; t.charseq[t.csi].b_f=5553 ; t.charseq[t.csi].c_f=4 ; t.charseq[t.csi].freeze=1 ; ++t.csi;
	t.charseq[t.csi].mode=54 ; ++t.csi;
	t.charseq[t.csi].mode=92 ; t.charseq[t.csi].a_f=t.csi_stood[t.weapstyle] ; ++t.csi;
	Dim (  t.csi_stoodmovefore,t.weapstylemax   ); t.csi_stoodmovefore[t.weapstyle]=t.csi;
	Dim (  t.csi_stoodmoveforeANIM,t.weapstylemax   ); t.csi_stoodmoveforeANIM[t.weapstyle]=t.csi;
	t.charseq[t.csi].mode=5 ; t.charseq[t.csi].trigger=1 ; t.charseq[t.csi].a_f=685 ; t.charseq[t.csi].b_f=707 ; t.charseq[t.csi].c_f=2 ; t.charseq[t.csi].angle_f=0.0 ; t.charseq[t.csi].speed_f=1.5 ; ++t.csi;
	t.charseq[t.csi].mode=92 ; t.charseq[t.csi].a_f=t.csi_stoodmovefore[t.weapstyle] ; ++t.csi;
	Dim (  t.csi_stoodmoveback,t.weapstylemax   ); t.csi_stoodmoveback[t.weapstyle]=t.csi;
	Dim (  t.csi_stoodmovebackANIM,t.weapstylemax   ); t.csi_stoodmovebackANIM[t.weapstyle]=t.csi;
	t.charseq[t.csi].mode=5 ; t.charseq[t.csi].trigger=1 ; t.charseq[t.csi].a_f=710 ; t.charseq[t.csi].b_f=735 ; t.charseq[t.csi].c_f=2 ; t.charseq[t.csi].angle_f=0.0 ; t.charseq[t.csi].speed_f=1.0 ; ++t.csi;
	Dim (  t.csi_stoodmoveleft,t.weapstylemax   ); t.csi_stoodmoveleft[t.weapstyle]=t.csi;
	Dim (  t.csi_stoodmoveleftANIM,t.weapstylemax   ); t.csi_stoodmoveleftANIM[t.weapstyle]=t.csi;
	t.charseq[t.csi].mode=5 ; t.charseq[t.csi].trigger=1 ; t.charseq[t.csi].a_f=740 ; t.charseq[t.csi].b_f=762 ; t.charseq[t.csi].c_f=2 ; t.charseq[t.csi].angle_f=0.0 ; t.charseq[t.csi].speed_f=1.0 ; ++t.csi;
	Dim (  t.csi_stoodmoveright,t.weapstylemax   ); t.csi_stoodmoveright[t.weapstyle]=t.csi;
	Dim (  t.csi_stoodmoverightANIM,t.weapstylemax   ); t.csi_stoodmoverightANIM[t.weapstyle]=t.csi;
	t.charseq[t.csi].mode=5 ; t.charseq[t.csi].trigger=1 ; t.charseq[t.csi].a_f=765 ; t.charseq[t.csi].b_f=789 ; t.charseq[t.csi].c_f=2 ; t.charseq[t.csi].angle_f=0.0 ; t.charseq[t.csi].speed_f=1.0 ; ++t.csi;
	Dim (  t.csi_stoodstepleft,t.weapstylemax   ); t.csi_stoodstepleft[t.weapstyle]=t.csi;
	Dim (  t.csi_stoodstepleftANIM,t.weapstylemax   ); t.csi_stoodstepleftANIM[t.weapstyle]=t.csi;
	t.charseq[t.csi].mode=5 ; t.charseq[t.csi].trigger=1 ; t.charseq[t.csi].a_f=610 ; t.charseq[t.csi].b_f=640 ; t.charseq[t.csi].c_f=2 ; t.charseq[t.csi].angle_f=0.0 ; t.charseq[t.csi].speed_f=0.5 ; ++t.csi;
	Dim (  t.csi_stoodstepright,t.weapstylemax   ); t.csi_stoodstepright[t.weapstyle]=t.csi;
	Dim (  t.csi_stoodsteprightANIM,t.weapstylemax   ); t.csi_stoodsteprightANIM[t.weapstyle]=t.csi;
	t.charseq[t.csi].mode=5 ; t.charseq[t.csi].trigger=1 ; t.charseq[t.csi].a_f=645 ; t.charseq[t.csi].b_f=676 ; t.charseq[t.csi].c_f=2 ; t.charseq[t.csi].angle_f=0.0 ; t.charseq[t.csi].speed_f=0.5 ; ++t.csi;
	Dim (  t.csi_stoodstrafeleft,t.weapstylemax   ); t.csi_stoodstrafeleft[t.weapstyle]=t.csi;
	Dim (  t.csi_stoodstrafeleftANIM,t.weapstylemax   ); t.csi_stoodstrafeleftANIM[t.weapstyle]=t.csi;
	t.charseq[t.csi].mode=5 ; t.charseq[t.csi].trigger=1 ; t.charseq[t.csi].a_f=855 ; t.charseq[t.csi].b_f=871 ; t.charseq[t.csi].c_f=2 ; t.charseq[t.csi].angle_f=-90.0 ; t.charseq[t.csi].speed_f=1.0 ; ++t.csi;
	Dim (  t.csi_stoodstraferight,t.weapstylemax   ); t.csi_stoodstraferight[t.weapstyle]=t.csi;
	Dim (  t.csi_stoodstraferightANIM,t.weapstylemax   ); t.csi_stoodstraferightANIM[t.weapstyle]=t.csi;
	t.charseq[t.csi].mode=5 ; t.charseq[t.csi].trigger=1 ; t.charseq[t.csi].a_f=875 ; t.charseq[t.csi].b_f=892 ; t.charseq[t.csi].c_f=2 ; t.charseq[t.csi].angle_f=90.0 ; t.charseq[t.csi].speed_f=1.0 ; ++t.csi;
	Dim (  t.csi_stoodvault,t.weapstylemax   ); t.csi_stoodvault[t.weapstyle]=t.csi;
	Dim (  t.csi_stoodvaultANIM,t.weapstylemax   ); t.csi_stoodvaultANIM[t.weapstyle]=t.csi;
	t.charseq[t.csi].mode=4 ; t.charseq[t.csi].trigger=99 ; t.charseq[t.csi].a_f=0 ; t.charseq[t.csi].b_f=0 ; t.charseq[t.csi].c_f=1.0 ; t.charseq[t.csi].angle_f=0.0 ; t.charseq[t.csi].speed_f=2.0 ; ++t.csi; // 220217 - these now need to come from FPE
	t.charseq[t.csi].mode=3 ; t.charseq[t.csi].speed_f=3.0 ; t.charseq[t.csi].angle_f=0 ; ++t.csi;
	t.charseq[t.csi].mode=92 ; t.charseq[t.csi].a_f=t.csi_stood[t.weapstyle] ; ++t.csi;
	Dim (  t.csi_stoodmoverun,t.weapstylemax   ); t.csi_stoodmoverun[t.weapstyle]=t.csi;
	Dim (  t.csi_stoodmoverunANIM,t.weapstylemax   ); t.csi_stoodmoverunANIM[t.weapstyle]=t.csi;
	t.charseq[t.csi].mode=5 ; t.charseq[t.csi].trigger=1 ; t.charseq[t.csi].a_f=795 ; t.charseq[t.csi].b_f=811 ; t.charseq[t.csi].c_f=2 ; t.charseq[t.csi].angle_f=0.0 ; t.charseq[t.csi].speed_f=3.0 ; ++t.csi;
	t.charseq[t.csi].mode=92 ; t.charseq[t.csi].a_f=t.csi_stood[t.weapstyle] ; ++t.csi;
	Dim (  t.csi_stoodmoverunleft,t.weapstylemax   ); t.csi_stoodmoverunleft[t.weapstyle]=t.csi;
	Dim (  t.csi_stoodmoverunleftANIM,t.weapstylemax   ); t.csi_stoodmoverunleftANIM[t.weapstyle]=t.csi;
	t.charseq[t.csi].mode=5 ; t.charseq[t.csi].trigger=1 ; t.charseq[t.csi].a_f=815 ; t.charseq[t.csi].b_f=830 ; t.charseq[t.csi].c_f=2 ; t.charseq[t.csi].angle_f=0.0 ; t.charseq[t.csi].speed_f=2.0 ; ++t.csi;
	t.charseq[t.csi].mode=92 ; t.charseq[t.csi].a_f=t.csi_stood[t.weapstyle] ; ++t.csi;
	Dim (  t.csi_stoodmoverunright,t.weapstylemax   ); t.csi_stoodmoverunright[t.weapstyle]=t.csi;
	Dim (  t.csi_stoodmoverunrightANIM,t.weapstylemax   ); t.csi_stoodmoverunrightANIM[t.weapstyle]=t.csi;
	t.charseq[t.csi].mode=5 ; t.charseq[t.csi].trigger=1 ; t.charseq[t.csi].a_f=835 ; t.charseq[t.csi].b_f=850 ; t.charseq[t.csi].c_f=2 ; t.charseq[t.csi].angle_f=0.0 ; t.charseq[t.csi].speed_f=2.0 ; ++t.csi;
	t.charseq[t.csi].mode=92 ; t.charseq[t.csi].a_f=t.csi_stood[t.weapstyle] ; ++t.csi;
	Dim (  t.csi_stoodreload,t.weapstylemax   ); t.csi_stoodreload[t.weapstyle]=t.csi;
	Dim (  t.csi_stoodreloadANIM,t.weapstylemax   ); t.csi_stoodreloadANIM[t.weapstyle]=t.csi;
	t.charseq[t.csi].mode=1 ; t.charseq[t.csi].loopback=0 ; t.charseq[t.csi].a_f=515 ; t.charseq[t.csi].b_f=605 ; t.charseq[t.csi].c_f=3 ; t.charseq[t.csi].freeze=1 ; ++t.csi;
	t.charseq[t.csi].mode=51 ; t.charseq[t.csi].trigger=1 ; ++t.csi;
	t.charseq[t.csi].mode=92 ; t.charseq[t.csi].a_f=t.csi_stood[t.weapstyle] ; ++t.csi;
	Dim (  t.csi_stoodreloadrocket,t.weapstylemax   ); t.csi_stoodreloadrocket[t.weapstyle]=t.csi;
	Dim (  t.csi_stoodreloadrocketANIM,t.weapstylemax   ); t.csi_stoodreloadrocketANIM[t.weapstyle]=t.csi;
	t.charseq[t.csi].mode=1 ; t.charseq[t.csi].loopback=0 ; t.charseq[t.csi].a_f=6233 ; t.charseq[t.csi].b_f=6315 ; t.charseq[t.csi].c_f=3 ; t.charseq[t.csi].freeze=1 ; ++t.csi;
	t.charseq[t.csi].mode=51 ; t.charseq[t.csi].trigger=1 ; ++t.csi;
	t.charseq[t.csi].mode=92 ; t.charseq[t.csi].a_f=t.csi_stood[t.weapstyle] ; ++t.csi;
	Dim (  t.csi_stoodwave,t.weapstylemax   ); t.csi_stoodwave[t.weapstyle]=t.csi;
	Dim (  t.csi_stoodwaveANIM,t.weapstylemax   ); t.csi_stoodwaveANIM[t.weapstyle]=t.csi;
	t.charseq[t.csi].mode=1 ; t.charseq[t.csi].loopback=0 ; t.charseq[t.csi].a_f=1470 ; t.charseq[t.csi].b_f=1520 ; t.charseq[t.csi].c_f=3 ; t.charseq[t.csi].freeze=1 ; ++t.csi;
	t.charseq[t.csi].mode=92 ; t.charseq[t.csi].a_f=t.csi_stood[t.weapstyle] ; ++t.csi;
	Dim (  t.csi_stoodtoss,t.weapstylemax   ); t.csi_stoodtoss[t.weapstyle]=t.csi;
	Dim (  t.csi_stoodtossANIM,t.weapstylemax   ); t.csi_stoodtossANIM[t.weapstyle]=t.csi;
	t.charseq[t.csi].mode=1 ; t.charseq[t.csi].loopback=100 ; t.charseq[t.csi].a_f=2390 ; t.charseq[t.csi].b_f=2444 ; t.charseq[t.csi].c_f=1 ; t.charseq[t.csi].freeze=1 ; ++t.csi;
	t.charseq[t.csi].mode=92 ; t.charseq[t.csi].a_f=t.csi_stood[t.weapstyle] ; ++t.csi;
	Dim (  t.csi_stoodfirerocket,t.weapstylemax   ); t.csi_stoodfirerocket[t.weapstyle]=t.csi;
	Dim (  t.csi_stoodfirerocketANIM,t.weapstylemax   ); t.csi_stoodfirerocketANIM[t.weapstyle]=t.csi;
	t.charseq[t.csi].mode=1 ; t.charseq[t.csi].loopback=0 ; t.charseq[t.csi].a_f=6207 ; t.charseq[t.csi].b_f=6232 ; t.charseq[t.csi].c_f=1 ; t.charseq[t.csi].freeze=1 ; ++t.csi;
	t.charseq[t.csi].mode=92 ; t.charseq[t.csi].a_f=t.csi_stood[t.weapstyle] ; ++t.csi;
	Dim (  t.csi_stoodincoverleft,t.weapstylemax   ); t.csi_stoodincoverleft[t.weapstyle]=t.csi;
	Dim (  t.csi_stoodincoverleftANIM,t.weapstylemax   ); t.csi_stoodincoverleftANIM[t.weapstyle]=t.csi;
	t.charseq[t.csi].mode=2 ; t.charseq[t.csi].trigger=7 ; t.charseq[t.csi].loopback=1580 ; t.charseq[t.csi].a_f=1580 ; t.charseq[t.csi].b_f=1580 ; t.charseq[t.csi].c_f=1 ; t.charseq[t.csi].freeze=1 ; ++t.csi;
	t.charseq[t.csi].mode=92 ; t.charseq[t.csi].a_f=t.csi_stoodincoverleft[t.weapstyle] ; ++t.csi;
	Dim (  t.csi_stoodincoverpeekleft,t.weapstylemax   ); t.csi_stoodincoverpeekleft[t.weapstyle]=t.csi;
	Dim (  t.csi_stoodincoverpeekleftANIM,t.weapstylemax   ); t.csi_stoodincoverpeekleftANIM[t.weapstyle]=t.csi;
	t.charseq[t.csi].mode=1 ; t.charseq[t.csi].trigger=7 ; t.charseq[t.csi].loopback=1581 ; t.charseq[t.csi].a_f=1581 ; t.charseq[t.csi].b_f=1623 ; t.charseq[t.csi].c_f=1 ; t.charseq[t.csi].freeze=1 ; ++t.csi;
	t.charseq[t.csi].mode=92 ; t.charseq[t.csi].a_f=t.csi_stoodincoverleft[t.weapstyle] ; ++t.csi;
	Dim (  t.csi_stoodincoverthrowleft,t.weapstylemax   ); t.csi_stoodincoverthrowleft[t.weapstyle]=t.csi;
	Dim (  t.csi_stoodincoverthrowleftANIM,t.weapstylemax   ); t.csi_stoodincoverthrowleftANIM[t.weapstyle]=t.csi;
	t.charseq[t.csi].mode=1 ; t.charseq[t.csi].trigger=7 ; t.charseq[t.csi].loopback=2680 ; t.charseq[t.csi].a_f=2680 ; t.charseq[t.csi].b_f=2778 ; t.charseq[t.csi].c_f=1 ; t.charseq[t.csi].freeze=1 ; ++t.csi;
	t.charseq[t.csi].mode=92 ; t.charseq[t.csi].a_f=t.csi_stoodincoverleft[t.weapstyle] ; ++t.csi;
	Dim (  t.csi_stoodincoverright,t.weapstylemax   ); t.csi_stoodincoverright[t.weapstyle]=t.csi;
	Dim (  t.csi_stoodincoverrightANIM,t.weapstylemax   ); t.csi_stoodincoverrightANIM[t.weapstyle]=t.csi;
	t.charseq[t.csi].mode=2 ; t.charseq[t.csi].trigger=7 ; t.charseq[t.csi].loopback=1525 ; t.charseq[t.csi].a_f=1525 ; t.charseq[t.csi].b_f=1525 ; t.charseq[t.csi].c_f=1 ; t.charseq[t.csi].freeze=1 ; ++t.csi;
	t.charseq[t.csi].mode=92 ; t.charseq[t.csi].a_f=t.csi_stoodincoverright[t.weapstyle] ; ++t.csi;
	Dim (  t.csi_stoodincoverpeekright,t.weapstylemax   ); t.csi_stoodincoverpeekright[t.weapstyle]=t.csi;
	Dim (  t.csi_stoodincoverpeekrightANIM,t.weapstylemax   ); t.csi_stoodincoverpeekrightANIM[t.weapstyle]=t.csi;
	t.charseq[t.csi].mode=1 ; t.charseq[t.csi].trigger=7 ; t.charseq[t.csi].loopback=1526 ; t.charseq[t.csi].a_f=1526 ; t.charseq[t.csi].b_f=1573 ; t.charseq[t.csi].c_f=1 ; t.charseq[t.csi].freeze=1 ; ++t.csi;
	t.charseq[t.csi].mode=92 ; t.charseq[t.csi].a_f=t.csi_stoodincoverright[t.weapstyle] ; ++t.csi;
	Dim (  t.csi_stoodincoverthrowright,t.weapstylemax   ); t.csi_stoodincoverthrowright[t.weapstyle]=t.csi;
	Dim (  t.csi_stoodincoverthrowrightANIM,t.weapstylemax   ); t.csi_stoodincoverthrowrightANIM[t.weapstyle]=t.csi;
	t.charseq[t.csi].mode=1 ; t.charseq[t.csi].trigger=7 ; t.charseq[t.csi].loopback=2570 ; t.charseq[t.csi].a_f=2570 ; t.charseq[t.csi].b_f=2668 ; t.charseq[t.csi].c_f=1 ; t.charseq[t.csi].freeze=1 ; ++t.csi;
	t.charseq[t.csi].mode=92 ; t.charseq[t.csi].a_f=t.csi_stoodincoverright[t.weapstyle] ; ++t.csi;
	Dim (  t.csi_stoodandturn,t.weapstylemax   ); t.csi_stoodandturn[t.weapstyle]=t.csi;
	Dim (  t.csi_stoodandturnANIM,t.weapstylemax   ); t.csi_stoodandturnANIM[t.weapstyle]=t.csi;
	t.charseq[t.csi].mode=1 ; t.charseq[t.csi].trigger=7 ; t.charseq[t.csi].loopback=6051 ; t.charseq[t.csi].a_f=6070 ; t.charseq[t.csi].b_f=2668 ; t.charseq[t.csi].c_f=1 ; t.charseq[t.csi].freeze=1 ; ++t.csi;
	t.charseq[t.csi].mode=92 ; t.charseq[t.csi].a_f=t.csi_stood[t.weapstyle] ; ++t.csi;
	Dim (  t.csi_crouchidle,t.weapstylemax   ); t.csi_crouchidle[t.weapstyle]=t.csi;
	t.charseq[t.csi].mode=61 ; t.charseq[t.csi].trigger=2 ; ++t.csi;
	Dim (  t.csi_crouchidlenormal,t.weapstylemax   ); t.csi_crouchidlenormal[t.weapstyle]=t.csi;
	t.charseq[t.csi].mode=91 ; t.charseq[t.csi].a_f=2 ; ++t.csi;
	Dim (  t.csi_crouchidlenormalANIM0,t.weapstylemax   ); t.csi_crouchidlenormalANIM0[t.weapstyle]=t.csi;
	t.charseq[t.csi].mode=1 ; t.charseq[t.csi].trigger=2 ; t.charseq[t.csi].a_f=1670 ; t.charseq[t.csi].b_f=1819 ; t.charseq[t.csi].c_f=5.0 ; ++t.csi;
	t.charseq[t.csi].mode=92 ; t.charseq[t.csi].a_f=t.csi_crouchidle[t.weapstyle] ; ++t.csi;
	Dim (  t.csi_crouchidlenormalANIM1,t.weapstylemax   ); t.csi_crouchidlenormalANIM1[t.weapstyle]=t.csi;
	t.charseq[t.csi].mode=1 ; t.charseq[t.csi].trigger=2 ; t.charseq[t.csi].a_f=1825 ; t.charseq[t.csi].b_f=1914 ; t.charseq[t.csi].c_f=5.0 ; ++t.csi;
	t.charseq[t.csi].mode=92 ; t.charseq[t.csi].a_f=t.csi_crouchidle[t.weapstyle] ; ++t.csi;
	Dim (  t.csi_crouchidlerocket,t.weapstylemax   ); t.csi_crouchidlerocket[t.weapstyle]=t.csi;
	Dim (  t.csi_crouchidlerocketANIM,t.weapstylemax   ); t.csi_crouchidlerocketANIM[t.weapstyle]=t.csi;
	t.charseq[t.csi].mode=1 ; t.charseq[t.csi].trigger=2 ; t.charseq[t.csi].loopback=0 ; t.charseq[t.csi].a_f=6472 ; t.charseq[t.csi].b_f=6545 ; t.charseq[t.csi].c_f=1 ; t.charseq[t.csi].freeze=1 ; ++t.csi;
	t.charseq[t.csi].mode=92 ; t.charseq[t.csi].a_f=t.csi_crouchidle[t.weapstyle] ; ++t.csi;
	Dim (  t.csi_crouchdown,t.weapstylemax   ); t.csi_crouchdown[t.weapstyle]=t.csi;
	Dim (  t.csi_crouchdownANIM,t.weapstylemax   ); t.csi_crouchdownANIM[t.weapstyle]=t.csi;
	t.charseq[t.csi].mode=1 ; t.charseq[t.csi].loopback=0 ; t.charseq[t.csi].a_f=1630 ; t.charseq[t.csi].b_f=1646 ; t.charseq[t.csi].c_f=1 ; t.charseq[t.csi].freeze=1 ; ++t.csi;
	t.charseq[t.csi].mode=92 ; t.charseq[t.csi].a_f=t.csi_crouchidle[t.weapstyle] ; ++t.csi;
	Dim (  t.csi_crouchdownrocket,t.weapstylemax   ); t.csi_crouchdownrocket[t.weapstyle]=t.csi;
	Dim (  t.csi_crouchdownrocketANIM,t.weapstylemax   ); t.csi_crouchdownrocketANIM[t.weapstyle]=t.csi;
	t.charseq[t.csi].mode=1 ; t.charseq[t.csi].loopback=0 ; t.charseq[t.csi].a_f=6316 ; t.charseq[t.csi].b_f=6356 ; t.charseq[t.csi].c_f=1 ; t.charseq[t.csi].freeze=1 ; ++t.csi;
	t.charseq[t.csi].mode=92 ; t.charseq[t.csi].a_f=t.csi_crouchidle[t.weapstyle] ; ++t.csi;
	Dim (  t.csi_crouchrolldown,t.weapstylemax   ); t.csi_crouchrolldown[t.weapstyle]=t.csi;
	t.charseq[t.csi].mode=3 ; t.charseq[t.csi].speed_f=-7.0 ; t.charseq[t.csi].angle_f=0 ; ++t.csi;
	Dim (  t.csi_crouchrolldownANIM,t.weapstylemax   ); t.csi_crouchrolldownANIM[t.weapstyle]=t.csi;
	t.charseq[t.csi].mode=4 ; t.charseq[t.csi].loopback=1670 ; t.charseq[t.csi].a_f=2160 ; t.charseq[t.csi].b_f=2216 ; t.charseq[t.csi].c_f=1 ; t.charseq[t.csi].angle_f=0.0 ; t.charseq[t.csi].speed_f=0.25 ; t.charseq[t.csi].speedinmiddle_f=1.25 ; ++t.csi;
	t.charseq[t.csi].mode=3 ; t.charseq[t.csi].speed_f=4.0 ; t.charseq[t.csi].angle_f=0 ; ++t.csi;
	t.charseq[t.csi].mode=92 ; t.charseq[t.csi].a_f=t.csi_crouchidle[t.weapstyle] ; ++t.csi;
	Dim (  t.csi_crouchrollup,t.weapstylemax   ); t.csi_crouchrollup[t.weapstyle]=t.csi;
	t.charseq[t.csi].mode=3 ; t.charseq[t.csi].speed_f=-7.0 ; t.charseq[t.csi].angle_f=0 ; ++t.csi;
	Dim (  t.csi_crouchrollupANIM,t.weapstylemax   ); t.csi_crouchrollupANIM[t.weapstyle]=t.csi;
	t.charseq[t.csi].mode=4 ; t.charseq[t.csi].loopback=100 ; t.charseq[t.csi].a_f=2225 ; t.charseq[t.csi].b_f=2281 ; t.charseq[t.csi].c_f=1 ; t.charseq[t.csi].angle_f=0.0 ; t.charseq[t.csi].speed_f=0.25 ; t.charseq[t.csi].speedinmiddle_f=1.25 ; ++t.csi;
	t.charseq[t.csi].mode=3 ; t.charseq[t.csi].speed_f=3.0 ; t.charseq[t.csi].angle_f=0 ; ++t.csi;
	t.charseq[t.csi].mode=92 ; t.charseq[t.csi].a_f=t.csi_stood[t.weapstyle] ; ++t.csi;
	Dim (  t.csi_crouchmovefore,t.weapstylemax   ); t.csi_crouchmovefore[t.weapstyle]=t.csi;
	t.charseq[t.csi].mode=52 ; t.charseq[t.csi].trigger=1 ; ++t.csi;
	Dim (  t.csi_crouchmoveforeANIM,t.weapstylemax   ); t.csi_crouchmoveforeANIM[t.weapstyle]=t.csi;
	t.charseq[t.csi].mode=1 ; t.charseq[t.csi].loopback=2075 ; t.charseq[t.csi].a_f=2075 ; t.charseq[t.csi].b_f=2102 ; t.charseq[t.csi].c_f=1 ; ++t.csi;
	t.charseq[t.csi].mode=3 ; t.charseq[t.csi].speed_f=29.5 ; t.charseq[t.csi].angle_f=0 ; ++t.csi;
	t.charseq[t.csi].mode=52 ; t.charseq[t.csi].trigger=0 ; ++t.csi;
	t.charseq[t.csi].mode=92 ; t.charseq[t.csi].a_f=t.csi_crouchidle[t.weapstyle] ; ++t.csi;
	Dim (  t.csi_crouchmoveback,t.weapstylemax   ); t.csi_crouchmoveback[t.weapstyle]=t.csi;
	t.charseq[t.csi].mode=52 ; t.charseq[t.csi].trigger=1 ; ++t.csi;
	t.charseq[t.csi].mode=3 ; t.charseq[t.csi].speed_f=-29.5 ; t.charseq[t.csi].angle_f=0 ; ++t.csi;
	Dim (  t.csi_crouchmovebackANIM,t.weapstylemax   ); t.csi_crouchmovebackANIM[t.weapstyle]=t.csi;
	t.charseq[t.csi].mode=1 ; t.charseq[t.csi].loopback=0 ; t.charseq[t.csi].a_f=2102 ; t.charseq[t.csi].b_f=2131 ; t.charseq[t.csi].c_f=1 ; ++t.csi;
	t.charseq[t.csi].mode=52 ; t.charseq[t.csi].trigger=0 ; ++t.csi;
	t.charseq[t.csi].mode=92 ; t.charseq[t.csi].a_f=t.csi_crouchidle[t.weapstyle] ; ++t.csi;
	Dim (  t.csi_crouchmoveleft,t.weapstylemax   ); t.csi_crouchmoveleft[t.weapstyle]=t.csi;
	Dim (  t.csi_crouchmoveleftANIM,t.weapstylemax   ); t.csi_crouchmoveleftANIM[t.weapstyle]=t.csi;
	t.charseq[t.csi].mode=4 ; t.charseq[t.csi].loopback=0 ; t.charseq[t.csi].a_f=2015 ; t.charseq[t.csi].b_f=2043 ; t.charseq[t.csi].c_f=1 ; t.charseq[t.csi].angle_f=0.0 ; t.charseq[t.csi].speed_f=0.5 ; ++t.csi;
	t.charseq[t.csi].mode=92 ; t.charseq[t.csi].a_f=t.csi_crouchidle[t.weapstyle] ; ++t.csi;
	Dim (  t.csi_crouchmoveright,t.weapstylemax   ); t.csi_crouchmoveright[t.weapstyle]=t.csi;
	Dim (  t.csi_crouchmoverightANIM,t.weapstylemax   ); t.csi_crouchmoverightANIM[t.weapstyle]=t.csi;
	t.charseq[t.csi].mode=4 ; t.charseq[t.csi].loopback=0 ; t.charseq[t.csi].a_f=2043 ; t.charseq[t.csi].b_f=2072 ; t.charseq[t.csi].c_f=1 ; t.charseq[t.csi].angle_f=0.0 ; t.charseq[t.csi].speed_f=0.5 ; ++t.csi;
	t.charseq[t.csi].mode=92 ; t.charseq[t.csi].a_f=t.csi_crouchidle[t.weapstyle] ; ++t.csi;
	Dim (  t.csi_crouchmoverun,t.weapstylemax   ); t.csi_crouchmoverun[t.weapstyle]=t.csi;
	Dim (  t.csi_crouchmoverunANIM,t.weapstylemax   ); t.csi_crouchmoverunANIM[t.weapstyle]=t.csi;
	t.charseq[t.csi].mode=5 ; t.charseq[t.csi].trigger=2 ; t.charseq[t.csi].a_f=2135 ; t.charseq[t.csi].b_f=2153 ; t.charseq[t.csi].c_f=10 ; t.charseq[t.csi].angle_f=0.0 ; t.charseq[t.csi].speed_f=1.5 ; ++t.csi;
	t.charseq[t.csi].mode=92 ; t.charseq[t.csi].a_f=t.csi_crouchidle[t.weapstyle] ; ++t.csi;
	Dim (  t.csi_crouchreload,t.weapstylemax   ); t.csi_crouchreload[t.weapstyle]=t.csi;
	Dim (  t.csi_crouchreloadANIM,t.weapstylemax   ); t.csi_crouchreloadANIM[t.weapstyle]=t.csi;
	t.charseq[t.csi].mode=1 ; t.charseq[t.csi].loopback=0 ; t.charseq[t.csi].a_f=1920 ; t.charseq[t.csi].b_f=2010 ; t.charseq[t.csi].c_f=10 ; t.charseq[t.csi].freeze=1 ; ++t.csi;
	t.charseq[t.csi].mode=51 ; t.charseq[t.csi].trigger=1 ; ++t.csi;
	t.charseq[t.csi].mode=92 ; t.charseq[t.csi].a_f=t.csi_crouchidle[t.weapstyle] ; ++t.csi;
	Dim (  t.csi_crouchreloadrocket,t.weapstylemax   ); t.csi_crouchreloadrocket[t.weapstyle]=t.csi;
	Dim (  t.csi_crouchreloadrocketANIM,t.weapstylemax   ); t.csi_crouchreloadrocketANIM[t.weapstyle]=t.csi;
	t.charseq[t.csi].mode=1 ; t.charseq[t.csi].loopback=0 ; t.charseq[t.csi].a_f=6380 ; t.charseq[t.csi].b_f=6471 ; t.charseq[t.csi].c_f=10 ; t.charseq[t.csi].freeze=1 ; ++t.csi;
	t.charseq[t.csi].mode=51 ; t.charseq[t.csi].trigger=1 ; ++t.csi;
	t.charseq[t.csi].mode=92 ; t.charseq[t.csi].a_f=t.csi_crouchidle[t.weapstyle] ; ++t.csi;
	Dim (  t.csi_crouchwave,t.weapstylemax   ); t.csi_crouchwave[t.weapstyle]=t.csi;
	Dim (  t.csi_crouchwaveANIM,t.weapstylemax   ); t.csi_crouchwaveANIM[t.weapstyle]=t.csi;
	t.charseq[t.csi].mode=1 ; t.charseq[t.csi].loopback=0 ; t.charseq[t.csi].a_f=2460 ; t.charseq[t.csi].b_f=2510 ; t.charseq[t.csi].c_f=10 ; t.charseq[t.csi].freeze=1 ; ++t.csi;
	t.charseq[t.csi].mode=92 ; t.charseq[t.csi].a_f=t.csi_crouchidle[t.weapstyle] ; ++t.csi;
	Dim (  t.csi_crouchtoss,t.weapstylemax   ); t.csi_crouchtoss[t.weapstyle]=t.csi;
	Dim (  t.csi_crouchtossANIM,t.weapstylemax   ); t.csi_crouchtossANIM[t.weapstyle]=t.csi;
	t.charseq[t.csi].mode=1 ; t.charseq[t.csi].loopback=0 ; t.charseq[t.csi].a_f=2520 ; t.charseq[t.csi].b_f=2555 ; t.charseq[t.csi].c_f=10 ; t.charseq[t.csi].freeze=1 ; ++t.csi;
	t.charseq[t.csi].mode=92 ; t.charseq[t.csi].a_f=t.csi_crouchidle[t.weapstyle] ; ++t.csi;
	Dim (  t.csi_crouchfirerocket,t.weapstylemax   ); t.csi_crouchfirerocket[t.weapstyle]=t.csi;
	Dim (  t.csi_crouchfirerocketANIM,t.weapstylemax   ); t.csi_crouchfirerocketANIM[t.weapstyle]=t.csi;
	t.charseq[t.csi].mode=1 ; t.charseq[t.csi].loopback=0 ; t.charseq[t.csi].a_f=6357 ; t.charseq[t.csi].b_f=6379 ; t.charseq[t.csi].c_f=1 ; t.charseq[t.csi].freeze=1 ; ++t.csi;
	t.charseq[t.csi].mode=92 ; t.charseq[t.csi].a_f=t.csi_crouchidle[t.weapstyle] ; ++t.csi;
	Dim (  t.csi_crouchimpactfore,t.weapstylemax   ); t.csi_crouchimpactfore[t.weapstyle]=t.csi;
	Dim (  t.csi_crouchimpactforeANIM,t.weapstylemax   ); t.csi_crouchimpactforeANIM[t.weapstyle]=t.csi;
	t.charseq[t.csi].mode=1 ; t.charseq[t.csi].loopback=0 ; t.charseq[t.csi].a_f=5240 ; t.charseq[t.csi].b_f=5277 ; t.charseq[t.csi].c_f=1 ; t.charseq[t.csi].freeze=1 ; ++t.csi;
	t.charseq[t.csi].mode=92 ; t.charseq[t.csi].a_f=g.csi_limbo ; ++t.csi;
	Dim (  t.csi_crouchimpactback,t.weapstylemax   ); t.csi_crouchimpactback[t.weapstyle]=t.csi;
	Dim (  t.csi_crouchimpactbackANIM,t.weapstylemax   ); t.csi_crouchimpactbackANIM[t.weapstyle]=t.csi;
	t.charseq[t.csi].mode=1 ; t.charseq[t.csi].loopback=0 ; t.charseq[t.csi].a_f=5290 ; t.charseq[t.csi].b_f=5339 ; t.charseq[t.csi].c_f=1 ; t.charseq[t.csi].freeze=1 ; ++t.csi;
	t.charseq[t.csi].mode=92 ; t.charseq[t.csi].a_f=g.csi_limbo ; ++t.csi;
	Dim (  t.csi_crouchimpactleft,t.weapstylemax   ); t.csi_crouchimpactleft[t.weapstyle]=t.csi;
	Dim (  t.csi_crouchimpactleftANIM,t.weapstylemax   ); t.csi_crouchimpactleftANIM[t.weapstyle]=t.csi;
	t.charseq[t.csi].mode=1 ; t.charseq[t.csi].loopback=0 ; t.charseq[t.csi].a_f=5409 ; t.charseq[t.csi].b_f=5466 ; t.charseq[t.csi].c_f=1 ; t.charseq[t.csi].freeze=1 ; ++t.csi;
	t.charseq[t.csi].mode=92 ; t.charseq[t.csi].a_f=g.csi_limbo ; ++t.csi;
	Dim (  t.csi_crouchimpactright,t.weapstylemax   ); t.csi_crouchimpactright[t.weapstyle]=t.csi;
	Dim (  t.csi_crouchimpactrightANIM,t.weapstylemax   ); t.csi_crouchimpactrightANIM[t.weapstyle]=t.csi;
	t.charseq[t.csi].mode=1 ; t.charseq[t.csi].loopback=0 ; t.charseq[t.csi].a_f=5350 ; t.charseq[t.csi].b_f=5395 ; t.charseq[t.csi].c_f=1 ; t.charseq[t.csi].freeze=1 ; ++t.csi;
	t.charseq[t.csi].mode=92 ; t.charseq[t.csi].a_f=g.csi_limbo ; ++t.csi;
	Dim (  t.csi_crouchgetup,t.weapstylemax   ); t.csi_crouchgetup[t.weapstyle]=t.csi;
	Dim (  t.csi_crouchgetupANIM,t.weapstylemax   ); t.csi_crouchgetupANIM[t.weapstyle]=t.csi;
	t.charseq[t.csi].mode=1 ; t.charseq[t.csi].loopback=100 ; t.charseq[t.csi].a_f=1646 ; t.charseq[t.csi].b_f=1663 ; t.charseq[t.csi].c_f=10 ; t.charseq[t.csi].freeze=1 ; ++t.csi;
	t.charseq[t.csi].mode=92 ; t.charseq[t.csi].a_f=t.csi_stood[t.weapstyle] ; ++t.csi;
	Dim (  t.csi_crouchgetuprocket,t.weapstylemax   ); t.csi_crouchgetuprocket[t.weapstyle]=t.csi;
	Dim (  t.csi_crouchgetuprocketANIM,t.weapstylemax   ); t.csi_crouchgetuprocketANIM[t.weapstyle]=t.csi;
	t.charseq[t.csi].mode=1 ; t.charseq[t.csi].loopback=100 ; t.charseq[t.csi].a_f=6573 ; t.charseq[t.csi].b_f=6607 ; t.charseq[t.csi].c_f=10 ; t.charseq[t.csi].freeze=1 ; ++t.csi;
	t.charseq[t.csi].mode=92 ; t.charseq[t.csi].a_f=t.csi_stood[t.weapstyle] ; ++t.csi;
	}
	g.csi_csimax = t.csi-1;
	if (  g.csi_csimax>t.tcharseqmax-10 ) 
	{
		ExitPrompt (  "getting close to g.csi_csimax max",""  ); 
		ExitProcess ( 0 );
	}
}

void char_getcharseqcsifromplaycsi ( void )
{
	//  returns charseqcsia# and charseqcsib#
	t.q=t.entityprofile[t.ttentid].startofaianim;
	t.charseqcsia_f=t.entityanim[t.ttentid][t.q+t.charanimstate.playcsi].start;
	t.charseqcsib_f=t.entityanim[t.ttentid][t.q+t.charanimstate.playcsi].finish;
	if (  (t.charseqcsia_f == 100 && t.charseqcsib_f == 101) || t.charseqcsia_f == -1 || t.charseqcsib_f == -1 ) 
	{
		t.charseqcsia_f=t.entityanim[t.ttentid][t.q+t.csi_stoodnormalANIM[t.charanimstate.weapstyle]].start;
		t.charseqcsib_f=t.entityanim[t.ttentid][t.q+t.csi_stoodnormalANIM[t.charanimstate.weapstyle]].start+1;
	}
	t.charseqcsiloopback=t.charseq[t.charanimstate.playcsi].loopback;
	if (  t.charseqcsiloopback>0 ) 
	{
		if (  t.charseqcsiloopback == 100  )  t.charseqcsiloopback = t.entityanim[t.ttentid][t.q+t.csi_stoodnormalANIM[t.charanimstate.weapstyle]].start;
		if (  t.charseqcsiloopback == t.charseqcsia_f  )  t.charseqcsiloopback = t.charseqcsia_f;
		if (  t.charseqcsiloopback == t.charseqcsib_f  )  t.charseqcsiloopback = t.charseqcsib_f;
		t.tCrouchCSIANIMIndex=t.entityanim[t.ttentid][t.q+t.csi_crouchidlenormalANIM0[t.charanimstate.weapstyle]].start;
		if (  t.charseqcsiloopback == t.tCrouchCSIANIMIndex  )  t.charseqcsiloopback = t.tCrouchCSIANIMIndex;
	}
}

void char_loop ( void )
{
	// all animation handled by scripts and wickeds own animation system

	// Update anim system for smoothing (or transitions if wicked)
	smoothanimupdate ( t.charanimstate.obj );
}

void darkai_ischaracterhit ( void )
{
	//  takes; px#,py#,pz#,tobj
	t.darkaifirerayhitcharacter=0;
	for ( g.charanimindex = 1 ; g.charanimindex<=  g.charanimindexmax; g.charanimindex++ )
	{
		if (  t.entityelement[t.charanimstates[g.charanimindex].e].health>0 ) 
		{
			if (  t.tobj == t.charanimstates[g.charanimindex].obj ) 
			{
				//  if melee attack on character, half force for better organic response
				if (  t.gun[t.gunid].settings.ismelee == 2  )  t.tforce_f = t.tforce_f/2.0;
				//  damage this character
				t.twhox_f=t.px_f ; t.twhoy_f=t.py_f ; t.twhoz_f=t.pz_f;
				darkai_shootcharacter ( );
				t.darkaifirerayhitcharacter=1;
			}
		}
	}
}

void smoothanimtriggerrev ( int obj, float st, float fn, int speedoftransition, int rev, int playflag )
{
	// transition to the start of the loop frame
	if ( t.smoothanim[obj].st != st ) 
	{
		StopObject (  obj );
		SetObjectInterpolation (  obj,100.0/speedoftransition );
		if (  rev == 1 ) 
		{
			SetObjectFrame (  obj,fn );
		}
		else
		{
			SetObjectFrame (  obj,st );
		}
		t.smoothanim[obj].st=st;
		t.smoothanim[obj].fn=fn;
		t.smoothanim[obj].rev=rev;
		t.smoothanim[obj].transition=speedoftransition;
		t.smoothanim[obj].playflag=playflag;
		t.smoothanim[obj].playstarted=0;
	}
}

void smoothanimtrigger ( int obj, float st, float fn, int speedoftransition )
{
	smoothanimtriggerrev(obj, st, fn, speedoftransition,0,0);
}

void smoothanimupdate ( int obj )
{
	if (  t.smoothanim[obj].transition>0 ) 
	{
		t.smoothanim[obj].transition=t.smoothanim[obj].transition-1;
		if (  t.smoothanim[obj].transition == 0 ) 
		{
			SetObjectInterpolation (  obj,100.0 );
			if (  t.smoothanim[obj].playflag == 1 ) 
			{
				if (  t.smoothanim[obj].playstarted == 0 ) 
				{
					PlayObject (  obj,t.smoothanim[obj].st,t.smoothanim[obj].fn );
					if (  t.smoothanim[obj].rev == 0 ) 
					{
						SetObjectSpeed (  obj,abs(GetSpeed(obj)) );
					}
					else
					{
						SetObjectSpeed (  obj,abs(GetSpeed(obj))*-1 );
						SetObjectFrame (  obj,t.smoothanim[obj].fn );
					}
					t.smoothanim[obj].playstarted=1;
				}
			}
			else
			{
				LoopObject (  obj,t.smoothanim[obj].st,t.smoothanim[obj].fn );
				if (  t.smoothanim[obj].rev == 0 ) 
				{
					SetObjectSpeed (  obj,abs(GetSpeed(obj)) );
				}
				else
				{
					SetObjectSpeed (  obj,abs(GetSpeed(obj))*-1 );
				}
			}
		}
	}
}
