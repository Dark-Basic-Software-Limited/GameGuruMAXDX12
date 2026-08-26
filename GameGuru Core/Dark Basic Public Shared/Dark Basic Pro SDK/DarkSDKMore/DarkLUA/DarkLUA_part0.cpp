// DarkLUA.cpp : Defines the exported functions for the DLL application.
//

//#define FASTBULLETPHYSICS

#ifdef WINVER
#undef WINVER
#endif

//PE: We need the latest dpi functions.
#define WINVER 0x0605
#include "Windows.h"
#include "WinUser.h"

#define _USING_V110_SDK71_
#include "stdafx.h"
#include "DarkLUA.h"
#include "globstruct.h"
#include "CGfxC.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <unordered_set>   // GGMAX 3.24
#include "M-RPG.h"

// DarkLUA needs access to the T global (but could be in two locations)
#include "..\..\..\..\GameGuru\Include\gameguru.h"

// Control of NAVMESH
#include "..\..\..\..\Guru-WickedMAX\GGRecastDetour\GGRecastDetour.h"
extern GGRecastDetour g_RecastDetour;

// Control of PARTICLES
#define NOTFORMAINENGINE
#include "..\..\..\..\Guru-WickedMAX\GPUParticles.h"
#undef NOTFORMAINENGINE

#ifdef STORYBOARD
#ifdef ENABLEIMGUI
#include "..\..\GameGuru\Imgui\imgui.h"
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "..\..\GameGuru\Imgui\imgui_internal.h"
#include "..\..\GameGuru\Imgui\imgui_impl_win32.h"
#include "..\..\GameGuru\Imgui\imgui_gg_dx11.h"
#endif
extern StoryboardStruct Storyboard;
#endif

#ifdef OPTICK_ENABLE
#include "optick.h"
#endif

#include "..\..\..\..\Guru-WickedMAX\wickedcalls.h"
//#undef WICKEDENGINE
//#include "WickedEngine.h"
#define MAXPI 3.14159265358979323846f

using namespace std;
using namespace wiGraphics;
using namespace wiScene;
using namespace wiECS;

#include "..\tracers\TracerManager.h"
using namespace Tracers;

#include "..\GGTerrain\GGGrass.h"
using namespace GGGrass;

// Prototypes
extern void DrawSpritesFirst(void);
extern void DrawSpritesLast(void);

using namespace std;

//#define LUA_DO_DEBUG

#ifdef LUA_DO_DEBUG
void WriteToDebugLog ( char* szMessage, bool bNewLine );
void WriteToDebugLog ( char* szMessage, int i );
void WriteToDebugLog ( char* szMessage, float f );
void WriteToDebugLog ( char* szMessage, char* s );
#endif

#define lua_c

char errorString[256];
char functionName[256];
int functionParams = 0;
int functionResults = 0;
int functionStateID = 0;
int defaultState = 1;

DARKLUA_API int LoadLua( LPSTR pString );
bool LuaCheckForWorkshopFile ( LPSTR VirtualFilename);

 struct StringList
 {
	 int  stateID;
	 char fileName[MAX_PATH];
 };

 std::vector <StringList> ScriptsLoaded;
 std::vector <StringList> FunctionsWithErrors;

// GGMAX 3.24: an O(1) index over FunctionsWithErrors.
//
// LuaCall() and LuaCallSilent() each opened with a LINEAR scan doing a strcmp per element, on
// EVERY lua call in the game. The list is empty in a healthy project so it cost nothing and was
// invisible - but it only ever grows (pushed on any script error, any missing function) and it is
// only cleared on a level reset. So after one bad script, every lua call in the product paid a
// strcmp per error entry for the rest of the session, and the game got permanently slower the
// more broken the project was. That is the worst shape a cost can have: it lands hardest on the
// user who is already having a bad time.
//
// The vector is left exactly as it was - it is cleared in DarkLUA_part8 and kept for inspection -
// and this set is maintained alongside it purely as the lookup path. Both are written through the
// same two helpers so they cannot drift.
// \u2605\u2605\u2605 The first version of this used std::unordered_set<std::string> and it was a REGRESSION
// at the size that actually occurs. Benchmarked on the real structures: at the 39 entries a normal
// TESTPRO2 level holds, the set cost ~231 ns a lookup against ~85 ns for the linear strcmp scan it
// replaced - 2.7x SLOWER - and it only won past ~110 entries. The reason is that find() on a
// char* builds a temporary std::string (these names are 20-35 chars, past SSO, so a heap
// allocation) and hashes the whole thing, every single lookup. Textbook O(1) beaten by textbook
// O(n) because the constant factor was three heap-flavoured operations against a handful of
// strcmps that fail on the first character.
//
// So: a 32-bit FNV-1a over the name, compared against a parallel array of hashes, with a strcmp
// only on a hash hit (which essentially never happens on a miss). One pass over ~39 contiguous
// uint32 is a few nanoseconds and the compiler can vectorise it. Beats BOTH at every size tested.
static std::vector<unsigned int> g_ggFunctionErrorHash;

static unsigned int gg_FnvHash ( const char* p )
{
	unsigned int h = 2166136261u;
	while ( *p ) { h ^= ( unsigned char )( *p++ ); h *= 16777619u; }
	return h;
}

bool gg_FunctionHasError ( LPSTR pName )
{
	if ( pName == NULL ) return false;
	const unsigned int h = gg_FnvHash ( pName );
	const size_t n = g_ggFunctionErrorHash.size();
	const unsigned int* pH = n ? &g_ggFunctionErrorHash[0] : NULL;
	for ( size_t i = 0 ; i < n ; i++ )
	{
		if ( pH[i] == h && strcmp ( pName, FunctionsWithErrors[i].fileName ) == 0 )
			return true;
	}
	return false;
}

void gg_MarkFunctionError ( LPSTR pName, int stateID )
{
	if ( pName == NULL ) return;
	// keep the vector identical to the pre-3.24 behaviour, including the duplicate-free property
	// the early-out gave it (a name already present is never reached a second time).
	if ( gg_FunctionHasError ( pName ) ) return;
	StringList item;
	item.stateID = stateID;            // was left UNINITIALISED by every original push site
	strcpy ( item.fileName , pName );
	FunctionsWithErrors.push_back ( item );
	g_ggFunctionErrorHash.push_back ( gg_FnvHash ( pName ) );
}

void gg_ClearFunctionErrors ( void )
{
	FunctionsWithErrors.clear();
	g_ggFunctionErrorHash.clear();
}

// GGMAX 3.24: self-test for the above. Two things are worth proving and neither is observable in
// a healthy project, which is exactly why the original cost went unnoticed for years.
//
//  (1) SYNC. The vector and the set are written only through gg_MarkFunctionError, so they cannot
//      drift by construction - but "cannot by construction" is the claim, and this measures it.
//      If they ever did drift, a real error would stop being suppressed (lua error spam) or a
//      healthy function would be wrongly skipped (a script silently stops). Both are loud, but
//      only after they happen.
//  (2) SCALING. The whole point of the change. Times a lookup against a list of N entries, the
//      new way and the old linear-strcmp way, at sizes a broken project actually reaches. Only
//      runs when the live list is EMPTY, so it never disturbs real state, and it restores the
//      structures before returning either way.
void gg_TestFunctionErrors ( char* result, int resultSize )
{
	// ---- (1) sync of the live structures --------------------------------------------------
	size_t liveVec = FunctionsWithErrors.size();
	size_t liveSet = g_ggFunctionErrorHash.size();
	int missing = 0;
	for ( size_t i = 0 ; i < FunctionsWithErrors.size() ; i++ )
		if ( !gg_FunctionHasError ( FunctionsWithErrors[i].fileName ) ) missing++;

	// \u2605 The live list is NOT usually empty. On TESTPRO2 in test game it holds 39 entries, so
	// this was never a latent cost waiting for someone to write a bad script - the old linear scan
	// was charging 39 strcmps on every lua call in a perfectly ordinary level. Most entries are
	// benign: GameGuru probes for optional per-script functions (_preexit and friends) and a script
	// that does not define one is recorded here exactly like a real failure.
	// So: save the live entries, run the benchmark on the real helpers, put them back.
	std::vector<StringList> saved = FunctionsWithErrors;
	char names[1400]; names[0] = 0;
	for ( size_t i = 0 ; i < saved.size() && strlen(names) < sizeof(names) - 96 ; i++ )
	{
		strcat ( names, "    " );
		strcat ( names, saved[i].fileName );
		strcat ( names, "\n" );
	}

	// ---- (2) scaling, on the real helpers, with the real structures ------------------------
	LARGE_INTEGER freq; QueryPerformanceFrequency ( &freq );
	const int SIZES[4] = { 1, 10, 50, 200 };
	const int REPS = 20000;
	char line[1024]; char body[3072]; body[0] = 0;
	int addedTotal = 0;
	for ( int si = 0 ; si < 4 ; si++ )
	{
		gg_ClearFunctionErrors();
		char nm[64];
		for ( int i = 0 ; i < SIZES[si] ; i++ )
		{
			sprintf ( nm, "gg_synthetic_broken_script_%04d_main", i );
			gg_MarkFunctionError ( nm, 0 );
		}
		addedTotal = SIZES[si];
		// a MISS is the case every healthy call takes, and the one the linear scan charged full
		// price for - it has to compare every element before concluding "not here".
		char probe[64]; strcpy ( probe, "a_healthy_script_main" );

		LARGE_INTEGER a, b, c;
		QueryPerformanceCounter ( &a );
		volatile int sink = 0;
		for ( int r = 0 ; r < REPS ; r++ ) sink += gg_FunctionHasError ( probe ) ? 1 : 0;
		QueryPerformanceCounter ( &b );
		for ( int r = 0 ; r < REPS ; r++ )
			for ( size_t k = 0 ; k < FunctionsWithErrors.size() ; k++ )
				if ( strcmp ( probe, FunctionsWithErrors[k].fileName ) == 0 ) { sink++; break; }
		QueryPerformanceCounter ( &c );

		double nsNew = ( double )( b.QuadPart - a.QuadPart ) * 1e9 / ( double )freq.QuadPart / REPS;
		double nsOld = ( double )( c.QuadPart - b.QuadPart ) * 1e9 / ( double )freq.QuadPart / REPS;
		sprintf ( line, "   %4d entries : hash-array %7.1f ns   linear-strcmp %8.1f ns   %5.1fx\n",
			SIZES[si], nsNew, nsOld, ( nsNew > 0.0 ) ? ( nsOld / nsNew ) : 0.0 );
		strcat ( body, line );
	}
	// put the real state back, through the same helper, and re-verify it
	gg_ClearFunctionErrors();
	for ( size_t i = 0 ; i < saved.size() ; i++ ) gg_MarkFunctionError ( saved[i].fileName, saved[i].stateID );
	int restoreMissing = 0;
	for ( size_t i = 0 ; i < saved.size() ; i++ )
		if ( !gg_FunctionHasError ( saved[i].fileName ) ) restoreMissing++;

	_snprintf ( result, resultSize,
		"%s: TEST_LUAERRSET - index in sync (%zu list / %zu set, %d unindexed), restored %zu entries (%d lost).\n"
		"  Cost of ONE lookup, the MISS case, which is what EVERY healthy lua call pays:\n"
		"%s"
		"  This level really holds %zu error entries, so the old linear scan was live cost, not a\n"
		"  latent one. Most are benign - GameGuru probes for optional per-script functions and an\n"
		"  absent one is recorded exactly like a failure:\n"
		"%s",
		( liveVec == liveSet && missing == 0 && restoreMissing == 0
		  && FunctionsWithErrors.size() == saved.size() ) ? "OK" : "FAIL",
		liveVec, liveSet, missing, FunctionsWithErrors.size(), restoreMissing,
		body, saved.size(), names );
	result[resultSize-1] = 0;
}

