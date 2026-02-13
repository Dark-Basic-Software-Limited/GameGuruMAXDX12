//----------------------------------------------------
//--- GAMEGURU - G-Entity
//----------------------------------------------------

#include "stdafx.h"
#include "gameguru.h"
#include "CObjectsC.h"

#include "..\Imgui\imgui.h"
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "..\Imgui\imgui_internal.h"
#include "..\Imgui\imgui_impl_win32.h"
#include "..\Imgui\imgui_gg_dx11.h"

#include ".\..\..\Guru-WickedMAX\wickedcalls.h"
#include ".\\..\..\\Guru-WickedMAX\\GPUParticles.h"
using namespace GPUParticles;

#include "GGRecastDetour.h"
extern GGRecastDetour g_RecastDetour;

#ifdef OPTICK_ENABLE
#include "optick.h"
#endif

// Globals
std::vector<int> g_iDestroyedEntitiesList;

// 
//  ENTITY GAME CODE
// 

void entity_init_overwritefireratesettings (void)
{
	// when all entities loaded, some contain override settings for weapons,
	// so ensure they are used after the weapon default settings are loaded
	// so new weapons added to level and tested reflect overrides
	for (t.e = 1; t.e <= g.entityelementlist; t.e++)
	{
		t.entid = t.entityelement[t.e].bankindex;
		if (t.entid > 0)
		{
			//  update gun/flak settings from latest entity properties
			t.tgunid_s = t.entityprofile[t.entid].isweapon_s;
			entity_getgunidandflakid ();
			if (t.tgunid > 0)
			{
				// entity properties should only edit first primary gun settings (so we dont mess up enhanced weapons)
				int firemode = 0;
				g.firemodes[t.tgunid][firemode].settings.damage = t.entityelement[t.e].eleprof.damage;
				g.firemodes[t.tgunid][firemode].settings.accuracy = t.entityelement[t.e].eleprof.accuracy;
				g.firemodes[t.tgunid][firemode].settings.reloadqty = t.entityelement[t.e].eleprof.reloadqty;
				g.firemodes[t.tgunid][firemode].settings.iterate = t.entityelement[t.e].eleprof.fireiterations;
				g.firemodes[t.tgunid][firemode].settings.range = t.entityelement[t.e].eleprof.range;
				g.firemodes[t.tgunid][firemode].settings.dropoff = t.entityelement[t.e].eleprof.dropoff;
				g.firemodes[t.tgunid][firemode].settings.usespotlighting = t.entityelement[t.e].eleprof.usespotlighting;
				g.firemodes[t.tgunid][firemode].settings.clipcapacity = t.entityelement[t.e].eleprof.clipcapacity;
				g.firemodes[t.tgunid][firemode].settings.weaponpropres1 = t.entityelement[t.e].eleprof.weaponpropres1;
				g.firemodes[t.tgunid][firemode].settings.weaponpropres2 = t.entityelement[t.e].eleprof.weaponpropres2;
			}
		}
	}
}

