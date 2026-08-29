//----------------------------------------------------
//--- GAMEGURU - G-Gun
//----------------------------------------------------

#include "stdafx.h"
#include "gameguru.h"

#include "GGVR.h"

#ifdef OPTICK_ENABLE
#include "optick.h"
#endif

#include "tracers/TracerManager.h"

// GGMAX 3.35i: diagnostic files go beside the EXE, not into whatever the CWD currently is
// (a file dialog can move it). Defined in Guru-WickedMAX/master_part1.cpp.
extern FILE* GGDiagFopen(const char* name, const char* mode);
using namespace Tracers;

// global store for weapon shader effect indexes
cstr g_guns_customArms_s = "";
int g_weaponbasicshadereffectindex = 0;
int g_weaponboneshadereffectindex = 0;

// is used to select a target for a player counter attack
int g_iCounterAttackTargetForPlayer = 0;

void gun_flashbrass_position(float* pfWorldPosX, float* pfWorldPosY, float* pfWorldPosZ, float fModX, float fModY, float fModZ);
template<typename T> static inline T PELerp(T a, T b, float t) { return (T)(a + (b - a) * t); }

// 
//  GUN CORE
// 

void gun_restart ( void )
{
	//  No gun to start with
	g.weaponammoindex=0;
	g.autoloadgun=-1 ; t.gunid=0;
	t.triggerweapononeifexists=1;
	for ( t.t = 1 ; t.t<=  11; t.t++ )
	{
		t.weaponslot[t.t].pref=0;
		t.weaponslot[t.t].got=0;
	}
	for ( t.t = 1 ; t.t<=  20; t.t++ )
	{
		t.weaponammo[t.t]=0;
		t.weaponclipammo[t.t]=0;
	}
	for ( t.i = 1 ; t.i<=  100; t.i++ )
	{
		t.ammopool[t.i].ammo=0;
	}
	for (t.tgunid = 1; t.tgunid <= g.gunmax; t.tgunid++)
	{
		t.gun[t.tgunid].storeammo = 0;
		t.gun[t.tgunid].storeclipammo = 0;
	}

	//  set maximum slots allowed (for games that allow only a few weapons to be carried)
	//  LEE, find out if these are set elsewhere and remove (and move this code to coirrect place)
	g.maxslots=10;
	g.autoswap=1;

	// and load the layout
	extern void gun_gatherslotorder_load ( void );
	gun_gatherslotorder_load();
}

void gun_resetactivateguns ( void )
{
	// reset gun activations before levels start
	for ( t.tgunid = 1 ; t.tgunid<=  g.gunmax; t.tgunid++ )
	{
		t.gun[t.tgunid].activeingame=0;
	}

	// some temp variables in the main gun structure
	for ( t.tgunid = 1 ; t.tgunid<=  g.gunmax; t.tgunid++ )
	{
		t.gun[t.tgunid].settings.canaddtospare=0;
		t.gun[t.tgunid].settings.ismelee=0;
	}
}

void gun_activategunsfromentities ( void )
{
	// Custom arms system reset
	g_guns_customArms_s = "";

	// Only flag those guns present in level
	for ( t.e = 1 ; t.e <= g.entityelementlist; t.e++ )
	{
		t.entid=t.entityelement[t.e].bankindex;
		if ( t.entid>0 ) 
		{
			if ( t.entityprofile[t.entid].ismarker == 1 ) 
			{
				// Entity is Start Marker
				t.tgunid=t.entityelement[t.e].eleprof.hasweapon;
				if ( t.tgunid>0 ) 
				{
					t.gun[t.tgunid].activeingame=1;
				}

				// Place any custom arms before gun load
				g_guns_customArms_s = t.entityelement[t.e].eleprof.texaltd_s; // texaltd_s used to store custom arms choice from start marker
			}
			else
			{
				// Entity Is A Gun
				t.tgunid=t.entityprofile[t.entid].isweapon;
				if ( t.tgunid>0 ) t.gun[t.tgunid].activeingame = 1;

				// Entity is carrying a gun
				t.tgunid = t.entityelement[t.e].eleprof.hasweapon;
				if ( t.tgunid>0 ) 
				{
					t.gun[t.tgunid].activeingame=1;
				}
			}
		}
	}
}

void gun_decaldetails ( void )
{
	for ( t.i = 1 ; t.i<=  g.gunmax; t.i++ )
	{
		for ( t.y = 0 ; t.y<=  1; t.y++ )
		{
			if (  g.firemodes[t.i][t.y].particle.decal_s != "" ) 
			{
				t.decal_s=g.firemodes[t.i][t.y].particle.decal_s;
				decal_find ( );
				if (  t.decalid != -1 ) 
				{
					g.firemodes[t.i][t.y].particle.id=t.decalid;
				}
				else
				{
					g.firemodes[t.i][t.y].particle.decal_s="";
				}
			}
		}
	}
}

void gun_loadonlypresent ( void )
{
	//  Load all guns that have been activated
	for ( t.gunid = 1 ; t.gunid <= g.gunmax; t.gunid++ )
	{
		if (  t.gun[t.gunid].activeingame == 1 ) 
		{
			t.gun_s=t.gun[t.gunid].name_s; 
			gun_load ( );
		}
	}

	//  And now fill in player weapon details
	for ( t.tww = 1 ; t.tww<=  11; t.tww++ )
	{
		t.gunid=t.weaponslot[t.tww].pref;
		if (  t.gunid>0 ) 
		{
			if (  t.gunid <= ArrayCount(t.gun) ) 
			{
				if (  t.gun[t.gunid].activeingame == 1 ) 
				{
					t.weaponhud[t.tww]=t.gun[t.gunid].hudimage;
				}
			}
		}
	}

	//  Ensure gun vars are reset
	t.gunid=0;
}

void gun_resetgunsettings ( void )
{
	int ws = 0;
	//  Reset weapons (1=restart)
	if (  t.tcopyorrestart == 0 ) 
	{
		//  copy
		Dim (  t.copyweaponslot,10 );
		//  AirMod - Next 2 Lines modified for Alt Fire
		Dim (  t.copyweaponammo,20 );
		Dim (  t.copyweaponclipammo,20 );
		Dim (  t.copyweaponhud,10 );
		for ( ws = 1 ; ws<= 20; ws++ )
		{
			//  AirMod - Line (  Modified for Alt Fire )
			if (  ws < 11  )  t.copyweaponslot[ws] = t.weaponslot[ws];
			t.copyweaponammo[ws]=t.weaponammo[ws];
			t.copyweaponclipammo[ws]=t.weaponclipammo[ws];
			//  AirMod - Line (  Modified for Alt Fire )
			if (  ws < 11  )  t.copyweaponhud[ws] = t.weaponhud[ws];
		}
	}
	else
	{
		//  restore
		//  AirMod - Line (  Modified for alt Fire )
		for ( ws = 1 ; ws<=  20; ws++ )
		{
			//  AirMod - Line (  Modified for Alt Fire )
			if (  ws < 11  )  t.weaponslot[ws] = t.copyweaponslot[ws];
			t.weaponammo[ws]=t.copyweaponammo[ws];
			t.weaponclipammo[ws]=t.copyweaponclipammo[ws];
			//  AirMod - Line (  Modified for Alt Fire )
			if (  ws < 11  )  t.weaponhud[ws] = t.copyweaponhud[ws];
		}
	}
}

