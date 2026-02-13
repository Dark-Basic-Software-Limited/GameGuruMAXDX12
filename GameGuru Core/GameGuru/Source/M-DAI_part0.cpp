//----------------------------------------------------
//--- GAMEGURU - M-DAI
//----------------------------------------------------

#include "stdafx.h"
#include "gameguru.h"

// prototypes to link with entity avoidance
void entity_lua_moveforward_core_nooverlap ( int te, float* pNX, float* pNZ );

// 
//  Dark Dave's A.I System
// 

void darkai_init ( void )
{
	//  Init AI
	AIStart( g.guishadereffectindex ); 
	AISetRadius ( t.aisystem.obstacleradius  ); 
	t.aisystem.on=1;

	//  Generate OBS by default
	t.aisystem.generateobs=1;

	//  Used in LUA script to disable components in-play
	t.aisystem.processlogic=1;
	t.aisystem.processplayerlogic=1;

	//  Initial A.I System defaults
	t.aisystem.defaulteyeheight_f=65.0;
	t.aisystem.defaulteyehalfheight_f=t.aisystem.defaulteyeheight_f/2.0;
	t.aisystem.currenteyeheight_f=t.aisystem.defaulteyeheight_f;

	//  Generate character debug object (capsule with cone nose)
	if (  GetMeshExist(t.aisystem.debugentitymesh) == 0 ) 
	{
		MakeObjectCone (  t.aisystem.debugentityworkobj,33 );
		MakeObjectSphere (  t.aisystem.debugentityworkobj2,33  ); MakeMeshFromObject (  t.aisystem.debugentitymesh,t.aisystem.debugentityworkobj2  ); DeleteObject (  t.aisystem.debugentityworkobj2 );
		MakeObjectCylinder (  t.aisystem.debugentityworkobj2,65  ); MakeMeshFromObject (  t.aisystem.debugentitymesh2,t.aisystem.debugentityworkobj2  ); DeleteObject (  t.aisystem.debugentityworkobj2 );
		OffsetLimb (  t.aisystem.debugentityworkobj,0,0,10,-t.aisystem.defaulteyeheight_f  ); ScaleLimb (  t.aisystem.debugentityworkobj,0,50,50,50 );
		AddLimb (  t.aisystem.debugentityworkobj,1,t.aisystem.debugentitymesh  ); OffsetLimb (  t.aisystem.debugentityworkobj,1,0,0,-t.aisystem.defaulteyeheight_f  ); ScaleLimb (  t.aisystem.debugentityworkobj,1,50,50,50 );
		AddLimb (  t.aisystem.debugentityworkobj,2,t.aisystem.debugentitymesh2  ); OffsetLimb (  t.aisystem.debugentityworkobj,2,0,0,t.aisystem.defaulteyeheight_f/-2.0  ); ScaleLimb (  t.aisystem.debugentityworkobj,2,15,100,15  ); RotateLimb (  t.aisystem.debugentityworkobj,2,-90,0,0 );
		XRotateObject (  t.aisystem.debugentityworkobj,90  ); FixObjectPivot (  t.aisystem.debugentityworkobj );
		MakeMeshFromObject (  t.aisystem.debugentitymesh,t.aisystem.debugentityworkobj );
		DeleteObject (  t.aisystem.debugentityworkobj );
	}

	//  Associate 'DarkA.I Player' with unique player object
	if (  ObjectExist(t.aisystem.objectstartindex) == 1  )  DeleteObject (  t.aisystem.objectstartindex );
	MakeObject (  t.aisystem.objectstartindex,t.aisystem.debugentitymesh,0 );
	if (  t.aisystem.usingphysicsforai == 1 ) 
	{
		OffsetLimb (  t.aisystem.objectstartindex,0,0,ObjectSizeY(t.aisystem.objectstartindex)/-2,0 );
	}
	HideObject (  t.aisystem.objectstartindex );
	PositionObject (  t.aisystem.objectstartindex,(512*50),1000,(512*50) );
	AIAddPlayer (  t.aisystem.objectstartindex );
	AISetPlayerContainer (  0 );

	//  new DarkA.I player height system where 'defaulteyehalfheight#' is difference between stood and ducked height
	AISetPlayerHeight (  t.aisystem.defaulteyehalfheight_f );

	// 061115 - reset smoothanim array so no carryover of transitions to new test game
	darkai_resetsmoothanims();
}

void darkai_resetsmoothanims ( void )
{
	for ( int n = 0; n < t.tmaxobjectnumber; n++ )
	{
		t.smoothanim[n].fn = 0;
		t.smoothanim[n].playflag = 0;
		t.smoothanim[n].playstarted = 0;
		t.smoothanim[n].rev = 0;
		t.smoothanim[n].st = 0;
		t.smoothanim[n].transition = 0;
	}
}

void darkai_free ( void )
{
	//  Ensure any debug mode is zero to zero when leave test game
	if (  t.visuals.debugvisualsmode>0 ) 
	{
		t.visuals.debugvisualsmode=0;
	}

	//  Debug view of all obstacles on the ground Floor (  )
	if (  t.aisystem.usingdebugobjects == 1 ) 
	{
		t.aisystem.usingdebugobjects=0;
		AIDebugHideObstacleBounds ( -1 );
		AIDebugHidePaths (  );
		AIDebugHideViewArcs (  );
		//AIDebugHideSounds (  ); // 150416 - froze gun and character animations
	}

	//  free A.I resources (and 1 to stop thread running)
	AIReset ( 1 ); 
	t.aisystem.on=0 ; t.aisystem.generateobs=1;
}

void darkai_preparedata ( void )
{
	// Create obstacles, paths and finalise this data (done after all data loaded and ready to use)
	t.aisystem.obs=0;

	// If MAP.OBS file exists, load instead of generate
	t.tobsfile_s=g.mysystem.levelBankTestMap_s+"map.obs"; //"levelbank\\testmap\\map.obs";
	if ( FileExist(t.tobsfile_s.Get()) == 1 ) 
	{
		// NOTE: gridedit_changemodifiedflag deletes this file if static entity altered
	
		// load from file (standalone or last level state with no changes)
		timestampactivity(0,"_darkai_loadobstacles");
		darkai_loadobstacles ( );

		// does not generate new OBS from static entities but completes after that process
		t.aisystem.generateobs=0;
	}

	// Terrain Obstacles (requires terrain height data loaded)
	t.terrain.terrainregionupdate=0;
	terrain_refreshterrainmatrix ( );
	if ( t.game.runasmultiplayer == 1 ) mp_refresh ( );
	if ( t.aisystem.generateobs == 1 ) 
	{
		timestampactivity(0,"darkai_obstacles_terrain");
		if ( t.game.runasmultiplayer == 1 ) mp_refresh ( );
	}

	// Setup all AI paths and AI containers(navmeshes) via waypoints data
	DWORD dwAIPathCount = 1; // 20316 - path indices must be sequential
	DWORD dwContainerObstacleIndex = 100000;
	timestampactivity(0,"darkai_paths");
	for ( t.twaypointindex = 1 ; t.twaypointindex <= g.waypointmax; t.twaypointindex++ )
	{
		// if waypoint, create path in AI system
		if ( t.waypoint[t.twaypointindex].style == 1 ) 
		{
			if ( t.waypoint[t.twaypointindex].count>0 ) 
			{
				AIMakePath ( dwAIPathCount );
				for ( t.w = t.waypoint[t.twaypointindex].start ; t.w<=  t.waypoint[t.twaypointindex].finish; t.w++ )
				{
					AIPathAddPoint ( dwAIPathCount,t.waypointcoord[t.w].x,t.waypointcoord[t.w].y,t.waypointcoord[t.w].z,0 );
				}
				dwAIPathCount++;
			}
		}

		// if navmeshzone, create enclosed obstacle (inverted obstacle will only allow paths WITHIN it)
		if ( t.waypoint[t.twaypointindex].style == 3 ) 
		{
			if ( t.waypoint[t.twaypointindex].count>0 ) 
			{
				// by creating plots backwards, we create a containment rather than an obstacle
				DWORD dwAIContainerIDs = t.twaypointindex;
				AIAddContainer ( dwAIContainerIDs );
				int iContainerID = dwAIContainerIDs, iFullHeight = 1, iViewBlocker = 0;
				AIStartNewObstacle ( dwContainerObstacleIndex );
				for ( t.w = t.waypoint[t.twaypointindex].finish-1; t.w >= t.waypoint[t.twaypointindex].start; t.w-- )
				{
					AIAddObstacleVertex ( t.waypointcoord[t.w].x,t.waypointcoord[t.w].z );
				}
				AIEndNewObstacle ( iContainerID, iFullHeight, iViewBlocker );
				dwContainerObstacleIndex++;
			}
		}
	}
	if ( t.game.runasmultiplayer == 1 ) mp_refresh ( );

	//  Setup manual cover positions from cover marker entities
	for ( t.e = 1 ; t.e <= g.entityelementlist; t.e++ )
	{
		if ( t.entityelement[t.e].bankindex>0 ) 
		{
			if (  t.entityprofile[t.entityelement[t.e].bankindex].ismarker == 9 ) 
			{
				t.ttx_f=t.entityelement[t.e].x;
				t.tty_f=t.entityelement[t.e].y;
				t.ttz_f=t.entityelement[t.e].z;
				t.tta_f=t.entityelement[t.e].ry;
				LPSTR pIfUsedString = t.entityelement[t.e].eleprof.ifused_s.Get();
				AIAddCoverPoint ( t.ttx_f, t.tty_f, t.ttz_f, t.tta_f, pIfUsedString );
			}
		}
	}
	if ( t.game.runasmultiplayer == 1 ) mp_refresh ( );
}

void darkai_completeobstacles ( void )
{
	// OLD - still need to complete obstacles, even if loaded data in
	// 060917 - don't regenerate waypoints, etc - data has been loaded and is ready
	if ( t.aisystem.generateobs == 1 ) 
	{
		if ( t.game.runasmultiplayer == 1 ) mp_refresh ( );
		AICompleteObstacles (  );
	}

	//  when ALL obstacles done, can calculate paths
	if (  t.aisystem.generateobs == 1 ) 
	{
		//  after creating obstacles, save into file
		if ( t.game.runasmultiplayer == 1 ) mp_refresh ( );
		darkai_saveobstacles ( );
	}

	// Add additional container connections to scene
	timestampactivity(0,"darkai_zoneconnections");
	for ( int thiswaypointindex = 1 ; thiswaypointindex <= g.waypointmax; thiswaypointindex++ )
	{
		// if waypoint, create path in AI system
		if ( t.waypoint[thiswaypointindex].style == 1 ) 
		{
			if ( t.waypoint[thiswaypointindex].count>0 ) 
			{
				// create connecting paths between containers
				int iLastContainerStartIndex = -1;
				float fLastPointX = 0, fLastPointZ = 0;
				for ( int tw = t.waypoint[thiswaypointindex].start; tw <= t.waypoint[thiswaypointindex].finish; tw++ )
				{
					// which container is it in
					int iContainerStartIndex = 0;
					for ( t.waypointindex = 1; t.waypointindex <= g.waypointmax; t.waypointindex++ )
					{
						if ( waypoint_ispointinzoneex ( t.waypointindex, t.waypointcoord[tw].x, t.waypointcoord[tw].y, t.waypointcoord[tw].z, 1 ) == 1 ) 
						{
							iContainerStartIndex = t.waypointindex;
							break;
						}
					}

					// going to this point (see below)
					t.tpointx_f = t.waypointcoord[tw].x;
					t.tpointz_f = t.waypointcoord[tw].z;

					// create connection between containers
					if ( iLastContainerStartIndex != -1 )
					{
						// if points cross container barrier
						if ( iContainerStartIndex != iLastContainerStartIndex )
						{
							// create a connection between the points
							AIConnectContainers ( iLastContainerStartIndex, fLastPointX, fLastPointZ, iContainerStartIndex, t.tpointx_f, t.tpointz_f );
						}
					}

					// record for next cycle
					iLastContainerStartIndex = iContainerStartIndex;
					fLastPointX = t.tpointx_f;
					fLastPointZ = t.tpointz_f;
				}
			}
		}
	}
}