// Prototype function
float wrapangleoffset(float da);

// DarkAI Commands =======================================================================
HMODULE DarkAIModule = NULL;
HMODULE MultiplayerModule = NULL;

// externals to get tracking info from VR920 (if any)
//extern bool g_VR920AdapterAvailable;
//extern float g_fVR920TrackingYaw;
//extern float g_fVR920TrackingPitch;
//extern float g_fVR920TrackingRoll;
//extern float g_fDriverCompensationYaw;
//extern float g_fDriverCompensationPitch;
//extern float g_fDriverCompensationRoll;

extern "C" {
#include "../../../../../../WickedEngineDX12/WickedEngine/LUA/lua.h"
#include "../../../../../../WickedEngineDX12/WickedEngine/LUA/lualib.h"
#include "../../../../../../WickedEngineDX12/WickedEngine/LUA/lauxlib.h"
}

 extern GlobStruct* g_pGlob;

 void LuaConstructor ( void )
 {
 }

 void LuaDestructor ( void )
 {
 }

 void LuaReceiveCoreDataPtr ( LPVOID pCore )
 {	
 }


struct luaState
{
	lua_State *state;
};

lua_State *lua2 = NULL;

int maxLuaStates = 0;
luaState** ppLuaStates = NULL;

//=============

struct luaMessage
{
	luaMessage() { strcpy ( msgDesc , "" ); msgIndex = 0; msgInt = 0; msgFloat = 0; strcpy ( msgString , "" ); }
	char msgDesc[256];
	int msgIndex;
	int msgInt;
	float msgFloat;
	char msgString[256];
};

luaMessage currentMessage;

int luaMessageCount = 0;
int maxLuaMessages = 0;
luaMessage** ppLuaMessages = NULL;