void entity_init ( void )
{
	// all entities initialised, and any old destroy list items cleared
	g_iDestroyedEntitiesList.clear();
	 
	//  pre-create element data (load from eleprof)
	timestampactivity(0,"Configure entity instances for use");
	for ( t.e = 1 ; t.e<=  g.entityelementlist; t.e++ )
	{
		t.entid=t.entityelement[t.e].bankindex;
		entity_configueelementforuse ( );
	}

	//  activate all entities and perform any pre-test game setup
	timestampactivity(0,"Configure entity attachments and AI obstacles");
	g.entityattachmentindex=0;

	//PE: Could we use collisionmode == 0 and only create it as a box ? and not all faces.
	//PE: t.tobstype=t.entityprofile[t.entid].forcesimpleobstacle = true;
	//PE: This takes 30 sec. and take 400 MB. mem in FatherIsland, perhaps another faster way could be made.
	for ( t.e = 1 ; t.e <= g.entityelementlist; t.e++ )
	{
		t.entid=t.entityelement[t.e].bankindex;
		if (  t.entid>0 ) 
		{
			// Activate entity
			if (t.entityelement[t.e].eleprof.spawnatstart == 1 )
			{
				// LB: only activate entities that are set to spawn at start
				t.entityelement[t.e].active = 1;
			}
			t.tobj=t.entityelement[t.e].obj;
			if (  t.tobj>0 ) 
			{
				//  if object exists
				if (  ObjectExist(t.tobj) == 1 ) 
				{
					//  Create attachents for entity
					//  Reset AI Obstacle Center references (used later by physics placement)
					t.entityelement[t.e].abscolx_f=-1;
					t.entityelement[t.e].abscolz_f=-1;
					t.entityelement[t.e].abscolradius_f=-1;
					//  Create AI obstacles for all static entities
					if (  t.entityprofile[t.entid].ismarker == 0 ) 
					{
						bool bSceneStatic = false;
						if ( t.entityelement[t.e].staticflag == 1 ) bSceneStatic = true;

						// leelee, sometimes want to make dynamic immobile entities AI obstacles!! (exploding crate in Escape level)
						if ( bSceneStatic == true && t.entityprofile[t.entid].collisionmode != 11 && t.entityprofile[t.entid].collisionmode != 12 ) 
						{
							t.tfullheight=1;
							t.tcontainerid=0;
							if (  t.entityprofile[t.entid].collisionmode >= 50 && t.entityprofile[t.entid].collisionmode<60 ) 
							{
								t.ttreemode=t.entityprofile[t.entid].collisionmode-50;
							}
						}
					}
					//  ensure all transparent static objects are removed from 'intersect all' consideration
					t.tokay=0;
					if (  t.entityelement[t.e].staticflag == 1 ) 
					{
						if (  t.entityprofile[t.entid].canseethrough == 1 ) 
						{
							t.tokay=1;
						}
					}
					if (  t.entityprofile[t.entid].ischaracter == 0 ) 
					{
						if (  t.entityprofile[t.entid].collisionmode == 11  )  t.tokay = 1;
					}
					if (  t.tokay == 1 ) 
					{
						SetObjectCollisionProperty (  t.entityelement[t.e].obj,1 );
					}
					//  ensure all transparency modes set for each entity
					if (  t.entityprofile[t.entid].ismarker == 0 ) 
					{
						//PE: Wicked material can overwrite objects settings.
						if (t.entityelement[t.e].eleprof.WEMaterial.MaterialActive) {
							WickedSetEntityId(t.entid);
							WickedSetElementId(t.e);
							SetObjectTransparency(t.entityelement[t.e].obj, t.entityelement[t.e].eleprof.WEMaterial.bTransparency[0]);
							WickedSetEntityId(-1);
							WickedSetElementId(0);
						}
						else 
						{
							int iNeverFive = t.entityelement[t.e].eleprof.transparency;
							if (iNeverFive == 5) iNeverFive = 6;
							WickedSetEntityId(t.entid);
							WickedSetElementId(t.e);
							SetObjectTransparency(t.entityelement[t.e].obj, iNeverFive);
							WickedSetEntityId(-1);
							WickedSetElementId(0);
						}
					}
					// ensure correct zdepth when game level starts
					entity_preparedepth(t.entid, t.tobj);
				}
			}

			// reset prescanned vis lit at start of level
			t.entityelement[t.e].iPreScanVisibleCurrent = 0;
			t.entityelement[t.e].iPreScannedVisible.clear();
		}

		// reset spawn flags (even ones that are blank (i.e, bankindex=0)
		t.entityelement[t.e].iWasSpawnedInGame = 0;
	}
}

void entity_init_nowcreateattachments (void)
{
	// Create attachents for entity
	for (t.e = 1; t.e <= g.entityelementlist; t.e++)
	{
		t.entid = t.entityelement[t.e].bankindex;
		if (t.entid > 0)
		{
			t.tobj = t.entityelement[t.e].obj;
			if (t.tobj > 0)
			{
				if (ObjectExist(t.tobj) == 1)
				{
					entity_createattachment ();
				}
			}
		}
	}
}

