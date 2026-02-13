void terrain_shadowupdate ( void )
{
	// Shadow Mapping ; Activate cascade shadow mapping for terrain
	if ( t.visuals.shadowmode>0 ) 
	{
		//PE: Still Z negative when using g.luacameraoverride=3 || 2
		float oldcamrx, oldcamry, oldcamrz;
		if (t.aisystem.processplayerlogic != 1 && (g.luacameraoverride == 2 || g.luacameraoverride == 3) ) 
		{
			if (t.freezeplayerposonly == 0) 
			{
				oldcamrx = CameraAngleX(0);
				oldcamry = CameraAngleY(0);
				oldcamrz = CameraAngleZ(0);
				RotateCamera(0, CameraAngleX(0), CameraAngleY(0), -CameraAngleZ(0));
			}
		}
		if ( 1 ) 
		{
			#ifdef DYNAMICSUNPOSITION
			if (1)
			{
				#ifdef DISPLAYDEBUGSUN
				float sunsize = 1500.0;
				//Rotate light , for shadow testing.
				int sunobj = g.shadowdebugobjectoffset + 5;
				if (ObjectExist(sunobj) == 0 ) {
					MakeObjectSphere(sunobj, sunsize,32,32); //Larger sun
					SetObjectCollisionOff(sunobj);
					SetAlphaMappingOn(sunobj, 255);
					DisableObjectZRead(sunobj);
					SetObjectMask(sunobj, 1);
					SetObjectEffect(sunobj, g.guishadereffectindex);
					SetObjectEmissive(sunobj, Rgb(255, 255, 255));
					SetObjectMask(sunobj, 1);
					ShowObject(sunobj);
				}
			
				g.globals.speedshadows = 0; //disable speedshadows.

				static float rotatecound = 0;
				rotatecound = rotatecound + (0.5*t.ElapsedTime_f);
				if (rotatecound > 360.0) rotatecound = rotatecound - 360.0;
				static float rotatecoundy = 0;
				rotatecoundy = rotatecoundy + (1.25*t.ElapsedTime_f);
				if (rotatecoundy > 360.0) rotatecoundy = rotatecoundy - 360.0;

				t.x_f = (sin(rotatecound)*1000.0);
				t.y_f = (sin(rotatecoundy)*250.0)+500.0; //500-1000
				t.z_f = (cos(rotatecound)*1000.0);
				PositionLight(0, t.x_f , t.y_f , t.z_f );
				PointLight(0, 0.0, 0.0, 0.0); //direction always to zero.

				t.terrain.sundirectionx_f = t.x_f;
				t.terrain.sundirectiony_f = t.y_f+ t.terrain.waterliney_f;
				t.terrain.sundirectionz_f = t.z_f;
				if (ObjectExist(sunobj) == 1) {
					PositionObject(sunobj, (t.x_f*15.0) + CameraPositionX(0), (t.y_f*15.0)+ t.terrain.waterliney_f+ sunsize, (t.z_f*15.0) + CameraPositionZ(0));
					ShowObject(sunobj);
				}
				#else
				//We need to make sunposition match shadow direction. (125,100,100) = 5000,10000,5000.
				PositionLight(0, t.terrain.sundirectionx_f*0.75, t.terrain.sundirectiony_f*2.0, t.terrain.sundirectionz_f);
				PointLight(0, 0.0, 0.0, 0.0); //direction match old sun pos.
				#endif
			}
			#else
			//  place shadowlight
			t.x_f=0 ; t.y_f=1000 ; t.z_f=0;
			PositionLight ( 0, t.x_f-5000, t.y_f+10000, t.z_f-5000 ); // team decided sun default should be behind/left of forward facing direction
			PointLight ( 0, t.x_f, t.y_f, t.z_f );
			#endif
			//  hide any jetpack
			//  NOTE; This should be MASK based, not hidden!
			if (  t.playercontrol.jetobjtouse>0 ) 
			{
				if (  ObjectExist(t.playercontrol.jetobjtouse) == 1 ) 
				{
					t.tjet1v=GetVisible(t.playercontrol.jetobjtouse);
					HideObject (  t.playercontrol.jetobjtouse );
				}
			}

			//  hide skybox before rendering cascade
			if (  ObjectExist(t.terrain.objectstartindex+4) == 1 ) 
			{
				t.tskyobj1v=GetVisible(t.terrain.objectstartindex+4);
				HideObject (  t.terrain.objectstartindex+4 );
			}
			if (  ObjectExist(t.terrain.objectstartindex+8) == 1 ) 
			{
				t.tskyobj2v=GetVisible(t.terrain.objectstartindex+8);
				HideObject (  t.terrain.objectstartindex+8 );
			}
			if (  ObjectExist(t.terrain.objectstartindex+9) == 1 ) 
			{
				t.tskyobj3v=GetVisible(t.terrain.objectstartindex+9);
				HideObject (  t.terrain.objectstartindex+9 );
			}

			//  hide editor water
			if (  ObjectExist(t.terrain.objectstartindex+2) == 1 ) 
			{
				t.twaterobj=GetVisible(t.terrain.objectstartindex+2);
				HideObject (  t.terrain.objectstartindex+2 );
			}

			//  hide superflat terrain
			if (  t.terrain.TerrainID == 0 ) 
			{
				if (  ObjectExist(t.terrain.terrainobjectindex) == 1 ) 
				{
					HideObject (  t.terrain.terrainobjectindex );
				}
			}

			//  hide all vegetation
			if (  t.hardwareinfoglobals.nograss == 0 ) 
			{
				HideVegetationGrid (  );
			}

			//  shadows for map editor and game
			if ( 1 ) 
			{
				//  prepare scene shaders to render into the cascade shadow textures
				for ( t.t = -6 ; t.t<=  g.effectbankmax; t.t++ )
				{
					if ( t.t == -6  )  t.effectid = g.lightmappbreffectillum;
					if ( t.t == -5  ) t.effectid = g.controllerpbreffect;
					if ( t.t == -4  )  t.effectid = g.lightmappbreffect;
					if ( t.t == -3  )  t.effectid = g.thirdpersonentityeffect;
					if ( t.t == -2  )  t.effectid = g.thirdpersoncharactereffect;
					if ( t.t == -1  )  t.effectid = g.staticlightmapeffectoffset;
					if ( t.t == 0  )  t.effectid = g.staticshadowlightmapeffectoffset;
					if ( t.t>0  )  t.effectid = g.effectbankoffset+t.t;
					if ( t.effectid>0 ) 
					{
						if ( GetEffectExist(t.effectid) == 1 )  SetEffectTechnique ( t.effectid,"DepthMap" );
						//PE: This produce non transparent shadows on PBR objects in mode g.gpbroverride == 0
						//if ( g.gpbroverride == 0 )
						//{
						//	if ( t.visuals.shaderlevels.terrain >= 3 && t.visuals.shaderlevels.lighting != 1 ) 
						//	{
						//		// if this special technique does not exist in shaders, no new technique is used
						//		if ( GetEffectExist(t.effectid) == 1 ) SetEffectTechnique ( t.effectid,"DepthMapNoAnim" );
						//	}
						//}
					}
				}

				//  set static entity shader to depthmap technique also
				if (  GetEffectExist(g.staticlightmapeffectoffset) == 1  )  SetEffectTechnique (  g.staticlightmapeffectoffset,"DepthMap" );

				//  control how many cascade shadows to render based on shader levels for terrain shader
				//t.tonlyusingcheapestcascade=0;
				t.game.set.shaderrequirecheapshadow=0;
				SetEffectShadowMappingMode ( 0 );

				//PE: If we can set m_fCascadeFrustumsEyeSpaceDepths here (0) if not used,
				//PE: then all shaders can use GetShadow highest/medium and we can control what cascades is available here.
				//PE: current: old terrain medium/low = cascade 7 , veg high = cascade 3 , nonpbrentity medium = none , pbr entity medium = all.

				if ( 1 ) // g.gpbroverride == 1
				{
					// PBR default mode is to render all shadows in editor mode

					//PE: Set frustum percent so we can fake a medium.

					g_CascadedShadow.m_iCascadePartitionsZeroToOne[0] = g.globals.realshadowcascade[0];
					g_CascadedShadow.m_iCascadePartitionsZeroToOne[1] = g.globals.realshadowcascade[1];
					g_CascadedShadow.m_iCascadePartitionsZeroToOne[2] = g.globals.realshadowcascade[2];
					g_CascadedShadow.m_iCascadePartitionsZeroToOne[3] = g.globals.realshadowcascade[3];
					g_CascadedShadow.m_iCascadePartitionsZeroToOne[4] = g.globals.realshadowcascade[4];
					g_CascadedShadow.m_iCascadePartitionsZeroToOne[5] = g.globals.realshadowcascade[5];
					g_CascadedShadow.m_iCascadePartitionsZeroToOne[6] = g.globals.realshadowcascade[6];
					g_CascadedShadow.m_iCascadePartitionsZeroToOne[7] = g.globals.realshadowcascade[7];
					g.globals.realshadowdistance = g.globals.realshadowdistancehigh;
					SetShadowTexelSize(g.globals.realshadowresolution);
					SetEffectShadowMappingMode ( 255 );

					static int speed_shadows = 0;
					static int speed_shadows_new = 0;
					speed_shadows = 1 - speed_shadows;
					//PE: PBR both terrain and entity must be set to low/medium before lowering cascades.
					//PE: Editor always use medium.
					if (t.game.set.ismapeditormode != 1 && t.visuals.shaderlevels.terrain >= 4 && t.visuals.shaderlevels.entities >= 3) 
					{
						//PE: Lowest disable shadows.
						SetEffectShadowMappingMode(0);
					}
					else if ( (g.globals.editorusemediumshadows == 1 && t.game.set.ismapeditormode == 1 ) || (t.game.set.ismapeditormode != 1 && t.visuals.shaderlevels.terrain >= 2 && t.visuals.shaderlevels.entities >= 2 ) && t.playercontrol.thirdperson.enabled == 0 ) 
					{
						for (int icl = 0; icl < g.globals.realshadowcascadecount; icl++)
							g_CascadedShadow.m_iCascadePartitionsZeroToOne[icl] = 0;

						SetEffectShadowMappingMode( (1<< g.globals.realshadowcascadecount-1) + (1<< (g.globals.realshadowcascadecount-2)) );
						g.globals.realshadowdistance = g.globals.realshadowdistancehigh*0.6; //PE: Lower distance by 40%.
						//SetShadowTexelSize(2048); // Needed when we use so low a resolution.
						g_CascadedShadow.m_iCascadePartitionsZeroToOne[g.globals.realshadowcascadecount - 2] = 25;
						g_CascadedShadow.m_iCascadePartitionsZeroToOne[g.globals.realshadowcascadecount - 1] = 100;

						if (g.globals.speedshadows != 0)
						{
							if (speed_shadows) {
								SetEffectShadowMappingMode((1 << g.globals.realshadowcascadecount - 1));
							}
							else {
								SetEffectShadowMappingMode((1 << g.globals.realshadowcascadecount - 2));
							}
						}
					}
					else {
						//Full shadows.
						if (g.globals.speedshadows != 0)
						{
							if (g.globals.realshadowcascadecount == 4) {
								if (g.globals.speedshadows == 2) {
									if (speed_shadows_new == 0) {
										SetEffectShadowMappingMode(1 + 2);
									}
									else if (speed_shadows_new == 1) {
										SetEffectShadowMappingMode(1 + 4);
									}
									else if (speed_shadows_new == 2) {
										SetEffectShadowMappingMode(1 + 2);
									}
									else if (speed_shadows_new == 3) {
										SetEffectShadowMappingMode(1 + 8);
									}
									speed_shadows_new++;
									if (speed_shadows_new >= 4)
										speed_shadows_new = 0;
								}
								else {
									if (speed_shadows) {
										SetEffectShadowMappingMode(1 + 4);
									}
									else {
										SetEffectShadowMappingMode(2 + 8);
									}
								}
							}
						}
					}
				}
				else
				{
					if (  t.visuals.shaderlevels.lighting == 1 && t.game.set.ismapeditormode == 0 ) 
					{
						//  PREBAKE requires second cascade for near-shadows (dynamic objects in test/game)
						if (  t.visuals.shaderlevels.entities == 1 ) 
						{
							SetEffectShadowMappingMode ( 255 ); //%11111111 //15 ); //%1111
						}
						else
						{
							//  uses cascade 2 and 4 (near and far)
							SetEffectShadowMappingMode ( 10 ); //%1010
						}
					}
					else
					{
						if (  t.visuals.shaderlevels.terrain <= 2 || t.visuals.shaderlevels.entities == 1 ) 
						{
							//  for HIGH/HIGHEST shaders that require all FOUR shadow cascades
							SetEffectShadowMappingMode ( 255 ); // %11111111 15 ); //%1111
						}
						else
						{
							//  for LOW/LOWEST only DISTANT CASCADE is used
							//SetEffectShadowMappingMode ( 8 ); //%00001000 //%1000
							SetEffectShadowMappingMode ( 128 ); //%10000000 (changed terrain_basic to use cascade 7) //120418 - but keep cascade 4 for PBR shaders
						}
					}
				}

				//  render primary cascade (terrainshaderindex)
				if (  t.terrain.terrainshaderindex>0 ) 
				{
					// primary shader renders actual cascade shadow RTs
					RenderEffectShadowMapping ( t.terrain.terrainshaderindex );
				}

				//  restore all scene shaders after render to cascade shadow textures
				visuals_shaderlevels_entities_update ( );

				// give shaders that use shadows the latest-realtime values
				if ( t.terrain.vegetationshaderindex>0 ) 
				{
					// veg - secondary shader simply conveys required shadow constants to shader
					if ( GetEffectExist(t.terrain.vegetationshaderindex) == 1 ) 
					{
						RenderEffectShadowMapping ( t.terrain.vegetationshaderindex );
					}
				}
				if (  t.gunid>0 && t.gun[t.gunid].effectidused>0 ) 
				{
					//  gun - secondary shader simply conveys required shadow constants to shader
					RenderEffectShadowMapping (  t.gun[t.gunid].effectidused );
				}
				for ( t.t = -6 ; t.t<=  g.effectbankmax; t.t++ )
				{
					if (  t.t == -6  )  t.effectid = g.lightmappbreffectillum;
					if (  t.t == -5  ) t.effectid = g.controllerpbreffect;
					if (  t.t == -4  )  t.effectid = g.lightmappbreffect;
					if (  t.t == -3  )  t.effectid = g.thirdpersonentityeffect;
					if (  t.t == -2  )  t.effectid = g.thirdpersoncharactereffect;
					if (  t.t == -1  )  t.effectid = g.staticlightmapeffectoffset;
					if (  t.t == 0  )  t.effectid = g.staticshadowlightmapeffectoffset;
					if (  t.t>0  )  t.effectid = g.effectbankoffset+t.t;
					if (  GetEffectExist(t.effectid) == 1 ) 
					{
						//  entities - secondary shader simply conveys required shadow constants to shader
						RenderEffectShadowMapping (  t.effectid );
					}
				}
			}

			//  show all vegetation, but only if in game (we don't show veg in grid edit mode)
			if (  t.hardwareinfoglobals.nograss == 0 ) 
			{
				if (  t.game.gameloop  !=  0 || bEnableVeg )  ShowVegetationGrid (  );
			}

			//  show water in editor
			if (  ObjectExist(t.terrain.objectstartindex+2) == 1 && t.twaterobj == 1  )  ShowObject (  t.terrain.objectstartindex+2 );

			//  show superflat terrain
			if (  t.terrain.TerrainID == 0 ) 
			{
				if (  ObjectExist(t.terrain.terrainobjectindex) == 1 ) 
				{
					ShowObject (  t.terrain.terrainobjectindex );
				}
			}

			//  show sky Box (  after rendering shadow cascades )
			if (  t.hardwareinfoglobals.nosky == 0 ) 
			{
				if (  ObjectExist(t.terrain.objectstartindex+4) == 1 && t.tskyobj1v == 1  )  ShowObject (  t.terrain.objectstartindex+4 );
				if (  ObjectExist(t.terrain.objectstartindex+8) == 1 && t.tskyobj2v == 1  )  ShowObject (  t.terrain.objectstartindex+8 );
				if (  ObjectExist(t.terrain.objectstartindex+9) == 1 && t.tskyobj3v == 1  )  ShowObject (  t.terrain.objectstartindex+9 );
			}

			//  show any jetpack
			if (  t.playercontrol.jetobjtouse>0 ) 
			{
				if (  ObjectExist(t.playercontrol.jetobjtouse) == 1 && t.tskyobj1v == 1  )  ShowObject (  t.playercontrol.jetobjtouse );
			}

			// shadow debug when required
			if ( t.visuals.debugvisualsmode == 1 ) 
			{
				// show debug shadow map (first cascade)
				if ( g.shadowdebugobjectoffset+0 > 0 )
				{
					if ( ObjectExist ( g.shadowdebugobjectoffset+0 ) == 1 )
					{
						if ( ReturnKey() == 1 ) 
							ShowObject ( g.shadowdebugobjectoffset+0 );
						else
							HideObject ( g.shadowdebugobjectoffset+0 );
					}
				}
			}
		}
		if (t.aisystem.processplayerlogic != 1 && (g.luacameraoverride == 2 || g.luacameraoverride == 3)) 
		{
			if (t.freezeplayerposonly == 0) 
			{
				RotateCamera(0, oldcamrx, oldcamry, oldcamrz);
			}
		}
	}

	//  occlusion debug when required (put near shadow debug so can find easier)
	if ( t.visuals.debugvisualsmode == 2 ) 
	{
		if (  ReturnKey() == 1 ) 
		{
			CPU3DShow (  1 );
		}
		else
		{
			CPU3DShow (  0 );
		}
	}

	//  view of dynamic terrain shadow texture
	if ( t.visuals.debugvisualsmode == 3 ) 
	{
		if (  ImageExist(g.postprocessimageoffset+5) == 1 ) 
		{
			if (  ReturnKey() == 1 ) 
			{
				t.timg=g.postprocessimageoffset+5;
				Sprite (  g.postprocessimageoffset+5,-10000,-10000,t.timg );
				PasteSprite (  g.postprocessimageoffset+5,(GetDisplayWidth()-ImageWidth(t.timg))/2,(GetDisplayHeight()-ImageHeight(t.timg))/2 );
			}
			else
			{
				if (  SpriteExist(g.postprocessimageoffset+5) == 1  )  DeleteSprite (  g.postprocessimageoffset+5 );
			}
		}
	}

	/* completely removed old dynamic cheap shadow
	// Render dynamic terrain shadow camera
	if ( t.gdynamicterrainshadowcameraid>0 ) 
	{
		//  only if quad for rendering cheap shadow is visible (prebake can hide it)
		if (  ObjectExist(g.postprocessobjectoffset+5) == 1 ) 
		{
			//  Clear the dynamic terrain shadow camera
			if (  t.gdynamicterrainshadowcameragenerate>0 ) 
			{
				--t.gdynamicterrainshadowcameragenerate;
				if (  t.gdynamicterrainshadowcameragenerate == 1 ) 
				{
					SyncMask (  1<<t.gdynamicterrainshadowcameraid );
					BackdropOn (  t.gdynamicterrainshadowcameraid );
					BackdropColor (  t.gdynamicterrainshadowcameraid,Rgb(0,0,0) );
					FastSync (  );
					if (  t.game.set.ismapeditormode == 1 ) 
					{
						SyncMask (  1 );
					}
					else
					{
						SyncMask (  0xfffffff9 );
					}
					BackdropOff (  t.gdynamicterrainshadowcameraid );
				}
			}
			else
			{
				//  instead, we do a single snapshot when triggered
				if (  t.gdynamicterrainshadowcameratrigger == 1 ) 
				{
					t.gdynamicterrainshadowcameratrigger=0;
					SyncMask (  1<<t.gdynamicterrainshadowcameraid );
					FastSync (  );
					if (  t.game.set.ismapeditormode == 1 ) 
					{
						SyncMask (  1 );
					}
					else
					{
						SyncMask (  0xfffffff9 );
					}
				}
			}
		}
	}
	*/

	/* completely removed old dynamic cheap shadow, so don't need this refresh
	//  Generate heightmap texture for cheap shadows (triggered by loading new level)
	if (  t.terrain.terraintriggercheapshadowrefresh>0 ) 
	{
		t.terrain.terraintriggercheapshadowrefresh=t.terrain.terraintriggercheapshadowrefresh-1;
		if (  t.terrain.terraintriggercheapshadowrefresh == 0 ) 
		{
			t.terrain.terrainquickupdate=1;
			t.terrain.terrainquickx1=0;
			t.terrain.terrainquickx2=1024;
			t.terrain.terrainquickz1=0;
			t.terrain.terrainquickz2=1024;
			terrain_quickupdateheightmapfromheightdata ( );
		}
	}
	*/
}

