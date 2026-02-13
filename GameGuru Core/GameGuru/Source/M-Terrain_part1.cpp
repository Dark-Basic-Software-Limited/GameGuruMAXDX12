void terrain_getpaintmode ( void )
{
	if (  t.conkit.entityeditmode == 1 ) 
	{
		if ( t.gridentitymarkersmodeonly == 0 )
			t.mode_s="Entity Mode";
		else
			t.mode_s="Marker Mode";
	}
	else
	{
		if (  t.terrain.terrainpaintermode >= 1 && t.terrain.terrainpaintermode <= 5 ) 
		{
			t.mode_s="Sculpt Terrain - ";
			if (  t.terrain.terrainpaintermode == 1  )  t.mode_s = t.mode_s+"Shape";
			if (  t.terrain.terrainpaintermode == 2  )  t.mode_s = t.mode_s+"Level";
			if (  t.terrain.terrainpaintermode == 3  )  t.mode_s = t.mode_s+"Stored Level";
			if (  t.terrain.terrainpaintermode == 4  )  t.mode_s = t.mode_s+"Blend";
			if (  t.terrain.terrainpaintermode == 5  )  t.mode_s = t.mode_s+"Ramp";
		}
		else
		{
			t.mode_s="Paint Terrain - ";
			if (  t.terrain.terrainpaintermode == 6  )  t.mode_s = t.mode_s+"Texture";
			if (  t.terrain.terrainpaintermode == 7  )  t.mode_s = t.mode_s+"Texture";
			if (  t.terrain.terrainpaintermode == 8  )  t.mode_s = t.mode_s+"Texture";
			if (  t.terrain.terrainpaintermode == 9  )  t.mode_s = t.mode_s+"Texture";
			if (  t.terrain.terrainpaintermode == 10  )  t.mode_s = t.mode_s+"Grass";
		}
	}
}

void terrain_loop ( void )
{
	//  F9 Editing Mode can change terrain heights, so need to
	//  adjust AI obstacle map to compensate
	if (  t.terrain.triggerobstaclerefresh>0 ) 
	{
		--t.terrain.triggerobstaclerefresh;
		if (  t.terrain.triggerobstaclerefresh == 0 ) 
		{
			t.terrain.terrainregionupdate=0;
			terrain_refreshterrainmatrix ( );
			darkai_obstacles_terrain_refresh ( );
		}
	}
}

void terrain_terraintexturesystempainterentry ( void )
{
	// track if terrain paint mode used or not (so can switch panel off when done)
	terrainbuild.bUsingTerrainTextureSystemPaintSelector = false;
}

void terrain_detectendofterraintexturesystempainter ( void )
{
	// if no terrain texture system painter in use, hide panel
	if ( terrainbuild.bUsingTerrainTextureSystemPaintSelector == false ) 
		terrain_paintselector_hide();
}