void entity_bringnewentitiestolife (bool bAllNewOnes)
{
	// scan if any new entities are characters, if so, scan for adding them to character array
	bool bNewEntityIsCharacter = false;
	for (t.e = 1; t.e <= g.entityelementlist; t.e++)
	{
		t.entid = t.entityelement[t.e].bankindex;
		if (t.entid > 0)
		{
			if (t.entityprofile[t.entid].isebe == 0)
			{
				if (t.entityelement[t.e].active == 2)
				{
					t.tobj = t.entityelement[t.e].obj;
					if (t.tobj > 0)
					{
						//  if object exists
						if (ObjectExist(t.tobj) == 1)
						{
							// Only redo script for characters (as they were wiped out)
							if (t.entityprofile[t.entid].ischaracter == 1) 
							{
								bNewEntityIsCharacter = true;
								break;
							}
						}
					}
				}
			}
		}
	}
	if (bNewEntityIsCharacter == true)
	{
		extern void darkai_refresh_characters(bool);
		darkai_refresh_characters(true);
	}

	// new entities have active=2
	for ( t.e = 1 ; t.e<=  g.entityelementlist; t.e++ )
	{
		t.entid = t.entityelement[t.e].bankindex;
		if ( t.entid>0 ) 
		{
			if ( t.entityprofile[t.entid].isebe == 0 )
			{
				if ( t.entityelement[t.e].active == 2 ) 
				{
					t.tobj = t.entityelement[t.e].obj;
					if ( t.tobj>0 ) 
					{
						//  if object exists
						if ( ObjectExist(t.tobj) == 1 ) 
						{
							// Only redo script for characters (as they were wiped out)
							if ( t.entityprofile[t.entid].ischaracter==1 || bAllNewOnes == true )
							{
								//  Launch the entity AI
								lua_loadscriptin ( );
								//  Launch init script
								lua_initscript ( );
								//  configure new entity for action
								entity_configueelementforuse ( );
								//  Create attachents for entity
								entity_createattachment ( );
							}
						}
					}
				}
			}
		}
	}

	// 161115 - now restore ALL entities back to actve=1 (now new entities determined)
	for ( t.e = 1 ; t.e<=  g.entityelementlist; t.e++ )
	{
		t.entid=t.entityelement[t.e].bankindex;
		if ( t.entid>0 ) 
		{
			if ( t.entityprofile[t.entid].isebe == 0 )
			{
				if ( t.entityelement[t.e].active == 2 || t.entityelement[t.e].active == 3 || t.entityelement[t.e].active == 4 ) 
				{
					// show attachment
					if (  t.entityelement[t.e].attachmentobj>0 ) 
					{
						if (  ObjectExist(t.entityelement[t.e].attachmentobj) == 1 ) 
						{
							ShowObject (  t.entityelement[t.e].attachmentobj );
						}
					}

					// 171115 - only [3 with character] requires characters to be restored to clones
					if (  t.entityelement[t.e].active == 3 ) 
					{
						t.tte = t.e; 
						entity_converttoclone ( );
					}

					// restore active flag for in-game use
					if (t.entityelement[t.e].collected >= 3)
					{
						// remain zero, objects in shop/chest must remain inert until moved to plrinventory/hotkeypanel!
						t.entityelement[t.e].active = 0;
					}
					else
					{
						t.entityelement[t.e].active = 1;
					}
				}
			}
		}
	}
}

float g_fActivationWaveDistance = 0.0f;

void entity_initafterphysics ( void )
{
	//  Handle spawn entities
	for ( t.e = 1 ; t.e<=  g.entityelementlist; t.e++ )
	{
		t.entid=t.entityelement[t.e].bankindex;
		if (  t.entid>0 ) 
		{
			if (  t.entityelement[t.e].eleprof.spawnatstart == 0 ) 
			{
				t.entityelement[t.e].active=0;
				entity_lua_hide ( );
				entity_lua_collisionoff ( );
			}
			else
			{
				entity_lua_collisionon ( );
			}
		}
	}

	// special wave to activate physics in a sequence
	g_fActivationWaveDistance = 0.0f;
}

void entity_refreshelementforuse ( void )
{
	// reset entity flags when entity is new (renewed, i.e. load game)

	// internal entity speed modulator defaults at 1.0
	t.entityelement[t.e].speedmodulator_f=1.0;

	// reset explosion state
	t.entityelement[t.e].explodefusetime=0;

	// reset ragdoll state
	t.entityelement[t.e].ragdollified=0;
}