void terrain_updaterealheights ( void )
{
	if (  t.terrain.TerrainID>0 ) 
	{
		//  sculp terrain from existing height data
		for ( t.z = 1 ; t.z<=  1023; t.z++ )
		{
			for ( t.x = 1 ; t.x<=  1023; t.x++ )
			{
				t.h_f=t.terrainmatrix[t.x][t.z];
				if (  t.h_f<0  )  t.h_f = 0;
				BT_SetPointHeight (  t.terrain.TerrainID,t.x,t.z,t.h_f );
			}
		}

		//  cap edges
		for ( t.z = 1 ; t.z<=  1024; t.z++ )
		{
			BT_SetPointHeight (  t.terrain.TerrainID,0,t.z,0 );
			BT_SetPointHeight (  t.terrain.TerrainID,1024,t.z,0 );
		}
		for ( t.x = 1 ; t.x<=  1024; t.x++ )
		{
			BT_SetPointHeight (  t.terrain.TerrainID,t.x,0,0 );
			BT_SetPointHeight (  t.terrain.TerrainID,t.x,1024,0 );
		}

		// after amending terrain, update height map
		BT_SetCurrentCamera (  0 );
		BT_UpdateTerrainCull (  t.terrain.TerrainID );
		BT_UpdateTerrainLOD (  t.terrain.TerrainID );
		BT_RenderTerrain (  t.terrain.TerrainID );

		//  update terrain internals and empty queue (was SyncOn (  260115) )
		BT_Intern_Render( );
	}
}

void terrain_randomiseorflattenterrain ( void )
{
	// 100417 - dont use water line for terrain generation
	float fTerrainDefaultHeight = g.gdefaultterrainheight;
	if (  t.terrainflattenmode == 1 ) 
	{
		//  flat
		for ( t.z = 0 ; t.z<=  1023; t.z++ )
		{
			for ( t.x = 0 ; t.x<=  1023; t.x++ )
			{
				t.h_f = fTerrainDefaultHeight;
				t.terrainmatrix[t.x][t.z]=t.h_f;
			}
		}
	}
	else
	{
		// first reset
		for ( t.z = 0 ; t.z<=  1023; t.z++ )
			for ( t.x = 0 ; t.x<=  1023; t.x++ )
				t.terrainmatrix[t.x][t.z] = 100;

		//  random - generate seed heights
		t.terrain.terrain_seed = Timer();
		t.terrain.terrain_range = 5 + Rnd(4);
		generate_terrain(t.terrain.terrain_seed,t.terrain.terrain_range,terrain_chunk_size);

		//  ensure terrain is above the waterline (and high enough to avoid Z clip at max editor zoomout)
		for ( t.z = 1 ; t.z<=  1023; t.z++ )
		{
			for ( t.x = 1 ; t.x<=  1023; t.x++ )
			{
				t.h_f=t.terrainmatrix[t.x][t.z]*2.5f;
				if (  t.h_f<0  )  t.h_f = 0;
				t.terrainmatrix[t.x][t.z] = fTerrainDefaultHeight + t.h_f;
			}
		}
	}

	//  sculp terrain from existing height data
	terrain_updaterealheights ( );
}