void terrain_editcontrol ( void )
{
	// terrain edit speed
	if (  t.inputsys.keycontrol == 1 ) 
		t.terrain.ts_f=t.ts_f/5.0;
	else
		t.terrain.ts_f=t.ts_f/2.5;

	// locate terrain coordinate
	t.tupdateterraincursor=0;
	if ( t.terrain.terrainpaintermode == 1 ) 
	{
		//  aise/lower should not shift cursor position as land changes
		if ( t.inputsys.mclick == 0 ) 
		{
			t.tupdateterraincursor=1;
		}
		else
		{
			if ( t.terrain.lastxmouse == t.inputsys.xmouse && t.terrain.lastymouse == t.inputsys.ymouse ) 
			{
				// do not move terrain cursor to create single hill/dip
			}
			else
			{
				t.tupdateterraincursor=1;
			}
		}
	}
	else
	{
		//  other modes can move cursor in all states
		t.tupdateterraincursor=1;
	}
	t.terrain.lastxmouse=t.inputsys.xmouse;
	t.terrain.lastymouse=t.inputsys.ymouse;
	if (  t.tupdateterraincursor == 1 ) 
	{
		t.terrain.X_f=t.inputsys.localx_f;
		t.terrain.Y_f=t.inputsys.localy_f;
	}

	//  Height at camera coordinate
	if ( t.terrain.TerrainID>0 ) 
		t.terrain.camheightatcoord_f=BT_GetGroundHeight(t.terrain.TerrainID,t.terrain.X_f,t.terrain.Y_f);
	else
		t.terrain.camheightatcoord_f=g.gdefaultterrainheight;

	// Only control keys while not editing entities
	if ( t.conkit.entityeditmode == 0 ) 
	{
		// Control painter objects
		if ( t.inputsys.k_s == "1"  )  t.terrain.terrainpaintermode = 1;
		if ( t.inputsys.k_s == "2"  )  t.terrain.terrainpaintermode = 2;
		if ( t.inputsys.k_s == "3"  )  t.terrain.terrainpaintermode = 3;
		if ( t.inputsys.k_s == "4"  )  t.terrain.terrainpaintermode = 4;
		if ( t.inputsys.k_s == "5"  )  t.terrain.terrainpaintermode = 5;
		if (t.inputsys.k_s == "6") {
			t.terrain.terrainpaintermode = 6;
		}
		if ( t.inputsys.k_s == "7"  )  t.terrain.terrainpaintermode = 7;
		if ( t.inputsys.k_s == "8"  )  t.terrain.terrainpaintermode = 8;
		if ( t.inputsys.k_s == "9"  )  t.terrain.terrainpaintermode = 9;
		if ( t.inputsys.k_s == "0"  )  t.terrain.terrainpaintermode = 10;
		t.tmin = 50; // 220216 - new standard size for both modes
		if ( t.inputsys.k_s == "-" && t.terrain.RADIUS_f>t.tmin  )  t.terrain.RADIUS_f = t.terrain.RADIUS_f-(25*t.terrain.ts_f);
		if ( t.inputsys.k_s == "=" && t.terrain.RADIUS_f<g.fTerrainBrushSizeMax  )  t.terrain.RADIUS_f = t.terrain.RADIUS_f+(25*t.terrain.ts_f);

		// move any entities lifted/dropped due to terrain sculpting (see code below)
		if ( t.terrain.TerrainID > 0 ) 
		{
			if ( t.terrain.terrainpaintermode >= 1 && t.terrain.terrainpaintermode <= 5 ) 
			{
				// this can also undo all InstanceStamp ( constructs )
				t.trevealallinstancestampentities=0;

				// raise any entities subject to this terrain radius
				for ( t.e = 1 ; t.e<=  g.entityelementlist; t.e++ )
				{
					t.entid=t.entityelement[t.e].bankindex;
					if (  t.entityelement[t.e].floorposy > 0 && t.entityelement[t.e].editorlock == false ) 
					{
						t.obj=t.entityelement[t.e].obj;
						if (  g.gridlayershowsingle == 1 ) 
						{
							//  do not select if TAB slice mode active and entity too big (buildings, walls, etc)
							if (  t.obj>0 ) 
							{
								if (  ObjectSizeX(t.obj)>95 && ObjectSizeY(t.obj)>95 && ObjectSizeZ(t.obj)>95 ) 
								{
									t.obj=0;
								}
							}
						}
						if (  t.obj>0 ) 
						{
							if (  ObjectExist(t.obj) == 1 ) 
							{
								t.tadjy_f=BT_GetGroundHeight(t.terrain.TerrainID,t.entityelement[t.e].x,t.entityelement[t.e].z)-t.entityelement[t.e].floorposy;
								if (  t.tadjy_f != 0 ) 
								{
									t.entityelement[t.e].y=t.entityelement[t.e].y+t.tadjy_f;
									if (  t.conkit.editmodeactive == 1 ) 
									{
										//  when in FPS 3D Edit Mode - when physics is ACTIVE - must reset entity in new position
										t.tphyobj=t.entityelement[t.e].obj;
										physics_disableobject ( );
										PositionObject (  t.tphyobj,t.entityelement[t.e].x,t.entityelement[t.e].y,t.entityelement[t.e].z );
										RotateObject (  t.tphyobj,t.entityelement[t.e].rx,t.entityelement[t.e].ry,t.entityelement[t.e].rz );
										physics_prepareentityforphysics ( );
										//  also record this change PERMINANTLY for return to editor
										if (  ArrayCount(t.storedentityelement)>0 ) 
										{
											t.storedentityelement[t.e].y=t.storedentityelement[t.e].y+t.tadjy_f;
										}
									}
									else
									{
										PositionObject (  t.obj,t.entityelement[t.e].x,t.entityelement[t.e].y,t.entityelement[t.e].z );
									}
								}
							}
						}
						t.entityelement[t.e].floorposy=0;
					}
				}
				if ( t.trevealallinstancestampentities == 1 ) 
				{
					// we have edited the entities in the game itself, so remove
					// InstanceStamp (  system for manual entity edit mode )
					for ( t.e = 1 ; t.e<=  g.entityelementlist; t.e++ )
					{
						t.entid=t.entityelement[t.e].bankindex;
						t.obj=t.entityelement[t.e].obj;
						if (  t.obj>0 ) 
						{
							if (  ObjectExist(t.obj) == 1 ) 
							{
								if (  t.entityprofile[t.entid].ismarker == 0 ) 
								{
									ShowObject (  t.obj );
								}
							}
						}
					}
				}
			}
		}

		// Any click means we modified the project
		t.mc = t.inputsys.mclick ; if (  t.mc == 2 || t.mc == 4  )  t.mc = 0;
		if ( t.mc!=0 )
		{
			g.projectmodified=1;
			gridedit_changemodifiedflag ( ); 
			// not affected g.projectmodifiedstatic
		}

		// record before area modified
		if ( t.mc != 0 ) 
		{ 
			// 301115 - wipe but can create new entity action below (for restoring entities displayed by terrain)
			if ( t.terrain.lastmc == 0 ) t.entityundo.action=0;
			terrain_recordbuffer ( );
		}

		// paint heights
		if ( t.terrain.TerrainID>0 ) 
		{
			if ( t.terrain.terrainpaintermode >= 1 && t.terrain.terrainpaintermode <= 5 ) 
			{
				//  Sculpt
				t.tmaketerraindirty=0;
				if (  t.mc != 0 ) 
				{
					if (  t.terrain.lastmc == 0  )  t.mconce = 1; else t.mconce = 0;
					if (  t.mc == 1 && t.inputsys.keyshift == 0 ) 
					{
						t.terrain.AMOUNT_f=50*t.terrain.zoom_f;
					}
					if (  t.mc == 1 && (t.inputsys.keyshift == 1 || (iTerrainRaiseMode == 0 && t.terrain.terrainpaintermode == 1) ) )
					{
						t.terrain.AMOUNT_f=-50*t.terrain.zoom_f;
					}
					if (  t.terrain.X_f>1 && t.terrain.X_f<(1024*50.0)-1 ) 
					{
						if (  t.terrain.Y_f>1 && t.terrain.Y_f<(1024*50.0)-1 ) 
						{
							//  record old entity Y positions relative to terrain Floor (  )
							if ( t.mconce == 1 )
							{
								g.entityrubberbandlistundo.clear();
								for ( t.e = 1 ; t.e<=  g.entityelementlist; t.e++ )
								{
									t.obj=t.entityelement[t.e].obj;
									if (  t.obj>0 ) 
									{
										t.ttdx_f=t.entityelement[t.e].x-t.terrain.X_f;
										t.ttdz_f=t.entityelement[t.e].z-t.terrain.Y_f;
										t.ttdd_f=Sqrt(abs(t.ttdx_f*t.ttdx_f)+abs(t.ttdz_f*t.ttdz_f));
										if (  t.ttdd_f <= t.terrain.RADIUS_f ) 
										{
											t.tentitytoselect = t.e;
											entity_recordbuffer_move();
											g.entityrubberbandlistundo.push_back ( t.entityundo );
										}
									}
								}
								if ( g.entityrubberbandlistundo.size() > 0 )
								{
									// special code to point this undo event to the rubberbandlist undo buffer
									t.entityundo.entityindex = -123;
									t.entityundo.bankindex = -123;
								}
							}
							for ( t.e = 1 ; t.e<=  g.entityelementlist; t.e++ )
							{
								t.obj=t.entityelement[t.e].obj;
								if (  t.obj>0 ) 
								{
									t.ttdx_f=t.entityelement[t.e].x-t.terrain.X_f;
									t.ttdz_f=t.entityelement[t.e].z-t.terrain.Y_f;
									t.ttdd_f=Sqrt(abs(t.ttdx_f*t.ttdx_f)+abs(t.ttdz_f*t.ttdz_f));
									if (  t.ttdd_f <= t.terrain.RADIUS_f ) 
									{
										t.entityelement[t.e].floorposy=BT_GetGroundHeight(t.terrain.TerrainID,t.entityelement[t.e].x,t.entityelement[t.e].z);
									}
								}
							}
							//  modify height map
							if (  t.terrain.terrainpaintermode == 1 ) 
							{
								BT_RaiseTerrain (  t.terrain.TerrainID,t.terrain.X_f,t.terrain.Y_f,t.terrain.RADIUS_f,t.terrain.AMOUNT_f*t.terrain.ts_f );
								t.tmaketerraindirty=1;
							}
							if (  t.terrain.terrainpaintermode == 2 ) 
							{
								if (  t.mconce == 1 ) 
								{
									t.terrain.terrainlevel_f=BT_GetGroundHeight(t.terrain.TerrainID,t.terrain.X_f,t.terrain.Y_f);
								}
								t.terrain.terrainlevelt_f=t.terrain.terrainlevel_f;
								if (  t.mc == 1 && t.inputsys.keyshift == 1 ) 
								{
									t.terrain.AMOUNT_f=50*t.terrain.zoom_f;
									t.terrain.terrainlevelt_f=t.terrain.terrainlevelt_f+Rnd(50)-25;
								}
								BT_RaiseTerrain (  t.terrain.TerrainID,t.terrain.X_f,t.terrain.Y_f,t.terrain.RADIUS_f,t.terrain.AMOUNT_f*t.terrain.ts_f,t.terrain.terrainlevelt_f );
								t.tmaketerraindirty=1;
							}
							if (  t.terrain.terrainpaintermode == 3 ) 
							{
								t.terrain.AMOUNT_f=50*t.terrain.zoom_f;
								BT_RaiseTerrain (  t.terrain.TerrainID,t.terrain.X_f,t.terrain.Y_f,t.terrain.RADIUS_f,t.terrain.AMOUNT_f*t.terrain.ts_f,t.terrain.terrainlevel_f );
								t.tmaketerraindirty=1;
							}
							if (  t.terrain.terrainpaintermode == 4 ) 
							{
								//  blend terrain heights
								t.trx=t.terrain.X_f/50.0;
								t.ttry=t.terrain.Y_f/50.0;
								t.trrad=t.terrain.RADIUS_f/50.0;
								//int newSize = (((t.trrad+1)*2)*((t.trrad+1)*2));
								//Dim (  t.storeheights_f, newSize );
								t.tstartx=t.trx-t.trrad ; t.tfinishx=t.trx+t.trrad;
								//t.tstarty=t.t-t.trrad ; t.tfinishy=t.ttry+t.trrad;
								t.tstarty=t.ttry-t.trrad ; t.tfinishy=t.ttry+t.trrad; // 310317 - typo creates crash!
								if ( t.trrad < 4 ) t.trrad = 4;
								int newSize = (t.tfinishx-t.tstartx)+((t.tfinishy-t.tstarty)*(t.trrad*2));
								Dim (  t.storeheights_f, newSize );

								for ( t.trady = t.tstarty ; t.trady<=  t.tfinishy; t.trady++ )
								{
									for ( t.tradx = t.tstartx ; t.tradx<=  t.tfinishx; t.tradx++ )
									{
										t.tindex=(t.tradx-t.tstartx)+(((t.trady-t.tstarty)*t.trrad*2));
										t.storeheights_f[t.tindex]=BT_GetGroundHeight(t.terrain.TerrainID,t.tradx*50,t.trady*50,1);
									}
								}
								for ( t.trady = t.tstarty+1 ; t.trady<=  t.tfinishy-1; t.trady++ )
								{
									for ( t.tradx = t.tstartx+1 ; t.tradx<=  t.tfinishx-1; t.tradx++ )
									{
										t.tdistx=t.tradx-t.trx ; t.tdisty=t.trady-t.ttry;
										t.tdist=Sqrt(abs(t.tdistx*t.tdistx)+abs(t.tdisty*t.tdisty));
										if (  t.tdist <= t.trrad ) 
										{
											t.tweightrad=t.trrad-1 ; t.tweight_f=((t.tweightrad-t.tdist)+0.0)/(t.tweightrad+0.0);
											if (  t.tweight_f<0  )  t.tweight_f = 0.0;
											if (  t.tweight_f>1.0  )  t.tweight_f = 1.0;
											t.tcurrentheight_f=BT_GetGroundHeight(t.terrain.TerrainID,t.tradx*50,t.trady*50,1);
											t.tindex=(t.tradx-t.tstartx)+(((t.trady-t.tstarty)*t.trrad*2));
											t.tcount=0 ; t.theight_f=0;
											for ( t.ttty = t.trady-1 ; t.ttty<=  t.trady+1; t.ttty++ )
											{
												for ( t.tttx = t.tradx-1 ; t.tttx<=  t.tradx+1; t.tttx++ )
												{
													if (  t.ttty >= 0 && t.ttty <= 1023 ) 
													{
														if (  t.tttx >= 0 && t.tttx <= 1023 ) 
														{
															t.ttindex=(t.tttx-t.tstartx)+(((t.ttty-t.tstarty)*t.trrad*2));
															if (  t.ttindex >= 0 && t.ttindex <= ArrayCount(t.storeheights_f) ) 
															{
																t.theight_f=t.theight_f+t.storeheights_f[t.ttindex];
																++t.tcount;
															}
														}
													}
												}
											}
											t.theight_f=t.theight_f/t.tcount;
											t.tblendfactor_f=(0.2*t.terrain.ts_f)*t.tweight_f;
											t.theight_f=(t.theight_f*t.tblendfactor_f)+(t.tcurrentheight_f*(1.0-t.tblendfactor_f));
											BT_SetPointHeight (  t.terrain.TerrainID,t.tradx,t.trady,t.theight_f );
										}
									}
								}
								t.tmaketerraindirty=1;
							}
							if (  t.terrain.terrainpaintermode == 5 ) 
							{
								//  form ramp
								if (  t.terrain.rubberbandrampflag == 0 ) 
								{
									t.terrain.rubberbandrampflag=1;
									t.terrain.rubberbandrampx=t.terrain.X_f;
									t.terrain.rubberbandrampy=t.terrain.Y_f;
									t.terrain.rubberbandrampradius=t.terrain.RADIUS_f;
									//  finished later when MC is released
								}
							}
						}
					}
					//  flag that we have changed the terrain height (for F9 mode to generate new water boundaries or not)
					t.terrain.heightsmodified=1;
					//  also flag to delete any grass within area of modification
					if (  t.conkit.editmodeactive == 1 ) 
					{
						if (  t.terrain.terrainregionupdate == 1 ) 
						{
							t.terrain.terrainregionupdate=0;
							t.terrain.grassregionupdate=0;
							grass_clearregion ( );
						}
					}
				}
				else
				{
					//  released left mouse button
					if (  t.terrain.terrainquickupdate == 1 ) 
					{
						if (  t.game.set.shaderrequirecheapshadow == 1 ) 
						{
							//  and update heightmap texture
							terrain_quickupdateheightmapfromheightdata ( );
						}
					}
					if (  t.terrain.rubberbandrampflag == 1 ) 
					{
						//  only if mouse not LEFT the edit area
						t.terrain.rubberbandrampflag=0;
						if (  t.inputsys.xmouse != 500000 ) 
						{
							//  create ramp from last recorded position to here
							t.tfromx_f=t.terrain.rubberbandrampx/50.0;
							t.tfromy_f=t.terrain.rubberbandrampy/50.0;
							t.ttox_f=t.terrain.X_f/50.0;
							t.ttoy_f=t.terrain.Y_f/50.0;
							t.tdiffx_f=t.tfromx_f-t.ttox_f;
							t.tdiffy_f=t.tfromy_f-t.ttoy_f;
							t.tangle_f=atan2deg(t.tdiffx_f,t.tdiffy_f);
							t.tdistance_f=Sqrt(abs(t.tdiffx_f*t.tdiffx_f)+abs(t.tdiffy_f*t.tdiffy_f))*50.0;
							t.trx1=t.tfromx_f;
							t.try1=t.tfromy_f;
							t.trrad1=t.terrain.rubberbandrampradius/50.0;
							t.theight1_f=BT_GetGroundHeight(t.terrain.TerrainID,t.trx1*50,t.try1*50,1);
							t.trx2=t.ttox_f;
							t.try2=t.ttoy_f;
							t.trrad2=t.terrain.RADIUS_f/50.0;
							t.theight2_f=BT_GetGroundHeight(t.terrain.TerrainID,t.trx2*50,t.try2*50,1);
							t.theightstep_f=(t.theight2_f-t.theight1_f);
							t.tslices_f=t.tdistance_f/25.0;
							t.tstepbit_f=t.theightstep_f/t.tslices_f;
							t.trbitx_f=(t.trx2-t.trx1+0.0)/t.tslices_f;
							t.trbity_f=(t.try2-t.try1+0.0)/t.tslices_f;
							for ( t.t = 0 ; t.t<=  (t.tslices_f-1); t.t++ )
							{
								t.trx3_f=(t.trx1+0.0)+(t.trbitx_f*t.t);
								t.try3_f=(t.try1+0.0)+(t.trbity_f*t.t);
								// Ensure a min size, so even the smallest radius does something
								t.ttrrad1 = t.trrad1-2 ; if (  t.ttrrad1<5  )  t.ttrrad1 = 5;
								t.ttrrad2 = t.trrad2-2 ; if (  t.ttrrad2<5  )  t.ttrrad2 = 5;
								t.ttr=t.ttrrad1+(((t.ttrrad2-t.ttrrad1+0.0)/t.tslices_f)*t.t);
								t.tedgettr=t.ttr;
								for ( t.tt = t.tedgettr*-1 ; t.tt<=  t.tedgettr; t.tt++ )
								{
									t.trxx=NewXValue(t.trx3_f,t.tangle_f+90,t.tt) ;
									t.tryy=NewZValue(t.try3_f, t.tangle_f+90,t.tt);
									t.tdestheight_f=(t.tstepbit_f*t.t);
									if (  t.tt >= t.tedgettr-2 || t.tt <= (t.tedgettr*-1)+2 ) 
									{
										//  set the ramp edge
										if (  t.theight1_f>t.theight2_f ) 
										{
											t.tdiff_f=(t.theight1_f-t.theight2_f)+t.tdestheight_f;
											t.tlowesth_f=t.theight2_f;
										}
										else
										{
											t.tdiff_f=(t.theight2_f-t.theight1_f)-t.tdestheight_f;
											t.tlowesth_f=t.theight1_f;
										}
										/* 061115 - removed feathering for now (low to high corrupts)
										if (  t.tt == t.tedgettr || t.tt == (t.tedgettr*-1) ) 
										{
											t.tfeather_f=0.2;
										}
										else
										{
											if (  t.tt == t.tedgettr-1 || t.tt == (t.tedgettr*-1)+1 ) 
											{
												t.tfeather_f=0.5;
											}
											else
											{
												t.tfeather_f=0.9;
											}
										}
										*/
										t.tfinalheight_f=t.tlowesth_f; //+(t.tdiff_f*t.tfeather_f);
										if (  t.tfinalheight_f<BT_GetGroundHeight(t.terrain.TerrainID,t.trxx*50,t.tryy*50,1) ) 
										{
											//  skip height set if current Floor (  HIGHER than the new height trying to set )
										}
										else
										{
											BT_SetPointHeight (  t.terrain.TerrainID,t.trxx,t.tryy,t.tfinalheight_f );
										}
									}
									else
									{
										//  set the solid ramp height
										BT_SetPointHeight (  t.terrain.TerrainID,t.trxx,t.tryy,t.theight1_f+t.tdestheight_f );
									}
								}
							}
							t.tmaketerraindirty=1;
						}
					}
				}
			}
			if (  t.tmaketerraindirty == 1 ) 
			{
				// mark dirty region for water mask (and other things eventually)
				t.x=t.terrain.X_f/50.0 ; t.z=t.terrain.Y_f/50.0 ; t.r=1+(t.terrain.RADIUS_f/50.0);
				if ( t.terrain.terrainregionupdate == 0 ) 
				{
					t.terrain.terrainregionupdate=1;
					t.terrain.terrainregionx1=t.x;
					t.terrain.terrainregionx2=t.x;
					t.terrain.terrainregionz1=t.z;
					t.terrain.terrainregionz2=t.z;
				}
				if ( t.x-t.r<t.terrain.terrainregionx1  )  t.terrain.terrainregionx1 = t.x-t.r;
				if ( t.x+t.r>t.terrain.terrainregionx2  )  t.terrain.terrainregionx2 = t.x+t.r;
				if ( t.z-t.r<t.terrain.terrainregionz1  )  t.terrain.terrainregionz1 = t.z-t.r;
				if ( t.z+t.r>t.terrain.terrainregionz2  )  t.terrain.terrainregionz2 = t.z+t.r;
				if ( t.terrain.terrainregionx1<0  )  t.terrain.terrainregionx1 = 0;
				if ( t.terrain.terrainregionx2>1024  )  t.terrain.terrainregionx2 = 1024;
				if ( t.terrain.terrainregionz1<0  )  t.terrain.terrainregionz1 = 0;
				if ( t.terrain.terrainregionz2>1024  )  t.terrain.terrainregionz2 = 1024;
				// mark quick refresh region (used to update heightmap texture for cheap shadow)
				if ( t.terrain.terrainquickupdate == 0 ) 
				{
					t.terrain.terrainquickupdate=1;
					t.terrain.terrainquickx1=t.x;
					t.terrain.terrainquickx2=t.x;
					t.terrain.terrainquickz1=t.z;
					t.terrain.terrainquickz2=t.z;
				}
				if ( t.x-t.r<t.terrain.terrainquickx1  )  t.terrain.terrainquickx1 = t.x-t.r;
				if ( t.x+t.r>t.terrain.terrainquickx2  )  t.terrain.terrainquickx2 = t.x+t.r;
				if ( t.z-t.r<t.terrain.terrainquickz1  )  t.terrain.terrainquickz1 = t.z-t.r;
				if ( t.z+t.r>t.terrain.terrainquickz2  )  t.terrain.terrainquickz2 = t.z+t.r;
				if ( t.terrain.terrainquickx1<0  )  t.terrain.terrainquickx1 = 0;
				if ( t.terrain.terrainquickx2>1024  )  t.terrain.terrainquickx2 = 1024;
				if ( t.terrain.terrainquickz1<0  )  t.terrain.terrainquickz1 = 0;
				if ( t.terrain.terrainquickz2>1024  )  t.terrain.terrainquickz2 = 1024;
				//  set terrain to dirty
				t.terrain.dirtyterrain=1;
				t.terrain.dirtyx1=((t.terrain.X_f-t.terrain.RADIUS_f)/50.0)-1;
				t.terrain.dirtyx2=((t.terrain.X_f+t.terrain.RADIUS_f)/50.0)+1;
				t.terrain.dirtyz1=((t.terrain.Y_f-t.terrain.RADIUS_f)/50.0)-1;
				t.terrain.dirtyz2=((t.terrain.Y_f+t.terrain.RADIUS_f)/50.0)+1;
				if (  t.terrain.dirtyx1<0  )  t.terrain.dirtyx1 = 0;
				if (  t.terrain.dirtyz1<0  )  t.terrain.dirtyz1 = 0;
				if (  t.terrain.dirtyx2>1023  )  t.terrain.dirtyx2 = 1023;
				if (  t.terrain.dirtyz2>1023  )  t.terrain.dirtyz2 = 1023;
				// now we need to raise the grass to this new terrain height
				t.terrain.grassupdateafterterrain = 1;
				t.terrain.grassregionx1 = t.terrain.dirtyx1;
				t.terrain.grassregionx2 = t.terrain.dirtyx2;
				t.terrain.grassregionz1 = t.terrain.dirtyz1;
				t.terrain.grassregionz2 = t.terrain.dirtyz2;
				bVegHasChanged = true;
			}
		}
		t.terrain.lastmc=t.mc;

		// Paint textures (6 through 10 was the old paint modes, used now to switch into paint system mode)
		if ( t.terrain.terrainpaintermode >= 6 && t.terrain.terrainpaintermode <= 10 ) 
		{
			// new terrain texture system paint selector
			terrainbuild.bUsingTerrainTextureSystemPaintSelector = true;
			terrain_paintselector_control ( );

			// detect if paint mode changes (reflect for now in currenttexture index)
			if ( t.terrain.terrainpaintermode != t.terrain.lastterrainpaintermode )
			{
				t.terrain.lastterrainpaintermode = t.terrain.terrainpaintermode;
				if ( t.terrain.terrainpaintermode == 6 ) terrainbuild.iCurrentTexture = 0;
				if ( t.terrain.terrainpaintermode == 7 ) terrainbuild.iCurrentTexture = 8;
				if ( t.terrain.terrainpaintermode == 8 ) terrainbuild.iCurrentTexture = 12;
				if ( t.terrain.terrainpaintermode == 9 ) terrainbuild.iCurrentTexture = 15;
				terrainbuild_settexturehighlight ( );
			}

			// handle painting of the terrain itself
			if ( t.mc == 1 && (t.terrain.X_f != t.terrain.lastpaintx_f || t.terrain.Y_f != t.terrain.lastpaintz_f) ) 
			{
				//  paint color
				t.terrain.lastpaintx_f=t.terrain.X_f;
				t.terrain.lastpaintz_f=t.terrain.Y_f;
				if ( t.terrain.terrainpaintermode != 10 ) 
				{
					if ( t.inputsys.keyshift == 1 || iTerrainPaintMode != 1 )
						t.texselect = Rgb(0,0,64);
					else
						t.texselect = Rgb(0,0,terrainbuild.iCurrentTexture*17);

					t.texselectgrass = 0;
					//if ( t.terrain.terrainpaintermode == 6 )  t.texselect  =  Rgb(0,0,0);
					//if ( t.terrain.terrainpaintermode == 7 )  t.texselect  =  Rgb(0,0,128);
					//if ( t.terrain.terrainpaintermode == 8 )  t.texselect  =  Rgb(0,0,192);
					//if ( t.terrain.terrainpaintermode == 9 )  t.texselect  =  Rgb(0,0,255);
				}
				if ( t.terrain.terrainpaintermode == 10 ) 
				{
					//  painting grass
					t.x=t.terrain.X_f/50.0 ; t.z=t.terrain.Y_f/50.0 ; t.r=1+(t.terrain.RADIUS_f/50.0);
					if (  t.terrain.grassregionupdate == 0 ) 
					{
						t.terrain.grassregionupdate=1;
						t.terrain.grassregionx1=t.x;
						t.terrain.grassregionx2=t.x;
						t.terrain.grassregionz1=t.z;
						t.terrain.grassregionz2=t.z;
					}
					if (  t.x-t.r<t.terrain.grassregionx1  )  t.terrain.grassregionx1 = t.x-t.r;
					if (  t.x+t.r>t.terrain.grassregionx2  )  t.terrain.grassregionx2 = t.x+t.r;
					if (  t.z-t.r<t.terrain.grassregionz1  )  t.terrain.grassregionz1 = t.z-t.r;
					if (  t.z+t.r>t.terrain.grassregionz2  )  t.terrain.grassregionz2 = t.z+t.r;
					if (  t.terrain.grassregionx1<0  )  t.terrain.grassregionx1 = 0;
					if (  t.terrain.grassregionx2>1024  )  t.terrain.grassregionx2 = 1024;
					if (  t.terrain.grassregionz1<0  )  t.terrain.grassregionz1 = 0;
					if (  t.terrain.grassregionz2>1024  )  t.terrain.grassregionz2 = 1024;

					// grass value stored in RED component
					t.texselectgrass = 1;
					if ( t.inputsys.keyshift == 1 || iTerrainPaintMode != 1 )
						t.texselect = Rgb(0,0,0);
					else
						t.texselect = Rgb(255,0,0);

					bVegHasChanged = true;
				}

				// finally paint to veghshadow texture
				terrain_paintterrain ( );

				// invalidate super texture as terrain paint changed
				t.terrain.generatedsupertexture = 0;
			}
		}

		// allows second sculpt/paint to erase first undo buffer
		if ( t.mc == 0  )  t.terrainundo.mode = 0;
	}
}