void entity_configueelementforuse ( void )
{
	//  Spawn values
	t.entityelement[t.e].spawn.atstart=t.entityelement[t.e].eleprof.spawnatstart;
	t.entityelement[t.e].spawn.max=t.entityelement[t.e].eleprof.spawnmax;
	t.entityelement[t.e].spawn.delay=t.entityelement[t.e].eleprof.spawndelay;
	t.entityelement[t.e].spawn.qty=t.entityelement[t.e].eleprof.spawnqty;
	t.entityelement[t.e].spawn.upto=t.entityelement[t.e].eleprof.spawnupto;
	t.entityelement[t.e].spawn.afterdelay=t.entityelement[t.e].eleprof.spawnafterdelay;
	t.entityelement[t.e].spawn.whendead=t.entityelement[t.e].eleprof.spawnwhendead;
	t.entityelement[t.e].spawn.delayrandom=t.entityelement[t.e].eleprof.spawndelayrandom;
	t.entityelement[t.e].spawn.qtyrandom=t.entityelement[t.e].eleprof.spawnqtyrandom;
	t.entityelement[t.e].spawn.vel=t.entityelement[t.e].eleprof.spawnvel;
	t.entityelement[t.e].spawn.velrandom=t.entityelement[t.e].eleprof.spawnvelrandom;
	t.entityelement[t.e].spawn.angle=t.entityelement[t.e].eleprof.spawnangle;
	t.entityelement[t.e].spawn.anglerandom=t.entityelement[t.e].eleprof.spawnanglerandom;
	t.entityelement[t.e].spawn.life=t.entityelement[t.e].eleprof.spawnlife;
	if (  t.entityelement[t.e].spawn.atstart == 0 && t.entityelement[t.e].spawn.max == 0 ) 
	{
		t.entityelement[t.e].spawn.max=1;
		if (  t.entityelement[t.e].spawn.afterdelay == 0 && t.entityelement[t.e].spawn.whendead == 0 ) 
		{
			t.entityelement[t.e].spawn.afterdelay=1;
		}
		if (  t.entityelement[t.e].spawn.qty == 0  )  t.entityelement[t.e].spawn.qty = 1;
		if (  t.entityelement[t.e].spawn.upto == 0  )  t.entityelement[t.e].spawn.upto = 1;
	}

	//  Configure health from strength
	if (  t.entityelement[t.e].eleprof.strength>0 ) 
	{
		if (strcmp(t.entityelement[t.e].eleprof.aimain_s.Get(), "animals\\bird.lua") == 0)
		{
			// Birds will freeze when health reaches 0. We can remove this if script is updated to handle death.
			t.entityelement[t.e].health = 99999999;

			// Birds are also interfering with ally enemy detection (and cannot be killed at the moment, so no point in them having their allegiance changed)
			t.entityelement[t.e].eleprof.iCharAlliance = 2;//Neutral
		}
		else
		{
			t.entityelement[t.e].health = t.entityelement[t.e].eleprof.strength;
		}
	}
	else
	{
		t.entityelement[t.e].health=1;
	}

	//  Resolve default weapon gun ids
	if (  t.entityelement[t.e].eleprof.hasweapon_s != "" ) 
	{
		t.findgun_s = Lower( t.entityelement[t.e].eleprof.hasweapon_s.Get() ) ; 
		gun_findweaponindexbyname ( );
		t.entityelement[t.e].eleprof.hasweapon=t.foundgunid;
		if (  t.foundgunid>0 && t.entityprofile[t.entid].isammo == 0  )  t.gun[t.foundgunid].activeingame = 1;
	}
	else
	{
		t.entityelement[t.e].eleprof.hasweapon=0;
	}

	// Reset general flags when entity is newified
	entity_refreshelementforuse();
}

void entity_freeragdoll ( void )
{
	if (  t.entityelement[t.tte].ragdollified == 1 ) 
	{
		t.tphyobj=t.entityelement[t.tte].obj ; ragdoll_destroy ( );
		t.entityelement[t.tte].ragdollified=0;
	}
}