void terrain_flattenterrain ( void )
{
	t.terrainflattenmode=1;
	terrain_randomiseorflattenterrain ( );
}

void terrain_randomiseterrain ( void )
{
	t.terrainflattenmode=0;
	terrain_randomiseorflattenterrain ( );
}

void terrain_refreshterrainmatrix ( void )
{
	if (  t.terrain.TerrainID>0 ) 
	{
		if (  t.terrain.terrainregionupdate == 1 ) 
		{
			t.xs1=t.terrain.terrainregionx1;
			t.xs2=t.terrain.terrainregionx2;
			t.zs1=t.terrain.terrainregionz1;
			t.zs2=t.terrain.terrainregionz2;
		}
		else
		{
			t.xs1=0;
			t.xs2=1024;
			t.zs1=0;
			t.zs2=1024;
		}
		for ( t.z = t.zs1 ; t.z<=  t.zs2; t.z++ )
		{
			for ( t.x = t.xs1 ; t.x<=  t.xs2; t.x++ )
			{
				t.h_f=BT_GetGroundHeight(t.terrain.TerrainID,t.x*50.0,t.z*50.0,1);
				t.terrainmatrix[t.x][t.z]=t.h_f;
			}
		}
	}
}

void terrain_skipifnowaterexposed ( void )
{
	// returns tokay=1 if we should skip
	t.xs1=2; t.xs2=1024-2; t.zs1=2; t.zs2=1024-2;
	for ( t.z = t.zs1 ; t.z <= t.zs2; t.z++ )
	{
		for ( t.x = t.xs1 ; t.x <= t.xs2; t.x++ )
		{
			if ( t.terrainmatrix[t.x][t.z] < t.terrain.waterliney_f ) 
			{
				// below water - we cannot skip
				t.tokay=0; 
				return;
			}
		}
	}

	//  no water found, we can skip
	t.tokay=1;
}

void terrain_updatewatermask ( void )
{
	//  takes tfilewater$
	//  if VIDMEM reset, must leave silently

	if (g.memskipwatermask == 1)  return; //PE: if watermask not used.

	if (  BitmapExist(g.terrainworkbitmapindex) == 0  )  return;
	//  if no water exposed, we can skip this
	terrain_skipifnowaterexposed() ; if (  t.tokay == 1  )  return;
	//  terrain.terrainregionupdate ; 0-full update, 1-local region update, 2-local but save new file
	if (  ImageExist(t.terrain.imagestartindex+4) == 0 ) 
	{
		if (  FileExist(t.tfilewater_s.Get()) == 1 ) 
		{
			LoadImage (  t.tfilewater_s.Get(),t.terrain.imagestartindex+4,10,0 );
		}
	}
	if (  t.game.gameisexe == 0 ) 
	{
		//  only create new water mask and save if not standalone
		if (  t.terrain.terrainregionupdate>0 ) 
		{
			t.xs1=t.terrain.terrainregionx1*MAXTEXTURESIZEMULTIPLIER;
			t.xs2=(t.terrain.terrainregionx2*MAXTEXTURESIZEMULTIPLIER)-1;
			t.zs1=t.terrain.terrainregionz1*MAXTEXTURESIZEMULTIPLIER;
			t.zs2=(t.terrain.terrainregionz2*MAXTEXTURESIZEMULTIPLIER)-1;
		}
		else
		{
			t.xs1=0;
			t.xs2=MAXTEXTURESIZE-1;
			t.zs1=0;
			t.zs2=MAXTEXTURESIZE-1;
		}
		SetCurrentBitmap (  g.terrainworkbitmapindex );
		if (  ImageExist(t.terrain.imagestartindex+4) == 1 ) 
		{
			PasteImage (  t.terrain.imagestartindex+4,0,0 );
		}
		LockPixels (  );
		for ( t.z = t.zs1 ; t.z<=  t.zs2; t.z++ )
		{
			for ( t.x = t.xs1 ; t.x<=  t.xs2; t.x++ )
			{
				t.nx_f=t.x;
				t.nz_f=t.z;
				t.nx=t.nx_f/(0.0+MAXTEXTURESIZEMULTIPLIER);
				t.nz=t.nz_f/(0.0+MAXTEXTURESIZEMULTIPLIER);
				t.nxs=t.nx;
				t.nzs=t.nz;
				t.nxe = t.nx+1 ; if (  t.nxe>1023  )  t.nxe = 1023;
				t.nze = t.nz+1 ; if (  t.nze>1023  )  t.nze = 1023;
				t.nxb_f=(t.nx_f-(t.nx*MAXTEXTURESIZEMULTIPLIER))/(0.0+MAXTEXTURESIZEMULTIPLIER);
				t.nzb_f=(t.nz_f-(t.nz*MAXTEXTURESIZEMULTIPLIER))/(0.0+MAXTEXTURESIZEMULTIPLIER);
				t.h1_f=t.terrainmatrix[t.nxs][t.nzs];
				t.h2_f=t.terrainmatrix[t.nxe][t.nzs];
				t.h3_f=t.terrainmatrix[t.nxs][t.nze];
				t.h4_f=t.terrainmatrix[t.nxe][t.nze];
				t.ha_f=t.h1_f+((t.h2_f-t.h1_f)*t.nxb_f);
				t.hb_f=t.h3_f+((t.h4_f-t.h3_f)*t.nxb_f);
				t.h_f=t.ha_f+((t.hb_f-t.ha_f)*t.nzb_f);
				if (  t.h_f<0  )  t.h_f = 0;
				if (  t.h_f >= t.terrain.waterliney_f ) 
				{
					//  above water
					t.a=0;
				}
				else
				{
					//  below/in water
					t.a=(t.terrain.waterliney_f-t.h_f)*10.0;
					if (  t.a>255  )  t.a = 255;
				}
				//  ripple effect
				t.nxs = t.nx-1 ; if (  t.nxs<0  )  t.nxs = 0;
				t.nzs = t.nz-1 ; if (  t.nzs<0  )  t.nzs = 0;
				t.nxe = t.nx+1 ; if (  t.nxe>1023  )  t.nxe = 1023;
				t.nze = t.nz+1 ; if (  t.nze>1023  )  t.nze = 1023;
				t.nxb_f=(t.nx_f-(t.nx*MAXTEXTURESIZEMULTIPLIER))/(0.0+MAXTEXTURESIZEMULTIPLIER);
				t.nzb_f=(t.nz_f-(t.nz*MAXTEXTURESIZEMULTIPLIER))/(0.0+MAXTEXTURESIZEMULTIPLIER);
				t.h1_f=t.terrainmatrix[t.nxs][t.nzs];
				t.h2_f=t.terrainmatrix[t.nxe][t.nzs];
				t.h3_f=t.terrainmatrix[t.nxs][t.nze];
				t.h4_f=t.terrainmatrix[t.nxe][t.nze];
				t.b=t.a;
				if (  t.h1_f >= t.terrain.waterliney_f || t.h2_f>= t.terrain.waterliney_f || t.h3_f >= t.terrain.waterliney_f || t.h4_f>= t.terrain.waterliney_f ) 
				{
					t.b=0;
				}
				Dot (  t.x,t.z,Rgb(t.a,t.b,t.a) );
				++t.tdotcount;
			}
		}
		UnlockPixels (  );
		if (  ImageExist(t.terrain.imagestartindex+4)  ==  1  )  DeleteImage (  t.terrain.imagestartindex+4 );
		GrabImage (  t.terrain.imagestartindex+4,0,0,MAXTEXTURESIZE,MAXTEXTURESIZE );
		if (  ImageExist(t.terrain.imagestartindex+4)  ==  1 ) 
		{
			if (  t.terrain.terrainregionupdate == 0 || t.terrain.terrainregionupdate == 2 || FileExist(t.tfilewater_s.Get()) == 0 ) 
			{
				if (  FileExist(t.tfilewater_s.Get()) == 1  )  DeleteAFile (  t.tfilewater_s.Get() );
				SaveImage (  t.tfilewater_s.Get(),t.terrain.imagestartindex+4,5 );
			}
		}
		SetCurrentBitmap (  0 );
	}
}

void terrain_updatewatermask_new ( void )
{
	SetCurrentBitmap ( g.terrainworkbitmapindex );
	CLS ( 0 );
	GrabImage ( t.terrain.imagestartindex+4, 0, 0, 1, 1 );
	if ( FileExist(t.tfilewater_s.Get()) == 1 ) DeleteAFile ( t.tfilewater_s.Get() );
	SaveImage ( t.tfilewater_s.Get(),t.terrain.imagestartindex+4 );
	SetCurrentBitmap (  0 );
}

void terrain_whitewashwatermask ( void )
{
	SetCurrentBitmap ( g.terrainworkbitmapindex );
	LockPixels ( );
	Dot ( 0,0,Rgb(255,255,255) );
	UnlockPixels ( );
	GrabImage ( t.terrain.imagestartindex+4,0,0,1,1 );
	SetCurrentBitmap ( 0 );
	if ( ObjectExist(t.terrain.objectstartindex+5) == 1 ) 
	{
		TextureObject ( t.terrain.objectstartindex+5,3,t.terrain.imagestartindex+4 );
	}
}

void terrain_createheightmapfromheightdata ( void )
{
	// take current terrain heights and make a height map image
	SetCurrentBitmap ( g.terrainworkbitmapindex );
	LockPixels ( );
	for ( t.z = 0 ; t.z<=  1024-1; t.z++ )
	{
		for ( t.x = 0 ; t.x<=  1024-1; t.x++ )
		{
			t.h_f=0.0+(t.terrainmatrix[t.x][t.z]*100.0);
			if ( t.h_f<0  )  t.h_f = 0.0;
			t.tcolr=t.h_f/65536.0;
			t.tcolg=(t.h_f-(t.tcolr*65536.0))/256.0;
			t.tcolb=t.h_f-(t.tcolr*65536.0)-(t.tcolg*256.0);
			Dot ( t.x,t.z,Rgb(t.tcolr,t.tcolg,t.tcolb) );
		}
	}
	UnlockPixels ( );

	//  re-used this for dynamic terrain shadow texture generation (need terrain heights for shader)
	if ( ImageExist(g.postprocessimageoffset+6) == 1 ) DeleteImage ( g.postprocessimageoffset+6 );
	GrabImage ( g.postprocessimageoffset+6,0,0,1024,1024 );
	SetCurrentBitmap ( 0 );
}