void darkai_invalidateobstacles ( void )
{
	// Remove container zero obstacles file
	if ( t.tignoreinvalidateobstacles == 0 ) 
	{
		t.tobsfile_s = g.mysystem.levelBankTestMapAbs_s+"map.obs"; //g.fpscrootdir_s+"\\Files\\levelbank\\testmap\\map.obs";
		if ( FileExist(t.tobsfile_s.Get()) == 1 ) DeleteAFile ( t.tobsfile_s.Get() );
		t.aisystem.generateobs=1;
	}
}

void darkai_saveobstacles ( void )
{
	// Save container zero obstacles
	t.tobsfile_s=g.mysystem.levelBankTestMap_s+"map.obs"; //"levelbank\\testmap\\map.obs";
	if ( FileExist(t.tobsfile_s.Get()) == 1 ) DeleteAFile ( t.tobsfile_s.Get() );
	AISaveObstacleData ( 0, t.tobsfile_s.Get() );
	t.aisystem.generateobs=0;
}

void darkai_loadobstacles ( void )
{
	// Load container zero obstacles
	t.tobsfile_s=g.mysystem.levelBankTestMap_s+"map.obs"; //"levelbank\\testmap\\map.obs";
	if ( FileExist(t.tobsfile_s.Get()) == 1 ) 
	{
		AILoadObstacleData ( 0, t.tobsfile_s.Get() );
	}
}

int darkai_finddoorcontainer ( int iObj )
{
	int iContainerID = 0;
	for ( int thiswaypointindex = 1 ; thiswaypointindex <= g.waypointmax; thiswaypointindex++ )
	{
		// this is now done inside waypoint_ispointinzoneex
		//if ( t.waypoint[thiswaypointindex].style == 3 ) 
		//{
		// this is now done inside waypoint_ispointinzoneex
		//if ( t.waypoint[thiswaypointindex].count>0 ) 
		//{
		if ( waypoint_ispointinzoneex ( thiswaypointindex, ObjectPositionX(iObj), ObjectPositionY(iObj), ObjectPositionZ(iObj), 1 ) == 1 )
		{
			iContainerID = thiswaypointindex;
			break;
		}
		//}
		//}
	}
	return iContainerID;
}

void darkai_adddoor ( void )
{
	// gate dimensions for now
	if ( t.tobj>0 ) 
	{
		if ( ObjectExist(t.tobj) == 1 ) 
		{
			// maximum bounds of this object at any Y rotation creates a dynamic obstacle for DarkAI
			t.tsx_f=ObjectSizeX(t.tobj,1)/2;
			t.tsz_f=ObjectSizeZ(t.tobj,1)/2;
			if ( t.tsx_f<t.tsz_f ) t.tsx_f = t.tsz_f;
			if ( t.tsz_f<t.tsx_f ) t.tsz_f = t.tsx_f;
			t.tx1_f=ObjectPositionX(t.tobj)-t.tsx_f;
			t.tz1_f=ObjectPositionZ(t.tobj)-t.tsz_f;
			t.tx2_f=ObjectPositionX(t.tobj)+t.tsx_f;
			t.tz2_f=ObjectPositionZ(t.tobj)+t.tsz_f;

			// use collision center to plot accurate bounds of door
			t.tx1_f = ObjectPositionX ( t.tobj ) + GetObjectCollisionCenterX ( t.tobj );
			t.tz1_f = ObjectPositionZ ( t.tobj ) + GetObjectCollisionCenterZ ( t.tobj );
			t.tx2_f = t.tx1_f + t.tsx_f;
			t.tz2_f = t.tz1_f + t.tsz_f;
			t.tx1_f = t.tx1_f - t.tsx_f;
			t.tz1_f = t.tz1_f - t.tsz_f;

			// work out which zone (container) this door resides on(in)
			int iContainerID = darkai_finddoorcontainer ( t.tobj );

			// finally add the blocking door element
			AIAddDoor ( t.tobj, iContainerID, t.tx1_f,t.tz1_f,t.tx2_f,t.tz2_f );
		}
	}
}

void darkai_removedoor ( void )
{
	if ( t.tobj>0 ) 
	{
		if ( ObjectExist(t.tobj) == 1 ) 
		{
			int iContainerID = darkai_finddoorcontainer ( t.tobj );
			AIRemoveDoor ( t.tobj, iContainerID );
		}
	}
}

void darkai_createinternaldebugvisuals ( void )
{
	if (  t.aisystem.usingphysicsforai == 1 ) 
	{
		if (  t.terrain.TerrainID>0 ) 
		{
			t.debuglayerheight_f=BT_GetGroundHeight(t.terrain.TerrainID,t.terrain.playerx_f,t.terrain.playerz_f)+15;
		}
		else
		{
			t.debuglayerheight_f=g.gdefaultterrainheight+15;
		}
	}
	else
	{
		t.debuglayerheight_f=15;
	}
	AIDebugShowObstacleBounds ( -1, t.debuglayerheight_f );
	AIDebugShowPaths ( t.debuglayerheight_f );
	AIDebugShowViewArcs ( t.debuglayerheight_f );
	//AIDebugShowSounds ( t.debuglayerheight_f ); // 150416 - froze gun and character animations
	//AIDebugShowAvoidanceAngles ( t.debuglayerheight_f );
}

void darkai_destroyinternaldebugvisuals ( void )
{
	AIDebugHideObstacleBounds ( -1 );
	AIDebugHidePaths (  );
	AIDebugHideViewArcs (  );
	//AIDebugHideSounds (  ); // 150416 - froze gun and character animations
}

void darkai_updatedebugobjects ( void )
{
	//  Debug view of all obstacles on the ground Floor (  )
	if (  t.aisystem.on == 1 ) 
	{
		if (  t.visuals.debugvisualsmode >= 10 ) 
		{
			if (  t.aisystem.usingdebugobjects == 0 ) 
			{
				t.aisystem.usingdebugobjects=1;
				//  show A.I debug renders
				darkai_createinternaldebugvisuals ( );
			}
		}
		else
		{
			if (  t.aisystem.usingdebugobjects == 1 ) 
			{
				t.aisystem.usingdebugobjects=0;
				//  hide A.I debug renders
				darkai_destroyinternaldebugvisuals ( );
			}
		}

	}
}

void darkai_obstacles_terrain ( void )
{
	//  can skip obstacle generation
	if (  g.gskipobstaclecreation == 1  )  return;
	if (  g.gskipterrainobstaclecreation == 1  )  return;
	
	//  scan heightmap to create obstacles around water bodies
	obs_fillterraindot ( );

	//  go through loop until all terrain features added to OBS list
	g.obsindex=0;
	t.mooreneighborhood.mode=0;
	while (  t.mooreneighborhood.mode != 99 ) 
	{
		ode_doterraindotwork ( );
	}
	//  create obstacle polygons from data
	if (  g.obsindex>0 ) 
	{
		//  reverse all OBS coords
		Dim (  t.finalobs,g.obsindex  );
		for ( t.oo = 1 ; t.oo<=  g.obsindex; t.oo++ )
		{
			t.finalobs[1+(g.obsindex-t.oo)]=t.obs[t.oo];
		}
		//  create obstacle polygons
		AIStartNewObstacle (  t.aisystem.terrainobsnum  ); t.tobjstarted=1;
		t.tcx_f=0 ; t.tcz_f=0 ; t.tcc=0;
		for ( t.o = 1 ; t.o<=  g.obsindex; t.o++ )
		{
			t.x=t.finalobs[t.o].x ; t.z=t.finalobs[t.o].z;
			if (  t.x == -1 && t.o == 1 ) 
			{
				//  ignore terminator at first index
			}
			else
			{
				if ( t.x == -1 ) break ; else { t.tcx_f=t.tcx_f+t.x ; t.tcz_f=t.tcz_f+t.z ; ++t.tcc ; }
			}
		}
		t.tcx_f=t.tcx_f/t.tcc ; t.tcz_f=t.tcz_f/t.tcc;
		for ( t.o = 1 ; t.o<=  g.obsindex; t.o++ )
		{
			t.x=t.finalobs[t.o].x ; t.z=t.finalobs[t.o].z;
			if (  t.x == -1 && t.o == 1 ) 
			{
				//  ignore terminator at first index
			}
			else
			{
				if (  t.x == -1 ) 
				{
					if (  t.tobjstarted == 1 ) 
					{
						AIEndNewObstacle (  0,0,0  ); t.tobjstarted=0;
						if (  t.o<g.obsindex ) 
						{
							AIStartNewObstacle (  t.aisystem.terrainobsnum  ); t.tobjstarted=1;
						}
					}
					t.tcx_f=0 ; t.tcz_f=0 ; t.tcc=0;
					for ( t.oo = t.o+1 ; t.oo<=  g.obsindex; t.oo++ )
					{
						t.x=t.finalobs[t.oo].x ; t.z=t.finalobs[t.oo].z;
						if ( t.x == -1 ) break; else { t.tcx_f=t.tcx_f+t.x ; t.tcz_f=t.tcz_f+t.z ; ++t.tcc; }
					}
					t.tcx_f=t.tcx_f/t.tcc ; t.tcz_f=t.tcz_f/t.tcc;
				}
				else
				{
					//  expand coordinate based on center (above) so A.I goes around obstacle much wider
					t.tx_f=t.x-t.tcx_f ; t.tz_f=t.z-t.tcz_f ; t.td_f=Sqrt(abs(t.tx_f*t.tx_f)+abs(t.tz_f*t.tz_f));
					t.tx_f=t.tx_f/t.td_f ; t.tz_f=t.tz_f/t.td_f;
					//      `x=x+(tx#*2.0) ; z=z+(tz#*2.0)
					t.x=t.x+(t.tx_f*3.5) ; t.z=t.z+(t.tz_f*3.5);
					AIAddObstacleVertex (  (t.x*50)+25,(t.z*50)+25 );
				}
			}
		}
		UnDim (  t.finalobs );
		if (  t.tobjstarted == 1 ) 
		{
			AIEndNewObstacle (  0,0,0 );
		}
	}

	//  free fillterraindot
	obs_freeterraindot ( );
}

void darkai_obstacles_terrain_refresh ( void )
{
	//  can delete obstacle and regenerate it
	if (  t.aisystem.generateobs == 1 ) 
	{
		if ( t.aisystem.usingdebugobjects == 1 ) darkai_destroyinternaldebugvisuals ( );
		AIRemoveObstacle (  t.aisystem.terrainobsnum );
		AICompleteObstacles (  );
		darkai_obstacles_terrain ( );
		AICompleteObstacles (  );
		if ( t.aisystem.usingdebugobjects == 1 ) darkai_createinternaldebugvisuals ( );
	}
}