void ConvertVegMemblock ( int grassmemblock )
{
	// bitmap image now stored in RGBA format, so need to convert so memblock keeps using old format
	t.tPindex = 4+4+4;
	for ( t.tP = 0 ; t.tP<=  MAXTEXTURESIZE*MAXTEXTURESIZE - 1; t.tP++ )
	{
		BYTE pNewRed = ReadMemblockByte ( grassmemblock, t.tPindex+0 );
		BYTE pNewBlue = ReadMemblockByte ( grassmemblock, t.tPindex+2 );
		WriteMemblockByte ( grassmemblock,t.tPindex+0,pNewBlue );
		WriteMemblockByte ( grassmemblock,t.tPindex+2,pNewRed );
		t.tPindex += 4;
	}
}

void terrain_recordbuffer ( void )
{
	if (  1 ) 
	{
		if (  t.terrainundo.mode == 0 ) 
		{
			t.terrainundo.tux1=(t.terrain.X_f/50.0) ; t.terrainundo.tux2=t.terrainundo.tux1;
			t.terrainundo.tuz1=(t.terrain.Y_f/50.0) ; t.terrainundo.tuz2=t.terrainundo.tuz1;
			for ( t.tuz = 0 ; t.tuz<=  1024; t.tuz++ )
			{
				for ( t.tux = 0 ; t.tux<=  1024; t.tux++ )
				{
					t.terrainundobuffer[t.tux][t.tuz]=-1;
				}
			}
			//  snapshot present terrain paint and store in 124
			if (  MemblockExist(124) == 0  ) CreateMemblockFromImage (  124,t.terrain.imagestartindex+2 );
			if (  MemblockExist(123) == 1  )  CopyMemblock (  123,124,0,0,GetMemblockSize(123) );
			t.terrainundo.mode=1;
		}
		else
		{
			t.turadius=t.terrain.RADIUS_f/50.0;
			t.tux1 = (t.terrain.X_f/50.0)-1-t.turadius ; if (  t.tux1<0  )  t.tux1 = 0;
			t.tux2 = (t.terrain.X_f/50.0)+1+t.turadius ; if (  t.tux2>1024  )  t.tux2 = 1024;
			t.tuz1 = (t.terrain.Y_f/50.0)-1-t.turadius ; if (  t.tuz1<0  )  t.tuz1 = 0;
			t.tuz2 = (t.terrain.Y_f/50.0)+1+t.turadius ; if (  t.tuz2>1024  )  t.tuz2 = 1024;
			if (  t.terrainundo.tux1>t.tux1  )  t.terrainundo.tux1 = t.tux1;
			if (  t.terrainundo.tux2<t.tux2  )  t.terrainundo.tux2 = t.tux2;
			if (  t.terrainundo.tuz1>t.tuz1  )  t.terrainundo.tuz1 = t.tuz1;
			if (  t.terrainundo.tuz2<t.tuz2  )  t.terrainundo.tuz2 = t.tuz2;
			for ( t.tuz = t.tuz1 ; t.tuz<=  t.tuz2; t.tuz++ )
			{
				for ( t.tux = t.tux1 ; t.tux<=  t.tux2; t.tux++ )
				{
					if (  t.terrainundobuffer[t.tux][t.tuz] == -1 ) 
					{
						t.tx_f=t.tux*50 ; t.tz_f=t.tuz*50;
						t.terrainundobuffer[t.tux][t.tuz]=BT_GetGroundHeight(t.terrain.TerrainID,t.tx_f,t.tz_f,1);
					}
				}
			}
			t.terrainundo.bufferfilled=1;

			// 161115 - need this flag to ensure undo can happen again
			t.entityundo.undoperformed = 0;

		}
	}
}