void terrain_quickupdateheightmapfromheightdata ( void )
{
	//  work out if we update ALL or just a region
	t.xs1=0 ; t.xs2=1024 ; t.zs1=0 ; t.zs2=1024;
	if (  t.terrain.terrainquickupdate == 1 ) 
	{
		t.terrain.terrainquickupdate=0;
		t.xs1=t.terrain.terrainquickx1;
		t.xs2=t.terrain.terrainquickx2;
		t.zs1=t.terrain.terrainquickz1;
		t.zs2=t.terrain.terrainquickz2;
	}
	//  get latest heights from terrain
	if (  t.terrain.TerrainID>0 ) 
	{
		for ( t.z = t.zs1 ; t.z<=  t.zs2; t.z++ )
		{
			for ( t.x = t.xs1 ; t.x<=  t.xs2; t.x++ )
			{
				t.h_f=BT_GetGroundHeight(t.terrain.TerrainID,t.x*50.0,t.z*50.0,1);
				t.terrainmatrix[t.x][t.z]=t.h_f;
			}
		}
	}
	else
	{
		for ( t.z = t.zs1 ; t.z<=  t.zs2; t.z++ )
		{
			for ( t.x = t.xs1 ; t.x<=  t.xs2; t.x++ )
			{
				t.h_f=1000.0 ; t.terrainmatrix[t.x][t.z]=t.h_f;
			}
		}
	}
	//  update heightmap texture
	SetCurrentBitmap (  g.terrainworkbitmapindex );
	if (  ImageExist(g.postprocessimageoffset+6) == 1  )  PasteImage (  g.postprocessimageoffset+6,0,0 );
	LockPixels (  );
	for ( t.z = t.zs1 ; t.z<=  t.zs2-1; t.z++ )
	{
		for ( t.x = t.xs1 ; t.x<=  t.xs2-1; t.x++ )
		{
			t.h_f=0.0+(t.terrainmatrix[t.x][t.z]*100.0);
			if (  t.h_f<0  )  t.h_f = 0.0;
			t.tcolr=t.h_f/65536.0;
			t.tcolg=(t.h_f-(t.tcolr*65536.0))/256.0;
			t.tcolb=t.h_f-(t.tcolr*65536.0)-(t.tcolg*256.0);
			Dot (  t.x,t.z,Rgb(t.tcolr,t.tcolg,t.tcolb) );
		}
	}
	UnlockPixels (  );
	if (  ImageExist(g.postprocessimageoffset+6) == 1  )  DeleteImage (  g.postprocessimageoffset+6 );
	GrabImage (  g.postprocessimageoffset+6,0,0,1024,1024 );
	SetCurrentBitmap (  0 );
}

void terrain_generatetextureselect ( void )
{
	SetCurrentBitmap (  g.terrainworkbitmapindex );
	LockPixels (  );
	for ( t.z = 0 ; t.z<=  MAXTEXTURESIZE; t.z++ )
	{
		for ( t.x = 0 ; t.x<=  MAXTEXTURESIZE; t.x++ )
		{
			t.h_f=t.terrainmatrix[t.x/MAXTEXTURESIZEMULTIPLIER][t.z/MAXTEXTURESIZEMULTIPLIER];
			if (  t.h_f<0  )  t.h_f = 0;
			t.texselect=Rgb(0,0,0);
			t.grasscoverage=Rgb(0,0,0);
			if (  t.h_f >= t.terrain.waterliney_f-100.0 && t.h_f <= t.terrain.waterliney_f+50.0 ) 
			{
				//  shore
				t.by_f=(t.h_f-(t.terrain.waterliney_f-100.0))/150.0;
				t.texselect=Rgb(0,0,196.0-(t.by_f*128.0));
			}
			else
			{
				if (  t.h_f<t.terrain.waterliney_f-50.0 ) 
				{
					//  underwater
					t.texselect=Rgb(0,0,192);
				}
				else
				{
					if (  t.h_f>t.terrain.waterliney_f+50.0 ) 
					{
						//  inland
						t.texselect=Rgb(0,0,64);
						t.thresh_f=750.0;
						if (  t.h_f>t.terrain.waterliney_f+t.thresh_f ) 
						{
							//  too high for grass
							t.by_f=(t.h_f-(t.terrain.waterliney_f+t.thresh_f))/500.0;
							t.cmax = 64.0+(t.by_f*64.0) ; if (  t.cmax>128+32  )  t.cmax = 128+32;
							t.texselect=Rgb(0,0,t.cmax);
							t.by_f=(t.h_f-(t.terrain.waterliney_f+t.thresh_f))/200.0;
							t.cmax=255-(t.by_f*255.0);
							if (  t.cmax<0  )  t.cmax = 0;
							if (  t.cmax>255  )  t.cmax = 255;
							t.grasscoverage=Rgb(t.cmax,0,0);
						}
						else
						{
							//  grass in sweet spot
							t.by_f=(t.h_f-(t.terrain.waterliney_f+100.0))/200.0;
							t.cmax=(t.by_f*255.0);
							if (  t.cmax<0  )  t.cmax = 0;
							if (  t.cmax>255  )  t.cmax = 255;
							t.grasscoverage=Rgb(t.cmax,0,0);
						}
					}
				}
			}
			Dot (  t.x,t.z,t.texselect+t.grasscoverage );
		}
	}
	UnlockPixels (  );
	GrabImage (  t.terrain.imagestartindex+2,0,0,MAXTEXTURESIZE,MAXTEXTURESIZE );
	if (  MemblockExist(123) == 1  )  DeleteMemblock (  123 );
	CreateMemblockFromImage (  123,t.terrain.imagestartindex+2 );
	TextureObject (  t.terrain.terrainobjectindex,0,t.terrain.imagestartindex+2 );
	SetCurrentBitmap (  0 );
}

void terrain_deletesupertexturepalette ( void )
{
	#ifdef ENABLECUSTOMTERRAIN
	if ( g.terrainstyleindex == 1 )
	{
		// custom from levelbank\testmap
		t.tfile_s = g.mysystem.levelBankTestMap_s+"superpalette.ter"; //cstr("levelbank\\testmap\\superpalette.ter");
	}
	else
	#endif
	{
		// regular from terrainbank
		g.terrainstyle_s = t.terrainstylebank_s[g.terrainstyleindex];
		t.tfile_s = cstr("terrainbank\\")+g.terrainstyle_s+"\\superpalette.ter";
	}
	if ( FileExist(t.tfile_s.Get()) == 1 ) DeleteFile ( t.tfile_s.Get() );
}

void terrain_generatesupertexture ( bool bForceRecalcOfPalette )
{
	// In superflat mode 2, we don't have a terrain
	if ( t.terrain.superflat == 2 )  return;

	// determine location for terrain texture palette
	#ifdef ENABLECUSTOMTERRAIN
	if ( g.terrainstyleindex == 1 )
	{
		// custom from levelbank\testmap
		t.tfile_s = g.mysystem.levelBankTestMap_s+"superpalette.ter"; //cstr("levelbank\\testmap\\superpalette.ter");
	}
	else
	#endif
	{
		// regular from terrainbank
		g.terrainstyle_s = t.terrainstylebank_s[g.terrainstyleindex];
		t.tfile_s = cstr("terrainbank\\")+g.terrainstyle_s+"\\superpalette.ter";
	}

	// First obtain paint palette from terrainbank
	if ( FileExist(t.tfile_s.Get()) == 1 ) 
	{
		// eraese older versions of file
		OpenToRead ( 7, t.tfile_s.Get() );
		t.tversion = ReadLong ( 7 );
		CloseFile ( 7 );
		if ( t.tversion == 100 ) 
			DeleteFile ( t.tfile_s.Get() );
	}
	if ( FileExist(t.tfile_s.Get()) == 1 && bForceRecalcOfPalette == false ) 
	{
		// Load pre-calculated data into arrays
		OpenToRead (  7,t.tfile_s.Get() );
		t.tversion = ReadLong ( 7 );
		if ( t.tversion == 100 )
		{
			// version 100
			for ( t.p = 1 ; t.p <= 5; t.p++ )
			{
				for ( t.z = 0 ; t.z <= 3; t.z++ )
				{
					for ( t.x = 0 ; t.x <= 3; t.x++ )
					{
						t.a = ReadLong ( 7 ); 
						t.pot[t.p][t.x][t.z]=t.a;
					}
				}
			}
		}
		if ( t.tversion == 103 )
		{
			// version 103 - March 2017
			for ( t.p = 1 ; t.p <= 16; t.p++ )
			{
				for ( t.z = 0 ; t.z <= 3; t.z++ )
				{
					for ( t.x = 0 ; t.x <= 3; t.x++ )
					{
						t.a = ReadLong ( 7 ); 
						t.pot[t.p][t.x][t.z] = t.a;
					}
				}
			}
		}
		CloseFile (  7 );
		if ( t.game.runasmultiplayer == 1 ) mp_refresh ( );
	}
	else
	{
		// Delete old file first
		if ( FileExist(t.tfile_s.Get()) == 1 ) DeleteFile ( t.tfile_s.Get() );

		// Generate pallete data for super terrain and save as file
		SetCurrentBitmap ( g.terrainworkbitmapindex );
		for ( t.p = 1 ; t.p <= 16; t.p++ )
		{
			t.timg=t.terrain.imagestartindex+13;
			int pasterow = (t.p-1)/4;
			int pastecol = (t.p-1)-(pasterow*4);
			int pasteatx = pastecol*1024;
			int pasteaty = pasterow*1024;
			if ( GetImageExistEx ( t.timg ) ) PasteImage ( t.timg, -pasteatx, -pasteaty );
			LockPixels ( );
			t.timgw=1024/4;
			t.timgh=1024/4;
			for ( t.z = 0 ; t.z<=  3; t.z++ )
			{
				for ( t.x = 0 ; t.x<=  3; t.x++ )
				{
					//  find average of this quadrant of the paint texture
					t.tr_f=0 ; t.tg_f=0 ; t.tb_f=0 ; t.tt_f=0;
					for ( t.avz = t.z*t.timgh ; t.avz<=  ((t.z+1)*t.timgh)-1; t.avz++ )
					{
						for ( t.avx = t.x*t.timgw ; t.avx<=  ((t.x+1)*t.timgw)-1; t.avx++ )
						{
							t.trgba=GetPoint(t.avx,t.avz);
							t.tr_f=t.tr_f+RgbR(t.trgba);
							t.tg_f=t.tg_f+RgbG(t.trgba);
							t.tb_f=t.tb_f+RgbB(t.trgba);
							t.tt_f=t.tt_f+1;
						}
						if ( t.game.runasmultiplayer == 1 ) mp_refresh ( );
					}
					t.tr=t.tr_f/t.tt_f;
					t.tg=t.tg_f/t.tt_f;
					t.tb=t.tb_f/t.tt_f;
					if (  t.tr>255  )  t.tr = 255;
					if (  t.tg>255  )  t.tg = 255;
					if (  t.tb>255  )  t.tb = 255;
					t.pot[t.p][t.x][t.z]=(t.tr<<16)+(t.tg<<8)+(t.tb);
				}
				if ( t.game.runasmultiplayer == 1 ) mp_refresh ( );
			}
			UnlockPixels (  );
		}
		SetCurrentBitmap (  0 );

		//  Save pre-calculated data
		OpenToWrite ( 7, t.tfile_s.Get() );
		int iVersionNumber = 103;
		WriteLong ( 7, iVersionNumber );
		for ( t.p = 1 ; t.p <= 16; t.p++ )
		{
			for ( t.z = 0 ; t.z <= 3; t.z++ )
			{
				for ( t.x = 0 ; t.x <= 3; t.x++ )
				{
					t.a = t.pot[t.p][t.x][t.z]; 
					WriteLong ( 7, t.a );
				}
			}
		}
		CloseFile (  7 );
		if ( t.game.runasmultiplayer == 1 ) mp_refresh ( );
	}

	// New command to create the mega texture entirely inside the Image Module (special mode)
	if ( MemblockExist(9) == 1  ) DeleteMemblock ( 9 );
	MakeMemblock ( 9, (17*16)*4 );
	for ( t.p = 1 ; t.p <= 16; t.p++ )
	{
		for ( t.z = 0 ; t.z <= 3; t.z++ )
		{
			for ( t.x = 0 ; t.x <= 3; t.x++ )
			{
				WriteMemblockDWord ( 9, ((t.p*16)+(t.z*4)+t.x)*4,t.pot[t.p][t.x][t.z] );
			}
		}
	}
	TransferImage ( t.terrain.imagestartindex+17,t.terrain.imagestartindex+2,1,9 );

	//  Get the 'approximate Floor ( colour (from default_D) top-left corner pixel (for hemisphere lighting) )
	t.floorcolor=t.pot[3][0][0];
	t.terrain.floorcolorr_f=RgbR(t.floorcolor);
	t.terrain.floorcolorg_f=RgbG(t.floorcolor);
	t.terrain.floorcolorb_f=RgbB(t.floorcolor);
	t.terrain.floorcolorr_f=(t.terrain.floorcolorr_f*0.25)+(255*0.75);
	t.terrain.floorcolorg_f=(t.terrain.floorcolorg_f*0.25)+(255*0.75);
	t.terrain.floorcolorb_f=(t.terrain.floorcolorb_f*0.25)+(255*0.75);

	if ( t.game.runasmultiplayer == 1 ) mp_refresh ( );

	TextureObject (  t.terrain.terrainobjectindex,3,t.terrain.imagestartindex+17 );
}