void darkai_setup_characters ( void )
{
	//  Create A.I entities for all characters
	for ( t.e = 1 ; t.e<=  g.entityelementlist; t.e++ )
	{
		t.entid=t.entityelement[t.e].bankindex;
		if (  t.entid>0 ) 
		{
			if (  t.entityprofile[t.entid].ischaracter == 1 && t.entityelement[t.e].ragdollified == 0 ) 
			{
				t.tobj=t.entityelement[t.e].obj;
				if (  t.tobj>0 ) 
				{
					if (  ObjectExist(t.tobj) == 1 ) 
					{
						// 161115 - but only if not already part of the char anim list
						bool bFound = false;
						for ( int n = 1; n <= g.charanimindexmax; n++ )
						{
							if ( t.charanimstates[n].e == t.e )
							{
								bFound = true;
							}
						}
						if ( bFound==false )
						{
							//  Set up object one as character
							++g.charanimindexmax;
							g.charanimindex=g.charanimindexmax;
							Dim (  t.charanimcontrols,g.charanimindexmax  );
							Dim (  t.charanimstates,g.charanimindexmax );
							Dim2(  t.charactergunpose,g.charanimindexmax, 36 );
							t.charanimstates[g.charanimindex].obj=t.tobj;
							t.charanimstates[g.charanimindex].e=t.e;
							t.charanimstates[g.charanimindex].originale=t.e;
							darkai_setupcharacter ( );

							//  Load in any 'waste twist' data
							/* 301015 - monumentally not worked - instead use new UBER frames for future waste pivot
							t.charanimstates[g.charanimindex].usingcharacterposedat=0;
							t.ent_s=t.entitybank_s[t.entid] ; t.entpath_s=getpath(t.ent_s.Get());
							t.charactergunposefile_s=t.entdir_s+t.entpath_s+t.entityprofile[t.entid].model_s;
							t.charactergunposefile_s=Left(t.charactergunposefile_s.Get(),Len(t.charactergunposefile_s.Get())-2);
							t.charactergunposefile_s+=".dat";
							if (  FileExist(t.charactergunposefile_s.Get()) == 1 ) 
							{
								OpenToRead (  1,t.charactergunposefile_s.Get() );
								for ( t.i = 0 ; t.i<=  36; t.i++ )
								{
									t.a_f = ReadFloat (  1 ); t.charactergunpose[g.charanimindex][t.i].x=t.a_f;
									t.a_f = ReadFloat (  1 ); t.charactergunpose[g.charanimindex][t.i].y=t.a_f;
									t.a_f = ReadFloat (  1  ); t.charactergunpose[g.charanimindex][t.i].z=t.a_f;
								}
								CloseFile (  1 );
								t.charanimstates[g.charanimindex].usingcharacterposedat=1;
							}
							else
							{
							*/
							for ( t.i = 0 ; t.i<=  36; t.i++ )
							{
								t.charactergunpose[g.charanimindex][t.i].x=0;
								t.charactergunpose[g.charanimindex][t.i].y=0;
								t.charactergunpose[g.charanimindex][t.i].z=0;
							}
						}
					}
				}
			}
		}
	}
}

void darkai_destroy_all_characterdata ( void )
{
	//  ensure ALL character data is wiped
	// 161115 - only ADDS characters now (what about deleted characters during F9?)
	//g.charanimindexmax=0;
	//UnDim (  t.charanimcontrols );
	//UnDim (  t.charanimstates );
	//UnDim (  t.charactergunpose );
}

void darkai_release_characters ( void )
{
	//  Reset char max count
	g.charanimindexmax=0;
}

void darkai_setup_tree ( void )
{
	//  set up entity (ttreemode/tobj/tcontainerid/entid) - half height for all trees (so AI can see plr when stood behind tree)
	//  ttreemode ; 0=center on obj using slice 1/32th, 1=bottom root, 2-9=16th's from center downwards
	t.tworldstepy_f=(ObjectSizeY(t.tobj)/16.0);
	if (  t.ttreemode == 0  )  t.tworldy_f = (t.tworldstepy_f*0.25);
	if (  t.ttreemode == 1  )  t.tworldy_f = (t.tworldstepy_f*-0.5);
	if (  t.ttreemode>1  )  t.tworldy_f = (t.tworldstepy_f*8)-((t.ttreemode-2)*t.tworldstepy_f);
	//  0=create SINGLE Box (  that encompases this slice )
	t.tsimplebox=0;
	//  can skip obstacle generation
	if (  g.gskipobstaclecreation == 0 ) 
	{
		//  adding them in as full height so extra cover points arent made for every tree
		AIAddObstacleFromLevel (  t.tobj,t.tcontainerid,1,ObjectPositionY(t.tobj)+t.tworldy_f,t.tsimplebox,((t.entityprofile[t.entid].collisionscaling+0.0)/100.0) );
		//  this is eventually passed on the physics creation (perfect cylinder for tree trunk)
		t.entityelement[t.e].abscolx_f=AILastObstacleCenterX();
		t.entityelement[t.e].abscolz_f=AILastObstacleCenterZ();
		t.entityelement[t.e].abscolradius_f=AILastObstacleRadius();
	}
	else
	{
		t.entityelement[t.e].abscolx_f=ObjectPositionX(t.tobj);
		t.entityelement[t.e].abscolz_f=ObjectPositionZ(t.tobj);
		t.entityelement[t.e].abscolradius_f=ObjectSize(t.tobj,1);
	}
}

void darkai_setup_entity ( void )
{
	// set up entity (tobj/tfullheight/tcontainerid)
	// check if entity is more than plr-height above ground,
	// if so, cannot create obstacle on ground!
	t.tonground=1;
	if ( t.terrain.TerrainID>0 ) 
	{
		t.tusecurrentgroundheight_f=BT_GetGroundHeight(t.terrain.TerrainID,t.entityelement[t.e].x,t.entityelement[t.e].z);
	}
	else
	{
		t.tusecurrentgroundheight_f=g.gdefaultterrainheight;
	}
	if ( t.entityelement[t.e].y>t.tusecurrentgroundheight_f+75.0 ) 
	{
		t.tonground=0;
	}
	//  receives e,entid
	t.tobstype=0 ; t.tfullheight=1;
	if ( t.tonground == 1 ) 
	{
		if ( t.entityprofile[t.entid].forcesimpleobstacle == 0 ) 
		{
			if ( t.entityprofile[t.entid].collisionmode == 11 ) 
			{
				//  no obs
			}
			else
			{
				if (  t.entityprofile[t.entid].collisionmode == 40 ) 
				{
					t.tobstype=3;
				}
				else
				{
					if (  ObjectSizeX(t.tobj,1)>120 || ObjectSizeZ(t.tobj,1)>120 ) 
					{
						t.tobstype=2;
					}
					else
					{
						t.tobstype=1;
					}
				}
			}
		}
		else
		{
			t.tobstype=t.entityprofile[t.entid].forcesimpleobstacle;
		}
	}
	if (  t.tobstype>0 && g.gskipobstaclecreation == 0 ) 
	{
		if (  t.tobstype == 3 ) 
		{
			if (  t.aisystem.generateobs == 1 ) 
			{
				float forceobstacleslicemin = t.entityprofile[t.entid].forceobstaclepolysize;
				float forceobstaclesliceheight = t.entityprofile[t.entid].forceobstaclesliceheight;
				float forceobstaclesliceminsize = t.entityprofile[t.entid].forceobstaclesliceminsize;
				// 051718 - adjust if the entity is sunk into the floor (terrain height would slice higher)
				float fTerrainWouldSliceHigher = t.tusecurrentgroundheight_f - t.entityelement[t.e].y;
				if ( fTerrainWouldSliceHigher > 0 ) forceobstaclesliceheight += fTerrainWouldSliceHigher;
				darkai_addobstoallneededcontainers ( 3, t.tobj, t.tfullheight, forceobstacleslicemin, forceobstaclesliceheight, forceobstaclesliceminsize );
			}
			AIAddAlternateVisibilityObject (  t.tobj,0 );
		}
		if (  t.tobstype == 2 ) 
		{
			if (  t.aisystem.generateobs == 1 ) 
			{
				//AIAddStaticObstacle (  t.tobj,t.tfullheight,t.tcontainerid );
				darkai_addobstoallneededcontainers ( 2, t.tobj, t.tfullheight, 0.0f, 0.0f, 5.0f );
			}
			AIAddAlternateVisibilityObject (  t.tobj,0 );
		}
		if (  t.tobstype == 1 ) 
		{
			if (  ObjectExist(g.darkaiobsboxobject)  ==  1  ) DeleteObject (  g.darkaiobsboxobject ) ;
			MakeObjectBox (  g.darkaiobsboxobject , ObjectSizeX(t.tobj,1) ,ObjectSizeY(t.tobj,1),ObjectSizeZ(t.tobj,1) );
			OffsetLimb (   g.darkaiobsboxobject, 0, GetObjectCollisionCenterX(t.tobj) + LimbOffsetX(t.tobj,0),0,GetObjectCollisionCenterZ(t.tobj) + LimbOffsetZ(t.tobj,0) );
			PositionObject (  g.darkaiobsboxobject,ObjectPositionX(t.tobj),ObjectPositionY(t.tobj),ObjectPositionZ(t.tobj) );
			RotateObject (  g.darkaiobsboxobject , ObjectAngleX(t.tobj),ObjectAngleY(t.tobj),ObjectAngleZ(t.tobj) );
			if (  ObjectSizeY(t.tobj,1)  <=  50  )  t.tfullheight  =  0;
			if (  t.aisystem.generateobs == 1 ) 
			{
				//AIAddStaticObstacle (  g.darkaiobsboxobject,t.tfullheight,t.tcontainerid );
				darkai_addobstoallneededcontainers ( 1, g.darkaiobsboxobject, t.tfullheight, 0.0f, 0.0f, 5.0f );
			}
			AIAddAlternateVisibilityObject (  g.darkaiobsboxobject,0 );
			DeleteObject (  g.darkaiobsboxobject );
		}
	}
}

void darkai_addobstoallneededcontainers ( int iType, int iObj, int iFullHeight, float fMinHeight, float fSliceHeight, float fSliceMinSize )
{
	// go through all zones (zone zero is container zero)
	for ( t.twaypointindex = 0; t.twaypointindex <= g.waypointmax; t.twaypointindex++ )
	{
		if ( t.twaypointindex == 0 || t.waypoint[t.twaypointindex].style == 3 ) 
		{
			if ( t.twaypointindex == 0 || t.waypoint[t.twaypointindex].count > 0 ) 
			{
				// work out center of whole zone and radius to outer most node
				bool bIncludeInContainer = false;
				if ( t.twaypointindex == 0 )
				{
					// container zero zone is everything on the terrain floor
					bIncludeInContainer = true;
				}
				else
				{
					// sub-zones have boundaries
					int iAvCount = 0;
					float fAvX = 0.0f;
					float fAvZ = 0.0f;
					for ( t.w = t.waypoint[t.twaypointindex].finish-1; t.w >= t.waypoint[t.twaypointindex].start; t.w-- )
					{
						fAvX += t.waypointcoord[t.w].x;
						fAvZ += t.waypointcoord[t.w].z;
						iAvCount++;
					}
					if ( iAvCount > 0 )
					{
						// this is the center, now thr radius
						fAvX /= iAvCount;
						fAvZ /= iAvCount;
						float fZoneRadius = 0.0;
						for ( t.w = t.waypoint[t.twaypointindex].finish-1; t.w >= t.waypoint[t.twaypointindex].start; t.w-- )
						{
							float fDX = t.waypointcoord[t.w].x - fAvX;
							float fDZ = t.waypointcoord[t.w].z - fAvZ;
							float fDD = sqrt ( fabs(fDX*fDX)+fabs(fDZ*fDZ) );
							if ( fDD > fZoneRadius ) fZoneRadius = fDD;
							iAvCount++;
						}

						// get distance from this entity and zone center
						float fDX = ObjectPositionX ( iObj ) - fAvX;
						float fDZ = ObjectPositionZ ( iObj ) - fAvZ;
						float fDistanceBetweenZoneAndEnt = sqrt ( fabs(fDX*fDX)+fabs(fDZ*fDZ) );
						float fEntRadius = GetObjectCollisionRadius ( iObj );
						if ( fDistanceBetweenZoneAndEnt < fZoneRadius+fEntRadius )
							bIncludeInContainer = true;
					}
				}
				if ( bIncludeInContainer == true )
				{
					// yes, point is inside this zone
					int iContainerID = t.twaypointindex;
					switch ( iType )
					{
						case 1 : AIAddStaticObstacle ( iObj, iFullHeight, iContainerID ); break;
						case 2 : AIAddStaticObstacle ( iObj, iFullHeight, iContainerID ); break;
						case 3 : AIAddObstacleFromLevel ( iObj, iContainerID, iFullHeight, ObjectPositionY(iObj)+fSliceHeight, fSliceMinSize, fMinHeight, 0 ); break;
					}
				}
			}
		}
	}
}