void terrain_undo ( void )
{
	if (  t.entityundo.undoperformed == 0 ) 
	{
		if (  t.terrainundo.bufferfilled == 1 ) 
		{
			//  record heights for redo buffer
			for ( t.tuz = 0 ; t.tuz<=  1024; t.tuz++ )
			{
				for ( t.tux = 0 ; t.tux<=  1024; t.tux++ )
				{
					t.terrainredobuffer[t.tux][t.tuz]=-1;
				}
			}
			//  replace with old terrain heights
			for ( t.tuz = t.terrainundo.tuz1 ; t.tuz<=  t.terrainundo.tuz2; t.tuz++ )
			{
				for ( t.tux = t.terrainundo.tux1 ; t.tux<=  t.terrainundo.tux2; t.tux++ )
				{
					if (  t.terrainundobuffer[t.tux][t.tuz] != -1 ) 
					{
						if (  t.terrainredobuffer[t.tux][t.tuz] == -1 ) 
						{
							t.tx_f=t.tux*50 ; t.tz_f=t.tuz*50;
							t.terrainredobuffer[t.tux][t.tuz]=BT_GetGroundHeight(t.terrain.TerrainID,t.tx_f,t.tz_f,1);
						}
						BT_SetPointHeight (  t.terrain.TerrainID,t.tux,t.tuz,t.terrainundobuffer[t.tux][t.tuz] );
					}
				}
			}

			// replace new terrain paint with old one
			if (  MemblockExist(123) == 1 && MemblockExist(124) == 1 ) 
			{
				// first remember the original image (for the redo)
				CreateMemblockFromImage ( 125, t.terrain.imagestartindex+2 );

				// create restored paint texture from undo memblock
				CopyMemblock ( 124,123,0,0,GetMemblockSize(124) );
				CreateImageFromMemblock ( t.terrain.imagestartindex+2,123 );
				TextureObject ( t.terrain.terrainobjectindex, 0, t.terrain.imagestartindex+2 );
			}
			t.terrainundo.bufferfilled=1;
		}
	}
}

void terrain_redo ( void )
{
	//  310315 - can only redo if something in buffer (cannot redo if NO terrain activity yet)
	if (  t.entityundo.undoperformed == 1 && t.terrainundo.bufferfilled == 1 ) 
	{
		//  reverse terrain buffers
		for ( t.tuz = 0 ; t.tuz<=  1024; t.tuz++ )
		{
			for ( t.tux = 0 ; t.tux<=  1024; t.tux++ )
			{
				t.terrainundobuffer[t.tux][t.tuz]=t.terrainredobuffer[t.tux][t.tuz];
				t.terrainredobuffer[t.tux][t.tuz]=-1;
			}
		}
		//  replace with old terrain heights
		for ( t.tuz = 0 ; t.tuz<=  1024; t.tuz++ )
		{
			for ( t.tux = 0 ; t.tux<=  1024; t.tux++ )
			{
				if (  t.terrainundobuffer[t.tux][t.tuz] != -1 ) 
				{
					if (  t.terrainredobuffer[t.tux][t.tuz] == -1 ) 
					{
						t.tx_f=t.tux*50 ; t.tz_f=t.tuz*50;
						t.terrainredobuffer[t.tux][t.tuz]=BT_GetGroundHeight(t.terrain.TerrainID,t.tx_f,t.tz_f,1);
					}
					BT_SetPointHeight (  t.terrain.TerrainID,t.tux,t.tuz,t.terrainundobuffer[t.tux][t.tuz] );
				}
			}
		}

		//  put back the original image before the undo happened
		if (  MemblockExist(123) == 1 && MemblockExist(124) == 1 && MemblockExist(125) == 1 ) 
		{
			//  before redo image, record present image for possible later undo
			CreateMemblockFromImage ( 124, t.terrain.imagestartindex+2 );

			//  create image from redo buffer memblock (123 on this occasion)
			CopyMemblock (  125,123,0,0,GetMemblockSize(125) );
			CreateImageFromMemblock (  t.terrain.imagestartindex+2,123 );
			TextureObject (  t.terrain.terrainobjectindex,0,t.terrain.imagestartindex+2 );
		}

		//  undo buffer ready for undoing the redo
		for ( t.tuz = 0 ; t.tuz<=  1024; t.tuz++ )
		{
			for ( t.tux = 0 ; t.tux<=  1024; t.tux++ )
			{
				t.terrainundobuffer[t.tux][t.tuz]=t.terrainredobuffer[t.tux][t.tuz];
			}
		}
		t.terrainundo.bufferfilled=1;
	}
}