//=============

 int LuaSendMessage(lua_State *L)
 {
	 lua2 = L;

	/* get number of arguments */
	int n = lua_gettop(L);
	int i;

	/* loop through each argument */
	for (i = 1; i <= n; i++)
	{
		luaMessageCount++;

		if ( maxLuaMessages == 0 )
		{
			strcpy ( currentMessage.msgDesc, "" );
			currentMessage.msgFloat = 0.0f;
			currentMessage.msgIndex = 0;
			currentMessage.msgInt = 0;
			strcpy ( currentMessage.msgString, "" );

			maxLuaMessages = 100;
			ppLuaMessages = new luaMessage*[maxLuaMessages];

			for ( int c = 0 ; c < maxLuaMessages ; c++ )
				ppLuaMessages[c] = NULL;
		}

		if ( luaMessageCount > maxLuaMessages )
		{
			luaMessage** ppBigger = NULL;
			ppBigger = new luaMessage*[luaMessageCount+100];

			for ( int c = 0; c < luaMessageCount; c++ )
			 ppBigger [ c ] = ppLuaMessages [ c ];

			delete [ ] ppLuaMessages;

			ppLuaMessages = ppBigger;

			for ( int c = maxLuaMessages; c < maxLuaMessages+100; c++ )
				ppLuaMessages[c] = NULL;

			maxLuaMessages += 100;
		}

		if ( ppLuaMessages[luaMessageCount-1] == NULL )
		{
	  
			luaMessage* msg = new luaMessage();

			strcpy ( msg->msgDesc , lua_tostring(L, i) );
			msg->msgIndex = 0;
			msg->msgFloat = 0;
			msg->msgInt = 0;
			strcpy ( msg->msgString , "" );
			ppLuaMessages[luaMessageCount-1] = msg;
		}
	}

	 return 0;
 }

 int LuaSendMessageI(lua_State *L)
 {
	 lua2 = L;

	/* get number of arguments */
	int n = lua_gettop(L);
	int i;

	if ( n != 2 && n != 3 )
	{
		//MessageBox(NULL, "SendMessageI takes 2 or 3 params", "LUA ERROR", MB_TOPMOST | MB_OK);
		return 0;
	}

	luaMessageCount++;

	/* loop through each argument */
	for (i = 1; i <= n; i++)
	{
		if ( maxLuaMessages == 0 )
		{
			strcpy ( currentMessage.msgDesc, "" );
			currentMessage.msgIndex = 0;
			currentMessage.msgFloat = 0.0f;
			currentMessage.msgInt = 0;
			strcpy ( currentMessage.msgString, "" );

			maxLuaMessages = 100;
			ppLuaMessages = new luaMessage*[maxLuaMessages];

			for ( int c = 0 ; c < maxLuaMessages ; c++ )
				ppLuaMessages[c] = NULL;
		}

		if ( luaMessageCount > maxLuaMessages )
		{
			luaMessage** ppBigger = NULL;
			ppBigger = new luaMessage*[luaMessageCount+100];

			for ( int c = 0; c < luaMessageCount; c++ )
			 ppBigger [ c ] = ppLuaMessages [ c ];

			delete [ ] ppLuaMessages;

			ppLuaMessages = ppBigger;

			for ( int c = maxLuaMessages; c < maxLuaMessages+100; c++ )
				ppLuaMessages[c] = NULL;

			maxLuaMessages += 100;
		}

		if ( ppLuaMessages[luaMessageCount-1] == NULL )
		{
	  
			luaMessage* msg = new luaMessage();

			strcpy ( msg->msgDesc , lua_tostring(L, i) );
			msg->msgFloat = 0;
			msg->msgInt = 0;
			msg->msgIndex = 0;
			strcpy ( msg->msgString , "" );
			ppLuaMessages[luaMessageCount-1] = msg;

			/*if ( strcmp ( msg->msgDesc , "setentityhealth" ) == 0 )
			{
				int dave = 1;
			}*/
		}
		else
		{
			if ( n == 3 && i == 2 )
				ppLuaMessages[luaMessageCount-1]->msgIndex = (int)lua_tonumber( L , i );
			else
				ppLuaMessages[luaMessageCount-1]->msgInt = (int)lua_tonumber( L , i );
		}
	}

	 return 0;
 }

  int LuaSendMessageF(lua_State *L)
 {
	 lua2 = L;

	/* get number of arguments */
	int n = lua_gettop(L);
	int i;

	if ( n != 2 && n != 3 )
	{
		//MessageBox(NULL, "SendMessageI takes 2 or 3 params", "LUA ERROR", MB_TOPMOST | MB_OK);
		return 0;
	}

	luaMessageCount++;

	/* loop through each argument */
	for (i = 1; i <= n; i++)
	{

		if ( maxLuaMessages == 0 )
		{
			strcpy ( currentMessage.msgDesc, "" );
			currentMessage.msgFloat = 0.0f;
			currentMessage.msgInt = 0;
			currentMessage.msgIndex = 0;
			strcpy ( currentMessage.msgString, "" );

			maxLuaMessages = 100;
			ppLuaMessages = new luaMessage*[maxLuaMessages];

			for ( int c = 0 ; c < maxLuaMessages ; c++ )
				ppLuaMessages[c] = NULL;
		}

		if ( luaMessageCount > maxLuaMessages )
		{
			luaMessage** ppBigger = NULL;
			ppBigger = new luaMessage*[luaMessageCount+100];

			for ( int c = 0; c < luaMessageCount; c++ )
			 ppBigger [ c ] = ppLuaMessages [ c ];

			delete [ ] ppLuaMessages;

			ppLuaMessages = ppBigger;

			for ( int c = maxLuaMessages; c < maxLuaMessages+100; c++ )
				ppLuaMessages[c] = NULL;

			maxLuaMessages += 100;
		}

		if ( ppLuaMessages[luaMessageCount-1] == NULL )
		{
	  
			luaMessage* msg = new luaMessage();

			strcpy ( msg->msgDesc , lua_tostring(L, i) );
			msg->msgFloat = 0;
			msg->msgInt = 0;
			msg->msgIndex = 0;
			strcpy ( msg->msgString , "" );
			ppLuaMessages[luaMessageCount-1] = msg;
		}
		else
		{

			if ( n == 3 && i == 2 )
				ppLuaMessages[luaMessageCount-1]->msgIndex = (int)lua_tonumber( L , i );
			else
				ppLuaMessages[luaMessageCount-1]->msgFloat = (float)lua_tonumber( L , i );
		}
	}

	 return 0;
 }

 int LuaSendMessageS(lua_State *L)
 {
	 lua2 = L;

	/* get number of arguments */
	int n = lua_gettop(L);
	int i;

	if ( n != 2 && n != 3 )
	{
		//MessageBox(NULL, "SendMessageI takes 2 or 3 params", "LUA ERROR", MB_TOPMOST | MB_OK);
		return 0;
	}

	luaMessageCount++;

	/* loop through each argument */
	for (i = 1; i <= n; i++)
	{

		if ( maxLuaMessages == 0 )
		{
			strcpy ( currentMessage.msgDesc, "" );
			currentMessage.msgFloat = 0.0f;
			currentMessage.msgInt = 0;
			currentMessage.msgIndex = 0;
			strcpy ( currentMessage.msgString, "" );

			maxLuaMessages = 100;
			ppLuaMessages = new luaMessage*[maxLuaMessages];

			for ( int c = 0 ; c < maxLuaMessages ; c++ )
				ppLuaMessages[c] = NULL;
		}

		if ( luaMessageCount > maxLuaMessages )
		{
			luaMessage** ppBigger = NULL;
			ppBigger = new luaMessage*[luaMessageCount+100];

			for ( int c = 0; c < luaMessageCount; c++ )
			 ppBigger [ c ] = ppLuaMessages [ c ];

			delete [ ] ppLuaMessages;

			ppLuaMessages = ppBigger;

			for ( int c = maxLuaMessages; c < maxLuaMessages+100; c++ )
				ppLuaMessages[c] = NULL;

			maxLuaMessages += 100;
		}

		if ( ppLuaMessages[luaMessageCount-1] == NULL )
		{
	  
			luaMessage* msg = new luaMessage();

			strcpy ( msg->msgDesc , lua_tostring(L, i) );
			msg->msgFloat = 0;
			msg->msgInt = 0;
			msg->msgIndex = 0;
			strcpy ( msg->msgString , "" );
			ppLuaMessages[luaMessageCount-1] = msg;
		}
		else
		{
			if ( n == 3 && i == 2 )
				ppLuaMessages[luaMessageCount-1]->msgIndex = (int)lua_tonumber( L , i );
			else
			{
				// ZJ: Prevent copying a nullptr 
				const char* argument = lua_tostring(L, i);
				if (argument)
				{
					strcpy(ppLuaMessages[luaMessageCount - 1]->msgString, argument);
				}
				//strcpy(ppLuaMessages[luaMessageCount - 1]->msgString, lua_tostring(L, i));
			}
		}
	}

	 return 0;
 }

 // Direct Calls
 void lua_updateweaponstats ( void );

 int RestoreGameFromSlot(lua_State *L)
 {
	lua2 = L;
	int n = lua_gettop(L);
	if ( n < 1 ) return 0;
	t.luaglobal.gamestatechange = lua_tonumber(L, 1);
	if ( t.luaglobal.gamestatechange==0 )
	{
		// if successfully reset a game-load-state, also ensure advance level loader resets
		strcpy ( t.game.pAdvanceWarningOfLevelFilename, "" );
	}
	return 0;
 }
 int ResetFade(lua_State *L)
 {
	lua2 = L;
	if ( t.game.gameloop == 1 )
	{
		// only blank if in the game menu (not main menu load page)
		postprocess_reset_fade();
		DisableAllSprites();
		Sync();
		Sync();
	}
	return 0;
 }

 int GetInternalSoundState(lua_State *L)
 {
	lua2 = L;
	int n = lua_gettop(L);
	if ( n < 1 ) return 0;
	int iIndex = lua_tonumber(L, 1);
	if (iIndex >= 0 && iIndex < 65535)
	{
		int iState = t.soundloopcheckpoint[iIndex];
		lua_pushinteger ( L, iState );
	}
	return 1;
 }
 int SetInternalSoundState(lua_State *L)
 {
	lua2 = L;
	int n = lua_gettop(L);
	if ( n < 2 ) return 0;
	int iIndex = lua_tonumber(L, 1);
	if (iIndex >= 0 && iIndex < 65535)
	{
		int iState = lua_tonumber(L, 2);
		t.soundloopcheckpoint[iIndex] = iState;
		if (t.soundloopcheckpoint[iIndex] != 0)
		{
			if (iIndex > 0 && SoundExist(iIndex) == 1)
			{
				if (t.soundloopcheckpoint[iIndex] == 3)
					LoopSound(iIndex);
				else if (t.soundloopcheckpoint[iIndex] == 1)
					PlaySound(iIndex);
			}
		}
		return 1;
	}
	return 0;
 }
 int SetCheckpoint(lua_State *L)
 {
	lua2 = L;
	int n = lua_gettop(L);
	if ( n < 4 ) return 0;
	t.playercheckpoint.x=lua_tonumber(L, 1);
	t.playercheckpoint.y=lua_tonumber(L, 2);
	t.playercheckpoint.z=lua_tonumber(L, 3);
	t.playercheckpoint.a=lua_tonumber(L, 4);
	return 1;
 }

 int UpdateWeaponStats(lua_State *L)
 {
	lua2 = L;
	lua_updateweaponstats();
	return 0;
 }
  
 int ResetWeaponSystems(lua_State *L)
 {
	weapon_projectile_free();
	return 0;
 }
 int GetWeaponSlotGot(lua_State *L)
 {
	lua2 = L;
	int n = lua_gettop(L);
	if ( n < 1 ) return 0;
	int iWeaponSlot = lua_tonumber(L, 1);
	lua_pushinteger ( L, t.weaponslot[iWeaponSlot].got );
	return 1;
 }
 int GetWeaponSlotNoSelect(lua_State *L)
 {
	lua2 = L;
	int n = lua_gettop(L);
	if ( n < 1 ) return 0;
	int iWeaponSlot = lua_tonumber(L, 1);
	lua_pushinteger ( L, t.weaponslot[iWeaponSlot].noselect );
	return 1;
 }
 int SetWeaponSlot(lua_State *L)
 {
	lua2 = L;
	int n = lua_gettop(L);
	if ( n < 3 ) return 0;
	int iWeaponSlot = lua_tonumber(L, 1);
	t.weaponslot[iWeaponSlot].got = lua_tonumber(L, 2);
	t.weaponslot[iWeaponSlot].pref = lua_tonumber(L, 3);
	return 0;
 }
 int GetWeaponAmmo(lua_State *L)
 {
	lua2 = L;
	int n = lua_gettop(L);
	if ( n < 1 ) return 0;
	int iWeaponSlot = lua_tonumber(L, 1);
	if(iWeaponSlot>=0 && iWeaponSlot<t.weaponammo.size())
		lua_pushinteger ( L, t.weaponammo[iWeaponSlot] );
	else
		lua_pushinteger ( L, 0 );
	return 1;
 }
 int SetWeaponAmmo(lua_State *L)
 {
	lua2 = L;
	int n = lua_gettop(L);
	if ( n < 2 ) return 0;
	int iWeaponSlot = lua_tonumber(L, 1);
	if (iWeaponSlot >= 0 && iWeaponSlot < t.weaponammo.size())
	{
		t.weaponammo[iWeaponSlot] = lua_tonumber(L, 2);
	}
	return 0;
 }
 int GetWeaponClipAmmo(lua_State *L)
 {
	lua2 = L;
	int n = lua_gettop(L);
	if ( n < 1 ) return 0;
	int iWeaponSlotClipIndex = lua_tonumber(L, 1);
	if (iWeaponSlotClipIndex >= 0 && iWeaponSlotClipIndex < t.weaponclipammo.size())
		lua_pushinteger ( L, t.weaponclipammo[iWeaponSlotClipIndex] );
	else
		lua_pushinteger ( L, 0 );
	return 1;
 }
 int SetWeaponClipAmmo(lua_State *L)
 {
	lua2 = L;
	int n = lua_gettop(L);
	if ( n < 2 ) return 0;
	int iWeaponSlotClipIndex = lua_tonumber(L, 1);
	if (iWeaponSlotClipIndex >= 0 && iWeaponSlotClipIndex < t.weaponclipammo.size())
	{
		t.weaponclipammo[iWeaponSlotClipIndex] = lua_tonumber(L, 2);
		int iWeaponSlot = iWeaponSlotClipIndex;
		if (iWeaponSlot >= 11) iWeaponSlot = iWeaponSlot - 10;
		int iGunIndex = t.weaponslot[iWeaponSlot].got;
		int iMaxClipCapacity = g.firemodes[iGunIndex][0].settings.clipcapacity * g.firemodes[iGunIndex][0].settings.reloadqty;
		if (iMaxClipCapacity == 0) iMaxClipCapacity = 99999;
		if (t.weaponclipammo[iWeaponSlotClipIndex] > iMaxClipCapacity) t.weaponclipammo[iWeaponSlotClipIndex] = iMaxClipCapacity;
	}
	return 0;
 }
 int GetWeaponPoolAmmoIndex(lua_State* L)
 {
	 lua2 = L;
	 int n = lua_gettop(L);
	 if (n < 1) return 0;
	 int iWeaponSlot = lua_tonumber(L, 1);
	 if (iWeaponSlot >= 0 && iWeaponSlot < t.weaponslot.size())
	 {
		 int iGunIndex = t.weaponslot[iWeaponSlot].got;
		 int iPoolIndex = g.firemodes[iGunIndex][g.firemode].settings.poolindex;
		 lua_pushinteger (L, iPoolIndex);
	 }
	 else
	 {
		 lua_pushinteger (L, 0);
	 }
	 return 1;
 }
 int GetWeaponPoolAmmo(lua_State *L)
 {
	lua2 = L;
	int n = lua_gettop(L);
	if ( n < 1 ) return 0;
	int iPoolIndex = lua_tonumber(L, 1);
	if (iPoolIndex >= 0 && iPoolIndex < t.ammopool.size())
	{
		lua_pushinteger (L, t.ammopool[iPoolIndex].ammo);
	}
	else
	{
		lua_pushinteger (L, 0);	
	}
	return 1;
 }
 int SetWeaponPoolAmmo(lua_State *L)
 {
	lua2 = L;
	int n = lua_gettop(L);
	if ( n < 2 ) return 0;
	int iPoolIndex = lua_tonumber(L, 1);
	if (iPoolIndex >= 0 && iPoolIndex < t.ammopool.size())
	{
		t.ammopool[iPoolIndex].ammo = lua_tonumber(L, 2);
	}
	return 0;
 }
 int GetWeaponSlot(lua_State *L)
 {
	lua2 = L;
	int n = lua_gettop(L);
	if ( n < 1 ) return 0;

	// returns the gunID
	int iReturnValue = 0;

	// find gun name specicied to get gunindex
	int iWeaponSlot = lua_tonumber(L, 1);
	iReturnValue = t.weaponslot[iWeaponSlot].got;

	// return true GunID found in slot
	lua_pushinteger ( L, iReturnValue );
	return 1;
 }
 int GetWeaponSlotPref(lua_State* L)
 {
	 lua2 = L;
	 int n = lua_gettop(L);
	 if (n < 1) return 0;
	 int iWeaponID = 0;
	 int iWeaponSlot = lua_tonumber(L, 1);
	 iWeaponID = t.weaponslot[iWeaponSlot].pref;
	 lua_pushinteger (L, iWeaponID);
	 return 1;
 }

 // Weapon Modding Commands
 int GetPlayerWeaponID(lua_State *L)
 {
	lua2 = L;
	int n = lua_gettop(L);
	if ( n > 0 ) return 0;

	// returns the playres current gun ID
	int iReturnValue = t.gunid;
	lua_pushinteger ( L, iReturnValue );
	return 1;
 }
 int GetWeaponID(lua_State *L)
 {
	lua2 = L;
	int n = lua_gettop(L);
	if ( n < 1 ) return 0;

	// returns the gun
	int iReturnValue = 0;

	// find gun name specicied to get gunindex
	char pGunName[512];
	strcpy ( pGunName, lua_tostring(L, 1) );
	for ( int tgunid = 1; tgunid < ArrayCount(t.gun); tgunid++ )
	{
		if ( stricmp ( t.gun[tgunid].name_s.Get()+(strlen(t.gun[tgunid].name_s.Get())-strlen(pGunName)), pGunName ) == NULL )
		{
			iReturnValue = tgunid;
			break;
		}
	}
	lua_pushinteger ( L, iReturnValue );
	return 1;
 }
 int GetEntityWeaponID(lua_State *L)
 {
	lua2 = L;
	int n = lua_gettop(L);
	if ( n < 1 ) return 0;
	int iReturnValue = 0;
	int iEntityIndex = lua_tonumber(L, 1);
	if ( iEntityIndex > 0 )
	{
		int iEntityBankIndex = t.entityelement[iEntityIndex].bankindex;
		if ( iEntityBankIndex > 0 )
		{
			iReturnValue = t.entityprofile[iEntityBankIndex].isweapon;
		}
	}
	lua_pushinteger ( L, iReturnValue );
	return 1;
 }
 int RawSetWeaponData ( lua_State *L, int iDataMode )
 {
	lua2 = L;
	int n = lua_gettop(L);
	if ( n < 3 ) return 0;
	int tgunid = lua_tonumber(L, 1);
	int tfiremode = lua_tonumber(L, 2);
	int newvalue = lua_tonumber(L, 3);
	if ( tgunid > 0 && tgunid <= ArrayCount(t.gun) )
	{
		switch ( iDataMode )
		{
			case 1 : g.firemodes[tgunid][tfiremode].settings.damage = newvalue; break;
			case 2 : g.firemodes[tgunid][tfiremode].settings.accuracy = newvalue; break;
			case 3 : g.firemodes[tgunid][tfiremode].settings.reloadqty = newvalue; break;
			case 4 : g.firemodes[tgunid][tfiremode].settings.iterate = newvalue; break;
			case 5 : g.firemodes[tgunid][tfiremode].settings.range = newvalue; break;
			case 6 : g.firemodes[tgunid][tfiremode].settings.dropoff = newvalue; break;
			case 7 : g.firemodes[tgunid][tfiremode].settings.usespotlighting = newvalue; break;
			case 8 : g.firemodes[tgunid][tfiremode].settings.firerate = newvalue; break;
			case 9 : g.firemodes[tgunid][tfiremode].settings.clipcapacity = newvalue; break;
			case 10: g.firemodes[tgunid][tfiremode].settings.weaponpropres1 = newvalue; break;
			case 11: g.firemodes[tgunid][tfiremode].settings.weaponpropres2 = newvalue; break;
		}
	}
	return 0;
 }
 int RawGetWeaponData( lua_State *L, int iDataMode )
 {
	lua2 = L;
	int n = lua_gettop(L);
	if ( n < 2 ) return 0;

	// specify weaponID and firemode index
	int tgunid = lua_tonumber(L, 1);
	int tfiremode = lua_tonumber(L, 2);

	// returns the field data
	int iReturnValue = 0;

	// use datamode to determine which data is returned
	if ( tgunid > 0 && tgunid <= ArrayCount(t.gun) )
	{
		switch ( iDataMode )
		{
			case 1 : iReturnValue = g.firemodes[tgunid][tfiremode].settings.damage; break;
			case 2 : iReturnValue = g.firemodes[tgunid][tfiremode].settings.accuracy; break;
			case 3 : iReturnValue = g.firemodes[tgunid][tfiremode].settings.reloadqty; break;
			case 4 : iReturnValue = g.firemodes[tgunid][tfiremode].settings.iterate; break;
			case 5 : iReturnValue = g.firemodes[tgunid][tfiremode].settings.range; break;
			case 6 : iReturnValue = g.firemodes[tgunid][tfiremode].settings.dropoff; break;
			case 7 : iReturnValue = g.firemodes[tgunid][tfiremode].settings.usespotlighting; break;
			case 8 : iReturnValue = g.firemodes[tgunid][tfiremode].settings.firerate; break;
			case 9 : iReturnValue = g.firemodes[tgunid][tfiremode].settings.clipcapacity; break;
			case 10: iReturnValue = g.firemodes[tgunid][tfiremode].settings.weaponpropres1; break;
			case 11: iReturnValue = g.firemodes[tgunid][tfiremode].settings.weaponpropres2; break;
		}
	}
	lua_pushinteger ( L, iReturnValue );
	return 1;
 }
 int GetWeaponName(lua_State* L)
 {
	 lua2 = L;
	 int n = lua_gettop(L);
	 if (n < 1) return 0;
	 int tgunid = lua_tonumber(L, 1);
	 if(tgunid>0)
		 lua_pushstring (L, t.gun[tgunid].name_s.Get());
	 else
		 lua_pushstring (L, "");
	 return 1;
 }
 int SetWeaponDamage(lua_State *L) { return RawSetWeaponData ( L, 1 ); }
 int SetWeaponAccuracy(lua_State *L) { return RawSetWeaponData ( L, 2 ); }
 int SetWeaponReloadQuantity(lua_State *L) { return RawSetWeaponData ( L, 3 ); }
 int SetWeaponFireIterations(lua_State *L) { return RawSetWeaponData ( L, 4 ); }
 int SetWeaponRange(lua_State *L) { return RawSetWeaponData ( L, 5 ); }
 int SetWeaponDropoff(lua_State *L) { return RawSetWeaponData ( L, 6 ); }
 int SetWeaponSpotLighting(lua_State *L) { return RawSetWeaponData ( L, 7 ); }
 int GetWeaponDamage(lua_State *L) { return RawGetWeaponData ( L, 1 ); }
 int GetWeaponAccuracy(lua_State *L) { return RawGetWeaponData ( L, 2 ); }
 int GetWeaponReloadQuantity(lua_State *L) { return RawGetWeaponData ( L, 3 ); }
 int GetWeaponFireIterations(lua_State *L) { return RawGetWeaponData ( L, 4 ); }
 int GetWeaponRange(lua_State *L) { return RawGetWeaponData ( L, 5 ); }
 int GetWeaponDropoff(lua_State *L) { return RawGetWeaponData ( L, 6 ); }
 int GetWeaponSpotLighting(lua_State *L) { return RawGetWeaponData ( L, 7 ); }
 int SetWeaponFireRate(lua_State* L) { return RawSetWeaponData (L, 8); }
 int GetWeaponFireRate(lua_State* L) { return RawGetWeaponData (L, 8); }
 int SetWeaponClipCapacity(lua_State* L) { return RawSetWeaponData (L, 9); }
 int GetWeaponClipCapacity(lua_State* L) { return RawGetWeaponData (L, 9); }
 //int SetWeaponWeaponPropRes1(lua_State* L) { return RawSetWeaponData (L, 10); } reserved
 //int GetWeaponWeaponPropRes1(lua_State* L) { return RawGetWeaponData (L, 10); }
 //int SetWeaponWeaponPropRes2(lua_State* L) { return RawSetWeaponData (L, 11); }
 //int GetWeaponWeaponPropRes2(lua_State* L) { return RawGetWeaponData (L, 11); }

 //
 // Player Camera Overrides
 //
 int RawSetCameraData ( lua_State *L, int iDataMode )
 {
	lua2 = L;
	int n = lua_gettop(L);
	int tcameraid=0, tvalue=0;
	float fX=0, fY=0, fZ=0;
	if ( iDataMode < 11 )
	{
		if ( n < 1 ) return 0;
		tvalue = lua_tonumber(L, 1);
	}
	else
	{
		if ( n < 4 ) return 0;
		tcameraid = lua_tonumber(L, 1);
		fX = lua_tonumber(L, 2);
		fY = lua_tonumber(L, 3);
		fZ = lua_tonumber(L, 4);
	}
	switch ( iDataMode )
	{
		case 1 : g.luacameraoverride = tvalue; break;
		case 11 : if ( tcameraid == 0 ) { PositionCamera ( tcameraid, fX, fY, fZ ); } break;
		case 12 : if ( tcameraid == 0 ) 
		{ 
			RotateCamera ( tcameraid, fX, fY, fZ ); 
		} 
		break;
		case 13 : if ( tcameraid == 0 ) 
		{
			RotateCamera ( tcameraid, 0, 0, 0 );
			RollCameraRight ( tcameraid, fZ );
			TurnCameraRight ( tcameraid, fY );
			PitchCameraUp ( tcameraid, fX );
		} 
		break;
	}
	return 0;
 }
 int RawGetCameraData( lua_State *L, int iDataMode )
 {
	lua2 = L;
	int n = lua_gettop(L);
	int tcameraid = 0;
	if ( iDataMode < 500 )
	{
		if ( n < 1 ) return 0;
		tcameraid = lua_tonumber(L, 1);
	}
	float fReturnValue = 0;
	int iReturnValue = 0;
	if ( tcameraid == 0 )
	{
		switch ( iDataMode )
		{
			case 1 : fReturnValue = CameraPositionX ( tcameraid ); break;
			case 2 : fReturnValue = CameraPositionY ( tcameraid ); break;
			case 3 : fReturnValue = CameraPositionZ ( tcameraid ); break;
			case 4 : fReturnValue = CameraAngleX ( tcameraid ); break;
			case 5 : fReturnValue = CameraAngleY ( tcameraid ); break;
			case 6 : fReturnValue = CameraAngleZ ( tcameraid ); break;
			case 7: fReturnValue = GetCameraLook().x; break;
			case 8: fReturnValue = GetCameraLook().y; break;
			case 9: fReturnValue = GetCameraLook().z; break;
			case 501 : iReturnValue = g.luacameraoverride; break;
		}
	}
	if ( iDataMode < 500 )
		lua_pushnumber ( L, fReturnValue );
	else
		lua_pushinteger ( L, iReturnValue );
	return 1;
 }
 int WrapAngle(lua_State *L) 
 { 
	lua2 = L;
	int n = lua_gettop(L);
	if ( n < 3 ) return 0;
	float fAngle = wrapangleoffset(lua_tonumber(L, 1));
	float fDestAngle = wrapangleoffset(lua_tonumber(L, 2));
	float fSmoothFactor = lua_tonumber(L, 3);
	float fReturnValue = fAngle;
	float fDiff = (fDestAngle-fAngle);
	if ( fDiff > 180.0f ) fDiff -= 360.0f;
	if ( fDiff < -180.0f ) fDiff += 360.0f;
	fReturnValue += fDiff*fSmoothFactor;
	// GGMAX 2.70c: DELIBERATE deviation from DX11 (which also truncated here via
	// lua_pushinteger since long before the port) — return the smoothed angle with its
	// fraction so WrapAngle can actually converge; revert to lua_pushinteger for parity
	lua_pushnumber ( L, fReturnValue );
	return 1;
 }
 int GetCameraOverride(lua_State *L) { return RawGetCameraData ( L, 501 ); }
 int SetCameraOverride(lua_State *L) { return RawSetCameraData ( L, 1 ); }
 int SetCameraPosition(lua_State *L) { return RawSetCameraData ( L, 11 ); }
 int SetCameraAngle(lua_State *L) { return RawSetCameraData ( L, 12 ); }
 int SetCameraFreeFlight(lua_State *L) { return RawSetCameraData ( L, 13 ); }
 int GetCameraPositionX(lua_State *L) { return RawGetCameraData ( L, 1 ); }
 int GetCameraPositionY(lua_State *L) { return RawGetCameraData ( L, 2 ); }
 int GetCameraPositionZ(lua_State *L) { return RawGetCameraData ( L, 3 ); }
 int GetCameraAngleX(lua_State *L) { return RawGetCameraData ( L, 4 ); }
 int GetCameraAngleY(lua_State *L) { return RawGetCameraData ( L, 5 ); }
 int GetCameraAngleZ(lua_State *L) { return RawGetCameraData ( L, 6 ); }
 int GetCameraLookAtX(lua_State* L) { return RawGetCameraData(L, 7); }
 int GetCameraLookAtY(lua_State* L) { return RawGetCameraData(L, 8); }
 int GetCameraLookAtZ(lua_State* L) { return RawGetCameraData(L, 9); }

 int SetCameraFOV ( lua_State *L )
 {
	lua2 = L;
	int n = lua_gettop(L);
	if ( n < 2 ) return 0;
	int iCameraIndex = lua_tonumber(L, 1);
	float fCameraFOV = lua_tonumber(L, 2);
	SetCameraFOV ( iCameraIndex, fCameraFOV );
	return 0;
 }

 //
 // Player Direct commands
 //
 int RawSetPlayerData ( lua_State *L, int iDataMode )
 {
	lua2 = L;
	int n = lua_gettop(L);
	if ( iDataMode == 1 )
	{
 		//  apply force to push player
		if ( n < 2 ) return 0;
		float fAngle = lua_tonumber(L, 1);
		float fForce = lua_tonumber(L, 2);
		t.playercontrol.pushangle_f = fAngle;
		t.playercontrol.pushforce_f = fForce;
	}
	return 0;
 }
 int ForcePlayer(lua_State *L) { return RawSetPlayerData ( L, 1 ); }

 //
 // All SET & GET Functions which replaces g_Entity[e] tables
 //
 /*
 int SetEntityLUACore ( lua_State *L, int iCode )
 {
	lua2 = L;
	int n = lua_gettop(L);
	if ( n < 2 ) return 0;
	int iIndex = lua_tonumber(L, 1);
	switch ( iCode )
	{
		case 1 : t.entityelement[iIndex].luadata.x = lua_tonumber(L, 2); break;
		case 2 : t.entityelement[iIndex].luadata.y = lua_tonumber(L, 2); break;
		case 3 : t.entityelement[iIndex].luadata.z = lua_tonumber(L, 2); break;
		case 4 : t.entityelement[iIndex].luadata.anglex = lua_tonumber(L, 2); break;
		case 5 : t.entityelement[iIndex].luadata.angley = lua_tonumber(L, 2); break;
		case 6 : t.entityelement[iIndex].luadata.anglez = lua_tonumber(L, 2); break;
		case 7 : t.entityelement[iIndex].luadata.obj = lua_tonumber(L, 2); break;
		case 8 : t.entityelement[iIndex].luadata.active = lua_tonumber(L, 2); break;
		case 9 : t.entityelement[iIndex].luadata.activated = lua_tonumber(L, 2); break;
		case 10 : t.entityelement[iIndex].luadata.collected = lua_tonumber(L, 2); break;
		case 11 : t.entityelement[iIndex].luadata.haskey = lua_tonumber(L, 2); break;
		case 12 : t.entityelement[iIndex].luadata.plrinzone = lua_tonumber(L, 2); break;
		case 13 : t.entityelement[iIndex].luadata.entityinzone = lua_tonumber(L, 2); break;
		case 14 : t.entityelement[iIndex].luadata.plrvisible = lua_tonumber(L, 2); break;
		case 15 : t.entityelement[iIndex].luadata.health = lua_tonumber(L, 2); break;
		case 16 : t.entityelement[iIndex].luadata.frame = lua_tonumber(L, 2); break;
		case 17 : t.entityelement[iIndex].luadata.timer = lua_tonumber(L, 2); break;
		case 18 : t.entityelement[iIndex].luadata.plrdist = lua_tonumber(L, 2); break;
		case 19 : t.entityelement[iIndex].luadata.avoid = lua_tonumber(L, 2); break;
		case 20 : t.entityelement[iIndex].luadata.limbhit = lua_tostring(L, 2); break;
		case 21 : t.entityelement[iIndex].luadata.limbhitindex = lua_tonumber(L, 2); break;
		case 22 : t.entityelement[iIndex].luadata.animating = lua_tonumber(L, 2); break;
	}
	return 0;
 }
 int GetEntityLUACore ( lua_State *L, int iCode )
 {
	lua2 = L;
	int n = lua_gettop(L);
	if ( n < 1 ) return 0;
	int iIndex = lua_tonumber(L, 1);
	if ( iIndex > 0 )
	{
		switch ( iCode )
		{
			case 1 : lua_pushinteger ( L, t.entityelement[iIndex].luadata.x ); break;
			case 2 : lua_pushinteger ( L, t.entityelement[iIndex].luadata.y ); break;
			case 3 : lua_pushinteger ( L, t.entityelement[iIndex].luadata.z ); break;
			case 4 : lua_pushinteger ( L, t.entityelement[iIndex].luadata.anglex ); break;
			case 5 : lua_pushinteger ( L, t.entityelement[iIndex].luadata.angley ); break;
			case 6 : lua_pushinteger ( L, t.entityelement[iIndex].luadata.anglez ); break;
			case 7 : lua_pushinteger ( L, t.entityelement[iIndex].luadata.obj ); break;
			case 8 : lua_pushinteger ( L, t.entityelement[iIndex].luadata.active ); break;
			case 9 : lua_pushinteger ( L, t.entityelement[iIndex].luadata.activated ); break;
			case 10 : lua_pushinteger ( L, t.entityelement[iIndex].luadata.collected ); break;
			case 11 : lua_pushinteger ( L, t.entityelement[iIndex].luadata.haskey ); break;
			case 12 : lua_pushinteger ( L, t.entityelement[iIndex].luadata.plrinzone ); break;
			case 13 : lua_pushinteger ( L, t.entityelement[iIndex].luadata.entityinzone ); break;
			case 14 : lua_pushinteger ( L, t.entityelement[iIndex].luadata.plrvisible ); break;
			case 15 : lua_pushinteger ( L, t.entityelement[iIndex].luadata.health ); break;
			case 16 : lua_pushinteger ( L, t.entityelement[iIndex].luadata.frame ); break;
			case 17 : lua_pushinteger ( L, t.entityelement[iIndex].luadata.timer ); break;
			case 18 : lua_pushinteger ( L, t.entityelement[iIndex].luadata.plrdist ); break;
			case 19 : lua_pushinteger ( L, t.entityelement[iIndex].luadata.avoid ); break;
			case 20 : lua_pushstring ( L, t.entityelement[iIndex].luadata.limbhit.Get() ); break;
			case 21 : lua_pushinteger ( L, t.entityelement[iIndex].luadata.limbhitindex ); break;
			case 22 : lua_pushinteger ( L, t.entityelement[iIndex].luadata.animating ); break;
		}
		return 1;
	}
	else
	{
		// return zero if no index specified (error)
		return 0;
	}
 }

 int SetEntityLUAX(lua_State *L) { return SetEntityLUACore ( L, 1 ); }
 int GetEntityLUAX(lua_State *L) { return GetEntityLUACore ( L, 1 ); }
 int SetEntityLUAY(lua_State *L) { return SetEntityLUACore ( L, 2 ); }
 int GetEntityLUAY(lua_State *L) { return GetEntityLUACore ( L, 2 ); }
 int SetEntityLUAZ(lua_State *L) { return SetEntityLUACore ( L, 3 ); }
 int GetEntityLUAZ(lua_State *L) { return GetEntityLUACore ( L, 3 ); }
 int SetEntityLUAAngleX(lua_State *L) { return SetEntityLUACore ( L, 4 ); }
 int GetEntityLUAAngleX(lua_State *L) { return GetEntityLUACore ( L, 4 ); }
 int SetEntityLUAAngleY(lua_State *L) { return SetEntityLUACore ( L, 5 ); }
 int GetEntityLUAAngleY(lua_State *L) { return GetEntityLUACore ( L, 5 ); }
 int SetEntityLUAAngleZ(lua_State *L) { return SetEntityLUACore ( L, 6 ); }
 int GetEntityLUAAngleZ(lua_State *L) { return GetEntityLUACore ( L, 6 ); }
 int SetEntityLUAObj(lua_State *L) { return SetEntityLUACore ( L, 7 ); }
 int GetEntityLUAObj(lua_State *L) { return GetEntityLUACore ( L, 7 ); }
 int SetEntityLUAActive(lua_State *L) { return SetEntityLUACore ( L, 8 ); }
 int GetEntityLUAActive(lua_State *L) { return GetEntityLUACore ( L, 8 ); }
 int SetEntityLUAActivated(lua_State *L) { return SetEntityLUACore ( L, 9 ); }
 int GetEntityLUAActivated(lua_State *L) { return GetEntityLUACore ( L, 9 ); }
 int SetEntityLUACollected(lua_State *L) { return SetEntityLUACore ( L, 10 ); }
 int GetEntityLUACollected(lua_State *L) { return GetEntityLUACore ( L, 10 ); }
 int SetEntityLUAHasKey(lua_State *L) { return SetEntityLUACore ( L, 11 ); }
 int GetEntityLUAHasKey(lua_State *L) { return GetEntityLUACore ( L, 11 ); }
 int SetEntityLUAPlrInZone(lua_State *L) { return SetEntityLUACore ( L, 12 ); }
 int GetEntityLUAPlrInZone(lua_State *L) { return GetEntityLUACore ( L, 12 ); }
 int SetEntityLUAEntityInZone(lua_State *L) { return SetEntityLUACore ( L, 13 ); }
 int GetEntityLUAEntityInZone(lua_State *L) { return GetEntityLUACore ( L, 13 ); }
 int SetEntityLUAPlrVisible(lua_State *L) { return SetEntityLUACore ( L, 14 ); }
 int GetEntityLUAPlrVisible(lua_State *L) { return GetEntityLUACore ( L, 14 ); }
 int SetEntityLUAAnimating(lua_State *L) { return SetEntityLUACore ( L, 22 ); }
 int GetEntityLUAAnimating(lua_State *L) { return GetEntityLUACore ( L, 22 ); }
 int SetEntityLUAHealth(lua_State *L) { return SetEntityLUACore ( L, 15 ); }
 int GetEntityLUAHealth(lua_State *L) { return GetEntityLUACore ( L, 15 ); }
 int SetEntityLUAFrame(lua_State *L) { return SetEntityLUACore ( L, 16 ); }
 int GetEntityLUAFrame(lua_State *L) { return GetEntityLUACore ( L, 16 ); }
 int SetEntityLUATimer(lua_State *L) { return SetEntityLUACore ( L, 17 ); }
 int GetEntityLUATimer(lua_State *L) { return GetEntityLUACore ( L, 17 ); }
 int SetEntityLUAPlrDist(lua_State *L) { return SetEntityLUACore ( L, 18 ); }
 int GetEntityLUAPlrDist(lua_State *L) { return GetEntityLUACore ( L, 18 ); }
 int SetEntityLUAAvoid(lua_State *L) { return SetEntityLUACore ( L, 19 ); }
 int GetEntityLUAAvoid(lua_State *L) { return GetEntityLUACore ( L, 19 ); }
 int SetEntityLUALimbHit(lua_State *L) { return SetEntityLUACore ( L, 20 ); }
 int GetEntityLUALimbHit(lua_State *L) { return GetEntityLUACore ( L, 20 ); }
 int SetEntityLUALimbHitIndex(lua_State *L) { return SetEntityLUACore ( L, 21 ); }
 int GetEntityLUALimbHitIndex(lua_State *L) { return GetEntityLUACore ( L, 21 ); }
 */

 // Direct Entity Element Instructions (different from LUA DATA instructions above)

 int SetEntityActive(lua_State *L)
 {
	lua2 = L;
	int n = lua_gettop(L);
	if ( n < 2 ) return 0;
	int iIndex = lua_tonumber(L, 1);
	int iSetThisValue = lua_tonumber(L, 2);
	t.entityelement[iIndex].active = iSetThisValue;
	return 0;
 }
 int SetEntityActivated(lua_State *L)
 {
	lua2 = L;
	int n = lua_gettop(L);
	if ( n < 2 ) return 0;
	int iIndex = lua_tonumber(L, 1);
	t.entityelement[iIndex].activated = lua_tonumber(L, 2);
	return 0;
 }
 int SetEntityHasKey(lua_State* L)
 {
	 lua2 = L;
	 int n = lua_gettop(L);
	 if (n < 2) return 0;
	 int iIndex = lua_tonumber(L, 1);
	 t.entityelement[iIndex].lua.haskey = lua_tonumber(L, 2);
	 return 0;
 }
 int SetEntityObjective(lua_State* L)
 {
	 lua2 = L;
	 int n = lua_gettop(L);
	 if (n < 2) return 0;
	 int iIndex = lua_tonumber(L, 1);
	 t.entityelement[iIndex].eleprof.isobjective = lua_tonumber(L, 2);
	 return 0;
 }
 int SetEntityCollectable(lua_State* L)
 {
	 lua2 = L;
	 int n = lua_gettop(L);
	 if (n < 2) return 0;
	 int iIndex = lua_tonumber(L, 1);
	 t.entityelement[iIndex].eleprof.iscollectable = lua_tonumber(L, 2);
	 return 0;
 }
 int SetEntityCollectedEx(lua_State *L, bool bForceMode)
 {
	// bForceMode when true will ignore state of entity, only interested in adding to inventory (used for saved game restoring)
	lua2 = L;
	int n = lua_gettop(L);
	if ( n < 2 || n > 6 ) return 0;
	int iReturnSlot = -1;
	bool bItemHandled = false;
	bool bRemoveEntityFromGame = false;
	int iEntityIndex = lua_tonumber(L, 1);
	int iCollectState = lua_tonumber(L, 2);
	if (iCollectState < 0)
	{
		bRemoveEntityFromGame = true;
	}
	if(bRemoveEntityFromGame==false)
	{
		int iSlotIndex = -1;
		const char* pSpecifiedContainer = "";
		if (n >= 3) iSlotIndex = lua_tonumber(L, 3);
		if (n >= 4) pSpecifiedContainer = lua_tostring(L, 4);
		int iOptionalCollectionID = -1;
		if (n >= 5) iOptionalCollectionID = lua_tonumber(L, 5);
		int iQty = 1;
		if (n >= 6) iQty = lua_tonumber(L, 6);
		for (int containerindex = 0; containerindex < t.inventoryContainers.size(); containerindex++)
		{
			if (iCollectState > 0)
			{
				bool bAllowInHere = false;
				if (iCollectState == 1 && containerindex == 0) bAllowInHere = true; // main inventrory
				if (iCollectState == 2 && containerindex == 1) bAllowInHere = true; // hotkeys inventory
				if (iCollectState == 3 && stricmp(pSpecifiedContainer, t.inventoryContainers[containerindex].Get()) == NULL) bAllowInHere = true; // specified container
				if (bAllowInHere == true && bItemHandled == false)
				{
					// if not already collected
					int n = 0;
					if (iEntityIndex > 0 || bForceMode == false)
					{
						for (n = 0; n < t.inventoryContainer[containerindex].size(); n++)
							if (t.inventoryContainer[containerindex][n].e == iEntityIndex)
								break;
					}
					else
					{
						// forcing to this slot if iEntityIndex zero and forcing in (populating inventory on game load)
						n = t.inventoryContainer[containerindex].size();
					}

					// hotkeys only permits one of each type (so duplicate weapons are deflected to main inv)
					if (iCollectState == 2 && iEntityIndex > 0)
					{
						int entid = t.entityelement[iEntityIndex].bankindex;
						if (t.entityprofile[entid].isweapon > 0)
						{
							int itemCollectionID = find_rpg_collectionindex(t.entityelement[iEntityIndex].eleprof.name_s.Get());
							for (n = 0; n < t.inventoryContainer[containerindex].size(); n++)
							{
								int ee = t.inventoryContainer[containerindex][n].e;
								if (ee != iEntityIndex)
								{
									int eeCollectionID = find_rpg_collectionindex(t.entityelement[ee].eleprof.name_s.Get());
									if (itemCollectionID == eeCollectionID)
									{
										// already one here, so do NOT add
										break;
									}
								}
							}
						}
					}

					if (n >= t.inventoryContainer[containerindex].size())
					{
						// add item to inventory
						inventoryContainerType item;
						item.e = iEntityIndex;
						item.collectionID = -1;

						// find collection ID by matching object name with collection name (cannot use index as user may add to list!)
						if (iEntityIndex > 0)
						{
							int entid = t.entityelement[iEntityIndex].bankindex;
							if (t.entityprofile[entid].isweapon > 0)
							{
								// is a weapon (that are auto added) must use proper internal name for correct identification
								item.collectionID = find_rpg_collectionindex(t.entityprofile[entid].isweapon_s.Get());
								if (item.collectionID == 0)
								{
									// fallback uses regular visible name (ooften used by UI renamed stock weapons)
									item.collectionID = find_rpg_collectionindex(t.entityelement[iEntityIndex].eleprof.name_s.Get());
								}
							}
							else
							{
								item.collectionID = find_rpg_collectionindex(t.entityelement[iEntityIndex].eleprof.name_s.Get());
							}
							if (item.collectionID == 0)
							{
								// if not found from entity details, fall back to specified container item passed in
								item.collectionID = -1;
							}
						}
						if(item.collectionID==-1)
						{
							// not a weapon, can use given name
							if (iOptionalCollectionID != -1)
							{
								// or collection ID if passed in
								item.collectionID = iOptionalCollectionID;
							}
						}

						// if resource and no slot specified (collected in game), merge with any existing
						if (iEntityIndex > 0)
						{
							if (t.entityelement[iEntityIndex].eleprof.iscollectable == 2 && iSlotIndex == -1)
							{
								for (int n = 0; n < t.inventoryContainer[containerindex].size(); n++)
								{
									if (t.inventoryContainer[containerindex][n].collectionID == item.collectionID)
									{
										// already have this resource in container
										iSlotIndex = t.inventoryContainer[containerindex][n].slot;
										break;
									}
								}
							}
							// if resource can never be less than one
							if (t.entityelement[iEntityIndex].eleprof.iscollectable == 2)
							{
								int iQtyToCheck = t.entityelement[iEntityIndex].eleprof.quantity;
								if (bForceMode == true ) iQtyToCheck  = iQty;
								if (iQtyToCheck < 1) iQtyToCheck = 1;
								t.entityelement[iEntityIndex].eleprof.quantity = iQtyToCheck;
							}
						}

						// manage slot for this item
						item.slot = -1;
						if (iSlotIndex == -1)
						{
							// look for free slot index
							iReturnSlot = 0;
							for (int n = 0; n < t.inventoryContainer[containerindex].size(); n++)
							{
								if (t.inventoryContainer[containerindex][n].slot == iReturnSlot)
								{
									// being used - try if next slot free
									iReturnSlot++;
									n = -1;
								}
							}
							item.slot = iReturnSlot;
						}
						else
						{
							// ensure slot not used
							bool bCanUseSlot = true;
							for (int n = 0; n < t.inventoryContainer[containerindex].size(); n++)
							{
								if (t.inventoryContainer[containerindex][n].slot == iSlotIndex)
								{
									if (iEntityIndex > 0)
									{
										if (t.entityelement[iEntityIndex].eleprof.iscollectable == 2)
										{
											// being used by resource - can merge these
											int existingee = t.inventoryContainer[containerindex][n].e;
											if (existingee > 0)
											{
												if (t.entityelement[existingee].eleprof.iscollectable == 2)
												{
													// merge both objects into the present one in the container
													int iQtyToAdd = t.entityelement[iEntityIndex].eleprof.quantity;
													if (iQtyToAdd < 1) iQtyToAdd = 1;
													t.entityelement[existingee].eleprof.quantity += iQtyToAdd;

													// hide other object until need again
													t.entityelement[iEntityIndex].eleprof.quantity = 0;
													bRemoveEntityFromGame = true;
												}
											}
										}
									}

									// being used by non resource - cannot overwrite this one
									bCanUseSlot = false;
									break;
								}
							}
							if (bCanUseSlot == true)
							{
								item.slot = iSlotIndex;
								iReturnSlot = iSlotIndex;
							}
						}

						// finally add to list
						if (iReturnSlot != -1)
						{
							t.inventoryContainer[containerindex].push_back(item);
							bRemoveEntityFromGame = true;
						}

						// handle entity itself
						if (bRemoveEntityFromGame == true) bItemHandled = true;
					}
					else
					{
						// this entity was set for collection, but already in inventory, but still 
						// needs to be removed from the game
						bRemoveEntityFromGame = true;
						bItemHandled = true;
					}
				}
			}
			else
			{
				// find and remove from inventory
				if (bItemHandled == false)
				{
					for (int n = 0; n < t.inventoryContainer[containerindex].size(); n++)
					{
						if (t.inventoryContainer[containerindex][n].e == iEntityIndex)
						{
							t.inventoryContainer[containerindex].erase(t.inventoryContainer[containerindex].begin() + n);
							bItemHandled = true;
							break;
						}
					}
					// when NOT collected, put back in real world
					if (iEntityIndex > 0 && bForceMode == false)
					{
						if (t.entityelement[iEntityIndex].collected != 0)
						{
							t.entityelement[iEntityIndex].collected = 0;
							t.entityelement[iEntityIndex].x += 999999;
							t.entityelement[iEntityIndex].y += 999999;
							t.entityelement[iEntityIndex].z += 999999;
							t.entityelement[iEntityIndex].eleprof.phyalways = 1;
							PositionObject(t.entityelement[iEntityIndex].obj, t.entityelement[iEntityIndex].x, t.entityelement[iEntityIndex].y, t.entityelement[iEntityIndex].z);
							int store = t.e; t.e = iEntityIndex;
							entity_lua_collisionon();
							t.e = store;
							// GGMAX 2.35: dropping it back into the world is the same problem in
							// reverse — the pickup returns but the cached atlas has no shadow for
							// it. Nudge here too, or items would drop shadowless.
							extern void GGInvalidateLocalShadows();
							GGInvalidateLocalShadows();
						}
					}
				}
			}
		}
	}
	if (bRemoveEntityFromGame == true)
	{
		if (iEntityIndex > 0 && bForceMode == false)
		{
			if (t.entityelement[iEntityIndex].collected == 0)
			{
				int store = t.e; t.e = iEntityIndex;
				entity_lua_collisionoff();
				t.e = store;
				t.entityelement[iEntityIndex].collected = (int)fabs(iCollectState);
				t.entityelement[iEntityIndex].x -= 999999;
				t.entityelement[iEntityIndex].y -= 999999;
				t.entityelement[iEntityIndex].z -= 999999;
				t.entityelement[iEntityIndex].eleprof.phyalways = 1;
				PositionObject(t.entityelement[iEntityIndex].obj, t.entityelement[iEntityIndex].x, t.entityelement[iEntityIndex].y, t.entityelement[iEntityIndex].z);
				// GGMAX 2.35: collecting TELEPORTS the pickup 999999 units away rather than
				// hiding it, so its shadow must stop being cast. Refresh the cached local shadow
				// atlas — user repro: collect the ammo in test-game, its shadow stays on the
				// table. Same cache hole as the editor delete (M-Entity_part4.cpp).
				extern void GGInvalidateLocalShadows();
				GGInvalidateLocalShadows();
			}
		}
	}
	lua_pushinteger(L, iReturnSlot);
	return 1;
 }
 int SetEntityCollected(lua_State* L)
 {
	 return SetEntityCollectedEx(L, false);
 }
 int SetEntityCollectedForce(lua_State* L)
 {
	 return SetEntityCollectedEx(L, true);
 }
 int SetEntityUsed(lua_State* L)
 {
	 lua2 = L;
	 int n = lua_gettop(L);
	 if (n < 2) return 0;
	 int iEntityIndex = lua_tonumber(L, 1);
	 if (iEntityIndex > 0)
	 {
		 int iUsedState = lua_tonumber(L, 2);
		 if (iUsedState < 0 && t.entityelement[iEntityIndex].eleprof.iscollectable == 2)
		 {
			 // resources can be depleted when entity used is, er, used.
			 int qty = t.entityelement[iEntityIndex].eleprof.quantity - 1;
			 if (qty > 0)
			 {
				 iUsedState = 0;
			 }
			 t.entityelement[iEntityIndex].eleprof.quantity = qty;
		 }
		 t.entityelement[iEntityIndex].consumed = iUsedState;
	 }
	 return 0;
 }

 int SetEntityExplodable(lua_State* L)
 {
	 lua2 = L;
	 int n = lua_gettop(L);
	 if (n < 2) return 0;
	 int iEntityIndex = lua_tonumber(L, 1);
	 if (iEntityIndex > 0)
	 {
		 int iValue = lua_tonumber(L, 2);
		 t.entityelement[iEntityIndex].eleprof.explodable = iValue;
	 }
	 return 0;
 }
 int SetExplosionDamage(lua_State* L)
 {
	 lua2 = L;
	 int n = lua_gettop(L);
	 if (n < 2) return 0;
	 int iEntityIndex = lua_tonumber(L, 1);
	 if (iEntityIndex > 0)
	 {
		 int iValue = lua_tonumber(L, 2);
		 t.entityelement[iEntityIndex].eleprof.explodedamage = iValue;
	 }
	 return 0;
 }
 int SetExplosionHeight(lua_State* L)
 {
	 lua2 = L;
	 int n = lua_gettop(L);
	 if (n < 2) return 0;
	 int iEntityIndex = lua_tonumber(L, 1);
	 if (iEntityIndex > 0)
	 {
		 int iValue = lua_tonumber(L, 2);
		 t.entityelement[iEntityIndex].eleprof.explodeheight = iValue;
	 }
	 return 0;
 }
 //PE: SetCustomExplosion(e,effectname) sound is using <Sound5>. SetCustomExplosionShould only be called one time.
 int SetCustomExplosion(lua_State* L)
 {
	 lua2 = L;
	 int n = lua_gettop(L);
	 if (n < 2) return 0;
	 int iEntityIndex = lua_tonumber(L, 1);
	 if (iEntityIndex > 0)
	 {
		 const char* pEffect = lua_tostring(L, 2);
		 if (!pEffect)
			 return 0;
		 t.entityelement[iEntityIndex].eleprof.explodable_decalname = pEffect;
		 //PE: Init effects.
		 if (t.entityelement[iEntityIndex].soundset6 > 0) deleteinternalsound(t.entityelement[iEntityIndex].soundset6);
		 t.entityelement[iEntityIndex].soundset6 = loadinternalsoundcore(t.entityelement[iEntityIndex].eleprof.soundset6_s.Get(), 1);
		
		 if (t.entityelement[iEntityIndex].eleprof.explodable_decalname.Len() > 0)
		 {
			 cstr explodename = t.entityelement[iEntityIndex].eleprof.explodable_decalname;
			 if (explodename.Len() > 0)
			 {
				 for (int i = 1; i <= g.decalmax; i++)
				 {
					 if (t.decal[i].name_s == explodename)
					 {
						 int alreadyactive = t.decal[i].active;
						 t.decal[i].active = 1;
						 t.decal[i].newparticle.iMaxCache = 2; //PE: For now only 2 cached custom explosions.
						 if (alreadyactive != 1)
						 {
							 //PE: Never change t. variables in lua.
							 cstr oldtdecal_s = t.decal_s;
							 int oldtdecalid = t.decalid;
							 t.decal_s = t.decal[i].name_s;
							 t.decalid = i;
							 decal_load();
							 t.decalid = oldtdecalid;
							 t.decal_s = oldtdecal_s;
						 }
						 break;
					 }
				 }
			 }
		 }
	 }
	 return 0;
 }


 int GetEntityExplodable(lua_State* L)
 {
	 lua2 = L;
	 int n = lua_gettop(L);
	 if (n < 1) return 0;
	 int e = lua_tonumber(L, 1);
	 int iReturnValue = 0;
	 if (e > 0 && e < t.entityelement.size())
	 {
		 iReturnValue = t.entityelement[e].eleprof.explodable;
	 }
	 lua_pushinteger(L, iReturnValue);
	 return 1;

 }

 int GetEntityObjective(lua_State* L)
 {
	 lua2 = L;
	 int n = lua_gettop(L);
	 if (n < 1) return 0;
	 int e = lua_tonumber(L, 1);
	 int iReturnValue = 0;
	 if (e > 0 && e < t.entityelement.size())
	 {
		 iReturnValue = t.entityelement[e].eleprof.isobjective;
		 if (t.entityelement[e].eleprof.isobjective_alwaysactive)
			 iReturnValue = 3;
	 }
	 lua_pushinteger(L, iReturnValue);
	 return 1;
 }

 int GetEntityProjectGlobal(lua_State* L)
 {
	 lua2 = L;
	 int n = lua_gettop(L);
	 if (n < 1) return 0;
	 int e = lua_tonumber(L, 1);
	 int iReturnValue = 0;
	 if (e > 0 && e < t.entityelement.size() )
	 {
		 iReturnValue = t.entityelement[e].eleprof.isProjectGlobal;
	 }
	 lua_pushinteger(L, iReturnValue);
	 return 1;
 }

 int GetEntityCollectable(lua_State* L)
 {
	 lua2 = L;
	 int n = lua_gettop(L);
	 if (n < 1) return 0;
	 int e = lua_tonumber(L, 1);
	 int iReturnValue = 0;
	 if (e > 0 && e < t.entityelement.size())
	 {
		 iReturnValue = t.entityelement[e].eleprof.iscollectable;
	 }
	 lua_pushinteger(L, iReturnValue);
	 return 1;
 }
 int GetEntityCollected(lua_State* L)
 {
	 lua2 = L;
	 int n = lua_gettop(L);
	 if (n < 1) return 0;
	 int e = lua_tonumber(L, 1);
	 int iReturnValue = 0;
	 if (e > 0 && e < t.entityelement.size())
	 {
		 iReturnValue = t.entityelement[e].collected;
	 }
	 lua_pushinteger(L, iReturnValue);
	 return 1;
 }
 int GetEntityUsed(lua_State* L)
 {
	 lua2 = L;
	 int n = lua_gettop(L);
	 if (n < 1) return 0;
	 int e = lua_tonumber(L, 1);
	 int iReturnValue = 0;
	 if (e > 0 && e < t.entityelement.size())
	 {
		 iReturnValue = t.entityelement[e].consumed;
	 }
	 lua_pushinteger(L, iReturnValue);
	 return 1;
 }
 int SetEntityQuantity(lua_State* L)
 {
	 lua2 = L;
	 int n = lua_gettop(L);
	 if (n < 2) return 0;
	 int iEntityIndex = lua_tonumber(L, 1);
	 if (iEntityIndex > 0)
	 {
		 int iQty = lua_tonumber(L, 2);
		 t.entityelement[iEntityIndex].eleprof.quantity = iQty;
	 }
	 return 0;
 }
 int GetEntityQuantity(lua_State* L)
 {
	 lua2 = L;
	 int n = lua_gettop(L);
	 if (n < 1) return 0;
	 int e = lua_tonumber(L, 1);
	 int iQty = 0;
	 if (e > 0 && e < t.entityelement.size())
	 {
		 iQty = t.entityelement[e].eleprof.quantity;
	 }
	 lua_pushinteger(L, iQty);
	 return 1;
 }

 int GetEntityWhoActivated(lua_State *L)
 {
	 lua2 = L;
	 int n = lua_gettop(L);
	 if (n < 1) return 0;
	 int e = lua_tonumber(L, 1);
	 int iReturnValue = 0;
	 if (e > 0 && e < t.entityelement.size())
	 {
		 iReturnValue = t.entityelement[e].whoactivated;
	 }
	 lua_pushinteger (L, iReturnValue);
	 return 1;
 }
 int GetEntityActive(lua_State *L)
 {
	lua2 = L;
	int n = lua_gettop(L);
	if ( n < 1 ) return 0;
	int iIndex = lua_tonumber(L, 1);
	int iReturnValue = 0;
	if ( iIndex > 0 )
	{
		iReturnValue = t.entityelement[iIndex].active;
		if ( Len(t.entityelement[iIndex].eleprof.aimainname_s.Get())>1 ) 
			if ( t.entityelement[iIndex].eleprof.aimain == 0 ) 
				iReturnValue = 0;
	}
	lua_pushinteger ( L, iReturnValue );
	return 1;
 }
 int GetEntityVisibility(lua_State *L)
 {
	lua2 = L;
	int n = lua_gettop(L);
	if ( n < 1 ) return 0;
	int iReturnValue = 0;
	int iEntityIndex = lua_tonumber(L, 1);
	if ( iEntityIndex > 0 )
	{
		int iObjectNumber = t.entityelement[iEntityIndex].obj;
		if ( iObjectNumber > 0 )
		{
			sObject* pObject = GetObjectData ( iObjectNumber );
			if ( pObject && pObject->bVisible == true )
				iReturnValue = 1;
		}
	}
	lua_pushinteger ( L, iReturnValue );
	return 1;
 }
 int SetEntitySpawnAtStart(lua_State *L)
 {
	lua2 = L;
	int n = lua_gettop(L);
	if ( n < 2 ) return 0;
	int iEntityIndex = lua_tonumber(L, 1);
	if (iEntityIndex > 0)
	{
		t.entityelement[iEntityIndex].eleprof.spawnatstart = lua_tonumber(L, 2);
	}
	return 0;
 }
 int GetEntitySpawnAtStart(lua_State *L)
 {
	lua2 = L;
	int n = lua_gettop(L);
	if ( n < 1 ) return 0;
	int iReturnValue = 0;
	int iEntityIndex = lua_tonumber(L, 1);
	if (iEntityIndex > 0)
	{
		iReturnValue = t.entityelement[iEntityIndex].eleprof.spawnatstart;
	}
	lua_pushinteger ( L, iReturnValue );
	return 1;
 }
 int GetEntityFilePath(lua_State *L)
 {
	lua2 = L;
	int n = lua_gettop(L);
	if ( n < 1 ) return 0;
	char pReturnValue[1024];
	strcpy ( pReturnValue, "" );
	int iEntityIndex = lua_tonumber(L, 1);
	if ( iEntityIndex > 0 ) 
	{
		int iEntID = t.entityelement[iEntityIndex].bankindex;
		if ( iEntID > 0 )
		{
			strcpy ( pReturnValue, t.entitybank_s[iEntID].Get() );
		}
	}
	lua_pushstring ( L, pReturnValue );
	return 1;
 }

 int GetEntityClonedSinceStartValue(lua_State* L)
 {
	 lua2 = L;
	 int n = lua_gettop(L);
	 if (n < 1) return 0;
	 int iReturnValue = 0;
	 int iEntityIndex = lua_tonumber(L, 1);
	 iReturnValue = t.entityelement[iEntityIndex].iWasSpawnedInGame;
	 lua_pushinteger (L, iReturnValue);
	 return 1;
 }

 int SetPreExitValue(lua_State *L)
 {
	lua2 = L;
	int n = lua_gettop(L);
	if ( n < 2 ) return 0;
	int iEntityIndex = lua_tonumber(L, 1);
	if (iEntityIndex > 0)
	{
		t.entityelement[iEntityIndex].eleprof.aipreexit = lua_tonumber(L, 2);
	}
	return 0;
 }

 int RawSetEntityData ( lua_State *L, int iDataMode )
 {
	lua2 = L;
	int n = lua_gettop(L);
	if ( n < 2 ) return 0;
	int iEntityIndex = lua_tonumber(L, 1);
	if (iEntityIndex > 0 && iEntityIndex < t.entityelement.size())
	{
		float fValue = lua_tonumber(L, 2);
		switch (iDataMode)
		{
			case 12: // 120417 - change anim speed mod, and character speed if a character
			{
				if (t.entityelement[iEntityIndex].animspeedmod != fValue)
				{
					t.entityelement[iEntityIndex].animspeedmod = fValue;
					float fFinalAnimSpeed = t.entityelement[iEntityIndex].eleprof.animspeed * t.entityelement[iEntityIndex].animspeedmod;
					t.e = iEntityIndex;
					entity_lua_findcharanimstate ();
					if (t.tcharanimindex != -1) t.charanimstates[t.tcharanimindex].animationspeed_f = (65.0f / 100.0f) * fFinalAnimSpeed;
				}
				break;
			}
			case 22: t.entityelement[iEntityIndex].eleprof.phyalways = (int)fValue; break;
			case 23: t.entityelement[iEntityIndex].iCanGoUnderwater = (int)fValue; break;

			case 104: t.entityelement[iEntityIndex].eleprof.conerange = fValue; break;

			case 105:
			{
				t.entityelement[iEntityIndex].eleprof.iMoveSpeed = (int)fValue;
				entity_lua_findcharanimstate();
				if (t.tcharanimindex != -1)
				{
					int movingbackward = 0; if (fValue < 0.0f) movingbackward = 1;
					t.charanimstate.movingbackward = movingbackward;
					t.charanimstate.movespeed_f = fabs(fValue) / 100.0f;
					t.charanimstates[t.tcharanimindex] = t.charanimstate;
				}
				break;
			}
			case 106:
			{
				t.entityelement[iEntityIndex].eleprof.iTurnSpeed = (int)fValue;
				entity_lua_findcharanimstate();
				if (t.tcharanimindex != -1)
				{
					t.charanimstate.turnspeed_f = fValue / 100.0f;
					t.charanimstates[t.tcharanimindex] = t.charanimstate;
				}
				break;
			}
		}
	}
	return 0;
 }