void darkai_staggerAIprocessing ( void )
{
	//  ensures AI processing spread evenly across vision delay
	t.tstep_f=2000.0/g.charanimindexmax;
	for ( g.charanimindex = 1 ; g.charanimindex<=  g.charanimindexmax; g.charanimindex++ )
	{
		t.charanimstates[g.charanimindex].visiondelaylastime=Timer()+(g.charanimindex*t.tstep_f);
	}
}

void darkai_setupcharacter ( void )
{
	//  Entity profile index
	t.ttentid=t.entityelement[t.charanimstates[g.charanimindex].e].bankindex;

	//  Setup character defaults
	t.charanimstates[g.charanimindex].playcsi=g.csi_limbo;
	t.charanimstates[g.charanimindex].limbomanualmode=0;
	t.charanimstates[g.charanimindex].alerted=0;
	t.charanimstates[g.charanimindex].realheadangley_f=0.0;
	t.charanimstates[g.charanimindex].animationspeed_f=(65.0/100.0)*t.entityelement[t.charanimstates[g.charanimindex].e].eleprof.animspeed;
	if (  t.charanimstates[g.charanimindex].animationspeed_f <= 0 ) 
	{
		//  300615 - character animation speed should never be zero (legacy maps)
		t.charanimstates[g.charanimindex].animationspeed_f=65.0;
	}
	t.charanimstates[g.charanimindex].outofrange=1;
	t.charanimstates[g.charanimindex].currentangle_f=t.entityelement[t.charanimstates[g.charanimindex].e].ry;
	t.charanimstates[g.charanimindex].moveangle_f=t.charanimstates[g.charanimindex].currentangle_f;
	t.charanimstates[g.charanimindex].visiondelay=2000;
	t.charanimstates[g.charanimindex].visiondelaylastime=0;

	//  By default, characters have default PISTOL weapon style OR new WEAPSTYLE value
	t.tgunid=t.entityelement[t.charanimstates[g.charanimindex].e].eleprof.hasweapon;
	if (  t.entityprofile[t.ttentid].usesweapstyleanims == 0 ) 
	{
		if (  t.tgunid>0 ) 
		{
			t.charanimstates[g.charanimindex].weapstyle=1;
		}
		else
		{
			t.charanimstates[g.charanimindex].weapstyle=0;
		}
	}
	else
	{
		//  1-pistol, 2-rocket, 3-shotgun, 4-uzi, 5-assault
		t.charanimstates[g.charanimindex].weapstyle=t.gun[t.tgunid].weapontype;
	}

	//  populate character with weapon details
	t.tgunid=t.entityelement[t.charanimstates[g.charanimindex].e].eleprof.hasweapon;
	t.charanimstates[g.charanimindex].ammoinclipmax=g.firemodes[t.tgunid][0].settings.reloadqty;
	t.charanimstates[g.charanimindex].ammoinclip=t.charanimstates[g.charanimindex].ammoinclipmax;
	if (  t.charanimstates[g.charanimindex].ammoinclip>0 ) 
	{
		//  allows characters to reload at different times
		t.charanimstates[g.charanimindex].ammoinclip=1+Rnd(t.charanimstates[g.charanimindex].ammoinclip-1);
	}

	//  Set collision property
	SetObjectCollisionProperty (  t.charanimstates[g.charanimindex].obj,0 );

	//  if this character is controlled by third person control, override
	if (  t.playercontrol.thirdperson.enabled == 1 && t.playercontrol.thirdperson.charactere == t.charanimstates[g.charanimindex].e ) 
	{
		//  THIRD PERSON CONTROL
		t.playercontrol.thirdperson.characterindex=g.charanimindex;
		int iWeaponOrDefault = t.charanimstates[g.charanimindex].weapstyle;
		if ( iWeaponOrDefault < 1 ) iWeaponOrDefault = 1; // for zombies and barbarians who have no weapons
		t.charanimstates[g.charanimindex].weapstyle = iWeaponOrDefault; // so related code can find csi anim frames for TPP
		t.charanimstates[g.charanimindex].playcsi=t.csi_stood[iWeaponOrDefault];
		t.charanimstates[g.charanimindex].e=0;
		t.charanimstates[g.charanimindex].rocketstyle=0;
		physics_player_thirdpersonreset ( );
	}
	else
	{
		//  REGULAR AI
		//  determine if character holds 'gun' or 'rocket' style weapon
		if (  t.charanimstates[g.charanimindex].weapstyle <= 1 ) 
		{
			//  only if older legacy character (newer Uber characters use weapstyle=2)
			t.charanimstates[g.charanimindex].rocketstyle=0;
			if (  t.tgunid>0 ) 
			{
				if (  g.firemodes[t.tgunid][0].settings.flakindex>0 ) 
				{
					t.charanimstates[g.charanimindex].rocketstyle=1;
				}
			}
		}

		//  Hard-Code ENEMY or NEUTRAL in entity profile (can be changed via LUA scripting)
		t.charanimstates[g.charanimindex].aiobjectexists=1;
		t.i=t.charanimstates[g.charanimindex].obj;
		AIAddNeutral ( t.i, 0 );

		// Default A.I entity settings
		t.tx_f = ObjectPositionX(t.i);
		t.ty_f = ObjectPositionY(t.i);
		t.tz_f = ObjectPositionZ(t.i);
		AISetEntityPosition (  t.i,t.tx_f,t.ty_f,t.tz_f );
		t.tconeangle=t.entityelement[t.charanimstates[g.charanimindex].e].eleprof.coneangle;
		//if (  t.tconeangle == 0  )  t.tconeangle = 90;
		if (  t.tconeangle == 0  )  t.tconeangle = 135; // change default so characters more responsive (can see extreme sides!)
		AISetEntityFireArc (  t.i,t.tconeangle );
		AISetEntityViewArc (  t.i,t.tconeangle,t.tconeangle*2 );
		t.tconerange=t.entityelement[t.charanimstates[g.charanimindex].e].eleprof.conerange;
		if (  t.tconerange == 0  )  t.tconerange = 800*2;
		AISetEntityViewRange (  t.i,t.tconerange );
		AISetEntityHearingRange (  t.i,100 );
		AISetEntityHeight (  t.i,t.aisystem.defaulteyeheight_f );
		AISetEntityCanHear (  t.i,1 );
		AISetEntityAvoidDistance (  t.i,0 );
		AISetEntityCanHideBehindCorner (  t.i,0 );
		AISetEntityCanDuck (  t.i,0 );
		AISetEntityCanAttack (  t.i,0 );
		AISetEntityCanStrafe (  t.i,0 );
		AISetEntityCanSearch (  t.i,0 );
		AISetEntityCanRoam (  t.i,0 );

		// set propertional to entity move speed (100=750)
		float fTurnSpeed = t.entityelement[t.charanimstates[g.charanimindex].e].eleprof.speed * 7.5f;
		AISetEntityTurnSpeed ( t.i, fTurnSpeed );

		// set the starting container for this character (based on feet proximity to any navmeshzone in level)
		int iContainerStartIndex = 0;
		for ( t.waypointindex = 1; t.waypointindex <= g.waypointmax; t.waypointindex++ )
		{
			if ( waypoint_ispointinzoneex ( t.waypointindex, t.tx_f, t.ty_f, t.tz_f, 1 ) == 1 ) 
			{
				iContainerStartIndex = t.waypointindex;
				break;
			}
		}
		AISetEntityContainer ( t.i, iContainerStartIndex );

		// Command A.I entity to manually stop at start location
		AISetEntityControl ( t.i, 0 );
		AIEntityStopNoMoveAddition ( t.i );
		AISetEntityAngleY ( t.i,t.charanimstates[g.charanimindex].currentangle_f );

		// set whether AI bot is ALWAYS active (for distant enemies on paths, etc)
		if ( t.entityelement[t.charanimstates[g.charanimindex].e].eleprof.phyalways == 0 )
			AISetEntityAlwaysActive ( t.i, false );
		else
			AISetEntityAlwaysActive ( t.i, true );

		// by default, avoidance plotting is instant avoidance
		// NOTE: Real-Time (mode 1) entities will avoid each other and check for other nearby entities every update, 
		// constantly changing positions can result in an entity jumping about as its movement direction changes every update. 
		// This was the only mode in the original release version and is default.
		// PROBLEM is that they PUSH AI out of zones and INTO obstacles, so this needs to be tied to path generation
		// if MODE 1 is to be used again!
		AISetEntityAvoidMode ( t.i, 99 );
	}

	// character speaking settings reset
	t.charanimstates[g.charanimindex].ccpo.speak.mouthData.clear();
	t.charanimstates[g.charanimindex].ccpo.speak.fMouthTimeStamp = 0.0f;
	t.charanimstates[g.charanimindex].ccpo.speak.iMouthDataShape = 0;
	t.charanimstates[g.charanimindex].ccpo.speak.iMouthDataIndex = 0;
	t.charanimstates[g.charanimindex].ccpo.speak.fSmouthDataSpeedToNextShape = 4.0f;
	t.charanimstates[g.charanimindex].ccpo.speak.fNeedToBlink = 0.0f;

	// must be full object to be a character
	int iStoreOBJ = t.obj;
	int iStoreENTID = t.tentid;
	int iStoreTTE = t.tte;
	t.obj = t.charanimstates[g.charanimindex].obj;
	t.tentid = t.entityelement[t.charanimstates[g.charanimindex].e].bankindex;
	t.tte = t.charanimstates[g.charanimindex].e; entity_converttoclone ( );
	t.obj = iStoreOBJ; t.tentid = iStoreENTID; t.tte = iStoreTTE;

	// find neck bone for this base body model (can shift index)
	int iNeckBone = 0;
	PerformCheckListForLimbs(t.charanimstates[g.charanimindex].obj);
	for ( int c = 1; c <= ChecklistQuantity(); c++ )
		if ( iNeckBone == 0 && (strstr ( ChecklistString ( c ), "_Head" ) != NULL || strstr ( ChecklistString ( c ), "_head" ) != NULL) )
			iNeckBone = c - 1;
	t.charanimstates[g.charanimindex].ccpo.settings.iNeckBone = iNeckBone;
}

void darkai_staywithzone ( int iAIObj, float fLastX, float fLastZ, float* pX, float* pZ )
{
	// when move entity under AI control, ensure ALWAYS stay within containing zone if any
	AIStayWithinContainer ( iAIObj, fLastX, fLastZ, pX, pZ );
}