void terrain_editcontrol_auxiliary ( void )
{
	//  some terrain controls are triggered by entity placement
	if (  t.terrain.TerrainID>0 ) 
	{
		//  Sculpt Terrain Shapes
		if (  t.terrain.terrainpainteroneshot == 1 ) 
		{
			//  reset terrain region
			terrain_clearterraindirtyregion ( );
			t.terrain.terrainregionx1=-1;
			//  magnify (by a percent)
			t.terrain.shapeWidth_f=t.terrain.shapeWidth_f*1.1;
			t.terrain.shapeLong_f=t.terrain.shapeLong_f*1.25;
			//  record old terrain for the undo
			t.tstrad_f=t.terrain.RADIUS_f;
			t.terrain.RADIUS_f=t.terrain.shapeWidth_f;
			if (  t.terrain.shapeLong_f>t.terrain.RADIUS_f  )  t.terrain.RADIUS_f = t.terrain.shapeLong_f;
			t.terrainundo.mode=0;
			t.terrain.RADIUS_f=t.terrain.RADIUS_f*1.25;
			terrain_recordbuffer ( );
			terrain_recordbuffer ( );
			t.terrain.RADIUS_f=t.tstrad_f;
			//  set ground height to rotated rectangular platform (building placement)
			t.ttsideshiftstart_f=((t.terrain.shapeWidth_f/-2));
			t.ttsideshiftfinish_f=((t.terrain.shapeWidth_f/2)+50);
			t.ttsideshiftstartmargin_f=t.ttsideshiftstart_f*1.75;
			t.ttsideshiftfinishmargin_f=t.ttsideshiftfinish_f*1.75;
			for ( t.ttsideshift_f = t.ttsideshiftstartmargin_f ; t.ttsideshift_f<=  t.ttsideshiftfinishmargin_f ; t.ttsideshift_f+= 10 )
			{
				t.terrain.shapestartX_f=NewXValue(t.terrain.X_f,t.terrain.shapeA_f+90,t.ttsideshift_f);
				t.terrain.shapestartZ_f=NewZValue(t.terrain.Y_f,t.terrain.shapeA_f+90,t.ttsideshift_f);
				t.terrain.shapelineX1_f=NewXValue(t.terrain.shapestartX_f,t.terrain.shapeA_f+180,(t.terrain.shapeLong_f/2));
				t.terrain.shapelineZ1_f=NewZValue(t.terrain.shapestartZ_f,t.terrain.shapeA_f+180,(t.terrain.shapeLong_f/2));
				t.terrain.shapelineX2_f=NewXValue(t.terrain.shapestartX_f,t.terrain.shapeA_f+0,(t.terrain.shapeLong_f/2)+50);
				t.terrain.shapelineZ2_f=NewZValue(t.terrain.shapestartZ_f,t.terrain.shapeA_f+0,(t.terrain.shapeLong_f/2)+50);
				t.terrain.shapeStep_f=t.terrain.shapeLong_f;
				t.ttsubsteps=10;
				t.terrain.shapeincX_f=((t.terrain.shapelineX2_f-t.terrain.shapelineX1_f))/(t.terrain.shapeStep_f/t.ttsubsteps);
				t.terrain.shapeincZ_f=((t.terrain.shapelineZ2_f-t.terrain.shapelineZ1_f))/(t.terrain.shapeStep_f/t.ttsubsteps);
				t.terrain.stpX_f=t.terrain.shapelineX1_f;
				t.terrain.stpZ_f=t.terrain.shapelineZ1_f;
				t.terrain.stpX_f=t.terrain.stpX_f-(t.terrain.shapeincX_f*(t.terrain.shapeStep_f/t.ttsubsteps/2));
				t.terrain.stpZ_f=t.terrain.stpZ_f-(t.terrain.shapeincZ_f*(t.terrain.shapeStep_f/t.ttsubsteps/2));
				for ( t.ttstp = 1 ; t.ttstp <= int(t.terrain.shapeStep_f*2) ; t.ttstp+= t.ttsubsteps )
				{

					//  work out percentage of new terrain height influence
					t.tnewheightinfluence_f=0.0;
					if (  t.ttsideshift_f>t.ttsideshiftstart_f && t.ttsideshift_f<t.ttsideshiftfinish_f ) 
					{
						t.tnewheightinfluence_f=1.0;
					}
					else
					{
						t.tsize_f=abs(t.ttsideshiftstartmargin_f-t.ttsideshiftstart_f);
						if (  t.ttsideshift_f <= t.ttsideshiftstart_f ) 
						{
							t.tnewheightinfluence_f=(abs(t.ttsideshiftstartmargin_f)-abs(t.ttsideshift_f))/t.tsize_f;
						}
						else
						{
							if (  t.ttsideshift_f >= t.ttsideshiftfinish_f ) 
							{
								t.tnewheightinfluence_f=(abs(t.ttsideshiftfinishmargin_f)-abs(t.ttsideshift_f))/t.tsize_f;
							}
						}
					}
					t.tnewheightinfluence2_f=0.0;
					if (  t.ttstp>int(t.terrain.shapeStep_f*0.5) && t.ttstp<int(t.terrain.shapeStep_f*1.5) ) 
					{
						t.tnewheightinfluence2_f=1.0;
					}
					else
					{
						t.tsize_f=t.terrain.shapeStep_f*0.5;
						if (  t.ttstp <= int(t.terrain.shapeStep_f*0.5) ) 
						{
							t.tnewheightinfluence2_f=(t.ttstp+0.0)/t.tsize_f;
						}
						else
						{
							if (  t.ttstp >= int(t.terrain.shapeStep_f*1.5) ) 
							{
								t.tnewheightinfluence2_f=((abs(t.terrain.shapeStep_f*2)-abs(t.ttstp))+0.0)/t.tsize_f;
							}
						}
					}
					t.tnewheightinfluence_f=(t.tnewheightinfluence_f+t.tnewheightinfluence2_f)/2.0;
					if (  t.tnewheightinfluence_f<0.0  )  t.tnewheightinfluence_f = 0.0;
					if (  t.tnewheightinfluence_f>1.0  )  t.tnewheightinfluence_f = 1.0;

					//  get old height
					t.x=t.terrain.stpX_f/50.0;
					t.z=t.terrain.stpZ_f/50.0;
					t.toldheight_f=BT_GetGroundHeight(t.terrain.TerrainID,t.x*50.0,t.z*50.0);
					t.tfinalheight_f=(t.toldheight_f*(1.0-t.tnewheightinfluence_f))+(t.terrain.shapeHeight_f*t.tnewheightinfluence_f);

					//  apply new height
					BT_SetPointHeight (  t.terrain.TerrainID,t.x,t.z,t.tfinalheight_f );

					//  advance step
					t.terrain.stpX_f=t.terrain.stpX_f+t.terrain.shapeincX_f;
					t.terrain.stpZ_f=t.terrain.stpZ_f+t.terrain.shapeincZ_f;

					//  record region dirtied
					if (  t.terrain.terrainregionx1 == -1 ) 
					{
						t.terrain.terrainregionx1=t.x;
						t.terrain.terrainregionx2=t.x;
						t.terrain.terrainregionz1=t.z;
						t.terrain.terrainregionz2=t.z;
					}
					if (  t.x<t.terrain.terrainregionx1  )  t.terrain.terrainregionx1 = t.x;
					if (  t.x>t.terrain.terrainregionx2  )  t.terrain.terrainregionx2 = t.x;
					if (  t.z<t.terrain.terrainregionz1  )  t.terrain.terrainregionz1 = t.z;
					if (  t.z>t.terrain.terrainregionz2  )  t.terrain.terrainregionz2 = t.z;

				}
			}

			//  ensure terrain update region is updated
			if (  t.terrain.terrainregionx1<0  )  t.terrain.terrainregionx1 = 0;
			if (  t.terrain.terrainregionx2>1024  )  t.terrain.terrainregionx2 = 1024;
			if (  t.terrain.terrainregionz1<0  )  t.terrain.terrainregionz1 = 0;
			if (  t.terrain.terrainregionz2>1024  )  t.terrain.terrainregionz2 = 1024;

			//  only once
			t.terrain.X_f=-1000000 ; t.terrain.Y_f=-1000000;
			t.terrain.terrainpainteroneshot=2;

		}
		else
		{

			//  After terrain sculpt, move any
			if (  t.terrain.terrainpainteroneshot == 2 ) 
			{
				//  also rescue any entities that have been sent underground as a result
				for ( t.teee = 1 ; t.teee<=  g.entityelementlist; t.teee++ )
				{
					if (  t.entityelement[t.teee].x >= t.terrain.terrainregionx1*50 && t.entityelement[t.teee].x <= t.terrain.terrainregionx2*50 ) 
					{
						if (  t.entityelement[t.teee].z >= t.terrain.terrainregionz1*50 && t.entityelement[t.teee].z <= t.terrain.terrainregionz2*50 ) 
						{
							t.ttobj=t.entityelement[t.teee].obj;
							if (  t.ttobj>0 ) 
							{
								if (  ObjectExist(t.ttobj) == 1 ) 
								{
									if (  GetVisible(t.ttobj) == 1 ) 
									{
										t.entityelement[t.teee].y=BT_GetGroundHeight(t.terrain.TerrainID,t.entityelement[t.teee].x,t.entityelement[t.teee].z);
										PositionObject (  t.ttobj,t.entityelement[t.teee].x,t.entityelement[t.teee].y,t.entityelement[t.teee].z );
									}
								}
							}
						}
					}
				}
				//  now done
				t.terrain.terrainpainteroneshot=0;
			}
		}
	}
}

void terrain_paintterrain ( void )
{
	// receives terrain.X# Y# RADIUS# texselect texpainttype
	// create memblock for image manipulation
	if ( MemblockExist(123) == 0 )  
	{
		CreateMemblockFromImage ( 123, t.terrain.imagestartindex+2 );
	}
	if ( MemblockExist(123) == 1 ) 
	{
		// memblock header
		t.twid=ReadMemblockDWord(123,0);
		t.thig=ReadMemblockDWord(123,4);
		t.tdep=ReadMemblockDWord(123,8);

		// calculate coordinates to draw to
		t.tcntx=int(t.terrain.X_f/25.0)+1;
		t.tcntz=int(t.terrain.Y_f/25.0)+1;
		t.trad_f=t.terrain.RADIUS_f/35.0;
		t.trad=int(t.trad_f)-1;
		t.tzs = t.tcntz-t.trad; if ( t.tzs < 0 ) t.tzs = 0;
		t.tzf = t.tcntz+t.trad; if ( t.tzf > 2047 ) t.tzf = 2047;
		for ( t.tz = t.tzs; t.tz <= t.tzf; t.tz++ )
		{
			t.tti = (((t.tz-t.tzs)+0.0)/((t.tzf+0.0)-(t.tzs+0.0)))*100;
			if ( t.tti < 0 ) t.tti = 0;
			if ( t.tti > 100 ) t.tti = 100;
			t.txs = t.tcntx-(t.curve_f[t.tti]*t.trad); if ( t.txs < 0 ) t.txs = 0;
			t.txf = t.tcntx+(t.curve_f[t.tti]*t.trad); if ( t.txf > 2047 ) t.txf = 2047;
			for ( t.tx = t.txs; t.tx <= t.txf; t.tx++ )
			{
				t.mpos=12+(((t.tz*2048)+t.tx)*4);
				t.mrgb=ReadMemblockDWord(123,t.mpos);
				if ( t.texselectgrass == 1 )
				{
					// paint grass - do not affect floor
					t.tnewr=(RgbR(t.mrgb)*0.85f)+(RgbR(t.texselect)*0.15f);
					t.tnewg=RgbG(t.mrgb);
					t.tnewb=RgbB(t.mrgb);
				}
				else
				{
					// paint terrain floor - leave grass alone
					if ( RgbB(t.texselect) == 0 )
					{
						// slot 1 (path texture slot) also paints into overlay channel
						t.tnewr=RgbR(t.mrgb);
						t.tnewg=(RgbG(t.mrgb)*0.85f)+(255*0.15f);
						t.tnewb=RgbB(t.mrgb);
					}
					else
					{
						// regular transitions layer for 16 textures
						t.tnewr=RgbR(t.mrgb);
						t.tnewg=(RgbG(t.mrgb)*0.85f)+(RgbG(t.texselect)*0.15f);
						t.tnewb=(RgbB(t.mrgb)*0.85f)+(RgbB(t.texselect)*0.15f);
					}
				}
				WriteMemblockDWord ( 123, t.mpos, Rgb(t.tnewr,t.tnewg,t.tnewb) );
			}
		}
		CreateImageFromMemblock (  t.terrain.imagestartindex+2,123 );
		TextureObject (  t.terrain.terrainobjectindex,0,t.terrain.imagestartindex+2 );
	}
}

void terrain_cursor ( void )
{
	if (  t.terrain.terrainshaderindex>0 ) 
	{
		t.thalf_f=1024.0*25.0;
		float fHighlightX = t.terrain.X_f + 25;
		float fHighlightY = t.terrain.Y_f + 25;
		SetVector4 (  g.terrainvectorindex,(fHighlightX-t.thalf_f)/102.4,(fHighlightY-t.thalf_f)/102.4,t.terrain.RADIUS_f,0 );
		SetEffectConstantV (  t.terrain.terrainshaderindex,"HighlightCursor",g.terrainvectorindex );
		//if (  t.terrain.terrainpaintermode >= 6 && t.terrain.terrainpaintermode <= 10 ) 
		if ( t.terrain.terrainpaintermode == 10 ) 
		{
			if(!bEnableVeg || (bVegHasChanged && t.inputsys.mclick == 1) )
				SetVector4(g.terrainvectorindex, 1, 1, 1, 1);
			else
				SetVector4(g.terrainvectorindex, 1, 0, 1, 1);
		}
		else
		{
			SetVector4 (  g.terrainvectorindex,1,0,1,1 );
		}
		SetEffectConstantV (  t.terrain.terrainshaderindex,"HighlightParams",g.terrainvectorindex );
	}
}

void terrain_cursor_nograsscolor ( void )
{
	if (  t.terrain.terrainshaderindex>0 ) 
	{
		t.thalf_f=1024.0*25.0;
		SetVector4 (  g.terrainvectorindex,(t.terrain.X_f-t.thalf_f)/102.4,(t.terrain.Y_f-t.thalf_f)/102.4,t.terrain.RADIUS_f,0 );
		SetEffectConstantV (  t.terrain.terrainshaderindex,"HighlightCursor",g.terrainvectorindex );
		SetVector4 (  g.terrainvectorindex,1,0,t.terrain.entityeditmodecursorR,t.terrain.entityeditmodecursorG );
		SetEffectConstantV (  t.terrain.terrainshaderindex,"HighlightParams",g.terrainvectorindex );
	}
}