void terrain_generateshadows ( void )
{
	/*      
	//  NOTE; need to fix from 4096 to MAXTEXTURESIZE
	//  shadow light setup
	t.terrain.lightrange_f=2500;
	t.terrain.ldirx_f=-0.25*(-1*t.terrain.lightrange_f);
	t.terrain.ldiry_f=-0.5*(-1*t.terrain.lightrange_f);
	t.terrain.ldirz_f=-0.25*(-1*t.terrain.lightrange_f);
	t.terrain.ldirnx_f=t.terrain.ldirx_f/t.terrain.lightrange_f;
	t.terrain.ldirny_f=t.terrain.ldiry_f/t.terrain.lightrange_f;
	t.terrain.ldirnz_f=t.terrain.ldirz_f/t.terrain.lightrange_f;
	//  paint shadows to map
	if (  BitmapExist(g.terrainworkbitmapindex) == 1  )  DeleteBitmapEx (  g.terrainworkbitmapindex );
	if (  BitmapExist(g.terrainworkbitmapindex) == 0 ) 
	{
		CreateBitmap (  g.terrainworkbitmapindex,MAXTEXTURESIZE,MAXTEXTURESIZE );
		PasteImage (  t.terrain.imagestartindex+2,0,0 );
		LockPixels (  );
		//  slope
		lightslope_f=abs(t.terrain.ldiry_f)/1024.0;
		//  paint shadow pixels
		for ( t.z = (MAXTEXTURESIZE/2)-200 ; t.z<=  (MAXTEXTURESIZE/2)+600; t.z++ )
		{
			for ( t.x = (MAXTEXTURESIZE/2)-200 ; t.x<=  (MAXTEXTURESIZE/2)+600; t.x++ )
			{
				t.x_f=(t.x-(MAXTEXTURESIZE/2))*12.5;
				t.z_f=(t.z-(MAXTEXTURESIZE/2))*12.5;
				//  interpolate correct height
				t.nx_f=t.x;
				t.nz_f=t.z;
				t.nx=t.nx_f/(0.0+MAXTEXTURESIZEMULTIPLIER);
				t.nz=t.nz_f/(0.0+MAXTEXTURESIZEMULTIPLIER);
				t.nxb_f=(t.nx_f-(t.nx*MAXTEXTURESIZEMULTIPLIER))/(0.0+MAXTEXTURESIZEMULTIPLIER);
				t.nzb_f=(t.nz_f-(t.nz*MAXTEXTURESIZEMULTIPLIER))/(0.0+MAXTEXTURESIZEMULTIPLIER);
				t.h1_f=t.terrainmatrix[t.nx][t.nz];
				t.h2_f=t.terrainmatrix[t.nx+1][t.nz];
				t.h3_f=t.terrainmatrix[t.nx][t.nz+1];
				t.h4_f=t.terrainmatrix[t.nx+1][t.nz+1];
				//  can get normal from four height corners, and produce normal for Dot (  against light direction )
				t.h_f=99999.9;
				if (  t.h1_f<t.h_f  )  t.h_f = t.h1_f;
				if (  t.h2_f<t.h_f  )  t.h_f = t.h2_f;
				if (  t.h3_f<t.h_f  )  t.h_f = t.h3_f;
				if (  t.h4_f<t.h_f  )  t.h_f = t.h4_f;
				hn1_f=t.h1_f-t.h_f;
				hn2_f=t.h2_f-t.h_f;
				hn3_f=t.h3_f-t.h_f;
				hn4_f=t.h4_f-t.h_f;
				terraintilesize_f=12.5*MAXTEXTURESIZEMULTIPLIER;
				thisnormalx_f=(((hn1_f-hn2_f)/terraintilesize_f)+((hn3_f-hn4_f)/terraintilesize_f))/2.0;
				thisnormalz_f=(((hn1_f-hn3_f)/terraintilesize_f)+((hn2_f-hn4_f)/terraintilesize_f))/2.0;
				thisnormaly_f=1.0-thisnormalx_f-thisnormalz_f;
				thisdot_f=(thisnormalx_f*t.terrain.ldirnx_f)+(thisnormaly_f*t.terrain.ldirny_f)+(thisnormalz_f*t.terrain.ldirnz_f);
				//  trace from terrain GetPoint (  towards sun )
				//  stepping up the ray slope, and if
				//  any terrain height is HIGHER, then
				//  the sun has been blocked by terrain
				t.ha_f=t.h1_f+((t.h2_f-t.h1_f)*t.nxb_f);
				t.hb_f=t.h3_f+((t.h4_f-t.h3_f)*t.nxb_f);
				t.h_f=t.ha_f+((t.hb_f-t.ha_f)*t.nzb_f);
				t.d=0;
				dleadup=0;
				t.nx_f=t.x;
				ny_f=t.h_f;
				t.nz_f=t.z;
				for ( t.n = 0 ; t.n<=  MAXTEXTURESIZE-1; t.n++ )
				{
					t.nx_f=t.nx_f+(t.terrain.ldirnx_f*5.0);
					ny_f=ny_f+(t.terrain.ldirny_f*5.0);
					t.nz_f=t.nz_f+(t.terrain.ldirnz_f*5.0);
					t.nx=t.nx_f/4;
					t.nz=t.nz_f/4;
					if (  t.nx>0 && t.nx<1024 && t.nz>0 && t.nz<1024 ) 
					{
						//  interpolate between the four corner heights
						t.nxb_f=(t.nx_f-(t.nx*4))/4.0;
						t.nzb_f=(t.nz_f-(t.nz*4))/4.0;
						t.h1_f=t.terrainmatrix[t.nx][t.nz];
						t.h2_f=t.terrainmatrix[t.nx+1][t.nz];
						t.h3_f=t.terrainmatrix[t.nx][t.nz+1];
						t.h4_f=t.terrainmatrix[t.nx+1][t.nz+1];
						t.ha_f=t.h1_f+((t.h2_f-t.h1_f)*t.nxb_f);
						t.hb_f=t.h3_f+((t.h4_f-t.h3_f)*t.nxb_f);
						t.h_f=t.ha_f+((t.hb_f-t.ha_f)*t.nzb_f);
						if (  t.h_f>ny_f ) 
						{
							//  terrain blocked sun ray = shadow!
							dleadupsize=dleadup;
							inc t.d ; if (  t.d >= 16  )  break;
						}
						inc dleadup;
						if (  ny_f>5500.0 ) 
						{
							//  ray cleared highest peek
							break;
						}
					}
					else
					{
						//  ray left map
						break;
					}
				}
				t.col=GetPoint(t.x,t.z);
				colr=RgbR(t.col);
				colg=RgbG(t.col);
				colb=RgbB(t.col);
				colg=colg+abs(thisdot_f)*64;
				colgbase=colg;
				if (  t.d == 0 ) 
				{
					//  can skip, no change, in full light
				}
				else
				{
					castshadowstr=(t.d*12);
					castshadowstr=castshadowstr-(dleadupsize);
					colg=colg+castshadowstr;
				}
				if (  colg<colgbase  )  colg = colgbase;
				if (  colg>255  )  colg = 255;
				Dot (  t.x,t.z,Rgb(colr,colg,colb) );
			}
		}
		//  blue step to smooth in strange step artefacts?!?
		if (  0 ) 
		{
		for ( t.z = 2048-200 ; t.z<=  2048+200; t.z++ )
		{
			for ( t.x = 2048-200 ; t.x<=  2048+200; t.x++ )
			{
				t.col=GetPoint(t.x,t.z);
				colr=RgbR(t.col);
				colg=RgbG(t.col);
				colb=RgbB(t.col);
				if (  colg>0 ) 
				{
					//  back
					colgstore=colg/2;
					t.col=GetPoint(t.x-1,t.z);
					colr=RgbR(t.col);
					colg=RgbG(t.col)+colgstore;
					colb=RgbB(t.col);
					if (  colg>255  )  colg = 255;
					Dot (  t.x-1,t.z,Rgb(colr,colg,colb) );
					//  up
					t.col=GetPoint(t.x,t.z-1);
					colr=RgbR(t.col);
					colg=RgbG(t.col)+colgstore;
					colb=RgbB(t.col);
					if (  colg>255  )  colg = 255;
					Dot (  t.x,t.z-1,Rgb(colr,colg,colb) );
					//  further blur
					colgstore=colgstore/2;
					t.col=GetPoint(t.x-1,t.z-1);
					colr=RgbR(t.col);
					colg=RgbG(t.col)+colgstore;
					colb=RgbB(t.col);
					if (  colg>255  )  colg = 255;
					Dot (  t.x-1,t.z-1,Rgb(colr,colg,colb) );
					t.col=GetPoint(t.x-2,t.z);
					colr=RgbR(t.col);
					colg=RgbG(t.col)+colgstore;
					colb=RgbB(t.col);
					if (  colg>255  )  colg = 255;
					Dot (  t.x-2,t.z,Rgb(colr,colg,colb) );
					t.col=GetPoint(t.x,t.z-2);
					colr=RgbR(t.col);
					colg=RgbG(t.col)+colgstore;
					colb=RgbB(t.col);
					if (  colg>255  )  colg = 255;
					Dot (  t.x,t.z-2,Rgb(colr,colg,colb) );
				}
			}
		}
		}
		UnlockPixels (  );
		GrabImage (  t.terrain.imagestartindex+2,0,0,MAXTEXTURESIZE,MAXTEXTURESIZE );
		TextureObject (  t.terrain.objectstartindex+1,t.terrain.imagestartindex+2 );
		ShowObject (  t.terrain.objectstartindex+1 );
		DeleteBitmapEx (  g.terrainworkbitmapindex );
	}
	else
	{
			"Bitmap "+Str(g.terrainworkbitmapindex)+" already exists!","Terrain" ; end;
	}
	*/ 
}

void generate_terrain ( int seed, int scale, int mchunk_size )
{
	// restored to original method
	// diamond-square algorithm - thanks to Lewis999 from the DBP 20 liner challenge forum!
	Randomize(seed);
	int s = scale;
	// generates the main point on the line z=0 for the first matrix 
	for ( int x = 2^s; x <= mchunk_size; x += 2^s )
	{
		t.terrainmatrix[x][0] = t.terrainmatrix[x-2^s][0] + Rnd((2^s)*10)-((2^s)*10)/2;
	}
	//generates the main point on theline y=0 for the first matrix 
	for ( int y = 2^s; y <= mchunk_size; y += 2^s )
	{
		t.terrainmatrix[0][y] = t.terrainmatrix[0][y-2^s] + Rnd((2^s)*10)-((2^s)*10)/2;
	}
	//generates the rest of the main points for the first matrix
	for ( int y = 2^s; y <= mchunk_size; y += 2^s )
	{
		for ( int x = 2^s; x <= mchunk_size; x += 2^s )
		{
			t.terrainmatrix[x][y] = (t.terrainmatrix[x-2^s][y] + t.terrainmatrix[x][y-2^s])/2 + Rnd((2^s)*10)-((2^s)*10)/2;
		}
	}
	//calculates the rest of the points based on the main points
	for ( int o = 1; o <=s; o++ )
	{
		int p = s-o+1;
		int pow = powf(2,p); // replaces 2^p
		for ( int y = 0; y <=mchunk_size; y += pow )
		{
			for ( int x = pow/2; x <= mchunk_size; x += pow )
			{
				t.terrainmatrix[x][y] = (t.terrainmatrix[x-(pow/2)][y] + t.terrainmatrix[x+(pow/2)][y])/2 + Rnd(pow*2)-pow;
			}
		}
		for ( int y = pow/2; y <= mchunk_size; y += pow )
		{
			for ( int x = 0; x <= mchunk_size; x += pow )
			{
				t.terrainmatrix[x][y] = (t.terrainmatrix[x][y-(pow/2)] + t.terrainmatrix[x][y+(pow/2)])/2 + Rnd(pow*2)-pow;
			}
		}
		for ( int y = pow/2; y <= mchunk_size; y += pow )
		{
			for ( int x = pow/2; x <= mchunk_size; x += pow )
			{
				t.terrainmatrix[x][y] = (t.terrainmatrix[x][y-(pow/2)] + t.terrainmatrix[x][y+(pow/2)] + t.terrainmatrix[x-(pow/2)][y] + t.terrainmatrix[x+(pow/2)][y])/4 + Rnd(pow*2)-pow;
			}
		}
	}
}