void entity_resetlimbtwists(sObject* pObject, int e)
{
	// reset any animation motion within the object
	if (pObject)
	{
		if (pObject->ppFrameList)
		{
			if (pObject->dwSpineCenterLimbIndex > 0)
			{
				sFrame* pFrame = pObject->ppFrameList[pObject->dwSpineCenterLimbIndex];
				if (pFrame)
				{
					WickedCall_SetBip01Position(pObject, pFrame, 0, 0, 0);
				}
			}
		}
		pObject->bSpineTrackerMoving = false;

		// find this character
		int entid = t.entityelement[e].bankindex;
		t.ttte = e;
		entity_find_charanimindex_fromttte();
		if (t.tcharanimindex > 0)
		{
			t.charanimstate = t.charanimstates[t.tcharanimindex];
			// reset the spine twists
			t.charanimstate.spineRightAndLeft = 0;
			t.charanimstate.spineYAdjust = 0;
			t.charanimstate.spineZAdjust = 0;
			int iFrameIndex = t.entityprofile[entid].spine2;
			if (iFrameIndex > 0)
			{
				sFrame* pFrameOfLimb = pObject->ppFrameList[iFrameIndex];
				if (pFrameOfLimb)
				{
					WickedCall_RotateLimb(pObject, pFrameOfLimb, t.charanimstate.spineRightAndLeft, t.charanimstate.spineYAdjust, t.charanimstate.spineZAdjust);
				}
			}
			// reset head
			t.charanimstate.neckRightAndLeft = 0;
			t.charanimstate.neckUpAndDown = 0;
			if (t.charanimstate.ccpo.settings.iNeckBone > 0)
			{
				sFrame* pFrameOfLimb = pObject->ppFrameList[t.charanimstate.ccpo.settings.iNeckBone];
				if (pFrameOfLimb)
				{
					WickedCall_RotateLimb(pObject, pFrameOfLimb, t.charanimstate.neckRightAndLeft, t.charanimstate.neckUpAndDown, 0);
				}
			}
			// put back to main array (probably redundant as will be reconstructed when next in game level)
			t.charanimstates[t.tcharanimindex] = t.charanimstate;
		}
	}
}

void entity_free ( void )
{
	// free any ragdoll plus objects
	for (int iSpareRagdollPlusPhyObj = g.ragdollplussystemobjstart; iSpareRagdollPlusPhyObj <= g.ragdollplussystemobjfinish; iSpareRagdollPlusPhyObj++)
	{
		if (ObjectExist(iSpareRagdollPlusPhyObj) == 1)
		{
			ODEDestroyObject(iSpareRagdollPlusPhyObj);
			DeleteObject(iSpareRagdollPlusPhyObj);
		}
	}

	if (ObjectExist(g.ragdollplussystemdebugobj) == 1)
	{
		DeleteObject(g.ragdollplussystemdebugobj);
	}
	
	// close down game entities
	for ( t.e = 1 ; t.e<=  g.entityelementlist; t.e++ )
	{
		t.entid=t.entityelement[t.e].bankindex;
		t.obj=t.entityelement[t.e].obj;
		if (  t.entid>0 && t.obj>0 ) 
		{
			if ( ObjectExist(t.obj) == 1 ) 
			{
				// ensure all ragdolls/dynamic entities are restored
				if ( t.entityprofile[t.entid].ismarker == 0 && t.entityprofile[t.entid].isebe == 0 ) 
				{
					// and restore object for the editor
					t.tte = t.e ; entity_converttoinstance ( );
					PositionObject (  t.obj,t.entityelement[t.e].x,t.entityelement[t.e].y,t.entityelement[t.e].z );
					RotateObject (  t.obj,t.entityelement[t.e].rx,t.entityelement[t.e].ry,t.entityelement[t.e].rz );
					t.tentid=t.entid ; t.tte=t.e ; t.tobj=t.obj ; entity_resettodefaultanimation ( );
					ShowObject (  t.obj );

					sObject* pObject = GetObjectData(t.obj);
					if (t.entityprofile[t.entid].ischaracter == 1)
					{
						// reset any animation motion within the object
						entity_resetlimbtwists(pObject, t.e);
						// new ragdoll plus wipes pivot, need this restoring
						RotateLimb(t.obj, 0, 0, 0, 0);
						RotateObject (t.obj, t.entityelement[t.e].rx, t.entityelement[t.e].ry, t.entityelement[t.e].rz);
					}
					else
					{
						// simply reset the animation frame if not a character object
						t.tte = t.e; entity_resettodefaultanimation();
					}
					// and ensure wicked updates the object 
					pObject->bAnimUpdateOnce = true;
					WickedCall_UpdateObject(pObject);
					// also force the new frame (skipping the slerp as we only have one update once pass here)
					WickedCall_InstantObjectFrameUpdate(pObject);
				}
			}
			entity_freeattachment ( );
			t.entityelement[t.e].active=0;
			t.entityelement[t.e].ragdollifiedplusphyobj = 0;
			t.entityelement[t.e].ragdollplusactivate = 0;
			t.entityelement[t.e].ragdollplusweapontypeused = 0;
		}
		// release clonse sounds before leave
		if ( t.entityelement[t.e].soundset>0  )  deleteinternalsound(t.entityelement[t.e].soundset);
		if ( t.entityelement[t.e].soundset<0  )  DeleteAnimation (  abs(t.entityelement[t.e].soundset) );
		t.entityelement[t.e].soundset=0;
		if ( t.entityelement[t.e].soundset1>0  )  deleteinternalsound(t.entityelement[t.e].soundset1);
		if ( t.entityelement[t.e].soundset1<0  )  DeleteAnimation (  abs(t.entityelement[t.e].soundset1) );
		t.entityelement[t.e].soundset1=0;
		if ( t.entityelement[t.e].soundset2>0  )  deleteinternalsound(t.entityelement[t.e].soundset2);
		t.entityelement[t.e].soundset2=0;
		if ( t.entityelement[t.e].soundset3>0  )  deleteinternalsound(t.entityelement[t.e].soundset3);
		t.entityelement[t.e].soundset3 = 0;
		t.entityelement[t.e].soundset4 = 0;
		if (t.entityelement[t.e].soundset5 > 0)  deleteinternalsound(t.entityelement[t.e].soundset5);
		t.entityelement[t.e].soundset5 = 0;
		if (t.entityelement[t.e].soundset6 > 0)  deleteinternalsound(t.entityelement[t.e].soundset6);
		t.entityelement[t.e].soundset6 = 0;
		// reset any animation motion within the object
		sObject* pObject = GetObjectData(t.obj);
		entity_resetlimbtwists(pObject, t.e);
	}
}