void terrain_cursor_off ( void )
{
	if (  t.terrain.terrainshaderindex>0 ) 
	{
		SetVector4 (  g.terrainvectorindex,0,0,0,0 );
		SetEffectConstantV (  t.terrain.terrainshaderindex,"HighlightCursor",g.terrainvectorindex );
		SetEffectConstantV (  t.terrain.terrainshaderindex,"HighlightParams",g.terrainvectorindex );
	}
}

void terrain_renderonly ( void )
{
	BT_RenderTerrain (  t.terrain.TerrainID );
}

void terrain_update ( void )
{
	if ( t.terrain.TerrainID > 0 ) 
	{
		BT_SetCurrentCamera (  t.terrain.gameplaycamera );
		BT_UpdateTerrainCull (  t.terrain.TerrainID );
		BT_UpdateTerrainLOD (  t.terrain.TerrainID );
		if (g.globals.riftmode > 0 || OpenXRIsSessionActive())
		{
			//  rendered in _postprocess_preterrain
		}
		else
		{
			//  main terrain render for normal mode
			terrain_renderonly ( );
		}
	}
}

void terrain_clearterraindirtyregion ( void )
{
	t.terrain.terrainregionupdate=0;
	t.terrain.terrainregionx1=0;
	t.terrain.terrainregionx2=0;
	t.terrain.terrainregionz1=0;
	t.terrain.terrainregionz2=0;
}

void terrain_cleargrassdirtyregion ( void )
{
	t.terrain.grassregionupdate=0;
	t.terrain.grassregionx1=0;
	t.terrain.grassregionx2=0;
	t.terrain.grassregionz1=0;
	t.terrain.grassregionz2=0;
}

void terrain_cleardirtyregion ( void )
{
	terrain_clearterraindirtyregion ( );
	terrain_cleargrassdirtyregion ( );
}

void terrain_waterineditor ( void )
{
	PositionObject (  t.terrain.objectstartindex+2,ObjectPositionX(t.terrain.objectstartindex+2),t.terrain.waterliney_f-t.terrain.waterlineyadjustforclip_f,ObjectPositionZ(t.terrain.objectstartindex+2) );
}

void terrain_assignnewshader ( void )
{
	// Choose the terrain shader to use
	//if ( t.terrain.iTerrainPBRMode == 1 )
	//if ( g.gpbroverride == 1 )
	//	t.terrain.terrainshaderindex = t.terrain.effectstartindex+5;
	//else
	t.terrain.terrainshaderindex = t.terrain.effectstartindex+0;

	// Terrain shader constants
	memset ( &t.effectparam.terrain, 0, sizeof(t.effectparam.terrain) );
	if ( GetEffectExist ( t.terrain.terrainshaderindex ) == 1 )
	{
		t.effectparam.terrain.g_lights_data=GetEffectParameterIndex(t.terrain.terrainshaderindex,"g_lights_data");
		t.effectparam.terrain.g_lights_pos0=GetEffectParameterIndex(t.terrain.terrainshaderindex,"g_lights_pos0");
		t.effectparam.terrain.g_lights_atten0=GetEffectParameterIndex(t.terrain.terrainshaderindex,"g_lights_atten0");
		t.effectparam.terrain.g_lights_diffuse0=GetEffectParameterIndex(t.terrain.terrainshaderindex,"g_lights_diffuse0");
		t.effectparam.terrain.g_lights_pos1=GetEffectParameterIndex(t.terrain.terrainshaderindex,"g_lights_pos1");
		t.effectparam.terrain.g_lights_atten1=GetEffectParameterIndex(t.terrain.terrainshaderindex,"g_lights_atten1");
		t.effectparam.terrain.g_lights_diffuse1=GetEffectParameterIndex(t.terrain.terrainshaderindex,"g_lights_diffuse1");
		t.effectparam.terrain.g_lights_pos2=GetEffectParameterIndex(t.terrain.terrainshaderindex,"g_lights_pos2");
		t.effectparam.terrain.g_lights_atten2=GetEffectParameterIndex(t.terrain.terrainshaderindex,"g_lights_atten2");
		t.effectparam.terrain.g_lights_diffuse2=GetEffectParameterIndex(t.terrain.terrainshaderindex,"g_lights_diffuse2");
		t.effectparam.terrain.SpotFlashPos=GetEffectParameterIndex(t.terrain.terrainshaderindex,"SpotFlashPos");
		t.effectparam.terrain.SpotFlashColor=GetEffectParameterIndex(t.terrain.terrainshaderindex,"SpotFlashColor");

		// wipe any previous param storage
		ResetEffect ( t.terrain.terrainshaderindex );

		// and ensure this terrain becoms responsible for shadow map generation
		ChangeShadowMappingPrimary ( t.terrain.terrainshaderindex );
	}
}

void terrain_applyshader ( void )
{
	// Apply effect and textures to terrain object
	if ( ObjectExist(t.terrain.terrainobjectindex) == 1 ) 
	{
		// Choose the terrain shader to use
		terrain_assignnewshader();

		// get handles to this new shader to place shadow ptrs
		LPGGEFFECT pEffectPtr = NULL;
		cSpecialEffect* pEffectObject = m_EffectList [ t.terrain.terrainshaderindex ]->pEffectObj;
		if ( pEffectObject )
			if ( pEffectObject->m_pEffect )
				pEffectPtr = pEffectObject->m_pEffect;
		if ( pEffectPtr )
		{
			g_CascadedShadow.m_depthHandle[0] = pEffectPtr->GetVariableByName( "DepthMapTX1" );
			g_CascadedShadow.m_depthHandle[1] = pEffectPtr->GetVariableByName( "DepthMapTX2" );
			g_CascadedShadow.m_depthHandle[2] = pEffectPtr->GetVariableByName( "DepthMapTX3" );
			g_CascadedShadow.m_depthHandle[3] = pEffectPtr->GetVariableByName( "DepthMapTX4" );
			g_CascadedShadow.m_depthHandle[4] = pEffectPtr->GetVariableByName( "DepthMapTX5" );
			g_CascadedShadow.m_depthHandle[5] = pEffectPtr->GetVariableByName( "DepthMapTX6" );
			g_CascadedShadow.m_depthHandle[6] = pEffectPtr->GetVariableByName( "DepthMapTX7" );
			g_CascadedShadow.m_depthHandle[7] = pEffectPtr->GetVariableByName( "DepthMapTX8" );
		}

		// Apply terrain shader
		SetObjectEffect ( t.terrain.terrainobjectindex, t.terrain.terrainshaderindex );

		// Apply textures for shader
		if ( g.gpbroverride == 0 )//t.terrain.iTerrainPBRMode == 0 )
		{
			// non-PBR
			TextureObject ( t.terrain.terrainobjectindex,0,t.terrain.imagestartindex+2 );
			TextureObject ( t.terrain.terrainobjectindex,1,g.postprocessimageoffset+5 );
			TextureObject ( t.terrain.terrainobjectindex,2,t.terrain.imagestartindex+13 );
			TextureObject ( t.terrain.terrainobjectindex,3,t.terrain.imagestartindex+17 );
			TextureObject ( t.terrain.terrainobjectindex,4,t.terrain.imagestartindex+21 );
		}
		else
		{
			// Get PBR support textures
			if ( ImageExist ( t.terrain.imagestartindex+14 ) == 0 ) LoadImage ( "effectbank\\reloaded\\media\\AGED.png",t.terrain.imagestartindex+14 );
			if ( ImageExist ( t.terrain.imagestartindex+15 ) == 0 ) LoadImage ( "effectbank\\reloaded\\media\\blank_black.dds",t.terrain.imagestartindex+15 );

			// PBR
			TextureObject ( t.terrain.terrainobjectindex, 0, t.terrain.imagestartindex+2 ); //VEGMAP RBG = Distant terrain texture,  A=circle for editor
			TextureObject ( t.terrain.terrainobjectindex, 1, t.terrain.imagestartindex+14 );//AGED map (ao, gloss, height, detail)
			TextureObject ( t.terrain.terrainobjectindex, 2, t.terrain.imagestartindex+13 );//Diffuse
			TextureObject ( t.terrain.terrainobjectindex, 3, t.terrain.imagestartindex+17 );//Highlighter
			TextureObject ( t.terrain.terrainobjectindex, 4, t.terrain.imagestartindex+21 );//Normal
			TextureObject ( t.terrain.terrainobjectindex, 5, t.terrain.imagestartindex+15 );//SpecularMap
			TextureObject ( t.terrain.terrainobjectindex, 6, t.terrain.imagestartindex+31 );//EnvironmentMap
			if (g.memskipibr == 0) TextureObject ( t.terrain.terrainobjectindex, 8, t.terrain.imagestartindex+32 );//GlossCurveMap
		}
	}
}

void terrain_createactualterrain ( void )
{
	// Load PBR env map (real CUBE moved to later when terrain loads in, for now use default)
	cstr texEnvMap = "effectbank\\reloaded\\media\\CUBE.dds";
	LoadImage ( texEnvMap.Get(), t.terrain.imagestartindex+31, 2 );

	//  Uses terrain.imagestartindex+3 for heightmap image
	if (  t.terrain.TerrainID != 0 ) 
	{
		BT_DeleteTerrain (  t.terrain.TerrainID );
		t.terrain.TerrainID=0;
	}
	if (  t.terrain.superflat == 0 ) 
	{
		t.terrain.TerrainID=BT_MakeTerrain();
		g_iTerrainIDForShadowMap = t.terrain.TerrainID;
		if (  FileExist(t.theightfile_s.Get()) == 1 ) 
		{
			SetMipmapNum(1); //PE: mipmaps not needed.
			LoadImage (  t.theightfile_s.Get(),t.terrain.imagestartindex+3 );
			SetMipmapNum(-1);
		}
		else
		{
			SetMipmapNum(1); //PE: mipmaps not needed.
			MakeImageJustFormat(t.terrain.imagestartindex + 3, 1024, 1024, GGFMT_A8R8G8B8);
//			LoadImage (  "effectbank\\reloaded\\media\\heightmap.dds",t.terrain.imagestartindex+3 );
			SetMipmapNum(-1);
		}
		BT_SetTerrainHeightmap (  t.terrain.TerrainID,t.terrain.imagestartindex+3 );
		BT_SetTerrainScale (  t.terrain.TerrainID,50 );
		BT_SetTerrainYScale (  t.terrain.TerrainID,8 );
		//BT_SetTerrainSplit (  t.terrain.TerrainID,32 ); // 220317 - for new terrain texture resolution - 16 );
		BT_SetTerrainSplit(t.terrain.TerrainID, 16); //PE: 180420 - GPU today, drawcalls is the big deal, this will decrease dc by half.
		BT_SetTerrainQuadRotation (  t.terrain.TerrainID,1 );
		BT_SetTerrainSmoothing (  t.terrain.TerrainID,0 );
		BT_SetTerrainDetailTile (  t.terrain.TerrainID,2 );
		BT_SetTerrainLOD (  t.terrain.TerrainID,3 );
		BT_SetTerrainLODDistance (  t.terrain.TerrainID,1,8000 );
		BT_SetTerrainLODDistance (  t.terrain.TerrainID,2,15000 );
		BT_BuildTerrain (  t.terrain.TerrainID,t.terrain.terrainobjectindex,1 );
		t.terrain.TerrainSize_f=BT_GetTerrainSize(t.terrain.TerrainID);
		SetTerrainRenderLevel (  100 );
	}
	else
	{
		//  Super Flat Terrain (for non landscape levels)
		if (  ObjectExist(t.terrain.terrainobjectindex) == 1  )  DeleteObject (  t.terrain.terrainobjectindex );
		if (  t.terrain.superflat == 2 ) 
		{
			//  no terrain object at all
		}
		else
		{
			MakeObjectPlane (  t.terrain.terrainobjectindex,512*100,512*100 );
			RotateObject (  t.terrain.terrainobjectindex,90,180,0 );
			PositionObject (  t.terrain.terrainobjectindex,256*100,1000,256*100 );
			SetObjectCull (  t.terrain.terrainobjectindex,0 );
			ScaleObject (  t.terrain.terrainobjectindex,100,100,-100 );
		}
		t.terrain.TerrainID=0;
	}

	//  mask set in visuals_loop (nope, now here so shadow not rendering this object)
	if (  ObjectExist(t.terrain.terrainobjectindex) == 1 ) 
	{
		SetObjectMask (  t.terrain.terrainobjectindex, 1 );
	}

	//  Apply effect and textures to terrain object
	terrain_applyshader();
}