void generate_terrain_dave ( int seed, int scale, int mchunk_size )
{
	// flatten out initially
	for ( int y = 0 ; y < 1024 ; y++ )
	{
		for ( int x = 0 ; x < 1024 ; x++ )
		{
			t.terrainmatrix[x][y] = 0;
		}
	}
	DiamondSquare ( 0,0,1024,1024,400,128 );
	return;
}

void DiamondSquare(unsigned x1, unsigned y1, unsigned x2, unsigned y2, float range, unsigned level) 
{
    if (level < 1) return;

    // diamonds
    for (unsigned i = x1 + level; i < x2; i += level)
	{
        for (unsigned j = y1 + level; j < y2; j += level) 
		{
            float a = t.terrainmatrix[i - level][j - level];
            float b = t.terrainmatrix[i][j - level];
            float c = t.terrainmatrix[i - level][j];
            float d = t.terrainmatrix[i][j];
            float e = t.terrainmatrix[i - level / 2][j - level / 2] = (a + b + c + d) / 4 + ((float)Rnd(100) / 100.0f) * range;
        }
	}

    // squares
    for (unsigned i = x1 + 2 * level; i < x2; i += level)
	{
        for (unsigned j = y1 + 2 * level; j < y2; j += level) 
		{
            float a = t.terrainmatrix[i - level][j - level];
            float b = t.terrainmatrix[i][j - level];
            float c = t.terrainmatrix[i - level][j];
            float d = t.terrainmatrix[i][j];
            float e = t.terrainmatrix[i - level / 2][j - level / 2];

            float f = t.terrainmatrix[i - level][j - level / 2] = (a + c + e + t.terrainmatrix[i - 3 * level / 2][j - level / 2]) / 4 + ((float)Rnd(100) / 100.0f) * range;
            float g = t.terrainmatrix[i - level / 2][j - level] = (a + b + e + t.terrainmatrix[i - level / 2][j - 3 * level / 2]) / 4 + ((float)Rnd(100) / 100.0f) * range;
        }
	}

    DiamondSquare(x1, y1, x2, y2, range / 2, level / 2);
}

// 
//  TERRAIN IN-GAME
// 

void terrain_start_play ( void )
{
	//  Turn highlighting off
	if ( t.terrain.terrainshaderindex>0 ) 
	{
		SetVector4 (  g.terrainvectorindex,-999999,-999999,0,0 );
		SetEffectConstantV (  t.terrain.terrainshaderindex,"HighlightCursor",g.terrainvectorindex );
		SetVector4 (  g.terrainvectorindex,0,0,0,0 );
		SetEffectConstantV (  t.terrain.terrainshaderindex,"HighlightParams",g.terrainvectorindex );
	}

	// switch off normals in terrain (allow baked shadows to takeover)
	if ( ObjectExist(t.terrain.terrainobjectindex) == 1 ) 
	{
		SetObjectLight (  t.terrain.terrainobjectindex,0 );
	}

	//  fog effect to create sense of distance
	t.tFogNear_f=t.visuals.FogNearest_f ; t.tFogFar_f=t.visuals.FogDistance_f;
	t.tFogR_f=t.visuals.FogR_f ; t.tFogG_f=t.visuals.FogG_f ; t.tFogB_f=t.visuals.FogB_f ; ; t.tFogA_f=t.visuals.FogA_f;
	terrain_setfog ( );
}

void terrain_stop_play ( void )
{
	t.tFogR_f = 0; t.tFogG_f = 0; t.tFogB_f = 0; t.tFogA_f = 0; t.tFogNear_f = 500000; t.tFogFar_f = 15000000;
	terrain_setfog ( );

	// switch terrain normals back on for real-time editing
	if ( ObjectExist(t.terrain.terrainobjectindex) == 1 ) 
	{
		SetObjectLight (  t.terrain.terrainobjectindex,1 );
		SetObjectMask (  t.terrain.terrainobjectindex, 1 );
	}

	// Restore camera range for editing
	SetCameraRange (  100,55000 );
}

void terrain_setfog ( void )
{
	// takes; tFogR#,tFogG#,tFogB#,tFogNear#,tFogFar#
	if ( t.terrain.terrainshaderindex>0 ) 
	{
		SetVector4 ( g.terrainvectorindex,0,0,0,0 );
		SetEffectConstantV ( t.terrain.terrainshaderindex,"FogColor",g.terrainvectorindex );
		SetVector4 ( g.terrainvectorindex,t.tFogR_f/255.0,t.tFogG_f/255.0,t.tFogB_f/255.0,t.tFogA_f/255.0 );
		SetEffectConstantV ( t.terrain.terrainshaderindex,"HudFogColor",g.terrainvectorindex );
		SetVector4 ( g.terrainvectorindex,t.tFogNear_f,t.tFogFar_f,0,0 );
		SetEffectConstantV ( t.terrain.terrainshaderindex,"HudFogDist",g.terrainvectorindex );
	}
	if ( t.terrain.vegetationshaderindex>0 ) 
	{
		SetVector4 ( g.vegetationvectorindex,0,0,0,0 );
		SetEffectConstantV ( t.terrain.vegetationshaderindex,"FogColor",g.vegetationvectorindex );
		SetVector4 ( g.vegetationvectorindex,t.tFogR_f/255.0,t.tFogG_f/255.0,t.tFogB_f/255.0,t.tFogA_f/255.0 );
		SetEffectConstantV ( t.terrain.vegetationshaderindex,"HudFogColor",g.vegetationvectorindex );
		SetVector4 ( g.vegetationvectorindex,t.tFogNear_f,t.tFogFar_f,0,0 );
		SetEffectConstantV ( t.terrain.vegetationshaderindex,"HudFogDist",g.vegetationvectorindex );
	}
}

void terrain_parsed_getstring ( void )
{
	for ( t.n = 1 ; t.n<=  Len(t.line_s.Get()); t.n++ )
	{
		if ( cstr(Mid(t.line_s.Get(),t.n)) == "=" ) 
		{
			if ( cstr(Mid(t.line_s.Get(),t.n+1)) == " " ) 
				t.rest_s=Right(t.line_s.Get(),(Len(t.line_s.Get())-t.n)-1);
			else
				t.rest_s=Right(t.line_s.Get(),Len(t.line_s.Get())-t.n);

			t.n=Len(t.line_s.Get());
		}
	}
}

void terrain_parsed_getvalues ( void )
{
	for ( t.n = 1 ; t.n<=  Len(t.line_s.Get()); t.n++ )
	{
		if ( cstr(Mid(t.line_s.Get(),t.n)) == "=" ) 
		{
			t.rest_s=Right(t.line_s.Get(),Len(t.line_s.Get())-t.n);
			t.n=Len(t.line_s.Get());
		}
	}
	t.valuei=0;
	for ( t.n = 1 ; t.n<=  Len(t.rest_s.Get()); t.n++ )
	{
		if (  cstr(Mid(t.rest_s.Get(),t.n)) == "," || t.n == Len(t.rest_s.Get()) ) 
		{
			if (  t.n == Len(t.rest_s.Get()) ) 
			{
				t.value_s=Left(t.rest_s.Get(),t.n);
			}
			else
			{
				t.value_s=Left(t.rest_s.Get(),t.n-1);
			}
			t.value_f[t.valuei]=ValF(t.value_s.Get()) ; ++t.valuei;
			t.rest_s=Right(t.rest_s.Get(),Len(t.rest_s.Get())-t.n);
			t.n=0;
		}
	}
}

void terrain_water_init ( void )
{
	//  Setup Reflection Camera (range set in visuals)
	CreateCamera (  2  ); BackdropOff (  2 );
	t.terrain.reflsizer=g.greflectionrendersize;
	SetCameraToImage (  2,t.terrain.imagestartindex+6,t.terrain.reflsizer,t.terrain.reflsizer );

	//  Re-make water mask if required
	t.tfilewater_s=g.mysystem.levelBankTestMap_s+"watermask.dds"; //"levelbank\\testmap\\watermask.dds";
	t.terrain.terrainregionupdate=0;
	terrain_refreshterrainmatrix ( );
	terrain_updatewatermask ( );
	terrain_clearterraindirtyregion ( );

	//  Make Water plain
	LoadImage (  "effectbank\\reloaded\\media\\waves2.dds",t.terrain.imagestartindex+7,0,0);//g.gdividetexturesize );
	
	if ( ImageExist(t.terrain.imagestartindex+4) == 0 ) 
	{
		//PE: Just create a 1x1 image for shader , if OLDWATER is used.
		if (g.memskipwatermask == 1) 
		{
			//  blank water mask
			SetCurrentBitmap(g.terrainworkbitmapindex);
			if (FileExist(t.tfilewater_s.Get()) == 1)  DeleteAFile(t.tfilewater_s.Get());
			CLS(Rgb(0, 0, 0));
			GrabImage(t.terrain.imagestartindex + 4, 0, 0, 1, 1);
			//SaveImage(t.tfilewater_s.Get(), t.terrain.imagestartindex + 4);
			SetCurrentBitmap(0);
		}
		else 
		{
			SetMipmapNum(1);
			cstr WaterMask_s = cstr(g.mysystem.levelBankTestMap_s+"watermask.png");
			if ( FileExist(WaterMask_s.Get()) == 0 ) WaterMask_s = cstr(g.mysystem.levelBankTestMap_s+"watermask.dds");
			LoadImage ( WaterMask_s.Get(), t.terrain.imagestartindex + 4, 10, 0);
			SetMipmapNum(-1);
		}
	}
	MakeObjectPlane (  t.terrain.objectstartindex+5,1024*50,1024*50 );
	PositionObject (  t.terrain.objectstartindex+5,1024*25,t.terrain.waterliney_f,1024*25 );
	TextureObject (  t.terrain.objectstartindex+5,0,t.terrain.imagestartindex+7 );
	TextureObject (  t.terrain.objectstartindex+5,1,t.terrain.imagestartindex+5 );
	TextureObject (  t.terrain.objectstartindex+5,2,t.terrain.imagestartindex+6 );
	TextureObject (  t.terrain.objectstartindex+5,3,t.terrain.imagestartindex+4 );
	// now done for editor water earlier
	SetEffectTechnique ( t.terrain.effectstartindex+1, "UseReflection" );
	//if ( GetEffectExist ( t.terrain.effectstartindex+1 ) == 0 )
	//{
	//	LoadEffect (  "effectbank\\reloaded\\water_basic.fx",t.terrain.effectstartindex+1,0 );
	//	t.effectparam.water.HudFogDist=GetEffectParameterIndex(t.terrain.effectstartindex+1,"HudFogDist");
	//	t.effectparam.water.HudFogColor=GetEffectParameterIndex(t.terrain.effectstartindex+1,"HudFogColor");
	//}
	SetObjectEffect (  t.terrain.objectstartindex+5,t.terrain.effectstartindex+1 );
	XRotateObject (  t.terrain.objectstartindex+5,90 );
	SetObjectTransparency (  t.terrain.objectstartindex+5, 5 ); // 021215 - the only object that should have this flag (WATER PLANE)
	SetObjectOcclusion (  t.terrain.objectstartindex+5,0,0,0,0,0 );

	//  set fog settings
	t.tFogNear_f=t.visuals.FogNearest_f ; t.tFogFar_f=t.visuals.FogDistance_f;
	t.tFogR_f=t.visuals.FogR_f ; t.tFogG_f=t.visuals.FogG_f ; t.tFogB_f=t.visuals.FogB_f ; ; t.tFogA_f=t.visuals.FogA_f;
	terrain_water_setfog ( );

	//  hide editor water object
	if (  ObjectExist(t.terrain.objectstartindex+2) == 1 ) 
	{
		HideObject (  t.terrain.objectstartindex+2 );
	}
}