void entity_reset_defaults(void)
{
	t.entityelement[t.e].lipset.clear();
	t.entityelement[t.e].lipset1.clear();
	t.entityelement[t.e].lipset2.clear();
	t.entityelement[t.e].lipset3.clear();
	t.entityelement[t.e].lipset4.clear();
	t.entityelement[t.e].climbgravity = 0.0f;
	t.entityelement[t.e].lastfootfallsound = 0;
	t.entityelement[t.e].lastfootfallframeindex = -1;
	t.entityelement[t.e].iHasParentIndex = 0;
	t.entityelement[t.e].detectedlimbhit = 0;
	t.entityelement[t.e].beenmoved = 0;
	t.entityelement[t.e].underground = 0;
	t.entityelement[t.e].mp_rotateTimer = 0;
	t.entityelement[t.e].mp_rotateType = 0;
	t.entityelement[t.e].mp_isLuaChar = 0;
	t.entityelement[t.e].mp_lastUpdateSent = 0;
	t.entityelement[t.e].mp_updateOn = 0;
	t.entityelement[t.e].mp_coopLastTimeSwitchedTarget = 0;
	t.entityelement[t.e].mp_coopControlledByPlayer = 0;
	t.entityelement[t.e].mp_killedby = 0;
	t.entityelement[t.e].mp_networkkill = 0;
	t.entityelement[t.e].overpromptuse3D = false;
	t.entityelement[t.e].overprompt3dX = 0;
	t.entityelement[t.e].overprompt3dY = 0;
	t.entityelement[t.e].overprompt3dZ = 0;
	t.entityelement[t.e].overprompt3dAY = 0;
	t.entityelement[t.e].overprompt3dFaceCamera = false;
	t.entityelement[t.e].overprompttimer = 0;
	t.entityelement[t.e].overprompt_s = "";
	t.entityelement[t.e].editorlock = 0;
	t.entityelement[t.e].ragdollplusactivate = 0;
	t.entityelement[t.e].ragdollplusweapontypeused = 0;
	t.entityelement[t.e].ragdollifiedplusphyobj = 0;
	t.entityelement[t.e].ragdollifiedforcelimb = 0;
	t.entityelement[t.e].ragdollifiedforcevalue_f = 0.0f;
	t.entityelement[t.e].ragdollifiedforcez_f = 0.0f;
	t.entityelement[t.e].ragdollifiedforcey_f = 0.0f;
	t.entityelement[t.e].ragdollifiedforcex_f = 0.0f;
	t.entityelement[t.e].ragdollified = 0;
	t.entityelement[t.e].particleemitterid = 0;
	t.entityelement[t.e].characterSoundBankNumber = 0;
	t.entityelement[t.e].abscolradius_f = 0.0f;
	t.entityelement[t.e].abscolz_f = 0.0f;
	t.entityelement[t.e].abscolx_f = 0.0f;
	t.entityelement[t.e].hasbeenbatched = 0;
	t.entityelement[t.e].donotreflect = 0;
	t.entityelement[t.e].speedmodulator_f = 1.0f;
	t.entityelement[t.e].limbhurta_f = 0.0f;
	t.entityelement[t.e].limbhurt = 0;
	t.entityelement[t.e].plrvisible = 0;
	t.entityelement[t.e].isclone = 0;
	t.entityelement[t.e].consumed = 0;
	t.entityelement[t.e].distance = 0;
	t.entityelement[t.e].soundplaying = 0;
	t.entityelement[t.e].iCanGoUnderwater = 0;
	t.entityelement[t.e].ishidden = 0;
	t.entityelement[t.e].precreatedspawnedentityelementindex = 0;
	t.entityelement[t.e].explodefusetime = 0;
	t.entityelement[t.e].destroyme = 0;
	t.entityelement[t.e].usingphysicsnow = 0;
	t.entityelement[t.e].doorobsactive = 0;
	t.entityelement[t.e].videotexture = 0;
	t.entityelement[t.e].alttextureused = 0;
	t.entityelement[t.e].soundistalking = 0;
	t.entityelement[t.e].soundlooping = 0;
	t.entityelement[t.e].soundisnonthreedee = 0;
	t.entityelement[t.e].soundset6 = 0;
	t.entityelement[t.e].soundset5 = 0;
	t.entityelement[t.e].soundset4 = 0;
	t.entityelement[t.e].soundset3 = 0;
	t.entityelement[t.e].soundset2 = 0;
	t.entityelement[t.e].soundset1 = 0;
	t.entityelement[t.e].soundset = 0;
	t.entityelement[t.e].activated = 0;
	t.entityelement[t.e].whoactivated = 0;
	t.entityelement[t.e].collected = 0;
	t.entityelement[t.e].beenkilled = 0;
	t.entityelement[t.e].briefimmunity = 0;
	t.entityelement[t.e].health = 0;
	t.entityelement[t.e].animframeupdate = 0;
	t.entityelement[t.e].destanimframe = 0.0f;
	t.entityelement[t.e].animonce = 0;
	t.entityelement[t.e].animframe = 0.0f;
	t.entityelement[t.e].animtime = 0;
	t.entityelement[t.e].animdo = 0;
	t.entityelement[t.e].animdir = 0;
	t.entityelement[t.e].animset = 0;
	t.entityelement[t.e].plrdist = 0.0f;
	t.entityelement[t.e].colb = 0;
	t.entityelement[t.e].colg = 0;
	t.entityelement[t.e].colr = 0;
	t.entityelement[t.e].soundset5 = 0;
	t.entityelement[t.e].soundset6 = 0;
	t.entityelement[t.e].floorposy = -90000.0f;
	t.entityelement[t.e].delay_floorposy = -90000.0f;
	t.entityelement[t.e].dry = 0.0f;
	t.entityelement[t.e].hoverfactoroverride = 0;
	t.entityelement[t.e].nogravity = 0;
	t.entityelement[t.e].scalez = 0.0f;
	t.entityelement[t.e].scaley = 0.0f;
	t.entityelement[t.e].scalex = 0.0f;
	t.entityelement[t.e].rz = 0.0f;
	t.entityelement[t.e].ry = 0.0f;
	t.entityelement[t.e].rx = 0.0f;
	t.entityelement[t.e].quatmode = 0;
	t.entityelement[t.e].quatx = 0.0f;
	t.entityelement[t.e].quaty = 0.0f;
	t.entityelement[t.e].quatz = 0.0f;
	t.entityelement[t.e].quatw = 1.0f;
	t.entityelement[t.e].z = 0.0f;
	t.entityelement[t.e].y = 0.0f;
	t.entityelement[t.e].x = 0.0f;
	t.entityelement[t.e].lastx = 0.0f;
	t.entityelement[t.e].lasty = 0.0f;
	t.entityelement[t.e].lastz = 0.0f;
	t.entityelement[t.e].customlastx = 0.0f;
	t.entityelement[t.e].customlasty = 0.0f;
	t.entityelement[t.e].customlastz = 0.0f;
	t.entityelement[t.e].attachmentobjfirespotlimb = 0;
	t.entityelement[t.e].attachmentbaseobj = 0;
	t.entityelement[t.e].attachmentobj = 0;
	t.entityelement[t.e].obj = 0;
	t.entityelement[t.e].staticflag = 0;
	t.entityelement[t.e].profileobj = 0;
	t.entityelement[t.e].bankindex = 0;
	t.entityelement[t.e].maintype = 0;
	t.entityelement[t.e].active = 0;
	t.entityelement[t.e].editorfixed = 0;
	t.entityelement[t.e].etimer = 0;
	t.entityelement[t.e].ttarget = 0;
	t.entityelement[t.e].spine = 0;
	t.entityelement[t.e].isflak = 0;
	t.entityelement[t.e].animspeedmod = 1.0f;
	t.entityelement[t.e].dc_obj[0] = 0;
	t.entityelement[t.e].dc_obj[1] = 0;
	t.entityelement[t.e].dc_obj[2] = 0;
	t.entityelement[t.e].dc_obj[3] = 0;
	t.entityelement[t.e].dc_obj[4] = 0;
	t.entityelement[t.e].dc_obj[5] = 0;
	t.entityelement[t.e].dc_obj[6] = 0;
	t.entityelement[t.e].dc_entid[0] = 0;
	t.entityelement[t.e].dc_entid[1] = 0;
	t.entityelement[t.e].dc_entid[2] = 0;
	t.entityelement[t.e].dc_entid[3] = 0;
	t.entityelement[t.e].dc_entid[4] = 0;
	t.entityelement[t.e].dc_entid[5] = 0;
	t.entityelement[t.e].dc_entid[6] = 0;
	t.entityelement[t.e].draw_call_obj = 0;
	t.entityelement[t.e].dc_merged = false;
	// wipe out relational data!
	t.entityelement[t.e].eleprof.iObjectLinkID = 0;
	for (int i = 0; i < 10; i++)
	{
		t.entityelement[t.e].eleprof.iObjectRelationships[i] = 0;
		t.entityelement[t.e].eleprof.iObjectRelationshipsData[i] = 0;
		t.entityelement[t.e].eleprof.iObjectRelationshipsType[i] = 0;
	}

	t.entityelement[t.e].eleprof.blendmode = 0;

}
void entity_delete ( void )
{
	//  delete all entities
	for ( t.e = 1 ; t.e<=  g.entityelementlist; t.e++ )
	{
		if (  t.e <= ArrayCount(t.entityelement) ) 
		{
			// find and delete entity element obj
			t.obj=t.entityelement[t.e].obj;
			if (  t.obj>0 ) 
			{
				if (  ObjectExist(t.obj) == 1 ) 
				{
					DeleteObject (  t.obj );
				}
			}

			t.entid = t.entityelement[t.e].bankindex;

			t.entityelement[t.e].bankindex=0;
			t.entityelement[t.e].obj=0;

			//PE: as we are going to reuse the array in next level , reset everything.
			entity_reset_defaults(); //PE: takes t.e
		}
	}

	//PE: Alle objects has been removed reset counters.
	g.entityviewendobj = 0;
	//PE: Create new entities from beginning
	g.entityviewcurrentobj = g.entityviewstartobj;

}