void darkai_makesound ( void )
{
	//  tsx#,tsz#,tradius#
	AICreateSound (  t.tsx_f , t.tsz_f , (int)t.tradius_f/10 ,t.tradius_f , -1 );
}

void darkai_makeexplosionsound ( void )
{
	//  tsx#,tsz#
	AICreateSound (  t.tsx_f,t.tsz_f,(int)200,200.0f,-1 );
}

std::vector<int> g_AIPlayerVisibleChecked;

void darkai_shootplayer ( void )
{
	//  takes tcharanimindex
	//  recalc PLRVISIBLE to ensure the enemy can truly STILL see PLAYER (VisionDelay can make this value erroneous)

	if (!t.entityelement[t.charanimstate.e].bPlrVisibleCheckDone)
		darkai_calcplrvisible();

	t.entityelement[t.charanimstate.e].bPlrVisibleCheckDone = true;

	//  takes charanimstate, if A.I not currently firing
	//  if want to shoot, can override firesound in use (otherwise can wait 7 seconds while sound fades)
	t.tpermitanoverride=0;
	if (  t.charanimstate.firesoundindex>0 && Timer()>(int)t.charanimstate.firesoundstarted+50  )  t.tpermitanoverride = 1;
	if (  (t.charanimstate.firesoundindex == 0 || t.tpermitanoverride == 1) && t.entityelement[t.charanimstate.e].plrvisible == 1 ) 
	{
		//  handle player being shot at
		t.te=t.charanimstate.e;
		if (  t.te>0 ) 
		{
			t.tentid=t.entityelement[t.te].bankindex;
			t.tgunid=t.entityprofile[t.tentid].hasweapon;
			t.tcannotfirenow=0;
			if (  t.charanimstate.playcsi >= t.csi_crouchmovefore[t.charanimstate.weapstyle] && t.charanimstate.playcsi <= t.csi_crouchmoverun[t.charanimstate.weapstyle] ) 
			{
				//  cannot fire if crouch moving
				t.tcannotfirenow=1;
			}
			t.tattachedobj=t.entityelement[t.te].attachmentobj;
			if (  t.tattachedobj>0 ) 
			{
				//  cannot fire if weapon not pointing at player
				t.tattachmentobjfirespotlimb=t.entityelement[t.te].attachmentobjfirespotlimb;
				if (  t.tgunid>0 && t.tattachmentobjfirespotlimb>0 ) 
				{
					t.tx_f=LimbPositionX(t.tattachedobj,t.tattachmentobjfirespotlimb)-ObjectPositionX(t.aisystem.objectstartindex);
					t.ty_f=LimbPositionY(t.tattachedobj,t.tattachmentobjfirespotlimb)-ObjectPositionY(t.aisystem.objectstartindex);
					t.tz_f=LimbPositionZ(t.tattachedobj,t.tattachmentobjfirespotlimb)-ObjectPositionZ(t.aisystem.objectstartindex);
					t.tdist_f=Sqrt(abs(t.tx_f*t.tx_f)+abs(t.ty_f*t.ty_f)+abs(t.tz_f*t.tz_f));
					if (  ObjectExist(g.projectorsphereobjectoffset) == 0 ) 
					{
						MakeObjectSphere (  g.projectorsphereobjectoffset,10 );
						HideObject (  g.projectorsphereobjectoffset );
					}
					PositionObject (  g.projectorsphereobjectoffset,LimbPositionX(t.tattachedobj,t.tattachmentobjfirespotlimb),LimbPositionY(t.tattachedobj,t.tattachmentobjfirespotlimb),LimbPositionZ(t.tattachedobj,t.tattachmentobjfirespotlimb) );
					RotateObject (  g.projectorsphereobjectoffset,LimbDirectionX(t.tattachedobj,t.tattachmentobjfirespotlimb),LimbDirectionY(t.tattachedobj,t.tattachmentobjfirespotlimb),LimbDirectionZ(t.tattachedobj,t.tattachmentobjfirespotlimb) );
					MoveObject (  g.projectorsphereobjectoffset,t.tdist_f*-1 );
					t.tx_f=ObjectPositionX(g.projectorsphereobjectoffset)-ObjectPositionX(t.aisystem.objectstartindex);
					t.ty_f=ObjectPositionY(g.projectorsphereobjectoffset)-ObjectPositionY(t.aisystem.objectstartindex);
					t.tz_f=ObjectPositionZ(g.projectorsphereobjectoffset)-ObjectPositionZ(t.aisystem.objectstartindex);
					t.tdist2_f=Sqrt(abs(t.tx_f*t.tx_f)+abs(t.ty_f*t.ty_f)+abs(t.tz_f*t.tz_f));
					t.tactualdistance_f=t.tdist2_f;
					t.tdist2_f=t.tdist2_f/t.tdist_f;
					t.tdist2_f=int(t.tdist2_f*100);
					if (  t.playercontrol.thirdperson.enabled == 1 ) 
					{
						//  can be less accurate and still hit in third person (no camera scrutiny)
						if (  t.tdist2_f>100  )  t.tcannotfirenow = 1;
					}
					else
					{
						if (  t.tdist2_f>50 && t.tactualdistance_f>100.0  )  t.tcannotfirenow = 1;
					}
				}
			}
			if (  t.tgunid>0 && t.tcannotfirenow == 0 ) 
			{
				//  frequenty of fire
				t.ttratecalc_f=(1.0/(1.0+g.firemodes[t.tgunid][0].settings.firerate))*g.timeelapsed_f*2.0;
				t.charanimstate.firerateaccumilator=t.charanimstate.firerateaccumilator-t.ttratecalc_f;
				if (  t.charanimstate.firerateaccumilator<0.0 ) 
				{
					t.charanimstate.firerateaccumilator=0.5+(Rnd(100)/100.0);
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
									t.toldsndid=t.charanimstate.firesoundindex;
									if (  t.toldsndid>0 ) 
									{
										if (  SoundExist(t.toldsndid) == 1 ) 
										{
											StopSound (  t.toldsndid );
										}
									}
									t.charanimstate.firesoundindex=t.ttsnd ; t.tt=3;
									t.tfireloopend=g.firemodes[t.tgunid][0].sound.fireloopend;
									t.charanimstate.firesoundstarted=Timer();
									if (  t.tfireloopend>0 && t.game.runasmultiplayer == 0 ) 
									{
										//  sound loops (need to cap it off)
										t.charanimstate.firesoundexpiry=Timer()+200+Rnd(200);
									}
									else
									{
										//  can let sound fade out slowly naturally
										t.charanimstate.firesoundexpiry=Timer()+5000;
									}
								}
							}
						}
					}
					if (  t.charanimstate.firesoundindex>0 ) 
					{
						//  shoot effects
						t.tgunid=t.entityelement[t.te].eleprof.hasweapon;
						t.tattachedobj=t.entityelement[t.te].attachmentobj;
						darkai_shooteffect ( );
					}
				}
			}
		}
	}
}