void gun_manager ( void )
{
#ifdef OPTICK_ENABLE
	OPTICK_EVENT();
#endif
	// exit early if gun system disabled
	if ( ObjectExist(g.hudbankoffset+5) == 0  )
	{
		// GGMAX 2.42: the tracer has to see this too. Two FIRE_WEAPON attempts produced no shot,
		// and "the gun update returned before reading the trigger at all" is a completely
		// different fault from "it read it and a gate zeroed it" — an instrument that only
		// samples below this line cannot tell them apart.
		extern int g_ggFireTraceFrames; extern int g_ggFireTraceEarlyOut;
		if (g_ggFireTraceFrames > 0) { g_ggFireTraceEarlyOut++; g_ggFireTraceFrames--; }
		return;
	}

	// new freeze mode (PLRDISABLE) which stops player attacking
	t.gunclick=t.player[1].state.firingmode;
	if ( t.gunclick == 0 && t.gunmode >= 7 ) t.gunandmelee.pressedtrigger = 0;
	if ( g.mefrozen>0 && g.mefrozentype == 2  )  t.gunclick = 0;
	if ( t.gunclick == 1 && g.firemodes[t.gunid][g.firemode].settings.disablerunandshoot == 1 && t.playercontrol.isrunning == 1 && t.player[1].state.moving == 1  )  t.gunclick = 0;
	if ( t.gunclick == 1 && (g.lowfpswarning == 1 || g.lowfpswarning == 2)  )  t.gunclick = 0;

	// stop gun firing if in any HUD screen
	if (t.game.activeStoryboardScreen > -1)
	{
		extern bool g_bEnableGunFireInHUD;
		if (g_bEnableGunFireInHUD == true)
		{
			// some user in-game huds CAN fire while displayed
		}
		else
		{
			// generally do not shoot when using a HUD screen
			t.gunclick = 0;
		}
	}

	// in VR mode, if trigger being used to open/close/activate, disable any shooting
	extern int g_iActivelyUsingVRNow;
	if (g_iActivelyUsingVRNow == 1)
	{
		static int iHUDDampeningPhase = 0;
		if (t.game.activeStoryboardScreen >= 0) iHUDDampeningPhase = 20;
		if (t.gunclick == 1)
		{
			if (t.luaglobal.scriptprompt3dtime > 0) t.gunclick = 0;
			if (iHUDDampeningPhase > 0) t.gunclick = 0;
		}
		else
		{
			// when not pressing trigger, wait some cycles then can shoot again (prevents shooting when leaving a HUD)
			if (iHUDDampeningPhase > 0) iHUDDampeningPhase--;
		}
	}

	// ============================================================================================
	// GGMAX 2.42: FIRE trigger tracer. Sampled HERE — after every gunclick-zeroing condition above
	// and before the melee/edge-latch logic below — so one row shows both what the trigger came in
	// as (firingmode) and what survived the gates (gunclick), plus each gate's own inputs. Armed
	// by FIRE_WEAPON, read by DUMP_FIRE.
	// Built because two attempts at pulling the trigger produced no shot and reading the code gave
	// a plausible story for each: an edge latch, a zeroing gate, the wrong field entirely. A row
	// per frame decides between them instead of a third guess.
	// ============================================================================================
	{
		extern int g_ggFireTraceFrames;
		extern void GGFireTraceSample(int firingmode, int gunclick, int gunmode, int pressedtrigger,
			int mustrelease, int ammo, int gunid, int mefrozen, int lowfps, int hudscreen,
			int isrunning, int moving, int disablerunshoot);
		if (g_ggFireTraceFrames > 0)
		{
			g_ggFireTraceFrames--;
			GGFireTraceSample(
				t.player[1].state.firingmode, t.gunclick, t.gunmode,
				t.gunandmelee.pressedtrigger, t.gunmustreleasefirst,
				(g.weaponammoindex + g.ammooffset >= 0) ? t.weaponammo[g.weaponammoindex + g.ammooffset] : -1,
				t.gunid, g.mefrozen, g.lowfpswarning, t.game.activeStoryboardScreen,
				t.playercontrol.isrunning, t.player[1].state.moving,
				(t.gunid > 0) ? g.firemodes[t.gunid][g.firemode].settings.disablerunandshoot : -1);
		}
	}

	// Melee control (for TPP)
	bool bGunshotOverridden = false;
	if ( t.playercontrol.thirdperson.enabled == 1 ) 
	{
		// sometimes gunid can be zero? (start marker/fantasy ranger=zero)
		t.gunid = t.entityelement[t.playercontrol.thirdperson.charactere].eleprof.hasweapon;
		bool bWeaponIsMeleeBased = false;
		if (t.gunid > 0)
			if (t.gun[t.gunid].statuspanelcode == 7 || t.gun[t.gunid].statuspanelcode == 8 || t.gun[t.gunid].statuspanelcode == 10) // bow, axe, sword
				bWeaponIsMeleeBased = true;
		if (t.gunid == 0 || bWeaponIsMeleeBased == true)
		{
			// use gunclick to detect one off press for melee
			if (t.gunclick != 1)	t.gunmustreleasefirst = 0;
			if (t.gunclick != 0)
			{
				if (t.gunclick == 1 && t.gunmustreleasefirst == 0)
				{
					// if third person, detect melee override of LMB shooting
					int iE = t.playercontrol.thirdperson.charactere;
					int iWeaponIndex = t.entityelement[iE].eleprof.hasweapon;
					if (iWeaponIndex == 0)
					{
						// but only trigger 'punch' melee if specified in FPE
						int entid = t.entityelement[iE].bankindex;
						t.q = t.entityprofile[entid].startofaianim;
						if (t.entityanim[entid][t.q + t.csi_stoodpunch[1]].start > 0)
						{
							t.charanimcontrols[t.playercontrol.thirdperson.characterindex].meleeing = 1;
						}
						bGunshotOverridden = true;
					}
					else
					{
						if (bWeaponIsMeleeBased == true)
						{
							int entid = t.entityelement[iE].bankindex;
							t.q = t.entityprofile[entid].startofaianim;
							if (t.entityanim[entid][t.q + t.csi_stoodpunch[1]].start > 0)
							{
								t.charanimcontrols[t.playercontrol.thirdperson.characterindex].meleeing = 1;
							}
							bGunshotOverridden = true;
						}
					}
				}
			}
		}
	}

	// Gun controls
	if ( t.gunmode < 100 && bGunshotOverridden == false ) 
	{
		//  Gun Firing
		if (  t.gunclick != 1  )
			t.gunmustreleasefirst = 0;
		if (  t.gunclick != 0 ) 
		{
			bool bGunshotOverridden = false;
			if ( t.gunclick == 1 && t.gunmustreleasefirst == 0 && t.gunandmelee.pressedtrigger == 0 )
			{
				// if gun ready, trigger a shot (only if t.gunandmelee.pressedtrigger is zero so dry fire anim does not freeze)
				if ( t.gunmode >= 5 && t.gunmode <= 26 ) 
				{
					t.gunmodelast = t.gunmode;
					t.gunmode = 101;
					if ( t.game.runasmultiplayer == 1 ) mp_shoot ( );
				}
			}
			t.gunfull=1;
			if ( t.weaponammo[g.weaponammoindex+g.ammooffset] < g.firemodes[t.gunid][g.firemode].settings.reloadqty+1 && g.firemodes[t.gunid][g.firemode].settings.chamberedround > 0 )  t.gunfull  =  0;
			if ( t.weaponammo[g.weaponammoindex+g.ammooffset] < g.firemodes[t.gunid][g.firemode].settings.reloadqty && g.firemodes[t.gunid][g.firemode].settings.chamberedround == 0 )  t.gunfull  =  0;
			if ( t.weaponammo[g.weaponammoindex+g.ammooffset]>0 && g.firemodes[t.gunid][g.firemode].settings.emptyreloadonly  ==  1 )  t.gunfull = 1;
			if ( t.gunfull == 1 ) g.plrreloading = 0;
			if ( t.gunmode < 30 ) 
			{
				if ( t.gunclick == 2 && (t.gunfull == 0 || g.firemodes[t.gunid][g.firemode].settings.nofullreload == 0)  )  
				{
					// 110718 - ensure cannot reload while running or transitioning (will reload as soon as transition ends)
					if ( t.playercontrol.usingrun == -1 && (t.gunmode<27 || t.gunmode>28) )
					{
						t.gunmode = 121;
					}
				}
			}
		}
		else
		{
			//  Gun Movement
			if (  t.gunmode<21 || t.gunmode>39 ) 
			{
				//  AirMod - Enable MOVE animation in crouch
				if (  t.player[1].state.moving != 0  )  t.gunmode = 21;
			}
		}
	}

	// trigger melee block
	if (t.player[1].state.blockingaction > 0 && t.gunmode < 100)
	{
		if (g.firemodes[t.gunid][g.firemode].blockaction.start.s > 0)
		{
			if (t.gunzoommode >= 8) t.gunzoommode = 11;
			if (t.player[1].state.blockingaction == 1)
			{
				t.player[1].state.blockingaction = 2;
			}
			else
			{
				if (t.player[1].state.blockingaction == 2)
				{
					t.gunmode = 1001;
				}
				else
				{
					t.player[1].state.blockingaction = 0;
				}
			}
		}
		else
		{
			t.player[1].state.blockingaction = 0;
		}
	}

	// also reset counteredaction flag when blocking is reset (self resetting)
	if (t.player[1].state.blockingaction==0 ) t.player[1].state.counteredaction = 0;

	// trigger melee attack
	if ( t.gun[t.gunid].settings.ismelee>0 && t.gunmode<100 ) 
	{
		if (  g.firemodes[t.gunid][g.firemode].meleeaction.start.s>0 ) 
		{
			if ( t.gunzoommode >= 8 ) t.gunzoommode = 11; // catches all states of a zoomed in state
			if ( t.gun[t.gunid].settings.ismelee == 2 ) t.gunmode = 1020;
			if ( g.firemodes[t.gunid][g.firemode].settings.simplezoom != 0 && g.firemodes[t.gunid][g.firemode].settings.simplezoomanim != 0 && t.gunzoommode != 0 ) 
			{
				t.gunmode=2003;
			}
			else
			{
				t.gun[t.gunid].settings.ismelee=2;
			}
		}
		else
		{
			t.gun[t.gunid].settings.ismelee=0;
		}
	}

	//  update HUD object with flash img
	if (  g.firemodes[t.gunid][g.firemode].settings.flashimg != t.gunandmelee.hudbankcrosshairtexture ) 
	{
		TextureObject (  g.hudbankoffset+5,0,g.firemodes[t.gunid][g.firemode].settings.flashimg );
		t.gunandmelee.hudbankcrosshairtexture=g.firemodes[t.gunid][g.firemode].settings.flashimg;
	}

	// gun selection
	if ( t.gunmode<31 || t.gunmode>35 ) 
	{
		if ( t.player[1].command.newweapon>0 ) 
		{
			t.sel=t.player[1].command.newweapon;
			t.player[1].command.newweapon = 0;
			if (t.sel != t.gunid)
			{
				if (g.weaponammoindex > 0)
				{
					//  only if 'different weapon'
					///if (t.weaponslot[g.weaponammoindex].pref != t.sel)
					if (t.weaponslot[g.weaponammoindex].got != t.sel)
					{
						t.gunmode = 31; t.gunselectionafterhide = t.sel;
						t.gunandmelee.tmouseheld = 0;
					}
					else
					{
						//  Alternate Fire
						if (g.ggunaltswapkey1 == -1)
						{
							if ((t.gun[t.gunid].settings.alternateisflak == 1 || t.gun[t.gunid].settings.alternateisray == 1) && t.gunmode <= 100)
							{
								if (t.gun[t.gunid].settings.alternate == 1) { t.gunmode = 2009; t.gun[t.gunid].settings.alternate = 0; }
								else { t.gunmode = 2007; t.gun[t.gunid].settings.alternate = 1; }
							}
						}
					}
				}
				else
				{
					t.gunmode = 131; g.autoloadgun = t.sel;
					if (g.autoloadgun != t.gunid)  t.gunandmelee.tmouseheld = 0;
				}
			}
		}
	}

	//  Change weapon
	gun_change ( );

	//  Need to update hud object for gun here (and again after Sync ( ) )
	gun_update_hud ( );

	// Gun control
	if ( t.gunid > 0 ) 
	{
		if ( ObjectExist(t.currentgunobj) == 1 ) 
		{
			// handle gun and soundcontrol
			if ( g.firemode != t.gun[t.gunid].settings.alternate )
			{
				t.tfireanim = 0;
				t.tmeleeanim = 0;
			}
			g.firemode=t.gun[t.gunid].settings.alternate;

			g.ammooffset=g.firemode*10;
			if (  t.gun[t.gunid].settings.modessharemags == 1  )  g.ammooffset = 0;
			if ( t.player[t.plrid].health>0 ) gun_control ( );
			gun_shoot ( );
			if (  t.playercontrol.thirdperson.enabled == 0 ) 
			{
				gun_flash ();
				gun_brass ();
				gun_smoke ();
			}
			gun_soundcontrol ( );

			//  handle replacing of projectile for FLAK weapons and hiding it when no ammo left
			t.flakid=g.firemodes[t.gunid][g.firemode].settings.flakindex;
			if (  t.flakid>0 ) 
			{
				if (  g.firemodes[t.gunid][g.firemode].settings.flaklimb != -1 ) 
				{
					t.tshowammobeingloaded=0;
					if (  t.gun[t.gunid].projectileframe == 0 ) 
					{
						if (  GetFrame(t.currentgunobj) >= g.firemodes[t.gunid][g.firemode].action.startreload.s && GetFrame(t.currentgunobj) <= g.firemodes[t.gunid][g.firemode].action.startreload.e  )  t.tshowammobeingloaded = 1;
						if (  GetFrame(t.currentgunobj) >= g.firemodes[t.gunid][g.firemode].action.reloadloop.s && GetFrame(t.currentgunobj) <= g.firemodes[t.gunid][g.firemode].action.reloadloop.e  )  t.tshowammobeingloaded = 1;
						if (  GetFrame(t.currentgunobj) >= g.firemodes[t.gunid][g.firemode].action.endreload.s && GetFrame(t.currentgunobj) <= g.firemodes[t.gunid][g.firemode].action.endreload.e  )  t.tshowammobeingloaded = 1;
					}
					else
					{
						if (  GetFrame(t.currentgunobj) >= g.firemodes[t.gunid][g.firemode].action.start.s && GetFrame(t.currentgunobj) <= t.gun[t.gunid].projectileframe  )  t.tshowammobeingloaded = 1;
					}
					if (  t.weaponammo[g.weaponammoindex] == 0 && t.tshowammobeingloaded == 0 ) 
					{
						HideLimb (  t.currentgunobj,g.firemodes[t.gunid][g.firemode].settings.flaklimb );
					}
					else
					{
						//  hide IMMEDIATELY or DELAYED
						t.thideprojectileinhudmodel=0;
						if (  t.gun[t.gunid].projectileframe == 0 ) 
						{
							//  RPG - always hide rocket as it leaves launcher IMMEDIATELY
							if (  GetFrame(t.currentgunobj) >= g.firemodes[t.gunid][g.firemode].action.start.s && GetFrame(t.currentgunobj) <= g.firemodes[t.gunid][g.firemode].settings.flakrearmframe ) 
							{
								t.thideprojectileinhudmodel=1;
							}
						}
						else
						{
							//  HAND GRENADE - hide grenade after the throw
							if (  GetFrame(t.currentgunobj) >= t.gun[t.gunid].projectileframe && GetFrame(t.currentgunobj) <= g.firemodes[t.gunid][g.firemode].settings.flakrearmframe ) 
							{
								t.thideprojectileinhudmodel=1;
							}
						}
						if (  t.thideprojectileinhudmodel == 1 ) 
						{
							HideLimb (  t.currentgunobj,g.firemodes[t.gunid][g.firemode].settings.flaklimb );
						}
						else
						{
							ShowLimb (  t.currentgunobj,g.firemodes[t.gunid][g.firemode].settings.flaklimb );
						}
					}
				}
			}
		}
	}

	// TPP Melee Control
	if ( t.playercontrol.thirdperson.enabled == 1 ) 
	{
		if ( t.charanimcontrols[t.playercontrol.thirdperson.characterindex].meleeing != 0 )
		{
			// player meleeing, detect strike frames and inflict damage on any entity in front of character
			int iE = t.playercontrol.thirdperson.charactere;
			int iObj = t.entityelement[iE].obj;
			if ( iObj > 0 )
			{
				if ( ObjectExist ( iObj ) == 1 )
				{
					float fCurrentFrame = GetFrame(iObj);
					float fCurrentAngle = WrapValue ( ObjectAngleY ( iObj ) );
					int iEntID = t.entityelement[iE].bankindex;
					t.q = t.entityprofile[iEntID].startofaianim;
					float fStrikeRadius = t.entityprofile[iEntID].meleerange;
					float fStrikeStart = t.entityanim[iEntID][t.q+t.csi_stoodpunch[1]].start;
					float fStrikeFinish = t.entityanim[iEntID][t.q+t.csi_stoodpunch[1]].finish;
					float fStrikeThird = (fStrikeFinish-fStrikeStart)/3.0f;
					fStrikeStart += fStrikeThird;
					fStrikeFinish -= fStrikeThird;
					if ( t.entityprofile[iEntID].meleestrikest > 0 )
					{
						fStrikeStart = t.entityprofile[iEntID].meleestrikest;
						fStrikeFinish = t.entityprofile[iEntID].meleestrikefn;
					}
					if ( fCurrentFrame < fStrikeStart ) t.playercontrol.thirdperson.meleestruck = 0;
					if ( fCurrentFrame >= fStrikeStart && fCurrentFrame <= fStrikeFinish && t.playercontrol.thirdperson.meleestruck == 0 )
					{
						// is character facing an entity and in range
						float fCurrentX = ObjectPositionX ( iObj );
						float fCurrentY = ObjectPositionY ( iObj );
						float fCurrentZ = ObjectPositionZ ( iObj );
						for ( int iEE = 1; iEE <= g.entityelementlist; iEE++ )
						{
							if ( iEE != iE )
							{
								float fDX = t.entityelement[iEE].x - fCurrentX;
								float fDZ = t.entityelement[iEE].z - fCurrentZ;
								float fDD = sqrt ( fabs(fDX*fDX)+fabs(fDZ*fDZ) );
								int entid = t.entityelement[iEE].bankindex;
								if ( fDD < fStrikeRadius && t.entityelement[iEE].staticflag == 0 && t.entityelement[iEE].health > 0 && t.entityprofile[entid].ismarker == 0 )
								{
									if ( fabs ( t.entityelement[iEE].y - fCurrentY ) < 50.0f )
									{
										float fDA = WrapValue ( Atan2 ( fDX, fDZ ) );
										float fCompareA = WrapValue(fabs(fCurrentAngle) - fabs(fDA));
										if ( fCompareA > 180 ) fCompareA = fCompareA - 360;
										if ( fabs(fCompareA) < t.entityprofile[iEntID].meleehitangle )
										{
											// apply some damage
											t.tttriggerdecalimpact = 0;
											t.tdamagesource = 1;
											int iDamageRange = t.entityprofile[iEntID].meleedamagefn - t.entityprofile[iEntID].meleedamagest;
											t.tdamage = t.entityprofile[iEntID].meleedamagest + (rand()%iDamageRange);
											entity_hitentity ( iEE, t.entityelement[iEE].obj );
											entity_triggerdecalatimpact ( t.entityelement[iEE].x, t.entityelement[iEE].y+50.0f, t.entityelement[iEE].z );
											t.playercontrol.thirdperson.meleestruck = 1;
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	// independent handling (no matter which guns is selected)
	gun_brass_indi ( );
}

void gun_SetObjectInterpolation(int iObjID, float fValue)
{
	SetObjectInterpolation (iObjID, fValue);
	int iGunSecondaryObj = g.gunbankextraobjoffset + (iObjID - g.gunbankoffset);
	if (ObjectExist(iGunSecondaryObj) == 1)
	{
		SetObjectInterpolation (iGunSecondaryObj, fValue);
	}
}

void gun_SetObjectFrame(int iObjID, float fValue)
{
	SetObjectFrame (iObjID, fValue);
	int iGunSecondaryObj = g.gunbankextraobjoffset + (iObjID - g.gunbankoffset);
	if (ObjectExist(iGunSecondaryObj) == 1)
	{
		SetObjectFrame (iGunSecondaryObj, fValue);
	}
}

void gun_SetObjectSpeed(int iObjID, float fValue)
{
	SetObjectSpeed (iObjID, fValue);
	int iGunSecondaryObj = g.gunbankextraobjoffset + (iObjID - g.gunbankoffset);
	if (ObjectExist(iGunSecondaryObj) == 1)
	{
		SetObjectSpeed (iGunSecondaryObj, fValue);
	}
}

void gun_PlayObject(int iObjID, float fStart, float fEnd)
{
	PlayObject (iObjID, fStart, fEnd);
	int iGunSecondaryObj = g.gunbankextraobjoffset + (iObjID - g.gunbankoffset);
	if (ObjectExist(iGunSecondaryObj) == 1)
	{
		PlayObject (iGunSecondaryObj, fStart, fEnd);
	}
}

void gun_LoopObject(int iObjID, float fStart, float fEnd)
{
	LoopObject (iObjID, fStart, fEnd);
	int iGunSecondaryObj = g.gunbankextraobjoffset + (iObjID - g.gunbankoffset);
	if (ObjectExist(iGunSecondaryObj) == 1)
	{
		LoopObject (iGunSecondaryObj, fStart, fEnd);
	}
}

void gun_LoopObject(int iObjID)
{
	LoopObject (iObjID);
	int iGunSecondaryObj = g.gunbankextraobjoffset + (iObjID - g.gunbankoffset);
	if (ObjectExist(iGunSecondaryObj) == 1)
	{
		LoopObject (iGunSecondaryObj);
	}
}

void gun_StopObject(int iObjID)
{
	StopObject (iObjID);
	int iGunSecondaryObj = g.gunbankextraobjoffset + (iObjID - g.gunbankoffset);
	if (ObjectExist(iGunSecondaryObj) == 1)
	{
		StopObject (iGunSecondaryObj);
	}
}

void gun_change ( void )
{
	//  at start of level, this is set to one
	if (  t.triggerweapononeifexists>0 ) 
	{
		t.triggerweapononeifexists=0;
		t.tgunid=t.weaponslot[1].got;
		if (  t.tgunid>0 ) 
		{
			if (  Len(t.gun[t.tgunid].name_s.Get()) > 1 ) 
			{
				g.autoloadgun=t.tgunid;
			}
		}
	}
	else
	{
		if (  t.playercontrol.thirdperson.enabled == 1 ) 
		{
			// check if third person has weapon but not yet assigned
			if ( t.entityelement[t.playercontrol.thirdperson.charactere].eleprof.hasweapon != 0 && t.gunid == 0 )
			{
				// allow g.autoloadgun through..
			}
			else
			{
				// third person character cannot equip new weapons for the moment
				return;
			}
		}
	}

	if ( g.autoloadgun != -1 && g.autoloadgun <= ArrayCount(t.gun) ) 
	{
		//  if in jet pack mode, switch it off
		if (  t.playercontrol.jetpackhidden == 0 ) 
		{
			if (  g.autoloadgun>0 ) 
			{
				if (  t.playercontrol.jetpackmode>0 ) 
				{
					t.playercontrol.jetpackmode=3;
				}
			}
		}

		//  Free the old gun
		gun_free ( );

		//  Gun selection
		t.gunid=g.autoloadgun;
		t.gun_s=t.gun[t.gunid].name_s;
		g.autoloadgun=-1;

		// reset any random fire choice (so must choose from availale ones for new gun)
		t.tfireanim = 0;

		//  If gun selection valid, load it
		if (  t.gun_s != "" ) 
		{
			g.firemode=0;
			gun_selectandorload ( );
		}

		//  cause gun lighting to reset
		t.currentguncolr=-1;

		//  Show gun as active
		t.currentgunobj=t.gun[t.gunid].obj;
		if (  t.currentgunobj>0 ) 
		{
			gun_SetObjectInterpolation (  t.currentgunobj,100 );
			gun_SetObjectFrame (  t.currentgunobj,g.firemodes[t.gunid][0].action.show.s );
			ShowObject (  t.currentgunobj );
		}
		else
		{
			t.gunid=0;
		}

		//  Default gun action is to SHOW and reveal gun (then goes to gunmode=5 idle)
		t.gunmode=131 ; t.keyboardpress=0;

		//  locate slot for ammo usage
		g.weaponammoindex=0;
		if (  t.gunid>0 ) 
		{
			for ( t.ws = 1 ; t.ws < 12; t.ws++ )
			{
				if (  t.weaponslot[t.ws].got == t.gunid ) 
				{
					g.weaponammoindex=t.ws ; break;
				}
			}
		}

		//  show all ammo to begin with for new weapon
		if (  t.gunid>0 ) 
		{
			if (  t.gun[t.gunid].obj>0 ) 
			{
				if (  ObjectExist(t.gun[t.gunid].obj) == 1 ) 
				{
					for ( t.p = t.gun[t.gunid].settings.bulletlimbstart ; t.p<=  t.gun[t.gunid].settings.bulletlimbend; t.p++ )
					{
						if (  t.p <= ArrayCount(t.bulletlimbs) ) 
						{
							t.limbnumber=t.bulletlimbs[t.p];
							ScaleLimb (  t.gun[t.gunid].obj,t.limbnumber,100,100,100 );
						}
					}
				}
			}
		}

		// if in VR mode, hide any arms (that can be detected)
		// and eventually allow arms/hands to be specified in special new VR fields in gunspec
		if (t.gunid > 0)
		{
			bool bNormalOrVRMode = false;
			if (g.vrglobals.GGVREnabled > 0 && g.vrglobals.GGVRUsingVRSystem == 1) bNormalOrVRMode = true;
			if (bNormalOrVRMode == true)
			{
				sObject* pGunObj = GetObjectData(t.currentgunobj);
				for (int i = 0; i < pGunObj->iFrameCount; i++)
				{
					sFrame* pFrame = pGunObj->ppFrameList[i];
					if (pFrame->pMesh)
					{
						if (strstr(pFrame->szName, "arms") != NULL)
						{
							// found arms, hide this limb
							HideLimb(t.currentgunobj, pFrame->iID);
						}
					}
				}
			}
		}
	}
}

bool g_bNeedToRestoreLimbsAfterVR = false;

void gun_update_hud ( void )
{
	// need to know gun object globally to exclude from ray cast detection
	extern int g_iCurrentGunObj;
	g_iCurrentGunObj = t.currentgunobj;

	//  HUD marker update
	if (  ObjectExist(g.hudbankoffset+2) == 1 ) 
	{
		t.tsimwoddle_f=0;
		if (  t.currentgunobj>0 ) 
		{
			if (  ObjectExist(t.currentgunobj) == 1 ) 
			{
				if (  t.playercontrol.movement != 0 ) 
				{
					if (  GetNumberOfFrames(t.currentgunobj) == 0 ) 
					{
						t.gfakewoddle_f=WrapValue(t.gfakewoddle_f+4);
						t.tsimwoddle_f=CurveValue(Cos(t.gfakewoddle_f)*2,t.tsimwoddle_f,15);
					}
				}
				else
				{
					t.tsimwoddle_f=CurveValue(0,t.tsimwoddle_f,15);
				}
			}
		}
		if (  g.globals.riftmode == 0 ) 
		{
			bool bVRMode = false;
			if (g.vrglobals.GGVREnabled > 0 && g.vrglobals.GGVRUsingVRSystem == 1) bVRMode = true;
			if (bVRMode == false)
			{
				// non-VR mode
				t.gunax_f = CameraAngleX(); t.gunay_f = CameraAngleY();

				if (g.luacameraoverride == 3)
				{
					//PE: In this mode , make sure arms/weapon display correct and tilt with camera.
					RotateObject(g.hudbankoffset + 2, CameraAngleX(), CameraAngleY(), CameraAngleZ());
				}
				else
				{
					RotateObject(g.hudbankoffset + 2, t.gunax_f, t.gunay_f, 0);
				}

				PositionObject(g.hudbankoffset + 2, CameraPositionX(), CameraPositionY() + t.tsimwoddle_f, CameraPositionZ());
				if (g_bNeedToRestoreLimbsAfterVR == true)
				{
					if (t.currentgunobj > 0 && ObjectExist(t.currentgunobj) == 1)
					{
						PerformCheckListForLimbs(t.currentgunobj);
						int iGunLimbCount = ChecklistQuantity();
						for (int c = 1; c <= iGunLimbCount; c++)
						{
							int iLimbID = c - 1;
							ShowLimb(t.currentgunobj, iLimbID);
						}
					}
					g_bNeedToRestoreLimbsAfterVR = false;
				}
			}
			else
			{
				// VR Mode
				int iControllerObj = GGVR_GetRightHandObject();
				if ( iControllerObj > 0 )
				{
					if ( ObjectExist(iControllerObj) == 1 )
					{
						if ( t.currentgunobj > 0 && ObjectExist(t.currentgunobj) == 1 && GetVisible(t.currentgunobj) == 1)
						{
							// scale VR weapon
							float fScl = 100.0f;// 75.0f;
							if (t.gun[t.gunid].settings.iVRWeaponMode > 0)
							{
								fScl = t.gun[t.gunid].settings.fVRWeaponScale;
							}
							ScaleObject(g.hudbankoffset + 2, fScl, fScl, fScl);

							// first align weapon HUD obj to controller
							SetObjectToObjectOrientation(g.hudbankoffset + 2, iControllerObj);

							// then adjust by any angle tweaking from VRweapon settings
							static GGVECTOR3 vecVRWeaponAngleSetting;
							vecVRWeaponAngleSetting.x = 0.0f;
							vecVRWeaponAngleSetting.y = 0.0f;
							vecVRWeaponAngleSetting.z = 0.0f;
							if ( t.gun[t.gunid].settings.iVRWeaponMode > 0 )
							{
								vecVRWeaponAngleSetting.x = t.gun[t.gunid].settings.fVRWeaponAngleX;
								vecVRWeaponAngleSetting.y = t.gun[t.gunid].settings.fVRWeaponAngleY;
								vecVRWeaponAngleSetting.z = t.gun[t.gunid].settings.fVRWeaponAngleZ;
							}
							GGVECTOR3 vecVRWeaponAngle = vecVRWeaponAngleSetting;
							if ( vecVRWeaponAngle.z != 0.0f ) RollObjectRight(g.hudbankoffset + 2, vecVRWeaponAngle.z);
							if ( vecVRWeaponAngle.x != 0.0f ) PitchObjectDown(g.hudbankoffset + 2, vecVRWeaponAngle.x);
							if ( vecVRWeaponAngle.y != 0.0f ) TurnObjectRight(g.hudbankoffset + 2, vecVRWeaponAngle.y);

							// we are gong to work out world position for the weapon model using weapon HUD object and controller position
							GGVECTOR3 vecWorldPos;
							vecWorldPos = GGVECTOR3(ObjectPositionX(iControllerObj), ObjectPositionY(iControllerObj), ObjectPositionZ(iControllerObj));

							// quick way to edit VR WEAPON SETTINGS and save gunspec new settings
							PerformCheckListForLimbs(t.currentgunobj);
							int iGunLimbCount = ChecklistQuantity();

							// debug code to check limb IDs and hide necessary ones when in VR
							bool bAllowKeyEditingLive = true;
							if (bAllowKeyEditingLive == true)
							{
								// always need mode on when here for this weapon
								t.gun[t.gunid].settings.iVRWeaponMode = 1;

								int iKeyPressed = 0;
								bool bOneKeyDown = false;
								static int iKeyPressedDown = 0;
								for (int keyi = 2; keyi <= 11; keyi++)
								{
									if (KeyState(keyi) == 1) 
									{ 
										bOneKeyDown = true;
										if (iKeyPressedDown == 0)
										{
											iKeyPressed = keyi;
											iKeyPressedDown = 1;
										}
									}
								}
								if(bOneKeyDown==false)
								{
									iKeyPressedDown = 0;
								}
								if (iKeyPressed == 2) t.gun[t.gunid].settings.iVRWeaponLimbOfWeapon--;
								if (iKeyPressed == 3) t.gun[t.gunid].settings.iVRWeaponLimbOfWeapon++;
								if (iKeyPressed == 4) t.gun[t.gunid].settings.iVRWeaponStaticFrame--;
								if (iKeyPressed == 5) t.gun[t.gunid].settings.iVRWeaponStaticFrame++;
								if (iKeyPressed == 6) t.gun[t.gunid].settings.fVRWeaponOffsetX--;
								if (iKeyPressed == 7) t.gun[t.gunid].settings.fVRWeaponOffsetX++;
								if (iKeyPressed == 8) t.gun[t.gunid].settings.fVRWeaponOffsetY--;
								if (iKeyPressed == 9) t.gun[t.gunid].settings.fVRWeaponOffsetY++;
								if (iKeyPressed == 10) t.gun[t.gunid].settings.fVRWeaponOffsetZ--;
								if (iKeyPressed == 11)
								{
									LPSTR pSaveSettingsLocal = "quickweaponsettings.txt";
									if (FileExist(pSaveSettingsLocal) == 1) DeleteFileA(pSaveSettingsLocal);
									OpenToWrite(3, pSaveSettingsLocal);
									char pLineToWrite[256];
									sprintf(pLineToWrite, ";// VR Support"); WriteString(3, pLineToWrite);
									sprintf(pLineToWrite, "vrweaponmode=%d", t.gun[t.gunid].settings.iVRWeaponMode); WriteString(3, pLineToWrite);
									sprintf(pLineToWrite, "vrweaponlimbofweapon=%d", t.gun[t.gunid].settings.iVRWeaponLimbOfWeapon); WriteString(3, pLineToWrite);
									sprintf(pLineToWrite, "vrweaponstaticframe=%d", t.gun[t.gunid].settings.iVRWeaponStaticFrame); WriteString(3, pLineToWrite);
									sprintf(pLineToWrite, "vrweaponoffsetx=%d", (int)t.gun[t.gunid].settings.fVRWeaponOffsetX); WriteString(3, pLineToWrite);
									sprintf(pLineToWrite, "vrweaponoffsety=%d", (int)t.gun[t.gunid].settings.fVRWeaponOffsetY); WriteString(3, pLineToWrite);
									sprintf(pLineToWrite, "vrweaponoffsetz=%d", (int)t.gun[t.gunid].settings.fVRWeaponOffsetZ); WriteString(3, pLineToWrite);
									CloseFile(3);
								}
							}

							// limits
							if (t.gun[t.gunid].settings.iVRWeaponLimbOfWeapon < 0) t.gun[t.gunid].settings.iVRWeaponLimbOfWeapon = 0;
							if (t.gun[t.gunid].settings.iVRWeaponLimbOfWeapon > iGunLimbCount-1) t.gun[t.gunid].settings.iVRWeaponLimbOfWeapon = iGunLimbCount-1;
							if (t.gun[t.gunid].settings.iVRWeaponStaticFrame < 0) t.gun[t.gunid].settings.iVRWeaponStaticFrame = 0;
							//

							// get offset from base currentgunobj to FIRESPOT (common amongst weapons and indicator of where hand might be)
							GGVECTOR3 vecVRWeaponOffsetSetting;
							vecVRWeaponOffsetSetting.x = 0;
							vecVRWeaponOffsetSetting.y = 0;
							vecVRWeaponOffsetSetting.z = 0;
							if ( t.gun[t.gunid].settings.iVRWeaponMode > 0 )
							{
								vecVRWeaponOffsetSetting.x = t.gun[t.gunid].settings.fVRWeaponOffsetX;
								vecVRWeaponOffsetSetting.y = t.gun[t.gunid].settings.fVRWeaponOffsetY;
								vecVRWeaponOffsetSetting.z = t.gun[t.gunid].settings.fVRWeaponOffsetZ;
							}

							// rotate offset by orientation of weapon HUD 
							sObject* pWeaponHUDObj = GetObjectData(g.hudbankoffset + 2);
							GGMATRIX matHUDRot = pWeaponHUDObj->position.matObjectNoTran;
							GGVECTOR3 vecVRWeaponOffset = vecVRWeaponOffsetSetting;
							GGVec3TransformCoord(&vecVRWeaponOffset, &vecVRWeaponOffset, &matHUDRot);

							// apply rotated offset to world
							vecWorldPos += vecVRWeaponOffset;
							PositionObject(g.hudbankoffset + 2, vecWorldPos.x, vecWorldPos.y, vecWorldPos.z);

							// VR supported or not
							if (t.gun[t.gunid].settings.iVRWeaponMode == 1)
							{
								// only show specified limb (so can hide hands,etc)
								for (int c = 1; c <= iGunLimbCount; c++)
								{
									int iLimbID = c - 1;
									if (iLimbID == t.gun[t.gunid].settings.iVRWeaponLimbOfWeapon)
										ShowLimb(t.currentgunobj, iLimbID);
									else
										HideLimb(t.currentgunobj, iLimbID);
								}
							}
							else
							{
								// no specific VR support, keep object but hide all limbs so cannot see anything bad
								for (int c = 1; c <= iGunLimbCount; c++)
								{
									int iLimbID = c - 1;
									HideLimb(t.currentgunobj, iLimbID);
								}
							}
							g_bNeedToRestoreLimbsAfterVR = true;

							// then look to hide the VR controller
							GGVR_LeftIsBest(true); // only left used for motion
							GGVR_SetRightHandInvisible(true);
							GGVR_SetLeftHandInvisible(false);

							// extend laser starting point
							GGVR_SetLaserForwardDistance(105.0f);
						}
						else
						{
							// normal VR controller mode
							GGVR_LeftIsBest(false); // left and right can control motion
							GGVR_SetRightHandInvisible(false);
							GGVR_SetLeftHandInvisible(false);
							GGVR_SetLaserForwardDistance(100.0f);
						}
					}
				}
			}
		}
	}

	// and update visibility
	gun_update_hud_visibility ( );
}

void gun_update_hud_visibility ( void )
{
	if (  ObjectExist(g.hudbankoffset+2) == 1 ) 
	{
		if (  t.currentgunobj>0 ) 
		{
			if (  ObjectExist(t.currentgunobj) == 1 ) 
			{
				if (  (t.player[1].health>0 || t.playercontrol.startstrength == 0) && t.playercontrol.thirdperson.enabled == 0 ) 
				{
					ShowObject (  t.currentgunobj );
				}
				else
				{
					HideObject (  t.currentgunobj );
				}
			}
		}
	}
}

void gun_update_overlay ( void )
{
}

void gun_picksndvariant ( void )
{
	//  takes gunid,tgunsoundindex, returns sndid
	t.trr=1+Rnd(3) ; t.tttokay=0;
	if (  t.trr == 1 && t.gunsound[t.gunid][t.tgunsoundindex].soundid1>0  )  t.tttokay = 1;
	if (  t.trr == 2 && t.gunsound[t.gunid][t.tgunsoundindex].soundid2>0  )  t.tttokay = 1;
	if (  t.trr == 3 && t.gunsound[t.gunid][t.tgunsoundindex].soundid3>0  )  t.tttokay = 1;
	if (  t.trr == 4 && t.gunsound[t.gunid][t.tgunsoundindex].soundid4>0  )  t.tttokay = 1;
	if (  t.tttokay == 1 ) 
	{
		if (  t.trr == 1  )  t.sndid = t.gunsound[t.gunid][t.tgunsoundindex].soundid1;
		if (  t.trr == 2  )  t.sndid = t.gunsound[t.gunid][t.tgunsoundindex].soundid2;
		if (  t.trr == 3  )  t.sndid = t.gunsound[t.gunid][t.tgunsoundindex].soundid3;
		if (  t.trr == 4  )  t.sndid = t.gunsound[t.gunid][t.tgunsoundindex].soundid4;
	}
	else
	{
		t.sndid=t.gunsound[t.gunid][t.tgunsoundindex].soundid1;
	}
}

bool gun_getstartandfinish ( bool bIgnoreGunMode )
{
	//  get start and finish fire animation
	t.gstart = g.firemodes[t.gunid][g.firemode].action.start;
	t.gfinish = g.firemodes[t.gunid][g.firemode].action.finish;

	//  if last bullet though, use alternative if available
	if ( g.firemodes[t.gunid][g.firemode].action.laststart.s>0 ) 
	{
		if ( t.gunmode <= 104 || t.gunmode>106 ) //PE: Last can now happen in 104
		{
			// ensure start/finish not change WHILST performing last fire animation
			if ( t.weaponammo[g.weaponammoindex+g.ammooffset] <= 1 )
			{
				t.gstart = g.firemodes[t.gunid][g.firemode].action.laststart;
				t.gfinish = g.firemodes[t.gunid][g.firemode].action.lastfinish;
				return false;
			}
		}
	}

	// normal
	return true;
}

bool gun_getzoomstartandfinish ( void )
{
	t.gstart = g.firemodes[t.gunid][g.firemode].zoomaction.start;
	t.gfinish = g.firemodes[t.gunid][g.firemode].zoomaction.finish;
	if ( g.firemodes[t.gunid][g.firemode].zoomaction.laststart.s>0 ) 
	{
		// if last bullet though, use alternative if available
		if ( t.gunmode<=104 || t.gunmode>106 )
		{
			if ( t.weaponammo[g.weaponammoindex+g.ammooffset] <= 1 )
			{
				t.gstart = g.firemodes[t.gunid][g.firemode].zoomaction.laststart;
				t.gfinish = g.firemodes[t.gunid][g.firemode].zoomaction.lastfinish;
				return false;
			}
		}
	}
	return true;
}

bool gun_detectandperformquickrepeatattack(void)
{
	// takes all vars from gun_control (below)
	bool bPerformingQuickRepeatAttack = false;
	if (t.gun[t.gunid].weapontype >= 51 || t.gun[t.gunid].settings.ismelee != 0)
	{
		// for melee up-close combat weapons
		// weapontype ; 0-grenade, 1-pistol, 2-rocket, 3-shotgun, 4-uzi, 5-assault, 51-melee(noammo)
		// during the gunmode animation, and after the 'strike' frame has been passed,
		// we can have a special follow-up animation for more responsive and rapid attacks
		// and will also remove the sense that clicks are being missed (i.e. feels sluggish and faulty)
		// LB: by imposing this by default, we mess up other game projects, so move this to a new gunspec
		// field so it can be controlled on a per weapon basis!
		float fGraceAfterStrike = g.firemodes[t.gunid][0].settings.meleequickrepeat;// 5.0f;
		if (fGraceAfterStrike > 0)
		{
			bool bTriggeredQuickRepeatAttack = false;
			if (t.gunclick == 0 && (KeyState(g.ggunmeleekey) == 0)) t.gunmustreleasefirst = 0;
			if (t.gunmustreleasefirst == 0)
			{
				if (t.gunclick != 0) bTriggeredQuickRepeatAttack = true;
				if (t.gun[t.gunid].settings.ismelee == 2 && KeyState(g.ggunmeleekey) == 1) bTriggeredQuickRepeatAttack = true;
			}
			if (bTriggeredQuickRepeatAttack == true)
			{
				// only permit after a few post-sprike frames so does not look glitchy (and if anim changes, it will be OUTSIDE of this technique)
				if (GetFrame(t.currentgunobj) >= t.gfinish.s + fGraceAfterStrike && GetFrame(t.currentgunobj) <= t.gfinish.e)
				{
					// and only if not using stamina, or if using, that we only if have stamina for it
					char pUserDefinedGlobalMAX[256];
					sprintf(pUserDefinedGlobalMAX, "g_UserGlobal['%s']", "MyStaminaMax");
					int iMyStaminaMAXFromLUA = LuaGetInt(pUserDefinedGlobalMAX);
					char pUserDefinedGlobal[256];
					sprintf(pUserDefinedGlobal, "g_UserGlobal['%s']", "MyStamina");
					int iMyStaminaFromLUA = LuaGetInt(pUserDefinedGlobal);
					int iStaminaCostOfRepeatAttack = 10;
					if (iMyStaminaMAXFromLUA == 0 || (iMyStaminaMAXFromLUA > 0 && iMyStaminaFromLUA >= iStaminaCostOfRepeatAttack))
					{
						// t.gfinish.s is the strke frame, so we want to back up a little so we have some visual run-up to the repeated action
						float fBackUpFrames = (t.gstart.e - t.gstart.s) / 3.0f; // X-way back from start of initial attack (rather than fixed value)
						if (fBackUpFrames < 6)fBackUpFrames = 6;
						float fStartFromRunUpFrame = t.gfinish.s - fBackUpFrames;
						gun_StopObject (t.currentgunobj);
						gun_SetObjectFrame (t.currentgunobj, fStartFromRunUpFrame);
						gun_PlayObject (t.currentgunobj, fStartFromRunUpFrame, t.gfinish.e);
						t.gunmustreleasefirst = 1;

						// extract stamin as cost
						LuaSetFunction ("PlayerStaminaDrain", 1, 0); LuaPushInt (iStaminaCostOfRepeatAttack); LuaCall ();

						// and return to 102 to allow start part to finish, and then gunshoot to do its thing again
						bPerformingQuickRepeatAttack = true;
					}
				}
			}
		}
	}
	return bPerformingQuickRepeatAttack;
}


// ================================================================================================
// GGMAX 2.42: FIRE trigger tracer storage. See the sampler in the gun update above.
// ================================================================================================
int g_ggFireTraceFrames = 0;      // frames left to sample (armed by FIRE_WEAPON)
int g_ggFireTraceEarlyOut = 0;    // frames the gun update returned before ever reading the trigger

struct GGFireTraceRow
{
	int firingmode, gunclick, gunmode, pressedtrigger, mustrelease, ammo, gunid;
	int mefrozen, lowfps, hudscreen, isrunning, moving, disablerunshoot;
};
static GGFireTraceRow g_ggFireTrace[240];
static int g_ggFireTraceCount = 0;

void GGFireTraceReset(int frames)
{
	g_ggFireTraceCount = 0;
	g_ggFireTraceEarlyOut = 0;
	g_ggFireTraceFrames = frames;
}

void GGFireTraceSample(int firingmode, int gunclick, int gunmode, int pressedtrigger,
	int mustrelease, int ammo, int gunid, int mefrozen, int lowfps, int hudscreen,
	int isrunning, int moving, int disablerunshoot)
{
	if (g_ggFireTraceCount >= 240) return;
	GGFireTraceRow& r = g_ggFireTrace[g_ggFireTraceCount++];
	r.firingmode = firingmode; r.gunclick = gunclick; r.gunmode = gunmode;
	r.pressedtrigger = pressedtrigger; r.mustrelease = mustrelease; r.ammo = ammo; r.gunid = gunid;
	r.mefrozen = mefrozen; r.lowfps = lowfps; r.hudscreen = hudscreen;
	r.isrunning = isrunning; r.moving = moving; r.disablerunshoot = disablerunshoot;
}

// Writes Files/firetrace.txt and returns a one-line verdict naming the stage that stopped the shot.
void GGFireTraceDump(char* result, int resultSize)
{
	FILE* f = GGDiagFopen("firetrace.txt", "w");
	if (f != NULL)
	{
		fprintf(f, "GGMAX 2.42 FIRE trace — %d sampled frames, %d frames the gun update EARLY-OUT\n",
			g_ggFireTraceCount, g_ggFireTraceEarlyOut);
		fprintf(f, "%-4s %-5s %-5s %-6s %-8s %-8s %-6s %-4s %-4s %-4s %-4s %-4s %-4s %s\n",
			"n", "fmode", "click", "gmode", "prsTrig", "mustRel", "ammo", "gid", "froz", "lofps", "hud", "run", "mov", "norunshoot");
		for (int i = 0; i < g_ggFireTraceCount; ++i)
		{
			const GGFireTraceRow& r = g_ggFireTrace[i];
			fprintf(f, "%-4d %-5d %-5d %-6d %-8d %-8d %-6d %-4d %-4d %-4d %-4d %-4d %-4d %d\n",
				i, r.firingmode, r.gunclick, r.gunmode, r.pressedtrigger, r.mustrelease, r.ammo,
				r.gunid, r.mefrozen, r.lowfps, r.hudscreen, r.isrunning, r.moving, r.disablerunshoot);
		}
		fclose(f);
	}
	// Verdict. Each branch is a DIFFERENT fault, which is the whole point of sampling rather than
	// reasoning: they all look like "the gun did not fire" from outside.
	int sawFiringmode = 0, sawClick = 0, ammoStart = -1, ammoEnd = -1;
	for (int i = 0; i < g_ggFireTraceCount; ++i)
	{
		if (g_ggFireTrace[i].firingmode == 1) sawFiringmode++;
		if (g_ggFireTrace[i].gunclick == 1) sawClick++;
		if (ammoStart < 0) ammoStart = g_ggFireTrace[i].ammo;
		ammoEnd = g_ggFireTrace[i].ammo;
	}
	const char* verdict;
	if (g_ggFireTraceCount == 0 && g_ggFireTraceEarlyOut > 0)
		verdict = "gun update EARLY-OUT every frame (ObjectExist(hudbankoffset+5)==0) — the trigger is never read";
	else if (g_ggFireTraceCount == 0)
		verdict = "the gun update never ran at all while armed";
	else if (sawFiringmode == 0)
		verdict = "firingmode NEVER reached the gun update as 1 — the hold is not landing where the gun reads it";
	else if (sawClick == 0)
		verdict = "firingmode arrived but a gunclick ZEROING GATE killed it — see the froz/lofps/hud/run/mov columns";
	else if (ammoStart == ammoEnd)
		verdict = "gunclick survived the gates but NO AMMO WAS CONSUMED — blocked below, in the edge latch or fire logic";
	else
		verdict = "SHOT FIRED (ammo decreased)";
	// GGMAX 2.42b: report the HOLD COUNTER too. When firingmode never arrives as 1 there are two
	// very different causes, and this number separates them without another build: if the counter
	// still sits at its armed value, physics_player_gatherkeycontrols (where the hold is applied)
	// never ran at all; if it drained to 0, the hold DID run and something cleared firingmode
	// between that function and the gun update.
	extern int g_ggFireHoldFrames;
	_snprintf(result, resultSize,
		"OK: DUMP_FIRE frames=%d earlyOut=%d firingmode1=%d gunclick1=%d ammo %d->%d holdLeft=%d -> Files/firetrace.txt\nVERDICT: %s%s",
		g_ggFireTraceCount, g_ggFireTraceEarlyOut, sawFiringmode, sawClick, ammoStart, ammoEnd,
		g_ggFireHoldFrames, verdict,
		(sawFiringmode == 0 && g_ggFireHoldFrames > 0)
			? "  >>> HOLD NEVER APPLIED: physics_player_gatherkeycontrols did not run <<<"
			: ((sawFiringmode == 0) ? "  >>> hold WAS applied (counter drained) but firingmode was cleared before the gun read it <<<" : ""));
	result[resultSize - 1] = 0;
}