void terrain_water_free ( void )
{
	//  free quick test water and sky effects
	if (  ObjectExist(t.terrain.objectstartindex+5) == 1  )  DeleteObject (  t.terrain.objectstartindex+5 );

	// DX11 cannot delete effect - sort this later with Shader Editor
	SetEffectTechnique ( t.terrain.effectstartindex+1, "Editor" );
	//if (  GetEffectExist(t.terrain.effectstartindex+1) == 1  )  DeleteEffect (  t.terrain.effectstartindex+1 );

	if (  CameraExist(2) == 1  )  DestroyCamera (  2 );
	if (  ImageExist(t.terrain.imagestartindex+6) == 1  )  DeleteImage (  t.terrain.imagestartindex+6 );
	if (  ImageExist(t.terrain.imagestartindex+7) == 1  )  DeleteImage (  t.terrain.imagestartindex+7 );

	//  show editor water plane
	if (  ObjectExist(t.terrain.objectstartindex+2) == 1 ) 
	{
		if ( t.game.gameisexe == 1 )
			HideObject (  t.terrain.objectstartindex+2 );
		else
			ShowObject (  t.terrain.objectstartindex+2 );
	}
}

void terrain_updatewatermechanism ( void )
{
	//  water visiblity
	if ( t.hardwareinfoglobals.nowater == 0 ) lua_showwater ( );
	if ( t.hardwareinfoglobals.nowater != 0 ) { t.twf=t.hardwareinfoglobals.nowater ; lua_hidewater() ; t.hardwareinfoglobals.nowater=t.twf; }
	//  water physics
//terrain_updatewaterphysics ( );
return;

}

void terrain_updatewaterphysics ( void )
{
	// water physics
	if ( g.gphysicssessionactive == 1 ) 
	{
		if ( t.hardwareinfoglobals.nowater == 0 && t.visuals.reflectionmode>0 ) 
		{
			ODESetWaterLine ( t.terrain.waterliney_f - 20.0f );// 480.0 );
		}
		else
		{
			ODESetWaterLine ( -10000.0 );
		}
	}
}

void terrain_water_setfog ( void )
{
	//  takes; tFogR#,tFogG#,tFogB#,tFogNear#,tFogFar#
	if ( GetEffectExist(t.terrain.effectstartindex+1) ) 
	{
		SetVector4 (  g.terrainvectorindex,t.tFogR_f/255.0,t.tFogG_f/255.0,t.tFogB_f/255.0,t.tFogA_f/255.0 );
		SetEffectConstantVEx (  t.terrain.effectstartindex+1,t.effectparam.water.HudFogColor,g.terrainvectorindex );
		SetVector4 (  g.terrainvectorindex,t.tFogNear_f,t.tFogFar_f,0,0 );
		SetEffectConstantVEx (  t.terrain.effectstartindex+1,t.effectparam.water.HudFogDist,g.terrainvectorindex );
	}
}

void terrain_water_loop ( void )
{
	if (g.globals.forcenowaterreflection == 1) {
		t.visuals.reflectionmode = 1; //PE:
		t.visuals.reflectionmodepixelsrendered = 1;
	}

	// Adjust reflective processing based on actual number of water pixels in final scene
	t.visuals.reflectionmodepixelsrendered=0;
	if ( t.visuals.reflectionmode>0 ) 
	{
		if ( ObjectExist(t.terrain.objectstartindex+5) == 1 ) 
		{
			// DX11 had the query removed so cannot count pixels rendered (would also fix 'one cycle' no reflection issue)
			t.visuals.reflectionmodepixelsrendered=1;//GetObjectOcclusionValue(t.terrain.objectstartindex+5);
			if ( t.visuals.reflectionmodepixelsrendered>0 ) 
			{
				if ( t.visuals.reflectionmodemodified == 0 ) 
				{
					t.visuals.reflectionmodemodified=t.visuals.reflectionmode;
					visuals_updateobjectmasks ( );
				}
			}
			else
			{
				if ( t.visuals.reflectionmodemodified>0 ) 
				{
					t.visuals.reflectionmodemodified=0;
					visuals_updateobjectmasks ( );
				}
			}
		}
	}

	// Update Water plain
	if ( ObjectExist(t.terrain.objectstartindex+5) == 1 && t.visuals.reflectionmodepixelsrendered>0 ) 
	{
		PositionObject ( t.terrain.objectstartindex+5,ObjectPositionX(t.terrain.objectstartindex+5),t.terrain.waterliney_f,ObjectPositionZ(t.terrain.objectstartindex+5) );
		t.terrain.WaterCamY_f=CameraPositionY()-t.terrain.waterliney_f;

		// Refraction camera (looks bad with stuff floating in it)
		HideObject (  t.terrain.objectstartindex+5 );

		// Reflection camera
		if ( t.visuals.reflectionmode>1 ) 
		{
			// if in VR mode, lef/right eye math messy, so dont change reflected sky angle for clean render
			if ( g.gvrmode != 0 )//&& CameraExist(6) == 1 ) 
			{
				// special render technique for terrain reflection
				SetEffectTechnique ( t.terrain.effectstartindex+1, "UseReflectionNoSky" );
			}
			else
			{
				// special render technique for terrain reflection
				SetEffectTechnique ( t.terrain.effectstartindex+1, "UseReflection" );
			}

			// set sky position to be relative to reflection render
			PositionObject (  t.terrain.objectstartindex+4,CameraPositionX(),t.terrain.waterliney_f-t.terrain.WaterCamY_f,CameraPositionZ() );
			if (  ObjectExist(t.terrain.objectstartindex+8) == 1  )  PositionObject (  t.terrain.objectstartindex+8,CameraPositionX(),t.terrain.waterliney_f-t.terrain.WaterCamY_f,CameraPositionZ() );
			PositionCamera (  2,CameraPositionX(),t.terrain.waterliney_f-t.terrain.WaterCamY_f,CameraPositionZ() );

			if (g.luacameraoverride == 2 || g.luacameraoverride == 3) 
			{
				//PE: Why ohh why is Z negative when using g.luacameraoverride=3 || 2 ???????
				//PE: ? ? ? a matrix problem somewhere ? ? ?
				RotateCamera(2, -CameraAngleX(0), CameraAngleY(0), -CameraAngleZ(0));
			}
			else 
			{
				RotateCamera(2, -CameraAngleX(0), CameraAngleY(0), CameraAngleZ(0));
			}
			// only render terrain/objects if mode allows
			if ( t.visuals.reflectionmode>25 ) 
			{
				// special render technique for terrain reflection
				SetEffectTechnique ( t.terrain.effectstartindex+1, "UseReflection" );

				// set sky position to be relative to reflection render
				PositionObject ( t.terrain.objectstartindex+4,CameraPositionX(),t.terrain.waterliney_f-t.terrain.WaterCamY_f,CameraPositionZ() );
				if ( ObjectExist(t.terrain.objectstartindex+8) == 1 ) PositionObject ( t.terrain.objectstartindex+8,CameraPositionX(),t.terrain.waterliney_f-t.terrain.WaterCamY_f,CameraPositionZ() );

				PositionCamera ( 2,CameraPositionX(),t.terrain.waterliney_f-t.terrain.WaterCamY_f,CameraPositionZ() );
				RotateCamera ( 2,-CameraAngleX(),CameraAngleY(),CameraAngleZ() );

				// only render terrain/objects if mode allows
				if (t.visuals.reflectionmode > 25)
				{
					// full reflection mode renders terrain
					if ( t.terrain.WaterCamY_f>0 ) 
					{
						SetCameraClip ( 2,1,0,t.terrain.waterliney_f-0.0,0,0,1,0 );
					}
					else
					{
						SetCameraClip ( 2,1,0,t.terrain.waterliney_f+50.0,0,0,-1,0 );
					}
					// if terrain exists, render it
					if ( t.terrain.TerrainID>0 ) 
					{
						BT_SetCurrentCamera ( 2 );
						BT_SetTerrainLODDistance ( t.terrain.TerrainID,1,700.0 );
						BT_SetTerrainLODDistance ( t.terrain.TerrainID,2,701.0 );
						BT_UpdateTerrainLOD ( t.terrain.TerrainID );
						BT_UpdateTerrainCull ( t.terrain.TerrainID );
						BT_RenderTerrain ( t.terrain.TerrainID );
						BT_SetCurrentCamera (  0 );
						BT_SetTerrainLODDistance ( t.terrain.TerrainID,1,1401.0+t.visuals.TerrainLOD1_f );
						BT_SetTerrainLODDistance ( t.terrain.TerrainID,2,1401.0+t.visuals.TerrainLOD2_f );
					}
				}
				else
				{
					// no terrain in reflection render
				}
				if (t.visuals.reflectionmode == 1) 
				{
					//PE: Special mode that only clear the reflection image mainly for underwater.
					SetCameraClip(2, 1, 0, t.terrain.waterliney_f - 100000.0, 0, 0, -1, 0);
				}
				SyncMask ( 1<<2 );

				// simpler terrain for reflection render
				if ( GetEffectExist(t.terrain.terrainshaderindex) == 1  )  SetEffectTechnique (  t.terrain.terrainshaderindex, "ReflectedOnly" );
				FastSync ( );
				// restore terrain shader technique
				if ( GetEffectExist(t.terrain.terrainshaderindex) == 1  ) visuals_shaderlevels_terrain_update ( );
				// restore sky position to main camera
				PositionObject ( t.terrain.objectstartindex+4,CameraPositionX(),CameraPositionY(),CameraPositionZ() );
				if ( ObjectExist(t.terrain.objectstartindex+8) == 1  )  PositionObject (  t.terrain.objectstartindex+8,CameraPositionX(),CameraPositionY(),CameraPositionZ() );
			}
			else { //PE: This is also needed in Classic.
				//PE: we need to restore skymap for normal test game if reflectionmode > 1 && reflectionmode <= 25
				PositionObject(t.terrain.objectstartindex + 4, CameraPositionX(), CameraPositionY(), CameraPositionZ());
				if (ObjectExist(t.terrain.objectstartindex + 8) == 1)  PositionObject(t.terrain.objectstartindex + 8, CameraPositionX(), CameraPositionY(), CameraPositionZ());
			}
		}
		else
		{
			SetEffectTechnique ( t.terrain.effectstartindex+1, "NoReflection" );
		}
	}

	// Restore regular rendering
	SyncMask ( 0xfffffff9 );

	// Show water again for main rendering
	if ( t.hardwareinfoglobals.nowater == 0 ) 
	{
		if ( ObjectExist(t.terrain.objectstartindex+5) == 1 ) 
		{
			ShowObject ( t.terrain.objectstartindex+5 );
		}
	}
}