void darkai_shooteffect ( void )
{
	//  needs tgunid, example; tgunid=entityprofile(tentid).hasweapon
	//  needs tattachedobj, example; tattachedobj=entityelement(te).attachmentobj
	//  needs te (entityelement index), example; te = e
	//  charanimstate.firesoundindex needs to be set, examle; charanimstate.firesoundindex=ttsnd

	//  Because of coop mode, the ai may not always be shooting at the camera
	if (  t.game.runasmultiplayer == 0 || g.mp.coop == 0 ) 
	{
		t.tplayerx_f = ObjectPositionX(t.aisystem.objectstartindex);
		t.tplayery_f = ObjectPositionY(t.aisystem.objectstartindex);
		t.tplayerz_f = ObjectPositionZ(t.aisystem.objectstartindex);
	}
	else
	{
		t.ttentid=t.entityelement[t.te].bankindex;
		//  If they are not a player, they must be ai, so grab their gunid and attacheobj
		if (  t.entityprofile[t.ttentid].ismultiplayercharacter  ==  0 ) 
		{
			t.tgunid=t.entityprofile[t.ttentid].hasweapon;
			t.tattachedobj=t.entityelement[t.te].attachmentobj;
		}
		if (  t.entityelement[t.te].mp_coopControlledByPlayer  ==  g.mp.me || t.entityelement[t.te].mp_coopControlledByPlayer  ==  -1 || t.entityprofile[t.ttentid].ismultiplayercharacter  ==  1 ) 
		{
			t.tplayerx_f = CameraPositionX(0);
			t.tplayery_f = CameraPositionY(0);
			t.tplayerz_f = CameraPositionZ(0);
		}
		else
		{
			t.tsteamplayer = t.entityelement[t.te].mp_coopControlledByPlayer;
			if ( t.mp_playerEntityID[t.tsteamplayer] > 0 )
			{
				t.tplayerx_f = ObjectPositionX(t.entityelement[t.mp_playerEntityID[t.tsteamplayer]].obj);
				t.tplayery_f = ObjectPositionY(t.entityelement[t.mp_playerEntityID[t.tsteamplayer]].obj);
				t.tplayerz_f = ObjectPositionZ(t.entityelement[t.mp_playerEntityID[t.tsteamplayer]].obj);
			}
		}
	}

	//  emit spot flash
	if (  t.tattachedobj>0 ) 
	{
		//  best coordinate is firespot on attached gun
		t.tokay=0;
		t.tattachmentobjfirespotlimb=t.entityelement[t.te].attachmentobjfirespotlimb;
		if (  t.tgunid>0 && t.tattachmentobjfirespotlimb>0 ) 
		{
			t.tx_f=LimbPositionX(t.tattachedobj,t.tattachmentobjfirespotlimb);
			t.ty_f=LimbPositionY(t.tattachedobj,t.tattachmentobjfirespotlimb);
			t.tz_f=LimbPositionZ(t.tattachedobj,t.tattachmentobjfirespotlimb);
			t.tokay=1;
		}
		if (  t.tokay == 0 ) 
		{
			//  actual gun position is better source coordinate
			t.tx_f=ObjectPositionX(t.tattachedobj);
			t.ty_f=ObjectPositionY(t.tattachedobj);
			t.tz_f=ObjectPositionZ(t.tattachedobj);
		}
	}
	else
	{
		//  fallback is entity center
		t.tobj=t.entityelement[t.te].obj;
		t.tx_f=ObjectPositionX(t.tobj);
		t.ty_f=ObjectPositionY(t.tobj)+50.0;
		t.tz_f=ObjectPositionZ(t.tobj);
	}
	t.tcolr=g.firemodes[t.entityelement[t.te].eleprof.hasweapon][0].settings.muzzlecolorr/5;// /2; 100718 - tone it down a touch
	t.tcolg=g.firemodes[t.entityelement[t.te].eleprof.hasweapon][0].settings.muzzlecolorg/5;// /2;
	t.tcolb=g.firemodes[t.entityelement[t.te].eleprof.hasweapon][0].settings.muzzlecolorb/5;// /2;
	lighting_spotflash_forenemies ( );

	//  initiate decal
	t.decalid=g.firemodes[t.tgunid][0].decalid;
	g.decalx=t.tx_f ; g.decaly=t.ty_f ; g.decalz=t.tz_f;
	t.decalscalemodx=0 ; t.decalorient=11;
	t.originatore=-1;
	t.originatorobj=t.tattachedobj;
	t.decalforward=g.firemodes[t.tgunid][0].settings.decalforward;
	if (  g.firemodes[t.tgunid][0].action.automatic.s>0 ) 
	{
		//  special instruction for decal to loop X times
		t.decalburstloop=4;
	}
	else
	{
		t.decalburstloop=0;
	}
	decalelement_create ( );
	t.decalburstloop=0;

	//  emit sound
	t.tsx_f=t.entityelement[t.te].x ; t.tsz_f=t.entityelement[t.te].z ; t.tradius_f=200;
	darkai_makesound ( );
	t.ttsnd=t.charanimstate.firesoundindex;
	if (  t.ttsnd>0 ) 
	{
		if (  SoundExist(t.ttsnd) == 1 ) 
		{
			t.tfireloopend=g.firemodes[t.tgunid][0].sound.fireloopend;
			if (  t.tfireloopend>0 && t.game.runasmultiplayer == 0 ) 
			{
				PlaySoundOffset (  t.ttsnd,t.tfireloopend  ); LoopSound (  t.ttsnd,0,t.tfireloopend );
			}
			else
			{
				PlaySound (  t.ttsnd );
			}
			PositionSound (  t.ttsnd,t.entityelement[t.te].x,t.entityelement[t.te].y,t.entityelement[t.te].z );
			t.tvolume_f=soundtruevolume(95.0);
			SetSoundVolume (  t.ttsnd,t.tvolume_f );
			SetSoundSpeed (  t.ttsnd,43000+Rnd(2000) );
		}
	}

	//  is bullet or flak
	t.tflakid=g.firemodes[t.tgunid][0].settings.flakindex;
	if (  t.tflakid == 0 ) 
	{
		//  BULLET
		//  determine if bullet hit based on distance (ttdistanceaccuracy# lower is better)
		t.ttdx_f=t.tplayerx_f-t.tx_f;
		t.ttdy_f=t.tplayery_f-t.ty_f;
		t.ttdz_f=t.tplayerz_f-t.tz_f;
		t.ttdd_f=Sqrt(abs(t.ttdx_f*t.ttdx_f)+abs(t.ttdy_f*t.ttdy_f)+abs(t.ttdz_f*t.ttdz_f));
		t.ttdistanceaccuracy_f=t.ttdd_f/800.0;
		if (  t.ttentid>0  )  t.tisnotmpchar  =  t.entityprofile[t.ttentid].ismultiplayercharacter; else t.tisnotmpchar  =  0;
		if (  t.game.runasmultiplayer == 0 || ( g.mp.coop  ==  1 && t.tisnotmpchar  ==  0 ) ) 
		{
			if (  t.aisystem.playerducking == 1 ) 
			{
				if (  t.playercontrol.movement == 0 ) 
				{
					t.tchancetohit_f=4.0;
				}
				else
				{
					t.tchancetohit_f=12.0;
				}
			}
			else
			{
				if (  t.playercontrol.movement == 0 ) 
				{
					t.tchancetohit_f=2.0;
				}
				else
				{
					t.tchancetohit_f=6.0;
				}
			}
			if (  t.ttdistanceaccuracy_f<0.3 || Rnd(t.tchancetohit_f*t.ttdistanceaccuracy_f) == 0 ) 
			{
				//  amount of damage to player
				if (  t.game.runasmultiplayer  ==  0 || ( g.mp.coop  ==  1 && t.entityelement[t.te].mp_coopControlledByPlayer  ==  g.mp.me ) ) 
				{
					t.tdamage=g.firemodes[t.tgunid][0].settings.damage;
					physics_player_takedamage ( );
				}
			}
			else
			{
					//  play bullet whiz sound because the AI missed
					t.tSndID = t.playercontrol.soundstartindex + 25 + Rnd(3);
					if (SoundExist(t.tSndID) == 1)
					{
						t.tSndX_f = t.tplayerx_f - 100 + Rnd(200);
						t.tSndY_f = t.tplayery_f - 100 + Rnd(200);
						t.tSndZ_f = t.tplayerz_f - 100 + Rnd(200);
						PositionSound(t.tSndID, t.tSndX_f, t.tSndY_f, t.tSndZ_f);
						SetSoundVolume(t.tSndID, soundtruevolume(100));
						SetSoundSpeed(t.tSndID, 36000 + Rnd(10000));
						PlaySound(t.tSndID);
					}
			}
		}
	}
	else
	{
		//  FLAK (projectile)
		//  find starting GetPoint (  for projectile )
		t.tobj=t.entityelement[t.te].attachmentobj;
		if (  t.tobj>0 ) 
		{
			t.flakx_f=LimbPositionX(t.tobj,t.entityelement[t.te].attachmentobjfirespotlimb);
			t.flaky_f=LimbPositionY(t.tobj,t.entityelement[t.te].attachmentobjfirespotlimb);
			t.flakz_f=LimbPositionZ(t.tobj,t.entityelement[t.te].attachmentobjfirespotlimb);
			t.tdx_f=t.tplayerx_f-t.flakx_f;
			t.tdy_f=t.tplayery_f-t.flaky_f;
			t.tdz_f=t.tplayerz_f-t.flakz_f;
			t.tdd_f=Sqrt(abs(t.tdx_f*t.tdx_f)+abs(t.tdz_f*t.tdz_f));
			t.flakangle_f=atan2deg(t.tdx_f,t.tdz_f)+(Rnd(4)-2);
			t.flakpitch_f=((t.tdy_f/t.tdd_f)*-35.0)+Rnd(4)-2;

			t.tsteamismpchar = 0;
			t.ttzentid=t.entityelement[t.te].bankindex;
			if (  t.ttzentid>0  )  t.tsteamismpchar  =  t.entityprofile[t.ttzentid].ismultiplayercharacter;

			//  create and launch projectile
			if (  t.game.runasmultiplayer == 0 || t.tsteamismpchar  ==  0 ) 
			{
				//t.tProjectileType=1; characters can shoot ANY projectile type now
				int iStoreGunID = t.gunid;
				t.gunid = t.tgunid;
				t.tProjectileType_s=t.gun[t.gunid].projectile_s; weapon_getprojectileid ( );
				t.tSourceEntity=t.te ; t.tTracerFlag=0;
				t.tStartX_f=t.flakx_f ; t.tStartY_f=t.flaky_f ; t.tStartZ_f=t.flakz_f;
				t.tAngX_f=t.flakpitch_f ; t.tAngY_f=t.flakangle_f ; t.tAngZ_f=0;
				weapon_projectile_make ( false );
				t.gunid = iStoreGunID;
			}
		}
		t.tolde = t.e;
		t.e = t.te;
		entity_lua_findcharanimstate ( );
		t.e=t.tolde;
		if (  t.game.runasmultiplayer == 0 || t.tsteamismpchar  ==  0 ) 
		{
			// if not AI manual mode
			if ( t.charanimstate.limbomanualmode != 1 )
			{
				// flak can trigger recoil anim in character if not moving
				if ( t.tcharanimindex  !=  -1 ) 
				{
					if ( t.charanimcontrols[t.tcharanimindex].moving == 0 ) 
					{
						// use INSTANT method too!
						t.charanimcontrols[t.tcharanimindex].spotactioning=4;
						t.smoothanim[t.charanimstate.obj].transition=0;
						if (  t.charanimstate.playcsi >= t.csi_crouchidle[t.charanimstate.weapstyle] && t.charanimstate.playcsi <= t.csi_crouchgetuprocket[t.charanimstate.weapstyle] ) 
						{
							t.charanimstate.playcsi=t.csi_crouchfirerocket[t.charanimstate.weapstyle];
						}
						else
						{
							t.charanimstate.playcsi=t.csi_stoodfirerocket[t.charanimstate.weapstyle];
						}
					}
				}
			}
		}
		//  deduct one unit of ammo (only if npc oes NOT ignore need to reload)
		if (  g.firemodes[t.tgunid][0].settings.npcignorereload == 0 ) 
		{
			t.charanimstate.ammoinclip=t.charanimstate.ammoinclip-1;
			if (  t.charanimstate.ammoinclip<0  )  t.charanimstate.ammoinclip = 0;
		}
	}

	if (  t.game.runasmultiplayer  ==  1 && g.mp.coop  ==  1 && t.tLuaDontSendLua  ==  0 ) 
	{
		mp_sendlua (  MP_LUA_FireWeaponEffectOnly,t.te,0 );
	}
}

void darkai_killai ( void )
{
	if (  t.charanimstates[t.tcharanimindex].aiobjectexists == 1 ) 
	{
		//  Attempt to call the _exit function for the characters script
		if (  t.entityelement[t.charanimstates[t.tcharanimindex].e].eleprof.aimain == 1 ) 
		{
			t.strwork = Lower(t.entityelement[t.charanimstates[t.tcharanimindex].e].eleprof.aimainname_s.Get());
			t.strwork += "_exit";
			LuaSetFunction ( t.strwork.Get() ,1,0 );
			LuaPushInt (  t.charanimstates[t.tcharanimindex].e  ); LuaCallSilent (  );
		}

		// remove AI from AI system
		AIKillEntity ( t.charanimstates[t.tcharanimindex].obj );

		// free this AI from the game loop
		t.charanimstates[t.tcharanimindex].aiobjectexists=0;
		if ( t.entityelement[t.charanimstates[t.tcharanimindex].e].usingphysicsnow != 0 ) 
		{
			t.tphyobj=t.charanimstates[t.tcharanimindex].obj ; physics_disableobject ( );
			t.entityelement[t.charanimstates[t.tcharanimindex].e].usingphysicsnow=0;
		}
		SetObjectCollisionProperty ( t.charanimstates[t.tcharanimindex].obj,1 );
	}

	//  reset any limbs of character
	if (  t.entityelement[t.charanimstates[t.tcharanimindex].e].health>0 ) 
	{
		t.headlimbofcharacter=t.entityprofile[t.entityelement[t.charanimstates[t.tcharanimindex].e].bankindex].headlimb;
		if (  t.headlimbofcharacter>0 ) 
		{
			if (  LimbExist(t.charanimstates[t.tcharanimindex].obj,t.headlimbofcharacter) == 1 ) 
			{
				RotateLimb (  t.charanimstates[t.tcharanimindex].obj,t.headlimbofcharacter,0,0,0 );
			}
		}
		t.spinelimbofcharacter=t.entityprofile[t.entityelement[t.charanimstates[t.tcharanimindex].e].bankindex].spine;
		if (  t.spinelimbofcharacter>0 ) 
		{
			if (  LimbExist(t.charanimstates[t.tcharanimindex].obj,t.spinelimbofcharacter) == 1 ) 
			{
				RotateLimb (  t.charanimstates[t.tcharanimindex].obj,t.spinelimbofcharacter,0,0,0 );
			}
		}
	}

	//  reset any looping/sounds
	t.ttsnd=t.charanimstates[t.tcharanimindex].firesoundindex;
	t.charanimstates[t.tcharanimindex].firesoundindex=0;
	if (  t.ttsnd>0 ) 
	{
		if (  SoundExist(t.ttsnd) == 1 ) 
		{
			StopSound (  t.ttsnd );
		}
	}
}

void darkai_shootcharacter ( void )
{
	//  create sound for A.I when shot made
	t.tsx_f=t.twhox_f ; t.tsz_f=t.twhoz_f ; t.tradius_f=200 ;darkai_makesound ( );
	//  receives charanimindex tobj tdamage twhox# twhoy# twhoz#
	if (  t.entityelement[t.charanimstates[g.charanimindex].e].health>0 ) 
	{
		//  handle shooting of character
		t.ttte=t.charanimstates[g.charanimindex].e;
		t.tdamage=g.firemodes[t.gunid][g.firemode].settings.damage ; t.tdamageforce=t.tforce_f;
		// 100415 - melee attack can override damage
		// 011215 - specify damage using fire mode zero default
		if (  t.gun[t.gunid].settings.ismelee == 2  )  
			t.tdamage = g.firemodes[t.gunid][0].settings.meleedamage;

		t.tdamagesource=1;
		entity_applydamage ( );
	}
}