void terrain_make ( void )
{
	// Terrain system
	t.terrain.terrainobjectindex=t.terrain.objectstartindex+3;
	if ( ObjectExist(t.terrain.terrainobjectindex) == 0 ) 
	{
		// Load terrain shaders (non-PBR and PBR) (applied later in terrain_applyshader)
		if ( GetEffectExist(t.terrain.effectstartindex+0) == 0 )
		{
			LPSTR pEffectToUse = "effectbank\\reloaded\\terrain_basic.fx";
			if ( g.gpbroverride == 1 ) pEffectToUse = "effectbank\\reloaded\\apbr_terrain.fx";
			LoadEffect ( pEffectToUse, t.terrain.effectstartindex+0, 0 );
			timestampactivity(0, cstr(cstr("Terrain Shader:")+pEffectToUse).Get() );
		}
		terrain_assignnewshader();

		// IBR curve loopup map as globals
		if (g.memskipibr == 0) 
		{
			cstr texIBRMap = "effectbank\\reloaded\\media\\IBR.png";
			LoadImage(texIBRMap.Get(), t.terrain.imagestartindex + 32);
		}

		// Prepare shader as a shadow mapping primary effect (updated also in terrain_applyshader function if iTerrainPBRMode changes)
		SetEffectShadowMappingMode ( 255 );
		SetEffectToShadowMappingEx ( t.terrain.terrainshaderindex, g.shadowdebugobjectoffset, g.guidepthshadereffectindex, g.globals.hidedistantshadows, 0, g.globals.realshadowresolution, g.globals.realshadowcascadecount, g.globals.realshadowcascade[0], g.globals.realshadowcascade[1], g.globals.realshadowcascade[2], g.globals.realshadowcascade[3], g.globals.realshadowcascade[4], g.globals.realshadowcascade[5], g.globals.realshadowcascade[6], g.globals.realshadowcascade[7] );

		//  Load all terrain textures
		if ( t.game.runasmultiplayer == 1 ) mp_refresh ( );
		if (  t.terrain.superflat == 2 ) 
		{
			//  No terrain to texture
		}
		else
		{
			cstr VegMask_s = cstr(g.mysystem.levelBankTestMap_s + "vegmask.png");
			if ( FileExist(VegMask_s.Get()) == 0 ) VegMask_s = cstr(g.mysystem.levelBankTestMap_s + "vegmask.dds");
			if ( FileExist(VegMask_s.Get()) == 1 )
			{
				if ( ImageExist(t.terrain.imagestartindex+2) == 0 ) 
				{
					SetMipmapNum(1);
					LoadImage ( VegMask_s.Get(), t.terrain.imagestartindex+2, 10, 0 );
					if ( ImageExist ( t.terrain.imagestartindex+2 ) == 0 )
					{
						terrain_generatevegandmask_grab ( );
					}
					SetMipmapNum(-1);
					if ( MemblockExist(123) == 1 ) DeleteMemblock ( 123 );
					CreateMemblockFromImage ( 123, t.terrain.imagestartindex+2 );
				}
			}
			else
			{
				terrain_generatevegandmask_grab ( );
			}

			if ( t.game.runasmultiplayer == 1 ) mp_refresh ( );

			if (  g.gdividetexturesize == 0 ) 
			{
				t.tthistexdir_s="effectbank\\reloaded\\media\\white_D.dds";
				LoadImage ( t.tthistexdir_s.Get(), t.terrain.imagestartindex+13, 0, g.gdividetexturesize );
				LoadImage ( t.tthistexdir_s.Get(), t.terrain.imagestartindex+21, 0, g.gdividetexturesize );
			}
			else
			{
				t.tsplashstatusprogress_s="LOADING TERRAIN DIFFUSE";
				timestampactivity(0,t.tsplashstatusprogress_s.Get());
				version_splashtext_statusupdate ( );
				if ( FileExist ( cstr(cstr("terrainbank\\")+g.terrainstyle_s+"\\"+TEXTURE_D_NAME).Get() ) == 1 )
					LoadImage ( cstr(cstr("terrainbank\\")+g.terrainstyle_s+"\\"+TEXTURE_D_NAME).Get(),t.terrain.imagestartindex+13,0,g.gdividetexturesize );
				else
					LoadImage ( cstr(cstr("terrainbank\\")+g.terrainstyle_s+"\\texture_D.dds").Get(),t.terrain.imagestartindex+13,0,g.gdividetexturesize );

				 LoadImage ( "effectbank\\reloaded\\media\\blank_N.dds", t.terrain.imagestartindex+21, 0, g.gdividetexturesize );
			}
			if ( t.game.runasmultiplayer == 1 ) mp_refresh ( );

			// This texture acts as highlight graphic and also store for mega texture (distant terrain texture composite)
			SetImageAutoMipMap (  0 ); // PE: SetImageAutoMipMap Dont work anymore.
			SetMipmapNum(1); //PE: mipmaps not needed.
			LoadImage("effectbank\\reloaded\\media\\circle2.dds", t.terrain.imagestartindex + 17, 10, 0);
			SetMipmapNum(-1);
			SetImageAutoMipMap (  1 );
		}
		if ( t.game.runasmultiplayer == 1 ) mp_refresh ( );

		//  Water handling vars
		t.terrain.WaterCamY_f=0.0;
	}

	//  By default, always select NEAR technique
	BT_ForceTerrainTechnique ( 1 );

	// 100417 - when terrain created, reset water line for new terrain
	t.terrain.waterliney_f = g.gdefaultwaterheight;
}

void terrain_make_image_only(void)
{
	// Terrain system
	t.terrain.terrainobjectindex = t.terrain.objectstartindex + 3;

	// IBR curve loopup map as globals
	if (g.memskipibr == 0)
	{
		cstr texIBRMap = "effectbank\\reloaded\\media\\IBR.png";
		LoadImage(texIBRMap.Get(), t.terrain.imagestartindex + 32);
	}

	//  Load all terrain textures
	if (t.game.runasmultiplayer == 1) mp_refresh();
	if (t.terrain.superflat == 2)
	{
		//  No terrain to texture
	}
	else
	{
		cstr VegMask_s = cstr(g.mysystem.levelBankTestMap_s + "vegmask.png");
		if ( FileExist(VegMask_s.Get()) == 0 ) VegMask_s = cstr(g.mysystem.levelBankTestMap_s + "vegmask.dds");
		if ( FileExist(VegMask_s.Get()) == 1 )
		{
			if (ImageExist(t.terrain.imagestartindex + 2) == 0)
			{
				SetMipmapNum(1);
				LoadImage(VegMask_s.Get(), t.terrain.imagestartindex + 2, 10, 0);
				if (ImageExist(t.terrain.imagestartindex + 2) == 0)
				{
					terrain_generatevegandmask_grab();
				}
				SetMipmapNum(-1);
				if (MemblockExist(123) == 1) DeleteMemblock(123);
				CreateMemblockFromImage(123, t.terrain.imagestartindex + 2);
			}
		}
		else
		{
			terrain_generatevegandmask_grab();
		}

		if (t.game.runasmultiplayer == 1) mp_refresh();

		if (g.gdividetexturesize == 0)
		{
			t.tthistexdir_s = "effectbank\\reloaded\\media\\white_D.dds";
			LoadImage(t.tthistexdir_s.Get(), t.terrain.imagestartindex + 13, 0, g.gdividetexturesize);
			LoadImage(t.tthistexdir_s.Get(), t.terrain.imagestartindex + 21, 0, g.gdividetexturesize);
		}
		else
		{
			t.tsplashstatusprogress_s = "LOADING TERRAIN DIFFUSE";
			timestampactivity(0, t.tsplashstatusprogress_s.Get());
			version_splashtext_statusupdate();
			if (FileExist(cstr(cstr("terrainbank\\") + g.terrainstyle_s + "\\" + TEXTURE_D_NAME).Get()) == 1)
				LoadImage(cstr(cstr("terrainbank\\") + g.terrainstyle_s + "\\" + TEXTURE_D_NAME).Get(), t.terrain.imagestartindex + 13, 0, g.gdividetexturesize);
			else
				LoadImage(cstr(cstr("terrainbank\\") + g.terrainstyle_s + "\\texture_D.dds").Get(), t.terrain.imagestartindex + 13, 0, g.gdividetexturesize);

			LoadImage("effectbank\\reloaded\\media\\blank_N.dds", t.terrain.imagestartindex + 21, 0, g.gdividetexturesize);
		}
		if (t.game.runasmultiplayer == 1) mp_refresh();

		// This texture acts as highlight graphic and also store for mega texture (distant terrain texture composite)
		SetImageAutoMipMap(0); // PE: SetImageAutoMipMap Dont work anymore.
		SetMipmapNum(1); //PE: mipmaps not needed.
//			LoadImage (  "effectbank\\reloaded\\media\\circle.dds",t.terrain.imagestartindex+17,10,0 );
		LoadImage("effectbank\\reloaded\\media\\circle2.dds", t.terrain.imagestartindex + 17, 10, 0);
		SetMipmapNum(-1);
		SetImageAutoMipMap(1);

	}
}

