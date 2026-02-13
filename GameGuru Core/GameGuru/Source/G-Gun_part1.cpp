bool bCustomGunAnimationRunning = false;

void gun_control ( void )
{
	//  trigger gun to show and play custom anim (custstart,custend)
	if (  t.gunmode == 9998 ) 
	{
		ShowObject (  t.currentgunobj );
		t.currentgunanimspeed_f = t.genericgunanimspeed_f;
		gun_SetObjectSpeed (  t.currentgunobj,t.currentgunanimspeed_f );
		gun_SetObjectInterpolation (  t.currentgunobj,100 );
		gun_PlayObject (  t.currentgunobj,g.custstart,g.custend );
		t.gunmode=9999;
	}
	if (  t.gunmode == 9999 ) 
	{
		t.currentgunanimspeed_f = t.genericgunanimspeed_f;
		gun_SetObjectSpeed (  t.currentgunobj, t.currentgunanimspeed_f );
		if (  GetFrame(t.currentgunobj) >= g.custend  )  t.gunmode = 5;
	}

	//  Gun Lag
	if (  t.gunzoommode != 0 ) 
	{
		t.gunlagspeed_f=g.firemodes[t.gunid][g.firemode].settings.zoomgunlagSpeed;
		t.gunlagxmax_f=g.firemodes[t.gunid][g.firemode].settings.zoomgunlagXmax;
		t.gunlagymax_f=g.firemodes[t.gunid][g.firemode].settings.zoomgunlagYmax;
	}
	else
	{
		t.gunlagspeed_f=g.firemodes[t.gunid][g.firemode].settings.gunlagSpeed;
		t.gunlagxmax_f=g.firemodes[t.gunid][g.firemode].settings.gunlagXmax;
		t.gunlagymax_f=g.firemodes[t.gunid][g.firemode].settings.gunlagYmax;
	}
	g.gunlagX_f = CurveValue(g.gunlagX_f-(t.cammovex_f)*0.01,g.gunlagX_f,t.gunlagspeed_f);
	g.gunlagY_f = CurveValue(g.gunlagY_f+(t.cammovey_f)*0.01,g.gunlagY_f,t.gunlagspeed_f);
	if (  g.gunlagX_f < - t.gunlagxmax_f  )  g.gunlagX_f  =  -t.gunlagxmax_f;
	if (  g.gunlagY_f < - t.gunlagymax_f  )  g.gunlagY_f  =  -t.gunlagymax_f;
	if (  g.gunlagX_f >  t.gunlagxmax_f  )  g.gunlagX_f  =  t.gunlagxmax_f;
	if (  g.gunlagY_f >  t.gunlagymax_f  )  g.gunlagY_f  =  t.gunlagymax_f;
	g.gunlagX_f = CurveValue(0,g.gunlagX_f,t.gunlagspeed_f*1.2);
	g.gunlagY_f = CurveValue(0,g.gunlagY_f,t.gunlagspeed_f*1.2);

	//  Gun Offset X and Y
	if (  t.gunzoommode != 0 && t.gunzoommode<11 && g.firemodes[t.gunid][g.firemode].settings.simplezoom != 0 ) 
	{
		g.gunOffsetX_f = CurveValue(g.firemodes[t.gunid][g.firemode].settings.simplezoomx_f,g.gunOffsetX_f,g.firemodes[t.gunid][g.firemode].settings.simplezoomspeed);
		g.gunOffsetY_f = CurveValue(g.firemodes[t.gunid][g.firemode].settings.simplezoomy_f,g.gunOffsetY_f,g.firemodes[t.gunid][g.firemode].settings.simplezoomspeed);
	}
	else
	{
		t.tx_f=0 ; t.ty_f=0;
		if (  (t.plrkeySHIFT) == 1 && t.playercontrol.movement != 0 ) { t.tx_f  =  g.firemodes[t.gunid][g.firemode].settings.runx_f  ; t.ty_f  =  g.firemodes[t.gunid][g.firemode].settings.runy_f; }
		g.gunOffsetX_f = CurveValue(t.tx_f,g.gunOffsetX_f,g.firemodes[t.gunid][g.firemode].settings.simplezoomspeed);
		g.gunOffsetY_f = CurveValue(t.ty_f,g.gunOffsetY_f,g.firemodes[t.gunid][g.firemode].settings.simplezoomspeed);
	}

	//  use player wobble to affect weapon bounce
	t.tadjustbasedonwobbley_f=((Cos(t.playercontrol.wobble_f)*t.playercontrol.wobbleheight_f)/10.0)-0.275;

	//  gun position offset and rotation
	if (  t.plrzoomin_f != 0.0 ) 
	{
		//  place gun when in zoom mode
		if (  g.firemodes[t.gunid][g.firemode].settings.simplezoom  !=  0 ) 
		{
			if (  g.firemodes[t.gunid][g.firemode].settings.simplezoommod  ==  0 ) 
			{
				t.tzplacement_f=g.firemodes[t.gunid][g.firemode].forward_f-(t.plrzoomin_f*5);
				PositionObject (  t.currentgunobj,g.firemodes[t.gunid][g.firemode].horiz_f+g.gunlagX_f+g.gunOffsetX_f,t.tadjustbasedonwobbley_f+g.firemodes[t.gunid][g.firemode].vert_f+g.gunlagY_f+g.gunOffsetY_f,t.tzplacement_f );
			}
			else
			{
				PositionObject (  t.currentgunobj,g.firemodes[t.gunid][g.firemode].horiz_f+g.gunlagX_f+g.gunOffsetX_f,t.tadjustbasedonwobbley_f+g.firemodes[t.gunid][g.firemode].vert_f+g.gunlagY_f+g.gunOffsetY_f,(g.firemodes[t.gunid][g.firemode].forward_f-(t.plrzoomin_f/g.firemodes[t.gunid][g.firemode].settings.simplezoommod)) );
			}
		}
		else
		{
			PositionObject (  t.currentgunobj,g.firemodes[t.gunid][g.firemode].horiz_f,t.tadjustbasedonwobbley_f+g.firemodes[t.gunid][g.firemode].vert_f,(g.firemodes[t.gunid][g.firemode].forward_f-(t.plrzoomin_f*10.0)) );
		}
		//  when in zoom mode, sway camera based on accuracy
		t.plrzoomaccuracydest_f=g.firemodes[t.gunid][g.firemode].settings.zoomaccuracy/10000.0;
		t.plrzoomaccuracyangleh_f=WrapValue(t.plrzoomaccuracyangleh_f+(Rnd(20)/15.0));
		t.plrzoomaccuracyanglev_f=WrapValue(t.plrzoomaccuracyanglev_f+(Rnd(20)/15.0));
		if (  t.plrzoomaccuracybreath >= 0 ) 
		{
			if (  KeyState(g.keymap[g.gzoomholdbreath]) == 1 && g.firemodes[t.gunid][g.firemode].settings.zoomaccuracybreathhold == 1 ) 
			{
				if (  t.plrzoomaccuracybreath == 0 ) 
				{
					t.plrzoomaccuracybreath= MAXTimer()+g.firemodes[t.gunid][g.firemode].settings.zoomaccuracybreath;
					if (  SoundExist(t.playercontrol.soundstartindex+31) == 1 ) 
					{
						PlaySound (  t.playercontrol.soundstartindex+31 );
					}
				}
				else
				{
					if (MAXTimer()<t.plrzoomaccuracybreath )
					{
						t.plrzoomaccuracydest_f=t.plrzoomaccuracydest_f*(g.firemodes[t.gunid][g.firemode].settings.zoomaccuracyheld/100.0);
					}
					else
					{
						t.plrzoomaccuracybreath=(MAXTimer()+2000)*-1;
						if (  SoundExist(t.playercontrol.soundstartindex+33) == 1 ) 
						{
							PlaySound (  t.playercontrol.soundstartindex+33 );
						}
					}
				}
			}
			else
			{
				if (  t.plrzoomaccuracybreath>0 ) 
				{
					t.plrzoomaccuracybreath=(MAXTimer()+200)*-1;
					if (  SoundExist(t.playercontrol.soundstartindex+31) == 1 ) 
					{
						if (  SoundPlaying(t.playercontrol.soundstartindex+31) == 0 ) 
						{
							if (  SoundExist(t.playercontrol.soundstartindex+32) == 1 ) 
							{
								PlaySound (  t.playercontrol.soundstartindex+32 );
							}
						}
					}
				}
			}
		}
		else
		{
			if (MAXTimer()<abs(t.plrzoomaccuracybreath) )
			{
				t.plrzoomaccuracydest_f=t.plrzoomaccuracydest_f*2.0;
			}
			else
			{
				if (  KeyState(g.keymap[g.gzoomholdbreath]) == 0 ) 
				{
					t.plrzoomaccuracybreath=0;
				}
			}
		}
		t.plrzoomaccuracy_f=CurveValue(t.plrzoomaccuracydest_f,t.plrzoomaccuracy_f,20.0);
		t.tswayx_f=Cos(t.plrzoomaccuracyangleh_f)*t.plrzoomaccuracy_f*g.timeelapsed_f*5;
		t.tswayy_f=Sin(t.plrzoomaccuracyanglev_f)*t.plrzoomaccuracy_f*g.timeelapsed_f*5;
		RotateCamera (  CameraAngleX()+t.tswayx_f,CameraAngleY()+t.tswayy_f,CameraAngleZ() );
	}
	else
	{
		//  place gun when not in zoom
		t.tforwardoffsettohideshoulder_f=-5;
		PositionObject (  t.currentgunobj,g.firemodes[t.gunid][g.firemode].horiz_f+g.gunlagX_f+g.gunOffsetX_f,t.tadjustbasedonwobbley_f+g.firemodes[t.gunid][g.firemode].vert_f+g.gunlagY_f+g.gunOffsetY_f,g.firemodes[t.gunid][g.firemode].forward_f+t.tforwardoffsettohideshoulder_f );
		t.plrzoomaccuracybreath=0;
		t.plrzoomaccuracy_f=0;
	}

	//  Rotate gun for natural effect
	if (  t.plrkeySLOWMOTION == 1 ) 
	{
		t.wax_f=CameraAngleX();
		t.way_f=CameraAngleY();
		t.waz_f=CameraAngleZ();
	}
	else
	{
		t.wax_f=CameraAngleX()-t.lastcamax_f;
		t.way_f=CameraAngleY()-t.lastcamay_f;
		t.waz_f=CameraAngleZ()-t.lastcamaz_f;
		t.lastcamax_f=CameraAngleX();
		t.lastcamay_f=CameraAngleY();
		t.lastcamaz_f=CameraAngleZ();
		float fGunLagSpeed = 20.0f-(t.gunlagspeed_f/5.0f);
		if ( fGunLagSpeed < 1.0f ) fGunLagSpeed = 1.0f;
		if ( fGunLagSpeed > 20.0f ) fGunLagSpeed = 20.0f;
		t.wox_f=t.wox_f+(t.wax_f/fGunLagSpeed);
		t.woy_f=t.woy_f+(t.way_f/fGunLagSpeed);
		t.woz_f=t.woz_f+(t.waz_f/fGunLagSpeed);
		t.wox_f=t.wox_f*0.9;
		t.woy_f=t.woy_f*0.9;
		t.woz_f=t.woz_f*0.9;
		t.sway_f=30.0 ; t.swayn_f=t.sway_f*-1;
		if (  t.woz_f<t.swayn_f  )  t.woz_f = t.swayn_f;
		if (  t.woz_f>t.sway_f  )  t.woz_f = t.sway_f;
		float fSwayX = t.gunlagxmax_f;
		float fSwayXN = -t.gunlagxmax_f;
		float fSwayY = t.gunlagymax_f;
		float fSwayYN = -t.gunlagymax_f;
		if ( t.wox_f < fSwayXN ) t.wox_f = fSwayXN;
		if ( t.wox_f > fSwayX ) t.wox_f = fSwayX;
		if ( t.woy_f < fSwayYN ) t.woy_f = fSwayYN;
		if ( t.woy_f > fSwayY ) t.woy_f = fSwayY;
	}

	RotateObject ( t.currentgunobj, t.wox_f, 180-t.woy_f, t.woz_f );

	//  hide the object if weapon-ammo and no qty left
	//  OR a grenade with no ammo and not throwing at the time
	t.tokay=0;
	if (  t.gun[t.gunid].settings.weaponisammo == 1 && t.weaponammo[g.weaponammoindex] == 0  )  t.tokay = 1;
	if (  t.gun[t.gunid].projectileframe != 0 && t.weaponammo[g.weaponammoindex] == 0 && t.gunmode<100  )  t.tokay = 1;
	if (  t.tokay == 1 ) 
	{
		HideObject (  t.currentgunobj );
	}
	else
	{
		gun_update_hud_visibility ( );
	}


	//PE: Custom Animation playing.
	if (bCustomGunAnimationRunning)
		return;

	//  generic speed of gun animations
	t.tidleormoving=0;
	for ( t.i = 0 ; t.i<=  1; t.i++ )
	{
		if (  GetFrame(t.currentgunobj)>g.firemodes[t.gunid][t.i].action.idle.s && GetFrame(t.currentgunobj)<g.firemodes[t.gunid][t.i].action.idle.e  )  t.tidleormoving = 1;
		if (  GetFrame(t.currentgunobj)>g.firemodes[t.gunid][t.i].action.move.s && GetFrame(t.currentgunobj)<g.firemodes[t.gunid][t.i].action.move.e  )  t.tidleormoving = 1;
		if (  GetFrame(t.currentgunobj)>g.firemodes[t.gunid][t.i].action.run.s && GetFrame(t.currentgunobj)<g.firemodes[t.gunid][t.i].action.run.e  )  t.tidleormoving = 1;
		if (  GetFrame(t.currentgunobj)>g.firemodes[t.gunid][t.i].zoomaction.idle.s && GetFrame(t.currentgunobj)<g.firemodes[t.gunid][t.i].zoomaction.idle.e  )  t.tidleormoving = 1;
		if (  GetFrame(t.currentgunobj)>g.firemodes[t.gunid][t.i].zoomaction.move.s && GetFrame(t.currentgunobj)<g.firemodes[t.gunid][t.i].zoomaction.move.e  )  t.tidleormoving = 1;
		if (  GetFrame(t.currentgunobj)>g.firemodes[t.gunid][t.i].emptyaction.idle.s && GetFrame(t.currentgunobj)<g.firemodes[t.gunid][t.i].emptyaction.idle.e  )  t.tidleormoving = 1;
		if (  GetFrame(t.currentgunobj)>g.firemodes[t.gunid][t.i].emptyaction.move.s && GetFrame(t.currentgunobj)<g.firemodes[t.gunid][t.i].emptyaction.move.e  )  t.tidleormoving = 1;
		if (  GetFrame(t.currentgunobj)>g.firemodes[t.gunid][t.i].emptyaction.run.s && GetFrame(t.currentgunobj)<g.firemodes[t.gunid][t.i].emptyaction.run.e  )  t.tidleormoving = 1;
	}

	// 060217 - choose the fast and slow speeds of the gun model
	float fGunAnimSpeedFast = 130.0f * t.gun[t.gunid].keyframespeed_f;
	float fGunAnimSpeedSlow = 5.0f * t.gun[t.gunid].keyframespeed_f;

	// Jump will slow down gun animation
	if (  t.playercontrol.jumpmode == 1 && t.tidleormoving == 1 ) 
	{
		//  slow right down if jumping
		if (  t.genericgunanimspeed_f == fGunAnimSpeedFast ) 
		{
			t.currentgunanimspeed_f = t.genericgunanimspeed_f;
			gun_SetObjectSpeed (  t.currentgunobj,t.currentgunanimspeed_f );
		}
		t.genericgunanimspeed_f = fGunAnimSpeedSlow;
	}
	else
	{
		//  regular speed for all weapon animations
		if (  t.genericgunanimspeed_f == fGunAnimSpeedSlow ) 
		{
			t.currentgunanimspeed_f = t.genericgunanimspeed_f;
			gun_SetObjectSpeed (  t.currentgunobj,t.currentgunanimspeed_f );
		}
		t.genericgunanimspeed_f = fGunAnimSpeedFast;
	}

	//  Zoom To/From Animations (show/hide)
	t.tzoomactionshow=g.firemodes[t.gunid][g.firemode].zoomaction.show;
	t.tzoomactionhide=g.firemodes[t.gunid][g.firemode].zoomaction.hide;
	t.tzoomactionidle=g.firemodes[t.gunid][g.firemode].zoomaction.idle;
	t.tzoomactionmove=g.firemodes[t.gunid][g.firemode].zoomaction.move;
	t.taltactionto=t.gun[t.gunid].altaction.to;
	t.taltactionfrom=t.gun[t.gunid].altaction.from;
	if (  t.weaponammo[g.weaponammoindex+g.ammooffset] == 0 ) 
	{
		//  080415 - when have no bullets, modify some anim sets
		if (  g.firemodes[t.gunid][g.firemode].emptyzoomactionshow.s>0 ) 
		{
			t.tzoomactionshow=g.firemodes[t.gunid][g.firemode].emptyzoomactionshow;
		}
		if (  g.firemodes[t.gunid][g.firemode].emptyzoomactionhide.s>0 ) 
		{
			t.tzoomactionhide=g.firemodes[t.gunid][g.firemode].emptyzoomactionhide;
		}
		if (  g.firemodes[t.gunid][g.firemode].emptyzoomactionidle.s>0 ) 
		{
			t.tzoomactionidle=g.firemodes[t.gunid][g.firemode].emptyzoomactionidle;
		}
		if (  g.firemodes[t.gunid][g.firemode].emptyzoomactionmove.s>0 ) 
		{
			t.tzoomactionmove=g.firemodes[t.gunid][g.firemode].emptyzoomactionmove;
		}
		if (  t.gun[t.gunid].emptyaltactionto.s>0 ) 
		{
			t.taltactionto=t.gun[t.gunid].emptyaltactionto;
		}
		if (  t.gun[t.gunid].emptyaltactionfrom.s>0 ) 
		{
			t.taltactionfrom=t.gun[t.gunid].emptyaltactionfrom;
		}
	}
	if (  t.gunmode  >=  2001 && t.gunmode  <=  2010 ) 
	{
		t.currentgunanimspeed_f = t.genericgunanimspeed_f;
		gun_SetObjectSpeed (  t.currentgunobj,t.currentgunanimspeed_f );
	}
	if (  t.gunmode  ==  2001 ) 
	{
		gun_SetObjectInterpolation (  t.currentgunobj,100 );
		t.gunmode = 2002;
		gun_PlayObject (  t.currentgunobj,t.tzoomactionshow.s,t.tzoomactionshow.e );
	}
	if (  t.gunmode  ==  2002 ) 
	{
		if (  GetFrame(t.currentgunobj) >= t.tzoomactionshow.e  )  t.gunmode = 5;
	}
	if (  t.gunmode  ==  2003 ) 
	{
		gun_SetObjectInterpolation (  t.currentgunobj,100 );
		t.gunmode = 2004;
		gun_PlayObject (  t.currentgunobj,t.tzoomactionhide.s,t.tzoomactionhide.e );
	}
	if (  t.gunmode  ==  2004 ) 
	{
		if (  GetFrame(t.currentgunobj) >= t.tzoomactionhide.e && t.gunzoommode == 0  )  t.gunmode = 5;
	}
	if (  t.gunmode  ==  2005 ) 
	{
		gun_SetObjectInterpolation (  t.currentgunobj,100 );
		t.gunmode = 2006;
		gun_PlayObject (  t.currentgunobj,t.tzoomactionhide.s,t.tzoomactionhide.e );
	}
	if (  t.gunmode  ==  2006 ) 
	{
		if (  GetFrame(t.currentgunobj) >= t.tzoomactionhide.e && t.gunzoommode == 0  )  t.gunmode = 121;
	}

	// handle switch weapon firemode
	if ( t.gunmode == 2007 ) 
	{
		if ( t.gunzoommode >=8 ) 
		{
			t.gunzoommode = 11;
			if ( t.tzoomactionhide.s > 0 )
			{
				t.gunmode = 2027;
				gun_SetObjectInterpolation (  t.currentgunobj,100 );
				gun_PlayObject ( t.currentgunobj, t.tzoomactionhide.s, t.tzoomactionhide.e );
			}
			else
				t.gunmode = 2017;
		}
		else
			t.gunmode = 2017;

		// 110718 - if about to perform firemode switch, and running
		if ( t.gunmode == 2017 && t.playercontrol.usingrun == 1 )
		{
			// intercept with anim to transition back to move first
			gun_SetObjectInterpolation ( t.currentgunobj, 100 );
			t.gruntofrom = g.firemodes[t.gunid][g.firemode].action.runfrom;
			PlayObject ( t.currentgunobj, t.gruntofrom.s, t.gruntofrom.e );
			t.gunmodewaitforframe = t.gruntofrom.e;
			t.gunmode = 2037;
		}
	}
	if ( t.gunmode == 2008 ) 
	{
		if ( GetFrame(t.currentgunobj) >= t.taltactionto.e ) 
		{
			t.tfireanim = 0;
			t.tmeleeanim = 0;
			t.gun[t.gunid].settings.alternate = 1;
			g.firemode=t.gun[t.gunid].settings.alternate;
			if ( t.playercontrol.usingrun == 1 )
			{
				t.gruntofrom = g.firemodes[t.gunid][g.firemode].action.runto;
				t.gunmode = 27;
			}
			else
				t.gunmode = 5;
		}
	}
	if ( t.gunmode == 2009 ) 
	{
		if ( t.gunzoommode >=8 ) 
		{
			t.gunzoommode = 11;
			if ( t.tzoomactionhide.s > 0 )
			{
				t.gunmode = 2029;
				gun_SetObjectInterpolation (  t.currentgunobj,100 );
				PlayObject ( t.currentgunobj, t.tzoomactionhide.s, t.tzoomactionhide.e );
			}
			else
				t.gunmode = 2019;
		}
		else
			t.gunmode = 2019;

		// 110718 - if about to perform firemode switch, and running
		if ( t.gunmode == 2019 && t.playercontrol.usingrun == 1 )
		{
			// intercept with anim to transition back to move first
			gun_SetObjectInterpolation ( t.currentgunobj, 100 );
			t.gruntofrom = g.firemodes[t.gunid][g.firemode].action.runfrom;
			gun_PlayObject ( t.currentgunobj, t.gruntofrom.s, t.gruntofrom.e );
			t.gunmodewaitforframe = t.gruntofrom.e;
			t.gunmode = 2039;
		}
	}
	if ( t.gunmode == 2010 ) 
	{
		if ( GetFrame(t.currentgunobj) >= t.taltactionfrom.e  )  
		{
			t.tfireanim = 0;
			t.tmeleeanim = 0;
			t.gun[t.gunid].settings.alternate = 0;
			g.firemode=t.gun[t.gunid].settings.alternate;
			if ( t.playercontrol.usingrun == 1 )
			{
				t.gruntofrom = g.firemodes[t.gunid][g.firemode].action.runto;
				t.gunmode = 27;
			}
			else
				t.gunmode = 5;
		}
	}
	if ( t.gunmode == 2017 )
	{
		t.gunmode = 2008;
		gun_SetObjectInterpolation (  t.currentgunobj,100 );
		gun_PlayObject (  t.currentgunobj,t.taltactionto.s,t.taltactionto.e );
		TextureObject (  g.hudbankoffset+5,0,g.firemodes[t.gunid][1].settings.flashimg );
	}
	if ( t.gunmode == 2019 )
	{
		t.gunmode = 2010;
		gun_SetObjectInterpolation (  t.currentgunobj,100 );
		gun_PlayObject (  t.currentgunobj,t.taltactionfrom.s,t.taltactionfrom.e );
		TextureObject (  g.hudbankoffset+5,0,g.firemodes[t.gunid][0].settings.flashimg );
	}
	if ( t.gunmode == 2027 ) 
	{
		if ( GetFrame(t.currentgunobj) >= t.tzoomactionhide.e ) t.gunmode = 2017;
	}
	if ( t.gunmode == 2029 ) 
	{
		if ( GetFrame(t.currentgunobj) >= t.tzoomactionhide.e )  t.gunmode = 2019;
	}
	if ( t.gunmode == 2037 ) 
	{
		if ( GetFrame(t.currentgunobj) >= t.gunmodewaitforframe ) t.gunmode = 2017;
	}
	if ( t.gunmode == 2039 ) 
	{
		if ( GetFrame(t.currentgunobj) >= t.gunmodewaitforframe )  t.gunmode = 2019;
	}
	
	// player must be at top speed before transitioning to run animation
	// when fire weapon, t.playercontrol.isrunningtime is updated with timer so does not trigger immediate run after firing
	bool bReallyRunning = false;
	bool bTrueFiring = false;
	if ( t.player[t.plrid].state.firingmode == 1 && t.weaponammo[g.weaponammoindex+g.ammooffset] > 0 ) bTrueFiring = true;
	if ( t.playercontrol.isrunning == 1 && bTrueFiring == false && t.gun[t.gunid].settings.ismelee == 0 ) 
	{
		// also ensure player is not reloading, meleeing, firing but is moving
		if ( t.playercontrol.movement != 0 && (t.gunmode < 121 || t.gunmode > 126) && (t.gunmode < 700 || t.gunmode > 707) )
		{
			if ( t.playercontrol.isrunningtime == 0 ) 
				t.playercontrol.isrunningtime = MAXTimer();
			else
				if (MAXTimer() > t.playercontrol.isrunningtime + g.firemodes[t.gunid][g.firemode].settings.runanimdelay )
					bReallyRunning = true;
		}
	}
	else
		t.playercontrol.isrunningtime = 0;

	if (  g.firemodes[t.gunid][g.firemode].settings.hasempty == 1 && g.firemodes[t.gunid][g.firemode].settings.isempty == 1 ) 
	{
		if (  t.gunzoommode == 10 && g.firemodes[t.gunid][g.firemode].settings.simplezoom  !=  0 ) 
		{
			if (  g.firemodes[t.gunid][g.firemode].settings.simplezoomanim  !=  0 ) 
			{
				//  Zoom out once we finish off the last of our shooting gunmode
				if (  t.gunmode<101 || t.gunmode>107 ) { t.gunzoommode = 11  ; t.gunmode  =  2003; }
				gun_getzoomstartandfinish ( );
				t.gautomatic=g.firemodes[t.gunid][g.firemode].zoomaction.automatic;
			}
			else
			{
				t.gunzoommode=11;
				gun_getstartandfinish ( false );
				t.gautomatic=g.firemodes[t.gunid][g.firemode].action.automatic;
			}
		}
		else
		{
			gun_getstartandfinish ( false );
			t.gautomatic=g.firemodes[t.gunid][g.firemode].action.automatic;
		}
		t.gshow=g.firemodes[t.gunid][g.firemode].emptyaction.show;
		t.gidle=g.firemodes[t.gunid][g.firemode].emptyaction.idle;
		if ( bReallyRunning==false && t.playercontrol.usingrun != -1 ) 
		{
			t.playercontrol.usingrun=-1;
			if (  t.gunmode >= 21 && t.gunmode <= 28  )  
			{
				bool bInRunningFrames = false;
				float fThisFrame = GetFrame(t.currentgunobj);
				if ( fThisFrame >= g.firemodes[t.gunid][g.firemode].emptyaction.run.s && fThisFrame <= g.firemodes[t.gunid][g.firemode].emptyaction.run.e ) bInRunningFrames = true;
				if ( g.firemodes[t.gunid][g.firemode].emptyaction.runfrom.s > 0 )// && bInRunningFrames == true )
					t.gunmode = 27; //21; // use run to move animation
				else
					t.gunmode = 21;
			}
		}
		if ( bReallyRunning == true && t.playercontrol.usingrun != 1 ) 
		{
			t.playercontrol.usingrun=1;
			if ( t.gunmode >= 21 && t.gunmode <= 28 )  
			{
				if ( g.firemodes[t.gunid][g.firemode].emptyaction.runto.s > 0 )
					t.gunmode = 27;//21; // use move to run animation
				else
					t.gunmode = 21;
			}
		}
		if ( t.playercontrol.usingrun == -1 ) 
		{
			t.gruntofrom = g.firemodes[t.gunid][g.firemode].emptyaction.runfrom;
			t.gmove = g.firemodes[t.gunid][g.firemode].emptyaction.move;
		}
		if ( t.playercontrol.usingrun == 1 ) 
		{
			t.gruntofrom = g.firemodes[t.gunid][g.firemode].emptyaction.runto;
			t.gmove = g.firemodes[t.gunid][g.firemode].emptyaction.run;
		}
		t.gstartreload=g.firemodes[t.gunid][g.firemode].emptyaction.startreload;
		t.greloadloop=g.firemodes[t.gunid][g.firemode].emptyaction.reloadloop;
		t.gendreload=g.firemodes[t.gunid][g.firemode].emptyaction.endreload;
		t.gcock=g.firemodes[t.gunid][g.firemode].emptyaction.cock;
		t.ghide=g.firemodes[t.gunid][g.firemode].emptyaction.hide;
	}
	else
	{
		t.gshow=g.firemodes[t.gunid][g.firemode].action.show;
		t.gidle=g.firemodes[t.gunid][g.firemode].action.idle;
		if ( bReallyRunning == false && t.playercontrol.usingrun != -1 ) 
		{
			t.playercontrol.usingrun=-1;
			if ( t.gunmode >= 21 && t.gunmode <= 28 )  
			{
				bool bInRunningFrames = false;
				float fThisFrame = GetFrame(t.currentgunobj);
				if ( fThisFrame >= g.firemodes[t.gunid][g.firemode].action.run.s && fThisFrame <= g.firemodes[t.gunid][g.firemode].action.run.e ) bInRunningFrames = true;
				if ( g.firemodes[t.gunid][g.firemode].action.runfrom.s > 0 )//&& bInRunningFrames == true )
					t.gunmode = 27; //21; // use move to run animation
				else
					t.gunmode = 21;
			}
		}
		if ( bReallyRunning == true && t.playercontrol.usingrun != 1 ) 
		{
			t.playercontrol.usingrun=1;
			if ( t.gunmode >= 21 && t.gunmode <= 28 )  
			{
				if ( g.firemodes[t.gunid][g.firemode].action.runto.s > 0 )
					t.gunmode = 27;//21; // use move to run animation
				else
					t.gunmode = 21;
			}
		}
		if ( t.playercontrol.usingrun == -1 ) 
		{
			t.gruntofrom = g.firemodes[t.gunid][g.firemode].action.runfrom;
			t.gmove = g.firemodes[t.gunid][g.firemode].action.move;
		}
		if ( t.playercontrol.usingrun == 1 ) 
		{
			t.gruntofrom = g.firemodes[t.gunid][g.firemode].action.runto;
			t.gmove = g.firemodes[t.gunid][g.firemode].action.run;
		}
		t.gautomatic=g.firemodes[t.gunid][g.firemode].action.automatic;
		t.gstartreload=g.firemodes[t.gunid][g.firemode].action.startreload;
		t.greloadloop=g.firemodes[t.gunid][g.firemode].action.reloadloop;
		t.gendreload=g.firemodes[t.gunid][g.firemode].action.endreload;
		t.gcock=g.firemodes[t.gunid][g.firemode].action.cock;
		t.ghide=g.firemodes[t.gunid][g.firemode].action.hide;
		if ( t.tfireanim == 0 ) 
		{
			if ( gun_getstartandfinish ( false ) == true )
			{
				// new true random selection of fire
				t.tempani = 1 + Rnd(2);
				if ( t.tempani == 2 && g.firemodes[t.gunid][g.firemode].action.start2.s == 0 ) t.tempani = 1;
				if ( t.tempani == 3 && g.firemodes[t.gunid][g.firemode].action.start3.s == 0 ) t.tempani = 1;
				if ( t.tempani == t.templastani ) 
				{
					t.tempani = t.templastani + 1;
					if ( t.tempani > 3 ) t.tempani = 1;
				}
				if ( t.tempani == 2 && g.firemodes[t.gunid][g.firemode].action.start2.s == 0 ) t.tempani = 1;
				if ( t.tempani == 3 && g.firemodes[t.gunid][g.firemode].action.start3.s == 0 ) t.tempani = 1;
				t.templastani = t.tempani;
				if ( t.tempani == 2 )
				{
					t.gstart=g.firemodes[t.gunid][g.firemode].action.start2;
					t.gfinish=g.firemodes[t.gunid][g.firemode].action.finish2;
				}
				if ( t.tempani == 3 )
				{
					t.gstart=g.firemodes[t.gunid][g.firemode].action.start3;
					t.gfinish=g.firemodes[t.gunid][g.firemode].action.finish3;
				}
				t.tfireanim = t.tempani;
			}
		}
		else
		{
			//if ( gun_getstartandfinish ( false ) == true )
			//{
			if (  t.tfireanim == 1 ) 
			{
				gun_getstartandfinish ( false );
			}
			if (  t.tfireanim == 2 ) 
			{
				t.gstart=g.firemodes[t.gunid][g.firemode].action.start2;
				t.gfinish=g.firemodes[t.gunid][g.firemode].action.finish2;
			}
			if (  t.tfireanim == 3 ) 
			{
				t.gstart=g.firemodes[t.gunid][g.firemode].action.start3;
				t.gfinish=g.firemodes[t.gunid][g.firemode].action.finish3;
			}
			//}
		}
	}

	if (  t.gunzoommode != 0 && g.firemodes[t.gunid][g.firemode].settings.simplezoom  !=  0 && g.firemodes[t.gunid][g.firemode].settings.simplezoomanim != 0 ) 
	{
		t.gidle=t.tzoomactionidle;
		t.gmove=t.tzoomactionmove;
		if ( t.playercontrol.usingrun == -1 ) t.gruntofrom = g.firemodes[t.gunid][g.firemode].action.runfrom;
		if ( t.playercontrol.usingrun == 1 ) t.gruntofrom = g.firemodes[t.gunid][g.firemode].action.runto;
		t.gautomatic=g.firemodes[t.gunid][g.firemode].zoomaction.automatic;
		t.gstartreload=g.firemodes[t.gunid][g.firemode].zoomaction.startreload;
		t.greloadloop=g.firemodes[t.gunid][g.firemode].zoomaction.reloadloop;
		t.gendreload=g.firemodes[t.gunid][g.firemode].zoomaction.endreload;
		t.gcock=g.firemodes[t.gunid][g.firemode].zoomaction.cock;
		t.gshow=t.tzoomactionshow;
		t.ghide=t.tzoomactionhide;

		//gun_getzoomstartandfinish ( );
		if ( t.tfireanim == 0 ) 
		{
			if ( gun_getzoomstartandfinish ( ) == true )
			{
				// new true random selection of fire
				t.tempani = 1 + Rnd(2);
				if ( t.tempani == 2 && g.firemodes[t.gunid][g.firemode].zoomaction.start2.s == 0 ) t.tempani = 1;
				if ( t.tempani == 3 && g.firemodes[t.gunid][g.firemode].zoomaction.start3.s == 0 ) t.tempani = 1;
				if ( t.tempani == t.templastani ) 
				{
					t.tempani = t.templastani + 1;
					if ( t.tempani > 3 ) t.tempani = 1;
				}
				if ( t.tempani == 2 && g.firemodes[t.gunid][g.firemode].zoomaction.start2.s == 0 ) t.tempani = 1;
				if ( t.tempani == 3 && g.firemodes[t.gunid][g.firemode].zoomaction.start3.s == 0 ) t.tempani = 1;
				t.templastani = t.tempani;
				if ( t.tempani == 2 )
				{
					t.gstart=g.firemodes[t.gunid][g.firemode].zoomaction.start2;
					t.gfinish=g.firemodes[t.gunid][g.firemode].zoomaction.finish2;
				}
				if ( t.tempani == 3 )
				{
					t.gstart=g.firemodes[t.gunid][g.firemode].zoomaction.start3;
					t.gfinish=g.firemodes[t.gunid][g.firemode].zoomaction.finish3;
				}
				t.tfireanim = t.tempani;
			}
		}
		else
		{
			if (  t.tfireanim == 1 ) 
			{
				gun_getzoomstartandfinish ( );
			}
			if (  t.tfireanim == 2 ) 
			{
				t.gstart=g.firemodes[t.gunid][g.firemode].zoomaction.start2;
				t.gfinish=g.firemodes[t.gunid][g.firemode].zoomaction.finish2;
			}
			if (  t.tfireanim == 3 ) 
			{
				t.gstart=g.firemodes[t.gunid][g.firemode].zoomaction.start3;
				t.gfinish=g.firemodes[t.gunid][g.firemode].zoomaction.finish3;
			}
		}
	}

	if (  t.gun[t.gunid].settings.ismelee == 2 ) 
	{
		if (  g.firemodes[t.gunid][g.firemode].settings.isempty == 0 || g.firemodes[t.gunid][g.firemode].emptyaction.start.s == 0 ) 
		{
			if (  t.tmeleeanim == 0 ) 
			{
				// new true random selection of melee
				t.tempmeani = 1 + Rnd(2);
				if ( t.tempmeani == 2 && g.firemodes[t.gunid][g.firemode].meleeaction.start2.s == 0 ) t.tempmeani = 1;
				if ( t.tempmeani == 3 && g.firemodes[t.gunid][g.firemode].meleeaction.start3.s == 0 ) t.tempmeani = 1;
				if ( t.tempmeani == t.tlastmeleeanim ) 
				{
					t.tempmeani = t.tlastmeleeanim + 1;
					if ( t.tempmeani > 3 ) t.tempmeani = 1;
				}
				if ( t.tempmeani == 2 && g.firemodes[t.gunid][g.firemode].meleeaction.start2.s == 0 ) t.tempmeani = 1;
				if ( t.tempmeani == 3 && g.firemodes[t.gunid][g.firemode].meleeaction.start3.s == 0 ) t.tempmeani = 1;
				t.tlastmeleeanim = t.tempmeani;
				if ( t.tempmeani == 1 )
				{
					t.gstart=g.firemodes[t.gunid][g.firemode].meleeaction.start;
					t.gfinish=g.firemodes[t.gunid][g.firemode].meleeaction.finish;
				}
				if ( t.tempmeani == 2 )
				{
					t.gstart=g.firemodes[t.gunid][g.firemode].meleeaction.start2;
					t.gfinish=g.firemodes[t.gunid][g.firemode].meleeaction.finish2;
				}
				if ( t.tempmeani == 3 )
				{
					t.gstart=g.firemodes[t.gunid][g.firemode].meleeaction.start3;
					t.gfinish=g.firemodes[t.gunid][g.firemode].meleeaction.finish3;
				}
			}
			else
			{
				if (  t.tmeleeanim == 1 ) 
				{
					t.gstart=g.firemodes[t.gunid][g.firemode].meleeaction.start;
					t.gfinish=g.firemodes[t.gunid][g.firemode].meleeaction.finish;
				}
				if (  t.tmeleeanim == 2 ) 
				{
					t.gstart=g.firemodes[t.gunid][g.firemode].meleeaction.start2;
					t.gfinish=g.firemodes[t.gunid][g.firemode].meleeaction.finish2;
				}
				if (  t.tmeleeanim == 3 ) 
				{
					t.gstart=g.firemodes[t.gunid][g.firemode].meleeaction.start3;
					t.gfinish=g.firemodes[t.gunid][g.firemode].meleeaction.finish3;
				}
			}
		}
		else
		{
			if (  g.firemodes[t.gunid][g.firemode].settings.isempty>0 ) 
			{
				if (  t.tmeleeanim == 0 ) 
				{
					// new true random selection of melee
					t.tempmeani = 1 + Rnd(2);
					if ( t.tempmeani == 2 && g.firemodes[t.gunid][g.firemode].emptyaction.start2.s == 0 ) t.tempmeani = 1;
					if ( t.tempmeani == 3 && g.firemodes[t.gunid][g.firemode].emptyaction.start3.s == 0 ) t.tempmeani = 1;
					if ( t.tempmeani == t.tlastmeleeanim ) 
					{
						t.tempmeani = t.tlastmeleeanim + 1;
						if ( t.tempmeani > 3 ) t.tempmeani = 1;
					}
					if ( t.tempmeani == 2 && g.firemodes[t.gunid][g.firemode].emptyaction.start2.s == 0 ) t.tempmeani = 1;
					if ( t.tempmeani == 3 && g.firemodes[t.gunid][g.firemode].emptyaction.start3.s == 0 ) t.tempmeani = 1;
					t.tlastmeleeanim = t.tempmeani;
					if ( t.tempmeani == 1 )
					{
						t.gstart=g.firemodes[t.gunid][g.firemode].emptyaction.start;
						t.gfinish=g.firemodes[t.gunid][g.firemode].emptyaction.finish;
					}
					if ( t.tempmeani == 2 )
					{
						t.gstart=g.firemodes[t.gunid][g.firemode].emptyaction.start2;
						t.gfinish=g.firemodes[t.gunid][g.firemode].emptyaction.finish2;
					}
					if ( t.tempmeani == 3 )
					{
						t.gstart=g.firemodes[t.gunid][g.firemode].emptyaction.start3;
						t.gfinish=g.firemodes[t.gunid][g.firemode].emptyaction.finish3;
					}
				}
				else
				{
					if (  t.tmeleeanim == 1 ) 
					{
						t.gstart=g.firemodes[t.gunid][g.firemode].emptyaction.start;
						t.gfinish=g.firemodes[t.gunid][g.firemode].emptyaction.finish;
					}
					if (  t.tmeleeanim == 2 ) 
					{
						t.gstart=g.firemodes[t.gunid][g.firemode].emptyaction.start2;
						t.gfinish=g.firemodes[t.gunid][g.firemode].emptyaction.finish2;
					}
					if (  t.tmeleeanim == 3 ) 
					{
						t.gstart=g.firemodes[t.gunid][g.firemode].emptyaction.start3;
						t.gfinish=g.firemodes[t.gunid][g.firemode].emptyaction.finish3;
					}
				}
			}
		}
		if (  t.tempmeani != 0 ) { t.tmeleeanim = t.tempmeani  ; t.tempmeani = 0; }
	}

	// Burst control
	if (  t.gunburst <= 0 ) 
	{
		t.gunburst=g.firemodes[t.gunid][g.firemode].settings.burst;
	}

	// 280618 - active/idle constant loopsound feature for weapons
	if ( 1 )
	{
		int iWeaponLoopSound = g.firemodes[t.gunid][g.firemode].sound.loopsound;
		if ( t.weaponammo[g.weaponammoindex+g.ammooffset] == 0 ) iWeaponLoopSound = g.firemodes[t.gunid][g.firemode].sound.emptyloopsound;
		t.sndid = 0;		
		if ( iWeaponLoopSound > 0 ) 
		{
			// only start activeidle once retrieve finished and in idle/move/run
			if ( t.gunmode >= 5 && t.gunmode < 31 )
				t.sndid = t.gunsound[t.gunid][iWeaponLoopSound].soundid1;
			else
				t.sndid = t.gunactiveidlesoundloopindex;
		}
		else
		{
			// only stop activeidle when finish switching alternate modes
			if ( t.gunmode >= 2007 && t.gunmode <= 2010 )
				t.sndid = t.gunactiveidlesoundloopindex;
			else
				t.sndid = 0;
		}
		if ( t.sndid != t.gunactiveidlesoundloopindex )
		{
			if ( t.sndid > 0 )
			{
				if ( SoundExist ( t.sndid ) ==1 )
					LoopSound ( t.sndid );
			}
			else
			{
				if ( t.gunactiveidlesoundloopindex > 0 )
					if ( SoundExist ( t.gunactiveidlesoundloopindex ) == 1 )
						StopSound ( t.gunactiveidlesoundloopindex );
			}
			t.gunactiveidlesoundloopindex = t.sndid;
		}
	}

	//  gun idle control ((4*0.75)=3.0)
	if ( t.gunmode == 5 ) 
	{
		t.gunmode=6;
		t.guninterp=4;
		gun_StopObject (  t.currentgunobj );
		gun_SetObjectInterpolation (  t.currentgunobj,25 );
		gun_SetObjectFrame (  t.currentgunobj,t.gidle.s+3.0 );
	}
	if (  t.gunmode == 6 ) 
	{
		--t.guninterp;
		if (  t.guninterp <= 0 ) 
		{
			gun_SetObjectInterpolation (  t.currentgunobj,100 );
			gun_SetObjectFrame (  t.currentgunobj,t.gidle.s+3.0 );
			t.gunmode=7;
		}
	}
	if (  t.gunmode == 7 ) 
	{
		t.gunmode=8;
		gun_PlayObject (  t.currentgunobj,t.gidle.s+3.0,t.gidle.e );
		t.currentgunanimspeed_f = t.genericgunanimspeed_f;
		gun_SetObjectSpeed (  t.currentgunobj,t.currentgunanimspeed_f );
	}
	if (  t.gunmode == 8 ) 
	{
		t.currentgunanimspeed_f = t.genericgunanimspeed_f;
		gun_SetObjectSpeed (  t.currentgunobj,t.currentgunanimspeed_f );
		if (  GetFrame(t.currentgunobj) >= t.gidle.e  )  t.gunmode = 9;
	}
	if (  t.gunmode == 9 ) 
	{
		t.gunmode=10;
		t.guninterp=4;
		gun_StopObject (  t.currentgunobj );
		gun_SetObjectInterpolation (  t.currentgunobj,25 );
		gun_SetObjectFrame (  t.currentgunobj,t.gidle.s+3.0 );
	}
	if (  t.gunmode == 10 ) 
	{
		--t.guninterp;
		if (  t.guninterp <= 0 ) 
		{
			gun_SetObjectInterpolation (  t.currentgunobj,100 );
			gun_SetObjectFrame (  t.currentgunobj,t.gidle.s+3.0 );
			t.gunmode=7;
		}
	}

	//  gun movment control ((4*0.75)=3.0
	if (  t.gunmode == 21 ) 
	{
		t.gunmode=22;
		gun_StopObject (  t.currentgunobj );
		gun_SetObjectInterpolation ( t.currentgunobj, 25 );
		t.guninterp=4;
		gun_SetObjectFrame (  t.currentgunobj,t.gmove.s+3.0 );
	}
	if (  t.gunmode == 22 ) 
	{
		--t.guninterp;
		if (  t.guninterp <= 0 ) 
		{
			gun_SetObjectInterpolation (  t.currentgunobj,100 );
			gun_SetObjectFrame (  t.currentgunobj,t.gmove.s+3.0 );
			t.gunmode=23;
		}
	}
	if (  t.gunmode >= 23 && t.gunmode <= 26 ) 
	{
		if (  t.playercontrol.movement == 0  )  t.gunmode = 5;
	}
	if (  t.gunmode == 23 ) 
	{
		t.gunmode=24;
		gun_PlayObject (  t.currentgunobj,t.gmove.s+3.0,t.gmove.e );
		if (  g.firemodes[t.gunid][g.firemode].settings.movespeedmod  ==  0 ) 
		{
			t.currentgunanimspeed_f = (t.playercontrol.basespeed_f*t.genericgunanimspeed_f);
			gun_SetObjectSpeed (  t.currentgunobj,t.currentgunanimspeed_f );
		}
		else
		{
			t.currentgunanimspeed_f = t.genericgunanimspeed_f;
			gun_SetObjectSpeed (  t.currentgunobj,t.currentgunanimspeed_f );
		}
	}
	if (  t.gunmode == 24 ) 
	{
		if (  g.firemodes[t.gunid][g.firemode].settings.movespeedmod  ==  0 ) 
		{
			t.currentgunanimspeed_f = (t.playercontrol.basespeed_f*t.genericgunanimspeed_f);
			gun_SetObjectSpeed (  t.currentgunobj,t.currentgunanimspeed_f );
		}
		else
		{
			t.currentgunanimspeed_f = t.genericgunanimspeed_f;
			gun_SetObjectSpeed (  t.currentgunobj,t.currentgunanimspeed_f );
		}
		if (  GetFrame(t.currentgunobj) >= t.gmove.e  )  t.gunmode = 25;
	}
	if (  t.gunmode == 25 ) 
	{
		t.gunmode=26;
		t.guninterp=4;
		gun_StopObject (  t.currentgunobj );
		gun_SetObjectInterpolation (  t.currentgunobj,25 );
		gun_SetObjectFrame (  t.currentgunobj,t.gmove.s+3.0 );
	}
	if (  t.gunmode == 26 ) 
	{
		--t.guninterp;
		if (  t.guninterp <= 0 ) 
		{
			gun_SetObjectInterpolation (  t.currentgunobj,100 );
			gun_SetObjectFrame (  t.currentgunobj,t.gmove.s+3.0 );
			t.gunmode=23;
		}
	}

	// 270618 - move to run animation sequence
	if ( t.gunmode == 27 ) 
	{
		t.gunmode = 28;
		gun_SetObjectInterpolation (  t.currentgunobj, 100 );
		gun_SetObjectFrame ( t.currentgunobj, t.gruntofrom.s );
		gun_PlayObject ( t.currentgunobj, t.gruntofrom.s, t.gruntofrom.e );
		t.currentgunanimspeed_f = (t.playercontrol.basespeed_f*t.genericgunanimspeed_f);
		gun_SetObjectSpeed (  t.currentgunobj, t.currentgunanimspeed_f );
		t.gunmodewaitforframe = t.gruntofrom.e;
	}
	if ( t.gunmode == 28 ) 
	{
		// monitor for when move to run transition finished
		if ( GetFrame(t.currentgunobj) >= t.gunmodewaitforframe ) t.gunmode = 21;
	}
	
	// gun put away and hide control
	if (  t.gunmode == 31 && g.noholster == 1 ) 
	{
		// Clear fired count if holstering
		t.gunmode=32;
		t.guninterp=4;
		gun_StopObject (  t.currentgunobj );
		gun_SetObjectInterpolation (  t.currentgunobj,100 );
		gun_SetObjectFrame (  t.currentgunobj,t.ghide.s );
		if (  t.gun[t.gunid].settings.alternate == 0  )  t.sndid = t.gunsound[t.gunid][4].soundid1 ; else t.sndid = t.gunsound[t.gunid][4].altsoundid;
		if (  t.sndid>0 ) 
		{
			if (  SoundExist(t.sndid) == 1 ) 
			{
				if (  SoundPlaying(t.sndid) == 0 ) 
				{
					playinternalsound(t.sndid);
				}
			}
		}
	}
	if (  t.gunmode == 32 ) 
	{
		--t.guninterp;
		if (  t.guninterp <= 0 ) 
		{
			gun_SetObjectInterpolation (  t.currentgunobj,100 );
			gun_SetObjectFrame (  t.currentgunobj,t.ghide.s );
			t.currentgunanimspeed_f = (t.genericgunanimspeed_f*2.5);
			gun_SetObjectSpeed (  t.currentgunobj,t.currentgunanimspeed_f );
			t.gunmode=33;
		}
	}
	if (  t.gunmode == 33 ) 
	{
		t.gunmode=34;
		gun_SetObjectInterpolation (  t.currentgunobj,100 );
		gun_PlayObject (  t.currentgunobj,t.ghide.s,t.ghide.e );
		t.currentgunanimspeed_f = (t.genericgunanimspeed_f*2.5);
		gun_SetObjectSpeed (  t.currentgunobj,t.currentgunanimspeed_f );
	}
	if (  t.gunmode == 34 ) 
	{
		t.currentgunanimspeed_f = (t.genericgunanimspeed_f*2.5);
		gun_SetObjectSpeed (  t.currentgunobj,t.currentgunanimspeed_f );
		if (  GetFrame(t.currentgunobj) >= t.ghide.e  )  t.gunmode = 35;
	}
	if (  t.gunmode == 35 ) 
	{
		t.currentgunanimspeed_f = (t.genericgunanimspeed_f*2.5);
		gun_SetObjectSpeed (  t.currentgunobj,t.currentgunanimspeed_f );
		g.autoloadgun=t.gunselectionafterhide;
		t.gunmode=5;
		if (  t.runwhenaway == 1 ) 
		{
			t.runwhenaway=2;
		}
	}

	// blocking control
	if ( t.gunmode == 1001 ) 
	{
		t.gunmode = 1002;
		t.currentgunanimspeed_f = t.genericgunanimspeed_f;
		gun_SetObjectSpeed ( t.currentgunobj,t.currentgunanimspeed_f );
		gun_SetObjectInterpolation ( t.currentgunobj,100 );
		t.gblock.s = g.firemodes[t.gunid][g.firemode].blockaction.start.s;
		t.gblock.e = g.firemodes[t.gunid][g.firemode].blockaction.finish.e;
		gun_PlayObject ( t.currentgunobj,t.gblock.s,t.gblock.e );
	}
	if (  t.gunmode == 1002 ) 
	{
		// currently in mist of blocking
		extern int g_iSuccessfullyBlockedAtTime;
		t.currentgunanimspeed_f = t.genericgunanimspeed_f;
		if (g_iSuccessfullyBlockedAtTime > 0)
		{
			// when the block actually repelled a hit
			float actualblockframe = g.firemodes[t.gunid][g.firemode].blockaction.start.e;
			if (GetFrame(t.currentgunobj) >= actualblockframe)
			{
				// sometimes block has gone PAST the ideal block frame, and this throws
				// off the expected counter phase timing
				gun_SetObjectInterpolation (t.currentgunobj, 25);
				if (g_iSuccessfullyBlockedAtTime == 1)
				{
					// so give the player a predicted start point for the block
					// when it receives an attack hitso the counter can be 'easily' timed
					gun_SetObjectFrame(t.currentgunobj, actualblockframe);
					gun_PlayObject (t.currentgunobj, actualblockframe, g.firemodes[t.gunid][g.firemode].blockaction.finish.e);
					g_iSuccessfullyBlockedAtTime = 2;
				}

				// slow down block animation during the remaining block animation (allows for the counter attack to be selected)
				float lastblockframe = g.firemodes[t.gunid][g.firemode].blockaction.finish.e;
				float halfthesize = (lastblockframe - actualblockframe)/2.0f;
				bool bWithinCounterWindowPhase = false;
				if (GetFrame(t.currentgunobj) <= actualblockframe + halfthesize)
				{
					// slow for 'bullet-time' chance to counter
					t.currentgunanimspeed_f = t.currentgunanimspeed_f * 0.2f;
					bWithinCounterWindowPhase = true;
				}

				// detect for attack trigger
				bool bTriggerCounterAttack = false;
				if (t.gunclick != 1 && t.gunclick != 3) t.gunmustreleasefirst = 0;
				if (t.gunmustreleasefirst == 0)
				{
					if (t.gunclick == 1 || t.gunclick == 3) bTriggerCounterAttack = true;
				}
				if (bTriggerCounterAttack == true)
				{
					// counter attack triggered
					t.gunmustreleasefirst = 0;
					if (bWithinCounterWindowPhase == true)
					{
						// yes, we are within the counter window
						t.gunmode = 1006;
					}
					else
					{
						// if miss counter phase window (waited too long after block), you
						// get the counter fail animation so the player knows they just missed it
						t.gunmode = 1008;
					}
				}
			}
			else
			{
				// or if block frame not reached yet, speed up to show it (as though we blocked on this frame)
				t.currentgunanimspeed_f = t.currentgunanimspeed_f * 4.0f;
			}
			if (t.currentgunanimspeed_f < 1) t.currentgunanimspeed_f = 1.0f;
		}
		gun_SetObjectSpeed (t.currentgunobj, t.currentgunanimspeed_f);
		t.gblock.e = g.firemodes[t.gunid][g.firemode].blockaction.finish.e;
		if (GetFrame(t.currentgunobj) >= t.gblock.e)
		{
			g_iSuccessfullyBlockedAtTime = 0;
			t.gunmode = 1003;
		}
		else
		{
			// if block has failed, play block fail animation
			if (t.player[1].state.blockingaction == 4)
			{
				int blockfailstart = g.firemodes[t.gunid][g.firemode].blockaction.dryfire.s;
				int blockfailfinish = g.firemodes[t.gunid][g.firemode].blockaction.dryfire.e;
				gun_PlayObject (t.currentgunobj, blockfailstart, blockfailfinish);
				g_iSuccessfullyBlockedAtTime = 0;
				t.gunmode = 1004;
			}
		}
	}
	if ( t.gunmode == 1003 ) 
	{
		gun_SetObjectInterpolation ( t.currentgunobj,100 );
		gun_SetObjectFrame( t.currentgunobj, g.firemodes[t.gunid][g.firemode].action.idle.s);
		t.gunmode=5;
		t.player[1].state.blockingaction=3;
	}
	if ( t.gunmode == 1004)
	{
		t.currentgunanimspeed_f = t.genericgunanimspeed_f;
		gun_SetObjectSpeed (t.currentgunobj, t.currentgunanimspeed_f);
		int blockfailfinish = g.firemodes[t.gunid][g.firemode].blockaction.dryfire.e;
		if (GetFrame(t.currentgunobj) >= blockfailfinish)
		{
			t.gunmode = 1003;
		}
	}

	// block counter handling
	if (t.gunmode == 1006 || t.gunmode == 1008)
	{
		// counter action triggered at right time, execute player animation for it
		// blockaction.start2.s - counter (1006) inside counter window
		// blockaction.finish2.s - counter fail (1008) outside counter window
		t.currentgunanimspeed_f = t.genericgunanimspeed_f;
		gun_SetObjectSpeed (t.currentgunobj, t.currentgunanimspeed_f);
		gun_SetObjectInterpolation (t.currentgunobj, 100);
		if (t.gunmode == 1006)
		{
			t.gblock.s = g.firemodes[t.gunid][g.firemode].blockaction.start2.s;
			t.gblock.e = g.firemodes[t.gunid][g.firemode].blockaction.start2.e;
		}
		if (t.gunmode == 1008)
		{
			t.gblock.s = g.firemodes[t.gunid][g.firemode].blockaction.finish2.s;
			t.gblock.e = g.firemodes[t.gunid][g.firemode].blockaction.finish2.e;
		}
		gun_PlayObject (t.currentgunobj, t.gblock.s, t.gblock.e);
		if (t.gunmode == 1006) t.gunmode = 1007;
		if (t.gunmode == 1008) t.gunmode = 1009;
	}
	if (t.gunmode == 1007 || t.gunmode == 1009)
	{
		// handle player animation of the counter attack
		t.currentgunanimspeed_f = t.genericgunanimspeed_f;
		gun_SetObjectSpeed (t.currentgunobj, t.currentgunanimspeed_f);
		gun_SetObjectInterpolation (t.currentgunobj, 25);
		if (GetFrame(t.currentgunobj) >= t.gblock.e)
		{
			g_iCounterAttackTargetForPlayer = 0;
			t.gunmode = 1003;
		}

		// if have a target in sight (the one who originally hit the player and was blocked)
		if (t.gunmode == 1007)
		{
			if (g_iCounterAttackTargetForPlayer > 0)
			{
				// make it a fast attack by default, not a tickle!
				t.currentgunanimspeed_f = t.genericgunanimspeed_f * 1.25f;
				gun_SetObjectSpeed (t.currentgunobj, t.currentgunanimspeed_f);

				// during the counter attack, the player locks onto the target
				int e = g_iCounterAttackTargetForPlayer;
				//needs to be smooth if this is to work!
				float fDX = t.entityelement[e].x - CameraPositionX();
				float fDZ = t.entityelement[e].z - CameraPositionZ();
				float fDA = atan2deg(fDX, fDZ);

				// and forces forward as through stepping forward
				t.playercontrol.pushangle_f = CameraAngleY();// fDA;
				t.playercontrol.pushforce_f = 0.01f;

				// when CONTACT (fist/etc) frame strikes
				float fContactFrame = t.gblock.s + ((t.gblock.e - t.gblock.s) * 0.5f);
				if (GetFrame(t.currentgunobj) >= fContactFrame)
				{
					// make a punch/thump sound
					extern void physics_play_thump_sound (float fX, float fY, float fZ, float fFreqStart, float fFreqRange);
					physics_play_thump_sound(CameraPositionX(), CameraPositionY(), CameraPositionZ(), 44000, Rnd(6000));

					// and a camera thumping shake
					t.playercontrol.camerashake_f = 75.0f;

					// until we get more char animations for the rollback, just ragdoll/deatrhanimit
					t.ttte = e;
					t.tdamage = 1;// t.entityelement[e].health;
					t.tdamageforce = 0.0f;
					t.brayx1_f = CameraPositionX();
					t.brayy1_f = CameraPositionY();
					t.brayz1_f = CameraPositionZ();
					t.brayx2_f = t.entityelement[e].x;
					t.brayy2_f = t.entityelement[e].y;
					t.brayz2_f = t.entityelement[e].z;
					t.tallowanykindofdamage = 0;
					t.twhox_f = t.brayx1_f;
					t.twhoz_f = t.brayz1_f;
					t.tdamagesource = 1;
					entity_applydamage();

					// do this once
					g_iCounterAttackTargetForPlayer = 0;

					// and signal the counter striek frame has been reached,
					// so logic can pick this up and do something (typically perform an animation in the DAMAGE state of the enemy)
					// and this is naturally reset when the blocking value is zero (t.player[1].state.blockingaction)
					t.player[1].state.counteredaction = 1;
				}
			}
		}
	}

	// control player being pushed reaction
	if (t.gunmode == 1011)
	{
		// thump to mark the impact
		extern void physics_play_thump_sound (float fX, float fY, float fZ, float fFreqStart, float fFreqRange);
		physics_play_thump_sound(CameraPositionX(), CameraPositionY(), CameraPositionZ(), 44000, Rnd(6000));

		// start player arms reaction
		t.currentgunanimspeed_f = t.genericgunanimspeed_f;
		gun_SetObjectSpeed (t.currentgunobj, t.currentgunanimspeed_f);
		gun_SetObjectInterpolation (t.currentgunobj, 100);
		if (rand()%2 == 0)
		{
			t.gblock.s = g.firemodes[t.gunid][g.firemode].blockaction.dryfire.s;
			t.gblock.e = g.firemodes[t.gunid][g.firemode].blockaction.dryfire.e;
		}
		else
		{
			t.gblock.s = g.firemodes[t.gunid][g.firemode].blockaction.flattentochest.s;
			t.gblock.e = g.firemodes[t.gunid][g.firemode].blockaction.flattentochest.e;
		}
		gun_PlayObject (t.currentgunobj, t.gblock.s, t.gblock.e);
		t.gunmode = 1012;
	}
	if (t.gunmode == 1012)
	{
		// monitor arms reaction animation
		t.currentgunanimspeed_f = t.genericgunanimspeed_f;
		gun_SetObjectSpeed (t.currentgunobj, t.currentgunanimspeed_f);
		gun_SetObjectInterpolation (t.currentgunobj, 25);
		if (GetFrame(t.currentgunobj) >= t.gblock.e)
		{
			// reset player arms to idle
			gun_SetObjectInterpolation (t.currentgunobj, 100);
			gun_SetObjectFrame(t.currentgunobj, g.firemodes[t.gunid][g.firemode].action.idle.s);
			t.gunmode = 5;
		}
		else
		{
			// and push player half the way
			float fHalfWay = (t.gblock.e - t.gblock.s) / 2;
			if (GetFrame(t.currentgunobj) <= t.gblock.e - fHalfWay)
			{
				t.playercontrol.pushangle_f = CameraAngleY() + 180;
				t.playercontrol.pushforce_f = 0.05f;
			}
		}
	}

	// melee gun modes
	if ( t.gunmode == 1020 ) 
	{
		t.gunmode=1021;
		t.currentgunanimspeed_f = t.genericgunanimspeed_f;
		gun_SetObjectSpeed (  t.currentgunobj,t.currentgunanimspeed_f );
		gun_SetObjectInterpolation (  t.currentgunobj,100 );
		gun_PlayObject (  t.currentgunobj,t.gstart.s,t.gstart.e );
	}
	if (  t.gunmode == 1021 ) 
	{
		t.currentgunanimspeed_f = t.genericgunanimspeed_f;
		gun_SetObjectSpeed (  t.currentgunobj,t.currentgunanimspeed_f );
		if (  GetFrame(t.currentgunobj) >= t.gstart.e  )  t.gunmode = 1022;
	}
	if (  t.gunmode == 1022 ) 
	{
		t.gunmode=1023;
		gun_PlayObject (  t.currentgunobj,t.gfinish.s,t.gfinish.e  ); t.gunshoot=1;
	}
	if (  t.gunmode == 1023 ) 
	{
		t.currentgunanimspeed_f = t.genericgunanimspeed_f;
		gun_SetObjectSpeed (  t.currentgunobj,t.currentgunanimspeed_f );
		if (  GetFrame(t.currentgunobj) >= t.gfinish.e ) 
		{  
			t.gun[t.gunid].settings.ismelee = 0; t.gunmode = 5; t.tmeleeanim = 0; 
		}
		else
		{
			// check if can perform a quick-reepeat attack
			if (gun_detectandperformquickrepeatattack() == true)
			{
				// and return to previous mode to allow start part to finish, and then gunshoot to do its thing again
				t.gunmode = 1021;
			}
		}
	}

	//  Player presses mouse button but has no ammo (avoid model animation freezing)
	if (  t.gunshootnoammo == 1 ) 
	{
		if (  t.weaponammo[g.weaponammoindex+g.ammooffset]>0 ) 
		{
			// do nothing in this case
		}
		else
		{
			t.gunandmelee.tmouseheld=1;
			if ( t.gunclick != 1 ) 
			{
				// 270618 - ensure cannot do dry fire sound when running
				if ( t.playercontrol.usingrun != 1 && t.gunandmelee.pressedtrigger == 0 )
				{
					// dry fire
					t.gunandmelee.pressedtrigger = 1;
					if ( t.gun[t.gunid].settings.alternate == 0  )  t.sndid = t.gunsound[t.gunid][3].soundid1; else t.sndid = t.gunsound[t.gunid][3].altsoundid;
					if ( t.sndid>0 ) 
					{
						if ( SoundExist(t.sndid) == 1 ) 
						{
							if ( SoundPlaying(t.sndid) == 0 ) 
							{
								if ( g.firemodes[t.gunid][g.firemode].settings.equipment == 0 ) 
								{
									playinternalsound(t.sndid);
								}
							}
						}
					}
				}
			}
		}
		t.gunshootnoammo=0;
	}

	//  Player presses reload button but has no ammo (avoid model animation freezing)
	if (  t.gunreloadnoammo == 1 ) 
	{
		g.plrreloading=0;
		t.tpool=g.firemodes[t.gunid][g.firemode].settings.poolindex;
		if (  t.tpool == 0  )  t.ammo = t.weaponclipammo[g.weaponammoindex+g.ammooffset]; else t.ammo = t.ammopool[t.tpool].ammo;
		if (  t.ammo == 0 || t.gun[t.gunid].settings.weaponisammo == 1 ) 
		{
			if (  t.gun[t.gunid].settings.weaponisammo == 0 ) 
			{
				// dry fire fake replaced with unique noammo sound
				t.sndid = t.playercontrol.soundstartindex+19;
				if ( t.sndid>0 ) 
				{
					if ( SoundExist(t.sndid) == 1 ) 
					{
						if ( SoundPlaying(t.sndid) == 0 ) 
						{
							playinternalsound(t.sndid);
						}
					}
				}
			}
		}
		t.gunreloadnoammo=0;
	}

	// gun firing control (or activate with equipment)
	t.tgunactivateequipment=0;
	if ( t.gunmode == 101 ) 
	{
		if ( g.firemodes[t.gunid][g.firemode].settings.reloadqty == 0 ) t.weaponammo[g.weaponammoindex+g.ammooffset] = 99999;
		if ( t.weaponammo[g.weaponammoindex+g.ammooffset]>0 ) 
		{
			if ( g.firemodes[t.gunid][g.firemode].settings.flaklimb != -1 ) 
			{
				// Dave guntimercount = 0 causes an additional nade to be thrown at the start, so we ensure it isnt 0 here
				g.guntimercount = 6;

				//  weapons with flak attachments fire immediately
				if ( t.gun[t.gunid].projectileframe == 0 ) 
				{
					// unless its a delayed flak like a hand grenade
					t.gunflash=1 ; t.gunshoot=1 ; g.guntimercount=g.firemodes[t.gunid][g.firemode].settings.firerate/2;
				}
			}
			t.gunmode=102;
			gun_SetObjectInterpolation (  t.currentgunobj,100 );

			gun_PlayObject (  t.currentgunobj,t.gstart.s,t.gstart.e );
			t.currentgunanimspeed_f = t.genericgunanimspeed_f;
			gun_SetObjectSpeed (  t.currentgunobj,t.currentgunanimspeed_f );
		}
		else
		{
			if (  t.gunandmelee.pressedtrigger == 0 ) 
			{
				// ensure cannot click again until dryfire finished and mouse released
				t.gunandmelee.pressedtrigger=1;

				// dryfire animation (only if was not running)
				if ( t.gunmodelast < 21 || t.gunmodelast > 26 || (t.gunmodelast >= 21 && t.gunmodelast <= 26 && t.playercontrol.usingrun == -1) )
				{
					if (  t.gunzoommode == 0 ) 
					{
						t.gdryfire=g.firemodes[t.gunid][g.firemode].emptyaction.dryfire;
					}
					else
					{
						t.gdryfire=g.firemodes[t.gunid][g.firemode].emptyzoomactiondryfire;
					}
					if (  t.gdryfire.s>0 ) 
					{
						// play dryfire animation 
						gun_StopObject ( t.currentgunobj );
						gun_SetObjectInterpolation (  t.currentgunobj, 100 );
						gun_SetObjectFrame ( t.currentgunobj, t.gdryfire.s );
						gun_PlayObject ( t.currentgunobj, t.gdryfire.s, t.gdryfire.e );
						t.gunmode=109;

						// dryfire sound
						if ( t.gun[t.gunid].settings.alternate == 0 ) t.sndid = t.gunsound[t.gunid][3].soundid1; else t.sndid = t.gunsound[t.gunid][3].altsoundid;
						if ( t.sndid>0 ) 
						{
							if ( SoundExist(t.sndid) == 1 ) 
							{
								if ( SoundPlaying(t.sndid) == 0 ) 
								{
									if ( g.firemodes[t.gunid][g.firemode].settings.equipment == 0 ) 
									{
										playinternalsound(t.sndid);
									}
								}
							}
						}
					}
					else
					{
						t.gunmode=107;
					}
				}
				else
				{
					// return gunmode state right back to when before '101' fire was triggered (seamless animation with no dryfire interuption)
					t.gunmode = t.gunmodelast;
				}
			}
			else
			{
				t.gunmode=107;
			}
		}
	}
	if ( t.gunmode == 102 ) 
	{
		t.currentgunanimspeed_f = t.genericgunanimspeed_f;
		gun_SetObjectSpeed (  t.currentgunobj,t.currentgunanimspeed_f );
		//PE: Wizard (fire) is waiting here for the animation to change ?
		//https://github.com/TheGameCreators/GameGuruRepo/issues/672
		//not really a bug as "start fire = 44,45" in \gamecore\guns\fantasy\Staff\gunspec.txt will fix it.
		if (  GetFrame(t.currentgunobj) >= t.gstart.e  )  
		{
			// moved triggering of brass, smoke and flash to END of start of firing
			if ( t.gun[t.gunid].settings.brasslimb != -1 || t.gun[t.gunid].settings.brasslimb == -2 )
			{  
				// eject brass immediately
				// use regular or zoomed brass delay value (milliseconds)
				int iBrassDelay = g.firemodes[t.gunid][g.firemode].settings.brassdelay;
				if ( g.firemodes[t.gunid][g.firemode].settings.simplezoom != 0 && t.gunzoommode != 0 ) 
					iBrassDelay = g.firemodes[t.gunid][g.firemode].settings.zoombrassdelay;
				if ( iBrassDelay > 0 )
				{
					if ( t.gunbrasstrigger == 0 )
						t.gunbrasstrigger = timeGetTime() + iBrassDelay;
				}
				else
				{
					t.gunbrass = 1;
				}
				g.gunbrasscount = g.firemodes[t.gunid][g.firemode].settings.firerate/2; 
			}
			t.gunmode = 103;
		}
		if (  t.gun[t.gunid].projectileframe>0 ) 
		{
			//  Dave, guntimercount = 0 causes an additional nade to be thrown at the start, so we ensure it isnt 0 here
			g.guntimercount = 6;
			//  if a delayed flak, check when frame triggers it
			if (  GetFrame(t.currentgunobj) >= t.gun[t.gunid].projectileframe ) 
			{
				t.gunflash=1 ; t.gunshoot=1 ; g.guntimercount=g.firemodes[t.gunid][g.firemode].settings.firerate/2;
			}
		}
	}
	if (  t.gunmode == 103 ) 
	{
		t.gunmode=104;
		if (  g.firemodes[t.gunid][g.firemode].settings.equipment == 1 ) 
		{
			if (  GetNumberOfFrames(t.currentgunobj) == 0  )  t.tgunactivateequipment = 1;
		}
		else
		{
			if (  t.gun[t.gunid].projectileframe == 0 ) 
			{
				if (  g.firemodes[t.gunid][g.firemode].settings.flaklimb == -1 ) 
				{
					t.gunflash=1 ; t.gunshoot=1 ; g.guntimercount=g.firemodes[t.gunid][g.firemode].settings.firerate/2;
				}
			}
		}
		if ( g.firemodes[t.gunid][g.firemode].settings.doesnotuseammo == 0 )
		{
			t.weaponammo[g.weaponammoindex+g.ammooffset]=t.weaponammo[g.weaponammoindex+g.ammooffset]-1; 
		}
		--t.gunburst;
		if ( t.gun[t.gunid].settings.smokelimb != -1 || t.gun[t.gunid].settings.smokelimb==-2 ) {  t.gunsmoke = 1 ; g.gunsmokecount = g.firemodes[t.gunid][g.firemode].settings.firerate/2; }
		if ( g.firemodes[t.gunid][g.firemode].settings.equipment == 0 )
		{
			// trigger sound
			if ( t.gun[t.gunid].settings.alternate == 0 ) 
			{
				t.tgunsoundindex=1  ; gun_picksndvariant ( );
			}
			else
			{
				t.sndid=t.gunsound[t.gunid][1].altsoundid;
			}
			if (  t.sndid>0 ) 
			{
				if (  SoundExist(t.sndid) == 1 ) 
				{
					if (  t.gautomatic.s>0 && t.gun[t.gunid].settings.alternate == 0 || t.gautomatic.s>0 && t.gun[t.gunid].settings.alternate == 1 && t.gun[t.gunid].settings.alternateisray == 1 ) 
					{
						gun_LoopObject (  t.currentgunobj,t.gautomatic.s,t.gautomatic.e );
						t.currentgunanimspeed_f = t.genericgunanimspeed_f;
						gun_SetObjectSpeed (  t.currentgunobj,t.currentgunanimspeed_f );
						if (  t.gunmodeloopsnd>0 ) 
						{
							if (  SoundExist(t.gunmodeloopsnd) == 1  )  StopSound (  t.gunmodeloopsnd );
						}
						t.fireloopend = g.firemodes[t.gunid][g.firemode].sound.fireloopend;
						if ( t.fireloopend >= 0 )
						{
							// fireloop for automatic weapons
							PlaySoundOffset ( t.sndid, t.fireloopend  ); 
							LoopSound ( t.sndid, 0, t.fireloopend );
							t.gunmodeloopsnd=t.sndid ; t.gunmodeloopstarted= MAXTimer();
						}
						else
						{
							// when fireloop is negative, we use 'single instance' shots
							// and use negative value as MS time between instance plays
							PlaySound ( t.sndid );
							t.gunmodeloopsnd=0; t.gunmodeloopstarted= MAXTimer();
						}
						t.tvolume_f = 100.0f;// 95.0;
						t.tvolume_f = t.tvolume_f * t.audioVolume.soundFloat;
						SetSoundVolume ( t.sndid, t.tvolume_f );
					}
				}
			}
		}
	}
	if ( t.gunmode == 104 ) 
	{
		// create decal particles
		if (  g.firemodes[t.gunid][g.firemode].particle.decal_s != "" ) 
		{
			if (  g.firemodes[t.gunid][g.firemode].settings.simplezoom  !=  0 && t.gunzoommode  !=  0 && g.firemodes[t.gunid][g.firemode].settings.simplezoomflash  ==  1 ) 
			{
				t.x1_f=ObjectPositionX(t.currentgunobj)+g.firemodes[t.gunid][g.firemode].settings.zoommuzzlex_f;
				t.y1_f=ObjectPositionY(t.currentgunobj)+g.firemodes[t.gunid][g.firemode].settings.zoommuzzley_f;
				t.z1_f=ObjectPositionZ(t.currentgunobj)+g.firemodes[t.gunid][g.firemode].settings.zoommuzzlez_f;
			}
			else
			{
				t.x1_f=ObjectPositionX(t.currentgunobj)+g.firemodes[t.gunid][g.firemode].settings.muzzlex_f;
				t.y1_f=ObjectPositionY(t.currentgunobj)+g.firemodes[t.gunid][g.firemode].settings.muzzley_f;
				t.z1_f=ObjectPositionZ(t.currentgunobj)+g.firemodes[t.gunid][g.firemode].settings.muzzlez_f;
			}
			t.decalid=g.firemodes[t.gunid][g.firemode].particle.id;
			t.decalorient=2;
			g.decalx=t.x1_f;
			g.decaly=t.y1_f;
			g.decalz=t.z1_f;
			t.decalscalemodx=100 ; t.decalscalemody=t.decalscalemodx;
			t.originatore = 0; decalelement_create ( );
		}
		if ( t.weaponammo[g.weaponammoindex+g.ammooffset] > 0 )
		{
			// using old or delayed brass ejection system
			g.gunbrasscount -= g.timeelapsed_f;
			g.gunsmokecount -= g.timeelapsed_f;
			g.guntimercount -= g.timeelapsed_f;
			bool bBrassEjected = false;
			if ( g.gunbrasscount <= 0 && (t.gun[t.gunid].settings.brasslimb != -1 || t.gun[t.gunid].settings.brasslimb == -2))
			{ 
				// use regular or zoomed brass delay value (milliseconds)
				int iBrassDelay = g.firemodes[t.gunid][g.firemode].settings.brassdelay;
				if ( g.firemodes[t.gunid][g.firemode].settings.simplezoom != 0 && t.gunzoommode != 0 ) 
					iBrassDelay = g.firemodes[t.gunid][g.firemode].settings.zoombrassdelay;

				bBrassEjected = true; 
				if ( iBrassDelay > 0 )
				{
					// see below for delayed brass ejection from this trigger
					if ( t.gunbrasstrigger == 0 )
					{
						t.gunbrasstrigger = timeGetTime() + iBrassDelay;
					}
				}
				else
				{
					t.gunbrass = 1; 
				}
				g.gunbrasscount = g.firemodes[t.gunid][g.firemode].settings.firerate/2; 
			}
			if (  g.gunsmokecount <= 0 && (t.gun[t.gunid].settings.smokelimb != -1 || t.gun[t.gunid].settings.smokelimb == -2) ) { t.gunsmoke = 1  ; g.gunsmokecount = g.firemodes[t.gunid][g.firemode].settings.firerate/2; }
			if (  g.firemodes[t.gunid][g.firemode].settings.equipment == 0 ) 
			{
				if (  t.gunflash == 0  )  t.gunflash = 1;
				if (  g.guntimercount <= 0 ) 
				{
					t.gunshoot=1 ; g.guntimercount=g.firemodes[t.gunid][g.firemode].settings.firerate/2; 
					if ( g.firemodes[t.gunid][g.firemode].settings.doesnotuseammo == 0 )
					{
						t.weaponammo[g.weaponammoindex+g.ammooffset]=t.weaponammo[g.weaponammoindex+g.ammooffset]-1;
					}
					--t.gunburst;
				}
			}
			if (  t.gautomatic.s == 0 || t.gun[t.gunid].settings.alternate == 1 && t.gun[t.gunid].settings.alternateisflak == 1  )  t.gunmode = 105;
			if (  t.gunclick != 1 && g.firemodes[t.gunid][g.firemode].settings.burst<1  )  t.gunmode = 105;
			if (  t.gunburst<1 && g.firemodes[t.gunid][g.firemode].settings.burst>0  )  t.gunmode = 105;
			if (  g.firemodes[t.gunid][g.firemode].settings.equipment == 0 ) 
			{
				if (  t.gun[t.gunid].settings.alternate == 0 ) 
				{
					t.tgunsoundindex=1 ; gun_picksndvariant ( );
				}
				else
				{
					t.sndid=t.gunsound[t.gunid][1].altsoundid;
				}
				t.fireloopend = g.firemodes[t.gunid][g.firemode].sound.fireloopend;
				if ( t.fireloopend >= 0 )
				{
					// regular fireloop handles loop timing
					if (  t.gunmodeloopsnd>0  )  t.gunmodeloopstarted = MAXTimer();
				}
				else
				{
					// negative fireloop causes single instance plays
					if ( bBrassEjected == true )
					{
						if (t.sndid > 0 && SoundExist(t.sndid) == 1)
						{
							PlaySound(t.sndid);
							t.tvolume_f = 100.0f;//95.0;
							t.tvolume_f = t.tvolume_f * t.audioVolume.soundFloat;
							SetSoundVolume(t.sndid, t.tvolume_f);
						}
					}
				}
				if (t.sndid > 0 && SoundExist(t.sndid) == 1)
					posinternal3dsound(t.sndid,CameraPositionX(),CameraPositionY(),CameraPositionZ());
			}

			// 270618 - intercept automatic loop shoot if 

			//PE: In autofire we should wait until ammo 0 , as we have not actually made the shot yet.
			//PE: When ammo 0 it will then trigger the "end fire" animation at the same time as the last shot.
			//if (t.weaponammo[g.weaponammoindex + g.ammooffset] == 1)
			if ( t.weaponammo[g.weaponammoindex+g.ammooffset] == 0 )
			{
				bool bUseLastStartAnim = false;
				if ( t.gunzoommode != 0 )
				{
					if ( g.firemodes[t.gunid][g.firemode].zoomaction.laststart.s > 0 )
					{
						gun_getzoomstartandfinish ( );
						bUseLastStartAnim = true;
					}
				}
				else
				{
					if ( g.firemodes[t.gunid][g.firemode].action.laststart.s > 0 )
					{
						gun_getstartandfinish ( true );
						bUseLastStartAnim = true;
					}
				}
				if ( bUseLastStartAnim == true )
				{
					t.gunmode=105;
					t.gunburst=0;
				}
			}
		}
		else
		{
			//  reset variable so burst won't attempt to finish upon reloading
			//  should also stop alt mode from retaining primary burst mode settings
			t.gunmode=105;
			t.gunburst=0;
		}
	}
	if ( t.gunbrasstrigger > 0 )
	{
		if ( timeGetTime() > t.gunbrasstrigger )
		{
			t.gunbrasstrigger = 0;
			t.gunbrass = 1; 
		}
	}
	if (  t.gunmode == 105 ) 
	{
		if ( t.gautomatic.s>0 && g.firemodes[t.gunid][g.firemode].settings.burst<1 ) 
		{
			// automatic weapons cannot resume firing right away
		}
		else
		{
			t.gunmustreleasefirst=1;
		}
		if ( t.gfinish.s>0 ) 
		{
			t.gunmode=106;
			t.gunmodewaitforframe=t.gfinish.e;
			gun_PlayObject (  t.currentgunobj,t.gfinish.s,t.gfinish.e );
			t.currentgunanimspeed_f = t.genericgunanimspeed_f;
			gun_SetObjectSpeed (  t.currentgunobj,t.currentgunanimspeed_f );
			if ( g.firemodes[t.gunid][g.firemode].settings.equipment == 0 ) 
			{
				t.fireloopend=g.firemodes[t.gunid][g.firemode].sound.fireloopend;
				if ( t.fireloopend >= 0 )
				{
					if (  t.gun[t.gunid].settings.alternate == 0 ) 
					{
						t.tgunsoundindex=1 ; gun_picksndvariant ( );
					}
					else
					{
						t.sndid=t.gunsound[t.gunid][1].altsoundid;
					}
					if (  t.sndid>0 ) 
					{
						if (  SoundExist(t.sndid) == 1 ) 
						{
							PositionSound ( t.sndid,CameraPositionX()/10.0,CameraPositionY()/3.0,CameraPositionZ()/10.0 );
							PlaySoundOffset ( t.sndid,t.fireloopend );
						}
					}
					posinternal3dsound(t.sndid,CameraPositionX(),CameraPositionY(),CameraPositionZ());
				}
			}
		}
		else
		{
			t.gunmode=107;
		}
	}
	if (  t.gunmode == 106 ) 
	{
		t.currentgunanimspeed_f = t.genericgunanimspeed_f;
		gun_SetObjectSpeed (  t.currentgunobj,t.currentgunanimspeed_f );

		// monitor for end of gunmode animation
		if (GetFrame(t.currentgunobj) >= t.gunmodewaitforframe)
		{
			// finalise animation
			t.gunmode = 107;
		}
		else
		{
			// check if can perform a quick-reepeat attack
			if (gun_detectandperformquickrepeatattack() == true )
			{
				// and return to 102 to allow start part to finish, and then gunshoot to do its thing again
				t.gunmode = 102;
			}
		}

		//  if a delayed flak, check when frame triggers it; Only one grenade, not SIX!!!
		if (  t.gun[t.gunid].projectileframe>0 ) 
		{
			if (  GetFrame(t.currentgunobj) >= t.gun[t.gunid].projectileframe ) 
			{
				t.gunflash=1 ; t.gunshoot=1 ; g.guntimercount=g.firemodes[t.gunid][g.firemode].settings.firerate/2;
				t.gunmode=108;
			}
		}
		//  detect sound slot 1 triggers, as these are USE actions and should ACTIVATE the equipment
		if (  g.firemodes[t.gunid][g.firemode].settings.equipment == 1 && GetNumberOfFrames(t.currentgunobj)>0 ) 
		{
			if (  t.gun[t.gunid].sound.soundframes>0 ) 
			{
				for ( t.p = 0 ; t.p<=  t.gun[t.gunid].sound.soundframes; t.p++ )
				{
					if (t.p < 100) // ensure cannot access sounditem items out of bounds!
					{
						if (t.gunsounditem[t.gunid][t.p].playsound == 1)
						{
							t.sndid = t.gunsound[t.gunid][t.gunsounditem[t.gunid][t.p].playsound].soundid1;
							if (t.sndid > 0)
							{
								if (int(t.gunsounditem[t.gunid][t.p].keyframe) == int(GetFrame(t.currentgunobj)))
								{
									t.tgunactivateequipment = 1;
								}
							}
						}
					}
				}
			}
		}
	}

	//  if equipment being used and LOCKCAMERA flag set, freeze out player
	if (  g.firemodes[t.gunid][g.firemode].settings.equipment != 0 && g.firemodes[t.gunid][g.firemode].settings.lockcamera == 1 ) 
	{
		if (  t.gunmode >= 101 && t.gunmode<107 ) 
		{
			g.mefrozentype=2 ; g.mefrozen= MAXTimer()+100;
		}
	}

	if ( t.gunmode == 107 ) 
	{
		// reset to normal
		if ( 1 ) 
		{
			// restore to idle

			//PE: After autofire we are here with one bullet left, and we must not play "alt last start fire" on next 101 run.
			//PE: Just played "alt end fire"

			t.gunmode=5;
			t.tfireanim=0;

			// ensure run anim does not kick in right away, leave for Xms until 
			t.playercontrol.isrunningtime = MAXTimer();

			// auto-reload if no bullets
			t.tpool=g.firemodes[t.gunid][g.firemode].settings.poolindex;
			if ( t.tpool == 0 ) t.ammo = t.weaponclipammo[g.weaponammoindex+g.ammooffset]; else t.ammo = t.ammopool[t.tpool].ammo;
			if ( t.weaponammo[g.weaponammoindex+g.ammooffset] == 0 ) 
			{
				if ( t.ammo>0 ) 
				{
					//  AirMod - No Auto Reload Feature
					if (  g.firemodes[t.gunid][g.firemode].settings.noautoreload == 0 ) 
					{
						t.gunmode=121;
					}
				}
			}

			// if equipment, reset freeze if any
			if ( g.firemodes[t.gunid][g.firemode].settings.equipment != 0 && g.firemodes[t.gunid][g.firemode].settings.lockcamera == 1 ) 
			{
				g.mefrozentype=0;
			}
		}
	}
	if (  t.gunmode == 108 ) 
	{
		//  continue running animation after a projectile delayed gunshoot
		t.currentgunanimspeed_f = t.genericgunanimspeed_f;
		gun_SetObjectSpeed (  t.currentgunobj,t.currentgunanimspeed_f );
		if (  GetFrame(t.currentgunobj) >= t.gfinish.e  )  t.gunmode = 107;
	}
	if ( t.gunmode == 109 ) 
	{
		// dryfire animation control
		t.currentgunanimspeed_f = t.genericgunanimspeed_f;
		gun_SetObjectSpeed (  t.currentgunobj,t.currentgunanimspeed_f );
		if ( GetFrame(t.currentgunobj) >= t.gdryfire.e ) 
		{
			t.gunmode=107;
		}
	}

	//  if gunloop sound continues beyond gunfire loop, end it!
	if (  t.gunmodeloopsnd>0 ) 
	{
		if (MAXTimer()>t.gunmodeloopstarted+300 )
		{
			if (  t.gunmode<101 || t.gunmode>107 ) 
			{
				if (  SoundExist(t.gunmodeloopsnd) == 1 ) 
				{
					if (  SoundPlaying(t.gunmodeloopsnd) == 1 ) 
					{
						StopSound (  t.gunmodeloopsnd );
						t.gunmodeloopsnd=0 ; t.gunmodeloopstarted=0;
					}
				}
			}
		}
	}
	//  and after 1 second, terminate LoopSound (  if not refreshed Sin ( ce (looping uzi issue) ) )
	if (  t.gunmodeloopsnd>0 ) 
	{
		if (MAXTimer()>t.gunmodeloopstarted+1000 )
		{
			if (  SoundExist(t.gunmodeloopsnd) == 1 ) 
			{
				if (  SoundPlaying(t.gunmodeloopsnd) == 1 ) 
				{
					StopSound (  t.gunmodeloopsnd );
					t.gunmodeloopsnd=0 ; t.gunmodeloopstarted=0;
				}
			}
		}
	}

	//  gun reload and cock control
	if (  t.gunmode == 121 ) 
	{
		//  240315 - if in zoom mode, get out if flagged
		if (  g.firemodes[t.gunid][g.firemode].settings.forcezoomout == 1 ) 
		{
			if (  t.gunzoommode == 9 || t.gunzoommode == 10  )  t.gunzoommode = 11;
		}
		//  Chambered round
		g.plrreloading=0;
		t.gunchamber = 0;
		if (  g.firemodes[t.gunid][g.firemode].settings.chamberedround>0  )  t.gunchamber  =  1;
		if (  t.weaponammo[g.weaponammoindex+g.ammooffset]  ==  0  )  t.gunchamber  =  0;
		//  Disable Simple Zoom on Reload
		if (  g.firemodes[t.gunid][g.firemode].settings.simplezoom  !=  0 && t.gunzoommode  !=  0 ) 
		{
			if (  t.gunzoommode == 10  )  t.gunzoommode = 11;
			if (  g.firemodes[t.gunid][g.firemode].settings.simplezoomanim  !=  0 ) {  t.gunmode  =  2005  ; gunmode121_cancel(); return; }
		}
		if (  g.firemodes[t.gunid][g.firemode].settings.forcezoomout == 1 && t.gunzoommode != 0 ) 
		{
			if (  t.gunzoommode == 10  )  t.gunzoommode = 11;
		}
		t.tpool=g.firemodes[t.gunid][g.firemode].settings.poolindex;
		if (  t.tpool == 0  )  t.ammo = t.weaponclipammo[g.weaponammoindex+g.ammooffset]; else t.ammo = t.ammopool[t.tpool].ammo;
		if (  t.ammo == 0 || t.gun[t.gunid].settings.weaponisammo == 1 ) 
		{
			t.gunmode=5;
		}
		else
		{
			t.gunmode=122;
			t.guninterp=4;
			gun_StopObject (  t.currentgunobj );
			gun_SetObjectInterpolation (  t.currentgunobj,25 );
			gun_SetObjectFrame (  t.currentgunobj,t.gstartreload.s );
		}
	}
	gunmode121_cancel();

	// map secondary gun object to primary for legacy arms trick
	if (t.currentgunobj > 0)
	{
		// seems the new sync anim system is thwarted by n-core work on the anim system - it seems
		// fallback is to match animation commands to secondary object :(
		int iGunSecondaryObj = g.gunbankextraobjoffset + (t.currentgunobj - g.gunbankoffset);
		if (ObjectExist(iGunSecondaryObj) == 1)
		{
			PositionObject(iGunSecondaryObj, ObjectPositionX(t.currentgunobj), ObjectPositionY(t.currentgunobj), ObjectPositionZ(t.currentgunobj));
			SetObjectToObjectOrientation(iGunSecondaryObj, t.currentgunobj);
			if (GetVisible(t.currentgunobj) == 1)
				ShowObject(iGunSecondaryObj);
			else
				HideObject(iGunSecondaryObj);
		}
	}
}