void darkai_calcplrvisible ( void )
{
	// if the ai is controlled by another player, we can just set as visible here
	if ( t.game.runasmultiplayer  ==  1 ) 
	{
		if ( t.entityelement[t.charanimstate.e].mp_coopControlledByPlayer != g.mp.me ) 
		{
			t.entityelement[t.charanimstate.e].plrvisible=0;
			t.entityelement[t.charanimstate.e].lua.flagschanged=1;
			return;
		}
	}

	// takes tcharanimindex
	// work out if entity A.I can see (stored until recalculated) (called from _darkai_loop and _darkai_shootplayer)
	t.entityelement[t.charanimstate.e].plrvisible=0;
	t.entityelement[t.charanimstate.e].lua.flagschanged=1;
	if ( t.player[t.plrid].health > 0 ) 
	{
		// work out distance between player and entity
		t.ttdx_f=ObjectPositionX(t.aisystem.objectstartindex)-ObjectPositionX(t.charanimstate.obj);
		t.ttdz_f=ObjectPositionZ(t.aisystem.objectstartindex)-ObjectPositionZ(t.charanimstate.obj);
		t.ttdd_f=Sqrt(abs(t.ttdx_f*t.ttdx_f)+abs(t.ttdz_f*t.ttdz_f));
		if (  t.ttdd_f<1500.0 ) 
		{
			//  player within 1500 units, otherwise skip further vis checking
			t.ttda_f=atan2deg(t.ttdx_f,t.ttdz_f);
			t.ttdiff_f=WrapValue(t.ttda_f)-WrapValue(ObjectAngleY(t.charanimstate.obj));
			if ( t.ttdiff_f<-180 ) t.ttdiff_f = t.ttdiff_f+360;
			if ( t.ttdiff_f>180 ) t.ttdiff_f = t.ttdiff_f-360;
			t.tconeangle=t.entityelement[t.charanimstate.e].eleprof.coneangle;
			if ( t.tconeangle == 0  ) t.tconeangle = 179; // new default
			if ( abs(t.ttdiff_f) <= t.tconeangle ) 
			{
				// and player is within hemisphere of entity look angle
				t.tgetentcanseevalue=AIGetEntityCanSee(t.charanimstate.obj,ObjectPositionX(t.aisystem.objectstartindex),ObjectPositionY(t.aisystem.objectstartindex),ObjectPositionZ(t.aisystem.objectstartindex),1);
				if ( t.tgetentcanseevalue>0 )
				{
					// player can be seen within inner arc
					t.ttokay=1;
					t.tthavegunobject=0;
					t.tgunobj=t.entityelement[t.charanimstate.e].attachmentobj;
					if ( t.tgunobj>0 ) 
					{
						if ( ObjectExist(t.tgunobj) == 1 ) 
						{
							t.brayx1_f=ObjectPositionX(t.tgunobj);
							t.brayy1_f=ObjectPositionY(t.tgunobj);
							t.brayz1_f=ObjectPositionZ(t.tgunobj);
							t.tthavegunobject=1;
						}
					}
					if ( t.tthavegunobject == 0 ) 
					{
						t.brayx1_f=ObjectPositionX(t.charanimstate.obj);
						t.brayy1_f=ObjectPositionY(t.charanimstate.obj)+20; // 070918 - raised a bit closer to character eyes 
						t.brayz1_f=ObjectPositionZ(t.charanimstate.obj);
						t.tsrcobj=g.entitybankoffset+t.entityelement[t.charanimstate.e].bankindex;
						if ( ObjectExist(t.tsrcobj) == 1 ) t.brayy1_f = t.brayy1_f + (ObjectSizeY(t.tsrcobj,1)*0.5f);
					}

					// 090417 - improve accuracy of enemy plr detection (was putting player at waste level)
					entity_gettrueplayerpos( );
					t.brayx2_f = t.tcamerapositionx_f;
					t.brayy2_f = t.tcamerapositiony_f;
					t.brayz2_f = t.tcamerapositionz_f;

					// first ensure not going through physics terrain
					if ( ODERayTerrain(t.brayx1_f,t.brayy1_f,t.brayz1_f,t.brayx2_f,t.brayy2_f,t.brayz2_f) == 1 ) 
					{
						t.ttokay=0;
					}
					else
					{
						//  140514 - actually move ray BACK a little in case enemy right up against something!
						t.ttdx_f=t.brayx2_f-t.brayx1_f;
						t.ttdy_f=t.brayy2_f-t.brayy1_f;
						t.ttdz_f=t.brayz2_f-t.brayz1_f;
						t.ttdd_f=Sqrt(abs(t.ttdx_f*t.ttdx_f)+abs(t.ttdy_f*t.ttdy_f)+abs(t.ttdz_f*t.ttdz_f));
						t.ttdx_f=t.ttdx_f/t.ttdd_f;
						t.ttdy_f=t.ttdy_f/t.ttdd_f;
						t.ttdz_f=t.ttdz_f/t.ttdd_f;
						t.brayx1_f=t.brayx1_f-(t.ttdx_f*10.0);
						t.brayy1_f=t.brayy1_f-(t.ttdy_f*10.0);
						t.brayz1_f=t.brayz1_f-(t.ttdz_f*10.0);
						//  if third person, target is an OBJ (so need to cut back dest coordinate so as not to intersect it)
						if (  t.playercontrol.thirdperson.enabled == 1 ) 
						{
							t.brayx2_f=t.brayx2_f-(t.ttdx_f*30.0);
							t.brayy2_f=t.brayy2_f-(t.ttdy_f*30.0);
							t.brayz2_f=t.brayz2_f-(t.ttdz_f*30.0);
						}
						//  (first intersectall command simply fills a secondary range of objects)
						if (  g.gnumberofraycastsallowedincycle>0 ) 
						{
							t.tttokay = 0 ; if (  t.ttdd_f<300  )  t.tttokay = 1;
							if (  g.gnumberofraycastslastoneused != t.tcharanimindex || g.gnumberofraycastsallowedincycle <= 2 || t.tttokay == 1 ) 
							{
								if (  t.tttokay == 0 && g.gnumberofraycastsallowedincycle>0  )  --g.gnumberofraycastsallowedincycle;
								g.gnumberofraycastslastoneused=t.tcharanimindex;
							}
						}
						if (  g.gnumberofraycastslastoneused == t.tcharanimindex ) 
						{
							if (g.lightmappedobjectoffset >= g.lightmappedobjectoffsetfinish)
								t.ttt = IntersectAll(87000, 87000 + g.merged_new_objects - 1, 0, 0, 0, 0, 0, 0, -123);
							else
								t.ttt=IntersectAll(g.lightmappedobjectoffset,g.lightmappedobjectoffsetfinish,t.brayx1_f,t.brayy1_f,t.brayz1_f,0,0,0,-123);

							t.tintersectvalue=IntersectAll(g.entityviewstartobj,g.entityviewendobj,t.brayx1_f,t.brayy1_f,t.brayz1_f,t.brayx2_f,t.brayy2_f,t.brayz2_f,t.charanimstate.obj);//220618 yuk >0;
							if (  t.tintersectvalue>0 ) 
							{
								t.ttokay=0;
							}
						}
						else
						{
							//  allows engine to limit expensive ray casts to a few per cycle
							t.ttokay=0;
						}
					}
					if (  t.ttokay == 1 ) 
					{
						t.entityelement[t.charanimstate.e].plrvisible=1;
						t.entityelement[t.charanimstate.e].lua.flagschanged=1;
					}
				}
			}
		}
	}	
}

void darkai_mouthandheadtracking ( void )
{
	// impose anim frame overrides on top of regular animation
	int iCharObj = t.charanimstate.obj;

	// mouth simulation
	float fTimeFromStartOfSpeak = 0;
	if ( t.charanimstate.ccpo.speak.fMouthTimeStamp == 0.0f )
	{
		// waiting for mouth timer to be started (elsewhere)
		t.charanimstate.ccpo.speak.iMouthDataShape = 0;
	}
	else
	{
		// only if mouth data
		if ( t.charanimstate.ccpo.speak.mouthData.size() > 0 )
		{
			fTimeFromStartOfSpeak = ((float)Timer()/1000.0f) - t.charanimstate.ccpo.speak.fMouthTimeStamp;
			int iMouthDataNextIndex = t.charanimstate.ccpo.speak.iMouthDataIndex;
			if ( fTimeFromStartOfSpeak > t.charanimstate.ccpo.speak.mouthData[iMouthDataNextIndex].fTimeStamp )
			{
				int iMouthDataShape = t.charanimstate.ccpo.speak.mouthData[iMouthDataNextIndex].iMouthShape;
				t.charanimstate.ccpo.speak.iMouthDataShape = iMouthDataShape;
				iMouthDataNextIndex++;
				t.charanimstate.ccpo.speak.fSmouthDataSpeedToNextShape = 4.0f;
				t.charanimstate.ccpo.speak.iMouthDataIndex = iMouthDataNextIndex;
				if ( t.charanimstate.ccpo.speak.iMouthDataIndex >= t.charanimstate.ccpo.speak.mouthData.size() )
				{
					t.charanimstate.ccpo.speak.fMouthTimeStamp = 0;
					t.charanimstate.ccpo.speak.iMouthDataIndex = 0;
				}
			}
			else
			{
				// modulate speed to final mouth shape based on closeness to next shape
				int iMouthDataCurrentIndex = iMouthDataNextIndex-1;
				if ( iMouthDataCurrentIndex >= 0 )
				{
					float fTimeDifference = t.charanimstate.ccpo.speak.mouthData[iMouthDataNextIndex].fTimeStamp - t.charanimstate.ccpo.speak.mouthData[iMouthDataCurrentIndex].fTimeStamp;
					float fTimeToNextShape = fTimeFromStartOfSpeak - t.charanimstate.ccpo.speak.mouthData[iMouthDataCurrentIndex].fTimeStamp;
					t.charanimstate.ccpo.speak.fSmouthDataSpeedToNextShape = 1.0f + ((1.0f-(fTimeToNextShape/fTimeDifference))*3.0f);
				}
			}
		}
	}
	sObject* pCharObject = GetObjectData ( iCharObj );
	int iFinalFrameToUse = t.charanimstate.ccpo.speak.iMouthDataShape;
	if ( iFinalFrameToUse == 0 ) 
	{
		iFinalFrameToUse = 12;
		if ( t.charanimstate.ccpo.speak.fNeedToBlink > 1.0f ) 
		{
			// randomise blink (maybe use blink logic in future)
			t.charanimstate.ccpo.speak.fNeedToBlink = -0.05f;
		}
		if ( t.charanimstate.ccpo.speak.fNeedToBlink < 0.0f ) 
		{
			// allow blink for a few frames
			t.charanimstate.ccpo.speak.fSmouthDataSpeedToNextShape = 5.0f;
			iFinalFrameToUse = 13;
		}
	}
	if ( t.charanimstate.ccpo.speak.fNeedToBlink > 0.0f )
	{
		double dPowerRandom = rand()%10000;
		if ( dPowerRandom > 9900.0 )
			dPowerRandom = 5.0;
		else
			dPowerRandom = dPowerRandom/100000.0;
		t.charanimstate.ccpo.speak.fNeedToBlink+=0.0001f+(float)(dPowerRandom/10.0f);
	}
	else
		t.charanimstate.ccpo.speak.fNeedToBlink+=0.01f; 

	// clever system to reset pose to use a specific frame, and then allow regular animation to transform on top of it
	sFrame* pFrameOfLimb = pCharObject->ppFrameList[t.charanimstate.ccpo.settings.iNeckBone];
	if (pFrameOfLimb)
	{
		WickedCall_SetObjectPreFrames(pCharObject, "Bip01_Head", iFinalFrameToUse, t.charanimstate.ccpo.speak.fSmouthDataSpeedToNextShape, 2);
	}

	// Neck Bone adjust as part of manual and regular anim calculation
	if (t.charanimstate.ccpo.settings.iNeckBone > 0)
	{
		sFrame* pFrameOfLimb = pCharObject->ppFrameList[t.charanimstate.ccpo.settings.iNeckBone];
		if (pFrameOfLimb)
		{
			WickedCall_RotateLimb(pCharObject, pFrameOfLimb, t.charanimstate.neckRightAndLeft, t.charanimstate.neckUpAndDown, 0);
		}
	}
}