void entity_pauseanimations ( void )
{
	Dim (  t.storeanimspeeds,g.entityelementlist  );
	for ( t.e = 1 ; t.e<=  g.entityelementlist; t.e++ )
	{
		t.obj=t.entityelement[t.e].obj;
		if (  t.obj>0 ) 
		{
			if (  ObjectExist(t.obj) == 1 ) 
			{
				t.storeanimspeeds[t.e]=GetSpeed(t.obj);
				SetObjectSpeed (  t.obj,0 );
			}
		}
	}
}

void entity_resumeanimations ( void )
{
	if (t.storeanimspeeds.size() <= 0) return; //PE: Crash if pause not called before this.

	for ( t.e = 1 ; t.e<=  g.entityelementlist; t.e++ )
	{
		t.obj=t.entityelement[t.e].obj;
		if (  t.obj>0 ) 
		{
			if (  ObjectExist(t.obj) == 1 ) 
			{
				//PE: Resume without pause. got a crash here.
				int speed = 100;
				if (t.e < t.storeanimspeeds.size())
				{
					speed = t.storeanimspeeds[t.e];
				}
				else
				{
					speed = GetSpeed(t.obj);
				}
				SetObjectSpeed (  t.obj, speed );
			}
		}
	}
	UnDim (  t.storeanimspeeds );
}