void terrain_load ( void )
{
	if ( FileExist(t.tfile_s.Get()) == 1 && t.terrain.TerrainID > 0 ) 
	{
		OpenToRead ( 1, t.tfile_s.Get() );
		if ( MemblockExist(1) == 0 ) 
		{
			ReadMemblock ( 1, 1 );
			t.mi = 0;
			for ( t.z = 0; t.z <= 1023; t.z++ )
			{
				for ( t.x = 0 ; t.x <= 1023; t.x++ )
				{
					t.h_f = ReadMemblockFloat(1,t.mi);
					t.terrainmatrix[t.x][t.z] = t.h_f;
					t.mi += 4;
					BT_SetPointHeight ( t.terrain.TerrainID, t.x, t.z, t.h_f );
				}
			}
			DeleteMemblock (  1 );

			// force a shore on all terrain imported
			t.x = 0; for ( t.z = 0 ; t.z <= 1023 ; t.z++ ) BT_SetPointHeight ( t.terrain.TerrainID, t.x, t.z, 0.0  );
			t.x = 1023; for ( t.z = 0 ; t.z<= 1023 ; t.z++ ) BT_SetPointHeight ( t.terrain.TerrainID, t.x, t.z, 0.0  );
			t.z = 0 ; for ( t.x = 0 ; t.x <= 1023 ; t.x++ ) BT_SetPointHeight ( t.terrain.TerrainID, t.x, t.z, 0.0  );
			t.z = 1023; for ( t.x = 0 ; t.x <= 1023 ; t.x++ ) BT_SetPointHeight ( t.terrain.TerrainID, t.x, t.z, 0.0  );
		}
		else
		{
			ExitPrompt (  "Memblock 1 already exists!","Terrain"  ); ExitProcess ( 0 );
		}
		CloseFile (  1 );
	}
}

void terrain_save ( void )
{
	if (  t.terrain.TerrainID>0 ) 
	{
		if ( FileExist(t.tfile_s.Get()) == 1 ) DeleteAFile ( t.tfile_s.Get() );
		if ( MemblockExist(1) == 0 ) 
		{
			MakeMemblock ( 1,1024*1024*4 );
			if ( OpenToWriteEx ( 1, t.tfile_s.Get() ) == true )
			{
				t.mi=0;
				for ( t.z = 0 ; t.z<=  1023; t.z++ )
				{
					for ( t.x = 0 ; t.x<=  1023; t.x++ )
					{
						t.h_f=BT_GetGroundHeight(t.terrain.TerrainID,t.x*50.0,t.z*50.0,1);
						if (  t.h_f<0.0  )  t.h_f = 0.0;
						WriteMemblockFloat (  1,t.mi,t.h_f );
						t.mi += 4;
					}
				}
				WriteMemblock ( 1,1 );
				CloseFile ( 1 );
			}
			DeleteMemblock ( 1 );
		}
		else
		{
			ExitPrompt (  "Memblock 1 already exists!","Terrain"  ); ExitProcess ( 0 );
		}
	}
}

void terrain_savetextures ( void )
{
	// save veg map texture
	if ( FileExist(t.tfileveg_s.Get()) == 1 ) DeleteAFile ( t.tfileveg_s.Get() );
	if ( ImageExist(t.terrain.imagestartindex+2) == 1 ) 
	{
		SaveImage ( t.tfileveg_s.Get(),t.terrain.imagestartindex+2 );
	}
	SetCurrentBitmap ( 0 );

	// save water mask texture
	if ( t.terrain.terrainregionx1 != t.terrain.terrainregionx2 || FileExist(t.tfilewater_s.Get()) == 0 ) 
	{
		t.terrain.terrainregionupdate=2;
		terrain_refreshterrainmatrix ( );
		terrain_updatewatermask ( );
	}
}

void terrain_generatevegandmask_grab ( void )
{
	SetCurrentBitmap ( g.terrainworkbitmapindex );
	CLS ( 0, 0, 64 );
	GrabImage ( t.terrain.imagestartindex+2,0,0,MAXTEXTURESIZE,MAXTEXTURESIZE );
	SetCurrentBitmap ( 0 );
}

void terrain_generatevegandmaskfromterrain ( void )
{
	// newly generated terrain needs updated veg map and water mask
	if ( FileExist(t.tfileveg_s.Get()) == 1 ) DeleteAFile ( t.tfileveg_s.Get() );
	terrain_generatevegandmask_grab ( );
	timestampactivity(0, cstr(cstr("Deleting old memblock:")+Str(MemblockExist(123))).Get() );
	if ( MemblockExist(123) == 1  ) DeleteMemblock ( 123 );
	CreateMemblockFromImage ( 123, t.terrain.imagestartindex+2 );

	// update water mask
	t.terrain.terrainregionupdate=2;
	terrain_refreshterrainmatrix ( );
	if (  t.tgeneratefreshwatermaskflag == 1 ) 
	{
		terrain_updatewatermask_new ( );
		t.tgeneratefreshwatermaskflag=0;
	}
	else
	{
		terrain_updatewatermask ( );
	}

	// trigger vegmap image to be pasted to paint camera
	if ( ObjectExist(t.terrain.terrainobjectindex)  ==  1 ) 
	{
		TextureObject ( t.terrain.terrainobjectindex, 0, t.terrain.imagestartindex+2 );
	}
}

void terrain_generateblanktextures ( void )
{
	//  blank veg
	if (  FileExist(t.tfileveg_s.Get()) == 1  )  DeleteAFile (  t.tfileveg_s.Get() );
	SetCurrentBitmap (  g.terrainworkbitmapindex );
	CLS ( 0, 0, 64 );
	GrabImage (  t.terrain.imagestartindex+2,0,0,MAXTEXTURESIZE,MAXTEXTURESIZE );
	SaveImage (  t.tfileveg_s.Get(),t.terrain.imagestartindex+2 );
	if (  MemblockExist(123) == 1  )  DeleteMemblock (  123 );
	CreateMemblockFromImage (  123,t.terrain.imagestartindex+2 );

	//  blank water mask
	if (  FileExist(t.tfilewater_s.Get()) == 1  )  DeleteAFile (  t.tfilewater_s.Get() );
	CLS (  Rgb(0,0,0) );
	if (g.memskipwatermask == 1) {
		GrabImage(t.terrain.imagestartindex + 4, 0, 0, 1, 1);
	}
	else {
		GrabImage(t.terrain.imagestartindex + 4, 0, 0, MAXTEXTURESIZE, MAXTEXTURESIZE);
	}
	SaveImage (  t.tfilewater_s.Get(),t.terrain.imagestartindex+4 );
	SetCurrentBitmap (  0 );
}

void terrain_loaddata ( void )
{
	// load new terrain into engine
	if ( t.terrain.superflat != 2 )
	{
		t.filename_s=t.levelmapptah_s+"m.dat";
		if ( FileExist(t.filename_s.Get()) == 1 ) 
		{
			// Load terrain height data
			t.tfile_s=t.filename_s ; terrain_load ( );

			// Load veg shadow map
			cstr VegMask_s = cstr(t.levelmapptah_s + "vegmask.png");
			if ( FileExist(VegMask_s.Get()) == 0 ) VegMask_s = cstr(t.levelmapptah_s + "vegmask.dds");
			if ( FileExist(VegMask_s.Get()) == 1 )
			{
				if ( ImageExist(t.terrain.imagestartindex+2) == 1 ) DeleteImage ( t.terrain.imagestartindex+2 );
				SetMipmapNum(1);
				LoadImage ( VegMask_s.Get(), t.terrain.imagestartindex+2, 10, 0 );
				SetMipmapNum(-1);
				if ( MemblockExist(123) == 1 ) DeleteMemblock ( 123 );
				CreateMemblockFromImage ( 123,t.terrain.imagestartindex+2 );
				TextureObject ( t.terrain.terrainobjectindex,0,t.terrain.imagestartindex+2 );
			}

			// Load water mask
			cstr WaterMask_s = cstr(t.levelmapptah_s + "watermask.png").Get();
			if ( FileExist(WaterMask_s.Get()) == 0 ) WaterMask_s = cstr(t.levelmapptah_s + "watermask.dds").Get();
			if ( g.memskipwatermask == 0 && FileExist( WaterMask_s.Get() ) == 1 )
			{
				if ( ImageExist(t.terrain.imagestartindex+4) == 1 ) DeleteImage ( t.terrain.imagestartindex+4 );
				SetMipmapNum(1);
				LoadImage ( WaterMask_s.Get(), t.terrain.imagestartindex+4, 10, 0, 1 );
				SetMipmapNum(-1);
			}
			if (ImageExist(t.terrain.imagestartindex + 4) == 0)
			{
				GrabImage(t.terrain.imagestartindex + 4, 0, 0, 1, 1);
			}

			// Load veg grass memblock
			if ( FileExist( cstr(t.levelmapptah_s+"vegmaskgrass.dat").Get() ) == 1 ) 
			{
				t.tfileveggrass_s=t.levelmapptah_s+"vegmaskgrass.dat";
				grass_loadgrass ( );
			}

			// Load any custom terrain texture if present
			//PE: Only if CUSTOM is selected in tab tab. https://github.com/TheGameCreators/GameGuruRepo/issues/641
			#ifdef ENABLECUSTOMTERRAIN
			if (g.terrainstyleindex == 1 && (FileExist( cstr(t.levelmapptah_s+TEXTURE_D_NAME).Get() ) == 1
			||   FileExist( cstr(t.levelmapptah_s+"texture_D.dds").Get() ) == 1) ) 
			{
				// custom texture in FPM overrides one specified in visual
				g.terrainstyleindex = 1;
			}
			else
			#endif
			{
				// find terrainstyle index specified in visual
				g.terrainstyle_s = t.visuals.terrain_s;
				for ( g.terrainstyleindex = 1 ; g.terrainstyleindex <= g.terrainstylemax; g.terrainstyleindex++ )
					if ( cstr(Lower(g.terrainstyle_s.Get())) == t.terrainstylebank_s[g.terrainstyleindex] ) 
						break;
				// 080517 - if exceed array (i.e not found) reset to last slot
				if ( g.terrainstyleindex > g.terrainstylemax ) 
				{
					g.terrainstyleindex = g.terrainstylemax;
					g.terrainstyle_s = t.terrainstylebank_s[g.terrainstyleindex];
				}
			}
			terrain_loadlatesttexture ( );

			// 251017 - moved from actualterrain creation (too soon before)
			// Load PBR env map - if no local CUBE file, load the global cube file if a PBR
			cstr texEnvMap = g.mysystem.levelBankTestMap_s+"globalenvmap.dds"; //"levelbank\\testmap\\globalenvmap.dds";
			if ( FileExist ( texEnvMap.Get() ) == 0 ) texEnvMap = "effectbank\\reloaded\\media\\CUBE.dds";
			LoadImage ( texEnvMap.Get(), t.terrain.imagestartindex+31, 2 );
			TextureObject ( t.terrain.terrainobjectindex, 6, t.terrain.imagestartindex+31 );

			// generate super texture from above existing texture
			terrain_generatesupertexture ( false );
		}
	}
}

void terrain_delete ( void )
{
	if (  t.terrain.TerrainID>0 ) 
	{
		BT_DeleteTerrain (  t.terrain.TerrainID );
		t.terrain.TerrainID=0;
	}
	if (  t.terrain.terrainshaderindex>0 ) 
	{
		if (  GetEffectExist(t.terrain.terrainshaderindex) == 1  )  DeleteEffect (  t.terrain.terrainshaderindex );
	}
}

void dynamic_sun_position(int effectid)
{
#ifdef DYNAMICSUNPOSITION
	DARKSDK_DLL void SetEffectConstantV(int iEffectID, LPSTR pConstantName, int iVector);
	SetVector4(g.terrainvectorindex, t.terrain.sundirectionx_f, t.terrain.sundirectiony_f, t.terrain.sundirectionz_f, 0.0);
	SetEffectConstantV(effectid, "LightSource", g.terrainvectorindex);
#endif
}