/* moved to grass module

// 
//  VEGEATATION SYSTEM
// 

void grass_assignnewshader ( void )
{
	// Choose the vegetation shader to use
	//if ( g.gpbroverride == 1 ) // t.terrain.iTerrainPBRMode == 1 )
	//	t.terrain.vegetationshaderindex = t.terrain.effectstartindex+6;
	//else
	//	t.terrain.vegetationshaderindex = t.terrain.effectstartindex+2;
	t.terrain.vegetationshaderindex = t.terrain.effectstartindex+2;

	// Veg shader constants
	memset ( &t.effectparam.vegetation, 0, sizeof(t.effectparam.vegetation) );
	if ( GetEffectExist ( t.terrain.vegetationshaderindex ) == 1 )
	{
		t.effectparam.vegetation.g_lights_data=GetEffectParameterIndex(t.terrain.vegetationshaderindex,"g_lights_data");
		t.effectparam.vegetation.g_lights_pos0=GetEffectParameterIndex(t.terrain.vegetationshaderindex,"g_lights_pos0");
		t.effectparam.vegetation.g_lights_atten0=GetEffectParameterIndex(t.terrain.vegetationshaderindex,"g_lights_atten0");
		t.effectparam.vegetation.g_lights_diffuse0=GetEffectParameterIndex(t.terrain.vegetationshaderindex,"g_lights_diffuse0");
		t.effectparam.vegetation.g_lights_pos1=GetEffectParameterIndex(t.terrain.vegetationshaderindex,"g_lights_pos1");
		t.effectparam.vegetation.g_lights_atten1=GetEffectParameterIndex(t.terrain.vegetationshaderindex,"g_lights_atten1");
		t.effectparam.vegetation.g_lights_diffuse1=GetEffectParameterIndex(t.terrain.vegetationshaderindex,"g_lights_diffuse1");
		t.effectparam.vegetation.g_lights_pos2=GetEffectParameterIndex(t.terrain.vegetationshaderindex,"g_lights_pos2");
		t.effectparam.vegetation.g_lights_atten2=GetEffectParameterIndex(t.terrain.vegetationshaderindex,"g_lights_atten2");
		t.effectparam.vegetation.g_lights_diffuse2=GetEffectParameterIndex(t.terrain.vegetationshaderindex,"g_lights_diffuse2");
		t.effectparam.vegetation.SpotFlashPos=GetEffectParameterIndex(t.terrain.vegetationshaderindex,"SpotFlashPos");
		t.effectparam.vegetation.SpotFlashColor=GetEffectParameterIndex(t.terrain.vegetationshaderindex,"SpotFlashColor");

		// wipe any previous param storage
		ResetEffect ( t.terrain.vegetationshaderindex );
	}
}

void grass_applyshader ( void )
{
	// Choose the vegetation shader to use
	grass_assignnewshader();

	// Apply veg shader to all veg objects
	UpdateGrassTexture ( g.gpbroverride );//t.terrain.iTerrainPBRMode );
	UpdateGrassShader ( t.terrain.vegetationshaderindex );
}

void grass_init ( void )
{
	//  init our resource values
	t.tGrassObj=t.terrain.objectstartindex+6;
	t.tGrassImg=t.terrain.imagestartindex+8;
	t.tObjectGridStart = t.terrain.objectstartindex + 6201;
	t.terrain.vegetationshaderindex=t.terrain.effectstartindex+2;
	t.tVegShadowTex = t.terrain.imagestartindex+2;
	t.tTempMesh = t.terrain.objectstartindex+7;

	// load shaders for non-PBR and PBR vegetation
	if ( GetEffectExist ( t.terrain.effectstartindex+2 ) == 0 )
	{
		//LoadEffect (  "effectbank\\reloaded\\apbr_veg.fx", t.terrain.effectstartindex+6, 0 );
		if ( g.gpbroverride == 1 )
			LoadEffect (  "effectbank\\reloaded\\apbr_veg.fx", t.terrain.effectstartindex+2, 0 );
		else
			LoadEffect (  "effectbank\\reloaded\\vegetation_basic.fx", t.terrain.effectstartindex+2, 0 );
	}

	// PBR Support textures
	int iPBRAGEDImg = t.terrain.imagestartindex+14;
	int iPBRSpecImg = t.terrain.imagestartindex+15;
	int iPBRCubeImg = t.terrain.imagestartindex+31;
	int iPBRCurveImg = t.terrain.imagestartindex+32;
	if ( ImageExist ( t.terrain.imagestartindex+14 ) == 0 ) LoadImage ( "effectbank\\reloaded\\media\\AGED.png",t.terrain.imagestartindex+14 );
	if ( ImageExist ( t.terrain.imagestartindex+15 ) == 0 ) LoadImage ( "effectbank\\reloaded\\media\\blank_black.dds",t.terrain.imagestartindex+15 );
	SetPBRResourceValues ( iPBRAGEDImg, iPBRSpecImg, iPBRCubeImg, iPBRCurveImg );

	//  Pass the veg system the following initialisation values first. These are just resource numbers and numerical settings;
	//  1. Grass object/mesh ID for base grass model
	//  2. Start object for grid of grass models (ensure grid size doesnt exist available object numbers when x2)
	//  3. Grass texture
	//  4. Shadow texture
	//  5. A temporary mesh used for building grass objects
	//  6. Shader ID
	//  7. Grass existance memblock
	//  8. Memblock width/height
	//  9. Camera mask for veg objects
	//C++ ISSUE %001 replaced with 1 - should be fine since %001 in binary is 1 in dec
	SetResourceValues (  t.tGrassObj,t.tObjectGridStart,t.tGrassImg,t.tVegShadowTex,t.tTempMesh,t.terrain.vegetationshaderindex,t.terrain.grassmemblock,MAXTEXTURESIZE, 1 );

	//  now load our grass mesh
	if (  ObjectExist(t.tGrassObj)  ==  1  )  DeleteObject (  t.tGrassObj );
	if (  GetMeshExist(t.tGrassObj)  ==  1  )  DeleteMesh (  t.tGrassObj );

	#ifdef WICKEDENGINE
	cstr pFileToLoad = cstr("vegbank\\veg.dbo");
	#else
	cstr pFileToLoad = cstr(cstr("vegbank\\")+g.vegstyle_s+"\\veg.DBO");
	if ( FileExist ( pFileToLoad.Get() ) == 0 ) pFileToLoad = cstr(cstr("vegbank\\")+g.vegstyle_s+"\\veg.X");
	#endif
	LoadObject ( pFileToLoad.Get(),t.tGrassObj );

	ScaleObject (  t.tGrassObj,200,200,200 );
	MakeMeshFromObject (  t.tGrassObj,t.tGrassObj );
	DeleteObject (  t.tGrassObj );

	//  load our grass piece image
	grass_setgrassimage ( );

	// apply correct shader
	grass_applyshader ();

	//  if the user has updated the grass bitmap in the editor and the init function has been called, we are testing a level
	//  and a new grass memblock file needs to be made from the bitmap. Otherwise there should be one to load
	t.tfileveggrass_s=g.mysystem.levelBankTestMap_s+"vegmaskgrass.dat"; //"levelbank\\testmap\\vegmaskgrass.dat";
	if (  t.terrain.grassregionx1 != t.terrain.grassregionx2 || t.terrain.grassregionupdate == 2 || FileExist(t.tfileveggrass_s.Get()) == 0 ) 
	{
		grass_updategrassfrombitmap ( );
	}
	else
	{
		grass_loadgrass ( );
	}

	//  calculate how big our grid of vegetation is
	grass_setgrassgridandfade ( );

	//  now create and setup all of our vegetation objects. This will delete any veg that already exists. Passing;
	//  1. Number of grass items per veg object
	//  2. Width of grass
	//  3. Height of grass
	//  4. Size of each veg area
	//  5. Dimension of the veg grid
	if (  t.terrain.superflat == 1 ) 
	{
		t.tTerrainID = 0;
	}
	else
	{
		t.tTerrainID = t.terrain.TerrainID;
	}
	int iTrimUsingGrassMemblock = 0;
	if ( t.game.gameisexe == 1 ) iTrimUsingGrassMemblock = t.terrain.grassmemblock;
	if( g.usegrassbelowwater > 0)
		MakeVegetationGrid(4.0f*t.visuals.VegQuantity_f, t.visuals.VegWidth_f, t.visuals.VegHeight_f, terrain_veg_areawidth, t.terrain.vegetationgridsize, t.tTerrainID, iTrimUsingGrassMemblock , true );
	else
		MakeVegetationGrid( 4.0f*t.visuals.VegQuantity_f,t.visuals.VegWidth_f,t.visuals.VegHeight_f,terrain_veg_areawidth,t.terrain.vegetationgridsize,t.tTerrainID, iTrimUsingGrassMemblock , false );
}

void grass_setgrassimage ( void )
{
	SetImageAutoMipMap (  1 );
	t.tGrassImg = t.terrain.imagestartindex+8;
	if ( ImageExist(t.tGrassImg) == 1 ) DeleteImage ( t.tGrassImg );
	if ( g.gdividetexturesize == 0 ) 
	{
		LoadImage ( "effectbank\\reloaded\\media\\white_D.dds",t.tGrassImg );
	}
	else
	{
		LoadImage ( cstr(cstr("vegbank\\")+g.vegstyle_s+"\\grass.dds").Get(), t.tGrassImg );
	}
	UpdateGrassTexture ( g.gpbroverride );
	SetImageAutoMipMap ( 0 );
}

void grass_setgrassgridandfade ( void )
{
	#ifdef VRTECH
	t.terrain.vegetationgridsize = t.visuals.vegetationmode * 0.3;
	#else
	t.terrain.vegetationgridsize = (20+(t.visuals.vegetationmode*0.8f)) * 0.3;
	#endif
	t.tGrassFadeDistance = terrain_veg_areawidth * (t.terrain.vegetationgridsize/2 - 1);
}

void grass_loop ( void )
{
	//  early exit if no veg used
	if (  t.visuals.vegetationmode == 0  )  return;

	//  if in superflat mode we call a version of update which is much faster
	if (  t.terrain.superflat == 1 ) 
	{
			UpdateSuperFlat (  CameraPositionX(0),CameraPositionZ(0),TERRAIN_SUPERFLAT_HEIGHT,g.postprocessimageoffset+5 );
	}
	else
	{

		//  do we need to update after an F9 terrain raise?
		if (  t.terrain.grassupdateafterterrain  ==  1 ) 
		{
			t.tRegionX1 = t.terrain.grassregionx1 * 50;
			t.tRegionX2 = t.terrain.grassregionx2 * 50;
			t.tRegionZ1 = t.terrain.grassregionz1 * 50;
			t.tRegionZ2 = t.terrain.grassregionz2 * 50;

			//  only update when MouseClick (  isn't pressed or cursor moves, to reduce chug slowdown )
			if (  MouseClick()  ==  0 || t.tRegionX1  !=  t.terrain.lastgrassupdatex1 || t.tRegionX2  !=  t.terrain.lastgrassupdatex2 || t.tRegionZ1  !=  t.terrain.lastgrassupdatez1 || t.tRegionZ2  !=  t.terrain.lastgrassupdatez2 ) 
			{
				UpdateVegZoneBlitzTerrain (  t.tRegionX1,t.tRegionZ1,t.tRegionX2,t.tRegionZ2,t.terrain.TerrainID );
				t.terrain.lastgrassupdatex1 = t.tRegionX1;
				t.terrain.lastgrassupdatex2 = t.tRegionX2;
				t.terrain.lastgrassupdatez1 = t.tRegionZ1;
				t.terrain.lastgrassupdatez2 = t.tRegionZ2;
				t.terrain.grassupdateafterterrain = 0;
			}
		}

		//  main terrain update
		if (  t.hardwareinfoglobals.nograss == 0 ) 
		{
			UpdateBlitzTerrain (  CameraPositionX(0),CameraPositionZ(0),t.terrain.TerrainID,g.postprocessimageoffset+5 );
		}

	}
}