int darkai_loop_counter = 0;
void darkai_loop ( void )
{
	
	darkai_loop_counter++;

	//  Handle Player AIStates
	AISetPlayerDucking (  t.aisystem.playerducking );
	AISetPlayerContainer (  t.aisystem.playercontainerid );

	//  allow two raycasts per cycle (and one of them needs to be unique)
	g.gnumberofraycastsallowedincycle=5;

	//  tic down nearby count
	if (  g.aidetectnearbycount > 0  )  --g.aidetectnearbycount;

	//  all characters in game
	for ( g.charanimindex = 1 ; g.charanimindex<=  g.charanimindexmax; g.charanimindex++ )
	{
		//  This character
		t.charanimstate = t.charanimstates[g.charanimindex];

		//PE: Lua calls darkai_calcplrvisible 2 times per char per sync, very expensive, only do one check.
		t.entityelement[t.charanimstate.e].bPlrVisibleCheckDone = false;

		//  Entity Element Index for this A.I character
		t.i=t.charanimstate.obj ; t.te=t.charanimstate.e;

		//  ensure can stop looping sound ANY time
		t.ttsnd=t.charanimstate.firesoundindex;
		if (  t.ttsnd>0 ) 
		{
			if (  SoundExist(t.ttsnd) == 1 ) 
			{
				if (  Timer()>(int)t.charanimstate.firesoundexpiry ) 
				{
					StopSound (  t.ttsnd );
				}
				if (  SoundPlaying(t.ttsnd) == 0 ) 
				{
					t.charanimstate.firesoundindex=0;
				}
			}
		}

		//  is entiy active here?
		if (  t.entityelement[t.te].active == 1 ) 
		{

		//  Controls distance at which all characters freeze back to instances
		entity_getmaxfreezedistance ( );

		//  Allows character that would normally be out of range to come alive for a while
		//  Is used when someone is shot by a sniper and those nearby react
		if (  g.aidetectnearbymode == 1 ) 
		{
			if (  g.aidetectnearbycount>0 ) 
			{
				t.dx_f=g.aidetectnearbymodeX_f-t.entityelement[t.charanimstate.e].x;
				t.dz_f=g.aidetectnearbymodeZ_f-t.entityelement[t.charanimstate.e].z;
				t.tdist_f=Sqrt(abs(t.dx_f*t.dx_f)+abs(t.dz_f*t.dz_f));
				if (  t.tdist_f < 300.0 ) 
				{
					t.entityelement[t.charanimstate.e].plrdist = t.maximumnonefreezedistance / 2.0;
				}
			}
			else
			{
				g.aidetectnearbymode = 0;
			}
		}

		//  Ensure characters can be placed back in range immediately if needed
		if (  t.charanimstate.outofrange == 1 && (t.entityelement[t.charanimstate.e].plrdist<t.maximumnonefreezedistance || t.entityelement[t.charanimstate.e].health <= 0) ) 
		{
			//  Back in range
			t.charanimstate.outofrange=0;

			//  Activate object
			if (  AIEntityExist(t.i) == 1  )  AISetEntityActive (  t.i,1 );

			//  Delete old instance obj to use animatable CloneObject (  )
			t.obj=t.charanimstate.obj;
			t.tentid=t.entityelement[t.charanimstate.e].bankindex;
			t.tte=t.charanimstate.e ; entity_converttoclone ( );
			entity_setupcharobjsettings ( );

			// always restore unfrozen characters in idle pose
			if ( t.entityprofile[t.tentid].animmax>0 && t.entityprofile[t.tentid].playanimineditor>0 ) 
			{
				t.q=t.entityprofile[t.tentid].playanimineditor-1;
				LoopObject ( t.obj,t.entityanim[t.tentid][t.q].start,t.entityanim[t.tentid][t.q].finish );
				t.tfinalspeed_f=t.entityelement[t.charanimstate.e].speedmodulator_f*t.charanimstate.animationspeed_f*2.5*g.timeelapsed_f;
				SetObjectSpeed ( t.obj,t.tfinalspeed_f );
			}
		}

		//  For valid A.I entities
		if (  AIEntityExist(t.i) == 1 && t.entityelement[t.charanimstate.e].health>0 ) 
		{
			//  If in range for activity
			if (  t.entityelement[t.charanimstate.e].plrdist<t.maximumnonefreezedistance ) 
			{

				//PE: Spread out load, even e in one sync and unenven in next.
				//PE: This is really expensive, and visiondelay is not used.
				//PE: Char should not move that much in 2 sync , so should not be a problem , also "shooting" makes a new darkai_calcplrvisible.
				if ( (darkai_loop_counter+t.charanimstate.e) % 2 == 0)
				{
					//  Only check for ray visibility when triggered (expensive task)
					if (Timer() > (int)t.charanimstate.visiondelaylastime + t.charanimstate.visiondelay)
					{
						//  switches to next char index in sycn with visiondelay
						t.charanimstate.visiondelaylastime = Timer();
						t.tcharanimindex = g.charanimindex;
						darkai_calcplrvisible();
					}
				}
				// character is active, in range, process mouth and head tracking
				if( g_ObjectList[t.charanimstate.obj]->iFrameCount >= 37 ) //PE: generated errors on old models.
				darkai_mouthandheadtracking();
			}
			else
			{
				//  Freeze All A.I for character out of range
				if (  t.charanimstate.outofrange == 0 ) 
				{
					//  Character is out of range
					t.charanimstate.outofrange=1;

					//  reset for when resume
					if ( t.charanimstate.playcsi != g.csi_limbo ) t.charanimstate.playcsi = g.csi_unarmed;
					t.charanimstate.alerted=0;

					//  capture latest position for later resume
					t.obj=t.charanimstate.obj;
					t.entityelement[t.charanimstate.e].x=ObjectPositionX(t.obj);
					t.entityelement[t.charanimstate.e].y=ObjectPositionY(t.obj);
					t.entityelement[t.charanimstate.e].z=ObjectPositionZ(t.obj);
					t.entityelement[t.charanimstate.e].ry=ObjectAngleY(t.obj);

					// Delete old obj and use cheaper InstanceObject (  )
					t.tentid=t.entityelement[t.charanimstate.e].bankindex;
					// converting to instance makes them animate again!
					t.tte=t.charanimstate.e ; entity_converttoinstance ( );
					entity_setupcharobjsettings ( );

					//  animate character object parent in standard idle post
					t.tttsourceobj=g.entitybankoffset+t.entityelement[t.charanimstate.e].bankindex;
					if (  ObjectExist(t.tttsourceobj) == 1 ) 
					{
						if (  t.entityprofile[t.tentid].animmax>0 && t.entityprofile[t.tentid].playanimineditor>0 ) 
						{
							t.q=t.entityprofile[t.tentid].playanimineditor-1;
							LoopObject (  t.tttsourceobj,t.entityanim[t.tentid][t.q].start,t.entityanim[t.tentid][t.q].finish );
							t.tfinalspeed_f=t.entityelement[t.charanimstate.e].speedmodulator_f*t.charanimstate.animationspeed_f*2.5*g.timeelapsed_f;
							SetObjectSpeed (  t.tttsourceobj,t.tfinalspeed_f );
						}
						//  for intense CPU animators, stop anim when in distance
						if (  t.entityprofile[t.tentid].cpuanims != 0  )  StopObject (  t.tttsourceobj );
					}

					//  Deactivate out of range character
					if (  AIEntityExist(t.i) == 1  )  AISetEntityActive (  t.i,0 );

				}

			//  end of 'if in range for activity'
			}

		}
		else
		{

			//  A.I not part of simulation

		}

		//  Active else branch
		}
		else
		{

		//  stop LoopSound (  if underlying AI deactives )
		t.ttsnd=t.charanimstate.firesoundindex;
		t.charanimstate.firesoundindex=0;
		if (  t.ttsnd>0 ) 
		{
			if (  SoundExist(t.ttsnd) == 1 ) 
			{
				StopSound (  t.ttsnd );
			}
		}

		//  Active branch
		}

		//  Handle character removal
		if (  t.entityelement[t.charanimstate.e].health <= 0 && t.charanimstate.timetofadeout>0 ) 
		{
			if (  Timer()>t.charanimstate.timetofadeout ) 
			{
				t.txDist_f = ObjectPositionX(t.charanimstate.obj) - CameraPositionX(0);
				t.tzDist_f = ObjectPositionZ(t.charanimstate.obj) - CameraPositionZ(0);
				if (  t.txDist_f * t.txDist_f + t.tzDist_f * t.tzDist_f > 500000 ) 
				{
					if (  GetInScreen(t.charanimstate.obj)  ==  0 ) 
					{
						// disable ability to remove character from system if ALWAYS ACTIVE has been set
						// (allows characters to respawn)
						if ( t.entityelement[t.charanimstate.e].eleprof.phyalways == 0 )
						{
							darkai_character_remove ( );
						}
					}
				}
			}
		}

		//  Store any changes
		t.charanimstates[g.charanimindex] = t.charanimstate;
	}
}

void darkai_update ( void )
{
	//  Update A.I system
	if (  t.visuals.debugvisualsmode<99 ) 
	{
		AIUpdate (  );
	}
}

void darkai_character_remove_charpart ( void )
{
	//  if ragdoll, remove this now
	t.tte=t.charanimstate.originale;
	entity_freeragdoll ( );

	//  switch back to instance
	t.charanimstate.fadeoutvalue_f=0.0;
	t.charanimstate.timetofadeout=0;
}

void darkai_character_remove ( void )
{
	darkai_character_remove_charpart ( );
	entity_converttoinstance ( );
	//  and COMPLETELY remove it from map
	t.entityelement[t.tte].x=-100000;
	t.entityelement[t.tte].y=-100000;
	t.entityelement[t.tte].z=-100000;
	if (  ObjectExist(t.charanimstate.obj) == 1 ) 
	{
		//  HideObject (  and move away )
		HideObject (  t.charanimstate.obj );
		PositionObject (  t.charanimstate.obj,t.entityelement[t.tte].x,t.entityelement[t.tte].y,t.entityelement[t.tte].z );
	}
	// 120416 - hide any attachment to entity
	if ( t.entityelement[t.tte].eleprof.cantakeweapon == 0 )
	{
		// if not meant to be collected after char disappears
		t.tattobj=t.entityelement[t.tte].attachmentobj;
		if ( t.tattobj>0  )  
		{
			if ( ObjectExist( t.tattobj ) == 1 )
			{
				ODEDestroyObject ( t.tattobj );
				HideObject ( t.tattobj );
			}
		}
	}
}

